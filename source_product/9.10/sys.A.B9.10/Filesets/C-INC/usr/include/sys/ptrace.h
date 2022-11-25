/* $Header: ptrace.h,v 1.16.83.4 93/09/17 18:32:46 kcs Exp $ */       

#ifndef _SYS_PTRACE_INCLUDED
#define _SYS_PTRACE_INCLUDED

/* ptrace.h: definitions for debugging and tracing other processes */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

/* Types needed */

#ifndef _PID_T
#  define _PID_T
   typedef long pid_t;		/* same as in <sys/types.h> */
#endif /* _PID_T */


/* Function prototype */

#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef _PROTOTYPES
   extern int ptrace(int, pid_t, int, int, int);
#else /* not _PROTOTYPES */
   extern int ptrace();
#endif /* not _PROTOTYPES */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */



/*
 * ptrace 'request' arguments
 */
#define	PT_SETTRC	0	/* Set Trace */
#define	PT_RIUSER	1	/* Read User I Space */
#define	PT_RDUSER	2	/* Read User D Space */
#define	PT_RUAREA	3	/* Read User Area */
#define	PT_WIUSER	4	/* Write User I Space */
#define	PT_WDUSER	5	/* Write User D Space */
#define	PT_WUAREA	6	/* Write User Area */
#define	PT_CONTIN	7	/* Continue */
#define	PT_EXIT		8	/* Exit */
#define	PT_SINGLE	9	/* Single Step */
#ifdef  __hp9000s800
#define	PT_RUREGS	10	/* Read User Registers */
#define	PT_WUREGS	11	/* Write User Registers */
#endif  /* __hp9000s800 */
#define	PT_ATTACH	12	/* Attach To Process */
#define	PT_DETACH	13	/* Detach From Attached Process */
#ifdef  __hp9000s800
#define	PT_RDTEXT	14	/* Read User I Space (multiple words) */
#define	PT_RDDATA	15	/* Read User D Space (multiple words) */
#define	PT_WRTEXT	16	/* Write User I Space (multiple words) */
#define	PT_WRDATA	17	/* Write User D Space (multiple words) */
#endif  /* __hp9000s800 */
#ifdef	__hp9000s300
#define PT_RFPREGS      50      /* Read  FPA registers */
#define PT_WFPREGS      51      /* Write FPA registers */
#endif  /* __hp9000s300 */

#ifdef	_KERNEL

#ifndef _CADDR_T
#  define _CADDR_T
   typedef char *caddr_t;	/* same as in ../h/types.h */
#endif /* _CADDR_T */

#ifdef __hp9000s800
/*
 * Save State References
 */
#define	pcsq	pcsqh
#define	pcoq	pcoqh
#define	pcsqh	ssp->ss_pcsq_head
#define	pcoqh	ssp->ss_pcoq_head
#define	pcsqt	ssp->ss_pcsq_tail
#define	pcoqt	ssp->ss_pcoq_tail
#define	flags	ssp->ss_flags
#define	iir	ssp->ss_iir
#define	isr	ssp->ss_isr
#define	ior	ssp->ss_ior
#define	ipsw	ssp->ss_ipsw


/*
 * Advance Save State PC Queue
 */
#define	advancepcq() \
	advancepcqh(); \
	advancepcqt()

#define	advancepcqh() \
	ssp->ss_pcsq_head = ssp->ss_pcsq_tail; \
	ssp->ss_pcoq_head = ssp->ss_pcoq_tail

#define	advancepcqt() \
	ssp->ss_pcoq_tail = ssp->ss_pcoq_tail+4


/*
 * Load Save State PC Queue from User Area
 */
#define	loadupcq() \
	loadupcqh(); \
	loadupcqt()

#define	loadupcqh() \
	ssp->ss_pcsq_head = u.u_pcsq_head; \
	ssp->ss_pcoq_head = u.u_pcoq_head

#define	loadupcqt() \
	ssp->ss_pcsq_tail = u.u_pcsq_tail; \
	ssp->ss_pcoq_tail = u.u_pcoq_tail


/*
 * Store Save State PC Queue into User Area
 */
#define	storeupcq() \
	storeupcqh(); \
	storeupcqt()

#define	storeupcqh() \
	u.u_pcsq_head = ssp->ss_pcsq_head; \
	u.u_pcoq_head = ssp->ss_pcoq_head

#define	storeupcqt() \
	u.u_pcsq_tail = ssp->ss_pcsq_tail; \
	u.u_pcoq_tail = ssp->ss_pcoq_tail


/*
 * Load Save State PC Queue With Advance of User Area
 */
#define	advanceupcq() \
	advanceupcqh(); \
	advanceupcqt()

#define	advanceupcqh() \
	ssp->ss_pcsq_head = u.u_pcsq_tail; \
	ssp->ss_pcoq_head = u.u_pcoq_tail

#define	advanceupcqt() \
	ssp->ss_pcsq_tail = u.u_pcsq_tail; \
	ssp->ss_pcoq_tail = u.u_pcoq_tail+4
#endif /* __hp9000s800 */


/*
 * Tracing structure.
 * Used to pass trace command from
 * parent to child being traced.
 * This data base cannot be
 * shared and is locked
 * per user.
 */
#ifdef __hp9000s800
#  define IP_DATA_SIZE	128
#endif /* __hp9000s800 */
struct ipc {
	int	ip_lock;
	int	ip_req;
#ifdef __hp9000s800
	caddr_t	ip_addr;
	int	ip_data[IP_DATA_SIZE];
	caddr_t	ip_addr2;
	int	ip_len;
#endif /* __hp9000s800 */
#ifdef __hp9000s300
	int	*ip_addr;
	int	ip_data;
#endif /* __hp9000s300 */
};

#ifdef __hp9000s800
extern
#endif
       struct ipc ipc;

#define	DOEXIT		-1
#define	DODETACH	-2

#ifdef __hp9000s800
   /*
    * HP PA Branch Instruction
    */
  struct branch_inst {
	unsigned bri_opcode:6;
	unsigned bri_link:5;
	unsigned :5;
	unsigned bri_extension:3;
  };

#  define	briopcode(x)	(((struct branch_inst *)&(x))->bri_opcode)
#  define	brilink(x)	(((struct branch_inst *)&(x))->bri_link)
#  define	briextension(x)	(((struct branch_inst *)&(x))->bri_extension)
#  define	BRI_BE		0x38
#  define	BRI_BLE		0x39
#  define	BRI_Branch	0x3a
#  define	BRI_BL		0x00
#  define	BRI_BLR		0x02


   extern caddr_t user_sstep, user_nullify;
   extern sstepsysret();
   extern sstepsigsysret(), sstepsigtrapret(), sstepsendsig();
#endif /* __hp9000s800 */
#endif /* _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_PTRACE_INCLUDED */
