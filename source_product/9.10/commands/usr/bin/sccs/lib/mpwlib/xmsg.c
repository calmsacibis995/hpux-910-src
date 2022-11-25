/* @(#) $Revision: 37.2 $ */      
#ifdef NLS
#define NL_SETN 12
#include	<msgbuf.h>
#else
#define nl_msg(i, s) (s)
#endif
# include	"../../hdr/defines.h"
# include	<errno.h>


/*
	Call fatal with an appropriate error message
	based on errno.  If no good message can be made up, it makes
	up a simple message.
	The second argument is a pointer to the calling functions
	name (a string); it's used in the manufactured message.
*/

xmsg(file,func)
char *file, *func;
{
	register char *str;
	char d[FILESIZE];
	extern int errno;
	extern char Error[];

	switch (errno) {
	case ENFILE:
		sprintf(Error,nl_msg(341,"no file"));
		str = strcat(Error," (ut3)");
		break;
	case ENOENT:
		sprintf(Error,nl_msg(342,"`%s' nonexistent"),file);
		str = strcat(Error, " (ut4)");
		break;
	case EACCES:
		str = d;
		copy(file,str);
		file = str;
		sprintf(Error,nl_msg(343,"directory `%s' unwritable"), dname(file));
		str = strcat(Error, " (ut2)");
		break;
	case ENOSPC:
		sprintf(Error,nl_msg(344,"no space!"));
		str = strcat(Error, " (ut10)");
		break;
	case EFBIG:
		sprintf(Error,nl_msg(345,"write error"));
		str = strcat(Error, " (ut8)");
		break;
	default:
#ifdef NLS
		sprintmsg(Error,(nl_msg(346,"errno = %1$d, function = `%2$s'")),errno,func);
		str = strcat(Error, " (ut11)");
#else
		sprintf(str = Error,"errno = %d, function = `%s' (ut11)",errno,
			func);
#endif
		break;
	}
	return(fatal(str));
}
