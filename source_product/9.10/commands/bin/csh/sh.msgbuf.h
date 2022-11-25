/* @(#) $Revision: 49.2 $ */     

/* <msgbuf.h> includes <stdio.h>.  But csh cannot include <stdio.h>. */

#ifndef O_RDONLY
#include <fcntl.h>
#endif

#define NL_MBUFLN 1024

#ifndef NL_SETN 
#define NL_SETN 1
#endif

extern	int	nl_fn;
extern	char	*nl_lang;
extern	char	nl_msg_buf[];
char	*getmsg();

#define nl_msg(i, s) (*getmsg(nl_fn, NL_SETN, i, nl_msg_buf, NL_MBUFLN) == '\0' ? (s) : nl_msg_buf)

#define NLSDIR "/usr/lib/nls/"

extern char _fn_buf[];
char *getenv(), *strcpy(), *strcat();

#define nl_catopen(cmdn) \
	strcpy(_fn_buf, NLSDIR);\
	if ((nl_lang = getenv("LANG")) != NULL) {\
		strcat (_fn_buf, nl_lang);\
		strcat (_fn_buf, "/");\
		strcat (_fn_buf, cmdn);\
		strcat (_fn_buf, ".cat");\
		if ((nl_fn = open(_fn_buf, O_RDONLY)) != -1)\
			fcntl(nl_fn, F_SETFD, 1);\
	} else\
		nl_fn = -1;
