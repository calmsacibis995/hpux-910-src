/* $Revision: 66.1 $ */
/* lpfence -- set the fence priority of printer */

#ifndef NLS
#define nl_msg(i,s) (s)
#else NLS
#define NL_SETN 20				/* set number */
#include <msgbuf.h>
#endif NLS

#include "lp.h"
#include "lpsched.h"
#ifdef TRUX
#include <sys/security.h>
#endif TRUX

char	printer[DESTMAX];			/* printer name */
char	work[BUFSIZ];
short	fence;

void
main(argc,argv)
int	argc;
char	*argv[];
{
	struct	pstat	p;

#ifdef TRUX
	if(ISB1){
		set_auth_parameters(argc, argv);
		initprivs();
		forcepriv(SEC_ALLOWMACACCESS);
		forcepriv(SEC_IDENTITY);
	}
#endif TRUX

	startup(argv[0]);
	getargument(argc,argv);
	
	getpdest(&p,printer);
	p.p_fence = fence;
	putpent(&p);
	endpent();
	exit(0);
/* NOTREACHED */
}


/* startup -- initialization routine */

startup(name)
char *name;
{
	int catch(), cleanup();
	struct passwd *adm, *getpwnam();
	extern char *f_name;
	extern int (*f_clean)();

	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, catch);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, catch);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, catch);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, catch);

	umask(0022);
	f_name = name;
	f_clean = cleanup;

	if(! ISADMIN)
		fatal(ADMINMSG, 1);

	if((adm = getpwnam(ADMIN)) == NULL)
		fatal((nl_msg(1,
		    "LP Administrator not in password file\n")), 1);

	if(setresuid(adm->pw_uid, -1, -1) == -1)
		fatal((nl_msg(2,
		    "can't set user id to LP Administrator's user id")), 1);

	if(chdir(SPOOL) == -1)
		fatal((nl_msg(3, "spool directory non-existent")), 1);

	if(enqueue(F_NOOP, "") == 0)	/* scheduler is running ? */
		fatal((nl_msg(4, "can't proceed - scheduler running")), 1);
}


/* usage -- command line usage */

void
usage(name)
char	*name;
{
	sprintf(work, (nl_msg(7,"usage : %s printer fence\n")), name);
	fatal(work, 1);
}


/* catch -- catch signals */

catch()
{
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	cleanup();
	exit(1);
}


/* cleanup -- called by catch() after interrupts or by fatal() after errors */

cleanup()
{
	endpent();
}


/* getargument -- get arguments ( name of printer, fence value ) */

getargument(argc,argv)
int	argc;
char	*argv[];
{
	if(argc != 3)
	    usage(argv[0]);

	if(!isprinter(strcpy(printer,argv[1]))){
		sprintf(work, (nl_msg(5, "no such printer \"%s\"")) ,printer);
		fatal(work, 1);
	}

	if(!isdigit((int)*argv[2]))
	    usage(argv[0]);

	fence = (short)atoi(argv[2]);	

	if(fence < MINPRI || fence > MAXPRI)
	    fatal((nl_msg(6, "fence value must be 0 to 7")), 1);
}
