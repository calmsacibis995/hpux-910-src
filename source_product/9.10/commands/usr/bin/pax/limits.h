/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/limits.h,v $
 *
 * $Revision: 66.1 $
 *
 * 	limits.h - POSIX compatible defnitions for some of <limits.h>
 *
 * DESCRIPTION
 *
 * 	We need to include <limits.h> if this system is being compiled with an 
 * 	ANSI standard C compiler, or if we are running on a POSIX confomrming 
 * 	system.  If the manifest constant _POSIX_SOURCE is not defined when 
 * 	<limits.h> is included, then none of the POSIX constants are defined 
 *	and we need to define them here.  It's a bit wierd, but it works.
 *
 * 	These values where taken from the IEEE P1003.1 standard, draft 12.
 * 	All of the values below are the MINIMUM values allowed by the standard.
 * 	Only those values used by PAX are included in this header.
 *
 * AUTHOR
 *
 *     Mark H. Colburn, Open Systems Architects, Inc. (mark@osa.com)
 *
 * COPYRIGHT
 *
 *	Copyright (c) 1989 Mark H. Colburn.  All rights reserved.
 *
 *	Redistribution and use in source and binary forms are permitted
 *	provided that the above copyright notice and this paragraph are
 *	duplicated in all such forms and that any documentation,
 *	advertising materials, and other materials related to such
 *	distribution and use acknowledge that the software was developed
 *	by Mark H. Colburn.
 *
 *	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 *	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 *	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _PAX_LIMITS_H
#define _PAX_LIMITS_H

/* Headers */

#ifdef LIMITSH
#   include <limits.h>
#endif /* LIMITSH */

#ifndef PATH_MAX
#define PATH_MAX	255	/* Max number of bytes in pathname */
#endif /* PATH_MAX */

#ifndef NAME_MAX
/* This should be 32 for BSD systems */
#define NAME_MAX	14	/* Max number of bytes in a filename */
#endif /* NAME_MAX */

#endif /* _PAX_LIMITS_H */
