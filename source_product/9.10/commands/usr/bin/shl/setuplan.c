/* @(#) $Revision: 29.1 $ */     
#include <netio.h>
#include <sys/errno.h>
#include "ptyrequest.h"

extern int errno;
extern void logerr();

setuplan(fd)
    int fd;
{
    int sap;
    extern char *net_aton();
    extern struct fis _vtfisbuf;

    _vtfisbuf.vtype   = INTEGERTYPE;
    _vtfisbuf.reqtype = LOG_SSAP;

    /* Allocate an IEEE802 sap */

    for (sap=VTCOMTYPE; sap < (VTCOMTYPE + 4 * MAXVTCOM); sap += 4) {
	_vtfisbuf.value.i = sap;
	if (ioctl(fd,NETCTRL,&_vtfisbuf) == 0) {
	    break;
	}
    }
    if (sap == (VTCOMTYPE + 4 * MAXVTCOM)) {
	if (errno == ENOMEM)
	    logerr("Out of lan memory","");
	else
	    logerr("Maximum local in/out vt connections exceeded","");
	return(-1);
    }

    /* Initialize packet cache value */

    _vtfisbuf.reqtype = LOG_READ_CACHE;
    _vtfisbuf.value.i = CACHE_PACKETS;
    if (ioctl(fd,NETCTRL,&_vtfisbuf) != 0) {
	logerr("Out of lan memory","");
	return(-1);
    }

#ifdef NOSELECT
    _vtfisbuf.reqtype = LOG_READ_TIMEOUT;
    _vtfisbuf.value.i = 20000;
    if (ioctl(fd,NETCTRL,&_vtfisbuf) != 0) {
	logerr("Could not set read timeout on lan device","");
	return(-1);
    }
#endif

    return(sap);
}

