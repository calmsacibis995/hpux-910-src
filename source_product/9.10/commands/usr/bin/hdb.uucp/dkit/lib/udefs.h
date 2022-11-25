/*	@(#) $Revision: 64.1 $	*/

/*
 *		@(#)udefs.h	2.2 DKHOST 85/09/11
 *
 *	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */

#include	<setjmp.h>
#include	<pwd.h>
#include	<sys/stat.h>

	typedef jmp_buf	Tjmp;

#	define Mjmp	/* mos, mou */ Tjmp
#	define Ajmp	auto Tjmp
#	define Ejmp	extern Tjmp
#	define Rjmp	register Tjmp
#	define Sjmp	static Tjmp
#	define Pjmp	/* parameter, value */ Tjmp
#	define Xjmp	/* extdef */ Tjmp

	typedef struct passwd		*TpwP;

#	define MpwP	/* mos, mou */ TpwP
#	define ApwP	auto TpwP
#	define EpwP	extern TpwP
#	define RpwP	register TpwP
#	define SpwP	static TpwP
#	define PpwP	/* parameter, value */ TpwP
#	define XpwP	/* extdef */ TpwP

	typedef struct stat		Tstat, *TstatP;

#	define Mstat	/* mos, mou */ Tstat
#	define Astat	auto Tstat
#	define Estat	extern Tstat
#	define Rstat	register Tstat
#	define Sstat	static Tstat
#	define Pstat	/* parameter, value */ Tstat
#	define Xstat	/* extdef */ Tstat
#	define MstatP	/* mos, mou */ TstatP
#	define AstatP	auto TstatP
#	define EstatP	extern TstatP
#	define RstatP	register TstatP
#	define SstatP	static TstatP
#	define PstatP	/* parameter, value */ TstatP
#	define XstatP	/* extdef */ TstatP
