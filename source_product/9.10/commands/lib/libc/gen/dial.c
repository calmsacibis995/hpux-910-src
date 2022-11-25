#define ddt
/* @(#) $Revision: 70.3 $ */    
/*LINTLIBRARY*/
/***************************************************************
 *      dial() returns an fd for an open tty-line connected to the
 *      specified remote.  The caller should trap all ways to
 *      terminate, and call undial(). This will release the `lock'
 *      file and return the outgoing line to the system.  This routine
 *      would prefer that the calling routine not use the `alarm()'
 *      system call, nor issue a `signal(SIGALRM, xxx)' call.
 *      If you must, then please save and restore the alarm times.
 *      The sleep() library routine is ok, though.
 *
 *	#include <sys/types.h>
 *	#include <sys/stat.h>
 *      #include "dial.h"
 *
 *      int dial(call);
 *      CALL call;
 *
 *      void undial(rlfd);
 *      int rlfd;
 *
 *      rlfd is the "remote-lne file descriptor" returned from dial.
 *
 *      The CALL structure as (defined in dial.h):
 *
 *      typedef struct {
 *              struct termio *attr;    ptr to term attribute structure
 *              int     baud;           transmission baud-rate
 *              int     speed;          212A modem: low=300, high=1200
 *              char    *line;          device name for out-going line
 *              char    *telno;         ptr to tel-no digit string
 *		char 	*device		Will hold the name of the device
 *					used to makes a connection.
 *		int	dev_len		This is the length of the device
 *					used to makes a connection.
 *      } CALL;
 *
 *      The error returns from dial are negative, in the range -1
 *      to -12, and their meanings are:
 *
 *              INTRPT   -1: interrupt occured
 *              D_HUNG   -2: dialer hung (no return from write)
 *              NO_ANS   -3: no answer within 20 seconds
 *              ILL_BD   -4: illegal baud-rate
 *              A_PROB   -5: acu problem (open() failure)
 *              L_PROB   -6: line problem (open() failure)
 *              NO_Ldv   -7: can't open L-devs file
 *              DV_NT_A  -8: specified device not available
 *              DV_NT_K  -9: specified device not known
 *              D_NO_BA -10: no device available at requested baud-rate
 *              D_NO_BK -11: no device known at requested baud-rate
 *		DV_NT_E -12: requested speed does not match
 *
 *      Setting attributes in the termio structure indicated in
 *      the `attr' field of the CALL structure before passing the
 *      structure to dial(), will cause those attributes to be set
 *      before the connection is made.  This can be important for
 *      some attributes such as parity and baud.
 *
 *      As a device-lockout semaphore mechanism, we create an entry,
 *      in the directory #defined as LOCK, whose name is LCK..dev
 *      where dev is the device name taken from the "line" column
 *      in the file #defined as LDEVS.  Be sure to trap every possible
 *      way out of execution in order to "release" the device.
 *      This entry is `touched' every hour in order to keep uucp
 *      from removing it on its 90 minute rounds.
 *      Also, have the system start-up procedure clean all such
 *      entries from the LOCK directory.
 *
 *      With an error return (negative value), there will not be
 *      any `lock-file' entry, so no need to call undial().
 ***************************************************************/


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define access _access
#define alarm _alarm
#define chmod _chmod
#define close _close
#define creat _creat
#define dial _dial
#define dup _dup
#define execl _execl
#define execvp _execvp
#define exit ___exit
#define fclose _fclose
#define fcntl _fcntl
#define fflush _fflush
#define fgets _fgets
#define fopen _fopen
#define fork _fork
#define fprintf _fprintf
#define geteuid _geteuid
#define getgid _getgid
#define getpid _getpid
#define getuid _getuid
#define ioctl _ioctl
#define isatty _isatty
#define kill _kill
#define link _link
#define longjmp __longjmp
#define malloc _malloc
#define open _open
#define perror _perror
#define read _read
#define setjmp  __setjmp
#define setuid  _setuid
#define signal _signal
#define sigvector __sigvector
#define sleep _sleep
#define socket _socket
#define sprintf _sprintf
#define sscanf _sscanf
#define strcat _strcat
#define strchr _strchr
#define strcmp _strcmp
#define strcpy _strcpy
#define strncpy _strncpy
#define strlen _strlen
#define strncmp _strncmp
#define strrchr _strrchr
#define strtok _strtok
#define times _times
#define ttyname _ttyname
#define undial _undial
#define unlink _unlink
#define utime _utime
#define wait _wait
#define write _write
#define atoi _atoi
#  ifdef __lint
#  define isalpha _isalpha
#  endif /* __lint */
#endif  /* _NAMESPACE_CLEAN */

#include <ndir.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <termio.h>
#include <errno.h>
#include <time.h>
#include <model.h>
#include <sys/param.h>
#include <sys/times.h>
#ifdef HPIRS
#include "dial.h"     /* Tempoary place */
#else
#include <dial.h>
#endif
#include <sys/modem.h>

#define ATTSV
#define DEV	"/dev/"
#define TTYD    "ttyd"
#define LOCKDIR "/usr/spool/uucp" /* uucp lock file directory */
#define	DEVSIZE	MAXNAMLEN + sizeof(DEVDIR) + 1	/* max size of device */
#define	LCKSIZE	MAXNAMLEN + sizeof(LOCK) + 2	/* max size of lock file */
#define UPDTE   3600            /* how often to touch the lock entry */
#define ACULAST "<"             /* character which terminates dialing*/
#define YES     1               /* mnemonic */
#define NO      0               /* mnemonic */
#define DIFFER  strcmp          /* mnemonic */
#define say     (void)fprintf   /* mnemonic */
#define HP
#define AUTODIALER	"/usr/lib/dialit"
#define FAIL -1
#define CDEBUG(l, f, s) if (_Debug >= 1) fprintf(stderr, f, s)
#define DEBUG(l, f, s) if (_Debug >= 1) fprintf(stderr, f, s)
#define NBUFSIZ	1024 		/* modified since BUFSIZ is now 8092	*/

#ifdef HDBuucp /*HDB*/ 
			/* fields in Devices */
#define TYPE 0          /* ACU*, systemname, network or Direct */
#define LINE 1          /* cua for non 801s, cul for 801s */
#define CALLDEV 2       /* cua for 801s ,'-' for non 801s*/
#define CLASS 3
#define CALLER 4
#define ARG 5
#define D_MAX 50
#define EQUALS(a,b)	((a) && (b) && (strcmp((a),(b)) == 0))
#define SAME 0
#define PREFIX(pre,str) (strncmp((pre),(str),strlen(pre)) == SAME)
#define GRPCHK(gid)     ( gid >= 2 && gid <= 10 ? 1 : 0 )
#define index strchr
#define NULLCHAR '\0'
#define F_PHONE 4
#define F_CLASS 3
#define SUCCESS 0
#define D_ACU  1 
#define D_DIRECT 2 
#define MAXEXPECTTIME 180
#define EOTMSG "\004\n\004\n"
struct caller {
		char *CA_type;
		int  (*CA_caller)();
};
#endif

extern unsigned
        sleep(),
        alarm();

extern int errno;

extern char *malloc();

	/* This structure tells about a device */
struct Devices {
	char type[20];
	char line[10];
	char calldev[10];
	char class[10];
	int speed;
	};



static char
	modemtype[50],
        cul[DEVSIZE] = DEV,	/* line's device-name */
        cua[DEVSIZE] = DEV,	/* acu's device-name */
        *find_dev(),            /* local function */
	lockdir[LCKSIZE],	/* lockfile */
	lockdir2[LCKSIZE],	/* second lockfile ttyd */
	tempfile[LCKSIZE];  	/* temporary lock file */

static int
	sperfg=0,		/* requested speed not available */
        found=0,                /* set when device is seen legal */
        saverr,                 /* hide errno during other calls */
        rlfd,                   /* fd for remote comm line */
        lfd= -1,                /* fd for the device-lock file */
        lfd2= -1,               /* fd for the ttyd device-lock file */
        intflag=NO,             /* interrupt indicator */
        connect(),              /* local function */
        intcatch(),             /* interrupt routine */
        alrmcatch(),            /* interrupt routine */
        hupcatch();             /* interrupt routine */

#ifdef ddt
static void dump();
#endif
#ifdef hp9000s500
#define	OLBELL_SIG -1
#endif hp9000s500

int     _Debug;
int     _debug;
int 	_dialit;
void    undial();
static	unsigned long _modemstat;
#ifdef HDBuucp /*HDB*/
static char *Bnptr;
char _line_requested[NBUFSIZ];  /* line reuquested by user of cu */
char _protocol[NBUFSIZ];        /* protocol */
static char devbuf[NBUFSIZ];
static char Dc[15];
static char *flds[D_MAX];
static char *dev[D_MAX];
static int Evenflag = 0;
static int Oddflag = 0;
static int Duplex = 0;
static int Terminal = 0;
static struct termio *TermChar;          /* termio characteristics as passed in call->
                                    attr */
static int noterm=0;             /* call->attr is null when zero */
static int nomodem=0;             /* call->modem */
static int Uerror;
static int getargs2();
static void bsfix();
static char *fdig();
#endif

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef dial
#pragma _HP_SECONDARY_DEF _dial dial
#define dial _dial
#endif

int
dial(call)
CALL call;
{
        FILE *Ldevices;         /* file pointer for Device name file */
        char dvc[DEVSIZE];
#ifdef  __hp9000s800
#define DVCELEN 4       /* S800 device name is ttydxpy */
#else
#define DVCELEN 3       /* S300, 400, 700 device file is ttydxx */
#endif  __hp9000s800
	char	dvcext[DVCELEN];
	char	dvcttyd[DEVSIZE];
        int lock_file_fd,pid;
#ifndef	hpux
        int (*savint)(), (*savhup)();
#else	hpux
	struct sigvec ivec, hvec, avec;
	struct sigvec savint, savhup;
#endif	hpux

#ifdef ddt
        if(_debug == YES) {
                say(stderr, "call dial(%d)\r\n", call);
                dump(&call, 0);
        }
#endif

        saverr = 0;
#ifndef	hpux
        savint = signal(SIGINT, intcatch);
        savhup = signal(SIGHUP, hupcatch);
        (void)signal(SIGALRM, alrmcatch);
#else	hpux
	ivec.sv_handler = intcatch;
	ivec.sv_mask = ivec.sv_onstack = 0;
	hvec.sv_handler = hupcatch;
	hvec.sv_mask = hvec.sv_onstack = 0;
	avec.sv_handler = alrmcatch;
	avec.sv_mask = avec.sv_onstack = 0;
	(void) sigvector(SIGINT, &ivec, &savint);
	(void) sigvector(SIGHUP, &hvec, &savhup);
	(void) sigvector(SIGALRM, &avec, (struct sigvec *)0);
#endif	hpux

#ifdef HDBuucp /*HDB*/
        nomodem = call.modem;  /* set these here so they are global */
        if (call.telno == NULL) nomodem |= 1;
        TermChar = call.attr;
        if (!call.attr) noterm = 1;    
        if(call.device == NULL && call.telno == NULL && call.line == NULL) {
#else
        if(call.telno == NULL && call.line == NULL) {
#endif
                	rlfd = DV_NT_K;
                	goto OUT;
        }

#ifdef HDBuucp /*HDB*/
        if((Ldevices = fopen(DEVICES, "r")) == NULL) {
#else
        if((Ldevices = fopen(LDEVS, "r")) == NULL) {
#endif
                saverr = errno;
                rlfd = NO_Ldv;
                goto OUT;
        }

#ifndef HDBuucp
        /* HDB doesn't care whether dialit is 1 or 0*/
	/* if device is ACU... then call HP-UX DIALIT mechanism */
	if ( prefix("ACU", call.device)) {
		if (strlen("ACU") != strlen(call.device)) 
			_dialit=1;
		else
			_dialit=0;
	}

	if (call.telno == NULL)
		_dialit=0;
	else
		_dialit=1;       /* if phone # given, always DIALIT */
#endif
        while(1) {
                int xx;
        	xx = getpid();

#ifdef HDBuucp /*HDB*/
                (void)strcpy(dvc, find_dev(Ldevices, &call,dev,devbuf));
#else
                (void)strcpy(dvc, find_dev(Ldevices, &call));
#endif
                if(strlen(dvc) == 0) 
                        goto F1;        /* failure to find device */
		(void)strcpy(lockdir, LOCK);
		(void)strcat(lockdir, dvc);
		(void) sprintf(tempfile, "%s/LTMP.%d", LOCKDIR, getpid());

		/*  Lock file mechanism changed to look like uucp's, to
		    avoid a possible race condition when multiple cu's
		    are running.  Code adapted from ulockf.c from the
		    uucp source.  ec.  4/22/92.  */

		if (onelock(xx, tempfile, lockdir) == -1) {
		   /* lock file exists.  Check if it is still valid */
		   (void) unlink (tempfile);
		   if (checkLock (lockdir))
		      goto F0;
		   else {
		      if (onelock (xx, tempfile, lockdir)) {
			 (void) unlink (tempfile);
			 goto F0;
		      }
		   }
		}
		/* Got the lock file now.  Now do the same for incoming 
		   lock file if necessary */

/* try to open device also */
/* if device won't open then look for another */
/* this can happen if the device is in use by an inbound call */
#ifndef HDBuucp
                /* Rightnow there is a modem driver bug on the s800 
                   that causes reads from the device to fail if 
                   one open the device twice. 
                   The ifdef' will be taken out after the bug is fixed.
                */ 
		{
		    int fd = open(cul, O_RDWR | O_NDELAY);
		    if (fd < 0)
			goto F0;
		    close(fd);
	        }	
#endif

		/* If the device found has a name that starts with cua or
		   cul, create a lock file with name prefixed with ttyd and 
		   the same extension.  This is done so, to detect if any
		   incoming call is in place for this line and also to
		   block any incoming call on this line. */
		lockdir2[0] = NULLCHAR;
		if (( dvc[0] == 'c') && (dvc[1] == 'u'))
		{
		   (void)strcpy (dvcttyd, TTYD);
		   (void)strncpy (dvcext, &dvc[3], DVCELEN);
		   (void)strncpy (&dvcext[DVCELEN], NULLCHAR, 1);
		   (void)strcat (dvcttyd, dvcext);

		   (void)strcpy (lockdir2, LOCK);
		   (void)strcat (lockdir2, dvcttyd);

		   if (onelock (xx, tempfile, lockdir2) == -1) {
		      /* Incoming lock file exists.  Check if is still valid */
		      (void) unlink (tempfile);
		      if (checkLock (lockdir2)) {
			 (void) unlink (lockdir);
			 goto F0;
		      }
		      else {
			 if (onelock (xx, tempfile, lockdir2)) {
			    (void) unlink (tempfile);
			    (void) unlink (lockdir);
			    goto F0;
			 }
		      }
		   }
		}
		break; /* we have a device get out of here */

        F0:
                if(!call.line)            /* dial device is busy */
                        continue;       /* try to find another */

        F1:
                if(call.line)
                        if(found)       /* specific device request */
                                rlfd = DV_NT_A;
                        else if(sperfg == 1)  
				rlfd = DV_NT_E;
			else
                                rlfd = DV_NT_K;
		else
                        if(found)        /* we are dialing */
                                rlfd = NO_BD_A;
                        else
                                rlfd = NO_BD_K;
                goto CLOUT;
        } /*while */ 
        if(intflag == YES){
                rlfd = INTRPT;
                goto CLOUT;
        }
        else 
#ifdef HDBuucp /*HDB*/
             {
                /* dummy flds descriptor */
                flds[0] = "dummy";   /* dummy system name */
                flds[1] = "Any";     /* time to call */
                flds[3] = dev[3]; /* speed */
                flds[6] = NULL;
                flds[2] = flds[4] = flds[5] = "";
                flds[2] = dev[0];     /* Type */
                if (call.telno) flds[4] = call.telno;   /* phone number */
                strcpy(call.device,cul);
                call.dev_len = strlen(call.device);
                if ((rlfd = processdev(flds,dev)) < 0 )
#else
                if( (rlfd = ( _dialit ? autocall(&call) : connect(&call))) < 0) 
#endif
                        undial(rlfd);
 		else {
			(void) sigvector(SIGALRM, &avec, (struct sigvec *)0);
                        (void)alarm(UPDTE);
		}
#ifdef HDBuucp
             }
#endif
CLOUT:
        (void)fclose(Ldevices);
OUT:
#ifndef	hpux
        (void)signal(SIGINT, savint);
        (void)signal(SIGHUP, savhup);
#else	hpux
# ifndef hp9000s500
	(void) sigvector(SIGINT, &savint, (struct sigvec *)0);
	(void) sigvector(SIGHUP, &savhup, (struct sigvec *)0);
# else hp9000s500
	if (savint.sv_mask == OLBELL_SIG)
		(void)signal(SIGINT, savint.sv_handler);
	else
		(void) sigvector(SIGINT, &savint, (struct sigvec *)0);
	if (savhup.sv_mask == OLBELL_SIG)
		(void)signal(SIGHUP, savhup.sv_handler);
	else
		(void) sigvector(SIGHUP, &savhup, (struct sigvec *)0);
# endif hp9000s500
#endif	hpux
        errno = saverr;
        return(rlfd);
}
/*
 * makes a lock on behalf of pid.  Code adapted from uucp.
 * input:
 *      pid - process id
 *      tempfile - name of a temporary in the same file system
 *      name - lock file name (full path name)
 * return:
 *      -1 - failed
 *      0  - lock made successfully
 */
static
onelock(pid,tempfile,name)
int pid;
char *tempfile, *name;
{
	char    cb[100];
	lfd = creat(tempfile, 0444);
	if (lfd <0) {
	   if ((errno == EMFILE) || (errno == ENFILE))
	      (void) unlink(tempfile);
	   return (-1);
	}
	/* Write the pid into the temp lock file */
	chmod (tempfile, 0664);
	(void) write (lfd, (char*) &pid, sizeof(int));
	(void) close(lfd);
	/* Now link it to the actual lock file */
	if(link(tempfile,name)<0) {
	   if (unlink(tempfile) < 0) 
	      say(stderr, "Can't unlink lock-file\r\n");
	   return (-1);
	}
	chmod (name, 0664);
	/* Link sucessful.  Remove the temporary lock file */
	if (unlink (tempfile) < 0) 
	   say(stderr, "Can't unlink lock-file\r\n");
	return (0);
}

/*
 * check to see if the lock file exists and is still active
 * - use kill(pid,0) 
 * Code adapted from uucp.  ec.  4/23/92.
 * return:
 *      0       -> success (lock file removed - no longer active
 *      FAIL    -> lock file still active
 */
static
checkLock(file)
register char *file;
{
	register int ret;
	int pid = -1;
	int fd;
	extern int errno;

	fd = open(file, O_RDONLY);
	if (fd == -1) {
	   if (errno == ENOENT)
	      return (0);
	   else
	      return (-1);
	}

	read (fd, (char *)&pid, sizeof (pid));
        (void) close(fd);
	if (kill (pid, 0) != -1 || errno != ESRCH)
	   return (-1);
	else {
	   if (unlink (file) != 0) 
		say(stderr, "Can't unlink lock-file\r\n");
	   else
	      	return (0);
	}
	return (-1);
}

/***************************************************************
 *      connect: establish dial-out or direct connection.
 *      Negative values returned (-1...-7) are error message indices.
 ***************************************************************/
static int
connect(call)
CALL *call;
{
        struct termio *lvp, lv;
        int er=0, dum, fdac, fd=0, t, w, x;
        char *p, sp_code, b[30];

#ifdef ddt
        if(_debug == YES) {
                say(stderr, "call connect(%o)\n", call);
                dump(call, 0);
        }
#endif

        switch(call->baud) {
                case 110:
                        sp_code = (B110 | CSTOPB);
                        break;
                case 134:
                        sp_code = B134;
                        break;
                case 150:
                        sp_code = B150;
                        break;
                case 300:
                        sp_code = B300;
                        break;
                case 600:
                        sp_code = B600;
                        break;
                case 1200:
                        sp_code = B1200;
                        break;
		case 2400:
			sp_code = B2400;
                        break;
		case 3600:
			sp_code = B3600;
                        break;
                case 4800:
                        sp_code = B4800;
                        break;
		case 7200:
			sp_code = B7200;
                        break;
                case 9600:
                        sp_code = B9600;
                        break;
		case 19200:
			sp_code = B19200;
                        break;
                default:
                        er = ILL_BD;
                        goto RTN;
        }
        if((fd = open(cul, O_EXCL | O_RDWR | O_NDELAY)) < 0) {
                perror(cul);
                er = L_PROB;
                goto RTN;
        }
	if(call->device && call->dev_len !=0) {
		strncpy(call->device, cul, call->dev_len);
		if(strlen(cul) >= call->dev_len)
			call->device[call->dev_len -1] = '\0';
	}
        if(!call->attr)
                lvp = &lv;
        else
                lvp = call->attr;
        lvp->c_cflag |= (CREAD | HUPCL);
        if(!(lvp->c_cflag & CSIZE))
                lvp->c_cflag |= CS8;
        if( (call->telno == NULL ) && (call->modem) ) {
		lvp->c_cflag |= CLOCAL;
        } else
                lvp->c_cflag &= ~CLOCAL;

#ifdef	CRTS
	lvp->c_cflag |= CRTS;
#endif	CRTS
	ioctl(fd, MCGETA, &_modemstat);
	_modemstat |= MRTS;
	if ( (_modemstat & MDTR) != MDTR) {
		_modemstat |= MDTR;
#ifdef  ddt
	if(_debug == YES) say(stderr,"MDTR was not set\n\r");
#endif
	}
	ioctl(fd, MCSETAW, &_modemstat);

#ifdef  ddt
	if(_debug == YES) say(stderr,"value of cflag = %o\n\r", lvp->c_cflag);
#endif
        lvp->c_cflag &= ~CBAUD;
        lvp->c_cflag |= sp_code;
        if((t = ioctl(fd, TCSETA, lvp)) < 0) {
                perror("stty for remote");
                er = L_PROB;
                goto RTN;
        }
        if(call->telno) {
		(void)alarm(30);
                if((fdac = open(cua, O_WRONLY)) < 0) {
                        perror(cua);
                        er = A_PROB;
                        goto RTN;
                }
		alrmcatch();
                t = strlen(strcat(strcpy(b, call->telno), ACULAST));
#ifdef ddt
                if(_debug == YES)
                        say(stderr, "dialing %s\n", b);
#endif
                w = write(fdac, b, (unsigned)t); /* dial the number */
		sleep(1);
                x = errno;
		p = &b[t-2];
		for(; *p-- == '-'; t--);
                if(w < t) {
                        errno = x;
                        if(w == -1)
                                perror("write to acu");
                        else
                                say(stderr, "%s: Semaphore failure\n", cul);
                        er = (errno == EINTR)? D_HUNG: A_PROB;
			(void)close(fdac);
                        goto RTN;
                }
                (void)close(fdac);      /* dialing is complete */
#ifdef ddt
                if(_debug == YES)
                        say(stderr, "dialing complete\n");
#endif
        }
        (void)alarm(20);        /* should answer within 20 seconds */
        dum = open(cul, O_RDWR); /* wait here for carrier */
        x = errno;
	alrmcatch();
        if(dum < 0) {
                errno = x;
#ifdef ddt
                if(_debug == YES)
                        perror(cul);
#endif
                er = (errno == EINTR)? NO_ANS: L_PROB;
                goto RTN;
        }
        (void)close(dum);       /* the dummy open used for waiting*/
        (void)fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NDELAY);
RTN:
        if(intflag == YES)
                er = INTRPT;
#ifdef ddt
        if(_debug == YES)
                say(stderr, "connect ends with er=%d, fd=%d\n", er, fd);
#endif
	if(er) {
		close(fd);		
		return(er);
	} else
		return(fd);
}

/***************************************************************
 *      find_dev: find a device pair with the wanted characteristics
 *            specified in line and baud arguments.
 *      Return pointer to device name for use in lock-out semaphore.
 *      The variables 'cua' and 'cul' will be set to contain the
 *              complete path names of the corresponding devices.
 *      If the L-devices list contains a '0' entry because the
 *              line is direct, the variable 'cua' is set to '\0'.
 ***************************************************************/

static char*
#ifdef HDBuucp /*HDB*/
find_dev(iop, call,dev,buf)
#else
find_dev(iop, call)
#endif
FILE *iop;
CALL *call;
#ifdef HDBuucp /*HDB*/
char *dev[];   /* holds all the fields a line of iop */
char buf[];
#endif
{
#ifndef HDBuucp /*HDB*/
        char buf[NBUFSIZ];
#endif
        char typ[24], temp[DEVSIZE], *b;
        int tspeed;
#ifdef HDBuucp  /*HDB*/
        int na; /* number of avaialable arg */
        int count;
#endif

#ifdef ddt
        if(_debug == YES) {
                say(stderr, "call find_dev(%o)\n", call);
                dump(call, 0);
        }
#endif

#ifdef HDBuucp /*HDB*/
        if (call->telno == NULL){  /*direct connections */
            if (call->line == NULL){ /* system name/network name ie cu hpsys1 */
                (void) strcpy(typ,call->device);
                /* get back the requested line if any */
                if (_line_requested[0] != 0 ) call->line = _line_requested;           
            }
            else 
                (void) strcpy(typ, "Direct");
        }else {
                 if (strlen(call->device) > 0 )
                 	(void) strcpy(typ,call->device); 
                 else 
                        (void) strcpy(typ,"ACU");
        }
#else
        if(call->telno == NULL)
                (void)strcpy(typ, "DIR");
        else {
		strcpy(typ, "ACU");
		if (_dialit) 
			if (strlen(call->device) > 3)
				(void)strcat(typ, call->device+3);
	}
#endif

        while(fgets(buf, NBUFSIZ, iop) != NULL) {

                if (strchr("# \t\n", buf[0]) != NULL)
                        continue;
#ifdef HDBuucp /*HDB*/
                na = getargs2(buf,dev,50);
                if (na<5) continue; /* Skip illegal entries */
                bsfix(dev);
                if (DIFFER(typ,dev[TYPE]))
			if (!prefix(typ, dev[TYPE]))
				continue;
                if (!strcmp(dev[CLASS],"Any")){
                     if ((call->speed<0) && (call->baud<0))
                         tspeed = 1200; /*default*/
                     else if (call->speed<0) tspeed = call->baud;
                     else tspeed = call->speed; 
                } else tspeed = atoi(fdig(dev[CLASS]));
		strcpy(modemtype, dev[TYPE]);
                (void)strcat(strcpy(cul,DEVDIR),dev[LINE]);
                (void)strcat(strcpy(cua,DEVDIR),dev[CALLDEV]);
                if (!strcmp(dev[CALLDEV],"-")) strcpy(cua,cul);
                if (call->line){
                                if (DIFFER(call->line,cul) && 
                                    DIFFER(call->line,dev[LINE])) 
                                	continue;
                                if (call->baud < 0){
                                          call->speed=call->baud=tspeed;
                                }else if (call->baud != tspeed){
                                        sperfg = 1; 
                                        continue;
                                }else
                                        sperfg = 0;
                }
#else
		if(DIFFER(typ, strtok(buf, " \t")))
			if (!prefix(typ, buf))
				continue;
		strcpy(modemtype, buf);

                (void)strcat(strcpy(cul,DEVDIR),strtok((char*)0," \t"));

                if(*(b = strtok((char*)0, " \t")) == '0')
                        cua[0] = '\0';
                else {
                        (void)strcat(strcpy(cua, DEVDIR), b);
                         /*if (!strcmp(cua,"/dev/-")) strcpy(cua,cul);*/
                } tspeed = atoi(strtok((char*)0," \t\n"));

                if(call->line) {
                        if(strchr((b=call->line), '/') == 0) {
                                (void)strcpy(temp, DEVDIR);
                                b = strcat(temp, call->line);
                        }
                        if(DIFFER(b, cul))
                                continue;
			if(call->baud < 0) {
				/*found line, no baud rate requested, set */
                        	call->baud = call->speed = tspeed;
			} else if(tspeed != call->baud) {
				/* found line at wrong speed, keep looking */
				sperfg = 1;
				continue;
			} else {
				/* found line at correct speed, clear error */
				sperfg = 0;
			}
                }
#endif
                if(call->telno) {
                        if(call->speed != tspeed)
                                continue;
                }
                if(call->baud > call->speed)
                        continue;
                ++found;
                return(1+strrchr(cul, '/'));
        }
        return("");
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef undial
#pragma _HP_SECONDARY_DEF _undial undial
#define undial _undial
#endif

void
undial(rfd)
int rfd;
{
	int lfpid, mypid;
	int fd;
#ifdef ddt
        if(_debug == YES)
                say(stderr, "call undial(%d)\n", rfd);
#endif
        /* make sure carrier is drop */
        ioctl(rfd,MCGETA,&_modemstat);
        _modemstat &= ~MDTR;
        _modemstat &= ~MRTS;
        ioctl(rfd,MCSETA,&_modemstat);
        if(rfd > 0) 
                (void)close(rfd);
        if(lfd > 0) {
                (void)close(lfd);
                lfd = -1;
		if(unlink(lockdir) < 0)
                        say(stderr, "Can't unlink lock-file\r\n");
		if (lockdir2[0] != NULLCHAR) {
		/*  Remove lock file2 only if it is mine */
		   fd = open (lockdir2, O_RDONLY);
		   if (fd > 0) {
		      read (fd, (char *)&lfpid, sizeof (int));
		      (void) close (fd);
        	      mypid = getpid();
		      if (lfpid == mypid)
			 if (unlink(lockdir2) != 0)
			    say(stderr, "Can't unlink lock-file\r\n");
		   }
		}
#ifdef ddt
                else if(_debug == YES)
                        say(stderr, "Lock-file unlinked\r\n");
#endif
        }
        return;
}

static int
alrmcatch()
{
        (void)alarm(UPDTE);
	(void)utime(lockdir, (struct {long a,b;} *)0);
	(void)utime(lockdir2, (struct {long a,b;} *)0);
#ifndef	hpux
        (void)signal(SIGALRM, alrmcatch);
#endif	hpux
}

static int
hupcatch()
{
        undial(rlfd);
}

static int
intcatch()
{
        intflag = YES;
#ifndef	hpux
        (void)signal(SIGINT, intcatch);
#endif	hpux
}

#ifdef ddt
static void
dump(arg, fd)
CALL *arg;
int fd;
{
        struct termio xv;
        int i;

        if(fd > 0) {
                say(stderr, "\r\ndevice status for fd=%d\r\n", fd);
                say(stderr, "F_GETFL=%o\r\n", fcntl(fd, F_GETFL,1));
                if(ioctl(fd, TCGETA, &xv) < 0) {
                        char buf[100];
                        int x=errno;

                        (void)sprintf(buf, "\rtdmp for fd=%d:", fd);
                        errno = x;
                        perror(buf);
                        return;
                }
                say(stderr, "iflag=`%o',", xv.c_iflag);
                say(stderr, "oflag=`%o',", xv.c_oflag);
                say(stderr, "cflag=`%o',", xv.c_cflag);
                say(stderr, "lflag=`%o',", xv.c_lflag);
                say(stderr, "line=`%o'\r\n", xv.c_line);
                say(stderr, "cc[0]=`%o',", xv.c_cc[0]);
                for(i=1; i<8; ++i)
                        say(stderr, "[%d]=`%o',", i, xv.c_cc[i]);
                say(stderr, "\r\n");
        }
        say(stderr,"baud=%d, ",arg->baud);
        say(stderr,"speed=%d, ",arg->speed);
        say(stderr,"line=%s, ",arg->line? arg->line: "(null)");
        say(stderr,"telno=%s\r\n",arg->telno? arg->telno: "(null)");
}
#endif



/*******
 *	prefix(s1, s2)	check s2 for prefix s1
 *	char *s1, *s2;
 *
 *	return 0 - !=
 *	return 1 - == 
 */

static 
prefix(s1, s2)
char *s1, *s2;
{
	char c;

	while ((c = *s1++) == *s2++)
		if (c == '\0')
			return(1);
	return(c == '\0');
}



#define MAXPH 60
#define NOCALLDEV   0   /* constant to indicate no call device specified*/
#define CALLCOMPLETE 0  /* return value indicating autodial complete */
#define CALLFAILED  -1  /* indicating autodial failed */

static char allow_protos[10];	/* string of allowable protocols from phone field */
			/* in L.sys, ie, g/9=555-1212 */
static jmp_buf Sjbuf;
static int alarmtr();
static  catch_int();

static
autocall(call)
CALL *call;
{
    char phone[MAXPH+1];          /* array holds the expanded phone number */
    char sp_code;

    int	 status,                  /* status indicator for child's exit code */
         fd_tmp,                  /* temporary line descriptor */
	 fd,                      /* line descriptor */
	 fd_calldev,              /* call device descriptor */
	 pid,                     /* process id for child spawned */
	 ret,                     /* return idicator for a wait call */
	 retval;                  /* return value for this process */

#ifndef	hpux
    int (*savint)(), (*savhup)();
#else	hpux
	struct sigvec cvec, savint, savhup;
#endif	hpux

    struct termio *lvp, lv;
    int  delay = 120;             /* delay time to do autodial and connect */
    int er = 0;

	pid = 0;
#ifndef	hpux
	savint = signal(SIGINT, catch_int);
	savhup = signal(SIGHUP, catch_int);
#else	hpux
	cvec.sv_handler = catch_int;
	cvec.sv_mask = cvec.sv_onstack = 0;
	(void) sigvector(SIGINT, &cvec, &savint);
	(void) sigvector(SIGHUP, &cvec, &savhup);
#endif	hpux

        switch(call->baud) {
                case 110:
                        sp_code = (B110 | CSTOPB);
                        break;
                case 134:
                        sp_code = B134;
                        break;
                case 150:
                        sp_code = B150;
                        break;
                case 300:
                        sp_code = B300;
                        break;
                case 600:
                        sp_code = B600;
                        break;
                case 1200:
                        sp_code = B1200;
                        break;
		case 2400:
			sp_code = B2400;
                        break;
		case 3600:
			sp_code = B3600;
                        break;
                case 4800:
                        sp_code = B4800;
                        break;
		case 7200:
			sp_code = B7200;
                        break;
                case 9600:
                        sp_code = B9600;
                        break;
		case 19200:
			sp_code = B19200;
                        break;
                default:
                        er = ILL_BD;
#ifndef	hpux
			signal(SIGINT, savint);
			signal(SIGHUP, savhup);
#else	hpux
# ifndef hp9000s500
			(void) sigvector(SIGINT, &savint, (struct sigvec *)0);
			(void) sigvector(SIGHUP, &savhup, (struct sigvec *)0);
# else hp9000s500
			if (savint.sv_mask == OLBELL_SIG)
				(void)signal(SIGINT, savint.sv_handler);
			else
				(void) sigvector(SIGINT, &savint, (struct sigvec *)0);
			if (savhup.sv_mask == OLBELL_SIG)
				(void)signal(SIGHUP, savhup.sv_handler);
			else
				(void) sigvector(SIGHUP, &savhup, (struct sigvec *)0);
# endif hp9000s500
#endif	hpux
			return(er);
        }

	if (!call->attr)
		lvp = &lv;
	else
		lvp = call->attr;

	strcpy(phone, call->telno);
	fd_tmp = open(cul, O_NDELAY | O_RDWR);

        if ((fd_calldev = open(cul ,O_NDELAY | O_RDWR)) == FAIL) {
		DEBUG(4,"CALL DEVICE DID NOT OPEN%s\n","");
#ifndef	hpux
		signal(SIGINT, savint);
		signal(SIGHUP, savhup);
#else	hpux
# ifndef hp9000s500
		(void) sigvector(SIGINT, &savint, (struct sigvec *)0);
		(void) sigvector(SIGHUP, &savhup, (struct sigvec *)0);
# else hp9000s500
		if (savint.sv_mask == OLBELL_SIG)
			(void)signal(SIGINT, savint.sv_handler);
		else
			(void) sigvector(SIGINT, &savint, (struct sigvec *)0);
		if (savhup.sv_mask == OLBELL_SIG)
			(void)signal(SIGHUP, savhup.sv_handler);
		else
			(void) sigvector(SIGHUP, &savhup, (struct sigvec *)0);
# endif hp9000s500
#endif	hpux
		return(L_PROB);
        }

/*      strcpy(modemtype, call->device);        */

	if(call->device && call->dev_len !=0) {
		strncpy(call->device, cul, call->dev_len);
		if(strlen(cul) >= call->dev_len)
			call->device[call->dev_len -1] = '\0';
	}

	if (setjmp(Sjbuf)){          /* setup for autodial timeout period */
#ifndef	hpux
		signal(SIGINT, savint);
		signal(SIGHUP, savhup);
#else	hpux
# ifndef hp9000s500
		(void) sigvector(SIGINT, &savint, (struct sigvec *)0);
		(void) sigvector(SIGHUP, &savhup, (struct sigvec *)0);
# else hp9000s500
		if (savint.sv_mask == OLBELL_SIG)
			(void)signal(SIGINT, savint.sv_handler);
		else
			(void) sigvector(SIGINT, &savint, (struct sigvec *)0);
		if (savhup.sv_mask == OLBELL_SIG)
			(void)signal(SIGHUP, savhup.sv_handler);
		else
			(void) sigvector(SIGHUP, &savhup, (struct sigvec *)0);
# endif hp9000s500
#endif	hpux
		if (intflag == YES)
			DEBUG(4,"Autodialer interrupt%s\n","");
		else
			DEBUG(4,"Autodialer timeout%s\n","");
		sleep(1);
		if (pid)
			kill(pid, SIGKILL);     /* kill DIALIT process */
		close(fd_tmp);          /* close call device in case of timeout */
		close(fd_calldev);
		sleep(1);
		if (intflag == YES)
			return(INTRPT);
		else
			return(D_HUNG);
	}

/*******************************************************************
*  configure the lines to be used for autodialing
*******************************************************************/

        lvp->c_cflag |= (CREAD | HUPCL);
        if(!(lvp->c_cflag & CSIZE))
                lvp->c_cflag |= CS8;
	lvp->c_cc[VMIN] = 1; /* WAS 6 */
	lvp->c_cc[VTIME] = 1;
        lvp->c_cflag &= ~CBAUD;
        lvp->c_cflag |= sp_code;
	lvp->c_cflag |= CLOCAL;     /* make it direct; change back later */
#ifdef	CRTS
	lvp->c_cflag |= CRTS;
#endif	CRTS
	ioctl(fd_tmp, MCGETA, &_modemstat);
	_modemstat |= MRTS;
	if ( (_modemstat & MDTR) != MDTR) {
		_modemstat |= MDTR;
#ifdef  ddt
	if(_debug == YES) say(stderr,"MDTR was not set\n\r");
#endif
	}
	ioctl(fd_tmp, MCSETAW, &_modemstat);

	if( ioctl(fd_tmp, TCSETA, lvp) < 0) {
                perror("stty for remote");
                er = L_PROB;
#ifndef	hpux
		signal(SIGINT, savint);
		signal(SIGHUP, savhup);
#else	hpux
# ifndef hp9000s500
		(void) sigvector(SIGINT, &savint, (struct sigvec *)0);
		(void) sigvector(SIGHUP, &savhup, (struct sigvec *)0);
# else hp9000s500
		if (savint.sv_mask == OLBELL_SIG)
			(void)signal(SIGINT, savint.sv_handler);
		else
			(void) sigvector(SIGINT, &savint, (struct sigvec *)0);
		if (savhup.sv_mask == OLBELL_SIG)
			(void)signal(SIGHUP, savhup.sv_handler);
		else
			(void) sigvector(SIGHUP, &savhup, (struct sigvec *)0);
# endif hp9000s500
#endif	hpux
		return(er);
        }
	DEBUG(4,"Setting up call device line%s\n","");

	/*****************************************************************
	 * set up alarm timeout and fork. Child will invoke to user written
	 * dialer routine.
	 *****************************************************************/

	 status = 0;                          /* clear status for wait */
#ifndef	hpux
	 signal(SIGALRM,alarmtr);
#else	hpux
	{
	 struct sigvec alarmvec;
	 alarmvec.sv_handler = alarmtr;
	 alarmvec.sv_mask = alarmvec.sv_onstack = 0;
	 (void) sigvector(SIGALRM, &alarmvec, (struct sigvec *)0);
	}
#endif	hpux
	 alarm(delay);
	 if ((pid = fork()) == 0) {
		 char cua[DEVSIZE], spd[6];    /* call dev path name for exec */

		 close(0);
		 close(1);
		 close(2);
		 signal(SIGINT,SIG_IGN);
		 signal(SIGHUP,SIG_IGN);
		 signal(SIGQUIT,SIG_IGN);
		 strcpy(cua, cul);
		 sprintf(spd, "%d", call->baud);
		 (void) setuid(getuid());
		 execl(AUTODIALER,"dialit",modemtype,cua,phone,
		       spd ,allow_protos,0);
		 /*logent("MISSING","DIALIT");*/
		 exit(-1);
	}

	/*******************************************************************
	 * wait for autodial routine to finish. Pick up exit code in status
	 * and determine whether success or failure on call.
	 ******************************************************************/
       
	 while ((ret = wait(&status)) != pid && ret != -1);
	 alarm(0);
	 DEBUG(4,"autodial return status > %o\n",status);
	 DEBUG(4,"ret > %o :",ret);
	 DEBUG(4,"pid > %o\n",pid);
	 if (setjmp(Sjbuf)){
		/*logent("NO CARRIER DETECTED","FAIL");*/
		close(fd_tmp);
#ifndef	hpux
		signal(SIGINT, savint);
		signal(SIGHUP, savhup);
#else	hpux
# ifndef hp9000s500
		(void) sigvector(SIGINT, &savint, (struct sigvec *)0);
		(void) sigvector(SIGHUP, &savhup, (struct sigvec *)0);
# else hp9000s500
		if (savint.sv_mask == OLBELL_SIG)
			(void)signal(SIGINT, savint.sv_handler);
		else
			(void) sigvector(SIGINT, &savint, (struct sigvec *)0);
		if (savhup.sv_mask == OLBELL_SIG)
			(void)signal(SIGHUP, savhup.sv_handler);
		else
			(void) sigvector(SIGHUP, &savhup, (struct sigvec *)0);
# endif hp9000s500
#endif	hpux
		 return(NO_ANS);
	 }
	 switch (status) {
	  case CALLCOMPLETE:
		  fflush(stdout);
		  if (!call->modem) {
			lvp->c_cflag &= ~CLOCAL;
			ioctl(fd_tmp, TCSETA, lvp);
		  }
		  alarm(4);
		  fd = open(cul, O_RDWR);
		  alarm(0);
		  close(fd_tmp);
		  /*logent("AUTODIAL","SUCCESSFUL");*/
		  retval = fd;
		  break;

	  case CALLFAILED:
	  default:
		  close(fd_tmp);
		  retval = NO_ANS;
		  break;
	}
#ifndef	hpux
	signal(SIGINT, savint);
	signal(SIGHUP, savhup);
#else	hpux
# ifndef hp9000s500
	(void) sigvector(SIGINT, &savint, (struct sigvec *)0);
	(void) sigvector(SIGHUP, &savhup, (struct sigvec *)0);
# else hp9000s500
	if (savint.sv_mask == OLBELL_SIG)
		(void)signal(SIGINT, savint.sv_handler);
	else
		(void) sigvector(SIGINT, &savint, (struct sigvec *)0);
	if (savhup.sv_mask == OLBELL_SIG)
		(void)signal(SIGHUP, savhup.sv_handler);
	else
		(void) sigvector(SIGHUP, &savhup, (struct sigvec *)0);
# endif hp9000s500
#endif	hpux
	return(retval);
}


static alarmtr()
{
	longjmp(Sjbuf, 1);
}

static catch_int()
{
	intflag = YES;
	longjmp(Sjbuf, 1);
}
#ifdef HDBuucp /*HDB*/
/*
 * generate a vector of pointers (arps) to the
 * substrings in string "s".
 * Each substring is separated by blanks and/or tabs.
 *	s	-> string to analyze -- s GETS MODIFIED
 *	arps	-> array of pointers -- count + 1 pointers
 *	count	-> max number of fields
 * returns:
 *	i	-> # of subfields
 *	arps[i] = NULL
 */

static
getargs2(s, arps, count)
register char *s, *arps[];
{
	register int i;

	for (i = 0; i < count; i++) {
		while (*s == ' ' || *s == '\t')
			*s++ = '\0';
		if (*s == '\n')
			*s = '\0';
		if (*s == '\0')
			break;
		arps[i] = s++;
		while (*s != '\0' && *s != ' '
			&& *s != '\t' && *s != '\n')
				s++;
	}
	arps[i] = NULL;
	return(i);
}

/***
 *      bsfix(args) - remove backslashes from args
 *
 *      \123 style strings are collapsed into a single character
 *	\000 gets mapped into \N for further processing downline.
 *      \ at end of string is removed
 *	\t gets replaced by a tab
 *	\n gets replaced by a newline
 *	\r gets replaced by a carriage return
 *	\b gets replaced by a backspace
 *	\s gets replaced by a blank 
 *	any other unknown \ sequence is left intact for further processing
 *	downline.
 */

static void
bsfix (args)
char **args;
{
	register char *str, *to, *cp;
	register int num;

	for (; *args; args++) {
		str = *args;
		for (to = str; *str; str++) {
			if (*str == '\\') {
				if (str[1] == '\0')
					break;
				switch (*++str) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					for ( num = 0, cp = str
					    ; cp - str < 3
					    ; cp++
					    ) {
						if ('0' <= *cp && *cp <= '7') {
							num <<= 3;
							num += *cp - '0';
						}
						else
						    break;
					}
					if (num == 0) {
						*to++ = '\\';
						*to++ = 'N';
					} else
						*to++ = num;
					str = cp-1;
					break;

				case 't':
					*to++ = '\t';
					break;

				case 's':	
					*to++ = ' ';
					break;

				case 'n':
					*to++ = '\n';
					break;

				case 'r':
					*to++ = '\r';
					break;

				case 'b':
					*to++ = '\b';
					break;

				default:
					*to++ = '\\';
					*to++ = *str;
					break;
				}
			}
			else
				*to++ = *str;
		}
		*to = '\0';
	}
}
static char *
fdig(cp)
char *cp;
{
   	char *c;
        for (c = cp;*c; c++)
            if (*c >= '0' && *c <= '9')
		break;
        return(c);
}


/* The following is taken from V.2.1 uucp's callers.c */
#ifdef BSD4_2
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#ifdef UNET
#include  "UNET/unetio.h"
#include  "UNET/tcp.h"
#endif
/*
 *	to add a new caller:
 *	declare the function that knows how to call on the device,
 *	add a line to the callers table giving the name of the device
 *	(from Devices file) and the name of the function
 *	add the function to the end of this file
 */

#ifdef DIAL801
static int	dial801();
#endif

#ifdef DATAKIT
static int	dkcall();
#endif DATAKIT

#ifdef TCP
static int	unetcall();
static int	tcpcall();
#endif TCP

#ifdef SYTEK
static int	sytcall();
#endif SYTEK

static struct caller Caller[] = {

#ifdef DIAL801
	{"801",		dial801},
	{"212",		dial801},
#endif DIAL801

#ifdef TCP
#ifdef BSD4_2
	{"TCP",		tcpcall},	/* 4.2BSD sockets */
#else !BSD4_2
#ifdef UNET
	{"TCP",		unetcall},	/* 3com implementation of tcp */
	{"Unetserver",	unetcall},
#endif UNET
#endif BSD4_2
#endif TCP

#ifdef DATAKIT
	{"DK",		dkcall},	/* standard btl datakit caller */
#endif DATAKIT

#ifdef SYTEK
	{"Sytek",	sytcall},	/* untested but should work */
#endif SYTEK

	{NULL, 		NULL}		/* this line must be last */
};

/***
 *	exphone - expand phone number for given prefix and number
 *
 *	return code - none
 */

static void
exphone(in, out)
char *in, *out;
{
	FILE *fn;
	char pre[MAXPH], npart[MAXPH], tpre[MAXPH], p[MAXPH];
	char buf[NBUFSIZ];
	char *s1;

	if (!isalpha(*in)) {
		(void) strcpy(out, in);
		return;
	}

	s1=pre;
	while (isalpha(*in))
		*s1++ = *in++;
	*s1 = NULLCHAR;
	s1 = npart;
	while (*in != NULLCHAR)
		*s1++ = *in++;
	*s1 = NULLCHAR;

	tpre[0] = NULLCHAR;
	fn = fopen(DIALCODES, "r");

	if (fn != NULL) {
		while (fgets(buf, NBUFSIZ, fn)) {
			if ( sscanf(buf, "%s%s", p, tpre) < 1)
				continue;
			if (EQUALS(p, pre))
				break;
			tpre[0] = NULLCHAR;
		}
		fclose(fn);
	}

	(void) strcpy(out, tpre);
	(void) strcat(out, npart);
	return;
}

/*
 * repphone - Replace \D and \T sequences in arg with phone
 * expanding and translating as appropriate.
 */
static char *
repphone(arg, phone, trstr)
register char *arg, *phone, *trstr;
{
	extern void translate();
	static char pbuf[2*(MAXPH+2)];
	register char *fp, *tp;

	for (tp=pbuf; *arg; arg++) {
		if (*arg != '\\') {
			*tp++ = *arg;
			continue;
		} else {
			switch (*(arg+1)) {
			case 'T':
				exphone(phone, tp);
				translate(trstr, tp);
				for(; *tp; tp++)
				    ;
				arg++;
				break;
			case 'D':
				for(fp=phone; *tp = *fp++; tp++)
				    ;
				arg++;
				break;
			default:
				*tp++ = *arg;
				break;
			}
		}
	}
	*tp = '\0';
	return(pbuf);
}

/*
 * processdev - Process a line from the Devices file
 *
 * return codes:
 *	file descriptor  -  succeeded
 *	FAIL  -  failed
 */

static
processdev(flds, dev)
register char *flds[], *dev[];
{
        mflag status;
        struct termio lv;
        char buffer[1024];
        int hascalldev = 0;
	int err = 0;
	int tmp2 = -1;
	int tmp = -1;
	register int dcf = -1;
	register struct caller	*ca;
	char dnname[20];
	char *args[D_MAX+1], dcname[20];
	register char **sdev;
	extern void translate();
	register nullfd;
	char *phonecl;			/* clear phone string */
	char phoneex[2*(MAXPH+2)];	/* expanded phone string */

        DEBUG(1,"nomodem is %d\n",nomodem);
	sdev = dev;
	for (ca = Caller; ca->CA_type != NULL; ca++) {
		/* This will find built-in caller functions */
		if (EQUALS(ca->CA_type, dev[CALLER])) {
			DEBUG(5, "Internal caller type %s\n", dev[CALLER]);
			if (dev[ARG] == NULL) {
				/* if NULL - assume translate */
				dev[ARG+1] = NULL;	/* needed for for loop later to mark the end */
				dev[ARG] = "\\T";
			}
			dev[ARG] = repphone(dev[ARG], flds[F_PHONE], "");
			if ((dcf = (*(ca->CA_caller))(flds, dev)) < 0)
				return(dcf) ;
			dev += 2; /* Skip to next CALLER and ARG */
			break;
		}
	}
	if (dcf == -1) {
		/* Here if not a built-in caller function */
                /* We already locked it in dial() */
		/*if (mlock(dev[LINE]) == FAIL) { 
			DEBUG(5, "mlock %s failed\n", dev[LINE]);
			Uerror = DV_NT_A ;
			return(Uerror);
		}
		DEBUG(5, "mlock %s succeeded\n", dev[LINE]); */
		/*
		 * Open the line
		 */
		(void) sprintf(dcname, "/dev/%s", dev[LINE]);
		/* take care of the possible partial open fd */
		(void) close(nullfd = open("/", 0));
		if (setjmp(Sjbuf)) {
			(void) close(nullfd);
			DEBUG(1, "generic open timeout\n", "");
			/*logent("generic open", "TIMEOUT");*/
			Uerror = L_PROB; 
			goto bad;
		}
#ifndef hpux
		(void) signal(SIGALRM, alarmtr);
#else
                {
	        struct sigvec alarmvec;
                alarmvec.sv_handler = alarmtr;
                alarmvec.sv_mask = alarmvec.sv_onstack = 0;
                (void) sigvector(SIGALRM,&alarmvec,(struct sigvec *) 0);
                }
#endif
		(void) alarm(10); 
                tmp = open(dcname,O_NDELAY | O_RDWR);
                (void) alarm(0); 
                /* make it direct */
                err = fixline(tmp, atoi(fdig(flds[F_CLASS])), D_DIRECT);
                if (err) { Uerror = err; goto bad;}
                (void) alarm(10); 
                /* now this open should succeeded immediately */
		dcf = open(dcname, 2);       /* This open allows us to send
                                                commands to the modem */
		(void) alarm(0); 
		if (dcf < 0) {
			(void) close(nullfd);
			DEBUG(1, "generic open failed, errno = %d\n", errno);
			/*logent("generic open", "FAILED");*/
			Uerror = L_PROB;
			goto bad;
		}
                /* Already done above. Original AT&T code has it here */
                /*fixline(dcf, atoi(fdig(flds[F_CLASS])), D_DIRECT);*/
	}
        /* 2 methods for talking to device: 
           If the caller field contains the keyword PROG, exec the
           program, else use chat script
        */
        if (strlen(dev[CALLER]) > 4 && !strncmp(dev[CALLER],"PROG",4)){
             DEBUG(1,"Using Progname %s to dial\n",dev[CALLER]+4);
             Uerror = Prog(flds,dev);
             if (Uerror != 0 ) goto bad;
             goto Success;
        }
	/*
	 * Now loop through the remaining callers and chat
	 * according to scripts in dialers file.
	 */
	for (; dev[CALLER] != NULL; dev += 2) {
		register int w;
		/*
		 * Scan Dialers file to find an entry
		 */
		if ((w = gdial(dev[CALLER], args, D_MAX)) < 1) {
			DEBUG(1, "%s not found in Dialers file\n", dev[CALLER]);
			/*logent("generic call to gdial", "FAILED");*/
			Uerror = DV_NT_A; 
			goto bad;
		}
		if (w <= 2)	/* do nothing - no chat */
			break;
		/*
		 * Translate the phone number
		 */
		if (dev[ARG] == NULL) {
			/* if NULL - assume no translation */
			dev[ARG+1] = NULL; /* needed for for loop to mark the end */
			dev[ARG] = "\\D";
		}
		
		phonecl = repphone(dev[ARG], flds[F_PHONE], args[1]);
		exphone(phonecl, phoneex);
		translate(args[1], phoneex);
		/*
		 * Chat
		 */
		if (chat(w-2, &args[2], dcf, phonecl, phoneex) != SUCCESS) {
			Uerror = NO_ANS; 
			goto bad;
		}
	}
	/*
	 * Success at last!
	 */
Success:
	strcpy(Dc, sdev[LINE]);
        close(dcf);

        /* if call.modem = 0, set it back */
        if (!nomodem) {
        	ioctl(tmp,TCGETA,&lv);
        	lv.c_cflag &= ~CLOCAL;
                ioctl(tmp,TCSETA,&lv);
        }
        errno =0;
	if (setjmp(Sjbuf)) {
		DEBUG(1, "generic open timeout\n", "");
		Uerror = L_PROB; 
		goto bad;
	}
        alarm(10); 
      	dcf = open(dcname,O_RDWR);  
        alarm(0); 
				      /* Now that connection is
                                         established (with DCD high)
                                         we can open a file with
                                         real modem control */
      	close(tmp);                   /* close the original file */
	return(dcf);
bad:
	if (dcf > 0 )(void)close(dcf);
	if (tmp > 0 )(void)close(tmp);
	/*delock(sdev[LINE]);*/      /* done in undial() */
	return(Uerror);
}

/*
 * translate the pairs of characters present in the first
 * string whenever the first of the pair appears in the second
 * string.
 */
static void
translate(ttab, str)
register char *ttab, *str;
{
	register char *s;

	for(;*ttab && *(ttab+1); ttab += 2)
		for(s=str;*s;s++)
			if(*ttab == *s)
				*s = *(ttab+1);
}

#define MAXLINE	512
/*
 * Get the information about the dialer.
 
 *	type	-> type of dialer (e.g., penril)
 *	arps	-> array of pointers returned by gdial
 *	narps	-> number of elements in array returned by gdial
 * Return value:
 *	-1	-> Can't open DIALERFILE
 *	0	-> requested type not found
 *	>0	-> success - number of fields filled in
 */
static
gdial(type, arps, narps)
register char *type, *arps[];
register int narps;
{
	static char info[MAXLINE];
	register FILE *ldial;
	register na;

	DEBUG(2, "gdial(%s) called\n", type);
	if ((ldial = fopen(DIALERS, "r")) == NULL)
		return(-1);
	while (fgets(info, sizeof(info), ldial) != NULL) {
		if ((info[0] == '#') || (info[0] == ' ') ||
		    (info[0] == '\t') || (info[0] == '\n'))
			continue;
		if ((na = getargs2(info, arps, narps)) == 0)
			continue;
		if (EQUALS(arps[0], type)) {
			(void)fclose(ldial);
			bsfix(arps);
			return(na);
		}
	}
	(void)fclose(ldial);
	return(0);
}


#ifdef DATAKIT

/***
 *	dkcall(flds, dev)	make datakit ps connection
 *			datakit ps is a trademark of att (or is it bell labs?)
 *
 *	return codes:
 *		>0 - file number - ok
 *		FAIL - failed
 */

#include "dk.h"
static
dkcall(flds, dev)
char *flds[], *dev[];
{
	register fd;

	DEBUG(4, "dkcall(%s)\n", dev[ARG]);
	if (setjmp(Sjbuf)) {
		Uerror = SS_DIAL_FAILED;
		return(FAIL);
	}
#ifndef hpux
		(void) signal(SIGALRM, alarmtr);
#else
                {
                struct sigvec alarmvec;
                alarmvec.sv_handler = alarmtr;
                alarmvec.sv_mask = alarmvec.sv_onstack = 0;
                (void) sigvector(SIGALRM,&alarmvec,(struct sigvec *) 0);
                }
#endif
	(void) alarm(15);
#ifndef STANDALONE
	if (*dev[LINE] == '0')
#endif
		fd = dkdial(dev[ARG], 0, 0);
#ifndef STANDALONE
	else
		fd = dkdial(dev[ARG], dev[LINE], 0);
#endif
	(void) alarm(0);
	(void) strcpy(Dc, "DK");
	if (fd < 0) {
		Uerror = SS_DIAL_FAILED;
		return(FAIL);
	}
	else
		return(fd);
}

#endif DATAKIT

#ifdef TCP

/***
 *	tcpcall(flds, dev)	make ethernet/socket connection
 *
 *	return codes:
 *		>0 - file number - ok
 *		FAIL - failed
 */

#ifndef BSD4_2
/*ARGSUSED*/
static
tcpcall(flds, dev)
char	*flds[], *dev[];
{
	Uerror = SS_NO_DEVICE;
	return(FAIL);
}
#else BSD4_2
static
tcpcall(flds, dev)
char *flds[], *dev[];
{
	int ret;
	short port;
	extern int	errno, sys_nerr;
	extern char *sys_errlist[];
	struct servent *sp;
	struct hostent *hp;
	struct sockaddr_in sin;

	port = atoi(dev[ARG]);
	if (port == 0) {
		sp = getservbyname("uucp", "tcp");
		ASSERT(sp != NULL, "No uucp server", 0, 0);
		port = sp->s_port;
	}
	else port = htons(port);
	hp = gethostbyname(flds[F_NAME]);
	if (hp == NULL) {
		/*logent("tcpopen", "no such host");*/
		Uerror = SS_NO_DEVICE;
		return(FAIL);
	}
	DEBUG(4, "tcpdial host %s, ", flds[F_NAME]);
	DEBUG(4, "port %d\n", ntohs(port));

	ret = socket(AF_INET, SOCK_STREAM, 0);
	if (ret < 0) {
		if (errno < sys_nerr) {
			DEBUG(5, "no socket: %s\n", sys_errlist[errno]);
			/*logent("no socket", sys_errlist[errno]);*/
		}
		else {
			DEBUG(5, "no socket, errno %d\n", errno);
			/*logent("tcpopen", "NO SOCKET");*/
		}
		Uerror = SS_NO_DEVICE;
		return(FAIL);
	}
	sin.sin_family = hp->h_addrtype;
	bcopy(hp->h_addr, (caddr_t)&sin.sin_addr, hp->h_length);
	sin.sin_port = port;
	if (setjmp(Sjbuf)) {
		DEBUG(4, "timeout tcpopen\n", "");
		/*logent("tcpopen", "TIMEOUT");*/
		Uerror = SS_NO_DEVICE;
		return(FAIL);
	}
#ifndef hpux
		(void) signal(SIGALRM, alarmtr);
#else
                {
                struct sigvec alarmvec;
                alarmvec.sv_handler = alarmtr;
                alarmvec.sv_mask = alarmvec.sv_onstack = 0;
                (void) sigvector(SIGALRM,&alarmvec,(struct sigvec *) 0);
                }
#endif
	(void) alarm(30);
	DEBUG(7, "family: %d\n", sin.sin_family);
	DEBUG(7, "port: %d\n", sin.sin_port);
	DEBUG(7, "addr: %08x\n",*((int *) &sin.sin_addr));
	if (connect(ret, (caddr_t)&sin, sizeof (sin)) < 0) {
		(void) alarm(0);
		(void) close(ret);
		if (errno < sys_nerr) {
			DEBUG(5, "connect failed: %s\n", sys_errlist[errno]);
			/*logent("connect failed", sys_errlist[errno]);*/
		}
		else {
			DEBUG(5, "connect failed, errno %d\n", errno);
			/*logent("tcpopen", "CONNECT FAILED");*/
		}
		Uerror = SS_NO_DEVICE;
		return(FAIL);
	}
	(void) signal(SIGPIPE, SIG_IGN);  /* watch out for broken ipc link...*/
	(void) alarm(0);
	(void) strcpy(Dc, "IPC");
	return(ret);
}

#endif BSD4_2

/***
 *	unetcall(flds, dev)	make ethernet connection
 *
 *	return codes:
 *		>0 - file number - ok
 *		FAIL - failed
 */

#ifndef UNET
static
unetcall(flds, dev)
char	*flds[], *dev[];
{
	Uerror = SS_NO_DEVICE;
	return(FAIL);
}
#else UNET
static
unetcall(flds, dev)
char *flds[], *dev[];
{
	int ret;
	int port;
	extern int	errno;

	port = atoi(dev[ARG]);
	DEBUG(4, "unetdial host %s, ", flds[F_NAME]);
	DEBUG(4, "port %d\n", port);
	(void) alarm(30);
	ret = tcpopen(flds[F_NAME], port, 0, TO_ACTIVE, "rw");
	(void) alarm(0);
	endhnent();
	if (ret < 0) {
		DEBUG(5, "tcpopen failed: errno %d\n", errno);
		Uerror = SS_DIAL_FAILED;
		return(FAIL);
	}
	(void) strcpy(Dc, "UNET");
	return(ret);
}
#endif UNET

#endif TCP

#ifdef SYTEK

/****
 *	sytcall(flds, dev)	make a sytek connection
 *
 *	return codes:
 *		>0 - file number - ok
 *		FAIL - failed
 */

/*ARGSUSED*/
static
sytcall(flds, dev)
char *flds[], *dev[];
{
	extern int errno;
	int dcr, dcr2, nullfd, ret;
	char dcname[20], command[NBUFSIZ];

	(void) sprintf(dcname, "/dev/%s", dev[LINE]);
	DEBUG(4, "dc - %s, ", dcname);
	dcr = open(dcname, O_WRONLY|O_NDELAY);
	if (dcr < 0) {
		Uerror = SS_DIAL_FAILED;
		DEBUG(4, "OPEN FAILED %s\n", dcname);
		delock(dev[LINE]);
		return(FAIL);
	}

	sytfixline(dcr, atoi(fdig(dev[CLASS])), DIRECT);
	(void) sleep(2);
	(void) sprintf(command,"\r\rcall %s\r",flds[F_PHONE]);
	ret = write(dcr, command, strlen(command));
	(void) sleep(1);
	DEBUG(4, "COM1 return = %d\n", ret);
	sytfix2line(dcr);
	(void) close(nullfd = open("/", 0));
#ifndef hpux
		(void) signal(SIGALRM, alarmtr);
#else
               {
                struct sigvec alarmvec;
                alarmvec.sv_handler = alarmtr;
                alarmvec.sv_mask = alarmvec.sv_onstack = 0;
                (void) sigvector(SIGALRM,&alarmvec,(struct sigvec *) 0);
               }
#endif
	if (setjmp(Sjbuf)) {
		DEBUG(4, "timeout sytek open\n", "");
		(void) close(nullfd);
		(void) close(dcr2);
		(void) close(dcr);
		Uerror = SS_DIAL_FAILED;
		delock(dev[LINE]);
		return(FAIL);
	}
	(void) alarm(10);
	dcr2 = open(dcname,O_RDWR);
	(void) alarm(0);
	(void) close(dcr);
	if (dcr2 < 0) {
		DEBUG(4, "OPEN 2 FAILED %s\n", dcname);
		Uerror = SS_DIAL_FAILED;
		(void) close(nullfd);	/* kernel might think dc2 is open */
		delock(dev[LINE]);
		return(FAIL);
	}
	return(dcr2);
}

#endif SYTEK

#ifdef DIAL801

/***
 *	dial801(flds, dev)	dial remote machine on 801/801
 *	char *flds[], *dev[];
 *
 *	return codes:
 *		file descriptor  -  succeeded
 *		FAIL  -  failed
 *
 *	unfortunately, open801() is different for usg and non-usg
 */

/*ARGSUSED*/
static
dial801(flds, dev)
char *flds[], *dev[];
{
	char dcname[20], dnname[20], phone[MAXPH+2], *fdig();
	int dcf = -1, speed;

	if (mlock(dev[LINE]) == FAIL) {
		DEBUG(5, "mlock %s failed\n", dev[LINE]);
		Uerror = SS_LOCKEDEVICE;
		return(FAIL);
	}
	(void) sprintf(dnname, "/dev/%s", dev[CALLDEV]);
	(void) sprintf(phone, "%s%s", dev[ARG]   , ACULAST);
	(void) sprintf(dcname, "/dev/%s", dev[LINE]);
	CDEBUG(1, "Use Port %s, ", dcname);
	DEBUG(4, "acu - %s, ", dnname);
	CDEBUG(1, "Phone Number  %s\n", phone);
	VERBOSE("Trying modem - %s, ", dcname);	/* for cu */
	VERBOSE("acu - %s, ", dnname);	/* for cu */
	VERBOSE("calling  %s:  ", phone);	/* for cu */
	speed = atoi(fdig(dev[CLASS]));
	dcf = open801(dcname, dnname, phone, speed);
	if (dcf >= 0) {
		fixline(dcf, speed, ACU);
		(void) strcpy(Dc, dev[LINE]);	/* for later unlock() */
		VERBOSE("SUCCEEDED\n", 0);
	} else {
		delock(dev[LINE]);
		VERBOSE("FAILED\n", 0);
	}
	return(dcf);
}
static
open801(dcname, dnname, phone, speed)
char *dcname, *dnname, *phone;
{
	int nw, lt, dcf = -1, nullfd, dnf = -1, ret;
	unsigned timelim;

	(void) close(nullfd = open("/", 0));	/* partial open hack */
	if (setjmp(Sjbuf)) {
		DEBUG(4, "DN write %s\n", "timeout");
		(void) close(dnf);
		(void) close(dcf);
		(void) close(nullfd);
		Uerror = SS_DIAL_FAILED;
		return(FAIL);
	}
#ifndef hpux
		(void) signal(SIGALRM, alarmtr);
#else
                {
                struct sigvec alarmvec;
                alarmvec.sv_handler = alarmtr;
                alarmvec.sv_mask = alarmvec.sv_onstack = 0;
                (void) sigvector(SIGALRM,&alarmvec,(struct sigvec *) 0);
                }
#endif
	timelim = 5 * strlen(phone);
	(void) alarm(timelim < 30 ? 30 : timelim);

	if ((dnf = open(dnname, O_WRONLY)) < 0 ) {
		DEBUG(5, "can't open %s\n", dnname);
		Uerror = SS_CANT_ACCESS_DEVICE;
		return(FAIL);
	}
	DEBUG(5, "%s is open\n", dnname);
	if (  (dcf = open(dcname, O_RDWR | O_NDELAY)) < 0 ) {
		DEBUG(5, "can't open %s\n", dcname);
		Uerror = SS_CANT_ACCESS_DEVICE;
		return(FAIL);
	}

	DEBUG(4, "dcf is %d\n", dcf);
	fixline(dcf, speed, ACU);
	nw = write(dnf, phone, lt = strlen(phone));
	if (nw != lt) {
		(void) alarm(0);
		DEBUG(4, "ACU write error %d\n", errno);
		(void) close(dnf);
		(void) close(dcf);
		Uerror = SS_DIAL_FAILED;
		return(FAIL);
	} else 
		DEBUG(4, "ACU write ok%s\n", "");

	(void) close(dnf);
	(void) close(nullfd = open("/", 0));	/* partial open hack */
	ret = open(dcname, 2);  /* wait for carrier  */
	(void) alarm(0);
	(void) close(ret);	/* close 2nd modem open() */
	if (ret < 0) {		/* open() interrupted by alarm */
		DEBUG(4, "Line open %s\n", "failed");
		Uerror = SS_DIAL_FAILED;
		(void) close(nullfd);		/* close partially opened modem */
		return(FAIL);
	}
	(void) fcntl(dcf,F_SETFL, fcntl(dcf, F_GETFL, 0) & ~O_NDELAY);
	return(dcf);
}
#endif DIAL801

/* The following came from V.2.1's conn.c */

static int _Echoflag;
/*
 * chat -	do conversation
 * input:
 *	flds - fields from Systems file
 *	nf - number of fields in flds array
 *	dcr - write file number
 *	phstr1 - phone number to replace \D
 *	phstr2 - phone number to replace \T
 *
 *	return codes:  0  |  FAIL
 */

static
chat(nf, flds, fn, phstr1, phstr2)
char *flds[], *phstr1, *phstr2;
int nf, fn;
{
	char *want, *altern;
	extern char *index();
	int k, ok;

	_Echoflag = 0;
	for (k = 0; k < nf; k += 2) {
		want = flds[k];
		ok = FAIL;
		while (ok != 0) {
			altern = index(want, '-');
			if (altern != NULL)
				*altern++ = NULLCHAR;
			ok = expect(want, fn);
			if (ok == 0)
				break;
			if (altern == NULL) {
				/*Uerror = 6;   LOGIN FIALED 
				logent(UERRORTEXT, "FAILED");*/
				return(FAIL);
			}
			want = index(altern, '-');
			if (want != NULL)
				*want++ = NULLCHAR;
			sendthem(altern, fn, phstr1, phstr2);
		}
		sleep(2);
		if (flds[k+1])
		    sendthem(flds[k+1], fn, phstr1, phstr2);
	}
	return(0);
}

#define MR 300

/***
 *	expect(str, fn)	look for expected string
 *	char *str;
 *
 *	return codes:
 *		0  -  found
 *		FAIL  -  lost line or too many characters read
 *		some character  -  timed out
 */

static
expect(str, fn)
char *str;
int fn;
{
	static char rdvec[MR];
	char *rp = rdvec;
	register int kr, c;
	char nextch;
	extern	errno;

	*rp = 0;

	CDEBUG(4, "expect: (", 0);
	for (c=0; kr=str[c]; c++)
		if (kr < 040) {
			CDEBUG(4, "^%c", kr | 0100);
		} else
			CDEBUG(4, "%c", kr);
	CDEBUG(4, ")\n", 0);

	if (EQUALS(str, "\"\"")) {
		CDEBUG(4, "got it\n", 0);
		return(0);
	}
	if (setjmp(Sjbuf)) {
		return(FAIL);
	}
#ifndef hpux
		(void) signal(SIGALRM, alarmtr);
#else
                {
                struct sigvec alarmvec;
                alarmvec.sv_handler = alarmtr;
                alarmvec.sv_mask = alarmvec.sv_onstack = 0;
                (void) sigvector(SIGALRM,&alarmvec,(struct sigvec *) 0);
                }
#endif
	alarm(MAXEXPECTTIME);
	while (notin(str, rdvec)) {
		errno = 0;
		kr = read(fn, &nextch, 1);
		if (kr <= 0) {
			alarm(0);
			CDEBUG(4, "lost line errno - %d\n", errno);
			/*logent("LOGIN", "LOST LINE");*/
			return(FAIL);
		}
		c = nextch & 0177;
		CDEBUG(4, "%s", c < 040 ? "^" : "");
		CDEBUG(4, "%c", c < 040 ? c | 0100 : c);
		if ((*rp = nextch & 0177) != NULLCHAR)
			rp++;
		if (rp >= rdvec + MR) {
			CDEBUG(4, "enough already\n", 0);
			alarm(0);
			return(FAIL);
		}
		*rp = NULLCHAR;
	}
	alarm(0);
	CDEBUG(4, "got it\n", 0);
	return(0);
}


/***
 *	alarmtr()  -  catch alarm routine for "expect".
 */


/***
 *	sendthem(str, fn, phstr1, phstr2)	send line of chat sequence
 *	char *str, *phstr;
 *
 *	return codes:  none
 */

static
sendthem(str, fn, phstr1, phstr2)
char *str, *phstr1, *phstr2;
int fn;
{
	int sendcr = 1;
	register char	*sptr, *pptr;

	if (PREFIX("BREAK", str)) {
		/* send break */
		CDEBUG(5, "BREAK\n", 0);
		genbrk(fn);
		return;
	}

	if (EQUALS(str, "EOT")) {
		CDEBUG(5, "EOT\n", 0);
		(void) write(fn, EOTMSG, strlen(EOTMSG));
		return;
	}

	if (EQUALS(str, "\"\"")) {
		CDEBUG(5, "\"\"\n", 0);
		str += 2;
	}

	CDEBUG(5, "sendthem (", "");
	if (setjmp(Sjbuf))	/* Timer so echo check doesn't last forever */
		goto err;
#ifndef hpux
		(void) signal(SIGALRM, alarmtr);
#else
               { struct sigvec alarmvec;
                alarmvec.sv_handler = alarmtr;
                alarmvec.sv_mask = alarmvec.sv_onstack = 0;
                (void) sigvector(SIGALRM,&alarmvec,(struct sigvec *) 0);
               }
#endif
	alarm(MAXEXPECTTIME);
	for (sptr = str; *sptr; sptr++) {
		if (*sptr == '\\')
			switch(*++sptr) {
			case 'D':
				for (pptr=phstr1; *pptr; pptr++)
					if (wrchar(fn, pptr))
						goto err;
				continue;
			case 'T':
				for (pptr=phstr2; *pptr; pptr++)
					if (wrchar(fn, pptr))
						goto err;
				continue;
			case 'N':
				*sptr = 0;
				break;
			case 'd':
				CDEBUG(5, "DELAY\n", 0);
				sleep(2);
				continue;
			case 'c':
				if (*(sptr+1) == NULLCHAR) {
				CDEBUG(5, "<NO CR>", 0);
					sendcr = 0;
					continue;
				}
				CDEBUG(5, "<NO CR - MIDDLE IGNORED>\n", 0);
				continue;
			case 's':
				*sptr = ' ';
				break;
			case 'p':
				CDEBUG(5, "PAUSE\n", 0);
				nap(HZ/4);	/* approximately 1/4 second */
				continue;
			case 'E':
				CDEBUG(5, "ECHO CHECK ON\n", 0);
				_Echoflag = 1;
				continue;
			case 'e':
				CDEBUG(5, "ECHO CHECK OFF\n", 0);
				_Echoflag = 0;
				continue;
			case 'K':
				CDEBUG(5, "BREAK\n", 0);
				genbrk(fn);
				continue;
			case '\\':
				/* backslash escapes itself */
				break;
			default:
				/* send the backslash */
				--sptr;
				break;
			}
		if (wrchar(fn, sptr))
			goto err;
	}

	if (sendcr) {
		CDEBUG(5, "^M", 0);
		(void) write(fn, "\r", 1);
	}
err:
	alarm(0);
	CDEBUG(5, ")\n", 0);
	return;
}

static
wrchar(fn, sptr)
register int fn;
register char *sptr;
{
	if (GRPCHK(getgid()))	/* protect Systems file */
	{
		CDEBUG(5, "%s", *sptr < 040 ? "^" : "");
		CDEBUG(5, "%c", *sptr < 040 ? *sptr | 0100 : *sptr);
	}
	if ((write(fn, sptr, 1)) != 1)
		return(FAIL);
	if (_Echoflag) {
		char chr;
		while(1) {	/* Should set a timer here */
			if (read(fn, &chr, 1) != 1)
				return(FAIL);
			chr &= 0177;
			CDEBUG(4, "%s", chr < 040 ? "^" : "");
			CDEBUG(4, "%c", chr < 040 ? chr | 0100 : chr);
			if (chr == ((*sptr) & 0177))
				break;
		}

	}
	return(SUCCESS);
}


/***
 *	notin(sh, lg)	check for occurrence of substring "sh"
 *	char *sh, *lg;
 *
 *	return codes:
 *		0  -  found the string
 *		1  -  not in the string
 */

static
notin(sh, lg)
char *sh, *lg;
{
	while (*lg != NULLCHAR) {
		if (PREFIX(sh, lg))
			return(0);
		else
			lg++;
	}
	return(1);
}



#ifdef FASTTIMER
/*	Sleep in increments of 60ths of second.	*/
static nap (time)
register int time;
{
	static int fd;

	if (fd == 0)
		fd = open (FASTTIMER, 0);

	read (fd, 0, time);
}

#endif FASTTIMER

#if ! defined FASTTIMER && defined BSD4_2

	/* nap(n) -- sleep for 'n' ticks of 1/60th sec each. */
	/* This version uses the select system call */


static nap(n)
unsigned n;
{
	struct timeval tv;
	int rc;

	if (n==0)
		return;
	tv.tv_sec = n/60;
	tv.tv_usec = ((n%60)*1000000L)/60;
	rc = select(32, 0, 0, 0, &tv);
}

#endif ! FASTTIMER && BSD4_2

#if ! defined  FASTTIMER && !  defined BSD4_2

/*	nap(n) where n is ticks
 *
 *	loop using n/HZ part of a second
 *	if n represents more than 1 second, then
 *	use sleep(time) where time is the equivalent
 *	seconds rounded off to full seconds
 *	NOTE - this is a rough approximation and chews up
 *	processor resource!
 */

static nap(n)
unsigned n;
{
	struct tms	tbuf;
	long endtime;
	int i;

	if (n > HZ) {
		/* > second, use sleep, rounding time */
		sleep( (int) (((n)+HZ/2)/HZ) );
		return;
	}

	/* use timing loop for < 1 second */
	endtime = times(&tbuf) + 3*n/4;	/* use 3/4 because of scheduler! */
	while (times(&tbuf) < endtime) {
	    for (i=0; i<1000; i++, i*i)
		;
	}
	return;
}


#endif ! FASTTIMER && ! BSD4_2

/* The following is from V.2.1 cu culine.c */
static struct sg_spds {
	int	sp_val,
		sp_name;
} spds[] = {
	{ 110,  B110},
	{ 134,  B134},
	{ 150,  B150},
	{ 300,  B300},
	{ 600,  B600},
	{1200, B1200},
	{2400, B2400},
	{3600, B3600},
	{4800, B4800},
	{7200, B7200},
	{9600, B9600},
	{19200,	B19200},
	{38400,	B38400},
#ifdef EXTA
	{19200,	EXTA},
#endif
	{0,    0}
};

static struct termio Savettyb;
/*
 * set speed/echo/mode...
 *	tty 	-> terminal name
 *	spwant 	-> speed
 *	type	-> type
 *
 *	if spwant == 0, speed is untouched
 *	type is unused, but needed for compatibility
 *
 * return:  
 *	none
 */
static fixline(tty, spwant, type)
int	tty, spwant, type;
{
	register struct sg_spds	*ps;
	struct termio		*lvp,lv;
	int			speed = -1;

	DEBUG(6, "fixline(%d, ", tty);
	DEBUG(6, "%d)\n", spwant);
        if (!noterm) lvp = TermChar;  /* Copy terminal attr over */
        else {
                lvp = &lv;
		if (ioctl(tty, TCGETA, &lv) != 0)
			return;
        }

/* set line attributes associated with -h, -t, -e, and -o options */

        if (noterm){
		lvp->c_iflag = lvp->c_oflag = lvp->c_lflag = (ushort)0;
		lvp->c_iflag = (IGNPAR | IGNBRK | ISTRIP | IXON | IXOFF);
		lvp->c_cc[VEOF] = '\1';
        }

	if (spwant > 0) {
		for (ps = spds; ps->sp_val; ps++)
			if (ps->sp_val == spwant) {
				speed = ps->sp_name;
				break;
			}
		if (speed < 0) return(ILL_BD);
		lvp->c_cflag &= ~CBAUD; /* clear baud field */
		lvp->c_cflag |= speed; /* was just =. Change to |= since 
                                          it will clear parity bit */
	} else
		lvp->c_cflag &= CBAUD;

	lvp->c_cflag |= ( CREAD | (speed ? HUPCL : 0));
        lvp->c_cc[VTIME] = 1;
        lvp->c_cc[VMIN] = 1;  /* WAS 6 */
        if (!(lvp->c_cflag & CSIZE )) lvp->c_cflag |= CS8;
	/*   CLOCAL may cause problems on pdp11s with DHs */
	if (type == D_DIRECT) {
		DEBUG(4, "fixline - direct\n", "");
		lvp->c_cflag |= CLOCAL;
	} else 
		lvp->c_cflag &= ~CLOCAL;

	ioctl(tty, MCGETA, &_modemstat);
	_modemstat |= MRTS;
	if ( (_modemstat & MDTR) != MDTR) {
		_modemstat |= MDTR;
#ifdef  ddt
	if(_debug == YES) say(stderr,"MDTR was not set\n\r");
#endif
	}
	ioctl(tty, MCSETAW, &_modemstat);
	
        if (ioctl(tty,TCSETA,lvp) < 0 ) return(L_PROB);
	return;
}
static genbrk(fn)
register int	fn;
{
	if (isatty(fn)) 
		(void) ioctl(fn, TCSBRK, 0);
}
static Prog(flds,dev)
char *flds[];
char *dev[];
{
    int status = 0;
    int pid = -1;
    char prog[NBUFSIZ];
    int i,j;
    int tty;

    /* exec off prog specified in the dev vector */
    /* This allows the user to specifiy virtually any kind
       of program to do the dialing, for example dialit.
       The args to the program will be all the rest of
       the dev[] vector starting from the 5th field.
       The following is allowed though:
       \T -- translate to flds[PHONE]
       \S -- translate to dev[CLASS]
    */
    j=0;
    strcpy(prog,dev[CALLER]+4);
    dev[CALLER] = dev[CALLER]+4;
    CDEBUG(1,"Prog: %s\n",prog);
    for (i=CALLER+1; dev[i] != NULL; i++){
       if (!strcmp(dev[i],"\\P")){
                   /*strcpy(_protocol,"i");  */
                   dev[i] = _protocol;
       }
       if (!strcmp(dev[i],"\\S"))
           dev[i] = dev[CLASS];
       if (!strcmp(dev[i],"\\D") || !strcmp(dev[i],"\\T"))
           dev[i] = repphone(dev[i],flds[F_PHONE],"");
       /*CDEBUG(1,"dev[i] %s\n",dev[i]);*/
    }
    tty = open(ttyname(0),2);
    if ((pid =fork()) == 0){
        fflush(stdout);
        fflush(stderr);
	close(0);dup(tty);
        close(1);dup(tty);
        close(2);dup(tty);
        close(tty);
        setuid(getuid());
        execvp(prog,&dev[CALLER]);
        perror("execvp");
        exit(-1);
    }else
         j=wait(&status);
    if ((status & 0xffff) || j != pid || j == -1) {
       /*say("status upper %d\n",status >>8);*/
       say(stderr,"child retuned with status  %o\n",status);
       say(stderr,"child dies with error\n");
       return(NO_ANS);
    }
    return(0);
}
#endif /*HDB*/
