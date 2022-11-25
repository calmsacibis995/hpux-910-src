/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/kic/RCS/kic.c,v $
 * $Revision: 1.2.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:43:09 $
 */

/* Process to enable trace stubs in kernel and display clocks & flags */

#ifdef _KERNEL_BUILD
#include 	"../h/param.h"
#include 	"../h/time.h"
#include 	"../h/ki_calls.h"
#include 	"../h/mp.h"
#include 	"../h/signal.h"
#include 	"../h/unistd.h"
#include 	"../h/errno.h"
#else /* ! _KERNEL_BUILD */
#include 	<sys/param.h>
#include 	<sys/time.h>
#include 	<sys/ki_calls.h>
#include 	<sys/mp.h>
#include 	<sys/signal.h>
#include 	<unistd.h>
#include 	<errno.h>
#endif /* _KERNEL_BUILD */
#include	<stdio.h>

/* Process to enable trace stubs in kernel and display clocks & flags */

#define	ferr3(A, B, C)	{fprintf(stderr, A, B, C); perror("");}
#define	ferr2(A, B)	{fprintf(stderr, A, B); perror("");}

#define	KtoS(KT)	double_ki_timeval_to_sec(KT)

char	*trace_names[KI_MAXKERNCALLS] = 
{
	"KTC_CLOCKS_0",
	"KI_SYSCALLS_1",
	"KI_SERVESTRAT_2",
	"KI_SERVSYNCIO_3",
	"KI_ENQUEUE_4",
	"KI_QUEUESTART_5",
	"KI_QUEUEDONE_6",
	"KI_HARDCLOCK_7",
	"KI_CLOSEF_8",
	"KI_BRELSE_9",
	"KI_GETNEWBUF_10",
	"KI_SWAP_11",
	"KI_SWTCH_12",
	"KI_13",
	"KI_RESUME_CSW_14",
	"KI_HARDCLOCK_IDLE_15",
	"KI_SUIDPROC_16",
	"KI_DM1_SEND_17",
	"KI_DM1_RECV_18",
	"KI_RFSCALL_19",
	"KI_RFS_DISPATCH_20",
	"KI_SETRQ_21",
	"KI_DO_BIO_22",
	"KI_USERPROC_23",
	"KI_MEMFREE_24",
	"KI_ASYNCPAGEIO_25",
	"KI_LOCALLOOKUPPN_26",
	"KI_VFAULT_27",
	"KI_MISS_ALPHA_28",
	"KI_PREGION_29",
	"KI_REGION_30",
	"KI_31", 
	"KI_32", 
	"KI_33",
	"KI_34", 
	"KI_35", 
	"KI_36", 
	"KI_37", 
	"KI_38", 
	"KI_39",
	"KI_40", 
	"KI_41", 
	"KI_42", 
	"KI_43", 
	"KI_44", 
	"KI_45",
	"KI_46", 
	"KI_47", 
	"KI_48", 
	"KI_49"
};

struct	ki_config	ki_cf;
struct	ki_config	before_ki_cf;
struct	timeval	sys_time;

/* configuration table of counters */

int	kic_flag= 0;	/* print additional usage message for kic behaviour */
int	rflag	= 0;	/* Read global counter values for non-zero traces */
int	Rflag	= 0;	/* Read global counter values for all traces */

int	Aflag	= 0;	/* Alloc sys mem for trace bufs - # bytes (16384 min) */
int	aflag	= 0;	/* Dealloc sys mem for trace bufs */

int	Mflag	= 0;	/* Set max time (50 = 1 Sec) daemon wait trace buffer */

int	Sflag	= 0;	/* on  all SYSTEM CALL traces */
int	sflag	= 0;	/* off all SYSTEM CALL traces */

int	Tflag	= 0;	/* on  selected SYSTEM CALL traces */
int	tflag	= 0;	/* off selected SYSTEM CALL traces */

int	Kflag	= 0;	/* on  all KI stub traces */
int	kflag	= 0;	/* off all KI stub traces */

int	Lflag	= 0;	/* on  selected KI stub traces */
int	lflag	= 0;	/* off selected KI stub traces */

int	zflag	= 0;	/* clear counter memory */

int	nflag	= 0;	/* dont sort -r/-R by numeric usage */
int	cflag	= 0;	/* sort by trace counts */

int	qflag	= 0;	/* quiet flag -- do not print output */

int	errflg	= 0;


struct  print_list 
{
	char	*trace_name;
	u_int	trace_counts;
	int	on_flag;
	double	atime;
};

#include "set_nams.h"
	
int kic_exit_cnt = 0;

kic_exit(exit_code)
int	exit_code;
{
	u_int	optn;

	if (kic_exit_cnt++) exit(2);	/* XXX */

	/* Do it silently as possible */
	qflag++;

	/* get current flags */
	ki_config_read(&ki_cf);

	/* Turn off the kernel stubs that were off */
	for (optn = 0; optn < KI_MAXKERNCALLS; optn++)
	{
		if ((before_ki_cf.kc_kernelenable[optn] == 0) &&
			(ki_cf.kc_kernelenable[optn])) 
		{
			lflag_sub(optn);
		}
	}
	/* Turn off the systemcalls that were off */
	for (optn = 0; optn < KI_MAXSYSCALLS; optn++)
	{
		if ((before_ki_cf.kc_syscallenable[optn] == 0) &&
			(ki_cf.kc_syscallenable[optn])) 
		{
			tflag_sub(optn);
		}
	}
	exit(exit_code);
}

struct  print_list print_l[KI_MAXSYSCALLS];

int	set_nams_fl = 0;

cset_nams()
{
	if (set_nams_fl) return;
	set_nams(print_l);
	set_nams_fl++;
}


/* very slow, but we have plenty of time */
find_no_Kmatch_to(s, value)
char	s[];
int	value;
{
	int	optn, max;

	for (max = 31; max; max--)
	{
		for (optn = 1; optn < KI_MAXKERNCALLS; optn++)
		{
			if (optn == value) continue;
			if (strncmp(s, trace_names[optn]+3, max) == 0)
			{
				s[max+1] = '\0';
				return;
			}
		}
	}
	s[1] = '\0';
}

print_ill_KC(s)
char *s;
{
	char	s32[32];
	int	value;

	fprintf(stderr, "\n'%s' is not a legal option -- try:\n\n", s);
	fprintf(stderr, "KTC_CLOCKS_0             0  KT\n");

	/* skip the 1st one, because it has a funny name */
	for (value = 1; value < KI_MAXKERNCALLS; value++)
	{
		s = trace_names[value];

		/* Skip short names */
		if (strlen(s) < 6) continue;

		strncpy(s32, s+3, 31); s32[31] = '\0';

		find_no_Kmatch_to(s32, value);

		fprintf(stderr, "%-22s  %2d  %s\n", s, value, s32);
	}
}

/* very slow, but we have plenty of time */
find_no_Smatch_to(s, value)
char	s[];
int	value;
{
	int	optn, max;

	for (max = 31; max; max--)
	{
		for (optn = 0; optn < KI_MAXSYSCALLS; optn++)
		{
			if (optn == value) continue;
			if (strncmp(s, print_l[optn].trace_name, max) == 0)
			{
				s[max+1] = '\0';
				return;
			}
		}
	}
	s[1] = '\0';
}

print_ill_SC(s)
char *s;
{
	char	s32[32];
	int	optn;
	int	pfl = 0;

fprintf(stderr, "\n'%s' is not a legal option -- try:\n\n", s);
fprintf(stderr,
"           -or-              <number>                -or-\n");

	for (optn = 0; optn < KI_MAXSYSCALLS; optn++)
	{
		s = print_l[optn].trace_name;

		/* Skip unimplemented system call names */
		if (*s == 'S') continue;

		strncpy(s32, s, 31); s32[31] = '\0';

		find_no_Smatch_to(s32, optn);

		fprintf(stderr, "%-19s ", s);

		if (++pfl & 1)
		{
			fprintf(stderr, " %-16s    ", s32);
		} else
		{
			fprintf(stderr, " %s\n", s32);
		}
	}
	/* check if odd number of items */
	if (pfl) fprintf(stderr, "\n");
}

/* routine to return int from KERNELTRACE trace number or name */
u_int
atoKcalli(s)
char	*s;
{
	char	*s1;
	u_int	optn;
	int	len;	/* len chars for a unique match */

	/* allow input in any base */
	optn = (u_int)strtoul(s, &s1, 0);

	/* if non-numeric conversion then try ascii */
	if (s == s1)
	{	/* check if skip over the "KI_" */
		if (strncmp(s, "KI_", 3) == 0) { s1 += 3; } 

		len = strlen(s1);

		for (optn = 0; optn < KI_MAXKERNCALLS; optn++)
		{
			if (strncmp(s1, trace_names[optn]+3, len) == 0)
				 { return(optn); }
		}
	} else if (optn < KI_MAXKERNCALLS)
	{ 
		return(optn); 
	}
	print_ill_KC(s);
	kic_exit(2);
}

/* routine to return int from SYSTEMCALL trace number or name */
u_int
atoScalli(s)
char *s;
{
	char	*s1;
	u_int	optn;
	int	len;	/* len chars for a unique match */

	/* allow input in any base */
	optn = (u_int)strtoul(s, &s1, 0);

	/* if non-numeric conversion then try ascii */
	if (s == s1)
	{
		cset_nams();

		len = strlen(s);
		for (optn = 0; optn < KI_MAXSYSCALLS; optn++)
		{
			if (strncmp(s, print_l[optn].trace_name, len) == 0)
			{	return(optn);
			}
		}
	} 
	else if (optn < KI_MAXSYSCALLS)
	{
		return(optn);
	}
	print_ill_SC(s);
	kic_exit(2);
}

char	my_name[4];

main(argc, argv)
int	 argc;
char	*argv[];
{
	extern char	*optarg;
	extern int 	 optind;
	register 	 ii;

	/* if no parameters, then act like a "time" command */

	/* get last 3 chars of my name */
	strcpy(my_name, argv[0] + strlen(argv[0]) -3);

	/* check if kic */
	if (strcmp(my_name, "kic") == 0) kic_flag++;

	/* read present configuration */
	ki_config_read(&before_ki_cf);

	/* copy */
	ki_cf = before_ki_cf;

	/* check for special case of no parameters */
	if (argc == 1)
	{
		goto usage_mesg;
	}
	/* get parameters */
	while ((ii = getopt(argc, argv, "crRA:aM:SsT:t:KkL:l:znOoqQG")) != EOF) 
	{
		switch (ii) 
		{
		case	'c':	
			cflag++;
			break;

		case	'r':	
			rflag++;
			rflag_sub();
			break;

		case	'R':	
			Rflag++;
			Rflag_sub();
			break;

		case	'A':	
			Aflag++; 
			Aflag_sub(atoi(optarg));
			break;
		
		case	'a':	
			aflag++; 
			aflag_sub();
			break;
		
		case	'M':	
			Mflag++;
			Mflag_sub(atoi(optarg));
			break;
		
		case	'S':	
			Sflag++;
			Sflag_sub();
			break;
		
		case	's':	
			sflag++;
			sflag_sub();
			break;
		
		case	'T':	
			Tflag++;
			Tflag_sub(atoScalli(optarg));
			break;
		
		case	't':	
			tflag++;
			tflag_sub(atoScalli(optarg));
			break;
		
		case	'K':	
			Kflag++;
			Kflag_sub();
			break;
		
		case	'k':	
			kflag++;
			kflag_sub();
			break;
		
		case	'L':	
			Lflag++;
			Lflag_sub(atoKcalli(optarg));
			break;
		
		case	'l':	
			lflag++;
			lflag_sub(atoKcalli(optarg));
			break;
		
		case	'z':	
			zflag++;
			zflag_sub();
			break;
		
		case	'n':	
			nflag++;
			break;
		
		case	'o':	
			lflag_sub(KI_GETPRECTIME);
			break;

		case	'O':	
			Lflag_sub(KI_GETPRECTIME);
			break;
		
		case	'q':	
			qflag = 1;
			break;

		case	'Q':	
			qflag = 0;
			break;

		case	'G':	
			Gflag_sub();
			break;

		default: 
usage_mesg:
fprintf(stderr,
"\nUsage: %s <options> <Command param1 param2 etc...>\n\n", my_name);
fprintf(stderr, 
"  -r                >> Print non-zero counter/KT clock values\n");
fprintf(stderr, 
"  -R                >> Print all counter/KT clock values\n");
fprintf(stderr, 
"  -o                >> Suspend KT clocks from ticking\n");
fprintf(stderr, 
"  -O                >> Resume KT clocks\n");
fprintf(stderr, 
"  -n                >> Do not sort output (before -r or -R)\n");
fprintf(stderr, 
"  -c                >> Sort by SYSCALL counts, not KT clocks (before -r or -R)\n");
if (kic_flag)
{
fprintf(stderr, 
"  -S                >> Turn ON  all SYSTEM CALL traces\n");
fprintf(stderr, 
"  -s                >> Turn OFF all SYSTEM CALL traces\n");
fprintf(stderr, 
"  -K                >> Turn ON  all KI stub traces\n");
fprintf(stderr, 
"  -k                >> Turn OFF all KI stub traces\n");
fprintf(stderr, 
"  -A <amount>       >> Allocate Kernel memory for trace # bytes (65536+ min)\n");
fprintf(stderr, 
"  -a                >> Dealloc Kernel memory for trace buffers\n");
fprintf(stderr, 
"  -M <maxticks>     >> Set max time (%d = 1 Sec) daemon wait trace buffer\n",
							sysconf(_SC_CLK_TCK));
fprintf(stderr, 
"  -T <call numb>    >> Turn ON  selected SYSTEM Call traces\n");
fprintf(stderr, 
"  -t <call numb>    >> Turn OFF selected SYSTEM Call traces\n");
fprintf(stderr, 
"  -L <call numb>    >> Turn ON  selected KI Stub traces\n");
fprintf(stderr, 
"  -l <call numb>    >> Turn OFF selected KI Stub traces\n");
}
fprintf(stderr, 
"  -q                >> Turn ON quiet flag - suppress fprintf's\n");
fprintf(stderr, 
"  -Q                >> Turn OFF quiet flag - restore fprintf's\n");
fprintf(stderr, 
"  -G                >> %s -KozO;  sysconf(_SC_CLK_TCK); %s -okr\n", my_name, my_name);
fprintf(stderr, 
"  -z                >> Zero counter/KT clock memory\n");
			kic_exit(2);
		}
		if (errflg) kic_exit(0);
	}
	/* if still remaining parameters, go execute */
	if (optind < argc)
	{
		run_command(argv[optind], &argv[optind]);
	}
	kic_exit(0);
}

ki_config_read(ki_cfp)
struct ki_config *ki_cfp;
{
	if (ki_call(KI_CONFIG_READ, ki_cfp, KI_CF) < 0) 
	{
		ferr2("KI_CONFIG_READ failed errno = %d - ", errno);
		if (errno == EPERM)
		{
			fprintf(stderr, "Must be setuid root (chown root %s; chmod 4555 %s)\n",
				my_name, my_name);
		}
		errflg++;
		exit(2);
	}
}

Aflag_sub(isize)
int	isize;
{
	int	iactl;	/* actual amount */

	/* Allocate the trace buffer from kernel memory */
	if (!qflag)
	fprintf(stderr, "Call KI_ALLOC_TRACEMEM with %d bytes\n", isize);

	if ((iactl = ki_call(KI_ALLOC_TRACEMEM, isize)) < 0) 
	{
		ferr3("KI_ALLOC_TRACEMEM failed with %d bytes, errno = %d - ",
			isize, errno);
		errflg++;
	} else {
		if (!qflag) {
		fprintf(stderr, "Actual Trace buf len = %d X 4 bytes\n", iactl);
		}
	}
}
aflag_sub()
{
	if (!qflag)
	fprintf(stderr, "Free trace buffers system memory\n");
	if (ki_call(KI_FREE_TRACEMEM) < 0) 
	{
		ferr2("KI_FREE_TRACEMEM failed errno = %d - ", errno);
		errflg++;
	}
}
Sflag_sub()
{

	if (!qflag)
	fprintf(stderr, "Enable ALL system CALL traces\n");
	if (ki_call(KI_SET_ALL_SYSCALTRACES) < 0) 
	{
		ferr2("KI_SET_ALL_SYSCALTRACE failed, errno = %d - ", errno);
		errflg++;
	}
}

sflag_sub()
{
	if (!qflag)
	fprintf(stderr, "Disable ALL system CALL traces\n");
	if (ki_call(KI_CLR_ALL_SYSCALTRACES) < 0) 
	{
		ferr2("KI_CLR_ALL_SYSCALTRACE failed errno = %d - ", errno);
		errflg++;
	}
}
Kflag_sub()
{
	if (!qflag)
	fprintf(stderr, "Enable ALL kernel STUB traces\n");
	if (ki_call(KI_SET_ALL_KERNELTRACES) < 0) 
	{
		ferr2("KI_SET_ALL_KERNELTRACES %d failed, errno = %d - ", errno);
		errflg++;
	}
}

kflag_sub()
{
	if (!qflag)
	fprintf(stderr, "Disable ALL kernel STUB traces\n");
	if (ki_call(KI_CLR_ALL_KERNELTRACES) < 0) 
	{
		ferr2( "KI_CLR_ALL_KERNELTRACES failed errno = %d - ", errno);
		errflg++;
	}
}

Mflag_sub(msize)
int	msize;	/* number */
{
	if (!qflag)
	fprintf(stderr, "Set MAX daemon sleep time to %5.2f Secs\n", (double)msize/HZ);
	if (ki_call(KI_TIMEOUT_SET, msize) < 0) 
	{
		ferr3("KI_TIMEOUT_SET %d failed, errno = %d - ", msize, errno);
		errflg++;
	}
}

zflag_sub()
{
	if (!qflag)
	{
		if (kic_flag) { fprintf(stderr, "KI_CONFIG_CLEAR\n"); }
		else { fprintf(stderr, "Clear KT clocks\n"); }
	}
	if (ki_call(KI_CONFIG_CLEAR) < 0) 
	{
		ferr2("KI_CONFIG_CLEAR failed errno = %d - ", errno);
		errflg++;
	}
}

Rflag_sub()
{
	gettimeofday(&sys_time, NULL);
	print_ki_config(1);
}

rflag_sub()
{
	gettimeofday(&sys_time, NULL);
	print_ki_config(0);
}

Tflag_sub(opt)
u_int	opt;
{

	cset_nams();

	if (!qflag && (opt < KI_MAXSYSCALLS))
	fprintf(stderr, "Enable SYSCALL trace  '%s'\n", 
		print_l[opt].trace_name); 

	if (ki_call(KI_SET_SYSCALLTRACE, opt) < 0) 
	{
		ferr3("KI_SET_SYSCALLTRACE %d failed, errno = %d - ", opt, errno);
		errflg++;
	}
}

tflag_sub(opt)
u_int	opt;
{
	cset_nams();

	if (!qflag && (opt < KI_MAXSYSCALLS))
		fprintf(stderr, "Disable SYSCALL trace  '%s'\n", 
			print_l[opt].trace_name); 
	
	if (ki_call(KI_CLR_SYSCALLTRACE, opt) < 0) 
	{
		ferr3("KI_CLR_SYSCALLTRACE %d failed, errno = %d - ", opt, errno);
		errflg++;
	}
}

Lflag_sub(opt)
u_int	opt;
{
	if (!qflag)
	{
		if (opt == KI_GETPRECTIME) 
		{ 
			fprintf(stderr, "Resume/ON KT clocks\n");
		}
		else if (opt < KI_MAXKERNCALLS)
		{ 
			fprintf(stderr, "Enable KERNEL trace '%s'\n", trace_names[opt]);
		}
	}
	if (ki_call(KI_SET_KERNELTRACE, opt) < 0) 
	{
		ferr3("KI_SET_KERNELTRACE %d failed, errno = %d - ", opt, errno);
		errflg++;
	}
}

lflag_sub(opt)
u_int	opt;
{
	if (ki_call(KI_CLR_KERNELTRACE, opt) < 0) 
	{
		ferr3("KI_CLR_KERNELTRACE %d failed, errno = %d - ", opt, errno);
		errflg++;
	}
	if (!qflag)
	{
		if (opt == KI_GETPRECTIME) 
		{ 
			fprintf(stderr, "Suspend KT clocks\n");
		} 
		else if (opt < KI_MAXKERNCALLS)
		{
			fprintf(stderr, "Disable KERNEL trace '%s'\n", trace_names[opt]);
		}
	}
}

Gflag_sub()
{
	int	hz;
	struct ki_config after_ki_cf;

	/* turn on all kernel stubs */
	Kflag_sub();

	/* turn off clocks */
	lflag_sub(KI_GETPRECTIME);

	/* read clocks */
	ki_config_read(&ki_cf);

	/* Turn on clocks */
	Lflag_sub(KI_GETPRECTIME);

	/* execute the benchmark */
	hz = sysconf(_SC_CLK_TCK);

	/* Turn off clocks */
	lflag_sub(KI_GETPRECTIME);

	/* read clocks again */
	ki_config_read(&after_ki_cf);

	/* take difference */
	ki_cf_diff(&after_ki_cf);

	/* print them out */
	rflag_sub();

	/* Turn on all kernel stubs */
	Kflag_sub();

	/* turn on clocks */
	Lflag_sub(KI_GETPRECTIME);
}

#define	diff_u_int(old, new)	((old) = (new)-(old))

diff_ki_timeval(old, new)
struct	ki_timeval *old;
struct	ki_timeval *new;
{
	old->tv_sec = new->tv_sec - old->tv_sec;
	old->tv_nunit = new->tv_nunit - old->tv_nunit;

	if ((u_int)(old->tv_nunit) >= ki_nunit_per_sec)
	{
		old->tv_sec--;
		old->tv_nunit += ki_nunit_per_sec;
	}
	if ((u_int)old->tv_nunit >= ki_nunit_per_sec)
	{
fprintf(stderr, "tv_sec = %d, tv_nunit = %d, nunit_per_sec = %d\n",
		old->tv_sec,
		old->tv_nunit, ki_nunit_per_sec);
	}
}

ki_cf_diff(new_ki_cf)
struct ki_config *new_ki_cf;
{
	int			pp, ii;

	register struct ki_proc_info *kp_old;
	register struct ki_proc_info *kp_new;

	for (pp = 0; pp < KI_MAX_PROCS; pp++)
	{
	/* Skip non running processors */
	if (ki_freq_ratio(pp) == 0.0) continue;

	/* get pointers to ki_proc_info */
	kp_old = &(ki_proc(pp));
	kp_new = &(new_ki_cf->ki_proc_info[pp]);

	for (ii = 0; ii < KT_NUMB_CLOCKS; ii++)
	{
		register struct ki_runtimes *kt_old;
		register struct ki_runtimes *kt_new;

		kt_old = &(kp_old->kt_timestruct[ii]);
		kt_new = &(kp_new->kt_timestruct[ii]);

		/* diff the KTC timers */
		diff_ki_timeval(&kt_old->kp_accumtm, &kt_new->kp_accumtm);

		/* diff the KTC instruction counters */
		diff_ki_timeval(&kt_old->kp_accumin, &kt_new->kp_accumin);

		/* diff the KTC clock counters */
		diff_u_int(kt_old->kp_clockcnt, kt_new->kp_clockcnt);
	}

	for (ii = 0; ii < KI_MAXSYSCALLS; ii++)
	{
		register struct ki_timeval *km_old;
		register struct ki_timeval *km_new;

		km_old = &(kp_old->kt_syscall_times[ii]);
		km_new = &(kp_new->kt_syscall_times[ii]);

		/* diff the KTC systemcall counters */
		diff_u_int(
			kp_old->kt_syscallcounts[ii],
			kp_new->kt_syscallcounts[ii]);

		/* diff the KTC timers */
		diff_ki_timeval(km_old, km_new);
	}

	for (ii = 0; ii < KI_MAXKERNCALLS; ii++)
	{
		/* diff the KI stub counters */
		diff_u_int(
			kp_old->kt_kernelcounts[ii],
			kp_new->kt_kernelcounts[ii]);
	}
	}
}


run_command(command, command_args)
char *command;
char **command_args;
{
	char **args;
	register p;
	extern	errno;
	int	status, ruid;
	long	before, after;
	extern long times();
	struct {
		time_t user;
		time_t sys;
		time_t childuser;
		time_t childsys;
	} buffer1, buffer2;        /* buffer1 and buffer2 has the starting */

	struct ki_config after_ki_cf;

	/* if no command then the default */
	if (*command == '\0') { rflag_sub(); kic_exit(0); }

	/* get the real-user-ID */
	ruid = getuid();

	/* Make sure KTC clocks are on */
	Lflag_sub(KI_GETPRECTIME);

	/* output the command and arguments */
	fprintf(stderr, "\n");
	for (args = command_args; *args; args++)
	{	fprintf(stderr, "%s ", *args); }
	fprintf(stderr, "\n");

	/* Read the clocks before command execution */
	ki_config_read(&ki_cf);

	before = times(&buffer1);

	if ((p = fork()) == -1) 
	{
		ferr2("fork() failed errno = %d - ", errno);
		kic_exit(2);
	}
	/* check if child */
	if (p == 0) 
	{
		/* put privlages back to normal */
		setresuid(ruid, ruid, ruid);

		/* Child, exec command */
		execvp(command, command_args);
		ferr2("exec() failed errno = %d - ", errno);
		kic_exit(2);
	}
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	while (wait(&status) != p)
		continue;

	after = times(&buffer2);

	/* read clocks again */
	ki_config_read(&after_ki_cf);

	/* make sure 'after' was successfully and correctly updated */
	if (after < before) {
		ferr2("command terminated abnormally - status = %d ", status & 0377);
		kic_exit(2);
	}
	if ((status & 0377) != '\0')
	{
		ferr2("command terminated abnormally - status = %d ", status & 0377);
	}
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);

	fprintf(stderr, "\n");
	printt("real", (after-before));
	printt("user", (buffer2.childuser - buffer1.childuser));
	printt("sys ", (buffer2.childsys  - buffer1.childsys));

	/* get the KTC clocks difference */
	ki_cf_diff(&after_ki_cf);

	if (Rflag) Rflag_sub();
	else	   rflag_sub();

	kic_exit(status >> 8);
}

/*
The following use of HZ/10 will work correctly only if HZ is a multiple
of 10.  However the only values for HZ now in use are 100 for the 3B
and 60 for other machines.
*/
char quant[] = { HZ/10, 10, 10, 6, 10, 6, 10, 10, 10 };
char *pad  = "000      ";
char *sep  = "\0\0.\0:\0:\0\0";
char *nsep = "\0\0.\0 \0 \0\0";

printt(s, a)
char *s;
long a;
{
	register i;
	char	digit[9];
	char	c;
	int	nonzero;

	for(i=0; i<9; i++) {
		digit[i] = a % quant[i];
		a /= quant[i];
	}
	fputs(s, stderr);
	nonzero = 0;
	while(--i>0) {
		c = digit[i]!=0 ? digit[i]+'0':
		    nonzero ? '0':
		    pad[i];
		if (c != '\0')
			putc (c, stderr);
		nonzero |= digit[i];
		c = nonzero?sep[i]:nsep[i];
		if (c != '\0')
			putc (c, stderr);
	}
	fputc('\n', stderr);
}

/* compare routine for sorting by systemcall time (qsort) */
int
tcomp(A, B)
struct print_list *A, *B;
{
	double fdiff;

	/* compare syscall times */
	fdiff = B->atime - A->atime;

	if	(fdiff == 0.0)	return(0);
	else if (fdiff < 0.0)	return(-1);
	else			return(1);
}

/* compare routine for sorting by trace counts (qsort) */
int
ccomp(A, B)
struct print_list *A, *B;
{
	register int	rtnval;

	/* first compare counter values */
	if (rtnval = B->trace_counts - A->trace_counts) return(rtnval);

	/* if same than sort alphabetically */
	return(strcmp(A->trace_name, B->trace_name));
}

double F_total 	= 	0.0;

print_gclock(name, F_time, tcounts) 
char	*name;
double	F_time;
unsigned int	tcounts;
{

	if (tcounts)
	{
		fprintf(stderr, "%-22s %8d ", name, tcounts);
	} else
	{
		fprintf(stderr, "%-22s          ", name);
	}
	if (F_total == 0.0) F_total = 1E-7;
	fprintf(stderr, "%17.6f Sec ~ %7.3f%%", F_time, F_time/F_total*100.0);

	if (tcounts) 
	{ 
		fprintf(stderr, " %10.1f uS/\n", F_time*1E6/tcounts); 
	} else 
	{ 
		fprintf(stderr, "         Total\n"); 
	}
}

print_ki_config(allflag)
{
	struct  print_list print_bl[KI_MAXSYSCALLS+KI_MAXKERNCALLS];
	int Total_syscalls, ii, jj, pp;
	int	tcounts;
	double fTime;
	time_t timer;
	char	*str;
	double F_sys_ck;
	double F_sys;
	double F_usr;
	double F_csw;
	double F_idle;
	double F_intusr;
	double F_intidle;
	double F_intsys;
	double F_vfault;
	double F_trap;
	double F_spare;
	double F_spare1;
	double F_spare2;


for (pp = 0; pp < KI_MAX_PROCS; pp++)
{
	/* check if cpu is running */
	if (ki_freq_ratio(pp) == 0.0) 
	{
		fprintf(stderr, "Processor number %d not running\n", pp);
		continue;
	}
	F_sys_ck 	= 0.0;
	F_sys		= KtoS(ki_accumtm(pp, KT_SYS_CLOCK));
	F_usr		= KtoS(ki_accumtm(pp, KT_USR_CLOCK));
	F_csw		= KtoS(ki_accumtm(pp, KT_CSW_CLOCK));
	F_idle		= KtoS(ki_accumtm(pp, KT_IDLE_CLOCK));
	F_intusr	= KtoS(ki_accumtm(pp, KT_INTUSR_CLOCK));
	F_intidle	= KtoS(ki_accumtm(pp, KT_INTIDLE_CLOCK));
	F_intsys	= KtoS(ki_accumtm(pp, KT_INTSYS_CLOCK));
	F_vfault	= KtoS(ki_accumtm(pp, KT_VFAULT_CLOCK));
	F_trap		= KtoS(ki_accumtm(pp, KT_TRAP_CLOCK));
	F_spare		= KtoS(ki_accumtm(pp, KT_SPARE_CLOCK));
	F_spare1	= KtoS(ki_accumtm(pp, KT_SPARE1_CLOCK));
	F_spare2	= KtoS(ki_accumtm(pp, KT_SPARE2_CLOCK));

	if (F_sys == 0.0) F_sys = 1E-7;
	F_total = F_sys		+ 
		  F_usr		+ 
		  F_csw		+ 
		  F_idle	+ 
		  F_intusr	+ 
		  F_intidle	+ 
		  F_intsys	+ 
		  F_vfault	+ 
		  F_trap	+
		  F_spare	+
		  F_spare1	+
		  F_spare2;

	/* calculate number of times USR process was dispached */ 
	if (ki_clockcounts(pp, KT_USR_CLOCK) == 0)
	{
 			ki_clockcounts(pp, KT_USR_CLOCK) =
			ki_clockcounts(pp, KT_SYS_CLOCK)	+
			ki_clockcounts(pp, KT_CSW_CLOCK)	+
			ki_clockcounts(pp, KT_VFAULT_CLOCK)+
			ki_clockcounts(pp, KT_TRAP_CLOCK);
	}
/* print out header information */
	time(&timer);
	str = ctime(&timer);
	fprintf(stderr, "\nki_version = %s, Processor number %d of %d, %s\n", 
		ki_version, pp, ki_runningprocs, str);
#ifdef	DEBUG
	fprintf(stderr, "ki_cbuf          = 0x%x\n", ki_cbuf); 
	fprintf(stderr, "ki_count_sz      = %d\n",   ki_count_sz);
	fprintf(stderr, "ki_timeout       = %d\n",   ki_timeout);
	fprintf(stderr, "ki_timeoutct     = %d\n",   ki_timeoutct);
	fprintf(stderr, "ki_nunit_per_sec = %d\n",   ki_nunit_per_sec);
	fprintf(stderr, "ki_tbuf[0]       = 0x%x\n", ki_tbuf(pp)[0]);
	fprintf(stderr, "ki_tbuf[1]       = 0x%x\n", ki_tbuf(pp)[1]);
	fprintf(stderr, "ki_tbuf[2]       = 0x%x\n", ki_tbuf(pp)[2]);
	fprintf(stderr, "ki_tbuf[3]       = 0x%x\n", ki_tbuf(pp)[3]);
#endif	/* DEBUG */
	if (kic_flag)
	{
		if (ki_trace_sz) 
		{
			fprintf(stderr, "KI_tracing is ON with buffer size = %8d\n", ki_trace_sz);
			if (ki_timeout) fprintf(stderr, "ki_timeout    = %8d\n", ki_timeout);
		} else
		{
			fprintf(stderr, "KI_traceing is OFF\n");
		}
		if (ki_sequence_count(pp)) 
		{
			fprintf(stderr, "ki_sequence_count   = %8d\n", ki_sequence_count(pp));
		}
	}
	if (ki_kernelenable[KI_GETPRECTIME]) { fprintf(stderr, "KT clocks are ticking\n"); }
	else                                 { fprintf(stderr, "KT clocks are suspended\n"); }
#ifdef	DEBUG
	fprintf(stderr, "ki_next[0]          = %8d\n", ki_next(pp)[0]);
	fprintf(stderr, "ki_next[1]          = %8d\n", ki_next(pp)[1]);
	fprintf(stderr, "ki_next[2]          = %8d\n", ki_next(pp)[2]);
	fprintf(stderr, "ki_next[3]          = %8d\n", ki_next(pp)[3]);
	fprintf(stderr, "ki_curbuf           = %8d\n", ki_curbuf(pp));
	fprintf(stderr, "ki_seq_ct           = %8d\n", ki_sequence_count(pp));
	fprintf(stderr, "ki_inckclk          = %8d\n", ki_inckclk(pp));
	fprintf(stderr, "ki_incmclk          = %8d\n", ki_incmclk(pp));
	fprintf(stderr, "ki_offset_correction= %8d\n", ki_offset_correction(pp));
	fprintf(stderr, "ki_freq_ratio       = %15.13f\n", ki_freq_ratio(pp));
fTime = (sys_time.tv_sec - ki_time(pp).tv_sec) + 
	(sys_time.tv_usec/1E6 - double_nunit_to_sec(ki_time(pp).tv_nunit));
fprintf(stderr,
" Difference of KI time from system   : %14.6f Sec\n", fTime);
#endif	/* DEBUG */

fprintf(stderr,
"Global Clock    Number of calls          Cpu used        Percent      Ave Time/\n\n");

print_gclock("SYSTEMCALL time    ", F_sys,    ki_clockcounts(pp, KT_SYS_CLOCK));
print_gclock("USER time          ", F_usr,    ki_clockcounts(pp, KT_USR_CLOCK));
ki_clockcounts(pp, KT_USR_CLOCK) = 0;
print_gclock("CONTEXT SWITCH     ", F_csw,    ki_clockcounts(pp, KT_CSW_CLOCK));
print_gclock("IDLE cpu time      ", F_idle,   ki_clockcounts(pp, KT_IDLE_CLOCK));
print_gclock("INTERRUPT from USR ", F_intusr, ki_clockcounts(pp, KT_INTUSR_CLOCK));
print_gclock("INTERRUPT from IDLE", F_intidle,ki_clockcounts(pp, KT_INTIDLE_CLOCK));
print_gclock("INTERRUPT from SYS ", F_intsys, ki_clockcounts(pp, KT_INTSYS_CLOCK));
print_gclock("VFAULT time        ", F_vfault, ki_clockcounts(pp, KT_VFAULT_CLOCK));
print_gclock("TRAP time          ", F_trap,   ki_clockcounts(pp, KT_TRAP_CLOCK));
if (F_spare)
{
print_gclock("Spare clock time   ", F_spare,  ki_clockcounts(pp, KT_SPARE_CLOCK));
}
if (F_spare1)
{
print_gclock("Spare1 clock time   ", F_spare1,  ki_clockcounts(pp, KT_SPARE1_CLOCK));
}
if (F_spare2)
{
print_gclock("Spare2 clock time   ", F_spare2,  ki_clockcounts(pp, KT_SPARE2_CLOCK));
}
print_gclock("\nFor an Elapsed Time = ", F_total, 0);

if (kic_flag) { fprintf(stderr, "\nTrace record name number calls number ON/OFF\n\n"); }

/* print out number of invocation of stub routines */

	Total_syscalls = 0;

#ifdef	DEBUG
fprintf(stderr, "number KI_MAXSYSCALLS  = %d\n", KI_MAXSYSCALLS); 
fprintf(stderr, "number KI_MAXKERNCALLS = %d\n", KI_MAXKERNCALLS); 
#endif	/* DEBUG */

	set_nams(print_bl);

	for (ii = 0; ii < KI_MAXSYSCALLS; ii++) 
	{
		print_bl[ii].trace_counts = ki_syscallcounts(pp)[ii];
		if (ii < (KI_MAXSYSCALLS-50)) Total_syscalls += ki_syscallcounts(pp)[ii];
		print_bl[ii].on_flag		= (int)ki_syscallenable[ii];
		print_bl[ii].atime		= KtoS(ki_syscall_times(pp)[ii]);
	}
	for (ii = 0; ii < KI_MAXKERNCALLS; ii++) 
	{
		jj = ii + KI_MAXSYSCALLS;
		print_bl[jj].trace_name		= trace_names[ii];
		print_bl[jj].trace_counts = ki_kernelcounts(pp)[ii];
		print_bl[jj].on_flag		= ki_kernelenable[ii];
		print_bl[jj].atime		= 0.0;
	}
	/* sort the table by number of times called */
	if (!nflag) 
	{
		if (cflag) 
		{
			/* sort by counts */
			qsort (print_bl, 
			KI_MAXSYSCALLS+KI_MAXKERNCALLS, 
			sizeof(struct print_list), 
			ccomp);

		} else
		{	
			/* sort by time */
			qsort (print_bl, 
			KI_MAXSYSCALLS+KI_MAXKERNCALLS, 
			sizeof(struct print_list), 
			tcomp);
		}
	}
	fprintf(stderr,
"\nThe SYSTEMCALL time is further divided to KERNEL systemcalls and SSYS_daemons.\n\n");

	fprintf(stderr,
"System call/SSYS_Daemon  Counts          Cpu used        Percent      Ave Time/\n\n");

	for (ii = 0; ii < KI_MAXSYSCALLS+KI_MAXKERNCALLS; ii++) 
	{
		if (print_bl[ii].trace_counts || allflag || print_bl[ii].atime)
		{
			tcounts = print_bl[ii].trace_counts;

			if (tcounts)
			{
			fprintf(stderr, "%-22s %8d ", 
				print_bl[ii].trace_name,
				print_bl[ii].trace_counts);
			} else
			{
			fprintf(stderr, "%-22s          ", 
				print_bl[ii].trace_name);
			}
			if (print_bl[ii].on_flag)
		 	{
				fprintf(stderr, "ON ");
			} else 
			{					
				fprintf(stderr, "   ");
			}
			fTime = print_bl[ii].atime;
			if (fTime) 
			{
				F_sys_ck += fTime;
				fprintf(stderr, "%14.6f Sec ~ %7.3f%%", fTime, fTime/F_sys*100.0);

				tcounts = print_bl[ii].trace_counts;
				if (tcounts)
				{
					fprintf(stderr, " %10.1f uS/", fTime*1E6/tcounts);
				} else
				{
					fprintf(stderr, "    Unfinished");
				}
			}	
			fprintf(stderr, "\n");
		}
	}
#ifdef	DEBUG
	fprintf(stderr,
"For Cpu %d Syscalls = %d, System call time of  %14.6f Sec ~ %6.2f%%   ", 
		pp, Total_syscalls, F_sys_ck, F_sys_ck/F_sys*100.0);
#endif	/* DEBUG */
	fprintf(stderr, "\n\n");
}
}
