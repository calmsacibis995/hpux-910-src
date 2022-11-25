/*
 * @(#)ntp_types.h: $Revision: 1.2.109.1 $ $Date: 94/10/26 16:55:18 $
 * $Locker:  $
 */

/* ntp_types.h,v 3.1 1993/07/06 01:07:00 jbj Exp
 *  ntp_types.h - defines how LONG and U_LONG are treated.  For 64 bit systems
 *  like the DEC Alpha, they has to be defined as int and u_int.  for 32 bit
 *  systems, define them as long and u_long
 */

#ifndef _NTP_TYPES_
#define _NTP_TYPES_


/*
 * DEC Alpha systems need LONG and U_LONG defined as int and u_int
 */
#if defined(__alpha)
#  ifndef LONG
#    define LONG int
#  endif
#  ifndef U_LONG
#    define U_LONG u_int
#  endif
/*
 *  All other systems fall into this part
 */
#else
#  ifndef LONG
#    define LONG long
#  endif
#  ifndef U_LONG
#    define U_LONG u_long
#  endif
#endif

    
#endif /* _NTP_TYPES_ */

