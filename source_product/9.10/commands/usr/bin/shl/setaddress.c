/* @(#) $Revision: 29.1 $ */   
#include <netio.h>
#include "ptyrequest.h"

setaddress(fd,dsap,daddr)
    int fd;
    short dsap;
    char *daddr;
{
    extern struct fis _vtfisbuf;

    /* Log the destination sap */

    _vtfisbuf.vtype   = INTEGERTYPE;
    _vtfisbuf.reqtype = LOG_DSAP;
    _vtfisbuf.value.i = dsap;
    if (ioctl(fd,NETCTRL,&_vtfisbuf) != 0)
	return(-1);

    /* log destination address */

    _vtfisbuf.reqtype = LOG_DEST_ADDR;
    _vtfisbuf.vtype   = ADDRSIZE;
    memcpy(_vtfisbuf.value.s,daddr,ADDRSIZE);

    if (ioctl(fd,NETCTRL,&_vtfisbuf) != 0)
	return(-1);

    return(0);
}
