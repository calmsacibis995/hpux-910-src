/* @(#) $Revision: 70.7 $ */

#ifndef		_SHL_INCLUDED	/* allow multiple inclusion */
#define		_SHL_INCLUDED

#ifdef		__hp9000s300

/* structure definitions for shared library support */

/* tables placed at start of text segment */

/* DL_HEADER extension header */
struct _dl_header
{
	long zero;		/* zero for null pointer dereference */
	long dynamic;		/* offset from file text of DYNAMIC */
	long shlt;		/* offset of shared library table */
	long import;		/* offset of import table */
	long export;		/* offset of export table */
	long shl_export;	/* offset of more fields for .sl's */
	long hash;		/* offset of hash table for export */
	long string;		/* offset of string table */
	long module;		/* offset of module imports */
	long dmodule;		/* offset of dmodule table */
	long dreloc;		/* offset of dynamic relocation records */
	long spare1[2];
};

/* shared library table */
struct shl_entry
{
	long name;		/* offset within string table */
	unsigned char load;	/* specified via "-l" or pathname */
	unsigned char bind;	/* immediate or deferred */
	short highwater;	/* high water mark */
	long spare1;
};

/* load flags */
#define	LOAD_LIB	0x01	/* "-l" option */
#define	LOAD_PATH	0x02	/* full pathname */
#define	LOAD_IMPLICIT	0x03	/* installed library (NOT IMPLEMENTED) */

/* bind flags */
#define	BIND_IMMEDIATE	0x01
#define	BIND_DEFERRED	0x02
#define	BIND_NOSTART	0x04
#define	BIND_NONFATAL	0x08
#define	BIND_VERBOSE	0x10
#define	BIND_FIRST	0x20
#define	DYNAMIC_PATH	0x40

/* import table */
struct import_entry
{
	long name;		/* offset within string table */
	long shl;		/* index into shared library table */
	short type;		/* data or procedure */
	short spare1;
};

/* export table */
struct export_entry
{
	long name;		/* offset within string table */
	long value;		/* offset from file text */
	short type;		/* data or procedure */
	short highwater;	/* module version */
	long size;		/* for data and comm storage request */
	long next_export;	/* index of next entry in hash/version chain */
	long spare1;
};

/* import/export types */
#define	TYPE_UNDEFINED	0x00
#define	TYPE_DATA	0x01
#define	TYPE_PROCEDURE	0x02
#define	TYPE_COMMON	0x03
#define	TYPE_BSS	0x04
#define	TYPE_CDATA	0x05
#define	TYPE_ABSOLUTE	0x06
#define	TYPE_SHL_PROC	0x07
#define	TYPE_INTRA	0x10

#define	TYPE_MASK	0x7f

#define	TYPE_EXTERN2	0x80

/* shared library export supplement */
struct shl_export_entry
{
	long dreloc;		/* index of first dreloc entry for symbol */
	long next_symbol;	/* index to others symbols for this piece */
	long dmodule;		/* index into dynamic module table */
	long next_module;	/* index to other symbols in this module */
	long spare1;
};

/* hash table for export */
struct hash_entry
{
	long symbol;		/* index into export table */
};

/* module import table entry */
struct module_entry
{
	long import;		/* index into import table */
};

#define	MODULE_IMPORT_MASK	0x0fffffff
#define	MODULE_DLT_FLAG		0x20000000
#define	MODULE_END_FLAG		0x10000000

/* dynamic loader module table entry */
struct dmodule_entry
{
	unsigned long flags;
	long module_imports;
	long dreloc;
	long a_dlt;
};

#define	DM_END		0xffffffff
#define	DM_BOUND	0x00000001
#define	DM_DLTS		0x00000002
#define	DM_INVOKES	0x00000004

/* dynamic relocation record */
struct relocation_entry
{
	long address;		/* offset from file text */
	long symbol;		/* index into import table */
	unsigned char type;	/* external or file text relative */
	char length;		/* byte, word, or long */
	short spare1;
};

/* relocation types */
#define	DR_NOOP		0xff
#define	DR_END		0x00
#define	DR_EXT		0x01
#define	DR_FILE		0x02
#define	DR_PROPAGATE	0x03
#define	DR_COPY		0x04
#define	DR_INVOKE	0x05

/* relocation sizes */
#define	DR_BYTE		0x01
#define	DR_WORD		0x02
#define	DR_LONG		0x04

/* tables placed at end of data segment */

/* data linkage table */
struct dlt_entry
{
	char *address;		/* absolute address of symbol */
};

/* procedure linkage table */
struct plt_entry
{
	unsigned char opcode;	/* BRA or BSR */
	unsigned char size;	/* BYTE, WORD, or LONG */
	long displacement;	/* target - here */
	short spare1;
};

/* opcodes */
#define	BRA	0x60
#define	BSR	0x61

/* sizes */
#define	LONG	0xff

/* DYNAMIC structure */
struct dynamic
{
	struct dynamic *next_shl;	/* pointer to next shared library */
	unsigned long text;		/* address of text segment */
	unsigned long data;		/* address of data segment */
	unsigned long bss;		/* address of bss segment */
	unsigned long end;		/* end of text/data/bss */
	char *dmodule;			/* address of dynamic module flags */
	struct dlt_entry *dlt;		/* address of DLT */
	struct dlt_entry *a_dlt;	/* address of anonymous DLT */
	struct plt_entry *plt;		/* address of PLT */
	struct plt_entry bor;		/* a real PLT entry for deferred bind */
	const struct header_extension *dl_header;	/* address of header */
	short status;			/* status of library in process */
	short spare1;
	unsigned char *bound;		/* status of imports */
	const struct shl_entry *shl;	/* address of shl entry in executable */
	unsigned long ptext;		/* pseudo "file text" address */
	long elaborator;		/* function to call on DR_INVOKE */
	long initializer;		/* function to call on load/unload */
	unsigned long dld_flags;	/* flags for debugger */
	unsigned long dld_hook;		/* routine to call at hook points */
	unsigned long dld_list;		/* head of symbol search path */
	char *shlib_path;		/* pointer to $SHLIB_PATH in data */
	int embed_path;			/* string table offset of "+b" path */
	long spare2[57];
};

/* status flags */
#define	ATTACHED	0x0001
#define	DLT_BOUND	0x0002	/* data linkage table bound */
#define	PLT_BOUND	0x0004	/* procedure linkage table bound */
#define	DRELOCATED	0x0008	/* dynamic relocation records processed */
#define	INITIALIZE	0x0010	/* requires initialization */
#define DYNAMIC_LAST	0x0020	/* search $SHLIB_PATH after "+b" path */
#define DYNAMIC_FIRST	0x0040	/* search $SHLIB_PATH before "+b" path */

#endif		/* __hp9000s300 */

#ifdef 		__hp9000s800

/*
 *  HP 9000 Series 800 Linker
 *  External Definitions for Shared Library manipulation routines and
 *					    data structures
 *
 *  Copyright Hewlett-Packard Co. 1989
 *
 *  $Header: shl.h,v 70.7 92/04/22 10:50:25 ssa Exp $
 */

#define DL_HDR_VERSION_ID	89060912
#define SHLIB_UNW_VERS_ID	89081712

#define DLT_ENTRY     int

struct dl_header {
    int	hdr_version;		/* header version number */
    int	ltptr_value;		/* space-rel offset of linkage table ptr(r19)*/
    int	shlib_list_loc;		/* text relative offset of shlib list */
    int	shlib_list_count;	/* count of items in shlib list */
    int	import_list_loc;	/* text relative offset of import list */
    int	import_list_count;	/* count of items in import list */
    int	hash_table_loc;		/* text relative offset of export hash table */
    int	hash_table_size;	/* count of slots in export hash table */
    int	export_list_loc;	/* text relative offset of export list */
    int	export_list_count;	/* count of items in export list */
    int	string_table_loc;	/* text relative offset of string table */
    int	string_table_size;	/* length in bytes of string table */

    int	dreloc_loc;	/* text-relative offset of dynamic relocation records */
    int	dreloc_count;	/* number of dynamic relocation records */
    int dlt_loc;	/* space-relative offset of data linkage table */
    int plt_loc;	/* space-relative offset of procedure linkage table */
    int dlt_count;	/* number of dlt entries in linkage table */
    int plt_count;	/* number of plt entries in linkage table */
    short highwater_mark; 	/* highest version number seen in the library */
    short flags;	/* various flags -- currently elab_used, init_used */
    int export_ext_loc; /* text-relative offset of export extension table */  
    int module_loc;     /* text-relative offset of module table */
    int module_count;   /* number of module entries */
    int elaborator;	/* import index of elaborator */
    int initializer;	/* import index of initializer */
    int embedded_path;	/* index into the string_table for the search path */
			/* index must be > 0 to be valid                   */
    int reserved2;	
    int reserved3;	
    int reserved4;	
};

/* dl_header flags */
#define ELAB_DEFINED 0x1  /* An elaborator has been defined for this library  */
#define INIT_DEFINED 0x2  /* An initializer has been defined for this library */
#define SHLIB_PATH_ENABLE      0x4  /* allow search of SHLIB_PATH at runtime  */
#define EMBED_PATH_ENABLE      0x8  /* allow search of embed path at runtime  */
#define SHLIB_PATH_FIRST       0x10 /* search SHLIB_PATH first                */

struct import_entry {
    int	name;		/* offset in string table */
    short lib_index;	/* index into the shared library list */
    unsigned char type;	/* symbol type */
    unsigned char reserved1;
};

#ifndef EXPORT_VERSION

struct misc_info {
    short version;		/* months since January, 1990 */
    unsigned int reserved2 :  6;
    unsigned int arg_reloc : 10; /* parameter relocation bits (5 * 2) */
};

struct export_entry {
    int 	    next;	/* index of next export entry in hash chain */
    int 	    name;	/* offset within string table */
    int 	    value;	/* offset of symbol (subject to relocation) */
    union {
    	int   size;		/* storage request area size in bytes */
    	struct misc_info misc;	/* version, etc. N/A to storage requests */
    } info;
    unsigned char type;		/* symbol type */
    char	reserved1;	/* currently unused */
    short	module_index;   /* index of module defining this symbol */
};

#define EXPORT_VERSION		info.misc.version
#define EXPORT_ARG_RELOC	info.misc.arg_reloc

#endif   /* ifndef EXPORT_VERSION */

/* extension records to be used in shared libraries */
struct export_entry_ext {
    int size;	     /* export symbol size, data only. */
    int dreloc;      /* start of dreloc list for this symbol. */
    int same_list;   /* circular list of exports that have the same value */
    int reserved2;
    int reserved3;
};

struct shlib_list_entry {
    int	shlib_name;		/* offset within string table */
    unsigned char dash_l_reference;	/* referenced with -l<> (True) or 
					   absolute path (False) */
    unsigned char bind;		/* bind immediate, or deferred, or reference */
    short highwater_mark;	/* highest version number of any exported sym */
};

struct PLT_entry {
    int	proc_addr;	/* address  of procedure */
    int	ltptr_value;	/* value of r19 required for this procedure */
};

struct shlib_unwind_info {
	int	magic;	        /* magic number for unwind detection */
	int	shlib_name;	/* index into string table */
	int	text_start;	/* virtual address of the start of text */
	int	data_start;	/* virtual address of the start of data */
	int	unwind_start;	/* text-relative offset of unwind table */ 
	int	unwind_end;	/* text-relative offset of stub unwind table */
	int	recover_start;  /* text-relative offset of recover table */
	int	recover_end;	/* text-relative offset of the line table */
};

/* dynamic relocation record */
struct dreloc_record {
	int	shlib;    /* if a.out fixup, this is the index in shlib_list of 
			     shlib from which this fixup was copied, else -1 */
	int	symbol;	  /* index into import table if *_EXT type */
	int 	location; /* offset of location to patch (dp-relative); */
	int	value;	  /* text or data-relative offset to use for patch if
			     internal-type fixup; else NULL */
	unsigned char type; /* type of dreloc record */
	char	reserved;     /* currently unused */
 	short	module_index; /* index of module that this record refers to */	
};

/* dynamic relocation types */
#define DR_PLABEL_EXT 	1
#define DR_PLABEL_INT 	2
#define DR_DATA_EXT 	3
#define DR_DATA_INT 	4
#define DR_PROPAGATE    5
#define DR_INVOKE       6
#define DR_TEXT_INT     7

/* module descriptor */
struct module_entry {
        int drelocs;       /* text-relative offset into module dynamic 
			      relocation array. */
 	int imports;       /* text-relative offset into module import array */ 
	int import_count;  /* number of entries into module import array    */ 
        char flags;        /* currently flags defined: ELAB_REF */
        char reserved1;
        unsigned short module_dependencies;
        int reserved2;
};

/* module flags */
#define ELAB_REF	0x1   	/* elaborator referenced in this module */

/* extra parms passed to dld from crt0 */
struct dld_parms {
    long version;               /*  version num of dld_parms */
    long text_addr;             /*  text address of dld */
    long text_end;              /*  text end of dld */
    long prog_data_addr;        /*  start of data in program file */
    char **envp;	        /*  environment pointer */
};

#define PARMS_STRUCT_FLD4	0
#define PARMS_STRUCT_FLD5	1

#define PARMS_STRUCT_USED      -1

#endif 		/* __hp9000s800 */

#endif		/* _SHL_INCLUDED */
