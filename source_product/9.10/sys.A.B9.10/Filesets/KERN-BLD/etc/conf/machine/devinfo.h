/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/devinfo.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:11:06 $
 */
/* HPUX_ID: @(#)devinfo.h	52.2		88/04/26 */
#ifndef _MACHINE_DEVINFO_INCLUDED /* allow multiple inclusions */
#define _MACHINE_DEVINFO_INCLUDED

/* the devinfo data structure */

struct devinfo {
	unsigned char	sc;
	unsigned char	ba;
	unsigned char	unit;
	unsigned char	volume;
	unsigned char	flags;
	unsigned char	type;
	daddr_t		start_partition;
	daddr_t		size_partition;
	struct iobuf   *queue_head;
};
#endif /* _MACHINE_DEVINFO_INCLUDED */
