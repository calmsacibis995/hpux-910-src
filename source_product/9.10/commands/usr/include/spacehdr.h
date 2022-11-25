/* @(#) $Revision: 70.1 $ */
/*
 *  HP 9000 Series 800
 *  Object File Format Definitions
 *
 *  Copyright Hewlett-Packard Co. 1985
 */
#ifndef _SPACEHDR_INCLUDED /* allow multiple inclusions */
#define _SPACEHDR_INCLUDED

#ifdef __hp9000s800
#include "aouttypes.h"

struct space_dictionary_record {
        union name_pt name;                 /* index to subspace name         */
        unsigned int  is_loadable: 1;       /* space is loadable              */
        unsigned int  is_defined : 1;       /* space is defined within file   */
        unsigned int  is_private : 1;       /* space is not sharable 	      */
        unsigned int  has_intermediate_code: 1;/* space has intermediate code */
        unsigned int  reserved   : 12;      /* reserved for future expansion  */
	unsigned int  sort_key   : 8;       /* sort key for space             */
        unsigned int  reserved2  : 8;       /* reserved for future expansion  */
        int           space_number;         /* space index                    */
        int           subspace_index;       /* index into subspace dictionary */
        unsigned int  subspace_quantity;    /* number of subspaces in space   */
        int           loader_fix_index;     /* index into loader fixup array  */
        unsigned int  loader_fix_quantity;  /* number of loader fixups in 
					       this space                     */
        int           init_pointer_index;   /* index into data(initialization)
                                               pointer array                  */
        unsigned int  init_pointer_quantity;/* number of data (init) pointers */
};

#define SPAHDR	struct space_dictionary_record
#define SPAHSZ	sizeof(SPAHDR)

#endif /* __hp9000s800 */
#endif /* _SPACEHDR_INCLUDED */
