#ifndef _SYMTAB_INCLUDED /* allow multiple inclusions */
#define _SYMTAB_INCLUDED
/*****************************************************************
 * $Source: /misc/source_product/9.10/commands.rcs/usr/include/symtab.h,v $
 * $Revision: 70.6 $
 * $Date: 92/05/04 15:07:36 $
 *****************************************************************
 * Revision 66.25  91/01/13  19:03:48  19:03:48  ssa (Shared Source Administrator)
 * Author: jmn@sluggo
 * added DNTT_FUNC.localloc flag; for C++300 compiler (jmn for bruno)
 * 
 * Revision 66.24  90/08/21  10:38:03  10:38:03  markm (Mark McDowell)
 * A distinction is now made between "float" and "double" for the declared
 * type of a parameter or variable.  This allows us to print the correct
 * declared type for a floating point parameter regardless of how that
 * parameter is actually passed.  Note that this must be "turned on" in
 * PrintType by removing some comment delimiters when the C and C++ compilers
 * generate this new type information.
 * 
 * Revision 66.23  90/07/26  09:53:53  09:53:53  bruno (Bruno Melli)
 * Added a new define for pxdb version number
 * 
 * Revision 66.22  90/06/08  16:20:48  16:20:48  markm (Mark McDowell)
 * The difference between "int" and "long" is now recognized.
 * 
 * Revision 66.21  90/03/14  16:23:59  16:23:59  markm (Mark McDowell)
 * Added "functions" and "files" fields to pxdb header.
 * 
 * Revision 66.20  89/12/22  09:24:37  09:24:37  markm (Mark McDowell)
 * Made the PDS's language field visible to non-C++ debuggers.
 * 
 * Revision 66.19  89/12/15  15:58:14  15:58:14  markm (Mark McDowell)
 * Added "classflag" bits to DNTT_BEGIN and DNTT_END to handle local class
 * definitions.  Added K_MEMFUNC kind to handle "dangling" DNTT_FUNC
 * definitions created in class definitions.
 * 
 * Revision 66.18  89/12/08  11:45:23  11:45:23  markm (Mark McDowell)
 * Added "segoffset" field to DNTT_OBJECT_ID for use by the linker.
 * 
 * Revision 66.17  89/11/30  14:37:21  14:37:21  markm (Mark McDowell)
 * Added stub flag to procedure descriptor structure for bodyless functions.
 * 
 * Revision 66.13  89/11/14  09:28:36  09:28:36  markm (Mark McDowell)
 * Add classmem bits to DNTT_CONST and DNTT_MEMENUM.
 * 
 * Revision 66.12  89/11/07  14:50:25  14:50:25  jmn (John Newman)
 * fixed #define of K_MAX to correct value
 * 
 * Revision 66.11  89/11/01  10:43:37  10:43:37  markm (Mark McDowell)
 * The lowscope and hiscope fields of the class descriptor were changed
 * from VTPOINTER's to SLTPOINTER's.
 * 
 * Revision 66.10  89/10/27  10:50:18  10:50:18  markm (Mark McDowell)
 * Add promoted bit to DNTT_INHERITANCE.
 * 
 * Revision 66.9  89/10/23  15:15:22  15:15:22  markm (Mark McDowell)
 * Add inlined bit to PXDB_header.
 * 
 * Revision 66.8  89/10/23  14:38:47  14:38:47  markm (Mark McDowell)
 * Add alternate bit to AAS.
 * 
 * Revision 66.7  89/10/23  14:06:15  14:06:15  markm (Mark McDowell)
 * Add Operator and inlined bits to PDS; lowscope and hiscope fields
 * to CDS.
 * 
 * Revision 66.6  89/10/11  08:23:30  08:23:30  markm (Mark McDowell)
 * Added anonymous union bit to DNTT_GENFIELD.
 * 
 * Revision 66.5  89/10/05  17:24:05  17:24:05  bruno (Bruno Melli)
 * split unused field in PXDB_header into unused and short for
 * version number.
 * 
 * Revision 66.4  89/09/22  16:24:30  16:24:30  jmn (John Newman)
 * Merged in all C++ definitions.
 * Changed semantics for Block Data; now uses K_BLOCKDATA
 * instead of F_ARGMODE_BLKDATA.
 * 
 * Revision 66.3  89/09/06  15:33:30  15:33:30  jww (Jim Wichelman)
 * Add _SYMTAB_INCLUDED to support multiple includes (for s300 ccom).
 * 
 * Revision 66.2  89/08/29  09:39:05  09:39:05  jmn (John Newman)
 * added language field to DNTT_XREF (requested by Schumacher)
 * 
 * Revision 66.1  89/08/21  09:22:07  09:22:07  markm (Mark McDowell)
 * Merged s300 and s800 symtab.h formats.
 * 
 * Revision 7.2  89/04/12  10:21:57  10:21:57  xdb (John Newman)
 * added DNTT_IMPORT.explicit flag (re S. Folkman, SR#4700749770)
 * 
 * Revision 7.1  89/02/28  11:20:18  11:20:18  jmn (John Newman)
 * rearranged RCS header a bit
 * 
 * Revision 7.0  89/02/28  11:13:08  11:13:08  jmn (John Newman)
 * more HPUX 7.0 additions:
 * 	- commentary on T_WIDE_CHAR basetype
 * 	- new fields in DNTT_FUNC: varargs, info
 * 	- values for DNTT_FUNC.info field (relating to C fparams); commentary
 * 
 * Revision 2.1  88/11/03  11:08:40  11:08:40  jmn (John Newman)
 * new basetype: T_WIDE_CHAR
 * 
 * Revision 2.0  88/10/24  11:00:34  11:00:34  jmn (John Newman)
 * Initial (RCS) revision
 *
 *****************************************************************
 * Initial (pre-RCS) revision: 86/12/17
 *****************************************************************
 * Statis Analysis:
 *   - Revised to include new XREF information for 
 *     Excalibur Static Analysis support.  - mcgrory, ras (7/25/88)
 *   - Revised Static Analysis info including new INFO2 record
 *     in XT table, the addition of the optimize field in the 
 *     DNTT_FUNC record, the definition of K_MAX, and the removal
 *     of the definition of uint.  - sjl (8/8/88)
 *   - Moved definitions of the quick look-up table array elements
 *     into symtab.h.  Added defns for K_SA, and K_MACRO dntts - sjl (8/23/88)
 *   - Added #if[n]def NO_SA to provide backward compatability with non-SA
 *     compilers. - jmn (9/23/88)
 *   - Added extension_header flag to XDB_header - jmn (10/1/88)
 *   - Added sa_header flag to PXDB_header - jmn (10/1/88)
 *****************************************************************
 * 
 */

/*
 *
 *                      SYMBOLIC DEBUG FORMAT ACD
 *                           $Revision: 70.6 $
 *
 *
 *
 * ---- 1.  INTRODUCTION
 *
 *
 *      This document describes the current format for data tables which
 *      appear in HP-UX / HPE object files (a.out files).  These tables
 *      will be generated by the compilers, fixed up by the linker, and
 *      used by various programs (primarily the symbolic debugger(s)) to
 *      reconstruct information about the program.  The form of this
 *      document is a C include file annotated with comments.
 *
 *      On SPECTRUM, a major goal was that the linker need not know
 *      anything about the format.  To this end, it was decided that the
 *      debug information be composed of several unloadable subspaces
 *      within an unloadable space (named $DEBUG$), and that link time
 *      updates to the debug information be made through the standard
 *      mechanism of a list of fixups.  The linker will perform the
 *      required fixups for the debug spaces, and subspaces from
 *      separate compilation units will be concatenated.  However, at
 *      exec time, the loader would know that the debug space is not to
 *      be loaded.
 *
 *      Similarly, on the series 300, several debug tables are present
 *      in the a.out format which are not loaded at exec time.  Debug
 *      tables are simply concatenated into larger tables at link time
 *      and all fixups are then performed by pxdb.
 */

/*
 * ---- 2.  SUMMARY OF STRUCTURES
 *
 *
 *      The debug information consists of six tables:  a header table
 *      and five special tables.  The header table will contain one
 *      header record for each compilation unit.  Each header record
 *      identifies the size (in bytes) of the five tables generated by
 *      that compilation unit.  Two of the tables are very similar.  The
 *      GNTT and LNTT both contain name and type information (NTT for
 *      Name and Type Table).  The GNTT contains information about
 *      globals, and is thus limited to variables, types, and constants.
 *      The LNTT is for information about locals.  The LNTT must
 *      therefore contain scoping information such as procedure nesting,
 *      begin-end blocks, etc.  The GNTT and LNTT are both DNTTs (Debug
 *      Name and Type Tables), so the prefix DNTT is attached to objects
 *      (like a DNTTPOINTER) that are relevant to both the GNTT and
 *      LNTT.  The SLT contains information relating source (or listing)
 *      lines to code addresses.  The SLT and LNTT contain pointers
 *      between the two tables, so that the scoping information
 *      contained in the LNTT can also be used with the SLT.  The VT
 *      contains ascii strings (such as variable names) and the values
 *      of named constants.  The five tables are summarized below:
 *
 *  
 *      Table           Abbr  Contains                   Points into  
 *      =============   ====  =========================  ===============
 *      Global symbols  GNTT  global name-and-type info  GNTT
 *      Local symbols   LNTT  local name-and-type info   GNTT,LNTT,SLT,VT
 *      source line     SLT   source/listing line info   LNTT,SLT
 *      value           VT    names and constants        -
 *      xref            XT    File offsets and Attributes XT,VT
 *
 *
 *      The pointers needed within the debug tables are in fact indexes
 *      into the tables.  The GNTT, LNTT, and SLT each consist of a series
 *      of equal-sized entries.  Some DNTT entries begin a data structure
 *      and some are extension entries.  Some SLT entries are "special"
 *      (point back to the LNTT), others are "assist" (point forward in 
 *      the SLT), but most are "normal" (point to code).
 *
 *      There can be pointers from the LNTT to the GNTT, as it is common
 *      to have local variables of a global type.  However, there are
 *      never pointers from the GNTT to the LNTT, as global variables
 *      are never of a local type.
 *
 *      The tables are defined to be as machine-independent as possible,
 *      but the debugger may need to "know" some facts about the system 
 *      and language it is dealing with.
 *
 *      The GNTT and LNTT are the only tables that require fixups to be
 *      generated by the compiler and acted upon by the linker.  There
 *      are other fixups to be done, but these are all done by the pre-
 *      processor.
 */

/*
 * ---- 3.  LOW-LEVEL TYPE DECLARATIONS
 */             

/*
 * Code or data address:
 *
 *      For the series 300:
 *
 *        A virtual Address 
 *
 *      For Spectrum:
 *
 *        A Spectrum short pointer.
 *
 */

#if __cplusplus
#define public global
#endif

typedef long          ADDRESS;
typedef unsigned long ADRT, *pADRT;

/*
 * Language types:
 * 
 *      Sizeof  (LANGTYPE)  = 4  bits,  for  a  maximum  of 16  possible
 *      language types.
 */

typedef unsigned int LANGTYPE;

#define LANG_UNKNOWN    0
#define LANG_C          1
#define LANG_HPF77      2
#define LANG_HPPASCAL   3
#define LANG_HPMODCAL   4
#define LANG_HPCOBOL    5
#define LANG_HPBASIC    6
#define LANG_HPADA      7
#ifdef CPLUSPLUS
#define LANG_CPLUSPLUS  8
#endif


/*
 * Location types:
 *
 *      32-bit,  machine-dependent and  context-dependent  specifiers of
 *      variable storage location.
 */

typedef unsigned long STATTYPE;         /* static-type location         */
typedef          long DYNTYPE;          /* dynamic-type location        */
typedef unsigned long REGTYPE;          /* register-type location       */

#define STATNIL (-1)                    /* no location for STATTYPE     */

/*
 *      Loc type     Series 300           Spectrum
 *      ========  ================     ===============
 *
 *      STATTYPE  Absolute address     A Spectrum
 *                into process         short pointer.
 *                space (could be    
 *                code or data).    
 *                                 
 *
 *      DYNTYPE   A6-register-         SP-register
 *                relative byte        relative byte
 *                offset (+/-).        offset (+/-)
 *
 *      REGTYPE   Register number      Register number
 *                (see below).         (see below).
 *
 *      All location types are always byte (not word) pointers when they
 *      address memory, and they always point to the first byte
 *      containing the object, skipping any padding bytes.  For example,
 *      if in Pascal a CHAR is allocated in the last byte of a whole
 *      word, the pointer is to that byte.  (In C, four different CHAR
 *      variables might be packed into one word.)
 */

/*
 * Meaning of STATTYPE for CONST entries:
 *
 *      Sizeof  (LOCDESCTYPE)  = 3 bits,  for a  maximum  of 8  possible
 *      desctypes.
 */

typedef unsigned int LOCDESCTYPE;

#define LOC_IMMED       0       /* immediate constant                   */
#define LOC_PTR         1       /* standard STATTYPE                    */
#define LOC_VT          2       /* value table byte offset              */

/*
 * Register numbers for REGTYPE (Series 300 only):
 */

#define REG_D0   0
#define REG_D1   1
#define REG_D2   2
#define REG_D3   3
#define REG_D4   4
#define REG_D5   5
#define REG_D6   6
#define REG_D7   7

#define REG_A0   8
#define REG_A1   9
#define REG_A2  10
#define REG_A3  11
#define REG_A4  12
#define REG_A5  13
#define REG_A6  14
#define REG_A7  15

#define	REG_FP0	16
#define	REG_FP1	17
#define	REG_FP2	18
#define	REG_FP3	19
#define	REG_FP4	20
#define	REG_FP5	21
#define	REG_FP6	22
#define	REG_FP7	23

#define	REG_FPA0 24
#define	REG_FPA1 25
#define	REG_FPA2 26
#define	REG_FPA3 27
#define	REG_FPA4 28
#define	REG_FPA5 29
#define	REG_FPA6 30
#define	REG_FPA7 31
#define	REG_FPA8 32
#define	REG_FPA9 33
#define	REG_FPA10 34
#define	REG_FPA11 35
#define	REG_FPA12 36
#define	REG_FPA13 37
#define	REG_FPA14 38
#define	REG_FPA15 39

/* 
 * generic floating point registers; 
 * actual register determined at runtime
 */

#define	REG_FGEN0 40
#define	REG_FGEN1 41
#define	REG_FGEN2 42
#define	REG_FGEN3 43
#define	REG_FGEN4 44
#define	REG_FGEN5 45
#define	REG_FGEN6 46
#define	REG_FGEN7 47
#define	REG_FGEN8 48
#define	REG_FGEN9 49
#define	REG_FGEN10 50
#define	REG_FGEN11 51
#define	REG_FGEN12 52
#define	REG_FGEN13 53
#define	REG_FGEN14 54
#define	REG_FGEN15 55

/*
 * Basetypes:
 *
 *      Sizeof  (BASETYPE)  = 5  bits,  for  a  maximum  of 32  possible
 *      basetypes.
 */

typedef unsigned int BASETYPE;

#define T_UNDEFINED        0    /* unheard of                    */
#define T_BOOLEAN          1    /* true/false or LOGICAL         */
#define T_CHAR             2    /* ASCII, signed if used as int  */
#define T_INT              3    /* signed integer                */
#define T_UNS_INT          4    /* unsigned integer              */
#define T_REAL             5    /* binary or decimal real        */
#define T_COMPLEX          6    /* pair of reals                 */
#define T_STRING200        7    /* Series 300 string type        */
#define T_LONGSTRING200    8    /* Series 300 long string type   */
#define T_TEXT             9    /* for Pascal TEXT file          */
#define T_FLABEL          10    /* for any program labels        */
#define T_FTN_STRING_SPEC 11    /* Spectrum FORTRAN string type  */
#define T_MOD_STRING_SPEC 12    /* Spectrum Modcal/Pascal string */
#define T_PACKED_DECIMAL  13    /* packed decimal                */
#define T_REAL_3000       14    /* HP3000 format real            */
#define T_MOD_STRING_3000 15    /* HP3000 Modcal/Pascal string   */
#define T_ANYPTR          16    /* Pascal any-pointer            */
#define T_GLOBAL_ANYPTR   17    /* Pascal global any-pointer     */
#define T_LOCAL_ANYPTR    18    /* Pascal local any-pointer      */
#define T_COMPLEXS3000    19    /* HP3000 format complex         */
#define T_FTN_STRING_S300_COMPAT 20 /* 9000/s300 compatible fortran string  */
#define T_FTN_STRING_VAX_COMPAT  21 /* VAX compatible fortran string        */
#define T_BOOLEAN_S300_COMPAT    22 /* 9000/s300 compatible fortran logical */
#define T_BOOLEAN_VAX_COMPAT     23 /* VAX compatible fortran logical       */
#define T_WIDE_CHAR       24    /* ANSI/C wchar_t pseudo-type */
#define T_LONG            25    /* signed long                   */
#define T_UNS_LONG        26    /* unsigned long                 */
#define T_DOUBLE          27    /* binary or decimal double      */
#ifdef TEMPLATES
#define T_TEMPLATE_ARG	  28	/* template argument immediate type */
#endif /* TEMPLATES */

/* THE HIGHEST BASE_TYPE ALLOWABLE is 31 (see DNTTP_IMMEDIATE) */
/*
 *      The string  types are reserved  for cases where the language has
 *      an explicit string type separate from "array of char".  
 *	
 *	The ANSI/C wchar_t typedef defines a special base-type to
 *	the debugger.  The interpretation of wide-characters during
 *	input or display (i.e.  their mapping to/from "external"
 *	characters) is defined by the ANSI/C functions mbtowc() and
 *	wctomb(), the "multi-byte" translation functions.
 *	
 *      T_FLABEL is used for CONSTs which are actually FORTRAN labels.
 *      The T_FLABEL is needed for the following:  in FORTRAN there is
 *      the ASSIGN statement (ASSIGN <label> TO <integer variable>),
 *      which places the address of the statement prefixed with the
 *      label <label> into the integer variable.  This integer variable
 *      can then be used as a label (e.g. GOTO <integer variable>).
 *      The user may wish to display the contents of the integer variable
 *      as a label.  The DNTT LABEL entry is not sufficient, as the label
 *      need not be on an executable statement (e.g. a FORMAT statement),
 *      and the DNTT LABEL can only be used with executable statements.
 *
 *      The  bitlength in a DNTT entry further  qualifies the  basetype.
 *      Here is a summary of the legal  values  for  bitlength.  See the
 *      appropriate sections below for details.
 *
 *      T_UNDEFINED     any     probably treat as int
 *      T_BOOLEAN       1       one-bit value
 *                      16,32   FORTRAN LOGICAL
 *      T_CHAR          1..8    size of char (really can be < 8 bits in C)
 *      T_INT           2..n    probably n <= 64; incl. sign bit
 *      T_UNS_INT       1..n    probably n <= 64
 *      T_REAL          32      short binary
 *                      64      long binary
 *                      128     extended real
 *      T_COMPLEX       64      two short binaries
 *                      128     two long binaries
 *                      192     two decimals
 *      T_STRING200     n * 8   maximum allocated memory, including
 *                              length byte and/or terminator byte
 *      T_FTN_STRING_SPEC       (to be determined)
 *      T_MOD_STRING_SPEC       (to be determined)
 *      T_TEXT          n       size of the element buffer only
 *      T_FLABEL        n * 8   size of the format label
 *      T_PACKED_DECIMAL        (to be determined)
 *	T_WIDE_CHAR     32	determined by HP's NLS/Ansi-C committees
 */

typedef unsigned int BITS;

/*
 * DNTT pointer:
 */

struct  DNTTP_IMMEDIATE {
        BITS        extension:  1;          /* always set to 1      */
        BITS        immediate:  1;          /* always set to 1      */
        BITS        global:     1;          /* always set to 0      */
        BASETYPE    type:       5;          /* immediate basetype   */
        BITS        bitlength: 24;          /* immediate bitlength  */
};

/*
 * Note that for type T_TEMPLATE_ARG bitlength is an positioning
 * index into the chain of DNTT_TEMPLATE_ARG hanging out of
 * the DNTT_TEMPLATE arglist field.
 */

struct  DNTTP_NONIMMED {
        BITS        extension:  1;          /* always set to 1      */
        BITS        immediate:  1;          /* always set to 0      */
        BITS        global:     1;          /* 1 => GNTT, 0 => LNTT */
        BITS        index:     29;          /* DNTT table index     */
};

typedef union {
        struct  DNTTP_IMMEDIATE dntti;
        struct  DNTTP_NONIMMED  dnttp;
        long                    word;           /* for generic access   */
} DNTTPOINTER;                                  /* one word             */

#define DNTTNIL (-1)

/*
 *      A  DNTTPOINTER  of DNTTNIL means a nil  pointer.  In the DNTT
 *      immediate case there is always at least one zero bit (the global
 *      bit) to distinguish that case from nil pointer  (-1).  In  the
 *      non-immediate, non-nil case DNTTPOINTER is the block index, base
 *      zero, of another DNTT entry; the global bit indicates which table
 *      it is an index into, the GNTT or LNTT.  Each block is 12 bytes.
 *
 *      Extension bits really have nothing to do with DNTT pointers, but
 *      are needed for constructing the DNTT.  See the next section.
 *
 *      Bitlength is the MINIMUM  (packed)  size of the object.  In lieu
 *      of other  information  (i.e.,  outside of a structure or array),
 *      the  object is  assumed  to be  right-justified  in the  minimum
 *      number  of  whole  bytes  required  to hold  the  bitlength.  An
 *      immediate  DNTTPOINTER  is only  allowed if the type is a simple
 *      BASETYPE.  Otherwise, a separate DNTT entry must be used.
 */


/*
 * SLT pointer:
 *
 *      Signed  entry  index,  base zero, into the source line table.
 *      Each entry is eight bytes.
 */

typedef long SLTPOINTER;

#define SLTNIL (-1)


/*
 * VT pointer:
 *
 *      Unsigned byte offset into the value table.  Note that VTNIL
 *      is not actually a nil pointer, but rather a pointer to a nil
 *      string (see section 6).
 */

typedef long VTPOINTER;

#define VTNIL 0


/*
 * XREF pointer:
 *
 *      Signed  entry  index,  base zero, into the cross reference table.
 *      Each entry is four bytes.
 */

typedef long XREFPOINTER;

#define XREFNIL (-1)


/*
 * Values for "declaration" fields describing packing method
 */

#define DECLNORMAL   0
#define DECLPACKED   1
#define DECLCRUNCHED 2


/*
 * ---- 4.  DEBUG HEADER
 */

/*
 *     The header table is composed of five word header records.  For
 *     each compilation unit, the compiler must generate a header
 *     record, indicating the length (in bytes) of the five tables
 *     (GNTT, LNTT, SLT, VT and XT) produced for that compilation unit.
 */

              struct XDB_header {
                 long gntt_length; 
                 long lntt_length; 
                 long slt_length; 
                 long vt_length; 
                 long xt_length; 
              };
  
#define  extension_header  0x80000000

/*
 *      The purpose of the header record is as follows:  the five tables
 *      are each contained in a separate subspace on SPECTRUM or in a
 *      separate section of the a.out file on the series 300.  Therefore
 *      at link time, the tables from different compilation units will
 *      be con- catenated separately, GNTTs to GNTTS, SLTs to SLTs, etc.
 *      However, the preprocessor requires the number of compilation
 *      units, and the size of each of the five tables produced by each
 *      compilation unit.  The header records supply this size
 *      information, and the number of header records equals the number
 *      of compilation units.
 *
 *      For SPECTRUM, the header_extension flag (MSB) is set in the
 *      gntt_length word in each header-record by the HP-UX 3.1+ s800 C
 *      compiler to indicate the header contains an xt_length and is 5
 *      words long.  This bit is used to distinguish SOM's that were
 *      created with the pre-SA compiler (HP-UX 3.0, /bin/cc vers.
 *      A.00.15 or earlier) from SOM's that contain an $XT$ subspace.
 *
 *      For SPECTRUM, pxdb and xdb version A.02.xx can be used on
 *      >>all<< SOM's (4 or 5 word XDB headers) that have not already
 *      been pxdb'd.  Earlier versions of either are completely
 *      incompatible with SOM's containing an $XT$ (HP-UXS 3.1 or later)
 *      because of the header-length.
 *
 *      For the series 300, the header_extension flag is not used (i.e.
 *      the gntt_length occupies a full 32 bits).
 */

/*
 * ---- 5.  DEBUG SYMBOL TABLE (DNTT) ENTRY FORMAT
 */

/*
 *      The DNTT consists of a series of three-word  blocks.  Each block
 *      starts with an  "extension  bit".  Each  structure  in the union
 *      "dnttentry"  begins in an  "initial  block"  with a bit which is
 *      always  zero.  If a  structure  is more than  three  words  (one
 *      block)  long,  it  occupies  one or more  additional  "extension
 *      blocks",  each  of  which  starts  with  a bit  set  to  one  to
 *      distinguish it from an initial block.
 *
 *      Note well that every  DNTTPOINTER has a high bit of one and that
 *      every DNTT structure bigger than one block is carefully arranged
 *      so that a DNTTPOINTER  resides in the fourth and seventh  words.
 *      (The extension bit is in the  DNTTPOINTER to avoid wasting space
 *      due to structure packing rules.)
 */

#define DNTTBLOCKSIZE   12

/*      The second field in each  structure is "kind", which acts like a
 *      Pascal  variant  tag to denote  the type of the  structure.  The
 *      "unused"  fields are just included for clarity.  The whole union
 *      "dnttentry" is declared after the definition of KINDTYPE and all
 *      the various structures (below).
 */

typedef int KINDTYPE;

#define K_NIL          (-1)    /* guaranteed illegal value */

#define K_SRCFILE        0

#define K_MODULE         1
#define K_FUNCTION       2
#define K_ENTRY          3
#define K_BEGIN          4
#define K_END            5
#define K_IMPORT         6
#define K_LABEL          7
#define K_WITH          27
#define K_COMMON        28

#define K_FPARAM         8
#define K_SVAR           9
#define K_DVAR          10
#define K_CONST         12

#define K_TYPEDEF       13
#define K_TAGDEF        14
#define K_POINTER       15
#define K_ENUM          16
#define K_MEMENUM       17
#define K_SET           18
#define K_SUBRANGE      19
#define K_ARRAY         20
#define K_STRUCT        21
#define K_UNION         22
#define K_FIELD         23
#define K_VARIANT       24
#define K_FILE          25
#define K_FUNCTYPE      26
#define K_COBSTRUCT     29

#define K_XREF          30
#define K_SA            31
#define K_MACRO         32
#define K_BLOCKDATA     33

#define K_MODIFIER      45	/* used for C too so we can qualify type */

#ifdef CPLUSPLUS
#define K_CLASS_SCOPE   34
#define K_REFERENCE     35
#define K_PTRMEM        36
#define K_PTRMEMFUNC    37
#define K_CLASS		38
#define K_GENFIELD      39
#define K_VFUNC         40
#define K_MEMACCESS     41
#define K_INHERITANCE   42
#define K_FRIEND_CLASS  43
#define K_FRIEND_FUNC   44
#define K_OBJECT_ID     46
#define K_MEMFUNC       47
#ifdef TEMPLATES
#define K_TEMPLATE	48
#define K_TEMPL_ARG	49
#define K_FUNC_TEMPLATE	50
#define K_LINK		51
#endif /* TEMPLATES */
#endif

#ifdef CPLUSPLUS
#ifdef TEMPLATES
#define K_MAX		K_LINK
#else /* TEMPLATES */
#define K_MAX           K_MEMFUNC
#endif /* TEMPLATES */
#else
#define K_MAX           K_BLOCKDATA
#endif

/*
 * ---- 5.1.  FILE-CLASS ("FILE") DNTT ENTRIES
 */


struct  DNTT_SRCFILE {
/*0*/   BITS          extension: 1;   /* always zero                  */
        KINDTYPE      kind:     10;   /* always K_SRCFILE             */
        LANGTYPE      language:  4;   /* type of language             */
        BITS          unused:   17;
/*1*/   VTPOINTER     name;           /* name of source/listing file  */
/*2*/   SLTPOINTER    address;        /* code and text locations      */
};                                    /* three words                  */

/*
 *      One  SRCFILE is emitted  for the start of each  source  file, the
 *      start of each  included  file, and the return from each  included
 *      file.  Additional SRCFILE entries must also be output before each
 *      DNTT_FUNC entry.  This guarantees the debuggers know which file a
 *      function  came  from.  Specifically, the rules are as follows:
 * 
 *	Definitions:
 *	  Source block:  contiguous block of one or more lines of text in a
 *	      source-file,  bounded by beginning or end-of-file  or include
 *	      directives  (conceptually  identical to the "basic  block" in
 *	      optimizer  jargon).  No  distinction  is made between  blocks
 *	      that contain compilable code and those that don't.
 *
 *        Code  segment:  contiguous  LINEAR  block of DNTT (and associated
 *            SLT) entries that are generated from the same "source block".
 *	      "SLT_SRC" is used here to actually refer to an SLT_SPEC entry
 *	      of type SLT_SRCFILE.  Same goes for SLT_FUNC.
 *
 *	1. One DNTT_SRCFILE and SLT_SRC must be emitted at the head of each
 *	   code segment to facilitate reading backwards through the DNTT or
 *	   SLT  tables  from any  point in the  segment  to  determine  the
 *	   enclosing  source file.  If the  source-file  changes within the
 *	   body of a function/subprogram, a DNTT_SRCFILE/SLT_SRC  pair must
 *	   be emitted prior to any additional DNTT or SLT entries generated
 *	   by the remainder of that function/subprogram.
 *
 *      2. One DNTT_SRCFILE/SLT_SRC  pair is always emitted *immediately*
 *         before  any   DNTT_FUNC/SLT_FUNC.  Exception:  a  DNTT_SA  and
 *         associated  DNTT_XREF may appear  between a DNTT_FUNC and it's
 *         preceding  DNTT_SRCFILE.  There  can be  nothing  between  the
 *         SLT_SRC and the  SLT_FUNC.  The  DNTT_SRCFILE  (preceding  the
 *         DNTT_FUNC)  must  name  the  file   containing  the  functions
 *         declaration.  The SLT_FUNC must contain the line number of the
 *         line in the function's  declaration  where the function's name
 *         appears.  This line  number  must match the line  number  that
 *         appears in the XT record denoting the function's  declaration.
 *         The SLT_END associated with the SLT_FUNC must contain the line
 *         number of the source line containing the  scope-closing  token
 *         (i.e.  "}" or "end").
 *
 *	3. One DNTT_SRCFILE/SLT_SRC  pair must be emitted for a source file
 *	   that  otherwise  would not be mentioned in the DNTT i.e.  source
 *	   files that do not generate a code segment.  This is required for
 *	   Static analysis only.
 *
 *
 *      "address"  points to a special  SLT  entry  (for the line  number
 *      only), but the code location is known from context in the SLT.  *
 *
 *      NOTE: Listing files and listing file line numbers may be used in
 *            place of source files and source file line numbers.  A
 *            special compiler option will designate which is generated
 *            by the compiler.
 *
 *      SRCFILE names are exactly as seen by the  compiler,  i.e.  they
 *      may be relative,  absolute, or whatever.  C include  file names
 *      must be given as absolute  paths if found "in the usual  place",
 *      i.e., /usr/include/...
 */

/*
 * ---- 5.2.  CODE-CLASS ("SCOPING") DNTT ENTRIES
 */


struct  DNTT_MODULE {
/*0*/   BITS          extension: 1;   /* always zero                  */
        KINDTYPE      kind:     10;   /* always K_MODULE              */
        BITS          unused:   21;
/*1*/   VTPOINTER     name;           /* name of module               */
/*2*/   VTPOINTER     alias;          /* alternate name, if any       */
/*3*/   DNTTPOINTER   dummy;          /* 4th word must be DNTTPOINTER */
/*4*/   SLTPOINTER    address;        /* code and text locations      */
};                                    /* five words                   */

/*
 *      One MODULE is emitted for the start of each Pascal/Modcal module
 *      or C source file (C sources are considered a nameless module).
 *      "address" points to a special SLT entry, but the code location 
 *      is known from context in the SLT.
 *
 *      In the case of languages that do not support modules (such as
 *      FORTRAN) a DNTT_MODULE and DNTT_END pair are not used. Every
 *      MODULE must have a matching END (see  below).  If a Pascal/Modcal
 *      module has a module body (some code), the latter must be represented
 *      by a FUNCTION-END pair as well (see below).
 *
 *      For items within a module, the public bit is true if that item
 *      is exported by the module.  If the public bit of an item is set,
 *      that item is visible within any module or procedure that imports
 *      the module containing the item.  If the public bit of an item
 *      is not set, then the item is only visible within the module.
 *
 *      The "dummy" field exists only because the first word of each
 *      extension block must be a DNTTPOINTER; it is important only
 *      that the extension bit of the DNTTPOINTER be set.
 *
 *      The MODULE DNTT should be used only in the LNTT.
 */

#ifdef TEMPLATES

struct	DNTT_LINK
{
/*0*/   BITS           extension: 1;   /* always zero                  */
	KINDTYPE       kind:     10;   /* always K_LINK                */
	BITS           linkKind:  4;   /* always LINK_UNKNOWN	       */
	BITS           unused:   17;
/*1*/   long           future1;        /* expansion                    */
/*2*/   DNTTPOINTER    ptr1;	       /* link from template           */
/*3*/   DNTTPOINTER    ptr2;	       /* to expansion                 */
/*4*/   long           future[2];
};

#if 1
struct	DNTT_TFUNC_LINK
{
/*0*/   BITS           extension: 1;   /* always zero                  */
	KINDTYPE       kind:     10;   /* always K_LINK                */
	BITS           linkKind:  4;   /* always LINK_FUNC_TEMPLATE    */
	BITS           unused:   17;
/*1*/   long           args;           /* expansion                    */
/*2*/   DNTTPOINTER    pTemplate;      /* link from template           */
/*3*/   DNTTPOINTER    pExpansion;     /* to expansion                 */
/*4*/   long           future[2];
};
#endif /* 0 */   
/* temporary until we get a new cfront */
#if 0
struct	DNTT_TFUNC_LINK
{
/*0*/   BITS           extension: 1;   /* always zero                  */
	KINDTYPE       kind:     10;   /* always K_LINK                */
	BITS           linkKind:  4;   /* always LINK_FUNC_TEMPLATE    */
	BITS           unused:   17;
/*2*/   DNTTPOINTER    pTemplate;      /* link from template           */
/*3*/   DNTTPOINTER    pExpansion;     /* to expansion                 */
/*1*/   long           args;           /* expansion                    */
/*4*/   long           future[2];
};
#endif /* 0 */
/*
 * Note the linkKind bit. The idea is that we might have other
 * LINKs in the future that share the same format but where we would
 * call the fields another name. It's hard to debug a program
 * where fields are called link_word1 and link_word2.
 */

#define LINK_UNKNOWN	        0
#define LINK_FUNC_TEMPLATE	1

struct  DNTT_FUNC_TEMPLATE {
/*0*/   BITS           extension: 1;   /* always zero                  */
        KINDTYPE       kind:     10;   /* K_FUNC_TEMPLATE              */
        BITS           public:    1;   /* 1 => globally visible        */
        LANGTYPE       language:  4;   /* type of language             */
        BITS           level:     5;   /* nesting level (top level = 0)*/
	BITS           optimize:  2;   /* level of optimization        */
	BITS           varargs:   1;   /* ellipses.  Pascal/800 later  */
	BITS           info:   	  4;   /* lang-specific stuff; F_xxxx  */
#ifdef CPLUSPLUS
	BITS           inlined:	  1;
	BITS           localloc:  1;   /* 0 at top, 1 at end of block  */
	BITS           unused:    2;
#else
	BITS           unused:    4;
#endif
/*1*/   VTPOINTER      name;           /* name of function             */
/*2*/   VTPOINTER      alias;          /* alternate name, if any       */
/*3*/   DNTTPOINTER    firstparam;     /* first FPARAM, if any         */
/*4*/   DNTTPOINTER    retval;         /* return type, if any          */
/*5*/	DNTTPOINTER    arglist;        /* ptr to argument list	       */
};                                     /* nine  words                  */

/*
 * DNTT_FUNC_TEMPLATEs only appear in the GNTT. Functions and
 * classes templates cannot be local. (Their instantions may be).
 */
#endif /* TEMPLATES */

struct  DNTT_FUNC {
/*0*/   BITS           extension: 1;   /* always zero                  */
        KINDTYPE       kind:     10;   /* K_FUNCTION, K_ENTRY,         */
				       /* K_BLOCKDATA, or K_MEMFUNC    */
        BITS           public:    1;   /* 1 => globally visible        */
        LANGTYPE       language:  4;   /* type of language             */
        BITS           level:     5;   /* nesting level (top level = 0)*/
	BITS           optimize:  2;   /* level of optimization        */
	BITS           varargs:   1;   /* ellipses.  Pascal/800 later  */
	BITS           info:   	  4;   /* lang-specific stuff; F_xxxx  */
#ifdef CPLUSPLUS
	BITS           inlined:	  1;
	BITS           localloc:  1;   /* 0 at top, 1 at end of block  */
#ifdef TEMPLATES
	BITS	       expansion: 1;   /* 1 = function expansion       */
	BITS	       unused:	  1;
#else /* TEMPLATES */
	BITS           unused:    2;
#endif /* TEMPLATES */
#else
	BITS           unused:    4;
#endif
/*1*/   VTPOINTER      name;           /* name of function             */
/*2*/   VTPOINTER      alias;          /* alternate name, if any       */
/*3*/   DNTTPOINTER    firstparam;     /* first FPARAM, if any         */
/*4*/   SLTPOINTER     address;        /* code and text locations      */
/*5*/   ADDRESS        entryaddr;      /* address of entry point       */
/*6*/   DNTTPOINTER    retval;         /* return type, if any          */
/*7*/   ADDRESS        lowaddr;        /* lowest address of function   */
/*8*/   ADDRESS        hiaddr;         /* highest address of function  */
};                                     /* nine words                   */

/*
 * Additional function semantics: Values for DNTT_FUNC.info
*/

					/* In command-line C proc-call...     */
#define F_ARGMODE_COMPAT_C	0	/* all real params passed as double   */
#define F_ARGMODE_ANSI_C	1	/* floats-is-floats but PASS as dbl   */
#define F_ARGMODE_ANSI_C_PROTO	2	/* all real params passed as declared */

					/* special DNTT_FUNC semantics        */
#define F_ARGMODE_BLKDATA	3	/* Fortran "block data" construct     */
					/* NOT A FUNCTION!                    */
	/* F_ARGMODE_BLKDATA is retained for backward compatability only */

#ifdef CPLUSPLUS
#define CPP_OVERLOADED        0x1       /* overloaded function         */
#define CPP_MEMBERFUNC        0x2       /* member function             */
#define CPP_INLINE            0x4       /* inline function             */
#define CPP_OPERATOR          0x8       /* operator function           */
#endif

/*
 *      Struct  DNTT_FUNC is used for dfunc and dentry, and dblockdata  types.
 *      One FUNCTION  or  ENTRY  is  emitted   for  each   formal   function
 *      declaration   (with  a   body)   or   secondary   entry   point,
 *      respectively.  They are not emitted  for  bodyless  declarations
 *      (FORWARD, EXTERNAL, "int x ();" etc.).  A dblockdata is emitted for
 *      Fortran BLOCK DATA constructs only.
 *
 *      "address" always points to a special SLT entry.
 *
 *      For FUNCTION types, the "entryaddr" field is the code address of
 *      the primary entry point of the function.  The "lowaddr" field is
 *      the lowest code address of the function.  The "hiaddr" field
 *      is the highest code address of the function.  This both gives
 *      the size of the  function  and helps in mapping code locations 
 *      to functions when there are anonymous (non-debuggable) functions
 *      present.  These three fields should be filled in by the generation
 *      of fixups.
 *
 *      For ENTRY types, the "entryaddr" field points to the proper code
 *      location for calling the function at the  secondary  entrypoint,
 *      and the "lowaddr" and "hiaddr" fields are nil (zero).  For a
 *      FORTRAN subroutine with alternate entries, DNTT_DVARs are required
 *      to represent the parameters, see the DNTT_FPARAM definition for
 *      the details.
 *
 *      For BLOCKDATA types, the "public" bit should be set to 1, the
 *      "level", "optimize", "varargs" and "info" fields should all be 0.
 *      The "firstparam" field should be DNTTNIL.  The "entryaddr" and
 *      "lowaddr" fields should be 0, and the "highaddr" field should be
 *      FFFFFFFC (-4).  The "retval" field should be set to T_UNDEFINED,
 *      with length 0.  An SLT_FUNCTION/SNT_END pair should be emitted
 *      for each DNTT_FUNC (BLOCKDATA).
 *
 *      Every FUNCTION or BLOCKDATA must have a matching END (see below).
 *
 *      For languages in which a functions return value is set by assigning
 *      the value to the function name (such as FORTRAN & Pascal), a DVAR
 *      entry should also be emitted for the function.  The address of this
 *      DVAR for the function should be the address of the answer spot for
 *      the function.  This will allow the user to display the current 
 *      return value while the function is executing.
 *
 *	The "varargs" field indicates whether the function was declared as
 *	having a variable-length parameter list.  This is currently possible
 *	only via ANSI/C function-prototype "ellipses" (...).  The "info" field
 *	provides additional language-specific characteristics of the function
 *	and/or its parameter-list.
 *	
 *	The localloc (local variables location) is currently only used
 *	in the following context: If the function
 *	language is LANG_CPLUSPLUS, then 0 means that locals are
 *	at the beginning of the block, and 1 means that locals appears
 *	at the end of a block. For all other languages
 *	this bit is not used.
 *
 *      The FUNCTION DNTT should be used only in the LNTT.
 */


struct  DNTT_BEGIN {
/*0*/   BITS          extension: 1;   /* always zero                  */
        KINDTYPE      kind:     10;   /* always K_BEGIN               */
#ifdef CPLUSPLUS
        BITS          classflag: 1;   /* beginning of class def'n     */
        BITS          unused:   20;
#else
        BITS          unused:   21;
#endif
/*1*/   SLTPOINTER    address;        /* code and text locations      */
};                                    /* two words                    */

/*
 *      BEGINs are emitted as required to open a new (nested)  scope for
 *      any type of  variable or label, at any level  within  MODULE-END
 *      and  FUNCTION-END  pairs.  Every BEGIN must have a matching  END
 *      (see  below).  "address"  points to a special SLT entry, but the
 *      code location is known from context in the SLT.  Because a DNTT
 *      BEGIN-END is used to indicate a new scope, the Pascal BEGIN-
 *      END pair does not produce a DNTT BEGIN-END, while the C { }
 *      construct does.
 *
 *      The BEGIN DNTT should be used only in the LNTT.
 */


struct  DNTT_COMMON {
/*0*/   BITS         extension: 1;   /* always zero                  */
        KINDTYPE     kind:     10;   /* always K_COMMON              */
        BITS         unused:   21;
/*1*/   VTPOINTER    name;           /* name of common block         */
/*2*/   VTPOINTER    alias;          /* alternate name, if any       */
};                                   /* three words                  */

/*
 *      COMMONs are used to indicate that a group of variables are members
 *      of a given FORTRAN common block.  For each common block, a DNTT_
 *      COMMON is emitted, followed by a DNTT_SVAR for each member of the
 *      common block, and finally a DNTT_END.  If type information is
 *      required for a member of the common block (such as an array), it
 *      may also be within the DNTT_COMMON, DNTT_END pair.
 *
 *      The COMMON DNTT should be used only in the LNTT.
 */


struct DNTT_WITH {
/*0*/  BITS           extension: 1;   /* always zero                  */
       KINDTYPE       kind:     10;   /* always K_WITH                */
       BITS           addrtype:  2;   /* 0 => STATTYPE                */
                                      /* 1 => DYNTYPE                 */
                                      /* 2 => REGTYPE                 */ 
       BITS           indirect:  1;   /* 1 => pointer to object       */
       BITS           longaddr:  1;   /* 1 => in long pointer space   */
       BITS           nestlevel: 6;   /* # of nesting levels back     */
       BITS           unused:   11; 
/*1*/  long           location;       /* where stored (allocated)     */
/*2*/  SLTPOINTER     address;     
/*3*/  DNTTPOINTER    type;           /* type of with expression      */
/*4*/  VTPOINTER      name;           /* name of with expression      */
/*5*/  unsigned long  offset;         /* byte offset from location    */
};                                    /* six words                    */

/*
 *      WITHs are emitted to open a with scope.  Like a BEGIN, a
 *      WITH requires a matching END to close the scope.  A single WITH
 *      statement possessing more than one record expression, should
 *      be handled as multiple nested withs with only one expression
 *      each.  The "addrtype" field indicates the addressing mode used
 *      for the record expression, and along with the "indirect" field,
 *      tells how to interpret the "location" and "offset" fields.  Thus,
 *      depending upon the value of "addrtype", "location" may contain
 *      a short pointer, an offset from the local frame pointer, or a 
 *      register number.  If "nestlevel" is non-zero and "addrtype" is
 *      DYNTYPE, the address for the record expression is computed by 
 *      tracing back "nestlevel" static links and using "location" as
 *      an offset from the frame pointer at that level.  (This situation
 *      occurs only on the HP9000 FOCUS architecture.)  The use of the 
 *      "offset" field is the same as for the DNTT_SVAR entry (see below).
 *      The "type" field is the type of the record expression.  The "name"
 *      field is the symbolic representation of the record expression 
 *      (ex. "p[i]^").  "address" points to a special SLT, but the code 
 *      location is known from context in the SLT.
 *
 *      The WITH DNTT should be used only in the LNTT.
 */

struct  DNTT_END {
/*0*/   BITS           extension: 1;   /* always zero                  */
        KINDTYPE       kind:     10;   /* always K_END                 */
        KINDTYPE       endkind:  10;   /* DNTT kind closing scope for  */
#ifdef CPLUSPLUS
        BITS           classflag: 1;   /* end of class def'n           */
        BITS           unused:   10;
#else
        BITS           unused:   11;
#endif
/*1*/   SLTPOINTER     address;        /* code and text locations      */
/*2*/   DNTTPOINTER    beginscope;     /* start of scope               */
};                                     /* three words                  */

/*
 *      ENDs are  emitted  as  required  to close a scope  started  by a
 *      MODULE, FUNCTION, WITH, COMMON, or BEGIN (but not an ENTRY). 
 *      Each points back to the DNTT entry that opened the scope.  
 *      "endkind" indicates which kind of DNTT entry is associated with 
 *      the END and is filled in by the preprocessor.  "address"  points 
 *      to a special SLT entry, but the code location is known from context 
 *      in the SLT.
 *
 *      The END DNTT should be used only in the LNTT.
 */


struct  DNTT_IMPORT {
/*0*/   BITS         extension: 1;   /* always zero                  */
        KINDTYPE     kind:     10;   /* always K_IMPORT              */
	BITS         explicit:  1;   /* module directly imported     */
        BITS         unused:   20;
/*1*/   VTPOINTER    module;         /* module imported from         */
/*2*/   VTPOINTER    item;           /* name of item imported        */
};                                   /* three words                  */

/*
 *      Within a module,  there is one  IMPORT  entry for each  imported
 *      module,  function,  or  variable.  The item field is nil when an
 *      entire  module is  imported.  Used only  by Pascal/Modcal.  Note  
 *      that exported functions and variables have their public bits set.
 *
 *	The "explicit" flag indicates the module was directly imported.
 *      When not set, the module was imported by an imported module.
 *
 *      The IMPORT DNTT should be used only in the LNTT.
 */


struct  DNTT_LABEL {
/*0*/   BITS          extension: 1;   /* always zero                  */
        KINDTYPE      kind:     10;   /* always K_LABEL               */
        BITS          unused:   21;
/*1*/   VTPOINTER     name;           /* name of label                */
/*2*/   SLTPOINTER    address;        /* code and text locations      */
};                                    /* three words                  */

/*
 *      One LABEL is emitted for each source  program  statement  label,
 *      referencing  the  matching  physical  line (SLT  entry).  An SLT
 *      pointer  is  used,  instead  of  just  a  linenumber,  so a code
 *      location  is known for  setting a  breakpoint.  This is the only
 *      case of  SLTPOINTER  that points to a normal (not  special)  SLT
 *      entry.
 *
 *      If a label appears at the very end of a function (after all
 *      executable  code), a normal  SLT entry  must be  emitted  for it
 *      anyway.  In this case the SLT entry  points to an exit  (return)
 *      instruction.
 *
 *      Numeric labels are named as the equivalent character string with
 *      no leading zeroes, except in those languages where the leading
 *      zeroes are significant (i.e. COBOL).
 *
 *      The LABEL DNTT should be used only in the LNTT.
 */


/*
 * ---- 5.3.  STORAGE-CLASS ("NAME") DNTT ENTRIES
 */

struct	 DNTT_FPARAM {
 /*0*/   BITS           extension:   1;   /* always zero                  */
         KINDTYPE       kind:       10;   /* always K_FPARAM              */
         BITS           regparam:    1;   /* 1 => REGTYPE, not DYNTYPE    */
         BITS           indirect:    1;   /* 1 => pass by reference       */
         BITS           longaddr:    1;   /* 1 => in long pointer space   */
         BITS           copyparam:   1;   /* 1 => Copied to a local       */
	 				  /* only for fortran strings     */
#ifdef CPLUSPLUS
	 BITS		dflt:	     1;   /* default parameter value?     */
         BITS           unused:     16;
#else
         BITS           unused:     17;
#endif
 /*1*/   VTPOINTER      name;             /* name of parameter            */
 /*2*/   DYNTYPE        location;         /* where stored                 */
 /*3*/   DNTTPOINTER    type;             /* type information             */
 /*4*/   DNTTPOINTER    nextparam;        /* next FPARAM, if any          */
 /*5*/   int            misc;             /* assorted uses                */
 };                                       /* six words                    */
 
 /*
  *      FPARAMs are chained  together in  parameter  list order (left to
  *      right) from every FUNCTION,  ENTRY, or FUNCTYPE (see below), one
  *      for  each  parameter,  whether  or not the  type  is  explicitly
  *      declared.  For unnamed parameters, the FPARAM name is "*".
  *
  *      "regparam"  implies  that the  storage  location  given is to be
  *      interpreted  as a REGTYPE, not a DYNTYPE, that is, the parameter
  *      was passed in a register.
  *
  *      "indirect"  implies that the storage  location given  contains a
  *      data  pointer  to the  parameter  described,  not the  parameter
  *      itself, due to a call by reference  (Pascal VAR, for  instance).
  *      In the case  where a  call-by-value  parameter  is too big to be
  *      passed in the parameter list (e.g., a copied-value  parameter in
  *      Pascal), the "location" must be given as the actual  (post-copy)
  *      location of the parameter.
  *
  *      "longaddr" is meaningful only for varparams, and indicates that
  *      the storage location given contains a 64 bit Spectrum long
  *      pointer.  The long pointer could be in 2 consecutive words, or
  *      in the case of a regparam, two consecutive registers.
  *
  *      "copyparam" implies that the parameter has been copied to a local,
  *      and thus the location is relative to the sp of the current procedure,
  *      not the sp of the previous procdeure.
  *
  *      "misc" is for assorted values. Current uses are:
  *           (1) if the parameter is of type T_FTN_STRING_S300
  *               then the "misc" field contains the SP relative
  *               offset of the word containing the length of 
  *               the string
  *
  *      In the case of a FORTRAN routine with alternate entries, DNTT
  *      DVARs also must be emited for each parameter. The reason is
  *      that with FORTRAN alternate entries, the same parameter can
  *      be in two different entry's parameter lists, in a different 
  *      location (ex. the parameter "x" in "subroutine a(x,y,z)" and
  *      "entry b(v,w,x)") and yet they both represent the same parameter.
  *      Thus in order to insure a consistant address for such parameters,
  *      the compiler allocates a local temporary, and the prologue code
  *      for each entry copies the parameters into the local temps. So, to
  *      insure that the debugger can find the parameters, a DNTT DVAR
  *      must be generated for each temporary, with the name of the DVAR
  *      being the name of the FPARAM for which the temp. was allocated.
  *
  *      The FPARAM DNTT should be used only in the LNTT.
  */


struct  DNTT_SVAR {
/*0*/   BITS             extension:   1;   /* always zero                  */
        KINDTYPE         kind:       10;   /* always K_SVAR                */
        BITS             public:      1;   /* 1 => globally visible        */
        BITS             indirect:    1;   /* 1 => pointer to object       */
        BITS             longaddr:    1;   /* 1 => in long pointer space   */
#ifdef CPLUSPLUS
	BITS		 staticmem:   1;   /* 1 => member of a class       */
	BITS		 a_union:     1;   /* 1 => anonymous union member  */
        BITS             unused:     16;
#else
        BITS             unused:     18;
#endif
/*1*/   VTPOINTER        name;           /* name of object (variable)    */
/*2*/   STATTYPE         location;       /* where stored (allocated)     */
/*3*/   DNTTPOINTER      type;           /* type information             */
/*4*/   unsigned long    offset;         /* post indirection byte offset */
/*5*/   unsigned long    displacement;   /* pre indirection byte offset  */
};                                       /* six words                    */

struct  DNTT_DVAR {
/*0*/   BITS           extension: 1;   /* always zero                  */
        KINDTYPE       kind:     10;   /* always K_DVAR                */
        BITS           public:    1;   /* 1 => globally visible        */
        BITS           indirect:  1;   /* 1 => pointer to object       */
        BITS           regvar:    1;   /* 1 => REGTYPE, not DYNTYPE    */
#ifdef CPLUSPLUS
	BITS	       a_union:   1;   /* 1 => anonymous union member  */
        BITS           unused:   17;
#else
        BITS           unused:   18;
#endif
/*1*/   VTPOINTER      name;           /* name of object (variable)    */
/*2*/   DYNTYPE        location;       /* where stored (allocated)     */
/*3*/   DNTTPOINTER    type;           /* type information             */
/*4*/   unsigned long  offset;         /* post indirection byte offset */
                                       /* for use in cobol structures  */
};                                     /* five words                   */

/*
 *      SVARs describe  static  variables  (with respect to storage, not
 *      visibility)  and  DVARs  describe  dynamic variables, and also
 *      describe  register  variables.  Note  that  SVARs  have an extra
 *      word, "offset", not needed for the other  types.  This  provides
 *      for direct data which is indexed from a base, and indirect  data
 *      which is accessed through a pointer, then indexed.  
 
 *      The "location" field of an SVAR will require a fixup.  An
 *      example of when the offset field can be useful, is a FORTRAN
 *      common block.  In a common block declaration such as "common
 *      /marx/ groucho, harpo, chico", the symbol "marx" is the only
 *      global symbol.  If "marx" is accessed indirectly, then the
 *      address of "harpo" would contain the address of "marx" in the
 *      location field (with the indirect bit on), and the offset of
 *      "harpo" from "marx" in the offset field.  If "marx" is not
 *      indirect, then location field can be filled in by a fixup of the
 *      form address(marx) + offset of harpo, and the offset field is
 *      not needed.
 *
 *      The  compilers  must emit SVARs even for data objects the linker
 *      does not know about by name, such as variables in common blocks.
 *
 *      As in the FPARAM entry, the longaddr field indicates the use
 *      of a Spectrum long pointer, and is valid only if the indirect
 *      flag is true.  The "regvar" field also has the same meaning as in
 *      the FPARAM case.
 *
 *      For languages in which a functions return value is set by assigning
 *      the value to the function name (such as FORTRAN & Pascal), a DVAR
 *      entry should also be emitted for the function.  The address of this
 *      DVAR for the function should be the address of the answer spot for
 *      the function.  This will allow the user to display the current 
 *      return value while the function is executing.
 *
 *      For a FORTRAN subroutine with alternate entries, DNTT_DVARs are 
 *      required to represent the parameters, see the DNTT_FPARAM 
 *      definition for the details.
 *
 *      The SVAR can be used in both the GNTT and LNTT, while the DVAR
 *      is only applicable to the LNTT.
 */


struct  DNTT_CONST {
/*0*/   BITS             extension:   1;   /* always zero                */
        KINDTYPE         kind:       10;   /* always K_CONST             */
        BITS             public:      1;   /* 1 => globally visible      */
        BITS             indirect:    1;   /* 1 => pointer to object     */
        LOCDESCTYPE      locdesc:     3;   /* meaning of location field  */
#ifdef CPLUSPLUS
        BITS             classmem:    1;   /* 1 => member of a class     */
        BITS             unused:     15;
#else
        BITS             unused:     16;
#endif
/*1*/   VTPOINTER        name;           /* name of object               */
/*2*/   STATTYPE         location;       /* where stored                 */
/*3*/   DNTTPOINTER      type;           /* type information             */
/*4*/   unsigned long    offset;         /* post indirection byte offset */
/*5*/   unsigned long    displacement;   /* pre indirection byte offset  */
};                                       /* six words                    */

/*
 *      The  value  of  locdesc  determines  the  meaning  of  location.
 *      Compilers  are free to use any of the  three  types  (LOC_IMMED,
 *      LOC_PTR,  LOC_VT) as feasible and  appropriate.  They might, for
 *      example,  merely  dump all CONST  values  into the VT, with some
 *      redundancy,  if they could do no better.  Ideally, each compiler
 *      would use all three types  according  to whether the constant is
 *      stored in an immediate  instruction  (so a copy is needed here),
 *      in code or data space, or nowhere else, respectively.
 *
 *      If locdesc == LOC_PTR,  CONST is very much like an SVAR, and the
 *      indirect and offset values are relevant.
 *
 *      The CONST DNTT can be used in both the GNTT and LNTT.
 */


/*
 * ---- 5.4.  TYPE-CLASS ("TYPE") DNTT ENTRIES
 */


struct  DNTT_TYPE {
/*0*/   BITS           extension: 1;   /* always zero                  */
        KINDTYPE       kind:     10;   /* either K_TYPEDEF or K_TAGDEF */
        BITS           public:    1;   /* 1 => globally visible        */
        BITS           typeinfo:  1;   /* 1 => type info available     */
        BITS           unused:   19;
/*1*/   VTPOINTER      name;           /* name of type or tag          */
/*2*/   DNTTPOINTER    type;           /* type information             */
};                                     /* three words                  */

/*
 *      The DNTT_TYPE  type is used for dtype and dtag entries.  TYPEDEFs
 *      are  just  a way  of  remembering  names  associated  with  types
 *      declared in Pascal, via "type" sections, or in C, via "typedef"s.
 *      TAGDEFs are used for C "struct",  "union", and "enum" tags, which
 *      may be  named  identically  to  "typedef"s  in  the  same  scope.
 *      TAGDEFs  always  point at STRUCTs,  UNIONs, or ENUMs (see below),
 *      and provide a way to "hang" a name onto a subtree.
 *      
 *      Note  that  named  types  point   directly   to  the   underlying
 *      structures,   not  to  intervening   TYPEDEFs  or  TAGDEFs.  Type
 *      information in TYPEDEFs and TAGDEFs point to the same  structures
 *      independent of named instantiations of the types.
 *      
 *      For example:
 *      				+
 *      	typedef struct S {	+	typedef enum E { ... } EEE;
 *      		...		+
 *      	} *pS;			+
 *      
 *      would generate something like this (shown graphically)
 *      
 *      	TYPEDEF	"pS"		+	TYPEDEF "EEE"
 *      	  |			+	  |
 *      	POINTER			+	TAG	"E"
 *      	  |			+	  |
 *      	TAG	"S"		+	ENUM
 *      	  |			+	  |
 *      	STRUCT			+	  :
 *      	  |			+	  :
 *      	  :			+
 *      	  :			+
 *      
 *      Note also that variables (of a named non-base type) must point to
 *      TYPEDEF or TAGDEF  dntt, and not the  underlying  structures.  If
 *      this is not done, the removal of duplicate global  information is
 *      impossible.
 *      
 *      The "typeinfo" flag only applies to TAGDEFs.  When not set, it is
 *      used to indicate  that an  underlying  struct,  union, or enum is
 *      named,  but  the  actual  type  is  not   declared.  In  general,
 *      "typeinfo"  will be set to 1.  It will be set to a 0 if the  type
 *      subtree is not available.  Consider the C file:
 *      
 *		typedef struct s *Sptr;
 *		main(){}
 *      
 *      which is a valid  compilation  unit with  "struct s"  defined  in
 *      another file.  For this case, the  "typeinfo" for TAGDEF "s" will
 *      be set to 0, and "type"  points to a "nil"  DNTT_STRUCT  (i.e.  a
 *      DNTT_STRUCT  entry  with  its  "firstfield",  "vartagfield",  and
 *      "varlist"  fields  set  to  DNTTNIL  and  its  "declaration"  and
 *      "bitlength"  fields set to 0).  Graphically:
 *
 *		TYPEDEF	"Sptr"
 *		  |
 *		POINTER
 *		  |
 *		TAG	"s"
 *		  |
 *		STRUCT
 *		   \---<firstfield>---> DNTTNIL
 *		    \--<vartagfield>--> DNTTNIL
 *		     \-<varlist>------> DNTTNIL
 *		      \- other fields > all set to 0
 *
 *
 *      Thus,  whenever   "typeinfo"  is  0,  "type"  must  point  to  an
 *      appropriate DNTT entry which has all its fields correctly NIL'ed.
 *      This  applies  to  *named*   DNTT_STRUCT's,   DNTT_UNION's,   and
 *      DNTT_ENUM's.
 *      
 *      The  TYPEDEF  and  TAGDEF  DNTTs may be used in both the GNTT and
 *      LNTT.
 *      
 */


struct  DNTT_POINTER {
/*0*/   BITS             extension: 1;   /* always zero                  */
#ifdef CPLUSPLUS
        KINDTYPE         kind:     10;   /* K_POINTER or K_REFERENCE     */
#else
        KINDTYPE         kind:     10;   /* always K_POINTER             */
#endif
        BITS             unused:   21;
/*1*/   DNTTPOINTER      pointsto;       /* type of object               */
/*2*/   unsigned long    bitlength;      /* size of pointer, not object  */
};                                       /* three words                  */


struct  DNTT_ENUM {
/*0*/   BITS             extension: 1;   /* always zero                  */
        KINDTYPE         kind:     10;   /* always K_ENUM                */
        BITS             unused:   21;
/*1*/   DNTTPOINTER      firstmem;       /* first MEMENUM (member)       */
/*2*/   unsigned long    bitlength;      /* packed size                  */
};                                       /* three words                  */

struct  DNTT_MEMENUM {
/*0*/   BITS             extension: 1;   /* always zero                  */
        KINDTYPE         kind:     10;   /* always K_MEMENUM             */
#ifdef CPLUSPLUS
        BITS             classmem:  1;   /* 1 => member of a class       */
        BITS             unused:   20;
#else
        BITS             unused:   21;
#endif
/*1*/   VTPOINTER        name;           /* name of member               */
/*2*/   unsigned long    value;          /* equivalent number            */
/*3*/   DNTTPOINTER      nextmem;        /* next MEMENUM, else ENUM type */
};                                       /* four words                   */

/*
 *      Each ENUM  begins a chain of (name,  value)  pairs.  The  nextmem
 *      field of the last  memenum,  should  be DNTT  NIL.  The  POINTER,
 *      ENUM,  and  MEMENUM  DNTTs  can all be used in both the  GNTT and
 *      LNTT.
 */


struct  DNTT_SET {
/*0*/   BITS             extension:   1; /* always zero                  */
        KINDTYPE         kind:       10; /* always K_SET                 */
        BITS             declaration: 2; /* normal, packed, or crunched  */
        BITS             unused:     19;
/*1*/   DNTTPOINTER      subtype;        /* type implies bounds of set   */
/*2*/   unsigned long    bitlength;      /* packed size                  */
};                                       /* three words                  */


struct  DNTT_SUBRANGE {
/*0*/   BITS             extension: 1;   /* always zero                  */
        KINDTYPE         kind:     10;   /* always K_SUBRANGE            */
        BITS             dyn_low:   2;   /* >0 => nonconstant low bound  */
        BITS             dyn_high:  2;   /* >0 => nonconstant high bound */
        BITS             unused:   17;
/*1*/   long             lowbound;       /* meaning depends on subtype   */
/*2*/   long             highbound;      /* meaning depends on subtype   */
/*3*/   DNTTPOINTER      subtype;        /* immediate type or ENUM       */
/*4*/   unsigned long    bitlength;      /* packed size                  */
};                                       /* five words                   */


struct  DNTT_ARRAY {
/*0*/   BITS             extension:    1;   /* always zero                  */
        KINDTYPE         kind:        10;   /* always K_ARRAY               */
        BITS             declaration:  2;   /* normal, packed, or crunched  */
        BITS             dyn_low:      2;   /* >0 => nonconstant low bound  */
        BITS             dyn_high:     2;   /* >0 => nonconstant high bound */
        BITS             arrayisbytes: 1;   /* 1 => array size is in bytes  */
        BITS             elemisbytes:  1;   /* 1 => elem. size is in bytes  */
        BITS             elemorder:    1;   /* 0 => in increasing order     */
        BITS             justified:    1;   /* 0 => left justified          */
        BITS             unused:      11;
/*1*/   unsigned long    arraylength;       /* size of whole array          */
/*2*/   DNTTPOINTER      indextype;         /* how to index the array       */
/*3*/   DNTTPOINTER      elemtype;          /* type of each array element   */
/*4*/   unsigned long    elemlength;        /* size of one element          */
};                                          /* five words                   */

/*
 *      The  dyn_low  and  dyn_high  fields  are  non-zero  only  if  the
 *      DNTT_SUBRANGE  is defining the range of an array index, otherwise
 *      they  are  always  zero.  The  dyn_low  and  dyn_high   bits  are
 *      duplicated in the  DNTT_SUBRANGE  defining the range of the array
 *      index  (so  sllic  can fix  the  pointers).  "dyn_low"  indicates
 *      whether  the  lower  bound  for the  subscript  of the  array  is
 *      dynamic.  If the dyn_low  field is zero, then the lowbound  field
 *      of the DNTT_SUBRANGE  entry, pointed to by the indextype field in
 *      the  DNTT_ARRAY  entry, is interpreted as a constant lower bound.
 *      If the dyn_low  field is 1, then the  lowbound  field of the DNTT
 *      SUBRANGE is interpreted as a DYNTYPE giving a local address where
 *      the lower  bound can be found.  If the  dyn_low  field is 2, then
 *      the  lowbound  field of the  DNTT_SUBRANGE  is  interpreted  as a
 *      DNTTPOINTER to a variable  whose value is the lower bound (needed
 *      if the lower bound is a static variable).  The dyn_low value of 3
 *      is not used.  The "dyn_high" bit has a similar  meaning  relating
 *      to the upper bound.  If an upper bound for an array  parameter is
 *      not given (like  assumed  size arrays in FORTRAN, or "char foo[]"
 *      in C) then the upper  bound in the  DNTT_SUBRANGE  should  be the
 *      largest  integer  that fits in a long  integer, so that any value
 *      the user can give is legal.
 *
 *      "arrayisbytes"  indicates that the field  "arraylength"  contains
 *      the length in bytes rather then bits.  This is needed on Spectrum
 *      where an array  could be up to 2**32  bytes.  A value of zero for
 *      bitsize will be used to represent 2**32.
 *
 *      "elemisbytes"  indicates that the field "elemlength" contains the
 *      elem.  length in bytes rather then bits.  The "elemlength"  field
 *      contains  the not the "true"  size of an array  element,  but the
 *      size  allocated to each element within the array (the "true" size
 *      plus any wasted  bits on the left or right).  As an example for a
 *      Pascal array of a 13 bit  structure, the array element size might
 *      equal 16, with the  justified  field  equal to 0 to indicate  the
 *      structure is left justified  within the 16 bits.  The "true" size
 *      of the  structure  would  be  found  in  the  size  field  of the
 *      DNTT_STRUCT pointed to by the "elemtype" field of the DNTT_ARRAY.
 *
 *      "indextype"   typically   points  to  a  SUBRANGE   for   bounds.
 *      "elemtype"  may  point to  another  ARRAY  for  multi-dimensional
 *      arrays.  Row or column precedence in the language is reflected in
 *      the order of the ARRAY  entries  on the chain.  For  example,  in
 *      Pascal, which is  row-precedent,  an array declared  [1..2, 3..4,
 *      5..6] would  result in "array 1..2 of array 3..4 of array 5..6 of
 *      ...".   The   same    declaration    in    FORTRAN,    which   is
 *      column-precedent,  would  result in "array  5..6 of array 3..4 of
 *      array 1..2 of ...".  This makes index-to-address  conversion much
 *      easier.  Either  way  an   expression   handler   must  know  the
 *      precedence for the language.
 *
 *      The SET,  SUBRANGE,  and ARRAY DNTTs can be used in both the GNTT
 *      and LNTT.
 */


struct  DNTT_STRUCT {
/*0*/   BITS             extension:   1; /* always zero                  */
        KINDTYPE         kind:       10; /* always K_STRUCT              */
        BITS             declaration: 2; /* normal, packed, or crunched  */
        BITS             unused:     19;
/*1*/   DNTTPOINTER      firstfield;     /* first FIELD, if any          */
/*2*/   DNTTPOINTER      vartagfield;    /* variant tag FIELD, or type   */
/*3*/   DNTTPOINTER      varlist;        /* first VARIANT, if any        */
/*4*/   unsigned long    bitlength;      /* total at this level          */
};                                       /* five words                   */

/*
 *      The "declaration", "vartagfield", and "varlist" fields apply to
 *      Pascal/Modcal records only and are nil for record structures in 
 *      other languages.  If there is a tag, then the "vartagfield" points 
 *      to the FIELD DNTT describing the tag.  Otherwise, the "vartagfield"
 *      points to the tag type.
 *
 *      The STRUCT DNTT may be used in both the GNTT and LNTT.
 */


struct  DNTT_UNION {
/*0*/   BITS             extension: 1;   /* always zero                  */
        KINDTYPE         kind:     10;   /* always K_UNION               */
        BITS             unused:   21;
/*1*/   DNTTPOINTER      firstfield;     /* first FIELD entry            */
/*2*/   unsigned long    bitlength;      /* total at this level          */
};                                       /* three words                  */

/*
 *      This type supports C unions only and is not used otherwise.
 *
 *      Since  STRUCTUREs  and UNIONs are not  packable  inside of outer
 *      STRUCTUREs and UNIONs, their  bitlengths  tell their actual (not
 *      necessarily  packed)  size,  according  only as to how they  are
 *      internally packed.
 *
 *      The STRUCT DNTT may be used in both the GNTT and LNTT.
 */


struct  DNTT_FIELD {
/*0*/   BITS             extension: 1;   /* always zero                  */
        KINDTYPE         kind:     10;   /* always K_FIELD               */
#ifdef CPLUSPLUS
	BITS	         visibility:2;   /* pub = 0, prot = 1, priv = 2  */
	BITS	         a_union:   1;   /* 1 => anonymous union member  */
#ifdef TEMPLATES
	BITS		 staticMem: 1;	 /* 1 -> static member of a template */
        BITS             unused:   17;
#else /* TEMPLATES */
        BITS             unused:   18;
#endif /* TEMPLATES */
#else
        BITS             unused:   21;
#endif
/*1*/   VTPOINTER        name;           /* name of field, if any        */
/*2*/   unsigned long    bitoffset;      /* of object itself in STRUCT   */
/*3*/   DNTTPOINTER      type;           /* type information             */
/*4*/   unsigned long    bitlength;      /* size at this level           */
/*5*/   DNTTPOINTER      nextfield;      /* next FIELD in STRUCT, if any */
};                                      /* six words                    */

/*
 *      This  type  describes  the  fields  in  Pascal   records  and  C
 *      structures  and unions.  The  bitoffset is from the start of the
 *      STRUCT  or UNION  that  started  the  chain, to the start of the
 *      object  itself,   ignoring  any  padding.  Note  that  bitoffset
 *      does not  have  to  be  on a  byte  boundary.  For  unions,  each
 *      bitoffset should be zero since all fields overlap.
 *
 *      The bitlength field is the same as that of the type except for C
 *      bit fields, which may be a different size than the base type.
 * 
 *      The FIELD DNTT can be used in both the GNTT and LNTT.
 */


struct  DNTT_VARIANT {
/*0*/   BITS             extension: 1;   /* always zero                  */
        KINDTYPE         kind:     10;   /* always K_VARIANT             */
        BITS             unused:   21;
/*1*/   long             lowvarvalue;    /* meaning depends on vartype   */
/*2*/   long             hivarvalue;     /* meaning depends on vartype   */
/*3*/   DNTTPOINTER      varstruct;      /* this variant STRUCT, if any  */
/*4*/   unsigned long    bitoffset;      /* of variant, in outer STRUCT  */
/*5*/   DNTTPOINTER      nextvar;        /* next VARIANT, if any         */
};                                       /* six words                    */

/*
 *      "varstruct"  points to the STRUCT  which in turn  describes  the
 *      contents  of the  variant.  The  latter  might in turn  point to
 *      VARIANTs of its own, and to FIELDs which point to other STRUCTs.
 *      "lowvarvalue" and "hivarvalue" are the range of values for which
 *      this variant applys; more than one dntt VARIANT may be necessary  
 *      to describe the range (e.g., 'a'..'n','q':).  A type field is un-
 *      necessary, as the type can be obtained from the "vartagfield" 
 *      field of the STRUCT DNTT.
 *
 *      The VARIANT DNTT can be used in both the GNTT and LNTT.
 */
          

struct  DNTT_FILE {
/*0*/   BITS             extension: 1;   /* always zero                  */
        KINDTYPE         kind:     10;   /* always K_FILE                */
        BITS             ispacked:  1;   /* 1 => file is packed          */
        BITS             unused:   20;
/*1*/   unsigned long    bitlength;      /* of whole element buffer      */
/*2*/   unsigned long    bitoffset;      /* of current element in buffer */
/*3*/   DNTTPOINTER      elemtype;       /* type and size of of element  */
};                                       /* four words                   */

/*
 *      Pascal/Modcal is the only language of interest with built-in file
 *      buffering.  For Pascal/Modcal files, the symbol table tells the file
 *      element type, the sizes of the current element (via  "elemtype")
 *      and the whole buffer (via "bitlength"),  and the locations of the
 *      element  buffer (from the parent  "NAME"  entry) and the element
 *      itself within the buffer,  following  header  information  (from
 *      "bitoffset").
 *
 *      The FILE DNTT can be used in both the GNTT and LNTT.
 */

          
struct  DNTT_FUNCTYPE {
/*0*/   BITS             extension: 1;   /* always zero                  */
        KINDTYPE         kind:     10;   /* always K_FUNCTYPE            */
	BITS             varargs:   1;   /* func-proto ellipses.         */
	BITS             info:      4;   /* lang-specific stuff; F_xxxx  */
        BITS             unused:   16;
/*1*/   unsigned long    bitlength;      /* size of function pointer     */
/*2*/   DNTTPOINTER      firstparam;     /* first FPARAM, if any         */
/*3*/   DNTTPOINTER      retval;         /* return type, if any          */
};                                       /* four words                   */

/*
 *      This  type  supports  function   variables  in  a  limited  way,
 *      including the parameter types (if any) and the return value type
 *      (if any).
 *
 *      See DNTT_FUNC for discussion of various fields.
 *
 *      The FUNCTYPE DNTT can be used in both the GNTT and LNTT.
 */


struct  DNTT_COBSTRUCT {
/*0*/   BITS             extension: 1;   /* always zero                  */
        KINDTYPE         kind:     10;   /* always K_COBSTRUCT           */
        BITS             hasoccurs: 1;   /* descendant has OCCURS clause */
        BITS             istable:   1;   /* is a table item?             */
        BITS             unused:   19;
/*1*/   DNTTPOINTER      parent;         /* next higher data item        */
/*2*/   DNTTPOINTER      child;          /* 1st descendant data item     */
/*3*/   DNTTPOINTER      sibling;        /* next data item at this level */
/*4*/   DNTTPOINTER      synonym;        /* next data item w/ same name  */
/*5*/   BITS             catusage:  6;   /* category or usage of item    */
        BITS             pointloc:  8;   /* location of decimal point    */
        BITS             numdigits:10;   /* number of digits             */
        BITS             unused2:   8;  
/*6*/   DNTTPOINTER      table;          /* array entry describing table */
/*7*/   VTPOINTER        editpgm;        /* name of edit subprogram      */
/*8*/   unsigned long    bitlength;      /* size of item in bits         */
};                                       /* nine words                   */

/*
 *      This entry is used to describe COBOL data items and table items.
 *      A Cobol variable will begin with a DNTT_SVAR, DNTT_DVAR, or DNTT_
 *      FPARAM whose "type" field is a DNTTPOINTER to a DNTT_COBSTRUCT.
 *   
 *      "parent", "child", "sibling", and "synonym" are DNTTPOINTER to
 *      other DNTT_SVAR, DNTT_DVAR, or DNTT_FPARAMs having these particular
 *      relationships with the current DNTT_COBSTRUCT (or are set to DNTTNIL 
 *      if no such relationship exists).
 *
 *      "hasoccurs" is set to 1 if the descendent of this COBOL element
 *      (pointed to by "child") has an OCCURS ... DEPENDING ON clause.
 *
 *      "istable" is set to 1 if this COBOL data item is a table.  In this
 *      case, "table" will point to a DNTT_ARRAY entry describing the table.
 *
 *      The COBSTRUCT DNTT can be used in both the GNTT and LNTT.
 */

/*
 * Used for C too so pulled out of ifdef CPLUSPLUS.
 */

struct  DNTT_MODIFIER {
/*0*/   BITS             extension:   1; /* always zero                   */
        KINDTYPE         kind:       10; /* always K_MODIFIER             */
	BITS		 m_const:     1; /* const                         */
	BITS		 m_static:    1; /* static                        */
	BITS		 m_void:      1; /* void                          */
	BITS		 m_volatile:  1; /* volatile                      */
	BITS		 m_duplicate: 1; /* duplicate                     */
	BITS		 unused:     16;
/*1*/   DNTTPOINTER      type;           /* subtype                       */
};                                       /* two words                     */

#ifdef CPLUSPLUS
struct  DNTT_GENFIELD {
/*0*/   BITS             extension:   1; /* always zero                   */
        KINDTYPE         kind:       10; /* always K_GENFIELD             */
        BITS             visibility:  2; /* pub = 0, prot = 1, priv = 2   */
	BITS		 a_union:     1; /* 1 => anonymous union member   */
        BITS             unused:     18;
/*1*/   DNTTPOINTER      field;          /* pointer to field or qualifier */
/*2*/   DNTTPOINTER      nextfield;      /* pointer to next field         */
};                                       /* three words                   */

struct  DNTT_MEMACCESS {
/*0*/   BITS             extension:   1; /* always zero                   */
        KINDTYPE         kind:       10; /* always K_MEMACCESS            */
        BITS             unused:     21;
/*1*/   DNTTPOINTER      classptr;       /* pointer to base class         */
/*2*/   DNTTPOINTER      field;          /* pointer field                 */
};                                       /* three words                   */

struct  DNTT_VFUNC {
/*0*/   BITS             extension:   1; /* always zero                   */
        KINDTYPE         kind:       10; /* always K_VFUNCTION            */
	BITS		 pure:        1; /* pure virtual function ?       */
	BITS		 unused:     20;
/*1*/   DNTTPOINTER	 funcptr;        /* function name                 */
/*2*/   unsigned long    vtbl_offset;    /* offset into vtbl for virtual  */
};                                       /* three words                   */

struct  DNTT_CLASS_SCOPE {
/*0*/   BITS             extension:   1; /* always zero                   */
        KINDTYPE         kind:       10; /* always K_CLASS_SCOPE          */
	BITS		 unused:     21;
/*1*/   SLTPOINTER	 address;        /* pointer to SLT entry          */
/*2*/   DNTTPOINTER      type;           /* pointer to class type DNTT    */
};                                       /* three words                   */

struct  DNTT_FRIEND_CLASS {
/*0*/   BITS             extension:   1; /* always zero                   */
        KINDTYPE         kind:       10; /* always K_FRIEND_CLASS         */
	BITS		 unused:     21;
/*1*/   DNTTPOINTER      classptr;       /* pointer to class DNTT         */
/*2*/   DNTTPOINTER      next;           /* next DNTT_FRIEND              */
};                                       /* three words                   */

struct  DNTT_FRIEND_FUNC {
/*0*/   BITS             extension:   1; /* always zero                   */
        KINDTYPE         kind:       10; /* always K_FRIEND_FUNC          */
	BITS		 unused:     21;
/*1*/   DNTTPOINTER      funcptr;        /* pointer to function           */
/*2*/   DNTTPOINTER      classptr;       /* pointer to class DNTT         */
/*3*/   DNTTPOINTER      next;           /* next DNTT_FRIEND              */
};                                       /* four words                    */

struct  DNTT_CLASS {
/*0*/   BITS             extension:   1; /* always zero                   */
        KINDTYPE         kind:       10; /* always K_CLASS                */
	BITS		 abstract:    1; /* is this an abstract class?    */
	BITS		 class_decl:  2; /* 0=class,1=union,2=struct      */
#ifdef TEMPLATES
	BITS		 expansion:   1; /* 1=template expansion	  */
	BITS		 unused:     17;
#else /* TEMPLATES */
	BITS		 unused:     18;
#endif /* TEMPLATES */
/*1*/   DNTTPOINTER      memberlist;     /* ptr to chain of K_[GEN]FIELDs */
/*2*/   unsigned long    vtbl_loc;       /* offset in obj of ptr to vtbl  */
/*3*/   DNTTPOINTER      parentlist;     /* ptr to K_INHERITANCE list     */
/*4*/   unsigned long    bitlength;      /* total at this level           */
/*5*/   DNTTPOINTER      identlist;      /* ptr to chain of class ident's */
/*6*/   DNTTPOINTER      friendlist;     /* ptr to K_FRIEND list          */
#ifdef TEMPLATES
/*7*/	DNTTPOINTER	 templateptr;	 /* ptr to template		  */
/*8*/	DNTTPOINTER	 nextexp;	 /* ptr to next expansion         */
#else /* TEMPLATES */
/*7*/   unsigned long    future2;
/*8*/   unsigned long    future3;
#endif /* TEMPLATES */
};                    
                   /* nine words                    */
#ifdef TEMPLATES
struct  DNTT_TEMPLATE {
/*0*/   BITS             extension:   1; /* always zero                   */
        KINDTYPE         kind:       10; /* always K_TEMPLATE             */
	BITS		 abstract:    1; /* is this an abstract class?    */
	BITS		 class_decl:  2; /* 0=class,1=union,2=struct      */
	BITS		 unused:     18;
/*1*/   DNTTPOINTER      memberlist;     /* ptr to chain of K_[GEN]FIELDs */
/*2*/   long             unused2;        /* offset in obj of ptr to vtbl  */
/*3*/   DNTTPOINTER      parentlist;     /* ptr to K_INHERITANCE list     */
/*4*/   unsigned long    bitlength;      /* total at this level           */
/*5*/   DNTTPOINTER      identlist;      /* ptr to chain of class ident's */
/*6*/   DNTTPOINTER      friendlist;     /* ptr to K_FRIEND list          */
/*7*/   DNTTPOINTER	 arglist;	 /* ptr to argument list	  */
/*8*/   DNTTPOINTER	 expansions;	 /* ptr to expansion list	  */
};                    

/*
 * DNTT_TEMPLATEs only appear in the GNTT. Functions and
 * classes templates cannot be local. (Their instantions may be).
 */

struct  DNTT_TEMPL_ARG {
/*0*/   BITS         extension: 1;   /* always zero                  */
        KINDTYPE     kind:     10;   /* always K_TEMPL_ARG           */
	BITS	     usagetype:1;    /* 0 type-name 1 expression     */
        BITS         unused:   20;
/*1*/   VTPOINTER    name;           /* name of argument             */
/*2*/	DNTTPOINTER  type;	     /* for non type arguments	     */
/*3*/   DNTTPOINTER  nextarg;        /* Next argument if any         */
/*4*/	long	     unused2[2];
};                                   /* 6 words                      */

/*
 * Pxdb fills in the prevexp, and nextexp in the
 * DNTT_CLASS. Pxdb also fills in the expansions field in the
 * DNTT_TEMPLATE.
 */
#endif /* TEMPLATES */

struct  DNTT_PTRMEM {
/*0*/   BITS             extension:   1; /* always zero                   */
        KINDTYPE         kind:       10; /* K_PTRMEM or K_PTRMEMFUNC      */
	BITS		 unused:     21;
/*1*/   DNTTPOINTER      pointsto;       /* pointer to class DNTT         */
/*2*/   DNTTPOINTER      memtype;	 /* type of member		  */
};                                       /* three words                   */

struct	DNTT_INHERITANCE {
/*0*/   BITS             extension:   1; /* always zero                   */
        KINDTYPE         kind:       10; /* K_INHERITANCE                 */
	BITS		 Virtual:     1; /* virtual base class ?          */
	BITS		 visibility:  2; /* pub = 0, prot = 1, priv = 2   */
	BITS		 unused:     18;
/*1*/   DNTTPOINTER      classname;      /* first parent class, if any    */
/*2*/   unsigned long    offset;         /* offset to start of base class */
/*3*/   DNTTPOINTER      next;       	 /* pointer to next K_INHERITANCE */
};                                       /* four words                    */

struct	DNTT_OBJECT_ID {
/*0*/   BITS             extension:   1; /* always zero                   */
        KINDTYPE         kind:       10; /* K_OBJECT_ID                   */
	BITS		 unused:     21;
/*1*/   unsigned long    object_ident;   /* object identifier             */
/*2*/   unsigned long    offset;         /* offset to start of base class */
/*3*/   DNTTPOINTER      next;       	 /* pointer to next K_OBJECT_ID   */
/*4*/   unsigned long    segoffset;      /* for linker fixup              */
};                                       /* five words                    */
#endif

/*
 *  DNTT_XREF ENTRY:
 *      This entry is used to retrieve cross-reference information from
 *      the XREF Table (XT). A DNTT_XREF entry immediately follows the  
 *      DNTT_SVAR, DNTT_DVAR, DNTT_TYPE, etc. entry to which it pertains.   
 *
 *      The XREFPOINTER points into the XT table where the information
 *      about the previous DNTT entry is contained.  If no entries are
 *	generated in the XT table, the xreflist field should contain
 *	XREFNIL.  The language field contains the source language
 *      (LANG_xxx) value of the DNTT object.
 *
 *      The XREF DNTT can be used in both the GNTT and LNTT.
 */

struct  DNTT_XREF {
/*0*/   BITS             extension: 1;   /* always zero                  */
        KINDTYPE         kind:     10;   /* always K_XREF                */
	BITS             language:  4;   /* language of DNTT object      */
        BITS             unused:   17;
/*1*/   XREFPOINTER      xreflist;       /* index into XREF subspace     */
/*2*/   long             extra;          /* free                         */
};                                       /* three words                  */


/*
 *  DNTT_SA ENTRY:
 *      This entry is used with static analysis info.  It supplies the
 *      name and kind for a few special cases not currently handled by a
 *      DNTT_SVAR, DNTT_DVAR, DNTT_TYPE, etc.  It is used for a local
 *      entity that has a global scope.
 *
 *      Example: a function, has a DNTT_FUNCTION entry in the LNTT;
 *      but it can be seen globally, thus a K_SA will be emitted in
 *      the GNTT, with the functions name and a base_kind of K_FUNCTION;
 *      the DNTT_XREF will follow the DNTT_SA, not the DNTT_FUNCTION.
 *
 *      The DNTT_SA is also used for C macros.
 *
 *      The XREF DNTT can be used in both the GNTT and LNTT.
 */

struct  DNTT_SA {
/*0*/   BITS             extension: 1;   /* always zero                  */
        KINDTYPE         kind:     10;   /* always K_SA                  */
        KINDTYPE         base_kind:10;   /* K_FUNCTION, K_LABEL, etc     */
        BITS             unused:   11;
/*1*/   VTPOINTER        name;
/*2*/   long             extra;          /* free                         */
};                                       /* three words                  */


/*
 * ---- 5.5.  OVERALL DNTT ENTRY FORMAT
 */


/*
 * Generic entry for easy access:
 */

struct  DNTT_GENERIC {            /* rounded up to whole number of blocks */
        unsigned long   word [9];
};

struct DNTT_BLOCK {                    /* easy way to deal with one block */
/*0*/  BITS             extension: 1;  /* always zero                     */
       KINDTYPE         kind:     10;  /* kind of dnttentry               */
       BITS             unused:   21;
/*1*/  unsigned long    word [2];
};


/*
 * Overall format:
 */

union   dnttentry {
        struct  DNTT_SRCFILE    dsfile;

        struct  DNTT_MODULE     dmodule;
        struct  DNTT_FUNC       dfunc;
        struct  DNTT_FUNC       dentry;
        struct  DNTT_FUNC       dblockdata;
        struct  DNTT_BEGIN      dbegin;
        struct  DNTT_END        dend;
        struct  DNTT_IMPORT     dimport;
        struct  DNTT_LABEL      dlabel;
        struct  DNTT_WITH       dwith;
        struct  DNTT_COMMON     dcommon;

        struct  DNTT_FPARAM     dfparam;
        struct  DNTT_SVAR       dsvar;
        struct  DNTT_DVAR       ddvar;
        struct  DNTT_CONST      dconst;
 
        struct  DNTT_TYPE       dtype;
        struct  DNTT_TYPE       dtag;
        struct  DNTT_POINTER    dptr;
        struct  DNTT_ENUM       denum;
        struct  DNTT_MEMENUM    dmember;
        struct  DNTT_SET        dset;
        struct  DNTT_SUBRANGE   dsubr;
        struct  DNTT_ARRAY      darray;
        struct  DNTT_STRUCT     dstruct;
        struct  DNTT_UNION      dunion;
        struct  DNTT_FIELD      dfield;
        struct  DNTT_VARIANT    dvariant;
        struct  DNTT_FILE       dfile;
        struct  DNTT_FUNCTYPE   dfunctype;
        struct  DNTT_COBSTRUCT  dcobstruct;

#ifdef CPLUSPLUS
	struct	DNTT_CLASS_SCOPE  dclass_scope;
	struct	DNTT_POINTER      dreference;
	struct	DNTT_PTRMEM       dptrmem;
	struct	DNTT_PTRMEM       dptrmemfunc;
	struct	DNTT_CLASS	  dclass;
	struct	DNTT_GENFIELD     dgenfield;
	struct	DNTT_VFUNC        dvfunc;
	struct	DNTT_MEMACCESS    dmemaccess;
	struct	DNTT_INHERITANCE  dinheritance;
	struct	DNTT_FRIEND_CLASS dfriend_class;
	struct	DNTT_FRIEND_FUNC  dfriend_func;
	struct	DNTT_MODIFIER     dmodifier;
	struct	DNTT_OBJECT_ID    dobject_id;
	struct	DNTT_FUNC         dmemfunc;
#ifdef TEMPLATES
	struct DNTT_TEMPLATE	  dtemplate;
	struct DNTT_TEMPL_ARG	  dtempl_arg;
	struct DNTT_FUNC_TEMPLATE dfunctempl;
	struct DNTT_LINK	  dlink;	/* generic */
	struct DNTT_TFUNC_LINK	  dtflink;
#endif /* TEMPLATES */
#endif

        struct  DNTT_XREF       dxref;
        struct  DNTT_SA         dsa;

        struct  DNTT_GENERIC    dgeneric;
        struct  DNTT_BLOCK      dblock;
};


/*
 * ---- 6.  SOURCE LINE TABLE (SLT) ENTRY FORMAT
 */

/*
 * Type of SLT special entry:
 *
 *      Sizeof  (SLTTYPE) = 4 bits, for a maximum of 16 possible special
 *      slttypes.  Note that SLT_NIL is the same as SLTNIL.
 */

typedef unsigned int SLTTYPE;

#define SLT_NIL  SLTNIL

#define SLT_NORMAL      0              /* note that the field is unsigned */
#define SLT_SRCFILE     1
#define SLT_MODULE      2
#define SLT_FUNCTION    3
#define SLT_ENTRY       4
#define SLT_BEGIN       5
#define SLT_END         6
#define SLT_WITH        7
#define SLT_EXIT        8
#define SLT_ASSIST      9
#define SLT_MARKER     10
#ifdef CPLUSPLUS
#define SLT_CLASS_SCOPE 11
#endif

struct  SLT_NORM {
        SLTTYPE        sltdesc: 4;     /* always zero          */
        BITS           line:   28;     /* where in source text */
        ADDRESS        address;        /* where in function    */
};                                     /* two words            */

struct  SLT_SPEC {
        SLTTYPE        sltdesc: 4;     /* special entry type   */
        BITS           line:   28;     /* where in source text */
        DNTTPOINTER    backptr;        /* where in DNTT        */
};                                     /* two words            */

struct  SLT_ASST {
        SLTTYPE        sltdesc:   4;   /* always nine          */
        BITS           unused:   28;
        SLTPOINTER     address;        /* first SLT normal     */
};                                     /* two words            */

struct  SLT_GENERIC {
        unsigned long   word[2];
};                                      /* two words            */


union   sltentry {
        struct  SLT_NORM    snorm;
        struct  SLT_SPEC    sspec;
        struct  SLT_ASST    sasst;
        struct  SLT_GENERIC sgeneric;
};                                      /* two words            */

#define SLTBLOCKSIZE    8
#define SLT_LN_PROLOGUE 0x0fffffff

/*
 *      This table  consists  of a series of  entries,  each of which is
 *      either normal, special, or assist according to the sltdesc field 
 *      of the first word.  Normal entries contain an address (actually
 *      a code offset relative to the beginning of the current function) 
 *      and a source/listing line (by line number).  Listing line numbers
 *      may be used in place of source line numbers based upon a compiler
 *      option.  This will also be reflected in the DNTT_SRCFLE entries.
 *      Special  entries  also provide a line number (where something was 
 *      declared) and point back to the DNTT which references them.  This
 *      is used for quick determination of scope, including source/listing
 *      file, after an interrupt.  Even if there are multiple source/listing
 *      files, all source/listing line information is accumulated in this 
 *      one table.
 *
 *      The SLT was originally designed to be unnested, even for those
 *      languages whose LNTT must reflect their nesting.  The debuggers
 *      depend upon this.  For those languages that are nested the SLT
 *      must now be nested and an SLT_ASST must immediately follow each 
 *      SLT_SPEC of type FUNC.  The "address" field will be filled in by 
 *      the compiler back-ends to point forward to the first SLT_NORM in 
 *      the FUNC's scope.  The "firstnorm" is set to one if this SLT_NORM 
 *      is the first SLT_NORM looking sequentially forward in the SLT.
 *   
 *      The one exception to the normal/special/assist rule is the EXIT SLT.  
 *      The EXIT SLT is used to identify exit points for a routine.  The
 *      EXIT SLT is a special only in the sense that the sltdesc field
 *      is not equal to SLT_NORMAL.  However, it contains a line number 
 *      and address like a normal SLT.  The EXIT SLT is used in place of
 *      a NORMAL SLT for all exit statements (such as "return" in C and
 *      FORTRAN, or the "end" of a procedure body in Pascal).
 *
 *      The SLT_MARKER is for use in "Chunk-Per-Som". The address field
 *      contains a new base address (replacing the current procedure's
 *      low-address field.  This new base address will be added to succeding
 *      SLT_NORMALs and SLT_EXITs to produce an absolute address.
 *
 *      To distinguish prologue (function setup) code emitted at the END
 *      of a function from the last line (normal SLT) of the function, a
 *      normal  SLT entry with a line number of  SLT_LN_PRLOGUE is used.
 *      Such SLT entries are only emitted if there is trailing  prologue
 *      code, and they are always the last SLT emitted for the  function
 *      except for the special SLT entry for the function END.  For com-
 *      pilers that emit the prologue code before the main body, no 
 *      special prologue SLT entry is required.
 *
 *      One SLT entry is emitted  for (the FIRST  physical line of) each
 *      executable  statement, for each  construct that generates a DNTT
 *      entry which  points to an SLT entry, and for the prologue  code,
 *      if any.  The user cannot set a breakpoint without a corresponding
 *      SLT entry.  Compilers  must emit  multiple SLT  entries for parts
 *      of a composite statement (such as FOR) and for multiple statements
 *      appearing on one source line.
 *
 *      For compatibility, the high bits of DNTTPOINTERs  in SLT entries
 *      are also set to 1, even though they are not needed here.
 *
 *      The global bit on DNTTPOINTERs in SLT entries should always be 0,
 *      as the LNTT contains all the scoping information.
 */

/*
 * ---- 7.  VALUE TABLE (VT) ENTRY FORMAT
 *
 *
 *      This table  contains  symbol  names  plus  values for DNTT_CONST
 *      entries of type LOC_VT.  All strings are null-terminated, as in C.
 *      There are no restrictions on the lengths of values nor the order 
 *      in which they may appear.  All symbol names are exactly as given 
 *      by the user, e.g. there are no prepended underscores.
 *
 *      CONST  values are not (and need not be)  terminated  in any way.
 *      They  may be  forced  to  word  boundaries  if  necessary,  with
 *      resulting wasted bytes.
 *
 *      The  first  byte  of the  table  must  be  zero (a  null  string
 *      terminator), so that the null VTPOINTER results in a null name.
 */

/*
 * ---- 8.  XREF TABLE (XT) ENTRY FORMAT
 *
 *      This table contains static information about each named object in
 *      a  compilation  unit.  It consists of a  collection  of of lists,
 *      each list  associated  with a DNTT object via the DNTT_XREF  that
 *      follows the object.  The DNTT_XREF  contains an XREFPOINTER which
 *      is an offset into the XT table, and denotes the  beginning of the
 *      reference list.
 *      
 *      Each list is  actually  one or more of linear  sub-list  that are
 *      linked  together.  Each  sublist  begins with an XREFNAME  entry,
 *      which names a (current)  source file.  Following  the XREFNAME is
 *      one or more  XREFINFO  entries,  one for each  appearance  of the
 *      object's name in the current  file.  These entries list what type
 *      of reference  and the line no.  within the file.  Column  numbers
 *      are currently  unsupported.  The XREFINFO1  structure is normally
 *      used.  The XREFINFO2A/B structure pair is only used for compilers
 *      which  support  line  numbers  greater  than  16  bits  long.  An
 *      XREFLINK marks the end of a sublist, so a typical  sequence looks
 *      like:
 *
 *		XREFNAME, XREFINFO1, XREFINFO1, ... , XREFLINK
 *
 *      Note that all  elements  of a sublist  must  appear  in  sequence
 *      (linearly).  If the list must be continued,  the XREFLINK  serves
 *      as a  continuation  pointer  from one  sublist  to the  next, and
 *      contains  another  offset  into the XT where the next  sublist is
 *      found  for the same  named  object.  If  there  is no  additional
 *      sublist, the XREFLINK contains a 0 index, denoting the end of the
 *      current list.
 *      
 *      Lists  for  the  same  named   object  may  appear  in  different
 *      compilation  units.  It is the  responsibility  of  PXDB  to link
 *      these together.
 *      
 */

#define XTBLOCKSIZE  4

#define XINFO1  0
#define XINFO2  1
#define XLINK   2
#define XNAME   3

struct XREFINFO1 {
       BITS       tag:          3;  /* always XINFO1            */
       BITS       definition:   1;  /* True => definition	*/
       BITS       declaration:  1;  /* True => declaration	*/
       BITS       modification: 1;  /* True => modification	*/
       BITS       use:          1;  /* True => use		*/
       BITS       call:         1;  /* True => call             */
       BITS       column:       8;  /* Unsigned Byte for Column within line */
       BITS       line:        16;  /* Unsigned 16-bits for line # relative */
                                    /* to beginning of current inlude file. */
};

struct XREFINFO2A {
  /* first word */
       BITS       tag:          3;  /* always XINFO2A           */
       BITS       definition:   1;  /* True => definition	*/
       BITS       declaration:  1;  /* True => declaration	*/
       BITS       modification: 1;  /* True => modification	*/
       BITS       use:          1;  /* True => use		*/
       BITS       call:         1;  /* True => call             */
       BITS       extra:       16;  /* ?                        */
       BITS       column:       8;  /* ?                        */
};

struct XREFINFO2B {
  /* second word */
       BITS       line:        32;  /* Unsigned 32-bits for line # relative */
                                    /* to beginning of current file.        */
};

struct XREFLINK {
       BITS       tag:       3;   /* always XLINK for XREFLINK           */
       BITS       next:     29;   /* index of next list. If              */
                                  /* zero then this is the end of line.  */
                                  /* a.k.a. continuation pointer         */
};
struct XREFNAME {
       BITS       tag:       3;   /* always XNAME for XREFNAME           */
       BITS       filename: 29;   /* VTPOINTER to file name              */
};

union xrefentry {
       struct XREFINFO1  xrefshort;
       struct XREFINFO2A xreflong;
       struct XREFINFO2B xrefline;
       struct XREFLINK   xlink;
       struct XREFNAME   xfname;
};


/*
 * ---- 9.  ORDERING OF TABLE ENTRIES
 *
 *
 *      LNTT and SLT  entries  must be emitted  and kept in source  file
 *      order wherever  possible.  As a minimum, named LNTT entries must
 *      be  emitted  and kept  within  the  proper  scope,  though  some
 *      compilers  may emit  them at the end of a scope  instead  of the
 *      beginning.  In  general,  the  debugger  must know the  emission
 *      rules for the  language it is dealing  with, and search the LNTT
 *      accordingly,  or else  always  search in both  directions.  
 *
 *      Items in the GNTT are all global, so the  public bit must always
 *      be set.  Within the LNTT, the public bit indicates that the item
 *      is exported  by the module in which  it resides, and  is visible
 *      within a module  or procedure that imports the containing module.
 *
 *      Compilers and linkers are encouraged to make multiple references
 *      to DNTT, SLT, and VT entries (even chains of DNTT entries) where
 *      possible  to reduce  redundancy  with no loss of data.  They are
 *      also  encouraged to emit entries grouped so that related entries
 *      are physically close, as long as no scope rules are violated.
 *
 *      SLT entries  must be emitted in sorted line number order  within
 *      each  file,  except  for  special  SLT  entries  for  ENTRYs and
 *      FUNCTIONs  only.  They may be out of line  number  order (due to
 *      nested  functions, etc.) so long as the next normal SLT entry is
 *      the proper place to breakpoint  the entity.  For example,  there
 *      can be numerous  ENTRY types after a FUNCTION, all  referring to
 *      the same code  location.  (If  there are no normal  SLT  entries
 *      before the next FUNCTION or MODULE entry and a SLT_ASST does not
 *      immediately  follow the  SLT_SPEC for a FUNC, the  entity has no
 *      breakpointable locations.)
 *
 *      SLT  entries  must be sorted in  ascending  code  address  order
 *      WITHIN  EACH  MODULE  or  FUNCTION  body.  It is  impossible  to
 *      require  that they be sorted  both by file line  number and code
 *      address  because  function  object code may be emitted or linked
 *      out of source order in a segment.
 *
 *      It is reasonable  to expect  sequential SLT entries may have the
 *      same line numbers or code locations (but not both, as that would 
 *      be  redundant).  This might be due to multiple statements on one
 *      source line or several scope levels starting at one place in the
 *      code.
 *
 *      Thus, for  nested languages  like  Pascal and  Modcal, the  LNTT 
 *      entries must  be nested to reflect the program's scope.  The SLT
 *      entries should also be  nested with an SLT_ASST  entry following
 *      each SLT_SPEC of type FUNC.
 */


/*
 * ---- 10.  LINKER CONSIDERATIONS
 *
 *      As stated earlier, all fixups to the debug information are
 *      done through the generation of a list of fixups for the GNTT
 *      and LNTT subspaces within the debug space.  Other than these 
 *      fixups, the only other task for the linker is the concatenation
 *      of the debug spaces from separate compilation units.
 */


/*
 * --- 11.  PREPROCESSOR
 */

/*
 *     The preprocessor (PXDB) which must be run on the debug info in
 *     the executable program file massages this debug info so that the
 *     debugger may start up and run more efficiently.  Some of the
 *     tasks performed by PXDB are: remove duplicate global type and
 *     variable information from the GNTT, append the GNTT onto the end
 *     of the LNTT and place both back in the LNTT section, build quick
 *     look-up tables for files, procedures, modules, and paragraphs
 *     (for Cobol), placing these in the GNTT section, and reconstruct
 *     the header appearing in the header section to access this
 *     information.
 *
 *      This post-PXDB header is as follows:
 */

struct  PXDB_header {
        int      pd_entries;   /* # of entries in function look-up table */
        int      fd_entries;   /* # of entries in file look-up table */
        int      md_entries;   /* # of entries in module look-up table */
        BITS     pxdbed : 1;   /* 1 => file has been preprocessed      */
        BITS     bighdr : 1;   /* 1 => this header contains 'time' word */
	BITS     sa_header : 1;/* 1 => created by SA version of pxdb */
			       /*   used for version check in xdb */
#ifdef CPLUSPLUS
	BITS	 inlined: 1;   /* one or more functions have been inlined */
	BITS	 spare:12;
	short    version;      /* pxdb header version */
#else /* CPLUSPLUS */
	BITS	 spare:29;
#endif /* CPLUSPLUS */
        int      globals;      /* index into the DNTT where GNTT begins */
        BITS     time;         /* modify time of file before being pxdbed */
        int      pg_entries;   /* # of entries in label look-up table */
	int	 functions;    /* actual number of functions */
	int	 files;        /* actual number of files */
#ifdef CPLUSPLUS
        int      cd_entries;   /* # of entries in class look-up table */
        int      aa_entries;   /* # of entries in addr alias look-up table */
        int      oi_entries;   /* # of entries in object id look-up table */
#endif
};

#define PXDB_VERSION_CPLUSPLUS	1
#define PXDB_VERSION_7_4	2
#define PXDB_VERSION_CPP_30	3

#define PXDB_VERSION_2_1	1

/*
 *      The structures for the quick look-up tables in the 
 *      post-PXDB GNTT section are:
 */

/*
 *      Source File Descriptor:
 *
 *      An element of the source file quick look-up table
 */

typedef struct FDS {
        long	       isym;		/* first symbol for file	   */
        ADRT	       adrStart;	/* mem adr of start of file's code */
        ADRT	       adrEnd;		/* mem adr of end of file's code   */
        char	      *sbFile;		/* name of source file		   */
        BITS	       fHasDecl: 1;	/* do we have a .d file?	   */
        BITS	       fWarned:  1;	/* have warned about age problems? */
        unsigned short ilnMac;		/* lines in file (0 if don't know) */
        int	       ipd;		/* first proc for file, in PD []   */
        BITS	      *rgLn;		/* line pointer array, if any	   */
} FDR, *pFDR;

/*
 *      Procedure Descriptor:
 *
 *      An element of the procedure quick look-up table
 */

typedef struct PDS {
        long	 isym;		/* first symbol for proc	*/
        ADRT	 adrStart;	/* memory adr of start of proc	*/
        ADRT	 adrEnd;	/* memory adr of end of proc	*/
        char	*sbAlias;	/* alias name of procedure	*/
        char	*sbProc;	/* real name of procedure	*/
        ADRT	 adrBp;		/* address of entry breakpoint  */
        ADRT	 adrExitBp;	/* address of exit breakpoint   */
#ifdef CPLUSPLUS
	int      icd;           /* member of this class         */	
#else /* CPLUSPLUS */
        BITS	 inst;		/* instruction at entry         */
#endif /* CPLUSPLUS */
#ifdef TEMPLATES
	BITS	ipd;		/* index of template for this function */
#else /* TEMPLATES */
        BITS	 instExit;	/* instruction at exit          */
#endif /* TEMPLATES */
#ifdef CPLUSPLUS
#ifdef TEMPLATES
        BITS	 unused:    6;
        BITS	 fTemplate: 1;	/* function template		*/
        BITS	 fExpansion: 1;	/* function expansion		*/
	BITS	 linked	  : 1;	/* linked with other expansions	*/
#else /* TEMPLATES */
        BITS	 unused:    9;
#endif /* TEMPLATES */
	BITS	 duplicate: 1;  /* clone of another procedure   */
	BITS	 overloaded:1;  /* overloaded function          */
	BITS	 member:    1;  /* class member function        */
	BITS	 constructor:1; /* constructor function         */
	BITS	 destructor:1;  /* destructor function          */
	BITS     Static:    1;  /* static function              */
	BITS     Virtual:   1;  /* virtual function             */
	BITS     constant:  1;  /* constant function            */
	BITS     pure:      1;  /* pure (virtual) function      */
	BITS     language:  4;  /* procedure's language         */
	BITS     inlined:   1;  /* function has been inlined    */
	BITS     Operator:  1;  /* operator function            */
	BITS	 stub:      1;  /* bodyless function            */
#else
	BITS     unused1:  18;
	BITS     language:  4;  /* procedure's language         */
	BITS     unused2:   3;
#endif
        BITS	 optimize:  2;	/* optimization level   	*/
        BITS	 level:     5;	/* nesting level (top=0)	*/
} PDR, *pPDR;

/*
 *      Module Descriptor:
 *
 *      An element of the module quick reference table
 */

typedef struct MDS {
        long	 isym;		   /* first symbol for module	*/
        ADRT	 adrStart;	   /* adr of start of mod.	*/
        ADRT	 adrEnd;	   /* adr of end of mod.	*/
        char	*sbAlias;	   /* alias name of module   	*/
        char	*sbMod;		   /* real name of module	*/
        BITS     imports:       1; /* module have any imports?  */
        BITS     vars_in_front: 1; /* module globals in front?  */
        BITS     vars_in_gaps:  1; /* module globals in gaps?   */
        BITS     unused      : 29;
        BITS     unused2;	   /* space for future stuff	*/
} MDR, *pMDR;


/*
 *      Paragraph Descriptor:
 *
 *      An element of the paragraph quick look-up table
 */

typedef struct PGS {
        long     isym;       /* first symbol for label          */
        ADRT     adrStart;   /* memory adr of start of label    */
        ADRT     adrEnd;     /* memory adr of end of label      */
        char    *sbLab;      /* name of label                   */
        BITS     inst;       /* Used in xdb to store inst @ bp  */
        BITS     sect:    1; /* true = section, false = parag.  */
        BITS     unused: 31; /* future use                      */
} PGR, *pPGR;

#ifdef CPLUSPLUS
/*
 *      Class Descriptor:
 *
 *      An element of the class quick look-up table
 */

typedef struct CDS {
        char	 *sbClass;	/* name of class	        */
        long      isym;         /* class symbol (tag)           */
	BITS	  type : 2;	/* 0=class, 1=union, 2=struct   */
#ifdef TEMPLATES
	BITS	  fTemplate : 1;/* class template */
	BITS	  expansion : 1;/* template expansion */
	BITS	  unused    :28;
#else /* TEMPLATES */
	BITS	  unused : 30;
#endif /* TEMPLATES */
	SLTPOINTER lowscope;	/* beginning of defined scope   */
	SLTPOINTER hiscope;	/* end of defined scope         */
} CDR, *pCDR;

/*
 *      Address Alias Entry
 *
 *      An element of the address alias quick look-up table
 */

typedef struct AAS {
	ADRT    low;
	ADRT    high;
	int	index;
	BITS	unused : 31;
	BITS	alternate : 1;	/* alternate unnamed aliases?   */
} AAR, *pAAR;

/*
 *      Object Identification Entry
 *
 *      An element of the object identification quick look-up table
 */

typedef struct OIS {
	ADRT    obj_ident;		/* class identifier         */
	long	isym;			/* class symbol             */
	long    offset;			/* offset to object start   */
} OIR, *pOIR;

#endif /*CPLUSPLUS*/

#if __cplusplus
#undef public
#endif

#endif /* _SYMTAB_INCLUDED */
