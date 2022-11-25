/*
 * @(#)un.h: $Revision: 1.11.83.4 $ $Date: 93/09/17 18:37:36 $
 * $Locker:  $
 */

#ifndef	_SYS_UN_INCLUDED
#define	_SYS_UN_INCLUDED

/* 
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)un.h	7.3 (Berkeley) 6/27/88
 */

/*
 * Definitions for UNIX IPC domain.
 */
struct	sockaddr_un {
	short	sun_family;		/* AF_UNIX */
	char	sun_path[92];		/* path name (gag) */
};

#ifdef _KERNEL
int	unp_discard();
#endif
#endif	/* _SYS_UN_INCLUDED */
