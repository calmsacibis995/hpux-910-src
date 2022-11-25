static char *HPUX_ID = "@(#) $Revision: 70.2 $";
/*
 * chsh name  [shell]
 * shell ::= { set of path names returned by getusershell }
 * default value of shell is /bin/sh
 */
#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef AUDIT
#include <sys/audit.h>
#include <sys/errno.h>
#include <sys/stat.h>
#endif
#ifdef TRUX
#include <sys/stat.h>
#include <sys/security.h>
#endif


#ifdef HP_NFS
#   include <sys/param.h>
char	*passwdfile = "/etc/passwd";
#else
char	passwd[] = "/etc/passwd";
#endif
char	temp[]	 = "/etc/ptmp";
char	opasswd[] = "/etc/opasswd";
struct	passwd *pwd;
struct	passwd *getpwnam();
char	*crypt();
char	*getpass();
char	*getusershell();

#if defined (AUDIT) || defined (TRUX)
struct stat sbuf;
#endif

#ifdef HP_NFS
/*
 *  These are local copies of the endpwent() and getpwent() routines.
 */
static void Endpwent();
static struct passwd *Getpwent();
#else
/* from <pwd.h> */
void endpwent();
struct passwd *getpwent();
#endif

main(argc, argv, environ)
int argc;
char *argv[];
char **environ;
{
	int u;
	FILE *tf;
	char *p;

#ifdef TRUX
	set_auth_parameters();
	initprivs();
	forcepriv(SEC_OWNER);
#endif
	cleanenv(&environ, "LANG", "LANGOPTS", "NLSPATH", 0 );

#ifdef AUDIT
	/* Do self-auditing */
	if (audswitch(AUD_SUSPEND) == -1) {
		perror("audswitch");
		exit(1);
	}
#endif
	if (argc < 2 || argc > 3)
		usage();


	/* default shell is /bin/sh                       */
	if (argc == 2)
		argv[2] = "/bin/sh";
	else
	{
		char *shfn;
		for (shfn = getusershell(); shfn; shfn = getusershell()) {
			if (!strcmp(argv[2], shfn)) {
				break;
			}
		}
		if (shfn == NULL) {
			fprintf(stderr,"chsh: invalid shell %s\n",argv[2]);
			usage();
		}
	}

	if (( pwd = getpwnam(argv[1])) != NULL) {
		u = getuid();
		if (u!=0 && u != pwd->pw_uid){
			printf("Permission denied.\n");
#ifdef AUDIT
			/* Construct failed audit record */
			audit(argv[1], argv[2]," Permission denied",1);
#endif
			exit(1);
		}
	}

	/* check existency of user's given loginname */
	if (pwd == NULL) {
		fprintf(stderr,"chsh: invalid name %s\n",argv[1]);
		usage();
	}

	/* check for existency of chosen shell */
	if ((access (argv[2], F_OK)) < 0) {
		fprintf (stderr, "chsh: can't find ");
		perror(argv[2]);
		exit (1);
	}

#if defined (AUDIT) || defined (TRUX)
#ifdef HP_NFS
	/* Stat password file to save owner/group/mode */
	if (stat(passwdfile, &sbuf) != 0) {
		fprintf (stderr, "chsh: cannot stat %s\n", passwdfile);
		exit (1);
	}
#endif
#endif

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
#ifdef	SIGTSTP
	signal(SIGTSTP, SIG_IGN);
#endif	SIGTSTP

	/***********
	* Set a reasonably large ulimit to ensure that /etc/passwd
	* can be written.  20,000 * 512 = 10 meg limit
	***********/
	ulimit (2,20000);    
	umask (0333);
	if (access(temp, 0) >= 0) {
		printf("Temporary file busy -- try again\n");
#ifdef AUDIT
		/* Construct failed audit record */
		audit(argv[1], argv[2]," Temporary file busy",1);
#endif
		exit(1);
	}
	if ((tf = fopen(temp,"w")) == NULL) {
		printf("Cannot create temporary file\n");
#ifdef AUDIT
		/* Construct failed audit record */
		audit(argv[1], argv[2]," Can't create temporary file",1);
#endif
		exit(1);
	}

/*
 *	copy passwd to temp, replacing matching lines
 *	with new shell.
 */

	setpwent();
#ifdef HP_NFS
	while ((pwd = Getpwent()) != NULL) {
#else
	while ((pwd = getpwent()) != NULL) {
#endif
		p = pwd->pw_name;
		if (*p == '+') p++;
		if (strcmp (p, argv[1]) == 0) {
			u = getuid();
			if (u != 0 && u != pwd->pw_uid) {
				printf("Permission denied.\n");
				goto out;
			}
			pwd->pw_shell = argv[2];
#ifdef HP_NFS
			if (pw_toolong(pwd))
			{
			    fprintf (stderr, "Password entry is too long.\n");
			    goto out;
			}
#endif /* HP_NFS */
		}
		putpwent(pwd,tf);
	}
#ifdef HP_NFS
	Endpwent();
#else
	endpwent();
#endif

	fsync(fileno(tf));
	fclose(tf);

/*
 *	rename temp file back to passwd file
 */

	if (unlink(opasswd) && access(opasswd, 0) == 0)
	{
		fprintf(stderr, "cannot unlink %s\n", opasswd);
		goto out;
	}

#ifdef HP_NFS
	if (link(passwdfile, opasswd))
	{
		fprintf(stderr, "cannot link %s to %s\n", passwdfile, opasswd);
		goto out;
	}

	if (unlink(passwdfile))
	{
		fprintf(stderr, "cannot unlink %s\n", passwdfile);
		goto out;
	}

	if (link(temp, passwdfile))
	{
		fprintf(stderr, "cannot link %s to %s\n", temp, passwdfile);
		if (link(opasswd, passwdfile))
		{
			fprintf(stderr, "cannot recover %s\n", passwdfile);
#ifdef AUDIT
			/* Construct failed audit record */
			audit(argv[1], argv[2]," Can't recover",1);
#endif
			exit(1);
		}
		goto out;
	}
#else not NFS
	if (link(passwd, opasswd))
	{
		fprintf(stderr, "cannot link %s to %s\n", passwd, opasswd);
		goto out;
	}

	if (unlink(passwd))
	{
		fprintf(stderr, "cannot unlink %s\n", passwd);
		goto out;
	}

	if (link(temp, passwd))
	{
		fprintf(stderr, "cannot link %s to %s\n", temp, passwd);
		if (link(opasswd, passwd))
		{
			fprintf(stderr, "cannot recover %s\n", passwd);
#ifdef AUDIT
			/* Construct failed audit record */
			audit(argv[1], argv[2]," Can't recover",1);
#endif
			exit(1);
		}
		goto out;
	}
#endif not NFS

#if defined (AUDIT) || defined (TRUX)
#ifdef HP_NFS
	/* Restore owner/group/mode of password file */
	if (chown(passwdfile, sbuf.st_uid, sbuf.st_gid) != 0) {
		fprintf(stderr, "chsh: cannot chown %s\n", passwdfile);
		goto out;
	}

	if (chmod(passwdfile, sbuf.st_mode) != 0) {
		fprintf(stderr, "chsh: cannot chmod %s\n", passwdfile);
		goto out;
	}
#endif
#endif

	if (unlink(temp))
	{
		fprintf(stderr,"cannot unlink %s\n", temp);
		goto out;
	}

#ifdef AUDIT
	/* Construct successful audit record */
	audit(argv[1], argv[2]," Successfully changed",0);
#endif
	exit(0);


out:
	unlink(temp);
#ifdef AUDIT
	/* Construct failed audit record */
	audit(argv[1], argv[2]," Chsh failed",1);
#endif
	exit(1);
}

usage()
{
	fprintf(stderr,"Usage: chsh name [ shell ]\n");
#ifdef AUDIT
	/* Just resume auditing - no audit record - usage error */
	audswitch(AUD_RESUME);
#endif
	exit(1);
}


#ifdef HP_NFS
extern FILE *fopen();
extern int fclose();
extern char *fgets();

static char EMPTY[] = "";
static FILE *pwf = NULL;
static char line[BUFSIZ+1];
static struct passwd passwd;

static void
Endpwent()
{
	if (pwf != NULL) {
		(void) fclose(pwf);
		pwf = NULL;
	}
}

static char *
pwskip(p)
register char *p;
{
	while (*p && *p != ':' && *p != '\n')
		++p;
	if (*p == '\n')
		*p = '\0';
	else if (*p)
		*p++ = '\0';
	return(p);
}


static struct passwd *
Getpwent()
{
	register char *p;
	char *end;
	long	x, strtol();
	char *memchr();

	if (pwf == NULL) {
		if ((pwf = fopen(passwdfile, "r" )) == NULL )
			return(0);
	}

	p = fgets(line, BUFSIZ, pwf);
	if (p == NULL)
		return(NULL);
	passwd.pw_name = p;
	p = pwskip(p);
	passwd.pw_passwd = p;
	p = pwskip(p);
	x = strtol(p, &end, 10);
	p = pwskip(p);
	passwd.pw_uid = (x < -2 || x > MAXUID)? (MAXUID+1): x;
	x = strtol(p, &end, 10);
	p = pwskip(p);
	passwd.pw_gid = (x < -2 || x > MAXUID)? (MAXUID+1): x;
	passwd.pw_comment = EMPTY;
	passwd.pw_gecos = p;
	p = pwskip(p);
	passwd.pw_dir = p;
	p = pwskip(p);
	passwd.pw_shell = p;
	(void) pwskip(p);

	p = passwd.pw_passwd;
	while (*p && *p != ',')
		p++;
	if (*p)
		*p++ = '\0';
	passwd.pw_age = p;
	return(&passwd);
}
#endif HP_NFS

#ifdef AUDIT
/***************************************************************************
 *
 *	audit - Construct a self audit record for the event and write
 *		it the the audit log.  This routine assumes that the
 *		effective uid is currently 0.
 *
 ***************************************************************************/

audit(name, sh, msg, errnum)
char *name;
char *sh;
char *msg;
int errnum;
{
	char *txtptr;
	struct self_audit_rec audrec;

	txtptr = (char *)audrec.aud_body.text;
	strcpy (txtptr, "User= ");
	strcat (txtptr, name);
	strcat (txtptr, " shell= ");
	strcat (txtptr, sh);
	strcat (txtptr, msg);
	audrec.aud_head.ah_pid = getpid();
	audrec.aud_head.ah_error = errnum;
	audrec.aud_head.ah_event = EN_CHSH;
	audrec.aud_head.ah_len = strlen (txtptr);
	/* Write the audit record */
	audwrite(&audrec);

	/* Resume auditing */
	audswitch(AUD_RESUME);
}
#endif

#ifdef HP_NFS
pw_toolong(pwd)
  struct passwd *pwd;
{
    int len;
    char t[BUFSIZ];

    len  = strlen(pwd->pw_name);
    len += strlen(pwd->pw_passwd);
    len += strlen(pwd->pw_age) <= 0 ? 0 : strlen(pwd->pw_age) + 1;
    len += strlen(pwd->pw_gecos);
    len += strlen(pwd->pw_dir);
    len += strlen(pwd->pw_shell);
    sprintf(t,"%d%d",pwd->pw_uid, pwd->pw_gid);
    len += strlen(t);
    len += 6;	/* separators ':' */
    
    return (len >= BUFSIZ);
}
#endif /* HP_NFS */
