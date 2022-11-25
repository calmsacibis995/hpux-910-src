/* @(#) $Revision: 29.2 $ */   
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include "ptyrequest.h"

void ftdoio(), sendstatusresponse();
long doftrecvio(), doftsendio();

extern int errno;

extern int transmitflag;
extern int ounacked;
extern int vtftcomplete;
extern int vtftaborted;
extern int breakflag;
extern int nrexmit;
extern int ptybufsize;
extern int gotpacket;

#define FTIOSIZE 16384
#define FTBUFSIZE (FTIOSIZE + MAXDATASIZE)
static char ftbuf[FTBUFSIZE];

void
ftdoio(lanfd,data)
    int lanfd;
    char *data;
{
    static struct vtftrequest vtftrq;
    struct passwd *pw;
    struct stat sbuf;
    static char user[USERLEN] = "";
    static char password[PASSWORDLEN] = "";
    static char homedir[MAXARGSTRLEN] = "";
    static int ftuid;
    static int ftgid;
    int ftfd;
    char *filename;
    int reqtype;
    extern struct passwd *getpwnam();
    long ntransfered;

    memcpy((char *)&vtftrq,data,sizeof (struct vtftrequest));

    filename = vtftrq.filename;
    reqtype  = vtftrq.ftreqtype;

    switch (reqtype) {

    case FTRECEIVE:

	if (user[0] == '\0') {
	    sendstatusresponse(lanfd,0,0,"File transfer user not logged.");
	    return;
	}

	if ((ftfd = open(filename,O_RDONLY)) < 0) {
	    sendstatusresponse(lanfd,0,0,"Could not open remote file for reading.");
	    return;
	}

	if (fstat(ftfd,&sbuf) != 0) {
	    close(ftfd);
	    sendstatusresponse(lanfd,0,0,"Could not stat remote file.");
	    return;
	}

	/* Send successful status response */

	sendstatusresponse(lanfd,sbuf.st_size,(sbuf.st_mode & 0777),"");

	/* Do the actual file tranfer */

	ntransfered = doftsendio(lanfd,ftfd);

	close(ftfd);

	/* Send successful finish status response */

	sendstatusresponse(lanfd,ntransfered,0,"");
	return;

    case FTSEND:

	if (user[0] == '\0') {
	    sendstatusresponse(lanfd,0,0,"File transfer user not logged.");
	    return;
	}

	if ((ftfd = open(filename,(O_WRONLY|O_TRUNC|O_CREAT),vtftrq.modes)) < 0) {
	    sendstatusresponse(lanfd,0,0,"Could not open/create remote file for writing.");
	    return;
	}

	/* Send successful status response */

	sendstatusresponse(lanfd,0,0,"");

	/* Do the actual file tranfer */

	ntransfered = doftrecvio(lanfd,ftfd);

	close(ftfd);

	/* Send successful finish status response */

	sendstatusresponse(lanfd,ntransfered,0,"");
	return;

    case FTLOGUSER:

	/* Validate User */

	if ((pw = getpwnam(vtftrq.user)) == (struct passwd *)0) {
	    sendstatusresponse(lanfd,0,0,"Bad user name.");
	    return;
	}

	if (pw->pw_passwd[0] != '\0') {
	    if (strcmp(crypt(vtftrq.password,pw->pw_passwd),pw->pw_passwd) != 0) {
		sendstatusresponse(lanfd,0,0,"Incorrect password supplied.");
		return;
	    }
	}

	/* OK we have a good user and password */

	strcpy(user,vtftrq.user);
	strcpy(password,vtftrq.password);
	strncpy(homedir,pw->pw_dir,MAXARGSTRLEN);
	homedir[MAXARGSTRLEN - 1] = '\0';
	if (chdir(homedir) != 0)
	    homedir[0] = '\0';
	ftuid = pw->pw_uid;
	ftgid = pw->pw_gid;
	setgid(ftgid);
	setuid(ftuid);

	/* Send successful status response */

	sendstatusresponse(lanfd,0,0,"");
	return;

    case FTGETREXMIT:

	sendstatusresponse(lanfd,nrexmit,VT_MINOR_VERSION,"");
	return;

    case FTCD:
	if (filename[0] == '\0')
	    filename = homedir;

	if (filename[0] != '\0') {
	    if (chdir(filename) != 0) {
		sendstatusresponse(lanfd,0,0,"Remote directory change failed.");
		return;
	    }
	}

	sendstatusresponse(lanfd,0,0,"");
	return;

    case FTCHGBUFSIZE:
	if (vtftrq.filesize > 0) {

	    if (vtftrq.filesize > MAXDATASIZE)
		ptybufsize = MAXDATASIZE;
	    else
		ptybufsize = vtftrq.filesize;
	}

	/* Send back pty buffer size */

	sendstatusresponse(lanfd,ptybufsize,0,"");
	return;

    default:
	sendstatusresponse(lanfd,0,0,"Unrecognized file transfer request received.");
	return;
    }
}

#define FT_TIMEOUT 60 /* 60 seconds */

long
doftrecvio(lanfd,ftfd)
    int lanfd;
    int ftfd;
{
    struct timeval timeout;
    int nfds;
    int ftlanfds;
    long nwritten;
    int nread;
    char *bufptr,*topptr;
    int save_errno;
    int ret;

    nfds = lanfd + 1;
    ftlanfds = (1 << lanfd);
    vtftcomplete = FALSE;
    nwritten = 0;
    bufptr = ftbuf;
    topptr = ftbuf + FTIOSIZE;
    do {

	timeout.tv_sec = FT_TIMEOUT;
	timeout.tv_usec = 0;

	sigsetmask(0L);
	ret = select(nfds,&ftlanfds,(int *)0,(int *)0,&timeout);
	save_errno = errno;
	sigsetmask(BLOCK_SIGNALS);
	if (ret == -1 || breakflag == TRUE) {
	    if (breakflag == TRUE) {

		/* For performance reasons doftrecvio() does   */
		/* not call resend_packets(), therefore we     */
		/* cannot reliably send an abort. We therefore */
		/* send an abort request twice for each time   */
		/* the interrupt or break key is pressed which */
		/* should reduce the chance of the abort not   */
		/* getting through to almost zero. In the worst*/
		/* case the user will have to keep hitting the */
		/* break or interrupt key.                     */

		(void) send_packet(lanfd,(char *)0,0,VTFTABORT);
		(void) send_packet(lanfd,(char *)0,0,VTFTABORT);

		/* Reset ftlanfds so the interupt won't look   */
		/* like a select timeout.                      */

		ftlanfds = (1 << lanfd);
	    }
	    else {
		logerr("Select failed","");
		vtexit();
	    }
	}

	if (ftlanfds == 0) {
	    logerr("File Transfer Connection Timeout","");
	    vtexit();
	}

	/* loop, repolling until there is nothing more to read */
	/* (O_NDELAY should be set on lanfd)                   */

	do {
	    if ((nread = read_packet(-1,lanfd,bufptr)) == -1) {
		logerr("Read failed on LAN fd","");
		vtexit();
	    }

	    if (send_packet(lanfd,(char *)0,0,VTACK) == -1) {
		logerr("Error in sending ack packet.","");
		vtexit();
	    }

	    if (nread > 0) {
		bufptr += nread;
		if (bufptr >= topptr) {
		    if (write(ftfd,ftbuf,FTIOSIZE) != FTIOSIZE) {
			logerr("write error during file transfer","");
			vtexit();
		    }

		    nwritten += FTIOSIZE;
		    memcpy(ftbuf,topptr,(int)(bufptr - topptr));
		    bufptr -= FTIOSIZE;
		}
	    }
	} while (gotpacket == TRUE && vtftcomplete == FALSE);
    } while (vtftcomplete == FALSE);

    /* Write out remaining bytes in buffer */

    nread = (int)(bufptr - ftbuf);
    if (write(ftfd,ftbuf,nread) != nread) {
	logerr("write error during file transfer","");
	vtexit();
    }

    nwritten += nread;
    return(nwritten);
}

long
doftsendio(lanfd,ftfd)
    int lanfd;
    int ftfd;
{
    struct timeval timeout;
    int ret;
    int save_errno;
    int nfds;
    int ftlanmask;
    int ftlanfds;
    int nstored;
    int nread;
    int nsend;
    long nsent;
    char *bufptr;

    nfds = lanfd + 1;
    ftlanmask = (1 << lanfd);

    timeout.tv_sec = FT_TIMEOUT;
    timeout.tv_usec = 0;

    nstored = 0;
    nsent   = 0;
    nread   = 1;
    bufptr = ftbuf;
    vtftaborted = FALSE;
    do {

	while (transmitflag == TRUE) {

	    if (nstored < MAXDATASIZE) {
		if (vtftaborted == TRUE)
		    nread = 0;
		else {
		    memcpy(ftbuf,bufptr,nstored);
		    bufptr = ftbuf;

		    if ((nread = read(ftfd,&ftbuf[nstored],FTIOSIZE)) < 0) {
			logerr("Read Error during file transfer","");
			vtexit();
		    }
		    nstored  += nread;
		}
	    }

	    if (nstored > 0) {
		nsend = (nstored < MAXDATASIZE ? nstored : MAXDATASIZE);

		if (send_packet(lanfd,bufptr,nsend,VTDATA) == -1) {
		    logerr("Error in sending packet during file tranfer","");
		    vtexit();
		}
		bufptr  += nsend;
		nsent   += nsend;
		nstored -= nsend;
	    }
	    else
		break;
	}

	ftlanfds = ftlanmask;
	sigsetmask(0L);
	ret = select(nfds,&ftlanfds,(int *)0,(int *)0,&timeout);
	save_errno = errno;
	sigsetmask(BLOCK_SIGNALS);
	if (ret == -1 || breakflag == TRUE) {
	    if (breakflag == TRUE) {
		vtftaborted = TRUE;
		nread = 0;
		ftlanfds = ftlanmask; /* Reset so this doesn't look */
	    }                         /* like a select timeout.     */
	    else {
		logerr("Select failed","");
		vtexit();
	    }
	}

	if (ftlanfds == 0 && timeout.tv_sec != 0) {
	    logerr("File Transfer Connection Timeout","");
	    vtexit();
	}

	do {
	    if (read_packet(-1,lanfd,ftbuf) != 0) {
		logerr("Read failed on LAN fd","");
		vtexit();
	    }
	} while (gotpacket == TRUE);

	if (ounacked > 0) {

	    timeout.tv_sec  = 0;
	    timeout.tv_usec = SELECT_TIMEOUT;

	    nread = 1; /* Don't let loop terminate until channel is clear */
	    resend_packets(lanfd);
	}
	else {
	    timeout.tv_sec = FT_TIMEOUT;
	    timeout.tv_usec = 0;
	}
    } while (nread > 0);

    if (send_packet(lanfd,(char *)0,0,VTFTDONE) == -1) {
	logerr("Error in sending file transfer completed packet","");
	vtexit();
    }

    return(nsent);
}

void
sendstatusresponse(lanfd,arg1,arg2,errormsg)
    int lanfd;
    long arg1;
    short arg2;
    char *errormsg;
{
    static struct vtstatresponse vtstr;

    vtstr.arg1 = arg1;
    vtstr.arg2 = arg2;
    if (errormsg[0] == '\0') {
	vtstr.error = FALSE;
	vtstr.errormsg[0] = '\0';
    }
    else {
	vtstr.error = TRUE;
	strcpy(vtstr.errormsg,errormsg);
    }

    /* Send the status response */

    if (send_packet(lanfd,(char *)&vtstr,sizeof (struct vtstatresponse),
		    VTSTATRESPONSE) == -1) {
	logerr("Error in sending status response","");
	vtexit();
    }
    return;
}
