/*
 * @(#)nipc_protosw.h: $Revision: 1.2.83.4 $ $Date: 93/09/17 19:11:14 $
 * $Locker:  $
 */

#ifndef NIPC_PROTOSW.H
#define NIPC_PROTOSW.H


struct nipc_protosw {
	int			np_protocol;
	int			np_addr_len;
	u_short			np_grp_service;
	u_short			np_service_map;
	int			(*np_proto_addr)();
	struct protosw		*np_bsd_protosw;
	struct nipc_domain	*np_domain;
	int			np_type;
};
#endif /* NIPC_PROTOSW.H */
