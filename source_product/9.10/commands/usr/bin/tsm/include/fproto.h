/*****************************************************************************
** Copyright (c) 1986 - 1989 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fproto.h,v 66.2 90/09/20 12:21:22 kb Exp $ */
/*---------------------------------------------------------------------*\
| fproto.h                                                              |
|                                                                       |
| Facet protocol definitions.                                           |
\*---------------------------------------------------------------------*/

					/* Facet packet structure */
struct fpkt_header
{
	unsigned char cmd_byte;
	unsigned char pkt_window;
	unsigned char arg_cnt;
	unsigned char pkt_checksum;
};

struct facet_packet
{
	struct fpkt_header pkt_header;
	unsigned char pkt_args[MAXPROTOARGS];
};



					/* basic protocol values */

					/* Facet protocol lead-in character */
#define FP_LEADIN	0x7E
					/* window selection command mask */
#define WINSEL_MASK	0xF0
					/* mask to get window number */
#define WINDOW_MASK	0x0F
					/* select window command */
#define FP_WINSEL	0x20
					/* select window ack */
#define FP_WINACK	0x30
					/* mask for packet begin, end, ack */
#define PKT_MASK	0xF8
					/* mask for packet sequence number */
#define PKT_SEQ_MASK	0x07
					/* begin packet command */
#define FP_BEG_PKT	0x40
					/* end packet command */
#define FP_END_PKT	0x48
					/* ack begin packet */
#define FP_ACK_BEG_PKT	0x50
					/* ack end of packet */
#define FP_ACK_END_PKT	0x58
					/* negative ack - not used */
#define FP_NAK		0x60
					/* host requests facet logon */
#define FP_RQST_LOGIN	0x61
					/* login serial number was bad */
#define FP_BAD_LOGIN	0x62
					/* user typed control-q */
#define FP_QCONTROL	0x71
					/* user typed control-s */
#define FP_SCONTROL	0x73
					/* user typed control-q with 8th bit */
#define FP_QCTL8BIT	0x74
					/* user typed control-s with 8th bit */
#define FP_SCTL8BIT	0x75
					/* simulate hangup */
#define FP_HANGUP	0x76


					/* packet commands - these must not
					   conflict with the lower 8 bits of the
					   system call values defined in
					   windows.h */

					/* PC Facet is logging into the host
					   driver */
#define FACET_LOGIN	0xFF

					/* Positive and negative responses
					   to window system calls from PC
					   Facet */
#define FACET_POS_RESP	0xFE
#define FACET_NEG_RESP	0xFD

					/* Have facet host server run a program
					   in a window */
#define FACET_RUN_PROG		0xFC

					/* Have facet host server run the users
					   default shell in a window */
#define FACET_RUN_SHELL		0xFB

					/* Have facet host server tell us about
					   programs that are running */
#define FACET_RESTART		0xFA

					/* Error codes returned by PC Facet
					   which should be mapped into UNIX
					   errno values */

#define FE_INVAL	1
#define FE_NOENT	2
#define FE_EXIST	3
#define FE_NOSPC	4
#define FE_FAULT	5
#define FE_NOMEM	6
#define FE_NXIO		7
#define FE_BUSY		8

					/* Received packet available flags */
#define RPKT_AVAIL	1
#define RPKT_TIMEOUT	2

					/* Max number of seconds to wait for
					   an expected packet to be received */
#define RPKT_MAXWAIT	5
