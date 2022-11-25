/* @(#) $Revision: 70.1 $ */   
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <sys/signal.h>
#include <errno.h>
#include <sys/ptyio.h>
#ifdef SIGTSTP
#include <bsdtty.h>
#endif /*SIGTSTP*/
#include "ptyrequest.h"

extern struct termio tv0,tv1,tv2,tv3; /* Space declared in vtcommand.c */
#ifdef SIGTSTP
extern struct ltchars lt0, lt1;
#endif /*SIGTSTP*/

/* LOCAL GLOBALS */

static int lanfd;            /* contains file descriptor of lan device      */
static int uucpflag = FALSE; /* Are we talking to a master pty? (For UUCP)  */
static int reqtype  = -1;    /* vt request type.                            */

/* EXTERNAL GLOBALS */

extern int waitforclose;     /* Should we wait for TIOCCLOSE in vtexit()?   */
extern int conflag;          /* Has connection been made? (used in vtexit())*/
extern int breakflag;        /* Has break been hit?                         */
extern int escapeflag;       /* Has the escape character been hit ?         */
extern int inmask,saveinmask;
extern int doiomode;
extern int childpid;

struct vtpoll {
    char nodename[NODENAMESIZE];
    short gatewayflag;
    short hopcount;
};

void logerr(), usage(), do_poll(), intr(), vtquit(), vtexit(), child_death();
void ioctlwait(),alarm_intr();
extern void init_proto(), doio(), escape(), printescape();


#define NODE_PROMPT "Nodename? "
#define RETURN_MESSAGE "\r[Return to remote]\n"

void
alarm_intr()
{
    ioctlwait(TIOCCLOSE);
    exit(1);
}

void
child_death()
{
    struct sigvec vec;

    /* Return from shell escape */

    /* Wait for child */

    if (wait((int *)0) != childpid) {
	childpid = -1; /* Because vtexit calls childwait() */
	vtexit();
    }
    childpid = -1;

    /* Send Return message */

    write(1,RETURN_MESSAGE,strlen(RETURN_MESSAGE));

    /* Set modes back to raw */

    ioctl(0,TCSETAW,&tv1);
#ifdef SIGTSTP
    ioctl(0, TIOCSLTC, &lt1);
#endif /*SIGTSTP*/
    if (setndelay(0) != 0)
	vtexit();

    /* Restore Signals */

    setvec(vec, intr);
    (void) sigvector(SIGINT,  &vec, (struct sigvec *)0);
    setvec(vec, escape);
    (void) sigvector(SIGQUIT, &vec, (struct sigvec *)0);

    /* Send switch packet */

    if (send_packet(lanfd,(char *)0,0,VTSWITCH) == -1) {
	logerr("Error in sending vtswitch packet","");
	vtexit();
    }

    /* Reset inmask and doiomode */

    inmask   = saveinmask;
    doiomode = VTMODE;
    return;
}

void
intr()
{
    if (conflag == FALSE)
	vtexit();
    else
	breakflag = TRUE;
}

void
escape()
{
    escapeflag = TRUE;
}

void
childwait()
{
    /* Wait for SIGCLD as long as there is a child */

    sigsetmask(0L);
    while (childpid != -1)
	pause();
}

void
vtquit()
{
    logerr("\r\nConnection Terminated by remote server","");
    vtexit();
}

void
vtexit()
{

    if (conflag == TRUE)
	(void) send_packet(lanfd,(char *)0,0,VTQUIT);

    childwait();

    if (reqtype != VTR_POLL) {

	unsetndelay(0);

	if (uucpflag == FALSE)
	{
	    ioctl(0,TCSETA,&tv0);
#ifdef SIGTSTP
	    ioctl(0, TIOCSLTC, &lt0);
#endif /*SIGTSTP*/
	}
	else {
	    if (waitforclose == TRUE)
		ioctlwait(TIOCCLOSE);
	}
    }

    exit(0);
}

main(argc,argv)
    int argc;
    char *argv[];
{
    char *nodename, *getnodename();
    char *landevice;
    char *localaddress, *getaddress();
    char wantnodename[NODENAMESIZE];
    char gatewaynode[NODENAMESIZE];
    struct sigvec vec;
    extern char *strchr();
    short  mysap;
    int nottyflag;
    int ptyflags;
    int timeout;
    int ret,err;
    int i;

#ifdef DEBUG
    logerr("vt started","");
#endif
#ifdef WAITDEBUG
    for(;;);
#endif

    timeout = 25;
    landevice = DEFAULTLANDEV;
    wantnodename[0] = '\0';
    reqtype = VTR_VTCON;
    nottyflag = FALSE;
    for (i = 1; i < argc; i++) {
	if (argv[i][0] != '-') {
	    if (wantnodename[0] == '\0' && argv[i][0] != '/') {
		strncpy(wantnodename,argv[1],NODENAMESIZE);
		wantnodename[NODENAMESIZE - 1] = '\0';
	    }
	    else
		landevice = argv[i];
	}
	else {
	    switch(argv[i][1]) {

	    case 'p':
		reqtype = VTR_POLL;
		break;

	    case 'u':
		uucpflag     = TRUE;
		waitforclose = TRUE;
#ifdef PUTin? /* this is in original code.  It prevents uucp from working */
		nottyflag    = TRUE;
#endif
		break;

	    default:
		fprintf(stderr,"Illegal option -%c\n",argv[i][1]);
		usage();
		exit(1);

	    }  /* end switch */
	}
    }

#ifdef DEBUG
    logerr("vt: open lan device",landevice);
#endif
    /* Open lan device */

#ifdef NOSELECT
    if ((lanfd = open(landevice,O_RDWR)) < 0) {
#else
    if ((lanfd = open(landevice,(O_RDWR | O_NDELAY) )) < 0) {
#endif
	logerr("Could not open lan device:",landevice);
	exit(1);
    }

    /* setuid back to original user in case vt is setuid to something else */
    /* in order to restrict access to the lan device.                      */

    setuid(getuid());
    setgid(getgid());

    if (reqtype != VTR_POLL) {

	if (wantnodename[0] == '\0') {

	    do
		write(1,NODE_PROMPT,strlen(NODE_PROMPT));
	    while( (ret = vtreadline(0,wantnodename,NODENAMESIZE) ) == 0);

	    if (ret == -1) {
		if (uucpflag == TRUE)
		    ioctlwait(TIOCCLOSE);
		exit(1);
	    }

	    /* If nodename ends with a period ('.') remove it and set   */
	    /* nottyflag to FALSE. This is an undocumented feature that */
	    /* allows people to use cu with a portal.                   */

	    i = strlen(wantnodename) - 1;
	    if (wantnodename[i] == '.') {
		wantnodename[i] = '\0';
		nottyflag = FALSE;
	    }
	}

	if (uucpflag == FALSE) {

	    /* get  & set modes */

	    ioctl(0,TCGETA,&tv0);
	    tv1 = tv0;
	    tv2 = tv0;

	    /* tv1 contains normal raw mode for vt */

	    tv1.c_iflag &= ~(INLCR | ICRNL | IGNCR | IUCLC | IXANY);
	    tv1.c_iflag |= ( IXON | IXOFF);
	    tv1.c_oflag &= ~(OLCUC | ONLCR | OCRNL | ONOCR | ONLRET);
	    tv1.c_oflag |=  OPOST;
	    tv1.c_lflag &= ~(ICANON | ECHO);
	    tv1.c_lflag |= ISIG;
	    tv1.c_cc[VMIN] = '\01';
	    tv1.c_cc[VTIME] = '\0';
	    tv1.c_cc[VINTR] = '\377'; /* Disable interrupt character */
	    tv1.c_cc[VQUIT] = '\035'; /* Default escape character for vt */

	    /* tv2 contains modified cook mode for vt command mode. It   */
	    /* makes sure that break and interrupt are enabled, and that */
	    /* quit is enabled.                                          */

	    tv2.c_iflag |= BRKINT;
	    tv2.c_iflag &= ~(IGNBRK);
	    tv2.c_lflag |= ISIG;
	    tv2.c_cc[VQUIT] = '\377'; /* Disable quit character */

	    tv3 = tv2;

	    /* tv3 is the same as tv2 except that echo is disabled for */
	    /* password input.                                         */

	    tv3.c_lflag &= ~(ECHO);

	    ioctl(0,TCSETAW,&tv1);
#ifdef SIGTSTP
	    ioctl(0, TIOCGLTC, &lt0);
	    lt1.t_suspc  = '\377';
	    lt1.t_dsuspc = '\377';
	    lt1.t_rprntc = '\377';
	    lt1.t_flushc = '\377';
	    lt1.t_werasc = '\377';
	    lt1.t_lnextc = '\377';
	    ioctl(0, TIOCSLTC, &lt1);
#endif /*SIGTSTP*/
	}

	/* Set O_NDELAY on stdin */

	if (setndelay(0) != 0)
	    vtexit();
    }

    setvec(vec, intr);
    (void) sigvector(SIGINT,  &vec, (struct sigvec *)0);

    setvec(vec,escape);
    (void) sigvector(SIGQUIT,  &vec, (struct sigvec *)0);

    setvec(vec, vtexit);
    (void) sigvector(SIGHUP,  &vec, (struct sigvec *)0);
    (void) sigvector(SIGTERM, &vec, (struct sigvec *)0);

    setvec(vec, child_death);
    (void) sigvector(SIGCLD,  &vec, (struct sigvec *)0);

    if ((nodename = getnodename()) == (char *)0) {
	logerr("Cannot get nodename","");
	vtexit();
    }

    /* Get local address */

    if ((localaddress = getaddress(lanfd)) == (char *)0) {
	logerr("ioctl to get local address failed","");
	vtexit();
    }

#ifdef DEBUG
    logerr("vt: init lan connection","");
#endif
    /* Initialize the Lan connection */

    if ((mysap = setuplan(lanfd)) == -1)
	vtexit();

#ifdef DEBUG
    logerr("vt: init vt protocol","");
#endif
    /* Initialize the vt protocol. */

    if (uucpflag == TRUE) {
	doiomode = UUCPMODE;
	init_proto();
    }
    else {
	doiomode = VTMODE;
	init_proto();
    }

    if (reqtype == VTR_POLL) {
#ifdef NOSELECT
	logerr("Poll option disabled since select on lan does not work.","");
#else
	do_poll(lanfd,mysap,localaddress);
#endif
	vtexit();
    }

    if (nottyflag == TRUE)
	ptyflags = (CREATE_UTMP|CREATE_WTMP|NOTTY);
    else
	ptyflags = (PRINTISSUE|CREATE_UTMP|CREATE_WTMP);

    ret = vtrequest(reqtype,lanfd,mysap,nodename,localaddress,
		       wantnodename,(char *)0,(char *)0,
		       ptyflags,timeout,gatewaynode,&err);

    if (ret == -1) {
	logerr("Failure in setting up remote connection.","");
	logerr(pty_errlist[err],"");
	vtexit();
    }
    if (ret == 0) {
	logerr("No response from",wantnodename);
	vtexit();
    }

#ifdef DEBUG
    logerr("vt: connect set up","");
#endif
    umask(0); /* So modes will be correct for file transfer */

#ifndef DEBUGNOBLOCK
    sigsetmask(BLOCK_SIGNALS);
#endif

    if (uucpflag == TRUE)
	doio(lanfd,0,1,nodename);
    else {
	if (gatewaynode[0] != '\0')
	    fprintf(stdout,"Connected to %s via gateway %s.\n\r",
			   wantnodename,gatewaynode);
	else
	    fprintf(stdout,"Connected to %s.\n\r",wantnodename);
	printescape();
	putc('\r',stdout);
	fflush(stdout);
	doio(lanfd,0,1,nodename);
    }

    vtexit(); /* should never reach here */
}

#define SELECT_MASK 1  /* equivalent of (1 << 0(stdin)) */

/* Wait for desired ioctl request, timeout or error */

void
ioctlwait(desired_request)
    int desired_request;
{
    char iobuf[256];
    int readfds;
    int exceptfds;
    int request;
    struct request_info reqbuf;
    struct timeval timeout;

    timeout.tv_sec  = 240; /* 4 minutes */
    timeout.tv_usec = 0;

    request = -1;
    do {

	readfds   = SELECT_MASK;
	exceptfds = SELECT_MASK;
	if (select(1,&readfds,(int *)0,&exceptfds,&timeout) == -1)
	    return;

	if (readfds == 0 && exceptfds == 0)
	    return;

	if (readfds != 0) {

	    /* Read data and throw it away */

	    if (read(0,iobuf,256) < 0)
		return;
	}

	if (exceptfds != 0) {

	    if (ioctl(0,TIOCREQGET,&reqbuf) == 0) {

		ioctl(0,TIOCREQSET,&reqbuf);
		request = reqbuf.request;
	    }
	}
    } while (request != desired_request);

    return;
}

/* vtreadline() is used to read a line at a time from a terminal that */
/* may be in raw mode. vtreadline() removes the terminating newline.  */
/* vtreadline() returns the number of bytes read (not counting the    */
/* newline) or -1 if there was a read error or the EOF character was  */
/* typed (terminal is not in raw mode in the latter case).            */
/* vtreadline() also handshakes ioctl's if we are talking to the      */
/* master side of a pty.                                              */

vtreadline(fd,buf,buflen)
    int fd;
    char *buf;
    int buflen;
{
    int nread;
    int i;
    int readfds,exceptfds;
    int readmask,exceptmask;
    struct request_info reqbuf;
    struct timeval timeout;

    readmask   = (1 << fd);
    exceptmask = (1 << fd);

    timeout.tv_sec  = 240;
    timeout.tv_usec = 0;

    nread = 0;
    do {

	readfds = readmask;
	exceptfds = exceptmask;
	if (select(fd + 1,&readfds,(int *)0,&exceptfds,&timeout) <= 0)
	    return (-1);

	if (readfds != 0) {
	    i = read(fd,&buf[nread],buflen - nread);
	    if (i > 0)
		nread += i;
	    else
		nread = -1;
	}

	if (exceptfds != 0) {
	    if (ioctl(fd,TIOCREQGET,&reqbuf) == 0) {
		ioctl(fd,TIOCREQSET,&reqbuf);

		if (reqbuf.request == TIOCCLOSE)
		    exit(1);
	    }
	}

    } while (nread != -1 && nread != buflen && buf[nread - 1] != '\n');

    if (nread > 0 && buf[nread - 1] == '\n') {
	--nread;
	buf[nread] = '\0';
    }
    else
	nread = -1;

    return(nread);
}

void
do_poll(fd,mysap,localaddress)
    int fd;
    short mysap;
    char *localaddress;
{
    struct vtrequest vtr;
    char to[ADDRSIZE];
    struct vtpoll *nodelist;
    int nodecount;
    char *lastnodename;
    int i,j,k;
    int nrows,ncolumns,index;
    int minhops;
    int fieldsize;

#ifdef DEBUG
    logerr("entered do_pool","");
#endif
    /* Fill in vtrequest structure */

    vtr.hopcount = 0;
    vtr.flags = 0;
    vtr.reqtype = VTR_POLL;
    vtr.majorversion = VT_MAJOR_VERSION;
    vtr.minorversion = VT_MINOR_VERSION;
    vtr.sap = mysap;
    vtr.wantnode[0] = '\0';
    net_aton(vtr.fromaddr,localaddress,ADDRSIZE);

    /* Set destination address */

    net_aton(to,VTMULTICAST,ADDRSIZE);
    if (setaddress(fd,VTREQTYPE,to) != 0) {
	logerr("Could not set destination address","");
	vtexit();
    }

    nodecount = get_results(fd,&nodelist,(char *)&vtr,sizeof (struct vtrequest));

    if (nodecount == 0)
	fprintf(stdout,"No nodes currently servicing vt requests\n");
    else {

	/* eliminate duplicates */

	j = 0;
	lastnodename = "";
	for (i = 0; i < nodecount; i++) {
	    if (strcmp(lastnodename,nodelist[i].nodename) != 0) {
		if (j != 0)
		    nodelist[j - 1].hopcount = minhops;
		nodelist[j].gatewayflag = nodelist[i].gatewayflag;
		strcpy(nodelist[j++].nodename,nodelist[i].nodename);
		minhops = nodelist[i].hopcount;
		lastnodename = nodelist[i].nodename;
	    }
	    else {
		if (nodelist[i].hopcount < minhops)
		    minhops = nodelist[i].hopcount;
	    }
	}
	nodecount = j;

	if (nodecount != 0)
	    nodelist[nodecount - 1].hopcount = minhops;

	/* compute number of rows and columns */

	nrows    = (nodecount - 1) / 6 + 1;
	ncolumns = (nodecount - 1) / nrows + 1;

	/* print the results */

	fprintf(stdout,"Nodes currently servicing vt requests (%d total):\n",nodecount);

	for (i = 0; i < nrows; i++) {
	    for (j = 0; j < ncolumns; j++) {

		index = j * nrows + i;
		if (index < nodecount) {

		    fieldsize = strlen(nodelist[index].nodename);
		    fputs(nodelist[index].nodename,stdout);

		    if (nodelist[index].gatewayflag == TRUE) {
			putc('*',stdout);
			fieldsize++;
		    }

		    for (k = 0; k < nodelist[index].hopcount; k++)
			putc('+',stdout);

		    fieldsize += k;

		    for (k = fieldsize; k < 13; k++)
			putc(' ',stdout);
		}
	    }
	    putc('\n',stdout);
	}
    }
    exit(0);
}

nodecmp(e1,e2)
    struct vtpoll *e1, *e2;
{
    return(strcmp(e1->nodename,e2->nodename));
}

#define MAXNODES      2000  /* Max # of nodes we will read responses from */
#define POLLTIME         7  /* # of seconds to wait before giving up.     */
#define SELECT_POLLTIME  1  /* # of seconds before select times out.      */

#define timediff(u,v) ((u.tv_sec - v.tv_sec) * 1000000 + u.tv_usec - v.tv_usec)

get_results(fd,nlptr,senddata,sendlen)
    int fd;
    struct vtpoll **nlptr;
    char *senddata;
    int sendlen;
{

    struct vtpacket pckt;
    struct vtresponse vtr;
    struct timeval curtime,starttime,seltime;
    struct timezone tz;
    struct vtpoll  *nodelist;
    int readmask,readfds;
    int ret;
    int nodecount;
    int difftime;
    int sentsecond;
    extern char *malloc();
    int i;

    seltime.tv_sec  = SELECT_POLLTIME;
    seltime.tv_usec = 0;

    readmask = (1 << fd);
    nodecount = 0;

    /* Malloc space for responses */

    if ((nodelist = (struct vtpoll *)malloc(MAXNODES * sizeof (struct vtpoll)))
	    == (struct vtpoll *)0) {
	logerr("Could not malloc space for nodelist","");
	vtexit();
    }
    *nlptr = nodelist;

    /* Send the 1st poll request */

    if (send_packet(fd,senddata,sendlen,VTREQUEST) == -1) {
	logerr("Could not send poll request","");
	vtexit();
    }

    gettimeofday(&starttime,&tz);
    sentsecond = FALSE;
    do {
	readfds = readmask;
	if (select(fd + 1,&readfds,(int *)0,(int *)0,&seltime) < 0) {
	    logerr("Select error during wait for poll responses","");
	    vtexit();
	}

	gettimeofday(&curtime,&tz);
	difftime = timediff(curtime,starttime);

	if (readfds != 0)  {

	    /* read the response */

	    do {
		ret = read(fd,(char *)&pckt,MAXPACKSIZE);
#ifdef EWOULDBLOCK
		if ((ret < 0) && (errno == EWOULDBLOCK))
			ret = errno = 0;
#endif
		if(ret < 0) {
		    logerr("Read error in get_results()","");
		    vtexit();
		}

		if (ret == 0)
		    break;

		if (ret < FIXEDSIZE
			|| ret != (FIXEDSIZE + pckt.dlen + pckt.alen)) {
		    logerr("Partial Packet Read in get_results()","");
		    vtexit();
		}
		if (pckt.type != VTPOLLRESPONSE) {
		    logerr("Bad packet type received in get_results()","");
		    vtexit();
		}

		memcpy((char *)&vtr,pckt.data,sizeof (struct vtresponse));
		if (vtr.nodename[0] != '\0') {

		    /* Send a second poll request once we have gotten   */
		    /* a few responses.                                 */

		    if (sentsecond == FALSE && nodecount > 5) {

			/* Send the 2nd poll request. Ignore errors */

			send_packet(fd,senddata,sendlen,VTREQUEST);
			sentsecond = TRUE;
		    }

		    strcpy(nodelist[nodecount].nodename,vtr.nodename);
		    nodelist[nodecount].gatewayflag = vtr.gatewayflag;
		    nodelist[nodecount++].hopcount  = vtr.hopcount;
		}
	    } while (ret != 0 && nodecount < MAXNODES);
	}
    }  while (nodecount < MAXNODES && (difftime < (POLLTIME * 1000000) || readfds != 0) );

    /* Now sort the list */

    (void) qsort((char *) nodelist,nodecount,sizeof (struct vtpoll),nodecmp);

    return(nodecount);
}

void
usage()
{
    fprintf(stderr,"Usage: vt nodename [lan device name]\n");
    fprintf(stderr,"       vt -p [lan device name]\n");
}

void
logerr(s1,s2)
    char *s1,*s2;
{
    char *s3,*ctime();
    long curtime;

    childwait();

    if (uucpflag == TRUE) {
	curtime = time(0);
	s3 = ctime(&curtime);
	s3[strlen(s3) - 1] = '\0';
	fprintf(stderr,"%s pid=%d: %s %s\n",s3,getpid(),s1,s2);
	fflush(stderr);
    }
    else
	fprintf(stderr,"%s %s\n\r",s1,s2);

    return;
}
