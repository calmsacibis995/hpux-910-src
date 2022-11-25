/*
 * @(#)hdl_preg.h: $Revision: 1.5.84.4 $ $Date: 94/05/05 15:29:47 $
 * $Locker:  $
 */

#ifndef _MACHINE_HDL_PREG_INCLUDED
#define _MACHINE_HDL_PREG_INCLUDED

/*
 * Data structures to store page modes for mprotect()
 */
struct hdl_protrange {
    u_long mode:8;	/* protection mode (MPROT_*) */
    u_long idx:24; 	/* starting index of range */
};

struct hdl_subpregion {
    int nused;			    /* used elements in range[] */
    int nelements;		    /* actual size of range[] */
    int hint;			    /* hint index for next access */
    struct hdl_protrange range[5];  /* ranges (can be bigger) */
};

typedef struct hdl_subpregion hdl_subpreg_t;

/*
 * HDL fields for the pregion data structure
 */
struct hdlpregion {
	ushort p_hdlflags;	/* General flags for pregion */
	ushort p_ntran;		/* # translations under this preg */
	unsigned int
		p_physpfn;	/* For PT_IO, the base phsyical addr for */
				/* the I/O mapping */
	struct pregion_atl 
		*p_atl;	/* atls for this pregion */
	hdl_subpreg_t *p_spreg;	/* page modes for mprotect() */
};

/* Flags for p_hdlflags */
#define PHDL_RO 	0x0001	/* Read-only flag */
#define PHDL_ATTACH 	0x0002	/* Pregion has been hdl_attach()'ed */
#define PHDL_PROCATTACH	0x0004  /* Pregion has been hdl_procattached */
#define PHDL_CACHE_WT	0x0008  /* Pregion has cache in write through mode */
#define PHDL_CACHE_CB	0x0010  /* Pregion has cache in copyback mode */
#define PHDL_CACHE_CI	0x0020  /* Pregion is cache inhibited */
#define PHDL_CACHE_NS	0x0040  /* Pregion is cache inhibited non-serialized */
#define PHDL_IPTES	0x0080  /* Pregion is using indirect ptes */
#define PHDL_DPTES	0x0100  /* Pregion is using direct ptes */
#ifdef PFDAT32
#define PHDL_ATLS	0x0200  /* Pregion is attach lists */
#endif	

#define PHDL_CACHE_MASK 0x0078  /* mask for cache bits in p_hdlflags field */

#ifdef _KERNEL
/*
 * Macro to determine if a pregion has been mprotect()ed.
 */
#define IS_MPROTECTED(prp)	((prp)->p_hdl.p_spreg != (hdl_subpreg_t *)0)
#endif

#endif /* _MACHINE_HDL_PREG_INCLUDED */
