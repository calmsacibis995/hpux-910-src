/* @(#) $Revision: 64.2 $ */
/*
 *  HP 9000 Series 800
 *  Object File Format Definitions
 *
 *  Copyright Hewlett-Packard Co. 1985
 */
#ifndef _SYMS_INCLUDED /* allows multiple inclusions */
#define _SYMS_INCLUDED

#ifdef __hp9000s800
#include "aouttypes.h"

struct symbol_dictionary_record {
        unsigned int  hidden           : 1;
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
};

#define n_nptr		name.n_name
#define n_offset	name.n_strx
#define q_nptr		qualifier_name.n_name
#define q_offset	qualifier_name.n_strx

/* symbol types */

#define    ST_NULL         0
#define    ST_ABSOLUTE     1
#define    ST_DATA         2
#define    ST_CODE         3
#define    ST_PRI_PROG     4
#define    ST_SEC_PROG     5
#define    ST_ENTRY        6
#define    ST_STORAGE      7
#define    ST_STUB         8
#define    ST_MODULE       9
#define    ST_SYM_EXT     10
#define    ST_ARG_EXT     11
#define    ST_MILLICODE   12
#define    ST_PLABEL      13
#define	   ST_OCT_DIS	  14
#define	   ST_MILLI_EXT   15

/* symbol scopes */

#define    SS_UNSAT        0
#define    SS_EXTERNAL     1
#define    SS_LOCAL        2
#define    SS_GLOBAL       2
#define    SS_UNIVERSAL    3

/* symbol extension records (for type checking) */

union arg_descriptor {  
	struct {
		unsigned int reserved  :3;
		unsigned int packing   :1;
		unsigned int alignment :4;
		unsigned int mode      :4;
		unsigned int structure :4;
		unsigned int hash      :1;
		int arg_type           :15;
	} arg_desc;
	unsigned int 	word;
};

struct symbol_extension_record {
        unsigned int          type         :8; /* always type SYM_EXT (12) for 
                      			          this record                 */
        unsigned int          max_num_args :8;
        unsigned int          min_num_args :8;
        unsigned int          num_args     :8;
        union arg_descriptor  symbol_desc;
        union arg_descriptor  argument_desc[3];
};

struct argument_desc_array {
        unsigned int            type    : 8; /*  always type ARG_EXT (13) for 
                       				 this record          */
        unsigned int            reserved: 24; 
        union arg_descriptor    argument_desc[4];
};

#define	SYMENT	struct symbol_dictionary_record
#define	SYMESZ	sizeof(SYMENT)

#define	AUXENT	struct symbol_extension_record
#define	AUXESZ	sizeof(AUXENT)

/*	Defines for "special" symbols   */

#define _ETEXT	"etext"
#define	_EDATA	"edata"
#define	_END	"end"

#define _START	"_start"

#endif /* __hp9000s800 */
#endif /* _SYMS_INCLUDED */
