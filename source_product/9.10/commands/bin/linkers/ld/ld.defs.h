/* Internal RCS $Header: ld.defs.h,v 70.9 93/10/26 10:06:44 ssa Exp $ */

/* @(#) $Revision: 70.9 $ */     

#define  _HPUX_SOURCE

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

     Command:      Series 200 ld   ( HP -UX link editor )
     File   :      ld.defs.h

     Purpose:      definitions used throughout the source code
	   - common structured types
	   - widely used constants
	   - external variables and functions

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* The following mechanism let's us get away with only one definition for
 * each global variable.  LD_MAIN is defined by main.c only.
 */
#ifdef   LD_MAIN
#define  VAR
#define  I(a)    a
#else
#define  VAR     extern
#define  I(a)
#endif

/* These defines help with error recovery tasks.  Memory related routines
 * will check for NULL pointers returned. 
 */
char *ld_malloc(), *ld_calloc(), *ld_realloc();
#if  CHKMEM
#include "chkmem.h"
#undef malloc
#undef calloc
#undef realloc
#define    malloc( a )            ld_malloc( a, __FILE__, __LINE__ )
#define    calloc( a, b )         ld_calloc( a, b, __FILE__, __LINE__ )
#define    realloc( a, b )        ld_realloc( a, b, __FILE__, __LINE__ )
#else
#define    malloc( a )            ld_malloc( a )
#define    calloc( a, b )         ld_calloc( a, b )
#define    realloc( a, b )        ld_realloc( a, b )
#endif

/* pxdb is invoked from ld for version including and after 8.0.  The standard
 * pxdb is given by the define PXDB.  this can be overridden from the
 * command line if so desired.
 */
#ifndef   PXDB
#define   PXDB    "/usr/bin/pxdb"
#endif

/* this is the standard LPATH used if no LPATH environemt variable is set.
 */
#ifndef   STANDARD_LPATH
#define   STANDARD_LPATH   "/lib:/usr/lib"
#endif

/* control which version of ld we are making.  the default, unless it
 * is overridden from the cc command line is to build the 8.0 version.
 */
#ifndef VERSION
#define VERSION   90
#endif

/*
    #define's controlled by VERSION:

    The following symbols should be set to unique integers
    VFIXES      define this to get kludge
		in which linker changes references to certain symbols
		in old object files to the new name in the library
		define VFIXES to be the number of symbols to handle

    STACK_ZERO  define this to be the initial value of "___stack_zero"

    FTN_IO      define this to get kludge in which linker warns about
        unresolved externals that look like they might have been caused by
        mixing 300 and 800 style Fortran I/O library calls
*/

#if VERSION == 80
#define     STACK_ZERO   0x10000
#define     FTN_IO
#define     CPLUSPLUS

#define     EXPORTALIGNFIX
#define     SHLPROCFIX
#define     PICR
#define     MISC_OPTS
#define     EVIL_FIX
#define     DL_MAGIC_RPC_FIX
#define     XGFIX
#define     FUZZY2FIX

#elif VERSION == 801 /* PATCH */
#define     STACK_ZERO   0x10000
#define     FTN_IO
#define     CPLUSPLUS

#define     EXPORTALIGNFIX
#define     SHLPROCFIX
#define     PICR
#define     MISC_OPTS
#define     EVIL_FIX
#define     DL_MAGIC_RPC_FIX
#define     XGFIX
#define     FUZZY2FIX
#define     SKIP_OLD_VERSIONS
#define     RESTORE_MOD
#define		DONT_RESOLVE_RPC_TO_ABS
#define		REXT_LOCAL_FIX
#define		ZERO_MODULO_FIX

#elif VERSION == 802 /* ADA */
#define     STACK_ZERO   0x10000
#define     FTN_IO
#define     CPLUSPLUS

#define     EXPORTALIGNFIX
#define     SHLPROCFIX
#define     PICR
#define     MISC_OPTS
#define     EVIL_FIX
#define     DL_MAGIC_RPC_FIX
#define     XGFIX
#define     FUZZY2FIX
#define     SKIP_OLD_VERSIONS
#define     RESTORE_MOD
#define		DONT_RESOLVE_RPC_TO_ABS
#define		REXT_LOCAL_FIX
#define		ZERO_MODULO_FIX

#define		RECURSE_FIX
#define		SHORT_SYMBOLNUM_FIX
#define		EXPORT_MORE
#define		SAVE_SLOT
#define		TEMPLATES
#define		ALIGN_DORIGIN_FIX
#define		N_INCOMPLETE_FIX

#elif VERSION == 90
#define     STACK_ZERO   0x2000
#define     CPLUSPLUS

#define     EXPORTALIGNFIX
#define     SHLPROCFIX
#define     PICR
#define     MISC_OPTS
#define     EVIL_FIX
#define     DL_MAGIC_RPC_FIX
#define     XGFIX
#define     FUZZY2FIX
#define     ELABORATOR
#define     HOOKS
#define     VISIBILITY
#define     BIND_FLAGS
#define     SKIP_OLD_VERSIONS
#define     RESTORE_MOD
#define		DONT_RESOLVE_RPC_TO_ABS
#define		REXT_LOCAL_FIX
#define		ZERO_MODULO_FIX

#define		RECURSE_FIX
#define		SHORT_SYMBOLNUM_FIX
#define		EXPORT_MORE
#define		SAVE_SLOT
#define		TEMPLATES
#define		ALIGN_DORIGIN_FIX
#define		SHLIB_PATH
#define		SHLIB_PATH_LEN		1024
#define		N_INCOMPLETE_FIX
#define		NO_INTRA_FIX
#endif

/*----------------------------------------------------------------------
 * Include Files
 */
#include <stdio.h>
#include <stddef.h>
#include <a.out.h>
#include <ranlib.h>
#ifndef   _OFF_T
#define   _OFF_T          /* Bug in 7.X header files */
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <symtab.h>
#include <ar.h>
#include <assert.h>


/*----------------------------------------------------------------------
 * Constant Definitions
 */

/* this is the highest object file version number ld will willingly accept 
 */
#define HIGHEST_VERSION     3

/* these next three definitions are for cross compiling on the 800.  They
 * used to be defined in the code using  sizeof( )  some structure, but
 * the sizes are different on the 800 
 */
#define NLIST_SIZE 10            /* sizeof(cursym.s) and sizeof(sp->s) */
#define SYMBOLSIZE sizeof(struct symbol)      /* sizeof(struct symbol) */
#define ALIGN_ENTRY_SIZE sizeof(struct align_entry)     /* sizeof(*a1) */

/* initial size of symtab and hashmap, the linkers symbol table
 */
#define NSYM       2003    /* initial size of linker symbol table */
#define NSYMHASH   5003    /* initial size of linker symbol hash table */

/* bit maps to get at various bits of a word
 */
#define LOBYTE     0377    /* bit map to obtain low byte of a word */
#define FIVEBITS    037    /* bit map to obtain low 5 bits of a char */
#define FOURBITS    017    /* bit map to obtain low 4 bits of a char */
#define WSPACE        0    /* white space character */

/* address that a.out's are linked at (by default, -R can override)
 */
#define LADDR         0     /* load address */

/* size of text+data over which we make the file demand loaded
 */
#ifndef DEMAND_THRESHOLD
#define DEMAND_THRESHOLD    EXEC_PAGESIZE*20
#endif


/*----------------------------------------------------------------------
 * Structure Definitions
 */ 

/* This definition of struct nlist_ should be identical to the one in 
 * a.out.h except that the bitfields have been collapsed into n_flags 
 * instead of having n_plt, n_dlt, n_dreloc, and n_list.  Since many of
 * the bits in that word are unused, we use them for internal purposes
 * as well, and strip them when we output the symbol table.
 */
struct internal_nlist_ {
    long            n_value;     /* These four should be the same */
    unsigned char   n_type;
    unsigned char   n_length;
    short           n_almod;
    unsigned short  n_flags;     /* Comprises n_dlt, n_plt, n_dreloc, n_list */
#define   NLIST_DLT_FLAG            0x8000  /* Should be like in a.out.h */
#define   NLIST_PLT_FLAG            0x4000
#define   NLIST_DRELOC_FLAG         0x2000
#define   NLIST_LIST_FLAG           0x1000  /* end of list (size) */
#define   NLIST_REAL_MASK           0xf000  /* Mask for flags which are real */
#define   NLIST_STILL_FUZZY_FLAG    0x0001
#define   NLIST_EVER_FUZZY_FLAG     0x0002
#define   NLIST_EXPORT_PLT_FLAG     0x0004  /* if a plt is created in a.out */
#define   NLIST_STRING_TABLE_FLAG   0x0008
};

/* This is our internal symbol table, which has _nlist (above) as one part
 * of it.  This contains all the neat crufties that link symbols to 
 * the hash table and to other symbols and other tables.  Some contants
 * are included after the field that they may be found in.
 */
typedef struct symbol *symp;            /* ptr to a symbol */
struct symbol                           /* symbol definition */
{
    struct internal_nlist_  s;  /* type and value */
    char        *sname;         /* pointer to ascii name */
    int         shash;          /* hash table index */
    unsigned    sindex;         /* id (index) of symbol in symtab */
    long        stincr;         /* total increment due to previous .aligns */
    short       sincr;          /* whitespace size due to this .align */
#ifdef  VFIXES
    short       version;        /* which version .o did this come from */
    short       next_num;       /* number of the 'next' field */
#else
    short       dummy;          /* to make it a 32 byte value */
    short       next_num;
#endif
    unsigned short   flags;        /* Flags from nlist_ entry */
#ifdef   RESTORE_MOD
    long        save_shlib;
    long        save_expindex;
#endif
    long        next;              /* Used for internal symtab linked lists */
    long        expindex;          /* Export list entry index */
#define  EXP_UNDEF          -1       /* Symbol not assigned an export */
#define  EXP_DONT_EXPORT    -2       /* Don't assign this symbol an exp */
    long        dltindex;          /* DLT list entry index */
#define  DLT_UNDEF          -1       /* Symbol has yet to be assigned a DLT */
#define  ABS_DLT_FLAG  0x40000000    /* That DLT is an abs_dlt */
    long        pltindex;          /* PLT list entry index */
#define  PLT_UNDEF          -1       /* Symbol has yet to be assigned a PLT */
#define  PLT_LOCAL_RESOLVE  -2       /* Symbol is local, don't use a PLT */
    long        rimpindex;         /* Dynamic Relocation list entry index */
#define  RIMP_UNDEF         -1       /* Symbol has yet to be given a rimp */
    long        shlib;             /* Shlib this symbol Resides in */
#define  SHLIB_UNDEF        -1       /* Symbol is not from a shared lib */
    long        size;              /* Size of this item */
#ifdef	VISIBILITY
	long		module;
#endif
    long        alias;             /* linked list of shlib symbol aliases */
};

/* struct arg_link builds a linked list of arguments which are not options
 * which are passed to ld.  For example, .o's, .a's, .sl's, -l's, etc. are
 * considered arguments and are placed on this list.  
 */
typedef struct arg_link *arg;           /* linked list of arguments */
struct arg_link
{
    char *arg_name;         /* the actual argument string */
    arg arg_next;           /* next one in linked list */
    unsigned char lib;
    unsigned char bind;
};

/* The hidelist is the linked list of symbols which were specified by a
 * '-h' on the command line.  Symbols on this list will be stripped of their
 * EXTERN bit when they are output in the symbol table.
 */
typedef struct hide_link *hide;     /* linked list of symbols to be hidden */
struct hide_link
{
    char *hide_name;        /* the actual argument string */
    hide hide_next;         /* next one in linked list */
};

/* The undef list is very much like the hide list, where these symbols
 * are entered into the symbol table before pass1, as UNDEF's, so that
 * any definitions for them will be brought in.
 */
typedef struct undef_link *undef;   /* linked list of -u symbols */
struct undef_link
{
    char *undef_name;       /* the actual argument string */
    undef undef_next;       /* next one in linked list */
};

/* The predef list is analogous, but it allows you to predefine symbols */
typedef struct predef_link *predef;
struct predef_link
{
	char *predef_name;
	long predef_value;
	predef predef_next;
};

#ifdef	MISC_OPTS
typedef struct generic_link *generic;
struct generic_link
{
	char *name;
	generic next;
};
#endif

typedef struct arc_entry *arce;       /* linked list of archive entries */
struct arc_entry
{
    long    arc_offs;       /* offset in the file of this entry */
    arce    arc_e_next;     /* next entry, NULL if none */
};

typedef struct arc_link *arcp;        /* linked list of archive files */
struct arc_link
{
    arce arc_e_list;        /* list of archive entries */
    arcp arc_next;          /* next archive */
};


typedef struct align_entry *aligne;     /* linked list of align symbols */
struct align_entry
{
    symp    asymp;          /* ptr to hash table entry for aligner */
    aligne  nextale;        /* ptr to next element of the list */
    short   modulo;         /* alignment modulo specifier */
};

/* This structure is used to pass information from pass1 to pass2 about
 * what areas of the input file should be copied to the output file.  It 
 * is produced by AddShlib() and it's routines, and is terminated after
 * each shared library with a record containing .type = end_marker.
 */
struct shl_data_copy 
{
    short   type;           /* Type of the memory, usually data or text */
    short   align;          /* Amount to align this item to */
    long    offset;         /* offset within shared library for data item */
    long    size;           /* size of the data item to copy */
    long    sym;            /* Symbol Table slot for this entry */
};

/* Like struct shl_data_copy, this structure is for copying data from the 
 * shared library into the output file.  But since the area may be fuzzy,
 * this stuff is copied after all the rest of the object modules and
 * shared libraries have been processed.
 */ 
struct fuzzy_shl_copy
{
    long     sym;          /* Indicates when it is done, etc. */
    unsigned char flags;    /* Varios flags */
#define    FUZZY_PROPOGATE_DRELOC       1
    signed char  align;        /* Alignment in the original shared library */
    unsigned short shlib;      /* Shared library index */
    long     offset;       /* offset within shared library for data item */
    long     size;         /* Size of the data item */
    long     next;         /* Next item in linked list of items to copy */
};

typedef struct sized_data_entry *sdp;   /* linked list of sized data items */
struct sized_data_entry
{
    symp    sdsp;           /* pointer to symbol table entry */
    long    sdvalue;        /* maximum COMM size specified */
    long    sdfollow;       /* DATA element following this one */
    sdp sdnext;         /* next element of list */
};


/*----------------------------------------------------------------------
 * Global Variables
 */

VAR int            exit_status;     /* status to return eventually */
VAR arg            arglist;         /* linked list of arguments */
VAR hide           hidelist;        /* linked list of hidden symbols */
VAR undef          undeflist;       /* linked list of -u symbols */
VAR predef         predeflist;      /* linked list of -P symbols */
#ifdef	MISC_OPTS
VAR generic        trace_list I(= NULL);
#endif
VAR struct ar_hdr  archdr;          /* directory part of archive */
VAR arcp           arclist;         /* list of archives */
VAR arcp           arclast;         /* last archive on arclist */
VAR arce           arcelast;        /* last entry in this entry list */
VAR struct exec    filhdr;          /* header file for current file */
VAR struct symbol  cursym;          /* current symbol */
VAR char       csymbuf[SYMLENGTH];  /* buffer for current symbol name */
VAR struct symbol* symtab;          /* ptr to actual symbols */

VAR symp           lastsym;         /* last symbol entered */
VAR symp           Asymp;           /* end of -A symbols */

VAR int         dont_import_syms;   /* symtab index's not to include */
#ifdef	VISIBILITY
VAR int dont_export_syms;
#endif

VAR int            high_highwater;  /* Highest HOH we have seen on any .o */
VAR int            highwater;       /* HOH for the current file */
VAR int            shlibtextsize;   /* size of the shlib extension header */

VAR struct export_entry *exports;          /* export table */
VAR struct shl_export_entry *shl_exports;  /* shlib export supplement */
VAR int        ld_expsize I(= 2*NSYM);     /* size of export table */
VAR int        expindex;                   /* next export table entry */
VAR struct hash_entry   *exphash;          /* export hash table */
VAR int                 ld_exphashsize;    /* size of export hash table */

VAR char  *stringt, *sym_stringt;          /* string table */
VAR int   ld_stringsize I(= 5*NSYM);       /* size of shlib string table */
VAR int   ld_sym_stringsize I(= 10*NSYM);  /* size of symtab string table */
VAR int   stringindex, sym_stringindex;    /* next string table entry */

VAR struct import_entry *dltimports;       /* import table (dlt) */
VAR struct dlt_entry    *dlt;              /* import table (dlt) */
VAR int      ld_dltsize I(= NSYM);         /* size of dlt import table */
VAR int      dltindex;                     /* next dlt table entry */
VAR int      global_dlt;                   /* get start of anonymous dlt's */
VAR struct dlt_entry    *abs_dlt;          /* Absolute dlt's (hidden) */
VAR int                  abs_dltindex;     /* next abs_dlt table entry */
VAR int      ld_abs_dltsize I(= 10);       /* size of the abs_dlt table */
VAR int                  middle_dlt;       /* Middle dlt entry */

VAR struct import_entry *pltimports;        /* import table (plt)        */
VAR long                *pltchain;          /* links plts for same sym   */
VAR int      ld_pltsize I(= NSYM);          /* size of plt import table  */
VAR int                  pltindex;          /* next plt table entry      */

VAR int             local_dlt;
VAR long                plt_start;
VAR int                 plt_align;

VAR int bound_size;

VAR struct shl_entry *shlibs;       /* shlibs table              */
VAR int     ld_shlsize I(= 20);         /* size of shlibs table      */
VAR int     shlindex I(= 1);        /* next shlibs table entry   */

VAR struct shl_data_copy  *shldata;       /* data copied from shared libs */
VAR long                  shldata_size I(= 500);  
VAR long                  shldata_idx;
VAR long                  shldata_nidx;
VAR struct fuzzy_shl_copy *fuzzy_shl;
VAR long                  fuzzy_shl_idx;
VAR long                  fuzzy_shl_size I(= 250);
VAR long                  fuzzy_head;

VAR struct relocation_entry *relocs;
VAR struct import_entry *reloc_imports;
VAR int                      relindex I(= 1);
VAR int                      curr_relindex;
VAR long                     rimpindex;
VAR long                     ld_rimpsize I(= 100);

VAR struct exec              save_filhdr;
VAR struct header_extension  extheader;
VAR struct header_extension  debugheader;
VAR struct header_extension  debugheader2;
VAR struct dynamic           dynheader;

/* shlib_level is an internal number which represents how much infomration 
 * should be provided in the output file.  It is set with -b, -i and by
 * using a shared library when linking an a.out.  The order of these 
 * constants is important (since they somewhat indicate order of increasing
 * information in the a.out file).
 */
VAR int       shlib_level;
#define         SHLIB_NONE     0    /* None used */
#define         SHLIB_DLD      1    /* DLD being build (-i) */
#define         SHLIB_A_OUT    2    /* a.out uses shared libs */
#define         SHLIB_BUILD    3    /* building a shared lib */

VAR int                text_reloc_seen;
VAR int                dyn_count;          /* Number of dyn relocations */
VAR long               real_dorigin;

VAR long changed_head;   /* Used in enter() and load1() */

struct mod_entry
{
    long first_seen, last_seen;
};
VAR struct mod_entry *ld_module;
VAR long mod_number;          /* Current module number */
VAR long object_number;       /* Current object number */
VAR long ld_mod_number I(= 500);       /* Size which has been malloced() already */
VAR long mod_index;           /* Current module lists index */
VAR long ld_mod_index I(= 1500);        /* Size which has been malloced() already */
VAR long *module_next;        /* Array of next pointers */
VAR struct dmodule_entry *module; /* Module information */
VAR struct module_entry *module_table;   /* The actual table,
					     * containing lists of pointers to
					     * the import table.  */
#define   MODULE_ADD_DLTINDEX   0x40000000  /* Intern flags in module_table */
#define   MODULE_ADD_BOTH       0x20000000


VAR int            symindex;            /* next available symbol table entry */
VAR int *          hashmap;         /* ptr to hash table for symbols */
VAR int *          chain;           /* ptr to hash table for symbols */
VAR symp *         local;           /* ptr to symbols in current file */
VAR int            nloc;            /* number of local symbols per file */
VAR int            nloc_items;      /* number of local symbols, on a 
				     * symbol basis (used for supsym_gen)
				     */
VAR int            nund;            /* number of undefined syms in pass 2*/
VAR symp           entrypt;         /* pointer to entry point symbol */
VAR int            argnum;          /* current argument number */
VAR unsigned short align_seen;          /* true if align sym in current file */
VAR int            last_local;      /* index of last symbol in local syms */
VAR int            stamp;           /* value of -V option */
VAR int            entryval;            /* default entry value */
VAR int            ld_stsize I(= NSYM);     /* linker symbol table size */
VAR int            ld_sthashsize I(= NSYMHASH);    /* linker symbol table hash size */
VAR int            hash_cycle;      /* hash cycle size */

VAR char *         ofilename I(= OUT_NAME);     /* name of output file, default init */
VAR char *         filename;            /* name of current input file */
VAR char *         tempname;            /* name of temporary file for output */
VAR char *         Aname;           /* -A file */
VAR char *         e_name;          /* -e symbol */
VAR char *         lpath;           /* path to search libs specified via -l*/
VAR FILE *         text;            /* file descriptor for input file */
VAR FILE *         tout;            /* text portion */
VAR FILE *         dout;            /* data portion */
VAR FILE *         trout;           /* text relocation commands */
VAR FILE *         drout;           /* data relocation commands */
VAR FILE *         debugextheaderout;   /* EXTENSION HEADER */
VAR FILE *         headerout;       /* HEADER */
VAR FILE *         gnttout;         /* GNTT */
VAR FILE *         lnttout;         /* LNTT */
VAR FILE *         sltout;          /* SLT */
VAR FILE *         vtout;           /* VT */
VAR FILE *         xtout;           /* XT */
VAR int bss2data;
#ifdef	FTN_IO
VAR int ftn_io_mismatch;
#endif

/* flags */
VAR unsigned short  xflag;      /* discard local symbols */
VAR unsigned short  rflag;      /* preserve relocation bits, don't define common */
VAR unsigned short plus_rflag;	/* preserve relocation only */
/* In the -r case, if all the object modules contain supsym information,
 * we want to build a composite object module which also has a supsym table.
 * 'supsym_gen' is a flag which indicates that a supsym table is being 
 * built, while 'supsym' is the actual table, which is built in pass1.
 */
VAR unsigned short          supsym_gen; 
VAR struct supsym_entry     *supsym;  
VAR unsigned short  sflag;      /* discard all symbols */
VAR unsigned short  dflag;      /* define common even with rflag */
VAR unsigned short  Rflag;      /* -R seen in arguments - sets torigin */
VAR unsigned short  tflag;      /* list files used in link */
VAR unsigned short  nflag I(= 1);       /* create a 0410 file with segment alignment*/
VAR unsigned short  qflag;      /* create a 0413 file with segment alignment*/
VAR unsigned short  Vflag;      /* put a version stamp in the header */
VAR unsigned short  vflag;      /* verbose flag */

/* States for the 'aflag' and for the internal variable libtype.  Don't
 * change these values without first looking at sym.c:getfile().
 */
#define  EITHER   0   
#define  SHARED   1
#define  ARCHIVE  2
VAR unsigned short  aflag I(= EITHER);    /* Library search specification */

VAR unsigned short  bflag;                /* making a shared library flag */
VAR unsigned short  Bflag I(= BIND_DEFERRED);    /* Flag for type of load */
VAR unsigned short  iflag;                /* DLD build flag */
VAR unsigned short  Fflag;                /* Export ALL Global Symbols */

VAR int         savernum I(= -1);       /* argument for -R option */
VAR unsigned short   funding;   /* process -A file */
VAR unsigned short   warn_xg;   /* warn user about using -x on a debuggable file */
#ifdef	VFIXES
VAR unsigned short  version_seen[HIGHEST_VERSION+1];
#else
VAR short highest_seen I(= -1);
#endif
VAR unsigned short   this_version;  /* for -v version number printing */

#ifdef  VFIXES
struct vsym
{
    int location;       /* location of symbol in table */
    char    name[SYMLENGTH];    /* name of symbol */
};
struct vfix
{
    struct vsym new;        /* new version */
    struct vsym old;        /* backward compatible version */
    int     type;       /* type of symbol */
};
VAR struct vfix vfixes[VFIXES];
#endif

/* used after pass 1 */
VAR long torigin;       /* origin of text segment in final output */
VAR long dorigin;       /* origin of data segment in final output */
VAR long doffset;       /* current position in output data segment */
VAR long borigin;       /* origin of bss segment in final output */
VAR long corigin;       /* origin of common area */
VAR long morigin;       /* origin of MIS area */
VAR long lestorigin;        /* origin of lest */

/* cumulative sizes set in pass 1 */
VAR long    tsize;      /* size of text segment */
VAR long    dsize;      /* size of data segment */
VAR long    bsize;      /* size of bss segment */
VAR long    csize;      /* size of common area */
VAR long    rtsize;     /* size of text relocation area */
VAR long    rdsize;     /* size of data relocation area */
VAR long    ssize;      /* size of l.e. symbol area */

VAR long    debugoffset; /* offset within file of debug info */
VAR long    headersize;  /* size of HEADER */
VAR long    gnttsize;    /* size of GNTT */
VAR long    lnttsize;    /* size of LNTT */
VAR long    sltsize;     /* size of SLT */
VAR long    vtsize;      /* size of VT */
VAR long    xtsize;      /* size of XT */

/* variables needed to support ranlib */
VAR int rasciisize; /* sizeof ranlib asciz table in bytes */
VAR short   rantnum;    /* sizeof ranlib array (# of structures) */
VAR struct  ranlib  *tab;   /* dynamically allocated ranlib array head */
VAR char    *tabstr;    /* dynamically allocated string table for ranlib */
VAR long infilepos;  /* current position in input file (usually an archive) */

/* symbol relocation; both passes */
VAR long    ctrel;
VAR long    cdrel;
VAR long    cbrel;

/* variables to support dynamic alignment */
VAR aligne  alptr;  /* ptr to top of aligne linked list */

/* sized data items */
VAR sdp first_sized_data;   /* top of sd linked list */

#ifdef	ELABORATOR
VAR char *initializer_name I(= "__INITIALIZER");
VAR char *elaborator_name I(= "__ELABORATOR");
VAR int elaborator_index I(= -1);
#endif

#ifdef	HOOKS
VAR char *hook_name I(= "__DLD_HOOK");
#endif

#ifdef	VISIBILITY
VAR int hide_status;
#define		HIDE_HIDES	1
#define		EXPORT_HIDES	2
#endif

#ifdef	PICR
VAR int pic I(= M_PIC | M_DL);
VAR int global_align_seen;
#endif

#ifdef SHLIB_PATH
VAR char *embed_path;
VAR int enable_shlib_path;
#endif

#ifdef N_INCOMPLETE_FIX
VAR int shlibtext_pad;
#endif

/* error messages */
extern char *e1,*e2,*e3,*e4,*e5,*e6,*e7,*e8,*e9,*e10;
extern char *e11,*e12,*e13,*e14,*e15,*e16,*e17,*e18,*e19,*e20;
extern char *e21,*e22,*e23,*e24,*e25,*e26,*e27,*e28,*e29,*e30;
extern char *e31,*e32,*e33,*e34,*e35,*e36,*e37,*e38,*e39,*e40;
extern char *e41;
extern char *e42, *e43, *e44, *e45, *e46, *e47, *e48, *e53, *e54, *e55;
extern char *e49, *e50, *e51, *e52;

/* functions */

extern int          lookup();
extern int          slookup();
extern char *       asciz();
extern int          expand();

/*
 * values returned by ExpLookup()
 */
#define FOUND    0
#define BADMINOR 1
#define NOTFOUND 2


#define  FALSE   0
#define  TRUE    1

#ifdef   __STDC__
/* Load the function prototypes */
#include <stdarg.h>
#include "funcs.h"

/* These were changed for name space reasons */
#define  _cnt   __cnt
#define  _ptr   __ptr
#endif
