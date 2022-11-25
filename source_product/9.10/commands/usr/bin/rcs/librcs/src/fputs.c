/* $Header: fputs.c,v 4.1 85/08/14 15:50:39 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * fputs(s, iop) puts string s on file iop, abort on error.
 * Same as puts in stdio, but with different putc macro.
 */
#include <stdio.h>

fputs(s, iop)
register char *s;
register FILE *iop;
{
	register r;
	register c;

	while (c = *s++)
		r = putc(c, iop);
	return(r);
}
