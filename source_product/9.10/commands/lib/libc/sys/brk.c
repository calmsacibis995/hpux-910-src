/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/sys/brk.c,v $
 * $Revision: 64.4 $	$Author: marklin $
 * $State: Exp $   	$Locker:  $
 * $Date: 89/01/25 17:35:35 $
 *
 * $Log:	brk.c,v $
 * Revision 64.4  89/01/25  17:35:35  17:35:35  marklin
 * Change reference of end to _end. Current linker supports it.
 * 
 * Revision 64.3  89/01/19  14:55:33  14:55:33  marklin (Mark Lin)
 * Namespace cleanup using _NAMESPACE_CLEAN flag.
 * 
 * Revision 1.2  85/04/04  13:00:39  13:00:39  wallace
 * Header added.  
 * 
 * $Endlog$
 */

#ifdef _NAMESPACE_CLEAN
#define brk __brk
#define end _end
#endif

#include <errno.h>

extern	char end;
extern	int errno;
char	*_minbrk = &end;
char	*_curbrk = &end;

#ifdef _NAMESPACE_CLEAN
#undef brk
#pragma _HP_SECONDARY_DEF __brk brk
#define brk __brk
#endif

brk(endds)
	char *endds;
{
	if ((unsigned)endds < (unsigned)_minbrk) {
		errno = EINVAL;
		return(-1);
	}
	if (_brk(endds) == -1)
		return(-1);
	_curbrk = endds;
	return(0);
}
