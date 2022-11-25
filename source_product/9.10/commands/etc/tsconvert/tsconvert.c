static char *HPUX_ID = "@(#) $Revision: 66.1 $";

/*
 * tsconvert [-c] [-p] [-r]
 *
 *	By default, tsconvert converts the password file to a shadow
 *	password file, and converts at and crontab jobs to use this
 *	shadow password file.
 *
 *	The -r option reverses the notion of conversion (I.e., it
 *	restores the passwords in the regular password file and it
 *	removes the specially modified at and crontab jobs).
 *
 *	The -c option prevents at and crontab files from being converted.
 *
 *	The -p option prevents the password file from being converted.
 *
 *	Supplying both -c and -p is a noop.
 *
 *	An undocumented -d option is provided for debugging.  Debugging
 *	information will be intermingled with the normal error output.
 *	You can identify the debugging information by it's characteristic
 *	"command: function: error string..." style of printing.
 */

#include <sys/types.h>
#include <varargs.h>
#include <stdio.h>
#include <pwd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>


#define CRON_CDF        "/usr/spool/cron+"
#define	ATAIDS		"/usr/spool/cron/.ataids"
#define	ATDIR		"/usr/spool/cron/atjobs"
#define	CRONAIDS	"/usr/spool/cron/.cronaids"
#define	CRONDIR		"/usr/spool/cron/crontabs"
#define	OPASSWD		"/etc/opasswd"
#define	PASSWD		"/etc/passwd"
#define	PTMP		"/etc/ptmp"
#define	SPASSWD		"/.secure/etc/passwd"
#define	STMP		"/etc/stmp"

#define	STAR		"*"		/* replacement for password */
#define	EMPTY		""		/* replacement for age */

#define MAXDIR		1024		/* /usr/spool/cron+/cnode/... */

extern int		errno;
extern char *		sys_errlist[];	/* for _perror() strings */

extern int		delaudid();	/* delete audit ID files */
extern int		docron();	/* non-secure -> secure cron */
extern int		dopswd();	/* non-secure -> secure passwd */
extern int		get_opts();	/* get options */
extern int		main();
extern int		mkataudid();	/* make at's audit ID files */
extern int		mkcdirs();	/* create ATAIDS, CRONAIDS */
extern int		mkcronaudid();	/* make cron audit ID files */
extern int		mkpdirs();	/* create /.secure, etc */
extern int		undocron();	/* secure -> non-secure cron */
extern int		undopswd();	/* secure -> non-secure passwd */
extern int		is_CDF();	/* is /usr/spool/cron+ a CDF */
extern char *		_perror();	/* error string printing */
extern char *		basename();	/* trailing component of path */
extern char *		strrchr();
extern FILE *		getfp();	/* returns file pointers */
extern void		pcleanup();	/* toss unwanted cruft */
extern void		scleanup();	/* toss unwanted cruft */
extern void		error();	/* for more detailed errors */
extern void		usage();

int			cronflg = 1;	/* convert cron flag on by default */
int			debug = 0;	/* debug information off by default */
int			pswdflg = 1;	/* convert passwd flag on by default */
int			reverse = 0;	/* reverse conversion off by default */
int			CDF_flag = 0;   /* /usr/spool/cron+ is a CDF */

char *			arguments = "[-c] [-p] [-r]";
char *			command = (char *)0;
char *			opasswd = OPASSWD;
char *			passwd = PASSWD;
char *			ptmp = PTMP;
char *			spasswd = SPASSWD;
char *			stmp = STMP;

FILE *			passwdfp = (FILE *)0;
FILE *			spasswdfp = (FILE *)0;


int
main(ac, av)
int ac;
char *av[];
{

	auto	char *		function = "main";


	command = basename(*av);
	av++; ac--;

	/*
	 * only the superuser can run this
	 */
   	if (getuid() != 0) {                                        
		(void)fprintf(stderr, "Not Superuser.\n");
		exit(1);
   	}                                    

	/*
	 * get options
	 */
	if (get_opts(&ac, &av) < 0) {
		usage(command, arguments);
		exit(1);
	}

	/*
	 * prevent keyboard input from messing up the conversion
	 */
	(void)signal(SIGHUP, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);

	/*
	 * set limits high so processes with small limits
	 * don't truncate various files
	 */
	if (ulimit(2, LONG_MAX) == (long)-1) {
		error(command, function,
			"ulimit(2, %ld) failed: %s\n",
			LONG_MAX, _perror());
		(void)fprintf(stderr, "Can't reset limits.\n");
		return(-1);
	}

	/*
	 * check if we want to convert the password file
	 */
	if (pswdflg) {
		if (reverse) {
			if (undopswd() != 0) {
				error(command, function,
					"undopswd() failed\n");
				exit(1);
			}
		}
		else {
			if (dopswd() != 0) {
				error(command, function,
					"dopswd() failed\n");
				exit(1);
			}
		}
	}


        /*
	/*  If cronflg and pswdflg are TRUE then check to see if cron+ exists
	/*  and if its a CDF.
	 */
        if (cronflg && pswdflg) 
	  CDF_flag = is_CDF();


	/*
	 * check if we want to convert at/batch/crontab files
	 */
	if (cronflg) {
		if (reverse) {
			if (undocron() != 0) {
				error(command, function,
					"undocron() failed\n");
				exit(1);
			}
		}
		else {
			if (docron() != 0) {
				error(command, function,
					"docron() failed\n");
				exit(1);
			}
		}
	}
	exit(0);
}


/*
 * get_opts:
 *	parse command line options
 */
int
get_opts(ac, av)
int *ac;
char **av[];
{
	auto	int		rvalue = 0;
	auto	char *		function = "get_opts";
	auto	char *		sp;


	while ((*ac > 0) && (***av == '-')) {
		sp = **av;
		(*av)++; (*ac)--;
		while (*(++sp)) switch (*sp) {
		    case 'c':
			cronflg = 0;
			break;
		    case 'd':
			debug = 1;
			break;
		    case 'p':
			pswdflg = 0;
			break;
		    case 'r':
			reverse = 1;
			break;
		    default:
			(void)fprintf(stderr,
				"'%c': unknown option\n",
				*sp);
			rvalue = -1;
			break;
		}
	}
	return(rvalue);
}


/*
 * docron:
 *	make cron directories, scan directories
 */
int
docron()
{
	auto	char *		function = "docron";
        
	auto	struct dirent *dirent = (struct dirent *)0;
	auto    DIR *dirp = (DIR *)0;
	auto    struct stat buf;
	auto    char dir_path[MAXDIR];
        auto    char ataids_dir[MAXDIR];
        auto    char cronaids_dir[MAXDIR];
        auto    char at_dir[MAXDIR];
        auto    char cron_dir[MAXDIR];


	/*
	 * if secure passwd file doesn't exist, just return
	 */
	if (access(spasswd, 0)) {
		(void)fprintf(stderr,
			"Can't convert at and crontab jobs without %s\n",
			spasswd);
		return(0);
	}

	(void)printf("Converting at and crontab jobs...\n");
	(void)fflush(stdout);


	/*
	/*  /usr/spool/cron+ is a CDF so make all the appropriate directories
	/* and files for all the directories under cron+.
	 */
	if (CDF_flag)
	{
            if ((dirp = opendir(CRON_CDF)) == (DIR *)0) 
            {
                error(command, function, "opendir(%s) failed: %s\n", 
                      _perror());
                fprintf(stderr, "Can't open directory: %s\n", CRON_CDF);
                return(-1);
	    }

            /*
	    /*  For each directory in /usr/spool/cron+ create the
            /*  appropriate subdirectories and files.
             */

            while ((dirent = readdir(dirp)) != (struct dirent *) 0)
	    {
                if (dirent->d_ino == (ino_t) 0)
  		    continue;
		if (strcmp(dirent->d_name,".") == 0)
		    continue;
		if (strcmp(dirent->d_name,"..") == 0)
		    continue;

                /* dir_path == "/usr/spool/cron+/dirent->d_name" */
                sprintf(dir_path, "%s/%s", CRON_CDF, dirent->d_name);

		/* Determine if d_name is a directory or a regular file */
	        if (stat(dir_path,&buf) != 0) 
	        {
		    error(command, function, "stat(%s) failed: %s\n",
	        	  dir_path, _perror());
                    continue;
                }
	        
                if ((buf.st_mode & S_IFDIR) == 0) /* Not a directory */
                    continue;
		else  /* dirent->d_name is a directory */
                {
                    sprintf(ataids_dir, "%s/%s", dir_path, ".ataids");
                    sprintf(cronaids_dir, "%s/%s", dir_path, ".cronaids");
                    sprintf(at_dir, "%s/%s", dir_path, "atjobs");
                    sprintf(cron_dir, "%s/%s", dir_path, "crontabs");

       	            /*
    	             * make special directories
    	             */

                    if (mkcdirs(ataids_dir) != 0) 
                    {
            	        error(command, function, "mkcdirs(%s) failed\n",
            	              ataids_dir);
            	        continue;
                    }

                    if (mkcdirs(cronaids_dir) != 0) 
                    {
            	        error(command, function, "mkcdirs(%s) failed\n",
            	              cronaids_dir);
            	        continue;
                    }
    
                    /*
                     * make at audit ID files
                     */
                    if (scandir(at_dir, mkataudid) != 0) 
                    {
            	        error(command, function, 
                              "scandir(%s, mkataudid) failed\n", at_dir);
            	        continue;
                    }
    
                    /*
                     * make cron audit ID files
                     */
                    if (scandir(cron_dir, mkcronaudid) != 0) 
                    {
            	        error(command, function,
            	              "scandir(%s, mkcronaudid) failed\n", cron_dir);
            	        continue;
                    }

                } /* else we have a directory */

            } /* while files in cron+ */    
  	    (void)closedir(dirp);

	} /* if (CDF_flag) */
        else /* Not a CDF */
        {

	    /*
	     * make special directories
	     */
	    if (mkcdirs(ATAIDS) != 0) {
	        	error(command, function,
			    "mkcdirs(%s) failed\n",
			    ATAIDS);
	    	    return(-1);
	    }
	    if (mkcdirs(CRONAIDS) != 0) {
		    error(command, function,
			    "mkcdirs(%s) failed\n",
			    CRONAIDS);
		    return(-1);
	    }

	    /*
	     * make at audit ID files
	     */
	    if (scandir(ATDIR, mkataudid) != 0) {
		    error(command, function,
			    "scandir(%s, mkataudid) failed\n", ATDIR);
		    return(-1);
	    }

	    /*
	     * make cron audit ID files
	     */
	    if (scandir(CRONDIR, mkcronaudid) != 0) {
		    error(command, function,
			    "scandir(%s, mkcronaudid) failed\n", CRONDIR);
		    return(-1);
	    }

	    (void)printf("At and crontab files converted.\n");
	    return(0);
        } /* else not a CDF */

        (void)printf("At and crontab files converted.\n");
        return(0);

}


/*
 * undocron:
 *	unconvert at and crontab files (remove audit ID files)
 */
int
undocron()
{
	auto	char *		function = "undocron";

        auto    struct dirent *dirent = (struct dirent *)0;
        auto    DIR *dirp = (DIR *)0;
        auto    struct stat buf;
        auto    char dir_path[MAXDIR];
        auto    char ataids_dir[MAXDIR];
        auto    char cronaids_dir[MAXDIR];



	(void)printf("Deleting at and crontab audit ID files...\n");
	(void)fflush(stdout);


	/*
	/*  /usr/spool/cron+ is a CDF so make all the appropriate directories
	/* and files for all the directories under cron+.
	 */
	if (CDF_flag)
	{
            if ((dirp = opendir(CRON_CDF)) == (DIR *)0) 
            {
                error(command, function, "opendir(%s) failed: %s\n", 
                      _perror());
                fprintf(stderr, "Can't open directory: %s\n", CRON_CDF);
                return(-1);
	    }

            /*
	     *  For each directory in /usr/spool/cron+ unconvert the
             *  appropriate files.
             */

            while ((dirent = readdir(dirp)) != (struct dirent *) 0)
	    {
                if (dirent->d_ino == (ino_t) 0)
  		    continue;
		if (strcmp(dirent->d_name,".") == 0)
		    continue;
		if (strcmp(dirent->d_name,"..") == 0)
		    continue;

                /* dir_path == "/usr/spool/cron+/dirent->d_name" */
                sprintf(dir_path, "%s/%s", CRON_CDF, dirent->d_name);

		/* Determine if d_name is a directory or a regular file */
	        if (stat(dir_path,&buf) != 0) 
	        {
		    error(command, function, "stat(%s) failed: %s\n",
	        	  dir_path, _perror());
                    continue;
                }
	        
                if ((buf.st_mode & S_IFDIR) == 0) /* Not a directory */
                    continue;
		else  /* dirent->d_name is a directory */
                {
                    sprintf(ataids_dir, "%s/%s", dir_path, ".ataids");
                    sprintf(cronaids_dir, "%s/%s", dir_path, ".cronaids");

           	   /*
	            * just delete files; directories can stay
	            */
	            if (scandir(ataids_dir, delaudid) != 0) 
                    {
		        error(command, function, 
                              "scandir(%s, delaudid) failed\n", ataids_dir);
		        continue;
	            }
	            if (scandir(cronaids_dir, delaudid) != 0) 
                    {
		        error(command, function,
			      "scandir(%s, delaudid) failed\n", cronaids_dir);
		        continue;
	            }

		} /* else */

	    } /* while */
  	    (void)closedir(dirp);

	} /* if CDF */
	else
	{

	    /*
	     * just delete files; directories can stay
	     */
	    if (scandir(ATAIDS, delaudid) != 0) {
	    	    error(command, function,
			    "scandir(%s, delaudid) failed\n", ATAIDS);
		    return(-1);
	    }
	    if (scandir(CRONAIDS, delaudid) != 0) {
		    error(command, function,
			    "scandir(%s, delaudid) failed\n", CRONAIDS);
		    return(-1);
	    }
	}

	(void)printf("At and crontab audit ID files deleted.\n");
	return(0);
}


/*
 * dopswd:
 *	convert password file to shadow password file
 */
int
dopswd()
{
	auto	char		tmpbuf[20];
	auto	char *		function = "dopswd";
	auto	aid_t		auditid = (aid_t)0;
	auto	struct passwd *	pw = (struct passwd *)0;
	auto	struct s_passwd spw;
	auto	struct s_passwd *sptr = &spw;


	/*
	 * if secure passwd file already exists, just return
	 */
	if (!access(spasswd, 0)) {
		(void)fprintf(stderr, "%s already exists.\n", spasswd);
		return(0);
	}

	(void)printf("Creating %s...\n", spasswd);
	(void)fflush(stdout);

	/*
	 * make directories
	 */
	if (mkpdirs("/.secure") != 0) {
		error(command, function,
			"mkpdirs(/.secure) failed\n");
		return(-1);
	}
	if (mkpdirs("/.secure/etc") != 0) {
		error(command, function,
			"mkpdirs(/.secure/etc) failed\n");
		return(-1);
	}

	/*
	 * get file pointers for both password files
	 */
	if ((spasswdfp = getfp(stmp, 0400)) == (FILE *)0) {
		error(command, function,
			"getfp(%s, 0400) failed\n",
			stmp);
		scleanup();
		return(-1);
	}
	if ((passwdfp = getfp(ptmp, 0444)) == (FILE *)0) {
		error(command, function,
			"getfp(%s, 0444) failed\n",
			ptmp);
		pcleanup();
		scleanup();
		return(-1);
	}

	/*
	 * toss old password file if it exists
	 */
	if (unlink(opasswd) != 0) {
		/*
		 * something's wrong if errno isn't ENOENT
		 */
		if (errno != ENOENT) {
			error(command, function,
				"unlink(%s) failed: %s\n",
				opasswd, _perror());
			(void)fprintf(stderr,
				"Can't remove old password file.\n");
			pcleanup();
			scleanup();
			return(-1);
		}
	}

	/*
	 * hold on to password file
	 */
	if (link(passwd, opasswd) != 0) {
		error(command, function,
			"link(%s, %s) failed: %s\n",
			passwd, opasswd, _perror());
		(void)fprintf(stderr, "Can't backup password file.\n");
		pcleanup();
		scleanup();
		return(-1);
	}

	setpwent();

	/*
	 * for each entry in /etc/passwd, create an entry in
	 * /.secure/etc/passwd. Move the encrypted passwd over
	 * from /etc/passwd and '*' out the passwd field in
	 * /etc/passwd. If existing passwd field is null then
	 * set passwd to require user to enter one at login.
	 *
	 * assign audit ID's sequentially from 0.
	 */

	auditid = (aid_t)0;
	while((pw = getpwent()) != (struct passwd *)0) {
		sptr->pw_name =  pw->pw_name;
		sptr->pw_passwd = pw->pw_passwd;

		if ((strlen(pw->pw_passwd) <= 0) &&
		    (strlen(pw->pw_age) <= 0)) {
			strcpy(tmpbuf, "..");
			sptr->pw_age = tmpbuf;
		}
		else {
			sptr->pw_age = pw->pw_age;
		}

		sptr->pw_audid = auditid++;
		sptr->pw_audflg = 1;
		if (putspwent(sptr, spasswdfp) != 0) {
			error(command, function,
				"putspwent(%s) failed\n",
				stmp);
			(void)fprintf(stderr,
				"Can't write secure password file;\n");
			(void)fprintf(stderr,
				"password file unchanged.\n");
			pcleanup();
			scleanup();
			return(-1);
		}
		
		pw->pw_passwd = STAR;
		pw->pw_age = EMPTY;

		if (putpwent(pw, passwdfp) != 0) {
			error(command, function,
				"putpwent(%s) failed\n",
				ptmp);
			(void)fprintf(stderr,
				"Can't write new password file;\n");
			(void)fprintf(stderr,
				"password file unchanged.\n");
			pcleanup();
			scleanup();
			return(-1);
		}
	}

	endpwent();

	(void)fclose(spasswdfp);
	(void)fclose(passwdfp);

	/*
	 * toss original password file
	 */
	if (unlink(passwd) != 0) {
		error(command, function,
			"unlink(%s) failed: %s\n",
			passwd, _perror());
		(void)fprintf(stderr, "Can't remove old password file;\n");
		(void)fprintf(stderr, "password file unchanged.\n");
		pcleanup();
		scleanup();
		return(-1);
        }

	/*
	 * link newly created temporary
	 * password file to password file
	 */
        if (link(ptmp, passwd) != 0) {
		error(command, function,
			"link(%s, %s) failed: %s\n",
			ptmp, passwd, _perror());
		(void)fprintf(stderr, "Can't create new password file;\n");

		/*
		 * if this doesn't work, there is a major problem.
		 * just bail rather than clean up so that we can
		 * recover from /etc/opasswd.
		 */
		if (link(opasswd, passwd) != 0) {
			error(command, function,
				"link(%s, %s) failed: %s\n",
				opasswd, passwd, _perror());
			(void)fprintf(stderr,
				"WARNING: password file recovery FAILED;\n");
			(void)fprintf(stderr,
				"password file saved in %s\n",
				opasswd);
			exit(1);
		}
		/*
		 * if we get here, then recovery succeeded
		 * so it is okay to clean up
		 */
		pcleanup();
		scleanup();
		(void)fprintf(stderr,
			"password file unchanged.\n");
		return(-1);
	}

	/*
	 * link newly created secure password temporary file
	 * to the secure password file
	 */
        if (link(stmp, spasswd) != 0) {
		error(command, function,
			"link(%s, %s) failed: %s\n",
			stmp, spasswd, _perror());
		(void)fprintf(stderr, "Can't create shadow password file;\n");

		/*
		 * first unlink passwd file in order to recover from
		 * the old passwd file.  just bail rather than clean
		 * up so that there is a chance of manual recovery.
		 */
		if (unlink(passwd) != 0) {
			error(command, function,
				"unlink(%s) failed: %s\n", passwd);
			(void)fprintf(stderr,
				"WARNING: password file recovery FAILED;\n");
			(void)fprintf(stderr,
				"password file saved in %s\n",
				opasswd);
			exit(1);
		}

		/*
		 * if this doesn't work, there is a major problem.
		 */
		if (link(opasswd, passwd) != 0) {
			error(command, function,
				"link(%s, %s) failed: %s\n",
				opasswd, passwd, _perror());
			(void)fprintf(stderr,
				"WARNING: password file recovery FAILED;\n");
			(void)fprintf(stderr,
				"password file saved in %s\n",
				opasswd);
			exit(1);
		}

		/*
		 * if we get here, then recovery succeeded
		 * so it is okay to clean up
		 */
		pcleanup();
		scleanup();
		(void)fprintf(stderr,
			"password file unchanged.\n");
		return(-1);
        }

	/*
	 * toss unneeded files
	 */
	pcleanup();
	scleanup();

	(void)printf("%s installed.\n", spasswd);
	return(0);
}


/*
 * undopswd:
 *	get rid of shadow password file
 */
int
undopswd()
{
	auto	char		tmpbuf[20];
	auto	char *		function = "undopswd";
	auto	struct passwd *	pw = (struct passwd *)0;


	/*
	 * if secure passwd file doesn't exists, just return
	 */
	if (access(spasswd, 0)) {
		(void)fprintf(stderr, "%s doesn't exists.\n", spasswd);
		return(0);
	}

	(void)printf("Restoring %s...\n", passwd);
	(void)fflush(stdout);


	/*
	 * get file pointer for password file
	 */
	if ((passwdfp = getfp(ptmp, 0444)) == (FILE *)0) {
		error(command, function,
			"getfp(%s, 0444) failed\n",
			ptmp);
		pcleanup();
		return(-1);
	}

	/*
	 * toss old password file if it exists
	 */
	if (unlink(opasswd) != 0) {
		/*
		 * something's wrong if errno isn't ENOENT
		 */
		if (errno != ENOENT) {
			error(command, function,
				"unlink(%s) failed: %s\n",
				opasswd, _perror());
			(void)fprintf(stderr,
				"Can't remove old password file.\n");
			pcleanup();
			return(-1);
		}
	}

	/*
	 * hold on to password file
	 */
	if (link(passwd, opasswd) != 0) {
		error(command, function,
			"link(%s, %s) failed: %s\n",
			passwd, opasswd, _perror());
		(void)fprintf(stderr, "Can't backup password file.\n");
		pcleanup();
		return(-1);
	}

	setpwent();

	/*
	 * getpwent() does all the work for us.
	 */
	while((pw = getpwent()) != (struct passwd *)0) {
		if (putpwent(pw, passwdfp) != 0) {
			error(command, function,
				"putpwent(%s) failed\n",
				ptmp);
			(void)fprintf(stderr,
				"Can't write new password file;\n");
			(void)fprintf(stderr,
				"shadow password file unchanged.\n");
			pcleanup();
			return(-1);
		}
	}

	endpwent();

	(void)fclose(passwdfp);

	/*
	 * toss original password file
	 */
	if (unlink(passwd) != 0) {
		error(command, function,
			"unlink(%s) failed: %s\n",
			passwd, _perror());
		(void)fprintf(stderr, "Can't remove old password file;\n");
		(void)fprintf(stderr, "shadow password file unchanged.\n");
		pcleanup();
		return(-1);
        }

	/*
	 * link newly created temporary
	 * password file to password file
	 */
        if (link(ptmp, passwd) != 0) {
		error(command, function,
			"link(%s, %s) failed: %s\n",
			ptmp, passwd, _perror());
		(void)fprintf(stderr, "Can't create new password file;\n");

		/*
		 * if this doesn't work, there is a major problem.
		 * just bail rather than clean up so that we can
		 * recover from /etc/opasswd.
		 */
		if (link(opasswd, passwd) != 0) {
			error(command, function,
				"link(%s, %s) failed: %s\n",
				opasswd, passwd, _perror());
			(void)fprintf(stderr,
				"WARNING: password file recovery FAILED;\n");
			(void)fprintf(stderr,
				"password file saved in %s\n",
				opasswd);
			(void)fprintf(stderr,
				"shadow password file saved in %s\n",
				spasswd);
			exit(1);
		}
		/*
		 * if we get here, then recovery succeeded
		 * so it is okay to clean up
		 */
		pcleanup();
		(void)fprintf(stderr,
			"password file unchanged.\n");
		return(-1);
	}

	/*
	 * toss unneeded files
	 */
	pcleanup();

	/*
	 * toss shadow passwd file
	 */
	if (unlink(spasswd) != 0) {
		error(command, function,
			"unlink(%s) failed: %s\n", spasswd, _perror());
		(void)fprintf(stderr,
			"WARNING: Can't remove shadow password file.\n");
		return(-1);
	}

	(void)printf("%s restored.\n", passwd);
	return(0);
}

/*
 * mkpdirs:
 *	makes secure password file directories and sets up modes.
 */
int
mkpdirs(dir)
char *dir;
{
	auto	char *		function = "mkpdirs";
	auto	struct stat	sbuf;


	if (stat(dir, &sbuf) != 0) {
		if (errno == ENOENT) {
			if (mkdir(dir, 0500) != 0) {
				error(command, function,
					"mkdir(%s) failed: %s\n",
					dir, _perror());
				(void)fprintf(stderr,
					"Can't make directory: %s\n",
					dir);
				return(-1);
			}
		}
		else {
			error(command, function,
				"stat(%s) failed: %s\n",
				dir, _perror());
			(void)fprintf(stderr, "Can't make directory: %s\n",
				dir);
			return(-1);
		}
	}
	else {
		if ((sbuf.st_mode & S_IFDIR) == 0) {
			(void)fprintf(stderr,
				"%s exists and is not a directory.\n",
				dir);
			return(-1);
		}
	}

	/*
	 * fix up modes in the event it was already
	 * there but not set up properly
	 */
	if (chmod(dir, 0500) != 0) {
		error(command, function,
			"chmod(%s, 0500) failed: %s\n",
			dir, _perror());
		(void)fprintf(stderr, "Can't set modes: %s\n", dir);
		return(-1);
	}
}


/*
 * mkcdirs:
 *	makes cron directories and sets up modes.
 */
int
mkcdirs(dir)
char *dir;
{
	auto	char *		function = "mkcdirs";


	if (mkdir(dir, 0) != 0) {
		if (errno != EEXIST) {
			error(command, function,
				"mkdir(%s) failed: %s\n",
				dir, _perror());
			(void)fprintf(stderr,
				"Can't make directory: %s\n", dir);
			return(-1);
		}
	}
	if (chmod(dir, 0500) != 0) {
		error(command, function,
			"chmod(%s, 0500) failed: %s\n",
			dir, _perror());
		(void)fprintf(stderr,
			"Can't set modes: %s\n", dir);
		return(-1);
	}
	return(0);
}


/*
 * scandir:
 *	read through directory and apply the
 * 	supplied function to all of the entries
 */
int
scandir(dir, func)
char *dir;
int (*func)();
{
	auto	char *		function = "scandir";
	auto	struct dirent *	dirent = (struct dirent *)0;
	auto	DIR *		dirp = (DIR *)0;


	if ((dirp = opendir(dir)) == (DIR *)0) {
		error(command, function,
			"opendir(%s) failed: %s\n", _perror());
		(void)fprintf(stderr, "Can't open directory: %s\n", dir);
		return(-1);
	}

	/*
	 * for each file in the directory,
	 * apply the supplied function
	 */
	while ((dirent = readdir(dirp)) != (struct dirent *)0) {
		if (dirent->d_ino == (ino_t)0) {
			continue;
		}
		if (strcmp(dirent->d_name, ".") == 0) {
			continue;
		}
		if (strcmp(dirent->d_name, "..") == 0) {
			continue;
		}
		if ((*func)(dir, dirent->d_name) != 0) {
			/*
			 * don't know the function name so just bail
			 */
			(void)closedir(dirp);
			return(-1);
		}
	}

	(void)closedir(dirp);

	return(0);
}


/*
 * mkataudid:
 *	take the supplied file and add the
 *	associated audit ID file
 */
int
mkataudid(dir, file)
char *dir;
char *file;
{
	auto	char *		function = "mkataudid";
	auto	char		jobname[1024];
	auto	char		aidname[1024];
	auto	struct passwd *	pw = (struct passwd *)0;
	auto	struct stat	sbuf;
	auto	FILE *		wfp = (FILE *)0;
	auto    char 		tmp_dir[MAXDIR];
	auto    char * 		tmp_ptr;


	/*
	 * construct file names
	 */
	(void)sprintf(jobname, "%s/%s", dir, file);

	if (CDF_flag)
	{

	    /* modify the dir name which has come into the function as 
	     * /usr/spool/cron+/cnode/atjob so that it looks like
	     * /usr/spool/cron+/cnode/.ataids/file
	     */

	    strcpy(tmp_dir,dir); /* make a copy of dir to be modified */

	    /* replace last occurence of '/' with a null character */
	    tmp_ptr = strrchr(tmp_dir, '/');
	    *tmp_ptr = '\0';

	    /* make a valid /usr/spool/cron+/cnode/.ataids/filename aidname */
	    (void)sprintf(aidname, "%s/%s/%s", tmp_dir, ".ataids", file);
        }
	else
	    (void)sprintf(aidname, "%s/%s", ATAIDS, file);

	if (stat(jobname, &sbuf) != 0) {
		error(command, function,
			"stat(%s) failed: %s\n", jobname, _perror());
		(void)fprintf(stderr, "Can't stat file: %s\n",
			jobname);
		return(-1);
	}

	/*
	 * get the audit ID based on file ownership
	 */
	if ((pw = getpwuid(sbuf.st_uid)) == (struct passwd *)0) {
		(void)fprintf(stderr, "Can't determine owner of at job: %s\n",
			jobname);
		return(-1);
	}

	/*
	 * forceably toss old file (if it exists)
	 */
	(void)unlink(aidname);

	/*
	 * create audit ID file
	 */
	if ((wfp = getfp(aidname, 0400)) == (FILE *)0) {
		error(command, function,
			"getfp(%s, 0400) failed\n",
			aidname);
		return(-1);
	}
	(void)fprintf(wfp, "%ld\n", pw->pw_audid);
	(void)fclose(wfp);
	return(0);
}


/*
 * mkcronaudid:
 *	take the supplied file and add the
 *	associated audit ID file
 */
int
mkcronaudid(dir, file)
char *dir;
char *file;
{
	auto	char *		function = "mkcronaudid";
	auto	char		aidname[1024];
	auto	int		fd = -1;
	auto	struct passwd *	pw = (struct passwd *)0;
	auto	struct stat	sbuf;
	auto	FILE *		wfp = (FILE *)0;
	auto    char 		tmp_dir[MAXDIR];
	auto    char * 		tmp_ptr;


	/*
	 * construct file name;
	 * jobname is not needed
	 */
	if (CDF_flag)
	{

	    /* modify the dir name which has come into the function as 
	     * /usr/spool/cron+/cnode/crontab so that it looks like
	     * /usr/spool/cron+/cnode/.cronaids/file
	     */

	    strcpy(tmp_dir,dir); /* make a copy of dir to be modified */

	    /* replace last occurence of '/' with a null character */
	    tmp_ptr = strrchr(tmp_dir, '/');
	    *tmp_ptr = '\0';

        /* make a valid /usr/spool/cron+/cnode/.cronaids/filename aidname */
	    (void)sprintf(aidname, "%s/%s/%s", tmp_dir, ".cronaids", file);
        }
	else
	    (void)sprintf(aidname, "%s/%s", CRONAIDS, file);

	/*
	 * get the audit ID based on file name
	 */
	if ((pw = getpwnam(file)) == (struct passwd *)0) {
		(void)fprintf(stderr,
			"Can't determine owner of crontab job: %s\n",
			file);
		return(-1);
	}

	/*
	 * forceably toss old file (if it exists)
	 */
	(void)unlink(aidname);

	/*
	 * create audit ID file
	 */
	if ((wfp = getfp(aidname, 0400)) == (FILE *)0) {
		error(command, function,
			"getfp(%s, 0400) failed\n",
			aidname);
		return(-1);
	}

	(void)fprintf(wfp, "%ld\n", pw->pw_audid);
	(void)fclose(wfp);
	return(0);
}


/*
 * delaudid:
 *	delete file.  we use this front end function
 * 	instead of just passing unlink to scandir so it
 *	consistent with the use of mk{at,cron}audid.
 *	we also use this so that we get error messages.
 */
int
delaudid(dir, file)
char *dir;
char *file;
{
	auto	char *		function = "delaudid";
	auto	char		aidname[1024];


	/*
	 * construct file name
	 */
	(void)sprintf(aidname, "%s/%s", dir, file);

	if (unlink(aidname) != 0) {
		error(command, function,
			"unlink(%s) failed: %s\n", aidname, _perror());
		(void)fprintf(stderr, "Can't remove job: %s\n", aidname);
		return(-1);
	}
	return(0);
}


/*
 * getfp:
 *	return a file pointer to the supplied file.
 *	uses open() so that locking takes place.
 */
FILE *
getfp(file, mode)
char *file;
int mode;
{
	auto	int		fd = -1;
	auto	char *		function = "getfp";
	auto	FILE *		fp = (FILE *)0;


	/*
	 * do an exclusive open of the file
	 */
	if ((fd = open(file, O_CREAT | O_EXCL | O_WRONLY, 0)) < 0) {
		if (errno == EEXIST) {
			/*
			 * we are overloading this function, so make
			 * the message generic but clear enough to
			 * let the luser know what to do next.
			 */
			(void)fprintf(stderr,
				"%s is locked -- try again.\n");
			return((FILE *)0);
		}
		else {
			error(command, function,
				"open(%s) failed: %s\n",
				file, _perror());
			(void)fprintf(stderr,
				"Can't open %s\n", file);
			return((FILE *)0);
		}
	}

	/*
	 * set up proper modes
	 */
	if (fchmod(fd, mode) != 0) {
		error(command, function,
			"fchmod(%s, %o) failed: %s\n",
			file, mode, _perror());
		(void)fprintf(stderr, "Can't set modes: %s\n", file);
		(void)close(fd);
		(void)unlink(file);
		return((FILE *)0);
	}

	/*
	 * get a file pointer from this file descriptor
	 */
	if ((fp = fdopen(fd, "w")) == (FILE *)0) {
		error(command, function,
			"fdopen(%s) failed\n");
		(void)fprintf(stderr,
			"Can't open %s\n", file);
		(void)close(fd);
		(void)unlink(file);
		return((FILE *)0);
	}
	return(fp);
}


/*
 * pcleanup:
 *	toss temp password file
 */
void
pcleanup()
{
	(void)fclose(passwdfp);
	(void)unlink(ptmp);
	(void)unlink(opasswd);
}


/*
 * scleanup:
 *	toss temp shadow password file
 */
void
scleanup()
{
	(void)fclose(spasswdfp);
	(void)unlink(stmp);
}


/*
 * basename:
 *	return last component of path
 */
char *
basename(string)
char *string;
{

	auto	char *		ptr = (char *)0;


	if ((ptr = strrchr(string, '/')) != (char *)0) {
		return(++ptr);
	}
	return(string);
}


/*
 * _perror:
 *	print error string matching errno
 */
char *
_perror()
{
	auto	int		save_errno;

	save_errno = errno;
	errno = 0;
	return(sys_errlist[save_errno]);
}


/*
 * error:
 *	print arguments printf-style if debugging is on
 */
void
error(command, function, format, va_alist)
char *function;
char *format;
va_dcl
{
	va_list ap;

	va_start(ap);
	if (debug == 0) {
		return;
	}
	fflush(stdout);
	fprintf(stderr, "%s: %s: ", command, function);
	_doprnt(format, ap, stderr);
	va_end(ap);
}


/*
 * usage:
 *	print usage message
 */
void
usage(command, arguments)
char *command;
char *arguments;
{
	(void)fprintf(stderr, "usage: %s", command);
	if (arguments != NULL) {
		(void)fprintf(stderr, " %s", arguments);
	}
	(void)fputc('\n', stderr);
	return;
}

/*
 * is_CDF:  Determine if /usr/spool/cron+ exists and if it's a CDF
 */
int
is_CDF()

{

    	auto    char *function = "is_CDF";
	auto    struct stat buf;
 

        if (stat(CRON_CDF,&buf) != 0)
        {
            /* cron+ does not exist */
            if (errno == ENOENT)
                return(0);
            else
                error(command, function, "stat(%s) failed: %s\n",
                      CRON_CDF, _perror());
        }
        else
        {
            if (buf.st_mode & (S_CDF | S_IFDIR))
                return(1); /* cron+ is a CDF */
            else
                return(0);
        }

}



