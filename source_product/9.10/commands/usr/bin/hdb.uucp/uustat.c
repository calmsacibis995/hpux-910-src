/*	@(#) $Revision: 70.4 $	*/
static char *RCS_ID="@(#)$Revision: 70.4 $ $Date: 93/12/17 17:39:31 $";
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

#ifdef	V7
#define O_RDONLY	0
#endif

#define USAGE1	strcpy(msg3,(catgets(nlmsg_fd,NL_SETN,919, "[-a] [-q] or [-m] or [-kJOB] or [-rJOB] or [-p]")))
#define USAGE2	strcpy(msg3,(catgets(nlmsg_fd,NL_SETN,920, "[-sSYSTEM] [-uUSER]")))

#define STST_MAX	132
struct m {
	char	mach[15];		/* machine name */
	char	locked;
	int	ccount, xcount;
	int	count, type;
	long	retrytime;
	time_t lasttime;
	short	c_age;			/* age of oldest C. file */
	short	x_age;			/* age of oldest X. file */
	char	stst[STST_MAX];
} *M ; /* was M[UUSTAT_TBL+2] til 70.3 - now dynamically sized */
unsigned int v_uustat_tbl ;   /* dimension for M[v_uustat_tbl+2] */

extern long atol();
extern void qsort();		/* qsort(3) and comparison test */
int sortcnt = -1;
extern int machcmp();
extern int _age();		/* find the age of a file */

extern char Jobid[];	/* jobid for status or kill option */
short Kill;		/*  == 1 if -k specified */
short Rejuvenate;	/*  == 1 for -r specified */
short Uopt;		/*  == 1 if -u option specified */
short Sysopt;		/*  == 1 if -s option specified */
short Summary;		/*  == 1 if -q or -m is specified */
short Queue;		/*  == 1 if -q option set - queue summary */
short Machines;		/*  == 1 if -m option set - machines summary */
short Psopt;		/*  == 1 if -p option set - output "ps" of LCK pids */

main(argc, argv, envp)
char *argv[];
char **envp;
{
	struct m *m, *machine();
	DIR *spooldir, *subdir;
	char *str, *rindex();
	char f[256], subf[256];
	char *c, lckdir[BUFSIZ];
	char buf[BUFSIZ];
	char *vec[7];
	int i;

        /* Dynamic UUSTAT_TBL size determination |
	|  and allocation. Worst case cumulate   |
	|  + .Status # of files                  |
	|  + LCK..<system> # of files            |
	|  + spool/<system> # of directories     |
	|  + 2 (overflow area)                  */
	v_uustat_tbl = 0 ;
	/* count .Status entries */
            if (chdir(STATDIR) || (spooldir = opendir(STATDIR)) == NULL)
                exit(101);              /* good old code 101 */
            while (gnamef(spooldir, f) == TRUE) {
                if (freopen(f, "r", stdin) == NULL)
                        continue;
                v_uustat_tbl++;
            }
            closedir(spooldir);
	/* count LCK entries     */
            (void) strcpy(lckdir, LOCKPRE);
            *strrchr(lckdir, '/') = '\0';
            /* open lock directory */
            if (chdir(lckdir) != 0 || (subdir = opendir(lckdir)) == NULL)
                exit(101);              /* good old code 101 */
            while (gnamef(subdir, f) == TRUE) {
                if (EQUALSN("LCK..", f, 5)) {
                    if (!EQUALSN(f + 5, "cul", 3)
                     && !EQUALSN(f + 5, "tty", 3)
                     && !EQUALSN(f + 5, "dtsw", 4)
                     && !EQUALSN(f + 5, "vadic", 5)
                     && !EQUALSN(f + 5, "micom", 5))
                        v_uustat_tbl++;
                }
            }
	/* count spool entries   */
        if (chdir(SPOOL) != 0 || (spooldir = opendir(SPOOL)) == NULL)
                exit(101);              /* good old code 101 */
        while (gnamef(spooldir, f) == TRUE) {
            if (EQUALSN("LCK..", f, 5))
                continue;
            if (DIRECTORY(f) && (subdir = opendir(f))) {
                v_uustat_tbl++;
                closedir(subdir);
            }
        }
	M = (struct m *) calloc(2 + v_uustat_tbl, sizeof(struct m));
	if ( M == NULL) { /* malloc failed */
		errent("MACHINE TABLE FULL", "", v_uustat_tbl,
		sccsid, __FILE__, __LINE__);
		(void) fprintf(stderr,
		    (catgets(nlmsg_fd,
			NL_SETN,
			987,
			"WARNING: Table Overflow--output not complete\n"
			)));
		exit(-1);
	}

	User[0] = '\0';
	Rmtname[0] = '\0';
	Jobid[0] = '\0';
	Psopt=Machines=Summary=Queue=Kill=Rejuvenate=Uopt=Sysopt=0;
	(void) strcpy(Progname, "uustat");
      
#ifdef NLS
	nlmsg_fd = catopen("uucp",0);
#endif
	Uid = getuid();
	Euid = geteuid();
	guinfo(Uid, Loginuser);
	uucpname(Myname);
	while ((i = getopt(argc, argv, "ak:mpr:qs:u:x:")) != EOF) {
		switch(i){
		case 'a':
			Sysopt = 1;
			break;
		case 'k':
			(void) strncpy(Jobid, optarg, NAMESIZE);
			Jobid[NAMESIZE] = '\0';
			Kill = 1;
			break;
		case 'm':
			Machines = Summary = 1;
			break;
		case 'p':
			Psopt = 1;
			break;
		case 'r':
			(void) strncpy(Jobid, optarg, NAMESIZE);
			Jobid[NAMESIZE] = '\0';
			Rejuvenate = 1;
			break;
		case 'q':
			Queue = Summary = 1;
			break;
		case 's':
			(void) strncpy(Rmtname, optarg, MAXBASENAME);
			Rmtname[MAXBASENAME] = '\0';
			Sysopt = 1;
			break;
		case 'u':
			(void) strncpy(User, optarg, 8);
			User[8] = '\0';
			Uopt = 1;
			break;
		case 'x':
			Debug = atoi(optarg);
			if (Debug <= 0)
				Debug = 1;
			break;
		default:
			(void) fprintf(stderr, strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,921, "\tusage: %s %s\n"))),
			    Progname, USAGE1);
			(void) fprintf(stderr, strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,922, "or\n\tusage: %s %s\n"))),
			    Progname, USAGE2);
			exit(1);
		}
	}

	if (argc != optind) {
		(void) fprintf(stderr, (strcpy(msg1,catgets(nlmsg_fd,NL_SETN,923, "\tusage: %s %s\n"))), Progname, USAGE1);
		(void) fprintf(stderr, strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,924, "or\n\tusage: %s %s\n"))),
		    Progname, USAGE2);
		exit(1);
	}

	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,925, "Progname (%s): STARTED\n")), Progname);
	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,926, "User=%s, ")), User);
	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,927, "Loginuser=%s, ")), Loginuser);
	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,928, "Jobid=%s, ")), Jobid);
	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,929, "Rmtname=%s\n")), Rmtname);

	if ((Psopt + Machines + Queue + Kill + Rejuvenate + (Uopt|Sysopt)) >1) {
		/* only -u and -s can be used together */
		printf(strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,930, "\tusage: %s %s\n"))), Progname, USAGE1);
		printf(strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,931, "or\n\tusage: %s %s\n"))), Progname, USAGE2);
		exit(1);
	}
	if (  !(Kill | Rejuvenate | Uopt | Sysopt | Queue | Machines) ) {
		(void) strcpy(User, Loginuser);
		Uopt = 1;
	}

	if (Psopt) {
		/* do "ps -flp" or pids in LCK files */
		lckpid();
		/* lckpid will not return */
	}

	if (Summary) {
	    /*   Gather data for Summary option report  */
	    if (chdir(STATDIR) || (spooldir = opendir(STATDIR)) == NULL)
		exit(101);		/* good old code 101 */
	    while (gnamef(spooldir, f) == TRUE) {
		if (freopen(f, "r", stdin) == NULL)
			continue;
		m = machine(f);
		if (gets(buf) == NULL)
			continue;
		if (getargs(buf, vec, 5) < 5)
			continue;
		m->type = atoi(vec[0]);
		m->count = atoi(vec[1]);
		m->lasttime = atol(vec[2]);
		m->retrytime = atol(vec[3]);
		(void) strncpy(m->stst, vec[4], STST_MAX);
		str = rindex(m->stst, ' ');
		(void) machine(++str);	/* longer name? */
		*str = '\0';
			
	    }
	    closedir(spooldir);
	}


	if (Summary) {
	    /*  search for LCK machines  */

	    (void) strcpy(lckdir, LOCKPRE);
	    *strrchr(lckdir, '/') = '\0';
	    /* open lock directory */
	    if (chdir(lckdir) != 0 || (subdir = opendir(lckdir)) == NULL)
		exit(101);		/* good old code 101 */

	    while (gnamef(subdir, f) == TRUE) {
		if (EQUALSN("LCK..", f, 5)) {
		    if (!EQUALSN(f + 5, "cul", 3)
		     && !EQUALSN(f + 5, "tty", 3)
		     && !EQUALSN(f + 5, "dtsw", 4)
		     && !EQUALSN(f + 5, "vadic", 5)
		     && !EQUALSN(f + 5, "micom", 5))
			machine(f + 5)->locked++;
		}
	    }
	}

	if (chdir(SPOOL) != 0 || (spooldir = opendir(SPOOL)) == NULL)
		exit(101);		/* good old code 101 */
	while (gnamef(spooldir, f) == TRUE) {
	    if (EQUALSN("LCK..", f, 5))
		continue;

	    if (*Rmtname && !EQUALSN(Rmtname, f, SYSNSIZE))
		continue;

	    if ( (Kill || Rejuvenate)
	      && (!EQUALSN(f, Jobid, strlen(Jobid)-5)) )
		    continue;

	    if (DIRECTORY(f) && (subdir = opendir(f))) {
		m = machine(f);
	        while (gnamef(subdir, subf) == TRUE)
		    if (subf[1] == '.') {
		        if (subf[0] == CMDPRE) {
				m->ccount++;
				if (Kill || Rejuvenate)
				    kprocessC(f, subf);
				else if (Uopt | Sysopt)
				    uprocessC(f, subf);
				else 	/* get the age of the C. file */
				    if ( (i = _age(f, subf))>m->c_age)
					m->c_age = i;
			}

			else if (subf[0] == XQTPRE) {
			        m->xcount++;
				if ( (i = _age(f, subf)) > m->x_age)
					m->x_age = i;
			}

		    }
		closedir(subdir);
	    }
	}
	/* for Kill or Rejuvenate - will not get here unless it failed */
	if (Kill || Rejuvenate) {
	    printf(strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,932, "Can't find Job %s; Not %s\n"))), Jobid,
		Kill ? (strcpy(msg2,(catgets(nlmsg_fd,NL_SETN,933, "killed")))) : (strcpy(msg3,(catgets(nlmsg_fd,NL_SETN,934, "rejuvenated")))));
	    exit(1);
	}

	/* Make sure the overflow entry is null since it may be incorrect */
	M[v_uustat_tbl].mach[0] = NULLCHAR;
	if (Summary) {
	    for((sortcnt = 0, m = &M[0]);*(m->mach) != NULL;(sortcnt++,m++))
			;
	    qsort((char *)M, (unsigned int)sortcnt, sizeof(struct m), machcmp);
	    for (m = M; m->mach[0] != NULLCHAR; m++)
		printit(m);
	}
#ifdef NLS
	catclose(nlmsg_fd);
#endif
	exit(0);
}


/*
 * uprocessC - get information about C. file
 *
 */

uprocessC(dir, file)
char *file, *dir;
{
	struct stat s;
	register struct tm *tp;
	char fullname[MAXFULLNAME], buf[BUFSIZ], user[9];
	char xfullname[MAXFULLNAME];
	char file1[BUFSIZ], file2[BUFSIZ], file3[BUFSIZ], type[2], opt[256];
	FILE *fp, *xfp;
	short first = 1;
	extern long fsize();

	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,935, "uprocessC(%s, ")), dir);
	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,936, "%s);\n")), file);

	if (Jobid[0] != '\0' && (!EQUALS(Jobid, &file[2])) ) {
		/* kill job - not this one */
		return;
	}

	(void) sprintf(fullname, "%s/%s", dir, file);
	if (stat(fullname, &s) != 0) {
	     /* error - can't stat */
	    DEBUG(4, (catgets(nlmsg_fd,NL_SETN,937, "Can't stat file (%s),")), fullname);
	    DEBUG(4, (catgets(nlmsg_fd,NL_SETN,938, " errno (%d) -- skip it!\n")), errno);
	}

	fp = fopen(fullname, "r");
	if (fp == NULL) {
		DEBUG(4, (catgets(nlmsg_fd,NL_SETN,939, "Can't open file (%s), ")), fullname);
		DEBUG(4, (catgets(nlmsg_fd,NL_SETN,940, "errno=%d -- skip it!\n")), errno);
		return;
	}
	tp = localtime(&s.st_mtime);

	if (s.st_size == 0 && User[0] == '\0') { /* dummy D. for polling */
	    printf((catgets(nlmsg_fd,NL_SETN,941, "%-12s  %2.2d/%2.2d-%2.2d:%2.2d:%2.2d  (POLL)\n")),
		&file[2], tp->tm_mon + 1, tp->tm_mday, tp->tm_hour,
		tp->tm_min, tp->tm_sec);
	}
	else while (fgets(buf, BUFSIZ, fp) != NULL) {
	    if (sscanf(buf,"%s%s%s%s%s%s", type, file1, file2,
	      user, opt, file3) <5) {
		DEBUG(4, (catgets(nlmsg_fd,NL_SETN,942, "short line (%s)\n")), buf);
		continue;
	    }
	    DEBUG(9, (catgets(nlmsg_fd,NL_SETN,943, "type (%s), ")), type);
	    DEBUG(9, (catgets(nlmsg_fd,NL_SETN,944, "file1 (%s)")), file1);
	    DEBUG(9, (catgets(nlmsg_fd,NL_SETN,945, "file2 (%s)")), file2);
	    DEBUG(9, (catgets(nlmsg_fd,NL_SETN,946, "file3 (%s)")), file3);
	    DEBUG(9, (catgets(nlmsg_fd,NL_SETN,947, "user (%s)")), user);

	    if (User[0] != '\0' && (!EQUALS(User, user)) )
		continue;

/*
	    if ( (*file2 != 'X')
	      && (*file1 != '/' && *file1 != '~')
	      && (*file2 != '/' && *file2!= '~') )
		continue;
*/

	    if (first)
	        printf((catgets(nlmsg_fd,NL_SETN,948, "%-12s  %2.2d/%2.2d-%2.2d:%2.2d  ")),
		    &file[2], tp->tm_mon + 1, tp->tm_mday, tp->tm_hour,
		    tp->tm_min);
	    else
		printf((catgets(nlmsg_fd,NL_SETN,949, "%-12s  %2.2d/%2.2d-%2.2d:%2.2d  ")),
		    "", tp->tm_mon + 1, tp->tm_mday, tp->tm_hour,
		    tp->tm_min);
	    first = 0;

	    printf("%s  %s  ", type, dir);
	    if (*type == 'R')
	        printf("%s  %s\n", user, file1);
	    else if (file2[0] != 'X')
		printf("%s %ld %s\n", user, fsize(dir, file3, file1), file1);
	    else if (*type == 'S' && file2[0] == 'X') {
		(void) sprintf(xfullname, "%s/%s", dir, file1);
		xfp = fopen(xfullname, "r");
		if (xfp == NULL) { /* program error */
		    DEBUG(4, (catgets(nlmsg_fd,NL_SETN,950, "Can't read %s, ")), xfullname);
		    DEBUG(4, (catgets(nlmsg_fd,NL_SETN,951, "errno=%d -- skip it!\n")), errno);
		    printf("%s  %s  %s  ", type, dir, user);
		    printf((catgets(nlmsg_fd,NL_SETN,952, "????\n")));
		}
		else {
		    char command[BUFSIZ], uline_u[BUFSIZ], uline_m[BUFSIZ];
		    char retaddr[BUFSIZ], *username;

		    *retaddr = *uline_u = *uline_m = '\0';
		    while (fgets(buf, BUFSIZ, xfp) != NULL) {
			switch(buf[0]) {
			case 'C':
				strcpy(command, buf + 2);
				break;
			case 'U':
				sscanf(buf + 2, "%s%s", uline_u, uline_m);
				break;
			case 'R':
				sscanf(buf+2, "%s", retaddr);
				break;
			}
		    }
		    username = user;
		    if (*uline_u != '\0')
			    username = uline_u;
		    if (*retaddr != '\0')
			username = retaddr;
		    if (!EQUALS(uline_m, Myname))
			printf("%s!", uline_m);
		    printf("%s  %s", username, command);
		}
		if (xfp != NULL)
		    fclose(xfp);
	    }
	}

	fclose(fp);
	return;
}


/*
 * kprocessC - process kill or rejuvenate job
 */

kprocessC(dir, file)
char *file, *dir;
{
	struct stat s;
	register struct tm *tp;
	extern struct tm *localtime();
	extern int errno;
	char fullname[MAXFULLNAME], buf[BUFSIZ], user[9];
	char rfullname[MAXFULLNAME];
	char file1[BUFSIZ], file2[BUFSIZ], file3[BUFSIZ], type[2], opt[256];
	FILE *fp, *xfp;
 	time_t times[2];
	short ret;
	short first = 1;

	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,953, "kprocessC(%s, ")), dir);
	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,954, "%s);\n")), file);

	if ((!EQUALS(Jobid, &file[2])) ) {
		/* kill job - not this one */
		return;
	}

	(void) sprintf(fullname, "%s/%s", dir, file);
	if (stat(fullname, &s) != 0) {
	     /* error - can't stat */
	    fprintf(stderr, strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,955, "Can't stat:%s, errno (%d)--can't %s it!\n"))),
		fullname, errno, Kill ? strcpy(msg2,(catgets(nlmsg_fd,NL_SETN,956, "kill"))) : strcpy(msg3,(catgets(nlmsg_fd,NL_SETN,957, "rejuvenate"))));
	    exit(1);
	}

	fp = fopen(fullname, "r");
	if (fp == NULL) {
	    fprintf(stderr, strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,958, "Can't read:%s, errno (%d)--can't %s it!\n"))),
		fullname, errno, Kill ? strcpy(msg2,(catgets(nlmsg_fd,NL_SETN,959, "kill"))) : strcpy(msg3,(catgets(nlmsg_fd,NL_SETN,960, "rejuvenate"))));
	    exit(1);
	}

 	times[0] = times[1] = time((time_t *)NULL);
 
	while (fgets(buf, BUFSIZ, fp) != NULL) {
	    if (sscanf(buf,"%s%s%s%s%s%s", type, file1, file2,
	      user, opt, file3) <6) {
		fprintf(stderr, strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,961, "Bad format:%s, errno (%d)--can't %s it!\n"))),
		    fullname, errno, Kill ? strcpy(msg2,(catgets(nlmsg_fd,NL_SETN,962, "kill"))) : strcpy(msg3,(catgets(nlmsg_fd,NL_SETN,963, "rejuvenate"))));
	        exit(1);
	    }
	    DEBUG(9, (catgets(nlmsg_fd,NL_SETN,964, "type (%s), ")), type);
	    DEBUG(9, (catgets(nlmsg_fd,NL_SETN,965, "file1 (%s)")), file1);
	    DEBUG(9, (catgets(nlmsg_fd,NL_SETN,966, "file2 (%s)")), file2);
	    DEBUG(9, (catgets(nlmsg_fd,NL_SETN,967, "file3 (%s)")), file3);
	    DEBUG(9, (catgets(nlmsg_fd,NL_SETN,968, "user (%s)")), user);


	    if (first) {
	        if (Uid != 0
		    && !PREFIX(Loginuser, user)
		    && !PREFIX(user, Loginuser) ) {
			/* not allowed - not owner or root */
			fprintf(stderr,
			    strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,969, "Not owner or root - can't %s job %s\n"))),
    			    Kill ? strcpy(msg2,(catgets(nlmsg_fd,NL_SETN,970, "kill"))) : strcpy(msg3,(catgets(nlmsg_fd,NL_SETN,971, "rejuvenate"))), Jobid);
		    exit(1);
		}
		first = 0;
	    }

	    /* remove D. file */
	    (void) sprintf(rfullname, "%s/%s", dir, file3);
	    DEBUG(4, (catgets(nlmsg_fd,NL_SETN,972, "Remove %s\n")), rfullname);
	    if (Kill)
		ret = unlink(rfullname);
	    else /* Rejuvenate */
 		ret = utime(rfullname, times);
	    if (ret != 0 && errno != ENOENT) {
		/* program error?? */
		fprintf(stderr, strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,973, "Error: Can't %s, File ( %s), errno (%d)\n"))),
		    Kill ? strcpy(msg2,(catgets(nlmsg_fd,NL_SETN,974, "kill"))) : strcpy(msg3,(catgets(nlmsg_fd,NL_SETN,975, "rejuvenate"))), rfullname, errno);
		exit(1);
	    }
	}

	DEBUG(4, (catgets(nlmsg_fd,NL_SETN,976, "Remove %s\n")), fullname);
	if (Kill)
	    ret = unlink(fullname);
	else /* Rejuvenate */
		ret = utime(fullname, times);
	
	if (ret != 0) {
	    /* program error?? */
		fprintf(stderr, strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,977, "Error: Can't %s, File ( %s), errno (%d)\n"))),
		    Kill ? strcpy(msg2,(catgets(nlmsg_fd,NL_SETN,978, "kill"))) : strcpy(msg3,(catgets(nlmsg_fd,NL_SETN,979, "rejuvenate"))), fullname, errno);
	    exit(1);
	}
	fclose(fp);
	printf(strcpy(msg1,(catgets(nlmsg_fd,NL_SETN,980, "Job: %s successfully %s\n"))), Jobid,
	    Kill ? strcpy(msg2,(catgets(nlmsg_fd,NL_SETN,981, "killed"))) : strcpy(msg3,(catgets(nlmsg_fd,NL_SETN,982, "rejuvenated"))));
	exit(0);
}

/*
 * fsize - return the size of f1 or f2 (if f1 does not exist)
 *	f1 is the local name
 *
 */

long
fsize(dir, f1, f2)
char *dir, *f1, *f2;
{
	struct stat s;
	char fullname[BUFSIZ];

	(void) sprintf(fullname, "%s/%s", dir, f1);
	if (stat(fullname, &s) == 0) {
	    return(s.st_size);
	}
	if (stat(f2, &s) == 0) {
	    return(s.st_size);
	}

	return(-99999);
}

cleanup(){}
void logent(){}		/* to load ulockf.c */
void systat(){}		/* to load utility.c */

struct m	*
machine(name)
char	*name;
{
	struct m *m;
	int	namelen;

	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,983, "machine(%s), ")), name);
	namelen = strlen(name);
	for (m = M; m->mach[0] != NULLCHAR; m++)
		/* match on overlap? */
		if (EQUALSN(name, m->mach, SYSNSIZE)) {
			/* use longest name */
			if (namelen > strlen(m->mach))
				(void) strcpy(m->mach, name);
			return(m);
		}

	/*
	 * The table is set up with 2 extra entries
	 * When we go over by one, output error to errors log
	 * When more than one over, just reuse the previous entry
	 */
	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,984, "m-M=%d\n")), m-M);
	if (m-M >= v_uustat_tbl) {
	    if (m-M == v_uustat_tbl) {
		errent("MACHINE TABLE FULL", "", v_uustat_tbl,
		sccsid, __FILE__, __LINE__);
		(void) fprintf(stderr,
		    (catgets(nlmsg_fd,NL_SETN,987, "WARNING: Table Overflow--output not complete\n")));
	    }
	    else
		/* use the last entry - overwrite it */
		m = &M[v_uustat_tbl];
	}

	(void) strcpy(m->mach, name);
	m->c_age= m->x_age= m->lasttime= m->locked= m->ccount= m->xcount= 0;
	m->stst[0] = '\0';
	return(m);
}

printit(m)
struct m *m;
{
	register struct tm *tp;
	time_t	t;
	int	min;
	extern struct tm *localtime();

	if (m->ccount == 0
	 && m->xcount == 0
	 /*&& m->stst[0] == '\0'*/
	 && m->locked == 0
	 && Queue
	 && m->type == 0)
		return;
	printf("%-10s", m->mach);
	if (m->ccount)
		printf((catgets(nlmsg_fd,NL_SETN,988, "%3dC")), m->ccount);
	else
		printf("    ");
	if (m->c_age)
		printf("(%d)", m->c_age);
	else
		printf("   ");
	if (m->xcount)
		printf((catgets(nlmsg_fd,NL_SETN,989, "%4dX")), m->xcount);
	else
		printf("     ");
	if (m->x_age)
		printf("(%d) ", m->x_age);
	else
		printf("    ");

	if (m->lasttime) {
	    tp = localtime(&m->lasttime);
	    printf((catgets(nlmsg_fd,NL_SETN,990, "%2.2d/%2.2d-%2.2d:%2.2d ")),
		tp->tm_mon + 1, tp->tm_mday, tp->tm_hour,
		tp->tm_min);
	}
/*	if (m->locked && m->type != SS_INPROGRESS) */
	if (m->locked)
		printf((catgets(nlmsg_fd,NL_SETN,991, "Locked ")));
	if (m->stst[0] != '\0') {
		printf("%s", m->stst);
		switch (m->type) {
		case SS_SEQBAD:
		case SS_LOGIN_FAILED:
		case SS_DIAL_FAILED:
		case SS_BAD_LOG_MCH:
		case SS_BADSYSTEM:
		case SS_CANT_ACCESS_DEVICE:
		case SS_DEVICE_FAILED:
		case SS_WRONG_MCH:
		case SS_RLOCKED:
		case SS_RUNKNOWN:
		case SS_RLOGIN:
		case SS_UNKNOWN_RESPONSE:
		case SS_STARTUP:
		case SS_CHAT_FAILED:
			(void) time(&t);
			t = m->retrytime - (t - m->lasttime);
			if (t > 0) {
				min = (t + 59) / 60;
				printf((catgets(nlmsg_fd,NL_SETN,992, "Retry: %d:%2.2d")), min/60, min%60);
			}
			if (m->count > 1)
				printf((catgets(nlmsg_fd,NL_SETN,993, " Count: %d")), m->count);
		}
	}
	putchar('\n');
}

#define MAXLOCKS 100	/* Maximum number of lock files this will handle */

lckpid()
{
    register i;
    long pid, ret, fd;
#ifdef ASCIILOCKS
    char alpid[SIZEOFPID+2];	/* +2 for '\n' and null */
#endif
    long list[MAXLOCKS];
    char buf[BUFSIZ], f[MAXBASENAME];
    char *c, lckdir[BUFSIZ];
    DIR *dir;

    DEBUG(9, (catgets(nlmsg_fd,NL_SETN,994, "lckpid() - entered\n")), "");
    for (i=0; i<MAXLOCKS; i++)
	list[i] = -1;
    (void) strcpy(lckdir, LOCKPRE);
    *strrchr(lckdir, '/') = '\0';
    DEBUG(9, (catgets(nlmsg_fd,NL_SETN,995, "lockdir (%s)\n")), lckdir);

    /* open lock directory */
    if (chdir(lckdir) != 0 || (dir = opendir(lckdir)) == NULL)
		exit(101);		/* good old code 101 */
    while (gnamef(dir, f) == TRUE) {
	/* find all lock files */
	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,996, "f (%s)\n")), f);
	if (EQUALSN("LCK.", f, 4)) {
	    /* read LCK file */
	    fd = open(f, O_RDONLY);
	    printf("%s: ", f);
#ifdef ASCIILOCKS
	    ret = read(fd, alpid, SIZEOFPID+2); /* +2 for '\n' and null */
	    pid = atoi(alpid);
#else
	    ret = read(fd, &pid, sizeof (int));
#endif
	    (void) close(fd);
	    if (ret != -1) {
		printf("%d\n", pid);
		for(i=0; i<MAXLOCKS; i++) {
		    if (list[i] == pid)
			break;
		    if (list[i] == -1) {
		        list[i] = pid;
		        break;
		    }
		}
	    }
	    else
		printf((catgets(nlmsg_fd,NL_SETN,997, "????\n")));
	}
    }
    fflush(stdout);
    *buf = NULLCHAR;
    for (i=0; i<MAXLOCKS; i++) {
	if( list[i] == -1)
		break;
	(void) sprintf(&buf[strlen(buf)], "%d ", list[i]);
    }

    if (i > 0)

#ifdef V4FS
#ifdef V7
	execl("/usr/bin/ps", "uustat-ps", buf, 0);
#else
	execl("/usr/bin/ps", "ps", "-flp", buf, 0);
#endif V7
#else
#ifdef V7
	execl("/bin/ps", "uustat-ps", buf, 0);
#else
	execl("/bin/ps", "ps", "-flp", buf, 0);
#endif V7
#endif V4FS

    exit(0);
}

int machcmp(a,b)
char *a,*b;
{
	return(strcmp(((struct m *) a)->mach,((struct m *) b)->mach));
}

static long _sec_per_day = 86400L;

/*
 * _age - find the age of "file" in days
 * return:
 *	age of file
 *	0 - if stat fails
 */

int
_age(dir, file)
char * file;	/* the file name */
char * dir;	/* system spool directory */
{
	char fullname[MAXFULLNAME];
	static time_t ptime = 0;
	time_t time();
	struct stat stbuf;

	if (!ptime)
		(void) time(&ptime);
	(void) sprintf(fullname, "%s/%s", dir, file);
	if (stat(fullname, &stbuf) != -1) {
		return ((int)((ptime - stbuf.st_mtime)/_sec_per_day));
	}
	else
		return(0);
}
