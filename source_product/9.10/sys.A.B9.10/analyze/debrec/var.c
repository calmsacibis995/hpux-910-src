/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/debrec/RCS/var.c,v $
 * $Revision: 1.3.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:24:13 $
 */

/*
 * Original version based on: 
 * Revision 63.4  88/05/27  15:34:36  15:34:36  sjl (Steve Lilker)
 */

/*
 * Copyright Third Eye Software, 1983.
 * Copyright Hewlett Packard Company 1985.
 *
 * This module is part of the CDB/XDB symbolic debugger.  It is available
 * to Hewlett-Packard Company under an explicit source and binary license
 * agreement.  DO NOT COPY IT WITHOUT PERMISSION FROM AN APPROPRIATE HP
 * SOURCE ADMINISTRATOR.
 */

/*
 * These routines find addresses and sizes of various kinds of variables,
 * scan, display, and otherwise manage stack frames, list registers and
 * special variables, etc.
 */

#include "cdb.h"

#ifdef SPECTRUM
#include <sys/signal.h>
#ifdef DEBREC
#include <machine/save_state.h>
#endif

FLAGT intrpt_marker = 0;
#endif

long	vResult;		/* place to store function returns */

#ifndef XDB
extern  int	vcLinesMax;
#endif

#ifdef S200
#define cbProcMax cbTokMax
#endif

#ifdef DEBREC
extern int xdbtraceabort;
#endif

/***********************************************************************
 * PREDEFINED SPECIAL VARIABLES:
 *
 * These include accesses to debugger internal values, user registers,
 * and user procedure return values.  They are initialized here to
 * appropriate types and locations.  In general they're marked NOT
 * constant, e.g. the value is the address, and users can write them.
 * For registers, the address is the register index used with GetReg().
 */

#define SbCast(x)  ((int) x)	/* (char*) pretends to be int */

#ifndef M68000


static TYR	vrgTySpc[] = {

    /* name,		st,    const, unused, bt, desc (tq),  value */

    {SbCast ("$lang"),	stSpc, false, 0, btUShort,0,0,0,0,0,0, (ulong)&vlc,
		false, 0},
    {SbCast ("$line"),	stSpc, false, 0, btInt,  0,0,0,0,0,0, (ulong)&viln,
		false, 0},
    {SbCast ("$signal"),stSpc, false, 0, btUInt, 0,0,0,0,0,0, (ulong)&vsig,
		false, 0},
    {SbCast ("$malloc"),stSpc, false, 0, btInt,	 0,0,0,0,0,0,(ulong)&vcbMalloc,
		false, 0},
#ifdef XDB
    {SbCast ("$step"),	stSpc, false, 0, btInt,	 0,0,0,0,0,0, (ulong)&vcBadMax,
		false, 0},
#else  /* CDB */
    {SbCast ("$cBad"),	stSpc, false, 0, btInt,	 0,0,0,0,0,0, (ulong)&vcBadMax,
		false, 0},
    {SbCast ("$pagelines"),stSpc, false, 0,btInt,0,0,0,0,0,0,(ulong)&vcLinesMax,
                false, 21},
#endif /* CDB */
    {SbCast ("$pc"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, upc,
		false, 0},
#if (!SPECTRUM)
    {SbCast ("$fp"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, ufp,
		false, 0},
#endif
    {SbCast ("$sp"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, usp,
		false, 0},

#ifdef SPECTRUM
    {SbCast ("$r0"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u0,
		false, 0},
    {SbCast ("$r1"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u1,
		false, 0},
    {SbCast ("$r2"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u2,
		false, 0},
    {SbCast ("$r3"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u3,
		false, 0},
    {SbCast ("$r4"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u4,
		false, 0},
    {SbCast ("$r5"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u5,
		false, 0},
    {SbCast ("$r6"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u6,
		false, 0},
    {SbCast ("$r7"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u7,
		false, 0},
    {SbCast ("$r8"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u8,
		false, 0},
    {SbCast ("$r9"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u9,
		false, 0},
    {SbCast ("$r10"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u10,
		false, 0},
    {SbCast ("$r11"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u11,
		false, 0},
    {SbCast ("$r12"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u12,
		false, 0},
    {SbCast ("$r13"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u13,
		false, 0},
    {SbCast ("$r14"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u14,
		false, 0},
    {SbCast ("$r15"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u15,
		false, 0},
    {SbCast ("$r16"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u16,
		false, 0},
    {SbCast ("$r17"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u17,
		false, 0},
    {SbCast ("$r18"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u18,
		false, 0},
    {SbCast ("$r19"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u19,
		false, 0},
    {SbCast ("$r20"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u20,
		false, 0},
    {SbCast ("$r21"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u21,
		false, 0},
    {SbCast ("$r22"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u22,
		false, 0},
    {SbCast ("$r23"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u23,
		false, 0},
    {SbCast ("$r24"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u24,
		false, 0},
    {SbCast ("$r25"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u25,
		false, 0},
    {SbCast ("$r26"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u26,
		false, 0},
    {SbCast ("$r27"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u27,
		false, 0},
    {SbCast ("$r28"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u28,
		false, 0},
    {SbCast ("$r29"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u29,
		false, 0},
    {SbCast ("$r30"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u30,
		false, 0},
    {SbCast ("$r31"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u31,
		false, 0},
    {SbCast ("$dp"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, udp,
		false, 0},
    {SbCast ("$arg0"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, uarg0,
		false, 0},
    {SbCast ("$arg1"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, uarg1,
		false, 0},
    {SbCast ("$arg2"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, uarg2,
		false, 0},
    {SbCast ("$arg3"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, uarg3,
		false, 0},
   {SbCast ("$result"), stSpc, false, 0, btInt,	 0,0,0,0,0,0, (ulong) &vResult,
		false, 0},
    {SbCast ("$ret0"),stReg, false, 0, btInt,	 0,0,0,0,0,0, u28,
		false, 0},
    {SbCast ("$ret1"),stReg, false, 0, btInt,	 0,0,0,0,0,0, u29,
		false, 0},
    {SbCast ("$short"), stSpc, false, 0, btShort,0,0,0,0,0,0, (ulong) &vResult,
		false, 0},
    {SbCast ("$long"),	stSpc, false, 0, btLong, 0,0,0,0,0,0, (ulong) &vResult,
		false, 0},
    {SbCast ("$f0"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, u0,
		false, 0},
    {SbCast ("$f1"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, u1,
		false, 0},
    {SbCast ("$f2"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, u2,
		false, 0},
    {SbCast ("$f3"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp3,
		false, 0},
    {SbCast ("$f4"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp4,
		false, 0},
    {SbCast ("$f5"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp5,
		false, 0},
    {SbCast ("$f6"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp6,
		false, 0},
    {SbCast ("$f7"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp7,
		false, 0},
    {SbCast ("$f8"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp8,
		false, 0},
    {SbCast ("$f9"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp9,
		false, 0},
    {SbCast ("$f10"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp10,
		false, 0},
    {SbCast ("$f11"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp11,
		false, 0},
    {SbCast ("$f12"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp12,
		false, 0},
    {SbCast ("$f13"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp13,
		false, 0},
    {SbCast ("$f14"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp14,
		false, 0},
    {SbCast ("$f15"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp15,
		false, 0},
    {SbCast ("$f16"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp16,
		false, 0},
    {SbCast ("$f17"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp17,
		false, 0},
    {SbCast ("$f18"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp18,
		false, 0},
    {SbCast ("$f19"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp19,
		false, 0},
    {SbCast ("$f20"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp20,
		false, 0},
    {SbCast ("$f21"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp21,
		false, 0},
    {SbCast ("$f22"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp22,
		false, 0},
    {SbCast ("$f23"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp23,
		false, 0},
    {SbCast ("$f24"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp24,
		false, 0},
    {SbCast ("$f25"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp25,
		false, 0},
    {SbCast ("$f26"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp26,
		false, 0},
    {SbCast ("$f27"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp27,
		false, 0},
    {SbCast ("$f28"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp28,
		false, 0},
    {SbCast ("$f29"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp29,
		false, 0},
    {SbCast ("$f30"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp30,
		false, 0},
    {SbCast ("$f31"),	stReg, false, 0, btFloat,	 0,0,0,0,0,0, ufp31,
		false, 0},
#else

#ifdef FOCUS
   {SbCast ("$result"), stSpc, false, 0, btInt,	 0,0,0,0,0,0, (ulong) &vResult,
		false, 0},
   {SbCast ("$short"),	stSpc, false, 0, btShort,0,0,0,0,0,0,
		(ulong) (((char *) &vResult) + 2), false, 0},
   {SbCast ("$long"),	stSpc, false, 0, btLong, 0,0,0,0,0,0, (ulong) &vResult,
		false, 0},

#else /* not FOCUS */

    {SbCast ("$r0"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u0},
    {SbCast ("$r1"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u1},
    {SbCast ("$r2"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u2},
    {SbCast ("$r3"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u3},
    {SbCast ("$r4"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u4},
    {SbCast ("$r5"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u5},
    {SbCast ("$r6"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u6},
    {SbCast ("$r7"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u7},
    {SbCast ("$r8"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u8},
    {SbCast ("$r9"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u9},
    {SbCast ("$r10"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u10},
    {SbCast ("$r11"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u11},
    {SbCast ("$r12"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u12},
    {SbCast ("$r13"),	stReg, false, 0, btInt,	 0,0,0,0,0,0, u13},

#endif	/* not FOCUS */

#endif	/* not SPECTRUM */

};

#else /* M68000 */

static TYR	vrgTySpc[] = {

    /* name,		st,    const, unused, desc (tq), bt,  value */
    /*	adrisval, dummy */

    {SbCast ("$lang"),	stSpc, false, 0,0,0,0,0,0, 0, btUShort,(ulong)&vlc,
	false, 0},
    {SbCast ("$line"),	stSpc, false, 0,0,0,0,0,0, 0, btInt,  (ulong)&viln,
	false, 0},
    {SbCast ("$signal"),stSpc, false, 0,0,0,0,0,0, 0, btUInt, (ulong)&vsig,
	false, 0},
    {SbCast ("$malloc"),stSpc, false, 0,0,0,0,0,0, 0, btInt, (ulong)&vcbMalloc,
	false, 0},
#ifdef XDB
    {SbCast ("$step"),	stSpc, false, 0, btInt,	 0,0,0,0,0,0, (ulong)&vcBadMax,
		false, 0},
#else  /* CDB */
    {SbCast ("$cBad"),	stSpc, false, 0, btInt,	 0,0,0,0,0,0, (ulong)&vcBadMax,
		false, 0},
    {SbCast ("$pagelines"),stSpc, false, 0,btInt,0,0,0,0,0,0,(ulong)&vcLinesMax,
                false, 21},
#endif /* CDB */
    {SbCast ("$fpa"),stSpc,false,0,0,0,0,0,0, 0, btInt, (ulong)&viFpa_flag,
        false, 0},
    {SbCast ("$fpa_reg"),stSpc, false, 0,0,0,0,0,0, 0, btInt,(ulong)&viFpa_reg,
        false, 0},
    {SbCast ("$result"),stSpc, false, 0,0,0,0,0,0, 0, btInt, (ulong) &vResult,
	false, 0},
    {SbCast ("$short"),	stSpc, false, 0,0,0,0,0,0, 0, btShort, (ulong)
        (((char *) &vResult) + 2), false, 0},
    {SbCast ("$long"),	stSpc, false, 0,0,0,0,0,0, 0, btLong, (ulong) &vResult,
	false, 0},

    {SbCast ("$pc"),	stReg, false, 0,0,0,0,0,0, 0, btLong, upc,
	false, 0},
    {SbCast ("$fp"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ufp,
	false, 0},
    {SbCast ("$sp"),	stReg, false, 0,0,0,0,0,0, 0, btLong, usp,
	false, 0},
    {SbCast ("$ps"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ups,
	false, 0},

    {SbCast ("$a0"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ua0,
	false, 0},
    {SbCast ("$a1"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ua1,
	false, 0},
    {SbCast ("$a2"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ua2,
	false, 0},
    {SbCast ("$a3"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ua3,
	false, 0},
    {SbCast ("$a4"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ua4,
	false, 0},
    {SbCast ("$a5"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ua5,
	false, 0},
    {SbCast ("$a6"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ua6,
	false, 0},
    {SbCast ("$a7"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ua7,
	false, 0},

    {SbCast ("$d0"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ud0,
	false, 0},
    {SbCast ("$d1"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ud1,
	false, 0},
    {SbCast ("$d2"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ud2,
	false, 0},
    {SbCast ("$d3"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ud3,
	false, 0},
    {SbCast ("$d4"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ud4,
	false, 0},
    {SbCast ("$d5"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ud5,
	false, 0},
    {SbCast ("$d6"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ud6,
	false, 0},
    {SbCast ("$d7"),	stReg, false, 0,0,0,0,0,0, 0, btLong, ud7,
	false, 0},

/* Removed for Earlybird release -- not stored along with other registers;
   will need to be re-worked.
    {SbCast ("$f0"),	stReg, true,  0, 0,0,0,0,0,0, btLong, uf0,
	false, 0},
    {SbCast ("$f1"),	stReg, true,  0, 0,0,0,0,0,0, btLong, uf1,
	false, 0},
    {SbCast ("$f2"),	stReg, true,  0, 0,0,0,0,0,0, btLong, uf2,
	false, 0},
    {SbCast ("$f3"),	stReg, true,  0, 0,0,0,0,0,0, btLong, uf3,
	false, 0},
    {SbCast ("$f4"),	stReg, true,  0, 0,0,0,0,0,0, btLong, uf4,
	false, 0},
    {SbCast ("$f5"),	stReg, true,  0, 0,0,0,0,0,0, btLong, uf5,
	false, 0},
    {SbCast ("$f6"),	stReg, true,  0, 0,0,0,0,0,0, btLong, uf6,
	false, 0},
    {SbCast ("$f7"),	stReg, true,  0, 0,0,0,0,0,0, btLong, uf7,
	false, 0},
    {SbCast ("$fstatus"),stReg,true,  0, 0,0,0,0,0,0, btLong, ufs,
	false, 0},
    {SbCast ("$ferrbit"),stReg,true,  0, 0,0,0,0,0,0, btLong, ufe,
	false, 0},
*/
};

#endif /* M68000 */

#define	vispcMac (((long)(sizeof (vrgTySpc))) / cbTYR) /* total number */


/***********************************************************************
 * USER-DEFINED SPECIAL VARIABLES ("local vars"):
 *
 * They are set up as all tyLong, and their values are saved in the
 * value fields directly.
 */

static int	vilvMac;		/* current number */
static int	vilvMax;		/* current limit  */
static int	vilvDlta;		/* expansion increment */
static pTYR	vrgTyLv;		/* array of them  */


/*
 * These values refer to the LST (linker symbol table, not to be confused
 * with the SLT), since the "sym" values relate to the DNTT, and the NP
 * (name pool, not to be confused with the VT), since "ss" values refer to
 * the VT.  LST and NP values are set in InitSymfile() and used only in
 * SbFInp(), AdrFLabel(), and LabelFAdr().
 */

long	vcbLstFirst;		/* start of LST */
long	vilstMax;		/* size	 of LST */

#if (FOCUS || SPECTRUM)
long	vcbNpFirst;		/* start of NP	*/
long	vinpMax;		/* size	 of NP	*/

#ifdef SPECTRUM
LSTR  *vrglst;
char  *vrgnp;
#endif

#endif  /* FOCUS or SPECTRUM*/

#ifdef S200
static ADRT AdrFLSTAdr();
#endif

/***********************************************************************
 * I N I T   S P C
 *
 * Set up the debugger special variables.  It allocates memory and sets
 * default type information for user-defined variables (lv's), and sets
 * bitHighs in names of predefined variables (for the string space vs.
 * pointer hack).
 */

void InitSpc()
{
    TYR		  aty;			/* instance of a type */
    register long i;			/* which special var  */

#ifdef INSTR
    vfStatProc[394]++;
#endif

    aty = *vtyLong;			/* set to non-const long */
    aty.td.st = stSpc;			/* all are of type stSpc */

    for (i = 0; i < vispcMac; i++)
	vrgTySpc[i].sbVar |= bitHigh;	/* bitHigh is zero for some systems */

    vilvMax = 26;
    vilvDlta = 8;
    vrgTyLv = (pTYR) Malloc ((vilvMax + 1) * cbTYR, true); /* get memory */

    for (i = 0; i < vilvMax; i++)	/* set them all to defaults */
	vrgTyLv[i] = aty;
    return;

} /* InitSpc */


/***********************************************************************
 * A D R   F   S P E C I A L
 *
 * Return the address and type of a special variable from its name,
 * checking predefined variables first.	 An EXACT MATCH is required (no
 * substrings allowed).  If the variable is not found, add it to the
 * user-defined list if room permits, else UError() out.
 *
 * This is the only AdrF routine which returns void and returns the adr
 * through a parameter.
 */

void AdrFSpecial (sbSpc, rgTy, padr)
    register char *sbSpc;		/* name req'd, if any	*/
    pTYR	   rgTy;		/* type info to return	*/
    pADRT	   padr;		/* address to return	*/
{
    register long  i;			/* index */

#ifdef INSTR
    vfStatProc[395]++;
#endif

    for (i = 0; i < vispcMac; i++)		/* predefined specials */
    {
	if (FSbCmp (SbInCore (vrgTySpc[i].sbVar), sbSpc))	/* found it */
	{
	    *rgTy = vrgTySpc[i];
	    *padr = vrgTySpc[i].valTy;
	    return;
	}
    }

    for (i = 0; i < vilvMac; i++)		/* user defined specials */
    {
	if (FSbCmp (SbInCore (vrgTyLv[i].sbVar), sbSpc))
	{
	    *rgTy = vrgTyLv[i];
	    *padr = (ADRT) & (vrgTyLv[i].valTy);
	    return;
	}
    }

/*
 * DEFINE NEW VARIABLE:
 */

    if (vilvMac >= vilvMax)		/* table full -- expand it	*/
    {
	int	i = vilvMax;
	TYR	aty;			/* instance of a type */
	vilvMax += vilvDlta;
        vrgTyLv = (pTYR) Realloc (vrgTyLv, (vilvMax+1) * cbTYR);

	aty = *vtyLong;			/* set to non-const long */
	aty.td.st = stSpc;		/* all are of type stSpc */

        for ( ; i < vilvMax; i++)	/* set excess to defaults */
	    vrgTyLv[i] = aty;
    }

    i = vilvMac++;

    vrgTyLv[i].sbVar = (int) Malloc (strlen (sbSpc) + 2, true);
    strcpy (vrgTyLv[i].sbVar, sbSpc);
    vrgTyLv[i].sbVar |= bitHigh;			/* see sym.h */

    *rgTy = vrgTyLv[i];
    *padr = (ADRT) & (vrgTyLv[i].valTy);

} /* AdrFSpecial */


/***********************************************************************
 * A D R   F   R E G
 *
 * Given a desired fp and the register, walk back through the stack,
 * looking for frames that may have saved this register.  If we find one,
 * figure out the memory address of the saved register and continue.
 * Only when we reach the desired frame, after (perhaps) seeing this
 * register get saved N times, do we actually know for sure where it is!
 */

ADRT AdrFReg (reg, fpIn, ty)
    ADRT	reg;	/* register to find	*/
    ADRT	fpIn;	/* stop at frame ptr	*/
    pTYR	ty;	/* type to return	*/
{
#if (FOCUS || SPECTRUM)
    return (adrNil);			/* no saved registers on stack */
#else

#ifdef S200
#define maskReg 0x1
#else
#define maskReg 0x10000
#endif
    register int	maskSave;		/* save mask for frame	*/
    int		maskTest = maskReg << reg;	/* target frame mask	*/
    ADRT	adrRet	 = reg;			/* def: still in reg	*/
    ADRT	pc = vpc;			/* prog cntr (for call) */
    ADRT	fp;				/* current frame ptr	*/
    ADRT	ap;				/* arg ptr (for call)	*/
    register int	i;			/* index		*/
#ifdef S200
    ADRT	adrStart;			/* start address of proc */
    long	saveInst;			/* reg save instruction	*/
    long	saveDisp;			/* disp of reg save instr */
#ifdef S200BSD
    long	linkInst;			/* "link" instruction */
    long	movemOff;			/* offset of "movem" instr */
    long	dispInst;			/* instruction containing */
						/* register set offset */
    unsigned short linkOp;			/* "link" op code */
    unsigned short saveOp;			/* "save" op code */
#endif
#endif

#ifdef INSTR
    vfStatProc[396]++;
#endif

    for (fp = vfp; fp != fpIn; NextFrame (&fp, &ap, &pc))
    {
	if (fp == 0)
	    Panic ((nl_msg(204, "Invalid stack?")));

#ifdef S200
	adrStart = AdrFLSTAdr (pc);		/* map pc -> start adr	*/
#ifdef S200BSD

	/*
	 * There are several prologues we must understand:
	 *
	 *CASE 1:		(alternate instructions)
	 *  mov.l  %a6,-(%sp)	   pea    (%a6)
	 *  mov.l  %sp,%a6
	 *  adda.l &LF,%sp	   suba.l &-LF,%sp
	 *  movem.l &LS,(%sp)	   mov.l  %reg,(%sp)	    nothing
	 *
	 *CASE 2:
	 *  link.w %a6,&LF
	 *  movem.l &LS,(%sp)	   mov.l  %reg,(%sp)	    nothing
	 *
	 *CASE 3:
	 *  link.l %a6,&LF
	 *  movem.l &LS,LF(%a6)	   mov.l  %reg,LF(%a6)      nothing
	 *                         movem.l &LS,(%sp)
	 */

	linkInst = GetWord (adrStart, spaceText);
	linkOp = (linkInst & 0xffff0000) >> 16;
	if (linkOp == 0x480e)			/* link.l */
	{
	    saveDisp = GetWord (adrStart + 2, spaceText);
	    movemOff = 6;
	}
	else if (linkOp == 0x4e56)		/* link.w */
	{
	    saveDisp = GetWord (adrStart + 2, spaceText) >> 16;
	    movemOff = 4;
	}
	else if ((linkOp == 0x2f0e)		/* mov.l  %a6,-(%sp) */
	      || (linkOp == 0x4866))		/* pea    (%a6)      */
	{
	    dispInst = GetWord (adrStart + 4, spaceText) & 0xffff0000;
	    if (dispInst == 0xdffc0000)		/* adda.l  &LF,%sp   */
		saveDisp = GetWord (adrStart + 6, spaceText);
	    else if (dispInst == 0x9ffc0000)	/* suba.l  &-LF,%sp  */
		saveDisp = -GetWord (adrStart + 6, spaceText);
	    else
		continue;			/* no local space */

	    movemOff = 10;
	}
	else
	{
	    continue;				/* no "link" instruction */
	}

	saveInst = GetWord (adrStart + movemOff, spaceText);
	saveOp = (saveInst & 0xffff0000) >> 16;
	if ((saveOp == 0x48ee)		/* movem.l &LS,LF(%a6)  # 16-bit */
	 || (saveOp == 0x48f6)		/* movem.l &LS,LF(%a6)  # 32-bit */
	 || (saveOp == 0x48d7))		/* movem.l &LS,(%sp)		 */
	{
	    maskSave = saveInst & 0xffff;	/* get save mask from instr */
	    if (maskSave & maskTest)		/* frame saved copy of reg */
	    {
	        ty->td.st = stValue;		/* no longer a register	   */
	        adrRet    = fp + saveDisp;	/* base of saved registers */
						/* saveDisp is < 0	*/

	        for (i = 0; i < reg; i++)	/* look for other regs */
	        {
		    if (maskSave & (1 << i))
		        adrRet += CBLONG;	/* saved reg takes up space */
		}
	    }
	}
	else if (((saveOp = ((saveInst & 0xfff00000) >> 16)) == 0x2e80)
					/* mov.l  %reg,(%sp)    */
	       || (saveOp == 0x2c40))   /* mov.l  $reg,LF(%a6)  */
	{
	    if (((saveInst & 0x000f0000) >> 16) == reg)
	        adrRet    = fp + saveDisp;	/* base of saved registers */
						/* saveDisp is < 0	*/
	}
	else
	{
	    continue;				/* no saved registers */
	}
#else /* S200, not S200BSD */
	saveInst = GetWord (adrStart + 8, spaceText);
	if ((saveInst & 0xffff0000) == 0x48ee0000)  /* movem.l instr	*/
	{
	    maskSave = saveInst & 0xffff;	/* get save mask from instr */
	    if (maskSave & maskTest)		/* frame saved copy of reg */
	    {
		saveDisp = GetWord (adrStart + 12, spaceText) >> 16;
	        ty->td.st = stValue;		/* no longer a register	   */
	        adrRet    = fp + saveDisp;	/* base of saved registers */
						/* saveDisp is < 0	*/

	        for (i = 0; i < reg; i++)	/* look for other regs */
	        {
		    if (maskSave & (1 << i))
		        adrRet += CBLONG;	/* saved reg takes up space */
		}
	    }
	}
#endif /* S200, not S200BSD */
#else  /* not S200 */

	maskSave = GetWord (fp + 4, spaceData); /* save mask for this frame */

	if (maskSave & maskTest)		/* frame saved copy of reg */
	{
	    ty->td.st = stValue;		/* no longer a register	   */
	    adrRet    = fp + 20;		/* base of saved registers */

	    for (i = 0; i < reg; i++)		/* look for other regs */
	    {
		if (maskSave & maskReg)
		    adrRet += CBINT;		/* saved reg takes up space */
		maskSave >>= 1;
	    }
	}
#endif  /* not S200 */

    }

    if (ty->td.st == stValue)			/* register image on stack */
    {						/* adjust adr for objects  */
						/*   < 4 bytes in size     */
	long cb = CbFTy (ty, false);		/* size of item */
	if (cb == CBCHAR)
	    adrRet += 3;			/* adjust start address */
	else if (cb == CBSHORT)
	    adrRet += 2;			/* adjust start address */
    }

    return (adrRet);

#endif /* not FOCUS and not SPECTRUM */

} /* AdrFReg */


/***********************************************************************
 * A D R   F   F I E L D
 *
 * Given the start of a struct and its address, look up the given field
 * and return both its type and actual address (using offset info).
 * Field lookup is strictly by name; no other information about the
 * field is saved.
 */

ADRT AdrFField (adrLong, tyStruct, tyField)
    register long	adrLong;	/* start of structure	*/
    register pTYR	tyStruct;	/* parent structure	*/
    register pTYR	tyField;	/* field to get adr of	*/
{
    register int	iss;		/* index in string spc	 */
    register pXTYR	xty =      (pXTYR) (tyStruct + 1); /* ext rec */
    register pXTYR	xtyField = (pXTYR) (tyField + 1); /* ext rec */

#ifdef INSTR
    vfStatProc[397]++;
#endif

/*
 * INITIALIZE FIELD INFO:
 */

    iss = tyField[0].sbVar;		/* save the field name */
    tyField[0] = *vtyZeros;		/* wipe out field info */
    tyField[1] = *vtyZeros;
    tyField[0].sbVar = iss;		/* put the name back */

/*
 * NOT A STRUCTURE or NO INFO:
 *
 * Don't currently do a search through other structures, so you can't
 * reference a field name not part of the appropriate struct/union.
 * Check bt before checking xty in case the XTYR is in use as a DXTYR.
 */

    if (((tyStruct->td.bt != btStruct)
     AND (tyStruct->td.bt != btUnion ))
    OR  (xty->st != stExtend))
    {
	sprintf (vsbMsgBuf, (nl_msg(455, "Invalid field access:  \"%s\"")),
		 SbInCore (tyField->sbVar));
	UError (vsbMsgBuf);
	/* may not have a valid name in tyStruct */
    }
    else

/*
 * GET FIELD INFO:
 *
 * Offset comes back in tyField->valTy.
 */

    {

	TyFFieldHp (tyField, xty->isymRef);	/* isymRef is parent struct */

/*
 * NO SUCH FIELD:
 */

	if ((tyField->td.bt == btNil) && (xtyField->st != stExtend))
	{
	    if (vlc == lcPascal)
	    {
	        sprintf (vsbMsgBuf, (nl_msg(456, "No such field name \"%s\" for that record")),
		    	 SbInCore (tyField->sbVar));
	        UError (vsbMsgBuf);
	    }
	    else if (tyStruct->td.bt == btStruct)
	    {
	        sprintf (vsbMsgBuf, (nl_msg(457, "No such field name \"%s\" for that struct")),
		    	 SbInCore (tyField->sbVar));
	        UError (vsbMsgBuf);
	    }
	    else
	    {
	        sprintf (vsbMsgBuf, (nl_msg(458, "No such field name \"%s\" for that union")),
		    	 SbInCore (tyField->sbVar));
	        UError (vsbMsgBuf);
	    }
	}

    } /* else */

#ifdef HPSYMTABII
     tyField->valTy += tyStruct->valTy;
     adrLong += ((long) tyField->valTy) / SZCHAR;	/* avoid uns int div */
     tyField->valTy %= SZCHAR;
#else
    if (tyField->td.width)				/* it's a bit field  */
    {							/* set to byte bound */
	adrLong += ((long) tyField->valTy) / SZCHAR;	/* avoid uns int div */
	tyField->valTy %= SZCHAR;
    }
    else					/* not a bit field */
    {
	adrLong += tyField->valTy;		/* add offset to base address */
    }
#endif
    return (adrLong);

} /* AdrFField */


#ifdef HPSYMTAB
/***********************************************************************
 * A D R   F   V A R
 *
 * Look up the current variable (vsymCur).  It may be on the stack (at a
 * given depth or any depth).
 * The given ipd must be valid for local vars.	The given ipd may be
 * set to ipdNil for global variables. If the procedure is not on the stack,
 * returns adrNil, but still tries to return type info if possible, e.g.
 * *ty == *vtyZeros or else it is valid.
 */

ADRT AdrFVar (ipd, cnt, ty)
    int		ipd;			/* whose locals		*/
    int		cnt;			/* depth on stack or -1 */
    TYR		*ty;			/* type info to return	*/
{
    ADRT	fp, ap;			/* frame and arg ptrs	*/
    ADRT	adr;			/* the resulting adr	*/

#ifdef INSTR
    vfStatProc[398]++;
#endif

    if (ipd != ipdNil)
        FpApFIpd (&fp, &ap, ipd, cnt);	/* find the proc, or fp == 0  */

    TyFCurSym (ty);			/* must set ty first */

    if ((ipd != ipdNil) AND (fp == 0))
	return (adrNil);

    adr = AdrFIsym (visym, fp, ty);	/* makes all adjustments to net adr */

    return (adr);

} /* AdrFVar */
#endif /* HPSYMTAB */


#ifndef HPSYMTAB
/***********************************************************************
 * A D R   F   L O C A L
 *
 * Look up a local variable on the stack (at a given depth or any depth).
 * The given ipd must be valid.	 If the procedure is not on the stack,
 * returns adrNil, but still tries to return type info if possible, e.g.
 * *ty == *vtyZeros or else it is valid.
 */

ADRT AdrFLocal (ipd, cnt, sbVar, ty, isym)
    int		ipd;			/* whose locals		*/
    int		cnt;			/* depth on stack or -1 */
    char	*sbVar;			/* name pattern if any	*/
    TYR		*ty;			/* type info to return	*/
    int		isym;			/* isym of var if sbvar */
					/* == sbNil             */
{
    ADRT	fp, ap;			/* frame and arg ptrs	*/

#ifdef HPSYMTABII
    int         lang;
#endif

#ifdef SPECTRUM
    ADRT	pc;			/* pc needed to compute */
 					/* address of params    */
 					/* on spectrum          */
#endif

    ADRT	adr;			/* the resulting adr	*/
    int         searchtype;

#ifdef INSTR
    vfStatProc[399]++;
#endif

#ifdef SPECTRUM
#ifdef XDB
    if (vfNoDebugInfo) {
       return(adrNil);
    }
#endif

/* KLUDGE!! for spectrum new procedure call, there is no ap, so the value
   of the pc will be passed back by FpApFIpd instead, because it is
   need for stack unwinding, which is needed to determine the address
   of a parameter!!
*/
    FpApFIpd (&fp, &pc, ipd, cnt);	/* find the proc, or pc == 0  */
    ap = fp;
#else
    FpApFIpd (&fp, &ap, ipd, cnt);	/* find the proc, or fp == 0  */
#endif
    SetSym(vrgPd[ipd].isym);	/* get procedure's language type */
    lang = vsymCur->dfunc.language;
    if (sbVar == sbNil) {
       SetSym(isym);
    }
    else {
       if (FHasAltEntries(ipd)) {
          searchtype = K_DVAR; /* proc has alt entries, params are bad */ 
       }
       else {
          searchtype = K_FPARAM; 
       }
#ifdef HPSYMTABII
       if ((lang == LANG_C) && (pc != 0)) {
          int isltsave = -1;
          int isymbegin;
          int isymend;
          int isymstop;
          int inesting;

          SetSlt(vsymCur->dfunc.address);
          while (FNextSlt (SLT_NORMAL, SLT_EXIT, SLT_NIL, SLT_NIL, SLT_NIL,
		           SLT_FUNCTION, true)
                 AND (vsltCur->snorm.address <= pc)) {
             isltsave = vislt;
          }
          if (isltsave == -1) {
             SetSym(vrgPd[ipd].isym);
             isltsave = vsymCur->dfunc.address;
          }
          SetSlt(isltsave);
          inesting = 1;
          do {
             FNextSlt (SLT_END, SLT_BEGIN, SLT_NIL, SLT_NIL, SLT_NIL,
		       SLT_FUNCTION, true);
             if (vsltCur->sspec.sltdesc == SLT_BEGIN) {
                inesting++;
             }
             else {
                inesting--;
             }
           } while (inesting);
          SetSym(vsltCur->sspec.backptr.dnttp.index);
          for (;;) {
             isymend = visym;
             SetSym(vsymCur->dend.beginscope.dnttp.index);
             isymbegin = visym;
             if (FNextSym(K_BEGIN, K_NIL, K_NIL, K_END, sbNil, FSbCmp,
                          K_NIL, K_NIL, K_NIL, K_NIL)) {
                isymstop = K_BEGIN;
             }
             else {
                isymstop = K_END;
             }
             SetSym(isymbegin);
             if (FNextSym(K_FPARAM, K_DVAR, K_SVAR, isymstop, sbVar, FSbCmp,
                             K_CONST, K_NIL, K_NIL, K_NIL)) {
                goto foundit;
             }
             SetSym(isymend);
             if (vsymCur->dend.endkind == K_FUNCTION) {
	        *ty = *vtyZeros;
	        return (adrNil);		/* no such local var */
             }
             while (vsymCur->dend.beginscope.dnttp.index >= isymbegin) {
                FNextSym(K_END, K_NIL, K_NIL, K_FUNCTION, sbNil, FSbCmp,
                          K_NIL, K_NIL, K_NIL, K_NIL);
             }
          }
       }
       else if (! FNextVar (searchtype,
                            K_SVAR, K_DVAR, K_CONST, K_MEMENUM, sbVar))
       {
#else
       if (! FNextLocal (true, true, sbVar))	/* look for static locals */
       {
	   SetNext (vrgPd[ipd].isym + 1);
	   if (! FNextSym (stStatic, 0, 0, stProc, sbVar, FProcCmp))
#endif
	   *ty = *vtyZeros;
	   return (adrNil);			/* no such local var */
       }
    }

foundit:
#ifdef HPSYMTABII
    if (lang == LANG_HPCOBOL) {
#ifdef NOTDEF
       adr = AdrTyFCobolHp( ty, sbVar, visym, ipd);
#else
	printf("AdrTyFCobolHp attempted to be called\n");
#endif
       return(adr);
    }
    else {
       TyFLocal (ty, sbVar, visym);	/* must set ty first */
    }
#else
    TyFLocal (ty, sbVar, visym);	/* must set ty first */
#endif

#ifdef SPECTRUM
    if (pc == 0)
#else
    if (fp == 0)
#endif
	return (adrNil);

#ifdef HPSYMTABII
    adr = AdrFIsym (visym, fp, ap, pc, ty);	/* makes all adjustments to net adr */

#else /* not HPSYMTABII */

    adr = vsymCur->value;

/*
 * Things of type stReg do NOT get modified.  May want to do look up
 * some day, but this is almost impossible on ONYX.
 */

#endif /* not HPSYMTABII */

    return (adr);

} /* AdrFLocal */


/***********************************************************************
 * A D R   F   G L O B A L
 *
 * Look up the address and type of a global variable by name.
 */

ADRT AdrFGlobal (sbVar, ty)
    char	*sbVar;			/* var to find	    */
    TYR		*ty;			/* return type info */
{
#ifndef HPSYMTABII

    ADRT	adrRet = adrNil;	/* result adr	*/
    char	sbTemp [cbVarMax + 3];	/* to hold name */

#ifdef INSTR
    vfStatProc[400]++;
#endif

    sbTemp[0] = '_';			/* first check for "_foo" */
    strcpy ((sbTemp + 1), sbVar);
    sbTemp[cbVarMax] = chNull;

    SetNext (visymGlobal);

    if (FNextSym (N_DATA, N_BSS, 0, 0, sbTemp, FProcCmp))
	adrRet = vsymCur->value;
    else {				/* try just plain "foo" */
	sbVar[cbVarMax] = chNull;

	SetNext (visymGlobal);
	if (FNextSym (N_DATA, N_BSS, 0, 0, sbTemp, FProcCmp))
	    adrRet = vsymCur->value;
    }
    if (adrRet != adrNil)
	TyFGlobal (ty, sbVar);

    return (adrRet);

# else /* HPSYMTABII */
#ifdef XDB
    if (vfNoDebugInfo) {
       return(adrNil);
    }
#endif


    SetNext (visymGlobal);

    if (FNextVar (K_SVAR, K_CONST, K_MEMENUM, K_NIL, K_NIL, sbVar))
    {
	TyFGlobal (ty, sbVar);
	return (AdrFIsym (visym, 0, 0, 0, tyNil));
    } else {
	return (adrNil);
    }

#endif /* HPSYMTABII */

} /* AdrFGlobal */

#endif  /* not HPSYMTAB */


#ifdef HPSYMTABII
/***********************************************************************
 * A D R   F   I M P O R T E D   M O D U L E S
 *
 * Look up the address and type of a module global variable by name.
 * The variable is assumed to be within a module imported by the
 * module that includes the current procedure, or if the current
 * procedure is the main program, then the module may be imported
 * by the procedure.
 */

ADRT AdrFImportedModules (ipd, sbVar, ty)
    int          ipd;                   /* current procdeure  */
    char	*sbVar;			/* name of var 	      */
    TYR		*ty;			/* return type info   */
{
   int  imd;
   ADRT adr,AdrFModule();

#ifdef INSTR
    vfStatProc[401]++;
#endif

#ifdef XDB
    if (vfNoDebugInfo) {
       return(adrNil);
    }
#endif

   if (ipd == ipdNil) {
      return(adrNil);
   }
   if (!strcmp(vrgPd[ipd].sbAlias,"_MAIN_")) {
      SetSym(vrgPd[ipd].isym);
      while (FNextVar(K_IMPORT, K_NIL, K_NIL, K_NIL, K_NIL, sbNil)) {
         for(imd = 0; imd < vimdMac; imd++) {
            if (!strcmp(vrgMd[imd].sbMod,NameFCurSym())) {
               if ((adr = AdrFModule(imd,sbVar,ty,0)) != adrNil) {
                  return(adr);
               }
            }
         }
      }
   }
   return(AdrFModule(ImdFIpd(ipd),sbVar,ty,0));
   
}



/***********************************************************************
 * A D R   F   M O D U L E
 *
 * Look up the address and type of a module global variable by name,
 * within the supplied module (imd).
 */

ADRT AdrFModule (imd, sbVar, ty, fexport )
    int          imd;                   /* module to search   */
    char	*sbVar;			/* var to find	      */
    TYR		*ty;			/* return type info   */
    FLAGT       fexport;		/* must it be public? */
{
    int imd2;
    pMDR pmd;
    ADRT adr;
    long isymsave;

#ifdef INSTR
    vfStatProc[402]++;
#endif

#ifdef XDB
    if (vfNoDebugInfo) {
       return(adrNil);
    }
#endif

    if (imd == imdNil) {
       return(adrNil);
    }

    pmd = &vrgMd[imd];
    if (! (pmd->vars_in_front || pmd->vars_in_gaps || pmd->imports)) {
       return(adrNil);
    }

    SetNext(pmd->isym);
    if (pmd->vars_in_front && !pmd->vars_in_gaps) {
       if (FNextSym(K_SVAR, K_CONST, K_NIL, K_FUNCTION, sbVar, FSbCmp,
                    K_NIL, K_NIL, K_NIL, K_NIL )) {
          if (!fexport || vsymCur->dsvar.public) {
             TyFGlobal(ty, sbVar);
             return(AdrFIsym(visym,0,0,tyNil));
          }
       }
    }
    if (pmd->vars_in_gaps) {
       if (FNextVar(K_SVAR,K_CONST, K_NIL, K_NIL, K_NIL, sbVar)) {
          if (!fexport || vsymCur->dsvar.public) {
             TyFGlobal(ty, sbVar);
             return(AdrFIsym(visym,0,0,tyNil));
          }
       }
    }
    if (pmd->imports) {
       SetSym(pmd->isym);
       while (FNextSym(K_IMPORT, K_NIL, K_NIL, K_MODULE, sbNil, FSbCmp,
                    K_NIL, K_NIL, K_NIL, K_NIL )) {
          isymsave = visym;
          for(imd2 = 0; imd2 < vimdMac; imd2++) {
             if (!strcmp(vrgMd[imd2].sbMod,NameFCurSym())) {
                if ((adr = AdrFModule(imd2,sbVar,ty,0)) != adrNil) {
                   return(adr);
                }
                else {
                   SetSym(isymsave);
                   break;
                }
             }
          }
       }
    }
    return(adrNil);
} /* AdrFModule */
#endif /* HPSYMTABII */

/***********************************************************************
 * A D R   F   L A B E L
 *
 * Look up the address of a linker (not debugger) symbol from the label
 * (symbol name) and return an address usable for calling the procedure.
 */

ADRT AdrFLabel (sbVar)
    char	*sbVar;		/* label to find */
{

#if (FOCUS || SPECTRUM)

/*
 * We look for the label in the LST and return adr as an EPP (ready to call
 * the procedure), NOT as an EDSP.
 */

    LSTR        lstCur;			/* where to stuff LST symbol	*/
    long	ilstCur;		/* which entry we're at		*/
    long	cbSeek = vcbLstFirst;	/* place in file		*/
    long	cbRead;			/* number read in		*/
    long	type;			/* type of LST entry		*/
    char	*sbCheck;		/* name of entry, to check	*/

#ifdef FOCUS

/*
 * Need this to convert struct to ADRT (cast won't do it):
 */

    union {
	ENTRY	entry;			/* struct from LST */
	ADRT	epp;			/* equivalent word */
    }	conv;

#endif  /* FOCUS */

#ifdef INSTR
    vfStatProc[403]++;
#endif

/*
 * SCAN THE LST:
 */

    for (ilstCur = 0; ilstCur < vilstMax; ilstCur++, cbSeek += cbLSTR)
    {
#ifdef SPECTRUM
        lstCur = vrglst[ilstCur];
#else
	/* have to re-seek each time because SbFInp() seeks elsewhere */
	if (lseek (vfnSym, cbSeek, 0) < 0)
	{
	    sprintf (vsbMsgBuf, (nl_msg(572, "Internal Error (IE572) (%d)")), cbSeek);
	    Panic (vsbMsgBuf);
	}

	if ((cbRead = read (vfnSym, &lstCur, cbLSTR)) != cbLSTR)
        {
	    sprintf (vsbMsgBuf, (nl_msg(573, "Internal Error (IE573) (%d)")), cbRead);
	    Panic (vsbMsgBuf);
        }
#endif

#ifdef SPECTRUM

	type = lstCur.symbol_type;

	if ((type == (long) ST_PRI_PROG)
	OR  (type == (long) ST_SEC_PROG)
	OR  (type == (long) ST_ENTRY)
	OR  (type == (long) ST_DATA)
        OR  (type == (long) ST_CODE))
	{
	    sbCheck = SbFInp (lstCur.name.n_strx);

	    if (FSbCmp	 (sbCheck, sbVar)
	    OR  FProcCmp (sbCheck, sbVar))
	    {
               return(lstCur.symbol_value & ~3); /* mask off privilege level */
	    }
	}

#else /* FOCUS */

	type = lstCur.lst_notype.type;

	if ((type == (long) LST_FUNC)
	OR  (type == (long) LST_ENTRY)
	OR  (type == (long) LST_SYSTEM))
	{
	    sbCheck = SbFInp (lstCur.lst_proc.name);

/*
 * ENTRY FOUND, build proper return value:
 *
 * Note that sbVar must be equal to sbCheck (as usual), or sbCheck can
 * be a cbProcMax-sized substring of sbVar (since SbFInp() may truncate).
 */

	    if (FSbCmp	 (sbCheck, sbVar)
	    OR  FProcCmp (sbCheck, sbVar))
	    {
		if (type == (long) LST_SYSTEM)		/* we have an EPP */
		{
		    conv.entry = lstCur.lst_system.sys_entry;  /* copy struct */
		    return (conv.epp);
		}
		return ((1 << 31) |			/* turn on extern bit */
			(lstCur.lst_proc.stt_index << 14) |	 /* (STT * 4) */
			 lstCur.lst_proc.code_start.ptr_segment);
	    }
	}

#endif /* FOCUS */

    } /* for */

    return (adrNil);			/* failed */

#endif /* FOCUS || SPECTRUM */

#ifdef S200

/*
 * We look for the label in the LST Proc quick reference and return adr.
 */

    char	sb [cbProcMax + 2];	/* buffer for LST entry name	*/
    register char	*psb;		/* pointer to name of entry	*/
    register long 	iPd;		/* LST proc index		*/

#ifdef INSTR
    vfStatProc[403]++;
#endif

/*
 * Search the LST Proc quick reference array.
 * 1st time -- must match leading underscore.  If not found -- search
 * 2nd time without leading underscore.
 */

    for (iPd = 0; iPd < viLSTPdMax; iPd++)
    {
       /*
        * Note that sbVar must be equal to sbCheck (as usual).
	* We don't call FProcCmp since names in the LSTPd are full-length
        */

	if (FSbCmp (vrgLSTPd[iPd].sbProc, sbVar))
	    return (vrgLSTPd[iPd].adrStart);

    } /* for */

    for (iPd = 0; iPd < viLSTPdMax; iPd++)
    {
	psb = vrgLSTPd[iPd].sbProc;
	if (psb[0] == '_') {
	    psb++;
           /*
            * Note that sbVar must be equal to sbCheck (as usual).
	    * We don't call FProcCmp since names in the LSTPd are full-length
            */

	    if (FSbCmp (psb, sbVar))
	        return (vrgLSTPd[iPd].adrStart);
	}

    } /* for */
    return (adrNil);			/* failed			*/

#endif  /* S200 */

} /* AdrFLabel */


/***********************************************************************
 * F P   A P   F   I P D
 *
 * Look up the given procedure on the stack at the specified depth or at
 * any depth if not specified, and return the fp and ap for it.  Returns
 * fp == 0 if it runs out of stack, but complains if a depth was given,
 * the stack is deeper than that, and the proc isn't found by the depth.
 */

void FpApFIpd (pfp, pap, ipd, cnt)
    pADRT	pfp;		/* frame ptr to return	*/
    pADRT	pap;		/* arg	 ptr to return	*/
    int		ipd;		/* ipd to find on stack */
    int		cnt;		/* stack depth or -1	*/
{
    FLAGT	fAnyDepth;	/* any depth OK? */
    int		depth = 0;	/* current depth */
    ADRT	pc    = vpc;	/* current pc	 */

#ifdef INSTR
    vfStatProc[404]++;
#endif

    if (fAnyDepth = (cnt == -1))
	cnt = 10000;			/* set to a LARGE value */

#ifdef SPECTRUM
    *pfp = vsp;
    *pap = vsp;
#else
    *pfp = vfp;
    *pap = vap;
#endif

#ifdef SPECTRUM
    for ( ; pc; depth++)
#else
    for ( ; *pfp; depth++)
#endif
    {
	if ( (fAnyDepth OR (depth == cnt))	/* at any or right depth */
	AND  (IpdFAdr (pc) == ipd) )		/* hit the right proc	 */
	{
#ifdef SPECTRUM /* terrible kludge  (see below) */
            *pap = pc;
#endif
	    return;
        }

        if (depth >= cnt)
        {
	    sprintf (vsbMsgBuf, (nl_msg(419, "Procedure \"%s\" not found at stack depth %d")),
		     vrgPd[ipd].sbProc, cnt);
	    UError (vsbMsgBuf);
	}

	NextFrame (pfp, pap, &pc);
    }
#ifdef SPECTRUM 
/* terrible KLUDGE: with spectrum new procedure call,
   we need the pc for use with stack unwind descriptors, and there is
   not ap register, so we pass the pc back instead!!!
*/  
            *pap = pc;
#endif

} /* FpApFIpd */

#ifdef SPECTRUM

/***********************************************************************
 * I S T A C K   U N W I N D   I N D E X
 *
 * returns index of stack unwind descriptor for given pc value
 */

pSUR IStackUnwindIndex (pc)
   ADRT pc;
{
   register int ilow,ihigh,imid;

#ifdef INSTR
    vfStatProc[405]++;
#endif

   ilow = 0;
   ihigh = viUnwindMax - 1;
   while (ilow <= ihigh) {
      imid = (ilow + ihigh) / 2;
      if (pc < vrgUnwind[imid].adrstart) {
         ihigh = imid - 1;
      }
      else {
         if (pc > vrgUnwind[imid].adrend) {
            ilow = imid + 1;
         }
         else {
            return(&vrgUnwind[imid]);
         }
      }
   }
   return(NULL);
} /* IStackUnwindIndex */

/***********************************************************************
 * I S T U B   S T A C K   U N W I N D   I N D E X
 *
 * returns index of stub stack unwind descriptor for given pc value
 */

pSTUBSUR IStubStackUnwindIndex (pc)
   ADRT pc;
{
   register int ilow,ihigh,imid;

#ifdef INSTR
    vfStatProc[406]++;
#endif

   ilow = 0;
   ihigh = viStubUnwindMax - 1;
   while (ilow <= ihigh) {
      imid = (ilow + ihigh) / 2;
      if (pc < vrgStubUnwind[imid].adrstart) {
         ihigh = imid - 1;
      }
      else {
         if (pc > vrgStubUnwind[imid].adrstart + 
                  (vrgStubUnwind[imid].length << 2)) {
            ilow = imid + 1;
         }
         else {
            return(&vrgStubUnwind[imid]);
         }
      }
   }
   return(NULL);
} /* IStubStackUnwindIndex */

#endif  /* SPECTRUM */

/***********************************************************************
 * N E X T   F R A M E
 *
 * Step to the next stack (procedure) frame, if any, and return the
 * register values.  Returns fp == 0 if already at the last frame.
 * Note that ap == fp for most non-VAXen. sp == fp == ap for spectrum
 * non oldcall convention.
 */

void NextFrame (pfp, pap, ppc)
    ADRT	*pfp;		/* fp to return */
    ADRT	*pap;		/* ap to return */
    ADRT	*ppc;		/* pc to return */
{
#ifdef SPECTRUM

#define max_entex_len 30

#define milli_fudge_factor ((psu->save_rp || psu->save_mrp) ? 1 : 0)

    pSTUBSUR    pstubsu;
    int		param_push;
    int		param_pop;
    int		return_push;
    int		return_pop;
    int		curr_inst;
    int		store_rp_inst;

    pSUR        psu;
    int         idist_to_entry;
    int         idist_to_exit;
    int         ientry_len;
    int         iexit_len;
    int         iframe_size;
    int         vrgientry_code[max_entex_len];
    int         vrgientry_size[max_entex_len];
    int         vrgiexit_code[max_entex_len];
    int         vrgiexit_size[max_entex_len];
    char	*label;
    int		offset;
    FLAGT	vfEndOfStack = false;
#ifdef DEBREC
    struct save_state  *ss = *pfp;
#else
    struct sigcontext *scon = *pfp;
#endif

#ifdef INSTR
    vfStatProc[407]++;
#endif

    if (vipd_MAIN_ != ipdNil) {
       if (IpdFAdr(*ppc) == vipd_MAIN_) {
          vfEndOfStack = true;
       }
    }
    else {
       LabelFAdr(*ppc,&label,&offset);
#ifdef HPE
       if (!strcmp(label,"_start") ||
           !strcmp(label,"main")) {
#else
       if (!strcmp(label,"_start")) {
#endif
          vfEndOfStack = true;
       }
    }
    if (vfUnwind && ((psu = IStackUnwindIndex(*ppc)) != NULL)) {

       idist_to_entry = (*ppc - psu->adrstart) >> 2;
       idist_to_exit =  ((psu->adrend - *ppc) >> 2) + 1;

       if (psu->hpux_int_mark) {
          int flags;

#ifdef DEBREC
	  /* Get base of save state, remeber its rounded to double word */
	  ss = (struct save_state *)((int)ss - (int)(((psu->frame_size << 3) + 15) / 16 * 16) );

          flags = GetWord(&ss->ss_flags, spaceData);
          if (flags & SS_INTRAP) {
             *ppc = GetWord(&ss->ss_pcoq_head, spaceData) & ~3;
          }
          else {
             *ppc = GetWord(&ss->ss_rp, spaceData) & ~3;
          }
          *pfp -= (((psu->frame_size << 3) + 15) / 16 * 16);
#else
          scon--;
          flags = GetWord(&scon->sc_sl.sl_ss.ss_flags, spaceData);
          if (flags & SS_INTRAP) {
             *ppc = GetWord(&scon->sc_sl.sl_ss.ss_pcoq_head, spaceData) & ~3;
          }
          else {
             *ppc = GetWord(&scon->sc_sl.sl_ss.ss_rp, spaceData) & ~3;
          }
          *pfp -= sizeof(struct sigcontext);
#endif
          intrpt_marker = 1;
       }
       else if(!psu->millicode) {
          if ((idist_to_entry == 0) || (idist_to_exit == 0)) {
             *ppc = GetReg(urp);   /* fp is already correct */
          }
          else {
             build_entry_exit_info(
                ((int *) psu)[2],
                ((int *) psu)[3],
                vrgientry_code,
                vrgientry_size,
                &ientry_len,
                vrgiexit_code,
                vrgiexit_size,
                &iexit_len
             );
             if (idist_to_entry < ientry_len) {
                *pfp = *pfp - vrgientry_size[idist_to_entry - 1];
                *ppc = GetReg(urp);
             }
             else if (idist_to_exit < iexit_len) {
                *pfp = *pfp - vrgiexit_size[iexit_len - idist_to_exit - 1];
                if (psu->save_rp) {
                   *ppc = GetWord( *pfp - 20, spaceData ) & ~3;
                }
                else {
                   *ppc = GetReg(urp);
                }
             }
             else {
                if (psu->save_sp) {
                   int newfp;

                   newfp = GetWord( *pfp -  4, spaceData );
                   if (newfp == 0) {
                      vfEndOfStack = true;
                      *pfp = *pfp - 8 * psu->frame_size;
                   }
                   else {
                      *pfp = newfp;
                   }
                }
                else {
                   *pfp = *pfp - 8 * psu->frame_size;
                }
                if (*pfp != 0) {
                   if (psu->save_rp) {
                      *ppc = GetWord( *pfp - 20, spaceData ) & ~3;
                   }
                   else {
                      if (intrpt_marker) {
#ifdef DEBREC
                        *ppc = GetWord(&ss->ss_rp,
                                       spaceData) & ~3;
#else
                        *ppc = GetWord(&scon->sc_sl.sl_ss.ss_rp,
                                       spaceData) & ~3;
#endif
                      }
                      else {
                         *ppc = GetReg(urp);
                      }
                   }
                }
             } 
          } 
          intrpt_marker = 0;
       } 
       else {   /* millicode */
          if ((idist_to_entry == 0) || (idist_to_exit == 0)) {
             *ppc = GetReg(umrp);   /* fp is already correct */
          }
          else {
             int ihold_save_rp;

             ihold_save_rp = psu->save_rp;
             psu->save_rp = 0;
             build_entry_exit_info(
                ((int *) psu)[0],
                ((int *) psu)[1],
                vrgientry_code,
                vrgientry_size,
                &ientry_len,
                vrgiexit_code,
                vrgiexit_size,
                &iexit_len
             );
             psu->save_rp = ihold_save_rp;
             if (idist_to_entry < ientry_len + milli_fudge_factor) {
                *pfp = *pfp - vrgientry_size[idist_to_entry - 1];
                *ppc = GetReg(umrp);
             }
             else if (idist_to_exit < iexit_len) {
                iframe_size = vrgiexit_size[iexit_len - idist_to_exit - 1];
                *pfp = *pfp - iframe_size;
                if (iframe_size == psu->frame_size) {
                   if (psu->save_mrp) {
                      *ppc = GetWord(*pfp - 20, spaceData);
                   }
                   else {
                      *ppc = GetReg(umrp);
                   }
                }
                else {
                   *ppc = GetReg(umrp);
                }
             }
             else {
                if (psu->save_sp) {
                   *pfp = GetWord( *pfp -  4, spaceData );
                }
                else {
                   *pfp = *pfp - 8 * psu->frame_size;
                }
                if (psu->save_rp || psu->save_mrp) {
                   *ppc = GetWord( *pfp - 20, spaceData ) & ~3;
                }
                else {
                   if (intrpt_marker) {
#ifdef DEBREC
                     *ppc = GetWord(&ss->ss_gr31,
                                    spaceData) & ~3;
#else
                     *ppc = GetWord(&scon->sc_sl.sl_ss.ss_gr31,
                                    spaceData) & ~3;
#endif
                   }
                   else {
                      *ppc = GetReg(umrp);
                   }
                }
             }
          }
       }
    }
    else if (vfUnwind && ((pstubsu = IStubStackUnwindIndex(*ppc)) != NULL)) {
       switch (pstubsu->stub_type) {

       case long_branch_rp:
          *ppc = GetReg(urp);
          break;

       case long_branch_mrp:
          *ppc = GetReg(umrp);
          break;

       case import:
          *ppc = GetReg(urp);
          break;

       case relocation:
       case export_reloc:
          curr_inst = ((*ppc - pstubsu->adrstart) >> 2) + 1;
          param_push = 1;
          param_pop  = pstubsu->instr_num;
          if (pstubsu->stub_type == relocation) {
             return_push = param_pop + 4;
             return_pop  = pstubsu->length - 2;
          }
          else {
             return_push = param_pop + 3;
             return_pop  = pstubsu->length - 5;
          }
   
          if (((curr_inst > param_push) && (curr_inst <= param_pop)) ||
              ((curr_inst > return_push) && (curr_inst <= return_pop))) {
             *pfp = *pfp - 8;
          }
   
          if (pstubsu->stub_type == relocation) {
             store_rp_inst = param_pop + 1;
             if (curr_inst <= store_rp_inst) {
                *ppc = GetReg(urp);
             }
             else {
                *ppc = GetWord(*pfp - 8, spaceData);
             }
             break;
          }

       case export:
          *ppc = GetWord(*pfp - 24, spaceData);
       }
       intrpt_marker = 0;
    }
    else {
       *pfp = GetWord( *pfp -  4, spaceData );
       *ppc = GetWord( *pfp - 20, spaceData ) & ~3;
       intrpt_marker = 0;
    } 
    *pap = *pfp;
    *ppc = *ppc & ~3;
    if (((vipd_END_ != ipdNil) && (IpdFAdr(*ppc) == vipd_END_)) ||
        vfEndOfStack) {
       *ppc = 0;
    }
#endif

#ifdef M68000
#ifdef INSTR
    vfStatProc[407]++;
#endif
    *ppc = GetWord (*pfp + CBINT, spaceData);
    if (*ppc > 0xffff8000) *ppc = GetWord(*pfp + 38, spaceData);
    *pfp = GetWord (*pfp, spaceData);
    *pap = *pfp;
#endif

/*#ifdef S200BSD
    {
	char	*sbLabel;
	long	offset;

	LabelFAdr (*ppc, &sbLabel, &offset);

	if ( ! strcmp (sbLabel, "start"))
	    *pfp = *pap = 0;
    }
#endif
*/

#ifdef FOCUS	/* note that fp == Q register == top of stack marker */
    long	cbStack;			/* equal to (Q-3 - SB)	*/
    long	cbDelta;			/* delta Q to next S.M. */
    long	status;				/* status register	*/
    long	relpc;				/* segment-relative PC	*/
    long	adr;				/* the "fixed" EDSP	*/

    cbStack = ((*pfp) & 0x3fffffff) - 3;	/* ignore ptr type bits */
						/* ignore DISP bit:	*/
    cbDelta = GetWord ((*pfp) - 3, spaceData) & 0x7fffffff;

    if ((cbDelta % 4) OR (cbDelta > cbStack))
    {
	sprintf (vsbMsgBuf,
		 (nl_msg(574, "Internal Error (IE574) (%d, %d)")), *pfp, cbDelta);
	Panic (vsbMsgBuf);
    }

    if (cbDelta == 0)				/* at bottom of stack	   */
	*pfp = 0;				/* other values irrelevant */
    else {
	status = GetWord ((*pfp) -  7, spaceData);	/* get from S.M.    */
	relpc  = GetWord ((*pfp) - 11, spaceData);	/* get from S.M.    */
	adr    = ((status & 0xfff) << 19) | (relpc & 0x7ffff); /* make EDSP */
	*ppc   = AdrFAdr (adr, false, ptrCode);		/* break as ptrCode */

	*pfp -= cbDelta;			/* drop down delta Q bytes */
	*pap  = *pfp;
    }
#endif /* FOCUS */

} /* NextFrame */


/***********************************************************************
 * D I S P   F R A M E
 *
 * Display information about the current stack frame, which belongs to
 * a known (debuggable) procedure.  Prints parameter plus values or types,
 * optionally followed by locals.  Does NOT let DispVal() use alternate
 * form, in case a pointer is invalid (don't want to bomb out), and to
 * keep the listing short.
 */

void DispFrame (pc, ipd, fp, ap, cnt, fDoLocals, fDoVal)
    ADRT	pc;		/* proc pc if active	*/
    int		ipd;		/* ipd of current proc	*/
    ADRT	fp;		/* frame ptr, not used	*/
    ADRT	ap;		/* arg	 ptr, not used	*/
    int		cnt;		/* depth on stack	*/
    FLAGT	fDoLocals;	/* dump local vars too? */
    FLAGT	fDoVal;		/* show values of vars? */
{
    long	isymSave;	/* to remember position	*/
#ifdef HPSYMTABII
    long	isymLocal;	/* to position for locs */
#endif
    int		fComma = false; /* print comma?		*/
    long	adrLong;	/* adr	of parameter	*/
    TYR		rgTy[cTyMax];	/* type of parameter	*/

    char	sbName[cbTokMax];/* name of current proc */
    int		ifd;		/* fd of current proc	*/
    int		iln;		/* ln of current proc	*/
    int		slop;		/* offset from line	*/

#ifdef INSTR
    vfStatProc[408]++;
#endif

    printf ("%s (", vrgPd[ipd].sbProc);		/* print "proc (" */

/*
 * SHOW PARAMETERS:
 */

#ifdef HPSYMTAB
    SetUpSrch (vrgPd[ipd].isd);		/* search current procedure scope */
#else  /* HPSYMTABII */
    SetNext (isymLocal = vrgPd[ipd].isym + 1);  /* start at next  */
#endif

    while (FNextVar (K_FPARAM, K_NIL, K_NIL, K_NIL, K_NIL, sbNil))
    {
	isymSave = visym;
	strcpy (sbName, NameFCurSym());
#ifdef HPSYMTAB
	adrLong = AdrFVar (ipd, cnt, rgTy);
#else
	adrLong = AdrFLocal (ipd, cnt, sbName, rgTy, 0);
#endif

	if (fComma++)			/* finish previous param */
	    printf (", ");

	if (fDoVal)			/* show values of params */
	    DispVal (adrLong, rgTy, modeNil, false, true, false);
	else				/* just show types */
	    PxTy (rgTy);

	SetSym (isymSave);

#ifdef DEBREC
	if (xdbtraceabort)
		break;
#endif


    } /* while */

    printf (")");			/* finish off param list */

/*
 * SHOW CODE LOCATION:
 */

    if (pc != adrNil)			/* proc is active */
    {
	isymSave = visym;
#ifdef NOTDEF
	IfdLnFAdr (pc, vrgPd[ipd].isym, &ifd, &iln, &slop);
	printf ("    [%s: %d]", vrgFd[ifd].sbFile, iln);
#else
	printf(" pc 0x%x  ",pc);
#endif
	SetSym (isymSave);
    }

/*
 * SHOW LOCAL VARS:
 */

    if (fDoLocals)
    {
#ifdef HPSYMTABII
	SetNext (isymLocal);	/* make it the next one to search */
#endif
	vcNest++;		/* in case have struct to indent  */

	while (FNextVar (K_SVAR, K_DVAR, K_CONST, K_NIL, K_NIL, sbNil))
	{
	    strcpy (sbName, NameFCurSym());
#ifdef HPSYMTAB
	    adrLong  = AdrFVar (ipd, cnt, rgTy);
#else
            isymSave = visym;
	    adrLong  = AdrFLocal (ipd, cnt, sbNil, rgTy, visym);
#endif
	    printf  ("\n\t");
	    DispVal (adrLong, rgTy, modeNil, true, true, false);
#ifdef HPSYMTABII
            SetSym (isymSave);
#endif
#ifdef DEBREC
	if (xdbtraceabort)
		break;
#endif
        }

	vcNest--;
    } /* if */

    printf ("\n");

} /* DispFrame */


/***********************************************************************
 * D I S P   U N K N O W N
 *
 * Display information about the current stack frame, which belongs to
 * an unknown (non-debuggable) procedure.  All it knows is the linker's
 * name for it and the offset in the proc, plus values on the stack.
 */

void DispUnknown (pc, fp, ap)
    ADRT  	  pc;		/* proc pc	*/
    ADRT	  fp;		/* frame ptr	*/
    ADRT 	ap;		/* argument ptr */
{
    register int  i;		/* index */

#ifdef SPECTRUM
    pSTUBSUR    pstubsu;
#define cArgsMax 4		/* max param words to show */
#else
#define cArgsMax 5		/* max param words to show */
#endif

/*
 * SHOW PROC NAME and offset:
 */

    char	*sbLabel;	/* proc name */
    long	offset;		/* in proc   */

#ifdef INSTR
    vfStatProc[409]++;
#endif

    LabelFAdr (pc, &sbLabel, &offset);		/* returns values directly */
    printf ("%s", sbLabel);

    if (offset)
	printf (" + %s", SbFAdr (offset, false));

/*
 * SHOW PARAMETERS, or at least what looks like them:
 */

    printf (" (");

#ifdef SPECTRUM
    if (vfUnwind && ((pstubsu = IStubStackUnwindIndex(pc)) != NULL)) {
       switch (pstubsu->stub_type) {
       case long_branch_rp:
          printf("long branch stub)\n");
          break;
       case long_branch_mrp:
          printf("millicode long branch stub)\n");
          break;
       case import:
          printf("import stub)\n");
          break;
       case export:
          printf("export stub)\n");
          break;
       case relocation:
          printf("parameter relocation stub)\n");
          break;
       case export_reloc:
          printf("export parameter relocation stub)\n");
          break;
       }
       return;
    }
    i = intrpt_marker;
    NextFrame(&fp,&ap,&pc);
    intrpt_marker = i;
    fp -= 36;

    for (i = 0; i < cArgsMax; i++)
    {
#ifdef DEBREC
	/* Do not look beyond start of stack */
#ifndef REGION
	if (fp < 0x80000000)
		break;
#else
	if (fp < 0x68007000)
		break;

#endif
#endif
	printf ("%#lx%s", GetWord (fp, spaceData),
		(i < cArgsMax - 1) ? ", " : "");
	fp -= CBINT;
    }

#else
#ifndef FOCUS

#ifdef S200
    ap += 8;
#endif

#define cArgsMax 5			/* max words to show */

    for (i = 0; i < cArgsMax; i++)
    {
	printf ("%#lx%s", GetWord (ap, spaceData),
		(i < cArgsMax - 1) ? ", " : "");
	ap += CBINT;
    }

#else /* FOCUS */

/*
 * We have to be careful not to go too far, in case the procedure we are
 * showing was called with the MODCAL calling sequence (in which case there
 * is no valid argument count).
 */

    {	long	cArgs = GetWord ((fp -= 27), spaceData);	/* arg count */

	if ((cArgs < 0) OR (cArgs > cArgsMax))		 /* probably invalid */
	    cArgs = cArgsMax;

	while (cArgs--)					  /* some args left */
	    printf ("%#lx%s", GetWord ((fp -= 4), spaceData),	/* get next */
		(cArgs ? ", " : ""));
    }

#endif /* FOCUS */
#endif /* not SPECTRUM */

    printf (")\n");			/* finish line */

} /* DispUnknown */


/***********************************************************************
 * S T A C K   T R A C E
 *
 * Trace the child process's stack from the newest stack frame (procedure
 * invocation) to the oldest, printing info about each stack frame, which
 * represents either a known (debuggable) or unknown procedure.	 Go no
 * deeper than "cnt" frames, if given.
 */

void StackTrace (cnt, fDoLocals)
    int		cnt;		/* max stack depth  */
    FLAGT	fDoLocals;	/* show local vars? */
{
    int		depth;		/* curr stack depth */
    int		ipd;		/* current proc	    */
    ADRT	pc = vpc;	/* current values   */
#ifdef SPECTRUM
    ADRT	fp = vsp;
    ADRT	ap = vsp;
#else
    ADRT	fp = vfp;
    ADRT	ap = vap;
#endif
#ifdef DEBREC
	int icsbase;
#endif

#ifdef INSTR
    vfStatProc[410]++;
#endif
#ifdef DEBREC
	icsbase = lookup("icsBase");
#endif

    if ((vpid == pidNil) && (vfnCore == fnNil))
	UError ((nl_msg(304, "No child process")));

#ifdef SPECTRUM
    for (depth = 0; ((depth < cnt) AND (pc != 0)); depth++)
#else
    for (depth = 0; ((depth < cnt) AND (fp != 0)); depth++)
#endif
    {
#ifdef DEBREC
	if (xdbtraceabort)
		break;

	/* Kludge for now   STACKADDR */
	if ((fp <= 0x80000060) && (fp > 0x70000000))
		break;
	if (fp <= (lookup("icsBase") + 0x60))
		break;
	/* REGIONS... A guess.. STACKADDR for now must go and see */
	if ((fp <= 0x68007060) && (fp > 0x68000000))
		break;
#endif
	printf ("%2d ", depth);

	if ((ipd = IpdFAdr (pc)) == ipdNil)	/* unknown proc */
	    DispUnknown (pc, fp, ap);
	else					/* known proc */
	    DispFrame (pc, ipd, fp, ap, depth, fDoLocals, true);

	NextFrame (&fp, &ap, &pc);
    }
} /* StackTrace */


/***********************************************************************
 * L I S T   R E G S
 *
 * List register special variables (all of which are predefined only),
 * along with current values (if available).  Optionally select only
 * matching names.  There are always registers (no reason to check for
 * none as a special case).
 */

void ListRegs (sbRegs)
    register char *sbRegs;	/* name to match */
{
    FLAGT	  fNoData;	/* data missing? */
    register int  ispc;		/* current index */
    long	  adr;		/* adr of var	 */
    MODER	  dispMode;	/* display mode for registers */
    
#ifdef INSTR
    vfStatProc[411]++;
#endif

#ifdef SPECTRUM
    if (fNoData = ((vpid == pidNil) AND (vfnCore == fnNil)))
	printf ((nl_msg(233, "No child process or corefile; here are the names:\n")));

    dispMode.df = dfHex;
    dispMode.len = 4;
    dispMode.cnt = 1;
    dispMode.imap = 0;

    for (ispc = 0; ispc < vispcMac; ispc++)
    {
	if ( (vrgTySpc[ispc].td.st != stReg)
	OR   ((sbRegs != sbNil)
	  AND (! FHdrCmp (sbRegs, SbInCore (vrgTySpc[ispc].sbVar)))) )
	{
	    continue;			/* skip this one */
	}

	if (fNoData)
	    printf ("%s", SbInCore (vrgTySpc[ispc].sbVar));
	else
	{ 
	    adr = vrgTySpc[ispc].valTy;
	    DispVal (adr, (vrgTySpc + ispc), & dispMode, true, true, true);
	}
	printf ("\n");
    }
#else
    if (fNoData = ((vpid == pidNil) AND (vfnCore == fnNil)))
    {
	printf ((nl_msg(461, "No child process or corefile.\n")));
	return;
    };

    if (sbRegs == sbNil)
    {
#ifdef M68000
	STUFFU	stuff;
	/* ps */
        printf ("$ps  0x%.2x\n", GetReg(ups));
	/* pc */
	printf ("$pc  0x%.8x  ", vpc);
	PrintInst(vpc);
	printf ("\n\n");
	/* sp */
/*	stuff.lng = GetReg(usp);
	printf ("$sp  0x%.8x\n", stuff.lng); */
	/* fp */
/*	stuff.lng = GetReg(ufp);
	printf ("$fp  0x%.8x\n\n", stuff.lng); */
	/* registers */
	for (ispc = 0; ispc < ua0; ispc++)
	{
	    stuff.lng = GetReg(ispc + ud0);
	    printf ("$d%.1d  0x%.8x        ", ispc, stuff.lng);
	    stuff.lng = GetReg(ispc + ua0);
	    printf ("$a%.1d  0x%.8x", ispc, stuff.lng);
	    if (ispc == 6)
		printf ("  <-- $fp\n");
	    else if (ispc == 7)
		printf ("  <-- $sp\n");
	    else
		printf ("\n");
	}
#ifdef S200BSD
	ListFloatRegs();
#endif
	
#endif
#ifdef FOCUS
	dispMode.df = dfHex;
	dispMode.len = 4;
	dispMode.cnt = 1;
	dispMode.imap = 0;

	for (ispc = 0; ispc < vispcMac; ispc++)
	{
	    if (vrgTySpc[ispc].td.st != stReg)
	    {
		continue;			/* skip this one */
	    }

	    adr = vrgTySpc[ispc].valTy;
	    DispVal (adr, (vrgTySpc + ispc), & dispMode, true, true, true);
	    printf ("\n");
	}
#endif
    }
    else
    {
        dispMode.df = dfHex;
        dispMode.len = 4;
        dispMode.cnt = 1;
        dispMode.imap = 0;

        for (ispc = 0; ispc < vispcMac; ispc++)
        {
    	    if ( (vrgTySpc[ispc].td.st != stReg)
	      OR (! FHdrCmp (sbRegs, SbInCore (vrgTySpc[ispc].sbVar))) )
	    {
	        continue;			/* skip this one */
	    }

	    if (fNoData)
	        printf ("%s", SbInCore (vrgTySpc[ispc].sbVar));
	    else
	    { 
	        adr = vrgTySpc[ispc].valTy;
	        DispVal (adr, (vrgTySpc + ispc), & dispMode, true, true, true);
	    }
	    printf ("\n");
        }
    }
#endif
} /* ListRegs */


/***********************************************************************
 * L I S T   S P E C I A L
 *
 * List non-register special variables (predefined ones first), along
 * with current values.	 Optionally select by name only.
 *
 * There are always specials; no need to check for none as a special case.
 *
 * This routine overlaps a lot with ListRegister(); they could be merged.
 */

void ListSpecial (sbSpecial)
    register char *sbSpecial;	/* name to match */
{
    register long  i;		/* current index */
    long	   adr;		/* adr of var	 */

#ifdef INSTR
    vfStatProc[412]++;
#endif

    for (i = 0; i < vispcMac; i++)		/* predefined specials */
    {
	if ((vrgTySpc[i].td.st == stReg)
	OR  ((sbSpecial != sbNil)
	 AND (! FHdrCmp (sbSpecial, SbInCore (vrgTySpc[i].sbVar)))))
	{
	    continue;				/* skip this one */
	}
	adr = vrgTySpc[i].valTy;
	DispVal (adr, (vrgTySpc + i), modeNil, true, true, true);
	printf ("\n");
    }

    for (i = 0; i < vilvMac ; i++)		/* user-defined specials */
    {						/* (no registers here)	 */
	if ((sbSpecial != sbNil)
	AND (! FHdrCmp (sbSpecial, SbInCore (vrgTyLv[i].sbVar))))
	{
	    continue;				/* skip this one */
	}
	adr = (long) & (vrgTyLv[i].valTy);
	DispVal (adr, (vrgTyLv + i), modeNil, true, true, true);
	printf ("\n");
    }
} /* ListSpecial */

#ifdef S200

/***********************************************************************
 * A D R   F   L S T    A D R 
 *
 * Given the address of any procedure, look in the LST
 * quick reference table and return the starting address.
 */

static ADRT AdrFLSTAdr (adr)
    ADRT	  adr;		/* address to look up		*/
{
    register long iPd;		/* table index			*/

#ifdef INSTR
    vfStatProc[413]++;
#endif

   /*
    * Search LST quick reference table for nearest address.  LST
    * table is in sorted order, so we can quit when we find first
    * address larger than the one we want (return previous).
    */

    for (iPd = 0; iPd < viLSTPdMax; iPd++)
    {
	if (vrgLSTPd[iPd].adrStart > adr)
	{
	    if (iPd == 0)
	    {
		sprintf (vsbMsgBuf, (nl_msg(462, "Address out of bounds in AdrFLSTAdr() (%d)")),
			 adr);
		Panic (vsbMsgBuf);
	    }

	    return (vrgLSTPd[iPd-1].adrStart);
	}
	else if (vrgLSTPd[iPd].adrStart == adr)
	    return (adr);
    }
    sprintf (vsbMsgBuf, (nl_msg(463, "Address out of bounds in AdrFLSTAdr() (%d)")), adr);
    Panic (vsbMsgBuf);					/* address not found */

}  /* AdrFLSTAdr */

#endif  /* S200 */


/***********************************************************************
 * L A B E L   F   A D R
 *
 * Given the address of a non-debuggable procedure, look up the name in
 * the linker symbol table.
 */

void LabelFAdr (adr, psbLabel, pOffset)
    register ADRT	adr;	/* address to look up		 */
    char	**psbLabel;	/* HPSYMTAB only: name of label	 */
    long	*pOffset;	/* HPSYMTAB only: offset in proc */
{

#ifdef SPECTRUM

    LSTR        lstCur;			/* where to stuff LST symbol	*/
    long	ilstCur;		/* which entry we're at		*/
    long	cbRead;			/* bytes read in		*/
    long	type;			/* type of LST entry		*/
    long	offCur;			/* seg offset of current entry	*/
    long	offNearest = -1;	/* seg offset of nearest entry	*/
    long	inpNearest;		/* name of nearest LST entry	*/

    for (ilstCur = 0; ilstCur < vilstMax; ilstCur++)	/* search the LST */
    {
        lstCur = vrglst[ilstCur];
	type = lstCur.symbol_type;

	if (((type == (long) ST_PRI_PROG)
	OR   (type == (long) ST_SEC_PROG)
	OR   (type == (long) ST_ENTRY)
	OR  (type == (long) ST_DATA)
        OR   (((type == (long) ST_CODE) 
        OR   (type == (long) ST_MILLICODE))
        AND   (lstCur.symbol_scope == SS_UNIVERSAL)))
	AND (((offCur = lstCur.symbol_value & ~3) <= adr)
	AND  (offCur > offNearest)))
	{						/* new best guess */
	    offNearest = offCur;			/* best offset	 */
	    inpNearest = lstCur.name.n_strx;		/* best name	 */
	    if (offCur == adr)				/* direct hit	 */
		break;					/* can quit now	 */
	}
    } /* for */

   *psbLabel = SbFInp (inpNearest);
   *pOffset  = adr - offNearest;

#else /* not SPECTRUM */

#ifdef S200

/*
 * Return nearest TEXT label <= adr:
 */

    static char	sb [cbProcMax + 2];	/* name of nearest LST entry	*/
    register long	adrTest;	/* start adr of test proc	*/
    register long 	iPd;		/* LST proc index		*/

#ifdef INSTR
    vfStatProc[414]++;
#endif

/*
 * Search the LST Proc quick reference array.  It is already in ascending
 * order by proc starting address, so we can quit when we find an address
 * >= the one we want.
 */

    for (iPd = 0; iPd < viLSTPdMax; iPd++)
    {
	adrTest = vrgLSTPd[iPd].adrStart;
	if ((adrTest >= adr) || (iPd == (viLSTPdMax - 1)))
	{

	    if (adrTest > adr)
	    {
		iPd--;				/* iPd -> proc we want	*/
	        if (iPd == -1)
	        {
		    *psbLabel = vsbunknown;
		    *pOffset  = 0;
		    return;
	        }
	    }

	    strncpy (sb, vrgLSTPd[iPd].sbProc, cbProcMax+1);
	    sb[cbProcMax+1] = '\0';		/* ensure terminating null*/
	    *psbLabel = sb; 			/* don't skip leading '_' */
	    *pOffset  = adr - vrgLSTPd[iPd].adrStart;
	    return;
	}
    }

#endif  /* S200 */

#ifdef FOCUS

/*
 * Adr is given as an EDSP, as usual, NOT as an EPP.  We look for the FUNC or
 * ENTRY entry in the LST with the same segment number and the greatest offset
 * not exceeding that in the given adr.	 We don't check system entry points
 * (LST_SYSTEMs) because they don't have offsets given, but that's OK because
 * we should never be trying to find the name of one of them, anyway.
 *
 * We can't set vsymCur to an LST entry, so instead we return the name and
 * offset directly.  They're static so they're only good until the next call.
 */

    LINK_SYMBOL lstCur;			/* where to stuff LST symbol	*/
    long	ilstCur;		/* which entry we're at		*/
    long	cbRead;			/* bytes read in		*/
    long	type;			/* type of LST entry		*/
    long	segIn = Extract (adr,  1, 12);	/* required seg number	*/
    long	offIn = Extract (adr, 13, 19);	/* desired  seg offset	*/
    long	offCur;			/* seg offset of current entry	*/
    long	offNearest = -1;	/* seg offset of nearest entry	*/
    long	inpNearest;		/* name of nearest LST entry	*/

    if (lseek (vfnSym, vcbLstFirst, 0) < 0)		/* seek start of LST */
    {
	sprintf (vsbMsgBuf, (nl_msg(576, "Internal Error (IE576) (%d)")), vcbLstFirst);
	Panic (vsbMsgBuf);
    }

    for (ilstCur = 0; ilstCur < vilstMax; ilstCur++)	/* search the LST */
    {
	if ((cbRead = read (vfnSym, &lstCur, cbLSTR)) == 0)
	    Panic ((nl_msg(577, "Internal Error (IE577)")));

	if (cbRead != cbLSTR)
	{
	    sprintf (vsbMsgBuf, (nl_msg(575, "Internal Error (IE575) (%d)")), cbRead);
	    Panic (vsbMsgBuf);
	}

	type = lstCur.lst_notype.type;

	if ( ((type == (long) LST_FUNC) OR (type == (long) LST_ENTRY))
	AND  (lstCur.lst_proc.code_start.ptr_segment == segIn)
	AND  ((offCur = lstCur.lst_proc.code_start.ptr_offset) <= offIn)
	AND  (offCur > offNearest))
	{					/* new best guess for it */
	    offNearest = offCur;		/* note best offset	 */
	    inpNearest = lstCur.lst_proc.name;	/* note best name	 */
	    if (offCur == offIn)		/* direct hit		 */
		break;				/* we can quit now	 */
	}
    } /* for */

    if (offNearest < 0)				/* nothing in same seg, even */
    {
	*psbLabel = vsbunknown;
	*pOffset  = 0;
    }
    else {					/* take best guess */
	*psbLabel = SbFInp (inpNearest);
	*pOffset  = offIn - offNearest;
	if (**psbLabel == '_')			/* skip leading "_" */
	    (*psbLabel)++;
    }
#endif /* FOCUS */

#endif /* not SPECTRUM */

} /* LabelFAdr */


/***********************************************************************
 * O P E N   S T A C K
 *
 * Set global values so the current stack frame is the one at the
 * given depth.
 */

void OpenStack (cnt)
    int		cnt;		/* stack depth to set */
{
    ADRT	pc = vpc;	/* current values */
#ifdef SPECTRUM
    ADRT	fp = vsp;
    ADRT	ap = vsp;
#else
    ADRT	fp = vfp;
    ADRT	ap = vap;
#endif

#ifdef INSTR
    vfStatProc[415]++;
#endif

#ifdef SPECTRUM
    while ((cnt--) AND pc)		/* look for the frame */
#else
    while ((cnt--) AND fp)		/* look for the frame */
#endif
	NextFrame (&fp, &ap, &pc);

#ifdef SPECTRUM
    if (pc == 0)
#else
    if (fp == 0)
#endif
	UError ((nl_msg(311, "Stack isn't that deep")));

#ifdef NOTDEF
    if (vfscreen_io) {
	PrintPos (pc, fmtNil);
    }
    else {
	PrintPos (pc, fmtFile + fmtProc);
    }
#endif

} /* OpenStack */


#ifdef HPSYMTABII
/***********************************************************************
 * F   H A S   A L T    E N T R I E S
 *
 * Returns true if ipd is an alternate entry, or is a procedure that
 * has alternate entries
 */

void FHasAltEntries(ipd)
int	ipd;
{

#ifdef INSTR
    vfStatProc[416]++;
#endif

    if (vrgPd[ipd].adrStart == vrgPd[ipd].adrEnd) {
       return(1);
    }
    else if ((ipd + 1 < vipdMac) &&
             (vrgPd[ipd + 1].adrStart == vrgPd[ipd + 1].adrEnd)) {
       return(1);
    }
    return(0);
}
#endif
