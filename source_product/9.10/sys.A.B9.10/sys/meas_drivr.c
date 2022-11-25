/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/meas_drivr.c,v $
 * $Revision: 1.18.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:08:24 $
 */

  /* POSIX Bull */
#ifdef __hp9000s800
#define hp9000s800 1
#endif

#include	"../h/errno.h"

#ifdef hp9000s800
meas_drivr_open()
#else
meas_sys_open(dev)
#endif
{
	return(ENXIO);
}

#ifdef hp9000s800
meas_drivr_close()
#else
meas_sys_close()
#endif
{
	return(ENXIO);
}

#ifdef hp9000s800
meas_drivr_read()
#else
meas_sys_read()
#endif
{
	return(ENXIO);
}

#ifdef hp9000s800
meas_drivr_write()
#else
meas_sys_write()
#endif
{
	return(ENXIO);
}

#ifdef hp9000s800
meas_drivr_ioctl()
#else
meas_sys_ioctl()
#endif
{
	return(ENXIO);
}
