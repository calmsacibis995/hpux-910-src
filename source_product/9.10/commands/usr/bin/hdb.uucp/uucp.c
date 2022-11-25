/*	@(#) $Revision: 72.3 $	*/
static char *RCS_ID="@(#)$Revision: 72.3 $ $Date: 93/12/17 15:39:47 $";
#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1
#include <nl_types.h>
#endif NLS

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "uucp.h"

/*
 * uucp
 * user id 
 * make a copy in spool directory
 */
int Copy = 0;
char Nuser[32];
char *Ropt = " ";
char Optns[10];
char Uopts[BUFSIZ];
char Grade = 'N';

int	flag	= 0;	/* This flag specified if the user had given the
			   name of the "to system" or not	*/

char	Sfile[MAXFULLNAME];

main(argc, argv, envp)
char	*argv[];
char	**envp;
{
	int	ret;
	int	errors = 0;
	char	*fopt;
	char	sys1[MAXFULLNAME], sys2[MAXFULLNAME];
	char	fwd1[MAXFULLNAME], fwd2[MAXFULLNAME];
	char	file1[MAXFULLNAME], file2[MAXFULLNAME];
	short	jflag = 0;	/* -j flag  Jobid printout */
        char dpath[BUFSIZ]; 
	int err,uucpuid;

	void	split();
#ifndef V7
	long	limit, dummy;
	char	msg[100];
#endif V7



#ifdef NLS
	nlmsg_fd = catopen("uucp",0);
#endif
	/* this fails in some versions, but it doesn't hurt */
	Uid = getuid();
	Euid = geteuid();
	if (Uid == 0){
		err = gninfo("uucp",&uucpuid,dpath);
                if (err == FAIL) uucpuid = UUCPUID;
		(void) setuid(uucpuid);
        }

	/* choose LOGFILE */
	(void) strcpy(Logfile, LOGUUCP);

	Env = envp;
	fopt = NULL;
	(void) strcpy(Progname, "uucp");
	Pchar = 'U';
	*Uopts =  NULLCHAR;

	Optns[0] = '-';
	Optns[1] = 'd';
	Optns[2] = 'c';
	Optns[3] = Nuser[0] = Sfile[0] = NULLCHAR;

	/** find name of local system **/
	
	uucpname(Myname);

	/*
	 * find id of user who spawned command to 
	 * determine
	 */
	(void) guinfo(Uid, User);

	while ((ret = getopt(argc, argv, "Ccdfg:jmn:rs:x:")) != EOF) {
		switch (ret) {

		/*
		 * make a copy of the file in the spool
		 * directory.
		 */
		case 'C':
			Copy = 1;
			Optns[2] = 'C';
			break;

		/*
		 * not used (default)
		 */
		case 'c':
			break;

		/*
		 * not used (default)
		 */
		case 'd':
			break;
		case 'f':
			Optns[1] = 'f';
			break;

		/*
		 * set service grade
		 */
		case 'g':
			Grade = *optarg;
			break;

		case 'j':	/* job id */
			jflag = 1;
			break;

		/*
		 * send notification to local user
		 */
		case 'm':
			(void) strcat(Optns, "m");
			break;

		/*
		 * send notification to user on remote
		 * if no user specified do not send notification
		 */
		case 'n':
			(void) strcat(Optns, "n");
			(void) sprintf(Nuser, "%.8s", optarg);
			(void) sprintf(Uopts+strlen(Uopts), "-n%s ", Nuser);
			break;

		/*
		 * create JCL files but do not start uucico
		 */
		case 'r':
			Ropt = "-r";
			break;

		/*
		 * return status file
		 */
		case 's':
			fopt = optarg;
			/* "m" needed for compatability */
			(void) strcat(Optns, "mo");
			break;

		/*
		 * turn on debugging
		 */
		case 'x':
			Debug = atoi(optarg);
			if (Debug <= 0)
				Debug = 1;
			break;

		default:
			printf((catgets(nlmsg_fd,NL_SETN,794, "unknown flag %s\n")), argv[1]);
			break;
		}
	}
	DEBUG(4, "\n\n** %s **\n", (catgets(nlmsg_fd,NL_SETN,796, "START")));
	gwd(Wrkdir);
	if (fopt) {
		if (*fopt != '/')
			(void) sprintf(Sfile, "%s/%s", Wrkdir, fopt);
		else
			(void) sprintf(Sfile, "%s", fopt);

	}
	/*
	 * work in WORKSPACE directory
	 */
	ret = chdir(WORKSPACE);
	if (ret != 0) {
		(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,797, "No work directory - %s - get help\n")),
		    WORKSPACE);
		cleanup(-2);
	}

	if (Nuser[0] == NULLCHAR)
		(void) strcpy(Nuser, User);
	(void) strcpy(Loginuser, User);
	DEBUG(4, (catgets(nlmsg_fd,NL_SETN,798, "UID %d, ")), Uid);
	DEBUG(4, (catgets(nlmsg_fd,NL_SETN,799, "User %s\n")), User);
	if (argc - optind < 2) {
		(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,800, "usage uucp from ... to\n")));
		cleanup(-3);
	}

	/*
	 * set up "to" system and file names
	 */

	split(argv[argc - 1], sys2, fwd2, file2);
	if (*sys2 != NULLCHAR) {
		flag = 1; /* The user has given the name of second system */
		if (versys(sys2) != 0) {
			(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,801, "bad system: %s\n")), sys2);
			cleanup(-4);
		}
	}

	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,802, "sys2: %s, ")), sys2);
	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,803, "fwd2: %s, ")), fwd2);
	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,804, "file2: %s\n")), file2);

	/*
	 * if there are more than 2 argsc, file2 is a directory
	 */
	if (argc - optind > 2)
		(void) strcat(file2, "/");

	/*
	 * do each from argument
	 */

	for ( ; optind < argc - 1; optind++) {
	    split(argv[optind], sys1, fwd1, file1);
	    if (*sys1 != NULLCHAR) {
		if (versys(sys1) != 0) {
			(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,805, "bad system: %s\n")), sys1);
			cleanup(-4);
		}
		/* Get the name of the remote System	*/
	    	(void) strncpy(Rmtname, sys1, MAXBASENAME);
	    }

	    if (flag)	{
	    	(void) strncpy(Rmtname, sys2, MAXBASENAME);
	    }
	    Rmtname[MAXBASENAME] = NULLCHAR;

	    /*
		Find the name of my system based on the name of Rmtname.
		If there is an alias name for the local system against
		the name of the Remote system in the Permissions file
		then the variable _MyName is initialised to the Alias
		name.
	    */

	    mchFind(Rmtname);

	    /*
		If the Alias name is set, initialize Myname with 
		_MyName else, get it from uucpname.
	    */

	    myName(Myname);

	    if (*sys1 == NULLCHAR)
	    	(void) strncpy(sys1, Myname, MAXBASENAME);
	    if (!flag)
	    	(void) strncpy(sys2, Myname, MAXBASENAME);

	    /*  source files can have at most one ! */
	    if (*fwd1 != NULLCHAR) {
		/* syntax error */
	        (void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,806, "illegal  syntax %s\n")), argv[optind]);
	        exit(1);
	    }

	    /*
	     * check for required remote expansion of file names -- generate
	     *	and execute a uux command
	     * e.g.
	     *		uucp   owl!~/dan/*   ~/dan/
	     *
	     * NOTE: The source file part must be full path name.
	     *  If ~ it will be expanded locally - it assumes the remote
	     *  names are the same.
	     */

	    if ((*sys1 != NULLCHAR) && strcmp(sys1, Myname))
		if ((strchr(file1, '*') != NULL
		      || strchr(file1, '?') != NULL
		      || strchr(file1, '[') != NULL)) {
		        /* do a uux command */
		        if (ckexpf(file1) == FAIL)
			    exit(1);
		        ruux(sys1, sys1, file1, sys2, fwd2, file2);
		        continue;
		}

	    /*
	     * check for forwarding -- generate and execute a uux command
	     * e.g.
	     *		uucp uucp.c raven!owl!~/dan/
	     */

	    if (*fwd2 != NULLCHAR) {
	        ruux(sys2, sys1, file1, "", fwd2, file2);
	        continue;
	    }

	    /*
	     * check for both source and destination on other systems --
	     *  generate and execute a uux command
	     */

	    if (*sys1 != NULLCHAR )
		if ( (!EQUALS(Myname, sys1))
	    	  && *sys2 != NULLCHAR
	    	  && (!EQUALS(sys2, Myname)) ) {
		    ruux(sys2, sys1, file1, "", fwd2, file2);
	            continue;
	        }

	    DEBUG(4, (catgets(nlmsg_fd,NL_SETN,807, "sys1 - %s, ")), sys1);
	    DEBUG(4, (catgets(nlmsg_fd,NL_SETN,808, "file1 - %s, ")), file1);
	    DEBUG(4, (catgets(nlmsg_fd,NL_SETN,809, "Rmtname - %s\n")), Rmtname);
	    DEBUG(9, (catgets(nlmsg_fd,NL_SETN,802, "sys2: %s, ")), sys2);
	    if (copy(sys1, file1, sys2, file2))
	    	errors++;
	}

	/* move the work files to their proper places */
	commitall();

	/*
	 * do not spawn daemon if -r option specified
	 */
	if (*Ropt != '-')
		xuucico(Rmtname);
	if (jflag)
	    printf("%s\n", Jobid);
	cleanup(errors);
}

/*
 * cleanup lock files before exiting
 */
cleanup(code)
register int	code;
{
	rmlock(CNULL);
	if (code != 0)
		wfabort();	/* this may be extreme -- abort all work */
	if (code < 0)
		(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,812, "uucp failed completely: code %d\n")),
		    code);
	else if (code > 0) {
		(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,813, "uucp failed partially: %d error(s)\n")),
		    code);
	}
        
#ifdef NLS
	catclose(nlmsg_fd);
#endif
	exit(code);
}

/*
 * generate copy files for s1!f1 -> s2!f2
 *	Note: only one remote machine, other situations
 *	have been taken care of in main.
 * return:
 *	0	-> success
 *	FAIL	-> failure
 */

copy(s1, f1, s2, f2)
char *s1, *f1, *s2, *f2;
{
	FILE *cfp, *syscfile();
	struct stat stbuf, stbuf1;
	int type, statret;
	char dfile[NAMESIZE];
	char cfile[NAMESIZE];
	char file1[MAXFULLNAME], file2[MAXFULLNAME];
	char msg[BUFSIZ];

	type = 0;
	(void) strcpy(file1, f1);
	(void) strcpy(file2, f2);
	if (!EQUALS(s1, Myname))
		type = 1;
	if (!EQUALS(s2, Myname))
		type = 2;

	switch (type) {
	case 0:

		/*
		 * all work here
		 */
		DEBUG(4, (catgets(nlmsg_fd,NL_SETN,816, "all work here %d\n")), type);

		/*
		 * check access control permissions
		 */
		if (ckexpf(file1))
			 return(FAIL);
		if (ckexpf(file2))
			 return(FAIL);
		if (uidstat(file1, &stbuf) != 0) {
			(void) fprintf(stderr,
			    (catgets(nlmsg_fd,NL_SETN,817, "can't get file status %s\n copy failed\n")), file1);
			return(2);
		}
		statret = uidstat(file2, &stbuf1);
		if (statret == 0
		  && stbuf.st_ino == stbuf1.st_ino
		  && stbuf.st_dev == stbuf1.st_dev) {
			(void) fprintf(stderr,
			    (catgets(nlmsg_fd,NL_SETN,818, "%s %s - same file; can't copy\n")), file1, file2);
			return(3);
		}
		if (chkperm(file1, file2, strchr(Optns, 'd')) ) {
			(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,819, "permission denied\n")));
			cleanup(1);
		}
		if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
			(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,820, "directory name illegal - %s\n")),
			  file1);
			return(4);
		}
		/* see if I can read this file as read uid, gid */
		if ( !(stbuf.st_mode & ANYREAD)
		  && !(stbuf.st_uid == Uid && stbuf.st_mode & 0400)
		  && !(stbuf.st_gid == getgid() && stbuf.st_mode & 0040)
		  ) {
			(void) fprintf(stderr,
			    (catgets(nlmsg_fd,NL_SETN,821, "uucp can't read (%s) mode (%o)\n")),
			    file1, stbuf.st_mode);
			(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,822, "use cp command\n")));
			return(5);
		}
		if (statret == 0 && (stbuf1.st_mode & ANYWRITE) == 0) {
			(void) fprintf(stderr,
			    (catgets(nlmsg_fd,NL_SETN,823, "can't write file (%s) mode (%o)\n")),
			    file2, stbuf1.st_mode);
			return(6);
		}

		/*
		 * copy file locally
		 */
		DEBUG(2, (catgets(nlmsg_fd,NL_SETN,824, "local copy:  uidxcp(%s, ")), file1);
		DEBUG(2, (catgets(nlmsg_fd,NL_SETN,825, "%s\n")), file2);

		/* do copy as Uid, but the file will be owned by uucp */
		if (uidxcp(file1, file2) == FAIL) {
			(void) fprintf(stderr,
			    (catgets(nlmsg_fd,NL_SETN,826, "can't copy file (%s) errno %d\n")), file2, errno);
			return(7);
		}
		/* Filter out UID and GID and enable read/write */
		(void) chmod(file2, (int) ((stbuf.st_mode | 0666) & 0777));
		return(0);
	case 1:

		/*
		 * receive file
		 */
		DEBUG(4, (catgets(nlmsg_fd,NL_SETN,827, "receive file - %d\n")), type);

		/*
		 * expand source and destination file names
		 * and check access permissions
		 */
		if (file1[0] != '~')
			if (ckexpf(file1))
				 return(7);
		if (ckexpf(file2))
			 return(8);

		/*
		 * insert JCL card in file
		 */
		cfp = syscfile(cfile, s1);
		(void) fprintf(cfp, "R %s %s %s %s %s %o %s\n", file1, file2,
			User, Optns,
			*Sfile ? Sfile : "dummy",
			0777, Nuser);
		(void) fclose(cfp);
		(void) sprintf(msg, "s!%s --> %s!%s", Rmtname, file1,
		    Myname, file2);
		logent(msg, "QUEUED");
		break;
	case 2:

		/*
		 * send file
		 */
		if (ckexpf(file1))
			 return(10);
		/* XQTDIR hook enables 3rd party uux requests (cough) */
		if (file2[0] != '~' && !EQUALS(Wrkdir, XQTDIR))
			if (ckexpf(file2))
				 return(11);
		DEBUG(4, (catgets(nlmsg_fd,NL_SETN,831, "send file - %d\n")), type);

		if (uidstat(file1, &stbuf) != 0) {
			(void) fprintf(stderr,
			    (catgets(nlmsg_fd,NL_SETN,832, "can't get status for file %s\n")), file1);
			return(13);
		}
		if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
			(void) fprintf(stderr,
			    (catgets(nlmsg_fd,NL_SETN,833, "directory name illegal - %s\n")), file1);
			return(14);
		}

		/*
		 * make a copy of file in spool directory
		 */
		if (Copy || !READANY(file1) ) {
			gename(DATAPRE, s2, Grade, dfile);

			if (uidxcp(file1, dfile))
			    return(16);

			(void) chmod(dfile, DFILEMODE);
			wfcommit(dfile, dfile, s2);
		} else {

			/*
			 * make a dummy D. name
			 * cntrl.c knows names < 6 chars are dummy D. files
			 */
			(void) strcpy(dfile, "D.0");
		}

		/*
		 * insert JCL card in file
		 */
		cfp = syscfile(cfile, s2);
		(void) fprintf(cfp, "S  %s %s %s %s %s %o %s %s\n",
		    file1, file2, User, Optns, dfile,
		    stbuf.st_mode & 0777, Nuser, Sfile);
		(void) fclose(cfp);
		(void) sprintf(msg, "%s!%s --> %s!%s", Myname, file1,
		    Rmtname, file2);
		logent(msg, "QUEUED");
		break;
	}
	return(0);
}


/*
 *	syscfile(file, sys)
 *	char	*file, *sys;
 *
 *	get the cfile for system sys (creat if need be)
 *	return stream pointer
 *
 *	returns
 *		stream pointer to open cfile
 *		
 */

static FILE	*
syscfile(file, sys)
char	*file, *sys;
{
	FILE	*cfp;

	if (gtcfile(file, sys) == FAIL) {
		gename(CMDPRE, sys, Grade, file);
		ASSERT(access(file, 0) != 0, Fl_EXISTS, file, errno);
		cfp = fdopen(creat(file, CFILEMODE), "w");
		svcfile(file, sys);
	} else
		cfp = fopen(file, "a");
	/*  set Jobid -- C.jobid */
	(void) strncpy(Jobid, BASENAME(file, '.'), NAMESIZE);
	Jobid[NAMESIZE-1] = '\0';
	ASSERT(cfp != NULL, Ct_OPEN, file, errno);
	return(cfp);
}


/*
 * split - split the name into parts:
 * sys - system name
 * fwd - intermediate destinations
 * file - file part
 */

static
void
split(arg, sys, fwd, file)
char *arg, *sys, *fwd, *file;
{
    register char *cl, *cr, *n;
    *sys = *fwd = *file = NULLCHAR;
	
    for (n=arg ;; n=cl+1) {
	cl = strchr(n, '!');
	if ( cl == NULL) {
	    /* no ! in n */
	    (void) strcpy(file, n);
	    return;
	}

	/*if (PREFIX(Myname, n)) Bug fixes from AT&T release notes */
	if (EQUALS(Myname, n))
	    continue;

	cr = strrchr(n, '!');
	(void) strcpy(file, cr+1);
	(void) strncpy(sys, n, cl-n);
	sys[cl-n] = NULLCHAR;

	if (cr != cl) {
	    /*  more than one ! */
	    (void) strncpy(fwd, cl+1, cr-cl-1);
	    fwd[cr-cl-1] = NULLCHAR;
	}
	return;
    }
    /*NOTREACHED*/
}


/*
 * generate and execute a uux command
 */

ruux(rmt, sys1, file1, sys2, fwd2, file2)
char *rmt, *sys1, *file1, *sys2, *fwd2, *file2;
{
    char cmd[BUFSIZ];

    if (*Uopts != NULLCHAR)
	(void) sprintf(cmd, "/usr/bin/uux -C %s %s!uucp -C \\(%s\\) ", Ropt, rmt, Uopts);
    else
	(void) sprintf(cmd, "/usr/bin/uux -C %s %s!uucp -C ", Ropt, rmt);

    if (*sys1 == NULLCHAR || EQUALS(sys1, Myname)) {
        if (ckexpf(file1))
  	    exit(1);
	/* 
	 * BUG fix: DTS: OSSsr00127  SR# 5003144824
	 */
	(void) sprintf(cmd+strlen(cmd), " !%s ", file1);
    }
    else
	if (!EQUALS(rmt, sys1))
	    (void) sprintf(cmd+strlen(cmd), " \\(%s!%s\\) ", sys1, file1);
	else
	    (void) sprintf(cmd+strlen(cmd), " \\(%s\\) ", file1);

    if (*fwd2 != NULLCHAR) {
	if (*sys2 != NULLCHAR)
	    (void) sprintf(cmd+strlen(cmd),
		" \\(%s!%s!%s\\) ", sys2, fwd2, file2);
	else
	    (void) sprintf(cmd+strlen(cmd), " \\(%s!%s\\) ", fwd2, file2);
    }
    else {
	if (*sys2 == NULLCHAR || EQUALS(sys2, Myname))
	    if (ckexpf(file2))
		exit(1);
	(void) sprintf(cmd+strlen(cmd), " \\(%s!%s\\) ", sys2, file2);
    }

    DEBUG(2, (catgets(nlmsg_fd,NL_SETN,837, "cmd: %s\n")), cmd);
    logent(cmd, "QUEUED");
    system(cmd);
    return;
}
