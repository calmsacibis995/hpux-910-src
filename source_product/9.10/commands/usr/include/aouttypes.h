/* @(#) $Revision: 64.2 $ */
/*
 *  HP 9000 Series 800 Linker
 *  Object File Format Definitions
 *
 *  Copyright Hewlett-Packard Co. 1985
 */

#ifndef _AOUTTYPES_INCLUDED /* allow multiple inclusions */
#define _AOUTTYPES_INCLUDED

#ifdef __hp9000s800
struct sys_clock { 
	unsigned int secs; 
	unsigned int nanosecs; 
};

union name_pt {
	char *n_name;
	unsigned int n_strx;
};

#define NAME_PT name.n_name
#define STR_INDEX name.n_strx

#endif /* __hp9000s800 */
#endif /* _AOUTTYPES_INCLUDED */
