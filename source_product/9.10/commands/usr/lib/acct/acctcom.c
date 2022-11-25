/* @(#) $Revision: 70.1 $ */      
/*
 *	acctcom [[options] [file] ... ]
 *
 *	Acctcom reads file, the standard input, or /usr/adm/pacct, in the
 *	form described by acct(5) and writes selected records to the
 *	standard output. Each record represents the execution of one
 *	process. The output shows the COMMAND NAME, USER, TTYNAME, START
 *	TIME, END TIME, REAL (SEC), CPU (SEC), MEAN SIZE (K), and
 *	optionally, F (the fork/exec flag: 1 for fork without exec), 
 *	STAT (the system exit status), HOG FACTOR, KCORE MIN, CPU FACTOR,
 *	CHARS TRNSFD, and BLOCKS R/W (total blocks read and written).
 *	
 *	The command name is prepended with a # if it was executed with
 *	super-user privileges. If a process is not associated with a
 *	known terminal, a ? is printed in the TTYNAME field.
 *	
 *	If no files are specified, and if the standard input is associated
 *	with a terminal or /dev/null (as is the case when using & in the
 *	shell), /usr/adm/pacct is read; otherwise, the standard input
 *	is read.
 *	
 *	If any file arguments are given, they are read in their respective
 *	order. Each file is normally read forward, i.e. in chronological
 *	order by process completion time. The file /usr/adm/pacct is
 *	usually the current file to be examined; a busy system may need
 *	several such files of which all but the current file are found 
 *	in /usr/adm/pacct?. The options are:
 *	
 *	-a		Show some average statistics about the processes
 *			selected. The statistics will be printed after
 *			the output records.
 *	
 *	-b		Read backwards, showing latest commands first. 
 *			This option has no effect when the standard input
 *			is read.
 *	
 *	-F		Print the fair share group ID (s800 only for now).
 *	
 *	-f		Print the fork/exec flag and system exit status
 *			columns in the output.
 *	
 *	-h		Instead of mean memory size, show the fraction
 *			of total available CPU time consumed by the
 *			process during its execution. This "hog factor"
 *			is computed as: (total CPU time)/(elapsed time).
 *	
 *	-i		Print columns containing I/O counts in the output.
 *	
 *	-k		Instead of memory size, show total kcore-minutes.
 *	
 *	-m		Show mean core size (the default).
 *	
 *	-r		Show CPU factor (user time)/(system time + user time).
 *	
 *	-t		Show separate system and user CPU times.
 *	
 *	-v		Exclude column headings from the output.
 *	
 *	-l line		Show only processes belonging to terminal /dev/line.
 *	
 *	-u user		Show only processes belonging to user that may
 *			be specified by: a user ID, a login name that is
 *			then converted to a user ID, a # which designates
 *			only those processes executed with super-user
 *			processes, or ? which designates only those
 *			processes associated with unknown user IDs.
 *	
 *	-G fsgrp	Show only processes belonging to fair share group
 *			(s800 only for now).  The group may be designated
 *			by either the fair share group ID or fair share
 *			group name.
 *	
 *	-g group	Show only processes belonging to group. The group
 *			may be designated by either the group ID or group
 *			name.
 *	
 *	-s time		Select processes existing at or after time, given
 *			in the format hr[:min[:sec]].
 *	
 *	-e time		Select processes existing at or before time.
 *	
 *	-S time		Select processes starting at or after time.
 *	
 *	-E time		Select processes ending at or before time. Using
 *			the same time for both -S and -E shows the
 *			processes that existed at time.
 *	
 *	-n pattern	Show only commands matching pattern that may be
 *			a regular expression as in ed(1) except that +
 *			means one or more occurences.
 *
 *	-q		Do not print any output records, just print the
 *			average statistics as with the -a option.
 *	
 *	-o ofile	Copy selected process records in the input data
 *			format to ofile; suppress standard output printing.
 *	
 *	-H factor	Show only processes that exceed factor, where
 *			factor is the "hog factor" as explained in
 *			option -h above.
 *	
 *	-O sec		Show only processes with CPU system time exceeding
 *			sec seconds.
 *	
 *	-C sec		Show only processes with total CPU time, system
 *			plus user, exceeding sec seconds.
 *	
 *	-I chars	Show only processes transferring more characters
 *			than the cut-off number given by chars.
 *	
 */

#include	<time.h>
#include	<string.h>
#include	<sys/types.h>
#include	"acctdef.h"
#include	<grp.h>
#include	<stdio.h>
#include	<sys/acct.h>
#include	<pwd.h>
#include	<sys/stat.h>
#ifdef	FSS
#include	<signal.h>
#include	<fsg.h>
#include	<sys/fss.h>
#endif

#define MYKIND(flag)	((flag & ACCTF) == 0)
#define SU(flag)	((flag & ASU) == ASU)
#ifdef	FSS
#define fsid(a)		((((a)->ac_flag) >> 2) & 0xF)
#endif
#define PACCT		"/usr/adm/pacct"
#define MEANSIZE	01
#define KCOREMIN	02
#define HOGFACTOR	04
#define	SEPTIME		010
#define	CPUFACTOR	020
#define IORW		040
#ifdef	FSS
#define FSID		0100
#endif
#define	pf(dble)	fprintf(stdout, "%8.2lf", dble)
#define	ps(s)		fprintf(stdout, "%8.8s", s)
#define	diag(string)		fprintf(stderr, "\t%s\n", string)

struct	acct ab;
char	command_name[16];
char	timestring[16];
char	obuf[BUFSIZ];

double	cpucut,
	syscut,
	hogcut,
	iocut,
	realtot,
	cputot,
	usertot,
	systot,
	kcoretot,
	iotot,
	rwtot;
extern long	timezone;
long    curtzone;		/* DST adjusted timezone */
long	daydiff,
	offset = -2,
	elapsed,
	sys,
	user,
	cpu,
	mem,
	io,
	rw,
	cmdcount;
time_t	tstrt_b,
	tstrt_a,
	tend_b,
	tend_a,
	etime;
char	sstrt_b[10],
	sstrt_a[10],
	send_b[10],
	send_a[10];
int	backward,
	flag_field,
	average,
	quiet,
	option,
	verbose = 1,
	uidflag,
	gidflag,
#ifdef	FSS
	fsidflag,
#endif
	unkid,	/*user doesn't have login on this machine*/
	errflg,
	su_user,
	fileout = 0,
	stdinflg,
	nfiles = 0;
dev_t	linedev	= -1;
uid_t	uidval,
	gidval;
#ifdef	FSS
int	fsidval;
#endif
char	*cname = NULL; /* command name pattern to match*/

struct passwd *getpwnam(), *pw;
struct group *getgrnam(),*grp;
#ifdef	FSS
struct fsg *getfsnam(), *fsgrp;
#endif
long	ftell(),
	convtime(),
	time(),
	tmsecs(),
	expand();
char	*ctime(),
	*ofile,
	*devtolin(),
	*uidtonam();
dev_t	lintodev();
FILE	*ostrm;

#ifdef	FSS
int	fssincluded = 1;	/* assume there's a fair share scheduler */
int	oldsig;

handler() {
	fssincluded = 0;	/* if we got here, there's no fss */
}
#endif

#ifdef	FSS
#define	OPTIONS	"C:E:H:I:O:S:abe:fg:hikl:mn:o:qrs:tu:vFG:"
#else
#define	OPTIONS	"C:E:H:I:O:S:abe:fg:hikl:mn:o:qrs:tu:v"
#endif

main(argc, argv)
char **argv;
{
	register int	c;
	extern int	optind;
	extern char	*optarg;

#ifdef	FSS
	oldsig = signal(SIGSYS, handler);
	fss(FS_STATE);		/* try it and see what happens */
	signal(SIGSYS, oldsig);	/* restore previous value */
#endif

	setbuf(stdout,obuf);
	while((c = getopt(argc, argv, OPTIONS)) != EOF) {
		switch(c) {
		case 'C':
			sscanf(optarg,"%lf",&cpucut);
			continue;
		case 'O':
			sscanf(optarg,"%lf",&syscut);
			continue;
		case 'H':
			sscanf(optarg,"%lf",&hogcut);
			continue;
		case 'I':
			sscanf(optarg,"%lf",&iocut);
			continue;
		case 'a':
			average++;
			continue;
		case 'b':
			/* test if we are reading from stdin (not
			 * a pipe) which would allow reading backwards.
			 * If input from a pipe allow forward read ONLY.
			 */
			if(isatty(0) || isdevnull())
				backward++;
			else
				fprintf(stderr,"acctcom: -b on files only\n");
			continue;
#ifdef	FSS
		case 'F':
			if (fssincluded)
				option |= FSID;
			continue;
		case 'G':
			if (fssincluded) {
				if (sscanf(optarg, "%d", &fsidval) == 1)
					fsidflag++;
				else if ((fsgrp=getfsgnam(optarg)) == NULL)
					fatal("Unknown fair share group",
									optarg);
				else {
					fsidval = fsgrp->fs_id;
					fsidflag++;
				}
			}
			continue;
#endif
		case 'g':
			if(sscanf(optarg,"%hu",&gidval) == 1)
				gidflag++;
			else if((grp=getgrnam(optarg)) == NULL)
				fatal("Unknown group", optarg);
			else {
				gidval=grp->gr_gid;
				gidflag++;
			}
			continue;
		case 'h':
			option |= HOGFACTOR;
			continue;
		case 'i':
			option |= IORW;
			continue;
		case 'k':
			option |= KCOREMIN;
			continue;
		case 'm':
			option |= MEANSIZE;
			continue;
		case 'n':
			cname=(char *)cmset(optarg);
			continue;
		case 't':
			option |= SEPTIME;
			continue;
		case 'r':
			option |= CPUFACTOR;
			continue;
		case 'v':
			verbose=0;
			continue;
		case 'l':
			linedev = lintodev(optarg);
			continue;
		case 'u':
			if(*optarg == '?')
				unkid++;
			else if(*optarg == '#')
				su_user++;
			else if(sscanf(optarg, "%hu", &uidval) == 1)
				uidflag++;
			else if((pw = getpwnam(optarg)) == NULL)
				fprintf(stderr, "%s: Unknown user %s\n",
					argv[0], optarg);
			else {
				uidval = pw->pw_uid;
				uidflag++;
			}
			continue;
		case 'q':
			quiet++;
			verbose=0;
			average++;
			continue;
		case 's':
			tend_a = 1;
			strcpy(send_a, optarg);
			continue;
		case 'S':
			tstrt_a = 1;
			strcpy(sstrt_a, optarg);
			continue;
		case 'f':
			flag_field++;
			continue;
		case 'e':
			tstrt_b = 1;
			strcpy(sstrt_b, optarg);
			continue;
		case 'E':
			tend_b = 1;
			strcpy(send_b, optarg);
			continue;
		case 'o':
			ofile = optarg;
			fileout++;
			if((ostrm = fopen(ofile, "w")) == NULL) {
				perror("open error on output file");
				errflg++;
			}
			continue;
		case '?':
			errflg++;
			continue;
		}
	}

	if(errflg) {
		usage();
		exit(1);
	}

	for ( ; optind < argc; optind++){	/* Bug fix to allow acctcom */
		dofile(argv[optind]);		/* to properly process mult.*/
		nfiles++;			/* pacct file arguments.    */
	}

	if(nfiles==0) {
		if(isatty(0) || isdevnull())
			dofile(PACCT);
		else {
			stdinflg = 1;
			backward = offset = 0;
			dofile(NULL);
		}
	}
	doexit(0);
}

dofile(fname)
char *fname;
{
	register struct acct *a = &ab;
	long curtime;
	time_t	ts_a = 0,
		ts_b = 0,
		te_a = 0,
		te_b = 0;
	long	daystart;
	long	nsize;
	struct tm *t;

	if(fname != NULL)
		if(freopen(fname, "r", stdin) == NULL) {
			fprintf(stderr, "acctcom: Cannot open %s\n", fname);
			return;
		}

	if(backward) {
		backward = 0;
		if(aread() == 0)
			return;
		backward = 1;
		nsize = sizeof(struct acct);	/* make sure offset is signed */
		fseek(stdin, (long)(-nsize), 2);
	} else {
		/* not a backwards read, so we can do a normal read.
	 	 * This code used to perform a rewind. And the do while
		 * loop below was previously a while loop. The rewind allowed
	 	 * the while to re-read the first line. The rewind however
		 * did not allow this code to work correctly when the stdin
		 * was a pipe. So the rewind was removed and the first line
		 * is just re-used from the first read. All subsequent reads
		 * are performed at the bottom of the do while.
		 */
		if(aread() == 0)
			return;
	}

	/*
         * DTS: UCSqm01001, UCSqm01034
         * Note that the global timezone is relative to standard time only.
         * If in Daylight Saving Time, this value need to be reduced by 1 hr,
         * with the execption where TZ="EST?CDT" (see tztab(4)).
         */
	tzset();
	t = localtime(&(a->ac_btime));
	curtzone = (t->tm_isdst > 0 && (strcmp(tzname[0], "EST") ||
                    strcmp(tzname[1], "CDT"))) ? timezone - 3600 : timezone;

	daydiff = a->ac_btime - (a->ac_btime % SECSINDAY);
	daystart = (a->ac_btime-curtzone)-((a->ac_btime-curtzone) % SECSINDAY);
	time(&curtime);
	if(daydiff < (curtime - (curtime % SECSINDAY))) {
		/*
		 * it is older than today
		 */
		fprintf(stdout,
			"\nACCOUNTING RECORDS FROM:  %s", ctime(&a->ac_btime));
	}

	if(tstrt_a) {
		tstrt_a = convtime(sstrt_a);
		ts_a = tstrt_a + daystart;
		fprintf(stdout, "START AFT: %s", ctime(&ts_a));
	}
	if(tstrt_b) {
		tstrt_b = convtime(sstrt_b);
		ts_b = tstrt_b + daystart;
		fprintf(stdout, "START BEF: %s", ctime(&ts_b));
	}
	if(tend_a) {
		tend_a = convtime(send_a);
		te_a = tend_a + daystart;
		fprintf(stdout, "END AFTER: %s", ctime(&te_a));
	}
	if(tend_b) {
		tend_b = convtime(send_b);
		te_b = tend_b + daystart;
		fprintf(stdout, "END BEFOR: %s", ctime(&te_b));
	}
	if(ts_a) {
		if (te_b && ts_a > te_b) te_b += SECSINDAY;
	}

	/* now process the previously read line and read
 	 * the next line at the bottom of this loop.
	 */
	if(backward) {	/* already read first line above ... need last line */
		if(aread() == 0)
			return;
	}

	do {
		elapsed = expand(a->ac_etime);
		etime = a->ac_btime + (long)SECS(elapsed);
		if(ts_a || ts_b || te_a || te_b) {

			if(te_a && (etime < te_a)) {
				if(backward) return;
				else continue;
			}
			if(te_b && (etime > te_b)) {
				if(backward) continue;
				else return;
			}
			if(ts_a && (a->ac_btime < ts_a))
				continue;
			if(ts_b && (a->ac_btime > ts_b))
				continue;
		}
		if(!MYKIND(a->ac_flag))
			continue;
		if(su_user && !SU(a->ac_flag))
			continue;
		sys = expand(a->ac_stime);
		user = expand(a->ac_utime);
		cpu = sys + user;
		if(cpu == 0)
			cpu = 1;
		mem = expand(a->ac_mem);
		strncpy(command_name, a->ac_comm, 8);
		io=expand(a->ac_io);
		rw=expand(a->ac_rw);
		if(cpucut && cpucut >= SECS(cpu))
			continue;
		if(syscut && syscut >= SECS(sys))
			continue;
		if(linedev != -1 && a->ac_tty != linedev)
			continue;
		if(uidflag && a->ac_uid != uidval)
			continue;
		if(gidflag && a->ac_gid != gidval)
			continue;
#ifdef	FSS
		if (fsidflag && fsid(a) != fsidval)
			continue;
#endif
		if(cname && !cmatch(a->ac_comm,cname))
			continue;
		if(iocut && iocut > io)
			continue;
		if(unkid && uidtonam(a->ac_uid)[0] != '?')
			continue;
		if(verbose && (fileout == 0)) {
			printhd();
			verbose = 0;
		}
		if(elapsed == 0)
			elapsed++;
		if(hogcut && hogcut >= (double)cpu/(double)elapsed)
			continue;
		if(fileout)
			fwrite(&ab, sizeof(ab), 1, ostrm);
		else
			println();
		if(average) {
			cmdcount++;
			realtot += (double)elapsed;
			usertot += (double)user;
			systot += (double)sys;
			kcoretot += (double)mem;
			iotot += (double)io;
			rwtot += (double)rw;
		};
	} while(aread() != 0);
}

aread()
{
	register flag;
	static	 ok = 1;

	if(fread((char *)&ab, sizeof(struct acct), 1, stdin) != 1)
		flag = 0;
	else
		flag = 1;

	if(backward) {
		if(ok) {
			if(fseek(stdin,
				(long)(offset*sizeof(struct acct)), 1) != 0) {
					rewind(stdin);	/* get 1st record */
					ok = 0;
			}
		} else
			flag = 0;
	}
	return(flag);
}

printhd()
{
	fprintf(stdout, "COMMAND                      START    END          REAL");
	ps("CPU");
	if(option & SEPTIME)
		ps("(SECS)");
	if(option & IORW){
		ps("CHARS");
		ps("BLOCKS");
	}
	if(option & CPUFACTOR)
		ps("CPU");
	if(option & HOGFACTOR)
		ps("HOG");
	if(!option || (option & MEANSIZE))
		ps("MEAN");
	if(option & KCOREMIN)
		ps("KCORE");
	if(flag_field)
		fprintf(stdout, "        ");
#ifdef	FSS
	if (option & FSID)
		fprintf(stdout, "          ");
#endif
	fprintf(stdout, "\n");
	fprintf(stdout, "NAME       USER     TTYNAME  TIME     TIME       (SECS)");
	if(option & SEPTIME) {
		ps("SYS");
		ps("USER");
	} else
		ps("(SECS)");
	if(option & IORW) {
		ps("TRNSFD");
		ps("R/W");
	}
	if(option & CPUFACTOR)
		ps("FACTOR");
	if(option & HOGFACTOR)
		ps("FACTOR");
	if(!option || (option & MEANSIZE))
		ps("SIZE(K)");
	if(option & KCOREMIN)
		ps("MIN");
	if(flag_field)
		fprintf(stdout, "  F STAT");
#ifdef	FSS
	if (option & FSID)
		fprintf(stdout, "  FSID    ");
#endif
	fprintf(stdout, "\n");
	fflush(stdout);
}

println()
{

	char name[32];
	register struct acct *a = &ab;

	if(quiet)
		return;
	if(!SU(a->ac_flag))
		strcpy(name,command_name);
	else {
		strcpy(name,"#");
		strcat(name,command_name);
	}
	fprintf(stdout, "%-9.9s", name);
	strcpy(name,uidtonam(a->ac_uid));
	if(*name != '?')
		fprintf(stdout, "  %-8.8s", name);
	else
		fprintf(stdout, "  %-8d",a->ac_uid);
	fprintf(stdout, " %-8.8s",a->ac_tty != -1? devtolin(a->ac_tty):"?");
	fprintf(stdout, "%.9s", &ctime(&a->ac_btime)[10]);
	fprintf(stdout, "%.9s ", &ctime(&etime)[10]);
	pf((double)SECS(elapsed));
	if(option & SEPTIME) {
		pf((double)sys / HZ);
		pf((double)user / HZ);
	} else
		pf((double)cpu / HZ);
	if(option & IORW)
		fprintf(stdout, "%8ld%8ld",io,rw);
	if(option & CPUFACTOR)
		pf((double)user / cpu);
	if(option & HOGFACTOR)
		pf((double)cpu / elapsed);
	if(!option || (option & MEANSIZE))
		pf(KCORE(mem / cpu));
	if(option & KCOREMIN)
		pf(MINT(KCORE(mem)));
	if(flag_field)
		fprintf(stdout, "  %1o %3o", a->ac_flag & AFORK, a->ac_stat & 0377);
#ifdef	FSS
	if (option & FSID) {
		struct fsg *g;
		struct fsg *getfsgid();
		if ((g = getfsgid(fsid(a))) == NULL)
			fprintf(stdout, "  %-8d", fsid(a));
		else
			fprintf(stdout, "  %-8.8s", g->fs_grp);
	}
#endif
	fprintf(stdout, "\n");
}

/*
 * convtime converts time arg to internal value
 * arg has form hr:min:sec, min or sec are assumed to be 0 if omitted
 */
long
convtime(str)
char *str;
{
	long	hr, min, sec;

	min = sec = 0;

	if(sscanf(str, "%ld:%ld:%ld", &hr, &min, &sec) < 1) {
		fatal("acctcom: bad time:", str);
	}
	sec += (min*60);
	sec += (hr*3600);
	return(sec + curtzone);
}

cmatch(comm, cstr)
register char	*comm, *cstr;
{

	char	xcomm[9];
	register i;

	for(i=0;i<8;i++){
		if(comm[i]==' '||comm[i]=='\0')
			break;
		xcomm[i] = comm[i];
	}
	xcomm[i] = '\0';

	return(regex(cstr,xcomm));
}

cmset(pattern)
register char	*pattern;
{

	if((pattern=(char *)regcmp(pattern,0))==NULL){
		fatal("pattern syntax", NULL);
	}
	return((unsigned)pattern);
}

doexit(status)
{
	if(!average)
		exit(status);
	if(cmdcount) {
		fprintf(stdout, "cmds=%ld ",cmdcount);
		fprintf(stdout, "Real=%-6.2f ",SECS(realtot)/cmdcount);
		cputot = systot + usertot;
		fprintf(stdout, "CPU=%-6.2f ",SECS(cputot)/cmdcount);
		fprintf(stdout, "USER=%-6.2f ",SECS(usertot)/cmdcount);
		fprintf(stdout, "SYS=%-6.2f ",SECS(systot)/cmdcount);
		fprintf(stdout, "CHAR=%-8.2f ",iotot/cmdcount);
		fprintf(stdout, "BLK=%-8.2f ",rwtot/cmdcount);
		fprintf(stdout, "USR/TOT=%-4.2f ",usertot/cputot);
		fprintf(stdout, "HOG=%-4.2f ",cputot/realtot);
		fprintf(stdout, "\n");
	}
	else
		fprintf(stdout, "\nNo commands matched\n");
	exit(status);
}
isdevnull()
{
	struct stat	filearg;
	struct stat	devnull;

	if(fstat(0,&filearg) == -1) {
		fprintf(stderr,"acctcom: Cannot stat stdin\n");
		return(NULL);
	}
	if(stat("/dev/null",&devnull) == -1) {
		fprintf(stderr,"acctcom: Cannot stat /dev/null\n");
		return(NULL);
	}

	if(filearg.st_rdev == devnull.st_rdev) return(1);
	else return(NULL);
}

fatal(s1, s2)
char *s1, *s2;
{
	fprintf(stderr,"acctcom: %s %s\n", s1, s2);
	exit(1);
}

usage()
{
	fprintf(stderr, "Usage: acctcom [options] [files]\n");
	fprintf(stderr, "\nWhere options can be:\n");
	diag("-b	read backwards through file");
#ifdef	FSS
	if (fssincluded)
		diag("-F	print the fair share group ID");
#endif
	diag("-f	print the fork/exec flag and exit status");
	diag("-h	print hog factor (total-CPU-time/elapsed-time)");
	diag("-i	print I/O counts");
	diag("-k	show total Kcore minutes instead of memory size");
	diag("-m	show mean memory size");
	diag("-r	show CPU factor (user-time/(sys-time + user-time))");
	diag("-t	show separate system and user CPU times");
	diag("-v	don't print column headings");
	diag("-a	print average statistics of selected commands");
	diag("-q	print average statistics only");
	diag("-l line	\tshow processes belonging to terminal /dev/line");
	diag("-u user	\tshow processes belonging to user name or user ID");
	diag("-u #	\tshow processes executed by super-user");
	diag("-u ?	\tshow processes executed by unknown UID's");
#ifdef	FSS
	if (fssincluded)
		diag("-G fsgrp	show processes belonging to fair share group name");
#endif
	diag("-g group	show processes belonging to group name of group ID");
	diag("-s time	\tshow processes ending after time (hh[:mm[:ss]])");
	diag("-e time	\tshow processes starting before time");
	diag("-S time	\tshow processes starting after time");
	diag("-E time	\tshow processes ending before time");
	diag("-n regex	select commands matching the ed(1) regular expression");
	diag("-o file	\tdo not print, put selected pacct records into file");
	diag("-H factor	show processes that exceed hog factor");
	diag("-O sec	\tshow processes that exceed CPU system time sec");
	diag("-C sec	\tshow processes that exceed total CPU time sec");
	diag("-I chars	show processes that transfer more than char chars");
}
