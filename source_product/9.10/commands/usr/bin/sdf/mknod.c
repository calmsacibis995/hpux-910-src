/* @(#) $Revision: 37.1 $ */    
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                mknod.c                                *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "s500defs.h"
#include <sys/stat.h>
#include "sdf.h"
#include <errno.h>

#define	LIF_DIRTYPE	0xea43
#define	MAXD_LENGTH	2147483640

#define	min(a, b)	((a) < (b) ? (a) : (b))

#ifdef	DEBUG
#include <stdio.h>
extern int debug;
#endif	DEBUG

extern struct sdfinfo sdfinfo[];

char *strncpy();

/* ---------------------------------------------------- */
/* sdfmknod - create a file (other than a regular file) */
/* ---------------------------------------------------- */

/*ARGSUSED*/
sdfmknod(path, mode, dev)
register char *path;
register int mode, dev;
{
	register int fd;
	register struct sdfinfo *sp;
	char *sdftail();

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "sdfmknod(%s, (mode) 0%o, (dev) 0x%08x)\n",
		path, mode, dev);
}
#endif	DEBUG

	if (!sdfpath(path)) {
		errno = ENODEV;
		return(-1);
	}
	if (exist(path)) {
		errno = EEXIST;
		return(-1);
	}
	fd = sdfopen(path, WRITE);
	if (fd < 0)
		return(-1);
	sp = &sdfinfo[fd - SDF_OFFSET];
	sp->inode.di_mode = mode;
	if ((mode & S_IFMT) == S_IFDIR) {
		sp->inode.di_ftype = 1;
		sp->inode.di_uftype = LIF_DIRTYPE;
		sp->inode.di_max = MAXD_LENGTH;
		strncpy(sp->inode.di_name, sdftail(path), DIRSIZ+2);
		sp->inode.di_parent = sp->pinumber;
	}
	if (putino(sp, sp->inumber, &sp->inode) < 0)
		return(-1);
	sdfclose(fd);
	return(0);
}

/* ----------------------------------------- */
/* exist - SDF equivalent of access(path, 0) */
/* ----------------------------------------- */

exist(path)
register char *path;
{
	int fd;

	if (!sdfpath(path))
		return(access(path, 0));
	if ((fd = sdfopen(path, 0)) < 0)
		return(0);
	sdfclose(fd);
	return(1);
}
