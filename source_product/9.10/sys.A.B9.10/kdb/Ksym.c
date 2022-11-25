/* @(#) $Revision: 70.2 $ */    

#include	"Kdb.h"
#include	"basic.h"
#include	<a.out.h>
extern int bss_addr;			/* use as a temporary buffer */
extern int vfnSym, vsbErrSfailSD;

export int vcbLstFirst;

struct symentry *pSymTable;		/* pointer to the LST		*/
char *pNamePool;			/* pointer to the Name Pool	*/
int symcount = 0;			/* symbol count			*/
extern int vimap;

InitLst(cbLstCache)
register int cbLstCache;
{
    register int	cRead;			/* bytes read	*/

    if (seekread(vfnSym, vcbLstFirst, bss_addr, cbLstCache, "InitLst") < 0)
	Panic (vsbErrSfailSD, "Read", "InitLst", cbLstCache);

    kdb_symcollect(bss_addr,cbLstCache);
}

struct symentry *find_by_name(name)
char *name;
{
	register int i;

	for (i = 0; i < symcount; i++)
		if (strcmp(name,&pNamePool[pSymTable[i].npEntry]) == 0) {
			return(&pSymTable[i]);
		}
	return((struct symentry *) 0);
}

AdrFLabel(sbVar)
char *sbVar;
{
	register struct symentry *sym;
	sym = find_by_name(sbVar);
	if (sym) return sym->value;
	else return -1;
}

struct symentry *find_by_value(value)
unsigned int value;
{
	register int low, high, mid;

	if (symcount == 0)
		return((struct symentry *) 0);

	low = 0;
	high = symcount-1;
	
	while (low <= high) {
		mid = (low + high) / 2;
		if (value < pSymTable[mid].value)
			high = mid-1;
		else if (value > pSymTable[mid].value)
			low = mid+1;
		else
			return(&pSymTable[mid]);
	}
	return(&pSymTable[high]);
}

AdrFLSTAdr(adr)
unsigned int adr;
{
	register struct symentry *sym;
	sym = find_by_value(adr);
	if (sym) return sym->value;
	else return -1;
}

LabelFAdr(adr, psbLabel, pOffset)
unsigned int adr;
char **psbLabel;
long *pOffset;
{
	register struct symentry *sym;
	sym = find_by_value(adr);
	*psbLabel = &pNamePool[sym->npEntry];
	*pOffset = adr - sym->value;
}

print_symbol(addr, string)
unsigned int addr;
char *string;
{
	register struct symentry *s;
	register unsigned int disp;

	s = find_by_value(addr);

	if (vimap) s = 0;
	if (s == 0)
		printf("0x%x", addr);
	else {
		disp = addr - s->value;
		if (disp > 0x200000)
			printf("0x%x", addr);
		else
		{	printf("%s", &pNamePool[s->npEntry]);
			if (disp) printf("+0x%x", disp);
		}
	}
	printf(string);
}

kdb_symcollect(position, size)
char *position;
int size;
{
	register char *pos;

	struct nlist_ sym;
	register struct nlist_ *sy = &sym;

	char symbuf[SYMLENGTH +1];
	register char *sb = symbuf;

	register int i, last;
	register char *sp, *p;

	register struct symentry *st;
	register unsigned long npsize = 0;
	int numcomp();

	last = (int)(position+size);

	/* the first time we execute this loop we just count the number of
	 * symbols and calculate the size of the name pool
	 */
	for (pos = position ; (pos < (char *) last);) {
		/* put a symbol into sym */
		for (p=(char *) sy, i=0; i < sizeof(struct nlist_); i++)
			*p++ = *pos++;
		symcount++;		/* inc symbol count */

		/* put the symbol name into the symbol buffer */
		for(sp = sb,i=sy->n_length;i>0;i--)
			*sp++ = *pos++;
		*sp = 0;
		if (sy->n_type&N_EXT == 0)
			continue;

		if (sb[0] == '~' || sb[0] == '_')
			npsize += sp-sb;
		else
			npsize += sp-sb+1;
	}

	pSymTable = (struct symentry *)Malloc
				(symcount * sizeof (struct symentry),"InitLST");
	pNamePool = (char *)Malloc(npsize,"InitLST");
	st = pSymTable;
	npsize = 0;
	for (pos = position ; (pos < (char *) last);) {
		/* put a symbol into sym */
		for (p=(char *) sy, i=0; i < sizeof(struct nlist_); i++)
			*p++ = *pos++;

		/* put the symbol name into the symbol buffer */
		for(sp = sb,i=sy->n_length;i>0;i--)
			*sp++ = *pos++;
		*sp = 0;
		if (sy->n_type&N_EXT == 0)
			continue;
		
		p = pNamePool + npsize;
		st->value = sy->n_value;
		st->npEntry = npsize;
		if (st->npEntry != npsize) {
			printf("Hey! failed for %d %s\n",
				npsize, pNamePool[npsize]);
		}

		if (sb[0] == '~' || sb[0] == '_')
		{	npsize += sp-sb;
			sp = sb+1;
		}
		else
		{	npsize += sp-sb+1;
			sp = sb;
		}
		strcpy(p,sp);
		st++;
	}

	qsort(pSymTable, symcount, sizeof(struct symentry), numcomp);
	printf("%d symbols\n",symcount);

}

numcomp(p1, p2)
struct symentry *p1, *p2;
{
	return(p1->value - p2->value);
}

ListLabels(sbLabel)
register char *sbLabel;
{
	register int i;
	register char* sym;

	for (i = 0; i < symcount; i++)
	{
		sym = &pNamePool[pSymTable[i].npEntry];
		if ((sbLabel != 0) && (! FHdrCmp(sbLabel,sym)))
			continue;
		printf("%-15s  0x%x\n",sym, pSymTable[i].value);
	}
}
