/* HPUX_ID: @(#) $Revision: 66.2 $  */

/* Constants */

#define RMP_PROTO_VERSION 2

#define SESSION_TIMEOUT 600    /* Ten minutes (600 seconds) */

#define CACHE_PACKETS   4      /* packets to cache on lan card */

#define MAXDATASIZE     1482   /* 1497 - 8(RMP header) - 7(ExIEEE) */
#define MAXPACKSIZE	1497

/*
 * RMP Errors
 *
 * Not implemented:
 *	Lengthen time out = 5
 *
*/

/* Generic Errors */

#define NOERROR		0	/* No Error */
#define BADPACKET	27 	/* Bad packet detected */
#define ABORT		3 	/* Abort this operation */

/* Send Errors 			*/
/* Boot Reply Errors		*/

#define	BUSY		4 	/* Server Busy */

#ifdef DTC
#define DTCCANNOTOPEN	2	/* Filename specified by DTC cannot be opened*/
#endif /* DTC */

#define	SFILENOTFOUND	16	/* Filename specified does not exist */
#define SCANNOTOPEN	17	/* Filename cannot be opened */
#define	DFILENOTFOUND	18	/* Default filename does not exist */
#define DCANNOTOPEN	19	/* Default filename cannot be opened */

/* SSAP and DSAPs for RMP */

#define SSAP      1544
#define DSAP      1545
#define LOG_XSSAP  801   /* eventually add to netio.h (if supported) */
#define LOG_XDSAP  802   /* eventually add to netio.h (if supported) */

/* HP LAN boot multicast address */
#define BOOTMULTICAST	"0x090009000004"

#define PROBESID        -1   /* Sid of probe */

/*
 * RMP Message Types, value put in type field of packets
 */
#define BOOT_REQUEST	1
#define BOOT_REPLY	129
#define READ_REQUEST	2
#define READ_REPLY	130
#define BOOT_COMPLETE	3

#ifdef DTC
#define INDICATION_REQUEST	47
#define INDICATION_REPLY	175
#endif /* DTC */

/* Read Reply Errors */

#define ENDOFFILE	2	/* End of File encountered */
#define BADSID		25	/* Bad session ID detected */

/*
 * PACKET STRUCTURE DEFINITIONS.
 *     seqno and offset fields are declared as character arrays instead
 *     of longs, since they are not on four byte boundaries.
 *     HPPA architecture requires alignment of longs on four byte
 *     boundaries, so we get around the problem this way.
 */

/*
 * Boot Request
 */
#define MACH_TYPE_LEN 20
#define BOOTREQUEST_NOFILE_SIZE 30 /* Size without flength field
				      (i.e. zero length filename). */

typedef struct
{
    unsigned char  type;            /* packet type */
    char  error;                    /* return code */
    char  seqno[4];                 /* sequence number */
    short sid;                      /* session ID */
    short version;                  /* version of RMP software */
    char  mach_type[MACH_TYPE_LEN]; /* hardware type of machine */
    unsigned char flength;          /* Length of filename */
    char  filename[MAXPATHBYTES];   /* name of req'd file (var. length
				       determined by size of packet) */
} boot_request;

struct pinfo
{
    int lanfd;
    int packetsize;
    char fromaddr[ADDRSIZE];
};


/*
 * Boot Reply
 */

/*
 * BOOTREPLYSIZE macro handles semantics properly for zero length file
 * name (flength field doesn't even get transmitted).
 */
#define BOOTREPLYSIZE(size) (size == 0 ? 10 : 11+size)

typedef struct
{
    unsigned char  type;            /* packet type */
    char  error;                    /* return code */
    char  seqno[4];                 /* sequence number */
    short sid;                      /* session ID */
    short version;                  /* version of RMP software */
    unsigned char flength;          /* Length of filename (or probe) */
    char  name[MAXPATHBYTES];       /* nodename (probe) or filename */
} boot_reply;

/*
 * Read Request
 */
typedef struct
{
    unsigned char  type;            /* packet type */
    char  error;                    /* return code */
    char  offset[4];                /* offset */
    short sid;                      /* session ID */
    short size;                     /* size of request */
} read_request;

/*
 * Read Reply
 */
#define READREPLYSIZE(size)     (8+size)

typedef struct
{
    unsigned char  type;            /* packet type */
    char  error;                    /* return code */
    char  offset[4];                /* offsetc */
    short sid;                      /* session ID */
    char data[MAXDATASIZE];         /* data pointer (variable size
				       determined by packet length) */
} read_reply;

#ifdef DTC
/*
 * DTC boot request
 */
#define DTCFILENAMESIZE		20

typedef struct
{
    char port_status[2];	    /* status of ports on the board */
    char selftest;		    /* selftest status              */
    unsigned char connector_type;   /* type of port direct connect
				       etc. */
} sic_status;

typedef struct
{
    unsigned char type;		    /* packet type */
    char error;			    /* return code */
    char seqno[4];		    /* sequence number */
    short sid;			    /* session id */
    short version;		    /* version of RMP software */
    char mach_type[MACH_TYPE_LEN];  /* hardware type of machine */
    char filename[DTCFILENAMESIZE]; /* File code specified as an ASCII
				       string and used to identify
				       download file */
    sic_status board[6];	    /* data about each board is given
				       by struct sic_status */
    unsigned char pad[12];	    /* unsused */
} dtcboot_request;

/*
 * Indication Request
 */
typedef struct
{
    unsigned char type;		    /* packet type */
    char error;			    /* return code */
    char seqno[4];		    /* sequence number */
    short sid;			    /* session ID */
    unsigned char pad1[4];	    /* reserved for extension */
    short version;		    /* senders version of RMP software*/
    unsigned char canonical_addr[2];
    unsigned char pad2[4];	    /* reserved for extension */
    short func_type;		    /* DTC function */
    short function;	            /* function code, 33 for Process
				       Indication */
    short sub_func_obj;		    /* subfunction code, [1-4] */
    unsigned char rec_len[4];	    /* function record length, [0,8] */
    char func_rec[8];		    /* selftest time */
} ind_request;

/*
 * Indication Reply
 */
typedef struct
{
    unsigned char type;		    /* packet type */
    char error;			    /* return code - 0 */
    char seqno[4];		    /* sequence number, matches req. */
    short sid;			    /* session ID, matches request */
    unsigned char pad1[4];	    /* reserved for extension */
    short version;		    /* senders version of RMP software*/
    unsigned char canonical_addr[2]; /* matches request */
    unsigned char pad2[4];	    /* reserved for extension */
    short func_type;		    /* DTC function, matches request */
    short function;	         /* function code, matches request */
    short sub_func_obj;		    /* subfunction code, matches req. */
    unsigned char rec_len[4];	    /* function record length - 0 */
} ind_reply;

/*
 * nmmsg
 */

typedef struct
{
    unsigned char type;
    char returncode;
    char seqno[4];
    short sid;
    short version;
} bootrep;

typedef struct
{
    unsigned char type;
    char returncode;
    char seqno[4];
    short sid;
} bootrdrep;

typedef struct
{
    unsigned char type;
    char returncode;
    char seqno[4];
    short sid;
} bootcmp;

typedef struct
{
    char msg_num[4];			/* message number */
    unsigned char dxsap[2];		/* extended dsap */
    unsigned char sxsap[2];		/* extended ssap */
    unsigned char phy_addr[6];		/* 802.3/Ethernet address */
    short len;				/* length of pkt + xsaps */
    union {
        char landata[1490];		/* 1490=MAXDATSIZE from host
					   host based dtcmgr code. */
	dtcboot_request bootreqpkt;
	bootrep bootreppkt;
	bootrdrep bootrdreppkt;
	ind_request indreqpkt;
	bootcmp bootcmppkt;
    } pkt;
} nmmsg;
#endif /* DTC */
