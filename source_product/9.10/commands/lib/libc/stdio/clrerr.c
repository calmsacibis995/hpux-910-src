/* @(#) $Revision: 64.1 $ */      
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#ifdef __lint
#define clearerr _clearerr
#endif /* __lint */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#undef clearerr

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _clearerr clearerr
#define clearerr _clearerr
#endif /* _NAMESPACE_CLEAN */

void
clearerr(iop)
register FILE *iop;
{
	iop->_flag &= ~(_IOERR | _IOEOF);
}
