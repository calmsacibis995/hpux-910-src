/*
 * @(#)nipc_hpdsn.h: $Revision: 1.3.83.4 $ $Date: 93/09/17 19:10:20 $
 * $Locker:  $
 */

/*
 *	The HPDSN domain identifier and version number.
 */
#ifndef NIPC_HPDSN.H
#define NIPC_HPDSN.H


#define	HPDSN_DOM 1
#define HPDSN_VER 0

/*
 *	The constants defined below are the protocol identifiers for
 *	the services and transport groups and the individual protocols within
 *	the HPDSN domain.  These are according to the path reports spec.
 */

#define	NSP_SERVICES	255
#define NSP_TRANSPORT	254
#define	NSP_ETHERNET	1
#define	NSP_X25		2
#define	NSP_TCP		4
#define	NSP_HPPXP	6
#define NSP_IEEE802	7
#define	NSP_IP		8

/*
 *	The NSB_protocol constants define the service map bit index for each
 *	protocol in the HPDSN services and transport groups.
 *	The PR_BIT macro defines the bit position according
 *	to the path reports specification. The NSM_ is the mask used in the
 *	path reports for the services and transport.
 */

#define	PR_BIT(x)	(1<<(15 - (x)))	/* short bit posi for path reports */

#define NSB_NFT		PR_BIT(0)
#define NSB_IPCSR	PR_BIT(2)
#define NSB_LOOPBACK	PR_BIT(8)

#define	NSB_TCPCKSUM	PR_BIT(0)
#define	NSB_TCP		PR_BIT(1)
#define	NSB_HPPXP	PR_BIT(2)

#define NSM_SERVICE	(NSB_NFT | NSB_IPCSR | NSB_LOOPBACK)
#define NSM_XPORT	(NSB_TCPCKSUM | NSB_TCP | NSB_HPPXP)
#endif /* NIPC_HPDSN.H */
