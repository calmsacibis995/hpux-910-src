 
/****************************************************************************/
/****************************************************************************/
/**			       SOCKETVAR.H				   **/
/**									   **/
/**  AUTHOR: Bill Mowson/Mike Robinson					   **/
/**									   **/
/**  DATE BEGUN: 1/14/85						   **/
/**									   **/
/**  MODULE:								   **/
/**  PROJECT:								   **/
/**  REVISION:								   **/
/**  SCCS NAME:								   **/
/**  LAST DELTA DATE:							   **/
/**									   **/
/**  DESCRIPTION:  This file contains the structure and constant	   **/
/**		   definitions required by the socket.c and socketfs.c	   **/	
/**		   files.  
/****************************************************************************/
/****************************************************************************/

/* the following typedefs define the structures which are required for	    */ 
/* parameter passing of the new socket intrinsic calls			    */ 

typedef struct {
   dword   domain;
   dword   type;
   dword   protocol;
} socket_parms;


typedef struct {
   dword   s;
   struct sockaddr_in *addr;
   dword   *addrlen;
} accept_parms, getsockname_parms, getpeername_parms;


typedef struct {
   dword   s;
   struct  sockaddr_in *addr;
   dword   addrlen;
} bind_parms, connect_parms;


typedef struct {
   dword   s;
   dword   backlog;
} listen_parms;


typedef struct {
   dword   s;
   char	   *buf;
   dword   len;
   dword   flags;
} send_parms, recv_parms;

typedef struct {
   dword   s;
   char	   *buf;
   dword   len;
   dword   flags;
   struct  sockaddr_in *addr;
   dword   addrlen;
} sendto_parms;

typedef struct {
   dword   s;
   char	   *buf;
   dword   len;
   dword   flags;
   struct  sockaddr_in *addr;
   dword   *addrlen;
} recvfrom_parms;

typedef struct {
   dword   s;
   dword   how;
} shutdown_parms;

typedef struct {
   dword   s;
   int	   level;
   dword   optname;
   char	   *val;
   dword   val_size;
} setsockopt_parms;

typedef struct {
   dword   s;
   int	   level;
   dword   optname;
   char	   *val;
   dword   *val_size;
} getsockopt_parms;

/******************************************************************************/
/******************************************************************************/

/* the following constants and structures are used only by kernel socket code */

/* socket states */

#define SO_LISTENING	  0x001	  /* Set if a listen has been done	  */
#define SO_NBIO		  0x002	  /* Socket is in non-block mode	  */
#define SO_COLLR	  0x004	  /* socket is currently select reading	  */
#define SO_COLLW	  0x008	  /* socket is currently select writing	  */
#define SO_ISCONNECTING	  0x010	  /* socket is in process of connectiong  */
#define SO_UCANTSEND	  0x020	  /* user can't send any more		  */
#define SO_UCANTRECV	  0x040	  /* user can't recv any more		  */
#define SO_ISBOUND	  0x080	  /* Socket is bound to a port.		  */
#define SO_SHUTDOWN	  0x100   /* The socket has been shutdown	  */

/* socket options */

#define SO_PRIV		  0x001	  /* socket is priviledged - super user	  */

/* MISCELLANEI */

#define U_ERROR		  -1	  /* user error return value		   */
#define IPPORT_RESERVED	  1024	  /* 0 - 1024 ports may only be used by SU */
#define SO_NOWAIT	  -1	  /* tell SI not to block		   */
#define SO_INFWAIT	  0	  /* tell SI to wait forever		   */
#define PMAX	SI_PARM_BLK_SIZE  /* The maximum size of SI's parameter blk*/
#define SO_MSG_SIZE	  2860	  /* Max In/Out size for SI		   */
#define SO_SEND_CNT	  2	  /* Max send cnt			   */
#define SO_RECV_CNT	  2	  /* Max recv cnt			   */
#define SO_SHUT1	  0x2	  /* Shutdown type 1			   */ 
#define SO_SHUT0	  0x1	  /* Shutdown type 0			   */
#define SEC_SIZE	  9	  /* Size of IP security field		   */
#define PREC_SIZE	  1	  /* Size of TCP precedence field	   */
#define SO_DGRAM_SEND_CNT 1	  /* The number of outstanding sends	   */
#define SO_RAW_MSG_SIZE   4096	  /* The size of in/out msgs for raw socks */
#define SEND_SIGNAL	  21	  /* Send signal 			   */
#define NO_SIGNAL	  22	  /* Don't send a signal		   */

/* socket structure used by kernel level socket code to store all
   pertinent socket information	 -  one of these exist for each socket	*/

typedef struct {
   unsword	 rport;	    /* remote service port			     */
   unsdword	 rinet_addr;/* internet address of the remote system	     */ 
   unsword	 lport;	    /* local service port			     */
   unsdword	 linet_addr;/* internet address of the local system	     */ 
   byte		 proto;	    /* protocl type				     */
   word		 type;	    /* type of socket				     */ 
   word		 family;    /* the family of this socket		     */ 
   unsword	 options;   /* current options set for this socket	     */
   unsword	 state;	    /* current state of this socket		     */
   struct proc	*so_selr;   /* pointer to blocked read process structure     */
   struct proc	*so_selw;   /* pointer to blocked write process structure    */
   dword	 cd;	    /* SI's connection descriptor/ or listen desc.   */
   struct file	 *fp;	    /* pointer to the file structure.		     */
   error_number	 error;	    /* set if there was a error on the connect.	     */
   iodescr_type	 iodescr;   /* Holder for SI's iodescr.			     */
   dword	 linger;    /* Holder for the linger interval		     */
   dword	 msg_size;  /* In/Out bound message size.		     */
   si_sec_prec	 s_and_p;   /* Holds the security and precedence	     */
   dword	 pid;	    /* The ID of the process for signals	     */
   char		 pblk[PMAX];/* SI parameter passing block		     */
} socket_type;

