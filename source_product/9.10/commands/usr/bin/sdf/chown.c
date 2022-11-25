/* @(#) $Revision: 37.1 $ */    
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                chown.c                                *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "s500defs.h"
#include "sdf.h"			
#include <errno.h>
#include <sys/stat.h>
#include <ustat.h>

extern struct sdfinfo sdfinfo[];
extern int errno;

#ifdef	DEBUG
#include <stdio.h>
extern int debug;
#endif	DEBUG

/* --------------------------------------------- */
/* sdfchown - change owner and group of SDF file */
/* --------------------------------------------- */

sdfchown(path, owner, group)
register char *path;
register int owner, group;
{
	register int fd, ret;
	register struct sdfinfo *sp;

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "sdfchown(%s, (owner) %d, (group) %d)\n",
		path, owner, group);
}
#endif	DEBUG

	if (!sdfpath(path)) {
		errno = ENODEV;
		return(-1);
	}
	fd = sdfopen(path, 0);
	if (fd < 1)
		return(-1);
	sp = &sdfinfo[fd - SDF_OFFSET];
	sp->inode.di_uid = owner;
	sp->inode.di_gid = group;
	ret = putino(sp, sp->inumber, &sp->inode);
	sdfclose(fd);
	if (ret < 0)
		return(-1);
	return(0);
}

/* ---------------------------------- */
/* sdfchmod - change mode of SDF file */
/* ---------------------------------- */

sdfchmod(path, mode)
register char *path;
register int mode;
{
	register int fd, ret;
	register struct sdfinfo *sp;

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "sdfchmod(%s, (mode) 0%o)\n", path, mode);
}
#endif	DEBUG

	if (!sdfpath(path)) {
		errno = ENODEV;
		return(-1);
	}
	fd = sdfopen(path, 0);
	if (fd < 0)
		return(-1);
	sp = &sdfinfo[fd - SDF_OFFSET];
	sp->inode.di_mode &= ~07777;
	sp->inode.di_mode |= (mode & 07777);
	ret = putino(sp, sp->inumber, &sp->inode);
	sdfclose(fd);
	if (ret < 0)
		return(-1);
	return(0);
}
