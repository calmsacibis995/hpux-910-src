
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

/* This file is a subset of net.h, used only in gram.y, because of
 * a cc internal limit.  The definitions here are duplicates of those
 * in net.h
 */

#include <sys/mbuf.h>
extern struct mbuf *mbuf_memory;
#define vmbtocl(v)     (mtocl(vmtod(v)&~NETCLOFSET))/* virtual mbtocl()	*/
