/*
 * @(#)hdl_vas.h: $Revision: 1.4.84.3 $ $Date: 93/09/17 21:05:33 $
 * $Locker:  $
 */
#ifndef MACHINE_HDL_VAS_INCLUDED
#define MACHINE_HDL_VAS_INCLUDED

/*
 * HDL fields for the vas data structure
 */

#ifdef _KERNEL_BUILD
#include "../machine/param.h"
#include "../machine/pte.h"
#include "../h/vas.h"
#else /* ! _KERNEL_BUILD */
#include <machine/param.h>
#include <machine/pte.h>
#include <sys/vas.h>
#endif /* _KERNEL_BUILD */

/* To keep track of valid segment table entries. */
struct vste {
	unsigned int vste_offset;
	struct vste *vste_next;
	struct vste *vste_prev;
};

/* Hardware-dependent fields of VAS */
struct hdlvas {
	/* This field is a pointer to the mmu hardware segment table.
	 * Segment tables are allocated out of the tt (transparent
	 * translation) window and are always mapped logical = physical.
	 * Thus this value is the physical address used to load the mmu's
	 * root pointer register and it is the kernel logical address
	 * used by the HDL layer to manipulate the table.
	 */
	struct ste *va_seg;

	/*
	 * This is the head of a linked list of valid segment table entries.
	 * We keep this list because the segment table is always very sparse.
	 * Typically a process will have 3 valid segment table entries.  By 
	 * maintaining this table we can duplicate (vm_duptab for fork) or 
	 * destroy (vm_dealloc_tables for exit and exec) without having to 
	 * walk the entire segment table.
	 */
	struct vste *va_vsegs;

	/*
	 * This field is a hint as to where to attach new pregions.
	 * New pregions are attached just after this one.
	 */
	preg_t *va_attach_hint;
};

#define INIT_ATTCHPT	0x80000000

extern int hil_pbase;
extern int etext, end;
extern unsigned int kgenmap_ptes;
extern unsigned int tt_region_addr;
extern caddr_t highest_kvaddr;
extern struct map *sysmap;
extern struct map *validsegmap;
extern struct map *blocktablemap;
extern struct map *pagetablemap;
extern unsigned int physpfdat;

extern unsigned int ttw_alloc();
extern unsigned int alloc_page();
extern int ttw_free();
extern int free_page();

#define INIT_ATL 20
#define ZERO_MEM	1
#define DONT_ZERO_MEM	0

#define alloc_segtab() (three_level_tables ? 				\
                      ttw_alloc(blocktablemap, NBBT, 1) : alloc_page(1))

#define free_segtab(table) (three_level_tables ? 			\
                      ttw_free(blocktablemap, table, NBBT) : free_page(table))

#endif /* not MACHINE_HDL_VAS_INCLUDED */
