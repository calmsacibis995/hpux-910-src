/*****************************************************************************
** Copyright (c) 1986 - 1989 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: aioc.h,v 66.2 90/09/20 12:20:43 kb Exp $ */
#ifdef IX386
					/*** IX386 ***/
#ifdef OLD_VPIX
						/*** IX386 && OLD_VPIX ***/
#define AIOC		('A'<<8)
#define AIOCDOSMODE	(AIOC|1)	/* set DOSMODE */
#define AIOCNONDOSMODE	(AIOC|2)	/* reset DOSMODE */
#define AIOCINFO	(AIOC|3)	/* tell user what device we are */
#define AIOCINTTYPE	(AIOC|6)	/* set pseudorupt personality */
#define AIOCSETSS	(AIOC|7)	/* set start/stop chars */
						/*** END ***/
#else
						/*** IX386 && NOT OLD_VPIX ***/
#define AIOC	        ('A'<<8)
#define AIOCDOSMODE     (AIOC|61)	/* set DOSMODE */
#define AIOCNONDOSMODE  (AIOC|62)	/* reset DOSMODE */
#define AIOCINFO        (AIOC|66)	/* tell user what device we are */
#define AIOCINTTYPE     (AIOC|60)	/* set pseudorupt personality */
#define AIOCSETSS       (AIOC|65)	/* set start/stop chars */
						/*** END ***/
#endif
					/*** END IX386 ***/
#endif

/* xenix 386 already includes sys/machdep.h which has these */
