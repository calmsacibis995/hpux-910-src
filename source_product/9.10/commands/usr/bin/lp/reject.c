/* $Revision: 66.2 $ */
/* reject [-r[reason]] dest ...  -- prevent lp from accepting requests */

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 15					/* set number */
#include <msgbuf.h>
#endif NLS

#include	"lp.h"
#ifdef TRUX
#include <sys/security.h>
#endif

char errmsg[200];
int rc_rej = 0;			/* Return code */

main(argc, argv)
int argc;
char *argv[];
{
	int i, dests = 0;
	char *arg, *reason = NULL, *trim();

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
		printf((nl_msg(1, "usage: %s [-r[reason]] dest ...\n")), argv[0]);
		exit(1);
	}

	for(i = 1; i < argc; i++) {
		arg = argv[i];
		if(*(arg) == '-') {
			if(*(arg + 1) != 'r') {
				sprintf(errmsg,(nl_msg(2, "unknown option \"%s\"")),arg);
				fatal(errmsg, 1);
			}
			else {
				reason = arg + 2;
				if(*trim(reason) == '\0')
					reason = NULL;
			}
		}
		else if(isdest(arg)) {
			dests++;
			reject(arg, reason);
		}
		else {
			dests++;
			sprintf(errmsg, (nl_msg(3, "destination \"%s\" non-existent")), arg);
			fatal(errmsg, 0);
			rc_rej = 1;
		}
	}

	if(dests == 0)
		fatal((nl_msg(4, "no destinations specified")), 1);
	exit(rc_rej);
/* NOTREACHED */
}

reject(dest, reason)
char *dest;
char *reason;
{
	struct qstat q;
	char *strcpy(), *strncpy();

	if(getqdest(&q, dest) == EOF) {
		sprintf(errmsg, (nl_msg(5, "destination \"%s\" has disappeared")), dest);
		fatal(errmsg, 0);
		rc_rej = 1;
	}
	time(&q.q_date);
	if(reason != NULL) {
		strncpy(q.q_reason, reason, Q_RSIZE);
		q.q_reason[Q_RSIZE - 1] = '\0';
	}
	else
		strcpy(q.q_reason, (nl_msg(6, "reason unknown")));

	if(q.q_accept) {
	printf((nl_msg(7, "destination \"%s\" will no longer accept requests\n")),dest);
	}
	if(!q.q_accept) {
	printf((nl_msg(8, "destination \"%s\" was already not accepting requests\n")),dest);
	}
	q.q_accept = FALSE;
	putqent(&q);
	endqent();
}

startup(name)
char *name;
{
	int catch(), cleanup();
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

	f_name = name;
	f_clean = cleanup;
	if(chdir(SPOOL) == -1)
		fatal((nl_msg(9, "spool directory non-existent")), 1);
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
