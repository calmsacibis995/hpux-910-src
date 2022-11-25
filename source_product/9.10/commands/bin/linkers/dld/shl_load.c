/* @(#) $Revision: 70.2 $ */

/*
 *	shl_load.c
 *
 *	Series 300 dynamic loader
 *
 *	new stub for shl_load (uses strdup; potentially incompatible)
 *
 */


#pragma		HP_SHLIB_VERSION	"5/91"


#ifdef	ARCHIVE
#define		shl_load	_shl_load
#endif

#include	<errno.h>
#include	<dl.h>


typedef (*__fp) ();
extern unsigned long __dld_loc;


shl_t shl_load (const char *path, int flags, long address)
{
	shl_t handle;

	handle = (shl_t)(*(__fp)(__dld_loc+SHL_LOAD))(path,flags,address,&errno);
	if (handle != NULL)
	{
#if defined(ALLOCATE_NAME)
		((struct dynamic *)handle)->dld_hook = (unsigned long)strdup(path);
#elif defined(SHLIB_PATH)
		if (((struct dynamic *)handle)->shlib_path == NULL)
			((struct dynamic *)handle)->shlib_path = strdup(path);
#endif
	}
	return(handle);
}
