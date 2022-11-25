/*
 * @(#)nipc_domain.h: $Revision: 1.2.83.4 $ $Date: 93/09/17 19:09:49 $
 * $Locker:  $
 */

#ifndef NIPC_DOMAIN.H
#define NIPC_DOMAIN.H


struct nipc_domain {
	short int		nd_domain;	/*Netipc domain id */
	struct nipc_protosw	*nd_protosw;
	struct nipc_protosw	*nd_protoswNPROTOSW;
	int			(*nd_reportvalid)();
	struct mbuf *		(*nd_buildcsite)();
	int 			(*nd_getcsiteaddr)();
	int			(*nd_getnodeaddr)();
	struct mbuf *		(*nd_nodetocsite)();
	struct nipc_domain	*nd_next_domain;
};

#define NIPC_DOMAIN_MASK	0x00ff	/* mask domain id from nd_domain */

/* Netipc domain id */
#endif /* NIPC_DOMAIN.H */
