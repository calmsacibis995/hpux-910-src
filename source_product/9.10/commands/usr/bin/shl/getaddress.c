/* @(#) $Revision: 29.1 $ */   
#include <netio.h>
#include "ptyrequest.h"

extern void logerr();

char *
getaddress(fd)
    int fd;
{
    static char addrbuf[2 * ADDRSIZE + 3];
    extern char *net_ntoa();
    extern struct fis _vtfisbuf;

    _vtfisbuf.reqtype = LOCAL_ADDRESS;

    if (ioctl(fd,NETSTAT,&_vtfisbuf) != 0) {
	logerr("ioctl to get local address failed","");
	return((char *)0);
    }
    net_ntoa(addrbuf,_vtfisbuf.value.s,_vtfisbuf.vtype);
    return(addrbuf);
}
