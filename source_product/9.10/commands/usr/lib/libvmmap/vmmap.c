/* $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libvmmap/vmmap.c,v $
 * $Revision: 70.1 $        $Author: lkc $
 * $State: Exp $        $Locker:  $
 * $Date: 93/03/23 14:04:45 $
 */

/*
 * vmmap.c -- originally taken from netstat.c for the purpose of
 *            grubbing around in core-dump files... this file
 *            is now sync'd with similar routines found in adb.
 */

static char *HPUX_ID = "@(#) $Revision: 70.1 $";

#ifdef LINT
#define volatile
#endif LINT

#ifdef UNI_REL
#include <Uni_rel.h>
#endif /* UNI_REL */

/*
 * vmmap 
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/vmmac.h>
#include <machine/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#ifdef	hp9000s800
#include <machine/pde.h>
#endif

extern int errno;

/* NLS */
#include <nl_types.h>
#include <locale.h>
#define	NL_SETN		1
#define	NL_PDIR_SET	2
#define	NATIVE_COMPUTER "C"

extern char *catgets();
extern char *setlocale();
extern char *getenv();

#define PERROR(nls_catalog_descriptor, set_num, msg_num, def_str)           \
	if (nls_catalog_descriptor == ((nl_catd) -1)) {                         \
		perror(def_str);                                                    \
	} else {                                                                \
		perror(catgets(nls_catalog_descriptor, set_num, msg_num, def_str)); \
	}


#ifdef  SPARSE_PDIR
struct  hpde *htbl, *ahtbl, *vhtbl;
struct  hpde *pdir, *vpdir;
struct  hpde *base_pdir;
struct  hpde *max_pdir;
struct  hpde *vbase_pdir;
struct  hpde *vmax_pdir;
int pdirhash_type = -1;
int scaled_npdir = -1;
#else   /* !SPARSE_PDIR */
struct  pde *pdir, *iopdir, *aiopdir, *vpdir, *viopdir;
struct  hte *htbl, *ahtbl, *vhtbl;
#endif  /* !SPARSE_PDIR */

int *khtbl;
int *kpdir;

#ifdef  hp9000s800

#define N_HTBL          0
#define N_NHTBL         1
#define N_PDIR          2
#define N_NPDIR         3
#define N_BASE_PDIR     4
#define N_MAX_PDIR      5
#define N_IOPDIR        6
#define N_NIOPDIR       7

#ifdef  SPARSE_PDIR

#define N_SCALED_NPDIR  8
#define N_PDIRHASH_TYPE 9
#define NLSZ            10

#else   /* !SPARSE_PDIR */

#define NLSZ            8

#endif  /* SPARSE_PDIR */
#endif /* hp9000s800 */

#ifdef hp9000s300
#define N_DIO_IFT       0
#define NLSZ            1
#endif /* hp9000s200 */


struct nlist pdir_nl [] = {
	{ "htbl"          },
	{ "nhtbl"         },
	{ "pdir"          },
	{ "npdir"         },
	{ "base_pdir"     },
	{ "max_pdir"      },
	{ "iopdir"        },
	{ "niopdir"       },
	{ "scaled_npdir"  },
	{ "pdirhash_type" },
	{ NULL }
};

#include <sys/vmmac.h>


/* 
 * This define enables the sparse pdir code which should handle both the
 * 4K sparse pdir and the 2k old pdir.
#define SPARSE_PDIR 1
 */

#ifdef SPARSE_PDIR
/*
 * All pdir structures are here for the transition since we do not know what
 * will be in the actual header file, and tihs will insulate us. Over time
 * we can migrateto just one definition that is held in pde.h 
 */

/* #define SPARSE_2K 1 for 2K sparse, this is only for debugging on an 840 */

#ifdef SPARSE_2K
/* Change for 2K page */
#define SPARSE_NBPG 11
#define NPDE_VPAGE 21
#define NPDE_VOFF 11
#define NPDE_PPAGE 21
#define NPDE_PAD 4
#else
/* Change for 4K page */
#define SPARSE_NBPG 12
#define NPDE_VPAGE 20
#define NPDE_VOFF 12
#define NPDE_PPAGE 20
#define NPDE_PAD 5
#endif

#define MP_HASH_TYPE 4

struct mp_newhpde {
 
	unsigned int	pde_valid_e  :1;	/* Valid bit */
	unsigned int	pde_page_e  :15;	/* V. page - high 15 bits */
	unsigned int	pde_space_e:16;		/* Virtual space number */
	unsigned int	pde_ref_e :1; /* Reference bit */
	unsigned int	pde_accessed_e :1;	
	unsigned int	pde_rtrap_e  :1; /*Data reference trap enable bit*/
	unsigned int	pde_dirty_e  :1; /* Dirty bit */
	unsigned int	pde_dbrk_e   :1; /* Data break */
	unsigned int	pde_ar_e     :7; /* Access rights */
	unsigned int		     :2;
	unsigned int	pde_shadow_e :1; /* Shadow */
	unsigned int	pde_executed_e :1;	
	unsigned int	pde_protid_e   :15; /* Protection ID */
	unsigned int	pde_modified_e :1;	
	volatile unsigned int	pde_uip_e :1;		/* Update in progress. */
	unsigned int		   :6;
	unsigned int	pde_phys_e :20;  /* Physical page */
	unsigned int		   :5;
	struct hpde 	*pde_next;	/* Next entry */
	unsigned int	pde_valid_o  :1;	/* Valid bit */
	unsigned int	pde_page_o  :15;	/* V. page - high 15 bits */
	unsigned int	pde_space_o:16;		/* Virtual space number */
	unsigned int	pde_ref_o   :1;  /* Reference bit */
	unsigned int	pde_accessed_o :1;	
	unsigned int	pde_rtrap_o  :1; /*Data reference trap enable bit */
	unsigned int	pde_dirty_o  :1; /* Dirty bit */
	unsigned int	pde_dbrk_o   :1; /* Data break */
	unsigned int	pde_ar_o     :7; /* Access rights */
	unsigned int		     :2;
	unsigned int	pde_shadow_o :1; /* Shadow */
	unsigned int	pde_executed_o :1;	
	unsigned int	pde_protid_o   :15;/* Protection ID */
	unsigned int	pde_modified_o :1;	
	volatile unsigned int	pde_uip_o :1;		/* Update in progress. */
	unsigned int		   :6;
	unsigned int	pde_phys_o :20;	/* Physical page */
	unsigned int		   :5;
	unsigned int	pde_os;		/* Alias in high bits, usage in low 2*/
};

struct newhpde {
 
	unsigned int	pde_page  :NPDE_VPAGE;/* Virtual page number */
 	unsigned int 	          :NPDE_VOFF;
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
	unsigned int	pde_phys  :NPDE_PPAGE;/* Physical page */
	unsigned int		  :NPDE_PAD;
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
	unsigned int	pde_phys2 :NPDE_PPAGE;/* Physical page */
	unsigned int		  :NPDE_PAD;
	struct newhpde 	*pde_next;	/* Next entry */
	unsigned int	pde_os;		/* Alias in high bits, usage in low 2*/
};




/* To select whether we interpret the old pdir as 2 or 4K */
#define OLDPDIR_2K 1 

#ifdef OLDPDIR_2K 
/* 2K OLD PDIR */
#define OLDPDIR_NBPG 11
#define OPDE_VPAGE 21
#define OPDE_VOFF 11
#define OPDE_PPAGE 21
#define OPDE_PAD 4
/* Old pdir structure */
struct oldlle {
        unsigned int    lle_empty :1;       /* Hash chain empty */
        int               :6;
        int     lle_index :OPDE_PPAGE;       /* Index to the first pdir entry */
        int               :4;
};

#else

/* 4K OLD PDIR */
#define OLDPDIR_NBPG 12
#define OPDE_VPAGE 20
#define OPDE_VOFF 12
#define OPDE_PPAGE 20
#define OPDE_PAD 5
/* Old pdir structure */
struct oldlle {
        unsigned int    lle_empty :1;       /* Hash chain empty */
        int               :7;
        int     lle_index :OPDE_PPAGE;       /* Index to the first pdir entry */
        int               :4;
};
#endif

struct oldpde {
        struct oldlle      Pde_lle;            /* First word is a linked list */
#define old_pde_end Pde_lle.lle_empty
#define old_pde_next Pde_lle.lle_index
        unsigned int    pde_space :32;      /* Virtual space number */
 
        unsigned int    pde_page  :OPDE_VPAGE;      /* Virtual page number */
        unsigned int              :OPDE_VOFF;
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

#define oldpgtopde(pg)	(oldpdir + pg)
#define oldpdetopg(p)	(p - oldpdir)

struct oldpde *oldpdir;
struct oldhte *oldhtbl;
unsigned int npdir, nhtbl;

#else

struct pde *iopdir, *pdir;
struct hte *htbl;
unsigned int niopdir, npdir, nhtbl;

#endif

#define MP_HASH_TYPE 4

u_int get_phys_addr ();

#ifdef SPARSE_PDIR
#define newbtop(x) ((unsigned int)(x) >> newpageshift)
#define newptob(x) ((x) << newpageshift)
#endif SPARSE_PDIR

int newpageshift = 0;

static int pdir_kmem = 0;
static int pdir_core_fd = 0;
static nl_catd pdir_nls_fd = 0;

init_vm_structures (kernel, dev_kmem, core_fd, nls_fd)
	char *kernel;
	int dev_kmem;
	int core_fd;
	nl_catd nls_fd;
{
	nlist(kernel, pdir_nl);
	if (pdir_nl[0].n_type == 0)
		return(0);

	pdir_kmem = dev_kmem;
	pdir_core_fd = core_fd;
	pdir_nls_fd = nls_fd;

	if (!pdir_kmem)
		get_maps();
	/*Stuff*/
	return (1);
}

#ifdef	hp9000s800

get_maps()
{
	int tmp;
	int alloc_size;
	int page_offset;
	int pdir_size;

#ifdef	SPARSE_PDIR

	pdirhash_type = kl_sym_get(pdir_nl[N_PDIRHASH_TYPE].n_value);
	newpageshift = 11;
	if (pdirhash_type >= 3) {
		newpageshift = 12;
	}

	/* Existence check.  This variable did not used to exist.  If
	 * it doesn't in the current kernel, do things the old way.
	 * Otherwise, we use the base_pdir variable as the lowest possible
	 * point in the Pdir, and the max_pdir variable as the highest
	 * point.  Everything else is in between.  We read in the entire
	 * mass in one big chunk.  Soon, hopefully, this test can go
	 * away, and we can use this code exclusively.
	 */

if (pdir_nl [N_BASE_PDIR].n_value != 0) {
	vbase_pdir = (struct hpde *) kl_sym_get (pdir_nl [N_BASE_PDIR].n_value);
	vmax_pdir = (struct hpde *) kl_sym_get (pdir_nl [N_MAX_PDIR].n_value);
	pdir_size = (((int)vmax_pdir) - ((int)vbase_pdir));
	alloc_size = pdir_size + newptob(1);

	/* Allocate the space for the pdir.  We need for the base of
	 * the pdir structure to be aligned on the same boundary as in
	 * the target kernel.  This ensures that the hash table is so
	 * aligned, making sure that the bits that are used as virtual
	 * index bits match up.  This is really only needed for
	 * physical->virtual translations, but it doesn't hurt, and it
	 * allows for sharing of structures with code that allows
	 * physical->virtual lookups.
	 */
	base_pdir = (struct hpde *) malloc (alloc_size);
	if (base_pdir == NULL) {
		PERROR(pdir_nls_fd,NL_PDIR_SET,0, "Could not allocate sufficient memory for pdir.\n");
		exit (1);
	}
	page_offset = ((int)vbase_pdir) & (newptob(1)-1);
	base_pdir = (struct hpde *) (((int)base_pdir) + (newptob(1)-1));
	base_pdir = (struct hpde *)
		((((int)base_pdir) & (~(newptob(1)-1))) + page_offset);

        /* Exception to the atype = getphyaddr(vtype), only allowed in s800 */
        vpdir = (struct hpde *)kl_sym_get(pdir_nl[N_PDIR].n_value);
        npdir = kl_sym_get(pdir_nl[N_NPDIR].n_value);

        /* Exception to the atype = getphyaddr(vtype), only allowed in s800 */
        pdirhash_type = kl_sym_get(pdir_nl[N_PDIRHASH_TYPE].n_value);
        scaled_npdir = kl_sym_get(pdir_nl[N_SCALED_NPDIR].n_value);
        vhtbl = ahtbl = (struct hpde *)kl_sym_get(pdir_nl[N_HTBL].n_value);
        nhtbl = kl_sym_get(pdir_nl[N_NHTBL].n_value);

	htbl = (struct hpde *)((int)base_pdir + ((int)vhtbl - (int)vbase_pdir));
	pdir = (struct hpde *)((int)base_pdir + ((int)vpdir - (int)vbase_pdir));
	/* read in pdir */
	lseek(pdir_core_fd, (long)(vbase_pdir), 0);
	if (read(pdir_core_fd, (char *)base_pdir, ((int)vmax_pdir - (int)vbase_pdir))
			!=  ((int)vmax_pdir - (int)vbase_pdir)) {
		PERROR(pdir_nls_fd,NL_PDIR_SET,1, "Could not read page directory\n");
		exit(1);
	}
} else {
	/* Exception to the atype = getphyaddr(vtype), only allowed in s800 */
	vpdir = (struct hpde *)kl_sym_get(pdir_nl[N_PDIR].n_value);
	npdir = kl_sym_get(pdir_nl[N_NPDIR].n_value);

	/* Exception to the atype = getphyaddr(vtype), only allowed in s800 */
	pdirhash_type = kl_sym_get(pdir_nl[N_PDIRHASH_TYPE].n_value);
	scaled_npdir = kl_sym_get(pdir_nl[N_SCALED_NPDIR].n_value);
	vhtbl = ahtbl = (struct hpde *)kl_sym_get(pdir_nl[N_HTBL].n_value);
	nhtbl = kl_sym_get(pdir_nl[N_NHTBL].n_value);

	alloc_size = (nhtbl * sizeof(struct hpde)) +
		(scaled_npdir * sizeof(struct hpde)) +
		ptob (5);

	/* pdir is an offset into iopdir */
	htbl = (struct hpde *)malloc (alloc_size);

	tmp = (int)htbl;
	tmp = ((tmp + (newptob(1)-1)) & (~(newptob(1)-1))) +
		(((int)vhtbl) & (newptob(1)-1));
	htbl = (struct hpde *) tmp;

	/* Get pdir and hash table space and addesses */
	pdir = htbl + (vpdir - vhtbl);

	/* read in pdir */
	lseek(pdir_core_fd, (long)(vpdir), 0);
	if (read(pdir_core_fd, (char *)pdir, (scaled_npdir) * sizeof (struct hpde))
			!=  (scaled_npdir) * sizeof (struct hpde)) {
		PERROR(pdir_nls_fd,NL_PDIR_SET,2, "Could not read page directory\n");
		exit(1);
	}

	/* read in hash table */
	lseek(pdir_core_fd, (long)(vhtbl), 0);
	if (read(pdir_core_fd, (char *)htbl, nhtbl * sizeof (struct hpde))
			!=  nhtbl * sizeof (struct hpde)) {
		perror("get_maps 4");
		PERROR(pdir_nls_fd,NL_PDIR_SET,3, "Could not read hash table\n");
		exit(1);
	}
}

#else	/* !SPARSE_PDIR */
	vpdir = (struct pde *)kl_sym_get(pdir_nl[N_PDIR].n_value);
	npdir = kl_sym_get(pdir_nl[N_NPDIR].n_value);
	viopdir = aiopdir = (struct pde *)kl_sym_get(pdir_nl[N_IOPDIR].n_value);
	niopdir = kl_sym_get(pdir_nl[N_NIOPDIR].n_value);
	vhtbl = ahtbl = (struct hte *)kl_sym_get(pdir_nl[N_HTBL].n_value);
	nhtbl = kl_sym_get(pdir_nl[N_NHTBL].n_value);

	/* Get pdir and hash table space and addesses */
	iopdir = (struct pde *)malloc((niopdir +npdir) * sizeof (struct pde));
	if (iopdir == NULL) {
	    PERROR(pdir_nls_fd,NL_PDIR_SET,4,"Could not allocate memory for page table\n");
		exit(1);
	}

	/* pdir is an offset into iopdir */
	pdir = iopdir + niopdir;
	htbl = (struct hte *)malloc(nhtbl * sizeof (struct hte));
	if (htbl == NULL) {
		PERROR(pdir_nls_fd,NL_PDIR_SET,5, "Could not allocate memory for hash table\n");
		exit(1);
	}

	/* read in pdir */
	lseek(pdir_core_fd, (long)(aiopdir), 0);
	if (read(pdir_core_fd, (char *)iopdir, (niopdir+npdir) * sizeof (struct pde))
	  !=  (niopdir + npdir) * sizeof (struct pde)) {
		PERROR(pdir_nls_fd,NL_PDIR_SET,6, "Could not read page directory\n");
		exit(1);
	}

	/* read in hash table */
	lseek(pdir_core_fd, (long)(ahtbl), 0);
	if (read(pdir_core_fd, (char *)htbl, nhtbl * sizeof (struct hte))
	  !=  nhtbl * sizeof (struct hte)) {
		PERROR(pdir_nls_fd,NL_PDIR_SET,7, "Could not read hash table\n");
		exit(1);
	}
#endif  /* SPARSE_PDIR */
}

 
kl_sym_get(loc)
unsigned loc;
{
	int x;
	
	if (loc == 0) {
		PERROR(pdir_nls_fd,NL_PDIR_SET,8, "Bad location\n");
		exit(1);
	}

	lseek(pdir_core_fd, (long)(loc), 0);
	if (read(pdir_core_fd, (char *)&x, sizeof (int)) != sizeof (int)) {
		PERROR(pdir_nls_fd,NL_PDIR_SET,9, "Bad core read\n");
    		exit(1);
	}
	return (x);
}			/***** get *****/


#ifdef SPARSE_PDIR

/* Change for 4K page */
#undef PDE_SHADOW_BOFFSET
#define PDE_SHADOW_BOFFSET (newptob(1))

#define COMMON_BTOP(x) (newbtop(x) & ~0x1)		/* Find common page */
#define IS_ODD_OFFSET(x) ((u_int)(x) & PDE_SHADOW_BOFFSET)
#define IS_ODD_PAGE(x)   ((u_int)(x) & 0x1)	/* Is it an odd page */
#define IS_ODD_PDE(x)   ((u_int)(x) & 0x1)	/* Is it an odd pde */
#ifndef VPAGE_TO_PDE_VPAGE
#define VPAGE_TO_PDE_VPAGE(x) (((u_int)(x) >> 5) & 0x7FFF)
#endif


#undef sign21ext
#ifdef OLDPDIR_2K
#define sign21ext(num) \
	(((num) & 0x100000) ? ((num) | 0xffe00000) : (num))

/* Original old pdir 2K 5 instr hash */
#define pdirhash0(sid, sof)	\
	((sid << 5 ) ^ ((unsigned int) (sof) >> 11)  ^ \
	(sid << 13 ) ^ ((unsigned int) (sof) >> 19))
#else
#define sign21ext(num) \
	(((num) & 0x080000) ? ((num) | 0xfff00000) : (num))

/* Original old pdir 4K 5 instr hash */
#define pdirhash0(sid, sof)	\
	((sid << 5 ) ^ ((unsigned int) (sof) >> 12)  ^ \
	(sid << 13 ) ^ ((unsigned int) (sof) >> 19))

#endif /* if or if not OLDPDIR_2K */




#ifdef SPARSE_2K
/* New pdir 2K 3 instr XOR hash */
#define pdirhash1(sid, sof)	((sid << 5 ) ^ ((unsigned int) (sof) >> 12))

/* New pdir 2K 3 instr ADD hash */
#define pdirhash2(sid, sof)	((sid << 5 ) + ((unsigned int) (sof) >> 12))

/* New pdir 2K 5 instr hash */
#define pdirhash3(sid, sof)	\
	((sid << 5 ) ^ ((unsigned int) (sof) >> 12)  ^ \
	(sid << 13 ) ^ ((unsigned int) (sof) >> 19))
#else

/* New pdir 4K 3 instr XOR hash */
#define pdirhash1(sid, sof)	((sid << 5 ) ^ ((unsigned int) (sof) >> 13))

/* New pdir 4K 3 instr ADD hash */
#define pdirhash2(sid, sof)	((sid << 5 ) + ((unsigned int) (sof) >> 13))

/* New pdir 4K 5 instr hash */
#define pdirhash3(sid, sof)	\
	((sid << 5 ) ^ ((unsigned int) (sof) >> 13)  ^ \
	(sid << 13 ) ^ ((unsigned int) (sof) >> 19))

/* New pdir 4K 3 instr XOR hash, PCX-T style */
#define pdirhash4(sid, sof)	((sid << 4 ) ^ ((unsigned int) (sof) >> 13))

#endif  /* if or if not SPARSE_2K */


pdirhashproc(sid,off)
unsigned sid, off;
{
	switch (pdirhash_type){
	case 0:
		return(pdirhash0(sid,off));
	case 1:
		return(pdirhash1(sid,off));
	case 2:
		return(pdirhash2(sid,off));
	case 3:
		return(pdirhash3(sid,off));
	case 4:
		return(pdirhash4(sid,off));

	default: printf("Bad pdirhash_type, default  to 0\n");
		return(pdirhash0(sid,off));

	}
}
#endif  /* if SPARSE_PDIR */



/* Translate the virtual address to a real address, looking for both
 * valid and prevalid addresses.  Although the contents of this 
 * routine were taken from adb, the printf's (and everything else related
 * to verbose adb use) have been removed to protect the innocent.
 */

#ifdef SPARSE_PDIR

int equiv_mode = 0;

u_int
get_phys_addr(space, offset)
	u_int space;
	u_int offset;
#undef pde_next
{

	struct newhpde *newpde;
	struct mp_newhpde *mp_newpde;
	struct oldpde *pde;
	u_int	page;
	u_int raddr;
	u_int odd;
	u_int vpage;

	if (pdir_kmem) {
		return (offset);
	}

	odd = IS_ODD_OFFSET(offset);
	vpage = VPAGE_TO_PDE_VPAGE(newbtop(offset));

	if (equiv_mode) {
		return (offset);
	}


	 /* Basically this is ripped out of vm_machdep.c. It of course
	  * cannot use the lpa instruction, so that piece has been 
	  * removed. It also compensates for htbl, and pdir being
	  * local structures, but the internal pointers are still
	  * relative to the core files pdir and hash table.
	  *
	  * REQUIRES:
	  *          pdir to be a pointer to a local copy of the pdir.
	  *          htbl to be a pointer to a local copy of the hash table.
	  */


	if ((pdirhash_type != -1) && (pdirhash_type != MP_HASH_TYPE)) {
		
		/* New SPARSE pdir */
		page = COMMON_BTOP(offset);

		newpde = (struct newhpde *) &htbl[ pdirhashproc(space, (u_int) offset) & (nhtbl - 1) ];
	
		/* Run down the chain looking for the space, offset pair. */
		for (;newpde;) {
			if (((newpde->pde_space) == space)
				&& (newpde->pde_page == page)){
				break;
			}
			newpde = newpde->pde_next;
			
			/* See if end of chain */
			if (newpde == 0)
				break;

			/* 
 			 * Convert to internal address.
			 */
			if (base_pdir == NULL) {
				newpde = htbl + (newpde - vhtbl);
			} else {
				newpde = base_pdir + (newpde - vbase_pdir);
			}
		}

		/* did we find it or its buddy ? */
		if (newpde == 0)
			return(-1);

		/* Check correct half */
		if IS_ODD_OFFSET(offset){

			if (newpde->pde_phys2 == 0)
				return(-1);
			raddr = newptob(newpde->pde_phys2);
		} else {

			if (newpde->pde_phys == 0)
				return(-1);
			raddr = newptob(newpde->pde_phys);
		}
		raddr =  raddr +
				( (u_int) offset & ( (1 << newpageshift) -1));
	
		return (raddr);

	} else if (pdirhash_type == MP_HASH_TYPE) {
		
		/* New SPARSE pdir */
		page = COMMON_BTOP(offset);

		mp_newpde = (struct mp_newhpde *) &htbl[ pdirhashproc(space, (u_int) offset) & (nhtbl - 1) ];
	
		/* Run down the chain looking for the space, offset pair. */
		for (;mp_newpde;) {

			if (odd) {
				if (((mp_newpde->pde_space_o) == space)
					&& (mp_newpde->pde_page_o == vpage)){
					break;
				}
			} else {
				if (((mp_newpde->pde_space_e) == space)
					&& (mp_newpde->pde_page_e == vpage)){
					break;
				}
			}
			mp_newpde = mp_newpde->pde_next;
			
			/* See if end of chain */
			if (mp_newpde == 0)
				break;

			/* 
 			 * Convert to internal address.
			 */
			if (base_pdir == NULL) {
				mp_newpde = htbl + (mp_newpde - vhtbl);
			} else {
				mp_newpde = base_pdir + (mp_newpde - vbase_pdir);
			}
		}

		/* did we find it or its buddy ? */
		if (mp_newpde == 0)
			return(-1);

		/* Check correct half */
		if IS_ODD_OFFSET(offset){

			if (mp_newpde->pde_phys_o == 0)
				return(-1);
			raddr = newptob(mp_newpde->pde_phys_o);
		} else {

			if (mp_newpde->pde_phys_e == 0)
				return(-1);
			raddr = newptob(mp_newpde->pde_phys_e);
		}
		raddr =  raddr +
				( (u_int) offset & ( (1 << newpageshift) -1));
	
		return (raddr);

	} else {

		/* OLD PDIR */
		page = newbtop(offset);
		pde = (struct oldpde *) &oldhtbl[
			pdirhash0(space, (u_int) newptob(page)) & (nhtbl - 1)
			];
		
		for (;;) {
			if (pde->old_pde_next == 0) {
				return((u_int) -1);
			}
			pde = &oldpdir[sign21ext(pde->old_pde_next)];
			if (pde->pde_space == space
				&& pde->pde_page == page)
				break;
		}
	
		raddr = newptob(oldpdetopg(pde));
		raddr = raddr +
				( (u_int) offset & ( (1 << newpageshift) -1));
	
		return (raddr);

	}
}

#else  /* if not SPARSE_PDIR */

off_t
get_phys_addr(space, offset)
	off_t space;
	off_t offset;

{
	struct pde *pde;
	u_int	page, pgoffset;
	off_t raddr;


	 /* Basically this is ripped out of vm_machdep.c. It of course
	  * cannot use the lpa instruction, so that piece has been 
	  * removed. It also compensates for htbl, and pdir being
	  * local structures, but the internal pointers are still
	  * relative to the core files pdir and hash table.
	  *
	  * REQUIRES:
	  *          pdir to be a pointer to a local copy of the pdir.
	  *          htbl to be a pointer to a local copy of the hash table.
	  */

	page = btop(offset);
	pde = (struct pde *) &htbl[
		pdirhash(space, (u_int) ptob(page)) & (nhtbl - 1)
		];
	
	for (;;) {
		if (pde->pde_next == 0) {
			return((off_t) -1);
		}
		pde = &pdir[sign21ext(pde->pde_next)];
		if (pde->pde_space == space
			&& pde->pde_page == page)
			break;
	}

	raddr = (off_t) ptob(pdetopg(pde));
	raddr = (off_t) (u_int) raddr +
			( (u_int) offset & ( (1 << PGSHIFT) -1));

	return (raddr);
}
#endif  /* if or if not SPARSE_PDIR */


#endif	/* hp9000s800 */


