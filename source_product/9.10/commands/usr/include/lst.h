/* @(#) $Revision: 64.3 $ */
/*
 *  HP 9000 Series 800
 *  Object File Format Definitions
 *
 *  Copyright Hewlett-Packard Co. 1985
 */
#ifndef _LST_INCLUDED /* allow multiple inclusions */
#define _LST_INCLUDED

#ifdef __hp9000s800
#include "aouttypes.h"

#define DIRNAME "/               "
#define LIBMAGIC	0x0619		/* Library Symbol Table magic */

/*
 * Library Symbol Table structure
 */

struct lst_header {
	short int	system_id;
	short int	a_magic;
	unsigned int	version_id;
	struct sys_clock file_time;
	unsigned int	hash_loc;
	unsigned int	hash_size;
	unsigned int	module_count;
	unsigned int	module_limit;
	unsigned int	dir_loc;
	unsigned int	export_loc;
	unsigned int	export_count;
	unsigned int	import_loc;
	unsigned int	aux_loc;
	unsigned int	aux_size;
	unsigned int	string_loc;
	unsigned int	string_size;
	unsigned int	free_list;
	unsigned int	file_end;
	unsigned int	checksum;
	};

struct som_entry {
	unsigned int	location;
	unsigned int	length;
	};

struct lst_symbol_record {
        unsigned int  hidden	       : 1;
        unsigned int  secondary_def    : 1;
        unsigned int  symbol_type      : 6;
        unsigned int  symbol_scope     : 4;
        unsigned int  check_level      : 3;
        unsigned int  must_qualify     : 1;
        unsigned int  initially_frozen : 1;
        unsigned int  memory_resident  : 1;
        unsigned int  is_common        : 1;
        unsigned int  dup_common       : 1;
        unsigned int  xleast           : 2;
        unsigned int  arg_reloc        :10;
        union name_pt name;
        union name_pt qualifier_name; 
        unsigned int  symbol_info;
        unsigned int  symbol_value;
	unsigned int  symbol_descriptor;
	unsigned int  reserved         : 8;
	unsigned int  max_num_args     : 8;
	unsigned int  min_num_args     : 8;
	unsigned int  num_args         : 8;
	unsigned int  som_index;
	unsigned int  symbol_key;
	unsigned int  next_entry;
	};

#define LSTSYMESZ	sizeof(struct lst_symbol_record)
#define SLSTHDR         sizeof(struct lst_header)

#endif /* __hp9000s800 */
#endif /* _LST_INCLUDED */
