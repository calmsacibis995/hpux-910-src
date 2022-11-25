/* @(#) $Revision: 64.2 $ */
/*
 *  HP 9000 Series 800
 *  Object File Format Definitions
 *
 *  Copyright Hewlett-Packard Co. 1985
 */
#ifndef _SCNHDR_INCLUDED /* allow multiple inclusions */
#define _SCNHDR_INCLUDED

#ifdef __hp9000s800
#include "aouttypes.h"

struct subspace_dictionary_record {
        int             space_index;
        unsigned int    access_control_bits: 7; /* access for PDIR entries    */
        unsigned int    memory_resident    : 1; /* lock in memory during 
                                                   execution                  */
        unsigned int    dup_common         : 1; /* data name clashes allowed  */
        unsigned int    is_common          : 1; /* subspace is a common block */
        unsigned int    is_loadable        : 1;
        unsigned int    quadrant	   : 2; /* quadrant request           */
        unsigned int    initially_frozen   : 1; /* must be locked into memory 
                                                   when OS is booted          */
        unsigned int    is_first   	   : 1; /* must be first subspace     */
        unsigned int    code_only   	   : 1; /* must contain only code     */
        unsigned int    sort_key           : 8;	/* subspace sort key	      */
	unsigned int	replicate_init	   : 1;	/* init values replicated to
						   fill subspace_length       */
	unsigned int	continuation	   : 1;	/* subspace is a continuation */
        unsigned int    reserved           : 6;
        int             file_loc_init_value;    /* file location or 
                                                   initialization value       */
        unsigned int    initialization_length;
        unsigned int    subspace_start;         /* starting offset            */
        unsigned int    subspace_length;        /* number of bytes defined 
                                                   by this subspace           */
        unsigned int    reserved2	   :16;   
        unsigned int    alignment	   :16; /* alignment required for the
                                                   subspace (largest alignment 
                                                   requested for any item in 
                                                   the subspace)              */
        union name_pt   name;                   /* index of subspace name     */
        int             fixup_request_index;    /* index into fixup array     */
        unsigned int    fixup_request_quantity; /* number of fixup requests   */
};

#define	SCNHDR	struct subspace_dictionary_record
#define	SCNHSZ	sizeof(SCNHDR)

/*
 * Define constants for names of "special" subspaces
 */

#define _TEXT "$CODE$"
#define _DATA "$DATA$"
#define _BSS  "$BSS$"

#endif /* __hp9000s800 */
#endif /* _SCNHDR_INCLUDED */
