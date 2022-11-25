/* @(#) $Revision: 70.1 $ */
/*
 *  HP 9000 Series 800
 *  Object File Format Definitions
 *
 *  Copyright Hewlett-Packard Co. 1985
 */
#ifndef _AOUTHDR_INCLUDED /* allow multiple inclusions */
#define _AOUTHDR_INCLUDED

#ifdef __hp9000s800

#include "aouttypes.h"

struct aux_id { 
	unsigned int mandatory : 1;
	unsigned int copy : 1;
	unsigned int append : 1;
	unsigned int ignore : 1;
	unsigned int reserved : 12;
	unsigned int type : 16;
	unsigned int length;
};

/* Auxiliary Header types */

#define LINKER_AUX_ID		1
#define MPE_AUX_ID		2
#define DEBUGGER_AUX_ID		3
#define HPUX_AUX_ID		4
#define IPL_AUX_ID		5
#define VERSION_AUX_ID		6
#define MPE_PROG_AUX_ID		7
#define MPE_SOM_AUX_ID		8
#define COPYRIGHT_AUX_ID	9
#define SHLIB_VERSION_AUX_ID	10
#define PRODUCT_SPECIFIC_AUX_ID 11

struct debugger_footprint {
	struct aux_id	 header_id;
	char		 debugger_product_id[12];
        char             debugger_version_id[8];
        struct sys_clock debug_time;
};

struct linker_footprint {
        struct aux_id    header_id;
        char             product_id[12];
        char             version_id[12];
        struct sys_clock htime;
};

struct cap_list {
        unsigned int    reserved1	: 23;
        unsigned int    batch_acc 	:  1;	/* BA */
        unsigned int    inter_acc 	:  1;	/* IA */
        unsigned int    priv_mode 	:  1;	/* PM */
	unsigned int	reserved2	:  2;
        unsigned int    multiple_rins 	:  1;	/* MR */
	unsigned int	reserved3	:  1;
        unsigned int    extra_data_seg 	:  1;	/* DS */
        unsigned int    process_hand 	:  1;	/* PH */
};

struct mpe_aux_header { 
        struct aux_id   header_id;
        unsigned int    internal_flag :1;
        unsigned int    reserved      :31;
        unsigned int    num_xrts;
        unsigned int    entry_name;
        unsigned int    unsat_name;
        int             search_list;
        struct cap_list capabilities;
        unsigned int    max_stacksize;
        unsigned int    max_heap_size;
	unsigned int	unwind_start;
	unsigned int	unwind_end;
	unsigned int	recover_start;
	unsigned int	recover_end;
};       

struct mpe_prog_aux_hdr {
	struct aux_id	header_id;
	unsigned int	entry_name;
	unsigned int	unsat_name;
	int		search_list;
	struct cap_list	capabilities;
	unsigned int	max_stacksize;
	unsigned int	max_heap_size;
	unsigned int	reserved:15;
	unsigned int	posix:1;
	unsigned int	max_priority:8;
	unsigned int	priority:8;
};

struct mpe_som_aux_hdr {
	struct aux_id	header_id;
	unsigned int	reserved	:28;
	unsigned int    thread_private  :1;
	unsigned int	dumpworthy     	:1;
	unsigned int	hpe_som      	:1;
	unsigned int	system_som   	:1;
	unsigned int	num_xrts;
	unsigned int	unwind_start;
	unsigned int	unwind_end;
	unsigned int	recover_start;
	unsigned int	recover_end;
};

struct som_exec_auxhdr {
	struct aux_id som_auxhdr;	/* som auxiliary header header  */
	long exec_tsize;		/* text size in bytes           */
	long exec_tmem;			/* offset of text in memory     */
	long exec_tfile;		/* location of text in file     */
	long exec_dsize;		/* initialized data             */
	long exec_dmem;			/* offset of data in memory     */
	long exec_dfile;		/* location of data in file     */
	long exec_bsize;		/* uninitialized data (bss)     */
	long exec_entry;		/* offset of entrypoint         */
	long exec_flags;		/* loader flags                 */
	long exec_bfill;		/* bss initialization value     */
};

/*
 * The low-order bit of the field exec_flags specifies whether nil
 * pointers should be treated as an error.  (The -z linker flag sets
 * this bit).  If on, dereferencing a nil pointer will cause the
 * program to abort.  If off, dereferencing a nil pointer will return
 * zero.
 */

typedef struct som_exec_auxhdr AOUTHDR;

struct ipl_aux_hdr {
        struct aux_id   header_id;
	unsigned int	file_length;
	unsigned int    address_dest;
	unsigned int    entry_offset;
	unsigned int	bss_size;
	unsigned int    checksum;
};

struct user_string_aux_hdr {
        struct aux_id   header_id;
	unsigned int    string_length;
	char		user_string[1];
};

struct copyright_aux_hdr {
        struct aux_id   header_id;
	unsigned int    string_length;
	char		copyright[1];
};

struct shlib_version_aux_hdr {
	struct aux_id   header_id;
	short   version;
};

struct product_specific_aux_hdr { 
	struct aux_id   header_id;
	unsigned int key[1];
};

#endif /* __hp9000s800 */
#endif /* _AOUTHDR_INCLUDED */
