/*	@(#)spray.h	$Revision: 1.16.109.1 $	$Date: 91/11/19 14:43:13 $  */ 

/*      (c) Copyright 1987 Hewlett-Packard Company  */
/*      (c) Copyright 1985 Sun Microsystems, Inc.   */

#ifndef _RPCSVC_SPRAY_INCLUDED
#define _RPCSVC_SPRAY_INCLUDED

#define SPRAYPROG 100012
#define SPRAYPROC_SPRAY 1
#define SPRAYPROC_GET 2
#define SPRAYPROC_CLEAR 3
#define SPRAYVERS_ORIG 1
#define SPRAYVERS 1

#define SPRAYOVERHEAD 86	/* size of rpc packet when size=0 */
#define SPRAYMAX 8845		/* related to max udp packet of 9000 */

int xdr_sprayarr();
int xdr_spraycumul();

struct spraycumul {
	unsigned counter;
	struct timeval clock;
};

struct sprayarr {
	int *data;
	int lnth;
};

#endif /* _RPCSVC_SPRAY_INCLUDED */
