/* @(#) $Revision: 70.4 $ */

/*
 *	libdld.c
 *
 *	Series 300 dynamic loader
 *
 *	user interface library stubs
 *
 */


#ifndef __lint
static char *HPUX_ID = "@(#) $Revision: 70.4 $";
#endif


#ifdef	ARCHIVE
#define		shl_load	_shl_load
#define		shl_unload	_shl_unload
#define		shl_findsym	_shl_findsym
#define		shl_get		_shl_get
#define		shl_shlibsused	_shl_shlibsused
#ifdef	GETHANDLE
#define		shl_gethandle	_shl_gethandle
#endif
#endif


#include	<errno.h>
#include	<dl.h>


typedef (*__fp) ();
extern unsigned long __dld_loc;


shl_t shl_load (const char *path, int flags, long address)
{
	return((shl_t)(*(__fp)(__dld_loc+SHL_LOAD))(path,flags,address,&errno));
}

int shl_findsym (shl_t *handle, const char *symname, short type, void *value)
{
	return((int)(*(__fp)(__dld_loc+SHL_FINDSYM))(handle,symname,type,value,&errno));
}

int shl_unload (shl_t handle)
{
	return((int)(*(__fp)(__dld_loc+SHL_UNLOAD))(handle,&errno));
}

int shl_get (int index, struct shl_descriptor **desc)
{
	return((int)(*(__fp)(__dld_loc+SHL_GET))(index,desc));
}

#ifdef	GETHANDLE
int shl_gethandle (shl_t handle, struct shl_descriptor **desc)
{
	return((int)(*(__fp)(__dld_loc+SHL_GETHANDLE))(handle,desc));
}
#endif

#ifdef	ELABORATOR
int shl_shlibsused (void)
{
	return(__dld_loc != 0);
}
#endif
