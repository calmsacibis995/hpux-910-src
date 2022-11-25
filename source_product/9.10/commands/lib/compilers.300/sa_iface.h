/* file sa_iface.h */
/*    SCCS    REV(64.2);       DATE(92/04/03        14:22:26) */
/* KLEENIX_ID @(#)sa_iface.h	64.2 91/10/09 */
/* -*-C-*-
/***********************************************************************\
*
* File:         locfunc.c
* RCS:          $Header: sa_iface.h,v 70.3 92/04/03 14:17:26 ssa Exp $
* Description:  
* Author:       Jim Wichelman, SES
* Created:      Tue Aug  8 11:20:08 1989
* Modified:     Wed Aug  9 13:06:47 1989 (Jim Wichelman) jww@hpfcjww
* Language:     C
* Package:      N/A
* Status:       Experimental (Do Not Distribute)
*
* (c) Copyright 1989, Hewlett-Packard Company, all rights reserved.
*
************************************************************************/

#include "mfile1"
#ifndef GENERIC_STAB_P_DEFINED
#define GENERIC_STAB_P_DEFINED
#define GENERIC_STAB_P struct symtab *
#endif
/***********************************************************************\
*
* char *sym_name(symp)
*
* Given a symbol table entry pointer, return a pointer to the symbol name
*
***********************************************************************/
# define sym_name(symp) (symp->sname)

/***********************************************************************\
*
* int sym_is_global(symp)
*
* Given a symbol table entry pointer, return TRUE if the symbol is a 
* global symbol, FALSE otherwise.
*
***********************************************************************/
#define sym_is_global(symp) (symp->slevel == 0)

/***********************************************************************\
*
* int sym_lineo(symp)
*
* Given a symbol table entry pointer, return the line number for the 
* last reference to the symbol.
*
***********************************************************************/
#define sym_lineno(symp) (symp->suse)

/***********************************************************************\
*
* int sym_scope_level(symp)
*
* Given a symbol table entry pointer, return its scoping level
*
***********************************************************************/
#define sym_scope_level(symp) (symp->slevel)

/***********************************************************************\
*
* unsigned int sym_xt_index(symp)
*
* Given a symbol table entry pointer, return the index into the xt
* table that is stored there.
*
* Compiler MUST initialize this field to NULL_XT_INDEX.
*
***********************************************************************/
#define sym_xt_index(symp) (symp->xt_table_index)

/***********************************************************************\
*
* int sym_is_function(symp)
*
* Given a symbol table entry pointer, return TRUE if the symbol is a 
* function/procedure symbol.
*
***********************************************************************/
#define sym_is_function(symp) ( ISFTN(symp->stype) )

/***********************************************************************\
*
* void sym_set_xt_index(symp, index)
*
* Given a symbol table entry pointer, store the index of the xt table
* entry in the symbol.
*
* Compiler MUST initialize this field to NULL_XT_INDEX.
*
***********************************************************************/
#define sym_set_xt_index(symp, index) (symp->xt_table_index = index)

/***********************************************************************\
*
* void write_to_as_stream(s)
*
* Write the passed string to the assembler stream.  
*
***********************************************************************/
#define write_to_as_stream(s) fprntf(outfile, s)


extern flag Allflag;
extern char *satitle;
extern int cdbfile_vtindex;
extern int comp_unit_vtindex;
