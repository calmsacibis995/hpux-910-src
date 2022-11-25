/* @(#) $Revision: 70.1 $ */     
#include <time.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include "ptyrequest.h"

#define IOBUFSIZE 2 * MAXDATASIZE

int breakflag   = FALSE; /* Has break been hit? */
int escapeflag  = FALSE; /* Has the escape character been hit? */
int inmask,saveinmask;
int exceptmask,saveexceptmask;

int doiomode;

extern int errno;

extern void vtexit(), vtquit(), logerr(), resend_packets();

void
doio(lanfd,infd,outfd,nodename)
    int lanfd;
    int infd;
    int outfd;
    char *nodename; /* For vt command mode prompt */
{
    struct timeval timeout;
    extern int iunacked,ounacked;
    int nfds;
    int readfds,exceptfds;
    int lanmask;
    int ret;
    int ptype;
    extern int transmitflag;
    extern int transmittime;
    extern int receivetime;
    extern int ioctlflag;
    extern int gotpacket;
    extern int ptybufsize;
    extern struct vtioctl ioctlbuf;
    static char ptybuf[MAXDATASIZE];
    static char lanbuf[IOBUFSIZE];
    int nread;
    int longtimeout;
    int save_errno;

#ifdef DEBUG
    logerr("doio: entered","");
#endif

    nfds = (infd > lanfd ? infd + 1 : lanfd + 1);

#ifdef NOSELECT
    lanmask = 0;
#else
    lanmask = (1 << lanfd);
#endif
    inmask  = saveinmask  = (1 << infd);
    if (doiomode == VTMODE)
	exceptmask = saveexceptmask = 0;
    else
	exceptmask = saveexceptmask = inmask;

    if (doiomode == UUCPMODE)
	longtimeout = UUCP_TIMEOUT;
    else
	longtimeout = KEEPALIVE;

#ifdef NOSELECT
    (void) setndelay(lanfd);
#endif

    for (;;) {

#ifdef NOSELECT
	timeout.tv_sec  = 0;
	timeout.tv_usec = 50000;
#else
	if (ounacked > 0) {
	    timeout.tv_sec  = 0;
	    timeout.tv_usec = SELECT_TIMEOUT;
	}
	else {
	    if (doiomode == VTSWITCHMODE) {
		if (send_packet(lanfd,(char *)0,0,VTSWITCHACK) == -1) {
		    logerr("Error in sending vtswitchack packet","");
		    vtexit();
		}
		doiomode = VTNRMODE;
	    }
	    timeout.tv_sec = longtimeout;
	    timeout.tv_usec = 0;
	}
#endif

	if (transmitflag == TRUE)
	    readfds = inmask | lanmask;
	else
	    readfds = lanmask;

	if (ioctlflag == FALSE)
	    exceptfds = exceptmask;
	else
	    exceptfds = 0;

#ifdef DEBUG
    logerr("doio: doing select","");
#endif
	/* Allow signals to be delivered during select */

	sigsetmask(0L);
	ret = select(nfds,&readfds,(int *)0,&exceptfds,&timeout);
	save_errno = errno;
#ifndef DEBUGNOBLOCK
	sigsetmask(BLOCK_SIGNALS);
#endif
	if (ret == -1) {
	    if (save_errno != EINTR) {
		logerr("Select failed","");
		vtexit();
	    }
	    readfds = exceptfds = 0;
	}

#ifdef DEBUG
    logerr("doio: back from select","");
#endif
	/* Is there an ioctl/open/close waiting to be processed? */

	if (exceptfds & exceptmask) {
#ifdef DEBUG
            logerr("doio: process_ioctl(infd,lanfd)","");
#endif
	    if (process_ioctl(infd,lanfd) != 0)
		vtexit();
	}

	/* Is there anything to read from the LAN? */

	nread = 0;
#ifndef NOSELECT
	if (readfds & lanmask) {
#endif

	    /* loop, repolling until there is nothing more to read */
	    /* (O_NDELAY should be set on lanfd)                   */

	    do {
		if ((ret = read_packet(outfd,lanfd,&lanbuf[nread])) == -1) {
		    logerr("Read failed on LAN fd","");
		    vtexit();
		}
		nread += ret;
#ifdef DEBUG
                {
		   int errnosav=errno;
		   int indx;
		   char msgbuf[10];

                   lanbuf[nread]='\0';
                   logerr("doio: packet read ",lanbuf);
		   for (indx=0;indx<nread;indx++)
		   {
    		        sprintf(msgbuf,"char %d=%#x ",indx,lanbuf[indx]);
                        logerr("doio: packet read ",msgbuf);
		   }
		   errno=errnosav;
                }
#endif
	    } while (gotpacket == TRUE && nread <= (IOBUFSIZE - MAXDATASIZE));
#ifndef NOSELECT
	}
#endif

	/* If there are any unacknowledged packets check to see if  */
	/* its time to retransmit them.                             */

	if (ounacked > 0)
#ifdef DEBUG
        {
            logerr("doio: ounacked > 0 ","");
#endif
	    resend_packets(lanfd);
#ifdef DEBUG
        }
#endif

	/* Check to see if we have an advisory ioctl packet to send */

	if (ioctlflag == TRUE && transmitflag == TRUE) {
#ifdef DEBUG
            logerr("doio: send advisory packet","");
#endif
	    if (send_packet(lanfd,(char *)&ioctlbuf,sizeof (struct vtioctl),
			    VTIOCTL) == -1) {
		logerr("Error in sending ioctl packet","");
		vtexit();
	    }
	    ioctlflag = FALSE;
	}

	/* Check to see if break has been hit and send a break packet */
	/* as long as escapeflag is not true.                         */

	if (breakflag == TRUE && transmitflag == TRUE && escapeflag == FALSE) {
#ifdef DEBUG
            logerr("doio: send break packet","");
#endif
	    if (send_packet(lanfd,(char *)0,0,VTBREAK) == -1) {
		logerr("Error in sending break packet","");
		vtexit();
	    }
	    breakflag = FALSE;
	}

	if ((readfds & inmask) != 0
	    && (transmitflag == TRUE || doiomode == VTCMDMODE) ) {
#ifdef DEBUG
                logerr("doio: read ptybuf","");
#endif
	    	ret = read(infd,ptybuf,ptybufsize);
#ifdef DEBUG
                {
		   int errnosav=errno;
		   int indx;
		   char msgbuf[10];

        	   ptybuf[ret]='\0';
                   logerr("doio: read ptybuf",ptybuf);
		   for (indx=0;indx<ret;indx++)
		   {
    		        sprintf(msgbuf,"char %d=%#x ",indx,ptybuf[indx]);
                        logerr("doio: packet read ",msgbuf);
		   }
		   errno=errnosav;
                }
#endif

#ifdef EWOULDBLOCK
		if((ret < 0) && (errno == EWOULDBLOCK))
			ret = errno = 0;
#endif

		if(ret < 0) {	
			logerr("Read failed on PTY fd","");
			vtexit();
	    	}
	}
	else
	    ret = 0;

	if ( (ret > 0 && doiomode != VTCMDMODE)
	    || iunacked > 0
	    || (vt_time() - transmittime) >= (longtimeout * 100) ) {

	    if (doiomode == UUCPMODE && iunacked == 0 && ret == 0) {

		/* Reset receivetime so that if vtexit() calls     */
		/* send_packet we won't get into an infinite loop. */

		receivetime = vt_time();
		logerr("VT-UUCP Connection Timeout","");
		vtexit();
	    }

	    if (ret == 0 || doiomode == VTCMDMODE)
		ptype = VTACK;
	    else
		ptype = VTDATA;

#ifdef DEBUG
            {
		   int errnosav=errno;
		   int indx;
		   char msgbuf[10];

        	   ptybuf[ret]='\0';
                   logerr("doio: send_packet over lan",ptybuf);
		   for (indx=0;indx<ret;indx++)
		   {
    		        sprintf(msgbuf,"char %d=%#x ",indx,ptybuf[indx]);
                        logerr("doio: packet read ",msgbuf);
		   }
		   errno=errnosav;
            }
#endif
	    if (send_packet(lanfd,ptybuf,ret,ptype) == -1) {
		logerr("Error in sending packet","");
		vtexit();
	    }
	}

	/* write out what was read from lan */

	if (nread != 0) {
#ifdef DEBUG
            {
		   int errnosav=errno;
		   int indx;
		   char msgbuf[10];

                   lanbuf[nread]='\0';
                   logerr("doio: write lanbuf to outfd ",lanbuf);
		   for (indx=0;indx<nread;indx++)
		   {
    		        sprintf(msgbuf,"char %d=%#x ",indx,lanbuf[indx]);
                        logerr("doio: packet read ",msgbuf);
		   }
		   errno=errnosav;
            }
#endif
	    if (write(outfd,lanbuf,nread) != nread) {
		logerr("Write failed on PTY fd","");
		vtexit();
	    }
	}

	/* Check to see if escape character has been hit and go into */
	/* command mode if it has.                                   */

	if (escapeflag == TRUE)
	    vtcommand(lanfd,ptybuf,ret,nodename);
    }
}
