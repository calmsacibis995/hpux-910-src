/*
 * @(#)ntp_syslog.h: $Revision: 1.2.109.1 $ $Date: 94/10/26 16:55:16 $
 * $Locker:  $
 */

/* ntp_syslog.h,v 3.1 1993/07/06 01:06:59 jbj Exp
 * A hack for platforms which require specially built syslog facilities
 */
#ifdef GIZMO
#include "gizmo_syslog.h"
#else /* !GIZMO */
#include <syslog.h>
#endif /* GIZMO */
