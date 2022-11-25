/* @(#) $Revision: 64.1 $ */     
/*LINTLIBRARY*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _div div
#define div _div
#endif

#include <stdlib.h>

div_t
div(numer, denom)
int numer, denom;
{
    div_t res;

#if -3 / 2 == -1
    res.quot = numer / denom;
    res.rem = numer % denom;
#else
    res.quot = (numer >= 0 ? numer : -numer) / (denom >= 0 ? denom : -denom);
    if ((numer >= 0) != (denom >= 0))
	res.quot = -res.quot;
    res.rem = numer - denom * res.quot;
#endif

    return res;

}
