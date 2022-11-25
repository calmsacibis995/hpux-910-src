/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)pathnames.h	5.1 (Berkeley) 4/19/90
 *      @(#)$Header: pathnames.h,v 1.1.109.6 95/02/21 16:08:18 mike Exp $
 */
# ifdef PATCH_STRING
/* static char *patch_3997="@(#) PATCH_9.03: pathnames.h $Revision: 1.1.109.6 $ 94/03/24 PHNE_3997"; */
# endif	/* PATCH_STRING */

# ifdef V4FS

# 	include <paths.h>

# 	define	_PATH_SENDMAILCF	"/etc/mail/sendmail.cf";
# 	define	_PATH_SENDMAILFC	"/etc/mail/sendmail.fc";
# 	define _PATH_MAILSTATS		"/etc/mail/sendmail.st"
# 	define _PATH_MAILALIASES	"/etc/mail/aliases"
# 	define _PATH_MAILREVALIASES	"/etc/mail/rev-aliases"
# 	define _PATH_MAILBOXDIR	"/var/mail"
# else	/* ! V4FS */
# 	define	_PATH_SENDMAILCF	"/usr/lib/sendmail.cf";
# 	define	_PATH_SENDMAILFC	"/usr/lib/sendmail.fc";
# 	define _PATH_MAILSTATS		"/usr/lib/sendmail.st"
# 	define _PATH_MAILALIASES	"/usr/lib/aliases"
# 	define _PATH_MAILREVALIASES	"/usr/lib/rev-aliases"
# 	define _PATH_MAILBOXDIR	"/usr/mail"
# endif	/* V4FS */
