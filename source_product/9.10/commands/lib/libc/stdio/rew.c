/* @(#) $Revision: 70.2 $ */    

#ifdef PIC
#pragma HP_SHLIB_VERSION "4/92"
#endif

/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define fseek _fseek
#define rewind _rewind
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>

#ifdef _NAMESPACE_CLEAN
#undef rewind
#pragma _HP_SECONDARY_DEF _rewind rewind
#define rewind _rewind
#endif

void
rewind(iop)
register FILE *iop;
{
	(void) fseek(iop, 0L, SEEK_SET);
	/* Clear _IOERR and _IOEOF */
	iop->_flag &= ~(_IOERR | _IOEOF);
}
