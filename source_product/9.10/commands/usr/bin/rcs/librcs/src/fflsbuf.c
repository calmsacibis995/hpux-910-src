/* $Header: fflsbuf.c,v 4.1 85/08/14 15:50:34 scm HP_UXRel2 $ */

/*
 * extracted from rcslex.c
 */

/*
 * fflsbuf(c, iop) flush buffer on file iop, abort on error.
 * Same as _flsbuf in stdio, but but aborts program on error.
 */
#include <stdio.h>

fflsbuf(c, iop)
int c; register FILE * iop;
{       register result;
        if ((result=_flsbuf(c,iop))==EOF)
                faterror("write error");
        return result;
}
