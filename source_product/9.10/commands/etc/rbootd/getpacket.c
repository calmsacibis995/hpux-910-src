/* HPUX_ID: @(#) $Revision: 70.2 $  */
#include <stdio.h>
#include <errno.h>
#include <netio.h>

#include "rbootd.h"
#include "rmp_proto.h"

static int head = 0;
static int tail = 0;
static int qfullflag = FALSE;

extern int errno;
extern int nlandevs;
extern int lanfds[MAXLANDEVS];
extern int MAXSESSIONS;
extern boot_request *packet_pool;
extern struct pinfo *packetinfo_pool;

void readpackets();
void getsrcaddr();

/*
 * qempty --
 *    returns TRUE if queue is empty, FALSE is not empty
 */
int
qempty()
{
    return !qfullflag && head == tail;
}

/*
 * getpacket --
 *    Removes a packet from the buffer pool and returns the lan fd
 *    that the packet came in on.  getpacket also returns the source
 *    link address.
 *    (It will only be valid if packet type == BOOT_REQUEST).
 *
 *    Returns:
 *       lanfd if no error.
 *       -1    if error.
 */
int
getpacket(packet, src, packetsize)
boot_request *packet;
char *src;
int packetsize;
{
#ifdef DEBUG
    extern void print_packet();
    extern char *net_ntoa();
    extern int loglevel;
#endif /* DEBUG */

    register struct pinfo *pinfoptr;
    int old_head;
    char srcaddr[LADDRSIZE];

    if (qempty())
	return -1;

    /*
     * Save the index of the current head and advance to the next
     * element.
     */
    old_head = head;
    if (++head == MAXSESSIONS)
	head = 0;
    qfullflag = FALSE;

    pinfoptr = &packetinfo_pool[old_head];

    /*
     * If the packet is too large, drop it on the floor.  We log
     * a message at level 3 so as to not fill the log file with
     * lots of these messages unless the user wants it to
     */
    if (pinfoptr->packetsize > packetsize)
    {
	/*
	 * We mark broadcast requests by setting the broadcast bit
	 * in the source address.  For logging purposes, we want to
	 * clear this bit.
	 */
	pinfoptr->fromaddr[0] &= ~0x01;
	log(EL3, "Dropped packet from %s, too long (%d bytes)\n",
	    net_ntoa(srcaddr, pinfoptr->fromaddr, ADDRSIZE),
	    pinfoptr->packetsize);
	return -1;
    }

    memcpy((char *)packet, (char *)&packet_pool[old_head],
	    pinfoptr->packetsize);
    memcpy(src, pinfoptr->fromaddr, ADDRSIZE);

#ifdef DEBUG
    log(EL9, "Read from buffer RMP_HEADER:\n");
    log(EL9, "    src addr   %s\n",
	    net_ntoa(srcaddr, pinfoptr->fromaddr, ADDRSIZE));
    if (loglevel >= EL9)
	print_packet(packet, pinfoptr->packetsize);
#endif /* DEBUG */

    return pinfoptr->lanfd;
}

void
readpackets(lanfd, dev_name)
int lanfd;
char *dev_name;
{
    register boot_request *packet;
    register struct pinfo *pinfoptr;
    int ret;

    /*
     * Don't read anything if we don't have room on queue
     */
    if (qfullflag)
	return;

    do
    {
	packet = &packet_pool[tail];
	pinfoptr = &packetinfo_pool[tail];
	if ((ret = read(lanfd, (char *)packet, MAXPACKSIZE)) <= 0)
	{
	    if (ret == 0 || errno == EWOULDBLOCK)
		continue;
	    if (errno != EINTR)
		log(EL1, "Bad read in getpacket.\n");
	    else
		log(EL1, "Dropped packet due to read error\n");
	    continue;
	}

	/*
	 * Assign the lanfd and packetsize as soon as possible, so
	 * that the logging routines below have access to this
	 * information.
	 */
	pinfoptr->lanfd = lanfd;
	pinfoptr->packetsize = ret;

	/*
	 * Get source address.  We get the source address of all
	 * packets.
	 */
	getsrcaddr(lanfd, pinfoptr->fromaddr);

#ifdef DEBUG
	if (loglevel >= EL5)
	{
	    extern void print_packet();
	    extern char *net_ntoa();
	    extern int loglevel;

	    char srcaddr[LADDRSIZE];

	    log(EL5, "Read from %s:\n", dev_name);
	    log(EL5, "    src addr   %s\n",
		net_ntoa(srcaddr, pinfoptr->fromaddr, ADDRSIZE));
	    print_packet(packet, pinfoptr->packetsize);
	}
#endif /* DEBUG */

#ifdef DTC
	if (packet->type != BOOT_REQUEST &&
	    packet->type != READ_REQUEST &&
	    packet->type != BOOT_COMPLETE &&
	    packet->type != INDICATION_REQUEST)
	{
	    log(EL2, "Dropped packet: Unexpected packet type\n");
	    continue;
	}
#else
	if (packet->type != BOOT_REQUEST &&
	    packet->type != READ_REQUEST &&
	    packet->type != BOOT_COMPLETE)
	{
	    log(EL2, "Dropped packet: Unexpected packet type\n");
	    continue;
	}
#endif /* DTC */

#ifdef DTC
	/*
	 * If a boot request is from a DTC and its version number is
	 * not equal to either 1 or 3 then we drop the packet.
	 * For boot request packets which are not from a DTC, the
	 * packet is dropped if the version number is less than
	 * RMP_PROTO_VERSION (2).
	 */
	if (strncmp(packet->mach_type, DTCTYPE, DTC_LEN) == 0)
	{
	    if (strncmp(packet->mach_type, DTCMACHINETYPE, DTC_MACH_LEN) == 0 ||
		strncmp(packet->mach_type, DTC16TYPE, DTC_MACH_LEN) == 0)
	    {
		if ((packet->type == BOOT_REQUEST) &&
		    (packet->version != 1) &&
		    (packet->version != 3))
		{
		    log(EL2,
		       "Dropped packet: Bad RMP version (%d) for DTC\n",
		       packet->version);
		    continue;
		}

		/*
		 * Set flength field to zero if it wasn't transmitted
		 */
		if (ret == BOOTREQUEST_NOFILE_SIZE)
		    packet->flength = 0;
	    }
	    else
	    {
		log(EL2, "Dropped packet: Need memory extension card (%.7s)\n",
		    packet->mach_type);
		continue;
	    }
	}
	else
	{
	    if (packet->type == BOOT_REQUEST)
	    {
		if (packet->version < RMP_PROTO_VERSION)
		{
		    log(EL2, "Dropped packet: Bad RMP version (%d)\n",
			packet->version);
		    continue;
		}

		/*
		 * Set flength field to zero if it wasn't transmitted
		 */
		if (ret == BOOTREQUEST_NOFILE_SIZE)
		    packet->flength = 0;
	    }
	}
#else
	if (packet->type == BOOT_REQUEST)
	{
	    if (packet->version < RMP_PROTO_VERSION)
	    {
		log(EL2, "Dropped packet: Bad RMP version (%d)\n",
		    packet->version);
		continue;
	    }

	    /*
	     * Set flength field to zero if it wasn't transmitted
	     */
	    if (ret == BOOTREQUEST_NOFILE_SIZE)
		packet->flength = 0;
	}
#endif /* DTC */

#ifdef DTC
	/*
	 * Checking the version number in indication request packets.
	 */
	if (packet->type == INDICATION_REQUEST)
	{
	    ind_request *irequest = (ind_request *)packet;

	    if (irequest->version != 3)
	    {
		log(EL2,
		    "Dropped packet: Bad RMP version (%d) in indication request from DTC \n",
		    irequest->version);
		continue;
	    }
	}
#endif /* DTC */

#ifdef DEBUG
	log(EL5, "Got a good packet. size=%d\n", ret);
#endif DEBUG

	if (++tail == MAXSESSIONS)
	    tail = 0;

	qfullflag = (head == tail);

    } while (ret > 0 && !qfullflag);
}

/*
 * getsrcaddr --
 *    Copies source's link level address into srcaddr
 */
void
getsrcaddr(lanfd, srcaddr)
int lanfd;
char *srcaddr;  /* return value:  link address of requestor */
{
    struct fis arg;		/* buffer for ioctl requests */

    arg.reqtype = FRAME_HEADER;
    if (ioctl(lanfd, NETSTAT, &arg) != 0)
	errexit("Cannot read FRAME_HEADER from LAN device.\n");

    memcpy(srcaddr, &arg.value.s[ADDRSIZE], ADDRSIZE);

    /*
     * In some cases we need to know if this packet was specifically
     * addressed to us or not.  Rather than keeping extra information
     * around, we simply copy the "multicast" bit from the destination
     * address into the source address (a source can never have the
     * multicast bit set, so this does not lose information).
     */
    srcaddr[0] |= (arg.value.s[0] & 0x01);

#ifdef DEBUG
    {
	extern char *net_ntoa();
	char dstaddr[LADDRSIZE];

	log(EL8, "source address = %s\n",
	    net_ntoa(dstaddr, srcaddr, ADDRSIZE));
	log(EL8, "destination address = %s\n",
	    net_ntoa(dstaddr, arg.value.s, ADDRSIZE));
	log(EL8, "dsap = %d (0x%02x)\n",
	    arg.value.s[14], arg.value.s[14]);
	log(EL8, "ssap = %d (0x%02x)\n",
	    arg.value.s[15], arg.value.s[15]);
	log(EL8, "cntl = %d (0x%02x)\n",
	    arg.value.s[16], arg.value.s[16]);
    }
#endif DEBUG
}
