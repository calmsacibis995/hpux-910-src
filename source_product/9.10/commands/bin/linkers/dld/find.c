/* @(#) $Revision: 70.3 $ */

/*
 *	find.c
 *
 *	Series 300 dynamic loader
 *
 *	code to perform symbol lookup
 *
 */


#include	"dld.h"


static int search_all (import_t import, dynamic_t current_DYNAMIC, int mode, dynamic_t *target_DYNAMIC, export_t *target_export);
static int search_user (import_t import, dynamic_t current_DYNAMIC, int mode, dynamic_t *target_DYNAMIC, export_t *target_export);
static int search_one (import_t import, int mode, dynamic_t *target_DYNAMIC, export_t *target_export);
static int findlibsym (import_t import, int mode, dynamic_t target_DYNAMIC, export_t *target_export);
#ifndef	ALL_CODE
#pragma	HP_INLINE_NOCODE	search_all, search_one, findlibsym
#pragma	HP_INLINE_OMIT		search_user
#endif

static string_t g_name;


int findsym (import_t import, dynamic_t current_DYNAMIC, int mode, dynamic_t *target_DYNAMIC, export_t *target_export)
/*
 * on entry:
 *	import = import table entry for symbol to locate
 *	current_DYNAMIC = DYNAMIC structure of file containing reference
 *	mode = options
 * returns:
 *	non-zero normally
 *	zero if symbol not found
 * on exit:
 *	*target_DYNAMIC = DYNAMIC structure of file containing definition
 *	*target_export = export table entry for symbol
 * effects:
 *	search for specified symbol
 *	options include IGNORE_SHL_PROC, IGNORE_EXEC
 */
{
	int found;
	shl_export_t shl_export;
	char *dmodule_flag;

#if	DEBUG & 2
	printf("entering findsym\n");
#endif
#ifdef	VISIBILITY
	if (import->type == TYPE_INTRA)
	{
		static struct export_entry my_export;

#if	DEBUG & 512
		printf("intra reference: address 0x%08X, module %d\n",import->name,import->shl);
#endif
		dmodule_flag = current_DYNAMIC->dmodule + import->shl;
		if (!*dmodule_flag)
		{
			*dmodule_flag = -1;
			smart_bind(current_DYNAMIC,ADDR(dmodule)+import->shl);
		}
		my_export.value = import->name;
		my_export.type = import->type;
		*target_export = &my_export;
		*target_DYNAMIC = current_DYNAMIC;
		return(-1);
	}
#endif
#if	DEBUG & 2
	printf("symbol = %s\n",ADDR(string)+import->name);
#endif
	dld_last_hash = hash(g_name=ADDR(string)+import->name);
	switch (import->shl)
	{
		case -1:
			/* request came from shlib, dumb a.out, or dumb user */
			found = search_all(import,current_DYNAMIC,mode,target_DYNAMIC,target_export);
			break;
		case -2:
			/* request came from smart user */
			found = search_user(import,current_DYNAMIC,mode,target_DYNAMIC,target_export);
			break;
		default:
			/* request came from smart executable */
			if (current_DYNAMIC == exec_DYNAMIC)
				found = search_one(import,mode,target_DYNAMIC,target_export);
			/* request came from helpful library */
			else
				found = search_all(import,current_DYNAMIC,mode,target_DYNAMIC,target_export);
	}
	if (found)
	{
		if (*target_DYNAMIC != exec_DYNAMIC)
		{
			current_DYNAMIC = *target_DYNAMIC;
			shl_export = ADDR(shl_export) + g_eindex;
			if (shl_export->dmodule != -1)
			{
				dmodule_flag = current_DYNAMIC->dmodule + shl_export->dmodule;
				if (!*dmodule_flag)
				{
					*dmodule_flag = -1;
					smart_bind(current_DYNAMIC,ADDR(dmodule)+shl_export->dmodule);
				}
			}
		}
	}
#if	DEBUG & 2
	printf("exiting findsym - found = %d\n",found);
#endif
	return(found);
}


static int search_all (import_t import, dynamic_t current_DYNAMIC, int mode, dynamic_t *target_DYNAMIC, export_t *target_export)
/*
 * on entry:
 *	import = import table entry for symbol to locate
 *	current_DYNAMIC = DYNAMIC structure of file containing reference
 *	mode = options
 * returns:
 *	non-zero normally
 *	zero if symbol not found
 * on exit:
 *	*target_DYNAMIC = DYNAMIC structure of file containing definition
 *	*target_export = export table entry for symbol
 * effects:
 *	searches all currently loaded files for specified symbol
 */
{
	int here;
	int comm_g_eindex;
	dynamic_t comm_DYNAMIC;
	export_t comm_export;
	short type;

#if	DEBUG & 2
	printf("entering search_all\n");
#endif
#ifdef	SEARCH_ORDER
	*target_DYNAMIC = head_DYNAMIC;
#else
	/* don't bother looking in a.out to resolve reference from a.out */
	if ((*target_DYNAMIC = exec_DYNAMIC) == current_DYNAMIC)
		*target_DYNAMIC = (*target_DYNAMIC)->next_shl;
#endif
	/* search all libraries */
	comm_export = NULL;
	while (*target_DYNAMIC != NULL)
	{
#pragma	BBA_IGNORE_ALWAYS_EXECUTED
		if (*target_DYNAMIC == current_DYNAMIC)
		{
#ifdef	SEARCH_ORDER
			/* don't make a.out search self */
			if (current_DYNAMIC == exec_DYNAMIC)
				here = -1;
#endif
			/* otherwise, if library searching self, check for hint */
			if (import->shl >= 0)
			{
				*target_export = ADDR(export) + (g_eindex = import->shl);
				here = 1;
#ifdef NO_INTRA_FIX
				/* but wait! find right version, first */
				if (current_DYNAMIC->shl != NULL && current_DYNAMIC->shl->highwater >= 0)
				{
					short target_version = current_DYNAMIC->shl->highwater;
					export_t exports = (*target_export) - import->shl;
					long target_name = (*target_export)->name;

					while ((*target_export)->highwater > target_version)
					{
						if ((*target_export)->next_export == -1)
						{
							here = -1;
							break;
						}
						g_eindex = (*target_export)->next_export;
						*target_export = exports + g_eindex;
						if ((*target_export)->name != target_name)
						{
							here = -1;
							break;
						}
					}
				}
#endif
#if	DEBUG & 16
				printf("hint taken (thanks!) - export #%d\n",import->shl);
#endif
			}
			else
				here = -1;
		}
		else
			here = 0;
		if (here > 0 || (here == 0 && findlibsym(import,mode,*target_DYNAMIC,target_export)))
		{
			type = (*target_export)->type & TYPE_MASK;
			if (type == TYPE_COMMON)
			{
				/* keep searching for data or larger comm */
				if (comm_export == NULL || (*target_export)->size > comm_export->size)
				{
					comm_DYNAMIC = *target_DYNAMIC;
					comm_export = *target_export;
					comm_g_eindex = g_eindex;
				}
			}
			else if (comm_export == NULL || type != TYPE_PROCEDURE)
			{
				/* don't take PROCEDURE to satisfy COMMON */
#if	DEBUG & 2
				printf("exiting search_all - found!\n");
#endif
				return(-1);
			}
		}
		*target_DYNAMIC = (*target_DYNAMIC)->next_shl;
	}
	/* return largest comm */
	if (comm_export != NULL)
	{
		*target_DYNAMIC = comm_DYNAMIC;
		*target_export = comm_export;
		g_eindex = comm_g_eindex;
#if	DEBUG & 2
		printf("exiting search_all - found comm!\n");
#endif
		return(-1);
	}
	/* symbol not found */
#if	DEBUG & 2
	printf("exiting search_all - sorry!\n");
#endif
	return(0);
}


static int search_user (import_t import, dynamic_t current_DYNAMIC, int mode, dynamic_t *target_DYNAMIC, export_t *target_export)
/*
 * on entry:
 *	import = import table entry for symbol to locate
 *	current_DYNAMIC = DYNAMIC structure of file containing reference
 *	mode = options
 * returns:
 *	non-zero normally
 *	zero if symbol not found
 * on exit:
 *	*target_DYNAMIC = DYNAMIC structure of file containing definition
 *	*target_export = export table entry for symbol
 * effects:
 *	searches file specified by user for symbol
 */
{
	*target_DYNAMIC = (dynamic_t)current_DYNAMIC->dl_header->e_spec.dl_header.dynamic;
	if (findlibsym(import,mode,*target_DYNAMIC,target_export))
		return(-1);
	else
		return(0);
}


static int search_one (import_t import, int mode, dynamic_t *target_DYNAMIC, export_t *target_export)
/*
 * on entry:
 *	import = import table entry for symbol to locate
 *	mode = options
 * returns:
 *	non-zero normally
 *	zero if symbol not found
 * on exit:
 *	*target_DYNAMIC = DYNAMIC structure of file containing definition
 *	*target_export = export table entry for symbol
 * effects:
 *	searches file specified by import entry for symbol
 */
{
	int i;

	*target_DYNAMIC = exec_DYNAMIC;
	/* skip to library */
	for (i = 0; i < import->shl; ++i)
		*target_DYNAMIC = (*target_DYNAMIC)->next_shl;
	if (findlibsym(import,mode,*target_DYNAMIC,target_export))
		return(-1);
	else
		return(0);
}


static int findlibsym (import_t import, int mode, dynamic_t current_DYNAMIC, export_t *target_export)
/*
 * on entry:
 *	import = import table entry for symbol to locate
 *	mode = options
 *	current_DYNAMIC = DYNAMIC structure of file to search
 * returns:
 *	non-zero normally
 *	zero if symbol not found
 * on exit:
 *	*target_export = export table entry for symbol
 * effects:
 *	searches specified library for symbol
 *	performs type checking
 */
{
	short highwater;
	string_t string;
	hash_t hasht;
	int hashsize;
	short target_type;
	export_t export;

#if	DEBUG & 2
	printf("entering findlibsym target = %X\n",current_DYNAMIC);
#endif
	/* get what we need from file to search */
	if (current_DYNAMIC->shl == NULL)
		highwater = -1;
	else
		highwater = current_DYNAMIC->shl->highwater;
	export = ADDR(export);
	string = ADDR(string);
	hasht = ADDR(hash);
	hashsize = (hash_t)string - hasht;

	/* perform search */
	if (!ExpLookup(g_name,highwater,target_export,string,export,hasht,hashsize))
		return(0);
	if (((target_type = (*target_export)->type & TYPE_MASK) == TYPE_SHL_PROC) && (mode & IGNORE_SHL_PROC))
		return(0);
#if	DEBUG & 2
	printf("found something - let's do type checking - %d against %d\n",(import->type&TYPE_MASK),target_type);
#endif
	/* perform type checking */
	switch (import->type
#ifndef	VISIBILITY
	        & TYPE_MASK
#endif
	       )
	{
		case TYPE_UNDEFINED:
			break;
		case TYPE_COMMON:
		case TYPE_DATA:
		case TYPE_CDATA:
#pragma	BBA_IGNORE
		case TYPE_BSS:
			switch (target_type)
			{
				case TYPE_COMMON:
				case TYPE_DATA:
				case TYPE_CDATA:
#pragma	BBA_IGNORE
				case TYPE_BSS:
					break;
				default:
					return(0);
			}
			break;
		case TYPE_SHL_PROC:
#pragma	BBA_IGNORE
		case TYPE_PROCEDURE:
			switch (target_type)
			{
				case TYPE_PROCEDURE:
				case TYPE_SHL_PROC:
					break;
				default:
					return(0);
			}
			break;
		default:
#pragma	BBA_IGNORE
			break;
	}
#if	DEBUG & 2
	printf("type checking succeeded - exiting findlibsym\n");
#endif
	return(-1);
}
