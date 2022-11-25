/*
 * @(#)nipc_cb.h: $Revision: 1.2.83.4 $ $Date: 93/09/17 19:09:37 $
 * $Locker:  $
 */

#ifndef NIPC_CB.H
#define NIPC_CB.H

struct nipccb {
	int			n_type;
	int			n_flags;
	struct nipc_protosw	*n_protosw;
	caddr_t			n_name;
	int			n_dest_type;
	struct mbuf		*n_cspr;
	short			n_send_thresh;
	short			n_recv_thresh;
};

#define NF_ISCONNECTED	0x00000001
#define NF_COLLISION	0x00000002
#define NF_MESSAGE_MODE	0x00000004
#endif /* NIPC_CB.H */
