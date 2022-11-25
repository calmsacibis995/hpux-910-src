/* @(#) $Revision: 70.1 $ */    

#include <stdio.h>
#include <fcntl.h>
#include <netio.h>
#include <time.h>
#include <setjmp.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/ptyio.h>
#include "ptyrequest.h"

extern int errno;

#define CLOSE_ON_EXEC 1
#define BITS_PER_INT  32

#define MAXUUCPDEVS 48
#define MAXLANDEVS  4
#define MAXLANDEVLEN 32

struct uucp_config {
    int  ptyfd;
    int  vtpid;
    char calldevname[PTYNAMLEN];
    char mastername[PTYNAMLEN];
    char nodename[NODENAMESIZE];
    char landevname[MAXLANDEVLEN];
} uuconfiglist[MAXUUCPDEVS];

int uudevcnt = 0;

#define LOGFILE      "/etc/vtdaemonlog"
#define UUCONFIGFILE "/usr/lib/uucp/L-vtdevices"
FILE *logfptr;

#define RESFD 0        /* File descriptor of lan device for responses */
int landevcnt;         /* Number of lan devices                       */


struct lan_config {
    long lantime;
    int lanfd;                /* contains file descriptor of lan device */
    char *landevice;          /* contains lan device name               */
    char laddress[ADDRSIZE];  /* Contains Local address of lan card.    */
    char lannumber[ADDRSIZE]; /* Contains lan number.                   */
} laninfo[MAXLANDEVS];

jmp_buf Sjbuf;          /* for error recovery                         */

struct fis fisbuf;      /* temp buffer for lan ioctl requests         */

int maxgatewaycount;    /* How many gateway connections allowed?      */
int gatewaycount;       /* Number of current gateway connections      */
int gatewaypid[MAXVTCOM];

static struct vtrequest gtwypckt; /* Used in send_gateway_info() and  */
				  /* in send_gateway_poll().          */

int nohopflag;          /* If TRUE, don't allow requests that have    */
			/* arrived via a gateway.                     */

extern char *net_aton();
extern char *strchr();

void terminate(), childdeath(), logerr(), setupreqlan(), setupreslan();
void setup_uucp(), do_poll(), do_gateway(), do_vtcon(), do_pollrelay();
void dowork(), douucon(), closeuuptys(), closelandevs(), senderr();
void send_gateway_info(), send_gateway_poll(), gateway_config_update();
void send_allgtwy_info(), send_gateway_pckt(), catch_alarm();

void
terminate(sig)
    int sig;
{
    int i;
    char termbuf[80];

    if (sig != SIGTERM) {
	sprintf(termbuf,"vtdaemon received signal #%d.\n",sig);
	logerr(termbuf,"");
    }

    for (i = 0; i < uudevcnt; i++)
	unlink(uuconfiglist[i].calldevname);

    logerr("SHUTDOWN","");
    exit(0);
}

int exceptmask[2];  /* Has to be global since childdeath modifies it */

void
childdeath()
{
    int i,j;
    int ptyfd;
    int pid;

#ifdef DEBUG
    logerr("entered childdeath","wait for any child to terminate");
#endif
    if ((pid = wait((int *)0)) != -1) {

	for (i = 0; i < uudevcnt; i++) {
	    if (uuconfiglist[i].vtpid == pid) {

		/* Re-open master pty */

#ifdef DEBUG
                logerr("childdeath: open(master,O_RDWR","uuconfiglist[i].mastername");
#endif
		if (( ptyfd = open(uuconfiglist[i].mastername,O_RDWR) ) < 0) {
                    int errnosav;
                    char errmsgbuf[40];

		    errnosav=errno;
		    logerr("Portal Disabled. Could not re-open master pty for",
			   uuconfiglist[i].calldevname);
		    sprintf(errmsgbuf,"Portal Disabled. errno=%d",errnosav);
		    logerr(errmsgbuf,"");
		    unlink(uuconfiglist[i].calldevname);
		    break;
		}

		/* Turn off termio processing on pty */

#ifdef DEBUG
                logerr("childdeath: issue TIOCTTY off","");
#endif
		j = 0;
		if (ioctl(ptyfd,TIOCTTY,&j) != 0) {
		    logerr("Portal Disabled. Could not turn off termio processing for",
			   uuconfiglist[i].calldevname);
		    unlink(uuconfiglist[i].calldevname);
		    break;
		}

		/* Turn on ioctl/open/close trapping */
#ifdef DEBUG
                logerr("childdeath: issue TIOCTRAP off","");
#endif

		j = 1;
		if (ioctl(ptyfd,TIOCTRAP,&j) != 0) {
		    logerr("Portal Disabled. Could not turn on ioctl/open/close trapping for",
			   uuconfiglist[i].calldevname);
		    unlink(uuconfiglist[i].calldevname);
		    break;
		}

		/* Change modes for slave device so that anyone can read/write */

		if (chmod(uuconfiglist[i].calldevname,0666) != 0) {
		    logerr("Portal Disabled. Could not change modes on call device",
			   uuconfiglist[i].calldevname);
		    unlink(uuconfiglist[i].calldevname);
		    break;
		}

		exceptmask[ptyfd/BITS_PER_INT] |= (1 << (ptyfd % BITS_PER_INT));
		uuconfiglist[i].ptyfd = ptyfd;
		uuconfiglist[i].vtpid = -1;
		break;
	    }
	}

	for (i = 0; i < gatewaycount; i++) {
	    if (gatewaypid[i] == pid) {
		gatewaycount--;
		gatewaypid[i] = gatewaypid[gatewaycount];
		break;
	    }
	}
    }

    /* reset signal handler */

#ifdef hp9000s200
    (void) signal(SIGCLD,  childdeath);
#else
    (void) signal(SIGCLD,  (int (*)()) childdeath);
#endif
    return;
}

void
catch_alarm()
{
    longjmp(Sjbuf, 1);
}

main(argc,argv)
    int argc;
    char *argv[];
{
    int i;
    char *s;
    char *localaddress, *nodename;
    extern char *getaddress(), *getnodename();
    long startup_time;

    /* Make sure we are root */

    if (geteuid() != 0) {
	fprintf(stderr,"Must be root to start up vtdaemon.\n");
	exit(1);
    }

    /* Fork and let parent die */

    switch (fork()) {
	case -1:
	    fprintf(stderr,"Fork failed. cannot put vtdaemon in background.\n");
	    exit(1);
	case 0:
	    break;
	default:
	    exit(0);
    }

    (void) sigsetmask(BLOCK_SIGCLD);

    /* Ignore hangups, interrupts and quits. Do a setpgrp so that */
    /* we are not associated with anything.                       */

    (void) signal(SIGHUP,  SIG_IGN);
    (void) signal(SIGINT,  SIG_IGN);
    (void) signal(SIGQUIT, SIG_IGN);

#ifdef hp9000s200
    (void) signal(SIGTERM, terminate);
    (void) signal(SIGALRM, terminate);
    (void) signal(SIGCLD,  childdeath);

#else
    (void) signal(SIGTERM, (int (*)()) terminate);
    (void) signal(SIGALRM, (int (*)()) terminate);
    (void) signal(SIGCLD,  (int (*)()) childdeath);
#endif

    (void) setpgrp();

    /* Close all file descriptors */

    for (i=getnumfds()-1;i>=0;i--)
	(void) close(i);

    /* Open /dev/null for file descriptors 0,1 & 2 */

    if ((i = open("/dev/null",O_RDONLY)) != 0)
	exit(1);
    (void) dup(i);
    (void) dup(i);

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

    /* Get node name */

    if ((nodename = getnodename()) == (char *)0) {
	logerr("Cannot get nodename","");
	exit(1);
    }
    if (strcmp(nodename,"unknown") == 0) {
	logerr("hostname not set.","");
	exit(1);
    }

    logerr("NODENAME =",nodename);

    /* Change directory to / so that we are in a known location. */
    /* (primarily for file transfer after we fork a vtserver)    */

    if (chdir("/") != 0) {
	logerr("Could not change directory to /","");
	exit(1);
    }

    /* Check arguments, save lan device names */

    maxgatewaycount = 0;
    landevcnt   = 0;
    nohopflag   = FALSE;
    while (argc > 1) {
	if (argv[1][0] == '-') {

	    switch(argv[1][1]) {

	    case 'g':

		s = &argv[1][2];
		if (*s == '\0')
		    maxgatewaycount = MAXVTCOM;
		else {
		    i = atoi(s);
		    if (i <= 0) {
			logerr("Illegal gateway count on command line","");
			exit(1);
		    }
		    if (i > MAXVTCOM)
			maxgatewaycount = MAXVTCOM;
		    else
			maxgatewaycount = i;
		}
		break;

	    case 'n':
		nohopflag = TRUE;
		break;

	    default:
		logerr("Bad Option specified on command line:",argv[1]);
		exit(1);
	    }
	}
	else {
	    if (landevcnt == MAXLANDEVS) {
		logerr("Too many lan devices specified on command line.","");
		exit(1);
	    }

	    if (strlen(argv[1]) >= MAXLANDEVLEN) {
		logerr("Lan device name too long:",argv[1]);
		exit(1);
	    }

	    laninfo[landevcnt++].landevice = argv[1];
	}

	argc--;
	argv++;
    }

    if (landevcnt == 0)
	laninfo[landevcnt++].landevice = DEFAULTLANDEV;

    if (landevcnt == 1)
	maxgatewaycount = 0; /* Can't be a gateway with only 1 lan */

    /* Open Lan devices  and set up connections on each one */

    startup_time = time(0);
    for (i = 0; i < landevcnt; i++) {

	int fd;

	/* Open Lan device */

	if ((fd = open(laninfo[i].landevice,O_RDWR)) < 0) {
	    logerr("Could not open lan device:",laninfo[i].landevice);
	    exit(1);
	}

	/* Initialize the Lan connection */

	setupreqlan(fd);

	/* Get the local address for this lan device */

	if ((localaddress = getaddress(fd)) == (char *)0)
	    exit(1);

	laninfo[i].lanfd = fd;
	laninfo[i].lantime = startup_time;
	net_aton(laninfo[i].laddress,localaddress,ADDRSIZE);
	memcpy(laninfo[i].lannumber,laninfo[i].laddress,ADDRSIZE);
    }

    /* Set up uucp call devices */

#ifndef NOSELECT
    setup_uucp();
#endif


    if (maxgatewaycount > 0)
	send_gateway_poll(nodename);

    logerr("INITIALIZATION COMPLETE","");

#ifdef NOSELECT

    /* We can only handle one lan device if we don't have */
    /* select on lan devices.                             */

    gatewaycount = 0;
    for (;;) {
	(void) dorequest(0,nodename);
    }
#else
    dowork(nodename);
#endif
}

void
dowork(nodename)
    char *nodename;
{
    int readfds[2],readmask[2],exceptfds[2]; /* exceptmask is global */
    int ret;
    int nfds;
    int i,j;
    int errorcount;
    long lastinfotime;
    long difftime;
    struct timeval infotimeout;
    int save_errno;

#ifdef DEBUG
    logerr("entered dowork","");
#endif
    infotimeout.tv_sec  = 120;
    infotimeout.tv_usec = 0;

    readmask[0] = 0;
    nfds = -1;
    for (i = 0; i < landevcnt; i++) {
	j = laninfo[i].lanfd;
	readmask[j/BITS_PER_INT] |= (1 << (j % BITS_PER_INT));
	if (j > nfds)
	    nfds = j;
    }

    for (i = 0; i < uudevcnt; i++) {
	j = uuconfiglist[i].ptyfd;
	exceptmask[j/BITS_PER_INT] |= (1 << (j % BITS_PER_INT));
	if (j > nfds)
	    nfds = j;
    }

    ++nfds;
    errorcount = 0;
    lastinfotime = time(0);
    for (;;) {

	readfds[0]   = readmask[0];
	readfds[1]   = readmask[1];
	exceptfds[0] = exceptmask[0];
	exceptfds[1] = exceptmask[1];

	if (errorcount > 10) {
	    logerr("Too many consecutive errors in dowork (SHUTDOWN)","");
	    exit(1);
	}

	/* Unblock SIGCLD during the select so that child deaths will */
	/* be processed.                                              */

#ifdef DEBUG
	logerr("dowork: doing select","");
#endif
	sigsetmask(0L);
	if (maxgatewaycount > 0)
	    ret = select(nfds,readfds,(int *)0,exceptfds,&infotimeout);
	else
	    ret = select(nfds,readfds,(int *)0,exceptfds,(struct timeval *)0);
	save_errno = errno;
	sigsetmask(BLOCK_SIGCLD);
	if (ret == -1) {
	    if (save_errno == EINTR)
		continue;
	    else {
		logerr("Select failed in dowork","");
		exit(1);
	    }
	}

#ifdef DEBUG
	logerr("dowork: select completed","");
#endif
	if (maxgatewaycount > 0) {
	    difftime = time(0) - lastinfotime;
	    if (difftime >= 120) {
		infotimeout.tv_sec  = 120;
		lastinfotime = time(0);
		send_allgtwy_info(nodename);
	    }
	    else
		infotimeout.tv_sec  = 120 - difftime;
	}

	if (readfds[0] != 0 || readfds[1] != 0) {
	    for (i = 0; i < landevcnt; i++) {
		j = laninfo[i].lanfd;
		if (readfds[j/BITS_PER_INT] & (1 << (j % BITS_PER_INT))) {
		    if ((ret = dorequest(i,nodename)) == -1)
			errorcount++;
		    else {
			if (ret > 0)
			    errorcount = 0;
		    }
		}
	    }
	}

	if (exceptfds[0] != 0 || exceptfds[1] != 0) {

	    for (i = 0; i < uudevcnt; i++) {
		j = uuconfiglist[i].ptyfd;
		if (exceptfds[j/BITS_PER_INT] & (1 << (j % BITS_PER_INT)))
		    douucon(i);
	    }
	}
    }
}

#define DIALOUT "DIALOUT"

void
douucon(index)
    int index;
{
    struct request_info reqbuf;
    int ptyfd;
    int vtpid;
    int request;
    char *nodename;
    int i;

#ifdef DEBUG
	logerr("douucon entered","");
#endif
    ptyfd = uuconfiglist[index].ptyfd;

    /* Get request */

    if (ioctl(ptyfd,TIOCREQGET,&reqbuf) != 0) {
	logerr("ioctl request get failed","");
	return;
    }

    request = reqbuf.request;
#ifdef DEBUG
	logerr("douucon: complete handshake","");
#endif

    /* complete handshake */

    if (ioctl(ptyfd,TIOCREQSET,&reqbuf) != 0) {
	logerr("ioctl request set failed","");
	return;
    }

    if (request != TIOCOPEN)
	return;

    vtpid = uuconfiglist[index].vtpid;
    nodename = uuconfiglist[index].nodename;

    /* process open request */

    if (vtpid == -1) {
#ifdef DEBUG
	logerr("douucon: doing fork","");
#endif

	if ((vtpid = fork()) == -1) {
	    logerr("Could not fork to start up connection to node",
		    nodename);
	    return;
	}

	if (vtpid != 0) {

	    /* parent -- record pid, remove fd from exceptmask, */
	    /* close pty and return.                            */

	    exceptmask[ptyfd/BITS_PER_INT] &= ~(1 << (ptyfd % BITS_PER_INT));

	    uuconfiglist[index].vtpid = vtpid;
	    close(ptyfd);
	    uuconfiglist[index].ptyfd = -1;
	    return;
	}

#ifdef DEBUG
	logerr("douucon: exec VT with uucp option","");
#endif
	/* Child -- exec the vt program with the uucp option */

	close(0);
	close(1);
	if (dup(ptyfd) != 0)
	    _exit(1);
	if (dup(ptyfd) != 1)
	    _exit(1);

	close(2);
	if (dup(fileno(logfptr)) != 2)
	    _exit(1);

	closeuuptys();
	close(fileno(logfptr));
	closelandevs(-1);

	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT,SIG_DFL);
	signal(SIGHUP, SIG_DFL);
	sigsetmask(0L);

	if (strcmp(nodename,DIALOUT) == 0)
	    execl(VTPROG,VTARG0,"-u",uuconfiglist[index].landevname,0);
	else
	    execl(VTPROG,VTARG0,nodename,"-u",
		  uuconfiglist[index].landevname,0);
	_exit(0);
    }
}

void
closeuuptys()
{
    register int i;

    for(i = 0; i < uudevcnt; i++) {
	if (uuconfiglist[i].ptyfd != -1)
	    close(uuconfiglist[i].ptyfd);
    }
}

/* Close all lan devices except the one whose file descriptor */
/* is passed in. ( -1 to close all lan devices).              */

void
closelandevs(fd)
    int fd;
{
    register int i;
    register int j;

    for (i = 0; i < landevcnt; i++) {
	j = laninfo[i].lanfd;
	if (j != -1 && j != fd)
	    close(j);
    }
}

void
setupreqlan(fd)
    int fd;
{

    fisbuf.vtype   = INTEGERTYPE;

    /* Set up IEEE802 source sap for VT requests */

    fisbuf.reqtype = LOG_SSAP;
    fisbuf.value.i = VTREQTYPE;
    if (ioctl(fd,NETCTRL,&fisbuf) != 0) {
	logerr("Cannot set IEEE802 source sap for vt requests","");
	exit(1);
    }

    /* Initialize packet cache value */

    fisbuf.reqtype = LOG_READ_CACHE;
    fisbuf.value.i = CACHE_PACKETS;
    if (ioctl(fd,NETCTRL,&fisbuf) != 0) {
	logerr("Out of lan memory","");
	exit(1);
    }

    /* Add VTMULTICAST address to system multicast address list   */
    /* First we delete it (and don't bother checking the error).  */
    /* This is done because there is no way of differentiating    */
    /* whether an addition failed because the address was already */
    /* in the system multicast address list or because of some    */
    /* other error.                                               */

    fisbuf.vtype   = ADDRSIZE;
    net_aton(fisbuf.value.s,VTMULTICAST,ADDRSIZE);
    fisbuf.reqtype = DELETE_MULTICAST;
    ioctl(fd,NETCTRL,&fisbuf);
    fisbuf.reqtype = ADD_MULTICAST;
    if (ioctl(fd,NETCTRL,&fisbuf) != 0) {
	logerr("Cannot add VTMULTICAST address to system multicast address list","");
	exit(1);
    }
}

dorequest(lanindex,nodename)
    int lanindex;
    char *nodename;
{
    static struct vtrequest vp;
    char to[ADDRSIZE];
    short dsap;
    short reqtype;
    int  lan_fd;
    int  pid;
    int  dogateway;
    int  hopcount;
    int  bounceback;
    int  retval;
    char *s;

#ifdef DEBUG
    logerr("entered dorequest","");
#endif
    lan_fd = laninfo[lanindex].lanfd;

#ifdef DEBUG
    logerr("dorequest: getpacket","");
#endif
    retval = getpacket(lan_fd,&vp);
    if (retval <= 0)
	return(retval);

    hopcount = vp.hopcount++;

    s = vp.hoptrace + hopcount * ADDRSIZE;
    memcpy(s,laninfo[lanindex].lannumber,ADDRSIZE);

    reqtype = vp.reqtype;

    /* Ignore request if hopcount is greater than zero and */
    /* nohopflag is TRUE.                                  */

    if (hopcount > 0 && nohopflag == TRUE)
	return(retval);

    bounceback = FALSE;
    if (memcmp(vp.fromaddr,laninfo[lanindex].laddress,ADDRSIZE) == 0 )
	bounceback = TRUE;

    if (reqtype == VTR_GTWYPOLL || reqtype == VTR_GTWYINFO) {

	if (maxgatewaycount == 0 || bounceback == TRUE)
	    return(retval);

	if (reqtype == VTR_GTWYPOLL)
	    send_gateway_info(lanindex,nodename);
	else
	    gateway_config_update(lanindex,nodename,&vp);

	return(retval);
    }

    /* Ignore request if we just sent it to ourselves and either: */
    /*     1) Request type is VTR_VTPOLL and the hopcount is      */
    /*        greater than zero.                                  */
    /*     2) Request type is not VTR_VTPOLL and the request is   */
    /*        not for us.                                         */

    if (bounceback == TRUE) {
	if (reqtype != VTR_POLL) {
	    if (strcmp(nodename,vp.wantnode) != 0)
		return(retval);
	}
	else {
	    if (hopcount != 0)
		return(retval);
	}
    }

    dogateway = FALSE;

    if (reqtype != VTR_POLL && strcmp(nodename,vp.wantnode) != 0) {

	if (gatewaycount >= maxgatewaycount
	    || strcmp(nodename,vp.mynode) == 0
	    || strcmp(vp.mynode,vp.wantnode) == 0
	    || checktrace(lanindex,hopcount,vp.hoptrace) == FALSE)
	    return(retval);

	dogateway = TRUE;
    }

    if (reqtype == VTR_POLL
	&& gatewaycount < maxgatewaycount
	&& bounceback == FALSE
	&& checktrace(lanindex,hopcount,vp.hoptrace) == TRUE)
	dogateway = TRUE;

    /* Copy requesters address */

    memcpy(to,vp.fromaddr,ADDRSIZE);
    dsap = vp.sap;

    /* Check vt protocol version number */

    if (vp.majorversion != VT_MAJOR_VERSION) {

	if (dogateway == TRUE)
	    return(retval);

	if (vp.majorversion > VT_MAJOR_VERSION) {
	    logerr("VT Protocol version error. Please update!","");
	    senderr(lan_fd,nodename,to,dsap,E_OLDSERR);
	}
	else
	    senderr(lan_fd,nodename,to,dsap,E_OLDRERR);
	return(retval);
    }

    /* Make sure request type is in range if we are going to */
    /* service it (i.e. we are not just an intermediate      */
    /* gateway).                                             */

    if (dogateway == FALSE && (reqtype <= 0 || reqtype > VTR_MAX) ) {
	logerr("Unsupported request type received.","");
	senderr(lan_fd,nodename,to,dsap,E_SUPRTERR);
	return(retval);
    }

    /* Respond to a poll request here if we are not a gateway. If we */
    /* are a gateway we will wait until we have forked before we     */
    /* respond to the poll request.                                  */

    if (reqtype == VTR_POLL && dogateway == FALSE) {
	do_poll(lanindex,nodename,to,dsap);
	return(retval);
    }

    /* fork and let parent continue on. child will set up connection */
    /* and then exec the appropriate server.                         */
#ifdef DEBUG
    logerr("dorequest: doing fork","");
#endif

    switch (pid = fork()) {
	case -1:
	    logerr("Fork failure","");
	    if (dogateway == FALSE)
		senderr(lan_fd,nodename,to,dsap,E_NOFORK);
	    return(retval);
	case 0:
	    break;
	default:
	    if (dogateway == TRUE) {
		gatewaypid[gatewaycount++] = pid;
		if (reqtype == VTR_POLL)
		    do_poll(lanindex,nodename,to,dsap);
	    }
	    return(retval);
    }
#ifdef DEBUG
    sleep(2);  /*  give parent's debug info a chance to flush */
    logerr("child dorequest: entered","");
#endif

    /* Close other lan devices and uucp master ptys */

    closeuuptys();
    closelandevs(lan_fd);

    /* Start up a gateway server if we are just an intermediate stop */

    if (dogateway == TRUE) {
	do_gateway(lanindex,&vp,to,dsap);
	_exit(0);
    }

    /* Call the appropriate procedure for the request */

    switch (reqtype) {

    case VTR_VTCON:
#ifdef DEBUG
	{
	   char errmsgbuf[80];
	   sprintf(errmsgbuf,"mynode=%x (%d) mylogin=%s flags=%x (%d)",
		   vp.mynode,vp.mynode,vp.mylogin,vp.flags,vp.flags);
           logerr(errmsgbuf,"");
	}
#endif
	do_vtcon(lanindex,vp.mynode,vp.mylogin,vp.flags,to,dsap);
	break;

    case VTR_POLL:      /* should not get here */
	break;

    case VTR_GTWYPOLL:  /* should not get here */
	break;

    case VTR_GTWYINFO:  /* should not get here */
	break;
    }

    /* Should not get here! */

    _exit(0);
}

/* checktrace returns TRUE if it is ok to forward this packet */

checktrace(lanindex,hopcount,trace)
    int lanindex;
    int hopcount;
    char *trace;
{
    int i,j;

    if (hopcount >= MAXHOP)
	return (FALSE);

    for (i = 0; i < hopcount; i++, trace += ADDRSIZE) {
	for (j = 0; j < landevcnt; j++) {

	    if (j == lanindex)
		continue;

	    if (memcmp(trace,laninfo[j].lannumber,ADDRSIZE) == 0)
		return(FALSE);
	}
    }
    return(TRUE);
}

#define NTRIES 2

void
do_poll(lanindex,nodename,to,dsap)
    int lanindex;
    char *nodename;
    char *to;
    short dsap;
{
    char mysap[6];
    struct vtpacket   pckt;
    struct vtresponse vtr;
    struct timeval dtime;
    int lan_fd;
    long i;

    lan_fd = laninfo[lanindex].lanfd;

    memcpy(vtr.address,laninfo[lanindex].laddress,ADDRSIZE);
    strcpy(vtr.nodename,nodename);

    if (maxgatewaycount > 0)
	vtr.gatewayflag = TRUE;
    else
	vtr.gatewayflag = FALSE;
    vtr.hopcount = 0;
    vtr.minorversion = VT_MINOR_VERSION;
    vtr.sap = 0;
    vtr.error = 0;

    /* Compute a delay time based on lan card address */

    memcpy((char *)&i,&vtr.address[2],sizeof (long));
    i = (i & 0x7fffffff) % 150;

    dtime.tv_sec = 0;
    dtime.tv_usec = i * 1000;

    /* Send the response packet */

    for (i = 0; i < NTRIES; i++) {

	select(0,(int *)0,(int *)0,(int *)0,&dtime);

	if (sendresponse(VTPOLLRESPONSE,lan_fd,to,dsap,&vtr) != 0) {
	    logerr("Error in sending response in do_poll","");
	    return;
	}
    }
    return;
}

void
do_vtcon(lanindex,mynode,mylogin,flags,to,dsap)
    int  lanindex;
    char *mynode;
    char *mylogin;
    int  flags;
    char *to;
    short dsap;
{
    char mysap[6];
    int lan_fd;
    int ptyfd;
    int i;
#ifdef DEBUG
    logerr("entered do_vtcon","");
#endif

    lan_fd = laninfo[lanindex].lanfd;

    /* Handle a vt connection request */

    /* Do file descriptor manipulation. When finished they should be: */
    /*        0) lan connection                                       */
    /*        1) master pty                                           */
    /*        2) log file                                             */

    /* close 0,1 & 2 */

    close(0);
    close(1);
    close(2);

    /* Open up a lan connection for vt communications on file */
    /* descriptor zero. Return the allocated sap converted to */
    /* a string in mysap[].                                   */

    setupreslan(FALSE,lanindex,(char *)0,VTCOMTYPE,dsap,to,mysap);

    /* Allocate a login pty */

    if (setjmp(Sjbuf)) {
	senderr(lan_fd,(char *)0,to,dsap,E_DEMONERR);
	_exit(0);
    }

#ifdef hp9000s200
    (void) signal(SIGALRM, catch_alarm);

#else
    (void) signal(SIGALRM, (int (*)()) catch_alarm);
#endif

    alarm(15);

    ptyfd = getpty(LOGIN_PROGRAM,(char **)0,(char **)0,(char *)0,
			(char *)0,flags,(char *)0,(char *)0,&i);

    alarm(0);

#ifdef hp9000s200
    (void) signal(SIGALRM, terminate);

#else
    (void) signal(SIGALRM, (int (*)()) terminate);
#endif

    if (ptyfd < 0) {
	senderr(lan_fd,(char *)0,to,dsap,i);
	_exit(0);
    }
    else {
	if (ptyfd != 1) {
	    senderr(lan_fd,(char *)0,to,dsap,E_FDERR);
	    _exit(0);
	}
    }

#ifdef hp9000s500
    ioctl(ptyfd,0x80002601,0);
#endif

    if (flags & NOTTY) {

	/* disable termio processing on pty          */

#ifdef DEBUG
    logerr("do_vtcon: issue TIOCTTY off","");
#endif
	i = 0;
	if (ioctl(ptyfd,TIOCTTY,&i) != 0) {
	    senderr(lan_fd,(char *)0,to,dsap,E_IOCTLERR);
	    _exit(0);
	}
    }
    else {

	/* Enable read only trapping of termio ioctl requests */
#ifdef DEBUG
        logerr("do_vtcon: issue TIOCMONITOR on","");
#endif

	i = 1;
	if (ioctl(ptyfd,TIOCMONITOR,&i) != 0) {
	    senderr(lan_fd,(char *)0,to,dsap,E_IOCTLERR);
	    _exit(0);
	}
    }

#ifdef DEBUG
        logerr("do_vtcon: issue TIOCSIGMODE,TIOCSIGBLOCK","");
#endif
    if (ioctl(ptyfd,TIOCSIGMODE,TIOCSIGBLOCK) != 0) {
	senderr(lan_fd,(char *)0,to,dsap,E_IOCTLERR);
	_exit(0);
    }

    /* Set O_NDELAY on the pty */

    if (setndelay(ptyfd) != 0) {
	senderr(lan_fd,(char *)0,to,dsap,E_FCNTLERR);
	_exit(0);
    }

    /* dup the log file fd to 2 */

    if (dup(fileno(logfptr)) != 2) {
	senderr(lan_fd,(char *)0,to,dsap,E_FDERR);
	_exit(0);
    }

    /* Set the close on exec flag on all other open file descriptors */

    if (fcntl(fileno(logfptr),F_SETFD,CLOSE_ON_EXEC) == -1) {
	senderr(lan_fd,(char *)0,to,dsap,E_FCNTLERR);
	_exit(0);
    }
    if (fcntl(lan_fd,F_SETFD,CLOSE_ON_EXEC) == -1) {
	senderr(lan_fd,(char *)0,to,dsap,E_FCNTLERR);
	_exit(0);
    }

    /* exec the vtserver */

#ifdef DEBUG
    logerr("do_vtcon: exec vtserver","");
#endif
    execl(VTSERVERPROG,VTSERVERARG0,mynode,mylogin,mysap,0);
    senderr(lan_fd,(char *)0,to,dsap,E_EXECERR);
    _exit(0);
}

void
do_gateway(lanindex,vpptr,to,dsap)
    int  lanindex;
    struct vtrequest *vpptr;
    char *to;
    short dsap;
{
    char mysap[6];
    struct vtresponse vtrs;
    struct vtpacket pckt;
    char wantnodename[NODENAMESIZE];
    char multito[ADDRSIZE];
    int lan_fd;
    struct timeval timeout;
    int i,j;
    int len;
    int tmpfd,maxfd,fd;
    int tmpsap;
    int readfds;

    lan_fd = laninfo[lanindex].lanfd;

    /* Set up incoming lan connection (from requester) on file descriptor 0 */

    close(0);
    setupreslan(TRUE,lanindex,(char *)0,VTCOMTYPE,dsap,to,mysap);

    /* Initialize a vt packet */

    pckt.dlen   = sizeof (struct vtrequest);
    pckt.alen   = 0;
    pckt.type   = VTREQUEST;
    pckt.seqno  = 0;
    pckt.rexmit = 0;

    /* Create new outgoing lan connections (one per known lan device) */

    readfds = 0;
    maxfd = 0;
    len = FIXEDSIZE + sizeof (struct vtrequest); /* length of packet */
    net_aton(multito,VTMULTICAST,ADDRSIZE);
    for (i = 0; i < landevcnt; i++) {

	laninfo[i].lanfd = -1;

	/* Don't send request back out onto same lan */

	if (i == lanindex)
	    continue;

	if ((fd = open(laninfo[i].landevice, (O_RDWR | O_NDELAY) ) ) >= 0) {

	    /* Allocate a sap for this connection */

	    tmpsap = -1;
	    fisbuf.vtype   = INTEGERTYPE;
	    fisbuf.reqtype = LOG_SSAP;
	    for (j=VTCOMTYPE; j < (VTCOMTYPE + 4 * MAXVTCOM); j += 4) {
		fisbuf.value.i = j;
		if (ioctl(fd,NETCTRL,&fisbuf) == 0) {

		    fisbuf.reqtype = LOG_READ_CACHE;
		    fisbuf.value.i = CACHE_PACKETS;
		    if (ioctl(fd,NETCTRL,&fisbuf) == 0 &&
			setaddress(fd,VTREQTYPE,multito) == 0)
			tmpsap = j;

		    break;
		}
	    }

	    if (tmpsap != -1) {


		/* Send the request packet */

		memcpy(vpptr->fromaddr,laninfo[i].laddress,ADDRSIZE);
		vpptr->sap = tmpsap;
		memcpy(pckt.data,(char *)vpptr,sizeof (struct vtrequest));
		if (write(fd,(char *)&pckt,len) == len ) {
		    readfds |= (1 << fd);
		    laninfo[i].lanfd = fd;
		    if (fd > maxfd)
			maxfd = fd;
		}
		else
		    close(fd);
	    }
	    else
		close(fd);
	}
    }

    /* If we didn't send a request packet then just terminate (original */
    /* requester will just timeout).                                    */

    if (readfds == 0)
	_exit(0);

    if (vpptr->reqtype == VTR_POLL) {
	do_pollrelay(maxfd+1,RESFD,readfds);
	_exit(0);
    }

    /* Wait for timeout or first response */

    timeout.tv_sec  = 60;
    timeout.tv_usec = 0;

    if (select(maxfd+1,&readfds, (int *)0, (int *)0,&timeout) == -1)
	_exit(0);

    /* If we didn't get a response then we just exit and let the */
    /* original requester timeout.                               */

    if (readfds == 0)
	_exit(0);

    /* Find out which lan we got a response on */

    fd = -1;
    for (i = 0; i < landevcnt; i++) {
	if ((tmpfd = laninfo[i].lanfd) != -1) {
	    if (readfds & (1 << tmpfd)) {
		fd = tmpfd;
		break;
	    }
	}
    }

    /* Once again, just let the original requester timeout if couldn't */
    /* find the responding lan (Actually, this should never happen).   */

    if (fd == -1)
	_exit(0);

    /* Close all other lan devices */

    closelandevs(fd);

    /* Read the response */

    len = FIXEDSIZE + sizeof (struct vtresponse);
    if (read(fd,(char *)&pckt,MAXPACKSIZE) != len)
	_exit(0);

    if (pckt.type != VTRESPONSE)
	_exit(0);

    memcpy((char *)&vtrs,pckt.data,sizeof (struct vtresponse));

    /* Check to see if this is an error packet */

    if (vtrs.error != 0)
	_exit(0);

    /* Log destination address for outgoing lan connection */

    if (setaddress(fd,vtrs.sap,vtrs.address) != 0)
	_exit(0);

    /* Ok, we are ready to call the vtgateway server. Do file */
    /* descriptor manipulation so that:                       */
    /*                                                        */
    /*      0)  incoming lan connection (nearest requester)   */
    /*      1)  outgoing lan connection (nearest server)      */
    /*      2)  error log file.                               */

    close(1);
    close(2);

    if (dup(fd) != 1)
	_exit(0);

    close(fd);

    /* dup the log file fd to 2 */

    if (dup(fileno(logfptr)) != 2)
	_exit(0);

    /* Set the close on exec flag on all other open file descriptors */

    if (fcntl(fileno(logfptr),F_SETFD,CLOSE_ON_EXEC) == -1)
	_exit(0);

    if (fcntl(lan_fd,F_SETFD,CLOSE_ON_EXEC) == -1)
	_exit(0);

    /* exec the vtgateway server */

    execl(VTGATEWAYPROG,VTGATEWAYARG0,vpptr->mynode,vpptr->mylogin,
	 "to",vpptr->wantnode,mysap,0);
    _exit(0);
}

void
do_pollrelay(nfds,responsefd,readmask)
    int nfds;
    int responsefd;
    int readmask;
{
    struct timeval timeout;
    struct vtpacket pckt;
    struct vtresponse vtrs;
    int readfds;
    int tmpfd,fd;
    int nread;
    int i;

    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    do {

	readfds = readmask;

	if (select(nfds,&readfds, (int *)0, (int *)0,&timeout) == -1)
	    _exit(0);

	/* Find out which lan we got a response on */

	fd = -1;
	for (i = 0; i < landevcnt; i++) {
	    if ((tmpfd = laninfo[i].lanfd) != -1) {
		if (readfds & (1 << tmpfd)) {
		    fd = tmpfd;
		    break;
		}
	    }
	}

	if (fd != -1) {
	    do {
		if ((nread = read(fd,(char *)&pckt,MAXPACKSIZE)) > 0 ) {
		    memcpy((char *)&vtrs,pckt.data,sizeof (struct vtresponse));
		    vtrs.hopcount++;
		    memcpy(pckt.data,(char *)&vtrs,sizeof (struct vtresponse));
		    write(responsefd,(char *)&pckt,nread);
		}
	    } while (nread > 0);
	}
    } while (readfds != 0);
}

#define MAXLINE 1024

void
setup_uucp()
{
    FILE *fptr;
    char linebuf[MAXLINE];
    char slavename[PTYNAMLEN];
    char *calldev,*nodename,*landevname;
    int ptyfd;
    int i,error;
#ifdef DEBUG
    logerr("entered setup_uucp","");
#endif

    if ((fptr = fopen(UUCONFIGFILE,"r")) == (FILE *)0) {
	return;
    }

    while (getline(linebuf,MAXLINE,fptr) != 0) {

	if (uudevcnt == MAXUUCPDEVS) {
		logerr("Maximum number of portals reached","");
		break;
	}

	calldev    = uuconfiglist[uudevcnt].calldevname;
	nodename   = uuconfiglist[uudevcnt].nodename;
	landevname = uuconfiglist[uudevcnt].landevname;
	getargs(linebuf,calldev,nodename,landevname);

	/* Allocate a pty pair */

	i = 0;
	error = 0;
	do {
	    ptyfd = getpty("",(char **)0,(char **)0,(char *)0,(char *)0,
			   0,slavename,
			   uuconfiglist[uudevcnt].mastername,&error);
	    if (ptyfd < 0) {
		if (i > 1 || error != E_DEMONERR)
		    logerr("Could not allocate pty. ERROR:",pty_errlist[error]);
		else
		    sleep(20);
	    }
	} while(i++ < 2 && error == E_DEMONERR);

	if (ptyfd < 0)
	    break;

	unlink(calldev);


	/* Turn off termio processing on pty */

#ifdef DEBUG
    logerr("setup_uucp: issue TIOCTTY off","");
#endif
	i = 0;
	if (ioctl(ptyfd,TIOCTTY,&i) != 0) {
	    logerr("Could not turn off termio processing for",calldev);
	    break;
	}

	/* Turn on ioctl/open/close trapping */

#ifdef DEBUG
    logerr("setup_uucp: issue TIOTRAP on","");
#endif
	i = 1;
	if (ioctl(ptyfd,TIOCTRAP,&i) != 0) {
	    logerr("Could not turn on ioctl/open/close trapping for",calldev);
	    break;
	}

	/* Change modes for slave device so that anyone can read/write */

	if (chmod(slavename,0666) != 0) {
	    logerr("Could not change modes on call device",calldev);
	    break;
	}

	/* Attempt to link slave device to call device */

	if (link(slavename,calldev) != 0) {
	    logerr("Could not create call device:",calldev);
	    break;
	}

	if (strcmp(nodename,DIALOUT) == 0)
	    sprintf(linebuf,"Set up dialout portal %s via %s",
			    calldev,landevname);
	else
	    sprintf(linebuf,"Set up portal %s to %s via %s",
			    calldev,nodename,landevname);
	logerr(linebuf,"");

	/* Store the file descriptor and increment uudevcnt */

	uuconfiglist[uudevcnt].vtpid   = -1;
	uuconfiglist[uudevcnt++].ptyfd = ptyfd;
    }

    fclose(fptr);
    return;
}

getline(buf,n,stream)
    char *buf;
    int n;
    FILE *stream;
{
    char *s;
    int slen;

    do {
	if (fgets(buf,n,stream) == (char *)0)
	    return(0);

	/* strip off comments or trailing newline */

	if ((s = strchr(buf,'#')) != (char *)0 )
	    *s = '\0';
	else {
	    if ((s = strchr(buf,'\n')) != (char *)0 )
		*s = '\0';
	    else {
		logerr("Line too long in vt device configuration file:",
			UUCONFIGFILE);
		buf[0] ='\0';
	    }
	};

	/* strip off leading white space */

	s = buf;
	while (*s == ' ' || *s == '\t')
	    s++;
	if (s != buf)
	    strcpy(buf,s); /* left overlapping copy documented to work */
	slen = strlen(buf);
    } while (slen == 0);
    return(slen);
}

#define CALLDEVDIR "/dev/"

getargs(buf,calldev,nodename,landevname)
    char *buf;
    char *calldev;
    char *nodename;
    char *landevname;
{
    register char *s1,*s2;
    int i;

    s1 = buf;
    if (*s1 == '/') {
	logerr("leading slash not allowed in call device name","");
	return(-1);
    }

    while (*s1 != '\0') {
	if (*s1 == ' ' || *s1 == '\t') {
	    *s1++ = '\0';
	    break;
	}
	++s1;
    }

    if ((s2 = strchr(buf,',')) == (char *)0) {
	strcpy(landevname,laninfo[0].landevice);
    }
    else {

	*s2++ = '\0';
	if (*s2 == '\0') {
	    logerr("Zero length lan device specified in L-vtdevices.","");
	    return(-1);
	}

	if (strlen(s2) >= MAXLANDEVLEN) {
	    logerr("lan device name length too large in L-vtdevices.","");
	    return(-1);
	}

	for (i = 0; i < landevcnt; i++) {
	    if (strcmp(s2,laninfo[i].landevice) == 0)
		break;
	}

	if (i == landevcnt) {
	    logerr("lan device specified in L-vtdevices is not specified on command line.","");
	    return(-1);
	}

	strcpy(landevname,s2);
    }

    if (strlen(buf) > (PTYNAMLEN - sizeof(CALLDEVDIR))) {
	logerr("Call device name too long:",buf);
	return(-1);
    }
    strcpy(calldev,CALLDEVDIR);
    strcat(calldev,buf);

    while (*s1 == ' ' || *s1 == '\t')
	++s1;

    if (*s1 == '\0') {
	logerr("No nodename specified for call device:",calldev);
	return(-1);
    }

    s2  = s1;
    while (*s1 != '\0') {
	if (*s1 == ' ' || *s1 == '\t') {
	    *s1++ = '\0';
	    break;
	}
	++s1;
    }

    if (strlen(s2) > (NODENAMESIZE - 1)) {
	logerr("Node name too long:",s2);
	return(-1);
    }
    strcpy(nodename,s2);
    return(0);
}

void
setupreslan(gtwyflag,lanindex,nodename,saptype,dsap,to,sapstr)
    int gtwyflag;
    int lanindex;
    char *nodename;
    short saptype;
    short dsap;
    char *to;
    char *sapstr;
{
    int i;
    int lan_fd;

    lan_fd = laninfo[lanindex].lanfd;

    if (open(laninfo[lanindex].landevice,(O_RDWR | O_NDELAY)) != RESFD) {
	if (gtwyflag == FALSE)
	    senderr(lan_fd,nodename,to,dsap,E_OPENERR);
	_exit(0);
    }

    fisbuf.vtype   = INTEGERTYPE;
    fisbuf.reqtype = LOG_SSAP;

    *sapstr = '\0';
    for (i=VTCOMTYPE; i < (VTCOMTYPE + 4 * MAXVTCOM); i += 4) {
	fisbuf.value.i = i;
	if (ioctl(RESFD,NETCTRL,&fisbuf) == 0) {
	    sprintf(sapstr,"%d",i);
	    break;
	}
    }

    if (*sapstr == '\0') {
	if (gtwyflag == FALSE) {
	    if (errno == ENOMEM)
		senderr(lan_fd,nodename,to,dsap,E_NOMEMERR);
	    else
		senderr(lan_fd,nodename,to,dsap,E_NOSAPERR);
	}
	_exit(0);
    }

    if (i == dsap && memcmp(laninfo[lanindex].laddress,to,ADDRSIZE) == 0) {

	/* We are talking to ourselves!! (This can happen if the */
	/* requester is on the same machine and dies before we   */
	/* allocate a SSAP). If we didn't make this check we     */
	/* would sit here talking to ourselves forever complain- */
	/* about the bad packets we are sending ourselves.       */

	logerr("I caught myself about to talk to myself!","");
	_exit(0);
    }

    /* Initialize packet cache value */

    fisbuf.reqtype = LOG_READ_CACHE;
    fisbuf.value.i = CACHE_PACKETS;
    if (ioctl(RESFD,NETCTRL,&fisbuf) != 0) {
	if (gtwyflag == FALSE)
	    senderr(lan_fd,nodename,to,dsap,E_NOMEMERR);
	_exit(0);
    }

    /* Log the destination sap */

    fisbuf.reqtype = LOG_DSAP;
    fisbuf.value.i = dsap;
    if (ioctl(RESFD,NETCTRL,&fisbuf) != 0) {
	if (gtwyflag == FALSE)
	    senderr(lan_fd,nodename,to,dsap,E_SAPERR);
	_exit(0);
    }

    /* Log the destination address */

    fisbuf.reqtype = LOG_DEST_ADDR;
    fisbuf.vtype   = ADDRSIZE;
    memcpy(fisbuf.value.s,to,ADDRSIZE);

    if (ioctl(RESFD,NETCTRL,&fisbuf) != 0) {
	if (gtwyflag == FALSE)
	    senderr(lan_fd,nodename,to,dsap,E_ADDRERR);
	_exit(0);
    }
}

/* Returns -1) Error 0) Ok, but no packet 1) Got a packet */

getpacket(fd,vpptr)
    int fd;
    struct vtrequest *vpptr;
{
    int ret;
    static struct vtpacket  pckt;

    if ((ret = read(fd,(char *)&pckt,MAXPACKSIZE)) < 0) {

#ifdef EWOULDBLOCK
	if (errno == EWOULDBLOCK)
		return(0);
#endif

	if (errno != EINTR) {
	    logerr("Bad read in getpacket","");
	    return(-1);
	}
	else
	    return(0);
    }

    if (ret < FIXEDSIZE || ret != (FIXEDSIZE + pckt.dlen + pckt.alen)) {
	logerr("Partial packet read","");
	return(-1);
    }

    if (pckt.type != VTREQUEST)
	return(0);

    memcpy((char *)vpptr,pckt.data,sizeof (struct vtrequest));
    return(1);
}

void
senderr(fd,nodename,to,dsap,err)
    int  fd;
    char *nodename;
    char *to;
    short dsap;
    int err;
{
    struct vtresponse vtr;

    vtr.error = err;
    if (nodename != (char *)0)
	strcpy(vtr.nodename,nodename);
    else
	vtr.nodename[0] = '\0';

    sendresponse(VTRESPONSE,fd,to,dsap,&vtr);
    return;
}

sendresponse(responsetype,fd,to,dsap,vtrptr)
    int  responsetype;
    int  fd;
    char *to;
    short dsap;
    struct vtresponse *vtrptr;
{
    int len;
    struct vtpacket pckt;

    /* Log the destination sap */

    fisbuf.vtype   = INTEGERTYPE;
    fisbuf.reqtype = LOG_DSAP;
    fisbuf.value.i = dsap;
    if (ioctl(fd,NETCTRL,&fisbuf) != 0) {
	logerr("Could not log destination sap in sendresponse","");
	return(-1);
    }

    /* log destination address */

    fisbuf.reqtype = LOG_DEST_ADDR;
    fisbuf.vtype   = ADDRSIZE;
    memcpy(fisbuf.value.s,to,ADDRSIZE);

    if (ioctl(fd,NETCTRL,&fisbuf) != 0) {
	logerr("Could not log destination address in sendresponse","");
	return(-1);
    }

    pckt.dlen   = sizeof (struct vtresponse);
    pckt.alen   = 0;
    pckt.rexmit = 0;
    pckt.type   = responsetype;
    pckt.seqno  = 0;
    memcpy(pckt.data,(char *)vtrptr,sizeof (struct vtresponse));

    len = FIXEDSIZE + pckt.dlen + pckt.alen;
    if (write(fd,(char *)&pckt,len) != len) {
	logerr("Error in writing packet in sendresponse","");
	return(-1);
    }
    return(0);
}

void
gateway_config_update(lanindex,nodename,vpptr)
    int lanindex;
    char *nodename;
    struct vtrequest *vpptr;
{
    int i;
    static struct vtgatewayinfo vtginfo;

    memcpy((char *)&vtginfo,vpptr->argstr1,sizeof (struct vtgatewayinfo));

    i = memcmp(vtginfo.lannumber,laninfo[lanindex].lannumber,ADDRSIZE);
    if (i != 0) {
	if (vtginfo.lantime < laninfo[lanindex].lantime
	    || (vtginfo.lantime == laninfo[lanindex].lantime && i < 0)) {

	    /* Other node is correct, so update our information */

	    laninfo[lanindex].lantime = vtginfo.lantime;
	    memcpy(laninfo[lanindex].lannumber,vtginfo.lannumber,ADDRSIZE);
	}
	else {

	    /* We are correct. send an info packet in order to sync */
	    /* the other node.                                      */

	    send_gateway_info(lanindex,nodename);
	}
    }
}

void
send_allgtwy_info(nodename)
    char *nodename;
{
    int i;

#ifdef DEBUG
    logerr("entered send_allgtwy_info","");
#endif
    for (i = 0; i < landevcnt; i++)
	send_gateway_info(i,nodename);
}

void
send_gateway_info(lanindex,nodename)
    int lanindex;
    char *nodename;
{
    static struct vtgatewayinfo vtginfo;

#ifdef DEBUG
    logerr("entered send_gateway_info","");
#endif
    gtwypckt.hopcount     = 0;
    gtwypckt.flags        = 0;
    gtwypckt.reqtype      = VTR_GTWYINFO;
    gtwypckt.majorversion = VT_MAJOR_VERSION;
    gtwypckt.sap          = VTREQTYPE;
    gtwypckt.wantnode[0]  = '\0';
    gtwypckt.minorversion = VT_MINOR_VERSION;
    strcpy(gtwypckt.mynode,nodename);

    vtginfo.lantime = laninfo[lanindex].lantime;
    memcpy(vtginfo.lannumber,laninfo[lanindex].lannumber,ADDRSIZE);

    memcpy(gtwypckt.argstr1,(char *)&vtginfo,sizeof (struct vtgatewayinfo));
    memcpy(gtwypckt.fromaddr,laninfo[lanindex].laddress,ADDRSIZE);

    send_gateway_pckt(laninfo[lanindex].lanfd);
}

void
send_gateway_poll(nodename)
    char *nodename;
{
    int i;
#ifdef DEBUG
    logerr("entered send_gateway_poll","");
#endif

    gtwypckt.hopcount     = 0;
    gtwypckt.flags        = 0;
    gtwypckt.reqtype      = VTR_GTWYPOLL;
    gtwypckt.majorversion = VT_MAJOR_VERSION;
    gtwypckt.sap          = VTREQTYPE;
    gtwypckt.wantnode[0]  = '\0';
    gtwypckt.minorversion = VT_MINOR_VERSION;
    strcpy(gtwypckt.mynode,nodename);

    for (i = 0; i < landevcnt; i++) {
	memcpy(gtwypckt.fromaddr,laninfo[i].laddress,ADDRSIZE);
	send_gateway_pckt(laninfo[i].lanfd);
    }
}

void
send_gateway_pckt(fd)
    int fd;
{
    int len;
    static struct vtpacket pckt;

#ifdef DEBUG
    logerr("entered send_gateway_pckt","");
#endif
    /* Log the destination sap */

    fisbuf.vtype   = INTEGERTYPE;
    fisbuf.reqtype = LOG_DSAP;
    fisbuf.value.i = VTREQTYPE;
    if (ioctl(fd,NETCTRL,&fisbuf) != 0) {
	logerr("Could not log destination sap in send_gateway_pckt","");
	return;
    }

    /* log destination address */

    fisbuf.reqtype = LOG_DEST_ADDR;
    fisbuf.vtype   = ADDRSIZE;
    net_aton(fisbuf.value.s,VTMULTICAST,ADDRSIZE);

    if (ioctl(fd,NETCTRL,&fisbuf) != 0) {
	logerr("Could not log destination address in send_gateway_pckt","");
	return;
    }

    pckt.dlen   = sizeof (struct vtrequest);
    pckt.alen   = 0;
    pckt.rexmit = 0;
    pckt.type   = VTREQUEST;
    pckt.seqno  = 0;
    memcpy(pckt.data,(char *)&gtwypckt,sizeof (struct vtrequest));

    len = FIXEDSIZE + pckt.dlen + pckt.alen;
    if (write(fd,(char *)&pckt,len) != len)
	logerr("Error in writing packet in send_gateway_pckt","");

    return;
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
    return;
}
