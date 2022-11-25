static char *HPUX_ID = "@(#) $Revision: 72.6 $";
/*
 * login [ name ] [ environment args ]
 *
 *	Conditional assemblies:
 *
 *	PASSREQ     causes no password to be required
 *	NO_MAIL	    causes the MAIL environment variable not to be set
 *	NOSHELL     causes the SHELL environment variable not to be set
 *	CONSOLE     allows root logins only on the device specified by
 *                  CONSOLE.  CONSOLE MUST NOT be defined as
 *		    either "/dev/syscon" or "/dev/systty"!!
 *      QUOTA	    runs quota command after validation
 *      RFLAG       if defined, the "-r" flag, formerly used by rlogind,
 *                  causes login to read remote and local username, term
 *                  type and baud rate from the rlogin client, and to do
 *                  hosts.equiv(4) authentication.  Incompatible with 
 *                  #ifdef SecureWare
 *
 *                  All code within #ifdef RFLAG is obsolete after release 8.0
 *                  and should be removed then!
 *
 *                  8.0, and should be removed.
 *	TMAC	    causes an SNMP trap message (datagram) to be sent to 
 *		    all network nodes listed in /etc/snmptrap_dest
 *		    if the number of login attempts exceeds MAXATTEMPTS
 *	AUDIT       HP C2 security enhancements; checks for existence of
 *                  SECUREPASSWD file and authenticates user against
 *                  password contained in that file. Also performs
 *                  self-auditing of login actions.  Incompatible with 
 *                  #ifdef SecureWare
 *      SecureWare  SecureWare portable C2 and B1 security enhancements
 *	B1	    SecureWare B1-specific changes
 *	TRUX	    additional security changes related to SecureWare changes; 
 *		    TRUX is automatically defined if SecureWare is
 *
 * HP-UX changes include:
 *	Bad login attempts logged to /usr/adm/btmp;
 *	Issue hangup after three bad login attempts;
 *	Implement Berkeley /etc/securetty feature;
 *	Let superuser log in to "/" if home directory corrupt;
 *	Enforce single-user system.
 *	Host accounting for BSD rlogind, telnetd;
 *	Permit previous login authentication with "login -f" for BSD rlogind;
 *	Preserve the TERM environment variable with "login -p" for
 *	  BSD rlogind, telnetd;
 *	Enforce user limits.
 */

#include <sys/types.h>
#include <utmp.h>
#include <signal.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <termio.h>
#include <sys/ioctl.h>
#include <sys/ptyio.h>
#include <sys/sysmacros.h> /* for major/minor macro's for pty check */
#include <sys/param.h>     /* for MAXUID macro */

#ifdef SecureWare
#ifdef AUDIT
ERROR: "AUDIT and SecureWare #defines are incompatible";
#endif /* AUDIT */
#ifdef RFLAG
ERROR: "RFLAG and SecureWare #defines are incompatible";
#endif /* RFLAG */
#ifndef TRUX
#define TRUX
#endif 
#include <sys/errno.h>
#include <sys/security.h>
#include <sys/audit.h>
#include <prot.h>
#include <protcmd.h>

/* Used as an expedient way to catch attemtps to leave */

#define exit    login_die
#endif /* SecureWare */

#ifdef AUDIT
#include <sys/audit.h>
#include <sys/errno.h>
#endif /* AUDIT */

#ifdef QUOTA
#include <unistd.h>
#endif /* QUOTA */

/* macros for isapty() */
#define SPTYMAJOR 17            /* major number for slave  pty's */
#define PTYSC     0x00          /* select code for pty's         */
#define select_code(x) (((x) & 0xff0000) >> 16)

/* the following utmp field is used for user counting: */
#define how_to_count ut_exit.e_exit

#define	TRUE		1
#define	FALSE		0
#define	FAILURE		-1
#define SUCCESS		0

#define SCPYN(a, b)	strncpy((a), (b), sizeof(a))
#define DIAL_FILE	"/etc/dialups"
#define SECURETTY_FILE	"/etc/securetty"
#define DPASS_FILE	"/etc/d_passwd"

#ifdef AUDIT
#define SECUREPASS	"/.secure/etc/passwd"
#endif /* AUDIT */

#define MAX_SHELL_LEN	64
#define MAX_LOGNAME_LEN	30
#define MAX_HOME_LEN	64
#define MAX_TERM_LEN	64

#define SHELL		"/bin/sh"
#define	PATH		"PATH=:/bin:/usr/bin"
#define	ROOTPATH	"PATH=:/bin:/usr/bin:/etc"
#define SUBLOGIN	"<!sublogin>"

#define	ROOTUID		0

#define	MAXARGS		63
#define	MAXENV		1024
#define MAXLINE		256
#define TIMEOUTLEN	60	/* User has TIMEOUTLEN seconds to login */
#define MAXATTEMPTS	3
#define U_NAMELEN	64

struct	passwd nouser = {"", "nope",-1,};
#ifdef AUDIT
struct s_passwd nosuser = {"", "nope",};
#endif /* AUDIT */
struct	utmp utmp;
char	u_name[U_NAMELEN];
char	minusnam[16] = "-";
char	shell[MAX_SHELL_LEN] = { "SHELL=" };
char	home[MAX_HOME_LEN] = { "HOME=" };
char	log_name[MAX_LOGNAME_LEN] = { "LOGNAME=" };
char	term[MAX_TERM_LEN] = { "TERM=" };

#ifdef hp9000s300
    int num_users[] = { 2, 32767 };
#   define MIN_VERSION     'A'
#   define UNLIMITED       'B'
#else
    int num_users[] = { 2, 16, 32, 64 , 8 };
#   define MIN_VERSION 	'A'
#   define UNLIMITED	'U'
#endif

#define MAX_STRICT_USERS 8	/* Maximum number of users allowed with 
				   restricted license */

#define NUM_VERSIONS 	(sizeof(num_users)/sizeof(num_users[0])) - 1

#ifndef	NO_MAIL
char	mail[30] = { "MAIL=/usr/mail/" };
#endif

char	*envinit[6+MAXARGS] = {
		home,PATH,log_name,0,0,0
	};
int	basicenv = 3;  /** note 3 = # of pre-defined in envinit **/
char	envblk[MAXENV];
struct	passwd *pwd;
struct	utsname utsname;

#ifdef TRUX
    int remote_login=0;
#endif TRUX

#ifdef AUDIT
struct	s_passwd *s_pwd;
char	auditmsg[80];
struct stat s_pfile;
int secure;	/* flag to denote existance of secure passwd file */
#endif /* AUDIT */

extern char *ltoa();
char	*crypt(), *getpass();
unsigned alarm();
extern	char **environ;

char	rusername[U_NAMELEN], lusername[U_NAMELEN];

#define	WEEK	(24L * 7 * 60 * 60) /* 1 week in seconds */
time_t	when;
time_t	maxweeks;
time_t	minweeks;
time_t	now;
long 	a64l(), time();

#ifdef SecureWare
struct pr_passwd *prpwd;
struct pr_term *prtc;

void login_die();
struct pr_passwd * login_check_expired();
struct pr_passwd * login_bad_user();
struct pr_term * login_term_params();
struct pr_term * login_bad_tty();
struct pr_term * login_good_tty();
char * login_crypt();
#endif /* SecureWare */

extern int errno;

char *ttyntail;
int sublogin;

main(argc, argv ,renvp)
int argc;
char **argv,**renvp;
{
	register char *namep;
	int j,k,l_index,length;
	char *ttyn;
	int nopassword, fflag, pflag;
	int Tflag = FALSE;
#ifdef RFLAG
	int rflag;
#endif /* RFLAG */
#if defined(SecureWare) && defined(SEC_NET_TTY)
	int hflag;
#endif /* SecureWare && SEC_NET_TTY */
	register int i;
	register struct utmp *u = (struct utmp *) 0;
	char **envp,*ptr,*endptr;
	extern char **getargs();
	extern char *ttyname();
	int n, login_attempts=0, bailout();

#ifdef SecureWare
	/* Invoke before using any other routines from protection library */
	if (ISSECURE)
        	set_auth_parameters(argc, argv);
#endif /* SecureWare */

#ifdef AUDIT
	/* Do self auditing */
	if (audswitch(AUD_SUSPEND) == -1) {
		perror("audswitch");
		exit(1);
	}

	/* set the secure flag if SECUREPASS exists. If so, we */
	/* are using it for authentication instead of /etc/passwd */

	secure = (stat(SECUREPASS, &s_pfile) < 0) ? 0:1;

	/* Set the audit process flag */
	/* unconditionally on since we want to log all logins */
	/* regardless of whether the user's audit flag is set */

	if (secure)
		setaudproc(1);
#endif /* AUDIT */

        /*
	 * Set flag to disable the pid check if you find that you are
	 * a subsystem login.
	 */
	sublogin = FALSE;
	if (*renvp && strcmp(*renvp,SUBLOGIN) == 0)
		sublogin = TRUE;

	cleanenv(&renvp, "LANG", "LANGOPTS", "NLSPATH", "TERM", 0);

#ifdef SecureWare
	if (ISSECURE) {
        	login_set_sys();
	}
	else
#endif /* SecureWare */
	{
		(void) umask(0);
		(void) alarm((unsigned) TIMEOUTLEN);
		(void) signal(SIGALRM, bailout);
	}
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) nice(0);
	ttyn = ttyname(0);
	if (ttyn==0)
		ttyn = "/dev/tty??";
	ttyntail = ttyn + sizeof("/dev/") - 1;

	/*
  	 * -p is used by rlogind and telnetd to tell login not to
  	 *    destroy the environment (in particular, TERM)
	 * -f is used by rlogind to indicate that hosts.equiv(4)
	 *    authentication has already been done, or by a user
	 *    exec'ing login from the shell as him/herself to skip a
	 *    second login authentication
   	 * -h is used by rlogind, telnetd, etc. to pass the name of the
  	 *    remote host to login so that it may be placed in utmp and wtmp
#ifdef RFLAG
  	 * -r was formerly used by rlogind to cause login to read remote 
	 *    and local username, term type and baud rate from the
	 *    rlogin client, and to do hosts.equiv(4) authentication
#endif
#if defined(SecureWare) && defined(B1)
	 * -T is used to allow trusted network services to force login
	 *    to reprompt for the session sensitivity label even though
	 *    it is already set.  Thus, remote logins from X Terminals
	 *    appear to be directly connected.
#endif * SecureWare && B1 *
	 */

 	fflag = pflag = 0;
#ifdef RFLAG
 	rflag = 0;
#endif /* RFLAG */
#if defined(SecureWare) && defined(SEC_NET_TTY)
	hflag = 0;
#endif /* SecureWare && SEC_NET_TTY */

processargs:
	if (argc > 1) {
  		if (strcmp(argv[1], "-f") == 0) {
  			fflag = 1;
  			argc--; argv++;
  			goto processargs;
  		}
		else if (strcmp(argv[1], "-p") == 0) {
  			pflag = 1;
  			argc--; argv++;
  			goto processargs;
  		}
#ifdef RFLAG
		else if (strcmp(argv[1], "-r") == 0) {
  			fflag = 0;
			if ((rflag = doremotelogin(argv[2])) == -1) {
				/* ruserok() authentication failed */
				rflag = 0;
			}
			(void) SCPYN(utmp.ut_host, argv[2]);
			(void) doremoteterm(term);
			envinit[basicenv++] = term;
			(void) strncpy(u_name, lusername, U_NAMELEN);
			(void) SCPYN(utmp.ut_user, lusername);
			envp = &argv[3];
			argc = argc - 2;
  			goto first;
		}
#endif /* RFLAG */
#if defined(SecureWare) && defined(B1)
		else if (strcmp(argv[1], "-T") == 0) {
			Tflag = TRUE;
			argc--; argv++;
			goto processargs;
		}
#endif /* SecureWare && B1 */
		else if (strcmp(argv[1], "-h") == 0) {
#ifdef SecureWare
			if (ISSECURE) {
				/*
				 *  Allow h flag only if login is spawned from
				 *  a trusted service (rlogind or telnetd).
				 *  In this case, the user id will still be zero.
				 *  Note that login cannot be executed by a user.
				*/
				if (getuid() == 0) {
#ifdef TRUX
                    remote_login = 1;
#endif TRUX
#ifdef SEC_NET_TTY
					hflag = 1;
					prtc = login_net_term_params(ttyn, argv[2]);
#endif /* SEC_NET_TTY */
					SCPYN(utmp.ut_host, argv[2]);
				}
			} else
#endif /* SecureWare */
				{
				if (getuid() == 0) {
					SCPYN(utmp.ut_host, argv[2]);
				}
			}
  			argc -= 2; argv += 2;
  			goto processargs;
		}
		else {
#ifdef SecureWare
			if (ISSECURE)
#ifdef SEC_NET_TTY
			    if (!hflag)
#endif /* SEC_NET_TTY */
				prtc = login_term_params(ttyn, ttyntail);
#endif /* SecureWare */
			(void) SCPYN(utmp.ut_user, argv[1]);
			(void) strncpy(u_name, argv[1], U_NAMELEN);
			envp = &argv[2];
			goto first;
		}
	}

#ifdef SecureWare
	/*
	**  the following is necessary in case login is executed
	**  with no username argument (in particular by telnetd)
	*/
	if (ISSECURE)
#ifdef SEC_NET_TTY
		if (!hflag)
#endif /* SEC_NET_TTY */
			prtc = login_term_params(ttyn, ttyntail);
#endif /* SecureWare */

loop:
	u_name[0] = utmp.ut_user[0] = '\0';	/* Erase user name */
#ifdef SecureWare
	if (ISSECURE)
        	prpwd = (struct pr_passwd *) 0;
#endif /* SecureWare */
#ifdef RFLAG
	rflag = 0;
#endif /* RFLAG */
	fflag = 0;

first:
   	/*
 	 * Preserve any significant environment variables.
  	 * All we really care about with the -p option is
  	 * the terminal type.
  	 */
  	if (pflag) {
  		char *t = (char *)getenv("TERM");
  		if (t != NULL) {
  			strncat(term, t, (MAX_TERM_LEN - strlen(term) - 1));
	  		envinit[basicenv++] = term;
		}
		pflag = 0;
  	}
#ifdef TMAC
	/* don't let number of login attempts exceed MAXATTEMPTS */
	/* without attempting to send an SNMP trap */
	if (++login_attempts > MAXATTEMPTS) {
		sendsnmptrap();
		login_attempts = 0;
	}
#endif /* TMAC */
	while (utmp.ut_user[0] == '\0') {
		fputs("login: ", stdout);
		if ((envp = getargs()) != (char**)NULL) {
			(void) SCPYN(utmp.ut_user,*envp);
			(void) strncpy(u_name, *envp++, U_NAMELEN);
		}
	}

#ifdef AUDIT
	if (secure) {
		setspwent();
		s_pwd = getspwnam(u_name);
		endspwent();
	}
#endif /* AUDIT */
	setpwent();
	pwd = getpwnam(u_name);
	endpwent();

	if (pwd == NULL
#ifdef AUDIT
			|| secure && s_pwd == NULL
#endif /* AUDIT */
						  ) {
		pwd = &nouser;	
#ifdef AUDIT
		s_pwd = &nosuser;
#endif /* AUDIT */
	}

	if (fflag) {
#ifdef TRUX
		if (ISSECURE) {
			/*
			 *  Allow f flag only if login is spawned from
			 *  a trusted service (rlogind or telnetd).
			 *  In this case, the user id will still be zero.
			 *  Note that login cannot be executed by a user.
			*/
			uid_t uid = getuid();
			fflag = (uid == 0);
		} else
#endif /* TRUX */
		{
			/*
			**  allow f flag only if user running login is
			**  same as target user or superuser
			*/
			uid_t uid = getuid();
			fflag = (uid == 0) || (uid == pwd->pw_uid);
		}
	}
		
	nopassword = TRUE;

#ifdef SecureWare
	if (ISSECURE) {
        	login_fillin_user(u_name, &prpwd, &pwd);
		/*
		**  fflag parameter to login_validate() indicates whether
		**  pre-authentication has been done.
		*/
        	if(!login_validate(&prpwd, &prtc, &nopassword, fflag))
                	goto loop;
	}
	else
#endif /* SecureWare */
	{
	    /*
	    **  don't prompt for password if already authenticated
	    **  or if the user has no password
	    */
	    if ((!fflag)
#ifdef RFLAG
			 && (!rflag)) { 
#endif /* RFLAG */
pass:
	      	if (*pwd->pw_passwd != '\0') 
			nopassword = FALSE;
		if (validate_pass("Password:", ttyn) == FAILURE) {
		    if (++login_attempts >= MAXATTEMPTS)
			bailout();
		    else {
			goto loop;
		    }
		}
	    }
	}

	if (pwd == &nouser)
	    goto pass;

	if(chdir(pwd->pw_dir) < 0) {
	    if ((pwd->pw_uid == 0) && (chdir("/") == 0))
		fputs("Warning:  home directory is \"/\".\n", stderr);
	    else {
		fputs("Unable to change directory to \"", stdout);
		fputs(pwd->pw_dir, stdout);
		fputs("\"\n", stdout);
		goto loop;
	    }
	}

	/* Get the version info via uname.  If it doesn't look right,
	 * assume the smallest user configuration
	 */
	if (uname(&utsname) < 0)
		utsname.version[0] = MIN_VERSION;

	/*
	 * Mappings:
	 *    834 -> 834
	 *    844 -> 844
	 *    836 -> 635
	 *    846 -> 645
	 *    843 -> 642
	 *    853 -> 652
	 */
	if ((!strncmp(utsname.machine, "9000/834", UTSLEN)) ||
	       (!strncmp(utsname.machine, "9000/844", UTSLEN)) ||
	       (!strncmp(utsname.machine, "9000/836", UTSLEN)) ||
	       (!strncmp(utsname.machine, "9000/846", UTSLEN)) ||
	       (!strncmp(utsname.machine, "9000/843", UTSLEN)) ||
	       (!strncmp(utsname.machine, "9000/853", UTSLEN))) {
	    if (count_users_strict(utmp.ut_user) > MAX_STRICT_USERS) {
		fputs("Sorry.  Maximum number of users already logged in.\n",
			stdout);
#ifdef AUDIT
		/* Construct a failed audit record */
		strcpy (auditmsg,
		    " attempted to login - too many users on the system");
		audit(auditmsg, 20);
#endif /* AUDIT */
		exit(20);
	    }
	}
	else if (utsname.version[0] != UNLIMITED) {
	    if ((utsname.version[0]-'A' < 0) ||
	     	(utsname.version[0]-'A' > NUM_VERSIONS))
		utsname.version[0] = MIN_VERSION;

	    n = (int) utsname.version[0] - 'A';
	    if (count_users(add_count()) > num_users[n]) {
		fputs("Sorry.  Maximum number of users already logged in.\n",
			stdout);
#ifdef AUDIT
		/* Construct a failed audit record */
		strcpy (auditmsg,
		    " attempted to login - too many users on the system");
		audit(auditmsg, 20);
#endif /* AUDIT */
		exit(20);
	    }
	}


#ifdef CONSOLE
	if(pwd->pw_uid == 0) {
		if(strcmp(ttyn, CONSOLE)) {
			fputs("Not on system console\n", stdout);
#ifdef AUDIT
			/* Construct a failed audit record */
			strcpy (auditmsg,
			    " attempted to login - not on system console");
			audit(auditmsg, 10);
#endif
			exit(10);
		}
	}
#endif

	(void) time(&utmp.ut_time);
	utmp.ut_pid = getpid();

	/*
	 * Find the entry for this pid (or line if we are a sublogin)
	 * in the utmp file.
	 */
	while ((u = getutent()) != NULL) {
		if ( (u->ut_type == INIT_PROCESS ||
		      u->ut_type == LOGIN_PROCESS ||
		      u->ut_type == USER_PROCESS)
		  && ( (sublogin &&
		        strncmp(u->ut_line,ttyntail,sizeof(u->ut_line))==0) ||
			u->ut_pid == utmp.ut_pid) ) {

	    		/*
			 * Copy in the name of the tty minus the
			 * "/dev/", the id, and set the type of entry
			 * to USER_PROCESS.
			 */
			(void) SCPYN(utmp.ut_line, ttyntail);
			utmp.ut_id[0] = u->ut_id[0];
			utmp.ut_id[1] = u->ut_id[1];
			utmp.ut_id[2] = u->ut_id[2];
			utmp.ut_id[3] = u->ut_id[3];
			utmp.ut_type = USER_PROCESS;
			utmp.how_to_count = add_count();

			/*
			 * Copy remote host information
			 */
			SCPYN(utmp.ut_host, u->ut_host);
			utmp.ut_addr = u->ut_addr;

			/* Return the new updated utmp file entry. */
			pututline(&utmp);
			break;
		}
	}
	endutent();		/* Close utmp file */

	if (u == (struct utmp *)NULL) {
		fputs("No utmp entry.", stdout);
		fputs("You must exec \"login\" from the lowest level \"sh\".\n",
			stdout);
#ifdef AUDIT
		/* Construct a failed audit record */
		strcpy (auditmsg,
		" attempted to login - must exec login from the lowest level");
		audit(auditmsg, 1);
#endif /* AUDIT */
		exit(1);
	}

	/*
	 * Now attempt to write out this entry to the wtmp file if we
	 * were successful in getting it from the utmp file and the
	 * wtmp file exists.
	 */
	if (u != NULL) {
		int fd = open(WTMP_FILE, O_WRONLY|O_APPEND);

		if (fd != -1) {
			write(fd, &utmp, sizeof (struct utmp));
			close(fd);
		}
	}
#ifdef SecureWare
	if (ISSECURE)
        	login_chown_tty(ttyn, pwd->pw_uid, pwd->pw_gid);
	else
#endif /* SecureWare */
	{
		/*
	 	* Fix permissions on tty
	 	*/
		chown(ttyn, pwd->pw_uid, pwd->pw_gid);
	}

	/*
	 * If the shell field starts with a '*', do a chroot to the
	 * home directory and perform a new login.
	 */
	if(*pwd->pw_shell == '*') {
		if(chroot(pwd->pw_dir) < 0) {
			fputs("No Root Directory\n", stdout);
#ifdef SecureWare
		    	if (ISSECURE) {
			    prpwd = login_bad_user(prpwd);
			    prtc = login_bad_tty(prtc, prpwd);
			    login_delay("retry");
		    	}
#endif /* SecureWare */
			goto loop;
		}

		/*
		 * Set the environment flag <!sublogin> so that the next
		 * login knows that it is a sublogin.
		 */
		envinit[0] = SUBLOGIN;
		envinit[1] = (char*)NULL;
		fputs("Subsystem root: ", stdout);
		fputs(pwd->pw_dir, stdout);
		fputc('\n', stdout);
#ifdef SecureWare
		if (ISSECURE) {
                	login_do_sublogin(envinit);
                	prpwd = login_bad_user(prpwd);
                	prtc = login_bad_tty(prtc, prpwd);
                	login_delay("retry");
		}
		else
#endif /* SecureWare */
		{
			execle("/bin/login", "login", 0, &envinit[0]);
			execle("/etc/login", "login", 0, &envinit[0]);
			fputs("No /bin/login or /etc/login on root\n", stdout);
		}
		goto loop;
	}

#ifdef SecureWare
        /* We cannot set the group until finished with auth database */

	if (ISSECURE) {
       	    login_set_user(prpwd, prtc, pwd, Tflag);
            login_setgid(pwd->pw_gid);
	    login_setuid(pwd->pw_uid);
	}
	else
#endif /* Secureware */
	{
	    initgroups(pwd->pw_name,-1);

	    if ((pwd->pw_gid < 0) || (pwd->pw_gid > MAXUID) ||
		 setgid(pwd->pw_gid) == FAILURE) {
		fputs("Bad group id.\n", stdout);
#ifdef AUDIT
		/* Construct a failed audit record */
		strcpy (auditmsg, " attempted to login - bad group id");
		audit(auditmsg, 1);
#endif /* AUDIT */
		exit(1);
	    }
	    if ((pwd->pw_uid < 0) || (pwd->pw_uid > MAXUID) || 
		 setresuid(pwd->pw_uid,pwd->pw_uid,0) == FAILURE) {
		fputs("Bad user id.\n", stdout);
#ifdef AUDIT
		/* Construct a failed audit record */
		strcpy (auditmsg, " attempted to login - bad user id");
		audit(auditmsg, 1);
#endif /* AUDIT */
		exit(1);
	    }

	}
	/* give user time to come up with new password if needed */
	alarm(0);

#ifdef SecureWare
	if (ISSECURE) {
            if (!login_need_passwd(prpwd, prtc, &nopassword))
                login_die(1);
	}
	else
#endif /* Secureware */
	{
#ifdef PASSREQ
	    if (nopassword) {
		fputs("You don't have a password.  ", stdout);
		fputs("Choose one.\npasswd ", stdout);
		fputs(u_name, stdout);
		fputc('\n', stdout);
		n = system("/bin/passwd");
		if (n < 0) {
		    fputs("Cannot execute /bin/passwd\n", stdout);
#ifdef AUDIT
			/* Construct a failed audit record */
			strcpy (auditmsg,
			    " attempted to login - cannot execute /bin/passwd");
			setresuid(0,0,0);
			audit(auditmsg, 9);
#endif /* AUDIT */
		    exit(9);
		} else {
		    execl("/bin/login", "login", 0);
		    execl("/etc/login", "login", 0);
		    fputs("/bin/login and /etc/login not found\n", stdout);
		    goto loop;
		}
	    }
#endif /* PASSREQ */
	}
#ifdef SecureWare
	if (ISSECURE)
        	prpwd = login_check_expired(prpwd, prtc);
	else
#endif /* SecureWare */
	{
	    /*
	     * is the age of the password to be checked?
	     */
#ifdef AUDIT
	    if ((secure && (*s_pwd->pw_age != NULL)) ||
			    (!secure && (*pwd->pw_age != NULL))) {
#else
	    if (*pwd->pw_age != NULL) {
#endif
		/*
		 * retrieve (a) week of previous change;
		 *	    (b) maximum number of valid weeks
		 */
#ifdef AUDIT
		when = (long) a64l (secure ? (s_pwd -> pw_age)
					   : (pwd   -> pw_age));
#else /* not AUDIT */
		when = (long) a64l (pwd->pw_age);
#endif /* AUDIT */
		/*
		 * max, min and weeks since last change are packed
		 * radix 64
		 */
		maxweeks = when & 077;
		minweeks = (when >> 6) & 077;
		when >>= 12;
		now  = time(0)/WEEK;
		if (when > now ||
			(now > when + maxweeks) && (maxweeks >= minweeks)) {
		    fputs("Your password has expired. Choose a new one\n",
			stdout);
		    n = system("/bin/passwd");
		    if (n < 0) {
			fputs("Cannot execute /bin/passwd\n", stdout);
#ifdef AUDIT
			/* Construct a failed audit record */
			strcpy (auditmsg,
			    " attempted to login - cannot execute /bin/passwd");
			setresuid(0,0,0);
			audit(auditmsg, 9);
#endif /* AUDIT */
			exit(9);
		    } else {
			execl("/bin/login", "login", 0);
			execl("/etc/login", "login", 0);
			fputs("/bin/login and /etc/login not found\n", stdout);
			goto loop;
		    }
		}
	    }
	}
#ifdef SecureWare
	if (ISSECURE) {
        	prtc = login_good_tty(prtc, prpwd);
        	login_good_user(&prpwd, &prtc, pwd);
	}
#endif /* SecureWare */

#ifdef QUOTA
	if ((access("/usr/bin/quota", X_OK)==0) ||
	    (access("/etc/edquota", X_OK)==0)) {
		int retval;
		fputs("Please wait...checking for disk quotas\n",stdout);
		retval = system("quota");
		if (retval < 0) {
			fputs("could not execute quota command\n", stdout);
		}
	}
#endif /* QUOTA */

	/*
	 * Set up the basic environment for the exec.  This includes
	 * HOME, PATH, LOGNAME, SHELL, and MAIL.
	 */
	(void) strncat(home, pwd->pw_dir, (MAX_HOME_LEN - strlen(home) -1));
	(void) strncat(log_name, pwd->pw_name, 
		       (MAX_LOGNAME_LEN - strlen(log_name) -1));
	if(pwd->pw_uid == ROOTUID) {
#ifdef CONSOLE
		if(strcmp(ttyn, CONSOLE)) {
			fputs("Not on system console\n", stdout);
#ifdef AUDIT
			/* Construct a failed audit record */
			strcpy (auditmsg,
			    " attempted to login - not on system console");
                        setresuid(0,0,0);
			setuid(0);
			audit(auditmsg, 10);
#endif /* AUDIT */
			exit(10);
		}
#endif /* CONSOLE */
		envinit[1] = ROOTPATH;
	}
	if (*pwd->pw_shell == '\0') {
		pwd->pw_shell = SHELL;
		(void) strncat(shell, pwd->pw_shell, 
			       (MAX_SHELL_LEN - strlen(shell) - 1));
	}
#ifndef NOSHELL
	else {
		envinit[basicenv++] = shell;
		(void) strncat(shell, pwd->pw_shell, 
			       (MAX_SHELL_LEN - strlen(shell) - 1));
	}
#endif

	/*
	 * Find the name of the shell.
	 */
	if ((namep = strrchr(pwd->pw_shell, '/')) == NULL)
		namep = pwd->pw_shell;
	else
		namep++;

	/*
	 * Generate the name of the shell with a '-' sign in front of
	 * it.
	 * This causes .profile processing when a shell is exec'ed.
	 */
	(void) strcat(minusnam, namep);

#ifndef	NO_MAIL
	envinit[basicenv++] = mail;
	(void) strcat(mail,pwd->pw_name);
#endif

	/*
	 * Add in all the environment variables picked up from the
	 * argument list to "login" or from the user response to the
	 * "login" request.
	 */
	for (j=0,k=0,l_index=0,ptr= &envblk[0]; *envp && j < MAXARGS-1
		;j++,envp++) {

		/*
		 * Scan each string provided.  If it doesn't have the
		 * format xxx=yyy, then add the string "Ln=" to the
		 * beginning.
		 */
		if ((endptr = strchr(*envp,'=')) == (char*)NULL) {
			envinit[basicenv+k] = ptr;
			*ptr = 'L';
			strcpy(ptr+1, ltoa(l_index));
			strcat(ptr, "=");
			strcat(ptr, *envp);

			/*
			 * Advance "ptr" to the beginning of the next
			 * argument.
			 */
			while (*ptr++)
			    continue;
			k++;
			l_index++;
		} else {
			/*
			 * Check to see whether this string replaces
			 * any previously defined string.
			 */
			for (i=0,length= endptr+1-*envp; i < basicenv+k;i++) {
				if (strncmp(*envp,envinit[i],length) == 0) {

					/*
					 * If the variable to be changed
					 * is "SHELL=" or "PATH=", don't
					 * allow the substitution.
					 */
					if (strncmp(*envp,"SHELL=",6) != 0
					    && strncmp(*envp,"PATH=",5) != 0)
						envinit[i] = *envp;
					break;
				}
			}

			/*
			 * If it doesn't, place it at the end of
			 * environment array.
			 */
			if (i == basicenv+k) {
				envinit[basicenv+k] = *envp;
				k++;
			}
		}
	}

	if (strncmp("pri=", pwd->pw_gecos, 4) == 0) {
		int	mflg, pri;

		pri = 0;
		mflg = 0;
		i = 4;
		if (pwd->pw_gecos[i] == '-') {
			mflg++;
			i++;
		}
		while(pwd->pw_gecos[i] >= '0' && pwd->pw_gecos[i] <= '9')
			pri = (pri * 10) + pwd->pw_gecos[i++] - '0';
		if (mflg)
			pri = -pri;
		(void) nice(pri);
	}
#ifdef AUDIT
	/* Write self audit record and Resume auditing */
        setresuid(0,0,0);

	/* Check validity of the audit process flag */
	if (secure && (s_pwd->pw_audflg > 1 || s_pwd->pw_audflg < 0)) {
		fputs("Bad audit flag\n", stdout);
		/* Construct a failed audit record */
		strcpy (auditmsg, " attempted to login - bad audit flag");
		audit(auditmsg, 1);
		exit(1);
	}
	/* Set the audit id */
	if (secure && (setaudid(s_pwd->pw_audid) == FAILURE)) {
		fputs("Bad audit id\n", stdout);
		/* Construct a failed audit record */
		strcpy (auditmsg, " attempted to login - bad audit id");
		audit(auditmsg, 1);
		exit(1);
	}

	audit(" Successful login", 0);
	setaudproc(s_pwd->pw_audflg);
	setuid(pwd->pw_uid);
#endif /* AUDIT */

	/*
	 * Switch to the new environment.
	 */
	environ = envinit;
	(void) alarm((unsigned) 0);

	signal(SIGQUIT, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGTSTP, SIG_IGN);
	execlp(pwd->pw_shell, minusnam, 0);
	fputs("No shell\n", stdout);
#ifdef AUDIT
	/* Construct a failed audit record */
	strcpy (auditmsg, " attempted to login - no shell");
        setresuid(0,0,0);
	setaudproc(1);
	audit(auditmsg, 1);
#endif /* AUDIT */
	exit(1);
}

int
validate_pass(prompt, tty)
char *prompt;
char *tty;
{
	int tty_valid;
	int check_pass_ret_val = SUCCESS;
	int dialpass_ret_val = SUCCESS;

	/*
	 * If root is loggin in, see if root logins on this terminal
	 * are permitted.
	 * This is the /etc/securetty feature.  We want to prompt for a
	 * password even if we know that this is not a securetty because
	 * we don't want to give any clue if the potential intruder guesses
	 * the correct password.
	 */
	tty_valid = 1; /* assume login is allowed on this tty */
	if ((pwd->pw_uid == 0) && !rootterm(tty+sizeof("/dev/")-1))
		tty_valid = 0;

	/*
	 * Get the password and remember whether it was valid.
	 */
	if (*pwd->pw_passwd != '\0')
	   check_pass_ret_val = check_pass(prompt, pwd->pw_passwd, tty);

	/*
	 * Get the dialup password (if required) and remember whether it
	 * was valid.
	 */
	dialpass_ret_val = dialpass(tty);

	/*
	 * Print error message if any of the above failed.
	 */
	if ((check_pass_ret_val == FAILURE) || (dialpass_ret_val == FAILURE)
	    || !tty_valid) {
		fputs("Login incorrect\n", stdout);
		return(FAILURE);
	}
	return(SUCCESS);
}

int
dialpass(ttyn)
char *ttyn;
{
	register FILE *fp;
	char defpass[30];
	char line[80];
	register char *p1, *p2;

	if((fp=fopen(DIAL_FILE, "r")) == NULL)
	{
		/*
		 * dialup password not needed
		 */
		return(SUCCESS);
	}
	while((p1 = fgets(line, sizeof(line), fp)) != NULL) {
		while(*p1 != '\n' && *p1 != ' ' && *p1 != '\t')
			p1++;
		*p1 = '\0';
		if(strcmp(line, ttyn) == 0)
			break;
	}
	fclose(fp);
	if(p1 == NULL || (fp = fopen(DPASS_FILE, "r")) == NULL)
	{
		/*
		 * dialup password not needed
		 */
		return(SUCCESS);
	}
	while((p1 = fgets(line, sizeof(line)-1, fp)) != NULL) {
		while(*p1 && *p1 != ':')
			p1++;
		*p1++ = '\0';
		p2 = p1;
		while(*p1 && *p1 != ':')
			p1++;
		*p1 = '\0';
		if(strcmp(pwd->pw_shell, line) == 0) {
			fclose(fp);
			if (*p2 == '\0')
				return(SUCCESS);
			return(check_pass("Dialup Password:", p2, ttyn));
		}
		if(strcmp(SHELL, line) == 0)
			SCPYN(defpass, p2);
	}
	fclose(fp);
	return(check_pass("Dialup Password:", defpass, ttyn));
}

check_pass(prmt, pswd, ttyn)
char *prmt, *pswd, *ttyn;
{
	register char *p1;

	p1 = crypt(getpass(prmt), pswd);
	if(strcmp(p1, pswd)) {
		write_btmp(ttyn);
		return(FAILURE);
	}
	return(SUCCESS);
}

#define	WHITESPACE	0
#define	ARGUMENT	1

char **getargs()
{
	static char envbuf[MAXLINE];
	static char *args[MAXARGS];
	register char *ptr,**answer;
	register int c;
	int state;
	extern int quotec();

	for (ptr= &envbuf[0]; ptr < &envbuf[sizeof(envbuf)];)
		*ptr++ = '\0';
	for (answer= &args[0]; answer < &args[MAXARGS];)
		*answer++ = (char *)NULL;
	for (ptr= &envbuf[0],answer= &args[0],state = WHITESPACE;
	     (c = getc(stdin)) != EOF;) {
		switch (c) {
		case '\n' :
			if (ptr == &envbuf[0])
			    return (char **)NULL;
			return &args[0];
		case ' ' :
		case '\t' :
			if (state == ARGUMENT) {
				*ptr++ = '\0';
				state = WHITESPACE;
			}
			break;
		case '\\' :
			c = quotec();
			/* FALLS THROUGH */
		default :
			if (state == WHITESPACE) {
				*answer++ = ptr;
				state = ARGUMENT;
			}
			*ptr++ = c;
		}

		/*
		 * If the buffer is full, force the next character to be
		 * read to be a <newline>.
		 */
		if (ptr == &envbuf[sizeof(envbuf)-1]) {
			ungetc('\n',stdin);
			putc('\n',stdout);
		}
	}

	/*
	 * If we left loop because an EOF was received, exit
	 * immediately.
	 */
	exit(0);
}

int quotec()
{
	register int c,i,num;

	switch(c = getc(stdin)) {
	case 'n' :
		c = '\n';
		break;
	case 'r' :
		c = '\r';
		break;
	case 'v' :
		c = 013;
		break;
	case 'b' :
		c = '\b';
		break;
	case 't' :
		c = '\t';
		break;
	case 'f' :
		c = '\f';
		break;
	case '0' :
	case '1' :
	case '2' :
	case '3' :
	case '4' :
	case '5' :
	case '6' :
	case '7' :
		for (num=0,i=0; i < 3;i++) {
			num = num * 8 + (c - '0');
			if ((c = getc(stdin)) < '0' || c > '7')
				break;
		}
		ungetc(c,stdin);
		c = num & 0377;
		break;
	default :
		break;
	}
	return (c);
}

write_btmp(ttyn)
char *ttyn;
{
	int fd;
	pid_t pid;
	struct utmp *u;

	utmp.ut_pid = getpid();
	while ((u = getutent()) != NULL) {
		if ( (u->ut_type == INIT_PROCESS ||
		      u->ut_type == LOGIN_PROCESS ||
		      u->ut_type == USER_PROCESS)
		  && ( (sublogin &&
		        strncmp(u->ut_line,ttyntail,sizeof(u->ut_line))==0) ||
			u->ut_pid == utmp.ut_pid) ) {

			/*
			 * Copy remote host information
			 */
			SCPYN(utmp.ut_host, u->ut_host);
			utmp.ut_addr = u->ut_addr;

			break;
		}
	}
	endutent();		/* Close utmp file */

	/*
	 * If btmp exists, then record the bad attempt
	 */
	if ((fd = open(BTMP_FILE,O_WRONLY|O_APPEND)) >= 0) {
	    (void) SCPYN(utmp.ut_line, ttyn + sizeof("/dev/") - 1);
	    (void) time(&utmp.ut_time);
	    write(fd,(char *)&utmp,sizeof(utmp));
	    (void) close(fd);
	}
}

bailout()
{
	/* invoked after SIGALRM */
	struct termio s;
	ioctl(0, TCGETA, &s);
	s.c_cflag = s.c_cflag|HUPCL;
	ioctl(0, TCSETA, &s);
#ifdef AUDIT
	/* Construct a failed audit record */
        setresuid(0,0,0);
	audit(" Failed login (bailout)",1);
#endif /* AUDIT */
	exit(1);
}

rootterm(tty)
char *tty;
{
	register FILE *fd;
	char buf[100];

	/*
	 * check to see if /etc/securetty exists, if it does then scan
	 * file to see if passwd tty is in the file.  Return 1 if it is,
	 * 0 if not.
	 */
	if ((fd = fopen(SECURETTY_FILE, "r")) == NULL)
		return(1);
	while (fgets(buf, sizeof buf, fd) != NULL) {
		buf[strlen(buf)-1] = '\0';
		if (strcmp(tty, buf) == 0) {
			fclose(fd);
			return(1);
		}
	}
	fclose(fd);
	return(0);
}

#ifdef RFLAG
doremotelogin(host)
	char *host;
{
	FILE *hostf;
	int first = 1;
	char *cp;

	if (getuid()) {
		pwd = &nouser;
#ifdef AUDIT
		if (secure)
			s_pwd = &nosuser;
#endif /* AUDIT */
		goto bad;
	}

	getstr(rusername, sizeof (rusername), "remuser");
	getstr(lusername, sizeof (lusername), "locuser");
	getstr(term+5, sizeof(term)-5, "Terminal type");

#ifdef AUDIT
	if (secure) {
		setspwent();
		s_pwd = getspwnam(lusername);
		endspwent();
	}
#endif /* AUDIT */
	setpwent();
	pwd = getpwnam(lusername);
	endpwent();

	if (pwd == NULL
#ifdef AUDIT
			|| secure && s_pwd == NULL
#endif /* AUDIT */
						  ) {
		pwd = &nouser;	
#ifdef AUDIT
		s_pwd = &nosuser;
#endif /* AUDIT */
		goto bad;
	}

	if (ruserok(host, pwd->pw_uid == 0, rusername, lusername) == 0)
		return(1);	/* ruserok() returns 0 for success */
bad:
	/*
	 * if we cannot validate the user, return an error!
	 */
	return (-1);
}

getstr(buf, cnt, err)
	char *buf;
	int cnt;
	char *err;
{
	char c;

	do {
		if (read(0, &c, 1) != 1) {
			exit(1);
		}
		if (--cnt < 0) {
			fputs(err, stdout);
			fputs(" too long\r\n", stdout);
#ifdef AUDIT
			/* Construct a failed audit record */
                        setresuid(0,0,0);
			audit (" Failed login (getstr)", 1);
#endif /* AUDIT */
			exit(1);
		}
		*buf++ = c;
	} while (c != 0);
}

char *speeds[] = {
    "0", "50", "75", "110", "134", "150", "200", "300",
    "600", "900", "1200", "1800", "2400", "3600", "4800", "7200",
    "9600", "19200", "38400", "EXTA","EXTB"
};
#define	NSPEEDS	(sizeof (speeds) / sizeof (speeds[0]))

doremoteterm(term)
    char *term;
{
    struct termio tp;
    char *strcat();
	char *cp = strchr(term, '/');
	register int i;

	ioctl(0, TCGETA, &tp);
	if (cp) {
		*cp++ = 0;
		for (i = 0; i < NSPEEDS; i++)
			if (!strcmp(speeds[i], cp)) {
				tp.c_cflag &= ~CBAUD;
				tp.c_cflag |= i;
				break;
			}
	}
	tp.c_iflag &= ~INPCK;
	tp.c_iflag |= ICRNL;
	tp.c_oflag |= OPOST|ONLCR|TAB3;
    	tp.c_oflag &= ~ONLRET;
	tp.c_lflag |= (ECHO|ECHOE|ECHOK|ISIG|ICANON);
	tp.c_cflag &= ~PARENB;
	tp.c_cc[VEOF] = CEOF;
	ioctl(0, TCSETAF, &tp);
}
#endif /* RFLAG */

#define NCOUNT 16

count_users(added_users)
int added_users;
{
    int count[NCOUNT], nusers, i;
    struct utmp *entry;

    for (i=0; i<NCOUNT; i++)
	count[i] = 0;

    count[added_users]++;

    while (entry = getutent()) {
	if (entry->ut_type == USER_PROCESS) {
	    i = entry->how_to_count;
	    if (i < 0 || i >= NCOUNT)
		i = 1;          /* if out of range, then count */
				/* as ordinary user */
	    count[i]++;
	}
    }
    endutent();

    /*
     * KEY:
     *  [0]     does not count at all
     *  [1]     counts as real user
     *  [2]     logins via a pty which have not gone through login.  (ex.
     *		hpterm).  These logins are no longer counted.
     *  [3]     logins via a pty which have been logged through login (i.e.
     *		rlogin and telnet).  These collectively count as 1.
     *  [4-15]  may be used for groups of users which collectively
     *          count as 1
     */
    nusers = count[1];
    /*
     * Ignore count[2] since these logins are not counted (exit status of
     * 2).
     */
    for (i=3; i<NCOUNT; i++)
	if (count[i] > 0)
	    nusers++;

    return(nusers);
}

int
count_users_strict(new_user)
char *new_user;
{
    char pty_users[MAX_STRICT_USERS][8];
    int count[NCOUNT];
    int nusers;
    int i;
    int cnt;
    int pty_off = -1;
    int uname_off;
    struct utmp *entry;

    /* Initialize count array */
    for (i = 0; i < NCOUNT; i++)
	count[i] = 0;

    /* Add in the new user */
    if (add_count() == 1) { /* user is logged in via tty */
	count[1]++;
    }
    else {
	strncpy(pty_users[++pty_off], new_user, 8);
	count[3]++;
    }

    while (entry = getutent()) {
	if (entry->ut_type == USER_PROCESS) {
	    i = entry->how_to_count;

	    /* if out of range, then count as ordinary user logged in 
	       via a tty */
	    if (i == 1 || (i < 0 || i >= NCOUNT))
		count[1]++;
	    /* See if it is a pty login granted by login program */
	    else if (i == 3) {
	        count[3]++;
	        /* See if user is already logged in via login pty */
		uname_off = -1;
		for (cnt = 0; cnt <= pty_off; cnt++)
			if (strncmp(pty_users[cnt], entry->ut_user, 8) == 0)
				uname_off = cnt;

		if (uname_off == -1) { /* user is not logged in via pty yet */
			
		    if (pty_off >= MAX_STRICT_USERS)  /* cannot add any
		    					 more users */
		    	return(MAX_STRICT_USERS + 1);
		    /* add the user name to the array of pty users */
		    else
			strncpy(pty_users[++pty_off], entry->ut_user, 8);
		}
	    } /* end if (i == 3) */
	    else
		count[i]++;
	} /* end if entry->ut_type == USER_PROCESS */
    } /* end while (entry = getutent()) */

    endutent();
    /*
     * KEY:
     *  [0]	does not count at all
     *  [1]	counts as "real" user
     *  [2]     logins via a pty which have not gone trough login (i.e.
     *		hpterm).  these logins are no longer counted.
     *  [3]     logins via a pty which have been logged through login (i.e.
     *		rlogin and telnet).  these count as 1 "real" user per
     *		unique user name.
     *  [4-15]  may be used for groups of users which collectively count 
     *          as 1
     */

     nusers = pty_off + 1 + count[1];  /* Current number of users is sum of
					  users logged in via tty + the
					  number of unique users logged in 
					  via pty which have gone through
					  login */

    /*
     * Don't count any hpterm logins (exit status of 2).  We already
     * counted all pty logins granted by the login program.
     */
    for (i = 4; i < NCOUNT; i++)
        if (count[i] > 0)
	    nusers++;
    return(nusers);
}

/*
 * returns 1 if user should be counted as 1 user;
 * returns 3 if user should be counted as a pty
 * may return other stuff in the future
 */
int
add_count()
{
    if (isapty()) {
	if (is_nlio())
	    return(1);
	else
	    return(3);
    } else
	return(1);
}

/*
 * returns 1 if user is a pty, 0 otherwise
 */
isapty()
{
    struct stat sbuf;
    static int firsttime = 1;
    static int retval;

    if (firsttime) {
	firsttime = 0;
	if (fstat(0, &sbuf) != 0 ||                /* can't stat */
	    (sbuf.st_mode & S_IFMT) != S_IFCHR ||  /* not char special */
	    major(sbuf.st_rdev) != SPTYMAJOR ||    /* not pty major num */
	    select_code(sbuf.st_rdev) != PTYSC) {  /* not pty minor num */
	    retval = 0;
	} else {
	    retval = 1;
#ifdef TRUX
            if (ISB1 && !remote_login)
                retval=0;               /* local pty for td tty simulation */
#endif TRUX
	}
    }
    return(retval);
}

#define NLIOUCOUNT	_IO('K',28)	/* _IO(K, 28) */
#define NLIOSERV	0x4b33		/* (('K' << 8) + 51 */

/*
 * returns 1 if user is an NL server, else returns 0
 */
is_nlio()
{
    if (ioctl(0, NLIOUCOUNT) == NLIOSERV)
	return(1);
    else
	return(0);
}

#ifdef AUDIT
/*
 * Construct self audit record for event and write to audit trail.
 * This routine assumes that effective uid is currently 0.
 */
audit(msg, errnum)
char * msg;
int errnum;
{
	char *txtptr;
	struct self_audit_rec audrec;

	txtptr = (char *)audrec.aud_body.text;
	strcpy (txtptr, "User= ");
	strcat (txtptr, pwd->pw_name);
	strcat (txtptr, " uid=");
	strcat (txtptr, ltoa(pwd->pw_uid));
	strcat (txtptr, " audid=");
	strcat (txtptr, ltoa(s_pwd->pw_audid));
	strcat (txtptr, msg);
	audrec.aud_head.ah_pid = getpid();
	audrec.aud_head.ah_error = errnum;
	audrec.aud_head.ah_event = EN_LOGINS;
	audrec.aud_head.ah_len = strlen (txtptr);
	audwrite(&audrec);

	/* Resume auditing */
	audswitch(AUD_RESUME);
}
#endif /* AUDIT */

#ifdef TMAC

/* sendsnmptrap requires the real exit() routine */
#undef exit

sendsnmptrap()
{
	if (fork() == 0) {
		register int i=3;
		setsid();
		while (i<10)
			close(i++);
		execl("/tcb/bin/snmptrap", "/tcb/bin/snmptrap", "6", "1", 0);
		exit(1);
	}
}
#endif /* TMAC */
