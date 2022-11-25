/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/version.c,v $
 * $Revision: 1.22.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:32:31 $
 */


#include "../objects/version.h"

/* global version number */
char *version= VERSIONID;

/* no version previous releases */
/* version 2.0  rel 12 */
/* version 2.1  rel 13 pre-release */
/* version 2.2  rel 13  */
/* version 2.3  rel 13.1 */
/* version 2.4  rel 13.1 , Semaphore code added under ifdef SEMA */
/* version A.B1.01       , 13.2 version */
/* version A.B1.10       , 14.0 networking version 13.2 (compatable) */
/* version A.B1.10.1     , 14.0 networking version 13.2 (compatable) */
/*                         with extra verbose hacks to ltor and pdir */
/*                         index routines in find.c                  */
/* version A.B1.10.2     , 14.0 networking version 13.2 (compatable) */
/*                         with net option command in networking     */
/* version A.B1.10.3     , Added setopt command and pte formatting   */
/*                         for regions    */
/* version A.B1.10.1B     , Release 1.1    (added Nflg and DUX/NFS   */
/*                                          merged)                  */
/* version A.B2.00.1B     , Release 2.0    (added additional networking */
/*                          capability, and network memory debugging */
/*                          (debbuging under -DNS_MBUF_QA 1 2 3 4 )  */
/* version A.B2.00.3B     , Release 2.0 F                             */
/* version A.B2.00.3B.1   , Release for final build rel 2 IC 3        */
