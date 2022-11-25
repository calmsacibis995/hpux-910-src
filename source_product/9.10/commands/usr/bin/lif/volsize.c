/* @(#) $Revision: 27.1 $ */     
/*
 * discsize - poke around the raw disc device to find out how big the
 * 	      disc is in bytes.
 */
#include "lifdef.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

long lseek();

#define	MAXBSIZE	8192
#define	MINBSIZE	256
#define	DMAGRANULARITY	4

int debug=0;
long discsize();

long volsize(fd)
int fd;
{

	return discsize(fd);
}

/*
 * fbsize - return the size of blocks on the file system specified
 *	    by fd.  This call may return <n> or 1 or 0, where <n>
 *	    is the size of the blocks in bytes, or 1 is the default
 *	    size (real size could not be ascertained) or 0 indicates
 *	    the size could not be determined (such as an error occured).
 */

static int
fbsize(fd, pbuf, pbufsize)
register int fd;
register char *pbuf;
register int pbufsize;
{

	register long seekto = MINBSIZE;
	register int found, len;


	if (fd < 0) {
	    fprintf(stderr, "fbsize: bad fd %d\n", fd);
	    return 0;
	}
	found = 0;
	if (debug)
		fprintf(stderr, "bsize: %ld\n", seekto);
	while (seekto < pbufsize) {
		lseek(fd, seekto, 0);
		len = read(fd, pbuf, pbufsize);
		if (len == pbufsize) {
			found = 1;
			break;
		}
	        seekto <<= 1;
		if (debug)
		    fprintf(stderr, "bsize: %ld\n", seekto);
	}
	if (debug)
	    fprintf(stderr, "preliminary bsize: %ld\n", seekto);
	if (found == 0) {
	    if (debug)
		fprintf(stderr, "actual bsize not found\n");
	    seekto = MINBSIZE;
	    lseek(fd, seekto, 0);
	    len = read(fd, pbuf, DMAGRANULARITY);
	    if (len != DMAGRANULARITY ) {
		seekto = 0;
	    }
	}
	if (debug)
	    fprintf(stderr, "final bsize: %ld\n", seekto);
	return seekto;
}

long
discsize(fd)
register int fd;
{
	char *malloc();
	struct stat statbuf;
	register long offset;
	register long blockno;
	register int direction;
	register long step;
	register int bsize;
	register unsigned long ublockno;
	register char *pbuf;
	register int len;

	fstat(fd, &statbuf);
	statbuf.st_mode &= S_IFMT;
	if ((statbuf.st_mode != S_IFCHR) && (statbuf.st_mode != S_IFBLK)) {
	    return (0);
	}
	if (statbuf.st_mode == S_IFCHR) {
	    pbuf = malloc((unsigned) MAXBSIZE);
	    if (pbuf == NULL) {
		return (0);
	    }
	    bsize = fbsize(fd, pbuf, MAXBSIZE);
	}
	else {
	    pbuf = malloc(1);
	    if (pbuf == NULL) {
		return (0);
	    }
	    bsize = 1;
	}
	if (bsize <= 0) {
	    free(pbuf);
	    return (0);
	}
	ublockno = (unsigned) 0;
	ublockno = ~ublockno;
	ublockno >>= 1;
	blockno = (long)(ublockno/(unsigned)bsize);
	step = blockno/2 + 1;
	/* binary search for the end of the disc */
	while (step >= 1) {
		offset = blockno * bsize;
		lseek(fd, offset, 0);
		if (debug) 
			fprintf(stderr, "b %ld %d %d", blockno, step, direction);
		if ((len=read(fd, pbuf, bsize)) != bsize)
			direction = -1;
		else 
			direction = 1;
		if (debug)
			fprintf(stderr, " %d\n", len);
		blockno += (step*direction);
		step /= 2;
	}
	/* we are close, now sneak up on the end of the disc */
	blockno -= 4;
	offset = blockno * bsize;
	lseek(fd, offset, 0);
	if (debug) 
		fprintf(stderr, "w %ld\n", blockno);
	while (read(fd, pbuf, bsize) == bsize) {
		blockno++;
		offset = blockno * bsize;
		lseek(fd, offset, 0);
		if (debug) 
			fprintf(stderr, "w %ld\n", blockno);
	}
	free(pbuf);
	return (blockno * bsize);
}
