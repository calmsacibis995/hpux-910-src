/* @(#) $Revision: 70.1 $ */     
#include <stdio.h>
#include <netio.h>
#include <time.h>
#include <sys/signal.h>
#include "ptyrequest.h"

FILE *logfptr;

#define COMFD 0
#define PTYFD 1
#define LOGFD 2

extern int conflag; /* Is connection up? */
extern int doiomode;

void vtquit(), vtexit(), terminate(), logerr();
extern void init_proto(), doio();

void
vtquit()
{
    exit(0);
}

void
vtexit()
{
    if (conflag == TRUE)
	(void) send_packet(COMFD,(char *)0,0,VTQUIT);
    exit(0);
}

void
terminate()
{
    logerr("vtserver received signal 15","");
    vtexit();
}

main(argc,argv)
    int argc;
    char *argv[];
{
    struct vtresponse rpckt;
    char *localaddress, *getaddress();
    char *net_aton();

    /* The vtdaemon has set things up so that the following file */
    /* descriptors are open:                                     */
    /*      0) lan connection                                    */
    /*      1) master pty                                        */
    /*      2) error logging file                                */

    /* set up error logging file pointer from file descriptor 2  */

    logfptr = fdopen(LOGFD,"a");
    setbuf(logfptr,(char *)0);

#ifdef DEBUG
    {
       int indx;
       char errmsgbuf[80];

       logerr("vtserver starting","");
       for (indx=0;indx<argc;indx++)
       {
	  sprintf(errmsgbuf,"vtserver: argv[%d]=%s",indx,argv[indx]);
	  logerr(errmsgbuf,"");
       }
    }
#endif
    /* Set signal handler for SIGTERM to be terminate() */

#ifdef hp9000s200
    (void) signal(SIGTERM, terminate);
#else
    (void) signal(SIGTERM, (int (*)()) terminate);
#endif

    /* The vtdaemon passes the logged source sap in argv[3].     */

    rpckt.sap = atoi(argv[3]);

    /* Get local address */

    if ((localaddress = getaddress(COMFD)) == (char *)0) {
	logerr("ioctl to get local address failed","");
	exit(1);
    }
    net_aton(rpckt.address,localaddress,ADDRSIZE);

    /* Send the response packet */

    rpckt.gatewaynode[0] = '\0';
    rpckt.error = 0;

    doiomode = VTSERVERMODE;
    init_proto();
    if (send_packet(COMFD,(char *)&rpckt,sizeof (struct vtresponse),VTRESPONSE)
	== -1) {
	logerr("Failure in sending response packet","");
	exit(0);
    }

    umask(0); /* So modes will be right for file transfer */

    /* response sent, call doio() */

    doio(COMFD,PTYFD,PTYFD,"");

    /* NOT REACHED */
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
