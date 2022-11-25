/* @(#) $Revision: 70.9 $ */

#include <stdio.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <sys/fs.h>
#include <errno.h>
#include <sys/conf.h>
#include <fcntl.h>
#include <time.h>
#include "lifio.h"
#include "global.h"
#include <limits.h>
#include "lifdir.h"
#include <sys/diskio.h>
#ifdef	LVM
#include <lvm/pvres.h>
#include <lvm/bdres.h>
#include <sys/dsk_label.h>
#endif  LVM

#define BOOT700 "/usr/lib/uxbootlf.700"
#define BOOT800 "/usr/lib/uxbootlf"
#define BOOT300 "/etc/boot"
#define SPECDEV "/dev/root"
#define SPECRDEV "/dev/rroot"
#define RMBOOT "rmboot"

#define YES 1
#define NO 0

#define min(x,y)	((x) < (y) ? (x) : (y))
#define max(x,y)	((x) > (y) ? (x) : (y))

#ifdef	LVM
#define LVM_LIF_DIR_START 144  /* LIF directory start for LVM disks (in K)*/
#define LABEL_SECTORS	ROUNDUP(sizeof(struct dsk_label)/LIFSECTORSIZE,	\
				LIFALIGNMENT/2)
#define LVM_BOOTSECTORS	8396	/* Size of LIF volume on LVM disks */
#endif	/* LVM */
#define BOOTSECTORS	7820	/* Size of LIF volume on non-LVM disks */

#define ARCHTYPE name.machine[5]

struct preserve {
	struct dentry	dentry;
	short	dirmade;
	char	*data;
};

extern struct lif_directory *open_lifdir();	/* External references */
extern struct dentry *new_lifentry();
extern struct dentry *read_lifentry();
extern struct dentry *scan_lifdir();
extern void flush_lifdir();
extern void reset_lifdir();
extern void close_lifdir();
extern int lifstrcmp();
extern char *lifstrcpy();

void usage();			/* take care of forward references */
void install_300_boot();
void install_800_boot();
void pres_save();
struct preserve	*pres_entry();
char *inc_set();
char *make_bootable_label();
void gettime();
void update_boot();
int getsb();
int bread();
int getdisksize();
char *basename();
void clobber_boot();
int check_for_fit();
void rawname();			/* figures out raw device file name */

#define MAX_PRESERVE	8
#define MAX_INCLUDED	8
struct preserve	pres[MAX_PRESERVE+1];
char inc[MAX_INCLUDED][MAXFILENAME+1];

#ifdef LVM
struct dsk_label	label_data;
#endif /* LVM */

char *bootprogs;		/* Incoming boot info */
char rawbuf[MAXPATHLEN+1];	/* holds raw device name */
char device[MAXPATHLEN+1];	/* holds block device name */
char *userdev;			/* points to user-provided device name */
char *autostring;		/* string to put in AUTO file */
int specialdev = 0;		/* check for /dev/root or /dev/rroot */
struct utsname name;		/* use default series if not given */
struct stat statbuf;		/* saved result from device stat */
union {				/* superblock buffer */
	struct	fs sb;
	char pad[MAXBSIZE];
} sbun;
#define	sblock sbun.sb

int fi;				/* file descriptor for getsb and bread */
int fquiet = 0;			/* -F option for mkrs */

/*
 * routine to get the base name of a path
 */
char *
basename(p)
char *p;
{
    register char *s;

    if(s = strrchr(p, '/'))
    	s++;
    else
    	s = p;

    return(s);
}

/*
 * routine to read the superblock.  The following are the return values:
 * 0 ==> no I/O error
 * 1 ==> some type of I/O error (lseek or read)
 */
bread(bno, buf, cnt)
	daddr_t bno;
	char *buf;
        int cnt;
{
	register i;

	if (lseek(fi, bno * DEV_BSIZE, 0) == -1)
		return(1);
	if ((i = read(fi, buf, (unsigned)cnt)) != cnt) {
		for(i=0; i<sblock.fs_bsize; i++)
			buf[i] = 0;
		return (1);
	}
	return (0);
}

/*
 * Check to see if the new boot programs will fit on the device
 */
int
check_for_fit(fssize, swapsize, disksize, bootsize, dev, verbose, force)
int fssize, swapsize, disksize, bootsize, verbose, force;
char *dev;
{
    extern int swapinfo();		/* reads /dev/kmem */
    struct swdevt info;			/* data from /dev/kmem */
    extern int getlifvals();		/* reads the lif header */
    int lif_swapstart;			/* start of swap on lif vol */
    int lif_swapsize;			/* size of swap on lif vol */
    int lifend;				/* end of swap in lif header */
    int swapend;			/* end of swap in /dev/kmem */
    int avail;				/* determine if space is there */


    if(getlifvals(dev, &lif_swapstart, &lif_swapsize) == 0) {
	(void) fprintf(stderr, "mkboot: Unable to read the swap ");
	(void) fprintf(stderr, "information from the device header\n");
	exit(1);
    }
    lif_swapstart /= 4;		/* convert to K bytes */
    lif_swapsize /= 4;		/* convert to K bytes */

    if(swapinfo(dev, &info, force)) {		/* swap to device */
	if(info.sw_enable) {		/* swap currently active */
	    avail = disksize - (lif_swapstart + lif_swapsize);
	    if(avail >= bootsize) {
		if(verbose) {
		    (void) fprintf(stderr, "The new boot programs will fit");
		    (void) fprintf(stderr, " on the specified device.\n");
	        }
		return(0);
            }
	    else {
		if(verbose) {
		    (void) fprintf(stderr, "The new boot programs will not");
		    (void) fprintf(stderr, " fit on the specified device.\n");
	        }
		return(1);
	    }
        }
	else {				/* swap not enabled to device */
	    avail = 0;			/* assume there is no space */
	    lifend = lif_swapstart + lif_swapsize;
	    swapend = info.sw_start + info.sw_nblks;
	    /*
	     * take care of the 4 cases of no rawio, rawio before swap, rawio
	     * after swap and rawio before and after swap.
	     */

	    /*
	     * (1) no rawio
	     */
	    if((info.sw_start == lif_swapstart) &&
	       (swapend == lifend))
		    avail = disksize - fssize;

	    /*
	     * (2) rawio before swap
	     */
	    if((info.sw_start > lif_swapstart) &&
	       (swapend == lifend))
		    avail = disksize - (fssize + (info.sw_start - lif_swapstart));
	    
	    /*
	     * (3) rawio after swap
	     * (4) rawio before and after swap
	     */
	    if((info.sw_start >= lif_swapstart) &&
	       (lifend > swapend))
		    avail = disksize - (lif_swapstart + lif_swapsize);

	    if(avail >= bootsize) {
		if(verbose) {
		    (void) fprintf(stderr, "The new boot programs will fit");
		    (void) fprintf(stderr, " on the specified device.\n");
	        }
	        return(0);
	    }
	    else {
		if(verbose) {
		    (void) fprintf(stderr, "The new boot programs will not");
		    (void) fprintf(stderr, " fit on the specified device.\n");
	        }
		return(1);
	    }
        }
    }
    else {				/* swap not enabled to this device */
	avail = disksize - (lif_swapstart + lif_swapsize);
	if(avail >= bootsize) {
	    if(verbose) {
		(void) fprintf(stderr, "The new boot programs will fit");
		(void) fprintf(stderr, " on the specified device.\n");
	    }
	    return(0);
        }
	else {
	    if(verbose) {
		(void) fprintf(stderr, "The new boot programs will not");
		(void) fprintf(stderr, " fit on the specified device.\n");
	    }
	    return(1);
	}
    }
}

/*
 * routine to clobber the first 8K on device
 */
void
clobber_boot(dev)
char *dev;
{
    char buf[8192];
    int i;

    fi = open(dev, 2);
    if(fi == -1) {
	perror("open");
	(void) fprintf(stderr, "Unable to open %s\n", dev);
	usage(RMBOOT);
	exit(1);
    }

    for(i = 0; i < 8192; i++)
        buf[i] = '\0';

    if(write(fi, buf, sizeof(buf)) != sizeof(buf)) {
	perror("write");
	(void) fprintf(stderr, "Unable to remove the boot area from %s\n",dev);
    }
    exit(0);
}

    
/*
 * Determine if cp is a block or character device file.
 * If it is a character device, try to figure out the corresponding
 * block device name by removing an r where we think it might be.
 * If it is a block device, it is unmolested.
 */
void
getblockdev(cp)
char *cp;    
{
    static char blockbuf[MAXPATHLEN + 1];
    int ftype;
    char *dp = strrchr(cp, '/');

    /*
     * If the special device /dev/rroot or /dev/root was specified,
     * then everything is OK and quit
     */
    if(specialdev)
	return;

    /*
     * The full path was not specified on a character device, so it cannot
     * be converted to a block name.
     */
    if (dp == 0) {
	(void) fprintf(stderr, "The full path name for the device is ");
	(void) fprintf(stderr, "required\n");
	exit(1);
    }

    if(stat(cp, &statbuf) != 0) {
	perror("stat");
	(void) fprintf(stderr, "Unable to stat device file %s\n",cp);
	exit(1);
    }
    userdev = cp;
    ftype = statbuf.st_mode & S_IFMT;
    if(ftype == S_IFBLK)
        return;
    if(ftype != S_IFCHR) {
	(void) fprintf(stderr, "Either a block or character device is ");
	(void) fprintf(stderr, "required.\n");
	exit(1);
    }
    (void) strcpy(rawbuf, cp);		/* save the raw device name */
    userdev = rawbuf;
	
    while ( dp >= cp && *--dp != '/' );

    /*  Check for AT&T Sys V disk naming convention
     *    (i.e. /dev/rdsk/c0d0s3).
     *  If it doesn't fit the Sys V naming convention and
     *    this is a series 200/300, check for the old naming
     *    convention (i.e. /dev/hd).  
     *  If it's neither, change the name as if it's got Sys V
     *    naming.
     */
#ifdef __hp9000s300
    if ( strncmp(dp, "/rdsk", 5) ) {
	dp = strrchr(cp, '/');
	if ( strncmp(dp, "/rhd", 4) )
	    while ( dp >= cp && *--dp != '/' );
    }
#endif

    *dp = 0;
    (void)strcpy(blockbuf, cp);
    *dp = '/';
    (void)strcat(blockbuf, "/");
    (void)strcat(blockbuf, dp+2);

    if(stat(blockbuf, &statbuf) != 0) {
	perror("stat");
	(void) fprintf(stderr, "Unable to stat device file %s\n",blockbuf);
	exit(1);
    }
    ftype = statbuf.st_mode & S_IFMT;
    if(ftype != S_IFBLK) {
	(void) fprintf(stderr, "File %s is not a block special file\n",
		       blockbuf);
	exit(1);
    }
    (void) strcpy(cp, blockbuf);
    return;
}

/*
 * use popen to exec diskinfo for the work of getting the disk
 * information.
 */

int
getdisksize(dev, series)
char *dev;
int  series;
{
    int size;				/* used for disk size */
    char command[2 * MAXPATHLEN];	/* used by popen */
    FILE *fp;				/* result of popen */
    char buf[1024];			/* result of fgets for popen */
    
    rawname(dev);
    if (series == 800) {
	capacity_type	capacity;
	int		fd = open(rawbuf, O_RDONLY);
	if (fd < 0) {
	    perror("Could not open device");
	    exit(1);
	}
	if (ioctl(fd, DIOC_CAPACITY, &capacity) < 0) {
	    perror("Unable to get size information for disk");
	    exit(1);
	}
	close(fd);
	return((int)capacity.lba);
    }
    (void) sprintf(command, "%s %s","/etc/diskinfo -b",rawbuf);
    if((fp = popen(command, "r")) == NULL) {
	perror("popen");
	(void) fprintf(stderr, "Unable to execute /etc/diskinfo\n");
	exit(1);
    }
    (void) fgets(buf, 1024, fp);
    buf[strlen(buf) - 1] = '\0';
    size = atoi(buf);
    if(pclose(fp) == -1) {
	(void) fprintf(stderr, "Unable to get size information from %s\n",dev);
	exit(1);
    }
    return(size);
}




/*
 * get the superblock from the specified device file.  The return values
 * are:
 * 0 ==> no superblock
 * 1 ==> superblock present
 * 2 ==> some type of error (from open or bread)
 */

getsb(fs, file)
register struct fs *fs;
char *file;
{

	fi = open(file, O_RDONLY);
	if (fi < 0) {
	    return(2);
	}
	if (bread(SBLOCK, (char *)fs, SBSIZE)) {
	    close(fi);
	    return(2);
	}
	(void) close(fi);
#if defined(FD_FSMAGIC)
	if ((fs->fs_magic != FS_MAGIC) && (fs->fs_magic != FS_MAGIC_LFN)
		&& (fs->fs_magic != FD_FSMAGIC))
#else /* not NEW_MAGIC */
#ifdef LONGFILENAMES
	if ((fs->fs_magic != FS_MAGIC) && (fs->fs_magic != FS_MAGIC_LFN))
#else /* not LONGFILESNAMES */
	if (fs->fs_magic != FS_MAGIC)
#endif /* not LONGFILESNAMES */
#endif /* NEW_MAGIC */
	{
	    return(0);
	}
	return(1);
}


/*
 * If the series is other than 700, getsizes is not required.  Otherwise,
 * getsizes opens the device file and reads the super block to determine
 * the size of the file system.  It then executes a disk describe on the
 * device to get the disk size.  It finally gets the size of the boot
 * programs from the series type to set the value for swap size.  The
 * values are:
 * fssize = sb.fs_size
 * swapsize = disksize - fs_size - (boot_size + round_up_vlaue)
 */
void
getsizes(fssize, swapsize, disksize, bootsize, dev, series, verbose)
int *fssize, *swapsize, *disksize, *bootsize, series, verbose;
char *dev;
{
    int size;			/* temporary holder for swapsize */
    struct stat sbuf;		/* for size of uxbootlf */
    
    if(series != 700)
        return;

    /*
     * Read the superblock from device and pick off the file system
     * size.
     */
    switch(getsb(&sblock, dev)) {
    case 0:			/* super block not present */
	perror("super block");
	(void) fprintf(stderr, "There is no super block on device %s\n", dev);
	exit(1);
    case 2:			/* I/O error */
	perror("I/O error");
	(void) fprintf(stderr, "An I/O error was encountered while attempting to\n");
	(void) fprintf(stderr, "read the superblock from device %s\n", dev);
	exit(1);
    case 1:			/* found a super block */
	break;
    }
    
    *fssize = sblock.fs_size * (sblock.fs_fsize / 1024);
    if(sblock.fs_size < 0) {
	(void) fprintf(stderr, "The determined file system size is nonsense: ");
	(void) fprintf(stderr, "%d\n", sblock.fs_size);
	(void) fprintf(stderr, "mkboot exiting\n");
	exit(1);
    }

    size = getdisksize(dev, series);
    *disksize = size;
    if(size < 0) {
	(void) fprintf(stderr, "The determined size of the disk is nonsense: ");
	(void) fprintf(stderr, "%d\n", size);
	(void) fprintf(stderr, "mkboot exiting\n");
	exit(1);
    }

    if(stat(bootprogs, &sbuf) == -1) {
	perror("stat(uxbootlf)");
	(void) fprintf(stderr, "Unable to get the size of the boot programs\n");
	exit(1);
    }
    if((sbuf.st_size % 1024) != 0)
        sbuf.st_size = sbuf.st_size + (1024 - (sbuf.st_size % 1024));
    sbuf.st_size /= 1024;		/* convert to k bytes */
    if((sbuf.st_size % 2) != 0)		/* force to 2K boundary */
        sbuf.st_size = sbuf.st_size++;
    *bootsize = sbuf.st_size;

    *swapsize = (size - *fssize) - sbuf.st_size;
    if(*swapsize < 0) {
	(void) fprintf(stderr, "The computed swap size is nonsense: ");
	(void) fprintf(stderr, "%d\n", *swapsize);
	(void) fprintf(stderr, "mkboot exiting\n");
	exit(1);
    }
	
    if(verbose) {
	(void) fprintf(stdout, "mkboot: File system size:     %20d K bytes\n",
		       *fssize);
	(void) fprintf(stdout, "        Available swap space: %20d K bytes\n",
		       *swapsize);
	(void) fprintf(stderr, "        Boot program space:   %20d K bytes\n",
		       sbuf.st_size);
    }
    return;
}



/*
 * Copy the s300 boot programs to the device file.
 */
void
install_300_boot(dev, verbose)
char *dev;
int verbose;
{
	int fd;
	int devfd;
	char bootblock[MAXPATHLEN];
	char bootimage[BBSIZE];

	(void) sprintf(bootblock, "%s", bootprogs);
	if (verbose) {
		(void) fprintf(stderr, "installing boot code\n");
		(void) fprintf(stderr, "sector 0 boot = %s\n", bootblock);
	}
	devfd = open(dev, 2);
	if(devfd < 0) {
		perror("device open");
		exit(1);
	}
	fd = open(bootblock, 0);
	if (fd < 0) {
		perror("boot block open");
		exit(1);
	}
	if (read(fd, bootimage, BBSIZE) < 0) {
		perror("boot block read error");
		exit(1);
	}
	(void) close(fd);
	if (write(devfd, bootimage, BBSIZE) != BBSIZE) {
		perror("boot block write error");
		exit(1);
	}
	(void) close(devfd);
	return;
}




/*
 * This routine checks to make sure there is no file system residing on
 * this device.  If there is, a warning is issued and
 * confirmation to continue is requested.  If permission is given, the
 * boot programs are copied to the device.
 */
#ifdef	LVM
/*
 * Modified for LV Boot disk.   
 *     If the destination device is a LVM physical volume, mkboot will 
 *     fail unless the -B option was used with pvcreate. 
 *
 *     Mkboot will write the boot area skipping the LVM PVRA and BDRA
 *     disk labels.  It updates the lif header to make the ISL start,
 *     and directory locations reflect the changed size.
 */ 
#endif	/* LVM */

void
install_800_boot(tgt, disksize, included, lvm_target, preserve)
char *tgt;
int disksize;
int *included;
int lvm_target;
int preserve;
{
    struct lif_directory *srcldp, *tgtldp;
    struct dentry *srclif, *tgtlif;
    struct preserve *pe;
    int brem;
    int i;
    char buf[LIFBUFSIZE];
    char c_name[MAXFILENAME+1];
    char *cp;
    char *src = bootprogs;
    int tffd;
    int	writeloc = 0;

    switch(getsb(&sblock, tgt)) {
    case 0:				/* super block not present */
	break;
    case 1:				/* super block found */
	(void) fprintf(stderr, "There appears to be a file system ");
	(void) fprintf(stderr, "on this device.\nShould it be ");
	(void) fprintf(stderr, "overwritten [y/n]? ");
	while(1) {
	    (void) fflush(stderr);
	    (void) fgets(buf, 4, stdin);
	    if(buf[0] == 'y')
	        break;
	    if(buf[0] == 'n')
	        return;
	    (void) fprintf(stderr, "Please answer yes or no. Should the ");
	    (void) fprintf(stderr, "file system be overwritten? ");
	}
	break;
    case 2:				/* I/O error */
	perror("file system");
	(void) fprintf(stderr, "I/O error on device: %s\n", src);
	return;

    }

    /* Read the source header and directory */
    srcldp = open_lifdir(src, READONLY);

    /* Prepare to build the target header and directory */
    tgtldp = open_lifdir(tgt, APPEND);

    /*
     * If there are target LIF files to be preserved,
     * save the data somewhere.
     *
     * NO WRITES TO THE TARGET are allowed before we save this data.
     */
    if (preserve)
	pres_save(tgtldp, preserve, included, lvm_target);

    /* If the target is not a valid lif, use the source's header */
    if (!valid_lif(tgtldp->hdr)) {
	*tgtldp->hdr = *srcldp->hdr;
    }

    /* Take care to leave room for LVM data structures */
#ifdef LVM
    if (is_lvm(tgtldp->fd, tgtldp->name) || lvm_target) {
	tgtldp->hdr->dstart = ROUNDUP(LVM_LIF_DIR_START * DEV_BSIZE /
				      LIFSECTORSIZE, LIFALIGNMENT);
	tgtldp->hdr->spt = LVM_BOOTSECTORS;
    }
    else
#endif LVM
    {
	tgtldp->hdr->dstart = ROUNDUP(srcldp->hdr->dstart, LIFALIGNMENT);
	tgtldp->hdr->spt = BOOTSECTORS;
    }
    tgtldp->hdr->tps = tgtldp->hdr->spm = 1;

    /*
     * Use the source's ipllength and iplentry unless we're preserving
     * the target's ISL.  (iplstart gets set below).
     */
    if (!pres_entry(ISL, preserve)) {
	tgtldp->hdr->ipllength = srcldp->hdr->ipllength;
	tgtldp->hdr->iplentry  = srcldp->hdr->iplentry;
    }

    /*
     * Loop to create the target LIF directory from the source.
     * Take care of all files to be copied from source to target.
     */
    while (srclif = read_lifentry(srcldp)) {

	/* Convert the LIF-style name to something easier for C. */
	strncpy(c_name, srclif->fname, MAXFILENAME);
	c_name[MAXFILENAME] = '\0';
	for (i = 0; i < MAXFILENAME; i++)
	    if (c_name[i] == ' ') {
		c_name[i] = '\0';
		break;
	    }

	/*
	 * If we want only the included set of files, and this file is not
	 * in the set, skip it.
	 */
	if (*included && !inc_set(c_name, *included)) {
	    continue;
	}

	/*
	 * We actually want to use this file!  Get a blank entry for it.
	 */
	tgtlif = new_lifentry(tgtldp);

	/*
	 * If this is one of the files we want to preserve on the target,
	 * use the target's entry we saved away earlier.  Otherwise,
	 * use the source's entry.
	 */
	if (preserve && (pe = pres_entry(c_name, preserve))) {
	    *tgtlif = pe->dentry;
	    pe->dirmade = TRUE;
	}
	else
	    *tgtlif = *srclif;
    }

    /*
     * If any included file except LABEL was not found, quit.
     * If LABEL was not found, make a preserved entry for a new one.
     */
    for (i = 0; i < *included; i++) {
	if (!scan_lifdir(srcldp, inc[i])) {
#ifdef LVM
	    if (strcmp(inc[i], LABEL) == 0 &&
		(is_lvm(tgtldp->fd, tgtldp->name) || lvm_target))
	    {
		lifstrcpy(pres[preserve].dentry.fname, inc[i]);
		pres[preserve].dentry.ftype = LIFTYPE_BIN;
		pres[preserve].dentry.size  = LABEL_SECTORS;
		pres[preserve].dentry.lastvolnumber = LIF_LVN;
		gettime(&pres[preserve].dentry.date);
		pres[preserve].data = make_bootable_label(tgt, 800);
		preserve++;
	    }
	    else
#endif /* LVM */
	    {
		fprintf(stderr, "Included file %s not found\n", inc[i]);
		exit(1);
	    }
	}
    }

    /*
     * If there are preserved files that did not show up
     * in the source LIF, now's the time to make directory
     * entries for them.
     */
    for (i = 0; i < preserve; i++) {
	if (!pres[i].dirmade && *pres[i].dentry.fname && pres[i].data) {
	    tgtlif = new_lifentry(tgtldp);
	    *tgtlif = pres[i].dentry;
	    pres[i].dirmade = TRUE;
	}
    }

    /* Write out the target header and directory */
    flush_lifdir(tgtldp);

    /* Arrange to read the target directory from the start without doing IO's */
    reset_lifdir(tgtldp, READONLY);

    /* Get a separate descriptor for writing the lif files */
    tffd = Open(tgtldp->name, O_RDWR);

    /* Now copy the lif files themselves */
    while (tgtlif = read_lifentry(tgtldp)) {

	writeloc = tgtlif->start;
	Lseek(tffd, writeloc * LIFSECTORSIZE, SEEK_SET);

	/*
	 * If this is one of the files we want to preserve on the target,
	 * write back the data we read earlier.
	 */
	if (preserve && (pe = pres_entry(tgtlif->fname, preserve)) && pe->data)
	{
	    
	    int size = ROUNDUP(tgtlif->size, LIFALIGNMENT) * LIFSECTORSIZE;
	    for (brem = size, cp = pe->data;
		 brem > 0;
		 brem -= LIFBUFSIZE, cp += LIFBUFSIZE) {
		writeloc += SECTORSPERBUF;
		if (writeloc >= tgtldp->hdr->spt) {
		    fprintf(stderr, "Attempt to write past end of boot area: ");
		    fprintf(stderr, "validity of boot area unknown\n");
		    exit(1);
		}
		Write(tffd, cp, LIFBUFSIZE, tgtldp->name);
	    }
	    free(pe->data);
	    pe->data = (char *)0;
	    continue;
	}

	/* Find the file in the source lif */
	srclif = scan_lifdir(srcldp, tgtlif->fname);
	if (!srclif) {
	    fprintf(stderr, "mkboot internal error in install_800_boot()\n");
	    exit(1);
	}

	/* Copy the contents */
	Lseek(srcldp->fd, srclif->start*LIFSECTORSIZE, SEEK_SET);
	for (brem = tgtlif->size*LIFSECTORSIZE; brem > 0; brem -= LIFBUFSIZE) {
	    Read(srcldp->fd, buf, LIFBUFSIZE, srcldp->name);
	    writeloc += SECTORSPERBUF;
	    if (writeloc >= tgtldp->hdr->spt) {
		fprintf(stderr, "Attempt to write past end of boot area: ");
		fprintf(stderr, "validity of boot area unknown\n");
		exit(1);
	    }
	    Write(tffd, buf, LIFBUFSIZE, tgtldp->name); 
	}
    }

    Close(tffd);
    close_lifdir(srcldp);
    close_lifdir(tgtldp);
}


/*
 * Is this an LVM boot disk?
 *
 * This function may exit() on IO errors or if an LVM disk is detected
 * which was not pvcreate'd as a boot disk.
 */
#ifdef LVM
int
is_lvm(fd, name)
int	fd;
char	*name;
{
    int		lvmrec_addr;
    char	buf[MAKEBUF(lv_lvmrec_t)];
    lv_lvmrec_t	*lv_rec = (lv_lvmrec_t *)buf;

    /*
     * Read in the lvmrec to see if it's an LVM disk.
     */
    lvmrec_addr = PVRA_LVM_REC_SN1 * DEV_BSIZE;
    Lseek(fd, lvmrec_addr, SEEK_SET);
    Read(fd, buf, sizeof(buf), name);

    if (strncmp(lv_rec->lvm_id, LVMREC_MARK, 8) != 0)
	/* It's not an LVM disk */
	return FALSE;

    /*
     * Check to see if it is an LVM bootable disk.
     */
    if (lv_rec->vgra_psn != VGRA_PSN_BOOT) {
	char	usrbuf[8];
	fprintf(stderr, "\nDevice %s looks like an LVM physical volume ", name);
	fprintf(stderr, "which\nwas not initialized to be a boot disk.\n");
	fprintf(stderr, "Proceeding could make all existing data on %s ", name);
	fprintf(stderr, "inaccessible.\n\nDo you wish to continue? ");
	while(1) {
	    (void) fflush(stderr);
	    (void) fgets(usrbuf, 4, stdin);
	    if(usrbuf[0] == 'y' || usrbuf[0] == 'n')
	        break;
	    (void) fprintf(stderr, "Please answer yes or no.");
	    (void) fprintf(stderr, "  Do you wish to continue? ");
	}
	if (usrbuf[0] == 'n')
	    exit(1);
	return FALSE;
    }

    return TRUE;
}
#endif /* LVM */


/*
 * Preserve those LIF files on the target we don't want overwritten.
 * The target should not have been written before we get here.
 */
void
pres_save(tgtldp, preserve, included, lvm_target)
struct lif_directory	*tgtldp;
int			preserve;
int			*included;
int			lvm_target;
{
    struct dentry		*dentry;
    struct preserve		*pe;
    struct lif_directory	*ldp;
    char	buf[LIFBUFSIZE];
    char	*dp;
    char	*inc_file;
    int		brem, size;
    int		i;

    ldp = open_lifdir(tgtldp->name, READONLY);

    /* Save preserved files */
    while (dentry = read_lifentry(ldp)) {
	if ((pe = pres_entry(dentry->fname, preserve))) {

	    if (*included &&
		(inc_file = inc_set(pe->dentry.fname, *included)))
	    {
		int i;
		/*
		 * Preserved file is included also.  Remove it as included
		 * and fix up the inc array.
		 */
		fprintf(stderr, "%s: %s will be preserved on target device\n",
		       "mkboot", inc_file);
		for (i = 0; i < *included; i++)
		    if (inc_file < inc[i])
			strcpy(inc[i-1], inc[i]);
		*included = *included - 1;
	    }

	    /* This is a "preserved" file, get memory for contents */
	    size = ROUNDUP(dentry->size, LIFALIGNMENT) * LIFSECTORSIZE;
	    pe->data = malloc(size);
	    if (pe->data == (char *)0) {
		fprintf(stderr, "File %s too large to preserve:",
			dentry->fname);
		perror("");
		exit(1);
	    }

	    /* Read the file */
	    Lseek(ldp->fd, dentry->start * LIFSECTORSIZE, SEEK_SET);
	    for (brem = size, dp = pe->data;
		 brem > 0;
		 brem -= LIFBUFSIZE, dp += LIFBUFSIZE)
		Read(ldp->fd, dp, LIFBUFSIZE, ldp->name);

	    /* Remember the directory entry */
	    pe->dentry = *dentry;
	}
    }

    /*
     * Squawk and exit if a non-existent file was supposed to be preserved.
     * (We want to exit here since the error may have just been a typo.)
     */
    for (i = 0; i < preserve; i++)
	if (pres[i].data == (char *)0) {
	    /*
	     * Special cases:
	     * If the AUTO file was not found give a special exit value.	
	     * If the LABEL file was not found and this is an LVM disk,
	     * give a special exit value.
	     * If the LABEL file was not found and this is not an LVM disk,
	     * ignore the problem and continue.
	     */
	    int	exitval = 1;
	    if (lifstrcmp(pres[i].dentry.fname, LABEL) == 0) {
#ifdef LVM
		if (!is_lvm(ldp->fd, ldp->name) && !lvm_target)
		    continue;
#endif /* LVM */
		exitval = 3;
	    }

	    if (lifstrcmp(pres[i].dentry.fname, AUTO) == 0)
		exitval = 2;

	    fprintf(stderr, "LIF file %s not present on %s.\n",
		    pres[i].dentry.fname, ldp->name);
	    exit(exitval);
	}

    close_lifdir(ldp);
}


/*
 * Find a preserved files entry in the table.
 */
struct preserve *
pres_entry(name, preserve)
char	*name;
int	preserve;
{
    int	i;

    if (!*name)
	return (struct preserve *)0;

    for (i = 0; i < preserve; i++)
	if (lifstrcmp(name, pres[i].dentry.fname) == 0)
	    return &pres[i];

    return (struct preserve *)0;
}


/*
 * Is this file one of the included set?
 */
char *
inc_set(name, included)
char	*name;
int	included;
{
    int i;

    if (*name)
	for (i = 0; i < included; i++)
	    if (strcmp(name, inc[i]) == 0)
		return inc[i];

    return (char *)0;
}


/*
 * Create a minimally bootable (no swap or dump) LABEL file
 * for LVM boot disks.  Defines the root LV to start at the
 * usual place and run to the end of the disk.
 */
#ifdef LVM
char *
make_bootable_label(device, series)
char	*device;
{
    struct dsk_label	*label_data;
    int			size;

    label_data = (struct dsk_label *)malloc(MAKEBUF(*label_data));
    if (!label_data) {
	perror("Unable to create LABEL file");
	exit(1);
    }
    size = getdisksize(device, series);
    if (size <= 0) {
	(void) fprintf(stderr, "The determined size of the disk is nonsense: ");
	(void) fprintf(stderr, "%d\n", size);
	(void) fprintf(stderr, "mkboot exiting\n");
	exit(1);
    }

    bzero(label_data, sizeof(*label_data));
    label_data->dkl_lbstart = label_data->dkl_lbend = (long)time((time_t *)0);
    label_data->dkl_magic = DSK_LABEL_MAGIC;
    label_data->dkl_version = DSK_LABEL_VERSION;
    label_data->dkl_part[0].start = DATA_PSN_BOOT;
    label_data->dkl_part[0].length = size - DATA_PSN_BOOT;
    label_data->dkl_flag[0] = DK_PDISC | DK_ROOT | DK_NOBDRA;
    label_data->dkl_boot = DK_FLAG;

    return (char *)label_data;
}
#endif /* LVM */


void
gettime(buf)
char *buf;
{
    long time();
    struct tm *localtime();
    long now;
    register struct tm *date;
    register struct date_fmt *bcd_date;

    bcd_date = (struct date_fmt *)buf;

    now = time((time_t *) 0);       /* get GMT time */
    date = localtime(&now);         /* convert to local time */
    date->tm_mon++;                 /* localtime gives 0-11, we want 1-12 */

    bcd_date->year1 = date->tm_year / 10;
    bcd_date->year2 = date->tm_year % 10;
    bcd_date->mon1  = date->tm_mon  / 10;
    bcd_date->mon2  = date->tm_mon  % 10;
    bcd_date->day1  = date->tm_mday / 10;
    bcd_date->day2  = date->tm_mday % 10;
    bcd_date->hour1 = date->tm_hour / 10;
    bcd_date->hour2 = date->tm_hour % 10;
    bcd_date->min1  = date->tm_min  / 10;
    bcd_date->min2  = date->tm_min  % 10;
    bcd_date->sec1  = date->tm_sec  / 10;
    bcd_date->sec2  = date->tm_sec  % 10;
}


int
writeauto(tgt)
char	*tgt;
{
    struct lif_directory	*ldp;
    struct dentry		*autofile = (struct dentry *)0;
    char			buf[LIFBUFSIZE];

    /*
     * Look for AUTO in the target.
     */
    if (ldp = open_lifdir(tgt, READONLY))
	autofile = scan_lifdir(ldp, AUTO);
    else {
	fprintf(stderr, "No boot area found.  Cannot modify AUTO file.\n");
	exit(1);
    }

    reset_lifdir(ldp, APPEND);

    /*
     * If we didn't find one, create a new entry and write the
     * new directory.
     */
    if (autofile == (struct dentry *)0) {
	autofile = new_lifentry(ldp);
	autofile->size = 1;
	lifstrcpy(autofile->fname, AUTO);
	autofile->ftype = LIFTYPE_AUTO;
	gettime(autofile->date);
	autofile->lastvolnumber = LIF_LVN;
	flush_lifdir(ldp);
    }

    /*
     * Read, modify, write the new contents of the AUTO file.
     * ASSUME: AUTO is exactly 1 lif sector long.
     */
    Lseek(ldp->fd, autofile->start*LIFSECTORSIZE, SEEK_SET);
    Read(ldp->fd, buf, LIFBUFSIZE, ldp->name);
    bzero(buf, LIFSECTORSIZE);
    strcpy(buf, autostring);
    Lseek(ldp->fd, autofile->start*LIFSECTORSIZE, SEEK_SET);
    Write(ldp->fd, buf, LIFBUFSIZE, ldp->name);

    close_lifdir(ldp);

    return 0;
}


/*
 * this is a driver type procedure used to determine which boot program
 * installation procedure to invoke.
 */
void
putboot(head, included, lvm_target, preserve, series, update, force, verbose,
	dev, fssize, swapsize, disksize, bootsize)
int  head, *included, lvm_target, preserve, series, update, force, verbose,
     fssize, swapsize, disksize, bootsize;
char *dev;
{
    extern void install_boot();
    
    if(series == 300) {
	install_300_boot(dev, verbose);
	return;
    }
    if(series == 800) {
	install_800_boot(dev, disksize, included, lvm_target, preserve);
	return;
    }
    if(series == 700) {
	if(update) {
	    update_boot(dev, fssize, swapsize, disksize, bootsize, update,
			force, head);
	    return;
	}
	else {
	    install_boot(bootprogs, dev, fssize, swapsize, update,
			 force, head);
	    return;
	}
    }
}    
/*
 *	rawname - If cp is a block device file, try to figure out the
 *	corresponding character device name by adding an r where we think
 *	it might be.  If found, copy it into rawbuf.
 */
void
rawname(cp)
	char *cp;
{
	char *dp = strrchr(cp, '/');

	if(specialdev)			/* devices already set up */
	    return;

	if(rawbuf[0] != '\0')		/* a raw device was specified at */
	    return;			/* runtime and set in getblockdev */
	if (dp == 0) {
	    (void) fprintf(stderr, "The full path name is required to convert ");
	    (void) fprintf(stderr, "path names\n");
	    exit(1);
	}
	while ( dp >= cp && *--dp != '/' );

	/*  Check for AT&T Sys V disk naming convention
	 *    (i.e. /dev/dsk/c0d0s3).
	 *  If it doesn't fit the Sys V naming convention and
	 *    this is a series 200/300, check for the old naming
	 *    convention (i.e. /dev/hd).  
	 *  If it's neither, change the name as if it's got Sys V
	 *    naming.
	 */
#ifdef __hp9000s300
	if ( strncmp(dp, "/dsk", 4) ) {
		dp = strrchr(cp, '/');
		if ( strncmp(dp, "/hd", 3) )
			while ( dp >= cp && *--dp != '/' );
	}
#endif

	*dp = 0;
	(void)strcpy(rawbuf, cp);
	*dp = '/';
	(void)strcat(rawbuf, "/r");
	(void)strcat(rawbuf, dp+1);

	return;
}


/*
 * This routine picks up the run string arguments and sets the corresponding
 * variables to the correct values.  If the series is not specified, the
 * value is determined by a call to uname to get the architecture type.
 */
 
void
runstring(argc, argv, autoboot, check, head, included, lvm_target, preserve,
	  series, update, force, verbose, dev)
int argc;
char *argv[];
int *autoboot, *check, *head, *included, *lvm_target, *preserve, *series;
int *update, *force, *verbose;
char *dev;
{
    int c;			/* result of getopt() */
    extern char *optarg;	/* used by getopt() */
    extern int optind;		/* used by getopt() */
    int errflg = 0;		/* operator error */
    int bflag = 0;		/* boot programs selected */
    int len;			/* string length */
    char buf[10];		/* yse/no buffer */
    char *inc_file;		/* included file entry */
    
    /*
     * check to see if this is rmboot; if so clobber the boot first 8K
     * and exit.
     */
    if(strcmp(basename(argv[0]), RMBOOT) == 0)
        clobber_boot(argv[1]);

    /*
     * get the uname string that may be used here as well as in
     * getsizes().
     */
    if(uname(&name) == -1) {
	perror("uname");
	exit(1);
    }

    /*
     * use getopt to get the legal parameters and their arguments.
     */
    while((c = getopt(argc, argv, "a:b:cfFhi:lp:s:uv")) != EOF) {
	switch(c) {
	case 'F':		/* quiet mode */
	    fquiet++;		/* fall through to f option */
	case 'f':		/* force mode */
	    if (*check || *head) {
		(void) fprintf(stderr, "-f cannot be used with -c or -h\n");
		usage(argv[0]);
	    }
	    *force = *force + 1;
	    if (fquiet == 0) {
		(void) fprintf(stderr, "The system should be in ");
		(void) fprintf(stderr, "single-user state to use ");
		(void) fprintf(stderr, "this option\n");
		(void) fprintf(stderr, "Continue [y/n]? ");
		while(1) {
		    (void) fflush(stderr);
		    (void) fgets(buf, 4, stdin);
		    if((strncmp(buf, "yes", 3) == 0) ||
		       (strncmp(buf, "ye", 2) == 0) ||
		       (strncmp(buf, "y", 1) == 0))
			    break;
		    if((strncmp(buf, "no", 2) == 0) ||
		       (strncmp(buf, "n", 1) == 0))
			    exit(1);
		    (void) fprintf(stderr, "Please answer yes or no. ");
		    (void) fprintf(stderr, "Continue [y/n]? ");
		}
	    }
	    break;
	case 'a':		/* update the autoboot file (not done yet) */
	    *autoboot = 1;
	    autostring = optarg;
	    /* Append a newline if needed */
	    len = strlen(optarg);
	    if (autostring[len-1] != '\012') {
		autostring = malloc(len+2);
		if (autostring == (char *)0) {
		    perror("Could not copy auto boot string");
		    exit(1);
		}
		strcpy(autostring, optarg);
		strcat(autostring, "\012");
	    }
	    break;
	case 'i':		/* include only specified files */
	    if (*included == 0) {	/* but always include ISL and HPUX */
		(void)strncpy(inc[*included], ISL, strlen(ISL));
		*included = *included + 1;
		(void)strncpy(inc[*included], HPUX, strlen(HPUX));
		*included = *included + 1;
	    }
	    /* Don't include anything twice */
	    if (inc_set(optarg, *included))
		break;
	    if (*included >= MAX_INCLUDED) {
		fprintf(stderr, "Too many included files; only %d allowed\n",
			MAX_INCLUDED);
		exit(1);
	    }
	    (void)strncpy(inc[*included], optarg,
			  min(strlen(optarg), MAXFILENAME+1));
	    *included = *included + 1;
	    break;
	case 'l':		/* treat disk as LVM physical volume */
	    *lvm_target = 1;
	    break;
	case 'p':		/* preserve a file on the target */
	    /* Ignore repeated requests to preserve the same file */
	    if (pres_entry(optarg, *preserve))
		break;
	    if (*preserve >= MAX_PRESERVE) {
		fprintf(stderr, "Too many preserved files; only %d allowed\n",
			MAX_PRESERVE);
		exit(1);
	    }
	    (void)strncpy(pres[*preserve].dentry.fname, optarg,
			  min(strlen(optarg), MAXFILENAME));
	    *preserve = *preserve + 1;
	    break;
	case 'u':		/* update mode */
	    *update = *update + 1;
	    break;
	case 'v':		/* verbose mode */
	    *verbose = *verbose + 1;
	    break;
	case 's':		/* series specification */
	    if((strcmp(optarg, "300") != 0) &&
	       (strcmp(optarg, "700") != 0) &&
	       (strcmp(optarg, "800") != 0)) {
	        (void) fprintf(stderr, "mkboot: invalid series ");
		(void) fprintf(stderr, "specification %s\n",optarg);
		usage(argv[0]);
	    }
	    *series = atoi(optarg);
	    break;
        case 'b':
	    bootprogs = optarg;
	    bflag++;
	    break;
        case 'c':		/* check only for fit */
	    if (*head || *force) {
		(void) fprintf(stderr, "-c cannot be used with -f or -h\n");
		usage(argv[0]);
	    }
	    *check = *check + 1;
	    break;
        case 'h':		/* only modify the lif header */
	    if (*check || *force) {
		(void) fprintf(stderr, "-h cannot be used with -f or -c\n");
		usage(argv[0]);
	    }
	    *head = *head + 1;
	    break;
	case '?':
	    errflg++;
	}
	if(errflg)
	    usage(argv[0]);
    }

    /*
     * All that should be left is the device file.  If so, copy it to
     * the device parameter.
     */
    if((optind+1) != argc) {
	(void) fprintf(stderr, "Illegal number of arguments specified\n");
	usage(argv[0]);
    }
    (void) strcpy(dev, argv[optind]);
    if(strcmp(dev, SPECDEV) == 0) {
	(void) strcpy(rawbuf, SPECRDEV);
	specialdev++;
    }
    if(strcmp(dev, SPECRDEV) == 0) {
	(void) strcpy(dev, SPECDEV);
	(void) strcpy(rawbuf, SPECRDEV);
	specialdev++;
    }

    /*
     * If series was not specified in the run string, set it now (according
     * to the machine type).
     */
    if(*series == 0) {
	switch(ARCHTYPE) {
	case '3':
	case '4':
	    *series = 300;
	    break;
	case '7':
	    *series = 700;
	    break;
	case '8':
	    *series = 800;
	    break;
	default:
	    (void) fprintf(stderr, "Unable to identify the system ");
	    (void) fprintf(stderr, "architecture\n");
	    exit(1);
	}
    }

    if((bflag == 0) && (*series == 300))
	    bootprogs = BOOT300;
    if((bflag == 0) && (*series == 700))
	    bootprogs = BOOT700;
    if((bflag == 0) && (*series == 800))
	    bootprogs = BOOT800;


    if ( (*series != 700) &&
	 ((*force) || (*update) || (*check) || (*head)) ) {
	(void) fprintf(stderr, "At least one series 700 specific option\n");
	(void) fprintf(stderr, "(-f, -h, -u, or -c) was specified for the s");
	if (*series == 300)
	    (void) fprintf(stderr, "300\n");
	if (*series == 800)
	    (void) fprintf(stderr, "800\n");
	exit(1);
    }

    if ( (*series != 800) &&
	 ((*lvm_target) || (*preserve) || (*included) || (*autoboot)) ) {
	(void) fprintf(stderr, "At least one series 800 specific option\n");
	(void) fprintf(stderr, "(-a, -i, -l, or -p) was specified for the s");
	if (*series == 300)
	    (void) fprintf(stderr, "300\n");
	if (*series == 700)
	    (void) fprintf(stderr, "700\n");
	exit(1);
    }

    return;
}

/*
 * squeez the swap space to make room for the additional boot programs
 */
int
squeezswap(bootsize, dev, disksize, swapsize)
char *dev;
int bootsize;
int disksize;
int swapsize;
{
    extern int getlifvals();		/* get swap information */
    int lif_swapstart;			/* LIF swap start val */
    int lif_swapsize;			/* LIF swap size val */
    int avail;				/* available size for boot */
    int shrink;				/* amount to shrink swap by */
    char buf[10];			/* temp buffer */
    extern int putswap();		/* write swap val to LIF */

    if (getlifvals(dev, &lif_swapstart, &lif_swapsize) == 0) {
	(void) fprintf(stderr, "mkboot: Unable to read the swap ");
	(void) fprintf(stderr, "information from the device header\n");
	exit(1);
    }

    lif_swapstart /= 4;			/* convert to K bytes */
    lif_swapsize /= 4;			/* convert to K bytes */
    avail = disksize - (lif_swapstart + lif_swapsize);

    if (bootsize <= avail) {
	(void) fprintf(stderr, "No change is required.  The boot programs ");
	(void) fprintf(stderr, "fit\n");
	return(0);
    }
    else {
	shrink = bootsize - avail;
	lif_swapsize = lif_swapsize - shrink;
	if (lif_swapsize < 0) {
	    (void) fprintf(stderr, "There is not room on the disk for the ");
	    (void) fprintf(stderr, "requested boot programs\n");
	    exit(1);
	}
	if (lif_swapsize == 0) {
	    (void) fprintf(stderr, "There will be no swap available on ");
	    (void) fprintf(stderr, "the device if this operation continues\n");
	    (void) fprintf(stderr, "Continue [y/n]? ");
	    while (1) {
		(void) fflush(stderr);
		(void) fgets(buf, 4, stdin);
		if ((strncmp(buf, "yes", 3) == 0) ||
		    (strncmp(buf, "ye",  2) == 0) ||
		    (strncmp(buf, "y",   1) == 0))
		    break;
		if ((strncmp(buf, "no", 2) == 0) ||
		    (strncmp(buf, "n",  1) == 0))
		    exit(1);
		(void) fprintf(stderr, "Please answer yes or no. ");
		(void) fprintf(stderr, "Continue [y/n]? ");
	    }
	}
	if (putswap(dev, lif_swapsize) != 0) {
	    (void)fprintf(stderr, "mkboot: Unable to modify the lif header\n");
	    exit(1);
	}
	(void) fprintf(stderr, "mkboot: The LIF header has been adjusted ");
	(void) fprintf(stderr, "to accommodate the new boot programs\n");
	return(0);
    }
}


/*
 * update_boot reads the LIF volume on the s700 and modifies the boot
 * programs in place.  It takes care that neither the file system nor
 * the swap are overwritten.  If there is raw I/O active on the disk,
 * it will remain unmodified as well.
 */
void
update_boot(dev, fssize, swapsize, disksize, bootsize, update, force, head)
char *dev;
int head, fssize, swapsize, disksize, bootsize, update, force;
{
    extern int swapinfo();		/* reads /dev/kmem */
    struct swdevt info;			/* data from /dev/kmem */
    extern int getlifvals();		/* reads the lif header */
    int lif_swapstart;			/* start of swap on lif vol */
    int lif_swapsize;			/* size of swap on lif vol */
    int lifend;				/* end of swap in lif header */
    int swapend;			/* end of swap in /dev/kmem */
    int avail;				/* determine if space is there */

    /*
     * Check for possible swapping to the device.  If not swapping,
     * then go ahead and install_boot
     */
    if(ARCHTYPE == 8) {		/* swap is impossible */
	install_boot(bootprogs, dev, fssize, swapsize, update, force, head);
	return;
    }

    /*
     * We are running on a 300 or a 700 and swapping to the device might
     * be enabled.  Tread lightly here; make sure that we don't
     * overwrite either swap (and cause a panic), or clobber any
     * configured raw I/O area.  How is this done?
     * First, read /dev/kmem and the get the swap information associated
     * with the device.  If there is none, call install_boot.
     *
     * NB. The value returned by diskinfo for the disk size may be
     * a block off.  So subtract a block from the size of swap in
     * /dev/kmem and the lif header.  If we're worng, it's
     * better to be wrong in this direction rather than overwrite
     * critical space.
     */
    if(swapinfo(dev, &info, force)) {		/* swap to device */
	if(getlifvals(dev, &lif_swapstart, &lif_swapsize) == 0) {
	    (void) fprintf(stderr, "mkboot: Unable to read the swap ");
	    (void) fprintf(stderr, "information from the device header\n");
	    exit(1);
        }
	lif_swapstart /= 4;		/* convert to K bytes */
	lif_swapsize /= 4;		/* convert to K bytes */
	if(info.sw_enable) {		/* swap currently active */
	    avail = disksize - (lif_swapstart + lif_swapsize);
	    if(avail > bootsize) {
	        (void) fprintf(stderr, "mkboot: Shrinking the boot area will");
	        (void) fprintf(stderr, " not increase the available swap\n");
	        (void) fprintf(stderr, "        space until system reboot\n");
            }
	    if((avail >= bootsize) || (head) || (force)) {
	        install_boot(bootprogs, dev, fssize, swapsize, update,
			     force, head);
            }
	    else {
	        (void) fprintf(stderr, "mkboot: swapping is enabled to this ");
	        (void) fprintf(stderr, "device and the requested boot size\n");
	        (void) fprintf(stderr, "        is greater than the available ");
	        (void) fprintf(stderr, "space.  The requested boot programs\n");
	        (void) fprintf(stderr, "        must be installed without swap ");
	        (void) fprintf(stderr, "enabled\n");
		exit(1);
            }
        }
	else {				/* swapinfo succeeded, but no swap */
	    avail = 0;			/* assume there is no space */
	    lifend = lif_swapstart + lif_swapsize;
	    swapend = info.sw_start + info.sw_nblks;
	    /*
	     * take care of the 4 cases of no rawio, rawio before swap, rawio
	     * after swap and rawio before and after swap.
	     */

	    /*
	     * (1) no rawio
	     */
	    if((info.sw_start == lif_swapstart) &&
	       (swapend == lifend))
		    avail = disksize - fssize;

	    /*
	     * (2) rawio before swap
	     */
	    if((info.sw_start > lif_swapstart) &&
	       (swapend == lifend))
		    avail = disksize - (fssize + (info.sw_start - lif_swapstart));
	    
	    /*
	     * (3) rawio after swap
	     * (4) rawio before and after swap
	     */
	    if((info.sw_start >= lif_swapstart) &&
	       (lifend > swapend))
		    avail = disksize - (lif_swapstart + lif_swapsize);

	    if((avail >= bootsize) || (head) || (force))
	        install_boot(bootprogs, dev, fssize, swapsize, update,
			     force, head);
	    else {
		(void) fprintf(stderr, "mkboot: There is not enough room for ");
		(void) fprintf(stderr, "these boot programs since raw I/O\n");
		(void) fprintf(stderr, "appears to be configured in to the kernel\n");
		return;
	    }
        }
    }
    else {				/* swapinfo failed */
	install_boot(bootprogs, dev, fssize, swapsize, update, force, head);
    }
    return;
}


void
usage(p)
char *p;
{
    if(strcmp(basename(p), RMBOOT) == 0) {
	(void) fprintf(stderr, "usage: rmboot device\n");
	exit(1);
    }
    (void) fprintf(stderr, "usage: mkboot [-a auto file string] [-b boot file path] [-c] [-f] [-h] [-i lif file name] [-l] [-p lif file name] [-s series] [-u] [-v] device\n");
    (void) fprintf(stderr, "       where:  series is 300, 700 or 800\n");
    (void) fprintf(stderr, "               -a modifies the autoboot file\n");
    (void) fprintf(stderr, "               -b select a boot file path\n");
    (void) fprintf(stderr, "               -c check boot path for fit\n");
    (void) fprintf(stderr, "               -h modify only the lif header\n");
    (void) fprintf(stderr, "               -i include only specified file\n");
    (void) fprintf(stderr, "               -l treat device as an LVM physical volume\n");
    (void) fprintf(stderr, "               -p preserve specified lif file on device\n");
    (void) fprintf(stderr, "               -f force an update of the boot programs\n");
    (void) fprintf(stderr, "               -u specifies update mode\n");
    (void) fprintf(stderr, "               -v specifies verbose mode\n");
    (void) fprintf(stderr, "               device is special file for boot");
    (void) fprintf(stderr, " program installation\n");
    exit(1);
}


main(argc, argv)
int argc;
char *argv[];
{
    int fssize;			/* file system size in MB */
    int swapsize;		/* swap size in MB */
    int disksize;		/* size of the disk device */
    int bootsize;		/* size of the boot programs */
    int series = 0;		/* behavior flag */
    int update = 0;		/* behavior mode */
    int verbose = 0;		/* silent or not */
    int autoonly = 0;		/* update the autoboot file? */
    int checkonly = 0;		/* only check for boot program fit */
    int headonly = 0;		/* only modify the lif header */
    int force = 0;		/* force the boot programs to the LIF */
    int lvm_target = 0;		/* treat target as LVM disk */
    int included = 0;		/* copy only included set of files */
    int preserve = 0;		/* preserve selected file(s) on target */
    int squeezswap();		/* routine to modify the LIF header */
    char *updatedev;		/* name of device we write to */
    
    runstring(argc, argv, &autoonly, &checkonly, &headonly, &included,
	      &lvm_target, &preserve, &series, &update, &force, &verbose,
	      device);
    getblockdev(device);
    getsizes(&fssize, &swapsize, &disksize, &bootsize,
	     device, series, verbose);

    if (series == 800)		/* Series 800 updates whatever user typed */
	updatedev = userdev;
    else			/* Others always use the block device */
	updatedev = device;

    if (autoonly) {
	return(writeauto(updatedev));
    }
    if (headonly) {
	squeezswap(bootsize, updatedev, disksize, swapsize);
	return(0);
    }
    if (checkonly) {
	return(check_for_fit(fssize, swapsize, disksize, bootsize,
			     updatedev, verbose, force));
    }
    else {
	putboot(headonly, &included, lvm_target, preserve, series, update,
		force, verbose, updatedev, fssize, swapsize, disksize,
		bootsize);
	return(0);
    }
}
