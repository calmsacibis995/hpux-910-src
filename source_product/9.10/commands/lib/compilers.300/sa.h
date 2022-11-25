/* file sa.h */
/*    SCCS    REV(64.4);       DATE(92/04/03        14:22:26) */
/* KLEENIX_ID @(#)sa.h	64.4 91/10/09 */
/* -*-C-*-
********************************************************************************
*
* File:         sa.h
* RCS:          $Header: sa.h,v 70.3 92/04/03 14:16:58 ssa Exp $
* Description:  Types and visible procedure for the XT utilities
* Author:       Jim Wichelman, SES
* Created:      Tue Aug  8 15:40:37 1989
* Modified:     Thu Aug 10 13:12:53 1989 (Jim Wichelman) jww@hpfcjww
* Language:     C
* Package:      N/A
* Status:       Experimental (Do Not Distribute)
*
* (c) Copyright 1989, Hewlett-Packard Company, all rights reserved.
*
********************************************************************************
*/
#ifndef GENERIC_STAB_P_DEFINED
#define GENERIC_STAB_P_DEFINED
typedef union CombinedSymtab *GENERIC_STAB_P;
#endif
/* ####################################################### */
/* These defines are for specifying how a symbol is "used" */
/* to the "add_xt_info" routines.  It is simply a bitmask  */
/* indicating the type of "use".                           */
/* ####################################################### */

#ifndef _SA_INCLUDED
#define _SA_INCLUDED

#define XTKINDS      int

#define NULL_USE     0x0
#define DEFINITION   0x1
#define DECLARATION  0x2
#define MODIFICATION 0x4
#define USE          0x8
#define XT_CALL      0x10

#define NULL_XT_INDEX 0xffffffff

#define CTRL_A       '\001'
#define CTRL_B       '\002'
#define CTRL_C       '\003'
#define CTRL_D       '\004'

extern flag sadebug;
#if __STDC__
extern void init_xt_tables(void);
extern void init_xt_macro_table(void);
extern unsigned int next_free_table_index(int is_global);
extern void free_a_table_index(int is_global,unsigned int index);
extern void do_Scanner_macro_use(char *buffer,int file_vt,int line,int col);
extern void do_Scanner_macro_define(char *buffer,int file_vt,int line);
extern void do_Scanner_macro_use_NLS(char *buffer,int file_vt,int line,int col);
extern void do_Scanner_macro_define_NLS(char *buffer,int file_vt,int line,int col);
#else
extern void init_xt_tables();
extern void init_xt_macro_table();
extern unsigned int next_free_table_index();
extern void free_a_table_index();
extern void do_Scanner_macro_use();
extern void do_Scanner_macro_define();
extern void do_Scanner_macro_use_NLS();
extern void do_Scanner_macro_define_NLS();
#endif
/***********************************************************************\
*
* void init_xt_tables()
*
* Initializes the tables and global size variables.
* This should be called by the compiler during its initialization
* code if static analysis is turned on.
*
\***********************************************************************/
#if __STDC__
extern int set_sadebug(int i);
#else
extern int set_sadebug();
#endif
/***********************************************************************\
*
* int set_sadebug(i)
*
* Toggle the internal debugging flag.  Must have been compiled with
* "SADEBUG" defined to do anything.
*
\***********************************************************************/
#if __STDC__
extern void unknown_func_add_xt_info(void);
#else
extern void unknown_func_add_xt_info();
#endif
/***********************************************************************\
*
* void unknown_func_add_xt_info (use)
*
*  unknown_func_add_xt_info is a specialized version of add_xt_info for
*  recording function calls of the form "(*pt_to_func)()", where any TERM
*  can evaluate to a function.  A specialized routine is needed since the
*  symbol __Unknown_function is not actually entered in the symbol table of
*  ccom.
*
\***********************************************************************/
#if __STDC__
extern void add_xt_info();
#else
extern void add_xt_info();
#endif
/***********************************************************************\
*
* void add_xt_info(symp, use)
*
*
*  Puts a new XREFINFO struct into the XT buffer for an item.
*  Puts a new XREFNAME if the filename changed.
*
*  If "use" is non-zero, we add an entry for the specified use.  If
*  its zero, we will NOT add any such entry.
*
\***********************************************************************/
#if __STDC__
extern void move_xt_info(GENERIC_STAB_P from_symp,GENERIC_STAB_P to_symp);
extern void change_xt_table(void *symp,int old_is_global,int new_is_global);
#else
extern void move_xt_info();
extern void change_xt_table();
#endif
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


#ifdef FORT
/* I don't want to include symtab.h everywhere I include sa.h (Don) */
#if __STDC__
extern long increment_xt_index(int cnt);
#else
extern long increment_xt_index();
#endif
#else
extern XREFPOINTER increment_xt_index();
#endif /* FORT */
/***********************************************************************\
*
* XREFPOINTER increment_xt_index(cnt)
*
* Increment the global XTPOINTER variable by "cnt".  Returns the new
* value.
*
\***********************************************************************/

#ifdef FORT
/* I don't want to include symtab.h everywhere I include sa.h (Don) */
#if __STDC__
extern long current_xt_index(void);
#else
extern long current_xt_index();
#endif
#else
extern XREFPOINTER current_xt_index();
#endif /* FORT */
/***********************************************************************\
*
* XREFPOINTER current_xt_index()
*
* Returns the current value of the global XTPOINTER counter.
*
\***********************************************************************/

#ifdef FORT
/* I don't want to include symtab.h everywhere I include sa.h (Don) */
#if __STDC__
#ifdef IRIFORT
extern long compute_xt_index(GENERIC_STAB_P symp,int dntt);
#else /* IRIFORT */
extern long compute_xt_index(GENERIC_STAB_P symp);
#endif /* IRIFORT */
#else /* __STDC__ */
extern long compute_xt_index();
#endif
#else
extern XREFPOINTER compute_xt_index();
#endif /* FORT */
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
#if __STDC__
char *compute_symbolic_xt_index(GENERIC_STAB_P symp);
#else
char *compute_symbolic_xt_index();
#endif
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
#if __STDC__
extern void flush_xt_local(void);
#else
extern void flush_xt_local();
#endif
/***********************************************************************\
*
* void flush_xt_local()
*
*  Dumps all current entries in the local XT table.  This assumes
*  that compute_[symbolic]_xt_index has been called for each of
*  the symbols that is being dumped.
*
\***********************************************************************/

#if __STDC__
extern void flush_xt_global(void);
#else
extern void flush_xt_global();
#endif
/***********************************************************************\
*
* void flush_xt_global()
*
*  Dumps all current entries in the global XT table.  This assumes
*  that compute_[symbolic]_xt_index has been called for each of
*  the symbols that is being dumped.
*
\***********************************************************************/

/* ################################################ */
/* These are used for processing the cpp macros.    */
/* ################################################ */
#if __STDC__
extern void flush_xt_macro(void);
#else
extern void flush_xt_macro();
#endif
/***********************************************************************\
*
* extern void flush_xt_macro()
*
* Gets each macro Entry in the symbol table and processes it. 
* Each buffer must have a K_SA and K_XREF put out first.      
*
\***********************************************************************/

#ifndef ANSI
#if __STDC__
extern void sa_macro_file_open(char *filename);
#else
extern void sa_macro_file_open();
#endif
/***********************************************************************\
*
* sa_macro_file_open (filename)
*
*	Opens the macro filename as specified by the #pragma MACROFILE.
*
\***********************************************************************/
#if __STDC__
extern void sa_macro_file_remove(void);
#else
extern void sa_macro_file_remove();
#endif
/***********************************************************************\
*
* sa_macro_file_open (filename)
*
*	Closes and removes the file as specified by the #pragma MACROFILE.
*
\***********************************************************************/
#if __STDC__
extern void get_macro_file_info(void);
#else
extern void get_macro_file_info();
#endif

/***********************************************************************\
*
* void get_macro_file_info()
*
* This reads lines of the macro_file and sends the appropriate
* info to add_macro_info.
*
\***********************************************************************/

/***********************************************************************\
*
* sa_macro_file
*
*	Returns true if have a macro file open.  
*       Won't be true unless saflag is set.
*
\***********************************************************************/
extern FILE *Macro_fp;
#define sa_macro_file (Macro_fp)
#endif /* !ANSI */
#endif /* _SA_INCLUDED */
