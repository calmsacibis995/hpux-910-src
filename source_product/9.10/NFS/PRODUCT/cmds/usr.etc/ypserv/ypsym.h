/*	   @(#) ypsym.h:	$Revision: 1.27.109.2 $	$Date: 94/06/02 11:31:35 $  
	ypsym.h 1.1 86/02/05 Copyr 1985 Sun Microsystems, Inc
 	ypsym.h	2.1 86/04/16 NFSSRC 
*/

/*
 * This contains symbol and structure definitions for modules in the Network
 * Information Service server.  
 */

#include <dbm.h>			/* Pull this in first */
#define DATUM
extern void dbmclose();			/* Refer to dbm routine not in dbm.h */
#ifdef NULL
#undef NULL				/* Remove dbm.h's definition of NULL */
#endif
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/stat.h>
/* HPNFS Use param.h to get the value of MAXPATHLEN */
#include <sys/param.h>
/* HPNFS Use ndir.h instead of sys/dir.h so that we get the right definition */
/* HPNFS of DIRSIZ.							     */
#include <ndir.h>
#include <time.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <rpcsvc/ypclnt.h>

/*  HPNFS
 *
 *  Sun has a symbolic link from /etc/yp to /usr/etc/yp.  HP does not.
 *  Dave Erickson, 3-18-87.
 *
 *  HPNFS
 */
#define __YP_PATH_PREFIX "/usr/etc/yp"
#define YPDBPATH_LENGTH sizeof(__YP_PATH_PREFIX)
#define ORDER_KEY "YP_LAST_MODIFIED"
#define ORDER_KEY_LENGTH (sizeof(ORDER_KEY) - 1)
#define MAX_ASCII_ORDER_NUMBER_LENGTH 10
#define MASTER_KEY "YP_MASTER_NAME"
#define MASTER_KEY_LENGTH (sizeof(MASTER_KEY) - 1)
#define MAX_MASTER_NAME 256
#define INPUT_FILE "YP_INPUT_FILE"
#define INPUT_FILE_LENGTH (sizeof(INPUT_FILE) - 1)
#ifdef DBINTERDOMAIN
#define DNS_FALLBACK_KEY "YP_INTERDOMAIN"
#define DNS_FALLBACK_KEY_LENGTH (sizeof(DNS_FALLBACK_KEY) - 1)
#endif /* DBINTERDOMAIN */

#ifndef YPXFR_PROC
#define YPXFR_PROC "/usr/etc/yp/ypxfr"
#endif
#ifndef YPPUSH_PROC
#define YPPUSH_PROC "/usr/etc/yp/yppush"
#endif

typedef void (*PFV)();
typedef int (*PFI)();
typedef unsigned int (*PFU)();
typedef long int (*PFLI)();
typedef unsigned long int (*PFULI)();
typedef short int (*PFSI)();
typedef unsigned short int (*PFUSI)();

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef NULL
#undef NULL
#define NULL 0
#endif

#define YPINTERTRY_TIME 10		/* Secs between tries for peer bind */
#define YPTOTAL_TIME 30			/* Total secs until timeout */
#define YPNOPORT ((unsigned short) 0)	/* Out-of-range port value */

/* External refs to cells and functions outside of the nis */

extern int errno;

/* HPNFS With the introduction of NFS3_2 svc_fds is defined in the header */
/* 	 file rpc/svc.h and therefore, does not need to be defined inside */
/*	 this program as an extern.  svc.h also declares svc_fdset, which */
/*	 is used later on in this program, as an extern.	 	  */
/* 	 The header file rpc/svc.h is included by rpc/rpc.h.	          */

extern char *malloc();
extern char *strcpy();
extern char *strcat();
extern long atol();

/* External refs to nis server data structures */

extern bool ypinitialization_done;
extern char ypdbpath[];
extern int ypdbpath_length;
extern struct timeval ypintertry;
extern struct timeval yptimeout;
extern char myhostname[MAX_MASTER_NAME + 1];
extern char order_key[];
extern char master_key[];

/* External refs to nis server-only functions */

extern bool ypcheck_map_existence();
extern bool ypset_current_map();
extern void ypclr_current_map();
extern bool ypbind_to_named_server();
/*  HPNFS
 *
 *  The ypmkfilename function now returns one of these defined integer values.
 *  Dave Erickson, 3-20-87.
 *
 *  HPNFS
 */
extern int ypmkfilename();
#define MAPNAME_OK		0
#define PATH_TOO_LONG		1
#define MAPNAME_TOO_LONG	2
extern int yplist_maps();

#ifdef YPSERV_DEBUG
#define V2_PROC_OFF     0
#define V1_PROC_OFF     100
#define YPMAXKEY        64
#define YPMAXVAL        256
struct  debug_struct {

 u_long    rq_proc;             /* default req */

 char   mapname[YPMAXMAP];      /* map name from request */
 char   domainname[YPMAXDOMAIN];/* domainname from request */

 char   key[YPMAXKEY];          /* key string from request */
 int    keylen;                 /* len of key */

 char   val[YPMAXVAL];          /* value returned = max 256 bytes */
 int    vallen;                 /* len of value returned */

 int    result;                 /* for calls that return a bool/int */
 u_long resp_status;            /* response status */
 struct sockaddr_in     raddr;  /* caller's address */
};
#endif /* YPSERV_DEBUG */
