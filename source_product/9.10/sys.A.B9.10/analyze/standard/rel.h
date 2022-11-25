/*
 * @(#)rel.h: $Revision: 1.6.83.3 $ $Date: 93/09/17 16:31:43 $
 * $Locker:  $
 */


/*
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/

#if defined(R91) || defined(RVAX) || defined(R10)
/* some simple intuition */
#define OLD_NNC
#else
#undef  OLD_NNC
#endif

#if defined(R91) || defined(RVAX) || defined(R10) || defined(R11)
/* more intuition, use old style sio header files */
#define OLDSIOH
#else
#undef  OLDSIOH
#endif

/* general */
#define INTERACTIVE
/* TRUNK means use p0brcnt in proc table. */
#define TRUNK

/* rel 9.1 */
#ifdef R91
/* OLDIO means equivalently mapped port directory */
#define OLDIO
#endif R91

#ifdef RVAX
/* Top of trunk without stack unwind (vax) */
#define KLUDGE_UNWIND
#endif RVAX

#ifdef R10
/* nothing special */
#endif R10

#ifdef R11
/* nothing special */
#endif R11

/* New network buffer manager */
#define NEWBM

#ifdef UNDERSCORE
/* for IND compatibility only */
#ifndef OLD_NNC
#define OLD_NNC
#endif  OLD_NNC
#endif UNDERSCORE

/* define DEBUG if you want debugging output */
/* #define DEBUG */
