/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/lp.h,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:14:24 $
 */
/* @(#) $Revision: 1.5.84.3 $ */       
#ifndef _MACHINE_LP_INCLUDED /* allows multiple inclusion */
#define _MACHINE_LP_INCLUDED
#ifdef __hp9000s300

#ifdef _KERNEL_BUILD
#include "../h/buf.h"
#include "../wsio/iobuf.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/buf.h>
#include <sys/iobuf.h>
#endif /* _KERNEL_BUILD */

#define LPMAX 8

/* second commands */
#define		PR_SEC_DSJ 	SCG_BASE+16
#define		PR_SEC_DATA 	SCG_BASE+0
#define		PR_SEC_RSTA 	SCG_BASE+14
#define		PR_SEC_MASK 	SCG_BASE+01
#define		PR_SEC_STRD	SCG_BASE+10	/* 2608A */


/* misc. constants */
#define		PR_PSIZE	227	/* largest buffer 2631 */
#define 	PR_INBUF	1024	/* largest buffer from user request */
#define		prt_kind	b_s0 	/* scratch field in buf struct */
#define		prt_stat	b_s1	/* scratch field in buf struct */

/* output of DSJ operation 2631 */
#define		PR_RFDATA  	0x0000		
#define		PR_SDS	  	0x0001
#define		PR_RIOSTAT      0x0002	

/* output of DSJ operation 2608A */
#define		PR_ATTEN	0x0001
#define		PR_ATT_PAR	0x0003
#define		PR_PRINT	0x0040
#define		PR_PAPERF	0x0010
#define		PR_RIBBON	0x0002
#define		PR_SELF		0x0020


/* ppoll and SRQ mask bits */
#define		PR_M_PARITY	0x0008
#define		PR_M_RFD	0x0010
#define		PR_M_STATUS	0x0020
#define		PR_M_POWER	0x0040
#define		PR_M_PAPER	0x0080


/* masks for io result byte in case of 2631 */
#define 	PR_I_POWER	0x0001	
#define		PR_I_PAPER	0x0002	
#define		PR_I_PARITY	0x0008	
#define		PR_I_RFD	0x0040	
#define		PR_I_ONLINE	0x0080	

/* masks for io result byte in case of 2225 */
#define		PR_I_CMD	0x0001
#define		PR_I_BUFE	0x0004
#define		PR_I_BUFF	0x0008
#define		PR_I_OOP	0x0020

/* masks for io result byte in case of 2608 */
#define 	PR_I_POW	0x0001	
#define		PR_I_OPSTAT	0x0040	
#define		PR_I_LINE	0x0080	

/* printer kinds */
#define		P_DUMB		1
#define		P_2608		2
#define		P_2673		3
#define		P_2631		4
#define		P_2225		5
#define		P_2227		6
#define		P_2932		7
#define		P_3630		8
#define		P_1602		9

#define	SHORTTIME	HZ	/* for status: should be quick */
#define LONGTIME	HZ*25	/* for ppoll, etc. could be long */
#define	XFRTIME		HZ*15	/* transfer: must be less than LONGTIME */


/* storage local to driver */
struct PR_bufs {
	struct buf	*r_getebuf;	/* pointer for brelse */
	struct buf	r_bpbuf;	/* buf for io request */
	struct iobuf    r_iobuf;	/* iobuf for hpib stuff */
	char		r_inbuf[PR_INBUF]; /* raw input buffer from user land */
	char            r_outbuf[PR_PSIZE+3]; /* output from canon routine */
};
#endif /* __hp9000s300 */
#endif /* _MACHINE_LP_INCLUDED */
