/* @(#) $Header: sds.c,v 1.1.83.5 93/10/27 12:11:28 drew Exp $ */

/********************************* sds.c *************************************/
/*                                                                           */
/* This file implements a software disk striping driver which sits between   */
/* the filesystem (or any other service) and a physical disk driver.         */
/*                                                                           */
/* The main routines are:                                                    */
/*                                                                           */
/*   sds_add        - add the specified volume descriptor to the list of     */
/*                    volume descriptors for open volumes                    */
/*   sds_remove     - remove the specified volume descriptor from the        */
/*                    list of volume descriptors for open volumes            */
/*   sds_lookup     - look up the volume descriptor for the specified dev    */
/*                    in the list of volume descriptors for open volumes     */
/*                                                                           */
/*   sds_config     - perform a first-open on the volume for the specified   */
/*                    dev                                                    */
/*   sds_unconfig   - perform a last-close on the specified volume           */
/*                                                                           */
/*   sds_stripe     - split out and initiate one stripe's worth of I/O to    */
/*                    the specified volume                                   */
/*   sds_unstripe   - conclude and coalesce back one stripe's worth of I/O   */
/*                    to a volume                                            */
/*                                                                           */
/*   sds_open       - striping driver interface routines...                  */
/*   sds_close      -                                                        */
/*   sds_strategy   -                                                        */
/*   sds_ioctl      -                                                        */
/*   sds_read       -                                                        */
/*   sds_write      -                                                        */
/*   sds_size       -                                                        */
/*                                                                           */
/* The sds_size, sds_open, and sds_close function hierarchy look like:       */
/*                                                                           */
/*                                SDS_SIZE                                   */
/*                                   |                                       */
/*                   +---------------+--------------+                        */
/*                   |               |              |                        */
/*               SDS_OPEN       sds_lookup       SDS_CLOSE                   */
/*                   |                              |                        */
/*       +-----------+---------+          +---------+------------+           */
/*       |           |         |          |         |            |           */
/*  sds_lookup  sds_config  sds_add  sds_lookup sds_unconfig sds_remove      */
/*                   |                              |                        */
/*           +-------+-----+                        |                        */
/*           |             |                        |                        */
/*      s2disk_open   s2disk_ioctl              s2disk_close                 */
/*                                                                           */
/*                                                                           */
/* The sds_ioctl function hierarchy looks like:                              */
/*                                                                           */
/*                               SDS_IOCTL                                   */
/*                                   |                                       */
/*                            +------+-------+                               */
/*                            |              |                               */
/*                        sds_lookup   s2disk_ioctl                          */
/*                                                                           */
/*                                                                           */
/* The sds_read, sds_write, sds_strategy, and sds_unstripe function          */
/* hierarchy look like:                                                      */
/*                                                                           */
/*                        SDS_READ       SDS_WRITE                           */
/*                            |              |                               */
/*                            +------+-------+                               */
/*                                   |                                       */
/*                                   |                                       */
/*                                physio                                     */
/*                                   |                                       */
/*                                   |                                       */
/*                              SDS_STRATEGY                                 */
/*                                   |                                       */
/*                           +-------+-------+                               */
/*                           |               |                               */
/*                      sds_lookup       sds_stripe                          */
/*                                           |                               */
/*                                           |                               */
/*                                     s2disk_strategy                       */
/*                                           |                               */
/*                                           |                               */
/*                                        biodone                            */
/*                                           |                               */
/*                                           |                               */
/*                                     SDS_UNSTRIPE                          */
/*                                                                           */
/*****************************************************************************/

#include "../h/types.h"
#include "../h/malloc.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/scsi.h"
#include "../h/cs80.h"
#include "../h/diskio.h"
#include "../h/errno.h"
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/sysmacros.h"
#include "../h/file.h"

#include "sds.h"

#ifdef __hp9000s300
#define MAXPHYS (64 * 1024)
#endif

extern void sds_unstripe();
extern void sds_strategy();
extern unsigned minphys();
extern int sds_get_info();


/******************************** volumeList *********************************/
/*                                                                           */
/* This pointer is the base of a linked list of Volume structures.  When an  */
/* SDS array volume is opened for the first time, its Volume structure is    */
/* allocated, filled in, and added to the head of this list.  When the SDS   */
/* array volume is finally closed (last close), its Volume structure is      */
/* removed from this list and deallocated.                                   */
/*                                                                           */
/* Note that for most devices, there is a shared Volume entry for both       */
/* block and character opens.  For optical, however, there is a separate     */
/* one for each, which complicates DIOC_EXCLUSIVE and sds_lookup()           */
/* processing.                                                               */
/*                                                                           */
/*****************************************************************************/

struct Link volumeList;


/******************************* sds_log2() **********************************/
/*                                                                           */
/* This routine returns the log base 2 of the specified number.              */
/*                                                                           */
/*****************************************************************************/

unsigned sds_log2(i)
unsigned i;
{
  unsigned l;

  l = 0;
  while (i >>= 1) {
    l++;
  }
  return l;
}


/****************************** sds_empty() **********************************/
/*                                                                           */
/* This routine initializes a linked list to be empty.                       */
/*                                                                           */
/*****************************************************************************/

/* VARARGS */
void sds_empty(list)
struct Link *list;
{
  list->next = list;
  list->prev = list;
}


/******************************* sds_add() ***********************************/
/*                                                                           */
/* This routine adds a node to the tail of a linked list.                    */
/*                                                                           */
/*****************************************************************************/

/* VARARGS */
void sds_add(list, node)
struct Link *list;
struct Link *node;
{
  int x;

  x = CRIT();
  node->prev = list->prev;
  list->prev->next = node;
  node->next = list;
  list->prev = node;
  (void)UNCRIT(x);
}


/******************************** sds_remove() *******************************/
/*                                                                           */
/* This routine removes a node from a linked list.                           */
/*                                                                           */
/*****************************************************************************/

/* VARARGS */
void *sds_remove(node)
struct Link *node;
{
  int x;

  x = CRIT();
  node->prev->next = node->next;
  node->next->prev = node->prev;
  (void)UNCRIT(x);
  return node;
}


/******************************* sds_next() **********************************/
/*                                                                           */
/* This routine returns a pointer to the next node in a linked list.         */
/*                                                                           */
/*****************************************************************************/

/* VARARGS */
void *sds_next(list, node)
struct Link *list;
struct Link *node;
{
  struct Link *next;

  next = node->next;
  if (next != list) {
    return next;
  }
  return NULL;
}


/******************************* sds_head() **********************************/
/*                                                                           */
/* This routine returns a pointer to the head node in a linked list.         */
/*                                                                           */
/*****************************************************************************/

/* VARARGS */
void *sds_head(list)
struct Link *list;
{
  return sds_next(list, list);
}


/***************************** sds_remove_head() *****************************/
/*                                                                           */
/* This routine removes the head node from a linked list.                    */
/*                                                                           */
/*****************************************************************************/

/* VARARGS */
void *sds_remove_head(list)
struct Link *list;
{
  int x;
  void *node;

  x = CRIT();
  node = sds_head(list);
  if (node) {
    (void)sds_remove(node);
  }
  (void)UNCRIT(x);
  return node;
}


/***************************** sds_lookup() **********************************/
/*                                                                           */
/* This routine searches the volumeList for a Volume structure whose SDS     */
/* array volume *minor* number matches the specified minor number.  If one   */
/* is found (i.e., the SDS array volume was already open), it's pointer      */
/* is returned.  Otherwise (i.e., the SDS array volume has not been opened   */
/* yet), NULL is returned.                                                   */
/*                                                                           */
/*****************************************************************************/

struct Volume *sds_lookup(dev)
dev_t dev;
{
  struct Volume *v;

  for (v = sds_head(&volumeList); v; v = sds_next(&volumeList, v)) {
    if ((major(dev) == OPAL_MAJ_BLK || major(dev) == OPAL_MAJ_CHR)) {
      if (v->device.dev == dev) {
        return v;
      }
    } else {
      if (minor(v->device.dev) == minor(dev)) {
        return v;
      }
    }
  }
  return NULL;
}


/****************************** sds_config() *********************************/
/*                                                                           */
/* This routine is called to perform a "first open" on the specified SDS     */
/* array volume.  It fills in the already allocated Volume structure         */
/* describing the SDS array volume.                                          */
/*                                                                           */
/*****************************************************************************/

int sds_config(v, dev)
struct Volume *v;
dev_t dev;
{
  int i;
  int bdev;
  int cdev;
  int block;
  dev_t bad;
  int error;
  int partition;
  struct buf *sbp;
  union Device lead;
  sds_info_t array;
  sds_section_t *section;
  disk_describe_type describe;

  /* then fill in the SDS array device number and the physical device */
  /* count (used if we have to bail out early). */
  v->device.dev = dev;
  v->deviceCount = 0;

  /* figure out the block and character major numbers to access the */
  /* physical SDS array devices -- also get the SDS partition number */
  block = 0;
  switch (v->device.field.majorNumber) {
    case OPAL_MAJ_BLK:
      block = 1;
    case OPAL_MAJ_CHR:
      bdev = AC_MAJ_BLK;
      cdev = AC_MAJ_CHR;
      partition = 1;
      break;

    case CS80_MAJ_BLK:
      block = 1;
    case CS80_MAJ_CHR:
      bdev = CS80_MAJ_BLK;
      cdev = CS80_MAJ_CHR;
      partition = v->device.field.partition;
      break;

    case SCSI_MAJ_BLK:
      block = 1;
    case SCSI_MAJ_CHR:
      bdev = SCSI_MAJ_BLK;
      cdev = SCSI_MAJ_CHR;
      partition = v->device.field.partition;
      break;

    default:
      goto sds_config_error;
  }

  /* set up our pointers to access physical driver routines */
  v->bdevsw = &bdevsw[bdev];
  v->cdevsw = &cdevsw[cdev];

  /* then identify the lead device of the SDS array */
  lead.dev = dev;
  lead.field.majorNumber = bdev;
  if (bdev != AC_MAJ_BLK) {
    lead.field.partition = 0;
  }

  /* then read the SDS_INFO array configuration data from it */
  error = sds_get_info(lead.dev, &array, &bad);
  if (error != SDS_OK) {
    msg_printf("sds_config: cannot read SDS_INFO for 0x%08x\n", v->device.dev);
    msg_printf("            (error %d on device 0x%08x)\n", error, bad);
    goto sds_config_error;
  }

  /* and then find the appropriate SDS partition information */
  section = &array.section[partition-1];

  /* then make sure the requested SDS partition exists */
  if (section->size == 0) {
    msg_printf("sds_config: partition %d not defined for 0x%08x\n", partition,
               v->device.dev);
    goto sds_config_error;
  }

  /* then open the individual physical devices in the SDS array */
  for (i = 0; i < array.ndevs; i++) {
    v->devices[i].dev = array.devs[i];
    v->devices[i].field.majorNumber = (block)?(bdev):(cdev);

    if ((v->bdevsw->d_open)(v->devices[i].dev, 0)) {
      msg_printf("sds_config: cannot open 0x%08x.\n", v->devices[i].dev);
      goto sds_config_error;
    }

    v->deviceCount++;
  }

  /* and get the SCSI block size from the first device */
  if ((v->cdevsw->d_ioctl)(v->devices[0].dev, DIOC_DESCRIBE, &describe, 0)) {
    msg_printf("sds_config: cannot ioctl 0x%08x.\n", v->devices[0].dev);
    goto sds_config_error;
  }

  /* then fill in the Volume label and type information */
  strcpy(v->label, array.label);
  strcpy(v->type, array.type);

  /* and the Volume SCSI block size information */
  v->blockSize = describe.lgblksz;
  v->log2BlockSize = sds_log2(v->blockSize);

  /* and the Volume stripe size information */
  v->stripeSize = (array.ndevs == 1)?(MAXPHYS):(section->stripesize);
  v->log2StripeSize = sds_log2(v->stripeSize);

  /* and the Volume size and offset information */
  v->volumeSize = section->size >> v->log2BlockSize;
  v->physOffset = section->start;

  /* set the Volume open count to 0 */
  v->openCount = 0;

  /* and then preallocate bufs for striping on the ICS */
  sds_empty(&v->freeBufs);
  v->freeBufMax = 2*(MAXPHYS/v->stripeSize+1);
  v->freeBufCount = 0;
  while (v->freeBufCount < v->freeBufMax) {
    MALLOC(sbp, struct buf *, sizeof(*sbp), M_IOSYS, M_ALIGN);
    bzero(sbp, sizeof(*sbp));
    sds_add(&v->freeBufs, sbp);
    v->freeBufCount++;
  }
  sds_empty(&v->waitingBufs);

  return 0;

sds_config_error:

  /* close any devices in the SDS array which we had already opened */
  for (i = 0; i < v->deviceCount; i++) {
    (v->bdevsw->d_close)(v->devices[i].dev);
  }

  /* and return -1 to fail the open */
  return -1;
}


/******************************* sds_unconfig() ******************************/
/*                                                                           */
/* This routine is called to perform a "last close" on the specified SDS     */
/* array volume.                                                             */
/*                                                                           */
/*****************************************************************************/

void sds_unconfig(v)
struct Volume *v;
{
  int i;
  struct buf *sbp;

  /* close all devices in the SDS array which we had opened */
  for (i = 0; i < v->deviceCount; i++) {
    (v->bdevsw->d_close)(v->devices[i].dev);
  }

  /* then free the preallocated secondary bufs */
  while (sbp = sds_remove_head(&v->freeBufs)) {
    v->freeBufCount--;
    FREE(sbp, M_IOSYS);
  }
}


/******************************* sds_stripe() ********************************/
/*                                                                           */
/* This routine is the heart of the SDS driver.  Given a Volume structure    */
/* and an unstriped buf, it performs one stripe's worth of I/O at the        */
/* specified disk offset and buffer address for up to the specified length.  */
/* It then returns the number of bytes included in the stripe.               */
/*                                                                           */
/*****************************************************************************/

unsigned sds_stripe(v, bp, offset, length, addr, reserved)
struct Volume *v;
struct buf *bp;
unsigned offset;
unsigned length;
char *addr;
struct Link *reserved;
{
  unsigned bcount;
  unsigned stripeFirst;
  unsigned stripeLast;
  struct buf *sbp;

  /* allocate a secondary buf to perform one stripe's worth of I/O. */
  sbp = sds_remove_head(reserved);
  bzero(sbp, sizeof(*sbp));

  /* compute the numbers of the stripes containing the beginning and */
  /* end of the specified I/O request */
  stripeFirst = offset >> v->log2StripeSize;
  stripeLast = (offset+length-1) >> v->log2StripeSize;

  /* if the specified I/O request length is bigger than one stripe, */
  /* then prune it back to one stripe. */
  if (stripeFirst != stripeLast) {
    bcount = ((stripeFirst+1) << v->log2StripeSize) - offset;
  } else {
    bcount = length;
  }

  /* then fill in the fields of the secondary buf to perform the */
  /* stripe's worth of I/O -- set up to have the routine sds_unstripe */
  /* called when the stripe's I/O completes */
  sbp->b_flags = (bp->b_flags | B_CALL) & ~B_ERROR & ~B_FSYSIO;
  sbp->b_bptype = bp->b_bptype;  /* FSD_KI */
  sbp->b_dev = v->devices[stripeFirst % v->deviceCount].dev;
  sbp->b_offset = v->physOffset +
                  (stripeFirst/v->deviceCount << v->log2StripeSize) +
                  (offset & (v->stripeSize-1));
  sbp->b_blkno = (unsigned)sbp->b_offset >> DEV_BSHIFT;  /* FSD_KI */
  sbp->b_spaddr = bp->b_spaddr;
  sbp->b_un.b_addr = addr;
  sbp->b_bcount = bcount;
  sbp->b_forw = bp;
  sbp->b_iodone = (int (*)())sds_unstripe;

  /* then process the stripe */
  (v->bdevsw->d_strategy)(sbp);

  /* and return the number of bytes it included */
  return bcount;
}


/***************************** sds_unstripe() ********************************/
/*                                                                           */
/* This routine is called by biodone() when a stripe's I/O completes.  It    */
/* merges the stripe's completion status back into that of the original      */
/* unstriped buf.                                                            */
/*                                                                           */
/*****************************************************************************/

void sds_unstripe(sbp)
struct buf *sbp;
{
  struct buf *bp;
  struct Volume *v;

  /* get a pointer to the unstriped buf */
  bp = sbp->b_forw;

  /* subtract the number of bytes included in the stripe from the */
  /* unstriped buf's residual count */
  bp->b_resid -= sbp->b_bcount - sbp->b_resid;

  /* if the stripe had an error, propogate it back to the unstriped buf */
  if (sbp->b_flags & B_ERROR) {
    bp->b_flags |= B_ERROR;
    bp->b_error = sbp->b_error;
  }

  /* get the Volume structure for the SDS array volume */
  v = sds_lookup(bp->b_dev);

  /* then return the secondary buf to our preallocated supply */
  sds_add(&v->freeBufs, sbp);
  v->freeBufCount++;

  /* then check if this is the last stripe of the I/O request -- if */
  /* so, call iodone() on the unstriped buf and then start any new */
  /* requests which were waiting for bufs.  then wake up anyone waiting */
  /* on last close */
  if (! --bp->b_ba) {
    iodone(bp);

    if (sds_head(&v->waitingBufs)) {
      sds_strategy(NULL, v);
    }

    if (v->flags.busy) {
      wakeup(v);
    }
  }
}


/******************************* sds_init() **********************************/
/*                                                                           */
/* This routine performs SDS module initialization prior to the first open.  */
/*                                                                           */
/*****************************************************************************/

void sds_init()
{
  struct buf buf;
  struct Link link;

  /* make sure our Link structure has pointers in the same place as a buf */
  if ((int)&buf.av_forw-(int)&buf != (int)&link.next-(int)&link ||
      (int)&buf.av_back-(int)&buf != (int)&link.prev-(int)&link) {
    panic("sds_init: bad structure offset");
  }

  /* initialize the open SDS array volume list to empty */
  sds_empty(&volumeList);
}


/******************************* sds_open() **********************************/
/*                                                                           */
/* This routine opens the specified SDS array volume.                        */
/*                                                                           */
/*****************************************************************************/

int sds_open(dev, flag)
dev_t dev;
int flag;  /* unused */
{
  struct Volume *v;
  struct Volume *nv;
  static unsigned initialized;

  /* perform SDS module initialization on the first open ever */
  if (! initialized) {
    initialized = 1;
    sds_init();
  }

  /* for optical devices, we need a special check to see if either */
  /* the raw or block device has already been opened DIOC_EXCLUSIVE */
  /* (because there is a separate Volume struct for each) -- return */
  /* an error if so */
  if (major(dev) == OPAL_MAJ_BLK || major(dev) == OPAL_MAJ_CHR) {
    if ((v = sds_lookup(minor(dev))) && v->flags.locked) {
      return EBUSY;
    }
  }

  /* allocate a new Volume struct in case we need it -- we don't want */
  /* to have to sleep later (we will free it if we don't need it) */
  MALLOC(nv, struct Volume *, sizeof(*nv), M_IOSYS, M_ALIGN);

  /* then wait if this SDS array volume is already in the process of */
  /* being configured */
  while ((v = sds_lookup(dev)) && v->flags.busy) {
    sleep(v, PRIBIO);
  }

  /* if this is a first open, set up the new Volume struct as "busy" */
  /* and add it to the volume list to hold off any other processes which */
  /* might want it */
  if (! v) {
    bzero(nv, sizeof(*nv));
    nv->device.dev = dev;
    nv->flags.busy = 1;
    v = nv;
    sds_add(&volumeList, v);

    /* then get the SDS array config info */
    if (sds_config(v, dev) < 0) {
      (void)sds_remove(v);
      FREE(v, M_IOSYS);
      wakeup(v);
      return ENXIO;
    }

    /* then let any processes which were waiting on this volume proceed */
    v->flags.busy = 0;
    wakeup(v);

  /* otherwise, we don't need the Volume struct we just allocated */
  } else {
    FREE(nv, M_IOSYS);

    /* if the volume is locked, return an error */
    if (v->flags.locked) {
      return EBUSY;
    }
  }

  /* then increment the SDS array volume open count */
  v->openCount++;

  return 0;
}


/******************************* sds_close ***********************************/
/*                                                                           */
/* This routine closes the specified SDS array volume.                       */
/*                                                                           */
/*****************************************************************************/

int sds_close(dev)
dev_t dev;
{
  int x;
  struct Volume *v;

  /* get the Volume structure for the SDS array volume */
  v = sds_lookup(dev);

  /* if this is a last close, remove the Volume structure from the list */
  /* of open SDS array volumes and then unconfigure the SDS array volume */
  if (! --v->openCount) {

    /* wait for all of our bufs to complete */
    v->flags.busy = 1;
    x = CRIT();
    while (sds_head(&v->waitingBufs) || v->freeBufCount != v->freeBufMax) {
      sleep(v, PRIBIO);
    }
    (void)UNCRIT(x);

    (void)sds_remove(v);
    sds_unconfig(v);
    FREE(v, M_IOSYS);
  }

  return 0;
}


/******************************* sds_strategy() ******************************/
/*                                                                           */
/* This routine performs the specified I/O to an SDS array volume.           */
/*                                                                           */
/*****************************************************************************/

void sds_strategy(bp, pv)
struct buf *bp;
struct Volume *pv;
{
  int i;
  int x;
  unsigned stripe;
  unsigned length;
  unsigned offset;
  unsigned partial;
  unsigned stripes;
  char *addr;
  struct buf *extra;
  struct Link reserved;
  struct Volume *v;

  /* get the Volume structure for the corresponding SDS array volume. */
  /* then add this buf to the tail of the waiting buf list. */
  x = CRIT();
  if (bp) {
    v = sds_lookup(bp->b_dev);
    sds_add(&v->waitingBufs, bp);
  } else {
    v = pv;
  }

  /* get the next I/O request to process */
  bp = sds_head(&v->waitingBufs);

  /* compute how many stripes it will need */
  partial = v->stripeSize - (bp->b_offset & v->stripeSize-1);
  stripes = (partial + v->stripeSize-1)/v->stripeSize +
            (bp->b_bcount-partial + v->stripeSize-1)/v->stripeSize;

  /* if we don't have enough free secondary bufs preallocated to satisfy */
  /* the next request then just return -- we'll try again later under ISR */
  if (stripes > v->freeBufCount) {
    (void)UNCRIT(x);
    return;
  }

  /* else grab the next request -- we're committed */
  (void)sds_remove(bp);

  /* check the I/O request for gross errors and prune it back to */
  /* the size of the SDS array volume, if necessary */
  if (bpcheck(bp, v->volumeSize, v->log2BlockSize, 0)) {
    (void)UNCRIT(x);
    return;
  }

  /* then recompute how many stripes it will *really* need */
  partial = v->stripeSize - (bp->b_offset & v->stripeSize-1);
  stripes = (partial + v->stripeSize-1)/v->stripeSize +
            (bp->b_bcount-partial + v->stripeSize-1)/v->stripeSize;

  /* and reserve that many secondary bufs */
  sds_empty(&reserved);
  for (i = 0; i < stripes; i++) {
    sds_add(&reserved, sds_remove_head(&v->freeBufs));
    v->freeBufCount--;
  }

  /* then begin striping */
  (void)UNCRIT(x);

  /* set the count of outstanding stripes for this I/O request */
  bp->b_ba = stripes;

  /* get the basic I/O info */
  length = bp->b_bcount;
  offset = bp->b_offset;
  addr = bp->b_un.b_addr;

  /* while there are still more stripes to process...  process the */
  /* next stripe of the I/O request */
  while (length) {
    stripe = sds_stripe(v, bp, offset, length, addr, &reserved);

    offset += stripe;
    addr += stripe;
    length -= stripe;
  }

  /* then return any secondary bufs we didn't use to the free list */
  while (extra = sds_remove_head(&reserved)) {
    sds_add(&v->freeBufs, extra);
    v->freeBufCount++;
  }
}


/********************************* sds_ioctl() *******************************/
/*                                                                           */
/* This routine performs the ioctl on an SDS array volume.                   */
/*                                                                           */
/*****************************************************************************/

int sds_ioctl(dev, cmd, arg, flag)
dev_t dev;
int cmd;
int *arg;
int flag;
{
  int i;
  int error;
  int *dexc;
  struct Volume *v;
  struct inquiry *sinq;
  struct capacity *scap;
  capacity_type *dcap;
  disk_describe_type *ddes;

  /* return EINVAL for non-supported ioctl's */
  switch (cmd & IOCCMD_MASK) {
    case SIOC_INQUIRY & IOCCMD_MASK:
    case SIOC_CAPACITY & IOCCMD_MASK:
    case SIOC_GET_IR & IOCCMD_MASK:
    case SIOC_SET_IR & IOCCMD_MASK:
    case DIOC_DESCRIBE & IOCCMD_MASK:
    case DIOC_CAPACITY & IOCCMD_MASK:
    case DIOC_EXCLUSIVE & IOCCMD_MASK:
    case CIOC_DESCRIBE & IOCCMD_MASK:
      break;

    default:
      return EINVAL;
  }

  /* get the Volume structure for the SDS array volume */
  v = sds_lookup(dev);

  /* then perform the ioctl on the lead device of the SDS array */
  if (error = (v->cdevsw->d_ioctl)(v->devices[0].dev, cmd, arg, 0)) {
    return error;
  }

  /* then do the remaining ioctl processing */
  switch (cmd & IOCCMD_MASK) {
    case SIOC_INQUIRY & IOCCMD_MASK:
      sinq = (struct inquiry *)arg;
      bzero(sinq->vendor_id, sizeof(sinq->vendor_id));
      bzero(sinq->product_id, sizeof(sinq->product_id));
      strcpy(sinq->vendor_id, "HP");
      strncpy(sinq->product_id, v->type, sizeof(sinq->product_id));
      break;

    case SIOC_CAPACITY & IOCCMD_MASK:
      scap = (struct capacity *)arg;
      scap->lba = v->volumeSize;
      break;

    case SIOC_GET_IR & IOCCMD_MASK:
      break;

    case SIOC_SET_IR & IOCCMD_MASK:
      /* then set the immediate report mode for the remaining devices in */
      /* the SDS array */
      for (i = 1; i < v->deviceCount; i++) {
        if (error = (v->cdevsw->d_ioctl)(v->devices[i].dev, cmd, arg, 0)) {
          return error;
        }
      }
      break;

    case DIOC_DESCRIBE & IOCCMD_MASK:
      ddes = (disk_describe_type *)arg;
      bzero(ddes->model_num, sizeof(ddes->model_num));
      strncpy(ddes->model_num, v->type, sizeof(ddes->model_num));
      ddes->maxsva = v->volumeSize - 1;
      break;

    case DIOC_CAPACITY & IOCCMD_MASK:
      dcap = (capacity_type *)arg;
      dcap->lba = v->volumeSize*v->blockSize/(unsigned)DEV_BSIZE - 1;
      break;

    case DIOC_EXCLUSIVE & IOCCMD_MASK:
      dexc = (int *)arg;
      if (*dexc) {
        if (! (flag & FWRITE)) {
          return EACCES;
        } else if (v->openCount > 1) {
          return EBUSY;
        }

        /* for optical, we need a special case -- if this is the block */
        /* dev and the char dev is already open (or vice versa) then */
        /* fail the DIOC_EXCLUSIVE since the *total* open count is > 1 */
        if (major(dev) == OPAL_MAJ_BLK) {
          if (sds_lookup(makedev(OPAL_MAJ_CHR, minor(dev)))) {
            return EBUSY;
          }
        } else if (major(dev) == OPAL_MAJ_CHR) {
          if (sds_lookup(makedev(OPAL_MAJ_BLK, minor(dev)))) {
            return EBUSY;
          }
        }

        v->flags.locked = 1;
      } else {
        v->flags.locked = 0;
      }
      break;
  }

  return 0;
}


/******************************** sds_read() *********************************/
/*                                                                           */
/* This routine performs a read on an SDS array volume.                      */
/*                                                                           */
/*****************************************************************************/

int sds_read(dev, uio)
dev_t dev;
struct uio *uio;
{
  return physio(sds_strategy, NULL, dev, B_READ, minphys, uio);
}


/******************************* sds_write() *********************************/
/*                                                                           */
/* This routine performs a write on an SDS array volume.                     */
/*                                                                           */
/*****************************************************************************/

int sds_write(dev, uio)
dev_t dev;
struct uio *uio;
{
  return physio(sds_strategy, NULL, dev, B_WRITE, minphys, uio);
}


/****************************** sds_size() ***********************************/
/*                                                                           */
/* This routine returns the size of an SDS array volume.                     */
/*                                                                           */
/*****************************************************************************/

unsigned sds_size(dev)
dev_t dev;
{
  struct Volume *v;
  unsigned size;

  /* open the SDS array volume */
  if (sds_open(dev, 0)) {
    return 0;
  }

  /* get the Volume structure for the SDS array volume */
  v = sds_lookup(dev);

  /* then compute the SDS array volume size in DEV_BSIZE units */
  size = v->volumeSize*v->blockSize/(unsigned)DEV_BSIZE;

  /* then close the SDS array volume */
  sds_close(dev);

  return size;
}


/*****************************************************************************/
