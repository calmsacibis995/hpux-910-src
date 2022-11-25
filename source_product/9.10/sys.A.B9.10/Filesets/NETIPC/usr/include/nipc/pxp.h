/*
 *  Header - PXP 
 */
#ifndef PXP.H
#define PXP.H

struct pxphdr {
	u_short	ph_sport;		/* source port */
	u_short	ph_dport;		/* destination port */
	u_short	ph_len;	        	/* total message length in bytes */
	u_short	ph_cksum;		/* checksum, always zero */
        int     ph_msgid;		/* message identification */
	unsigned ph_ver: 3,		/* pxp version number, zero */
		 ph_cls: 2,		/* pxp service class, one */
	   	 ph_rbit: 1,		/* reply bit */
		 ph_ebit: 1, 		/* error bit */
		 ph_rsved: 9;		/* reserved -not used currently */
};

#define	PXP_HDRSIZE 14	/* needed because sizeof() returns wrong value on 800 */
  

struct pxpiphdr {
	struct   ipovly pi_i;		/* overlaid ip structure */
	struct   pxphdr pi_p;		/* pxp header */
};

#define	PXPIP_HDRSIZE (sizeof(struct ipovly) + PXP_HDRSIZE)  


/* The following defines are used to shorten pxpiphdr names */

#define pi_next			pi_i.ih_next
#define pi_prev			pi_i.ih_prev
#define pi_x1			pi_i.ih_x1
#define pi_pr			pi_i.ih_pr
#define pi_len			pi_i.ih_len
#define pi_src			pi_i.ih_src
#define pi_dst			pi_i.ih_dst
#define pi_sport		pi_p.ph_sport
#define pi_dport		pi_p.ph_dport
#define pi_plen			pi_p.ph_len
#define pi_cksum		pi_p.ph_cksum
#define pi_msgid		pi_p.ph_msgid
#define pi_ver			pi_p.ph_ver
#define pi_cls			pi_p.ph_cls
#define pi_rbit			pi_p.ph_rbit
#define pi_ebit			pi_p.ph_ebit
#define pi_rsved		pi_p.ph_rsved

#define PXP_TTL	30

#ifdef __hp9000s800
#define WORD_ALIGN	0x3
#endif

#ifdef __hp9000s300
#define	WORD_ALIGN	0x1
#endif

#endif /* PXP.H */
