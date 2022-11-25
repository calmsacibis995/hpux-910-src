/*
 * @(#)ntypes.h: $Revision: 35.1 $ $Date: 88/02/10 14:09:47 $
 * $Locker:  $
 * 
 */

/******************************************************************************
 ******************************************************************************
 **                             NTYPES.H         
 **
 **  AUTHOR:            Carl Dierschow, David Hendricks, Tim DeLeon
 **
 **  DATE BEGUN:        19 September 84
 **
 **  MODULE:            arch
 **  PROJECT:           leaf_project
 **  REVISION:          2.6
 **  SCCS NAME:         /users/fh/leaf/sccs/arch/header/s.ntypes.h
 **  LAST DELTA DATE:   85/06/26
 **
 **  DESCRIPTION:       Global type and constant definitions.
 **
 **                     This file is included by files which reside in the
 **                     either the network environment or the user environment.
 **                     
 ******************************************************************************
 *****************************************************************************/

/*
 * The following definitions are provided to allow the far segment changes
 * required for the minuet (CONFIG_PC, CONFIG_64000, TWO_SEGS) to be included
 * in shared files with other projects.
 * Added by Don Tiller April 16, 1985.
 */

#ifdef CONFIG_64000 && TWO_SEGS

#define FAR_PROC_ON $FAR_PROC ON$
#define FAR_PROC_OFF $FAR_PROC OFF$
#define _NEWPAGE $PAGE$

#else

#define FAR_PROC_ON
#define FAR_PROC_OFF
#define _NEWPAGE

#endif

typedef char   * anyptr;        /* A pointer that needs to be changed */
typedef anyptr * anyptrp;       /* to another ptr in order to use it  */
typedef int    * int_t;



#ifdef CONFIG_VAX

#define CONFIG_LAN      1
 
typedef          char byte;     /* 8 bits -128..127                          */
typedef unsigned char unsbyte;  /* 8 bits unsigned 0..255                    */
typedef          short word;   /* 16 bits -32768,,32767                     */
typedef unsigned short unsword; /* 16 bits unsigned 0..65535                 */
typedef          long dword;    /* 32 bits                                   */
typedef unsigned long unsdword; /* 32 bits unsigned                          */
typedef unsdword *longptr;      /* Pointer to reach anywhere in mem          */
typedef anyptr   ordptr;        /* Type for storing ordinary ptrs            */
typedef dword    status_type;

#define void unsword            /* Reserved but unimplemented on VAX         */
#define MAXMSGS                 3
#define MAX_USER_DATA_SIZE      (6400-(60*4))
#define IPCBOX_SIZE     10      /* Integers                                  */
#define MAX_NUM_ARGS    12      /* Size of 'arglist' used in $CMEXEC calls   */
#define MAX_MEM_SIZE    10000000        /* To be adjusted after testing      */
#define MIN_MEM_SIZE    10000           /* To be adjusted after testing      */
#define MAX_STATISTICS  28      /* Size of driver statistics array. This     */
				/* needs to be adjusted whenever statistics  */
				/* are added or deleted in drvhw.h           */

#define convert_kernel_ptr(kernp,longp) {*longp = (longptr)kernp;}
#define convert_user_ptr(userp,longp)   {*longp = (longptr)userp;}
#define evaluate_user_ptr(longp,addr)   0
#define copy_network_data(from,to,len)  {int l;l=len;lib$movc3(&l,from,to);}
#define copy_block_of_data(from,to,len) {int l;l=len;lib$movc3(&l,*(from),*(to));}
#define copy_data_to_user(from,to,len)  {int l;l=len;lib$movc3(&l,from,*(to));}
#define copy_data_from_user(from,to,len) {int l;l=len;lib$movc3(&l,*(from),to);}

typedef struct {
	int     (*rtn)();
	int     num_args;
	int     data[ IPCBOX_SIZE ];
} ipcbox_type;

typedef struct {
	int     argc;
	int     argv[ MAX_NUM_ARGS ];
} arglist_type;

typedef struct {
	unsdword lsb;    /* least significant bits of the quad word          */
	   dword msb;    /* most  significant bits of the quad word          */
} timer_quadword;

typedef enum { USERPROC, NETPROC } proc_type;

/* identifies what event-flag clusters are associated with process cluster   */
/* slots #2 and #3.                                                          */
typedef struct {           
	short   protect_misc;
	short   dispatch;
} process_cluster_rec;
/* specifies the location in the dispatch_mgr_rec's event flag cluster name  */
/* of the alpha digit used to uniquely identify each cluster.                */
#define CLUSTER_NUM_POS      9

/* used to initialize the <netuic> global used in <exec_in_net_envt> of      */
/* <usinf.c> and <customize_envt> of <netionf.c>.                            */
#define NETUIC      0x000100FF  /* UIC= [1,377] (octal)                      */

/* NOTE: MAX_DISPATCH_TOKENS needs to be replaced with an SI defined symbol  */
#define MAX_DISPATCH_TOKENS 96  /* 3 event flag clusters                     */
				/* Any changes in the above symbol may req-  */
				/* quire changes to the dispatch_mgr struct  */
#define LCK$K_EXMODE         5  /* From $LCKDEF macro; not available via     */
				/* a *.h file                                */
/* $IRS$ lock_rec */
typedef struct {
	struct dsc$descriptor_s id; /* A VMS-specific string structure.      */
	struct {
		word            reserved;
		word            code;
		unsdword        lock_id;
	} status;
} lock_rec;
/* $IRS$ */

/* $IRS$ dispatch_mgr_rec */
typedef struct {
	word    active_tokens;
	struct {
		struct dsc$descriptor_s cluster;
		long                    ef;
	} ef_cluster[ ((MAX_DISPATCH_TOKENS*2) + 31) / 32 ];
} dispatch_mgr_rec;
/* $IRS$ */

typedef struct {
	int             errn;
	char            *ptr;
	int             value;
	byte            valtype;
	word            routine_inst;
	char            image_name[16];
} saverr_type;

#endif
 


 
#define CONFIG_LAN      1

typedef          char byte;     /* 8 bits -128..127          */
typedef unsigned char unsbyte;  /* 8 bits unsigned 0..255    */

typedef          short word;    /* 16 bits -32768,,32767     */
typedef unsigned short unsword; /* 16 bits unsigned 0..65535 */

typedef          long dword;    /* 32 bits                   */
typedef unsigned long unsdword; /* 32 bits unsigned          */

typedef unsdword ordptr;        /* Type for storing ordinary pointers */

#ifdef	DUX          /* added from lnatypes.h. Needed  */   /*PEKING change*/
typedef	unsbyte	 sap_address; /* to get protocol.c cluster.c and recovery.c*/
typedef unsword  hp_addr_exp; /* for the 6.0 DUX Vnode kernel.  jtg */
typedef unsword  ethernet_type;
#endif	DUX               

#define MAXMSGS 3               /* Maximum number of outstanding msgs */
#define MAX_USER_DATA_SIZE 10240        /* Maximum user data message size.  Used
					 * primarily for outbound messages.
					 */
#define MAX_MEM_SIZE    10000000        /* To be adjusted after testing      */
#define MIN_MEM_SIZE    30000           /* To be adjusted after testing      */

/*      The ptr_type identifies whether the pointer is a kernel or user
 *      pointer.
 */

#define USER_PTR_TYPE   0
#define KERNEL_PTR_TYPE 1

typedef struct {
	word    ptr_type;
	dword   pointer;
} longptr;




#ifdef CONFIG_PC

/* 
 *  The following compiler directives are needed for the PC environment
 *  to control the code model used.  These directives are in effect for
 *  the entire compilation unit and cannot be re-declared elsewhere in the
 *  code.
 *
 */

#if CONFIG_64000 && TWO_SEGS

$FAR_LIBRARIES ON$      /* compiler generated library calls FAR             */
$SHORT_LIBRARIES ON$    /* addresses passed to library routines are 16 bits */
$POINTER_SIZE 16$       /* all addresses are 16 bits long (int *, char * etc*/

#endif

/*
 *  The following defines all routines which follows as FAR routines (i.e., 
 *  they use 4 bytes of return address, allowing inter-segment calls).  This
 *  can be toggled back to NEAR routines by the compiler directive         
 *  $FAR_PROC OFF$.
 *
 */

#if CONFIG_64000 && TWO_SEGS

$FAR_PROC ON$

#endif

typedef          char byte;     /* 8 bits -128..127          */
typedef unsigned char unsbyte;  /* 8 bits unsigned 0..255    */

typedef          int  word;     /* 16 bits -32768,,32767     */
typedef unsigned int  unsword; /* 16 bits unsigned 0..65535  */

typedef          long dword;    /* 32 bits                   */
typedef unsigned long unsdword; /* 32 bits unsigned          */

typedef unsword ordptr;           /* Type for storing ordinary pointers */

typedef struct {                  /* Type of a pointer which can point */
	unsword offset;           /* Anywhere in physical memory */
	unsword segment;          /* See 8086 Architecture Manual */
} longptr;

#define MAXMSGS 7                 /* Maximum number of outstanding msgs */
#define MAX_USER_DATA_SIZE 10240  /* Maximum user data message size.  Used */
				  /* primarily for outbound messages. */

#define void    int               /* for procedures without return values */

#endif




typedef byte boolean;
#define FALSE 0
#define TRUE 1

typedef char    * charptr;
typedef charptr * charptrp;
typedef byte    * byteptr;
typedef word    * wordptr;
#define NIL     0

#ifndef NULL
#define NULL    0
#endif

typedef unsword error_number;   /* For network environment errors only. */
typedef unsword user_error;     /* For user environment errors only.    */

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*     Various other simple types which need to be defined.                  */
/*                                                                           */
/* ------------------------------------------------------------------------- */

typedef unsdword        inet_address;           /* Internet address     */
typedef unsbyte         link_address[6];
typedef unsbyte         ASYNC_address[10];      /* RASP address */

/* Uses a distributed network directory. */
#define MAX_FIELD_LENGTH  16
#define MAX_NUM_FIELDS    3

#define MAX_NAME_LENGTH   (MAX_FIELD_LENGTH*MAX_NUM_FIELDS+(MAX_NUM_FIELDS-1))
typedef char              node_name_type[MAX_NAME_LENGTH+1];
					     /* first byte is string length! */
#define MAX_DWORD       2147483647      /* Maximum signed dword value   */
#define MAX_UNSDWORD    4294967295      /* Maximum unsigned dword value */


/* ------------------------------------------------------------------------- */
/*                                                                           */
/*      Addressing constants : The following list of constants below are     */
/*                             in addressing different architecture levels.  */
/*                                                                           */
/*      Reference:   Clark Johnson, HP-IND.    Canonical Addressing Standard */
/*                   HP-internal memo,   19 January 1984                     */
/*                                                                           */
/* ------------------------------------------------------------------------- */


/*      Cannonical Addresses.           */

#define IP_CADR_OFFSET  1024            /*  2000 Octal          */
#define IP_CADR         1280            /*  2400 Octal used by RASP     */
#define IEEE802_CADR    9800            /*  23110 Octal         */
#define ETHERNET_CADR   9799            /*  23107 Octal         */
#define PROBE_CADR      1283            /*  2403 Octal          */
#define TCP_CADR        1030            /*  2006 Octal          */
#define NFT_CADR        1536            /*  3000 Octal          */
#define RFA_CADR        4672            /*  11100 Octal         */
#define RMTLB_CADR      1260            /*  2354 Octal          */


/* $IRS$ nodemgr */
/****************************************************************************
 *
 *      Some definitions from nodal management. 
 *
 ***************************************************************************/

#define MAX_LEN_FILE_SPEC       128     /* For VAX/HP200 file specifiers.  */
#define MAX_LINE_LEN            132     /* For VAX/HP200 user commands.    */
#define MAX_CONNECTION_LIMIT     96     /* Ought to be a multiple of 32    */
					/* Any change may require changes  */
					/* in CONFIG_VAX ntask.c (see MAX_ */
					/* DISPATCH_TOKENS in this file).  */
typedef char    vax_device_type[16];            
typedef char    file_spec[MAX_LEN_FILE_SPEC];  

#ifdef DUX
/* the following structure was added for the 6.0 DUX VNode kernel.  jtg */
/* PEKING change */

typedef struct i_face {
	struct i_face *		next;		/* link to next one */
	inet_address		inet_addr;	/* local internet address */
	file_spec		dev_name;	/* device used in powerup */
	dword			mtu;		/* how many bytes per packet */
	inet_address		subnet_mask;	/* if subnets in use */
	anyptr			iface_infop;	/* driver information */
	byte			opt_slot;	/* select code */
} IFACE, *iface_ptr;
#endif DUX

/* This structure defines the local node configuration information. */
typedef struct {
	enum{ DOWN, UP }        network_state;
	unsdword                memlim;
	charptr                 start_dynamic_memory;
	dword                   connection_limit;
	node_name_type          node_name;
	inet_address            my_inet_address;

#ifdef CONFIG_VAX
	vax_device_type         device_name;
	link_address            my_link_address;
#else
	file_spec               device_name;
	dword                   device_number;
	byte                    opt_slot;
#endif

} config_type;


#ifdef CONFIG_VAX
/* Data structure used for driver statistics requested by the user via READSTAT
 * command.
 */
typedef struct {
	unsdword   statistics[MAX_STATISTICS];
	boolean    interlan_controller;  /* TRUE if Interlan, FALSE otherwise */
} hw_statis_type;
#endif
/* $IRS$ */

/* -------------------------- end of NTYPES.H ------------------------------- */
