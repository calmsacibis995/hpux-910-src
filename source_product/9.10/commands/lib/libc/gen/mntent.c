/* Header: $Header: mntent.c,v 66.11 91/07/01 16:37:41 ssa Exp $ */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#ifdef __lint
#  define isdigit _isdigit
#  define isspace _isspace
#endif /* __lint  */
#define fclose _fclose
#define fgets _fgets
#define fileno _fileno
#define fdopen _fdopen
#define ftruncate _ftruncate
#define open _open
#define close _close
#define fputc _fputc
#define fputs _fputs
#define fseek _fseek
#define lockf _lockf
#define ltoa _ltoa
#define stat _stat
#define strcpy _strcpy
#define strlen _strlen
#define strncmp _strncmp
#ifdef LOCAL_DISK
#define strcmp _strcmp
#endif /* LOCAL_DISK */
#define addmntent _addmntent
#define endmntent _endmntent
#define getmntent _getmntent
#define setmntent _setmntent
#define hasmntopt _hasmntopt
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <mntent.h>
#ifdef hpux
#include <unistd.h>
#else not hpux
#include <sys/file.h>
#endif not hpux
#ifdef LOCAL_DISK
#include <sys/stat.h>
#endif /* LOCAL_DISK */

static	struct mntent mnt;
#ifdef hpux
#ifdef BUFSIZ
#undef BUFSIZ
#endif /* BUFSIZ defined */
#define BUFSIZ 1024
#endif /* hpux */
static	char line[BUFSIZ+1];


static char *
mntstr(p)
	register char **p;
{
	char *cp = *p;
	char *retstr;

	while (*cp && isspace(*cp))
		cp++;
	retstr = cp;
	while (*cp && !isspace(*cp))
		cp++;
	if (*cp) {
		*cp = '\0';
		cp++;
	}
	*p = cp;
	return (retstr);
}

static int
mntdigit(p)
	register char **p;
{
	register int value = 0;
	char *cp = *p;
	char *retstr;

	while (*cp && isspace(*cp))
		cp++;
	for (; *cp && isdigit(*cp); cp++) {
		value *= 10;
		value += *cp - '0';
	}
	while (*cp && !isspace(*cp))
		cp++;
	if (*cp) {
		*cp = '\0';
		cp++;
	}
	*p = cp;
	return (value);
}

static
mnttabscan(mnttabp, mnt)
	FILE *mnttabp;
	struct mntent *mnt;
{
	char *cp;

	do {
	    do {
		    cp = fgets(line, BUFSIZ, mnttabp);
		    if (cp == NULL) {
			    return (EOF);
		    }
	    } while (*cp == '#');

	    while ((*cp == '\t') || (*cp == ' '))
		cp++;

	} while (*cp == '\n');

	mnt->mnt_fsname = mntstr(&cp);
	if (*cp == '\0')
		return (1);
	mnt->mnt_dir = mntstr(&cp);
	if (*cp == '\0')
		return (2);
	mnt->mnt_type = mntstr(&cp);
	if (*cp == '\0')
		return (3);
	mnt->mnt_opts = mntstr(&cp);
	if (*cp == '\0')
		return (4);
	mnt->mnt_freq = mntdigit(&cp);
	if (*cp == '\0')
		return (5);
	mnt->mnt_passno = mntdigit(&cp);
	if (*cp == '\0')
		return (6);
	mnt->mnt_time = mntdigit(&cp);
#ifdef LOCAL_DISK
	if (*cp == '\0')
		return (7);
	mnt->mnt_cnode = mntdigit(&cp);
	return(8);
#else /* LOCAL_DISK */
	return (7);
#endif /* LOCAL_DISK */
}
	
/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef setmntent
#pragma _HP_SECONDARY_DEF _setmntent setmntent
#define setmntent _setmntent
#endif

FILE *
setmntent(fname, flag)
	char *fname;
	char *flag;
{
	FILE *mnttabp;
	int lock;
	char *fflag=flag;
	int fd,plus,oflag;
	int save_errno;

	lock = F_TEST;
	while (*fflag) {
		if (*fflag == 'w' || *fflag == 'a' || *fflag == '+') {
			lock = F_TLOCK;
		}
		fflag++;
	}
	/* we need to interpret the mode flags so that we can
	   eventually do an fdopen on our file descriptor.  */
	plus = ((flag[1]=='+') || ((flag[1]=='b') && (flag[2]=='+')));
	switch (flag[0])
	{
	case 'w':
		oflag = (plus ? O_RDWR : O_WRONLY) | O_CREAT;
		break;
	case 'a':
		oflag = (plus ? O_RDWR : O_WRONLY) | O_APPEND | O_CREAT;
		break;
	case 'r':
		oflag = plus ? O_RDWR : O_RDONLY;
		break;
	default:
		errno = EINVAL;
		return NULL;
	}
	/* open the file using the supplied modes */
	if ((fd=open(fname,oflag,0666))<0)
		return NULL;
	/* is there an advisory lock on this file? */
	if (lockf(fd, lock, 0) < 0) {
		save_errno=errno;
		close(fd);
		errno=save_errno;
		return NULL;
	}
	/* since we don't want to truncate a file before we
	   get a lock, we wait until now. */
	if (flag[0]=='w') {
		if (ftruncate(fd,0)<0) {
			save_errno=errno;
			close(fd);
			errno=save_errno;
			return NULL;
		}
	}
	if ((mnttabp=fdopen(fd,flag)) == NULL) {
		save_errno=errno;
		close(fd);
		errno=save_errno;
	}
	return (mnttabp);
}


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef endmntent
#pragma _HP_SECONDARY_DEF _endmntent endmntent
#define endmntent _endmntent
#endif

int
endmntent(mnttabp)
	FILE *mnttabp;
{
	if (mnttabp) {
		fclose(mnttabp);
	}
	return (1);
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getmntent
#pragma _HP_SECONDARY_DEF _getmntent getmntent
#define getmntent _getmntent
#endif

struct mntent *
getmntent(mnttabp)
	FILE *mnttabp;
{
	int nfields;
#ifdef LOCAL_DISK
        struct stat	dev_statbuf;
#endif /* LOCAL_DISK */

	if (mnttabp == NULL)
	    return ((struct mntent *)NULL);

	mnt.mnt_fsname = mnt.mnt_dir = mnt.mnt_type = mnt.mnt_opts = NULL;
	mnt.mnt_freq = mnt.mnt_passno = -1;
	mnt.mnt_time = 0;
#ifdef LOCAL_DISK
	mnt.mnt_cnode = 0;
#endif /* LOCAL_DISK */

	nfields = mnttabscan(mnttabp, &mnt);

	if ((nfields == EOF) || (nfields < 1))
	    return ((struct mntent *)NULL);
	return (&mnt);
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef addmntent
#pragma _HP_SECONDARY_DEF _addmntent addmntent
#define addmntent _addmntent
#endif

addmntent(mnttabp, mnt)
	FILE *mnttabp;
	struct mntent *mnt;
{
	if (fseek(mnttabp, 0, SEEK_END) < 0) {
		return (1);
	}
	mntprtent(mnttabp, mnt);
	return (0);
}

static char tmpopts[256];

static char *
mntopt(p)
	char **p;
{
	char *cp = *p;
	char *retstr;

	while (*cp && isspace(*cp))
		cp++;
	retstr = cp;
	while (*cp && *cp != ',')
		cp++;
	if (*cp) {
		*cp = '\0';
		cp++;
	}
	*p = cp;
	return (retstr);
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef hasmntopt
#pragma _HP_SECONDARY_DEF _hasmntopt hasmntopt
#define hasmntopt _hasmntopt
#endif

char *
hasmntopt(mnt, opt)
	register struct mntent *mnt;
	register char *opt;
{
	char *f, *opts;
	int optlen = strlen(opt);
	char *rw	= MNTOPT_RW;
	char *suid	= MNTOPT_SUID;
#ifdef QUOTA
	char *noquota	= MNTOPT_NOQUOTA;
#endif
	char *fg	= MNTOPT_FG;
	char *hard	= MNTOPT_HARD;
	char *intr	= MNTOPT_INTR;
	char *devs	= MNTOPT_DEVS;

	strcpy(tmpopts, mnt->mnt_opts);
	opts = tmpopts;
	f = mntopt(&opts);
	for (; *f; f = mntopt(&opts)) {
		if ((strncmp(opt, f, optlen) == 0) &&
			    ((f[optlen] == '\0') || (f[optlen] == ',') ||
			     (f[optlen] == '=')))
		    return (f - tmpopts + mnt->mnt_opts);
	} 

	/*

	 Last ditch attempt:

	 This stuff tells you a default is set if the non-default is
	 *not* set.  This compensates for update_mnttab's stripping
	 default info.  It handles only the boolean options and does
	 nothing about options of the form optname=xxx.

	 It is reentrant.  The following should be carefully considered
	 when making further changes below this point:

	 1) Reentrancy should normally be avoided, especially in this
	 routine because char tmpopts[] is not on the stack and is, in
	 fact, reused.

	 2) In this case we will only reenter once (i.e.  to test for
	 "quota" if "noquota" is being checked for).

	 3) This will cause tmpopts to be overwritten, but at this point
	 we're done with tmpopts, so trashing it won't hurt anything.

	 The return value in the cases below really stretches the
	 specification in getmntent(3X), since the pointer returned
	 doesn't really point into the mnt->mnt_opts data, but to a
	 pseudo-mnt_opts.  Some really extraordinary assumptions, like
	 access to other parts of the string or structure relative to
	 this address, would be broken.

	 */

#define optis(s)	(strcmp(opt, s) == 0)
#define typeis(type)	(strcmp(mnt->mnt_type, type) == 0)
	if (optis(rw)) {
	    if (!hasmntopt(mnt, MNTOPT_RO))
		return rw;
	    else return NULL;
	}
	if (optis(suid)) {
	    if (!hasmntopt(mnt, MNTOPT_NOSUID))
		return suid;
	    else return NULL;
	}
#ifdef QUOTA
	if (optis(noquota)) {
	    if (typeis(MNTTYPE_HFS) && !hasmntopt(mnt, MNTOPT_QUOTA))
		return noquota;
	    else return NULL;
	}
#endif
	if (optis(fg)) {
	    if (typeis(MNTTYPE_NFS) && !hasmntopt(mnt, MNTOPT_BG))
		return fg;
	    else return NULL;
	}
	if (optis(hard)) {
	    if (typeis(MNTTYPE_NFS) && !hasmntopt(mnt, MNTOPT_SOFT))
		return hard;
	    else return NULL;
	}
	if (optis(intr)) {
	    if (typeis(MNTTYPE_NFS) && !hasmntopt(mnt, MNTOPT_NOINTR))
		return intr;
	    else return NULL;
	}
	if (optis(devs)) {
	    if (typeis(MNTTYPE_NFS) && !hasmntopt(mnt, MNTOPT_NODEVS))
		return devs;
	    else return NULL;
	}

	return (NULL);
}

static
mntprtent(mnttabp, mnt)
	FILE *mnttabp;
	register struct mntent *mnt;
{
    extern char *ltoa();

    if ((mnt->mnt_fsname != NULL) && (mnt->mnt_fsname[0] != '\0'))
    {
	fputs(mnt->mnt_fsname, mnttabp);
	if ((mnt->mnt_dir != NULL) && (mnt->mnt_dir[0] != '\0'))
	{
	    fputc(' ', mnttabp);
	    fputs(mnt->mnt_dir, mnttabp);
	    if ((mnt->mnt_type != NULL) && (mnt->mnt_type[0] != '\0'))
	    {
		fputc(' ', mnttabp);
		fputs(mnt->mnt_type, mnttabp);
		if ((mnt->mnt_opts != NULL) && (mnt->mnt_opts[0] != '\0'))
		{
		    fputc(' ', mnttabp);
		    fputs(mnt->mnt_opts, mnttabp);
		    if (mnt->mnt_freq >= 0)
		    {
			fputc(' ', mnttabp);
			fputs(ltoa(mnt->mnt_freq), mnttabp);
			if (mnt->mnt_passno >= 0)
			{
			    fputc(' ', mnttabp);
			    fputs(ltoa(mnt->mnt_passno), mnttabp);
			    if (mnt->mnt_time != 0)
			    {
				fputc(' ', mnttabp);
				fputs(ltoa(mnt->mnt_time), mnttabp);
			    }
#ifdef LOCAL_DISK
			    if (mnt->mnt_cnode >= 0)
			    {
				fputc(' ', mnttabp);
				fputs(ltoa(mnt->mnt_cnode), mnttabp);
			    }
#endif /* LOCAL_DISK */
			}
		    }
		}
	    }
	}
    }

    fputc('\n', mnttabp);
    return 0;
}
