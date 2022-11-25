/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: talk.h,v 70.1 92/03/09 15:47:15 ssa Exp $ */

	/******************************************************************
	* Receiver notifying that user pressed Hot-key ( Control-W ).
	* Lower byte of message is the Hot-key.
	******************************************************************/
#define FTPROC_WINDOW_KEY 0xEE00

	/******************************************************************
	* Receiver acknowledging "send_packet_signal" from sender that
	* requests control of the keyboard.
	******************************************************************/
#define FTPROC_WINDOW_PACKET 0xEF00

	/******************************************************************
	* Receiver notifying that user pressed Break and break is being 
	* sent to facetterm.
	******************************************************************/
#define FTPROC_WINDOW_WINDOW 0xED00

	/******************************************************************
	* Receiver canceling kestroke replay because characeter received
	* from keyboard.
	******************************************************************/
#define FTPROC_WINDOW_NOPLAY 0xEC00

	/******************************************************************
	* Receiver requesting report of screen-output activity since
	* last request.
	******************************************************************/
#define FTPROC_SCREEN_SAVER	0xEB00

	/******************************************************************
	* Receiver indicating initialization complete.
	******************************************************************/
#define FTPROC_WINDOW_READY	0xEA00

	/******************************************************************
	* Receiver notifying that user pressed No-prompt Hot-key.
	******************************************************************/
#define FTPROC_NO_PROMPT_WINDOW_KEY 0xE900
