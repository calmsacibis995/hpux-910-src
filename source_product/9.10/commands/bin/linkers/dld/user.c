/* @(#) $Revision: 70.5 $ */

/*
 *	user.c
 *
 *	Series 300 dynamic loader
 *
 *	user interface routines
 *
 */


#include	"dld.h"

#include	<sys/cache.h>


static struct header_extension my_DL_HEADER;
static struct dynamic my_DYNAMIC;
static struct shl_entry shl;
static struct import_entry import;
#ifdef SHLIB_PATH
static char real_string[2*MAXPATHLEN+3];
static char *string;
#else
static char string[MAXPATHLEN+1];
#endif
static struct shl_descriptor my_descriptor;

#ifdef CHECK_HANDLE
static int inituser(shl_t handle);
#else
static void inituser (void);
#endif
#ifdef	BIND_FLAGS
static void verbose (void);
#endif
#pragma	HP_INLINE_NOCODE	inituser


#ifdef	HOOKS
static int aborted_load = 0;
#endif


#ifdef CHECK_HANDLE
static int inituser (shl_t handle)
#else
static void inituser (void)
#endif
/*
 * on entry:
 *	handle = user specified library handle
 * effects:
 *	initializes data structures for user functions
 * returns:
 *	zero normally
 *	non-zero if handle is invalid
 */
{
	static int inited = 0;
#if defined(SHLIB_PATH) || defined(CHECK_HANDLE)
	dynamic_t current_DYNAMIC;
#endif

#ifdef CHECK_HANDLE
	if (handle != NULL && handle != &_DYNAMIC)
	{
		for (current_DYNAMIC = head_DYNAMIC; current_DYNAMIC != NULL; current_DYNAMIC = current_DYNAMIC->next_shl)
		{
			if (current_DYNAMIC == (dynamic_t)handle)
				break;
		}
		if (current_DYNAMIC == NULL)
			return -1;
	}
#endif
	if (inited)
	{
#ifdef CHECK_HANDLE
		return 0;
#else
		return;
#endif
	}
	inited = 1;
	my_DYNAMIC.dl_header = &my_DL_HEADER;
#ifdef SHLIB_PATH
	current_DYNAMIC = exec_DYNAMIC;
	if (current_DYNAMIC->embed_path)
	{
		my_DYNAMIC.embed_path = 1;
		strncpy(real_string+1,ADDR(string)+exec_DYNAMIC->embed_path,MAXPATHLEN);
		real_string[MAXPATHLEN+1] = 0;
		string = real_string + MAXPATHLEN + 2;
	}
	else
	{
		my_DYNAMIC.embed_path = 0;
		string = real_string;
	}
	my_DYNAMIC.shlib_path = current_DYNAMIC->shlib_path;
	if (current_DYNAMIC->status & DYNAMIC_FIRST)
		my_DYNAMIC.status |= DYNAMIC_FIRST;
	else if (current_DYNAMIC->status & DYNAMIC_LAST)
		my_DYNAMIC.status |= DYNAMIC_LAST;
#endif
	my_DL_HEADER.e_spec.dl_header.shlt = (ulong)&shl - (ulong)&my_DL_HEADER;
	my_DL_HEADER.e_spec.dl_header.import = (ulong)&import - (ulong)&my_DL_HEADER;
#ifdef SHLIB_PATH
	my_DL_HEADER.e_spec.dl_header.string = (ulong)real_string - (ulong)&my_DL_HEADER;
#else
	my_DL_HEADER.e_spec.dl_header.string = (ulong)string - (ulong)&my_DL_HEADER;
#endif
#ifdef CHECK_HANDLE
	return 0;
#endif
}


shl_t _shl_load (const char *path, int flags, long address, int *uerrno)
/*
 * on entry:
 *	path = pathname of shared library to load
 *	flags = BIND_IMMEDIATE or BIND_DEFERRED
 *	address = 0 or address at which library should be loaded
 * returns:
 *	library "handle" (address of DYNAMIC structure)
 *	NULL on error
 * on exit:
 *	uerrno is set
 * effects:
 *	attaches library to process
 *	performs all binding if specified
 */
{
	dynamic_t target_DYNAMIC, save_exec_DYNAMIC;
#ifdef	SEARCH_ORDER
	dynamic_t *back, next;
#endif

#ifdef CHECK_HANDLE
	if (inituser(NULL))
	{
		*uerrno = EINVAL;
		return NULL;
	}
#else
	inituser();
#endif
	/* set up for load() call */
	(void)strncpy(string,path,MAXPATHLEN);
#ifdef SHLIB_PATH
	string[MAXPATHLEN] = 0;
	shl.name = string - real_string;
#else
	shl.name = 0;
#endif
	shl.highwater = -1;
#ifdef SHLIB_PATH
	shl.load = (flags & DYNAMIC_PATH) ? LOAD_LIB : LOAD_PATH;
	flags &= ~DYNAMIC_PATH;
#else
	shl.load = LOAD_PATH;
#endif
	shl.bind = flags;
	save_exec_DYNAMIC = exec_DYNAMIC;

#ifdef	SEARCH_ORDER
	/* find place to insert new library */
	if (flags & BIND_FIRST)
	{
		back = &head_DYNAMIC;
		next = head_DYNAMIC;
	}
	else
	{
#endif
		/* find end of DYNAMIC chain */
		target_DYNAMIC = exec_DYNAMIC;
		while (target_DYNAMIC->next_shl != NULL)
#pragma	BBA_IGNORE_ALWAYS_EXECUTED
			target_DYNAMIC = target_DYNAMIC->next_shl;
#ifdef	SEARCH_ORDER
		back = &target_DYNAMIC->next_shl;
		next = NULL;
	}
#endif
	/* load library */
	exec_DYNAMIC = &my_DYNAMIC;
	if ((
#ifdef	SEARCH_ORDER
		 target_DYNAMIC
#else
	     target_DYNAMIC->next_shl
#endif
	     = load(&shl,(caddr_t)address)) == NULL)
	{
		*uerrno = errno;
		exec_DYNAMIC = save_exec_DYNAMIC;
		return(NULL);
	}
#ifdef	SEARCH_ORDER
	*back = target_DYNAMIC;
	target_DYNAMIC->next_shl = next;
#else
	target_DYNAMIC = target_DYNAMIC->next_shl;
#endif
	exec_DYNAMIC = save_exec_DYNAMIC;
	shl.name = -1;

	/* bind library */
#ifdef	ELABORATOR
	(void)cachectl(CC_FLUSH,NULL,0);
	if (target_DYNAMIC->status & INITIALIZE)
	{
		if (!(flags & BIND_NOSTART))
		{
			if (!bind_initial(target_DYNAMIC))
			{
				*uerrno = errno;
#ifdef	BIND_FLAGS
				if ((flags & BIND_VERBOSE) && errno == ENOSYM)
					verbose();
#endif
				return(NULL);
			}
		}
	}
	else
		target_DYNAMIC->initializer = -1;
#endif
	if (!bindfile(target_DYNAMIC))
	{
		/* warning - this may leave system in inconsistent state! */
		*uerrno = errno;
#ifdef	BIND_FLAGS
		if ((flags & BIND_VERBOSE) && errno == ENOSYM)
			verbose();
#endif
#ifdef	HOOKS
		aborted_load = 1;
#endif
		(void)_shl_unload(target_DYNAMIC,uerrno);
		return(NULL);
	}

#ifndef	ELABORATOR
	(void)cachectl(CC_FLUSH,NULL,0);
#endif
	target_DYNAMIC->shl = NULL;
#if defined(SHLIB_PATH)
	if (target_DYNAMIC->shlib_path != NULL && !*target_DYNAMIC->shlib_path)
	{
		strncpy(target_DYNAMIC->shlib_path,path,SHLIB_PATH_LEN-1);
		target_DYNAMIC->shlib_path[SHLIB_PATH_LEN-1] = 0;
	}
#elif defined(ALLOCATE_NAME)
	target_DYNAMIC->dld_hook = (ulong)path;
#endif
#ifdef	HOOKS
	if (dld_hook != NULL)
		(*dld_hook)(SHL_LOAD,(shl_t)target_DYNAMIC,flags);
#endif
	return((shl_t)target_DYNAMIC);
}


int _shl_findsym (shl_t *handle, const char *symname, short type, long *value, int *uerrno)
/*
 * on entry:
 *	handle = pointer to handle of shared library to search
 *	        or pointer to NULL
 *	symname = name of symbol to locate
 * returns:
 *	zero normally
 *	-1 if symbol not found
 * on exit:
 *	handle = shared library in which symbol was found
 *	value = value of symbol
 *	uerrno is set
 * effects:
 *	searches specified shared library for symbol
 *	if no library is specified, all libraries are searched
 */
{
	export_t export;

#ifdef CHECK_HANDLE
	if (inituser(*handle))
	{
		*uerrno = EINVAL;
		return -1;
	}
#else
	inituser();
#endif
	*value = 0;
	(void)strncpy(string,symname,MAXPATHLEN);
#ifdef SHLIB_PATH
	string[MAXPATHLEN] = 0;
	import.name = string - real_string;
#else
	import.name = 0;
#endif
	import.shl = (*handle == NULL) ? -1 : -2;
	import.type = type;
	shl.highwater = -1;
	my_DL_HEADER.e_spec.dl_header.dynamic = (ulong)*handle;
	switch (_setjmp(except))
	{
		case 0:
			break;
		case SYMBOL_ERR:
			*uerrno = ENOSYM;
			return(-1);
		case DRELOC_ERR:
#pragma	BBA_IGNORE
		default:
#pragma	BBA_IGNORE
			*uerrno = ENOEXEC;
			return(-1);
	}
	if (!findsym(&import,&my_DYNAMIC,0,(dynamic_t *)handle,&export))
	{
		*uerrno = 0;
		return(-1);
	}
	*value = resolve(*(dynamic_t *)handle,export);
	return(0);
}


int _shl_unload (shl_t handle, int *uerrno)
/*
 * on entry:
 *	handle = handle of shared library to unload
 * returns:
 *	zero if successful
 *	-1 otherwise
 * on exit:
 *	uerrno is set
 * effects:
 *	detaches library from process
 */
{
#ifdef	SEARCH_ORDER
	dynamic_t *back;
#else
	dynamic_t current_DYNAMIC;
#endif
	dynamic_t target_DYNAMIC;
	int shl_size;
	int i;
	ulong text, data, bss, end;

#ifdef CHECK_HANDLE
	if (inituser(handle))
	{
		*uerrno = EINVAL;
		return -1;
	}
#else
	inituser();
#endif
#ifdef	SEARCH_ORDER
	/* was it dynamically loaded? */
	if ((dynamic_t)handle->shl != NULL)
		return(EINVAL);
	/* get place in search order */
	if ((target_DYNAMIC = head_DYNAMIC) == (dynamic_t)handle)
		back = &head_DYNAMIC;
	else
	{
#else
	/* skip to last preloaded library */
	target_DYNAMIC = current_DYNAMIC = exec_DYNAMIC;
	shl_size = (shlt_t)ADDR(import) - ADDR(shlt);
	for (i = 0; i < shl_size - 1; ++i)
		target_DYNAMIC = target_DYNAMIC->next_shl;
#endif
		/* search for library */
		while (target_DYNAMIC != NULL && target_DYNAMIC->next_shl != (dynamic_t)handle)
			target_DYNAMIC = target_DYNAMIC->next_shl;
		if (target_DYNAMIC == NULL)
		{
			*uerrno = EINVAL;
			return(-1);
		}
#ifdef	SEARCH_ORDER
		back = &target_DYNAMIC->next_shl;
	}
#else
#define		back	(&target_DYNAMIC->next_shl)
#endif
#ifdef	HOOKS
	if (aborted_load)
		aborted_load = 0;
	else if (dld_hook != NULL)
		(*dld_hook)(SHL_UNLOAD,handle);
#endif

	/* unlink library */
#ifdef	ELABORATOR
	if (((*back)->status & INITIALIZE) && (*back)->initializer != -1)
		(*(int (*)())(*back)->initializer)((*back),0);
#endif
	text = (*back)->text;
	data = (*back)->data;
	bss = (*back)->bss;
	end = (*back)->end;
	*back = (*back)->next_shl;
	/* unmap it */
	if (munmap((caddr_t)text,data-text))
	{
#pragma	BBA_IGNORE
		*uerrno = errno;
		return(-1);
	}
	if (munmap((caddr_t)data,bss-data))
	{
#pragma	BBA_IGNORE
		*uerrno = errno;
		return(-1);
	}
	if ((end-bss) && munmap((caddr_t)bss,end-bss))
	{
#pragma	BBA_IGNORE
		*uerrno = errno;
		return(-1);
	}
	return(0);
}


int _shl_get (int index, struct shl_descriptor **desc)
/*
 * on entry:
 *	index = index of shared library to retrieve
 * returns:
 *	zero if successful
 *	-1 otherwise
 * on exit:
 *	desc = address of a static shared library descriptor
 * effects:
 *	retrieves information about a shared library
 */
{
	dynamic_t target_DYNAMIC;
#ifndef	GETHANDLE
	dynamic_t current_DYNAMIC;
	const char *filename;
#endif

	if (index == -1)
		target_DYNAMIC = &_DYNAMIC;
	else
	{
		target_DYNAMIC =
#ifndef	GETHANDLE
		                 current_DYNAMIC =
#endif
#ifdef	SEARCH_ORDER
		                 head_DYNAMIC;
#else
		                 exec_DYNAMIC;
#endif
		for ( ; index ; -- index )
		{
			if ((target_DYNAMIC = target_DYNAMIC->next_shl) == NULL)
				return(-1);
		}
	}
#ifdef	GETHANDLE
	return(_shl_gethandle(target_DYNAMIC,desc));
#else
	*desc = &my_descriptor;
	(*desc)->tstart = target_DYNAMIC->text;
	(*desc)->tend = target_DYNAMIC->data;
	(*desc)->dstart = target_DYNAMIC->data;
	(*desc)->dend = target_DYNAMIC->end;
	(*desc)->handle = (shl_t)target_DYNAMIC;
	if (index == -1)
		filename = "<dld.sl>";
	else if (target_DYNAMIC->shl != NULL)
		filename = ADDR(string) + target_DYNAMIC->shl->name;
#ifdef	ALLOCATE_NAME
	else if (target_DYNAMIC->dld_hook)
		filename = (char *)target_DYNAMIC->dld_hook;
#endif
	else
		filename = "<shl_load>";
	(void)strncpy((*desc)->filename,filename,MAXPATHLEN);
	return(0);
#endif
}


#ifdef	GETHANDLE
int _shl_gethandle (shl_t handle, struct shl_descriptor **desc)
/*
 * on entry:
 *	handle = library to retrieve
 * returns:
 *	zero if successful
 *	-1 otherwise
 * on exit:
 *	desc = address of a static shared library descriptor
 * effects:
 *	retrieves information about a shared library
 */
{
	dynamic_t current_DYNAMIC = exec_DYNAMIC;
	const char *filename;

#ifdef CHECK_HANDLE
	if (inituser(handle))
		return -1;
#endif
	*desc = &my_descriptor;
	(*desc)->tstart = ((dynamic_t)handle)->text;
	(*desc)->tend = ((dynamic_t)handle)->data;
	(*desc)->dstart = ((dynamic_t)handle)->data;
	(*desc)->dend = ((dynamic_t)handle)->end;
	(*desc)->handle = handle;
	if (handle == (shl_t)&_DYNAMIC)
		filename = "<dld.sl>";
#ifdef SHLIB_PATH
	else if ((dynamic_t)handle != exec_DYNAMIC && ((dynamic_t)handle)->shlib_path != NULL && *((dynamic_t)handle)->shlib_path)
		filename = (char *)(((dynamic_t)handle))->shlib_path;
#endif
	else if (((dynamic_t)handle)->shl != NULL)
		filename = ADDR(string) + ((dynamic_t)handle)->shl->name;
#ifdef	ALLOCATE_NAME
	else if (((dynamic_t)handle)->dld_hook)
		filename = (char *)(((dynamic_t)handle))->dld_hook;
#endif
	else
		filename = "<shl_load>";
	(void)strncpy((*desc)->filename,filename,MAXPATHLEN);
#ifdef SHLIB_PATH
	(*desc)->filename[MAXPATHLEN] = 0;
#endif
#ifdef	ELABORATOR
	if (((dynamic_t)handle)->initializer != -1)
		(*desc)->initializer = (void *)(((dynamic_t)handle)->initializer);
	else
		(*desc)->initializer = NO_INITIALIZER;
#endif
	return(0);
}
#endif


#ifdef	BIND_FLAGS
static void verbose (void)
{
	char buf[1024];

	strcpy(buf,"/lib/dld.sl: Unresolved external - ");
	strcat(buf,badsym);
	strcat(buf,"\n");
	write(2,buf,strlen(buf));
}
#endif
