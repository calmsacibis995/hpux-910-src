/* @(#) $Revision: 32.1 $ */       
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */

#define TRUE	(-1)
#define FALSE	0
#define LOBYTE	0377

#ifndef NLS
#define STRIP	0177
#define QUOTE	0200
#else NLS
#define STRIP	0x7fff		/* strips 16th bit only */
#define QUOTE	0x8000
#define NOTASCI 0xff80		/* high 9 bits not ascii or is quoted */
#define CHECK16	0x7f00		/* checks for 2 byte chars */
#endif NLS 		

#define EOF	0
#define NL	'\n'
#define SP	' '
#define LQ	'`'
#define RQ	'\''
#define MINUS	'-'
#define COLON	':'
#define TAB	'\t'

#ifndef hpux
#define MAX(a,b)	((a)>(b)?(a):(b))
#endif hpux

#define blank()		prc(SP)
#define	tab()		prc(TAB)
#define newline()	prc(NL)

