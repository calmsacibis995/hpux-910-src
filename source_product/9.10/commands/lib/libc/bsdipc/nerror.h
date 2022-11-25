/*
 * @(#)nerror.h: $Revision: 35.1 $ $Date: 88/02/10 14:05:55 $
 * $Locker:  $
 * 
 */

/******************************************************************************
 ******************************************************************************
 **				NERROR.H	       
 **
 **  AUTHOR:		Carl Dierschow, David Hendricks, Tim DeLeon
 **
 **  DATE BEGUN:	5 Mar 84
 **
 **  MODULE:		arch
 **  PROJECT:		leaf
 **  REVISION:		2.4
 **  SCCS NAME:		/users/fh/leaf/sccs/arch/header/s.nerror.h
 **  LAST DELTA DATE:	85/06/19
 **
 **  DESCRIPTION:	Definitions of all error numbers used throughout the
 *			network architecture.
 **			
 ******************************************************************************
 *****************************************************************************/

/*
 *			Error management philosophy:
 *
 *	The error values are set up so they could also work with the unix
 *	errno and errnet functions.  These error values are managed by
 *	Timo.  Do not add, delete, or modify without contacting me first.
 *	The errno and errnet mapping is shown below for the various network
 *	errors.
 */

#define E_NO_ERROR			0

/* -------------------------------------------------------------------- */
/*	errno = EPERM							*/
/* -------------------------------------------------------------------- */

#define E_FIRST_NETERR			500	/* First valid neterr	*/
#define E_NOT_OWNER			E_FIRST_NETERR
#define E_NOT_SUPER_USER		E_FIRST_NETERR+1
#define E_RFA_EPERM			E_FIRST_NETERR+2


/* -------------------------------------------------------------------- */
/*	errno = ENOENT							*/
/* -------------------------------------------------------------------- */

#define E_RFA_ENOENT			510


/* -------------------------------------------------------------------- */
/*	errno = ESRCH							*/
/* -------------------------------------------------------------------- */

#define E_RFA_ESRCH			520


/* -------------------------------------------------------------------- */
/*	errno = EINTR							*/
/* -------------------------------------------------------------------- */

#define E_SIGNAL_RECEIVED		530
#define E_RFA_EINTR			E_SIGNAL_RECEIVED+1


/* -------------------------------------------------------------------- */
/*	errno = EIO							*/
/* -------------------------------------------------------------------- */

#define E_IO_ERROR			540	/* Direct Access	*/
#define E_READ_TIMEOUT			E_IO_ERROR+1
#define E_RFA_EIO			E_IO_ERROR+2


/* -------------------------------------------------------------------- */
/*	errno = ENXIO							*/
/* -------------------------------------------------------------------- */

#define E_BAD_CONNECTION_ID		550	/* Direct Access	*/
#define E_NO_DEST_ADDR			E_BAD_CONNECTION_ID+1	/* DA	*/
#define E_RFA_ENXIO			E_BAD_CONNECTION_ID+2
#define E_DA_NETWORK_DOWN		E_BAD_CONNECTION_ID+3	/* DA	*/


/* -------------------------------------------------------------------- */
/*	errno = E2BIG							*/
/* -------------------------------------------------------------------- */

#define E_RFA_E2BIG			560


/* -------------------------------------------------------------------- */
/*	errno = EBADF							*/
/* -------------------------------------------------------------------- */

#define E_RFA_EBADF			570


/* -------------------------------------------------------------------- */
/*	errno = ENOMEM							*/
/* -------------------------------------------------------------------- */

#define E_OUT_OF_MEMORY			580
#define E_BAD_MEM_RANGE			E_OUT_OF_MEMORY+1
#define E_OUT_OF_PHYSICAL_MEM		E_OUT_OF_MEMORY+2
#define E_SI_TOO_MANY_SENDS		E_OUT_OF_MEMORY+3
#define E_SI_TOO_MANY_RECVS		E_OUT_OF_MEMORY+4
#define E_OUT_OF_PORTS			E_OUT_OF_MEMORY+5
#define E_RFA_ENOMEM			E_OUT_OF_MEMORY+6
#define E_ARP_TOO_MANY_REQ		E_OUT_OF_MEMORY+7


/* -------------------------------------------------------------------- */
/*	errno = EACCES							*/
/* -------------------------------------------------------------------- */

#define E_RFA_EACCES			600


/* -------------------------------------------------------------------- */
/*	errno = EFAULT							*/
/* -------------------------------------------------------------------- */

#define E_RFA_EFAULT			610
#define E_BAD_POINTER			E_RFA_EFAULT+1


/* -------------------------------------------------------------------- */
/*	errno = ENOTBLK							*/
/* -------------------------------------------------------------------- */

#define E_RFA_ENOTBLK			620


/* -------------------------------------------------------------------- */
/*	errno = EBUSY							*/
/* -------------------------------------------------------------------- */

#define E_RFA_EBUSY			630
#define E_DUPLICATE_CONNECTION		631


/* -------------------------------------------------------------------- */
/*	errno = EEXIST							*/
/* -------------------------------------------------------------------- */

#define E_RFA_EEXIST			640


/* -------------------------------------------------------------------- */
/*	errno = EXDEV							*/
/* -------------------------------------------------------------------- */

#define E_RFA_EXDEV			650


/* -------------------------------------------------------------------- */
/*	errno = ENODEV							*/
/* -------------------------------------------------------------------- */

#define E_RFA_ENODEV			660


/* -------------------------------------------------------------------- */
/*	errno = ENOTDIR							*/
/* -------------------------------------------------------------------- */

#define E_RFA_ENOTDIR			670


/* -------------------------------------------------------------------- */
/*	errno = EISDIR							*/
/* -------------------------------------------------------------------- */

#define E_RFA_EISDIR			680


/* -------------------------------------------------------------------- */
/*	errno = EINVAL							*/
/* -------------------------------------------------------------------- */

#define E_BAD_LENGTH			690
#define E_EINVAL			691
#define E_BAD_CHARACTER			E_EINVAL+2
#define E_BAD_NUM_FIELDS		E_EINVAL+3
#define E_BAD_NAME_NUMFIELDS		E_EINVAL+13
#define E_BAD_INET_CHAR			E_EINVAL+14
#define E_BAD_INET_RANGE		E_EINVAL+15
#define E_BAD_MEMLIMIT_CHAR		E_EINVAL+16
#define E_INVALID_VALUE			E_EINVAL+17
#define E_NULL_POINTER			E_EINVAL+18
#define E_SI_INVALID_DATA_LEN		E_EINVAL+19
#define E_SI_BAD_IODESCR		E_EINVAL+20
#define E_SI_BAD_NAME_LEN		E_EINVAL+21
#define E_SI_BAD_SEND_SIZE		E_EINVAL+22
#define E_SI_BAD_RECV_SIZE		E_EINVAL+23
#define E_SI_BAD_FLAGS			E_EINVAL+24
#define E_SI_BAD_CD			E_EINVAL+25
#define E_SI_BAD_CONN_TYPE		E_EINVAL+26
#define E_LLP_UNIMPLEMENTED		E_EINVAL+27
#define E_LLP_NO_SUCH_STAT		E_EINVAL+28
#define E_LLP_NO_BUFFER_SEND		E_EINVAL+29
#define E_NOT_INDIVIDUAL		E_EINVAL+30	/* LLP */
#define E_NOT_MULTICAST			E_EINVAL+31	/* LLP */
#define E_TOO_MANY_MULTICAST		E_EINVAL+32	/* LLP */
#define E_NOT_ON_MC_LIST		E_EINVAL+33	/* LLP */
#define E_ALREADY_ON_LIST		E_EINVAL+34	/* LLP */
				/* E_DUPLICATE_CONNECTION moved to EBUSY. */
#define E_WRONG_VTYPE			E_EINVAL+36	/* DA  */
#define E_BAD_OPEN_OPTS			E_EINVAL+37	/* DA  */
#define E_PROTOCOL_NOT_FOUND		E_EINVAL+40	/* Path*/
#define E_RFA_EINVAL			E_EINVAL+44
#define E_SI_BAD_ADDR			E_EINVAL+47
#define E_SI_BAD_TIMEOUT		E_EINVAL+48
#define E_PROTOCOL_ERROR		E_EINVAL+49	/* DA  */

/* -------------------------------------------------------------------- */
/*	errno = ENFILE							*/
/* -------------------------------------------------------------------- */

#define E_RFA_ENFILE			750


/* -------------------------------------------------------------------- */
/*	errno = EMFILE							*/
/* -------------------------------------------------------------------- */

#define E_SI_NO_CONNECTIONS		760
#define E_RFA_EMFILE			762


/* -------------------------------------------------------------------- */
/*	errno = ETXTBSY						*/
/* -------------------------------------------------------------------- */

#define E_RFA_ETXTBSY			770


/* -------------------------------------------------------------------- */
/*	errno = EFBIG							*/
/* -------------------------------------------------------------------- */

#define E_RFA_EFBIG			775


/* -------------------------------------------------------------------- */
/*	errno = ENOSPC							*/
/* -------------------------------------------------------------------- */

#define E_RFA_ENOSPC			780


/* -------------------------------------------------------------------- */
/*	errno = ESPIPE							*/
/* -------------------------------------------------------------------- */

#define E_RFA_ESPIPE			790


/* -------------------------------------------------------------------- */
/*	errno = EROFS							*/
/* -------------------------------------------------------------------- */

#define E_RFA_EROFS			800


/* -------------------------------------------------------------------- */
/*	errno = EMLINK							*/
/* -------------------------------------------------------------------- */

#define E_RFA_EMLINK			805


/* -------------------------------------------------------------------- */
/*	errno = EPIPE							*/
/* -------------------------------------------------------------------- */

#define E_RFA_EPIPE			810


/* -------------------------------------------------------------------- */
/*	errno = ENOTEMPTY						*/
/* -------------------------------------------------------------------- */

#define E_RFA_ENOTEMPTY			815


/* -------------------------------------------------------------------- */
/*	errnet = NE_INTERNAL						*/
/* -------------------------------------------------------------------- */

#define E_ERR_MAP_USE			820	/* 1st NE_INTERNAL err. */
#define E_BAD_TOKEN			E_ERR_MAP_USE+0
#define E_WAKEUP_PENDING		E_ERR_MAP_USE+1
#define E_ROUTINE_NOT_PRESENT		E_ERR_MAP_USE+2
#define E_SI_UNIMPLEMENTED		E_ERR_MAP_USE+3
#define E_SI_INTERNAL			E_ERR_MAP_USE+4
#define E_CONNECTION_NOT_FOUND		E_ERR_MAP_USE+5		/* QA only   */
#define E_CONNECTION_ATTACHED		E_ERR_MAP_USE+6		/* QA only   */
#define E_POOL_STRUCTURES_TRASHED	E_ERR_MAP_USE+7		/* QA only   */
#define E_MEMORY_STRUCTURES_TRASHED	E_ERR_MAP_USE+8		/* QA only   */
#define E_TIME_VALUE_TOO_LARGE		E_ERR_MAP_USE+9		/* QA only   */
#define E_BAD_LOCKLEVEL			E_ERR_MAP_USE+10	/* QA only   */
#define E_BAD_PORT_NUMBER		E_ERR_MAP_USE+11	/* QA only   */
#define E_GENERAL_ASSERTION_FAILURE	E_ERR_MAP_USE+12	/* QA only   */
#define E_CONN_EXISTS			E_ERR_MAP_USE+13	/* Path	     */
#define E_UNEXPECTED_ERROR_CODE		E_ERR_MAP_USE+15    
#define E_BAD_PROTECT_TOKEN		E_ERR_MAP_USE+16	/* QA only   */
#define E_TCP_CONN_EXISTS		E_ERR_MAP_USE+17
#define E_TCP_ILLEGAL_REQUEST		E_ERR_MAP_USE+18
#define E_TCP_CONN_NONEXISTENT		E_ERR_MAP_USE+19
#define E_IP_CANNOT_REASSEMBLE		E_ERR_MAP_USE+20
#define E_BAD_ADDRESS			E_ERR_MAP_USE+21	/* Path	     */
#define E_PORT_IN_USE			E_ERR_MAP_USE+22
#define E_PORT_NOT_FOUND		E_ERR_MAP_USE+23
#define E_TOO_MANY_DISPATCH_TOKENS	E_ERR_MAP_USE+24
#define E_STACK_OVERFLOW		E_ERR_MAP_USE+25	/* QA only   */
#define E_BAD_DISPATCH_TOKEN		E_ERR_MAP_USE+26	/* QA only   */
#define QA_LAN_MYLEVEL_NOT_ZERO		E_ERR_MAP_USE+27	/* driver QA  */
#define QA_LAN_PID_NOT_LAN		E_ERR_MAP_USE+28
#define QA_LAN_BUFFER_TOO_SMALL		E_ERR_MAP_USE+29
#define QA_LAN_SENDING_OVERSIZE_FRAME	E_ERR_MAP_USE+30
#define QA_LAN_TYPE_FIELD_ERROR		E_ERR_MAP_USE+31
#define QA_LAN_NO_DUMMY			E_ERR_MAP_USE+32
#define QA_LAN_PREV_NIL			E_ERR_MAP_USE+33
#define QA_LAN_2_PREV_NIL		E_ERR_MAP_USE+34
#define QA_LAN_ADDR_MC			E_ERR_MAP_USE+35
#define QA_LAN_NOT_EVEN			E_ERR_MAP_USE+36
#define QA_LAN_NEGATIVE_LEN		E_ERR_MAP_USE+37
#define QA_LAN_SIZE_NEGATIVE		E_ERR_MAP_USE+38
#define E_HW_NOT_ENP			E_ERR_MAP_USE+39	/* 200 HW QA */
#define E_HW_NOT_STP1			E_ERR_MAP_USE+40
#define E_HW_NOT_STP2			E_ERR_MAP_USE+41
#define E_HW_NOT_STP3			E_ERR_MAP_USE+42
#define E_HW_NOT_STP4			E_ERR_MAP_USE+43
#define E_HW_RXQUADWORD			E_ERR_MAP_USE+44
#define E_HW_TXQUADWORD			E_ERR_MAP_USE+45
#define E_HW_WRONG_STAT			E_ERR_MAP_USE+46	/* also VAX  */
#define E_HW_DEST_NOT_EVEN		E_ERR_MAP_USE+47
#define E_HW_SRC_NOT_EVEN		E_ERR_MAP_USE+48
#define E_HW_FRAME_RANGE		E_ERR_MAP_USE+49	/* also VAX  */
#define E_HW_IDON_SET			E_ERR_MAP_USE+50
#define E_HW_NOT_RXON			E_ERR_MAP_USE+51
#define E_HW_NOT_TXON			E_ERR_MAP_USE+52
#define E_HW_NOT_INDIVIDUAL		E_ERR_MAP_USE+53	/* also VAX  */
#define E_HW_NOT_MY_LOCAL		E_ERR_MAP_USE+54	/* also VAX  */
#define E_HW_NOT_UP			E_ERR_MAP_USE+55
#define E_HW_TX_PAST_END1		E_ERR_MAP_USE+56
#define E_HW_TX_PAST_END2		E_ERR_MAP_USE+57
#define E_HW_NOT_OWN1			E_ERR_MAP_USE+58
#define E_HW_NOT_OWN2			E_ERR_MAP_USE+59
#define E_HW_NOT_OWN3			E_ERR_MAP_USE+60
#define E_HW_NOT_OWN4			E_ERR_MAP_USE+61
#define E_HW_NO_ACK			E_ERR_MAP_USE+62
#define E_HW_WRONG_ADDR			E_ERR_MAP_USE+63
#define E_HW_WRONG_LEVEL		E_ERR_MAP_USE+64
#define E_HW_SKIP_BAD			E_ERR_MAP_USE+65
#define E_HW_TIMER_CALLED		E_ERR_MAP_USE+66
#define E_BAD_SEQN_FROM_REMOTE          E_ERR_MAP_USE+67    /* TCP */
#define E_RESET_FROM_REMOTE             E_ERR_MAP_USE+68    /* TCP */
#define E_BAD_ACK_FROM_REMOTE           E_ERR_MAP_USE+69    /* TCP */
#define E_FIN_FROM_REMOTE               E_ERR_MAP_USE+70    /* TCP */
#define E_BAD_SYN_FROM_REMOTE           E_ERR_MAP_USE+71    /* TCP */
#define E_SIMULTANEOUS_CONNECTIONS      E_ERR_MAP_USE+72    /* TCP */
#define E_TIMOUT_ON_CLOSED              E_ERR_MAP_USE+73    /* TCP */
#define E_TIMEOUT_ON_LISTEN             E_ERR_MAP_USE+74    /* TCP */
#define E_CARD_OVERFLOW                 E_ERR_MAP_USE+75    /* TCP */
#define E_IDLE_RETRY_TIMEOUT            E_ERR_MAP_USE+76    /* TCP */
#define E_DATA_RETRY_TIMEOUT            E_ERR_MAP_USE+77    /* TCP */
#define E_ACK_TIMEOUT                   E_ERR_MAP_USE+78    /* TCP */
#define E_CONNECTION_ATTEMPT_FAILED     E_ERR_MAP_USE+79    /* TCP */
#define E_BAD_INBOUND_SEG_LENGTH        E_ERR_MAP_USE+80    /* TCP */
#define E_CHECKSUM_REQUEST              E_ERR_MAP_USE+81    /* TCP */
#define E_UNKNOWN_OPTION                E_ERR_MAP_USE+82    /* TCP */
#define E_CURRENTLY_SENDING             E_ERR_MAP_USE+83    /* TCP */
#define E_WINDOW_TOO_SMALL              E_ERR_MAP_USE+84    /* TCP */
#define E_WINDOW_UPDATE                 E_ERR_MAP_USE+85    /* TCP */
#define E_CONN_TIMEOUT                  E_ERR_MAP_USE+86    /* TCP */
#define E_TCP_BAD_CHECKSUM              E_ERR_MAP_USE+87    /* TCP */
#define E_CHANGE_SEG_SIZE               E_ERR_MAP_USE+88    /* TCP */
#define E_REFERENCE_COUNT_NOT_ZERO      E_ERR_MAP_USE+89    /* PROBE */
#define E_BAD_REQUEST_LIST              E_ERR_MAP_USE+90    /* PROBE */
#define E_IP_NOT_UPPER_LAYER            E_ERR_MAP_USE+91    /* PROBE */
#define E_BAD_NETWORK_NUMBER            E_ERR_MAP_USE+92    /* PROBE */
#define E_UNRECOGNIZED_PACKET           E_ERR_MAP_USE+93    /* PROBE */
#define E_BAD_XMIT_COUNT                E_ERR_MAP_USE+94    /* PROBE */
#define E_BAD_IP_CHECKSUM               E_ERR_MAP_USE+95    /* IP */

#ifdef CONFIG_VAX
#define E_EVENTFLAG_FAILURE		E_ERR_MAP_USE+100
#define E_CANT_READ_PROCESS_STATUS	E_ERR_MAP_USE+101
#define E_CANT_MAP_TO_GLOBAL_SECTION	E_ERR_MAP_USE+102
#define E_CANT_ASSOCIATE_EVENT_FLAGS	E_ERR_MAP_USE+103
#define E_CANT_CREATE_PROCESS		E_ERR_MAP_USE+104
#define E_CANT_DELETE_PROCESS		E_ERR_MAP_USE+105
#define E_CANT_DELETE_EVENT_FLAGS	E_ERR_MAP_USE+106
#define E_CANT_CONNECT_PROCESS		E_ERR_MAP_USE+107
#define E_CANT_ASSIGN_DEVICE_CHANNEL	E_ERR_MAP_USE+108
#define E_SYSTEM_SERVICE_FAILURE	E_ERR_MAP_USE+109
#define E_CANT_CREATE_MAILBOX           E_ERR_MAP_USE+110 
#define E_HW_BUF_NOT_EVEN		E_ERR_MAP_USE+111
#define E_HW_EVENT_FLAG_ERROR		E_ERR_MAP_USE+112
#define E_INSUF_EVENT_FLAGS             E_ERR_MAP_USE+113
#define E_FREE_EVENT_FLAGS              E_ERR_MAP_USE+114
#define E_CANT_CANCEL_TIMER             E_ERR_MAP_USE+115
#define E_SETAST_FAILURE                E_ERR_MAP_USE+116
#define E_ENQDEQ_FAILURE                E_ERR_MAP_USE+117
#define E_SETPRI_FAILURE                E_ERR_MAP_USE+118
#define E_ENABLE_CTRL_FAILURE           E_ERR_MAP_USE+119
#define E_QIO_FAILURE                   E_ERR_MAP_USE+120
#define E_TOO_MANY_PARAMETERS           E_ERR_MAP_USE+121
#define E_CRIT_SECT_VIOL                E_ERR_MAP_USE+122
#define E_ABNORMAL_PROCESS_TERM         E_ERR_MAP_USE+123
#define E_UNKNOWN_UIC                   E_ERR_MAP_USE+124
#define E_NO_BUFFER			E_ERR_MAP_USE+125
#define E_SIGDRV_MBX_FAILURE            E_ERR_MAP_USE+126
#define E_CANT_READ_MAILBOX             E_ERR_MAP_USE+127 
#endif

/* -------------------------------------------------------------------- */
/*	errnet = NE_NETSTATE						*/
/* -------------------------------------------------------------------- */

#define E_NO_NETWORK_ENVIRONMENT	970	/* Nodal Management	*/
#define E_NETWORK_IS_UP			E_NO_NETWORK_ENVIRONMENT+1 /* Nodal */
#define E_NETWORK_IS_DOWN		E_NO_NETWORK_ENVIRONMENT+2 /* Nodal */
#define E_NETWORK_GOING_DOWN		E_NO_NETWORK_ENVIRONMENT+3 /* Nodal */

/* -------------------------------------------------------------------- */
/*	errnet = NE_HARDWARE						*/
/* -------------------------------------------------------------------- */

#define E_HARDWARE_FAILURE		975
#define E_WRONG_CARD_TYPE		E_HARDWARE_FAILURE+1
#define E_NO_SUCH_DEVICE		E_HARDWARE_FAILURE+2

/* -------------------------------------------------------------------- */
/*	errnet = NE_TIMEOUT						*/
/* -------------------------------------------------------------------- */

#define E_REMOTE_TIMEOUT		980
#define E_SI_TIMEOUT			E_REMOTE_TIMEOUT+1


/* -------------------------------------------------------------------- */
/*	errnet = NE_PROTOVIOL						*/
/* -------------------------------------------------------------------- */

#define E_FRAME_TOO_LONG		985
#define E_CONTROL_FIELD_WRONG		E_FRAME_TOO_LONG+1


/* -------------------------------------------------------------------- */
/*	errnet = NE_NOSERV						*/
/* -------------------------------------------------------------------- */

#define E_SI_RSC_BUSY			990
#define E_RFA_NE_NOSERV			E_SI_RSC_BUSY+1
#define E_NO_PATH			E_SI_RSC_BUSY+2 /* Probe req */
#define E_UNSUPPORTED_SERVICE		E_SI_RSC_BUSY+3 /* Probe req */


/* -------------------------------------------------------------------- */
/*	errnet = NE_NOREMOTE						*/
/* -------------------------------------------------------------------- */

#define E_NODE_NAME_NOT_FOUND		1000
#define E_TCP_OPEN_TIMEOUT		E_NODE_NAME_NOT_FOUND+1
#define E_TCP_OPEN_FAILURE		E_NODE_NAME_NOT_FOUND+2
#define E_CONN_NONEXISTENT		E_NODE_NAME_NOT_FOUND+3 /* Path */


/* -------------------------------------------------------------------- */
/*	errnet = NE_NOLOGIN						*/
/* -------------------------------------------------------------------- */

#define E_RFA_NE_NOLOGIN		1010


/* -------------------------------------------------------------------- */
/*	errnet = NE_NOUSERS						*/
/* -------------------------------------------------------------------- */

#define E_CONNECTION_LIMIT_EXCEEDED	1020


/* -------------------------------------------------------------------- */
/*	errnet = NE_CONNLOST						*/
/* -------------------------------------------------------------------- */

#define E_TCP_REMOTE_ABORT		1030
#define E_TCP_SERVICE_FAILURE		E_TCP_REMOTE_ABORT+1


/* -------------------------------------------------------------------- */
/*	errnet = NE_DONTOWN						*/
/* -------------------------------------------------------------------- */

#define E_SI_NOT_OWNER			1040


/* -------------------------------------------------------------------- */
/*	errnet = NE_NONBLOCK						*/
/* -------------------------------------------------------------------- */

#define E_SI_WOULD_BLOCK		1050


/* -------------------------------------------------------------------- */
/*	errnet = NE_BUFFER						*/
/* -------------------------------------------------------------------- */

#define E_SI_BUFFER_TOOSMALL		1060
#define E_LAST_NETERR			1060	/* Last valid neterr	*/

/* Messages for justlogs which do not result in errors                  */
#define M_MESSAGE_CARRIER_QUEUE_FULL    2000    /* SI                   */

/* -------------------- end of NERROR.H ------------------------------- */
