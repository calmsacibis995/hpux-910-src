/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/cdfs/RCS/cdfs_subr.c,v $
 * $Revision: 1.4.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:36:55 $
 */

/* HPUX_ID: @(#)cdfs_subr.c	54.4		88/12/02 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988  Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304

*/

#include "../h/types.h"
#include "../h/time.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/vnode.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#define	HOUR_SECOND	(60*60)
#define	DAY_SECOND	(24*HOUR_SECOND)
#define	YEAR_SECOND	(365*DAY_SECOND)
unsigned int mon_sec[] = { 0,
		(31) * DAY_SECOND, 
		(31 + 28) * DAY_SECOND,
		(31 + 28 + 31) * DAY_SECOND,
		(31 + 28 + 31 + 30) * DAY_SECOND,
		(31 + 28 + 31 + 30 + 31) * DAY_SECOND,
		(31 + 28 + 31 + 30 + 31 + 30) * DAY_SECOND,
		(31 + 28 + 31 + 30 + 31 + 30 + 31) * DAY_SECOND,
		(31 + 28 + 31 + 30 + 31 + 30 + 31 + 31) * DAY_SECOND,
		(31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30) * DAY_SECOND,
		(31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31) * DAY_SECOND,
		(31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 ) * DAY_SECOND};

/*****************************************************************************
 *
 * secnods()
 *
 * Convert the year, month, day, hour, minutes, second and timezone into
 * seconds stored in "time".
 *
 *****************************************************************************/
seconds(year, month, day, hour, minute, second, zone, time)
register unsigned char  year, month, day, hour, minute, second;
char	zone;
struct	two_time *time;
{
	unsigned int *sp, s;
	unsigned int leap = 0;	/*set if current year is leap year and 
				  month is beyone Feb. */
	unsigned int leaps = 0; /* how many leap years*/

	time->post_time = 0;
	time->pre_time = 0;
	if (((year & 3) == 0) && ((year % 100) != 0) && (month > 2)) 
		leap = 1;

	if (year >= 70) { /*after 1970 (inclusive)*/
		sp = &(time->post_time);
		year -= 70;
		if (year <= 2) 
			leaps = 0;
		else 
			leaps = (year + 1) >> 2; /* leap years between last year
						    and 1970*/
	} else {
		sp = &(time->pre_time);
		if (year == 0) 
			leaps = 0;
		else 
			leaps = (year-1) >> 2;
	} 
	leaps += leap;
	s = year * YEAR_SECOND + leaps * DAY_SECOND;   
	s += (mon_sec[month-1] + ((day-1)*DAY_SECOND) + (hour * HOUR_SECOND) +
	     (minute * 60) + second + (zone * 15 * 60));
	*sp = s;
}

