/* file sa.c */
/*    SCCS    REV(64.4);       DATE(92/04/03        14:22:24) */
/* KLEENIX_ID @(#)sa.c	64.4 92/02/25 */
/* -*-C-*-
********************************************************************************
*
* File:         sa.c
* RCS:          $Header: sa.c,v 70.3 92/04/03 14:16:23 ssa Exp $
* Description:  XT table building utilities for S300 compilers
* Author:       Jim Wichelman, SES
* Created:      Fri Aug  4 11:39:14 1989
* Modified:     Wed Sep 13 14:20:43 1989 (Jim Wichelman) jww@hpfcjww
* Language:     C
* Package:      N/A
* Status:       Experimental (Do Not Distribute)
*
* (c) Copyright 1989, Hewlett-Packard Company, all rights reserved.
*
********************************************************************************
* 
* NOTES:
*
*   1)  The "xt_index" field in your symbol table entries MUST be 
*       initialized with NULL_XT_INDEX (in "sa.h") when the entry
*       is allocated.
*
*   2)  Your symbol table can not be realloc'ed. I keep a pointer to
*       the symbol entries, so if they move, I'm in trouble.  If this
*       is a problem, we will have to work out another solution.
*
*   3)  You must call "init_xt_tables" to get things rolling.
*
*   4)  You must compile sa.c as a separte module and link it with 
*       your compiler
*
*   5)  sa.c includes:
*
*           #include <symtab.h>
*           #include "vtlib.h"
*
*       You may need to add "-I<dir>" directives to your makefile so 
*       the compiler can find them.
*
*   6)  Your "compiler supplied routines" are included with the statement:
*
*            #include "sa_iface.h"
*
*  7) The following "defines" can be used:
*
*	SA                  - This file will not be processed
*       SA_MACRO 	    - Routines to cross reference cpp macros
*       ANSI		    - ANSI cpp macro support
*       NLS_SUPPORT         - NLS ANSI cpp support 
*
*******************************************************************************/
static char rcs_identity[] = "$Header: sa.c,v 70.3 92/04/03 14:16:23 ssa Exp $";

#ifdef SA

#if( defined IRIF || defined IRIFORT )
#      define IRIForIRIFORT
#endif


/* #define TEST_IT */

#include <sys/types.h> 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "symtab.h"
#include "vtlib.h"


/* Now include the routines provided by the compiler */
/* ------------------------------------------------- */
#ifdef TEST_IT
#include "testfunc.h"
#else
#include "sa_iface.h"
#endif

/* Include sa.h after including sa_iface.h, because 
   of dependency on manifest.  Manifest is indirectly
   included by sa_iface.h through mfile1 */
#include "sa.h"

#ifdef IRIF
       extern dntt_type;    /* LNTT or GNTT [in 'cdbsyms.c'] */
#endif /* IRIF */
/******************************************************************************
*
*		XT buffer management in sa.c
*
*	This briefly describes the method of maintain the tables of XT info
*	for use by Static Analysis.  Information about identifiers in a
*	source program is kept in their own symbol tables during compilation.
*
*	Currently, 3 different tables of items are maintained:
*	    1) Locals   2) Globals   3) Macros
*	The info about all these items are kept in "buffers":  contiguous
*	lists of xrefentrys.
*	Info about items comes into this package from one of two functions:
*	    1) add_xt_info   2) flush_xt_macro, which calls get_macro_info.
*	New items have entries in the proper table and buffers created.
*	Others have the proper info dumped into the existing buffer.
*
*	When info comes in from add_xt_info, need to determine if it is for
*	the local or global table.  This is done by checking the symbol
*	table entry for the item, and determining its scope.
*
*	Macro information is gotten with get_macro_info() from a text
*	file emitted by the preprocessor.  These lines (currently) give
*	locations of macro definitions and uses (also "calls").  These
*	occurences are gathered all at once in get_macro_info, then emitted
*	with flush_xt_macro.
*
*	The info in the buffers should be dumped to the assembler stream at
*       two logical points.  Local buffers are emitted at function end.
*       Global and macro buffers are dumped at file end.
*
*	Certain items for which buffers of info are built do _not_ have an
*	XREF entry in a DNTT at the time they need to get dumped.  Thesne
*	are:   1) Macros  2) Non-static functions, unref. externals, etc.
*	These need to have a K_SA entry emitted _before_ the buffers are
*	dumped to enter_XT().  This is done automatically for the caller.
*
******************************************************************************/



#ifdef SADEBUG

#  define STATDEBUG(args) if (sadebug) printf args

#else

#  define STATDEBUG(args)

#endif 

/* ############ */
/* Global types */
/* ############ */

#ifndef BOOLEAN
#define BOOLEAN int
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


typedef enum {
   Local, Global, Macro
} SCOPE_TYPE;


/* - - - - - -*/
/* Misc stuff */
/* - - - - - -*/

typedef union xrefentry XT_BUFFER;

typedef struct {             /* info about contiguous XT buffer */
   XT_BUFFER  *pos;
   int        length;
} FREE_BUFFER_ENTRY;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* We have an array of these things to keep track of XT info.  There is one  */
/* entry in the array per symbol.  There is one array for globals, one for   */
/* locals, and one for macros.						     */
/*									     */
/* There is a free list of unused items in the arrays pointed to by	     */
/* variables "Xt_free_global_index" and "Xt_free_local_index".  These are    */
/* items interspurced within the existing array.  Such items use existing    */
/* fields for other purposes.  "ordered_list" is the index of the next free  */
/* item. "cur_leng" is ZERO (so we don't try and dump data when we walk      */
/* through the array).							     */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct xt_header_s {
   XT_BUFFER          *buf_start;  /* Ptr to contiguous XT entries */
   unsigned int       cur_end;	   /* Current space allocated for XT entries */
   unsigned int       cur_leng;    /* # of contiguous XT entries */
   int                cur_file;    /* File name vt index for this header */
   int                dntt_emitted; 
#ifdef IRIFORT
   int		      dntt_index;  /* Index into LNTT or GNTT table */
#endif /* IRIFORT */
   GENERIC_STAB_P     stab_pt;      /* Ptr to symbol table entry */
   unsigned int       ordered_list; /* walk this when dumping (array index) */
   unsigned int       label;	    /* Nonzero if symbolic X<label>: */
} XT_HEADER_ENTRY;


typedef struct {
   XT_BUFFER     *buf_start;
   unsigned int  cur_leng;
   unsigned int  cur_end;
   int           cur_file;
   char          *mac_name;
   char          should_flush;
} MACRO_HEADER_ENTRY;

/* ---------------- */
/* Global Variables */
/* ---------------- */

static XT_HEADER_ENTRY    	*Xt_local_table         = 0;
static XT_HEADER_ENTRY    	*Xt_global_table        = 0;

static unsigned int 	 	Xt_local_length         = 0;
static unsigned int 		Xt_local_end            = 0;
static unsigned int     	Xt_free_local_index     = NULL_XT_INDEX;
  
static unsigned int 		Xt_global_length        = 0;
static unsigned int 		Xt_global_end           = 0;
static unsigned int	    	Xt_free_global_index    = NULL_XT_INDEX;

static FREE_BUFFER_ENTRY 	*Free_buffer_table      = 0;

static unsigned int 		Free_table_length       = 0;
static unsigned int 		Free_table_end          = 0;

static int	       		Next_label_id           = 0;
static XREFPOINTER     		Next_xt_index        	= 0;

static unsigned int 		Last_local_ordered_list = NULL_XT_INDEX;
static unsigned int 		Local_ordered_list      = NULL_XT_INDEX;

static unsigned int 		Last_global_ordered_list = NULL_XT_INDEX;
static unsigned int 		Global_ordered_list      = NULL_XT_INDEX;

static MACRO_HEADER_ENTRY 	*Xt_macro_table         = 0;

static unsigned int       	Xt_macro_length   	= 0;
static unsigned int       	Xt_macro_end      	= 0;

FILE         		      	*Macro_fp         	= (FILE *) NULL;
char          		      	*Macro_fn         	= (char *) NULL;

static char 			Empty_link_data[20]     = { 0 };

/* ----------------*/
/* Error constants */
/* --------------- */

#define OUT_OF_MEMORY 1
#define CANT_OPEN_MACRO_FILE 2
#define UNDEFINED_MACRO_REFERENCE 3

/* -------------------------------- */
/* Defines for table size/growth    */
/* -------------------------------- */

#define LOCAL_HEADER_TABLE_STEP   256
#define GLOBAL_HEADER_TABLE_STEP  256
#define MACRO_HEADER_TABLE_STEP   256
#define FREE_TABLE_STEP           256 
#define FREE_TABLE_MAX            (FREE_TABLE_STEP * 1)

#define BUFFER_STEP               10
#define MAX_LINE_SIZE           65536  /* Max line length for xrefshort */

/* -------------------------------- */
/* Macros to add to/grow XT tables. */
/* -------------------------------- */

#define BUFFER_TOO_SHORT(arg)       \
   (arg ->cur_end < arg->cur_leng + 2)

#define LOCAL_TABLE_TOO_SHORT       \
   (Xt_local_end < Xt_local_length + 1)

#define GLOBAL_TABLE_TOO_SHORT      \
   (Xt_global_end < Xt_global_length + 1)

#define MACRO_TABLE_TOO_SHORT      \
   (Xt_macro_end < Xt_macro_length + 1)

#define FREE_TABLE_TOO_SHORT        \
   (Free_table_end < Free_table_length + 1)

#define FREE_TABLE_FULL	        \
   (FREE_TABLE_MAX < Free_table_length + 1)

#ifdef IRIFORT

#define ADD_ARGS(xtable,Xindex,arg1, arg2)        \
   xtable [Xindex].buf_start    = (arg1)->pos;    \
   xtable [Xindex].cur_end      = (arg1)->length; \
   xtable [Xindex].cur_leng     = 0;              \
   xtable [Xindex].cur_file     = 0;              \
   xtable [Xindex].dntt_emitted = 0;              \
   xtable [Xindex].dntt_index	= 0;              \
   xtable [Xindex].stab_pt      = arg2;           \
   xtable [Xindex].ordered_list = NULL_XT_INDEX;     \
   xtable [Xindex].label        = 0;            

#else /* not IRIFORT */

#define ADD_ARGS(xtable,Xindex,arg1, arg2)        \
   xtable [Xindex].buf_start    = (arg1)->pos;    \
   xtable [Xindex].cur_end      = (arg1)->length; \
   xtable [Xindex].cur_leng     = 0;              \
   xtable [Xindex].cur_file     = 0;              \
   xtable [Xindex].dntt_emitted = 0;              \
   xtable [Xindex].stab_pt      = arg2;           \
   xtable [Xindex].ordered_list = NULL_XT_INDEX;     \
   xtable [Xindex].label        = 0;            

#endif /* not IRIFORT */

#define ADD_TO_MACRO_TABLE(arg1,arg2,arg3)    \
      {	\
	Xt_macro_table [(arg3)].buf_start    = (arg1)->pos;    \
	Xt_macro_table [(arg3)].cur_end      = (arg1)->length; \
	Xt_macro_table [(arg3)].cur_leng     =  0;             \
	Xt_macro_table [(arg3)].cur_file     =  0;             \
	Xt_macro_table [(arg3)].mac_name     =  (arg2);        \
	Xt_macro_table [(arg3)].should_flush = 0;              \
      }

/* ------------------------------------ */
/* Types/Defines for Parsing Macro file */
/* ------------------------------------ */

typedef enum { 
    BAD_MACKIND, 
    MACDEF, 
    MACUSE, 
    MACLOC, 
    MACACT, 
    MACEND,        
    MACCALL
} MACKINDS;

#define SAME_FILE    "\"@\""
#define UNDER_SCORE  '_'
#define BLANK        ' '
#define TAB          '\t'
#define LEFT_PAREN   '('
#define CTRL_B       '\002'

#ifdef TEST_IT 
#include "testfunc.c"
#endif


/***********************************************************************\
*
* int set_sadebug(i)
*
* Toggle the internal debugging flag.  Must have been compiled with
* "SADEBUG" defined to do anything.
*
\***********************************************************************/

#ifdef SADEBUG
int set_sadebug(i)
    int i;
{
    int old;
    old = sadebug;
    sadebug = i;
    return old;
}
#endif


/***********************************************************************\
*
* static void panic_handler (cause)
*
\***********************************************************************/

static void panic_handler (cause)
    int cause;
{
    switch (cause) {

    case OUT_OF_MEMORY:
#ifdef FORT
	fprintf (stderr,"\nOut of memory; exiting\n");
#else /* not FORT */
	stderr_fprntf( "\nOut of memory; exiting\n");
#endif /* not FORT */
	exit (1);

    case CANT_OPEN_MACRO_FILE:
#ifdef FORT
	fprintf (stderr,"\nCan't open macro file; exiting\n");
#else /* not FORT */
	stderr_fprntf( "\nCan't open macro file; exiting\n");
#endif /* not FORT */
	exit (1);

    case UNDEFINED_MACRO_REFERENCE:
#ifdef FORT
	fprintf (stderr,"\nUndefined macro reference; exiting\n");
#else /* not FORT */
	stderr_fprntf( "\nUndefined macro reference; exiting\n");
#endif /* not FORT */
	exit (1);

    default:
	break;
    }
}


#ifdef SADEBUG
/***********************************************************************\
*
* static char *fmt_use(use)
*
* Return a formatted 5 char string of the "use" mask passed to us.
*
\***********************************************************************/

static char *fmt_use(use)
   XTKINDS	use;
{
    static char use_buff[6];
    
    sprintf(use_buff, "%c%c%c%c%c",
	    use & DEFINITION   ? 'd' : '-',
	    use & DECLARATION  ? 'D' : '-',
	    use & MODIFICATION ? 'M' : '-',
	    use & USE          ? 'U' : '-',
	    use & XT_CALL      ? 'C' : '-');

    return(use_buff);
}
#endif

/***********************************************************************\
*
* static char *format_label(header, s)
*
* Return a string with "label".  The caller does NOT own the string.
*
\***********************************************************************/
static char *format_label(header,s)
    XT_HEADER_ENTRY	*header;
    char		*s;
{
    static char label_buff[40];
    
    sprintf(label_buff, "X%d%s", header->label, s);
    return label_buff;
}


/***********************************************************************\
*
* static void expand_xt_buffer (header, incr)
*
* Uses realloc to expand the xt buffer for an item.
*
\***********************************************************************/

static void expand_xt_buffer (header, incr)
    XT_HEADER_ENTRY        *header;
    unsigned int	   incr;
{
    XT_BUFFER 	  *buffer;		/* For use by sizeof operator */
    unsigned int  newsize;
    
    newsize = (incr == 0) ? (header->cur_end * 2) : (header->cur_end + incr);
        
    header->buf_start = (XT_BUFFER *) 
              realloc ((char *) header->buf_start, (newsize * sizeof (*buffer)));

    if (! header->buf_start)
	panic_handler (OUT_OF_MEMORY, 0, 0, 0, 0);

    header->cur_end = newsize;
    /*   printf("expand_xt_buffer: cur_end = %d\n", header->cur_end);*/
}


/***********************************************************************\
*
* static void expand_free_table ()
*
* Uses realloc to expand the Free_buffer_table.
*
\***********************************************************************/

static void expand_free_table ()
{
    Free_buffer_table = (FREE_BUFFER_ENTRY *) 
	realloc ((char *) Free_buffer_table, (Free_table_end + 
					      FREE_TABLE_STEP) * sizeof (FREE_BUFFER_ENTRY));
    if (! Free_buffer_table)
	panic_handler (OUT_OF_MEMORY, 0, 0, 0, 0);
    Free_table_end += FREE_TABLE_STEP;
}


/***********************************************************************\
*
* static void expand_local_table ()
*
* Uses realloc to expand the Xt_local_table.
*
\***********************************************************************/

static void expand_local_table ()
{
    Xt_local_table = (XT_HEADER_ENTRY *) 
	realloc ((char *) Xt_local_table, (Xt_local_end + 
					   LOCAL_HEADER_TABLE_STEP) * sizeof (XT_HEADER_ENTRY));
    if (! Xt_local_table)
	panic_handler (OUT_OF_MEMORY, 0, 0, 0, 0);
    Xt_local_end += LOCAL_HEADER_TABLE_STEP;
}


/***********************************************************************\
*
* static void expand_global_table ()
*
* Uses realloc to expand the Xt_global_table.
*
\***********************************************************************/

static void expand_global_table ()
{
    Xt_global_table = (XT_HEADER_ENTRY *) 
	realloc ((char *) Xt_global_table, (Xt_global_end + 
					    GLOBAL_HEADER_TABLE_STEP) * sizeof (XT_HEADER_ENTRY));
    if (! Xt_global_table)
	panic_handler (OUT_OF_MEMORY, 0, 0, 0, 0);
    Xt_global_end += GLOBAL_HEADER_TABLE_STEP;
}


/***********************************************************************\
*
* unsigned int next_free_table_index(is_global)
*
* Return the index of the next free item in the array of header 
* entries (global or local).  We first check a linked list of
* entries that were freed by "move_xt_info" before we grab one
* at the end of the table.
*
\***********************************************************************/
unsigned int next_free_table_index(is_global)
    int	is_global;
{
    unsigned int	index;
    
    if (is_global)
	if (Xt_free_global_index != NULL_XT_INDEX)
	{
	    index = Xt_free_global_index;
	    Xt_free_global_index = Xt_global_table[index].ordered_list;
	    Xt_global_table[index].ordered_list = NULL_XT_INDEX;
	}
	else
	{
	    if (GLOBAL_TABLE_TOO_SHORT)
		expand_global_table ();

	    index = Xt_global_length++;
	}
    else
	if (Xt_free_local_index != NULL_XT_INDEX)
	{
	    index = Xt_free_local_index;
	    Xt_free_local_index = Xt_local_table[index].ordered_list;
	    Xt_local_table[index].ordered_list = NULL_XT_INDEX;
	}
	else
	{
	    if (LOCAL_TABLE_TOO_SHORT) 
		expand_local_table (); 
	    
	    index = Xt_local_length++;
	}
    
    return(index);
}

/***********************************************************************\
*
* void free_a_table_index(is_global, index)
*
* An item has been moved from the global to local table (or visa versa).
* We now have a free index.  Hook it onto a linked list of free indicies
* so we can reuse them.
*
\***********************************************************************/

void free_a_table_index(is_global, index)
    int			is_global;
    unsigned int	index;
{
    XT_HEADER_ENTRY     *header;

    if (is_global)
    {
	header = &Xt_global_table[index];
	header->ordered_list = Xt_free_global_index;
	Xt_free_global_index = index;
    }
    else
    {
	header = &Xt_local_table[index];
	header->ordered_list = Xt_free_local_index;
	Xt_free_local_index  = index;
    }


    header->stab_pt      = 0;
    header->cur_leng     = 0;
    header->buf_start    = 0;
    header->cur_end      = 0;
}

/***********************************************************************\
*
*  static void free_remaining_buffers()
*
\***********************************************************************/

static void free_remaining_buffers()
{
    register int	i;
    
    if (Free_table_length)
	for (i = 0; i < Free_table_length; i++)
	    free(Free_buffer_table[i].pos);

    Free_table_length = 0;
}

    
/***********************************************************************\
*
*  static void add_buffer_to_free_table(header, force_free)
*
\***********************************************************************/

static void add_buffer_to_free_table(header, force_free)
    XT_HEADER_ENTRY	*header;
    int			force_free;
    
{
    if ( (FREE_TABLE_FULL) || (force_free) )
    {
	free(header->buf_start); 
    }
    else
    {
	if (FREE_TABLE_TOO_SHORT)
	    expand_free_table ();

	Free_buffer_table [Free_table_length].pos = header->buf_start; 
	Free_buffer_table [Free_table_length].length = header->cur_end;
	Free_table_length++;
    }

    header->cur_leng     = 0;
    header->buf_start    = 0;
    header->cur_end      = 0;
}


/***********************************************************************\
*
*  static FREE_BUFFER_ENTRY *get_buffer ()
*
* Returns an available buffer for xt info; new or used (pre_owned).
*
\***********************************************************************/

static FREE_BUFFER_ENTRY *get_buffer ()
{
    static FREE_BUFFER_ENTRY  buf_entry;
    XT_BUFFER 		      *buffer;

    if (Free_table_length == 0) {
	buffer = (XT_BUFFER *) malloc (BUFFER_STEP * sizeof (* buffer));
	if (! buffer)
	    panic_handler (OUT_OF_MEMORY, 0, 0, 0, 0);
	buf_entry.pos = buffer;
	buf_entry.length = BUFFER_STEP;
	/*      fprintf(stderr, "malloc buffer: %d\n", BUFFER_STEP);*/
	return (&buf_entry);
    }
    /*   fprintf(stderr, "return buffer: %d\n", Free_buffer_table[Free_table_length].length);*/
    return &Free_buffer_table [--Free_table_length];
}


/***********************************************************************\
*
* static void add_xrefname (header, filename_vtindex)
*
* Puts a new XREFNAME struct into the buffer for an item.
*
\***********************************************************************/

static void add_xrefname (header, filename_vtindex)
    XT_HEADER_ENTRY  *header;
    int              filename_vtindex;
{
    XT_BUFFER   *buffer;
        
    STATDEBUG (("add_xrefname: %s, level = %d, filename = %d\n",
		sym_name(header->stab_pt), sym_scope_level(header->stab_pt),
		filename_vtindex));
    
    if (BUFFER_TOO_SHORT (header))
	expand_xt_buffer (header, 0);
    
    buffer = header->buf_start;
    buffer [header->cur_leng].xfname.tag = XNAME;
    buffer [header->cur_leng].xfname.filename = filename_vtindex;

    
    /* Keep using filename, to be consistent with what ccom stores */
    /* as its current filename. 				   */
    /* ----------------------------------------------------------- */
    header->cur_file = filename_vtindex;
    header->cur_leng++;
}


/***********************************************************************\
*
* static XT_HEADER_ENTRY *start_xt_buffer (symp, is_global)
*
* Starts a new xt buffer for an item.  Also fills the buffer with
* the first XREFNAME.
*
\***********************************************************************/

static XT_HEADER_ENTRY *start_xt_buffer (symp, is_global)
    GENERIC_STAB_P	symp;
    int	      		is_global;
    
{
#ifndef FLINT
    FREE_BUFFER_ENTRY   *buf_entry;
    XT_HEADER_ENTRY     *header;
    unsigned int	index;
    
    STATDEBUG (("start_xt_buffer: sname = %s, symp = 0x%x, is_global = %s\n",
	    sym_name(symp), symp, is_global ? "TRUE" : "FALSE"));

    buf_entry = get_buffer();
    index = next_free_table_index(is_global);
    sym_set_xt_index(symp, index);
        
    if (is_global) 
    {
	ADD_ARGS(Xt_global_table, index, buf_entry, symp);
	header = &Xt_global_table[index];
    }
    else 
    {
	ADD_ARGS(Xt_local_table, index, buf_entry, symp);
	header = &Xt_local_table[index];
    }
    
    add_xrefname (header, cdbfile_vtindex);
    return header;
#endif /* FLINT */
}


/***********************************************************************\
*
* void add_xt_info(symp, use)
*
*  Puts a new XREFINFO struct into the XT buffer for an item.
*  Puts a new XREFNAME if the filename changed.
*
*  If "use" is non-zero, we add an entry for the specified use.  If
*  its zero, we will NOT add any such entry.
*
\***********************************************************************/

void add_xt_info (symp, use)
    GENERIC_STAB_P	symp;
    XTKINDS      	use;
{
#ifndef FLINT
    XT_HEADER_ENTRY *header;
    XT_BUFFER       *buffer;
    unsigned int    length;

    int	            is_global;
    int             scope_level;
    unsigned int    index;
    int	            symbol_lineno;

    /* get info from the symbol table entry and from the compiler */
    /* ---------------------------------------------------------- */
    is_global 	   = sym_is_global(symp);
    scope_level	   = sym_scope_level(symp);
    symbol_lineno  = sym_lineno(symp);
    index          = sym_xt_index(symp);
    
    
    STATDEBUG (("add_xt_info: %s, %s, symp = 0x%x, lineno = %d, use lineno = %d,\n level = %d, file = %s\n",
		sym_name(symp),
		fmt_use(use),
		symp,
		lineno,
		symbol_lineno,
		scope_level,
		satitle));
    
#ifdef SADEBUG   
/*
    if (sadebug && symbol_lineno != lineno)
	printf("current lineno %d not equal to symbol_lineno %d for %s\n", 
	       lineno, symbol_lineno, sym_name(symp));
*/
#endif   
    /* If first time we are adding an XT reference, create new entry */
    /* ------------------------------------------------------------- */
    if (index == NULL_XT_INDEX)
	header = start_xt_buffer(symp, is_global);
    else 
	/* Get a ptr to the existing XT table entry */
	/* ---------------------------------------- */
	header = (is_global) ? &Xt_global_table [index] : &Xt_local_table [index];
    
    if (BUFFER_TOO_SHORT (header))
	expand_xt_buffer (header, 0);

    /* Good place to add fast strcmp! */      
    /* ------------------------------ */
    if (cdbfile_vtindex != header->cur_file)
	add_xrefname (header, cdbfile_vtindex);

    if (use == NULL_USE) return;
        
    buffer = header->buf_start;
    length = header->cur_leng;
    
    if (symbol_lineno > MAX_LINE_SIZE) {    /* use xreflong & xrefline */
	buffer [length].xreflong.tag          = XINFO2;
	buffer [length].xreflong.definition   = use & DEFINITION ? 1 : 0;
	buffer [length].xreflong.declaration  = use & DECLARATION ? 1 : 0;
	buffer [length].xreflong.modification = use & MODIFICATION ? 1 : 0;
	buffer [length].xreflong.use          = use & USE ? 1 : 0;
	buffer [length].xreflong.call         = use & XT_CALL ? 1 : 0;
	buffer [length].xreflong.column       = 0;
	length++;
	buffer [length].xrefline.line         = symbol_lineno;
	header->cur_leng += 2;
    } else {					/* use xrefshort */
	buffer [length].xrefshort.tag          = XINFO1;
	buffer [length].xrefshort.definition   = use & DEFINITION ? 1 : 0;
	buffer [length].xrefshort.declaration  = use & DECLARATION ? 1 : 0;
	buffer [length].xrefshort.modification = use & MODIFICATION ? 1 : 0;
	buffer [length].xrefshort.use          = use & USE ? 1 : 0;
	buffer [length].xrefshort.call         = use & XT_CALL ? 1 : 0;
	buffer [length].xrefshort.column       = 0;
	buffer [length].xrefshort.line         = symbol_lineno;
	header->cur_leng += 1;
    }
#endif /* not FLINT */
}

#ifdef FORT
/***********************************************************************\
*
* void move_xt_info(from_symp, to_symp)
*
*  Append any XT information associated with "from_symp" to any data
*  associated with "to_symp".  The, free up all data associated with 
*  the "from" entry in the XT tables.
*
*  WARNING: Both "from_symp" and "to_symp" are assumed to have
*           been entered in to the XT tables with "add_xt_info".
*           (with possibly no (NULL_USE) "use").
*
*  If "from_symp" equals "to_symp", we just return.
*
\***********************************************************************/

void move_xt_info(from_symp, to_symp)
    GENERIC_STAB_P	from_symp;
    GENERIC_STAB_P	to_symp;
{
    XT_HEADER_ENTRY 	*from_header;
    XT_HEADER_ENTRY 	*to_header;
    int			from_global;
    int			to_global;
    unsigned int 	from_index;
    unsigned int 	to_index;

    if (from_symp == to_symp) return;
            
    from_index = sym_xt_index(from_symp);
    to_index =   sym_xt_index(to_symp);

    /* Both must have been entered in our tables */
    /* ----------------------------------------- */
    if ( (from_index == NULL_XT_INDEX) || (to_index == NULL_XT_INDEX) )
    {
	fprintf(stderr, "OLDSYM='%s'  NEWSYM='%s'\n", 
		sym_name(from_symp),
		sym_name(to_symp));
	fatal("move_xt_info: symbol(s) not previously entered in XT table");
	exit(1);
    }
    
    /* Get ptr to the existing XT table entries */
    /* ---------------------------------------- */
    from_global = sym_is_global(from_symp);
    to_global   = sym_is_global(to_symp);

    from_header = (from_global) ? &Xt_global_table [from_index] 
                                : &Xt_local_table [from_index];

    to_header   = (to_global) ? &Xt_global_table [to_index] 
                              : &Xt_local_table [to_index];


    /* See if the "to" buffer has enough space for all of the */
    /* data we will be appending to it.                       */
    /* ------------------------------------------------------ */
    if (to_header->cur_end < to_header->cur_leng + from_header->cur_leng)
	expand_xt_buffer(to_header, from_header->cur_end);
    

    /* Now copy the data from one buffer to the other. */
    /* ----------------------------------------------- */
    {
        register unsigned int i,j,len;
	register XT_BUFFER    *to_buf, *from_buf;

	len      = from_header->cur_leng;
	to_buf   = to_header->buf_start;
	from_buf = from_header->buf_start;

	/* Don't stuff two duplicate "FNAME" entries together. */
	/* --------------------------------------------------- */
	i = 0;
	j = to_header->cur_leng;
	to_header->cur_leng += from_header->cur_leng;

	if ( (j) && ( to_buf[j-1].xfname.tag == XNAME ) )
	    if (to_buf[j-1].xrefline.line == (int)from_buf[i].xrefline.line) 
	    {
		i++;
		to_header->cur_leng--;
	    }
		
	/* Do the copy */
	/* ----------- */
	for (; (i < len); i++, j++)
	    to_buf[j] = from_buf[i];
    }
    
    /* Free up the now unused entry in the "from" table. */
    /* ------------------------------------------------- */
    add_buffer_to_free_table(from_header, FALSE);
    free_a_table_index(from_global, from_index);
}

#else /* !FORT */

/***********************************************************************\
*
* void change_xt_table(symp, old_is_global, new_is_global)
*
* Allocates a new entry in the xt table, copies header 
* info from the old to the new, frees the old.
*
\***********************************************************************/
void change_xt_table(symp, old_is_global, new_is_global) 
GENERIC_STAB_P symp;
int old_is_global, new_is_global;

{
   unsigned int new_index;
   XT_HEADER_ENTRY *old, *new;
   if (sym_xt_index(symp) != NULL_XT_INDEX && 
       old_is_global != new_is_global) {
      new_index = next_free_table_index(new_is_global);
      new = (new_is_global) ? &Xt_global_table[new_index]
	 : &Xt_local_table[new_index];
      old = (old_is_global) ? &Xt_global_table[sym_xt_index(symp)]
	 : &Xt_local_table[sym_xt_index(symp)];
      *new = *old;
      free_a_table_index(old_is_global, sym_xt_index(symp));
      sym_set_xt_index(symp, new_index);
   }
 /* else, either entered into an xt table, or the move
    would be within the same table, so don't do anything */
}
   
#endif /* FORT */

/***********************************************************************\
*
* XREFPOINTER increment_xt_index(cnt)
*
* Increment the global XTPOINTER variable by "cnt".  Returns the new
* value.
*
\***********************************************************************/

/* This is really an XREFPOINTER function even in FORT, but sa.h declares
 * it as long as a convenience for not including "symtab.h", and the
 * Domain compiler complains about the XREFPOINTER<->long difference.
 */
#ifdef FORT
long increment_xt_index(int cnt)
#else
XREFPOINTER increment_xt_index(cnt)
#endif
{
    return (Next_xt_index += cnt);
}

/***********************************************************************\
*
* XREFPOINTER current_xt_index()
*
* Returns the current value of the global XTPOINTER counter.
*
\***********************************************************************/

#ifdef FORT
long current_xt_index()
#else
XREFPOINTER current_xt_index()
#endif
{
    return (Next_xt_index);
}

/***********************************************************************\
*
* static void add_to_local_ordered_list(index)
*
* Add the "index" entry onto the chain of header entries that are
* ready to be dumped.  The entries MUST be written in the order
* specified by the chain.
*
\***********************************************************************/

static void add_to_local_ordered_list(index)
    unsigned int  index;
{
    if (Last_local_ordered_list != NULL_XT_INDEX) 
	Xt_local_table[Last_local_ordered_list].ordered_list = index;
    else 
	Local_ordered_list = index;

    Last_local_ordered_list = index;

}

/***********************************************************************\
*
* static void add_to_global_ordered_list(index)
*
* Add the "index" entry onto the chain of header entries that are
* ready to be dumped.  The entries MUST be written in the order
* specified by the chain.
*
\***********************************************************************/

static void add_to_global_ordered_list(index)
    unsigned int  index;
{
    if (Last_global_ordered_list != NULL_XT_INDEX)
	Xt_global_table[Last_global_ordered_list].ordered_list = index;
    else
	Global_ordered_list = index;
    
    Last_global_ordered_list = index;
}

/***********************************************************************\
*
* XREFPOINTER compute_xt_index(symp)
*
* Use the "xt_index" stored in the the symp to get at the block
* of XT data associated with the symbol.  Return the current
* value of the global XTPOINTER.  We are now committed to write
* this block of XT data at that starting at that index.
* Then, increment the global XTPOINTER values by the number of XT 
* so that this data is accounted for.
* 
\***********************************************************************/

#ifdef IRIFORT
XREFPOINTER compute_xt_index(symp,dntt)
    GENERIC_STAB_P	symp;
    int			dntt;

#else /* not IRIFORT */
XREFPOINTER compute_xt_index(symp)
    GENERIC_STAB_P	symp;
#endif /* not IRIFORT */

{
    XT_HEADER_ENTRY *header;
    int	            is_global;
    unsigned int    index;
    XREFPOINTER     curr;

    index          = sym_xt_index(symp);
    is_global 	   = sym_is_global(symp);

    /* Get a ptr to the existing XT table entry */
    /* ---------------------------------------- */
    header = (is_global) ? &Xt_global_table [index] : &Xt_local_table [index];

    header->dntt_emitted = TRUE;
#ifdef IRIFORT
    header->dntt_index = dntt;
#endif /* IRIFORT */

    /* Increment the global XT index.  Remember to add a one more for */
    /* the empty LINK field that is output after each block of data   */
    /* -------------------------------------------------------------- */
    curr = Next_xt_index;
    increment_xt_index(header->cur_leng + 1);

    if (is_global)
	add_to_global_ordered_list(index);
    else
	add_to_local_ordered_list(index);
    
    return (curr);
}

#ifndef IRIFORT

/***********************************************************************\
*
* char *compute_symbolic_xt_index(symp)
*
* Use the "xt_index" stored in the the symp to get at the block
* of XT data associated with the symbol.  We don't know how many
* XT entries there will ultimately be, so we can't compute the 
* actual XT index.  Instead, we will create a label that this
* group of data can be refered to by.  
*
* The returned string is owned by THIS routine.
*
\***********************************************************************/

char *compute_symbolic_xt_index(symp)
    GENERIC_STAB_P	symp;
{
    XT_HEADER_ENTRY *header;
    int	            is_global;
    unsigned int    index;

    index          = sym_xt_index(symp);
    is_global 	   = sym_is_global(symp);

    /* Get a ptr to the existing XT table entry */
    /* ---------------------------------------- */
    header = (is_global) ? &Xt_global_table [index] : &Xt_local_table [index];

    header->dntt_emitted = TRUE;
    header->label        = ++Next_label_id;

    return (format_label(header, (char *)0 ));
}

#endif /* NOT IRIFORT */

#ifdef SADEBUG
/***********************************************************************\
*
* static void dump_buffer (header)
*
*	Provides a formatted dump of local/global buffers before
*	flushing.
*
\***********************************************************************/

static void dump_buffer (header)
    XT_HEADER_ENTRY  *header;
{
    XT_BUFFER *buffer;
    int       i;
   
    buffer = header->buf_start;
    printf("File: %d; Buffer length: %d; Buffer end: %d; \n     sname: %s; Label: %d; Ordered List: 0x%x\n",
	   header->cur_file, 
	   header->cur_leng, 
	   header->cur_end, 
	   sym_name(header->stab_pt),
	   header->label,
	   header->ordered_list);

    for (i = 0; i < header->cur_leng; i++) {
	switch (buffer [i].xfname.tag) {
	case XINFO1:
	    printf ("\txrefshort--\n\t\ttag: %d; %c%c%c%c%c; line: %d; col: %d\n",
		    buffer [i].xrefshort.tag,
		    buffer [i].xrefshort.definition   ? 'd' : '-',
		    buffer [i].xrefshort.declaration  ? 'D' : '-',
		    buffer [i].xrefshort.modification ? 'M' : '-',
		    buffer [i].xrefshort.use          ? 'U' : '-',
		    buffer [i].xrefshort.call         ? 'C' : '-',
		    buffer [i].xrefshort.line,
		    buffer [i].xrefshort.column);
	    break;
	case XINFO2:
	    printf ("\txreflong--\n\t\ttag: %d;  %c%c%c%c%c; col: %d\n",
		    buffer [i].xreflong.tag,
		    buffer [i].xreflong.definition   ? 'd' : '-',
		    buffer [i].xreflong.declaration  ? 'D' : '-',
		    buffer [i].xreflong.modification ? 'M' : '-',
		    buffer [i].xreflong.use          ? 'U' : '-',
		    buffer [i].xreflong.call         ? 'C' : '-',
		    buffer [i].xreflong.column);
	    break;
	case XLINK:
	    printf ("\txlink--\n\t\ttag: %d; next: %d\n",
		    buffer [i].xlink.tag,
		    buffer [i].xlink.next);
	    break;
	case XNAME:
	    printf ("\txfname--\n\t\ttag: %d; filename: %d\n",
		    buffer [i].xfname.tag,
		    buffer [i].xfname.filename);
	    break;
	default:
	    printf("\txrefline--\n\t\tline: %d\n",
		   buffer [i].xrefline.line);
	    break;
	}
    }
}
#endif /* SADEBUG */


/***********************************************************************\
*
* static void emit_K_SA (header, scope)
*
*  Puts out a "generic" DNTT_SA, and a DNTT_XREF for an item
*  without any debug information.
*
\***********************************************************************/

#ifdef IRIFORT

static emit_K_SA (header, scope, k_type)
    XT_HEADER_ENTRY     *header;
    SCOPE_TYPE 		scope;
    KINDTYPE		k_type;
    
{

    STATDEBUG (("emit_K_SA\n"));

    (void)dnttentry( K_SA, k_type, sym_name(header->stab_pt), 1 );
    return dnttentry( K_XREF, current_xt_index(), 0, 1 );

}

#else /* not IRIFORT */

static void emit_K_SA (header, scope, k_type)
    XT_HEADER_ENTRY     *header;
    SCOPE_TYPE 		scope;
    KINDTYPE		k_type;
    
{
#ifndef IRIF
    VTPOINTER	     vtidx;
    char	     fmt_buff[60];

#define K_SA_fmt        "\tdnt_sa\t\t%d, %d\n" 
#define K_XREF_fmt	"\tdnt_xref\t%d, %d\n" 
#define GNTT_start_fmt  "\n\tgntt\n"
#define XT_start_fmt    "\n\txt\n"
        
    STATDEBUG (("emit_K_SA\n"));

    vtidx = add_to_vt(sym_name(header->stab_pt), TRUE, TRUE);
    
    /* Switch to GNTT in the assembler stream */
    /* -------------------------------------- */
    write_to_as_stream(GNTT_start_fmt); 
    
    sprintf(fmt_buff, K_SA_fmt, k_type, vtidx);
    write_to_as_stream(fmt_buff);
    
#ifdef FORT
    sprintf(fmt_buff, K_XREF_fmt, LANG_HPF77, current_xt_index());
#else
    sprintf(fmt_buff, K_XREF_fmt, LANG_C, current_xt_index());
#endif /* FORT */
    write_to_as_stream(fmt_buff);

    write_to_as_stream(XT_start_fmt); 

#else /* IRIF */

    int save_type = dntt_type;

    STATDEBUG (("emit_K_SA\n"));

    dntt_type = GNTT;
    (void)dnttentry( K_SA, k_type, header->stab_pt );
    header->stab_pt->symbolic_id = 
	 dnttentry( K_XREF, current_xt_index(), FALSE );
    dntt_type = save_type;
    return;

#endif /* IRIF */
}

#endif	/* not IRIFORT */


/***********************************************************************\
*
*  static void flush_buffer (header, scope)
*
* Gets an XT buffer.  Write its contents via enter_XT.
* Puts out an XREFLINK struct for the item.
*
\***********************************************************************/

static int flush_buffer (header, scope)
    XT_HEADER_ENTRY 	*header;
    SCOPE_TYPE 		scope;
{
    char		toa_buf[20];
    register unsigned 	int i, len, eol;
    register XT_BUFFER  *buf;
    register KINDTYPE   k_type;    
#ifdef IRIFORT
    int			gntt = header->dntt_index;
#endif /* IRIFORT */
    
    if (! header->dntt_emitted)
	if (scope == Global)
	{
#ifdef FORT
	    /* Check for invalid types some day?? */
	    /* ---------------------------------- */
	    k_type = sym_base_kind(header->stab_pt);
#ifdef IRIFORT
	    gntt = emit_K_SA (header, scope, k_type);
#else /* not IRIFORT */
	    emit_K_SA (header, scope, k_type);
#endif /* not IRIFORT */

#else /* not FORT */

	    /* For C, we only dumped referenced data.  So there */
	    /* may be lots of symbols with no DNTT emitted that */
            /* should not have one emitted.  The expection is   */
	    /* FUNC's, who always need one emitted.  If the user*/
            /* invoked the compiler with +Y (dump ALL info),    */
            /* the compiler guarantees all those symbols will   */
	    /* have dntt info emitted, so there is no need to   */
	    /* worry about that here.                           */
	    /* ------------------------------------------------ */
	    if (sym_is_function(header->stab_pt))
		emit_K_SA (header, scope, K_FUNCTION);
	    else
	    {
#ifdef SADEBUG	 
		if (sadebug)
		    fprintf(stderr, "XT: Have global object: %s with no dntt emitted.  No XT data generated\n",
			    sym_name(header->stab_pt));
#endif /* SADEBUG */
		return(FALSE);
	    }

#endif /* FORT */
	}
	else 
	{
#ifdef SADEBUG	   
	   if (sadebug)
	    fprintf(stderr,"XT: Have local object: %s, with no dntt emitted.  No XT data generated.\n",
		    sym_name(header->stab_pt));
#endif	    
	    return(FALSE);
	}

    
#ifdef SADEBUG   
    if (sadebug)  dump_buffer (header);
#endif   

#ifdef IRIFORT

    /* Use the ucif procedure uc_xt() to dump out this group   */
    /* of XT entries.                                          */
    /* ------------------------------------------------------- */

    len = 4*(header->cur_leng / 4);
    buf = header->buf_start;
    for (i = 0; i < len; i += 4)
    {
	uc_xt( 4, buf + i, gntt );
    }
    len = header->cur_leng;
    if( len % 4 ) {
	uc_xt( len % 4, buf + 4*(len/4), gntt );
    }

    /* Write out empty link data XT entry */
    {
	union xrefentry xref;

	xref.xlink.tag  = XLINK;
	xref.xlink.next = 0;
	uc_xt( 1, &xref, gntt );
    }
#else /* not IRIFORT */

#ifdef IRIF
    
    ir_xt( header->cur_leng, header->buf_start, header->stab_pt->symbolic_id );

#else /* not IRIF */

    /* Use the assembler pseudo-op "xt_block" to dump out this */
    /* group of XT entries.  The "xt" pseudo-op has already    */
    /* been written.  Be sure to emit a label if needed.       */
    /* ------------------------------------------------------- */

#define XT_block_fmt           "\txt_block\t"
#define XT_block_label_fmt     ":\txt_block\t"
#define XT_block_continue_fmt  "\n\txt_block\t"

    if (header->label)
	write_to_as_stream(format_label(header, XT_block_label_fmt));
    else
	write_to_as_stream(XT_block_fmt);
    

    len = header->cur_leng;
    buf = header->buf_start;
    for (i = 0; i < len; i++)
    {
	/* Five words to a line (to fit in 80 columns) */	
	/* ------------------------------------------- */
	eol = (!((i+1) % 5));
	sprintf(toa_buf, "0x%x%s", buf[i], (eol) ? "" : ",");
	write_to_as_stream(toa_buf);
	
	if (eol)
	    write_to_as_stream(XT_block_continue_fmt);
    }

    /* Write the final "xt_link 0" data */
    /* -------------------------------- */
    write_to_as_stream(Empty_link_data);

#endif /* not IRIF */
#endif /* not IRIFORT */

    return(TRUE);
}

/***********************************************************************\
*
* void flush_xt_local ()
*
*  Dumps all current entries in the local XT table.  This assumes
*  that compute_[symbolic]_xt_index has been called for each of
*  the symbols that is being dumped.
*
\***********************************************************************/

void flush_xt_local ()
{
    int 		i;
    XT_HEADER_ENTRY 	*header;
    
    STATDEBUG (("flush_xt_local: \n"));

#ifndef IRIForIRIFORT
    write_to_as_stream(XT_start_fmt);
#endif /* !IRIForIRIFORT */

    while (Local_ordered_list != NULL_XT_INDEX)
    {	
	STATDEBUG (("\n\t\tLocal buffer dump:\n"));
	header = &Xt_local_table[Local_ordered_list];
	flush_buffer (header, Local);
	Local_ordered_list = header->ordered_list;
    }

    /* We just dumped the ones that did not have    */
    /* symbolic labels.  Run through the list again */
    /* and dump those with symbolic labels.         */
    /* -------------------------------------------- */
    for (i = 0; i < Xt_local_length; i++) {
	header = &Xt_local_table [i];

	if (header->cur_leng)
	    if ( (header->label) || (!header->dntt_emitted) )
	    { 
		STATDEBUG (("\n\t\tLocal buffer dump:\n"));
		if (flush_buffer (header, Local))

		   /* Account for XT entries written.  Don't forget the */
		   /* empty LINK entry that was written.                */
		   /* ------------------------------------------------- */
		   increment_xt_index(header->cur_leng + 1);
	    }
	
	if (header->buf_start)
	    add_buffer_to_free_table (header, FALSE); /* Free all entries */
        header->stab_pt      = 0;
        header->ordered_list = NULL_XT_INDEX;
    }

    Xt_local_length         = 0;		/* Empty out the local table */
    Local_ordered_list      = NULL_XT_INDEX;
    Last_local_ordered_list = NULL_XT_INDEX;
    Xt_free_local_index     = NULL_XT_INDEX;
}

/***********************************************************************\
*
* void flush_xt_global ()
*
*  Dumps all current entries in the global XT table.  This assumes
*  that compute_[symbolic]_xt_index has been called for each of
*  the symbols that is being dumped.
*
\***********************************************************************/

void flush_xt_global()
{
    int 		i;
    XT_HEADER_ENTRY 	*header;
    
    STATDEBUG (("flush_xt_global: \n"));

#ifndef IRIForIRIFORT
    write_to_as_stream(XT_start_fmt);
#endif /* !IRIForIRIFORT */
        
    while (Global_ordered_list != NULL_XT_INDEX)
    {	
	STATDEBUG (("\n\t\tGlobal buffer dump:\n"));
	header = &Xt_global_table[Global_ordered_list];
	flush_buffer (header, Global);
	Global_ordered_list = header->ordered_list;
    }
    
    for (i = 0; i < Xt_global_length; i++) {
	header = &Xt_global_table [i];

	if (header->cur_leng)
	    if ( (header->label) || (!header->dntt_emitted) ) { 
		STATDEBUG (("\n\t\tGlobal buffer dump:\n"));
		if (flush_buffer (header, Global))

		   /* Account for XT entries written.  Don't forget the */
		   /* empty LINK entry that was written.                */
		   /* ------------------------------------------------- */
		   increment_xt_index(header->cur_leng + 1);
	    }
		    
	if (header->buf_start)
	    add_buffer_to_free_table (header, TRUE);
        header->stab_pt = 0;
    }

    free_remaining_buffers();
}

#ifdef SA_MACRO
/***********************************************************************\
*
* static void expand_macro_table ()
*
* Uses realloc to expand the Xt_macro_table.
*
\***********************************************************************/

static void expand_macro_table ()
{
    Xt_macro_table = (MACRO_HEADER_ENTRY *) 
	realloc ((char *) Xt_macro_table, (Xt_macro_end + 
					   MACRO_HEADER_TABLE_STEP) * sizeof (MACRO_HEADER_ENTRY));
    if (! Xt_macro_table)
	panic_handler (OUT_OF_MEMORY, 0, 0, 0, 0);
    Xt_macro_end += MACRO_HEADER_TABLE_STEP;
}


/***********************************************************************\
*
* static void expand_macro_buffer (header)
*
* Uses realloc to expand the macro buffer for an item.
*
\***********************************************************************/

static void expand_macro_buffer (macro_header)
    MACRO_HEADER_ENTRY 	*macro_header;
{
    XT_BUFFER *buffer;		/* For use by sizeof operator */

    macro_header->buf_start = (XT_BUFFER *) 
	realloc ((char *) macro_header->buf_start, 
		 (macro_header->cur_end + BUFFER_STEP) * sizeof (*buffer));
    if (! macro_header->buf_start)
	panic_handler (OUT_OF_MEMORY, 0, 0, 0, 0);
    macro_header->cur_end += BUFFER_STEP;
}

/***********************************************************************\
*
* static char *strsave (pc)
*
\***********************************************************************/

static char *strsave (pc)
    char 	*pc;
{
    char *s;
    
    if ((s = (char *) malloc (strlen (pc) * sizeof (char))) == 0)
	panic_handler (OUT_OF_MEMORY, 0, 0, 0, 0);
    return strcpy (s, pc);
}


/***********************************************************************\
*
* static void add_XREFNAME_macro (index, filename_vtindex)
*
* Puts a new XREFNAME struct into the buffer for an item.
* For Macro items only.
*
\***********************************************************************/

static void add_XREFNAME_macro (index, filename_vtindex)
    unsigned int 	index;
    int  		filename_vtindex;
{
    MACRO_HEADER_ENTRY 	*macro_header;
    XT_BUFFER 		*buffer;
            
    
    STATDEBUG (("add_XREFNAME_macro: %s, index = %d, filename = %d\n",
		Xt_macro_table[index].mac_name, index, filename_vtindex));
    
    macro_header = &Xt_macro_table [index];

    if (BUFFER_TOO_SHORT (macro_header))
	expand_macro_buffer(macro_header);
    
    buffer = macro_header->buf_start;
    buffer [macro_header->cur_leng].xfname.tag = XNAME;
    buffer [macro_header->cur_leng].xfname.filename = filename_vtindex;
        

    macro_header->cur_file = filename_vtindex;
    macro_header->cur_leng++;


    /* (void) search_filelist(Macro_f, filename_vtindex, TRUE); */
}   

/***********************************************************************\
*
* static unsigned int insert_macro (name, index)
*
*	Inserts the item into the Xt_macro_table at the (approximate)
*	location of the index.  The algorithm is:
*
*		1. If the first item, insert in index 0.
*		2. If the item is "higher" than the indexed item,
*		   bump the index.
*		3. Move all the items from the high end down to the index
*		   up by one slot.
*		4. Insert the item into the index position.
*		5. Return the index position.
*
\***********************************************************************/

static unsigned int insert_macro (name, index)
    char  *name;
    int   index;
{
    int 	      i;
    FREE_BUFFER_ENTRY *buf_entry;

    STATDEBUG (("insert_macro\n"));
    buf_entry = get_buffer();
    
    if (MACRO_TABLE_TOO_SHORT)
	expand_macro_table();
    
    if (Xt_macro_length == 0) {              /* First one in */
	ADD_TO_MACRO_TABLE (buf_entry, strsave(name), 0);
    }
    else {
	if (strcmp (name, Xt_macro_table [index].mac_name) > 0)
	    index++;
	for (i = Xt_macro_length - 1; i >= index; i--)
	    Xt_macro_table [i+1] = Xt_macro_table [i];
	ADD_TO_MACRO_TABLE (buf_entry, strsave (name), index);
    }
    
    Xt_macro_length++;
    return index;
}


/***********************************************************************\
*
*  static unsigned int search_macro_table (name)
*
*	The items in the Xt_macro_table are kept in alphabetic order,
*	in order to allow binary searches for them.  The algorithm is:
*
*		1. Do a binary search of the table for the item.
*		2. If found, return the index for the item.
*		3. If not, call insert_macro() to insert the item
*		   by moving all "higher" items up one, then inserting.
*
\***********************************************************************/

static unsigned int search_macro_table (name)
    char *name;
{
    register int lower, upper, index;
    int value;
    
    STATDEBUG (("search_macro_table\n"));
    
    if (Xt_macro_length == 0)     /* First one in */
	return insert_macro (name, 0);
    lower = 0;
    upper = Xt_macro_length - 1;
    
    for (;;) {
	if (upper < lower)
	    return insert_macro (name, index);
	index = (lower + upper) / 2;
	if ((value = strcmp (name, Xt_macro_table [index].mac_name)) == 0)
	    return index;
	if (value < 0)
	    upper = index - 1;
	else
	    lower = index + 1;
    }
}


/***********************************************************************\
*
* static unsigned int get_macro_index (name, use)
*
* Pretty much a dummy function at this point; calls search_macro_table
* to get the index for name.
*
\***********************************************************************/

static unsigned int get_macro_index (name, use)
    char      *name;
    MACKINDS  use;
{
    int index = 0;
    
    STATDEBUG (("get_macro_index\n"));

    switch (use) {
    case (MACDEF):
    case (MACUSE):
    case (MACCALL):
	index = search_macro_table (name);       
	break;
    case (MACACT):
    case (MACLOC):
    case (MACEND):                              
    default: 
	panic_handler (UNDEFINED_MACRO_REFERENCE,0,0,0,0);
	break;
    } 
    return index;
}

/***********************************************************************\
*
* static void add_macro_info (macname, kind, filename_vtindex, line, col)
*
* This reads the (opened) macro file and gets the lines out one by
* one.  For each line, the info is put into the Xt_macro_table
* after the proper index into the table is found with get_macro_index
*
\***********************************************************************/

static void add_macro_info (macname, kind, filename_vtindex, line, col)
    char 	*macname;
    char 	kind;
    int 	filename_vtindex;
    int 	line, col;
{  
    int 		index;
    MACRO_HEADER_ENTRY  *header;
    XT_BUFFER 		*buffer;
    unsigned int 	length;
    MACKINDS 		use;
    
    STATDEBUG (("add_macro_info: name=%s; file: %d; use=%s; (L,C)=%d,%d\n",
	        macname, filename_vtindex, use == MACDEF ? "DEF" : "USE/CALL",
		line, col));
    
    switch (kind) {
    case ('D'):
    case ('d'): use = MACDEF;	break;
    case ('U'):
    case ('u'): use = MACUSE;	break;
    case ('C'):
    case ('c'): use = MACCALL;  break;
    default:    use = MACEND;	break;
    }
    
    index = get_macro_index (macname, use);
    header = &Xt_macro_table [index];
    
    if (BUFFER_TOO_SHORT (header))    /* In-line code here; different header */
	expand_macro_buffer(header);

    if (header->cur_file != filename_vtindex) 
	add_XREFNAME_macro(index, filename_vtindex);

   if (!Allflag && (header->cur_file == comp_unit_vtindex))
      header->should_flush = TRUE;
	    
    buffer = header->buf_start;
    length = header->cur_leng;
    
    if (line > MAX_LINE_SIZE) {    /* use xreflong & xrefline */
	buffer [length].xreflong.tag          = XINFO2;
	buffer [length].xreflong.definition   = (use == MACDEF);
	buffer [length].xreflong.declaration  = (use == MACDEF);
	buffer [length].xreflong.modification = (use == MACDEF);
	buffer [length].xreflong.use          = (use == MACUSE);
	buffer [length].xreflong.call         = (use == MACCALL);
	buffer [length].xreflong.column       = col;
	length++;
	buffer [length].xrefline.line         = line;
	header->cur_leng += 2;
    } else {					/* use xrefshort */
	buffer [length].xrefshort.tag          = XINFO1;
	buffer [length].xrefshort.definition   = (use == MACDEF);
	buffer [length].xrefshort.declaration  = (use == MACDEF);
	buffer [length].xrefshort.modification = (use == MACDEF);
	buffer [length].xrefshort.use          = (use == MACUSE);
	buffer [length].xrefshort.call         = (use == MACCALL);
	buffer [length].xrefshort.column       = col;
	buffer [length].xrefshort.line         = line;
	header->cur_leng += 1;
    }
}
    
#ifdef SADEBUG
/***********************************************************************\
*
*static void dump_macro_buffer (header)
*
*	Provides a formatted dump of the contents of a macro buffer of
*	information.  Only called if debug is turned on.
*
\***********************************************************************/
static void dump_macro_buffer (header)
    MACRO_HEADER_ENTRY  *header;
{
    XT_BUFFER *buffer;
    int       i;
   
    buffer = header->buf_start;
    printf("Macro: %s; File: %d; Buffer length: %d; Buffer end: %d\n",
	   header->mac_name, header->cur_file,
	   header->cur_leng, header->cur_end);
    for (i = 0; i < header->cur_leng; i++) {
	switch (buffer [i].xfname.tag) {
	case XINFO1:
	    printf ("\txrefshort--\n\t\ttag: %d; %c%c%c%c%c; line: %d; col: %d\n",
		    buffer [i].xrefshort.tag,
		    buffer [i].xrefshort.definition   ? 'd' : '-',
		    buffer [i].xrefshort.declaration  ? 'D' : '-',
		    buffer [i].xrefshort.modification ? 'M' : '-',
		    buffer [i].xrefshort.use          ? 'U' : '-',
		    buffer [i].xrefshort.call         ? 'C' : '-',
		    buffer [i].xrefshort.line,
		    buffer [i].xrefshort.column);
	    break;
	case XINFO2:
	    printf ("\txreflong--\n\t\ttag: %d;  %c%c%c%c%c; col: %d\n",
		    buffer [i].xreflong.tag,
		    buffer [i].xreflong.definition   ? 'd' : '-',
		    buffer [i].xreflong.declaration  ? 'D' : '-',
		    buffer [i].xreflong.modification ? 'M' : '-',
		    buffer [i].xreflong.use          ? 'U' : '-',
		    buffer [i].xreflong.call         ? 'C' : '-',
		    buffer [i].xreflong.column);
	    break;
	case XLINK:
	    printf ("\txlink--\n\t\ttag: %d; next: %d\n",
		    buffer [i].xlink.tag,
		    buffer [i].xlink.next);
	    break;
	case XNAME:
	    printf ("\txfname--\n\t\ttag: %d; filename: %d\n",
		    buffer [i].xfname.tag,
		    buffer [i].xfname.filename);
	    break;
	default:
	    printf("\txrefline--\n\t\tline: %d\n",
		   buffer [i].xrefline.line);
	    break;
	}
    }
}
#endif /* SA DEBUG */


/***********************************************************************\
*
*  static void flush_macro_buffer (index)
*
*	Emits information for a single macro buffer
*
\***********************************************************************/

#ifdef IRIFORT

static void flush_macro_buffer (index)
    int index;
{
    MACRO_HEADER_ENTRY  *macro_header;
    VTPOINTER		vtidx;
    char		fmt_buff[60];
    char		toa_buf[20];
    int			gntt;

    register unsigned int i, len, eol;
    register XT_BUFFER    *buf;
    
        
    macro_header = &Xt_macro_table [index];

    /* For macros entirely in a header file, don't emit anything */
    /* --------------------------------------------------------- */
    if (!macro_header->should_flush && !Allflag) {
	
# ifdef SADEBUG   
	if (sadebug)      
	    printf("Not outputting macro %s\n", macro_header->mac_name);
# endif      
	
	return;
    }

    vtidx = add_to_vt(macro_header->mac_name, TRUE, TRUE);

    (void)dnttentry( K_MACRO, vtidx );
    gntt = dnttentry( K_XREF, current_xt_index(), 0, 1 );

    STATDEBUG (("\n\t\tMacro buffer dump:\n"));

#ifdef SADEBUG
    if (sadebug) dump_macro_buffer (macro_header);
#endif 

    len = 4*(macro_header->cur_leng/4);
    buf = macro_header->buf_start;
    for (i = 0; i < len; i += 4)
    {
	uc_xt( 4, buf + i, gntt );
    }
    len = macro_header->cur_leng;
    if( len % 4 ) {
	uc_xt( len % 4, buf + 4*(len/4), gntt );
    }

    /* Write out empty link data XT entry */
    {
	union xrefentry xref;

	xref.xlink.tag  = XLINK;
	xref.xlink.next = 0;
	uc_xt( 1, &xref, gntt );
    }

    /* Account for XT entries written.  Don't forget the */
    /* empty LINK entry that was written.                */
    /* ------------------------------------------------- */
    increment_xt_index(len + 1);
}       

#else /* not IRIFORT */

static void flush_macro_buffer (index)
    int index;
{
    MACRO_HEADER_ENTRY  *macro_header;

#ifndef IRIF
    VTPOINTER		vtidx;
    char		fmt_buff[60];
    char		toa_buf[20];

    register unsigned int i, len, eol;
    register XT_BUFFER    *buf;
#else /* IRIF */
    int save_type;
    int symbolic_id;
#endif /* IRIF */    
        
    macro_header = &Xt_macro_table [index];

    /* For macros entirely in a header file, don't emit anything */
    /* --------------------------------------------------------- */
    if (!macro_header->should_flush && !Allflag) {
	
# ifdef SADEBUG   
	if (sadebug)      
	    printf("Not outputting macro %s\n", macro_header->mac_name);
# endif      
	
	return;
    }

#ifndef IRIF
    vtidx = add_to_vt(macro_header->mac_name, TRUE, TRUE);

    /* Switch to GNTT in the assembler stream */
    /* -------------------------------------- */
    write_to_as_stream(GNTT_start_fmt); 

    sprintf(fmt_buff, K_SA_fmt, K_MACRO, vtidx);
    write_to_as_stream(fmt_buff);
    
    sprintf(fmt_buff, K_XREF_fmt, LANG_C, current_xt_index());
    write_to_as_stream(fmt_buff);

    write_to_as_stream(XT_start_fmt); 

#else /* IRIF */

    save_type = dntt_type;
    dntt_type = GNTT;
    (void)dnttentry( K_MACRO, macro_header->mac_name );
    symbolic_id = dnttentry( K_XREF, current_xt_index(), FALSE );
    dntt_type = save_type;

#endif /* IRIF */
    
    STATDEBUG (("\n\t\tMacro buffer dump:\n"));

    
#ifdef SADEBUG
    if (sadebug) dump_macro_buffer (macro_header);
#endif 

#ifdef IRIF

    ir_xt( macro_header->cur_leng, macro_header->buf_start, symbolic_id );

#else /* not IRIF */

    write_to_as_stream(XT_block_fmt);
    

    len = macro_header->cur_leng;
    buf = macro_header->buf_start;
    for (i = 0; i < len; i++)
    {
	/* Five words to a line (to fit in 80 columns) */	
	/* ------------------------------------------- */
	eol = (!((i+1) % 5));
	sprintf(toa_buf, "0x%x%s",  buf[i], (eol) ? "" : ",");
	write_to_as_stream(toa_buf);
	
	if (eol)
	    write_to_as_stream(XT_block_continue_fmt);
    }
    
    write_to_as_stream(Empty_link_data);

#endif /* not IRIF */

    /* Account for XT entries written.  Don't forget the */
    /* empty LINK entry that was written.                */
    /* ------------------------------------------------- */
    increment_xt_index( macro_header->cur_leng + 1);
}       

#endif /* not IRIFORT */


/***********************************************************************\
*
* extern void flush_xt_macro()
*
* Gets each macro Entry in the symbol table and processes it. 
* Each buffer must have a K_SA and K_XREF put out first.      
*
\***********************************************************************/

void flush_xt_macro() 
{
    int index;
    
    STATDEBUG (("flush_xt_macro: \n"));
    
#ifndef ANSI
    if (Macro_fp != (FILE *) NULL)
	get_macro_file_info();
#endif
    for (index = 0; index < Xt_macro_length; index++)
	flush_macro_buffer (index);

    /* flush_filelist(); */
}

#ifndef ANSI
/***********************************************************************\
*
* void sa_macro_file_open(filename)
*       Opens the macro filename as specified by the #pragma MACROFILE.
*       Doesn't call cerror because can still compile and
*       the most likely cause for failure is compiling
*       an old .i file long after the temp for macros
*       has been deleted.
*       Only call if saflag is set.  (For non-ANSI uses only.??)
*
\***********************************************************************/
void sa_macro_file_open(filename)
  char * filename;
{
if ((Macro_fp = fopen (filename, "r")) == NULL)
  uerror ("Can't open -y macro file: %s",filename);
Macro_fn = strsave(filename);
}

/***********************************************************************\
*
* void sa_macro_file_remove()
*
\***********************************************************************/
void sa_macro_file_remove()
{
if (Macro_fp)
  {
  fclose(Macro_fp);
  unlink(Macro_fn);
  }
}

/***********************************************************************\
*
* void get_macro_file_info()
*
* This reads lines of the macro_file and sends the appropriate
* info to add_macro_info.
*
\***********************************************************************/

void get_macro_file_info()
{
    char 	*fp,kind;
    static char macname [150], filename [400];
    int 	line, col, filename_vtindex, len;
    
    while (fscanf (Macro_fp, "%c %s %s %d %d\n", 
		   &kind, macname, filename, &line, &col) != EOF)
    {
	if (strcmp (filename, SAME_FILE)) 
	    { /* a file switch, get vtindex of new file */
	    fp = filename;
	    fp++; /* skip over "" and ./ */
	    if ( fp[0] == '.' && fp[1] == '/' ) fp +=2;
	    fp[strlen(fp) - 1] = '\0';
	    filename_vtindex = add_to_vt(fp, TRUE, TRUE);
	    }

	add_macro_info (macname, kind, filename_vtindex, line, col);
    }
}
#endif /* ! ANSI */

#ifdef ANSI
/***********************************************************************\
*
*  static void pick_up_macro_USEs (p, end, file_vt, line)
*
*	This routine picks up USEs of macros in various cases:
*	as arguments to other macro invocations, in #ifdef lines
*	and other conditional compilation directives, and in #define
*	macro defintion directives.
*
\***********************************************************************/

static void pick_up_macro_USEs (p, end, file_vt, line, col)
    char *p,*end;
    int  line,col,file_vt;
{   
    register char *q;
    char 	  *buffer;
    char          use,savech;

    while (p != end)
    {
	if (*p == CTRL_B)
	{                                /* Hit a USEd or CALLed macro        */
	    buffer = ++p;                /* First character of macro          */
	    while (isalnum (*p) || (*p == UNDER_SCORE))
		p++;                     /* Get all characters of macro       */
	    q = p;                       /* Leave p at end of token */
	    while ((*q == BLANK || *q == CTRL_D) && q < end)
		q++;
	    use = (*q == LEFT_PAREN) ? 'C' : 'U';
	    savech = *p;
	    *p = NULL;
	    add_macro_info (buffer, use, file_vt, line, col);
	    *p = savech;
        }
	else if (*p == CTRL_D)
	{
	    ++p;
	    ++line;
	    col = 1;
	}	
	else
    	{
	    ++p;
	    ++col;
	}
    }
}

/***********************************************************************\
*
* void do_Scanner_macro_use (buffer, file_vt, line, col)
*
*	This is used with ANSI implementations.  Called from the
*	Scanner, it sends macro USE information picked up from
*	the ANSI cpp in the form of ^ABC type input.  This routine
*	must call add_macro_info with the right stuff.
*
*	This is also called if conditional compilation text is
*	encountered.
*
\***********************************************************************/

void do_Scanner_macro_use (buffer, file_vt, line, col)
    char *buffer;
    int  line, col, file_vt;
{
    char 	   use;
    char	   *end;
    register char  *p = buffer;
   
    end = buffer + strlen (buffer);   /* Set end at the trailing null */
    
    if ((*(p+1) != UNDER_SCORE) && (!isalpha (*(p+1))))
	{ /* this is a marked directive, skip over the mark */
	p++;
	}
    pick_up_macro_USEs (p, end, file_vt, line, col);
}

/***********************************************************************\
*
*  void do_Scanner_macro_define (buffer, file_vt, line)
*
*	This is used with ANSI implementations.  Called from the
*	Scanner, it sends macro DEFINE information picked up from
*	the ANSI cpp in the form of ^ABC type input.  This routine
*	must call add_macro_info with the right stuff.
*
\***********************************************************************/
void do_Scanner_macro_define (buffer, file_vt, line)
    char *buffer;
    int  line, file_vt;
{
    int 	  col;
    char 	  savec,use;
    char	  *end;
    register char *p = buffer;
   
    end = buffer + strlen (buffer);   /* Set end at the trailing null */
    
    if (strncmp (buffer, "#define", 7))
	return;                /* Something is wrong... Should be a #define */
    p += 7;                   /* Advance p past the "#define" */
    col = 8;
    while (*p == BLANK || *p == TAB || *p == CTRL_D)
	{
	if (*p == CTRL_D)
	    {
	    line++;
	    col = 1;
	    }
	else
	    col++;
	p++;
	}
    buffer = p;
    while (isalnum(*p) || (*p == UNDER_SCORE))
	p++;
    if (p == buffer) return;   /* Nothing there */
    savec = *p;
    *p = NULL;
    use = 'D';
    add_macro_info (buffer, use, file_vt, line, col);
    *p = savec;
    col += (p-buffer);
 
    /*  Sent the DEFINEd macro to add_...().  Now check for
     *  ^B in the rest of the buffer.  These signify that the
     *  following string is a nested macro within the body.
     */
    
    pick_up_macro_USEs (p, end, file_vt, line, col);
}

#ifdef NLS_SUPPORT
/***********************************************************************\
*
*  void do_Scanner_macro_use_NLS (buffer, file_vt, line, col)
*
*	This is used with ANSI implementations, when NLS is desired.
*	It is analogous to the above routine, but "buffer" may contain
*	multibyte characters somewhere inside.  Only looking
*	for macro USEs/CALLs, so just look at each int in turn, 
*	convert to a char, and put into a buffer to send to add_...().
*
\***********************************************************************/

void do_Scanner_macro_use_NLS (buffer, file_vt, line, col)
    int  *buffer;
    int  line, col, file_vt;
{
    char p, use;
    char send_buf [512];    /* Horrible; use for now... */
    int  index = 0;
   
    send_buf [index] = p = (char) buffer [index];
    if (p != UNDER_SCORE && ! isalpha (p))
	return;  /* For now; not a USE/CALL of a macro */
    
    while (isalnum(p) || (p == UNDER_SCORE)) {
	index++;
	send_buf [index] = p = (char) buffer [index];
    }
    if (p == NULL)
	use = 'U';
    else {
	use = 'C';
	send_buf [index] = NULL;
    }
    add_macro_info (send_buf, use, file_vt, line, col);
}

/***********************************************************************\
*
* void do_Scanner_macro_define_NLS (buffer, file_vt, line, col)
*
*	This is used with ANSI implementations, when NLS is desired.
*	It is analogous to the above routine, but "buffer" may contain
*	multibyte characters somewhere inside.  Only looking
*	for macro DEFs, so just look at each int in turn, 
*	convert to a char, and put into a buffer to send to add_...().
*
\***********************************************************************/

void do_Scanner_macro_define_NLS (buffer, file_vt, line, col)
    int  *buffer;
    int  line, col, file_vt;
{
    char p, use;
    char send_buf [512];    /* Horrible; use for now... */
    char *start;
    int  i, index = 0;
   
    for (index = 0; index < 8; index++)   /* Fill send_buf with "define" */
	send_buf [index] = p = (char) buffer [index];
    
    if (strncmp (send_buf, "#define", 7))
	return;                            /* Something wrong... */
    while (p == BLANK || p == TAB) {       /* Get past the "define" */
	index++;
	send_buf [index] = p = (char) buffer [index];
    }
    start = &send_buf [index];            /* Start of macro being defined */
    while (isalnum(p) || (p == UNDER_SCORE)) {
	index++;
	send_buf [index] = p = (char) buffer [index];
    }
    if (start == &send_buf [index])       /* Nothing there */
	return;
    use = 'D';
    send_buf [index] = NULL;
    
    add_macro_info (start, use, file_vt, line, col);
}
#endif /* NLS_SUPPORT */
#endif /* ANSI */


/***********************************************************************\
*
* void init_xt_macro_table ()
*
* Initializes the Xt_macro_table.
*
\***********************************************************************/

extern unsigned int xt_global_end;   

void init_xt_macro_table ()
{
    Xt_macro_table = (MACRO_HEADER_ENTRY *)
	malloc (MACRO_HEADER_TABLE_STEP * sizeof (MACRO_HEADER_ENTRY));

    if (!Xt_macro_table)
	panic_handler (OUT_OF_MEMORY, 0, 0, 0, 0);

    Xt_macro_end = MACRO_HEADER_TABLE_STEP;

    /* Initialize the filelists too. */
    /* ---------------------------- */
    /*
    macro_f = (struct filelist *) malloc (FILELIST_STEP * 
					  sizeof (struct filelist));
    total_f = (struct filelist *) malloc (FILELIST_STEP * 
					  sizeof (struct filelist));

    if (macro_f == (struct filelist *) NULL ||
	total_f == (struct filelist *) NULL)
	panic_handler (OUT_OF_MEMORY, 0, 0, 0, 0);
    */
    
}
#endif /* SA_MACRO */

/***********************************************************************\
*
* void init_xt_tables()
*
* Initializes the tables and global size variables.
* This should be called by the compiler during its initialization
* code if static analysis is turned on.
*
\***********************************************************************/

void init_xt_tables ()
{
   STATDEBUG (("init_xt_tables\n"));

   Xt_local_table = (XT_HEADER_ENTRY *) 
		malloc (LOCAL_HEADER_TABLE_STEP * sizeof (XT_HEADER_ENTRY));
   Xt_global_table = (XT_HEADER_ENTRY *) 
		malloc (GLOBAL_HEADER_TABLE_STEP * sizeof (XT_HEADER_ENTRY));
   Free_buffer_table = (FREE_BUFFER_ENTRY *) 
		malloc (FREE_TABLE_STEP * sizeof (FREE_BUFFER_ENTRY));

   if (! (Xt_local_table && Xt_global_table && Free_buffer_table))
      panic_handler (OUT_OF_MEMORY, 0, 0, 0, 0);

   Xt_local_end   = LOCAL_HEADER_TABLE_STEP;
   Xt_global_end  = GLOBAL_HEADER_TABLE_STEP;
   Free_table_end = FREE_TABLE_STEP;
#ifdef SA_MACRO 
   init_xt_macro_table();
#endif /* SA_MACRO */

    /* Go to the work of creating the bit string for "xt_link 0" so */
    /* that if the bit fields change we don't have problems with a  */
    /* a hard coded string. 					    */
    /* ------------------------------------------------------------ */
    {
	union xrefentry xref;

	xref.xlink.tag  = XLINK;
	xref.xlink.next = 0;
        sprintf(Empty_link_data,"0x%x\n", xref.xrefline.line);
    }
}


#endif /* SA */
