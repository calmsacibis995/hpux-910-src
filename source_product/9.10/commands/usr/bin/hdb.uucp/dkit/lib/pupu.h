/*	@(#) $Revision: 66.1 $	*/

/*
 *		@(#)pupu.h	2.4 DKHOST 87/02/06
 *
 *	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/sysmacros.h>
#include	<sys/param.h>
#include	<sysexits.h>
#include	<signal.h>
#include	"tdefs.h"
#include	"udefs.h"

#undef DEBUG

#ifdef DEBUG
#define db	fprintf
#else
#define db	if(0)fprintf
#endif

#ifndef MAXFUPLIM
#define MAXFUPLIM 2048
#endif

#define MAXFD MAXFUPLIM-3 /* Absolute maximum number of open fd's - 3 */

/*
** Version numbers exchanged between local and remote.
**
** PPLEVEL1 - Original protocol version.
** PPLEVEL2 - Enhanced to return errorcodes on fatal remote errors.
*/

#define	PPLEVEL1	"1"	/* Original version			*/
#define	PPLEVEL2	"2"	/* Fixes remote error status problem	*/

/* End of Version Numbers */

#define	NIL		((TcharP) 0)
#define GOOD		0
#define BAD		1

#define	WTIMEOUT	90
#define	RTIMEOUT	60

#define	NAMELEN		512	/* maximum path name length */
#define BLOCKSIZE	2048	/* write length for file transfers */

	typedef struct expect {
		Mchar	type;
		MfnP	fn;
	} Texp, *TexpP;

#	define Mexp	/* mos, mou */ Texp
#	define Aexp	auto Texp
#	define Eexp	extern Texp
#	define Rexp	register Texp
#	define Sexp	static Texp
#	define Pexp	/* parameter, value */ Texp
#	define Xexp	/* extdef */ Texp
#	define MexpP	/* mos, mou */ TexpP
#	define AexpP	auto TexpP
#	define EexpP	extern TexpP
#	define RexpP	register TexpP
#	define SexpP	static TexpP
#	define PexpP	/* parameter, value */ TexpP
#	define XexpP	/* extdef */ TexpP

	typedef struct linkentry	Tlink, *TlinkP;

#	define Mlink	/* mos, mou */ Tlink
#	define Alink	auto Tlink
#	define Elink	extern Tlink
#	define Rlink	register Tlink
#	define Slink	static Tlink
#	define Plink	/* parameter, value */ Tlink
#	define Xlink	/* extdef */ Tlink
#	define MlinkP	/* mos, mou */ TlinkP
#	define AlinkP	auto TlinkP
#	define ElinkP	extern TlinkP
#	define RlinkP	register TlinkP
#	define SlinkP	static TlinkP
#	define PlinkP	/* parameter, value */ TlinkP
#	define XlinkP	/* extdef */ TlinkP

	struct linkentry {
		MlinkP	next;
		Mshort	count;
		ino_t	ino;
		dev_t	dev;
		Mchar	name[2];	/* really longer */
	};

#define	H_MAG1	0
#define H_MAG2	1
#define H_SEQN	2
#define		SEQMIN	1
#define		SEQMAX	4
#define	H_TYPE	3
#define H_NULL	4
#define	H_DATA	5

	Eint	cfd;

	Echar	mhead[H_DATA];
	Echar	buf[BLOCKSIZE];
	EcharP	mnext;
	Echar	sseq, rseq;

	Ejmp	giveup;
	EfnP	alarmwas;

	Echar	wdir[NAMELEN];
	Echar	basedir[NAMELEN];
	Echar	wholename[NAMELEN];

	Eint	cfd;
	EfnP	errorfunction;
	Eint	lasterror;
	EcharP	remoteorlocal;
	EcharP	currentfile, timeoutmsg;

	extern unsigned char	openfiles[];

	Elink	linkhead;

	Eint	sys_nerr;
	EcharP	sys_errlist[];

	Eexp	inittable[];
