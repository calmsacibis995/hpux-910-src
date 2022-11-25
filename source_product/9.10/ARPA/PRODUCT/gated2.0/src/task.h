/*
 * $Header: task.h,v 1.1.109.5 92/02/28 16:02:09 ash Exp $
 */

/*%Copyright%*/
/************************************************************************
*									*
*	GateD, Release 2						*
*									*
*	Copyright (c) 1990,1991,1992 by Cornell University		*
*	    All rights reserved.					*
*									*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY		*
*	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT		*
*	LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY		*
*	AND FITNESS FOR A PARTICULAR PURPOSE.				*
*									*
*	Royalty-free licenses to redistribute GateD Release		*
*	2 in whole or in part may be obtained by writing to:		*
*									*
*	    GateDaemon Project						*
*	    Information Technologies/Network Resources			*
*	    143 Caldwell Hall						*
*	    Cornell University						*
*	    Ithaca, NY 14853-2602					*
*									*
*	GateD is based on Kirton's EGP, UC Berkeley's routing		*
*	daemon	 (routed), and DCN's HELLO routing Protocol.		*
*	Development of Release 2 has been supported by the		*
*	National Science Foundation.					*
*									*
*	Please forward bug fixes, enhancements and questions to the	*
*	gated mailing list: gated-people@gated.cornell.edu.		*
*									*
*	Authors:							*
*									*
*		Jeffrey C Honig <jch@gated.cornell.edu>			*
*		Scott W Brim <swb@gated.cornell.edu>			*
*									*
*************************************************************************
*									*
*      Portions of this software may fall under the following		*
*      copyrights:							*
*									*
*	Copyright (c) 1988 Regents of the University of California.	*
*	All rights reserved.						*
*									*
*	Redistribution and use in source and binary forms are		*
*	permitted provided that the above copyright notice and		*
*	this paragraph are duplicated in all such forms and that	*
*	any documentation, advertising materials, and other		*
*	materials related to such distribution and use			*
*	acknowledge that the software was developed by the		*
*	University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote		*
*	products derived from this software without specific		*
*	prior written permission.  THIS SOFTWARE IS PROVIDED		*
*	``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,	*
*	INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF	*
*	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.		*
*									*
************************************************************************/


#define	TASK_TIMERS	5		/* Number of timers per task */

/* Task definitions */

struct _task {
    struct _task *task_forw;
    struct _task *task_back;
    const char *task_name;		/* Printable task name */
    flag_t task_flags;			/* Flags */
    int task_proto;			/* Protocol */
    int task_socket;			/* Socket (if applicable) */
    proto_t task_rtproto;		/* Routing table protocol (if applicable) */
    u_long task_rtrevision;		/* Revision level this task is up to */
    void (*task_recv) ();		/* Routine to receive packets (if applicable) */
    void (*task_accept) ();		/* Routine to process accepts (if applicable) */
    void (*task_write) ();		/* Routine to write when socket is ready */
    void (*task_connect) ();		/* Routine to process connect completions */
    void (*task_except) ();		/* Routine to handle exceptions */
    void (*task_terminate) ();		/* Routine to terminate task */
    void (*task_flash) ();		/* Routine to do flash updates */
    void (*task_ifchange) ();		/* Routine to call when an interface status changes */
    void (*task_cleanup) ();		/* Routine to cleanup before config file is re-read */
    void (*task_reinit) ();		/* Routine to cleanup after config file is re-read */
    void (*task_dump) ();		/* Routine to dump state */
    sockaddr_un task_addr;		/* Task dependent address */
    caddr_t task_data;			/* Task dependent pointer */

    int task_pid;			/* PID if this is a child */
    void (*task_process) ();		/* Routine to run after fork */
    void (*task_child) ();		/* Routine to run after child finishes */

    struct _timer *task_timer[TASK_TIMERS];	/* Pointer to timers this task owns */

};

#define	TASKF_ACCEPT		0x01	/* This socket is waiting for accepts, not reads */
#define	TASKF_CONNECT		0x02	/* This socket is waiting for connects, not writes */
#define	TASKF_IPHEADER		0x04	/* Received packets have IP header to be received */

#define	TASKOPTION_RECVBUF	0	/* Set receive buffer size */
#define	TASKOPTION_SENDBUF	1	/* Set send buffer size */
#define	TASKOPTION_LINGER	2	/* Set TCP linger on close */
#define	TASKOPTION_REUSEADDR	3	/* Enable/disable address reuse */
#define	TASKOPTION_BROADCAST	4	/* Enable/disable broadcast use */
#define	TASKOPTION_DONTROUTE	5	/* Enable/disable don't route */
#define	TASKOPTION_KEEPALIVE	6	/* Enable/disable keepalives */
#define	TASKOPTION_DEBUG	7	/* Enable/disable socket level debugging */
#define	TASKOPTION_NONBLOCKING	8	/* Enable/disable non-blocking I/O */
#define	TASKOPTION_USELOOPBACK	9

extern char *task_path_name;

#ifdef	USE_PROTOTYPES
extern task *task_alloc(const char *name);
extern int
task_create(task * tp,
	    int maxsize);
extern int task_fork(task * tp);
extern void task_delete(task * tp);
extern void task_init(void);
extern void task_main(void);
extern void task_flash(task * tp);
extern void task_ifchange(if_entry * ifp);
extern void
task_toall(task * tp,
	   void (*func) (),
	   int point_to_point,
	   int if_flag,
	   gw_entry * gw_list,
	   int flash_update);
extern int task_set_option(task * tp, int option, caddr_t value);
extern int task_get_socket(int domain, int type, int protocol);
extern void task_set_socket(task * tp, int socket);
extern void task_reset_socket(task * tp);
extern char *task_name(task * tp);
extern void task_dump(FILE * fd);
extern int task_receive_packet(task * tp, int *count);
extern int task_send_packet(task * tp, caddr_t msg, int len, flag_t flags, sockaddr_un * addr);

extern char *task_getwd(void);
extern int task_chdir(char *);
extern void task_getpaths(void);
#else				/* USE_PROTOTYPES */
extern task *task_alloc();
extern int task_create();
extern int task_fork();
extern void task_delete();
extern void task_init();
extern void task_main();
extern void task_flash();
extern void task_ifchange();
extern void task_toall();
extern int task_set_option();
extern int task_get_socket();
extern void task_set_socket();
extern void task_reset_socket();
extern char *task_name();
extern void task_dump();
extern int task_receive_packet();
extern int task_send_packet();

extern char *task_getwd();
extern int task_chdir();
extern void task_getpaths();
#endif				/* USE_PROTOTYPES */

#define	TASK_TABLE(tp)	{ task *_tp; \
			      for (tp = task_head.task_forw; _tp = tp->task_back, tp != &task_head; \
				   tp = (tp == _tp->task_forw) ? tp->task_forw : _tp->task_forw )
#define TASK_TABLEEND(tp)	}

/*  */
/* Timer definitions */

struct _timer {
    struct _timer *timer_forw;
    struct _timer *timer_back;
    const char *timer_name;		/* Printable name for this timer */
    flag_t timer_flags;			/* Flags */
    time_t timer_next_time;		/* Timer job wakeup time */
    time_t timer_last_time;		/* Last time job was called */
    time_t timer_interval;		/* Time to sleep between timer jobs */
    void (*timer_job) ();		/* Timer job (if applicable) */
    task *timer_task;			/* Task which owns this timer */
    int timer_index;			/* Index of this timer in task's table */
};

/* Timer flags */

#define	TIMERF_ABSOLUTE		0x01	/* Timer is relative to start time, not to last time */
#define	TIMERF_DELETE		0x02	/* Delete timer after it fires */

#ifdef	USE_PROTOTYPES
extern char *timer_name(timer * tip);	/* Return a string containing the name of a timer */
extern timer *
timer_create(task * tp,
	     int timer_index,
	     const char *name,
	     flag_t flags,
	     time_t interval,
	     void (*job) ());		/* Create a timer */
extern void timer_delete(timer * tip);	/* Delete a timer */
extern void timer_set(timer * tip, time_t interval);	/* Set a timer */
extern void timer_reset(timer * tip);	/* Reset a timer (clear it) */
extern void timer_interval(timer * tip, time_t interval);	/* Change a timer interval */

#else				/* USE_PROTOTYPES */
extern char *timer_name();		/* Return a string containing the name of a timer */
extern timer *timer_create();		/* Create a timer */
extern void timer_delete();		/* Delete a timer */
extern void timer_set();		/* Set a timer */
extern void timer_reset();		/* Reset a timer (clear it) */
extern void timer_interval();		/* Change a timer interval */

#endif				/* USE_PROTOTYPES */

#define	TIMER_ACTIVE(tip) { timer *_tip; \
				for (tip = timer_queue_active.timer_forw; _tip = tip->timer_back, tip != &timer_queue_active; \
				     tip = (tip == _tip->timer_forw) ? tip->timer_forw : _tip->timer_forw)
#define TIMER_ACTIVEEND(tip)	}

#define	TIMER_INACTIVE(tip) { timer *_tip; \
				for (tip = timer_queue_inactive.timer_forw; _tip = tip->timer_back, tip != &timer_queue_inactive; \
				     tip = (tip == _tip->timer_forw) ? tip->timer_forw : _tip->timer_forw)
#define TIMER_INACTIVEEND(tip) }
