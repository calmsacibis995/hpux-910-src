/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/pte.h,v $
 * $Revision: 1.5.84.5 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/07/20 10:57:42 $
 */
/* @(#) $Revision: 1.5.84.5 $ */       
#ifndef _MACHINE_PTE_INCLUDED
#define _MACHINE_PTE_INCLUDED

/* generic pte related macros */

#define	dirty(pte)	( (pte)->pg_pfnum && (pte)->pg_m )

#define	LPTESIZE	2		/* log2(sizeof(struct pte)) */

#ifndef  LOCORE
#define SEGTABSIZE	512
#define MC68040PTSIZE	256

/* segment table entry for three level table walks */

struct ste3
{
unsigned int	sg_pfnum:23,		/* block table frame number */
		:5,			/* reserved at 0 */
		sg_ref:1,		/* hardware reference bit */
		sg_prot:1,		/* write protect bit */
		sg_v:2;			/* valid bits */
};

/* block table entry for three level table walks */

struct bte
{
unsigned int	bt_pfnum:24,		/* page table frame number */
		:4,			/* reserved at 0 */
		bt_ref:1,		/* hardware reference bit */
		bt_prot:1,		/* write protect bit */
		bt_v:2;			/* valid bits */
};


/* WOPR segment table entry */

struct ste
{
unsigned int	sg_pfnum:20,		/* page table frame number */
		:8,			/* reserved at 0 */
		:1,			/* reserved at 1 */
		sg_prot:1,		/* write protect bit */
		sg_v:2;			/* valid bits */
};

/* WOPR page table entry */
struct pte
{
unsigned int	pg_pfnum:20,		/* page frame number or 0 */
		pg_notify:1,		/* want notification of xlation change*/
		pg_ropte:1,		/* marks ro version of ipte */
#ifdef UMEM_DRIVER
		pg_umem:1,		/* page monitored by umem driver */
		:2,                     /* Unused for now */
#else 
		:3,                     /* Unused for now */
#endif
		pg_ci:1,                /* cache inhibit bit */
		pg_cm:1,                /* cache mode bit (mc68040 only) */
		pg_m:1,			/* hardware modified (dirty) bit */
		pg_ref:1,		/* hardware reference bit */
		pg_prot:1,		/* write protect bit */
		pg_v:2;			/* valid bit */
};

extern int three_level_tables;
extern int indirect_ptes;
extern int hil_pbase;

#endif /* LOCORE */

/* Various defines dealing with segment table entries */
#define SG_V		0x00000002	/* ste valid bit */
#define	SG_PROT		0x00000004	/* mask for write protect bit */
#define SG_RO		0x00000004	/* ste write protect */
#define SG_RW		0x00000000	/* ste read/write */
#define	SG_FRAME	0xfffff000
#define SG_IMASK	0xffc00000	/* mask for segment index field */
#define SG_PMASK	0x003ff000	/* mask for ste page index field */
#define SG_ISHIFT	22		/* shift for segment index field */
#define SG_PSHIFT	12		/* shift for segment page field */
#define BT_V SG_V
#define SG3_FRAME       0xfffffe00      /* page frame number for ste */
#define SG3_IMASK       0xfe000000      /* mask for segment index field */
#define SG3_ISHIFT      25              /* shift for segment index field */
#define SG3_PMASK       0x0003f000      /* mask for ste page index field */
#define SG3_BMASK       0x01fc0000      /* mask for block index field */
#define SG3_BSHIFT      18              /* shift for block index field */
#define BLK_FRAME       0xffffff00      /* page frame number for bte */

/* Various defines dealing with page table entries */
#define	PG_V		0x00000001
#define PG_IV		0x00000002	/* indirect pte valid bit */
#define	PG_PROT		0x00000004	/* mask for write protect bit */
#define PG_REF		0x00000008	/* Hardware referenced flag */
#define	PG_ROPTE	0x00000400	/* identifies the read-only pte */
#define PG_NOTIFY	0x00000800	/* used by VDMA devices */
#define	PG_RO		0x00000004
#define	PG_RW		0x00000000
#define	PG_M		0x00000010
#define	PG_FRAME	0xfffff000
#define	PG_CI		0x00000040
#define PG_CB           0x00000020
#define PG_NS           0x00000020
#define	PG_PFNUM(x)	(((x) & PG_FRAME) >> PGSHIFT)
#define	PG_INCR		0x00001000
#define INVALID		0x00000000
#ifdef UMEM_DRIVER
#define PG_UMEM		0x00000200
#endif 

#define ttlbva(HIPAGE) ((caddr_t)(pctopfn((HIPAGE)+hil_pbase) << PGSHIFT))
#define hipage(TTLBVA) ((pfntopc((TTLBVA)-hil_pbase) >> PGSHIFT))
#define prplva(PRP)	((PRP)->p_vaddr + ptob((PRP)->p_count) -1)

			/* XXX VANDYS SZPT assumes 4K page size? */
    /* Get segment index from virtual addr */
#define MC68040_SEGIDX(v) ( (((unsigned long)v) & SG3_IMASK) >> SG3_ISHIFT )

    /* Get offset into block table from virtual addr */
#define MC68040_BTIDX(v) ( (((unsigned long)v) & SG3_BMASK) >> SG3_BSHIFT )

    /* Get segment index from virtual addr */
#define MC68030_SEGIDX(v) ( (((unsigned long)v) & SG_IMASK) >> SG_ISHIFT )

    /* Get segment offset from virtual addr */
#define MC68040_SEGOFF(v) (((((unsigned long)v)&SG3_IMASK) >> SG3_ISHIFT) << 2 )

    /* Get offset into block table from virtual addr */
#define MC68040_BTOFF(v) (((((unsigned long)v)&SG3_BMASK) >> SG3_BSHIFT) << 2 )

    /* Get offset into page table from virtual addr */
#define MC68040_PTOFF(v) (((((unsigned long)v) & SG3_PMASK) >> SG_PSHIFT) << 2 )

    /* Get segment index from virtual addr */
#define MC68030_SEGOFF(v) (((((unsigned long)v) & SG_IMASK) >> SG_ISHIFT) << 2 )

    /* Get offset into page table from virtual addr */
#define MC68030_PTOFF(v) (((((unsigned long)v) & SG_PMASK) >> SG_PSHIFT) << 2 )

#define MC68030_LREF(PRP, SGOFF) (((PRP)->p_prev != (preg_t *)(PRP)->p_vas) && \
	(MC68030_SEGOFF(prplva(prp->p_prev)) == (SGOFF)))

#define MC68030_RREF(PRP,SGOFF) (((PRP)->p_next != (preg_t *)(PRP)->p_vas) && \
	(MC68030_SEGOFF((PRP)->p_next->p_vaddr) == (SGOFF)))

#define SEGOFF(v) (three_level_tables ? MC68040_SEGOFF(((unsigned long)v)) :   \
		                        MC68030_SEGOFF(((unsigned long)v)))

#define MC68040_LREF(PRP, SGOFF, BLKOFF) 				\
	(								\
		((PRP)->p_prev != (preg_t *)(PRP)->p_vas) && 		\
		(MC68040_SEGOFF(prplva(prp->p_prev)) == (SGOFF)) &&	\
		(MC68040_BTOFF(prplva(prp->p_prev)) == (BLKOFF))	\
	)

#define MC68040_RREF(PRP, SGOFF, BLKOFF) 				\
	(								\
		((PRP)->p_next != (preg_t *)(PRP)->p_vas) && 		\
		(MC68040_SEGOFF((PRP)->p_next->p_vaddr) == (SGOFF)) &&	\
		(MC68040_BTOFF((PRP)->p_next->p_vaddr) == (BLKOFF))	\
	)

#define vastopte(VAS, VADDR) \
	((struct pte *)(tablewalk((VAS)->va_hdl.va_seg, (VADDR))))

#define vtopte(PRP, OFFSET) ((struct pte *)				\
	(tablewalk(((vas_t *)((preg_t *)PRP)->p_space)->va_hdl.va_seg,	\
		(((preg_t *)PRP)->p_vaddr+(OFFSET)))))

#define vastoipte(VAS, VADDR) \
	((struct pte *)(itablewalk((VAS)->va_hdl.va_seg, (VADDR))))

#define vtoipte(PRP, OFFSET) ((struct pte *)				\
	(itablewalk(((vas_t *)((preg_t *)PRP)->p_space)->va_hdl.va_seg,	\
		(((preg_t *)PRP)->p_vaddr+(OFFSET)))))

#define IPTE_CHECK(PRP, PFD)						\
	VASSERT((PRP)->p_hdl.p_hdlflags & PHDL_IPTES);			\
	VASSERT(((PRP)->p_hdl.p_hdlflags & PHDL_DPTES) == 0);		\
	VASSERT(((PFD)->pf_hdl.pf_bits & PFHDL_DPTES) == 0);		\
	VASSERT((PFD)->pf_hdl.pf_ro_pte.pg_m == 0);			\
	VASSERT((PFD)->pf_hdl.pf_ro_pte.pg_prot);

#ifndef LOCORE
#ifdef _KERNEL

#define END_PT_PAGE(PT) (three_level_tables ? 				\
			(((u_int)(PT) & (MC68040PTSIZE-1)) == 0) :	\
			(((u_int)(PT) & (NBPG-1)) == 0))

#define NBTEBT  128     /* # of block table entries per block table */
#define NPTEPT	(three_level_tables ? 64 : 1024)

/*
 * Given a potentially unaligned virtual address, return the number
 *  of bytes from that address to the end of the page.
 */
#define BYTES_ON_PAGE(v) ( NBPG - (((unsigned)v) % NBPG) )

extern int three_level_tables;

/* utilities defined in locore.s */
extern struct ste Syssegtab[];
#endif /* _KERNEL */
#endif /* not LOCORE */

#define NBBT    512
#define NBPT    (4096)
#define NBPT3   (256)

#define B32MB   ((unsigned int)0x2000000)
#define bto32M(NBYTES)                                                  \
		((caddr_t)((((unsigned int)NBYTES)+B32MB-1) & ~(B32MB-1)))

#define stetop(ste)    	((*(int *)(ste)) & SG_FRAME)
#define stetobt(ste3)  	((*(int *)(ste3)) & SG3_FRAME)
#define ste3top stetobt
#define btetop(bte)    	((*(int *)(bte)) & BLK_FRAME)

#ifndef NULL
#define NULL	0
#endif

#endif /* _MACHINE_PTE_INCLUDED */
