/* $Header: mbuf.h,v 1.32.83.4 93/09/17 18:29:18 kcs Exp $ */

#ifndef	_SYS_MBUF_INCLUDED
#define	_SYS_MBUF_INCLUDED
/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)mbuf.h	7.10 (Berkeley) 2/8/88 plus MULTICAST 1.1
 */
  

#ifdef	_KERNEL_BUILD
#include "../h/malloc.h"
#include "../h/uio.h"
#include "../machine/param.h"
#else	/* ! _KERNEL_BUILD */
#include <sys/uio.h>
#include <machine/param.h>
#endif	/* _KERNEL_BUILD */

/*
 * Constants related to memory allocator.
 */

#define	MSIZE		256			/* size of an mbuf */
#define	MCLBYTES	NBPG			
#define	MCLSHIFT	PGSHIFT
#define	MCLOFSET	(MCLBYTES - 1)

#define	MMINOFF		0			/* offset where data begins */
#define	MTAIL		32			/* header length */
#define	MMAXOFF		(MSIZE-MTAIL)		/* offset where data ends */
#define	MLEN		(MSIZE-MMINOFF-MTAIL)	/* mbuf data length */
#define	NMBPCL		(MCLBYTES/MSIZE)	/* # mbufs per cluster */

#ifdef	m_flags
#undef  m_flags         /* avoid collision with m_flags in sysmacros.h */
#endif
/*
 * Macros for type conversion
 */

/* address in mbuf to mbuf head */
#define	dtom(x)		((struct mbuf *)((int)(x) & ~(MSIZE-1)))

/* mbuf head, to typed data */
#define	mtod(x,t)	((t)((int)(x) + (x)->m_off))

struct mbuf {
        union {
                u_char  mun_dat[MLEN];  /* data storage */
		struct mcluster {
                        u_short	mun_cltype;     /* "cluster" type */
			u_short mun_clsize;	/* "cluster" size */
			struct  mbuf *mun_clhead;
			int     mun_clrefcnt;   
			union  {
                		struct { 
                        		int     (*mun_clfun)();
                        		int     mun_clarg;
                		} mun_cl_nfs;
#ifdef	__hp9000s800
                		struct { /* clhead & clrefcnt not used */
					int	(*mun_dux_clfun)();
					int	mun_dux_clarg;
					space_t mun_sid;
                                        u_long  mun_off;
				} mun_cl_dux;
#endif	/* __hp9000s800 */
                		struct {
                        		int     (*mun_osx_clfun)();
                        		int     mun_osx_clarg;
                		} mun_cl_osx;
#ifdef	__hp9000s800
				struct {	/* For copy-on-write */
					struct ioops	*mun_ops_ops;
					caddr_t		mun_ops_arg;
				} mun_cl_ops;
#endif	/* __hp9000s800 */

/* description of external storage mapped into mbuf, valid if MF_EXT set
 * (and cltype is MCL_EXT).  similar to 4.4BSD, but we use HP clhead and
 * clrefcnt mechanism instead of ext_ref list struct.
 */
				struct {
				   void    (*ext_clfun)();
				   caddr_t ext_clarg;   /* addl clfun arg */
				   caddr_t ext_buf;     /* start of buffer */
				   u_long  ext_size;    /* ext_buf size */
				} mun_ext;
			} mun_cl;
		} m_cluster; 
        } m_un;

	struct	mbuf *m_next;		/* next buffer in chain */
	u_long	m_off;			/* offset of data */
	short	m_len;			/* amount of data in this mbuf */
	u_char	m_type;			/* mbuf type (0 == free) */
        u_char  m_flags;                /* MF_XX flag values -- see below */
	struct	mbuf *m_act;		/* link in higher-level mbuf list */
	int	m_quad[4];		/* for i/o usage */
};

/* index for m_quad usage */
#define	MQ_IFP		0	/* ARP/Probe/IF_ENQUEUE_IF:interface pointer */
#define	MQ_RIF8025	2	/* ARP : 802.5 source routing info */
#define	MQ_CKO_IN	3	/* cko_info inbound checksum */
#define	MQ_CKO_OUT0	2	/* cko_info outbound, 1st word */
#define	MQ_CKO_OUT1	3	/* cko_info outbound, 2nd word */

#define m_dat         m_un.mun_dat
#define m_cltype      m_un.m_cluster.mun_cltype
#define m_clsize      m_un.m_cluster.mun_clsize
#define m_head        m_un.m_cluster.mun_clhead
#define m_refcnt      m_un.m_cluster.mun_clrefcnt
#define m_clfun       m_un.m_cluster.mun_cl.mun_cl_nfs.mun_clfun
#define m_clarg       m_un.m_cluster.mun_cl.mun_cl_nfs.mun_clarg
#define m_sid         m_un.m_cluster.mun_cl.mun_cl_dux.mun_sid
#define m_sid_off     m_un.m_cluster.mun_cl.mun_cl_dux.mun_off
#define m_nfs_head    m_head
#define m_nfs_refcnt  m_refcnt
#define m_dux_clfun   m_un.m_cluster.mun_cl.mun_cl_dux.mun_dux_clfun
#define m_dux_clarg   m_un.m_cluster.mun_cl.mun_cl_dux.mun_dux_clarg
#define m_osx_head    m_head
#define m_osx_refcnt  m_refcnt
#define m_osx_clfun   m_un.m_cluster.mun_cl.mun_cl_osx.mun_osx_clfun
#define m_osx_clarg   m_un.m_cluster.mun_cl.mun_cl_osx.mun_osx_clarg
#define m_ops_head    m_head
#define m_ops_refcnt  m_refcnt
#define m_ops_ops     m_un.m_cluster.mun_cl.mun_cl_ops.mun_ops_ops
#define m_ops_arg     m_un.m_cluster.mun_cl.mun_cl_ops.mun_ops_arg

#define m_ext_head    m_head    /* 8.0 compatibility */
#define m_ext_refcnt  m_refcnt  /* 8.0 compatibility */
#define m_ext_clsize  m_clsize  /* 8.0 compatibility */
#define m_ext_clfun   m_un.m_cluster.mun_cl.mun_ext.ext_clfun
#define m_ext_clarg   m_un.m_cluster.mun_cl.mun_ext.ext_clarg
#define m_ext_buf     m_un.m_cluster.mun_cl.mun_ext.ext_buf
#define m_ext_size    m_un.m_cluster.mun_cl.mun_ext.ext_size

/*cluster types */
#define MCL_NORMAL	1
#define MCL_NFS		2
#define MCL_DUX		3
#define MCL_STREAMS	4
#define MCL_OPS		5
#define MCL_EXT         6

/* mbuf types */
#define	MT_FREE		0	/* should be on free list */
#define	MT_DATA		1	/* dynamic (data) allocation */
#define	MT_HEADER	2	/* packet header */
#define	MT_SOCKET	3	/* socket structure */
#define	MT_PCB		4	/* protocol control block */
#define	MT_RTABLE	5	/* routing tables */
#define	MT_HTABLE	6	/* IMP host tables */
#define	MT_ATABLE	7	/* address resolution tables */
#define	MT_SONAME	8	/* socket name */
#define	MT_ZOMBIE	9	/* zombie proc status */
#define	MT_SOOPTS	10	/* socket options */
#define	MT_FTABLE	11	/* fragment reassembly header */
#define	MT_RIGHTS	12	/* access rights */
#define	MT_IFADDR	13	/* interface address */
#define	MT_IPMOPTS	14	/* internet multicast options */
#define	MT_IPMADDR	15	/* internet multicast address */
#define	MT_IFMADDR	16	/* link-level multicast address */
#define	MT_MRTABLE	17	/* multicast routing tables */
#define	MT_IPCCB	18	/* IPC specific control block */
#define	MT_TOTAL	19	/* total number of stats */

/* mbuf flags */
#define MF_EOM          0x1     /* end of message */
#define	MF_CKO_IN	0x2	/* inbound checksum assist in quad area */
#define	MF_CKO_OUT	0x4	/* outbound checksum assist in quad area */
#define MF_NOACC      	0x8	/* data has not been accessed */
#define MF_NODELAYFREE	0x10	/* don't defer the freeing of the mbuf */
#define MF_EXT          0x20    /* extended cluster record exists */

/* flags to m_get */
#define	M_DONTWAIT	M_NOWAIT
#define	M_WAIT		M_WAITOK

/* length to m_copy to copy all */
#define	M_COPYALL	1000000000

/*
 * m_pullup will pull up additional length if convenient;
 * should be enough to hold headers of second-level and higher protocols. 
 */
#define	MPULL_EXTRA	32

#ifdef  __hp9000s800
#define WORD_ALIGNED	0x3
#endif
#ifdef  __hp9000s300
#define WORD_ALIGNED	0x1
#endif

#define M_UNALIGNED(m)	((m)->m_off & WORD_ALIGNED)

#define	MGET(m, canwait, t) { 				\
	MALLOC((m), struct mbuf*, MSIZE, M_MBUF, canwait);\
	if (m) {					\
		int s = splimp();			\
		mbstat.m_mtypes[t]++;	 		\
		splx(s);				\
		(m)->m_type = t;			\
		(m)->m_act  = 0;			\
		(m)->m_next = 0;			\
		(m)->m_flags = 0;			\
		(m)->m_off  = MMINOFF;			\
	  } 						\
}

/*
 * Mbuf page cluster macros.
 * MCLGET adds such clusters to a normal mbuf.
 * m->m_len is set to MCLBYTES upon success, and to MLEN on failure.
 * m_free frees clusters allocated by MCLGET
 */

#define MCLALLOCVAR(p,len,clsize,canwait) {				\
	if ((len) > MCLBYTES/4) {					\
		if ((len) > MCLBYTES/2) {				\
			MALLOC(p, caddr_t, MCLBYTES,   M_MBUF, canwait);\
			clsize = MCLBYTES;				\
		} else {						\
			MALLOC(p, caddr_t, MCLBYTES/2, M_MBUF, canwait);\
			clsize = MCLBYTES/2;				\
		}							\
	} else {							\
		if ((len) > MCLBYTES/8) {				\
			MALLOC(p, caddr_t, MCLBYTES/4, M_MBUF, canwait);\
			clsize = MCLBYTES/4;				\
		} else {						\
			MALLOC(p, caddr_t, MCLBYTES/8, M_MBUF, canwait);\
			clsize = MCLBYTES/8;				\
		}							\
	}								\
}

#define MCLADD(m,p,len,clsize,cltype) {		\
	int s;                                  \
	(m)->m_off = (int)(p) - (int)(m);	\
	(m)->m_len = len;			\
	(m)->m_head = (m);                      \
	(m)->m_refcnt = 1;                      \
	(m)->m_cltype = cltype;			\
	(m)->m_clsize = clsize;			\
	s = splimp();                           \
	mbstat.m_clbytes += clsize;             \
	mbstat.m_clusters++;                    \
	splx(s);                                \
}

#define	MCLGET(m) { 						\
	caddr_t p;						\
	MALLOC(p, caddr_t, MCLBYTES, M_MBUF, M_NOWAIT); 	\
	if (p) {						\
		MCLADD((m), p, MCLBYTES, MCLBYTES, MCL_NORMAL);	\
	} else {						\
		int s;                                  	\
		(m)->m_len = MLEN;				\
		s = splimp();					\
		mbstat.m_drops++;				\
		splx(s);					\
	}							\
}

#define MCLGETVAR_DONTWAIT(m,len) {	                  	\
	caddr_t p; int clsize;                         		\
	MCLALLOCVAR(p, (len), clsize, M_NOWAIT);		\
	if (p) {						\
		MCLADD((m), p, (len), clsize, MCL_NORMAL);	\
	} else {						\
		int s;                                  	\
		(m)->m_len = MLEN;				\
		s = splimp();					\
		mbstat.m_drops++;				\
		splx(s);					\
	}							\
}

#define MCLGETVAR_WAIT(m,len) {                      	\
	caddr_t p; int clsize;                         	\
	MCLALLOCVAR(p, (len), clsize, M_WAITOK);	\
	MCLADD((m), p, (len), clsize, MCL_NORMAL);	\
}

#define MFREE(m, n)	(n) = m_free(m)

#define	M_HASCL(m)	((m)->m_off >= MSIZE)
#define	MTOCL(m)	((struct mbuf *)(mtod((m), int) &~ (((m)->m_clsize)-1)))

#define MCLARGADDR(m,cast)	((cast)((&(m)->m_un.m_cluster)+1))
#define MCLARGSIZE     		(MLEN-sizeof(struct mcluster))


#define MCLREFERENCED(m)  ((m)->m_head->m_refcnt > 1)


/*
 * Mbuf statistics.
 */
struct mbstat {
        u_long  m_mbufs;        /* mbufs obtained from page pool */
        u_long  m_clusters;     /* clusters obtained from page pool */
        u_long  m_spare;        /* spare field */
        u_long  m_clfree;       /* free clusters */
        u_long  m_drops;        /* times failed to find space */
        u_long  m_wait;         /* times waited for space */
        u_long  m_drain;        /* times drained protocols for space */
        u_long  m_mtypes[256];  /* type specific mbuf allocations */
};
#define m_clbytes m_spare

#ifdef	_KERNEL
extern struct mbstat	mbstat;
extern int		m_want;
extern struct mbuf	*m_get(), *m_getclr(), *m_free(), *m_more();
extern struct mbuf	*m_copy(), *m_pullup();
extern struct mbuf	*mfree;
#endif	/* _KERNEL */

#ifndef	MBUF_QA
/* 
 * The following defines are to be used ONLY by the mbuf code internally.
 */
#define	MCLFREE(m)	FREE(m, M_MBUF)
#define	MBUF_FREE(m)	FREE(m, M_MBUF)
#define INIT_Q(q)			/* nothing */

#if defined(_KERNEL) && defined(__hp9000s800)
#define MCLFREEBUF(m,flag)		\
	IOO_FREEBUF((m)->m_head->m_ops_ops,(m)->m_head->m_ops_arg,flag)

#define MCLCOPYTO(m,off,len,from)	\
	IOO_COPYTO((m)->m_head->m_ops_ops,(m)->m_head->m_ops_arg,mtod(m,caddr_t)+(off),len,from)

#define MCLCOPYFROM(m,off,len,to)	\
	IOO_COPYFROM((m)->m_head->m_ops_ops,(m)->m_head->m_ops_arg,mtod(m,caddr_t)+(off),len,to)

#define MCLPHYSADDR(m,off)		\
	IOO_PHYSADDR((m)->m_head->m_ops_ops,(m)->m_head->m_ops_arg,mtod(m,caddr_t)+(off))

#define MCLFLUSH(m,off,len,cond) 			\
	IOO_FLUSHCACHE((m)->m_head->m_ops_ops,(m)->m_head->m_ops_arg,	\
		mtod(m,caddr_t)+(off),len,cond)

#define MCLPURGE(m,off,len,cond) 			\
	IOO_PURGECACHE((m)->m_head->m_ops_ops,(m)->m_head->m_ops_arg,	\
		mtod(m,caddr_t)+(off),len,cond)
#endif	/* _KERNEL && __hp9000s800 */

#else	/* MBUF_QA defined */		/* you get it all for the price of 1 */

#undef	MGET
#undef	MCLGET

#define	MBQA_RLOSS	0x1		/* random loss enabled */
#define	MBQA_EXHAUST	0x2		/* sizeof mbuf == sizeof cluster */
#define	MBQA_INVAL	0x4		/* unvirtualize at free */

#define	FQ_LEN	64			/* must be power of 2! */
struct free_q {
	caddr_t v_addr[FQ_LEN];
	caddr_t pfn[FQ_LEN];
	int	head, tail, len;
};

#define INIT_Q(q)	(q).head = (q).tail = (q).len = 0
#define	INC_Q(index)	(index) = ++(index) & (FQ_LEN - 1)

#define M_FAIL_THRESH	16
#define	RND_MODULO	128
#define GEN_RAN(s, x)       ((((s) = (s) * 11109 + 13849) & 0xfffffff) % (x))

#define	MGET(m, canwait, type)	(m) = m_get_qa(canwait, type);
#define	MCLGET(m)		mclget_qa(m);
#define MCLFREE(m)		mclfree_qa(m);
#define MBUF_FREE(m)		mbuf_free_qa(m);


#ifdef	_KERNEL
extern struct mbuf	*mbuf_head, *mbuf_tail, *cl_head, *cl_tail;
extern int		mbuf_qa, m_fail, m_seed;
extern struct free_q	free_queue;
extern struct mbuf 	*get_mbuf_q(), *get_mbuf_l(), *get_cl_l();
extern struct mbuf 	*m_get_qa();
extern int 		mclget_qa(), mclfree_qa(), mbuf_free_qa();
#endif

#endif	/* MBUF_QA */
#endif	/* SYS_MBUF_INCLUDED */

