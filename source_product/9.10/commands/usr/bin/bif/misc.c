
/* LINTLIBRARY */

#include <stdio.h>
#include <sys/types.h>
#include "bif.h"

char *ctime(), *strchr(), *strrchr();

boolean debug;
#define triple(p) (((p)[0]<<16) + ((p)[1]<<8) + (p)[2])

dump_ino(p)
struct dinode *p;
{
	int bnum;

	if (!debug)
		return;

	bugout("*** Dump of inode ***");
	bugout("di_mode: 0%o",	p->di_mode);
	bugout("di_nlink: %d",	p->di_nlink);
	bugout("di_uid: %d",	p->di_uid);
	bugout("di_gid: %d",	p->di_gid);
	bugout("di_size: %d",	p->di_size);
	bugout("atime: %.24s",	ctime(&p->di_atime));
	bugout("mtime: %.24s",	ctime(&p->di_mtime));
	bugout("ctime: %.24s",	ctime(&p->di_ctime));

	printf("addresses: ");
	for (bnum=0; bnum<=12; bnum++) {
		unsigned char *addrs;
		int addr;

		addrs = (unsigned char *) &p->di_addr[bnum*3];
		addr = triple(addrs);
		printf("%d ", addr);
	}
	bugout("\n");
}


dump_sb(p)
struct filsys *p;
{
	char *s;
	bugout("*** Dump of super-block ***");
	bugout("s_isize: %d",	p->s_isize);
	bugout("s_fsize: %d",	p->s_fsize);
	bugout("s_nfree: %d",	p->s_nfree);
	bugout("s_ninode: %d",	p->s_ninode);
	bugout("s_flock: %d",	p->s_flock);
	bugout("s_ilock: %d",	p->s_ilock);
	bugout("s_fmod: %d",	p->s_fmod);
	bugout("s_ronly: %d",	p->s_ronly);
	bugout("s_time: %.24s",	ctime(&p->s_time));
	bugout("s_tfree: %d",	p->s_tfree);
	bugout("s_tinode: %d",	p->s_tinode);
	bugout("s_fname: %.6s",	(s=p->s_fname, *s) ? s : "<null>");
	bugout("s_fpack: %.6s",	(s=p->s_fpack, *s) ? s : "<null>");
	bugout("");
}



bfsfile(fd)
{
	return fd>=1000;
}


bfspath(s)
char *s;
{
	return strchr(s, ':')!=NULL;
}


char *
bfstail(s)
register char *s;
{
	register char *p;

	p=strchr(s, ':');	/* find leftmost colon */
	if (p!=NULL)		/* if we found a colon */
		s = p+1;	/* go past it */
	p = strrchr(s, '/');	/* find rightmost slash */
	if (p!=NULL)		/* if we found one */
		s = p+1;	/* go past it */
	return s;
}


bcopy(dest, source)
register char *dest, *source;
{
	register int i;

	i=BSIZE;
	do
		*dest++ = *source++;
	while (--i > 0);
}

char * normalize(path)
char * path;	/* return a path with single ajacent '/'s and none on end of path */
{
	char *ppath, *pathsave;
	register int len;
	register boolean doit = false;

	pathsave = ppath = path;
	while(*path != '\0')
	{
		if(*path != '/')
		{
			doit = false;
			*ppath++ = *path;
		}
		else		/* check for normalization needs */
		{
			if(doit == false)	/* first one */
			{
				doit = true;
				*ppath++ = *path;
			}
		}
		path++;
	}
	*ppath = '\0';
	len = strlen(pathsave);
	if(*(pathsave+len-1) == '/')	/* strip off trailing '/' */
		*(pathsave+len-1) = '\0';
	return(pathsave);
}
