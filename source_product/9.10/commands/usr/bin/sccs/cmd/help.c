static char *HPUX_ID = "@(#) $Revision: 37.2 $";
#ifdef NLS
#define NL_SETN  5
#include	<msgbuf.h>
#else
#define nl_msg(i, s) (s)
#endif
#include	<stdio.h>
#include	<sys/types.h>
#include	<macros.h>


char	Ohelpcmd[]   =   "/usr/lib/help/lib/help2";
extern	int	errno;

main(argc,argv)
int argc;
char *argv[];
{

#ifdef NLS
	nl_catopen("sccs");
#endif

	execv(Ohelpcmd,argv);

#ifdef NLS 
	fprintmsg(stderr,nl_msg(1,"help: Could not exec: %1$s.  Errno=%2$d"),Ohelpcmd,errno);
	fprintf(stderr,"\n");
#else
	fprintf(stderr,"help: Could not exec: %s.  Errno=%d\n",Ohelpcmd,errno);
#endif  
	exit(1);

}
