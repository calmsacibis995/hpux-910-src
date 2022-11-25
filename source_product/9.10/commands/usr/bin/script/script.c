static char *HPUX_ID = "@(#) $Revision: 70.1 $";

/*
 * Version from Mike Stroyan, 8804.
 * Revised by AJS, 8804, to notice if arg0 starts with "-" and if so,
 * pass this on to the invoked shell.  Other changes too.
 */

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <termio.h>
#include <sys/ptyio.h>
#include <time.h>
#include <errno.h>

#define BUFFER_SIZE 1024

char *logfile;	/* log file name */
int Lfd; /* log file descriptor */
int Mfd; /* pty master file descriptor */
struct termio old_termio, new_termio;

forward_signal(sig, code, scp)
int sig, code;
struct sigcontext *scp;
{
	ioctl(Mfd, TIOCSIGSEND, sig);
	scp->sc_syscall_action = SIG_RESTART;
}

handle_signal (sig)
int sig;
{
	log_date ("\nscript done due to signal on ");
	printf ("Script done due to signal %d, file is %s\r\n", sig, logfile);
	clean_up();
}

clean_up()
{
	/* return stdin termio to its original state */
	if (ioctl(0, TCSETA, &old_termio) == -1) {
		perror("script: error setting termio on stdin");
	}
	exit(0);
}

char *open_pty();

main(argc, argv)
int argc;
char *argv[];
{
	extern int optind;
	extern char *optarg;
	int opt;
	int trunc_or_append = O_TRUNC; 
	int error = 0;
	struct sigvec vec;

	while ((opt = getopt(argc, argv, "a")) != EOF) {
		switch (opt) {
		case 'a': 
			trunc_or_append = O_APPEND; 
			break;

		default:  
			error = 1; 
			break;
		}
	}

	if ( optind == argc - 1 ) {
		logfile = argv[optind];
	} else if ( optind > argc - 1 ) {
		logfile = "typescript";
	} else {
		error = 1;
	}

	if (error) {
		fprintf(stderr, "usage: script [ -a ] [ typescript ]\n");
		exit(1);
	}

	Lfd = open(logfile, O_WRONLY | O_CREAT | trunc_or_append, 0666);
	if (Lfd == -1) {
		perror("script: couldn't open log file");
		exit(1);
	}

	log_date ("Script started on ");
	printf ("Script started, file is %s\n", logfile);

	/* read termio settings from stdin */
	if (ioctl(0, TCGETA, &old_termio) == -1) {
		perror("script: error reading termio on stdin");
		exit(1);
	}

	/* Pass on SIGINTs and SIGQUITs.  If they weren't from termio, so be it. */
	vec.sv_mask = 0;
	vec.sv_onstack = 0;
	vec.sv_handler = forward_signal;
	sigvector(SIGINT, &vec, (struct sigvec *) 0);
	sigvector(SIGQUIT, &vec, (struct sigvec *) 0);

	/*
	 * Ignore death of children; the only interesting case is noticed
	 * in a different fashion.
	 */
	vec.sv_handler = SIG_IGN;
	sigvector(SIGCLD, &vec, (struct sigvec *) 0);

	/*
	 * Make announcement and clean up termio if killed:
	 */
	vec.sv_handler = handle_signal;
	sigvector(SIGHUP,    &vec, (struct sigvec *) 0);
	sigvector(SIGILL,    &vec, (struct sigvec *) 0);
	sigvector(SIGTRAP,   &vec, (struct sigvec *) 0);
	sigvector(SIGIOT,    &vec, (struct sigvec *) 0);
	sigvector(SIGEMT,    &vec, (struct sigvec *) 0);
	sigvector(SIGFPE,    &vec, (struct sigvec *) 0);
	sigvector(SIGBUS,    &vec, (struct sigvec *) 0);
	sigvector(SIGSEGV,   &vec, (struct sigvec *) 0);
	sigvector(SIGSYS,    &vec, (struct sigvec *) 0);
	sigvector(SIGPIPE,   &vec, (struct sigvec *) 0);
	sigvector(SIGALRM,   &vec, (struct sigvec *) 0);
	sigvector(SIGTERM,   &vec, (struct sigvec *) 0);
	sigvector(SIGUSR1,   &vec, (struct sigvec *) 0);
	sigvector(SIGUSR2,   &vec, (struct sigvec *) 0);
	sigvector(SIGVTALRM, &vec, (struct sigvec *) 0);

	/* set stdin to raw */
	new_termio = old_termio;
	new_termio.c_iflag = 0;
	new_termio.c_oflag = 0;
	new_termio.c_lflag = 0;
	new_termio.c_line  = 0;
	new_termio.c_cc[VMIN]  = 001;
	new_termio.c_cc[VTIME] = 001;

	if (ioctl(0, TCSETA, &new_termio) == -1) {
		/*
		 * Note: clean_up() is not called in this case, so nothing
		 * about the abort is logged...
		 */
		perror("script: error setting termio on stdin");
		exit(1);
	}

	/* tell it if program invocation name starts with "-": */

	start_shell (open_pty(), ((argc > 0) && (**argv == '-')));

	run();

	log_date ("\nscript done on ");
	printf ("Script done, file is %s\r\n", logfile);

	close(Lfd);
	close(Mfd);

	/* return stdin termio to its original state */
	clean_up();
}

log_date (prefix)
char *prefix;		/* precedes date value */
{
	long date_number  = time ((long *) 0);
	char *date_string = ctime (& date_number);

	write (Lfd, prefix, strlen (prefix));
	write (Lfd, date_string, strlen (date_string));
}

char *
open_pty()
{
	char letter = 'p';
	int num = 0;
	static char name[20];
	int enable = 1;

	/* Get a master pty */
	do {
		sprintf(name, "/dev/ptym/pty%c%.1x", letter, num);
		Mfd = open(name, O_RDWR);
		if ( Mfd == -1 )
		{
			if ( ++num > 0xf ) {
				num = 0;
				if ( ++letter > 'z' ) {
					fprintf(stderr, "script: Out of ptys\n");
					clean_up();
				}
			}

			if ((errno != EBUSY) && (errno != EACCES))
			{
				fprintf (stderr,
			"script: warning: errno %d attempting to open pty %s\n",
					errno, name);
			}
		}
	} while (Mfd == -1); 

/*
 * Used to say, more precisely but less robustly, and without the printed
 * warning above:
 *
 *	} while ( (Mfd == -1) && ( (errno == EBUSY) || (errno == EACCES) ) );
 *
 *
 *	if ( Mfd == -1 ) {
 *		perror("script: error opening Pty master");
 *		clean_up();
 *	}
 */

	/* enable pty trapping of opens, closes, and ioctls */
	ioctl(Mfd, TIOCTRAP, &enable);

	/* make slave side name */
	sprintf(name, "/dev/pty/tty%c%.1x", letter, num);
	return (name);
}

start_shell (tty_name, loginshell)
char *tty_name;
int loginshell;		/* run shell as login shell? */
{
	char shell[100], shell_base[100];
	char *getenv();
	int Sfd; /* pty slave file descriptor */

	strcpy(shell, getenv("SHELL"));
	if ( shell[0] == '\0' ) strcpy(shell, "/bin/sh");

	strcpy (shell_base, loginshell ? "-" : "");

	if (strrchr(shell, '/') == NULL)
		strcat (shell_base, shell);
	else
		strcat (shell_base, strrchr (shell, '/') + 1);

	switch (fork()) {

	case -1:
		perror ("script: fork failed");
		clean_up();
		break;	/* just in case */

	case 0:		/* child */
		setpgrp();	/* Start a new terminal process group */

		Sfd = open(tty_name, O_RDWR);
		if (Sfd == -1) {
			char message[128];
			sprintf(message, "script: error opening Pty slave (%s)", tty_name);
			perror(message);
			putc ('\r', stderr);   /* since stderr is in raw mode */

			/* make sure parent doesn't hang around! */
			kill (getppid(), SIGTERM);

			exit(1);
		}

		close(Lfd);
		close(Mfd);
		close(0);
		close(1);
		close(2);
		dup(Sfd);  /* stdin */
		dup(Sfd);  /* stdout */
		dup(Sfd);  /* stderr */
		close(Sfd);

		/* copy this program's original tty settings to shell's */

		if (ioctl(0, TCSETA, &old_termio) == -1) {
			perror("script: error setting termio on slave pty");
			exit(1);
		}

		execl(shell, shell_base, 0);

		perror("script: exec of shell failed");
		exit(1);
	
	default:	/* parent -- just return */
		break;
	}
}

#define BITS_PER_INT (8*sizeof(int))
#define NWORDS(n) ((n)%32==0 ? (n)>>5 : ((n)>>5)+1)

run()
{
	int fds;
	int *readfds, *exceptfds;
	char buffer[BUFFER_SIZE];	/* For reads, writes and ioctl args */
	int ret = 0;
	int i;
	struct request_info IOStuff;

	
	fds=getnumfds();
	/* allocate bitmasks and zero */
	/* readfds=(int *)calloc(sizeof(int)*NWORDS(fds)); */
	/* exceptfds=(int *)calloc(sizeof(int)*NWORDS(fds)); */

	readfds=(int *)calloc(NWORDS(fds),sizeof(int));
	exceptfds=(int *)calloc(NWORDS(fds),sizeof(int));

	do {
		/* select ( 0, Mfd ) and read loop */
		readfds  [0   / BITS_PER_INT] |= 1 << (0   % BITS_PER_INT);
		readfds  [Mfd / BITS_PER_INT] |= 1 << (Mfd % BITS_PER_INT);
		exceptfds[Mfd / BITS_PER_INT] |= 1 << (Mfd % BITS_PER_INT);

		ret = select(fds,readfds,(int *) 0,exceptfds,(struct timeval *) 0);
		if (ret < 1)
			perror("script: select failed");
			/* will exit loop below */

		/*
		 * Have data from stdin ("terminal"), pass it through:
		 */

		if ( readfds[0 / BITS_PER_INT] & (1 << (0 % BITS_PER_INT)) ) {
			if ( (ret=read(0, buffer, BUFFER_SIZE)) > 0 ) {
				if (write(Mfd, buffer, ret) < ret)
					perror("script: pty write failed");
			} else {
				perror("script: stdin read failed");
			}
		}

		/*
		 * Have data from master side (shell); pass it on to the
		 * "terminal" and log it:
		 */

		if ( readfds[Mfd / BITS_PER_INT] & (1 << (Mfd % BITS_PER_INT)) ) {
			if ( (ret=read(Mfd, buffer, BUFFER_SIZE)) > 0 ) {
				if (write(1, buffer, ret) < ret)
					perror("script: stdout write failed");
				if (write(Lfd, buffer, ret) < ret)
					perror("script: logfile write failed");
			} else {
				if (ret == -1)
					perror("script: pty read failed");
			}
		}

		/*
		 * Have exception; handle it:
		 */

		if ( exceptfds[Mfd / BITS_PER_INT] & (1 << (Mfd % BITS_PER_INT)) ) {
			/* Handle an ioctl from the slave side of the pty */

			if (ioctl(Mfd, TIOCREQGET, &IOStuff) == -1)
				perror("script: ioctl TIOCREQGET failed");
			switch (IOStuff.request) {

			case TIOCOPEN:
				break;

			case TIOCCLOSE:	/* last user of slave pty closed it */
				return;	/* so it's time for us to quit too  */

			default: /* all IOCTLs */

				/* If necessary, get ioctl structure */
				if (IOStuff.argget)
					if (ioctl(Mfd, IOStuff.argget, buffer) == -1)
						perror("script: ioctl IOStuff.argget failed");

				/*
				 * Set return and errno values from stdin.
				 */
				errno = 0;
				IOStuff.return_value = ioctl(0, IOStuff.request, buffer);
				IOStuff.errno_error = errno;

				/* If necessary, put ioctl structure */
				if (IOStuff.argset)
					if (ioctl(Mfd, IOStuff.argset, buffer) == -1)
						perror("script: ioctl IOStuff.argset failed");

				break;
			}

			/* Handshake the ioctl */
			if (ioctl(Mfd, TIOCREQSET, &IOStuff) == -1)
				perror("script: ioctl TIOCREQSET failed");

		} /* if */
	} while ( ret != -1 );
}
