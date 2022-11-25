/*
 * @(#)dux_def.h: $Revision: 1.7.83.3 $ $Date: 93/09/17 16:40:34 $
 * $Locker:  $
 */

#ifndef _SYS_DUX_DEV_INCLUDED	/* allows multiple inclusion */
#define _SYS_DUX_DEV_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/devices.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/devices.h>
#endif /* _KERNEL_BUILD */

/*
 * For compatability with older diskless kernels, we must use 0x7fff
 * for DEVINDEX, even though we could otherwise use 0xffff.  This
 * restricts the size of the hash table to 32, rather than 64 entries.
 */
#define DEVINDEX        0x7fff
#define DEVSITE         0xff

#ifdef _KERNEL
extern int brmtdev;
extern int crmtdev;

extern dtaddr_t deventry(); /* pointer to entry in remote dev table */
extern dev_t localdev();    /* convert remote dev_t to local dev_t */
extern int devindex();      /* compute DEVINDEX part for remote dev_t */
#endif /* _KERNEL */

/* if device is local, return 0, else non-zero */
#define bdevrmt(x)     (major(x)==brmtdev)
#define cdevrmt(x)     (major(x)==crmtdev)

/*  Does this inode live on a remote device? */
#define remoteip(ip)    bdevrmt((ip)->i_dev)
#define remotecdp(cdp)  bdevrmt((cdp)->cd_dev)

/* grab device where node lives; assumes remote device */
#define devsite(x)      ((site_t)(((x) >> 16) & DEVSITE))

/* Analogous makedev routines for charater and block devices */
#define mkbrmtdev(nd,in) ((brmtdev<<24)|((nd&DEVSITE)<<16)|(in&DEVINDEX))
#define mkcrmtdev(nd,in) ((crmtdev<<24)|((nd&DEVSITE)<<16)|(in&DEVINDEX))

/*
 * Structure for getting a device and site.  Passed as a dependent by
 * getmdev to lookuppn, but filled in by the unpack routine
 */
struct devandsite	/*DUX MESSAGE STRUCTURE*/
{
	dev_t  dev;
	site_t site;
};

/*
 * constants used by umount_dev_serve() and send_umount_dev to describe
 * the device number paramater
 */
#define DEV_NOT_ENCODED	0
#define DEV_RMT		1  /* remote device number */
#define DEV_LU_UNMAPPED	2  /* s800 only - LU in dev not mapped to MI */

#define DUX_DEVIDSHIFT  10
#define DUX_DEVIDMASK	0x3ff

#define DUX_GET_DEVHASH(dev)  (((dev) & DEVINDEX) >> DUX_DEVIDSHIFT)
#define DUX_GET_DEVID(dev)    ((dev) & DUX_DEVIDMASK)

#define DUX_MAKE_DEVHASH(hash, id)  ((hash) << DUX_DEVIDSHIFT | (id))

#endif /* _SYS_DUX_DEV_INCLUDED */
