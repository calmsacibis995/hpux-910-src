/* @(#) $Revision: 64.1 $ */     
#ifndef _SYMBOL_INCLUDED /* allow multiple inclusions */
#define _SYMBOL_INCLUDED
#ifdef HFS


/*
 * Structure of a symbol table entry
 */

struct	symbol {
	char	sy_name[8];
	char	sy_type;
	int	sy_value;
};

#endif /* HFS */
#endif /* _SYMBOL_INCLUDED */
