/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define sys_nerr _sys_nerr
#define sys_errlist _sys_errlist
#define catgetmsg _catgetmsg
#define catopen _catopen
#define catclose _catclose
#define strerror _strerror
#endif

#include <nl_types.h>			/* for nl_catd */

extern int errno;			/* error number */
extern int sys_nerr;			/* max error number */
extern char *sys_errlist[];		/* error message array */
extern char *catgetmsg();		/* get message from catalog */

#define MSG_LEN	 80	/* message buffer length (room for max sys_errlist + 30%) */

/*
** Description:
**	XPG3 function retrieves translated error messages without a new-line.
**	The translated message is determined by:
**		1). An error number (errnum)
**		2). The current settings of the LANG and NLSPATH
**		    environment variables through catopen().
**	The errno value is saved and restored since catopen or catgetmsg
**	might change it.  A catgetmsg is used instead of a catgets to avoid 
**	clobbering any previous catgets message by the caller (catgets returns
**	a pointer to a static buffer).  For good translated messages,
**	the message catalog is opened and closed each time through (ugh!).
**
** Future Directions:
**	The locale for error messages will be defined by setlocale through
**	a message catagory.
**
** Return Values:
**	1). NULL string -- An invalid error number (errnum).
**	2). sys_errlist[errnum] -- Either no strerror message catalog
**	    or we have a catalog with no message corresponding to the
**	    error number.
**	3). A translated message -- Only if we have a good error number
**	    and a catalog with the message.
*/

#ifdef _NAMESPACE_CLEAN
#undef strerror
#pragma _HP_SECONDARY_DEF _strerror strerror
#define strerror _strerror
#endif

char *
strerror( errnum)
register int errnum;			/* error number */
{
	register char *retval;		/* ptr to return value message */
	register int saverrno;		/* old value of errno */
	register nl_catd msgd;		/* message catalog descriptor */
	static char msgbuf[MSG_LEN];	/* error message buffer */

	if ((errnum > sys_nerr) || (errnum < 0)) {
		/* bad error number */
		retval = "";
	}
	else {
		/* good error number */
		saverrno = errno;
		if ((msgd = catopen( "strerror", 0)) == (nl_catd) -1) {
			/* unsuccessful catopen -- no message catalog */
			retval = sys_errlist[errnum];
		}
		else {
			/* successful catopen -- have a message catalog */
			if (*(retval = catgetmsg( msgd, 1, errnum + 1, msgbuf, MSG_LEN)) == '\0') {
				/* no translated message in catalog */
				retval = sys_errlist[errnum];
			}
			(void) catclose( msgd);
		}
		errno = saverrno;
	}
	return retval;
}
