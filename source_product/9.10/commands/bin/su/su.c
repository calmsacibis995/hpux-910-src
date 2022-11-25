static char *HPUX_ID = "@(#) $Revision: 70.1 $";
/*
 *	su [-] [name [arg ...]] change userid, `-' changes environment.
 *	If SULOG is defined, all attempts to su to another user are
 *	logged there.
 *	If CONSOLE is defined, all successful attempts to su to uid 0
 *	are also logged there.
 *
 *	If su cannot create, open, or write entries into SULOG,
 *	(or on the CONSOLE, if defined), the entry will not
 *	be logged -- thus losing a record of the su's attempted
 *	during this period.
 */
#include <sys/types.h>
#include <stdio.h>
#include <pwd.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#ifdef SecureWare
#include <sys/types.h>
#include <sys/security.h>
#define logname	Logname
#include <sys/audit.h>
#include <prot.h>
#include <protcmd.h>
#endif

#ifndef NLS
#   define catgets(i, sn,mn,s) (s)
#else /* NLS */
#   define NL_SETN 1	/* set number */
#   include <nl_types.h>
    nl_catd nlmsg_fd;
#endif /* NLS */

#define SULOG	"/usr/adm/sulog"	/*log file*/
#define PATH	"PATH=:/bin:/usr/bin"	/*path for users other than root*/
#define SUPATH	"PATH=/bin:/etc:/usr/bin"	/*path for root*/
#define SUPRMT	"PS1=# "		/*primary prompt for root*/
#define ELIM 128
#define ROOT 0
#define SYS 3

long time();
struct	passwd *pwd, *getpwnam();
struct	tm *localtime();
char	*malloc(), *strcpy();
char	*getpass(), *ttyname(), *strrchr(), *getenv();
uid_t	getuid();

void	remove_from_env();

#ifdef SecureWare
char    auditbuf[80];
#endif

char	*shell = "/bin/sh";	/*default shell*/
char	su[16] = "su";		/*arg0 for exec of shprog*/
char	envfile_s[] = "ENV=";
char	*homedir;
char	homedir_s[] = "HOME=";
char	*logname;
char	logname_s[] = "LOGNAME=";
char	*shellenv;
char	shellenv_s[] = "SHELL=";
char	*initstate = 0;
char	initstate_s[] = "INIT_STATE=";
char	*path = PATH;
char	*supath = SUPATH;
char	*suprmt = SUPRMT;
char	*envinit[ELIM];
extern	char **environ;
char *ttyn;

main(argc, argv)
int	argc;
char	**argv;
{
	char *nptr, *password;
	char	*pshell = shell;
	int badsw = 0;
	int eflag = 0;
	int uid, gid;
	char *dir, *shprog, *name;
	char *state;

#ifdef SecureWare
	if(ISSECURE)
		set_auth_parameters(argc, argv);
#ifdef B1
	if(ISB1){
		initprivs();
		(void) forcepriv(SEC_ALLOWMACACCESS);
	}
#endif
#endif
#ifdef NLS
	nlmsg_fd = catopen("su");
#endif

	if (argc > 1 && *argv[1] == '-') {
		eflag++;	/*set eflag if `-' is specified*/
		argv++;
		argc--;
	}

	/*determine specified userid, get their password file entry,
	  and set variables to values in password file entry fields
	*/
	nptr = (argc > 1)? argv[1]: "root";
	if((pwd = getpwnam(nptr)) == NULL) {
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,1, "su: Unknown id: %s\n")),nptr);
		exit(1);
	}
#ifdef SecureWare
	if(ISSECURE)
		su_ensure_secure(pwd);
#endif
	uid = pwd->pw_uid;
	gid = pwd->pw_gid;
	dir = strcpy(malloc(strlen(pwd->pw_dir)+1),pwd->pw_dir);
	shprog = strcpy(malloc(strlen(pwd->pw_shell)+1),pwd->pw_shell);
	name = strcpy(malloc(strlen(pwd->pw_name)+1),pwd->pw_name);
	if((ttyn=ttyname(0))==NULL)
		if((ttyn=ttyname(1))==NULL)
			if((ttyn=ttyname(2))==NULL)
				ttyn="/dev/tty??";

#ifdef SULOG
	/*if SULOG defined, create SULOG, if it does not exist, with
	  mode 600. Change owner and group to root
	*/
#ifdef SecureWare
	if(ISSECURE)
		su_check_sulog(SULOG, ROOT);
	else{
		close( open(SULOG, O_WRONLY | O_APPEND | O_CREAT, 0600) );
		chown(SULOG, ROOT, ROOT);
	}
#else
	close( open(SULOG, O_WRONLY | O_APPEND | O_CREAT, 0600) );
	chown(SULOG, ROOT, ROOT);
#endif
#endif

	/*Prompt for password if invoking user is not root or
	  if specified(new) user requires a password
	*/
#ifdef SecureWare
	if((pwd->pw_passwd[0] == '\0') ||
	   ((!ISSECURE) && (getuid() == 0)))
#else
	if(pwd->pw_passwd[0] == '\0' || getuid() == 0 )
#endif
		goto ok;
#ifdef SecureWare
	if(!ISSECURE)
	    password = getpass((catgets(nlmsg_fd,NL_SETN,2, "Password:")));
        if (((ISSECURE) && (!su_proper_password(pwd))) ||
	    ((!ISSECURE) && 
	      (badsw || (strcmp(pwd->pw_passwd, crypt(password, pwd->pw_passwd)) != 0))))
#else
	password = getpass((catgets(nlmsg_fd,NL_SETN,2, "Password:")));
	if(badsw || (strcmp(pwd->pw_passwd, crypt(password, pwd->pw_passwd)) != 0))
#endif
	{
#ifdef SULOG
		log(SULOG, nptr, 0);	/*log entry*/
#endif
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,3, "su: Sorry\n")));
#ifdef TRUX
		if (ISB1) {
			sprintf(auditbuf, "su to uid %d gid %d",uid,gid);
			audit_subsystem( auditbuf,
			   "password incorrect, abort su", ET_ACCESS_DENIAL);
		}
#endif TRUX
		exit(2);
	}
ok:
	endpwent();	/*close password file*/
#ifdef SecureWare
	if(ISSECURE)
#ifdef SULOG
	    su_ensure_admission(eflag, SULOG);
#else
	    su_ensure_admission(eflag, (char *) 0);
#endif
#endif
#ifdef SULOG
	log(SULOG, nptr, 1);	/*log entry*/
#endif

#ifdef SecureWare

	if(ISSECURE)
	    su_set_login(uid);

#ifdef B1
	/* require SEC_IDENTITY privilege to modify groups, gid or uid */
	if(ISB1)
	    (void) forcepriv(SEC_IDENTITY);

#endif /* B1 */
#endif /* SecureWare */

#if defined(__hp9000s300) || defined(__hp9000s800)
	initgroups(nptr,-1);
#endif
	/*set user and group ids to specified user*/
	if((setgid(gid) != 0) || (setuid(uid) != 0)) {
#ifdef SecureWare
                audit_subsystem("set user and group ids to specified user",
                        "invalid id; unable to set, abort su", ET_SUBSYSTEM);
#endif
		printf((catgets(nlmsg_fd,NL_SETN,4, "su: Invalid ID\n")));
		exit(2);
	}
#if defined(SecureWare) && defined(B1)
	if(ISB1)
	    (void) disablepriv(SEC_IDENTITY);
#endif

	/*set environment variables for new user;
	  arg0 for exec of shprog must now contain `-'
	  so that environment of new user is given
	*/
	if (eflag) {
		homedir=(char *)malloc(strlen(dir)+strlen(homedir_s)+1);
		strcpy(homedir, homedir_s);
		strcat(homedir, dir);
		logname=(char *)malloc(strlen(name)+strlen(logname_s)+1);
		strcpy(logname, logname_s);
		strcat(logname, name);
		envinit[2] = logname;
		chdir(dir);
		envinit[0] = homedir;
		if (uid == 0)
			envinit[1] = supath;
		else
			envinit[1] = path;
		if (*pwd->pw_shell == '\0') {	/*if no shell specifed*/
			shellenv=(char *)malloc(strlen(shell)+strlen(shellenv_s)+1);
			strcpy(shellenv, shellenv_s);
			strcat(shellenv, shell); /* give him default shell */
		} else {
			shellenv=(char *)malloc(strlen(pwd->pw_shell)+strlen(shellenv_s)+1);
			strcpy(shellenv, shellenv_s);
			strcat(shellenv, pwd->pw_shell);
		}
		envinit[3] = shellenv;
		/* If environment variable INIT_STATE is defined,
		 * pass it along.
		 */
		 if ( (state=getenv("INIT_STATE")) != NULL ) {
			initstate=(char *)malloc(strlen(state)+strlen(initstate_s)+1);
			strcpy(initstate, initstate_s);
			strcat(initstate, state);
			envinit[4] = initstate;
			envinit[5] = NULL;
		} else
			envinit[4] = NULL;
		environ = envinit;
		strcpy(su, "-su");
	} else {
	   /*
	    * If the - was not given, we want to preserve as much of the
	    * original user's environment as possible.  Unfortunately,
	    * we must clobber HOME and ENV for security reasons.
	    *
	    * Removing HOME prevents the csh from executing the old
	    * user's $HOME/.cshrc with the privileges of the new user.
	    * (Note that doing a putenv("HOME=") does not do it.)
	    *
	    * Removing ENV prevents the ksh from executing the old
	    * user's $ENV file with the privileges of the new user.
	    * (Doing a putenv("ENV=") would do it too.)
	    */

	    remove_from_env(homedir_s);
	    remove_from_env(envfile_s);
	}

	/*if new user is root:
		if CONSOLE defined, log entry there;
		if eflag not set, change environment to that of root.
	*/
	if (uid == 0)
	{
#ifdef CONSOLE
		if(strcmp(ttyn, CONSOLE) != 0) {
			signal(SIGALRM, to);
			alarm(30);
			log(CONSOLE, nptr, 1);
			alarm(0);
		}
#endif
		if (!eflag) envalt();
	}

	/*if new user's shell field is not NULL or equal to /bin/sh,
	  set:
		pshell = their shell
		su = [-]last component of shell's pathname
	*/
	if (shprog[0] != '\0' && (strcmp(shell,shprog) != 0) ) {
		pshell = shprog;
		strcpy(su, eflag ? "-" : "" );
		strcat(su, strrchr(pshell,'/') + 1);
	}

	/*if additional arguments, exec shell program with array
	    of pointers to arguments:
		-> if shell = /bin/sh, then su = [-]su
		-> if shell != /bin/sh, then su = [-]last component of
						     shell's pathname

	  if no additional arguments, exec shell with arg0 of su
	    where:
		-> if shell = /bin/sh, then su = [-]su
		-> if shell != /bin/sh, then su = [-]last component of
						     shell's pathname
	*/
#ifdef NLS
	catclose(nlmsg_fd);
#endif
#if defined(SecureWare) && defined(B1)
	if(ISB1)
	    (void) enablepriv(SEC_SUID);
#endif
	if (argc > 2) {
		argv[1] = su;
		execv(pshell, &argv[1]);
	} else {
		execl(pshell, su, 0);
	}

	fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,5, "su: No shell\n")));
	exit(3);
}

/*Environment altering routine -
	This routine is called when a user is su'ing to root
	without specifying the - flag.
	The user's PATH and PS1 variables are reset
	to the correct value for root.
	All of the user's other environment variables retain
	their current values after the su (if they are
	exported).
*/
envalt()
{
	int putenv();

	/*If user has PATH variable in their environment, change its value
			to /bin:/etc:/usr/bin ;
	  if user does not have PATH variable, add it to the user's
			environment;
	  if either of the above fail, an error message is printed.
	*/
	if ( ( putenv(supath) ) != 0 ) {
		printf((catgets(nlmsg_fd,NL_SETN,6, "su: unable to obtain memory to expand environment")));
		exit(4);
	}

	/*If user has PROMPT variable in their environment, change its value
			to # ;
	  if user does not have PROMPT variable, add it to the user's
			environment;
	  if either of the above fail, an error message is printed.
	*/
	if ( ( putenv(suprmt) ) != 0 ) {
		printf((catgets(nlmsg_fd,NL_SETN,7, "su: unable to obtain memory to expand environment")));
		exit(4);
	}

	return;

}

void
remove_from_env(s)
char *s;
{
   int i, len;

   len = strlen(s);
   if (len <= 0) return;

   for (i = 0; environ[i] != NULL; i++)
      if (strncmp(s, environ[i], len) == 0) break;
       
   while (environ[i] != NULL) {
      environ[i] = environ[i+1];
      i++;
   }
}

#if defined(SULOG) || defined(CONSOLE)
/*
 * user_name() --
 *    return a string representation of the user's name.
 *    Uses getlogin(), but if that fails we use getpwuid(real_user_id).
 *    If that also fails, we just return the ascii represtation of the
 *    real uid.
 */
char *
user_name()
{
    extern char *getlogin();
    extern struct passwd *getpwuid();
    extern char *ultoa();

    char *s;
    uid_t id;
    struct passwd *pw;
    static char buf[L_cuserid];

    if ((s = getlogin()) != NULL)
	return strcpy(buf, s);
    
    if ((pw = getpwuid(id = getuid())) != NULL)
	return strcpy(buf, pw->pw_name);
    
    return strcpy(buf, ultoa(id));

}

/*Logging routine -
	where = SULOG or CONSOLE
	towho = specified user ( user being su'ed to )
	how = 0 if su attempt failed; 1 if su attempt succeeded
*/
log(where, towho, how)
char *where, *towho;
int how;
{
	FILE *logf;
	long now;
	struct tm *tmp;

	now = time(0);
	tmp = localtime(&now);

	/*open SULOG or CONSOLE -
		if open fails, return
	*/
#if defined(SecureWare) && defined(B1)
	if(ISB1){
	    forcepriv(SEC_ALLOWDACACCESS);
#if SEC_ILB
	    forcepriv(SEC_ILNOFLOAT);
#endif
	    if ((logf=fopen(where,"a")) == NULL) {
		disablepriv(SEC_ALLOWDACACCESS);
		return;
	    }
	}
	else{
	    if ((logf=fopen(where,"a")) == NULL) return;
	}
#else
	if ((logf=fopen(where,"a")) == NULL) return;
#endif

	/*write entry into SULOG or onto CONSOLE -
		 if write fails, return
	*/
	if ((fprintf(logf,"SU %.2d/%.2d %.2d:%.2d %c %s %s-%s\n",
		tmp->tm_mon+1,tmp->tm_mday,tmp->tm_hour,tmp->tm_min,
		how?'+':'-',(strrchr(ttyn,'/')+1),
		user_name(),towho)) < 0)
	{
		fclose(logf);
#if defined(SecureWare) && defined(B1)
		if(ISB1){
			disablepriv(SEC_ALLOWDACACCESS);
#if SEC_ILB
	    		disablepriv(SEC_ILNOFLOAT);
#endif
		}
#endif
		return;
	}

	fclose(logf);	/*close SULOG or CONSOLE*/
#if defined(SecureWare) && defined(B1)
	if(ISB1){
		disablepriv(SEC_ALLOWDACACCESS);
#if SEC_ILB
    		disablepriv(SEC_ILNOFLOAT);
#endif
	}
#endif

	return;
}
#endif /* SULOG || CONSOLE */
