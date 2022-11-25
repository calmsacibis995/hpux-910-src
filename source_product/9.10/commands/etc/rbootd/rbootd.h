/* HPUX_ID: @(#) $Revision: 70.1 $  */
#include <stdio.h>     /* to get _NFILE */
#include <sys/types.h>
#include <sys/param.h> /* to get MAXPATHLEN, MAXHOSTNAMLEN */
#include <cluster.h>

/*
 * Constants
 */
#define TRUE            1
#define FALSE		0
#define MAXLANDEVS      10     /* Maximum number of lan devices */

extern char DEFAULTDEVICE[];	/* "/dev/ieee" */
extern char ERRLOGFILE[];	/* "/usr/adm/rbootd.log" */
extern char MALLOC_FAILURE[];   /* "Malloc failure.\n" */

#ifdef DTC
extern char DTCMGRDIR[];	/* "/usr/dtcmgr/" */
extern char DTC_CONFIG_FILE[];	/* "/usr/dtcmgr/map802" */
extern char IPC_NMDRBOOTD[];	/* "/usr/dtcmgr/ipc/rbootd" */
extern char DTCMACHINETYPE[];   /* "HP2345B" */
extern char DTC16TYPE[];        /* HP2340A */
extern char DTCTYPE[];          /* HP234   */

#define DTCNAMESIZE	9
#define NODENAMESIZE	50
#define DTC_MACH_LEN	7
#define DTC_LEN         5
#define SIC_STATUS_SIZE 36
#define DTCPATHNAMESIZE	100
#define INDREQ_POSTPROCESS  2
#endif /* DTC */

#define MAXPATHBYTES     256  /* max length for a pathname            */

#define ADDRSIZE         6    /* length of internal LAN address       */
#define LADDRSIZE        15   /* length of external LAN addr          */
#define CNODENAMESIZE    15   /* length of dux sitename               */
			      /* (from /etc/clusterconf)              */

/* states for client structure */

#define C_SETUP    0 /* No link address yet */
#define C_INACTIVE 1
#define C_ACTIVE   2
#define C_REMOVE   3 /* Active, but obsolete. Remove upon completion */
#define C_DELETE   4 /* Bad entry, delete */

/*
 * defines for various client types.  The various types are
 * searched in order, the last one being the lowest priority.
 *
 * BF_INSTALL types are handled specially for non-"HPS300" clients.
 *	      Only requests specifically addressed to the machine that
 *	      rbootd is running on are answered (we ignore abroadcast
 *	      requests).
 */
#define BF_SPECIFIC	0	/* Specific client entry */
#define BF_CLUSTER	1	/* Diskless client */
#define BF_PAWS		2	/* Pascal Workstation node */
#define BF_BASICWS	3	/* Basic Workstation node */
#define BF_INSTALL	4	/* Network Install */
#define BF_DEFAULT	5	/* anyone */
#define BF_DTC		6	/* DTC */
#define BF_TYPES	7	/* max number of types */

/* Error/Status levels for log() */

#define EL0 0 /* Startup, Initialization complete, Terminating */
#define EL1 1 /* Non Fatal Errors                              */
#define EL2 2 /* Requests from non registered servers          */
#define EL3 3 /* Requests from registered servers              */
#define EL4 4 /* Simple Debug                                  */
#define EL5 5 /* Detailed Debug                                */
#define EL6 6 /* Very Detailed debugging                       */
#define EL7 7 /* Very Very Detailed debugging                  */
#define EL8 8 /* Very Very Very Detailed debugging             */
#define EL9 9 /* buffer debugging                              */

#define MACHINE_TYPE_MAX  20   /* size of machine_type field in RMP */
/*
 * Client Block --
 *    Used to contain client configuration information
 *
 *    The client block also keeps track of any open sessions for a
 *    client, so we search the client block to see if he's got a session
 *    open.  If he does, we just reset the old one and reuse it.
 *    See open_session.
 */
struct cinfo
{
    struct cinfo *nextclient;     /* next client                     */
    struct bootinfo *bootlist;    /* list of bootable images         */
    long timestamp;               /* last time accessed by client    */
    short sid;                    /* session id associated client    */
    short state;                  /* current state                   */
    cnode_t cnode_id;             /* dux site id from clusterconf    */
#ifdef LOCAL_DISK
    cnode_t swap_site;            /* dux site id of swap server      */
#endif /* LOCAL_DISK */
    char linkaddr[ADDRSIZE];      /* client's linkaddress (internal) */
    char name[CNODENAMESIZE];     /* nodename (cnode_name if dux)    */
    char machine_type[MACHINE_TYPE_MAX]; /* machine_type from client */
    short cl_type;		  /* client type, (BF_*)	     */
#ifdef DTC
    short dtcsid;
#endif /* DTC */
};

#ifdef DTC
/*
 * map802record structure
 *  Used to contain information on DTC clients
 */
struct map802record
{
    char dtcname[DTCNAMESIZE];		/* mapname for a DTC */
    unsigned char curaddr[ADDRSIZE];	/* current IEEE address */
    unsigned char dldaddr[ADDRSIZE];	/* down loaded IEEE address */
    char nodename[NODENAMESIZE];	/* node name */
    unsigned short eventid;
};
#endif /* DTC */

/*
 * Session Structure --
 *    Used to contain information on open sessions
 */
struct session
{
    struct cinfo *client;   /* client info for this client   */
    long  curoffset;        /* current offset into boot file */
    int   bfd;              /* fd of boot file               */
};

struct bootinfo
{
    struct bootinfo *next;       /* pointer to next entry            */
    char *scanname;              /* Name to be returned on scan      */
    char *cmpname;               /* absolute pathname (minus root)   */
    char *fullname;              /* Fully expanded pathname          */
    short scanflag;              /* scanname returned on scan?       */
    short type;                  /* boot file type                   */
};

extern long time();
#define PUNCH_TIMESTAMP(loc) (*(loc) = time(0))
