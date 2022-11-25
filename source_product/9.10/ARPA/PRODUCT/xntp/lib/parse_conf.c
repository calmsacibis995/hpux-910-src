# if defined(REFCLOCK) && defined(PARSE)
/*
 * $Header: parse_conf.c,v 1.2.109.2 94/10/31 10:34:41 mike Exp $
 *  
 * parse_conf.c,v 3.2 1993/07/09 11:37:13 kardel Exp
 *
 * Parser configuration module for reference clocks
 *
 * STREAM define switches between two personalities of the module
 * if STREAM is defined this module can be used with dcf77sync.c as
 * a STREAMS kernel module. In this case the time stamps will be
 * a struct timeval.
 * when STREAM is not defined NTP time stamps will be used.
 *
 * Copyright (c) 1992,1993
 * Frank Kardel Friedrich-Alexander Universitaet Erlangen-Nuernberg
 *                                    
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * parse_conf.c,v
 * Revision 3.2  1993/07/09  11:37:13  kardel
 * Initial restructured version + GPS support
 *
 * Revision 3.1  1993/07/06  10:00:11  kardel
 * DCF77 driver goes generic...
 *
 *
 */

# 	include "sys/types.h"
# 	include "sys/time.h"
# 	include "sys/errno.h"
# 	include "ntp_fp.h"
# 	include "ntp_unixtime.h"
# 	include "ntp_calendar.h"

# 	include "parse.h"

# 	ifdef CLOCK_SCHMID
extern clockformat_t    clock_schmid;
# 	endif			/* CLOCK_SCHMID */

# 	ifdef CLOCK_DCF7000
extern clockformat_t    clock_dcf7000;
# 	endif			/* CLOCK_DCF7000 */

# 	ifdef CLOCK_MEINBERG
extern clockformat_t    clock_rawdcf;
# 	endif			/* CLOCK_MEINBERG */

# 	ifdef CLOCK_RAWDCF
extern clockformat_t    clock_meinberg[];
# 	endif			/* CLOCK_RAWDCF */

/*
 * format definitions
 */
clockformat_t  *clockformats[] =
{
# 	ifdef CLOCK_MEINBERG
    &clock_meinberg[0],
    &clock_meinberg[1],
    &clock_meinberg[2],
# 	endif			/* CLOCK_MEINBERG */
# 	ifdef CLOCK_RAWDCF
    &clock_rawdcf,
# 	endif			/* CLOCK_RAWDCF */
# 	ifdef CLOCK_DCF7000
    &clock_dcf7000,
# 	endif			/* CLOCK_DCF7000 */
# 	ifdef CLOCK_SCHMID
    &clock_schmid,
# 	endif			/* CLOCK_SCHMID */
# 	ifdef CLOCK_RAWDCF
    &clock_rawdcf,
# 	endif			/* CLOCK_RAWDCF */
    0
};				/* XXX dragons be here */

int     nformats = sizeof (clockformats)/sizeof (clockformats[0]);
# endif				/* REFCLOCK PARSE */
