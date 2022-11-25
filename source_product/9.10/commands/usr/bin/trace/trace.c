/*

Be sure we don't have a security problem with file tracing

*/

#define FILE_TRACE
#include <stdio.h>
#include <fcntl.h>
#include <nlist.h>
#include <unistd.h>
#include <time.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/syscall.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/strace.h>

static char *HPUX_ID = "@(#) $Revision: 70.12 $";


/*
 *	trace - allow user to watch system calls a process or family of
 *		processes is making - see usage() for options
 */

extern int errno, sys_nerr;
extern char *sys_errlist[];
extern char *malloc();

float time_delta();
void catch_sig();

 
struct nlist nl[] = {
#ifdef hp9000s800
	{ "nsysent" },
	{ "sysent" },
#else
	{ "_nsysent" },
	{ "_sysent" },
#endif
	{ "" }
};

char *sc_names[500];		/*  XXX should malloc  */
struct sysent *sysent_tbl;
int nsysent;
int *sc_counts;
float *sc_times;



int exits = 0, forks = 0, err = 0; 
#ifdef FILE_TRACE
int cflag = 0, dflag = 0, follow = 0, tflag = 0, Tflag = 0, aflag = 0;
#else
int cflag = 0, dflag = 0, follow = 0, tflag = 0, Tflag = 0;
#endif
extern char *optarg;
FILE *outfile = stderr;
char out_file_name[MAXPATHLEN];

int first_fd, highest_fd;
fd_set readset, exceptset;




main(argc, argv)
int argc;
char *argv[];
{
	int pid = 0;
	char buf[TRACE_BUF_SIZE];
	int n, i, j;
	fd_set tmpread, tmpexc;
#ifdef FILE_TRACE
	char *path;
		

	path = malloc(128);
#endif	
	/*
	 *	get system call names, # of arguments, etc, and start child
	 */
	get_syscalls();	

#ifdef FILE_TRACE	 
	get_arguments(argc, argv, &pid, path);
	if (aflag == 0 && pid == 0) 
#else
	get_arguments(argc, argv, &pid);
	if (pid == 0) 
#endif
		pid = start_child(argv);

#ifdef FILE_TRACE
	setup_files(pid, path);
#else
	setup_files(pid);
#endif
	setup_signals();
	
	/*
	 *	main loop - select on file descriptors (one per process
	 *	or pathname we are tracing), read buffers from the kernel
	 *	as they become available, and stop when the last process
	 *	we care about has exited
	 */
	do {	
		tmpread = readset;
		tmpexc = exceptset;
		n = select(highest_fd+1, &tmpread, NULL, &tmpexc, NULL);
		if (n <= 0) {
			if (n < 0)
				perror("select");
			break;
		}

		for (i = first_fd; i <= highest_fd; i++) {
			if (FD_ISSET(i, &tmpread)) {
				n = read(i, buf, sizeof(buf));
				if (n < (int) TR_LEN) {
					if (errno != ESRCH)
						fprintf(stderr, "short read: n is %d, errno is %d\n", n, errno);
					break;		
				} else if (dflag)
					fprintf(outfile, "\n>>> Got %d records\n", n/TR_LEN);

				process_buf((struct trace_record *) buf, n, i);
				fflush(outfile);
			}
			if (FD_ISSET(i, &tmpexc)) 
				close_trace(i);
		}
	} while (highest_fd >= first_fd);

	if (dflag)
		printf("Exiting: n is %d, highest_fd is %d\n", n, highest_fd);
		
	if (cflag)
		print_counts();
	fclose(outfile);
}




/*
 *	-c option (just print counts)
 */
print_counts()
{
	int i;
	

	for (i = 0; i < nsysent; i++)
		if (sc_counts[i] > 0) {
			fprintf(outfile, "%d %s %d", i, sc_names[i], sc_counts[i]);
			if (Tflag)
				fprintf(outfile, " (%4.2f)\n", sc_times[i]);
			else
				fputc('\n', outfile);
		}
}



setup_signals()
{
	struct sigaction act;


	/*
	 *	we want to ignore SIGINT so if user hits ^c it gets
	 *	through to application; catch SIGQUIT (^\) and SIGTERM
	 *	so user can kill us easily
	 */
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);
	act.sa_handler = catch_sig;
	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
}



#ifdef FILE_TRACE
setup_files(pid, path)
#else
setup_files(pid)
#endif
int pid;
#ifdef FILE_TRACE
char *path;
#endif
{
	int n, fd;
	int follow_flags = TD_FOLLOW;
	

	/*
	 *	open the device and get it set up properly
	 */	
	if ((fd = open("/dev/trace", O_RDWR)) < 0) {
		n = errno;
		perror("/dev/trace");
		if (n == ENODEV || n == ENXIO)
			fprintf(stderr, "Is the \"strace\" driver in the kernel?\n");
		exit(1);
	}	

#ifdef FILE_TRACE
	if (aflag && (ioctl(fd, STRACE_SET_PATH, &path) < 0)) {
		perror(path);
		exit(2);
	} else if (!aflag)
#endif	
	if (ioctl(fd, STRACE_SET_PID, &pid) < 0) {
		perror("ioctl");
		exit(2);
	}

	if (follow && (ioctl(fd, STRACE_SET_FLAGS, &follow_flags) < 0)) {
		perror("ioctl");
		exit(2);
	}

	first_fd = highest_fd = fd;
	FD_ZERO(&readset);
	FD_ZERO(&exceptset);
	FD_SET(first_fd, &readset);
	FD_SET(first_fd, &exceptset);		

	if (outfile == NULL)
		outfile = fopen(out_file_name, "w");
	if (outfile == NULL) {
	  	perror(out_file_name);
	   	exit(1);
	}
}




open_trace(trp)
struct trace_record *trp;
{
	int fd;
	

	if (dflag)
		fprintf(outfile, "open called for PID %d\n", trp->t_params[0]);
	fd = open("/dev/trace", O_RDWR);
	if (fd < 0) {
		perror("open");
		return(0);
	}

	trp->t_rv = trp->t_params[0];
	
	if (ioctl(fd, STRACE_NEXT_CHILD, &trp->t_rv) < 0) {
		close(fd);
		if (errno == EALREADY)
			return(0);
		if (errno == EPERM)
			fprintf(outfile, "PID %d's UID does not match yours (%d); permission denied\n", 
				trp->t_params[0], getuid());
		else
			fprintf(outfile, "open_trace: ioctl for PID %d failed; errno is %d\n",
				trp->t_params[0], errno);
		return(0);
	}
	
	if (fd > highest_fd)
		highest_fd = fd;

	if (dflag)
		fprintf(outfile, "PID %d is starting; fd, highest_fd are %d %d\n",
			trp->t_rv, fd, highest_fd);
				
	FD_SET(fd, &readset);
	FD_SET(fd, &exceptset);			
}



close_trace(fd)
int fd;
{
	int j;
	

	close(fd);
	FD_CLR(fd, &readset);
	FD_CLR(fd, &exceptset);

	if (fd == highest_fd) {
		j = fd;
		while (--j > 0) 
			if (FD_ISSET(j, &readset)) {
				highest_fd = j;
				break;
			} 
			
		if (j <= 0)
			highest_fd = 0;
	}

	if (dflag)
		printf("fd %d closed; highest is now %d\n", fd, highest_fd);

}




/*
 *   arrange to exit - kind of kludgey, but effective :-)
 */
void catch_sig()
{
	highest_fd = 0;
}



/*
 *   deal with one or more trace records
 *	- if we're following children, must count forks
 *	- we ignore T_BOGUS records - is this OK?
 */
process_buf(trp, n, this_fd)
struct trace_record *trp;
int n;
int this_fd;
{
	int count;
	int fd;
	

	while (n >= (int) TR_LEN) {

		if (dflag) {	
			fprintf(outfile, "n=%d flags=%#x sc=%d PID=%d ns=%d\n", 
				n, trp->t_flags, trp->t_syscall, trp->t_pid, trp->t_ns);
			fflush(outfile);
		}

	 	if ((trp->t_flags & T_SIGNAL) && follow && trp->t_syscall == -1)
		    	open_trace(trp);

		if ((trp->t_syscall == SYS_EXIT && !trp->t_flags) || (trp->t_flags & T_SETUID))
			close_trace(this_fd);

		if (trp->t_syscall > 0 && trp->t_syscall < nsysent) {
			if (!(trp->t_flags & T_BOGUS))
				if (trp->t_flags & T_SIGNAL)
					print_signal(trp);
				else if (cflag) {
				 	if ((trp->t_flags & T_CALL) == 0) {
						sc_counts[trp->t_syscall]++;
						if (Tflag)
							sc_times[trp->t_syscall] += time_delta(trp->t_entry_time, trp->t_exit_time);
					}
				} else
					print_record(trp);
			/*
			 *	jump over any trailing strings/signals
			 */
			count = trp->t_ns;
			while (count--) {
				trp++;
				n -= TR_LEN;
			}
		}
		trp++;
		n -= TR_LEN;
	}	
}
		


extern char *ctime();
#define max(a, b) ((a) >= (b) ? (a) : (b))


print_record(trp)
struct trace_record *trp;
{
	char *delim;	
	int i;
	char s[TR_LEN];
	char date[21];
	int sec_delta, mic_delta;
		

	if ((trp->t_flags & T_CALL) && (trp->t_flags & T_ALREADY)) {
		look_for_signals(trp);
		return(0);
	}
	if (dflag)
		fprintf(outfile, "%d:%d ", trp->t_pid, trp->t_spare2);	
	else if (follow)
		fprintf(outfile, "%d ", trp->t_pid);

	date[8] = date[20] = '\0';
	if (tflag) {
		strncpy(date, ctime(&trp->t_entry_time.tv_sec)+11, 8);
		fprintf(outfile, "%s.%d ", date, (trp->t_entry_time.tv_usec + 500)/1000);
	}

	fprintf(outfile, "%s", sc_names[trp->t_syscall]);
	if (sysent_tbl[trp->t_syscall].sy_narg)
		delim = "(";
	else
		fputc('(', outfile);

	/*
	 *   print the parameters, including strings and signals that may
	 *   be in "trailer" records
	 *	- we parse the arguments to ioctl and open
	 */		
	for (i = 0; i < sysent_tbl[trp->t_syscall].sy_narg; i++) {
		if ((trp->t_syscall == SYS_IOCTL) && (i == 1))	   
			print_ioctl(trp->t_params[i]);
		else if ((trp->t_syscall == SYS_OPEN) && (i == 1)) {
			print_open(trp);
			break;
		} else if (i < trp->t_ns && ((trp+i+1)->t_flags & T_STRING)) {
			s[STR_LEN] = '\0';
			strncpy(s, &(trp+i+1)->t_syscall, STR_LEN);
			fprintf(outfile, "%s\"%s\"", delim, s);		
		} else if ((trp->t_params[i] >= 0) && (trp->t_params[i] < 0x10))	/*  normal  */
			fprintf(outfile, "%s%d", delim, trp->t_params[i]);
		else
			fprintf(outfile, "%s%#x", delim, trp->t_params[i]);
		delim = ", ";
	}
	
	if (trp->t_flags & T_ARGC)	/*  didn't get all the args  */
		fprintf(outfile, ",..");
	if (trp->t_flags & T_CALL)	/*  don't have return value yet  */
		fprintf(outfile, ")...");	
	else if (trp->t_flags & T_ERROR) {
		if (trp->t_rv >= 1 && trp->t_rv <= sys_nerr)
			fprintf(outfile, ") ==> Error %d (%s)", 
				trp->t_rv, sys_errlist[trp->t_rv]);
		else
			fprintf(outfile, ") ==> Error %d", trp->t_rv);		
	} else		
		fprintf(outfile, ") ==> %d == 0x%x", trp->t_rv, trp->t_rv);
		
	if (Tflag && !(trp->t_flags & T_CALL)) {
		strncpy(date, ctime(&trp->t_exit_time.tv_sec)+11, 8);
		fprintf(outfile, " %s.%d ", date, (trp->t_exit_time.tv_usec + 500)/1000);
		fprintf(outfile, "(%4.2f)", time_delta(trp->t_entry_time, trp->t_exit_time));
	}
	fputc('\n', outfile);	
	look_for_signals(trp);
}


look_for_signals(trp)
struct trace_record *trp;
{
	int i;
	

	for (i = 0; i < trp->t_ns; i++)
		if ((trp+i+1)->t_flags & T_SIGNAL) 
			print_signal(trp+i+1);
}



char *sig_names[] = {
	"0",
	"SIGHUP",
	"SIGINT",
	"SIGQUIT",
	"SIGILL",
	"SIGTRAP",
	"SIGABRT",
	"SIGEMT",
	"SIGFPE",
	"SIGKILL",
	"SIGBUS",
	"SIGSEGV",
	"SIGSYS",
	"SIGPIPE",
	"SIGALRM",
	"SIGTERM",
	"SIGUSR1",
	"SIGUSR2",
	"SIGCHLD",
	"SIGPWR",
	"SIGVTALRM",
	"SIGPROF",
	"SIGIO",
	"SIGWINCH",
	"SIGSTOP",
	"SIGTSTP",
	"SIGCONT",
	"SIGTTIN",
	"SIGTTOU",
	"SIGURG",
	"SIGLOST",
	"SIGRESERVE",
	"SIGDIL"
};


print_signal(trp)
struct trace_record *trp;
{
	char date[21];


 	if ((trp->t_flags & T_SIGNAL) && follow && trp->t_syscall == -1) {
	    	open_trace(trp);
	    	return(0);
	}
	
	if (trp->t_flags & T_ALREADY)
		return(0);

	if (follow || dflag)
		fprintf(outfile, "%d ", trp->t_pid);
	if (tflag) {
		strncpy(date, ctime(&trp->t_entry_time.tv_sec)+11, 8);
		fprintf(outfile, "%s.%d ", date, (trp->t_entry_time.tv_usec + 500)/1000);
	}
	if (trp->t_syscall > 0 && trp->t_syscall <= 32)
		fprintf(outfile, "Signal %s", sig_names[trp->t_syscall]);
	else
		fprintf(outfile, "Signal %d", trp->t_syscall);
	fprintf(outfile, " from PID %d", trp->t_params[0]);
	fputc('\n', outfile);
}



print_open(trp)
struct trace_record *trp;
{
	int mode;
	

	mode = trp->t_params[1];
	fprintf(outfile, ", ");
		
	if ((mode & O_ACCMODE) == O_RDONLY)
		fprintf(outfile, "O_RDONLY");
	if ((mode & O_ACCMODE) == O_WRONLY)
		fprintf(outfile, "O_WRONLY");
	if ((mode & O_ACCMODE) == O_RDWR)
		fprintf(outfile, "O_RDWR");

	if (mode & O_CREAT)
		fprintf(outfile, " | O_CREAT");
	if (mode & O_TRUNC)
		fprintf(outfile, " | O_TRUNC");
	if (mode & O_EXCL)
		fprintf(outfile, " | O_EXCL");
	if (mode & O_NOCTTY)
		fprintf(outfile, " | O_NOCTTY");
	if (mode & O_APPEND)
		fprintf(outfile, " | O_APPEND");
	if (mode & O_NONBLOCK)
		fprintf(outfile, " | O_NONBLOCK");
	if (mode & O_SYNC)
		fprintf(outfile, " | O_SYNC");
	if (mode & O_NDELAY)
		fprintf(outfile, " | O_NDELAY");

	if (mode & O_CREAT)
		fprintf(outfile, ", %#o", trp->t_params[2]);
}


#define _INCLUDE_TERMIO
#define _TERMIOS_INCLUDED
#include <termio.h>
#include <bsdtty.h>



print_ioctl(cmd)
int cmd;
{
	fprintf(outfile, ", ");
	if (cmd == _IOR('T', 16, struct termios))
		fprintf(outfile, "TCGETATTR");
	else if (cmd == _IOW('T', 17, struct termios))
		fprintf(outfile, "TCSETATTR");
	else if (cmd == _IOW('T', 18, struct termios))
		fprintf(outfile, "TCSETATTRD");
	else if (cmd == _IOW('T', 19, struct termios))
		fprintf(outfile, "TCSETATTRF");
	else if (cmd == _IOR('T', 1, struct termio))
		fprintf(outfile, "TCGETA");
	else if (cmd == _IOW('T', 2, struct termio))
		fprintf(outfile, "TCSETA");
	else if (cmd == _IOW('T', 3, struct termio))
		fprintf(outfile, "TCSETAW");
	else if (cmd == _IOW('T', 4, struct termio))
		fprintf(outfile, "TCSETAF");
	else if (cmd == _IO('T', 5))
		fprintf(outfile, "TCSBRK");
	else if (cmd == _IO('T', 6))
		fprintf(outfile, "TCXONC");
	else if (cmd == _IO('T', 7))
		fprintf(outfile, "TCFLSH");
	else if (cmd == _IO('T', 33))
		fprintf(outfile, "TIOCSCTTY");
	else if (cmd == _IO('t', 104))
		fprintf(outfile, "TIOCCONS");
	else if (cmd == _IOR('t', 107, struct winsize))
		fprintf(outfile, "TIOCGWINSZ");
	else if (cmd == _IOW('t', 106, struct winsize))
		fprintf(outfile, "TIOCSWINSZ");
	else if (cmd == _IO('D', 0))
		fprintf(outfile, "LDIOC");
	else if (cmd == _IO('D', 0))
		fprintf(outfile, "LDOPEN");
	else if (cmd == _IO('D', 1))
		fprintf(outfile, "LDCLOSE");
	else if (cmd == _IO('D', 2))
		fprintf(outfile, "LDCHG");
	else if (cmd == _IOW('T', 32, int))
		fprintf(outfile, "TCDSET");
	else if (cmd == _IOW('T', 23, struct ltchars))
		fprintf(outfile, "TIOCSLTC");
	else if (cmd == _IOR('T', 24, struct ltchars))
		fprintf(outfile, "TIOCGLTC");
	else if (cmd == _IOW('T', 25, int))
		fprintf(outfile, "TIOCLBIS");
	else if (cmd == _IOW('T', 26, int))
		fprintf(outfile, "TIOCLBIC");
	else if (cmd == _IOW('T', 27, int))
		fprintf(outfile, "TIOCLSET");
	else if (cmd == _IOR('T', 28, int))
		fprintf(outfile, "TIOCLGET");
	else if (cmd == _IOW('T', 29, int))
		fprintf(outfile, "TIOCSPGRP");
	else if (cmd == _IOR('T', 30, int))
		fprintf(outfile, "TIOCGPGRP");
	else 
		fprintf(outfile, "%x", cmd);
}



float time_delta(start_time, stop_time)
struct timeval start_time, stop_time;
{
	int sec_delta, mic_delta;
	
	
	sec_delta = stop_time.tv_sec - start_time.tv_sec;
	mic_delta = stop_time.tv_usec - start_time.tv_usec;
	if (mic_delta < 0) {
		sec_delta--;
		mic_delta += 1000000;
	}
	return(sec_delta + mic_delta/1000000.0);
}	




/*
 *   start a child process - should this do fancier things with signals,
 *   process groups, etc?
 *
 *   note the magic 200ms delay below - this is so the parent can start
 *   tracing us before we really do any of our own system calls
 */
start_child(argv)
char *argv[];
{
	int pid;
	struct timeval tv;
	

	pid = fork();
	switch(pid) {
			
		case -1 : 
			perror("fork");
			exit(1);
				
		case 0 : 
			tv.tv_sec = 0;
			tv.tv_usec = 200000;  	/*  200ms  */
			select(0, 0, 0, 0, &tv);
			execvp(argv[optind], &argv[optind]);
			perror("exec");
			exit(1);
			
		default: 
			return(pid);
	}
}



/*
 *   get options user specified
 *	- note that -T and -d are undocumented/unsupported
 */
#ifdef FILE_TRACE
get_arguments(argc, argv, pidp, path)
#else
get_arguments(argc, argv, pidp)
#endif
int argc;
char *argv[];
int *pidp;
#ifdef FILE_TRACE
char *path;
#endif
{
	char ch;
	

#ifdef FILE_TRACE
	while ((ch = getopt(argc, argv, "a:cdftTo:p:")) != EOF)
#else	
	while ((ch = getopt(argc, argv, "cdftTo:p:")) != EOF)
#endif	
		switch (ch) {
#ifdef FILE_TRACE
			case 'a' : 
				aflag++;
				strncpy(path, optarg, 128);
				break;
#endif			
			case 'c' : 
				cflag++;
				break;
			
			case 'd' :
				dflag++;
				break;
				
			case 'f' : 
				follow++;
				break;
				  
			case 'o' : 
				strncpy(out_file_name, optarg, MAXPATHLEN-1);
				outfile = NULL;
				break;
				   
			case 'p' : 
				*pidp = atoi(optarg);
				break;
				
			case 't' :
				tflag++;
				break;
							
			case 'T' :
				Tflag++;
				break;
							
			default : err++;
		}
		
	if (err || 
#ifdef FILE_TRACE
	    (aflag == 0) &&
#endif	    	
	    (*pidp == 0) && (optind >= argc) || 
	    *pidp && (optind < argc))
		usage(argv[0]);		/*  exits after giving help...  */
}



usage(name)
char *name;
{
	fprintf(stderr, "\nUsage: %s [-c] [-f] [-o output filename] [-p PID] [-t] [cmd to run]\n", name);
	fprintf(stderr, "\t [-c] - print counts of systemcalls made rather than printing each one\n");
	fprintf(stderr, "\t [-f] - follow forks/vforks (include child processes)\n");
	fprintf(stderr, "\t [-o filename] - send output to <filename> rather than stderr\n");	
	fprintf(stderr, "\t [-p PID] - start tracing process <PID>; required if no <cmd> specified\n");		
	fprintf(stderr, "\t [-t] - print time just before each system call\n");
	exit(1);
}




/*
 *   get the number and names of the system calls - we get the number of
 *   calls and how many parameters they take from sysent[] in the kernel;
 *   we get names from /usr/include/sys/syscall.h
 *
 *   XXX is there a better way?
 */
get_syscalls()
{
	FILE *fp;
	int fd, n;
	char s[255]; 
	struct stat statbuf1, statbuf2;
#ifdef hp9000s300		
	char *syscall_names = "/tmp/.300syscall_names";
#else
	char *syscall_names = "/tmp/.700syscall_names";
#endif

	if (nlist("/hp-ux", nl) < 0) {
		perror("nlist");
		exit(5);
	}
	
	if ((fd = open("/dev/kmem", O_RDONLY)) < 0) {
		perror("/dev/kmem");
		exit(6);
	}
	
	setgid(getgid());	/*  give up our setGID status  */

	/*
	 *   get nsysent, calloc space, and get sysent[]
	 */
	lseek(fd, nl[0].n_value, 0);
	read(fd, &nsysent, sizeof(nsysent));
	sysent_tbl = (struct sysent *) calloc(nsysent, sizeof(struct sysent));
	lseek(fd, nl[1].n_value, 0);
	read(fd, sysent_tbl, nsysent*sizeof(struct sysent));

	close(fd);
	sc_counts = (int *) calloc(nsysent, sizeof(int));
	sc_times = (float *) calloc(nsysent, sizeof(float));

	/*
	 *   get the names of system calls; if there's a valid cache
	 *   of them, use it; if not, massage syscall.h
	 */
	if ((stat("/usr/include/sys/syscall.h", &statbuf1) == 0) &&
	    ((stat(syscall_names, &statbuf2) < 0) ||
	    (statbuf2.st_mtime < statbuf1.st_mtime))) {
#ifdef hp9000s300	    	
		sprintf(s, "unifdef -U__hp9000s800 -D__hp9000s300 /usr/include/sys/syscall.h | grep 'fine[ 	]SYS_[A-Z]' | awk '{print $3, $2}' | sed 's/SYS_//g' | tr '[A-Z]' '[a-z]' | sort -n > %s", syscall_names);
#else		
		sprintf(s, "unifdef -D__hp9000s800 -U__hp9000s300 /usr/include/sys/syscall.h | grep 'fine[ 	]SYS_[A-Z_]*[ 	]*[0-9]' | awk '{print $3, $2}' | sed 's/SYS_//g' | tr '[A-Z]' '[a-z]' | sort -n > %s", syscall_names);
#endif
		system(s);
		chmod(syscall_names, 0666);
	}

	if ((fp = fopen(syscall_names, "r")) == NULL) {
		perror(syscall_names);
		exit(7);
	}
	while (fscanf(fp, "%d %s\n", &n, s) > 0) {
		if (n >= nsysent) {
			fprintf(stderr, "Error in reading %s: %d not a valid system call number\n", syscall_names, n);
			continue;
		}
		sc_names[n] = malloc(strlen(s) + 1);
		strcpy(sc_names[n], s);
	}
	fclose(fp);
}


