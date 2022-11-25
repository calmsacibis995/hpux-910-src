/*
 *  $Header: task.c,v 1.1.109.6 92/03/09 12:52:30 ash Exp $
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


#include "include.h"
#include "bgp.h"
#include "egp.h"
#include "rip.h"
#include "hello.h"
#include "icmp.h"
#include "snmp.h"
#include "parse.h"
#include <signal.h>
#if	defined(_IBMR2)
#include <time.h>
#endif				/* defined(_IBMR2) */
#include <sys/time.h>
#ifdef	SYSV
#include <sys/sioctl.h>
#include <sys/stropts.h>
#else	/* SYSV */
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#endif	/* SYSV */
#include "task_sig.h"

char *task_path_name;			/* Directory where we were started */

static timer *timer_active;		/* Pointer to the active timer */
static timer timer_queue_active =
{&timer_queue_active, &timer_queue_active, "activeTimers"};	/* Doubly linked list of active timers */
static timer timer_queue_inactive =
{&timer_queue_inactive, &timer_queue_inactive, "inactiveTimers"};	/* Doubly linked list of inactive timers */

static task task_head =
{&task_head, &task_head, "taskHead"};	/* Head of doubly linked list of timers */
static fd_set task_select_readbits;
static fd_set task_select_writebits;
static fd_set task_select_exceptbits;
static int task_max_socket = 0;
static task **task_socket_tasks = (task **) 0;

#ifndef	SYSV
static int task_signal_mask = 0;	/* Signals that are blocked */
#endif	/* SYSV */
static int task_signals[] = {
    SIGTERM,
    SIGALRM,
    SIGUSR1,
    SIGINT,
    SIGHUP,
#ifndef	NO_FORK
    SIGCHLD,
#endif				/* NO_FORK */
    0};
#define	SIGNAL_LIST(ip)	{ int *ip; for (ip = task_signals; *ip; ip++)
#define	SIGNAL_LIST_END(ip) }

static bits task_flag_bits[] =
{
    {TASKF_ACCEPT, "Accept"},
    {TASKF_CONNECT, "Connect"},
    {TASKF_IPHEADER, "IPHeader"},
    {0}
};

static bits task_socket_options[] =
{
    {TASKOPTION_RECVBUF, "RecvBuffer"},
    {TASKOPTION_SENDBUF, "SendBuffer"},
    {TASKOPTION_LINGER, "Linger"},
    {TASKOPTION_REUSEADDR, "ReUseAddress"},
    {TASKOPTION_BROADCAST, "Broadcast"},
    {TASKOPTION_DONTROUTE, "DontRoute"},
    {TASKOPTION_KEEPALIVE, "KeepAlive"},
    {TASKOPTION_DEBUG, "Debug"},
    {TASKOPTION_NONBLOCKING, "NonBlocking"},
    {TASKOPTION_USELOOPBACK, "UseLoopback"},
    {0, NULL}
};

static bits task_msg_bits[] =
{
    {MSG_OOB, "MSG_OOB"},
    {MSG_PEEK, "MSG_PEEK"},
    {MSG_DONTROUTE, "MSG_DONTROUTE"},
#ifdef	MSG_EOR
    {MSG_EOR, "MSG_EOR"},
#endif				/* MSG_EOR */
#ifdef	MSG_TRUNC
    {MSG_TRUNC, "MSG_TRUNC"},
#endif				/* MSG_TRUNC */
#ifdef	MSG_CTRUNC
    {MSG_CTRUNC, "MSG_CTRUNC"},
#endif				/* MSG_CTRUNC */
#ifdef	MSG_WAITALL
    {MSG_WAITALL, "MSG_WAITALL"},
#endif				/* MSG_WAITALL */
    {0, NULL}
};

static bits timer_flag_bits[] =
{
    {TIMERF_ABSOLUTE, "Absolute"},
    {TIMERF_DELETE, "Delete"},
    {0}
};


/*
 *	Insert a timer on one of the queues.  Inactive timers are
 *	inserted at the beginning of their queue, Active timers are
 *	inserted in order of their expiration.
 */
static void
timer_insert(tip)
timer *tip;
{
    timer *tip1;

    if (tip->timer_interval) {
	TIMER_ACTIVE(tip1) {
	    if (tip->timer_next_time < tip1->timer_next_time) {
		break;
	    }
	} TIMER_ACTIVEEND(tip1);
    } else {
	tip1 = timer_queue_inactive.timer_forw;
    }

    insque((struct qelem *) tip, (struct qelem *) tip1->timer_back);
}


/*
 *	Return a pointer to a string containing the timer name
 */
char *
timer_name(tip)
timer *tip;
{
    static char name[MAXHOSTNAMELENGTH];

    if (tip->timer_task) {
	if (tip->timer_task->task_addr.in.sin_addr.s_addr) {
	    (void) sprintf(name, "%s_%s.%A",
			   tip->timer_task->task_name,
			   tip->timer_name,
			   &tip->timer_task->task_addr);
	} else {
	    (void) sprintf(name, "%s_%s",
			   tip->timer_task->task_name,
			   tip->timer_name);
	}
    } else {
	strcpy(name, tip->timer_name);
    }
    return (name);
}


/*
 *	Create a timer - returns pointer to timer structure
 */
timer *
timer_create(tp, indx, name, flags, interval, job)
task *tp;
int indx;
const char *name;
flag_t flags;
time_t interval;
void (*job) ();
{
    timer *tip;

    tip = (timer *) calloc(1, sizeof(timer));
    if (!tip) {
	trace(TR_ALL, LOG_ERR, "timer_create: calloc: %m");
	quit(errno);
    }
    tip->timer_name = name;
    tip->timer_task = tp;
    tip->timer_index = indx;
    tip->timer_flags = flags;
    tip->timer_interval = interval;
    tip->timer_job = job;

    /* Link timer to it's task if there is one */
    if (tip->timer_task) {
	tip->timer_task->task_timer[tip->timer_index] = tip;
    }
    /* If this timer is active, set the intervals */
    if (tip->timer_interval) {
	tip->timer_next_time = tip->timer_last_time = time_sec;
	if (tip->timer_flags & TIMERF_ABSOLUTE) {
	    tip->timer_next_time += tip->timer_interval;
	}
    }
    /* Insert in the correct queue */
    timer_insert(tip);

    /* If we have changed the wakeup time cause a wakeup now to recalculate */
    if ((tip == timer_queue_active.timer_forw) &&
	(timer_queue_active.timer_next_time > tip->timer_next_time)) {
	(void) kill(my_pid, SIGALRM);
    }
    trace(TR_TIMER, 0, "timer_create: created timer %s  flags <%s>  interval %#T at %T",
	  timer_name(tip),
	  trace_bits(timer_flag_bits, tip->timer_flags),
	  tip->timer_interval,
	  tip->timer_next_time);

    return (tip);
}


/*
 *	Delete a timer
 */
void
timer_delete(tip)
timer *tip;
{
    trace(TR_TIMER, 0, "timer_delete: %s", timer_name(tip));

    /* Unlink this timer from it's task if there is one */
    if (tip->timer_task) {
	tip->timer_task->task_timer[tip->timer_index] = (timer *) 0;
    }
    if (tip == timer_active) {
	tip->timer_flags |= TIMERF_DELETE;
    } else {
	remque((struct qelem *) tip);
	(void) free((caddr_t) tip);
    }

}


/*
 *	Reset a timer - move it to the inactive queue
 */
void
timer_reset(tip)
timer *tip;
{
    if (tip->timer_interval) {
	tip->timer_next_time = tip->timer_last_time = tip->timer_interval = (time_t) 0;

	remque((struct qelem *) tip);
	timer_insert(tip);

	trace(TR_TIMER, 0, "timer_reset: reset %s",
	      timer_name(tip));
    }
}


/*
 *	Set a timer to fire in interval seconds from now
 */
void
timer_set(tip, interval)
timer *tip;
time_t interval;
{
    tip->timer_interval = interval;
    if (!tip->timer_next_time) {
	tip->timer_last_time = tip->timer_next_time = time_sec;
    }
    tip->timer_next_time = time_sec + tip->timer_interval;

    /* Re-insert this timer in the active queue in expiration order */
    remque((struct qelem *) tip);
    timer_insert(tip);

    /* If we have changed the wakeup time cause a wakeup now to recalculate */
    if ((tip == timer_queue_active.timer_forw) &&
	(tip != timer_active) &&
	(timer_queue_active.timer_next_time > tip->timer_next_time)) {
	(void) kill(my_pid, SIGALRM);
    }
    trace(TR_TIMER, 0, "timer_set: timer %s interval set to %#T at %T",
	  timer_name(tip),
	  tip->timer_interval,
	  tip->timer_next_time);

}

/*
 *	Set a timer to fire in interval seconds from the last time it fired
 */
void
timer_interval(tip, interval)
timer *tip;
time_t interval;
{
    if (tip->timer_interval != interval) {
	tip->timer_interval = interval;
	if (!tip->timer_next_time) {
	    tip->timer_last_time = tip->timer_next_time = time_sec;
	}
	tip->timer_next_time = tip->timer_last_time + tip->timer_interval;

	/* Re-insert this timer in the active queue in expiration order */
	remque((struct qelem *) tip);
	timer_insert(tip);

	/* If we have changed the wakeup time cause a wakeup now to recalculate */
	if ((tip == timer_queue_active.timer_forw) &&
	    (tip != timer_active) &&
	    (timer_queue_active.timer_next_time > tip->timer_next_time)) {
	    (void) kill(my_pid, SIGALRM);
	}
	trace(TR_TIMER, 0, "timer_interval: timer %s interval set to %#T at %T",
	      timer_name(tip),
	      tip->timer_interval,
	      tip->timer_next_time);
    }
}


/*
 *	Dump the provided timer
 */
static void
timer_dump(fd, tip)
FILE *fd;
timer *tip;
{
    (void) fprintf(fd, "\t\t%s",
		   timer_name(tip));
    if (tip->timer_interval) {
	(void) fprintf(fd, "\tlast: %T\tnext: %T\tinterval: %#T",
		       tip->timer_last_time,
		       tip->timer_next_time,
		       tip->timer_interval);
    }
    if (tip->timer_flags) {
	(void) fprintf(fd, "\t<%s>",
		       trace_bits(timer_flag_bits, tip->timer_flags));
    }
    (void) fprintf(fd, "\n");
}


/*
 * timer control for periodic route-age and interface processing.
 * timer_dispatch() is called when the periodic interrupt timer expires.
 */
static void
timer_dispatch()
{
    time_t late = 0;
    timer *tip;
    struct itimerval value;

    /* Log a message if the system dispatched us late */
    if (timer_queue_active.timer_last_time) {
	trace(TR_TIMER, 0, "timer_dispatch: requested interval: %#T actual interval: %#T",
	      timer_queue_active.timer_interval,
	      time_sec - timer_queue_active.timer_last_time);

	late = time_sec - timer_queue_active.timer_last_time - timer_queue_active.timer_interval;
	if (late < 0) {
	    late = 0;
	}
	if (late) {
	    trace(TR_INT, 0, "timer_dispatch: interval timer interrupt %d seconds late",
		  late);
	}
    } else {
	trace(TR_TIMER, 0, "timer_dispatch: initializing");
    }

    /* Run the queues until all the expired timers have been serviced.  This allows for timers that expire while we are */
    /* working on other timers */
    do {
	TIMER_ACTIVE(tip) {
	    /* Timers are in time order so we don't have to scan the whole list */
	    if (time_sec < tip->timer_next_time) {
		break;
	    }
	    /* Log the timer */
	    trace(TR_TIMER, 0, "timer_dispatch: call %s, due at %T, last at %T, interval %#T",
		  timer_name(tip),
		  tip->timer_next_time,
		  tip->timer_last_time,
		  tip->timer_next_time - tip->timer_last_time);

	    /* Update time of last firing */
	    tip->timer_last_time = time_sec;

	    /* Call the timer routine */
	    timer_active = tip;
	    tip->timer_job(tip, tip->timer_interval);
	    timer_active = (timer *) 0;

	    tracef("timer_dispatch: returned from %s, ",
		   timer_name(tip));

	    /* If the timer is a one shot, delete it now */
	    if (tip->timer_flags & TIMERF_DELETE) {
		trace(TR_TIMER, 0, "deletion requested");
		timer_delete(tip);
		continue;
	    }
	    if (tip->timer_interval) {
		/* Reschedule again at the next interval after the current time */
		while (tip->timer_next_time <= time_sec) {
		    tip->timer_next_time += tip->timer_interval;
		}

		/* Remove and re-insert to maintain order in the queue */
		remque((struct qelem *) tip);
		timer_insert(tip);

		tracef("rescheduled ");
		if (tip->timer_interval > late) {
		    tracef("in %#T ",
			   tip->timer_interval - late);
		}
		trace(TR_TIMER, 0, "at %T",
		      tip->timer_next_time);
	    } else {
		trace(TR_TIMER, 0, "now inactive");
	    }

	} TIMER_ACTIVEEND(tip);

	/* Get the current time */
	getod();

    } while (timer_queue_active.timer_forw->timer_next_time <= time_sec);

    /* Calulate when we are supposed to wake up next and how long that is from now */
    timer_queue_active.timer_next_time = timer_queue_active.timer_forw->timer_next_time;
    timer_queue_active.timer_interval = timer_queue_active.timer_next_time - time_sec;
    timer_queue_active.timer_last_time = time_sec;

    trace(TR_TIMER, 0, "timer_dispatch: end, next job: %T delta: %#T",
	  timer_queue_active.timer_next_time,
	  timer_queue_active.timer_interval);

    /* Check for invalid intervals (should not happen) */
    if (timer_queue_active.timer_interval <= 0) {
	trace(TR_INT, 0, "timer_dispatch: timer interval (%#T) invalid, using 1 second",
	      timer_queue_active.timer_interval);
	timer_queue_active.timer_interval = 1;
    }
    /* Set the interval timer */
    value.it_interval.tv_sec = 0;	/* no auto timer reload */
    value.it_interval.tv_usec = 0;
    value.it_value.tv_sec = timer_queue_active.timer_interval;
    value.it_value.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &value, (struct itimerval *) 0)) {
	trace(TR_ALL, LOG_ERR, "timer_dispatch: setitimer: %m");
	quit(errno);
    }
}


/*  */
/*
 *	Return a pointer to a string containing the task name
 */
char *
task_name(tp)
task *tp;
{
    static char name[MAXHOSTNAMELENGTH];

    if (tp->task_addr.in.sin_addr.s_addr) {
	(void) sprintf(name, "%s.%A",
		       tp->task_name,
		       &tp->task_addr);
    } else {
	strcpy(name, tp->task_name);
    }

    if (tp->task_pid > 0) {
	(void) sprintf(&name[strlen(name)], "[%d]",
		       tp->task_pid);
    }
    return (name);
}


/*
 *	Receive packet and check for errors
 */
int
task_receive_packet(tp, count)
task *tp;
int *count;
{
    int iov;
    struct sockaddr_in *from;
    struct msghdr *msghdr;

    msghdr = &recv_msghdr;
    iov = tp->task_flags & TASKF_IPHEADER ? RECV_IOVEC_IP : RECV_IOVEC_DATA;
    msghdr->msg_iov = &recv_iovec[iov];
    msghdr->msg_iovlen = RECV_IOVEC_SIZE - iov;

    msghdr->msg_namelen = sizeof(recv_addr);	/* Set max size */
    memset(msghdr->msg_name, (char) 0, msghdr->msg_namelen);	/* Clean name */
    from = (struct sockaddr_in *) msghdr->msg_name;	/* Set pointer to address */

    *count = recvmsg(tp->task_socket, msghdr, 0);

    if (!*count) {
	return (-1);
    }
    if (*count < 0) {
	int do_log = LOG_ERR;

	switch (errno) {
	    case EINTR:
		break;
	    case ENETDOWN:
	    case ENETUNREACH:
	    case ENETRESET:
	    case ECONNABORTED:
	    case ECONNRESET:
	    case ENOBUFS:
	    case ETIMEDOUT:
	    case ECONNREFUSED:
	    case EHOSTDOWN:
	    case EHOSTUNREACH:
		do_log = 0;
	    default:
		trace(TR_ALL, do_log, "task_receive_packet: %s recvmsg: %m",
		      task_name(tp));
		break;
	}
	return (errno);
    }
    trace(TR_TASK, 0, "task_receive_packet: task %s from %#A socket %d length %d",
	  task_name(tp),
	  from,
	  tp->task_socket,
	  *count);

    if (msghdr->msg_namelen != socksize(from)) {
	trace(TR_INT, LOG_ERR, "task_receive_packet: %s fromlen %d invalid, expected %d",
	      task_name(tp),
	      msghdr->msg_namelen,
	      socksize(from));
	return (EINVAL);
    }
    return (0);
}


/*
 *	Send a packet
 */
int
task_send_packet(tp, msg, len, flags, addr)
task *tp;
caddr_t msg;
int len;
flag_t flags;
sockaddr_un *addr;
{
    int rc = 0;

    tracef("task_send_packet: task %s socket %d length %d",
	   task_name(tp),
	   tp->task_socket,
	   len);
    if (flags) {
	tracef(" flags %s(%X)",
	       trace_bits(task_msg_bits, flags),
	       flags);
    }
    if (addr) {
	tracef(" to %#A",
	       addr);
	rc = sendto(tp->task_socket, msg, len, (int) flags, addr, socksize(addr));
    } else {
	rc = send(tp->task_socket, msg, len, (int) flags);
    }

    if (rc < 0) {
	trace(TR_ALL, LOG_ERR, ": %m");
    } else if (rc != len) {
	trace(TR_ALL, LOG_ERR, ": %d bytes not accepted",
	      len - rc);
    } else {
	trace(TR_TASK, 0, NULL);
    }

    return rc;
}


/*
 *	Wait for incoming packets
 */
void
task_main()
{
#ifndef	SYSV
    int sigmask_save;
#endif	/* SYSV */
    int n, count, socket;
    fd_set read_bits, write_bits, except_bits;
    int forever = TRUE;
    task *tp;

    /* Allocate receive buffer know that we know it's maximum size */
    if (count = recv_iovec[RECV_IOVEC_DATA].iov_len) {
	if (!(recv_iovec[RECV_IOVEC_DATA].iov_base = (caddr_t) malloc((u_int) count))) {
	    trace(TR_ALL, LOG_ERR, "task_main: malloc: %m");
	    quit(errno);
	}
    }
    timer_dispatch();
    trace(TR_TASK, 0, NULL);

    while (forever) {
	trace(TR_TASK, 0, NULL);
	read_bits = task_select_readbits;
	write_bits = task_select_writebits;
	except_bits = task_select_exceptbits;
	n = select(task_max_socket + 1, &read_bits, &write_bits, &except_bits, (struct timeval *) 0);

	if (n < 0) {
	    if (errno == EINTR) {
		trace(TR_TASK, 0, "task_main: select: %m");
		continue;
	    } else {
		trace(TR_ALL, LOG_ERR, "task_main: select: %m");
		quit(errno);
	    }
	}
	getod();			/* current time */

#ifdef	SYSV
	SIGNAL_LIST(ip) {
	    sighold(*ip);
	} SIGNAL_LIST_END(ip) ;
#else	/* SYSV */
	sigmask_save = sigblock(task_signal_mask);
#endif	/* SYSV */

	for (socket = 0; socket < task_max_socket + 1; socket++) {
	    tp = task_socket_tasks[socket];

	    /* Check for ready for read on socket */
	    if (FD_ISSET(socket, &read_bits)) {
		if (tp) {
		    if (tp->task_flags & TASKF_ACCEPT) {
			if (tp->task_accept) {
			    trace(TR_TASK, 0, "task_main: accept ready for %s socket %d, protocol %d, port %d",
				  task_name(tp),
				  socket,
				  tp->task_proto,
				  ntohs(tp->task_addr.in.sin_port));
			    (void) tp->task_accept(tp);
			} else {
			    trace(TR_INT, 0, "task_main: no task for accept on socket %d", socket);
			}
		    } else {
			if (tp->task_recv) {
			    trace(TR_TASK, 0, "task_main: recv ready for %s socket %d, protocol %d, port %d",
				  task_name(tp),
				  socket,
				  tp->task_proto,
				  ntohs(tp->task_addr.in.sin_port));
			    (void) tp->task_recv(tp);
			} else {
			    trace(TR_INT, 0, "task_main: no task for read on socket %d", socket);
			}
		    }
		}
	    }
	    /* Check for ready for write on socket */
	    if (FD_ISSET(socket, &write_bits)) {
		if (tp) {
		    if (tp->task_flags & TASKF_CONNECT) {
			if (tp->task_connect) {
			    trace(TR_TASK, 0, "task_main: connect ready for %s socket %d, protocol %d, port %d",
				  task_name(tp),
				  socket,
				  tp->task_proto,
				  ntohs(tp->task_addr.in.sin_port));
			    (void) tp->task_connect(tp);
			} else {
			    trace(TR_TASK, 0, "task_main: no task for connect on socket %d, protocol %d, port %d",
				  socket,
				  tp->task_proto,
				  ntohs(tp->task_addr.in.sin_port));
			}
		    } else {
			if (tp->task_write) {
			    trace(TR_TASK, 0, "task_main: write ready for %s socket %d, protocol %d, port %d",
				  task_name(tp),
				  socket,
				  tp->task_proto,
				  ntohs(tp->task_addr.in.sin_port));
			    (void) tp->task_write(tp);
			} else {
			    trace(TR_TASK, 0, "task_main: no task for write on socket %d, protocol %d, port %d",
				  socket,
				  tp->task_proto,
				  ntohs(tp->task_addr.in.sin_port));
			}
		    }
		} else {
		    trace(TR_INT, 0, "task_main: no task for write/connect socket %d", socket);
		}
	    }
	    /* Check for exception on socket */
	    if (FD_ISSET(socket, &except_bits)) {
		if (tp) {
		    trace(TR_TASK, 0, "task_main: exception for %s socket %d, protocol %d, port %d",
			  task_name(tp),
			  socket,
			  tp->task_proto,
			  ntohs(tp->task_addr.in.sin_port));
		    (void) tp->task_except(tp);
		} else {
		    trace(TR_INT, 0, "task_main: no task for exception socket %d", socket);
		}
	    }
	}

#ifdef	SYSV
	SIGNAL_LIST(ip) {
	    sigrelse(*ip);
	} SIGNAL_LIST_END(ip) ;
#else	/* SYSV */
	(void) sigsetmask(sigmask_save);
#endif	/* SYSV */

    }
}


/*
 *	Call all tasks that have posted a cleanup routine
 */
static void
task_cleanup()
{
    task *tp;

    trace(TR_TASK, 0, NULL);
    TASK_TABLE(tp) {
	if (tp->task_cleanup) {
	    trace(TR_TASK, 0, "task_cleanup: Starting cleanup for task %s",
		  task_name(tp));
	    tp->task_cleanup(tp);
	    trace(TR_TASK, 0, "task_cleanup: Finished cleanup for task %s",
		  task_name(tp));
	}
    } TASK_TABLEEND(tp);
}


/*
 *	Call all task that have posted a reinit routine
 */
static void
task_reinit()
{
    task *tp;

    trace(TR_TASK, 0, NULL);
    TASK_TABLE(tp) {
	if (tp->task_reinit) {
	    trace(TR_TASK, 0, "task_reinit: Starting reinit for task %s",
		  task_name(tp));
	    tp->task_reinit(tp);
	    trace(TR_TASK, 0, "task_reinit: Finished reinit for task %s",
		  task_name(tp));
	}
    } TASK_TABLEEND(tp);
}


/*
 *	Call all tasks that have posted an ifchange routine
 */
void
task_ifchange(ifp)
if_entry *ifp;
{
    task *tp;

    trace(TR_TASK, 0, NULL);
    TASK_TABLE(tp) {
	if (tp->task_ifchange) {
	    trace(TR_TASK, 0, "task_ifchange: Starting ifchange for task %s",
		  task_name(tp));
	    tp->task_ifchange(tp, ifp);
	    trace(TR_TASK, 0, "task_ifchange: Finished ifchange for task %s",
		  task_name(tp));
	}
    } TASK_TABLEEND(tp);
}


static void	    
task_reconfigure()
{
    int i;
    u_int count = recv_iovec[RECV_IOVEC_DATA].iov_len;
#ifndef	SYSV
    int sigmask_save;
#endif	/* SYSV */

#ifdef	SYSV
    SIGNAL_LIST(ip) {
	sighold(*ip);
    } SIGNAL_LIST_END(ip) ;
#else	/* SYSV */
    sigmask_save = sigblock(task_signal_mask);
#endif	/* SYSV */

    trace(TR_ALL, 0, NULL);
    trace(TR_ALL, LOG_NOTICE, "task_receive_signal: re-initializing from %s",
	  EGPINITFILE);
    trace(TR_ALL, 0, NULL);
    i = adv_n_allocated;
    task_cleanup();
    if (adv_n_allocated) {
	trace(TR_ALL, LOG_ERR, "reinit: %d of %d adv_entry elements not freed",
	      adv_n_allocated, i);
    }
    /* Reset options */
    install = TRUE;

    if (parse_parse(EGPINITFILE)) {
	quit(0);
    }
    task_reinit();

    /* XXX - Do we need to reinit all of these? */
#if	defined(PROTO_ICMP) && !defined(RTM_ADD)
    icmp_init();
#endif				/* defined(PROTO_ICMP) && !defined(RTM_ADD) */
#ifdef	PROTO_EGP
    egp_init();
#endif				/* PROTO_EGP */
#ifdef	PROTO_BGP
    bgp_init();
#endif				/* PROTO_BGP */
#ifdef	PROTO_RIP
    rip_init();
#endif				/* PROTO_RIP */
#ifdef	PROTO_HELLO
    hello_init();
#endif				/* PROTO_HELLO */
#ifdef	AGENT_SNMP
    snmp_init();
#endif				/* AGENT_SNMP */

    /* Reallocate receive buffer if it's size has been increased */
    if (recv_iovec[RECV_IOVEC_DATA].iov_len > count) {
	count = recv_iovec[RECV_IOVEC_DATA].iov_len;

	free(recv_iovec[RECV_IOVEC_DATA]);
	
        if (!(recv_iovec[RECV_IOVEC_DATA].iov_base = (caddr_t) malloc(count))) {
            trace(TR_ALL, LOG_ERR, "task_receive_signal: malloc %m");
	    quit(errno);
        }
    }

    trace(TR_ALL, 0, NULL);
    trace(TR_ALL, LOG_NOTICE, "task_receive_signal: reinitializing done");
    trace(TR_ALL, 0, NULL);
#ifdef	SYSV
    SIGNAL_LIST(ip) {
	sigrelse(*ip);
    } SIGNAL_LIST_END(ip) ;
#else	/* SYSV */
    (void) sigsetmask(sigmask_save);
#endif	/* SYSV */
}


/*ARGSUSED*/
static SIGTYPE
task_receive_signal(sig, code, scp)
int sig, code;
struct sigcontext *scp;
{
    static int terminate = 0;
    task *tp;
    static const char *term_names[] =
    {
	"first",
	"second",
	"third"
    };

    getod();

    trace(TR_TASK, 0, NULL);
    trace(TR_TASK, 0, "task_receive_signal: received SIG%s code %d", trace_state(signal_names, sig - 1), code);
    switch (sig) {
	case SIGTERM:
	    trace(TR_INT, LOG_NOTICE, "task_receive_signal: %s terminate signal received", term_names[terminate]);

	    /* Subprocesses terminate immediately for now */
	    if (my_pid != my_mpid) {
		exit(0);
	    }
	    terminate++;
	    if (terminate > 2) {
		quit(0);
	    }
	    TASK_TABLE(tp) {
		if (tp->task_terminate) {
		    trace(TR_TASK, 0, "task_receive_signal: terminating task %s",
			  task_name(tp));
		    tp->task_terminate(tp);
		    trace(TR_TASK, 0, NULL);
		}
	    } TASK_TABLEEND(tp);
	    trace(TR_TASK, 0, "task_receive_signal: Exiting and waiting for completion");
	    break;

	case SIGALRM:
	    timer_dispatch();
	    break;

	case SIGHUP:
	    task_reconfigure();
	    break;

	case SIGINT:
	    trace_dump(FALSE);
	    break;

	case SIGUSR1:
	    if (trace_file == NULL) {
		trace(TR_ALL, LOG_ERR, "task_receive_signal: can not toggle tracing to console");
		break;
	    }
	    if (trace_flags) {
		trace_off();
	    } else {
		trace_on(trace_file, TRUE);
	    }
	    break;

#ifndef	NO_FORK
	case SIGCHLD:
	{
	    int pid;
	    WAIT_T statusp;
	
	retry:
	    pid = waitpid(-1, &statusp, WNOHANG|WUNTRACED);
	    if (pid) {
		if (pid < 0) {
		    switch (errno) {
		    case EINTR:
			/* Retry the syscall */
			goto retry;

		    case ECHILD:
			break;
		    default:
		        trace(TR_ALL, LOG_ERR, "task_receive_signal: waitpid() error: %m");
		    }
		} else {
		    TASK_TABLE(tp) {
			if (pid == tp->task_pid) {
			    break;
			}
		    } TASK_TABLEEND(tp);

		    if (tp) {
			int done = TRUE;
			    
			if (WIFSTOPPED(statusp)) {
			    /* Stopped by a signal */
			
			    trace(TR_ALL, LOG_ERR, "task_receive_signal: %s stopped by SIG%s",
				  task_name(tp),
				  trace_bits(signal_names, WSTOPSIG(statusp) - 1));
			    done = FALSE;
			} else if (WIFSIGNALED(statusp)) {
			    /* Terminated by a signal */
				
			    trace(TR_ALL, LOG_ERR, "task_receive_signal: %s terminated abnormally by SIG%s",
				  task_name(tp),
				  trace_bits(signal_names, WTERMSIG(statusp) - 1));
			} else if (WEXITSTATUS(statusp)) {
			    /* Non-zero exit status */
				
			    trace(TR_ALL, LOG_ERR, "task_receive_signal: %s terminated abnormally with retcode %d",
				  task_name(tp),
				  WEXITSTATUS(statusp));
			} else {
			    /* Normal termination */
				
			    trace(TR_TASK, 0, "task_receive_signal: %s terminated normally",
				  task_name(tp));
			    
			    if (tp->task_child) {
 				tp->task_child(tp);
			    }
			}
		    } else {
			trace(TR_ALL, LOG_ERR, "task_receive_signal: waitpid() returned status about unknown pid: %d",
			      pid);
		    }
		}
	    }
	}

	    break;
#endif	/* NO_FORK */

	default:
	    trace(TR_INT, LOG_ERR,
		  "task_receive_signal: Ignoring unknown signal SIG%s code %d",
		  trace_state(signal_names, sig - 1),
		  code);
    }
    trace(TR_TASK, 0, NULL);
    SIGRETURN;
}


#ifdef	notdef
/*
 *	close a task's socket and terminate
 */
void
task_close(tp)
task *tp;
{
    trace(TR_TASK, 0, "task_close: close socket %d task %s",
	  tp->task_socket,
	  task_name(tp));

    if (close(tp->task_socket)) {
	trace(TR_ALL, LOG_ERR, "task_close: close %s.%d: %m",
	      task_name(tp),
	      tp->task_socket);
    }
    task_delete(tp);
}

#endif				/* notdef */


/*
 *  Delete a task block and free allocated storage.  When the last task has been deleted, exit.
 */
void
task_delete(tp)
task *tp;
{
    int i;
    int socket;

    trace(TR_TASK, 0, "task_delete: deleting task %s",
	  task_name(tp));

    if (tp->task_socket != -1) {
	trace(TR_TASK, 0, "task_delete: closing socket %d for task %s",
	      tp->task_socket,
	      task_name(tp));
	socket = tp->task_socket;
	task_reset_socket(tp);
	if (close(socket)) {
	    trace(TR_ALL, LOG_ERR, "task_delete: close %s.%d: %m",
		  task_name(tp),
		  socket);
	}
    }
    /* Delete any timers associated with this task */
    for (i = 0; i < TASK_TIMERS; i++) {
	if (tp->task_timer[i]) {
	    timer_delete(tp->task_timer[i]);
	}
    }

    if (tp->task_forw) {
	remque((struct qelem *) tp);
	free((char *) tp);
    }
    if ((task_head.task_forw == task_head.task_back) & (task_head.task_forw == (task *) & task_head)) {
	trace(TR_TASK, 0, "task_delete: Removed last task, exiting");
	quit(0);
    }
}


void
task_flash(tp)
task *tp;
{
    task *tp1;

    trace(TR_TASK, 0, "task_flash: flash update request from %s revision is %d",
	  task_name(tp),
	  rt_revision);

    TASK_TABLE(tp1) {
	if (tp1->task_flash && (tp1->task_rtrevision < rt_revision)) {
	    trace(TR_TASK, 0, "task_flash: calling flash routine for %s revision %d",
		  task_name(tp1),
		  tp->task_rtrevision);
	    tp1->task_flash(tp1);
	    trace(TR_TASK, 0, "task_flash: return from routine for %s revision %d",
		  task_name(tp1),
		  tp->task_rtrevision);
	}
    } TASK_TABLEEND(tp1);
}


/*
 * Apply the function "f" to all non-passive
 * interfaces.  If the interface supports the
 * use of broadcasting use it, otherwise address
 * the output to the known router.
 */
void
task_toall(tp, func, point_to_point, if_flag, gw_list, flash_update)
task *tp;
void (*func) ();
int point_to_point;
int if_flag;
gw_entry *gw_list;
int flash_update;
{
    register if_entry *ifp;
    register gw_entry *gwp;
    register struct sockaddr *dst;

    trace(TR_TASK, 0, "task_toall: task %s revision %d rt_revision %d",
	  task_name(tp),
	  tp->task_rtrevision,
	  rt_revision);

    if (!point_to_point) {
	IF_LIST(ifp) {
	    if ((ifp->int_state & (if_flag | IFS_UP | IFS_LOOPBACK)) != IFS_UP) {
		continue;
	    }
	    if (ifp->int_state & IFS_BROADCAST) {
		dst = &ifp->int_broadaddr.a;
	    } else if (ifp->int_state & IFS_POINTOPOINT) {
		dst = &ifp->int_dstaddr.a;
	    } else if (ifp->int_state & IFS_NOAGE) {
		continue;
	    } else {
		dst = &ifp->int_addr.a;
	    }
	    (*func) (tp, dst, &ifp->int_addr.a, ifp, NULL, TRUE, flash_update);
	} IF_LISTEND(ifp) ;
    }
    GW_LIST(gw_list, gwp) {
	if (gwp->gw_flags & GWF_SOURCE) {
	    if ((ifp = if_withdst(&gwp->gw_addr)) <= (if_entry *) 0) {
		trace(TR_ALL, LOG_ERR, "task_toall: Source gateway %A not on same net",
		      &gwp->gw_addr);
		continue;
	    }
	    if ((ifp->int_state & (if_flag | IFS_UP)) != IFS_UP) {
		/* XXX -Should no rip out apply to source[rip|hello]gateways */
		continue;
	    }
	    (*func) (tp, &gwp->gw_addr, &ifp->int_addr.a, ifp, gwp, TRUE, flash_update);
	}
    } GW_LISTEND;

    tp->task_rtrevision = rt_revision;
}


/*
 *	Allocate a task block with the specified name
 */
task *
task_alloc(name)
const char *name;
{
    task *tp;

    if (!(tp = (task *) calloc(1, sizeof(task)))) {
	trace(TR_ALL, LOG_ERR, "task_alloc: calloc: %m");
	quit(errno);
    }
    tp->task_name = name;
    tp->task_terminate = task_delete;
    tp->task_socket = -1;

    trace(TR_TASK, 0, "task_alloc: allocated task block for %s", tp->task_name);
    return (tp);
}


/*
 *	Build a task block and add to the linked list
 */
int
task_create(tp, maxsize)
task *tp;
int maxsize;
{

    if (tp->task_flash) {
	tp->task_rtrevision = rt_revision;
    }
    if (tp->task_socket != -1) {
	task_set_socket(tp, tp->task_socket);
    }
    /*
     *	Set maximum receive buffer size
     */
    if (maxsize > recv_iovec[RECV_IOVEC_DATA].iov_len) {
	recv_iovec[RECV_IOVEC_DATA].iov_len = maxsize;
	trace(TR_TASK, 0, "task_create: receive buffer size set to %d", maxsize);
    }
    insque((struct qelem *) tp, (struct qelem *) & task_head);	/* Insert at the top of the task queue */

    tracef("task_create: %s",
	   task_name(tp));

    if (tp->task_proto) {
	tracef("  proto %d",
	       tp->task_proto);
    }
    if (tp->task_addr.in.sin_port) {
	tracef("  port %d",
	       ntohs(tp->task_addr.in.sin_port));
    }
    if (tp->task_socket != -1) {
	tracef("  socket %d",
	       tp->task_socket);
    }
    if (tp->task_rtproto) {
	tracef("  rt_proto <%s>",
	       trace_bits(rt_proto_bits, tp->task_rtproto));
    }
    trace(TR_TASK, 0, NULL);

    return (1);
}


/*
 * Terminate a subprocess
 */
static void
task_kill(tp)
task *tp;
{
    kill(tp->task_pid, SIGTERM);
}


/*
 * Spawn a process and create a task for it.
 */
int
task_fork(tp)
task *tp;
{
    int rc = 0;

    if (!(tp->task_pid = fork())) {
	tp->task_pid = my_pid = getpid();

	trace(TR_TASK, 0, "task_fork: %s forked",
	      task_name(tp));

	if (tp->task_process) {
	    tp->task_process(tp);
	}
	trace(TR_TASK, 0, "task_fork: %s exiting",
	      task_name(tp));

	exit(0);
    }
    if (tp->task_pid < 0) {
	trace(TR_ALL, LOG_ERR, "task_fork: could not fork %s: %m",
	      task_name(tp));
	task_delete(tp);
    } else {
	tp->task_terminate = task_kill;
	rc = task_create(tp, 0);
    }

    return rc;
}


/*  */

int
task_ioctl(fd, cmd, data, len)
int fd;
int cmd;
caddr_t data;
int len;
{
#if	!defined(SYSV)
    return ioctl(fd, cmd, data);
#else	/* !defined(SYSV) */
    struct strioctl si;
 
    si.ic_cmd = cmd;
    si.ic_timout = 0;
    si.ic_len = len;
    si.ic_dp = dp;
    
    return ioctl(fd, I_STR, &si);
#endif	/* !defined(SYSV) */
}

/**/

void
task_set_socket(tp, socket)
task *tp;
int socket;
{

    tp->task_socket = socket;

    trace(TR_TASK, 0, "task_set_socket: task %s socket %d",
	  task_name(tp),
	  tp->task_socket);

    /* Allocate space for socket to task index */
    if (!task_socket_tasks) {
	task_socket_tasks = (task **) calloc(1, sizeof(task *) * getdtablesize());
	if (!task_socket_tasks) {
	    trace(TR_ALL, LOG_ERR, "task_set_socket: calloc: %m");
	    quit(errno);
	}
    }
    if (tp->task_recv || tp->task_accept) {
	FD_SET(tp->task_socket, &task_select_readbits);
    }
    if (tp->task_write || tp->task_connect) {
	FD_SET(tp->task_socket, &task_select_writebits);
    }
    if (tp->task_except) {
	FD_SET(tp->task_socket, &task_select_exceptbits);
    }
    if (task_socket_tasks[tp->task_socket] && (task_socket_tasks[tp->task_socket] != tp)) {
	tracef("task_set_socket: attempt to assign socket %d to task %s ",
	       tp->task_socket,
	       task_name(tp));
	trace(TR_ALL, LOG_ERR, "socket already assigned to task %s",
	      task_name(task_socket_tasks[tp->task_socket]));
	quit(EBADF);
    }
    task_socket_tasks[tp->task_socket] = tp;
}


void
task_reset_socket(tp)
task *tp;
{

    trace(TR_TASK, 0, "task_reset_socket: task %s socket %d",
	  task_name(tp),
	  tp->task_socket);

    FD_CLR(tp->task_socket, &task_select_readbits);
    FD_CLR(tp->task_socket, &task_select_writebits);
    FD_CLR(tp->task_socket, &task_select_exceptbits);

    /* Delete from socket to task table if no routines present */
    if (!task_socket_tasks[tp->task_socket]) {
	trace(TR_ALL, LOG_ERR, "task_reset_socket: attempt to release socket %d by task %s - socket not assigned",
	      tp->task_socket,
	      task_name(tp));
	quit(EBADF);
    }
    task_socket_tasks[tp->task_socket] = (task *) 0;
    tp->task_socket = -1;
    tp->task_flags &= ~(TASKF_CONNECT | TASKF_ACCEPT);
    tp->task_recv = (void (*) ()) 0;
    tp->task_accept = (void (*) ()) 0;
    tp->task_write = (void (*) ()) 0;
    tp->task_connect = (void (*) ()) 0;
    tp->task_except = (void (*) ()) 0;
}


/*
 *	task_socket_options - Sets socket options.  Isolates protocols from system layer.
 */
int
task_set_option(tp, option, value)
task *tp;
int option;
caddr_t value;
{
    int opt;
    int rc = 0;
    int value_int;
    int len = sizeof(value_int);
    caddr_t ptr = (caddr_t) & value_int;

#ifdef	LINGER_PARAM
    struct linger linger;

#endif				/* LINGER_PARAM */
    int level = SOL_SOCKET;

    tracef("task_set_option: task %s socket %d option %s(%d)",
	   task_name(tp),
	   tp->task_socket,
	   trace_state(task_socket_options, option),
	   option);

    switch (option) {
	case TASKOPTION_RECVBUF:
#ifdef	SO_RCVBUF
	    opt = SO_RCVBUF;
	    goto int_value;
#else				/* SO_RCVBUF */
	    break;
#endif				/* SO_RCVBUF */

	case TASKOPTION_SENDBUF:
#ifdef	SO_SNDBUF
	    opt = SO_SNDBUF;
	    goto int_value;
#else				/* SO_SNDBUF */
	    break;
#endif				/* SO_SNDBUF */

	case TASKOPTION_LINGER:
	    opt = SO_LINGER;
#ifdef	LINGER_PARAM
	    linger.l_linger = (int) value;
	    linger.l_onoff = linger.l_linger ? TRUE : FALSE;
	    ptr = (caddr_t) & linger;
	    len = sizeof(struct linger);
	    tracef(" value { %d, %d }",
		   linger.l_linger,
		   linger.l_onoff);
#else				/* LINGER_PARAM */
	    ptr = 0;
	    len = 0;
#endif				/* LINGER_PARAM */
	    goto setsocketopt;

	case TASKOPTION_REUSEADDR:
	    opt = SO_REUSEADDR;
	    goto int_value;

	case TASKOPTION_BROADCAST:
#ifdef	SO_BROADCAST
	    opt = SO_BROADCAST;
	    goto int_value;
#else				/* SO_BROADCAST */
	    break;
#endif				/* SO_BROADCAST */

	case TASKOPTION_DONTROUTE:
	    opt = SO_DONTROUTE;
	    goto int_value;

	case TASKOPTION_KEEPALIVE:
	    opt = SO_KEEPALIVE;
	    goto int_value;

	case TASKOPTION_DEBUG:
	    opt = SO_DEBUG;
	    goto int_value;

	case TASKOPTION_USELOOPBACK:
	    opt = SO_USELOOPBACK;
	    goto int_value;

	int_value:
	    value_int = (int) value;
	    tracef(" value %d",
		   value);
	    /* goto setsocketopt; */

	setsocketopt:
	    if (!test_flag) {
		rc = setsockopt(tp->task_socket, level, opt, ptr, len);
	    }
	    break;

	case TASKOPTION_NONBLOCKING:
	    value_int = (int) value;
	    tracef(" value %d",
		   value);
	    if (!test_flag) {
#ifdef	SYSV
		rc = fcntl(tp->task_socket, F_SETFL, O_NDELAY);
#else	/* SYSV */
		rc = task_ioctl(tp->task_socket, FIONBIO, (caddr_t) & value_int, sizeof (value_int));
#endif	/* SYSV */
	    }
	    break;

	default:
	    rc = -1;
	    errno = EINVAL;
    }

    if (rc < 0) {
	trace(TR_ALL, LOG_ERR, ": %m");
    } else {
	trace(TR_TASK, 0, NULL);
    }

    return (rc);
}

/*
 * task_init()   set up for receiving signals and other initialization
 */
void
task_init()
{
#ifndef	SYSV
    struct sigvec vec, ovec;

    /* Set up signals to block */
    SIGNAL_LIST(ip) {
	task_signal_mask |= sigmask(*ip);
    } SIGNAL_LIST_END(ip) ;

    /* Setup signal processing */
    memset((char *) &vec, (char) 0, sizeof(struct sigvec));
    vec.sv_mask = task_signal_mask;
    vec.sv_handler = task_receive_signal;
#endif	/* SYSV */

    SIGNAL_LIST(ip) {
#ifdef	SYSV
	sigset (*ip, task_receive_signal);
#else	/* SYSV */
	if (sigvec(*ip, &vec, &ovec)) {
	    trace(TR_ALL, LOG_ERR, "task_init: sigvec SIG%s: %m", trace_state(signal_names, *ip));
	    quit(errno);
	}
#endif	/* SYSV */
    } SIGNAL_LIST_END(ip) ;
}


/*
 *	Dump task information to dump file
 */
void
task_dump(fd)
FILE *fd;
{
    int i;
    int first;
    int socket;
    task *tp;
    timer *tip;

    /* Print out task blocks */
    (void) fprintf(fd, "Task and Timers:\n\n");
    TASK_TABLE(tp) {
	(void) fprintf(fd, "\t%s",
		       task_name(tp));

	if (tp->task_proto) {
	    (void) fprintf(fd, "\tProto %3d",
			   tp->task_proto);
	}
	if (ntohs(tp->task_addr.in.sin_port)) {
	    (void) fprintf(fd, "\tPort %5u",
			   ntohs(tp->task_addr.in.sin_port));
	}
	if (tp->task_socket != -1) {
	    (void) fprintf(fd, "\tSocket %2d",
			   tp->task_socket);
	}
	if (tp->task_rtproto) {
	    (void) fprintf(fd, "\tRtProto %s",
			   trace_bits(rt_proto_bits, tp->task_rtproto));
	}
	if (tp->task_flags) {
	    (void) fprintf(fd, "\t<%s>",
			   trace_bits(task_flag_bits, tp->task_flags));
	}
	(void) fprintf(fd, "\n");
	first = TRUE;
	for (i = 0; i < TASK_TIMERS; i++) {
	    if (tp->task_timer[i]) {
		if (first) {
		    (void) fprintf(fd, "\n");
		    first = FALSE;
		}
		timer_dump(fd, tp->task_timer[i]);
	    }
	}
	(void) fprintf(fd, "\n");
    } TASK_TABLEEND(tp);
    (void) fprintf(fd, "\n");

    /* Print timers that are not associated with tasks */
    first = TRUE;
    TIMER_ACTIVE(tip) {
	if (!tip->timer_task) {
	    if (first) {
		(void) fprintf(fd, "\tTimers without tasks:\n\n");
		first = FALSE;
	    }
	    timer_dump(fd, tip);
	}
    } TIMER_ACTIVEEND(tip);

    TIMER_INACTIVE(tip) {
	if (!tip->timer_task) {
	    if (first) {
		(void) fprintf(fd, "\tTimers without tasks:\n\n");
		first = FALSE;
	    }
	    timer_dump(fd, tip);
	}
    } TIMER_INACTIVEEND(tip);

    if (!first) {
	(void) fprintf(fd, "\n");
    }
    /* Print mapping of sockets to tasks */
    (void) fprintf(fd, "Task to socket mapping:\n\n");
    for (socket = 0; socket <= task_max_socket; socket++) {
	if (tp = task_socket_tasks[socket]) {
	    (void) fprintf(fd, "\tsocket: %d\ttask: %s\n",
			   socket,
			   task_name(tp));
	}
    }
    (void) fprintf(fd, "\n");

    /* Do task-specific dumps */
    TASK_TABLE(tp) {
	if (tp->task_dump) {
	    tp->task_dump(fd);
	}
    } TASK_TABLEEND(tp);
}

static bits domains[AF_MAX + 1] =
{
    {AF_UNSPEC, "UNSPEC"},
    {AF_UNIX, "UNIX"},
    {AF_INET, "INET"},
    {AF_IMPLINK, "IMPLINK"},
    {AF_PUP, "PUP"},
    {AF_CHAOS, "CHAOS"},
    {AF_NS, "NS"},
#ifdef	AF_ISO
    {AF_ISO, "ISO"},
#else				/* AF_ISO */
    {AF_NBS, "NBS"},
#endif				/* AF_ISO */
    {AF_ECMA, "ECMA"},
    {AF_DATAKIT, "DATAKIT"},
    {AF_CCITT, "CCITT"},
    {AF_SNA, "SNA"},
#ifdef	AF_DECnet
    {AF_DECnet, "DECnet"},
    {AF_DLI, "DLI"},
    {AF_LAT, "LAT"},
    {AF_HYLINK, "HYLINK"},
    {AF_APPLETALK, "APPLETALK"},
#endif				/* AF_DECnet */
#ifdef	AF_ROUTE
    {AF_ROUTE, "Route"},
#endif				/* AF_ROUTE */
#ifdef	AF_NIT
    {AF_NIT, "NIT"},
#endif				/* AF_NIT */
};

static bits types[5] =
{
    {SOCK_STREAM, "STREAM"},
    {SOCK_DGRAM, "DGRAM"},
    {SOCK_RAW, "RAW"},
    {SOCK_RDM, "RDM"},
    {SOCK_SEQPACKET, "SEQPACKET"},
};

/*
 * task_get_socket gets a socket, retries later if no buffers at present
 */

int
task_get_socket(domain, type, protocol)
int domain, type, protocol;
{
    int retry, get_socket, error;

    if (test_flag) {
	if (task_max_socket) {
	    get_socket = ++task_max_socket;
	} else {
	    /* Skip first few that may be used for logging */
	    get_socket = (task_max_socket = 2);
	}
    } else {
	retry = 2;			/* if no buffers a retry might work */
	while ((get_socket = socket(domain, type, protocol)) < 0 && retry--) {
	    error = errno;
	    trace(TR_ALL, LOG_ERR, "task_get_socket: socket: %m");
	    if (error == ENOBUFS) {
		sleep(5);
	    } else {
		break;
	    }
	}
    }

    trace(TR_TASK, 0, "task_get_socket: domain AF_%s  type SOCK_%s  protocol %d  socket %d",
	  trace_state(domains, domain),
	  trace_state(types, type - 1),
	  protocol,
	  get_socket);

    if (get_socket >= getdtablesize()) {
	trace(TR_ALL, LOG_ERR, "task_get_socket: Too many sockets for select mask");
	quit(EMFILE);
    }
    if (get_socket > task_max_socket) {
	task_max_socket = get_socket;
    }
    return (get_socket);
}


/*
 *	Path names
 */
#ifndef	vax11c
char *
task_getwd()
{
    static char path_name[MAXPATHLEN];
    
    if (!getwd(path_name)) {
	trace(TR_ALL, LOG_ERR, "task_getwd: getwd: %s",
	      path_name);
	quit(ENOENT);
    }

    return path_name;
}


int
task_chdir(path_name)
const char *path_name;
{
    int rc;
    
    if (rc = chdir(path_name)) {
	trace(TR_ALL, LOG_ERR, "task_cwd: chdir: %m");
    }

    return rc;
}


void
task_getpaths()
{
    char *path = task_getwd();
    int len = strlen(path);
    
    /* Remember directory we were started from */
    task_path_name = (caddr_t) malloc(len);
    strcpy(task_path_name, path);
}
#endif	/* vax11c */
