/*
 * @(#)trigdef.h: $Revision: 1.19.83.4 $ $Date: 93/09/17 18:36:51 $
 * $Locker:  $
 * 
 */

#ifndef _SYS_TRIGDEF_INCLUDED
#define _SYS_TRIGDEF_INCLUDED

/***************************************************************/
/***************************************************************/
/**  AUTHOR: Alan Burke					      **/
/**							      **/
/**  DATE BEGUN: July 1984				      **/
/**							      **/
/**  MODULE: triggers					      **/
/**  PROJECT: leaf_project				      **/
/**  REVISION: 1.22					      **/
/**  SCCS NAME: /users/fh/leaf/sccs/triggers/header/s.trigdef.h**/
/**  LAST DELTA DATE: 85/01/18				      **/
/***************************************************************/
/***************************************************************/

#define		EMPTY		0
#define		NIL		0
#define		OK		0
#define		NULL_MOD	0

#ifdef CONFIG_HP500
#define	KSET_TRIGGER_ID		1006
#define KCLEAR_TRIGGER_ID	1007
#define KTRY_TRIGGER_ID		1008
#define KTRIGGER_DUMP_ID	1009
#define KUSET_ID		1010
#endif


#ifdef	CONFIG_HP200
#define		MEM_REQ_SIZE	4096
#endif
#ifdef  CONFIG_VAX
#define         MEM_REQ_SIZE    4096  /* until further notice! */
#endif

#ifdef	__hp9000s800
/* these ought to be the same as on the 500 ... */
#define	KSET_TRIGGER_ID		1021
#define	KCLEAR_TRIGGER_ID	1022
#define	KTRY_TRIGGER_ID		1023
#define	KTRIGGER_DUMP_ID	1024
#define KUSET_ID		1025
#define KMATCH_TRIGGER_ID	1026

#define		MEM_REQ_SIZE	4096
#endif

#define		HASH_TABLE_SIZE 32	/* must be a power of 2 */
#define		HT_MEM		200

/* the last POP_CNT popped triggers are kept in the table for debugging */
#define		POP_CNT		2

#define		MAX_TRIG_ALLOC	1	/* maximum number of mem blocks
					   to get from system */

/* triggers that use module FAST_TRIG_MOD take advantage of the bit map */
#define		FAST_TRIG_MOD	31
#define		TRIG_BMAP_SIZE	200	/* size of bit map in bits */

#define		MAX_TNUM	1000	/* largest allowable trigger # */
#define		MIN_TNUM	-300	/* smallest allowable trigger # */
#define		MAX_TMOD	31	/* largest allowable module # */
#define		MAX_MEM		5000

#define		NOT_FOUND	-1
#define		OUT_OF_MEM	-1
#define		NO_MEM_ALLOC	-1

#define		BAD_TMOD	-1
#define		BAD_TNUM	-1
#define		BAD_TCOUNT	-1
#define		TRIG_NOT_COMPILED -1
#define		CANT_SET_TRIG	-1
#define		CANT_CLEAR_TRIG -1
#define		TRIG_NOT_INIT	-1
#define		CALL_ERROR	-2000000


/*=================== SYSTEM TRIGGERS ================*/
/*---------------- HP-UX CALLS --------------------*/
#ifdef CONFIG_HP200
/* trigger numbers for module 0 */
/* trigger numbers < 0 are reserved (by convention) for system calls */
#define	 T_U2_ACCESS	       -100
#define	 T_U2_ALARM	       -101
#define	 T_U2_CHDIR	       -102
#define	 T_U2_CHMOD	       -103
#define	 T_U2_CHOWN	       -104
#define	 T_U2_CLOSE	       -105
#define	 T_U2_CREAT	       -106
#define	 T_U2_EXEC	       -107
#define	 T_U2_FCNTL	       -108
#define	 T_U2_FORK	       -109
#define	 T_U2_LINK	       -110
#define	 T_U2_LSEEK	       -111
#define	 T_U2_MKNOD	       -112
#define	 T_U2_OPEN	       -113
#define	 T_U2_READ	       -114
#define	 T_U2_STAT	       -115
#define	 T_U2_UNLINK	       -116
#define	 T_U2_UTIME	       -117
#define	 T_U2_WRITE	       -118
#define	 T_U2_SETUID	       -119
#define  T_U2_MKDIR	       -120
#define  T_U2_RMDIR	       -121
#define  T_U2_FSTAT            -122
#define  T_U2_SETGID           -123
#define  T_U2_IOCTL            -124

#define	 T_U3_GETPWNAM	       -130
#define	 T_U3_MALLOC	       -131
#define	 T_U3_PCLOSE	       -132
#define	 T_U3_POPEN	       -133
#define  T_U3_FEOF             -134
#define  T_U3_FERROR           -135
#define  T_U3_FOPEN            -136
#define  T_U3_GETCHAR          -137
#define  T_U3_NL_GETMSG        -138
#define  T_U3_NL_CURRLANGID    -139

/* SERVICES INTERFACE TRIGGERS */
#define	 T_SI_MUX_CONNECT      -71
#define	 T_SI_CONNECT	       -72
#define	 T_SI_RECVCN	       -73
#define	 T_SI_MUX_RECV	       -74
#define	 T_SI_SEND	       -75
#define	 T_SI_RECV	       -76
#define	 T_SI_CHG_OWNER	       -77
#define	 T_SI_IOWAIT	       -78
#define	 T_SI_SHUTDOWN	       -79
#define	 T_SI_GET_ADDR	       -80
#define	 T_SI_IS_THIS_ME       -81
#define	 T_SI_INIT_IODESCR     -82
#define	 T_SI_BUF_SIZE	       -83
#define	 T_SI_ABORT_PROC       -90
#define	 T_SI_ACK_CONN_PROC    -91
#define	 T_SI_ACK_SEND_PROC    -92
#define	 T_SI_ERROR_PROC       -93
#define	 T_SI_INIT_RECV_PROC   -94
#define	 T_SI_T_O_PROC	       -95
#define  T_SI_MUX_REF          -96


/* TCP INTERFACE TRIGGERS */ 
#define	 T_TCP_ABORT_PROC      -11 
#define	 T_TCP_ERROR_PROC      -12
#define	 T_TCP_INIT_CONN_PROC  -13
#define	 T_TCP_INIT_SEND_PROC  -14
#define	 T_TCP_RECV_PROC       -15
#define	 T_TCP_T_O_PROC	       -16
#define	 T_TCP_ACK_RECV_PROC   -17
#define  T_TCP_CONFIG_PROC     -18

/* IP INTERFACE TRIGGERS */
#define	 T_IP_ABORT_PROC       -21
#define	 T_IP_CONN_PROC	       -22
#define	 T_IP_ERROR_PROC       -23
#define	 T_IP_RECV_PROC	       -24
#define	 T_IP_SEND_PROC	       -25
#define	 T_IP_T_O_PROC	       -26

/* LLP INTERFACE TRIGGERS */
#define	 T_LLP_ABORT_PROC      -31
#define	 T_LLP_CONN_PROC       -32
#define	 T_LLP_ERROR_PROC      -33
#define	 T_LLP_RECV_PROC       -34
#define	 T_LLP_SEND_PROC       -35
#define	 T_LLP_T_O_PROC	       -36

/* USER INTERFACE TRIGGERS */
#define	 T_EXEC_IN_NET_ENVT    -41

/* TASK HANDLER TRIGGERS */
#define	 T_DISPATCH	       -44
#define	 T_WAKE_UP_USER_TASK   -45

/* PATH CREATION TRIGGERS */
#define	 T_CREATE_NAME_OUTB    -47
#define	 T_CREATE_PASSIVE_PATH -48
#define	 T_CREATE_ADDR_OUTB    -49
#define	 T_CREATE_DGRAM_SERV   -50
#define	 T_FIND_INBOUND_PATH   -51
#define	 T_FIND_BB_PATH	       -52
#define	 T_DESTROY_PATH	       -53
#define	 T_ACK_CREATE_PROC     -55

/* PATH INFORMATION TRIGGERS */
#define	 T_IS_THIS_ME	       -56
#define	 T_GET_NETWORK_INFO    -57

/* PORT MANAGEMENT TRIGGERS */
#define	 T_ALLOCATE_DYNAMIC_P  -58
#define	 T_RESERVE_STATIC_P    -59

/* MEMORY MANAGEMENT TRIGGERS */
#define	 T_ALLOCATE	       -60

/* BUFFER POOL MANAGEMENT TRIGGERS */
#define	 T_GET_BUFFER	       -61
#define	 T_CONTRIBUTE_BUFFERS  -63

#endif /* CONFIG_HP200 */

#ifdef __hp9000s800

/* PXP INTERFACE TRIGGERS */
#define T_PXP_PROC	        10

/* UDP INTERFACE TRIGGERS */
#define T_UDP_PROC	        20

/* NFS SUBSYSTEM TRIGGERS */
#define T_NFS_PROC		30

#define T_NFS_TRUNCATED		1	/* simulate truncated msg received */
#define T_NFS_TOO_LONG		2	/* simulate an overly long message */
#define T_NFS_XDRREPL		3	/* failure to xdr a reply */
#define T_NFS_PCBSETADDR	4	/* simulate in_pcbsetaddr failure */
#define T_NFS_FSEND_FAIL	5	/* udp/ip fastsend failure */
#define T_NFS_S_MA_EXPAND	6	/* server macct expand failed */
#define T_NFS_S_MA_CREATE	7	/* server macct create failed */
#define T_NFS_C_MA_EXPAND	8	/* client macct expand failed */
#define T_NFS_C_MA_CREATE	9	/* client macct create failed */
#define T_NFS_CHARGE		10	/* charge to nfs macct failed */

#define T_NFS_XDR_CALLHDR	11	/* failure to xdr a call header */
#define T_NFS_PROTOFIND 	12	/* failure to find protocol */
#define T_NFS_SOCREATE 		13	/* failure to create client socket */
#define T_NFS_MGET_1 		14	/* MGET failed in bindresvport */
#define T_NFS_MGET_2 		15	/* MGET failed in clntkudp_callit */
#define T_NFS_ENCODE 		16	/* failed to serialize call */
#define T_NFS_INTR_1 		17	/* got interrupted in callit */
#define T_NFS_TOO_SHORT 	18	/* hopelessly short mbuf */
#define T_NFS_BAD_AUTH 		19	/* reply authentication failed */
#define T_NFS_INTR_2 		20	/* got interrupted in callit again */

#define T_NFS_MGET_3 		21	/* MGET failed in ku_fastsend */
#define T_NFS_MGET_4 		22	/* MGET failed in ku_fastsend */
#define T_NFS_MGET_5 		23	/* MGET failed in ku_fastsend */
#define T_NFS_IF_ERROR 		24	/* interface output error */
#define T_NFS_BAD_PKT		25	/* send a bad rpc packet */

#define T_NFS_XDRMBUF_GETL	31	/* xdrmbuf_getlong returns error */
#define T_NFS_XDRMBUF_GETB	32	/* xdrmbuf_getbytes returns error */
#define T_NFS_XDRMBUF_PUTB	33	/* xdrmbuf_putbytes returns error */
#define T_NFS_MGET_6		34	/* mclgetx failed in xdrmbuf_putbuf */
#define T_NFS_MGET_7		35	/* MGET failed in xdrmbuf_putbuf */
#define T_NFS_XDRMBUF_INLINE	36	/* xdrmbuf_inline returns error */
#define T_NFS_SET_INLINE	37	/* set xdrmbuf_inline trigger */
#define T_NFS_MGET_8		38	/* MGET failed in xdr_rrok */

#define T_NFS_XDRMEM_GETL	41	/* xdrmem_getlong returns error */
#define T_NFS_XDRMEM_PUTL	42	/* xdrmem_putlong returns error */
#define T_NFS_XDRMEM_GETB	43	/* xdrmem_getbytes returns error */
#define T_NFS_XDRMEM_PUTB	44	/* xdrmem_putbytes returns error */
#define T_NFS_XDRMEM_INLINE	45	/* xdrmem_inline returns error */

	/* definitions of trigger module numbers */

#define FT_SAMPLE	0	/* sample module number */
#define	NFT_MOD		10	/* NFT (user space) */

	/* definition of trigger numbers for module FT_SAMPLE */

#define FT_SAMPLE_1	1	/* trigger number 1 in FT_SAMPLE */
#endif

/*------------------------- MACROS -------------------------------*/

#ifdef NTRIGGER

/* The following macros allow you to put trigger calls into the code and
 * include or remove them by defining or undefining NTRIGGER.
 */

#if defined(__hp9000s300) || defined(__hp9000s800)

/* NB: try_trigger and its macros examine the global 'my_modname' to determine
 * the module number.  utry_trigger accepts the module number as an argument.
 */

/* macro that checks the bit map */

#ifdef _KERNEL
#define FAST_COND(num, mod)	  (mod != FAST_TRIG_MOD ||\
				  trig_bit_map[(num)/8] & (1 << ((num)%8)))
#else
#define FAST_COND(num, mod)	  1	/* no bit map in user space */
#endif


#define OR_TRIG(num)			||try_trigger(num,NIL,NIL)
#define OR_UTRIG(num,mod)		||(FAST_COND(num,mod) && \
					   utry_trigger(num,mod,NIL,NIL))

#define OR_TRIG2(num,aux1,aux2)		||try_trigger(num,aux1,aux2)
#define OR_UTRIG2(num,mod,aux1,aux2)	||(FAST_COND(num,mod) && \
					   utry_trigger(num,mod,aux1,aux2))

#define OR_TRIG3(var,num)		||var=try_trigger(num,NIL,NIL)

#define OR_UTRIG3(var,num,mod)		||(var=utry_trigger(num,mod,NIL,NIL))

#define AND_NOT_UTRIG(var,num,mod)	&& !(var=utry_trigger(num,mod,NIL,NIL))
#define AND_NOT_UTRIG2(num,mod)		&& !(utry_trigger(num,mod,NIL,NIL))

#define TRIG(var,num)			var=try_trigger(num,NIL,NIL)
#define UTRIG(var,num,mod)		{ if FAST_COND(num,mod) \
					  var=utry_trigger(num,mod,NIL,NIL)\
					}

#define TRIG2(var,num,aux1,aux2)	var=try_trigger(num,aux1,aux2)
#define UTRIG2(var,num,mod,u1,u2)	{ if FAST_COND(num,mod) \
					  var=utry_trigger(num,mod,aux1,aux2)\
					}

#define TRIG_RET(var,num)		var=try_trigger(num,NIL,NIL);\
					{ if(var != E_NO_ERROR) \
					    return(var);\
					}

#define TRIG_RET2(num)			{ int temp;\
					  temp = try_trigger(num,NIL,NIL);\
					  if( temp != E_NO_ERROR )\
					    return( temp );\
					}
			     
#define UTRIG_RET(var,num,mod)		{ if FAST_COND(num,mod)\
					  var=utry_trigger(num,mod,NIL,NIL);\
					  if(var != E_NO_ERROR) \
					    return(var);\
					}
			     
#define SAVE_TRIG(var,num)		{ int temp;\
					  temp=try_trigger(num,NIL,NIL);\
					  if( temp )\
					    var = temp;\
					}

#define USAVE_TRIG(var,num,mod)		{ int temp;\
					 if FAST_COND(num,mod) {\
					  temp=utry_trigger(num,mod,NIL,NIL);\
					  if( temp )\
					   var = temp;\
					}

/* The TRIG2_OR macro can be placed at the beginning of a logical expression
 * to cause the else branch to be taken by forcing the condition to fail.
 * It differs from OR_UTRIG2 by not letting the rest of the logical 
 * expression execute.
 */
#define TRIG2_OR(A, mod_num, C, D)  (utry_trigger(A,mod_num,C,D))|| 
#define NOT_UTRIG_AND(num,mod)	    !(utry_trigger(num,mod,NIL,NIL))&&

#define RESET_MODNAME			{ my_modname=old_modname; }

#endif /* CONFIG_HP200 || __hp9000s800 */


/* Triggers don't exist for the VAX or the 150 PC.  The following defines
 * will allow the code to compile with NTRIGGER on nonethelesss.
 */

#ifdef CONFIG_VAX
#define SET_MY_MODNAME(x)
#define RESET_MODNAME
#define try_trigger(x,y,z)		0
#define utry_trigger(x,mod,y,z)		0
#endif

#ifdef CONFIG_PC
#define try_trigger(x,y,z)		0
#define utry_trigger(x,mod,y,z)		0
#endif

#else /* (not NTRIGGER -- magic disappearing macros) */

#define OR_TRIG(x)		
#define OR_UTRIG(x,mod)		

#define OR_TRIG2(x,y,z)		
#define OR_UTRIG2(x,mod,y,z)	

#define TRIG(x,y)		
#define UTRIG(x,y,mod)		

#define TRIG2(x,y,u1,u2)	
#define UTRIG2(x,y,mod,u1,u2)	

#define OR_UTRIG3(var,num,mod) 

#define AND_NOT_UTRIG(var,num,mod)
#define AND_NOT_UTRIG2(num,mod)

#define TRIG_RET(x,y)		
#define UTRIG_RET(x,y,mod)		 
			     
#define SAVE_TRIG(x,y)		
#define USAVE_TRIG(x,y,mod)		

#define TRIG2_OR(A, mod_num, C, D)
#define NOT_UTRIG_AND(num,mod)

#endif	/* NTRIGGER */

/*------------------------- STRUCTURES -------------------------*/

typedef	 struct	 tag {

	 struct	 tag  *next_trig; 
	 int	 trig_mod;
	 int	 trig_num;
	 int	 trig_cnt;
	 int	 trig_retval;
	 int	 pop_cnt;
	 int	 aux1;
	 int	 aux2;
	 struct	 tag  *next_free;

	 } trig_rec;

struct	param_type {

	 int	 trig_mod;
	 int	 trig_num;
	 int	 trig_cnt;
	 int	 trig_retval;
	 int	 pop_cnt;
	 int	 aux1;
	 int	 aux2;

	 };

typedef	 struct {
	 
	 trig_rec  *table [HASH_TABLE_SIZE];

	 } hash_type;

typedef	 struct	 {

	 int	 cur_mod;
	 int	 pop_cnt;
	 int	 assert_fail;	/* number of assert failures */
  /*
  *  more to come
  */

	 } global_trig_rec;

extern char trig_bit_map[];	/* bit map for fast triggers */
/* extern int my_modname; */		/* a temporary hiding spot */
/* ------------------ end of trigdef.h --------------------------- */

#endif /* not _SYS_TRIGDEF_INCLUDED */
