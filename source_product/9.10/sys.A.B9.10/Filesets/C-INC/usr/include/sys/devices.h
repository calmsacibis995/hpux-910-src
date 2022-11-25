/*
 * @(#)devices.h: $Revision: 1.5.83.4 $ $Date: 93/09/17 18:24:52 $
 * $Locker:  $
 */

#ifndef _SYS_DEVICES_INCLUDED /* to permit multiple inclusion */
#define _SYS_DEVICES_INCLUDED

#ifdef	FULLDUX
#include "../dux/sitemap.h"
#endif  /* FULLDUX */

#ifdef _KERNEL_BUILD
#include "../h/sem_beta.h"
#else /* _KERNEL_BUILD */
#include <h/sem_beta.h>
#endif /* _KERNEL_BUILD */

#ifdef _KERNEL
#define DORDER	150
#endif /* _KERNEL */

struct device_table {
    struct device_table *next;  /* next element in hash chain  */
    dev_t dt_dev;               /* The system device number    */
    u_int dt_mode;              /* Mode (IFBLK or IFCHR)       */
    u_int dt_flags;             /* Flags (see below)           */
#ifdef  FULLDUX
    struct sitemap dt_map       /* site-by-site count of opens */
#else
    long    dt_cnt;             /* count of opens              */
#endif
    u_short  dt_id;		/* unique id within hash chain */
    u_short  dt_reserved;	/* reserved for future use     */
    b_sema_t dt_sema;		/* semaphore to control access */
};

typedef struct device_table dtable_t, *dtaddr_t;

/*  Flags  */
#define D_INUSE         0x01    /* This slot is being used            */
#define D_ALLCLOSES     0x02    /* Call d_close routine on each close */
#define D_LOCKED        0x04    /* The device is locked - not used    */

/*
 * Hashing function for device table.
 */
#define DEVHSZ	32
#define DEVMASK (DEVHSZ-1)
/*
 * Divide and conquer -- this looks ugly, but is very efficient when
 *			 you have a good compiler.
 */
#define _DEVSUB_1(dev)  (         (dev) + (         (dev) >>  8))
#define _DEVSUB_2(dev)  (_DEVSUB_1(dev) + (_DEVSUB_1(dev) >> 12))
#define DEVHASH(dev)   ((_DEVSUB_2(dev) + (_DEVSUB_2(dev) >>  6)) & DEVMASK)

#ifdef _KERNEL
/*
 * Hash table of open devices.
 */
extern dtaddr_t devhash[];

/*
 * Table manipulation functions
 */
extern dtaddr_t lookup_dev();
extern dtaddr_t alloc_dev();
extern void	dev_rele();

#endif /* _KERNEL */
#endif /* _SYS_DEVICES_INCLUDED */
