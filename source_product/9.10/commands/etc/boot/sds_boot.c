/* @(#) $Header: sds_boot.c,v 70.2 93/11/23 11:02:27 ssa Exp $ */

#include <sys/types.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/scsi.h>
#include <sys/diskio.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/conf.h>
#include <sys/sysmacros.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "lifdef.h"
#include "lifglobal.h"
#include "saio.h"
#include "sds.h"

#   define LIFSIZE	    8192
#   define LIFSECTORSIZE     256

#ifdef __hp9000s300
#define MAXPHYS (64 * 1024)
#endif

extern unsigned int current_msus;

struct Volume sds_boot_volume;
struct Volume *v = &sds_boot_volume;

/******************************* sds_log2() **********************************/
/*                                                                           */
/* This routine returns the log base 2 of the specified number.              */
/*                                                                           */
/*****************************************************************************/

unsigned sds_log2(i)
unsigned i;
{
  unsigned l = 0;

  while (i >>= 1) l++;

  return l;
}

/******************************* sds_open() **********************************/
/*                                                                           */
/* This routine opens the specified SDS array volume.                        */
/*                                                                           */
/*****************************************************************************/

int sds_open()
{
  int i;
  unsigned int bad;
  int error;
  int partition;
  sds_info_t array;
  sds_section_t *section;

  /* then fill in the SDS array device number and the physical device */
  /* count (used if we have to bail out early). */
  v->volmsus = current_msus;
  v->deviceCount = 0;
  partition = 1;

  /* then read the SDS_INFO array configuration data from it */
  sds_get_info(&array);

  /* and then find the appropriate SDS partition information */
  section = &array.section[partition-1];

  /* then make sure the requested SDS partition exists */
  if (section->size == 0) _stop("sds_open: section size = 0");

  /* then open the individual physical devices in the SDS array */
  for (i = 0; i < array.ndevs; i++) {
    v->msus[i] = array.msus[i];
    v->io[i]->msus = v->msus[i];
    v->deviceCount++;
  }

  /* then fill in the Volume label and type information */
  strcpy(v->label, array.label);
  strcpy(v->type, array.type);

  /* and the Volume stripe size information */
  v->stripeSize = (array.ndevs == 1)?(MAXPHYS):(section->stripesize);
  v->log2StripeSize = sds_log2(v->stripeSize);

  /* and offset information */
  v->physOffset = section->start;

  /* set the Volume open count to 0 */
  v->openCount = 0;

  return 0;
}


/******************************* sds_stripe() ********************************/
/*                                                                           */
/* This routine is the heart of the SDS driver.  Given a Volume structure    */
/* and an unstriped buf, it performs one stripe's worth of I/O at the        */
/* specified disk offset and buffer address for up to the specified length.  */
/* It then returns the number of bytes included in the stripe.               */
/*                                                                           */
/*****************************************************************************/

unsigned sds_stripe(offset, length, addr)
unsigned offset;
unsigned length;
char *addr;
{
  struct iob *io;
  unsigned bcount;
  unsigned stripeFirst;
  unsigned stripeLast;

  /* compute the numbers of the stripes containing the beginning and */
  /* end of the specified I/O request */
  stripeFirst = offset >> v->log2StripeSize;
  stripeLast = (offset+length-1) >> v->log2StripeSize;

  /* if the specified I/O request length is bigger than one stripe, */
  /* then prune it back to one stripe. */
  if (stripeFirst != stripeLast) {
    bcount = ((stripeFirst+1) << v->log2StripeSize) - offset;
  } else {
    bcount = length;
  }

  io = v->io[stripeFirst % v->deviceCount];
  io->i_offset = v->physOffset +
			(stripeFirst/v->deviceCount << v->log2StripeSize) +
			(offset & (v->stripeSize-1));
  io->i_bn = (unsigned)io->i_offset >> DEV_BSHIFT;
  io->i_cc = bcount;
  io->i_ma = addr;
  io->i_offset = 0;

  /* and return the number of bytes it included */
  return devread(io);
}



/******************************* sds_close ***********************************/
/*                                                                           */
/* This routine closes the specified SDS array volume.                       */
/*                                                                           */
/*****************************************************************************/

int sds_close(io)
struct iob *io;
{
  int i;

  /* close all devices in the SDS array which we had opened */
  for (i = 0; i < v->deviceCount; i++) {
    close(v->io[i]);
  }

  return 0;
}


/******************************* sds_strategy() ******************************/
/*                                                                           */
/* This routine performs the specified I/O to an SDS array volume.           */
/*                                                                           */
/*****************************************************************************/

void sds_read(io)
struct iob *io;
{
  unsigned stripe;
  unsigned length;
  unsigned offset;
  unsigned partial;
  unsigned stripes;
  char *addr;

  /* get the basic I/O info */
  length = io->i_cc;
  offset = io->i_bn << DEV_BSHIFT;
  addr = io->i_ma;

  /* while there are still more stripes to process...  process the */
  /* next stripe of the I/O request */
  while (length) {
    stripe = sds_stripe(offset, length, addr);

    offset += stripe;
    addr += stripe;
    length -= stripe;
  }
}


/*
 * sds_get_file() --
 *    Get the SDS_INFO file from the given device.  The data is copied
 *    to the buffer 'fbuf' (which must be big enough to hold the entire
 *    file).
 *
 *    Returns an error condition (defined in sds.h) on error.
 *    Otherwise, returns SDS_OK.
 */
int
sds_get_file(volnum, fbuf)
int volnum;
char *fbuf;
{
    struct lvol *lifvol;
    struct dentry *lifdirp;
    struct dentry *lifdire;
    u_long start;
    u_long size = 0;
    struct iob *io;
    register int cc, bn;

    io = v->io[volnum];

    io->i_offset = 0;
    io->i_bn = 0;
    io->i_cc = LIFSIZE;
    io->i_ma = io->i_buf;
    cc = devread(io);
    if (cc < 0)
        _stop("sds_get_file: lif read failed");

    lifvol = (struct lvol *)io->i_buf;
    if (lifvol->discid != LIFID)
        _stop("sds_get_file: bad lif id");

    lifdirp = (struct dentry *)(io->i_buf + lifvol->dstart * LIFSECTORSIZE);
    lifdire = (struct dentry *)(io->i_buf +
	(lifvol->dstart + lifvol->dsize) * LIFSECTORSIZE);
    if ((char *)lifdirp < io->i_buf ||
        (char *)lifdirp > (io->i_buf + LIFSIZE - DESIZE) ||
	(char *)lifdire < ((char *)lifdirp + DESIZE) ||
	(char *)lifdire > (io->i_buf + LIFSIZE - DESIZE))
        _stop("sds_get_file: not lif volume");

    while (lifdirp < lifdire && lifdirp->ftype != EOD)
    {
	if (lifdirp->ftype != PURGED &&
	    strncmp(lifdirp->fname, SDS_INFO, 10) == 0)
	{
	    start = lifdirp->start * LIFSECTORSIZE;
	    size  = lifdirp->size * LIFSECTORSIZE;
	    break;
	}
	lifdirp++;
    }

    if (size == 0)
        _stop("sds_get_file: size = 0");

    if (size > SDS_MAXINFO)
        _stop("sds_get_file: too big");

    io->i_offset = 0;
    io->i_bn = start / DEV_BSIZE;
    io->i_cc = size;
    io->i_ma = io->i_buf;
    cc = devread(io);
    if (cc < 0)
        _stop("sds_get_file: lif file read failed");

    bcopy(io->i_buf, fbuf, size);
    return;
}

/*
 * sds_skip_comment() --
 *     skip comments and white space in an SDS description
 *     file.
 */
char *
sds_skip_comment(bp, pline)
register char *bp;
int *pline;
{
    while (*bp != '\0')
    {
	if (*bp == ' ' || *bp == '\t')
	    bp++;
	else if (*bp == '\n')
	{
		bp++;
		(*pline)++;
	}
	else if (*bp == '#')
	{
	    while (*bp && *bp != '\n')
		bp++;
	}
	else
	    return bp;
    }
    return bp;
}

/*
 * sds_word()
 *     return a pointer to the next word in '*pbuf', updating '*pbuf'
 *     to point to the end of the word.  The word is null terminated.
 */
char *
sds_word(pbuf, pline)
char **pbuf;
int *pline;
{
    char *s;
    char *t;

    s = t = sds_skip_comment(*pbuf, pline);
    while (*t != '\0' && *t != ' ' && *t != '\t' && *t != '\n')
	t++;
    if (*t == '\n')
	(*pline)++;

    *t++ = '\0';
    *pbuf = t;
    return s;
}

/*
 * sds_cvt_num() --
 *     convert a string in base 8, 10 or 16 to its numeric value.
 *     The string may optionally end in k, m or G (K M or G) to
 *     indicate units of kbytes, megabytes or gigabytes.
 *     Returns 0 on success, -1 on error.
 */
int
sds_cvt_num(s, pval)
char *s;
u_long *pval;
{
    u_int base = 10;
    u_long val = 0;
    u_long prev_val;

    if (*s == '\0') /* a null string is not a valid number */
	return -1;

    if (*s == '0')
    {
	s++;
	if (*s == 'x')
	{
	    s++;
	    if (*s == '\0') /* "0x" is not a valid number */
		return -1;
	    base = 16;
	}
	else
	    base = 8;
    }

    while (*s)
    {
	prev_val = val;

	if ((*s == 'k' || *s == 'K') && *(s+1) == '\0')
	    val *= 1024;
	else if ((*s == 'm' || *s == 'M') && *(s+1) == '\0')
	    val *= (1024 * 1024);
	else if ((*s == 'g' || *s == 'G') && *(s+1) == '\0')
	    val *= (1024 * 1024 * 1024);
	else
	{
	    val *= base;

	    if (*s >= '0' && *s <= '9')
	    {
		if (*s > '7' && base == 8)
		    return -1;
		val += (*s - '0');
	    }
	    else if (*s >= 'a' && *s <= 'f' && base == 16)
		val += (*s - 'a' + 10);
	    else if (*s >= 'A' && *s <= 'F' && base == 16)
		val += (*s - 'A' + 10);
	    else
		return -1;
	}
	s++;

	/*
	 * check for overflow...
	 */
	if (val < prev_val)
	    return -1;
    }
    *pval = val;
    return 0;
}

/*
 * kw_table[] --
 *    A table to map keyword strings to numeric values.  The
 *    strings are (roughly) ordered by their frequency of use.
 */
static struct {
    char *kw;		/* The keyword */
    int  id;		/* The corresponding numeric id */
} kw_table[] =
{
    "device",	  SDS_KW_DEVICE,
    "partition",  SDS_KW_SECTION,
    "start",	  SDS_KW_START,
    "size",	  SDS_KW_SIZE,
    "stripesize", SDS_KW_STRIPE,
    "disk",	  SDS_KW_DISK,
    "label",	  SDS_KW_LABEL,
    "unique",	  SDS_KW_UNIQUE,
    "type",	  SDS_KW_TYPE,
    (char *)0,	  SDS_KW_EOF,
    (char *)-1,	  SDS_KW_ERROR
};

/*
 * sds_keyword() --
 *     Return a keyword identifier for the next token in the input.
 */
int
sds_keyword(pbuf, pline, badstr)
char **pbuf;
int *pline;
char **badstr;
{
    char *s = sds_word(pbuf, pline);
    int i;

    /*
     * Save this token in case of an error.
     */
    *badstr = s;

    /*
     * Search the table.  Even EOF is handled by the table, since
     * sds_word() returns the empty string ("\0") on EOF.
     */
    for (i = 0; kw_table[i].kw != (char *)-1; i++)
    {
	if (strcmp(kw_table[i].kw, s) == 0)
	    return kw_table[i].id;
    }
    return SDS_KW_ERROR;
}

/*
 * sds_parse_raw() --
 *    parse the data in an SDS data file.
 *    This routine parses the data that is stored in the LIF file on
 *    the disk.
 */
int
sds_parse_raw(sds, buf)
sds_info_t *sds;
char *buf;
{
    int line;
    char *str;
    int kw;
    u_long partition;
    u_long n;
    int dev_index = 0;

    /*
     * Initialize the fields that we may fill in to 0.  We leave
     * other fields (like msus[] unchanged).
     */
    bzero(sds->label, sizeof sds->label);
    bzero(sds->type, sizeof sds->type);
    sds->which_disk = 0;
    bzero(sds->section, sizeof sds->section);

    partition = 0;
    for (;;)
    {
	kw = sds_keyword(&buf, &line, &str);
	str = sds_word(&buf, &line);

	switch (kw)
	{
	case SDS_KW_DISK:
	    if (sds_cvt_num(str, &n) == -1 || n == 0 || n > SDS_MAXDEVS)
		return -1;
	    sds->which_disk = n-1;
	    break;

	case SDS_KW_DEVICE:
	    if (sds_cvt_num(str, &n) == -1)
		return -1;
	    sds->msus[dev_index++] = devtomsus(n);
	    break;

	case SDS_KW_LABEL:
	    if (strlen(str) > SDS_MAXNAME)
		return -1;
	    strcpy(sds->label, str);
	    break;

	case SDS_KW_TYPE:
	    if (strlen(str) > SDS_MAXNAME)
		return -1;
	    strcpy(sds->type, str);
	    break;

	case SDS_KW_UNIQUE:
	    if (sds_cvt_num(str, &n) == -1)
		return -1;
	    sds->unique = n;
	    break;

	case SDS_KW_SECTION:
	    if (sds_cvt_num(str, &partition) == -1 ||
		partition == 0 || partition > SDS_MAXSECTS)
		return -1;
	    partition--;	/* 0..n-1, not 1..n */
	    break;

	case SDS_KW_START:
	    if (sds_cvt_num(str, &n) == -1)
		return -1;
	    sds->section[partition].start = n;
	    break;

	case SDS_KW_STRIPE:
	    if (sds_cvt_num(str, &n) == -1)
		return -1;
	    sds->section[partition].stripesize = n;
	    break;

	case SDS_KW_SIZE:
	    if (sds_cvt_num(str, &n) == -1)
		return -1;
	    sds->section[partition].size = n;
	    break;

	case SDS_KW_EOF:
	    if (dev_index == 0)
		sds->ndevs = 1;
	    else
		sds->ndevs = dev_index;
	    return SDS_OK;

	default:
	    return -1;
	}
    }
    /* NOTREACHED */
}

/*
 * sds_get_info() --
 *    given a block disk device, read the SDS description from the
 *    LIF header, verifying the data on all members of the array.
 *    The devs[] array is re-ordered if necessary to match reality.
 *
 *    Returns SDS_OK if all members of the array are present.
 *    Otherwise, the appropriate error value is returned.
 *
 *    If "pbad_dev" is non-null, it will be set to the dev_t of the
 *    first device that is incorrect.
 */
int
sds_get_info(sds)
sds_info_t *sds;
{
    int err;
    char *buf;
    int i;
    int primary;
    unsigned int new_msus[SDS_MAXDEVS];

    for (i = 0; i < SDS_MAXDEVS; i++)
	new_msus[i] = 0;

    v->io[0] = &iob[alloc_io()];
    buf = v->io[0]->i_buf;
    v->io[0]->msus = current_msus;
    sds_get_file(0, buf);

    /*
     * Set the "unique" value to something so that if the SDS data
     * does not include a "unique" specification, multiple disk
     * arrays will not work.
     */
    sds->unique = (u_long)current_msus;
    if ((err = sds_parse_raw(sds, buf)) != SDS_OK)
        _stop("sds_get_info: parse of lead device failed");

    /*
     * The kernel can not allow the user to access a section of an
     * array except through the "lead" device.  The "lead" device
     * is "disk1" of an array.
     * This is necessary so that things like mount(2) recognize that
     * a device is already mounted, etc.  I.e. we can only have a
     * single device handle to get to any device, multiple ones screw
     * things up.
     */
    if (sds->which_disk != 0)
        _stop("sds_get_info: not lead device");

    new_msus[sds->which_disk] = current_msus;

    /*
     * For the simple array that has only one disk, we do not need
     * the device information to be correct.
     */
    if (sds->ndevs == 1)
    {
	sds->msus[0] = current_msus;
	return;
    }

    /*
     * Make sure that one of the devices in the sds_info is this
     * device.
     */
    for (i = 0; i < sds->ndevs; i++)
    {
	if (sds->msus[i] == current_msus)
	{
	    primary = i;
	    break;
	}
    }

    if (i >= sds->ndevs)
        _stop("sds_get_info: bad primary device");

    /*
     * Ensure that the SDS information on the other members of the
     * array match the primary member.
     */
    for (i = 0; i < sds->ndevs; i++)
    {
	int j;
	sds_info_t sds2;

	if (i == primary)
	    continue;

	v->io[i] = &iob[alloc_io()];
	v->io[i]->msus = sds->msus[i];
	sds_get_file(i, buf);

	/*
	 * Compare the data in this SDS structure to the primary
	 * SDS structure.
	 */
	sds2.unique = 0;
	if ((err = sds_parse_raw(&sds2, buf)) != SDS_OK)
            _stop("sds_get_info: parse failed");

	if (sds->unique != sds2.unique ||
	    strcmp(sds->label, sds2.label) != 0 ||
	    strcmp(sds->type, sds2.type) != 0)
            _stop("sds_get_info: not member");

	for (j = 0; j < SDS_MAXSECTS; j++)
	{
	    if (sds->section[j].size	  != sds2.section[j].size ||
		sds->section[j].start	  != sds2.section[j].start ||
		sds->section[j].stripesize!= sds2.section[j].stripesize)
                _stop("sds_get_info: bad section");
	}

	if (new_msus[sds2.which_disk] != 0)
	    _stop("sds_get_info: dup member");

	new_msus[sds2.which_disk] = sds->msus[i];
    }

    /*
     * Update the SDS devs[] array with the actual configuration
     * of the disks.  This allows the members of the array to be
     * connected in an arbitrary configuration, as long as there
     * is a disk of the array on each device in the array.
     */
    for (i = 0; i < SDS_MAXDEVS; i++)
	sds->msus[i] = new_msus[i];

    return;
}
