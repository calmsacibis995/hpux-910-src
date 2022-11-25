/* @(#) $Header: sds_parse.c,v 70.5 91/10/15 17:01:44 ssa Exp $ */

#ifdef _KERNEL
#   include "../h/types.h"
#   include "../h/param.h"
#   include "../h/stat.h"
#   include "../h/vnode.h"
#   include "../ufs/inode.h"
#   include "../h/lifdef.h"
#   include "../h/lifglobal.h"
#   include "../h/file.h"
#   include "../h/buf.h"
#   include "../h/malloc.h"
#   include "sds.h"
#else
#   include <stdio.h>
#   include <sys/types.h>
#   include <sys/param.h>
#   include <sys/inode.h>
#   include <sys/stat.h>
#   include <sys/fcntl.h>
#   include "sds.h"
#   include "sds_user.h"
#endif /* _KERNEL */

/*
 * MAX_LIFIO is the maximum data that we will ever read from the
 * LIF volume.  We take the max of the LIF directory size and the
 * size of the SDS_INFO file.  We then add 2 * DEV_BSIZE so that we
 * can round a request down to a DEV_BSIZE boundary and up to a
 * DEV_BSIZE boundary (necessary for block device access).
 */
#define MAX_LIFIO  \
    ((LIFSIZE > SDS_MAXINFO ? LIFSIZE : SDS_MAXINFO) + (2 * DEV_BSIZE))

/*
 * lif_state --
 *    a structure for maintaining state information about a LIF
 *    i/o operation.  This structure is different for the user-land
 *    version and the kernel version.  It is used only by the low-level
 *    LIF i/o routines.  The callers of these routines simply must
 *    supply a state structure to be filled in/used.
 */
#ifndef _KERNEL
/*
 * user space version.  We need is a file descriptor and a
 * buffer to store the data that we read.
 */
struct lif_state
{
    int  fd;			/* open file descriptor */
    char buf[MAX_LIFIO];	/* buffer for performing I/O */
};
#else
/*
 * kernel space version.  We need the dev_vp pointer (the kernel
 * equivelant of an open fd) and a buf pointer which points to the
 * data that we have read.
 */
struct lif_state
{
    struct vnode *dev_vp;	/* handle to the open device */
    struct buf *bp;   		/* buffer for performing I/O */
};
#endif /* ! _KERNEL */

typedef struct lif_state lif_state_t;

#ifndef _KERNEL

#define MALLOC(x, y, z, a, b)	(x = (y)malloc(z))
#define FREE(x, y)		free(x)
#define bzero(x, y)		memset(x, '\0', y)
#define bcopy(x, y, z)		memcpy(y, x, z)

/*
 * sds_lif_open() -- User land version
 *    "open" the given device in preparation for reading the LIF
 *    information from it.  For user land, we simply search our
 *    files[] array for the given dev, then store the corresponding
 *    'fd' in the global variable 'fd'.
 *
 *    Returns 0 on success, -1 on error (should not happen in the
 *    user-land version, only in the kernel version).
 */
static int
sds_lif_open(dev, lif_state)
dev_t dev;
lif_state_t *lif_state;
{
    int i;

    if (files[TMP_FILE].dev == dev)
    {
	lif_state->fd = files[TMP_FILE].fd;
	return 0;
    }

    for (i = 0; i < n_files; i++)
    {
	if (files[i].dev == dev)
	{
	    lif_state->fd = files[i].fd;
	    return 0;
	}
    }

    lif_state->fd = -1;
    return -1;
}

/*
 * sds_lif_close() -- User land version
 *    "close" the given device after done getting the LIF information
 *    from it.  For user land, we just set the global 'fd' to -1.
 *    We really do not want to close the device, as the rest of the
 *    program will still use it, and we only have a reference to it,
 *    not a seperate open file descriptor.
 */
static void
sds_lif_close(dev, lif_state)
dev_t dev;
lif_state_t *lif_state;
{
    lif_state->fd = -1;
    return; /* we do not really want to close the file */
}

#else

/*
 * sds_lif_open() -- Kernel version
 *    "open" the given device in preparation for reading the LIF
 *    information from it.  For kernel land, we really do try to open
 *    the given device (must be block special).  We then store the
 *    result of devtovp(dev) in the global 'dev_vp', which is used to
 *    access the device.
 *
 *    Returns 0 on success, -1 on error (can happen in the kernel
 *    version).
 */
static int
sds_lif_open(dev, lif_state)
dev_t dev;
lif_state_t *lif_state;
{
    lif_state->bp = (struct buf *)0;

#ifdef FULLDUX
#ifdef HPNSE
    if (opend(&dev, IFBLK | IF_MI_DEV, my_site, FREAD, 0, 0, 0))
#else
    if (opend(&dev, IFBLK | IF_MI_DEV, my_site, FREAD, 0))
#endif
#else  /* not FULLDUX */
#ifdef HPNSE
    if (opend(&dev, IFBLK | IF_MI_DEV, FREAD, 0, 0, 0))
#else
    if (opend(&dev, IFBLK | IF_MI_DEV, FREAD, 0))
#endif
#endif /* FULLDUX */
    {
	lif_state->dev_vp = (struct vnode *)0;
        return -1;
    }

    lif_state->dev_vp = devtovp(dev);
    return 0;
}

/*
 * sds_lif_close() -- Kernel land version
 *    "close" the given device after done getting the LIF information
 *    from it.  For kernel land, we first call brelse() on the global
 *    bp (if non-NULL) to release any buffer that we may be holding
 *    onto.  We then call closed() to close the given device.
 */
static void
sds_lif_close(dev, lif_state)
dev_t dev;
lif_state_t *lif_state;
{
    if (lif_state->bp != (struct buf *)0)
    {
	brelse(lif_state->bp);
	lif_state->bp = (struct buf *)0;
    }

#ifdef FULLDUX
#ifdef HPNSE
    (void) closed(dev, IFBLK, FREAD, my_site, 0);
#else
    (void) closed(dev, IFBLK, FREAD, my_site);
#endif
#else
#ifdef HPNSE
    (void) closed(dev, IFBLK, FREAD, 0);
#else
    (void) closed(dev, IFBLK, FREAD);
#endif
#endif /* FULLDUX */

    lif_state->dev_vp = (struct vnode *)0;
}
#endif /* _KERNEL */

/*
 * sds_lif_read() --
 *    read 'size' amount of data from 'offset' from the device that
 *    was previously opened with sds_lif_open().  A pointer to the
 *    requested data is returned.  This data area should be considered
 *    to be kept in a 'static' buffer (which is true for the user-land
 *    version, but the data for the kernel version is stored in a
 *    file system buffer.
 *
 *    Since we must perform all I/O on DEV_BSIZE boundaries, we
 *    truncate the offset to a DEV_BSIZE boundary.  We then must
 *    round the size up so that the resulting request ends on a
 *    DEV_BSIZE boundary.  Consequently, (user-land only) the static
 *    buffer must have 2*DEV_BSIZE more bytes than the largest
 *    possible I/O request.
 *
 *    Returns (char *)0 on error, otherwise a pointer to the requested
 *    data.
 */
static char *
sds_lif_read(offset, size, lif_state)
u_long offset;
u_long size;
lif_state_t *lif_state;
{
    u_long rem, srem;

    /*
     * Round the offset down to the nearest DEV_BSIZE boundary.
     */
    rem = offset % DEV_BSIZE;
    if (rem)
    {
	offset -= rem;
	size += rem;
    }

    /*
     * Round the size up to the nearest DEV_BSIZE boundary.
     */
    srem = size % DEV_BSIZE;
    if (srem)
	size += (DEV_BSIZE - srem);

    if (size > MAX_LIFIO)
	return (char *)0;

#ifndef _KERNEL
    if (lseek(lif_state->fd, offset, SEEK_SET) != offset)
	return (char *)0;

    if (read(lif_state->fd, lif_state->buf, size) != size)
	return (char *)0;

    return lif_state->buf + rem;
#else
    if (lif_state->bp)
	brelse(lif_state->bp);
    lif_state->bp =
	bread(lif_state->dev_vp, offset / DEV_BSIZE, size, B_unknown);
    lif_state->bp->b_flags |= B_NOCACHE;
    if (lif_state->bp->b_flags & B_ERROR)
    {
	brelse(lif_state->bp);
	lif_state->bp = (struct buf *)0;
	return (char *)0;
    }

    return lif_state->bp->b_un.b_addr + rem;
#endif /* _KERNEL */
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
sds_get_file(dev, fbuf)
dev_t dev;
char *fbuf;
{
    char *buf;
    struct lvol *lifvol;
    struct dentry *lifdirp;
    struct dentry *lifdire;
    u_long start;
    u_long size = 0;
    lif_state_t lif_state;

    if (sds_lif_open(dev, &lif_state) != 0)
	return SDS_OPENFAILED;

    if ((buf = sds_lif_read(0, LIFSIZE, &lif_state)) == (char *)0)
    {
	sds_lif_close(dev, &lif_state);
	return SDS_OPENFAILED;
    }

    lifvol = (struct lvol *)buf;
    if (lifvol->discid != LIFID)
    {
	sds_lif_close(dev, &lif_state);
	return SDS_NOLIF;
    }

    lifdirp = (struct dentry *)(buf + lifvol->dstart * LIFSECTORSIZE);
    lifdire = (struct dentry *)(buf +
	(lifvol->dstart + lifvol->dsize) * LIFSECTORSIZE);
    if ((char *)lifdirp < buf ||
        (char *)lifdirp > (buf + LIFSIZE - DESIZE) ||
	(char *)lifdire < ((char *)lifdirp + DESIZE) ||
	(char *)lifdire > (buf + LIFSIZE - DESIZE))
    {
	sds_lif_close(dev, &lif_state);
	return SDS_NOLIF;
    }

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
    {
	sds_lif_close(dev, &lif_state);
	return SDS_NOTARRAY;
    }

    if (size > SDS_MAXINFO ||
        (buf = sds_lif_read(start, size, &lif_state)) == (char *)0)
    {
	sds_lif_close(dev, &lif_state);
	return SDS_NOTARRAY;
    }

    bcopy(buf, fbuf, size);
    sds_lif_close(dev, &lif_state);

    return SDS_OK;
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
	if ((*s == 'k' || *s == 'K') && *(s+1) == '\0')
	{
	    if (val >= (4 * 1024 * 1024))
		return -1; /* result would overflow */
	    val *= 1024;
	}
	else if ((*s == 'm' || *s == 'M') && *(s+1) == '\0')
	{
	    if (val >= (4 * 1024))
		return -1; /* result would overflow */
	    val *= (1024 * 1024);
	}
	else if ((*s == 'g' || *s == 'G') && *(s+1) == '\0')
	{
	    if (val >= 4)
		return -1; /* result would overflow */
	    val *= (1024 * 1024 * 1024);
	}
	else
	{
	    prev_val = val;
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

	    /*
	     * check for overflow...
	     */
	    if (val < prev_val)
		return -1;
	}
	s++;

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
#ifndef _KERNEL
    "reserve",	  SDS_KW_RESERVE,
#endif /* _KERNEL */
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
     * other fields (like dev[] unchanged).
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
	    sds->devs[dev_index++] = n;
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

#ifdef _KERNEL

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
sds_get_info(dev, sds, pbad_dev)
dev_t dev;
sds_info_t *sds;
dev_t *pbad_dev;
{
    int err;
    char *buf;
    char *fbuf;
    int i;
    int primary;
    dev_t dummy_dev;
    dev_t new_devs[SDS_MAXDEVS];

    /*
     * Allocate 'fbuf'.
     */
    MALLOC(fbuf, char *, SDS_MAXINFO, M_IOSYS, M_ALIGN);

    /*
     * Allow "pbad_dev" to be null.
     */
    if (pbad_dev == (dev_t *)0)
	pbad_dev = &dummy_dev;

    *pbad_dev = dev; /* assume primary is bad */

    for (i = 0; i < SDS_MAXDEVS; i++)
	new_devs[i] = NODEV;

    if ((err = sds_get_file(dev, fbuf)) != SDS_OK)
	goto sds_get_info_return;

    buf = fbuf;

    /*
     * Set the "unique" value to something so that if the SDS data
     * does not include a "unique" specification, multiple disk
     * arrays will not work.
     */
    sds->unique = (u_long)dev;
    if ((err = sds_parse_raw(sds, buf)) != SDS_OK)
	goto sds_get_info_return;

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
    {
	err = SDS_NOTPRIMARY;
	goto sds_get_info_return;
    }

    new_devs[sds->which_disk] = dev;

    /*
     * For the simple array that has only one disk, we do not need
     * the device information to be correct.
     */
    if (sds->ndevs == 1)
    {
	sds->devs[0] = dev;
	err = SDS_OK;
	goto sds_get_info_return;
    }

    /*
     * Make sure that one of the devices in the sds_info is this
     * device.
     */
    for (i = 0; i < sds->ndevs; i++)
    {
	if (sds->devs[i] == dev)
	{
	    primary = i;
	    break;
	}
    }

    if (i >= sds->ndevs)
    {
	err = SDS_BADPRIMARY;
	goto sds_get_info_return;
    }

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

	*pbad_dev = sds->devs[i]; /* in case of an error */
	if ((err = sds_get_file(sds->devs[i], fbuf)) != SDS_OK)
	    goto sds_get_info_return;

	buf = fbuf;

	/*
	 * Compare the data in this SDS structure to the primary
	 * SDS structure.
	 */
	sds2.unique = 0;
	if ((err = sds_parse_raw(&sds2, buf)) != SDS_OK)
	    goto sds_get_info_return;

	if (sds->unique != sds2.unique ||
	    strcmp(sds->label, sds2.label) != 0 ||
	    strcmp(sds->type, sds2.type) != 0)
	{
	    err = SDS_NOTMEMBER;
	    goto sds_get_info_return;
	}

	for (j = 0; j < SDS_MAXSECTS; j++)
	{
	    if (sds->section[j].size	  != sds2.section[j].size ||
		sds->section[j].start	  != sds2.section[j].start ||
		sds->section[j].stripesize!= sds2.section[j].stripesize)
	    {
		err = SDS_NOTMEMBER;
		goto sds_get_info_return;
	    }
	}

	if (new_devs[sds2.which_disk] != NODEV)
	{
	    err = SDS_DUPMEMBER;
	    goto sds_get_info_return;
	}

	new_devs[sds2.which_disk] = sds->devs[i];
    }

    /*
     * Update the SDS devs[] array with the actual configuration
     * of the disks.  This allows the members of the array to be
     * connected in an arbitrary configuration, as long as there
     * is a disk of the array on each device in the array.
     */
    for (i = 0; i < SDS_MAXDEVS; i++)
	sds->devs[i] = new_devs[i];

    err = SDS_OK;

sds_get_info_return:
    FREE(fbuf, M_IOSYS);
    return err;
}

#endif /* _KERNEL */
