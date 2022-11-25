/* @(#) $Revision: 70.1 $ */    
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <ndir.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <utmp.h>
#include <pwd.h>
#include "ptyrequest.h"

#define FALSE 0
#define TRUE  1
#define CLOSE_ON_EXEC 1

#ifdef hp9000s500
#define MPTYMAJOR 45    /* major number for master pty's */
#define SPTYMAJOR 29    /* major number for slave  pty's */
#define PTYSC     0xfe  /* select code for pty's         */
#define select_code(x) (((x) & 0xff0000) >> 16)
#define bus_address(x) (((x) & 0xff00) >> 8)
#endif
#if defined(hp9000s200) || defined(hp9000s800)
#define MPTYMAJOR 16    /* major number for master pty's */
#define SPTYMAJOR 17    /* major number for slave  pty's */
#define PTYSC     0x00  /* select code for pty's         */
#define select_code(x) (((x) & 0xff0000) >> 16)
#define bus_address(x) ((x) & 0xff)
#endif

#define BLOCK_SIGCLD (1 << ( SIGCLD - 1))

#define ISSUE_FILE    "/etc/issue"
#define LOGFILE       "/etc/ptydaemonlog"

/* Parts of this program count on MPTYDIR and SPTYDIR beginning with  */
/* "/dev/" and don't bother checking. If you change this, beware of   */
/* this depedency.                                                    */

#define MPTYDIR    "/dev/ptym/" /* default for mptydir                 */
#define SPTYDIR    "/dev/pty/"  /* default for sptydir                 */
#define DEVOFFSET  5            /* number of characters in "/dev/"     */

char *mptydir = MPTYDIR;     /* directory to look in for master pty's  */
char *sptydir = SPTYDIR;     /* directory to look in for slave  pty's  */

struct ptyinfo {
    struct ptyinfo  *nextptr;
    char            *pty_master;
    char            *pty_slave;
    short           address;
    char            lock;
};

struct ptyinfo *ptylist = (struct ptyinfo *)0;

struct proc_table {
    int                 pid;
    char                *path;
    int                 argvlen;
    char                *argvbuf;
    int                 envplen;
    char                *envpbuf;
    char                *user;
    short               address;
    int                 puid;
    int                 pgid;
    int                 flags;
    struct proc_table   *nextptr;
    char                status;
};

struct proc_table *ptabhead = (struct proc_table *)0;

#define control(x)      ('x'&037) /* Used in initialization of dflt_termio */

static struct  termio  dflt_termio = {
	BRKINT|IGNPAR|IXON|ICRNL,
	OPOST|ONLCR|TAB3,
	CS8|CREAD|B9600|CLOCAL,
	ISIG|ICANON|ECHO|ECHOK|ECHOE,
	0,0177,control(\\),'#','@',control(D),0,0,0
};

FILE *logfptr;

extern int  errno;

extern struct passwd *getpwnam(), *getpwuid();
extern char *malloc();
extern char *strchr(), *strrchr();

void setflag(), alarmcatch(), removequeues(), createqueues(), childdeath();
void getptys(), checkentry(), allocate_pty(), childexit(), childhangup();
void get_request(), send_status(), send_ptypair(), send_packet();
void logerr(), setptrs(), parse(), doexec(), handledeath();

long get_packet();

int ptyreq_queue;     /* queue identifier for request  queue */
int ptyres_queue;     /* queue identifier for response queue */

/* Signal handler for SIGTERM */

int shutdownflag = FALSE;
int waitingflag  = FALSE;
int childflag    = FALSE;

void
setflag() {
    if (childflag == TRUE)
	exit(0);

    if (waitingflag == TRUE)
	removequeues();
    shutdownflag = TRUE;
    return;
}

/* Signal handler for SIGALRM */

jmp_buf Jmpbuf;

void
alarmcatch() {

    (void) longjmp(Jmpbuf,1);
}

int    master_fd = -1;    /* Needs to be global so that respawn() can close it */
int    deadchildcnt = 0;  /* Number of dead children                           */

main(argc,argv)
    int argc;
    char **argv;
{
    struct ptyinfo *ptptr;
    int i;
    struct sigvec vec;
    struct utmp *getutent();

    if (argc > 1) {
	mptydir = argv[1];
	if (argc > 2)
	    sptydir = argv[2];
    }

    /* Make sure we are root */

    if (geteuid() != 0) {
	fprintf(stderr,"Must be root to start up ptydaemon.\n");
	exit(1);
    }

    /* Fork and let parent die */

    switch (fork()) {
	case -1:
	    fprintf(stderr,"Fork failed. cannot put ptydaemon in background.\n");
	    exit(1);
	case 0:
	    break;
	default:
	    exit(0);
    }

    /* Ignore hangups, interrupts and quits. Do a setpgrp so that */
    /* we are not associated with anything.                       */

    setvec(vec, SIG_IGN);
    (void) sigvector(SIGHUP,  &vec, (struct sigvec *)0);
    (void) sigvector(SIGINT,  &vec, (struct sigvec *)0);
    (void) sigvector(SIGQUIT, &vec, (struct sigvec *)0);
    (void) setpgrp();

    /* Close all file descriptors */

    for (i=getnumfds()-1; i>=0; i--)
	close(i);

    /* Open /dev/null for file descriptors 0,1 & 2 */

    if ((i = open("/dev/null",O_RDONLY)) != 0)
	exit(1);
    dup(i);
    dup(i);

    /* Open error logging file and set to unbuffered */

    if ((logfptr = fopen(LOGFILE,"a+")) == (FILE *)0)
	exit(1);
    setbuf(logfptr,(char *)0);

    logerr("STARTUP","");

    /* Check to make sure the above dups succeeded (If we checked */
    /* before we wouldn't be able to log the error).              */

    if (fileno(logfptr) != 3) {
	logerr("Error in initial file descriptor set up","");
	exit(1);
    }

    /* Set the close on exec flag for the log file */

    if (fcntl(fileno(logfptr),F_SETFD,CLOSE_ON_EXEC) == -1) {
	logerr("Could not set close on exec flag for log file","");
	exit(1);
    }

    /* Open the utmp file as root so that children can write it after */
    /* they have setuid(). (The utmp routines have a static int that  */
    /* holds the file descriptor of the utmp file).                   */
    /* endutent()  is called in doexec to close the utmp file.        */
    /* endutent() is not called in account() (even though that would  */
    /* seem to be the logical location) since account() is called     */
    /* from the parent in handledeath() to do death accounting.       */

    if (getutent() == (struct utmp *)0) {
	logerr("Could not open utmp file","");
	exit(1);
    }

    getptys();
    createqueues();
    logerr("INITIALIZATION COMPLETE","");

    /* Catch SIGTERM in order to shut down nicely */

    setvec(vec, setflag);
    (void) sigvector(SIGTERM, &vec, (struct sigvec *)0);

    /* Catch SIGALRM */

    setvec(vec, alarmcatch);
    (void) sigvector(SIGALRM, &vec, (struct sigvec *)0);

    /* Catch SIGCLD */

    setvec(vec, childdeath);
    (void) sigvector(SIGCLD, &vec, (struct sigvec *)0);

    /* Only allow SIGCLD to be delivered at times that we are looking */
    /* for it.                                                        */

    (void) sigsetmask(BLOCK_SIGCLD);

    for(;;) {
	if (shutdownflag == TRUE)
	    removequeues();
	allocate_pty(&ptptr);
	get_request(ptptr);
    }
}

void
createqueues() {

    key_t qid, ftok();
    struct msqid_ds mbuf;

    if ((qid = ftok(PTYDAEMONPROG,REQID)) == (key_t) -1) {
	logerr("ftok failure for request queue","");
	exit(1);
    }

    if ((ptyreq_queue = msgget(qid,(0622 | IPC_CREAT | IPC_EXCL))) == -1) {
	if (errno == EEXIST)
	    logerr("request queue already exists","");
	else
	    logerr("Cannot create request queue","");
	exit(1);
    }

    if ((qid = ftok(PTYDAEMONPROG,RESID)) == (key_t) -1) {
	logerr("ftok failure for response queue","");
	exit(1);
    }

    if ((ptyres_queue = msgget(qid,(0644 | IPC_CREAT | IPC_EXCL))) == -1) {
	if (errno == EEXIST)
	    logerr("response queue already exists","");
	else
	    logerr("Cannot create response queue","");
	exit(1);
    }

    /* Limit size of response queue to one packet */

    if (msgctl(ptyres_queue,IPC_STAT,&mbuf) != 0) {
	logerr("Msgctl failure. Cannot get status of response queue","");
	exit(1);
    }

    mbuf.msg_qbytes = sizeof(struct ptymsgresbuf);
    if (msgctl(ptyres_queue,IPC_SET,&mbuf) != 0) {
	logerr("Msgctl failure. Cannot set response queue size","");
	exit(1);
    }
}

void
removequeues() {

    if (msgctl(ptyreq_queue,IPC_RMID,0) != 0)
	logerr("Msgctl failure. Cannot remove request queue.","");
    if (msgctl(ptyres_queue,IPC_RMID,0) != 0)
	logerr("Msgctl failure. Cannot remove response queue.","");
    logerr("SHUTDOWN","");
    exit(0);
}

void
allocate_pty(ptyptrptr)
    struct ptyinfo **ptyptrptr;
{

    /* Check to see if the last allocated master pty needs to be  */
    /* closed. This can occur if dorequest() fails before it      */
    /* closes master_fd.                                          */

    if (master_fd != -1) {
	close(master_fd);
	master_fd = -1;
    }

    /* start at top of list every time. */

    *ptyptrptr = ptylist;
    while (*ptyptrptr != (struct ptyinfo *)0) {
	    if ((*ptyptrptr)->lock == FALSE) {
		if ((master_fd = open((*ptyptrptr)->pty_master,O_RDWR)) >= 0)
		    return;
	    }
	    *ptyptrptr = (*ptyptrptr)->nextptr;
    }

    return;
}

void
get_request(ptyptr)
    struct ptyinfo *ptyptr;
{
    struct ptyrequest packet;
    long reqmtype;
    int ret;


    reqmtype = get_packet(&packet);

    /* Now that we have a request, check to see if we have a pre-   */
    /* allocated pty pair. If not, try to allocate one. If we still */
    /* can't allocate a pty pair then we send an error condition.   */

    if (ptyptr == (struct ptyinfo *)0) {
	allocate_pty(&ptyptr);
	if (ptyptr == (struct ptyinfo *)0) {
	    send_status(reqmtype,E_NOPTYERR);
	    logerr("No ptys available for request","");
	    return;
	}
    }

    /* process this request */

    ret = dorequest(ptyptr,&packet,reqmtype);

    if (ret != 0)
	logerr("dorequest error:",pty_errlist[ret]);
    return;
}

long
get_packet(packetptr)
    struct ptyrequest *packetptr;
{
    struct ptymsgreqbuf mbuf;
    int ret,errcount;
    int msgflag;

    /* Unblock SIGCLD */

    sigsetmask(0);

    /* If there are no dead children then just hang on msgrcv */
    /* for request. Otherwise just poll, and if there is no   */
    /* pending request then handle dead child.                */

    errcount = 0;
    for (;;) {
	if (deadchildcnt > 0) {
	    waitingflag = FALSE;
	    msgflag = IPC_NOWAIT;
	}
	else {
	    waitingflag = TRUE;
	    msgflag = 0;
	}

	if ((ret = msgrcv(ptyreq_queue,(struct msgbuf *)&mbuf,REQUESTSIZE,0,msgflag))
	     != REQUESTSIZE) {
	    if (ret != -1 || (errno != EINTR && errno != ENOMSG)) {
		    logerr("get_packet error","");
		    if (++errcount > 10)
			removequeues();
	    }
	    else {
		if (deadchildcnt > 0) {
		    sigsetmask(BLOCK_SIGCLD);
		    handledeath();
		    sigsetmask(0);
		}
	    }
	}
	else
	    break;
    }

    waitingflag = FALSE;
    sigsetmask(BLOCK_SIGCLD);

    /* Got a packet, copy results and return packet type */

    memcpy((char *)packetptr,mbuf.mtext,REQUESTSIZE);
    return(mbuf.mtype);
}

dorequest(ptyptr,ptyrqptr,reqmtype)
    struct ptyinfo *ptyptr;
    struct ptyrequest *ptyrqptr;
    long reqmtype;
{
    int status;
    int wantuid, wantgid;
    int loginflag;
    int pipefd[2];
    int childpid,nread;
    struct passwd *pw;
    struct proc_table *ptabentry, *procinsert();
    char *path, *user, *password;
    int  requid, reqgid;
    char *crypt();

#ifdef DEBUG
    logerr("entered dorequest","");
#endif
    if (ptyrqptr->pathlength == 0)
	path = "";
    else
	path = ptyrqptr->pool + ptyrqptr->pathoffset;

    user     = ptyrqptr->user;
    password = ptyrqptr->password;
    requid   = ptyrqptr->myuid;
    reqgid   = ptyrqptr->mygid;

    /* Check to see if we are going to allow this request */

    if (strcmp(path,LOGIN_PROGRAM) == 0) {
	loginflag = TRUE;
	user = "LOGIN";
	pw   = (struct passwd *)0;
	ptyrqptr->flags |= (CREATE_UTMP|CREATE_WTMP);
    }
    else {
	loginflag = FALSE;
	if (*user == '\0') {

	    /* set user to name corresponding to requid from password file */

	    if ((pw = getpwuid(requid)) == (struct passwd *)0) {
		send_status(reqmtype,E_PERMERR);
		return(E_PERMERR);
	    }
	    user    = pw->pw_name;
	    wantuid = requid;
	    wantgid = reqgid;
	}
	else {
	    if ((pw = getpwnam(user)) == (struct passwd *)0) {
		send_status(reqmtype,E_PERMERR);
		return(E_PERMERR);
	    }
	    if (requid != 0 && *(pw->pw_passwd) != '\0') {
		if (strcmp(crypt(password,pw->pw_passwd),pw->pw_passwd) != 0) {
		    send_status(reqmtype,E_PERMERR);
		    return(E_PERMERR);
		}
	    }
	    wantuid = pw->pw_uid;
	    wantgid = pw->pw_gid;
	}
    }

#ifdef DEBUG
	logerr("dorequest: allowed","");
#endif
    /* Ok we are going to allow it (If he's faking requid he won't be */
    /* able to open the master pty since we chown it to requid.       */

    /* Chown the master pty to the users uid and gid */

    if (chown(ptyptr->pty_master,requid,reqgid) != 0) {
	send_status(reqmtype,E_CHOWNERR);
	return(E_CHOWNERR);
    }

    /* Chmod the master pty so only the owner can open it */

    if  (chmod(ptyptr->pty_master,0600) != 0) {
	send_status(reqmtype,E_CHMODERR);
	return(E_CHMODERR);
    }

    /* Chmod the slave pty to a default value */

    if  (chmod(ptyptr->pty_slave,0622) != 0) {
	send_status(reqmtype,E_CHMODERR);
	return(E_CHMODERR);
    }

    if (loginflag == FALSE) {

	/* Chown the slave pty so that application using it can have */
	/* control of it.                                            */

	if (chown(ptyptr->pty_slave,wantuid,wantgid) != 0) {
	    send_status(reqmtype,E_CHOWNERR);
	    return(E_CHOWNERR);
	}
    }

#ifdef DEBUG
	logerr("dorequest: creating pipe","");
#endif
    /* Create the pipe for the child to send status on */

    if (pipe(pipefd) != 0) {
	send_status(reqmtype,E_NOPIPE);
	return(E_NOPIPE);
    }

    /* Now close the allocated master pty and send the names of the */
    /* allocated pty pair to the requester. From now on we can no   */
    /* longer use send_status() since we have completed the first   */
    /* step of the hand-shake.                                      */

    close(master_fd);
    master_fd = -1;
    send_ptypair(reqmtype,ptyptr->pty_master,ptyptr->pty_slave);

    /* If the requested program is an empty string just sleep for 3 */
    /* seconds and then return successfully.                        */

    if (*path == '\0') {
	close(pipefd[0]);
	close(pipefd[1]);
	sleep(3);
	return(0);
    }

    /* Fork and exec desired program. doexec() will complete the    */
    /* handshake by opening the slave pty.                          */

    if ((childpid = fork()) == -1) {
	return(E_NOFORK);
    }

    if (childpid != 0) {

	/* parent */

	ptabentry = procinsert(childpid,path,ptyrqptr->argvlength,
				ptyrqptr->pool + ptyrqptr->argvoffset,
				ptyrqptr->environlength,
				ptyrqptr->pool + ptyrqptr->environoffset,
				user,ptyptr->address,
				wantuid,wantgid,ptyrqptr->flags);

	/* Lock this ptypair as long as there is a child alive */
	/* that may be using it.                               */

	ptyptr->lock = TRUE;

	close(pipefd[1]);
	if ((nread = read(pipefd[0],(char *)&status,sizeof (int))) == 0) {
	    close(pipefd[0]);
	    return(0); /* exec succeeded */
	}
	close(pipefd[0]);
	if (nread != sizeof (int))
	    status = E_READERR;
	if (ptabentry != (struct proc_table *)0)
	    ptabentry->status = status;
	return(status);
    }

    /* child */

    childflag = TRUE;
    close(pipefd[0]);
    if (loginflag == FALSE) {
	setgid(wantgid);
	setuid(wantuid);
    }
    doexec(path,ptyrqptr->argvlength,ptyrqptr->pool + ptyrqptr->argvoffset,
	   ptyrqptr->environlength, ptyrqptr->pool + ptyrqptr->environoffset,
	   user,ptyptr->pty_slave, pw,pipefd[1],loginflag, ptyrqptr->flags);
}

struct proc_table *
procinsert(pid,path,argvlen,argvbuf,envplen,envpbuf,user,address,uid,gid,flags)
    int  pid;
    char *path;
    int argvlen;
    char *argvbuf;
    int envplen;
    char *envpbuf;
    char *user;
    short address;
    int  uid,gid,flags;
{
    struct proc_table *entry;
    extern char *malloc();
    char *space, *s;
    int  psize,usize,totalsize;

    psize = strlen(path) + 1;
    usize = strlen(user) + 1;

    totalsize = sizeof(struct proc_table) + psize + argvlen + envplen + usize;

    if ((space = malloc(totalsize)) == (char *)0) {
	logerr("Could not Malloc space for proc table entry","");
	return((struct proc_table *)0);
    }

    entry = (struct proc_table *) space;
    s     = space + sizeof (struct proc_table);
    strcpy(s,path);
    entry->path = s;

    s += psize;
    memcpy(s,argvbuf,argvlen);
    entry->argvbuf  = s;

    s += argvlen;
    memcpy(s,envpbuf,envplen);
    entry->envpbuf  = s;

    s += envplen;
    strcpy(s,user);
    entry->user    = s;

    entry->pid     = pid;
    entry->address = address;
    entry->puid    = uid;
    entry->pgid    = gid;
    entry->flags   = flags;
    entry->status  = (char) 0;

    /* Insert at front of list */

    entry->nextptr = ptabhead;
    ptabhead       = entry;

    return(entry);
}

void
send_status(mtype,status)
    long mtype;
    int  status;
{
    struct ptyresponse respacket;

    respacket.error = status;
    send_packet(mtype,&respacket);
}

void
send_ptypair(mtype,master,slave)
    long mtype;
    char *master, *slave;
{
    struct ptyresponse respacket;

    respacket.error = 0;
    strncpy(respacket.master,master,PTYNAMLEN);
    strncpy(respacket.slave, slave, PTYNAMLEN);
    respacket.master[PTYNAMLEN - 1] = respacket.slave[PTYNAMLEN - 1] = '\0';
    send_packet(mtype,&respacket);
}

void
send_packet(mtype,packetptr)
    long mtype;
    struct ptyresponse *packetptr;
{
    struct ptymsgresbuf mbuf;

#ifdef DEBUG
    logerr("entered send_packet starting","");
#endif
    /* First attempt to clear out the queue (queue length is one so we */
    /* only need to read once.                                         */

    msgrcv(ptyres_queue,(struct msgbuf *)&mbuf,RESPONSESIZE,0,IPC_NOWAIT);

    /* Now send the desired packet */

    mbuf.mtype = mtype;
    memcpy(mbuf.mtext,(char *)packetptr,RESPONSESIZE);

    if (setjmp(Jmpbuf) != 0) {
	logerr("send_packet failure","");
	return;
    }

    alarm(ALARM_TIME);
    if (msgsnd(ptyres_queue,(struct msgbuf *)&mbuf,RESPONSESIZE,0) != 0 ) {
	alarm(0);
	logerr("send_packet failure","");
	return;
    }
    alarm(0);
    return;
}

#define PATH  "PATH=/bin:/usr/bin:/usr/contrib/bin:/usr/local/bin"
#define SHELL "/bin/sh"
#define LOGINPROMPT "login: "

#define	MAXLINE		255
#define MAXARGS         64      /* Maximum number of args to "login" */
#define MAXENVARGS      32      /* Maximum number of environment variables */

/* childhangup() is SIGHUP signal handler during doexec */

void
childhangup()
{

    /* Flush both input and output queues */

    ioctl(0,TCFLSH,2); /* Its possible that fd 0 may not be open */
    _exit(0);
}

void
doexec(path,argvlen,argvbuf,envplen,envpbuf,user,slave_pty,pw,status_fd,loginflag,flags)
    char *path;
    int argvlen;
    char *argvbuf;
    int envplen;
    char *envpbuf;
    char *user;
    char *slave_pty;
    struct passwd *pw;
    int  status_fd,loginflag,flags;
{
    int i,j;
    char *s;
    int slave_fd;
    char loginbuf[MAXLINE],*largs[MAXARGS + 1];
    static char issue_buf[64];
    static char shell[64]   = { "SHELL=" };
    static char home[64]    = { "HOME=" };
    static char logname[30] = { "LOGNAME=" };
    static char mail[30]    = { "MAIL=/usr/mail/" };
    static char *newenviron[MAXENVARGS];
    int envargcount;
    struct sigvec vec;

    if (fcntl(status_fd,F_SETFD,CLOSE_ON_EXEC) == -1)
	childexit(status_fd,E_FCNTLERR);

    /* Close stdin, stdout & stderr, do a setpgrp to become a process     */
    /* group leader and then open the slave pty for stdin and dup it for  */
    /* stdout & stderr.                                                   */

    close(0);
    close(1);
    close(2);
    setpgrp();

    /* To complete the handshake we wait for the requester to open  */
    /* the master pty. We will know that he has opened it by trying */
    /* to open the slave side. The open will hang until the master  */
    /* side is opened. We will set an alarm, just in case something */
    /* goes wrong on the requester end.                             */

    if (setjmp(Jmpbuf) != 0) {
	childexit(status_fd,E_OPENERR);
    }

    /* Set the signal handler for SIGHUP to childhangup() */

    setvec(vec,childhangup);
    (void) sigvector(SIGHUP, &vec, (struct sigvec *)0);

    alarm(ALARM_TIME);
    do {
	slave_fd = open(slave_pty,O_RDWR);
	if (slave_fd < 0)
	    sleep(1);
    } while (slave_fd < 0);
    alarm(0);

    if (slave_fd != 0)
	childexit(status_fd,E_OPENERR);

    dup(slave_fd);
    dup(slave_fd);

    /* Set default modes on line */

    ioctl(slave_fd,TCSETA,&dflt_termio);

    /* Do utmp & wtmp accounting as desired */

    if (flags & (CREATE_UTMP | CREATE_WTMP)) {

	if (loginflag == TRUE)
	    i = account(LOGIN_PROCESS,getpid(),user,slave_pty,flags);
	else
	    i = account(USER_PROCESS,getpid(),user,slave_pty,flags);

	if (i != 0)
	    childexit(status_fd,i);
    }

    /* Call endutent() to close the utmp file (opened in main()). It is */
    /* called here instead of in account() since account is also called */
    /* from the parent to do death accounting.                          */

    endutent();

    /* Set up environment variables */

    envargcount = 0;
    newenviron[0] = (char *)0;
    if (pw != (struct passwd *)0) {
	(void) strcat(home, pw->pw_dir);
	if (chdir(pw->pw_dir) != 0)
	    chdir("/"); /* Ignore Error */
	(void) strcat(logname, pw->pw_name);
	(void) strcat(mail,pw->pw_name);
	if (*pw->pw_shell == '\0')
	    (void) strcat(shell,SHELL);
	else
	    (void) strcat(shell,pw->pw_shell);

	newenviron[envargcount++] = PATH;
	newenviron[envargcount++] = home;
	newenviron[envargcount++] = logname;
	newenviron[envargcount++] = mail;
	newenviron[envargcount++] = shell;
	newenviron[envargcount]   = (char *)0;
    }
    else {
	chdir("/"); /* Ignore error */
    }

    /* Reset all signals  except SIGHUP (signal 1) to SIG_DFL */
    /* Since SIGHUP is being caught it will automatically be  */
    /* reset at exec time.                                    */

    setvec(vec,SIG_DFL);
    for (i = 2; i < NSIG; i++)
	(void) sigvector(i, &vec, (struct sigvec *)0);

    /* Restore the signal mask to 0 */

    (void) sigsetmask(0);

    if (loginflag == TRUE) {

	/* Let parent go on */

	/* Tell parent everything went ok */

	i = 0;
	write(status_fd,(char *)&i,sizeof(int));

	/* Since we just told the parent everything is ok we cannot */
	/* do anything about errors, so we stop checking for them   */
	/* from here on.                                            */

	/* this sleep is a KLUDGE to fix a problem with uucp over vt.  The
	 * parent does an ioctl(,TIOCTTY,) and that seems to flush the writes
	 * and mess up the read.  The NOTTY bit is set in the flags, so we
	 * check for it here and sleep to prevent race conditions.
	 */

	if (flags & NOTTY)
		sleep(10);


	/* Send a newline */

	write(slave_fd,"\n",1);

	for (;;) {

	    /* if PRINTISSUE flag is set then copy /etc/issue to slave */
	    /* All errors are non-fatal (ignored).                     */

	    if (flags & PRINTISSUE) {
		if ((i = open(ISSUE_FILE,O_RDONLY)) >= 0) {
		    while ((j = read(i,issue_buf,64)) > 0)
			write(slave_fd,issue_buf,j);
		    close(i);
		}
	    }

	    /* If write fails it probably means that the master pty */
	    /* has been closed, so we should exit and try again.    */

	    i = strlen(LOGINPROMPT);
	    if (write(slave_fd,LOGINPROMPT,i) != i)
		_exit(0);
#ifdef DEBUG
	    logerr("wrote login prompt","");	
#endif

	    if ((i = readline(slave_fd,loginbuf,MAXLINE)) == -1)
		_exit(0);

#ifdef DEBUG
	    logerr("read a line","");	
	    logerr(loginbuf,"");
#endif
	    if (i > 0)
	    {
		largs[0] = "login";
		parse(loginbuf,&largs[1],MAXARGS-1);
		largs[MAXARGS] = (char *)0;

		/* Close security hole:  prevent user from */
		/* spoofing login by running login -r.	   */

		if (largs [1] [0] != '-')
		    break;

		{
		    char *message = "login names may not start with '-'.\n";
		    i = strlen (message);
		    write(slave_fd,message,i);
		}
	    }
	}

	/* exec login program */

#ifdef DEBUG
	    logerr("exec login","");
#endif
	execve(LOGIN_PROGRAM,largs,newenviron);
	_exit(0);
    }
    else {

	/* add any desired environment variables */

	if (envplen != 0) {
	    setptrs(envplen,envpbuf,&newenviron[envargcount],
		    MAXENVARGS - (envargcount + 1));
	    newenviron[MAXENVARGS] = (char *)0;
	}

	/* set up argv */

	setptrs(argvlen,argvbuf,&largs[0],MAXARGS - 1);
	largs[MAXARGS] = (char *)0;

	if (largs[0] == (char *)0) {

	    if ((s = strrchr(path,'/')) == (char *)0)
		largs[0] = path;
	    else
		largs[0] = ++s;
	}

	/* exec the desired program */

	execve(path,largs,newenviron);
	childexit(status_fd,E_EXECERR);
    }
}

void
setptrs(buflen,buf,args,cnt)
    int buflen;
    char *buf;
    char **args;
    int cnt;
{
    register int i;
    char *s,*endbuf;

    for (i=0; i < cnt; i++)
	args[i] = (char *)0;

    s = buf;
    endbuf = buf + buflen;
    for (i = 0; i < cnt && s < endbuf; i++) {
	args[i] = s;

	while (*s++ != '\0')
	    ;
    }
    return;
}

/*      parse() and quoted() were taken directly from System V getty()  */
/*      The only change I made was to reformat the code to my tastes.   */
/*      (don't blame me for the code! jsm).                             */

/*	"parse" breaks up the user's response into seperate arguments	*/
/*	and fills the supplied array with those arguments.  Quoting	*/
/*	with the backspace is allowed.					*/

void
parse(string,args,cnt)
    char *string,**args;
    int cnt;
{
    register char *ptrin,*ptrout;
    register int i;
    extern char quoted();
    int qsize;

    for (i=0; i < cnt; i++)
	args[i] = (char *)0;

    for (ptrin = ptrout = string,i=0; *ptrin != '\0' && i < cnt; i++) {

	/* Skip excess white spaces between arguments.  */

	while(*ptrin == ' ' || *ptrin == '\t') {
	    ptrin++;
	    ptrout++;
	}

	/* Save the address of the argument if there    */
	/* is something there.                          */

	if (*ptrin == '\0')
	    break;
	else
	    args[i] = ptrout;

	/* Span the argument itself.  The '\' character */
	/* causes quoting of the next character to take */
	/* place (except for '\0').                     */

	while (*ptrin != '\0') {

	    /* Is this the quote character? */

	    if (*ptrin == '\\') {
		    *ptrout++ = quoted(ptrin,&qsize);
		    ptrin += qsize;

	    }
	    else {

		/* Is this the end of the argument? If  */
		/* so quit the loop.                    */

		if (*ptrin == ' ' || *ptrin == '\t') {
		    ptrin++;
		    break;

		}
		else {

		    /* If this is a normal letter of    */
		    /* the argument, save it, advancing */
		    /* the pointers at the same time.   */

		    *ptrout++ = *ptrin++;
		}
	    }
	}

	/* Null terminate the string. */

	*ptrout++ = '\0';
    }
}

/*      quoted() takes a quoted character, starting at the quote        */
/*	character, and returns a single character plus the size of	*/
/*	the quote string.  "quoted" recognizes the following as		*/
/*	special, \n,\r,\v,\t,\b,\f as well as the \nnn notation.	*/

char quoted(ptr,qsize)
char *ptr;
int *qsize;
{
    register char c,*rptr;
    register int i;

    rptr = ptr;
    switch(*++rptr) {

    case 'n':
	c = '\n';
	break;

    case 'r':
	c = '\r';
	break;

    case 'v':
	c = '\013';
	break;

    case 'b':
	c = '\b';
	break;

    case 't':
	c = '\t';
	break;

    case 'f':
	c = '\f';
	break;

    default:

	/* If this is a numeric string, take up to three characters */
	/* of it as the value of the quoted character.              */

	if (*rptr >= '0' && *rptr <= '7') {

	    for (i=0,c=0; i < 3;i++) {
		c = c*8 + (*rptr - '0');
		if (*++rptr < '0' || *rptr > '7') break;
	    }
	    rptr--;

	}
	else {

	    /* If the character following the '\\' is a NULL, back  */
	    /* up the ptr so that the NULL won't be missed. The     */
	    /* sequence backslash null is essentially illegal.      */

	    if (*rptr == '\0') {
		c = '\0';
		rptr--;

	    }
	    else {

		/* In all other cases the quoting does nothing. */

		c = *rptr;
	    }
	}
	break;
    }

    /* Compute the size of the quoted character. */

    (*qsize) = rptr - ptr + 1;
    return(c);
}

account(type,pid,user,slave,flags)
    short type,pid;
    char  *user,*slave;
    int   flags;
{
    struct utmp utmp, *oldu;
    FILE *fp;
    int writeit;
    char *s;

    /* initialize utmp structure */

    memset(utmp.ut_user,'\0',sizeof(utmp.ut_user));
    memset(utmp.ut_line,'\0',sizeof(utmp.ut_line));
    strncpy(utmp.ut_user,user,sizeof(utmp.ut_user));
    strncpy(utmp.ut_line,slave + DEVOFFSET,sizeof(utmp.ut_line));
    utmp.ut_pid  = pid;
    utmp.ut_type = type;

    s = strrchr(slave,'/') + 1; /* Must succeed since slave_pty */
				/* begins with /dev/.           */
    if (strncmp(s,"tty",3) == 0)
	s += 3;
    utmp.ut_id[0] = *s++;
    utmp.ut_id[1] = *s;
    utmp.ut_id[2] = utmp.ut_id[3] = '\0';

    utmp.ut_exit.e_termination = 0;
    utmp.ut_exit.e_exit = 0;
    (void) time(&utmp.ut_time);

    if (flags & CREATE_UTMP) {
	writeit = TRUE;
	if (type == DEAD_PROCESS) {

	    setutent();     /* Start at beginning of utmp file. */
	    if ((oldu = getutid(&utmp)) != (struct utmp *)0) {

		   /* Copy in the old "user" and "line" fields */
		   /* to our new structure.                    */
		    memcpy(&utmp.ut_user[0],&oldu->ut_user[0],sizeof(utmp.ut_user));
		    memcpy(&utmp.ut_line[0],&oldu->ut_line[0],sizeof(utmp.ut_line));
	    }
	    else
		writeit = FALSE;
	}
	if (writeit == FALSE || _pututline(&utmp) == (struct utmp *)0)
	    return(E_UTMPERR);
    }

    if (flags & CREATE_WTMP) {

	/* Attempt to add to the end of the wtmp file.  Do not create */
	/* if it doesn't already exist.  * Note * This is the reason  */
	/* "r+" is used instead of "a+". "r+" won't create a file,    */
	/* while "a+" will. Don't bomb out if this fails.             */

	if ((fp = fopen(WTMP_FILE,"r+")) != (FILE *)0) {
		fseek(fp,0L,2); /* Seek to end of file */
		fwrite(&utmp,sizeof(struct utmp),1,fp);
		fclose(fp);
	}
    }
    return(0);
}

void
childexit(fd,status)
    int fd,status;
{
    write(fd,(char *)&status,sizeof(int));
    close(fd);
    _exit(0);
}

void
childdeath()
{                     /* Don't handle death right now. This    */
    deadchildcnt++;   /* will insure that pending requests     */
}                     /* will have priority over dead children */

void
handledeath()
{
    int pid,retstatus;
    struct proc_table **ptabptrptr, *ptabptr;
    struct ptyinfo *ptyptr;
    struct sigvec vec;
    int ret;

again:
    if ((pid = wait(&retstatus)) == -1) {
	if (errno == EINTR)
	    goto again;
	else {
	    return;
	}
    }

    /* Reinstall handler if deadchildcnt is 0 to take advantage of  */
    /* Bell SIGCLD semantics (Rescan for zombies in case we lost    */
    /* a SIGCLD).                                                   */

    if (--deadchildcnt == 0) {
	setvec(vec, childdeath);
	(void) sigvector(SIGCLD, &vec, (struct sigvec *)0);
    }

    /* Find child in proc table */

    ptabptrptr = &ptabhead;

    while (*ptabptrptr != (struct proc_table *)0 && (*ptabptrptr)->pid != pid)
	ptabptrptr = &((*ptabptrptr)->nextptr);
    ptabptr = *ptabptrptr;

    if (ptabptr == (struct proc_table *)0) {
	logerr("Could not find pid in proc table","");
	return;
    }

    /* locate pty pair in ptylist */

    ptyptr = ptylist;
    while (ptyptr->address != ptabptr->address) /* Must be on list */
	ptyptr = ptyptr->nextptr;

    /* Do dead accounting if necessary */

    if (ptabptr->flags & (CREATE_UTMP | CREATE_WTMP))
	account(DEAD_PROCESS,ptabptr->pid,ptabptr->user,
			     ptyptr->pty_slave,ptabptr->flags);

    if ( ptabptr->status != 0 || respawn(ptabptr,ptyptr) == 2) {

	/* Remove this entry from the proc table */
	/* and unlock the pty pair.              */

	chown(ptyptr->pty_master,ROOTUID,OTHERGID);
	chown(ptyptr->pty_slave ,ROOTUID,OTHERGID);
	chmod(ptyptr->pty_master,0666);
	chmod(ptyptr->pty_slave ,0666);
	ptyptr->lock = FALSE;
	*ptabptrptr = ptabptr->nextptr;
	free(ptabptr);
    }
    return;
}

/* respawn() -- respawn process. Return Value:                             */
/*                0) Success               (Don't remove proc table entry) */
/*                1) Failure After fork()  (Don't remove proc table entry) */
/*                2) Failure Before fork() (Remove proc table entry)       */

respawn(p,ptyptr)
    struct proc_table *p;
    struct ptyinfo *ptyptr;
{
    int pipefd[2];
    int childpid,status,nread;
    int loginflag;
    struct passwd *pw;
    int tmpfd;

    /* Check for RESPAWN flag and if it is not set, just open the slave  */
    /* side and set the baud rate to zero to indicate we died.           */

    if ((p->flags & RESPAWN) == 0) {

	if ( (childpid = fork()) == -1)
	    return(2);

	if (childpid == 0) {

	    /* child */

	    childflag = TRUE;

	    if (master_fd != -1)  /* Don't let child inherit currently */
		close(master_fd); /* allocated master pty.             */

	    if (setjmp(Jmpbuf) != 0)
		_exit(0);

	    alarm(ALARM_TIME);
	    tmpfd = open(ptyptr->pty_slave,O_RDWR);

	    dflt_termio.c_cflag &= ~CBAUD;

	    if (tmpfd >= 0)
		(void) ioctl(tmpfd,TCSETA,&dflt_termio);

	    _exit(0);
	}

	/* Parent */

	p->pid    = childpid;
	p->flags  = 0;
	p->status = 1;
	return(1);
    }

    if (strcmp(p->path,LOGIN_PROGRAM) == 0) {
	loginflag = TRUE;
	pw = (struct passwd *)0;
    }
    else {
	loginflag = FALSE;
	pw = getpwnam(p->user); /* Non - Fatal if fails */
    }

    /* Create the pipe for the child to send status on */

    if (pipe(pipefd) != 0)
	return(2);  /* Failure before fork */

    /* Fork and exec desired program. */

    if ((childpid = fork()) == -1) {
	return(2);  /* Failure before fork */
    }

    if (childpid != 0) {

	/* parent */

	p->pid = childpid;
	close(pipefd[1]);
	if ((nread = read(pipefd[0],(char *)&status,sizeof (int))) == 0) {
	    close(pipefd[0]);
	    return(0); /* exec succeeded */
	}
	close(pipefd[0]);
	if (nread != sizeof (int))
	    status = E_READERR;

	p->status = status;
	return(1);  /* Failure after fork */
    }

    /* child */

    childflag = TRUE;

    if (master_fd != -1)  /* Don't let child inherit currently allocated */
	close(master_fd); /* master pty.                                 */
    close(pipefd[0]);
    if (loginflag == FALSE) {
	setgid(p->pgid);
	setuid(p->puid);
    }

    doexec(p->path,p->argvlen,p->argvbuf,p->envplen,p->envpbuf,p->user,
	   ptyptr->pty_slave,pw,pipefd[1],loginflag,p->flags);
}

void
getptys()
{
    DIR *dfptr;
    struct direct *dentry;
    struct ptyinfo **ptptrptr,*ptptr;

    /* find all master ptys */

    if ((dfptr = opendir(mptydir)) == (DIR *)0) {
	logerr("Cannot open master pty directory:",mptydir);
	exit(1);
    }

    while ((dentry = readdir(dfptr)) != (struct direct *)0)
	checkentry(dentry,MPTYMAJOR);
    closedir(dfptr);

    /* If no masters found, exit here */

    if (ptylist == (struct ptyinfo *)0) {
	logerr("No master pty's found","");
	exit(1);
    }

    /* find all slave ptys */

    if ((dfptr = opendir(sptydir)) == (DIR *)0) {
	logerr("Could not open slave pty directory:",sptydir);
	exit(1);
    }

    while ((dentry = readdir(dfptr)) != (struct direct *)0)
	checkentry(dentry,SPTYMAJOR);
    closedir(dfptr);

    /* Remove all entries for master pty's that have no corresponding */
    /* slave pty.                                                     */

    ptptrptr = &ptylist;
    ptptr     = ptylist;
    while (ptptr != (struct ptyinfo *)0) {
	if (ptptr->pty_slave == (char *)0) {

	    /* remove this entry */

	    *ptptrptr = ptptr->nextptr;
	    free((char *)ptptr);

	}
	else {

	    /* go to next entry */

	    ptptrptr = &(ptptr->nextptr);
	}
	ptptr = *ptptrptr;
    }

    /* If no masters left, exit here */

    if (ptylist == (struct ptyinfo *)0) {
	logerr("No pty pairs found","");
	exit(1);
    }
}

void
checkentry(dentry,majorno)
    struct direct *dentry;
    long majorno;
{
    static char ptyname[PTYNAMLEN];
    struct stat sbuf;
    long minorno;
    short address;
    struct ptyinfo **ptptrptr,*ptptr, *newcellptr;
    extern struct ptyinfo *initptycell();

    /* construct name */

    if (majorno == MPTYMAJOR)
	strcpy(ptyname,mptydir);
    else
	strcpy(ptyname,sptydir);
    strcat(ptyname,dentry->d_name);

    /* try to stat it. If the stat fails just ignore it */

    if (stat(ptyname,&sbuf) == 0) {
	if ((sbuf.st_mode & S_IFMT) == S_IFCHR && major(sbuf.st_rdev) == majorno) {
	    minorno = minor(sbuf.st_rdev);
	    if (select_code(minorno) == PTYSC) {

		address = bus_address(minorno);
		ptptrptr = &ptylist;
		ptptr    = ptylist;
		if (majorno == MPTYMAJOR) {

		    /* find position and insert new cell */

		    while (ptptr != (struct ptyinfo *)0 && ptptr->address < address) {
			ptptrptr = &(ptptr->nextptr);
			ptptr    = *ptptrptr;
		    }
		    if (ptptr == (struct ptyinfo *)0 || ptptr->address != address) {
			newcellptr = initptycell(ptyname,address);
			if (newcellptr != (struct ptyinfo *)0) {
			    newcellptr->nextptr = ptptr;      /* link in  */
			    *ptptrptr           = newcellptr; /* new cell */
			}
		    }
		}
		else {

		    /* find corresponding master pty. If not found then */
		    /* ignore this entry.                               */

		    while (ptptr != (struct ptyinfo *)0 && ptptr->address != address) {
			ptptrptr = &(ptptr->nextptr);
			ptptr    = *ptptrptr;
		    }
		    if (ptptr != (struct ptyinfo *)0 && ptptr->pty_slave == (char *)0) {
			if ((ptptr->pty_slave = malloc(strlen(ptyname) + 1)) != (char *)0)
			    strcpy(ptptr->pty_slave,ptyname);
		    }
		}
	    }
	}
    }
}

struct ptyinfo *
initptycell(masterptyname,address)
    char *masterptyname;
    short address;
{
    struct ptyinfo *cellptr;
    char *namebuf;

    cellptr = (struct ptyinfo *)malloc(sizeof(struct ptyinfo));
    if (cellptr != (struct ptyinfo *)0) {
	if ((namebuf = malloc(strlen(masterptyname) + 1)) == (char *)0)
	    return((struct ptyinfo *)0);

	strcpy(namebuf,masterptyname);
	cellptr->pty_master = namebuf;
	cellptr->pty_slave  = (char *)0;
	cellptr->address    = address;
	cellptr->lock       = FALSE;
	cellptr->nextptr    = (struct ptyinfo *)0;
    }
    return (cellptr);
}

void
logerr(s1,s2)
    char *s1,*s2;
{
    char *s3,*ctime();
    long curtime;

    curtime = time(0);
    s3 = ctime(&curtime);
    s3[strlen(s3) - 1] = '\0';
    fprintf(logfptr,"%s pid=%d: %s %s\n",s3,getpid(),s1,s2);
    fflush(logfptr);
}
