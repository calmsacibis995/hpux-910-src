/* $Header: cko.h,v 1.4.83.4 93/09/17 18:59:54 kcs Exp $ */

#ifndef	_SYS_CKO_INCLUDED
#define	_SYS_CKO_INCLUDED

/*
 *  HP : CKO - Checksum Offload
 *
 *  (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1992. ALL RIGHTS
 *  RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 *  REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 *  THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 *
 */

#ifdef _KERNEL
/*
 * CKO : Checksum Offload control block
 */
struct cko_info {
    u_short	cko_type;		/* Checksum assist control field   */
#define CKO_INSERT	0x0001		
#define CKO_OUTBOUND 	0x0002	
#define	CKO_CMD		0x0004
#define CKO_ALGO_NONE	0x0008
#define CKO_ALGO_UDP	0x0010
#define CKO_ALGO_TCP	0x0020
#define CKO_SUM_FLUSH	0x0040		/* Flush checksum in card 	*/
    u_short	cko_start;		/* Checksum starting offset 	*/
    u_short	cko_stop;		/* Checksum stop offset 	*/
    u_short	cko_insert;		/* Checksum insert offset 	*/
};

#define	CKO_SUM_MASK	    0xffff	/* sum is in lower 16 bits 	*/

#define	CKO_IPQSUM_OFF		 0	/* ipq_sum is invalid 		*/
#define	CKO_IPQSUM_ON	0x40000000	/* ipq_sum is valid   		*/

#define CKO_SAVE(m, save)	(save) = (m)->m_quad[MQ_CKO_IN];
#define CKO_RESTORE(m, save) 	(m)->m_quad[MQ_CKO_IN] = (save);

/* CKO_SET : setup values for checksum offload control block */
#define	CKO_SET(cko, start, stop, offset, cntl) { 			\
	((struct cko_info *)(cko))->cko_type   = (u_short) (cntl); 	\
	((struct cko_info *)(cko))->cko_start  = (u_short) (start);	\
	((struct cko_info *)(cko))->cko_stop   = (u_short) (stop); 	\
	((struct cko_info *)(cko))->cko_insert = (u_short) (offset); 	\
}

/* CKO_ADJ : adjust offsets in cko_info after pre-pending a header */
#define CKO_ADJ(cko, amt) {						\
	((struct cko_info *)(cko))->cko_start  += (amt);		\
	((struct cko_info *)(cko))->cko_stop   += (amt);		\
	((struct cko_info *)(cko))->cko_insert += (amt); 		\
}

/* CKO_MOVE : Move cko_info into 1st mbuf */
#define CKO_MOVE(m,n) {							\
	n->m_quad[MQ_CKO_OUT0] = m->m_quad[MQ_CKO_OUT0];		\
	n->m_quad[MQ_CKO_OUT1] = m->m_quad[MQ_CKO_OUT1];		\
	n->m_flags |= MF_CKO_OUT;					\
	m->m_quad[MQ_CKO_OUT0] = m->m_quad[MQ_CKO_OUT1] = 0;		\
	m->m_flags &= ~MF_CKO_OUT;					\
}

/******************
*  The macros below are Internet specific.
*******************/

/* CKO_PSEUDO : Account for pseudo_header in the checksum 		*/
#define CKO_PSEUDO(sum, m, hdr) {					\
	(sum) = 0xffff ^ in_3sum( ((struct ipovly *)(hdr))->ih_src.s_addr, \
		((struct ipovly *)(hdr))->ih_dst.s_addr, 		\
		((struct ipovly *)(hdr))->ih_len 			\
		+ (((struct ipovly *)(hdr))->ih_pr<<16) 		\
		+ ((m)->m_quad[MQ_CKO_IN] & CKO_SUM_MASK));		\
}

/* CKO_PSUM : Accumulate partial sum for fragments */
#define CKO_PSUM(m, psum) {						\
	if ((psum) & CKO_IPQSUM_ON)					\
		(psum) += m->m_quad[MQ_CKO_IN] & CKO_SUM_MASK;		\
	else								\
		(psum) = 0;						\
}

/* CKO_CARRY : Fold carry bit into cumulated partial sum of fragments 	*/
#define CKO_CARRY(m, psum) {						\
	if ((psum) & CKO_IPQSUM_ON) {					\
		m->m_quad[MQ_CKO_IN] = in_2sum((psum) & CKO_SUM_MASK,	\
				(psum) & ~(CKO_IPQSUM_ON|CKO_SUM_MASK));\
		m->m_flags  |= MF_CKO_IN;				\
	}								\
	else {								\
		m->m_quad[MQ_CKO_IN] = 0;				\
		m->m_flags  &= ~MF_CKO_IN;				\
	}								\
}
		
/* CKO_LOOPBACK : used by loopback driver				
*	Place pseudo-header sum in mquad[MQ_CKO_IN], so when layer 4 
*	protocol input routine does the checksum, it will come out zero.
*
*  CKO_LOOPBACK code does not contain Internet specific info, but the
*	need for this macro arises because of the Internet pseudo-header
*	implanted by Internet transport protocols.
*/
#define CKO_LOOPBACK(m) {						\
	if ((m)->m_flags & MF_CKO_OUT) {				\
	    int	offset;							\
	    offset = ((struct cko_info *)&((m)->m_quad[MQ_CKO_OUT0]))->cko_insert;\
	    (m)->m_quad[MQ_CKO_IN] = 0xffff ^ 				\
			(*(u_short *) (mtod((m),int)+offset));		\
	    (m)->m_flags = ((m)->m_flags & ~MF_CKO_OUT) | MF_CKO_IN;	\
	}								\
}

extern cko_cksum();
extern cko_fixup();

#endif	/* _KERNEL */
			
#endif	/* _SYS_CKO_INCLUDED */
