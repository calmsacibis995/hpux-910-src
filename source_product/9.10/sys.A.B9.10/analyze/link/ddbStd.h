/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/link/RCS/ddbStd.h,v $
 * $Revision: 1.9.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:26:59 $
 */
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


#ifdef DDB
#include <sys/types.h>
#endif DDB

typedef	int		VOID, (*PFI)(), ERROR;

typedef	char		*TEXT, TINY;

#define		FOREVER		for(;;)
#define		FAST		register

#define		EXPORT
#define		IMPORT		extern

#undef		NULL
#define		NULL		0
#define		true		1
#define		false		0
#define		YES		1
#define		NO		0
#define		SUCCESS		1
#define		FAIL		-1
