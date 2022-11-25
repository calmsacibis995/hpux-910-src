/*
 * $Header: if_probe.h,v 1.3.83.4 93/09/17 19:02:25 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/if_probe.h,v $
 * $Revision: 1.3.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:02:25 $
 */

#ifndef	_SYS_IF_PROBE_INCLUDED
#define	_SYS_IF_PROBE_INCLUDED

#ifndef NS_MAX_NODE_NAME
#define NS_MAX_NODE_NAME	32
#endif

#define	PRB_NUMMCASTS	2	/* primary and proxy multicasts addresses */
#define PRB_VNA		0	/* index into multicast array */
#define PRB_NAME	0	/* index into multicast array */
#define PRB_PROXY	1	/* index into multicast array */

struct	prb_hdr {
	u_char	ph_version;		/* Version number */
	u_char	ph_type;		/* Message type */
	short	ph_len;			/* Length of packet in bytes */
	u_short	ph_seq;			/* Packet sequence number */
};
#define	PRB_PHDRSIZE	sizeof(struct prb_hdr)

/* probe ph_version field */
#define PHV_VERSION	0		/* Version of probe */

/* probe ph_type flags */
#define PHT_NAMEREQ	0x10		/* probe name request */
#define	PHT_NAMEREP	0x13		/* probe name reply */
#define	PHT_USNAMEREP	0x15		/* probe unsolicited name reply */
#define	PHT_VNAREQ	0x11		/* probe vna request */
#define	PHT_VNAREP	0x14		/* probe vna reply */
#define PHT_PROXYREQ	0x17		/* probe proxy request */
#define PHT_PROXYREP	0x18		/* probe proxy reply */
/*
 * The following three packet types are NOT supported, but defined here
 * for consistency.
 */
#define PHT_GTWYREQ	0x12		/* probe where-is-gateway request */
#define PHT_GTWYREP	0x16		/* probe where-is-gateway reply */
#define PHT_NODEDOWN	0x19		/* probe unsolicited node-down msg */

struct	prb_vnareq {
	short prq_replen;		/* Report length */
	short prq_dreplen;		/* Domain report length */
	u_char prq_version;		/* Version number */
	u_char prq_domain;		/* Domain */
	u_char prq_iaddr[4];		/* Internet address */
};
#define PRB_VNAREQSIZE	sizeof(struct prb_vnareq)
#define PRQR_REPLEN	0x08
#define PRQD_DREPLEN	0x06
#define PRQV_VNAVERSION	0x00

struct probestat {
	int prb_seqnomissing;		/* Missing seq number */
	int prb_nombufs;		/* Out of space */
	int prb_lesshdr;		/* packet too short */
	int prb_badfield;		/* Unsupported type */
	int prb_badseq;			/* Bad sequence number */
};

struct	ntab {
	char nt_name[NS_MAX_NODE_NAME];	/* Node name */
	struct mbuf *nt_path;		/* Nodal path report */
	u_char nt_flags;		/* flags */
	u_char nt_rtcnt;		/* max. number of retransmits */
	u_short nt_timer;		/* seconds since creation */
	u_short nt_refcnt;		/* users reference count */
	u_short nt_seqno;		/* Sequence number */
	u_short nt_state;		/* FSM state */
	int nt_nlen;			/* Node name length */
	u_short nt_nrqlen;		/* Name Req. length (total len) */
	short nt_ntype;			/* Name request name's type */
	u_long nt_timer2;		/* used to retransmit every min */
};

#define NT_RETRANS	2		/* max number of retransmission */
#define NT_SLOTS	4		/* num of slots in one retrans cycle */
					/* MUST be a power of 2! */

/* nt_flags field values */
#define	NTF_INUSE	0x0001		/* entry in use */
#define NTF_COM		0x0002		/* completed entry */
#define NTF_DELETE	0x0004		/* delete entry when (nt_refcnt == 0) */

/* nt_state field values */
#define NTS_NULL	0		/* null state */
#define NTS_INCOMP	1		/* incomplete state */
#define NTS_HOLD	2		/* hold down state */
#define NTS_COMP	3		/* completed state */

struct prb_nmreq {
	u_char nrq_type;		/* Name type */
	u_char nrq_nlen;		/* Name length in bytes */
	char nrq_name[NS_MAX_NODE_NAME];/* Node name */
};
#define PRB_NMREQSIZE		sizeof(struct prb_nmreq)

/* Name types */
#define NRQT_NMNODE	0x01
/*
 * The following request types are NOT supported, but defined here
 * for consistency
 */
#define NRQT_NMIPC	0x02
#define NRQT_NMDOMAIN	0x03

/* structs used for the domain report attached to a name/proxy table entry. */
struct proxy_vna {
	u_char version;
	u_char domain;
	u_char ipaddr[4];
};

struct proxy_services {
	u_char pid;
	u_char elem_len;
	u_short service_mask;
};

struct proxy_path {
	u_char xport_grp_pid;
	u_char xport_grp_elem_len;
	u_short xport_grp_service_mask;
	u_char xport_pid;
	u_char xport_elem_len;
	u_short xport_sap;
	u_char link_pid;
	u_char link_elem_len;
	u_short link_sap;
};

struct prb_pr {
	struct proxy_services services;
	struct proxy_path path;
};

struct proxy_dpath {
	short d_pathlen;
	struct prb_pr path_rep;
};

struct proxy_drep {
	struct proxy_vna vna;
	struct proxy_dpath d_path;
};

struct proxy_dom {
	short d_replen;
	struct proxy_drep d_rep;
};

struct prb_pathrep {
	short rep_len;
	struct proxy_dom dom;
};

/* struct used to pass ioctl to proxy server */
struct prxentry {
	u_char nodename[NS_MAX_NODE_NAME];	/* node name */
	int name_len;				/* length of node name */
	struct in_addr ipaddr;			/* IP address */
	u_char medium;				/* 802.3 or Ethernet */
	u_char domain;
};
#endif	/* _SYS_IF_PROBE_INCLUDED */
