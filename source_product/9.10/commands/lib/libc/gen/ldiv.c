/* @(#) $Revision: 64.3 $ */     
/*LINTLIBRARY*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _ldiv ldiv
#define ldiv _ldiv
#endif /* _NAMESPACE_CLEAN */

#include <stdlib.h>

ldiv_t
ldiv(numer, denom)
long int numer, denom;
{
    ldiv_t res;

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

#pragma OPT_LEVEL 1

#ifdef __hp9000s300
asm("
global	___ldiv
");
#endif /* __hp9000s300 */

#pragma OPT_LEVEL 2
