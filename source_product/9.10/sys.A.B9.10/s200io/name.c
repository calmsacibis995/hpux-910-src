/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/name.c,v $
 * $Revision: 1.6.84.5 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/08 09:39:34 $
 */

#define UTSNAME_SYS "HP-UX"
#define UTSNAME_NODE "unknown"
#ifndef UTSNAME_REL
#define UTSNAME_REL "fix the makefile"
#endif
#define UTSNAME_VER "A"

/*
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


#include "../h/utsname.h"
#include "../h/param.h"

struct utsname utsname = {
	UTSNAME_SYS,
	UTSNAME_NODE,
	UTSNAME_REL,
	UTSNAME_VER
};

#ifndef HOSTNAME_NAME
#define HOSTNAME_NAME UTSNAME_NODE
#endif

char hostname[MAXHOSTNAMELEN+1] = HOSTNAME_NAME;
