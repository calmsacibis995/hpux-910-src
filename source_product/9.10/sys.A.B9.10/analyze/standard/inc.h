/*
 * @(#)inc.h: $Revision: 1.42.83.3 $ $Date: 93/09/17 16:31:21 $
 * $Locker:  $
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


#include "rel.h"
#ifdef BOBCAT
#include "../link/doio.h"
#endif

#ifdef BUILDFROMH
#include <stdio.h>
#include <h/types.h>
#define _KERNEL	/* hack */
#define KERNEL	/* hack */
#include <h/mp.h>
#include <h/param.h>
#undef _KERNEL
#undef KERNEL
#include <h/sysmacros.h>
#include <h/dir.h>
#include <h/conf.h>
#ifdef hp9000s800
#include <machine/pde.h>
#include <machine/crash.h>
#include <nlist.h>
#else
#include <machine/pte.h>
#endif
#include <setjmp.h>
#include <h/map.h>
#include <h/signal.h>
#include <h/user.h>
#include <h/proc.h>
#include <h/pfdat.h>
#include <h/var.h>
#include <h/vfd.h>
#include <h/vas.h>
#include <h/tuneable.h>
#include <h/sysinfo.h>
#include <h/swap.h>
#include <h/buf.h>
#include <h/vfs.h>
#include <h/time.h>
#include <h/vnode.h>
#include <h/dnlc.h>
#include <netinet/in.h>
#include <h/errno.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <nfs/nfs.h>
#include <nfs/nfs_clnt.h>
#include <nfs/rnode.h>
#undef rtov  /* conflict with <machine/vmparam.h> */
# include <dux/sitemap.h>
# include <ufs/inode.h>
#define _KERNEL	/* hack */
#define KERNEL	/* hack */
#include <h/mount.h>
#include <ufs/quota.h>
#undef _KERNEL
#undef KERNEL
#include <h/vmparam.h>
#include <h/vmmac.h>
#include <h/vmmeter.h>
#include <h/vmsystm.h>
#include <h/ipc.h>
#include <h/shm.h>
#include <h/pstat.h>
#ifdef hp9000s800
#include <machine/rpb.h>
#ifdef MP
#include <machine/cpu.h>
#endif
#endif
#include <h/devices.h>

#define KERNEL
#define _KERNEL
#include <dux/cct.h>
#include <dux/duxparam.h>
#include <dux/protocol.h>
#undef KERNEL
#undef _KERNEL
#include <dux/dm.h>
#include <h/dux_mbuf.h>
#include <dux/nsp.h>
#include <dux/selftest.h>
#include <dux/sitemap.h>

#include <h/file.h>

#include <h/malloc.h>
/* MEMSTATS */
#ifdef hp9000s800
#include <h/clist.h>
#else
#include <h/tty.h>
#endif

#else

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#define LONGFILENAMES
#include <sys/dir.h>
#ifdef hp9000s800
#include <machine/pde.h>
#include <machine/crash.h>
#include <nlist.h>
#else
#include <machine/pte.h>
#endif
#include <setjmp.h>
#include <sys/map.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/text.h>
#include <sys/pfdat.h>
#include <sys/var.h>
#include <h/vfd.h>
#include <h/vas.h>
#include <h/tuneable.h>
#include <h/sysinfo.h>
#include <h/swap.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/dnlc.h>
#include <netinet/in.h>
#include <nfs/nfs_clnt.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <nfs/nfs.h>
#include <nfs/rnode.h>
#undef rtov  /* conflict with <machine/vmparam.h> */
#include <sys/inode.h>
#include <mnttab.h>
#include <sys/mount.h>
#include <sys/vm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#ifdef hp9000s800
#include <machine/rpb.h>
#ifdef MP
#include <machine/cpu.h>
#include <h/kern_sem.h>
#endif
#endif

#include <sys/devices.h>
#define _KERNEL
#define KERNEL
#include <sys/cct.h>
#include <sys/duxparam.h>
#include <sys/protocol.h>
#include <ufs/quota.h>
#undef _KERNEL
#undef KERNEL
#include <sys/dm.h>
#include <sys/dux_mbuf.h>
#include <sys/nsp.h>
#include <sys/selftest.h>
#include <sys/sitemap.h>

#include <sys/file.h>
#include <sys/malloc.h>
#ifdef hp9000s800
#include <sys/clist.h>
#else
#include <sys/tty.h>
#endif
#endif
