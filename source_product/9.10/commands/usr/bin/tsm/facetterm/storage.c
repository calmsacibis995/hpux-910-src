/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: storage.c,v 70.1 92/03/09 15:47:04 ssa Exp $ */
/**************************************************************************
* storage.c
*	Localize storage functions to minimize lint complaint and
*	centralize error messages.
**************************************************************************/
#include <sys/types.h>

#include <malloc.h>

#include <fcntl.h>

#include "options.h"

long	Old_malloc_ptr = 0;
/**************************************************************************
* mymalloc
*	Allocate "bytes" bytes, using "operation" as an error message.
**************************************************************************/
long *
mymalloc( bytes, operation )
	int	bytes;
	char	*operation;
{
	long	*ptr;
	char	buff[ 80 ];

	ptr = (long *) malloc( bytes );
	if ( ptr == ( (long *) 0 ) )
	{
		printf( "ERROR: %s malloc failed\n", operation );
		wait_return_pressed();
		exit( 1 );
	}
	else
	{
		if ( Opt_malloc_record )
		{
			sprintf( buff,
				"MYMALLOC:   %-20.20s %10d %10ld %10lx %10ld\n",
				 operation, bytes, (long) ptr, (long) ptr,
				 (long) ptr - Old_malloc_ptr );
			record_malloc( buff );
			Old_malloc_ptr = (long) ptr;
		}
	}
	return( ptr );
}
/**************************************************************************
* malloc_run
*	Allocate "bytes" bytes, using "operation" as an error message.
**************************************************************************/
long *
malloc_run( bytes, operation )
	int	bytes;
	char	*operation;
{
	long	*ptr;
	char	buff[ 80 ];

	ptr = (long *) malloc( bytes );
	if ( ptr == ( (long *) 0 ) )
	{
		sprintf( buff, "ERROR: %s malloc failed\n", operation );
		record_malloc( buff );
	}
	else
	{
		if ( Opt_malloc_record )
		{
			sprintf( buff,
				"MALLOC_RUN: %-20.20s %10d %10ld %10lx %10ld\n",
				 operation, bytes, (long) ptr, (long) ptr,
				 (long) ptr - Old_malloc_ptr );
			record_malloc( buff );
			Old_malloc_ptr = (long) ptr;
		}
	}
	return( ptr );
}
/**************************************************************************
* link_last
*	Link a new terminal description file capability structure "new"
*	to the end of the chain pointed to by "base".
**************************************************************************/
#include "linklast.h"
link_last( new, base )
	T_STRUCT	*new;
	T_STRUCT	*base;
{
	T_STRUCT	*t_struct_ptr;

	t_struct_ptr = base;
	while( t_struct_ptr->next != (T_STRUCT *) 0 )
	{
		t_struct_ptr = t_struct_ptr->next;
	}
	t_struct_ptr->next = new;
	new->next = (T_STRUCT *) 0;
}
#ifdef lint
static int lint_alignment_warning_ok_1;
#endif
int	Malloc_fd = 0;
record_malloc( buff )
	char	*buff;
{
	if ( Malloc_fd == 0 )
	{
		Malloc_fd = open( "/tmp/ft_malloc",
				  O_WRONLY | O_APPEND | O_CREAT, 0666 );
		write( Malloc_fd, "====\n", 5 );
	}
	if ( Malloc_fd <= 0 )
		return;
	write( Malloc_fd, buff, strlen( buff ) );
}
