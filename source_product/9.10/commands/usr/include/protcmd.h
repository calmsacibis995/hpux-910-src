/* @(#) $Revision: 66.2 $ */
#ifndef _PROTCMD_INCLUDED /* allows multiple inclusion */
#define _PROTCMD_INCLUDED

/* Copyright (c) 1988, SecureWare, Inc.
 *   All rights reserved
 *
 * This Module contains Proprietary Information of SecureWare, Inc.
 * and should be treated as Confidential.
 *
 * protcmd.h	2.5 10:47:19 9/22/89 SecureWare
 */

/*
 * Definitions for security-relevant routines of enhanced UNIX commands
 * that must be protected.
 */

#define AT_SEC_WRITEMODE	00220	/* Secure mode for creating files */
#define AT_SEC_ATMODE		06040	/* Secure mode for using files */
#define CRONTAB_SEC_CRMODE	0440	/* Secure mode for creating crontabs. */
#define LOGIN_PROGRAM		"/tcb/lib/login"
#define PASSWD_PROGRAM		"/tcb/bin/passwd"
#define INITCOND_PROGRAM	"/tcb/lib/initcond"
#define	DEV_LEADER		"/dev/"
#define	INIT_INITTAB_LOCATION	"/tcb/files/inittab"
#define	INIT_SU_LOCATION	"/tcb/bin/su"

#define	LP_LABEL_PREFIX		'L'
#define	LP_FILTER_PREFIX	'f'

#endif  /* _PROTCMD_INCLUDED */
