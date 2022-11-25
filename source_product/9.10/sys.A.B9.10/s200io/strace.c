/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/strace.c,v $
 * $Revision: 1.2.84.17 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/11/29 14:44:54 $
 */
 
#ifdef STRACE


/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988  Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304

*/


/*
 *	pseudo driver for tracing processes - DKM 9/93
 *
 *	The basic idea here is simple: when a user process wants to 
 *	trace another one, it opens up /dev/trace and does an ioctl(2)
 *	to tell us which PID to trace.  
 *
 *	We then set a flag in the proc table entry, and every time that
 *	process makes a system call (and every time it returns from that
 *	call) strace_log() is called from trap.c.  strace_log() fills a
 *	buffer that is part of a "trace control block" for the traced
 *	process. 
 *
 *	The tracing process calls into strace_read() whenever it wants
 *	data, and we give it whatever is there (or put it to sleep if the
 * 	buffer is empty). 
 *
 *	To complicate/uglify the situation:
 *	
 *	    string arguments to system calls - the code is ugly, and
 *		needs to be fixed!  Part of the problem is the way 
 *		string arguments are handed back to the user process:
 *
 *			tr#1  |  tr#2  |  tr#3x  |  string from 3  |  tr#4
 *
 *		in other words, if a system call (in this case the 3rd one
 *		in the buffer) has a string argument, that arg gets 
 *		stuffed into the next "trace record".  There is much 
 *		singing and dancing to keep from stomping on the argument
 *		when the next call comes in.
 */

/*
 *  To Do
 *
 *	should our hook in trap.c be before or after setjmp?
 *  copy ends of strings (interesting part of a pathname is end, not beg)?  
 *  let user not get incomplete calls? 
 *  use p_cttyfp to record syscall # at trap time; use cttybp to get rid of
 *       linked list (strace_head)
 */



#include "../h/param.h"
#include "../h/errno.h"
#include "../h/uio.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/user.h"
#include "../h/vmmac.h"
#include "../h/malloc.h"
#ifdef __hp9000s300
#include "../s200io/strace.h"
#else
#include "../wsio/strace.h"
#endif
#ifdef FILE_TRACE
#include "../h/vnode.h"
#include "../h/pathname.h"
#include "../dux/lookupops.h"
#endif


extern struct timeval time;
struct tcb *get_new_tcb();
struct tcb *strace_head = NULL;		/*  all trace buffers on this list  */
int log_called = 0;
int strace_debug = 0;



#ifdef FILE_TRACE
/*
 *	interface for lookuppn()
 */
strace_file(pid, vp, path)
int pid;
struct vnode *vp;
char *path;
{
	struct tcb *tp;
	struct trace_record *trp, *next;
	int count;
			

	/*
	 *	find the trace control block & do sanity checking
	 */
	log_called++;			
	for (tp = strace_head; tp; tp = tp->td_next) 
		if ((tp->td_flag & TD_VNODE) && (vp == tp->td_vp))
			break;

	/*
	 *	if we didn't find it, something's wrong
	 *	ignore for now   XXX
	 */	
	if (!tp)
		return(0);
	
	if (tp->td_trp == NULL) {
		msg_printf("strace: no memory in strace_log() for path %s!\n", path);
		return(0);
	}

	/*
	 *	make sure there's room to log this file access
	 */
	if ((tp->td_size - tp->td_count) <= ONE_CALL_WORTH) {
		if (strace_debug)
			msg_printf("trace buffer for PID %d full (file)\n", tp->td_pid);
		return(0);
	}
	
	trp = tp->td_trp;
	if (strace_debug & 0x04) 
		printf("strace_file: PID is %d, path is %s, count is %d\n", 
			pid, path, tp->td_count);

	log_call(tp, trp, pid);
	count = trp->t_ns + 1;
	while (count--)
		tp->td_trp++;
	init_record(tp->td_trp);
	selwakeup((struct proc *) 0, 1);
}

#endif  /*  FILE_TRACE  */



/*
 *	interface for psig()
 */
strace_sig(pid, sig)
int pid;
int sig;
{
	struct tcb *tp;
	struct trace_record *trp, *next;
		

	if (strace_debug & 0x04) 
		printf("strace_sig: PID is %d, sig is %d, my PID is %d\n", 
			pid, sig, u.u_procp->p_pid);
	/*
	 *	find the trace control block & do sanity checking
	 */
	log_called++;			
	for (tp = strace_head; tp; tp = tp->td_next) 
		if (tp->td_pid == pid)
			break;

	/*
	 *	if we didn't find it, this must be a newborn 
	 *	child of a process we are tracing, but how did
	 *	it get a signal so quick?  ignore for now   XXX
	 */	
	if (!tp)
		return(0);
	
	if (tp->td_trp == NULL) {
		msg_printf("strace: no memory in strace_log() for PID %d!\n", pid);
		return(0);
	}

	/*
	 *	make sure there's room to log this signal
	 */
	if ((tp->td_size - tp->td_count) <= ONE_CALL_WORTH) {
		if (strace_debug)
			msg_printf("trace buffer for PID %d full (signal)\n", tp->td_pid);
		return(0);
	}
	
	trp = tp->td_trp;

	if (trp->t_flags & T_CALL) {	/*  syscall in progress  */
		trp->t_ns++;
		trp += trp->t_ns;
		trp->t_flags = T_SIGNAL;
	} else {
		trp->t_flags = T_SIGNAL;
		trp->t_spare2 = tp->td_seqno;
		tp->td_trp++;
		init_record(tp->td_trp);
	}	
	trp->t_syscall = sig;
	trp->t_entry_time = time;
	trp->t_pid = pid;
	if (ON_ISTACK)
		trp->t_params[0] = 0;
	else
		trp->t_params[0] = u.u_procp->p_pid;

	tp->td_count += TR_LEN;		
	selwakeup((struct proc *) 0, 1);
		
}



/*
 *	interface for syscall() (trap.c)
 */
strace_log(pid, cmd)
int pid;
int cmd;
{
	struct tcb *tp;
	struct trace_record *trp;
	int temp_uid;
		

	/*
	 *	find the trace control block & do sanity checking
	 */
	log_called++;			
	for (tp = strace_head; tp; tp = tp->td_next) 
		if (tp->td_pid == pid)
			break;

	/*
	 *	if we didn't find it, this must be a newborn 
	 *	child of a process we are tracing; start tracing
	 *	it if parent's TD_FOLLOW flag is set, otherwise 
	 *	clear the tracing bit (so a child of a tracee will
	 *	come through this path once no matter what, but will
	 *	only be back if we are really supposed to trace it)
	 */	
	if (!tp) {
		for (tp = strace_head; tp; tp = tp->td_next) 
			if (tp->td_pid == u.u_procp->p_ppid)
				break;

		if (tp && (tp->td_flag & TD_FOLLOW)) {
			/*
			 *	We are a child of a process that is being
			 *	traced, and we are supposed to be traced too.
			 *	Allocate/init a TCB, and let the tracer know
			 *	via a "signal" record.  The tracer will do a
			 *	STRACE_NEW_CHILD ioctl when he sees this.
			 */
			temp_uid = tp->td_tr_uid;
			if ((tp = get_new_tcb()) == NULL) {
				msg_printf("t_log: no memory to trace PID %d\n", pid);
				u.u_procp->p_flag &= ~SSCT;
				return(0);
			}
			/*  
			 *  initialize basic fields; note that we don't allow
			 *  tracing setuid programs, so we must set the uid of
			 *  the tracer immediately (to the uid of the tracer
			 *  of the parent, for now anyway...)
			 */
			tp->td_tr_uid = temp_uid;  
			tp->td_pid = pid;
			tp->td_proc = u.u_procp;
			tp->td_flag |= (TD_NEWBORN|TD_FOLLOW);
			strace_sig(u.u_procp->p_ppid, -1);

			if (u.u_syscall == SYS_FORK || u.u_syscall == SYS_VFORK)
				return(0);
		} else {
			if (strace_debug && 
			    (u.u_syscall != SYS_FORK) && (u.u_syscall != SYS_VFORK))
				printf("t_log: bogus request - pid is %d\n", pid);
			u.u_procp->p_flag &= ~SSCT;
			return(0);
		}
	}
	
	trp = tp->td_trp;
	if (trp == NULL) {
		msg_printf("strace: no memory in strace_log() for PID %d!\n", pid);
		return(0);
	}

	/*
	 *	special system call that user process didn't know it made;
	 *	done after handling a signal
	 */
	if (u.u_syscall == SYS_SIGCLEANUP) {
		trp->t_flags |= T_EINTR;
		return(0);
	}
	if (u.u_syscall == SYS_NOTSYSCALL)
		return(0);	
	
	if (strace_debug & 0x1) 
		printf("t_log: size is %d, count is %d, trp is %x, cmd is %d, syscall is %d\n", tp->td_size, tp->td_count, trp, cmd, u.u_syscall);

	switch(cmd) {
		case TL_ENTRY:	
			log_call(tp, trp, pid);
			break;
							
		case TL_EXIT:	
		case TL_ERR:	
			log_return(tp, trp, cmd);
			break;

		default:
			msg_printf("strace_log: bogus command\n");
			break;
	}
	selwakeup((struct proc *) 0, 1);
		
}



/*
 *	system call is being made - record which one, 
 *	parameters, & timestamp, and mark record as incomplete
 */
log_call(tp, trp, pid)
struct tcb *tp;
struct trace_record *trp;
int pid;	
{
	int count, i;
	char *x;	
	char **argp;
	

	/*
	 *	make sure there's room to log this call
	 */
	for (;;) 
		if ((tp->td_size - tp->td_count) <= 2*ONE_CALL_WORTH) {
			if (strace_debug)
				msg_printf("trace buffer for PID %d full\n", tp->td_pid);
			if (tp->td_flag & TD_WRAP) {
				tp->td_flag |= TD_WRAPPED;
				trp = tp->td_trp = (struct trace_record *) tp->td_addr;
				tp->td_count = tp->td_last_count = 0;
				bzero(trp, ONE_CALL_WORTH);
				break;
			} else {
				tp->td_flag |= TD_WSLEEP;
				sleep(&strace_head, PSTRACE+1);			
				if (tp->td_flag & TD_UNTRACED) {
					zap_tcb(tp);
					return(0);
				}
				tp->td_flag &= ~TD_WSLEEP;
			}
		} else
			break;

	if (trp->t_flags & T_CALL) {	/*  confused because of a signal?  */
		if (strace_debug)
			msg_printf("strace_log: in log_call but call %d for PID %d incomplete...\n", trp->t_spare2, trp->t_pid);
		trp++;		
		tp->td_trp++;
		init_record(trp);
	}	
	trp->t_syscall = u.u_syscall;
	trp->t_flags = T_CALL;
	trp->t_pid = pid;
	trp->t_entry_time = time;
	trp->t_spare2 = tp->td_seqno++;

	count = sysent[u.u_syscall].sy_narg;
	if (count > STRACE_PARAMS)
		count = STRACE_PARAMS;
	for (i = 0; i < count; i++)
		trp->t_params[i] = u.u_arg[i];

	/*
	 *	record string argument in next trace_record, noting this
	 *	fact appropriately in both flags fields and marking the
	 *	space as used
	 */
	switch(trp->t_syscall) {
	    case SYS_ACCESS:
	    case SYS_ACCT:
	    case SYS_CHDIR:
	    case SYS_CHOWN:
	    case SYS_CHMOD:
	    case SYS_CHROOT:
	    case SYS_CREAT:
	    case SYS_GETACCESS:
	    case SYS_GETACL:
	    case SYS_LSTAT:
	    case SYS_MKDIR:
	    case SYS_MKNOD:
	    case SYS_OPEN:
	    case SYS_PATHCONF:
	    case SYS_READLINK:
	    case SYS_RMDIR:
	    case SYS_STAT:
	    case SYS_STATFS:
	    case SYS_SWAPON:
	    case SYS_TRUNCATE:
	    case SYS_UMOUNT:
	    case SYS_UNLINK:
            case SYS_UTIME:
		get_string(tp, trp, trp + 1, trp->t_params[0]);
		break;

	    case SYS_LINK:		/*  2 args  */
	    case SYS_MOUNT:
    	    case SYS_RENAME:
	    case SYS_SYMLINK:
		get_string(tp, trp, trp + 1, trp->t_params[0]);
		get_string(tp, trp, trp + 2, trp->t_params[1]);
		break;
				
	    case SYS_EXECV:		/*  many args  XXX  */
	    case SYS_EXECVE:			    
		get_string(tp, trp, trp + 1, trp->t_params[0]);
		argp = (char **) trp->t_params[1];	/*  argv  */
		for (i = 2; i <= STRACE_PARAMS; i++) {
			x = (char *) fuword(argp++);
			if ((x == (char *) 0) || (x == (char *) -1))
				break;
			else
				get_string(tp, trp, trp + i, x);
		}
		if (i >= STRACE_PARAMS)
			trp->t_flags |= T_ARGC;
		break;

	    /*
	     *	exit(2) never returns, so we fake it
	     */			
	    case SYS_EXIT:
		trp->t_flags &= ~T_CALL;
		trp->t_rv = 0;
		trp->t_exit_time = time;
		tp->td_flag |= TD_DONE;
		if (strace_debug & 0x40)
			printf("strace_log: PID %d done - seqno %d\n", tp->td_pid, trp->t_spare2);
		break;
	}
	tp->td_count += TR_LEN;
	return(0);
}



/*
 *	get a string argument to a system call, and stuff it in the
 *	trace record pointed to by strp
 */
get_string(tp, trp, strp, user_addr)
struct tcb *tp;
struct trace_record *trp, *strp;
char *user_addr;
{
	int i, count;
	
	i = copyinstr(user_addr, &strp->t_syscall, STR_LEN, &count);
	if ((i != 0) || (count < 1)) {
		if (strace_debug)
			msg_printf("strace - copyinstr of %x failed: i is %d, count is %d\n", user_addr, i, count);
		return(0);
	}

	strp->t_flags = T_STRING;
	trp->t_ns++;
	tp->td_count += TR_LEN;
	return(1);
}




/*
 *	log both normal returns and error returns here
 */
log_return(tp, trp, cmd)
struct tcb *tp;
struct trace_record *trp;
int cmd;	
{
	int count;	

	/*	
	 *	system call is now returning - did we know 
	 *	it was going on?  in the 1st case, we just
	 *	missed the call (probably because we started
	 *	tracing after the call was made); in the
	 *	second, we've gotten really confused
	 */
	if (trp->t_syscall == 0) {	
		log_call(tp, trp, tp->td_pid);
		if (tp->td_seqno > 1)
			trp->t_flags |= T_BOGUS;
	}	
	if ((trp->t_syscall != u.u_syscall) && u.u_syscall)
		msg_printf("strace_log: missed a system call for PID %d (%s)...t %d u %d\n", u.u_procp->p_pid, u.u_comm, trp->t_syscall, u.u_syscall);

	if (cmd == TL_EXIT) 		/*  XXX what about u_rval2? (pipe) */
		trp->t_rv = u.u_rval1;		/*  normal  */
	else {
		trp->t_rv = u.u_error;		/*  error of some sort  */
		trp->t_flags |= T_ERROR;
	}
	trp->t_exit_time = time;
	trp->t_flags &= ~(T_CALL|T_ALREADY);	/*  record is complete  */

	/*
	 *	don't allow tracing of setuid/setgid executables
	 */
	if (trp->t_syscall == SYS_EXECV || trp->t_syscall == SYS_EXECVE) 
		if ((u.u_ruid != u.u_uid || u.u_rgid != u.u_gid) &&
		    tp->td_tr_uid) {
			trp->t_flags |= T_SETUID;
			u.u_procp->p_flag &= ~SSCT;
		}
			
	/*
	 *	we're done with this record, so move to the next one;
	 *	jump the pointer over any trailing strings and zero out
	 *	crucial fields in the new record
	 */
	count = trp->t_ns + 1;
	while (count--)
		tp->td_trp++;
	init_record(tp->td_trp);
	return(0);
}



init_record(trp)
struct trace_record *trp;
{
	trp->t_flags = 0;
	trp->t_ns = 0;
	trp->t_syscall = 0;
}



extern int (*log_syscall)();
extern int (*log_signal)();
#ifdef FILE_TRACE
extern int (*log_path)();
#endif

/*
 *  	one-time linking code (so this driver is configurable)
 */
strace_link()
{
	log_syscall = strace_log;   /*  give trap.c something to call  */
	log_signal = strace_sig;    /*  give psig() something to call  */	
#ifdef FILE_TRACE	
	log_path = strace_file;   /*  give lookuppn() something to call  */	
#endif	
}




/* 
 *  	Open the device - note that multiple opens are allowed, and
 *	u.u_fp->f_buf is used to keep the pointer to the TCB; if this
 *	was undesirable, it would be easy to record the tracer's PID
 *	in the structure and search for it each time the TCB was needed
 *
 *	note that this routine does *not* start tracing a process - an ioctl
 *	must be done to tell us which PID
 */
strace_open(dev, flag)
dev_t dev;
int flag;
{
	return(0);
}



int strace_buf_limit = 100;
int strace_bufs = 0;


/*
 *	allocate and initialize a new trace control block
 */
struct tcb *
get_new_tcb()
{
	struct tcb *tp;
	

	if (strace_bufs >= strace_buf_limit)
		return(NULL);
	if ((tp = (struct tcb *) kmalloc(sizeof(struct tcb), M_TRACE, M_WAITOK)) == NULL)
		return(NULL);
	bzero(tp, sizeof(struct tcb));	

	if ((tp->td_addr = kmalloc(TRACE_BUF_SIZE, M_TRACE, M_WAITOK)) == NULL) {
		kfree(tp, M_TRACE);
		return(NULL);
	}
	bzero(tp->td_addr, TRACE_BUF_SIZE);	

	if (strace_debug & 0x04)
		printf("get_new_tcb: tp = 0x%x, addr = 0x%x\n", tp, tp->td_addr);
	strace_bufs++;
	tp->td_size = TRACE_BUF_SIZE;
	tp->td_trp = (struct trace_record *) tp->td_addr;
	tp->td_next = strace_head;
	strace_head = tp;
	return(tp);	
}



/*
 *	close the device, clearing the SSCT bit in p_flag of the tracee(s)
 */
strace_close(dev)
dev_t dev;
{
	struct tcb *tp, *next;
	struct proc *p;
	

/*  XXX  do selwakeup?  */

	if ((tp = (struct tcb *) u.u_fp->f_buf) == NULL) {
		u.u_error = EBADF;
		return(0);
	}
	
#ifdef FILE_TRACE
	if (tp->td_flag & TD_VNODE) {
		tp->td_vp->v_flag &= ~VTRACE;
		VN_RELE(tp->td_vp);
		zap_tcb(tp);
	} else {
#endif	

		if (tp->td_proc && tp->td_proc->p_pid == tp->td_pid)
			tp->td_proc->p_flag &= ~SSCT;

		if (tp->td_flag & TD_WSLEEP) {
			tp->td_flag |= TD_UNTRACED;
			wakeup(&strace_head);
		} else 
			zap_tcb(tp);
#ifdef FILE_TRACE			
	}
#endif	
	u.u_fp->f_buf = (caddr_t) NULL;
}



/*
 *	unlink an entry from the global list (which strace_head anchors)
 *	and then free up the memory
 */
zap_tcb(tp)
struct tcb *tp;
{
	struct tcb *prev;
		
	
	if (tp == strace_head)
		strace_head = tp->td_next;
	else {
		for (prev = strace_head; prev; prev = prev->td_next) 
			if (prev->td_next == tp) {
				prev->td_next = tp->td_next;
				break;
			}
		if (!prev)		/*  something is badly wrong  */
			msg_printf("strace_close: tp %x not in list starting at %x!\n", tp, strace_head);
	}

	if (strace_debug & 0x04)
		printf("zap_tcb: tp = 0x%x, addr = 0x%x\n", tp, tp->td_addr);
	kfree(tp->td_addr, M_TRACE);	
	kfree(tp, M_TRACE);
	strace_bufs--;
}





/*
 *	read - allow user process to retrieve information from the buffer
 */
strace_read(dev, uio)
dev_t dev;
struct uio *uio;
{
	struct tcb *tp;
	char *addr;
	int count;
	struct trace_record *temp;
	
	
	tp = (struct tcb *) u.u_fp->f_buf;
	if (tp == NULL || (addr = tp->td_addr) == NULL) {  /*  sanity check  */
		uprintf("no memory in strace_read!\n");
		return(0);
	}

	/*
	 *	return data from the tracee, preserving state
	 */
	count = tp->td_count;
	addr = tp->td_addr;
	uiomove(addr, count, UIO_READ, uio);	/*  check error  */
	bcopy(tp->td_trp, addr, ONE_CALL_WORTH);
	tp->td_trp = (struct trace_record *) addr;

	/*
	 * 	this little mess is to deal with string arguments to 
	 *	in-progress system calls (with trailing strings?)
	 */
	if (tp->td_trp->t_flags & T_CALL) {
		tp->td_count = (tp->td_trp->t_ns + 1)*TR_LEN;
		tp->td_trp->t_flags |= T_ALREADY;
	} else
		tp->td_count = 0;

	/*
	 *	only report signals once (well, not really, but do flag
	 *	them after the first time)
	 */
	temp = tp->td_trp + tp->td_trp->t_ns;
	if (temp->t_flags & T_SIGNAL)
		temp->t_flags |= T_ALREADY;
		
	tp->td_last_count = tp->td_count;  /*  XXX what about completion of a record? right now it doesn't let tracer proceed  */
	if (tp->td_flag & TD_WSLEEP)	/*  now tracee can proceed  */
		wakeup(&strace_head);

	return(0);
}




/*
 *	ioctls
 *		- set PID to trace
 *		- OR in <flags> - currently only meaningful thing is to 
 *		  follow children (fork/vfork)
 *
 *	should have one to  - allow logging to continue even if buffer fills
 *	                    - set trace buffer size
 *
 */
strace_ioctl(dev, cmd, addr, flag)
dev_t dev;
int cmd;
int *addr;
int flag;
{
	struct tcb *tp;
	struct proc *p;
#ifdef FILE_TRACE
	struct pathname pn;
	struct vnode *vp;
	struct vattr va;
#endif			

	switch(cmd) {
		/*
		 *	find the PID; if it exists and belongs to this 
		 *	user (in one sense or another :-), set trace bit
		 */
		case STRACE_SET_PID:
			p = pfind(*addr);
			if (p == 0) 
				return(ESRCH);
			if (u.u_uid != p->p_uid && !suser())	/*  check GID?  */
				return(EPERM);
			if ((tp = get_new_tcb()) == NULL)
				return(ENOMEM);
			tp->td_pid = *addr;
			tp->td_proc = p;
			tp->td_flag &= ~TD_WRAP;	/*  XXX  below too?  */
			tp->td_tr_uid = u.u_uid;
			p->p_flag |= SSCT;
			u.u_fp->f_buf = (caddr_t) tp;
			break;

		case STRACE_NEXT_CHILD:
			for (tp = strace_head; tp; tp = tp->td_next)
				if (*addr == tp->td_pid ||
				    (*addr == 0 && (tp->td_flag & TD_NEWBORN)))
				    	break;
			if (!tp)
				return(ESRCH);
			if (!(tp->td_flag & TD_NEWBORN))
				return(EALREADY);
				    
			p = tp->td_proc;
			if (u.u_uid != p->p_uid && !suser()) {
				tp->td_flag |= TD_WRAP;
				return(EPERM);
			}
			tp->td_flag &= ~TD_NEWBORN;
			tp->td_tr_uid = u.u_uid;
			*addr = tp->td_pid;
			u.u_fp->f_buf = (caddr_t) tp;
			break;

		case STRACE_SET_FLAGS:
			tp = (struct tcb *) u.u_fp->f_buf;
			if (tp)
				tp->td_flag |= *addr;
			else
				return(ENXIO);
			break;

#ifdef FILE_TRACE
		case STRACE_SET_PATH:
		        if (u.u_error = pn_get(*addr, UIOSEG_USER, &pn)) 
	                        return(0);
			if (u.u_error = lookuppn(&pn, NO_FOLLOW, 
						 (struct vnode **)0, &vp, 
			                         LKUP_LOOKUP, (caddr_t)0)) {
			        pn_free(&pn);
			   	return(0);
			}                     	                
		        pn_free(&pn);
			if (u.u_error = VOP_GETATTR(vp, &va, u.u_cred,VSYNC)) 
				return(0);
			if ((va.va_uid != u.u_uid) && (!suser()))
				return(EPERM);
			if ((tp = get_new_tcb()) == NULL)
				return(ENOMEM);
			u.u_fp->f_buf = (caddr_t) tp;
			vp->v_flag |= VTRACE;
			VN_HOLD(vp);		
			tp->td_vp = vp;
			tp->td_flag |= TD_VNODE;
			break;
#endif			
		default:
			return(EIO);
	}
	return(0);
}


#define EXCEPT 0


strace_select(dev, which)
dev_t dev;
int which;
{
	struct tcb *tp;
	struct proc *p;


	if ((tp = (struct tcb *) u.u_fp->f_buf) == NULL) {
		u.u_error = EBADF;
		return(0);
	}

	if (strace_debug & 0x200)
		printf("s_sel: trp is %x, count is %d, type is %d, PID is %d\n",
			tp->td_trp, tp->td_count, which, tp->td_pid);
			
	switch (which) {
		
		case FREAD: 
			if ((tp->td_count - tp->td_last_count) > 0)
				return(1);
			break;
			
		case EXCEPT:
#ifdef FILE_TRACE		
			if (tp->td_flag & TD_VNODE)
				return(0);
#endif				
			p = tp->td_proc;
			if (p == NULL || p->p_stat == 0 || 
			    p->p_stat == SZOMB || p->p_pid != tp->td_pid)
			    	return(1);
			break;
	}
		
	return(0);		
}



#endif  /*  STRACE  */

