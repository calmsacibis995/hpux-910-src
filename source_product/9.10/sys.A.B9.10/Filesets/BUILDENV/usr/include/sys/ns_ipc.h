/* $Header: ns_ipc.h,v 1.52.83.4 93/09/17 18:31:15 kcs Exp $ */

#ifndef _SYS_NS_IPC_INCLUDED
#define _SYS_NS_IPC_INCLUDED

/* ns_ipc.h: NetIPC (NS) definitions */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

   /* Types */

#ifdef _KERNEL_BUILD
#  include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#  include <sys/types.h>
#endif /* _KERNEL_BUILD */

   typedef int ns_int_t;
   typedef int ns_flags_t;
   typedef char *ns_opt_t;
   typedef char *opt_t;


   /* Function prototypes */

#  ifndef _KERNEL
#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */

#  ifdef _PROTOTYPES
     extern void addopt(short [], short, short, short, short [], short *);
     extern void initopt(short [], short, short *);
     extern int optoverhead(short, short *);
     extern void readopt(short [], short, short *, short *, short [], short *);
     extern char *ipcerrstr(int);
     extern void ipcerrmsg(int, char *, int *, int *);
     extern void ipcconnect(ns_int_t, ns_int_t, ns_int_t *, short [],
				ns_int_t *, ns_int_t *);
     extern void ipccontrol(ns_int_t, ns_int_t, const char *, ns_int_t,
				char *, ns_int_t *, ns_int_t *, ns_int_t *);
     extern void ipccreate(ns_int_t, ns_int_t, ns_int_t *, short [],
				ns_int_t *, ns_int_t *);
     extern void ipcdest(ns_int_t, const char *, ns_int_t, ns_int_t,
				short *, ns_int_t, ns_int_t *, short [],
				ns_int_t *, ns_int_t *);
     extern void ipcgetnodename(char *, ns_int_t *, ns_int_t *);
     extern void ipclookup(const char *, ns_int_t, const char *, ns_int_t,
				ns_int_t *, ns_int_t *, ns_int_t *, ns_int_t *,
				ns_int_t *);
     extern void ipcname(ns_int_t, const char *, ns_int_t, ns_int_t *);
     extern void ipcnameerase(const char *, ns_int_t, ns_int_t *);
     extern void ipcrecv(ns_int_t, void *, ns_int_t *, ns_int_t *,
			   short [], ns_int_t *);
     extern void ipcrecvcn(ns_int_t, ns_int_t *, ns_int_t *, short [],
			   ns_int_t *);
     extern void ipcselect(ns_int_t *, int [2], int [2], int [2], ns_int_t,
			   ns_int_t *);
     extern void ipcsend(ns_int_t, const void *, ns_int_t, ns_int_t *,
			   short [], ns_int_t *);
     extern void ipcsetnodename(const char *, ns_int_t, ns_int_t *);
     extern void ipcshutdown(ns_int_t, ns_int_t *, short [], ns_int_t *);
#  else /* not _PROTOTYPES */
     extern void addopt();
     extern void initopt();
     extern int optoverhead();
     extern void readopt();
     extern char *ipcerrstr();
     extern void ipcerrmsg();
     extern void ipcconnect();
     extern void ipccontrol();
     extern void ipccreate();
     extern void ipcdest();
     extern void ipcgetnodename();
     extern void ipclookup();
     extern void ipcname();
     extern void ipcnameerase();
     extern void ipcrecv();
     extern void ipcrecvcn();
     extern void ipcselect();
     extern void ipcsend();
     extern void ipcsetnodename();
     extern void ipcshutdown();
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */
#  endif /* not _KERNEL */


   /*
    *	Naming constants
    */
#  define NS_MAX_NODE_NAME	50
#  define NS_MAX_NODE_PART	16
#  define NS_MAX_SOCKET_NAME	16

   /*
    * MAX TIMEOUT as enforced by itimerfix and hence by ipccontrol
    */
#  define NIPC_MAX_TIMEO	1000000

   /*
    *	socket type definitions
    */
#  define NS_CALL	3
#  define NS_VC		6
#  define NS_DEST	7
#  define NS_NULL_DESC	(-1)


   /*
    *	protocol IDs
    */
#  define NSP_TCP	4

   /*
    *	flags values
    */
#  define NSF_VECTORED		0x00000001	/* Netipc std bit 31 */
#  define NSF_PREVIEW		0x00000002	/* Netipc std bit 30 */
#  define NSF_MORE_DATA		0x00000020	/* Netipc std bit 26 */
#  define NSF_DATA_WAIT		0x00000800	/* Netipc std bit 20 */
#  define NSF_GRACEFUL_RELEASE	0x00004000	/* Netipc std bit 17 */
#  define NSF_MESSAGE_MODE	0x40000000	/* Netipc std bit 1  */


   /*
    *	ipccontrol() request values
    */
#  define NSC_NBIO_ENABLE		   1	/* */
#  define NSC_NBIO_DISABLE		   2	/* */
#  define NSC_TIMEOUT_RESET		   3	/* */
#  define NSC_TIMEOUT_GET		   4	/*  not in std */
#  define NSC_SOCKADDR			  16
#  define NSC_RECV_THRESH_RESET		1000	/* */
#  define NSC_SEND_THRESH_RESET		1001	/* */
#  define NSC_RECV_THRESH_GET		1002	/* */
#  define NSC_SEND_THRESH_GET		1003	/* */
#  define NSC_GET_NODE_NAME		9008	/* */


   /*
    * NS options
    */
#  define NSO_NULL			(ns_opt_t)0/* null option structure */
#  define NSO_MAX_SEND_SIZE		3
#  define NSO_MAX_RECV_SIZE		4
#  define NSO_MAX_CONN_REQ_BACK		6
#  define NSO_DATA_OFFSET		8
#  define NSO_PROTOCOL_ADDRESS		128

   /*
    * Netipc errors
    */
#  define NSR_NO_ERROR		000	/* no error occurred		*/
#  define NSR_BOUNDS_VIO	3	/* parameter bounds violation	*/
#  define NSR_NETWORK_DOWN	4	/* network not initialized	*/
#  define NSR_SOCK_KIND		5	/* invalid socket kind value	*/
#  define NSR_PROTOCOL		6	/* invalid protocol id		*/
#  define NSR_FLAGS		7	/* error in flags parameter	*/
#  define NSR_OPT_OPTION	8	/* invalid opt array option	*/
#  define NSR_PROTOCOL_NOT_ACTIVE  9	/* protocol not active		*/
#  define NSR_KIND_AND_PROTOCOL	10	/* sock kind/protocol mismatch	*/
#  define NSR_NO_MEMORY		11	/* out of memory (mbufs)	*/
#  define NSR_ADDR_OPT		14	/* illegal proto addr in opt arr*/
#  define NSR_NO_FILE_AVAIL	15	/* no file table entries avail  */
#  define NSR_OPT_SYNTAX	18	/* error in opt array syntax	*/
#  define NSR_DUP_OPTION	21	/* duplicate option in opt array*/
#  define NSR_MAX_CONNECTQ	24	/* max cnct rqs err in opt array*/
#  define NSR_NLEN		28	/* invalid name length		*/
#  define NSR_DESC		29	/* invalid descriptor		*/
#  define NSR_CANT_NAME_VC	30	/* can't name vc socket 	*/
#  define NSR_DUP_NAME		31	/* duplicate name specified	*/
#  define NSR_NAME_TABLE_FULL	36	/* table is full		*/
#  define NSR_NAME_NOT_FOUND	37	/* specified name not matched	*/
#  define NSR_NO_OWNERSHIP	38	/* user doesn_t own the socket	*/
#  define NSR_NODE_NAME_SYNTAX	39	/* invalid node name syntax	*/
#  define NSR_NO_NODE		40	/* node does not exist		*/
#  define NSR_CANT_CONTACT_SERVER 43	/* can't contact remote server	*/
#  define NSR_NO_REG_RESPONSE	44	/* no response from remote reg	*/
#  define NSR_SIGNAL_INDICATION	45	/* syscall aborted due to signal*/
#  define NSR_PATH_REPORT	46	/* can't interpret path report	*/
#  define NSR_BAD_REG_MSG	47	/* received garbage registry msg*/
#  define NSR_DLEN		50	/* invalid data length value	*/
#  define NSR_DEST		51	/* invalid dest descriptor	*/
#  define NSR_PROTOCOL_MISMATCH	52	/* source and dest have dif prot*/
#  define NSR_SOCKET_MISMATCH	53	/* dest has wrong type socket	*/
#  define NSR_NOT_CALL_SOCKET	54	/* invalid socket descriptor	*/
#  define NSR_WOULD_BLOCK	56	/* would block to satisfy req.	*/
#  define NSR_SOCKET_TIMEOUT	59	/* timer popped			*/
#  define NSR_NO_DESC_AVAIL	60	/* no file descriptos avail	*/
#  define NSR_CNCT_PENDING	62	/* must call IPCRECV		*/
#  define NSR_REMOTE_ABORT	64	/* remote aborted the connection*/
#  define NSR_LOCAL_ABORT	65	/* local side aborted the cnct	*/
#  define NSR_NOT_CONNECTION	66	/* not a connection descriptor	*/
#  define NSR_REQUEST		74	/* invalid IPCCONTROL request	*/
#  define NSR_TIMEOUT_VALUE	76	/* invalid in IPCCONTROL	*/
#  define NSR_ERRNUM		85	/* invalid netipc error number  */
#  define NSR_VECT_COUNT	99	/* invalid byte count in vector	*/
#  define NSR_TOO_MANY_VECTS	100	/* too many vect data descripts	*/
#  define NSR_DUP_ADDRESS	106	/* address already in use	*/
#  define NSR_REMOTE_RELEASED	109	/* graceful release; can't recv	*/
#  define NSR_UNANTICIPATED	110	/* netipc subsystem is disabled */
#  define NSR_DEST_UNREACHABLE	116	/* unable to reach destination	*/
#  define NSR_VERSION		118	/* version number mismatch	*/
#  define NSR_OPT_ENTRY_NUM	124	/* bad entry number specified	*/
#  define NSR_OPT_DATA_LEN	125	/* bad entry length specified	*/
#  define NSR_OPT_TOTAL		126	/* initopt( illegal total 	*/
#  define NSR_OPT_CANTREAD	127	/* cant read option  readopt(  */
#  define NSR_THRESH_VALUE	1002	/* bad integer to IpcControl	*/
#  define NSR_NOT_ALLOWED	2003	/* user not super-user 		*/
#  define NSR_MSGSIZE		2004	/* message size too big 	*/
#  define NSR_ADDR_NOT_AVAIL	2005	/* address not available 	*/

   /* used to map kernel errors to NSR equivalents */
#  define NIPC_ERROR_OFFSET	10000

   /* these exist for historical reasons */
#  define NSO_MIN_BURST_IN		7		/* ignored in 8.0 */
#  define NSO_MIN_BURST_OUT		11		/* ignored in 8.0 */
#  define NSOL_MIN_BURST_IN		2
#  define NSOL_MIN_BURST_OUT		2
#  define NSOL_MAX_CONN_REQ_BACK 	2
#  define NSOL_MAX_RECV_SIZE 		2
#  define NSOL_MAX_SEND_SIZE 		2

#  ifndef MIN
#     define MIN(a,b) (((a)<(b))?(a):(b))
#  endif

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_NS_IPC_INCLUDED */
