/* @(#) $Revision: 66.2 $ */

#include <errno.h>

#ifdef NON_PROFILE
#   define sbrk _sbrk_no_profile
#   define _brk  _brk_noprof
#   undef _NAMESPACE_CLEAN
#endif

extern	char *_curbrk;
extern	char *_minbrk;

#ifdef _NAMESPACE_CLEAN
#undef sbrk
#pragma _HP_SECONDARY_DEF _sbrk sbrk
#define sbrk _sbrk
#endif

char *
sbrk(incr)
long incr;
{
    register char *obrk;
    register char *endds;

    if (incr == 0)		/* tune for speed */
	return _curbrk;

    obrk = _curbrk;
    endds = obrk + incr;
    if ((unsigned)endds < (unsigned)_minbrk)
    {
	errno = EINVAL;
	return (char *)-1;
    }

    if (_brk(endds) == -1)
	return (char *)-1;

    _curbrk = endds;
    return obrk;
}
