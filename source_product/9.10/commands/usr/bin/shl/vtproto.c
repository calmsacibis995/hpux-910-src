/* @(#) $Revision: 70.1 $ */      
#include <time.h>
#include <sys/ptyio.h>
#include "ptyrequest.h"
#include <errno.h>

extern long time();
extern void vtexit(), vtquit(), logerr(), ftdoio();

extern int doiomode;
extern int inmask,saveinmask;
extern int exceptmask,saveexceptmask;

static struct vtpacket opcktpool[CACHE_PACKETS];
static long oslottime[CACHE_PACKETS];
static int iunackq[CACHE_PACKETS];
static int pcktmap[NUMSEQ];
static int iseqno;      /* expected sequence number of next incoming packet */
static int oseqno;      /* sequence number of next outgoing packet          */
static int oslot;       /* position to store next packet in opcktpool       */
static int prototimeout;/* time to wait before timing out                   */
static int protostime;  /* time in seconds when init_proto() was called.    */
int receivetime;        /* Last receive time.                               */
int transmittime;       /* Last transmit time.                              */
int iunacked;           /* number of unacked packets in iunackq             */
int ounacked;           /* number of transmitted but unacked packets        */
int nrexmit;            /* Number of total rexmits since startup.           */
int ptybufsize;         /* Max size of read from pty.                       */
int transmitflag;       /* TRUE if transmits are allowed.                   */
int vtswitched;         /* TRUE if pty read mode has just been switched.    */
int vtftcomplete;       /* TRUE if a VTFTDONE packet has been received.     */
int vtftaborted;        /* TRUE if a VTFTABORT packet has been received.    */
int gotpacket;          /* TRUE if a packet was received in read_packet()   */
int conflag = FALSE;    /* TRUE if connection is up (handshake completed)   */
int ioctlflag;          /* TRUE if an advisory ioctl is ready to be sent    */
int waitforclose=FALSE; /* TRUE if we should wait for close in vtexit().    */
struct vtioctl ioctlbuf;/* holds currently pending advisory ioctl.          */

#define INFINITE_RESEND_TIME 360000 /* Effectively infinite as long as it */
				    /* is greater than the largest        */
				    /* protocol timeout value.            */

static int resendtime[] = { 90, 90, 200, 200, 400, 400, 800, 1600,
			    6000, INFINITE_RESEND_TIME};

void
init_proto()
{
    register int i;
    struct timeval tvl;
    struct timezone tzn;

#ifdef DEBUG
    logerr("ntered init_proto","");
#endif
    iseqno = oseqno = oslot = iunacked = ounacked = nrexmit = 0;

    ptybufsize = MAXDATASIZE;

    /* protostime needs to be set before vt_time is called */

    gettimeofday(&tvl,&tzn);
    protostime = tvl.tv_sec;

    receivetime = transmittime = vt_time();

    vtswitched = FALSE;

    switch (doiomode) {

    case VTMODE:
	transmitflag = TRUE;
	prototimeout = RQ_TIMEOUT;
	break;

    case UUCPMODE:
	transmitflag = TRUE;
	prototimeout = SV_TIMEOUT;
	break;

    case VTSERVERMODE:
	transmitflag = FALSE;
	prototimeout = IN_TIMEOUT; /* Will change to SV_TIMEOUT after */
	break;                     /* handshake is completed.         */
    }

    ioctlflag = FALSE;

    for (i=0; i< CACHE_PACKETS;i++)
	oslottime[i] = 0;

    for (i=0; i<NUMSEQ;i++)
	pcktmap[i] = -1;
}

read_packet(iofd,lanfd,data)
    int iofd;
    int lanfd;
    char *data;
{
    static struct vtpacket pckt;
    char *s;
    int i;
    int slotno;
    int retval;
    int type;

    gotpacket = FALSE;

#ifdef DEBUG
    logerr("read_packet: entered","");
#endif
    /* Don't attempt read if there is no room on the unacked queue */
    /* to record this packet.                                      */

    if (iunacked == CACHE_PACKETS)
	return(0);

#ifdef DEBUG
    logerr("read_packet: reading packet","");
#endif
    if ((i = read(lanfd,(char *)&pckt,MAXPACKSIZE)) < 0) 
#ifdef EWOULDBLOCK
	if (errno == EWOULDBLOCK) 
		errno  = i = 0;
	else
#endif
		return(-1);


    if (i == 0)
	return(0);

#ifdef DEBUG
    logerr("read_packet: packed read","");
#endif
    gotpacket = TRUE;

    if (i < FIXEDSIZE || i != (FIXEDSIZE + pckt.dlen + pckt.alen)) {
	logerr("Partial Packet Read in read_packet","");
	return(-1);
    }

    type = pckt.type;

    /* Die if this is a quit packet */

    if (type == VTQUIT)
	vtquit();

    /* Terminate normally if this is a done packet */

    if (type == VTDONE) {
	logerr("\r\nvt terminated.","");
	vtexit();
    }

    /* Update receive time */

    receivetime = vt_time();

    /* process incoming acks */

    s = pckt.data + pckt.dlen;
    for (i=0; i < pckt.alen; i++,s++) {
	if ((slotno = pcktmap[(int) *s]) != -1) {
	    pcktmap[(int) *s] = -1;
	    oslottime[slotno] = 0;
	    if (slotno == oslot)
		transmitflag = TRUE;
	    ounacked--;
	}
    }

    if (type <= VTDATA) {

	/* Check to see if packet is either the currently expected one or */
	/* an older transmitted one. If so we need to ack (or re-ack) it. */
	/* Therefore we will put it on the input unacked packet queue and */
	/* increment the number of unacked input packets.                 */

	i = iseqno - pckt.seqno;
	if ( (i >= 0 && i <= CACHE_PACKETS) || i <= (CACHE_PACKETS - NUMSEQ))
	    iunackq[iunacked++] = pckt.seqno;

	/* reject the packet if it is not the currently expected one */

	if (pckt.seqno != iseqno)
#ifdef DEBUG
	{
            logerr("read_packet: rejecting packet","");
#endif
	    return(0);
#ifdef DEBUG
	}
#endif
	else {
#ifdef DEBUG
            logerr("read_packet: packet ok","");
#endif

	    /* increment expected input sequence number */

	    if (++iseqno == NUMSEQ)
		iseqno = 0;

	    /* Do what is indicated by the packet type and set the */
	    /* return value.                                       */

	    switch (type) {

	    case VTBREAK:
#ifdef DEBUG
                logerr("read_packet: type=VTBREAK","");
#endif
		if (iofd != -1)
		    ioctl(iofd,TIOCBREAK,0);
		retval = 0;
		break;

	    case VTIOCTL:
		{
		    static struct vtioctl ibuf;
		    static struct termio  tbuf;
#ifdef DEBUG
                    logerr("read_packet: type=VTIOCTL","");
#endif

		    if (iofd != -1 && doiomode != UUCPMODE) {

			/* WARNING! The termio structure does not align the  */
			/* same way on the Series 500 and Series 200. Do not */
			/* try to reference anything in the c_cc[] field of  */
			/* the termio structure passed in the packet.        */

			memcpy((char *)&ibuf,pckt.data,sizeof (struct vtioctl));

			if (ioctl(iofd,TCGETA,&tbuf) != 0)
			    return(0); /* Don't care. Allows i/o from pipes. */

			if ((tbuf.c_iflag & IXON) != (ibuf.arg.c_iflag & IXON)) {
			    if ((ibuf.arg.c_iflag & IXON) == 0)
				tbuf.c_iflag &= ~IXON;
			    else
				tbuf.c_iflag |= IXON;

			    if (ioctl(iofd,TCSETAW,&tbuf) != 0) {
				logerr("TCSETAW failed on pty","");
				return(-1);
			    }
			}
		    }
		    retval = 0;
		    break;
		}

	    case VTOK:
#ifdef DEBUG
                logerr("read_packet: type=VTOK","");
#endif
		conflag = TRUE;
		transmitflag = TRUE;
		prototimeout = SV_TIMEOUT;
		retval = 0;
		break;

	    case VTSWITCH: /* Should only be received by vtserver */
#ifdef DEBUG
                logerr("read_packet: type=VTSWITCH","");
#endif

		switch (doiomode) {

		case VTNRMODE:
		    doiomode = VTSERVERMODE;
		    inmask = saveinmask;
		    exceptmask = saveexceptmask;
		    break;

		case VTSERVERMODE:
		    doiomode = VTSWITCHMODE;
		    inmask = 0;
		    exceptmask = 0;
		    break;

		case VTSWITCHMODE:
		    break;

		default:
		    logerr("Bad value for doiomode encountered","");
		    vtexit();
		}
		retval = 0;
		break;

	    case VTSWITCHACK: /* Should only be received by vt */
#ifdef DEBUG
                logerr("read_packet: type=VTSWITCHACK","");
#endif
		vtswitched = TRUE;
		retval = 0;
		break;

		break;

	    case VTFTREQUEST:
#ifdef DEBUG
                logerr("read_packet: type=VTFTREQUEST","");
#endif
		if (pckt.dlen != sizeof (struct vtftrequest)) {
		    logerr("Partial ftrequest packet received in read_packet","");
		    vtexit();
		}
		ftdoio(lanfd,pckt.data);
		retval = 0;
		break;

	    case VTFTDONE:
#ifdef DEBUG
                logerr("read_packet: type=VTFTDONE","");
#endif
		vtftcomplete = TRUE;
		retval = 0;
		break;

	    case VTPOLLRESPONSE:
	    case VTSTATRESPONSE:
	    case VTDATA:
#ifdef DEBUG
                logerr("read_packet: type=VTDATA","");
#endif
		retval = pckt.dlen;
		memcpy(data,pckt.data,retval);
		break;

	    default:
		logerr("Bad Packet Type received in read_packet","");
		vtexit();
	    }
	}
    }
    else {
	switch (type) {

	case VTRESPONSE:
#ifdef DEBUG
            logerr("read_packet: type=VTRESPONSE","");
#endif
	    if (conflag == FALSE) {
		retval = pckt.dlen;
		memcpy(data,pckt.data,retval);
	    }
	    else
		retval = 0;
	    break;

	case VTFTABORT:
#ifdef DEBUG
            logerr("read_packet: type=VTFTABORT","");
#endif
	    vtftaborted = TRUE;
	    retval = 0;
	    break;

	default:
	    retval = 0;
	    break;
	}
    }

    /* return with the return value that was set above */

    return(retval);
}

send_packet(fd,data,len,type)
    int fd;
    char *data;
    int len;
    char type;
{
    static struct vtpacket vbuf; /* For packet types > VTDATA */
    struct vtpacket *vp;
    int i;
    long curtime;
    char *s;

    /* Die here if nothing has been received in more than prototimeout secs */
    /* and we are not currently doing a shell escape.                       */

    curtime = vt_time();
    if ((curtime - receivetime) > prototimeout && doiomode != VTCMDMODE) {

	if (conflag == TRUE) {
	    logerr("Connection Timeout","");
	}

	/* reset receivetime to prevent an infinite loop, since vtexit() */
	/* calls send_packet().                                          */

	receivetime = curtime;

	vtexit();
    }

    if (type <= VTDATA) {
	if  (oslottime[oslot] != 0)
	    return(-1);
	vp = &opcktpool[oslot];
    }
    else
	vp = &vbuf;

    /* Fill in packet info */

    vp->dlen = len;
    vp->alen = iunacked;
    vp->type = type;
    vp->seqno = oseqno;
    vp->rexmit = 0;
    if (len > 0)
	memcpy(vp->data,data,len);
    s = vp->data + len;

    /* Acknowledge unacked packets */

    for (i=0; i < iunacked;i++)
	*s++ = (char) iunackq[i];

    /* Send the packet */

    i = FIXEDSIZE + len + iunacked;
    if (write(fd,(char *)vp,i) != i)
	return(-1);

    /* Update transmit time */

    transmittime = curtime;

    if (type <= VTDATA) {

	/* store the sequence number to slot mapping */

	pcktmap[oseqno] = oslot;

	/* store the packet transmit time */

	oslottime[oslot] = vt_time();

	/* increment the output slot pointer */

	if (++oslot == CACHE_PACKETS)
	    oslot = 0;

	if (oslottime[oslot] != 0)
	    transmitflag = FALSE;

	/* increment the output unacked packet count. */

	ounacked++;

	/* bump the sequence number counter */

	if (++oseqno == NUMSEQ)
	    oseqno = 0;
    }

    /* reset the input unacked packet count */

    iunacked = 0;

    return(0);
}

void
resend_packets(fd)
    int fd;
{
    register int i,slotno;
    int pcktsize;
    long curtime;
    struct vtpacket *vp;

    curtime = vt_time();
    for (i = 0, slotno = oslot ; i < CACHE_PACKETS; i++, slotno++) {

	if (slotno == CACHE_PACKETS)
	    slotno = 0;

	if (oslottime[slotno] != 0) {

	    vp = &opcktpool[slotno];

	    if ( (curtime - oslottime[slotno]) >= resendtime[vp->rexmit]) {

		/* Attempt to retransmit this packet and update packet */
		/* transmit time if successful. Increment the re-      */
		/* transmit count, and decrement it if the write fails.*/

		(vp->rexmit)++;
		pcktsize = FIXEDSIZE + vp->dlen + vp->alen;
		if (write(fd,(char *)vp,pcktsize) == pcktsize) {
		    oslottime[slotno] = vt_time();
		    nrexmit++;
		}
		else
		    (vp->rexmit)--;
	    }
	}
    }
}

process_ioctl(infd,lanfd)
    int infd;
    int lanfd;
{
    static struct request_info reqbuf;
    int request;

    if (ioctl(infd,TIOCREQGET,&reqbuf) != 0) {
	logerr("ioctl request get failed","");
	return(-1);
    }
#ifdef DEBUG
    logerr("process_ioctl: entered","");
#endif

    request = reqbuf.request;

    if (request == TCSETA || request == TCSETAW || request == TCSETAF) {
	ioctlbuf.request = request;
	if (ioctl(infd,reqbuf.argget,&ioctlbuf.arg) != 0) {
	    logerr("ioctl argument get failed","");
	    return(-1);
	}

	if (doiomode == VTSERVERMODE) {
	    if ((ioctlbuf.arg.c_cflag & CBAUD) == B0) {
		send_packet(lanfd,(char *)0,0,VTDONE);
		vtquit();
	    }
	    ioctlflag = TRUE;
	}
    }

#ifdef DEBUG
    logerr("process_ioctl: finish handshake","");
#endif
    /* Finish handshake */

    ioctl(infd,TIOCREQSET,&reqbuf);

    if (doiomode == UUCPMODE && request == TIOCCLOSE) {
	waitforclose = FALSE;
	vtexit();
    }

    return(0);
}

/* vt_time() returns the time in hundredths of a second since the */
/* protocol initilization.                                        */

vt_time()
{
    static struct timeval tvl;
    static struct timezone tzn;

    gettimeofday(&tvl,&tzn);
    return( (tvl.tv_sec - protostime) * 100 + (tvl.tv_usec / 10000) );
}
