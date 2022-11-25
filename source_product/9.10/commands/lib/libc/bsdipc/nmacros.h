/*
 * @(#)nmacros.h: $Revision: 35.1 $ $Date: 88/02/10 14:06:18 $
 * $Locker:  $
 * 
 */

/******************************************************************************
 ******************************************************************************
 **                             NMACROS.H
 **
 **  AUTHOR:            Carl Dierschow, David Hendricks, Dave Barrett
 **
 **  DATE BEGUN:        23 Feb 84
 **
 **  MODULE:            arch
 **  PROJECT:           leaf_project
 **  REVISION:          2.3
 **  SCCS NAME:         /users/fh/leaf/sccs/arch/header/s.nmacros.h
 **  LAST DELTA DATE:   85/06/19
 **
 **  DESCRIPTION:       Some useful macro definitions and QA aids.
 **                     
 ******************************************************************************
 *****************************************************************************/

/* ASSERT is a QA aid for testing the validity of expressions.     */
#ifdef QA

#ifdef ASSERTION_CHECK

#ifdef CONFIG_HP200
#define ASSERT(CONDITION,ERRNO) WARNING( !(CONDITION), ERRNO, NIL, 0, 0)
#else
#define ASSERT(CONDITION,ERRNO) if (!(CONDITION)) bomb(ERRNO)
#endif

#else
#define ASSERT(CONDITION,ERRNO) {}
#endif

#else /* ifndef QA */
#define ASSERT(CONDITION,ERRNO) {}
#endif
#define COPY(SRCP,DESTP,LEN) {char *sp, *dp, *zp; zp=srcp+len; sp=srcp; \
	dp=destp; while(sp<zp) {*dp++= *sp++;} }

#define TO_UPPER(c) ( (c) >= 'a' && (c) <= 'z' ? (c) & 0x5F : (c) ) 

#ifdef CONFIG_VAX
#define __LINE__        0
#define __FILE__        NIL
#endif

/*
 * The following macros were added  for machine
 * independence of packet overlays of multiple byte quantities.
 *
 * dave barrett 06/11/84
 */

/*   
 *         int:                          long:
 *  MSB           LSB     MSB                             LSB
 * +--------+--------+   +--------+--------+--------+--------+
 * | byte 1 | byte 0 |   | byte 3 | byte 2 | byte 1 | byte 0 |
 * +--------+--------+   +--------+--------+--------+--------+
 *
 *        +--------+            +--------+
 * addr+0 | byte 1 |     addr+0 | byte 3 |
 *        +--------+            +--------+
 * addr+1 | byte 0 |     addr+1 | byte 2 |
 *        +--------+            +--------+
 *                       addr+2 | byte 1 |
 *                              +--------+
 *                       addr+3 | byte 0 |
 *                              +--------+
 *
 * NOTE: For the series 200 these macros are needed for accessing multiple
 *       byte quantities on odd addresses. For the hp150 these make the
 *       conversion to integers and longs correctly.  Note that the left
 *       and right shift operators take cars of the byte swapping issue 
 *       nicely. (Thanks to Jeff Wu for pointing this out to me). D.B.
 */
typedef unsword  packet_unsword;
typedef unsdword packet_unsdword;

#define TO_UNSWORD(d,s) ((d)=(unsword)(*(unsbyte *)(s)<<8 | *((unsbyte *)(s)+1)))

#define TO_PACKET_UNSWORD(d,s) (*(unsbyte *)(d)=(unsword)(s)>>8, \
*((unsbyte *)(d)+1)=(unsword)(s) & 0xff)

#define TO_UNSDWORD(d,s) ((d)=(unsdword)((unsdword) *(unsbyte *)(s)<<24 | \
(unsdword) *((unsbyte *)(s)+1)<<16 | *((unsbyte *)(s)+2)<<8 | *((unsbyte *)(s)+3)))

#define TO_PACKET_UNSDWORD(d, s) (*(unsbyte *)(d)=(unsdword)(s)>>24, \
*((unsbyte *)(d)+1)=(unsdword)(s)>>16 & 0xffl, \
*((unsbyte *)(d)+2)=(unsdword)(s)>>8 & 0xffl, \
*((unsbyte *)(d)+3)=(unsdword)(s) & 0xffl)

/* 
 *  These macros provide an inline version of the protect and unprotect
 *  functions found in prot.c
 *  WARNING: THESE MACROS WILL NOT PORT TO THE VAX IMPLEMENTATION; PROTECT
 *           AND UNPROTECT ARE BOTH MUCH MORE COMPLEX ON THE VAX.!!!!!!
 */

#define PROTECT(tokenp) { ASSERT(lock_level >= 0, E_BAD_LOCKLEVEL); \
	ASSERT(lock_level < 127, E_BAD_LOCKLEVEL); \
	ASSERT(find_prev_prot_token(tokenp) != NIL, E_BAD_PROTECT_TOKEN); \
	ASSERT((tokenp)->locked == FALSE, E_BAD_LOCKLEVEL); \
	lock_level++; \
	}

#define UNPROTECT(tokenp) { ASSERT(lock_level > 0, E_BAD_LOCKLEVEL); \
	ASSERT(find_prev_prot_token(tokenp) != NIL, E_BAD_PROTECT_TOKEN); \
	ASSERT((tokenp)->locked == TRUE, E_BAD_LOCKLEVEL); \
	if ((--lock_level) == 0) { if (timer_needs_service) timer_task(); \
			       if (receive_needs_service) receive_task(); } \
	}

/*------------------------- end of NMACROS.H ---------------------------------*/
