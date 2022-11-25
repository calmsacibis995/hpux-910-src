/* @(#) $Revision: 1.14.83.4 $ */      
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/dbg.h,v $
 * $Revision: 1.14.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:24:41 $
 */
#ifndef _SYS_DBG_INCLUDED /* allows multiple inclusion */
#define _SYS_DBG_INCLUDED
/*
 * Memory management debugging hooks.
 */
#define DBG_VMEMCHECK	0
#define DBG_VALVFDCHECK 1

/*
 * Process management debugging hooks.
 */
#define DBG_CHKRQS	10

/* 
 * The size of the debug flags array.
 */
#define DBG_NFLAGS	40

/*
 * The dbg macro and external variable declarations.
 */
#ifdef DBG
extern	int	dbgflags[DBG_NFLAGS];
#define dbg(which, proc) \
	if (dbgflags[which]) proc
#define dbgif(which, exp, proc) \
	if (dbgflags[which] && (exp)) proc
#else /* not DBG */
#define dbg(a,b) ;
#define dbgif(a,b,c) ;
#endif /* else not DBG */
#endif /* _SYS_DBG_INCLUDED */
