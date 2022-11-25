/* @(#) $Revision: 66.5 $ */
/*
 *  Program to act as a front end to any program to provide ksh-style
 *  editing.   Originally by Donn Terry, May 1989.
 *
 *  This version updated for 7.0.
 *  The -x flag was removed because the bug in the PTY driver that it
 *  worked around no longer was a problem.  The source is retained with
 *  the conditional compilation flag BAD_PTY
 *
 */

#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <termio.h>
#include <sys/ptyio.h>
#include <time.h>
#include <setjmp.h>
#include "defs.h"
#include "edit.h"
#include "history.h"

/* Down in the works the line buffer is partitioned 3 ways, each
   of MAXLINE genchars long */
#define LINELEN 3*MAXLINE*sizeof(genchar)
#define MASTER 0
#define SLAVE 1
#define READ 0
#define WRITE 1
#define FAIL 254

/*
 * The architecture here is a bit strange: there really are two main loops,
 * operating more or less as coroutines.  The "top" main loop simply does
 * reads on the tty thru the editing package and feeds the results to the
 * slave process a line at a time as it gets them.  The edit package does
 * its' input through ee_read, which appears below.  This routine is the
 * one that's really aware of what's going on: it uses select on the tty
 * and on the pty to shuttle data back and fourth.  It presumes that the
 * top loop will end up calling it quite often, such that from the user's
 * point of view it's almost always at the select waiting for something
 * to happen.  When "something" is "raw mode" it simply takes over completely
 * from the top loop, which is none the wiser.
 */

FILE *fopen();

int done();
void catcher();

char line[LINELEN];

int quitc;	/* counts SIGQUITS for abort */

int pid;

int pty[2];         /* fd's for pty to shell */

			/* Up is to user from appl, down is other way */
char dnbuffer[BUFSIZ+1];	/* for appl input (and during startup) */
char upbuffer[BUFSIZ+1];	/* for appl output; for prompt code */
int   dnbuflen = -1;
int   upbuflen = -1;
int prevbuflen = -1;

char p_buffer[80];

jmp_buf jmpbuf;

struct termio termio;
struct termio oldbuf, rawbuf;

int pflg = 0;		/* provide a prompt */
int rflg = 1;		/* use raw mode */
int iflg = 0;		/* force interactive */
int tflg = 0;		/* force transparent */
#ifdef BAD_PTY
int xflg = 0;		/* super-transparent */
#endif
int ibflg = 0;		/* known to be forced interactive */
int debug = 0;		/* debug printout:
			   print when raw mode goes off/on */
char *prompt = NULL;
char *history_file = NULL;
char *keymap = NULL;
int history_size = -1;
int rawmode = 0;

char *p_ptr;		/* pointer to the prompt when one is needed */

char keytable[256];

char dieing_msg[] = "ied terminating.";

main(argc,argv)
int argc;
char **argv;
{

	int len;
	int count;
	int syncpipe[2];
	int i,t;
	char ptyid[4];    	/* names of pty's */
	struct sigvec vec;

	int c;
	extern char *optarg;
	extern int optind;
	int errflg = 0;

	int file1same = 0;
	int file2same = 0;

#ifdef BAD_PTY
	while ((c = getopt(argc, argv, "dh:ik:p:rs:tx")) != EOF)
#else
	while ((c = getopt(argc, argv, "dh:ik:p:rs:t" )) != EOF)
#endif
	switch (c) {
		case 'd':
			debug++;
			break;
		case 'h':
			history_file = optarg;
			break;
		case 'i':
			iflg++;
			break;
		case 'k':
			keymap = optarg;
			break;
		case 'p':
			pflg++;
			prompt = optarg;
			break;
		case 'r':
			rflg = 0;
			break;
		case 's':
			history_size = atoi(optarg);
			break;
		case 't':
			tflg++;
			break;
#ifdef BAD_PTY
		case 'x':
			xflg++;
			break;
#endif
		case '?':
			errflg++;
	}
	if (optind+1 > argc) errflg++;

	if (errflg) {
#ifdef BAD_PTY
		fprintf(stderr, "usage: ied -irdtx -h <histfile> -s <size> -p <prompt> -k <keymap>\n");
#else
		fprintf(stderr, "usage: ied -irdt -h <histfile> -s <size> -p <prompt> -k <keymap>\n");
#endif
		exit(FAIL);
	}

	if (keymap != NULL && !process_keymap(keymap)) {
		exit(FAIL);
	}

	if (ioctl(0, TCGETA, &termio) != 0) {
		if (errno == ENOTTY) {
			/* not controlled by a tty device */
			if (debug) fprintf(stderr,"Not interactive(%d)... ",
				errno);
			if (iflg) {
				ibflg = 1;
				if (debug) fprintf(stderr, 
					"but hanging around.\n");
				/* dummy a termio struct up */
				termio.c_iflag = 
				  BRKINT|IGNPAR|ISTRIP|IXON|IXOFF;
				termio.c_oflag = 
				  OPOST|ONLCR|TAB0|BS0|VT0|CR0;
				termio.c_cflag = 
				  B19200|CS8|HUPCL|CREAD;
				termio.c_lflag =
				  ISIG|ICANON|ECHO|ECHOE|ECHOK;
				termio.c_line = 0;
				termio.c_cc[VINTR] = 'C'&0x1f;
				termio.c_cc[VQUIT] = '\\'&0x1f;
				termio.c_cc[VERASE] = 'H'&0x1f;
				termio.c_cc[VKILL] = 'U'&0x1f;
				termio.c_cc[VEOF] = 'D'&0x1f;
				termio.c_cc[VEOL] = '@'&0x1f;
			}
			else {
				if (debug) fprintf(stderr,"and leaving.\n");
				/* get out of the way */
				execvp(argv[optind],&argv[optind]);
				perror("ied: non-interact exec failed");
				exit(FAIL);
			}
		}
		else {
			perror("ied: initial TCGETA ioctl failed");
			exit(FAIL);
		}
	}

	/* in case of -x or other reasons, give these initial sane values */
	oldbuf = termio;
	rawbuf = termio;

	if (pipe(syncpipe) < 0) {
		perror("ied: no pipe");
		exit(FAIL);
	}

	if (getpty(pty, ptyid) != 0) {
		fprintf(stderr, "ied: could not get pty\r\n");
		exit(FAIL);
	}

#ifdef BAD_PTY
	/* there's a bug in pty so programs such as connect and shl
	   don't wake up out of a select call in TIOCREMOTE mode.
	   This is a bug that has been reported.  In the meantime,
	   except for handling EOF conditions, TIOCTTY off is equivalent,
	   so use it on request */

	if (!xflg) {
#endif
		i = 1;
		if (ioctl(pty[MASTER],TIOCREMOTE,&i) != 0) {
			perror("ied: TIOCREMOTE ioctl failed");
			exit(FAIL);
		}
#ifdef BAD_PTY
	}
	else {
		i = 0;
		if (ioctl(pty[MASTER],TIOCTTY,&i) != 0) {
			perror("ied: TIOCTTY ioctl failed");
			exit(FAIL);
		}
	}
#endif

	if (ioctl(pty[MASTER], TIOCSIGMODE, TIOCSIGNORMAL) != 0) {
		perror("ied: TIOCSIGMODE ioctl failed");
		exit(FAIL);
	}


	fcntl(syncpipe[READ], F_SETFD, 1);     /* set close on exec flag */
	fcntl(syncpipe[WRITE], F_SETFD, 1);

	pid = fork();
	if (pid < 0) {
	    perror("ied: fork failed");
	    die(dieing_msg);
	}

	{
	struct stat stat0buf, stat1buf;

	fstat(0, stat0buf);
	fstat(1, stat1buf);

	if (stat0buf.st_dev == stat1buf.st_dev &&
	    stat0buf.st_ino == stat1buf.st_ino) file1same++;

	fstat(2, stat1buf);

	if (stat0buf.st_dev == stat1buf.st_dev &&
	    stat0buf.st_ino == stat1buf.st_ino) file2same++;
	}


	if (pid == 0) {
	    close(syncpipe[READ]);
	    close(0);
	    close(pty[MASTER]);
	    setpgrp();
	    strcpy(upbuffer, "/dev/pty/tty");
	    strcat(upbuffer, ptyid);
	    if ( (i = open(upbuffer, O_RDONLY)) < 0 ) {
		    fprintf(stderr,"ied: pty %s failed reopen(0) ",upbuffer);
		    perror("");
		    write(syncpipe[WRITE], "f", 1);
		    exit(FAIL);
	    }

	    /* have to set this from the slave side */
	    if (ioctl(i, TCSETA, &termio) != 0) {
	    	    perror("ied: initial TCSETA ioctl failed");
	       	    exit(FAIL);
	    }
	
/* is this right, or should we dup (note bc echo in this mode) */
	    if (file1same) {
		    close(1);
		    if ( (i = open(upbuffer, O_WRONLY)) < 0 ) {
			    fprintf(stderr,"ied: pty %s failed reopen(1) ",upbuffer);
			    perror("");
			    write(syncpipe[WRITE], "f", 1);
			    exit(FAIL);
		    }
	    }

	    if (file2same) {
		    close(2);
		    if ( (i = open(upbuffer, O_RDWR)) < 0 ) {
			    perror("ied: pty failed reopen(2)\n");
			    write(syncpipe[WRITE], "f", 1);
			    exit(FAIL);
		    }
	    }

	    /* optind--;
	    argv[optind] = "exec"; */
	    execvp(argv[optind],&argv[optind]);
	    perror("ied: exec failed");
	    write(syncpipe[WRITE], "f", 1);
	    exit(FAIL);
	}

	/* parent */
	close(syncpipe[WRITE]);
	len = read(syncpipe[READ], &c, 1);        /* hang until child exec's */
	close(syncpipe[READ]);
	if (len != 0) {
	    fprintf(stderr, "ied: Could not invoke %s\n", argv[optind]);
	    close(pty[MASTER]);
	    kill(pid, SIGKILL);
	    exit(FAIL);
	}

#ifdef BAD_PTY
	/* this has to be done late because otherwise there's deadlock
	   because parent won't handle the open until after the exec
	   succeeds */

	if (!xflg) {
#endif
		i = 1;
		if (ioctl(pty[MASTER] ,TIOCMONITOR, &i) != 0) {
			perror("ied: TIOCMONITOR ioctl failed");
			exit(FAIL);
		}
#ifdef BAD_PTY
	}
	else {
		i = 1;
		if (ioctl(pty[MASTER] ,TIOCTRAP, &i) != 0) {
			perror("ied: TIOCTRAP ioctl failed");
			exit(FAIL);
		}
	}
#endif

	signal(SIGCLD,done);
	vec.sv_handler = catcher;
	vec.sv_mask = sigmask(SIGINT) | sigmask(SIGQUIT);
	vec.sv_flags = 0;
	sigvector(SIGQUIT,&vec,NULL);
	sigvector(SIGINT,&vec,NULL);

	set_edit(rflg);

	upbuffer[0] = '\0';	/* initially null */

	while (1) {
		if (setjmp(jmpbuf)) {
			continue;
		}

		if (prompt != NULL) {
			/* calculate it here, but wait for read ready to
			   actually print it */
			sprintf(p_buffer,prompt,count);
			p_ptr = p_buffer;
			editb.e_prbuff = p_buffer;
		}
		else {
			editb.e_prbuff = upbuffer;
		}
		len = e_read(0,line,LINELEN);
		quitc = 0;
		if (len < 0) {
			perror("ied: read fail");
			kill(pid,SIGTERM);
			kill(pid,SIGKILL);
			break;
		}

#ifdef NOTDEF
		/* kludge to assure that the write will be ready */
		{
		int writefds;
		writefds = 1 << pty[MASTER];    /* the program's pty */
		i = select(32, NULL, &writefds, NULL, NULL);
		}
#endif

		i = write(pty[MASTER],line,len);

		if (i<0) {
			perror("ied: write fail");
			continue;
		}
		count++;
	}
	tty_cooked(0);
	die(dieing_msg);
}

int done()
{

	int i, status;

	tty_cooked(0);

	/* and just to make sure */
	if (!ibflg && ioctl(0, TCSETA, &termio) != 0) {
	    perror("ied: final TCSETA ioctl failed");
	    die(dieing_msg);
	}

	i = waitpid(pid, &status, 0);
	if (i<0) {
		perror("ied: wait");
		die(dieing_msg);
	}
	else if (i != pid) { 
		fprintf(stderr,"What?!: wrong pid: %d\n",i);
		die(dieing_msg);
	}

	if ((status & 0xff) == 0) {
		exit((status>>8)&0xff);
	}
	if ((status & 0xff00) == 0) {
		i = status & 0x7f;
		if (i != SIGINT) {
			fprintf(stderr,"Child died with signal %d\n", i);
		}
		exit(0);
	}
	fprintf(stderr,"Child died of unknown causes: %d\n",status);
	die(dieing_msg);
}

int getpty(fds, ptyid)
int fds[];
char ptyid[];
{
    char c, d;

    for (c = 'p'; c <= 'w'; c++)
	for (d = '0'; d <= '9'; d++) {
	    char mpath[100], spath[100];
	    ptyid[0] = c;
	    ptyid[1] = d;
	    ptyid[2] = '\0';
	    strcpy(mpath, "/dev/ptym/pty");
	    strcat(mpath, ptyid);
	    if ( (fds[MASTER] = open(mpath, O_RDWR)) < 0 )
		continue;
	    /* we don't need the slave, but be must be able to open it;
	       since ied runs as an ordinary process, the pty's could
	       have gotten messed up, and we won't be able to open
	       the slave */
	    strcpy(spath, "/dev/pty/tty");
	    strcat(spath, ptyid);
	    if ( (fds[SLAVE] = open(spath, O_RDWR)) < 0 ) {
		close(fds[MASTER]);
		continue;
	    }
	    close(fds[SLAVE]);
	    return(0);
	}
    return(-1);
}

void catcher(sig, code, scp)
int sig;
int code;
struct sigcontext *scp;
{
	ioctl(pty[MASTER],TIOCSIGSEND,sig);
	if (sig == SIGQUIT) {
		switch (quitc++) {
		case 0:
			/* presumes that most of the time the kid will die */
			return;
		case 1: /* it didn't, but we're not sure yet */
			fprintf(stderr,"Abort ied with another QUIT.\n");
			return;
		case 2: /* OK... he really meant it */
		default:
			abort();
		}
	}
	if (!rawmode) longjmp(jmpbuf);
}

#define input 0

ee_read(fd,databuf,nbyte)
int fd;
char *databuf;
int nbyte;
{
    int i,j,t;
    int mask, ttymask, prgmask, readfds, writefds, exceptfds;
    struct timeval timeout;
    struct timeval *timeptr;
    static int timeflag = 0; /* 0: we don't know anything yet;
				1: Raw with ISIG off has been set;
				2: a write to pgm occurred; timing out respose
				3: successful response.
			     */

    rawmode = 0;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    /* no initial timeout, but we can turn it on */
    /* 
    timeptr = &timeout;
    */
    timeptr = NULL; 

    if (tflg) {
	/* if (!ibflg) */ ioctl(fd, TCGETA, &oldbuf);
	ioctl(pty[MASTER], TCGETA, &rawbuf);

	if (debug) fprintf(stderr, "rawmode on\n");
        rawmode = 1;
    }

    ttymask = 1 << fd;        /* the user's tty */
    prgmask = 1 << pty[MASTER];    /* the program's pty */
    mask = ttymask | prgmask;

    while (1) {
        readfds = mask;
        exceptfds = prgmask;
        writefds = prgmask & 
		((p_ptr != NULL || (rawmode && dnbuflen >= 0)) ?0xFFFFFFFF:0);

        i = select(32, &readfds, &writefds, &exceptfds, timeptr);

        if (i < 0) {
            if (errno != EINTR) {
                perror("ied: select failed");
                die(dieing_msg);
            }
            continue;
        }

	if (i == 0) {
#ifdef BAD_PTY
	    die("ied: process timed out; try the -x option\nied terminating.\n");
#else
	    die("ied: process timed out\nFE terminating.\n");
#endif
	}

	/* application changed the tty modes; figure out what to do */
        if (exceptfds & prgmask)  {
            /* Handshake ioctls from pty */

            struct request_info reqbuf;
	    long oldmask;

	    /* block signals so we don't get messed up */
	    oldmask = sigblock(sigmask(SIGINT) | sigmask(SIGQUIT));

            if (ioctl(pty[MASTER], TIOCREQCHECK, &reqbuf) != 0) {
                if (errno == EINVAL) {
	    		sigsetmask(oldmask);
			continue;
		}
                perror("ied: TIOCREQCHECK ioctl failed");
                die(dieing_msg);
	    }

            switch (reqbuf.request) {
            case TCSETA:
            case TCSETAW:
            case TCSETAF:
                if (ioctl(pty[MASTER], reqbuf.argget, &rawbuf) != 0) {
                    perror("ied: argget ioctl failed");
                    die(dieing_msg);
                }

                /* coded in heuristic order of "most likely" */
                if (termio.c_lflag != rawbuf.c_lflag ||
                    termio.c_cc[VEOF] != rawbuf.c_cc[VEOF] ||
                    termio.c_iflag != rawbuf.c_iflag ||
                    termio.c_lflag != rawbuf.c_lflag ||
                    termio.c_cc[VEOL] != rawbuf.c_cc[VEOL] ||
                    termio.c_cc[VERASE] != rawbuf.c_cc[VERASE] ||
                    termio.c_cc[VKILL] != rawbuf.c_cc[VKILL] ||
                    termio.c_cc[VINTR] != rawbuf.c_cc[VINTR] ||
                    termio.c_cc[VQUIT] != rawbuf.c_cc[VQUIT] ||
                    termio.c_line != rawbuf.c_line ||
                    tflg) {
            	    /* it changed; it must mean raw */
                    if (rawmode == 0) {
                        /* it wasn't raw before */

                	/* save previous state of input for restore */
                	ioctl(fd, TCGETA, &oldbuf);

                        if (debug) fprintf(stderr, "rawmode on\n");

                        /* copy forward to tty */
                        ioctl(fd, TCSETAW, &rawbuf);
                    }
                    else {
                        /* it was raw before */

                        /* copy forward to tty */
                        ioctl(fd, TCSETAW, &rawbuf);
                    }
                    rawmode = 1;
		    if (
#ifdef BAD_PTY
		      !xflg && 
#endif
		      (rawbuf.c_lflag & ISIG) == 0 && timeflag==0) {
			timeflag = 1;  /* if idle too long,
		        it must be sick (can't happen in x mode) */
		    }
                }
                else {
                    /* set it to non-raw */
                    if (rawmode == 0) {
                        /* it already was non-raw */
			/* unless/until conditions filtered above, no-op */
		    }
		    else {
			/* it's just becoming non-raw */
                
			/* restore line-edit tty modes */
                        ioctl(fd, TCSETAW, &oldbuf);
                        ioctl(pty[MASTER], TCSETA, &rawbuf);
                        if (debug) fprintf(stderr, "rawmode off\n");
	        	if (prompt == NULL && prevbuflen>0) {
		        	upbuffer[prevbuflen] = '\0';
		    		editb.e_prbuff = upbuffer;
		    		if (prevbuflen > PRSIZE) {
					editb.e_prbuff = upbuffer+prevbuflen;
					editb.e_prbuff -= PRSIZE;
				}
		    		ed_setup(0);
			}
                    }
                    rawmode = 0;
                }
		if (ioctl(pty[MASTER], TIOCREQSET, &reqbuf) != 0) {
                    perror("ied: TIOCREQSET ioctl failed");
                    die(dieing_msg);
		}
                break;
	    case TCXONC:
                if (ioctl(pty[MASTER], reqbuf.argget, &i) != 0) {
                    perror("ied: XON argget ioctl failed");
                    die(dieing_msg);
                }

		ioctl(fd, TCXONC, i);

		goto handshake;	 

            case TCGETA:
#ifdef BAD_PTY
                if (xflg) {
                    if (ioctl(pty[MASTER], reqbuf.argset, &rawbuf) != 0) {
                        perror("ied: argset ioctl failed");
                        die(dieing_msg);
                    }
                }
#endif
                /* drop thru */

            default:
	    handshake:
                /* Finish handshake */
                if (ioctl(pty[MASTER], TIOCREQSET, &reqbuf) != 0) {
                    if (errno == EINVAL) continue;
                    perror("ied: TIOCREQSET ioctl failed");
                    die(dieing_msg);
                }
                break;
            }
	    sigsetmask(oldmask);
	    continue;
        }

	/* pty (application) did a write; copy its output to user */
        if (readfds & prgmask) { 
	    /* copy pty to tty */
            i = read (pty[MASTER], upbuffer, BUFSIZ);
	    if (timeflag == 2) {
	        timeptr = NULL; /* the child is alive */
	        timeflag = 3;
	    }
            if (i < 0) {
                perror("ied: read output");
                die(dieing_msg);
            }
            write(1, upbuffer, i);
	    prevbuflen = i;
	    if (prompt == NULL && i>0) {
		    upbuffer[i] = '\0';
		    editb.e_prbuff = upbuffer;
		    if (i > PRSIZE) {
			editb.e_prbuff = upbuffer+i;
			editb.e_prbuff -= PRSIZE;
		    }
		    ed_setup(0);
		}
	    continue;
        }

	/* pty (application) can accept input (at least to kernel buffer) */
        if (writefds & prgmask) { 
	    if (rawmode) {
                if (keymap != NULL) {
                    for (j=0; j<dnbuflen; j++)
                        dnbuffer[j] = keytable[dnbuffer[j]];
                }
		write(pty[MASTER],dnbuffer,dnbuflen);
		if (timeflag == 1) {
	            timeptr = &timeout;
	            timeflag = 2;
		} 
		dnbuflen = -1;
	    }
	    else {
	        /* I could write on pty; print prompt */
                if (p_ptr != NULL) {
                    /* write a prompt */
                    write(1,p_ptr,strlen(p_ptr));
                    p_ptr = NULL;
                }
	        continue;
	    }
        }

	/* user has typed to ied; data is ready */
        /* do this last in case it returns to edit logic */
        if (readfds & ttymask) {
	    /* tty read would succeed */
            if (rawmode) {
		if (dnbuflen < 0) dnbuflen = 0;
            retry:   
		if (dnbuflen < BUFSIZ) {
			i = read (fd, &dnbuffer[dnbuflen], BUFSIZ-dnbuflen);
		}
		else {
			i = 0;
		}
                if (i < 0) {
                    if (errno == EINTR) goto retry;
                    perror("ied: raw read input");
                    die(dieing_msg);
                }
		dnbuflen += i;
                quitc = 0;
            }
            else {
                if ( (i = read(fd, databuf, nbyte)) < 0) {
                    if (errno == EINTR) longjmp(jmpbuf);
                    return(i);
                }
                if (keymap != NULL) {
                    for (j=0; j<i; j++)
                        databuf[j] =
                          keytable[databuf[j]];
                }
                return i;
            }
        }
    }
}


process_keymap(name)
char *name;
{
	int i,j;
	int eflag = 0;
	char chname[8];
	FILE *fd;

	fd = fopen(name,"r");
	if (fd == NULL) {
		fprintf(stderr, "Could not open keymap file\n");
		return (0);
	}

	for (i=0; i<256; i++) {
		keytable[i] = i;
	}
	for (i=0; i<256; i++) {
		j = fscanf(fd,"%5s%*[^\n]\n",chname);
		if (j == EOF) break;
		if (j != 1) {
			perror("ied: bad keymap read");
			eflag++;
		}
		switch (strlen(chname)) {
		case 0:
			fprintf(stderr, 
				"ied: null keymap entry at line %d\n",i);
			eflag++;
			break;
		case 1:
			keytable[i] = chname[0];
			break;
		case 2:
			switch (chname[0]) {
			case '^':
				if (chname[1] == '?') {
					keytable[i] = 0x7f;
				}
				else {
					keytable[i] = chname[1] & 0x1F;
				}
				break;
			case '\\':
				switch(chname[1]) {
				case 's':
					keytable[i] = ' '; 
					/* need visible space */
					break;
				case 'r':
					keytable[i] = '\r';
					break;
				case 'n':
					keytable[i] = '\n';
					break;
				case 'f':
					keytable[i] = '\f';
					break;
				case 't':
					keytable[i] = '\t';
					break;
				case 'v':
					keytable[i] = '\v';
					break;
				case 'b':
					keytable[i] = '\b';
					break;
				case '\\':
					keytable[i] = '\\';
					break;
				case '0':
					keytable[i] = '\0';
					break;
				default:
					fprintf(stderr,
						"ied: bad \\x keymap at line %d\n",i);
					eflag++;
				}
			}
			break;
		case 3:
			if (chname[0] != '\\') {
				fprintf(stderr, 
					"ied: bad \\ at line %d\n",i);
				eflag++;
			}
			keytable[i]=(chname[1]-'0')*8+(chname[2]-'0');
			break;
		case 4:
			if (chname[0] != '\\') {
				fprintf(stderr, 
					"ied: bad \\ at line %d\n",i);
				eflag++;
			}
			keytable[i]=
			    ((chname[1]-'0')*8+(chname[2]-'0'))*8
			    +(chname[3]-'0');

			break;
		default:
			fprintf(stderr, 
				"ied: keymap entry too long at line %d\n",i);
			eflag++;
		}
	}
	if (debug) {
		fwrite(keytable,1,256,stderr);
	}
	fclose(fd);
	return(eflag==0);
}

die(mesg)
char *mesg;
{

	tty_cooked(0);

	/* and just to make sure */
	ioctl(0, TCSETA, &termio);

	fprintf(stderr,mesg);

	exit(FAIL);
}

