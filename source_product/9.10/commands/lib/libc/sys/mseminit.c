/* @(#) $Revision: 70.1 $ */

#include <sys/mman.h>

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _msem_init msem_init
#define msem_init _msem_init
#endif /* _NAMESPACE_CLEAN */

msemaphore *
msem_init(sem, initial_value)
msemaphore *sem;
int initial_value;
{
    /*
     * Call the real msem_init.  If the return value indicates failure,
     * convert it from -1 to NULL.  This per OSF/AES.
     */
    if (__msem_init(sem, initial_value) == -1)
	return (msemaphore *)0;
    
    return sem;
}
