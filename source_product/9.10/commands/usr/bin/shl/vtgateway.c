/* @(#) $Revision: 66.1 $ */    
#include <stdio.h>
#include <time.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include "ptyrequest.h"

FILE *logfptr;

#define LAN1FD 0
#define LAN2FD 1
#define LOGFD  2

#define NFDS 2 /* for select */
#define LAN1MASK 1
#define LAN2MASK 2
#define READMASK (LAN1MASK | LAN2MASK)

extern int errno;

int connectflag = FALSE; /* Is connection up? */
struct timeval timeout;  /* needs to be global so that transfer packets */
			 /* can reset the timeout value after the conn- */
			 /* ection is up.                               */

void vtexit(), terminate(), sendquitpacket();
void logerr(), relaypackets(), transferpackets();

void
vtexit()
{
    if (connectflag == TRUE) {
	sendquitpacket(LAN1FD);
	sendquitpacket(LAN2FD);
    }
    exit(0);
}

void
terminate()
{
    logerr("vtgateway received signal 15","");
    vtexit();
}

main(argc,argv)
    int argc;
    char *argv[];
{
    struct vtpacket pckt;
    struct vtresponse vtr;
    char *localaddress, *getaddress();
    char *net_aton();
    char *nodename, *getnodename();
    int i;

    /* The vtdaemon has set things up so that the following file */
    /* descriptors are open:                                     */
    /*      0) incoming lan connection (nearest requester) LAN1  */
    /*      1) outgoing lan connection (nearest server)    LAN2  */
    /*      2) error logging file                                */

    /* set up error logging file pointer from file descriptor 2  */

    logfptr = fdopen(LOGFD,"a");
    setbuf(logfptr,(char *)0);

    /* Set signal handler for SIGTERM to be terminate() */

#ifdef hp9000s200
    (void) signal(SIGTERM, terminate);
#else
    (void) signal(SIGTERM, (int (*)()) terminate);
#endif

    /* The vtdaemon passes the logged source sap in argv[5].     */

    vtr.sap = atoi(argv[5]);

    /* Get local address */

    if ((localaddress = getaddress(LAN1FD)) == (char *)0)
	exit(1);

    net_aton(vtr.address,localaddress,ADDRSIZE);

    if ((nodename = getnodename()) == (char *)0)
	exit(1);

    strcpy(vtr.gatewaynode,nodename);

    /* Send the response packet */

    vtr.error = 0;

    pckt.dlen   = sizeof (struct vtresponse);
    pckt.alen   = 0;
    pckt.type   = VTRESPONSE;
    pckt.seqno  = 0;
    pckt.rexmit = 0;


    memcpy(pckt.data,(char *)&vtr,sizeof (struct vtresponse));

    i = FIXEDSIZE + sizeof (struct vtresponse); /* size of packet */
    if (write(LAN1FD,(char *)&pckt,i) != i ) {
	logerr("Failure in sending response packet","");
	exit(1);
    }

    /* response sent, call relaypackets() */

    relaypackets();
}

void
relaypackets()
{
    int readfds;

    timeout.tv_sec  = 40;
    timeout.tv_usec = 0;

    for (;;) {

	readfds = READMASK;

	/* Allow SIGTERM or SIGHUP to be delivered during select */

	sigsetmask(BLOCK_OTHERSIGS);
	if (select(NFDS,&readfds,(int *)0,(int *)0,&timeout) == -1) {
	    logerr("Select failed in gateway server","");
	    vtexit();
	}
	(void) sigsetmask(BLOCK_SIGNALS);

	/* Exit on timeout */

	if (readfds == 0)
	    vtexit();

	if (readfds & LAN1MASK)
	    transferpackets(LAN1FD,LAN2FD);

	if (readfds & LAN2MASK)
	    transferpackets(LAN2FD,LAN1FD);
    }
}

void
transferpackets(fromfd,tofd)
    int fromfd;
    int tofd;
{
    register int nread;
    register int packet_type;
    struct vtpacket pckt;

    do {
	nread = read(fromfd,(char *)&pckt,MAXPACKSIZE);

#ifdef EWOULDBLOCK
	if((nread < 0) && (errno == EWOULDBLOCK))
		nread = errno = 0;
#endif

	if(nread < 0) {
	    logerr("Read error in transferpackets","");
	    vtexit();
	}

	if (nread > 0) {
	    if (write(tofd,(char *)&pckt,nread) == -1) {
		logerr("Write error in transferpackets","");
		vtexit();
	    }

	    if (nread >= FIXEDSIZE) {

		packet_type = pckt.type;
		if (packet_type == VTQUIT || packet_type == VTDONE)
		    exit(0);

		if (packet_type == VTOK) {
		    timeout.tv_sec = SV_TIMEOUT;
		    connectflag = TRUE;
		}
	    }
	}
    } while (nread > 0);
}

void
sendquitpacket(fd)
    int fd;
{
    struct vtpacket pckt;

    pckt.dlen   = 0;
    pckt.alen   = 0;
    pckt.type   = VTQUIT;
    pckt.seqno  = 0;
    pckt.rexmit = 0;

    write(fd,(char *)&pckt,FIXEDSIZE);
}

void
logerr(s1,s2)
    char *s1,*s2;
{
    char *s3,*ctime();
    long curtime;

    if (logfptr != (FILE *)0) {
	curtime = time(0);
	s3 = ctime(&curtime);
	s3[strlen(s3) - 1] = '\0';
	fprintf(logfptr,"%s pid=%d: %s %s\n",s3,getpid(),s1,s2);
	fflush(logfptr);
    }
    return;
}
