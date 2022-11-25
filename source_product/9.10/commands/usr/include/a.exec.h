/* @(#) $Revision: 64.1 $ */       
/*
 * Format of an a.out header
 */
#ifndef _A_EXEC_INCLUDED /* allows multiple inclusions */
#define _A_EXEC_INCLUDED
 
#ifdef __hp9000s300
struct	exec {	/* a.out header */
	long	a_magic;	/* magic number */
	long	a_text;		/* size of text segment */
	long	a_data;		/* size of initialized data */
	long	a_bss;		/* size of uninitialized data */
	long	a_syms;		/* size of symbol table */
	long	a_trsize;	/* size of text relocation */
	long	a_drsize;	/* size of data relocation */
	long	a_entry;	/* entry point */
};
#endif /* __hp9000s300 */
#endif /* _A_EXEC_INCLUDED */
