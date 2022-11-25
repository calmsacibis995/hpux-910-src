/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: monitor.c,v 70.1 92/03/09 15:45:39 ssa Exp $ */
/**************************************************************************
* mon.c
*	Support profile monitoring.
**************************************************************************/
#ifdef MONITOR

#include <mon.h>
extern int main();
extern int etext();
#define MBUFFSIZE 50000

WORD	*Mon_buff = NULL;

long	Mon_len = 0;

/**************************************************************************
* start_mon
**************************************************************************/
start_mon()
{
	int	overhead;
	int	max;
	int	len;
	int	i;
	long	*malloc_run();

	overhead = ( sizeof( struct hdr ) + 3000 * sizeof( struct cnt ) )
			   / sizeof( WORD );
	max = MBUFFSIZE - overhead;
	len = (long) etext / sizeof( WORD );
	while( len > max )
		len >>= 1;
	Mon_len = len + overhead;
	if ( Mon_buff == NULL )
	{
		Mon_buff = ( WORD * ) malloc_run( Mon_len * sizeof( WORD ),
						  "monitor" );
		if ( Mon_buff == NULL )
			term_beep();
	}
	if ( Mon_buff != NULL )
	{
		for ( i = 0; i < Mon_len; i++ )
			Mon_buff[ i ] = 0;
		monitor( (int (*) () ) 2, etext, Mon_buff, Mon_len, 3000 );
	}
}
/**************************************************************************
* end_mon
**************************************************************************/
end_mon()
{
	monitor( (int (*)() )0, ( int (*)() )0, (WORD *)0, 0, 0 );
}
#endif
