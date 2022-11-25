/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/mf.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:14:31 $
 */
/* @(#) $Revision: 1.3.84.3 $ */       
#ifndef _SYS_MF_INCLUDED /* allows multiple inclusion */
#define _SYS_MF_INCLUDED
#ifdef __hp9000s300
/*****************************************************************************
  Unix Minifloppy Driver header    (dlb) 
 *****************************************************************************/
#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

#define	MFINITS	_IOWR('F', 1, struct MF_init_track) /* get parameters */

struct MF_init_track {
	short	error;		/* returned driver error codes */
	char	phy_track_num;	/* physical cylinder/head number to init */
	char	log_track_num;	/* logical cylinder/head number to init to */
	char	pig_count;	/* post index gap count */
	char	pig_data;	/* post index gap data */
	char	data;		/* data to write on the track */
	char	*crt_ptr;	/* guru pointer to alpha plane 9836 only */
	char	log_sectbl[16];	/* sector ordering on the track */
};
#endif /* __hp9000s300 */
#endif /* _SYS_MF_INCLUDED */
