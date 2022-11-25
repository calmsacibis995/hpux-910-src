static char *HPUX_ID = "@(#) $Revision: 66.5 $";
/*
 * newgrp [group]
 *
 * rules
 *	if no arg, group id in password file is used
 *	else if group id == id in password file
 *	else if login name is in member list
 *	else if password is present and user knows it
 *	else too bad
 */
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#ifdef SecureWare
#include <sys/types.h>
#include <sys/security.h>
#include <sys/audit.h>
#endif
#ifdef AUDIT
#include <sys/audit.h>
#include <sys/errno.h>
#endif

#define	SHELL	"/bin/sh"

#define PATH	"PATH=:/bin:/usr/bin"
#define SUPATH	"PATH=:/bin:/etc:/usr/bin"
#define ELIM	128

char	PW[] = "Password: ";
char	NG[] = "Sorry";
char	PD[] = "Permission denied";
char	UG[] = "Unknown group";
char	NS[] = "You have no shell";
#ifdef AUDIT
char 	ORIGGP[] = "Original group";
#endif

struct	group *getgrnam();
struct	passwd *getpwuid();
struct	passwd *getpwnam();
char	*getlogin();
char	*getpass();

char homedir[64]="HOME=";
char setSHELL[64]="SHELL=";
char alogname[20]="LOGNAME=";

char	*crypt();
char	*malloc();
char	*strcpy();
char	*strcat();
char	*strrchr();

char *envinit[ELIM];
extern char **environ;
char *path=PATH;
char *supath=SUPATH;

#ifdef AUDIT
char *grpname;
int wflag = 0;		/* flag to indicate a warning was issued */
#endif

#ifdef SecureWare
char auditbuf[80];
#endif

main(argc,argv,environ)
int argc;
char *argv[];
char **environ;
{
	char *s;
	int realuid;
	register struct passwd *p = (struct passwd *)0;
	char *rname();
	int eflag = 0;
	int uid;
	char *shell, *dir, *name;

#ifdef SecureWare
	if(ISSECURE)
        	set_auth_parameters(argc, argv);
#ifdef B1
	if(ISB1)
        	initprivs();
#endif /* B1 */
#endif /* SecureWare */
#ifdef	DEBUG
	chroot(".");
#endif

#ifdef AUDIT
	/* Do self-auditing */
	if (audswitch(AUD_SUSPEND) == -1) {
		fputs(PD, stderr);
		fputc('\n', stderr);
		exit(1);
	}
#endif /* AUDIT */
	realuid = getuid();

	/*
	 * First, we lookup the user's name based on the name they
	 * used to login.
	 */
	if ((s = getlogin()) != NULL)
		p = getpwnam(s);

	/*
	 * If we couldn't get them by name, lookup based on their
	 * real user id.
	 */
	if (p == (struct passwd *)0 &&
	    (p = getpwuid(realuid)) == (struct passwd *)0)
		error(NG);

	if (argc > 1 && *argv[1] == '-') {
		eflag++;
		argv++;
		--argc;
	}

#ifdef AUDIT
	if (argc > 1)
		grpname = argv[1];
	else
		grpname = ORIGGP;
#endif /* AUDIT */

	if (argc > 1)
		p->pw_gid = chkgrp(argv[1], p);

	uid = p->pw_uid;
	dir = strcpy(malloc(strlen(p->pw_dir)+1),p->pw_dir);
	name = strcpy(malloc(strlen(p->pw_name)+1),p->pw_name);
#if defined(SecureWare) && defined(B1)
	if(ISB1)
        	(void) forcepriv(SEC_IDENTITY);
#endif
	if (setgid(p->pw_gid) < 0 || setresuid(realuid, realuid, 0) < 0)
		error(NG);
#if defined(SecureWare) && defined(B1)
	if(ISB1)
        	(void) disablepriv(SEC_IDENTITY);
#endif
	if (!*p->pw_shell)
		p->pw_shell = SHELL;

	if (eflag) {
		char *simple;

		strcat(homedir, dir);
		strcat(alogname, name);
		strcat(setSHELL, p->pw_shell);
		envinit[2] = alogname;
		chdir(dir);
		envinit[0] = homedir;
		if (uid == 0)
			envinit[1] = supath;
		else
			envinit[1] = path;
		envinit[3] = setSHELL;
		envinit[4] = NULL;
		environ = envinit;
		shell = strcpy(malloc(sizeof(p->pw_shell + 2)), "-");
		shell = strcat(shell,p->pw_shell);
		simple = strrchr(shell,'/');
		if (simple) {
			*(shell+1) = '\0';
			shell = strcat(shell,++simple);
		}
	}
	else
		shell = p->pw_shell;
#if defined(SecureWare) && defined(B1)
	if(ISB1)
        	(void) enablepriv(SEC_EXECSUID);
#endif
#ifdef AUDIT
	/* construct passed audit record */
	setresuid(0,0,0);
	if (wflag)	/* a warning has been issued */
		audit(" Failed newgrp", 1);
	else
		audit(" Successful newgrp", 0);
	setuid(realuid);
#endif /* AUDIT */
	execl(p->pw_shell,shell, NULL);
	error(NS);
}

warn(s)
char *s;
{
#ifdef AUDIT
	wflag = 1;
#endif
	fputs(s, stderr);
	fputc('\n', stderr);
}

error(s)
char *s;
{
	warn(s);
#ifdef AUDIT
	/* failed self audit record */
	setresuid(0,0,0);
	audit(s, 1);
#endif
	exit(1);
}

chkgrp(gname, p)
char	*gname;
struct	passwd *p;
{
	register char **t;
	register struct group *g;

	g = getgrnam(gname);
	endgrent();
	if (g == NULL) {
		warn(UG);
		return getgid();
	}
#if defined(SecureWare) && defined(B1)
        if (p->pw_gid == g->gr_gid || ((ISB1) && hassysauth(SEC_IDENTITY)) ||
	    ((!ISB1) && (getuid() == 0)))
#else
	if (p->pw_gid == g->gr_gid || getuid() == 0)
#endif
		return g->gr_gid;
	for (t = g->gr_mem; *t; ++t)
		if (strcmp(p->pw_name, *t) == 0)
			return g->gr_gid;
	if (*g->gr_passwd) {
		if (!isatty(fileno(stdin)))
			error(PD);
#ifdef SecureWare
                if (((ISSECURE) && (newgrp_good_password(g, PW))) ||
		    ((!ISSECURE) &&
		    (strcmp(g->gr_passwd, crypt(getpass(PW), g->gr_passwd)) == 0)))
#else
		if (strcmp(g->gr_passwd, crypt(getpass(PW), g->gr_passwd)) == 0)
#endif
			return g->gr_gid;
	}
	warn(NG);
#ifdef SecureWare
	if (ISSECURE) {
               	sprintf(auditbuf, "change group to %s", gname);
               	audit_security_failure(OT_PROCESS, auditbuf,
                       	"not permitted to change to requested group",
                       	ET_ACCESS_DENIAL);
	}
#endif
	return getgid();
}
/*
 * return pointer to rightmost component of pathname
 */
char *rname(pn)
char *pn;
{
	register char *q;

	q = pn;
	while (*pn)
		if (*pn++ == '/')
			q = pn;
	return q;
}

#ifdef AUDIT
	/* Construct self audit record for event and write */
	/* to audit trail.                                 */
	/* This routine assumes that effective uid is      */
	/* currently 0                                     */

audit(msg, errnum)
char *msg;
int errnum;
{
	char *txtptr;
	struct self_audit_rec audrec;

	txtptr = (char *)audrec.aud_body.text;
	strcpy (txtptr, "newgrp=");
	strcat (txtptr, grpname);
	strcat (txtptr, msg);
	audrec.aud_head.ah_pid = getpid();
	audrec.aud_head.ah_error = errnum;
	audrec.aud_head.ah_event = EN_NEWGRP;
	audrec.aud_head.ah_len = strlen (txtptr);
	/* Write the audit record */
	audwrite(&audrec);

	/* Resume auditing */
        audswitch(AUD_RESUME);

}
#endif /* AUDIT */
