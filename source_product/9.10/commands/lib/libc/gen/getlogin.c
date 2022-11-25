/* @(#) $Revision: 64.4 $ */    
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define getlogin _getlogin
#define ttyslot _ttyslot
#define open _open
#define lseek _lseek
#define read _read
#define close _close
#define strncpy _strncpy
#endif

#include <sys/types.h>
#include <utmp.h>

#define NULL 0

extern long lseek();
extern int open(), read(), close(), ttyslot();

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getlogin
#pragma _HP_SECONDARY_DEF _getlogin getlogin
#define getlogin _getlogin
#endif

char *
getlogin()
{
	register me, uf;
	struct utmp ubuf ;
	static char answer[sizeof(ubuf.ut_user)+1] ;

	if((me = ttyslot()) < 0)
		return(NULL);
	if((uf = open(UTMP_FILE, 0)) < 0)
		return(NULL);
	(void) lseek(uf, (long)(me * sizeof(ubuf)), 0);
	if(read(uf, (char*)&ubuf, sizeof(ubuf)) != sizeof(ubuf)) {
		(void) close(uf);
		return(NULL);
	}
	(void) close(uf);
	if((ubuf.ut_user[0] == '\0') || (ubuf.ut_type != USER_PROCESS))
		return(NULL);
	(void)strncpy(&answer[0],&ubuf.ut_user[0],sizeof(ubuf.ut_user)) ;
	answer[sizeof(ubuf.ut_user)] = '\0' ;
	return(&answer[0]);
}
