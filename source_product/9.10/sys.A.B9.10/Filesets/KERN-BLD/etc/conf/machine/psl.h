/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/psl.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:03:40 $
 */
/* @(#) $Revision: 1.3.84.3 $ */      
#ifndef _MACHINE_PSL_INCLUDED /* allow multiple inclusions */
#define _MACHINE_PSL_INCLUDED

/*
 *  	68K processor status register
 */

#define	PS_C	0x1		/* carry bit */
#define	PS_V	0x2		/* overflow bit */
#define	PS_Z	0x4		/* zero bit */
#define	PS_N	0x8		/* negative bit */
#define	PS_E	0x10		/* extend bit */
#define PS_S	0x2000		/* supervisor state bit */
#define PS_T	0x8000		/* trace mode bit */
#define	PS_IPL	0x0700  	/* interrupt priority level */

/*
 * VAX-equivalent names - make all PS_* = PSL_*
 */

#define	PSL_C		PS_C
#define	PSL_V		PS_V
#define	PSL_Z		PS_Z
#define	PSL_N		PS_N
#define	PSL_ALLCC	0x0000000f	/* all cc bits - unlikely */
#define	PSL_E		PS_E
#define	PSL_S		PS_S
#define	PSL_T		PS_T
#define	PSL_IPL		PS_IPL

#define	PS_MBZ		0x58e0		/* must be zero bits */
#define	PSL_MBZ		PS_MBZ

#define	PS_USERSET	0
#define	PS_USERCLR	(PS_S|PS_IPL|PS_MBZ)

#define	PSL_USERSET	PS_USERSET
#define	PSL_USERCLR	PS_USERCLR


#endif /* _MACHINE_PSL_INCLUDED */
