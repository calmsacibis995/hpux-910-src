/* $Revision: 70.1 $ */
/* enable printer ... - enable specified printers */

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 5					/* set number */
#include <msgbuf.h>
#endif NLS

#include	"lp.h"
#ifdef TRUX
#include <sys/security.h>
#endif

int interrupt = FALSE;
char errmsg[200];

main(argc, argv)
int argc;
char *argv[];
{
	int i, saveint(), catch();
	int errcnt = 0;
	struct pstat p;
	char reason[P_RSIZE], *trim(), *pr, *strcpy();

#ifdef SecureWare
	if(ISSECURE){
            set_auth_parameters(argc, argv);
#ifdef B1
	    if(ISB1){
        	initprivs();
        	(void) forcepriv(SEC_ALLOWMACACCESS);
	    }
#endif
	    lp_verify_printqueue();
	}
#endif
#ifdef NLS
	nl_catopen("lp");
#endif NLS

	startup(argv[0]);

	if(argc == 1) {
		printf((nl_msg(1, "usage: %s printer ...\n")), argv[0]);
		exit(1);
	}

	sprintf(reason, (nl_msg(2, "enabled")));

	for(i = 1; i < argc; i++) {
		pr = argv[i];
		if(! isprinter(pr)) {
		        errcnt++;
			sprintf(errmsg, (nl_msg(3, "printer \"%s\" non-existent")), pr);
			fatal(errmsg, 0);
		}
		else if(getpdest(&p, pr) == EOF) {
		        errcnt++;
			sprintf(errmsg, (nl_msg(4, "printer \"%s\" has disappeared!")), pr);
			fatal(errmsg, 0);
		}
		else if(p.p_flags & P_ENAB) {
		        errcnt++;
			sprintf(errmsg, (nl_msg(5, "printer \"%s\" was already enabled")), pr);
			fatal(errmsg, 0);
		}
		else {
			time(&p.p_date);
			p.p_flags |= P_ENAB;
			p.p_flags &= ~P_BUSY;
#ifdef REMOTE
			p.p_seqno = 0;
			p.p_pid = -1;
#else
			p.p_seqno = p.p_pid = 0;
#endif REMOTE
			sprintf(p.p_rdest, "-");
			strcpy(p.p_reason, reason);
			if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
				signal(SIGHUP, saveint);
			if(signal(SIGINT, SIG_IGN) != SIG_IGN)
				signal(SIGINT, saveint);
			if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
				signal(SIGQUIT, saveint);
			if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
				signal(SIGTERM, saveint);
			putpent(&p);

			/* notify scheduler of new printer status */

			enqueue(F_ENABLE, pr);
			printf((nl_msg(6, "printer \"%s\" now enabled\n")), pr);
#ifdef B1
if (ISB1)
      audit_subsystem("Attempts to enable printer(s)","Successful", ET_SUBSYSTEM
);
#endif
			if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
				signal(SIGHUP, catch);
			if(signal(SIGINT, SIG_IGN) != SIG_IGN)
				signal(SIGINT, catch);
			if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
				signal(SIGQUIT, catch);
			if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
				signal(SIGTERM, catch);
			if(interrupt) {
				cleanup();
				exit(1);
			}
		}
		endpent();
	}

	if ( errcnt ) exit(1);
	else exit(0);

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
	endpent();
}

saveint()
{
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	interrupt = TRUE;
}
