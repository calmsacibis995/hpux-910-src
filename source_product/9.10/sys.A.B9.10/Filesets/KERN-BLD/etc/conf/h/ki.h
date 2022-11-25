/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/ki.h,v $
 * $Revision: 1.9.83.4 $	$Author: kcs $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 18:27:57 $
 */
/* HPUX_ID: @(#)ki.h    54.1            88/11/13 */
#ifndef _SYS_KI_INCLUDED /* allows multiple inclusion */
#define _SYS_KI_INCLUDED

#define KI_VERSION      "9.0 HP-UX"

/* header file for the KI */
/* set max pathname length from user process in trace buffer */
#define	KI_MAXPATH	256

/* set min timeout for KI_TIMEOUT_SET at 200 Ms */       
#define KI_MINTIMEOUT   (HZ/5)

#define MIN_TBSZ        16384   /* MIN size of trace buffer */
#define MAX_TBSZ        2097152 /* MAX size of trace buffer */
#define MIN_CTSZ        4       /* MIN size of counter buffer */
#define MAX_CTSZ        2097152 /* MAX size of counter buffer */
#define MAX_TRACE_REC   128     /* MAX size of USER trace struct */

/* unique bits to not match with any possible dev_t bits */
#define NO_DEV          -13
#define DEV_SOCKET      -12     /* communications endpoint */
#define DEV_NSIPC       -11     /* CND communication type */   
#define DEV_UNSP        -10     /* user nsp control */
#define DEV_PIPE        -9      /* Pipe (not used by KI) */
#define DEV_LLA         -8      /* link-level lan access */

/* sometimes pid and site are not valid as on ICS */
#define NO_PID          -1      /* Process id is not valid */

/* shorten some typing for structure reference */
#define kd_pid          kd_struct.P_pid
#define kd_recid        kd_struct.rec_id
#define kd_site         kd_struct.P_site
#define kd_cpu          kd_struct.P_cpu
#define ks_pid          kd_syscall.kd_struct.P_pid

/* common header for all trace structures */
struct kd_struct {
	u_short		rec_sz;		/* 2 record length		*/
	u_short		rec_id;		/* 2 record ident		*/
	pid_t		P_pid;		/* 4 process id			*/
	struct ki_timeval cur_time;	/* 8 time of record creation	*/
	u_short 	P_site;		/* 2 cnode number		*/
        short           P_cpu;          /* 2 cpu is process is running	*/
	u_int		seqcnt;		/* 4 sequence counter for debug */
/*      The sum here should add to mod 8  __ */
/*                                        24 */
};

/* specific ki iovec structure */
struct ki_iovec {
        caddr_t         iov_base;
        int             iov_len;
};

#define KS_stype        kd_syscall.ks_stype

#define KD_SYSCALL_GENERIC      0
/* this is the GENERIC system call trace structure */
struct  kd_syscall {
        struct kd_struct  kd_struct;    /* KI trace common header            */
        struct ki_timeval sttime;       /* start time of call                */
        u_short           ks_stype;     /* this structure type               */
        u_short           ks_nargs;     /* # of passed arguments from call   */
        u_short           ks_syscal;    /* system call number                */
        u_short           ks_error;     /* returned u_error                  */
        int               ks_rval1;     /* rtn value1 from system call       */
        int               ks_rval2;     /* rtn value2 from system call       */
        struct ki_timeval ks_sys_time;  /* KT_SYS time - last call or resume */
/*      int               u_arg[N];       arguments to the system call       */
};

/* Currently "f_type"s are DTYPE_VNODE, DTYPE_SOCKET, DTYPE_UNSP, DTYPE_LLA  */

#define KD_SYSCALL_FD           1       
struct  kd_fd {                         /* System calls fd = 1st parameter   */
        struct kd_syscall kd_syscall;   /* KI trace common header            */
        caddr_t		  f_data;       /* vnode, socket pointer etc.        */
        short		  f_type;       /* type of f_data                    */
	short		  pad;
/*      int               u_arg[N];        arguments to the system call      */
};

#define KD_SYSCALL_1STRING      2
struct  kd_1string {                    /* for string in 1st parameter       */
        struct kd_syscall kd_syscall;   /* KI trace common header            */
/*      int               u_arg[N];        arguments to the system call      */
/*      char              name1[s1];       path name from user process       */
};

#define KD_SYSCALL_2STRING      3
struct  kd_2string {                    /* for string in 1st/2nd parameters  */
        struct kd_syscall kd_syscall;   /* KI trace common header            */
/*      int               u_arg[N];        arguments to the system call      */
/*      char              name1[s1];       path name from user process       */
/*      char              name2[s2];       path name from user process       */
};

#define KD_SYSCALL_OPEN         4
struct  kd_open {                       /* for open/create system call       */
        struct kd_syscall kd_syscall;   /* KI trace common header            */
        caddr_t		  f_data;       /* vnode, socket pointer etc.        */
        short		  f_type;       /* type of f_data                    */
	short		  pad;
/*      int               u_arg[N];        arguments to the system call      */
/*      char              name1[s1];       path name from user process       */
};

#define KD_SYSCALL_EXEC         5
struct  kd_exec {                       /* for exec system calls             */
        struct kd_syscall kd_syscall;   /* KI trace common header            */
        struct vnode      *vp;          /* a.out vnode pointer               */
/*      int             u_arg[N];          arguments to the system call      */
/*      char            pname[s1];         exec process argv[0]              */
};

#define KD_SYSCALL_FORK         6
struct  kd_fork {                       /* for fork and vfork                */
        struct kd_syscall kd_syscall;   /* KI trace common header            */
        dev_t             p_ttyd;       /* controlling tty dev               */
        uid_t             p_uid;        /* [tty signals]   proc user  id     */
        uid_t             p_suid;       /* set (effective) proc user  id     */
        pid_t             p_pgrp;       /* name of process group leader      */
        pid_t             p_ppid;       /* process id of parent              */
        uid_t             kd_u_uid;	/*                 user user  id     */
        uid_t             kd_u_ruid;	/* real            user user  id     */
        gid_t             kd_u_gid;	/*                 user group id     */
        gid_t             kd_u_rgid;	/* real            user group id     */
        gid_t             u_sgid;	/* set (effective) user group id     */
/*      int               u_arg[N];        arguments to the system call */
};

#define KD_SYSCALL_FD2          7       
struct  kd_fd2 {                        /* System calls fd = 1st parameter   */
        struct kd_syscall kd_syscall;   /* KI trace common header            */
        caddr_t		  f_data1;      /* vnode pointer etc.                */
        caddr_t		  f_data2;      /* vnode pointer etc.                */
        short		  f_type1;      /* type of f_data1                   */
        short		  f_type2;      /* type of f_data2                   */
/*      int               u_arg[N];        arguments to the system call      */
};

#define KD_SYSCALL_MMAP         8
struct  kd_mmap {                       /* for exec system calls             */
        struct kd_syscall kd_syscall;   /* KI trace common header            */
        struct	vnode	  *r_fstore;    /* front vnode     pointer           */
        struct	vnode	  *r_bstore;    /* back  vnode     pointer           */
        reg_t             *rp;          /* process region  pointer           */
        preg_t            *prp;         /* process pregion pointer           */
        reg_t             region;       /* process region                    */
};

/* BEGIN the non-system call trace buffers */
struct  kd_enqueue {
        struct kd_struct kd_struct;     /* KI trace common header            */
        struct ki_timeval b_timeval_lu; /* buffer last used time             */
        struct buf      *bp;            /* buffer ident                      */
        dev_t           b_dev;          /* major+minor device name           */
        long            b_flags;        /* too much goes here to describe    */
        long            b_bcount;       /* transfer count                    */
        daddr_t         b_blkno;        /* block # on device                 */
        u_short         b_bptype;       /* buffer type                       */
        u_short         b_queuelen;     /* disc queue length (+long align bp)*/
        pid_t           b_upid;         /* pid that last used bp             */
        pid_t           b_apid;         /* pid that last allocated bp        */
        struct ki_timeval b_timeval_at; /* buffer allocation time            */
        caddr_t         b_vp;           /* vnode associated with block       */
};

struct  kd_queuestart {
        struct kd_struct kd_struct;     /* KI trace common header            */
        struct buf      *bp;            /* buffer ident                      */
        caddr_t          b_vp;          /* vnode associated with block       */
};

struct  kd_queuedone {
        struct kd_struct kd_struct;     /* KI trace common header            */
        struct ki_timeval b_timeval_qs; /* queuestart time of request        */
        struct buf      *bp;            /* buffer ident                      */
        dev_t           b_dev;          /* major+minor device name           */
        long            b_flags;        /* too much goes here to describe    */
        long            b_bcount;       /* transfer count                    */
        daddr_t         b_blkno;        /* block # on device                 */
        u_short         b_bptype;       /* buffer type                       */
        u_short         b_queuelen;     /* disc queue length (+long align bp)*/
        pid_t           b_upid;         /* pid that last used bp             */
        pid_t           b_apid;         /* pid that last allocated bp        */
        struct ki_timeval b_timeval_eq; /* enqueue time of request           */
        u_int           b_resid;        /* wds not transferred               */
        site_t          b_site;         /* site that allocated this bp       */
        caddr_t         b_vp;           /* vnode associated with block       */
};

struct  kd_hardclock {
        struct kd_struct kd_struct;     /* KI trace common header            */
        caddr_t          pc;            /* interrupt code pc value           */
        short            cpstate;       /* processor state                   */
};

struct  kd_brelse {
        struct kd_struct kd_struct;     /* KI trace common header            */
        struct ki_timeval sttime;       /* time of removal from lru          */
        struct buf      *bp;            /* buffer ident                      */
        caddr_t          b_vp;          /* vnode associated with block       */
        long             b_flags;       /* too much goes here to describe    */
        u_short          b_bptype;      /* buffer type                       */
	short		 pad;
};

struct  kd_getnewbuf {
        struct kd_struct kd_struct;     /* KI trace common header            */
        struct buf      *bp;            /* buffer ident                      */
        long             b_bufsize;     /* size of allocated buffer          */
        u_short          b_bptype;      /* buffer type                       */
        caddr_t          b_vp;          /* vnode associated with block       */
};

struct  kd_miss_alpha {
        struct kd_struct kd_struct;     /* KI trace common header            */
        int nswitches;                  /* # of switches for a missed alpha  */
};

struct  kd_swtch {
        struct kd_struct kd_struct;     /* KI trace common header            */
        caddr_t          ki_p_wchan;    /* event process is awaiting         */
        caddr_t          ki_sleep_caller;/* return address from sleep        */

	/* current KTC clocks */
	struct ki_runtimes	ks_swt[KT_NUMB_CLOCKS];

        long            ki_ru_minflt;   /* page reclaims                     */
        long            ki_ru_majflt;   /* page faults                       */
        long            ki_ru_nswap;    /* number swap for this proc         */
        long            ki_ru_nsignals; /* number signals sent to this proc  */
        long            ki_ru_maxrss;   /* high water mark rss of all pages  */
        long            ki_ru_msgsnd;   /* messages sent                     */
        long            ki_ru_msgrcv;   /* messages received                 */

	/* time last system call executed current process */
	struct ki_timeval ki_syscallbeg;/* start time of last systemcall     */
        struct ki_timeval ks_sys_time;  /* KT_SYS time - last call or resume */
        u_short         ks_syscal;      /* last system call number           */

        char            ki_p_stat;      /* status of current process         */
        u_char          ki_p_pri;       /* process priority                  */
        u_char          ki_p_cpu;       /* /* cpu usage for scheduling       */
        u_char          ki_p_usrpri;    /* priority based on p_cpu & p_nice  */
        u_char          ki_p_rtpri;     /* real time priority                */
        u_char          ki_p_nice;      /* process nice priority             */
};

struct  kd_region {
        struct kd_struct kd_struct;     /* KI trace common header            */
        int              cl_numb;       /* region operation                  */
        reg_t            *rp;           /* process region pointer            */
        reg_t            region;        /* process region                    */
};

struct  kd_pregion {
        struct kd_struct kd_struct;     /* KI trace common header            */
        int             cl_numb;        /* pregion operation                 */
        preg_t          *prp;           /* process pregion pointer           */
        preg_t          pregion;        /* process pregion                   */
};

struct  kd_resume_csw {
	struct kd_struct kd_struct;	/* header placed by ki_data */

	/* current KTC clocks */
	struct ki_runtimes	ks_swt[KT_NUMB_CLOCKS];

	u_char		ki_p_pri;	/* prio, lower numbers are higher pri */
	u_char		pad[3];		/* mod 4 size */
};

struct  kd_dm1_send {
        struct kd_struct kd_struct;     /* KI trace common header            */
        struct ki_timeval sttime;       /* start time of diskless operation  */
        u_short           kds_op;       /* diskless operation                */
        u_short           kds_site;     /* diskless cnode number             */
        u_short           kds_length;   /* packet length                     */
        short             kds_dmmsg_length;   /* message length part         */
        short             kds_data_length;    /* data    lenght part         */
};

struct  kd_dm1_recv {
        struct kd_struct kd_struct;     /* KI trace common header            */
        struct ki_timeval sttime;       /* start time of diskless operation  */
        struct ki_timeval send_time;    /* dm1_send time of request          */
        pid_t             b_upid;       /* pid that last used bp             */
        pid_t             b_apid;       /* pid that last allocated bp        */
        u_short           kdr_op;       /* diskless operation                */
        u_short           kdr_site;     /* diskless cnode number             */
        u_short           kdr_length;   /* packet length                     */
        short             kdr_dmmsg_length;   /* message length part         */
        short             kdr_data_length;    /* data    lenght part         */
};

struct  kd_servestrat {
        struct kd_struct kd_struct;     /* KI trace common header            */
        struct ki_timeval sttime;       /* start time of diskless operation  */
        u_char          rw;             /* read/write flag                   */
        dev_t           dev;            /* major+minor device name           */
        u_int           length;         /* length of read or write request   */
};

struct  kd_servsyncio {
        struct kd_struct kd_struct;     /* KI trace common header            */
        struct ki_timeval sttime;       /* start time of diskless operation  */
        u_char          rw;             /* read/write flag                   */
        dev_t           dev;            /* major+minor device name           */
        u_int           length;         /* length of read or write request   */
};

struct  kd_rfscall {
        struct kd_struct kd_struct;     /* KI trace common header            */
        struct ki_timeval sttime;       /* start time of NFS operation       */
        int               kr_which;     /* NFS layer number                  */
        dev_t             kr_mntno;     /* NFS mntinfo index                 */
        u_int             kr_addr;      /* internet number                   */
        u_int             kr_length;    /* length if read or write request   */
};

struct  kd_rfs_dispatch {
        struct kd_struct kd_struct;     /* KI trace common header            */
        struct ki_timeval sttime;       /* start time of NFS operation       */
        int               kr_which;     /* NFS layer number                  */
        u_int             kr_addr;      /* internet number                   */
        u_int             kr_length;    /* length if read or write request   */
};

struct  kd_do_bio {
        struct kd_struct kd_struct;     /* KI trace common header            */
        struct buf      *bp;            /* buffer ident                      */
        dev_t            mntno;         /* major+minor device name           */
        long             b_flags;       /* too much goes here to describe    */
        long             b_bcount;      /* transfer count                    */
        u_short          b_bptype;      /* buffer type                       */
        u_short          b_queuelen;    /* disc queue length (+long align bp)*/
        pid_t            b_upid;        /* pid that last used bp             */
        pid_t            b_apid;        /* pid that last allocated bp        */
        struct ki_timeval b_timeval_eq; /* enqueue time of request           */
        struct ki_timeval b_timeval_qs; /* queuestart time of request        */
        u_int            b_resid;       /* words not transferred             */
        u_int            ip_addr;       /* internet number                   */
        caddr_t          b_vp;          /* vnode associated with block       */
};

struct  kd_setrq {
        struct kd_struct kd_struct;     /* KI trace common header            */
        caddr_t          setrq_caller;  /* return address from setrq         */
        pid_t            p_pid;         /* pid being queued                  */
        u_char           p_pri;         /* process priority                  */
};

struct kd_closef {
        struct kd_struct kd_struct;     /* KI trace common header            */
        caddr_t		  f_data;       /* vnode, socket pointer etc.        */
        short		  f_type; 	/* type of f_data                    */
        short             f_count;      /* open count after closef           */
};

struct kd_locallookuppn {
        struct kd_struct kd_struct;     /* KI trace common header            */
        struct ki_timeval sttime;     	/* start time of call                */
        struct vnode	 *vp;		/* vnode                             */
        dev_t             dev;          /* major+minor device name           */
	ino_t		  i_number;	/* inode number                      */
	u_char            v_fstype;
	u_char            v_type;
        short             error;      	/* error code                        */
};

struct kd_vfault {
        struct kd_struct kd_struct;     /* KI trace common header            */
        int              cl_numb;       /* caller number                     */
        int              wrt;           /* read/write flag                   */
        int              space;         /* space addresss of fault           */
        caddr_t          vaddr;         /* virtual addresss of fault         */
        caddr_t          pcoq_head;     /* instruction @ of fault            */
        int              r_nvalid;      /* number resident pages in region   */
        preg_t           *prp;		/* pregion pointer of fault          */
        preg_t           pregion;       /* pregion of fault                  */
};

struct kd_asyncpageio {
        struct kd_struct kd_struct;     /* KI trace common header            */
        struct buf      *bp;            /* buffer ident                      */
        caddr_t         b_vp;           /* vnode associated with block       */
        long            b_flags;        /* too much goes here to describe    */
        long            b_bcount;       /* transfer count                    */
        daddr_t         b_blkno;        /* block # on device                 */
        u_short         b_bptype;       /* buffer type                       */
	short		pad;
};

struct kd_memfree {
        struct kd_struct kd_struct;     /* KI trace common header            */
        reg_t            *rp;		/* process region  pointer           */
        int              r_nvalid;      /* number resident pages in region   */
	int		 pfn;		/* page frame number	 	     */
};

struct  kd_userproc {
	struct kd_struct kd_struct;	/* header placed by ki_data */

	/* current KTC clocks */
	struct ki_runtimes	ks_swt[KT_NUMB_CLOCKS];
/*      char            userbuf[];      user passed buffer                   */
};
#endif /* _SYS_KI_INCLUDED */

