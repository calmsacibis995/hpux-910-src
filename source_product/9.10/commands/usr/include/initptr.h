/* @(#) $Revision: 64.2 $ */
/*
 *  HP 9000 Series 800
 *  Object File Format Definitions
 *
 *  Copyright Hewlett-Packard Co. 1985
 */
#ifndef _INITPTR_INCLUDED /* allow multiple inclusions */
#define _INITPTR_INCLUDED

#ifdef __hp9000s800
struct init_pointer_record {
        unsigned int space_index;               /* index of space entry       */
        unsigned int access_control_bits: 7;    /* access field for PDIR      */
        unsigned int has_data	 	: 1;    /* file pages exist for this 
                                                   area of memory             */
        unsigned int memory_resident	: 1;
        unsigned int initially_frozen	: 1;     /* must be locked into memory 
                                                    when OS is booted         */
	unsigned int new_locality       : 1;    /* this init ptr begins a new
						   locality                   */
        unsigned int reserved: 21;
        unsigned int file_loc_init_value;       /* starting location in file 
                                                  (page aligned)              */
        unsigned int initialization_length;
        unsigned int space_offset;              /* starting offset in space 
                                                   (page aligned)             */
};

#define INITPTR struct init_pointer_record
#define INITPTRSZ sizeof(INITPTR)

#endif /* __hp9000s800 */
#endif /* _INITPTR_INCLUDED */
