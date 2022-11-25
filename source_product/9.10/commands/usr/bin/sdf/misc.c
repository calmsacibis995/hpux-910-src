/* @(#) $Revision: 37.1 $ */     
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                misc.c                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include "s500defs.h"
#include "sdf.h"

char *strchr(), *strrchr();

extern struct sdfinfo sdfinfo[];

/* ------------------------------------------------- */
/* sdfpath - return non-zero if pathname has a colon */
/* ------------------------------------------------- */

sdfpath(path)
register char *path;
{
	return(strchr(path, ':') != (char *) NULL);
}

/* --------------------------------------------------- */
/* sdffile - return non-zero if file descriptor is SDF */
/* --------------------------------------------------- */

sdffile(fd)
register int fd;
{
	return(fd >= SDF_OFFSET);
}

/* ----------------------------------------------------- */
/* sdftail - return pointer to the file name of SDF path */
/* ----------------------------------------------------- */

char *
sdftail(s)
register char *s;
{
	register char *p, *t;
	char *sdfname();

	t = sdfname(s);
	if ((p = strrchr(t, '/')) != (char *) NULL)
		return(++p);
	return(t);
}

/* ------------------------------------------------------ */
/* sdfname - return pointer to SDF portion of device:path */
/* ------------------------------------------------------ */

char *
sdfname(s)
register char *s;
{
	register char *p;
	char *strchr();

	if ((p = strchr(s, ':')) == (char *) NULL)
		return((char *) NULL);
	return(++p);
}

/* -------------------------------------------------- */
/* sdfdir2str - NULL terminate a directory entry name */
/* -------------------------------------------------- */

void
sdfdir2str(s)
register char *s;
{
	register char *cp, *max;

	max = s + DIRSIZ + 2;	/* SDF file names are DIRSIZ + 2 bytes long */
	for (cp = s; cp < max; cp++)
		if (*cp == ' ')	/* SDF file names are not NULL terminated */
			*cp = NULL;
	return;
}

/* ---------------------------------- */
/* bcopy - copy a block from s2 to s1 */
/* ---------------------------------- */

bcopy(s1, s2, bytes)
register char *s1, *s2;
register int bytes;
{
	for ( ; bytes > 0; bytes--)
		*s1++ = *s2++;
}

#ifdef	DEBUG
/* ------------------------------------------------------------------- */
/* sdffileno - return the index into the sdfinfo table given a pointer */
/* ------------------------------------------------------------------- */

sdffileno(sp)
register struct sdfinfo *sp;
{
	return(sp - sdfinfo);
}
#endif	DEBUG
