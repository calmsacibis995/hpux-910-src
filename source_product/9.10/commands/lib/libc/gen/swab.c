/* @(#) $Revision: 70.1 $ */    
/*LINTLIBRARY*/
/*
 * Swap bytes in 16-bit [half-]words
 * for going between the 11 and the interdata
 */
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _swab swab
#define swab _swab
#endif

void
swab(pf, pt, n)
#ifdef hp9000s800
register char *pf, *pt;
#else
register short *pf, *pt;
#endif
register int n;
{

#ifdef hp9000s800
	char c;
#endif

	n /= 2;
	while(--n >= 0) {
#ifdef hp9000s800
		c = *pf;
		*pt++ = *(pf+1);
		*pt++ = c;
		pf += 2;
#else
		*pt++ = (*pf << 8) + ((*pf >> 8) & 0377);
		pf++;
#endif
	}
}
