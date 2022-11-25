/* $Revision: 66.2 $ */
/* accept dest ... - allow lp to accept requests */

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1					/* set number */
#include <msgbuf.h>
#endif NLS

#include	"lp.h"

char errmsg[200];

main(argc, argv)
int argc;
char *argv[];
{
	int i;
	int rc_acc = 0;			/* Return code */
	struct qstat q;
	char reason[Q_RSIZE], *trim(), *dest, *strcpy();

#ifdef SecureWare
	if(ISSECURE){
            set_auth_parameters(argc, argv);
#ifdef B1
	    if(ISB1){
        	initprivs();
        	(void) forcepriv(SEC_ALLOWMACACCESS);
	    }
#endif
	}
#endif
#ifdef NLS
	nl_catopen("lp");
#endif NLS

	startup(argv[0]);

	if(! ISADMIN)
		fatal(ADMINMSG, 1);

	if(argc == 1) {
		printf((nl_msg(1, "usage: %s dest ...\n")), argv[0]);
		exit(1);
	}

	sprintf(reason, (nl_msg(2, "accepting")));

	for(i = 1; i < argc; i++) {
		dest = argv[i];
		if(!isdest(dest)) {
			sprintf(errmsg, (nl_msg(3, "destination \"%s\" non-existent")), dest);
			fatal(errmsg, 0);
			rc_acc = 1;
		}
		else if(getqdest(&q, dest) == EOF) {
			sprintf(errmsg, (nl_msg(4, "destination \"%s\" has disappeared!")), dest);
			fatal(errmsg, 0);
			rc_acc = 1;
		}
		else if(q.q_accept) {
			sprintf(errmsg, (nl_msg(5, "destination \"%s\" was already accepting requests")),
			  dest);
			fatal(errmsg, 0);
			rc_acc = 1;
		}
		else {
			q.q_accept = TRUE;
			time(&q.q_date);
			strcpy(q.q_reason, reason);
			putqent(&q);
			printf((nl_msg(6, "destination \"%s\" now accepting requests\n")), dest);
		}
		endqent();
	}

	exit(rc_acc);
/* NOTREACHED */
}

startup(name)
char *name;
{
	int catch(), cleanup();
	extern char * f_name;
	extern int (*f_clean)();

	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, catch);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, catch);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, catch);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, catch);

	f_name = name;
	f_clean = cleanup;
	if(chdir(SPOOL) == -1)
		fatal((nl_msg(7, "spool directory non-existent")), 1);
}

/* catch -- catch signals */

catch()
{
	int cleanup();
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	cleanup();
	exit(1);
}

cleanup()
{
	endqent();
}
