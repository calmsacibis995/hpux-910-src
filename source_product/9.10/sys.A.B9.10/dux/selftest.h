/*
 * @(#)selftest.h: $Revision: 1.5.83.3 $ $Date: 93/09/17 16:46:01 $
 * $Locker:  $
 */

/* HPUX_ID: %W%		%E% */

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


/*
 * This file contains the definitions used for performing the automatic
 * selftest
 */

/*The following are the types of selftest performed*/

#define ST_BUFFER	0x1	/*Buffer test */
#define ST_NETBUF	0x2	/*Network buffer test*/
#define ST_DM_MESSAGE	0x4	/*DM message test*/
#define ST_NSP		0x8	/*NSP test*/

#ifdef __hp9000s300
#define ST_ALL (ST_BUFFER|ST_NETBUF|ST_DM_MESSAGE|ST_NSP)
#else
#define ST_ALL (ST_BUFFER|ST_DM_MESSAGE|ST_NSP)
#endif /* __hp9000s300 */

/*The following variable is used to record passing of a test.  When an
 *operation is performed, the appropriate bit above is set in it.
 */
#ifdef VOLATILE_TUNE
extern volatile int selftest_passed;
#else
extern int selftest_passed;
#endif /* VOLATILE_TUNE */

/*The following macro sets the bit*/

#define TEST_PASSED(test) (selftest_passed |= (test))
