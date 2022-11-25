#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/conf.h>

#include "lifio.h"
#include "global.h"
/* mkboot /etc/boot /dev/dsk/?? 2Mb */

#define DEV_BSIZE 1024
#define ALIGN(v, a)	((v) & (~((a)-1)))
#define ROUNDUP(v, a)	ALIGN((v)+(a)-1, (a))

extern int fquiet;		/* -F option for mkrs */
/*
 * put the new swap value in the lif header to accommodate the -h option
 */
int
putswap(dev, swapsize)
char *dev;
int swapsize;
{
    int fd;			/* open lif volume */
    struct dentry lifentry;	/* lif directory entry */
    struct dentry ignore;	/* lif directory entry */
    int status;			/* local results variable */
    int reccount;		/* seek counter */

    reccount = 0;
    if((fd=open(dev, O_RDWR)) == -1) {
	perror("putswap: open failed");
	exit(1);
    }

    if(lseek(fd, 2048, 0) == -1) {
	perror("putswap: seek failed");
	exit(1);
    }

    do {
	status = read(fd, (char*)&lifentry, sizeof( lifentry ));
	if(status != sizeof(lifentry)) {
	    perror("putswap: lif entry read error");
	    exit(1);
	}
	reccount++;
	if( lifentry.ftype == (short int)0 )
		continue; /* Skip purged entries */
	if( lifentry.ftype == 21059) {
		lifentry.size = swapsize * 4;
		if(lseek(fd, 2048, 0) == -1) {
		    perror("putswap: seek failed");
		    exit(1);
		}
		for (status = 0; status < (reccount - 1); status++) {
		    if (read(fd, (char*)&ignore, sizeof( ignore )) !=
			sizeof(ignore)) {
			perror("putswap: lif entry read error");
			exit(1);
		    }
		}
		if (write(fd, (char*)&lifentry, sizeof(lifentry)) != sizeof(lifentry)) {
		    perror("putswap: write swap failed");
		    exit(1);
		}
		(void) close(fd);
		return(0);
	}
    } while( lifentry.ftype != (short int)-1 );
    return(1);
}


/*
 * getlifvals returns 1 if it finds the swap entries and
 * 0 otherwise
 */
int
getlifvals(dev, swapstart, swapsize)
char *dev;
int *swapstart, *swapsize;
{
    int fd;			/* open lif volume */
    struct dentry lifentry;	/* lif directory entry */
    int status;			/* local results variable */

    if((fd=open(dev, O_RDONLY)) == -1) {
	perror("getlifvals: open failed");
	exit(1);
    }

    if(lseek(fd, 2048, 0) == -1) {
	perror("getlifvals: seek failed");
	exit(1);
    }

    do {
	status = read(fd, (char*)&lifentry, sizeof( lifentry ));
	if(status != sizeof(lifentry)) {
	    perror("getlifvals: lif entry read error");
	    exit(1);
	}
	if( lifentry.ftype == (short int)0 )
		continue; /* Skip purged entries */
	if( lifentry.ftype == 21059) {
		*swapstart = lifentry.start;
		*swapsize = lifentry.size;
		(void) close(fd);
		return(1);
	}
    } while( lifentry.ftype != (short int)-1 );
    return(0);
}



void
install_boot(s, t, fssize, swapsize, update, force, head)
char *s, *t;
int fssize, swapsize, update, force, head;
{
	LIFDIR *lif_opendir(), *lfp;
	LIFFILE *lif_fopen(), *lifsfp, *liftfp;
	struct lif_dirent *lif_readdir(), *lep;
	struct dentry lifentry;
	struct lvol hdr;
	int status, ifd, ofd;
	int brem, sec_offset;
	int toffset;
	int swapres;
	char buf[0x100];
	extern int swapinfo();
	struct swdevt info;


	/*
	 * If swapping is enabled, print an error message and bail out
	 */
	if((swapres = swapinfo(t, &info, force)) && !force) {
	    if(info.sw_enable && !update) {
	        (void) fprintf(stderr, "mkboot: swapping to device %s ",t);
	        (void) fprintf(stderr, "is enabled.  Use the update option\n");
	        return;
	    }
        }

	/*
	 * Figure out location on target for
	 * LIF entries. Need this to fix
	 * up directory.
	 */
	ofd = open( t, O_RDWR );
	if(ofd == -1) {
	    perror("device open failed");
	    exit(1);
	}
	sec_offset = (fssize + swapsize) * DEV_BSIZE;
	sec_offset = ROUNDUP(sec_offset, 2048) / 256;
	if(lseek( ofd, 0, SEEK_SET ) == -1) {
	    perror("device lseek failed");
	    exit(1);
	}


	/* Read the header */
	ifd = open( s, O_RDONLY);
	if(ifd == -1) {
	    perror("uxbootlf open failed");
	    exit(1);
	}
	if(read( ifd, (char *)&hdr, sizeof(hdr)) != sizeof(hdr)) {
	    perror("uxbootlf read error");
	    exit(1);
	}

	/* Modify and copy the directory */
	if(lseek(ifd, (off_t)hdr.dstart * 0x100, 0) == -1) {
	    perror("uxbootlf seek failed");
	    exit(1);
	}
	if(lseek(ofd, 2048, 0) == -1) {		/* for labels */
	    perror("device seek failed");
	    exit(1);
	}

	install_spec( ofd, "FS", &hdr,
		0, ((fssize*DEV_BSIZE)/256) );
	install_spec( ofd, "SWAP", &hdr,
		((fssize * DEV_BSIZE)/256), ((swapsize*DEV_BSIZE)/256));

	toffset = sec_offset;
	do {

		status = read(ifd, (char*)&lifentry, sizeof( lifentry ));
		if(status != sizeof(lifentry)) {
		    perror("uxbootlf lif entry read error");
		    exit(1);
		}
		if( lifentry.ftype == (short int)0 )
			continue; /* Skip purged entries */

		lifentry.start = toffset;
		status = write(ofd, (char*)&lifentry, sizeof( lifentry ));
		if(status != sizeof(lifentry)) {
		    perror("device write failed");
		    exit(1);
		}

		if( !strncmp(lifentry.fname,"ISL",3) ) {
			hdr.iplstart = toffset * 256;
		}

		/* Align on NEXT 2k (8 sectors) boundary */
		toffset = ROUNDUP( toffset+lifentry.size, 8 );

	} while( lifentry.ftype != (short int)-1 );

	(void) close(ifd);

	/* Write the header */
	if(lseek(ofd, 0, SEEK_SET) == -1) {
	    perror("lseek to beginning of device failed");
	    exit(1);
	}
	hdr.dstart = 8;			/* for labels */
	status = write(ofd, (char*)&hdr, sizeof( hdr ));
	if(status != sizeof(hdr)) {
	    perror("lif header write to device failed");
	    exit(1);
	}
	(void) close(ofd);

	/*
	 * The best way to walk a LIF
	 *
	 * all lif routines check for errors; if one's found the command
	 * exits
	 */

	lfp = lif_opendir( s );
	lif_seekdir(lfp, 0, 0);

	/* Now copy the lif entries themselves */
	while ( (lep = lif_readdir(lfp)) ) {

		(void)strcpy(buf, s); (void)strcat(buf,":");
		(void)strcat(buf, lep->fname);
		lifsfp = lif_fopen( buf, "r" );

		(void)strcpy(buf, t); (void)strcat(buf,":");
		(void)strcat(buf, lep->fname);
		liftfp = lif_fopen( buf, "rw" );

		for( brem = lep->size; brem > 0; brem -= 0x100){
			status = lif_fread(lifsfp, buf, 0x100); 
			status = lif_fwrite(liftfp, buf, 0x100); 
		}

		lif_fclose( lifsfp );
		lif_fclose( liftfp );
	}

	if(force && (fquiet == 0)) {
	    (void) fprintf(stderr, "******************************\n");
	    (void) fprintf(stderr, "The lif header and boot programs ");
	    (void) fprintf(stderr, "have been forcefully modified. In order ");
	    (void) fprintf(stderr, "to\n");
	    (void) fprintf(stderr, "reflect these changes ");
	    (void) fprintf(stderr, "on the running system, you must ");
	    (void) fprintf(stderr, "reboot.  Not rebooting\nat ");
	    (void) fprintf(stderr, "this point could ");
	    (void) fprintf(stderr, "cause system corruption.\n");
	    (void) fprintf(stderr, "******************************\n");
	    return;
	}
}
install_spec( fd, name, hdr, loc, size )
int fd;
char *name;
struct lvol *hdr;
int loc, size;
{
	struct dentry lifentry;
	int i;

	(void)strncpy(lifentry.fname,"          ", 10);
	for( i=0; *name; i++)
		lifentry.fname[i] = *name++;

	if(strncmp(lifentry.fname, "SWAP", 4) == 0)
	    lifentry.ftype = 21059;
	else
	    lifentry.ftype = -13000;
	lifentry.start = loc;
	lifentry.size = size;
	for(i=0; i<6; i++)
		lifentry.date[i] = hdr->date[i];
	lifentry.lastvolnumber = -32767 /*???*/;
	lifentry.extension = 0;

	if(write(fd, (char*)&lifentry, sizeof(lifentry)) != sizeof(lifentry)) {
	    perror("write to lif entry failed");
	    exit(1);
	}
}
