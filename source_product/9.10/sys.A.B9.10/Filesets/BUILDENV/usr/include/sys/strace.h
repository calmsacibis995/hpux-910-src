/*
 * @(#)strace.h: $Revision: 1.2.84.10 $ $Date: 94/11/29 14:44:58 $
 * $Locker:  $
 */

/* HPUX_ID: @(#)STRACE.h        52.3     88/08/02  */
#ifndef _MACHINE_STRACE_INCLUDED /* allow multiple inclusions */
#define _MACHINE_STRACE_INCLUDED

#ifdef  _KERNEL_BUILD
#include "../h/ioctl.h"
#else   /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */




/* 
 *      ioctls
 */
#define STRACE_SET_PID    _IOW('T', 1, int)     /*  which PID to trace  */
#define STRACE_SET_FLAGS  _IOW('T', 2, int)     /*  flags: see TD_* list  */
#define STRACE_SET_PATH   _IOW('T', 3, char *)  /*  pathname to watch  */
#define STRACE_NEXT_CHILD _IOWR('T', 4, int)    /*  trace last forked tracee  */


/* 
 *       commands for trap.c to use when calling strace_log()
 */
#define TL_ENTRY  1
#define TL_EXIT   2
#define TL_ERR    3



#define PSTRACE PZERO+2   /*  interruptible sleep when buffer is full/empty  */
#define STRACE_PARAMS 8   /*  max # of syscall arguments we'll record  */
#define TR_LEN sizeof(struct trace_record)
#define STR_LEN (TR_LEN - sizeof(short))  /*  strings go right after flags  */
#define ONE_CALL_WORTH (STRACE_PARAMS+1)*TR_LEN  /*  what it sounds like :-)  */
#define TRACE_BUF_SIZE 16384    /*  16384/64 = 256 syscalls to fill buffer  */


/*
 *      each system call generates one of these (plus one for each string arg)
 */
struct trace_record {
        short t_flags;
        short t_syscall;
        short t_pid;
        short t_ns;
        int t_rv;
        struct timeval t_entry_time;
        struct timeval t_exit_time;
        int t_params[STRACE_PARAMS];
        int t_spare2;
};


/*  
 *      trace_record flag bits  
 */
#define T_SIGNAL  0x0001  /*  this was really a signal, not a syscall   */
#define T_NEXT    0x0002  /*  the next record has data from this one    */
#define T_STRING  0x0004  /*  this is a string param from earlier record  */
#define T_EINTR   0x0008  /*  a signal was delivered in here somewhere  */
#define T_BOGUS   0x0010  /*  didn't get the call, so return may be flaky  */
#define T_CALL    0x0020  /*  call info is in here, but not the return val  */
#define T_ERROR   0x0040  /*  error; we're returning u.u_error, not u_rval1  */
#define T_ARGC    0x0080  /*  there were > 8 args (to exec(2))     */
#define T_SETUID  0x0100  /*  process just exec'ed setuid program  */
#define T_ALREADY 0x0200  /*  we've already reported this signal  */

/*
 *      one of these for each process being traced
 */
struct tcb {
        struct tcb *td_next;    /*  all traced processes  */
        struct proc *td_proc;   /*  process being traced  */
        int td_pid;             /*  which process is being traced  */
        char *td_addr;          /*  the actual trace buffer (malloced)  */
        struct trace_record *td_trp;  /*  where we are in buffer  */
        short td_size;          /*  how many bytes does <addr> point at?  */
        short td_count;         /*  how many of those bytes are taken?    */
        short td_last_count;    /*  what was it last time?  */
        short td_flag;          /*  flags - see below  */
        int td_seqno;
        uid_t td_tr_uid;        /*  uid of tracer  */
        struct vnode *td_vp;    /*  ptr to vnode we are tracing access to  */   
};


#define TD_WSLEEP   0x0001      /*  traced process is waiting (buffer full)  */
#define TD_RSLEEP   0x0002      /*  tracer is waiting (buffer empty)  */
#define TD_DONE     0x0004      /*  traced process has exited  */
#define TD_UNTRACED 0x0008      /*  tracer has exited  */
#define TD_FOLLOW   0x0010      /*  follow forks (start tracing children)  */
#define TD_VNODE    0x0020      /*  tracing access to vp, not proc syscalls  */
#define TD_NEWBORN  0x0040      /*  child not yet acquired by tracer  */
#define TD_WRAP     0x0080
#define TD_WRAPPED  0x0100
#endif /* _MACHINE_STRACE_INCLUDED */
