/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/REG.300/RCS/debug.nms.c,v $
 * $Revision: 1.6.84.4 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/06 11:25:00 $
 */

#ifdef OSDEBUG

#include "../h/types.h"
#include "../h/sema.h"
#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/errno.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/vas.h"
#include "../h/debug.h"
#include "../h/vnode.h"
#include "../h/tty.h"
#include "../h/pregion.h"
#include "../h/pfdat.h"
#include "../h/vmmeter.h"
#include "../h/file.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../h/mount.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#endif QUOTA

#include "../h/vdma.h"

#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/mbuf.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../h/un.h"
#include "../h/unpcb.h"
#include "../h/hpibio.h"
#include "../wsio/iobuf.h"

int assertions = 1;

#endif /* OSDEBUG */
