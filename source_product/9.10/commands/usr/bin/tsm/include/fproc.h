/*****************************************************************************
** Copyright (c) 1986 - 1989 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fproc.h,v 70.1 92/03/09 15:48:10 ssa Exp $ */
/*---------------------------------------------------------------------*\
| fproc.h                                                               |
|                                                                       |
| Ioctl values for /dev/facet.                                          |
\*---------------------------------------------------------------------*/

#ifdef OVERRIDE_CLSIZE
#undef CLSIZE
#define CLSIZE OVERRIDE_CLSIZE
#endif

#define WIOCF	('W' << 8)
					/* ioctl to get into Facet processing */
#define FPROC_KILL_SECONDS	(WIOCF | 0xF0)
#define FPROC_SIGNAL_WINDOW	(WIOCF | 0xF1)
#define FPROC_HANGUP		(WIOCF | 0xF2)
#define FPROC_TEST		(WIOCF | 0xF3)
#define FPROC_BREAK		(WIOCF | 0xF4)
#define FPROC_WINDOW_TERMIO	(WIOCF | 0xF5)
#define FPROC_OFFCARRIER	(WIOCF | 0xF6)
#define FPROC_FROMWINS		(WIOCF | 0xF7)
#define FPROC_FROMACKS		(WIOCF | 0xF8)
#define FPROC_ONCARRIER		(WIOCF | 0xF9)
#define FPROC_SNDPKTRESULT	(WIOCF | 0xFA)
#define FPROC_TOWINS		(WIOCF | 0xFB)
#define FPROC_TOACKS		(WIOCF | 0xFC)
#define FPROC_SERMATCH		(WIOCF | 0xFD)
#define FPROC_SETRECVPKT	(WIOCF | 0xFE)
#define FPROC_FROMWINS_MASK	(WIOCF | 0xEF)
#define FPROC_FROMWINS_CHECK	(WIOCF | 0xEE)
#define FPROC_ALLOW_TABS	(WIOCF | 0xED)
#define FPROC_WINDOWS_ACTIVE	(WIOCF | 0xEC)

					/* special Facet ioctl's which are
					   internal to Facet and not part of
					   the programmer's interface */

#define FCT_MAX_ACKCHARS	32
struct fromwins_struct
{
	int		seconds;	/* seconds to wait on fromacks */
	int		ackcount;
	unsigned int	ackchars[ FCT_MAX_ACKCHARS ];
	int		wincharcount[ USR_WINDS ];
	unsigned char	winchars[ USR_WINDS ][ CLSIZE ];
	int		packet_window;
	struct facet_packet	packet;
	int		winmask;	/* bit mask for windows win0=1 */
};
extern struct fromwins_struct Fromwins;

struct towins_struct
{
	int		window;
	int		wincharcount;
	unsigned char	winchars[ CLSIZE ];
};
extern struct towins_struct Towins;

struct signal_window_struct
{
	int		window;
	int		signal_to_send;
};
