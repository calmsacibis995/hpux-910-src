/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fct_ioctl.c,v 70.1 92/03/09 15:42:15 ssa Exp $ */

/**************************************************************************
* fct_ioctl
*	Communication between applications running on windows and facetterm
*	is done with ioctls on the driver and HP ptys.
*	Other pseudo ttys do not allow ioctls through the ptys and require
*	another mechanism.  This module decides (with IFDEFS) and implements
*	the other mechanism.
**************************************************************************/

#include "ptydefs.h"

fct_ioctl( fd, request, buffer )
	int	fd;
	int	request;
	char	*buffer;
{
	return( ioctl( fd, request, buffer ) );
}
fct_ioctl_reset()
{
}
fct_ioctl_close()
{
}
