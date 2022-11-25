/* $Revision: 70.1 $ */
/* disable [-c] [-rreason] printer ...  --  disable printers */

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 4					/* set number */
#include <msgbuf.h>
#endif NLS

#include	"lp.h"

int interrupt = FALSE;
char errmsg[200];

main(argc, argv)
int argc;
char *argv[];
{
	int i, cancel = FALSE, printers = 0;
	char *reason = 0;
	char *trim(), *arg;

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
		printf((nl_msg(1, "usage: %s [-c] [-r[reason]] printer ...\n")), argv[0]);
		exit(0);
	}

	for(i = 1; i < argc; i++) {
		if(strncmp(argv[i], "-c", 2) == 0) {
			cancel = TRUE;
			argv[i] = NULL;
		}
	}

	for(i = 1; i < argc; i++) {
		arg = argv[i];
		if(arg == NULL)
			continue;
		if(*(arg) == '-') {
			if(*(arg + 1) != 'r') {
				sprintf(errmsg, (nl_msg(2, "unknown option \"%s\"")), arg);
				fatal(errmsg, 1);
			}
			else {
				reason = arg + 2;
				if(*trim(reason) == '\0')
					reason = NULL;
			}
		}
		else if(! isprinter(arg)) {
			printers++;
			sprintf(errmsg, (nl_msg(3, "printer \"%s\" non-existent")), arg);
			fatal(errmsg, 0);
		}
		else {
			printers++;
			disable(arg, reason, cancel);
		}
	}

	if(printers == 0)
		fatal((nl_msg(4, "no printers specified")), 1);

	exit(0);
/* NOTREACHED */
}

disable(pr, reason, cancel)
char *pr;
char *reason;
int cancel;
{
	struct pstat p;
	struct outq o;
	char *strncpy(), *strcpy();

	setoent();
	if(getpdest(&p, pr) == EOF) {
		sprintf(errmsg, (nl_msg(5, "printer \"%s\" has disappeared!")), pr);
		fatal(errmsg, 0);
	}
	else if(! (p.p_flags & P_ENAB)) {
		sprintf(errmsg, (nl_msg(6, "printer \"%s\" was already disabled")), pr);
		fatal(errmsg, 0);
	}
	else {
		setsigs();
		if(p.p_flags & P_BUSY) {
			kill(-p.p_pid, SIGTERM);
			if(getoid(&o, p.p_rdest, p.p_seqno) != EOF) {
				if(cancel) {
#ifdef REMOTE
					if (p.p_rflags & P_ORC)
					{
					    char *user_name();
					    char *ruid_name;

					    if ((int)geteuid() == 0 || !strcmp(ruid_name = user_name(), "lp") || !strcmp(ruid_name, o.o_logname)) {
						o.o_flags |= O_DEL;
						printf((nl_msg(7, "request \"%s-%d\" cancelled\n")),
							p.p_rdest, p.p_seqno);
						mail(o.o_logname,o.o_host,p.p_rdest,p.p_seqno);
					    }
					    else {
						fprintf(stderr, nl_msg(12, "request \"%s-%d\" not cancelled: not owner\n"), p.p_rdest, p.p_seqno);
						o.o_flags &= ~O_PRINT;
						strcpy(o.o_dev, "-");
						cancel = 0;
					   }
					}
					else {
						o.o_flags |= O_DEL;
						printf((nl_msg(7, "request \"%s-%d\" cancelled\n")),
							p.p_rdest, p.p_seqno);
						mail(o.o_logname,o.o_host,p.p_rdest,p.p_seqno);
					}
#else not REMOTE
					o.o_flags |= O_DEL;
					printf((nl_msg(7, "request \"%s-%d\" cancelled\n")),
						p.p_rdest, p.p_seqno);
					mail(o.o_logname, p.p_rdest, p.p_seqno);
#endif
				}
				else {
					o.o_flags &= ~O_PRINT;
					strcpy(o.o_dev, "-");
				}
				putoent(&o);
			}
		}
		time(&p.p_date);
		p.p_flags &= ~P_ENAB;
		if(reason != NULL) {
			strncpy(p.p_reason, reason, P_RSIZE);
			p.p_reason[P_RSIZE - 1] = '\0';
		}
		else
			strcpy(p.p_reason, (nl_msg(8, "reason unknown")));
		p.p_flags &= ~P_BUSY;
#ifdef REMOTE
		p.p_seqno = 0;
		p.p_pid = -1;
#else
		p.p_seqno = p.p_pid = 0;
#endif REMOTE
		sprintf(p.p_rdest, "-");
		putpent(&p);

		/* notify scheduler of new printer status */

		if(cancel && strcmp(o.o_dest,""))  /* do not cancel if there are no jobs queued */
		{
			enqueue(F_ZAP, pr);
#ifdef REMOTE
			rmreq(o.o_dest, o.o_seqno, o.o_host, (o.o_rflags & O_OB3));
#else
			rmreq(o.o_dest, o.o_seqno);
#endif REMOTE
		}
		else
			enqueue(F_DISABLE, pr);
		printf((nl_msg(9, "printer \"%s\" now disabled\n")), pr);
		reset();
#ifdef B1
if (ISB1)
      audit_subsystem("Attempts to disable printer(s)","Successful", ET_SUBSYSTEM);
#endif
	}

	endpent();
	endoent();
}

#ifdef REMOTE
mail(logname, host, dest, seqno)
#else
mail(logname, dest, seqno)
#endif REMOTE
char *logname;
#ifdef REMOTE
char	*host;
#endif REMOTE
char *dest;
int seqno;
{
	char *name;

#ifdef SecureWare
        if(((ISSECURE)&&(strcmp((name=(char *) lp_getlname()), logname) != 0))||
	  ((!ISSECURE) && (strcmp((name=getname()), logname) != 0)))
#else	
	if(strcmp((name=getname()), logname) != 0) 
#endif
	{
		sprintmsg(errmsg, (nl_msg(10, "your printer request %1$s-%2$d was cancelled by %3$s.")), dest, seqno, name);
#ifdef REMOTE
		sendmail(logname, host, errmsg);
#else
		sendmail(logname, errmsg);
#endif REMOTE
	}
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
		fatal((nl_msg(11, "spool directory non-existent")), 1);
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

setsigs()
{
	int saveint();

	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, saveint);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, saveint);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, saveint);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, saveint);
}

reset()
{
	int catch();

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

saveint()
{
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	interrupt = TRUE;
}

cleanup()
{
	endpent();
	endoent();
}

#include <pwd.h>

/*
 * user_name() --
 *    return a string representation of the user's name.
 *    Uses getlogin(), but if that fails we use getpwuid(real_user_id).
 */
char *
user_name()
{
    extern char *getlogin();
    extern struct passwd *getpwuid();

    char *s;
    uid_t id;
    struct passwd *pw;
    static char buf[L_cuserid];

    if ((s = getlogin()) != NULL)
	return strcpy(buf, s);
    
    if ((pw = getpwuid(id = getuid())) != NULL)
	return strcpy(buf, pw->pw_name);

    buf[0] = '\0';
    return buf;
}
