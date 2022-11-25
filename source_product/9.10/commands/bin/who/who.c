static char *HPUX_ID = "@(#) $Revision: 70.2 $";

/* Program to read the "/etc/utmp" file and list the various */
/* entries. */

#include <sys/types.h>
#include <sys/stat.h>
#include <utmp.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

extern int h_errno;

#ifdef DISKLESS
#include <cluster.h>
#endif
#ifdef NLS
#define MAXLNAME 14	/* maximum length of a language name */
#define NL_SETN 1	/* message set number */
#include <nl_types.h>
#include <locale.h>
nl_catd nlmsg_fd;
nl_catd nlmsg_tfd;
#else
#define catgets(i, sn,mn,s) (s)
#endif NLS

#define TRUE	1
#define FALSE	0
#define FAILURE	-1

#define MAXLINE	100

#define FLAGS	UTMAXTYPE

#define SIZEUSER	8
#define MAX_HOST_LEN	100
				/* For -q (quick) who */
char **u_name;      		/* Holds output array of names.  */
int *iarray;			/* Pointer to user-ids in ucount. */
int ucount;			/* number of users */
#define USR_STR_SZ	(SIZEUSER + 2)
#define U_NAME(element)	((char *)u_name + ((element) * USR_STR_SZ))

char username[SIZEUSER + 1];

int shortflag;		/* Set when fast form of "who" is to run. */
int ttystatus;		/* Set when write to tty status desired. */
int allflag;		/* Set when all entries are to be dumped. */
int hflag;			/* Header */
int qflag;			/* Quick who */
int nspace;		/* leave a space between stats and output */
int ut_host_flag;	/* print out the host name (BSD-like format) */

#ifdef DISKLESS
int cflag;			/* cluster-wide who */
int initcdf;
#endif

char *utmpfile = "/etc/utmp";
char initfile[80];

static FILE *fpit;
static int entries[FLAGS+1];	/* Flag for each type entry */
char *ptr;
char *get_ut_host();

main(argc,argv)
int argc;
register char **argv;
{
    register int i;
    extern char *file, *strncpy(), *user(), *mode(), *line(), *etime(), *id();
    extern char *loc(), *ttyname(), *trash();
    extern void mytermonly();
    struct passwd *passwd;
    int done = FALSE;
    extern char username[];
    extern struct passwd *getpwuid();
    extern int shortflag, ttystatus, allflag;
#ifdef DISKLESS
    cnode_t nodes[MAXCNODE];
    struct cct_entry *cct;
    char tmps[80];
    int utmpcdf = 0, initcdf = 0;
    int found;
    struct stat statbuf;
#endif				/* DISKLESS */

#if defined(NLS) || defined(NLS16)
    /* 
     * initialize to the current locale
     */
    unsigned char lctime[5 + 4 * MAXLNAME + 4], *pc;
    unsigned char savelang[5 + MAXLNAME + 1];

    if (!setlocale(LC_ALL, ""))
    {				/* setlocale fails */
	fputs(_errlocale("who"), stderr);
	putenv("LANG=");
	nlmsg_fd = (nl_catd)-1;	/* use default messages */
	nlmsg_tfd = (nl_catd)-1;
    }
    else
    {				/* setlocale succeeds */
	nlmsg_fd = catopen("who", 0);	/* use $LANG messages */
	strcpy(lctime, "LANG=");/* $LC_TIME affects some msgs */
	strcat(lctime, getenv("LC_TIME"));
	if (lctime[5] != '\0')
	{			/* if $LC_TIME is set */
	    strcpy(savelang, "LANG=");	/* save $LANG */
	    strcat(savelang, getenv("LANG"));
	    if ((pc = (unsigned char *)strchr(lctime, '@')) != NULL)
		/* if modifier */
		*pc = '\0';	/* remove modifer part */
	    putenv(lctime);	/* use $LC_TIME for some msgs */
	    nlmsg_tfd = catopen("who", 0);
	    putenv(savelang);	/* reset $LANG */
	}
	else			/* $LC_TIME is not set */
	    nlmsg_tfd = nlmsg_fd;   /* use $LANG messages */
    }
#endif				/* NLS || NLS16 */

    /* 
     *  Use the kernel's id of timezone info when who is
     *  being run as a login shell.  This will use the
     *  default rules for changing to/from Daylight Savings
     *  Time (which won't always be correct for all users
     *  but is the best we can do without modifying the
     *  kernel to return a real TZ string).
     */
    if (!strcmp("-who", argv[0]))
    {
	struct timeval tp;
	struct timezone tzp;
	char buf[20];
	(void)gettimeofday(&tp, &tzp);
	if (tzp.tz_dsttime != 0)
	{
	    (void)sprintf(buf, "TZ=<NO TZ NAME>%d<NO TZ NAME>",
		    (tzp.tz_minuteswest / 60));
	}
	else
	{
	    (void)sprintf(buf, "TZ=<NO TZ NAME>%d", tzp.tz_minuteswest / 60);
	}
	putenv(buf);
    }

/* Initialize initfile to /etc/inittab */
    strcpy(initfile, "/etc/inittab");

/* Look up our own name for reference purposes. */

    if ((passwd = getpwuid(getuid())) == (struct passwd *)NULL)
    {
	username[0] = '\0';
    }
    else
    {
/* Copy the user's name to the array "username". */
	strncpy(&username[0], passwd->pw_name, SIZEUSER);
	username[SIZEUSER] = '\0';
    }

/* Set "ptr" to null pointer so that in normal case of "who" */
/* the "ut_line" field will not be looked at.  "ptr" is used */
/* only for "who am i", "who am I" or "who -m" options. */

    ptr = NULL;
    shortflag = FALSE;		/* not set yet */
    ttystatus = FALSE;
    hflag = FALSE;
    qflag = FALSE;
    ut_host_flag = FALSE;
    ucount = FALSE;
    nspace = FALSE;

/* Is this the "who am i" sequence? */

    if (strcmp(argv[1], "am") == 0 &&
        ((strcmp(argv[2], "i") == 0) || (strcmp(argv[2], "I") == 0)))
    {
	mytermonly();
	argc -= 2;
	argv += 2;
    }

    /* Is the next arg a file (utmp-like)?? */
    else if (access(argv[1], F_OK) != FAILURE)
    {
	/* Is there an argument left?  If so, assume that it is a
	 * utmp like file.  If there isn't one, use the default
	 * file. 
	 */

	utmpfile = argv[1];
	utmpname(argv[1]);
	++argv;
	--argc;
    }

    /*
     * Analyze the switches and set the flags for each type of entry 
     * that the user requests to be printed out. 
     */

    /*
     * NOTE:  This code has been modified to use the getopt() library
     * interface per the POSIX.2a specifications.  It preserves previously
     * "undocumented" argument ordering at the expense of making the
     * getopt() code less elegant, however, it will continue to support
     * things like:
     *     who /etc/utmp -T am I -s
     * -- raf 4/25/91
     */

    while (! done)
    {
	extern int optind;
	int c;

	c = getopt(argc, argv, "cqHrbtpluRdAasThm");
	if (c != -1)
	{
	    switch (c)
	    {
#ifdef DISKLESS
	    case 'c': 
	        cflag++;
	        break;
#endif /* DISKLESS */
	    case 'q': 
	        qflag++;
	        entries[USER_PROCESS] = TRUE;
	        break;
	    case 'H': 
	        hflag++;
	        break;
	    case 'r': 
	        entries[RUN_LVL] = TRUE;
	        nspace = TRUE;
	        break;
	    case 'b': 
	        entries[BOOT_TIME] = TRUE;
	        nspace = TRUE;
	        break;
	    case 't': 
	        entries[OLD_TIME] = TRUE;
	        nspace = TRUE;
	        entries[NEW_TIME] = TRUE;
	        break;
	    case 'p': 
	        entries[INIT_PROCESS] = TRUE;
	        break;
	    case 'l': 
	        entries[LOGIN_PROCESS] = TRUE;
	        if (shortflag == 0)
		    shortflag = -1;
	        break;
	    case 'u': 
	        entries[USER_PROCESS] = TRUE;
	        if (shortflag == 0)
		    shortflag = -1;
	        break;
	    case 'R': 
	        ut_host_flag = TRUE;
	        break;
	    case 'd': 
	        entries[DEAD_PROCESS] = TRUE;
	        break;
	    case 'A': 
	        entries[ACCOUNTING] = TRUE;
	        break;
	    case 'a': 
	        for (i = 1; i < FLAGS; i++)
		    entries[i] = TRUE;
	        if (shortflag == 0)
		    shortflag = -1;
	        allflag = TRUE;
	        nspace = TRUE;
	        break;
	    case 's': 
	        shortflag = 1;
	        break;
	    case 'T': 
	        ttystatus = TRUE;
	        if (shortflag == 0)
		    shortflag = -1;
	        break;
	    case 'm':
		mytermonly();
		break;
	    case '?': 
	    case 'h': 
	        usage("");
	        break;
	    default: 
	        fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 2, 
		        "who:  illegal option -- %c")), **argv);
	        usage("");
	    }
	} /* end if (c != -1) */
	else /* c == -1 */
	{
            /* Is this the "who am i" sequence? */

	    if (strcmp(argv[optind], "am") == 0 &&
	        ((strcmp(argv[optind + 1], "i") == 0) ||
	         (strcmp(argv[optind + 1], "I") == 0)))
	    {
		mytermonly();
	        optind += 2;
	        continue;
	    }

	    /* Is the next arg a file (utmp-like)?? */
	    else if (access(argv[optind], F_OK) != FAILURE)
	    {
	        /* Is there an argument left?  If so, assume that it is a
	         * utmp like file.  If there isn't one, use the default
	         * file. 
	         */

	        utmpfile = argv[optind];
	        utmpname(argv[optind]);
	        optind += 1;
	    }
	    else if (argv[optind] != (char *) NULL)
	    {
	         usage((catgets(nlmsg_fd, NL_SETN, 3, 
		       "%s doesn't exist or isn't readable")), argv[optind]);
	    }
	    else
	    {
		/* no more options to parse */
		done = TRUE;
	    }
	} /* end of (c == -1) section */
    } /* End of "while (! done)" */

    if (qflag)
    {
	/* reset all the other options * except c flag */
	for (i = 0; i < FLAGS; i++)
	    entries[i] = FALSE;
	entries[USER_PROCESS] = TRUE;
	nspace = FALSE;
	hflag = 0;
	allflag = FALSE;
	ttystatus = FALSE;
	ut_host_flag = FALSE;
	shortflag = 0;
    }
    /* Make sure at least one flag is set. */

    for (i = 0; entries[i] == FALSE && i <= FLAGS; i++)
	continue;
    if (i > FLAGS)
	entries[USER_PROCESS] = TRUE;


    /*
     * Now scan through the entries in the utmp type file and 
     * list those matching the requested types.
     */

#ifdef DISKLESS
    if (cflag)
    {
	sprintf(tmps, "%s+", utmpfile);
	if ((stat(tmps, &statbuf) == 0) && (statbuf.st_mode & S_IFDIR)
		&& (statbuf.st_mode & S_ISUID))
	    utmpcdf = 1;
	else
	{
	    /* 
	     * if passed utmp-like file is not a CDF,
	     * ignore the -c flag;
	     */
	    process_utmp(1);
	    exit(0);
	}

	if (stat("/etc/inittab+", &statbuf) == 0 &&
		(statbuf.st_mode & S_IFDIR) && (statbuf.st_mode & S_ISUID))
	    initcdf = 1;

	setccent();
	cnodes(nodes);
	while ((cct = getccent()) != NULL)
	{
	    found = 0;
	    for (i = 0; nodes[i]; i++)
		if (cct->cnode_id == nodes[i])
		{
		    found = 1;
		    break;
		}
	    if (found)
	    {
		printf("%s:\n", cct->cnode_name);
		if (utmpcdf)
		    sprintf(tmps, "%s+/%s", utmpfile,
			    cct->cnode_name);
		else
		    strcpy(tmps, utmpfile);
		utmpname(tmps);
		if (initcdf)
		{
		    sprintf(initfile, "/etc/inittab+/%s",
			    cct->cnode_name);
		    fpit = NULL;
		}
		ucount = 0;
		process_utmp(cnodeid() == cct->cnode_id);
		printf("\n");
	    }
	}
    }
    else
#endif				/* DISKLESS */
	process_utmp(1);
}

int
process_utmp(local)
int local;
{
    extern struct utmp *getutent();
    register struct utmp *utmp;
    int size;
    int inittab_comment_found = 0;
    char t_str[MAX_HOST_LEN + 5];
    struct stat st_buf;

    /* stat the utmp file to get its size */
    if (stat(utmpfile, &st_buf) == -1)
    {
	exit(0);
    }

    /* malloc the u_name array */
    size = st_buf.st_size / sizeof (struct utmp);
    u_name = (char **)malloc(size * USR_STR_SZ * sizeof (char));

    /* malloc the iarray array */
    iarray = (int *)malloc((size + 1) * sizeof (int));

    while ((utmp = getutent()) != NULL)
    {
	/* Are we looking for this type of entry? */

	if (allflag ||
		((utmp->ut_type >= 0 && utmp->ut_type <= FLAGS) &&
		    entries[utmp->ut_type] == TRUE))
	{
	    if (utmp->ut_type == EMPTY)
	    {
		printf(catgets(nlmsg_fd, NL_SETN, 4, "Empty slot.\n"));
		continue;
	    }
	    if (ptr != NULL && strncmp(utmp->ut_line, ptr, sizeof (utmp->ut_line)) != 0)
		continue;
	    if (qflag)
	    {
		sprintf(U_NAME(ucount++), "%.8s ", &utmp->ut_user[0]);
		continue;
	    }

	    /* if hflag (header) flag is set print header */

	    if (hflag && utmp->ut_type > INIT_PROCESS
		    && utmp->ut_type != DEAD_PROCESS)
	    {
		if (nspace)
		    printf("\n");
		if (shortflag < 0)
		    printf(catgets(nlmsg_fd, NL_SETN, 5, "NAME       LINE         TIME          IDLE    PID  COMMENTS\n"));
		else
		    printf(catgets(nlmsg_fd, NL_SETN, 6, "NAME       LINE         TIME\n"));
		hflag = FALSE;
	    }

	    printf("%8.8s %s %12.12s %s",
		    user(utmp), mode(utmp, local), line(utmp), etime(utmp));

	    if (utmp->ut_type == RUN_LVL)
	    {
		printf("    %c    %d    %c", utmp->ut_exit.e_termination,
			utmp->ut_pid, utmp->ut_exit.e_exit);
	    }
	    if (shortflag < 0 &&
		    (utmp->ut_type == LOGIN_PROCESS
			|| utmp->ut_type == USER_PROCESS
			|| utmp->ut_type == INIT_PROCESS
			|| utmp->ut_type == DEAD_PROCESS))
	    {
		printf("  %5d", utmp->ut_pid);
		if (utmp->ut_type == INIT_PROCESS
			|| utmp->ut_type == DEAD_PROCESS)
		{
		    printf(catgets(nlmsg_fd, NL_SETN, 7, "  id=%4.4s"), id(utmp));
		    if (utmp->ut_type == DEAD_PROCESS)
		    {
			printf(catgets(nlmsg_fd, NL_SETN, 8, " term=%-3d exit=%-3d"),
				utmp->ut_exit.e_termination, utmp->ut_exit.e_exit);
		    }
		}
		else if ((utmp->ut_type == LOGIN_PROCESS
			|| utmp->ut_type == USER_PROCESS) && 
			!ut_host_flag)
		{
		    printf("  %s", loc(utmp, &inittab_comment_found));
		}
	    }
	    if (utmp->ut_type == USER_PROCESS)
	    {
		if ((shortflag < 0) && (ut_host_flag))
		{
		    t_str[0] = t_str[1] = ' ';
		    strcpy(&t_str[2], get_ut_host(utmp));
		}
		else if ((shortflag < 0) && (!ut_host_flag))
		{
		    if (!inittab_comment_found)
		    {
		    if (utmp->ut_host[0] != '\0')
			sprintf(t_str, "%.*s", sizeof utmp->ut_host,
				utmp->ut_host);
		    else if (utmp->ut_addr != 0)
			sprintf(t_str, "%s", inet_ntoa(utmp->ut_addr));
		    else
			t_str[0] = '\0';
		    }
		    else
			t_str[0] = '\0';
		    inittab_comment_found = 0;
		}
		else if (ut_host_flag)
		{
		    sprintf(t_str, "  (%.*s)", MAX_HOST_LEN,
			    get_ut_host(utmp));
		}
		if ((shortflag < 0) || ut_host_flag)
		    printf("%s", t_str);
	    }
	    putchar('\n');
	}

    }				/* End of "while ((utmp = getutent()" */
    if (qflag)
	doquick();
}

int
doquick()
{

    int i, k, n, swapflag;

    for (i = 1; i < ucount; i++)
	iarray[i] = i;
    iarray[ucount] = 0;
    do
    {
	swapflag = 0;
	for (i = 1; i < ucount; i++)
	{
	    if (strcmp(U_NAME(iarray[i]), U_NAME(iarray[i + 1])) > 0)
	    {
		swapflag++;
		n = iarray[i];
		iarray[i] = iarray[i + 1];
		iarray[i + 1] = n;
	    }
	}
    } while (swapflag);

    n = 0;
    for (i = 1; i <= ucount; i++)
    {
	k = iarray[i];
	n += strlen(U_NAME(k));
	if (n > 79)
	{
	    putchar('\n');
	    n = strlen(U_NAME(k));
	}
	printf(U_NAME(k));
    }

    printf(catgets(nlmsg_fd, NL_SETN, 9, "\n# users=%d\n"), ucount);
}

/*
 * The mytermonly() routine sets up ptr and the entries[] array to
 * print information only for the tty from which the "who" command was
 * executed (the "am i", "am I" or "-m" options to who).
 */
void
mytermonly()
{
    entries[USER_PROCESS] = TRUE;

    /* Which tty am I at?  Get the name and set "ptr" to 
     * just past the "/dev/" part of the pathname.
     */

    if ((ptr = ttyname(fileno(stdin))) == NULL &&
        (ptr = ttyname(fileno(stdout))) == NULL &&
        (ptr = ttyname(fileno(stderr))) == NULL)
    {
	usage((catgets(nlmsg_fd, NL_SETN, 1, 
	      "process not attached to terminal")));
    }
    ptr += sizeof ("/dev/")-1;
}

int
usage(format,arg1)
char *format;
int arg1;
{
    fprintf(stderr, format, arg1);
#ifdef DISKLESS
    fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 10, "\nUsage:\twho [-rbtpludAasHTqcRm] [am i] [utmp_like_file]\n"));
#else
    fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 11, "\nUsage:\twho [-rbtpludAasHTqRm] [am i] [utmp_like_file]\n"));
#endif
    fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 12, "\nr\trun level\nb\tboot time\nt\ttime changes\n"));
    fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 13, "p\tprocesses other than getty or users\n"));
    fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 14, "l\tlogin processes\nu\tuseful information\n"));
    fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 15, "d\tdead processes\nA\taccounting information\n"));
    fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 16, "a\tall (rbtpludA options)\n"));
    fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 17, "s\tshort form of who (no time since last output or pid)\n"));
    fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 18, "H\tprint header\n"));
    fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 19, "T\tstatus of tty (+ writable, - not writable, x exclusive open, ? hung)\n"));
    fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 20, "q\tquick who\n"));
#ifdef DISKLESS
    fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 21, "c\tcluster-wide who\n"));
#endif
    fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 26, "R\tprint host name\n"));
    exit(1);
}

char *
user(utmp)
struct utmp *utmp;
{
    static char uuser[sizeof (utmp->ut_user) + 1];

    copypad(&uuser[0], &utmp->ut_user[0], sizeof (uuser));
    return (&uuser[0]);
}

char *
line(utmp)
struct utmp *utmp;
{
    static char uline[sizeof (utmp->ut_line) + 1];

    copypad(&uline[0], &utmp->ut_line[0], sizeof (uline));
    return (&uline[0]);
}

int
copypad(to,from,size)
register char *to;
char *from;
int size;
{
    register int i;
    register char *ptr;
    short printable;
    int halfway;

    size--;			/* Leave room for null */

/* Scan for something textual in the field.  If there is */
/* nothing except spaces, tabs, and nulls, then substitute */
/* '-' for the contents. */

    for (printable = FALSE, i = 0, ptr = from;
	    *ptr != '\0' && i < size; i++, ptr++)
    {
/* Break out if a printable character found.  Note that the test */
/* for "> ' '" eliminates all control characters and spaces from */
/* consideration as printing characters. */

	if (*ptr > ' ')
	{
	    printable = TRUE;
	    break;
	}
    }
    i = 0;
    halfway = 0;
    if (printable)
    {
	for (; *from != '\0' && i < size; i++)
	    *to++ = *from++;
    }
    else
    {
	halfway = (size + 1) / 2 - 1;	/* Where to put "-" */
    }

/* Add pad at end of string consisting of spaces and a '\0'. */
/* Put an asterisk at the halfway position.  (This only happens */
/* when padding out a null field.) */

    for (; i < size; i++)
    {
	*to++ = (i == halfway ? '.' : ' ');
    }
    *to = '\0';			/* Add null at end. */
}

char *
id(utmp)
register struct utmp *utmp;
{
    static char uid[sizeof (utmp->ut_id) + 1];
    register char *ptr;
    register int i;

    for (ptr = &uid[0], i = 0; i < sizeof (utmp->ut_id); i++)
    {
	if (isprint(utmp->ut_id[i]) || utmp->ut_id[i] == '\0')
	{
	    *ptr++ = utmp->ut_id[i];
	}
	else
	{
	    *ptr++ = '^';
	    *ptr = (utmp->ut_id[i] & 0x17) | 0100;
	}
    }
    *ptr = '\0';
    return (&uid[0]);
}

char *
etime(utmp)
struct utmp *utmp;
{
    extern char *ctime();
    register char *ptr;
    long lastactivity;
    char device[20];
    static char eetime[24];
    extern long time();
    struct stat statbuf;
    extern int shortflag;

#ifdef NLS
    ptr = nl_cxtime(&utmp->ut_time, catgets(nlmsg_tfd, NL_SETN, 22, "%b %2d %H:%M"));
#else
    ptr = ctime(&utmp->ut_time);

/* Erase the seconds, year and \n at the end of the string. */

    *(ptr + 16) = '\0';

/* Advance past the day of the week. */

    ptr += 4;
#endif NLS

    strcpy(&eetime[0], ptr);
    if (shortflag < 0
	    && (utmp->ut_type == INIT_PROCESS
		|| utmp->ut_type == LOGIN_PROCESS
		|| utmp->ut_type == USER_PROCESS
		|| utmp->ut_type == DEAD_PROCESS))
    {
	sprintf(&device[0], "/dev/%s", &utmp->ut_line[0]);

/* If the device can't be accessed, add a hyphen at the end. */

	if (stat(&device[0], &statbuf) == FAILURE)
	{
	    strcat(&eetime[0], "   .  ");
	}
	else
	{
/* Compute the amount of time since the last character was sent to */
/* the device.  If it is older than a day, just exclaim, otherwise */
/* if it is less than a minute, put in an '-', otherwise put in */
/* the hours and the minutes. */

	    lastactivity = time(NULL) - statbuf.st_atime;
	    if (lastactivity > 24L * 3600L)
	    {
		strcat(&eetime[0], catgets(nlmsg_tfd, NL_SETN, 23, "  old "));
	    }
	    else if (lastactivity < 60L)
	    {
		strcat(&eetime[0], catgets(nlmsg_tfd, NL_SETN, 24, "   .  "));
	    }
	    else
	    {
		sprintf(&eetime[strlen(eetime)], catgets(nlmsg_tfd, NL_SETN, 25, " %2u:%2.2u"),
			(unsigned)(lastactivity / 3600),
			(unsigned)((lastactivity / 60) % 60));
	    }
	}
    }
    return (&eetime[0]);
}

static int badsys;	/* Set by alarmclk() if open or close times out. */

char *
mode(utmp, local)
struct utmp *utmp;
int local;
{
    char device[20];
    int fd;
    struct stat statbuf;
    register char *answer;
    extern char username[];
    extern alarmclk();
    extern ttystatus;

    if (ttystatus == FALSE
	    || utmp->ut_type == RUN_LVL
	    || utmp->ut_type == BOOT_TIME
	    || utmp->ut_type == OLD_TIME
	    || utmp->ut_type == NEW_TIME
	    || utmp->ut_type == ACCOUNTING
	    || utmp->ut_type == DEAD_PROCESS)
	return (" ");

    if (!local)
	return ("*");
    sprintf(&device[0], "/dev/%s", &utmp->ut_line[0]);

/* Check "access" for writing to the line. */
/* To avoid getting hung, set up any alarm around the open.  If */
/* the alarm goes off, print a question mark. */

    badsys = FALSE;
    signal(SIGALRM, alarmclk);
    alarm(3);
#ifdef	CBUNIX
    fd = open(&device[0], O_WRONLY);
#else
    fd = open(&device[0], O_WRONLY | O_NDELAY);
#endif
    alarm(0);
    if (badsys)
	return ("?");

    if (fd == FAILURE)
    {
/* If our effective id is "root", then send back "x", since it */
/* must be exclusive use. */

	if (geteuid() == 0)
	    return ("x");
	else
	    return ("-");
    }

/* If we are effectively root or this is a login we own then we */
/* will have been able to open this line except when the exclusive */
/* use bit is set.  In these cases, we want to report the state as */
/* other people will experience it. */

    if (geteuid() == 0 || strncmp(&username[0], &utmp->ut_user[0],
		SIZEUSER) == 0)
    {
	if (fstat(fd, &statbuf) == FAILURE)
	    answer = "-";
	if (statbuf.st_mode & 2)
	    answer = "+";
	else
	    answer = "-";
    }
    else
	answer = "+";

/* To avoid getting hung set up any alarm around the close.  If */
/* the alarm goes off, print a question mark. */

    badsys = FALSE;
    signal(SIGALRM, alarmclk);
    alarm(3);
    close(fd);
    alarm(0);
    if (badsys)
	answer = "?";
    return (answer);
}

int
alarmclk()
{
/* Set flag saying that "close" timed out. */

    badsys = TRUE;
}

struct ttyline
{
    char *t_id;			/* Id of the inittab entry */
    char *t_comment;		/* Comment field if one found */
};

static long start,current;

char *
loc(utmp, found)
struct utmp *utmp;
int *found;
{
    struct ttyline *parseitab();
    register struct ttyline *pitab;
    long start, current;
    register int wrap;
    extern long ftell();

    *found = 0;
/* Open /etc/inittab if it is not already open. */

    if (fpit == NULL)
    {
	if ((fpit = fopen(initfile, "r")) == NULL)
	    return ("");
    }

/* Save the start position in /etc/inittab. */

    start = ftell(fpit);

/* Look through /etc/inittab for the entry relating to */
/* the line of interest.  To save time, do not start at beginning */
/* of /etc/inittab, but at the current point.  Care must be taken */
/* to stop after the whole file is scanned. */

    for (current = start, wrap = FALSE; wrap == FALSE || current < start;
	    current = ftell(fpit))
    {
/* If a line of interest was found, see if it was the line we */
/* were looking for. */

	if ((pitab = parseitab()) != (struct ttyline *)NULL)
	{
	    if (strncmp(pitab->t_id, utmp->ut_id, sizeof (utmp->ut_id)) == 0)
	    {
		*found = 1;
		return (pitab->t_comment);
	    }
	}
	else
	{

/* If a line wasn't found, we've hit the end of /etc/inittab.  Set */
/* the wrap around flag.  This will start checking to prevent us */
/* from cycling through /etc/inittab looking for something that is */
/* not there. */

	    wrap = TRUE;
	}
    }
    return ("");
}

struct ttyline *
parseitab()
{
    static struct ttyline answer;
    static char itline[512];
    register char *ptr, *optr;
    register int lastc;
    int i, cflag;
    extern char *strrchr();

    for (;;)
    {
/* Read in a full line, including continuations if there are any. */

	ptr = &itline[0];
	do
	{
	    if (ptr == &itline[0]
		    && fgets(ptr, &itline[sizeof (itline)] - ptr, fpit) == (char *)NULL)
	    {
		fseek(fpit, 0L, 0);
		return ((struct ttyline *)NULL);
	    }

/* Search for the <nl> at the end of the line, keeping track of */
/* quoting so that we will know if the <nl> is quoted. */

	for (lastc = '\0'; *ptr && *ptr != '\n';
		    lastc = (lastc == '\\' ? '\0' : *ptr), ptr++);

/* If there is no <nl> at the end of the line or the <nl> is not */
/* quoted, then set "cflag" to FALSE so that the "do" will */
/* terminate. */

	    if (*ptr == '\0' || lastc != '\\')
	    {
		cflag = FALSE;
		if (*ptr)
		    *ptr = '\0';/* Cover the <nl> */
		else
		{
		    cflag = TRUE;

/* Cover up the quoted <nl> by backing up to the backslash. */

		    --ptr;
		}
	    }
	} while (cflag);

/* Save pointer to the id field of the inittab entry. */

	answer.t_id = &itline[0];
	answer.t_comment = "";

/* Skip over the first three fields (delimited by ':'). */

	for (i = 3, ptr = &itline[0], cflag = TRUE; i; ptr++)
	{
/* If the line is badly formatted, ignore it. */

	    if (*ptr == '\0')
	    {
		cflag = FALSE;
		break;
	    }

/* Turn colons into nulls so that fields are seperate strings. */

	    if (*ptr == ':')
	    {
		*ptr = '\0';
		i--;
	    }
	}

	if (cflag == FALSE)
	    continue;

/* Search for a comment field.  This may be one of the form */
/* ";: comment <null>" or one of the form "# comment <null>" */
/* Note that while ";: comment; command" is legal shell syntax, */
/* it is not expected on a getty line in inittab and is actually */
/* improperly formed since only the first command in inittab */
/* will be executed since there is an implicit "exec" to the */
/* shell command line. */

	for (lastc = '\0';
		*ptr && *ptr != ';' && lastc != '\\' && *ptr != '#';
		lastc = (lastc == '\\' ? '\0' : *ptr), ptr++);

/* If there is no comment, return immediately. */

	if (*ptr == '\0')
	    return (&answer);

/* If there is an unquoted semicolon look to see if the argument */
/* following it is colon and a space or a tab. */

	else if (*ptr == ';')
	{
	    while (*++ptr == ' ' || *ptr == '\t');
	    if (*ptr != ':' || (*++ptr != ' ' && *ptr != '\t'))
		return (&answer);
	}

/* After the "#" or ":; " sequence, skip over white space and */
/* return the remainder as the comment. */

	while (*++ptr == ' ' || *ptr == '\t');
	if (*ptr == '\0')
	    return (&answer);
	answer.t_comment = ptr;
	return (&answer);
    }
}

/* "trash" prints possibly garbage strings so that non-printing */
/* characters appear as visible characters. */

char *
trash(ptrin,size)
register char *ptrin;
register int size;
{
    static char answer[128];
    register char *ptrout;

    ptrout = &answer[0];
    while (--size >= 0)
    {
/* If the character to be printed is negative, print it as <-x>. */

	if (*ptrin & 0x80)
	{
	    *ptrout++ = '<';
	    *ptrout++ = '-';
	}
	if (isprint(*ptrin & 0x7f))
	    *ptrout++ = (*ptrin & 0x7f);

/* If the low seven bits of the character are not printable, */
/* print as ^x, where 'x' is the low seven bits plus 0x40. */

	else
	{
	    *ptrout++ = '^';
	    *ptrout++ = (*ptrin & 0x7f) + 0x40;
	}

/* Finish up the corner brackets if the character was negative. */

	if (*ptrin & 0x80)
	    *ptrout++ = '>';
	ptrin++;
    }
    return (&answer[0]);
}

char *
get_ut_host(U)
struct utmp *U;
{
    static char buf[MAX_HOST_LEN];
    struct hostent *host_ent;
    char save_last;

    if (U->ut_exit.e_exit == 1)	/* Exit status of 1 ==> tty */
    {
	if (gethostname(buf, MAX_HOST_LEN) < 0)
	    strcpy(buf, U->ut_host);	/* Better than nothing */
    }
    else			/* We have a pty */
    {
	/* See if the host was less than 16 characters */
	save_last = U->ut_host[15];
	U->ut_host[15] = '\0';

	if (strlen(U->ut_host) < (sizeof U->ut_host - 1))
	    strcpy(buf, U->ut_host);
	else
	{
	    if (U->ut_addr != 0)/* We have a valid internet address */
	    {
		/* host_ent = (struct hostent *) malloc(sizeof(struct
		   hostent); */
		host_ent = gethostbyaddr((char *)&U->ut_addr,
			sizeof (struct in_addr), AF_INET);
		if (host_ent == NULL)
		{
		    U->ut_host[15] = save_last;
		    strncpy(buf, U->ut_host, sizeof U->ut_host);
		}
		else
		    strcpy(buf, host_ent->h_name);
	    }
	    else
	    {
		/* Assume local ??? or just return truncated value??? */
		U->ut_host[15] = save_last;
		strncpy(buf, U->ut_host, sizeof U->ut_host);
	    }
	}
    }
    return buf;
}
