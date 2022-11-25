/* @(#) $Revision: 1.10.83.4 $ */    
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/mknod.h,v $
 * $Revision: 1.10.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:29:46 $
 */

#ifndef _SYS_MKNOD_INCLUDED
#define _SYS_MKNOD_INCLUDED

/* FILE BEING OBSOLETED - the definitions in this file are now also
   in sys/sysmacros.h (as they are in Bell V.2). */

/* This file defines the major and minor device fields */
/* This version is for the Series 200 */

/* device type: officially declared in types.h */
/* typedef long dev_t; */

/* major part of a device */
#ifdef __hp9000s300
#define	major(x)	(long)((unsigned)(x)>>24)
#endif /* __hp9000s300 */
#ifdef __hp9000s800
#define	major(x)	((long)(((unsigned)(x)>>24)&0xff)) /* 8 bit major */
#endif /* __hp9000s800 */

/* minor part of a device */
#define	minor(x)	(long)((x)&0xffffff)

/* make a device number */
#define	makedev(x,y)	(dev_t)(((x)<<24) | (y & 0xffffff))

/* the standard print format for a minor device */
#define MINOR_FORMAT	"0x%06.6x"

#endif /* not _SYS_MKNOD_INCLUDED */
