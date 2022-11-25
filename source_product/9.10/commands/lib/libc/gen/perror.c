/* @(#) $Revision: 64.11 $ */      

/*LINTLIBRARY*/
/*
 * Print the error indicated
 * in the cerror cell.
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define perror _perror
#define strlen _strlen
#define strerror _strerror
#define sys_nerr _sys_nerr
#define sys_errlist _sys_errlist
#define write _write
#define catgetmsg _catgetmsg
#define catopen _catopen
#define catclose _catclose
#endif

#include <string.h>		/* for size_t, strlen and strerror */
#include <nl_types.h>		/* for nl_catd */

extern int errno;		/* error number */
extern int sys_nerr;		/* max error number */
extern char *sys_errlist[];	/* error message array */
extern nl_catd catopen();	/* open catalog */
extern char *catgetmsg();	/* get message from catalog */
extern int catclose();		/* close catalog */
extern int write();		/* write system call */

#define UKN_LEN	 25		/* room for "Unknown error" + 50% */

#ifdef _NAMESPACE_CLEAN
#undef perror
#pragma _HP_SECONDARY_DEF _perror perror
#define perror _perror
#endif

void
perror( s)
const char *s;
{
	char *c;
	size_t n;

#ifdef NLS

	/*
	** If valid errno,
	**	get translated message with strerror.
	** If invalid errno,
	** 	use strerror message catalog  to get translation of 
	**	"Unknown error".  This assumes "Unknown error" message
	**	will immediately follow sys_errlist[] error messages
	**	in the strerror message catalog.
	*/

	if (*(c = strerror( errno)) == '\0') {
		/* get translation of "Unknown error" */
		int saverrno = errno;		/* old value of errno */
		nl_catd msgd;			/* message catalog descriptor */
		static char msgbuf[UKN_LEN];	/* error message buffer */

		/* assume an untranslated message */
		c = "Unknown error";

		/* if possible get a translated message from the
			strerror message catalog */
		if ((msgd = catopen( "strerror", 0)) != (nl_catd) -1) {
			char *msg;
			if (*(msg = catgetmsg( msgd, 1, sys_nerr + 2, msgbuf, UKN_LEN)) != '\0') {
				c = msg;
			}
			(void) catclose( msgd);
		}
		errno = saverrno;
	}
#else
	if ((errno < sys_nerr) && (errno >= 0)) {
		c = sys_errlist[errno];
	}
	else {
		c = "Unknown error";
	}
#endif

	if (n = strlen(s)) {
		(void) write( 2, s, (unsigned int) n);
		(void) write( 2, ": ", 2);
	}

	(void) write( 2, c, (unsigned int) strlen( c));
	(void) write( 2, "\n", 1);
}
