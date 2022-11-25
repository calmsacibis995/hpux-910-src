static char *HPUX_ID = "@(#) $Revision: 70.1 $";
#ifndef lint
#endif
/*
 * last
 */
#include <sys/types.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <utmp.h>
#include <search.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/pstat.h>
#ifdef DISKLESS
#include <cluster.h>
#endif

#undef 	TRUE
#define TRUE	1

#undef  FALSE
#define FALSE	0

#define NMAX	sizeof(buf[0].ut_name)
#define LMAX	sizeof(buf[0].ut_line)
#define	HMAX	sizeof(buf[0].ut_host)
#define	SECDAY	(24*60*60)

#define	lineq(a,b)	(!strncmp(a,b,LMAX))
#define	nameq(a,b)	(!strncmp(a,b,NMAX))
#define	hosteq(a,b)	(!strncmp(a,b,HMAX))


/*
 * added ARGUMENTS and extern declarations.  hesh@hpda, 880510.
 */
#ifdef DISKLESS
#define	ARGUMENTS	"[ -n ] [ -c ] [ -R ] [ name ... ] [ tty ... ]"
#else /* DISKLESS */
#define	ARGUMENTS	"[ -n ] [ -R ] [ name ... ] [ tty ... ]"
#endif /* DISKLESS */

extern char *		basename();	/* like basename(1) */
extern void		usage();	/* generic usage handler */

char	**argv;
int	argc;
int	nameargs;

struct	utmp buf[128];
struct pst_static stbuf;
short    * index;
int 	last;
long  *logouts;
#ifdef DISKLESS
cnode_t	nodes[MAXCNODE + 1];
int	cflag;
#endif

int host_print = 0;

char	utmpfile[40];
long maxrec = 0x7fffffffL;
char	*ctime(), *strspl();
int	onintr();
int  maxuids;
/*
 * Define a list of directories to be searched for tty names. 
 * /dev is the only required directory. It aboarts if this does
 * not exist.
 */

struct ttydirs {
	char *name;
	int  required;
};

struct ttydirs ttydir_list[] = {
	"/dev", 1,
	"/dev/pty", 0,
	NULL, 0
};

main( ac , av )
	char **av;
{
    /* 
     * added command and arguments variables.  hesh@hpda, 880510.
     */
    int numdevs;
    char *command = basename(*av);
    char *arguments = ARGUMENTS;
    char *tp;
    int i;
#ifdef DISKLESS
    struct cct_entry *cct;
    int utmpcdf, found;
    struct stat statbuf;
#endif
    numdevs = getdevs();

/* Here we are adding some fixed number to account for entries
 * in the file for non ttys. Not sure if this number is correct.
 */

    hcreate(numdevs + 128 );   /* create hash table for devices */

  /* maxuids is the maximum allowed users on the system. This
  * we are getting from max_proc field in pst_static structure
  * which will be modified  for 3000+ users. 
  */
    pstat(PSTAT_STATIC, &stbuf, sizeof(stbuf), 1, 0);
    maxuids = stbuf.max_proc;
    logouts = malloc(sizeof(long) * maxuids);
    tp = strrchr(av[0], '/');
    if (tp != NULL)
	tp++;
    else
	tp = av[0];
    if (strcmp(tp, "lastb") == 0)
    {
	last = FALSE;
	strcpy(utmpfile, "/etc/btmp");
    }
    else
    {
	last = TRUE;
	strcpy(utmpfile, "/etc/wtmp");
    }

    time(&buf[0].ut_time);
    ac--, av++;
    /* 
     * using get_opts() now to make future
     * modification much easier.  hesh@hpirs, 880510.
     */
    if (get_opts(&ac, &av) < 0)
    {
	usage(command, arguments);
	exit(1);
    }
    nameargs = argc = ac;
    argv = av;

    for (i = 0; i < argc; i++)
    {
	if (strlen(argv[i]) > 4)
	    continue;
#ifdef bsd_init
	if (!strcmp(argv[i], "~"))
	    continue;
#endif bsd_init
#ifdef arpa_net
	if (!strcmp(argv[i], "ftp"))
	    continue;
#endif arpa_net
	if (getpwnam(argv[i]))
	    continue;
	argv[i] = strspl("tty", argv[i]);
    }

#ifdef DISKLESS
    if (cflag)
    {
	if (last)
	{
	    if (stat("/etc/wtmp+", &statbuf) == 0 ||
		    (statbuf.st_mode & S_IFDIR) &&
		    (statbuf.st_mode & S_ISUID))
	    {
		utmpcdf = 1;
	    }
	    else
	    {
		do_last();
		exit(0);
	    }
	}
	else
	{
	    if (stat("/etc/btmp+", &statbuf) == 0 ||
		    (statbuf.st_mode & S_IFDIR) &&
		    (statbuf.st_mode & S_ISUID))
	    {
		utmpcdf = 1;
	    }
	    else
	    {
		do_last();
		exit(0);
	    }
	}
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
		{
		    if (last)
		    {
			sprintf(utmpfile, "/etc/wtmp+/%s",
				cct->cnode_name);
		    }
		    else
		    {
			sprintf(utmpfile, "/etc/btmp+/%s",
				cct->cnode_name);
		    }
		}
		else
		{
		    if (last)
			strcpy(utmpfile, "/etc/wtmp");
		    else
			strcpy(utmpfile, "/etc/btmp");
		}
		do_last();
		printf("\n");
	    }
	}
    }
    else
#endif
	do_last();
    exit(0);
}

int
do_last()
{
    register int i, k, cp;
    short   count = 0;
    int bl, utmp;
    char *ct;
    register struct utmp *bp;
    long otime;
    struct stat stb;
    ENTRY item, *keyptr;
    int print;
    char *crmsg = (char *)0;
    long crtime;
    long outrec = 0;

#ifdef DISKLESS
    memset(buf, 0, sizeof (buf));
    memset(logouts, 0, sizeof (logouts));
#endif

    utmp = open(utmpfile, 0);
    if (utmp < 0)
    {
	perror(utmpfile);
	return (1);
    }
    fstat(utmp, &stb);
    bl = (stb.st_size + sizeof (buf)-1) / sizeof (buf);
    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
    {
	signal(SIGINT, onintr);
	signal(SIGQUIT, onintr);
    }
    for (bl--; bl >= 0; bl--)
    {
	lseek(utmp, bl * sizeof (buf), 0);
	bp = &buf[read(utmp, buf, sizeof (buf)) / sizeof (buf[0]) - 1];
	for (; bp >= buf; bp--)
	{
	    print = want(bp);
	    if (print)
	    {
		/* clean up non printables to blanks */
		for (cp = 0; cp <= 7; cp++)
		{
		    if ((bp->ut_name[cp] > 0) &&
			    (bp->ut_name[cp] < 32))
			bp->ut_name[cp] = ' ';
		}
		ct = ctime(&bp->ut_time);
		if (host_print)
		{
		    printf("%-*.*s %-*.*s %-*.*s %10.10s %5.5s ",
			    NMAX, NMAX, bp->ut_name,
			    LMAX, LMAX, bp->ut_line,
			    HMAX, HMAX, bp->ut_host,
			    ct, 11 + ct);
		}
		else
		{
		    printf("%-*.*s %-*.*s %10.10s %5.5s ",
			    NMAX, NMAX, bp->ut_name,
			    LMAX, LMAX, bp->ut_line,
		    /*  HMAX, HMAX, bp->ut_host, */
			    ct, 11 + ct);
		}
	    }
	    item.key = malloc(sizeof(bp->ut_line));
	    strncpy(item.key, bp->ut_line, sizeof(bp->ut_line));
	    if(( keyptr = hsearch( item, FIND)) == NULL) 
	    {
	        otime = logouts[count];
		logouts[count] = bp->ut_time;
	    	index = (int *)malloc(sizeof(int));
		*index = count;
	    	item.data = (char *)index;
		count++;
		hsearch( item, ENTER);
	    }
	    else
	    {
   		index = keyptr->data;
		otime = logouts[*index];
		logouts[*index] = bp->ut_time;
	    }
	    if (print)
	    {
		if (lineq(bp->ut_line, "~"))
		    printf("\n");
		else if (otime == 0)
		    if (last)
			printf("  still logged in\n");
		    else
			printf("\n");
		else
		{
		    long delta;
		    if (otime < 0)
		    {
			otime = -otime;
			printf("- %s", crmsg);
		    }
		    else
			printf("- %5.5s",
				ctime(&otime) + 11);
		    delta = otime - bp->ut_time;
		    if (delta < SECDAY)
			printf("  (%5.5s)\n",
				asctime(gmtime(&delta)) + 11);
		    else
			printf(" (%ld+%5.5s)\n",
				delta / SECDAY,
				asctime(gmtime(&delta)) + 11);
		}
		fflush(stdout);
		if (++outrec >= maxrec)
		    return (0);
	    }
	    if (lineq(bp->ut_line, "~"))
	    {
		for (i = 0; i < maxuids; i++)
		    logouts[i] = -bp->ut_time;
		if (nameq(bp->ut_name, "shutdown"))
		    crmsg = "down ";
		else
		    crmsg = "crash";
	    }
	}			/* for */
    }
    ct = ctime(&buf[0].ut_time);
    if (last)
	printf("\nwtmp begins %10.10s %5.5s \n", ct, ct + 11);
    else
	printf("\nbtmp begins %10.10s %5.5s \n", ct, ct + 11);
}

int
onintr(signo)
	int signo;
{
    char *ct;

    if (signo == SIGQUIT)
	signal(SIGQUIT, onintr);
    ct = ctime(&buf[0].ut_time);
    printf("\ninterrupted %10.10s %5.5s \n", ct, ct + 11);
    if (signo == SIGINT)
	exit(1);
}

int
want(bp)
	struct utmp *bp;
{
    register char **av;
    register int ac;

    if (bp->ut_type == BOOT_TIME) {
       strcpy(bp->ut_name, "reboot");	/* bandaid */
       bp->ut_type = USER_PROCESS;
       }
    if (strncmp(bp->ut_line, "ftp", 3) == 0)
	bp->ut_line[3] = '\0';
    if (bp->ut_name[0] == 0)
	return (0);
    if (last)
    {
	if ((nameargs == 0) && (bp->ut_type == USER_PROCESS))
	    return (1);
    }
    else
    {
	if (nameargs == 0)
	    return (1);
    }
    av = argv;
    for (ac = 0; ac < argc; ac++, av++)
    {
	if (av[0][0] == '-')
	    continue;

	if (last) {
	    if ((nameq(*av, bp->ut_name) || lineq(*av, bp->ut_line)) &&
	        (bp->ut_type == USER_PROCESS))
	        return (1);
	} else {
	    if (nameq(*av, bp->ut_name) || lineq(*av, bp->ut_line))
	        return (1);
	}
    }
    return (0);
}

char *
strspl(left, right)
	char *left, *right;
{
    char *res = (char *)malloc(strlen(left) + strlen(right) + 1);

    strcpy(res, left);
    strcat(res, right);
    return (res);
}

/*
 * hesh_code() starts here.  hesh@hpirs, 880510.
 */

/*
 * can't use getopt() because the -c option
 * needs to be ifdef'd.
 */
#define	reg		register

int
get_opts(ac, av)
int *ac;
char **av[];
{
    reg int rvalue = 0;
    reg char *sp;


    while ((*ac > 0) && (***av == '-'))
    {
	sp = **av;
	(*av)++;
	(*ac)--;
	while (*(++sp))
	    switch (*sp)
	    {
#ifdef DISKLESS
	    case 'c': 
		cflag++;
		break;
#endif				/* DISKLESS */
		/* Print in BSD format ==> show host name */
	    case 'R': 
		host_print++;
		break;
	    case '0': 
	    case '1': 
	    case '2': 
	    case '3': 
	    case '4': 
	    case '5': 
	    case '6': 
	    case '7': 
	    case '8': 
	    case '9': 
		/* 
		 * get maxrec, then skip over remaining digits
		 * (since they aren't options).
		 *
		 * back up one so main while()
		 * doesn't mess up.
		 *
		 * (a better scheme would be "[ -n number ]"
		 * rather than [ -n ] -- the former is much
		 * easier to parse.  but i'm only here to
		 * fix the -c flag ordering bug, not rewrite
		 * the program and documentation, so i'll
		 * just leave it as is.
		 */
		maxrec = atoi(sp);
		while (*(++sp))
		{
		    ;
		}
		--sp;
		break;
	    default: 
		(void)fprintf(stderr, "'%c': unknown option\n", *sp);
		rvalue = -1;
		break;
	    }
    }
    return (rvalue);
}

void
usage(command, arguments)
char *command;
char *arguments;
{
    (void)fprintf(stderr, "usage: %s", command);
    if (arguments != NULL)
    {
	(void)fprintf(stderr, " %s", arguments);
    }
    (void)fputc('\n', stderr);
    return;
}

char *
basename(string)
char *string;
{

    reg char *ptr = (char *)0;


    if ((ptr = strrchr(string, '/')) != (char *)0)
    {
	return (++ptr);
    }
return (string);
}


/* Searches list of directories for number of devices in them.    */
/* This routine has been borrowed from ps.c and slightly modified */
getdevs()
{
	struct direct *dp;
	struct stat st_buf;
	DIR *dirp;
	struct ttydirs *tptr;
	int end_offset, burst_count = 0, numdev = 0;

	numdev = 0;
	for(tptr = ttydir_list; tptr->name != NULL; tptr++) {
		if ((dirp = opendir(tptr->name)) == NULL) {
			if (tptr->required) {
				fprintf(stderr, "last: cannot open %s\n", tptr->name);
				exit(1);
			} else
				continue;
		}
		if (chdir(tptr->name) < 0) {
			if (tptr->required) {
				fprintf(stderr, "last: cannot change to %s\n", tptr->name);
				exit(1);
			} else {
				closedir(dirp);
				continue;
			}
		}
		for(dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
			if (stat(dp->d_name, &st_buf) < 0)
				continue;
			if (((st_buf.st_mode&S_IFMT) != S_IFCHR) 
			      || (!strcmp(dp->d_name,"syscon")) 
			      || (!strcmp(dp->d_name, "systty")))
				continue;
			numdev++;
	  		}
		}
		closedir(dirp);
		return(numdev);
}

