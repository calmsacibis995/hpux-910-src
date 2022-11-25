/* @(#) $Revision: 26.2 $ */    
#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <msgbuf.h>
#endif NLS
# include "e.h"

sqrt(p2) int p2; {
	yyval = p2;
	nrwid(p2, ps, p2);
#ifdef NLS
	printf(".ds %d \\v'%du'", p2, ebase[p2]);
	printf((nl_msg(24, "\\e")));
	printf("\\L'%du'\\l'\\n(%du'", -eht[p2], p2);
#else
	printf(".ds %d \\v'%du'\\e\\L'%du'\\l'\\n(%du'",
		p2, ebase[p2], -eht[p2], p2);
#endif
	printf("\\v'%du'\\h'-\\n(%du'\\*(%d\n", eht[p2]-ebase[p2], p2, p2);
	eht[p2] += VERT(1);
	if(dbg)printf(".\tsqrt: S%d <- S%d;b=%d, h=%d\n", 
		p2, p2, ebase[p2], eht[p2]);
}
