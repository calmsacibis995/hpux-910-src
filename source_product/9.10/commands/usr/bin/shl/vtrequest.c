/* @(#) $Revision: 70.1 $ */    
#include <time.h>
#include "ptyrequest.h"

/* vtrequest returns: -1) error, 0) no response 1) good response */

#define DEFAULTLOGIN  "UNKNOWN"
#define VTUUCPLOGIN   "VT-UUCP"

extern int conflag;

vtrequest(reqtype,fd,mysap,mynodename,myaddress,wantnodename,argstr1,
	     argstr2,flags,timeout,gatewaynode,error)
    int reqtype;
    int fd;
    short mysap;
    char *mynodename;
    char *myaddress;
    char *wantnodename;
    char *argstr1;
    char *argstr2;
    int flags;
    int timeout;
    char *gatewaynode;
    int *error;
{
    struct vtrequest vtr;
    struct vtresponse rpckt;
    char to[ADDRSIZE];
    char *ml;
    extern char *cuserid();
#ifndef NOSELECT
    int readfds;
#endif
    struct timeval seltimeout;
#ifdef DEBUG
    logerr("entered vtrequest","");
    {
       char errmsgbuf[80];

       sprintf(errmsgbuf,"vtrequest flags parameter = %#x (%d)",flags,flags);
       logerr(errmsgbuf,"");
    }
#endif

    /* Check arguments */

    if (mynodename == (char *)0 || myaddress == (char *)0
	|| wantnodename == (char *)0) {
	if (error != (int *)0)
	    *error = E_MARGERR;
	return(-1);
    }

    /* Fill in vtrequest structure */

    vtr.reqtype = reqtype;
    vtr.majorversion = VT_MAJOR_VERSION;
    vtr.minorversion = VT_MINOR_VERSION;
    vtr.sap = mysap;
    net_aton(vtr.fromaddr,myaddress,ADDRSIZE);
    strcpy(vtr.wantnode,wantnodename);
    strcpy(vtr.mynode,mynodename);
    if (flags & NOTTY) {
	strcpy(vtr.mylogin,VTUUCPLOGIN);
    }
    else {
	if ((ml = cuserid((char *) 0)) == (char *)0)
	    strcpy(vtr.mylogin,DEFAULTLOGIN);
	else
	    strcpy(vtr.mylogin,ml);
    }
    if (argstr1 == (char *)0)
	argstr1 = "";
    strncpy(vtr.argstr1,argstr1,MAXARGSTRLEN);
    vtr.argstr1[MAXARGSTRLEN - 1] = '\0';

    if (argstr2 == (char *)0)
	argstr2 = "";
    strncpy(vtr.argstr2,argstr2,MAXARGSTRLEN);
    vtr.argstr2[MAXARGSTRLEN - 1] = '\0';

    vtr.flags = flags;
    vtr.hopcount = 0;

    /* Set destination address */

    net_aton(to,VTMULTICAST,ADDRSIZE);
    if (setaddress(fd,VTREQTYPE,to) != 0) {
	if (error != (int *)0)
	    *error = E_ADDRERR;
	return(-1);
    }

#ifdef DEBUG
    logerr("vtrequest: send the request","");
#endif
    /* Send the request */

    if (send_packet(fd,(char *)&vtr,sizeof (struct vtrequest),VTREQUEST) == -1) {
	if (error != (int *)0)
	    *error = E_XPKTERR;
	return(-1);
    }

#ifdef DEBUG
    logerr("vtrequest: wait for response","");
#endif
    /* wait for response */

#ifndef NOSELECT
    readfds = (1 << fd);
    seltimeout.tv_sec  = timeout;
    seltimeout.tv_usec = 0;
    if (select(fd+1,&readfds, (int *)0, (int *)0,&seltimeout) == -1) {
	if (error != (int *)0)
	    *error = E_SELERR;
	return(-1);
    }
    if (readfds == 0)
	return(0);
#endif

#ifdef DEBUG
    logerr("vtrequest: read response","");
#endif
    /* read it */

    if (read_packet(-1,fd,(char *)&rpckt) != sizeof (struct vtresponse)) {
	if (error != (int *)0)
	    *error = E_RPKTERR;
	return(-1);
    }

#ifdef DEBUG
    logerr("vtrequest: checking packet","");
#endif
    /* check to make sure packet is good */

    if (rpckt.error != 0) {
	if (error != (int *)0)
	    *error = rpckt.error;
	return(-1);
    }

    /* set new address */

    if (setaddress(fd,rpckt.sap,rpckt.address) != 0) {
	if (error != (int *)0)
	    *error = E_ADDRERR;
	return(-1);
    }

#ifdef DEBUG
    logerr("vtrequest: send ok","");
#endif
    /* Send OK packet */

    if (send_packet(fd,(char *)0,0,VTOK) == -1) {
	if (error != (int *)0)
	    *error = E_XPKTERR;
	return(-1);
    }

    /* Set connection flag to TRUE */

    conflag = TRUE;

    if (rpckt.gatewaynode[0] == '\0')
	gatewaynode[0] = '\0';
    else
	strcpy(gatewaynode,rpckt.gatewaynode);

    /* successful return */

    return(1);
}
