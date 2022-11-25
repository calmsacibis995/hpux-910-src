/*
 * @(#)parse_conf.h: $Revision: 1.2.109.1 $ $Date: 94/10/26 16:55:26 $
 * $Locker:  $
 */

/*
 * /src/NTP/REPOSITORY/v3/include/parse_conf.h,v 3.2 1993/07/09 11:37:07 kardel Exp
 *
 * parse_conf.h,v 3.2 1993/07/09 11:37:07 kardel Exp
 *
 * Copyright (c) 1993
 * Frank Kardel Friedrich-Alexander Universitaet Erlangen-Nuernberg
 *                                    
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * parse_conf.h,v
 * Revision 3.2  1993/07/09  11:37:07  kardel
 * Initial restructured version + GPS support
 *
 * Revision 3.1  1993/07/06  09:59:14  kardel
 * DCF77 driver goes generic...
 *
 */

#ifndef __PARSE_CONF_H__
#define __PARSE_CONF_H__
#if	!(defined(lint) || defined(__GNUC__))
  static char dcfhrcsid[]="parse_conf.h,v 3.2 1993/07/09 11:37:07 kardel Exp FAU";
#endif

/*
 * field location structure (Meinberg clocks/simple format)
 */
#define O_DAY	0
#define O_MONTH 1
#define O_YEAR	2
#define O_HOUR	3
#define O_MIN	4
#define	O_SEC	5
#define O_WDAY	6
#define O_FLAGS 7
#define O_ZONE  8
#define O_UTCHOFFSET 9
#define O_UTCMOFFSET 10
#define O_UTCSOFFSET 11
#define O_COUNT (O_UTCSOFFSET+1)

#define MBG_EXTENDED	0x00000001

/*
 * see below for field offsets
 */

struct format
{
  struct foff
    {
      char offset;		/* offset into buffer */
      char length;		/* length of field */
    } field_offsets[O_COUNT];
  char *fixed_string;		/* string with must be chars (blanks = wildcards) */
  unsigned LONG flags;
};
#endif
