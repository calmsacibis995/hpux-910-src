/* $Source: /misc/source_product/9.10/commands.rcs/bin/ps/top.c,v $
 * $Revision: 70.3.1.1 $            $Author: hmgr $
 * $State: Exp $               $Locker:  $
 * $Date: 95/01/06 10:20:47 $
 */
#ifndef lint
static char *HPUX_ID = "@(#) top.c  $Revision: 70.3.1.1 $ $Date: 95/01/06 10:20:47 $";
#endif

/* 
 * This file contains the source for top, a screen-oriented process display
 * program.  Top displays information about processes in the system in sorted
 * order of their CPU usage.  It also displays information about the memory
 * and scheduling usage of the system. This code works on all HP platforms
 * including the multiprocessing platform. For now all S800 systems will have
 * MP flag defined, the ideal would be to be run-time smart. 
 */

#include <sys/stdsyms.h>
#if  defined(__hp9000s800) && !defined(_WSIO)
#define MP
#endif /* ! _WSIO */

#include "hp9000.h"
#define Default_TOPN   10
#define Default_DELAY   5
#define STDOUT	1
#define TOP_CCPU     0.95122942450071400909 /* Will eventually be in param.h */
#ifdef hp9000s800
#ifdef MAKE_TIMING_STATS
#include "./asm.h"
#endif MAKE_TIMING_STATS
#endif hp9000s800

#ifdef BUILD_FROM_H
#include <h/types.h>
#include <h/unistd.h>
#include <curses.h>
#include <termio.h>
#include <stdio.h>
#include <pwd.h>
#include <h/signal.h>
#include <h/param.h>
#include <ndir.h>
#include <h/user.h>
#include <h/proc.h>
#include <h/dk.h>
#include <h/vm.h>
#include <h/utsname.h>
#include <h/stat.h>
#include <h/sysmacros.h>
#include <h/pstat.h>
#include <h/fcntl.h>
#ifdef NOTDEF
#ifdef FOUR_ONE
#include <h/pte.h>
#else
#include <machine/pte.h>
#endif
#endif NOTDEF

#else  BUILD_FROM_H

#include <sys/types.h>
#include <sys/unistd.h>
#include <curses.h>
/* termio needs to be included in 8.0 */
#include <sys/termio.h>
#include <stdio.h>
#include <pwd.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <ndir.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/dk.h>
#include <sys/vm.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/pstat.h>
#include <sys/fcntl.h>
#ifdef NOTDEF
#ifdef FOUR_ONE
#include <sys/pte.h>
#else
#include <machine/pte.h>
#endif
#endif NOTDEF

#endif BUILD_FROM_H


#ifdef hpux
#define rindex strrchr
#endif hpux
#include <nlist.h>

/*
 *  The number of users in /etc/passwd CANNOT be greater than Table_size!
 *  If you get the message "table overflow: too many users", then increase
 *  Table_size.  Since this is the size of a hash table, things will work
 *  best if it is a prime number that is about twice the number of users.
 */

#define Table_size	853
/*
 * The number of process being read in at pstat call
 */
#define PS_CHUNK	10

/* Number of lines of header information on the standard screen */
/* This used to be (should be) 6, but the number of cpu states has increased
 * to the point where they take up two lines.
 * Added one more line for load. 7 + 1
 */
# define Header_lines	8

/* convert kernel page to K bytes */
/* pagetok(x) (((x) << PGSHIFT) / 1024) */
#define pagetok(x) (((x) << PGSHIFT) >> 10)

/* convert kernel page to M bytes */
/* #define pagetoM(x) ((((x) << PGSHIFT) / 1024) / 1024) */

/* useful externals */
extern int errno;
extern char *sys_errlist[];
extern int LINES;
dev_t	cons_dev;


/* signal handling routines */
int leave();
int onalrm();

/* other functions */
struct passwd *getpwent();
char *username();
char *get_tty ();
char *gettty ();
char *ctime();
char *rindex();
char *malloc();
int proc_compar();
double log();
double exp();
char *state_name();

/* cpu state names for percentages */
char *cpu_state[] = {
    "user", "nice", "system", "idle", "wait"
};

/* routines that don't return int */
struct proc_and_offset {
    struct pst_status *proc_ptr;
};
struct termio tbufsave;
int malloc_flag =0;
struct termio old_termio;
int arguement;
main(argc, argv)

int  argc;
char **argv;

{
    register struct pst_status *pp;
    register struct proc_and_offset *prefp;
    register int i;
    register int active_procs;
    register int change;
    register long cputime;

    struct proc_and_offset *pref;
    int total_procs;
    int proc_brkdn[7];
    int topn = Default_TOPN;
    int delay = Default_DELAY;
    int new_topn=0;
    int count;
    int apcount;
    int nproc;
    int bytes;
    int limit;
    int screeno = 0;
    long hz;          /* kernel "hz" variable -- clock rate */
    long curr_time;
    long time();
    long *pidptr;
    long *lptr;
    int index=0;
    int process_id=0;

    /* All this is to calculate the cpu state percentages */
    int   cpustates;
    int   known_cpustates;
    long *cp_time;
    long *cp_old;
    long *cp_change;
    long  total_change;
#ifdef MP 
    long mcp_time[PST_MAX_PROCS][PST_MAX_CPUSTATES];
    long mcp_old[PST_MAX_PROCS][PST_MAX_CPUSTATES];
    long mcp_change[PST_MAX_PROCS][PST_MAX_CPUSTATES];
    long mp_total_change[PST_MAX_PROCS];
    int  runningprocs;
#endif
    char do_cpu = 1;
    char cmd;
    char *myname = "top";
    char *lines_string;
    double pctcpu;
    double ccpu = TOP_CCPU; /*  Will eventually be in param.h  */
    double logcpu;
    int idx=0; /* index in pstat call */
    int total_count=0;
    int pid_count =0;
    int pid_index=0;
    int sorted_pid=0;
    int j =0;
    int	initial =0;
    int pid_table[300],ret=0;
    
    /* For the new options included*/
    int displays=0,display_flag=0;
    int display_uid=0;

    struct utsname name;
    /* Due to changes in pstat.h */
    struct pst_static system_info;
    struct pst_dynamic dynamic_info;

    struct pst_status *pstat_base;
    struct pst_status *pstat_ptr;
    struct pst_status pid_list[PS_CHUNK];
#ifndef	MY_OWN_RAW
    long temp;
#endif	MY_OWN_RAW

    /* get our name */
#ifndef spectrum
    if (argc > 0)
    {
	if ((myname = rindex(argv[0], '/')) == 0)
	    myname = argv[0];
	else
	    myname++;
    }
#endif
	initscr();
        new_topn = LINES - Header_lines;
        if ((new_topn > 0) && (new_topn < 1000)) {
            topn = new_topn;
        }
arguement = argc;
    for (i=1; --arguement< 0, *argv[i] == '-'; i++)
    {
	switch(*(argv[i]+1))
	{
	case 'u':
		display_uid =1;
		break;
	case 'd':
		if ((displays =atoi(argv[i]+2)) <= 0)
		{ 
		fprintf(stderr,"Quiting Top: display count should be greater than zero\n");
		system("tset -Q"); /* Q option will Suppress printing 
				      the "Erase set to" and "Kill set to"
                   		       messages. */
		exit(1);
		}
		else
		{
		display_flag = 1;
 		ioctl(STDOUT, TCGETA, &old_termio);
		}
		break;
	case 's':
		if ((delay =atoi(argv[i]+2)) <= 0)
		{ 
		fprintf(stderr,"Quiting Top: update interval should be greater than zero\n");
		system("tset -Q");
		exit(1);
		}
		break;
	case 'q':
		if (getuid() == 0)
		{
		(void) nice(-20);
		}
		else
		{
		fprintf(stderr,
			"%s: warning: '-q' option can only be used by root \n",
			 myname);
		sleep(3);
		}
		break;
	case 'n':
		if ((topn =atoi(argv[i]+2)) <= 0)
		{
			fprintf(stderr,
                        "Quiting Top: Number must be greater than zero\n");
			system("tset -Q");
			exit(1);
		}
		break;
	default: 
	fprintf(stderr, "Usage:  top [-u] [-q] [-dx] [-sx] [-n number] \n");
		system("tset -Q");
		exit(1);
	}
	
     }

    /* get count of top processes to display (if any) */
/*
    if (argc > 1)
    {
	topn = atoi(argv[1]);
    } else {
        int new_topn;
        new_topn = LINES - Header_lines;
        if ((new_topn > 0) && (new_topn < 1000)) {
            topn = new_topn;
        }
    }
*/
    /* get the symbol values */
    ret = pstat(PSTAT_STATIC,&system_info,sizeof(struct pst_static),0,0);
    pstat(PSTAT_DYNAMIC,&dynamic_info,sizeof(struct pst_dynamic),0,0);
    hz = sysconf (_SC_CLK_TCK);
    if (hz == -1)
	hz = HZ;
    nproc = system_info.max_proc;
    cons_dev = system_info.console_device.psd_major;
    set_console_device (cons_dev);         /* For tty look-up. */
    cpustates = system_info.cpu_states;
#ifdef MP 
    runningprocs = dynamic_info.psd_proc_cnt;
#endif
    if (cpustates < 1)
	cpustates = 1;
	known_cpustates = sizeof (cpu_state) / sizeof (cpu_state [0]);
    cp_time = (long *) malloc (cpustates * sizeof (long));
    cp_old = (long *) malloc (cpustates * sizeof (long));
    cp_change = (long *) malloc (cpustates * sizeof (long));
    if (cp_time == NULL || cp_old == NULL || cp_change == NULL) {
        fprintf (stderr, "malloc of cpu states array failed.  cpustates: %d\n", cpustates);
	exit (30);
    }
    pidptr = (long *)
    malloc (system_info.max_proc * sizeof (long));

/* This is used in the calculation of WCPU. 
 * WCPU is the Weighted CPU that the kernel calculates to use in the
 * scheduling algorithm.  This is also the value that "ps" reports as
 * CPU.  It is a weighted average over the last few minutes.
 *
 * If your scheduling algorithm doesn't use a weighted average, then
 * you really aren't missing much by not reporting it.
 */

#ifdef SUN
    logcpu = log((double)ccpu / FSCALE);
#else
    logcpu = log(ccpu);
#endif


/* 
 * allocate space for proc structure array and array of pointers 
 */
    bytes = nproc * sizeof(struct pst_status);
    pstat_base = (struct pst_status *)malloc(bytes);
    pref  = (struct proc_and_offset *)malloc(nproc * sizeof(struct proc_and_offset ));
    /* Just in case ... */
    if (pstat_base == (struct pst_status *)NULL || pref == (struct proc_and_offset *)NULL)

    {
	fprintf(stderr, "%s: can't allocate sufficient memory\n", myname);
	exit(1);
    }
    get_tty_devices ();

    /* initialize the hashing stuff */
    init_hash();

    /* initialize curses and screen (last) */
    /*initscr();*/ /* Routine shifted for default options to work well */
    /*scrollok(stdscr, 1); */
    /*wsetscrreg(stdscr, 7,24);*/
#ifdef	MY_OWN_RAW
    setraw();
#else	MY_OWN_RAW
    crmode();
#endif	MY_OWN_RAW
    clear();
    refresh();

    /* setup signal handlers */
    signal(SIGINT, leave);
    signal(SIGALRM, onalrm); 
    signal(SIGQUIT, leave);

    /* If the user didn't specify the number of lines to use, set it from
     * the environment variable.
     */
/*
    if (argc <= 1) {
        int new_topn;
        new_topn = LINES - Header_lines;
	printf(" new_topn = %d\n", new_topn);
        if ((new_topn > 0) && (new_topn < 1000)) {
            topn = new_topn;
        }
    }
*/
    /* can only display (LINES - Header_lines) processes */
    if (topn > LINES - Header_lines)
    {
	printw("Warning: this terminal can only display %d processes...\n",
	    LINES - Header_lines);
	refresh();
	sleep(2);
	topn = LINES - Header_lines;
	clear();
    }
/*
 * get system name.-- only need to do it once
 */
uname(&name);

/* 
 * main loop ...This is an infinite loop unless otherwise specified
 */
    while (1)
    {
loop:

/*
 * This will make the program terminate after the specified(user controlled)
 * number of display of the information. Code for the "d" option.  Note that 
 * the ioctl call restores the terminal related info when the program quits.
 */
if((displays >= 0) && (display_flag ==1))
{
	if(displays ==0)
	{
	ioctl(STDOUT, TCSETA, &old_termio);
	system("tset -Q"); 
	exit(0);
	}
	displays --;
}	
initial = 0;
index = 0;
pid_index =0;
idx =0;
total_count=0;

/*
 * Here we obtain all the processes number and store it in an array to be used
 * later.
 */
while((count = pstat(PSTAT_PROC,pid_list, sizeof(struct pst_status),PS_CHUNK,idx)) > 0 )
{
total_count = total_count + count;
idx = pid_list[count -1].pst_idx+1;
for (j = 1 ; j <= count; j++)
  pid_table[pid_index++] = pid_list[index++].pst_pid;
initial = initial +count;
index=0;
}
apcount = total_count;
pid_count = total_count;
pstat_ptr = pstat_base;
index =0;
for (lptr = pidptr; apcount ; apcount--) {
	process_id = pid_table[index++];
	pstat_status_if (0, process_id, pstat_ptr);
	pstat_ptr++;
	}
pstat(PSTAT_DYNAMIC,&dynamic_info,sizeof(struct pst_dynamic),0,0);
/* count up process states and get pointers to interesting procs */
total_procs = 0;
active_procs = 0;
memset((char *) proc_brkdn, 0, sizeof(proc_brkdn));
prefp = pref;
for (pp = pstat_base, i = 0; i < pid_count; pp++, i++)
{
    /* 
     * place pointers to each valid proc structure in pref[] 
     * (processes with SSYS set are system processes) 
     */
           total_procs++;
           proc_brkdn[pp->pst_stat]++;
           prefp->proc_ptr = pp;
           prefp++;
           active_procs++;
}
	
	/*
	 *  Display the current time.
	 *  "ctime" always returns a string that looks like this:
	 *  
	 *	Sun Sep 16 01:03:52 1973
	 *      012345678901234567890123
	 *	          1         2
	 *
	 *  We want indices 11 thru 18 (length 8).
	 */
/*
        move(0, 79-44);
printw("system: %s", name.nodename);
	curr_time = time(0);
	move(0, 79-8);
	printw("%-8.8s\n", &(ctime(&curr_time)[11]));
*/
	/* Display the system name */

/*        move(0, 79-44); */  /* Start at column zero, line one */

        printw("System: %s", name.nodename);

        /* display the current time */
        curr_time = time(0);
        move(0, 79-26);
        printw("%26s", &(ctime(&curr_time)[0]));

	/* display the load averages */
	printw("Load averages", dynamic_info.psd_last_pid);
	printw(": %4.2f, %4.2f, %4.2f",
#ifdef SUN
	    dynamic_info.load_avg_1_min,
	    dynamic_info.load_avg_5_min,
	    dynamic_info.load_avg_15_min);
	    printw("\n");
#else
	    dynamic_info.psd_avg_1_min,
	    dynamic_info.psd_avg_5_min,
	    dynamic_info.psd_avg_15_min);
	    printw("\n");
#endif

	/* display process state breakdown */
	printw("%d processes", total_procs);
	for (i = 1; i < 7; i++)
	{
	    if (proc_brkdn[i] != 0)
	    {
		printw("%c %d %s%s",
			i == 1 ? ':' : ',',
			proc_brkdn[i],
			state_name(i, 0),
			(i == SZOMB) && (proc_brkdn[i] > 1) ? "s" : "");
	    }
	}


	/* calculate percentage time in each cpu state */
	cpustates = system_info.cpu_states;
	printw("\nCpu states: ");
	if (do_cpu)	/* but not the first time */
	{
	    total_change = 0;
	    for (i = 0; i < cpustates; i++)
	    {
		cp_time [i] = dynamic_info.psd_cpu_time [i];
		/* calculate changes for each state and overall change */
		if (cp_time[i] < cp_old[i])
		{
		    /* this only happens when the counter wraps */
		    change = (int)
			((unsigned long)cp_time[i]-(unsigned long)cp_old[i]);
		}
		else
		{
		    change = cp_time[i] - cp_old[i];
		}
		total_change += (cp_change[i] = change);
	    }
#ifdef MP
}  /* The display of CPU states for MP is coded later */
#endif

/*
 * CPU states for uniprocessor systems
 */
#ifndef MP  
	    for (i = 0; i < cpustates; i++)
	    {
		if (i < known_cpustates)
		{
		   printw("%s%4.1f%% %s",
			i == 0 ? "" : ", ",
		        i == CP_IDLE ? 
		        ((float)(cp_change[i] + cp_change[i+1]) / (float)total_change) *100.0 :
			((float)cp_change[i] / (float)total_change) * 100.0,
			cpu_state[i]);
		} else {
		   printw("%s%4.1f%% %s%d",
			i == 0 ? "" : ", ",
		        i == CP_IDLE ? 
		        ((float)(cp_change[i] + cp_change[i+1]) / (float)total_change) *100.0 :
			((float)cp_change[i] / (float)total_change) * 100.0,
			"unk", i);
		}
		cp_old[i] = cp_time[i];
		if (i == CP_IDLE) {
		        cp_old[i+1] = cp_time[i+1];
			i++;
                }
	    }
	}
	else
	{
	    /* we'll do it next time */
	    for (i = 0; i < cpustates; i++)
	    {
		if (i == CP_IDLE) continue;
		if (i < known_cpustates)
		{
		   printw("%s      %s",
			i == 0 ? "" : ", ",
			cpu_state[i]);
		} else {
		   printw("%s      %s%d",
			i == 0 ? "" : ", ",
			"unk", i);
		}
		cp_old[i] = 0;
	    }
	    do_cpu = 1;
	}
printw("\n");
#endif /* uniprocessor CPU States */


/*
 * CPU states display for MP platform 
 */
#ifdef MP 
printw("\n");
for (i=0; i <= (runningprocs - 1); i++) {
    if (do_cpu) {		 /* but not the first time */
    	mp_total_change[i] = 0;
    	for (j = 0; j < cpustates; j++) {
		mcp_time[i][j] = dynamic_info.psd_mp_cpu_time[i][j];
		/* 
		 *calculate changes for each state and 
		 * overall change
		 */
	    	/* this only happens when the counter wraps */
		if (mcp_time[i][j] < mcp_old[i][j]) {
	    		change = (int) ((unsigned long)mcp_time[i][j] -
					(unsigned long)mcp_old[i][j]);
		} else {
	    		change = mcp_time[i][j] - mcp_old[i][j];
		}
		mp_total_change[i] += (mcp_change[i][j] = change);
    	}
    } else {
        mcp_old[i][j] = 0;
    }
}
/* print the header */
printw("CPU   LOAD   USER   NICE    SYS   IDLE   UNK5   UNK6   INTR   SSYS\n");
	/*
	 * print data for each processor
	 */
   if (runningprocs > 1) {
      for (i=0; i < runningprocs; i++) {

	   /* print the cpu number */
	   printw(" %1d   ", i);
	    /* print the one minute load average */
#ifdef SUN
           printw("%5.2f ", dynamic_info.load_avg_1_min);
#else 
	    /* fix this when per-processor run queues implemented */
	     printw("%5.2f ", dynamic_info.psd_mp_avg_1_min[i]); 
	   /* printw("  %s ", "n/a");*/
#endif
	    /* print cpu states data, but not first time through */
	   if (do_cpu) {

	  	for (j = 0; j < cpustates; j++) {
	    	    if (j < known_cpustates) {
	       		printw("%5.1f%% ",  j == CP_IDLE ? 
	           	   ((float)(mcp_change[i][j] + mcp_change[i][j+1]) / 
	    			(float)mp_total_change[i]) *100.0 :
	    	           ((float)mcp_change[i][j] / 
	    			(float)mp_total_change[i]) * 100.0);
	    	    } else {
	       	        printw("%5.1f%% ", j == CP_IDLE ? 
	            	    ((float)(mcp_change[i][j] + mcp_change[i][j+1]) / 
	    			(float)mp_total_change[i]) *100.0 :
	    		    ((float)mcp_change[i][j] / 
	    			(float)mp_total_change[i]) * 100.0);
	    	    }
	    	    mcp_old[i][j] = mcp_time[i][j];
	    	    if (j == CP_IDLE) {
	            	mcp_old[i][j+1] = mcp_time[i][j+1];
	    		j++;
                    }
	    	}
	    } else { 		/* we'll do it next time */
	    	for (j = 0; j < cpustates-1; j++)
	        		printw(" %s ", "00.0%");
	    }
	    printw("\n");
	}
/* print trailer */
printw("---   ----  -----  -----  -----  -----  -----  -----  -----  -----\n");
}
/* print global processor stats */
/* print "Avg" if running MP, otherwise print "1" */
if (dynamic_info.psd_proc_cnt == 1) {
	printw(" %1d   ", (int)1);
} else {
	printw("%s  ", "avg");
}
/* print the one minute load average */
#ifdef SUN
	printw("%5.2f ", dynamic_info.load_avg_1_min);
#else 
	printw("%5.2f ", dynamic_info.psd_avg_1_min);
#endif

/* print cpu states data, but not first time through */
if (do_cpu) {
    for (i = 0; i < cpustates; i++) {
	if (i < known_cpustates) {
	   	printw("%5.1f%% ",  i == CP_IDLE ? 
	       	   ((float)(cp_change[i] + cp_change[i+1]) / 
			(float)total_change) *100.0 :
	           ((float)cp_change[i] / (float)total_change) * 100.0);
	} else {
	   	printw("%5.1f%% ", i == CP_IDLE ? 
	           ((float)(cp_change[i] + cp_change[i+1]) / 
			(float)total_change) *100.0 :
		   ((float)cp_change[i] / (float)total_change) * 100.0);
	}
	cp_old[i] = cp_time[i];
	if (i == CP_IDLE) {
	       	cp_old[i+1] = cp_time[i+1];
		i++;
               }
    }
} else { 		/* we'll do it next time */
    	for (i = 0; i < cpustates-1; i++) {
        	printw(" %s ", "00.0%");
    	}
}
printw("\n");
do_cpu =1;
#endif /* MP CPU & Processes display */

#ifdef MP
	printw("\n");

	/* display main memory statistics */
/*	printw("Memory: %4dM (%4dM) real, %4dM (%4dM) virtual, %4dM free",
		pagetoM (dynamic_info.psd_rm),
		pagetoM (dynamic_info.psd_arm),
		pagetoM (dynamic_info.psd_vm),
		pagetoM (dynamic_info.psd_avm),
		pagetoM (dynamic_info.psd_free)); */
#endif 

/* #ifndef MP */
	/* display main memory statistics */
	printw("Memory: %4dK (%4dK) real, %4dK (%4dK) virtual, %4dK free",
		pagetok (dynamic_info.psd_rm),
		pagetok (dynamic_info.psd_arm),
		pagetok (dynamic_info.psd_vm),
		pagetok (dynamic_info.psd_avm),
		pagetok (dynamic_info.psd_free));
/* #endif   Not MP */

	printw("     Screen # %d/%d\n", 
	screeno + 1,
	total_procs % topn ? total_procs/topn+1 : total_procs/topn);

	if (topn > 0)
	{
	/*
	 * If display option is set then UID is displayed instead of
	 * USERNAME
	 */
/*
 * Uniprocessor Platform: Display of process information
 * Multiprocessor Platform: Display of CPU and process information
 */
#ifndef MP
	if (display_uid ==1)
	    printw("\nTTY   PID  UID      PRI   NI  SIZE  RES   STATE   TIME   %%WCPU %%CPU  COMMAND\n"); 
	else
	    printw("\nTTY   PID USERNAME  PRI   NI  SIZE  RES  STATE   TIME   %%WCPU  %%CPU  COMMAND\n"); 
	
#else /* MP Code: Header Info*/
if (display_uid ==1)
   printw("\nCPU  TTY   PID UID       PRI NI   SIZE   RESD STATE   TIME %%WCPU  %%CPU COMMAND\n"); 
else
   printw("\nCPU  TTY   PID USERNAME  PRI NI   SIZE   RESD STATE   TIME %%WCPU  %%CPU COMMAND\n"); 
#endif 

 /* sort by cpu percentage (pctcpu) */
 qsort(pref, pid_count, sizeof(struct proc_and_offset ), proc_compar);
 limit = active_procs;
 i = screeno * topn; 

  for (prefp = &pref[i] ; (i < limit) && (i <= (topn * (screeno +1)) -1); prefp++, i++)
    {
	pp = prefp->proc_ptr;
	cputime = 
#ifdef FOUR_ONE
	((float)(pp->pst_utime + pp->pst_stime)/hz);
#else
	(pp->pst_utime + pp->pst_stime);
#endif
#ifdef SUN
	pctcpu = (double)pp->pst_pctcpu / FSCALE;
#else
	pctcpu = pp->pst_pctcpu;
#endif
#ifdef DEBUG
	fprintf (stderr, "pid is %d\n", pp->pst_pid);
#endif

#ifdef SUN
	logcpu = log((double)ccpu/FSCALE);
#else
	logcpu = log(ccpu);
#endif

#ifndef MP /* Uniprocessor Code for process information */
	if (display_uid ==1)
		printw("%-4.4s %5d %-8d%3d %4d%6dK %4dK  %-5s%4d:%02d  %5.2f %5.2f %-10.10s\n",
		    gettty(pp->pst_major,pp->pst_minor), 
		    pp->pst_pid, 
		    pp->pst_uid,
		    pp->pst_pri,
		    pp->pst_nice, 
#ifdef hp9000s800
		pagetok((pp->pst_tsize) + (pp->pst_dsize) + (pp->pst_ssize)),
		 pagetok(pp->pst_rssize),
#else
		    (pp->pst_tsize + pp->pst_dsize + pp->pst_ssize) << 2,
		   pp->pst_rssize << 2,
#endif

		    state_name(pp->pst_stat, 1),
		    cputime / 60l,
		    cputime % 60l,
		    pp->pst_time == 0 ? 0.0 :
		(100.0 * pctcpu / (1.0 - exp(pp->pst_time * logcpu))),
		(100.0 * pctcpu),
		    pp->pst_cmd); 
	else
		printw("%-4.4s %5d %-8.8s%3d %4d%6dK %4dK  %-5s%4d:%02d  %5.2f  %5.2f %-10.10s\n",
		    gettty(pp->pst_major,pp->pst_minor), 
		    pp->pst_pid, 
		    username(pp->pst_uid)  ,
		    pp->pst_pri,
		    pp->pst_nice, 
#ifdef hp9000s800
		  pagetok((pp->pst_tsize) + (pp->pst_dsize) + (pp->pst_ssize)),
 		  pagetok(pp->pst_rssize),
#else
		    (pp->pst_tsize + pp->pst_dsize + pp->pst_ssize) << 2,
 		    pp->pst_rssize << 2,
#endif
                    state_name(pp->pst_stat, 1),
		    cputime / 60l,
		    cputime % 60l,
		    pp->pst_time == 0 ? 0.0 :
		(100.0 * pctcpu / (1.0 - exp(pp->pst_time * logcpu))),
		(100.0 * pctcpu),
		    pp->pst_cmd); 

#else /* MP Code: Process information */

if (display_uid ==1)
	printw(" %1d  %4.4s %5d %-8d  %3d %2d %5dK %5dK %-5s %3d:%02d %5.2f %5.2f %-8.8s\n",
	    pp->pst_procnum ,
	    gettty(pp->pst_major,pp->pst_minor), 
	    pp->pst_pid, 
	    pp->pst_uid,
	    pp->pst_pri,
	    pp->pst_nice, 
#ifdef hp9000s800
             pagetok((pp->pst_tsize) + (pp->pst_dsize) + (pp->pst_ssize)),
 	     pagetok(pp->pst_rssize),
#else
	    (pp->pst_tsize + pp->pst_dsize + pp->pst_ssize) << 2,
	     pp->pst_rssize << 2,
#endif
	    state_name(pp->pst_stat, 1),
	    cputime / 60l,
	    cputime % 60l,
	    pp->pst_time == 0 ? 0.0 :
		(100.0 * pctcpu / (1.0 - exp(pp->pst_time * logcpu))),
		/*pp->pst_cpu,*/
		(100.0 * pctcpu),
	    pp->pst_cmd); 
else
	printw(" %1d  %4.4s %5d %-8.8s  %3d %2d %5dK %5dK %-5s %3d:%02d %5.2f %5.2f %-8.8s\n",
	      pp->pst_procnum ,
	    gettty(pp->pst_major,pp->pst_minor), 
	    pp->pst_pid, 
	    username(pp->pst_uid)  ,
	    pp->pst_pri,
	    pp->pst_nice, 
#ifdef hp9000s800
             pagetok((pp->pst_tsize) + (pp->pst_dsize) + (pp->pst_ssize)),
 	     pagetok(pp->pst_rssize),
#else
		    (pp->pst_tsize + pp->pst_dsize + pp->pst_ssize) << 2,
	       pp->pst_rssize << 2,
#endif
	    state_name(pp->pst_stat, 1),
	    cputime / 60l,
	    cputime % 60l,
	    pp->pst_time == 0 ? 0.0 :
		(100.0 * pctcpu / (1.0 - exp(pp->pst_time * logcpu))),
		/*pp->pst_cpu,*/
		(100.0 * pctcpu),
	    pp->pst_cmd); 

#endif /* MP Code & UP Code */

	    }
	}
	refresh();
#ifndef	MY_OWN_RAW
	if (ioctl(0, FIONREAD, &temp) != -1 && temp > 0)
#endif	MY_OWN_RAW
	i = read(0, &cmd, 1);
	if (i == 1) {
		switch (cmd) {
			case 'j': if (!((screeno+1) * topn >= active_procs)) {
					screeno++;
				  	move(0, 0);
				  	goto loop;
				  }
				  break;
			case 'k': if (screeno > 0) screeno--;
				  move(0, 0);
				  goto loop;
				  /*kill (getpid(), SIGALRM); 
				  break; */
			case 't': screeno = 0; 
				  move(0, 0);
				  goto loop;
				  /* kill (getpid(), SIGALRM); */
				  break;
			case 'q': quit(0);
#ifndef	SIGTSTP
			case 26 : erase();
				  refresh();
				  unsetraw();
				  kill(getpid(), SIGTSTP);
				  setraw();
				  break; 
#endif	SIGTSTP
			default: i = i; break;
		}
	}
	/* wait ... */
	/*signal(SIGALRM, onalrm); */
	alarm(delay);
	pause();

	/* clear for new display */
	erase(); 
/*	clear(); */
    }
}

/*
 *  signal handlers
 */

leave()			/* exit under normal conditions -- INT handler */

{
    move(LINES - 1, 0);
    deleteln();			/* DSDe407107 */
    refresh();
#ifdef	MY_OWN_RAW
    unsetraw();
#endif	MY_OWN_RAW
    endwin();
    exit(0);
}

quit(status)		/* exit under duress */

int status;

{
#ifdef	MY_OWN_RAW
    unsetraw();
#endif	MY_OWN_RAW
    deleteln();			/* DSDe407107 */
    refresh();			/* DSDe407107 */
    endwin();
	pstat_statistic_summary ();
    exit(status);
}

onalrm()

{
    signal(SIGALRM, onalrm);
    return(0);
}

/*
 *  comparison function for "qsort"
 *  Do first order sort based on cpu percentage computed by kernel and
 *  second order sort based on number of cpu ticks the process has taken.
 */
 
proc_compar(p1, p2)

register struct pst_status **p1;
register struct pst_status **p2;

{
    if ((*p1)->pst_pctcpu < (*p2)->pst_pctcpu)
    {
        return(1);
    }   
    else if ((*p1)->pst_pctcpu == (*p2)->pst_pctcpu)
    {
        if ((*p1)->pst_cpticks < (*p2)->pst_cpticks)
        {
            return(1);
        }
        else
        {
            return(-1);
        }
    }
    else
    {
        return(-1);
    }
}

/*
 *  These routines handle uid to username mapping.
 *  They use a hashing table scheme to reduce reading overhead.
 */

struct hash_el {
    int  u_id;
    char name[8];
};

#define    H_empty	-1

/* simple minded hashing function */
#define    hashit(i)	((i) % Table_size)

struct hash_el hash_table[Table_size];

init_hash()

{
    register int i;
    register struct hash_el *h;

    for (h = hash_table, i = 0; i < Table_size; h++, i++)
    {
	h->u_id = H_empty;
    }

    setpwent();
}

char *username(uid)

register int uid;

{
    int index, x;
    register int found;
    register char *name;

    /* This is incredibly naive, but it'll probably get changed anyway */
    index = hashit(uid);
    while ((found = hash_table[index].u_id) != uid)
    {
#ifdef DEBUG
    fprintf (stderr,"uid is %d\n", uid);
    fprintf(stderr,"hash_table entry is %d \n", hash_table[index].u_id); 
#endif
	if (found == H_empty)
	{
	    /* not here -- get it out of passwd */
	    index = get_user(uid);
	    break;		/* out of while */
	}
	index = ++index % Table_size;
#ifdef DEBUG
	fprintf(stderr, "index is %d found is %d ", index, found);
#endif
    }
    return(hash_table[index].name);
}

enter_user(uid, name)

register int  uid;
register char *name;

{
    register int length;
    register int index;
    register int attempt;
    static int uid_count = 0;

    /* avoid table overflow -- insure at least one empty slot */
    if (++uid_count >= Table_size)
    {
	fprintf(stderr, "table overflow: too many users\n");
	quit(1);
    }

    index = hashit(uid);
    while ((attempt = hash_table[index].u_id) != H_empty)
    {
	if (attempt == uid)
	{
	    return(index);
	}
	index = ++index % Table_size;
    }
    hash_table[index].u_id = uid;
    strncpy(hash_table[index].name, name, 8);
    return(index);
}

get_user(uid)

register int uid;

{
    struct passwd *pwd;
    static char buff[20];
    register int last_index;

    while ((pwd = getpwent()) != NULL)
    {
	last_index = enter_user(pwd->pw_uid, pwd->pw_name);
	if (pwd->pw_uid == uid)
	{
	    return(last_index);
	}
    }
    sprintf(buff, "%d", uid);
    return(enter_user(uid, buff));
}

char *
gettty (pointer, device)
	long pointer;
	long device;
{
	char * cptr;

	cptr = get_tty (pointer, device);
	if ((cptr [0] == 't') && (cptr [2] == 'y'))
		cptr += 3;
	return (cptr);
}
	


/*
 *  outc - real subroutine that writes a character; for termcap
 */

outc(ch)

register char ch;

{
    putchar(ch);
}


#ifdef	MY_OWN_RAW
setraw()	/* set terminal I/O read is satisified after 1 char */
{
	struct termio tbuf;

	if (ioctl(0, TCGETA, &tbuf) == -1) {
		printf ("errror in ioctl\n");
		endwin();
		exit(-1);
	}
	tbufsave = tbuf;
	tbuf.c_iflag &= ~(INLCR | ICRNL | IUCLC | ISTRIP | IXON );
	tbuf.c_oflag &= ~OPOST;
	tbuf.c_lflag &= ~(ICANON | ECHO);
	tbuf.c_cc[5] = 0; /* TIME */
	tbuf.c_cc[4] = 0;  /* MIN */
	if (ioctl(0, TCSETAF, &tbuf) == -1) {
		printf("error in TSETAF1\n");
		endwin();
		exit(-1);
	}
}

unsetraw()
{
	if (ioctl(0, TCSETAF, &tbufsave) == -1) {
		printf("error in TSETAF1\n");
		endwin();
		exit(-1);
	}
}
#endif	MY_OWN_RAW

#ifdef hp9000s800
#ifdef MAKE_TIMING_STATS

int min_open = 0xFFFFFFF;
int max_open = 0;
int sum_open = 0;
int sum_open_secs = 0;

int min_close = 0xFFFFFFF;
int max_close = 0;
int sum_close = 0;
int sum_close_secs = 0;

int min_norm = 0xFFFFFFF;
int max_norm = 0;
int sum_norm = 0;
int sum_norm_secs = 0;

int min_pstat = 0xFFFFFFF;
int max_pstat = 0;
int sum_pstat = 0;
int sum_pstat_secs = 0;
int pstat_counter = 0;

pstat_status_if (command, pid, pstat_ptr)
    int command;
    int pid;
    struct pst_status *pstat_ptr;
{
	register int time0;
	register int time1;
	register unsigned int diff;
	register int ret;
	register int modulo;
	register int fd;
	_register(time0,time1);
	

	_MFCTL(16,time0);
	fd = open ("/dev/kmem", O_RDONLY);
	_MFCTL(16,time1);
	if (fd < 0) {
		fprintf (stderr, "Open failed, Errno %d.\n", errno);
		fflush (stdout);
	}
	diff = (unsigned)(time1) - (unsigned)(time0);
	if (diff < min_open)
		min_open = diff;
	if (diff > max_open)
		max_open = diff;
	sum_open += diff;
	modulo = sum_open / 8000000;
	sum_open_secs += modulo;
	sum_open = sum_open % 8000000;

	_MFCTL(16,time0);
	ret = close (fd);
	_MFCTL(16,time1);
	diff = (unsigned)(time1) - (unsigned)(time0);
	if (diff < min_close)
		min_close = diff;
	if (diff > max_close)
		max_close = diff;
	sum_close += diff;
	modulo = sum_close / 8000000;
	sum_close_secs += modulo;
	sum_close = sum_close % 8000000;

	_MFCTL(16,time0);
	getitimer (3, 0);
	_MFCTL(16,time1);
	diff = (unsigned)(time1) - (unsigned)(time0);
	if (diff < min_norm)
		min_norm = diff;
	if (diff > max_norm)
		max_norm = diff;
	sum_norm += diff;
	modulo = sum_norm / 8000000;
	sum_norm_secs += modulo;
	sum_norm = sum_norm % 8000000;

	_MFCTL(16,time0);
	ret = pstat(PSTAT_PROC,pstat_ptr, sizeof(struct pst_status),0,pid);
	_MFCTL(16,time1);
	diff = (unsigned)(time1) - (unsigned)(time0);
	if (diff < min_pstat)
		min_pstat = diff;
	if (diff > max_pstat)
		max_pstat = diff;
	sum_pstat += diff;
	modulo = sum_pstat / 8000000;
	sum_pstat_secs += modulo;
	sum_pstat = sum_pstat % 8000000;
	diff = (unsigned)(time1) - (unsigned)(time0);
	pstat_counter++;

	return (ret);
}

pstat_statistic_summary ()
{
	double count;

	printf ("%d pstat calls were made.\n", pstat_counter);
	count = (double)(min_norm) / (double)(8000.0);
	printf ("Minimum getitimer call time was %f milliseconds.\n", count);
	count = (double)(max_norm) / (double)(8000.0);
	printf ("Maximum getitimer call time was %f milliseconds.\n", count);
	count = ((((double)(sum_norm_secs) * (double)(8000000.0)) +
		(double) (sum_norm)) / (double) (pstat_counter));
	count = count / (double) (8000.0);
	printf ("Average getitimer call time was %f milliseconds.\n", count);

	count = (double)(min_open) / (double)(8000.0);
	printf ("Minimum open call time was %f milliseconds.\n", count);
	count = (double)(max_open) / (double)(8000.0);
	printf ("Maximum open call time was %f milliseconds.\n", count);
	count = ((((double)(sum_open_secs) * (double)(8000000.0)) +
		(double) (sum_open)) / (double) (pstat_counter));
	count = count / (double) (8000.0);
	printf ("Average open call time was %f milliseconds.\n", count);

	count = (double)(min_close) / (double)(8000.0);
	printf ("Minimum close call time was %f milliseconds.\n", count);
	count = (double)(max_close) / (double)(8000.0);
	printf ("Maximum close call time was %f milliseconds.\n", count);
	count = ((((double)(sum_close_secs) * (double)(8000000.0)) +
		(double) (sum_close)) / (double) (pstat_counter));
	count = count / (double) (8000.0);
	printf ("Average close call time was %f milliseconds.\n", count);

	count = (double)(min_pstat) / (double)(8000.0);
	printf ("Minimum pstat call time was %f milliseconds.\n", count);
	count = (double)(max_pstat) / (double)(8000.0);
	printf ("Maximum pstat call time was %f milliseconds.\n", count);
	count = ((((double)(sum_pstat_secs) * (double)(8000000.0)) +
		(double) (sum_pstat)) / (double) (pstat_counter));
	count = count / (double) (8000.0);
	printf ("Average pstat call time was %f milliseconds.\n", count);
}
#else  MAKE_TIMING_STATS

pstat_status_if (command, pid, pstat_ptr)
    int command;
    int pid;
    struct pst_status *pstat_ptr;
{
	register int ret;
	ret = pstat(PSTAT_PROC,pstat_ptr,sizeof(struct pst_status),0,pid);
	return (ret);
}

pstat_statistic_summary () {}

#endif MAKE_TIMING_STATS
#endif hp9000s800

#ifdef hp9000s300

pstat_status_if (command, pid, pstat_ptr)
    int command;
    int pid;
    struct pst_status *pstat_ptr;
{
	register int ret;
	
	ret = pstat(PSTAT_PROC,pstat_ptr, sizeof(struct pst_status),0,pid);
	return (ret);
}

pstat_statistic_summary () {}

#endif hp9000s300

/*++++++++++++++++++++++++++++++++++++++++++++++*/
char *state_name(state, abbrev)
     long state;
     int  abbrev;
{
  switch (state) {
  case PS_SLEEP:  return abbrev ? "sleep" : "sleeping";
  case PS_RUN:    return abbrev ? "run" : "running";
  case PS_STOP:   return abbrev ? "stop" : "stopped";
  case PS_ZOMBIE: return abbrev ? "zomb" : "zombie";
  case PS_IDLE:   return abbrev ? "idle" : "starting";
  case PS_OTHER:
  default:        return "other";
  }
}
