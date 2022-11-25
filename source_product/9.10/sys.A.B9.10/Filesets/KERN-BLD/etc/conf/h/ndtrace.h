/*
 * @(#)ndtrace.h: $Revision: 1.7.83.4 $ $Date: 93/09/17 18:30:44 $
 * $Locker:  $
 * 
 */

#ifndef _SYS_NDTRACE_INCLUDED
#define _SYS_NDTRACE_INCLUDED

/******************************************************************************
 ******************************************************************************
 **				DTRACE.H	       
 **
 **  AUTHOR:		Tim DeLeon
 **
 **  DATE BEGUN:	13 Mar 84
 **
 **  MODULE:		architecture
 **  PROJECT:		leaf_project
 **  REVISION:		2.9
 **  SCCS NAME:		/users/fh/saw/sccs/arch/header/s.ndtrace.h
 **  LAST DELTA DATE:	85/06/20
 **
 **  DESCRIPTION:	Definitions of all dtrace defines used throughout the
 *			network architecture.
 **			
 ******************************************************************************
 *****************************************************************************/
#ifdef CONFIG_VAX
#define __LINE__	0
#define __FILE__	NIL
#endif

#define NO_VALUE	0

/*	These defines are used to identify which procedure is being called.
 *	The specific use of these defines can be seen in the identifying traffic
 *	discussion.
 */

#define NIL_PROC_ID	0
#define INIT_CONN_ID	1
#define ACK_CONN_ID	2
#define INIT_SEND_ID	3
#define ACK_SEND_ID	4
#define INIT_RCV_ID	5
#define ACK_RCV_ID	6
#define CONN_ID		7
#define SEND_ID		8
#define RCV_ID		9
#define ERROR_ID	10
#define ABORT_ID	11
#define T_O_ID		12
#define ACK_CR_ID	13
#define CONFIG_ID	14

/**/
/*	These defines are used to identify the module using the justlog and 
 *	warning macros.
 */

#define NULL_MOD	0
#define LAN_MOD		1
#define ASYNC_MOD	2
#define DIRECT_MOD	3
#define PROBE_MOD	4
#define IP_MOD		5
#define TCP_MOD		6
#define SI_MOD		7
#define BB_MOD		8

#define NFT_MOD		10
#define RFAR_MOD	11
#define RFAS_MOD	12

#define TEST_HARN_MOD	15
#define EG_MOD		16

#define NMGR_MOD	17
#define OSINT_MOD	18
#define INTER_MOD	19
#define TIMER_MOD	20
#define PROT_MOD	21
#define PORT_MOD	22
#define POOL_MOD	23
#define PATH_MOD	24
#define TASK_MOD	25
#define INIT_MOD	26
#define MEM_MOD		27

#define RFAS_DAEMON_MOD 30
#define RFAS_INIT_MOD	31
#define TICL_MOD	32
#define DIAG_MOD	33
#define RLB_DAEMON_MOD	34
#define RLB_SERVER_MOD	35


/*	The following defines are used along with the dtrace array.
 *	Each mask is a bit position in the dtrace array.
 */


/*	This displays the first line of the traffic information.	*/
#define ALL_PROC_INTERFACE	1	

/*	This displays all information associated with procedure calls.	*/
#define ALL_PROC_INFO		2	

/*	This will allow justlog errors to be halted on.			*/
#define HALT_ON_JUSTLOG		3

/*	This will allow warning errors to be halted on.			*/
#define HALT_ON_WARNING		4

/*	This will display all debug information to the system console.	*/
#define DISP_DEBUG_TO_CONSOLE	5

/*	This will turn off all display to screen.			*/
#define NO_DISP			6

/*	This will turn off displaying of JUSTLOGS.			*/
#define TURN_OFF_JUSTLOGS	7

/*	This bit will turn on network logging iff log memory is allocated  */
#define LOGIT			8

/*	This bit will turn on tracing in the log facility		*/
#define LOGGER			9


/*	The following dtrace bits are used to configure the TCP test
 *	harness.  
 */

#define CONFIGURE_TCP_TH	10	/* Configures in the TCP TH.	*/
#define EXGEN_BELOW_TCP		11	/* Configure EG below TCP.	*/
#define EXGEN_BELOW_IP		12	/* Configure EG below IP.	*/


/* dtrace masks 20-25 reserved for Pool module.			*/

#define POOL_MOD_OPER	20		/* Pool module operations.	     */
#define GET_RET_BFFR	21		/* Get and Return operations.	     */


/* dtrace masks 26-35 reserved for Path module.			*/

#define PATHS_MODIFIED	26		/* Displays paths lists for changes. */
#define CREATED_CSRS	27		/* Displays newly created csrs.	     */
#define ADDR_PATH_REQ	28		/* Displays addr_list for csr reqs.  */
#define FIND_IPATH_INFO 29		/* Displays find_inbound_path info.  */
#define TEST_VERBOSE	30		/* Displays all test status.	     */
#define NODE_NAMES	31		/* Displays node name parameters.    */


/* dtrace masks 36-40 reserved for Usinf module.			*/

#define US_TEST_VER		36	/* Displays usinf information.	     */
#define ERROR_DISP		37	/* Displays neterr, errno, errnet.   */
#define NO_PROCESS_SWITCH	38	/* VAX: allows debugging in 1 process*/
#define IGNORE_TRAPS		39	/* VAX: dont disable ctl-y, AST deliv*/


/* dtrace masks 41-45 reserved for Port module.			*/

#define PORT_DISP	41		/* Displays port operations.	     */


/* task/protect masks 46-55 reserved for task/protect module.	*/

#define TASK_SCHED_OPR	46		/* Display scheduling info.	     */
#define SLP_WAK_OPR	47		/* Wakeup and sleep operations.	     */
#define PROT_OPR	48		/* Protect operations.		     */
#define DPT_BIT_TRACE	49		/* Display the dflags.		     */
#define PROT_VAX	50		/* Display VAX protect details	     */
#define DISP_VAX	51		/* Display VAX dispatch details	     */

/* timer masks 56-60 reserved for timer module.			*/

#define TIMER_TRACE	56		/* Display timer info.		    */
#define TIMER_DISP	57		/* Display get_network_time info.   */
#define TIMER_VOID	58		/* Abort wakemein/cancel_wkp calls. */
#define TIMER_MTRACE	59		/* Display more timer info.	    */
#define TIMEOUT_CALLS	60		/* Display all calls to timeout.    */


/* npowerup masks 61-65 reserved for npowerup module.		*/

#define NPOWERUP_TRACE		61	/* Display npowerup info.	    */
#define NETMAIN			62	/* VAX: display network main info   */
#define DONT_INIT_PROTO_MODS	63	/* stubs out init_proto_modules	    */
#define IGNORE_PROCESS_CREATE	64	/* For attaching terminals to process*/
#define IGNORE_NFTDAEM_CREATE	65	


/* npowerup masks 66-70 reserved for network memory module.	*/

#define ALLOC_DEALLOC_OPR	66	/* De/allocate operations.	    */

/* Architecture consistency checks.					*/
#define VERIFY_PROT_LIST	71
#define VERIFY_MEM_LIST		72
#define VERIFY_POOL_LIST	73


/* dtrace masks 100-120 reserved for SI module.				*/

#define SI_TRAFFIC		100	/* All SI traffic displayed */
#define TRACE_SI_SEND		101
#define TRACE_SI_INIT_RCV	102
#define SI_USER_REC		103	/* Displays user record activity    */
#define SI_TRACE_BELOW		104	/* Displays calls to lower protocol */
#define SI_WAKEUP		105	/* Displays wakeup calls	    */
#define SI_PARAMETERS		106	/* Prints out the user's parameters */
#define SI_TRACE_SLEEP		107	/* Displays dispatch calls.	    */
#define SI_NO_REQ_REPLY		108
#define SI_ALLOCATE		109	/* Traces buffer allocation requests */
#define SI_BUF_SIZE_TRACE	110	/* Trace buf size calls		*/

/* dtrace masks 121-140 reserved for TCP module.			*/

#define BB			121	 /* Bounce Back */
#define IP_BIT			122	 /* IP */
#define TCP			123
#define TEST_HARN		124	 /* TCP test harness - upper level */
#define EG			125	 /* TCP exception generator */
#define TCP_TRACE		126	 /* Trace through tcp functions	 */
#define TCP_STATES		127	 /* Print out TCP state vector	 */
#define TCP_QUEUES		128	 /* Print out TCP queues	 */
#define TCP_SLOW		129	 /* Slow down TCP retransmits	 */
#define TCP_HEADERS		130	 /* Print out TCP headers	 */
#define TCP_RETRANS		131	 /* Print out TCP retransmits	 */
#define TCP_TOFF		132	 /* turn off  TCP retransmits	 */
#define TEMP_ARCHS		133	 /* temp architecture - for user env */
#define TCP_CPU_TIME		134	 /* performance measurement bit	     */
#define TCP_IDLE_OFF		135	 /* turn off tcp idle timer	     */



/* dtrace masks 141-150 reserved for IP module.				*/

/* dtrace masks 151-170 reserved for Driver module.			*/


#define DRV_CALLS		151	/* print each procedure call	*/
#define DRV_DUMP_RCV		152	/* dump out each receive frame	*/
#define DRV_DUMP_SEND		153	/* dump out each transmit frame */
#define DRV_RCV_TYPE		154	/* print out type frame received*/

#define DRV_NOCARD		158	/* no card available		*/
#define DRV_DUP_DADDR		159	/* dest addr = src addr		*/


#define HW_CALLS		161	/* print each procedure call	*/
#define HW_LOGSTATS		162	/* print on special stats	*/
#define HW_PROMISCUOUS		163	/* put in promiscuous mode	*/
#define HW_CALLS_INT		164	/* print details in hw interf.	*/
#define HW_ADDR2		169	/* use a secondary address	*/


/* dtrace masks 171-190 reserved for Direct Access module.		*/

#define TRACE_GLOBAL_REC	171	/* print out global record	*/
#define TRACE_STATE_REC		172	/* print out state record	*/
#define TRACE_IOCTL_DATA	173	/* display ioctl data		*/
#define TRACE_RWDATA		174	/* display read/write data	*/
#define TRACE_FCM_LIST		175	/* display FCM list		*/
#define TRACE_FP		188	/* trace file pointer values	*/
#define TRACE_PCALLS		189	/* trace direct access calls	*/


/* dtrace masks 191-210 reserved for Probe module.			*/

#define PR_COMPARE_PATHS	191
#define PR_DUMP_REQUEST		192
#define PR_DUMP_REQ_LIST	193
#define PR_DUMP_GLOBALS		194
#define PR_TRACE_INBOUND	195
#define PR_TRACE_OUTBOUND	196
#define PR_PROC_TRACE		197
#define PR_REXMIT_TRACE		198
#define PR_OVERRIDE_SERVICES	199
#define PR_FORCE_802_PATH	200
#define PR_BIG_TIMEOUT		201
#define PR_FORCE_TCP_CHECKSUM	202

/* dtrace masks 211-230 reserved for RFA REQUESTOR module.		*/

#define RFA_DT_LOGIN		211	/* logins, logouts, etc		*/
#define RFA_DT_MEM		212	/* mem alloc and de-alloc	*/
#define RFA_DT_PROC		213	/* function entry/exit		*/
#define RFA_DT_SI		214	/* SI calls and results		*/
#define RFA_DT_PACKET		215	/* packet building and decoding */
#define RFA_DT_ERRMAP		216	/* error_map calls		*/
#define RFA_DT_ASSERT		217	/* info related to assertions	*/
#define RFA_DT_218		218	/*				*/
#define RFA_DT_219		219	/*				*/
#define RFA_DT_220		220	/*				*/
#define RFA_DT_221		221	/*				*/
#define RFA_DT_222		222	/*				*/
#define RFA_DT_223		223	/*				*/
#define RFA_DT_224		224	/*				*/
#define RFA_DT_225		225	/*				*/
#define RFA_DT_226		226	/*				*/
#define RFA_DT_227		227	/*				*/
#define RFA_DT_228		228	/*				*/
#define RFA_DT_229		229	/*				*/
#define RFA_DT_KLUDGE_NODENAME	230	/* temporary fix		*/

/* dtrace masks 231-250 reserved for RFA SERVER module.			*/

/* dtrace masks 251-270 reserved for TICL module.			*/

#define TI_PROC_TRACE		251	/* procedure entry trace	*/
#define TI_PROC_EXIT		252	/* procedure exit trace		*/
#define TI_GLOB_TRACE		253	/* TICL globals record		*/
#define TI_TCB_TRACE		254	/* transport control block dump */
#define TI_ACB_TRACE		255	/* asynchronous control block	*/
#define TI_CMD_TRACE		256	/* command entry block dump	*/

/* dtrace masks 271-290 reserved for NFT module.			*/
#define INIT_OPR		271	/* trace connection initiation	*/
#define NFTDAEM_TRACE		272	/* trace all nftdaem activity   */
#define NFT_PACKET_TRACE	275	/* display nft packets          */
#define DSCOPY_TRACE		276	/* trace dscopy activity        */
#define NFT_READ_TRACE		277	/* trace dscopy file reads      */

/* max possible dtrace bit is currently 399 (7/24/84) see narchdef.c	*/



typedef byte	modnametype;

/**/
/*
 *	Macros required are defined here.
 */
/*
 * Need these for triggers, not QA; the way these are defined, cannot have QA
 * without triggers and use modname.
 */
#ifdef NTRIGGER

#define SET_MY_MODNAME( modname) { old_modname=my_modname;my_modname=modname;}
#define USET_MY_MODNAME( modname) uset_my_modname( modname);
#define RESET_MODNAME	{ my_modname=old_modname; }

#else

#define SET_MY_MODNAME(modname)
#define USET_MY_MODNAME(modname)
#define RESET_MODNAME

#endif

/*
 * ERROR macro used for QA on and off, to display to user potential panic
 * situations
 */
#define ERROR(cond, err, csrp, level, loc) {if (cond) printf("ERROR:  \
err: %d  csrp: = %d  level: %d  loc: %d\n", err, csrp, level, loc);}

#ifdef QA

/* global memory which must be allocated for the log facility */
extern char	     *start_trace_ptr;
extern char	     *last_trace_ptr;
extern char	     *curr_trace_ptr;
extern char	     *dump_trace_ptr;
extern void	     mprintf ();

#define TESTBIT( DT_BIT, func)	{ if ( dtrace_array[(DT_BIT)/8] & ( 1 << ( (DT_BIT) % 8)) ) { func } }
#define BITON( DT_BIT )	 ( dtrace_array[(DT_BIT)/8] & ( 1 << ( (DT_BIT) % 8)) )
#define BITOFF( DT_BIT ) !BITON( DT_BIT )
#define DTON( DT_BIT)	{ dtrace_array[(DT_BIT)/8] |= (1 << ((DT_BIT) % 8));}
#define DTOFF( DT_BIT)	{ dtrace_array[(DT_BIT)/8] &= ~(1 << ((DT_BIT) % 8));}

#define WARNING( cond, err, csrp, minfo, loc) { if (cond) ndsp_warn(err,csrp,minfo,__LINE__, __FILE__, loc);}
#define JUSTLOG( cond, err, csrp, minfo, loc) { if (cond) ndsp_jlog(err,csrp,minfo,__LINE__, __FILE__, loc);}

#define DTRACE_TRAFFIC( p, m_typ, cp, mlvl, d_st, d_sz, pb) ndsp_traffic( p, m_typ, cp, mlvl, d_st, d_sz, pb);


#else

#define TESTBIT( DT_BIT, func)	
#define BITON( DT_BIT ) 
#define DTON( DT_BIT)		FALSE
#define DTOFF( DT_BIT)		TRUE

#define WARNING( cond, error_num, csrp, more_info, loc )
#define JUSTLOG( cond, error_num, csrp, more_info, loc )

#define DTRACE_TRAFFIC( p, m_typ, cp, mlvl, d_st, d_sz, pb)

#endif

#ifdef QA

#ifndef CONFIG_USER
#define printf	net_printf
#define nprintf net_printf
#define PRINTF	printf
#endif

#endif /* QA */

/* ---------------------- End of ndtrace.h ------------------------------ */

#endif /* not _SYS_NDTRACE_INCLUDED */
