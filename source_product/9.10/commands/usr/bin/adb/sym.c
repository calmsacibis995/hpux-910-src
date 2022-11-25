/* @(#) $Revision: 66.1 $ */     

/****************************************************************************

	DEBUGGER - symbol table management

****************************************************************************/
#include "defs.h"

MSG		BADFIL;
POS		maxoff;
long		mainsval;
short		symnum;
long		symbas;
int		pid;
struct symb	*symtab;
SYMPTR		symbol;
SYMPTR		symnxt;
SYMPTR		symend;
STRING		symnamb;

STRING		symfil;
STRING		errflg;
POS		findsym();

setupsym(ssize)
register long	ssize;
{
	register SYMPTR	symptr;
	register char	*nameptr;
	extern	char	*malloc();
	register short	i;
	register FILE	*bout;
	int 	mns;		/* maximum number of symbols */
	int 	tabsize;	/* size of adb's symbol table structure */

	/* 
		adb's symbol table structure

	 --------------------------------------------------------
	| symb structures -->                     <-- name pool  |
	 --------------------------------------------------------
	 ^							^
	 |							|
	symtab						      symnamb

	The table is one data structure, with the struct symb entries
	in the beginning, and the name pool at the end. The symb
	structures have pointers to the symbol name which is stored
	at the other side of this table. The symb structures and name
	pool grow towards each other. The size of the table, tabsize,
	is computed as follow
				a.out symbol table size
	1. Max # of symbols =   ------------------------
				sizeof(struct nlist_) + 1
							^
							|
				min. size of each symbol name is 1 char 

	2. Table Size =  size of a.out symbol table +
		 max # of symbols [1 char to make each symbol name asciz] +
		 mns * sizeof (char *) [because symb has an added fld symc] +
		 sizeof symb [for sentinal value at end, ESYM]

	*/

	mns = ssize / ( sizeof (struct nlist_) + 1);
	tabsize = ssize + mns * (1+sizeof(char *)) + sizeof (struct symb);
	symtab = (struct symb *) malloc (tabsize); 
	symnamb = (char *)symtab;
	symnamb += tabsize - 1;

	if ((bout = fopen(symfil,"r")) == NULL)
		 printf("\n\tcannot fopen %s", symfil);
	fseek(bout, symbas, 0);			/* seek start of symbols  */
	symnum = 0;
	symptr = symtab;
	if (ssize == 0) printf("\n\tno symbol table - proceeding anyway");
						/* build symbol table	  */
	while (ssize > 0)
	{
		if (fread(symptr, 1, sizeof(struct nlist_), bout) != sizeof(struct nlist_)) break; 
		symptr->smtp = SYMTYPE(symptr->symf);
		if ((ssize -= sizeof (struct nlist_)) == 0) break;
		*symnamb = 0;			/* to make asciz */
		nameptr = symnamb - symptr->slength;
		symnamb = nameptr -1;
		symptr->symc = nameptr;

		for (i=0;i<symptr->slength;i++) {
			if (fread(nameptr++, 1,1, bout) != 1) break;
			}
		ssize -= i;					/* MFM */
		symnum++; symptr++;
	}
	symend = symptr;			/* end of symbols */
	symend->smtp = ESYM;
	if ((char *)symptr >= symnamb)
		printf("\n\tbeware: internal error - symbol table");
	else if (ssize) printf("\n\tsymbol table problem - proceeding anyway");
	fclose(bout);
	if (symptr = lookupsym("main")) mainsval = symptr->vals;
}

longseek(f, adr)
FILE	*f;
long	adr;
{
	return(lseek(f, adr, 0) != -1);
}

psymoff(v, type, s)
long	v;
int	type;
char	*s;
{
	unsigned register w = findsym(v, type);
	if (w >= maxoff) printf("%X", v);
	else
	{
		printf("%s", symbol->symc);
		if (w) printf("+%x",w);
	}
	printf(s);
}

unsigned
findsym(svalue, type)
unsigned long	svalue;
int	type;
{
	register long	diff = 0377777L;
	register unsigned long symval;
	register SYMPTR	symptr;
	SYMPTR	symsav = NULL;

	if ((type != NSYM) && (symptr = symtab))
	{
		while (diff && (symptr->smtp != ESYM))
		{
			symval = symptr->vals;
			if (((svalue - symval) < diff)
				&& (svalue >= symval))
			{
				diff = svalue - symval;
				symsav = symptr;
			}
			symptr++;
		}
		if (symsav) symbol = symsav;
	}
	return(shorten(diff));
}

nextsym()
{
	if (++symbol == symend)
	{
		symbol--;
		return(FALSE);
	}
	else return(TRUE);
}

symset()
{
	symnxt = symtab;
}

SYMPTR
symget()
{
	SYMPTR	ptr;

	if (symnxt >= symend) return(NULL);
	ptr = symnxt; symnxt++;
	return(ptr);
}

