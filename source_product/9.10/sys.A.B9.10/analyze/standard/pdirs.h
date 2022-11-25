/*
 * @(#)pdirs.h: $Revision: 1.3.83.3 $ $Date: 93/09/17 16:31:32 $
 * $Locker:  $
 */



#ifdef SPARSE_PDIR2K
/* Change for 2K page */
#define NEWPDE_VPAGE 21
#define NEWPDE_VOFF 11
#define NEWPDE_PPAGE 21
#else
/* Change for 4K page */
#define NEWPDE_VPAGE 20
#define NEWPDE_VOFF 12
#define NEWPDE_PPAGE 20
#endif

struct newhpde {

	unsigned int	pde_page  :NEWPDE_VPAGE;/* Virtual page number */
	unsigned int 	          :NEWPDE_VOFF;
	unsigned int	pde_space :32;	/* Virtual space number */
	unsigned int	pde_ref   :1;	/* Reference bit */
	unsigned int	pde_accessed:1;
	unsigned int	pde_rtrap :1;	/* Data reference trap enable bit */
	unsigned int	pde_dirty :1;	/* Dirty bit */
	unsigned int	pde_dbrk  :1;	/* Data break */
	unsigned int	pde_ar	  :7;	/* Access rights */
	unsigned int		  :2;
	unsigned int	pde_shadow  :1;	/* Shadow */
	unsigned int	pde_executed:1;
	unsigned int	pde_protid  :15;/* Protection ID */
	unsigned int	pde_modified:1;
	unsigned int		  :7;
	unsigned int	pde_phys  :NEWPDE_PPAGE;/* Physical page */
	unsigned int		  :4;
	unsigned int	pde_ref2  :1;	/* Reference bit */
	unsigned int	pde_accessed2:1;
	unsigned int	pde_rtrap2:1;	/* Data reference trap enable bit */
	unsigned int	pde_dirty2:1;	/* Dirty bit */
	unsigned int	pde_dbrk2 :1;	/* Data break */
	unsigned int	pde_ar2	  :7;	/* Access rights */
	unsigned int		  :2;
	unsigned int	pde_shadow2  :1; /* Shadow */
	unsigned int	pde_executed2:1;
	unsigned int	pde_protid2  :15;/* Protection ID */
	unsigned int	pde_modified2:1;
	unsigned int		  :7;
	unsigned int	pde_phys2 :NEWPDE_PPAGE;/* Physical page */
	unsigned int		  :4;
	struct newhpde 	*pde_next;	/* Next entry */
	unsigned int	pde_os;		/* Alias in high bits, usage in low 2*/
};





#ifdef SPARSE_PDIR2K
/* Old pdir structure */
struct oldlle {
	unsigned int    lle_empty :1;       /* Hash chain empty */
	int               :6;
	int     lle_index :21;              /* Index to the first pdir entry */
	int               :4;
};


struct oldpde {
	struct oldlle      Pde_lle;            /* First word is a linked list */
#define old_pde_end Pde_lle.lle_empty
#define old_pde_next Pde_lle.lle_index
	unsigned int    pde_space :32;      /* Virtual space number */

	unsigned int    pde_page  :21;      /* Virtual page number */
	unsigned int              :11;
	unsigned int    pde_ref   :1;       /* Reference bit */
	unsigned int    pde_accessed :1;    /* May be present in dcache */
	unsigned int    pde_rtrap :1;       /* Data reference trap enable */
	unsigned int    pde_dirty :1;       /* Dirty bit */
	unsigned int    pde_dbrk  :1;       /* Data break */
	unsigned int    pde_ar    :7;       /* Access rights */
	unsigned int              :3;
	unsigned int    pde_executed :1;    /* May be present in icache */
	unsigned int    pde_protid   :15;   /*  Protection ID */
	unsigned int    pde_modified :1;    /* May be dirty in cache */
};


/*
 * The hte (hash table entry) structure.
 * The hte is defined in the HP Precison Architecture ACD and is used with
 * the page directory for virtual memory translation.
 */

struct oldhte {				/* PDIR hash table entry */
	struct oldlle Hte_lle;
#define hte_empty Hte_lle.lle_empty
#define hte_index Hte_lle.lle_index
};
#endif

#define oldpgtopde(pg)	(oldpdir + pg)
#define oldpdetopg(p)	(p - oldpdir)
