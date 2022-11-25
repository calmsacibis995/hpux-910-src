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

/*
 * What string for patches.
 */
#ifdef PATCHED
static char autoch_patch_id[] =
	"@(#) PATCH_9.0: autox.o 1.6.84.2 93/07/22 PHKL_2396";
#endif /* PATCHED */

/******************************************************************************
**
** Filename:	autox.c
** Purpose: 	The purpose of this module is to handle all the events
**		associated with the opening, closing, and controlling
**		the autochanger.  Each autochanger has a device corresponing
**		with it and the information for the autochangers is stored
**		in a circular queue.  When an autochanger is opened the
**		device file is set up and the autochanger is put onto a 
**		circular queue so it can be processed.  When an autochanger
**		device is closed it is removed from the queue and all the
**		memory for the device is freed.
**		The control of the autochanger is throught autox0_ioctl
**		which set up the appropriate buffers and structures and
**		interfaces through the scsi driver.
** Author:	Bruce Thompson
** Environment:	HP-UX 8.0
** History:	06/28/90, Bruce Thompson
**
******************************************************************************/

#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/sysmacros.h"
#include "../h/types.h"
#include "../h/time.h"
#include "../h/systm.h"
#include "../wsio/timeout.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../wsio/tryrec.h"
#include "../h/scsi.h"
#include "../s200io/scsi.h"
#include "../s200io/autox.h"
#include "../h/ioctl.h"

/**---------------------------------------------------------------------------
	Public Function Prototypes
---------------------------------------------------------------------------**/
/*
int autox0_open(dev_t dev, int flag);
int autox0_close(dev_t dev);
int autox0_read(dev_t dev, struct uio *uio);
int autox0_ioctl(dev_t dev, int order, caddr_t addr, int flag);
*/

/**---------------------------------------------------------------------------
	Private Function Prototypes
---------------------------------------------------------------------------**/
/*
struct autox_device* init_autox_device(dev_t dev);
struct autox_device* get_autox_device(dev_t dev);
*/

#define SCSI_MAJ_CHAR 47
#define make_scsi_dev(dev) makedev(SCSI_MAJ_CHAR, minor(dev))

#define RESERVE_ELEMENT_LIST_LENGTH 6
#define EL_STATUS_SZ      0x1000      /* 4k */

/* Locks for ioctl so two processes don't use the buffer
   at the same time. */
#define IN_IOCTL	0x1
#define WANT_IOCTL	0x2


/*
 * This macro adds a structure to a doublely linked list 
 * at the end of the list.  
 */

#define ADD_TO_LIST(head,struct_ptr) \
   {int priority; \
      /* keep out all interrupts that might modify the list */ \
      priority = spl6(); \
      if (head == NULL) {         /* list is empty     */ \
         head   = struct_ptr; \
         struct_ptr->back  = struct_ptr; \
         struct_ptr->forw  = struct_ptr; \
      } else {             /* list is not empty */ \
         head->back->forw = struct_ptr; \
         struct_ptr->back = head->back; \
         head->back = struct_ptr; \
         struct_ptr->forw = head; \
      } \
      splx(priority); \
   }

/*
** This macro deletes an  element from a doublely linked list.
*/

#define DELETE_FROM_LIST(head,struct_ptr) \
   {int priority; \
      /* keep out all interrupts that might modify the list */ \
      priority = spl6(); \
      if (struct_ptr->forw == struct_ptr) {  \
          /* only element in the list  */ \
         head = NULL; \
      } else { \
         struct_ptr->back->forw = struct_ptr->forw; \
         struct_ptr->forw->back = struct_ptr->back; \
         if (head == struct_ptr)      \
            /* first element on list  */ \
            head = struct_ptr->forw; \
      } \
      splx(priority); \
   }

/*
** This macro frees memory associated with a device that has been
** removed from the the list.
*/

#define FREE_ENTRY(struct_ptr,struct_type) \
   /* free up the memory from the deleted request */ \
   kmem_free ((caddr_t)struct_ptr,(u_int)(sizeof(struct struct_type))); 


/* in order to use these 3 macros,          */
/* you must have 'autox_device_ptr' defined */
#define FOR_EACH_AUTOX_DEVICE \
   {int priority; \
      /* keep out all interrupts that might modify the list */ \
      priority = spl6(); \
      /* loop thru the queue */ \
      autox_device_ptr = autox_device_head; \
      if (autox_device_ptr != NULL) { \
      do {
#define END_FOR \
         autox_device_ptr = autox_device_ptr->forw; \
         } while (autox_device_ptr != autox_device_head); \
      } \
      splx(priority); \
   }
#define RETURN_FROM_FOR(return_value) {splx(priority); return(return_value);}


struct autox_device {
	struct autox_device *forw, *back;
	dev_t dev;
	int autox_buflock;
	struct buf *ac_buf;
	unsigned char read_el_data[EL_STATUS_SZ];
};

struct autox_device *autox_device_head = NULL;

/*
** init_autox_device(dev_t dev)
**
** Parameters:
**	dev 	The device structure associated with a autochanger.
** Globals:
** Operation:
**	This routine is called when a new device is opened, it accepts
**	a dev associated with the autochanger and sets up the appropriate
**	fields for a new autochanger device structure.  
** Return: pointer to an autox_device
*/

/* Initializes an autox_device structure */
struct autox_device *
init_autox_device(dev)
dev_t dev;
{
	struct autox_device *autox_device_ptr;
        extern struct buf *geteblk();
	if ((autox_device_ptr = (struct autox_device *)kmem_alloc(
		sizeof(struct autox_device))) == NULL) {
		panic("Error with kmem_alloc in init_autox_device\n");
   	}
	autox_device_ptr->forw = NULL;
	autox_device_ptr->back = NULL;
	autox_device_ptr->dev = dev;
	autox_device_ptr->autox_buflock = 0;
	autox_device_ptr->ac_buf = geteblk(EL_STATUS_SZ);
	return(autox_device_ptr);

}  /* init_autox_device */

/*
** get_autox_device(dev_t dev)
**
** Parameters:
**	dev	the scsi device to be associated with a given
**	autochanger.
** Globals:
** Operation:
**	This routine is used to find the proper autochanger device for
**	the device that	is being read or closed.   This routine loops 
**	through the list of open autochangers and finds the one that is
**	associated with the scsi device.
** Return: if dev is found, pointer to the autox_device structure.
**	   if not found NULL
*/
struct autox_device *
get_autox_device(dev)
dev_t dev;
{
	/* this structure must be defined in order to use the following macros */
	struct autox_device *autox_device_ptr;
	FOR_EACH_AUTOX_DEVICE
	   if(autox_device_ptr->dev == dev)
	      RETURN_FROM_FOR(autox_device_ptr)
	END_FOR
	return(NULL);

} /* get_autox_device */


/*
** autox0_open(dev_t dev, int flag)
**
** Parameters:
**	dev	the device associated with the autochanger.
**	flag	used for standard open calls - not used in this
**		routine
** Globals:
** Operation:
** 	This routine is called by open_autochanger and is used
**	during the initial opening of the autochanger.  Every
**	autochanger connected to the system needs to be opened
**	and added to the list.
** Return: error status associated with a scsi open.
*/
int
autox0_open(dev, flag)
dev_t dev;
int flag;
{
    struct autox_device *autox_device_ptr;
    dev_t scsi_dev = make_scsi_dev(dev);
    int error = 0;
   
    autox_device_ptr = init_autox_device(scsi_dev);
    ADD_TO_LIST(autox_device_head,autox_device_ptr);
    error = scsi_open(scsi_dev);
    if (error) {
        DELETE_FROM_LIST(autox_device_head,autox_device_ptr);
        brelse(autox_device_ptr->ac_buf);
        FREE_ENTRY(autox_device_ptr,autox_device);
    }
    return(error);

} /* autox0_open */



/*
** autox0_close(dev_t dev)
**
** Parameters:
**	dev	autochanger device that will be closed.
** Globals:
** Operation:
**	This routine is used to close an autochanger when it is no
**	longer in use.  It finds the autochanger corresponding to
**	the device, deletes it from the autochanger device list,
**	releases the memory associated with the io buffers and
**	frees the memory associated with the entry.
**	Note: Harpoon does not call this routine.
** Return: error status associated with the closing of the scsi device
*/
int
autox0_close(dev)
dev_t dev;
{
    dev_t scsi_dev = make_scsi_dev(dev);
    struct autox_device *autox_device_ptr;
    if ((autox_device_ptr = get_autox_device(scsi_dev)) == NULL) {
	panic("Error with get_autox_device in autox0_close\n");
    }
    DELETE_FROM_LIST(autox_device_head,autox_device_ptr);
    if (autox_device_ptr->ac_buf)
        brelse(autox_device_ptr->ac_buf);
    FREE_ENTRY(autox_device_ptr,autox_device);
    return(scsi_close(scsi_dev));

} /* autox0_close */


/*
** autox0_read(dev_t dev, struct uio *uio)
**
** Parameters:
**	dev	the device corresponding to the autochanger device.
**	uio	pointer to the users io space in memory
** Globals:
** Operation:
**	This routine is used in conjunction with autox_ioctl during a
**	read element status.  This routine will move the read element
**	data from the autox_device structure into user memory space so
**	it can be processed.
** Return: zero - panic or passed.
*/
int
autox0_read(dev, uio)
dev_t dev;
struct uio *uio;
{
    struct autox_device *autox_device_ptr;
    dev_t scsi_dev = make_scsi_dev(dev);
    if ((autox_device_ptr = get_autox_device(scsi_dev)) == NULL) {
	panic("Error with get_autox_device in autox0_read\n");
    }
    uiomove(autox_device_ptr->read_el_data,uio->uio_iov->iov_len,UIO_READ,uio);

    return(0);

} /* autox0_read */


/*
** autox0_ioctl(dev_t dev, int order, caddr_t addr, int flag)
**
** Parameters:
**	dev	the device associated with the autochanger
**	order	operation to be perfomed on the autochanger 
**	addr	pointer to appropriate structures needed to
**		do the scsi_driver_ctl calls.
**	flag	used for standard ioctl calls - not used in
**		this routine
** Globals:
** Operation:
**	This routine handles all the scsi commands associated with 
**	the autochanger.  This routine moves the media, initializes 
**	element status, reads element status, reserves and releases 
**	elements.  All the autochanger commands are interfaces through
**	scsi driver controller (scsi_driver_ctl). 
** Return: returns an error associated with the io buffer 
*/

int
autox0_ioctl(dev, order, addr, flag)
dev_t dev;
int order;
caddr_t addr;
int flag;
{
    dev_t scsi_dev = make_scsi_dev(dev);
    struct autox_device *autox_device_ptr;
    struct reserve_parms *rp;  /* used for element addressing */
    /*** struct move_medium_parms *mp; used for debugging ****/
    struct buf *bp; /* buffer for the autochanger dev */
    int error = 0;  /* error status on returned routined */

    if ((autox_device_ptr = get_autox_device(scsi_dev)) == NULL) {
	panic("Error with get_autox_device in autox0_ioctl\n");
    }

    while  (autox_device_ptr->autox_buflock & IN_IOCTL) {
        /* have to wait */
        autox_device_ptr->autox_buflock |= WANT_IOCTL;
        error = sleep(&(autox_device_ptr->autox_buflock),PRIBIO);
    }
    autox_device_ptr->autox_buflock |= IN_IOCTL;

    bp = autox_device_ptr->ac_buf;
    bp->b_flags = B_BUSY;
    bp->b_resid = 0;
    bp->b_error = 0;
    switch (order) {
    case AUTOX0_INITIALIZE_ELEMENT_STATUS:
        bp->b_dev = scsi_dev;
        bp->b_bcount = SZ_VAR_CNT;
        scsi_driver_ctl(bp,INIT_ELEMENT_STATUS, addr);
        iowait(bp);
        break;

    case AUTOX0_READ_ELEMENT_STATUS:
        bp->b_dev = scsi_dev;
        bp->b_bcount = EL_STATUS_SZ;
        scsi_driver_ctl(bp,READ_ELEMENT_STATUS, addr);
        iowait(bp);
        if (!(geterror(bp))) {
           /* need to copy it into appropriate buffer based on dev etc. */
           bcopy(bp->b_un.b_addr, autox_device_ptr->read_el_data, EL_STATUS_SZ);
        }
        break;

    case AUTOX0_MOVE_MEDIUM:
        bp->b_dev = scsi_dev;
        bp->b_bcount = SZ_VAR_CNT;
	
	/* mp = (struct move_medium_parms *)addr; */
	/* printf("	AUTOX:transport = %d\n",mp->transport); */
	/* printf("	AUTOX:source    = %d\n",mp->source   ); */
	/* printf("	AUTOX:destination= %d\n",mp->destination); */
	/* printf("	AUTOX:invert    = %c\n",mp->invert   ); */
        scsi_driver_ctl(bp,MOVE_MEDIUM, addr);
        iowait(bp);
        break;

    case AUTOX0_RESERVE:
        rp = (struct reserve_parms *)addr;
        bp->b_bcount = RESERVE_ELEMENT_LIST_LENGTH;
        bp->b_dev = scsi_dev;
	bp->b_un.b_addr[0]= 0;            /* reserved must be 0 */
	bp->b_un.b_addr[1]= 0;            /* reserved must be 0 */
	bp->b_un.b_addr[2]= 0;            /* msb number of elements */
	bp->b_un.b_addr[3]= 1;            /* lsb number of elements */
	bp->b_un.b_addr[4]= rp->resv_id >> 8; /* msb element_address */
	bp->b_un.b_addr[5]= rp->resv_id;      /* lsb element_address */
        scsi_driver_ctl(bp,RESERVE, addr);
        iowait(bp);
        break;

    case AUTOX0_RELEASE:
        bp->b_dev = scsi_dev;
        bp->b_bcount = SZ_VAR_CNT;
        scsi_driver_ctl(bp,RELEASE, addr);
        iowait(bp);
        break;
    } /* switch order */

    /* wakeup sleepting requests */
    if (autox_device_ptr->autox_buflock & WANT_IOCTL) {
       wakeup((caddr_t)&(autox_device_ptr->autox_buflock));
    }
    autox_device_ptr->autox_buflock = 0;
    return (geterror(bp));

} /* autox0_ioctl */

