static char *HPUX_ID = "@(#) $Revision: 70.3 $";

/*
 * Enter a password in the password file.
 * This program should be suid with the owner
 * having write permission on /etc
 * When AUDIT is defined and the secure passwd file (SECUREPASS) exists, 
 * passwd() will change passwd field in the secure passwd file.
 */

#ifdef HP_NFS
#   include <sys/param.h>
#endif /* HP_NFS */
#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <sys/types.h>
#include <ctype.h>         /* isalpha(c), isdigit(c), islower(c), toupper(c) */
#include <sys/wait.h>
#ifdef SecureWare
#include <sys/security.h>
#include <sys/audit.h>
#include <prot.h>
#include <protcmd.h>

static struct pr_passwd *pr = (struct pr_passwd *) 0;
struct passwd *get_pwent();
void end_pwent();
#define getpwent        get_pwent
#define endpwent        end_pwent
#endif
#include <string.h>
#include <fcntl.h>	/* O_CREAT, O_EXCL, O_WRONLY */

#if defined(HP_NFS) || defined(AUDIT)
#   include <sys/stat.h>
#endif

#ifdef NLS
#   define NL_SETN 1
#   include <msgbuf.h>
#   include <locale.h>
#else
#   define nl_msg(i, s) (s)
#endif

#include <limits.h>
#include <errno.h>

#ifdef AUDIT
#include <sys/audit.h>

#define SECUREPASS	"/.secure/etc/passwd"
#define OSECUREPASS	"/.secure/etc/opasswd"
#define SECURETEMP	"/.secure/etc/ptmp"

struct s_passwd *s_pwd;
struct stat s_pfile;
int secure;		/* flag to denote the existance of secure passwd file */
#endif /* AUDIT */

extern int errno;

char	*passwd = "/etc/passwd";
char	opasswd[256] = "/etc/opasswd";
char	temp[256]	 = "/etc/ptmp";

#ifdef HP_NFS
extern struct passwd * l_getpwent();
extern void l_endpwent();
struct stat sbuf1, sbuf2;
int	try_nis_map = 0;
int	f_flag = 0;
int	euid = -1;
int	ruid = -1;
char	hostname[256];
char	*make14charname();
#endif /* HP_NFS */

struct	passwd *pwd;
char	*crypt();
char	*getpass();
char	*pw;
#ifdef SecureWare
char    pwbuf[BUFSIZ];
char    opwbuf[BUFSIZ];
char    buf[BUFSIZ];
#else 
char	pwbuf[10];
char	opwbuf[10];
char	buf[10];
#endif

time_t	when;
time_t	now;
time_t	maxweeks;
time_t	minweeks;
long	a64l();
char	*l64a();
long	time();
int	count; /* count verifications */

#define WEEK (24L * 7 * 60 * 60) /* seconds per week */
#define MINLENGTH 6  /* for passwords */

main (argc, argv, environ)
	int argc;
	char *argv[];
	char **environ;
{
	char *p;
	int i;
	char saltc[2];
	long salt;
	int u;
	int insist;
	int ok, flags;
	int c;
	int j;                     /* for triviality checks */
	int k;                     /* for triviality checks */
	int flag;                  /* for triviality checks */
	int opwlen=0;              /* for triviality checks */
	int pwlen=0;
	int tfd = -1;		   /* for ptmp */
	FILE *tf;
	char *o;                   /* for triviality checks */
	char *uname, *getlogin();

#ifdef SecureWare
	if(ISSECURE)
        	set_auth_parameters(argc, argv);
#endif
#ifdef AUDIT
	char *txtptr;
	struct self_audit_rec audrec;
#endif /* AUDIT */

        cleanenv( &environ, "LANG", "LANGOPTS", "NLSPATH", "LC_COLLATE",
#ifdef SecureWare
		"TZ",
#endif /* SecureWare */
		 "LC_CTYPE", "LC_MONETARY", "LC_NUMERIC", "LC_TIME", 0 );
	euid = geteuid();
	ruid = getuid();

#if !defined(SecureWare) && !defined(B1)
	/*
	 * set limits high so processes with small limits
	 * don't truncate the password file -- hesh
	 */
	if (ulimit(2, LONG_MAX) == (long)-1) {
		perror("ulimit");
		exit(1);
	}
#else
	if ((!ISSECURE) && (ulimit(2, LONG_MAX) == (long)-1)) {
		perror("ulimit");
		exit(1);
	}
#endif

#if defined(NLS) || defined(NLS16)
	/*
	 * initialize to the current locale
	 */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("passwd"), stderr);
		putenv("LANG=");
	}
	nl_catopen("passwd");
#endif /* NLS || NLS16 */

	insist = 0;
	count = 0;

#ifdef HP_NFS
	uname = NULL;
	while (argc > 1)
	{
	    if (argv[1][0] == '-')
	    {
		if (argv[1][1] != 'f' || argc < 3)
		{
		    usage();
		    exit(1);
		}

		/*
		 * if f flag already given,
		 * don't let user try again
		 */
		if (f_flag) {
			usage();
			exit(1);
		}

		/*
		 * stat supplied file and /etc/passwd to
		 * see if they are the same file.
		 */
		if (stat(passwd, &sbuf1) != 0) {
			perror(passwd);
			goto bex;
		}
		if (stat(argv[2], &sbuf2) != 0) {
			perror(passwd);
			goto bex;
		}
		if (sbuf1.st_dev == sbuf2.st_dev) {
			if (sbuf1.st_ino == sbuf2.st_ino) {
				/*
				 * supplied file is /etc/passwd.
				 * don't do anything differently.
				 */
				goto nodiff;
			}
		}

		/*
		 * supplied file is not /etc/passwd
		 */
		passwd = argv[2];
		f_flag = 1;

		/*
		 * change uid to real uid so temp
		 * files aren't owned by root
		 */
		(void)setresuid(-1, ruid, -1);

		/*
		 * if f flag is used, then it could be any file,
		 * so user must have normal access permissions
		 * to be able to read and modify it.
		 */
		if (access(passwd, 06) != 0) {
			perror(passwd);
			goto bex;
		}

		(void) strcpy(opasswd, passwd);
		(void) make14charname(opasswd, ".old");
		(void) strcpy(temp, passwd);
		(void) make14charname(temp, ".tmp");
		argc--;
		argv++;
	    }
	    else
	    {
		if (uname)
		{
		    usage();
		    exit(1);
		}
		uname = argv[1];
#ifdef SecureWare
		if(ISSECURE)
                    (void) passwd_getlname();
#endif
	    }
	nodiff:
	    argc--;
	    argv++;
	}
	if (uname == NULL)
	{
#ifdef SecureWare
	    if(((ISSECURE) && ((uname = passwd_getlname()) == NULL)) ||
	       ((!ISSECURE) && ((uname = getlogin()) == NULL)))
#else 
	    if ((uname = getlogin()) == NULL)
#endif
            {	
		usage();
		exit(1);
	    }
	    else
		fprintf (stderr, (nl_msg(2,"Changing password for %s\n")), uname);
	}
#else /* not HP_NFS */
	if (argc < 2) {
#ifdef SecureWare
		if(((ISSECURE) && ((uname = passwd_getlname()) == NULL)) ||
		   ((!ISSECURE) && ((uname = getlogin()) == NULL))) 
#else	
		if ((uname = getlogin()) == NULL) 
#endif
		{
			usage();
			exit(1);
		} else
			fprintf (stderr, (nl_msg(2,"Changing password for %s\n")), uname);
	} else
#ifdef SecureWare
        {
		if(ISSECURE){
                    uname = argv[1];
                    (void) passwd_getlname();
		}
		else
		    uname = argv[1];
        }
#else	
		uname = argv[1];
#endif
#endif /* not HP_NFS */

#if defined(SecureWare) && defined(B1)
	if(ISB1){
	    (void) forcepriv(SEC_LIMIT);
	    /*
	     * set limits high so processes with small limits
	     * don't truncate the password file -- hesh
	     * Do this after privilege setup in getlname.
	     */
	    if (ulimit(2, LONG_MAX) == (long)-1) {
		perror("ulimit");
		exit(1);
	    }
	    (void) disablepriv(SEC_LIMIT);
	}
#endif

#ifdef AUDIT
        /* Stat the password file so that group/owner/mode can be restored */
        if (stat(passwd, &sbuf1) != 0) {
	    perror(passwd);
	    goto bex;
	}
#endif

#ifdef AUDIT
	/* Do self-auditing */
	if (f_flag) {
		(void)setresuid(-1, euid, -1);
	}
	if (audswitch(AUD_SUSPEND) == -1) {
		perror("audswitch");
		exit(1);
	}
	if (f_flag) {
		(void)setresuid(-1, ruid, -1);
	}

#ifdef HP_NFS
	/*
	 * set secure flag only if we are touching /etc/passwd
	 */
	if (!f_flag)
#endif /* HP_NFS */
	{
		secure = (stat(SECUREPASS, &s_pfile) < 0) ? 0:1;
	}

	if (secure) {
		setspwent();
		s_pwd = getspwnam(uname);
		setpwent();
		pwd = getpwnam(uname);
		u = getuid();
		if ((u != 0 && u != pwd->pw_uid)) {
			fprintf (stderr, (nl_msg(3,"Permission denied.\n")));
			goto bex;
		}

		/*
		 * Added check for invalid uid field 
		 */
		else if (s_pwd == NULL || pwd == NULL) {
			fprintf (stderr, (nl_msg(30,"Invalid login name.\n")));
			goto bex;
		}


		endspwent();
		endpwent();
		if (s_pwd->pw_passwd[0] && u != 0) {
			strcpy (opwbuf, getpass ((nl_msg(4,"Old password:"))));
			opwlen = strlen(opwbuf);       /* get length of old password */
			pw = crypt (opwbuf, s_pwd->pw_passwd);
			if (strcmp (pw, s_pwd->pw_passwd) != 0) {
				fprintf (stderr, (nl_msg(5,"Sorry.\n")));
				goto bex;
			}
		} else
			opwbuf[0] = '\0';
		if (*s_pwd->pw_age != NULL) {
			/* password age checking applies */
			when = (long) a64l (s_pwd->pw_age);
			/* max, min and week of last change are encoded in radix 64 */
			maxweeks = when & 077;
			minweeks = (when >> 6) & 077;
			when >>= 12;
			now  = time ((long *) 0)/WEEK;
			if (when <= now) 
				if (u != 0 && (now < when + minweeks)) {
					fprintf (stderr, (nl_msg(6,"Sorry: < %ld weeks since the last change\n")), minweeks);
					goto bex;
				}

			/*
			 * Dont allow user to change password if
			 * last set week is in the future and M < m 
			 */
			if (minweeks > maxweeks && u != 0) {
					fprintf (stderr, (nl_msg(7,"You may not change this password.\n")));
					goto bex;
				}
		}
	} else {
#endif /* AUDIT */
	while ((pwd = l_getpwent()) != NULL && strncmp (pwd->pw_name, uname, L_cuserid-1) != 0)
#ifdef HP_NFS
	    if (pwd->pw_name[0] == '+' || pwd->pw_name[0] == '-')
		try_nis_map++
#endif
		;
#ifdef SecureWare
	if(ISSECURE){
            if (passwd_old_passwd(&u, &pr, &pwd, uname))  {
                passwd_store(opwbuf, sizeof(pwbuf) - 1, nl_msg(4,"Old password:"));
		opwlen = strlen(opwbuf);       /* get length of old password */
                pw = passwd_crypt(opwbuf, pwd->pw_passwd);
		if (strcmp (pw, pwd->pw_passwd) != 0) {
			fprintf (stderr, (nl_msg(5,"Sorry.\n")));
			goto bex;
		}
	    } else
		opwbuf[0] = '\0';
	}
	else{
	    u = getuid();
	    if ((u != 0 && u != pwd->pw_uid)) {
		fprintf (stderr, (nl_msg(3,"Permission denied.\n")));
		goto bex;
	    }

	    /*
	     * Added check for invalid uid field 
	     */
	    else if (pwd == NULL) {
		fprintf (stderr, (nl_msg(30,"Invalid login name.\n")));
		goto bex;
	    }
	    l_endpwent();
	    if (pwd->pw_passwd[0] && u != 0) {
		strcpy (opwbuf, getpass ((nl_msg(4,"Old password:"))));
		opwlen = strlen(opwbuf);       /* get length of old password */
		pw = crypt (opwbuf, pwd->pw_passwd);
		if (strcmp (pw, pwd->pw_passwd) != 0) {
			fprintf (stderr, (nl_msg(5,"Sorry.\n")));
			goto bex;
		}
	    } else
		opwbuf[0] = '\0';

            if (*pwd->pw_age != NULL) {
                /* password age checking applies */
                when = (long) a64l (pwd->pw_age);
                /* max, min and week of last change are encoded in radix 64 */
                maxweeks = when & 077;
                minweeks = (when >> 6) & 077;
                when >>= 12;
                now  = time ((long *) 0)/WEEK;
                if (when <= now) 
                        if (u != 0 && (now < when + minweeks)) {
                                fprintf (stderr, (nl_msg(6,"Sorry: < %ld weeks since the last change\n")), minweeks);
                                goto bex;
                        }
                 if (minweeks > maxweeks && u != 0) {
                                fprintf (stderr, (nl_msg(7,"You may not change this password.\n")));
                                goto bex;
                        }
            }
	}
#else
#ifdef HP_NFS
	if ((pwd == NULL) && try_nis_map && (!f_flag))
	{
	    if (run_yppasswd (uname))
	    {
		exit (0);
	    }
	    goto bex;
	}
#endif /* HP_NFS */		
	u = getuid();
	if ((u != 0 && u != pwd->pw_uid)) {
		fprintf (stderr, (nl_msg(3,"Permission denied.\n")));
		goto bex;
	}

	/*
	 * Added check for invalid uid field 
	 */
	else if (pwd == NULL) {
	    fprintf (stderr, (nl_msg(30,"Invalid login name.\n")));
	    goto bex;
	}
	l_endpwent();
	if (pwd->pw_passwd[0] && u != 0) {
		strcpy (opwbuf, getpass ((nl_msg(4,"Old password:"))));
		opwlen = strlen(opwbuf);       /* get length of old password */
		pw = crypt (opwbuf, pwd->pw_passwd);
		if (strcmp (pw, pwd->pw_passwd) != 0) {
			fprintf (stderr, (nl_msg(5,"Sorry.\n")));
			goto bex;
		}
	} else
		opwbuf[0] = '\0';

        if (*pwd->pw_age != NULL) {
                /* password age checking applies */
                when = (long) a64l (pwd->pw_age);
                /* max, min and week of last change are encoded in radix 64 */
                maxweeks = when & 077;
                minweeks = (when >> 6) & 077;
                when >>= 12;
                now  = time ((long *) 0)/WEEK;
                if (when <= now) 
                        if (u != 0 && (now < when + minweeks)) {
                                fprintf (stderr, (nl_msg(6,"Sorry: < %ld weeks since the last change\n")), minweeks);
                                goto bex;
                        }
                 if (minweeks > maxweeks && u != 0) {
                                fprintf (stderr, (nl_msg(7,"You may not change this password.\n")));
                                goto bex;
                        }
	}
#endif 
#ifdef AUDIT
	}
#endif /* AUDIT */
tryagn:
	if( insist >= 3) {      /* three chances to meet triviality standard */
		fprintf(stderr, (nl_msg(8,"Too many failures - try later.\n")));
		goto bex;
		}
#ifdef SecureWare
#ifdef TMAC
	if(!getunum(pr)) {	/* verify administrator supplied gen number */
		fprintf(stderr, (nl_msg(8,"Too many failures - try later.\n")));
		goto bex;
	} else
#endif /* TMAC */
	if((ISSECURE) && (passwd_is_random(uname, pr, u)))
                passwd_get_random(uname, pr, pwbuf, sizeof(pwbuf)-1);
        else  {
	    if(ISSECURE)
                passwd_store(pwbuf, sizeof(pwbuf) - 1, nl_msg(9,"New password:"));
	    else
		strcpy (pwbuf, getpass ((nl_msg(9,"New password:"))));
#else
	strcpy (pwbuf, getpass ((nl_msg(9,"New password:"))));
#endif
	pwlen = strlen (pwbuf);

	/* Make sure new password is long enough */

#ifdef SecureWare
	if ( (ISSECURE ? pwlen : ( u != 0 )) && (pwlen < MINLENGTH ) ) {
#else
        if (u != 0 && (pwlen < MINLENGTH ) ) {
#endif
		fprintf(stderr, (nl_msg(10,"Password is too short - must be at least 6 characters\n")));
		insist++;
		goto tryagn;
		}

	/* Check the circular shift of the logonid */

#ifdef SecureWare
	if ( (ISSECURE ? pwlen : ( u != 0 )) && circ(uname, pwbuf) ) {
#else
	if( u != 0 && circ(uname, pwbuf) ) {
#endif
		fprintf(stderr, (nl_msg(11,"Password cannot be circular shift of logonid\n")));
		insist++;
		goto tryagn;
		}

	/* Insure passwords contain at least two alpha characters */
	/* and one numeric or special character */               

	flags = 0;
	flag = 0;
	p = pwbuf;
#ifdef SecureWare
	if ( ISSECURE ? pwlen : (u != 0) ) {
#else
	if (u != 0) {
#endif
		while (c = *p++&0377){
			if( isalpha(c) && flag ) flags |= 1;
				else if( isalpha(c) && !flag ){
					flags |= 2;
					flag = 1;
					}
				else if( isdigit(c) ) flags |= 4;
				else flags |= 8;
				}
		 
		/*		7 = lca,lca,num
		 *		7 = lca,uca,num
		 *		7 = uca,uca,num
		 *		11 = lca,lca,spec
		 *		11 = lca,uca,spec
		 *		11 = uca,uca,spec
		 *		15 = spec,num,alpha,alpha
		 */
		 
			if ( flags != 7 && flags != 11 && flags != 15  ) {
				fprintf(stderr,(nl_msg(12,"Password must contain at least two alphabetic characters and\n")));
				fprintf(stderr,(nl_msg(13,"at least one numeric or special character.\n")));
				insist++;
				goto tryagn;
				}
		}
#ifdef SecureWare
	if ( ISSECURE ? pwlen : (u != 0) ) {
#else
	if ( u != 0 ) {
#endif
		p = pwbuf;
		o = opwbuf;
		if( pwlen >= opwlen) {
			i = pwlen;
			k = pwlen - opwlen;
			}
			else {
				i = opwlen;
				k = opwlen - pwlen;
				}
		for( j = 1; j  <= i; j++ ) if( *p++ != *o++ ) k++;
		if( k  <  3 ) {
			fprintf(stderr, (nl_msg(14,"Passwords must differ by at least 3 positions\n")));
			insist++;
			goto tryagn;
			}
	}
#ifdef SecureWare

        if ((ISSECURE) && (!passwd_legal(pwbuf, pr)))  {
                insist++;
                goto tryagn;
        }
#endif
	/* Insure password was typed correctly, user gets three chances */

#ifdef SecureWare
	if(ISSECURE)
            passwd_store(buf, sizeof(pwbuf) - 1, nl_msg(15,"Re-enter new password:"));
	else
	    strcpy (buf, getpass ((nl_msg(15,"Re-enter new password:"))));
#else
	strcpy (buf, getpass ((nl_msg(15,"Re-enter new password:"))));
#endif
	if (strcmp (buf, pwbuf)) {
#ifdef SecureWare
	    	if(ISSECURE){
                    passwd_buf(buf);
                    passwd_buf(pwbuf);
	    	}
#endif
		if (++count > 2) {
			fprintf (stderr, (nl_msg(16,"Too many tries; try again later.\n")));
			goto bex;
		} else
			fprintf (stderr, (nl_msg(17,"They don't match; try again.\n")));
		goto tryagn;
	}
#ifdef SecureWare
	if(ISSECURE)
            passwd_buf(buf);
        } /* closes else clause of is_random() check */
#endif

	/* Construct salt, then encrypt the new password */

#ifdef SecureWare
	if(ISSECURE)
            salt = get_seed(pr);
	else{
	    time (&salt);
	    salt += getpid();
	}
#else 
	time (&salt);
	salt += getpid();
#endif

	saltc[0] = salt & 077;
	saltc[1] = (salt >> 6) & 077;
	for (i=0; i<2; i++) {
		c = saltc[i] + '.';
		if (c>'9') c += 7;
		if (c>'Z') c += 6;
		saltc[i] = c;
	}
#ifdef SecureWare
	if(ISSECURE)
            pw = passwd_crypt(pwbuf, saltc);
	else
	    pw = crypt (pwbuf, saltc);
#else	
	pw = crypt (pwbuf, saltc);
#endif
	signal (SIGHUP, SIG_IGN);
	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
#ifdef SIGTSTP
	signal (SIGTSTP, SIG_IGN);
#endif /* SIGTSTP */

#ifdef SecureWare
	if(ISSECURE)
            passwd_change(uname, pr, pw);
#endif
#ifdef AUDIT
	if (secure)
		strcpy(temp, SECURETEMP);
#endif /* AUDIT */

	if ((tfd = open(temp, O_CREAT | O_EXCL | O_WRONLY, 0)) < 0) {
		if (errno == EEXIST) {
			fprintf (stderr, (nl_msg(18,"Temporary file busy; try again later.\n")));
		}
		else {
			fprintf (stderr, (nl_msg(19,"Cannot create temporary file\n")));
		}
		goto bex;
	}

#ifdef AUDIT
	if (secure)
		(void)fchmod(tfd, S_IRUSR);	/* only root can read it */
	else
#endif /* AUDIT */
		(void)fchmod(tfd, S_IRUSR|S_IRGRP|S_IROTH);
		

/*
 *	Between the time that the previous access completes
 *	and the following fopen completes, it is possible for
 *	some other user to sneak in and foul things up.
 *	It is not possible to solve this by using creat to
 *	create the file with mode 0444, because the creat will
 *	always succeed if our effective uid is 0.
 */

/*
 * 890313 -- hesh
 *
 * the above problem has been fixed.  the code used to do
 * access(ptmp) then fopen(ptmp).  we fix the problem by
 * using using open with the O_CREAT|O_EXCL flags; this
 * will fail for root too.  we can then do an fdopen()
 * to get a file pointer.
 */

	if ((tf = fdopen (tfd, "w")) == NULL) {
		/*
		 * reuse this error message since it's
		 * appropriate enough for this purpose
		 */
		fprintf (stderr, (nl_msg(19,"Cannot create temporary file\n")));
		goto bex;
	}

/*
 *	copy passwd to temp, replacing matching lines
 *	with new password.
 */

#ifdef AUDIT
	if (secure) {
		setspwent();
		while ((s_pwd = getspwent()) != NULL) {
			if (strcmp (s_pwd->pw_name, uname, L_cuserid-1) == 0) {
				s_pwd->pw_passwd = pw;
				if (*s_pwd->pw_age != NULL) {
					if (maxweeks == 0) 
						*s_pwd->pw_age = '\0';
					else {
						when = maxweeks + (minweeks << 6) + (now << 12);
						s_pwd->pw_age = l64a (when);
					}
				}
			}
			if (putspwent (s_pwd, tf) != 0) {
				fprintf(stderr, (nl_msg(27,"putspwent() failed\n")));
				perror(temp);
				unlink(temp);
				exit(1);
			}
		}
		endspwent();
	} else {
#endif /* AUDIT */
	while ((pwd = l_getpwent()) != NULL) {
		if (strcmp (pwd->pw_name, uname, L_cuserid-1) == 0) {
			u = getuid();
			if (u != 0 && u != pwd->pw_uid) {
				fprintf (stderr, (nl_msg(20,"Permission denied.\n")));
				goto out;
			}
			pwd->pw_passwd = pw;
			if (*pwd->pw_age != NULL) {
				if (maxweeks == 0) 
					*pwd->pw_age = '\0';
				else {
					when = maxweeks + (minweeks << 6) + (now << 12);
					pwd->pw_age = l64a (when);
				}
			}
			if (pw_toolong(pwd))
			{
			    fprintf (stderr, (nl_msg(31,"Password entry is too long\n")));
			    goto out;
			}
		}
		if (putpwent (pwd, tf) != 0) {
			fprintf(stderr, (nl_msg(27,"putpwent() failed\n")));
			perror(temp);
			unlink(temp);
			exit(1);
		}
	}
	l_endpwent ();
#ifdef AUDIT
	}
#endif /* AUDIT */
		/* fsync(2) does not exist on the AT&T 3B2 */
	fsync(tf);
	fclose (tf);

/*
 *	Rename temp file back to passwd file.
 */

#ifdef AUDIT
	if (secure) {
		strcpy(opasswd, OSECUREPASS);
		strcpy(passwd, SECUREPASS);
	}
#endif /* AUDIT */

	if (unlink (opasswd) && access (opasswd, 0) == 0) {
		fprintf (stderr, (nl_msg(21,"cannot unlink %s\n")), opasswd);
		goto out;
	}

	if (link (passwd, opasswd)) {
		fprintf (stderr, (nl_msg(22,"cannot link %s to %s\n")), passwd, opasswd);
		goto out;
	}

	if (unlink (passwd)) {
		fprintf (stderr, (nl_msg(23,"cannot unlink %s\n")), passwd);
		goto out;
	}

	if (link (temp, passwd)) {
		fprintf (stderr, (nl_msg(24,"cannot link %s to %s\n")), temp, passwd);
		if (link (opasswd, passwd)) {
			fprintf (stderr, (nl_msg(25,"cannot recover %s\n")), passwd);
			goto bex;
		}
		goto out;
	}

#ifdef AUDIT
        /* Restore group/owner/mode on password file if not secure password */
	if (! secure) {
	    if (chown(passwd, sbuf1.st_uid, sbuf1.st_gid) != 0) {
		fprintf (stderr, (nl_msg(28,"cannot chown %s\n")), passwd);
		goto out;
	    }

	    if (chmod(passwd, sbuf1.st_mode) != 0) {
		fprintf (stderr, (nl_msg(29,"cannot chmod %s\n")), passwd);
		goto out;
	    }
	}
#endif

	if (unlink (temp)) {
		fprintf (stderr, (nl_msg(26,"cannot unlink %s\n")), temp);
		goto out;
	}
#ifdef AUDIT
	/* Write audit record */
	txtptr = (char *)audrec.aud_body.text;
	strcpy (txtptr, "User= ");
	strcat (txtptr, uname);
	strcat (txtptr, " Passwd successfully changed");
	audrec.aud_head.ah_pid = getpid();
	audrec.aud_head.ah_error = 0;
	audrec.aud_head.ah_event = EN_PASSWD;
	audrec.aud_head.ah_len = strlen (txtptr);
	if (f_flag) {
		(void)setresuid(-1, euid, -1);
	}
	(void)audwrite(&audrec);
	/* Resume auditing */
	(void)audswitch(AUD_RESUME);
	if (f_flag) {
		(void)setresuid(-1, ruid, -1);
	}
#endif /* AUDIT */
	exit (0);

out:
	unlink (temp);

bex:
#ifdef AUDIT
	/* Write audit record */
	txtptr = (char *)audrec.aud_body.text;
	strcpy (txtptr, "User= ");
	strcat (txtptr, uname);
	strcat (txtptr, " Attempt to change passwd failed");
	audrec.aud_head.ah_pid = getpid();
	audrec.aud_head.ah_error = 1;
	audrec.aud_head.ah_event = EN_PASSWD;
	audrec.aud_head.ah_len = strlen (txtptr);
	if (f_flag) {
		(void)setresuid(-1, euid, -1);
	}
	(void)audwrite(&audrec);
	/* Resume auditing */
	(void)audswitch(AUD_RESUME);
	if (f_flag) {
		(void)setresuid(-1, ruid, -1);
	}
#endif /* AUDIT */
#ifdef SecureWare
	if(ISSECURE)
            passwd_no_change(uname, pr, (char *) 0);
#endif
	exit (1);
}
circ( s, t )
char *s, *t;
{
	char c, *p, *o, *r, buff[25], ubuff[25], pubuff[25];
	int i, j, k, l, m;
	 
	m = 2;
	i = strlen(s);
	o = &ubuff[0];
	for( p = s; c = *p++; *o++ = c ) if( islower(c) ) c = toupper(c);
	*o = '\0';
	o = &pubuff[0];
	for( p = t; c = *p++; *o++ = c ) if( islower(c) ) c = toupper(c);
	*o = '\0';
	 
	p = &ubuff[0];
	while( m-- ) {
		for( k = 0; k  <=  i; k++) {
			c = *p++;
			o = p;
			l = i;
			r = &buff[0];
			while(--l) *r++ = *o++;
			*r++ = c;
			*r = '\0';
			p = &buff[0];
			if( strcmp( p, pubuff ) == 0 ) return(1);
			}
		p = p + i;
		r = &ubuff[0];;
		j = i;
		while( j-- ) *--p = *r++;
	}
	return(0);
}


usage()
{
#ifdef HP_NFS
	fprintf (stderr, (nl_msg(1,"Usage: passwd [ -f file ] [ name ]\n")));
#else /* not HP_NFS */
	fprintf (stderr, (nl_msg(1,"Usage: passwd [ name ]\n")));
#endif /* not HP_NFS */
}

#ifdef HP_NFS
extern void rewind();
extern long atol();
extern FILE *fopen();
extern int fclose();
extern char *fgets();

static char EMPTY[] = "";
static FILE *pwf = NULL;
static char line[BUFSIZ+1];
static struct passwd pwdbuf;


void
l_endpwent()
{
	if(pwf != NULL) {
		(void) fclose(pwf);
		pwf = NULL;
	}
}

static char *
l_pwskip(p)
register char *p;
{
	while(*p && *p != ':' && *p != '\n')
		++p;
	if(*p == '\n')
		*p = '\0';
	else if(*p)
		*p++ = '\0';
	return(p);
}


struct passwd *
l_getpwent()
{
	register char *p;
	char *end;
	long	x, strtol();

	if (pwf == NULL) {
		if( (pwf = fopen( passwd, "r" )) == NULL )
			return(0);
	}

	p = fgets(line, BUFSIZ, pwf);
	if(p == NULL)
		return(NULL);
	pwdbuf.pw_name = p;
	p = l_pwskip(p);
	pwdbuf.pw_passwd = p;
	p = l_pwskip(p);
	x = strtol(p, &end, 10);	
	p = l_pwskip(p);
	pwdbuf.pw_uid = (x < -2 || x > MAXUID)? (MAXUID+1): x;
	x = strtol(p, &end, 10);	
	p = l_pwskip(p);
	pwdbuf.pw_gid = (x < 0 || x > MAXUID)? (MAXUID+1): x;
	pwdbuf.pw_comment = EMPTY;
	pwdbuf.pw_gecos = p;
	p = l_pwskip(p);
	pwdbuf.pw_dir = p;
	p = l_pwskip(p);
	pwdbuf.pw_shell = p;
	(void) l_pwskip(p);

	p = pwdbuf.pw_passwd;
	while(*p && *p != ',')
		p++;
	if(*p)
		*p++ = '\0';
	pwdbuf.pw_age = p;
	return(&pwdbuf);
}


char *make14charname(name, suffix)
char *name, *suffix;
{
    char *strncpy(), *strcat(), *strrchr();
    char *firstchar;
    int len, suflen;

    if ((suflen = strlen(suffix)) > 14)
	suflen = 14;

    if ((firstchar = strrchr(name, '/')) != NULL)
	firstchar++;
    else
	firstchar = name;
	

    len = strlen(firstchar);

    if ((len + suflen) > 14)
	(void) strncpy(firstchar + (14 - suflen), suffix, suflen);
    else
	(void) strcat(firstchar, suffix);

    return(name);
}


run_yppasswd (user)
  char	*user;
{
    int	ret;
    
    switch (fork())
    {
      case 0:
	(void) setresuid (-1, ruid, -1);
	execl ("/usr/bin/yppasswd", "yppasswd", user, (char *)0);
	exit (1);
	
      case -1:
	return(0);
	
      default:
	wait (&ret);
	if (WIFEXITED(ret))
	    return (WEXITSTATUS(ret) ? 0 : 1);
	else
	    return(0);
    }
}

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
