
#ifdef _NAMESPACE_CLEAN
#define catgets _catgets
#define catgetmsg _catgetmsg
#endif

#include <nl_types.h>
#include <limits.h>

/*
 * The following two definitions are placed here for support of the
 * nl_msg and nl_catopen macros defined in <msgbuf.h>.  The buffer
 * nl_msg_buf, used by catgets below, is no longer directly
 * referenced by nl_msg but is kept external to maintain backward
 * compatibility with user .o files that were compiled with the
 * old version of nl_msg.
 */

nl_catd	_nl_fn = -1;
char	_nl_msg_buf[NL_TEXTMAX+1];

#ifdef _NAMESPACE_CLEAN
#undef catgets
#pragma _HP_SECONDARY_DEF _catgets catgets
#define catgets _catgets
#endif

char *catgets(catd, set_num, msg_num, s)
	nl_catd	catd;
	int 	set_num, msg_num;
	char	*s;
/*
 * This routine attempts to read message msg_num in set set_num, from
 * the message catalogue identified by the catalogue descriptor catd.
 *
 * If the message is successfully retrieved, catgets returns a pointer
 * to an internal buffer area containing the null terminated message
 * string. 
 *
 * NOTE: X/Open change to the catgets definition: if the identified message
 * is not successfully retrieved, the default string is returned.
 *
 * If catd is less than the NL_SAFEFD (smallest available file descriptor), 
 * this implies an error on catopen(). A pointer to s is returned in this case.
 * Otherwise, catgetmsg() is called.  Catgetmsg sets errno != 0 on failure.
 * If the catgetmsg call was successful, return the catgetmsg return value
 * i.e. the retrieved message, otherwise return s.
 */ 
{
	extern char	*catgetmsg();
	extern int	errno;
	int		sav_err;
	char		*msg;

	if (catd < NL_SAFEFD)
		return(s);
	else {
		sav_err = errno;
		errno = 0;
		msg=catgetmsg(catd, set_num, msg_num, _nl_msg_buf, NL_TEXTMAX);
		if (errno == 0) {
			errno = sav_err;
			return msg;
			}
		else
			return s;
		}
}
