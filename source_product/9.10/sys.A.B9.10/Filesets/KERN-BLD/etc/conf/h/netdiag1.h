/*
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/netdiag1.h,v $
 * $Revision: 1.6.83.4 $	    $Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 18:30:58 $
 *
 *
 */

#ifdef _KERNEL
#include "../h/time.h"
#else  /* _KERNEL */
#include <sys/time.h>
#endif /* _KERNEL */

/* This include file is common to both NS and OSI for the psuedo driver. 
 * NS/OSI diagnostics ioctl()'s.
 */

#ifndef NULL
#define NULL  0
#endif


/* defines for the ioctl commands  */
#define NETD_GET_STATUS		_IOWR('D',1, dm_mask_t)
#define NETD_SET_MASK		_IOW('D',2, dm_mask_t)
#define NETD_RESET_MASK		_IOW('D',3, rs_masks)
#define NETD_SET_BUFSIZE	_IOWR('D',4, int)
#define NETD_NEW_BUF		_IOW('D',5, int)
#define NETD_STOP		_IOW('D',6, int)
#define NETD_START		_IOW('D',7, int)


#define	 OSIDEBUG(x)	printf(x);

#define	 NETDIAG_Q_DF	200	/* default for que pointers */
#define	 NETDIAG_MAX_Q	1000	/* maximum for que pointers */

#define	 NETDIAG_EMPTY	0
#define	 NETDIAG_OK	1
#define	 NETDIAG_STOP	-1

#define NO_QUES		4	/* 0-3, Number of diag ques */

#define NET_TRACE	0	/* minor number for NET Tracing	    */
#define NET_LOG		1	/* minor number for NET Logging	    */


#define QUE_FULL	2
#define QUE_EMPTY	0
#define QUE_OK		1
#define MAX_T_LEN	2000	/* this is the max number for the -m option */

#ifndef TRUE
#define TRUE		1 
#endif

#ifndef FALSE
#define FALSE		0 
#endif 


/* These may be useful once we figure out the log class mess */
#define	 INFO_ALL	(INFORMATIVE|ERROR|WARNING|DISASTER|NETMSG)
#define	 WARN_ALL	(WARNING|ERROR|DISASTER|NETMSG)
#define	 ERROR_ALL	(ERROR|DISASTER|NETMSG)
#define	 DISASTER_ALL	(DISASTER|NETMSG)
#define	 NETMSG_ALL	(NETMSG)


#define DIAG_OPEN	3
#define DIAG_CLOSED	4
#define DIAG_IOCTL	O_WRONLY  /* this flag for a process not reading*/
#define DIAG_READ	O_RDONLY  /* this is a flag for a device doing reads */
#define DIAG_CHECK	(O_WRONLY | O_APPEND) /* this is to check if the t/l daemon exists */

#define NETDIAG_MAX_IOVECS 20


/* This defines what the message looks like when it is stored. It has a */
/* length and a caddr_t base. The reason I used this instead of an iovec*/
/* is because I cannot set up the length prior to base, it overwrites   */
/* itself.                                                              */
struct ktl_buf_t {
     int     len;
     caddr_t base;
};


struct macct   *ktl_acct;	/* mbuf account for ktl */

/*-------------------  ques for tracing/logging ------------------------ */
/* Tracing and logging each have thier own ques. They are indexed by the */
/* device minor number which is 0 for tracing and 1 for logging.         */
/*---------------------------------------------------------------------- */
typedef struct {
  int  que_full;                /* msg queue full flag (initially false) */
  int  que_empty;               /* msg queue empty flag (initially true) */
  int  que_head;                /* msg queue head                        */
  int  que_tail;                /* msg queue tail                        */
  int  que_state;               /* diag device opened                    */
  int  que_msgs_qued;           /* number of msgs qued ctr               */
  int  que_size;                /* the present que size in iovec's       */
  int  que_high_water_mark;     /* high water mark.                      */
  int  que_msgs_dropped;        /* number of msgs dropped                */
  int  que_kill;                /* determines if the buf should be dealloc'd */
  int  que_sleep;               /* determines if a proc is sleeping or not */
  caddr_t *que_start;           /* ptr to ptrs to caddr_t's in the queue  */
} que_elements_t;


/* definitions for default values */
#define	 DF_UID		-1
#define	 DF_CLASS	(ERROR|DISASTER|NETMSG) 
#define	 DF_KIND	0
#define	 DF_PATH	-1

/* definitions for ingnore or don't care values */
#define	 IGN_UID	0
#define	 IGN_CLASS	0
#define	 IGN_PATH	0

/* definition for bit positions for the NETD_RESET_MASK call */
#define	 RS_CLASS	0x1 
#define	 RS_KIND	0x2
#define	 RS_UID		0x4
#define	 RS_SUBSYS	0x20

#define	 RS_ALL_NET_TR	0x3e
#define	 RS_ALL_NET_LOG 0x21

#define NETDIAG_ON	1
#define NETDIAG_OFF	0

/*		Trace and log filters, data and packet		 */
/* the controller uses this struct to send to the p_driver with	 */
/* send_msg()							 */
typedef struct {
	short		subsys_id;
	int		kind;
	int		class;
	unsigned short	log_instance;	/* 6-SEP-91 TM */
	int		uid;
	int		dev_id;
	int		pid;		/* 22-Nov-91 TM */
	int		rpr;		/* <- obsolete? */
	int		cm_path_id;
	int		packetlen;
	int		tracedlen;
	struct timeval	time;
	unsigned int	dropped_packets;
	char		pkt;
} ktl_msg_hdr_type;

/*		Trace and log filters				 */
/* Used by the daemon for sending filters to p_driver		 */
typedef struct {
	short		subsys_id;
	int		kind;
	int		class;
	int		device_id;
	int		size;
	int		uid;
	int		t_len;
} dm_mask_t;


/*  Used by the controller, pseudo driver and daemon to reset masks */
typedef struct {
	short		subsys_id;
	int		rs_filters;
} rs_masks;


/*  Turns on or off logging for a given subsystem		 */
#define LOGG_SUBSYS_MAP(subsys_id, tag) netdiag_log_map[subsys_id] = (tag); 


/*  Turns on or off tracing for a given subsystem		 */
#define TRC_SUBSYS_MAP(subsys_id, tag) netdiag_tr_map[subsys_id] = (tag); 


/*
 * nsdiag definitions
 */

#define NSDIAG_DEFAULT_BUFFER	2048	/* Use if we can't malloc buffer */



#define NS_TR_ENMAXFILTER	60	/* maximum short words per ioctl    */


