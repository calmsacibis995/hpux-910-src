/*
 * @(#)nusinf.h: $Revision: 35.2 $ $Date: 88/03/02 16:20:24 $
 * $Locker:  $
 * 
 */

/******************************************************************************
 *
 *			    usinf.h
 *
 *
 *    AUTHOR	      : Tim DeLeon
 *    DATE BEGUN      : February 28, 1884
 *
 *    MODULE	      : architecture
 *    PROJECT	      : saw_project
 *    REVISION	      : 2.3
 *    SCCS NAME	      : /users/fh/saw/sccs/arch/header/s.nusinf.h
 *    LAST DELTA DATE : 85/05/28
 *
 *    DESCRIPTION     : This file contains the user interface data structures.
 *
 *
 *****************************************************************************/

/* The following definitions are used to determine which network routine 
 * should be called upon entering the unix kernel.
 *
 *	NOTE: It is assumed that NBFA is always on if NTRIGGER is on.
 */
#ifdef NTRIGGER
#define	NBFA	1
#endif

#define KERN_ENV_ROUT			1000
#define RFA_NETUNAM_ID			0+KERN_ENV_ROUT
#define RFA_SRV_FCHDIR_ID		1+KERN_ENV_ROUT
#define RFA_SRV_CHANGE_USER_ID		2+KERN_ENV_ROUT
#define RFA_SRV_RETURN_TO_YOURSELF_ID	3+KERN_ENV_ROUT
#define RFA_SRV_RFA_NOT_ALLOWED_ID	4+KERN_ENV_ROUT
#define RFA_SRV_SETREMOTE_ID		5+KERN_ENV_ROUT

/*
 * for BFA
 */
#define BFA_COPY_ID			19+KERN_ENV_ROUT
#define BFA_ZERO_ID			20+KERN_ENV_ROUT

/*
 * for TRIGGERS
 */
#define KSET_TRIGGER_ID			21+KERN_ENV_ROUT
#define KCLEAR_TRIGGER_ID		22+KERN_ENV_ROUT
#define KTRY_TRIGGER_ID			23+KERN_ENV_ROUT
#define KTRIGGER_DUMP_ID		24+KERN_ENV_ROUT
#define KUSET_ID			25+KERN_ENV_ROUT
#ifdef hp9000s800
#define KMATCH_TRIGGER_ID		26+KERN_ENV_ROUT
#endif hp9000s800


#ifdef hp9000s200
/*
 * The following are entry points which are used exclusively for the
 * Berkeley 4.2 socket emulation.
 */
#define SOCKET_ID			6+KERN_ENV_ROUT
#define LISTEN_ID			7+KERN_ENV_ROUT
#define BIND_ID				8+KERN_ENV_ROUT
#define ACCEPT_ID			9+KERN_ENV_ROUT
#define CONNECT_ID			10+KERN_ENV_ROUT
#define SRECV_ID			11+KERN_ENV_ROUT
#define SSEND_ID			12+KERN_ENV_ROUT
#define SHUTDOWN_ID			13+KERN_ENV_ROUT
#define GETSOCKNAME_ID			14+KERN_ENV_ROUT
#define SETSOCKOPT_ID			15+KERN_ENV_ROUT
#define SENDTO_ID			16+KERN_ENV_ROUT
#define RECVFROM_ID			17+KERN_ENV_ROUT
#define GETPEERNAME_ID			18+KERN_ENV_ROUT

/*
 * for NS IPC
 */

#define IPCCONNECT_ID			26+KERN_ENV_ROUT
#define IPCCONTROL_ID			27+KERN_ENV_ROUT
#define IPCCREATE_ID			28+KERN_ENV_ROUT
#define IPCDEST_ID			29+KERN_ENV_ROUT
#define IPCRECV_ID			30+KERN_ENV_ROUT
#define IPCRECVCN_ID			31+KERN_ENV_ROUT
#define IPCSELECT_ID			32+KERN_ENV_ROUT
#define IPCSEND_ID			33+KERN_ENV_ROUT
#define IPCSHUTDOWN_ID			34+KERN_ENV_ROUT

/*
 *  Socket routine that should be moved up at some later time.
 */

#define GETSOCKOPT_ID			35+KERN_ENV_ROUT
/* added for 6.2 ... prabha 02/15/88 */
#define IPCNAME_ID			36+KERN_ENV_ROUT
#define IPCNAMERASE_ID			37+KERN_ENV_ROUT
#define IPCLOOKUP_ID  			38+KERN_ENV_ROUT
#define IPCRECVREQ_ID 			39+KERN_ENV_ROUT
#define IPCSENDREPLY_ID			40+KERN_ENV_ROUT
#define IPCSENDREQ_ID 			41+KERN_ENV_ROUT
#define IPCRECVREPLY_ID			42+KERN_ENV_ROUT
#define IPCSENDTO_ID  			43+KERN_ENV_ROUT
#define IPCRECVFROM_ID			44+KERN_ENV_ROUT
#define IPCMUXCONNECT_ID		45+KERN_ENV_ROUT
#define IPCMUXRECV_ID 			46+KERN_ENV_ROUT
#define IPCGIVE_ID    			47+KERN_ENV_ROUT
#define IPCGETSOCK_ID 			48+KERN_ENV_ROUT

#endif hp9000s200
/* The routines associated with the following definitions are called via
 * the exec_in_net_envt call.
 */

#define EXEC_ENV_ROUT		9999		/* Must be > KERN_ENV_ROUT */
#define NPOWERUP_ID		0+EXEC_ENV_ROUT
#define READ_NET_CONFIG_ID	1+EXEC_ENV_ROUT
#define READ_NUSERS_ID		2+EXEC_ENV_ROUT
#define SET_NUSERS_ID		3+EXEC_ENV_ROUT

#define SI_MUX_CONNECT		4+EXEC_ENV_ROUT
#define SI_CONNECT		5+EXEC_ENV_ROUT
#define SI_RECVCN		6+EXEC_ENV_ROUT
#define SI_MUX_RECV		7+EXEC_ENV_ROUT
#define SI_SEND			8+EXEC_ENV_ROUT
#define SI_RECV			9+EXEC_ENV_ROUT
#define SI_CHG_OWNER		10+EXEC_ENV_ROUT
#define SI_IOWAIT		11+EXEC_ENV_ROUT
#define SI_SHUTDOWN		12+EXEC_ENV_ROUT
#define SI_GET_ADDR		13+EXEC_ENV_ROUT
#define SI_IS_THIS_ME		14+EXEC_ENV_ROUT
#define SI_BUF_SIZE		15+EXEC_ENV_ROUT
#define SI_MUX_REF		16+EXEC_ENV_ROUT

#ifdef hp9000s800
#ifdef QA
#define DTON_ID			17+EXEC_ENV_ROUT
#define DTOFF_ID		18+EXEC_ENV_ROUT
#define REG_TEST_ID		19+EXEC_ENV_ROUT
#define USINF_TEST_ID		20+EXEC_ENV_ROUT
#define EG_COMM_ID		21+EXEC_ENV_ROUT
#define NETWORK_TH_ID		22+EXEC_ENV_ROUT
#define MUNGE_ID		23+EXEC_ENV_ROUT
#define TCP_TUNER_ID		24+EXEC_ENV_ROUT
#define TICL_FUNC_ID		25+EXEC_ENV_ROUT
#define NLOG_ID			26+EXEC_ENV_ROUT
#define MEM_AVAIL_ID		27+EXEC_ENV_ROUT
#endif QA
#endif hp9000s800

#ifdef hp9000s200
#define SI_LISTEN		17+EXEC_ENV_ROUT
/*
 * two dummy spaces
 */
#define SI_DUMMY18		18+EXEC_ENV_ROUT
#define SI_DUMMY19		19+EXEC_ENV_ROUT
/*
 * these next two are ifdef'd in nusinf.c (NARPA and NGRACEFUL)
 */
#define SI_CNTL			20+EXEC_ENV_ROUT
#define SI_CLOSE		21+EXEC_ENV_ROUT

/*
 * under ifdef NINET in nusinf.c
 */
#define IP_CONFIG		22+EXEC_ENV_ROUT

/*
 * NPERFORM id
 */
#define TCP_TUNER_ID		23+EXEC_ENV_ROUT

/*
 * QA only id's
 */
#define DTON_ID			24+EXEC_ENV_ROUT
#define DTOFF_ID		25+EXEC_ENV_ROUT
#define REG_TEST_ID		26+EXEC_ENV_ROUT
#define USINF_TEST_ID		27+EXEC_ENV_ROUT
#define EG_COMM_ID		28+EXEC_ENV_ROUT
#define NETWORK_TH_ID		29+EXEC_ENV_ROUT
#define MUNGE_ID		30+EXEC_ENV_ROUT
#define TICL_FUNC_ID		31+EXEC_ENV_ROUT
#define NLOG_ID			32+EXEC_ENV_ROUT
#define MEM_AVAIL_ID		33+EXEC_ENV_ROUT

/*
 * New additions for npower related functions
 */

#define READ_INTERFACES		34+EXEC_ENV_ROUT
#define ERRLOG_ID		35+EXEC_ENV_ROUT

/*
 * Netstat related functions
 */

#define READ_NUM_SOCKETS	36+EXEC_ENV_ROUT
#define READ_SOCKETS_INFO	37+EXEC_ENV_ROUT
#define READ_NET_STATS		38+EXEC_ENV_ROUT
#define READ_MEM_STATS		39+EXEC_ENV_ROUT
#define DUMP_TRACE		40+EXEC_ENV_ROUT

#endif hp9000s200


/*
 *	These constants and typedefs are used in the testing of the nusinf.c 
 *	module.
 */

#ifdef QA
#define NUSINF_TEST	500
#define ERR_RET		1+NUSINF_TEST
#define PID_RET		2+NUSINF_TEST
#define COPY_CHK	3+NUSINF_TEST


#define NDATA_SIZE	400
#define BDATA_SIZE	1600

typedef struct	{
	int	bound1;
	int	fdata[NDATA_SIZE];
	int	bound2;
	int	tdata[NDATA_SIZE];
	int	bound3;
} copyt_struct;


/*	This structure is passed from user space.	*/

typedef struct	{
	anyptr	ufrom_ptr;
	anyptr	uto_ptr;
	int	pattern;
} upass_struct;


/*	This structure is passed from user space.	*/

typedef struct	{
		int	nusinf_test;
		int	gen_usage;
} uparm_struct;

#endif
