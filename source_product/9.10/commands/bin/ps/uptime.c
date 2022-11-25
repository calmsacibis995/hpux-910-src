/* $Source: /misc/source_product/9.10/commands.rcs/bin/ps/uptime.c,v $
 * $Revision: 70.2 $            $Author: ssa $
 * $State: Exp $               $Locker:  $
 * $Date: 92/07/27 13:43:48 $
 */

#ifndef lint
static char *HPUX_ID = "@(#) uptime.c  $Revision: 70.2 $ $Date: 92/07/27 13:43:48 $";
#endif  lint

/*
 * Revision 1.1   89/04/06  13:49:50  13:49:50  jonb
 * Merged/converged  PSTAT-based version.
 *
 * Revision 49.1  87/02/20  15:10:30  15:10:30  jennie
 * Changed <spectrum/pde.h> to <machine/pde.h>.
 * 
 * Revision 1.5  85/12/03  15:21:25  15:21:25  jcm (J. C. Mercier)
 * Fixed problem dealing with types of utmp entries.  WE
 * now do only the USER ones.
 * 
 * Revision 1.4  85/12/03  15:10:46  15:10:46  jcm (J. C. Mercier)
 * Discard process of type LOGIN (see utmp.h) for spectrum.
 * 
 * Revision 1.3  85/12/03  14:44:43  14:44:43  jcm (J. C. Mercier)
 * Added capability for w for spectrum.
 * This version looks pretty good.
 * 
 * Revision 1.2  85/12/03  08:34:18  jcm (J. C. Mercier)
 * Header added.  
 */

/* 
 * HP-UX uptime -- print how long system has been up and 
 *		   load averages over last 1, 5, and 15 minutes.
 *
 *	It is the first line of a Berkeley w(1) command, thus most
 * 	of the code that corresponds to "w" has been ifdef'd
 *	out with "#ifdef W."
 */

/*
 * w - print system status (who and what)
 *
 * This program is similar to the systat command on Tenex/Tops 10/20
 * It needs read permission on /dev/mem, /dev/kmem, and /dev/drum.
 *
 * Note that w is still an "unsupported" command.  It is linked to
 * uptime.  Note also that the JCPU time is not calculated in this
 * release since the user structure can not be obtained.
 *
 */

#ifdef BUILDFROMH
#include <h/param.h>
#include <h/types.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <pwd.h>
#include <ndir.h>
#include <h/signal.h>
#include <h/sysmacros.h>
#include <h/user.h>
#include <h/proc.h>
#include <h/tty.h>
#include <h/stat.h>
#include <h/utsname.h>
#include <rpc/rpc.h>		/*  Delivered with the NFS product.  */
#include <rpcsvc/yp_prot.h>	/*  Delivered with the NFS product.  */
/* INCLUDES for mflg */
#ifdef __hp9000s300
#include <h/vm.h>
#endif hp9000s300
#ifdef hp9000s800
#include <machine/pde.h>
#endif /* __hp9000s800 */
#include <machine/vmparam.h>
#include <h/vmmac.h>
/* INCLUDES for mflg */
#include <h/pstat.h>
#include <utmp.h>

#else	/* ifdef BUILDFROMH */

#include <sys/param.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <pwd.h>
#include <ndir.h>
#include <sys/signal.h>
#include <sys/sysmacros.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <rpc/rpc.h>		/*  Delivered with the NFS product.  */
#include <rpcsvc/yp_prot.h>	/*  Delivered with the NFS product.  */
/* INCLUDES for mflg */
#ifdef __hp9000s300
#include <sys/vm.h>
#endif /* __hp9000s300 */
#ifdef __hp9000s800
#include <machine/pde.h>
#endif /* __hp9000s800 */
#include <machine/vmparam.h>
#include <sys/vmmac.h>
/* INCLUDES for mflg */
#include <sys/pstat.h>
#include <utmp.h>

#endif	/* ifdef BUILDFROMH */
#if defined(SecureWare) && defined(W)
#include <sys/security.h>
#endif

#define NMAX sizeof(utmp.ut_name)
#define LMAX sizeof(utmp.ut_line)

#define PSBURST	10 /* # of pstat entries to read at a time */


#ifdef W
struct pr {
	short	w_pid;			/* proc.p_pid */
	char	w_flag;			/* proc.p_flag */
	short	w_size;			/* proc.p_size */
	long	w_seekaddr;		/* where to find args */
	long	w_lastpg;		/* disk address of stack */
	int	w_igintr;		/* INTR+3*QUIT, 0=die, 1=ign, 2=catch */
	time_t	w_time;			/* CPU time used by this process */
	time_t	w_ctime;		/* CPU time used by children */
	dev_t	w_tty;			/* tty device of process */
	char	w_comm[PST_CLEN];	/* user.u_comm, null terminated */
} *pr;
#endif W


FILE	*ut;
/* mflg Variables. */
struct proc *mproc;


#ifdef __hp9000s800
off_t vtophys();

struct user user;
#define u user
#define	up	user

#endif /* __hp9000s800 */

#ifdef __hp9000s300

struct pte *Usrptmap_ps;
struct pte *usrpt_ps;
struct chtbl_entry *chtbl_ps;

union user_union {
	struct user user;
	char upages [UPAGES][NBPG];
} user;

#define u user.user
#define	up	user.user

#endif /* __hp9000s300 */

/* End mflg Variables. */

#ifdef W
int	nswap;
int	dmmin, dmmax;
dev_t	tty;
char	doing[520];		/* process attached to terminal */
time_t	proctime;		/* cpu time of process in doing */
#endif
double	avenrun[3];
#ifdef	W
struct	proc	*kproc, *proc;	/* proc must be exactly the kernel symbol */
struct  proc	 proc_buf;
struct	proc *aproc;
#endif	W

struct pst_static stat_info;
struct pst_dynamic dyn_info;

int dev_console_device = 0;
int system_console_device = 0;

#define	DIV60(t)	((t+30)/60)    /* x/60 rounded */ 
#define	TTYEQ		((tty == pr[i].w_tty) || \
	((tty == dev_console_device) && (pr [i].w_tty == system_console_device)))
#define IGINT		(1+3*1)		/* ignoring both SIGINT & SIGQUIT */

char	*getargs();
char	*ctime();
#if	defined(W) && defined(__hp9000s800)
off_t	vtophys();
#endif
#ifdef hpux
#define rindex strrchr 
#endif

#define strcmpn strncmp
#define strcatn strncat

char	*rindex();
FILE	*popen();
struct	tm *localtime();

int	debug;			/* true if -d flag: debugging output */
int	header = 1;		/* true if -h flag: don't print heading */
int	lflag = 1;		/* true if -l flag: long style output */
int	login;			/* true if invoked as login shell */
int	idle;			/* number of minutes user is idle */
int	nusers;			/* number of users logged in now */
#ifdef W
char *	sel_user;		/* login of particular user selected */
#endif W
char firstchar;			/* first char of name of prog invoked as */
#ifdef W
time_t	jobtime;		/* total cpu time visible */
#endif W
time_t	now;			/* the current time of day */
struct	tm *nowt;		/* current time as time struct */
struct	timeval boottime;
time_t	uptime;			/* time of last reboot & elapsed time since */
#ifdef W
int	np = 0;			/* number of processes currently active */
#endif W
struct	utmp utmp;
#ifdef W
#endif W

main(argc, argv)
	char **argv;
{
	int days, hrs, mins;
	register int i, j;
	char *cp;
	register int curpid, empty;
	char obuf[BUFSIZ];
	struct stat statbuf;
	int ret;

#if defined(W) && defined(SecureWare)
	if (ISSECURE)
		w_init(argc, argv);
#endif
	setbuf(stdout, obuf);
	login = (argv[0][0] == '-');
	cp = rindex(argv[0], '/');
	firstchar = login ? argv[0][1] : (cp==0) ? argv[0][0] : cp[1];
	cp = argv[0];	/* for Usage */

#ifdef W
	while (argc > 1) {
		if (argv[1][0] == '-') {
			for (i=1; argv[1][i]; i++) {
				switch(argv[1][i]) {

				case 'd':
					debug++;
					break;

				case 'h':
					header = 0;
					break;

				case 'l':
					lflag++;
					break;

				case 's':
					lflag = 0;
					break;

				case 'u':
				case 'w':
					firstchar = argv[1][i];
					break;

				default:
					printf("Bad flag %s\n", argv[1]);
					printf("Usage: %s [ -hlsuw ] [ user ]\n", cp);
					exit(1);
				}
			}
		} else {
			if (!isalnum(argv[1][0]) || argc > 2) {
				printf("Usage: %s [ -hlsuw ] [ user ]\n", cp);
				exit(1);
			} else
				sel_user = argv[1];
		}
		argc--; argv++;
	}
#endif

#if defined(TRUX) && defined(B1)
	if (ISB1)
		(void) forcepriv(SEC_OWNER);
#endif

	ret = pstat(PSTAT_STATIC, &stat_info, sizeof(stat_info), 0, 0);

#if defined(TRUX) && defined(B1)
	if(ISB1)
		(void) disablepriv(SEC_OWNER);
#endif

	if (ret < 0) {
	    perror ("uptime: Could not get pstat static info: ");
	    exit (1);
	}
	system_console_device = makedev(stat_info.console_device.psd_major,
					stat_info.console_device.psd_minor);
	ret = stat("/dev/console", &statbuf);
	if (ret < 0)
		dev_console_device = 0;
	else
		dev_console_device = statbuf.st_rdev;


#ifdef W
	if (firstchar != 'u') {
#ifdef NOT_DEFINED
		if ((kmem = open("/dev/kmem", 0)) < 0) {
			perror("/dev/kmem");
			exit(1);
		}
#endif NOT_DEFINED
	}
#endif W

	if ((ut = fopen("/etc/utmp","r")) == NULL) {
		perror("/etc/utmp");
		exit(1);
	}

	if (header) {
		/* Print time of day */
		time(&now);
		nowt = localtime(&now);
		prtat(nowt);

		/*
		 * Print how long system has been up.
		 * (Found by looking for "boottime" in kernel)
		 */
		boottime.tv_sec = stat_info.boot_time;

		uptime = now - boottime.tv_sec;
		uptime += 30;
		days = uptime / (60*60*24);
		uptime %= (60*60*24);
		hrs = uptime / (60*60);
		uptime %= (60*60);
		mins = uptime / 60;

		printf("  up");
		if (days > 0)
			printf(" %d day%s,", days, days>1?"s":"");
		if (hrs > 0 && mins > 0) {
			printf(" %2d:%02d,", hrs, mins);
		} else {
			if (hrs > 0)
				printf(" %d hr%s,", hrs, hrs>1?"s":"");
			if (mins > 0)
				printf(" %d min%s,", mins, mins>1?"s":"");
		}

		/* Print number of users logged in to system */
		while (fread((char *)&utmp, sizeof(utmp), 1, ut)) {
			if ((utmp.ut_name[0] != '\0')
			    && (utmp.ut_type == USER_PROCESS))
				nusers++;
		}
		rewind(ut);
		printf("  %d user%s", nusers, (nusers == 1) ? "" : "s");

		/*
		 * Print 1, 5, and 15 minute load averages.
		 * (Found by looking in kernel for avenrun).
		 */

#if defined(TRUX) && defined(B1)
		if (ISB1)
			(void) forcepriv(SEC_OWNER);
#endif
		ret = pstat(PSTAT_DYNAMIC, &dyn_info, sizeof(dyn_info), 0, 0);

#if defined(TRUX) && defined(B1)
		if(ISB1)
			(void) disablepriv(SEC_OWNER);
#endif

		if (ret < 0) {
			perror ("uptime: Could not get pstat dynamic info: ");
			exit (1);
		}
		avenrun [0] = dyn_info.psd_avg_1_min;
		avenrun [1] = dyn_info.psd_avg_5_min;
		avenrun [2] = dyn_info.psd_avg_15_min;
		printf(",  load average:");
		for (i = 0; i < (sizeof(avenrun)/sizeof(avenrun[0])); i++) {
			if (i > 0)
				printf(",");
			printf(" %.2f", avenrun[i]);
		}
		printf("\n");
		if (firstchar == 'u')
			exit(0);

#ifdef W
		/* Headers for rest of output */
		if (lflag)
			printf("User     tty           login@  idle   JCPU   PCPU  what\n");
		else
			printf("User    tty  idle  what\n");
		fflush(stdout);
#endif W
	}


#ifdef W
	readpr();
	
	for (;;) {	/* for each entry in utmp */
		if (!fread((char *)&utmp, sizeof(utmp), 1, ut)) {
			fclose(ut);
			exit(0);
		}
		if (utmp.ut_name[0] == '\0')
			continue;	/* that tty is free */
		if (sel_user && strcmpn(utmp.ut_name, sel_user, NMAX) != 0)
			continue;	/* we wanted only somebody else */
#if	defined(__hp9000s800)  ||  defined(__hp9000s300)
		if (utmp.ut_type != USER_PROCESS) continue;
#endif
#ifdef SecureWare
		if (ISSECURE && !w_print_entry(utmp.ut_line))
			continue;
#endif
		gettty();
		jobtime = 0;
		proctime = 0;
		strcpy(doing, "-");	/* default act: normally never prints */
		empty = 1;
		curpid = -1;
		idle = findidle();
		for (i=0; i<np; i++) {	/* for each process on this tty */
			if (!(TTYEQ)) {
				continue;
			}
			jobtime += pr[i].w_time + pr[i].w_ctime;
			proctime += pr[i].w_time;
			if (debug) {
				printf("\t\t%d\t%s", pr[i].w_pid, pr[i].w_comm);
				if ((j=pr[i].w_igintr) > 0)
					if (j==IGINT)
						printf(" &");
					else
						printf(" & %d %d", j%3, j/3);
				printf("\n");
			}
			if (empty && pr[i].w_igintr!=IGINT) {
				empty = 0;
				curpid = -1;
			}
			if(pr[i].w_pid>curpid && (pr[i].w_igintr!=IGINT || empty)){
				curpid = pr[i].w_pid;
				strcpy(doing, pr[i].w_comm);
#ifdef notdef
				if (doing[0]==0 || doing[0]=='-' && doing[1]<=' ' || doing[0] == '?') {
					strcat(doing, " (");
					strcat(doing, pr[i].w_comm);
					strcat(doing, ")");
				}
#endif
			}
		}
		putline();
	}

#endif W
}

#ifdef W
/* figure out the major/minor device # pair for this tty */
gettty()
{
	char ttybuf[20];
	struct stat statbuf;

	ttybuf[0] = 0;
	strcpy(ttybuf, "/dev/");
	strcat(ttybuf, utmp.ut_line);
	stat(ttybuf, &statbuf);
	tty = statbuf.st_rdev;
}

/*
 * putline: print out the accumulated line of info about one user.
 */
putline()
{
	register int tm;

	/* print login name of the user */
	printf("%-*.*s ", NMAX, NMAX, utmp.ut_name);

	/* print tty user is on */
	if (lflag)
		/* long form: all (up to) LMAX chars */
		printf("%-*.*s", LMAX, LMAX, utmp.ut_line);
	else {
		/* short form: 2 chars, skipping 'tty' if there */
		if (utmp.ut_line[0]=='t' && utmp.ut_line[1]=='t' && utmp.ut_line[2]=='y')
			printf("%-2.2s", &utmp.ut_line[3]);
		else
			printf("%-2.2s", utmp.ut_line);
	}

	if (lflag)
		/* print when the user logged in */
		prtat(localtime(&utmp.ut_time));

	/* print idle time */
	prttime(idle," ");

	if (lflag) {
		/* print CPU time for all processes & children */
		prttime(jobtime," ");
		/* print cpu time for interesting process */
		prttime(proctime," ");
	}

	/* what user is doing, either command tail or args */
	printf(" %-.80s\n",doing);
	fflush(stdout);
}

/* find & return number of minutes current tty has been idle */
findidle()
{
	struct stat stbuf;
	long lastaction, diff;
	char ttyname[20];

	strcpy(ttyname, "/dev/");
	strcatn(ttyname, utmp.ut_line, LMAX);
	stat(ttyname, &stbuf);
	time(&now);
	lastaction = stbuf.st_atime;
	diff = now - lastaction;
	diff = DIV60(diff);
	if (diff < 0) diff = 0;
	return(diff);
}
#endif W

/*
 * prttime prints a time in hours and minutes.
 * The character string tail is printed at the end, obvious
 * strings to pass are "", " ", or "am".
 */
prttime(tim, tail)
	time_t tim;
	char *tail;
{
	register int didhrs = 0;

	if (tim >= 60) {
		printf("%3d:", tim/60);
		didhrs++;
	} else {
		printf("    ");
	}
	tim %= 60;
	if (tim > 0 || didhrs) {
		printf(didhrs&&tim<10 ? "%02d" : "%2d", tim);
	} else {
		printf("  ");
	}
	printf("%s", tail);
}

/* prtat prints a 12 hour time given a pointer to a time of day */
prtat(p)
	struct tm *p;
{
	register int t, pm;

	t = p -> tm_hour;
	pm = (t > 11);
	if (t > 11)
		t -= 12;
	if (t == 0)
		t = 12;
	prttime(t*60 + p->tm_min, pm ? "pm" : "am");
}

#ifdef W
/*
 * readpr finds and reads in the array pr, containing the interesting
 * parts of the proc and user tables for each live process.
 */
readpr()
{
	int pn, mf, addr, c, count;
	int szpt, pfnum, i;
#ifdef  __hp9000s800
	long swapoffset;
	struct user *kuptr;
	dev_t		cons_mux_dev; /* alias major/minor for console	   */
#else
	struct pte *Usrptma, *usrpt, *pte, apte;
#endif /* __hp9000s800 */
	int active_procs = 0;
	int idx = 0;
	long *pidlist;
	struct pst_status *pst;
	struct pst_status pst_status_buf;
	int ignore2;
	int ignore3;
	int ret;
	struct proc * get_proc_structure ();
	struct pst_status ps[PSBURST];

#ifdef	notdef
	/*
	 * read mem to find swap dev.
	 */
	lseek(kmem, (long)nl[X_SWAPDEV].n_value, 0);
	read(kmem, &nl[X_SWAPDEV].n_value, sizeof(nl[X_SWAPDEV].n_value));
#endif

	mproc = &proc_buf;
	np = 0;
	pr = NULL;

#if defined(TRUX) && defined(B1)
	if (ISB1)
		(void) forcepriv(SEC_OWNER);
#endif

	while ((count = pstat(PSTAT_PROC, ps, sizeof(struct pst_status), PSBURST, idx)) > 0) {
		active_procs += count;
#ifdef SecureWare
/* original code has a bug -- data is changed past
   the end of this allocation.  I added 10K to it until
   you determine the correct size to allocate. */
		pr = (struct pr *) realloc(pr, 10000 + active_procs * sizeof (struct pr));
#else
		pr = (struct pr *) realloc(pr, active_procs * sizeof (struct pr));
#endif
		if (pr == NULL) {
			fprintf (stderr, "Could not allocate pr array.\n");
			exit(1);
		}


		/* 
		 * Changed the following For loop to only loop
		 * for COUNT times rather than active_procs time
		 *
		 * This is the defect I think that the ifdef SECUREWARE
		 * above is trying to solve, but the realloc is
		 * correct. -- pjw
		 */

		for (pn=0; pn< count ; pn++) {

			if (ps[pn].pst_term.psd_major == -1 && ps[pn].pst_term.psd_minor == -1)
				continue;
	
			/* decide if it's an interesting process */
			if (ps[pn].pst_stat==0 || ps[pn].pst_pgrp==0)
				continue;


#ifdef NOTUSED
			                      pid,        pstat_ptr
						  proc_ptr user_ptr command_ptr
			ret = get_prinfo (ps[pn].pst_pid, NULL, mproc, &up, NULL);
			if (ret <= 0) {
				fprintf (stderr, "get_prinfo returned %d.\n", ret);
				continue;
			}

#endif

			/* save the interesting parts */
			pr[np].w_pid = ps[pn].pst_pid;
			pr[np].w_flag = ps[pn].pst_flag;
			pr[np].w_size = ps[pn].pst_dsize + ps[pn].pst_ssize;

			/* figure out which type of process we are */
			
			ignore2 = (int) ((mproc->p_sigignore & 2) == 2);
			ignore3 = (int) ((mproc->p_sigignore & 4) == 4);
			pr[np].w_igintr =
				ignore2 +
			    2*((int) (!ignore2 && ((mproc->p_sigcatch & 2) == 2))) +
				3*(ignore3) +
			    6*((int) (!ignore3 && ((mproc->p_sigcatch & 4) == 4)));
			pr[np].w_time = ps[pn].pst_utime + ps[pn].pst_stime;

			/* get time of  other child processes on this tty*/
			pr[np].w_ctime =
			    up.u_cru.ru_utime.tv_sec +
				up.u_cru.ru_stime.tv_sec;

			
			pr[np].w_tty = makedev(ps[pn].pst_term.psd_major,
					       ps[pn].pst_term.psd_minor);

			strncpy (pr[np].w_comm, ps[pn].pst_cmd, PST_CLEN);
			np++;
		}
		idx = ps[count - 1].pst_idx + 1;
	}

#if defined(TRUX) && defined(B1)
	if(ISB1)
		(void) disablepriv(SEC_OWNER);
#endif

}

#endif W

