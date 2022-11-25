/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: restart.c,v 66.3 90/09/20 12:02:16 kb Exp $ */
#include "facetwin.h"
#include "fproto.h"
#include "restart.h"
int Saw_restart = 0;
restart()
{
	int			winno;
	char			*get_exec_list_ptr();
	struct facet_packet	Restart_pkt;
	int			result;

	for ( winno = 0; winno < USR_WINDS; winno++ )
	{
		if ( check_window_active( winno ) )
		{
			strncpy( Restart_pkt.pkt_args, 
				 get_exec_list_ptr( winno ), 
				 MAXPROTOARGS );
			Restart_pkt.pkt_args[ MAXPROTOARGS - 1 ] = '\0';
			Restart_pkt.pkt_header.cmd_byte = EXEC_LIST & 0xFF;
			Restart_pkt.pkt_header.pkt_window = winno;
			Restart_pkt.pkt_header.arg_cnt = MAXPROTOARGS;
			Restart_pkt.pkt_header.pkt_checksum = 0;
			result = fsend_packsend( winno, &Restart_pkt );
		}
	}
}
