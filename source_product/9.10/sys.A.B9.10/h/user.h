/* @(#) $Revision: 1.68.83.6 $ */

#ifndef _SYS_USER_INCLUDED /* allows multiple inclusion */
#define _SYS_USER_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _KERNEL
#include "../machine/pcb.h"
#include "../h/time.h"
#include "../h/resource.h"
#include "../h/privgrp.h"
#include "../h/errno.h"		/* u_error codes */
#include "../h/signal.h"	/* SIGARRAYSIZE */
#include "../h/proc.h"
#ifdef __hp9000s300
#include "../s200/a.out.h"
#endif /* __hp9000s300 */
#ifdef __hp9000s800
#include "../h/vmmac.h"
#include "../machine/save_state.h"
#include "../machine/som.h"
#endif /* __hp9000s800 */
#else /* not _KERNEL */
#include <machine/pcb.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/privgrp.h>
#include <errno.h>		/* u_error codes */
#include <sys/signal.h>		/* SIGARRAYSIZE */
#include <sys/proc.h>
#ifdef __hp9000s300
#include <a.out.h>
#endif /* __hp9000s300 */
#ifdef __hp9000s800
#include <sys/vmmac.h>
#include <machine/save_state.h>
#include <machine/som.h>
#endif /* __hp9000s800 */
#endif /* not _KERNEL */

/* 
 * NFDCHUNKS = number of file descriptor chunks of size SFDCHUNK available
 * per process.  SFDCHUNK must be NBTSPW = number of bits per int for
 * select to work.
 */
#define SFDCHUNK	NBTSPW
#define NWORDS(n)       ((((n) & (SFDCHUNK - 1)) == 0) ? (n >> 5) :	\
							 ((n >> 5) + 1))
/* NWORDS is the number of words necessary for n file descriptors to allow 
   for one bit per file descriptor. */

#define NFDCHUNKS(n)    NWORDS(n)

/*
 * Some constants for fast multiplying, dividing, and mod-ing (%) by SFDCHUNK
 */

#define SFDMASK	0x1f
#define SFDSHIFT 5

struct ofile_t {
	struct file *ofile[SFDCHUNK];	/* file descriptor slots */
	char pofile[SFDCHUNK];		/* per process open file flags */
};

/*
 * since fuser() needs this information, we move it to the proc structure
 * since uareas can be swapped out.  In previous releases, fuser() was
 * able to scan through the logical swap device to retrieve this information
 * however, that capability is no longer supported.
 */
#define u_maxof u_procp->p_maxof        /* max # of open files allowed */
#define u_rdir u_procp->p_rdir          /* root directory of current process */
#define u_cdir u_procp->p_cdir          /* current directory */
#define u_ofilep u_procp->p_ofilep      /* pointers to file descriptor chunks
                                           to be allocated as needed. */

/*
 * maxfiles is maximum number of open files per process.  
 * This is also the "soft limit" for the maximum number of open files per 
 * process.  maxfiles_lim is the "hard limit" for the maximum number of open
 * files per process.
 */
extern	int maxfiles; 
extern  int maxfiles_lim;

#define LOCK_TRACK_MAX          10              /* for qfs lock tracking */

/*
 * Per process structure containing data that
 * isn't needed in core when the process is swapped out.
 */
 
#define	SHSIZE		32

typedef struct user {
#ifdef __hp9000s800
	struct	pcb u_pcb;
#endif /* __hp9000s800 */
	struct	proc *u_procp;		/* pointer to proc structure */
#ifdef __hp9000s800
	struct	save_state *u_sstatep;	/* pointer to a saved state */
#endif /* __hp9000s800 */
#ifdef __hp9000s300
	int	*u_ar0;			/* address of users saved R0 */
#endif /* __hp9000s300 */
	char	u_comm[MAXCOMLEN + 1];

/* syscall parameters, results and catches */
	int     u_arg[10];              /* arguments to current system call */
	int	*u_ap;			/* pointer to arglist */
	label_t	u_qsave;		/* for non-local gotos on interrupts */
	u_short u_spare_short;		/* Replaces top half of u_error */
	u_short u_error;                /* return error code */

	union {				/* syscall return values */
		struct	{
			int	R_val1;
			int	R_val2;
		} u_rv;
#define	r_val1	u_rv.R_val1
#define	r_val2	u_rv.R_val2

/* Bell-to-Berkeley translations */
#define u_rval1		u_r.r_val1
#define u_rval2		u_r.r_val2

		off_t	r_off;
		time_t	r_time;
	} u_r;
	char	u_eosys;		/* special action on end of syscall */
	u_short	u_syscall;		/* syscall # passed to signal handler */

/* 1.1 - processes and protection */
	struct  ucred *u_cred;           /* user credentials (uid, gid, etc) */
#define	u_uid	u_cred->cr_uid
#define	u_gid	u_cred->cr_gid
#define	u_groups u_cred->cr_groups	/* groups, NOGROUP terminated */
#define	u_ruid	u_cred->cr_ruid
#define	u_rgid	u_cred->cr_rgid
#ifdef AUDIT
	aid_t	u_aid;			/* audit id */
	short	u_audproc;		/* audit process flag */
	short	u_audsusp;		/* audit suspend flag */
	struct	audit_filename *u_audpath;	/* ptr to audit pathname info */
	struct	audit_string *u_audstr;	/* ptr to string data for auditing */
	struct	audit_sock *u_audsock;	/* ptr to sockaddr data for auditing */
	char	*u_audxparam;		/* generic loc. to attach audit data */
#endif
#ifdef __hp9000s800
#ifdef AUDIT
	u_int   u_spare1[5];		/* spares for backward compatibility */
#else
 	u_int	u_spare1[11];		/* spares for backward compatibility */
#endif /* AUDIT */
#endif /* __hp9000s800 */
#ifdef _CLASSIC_ID_TYPES
	unsigned short u_filler_sgid;
	unsigned short u_sgid;		/* set (effective) gid */
#else
	gid_t	u_sgid;			/* set (effective) gid */
#endif
	u_int	u_priv[PRIV_MASKSIZ];	/* privlege mask */

/* 1.2 - memory management */
	label_t u_ssave;		/* label variable for swapping */
#ifdef __hp9000s800
	tlabel_t u_psave;		/* trap recovery vector - machine dep */
#endif /* __hp9000s800 */
#ifdef __hp9000s300
	label_t u_rsave;		/* for exchanging stacks */
	label_t u_psave;		/* for probe simulation */
#endif /* __hp9000s300 */
	time_t	u_outime;		/* user time at last sample */
	short   u_flag;                 /* See u_flag values */
#define UF_MEMSIGL  0x00000001          /* Signal upon memory allocation
					 *  and process locked 
					 */
#ifdef	_KERNEL
#ifdef MP_LIFTTW

/*
 * These are all involved with lifting the training wheels
 * For more details, see setjmp_lifttw() and longjmp_lifttw()
 */
#define UF_KS_REAQUIRE	0x0002		/* Reaquire kernel sema */
#define setjmp(ss)	(setjmp_lifttw(ss),setjmp_real(ss))
#define longjmp(ss)	(longjmp_lifttw(ss),longjmp_real(ss))

#endif /* MP_LIFTTW */
#endif	/* _KERNEL */

/* 1.3 - signal management */
		/* same for users and the kernel; see signal.h		  */
	void    (*u_signal[SIGARRAYSIZE])();    /* disposition of signals */
	int	u_sigmask[SIGARRAYSIZE];	/* signals to be blocked */
	int	u_sigonstack;		/* signals to take on sigstack */
	int	u_oldmask;		/* saved mask from before sigpause */
	int	u_code;			/* ``code'' to trap */
	struct	sigstack u_sigstack;	/* sp & on stack state variable */
#define	u_onstack	u_sigstack.ss_onstack
#define	u_sigsp		u_sigstack.ss_sp
#ifdef __hp9000s800
	void	(*u_sigreturn)();	/* handler return address */
#define PA83_CONTEXT	0x1
#define PA89_CONTEXT	0x2
	int	u_sigcontexttype;	/* to tell PA83 from PA89 contexts */
#endif /* __hp9000s800 */
#ifdef __hp9000s300
	int	u_sigcode[6];	  	/* signal "trampoline" code */
#endif /* __hp9000s300 */
	int	u_sigreset;	  	/* reset handler after catching */
#ifdef __hp9000s300
	size_t 	u_lockovh;		/* locked proc overhead size (clicks) */
					/* belongs with u_locksdsize */
#endif /* __hp9000s300 */

/* 1.4 - descriptor management */

#define	UF_EXCLOSE 	0x1		/* auto-close on exec */
#define	UF_MAPPED 	0x2		/* mapped from device */
	int	u_highestfd;		/* highest file descriptor currently
					   opened by this process. */
#ifdef _WSIO
	struct	file *u_fp;		/* current file pointer */
#endif /* _WSIO */
#define UF_FDLOCK	0x4		/* lockf was done,see vno_lockrelease */
	int	u_spare2[1];		/* spare */
#ifdef  HPNSE
        dev_t   u_ttyd;                 /* controlling tty dev */
#endif
	short	u_cmask;		/* mask for file creation */

/* 1.5 - timing and statistics */
	/* The user accumulated seconds and system accumulated seconds fields 
	 * of the following structure are maintained in the proc structure. 
	 * This should be taken into account in computations.
	 */
	struct	rusage u_ru;		/* stats for this proc */
	struct	rusage u_cru;		/* sum of stats for reaped children */
	struct	itimerval u_timer[3];
	int     u_XXX[2];
	time_t	u_ticks;
	short	u_acflag;

/* 1.6 - resource controls */
	struct	rlimit u_rlimit[RLIM_NLIMITS];

/* BEGIN TRASH */
	char	u_segflg;		/* 0:user D; 1:system; 2:user I */
	caddr_t	u_base;			/* base address for IO */
	unsigned int u_count;		/* bytes remaining for IO */
	off_t	u_offset;		/* offset in file for IO */

#ifdef __hp9000s800
	   /* The magic number, auxillary SOM header and spares */
	   struct{
	   	int	  u_magic;
	   	struct som_exec_auxhdr som_aux;
	   } u_exdata;
#endif /* __hp9000s800 */
#ifdef __hp9000s300
	union {
	   struct exec	Ux_A;
	   char ux_shell[SHSIZE];	/* #! and name of interpreter */
	} u_exdata;
#endif /* __hp9000s300 */
#ifdef __hp9000s800
	   int    u_spare[9];

#define	ux_mag		u_magic
#define	ux_tsize	som_aux.exec_tsize
#define	ux_dsize	som_aux.exec_dsize
#define	ux_bsize	som_aux.exec_bsize
#define	ux_entloc	som_aux.exec_entry
#define ux_tloc		som_aux.exec_tfile
#define ux_dloc		som_aux.exec_dfile
#define ux_tmem		som_aux.exec_tmem
#define ux_dmem         som_aux.exec_dmem
#define ux_flags	som_aux.exec_flags
#define Z_EXEC_FLAG	0x1
#endif /* __hp9000s800 */

#ifdef __hp9000s300
#define	ux_mag		Ux_A.a_magic.file_type
#define	ux_system_id	Ux_A.a_magic.system_id
#define ux_miscinfo     Ux_A.a_miscinfo
#define	ux_tsize	Ux_A.a_text
#define	ux_dsize	Ux_A.a_data
#define	ux_bsize	Ux_A.a_bss
#define	ux_entloc	Ux_A.a_entry
#endif	/* __hp9000s300 */

	caddr_t	u_dirp;			/* pathname pointer */
/* END TRASH */

	struct TrHeaderT *u_trptr;	/* QFS transaction header */
	int	u_lcount;		/* stack size of lock keys */
	int	u_ldebug;		/* for debug */
	int	u_lck_keys[LOCK_TRACK_MAX]; /* stack of lock keys */

	dev_t	u_devsused;		/* count of locked devices */
#ifdef __hp9000s800
 	u_int	u_spare3[8];		/* spares for backward compatibility */
	int u_sstep;			/* process single stepping flags */
#define	ULINK	0x01f			/* link register */
#define	USSTEP	0x020			/* process is single stepping */
#define	UPCQM	0x040			/* pc queue modified */
#define	UBL	0x080			/* branch and link at pcq head */
#define	UBE	0x100			/* branch external at pcq head */
	unsigned u_pcsq_head;		/* pc space and offset queue */
	unsigned u_pcoq_head;		/*   values for single stepping */
	unsigned u_pcsq_tail;
	unsigned u_pcoq_tail;
	unsigned u_ipsw;		/* ipsw for single stepping */
	int u_gr1;			/* value for general register 1 */
	int u_gr2;			/* value for general register 2 */
#endif /* __hp9000s800 */
	struct uprof {			/* profile arguments */
		short	*pr_base;	/* buffer base */
		unsigned pr_size;	/* buffer size */
		unsigned pr_off;	/* pc offset */
		unsigned pr_scale;	/* pc scaling */
	} u_prof;
#ifdef __hp9000s800
	u_int u_kpreemptcnt;		/* kernel preemption counter:  */
					/* read with GETKPREEMPTCNT()  */
					/* clear with CLRKPREEMPTCNT() */
					/* incremented in kpreempt()   */
#endif /* __hp9000s800 */
	dm_message u_request;		/* request message*/
	struct nsp *u_nsp;		/* nsp performing service*/
	site_t u_site;			/* site for which nsp executing */
	int	u_duxflags;		/* see defines below */
	char	**u_cntxp;		/* context pointer */
	struct locklist *u_prelock;	/* preallocated lock for lockadd() */

#ifdef	FSD_KI
	struct ki_timeval u_syscall_time; /* system call timestamp */
	dev_t	u_dev_t;		/* device location of this process */
	ino_t	u_inode;		/* inode number of this process */
	int	*ki_clk_tos_ptr;

#define KI_CLK_STACK_SIZE 20
	int ki_clk_stack[KI_CLK_STACK_SIZE];
#endif	/* FSD_KI */

        caddr_t u_vapor_mlist;          /* linked list of vapor_malloc mem */
	int u_ord_blk;			/* last ordered write block */
#ifdef	__hp9000s300
	struct	pcb	u_pcb;		/* should be last except u_stack */
#endif	/* __hp9000s300 */

	union {				/* double word aligned stack */
	 double	s_dummy;
	 int	s_stack[1];
	} u_s;				/* must be last thing in user_t */
#define	u_stack	u_s.s_stack
} user_t;

/*
 * These two defines are moved (logically) from param.h.  Need to have them
 * here to be able to get at sizeof(user_t)
 */
#ifdef __hp9000s800
#define	KSTACKBYTES	8192		/* size of kernel stack */
#define	UPAGES		btorp(sizeof(user_t) + KSTACKBYTES)
#endif

struct ucred {
#ifdef _CLASSIC_ID_TYPES
	unsigned short cr_filler_uid;
	unsigned short cr_uid;		/* effective user id */
#else
	uid_t cr_uid;			/* effective user id */
#endif
#ifdef _CLASSIC_ID_TYPES
	unsigned short cr_filler_gid;
	unsigned short cr_gid;		/* effective group id */
#else
	gid_t cr_gid;			/* effective group id */
#endif
#ifdef _CLASSIC_ID_TYPES
	int	  cr_groups[NGROUPS];	/* groups, 0 terminated */
#else
	gid_t     cr_groups[NGROUPS];	/* groups, 0 terminated */
#endif
#ifdef _CLASSIC_ID_TYPES
	unsigned short cr_filler_ruid;
	unsigned short cr_ruid;		/* real user id */
#else
	uid_t	cr_ruid;		/* real user id */
#endif
#ifdef _CLASSIC_ID_TYPES
	unsigned short cr_filler_rgid;
	unsigned short cr_rgid;		/* real group id */
#else
	gid_t	cr_rgid;		/* real group id */
#endif
	short	cr_ref;			/* reference count */
};

#ifdef _KERNEL
#define	crhold(cr)	{SPINLOCK(cred_lock);(cr)->cr_ref++;SPINUNLOCK(cred_lock);}
struct ucred *crget();
struct ucred *crcopy();
struct ucred *crdup();
#endif /* _KERNEL */


/* u_eosys values */
#define	EOSYS_NOTSYSCALL	0	/* not in kernel via syscall() */
#define	EOSYS_NORMAL		1	/* in syscall but nothing notable */
#define	EOSYS_INTERRUPTED	2	/* signal is not yet fully processed */
#define	EOSYS_RESTART    	3	/* user has requested restart */
#define	EOSYS_NORESTART    	4	/* user has requested error return */
#define	RESTARTSYS		EOSYS_INTERRUPTED /* temporary!!! */

/*
 * defines for u_duxflags
 */
#define DUX_UNSP	4	/* process is a user NSP */

/* u_error codes */
#ifdef _KERNEL_BUILD
#include "../h/errno.h"
#else  /* ! _KERNEL_BUILD */
#include <errno.h>		/* Traditional */
#endif /* _KERNEL_BUILD */

#if defined(__hp9000s800) && defined(_KERNEL)
/* WARNING: NEVER, NEVER, NEVER use u as a local variable
 * name or as a structure element in I/O system or elsewhere in the
 * kernel.
 */
#define u (*uptr)
#define udot (*uptr)
#endif /* __hp9000s800 && _KERNEL */

#ifdef _KERNEL
extern	struct user u;
#ifndef KMAP_CLEANUP
extern	struct user swaputl;
extern	struct user forkutl;
extern	struct user xswaputl;
extern	struct user xswap2utl;
extern	struct user pushutl;
extern	struct user vfutl;
#ifdef __hp9000s300
extern	struct user chklockutl;
#endif /* __hp9000s300 */
#endif /* not KMAP_CLEANUP */
#endif /* _KERNEL */

#endif /* ! _SYS_USER_INCLUDED */
