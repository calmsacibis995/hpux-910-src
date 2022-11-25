/* @(#) $Revision: 64.2 $ */   
#include <sys/termio.h>
#include <sys/param.h>

/* 1) Various sizes and bounding constants */

#define PTYNAMLEN     128         /* Maximum absolute path length for a pty */
#define MAXARGSTRLEN  128         /* Maximum argument string length         */
#define POOLSIZE      256         /* Buffer space for parms to ptydaemon    */
#define USERLEN       12          /* Maximum length for a user name         */
#define PASSWORDLEN   12          /* Maximum length for a password.         */
#define NODENAMESIZE  20	  /* Maximum length for a nodename.         */
#define MAXVTCOM      16          /* Maximum # of in/out connections        */
#define ADDRSIZE      6           /* Number of bytes in network address     */
#define CACHE_PACKETS 4           /* Maximum number of packets to cache     */
#define NUMSEQ        16          /* Number of valid sequence numbers       */
#define MAXHOP        3           /* Maximum # of hops a vtrequest can take */
#define FIXEDSIZE     8           /* Size of a packet is FIXEDSIZE +        */
				  /*                     dlen + alen        */
#define MAXDATASIZE   1480
#define MAXPACKSIZE (FIXEDSIZE + MAXDATASIZE + CACHE_PACKETS)

/* 2) Various Fixed constants */

#define VTMULTICAST "0x01AABBCCBBAA" /* Multicast address for vt requests  */
#define LOGIN_PROGRAM "/bin/login"
#define PTYDAEMONPROG "/etc/ptydaemon"      /* Used by ftok() */
#define DEFAULTLANDEV  "/dev/ieee"
#define VTSERVERPROG   "/etc/vtserver"
#define VTSERVERARG0   "vtserver"
#define VTPROG         "/usr/bin/vt"
#define VTARG0         "vt"
#define VTGATEWAYPROG  "/etc/vtgateway"
#define VTGATEWAYARG0  "vtgateway"

#ifdef hp9000s200
#define setvec(vec, a) vec.sv_handler = a; vec.sv_mask = vec.sv_onstack = 0
#else
#define setvec(vec, a) vec.sv_handler = (int (*)())a; vec.sv_mask = vec.sv_onstack = 0
#endif

#define BLOCK_SIGCLD  (1 << ( SIGCLD  - 1))
#define BLOCK_SIGTERM (1 << ( SIGTERM - 1))
#define BLOCK_SIGHUP  (1 << ( SIGHUP  - 1))
#define BLOCK_SIGNALS (~(1 << (SIGILL - 1)) ) /* Allow deliberate core dump */
#define BLOCK_DEATHSIGS (BLOCK_SIGHUP | BLOCK_SIGTERM)
#define BLOCK_OTHERSIGS (BLOCK_SIGNALS & ~BLOCK_DEATHSIGS)

#define VTREQTYPE     0x90           /* IEEE802 sap for vt requests        */
#define VTCOMTYPE     0x94           /* first IEEE802 sap for vt Tx & Rx   */

#define REQID '<'      /* Identifier used in ftok() for request  queue */
#define RESID '>'      /* Identifier used in ftok() for response queue */

/* The following times are in seconds */

#define ALARM_TIME  10 /* Default time out for blocking functions      */
#define KEEPALIVE  20  /* Send an empty packet every KEEPALIVE seconds */
#define UUCP_TIMEOUT 360 /* Kill a uucp connection if no traffic.      */

/* The following times are in hundredths of a second */

#define RQ_TIMEOUT (1200 * KEEPALIVE + 2000)
#define SV_TIMEOUT (1800 * KEEPALIVE + 2000)
#define IN_TIMEOUT (300 * KEEPALIVE)  /* Initial timeout for vtserver until */
				      /* connection handshake has completed.*/

/* The following time is in microseconds */

#define SELECT_TIMEOUT  350000

#define FALSE 0
#define TRUE  1

#define ROOTUID  0
#define OTHERGID 1

/*  VT_MAJOR_VERSION contains the vt protocol version. This will allow   */
/*  a check to make sure that the requester and server are both speaking */
/*  the same protocol (as long as the initial request/response protocol  */
/*  hasn't changed).                                                     */

#define VT_MAJOR_VERSION 4

/*  VT_MINOR_VERSION contains a secondary version number. This is used   */
/*  to differentiate protocols that are compatible and therefore don't   */
/*  justify bumping VT_MAJOR_VERSION.                                    */

#define VT_MINOR_VERSION 5

/* 3) Flags for ptydaemon */

#define CREATE_UTMP 1  /* Create entry in /etc/utmp             */
#define CREATE_WTMP 2  /* Create entry in /etc/wtmp             */
#define RESPAWN     4  /* Respawn process when it dies          */
#define PRINTISSUE  8  /* Copy /etc/issue to the slave          */

/* 4) Flags for vtdaemon */

#define NOTTY       32 /* Turn off termio processing    */

/* 5) Packet Types for vtserver and vtdaemon. Types whose number is <=  */
/*    VTDATA will be stored for possible retransmit and must be acked.  */
/*    Also, sequence number checking will be done on types whose number */
/*    is <= VTDATA. Types whose number is > VTDATA will not be stored   */
/*    and sequence numbers will not be checked (i.e. sequence numbers   */
/*    should only be incremented when sending packets whose type is     */
/*    <= VTDATA). Make sure that this convention is kept in mind if new */
/*    packet types are added.                                           */

#define VTPOLLRESPONSE  1                /* poll response             */
#define VTBREAK         2                /* send a break              */
#define VTIOCTL         3                /* advisory ioctl            */
#define VTOK            4                /* Complete Handshake        */
#define VTSWITCH        5                /* Switch pty read mode.     */
#define VTSWITCHACK     6                /* Ack pty read mode switch  */
#define VTSTATRESPONSE  7                /* status response           */
#define VTFTREQUEST     8                /* File transfer request     */
#define VTFTDONE        9                /* File transfer completed   */
#define VTDATA          19               /* Data from/to vtserver     */
#define VTREQUEST       20               /* Request to vtdaemon       */
#define VTACK           21               /* Acknowledgement packet    */
#define VTQUIT          22               /* Terminate vt              */
#define VTDONE          23               /* Normal termination        */
#define VTRESPONSE      24               /* Response from vtserver    */
#define VTFTABORT       25               /* Abort File Transfer       */

/* 6) Request types (for vtdaemon) */

#define VTR_VTCON    1               /* vt connection request     */
#define VTR_POLL     2               /* vt poll request           */
#define VTR_GTWYPOLL 3               /* vt gateway poll           */
#define VTR_GTWYINFO 4               /* vt gateway information    */
#define VTR_MAX      4               /* MAX request type          */

/* 7) Mode definitions for doio() */

#define VTMODE        0 /* Modes for vt (0-2) */
#define VTCMDMODE     1
#define UUCPMODE      2
#define VTSERVERMODE  3 /* Modes for vtserver (3-5) */
#define VTSWITCHMODE  4
#define VTNRMODE      5

/* 8) Structures used by the ptydaemon */

struct ptyrequest {
	int  myuid;
	int  mygid;
	int  flags;
	int  spare1;
	short pathoffset;
	short pathlength;
	short argvoffset;
	short argvlength;
	short environoffset;
	short environlength;
	char pool[POOLSIZE];
	char user[USERLEN];
	char password[PASSWORDLEN];
};

#define REQUESTSIZE  sizeof(struct ptyrequest)

struct ptymsgreqbuf {
	long    mtype;    /* message type */
	char    mtext[REQUESTSIZE];                  /* message text */
};

struct ptyresponse {
	int     error;
	char    master[PTYNAMLEN];
	char    slave[PTYNAMLEN];
};

#define RESPONSESIZE sizeof(struct ptyresponse)

struct ptymsgresbuf {
	long    mtype;    /* message type */
	char    mtext[RESPONSESIZE];                 /* message text */
};

/* 9) Structures used by the vtdaemon and vtserver */

/* WARNING struct vtpacket is set up so that alignment considerations for */
/* both Series 200 & series 500 machines are taken into account. Changing */
/* this structure can mess up s200 - s500 communications if these align-  */
/* considerations are not taken into account.                             */

struct vtpacket {
    short dlen;                      /* Number of data bytes */
    short alen;                      /* Number of ack  bytes */
    char type;                       /* Type of packet       */
    char seqno;                      /* sequence number      */
    char rexmit;                     /* number of re-xmits   */
    char spare1;
    char data[MAXDATASIZE + CACHE_PACKETS];
};

struct vtrequest {
    short hopcount;                 /* vtrequest hop count   */
    short flags;                    /* flags to getpty()     */
    short reqtype;                  /* type of request       */
    short majorversion;             /* protocol version #    */
    short sap;                      /* sap of requester      */
    char fromaddr[ADDRSIZE];        /* address of requester  */
    char wantnode[NODENAMESIZE];    /* desired node          */
    char mynode[NODENAMESIZE];      /* requesters node       */
    short minorversion;             /* sub version #         */
    short spare1;
    short spare2;
    char hoptrace[MAXHOP * ADDRSIZE];
    char mylogin[USERLEN];          /* requesters login name */
    char argstr1[MAXARGSTRLEN];     /* argument string #1    */
    char argstr2[MAXARGSTRLEN];     /* argument string #2    */
    char user[USERLEN];             /* requested user name   */
    char password[PASSWORDLEN];     /* password for user     */
    char spare4[MAXARGSTRLEN];
    char spare5[MAXARGSTRLEN];
};

struct vtresponse {
    int   spare1;
    short error;                    /* error value           */
    short sap;                      /* sap of responder      */
    short gatewayflag;              /* Is node a gateway?    */
    short hopcount;                 /* # of hops we've taken */
    short minorversion;             /* POLL only.            */
    short spare2;
    short spare3;
    char address[ADDRSIZE];         /* address of node(VTCON)*/
    char nodename[NODENAMESIZE];    /* name of node   (POLL) */
    char gatewaynode[NODENAMESIZE]; /* gateways used         */
    char spare4[MAXARGSTRLEN];
};

/* File transfer request types */

#define FTRECEIVE    0
#define FTSEND       1
#define FTLOGUSER    2
#define FTGETREXMIT  3
#define FTCD         4
#define FTCHGBUFSIZE 5

struct vtftrequest {
    long filesize;      /* Only valid for FTSEND,FTCHGBUFSIZE */
    long spare1;
    short ftreqtype;
    short modes;        /* Only valid for FTSEND */
    short spare2;
    short spare3;
    char filename[MAXARGSTRLEN];
    char spare4[MAXARGSTRLEN];
    char user[USERLEN];
    char password[PASSWORDLEN];
};

struct vtstatresponse {
    long  arg1;
    long  spare1;
    short error;                    /* error condition       */
    short arg2;
    short spare2;
    short spare3;
    char errormsg[MAXARGSTRLEN];    /* error message         */
    char  spare4[MAXARGSTRLEN];
};

struct vtgatewayinfo {
    long lantime;
    short spare1;
    char lannumber[ADDRSIZE];
};

struct vtioctl {
    int   request;                  /* ioctl type            */
    struct termio arg;
};

/* 10) Errors */

#define E_NOPIPE   1
#define E_NOFORK   2
#define E_READERR  3
#define E_FCNTLERR 4
#define E_OPENERR  5
#define E_UTMPERR  6
#define E_EXECERR  7
#define E_PERMERR  8
#define E_CHOWNERR 9
#define E_CHMODERR 10
#define E_FTOKERR  11
#define E_DEMONERR 12
#define E_MARGERR  13
/* Error 14 obsolete */
#define E_FDERR    15
#define E_NOSAPERR 16
#define E_NADDRERR 17
#define E_SAPERR   18
#define E_ADDRERR  19
#define E_XPKTERR  20
#define E_RPKTERR  21
#define E_SELERR   22
#define E_OLDSERR  23
#define E_OLDRERR  24
#define E_NOPTYERR 25
#define E_IOCTLERR 26
#define E_NOMEMERR 27
#define E_SUPRTERR 28
#define E_ALLOCERR 29
#define E_POOLERR  30

extern char *pty_errlist[];
