/* @(#) $Revision: 37.1 $ */    
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                ustat.c                                *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "s500defs.h"
#include "sdf.h"			
#include "bit.h"
#include <sys/stat.h>
#include <ustat.h>

char *ctime(), *strcpy(), *strchr();
long lseek();

extern struct sdfinfo sdfinfo[];

#ifdef	DEBUG
#include <stdio.h>
extern int debug;
#endif	DEBUG

/* ----------------------------------------------- */
/* sdfustat - return file system usage information */
/* ----------------------------------------------- */

sdfustat(devname, ustbuf)
register char *devname;
register struct ustat *ustbuf;
{
	register struct sdfinfo *sp;
	register char *sdfdev;
	register int fd;
	char *malloc();

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "sdfustat(%s, (ustbuf) 0x%08x)\n", devname, ustbuf);
}
#endif	DEBUG

	if (sdfpath(devname))
		return(-1);
	sdfdev = malloc((unsigned) (strlen(devname) + 2));
	if (sdfdev == NULL)
		nomemory();
	strcpy(sdfdev, devname);
	strcat(sdfdev, ":");
	fd = sdfopen(sdfdev, 0);
	if (fd < 0)
		return(-1);
	sp = &sdfinfo[fd - SDF_OFFSET];
	ustbuf->f_tfree = fbcount(sp);
	ustbuf->f_tinode = ustbuf->f_tfree * sp->filsys.s_blksz / FA_SIZ;
	ustbuf->f_blksize = sp->filsys.s_blksz;
	strcpy(ustbuf->f_fname, "      ");
	strcpy(ustbuf->f_fpack, "      ");
	sdfclose(fd);
	free(sdfdev);
	return(0);
}

/* ----------------------------------------------------------------- */
/* fbcount - count free blocks in an SDF file system (read free map) */
/* ----------------------------------------------------------------- */

fbcount(sp)
register struct sdfinfo *sp;
{
	register int bit, i;
	register int inum, bfree;
	register int maxblock, thisblock;
	int map[FA_SIZ / sizeof(int)];

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "fbcount(sdfinfo[%d])\n", sdffileno(sp));
}
#endif	DEBUG

	maxblock = sp->filsys.s_maxblk;
	thisblock = 0;
	for (bfree = 0, inum = FMAP_INUM; inum < sp->validi; inum++) {
		getino(sp, inum, (struct dinode *) map);
		for (i = 0; i < (FA_SIZ / sizeof(int)); i++) {
			for (bit = 0; bit < BITSPERWORD; bit++, thisblock++) {
				if (thisblock > maxblock)
					return(bfree);
				if (bitval(map[i], bit))   /* 1 == free */
					bfree++;
			}
		}
	}
	return(bfree);
}
