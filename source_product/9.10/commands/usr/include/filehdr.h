/* @(#) $Revision: 66.2 $ */
/*
 *  HP 9000 Series 800
 *  Object File Format Definitions
 *
 *  Copyright Hewlett-Packard Co. 1985
 */
#ifndef _FILEHDR_INCLUDED /* allow multiple inclusions */
#define _FILEHDR_INCLUDED

#ifdef __hp9000s800
#include <sys/magic.h>
#include "aouttypes.h"

/* Magic numbers (not in magic.h) */

#define EXECLIBMAGIC    0x0104		/* executable library */

#define VERSION_ID      85082112
#define NEW_VERSION_ID  87102412	/* relocatable with new fixups */

struct header {
        short int system_id;                 /* magic number - system        */
        short int a_magic;                   /* magic number - file type     */
        unsigned int version_id;             /* version id; format= YYMMDDHH */
        struct sys_clock file_time;          /* system clock- zero if unused */
        unsigned int entry_space;            /* index of space containing 
                                                entry point                  */
        unsigned int entry_subspace;         /* index of subspace for 
                                                entry point                  */
        unsigned int entry_offset;           /* offset of entry point        */
        unsigned int aux_header_location;    /* auxiliary header location    */
        unsigned int aux_header_size;        /* auxiliary header size        */
        unsigned int som_length;             /* length in bytes of entire som*/
        unsigned int presumed_dp;            /* DP value assumed during 
						compilation 		     */
        unsigned int space_location;         /* location in file of space 
                                                dictionary                   */
        unsigned int space_total;            /* number of space entries      */
        unsigned int subspace_location;      /* location of subspace entries */
        unsigned int subspace_total;         /* number of subspace entries   */
        unsigned int loader_fixup_location;  /* space reference array        */
        unsigned int loader_fixup_total;     /* total number of space 
                                                reference records            */
        unsigned int space_strings_location; /* file location of string area
                                                for space and subspace names */
        unsigned int space_strings_size;     /* size of string area for space 
                                                 and subspace names          */
        unsigned int init_array_location;    /* location in file of 
                                                initialization pointers      */
        unsigned int init_array_total;       /* number of init. pointers     */
        unsigned int compiler_location;      /* location in file of module 
                                                dictionary                   */
        unsigned int compiler_total;         /* number of modules            */
        unsigned int symbol_location;        /* location in file of symbol 
                                                dictionary                   */
        unsigned int symbol_total;           /* number of symbol records     */
        unsigned int fixup_request_location; /* location in file of fix_up 
                                                requests                     */
        unsigned int fixup_request_total;    /* number of fixup requests     */
        unsigned int symbol_strings_location;/* file location of string area 
                                                for module and symbol names  */
        unsigned int symbol_strings_size;    /* size of string area for 
                                                module and symbol names      */
        unsigned int unloadable_sp_location; /* byte offset of first byte of 
                                                data for unloadable spaces   */
        unsigned int unloadable_sp_size;     /* byte length of data for 
                                                unloadable spaces            */
        unsigned int checksum;
};

#define FILHDR	struct header
#define FILHSZ	sizeof(FILHDR)
#endif /* __hp9000s800 */
#endif /* _FILEHDR_INCLUDED */
