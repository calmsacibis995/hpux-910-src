/* @(#) $Revision: 37.1 $ */     
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                read.c                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "s500defs.h"
#include "sdf.h"			
#include <sys/stat.h>
#include <ustat.h>

extern struct sdfinfo sdfinfo[];

#ifdef	DEBUG
#include <stdio.h>
extern int debug;
#endif	DEBUG

/* --------------------------------------- */
/* sdfread - read from SDF file descriptor */
/* --------------------------------------- */

sdfread(fd, buf, size)
register int fd, size;
register char *buf;
{
	register struct sdfinfo *sp;
	register int rlen;

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "sdfread(%d, buf(0x%08x), (size)%d)\n", fd, buf, size);
}
#endif	DEBUG

	if (!sdffile(fd))
		return(read(fd, buf, (unsigned) size));
	sp = &sdfinfo[fd - SDF_OFFSET];
	if (!sp->opened)
		return(-1);
	if ((size + sp->offset) > sp->inode.di_size)
		size = sp->inode.di_size - sp->offset;
	if (size == 0)
		return(0);
	rlen = inoread(sp, sp->inumber, &sp->inode, sp->offset, buf, size);
	if (rlen != size)
		return(-1);
	sp->offset += rlen;
	return(rlen);
}

/* --------------------------------------- */
/* sdfwrite - write to SDF file descriptor */
/* --------------------------------------- */

sdfwrite(fd, buf, size)
register int fd;
register char *buf;
register int size;
{
	register struct sdfinfo *sp;
	register int wlen;

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "sdfwrite(%d, buf(0x%08x), (size)%d)\n", fd, buf, size);
}
#endif	DEBUG

	if (!sdffile(fd))
		return(write(fd, buf, (unsigned) size));
	sp = &sdfinfo[fd - SDF_OFFSET];
	if (!sp->opened)
		return(-1);
	wlen = inowrite(sp, sp->inumber, &sp->inode, sp->offset, buf, size);
	if (wlen != size)
		return(-1);
	sp->offset += wlen;
	if (sp->offset > sp->inode.di_size) {
		sp->inode.di_size = sp->offset;
		if (putino(sp, sp->inumber, &sp->inode) < 0)
			return(-1);
	}
	return(wlen);
}
