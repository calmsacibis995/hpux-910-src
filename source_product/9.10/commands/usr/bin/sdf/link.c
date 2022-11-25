/* @(#) $Revision: 37.1 $ */     
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                link.c                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "s500defs.h"
#include "sdf.h"			
#include "bit.h"
#include <errno.h>
#include <sys/stat.h>
#include <ustat.h>

#define	DELETED_ENTRY	187

extern struct sdfinfo sdfinfo[];
extern int errno;

#ifdef	DEBUG
#include <stdio.h>
extern int debug;
#endif	DEBUG

/* ------------------------------------ */
/* sdflink - make a link to an SDF file */
/* ------------------------------------ */

sdflink(oldpath, newpath)
register char *oldpath, *newpath;
{
	register int newfd, oldfd;
	register struct sdfinfo *oldsp, *newsp;
	struct direct dbuf;

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "sdflink(%s, %s)\n", oldpath, newpath);
}
#endif	DEBUG

	if (!sdfpath(oldpath) || !sdfpath(newpath)) {
		errno = ENODEV;
		return(-1);
	}
	if (exist(newpath)) {
		errno = EEXIST;
		return(-1);
	}
	oldfd = sdfopen(oldpath, READ);
	if (oldfd < 0)
		return(-1);
	oldsp = &sdfinfo[oldfd - SDF_OFFSET];
	if (oldsp->inode.di_count == MAXLINK) {
		errno = EMLINK;
		sdfclose(oldfd);
		return(-1);
	}
	newfd = sdfopen(newpath, WRITE);
	if (newfd < 0) {
		sdfclose(newfd);
		return(-1);
	}
	newsp = &sdfinfo[newfd - SDF_OFFSET];
	if (newsp->fd != oldsp->fd) {
		sdfclose(newfd);
		sdfclose(oldfd);
		errno = EXDEV;
		return(-1);
	}
	oldsp->inode.di_count++;
	if (putino(oldsp, oldsp->inumber, &oldsp->inode) < 0)
		return(-1);
	trunc(newsp, READ);
	get(newsp, newsp->diraddr, (char *) &dbuf, D_SIZ);
	freeino(newsp, dbuf.d_ino, &newsp->inode);
	dbuf.d_ino = oldsp->inumber;
	put(newsp, newsp->diraddr, (char *) &dbuf, D_SIZ);
	sdfclose(newfd);
	sdfclose(oldfd);
	return(0);
}

/* ------------------------------------------------------------- */
/* sdfunlink - remove an file or directory on an SDF file system */
/* ------------------------------------------------------------- */

sdfunlink(path)
register char *path;
{
	register int fd, off, inum, ret;
	register struct sdfinfo *sp;
	register char *fname;
	struct direct dbuf;
	struct dinode ibuf;
	char *sdftail();
	void sdfdir2str();

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "sdfunlink(%s)\n", path);
}
#endif	DEBUG

	if (!sdfpath(path)) {
		errno = ENODEV;
		return(-1);
	}
	fd = sdfopen(path, READ);
	if (fd == -1)
		return(-1);
	sp = &sdfinfo[fd - SDF_OFFSET];
	if (sp->inumber == ROOT_INUM) {
		sdfclose(fd);
		errno = EPERM;
		return(-1);
	}
	inum = sp->pinumber;
	getino(sp, inum, &ibuf);
	fname = sdftail(path);		/* get filename component  */
	for (off = 0; off < ibuf.di_size; off += D_SIZ) {
		if (inoread(sp, inum, &ibuf, off, (char *) &dbuf, D_SIZ) < 0) {
			sdfclose(fd);
			return(-1);
		}
		if (dbuf.d_ino == 0)
			continue;
		sdfdir2str(dbuf.d_name);
		if (strncmp(dbuf.d_name, fname, DIRSIZ + 2) == 0) {
			dbuf.d_ino = 0;
			dbuf.d_object_type = DELETED_ENTRY;
			inowrite(sp, inum, &ibuf, off, (char *) &dbuf, D_SIZ);
			sp->inode.di_count--;
			if (sp->inode.di_count == 0) {    /* last link? */
				if (trunc(sp, READ) < 0)
					return(-1);
				ret = freeino(sp, sp->inumber, &sp->inode);
			}
			else
				ret = putino(sp, sp->inumber, &sp->inode);
			sdfclose(fd);
			if (ret < 0)
				return(-1);
			return(0);
		}
	}
	sdfclose(fd);
	errno = ENOENT;
	return(-1);
}
