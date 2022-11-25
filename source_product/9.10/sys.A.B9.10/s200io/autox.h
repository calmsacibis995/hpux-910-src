/*
 * @(#)autox.h: $Revision: 1.2.84.3 $ $Date: 93/09/17 21:09:53 $
 * $Locker:  $
 */

#ifndef _SYS_AC_INCLUDED
#define _SYS_AC_INCLUDED

#ifndef _KERNEL_BUILD
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

/*
 * data structure for AUTOX0_RESERVE and AUTOX0_RELEASE
 */
struct reserve_parms {
	unsigned char    element;
	unsigned char    resv_id;        /* Reservation ID */
	unsigned short   ell;            /* Element list length */
};

#define AUTOX0_INITIALIZE_ELEMENT_STATUS _IO ('S', 1)
#define AUTOX0_READ_ELEMENT_STATUS	 _IOW('S', 2, int)
#define AUTOX0_MOVE_MEDIUM		 _IOW('S', 3, struct move_medium_parms)
#define AUTOX0_RESERVE                   _IOW('S', 4, struct reserve_parms)
#define AUTOX0_RELEASE                   _IOW('S', 5, struct reserve_parms)

#endif

