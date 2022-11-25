/* @(#) $Revision: 64.2 $ */
/*
 *  HP 9000 Series 800
 *  Object File Format Definitions
 *
 *  Copyright Hewlett-Packard Co. 1985
 */
#ifndef _COMPUNIT_INCLUDED /* allow multiple inclusions */
#define _COMPUNIT_INCLUDED

#ifdef __hp9000s800
#include "aouttypes.h"

struct compilation_unit {
        union name_pt    name;
        union name_pt    language_name;
        union name_pt    product_id;
        union name_pt    version_id;
        unsigned int	 reserved   : 31;
        unsigned int	 chunk_flag :  1;
        struct sys_clock compile_time;
        struct sys_clock source_time;
};

#define COMPUNIT struct compilation_unit
#define COMPUNITSZ sizeof(COMPUNIT)

#endif /* __hp9000s800 */
#endif /* _COMPUNIT_INCLUDED */
