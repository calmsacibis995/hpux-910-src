/* HPUX_ID: @(#) $Revision: 66.3 $  */
#include <netio.h>
#include "rbootd.h"
#include "rmp_proto.h"
#include <errno.h>

/* setuplan:  sets up LAN to accept requests */

void
setuplan(lanfd)
    int lanfd;
{
    struct fis arg;	/* buffer for ioctl requests */

    /* set up IEEE 802.3 source SAP for RMP requests */

    arg.reqtype = LOG_XSSAP;
    arg.vtype = INTEGERTYPE;
    arg.value.i = SSAP;
    if (ioctl(lanfd,NETCTRL,&arg) != 0)
	errexit("Cannot set Extended IEEE 802.3 source SAP for boot server requests, errno = %d\n", errno);

    /* Log X-DSAP for receiving */

    arg.reqtype = LOG_XDSAP;
    arg.vtype = INTEGERTYPE;
    arg.value.i = DSAP;
    if (ioctl(lanfd,NETCTRL,&arg) != 0)
	errexit("Cannot set Extended IEEE 802.3 destination SAP for boot server requests, errno = %d\n", errno);

    /* Set LAN card read cache */

    arg.reqtype = LOG_READ_CACHE;
    arg.vtype = INTEGERTYPE;
    arg.value.i = CACHE_PACKETS;
    if (ioctl(lanfd,NETCTRL,&arg) != 0)
	errexit("Cannot set LAN card read cache for boot server requests, errno = %d\n", errno);

    /* Add BOOTMULTICAST address to system multicast address list. */
    /* First we delete it (and don't bother checking the error).   */
    /* This is done because there is no way of differentiating     */
    /* whether an addition failed because the address was already  */
    /* in the system multicast address list or because of some     */
    /* other error.                                                */

    arg.reqtype = DELETE_MULTICAST;
    arg.vtype = ADDRSIZE;
    net_aton(arg.value.s,BOOTMULTICAST,ADDRSIZE);
    ioctl(lanfd,NETCTRL,&arg);
    arg.reqtype = ADD_MULTICAST;
    if (ioctl(lanfd,NETCTRL,&arg) != 0)
	errexit("Cannot add BOOTMULTICAST address to system multicast address list, errno = %d\n", errno);
}

#ifdef DTC
/*
 * setupdtclan() : sets up LAN to send replies to DTC
 */
void
setupdtclan(dtclanfd)
int dtclanfd;
{
    struct fis arg;	/* buffer for ioctl requests */

    /*
     * For a packet to a DTC the XSSAP is set to be 0x0609.
     * This is because the DTC expects to receive packets with
     * XDSAP AND XSSAP to be 0x0609.
     */
    arg.reqtype = LOG_XSSAP;
    arg.vtype   = INTEGERTYPE;
    arg.value.i = DSAP;	    /* DTC expects this */

    if (ioctl(dtclanfd, NETCTRL, &arg) != 0)
	errexit("Cannot set Extended IEEE 802.3 source SAP for DTC boot server requests, errno = %d\n", errno);

    /* log X-DSAP for receiving */

    arg.reqtype = LOG_XDSAP;
    arg.vtype   = INTEGERTYPE;
    arg.value.i = DSAP;
    if (ioctl(dtclanfd, NETCTRL, &arg) != 0)
	errexit("Cannot set Extended IEEE 802.3 destination SAP for DTC boot server requests, errno = %d\n", errno);
}
#endif /* DTC */
