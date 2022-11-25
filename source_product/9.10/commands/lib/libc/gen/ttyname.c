/* @(#) $Revision: 64.5 $ */     
/*LINTLIBRARY*/
/*
 * ttyname(f): return "/dev/[YYY/]ttyXX" which the the name of the
 * tty belonging to file f. This routine rejects /dev/syscon and
 * /dev/systty, which are links to other device names.
 *
 * This program works in two passes: the first pass tries to
 * find the device by matching device and inode numbers; if
 * that doesn't work, it tries a second time, this time doing a
 * stat on every file in /dev and trying to match device numbers
 * only. If that fails too, NULL is returned.
 */

#ifdef _NAMESPACE_CLEAN
#define open      _open
#define read      _read
#define close     _close
#define stat      _stat
#define fstat     _fstat
#define isatty    _isatty
#define strcpy    _strcpy
#define strcat    _strcat
#define lseek     _lseek
#define ttyname   _ttyname
#define opendir   _opendir
#define readdir   _readdir
#define closedir  _closedir
#define strcmp   _strcmp
#define strncmp   _strncmp
#endif

#define	NULL	0
#include <sys/types.h>
#include <ndir.h>
#include <sys/stat.h>

extern int open(), read(), close(), stat(), fstat(), isatty();
extern char *strcpy(), *strcat();
extern long lseek();

static char rbuf[64];

static char *dirlist[] = {  /* dirlist contains a list of directories to */
	"/dev/",            /* search for tty devices.                   */
	"/dev/pty/",
	(char *)0
};

#ifdef _NAMESPACE_CLEAN
#undef ttyname
#pragma _HP_SECONDARY_DEF _ttyname ttyname
#define ttyname _ttyname
#endif

char *
ttyname(f)
int	f;
{
	struct stat fsb, tsb;
	struct direct *db;
	register int pass1;
	register char **dirlistptr,*ttydir;
	DIR *dirp;

	if(isatty(f) == 0)
		return(NULL);
	if(fstat(f, &fsb) < 0)
		return(NULL);
	if((fsb.st_mode & S_IFMT) != S_IFCHR)
		return(NULL);
	pass1 = 1;
	do {
		dirlistptr = dirlist;
		for (ttydir = *dirlistptr; ttydir != (char *)0; ttydir = *++dirlistptr) {
			if((dirp = opendir(ttydir)) == (DIR *)0)
				continue;
			while((db = readdir(dirp)) != (struct direct *)0) {
				if(pass1 && db->d_ino != fsb.st_ino)
					continue;
				if (strcmp(ttydir,"/dev/") == 0 &&
				      ( strncmp(db->d_name,"syscon", 6) == 0 ||
					strncmp(db->d_name,"systty", 6) == 0))
					continue;
				(void) strcpy(rbuf, ttydir);
				(void) strcat(rbuf, db->d_name);
				if(stat(rbuf, &tsb) < 0)
					continue;
				if(tsb.st_rdev == fsb.st_rdev &&
					(tsb.st_mode&S_IFMT) == S_IFCHR &&
					(!pass1 || tsb.st_ino == fsb.st_ino)) {
					(void) closedir(dirp);
					return(rbuf);
				}
			}
			(void) closedir(dirp);
		}
	} while(pass1--);
	return(NULL);
}
