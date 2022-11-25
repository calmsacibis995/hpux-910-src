/* @(#) $Revision: 70.1 $ */     
/******************************************
  Getwd: get current directory
 ******************************************/
#include	"sh.local.h"
#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include        <ndir.h>

#define	dot	"."
#define	dotdot	".."
#if defined(DISKLESS) || defined(DUX)
#define dotdotplus "..+"
#endif

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#include <nl_types.h>
nl_catd nlmsg_fd;
#define NL_SETN 2	/* set number */
#endif NLS

#ifndef NONLS
typedef unsigned short int 	CHAR;
extern CHAR *to_short();
#else
typedef char			CHAR;
#define to_short
#endif

static	char	*name;

static  DIR     *dirfptr;
static	int	off	= -1;
static	struct	stat	d, dd;
static  struct  direct  *dir;
#if defined(DISKLESS) || defined(DUX)
static  int dothidden;
static  struct stat ddplus;
static  void hstat();
#endif

CHAR *
getwd(np)
CHAR *np;
{
	int rdev, rino;

	*np++ = '/';
	*np = 0;		/* in case you're already at '/' */
	name = (char *)np;
	stat("/", &d);
	rdev = d.st_dev;
	rino = d.st_ino;
	for (;;) {
		stat(dot, &d);
		if (d.st_ino==rino && d.st_dev==rdev)
			goto done;

#if defined(DISKLESS) || defined(DUX)
		if ((d.st_mode & S_ISUID) != 0)
			dothidden = 1;
		else
			dothidden = 0;
#endif

		if ((dirfptr = opendir(dotdot)) == (DIR *)0)
			prexit((catgets(nlmsg_fd,NL_SETN,1, "getwd: cannot open ..\n")));
		fstat(dirfptr->dd_fd, &dd);

#if defined(DISKLESS) || defined(DUX)
		if (stat(dotdotplus,&ddplus) == 0) {
			if (dd.st_dev != ddplus.st_dev
			    || dd.st_ino != ddplus.st_ino)
				d = ddplus; /* structure assignment */
		}
#endif

		chdir(dotdot);

		if(d.st_dev == dd.st_dev) {
			if(d.st_ino == dd.st_ino) {
				closedir(dirfptr);
				goto done;
			}

			do
				if ((dir = readdir(dirfptr)) == (struct direct *)0)
					prexit((catgets(nlmsg_fd,NL_SETN,2, "getwd: read error in ..\n")));
			while (dir->d_ino != d.st_ino);
		}
		else do {
				if ((dir = readdir(dirfptr)) == (struct direct *)0)
					prexit((catgets(nlmsg_fd,NL_SETN,3, "getwd: read error in ..\n")));

/* Fix for DTS DSDe419267: stat() changed to lstat(), 'cos
   stat() follows the symlinks whereas lstat() does not.
   csh used to hang even if the directory contained a symlink to
   an nfs mountpoint which is down.
*/
#if defined(DISKLESS) || defined(DUX)
				if (dothidden)
				    hstat(dir->d_name, &dd);
				else
				    lstat(dir->d_name, &dd);
#else
				lstat(dir->d_name, &dd);
#endif
			} while(dd.st_ino != d.st_ino || dd.st_dev != d.st_dev);
		closedir(dirfptr);
		cat();
	}
done:
	name--;
	if (chdir(name) < 0)
		prexit((catgets(nlmsg_fd,NL_SETN,4, "getwd: can't change back\n")));
	return (to_short(name));
}

cat()
{
	register i, j;

	i = -1;
	while (dir->d_name[++i] != 0)
	    ;

#if defined(DISKLESS) || defined(DUX)
	if (dothidden)
	    i++;
#endif

	if ((off+i+2) > 1024-1)
		return;
	for(j=off+1; j>=0; --j)
		name[j+i+1] = name[j];
	if (off >= 0)
		name[i] = '/';
	off=i+off+1;
	name[off] = 0;
#if defined(DISKLESS) || defined(DUX)
	if (dothidden)
	    name[--i] = '+';
#endif
	for(--i; i>=0; --i)
		name[i] = dir->d_name[i];
}

#if defined(DISKLESS) || defined(DUX)
static void
hstat(file,sbufptr)
	char *file;
	struct stat *sbufptr;
{
	char fbuf[MAXPATHLEN];

	if ((strlen(file) + 2) > MAXPATHLEN)
		prexit((catgets(nlmsg_fd,NL_SETN,5, "getwd: current directory name too long\n")));
	strcpy(fbuf,file);
	strcat(fbuf,"+");
	lstat(fbuf,sbufptr);
	return;
}
#endif

prexit(cp)
char *cp;
{
	write(FSHDIAG, cp, strlen(cp));

#ifdef DEBUG_EXIT
  printf ("prexit (1): %d, Calling exit (1)\n", getpid ());
#endif

	exit(1);
}
