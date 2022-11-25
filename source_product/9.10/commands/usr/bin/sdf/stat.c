/* @(#) $Revision: 37.1 $ */     
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                stat.c                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "s500defs.h"
#include "sdf.h"			
#include <sys/stat.h>

#define	IOS_TCONV	210866803200.0		/* IOS -> UNIX time conv */

extern struct sdfinfo sdfinfo[];

#ifdef	DEBUG
#include <stdio.h>
extern int debug;
#endif	DEBUG

/* ------------------------------------------------ */
/* sdfstat - return SDF file information given name */
/* ------------------------------------------------ */

sdfstat(path, stbuf)
register char *path;
register struct stat *stbuf;
{
	register int fd;

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "sdfstat(%s, (sdfst_buf) 0x%08x)\n", path, stbuf);
}
#endif	DEBUG

	if (!sdfpath(path))
		return(stat(path, stbuf));
	fd = sdfopen(path, 0);
	if (fd < 0)
		return(-1);
	sdffstat(fd, stbuf);
	sdfclose(fd);
	return(0);
}

/* ----------------------------------------------- */
/* sdffstat - return SDF file information given fd */
/* ----------------------------------------------- */

sdffstat(fd, stbuf)
register int fd;
register struct stat *stbuf;
{

	register struct sdfinfo *sp;
	register struct dinode *ip;
	register int ftype = 0;
	long ios2unix();

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "sdffstat(%d, (sdfst_buf) 0x%08x)\n", fd, stbuf);
}
#endif	DEBUG

	if (!sdffile(fd))
		return(fstat(fd, stbuf));
	sp = &sdfinfo[fd - SDF_OFFSET];
	ip = &sp->inode;
	ftype = ip->di_mode & S_IFMT;

	stbuf->st_dev = sp->dev;
	stbuf->st_ino = sp->inumber;
	stbuf->st_mode = ip->di_mode;
	stbuf->st_nlink = ip->di_count;
	stbuf->st_uid = ip->di_uid;
	stbuf->st_gid = ip->di_gid;
	if (ftype == S_IFCHR || ftype == S_IFBLK)
		stbuf->st_rdev = ip->di_dev;
	else
		stbuf->st_rdev = 0;
	stbuf->st_size = ip->di_size;
	stbuf->st_atime = ip->di_atime;
	stbuf->st_ctime = ios2unix(&ip->di_ctime);
	stbuf->st_mtime = ios2unix(&ip->di_mtime);
	return(0);
}

/* -------------------------------------------------------------------- */
/* ios2unix - convert IOS time to UNIX time for access/change/mod times */
/* -------------------------------------------------------------------- */

long
ios2unix(iostime)
register time_ios *iostime;
{
	return(iostime->ti_date * 86400.0 + iostime->ti_time - IOS_TCONV);
}
