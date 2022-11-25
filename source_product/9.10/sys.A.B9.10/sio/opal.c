/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sio/RCS/opal.c,v $
 * $Revision: 1.2.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 19:47:20 $
 */

#include "../h/types.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/scsi.h"
#include "../h/acdl.h"
#include "../sio/autoch.h"
#include "../h/diskio.h"
#include "../h/errno.h"
#include "../h/param.h"
#include "../h/conf.h"
#include "../wsio/sds.h"

int
opal_open (dev, flag)
dev_t dev;
int flag;
{
   return sds_open(makedev(OPAL_MAJ_BLK, minor(dev)), flag);
}

int
opal_close(dev)
dev_t dev;
{
   return sds_close(makedev(OPAL_MAJ_BLK, minor(dev)));
}

int
opal_strategy(bp)
struct buf *bp;
{
   sds_strategy(bp);
}

int
opal_read(dev, uio)
dev_t dev;
struct uio *uio;
{
   return physio(opal_strategy, NULL, makedev(OPAL_MAJ_BLK, minor(dev)),
                 B_READ, minphys, uio);
}

int
opal_write(dev, uio)
dev_t dev;
struct uio *uio;
{
   return physio(opal_strategy, NULL, makedev(OPAL_MAJ_BLK, minor(dev)),
                 B_WRITE, minphys, uio);
}

int
opal_ioctl(dev, cmd, arg, flag)
dev_t dev;
int cmd;
int *arg;
int flag;
{
   int error;

   if (ac_surface(dev) == 0)
      return EINVAL;

   error = sds_open(dev, flag);

   if (!error)
      switch (cmd & IOCCMD_MASK) {
         case SIOC_INQUIRY & IOCCMD_MASK:
         case SIOC_CAPACITY & IOCCMD_MASK:
         case DIOC_DESCRIBE & IOCCMD_MASK:
         case DIOC_CAPACITY & IOCCMD_MASK:
         case DIOC_EXCLUSIVE & IOCCMD_MASK:
            error = sds_ioctl(dev, cmd, arg, flag);
            break;

         default:
            error = EINVAL;
      }

   sds_close(dev);

   return error;
}

void
opal_mount(dev, mount_ptr, pmount, punmount, reboot)
dev_t dev;
caddr_t mount_ptr;
int (*pmount) ();
int (*punmount) ();
int reboot;
{
}
