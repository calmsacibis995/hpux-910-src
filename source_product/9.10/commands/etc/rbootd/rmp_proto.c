/* HPUX_ID: @(#) $Revision: 70.2 $  */
#include <stdio.h>
#include <errno.h>
#include <netio.h>
#ifdef LOCAL_DISK
#include <sys/types.h>
#include <cluster.h>
#endif
#include "rbootd.h"
#include "rmp_proto.h"
#ifdef DTC
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif /* DTC */

#ifdef DTC
extern int MAXSESSIONS;
#endif
extern struct session *session_blk;

extern struct cinfo *get_client();
#ifdef LOCAL_DISK
extern int swap_server_ok();
#endif /* LOCAL_DISK */

void send_boot_reply();
static void sendpacket();
static void send_read_reply();
#ifdef DTC
static int opensocket();
static int send_msg();
static int check_config();
static void write_nmmsg();
static void send_indication_reply();
#endif /* DTC */
void rcv_server_identify(), rcv_filelist_request(), rcv_boot_request();
void rcv_read_request(), rcv_boot_complete();
#ifdef DTC
void rcv_indication_request();
#endif /* DTC */

/*
 * rcv_server_identify --
 *    Routine to handle incoming "identify" requests.
 */
void
rcv_server_identify(lanfd, src, brequest, hostname)
int lanfd;
char *src;
boot_request *brequest;
char *hostname;
{
    boot_reply breply;
    struct cinfo *client;
    long seqno;

    /*
     * Call config() to make sure our configuration
     * information is up to date.
     */
    config();

    /* Call get_client for this guys link address */
    if ((client = get_client(src, brequest->mach_type)) ==
						      (struct cinfo *)0)
	return;

#ifdef LOCAL_DISK
    /*
     * Make sure his swap server (if necessary) is available.
     */
    if (!swap_server_ok(client))
	return;

    /*
     * If this client is a diskless node and we are an auxilliary
     * swap server, we lie and pretend we are the rootserver.  This
     * is so the information printed by the bootrom is consistent
     * with the cluster to which you would belong after you are booted.
     * (FSDlj07059).
     */
    if (client->cnode_id > 0) /* request is from diskless client */
    {
	extern cnode_t cnodeid();
	static char server_name[15];

	if (cnodeid() > 1 && server_name[0] == '\0')
	{
	    struct cct_entry *server = getcccid(1); /* server is 1 */

	    if (server != (struct cct_entry *)0)
		strcpy(server_name, server->cnode_name);
	}

	if (server_name[0] != '\0')
	    strcpy(breply.name, server_name);
	else
	    strcpy(breply.name, hostname); /* just in case */
    }
    else
#endif /* LOCAL_DISK */
	strcpy(breply.name, hostname);

    breply.type = BOOT_REPLY;
    breply.error = 0;
    seqno = 0;
    memcpy(breply.seqno, (char *)&seqno, 4);
    breply.sid = 0;
    breply.version = brequest->version;
    breply.flength = strlen(breply.name);

    sendpacket(lanfd, src, &breply,
	BOOTREPLYSIZE(breply.flength), client);
}

void
rcv_filelist_request(lanfd, src, seqno, brequest)
int lanfd;
char *src;
long seqno;
boot_request *brequest;
{
    boot_reply breply;
    int flength;
    struct cinfo *client;
    char *bootfilename;
    extern long config_time;
    extern char *getnbootfile();

    /*
     * Call config() to make sure our configuration
     * information is up to date.
     */
    config();

    /* Call get_client for this guys link address */
    if ((client = get_client(src, brequest->mach_type)) ==
						      (struct cinfo *)0)
	return;

#ifdef LOCAL_DISK
    /*
     * Make sure his swap server (if necessary) is available.
     */
    if (!swap_server_ok(client))
	return;
#endif /* LOCAL_DISK */

    breply.type = BOOT_REPLY;
    memcpy(breply.seqno, (char *)&seqno, 4);
    breply.sid = 0;
    breply.version = brequest->version;

    if (seqno > 1 && client->timestamp < config_time)
    {
	/*
	 * Send an ABORT if we have reconfigured since this client has
	 * asked for the first boot file and the client is asking for
	 * a boot file other than the first one.   Since this is
	 * considered a warning only, we punch the timestamp so that
	 * it won't happen again.
	 */
	PUNCH_TIMESTAMP(&(client->timestamp));
	breply.error = ABORT;
	flength = 0;
    }
    else
    {
	PUNCH_TIMESTAMP(&(client->timestamp));

	bootfilename = getnbootfile(client, seqno, brequest->mach_type);
	if (bootfilename == (char *)0)
	{
	    breply.error = DFILENOTFOUND;
	    flength = breply.flength = 0;
	}
	else
	{
	    breply.error = 0;
	    strcpy(breply.name, bootfilename);
	    flength = breply.flength = strlen(bootfilename);
	    breply.name[flength] = '\0';
	}
    }
    sendpacket(lanfd, src, &breply, BOOTREPLYSIZE(flength), client);
}

/*
 * rcv_boot_request --
 *    Routine to handle incoming boot requests.
 */
void
rcv_boot_request(lanfd, src, seqno, brequest)
int lanfd;
char *src;
long seqno;
boot_request *brequest;		/* boot request packet */
{
    struct cinfo *client;  /* client struct of requestor.  */
    int ret;		   /* return code to be used in boot reply */

    /*
     * Call config() to make sure our configuration
     * information is up to date.
     */
    config();

    /*
     * Call get_client for this guys link address
     */
    if ((client = get_client(src, brequest->mach_type)) ==
						      (struct cinfo *)0)
	return;

#ifdef LOCAL_DISK
    /*
     * Make sure his swap server (if necessary) is available.
     */
    if (!swap_server_ok(client))
	return;
#endif /* LOCAL_DISK */

    ret = open_session(client, brequest);

#ifdef DTC
    /*
     * if reply is for a DTC do
     * 1) if return from open_session is equal to BUSY then reset
     *    ret = ABORT as DTC does not understand a return code of BUSY.
     * 2) send struct nmmsg to DTCNMD.
     */
    if (client->cl_type == BF_DTC)
    {
	if (ret == BUSY)
	    ret = ABORT;

	/* write nmmsg to rbootd_dtc.log file */
	write_nmmsg(brequest, src);

	/*
	 * if ret != ABORT and if it is the first boot request packet
	 * from the DTC then check the configuration of the DTC.
	 * brequest->error specifies the number of the boot request
	 * for download from the DTC.
	 */
	if (ret != ABORT && brequest->error == 0)
	{
	    if (check_config(brequest, client) != 0)
		ret = ABORT;	/* A bad configuration */
	}
    }
#endif /* DTC */

    send_boot_reply(lanfd, client, brequest, seqno, ret);
}

void
rcv_read_request(lanfd, rrequest, src)
int lanfd;
read_request *rrequest;
char *src;
{
    register int sid = rrequest->sid;
    int ret;
    char dbuf[MAXDATASIZE];
    long offset;

#ifdef DTC
{
    struct cinfo *client;

    /*
     * Call get_client for this guy's link address
     */
    if ((client = get_client(src, NULL)) == (struct cinfo *)0)
	return;

    /*
     * On sending a boot reply to a DTC, the boot reply is sent with a
     * sid equal to that in the boot request packet, when
     * brequest->sid != 0. (This is needed for reset_sic to work).
     * Hence the sid value in the read request packet (which matches
     * that in the boot reply packet that DTC receives) does not
     * specify the session that is related to the DTC. The session that
     * is related to this request is in the client block (client->sid).
     * Hence client->sid is obtained for this request and that is
     * passed to update session.
     */
    if (client->cl_type == BF_DTC)
    {
	if (sid == client->dtcsid &&    /* For robust checking */
	    client->dtcsid != 0)
	    sid = client->sid;
    }
}
#endif /* DTC */

    /*
     * Try to update session
     */
    if ((ret = update_session(sid, src)) != NOERROR)
    {
#ifdef DEBUG
	log(EL5, "update_session returned an error.\n");
#endif DEBUG
	send_read_reply(lanfd, sid, dbuf, rrequest, ret, src);
	return;
    }

    if (rrequest->size > MAXDATASIZE)
    {				/* check file size */
#ifdef DEBUG
	log(EL4, "Read Request size too large, sending %d bytes.\n",
		MAXDATASIZE);
#endif DEBUG
	rrequest->size = MAXDATASIZE;
    }

    /*
     * Move file pointer to appropriate offset (if needed)
     */
    memcpy((char *)&offset, rrequest->offset, 4);
    if (session_blk[sid].curoffset != offset)
    {
	if ((session_blk[sid].curoffset =
			  lseek(session_blk[sid].bfd, offset, 0)) == -1)
	{
	    log(EL1, "Lseek error accessing bootfile.\n");
	    send_read_reply(lanfd, sid, dbuf, rrequest, ABORT, NULL);
	    return;
	}
    }

    /*
     * Read data
     */
    if ((ret = read(session_blk[sid].bfd, dbuf, rrequest->size)) <= 0)
    {
	if (ret == 0)
	    send_read_reply(lanfd, sid, dbuf, rrequest, ENDOFFILE, NULL);
	else
	    send_read_reply(lanfd, sid, dbuf, rrequest, ABORT, NULL);
	return;
    }

    /*
     * Check for short read and fix length in rrequest->size so
     * send_reply copys correct length.
     */
    if (ret < rrequest->size)
	rrequest->size = ret;

    session_blk[sid].curoffset += ret;
    send_read_reply(lanfd, sid, dbuf, rrequest, NOERROR, NULL);
}

void
rcv_boot_complete(bcomplete, from_addr)
boot_request *bcomplete;
char *from_addr;
{
#ifdef DTC
    /*
     * Ensure that sid is valid before checking the client type.
     */
    if (bcomplete->sid >= 1 && bcomplete->sid <= MAXSESSIONS)
    {
	/*
	 * if boot_complete packet is from a DTC then send nmmsg to
	 * DTCNMD.
	 */
	struct cinfo *client = session_blk[bcomplete->sid].client;

	if (client->cl_type == BF_DTC)
	    write_nmmsg(bcomplete, from_addr);
    }
#endif /* DTC */
    close_session(bcomplete->sid, from_addr);
}

void
send_boot_reply(lanfd, client, brequest, seqno, returncode)
int lanfd;
struct cinfo *client;
boot_request *brequest;
long seqno;
int returncode;
{
    register char *s;
    boot_reply breply;
    int flength;

#ifdef DEBUG
    if (returncode == 0)
	log(EL4, "Sending boot reply, client = %s\n", client->name);
    else
	log(EL4, "Sending boot reply, no client\n");
#endif DEBUG

    breply.type = BOOT_REPLY;
    breply.error = returncode;
    memcpy(breply.seqno, (char *)&seqno, 4);
    breply.version = brequest->version;

    if (returncode != 0)
    {
	/*
	 * Don't need to set breply.name since flength field
	 * dosen't get transmitted if flength is zero.
	 */
	breply.sid = 0;
	flength = 0;
    }
    else
    {
	breply.sid = client->sid;
#ifdef DTC
	/*
	 * The DTC sends a boot request packet to rbootd to either
	 * initiate a download sequence or to reset a SIC.
	 * For download case: All boot requests in the download process
	 * have a sid = 0. The DTC accepts boot reply packets with any
	 * value for the sid other than 0.
	 * For the reset sic case: The following sequence takes place
	 * before a boot request is sent to rbootd.
	 *
	 * host manager		DTC		   rbootd
	 *
	 *  Obj. Req (sid=x)--->
	 *		   Ind. Req. (sid=x)--->
	 *				   <---Ind. Reply(sid=x)
	 *		   Boot Req. (sid=x)--->
	 *				   <---Boot Reply(sid must be x)
	 *
	 * The sid value in all the boot_request packets in the
	 * reset_sic case matches the sid value in the object request
	 * packet that the DTC receives. The DTC then accepts boot
	 * reply packets which have a sid equal to that in the Object
	 * request packet that it received.
	 *
	 * The DTC thinks of a session from the receipt of an
	 * object_request (reset_sic) or on sending a boot_request
	 * packet with the error field = 0 and sid = 0 (power up DTC).
	 *
	 * For a boot reply packet to a DTC we do not care about the
	 * field breply.name as this does not get transmitted.
	 */
	if (brequest->sid != 0 && client->cl_type == BF_DTC)
	    breply.sid = brequest->sid;

	/*
	 * If the above "if statement" gets executed then a small
	 * amount of preprocessing has to be done on receiving a read
	 * request packet from a DTC and before making a call to update
	 * session.
	 */
#endif /* DTC */

	if (brequest->flength != 0)
	{
	    s = brequest->filename;
	    flength = breply.flength = brequest->flength;

	    /*
	     * Sometimes we get extra null bytes on the end of the
	     * name from the bootrom. So we just return exactly what
	     * we got.
	     */
	    memcpy(breply.name, s, flength);
	}
	else
	{
	    s = client->bootlist->scanname;
	    strcpy(breply.name, s);
	    flength = breply.flength = strlen(s);
	}
    }
#ifdef DTC
    /*
     * If the packet is being sent to a DTC then send a packet with a
     * length of 10 octets. Also send nmmsg to DTCNMD.
     */
    if (client->cl_type == BF_DTC)
    {
	breply.flength = 0; /* for printpacket()'s benefit */
	sendpacket(lanfd, client->linkaddr, &breply,
	    BOOTREPLYSIZE(0), client);
	write_nmmsg(&breply, client->linkaddr);
    }
    else
    {
	sendpacket(lanfd, client->linkaddr, &breply,
	    BOOTREPLYSIZE(flength), client);
    }
#else
    sendpacket(lanfd, client->linkaddr, &breply,
	BOOTREPLYSIZE(flength), client);
#endif /* DTC */
}

static void
send_read_reply(lanfd, sid, buf, rrequest, returncode, src)
int lanfd;
int sid;
char *buf;
read_request *rrequest;
int returncode;
char *src;  /* NULL or LLA of destination if returncode==BADSID */
{
    read_reply rreply;
    struct cinfo *client;

    if (returncode != BADSID)
	client = session_blk[sid].client;
    else
    {
	if ((client = get_client(src, NULL)) == (struct cinfo *)0)
	{
#ifdef DEBUG
	    extern char *net_ntoa();
	    char addrbuf[LADDRSIZE];

	    log(EL4, "send_read_reply, BADSID, can't get_client!\n");
	    log(EL4, "    source address = %s\n",
		net_ntoa(addrbuf, src, ADDRSIZE));
#endif	/* DEBUG */
	    return;
	}
    }
#ifdef DEBUG
    log(EL5, "Sending read reply, client: %s\n", client->name);
#endif DEBUG

    rreply.type = READ_REPLY;

#ifdef DTC
    /*
     * if the reply is for a DTC and if returncode is equal to BADSID
     * then set rreply.error to ABORT as DTC does not understand a
     * return code of BADSID.
     */
    if (returncode == BADSID && client->cl_type == BF_DTC)
	rreply.error = ABORT;
    else
	rreply.error = returncode;
#else
    rreply.error = returncode;
#endif /* DTC */

    memcpy(rreply.offset, rrequest->offset, 4);
    rreply.sid = sid;

#ifdef DTC
    /*
     * If reply is for a DTC and if sid = client->sid, then send reply
     * with a session id = client->dtcsid. (For reset_sic's benefit).
     */
    if (client->cl_type == BF_DTC)
    {
	if (client->sid == sid &&   /* For robust checking */
	    client->dtcsid != 0)
	{
	    rreply.sid = client->dtcsid;
	}
    }
#endif /* DTC */

    if (returncode)		/* no data */
	rrequest->size = 0;
    else
	memcpy(rreply.data, buf, rrequest->size);

    sendpacket(lanfd, client->linkaddr, &rreply,
	READREPLYSIZE(rrequest->size), client);
#ifdef DTC
    /*
     * If there was an error or on reaching EOF on a read and the reply
     * is for a DTC then send nmmsg to DTCNMD.
     */
    if (returncode != 0 && client->cl_type == BF_DTC)
	write_nmmsg(&rreply, client->linkaddr);
#endif /* DTC */
}

#ifdef DEBUG
void
print_packet(packet, pktlength)
boot_request *packet;  /* really any kind of packet */
int pktlength;
{
    boot_request *brequest = (boot_request *) packet;
    boot_reply   *breply   = (boot_reply *)   packet;
    read_request *rrequest = (read_request *) packet;
    read_reply   *rreply   = (read_reply *)   packet;
#ifdef DTC
    dtcboot_request *dtcbrequest = (dtcboot_request *) packet;
    ind_request     *irequest    = (ind_request *)     packet;
    ind_reply       *ireply      = (ind_reply *)       packet;
#endif /* DTC */
    char buf[MAXPATHLEN];
    char mtype[MACH_TYPE_LEN + 1];
    unsigned long ulong_val;

    log(EL0, "    packet len %d\n", pktlength);
    switch (packet->type)
    {
    case BOOT_REQUEST:
	memcpy(&ulong_val, brequest->seqno, 4);
	memcpy(mtype, brequest->mach_type, MACH_TYPE_LEN);
	mtype[MACH_TYPE_LEN] = '\0';

	log(EL0, "    type       BOOT_REQUEST\n");
	log(EL0, "    error      %d\n", brequest->error);
	log(EL0, "    seqno      %lu\n", ulong_val);
	log(EL0, "    sid        %d\n", brequest->sid);
	log(EL0, "    mach_type  \"%s\"\n", mtype);
#ifdef DTC
	if (strncmp(brequest->mach_type, DTCMACHINETYPE, DTC_MACH_LEN) == 0 ||
	    strncmp(brequest->mach_type, DTC16TYPE, DTC_MACH_LEN) == 0)
	{
	    int i;
	    extern void log_char();

	    log(EL0, "    version    %d\n",dtcbrequest->version);
	    strncpy(buf, dtcbrequest->filename, 20);
	    buf[20] = '\0';
	    log(EL0, "    filename   \"%s\"\n", buf);

	    for (i=0; i<=5; i++) {
		log(EL0, "    Board %d: port[0-7]flags  ", i);
		log_char(dtcbrequest->board[i].port_status[0]);
		log(EL0, "             port[8-15]flags ");
		log_char(dtcbrequest->board[i].port_status[1]);
		log(EL0, "             selftest-result %d\n",
		    dtcbrequest->board[i].selftest);
		log(EL0, "             connector_type  0x%x\n",
		    dtcbrequest->board[i].connector_type);
	    }
	    break;
	}
#endif /* DTC */
	if (pktlength == BOOTREQUEST_NOFILE_SIZE)
	    log(EL0, "    flength    <not present>\n");
	else
	{
	    log(EL0, "    flength    %d\n", brequest->flength);
	    strncpy(buf, brequest->filename, brequest->flength);
	    buf[brequest->flength] = '\0';
	    log(EL0, "    filename   \"%s\"\n", buf);
	}
	break;
    case BOOT_REPLY:
	memcpy(&ulong_val, breply->seqno, 4);

	log(EL0, "    type       BOOT_REPLY\n");
	log(EL0, "    error      %d\n", breply->error);
	log(EL0, "    seqno      %lu\n", ulong_val);
	log(EL0, "    sid        %d\n", breply->sid);
	log(EL0, "    version    %d\n", breply->version);
	log(EL0, "    flength    %d\n", breply->flength);
	strncpy(buf, breply->name, breply->flength);
	buf[breply->flength] = '\0';
	log(EL0, "    name       \"%s\"\n", buf);
	break;
    case READ_REQUEST:
	memcpy(&ulong_val, rrequest->offset, 4);

	log(EL0, "    type       READ_REQUEST\n");
	log(EL0, "    error      %d\n", rrequest->error);
	log(EL0, "    offset     %lu\n", ulong_val);
	log(EL0, "    sid        %d\n", rrequest->sid);
	log(EL0, "    size       %d\n", rrequest->size);
	break;
    case READ_REPLY:
	memcpy(&ulong_val, rreply->offset, 4);

	log(EL0, "    type       READ_REPLY\n");
	log(EL0, "    error      %d\n", rreply->error);
	log(EL0, "    offset     %lu\n", ulong_val);
	log(EL0, "    sid        %d\n", rreply->sid);
	break;
    case BOOT_COMPLETE:
	memcpy(&ulong_val, brequest->seqno, 4);

	log(EL0, "    type       BOOT_COMPLETE\n");
	log(EL0, "    error      %d\n", brequest->error);
	log(EL0, "    *unused*   %lu\n", ulong_val);
	log(EL0, "    sid        %d\n", brequest->sid);
	break;
#ifdef DTC
    case INDICATION_REQUEST:
	memcpy(&ulong_val, irequest->seqno, 4);

	log(EL0, "    type       INDICATION_REQUEST\n");
	log(EL0, "    error      %d\n", irequest->error);
	log(EL0, "    seqno      %lu\n", ulong_val);
	log(EL0, "    sid        %d\n", irequest->sid);
	log(EL0, "    *unused*   %02x-%02x-%02x-%02x\n",
	    irequest->pad1[0], irequest->pad1[1],
	    irequest->pad1[2], irequest->pad1[3]);
	log(EL0, "    version    %d\n", irequest->version);
	log(EL0, "canonical_addr %02x-%02x\n",
	    irequest->canonical_addr[0], irequest->canonical_addr[1]);
	log(EL0, "    *unused*   %02x-%02x-%02x-%02x\n",
	    irequest->pad2[0], irequest->pad2[1],
	    irequest->pad2[2], irequest->pad2[3]);
	log(EL0, "    func_type  %d\n", irequest->func_type);
	log(EL0, "    function   %d\n", irequest->function);
	log(EL0, "sub fun/object %d\n", irequest->sub_func_obj);

	memcpy(&ulong_val, irequest->rec_len, 4);
	log(EL0, "record length  %lu\n", ulong_val);

	memcpy(&ulong_val, irequest->func_rec, 4);
	log(EL0, "    pretime    %lu\n", ulong_val);

	memcpy(&ulong_val, &(irequest->func_rec[4]), 4);
	log(EL0, "    posttime   %lu\n", ulong_val);
	break;
    case INDICATION_REPLY:
	memcpy(&ulong_val, ireply->seqno, 4);

	log(EL0, "    type       INDICATION_REPLY\n");
	log(EL0, "    error      %d\n", ireply->error);
	log(EL0, "    seqno      %lu\n", ulong_val);
	log(EL0, "    sid        %d\n", ireply->sid);
	log(EL0, "    *unused*   %02x-%02x-%02x-%02x\n",
	    ireply->pad1[0], ireply->pad1[1],
	    ireply->pad1[2], ireply->pad1[3]);
	log(EL0, "    version    %d\n", ireply->version);
	log(EL0, "canonical_addr %02x-%02x\n",
	    ireply->canonical_addr[0], ireply->canonical_addr[1]);
	log(EL0, "    *unused*   %02x-%02x-%02x-%02x\n",
	    ireply->pad2[0], ireply->pad2[1],
	    ireply->pad2[2], ireply->pad2[3]);
	log(EL0, "    func_type  %d\n", ireply->func_type);
	log(EL0, "    function   %d\n", ireply->function);
	log(EL0, "sub fun/object %d\n", ireply->sub_func_obj);

	memcpy(&ulong_val, ireply->rec_len, 4);
	log(EL0, "record length  %lu\n", ulong_val);
	break;
#endif /* DTC */
    default:
	log(EL0, "print_packet: Bad packet type %d\n", packet->type);
    }
}
#endif /* DEBUG */

static void
sendpacket(lanfd, destaddr, packet, pktlength, client)
int lanfd;
char *destaddr;
read_reply *packet; /* really a variable size packet */
int pktlength;
struct cinfo *client;
{
    struct fis arg;		/* buffer for ioctl requests */
    int ret;

    /*
     * If the broadcast bit was set (indicating that the request
     * was from a broadcast address), clear it.  We do not have
     * to restore it, as nobody will ever need to know what this
     * bit was set to after this point.
     */
    destaddr[0] &= ~0x01;

#ifdef DEBUG
    {
	extern int loglevel;
	extern char *net_ntoa();
	char buf[LADDRSIZE];

	log(EL5, "Sending a packet to %s: \n",
	    net_ntoa(buf, destaddr, ADDRSIZE));

	if (loglevel >= EL5)
	    print_packet(packet, pktlength);
    }
#endif /* DEBUG */

#ifdef DTC
    /*
     * If packet is for a DTC then send the packet using the DTC file
     * descriptor (simply by changing lanfd to the corresponding
     * entry from the dtclanfds[] array).
     */
    {
	extern int dtclanfds[MAXLANDEVS];
	extern int lanfds[MAXLANDEVS];
	extern int nlandevs;
	int i;

	if (client->cl_type == BF_DTC)
	{
	    /* Obtain dtclanfd corresponding to lanfd */
	    for (i=0; i<=nlandevs; i++)
		if (lanfds[i] == lanfd)
		    break;
	    lanfd = dtclanfds[i];

#ifdef DEBUG
	    log (EL5,
		"Packet recvd on lanfd %d, reply on dtclanfd %d.\n",
		lanfds[i], dtclanfds[i]);
#endif
	}
    }
#endif /* DTC */

    /*
     * Log destination address
     */
    arg.reqtype = LOG_DEST_ADDR;
    arg.vtype = ADDRSIZE;
    memcpy(arg.value.s, destaddr, ADDRSIZE);
    if (ioctl(lanfd, NETCTRL, &arg) != 0)
	errexit("Cannot log destination address.\n");

    /*
     * Write out packet
     */
#ifdef DEBUG
    if ((ret = write(lanfd, (char *)packet, pktlength)) != pktlength)
	log(EL0, "Write returned %d in sendpacket(). errno=%d\n",
	    ret, errno);
    else
	log(EL5, "Sent packet successfully\n");
#else
    /*
     * Don't check error returns because we don't need to do anything
     * special if the write fails (i.e. the requester should try again)
     */
    write(lanfd, (char *)packet, pktlength);
#endif /* DEBUG */
}

#ifdef DTC

void
rcv_indication_request(lanfd, irequest, src)
int lanfd;
ind_request *irequest;
char *src;
{
    struct cinfo *client;

    /*
     * Call config() to make sure our configuration
     * information is up to date.
     */
    config();

    /*
     * Call get_client to make sure that he is a valid client.
     * Checking of version number is done when packet is received
     * by getpacket.
     */
    if ((client = get_client(src, NULL)) == (struct cinfo *)0)
	return;

    /* send nmmsg to DTCNMD on receiving an indication request */
    write_nmmsg(irequest, src);

    send_indication_reply(lanfd, irequest, src, client);

    /*
     * If the indication request packet is a post process ind. request
     * packet then we close the session as it indicates the completion
     * of Reset Sic.
     */
    if (irequest->sub_func_obj == INDREQ_POSTPROCESS)
	close_session(client->sid, src);
}

static void
send_indication_reply(lanfd, irequest, src, client)
int lanfd;
ind_request *irequest;
char *src;
struct cinfo *client;
{
    ind_reply  ireply;

    /*
     * build the indication reply packet and then call sendpacket to
     * send the reply packet.
     */
    ireply.type = INDICATION_REPLY;
    ireply.error = 0;
    memcpy(ireply.seqno, irequest->seqno, 4);
    ireply.sid = irequest->sid;
    memcpy(ireply.pad1, irequest->pad1, 4);
    ireply.version = irequest->version;
    memcpy(ireply.canonical_addr, irequest->canonical_addr, 2);
    memcpy(ireply.pad2, irequest->pad2, 4);
    ireply.func_type = irequest->func_type;
    ireply.function = irequest->function;
    ireply.sub_func_obj = irequest->sub_func_obj;
    memcpy(ireply.rec_len, (char *)0, 4);

    sendpacket(lanfd, src, &ireply, sizeof (ind_reply), client);
}

static void
write_nmmsg(packet, src)
boot_request *packet; /* really any kind of packet */
char *src;
{
    static int nmd_fd = -1;
    dtcboot_request *brequest  = (dtcboot_request *) packet;
    bootrep *breply	       = (bootrep *)	     packet;
    bootrdrep *rreply	       = (bootrdrep *)       packet;
    bootcmp *bcomplete	       = (bootcmp *)	     packet;
    ind_request *irequest      = (ind_request *)     packet;
    nmmsg entry;
    int x;
    short xsap;

    /*
     * build top part of entry
     */

    x = 0;
    memcpy(entry.msg_num, (char *)&x, 4);
    memcpy(entry.phy_addr, src, 6);

    switch (packet->type)
    {
    case BOOT_REQUEST:
	xsap = SSAP;  /* SSAP = 1544 = 0x0608 */
	memcpy(entry.dxsap, (unsigned char *)&xsap, 2);/*dxsap = sxsap*/
	memcpy(entry.sxsap, (unsigned char *)&xsap, 2);
	entry.len = sizeof(dtcboot_request) + 4;
	memcpy(entry.pkt.landata, brequest, sizeof(dtcboot_request));
	break;
    case BOOT_REPLY:
	xsap = DSAP; /* DSAP = 1545 = 0x0609 */
	memcpy(entry.dxsap, (unsigned char *)&xsap, 2);/*dxsap = sxsap*/
	memcpy(entry.sxsap, (unsigned char *)&xsap, 2);
	entry.len = sizeof(bootrep) + 4;
	memcpy(entry.pkt.landata, breply, sizeof(bootrep));
	break;
    case READ_REPLY:
	xsap = DSAP; /* DSAP = 1545 = 0x0609 */
	memcpy(entry.dxsap, (unsigned char *)&xsap, 2);/*dxsap = sxsap*/
	memcpy(entry.sxsap, (unsigned char *)&xsap, 2);
	entry.len = sizeof(bootrdrep) + 4;
	memcpy(entry.pkt.landata, rreply, sizeof(bootrdrep));
	break;
    case INDICATION_REQUEST:
	xsap = SSAP;  /* SSAP = 1544 = 0x0608 */
	memcpy(entry.dxsap, (unsigned char *)&xsap, 2);/*dxsap = sxsap*/
	memcpy(entry.sxsap, (unsigned char *)&xsap, 2);
	entry.len = sizeof(ind_request) + 4;
	memcpy(entry.pkt.landata, irequest, sizeof(ind_request));
	break;
    case BOOT_COMPLETE:
	xsap = SSAP;  /* SSAP = 1544 = 0x0608 */
	memcpy(entry.dxsap, (unsigned char *)&xsap, 2);/*dxsap = sxsap*/
	memcpy(entry.sxsap, (unsigned char *)&xsap, 2);
	entry.len = sizeof(bootcmp) + 4;
	memcpy(entry.pkt.landata, bcomplete, sizeof(bootcmp));
	break;
    default:
	log(EL1, "write_nmmsg: Bad packet type %d\n", packet->type);
    }

    /*
     * Sending the message built to dtcnmd. send_msg would fail on the
     * first attempt as the file descriptor has been initialized to -1.
     * A call to opensocket is then made after which send_msg is tried
     * again. We handle the case when we have a file descriptor and the
     * dtcnmd process has gone away.
     */
    if (nmd_fd == -1 || send_msg(nmd_fd, &entry, sizeof(nmmsg)) != 0)
    {
	/*
	 * Sending the message failed, lets (re)open the conncet to
	 * dtcnmd.
	 */
	if ((nmd_fd = opensocket()) >= 0)
	{
	    /*
	     * Try sending the message again
	     */
	    if (send_msg(nmd_fd, &entry, sizeof(nmmsg)) != 0)
	    {
#ifdef DEBUG
		log(EL5, "Unable to send message after opensocket succeeded\n");
#endif /* DEBUG */
		return;
	    }
	}
	else
	{
#ifdef DEBUG
	    log(EL5, "Unable to connect to DTCNMD\n");
#endif /* DEBUG */
	    return;
	}
    }

#ifdef DEBUG
    log(EL4, "Message sent to DTCNMD\n");
#endif /* DEBUG */

}

static int
check_config(packet, client)
dtcboot_request *packet;
struct cinfo *client;
{
    int i;
    char *slot_ptr;
    char selftest_board[6];
    char pathname[DTCPATHNAMESIZE];
    struct stat statbuf;

    /*
     * Get the selftest results from the boot request packet into the
     * array selftest_board.
     */
    for (i=0; i<=5; i++) {
	selftest_board[i] = packet->board[i].selftest;
    }

    strcpy(pathname, DTCMGRDIR);
    strcat(pathname, client->name);
    strcat(pathname, ".dtc/conf/slot");
    slot_ptr = pathname + strlen(pathname);
    strcpy(slot_ptr, "0/tioconf");

    for (i=0; i<=5; i++) {
	/*
	 * build pathname to tioconf, merely by replacing the
	 * slot number in pathname with the current slot number.
	 */
	*slot_ptr = i + '0';

	if (selftest_board[i] == 0) 		/* everything ok */
	{
	    if (stat(pathname, &statbuf) != -1) /*.../slotX/tioconf exists */
		continue;		/* check next board status */
	    else
	    {
	      log(EL1,
		"Good SIC(%d) present in DTC(%s) but not configured by user\n",
		  i, client->name);
		return -1;			/* BAD CONFIG */
	    }
	}
	else if (selftest_board[i] == 6) 	/* SIC is not present */
	{
	    if (stat(pathname, &statbuf) == -1) /*.../slotX/tioconf absent */
		continue;		/* check next board status */
	    else
	    {
		log(EL1,
		   "SIC(%d) not present in DTC(%s) but configured by user\n",
		    i, client->name);
		return -1;			/* BAD CONFIG */
	    }
	}
	else
	{
	    log(EL1,
		"BAD SIC(%d) in DTC(%s): selftest result = %d\n",
		i, client->name, selftest_board[i]);
	    log(EL1, "Download Process for DTC(%s) continued\n",
		client->name);
	}
    }

    log(EL3, "Configuration checking for DTC completed successfully\n");
    return 0;
}

static int
send_msg(fd, data, nbytes)
int fd;
char *data;
int nbytes;
{
    /*
     * send_msg sends a message on the given socket. On successful
     * completion it returns 0; returns -1 on sending less than
     * nbytes and for any other error case.
     */

    int ret;

    if ((ret = (write(fd, data, nbytes))) == nbytes)
	return 0;

    if (ret == -1)
    {
#ifdef DEBUG
	log(EL5, "Unable to send message. errno = %d\n", errno);
#endif /* DEBUG */
	return -1;
    }

#ifdef DEBUG
    log(EL5, "Unable to send entire message. %d vs %d\n", ret, nbytes);
#endif /* DEBUG */
    return -1;
}

static int
opensocket()
{
    /* opensocket opens a socket for SENDING an nmmsg */

    int sd;

    struct sockaddr_un sock_addr;

    if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
#ifdef DEBUG
	log(EL5, "opensocket(): socket call failed. errno = %d\n",
	    errno);
#endif /* DEBUG */
	return -1;
    }

    sock_addr.sun_family = AF_UNIX;
    strcpy(sock_addr.sun_path, IPC_NMDRBOOTD);

    /*
     * The socket is for sending nmmsg's, hence a call to connect is
     * made.
     */
    if (connect(sd, &sock_addr, (strlen(sock_addr.sun_path) + 2)) == -1)
    {
#ifdef DEBUG
	log(EL5, "Unable to connect to %s. errno = %d\n",
	    IPC_NMDRBOOTD, errno);
#endif /* DEBUG */
	close(sd);
	return -1;
    }

    return sd;
}

#endif /* DTC */
