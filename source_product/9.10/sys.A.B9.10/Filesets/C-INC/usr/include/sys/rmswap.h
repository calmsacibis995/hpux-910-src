/*
 * @(#)rmswap.h: $Revision: 1.6.83.3 $ $Date: 93/09/17 16:45:48 $
 * $Locker:  $
 */

/* HPUX_ID: %W%		%E% */

/* 
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/

#ifndef _SYS_RMSWAP_INCLUDED
#define _SYS_RMSWAP_INCLUDED

/* Debugging information */
#define	DNONE	0
#define	DFUNC	1
#define	DARGS	2
#define	DRTVA	4
#define	DSPEC	8

extern	int	vm_debug;

/* Chunk allocation/deallocation message */
struct dux_vmmesg {		/*DUX MESSAGE STRUCTURE*/
	int swaptab;
};

extern	int	minswapchunks;			/* will keep at least this many
						   chunks of swap */
extern	int	maxswapchunks;			/* will keep at most this many
						   chunks of swap */
extern	site_t	my_site;
extern	site_t	swap_site;

#endif /* _SYS_RMSWAP_INCLUDED */
