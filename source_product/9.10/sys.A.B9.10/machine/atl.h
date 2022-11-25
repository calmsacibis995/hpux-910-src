/*
 * @(#)atl.h: $Revision: 1.5.84.3 $ $Date: 93/09/17 21:02:18 $
 * $Locker:  $
 */

/*
 * atl.h--data structure for maintaining a list of attachers to
 * a pfdat entry.
 */
#ifndef _ATL_INCLUDED
#define _ATL_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/pregion.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/pregion.h>
#endif /* _KERNEL_BUILD */


#define ATL_PFD_CHUNK_SIZE 2
#define ATL_PRP_EXTRA 4


struct pfd_atl_entry {
	struct pte *pte;	/* The pte which maps this pfdat entry */	
	struct pregion *prp;
	ushort index;
};

struct pfd_atl {
	ushort max_index;
	ushort cur_index;
	struct pfd_atl_entry *atl_data;
};

struct pregion_atl_entry {
	ushort pfd_index;
	ushort entry_number;
};

struct pregion_atl {
	ushort count;
	ushort lowest;
	struct pregion_atl_entry *atl_data;
};

extern int atl_prot();
extern int atl_promote_write();
extern int do_addatlsc();

#endif _ATL_INCLUDED
