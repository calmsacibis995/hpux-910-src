/* @(#) $Revision: 70.1 $ */

/*
 *	bind.c
 *
 *	Series 300 dynamic loader
 *
 *	code to perform binding of symbols
 *
 */


#include	"dld.h"

#ifdef	PURGE_CACHE
#include	<sys/cache.h>
#endif

#ifdef	FLUSH_CACHE
#include	<sys/unistd.h>
#endif


static int bind_immediate = 0;

#if	DEBUG & 32
static int rec_level, highest_rec;
#endif

#ifdef	FLUSH_CACHE
static int cpu;
extern void flush_cache (void *addr, int flag);
#endif

char *bor (ulong plt_entry_addr);
static void bind_plt (dynamic_t current_DYNAMIC);
void smart_bind (dynamic_t current_DYNAMIC, dmodule_t dm);
static void bind_dlt (dynamic_t current_DYNAMIC);
static void dreloc (dynamic_t current_DYNAMIC);
static void module_bind_adlt (dynamic_t current_DYNAMIC, long start, long end);
static void module_dreloc (dynamic_t current_DYNAMIC, long start, long end);
static void smart_bind_lts (dynamic_t current_DYNAMIC, dmodule_t dm);
#ifndef	ALL_CODE
#pragma	HP_INLINE_NOCODE	module_bind_adlt, module_dreloc, smart_bind_lts
#pragma	HP_INLINE_OMIT		bind_dlt
#endif
static void dreloc_one (dynamic_t current_DYNAMIC, dreloc_t dr);
static ulong dr_ext (dreloc_t dr, dynamic_t current_DYNAMIC);
static void dr_propagate (dreloc_t dr, dynamic_t current_DYNAMIC);
static dreloc_t propagate (export_t export, dynamic_t current_DYNAMIC);
#ifndef	ALL_CODE
#pragma	HP_INLINE_NOCODE	dreloc_one, dr_ext, dr_propagate, propagate
#pragma	HP_INLINE_OMIT		bind_plt
#endif
#ifdef	ELABORATOR
static void module_invoke (dynamic_t current_DYNAMIC, long start, long end);
#ifndef	ALL_CODE
#pragma	HP_INLINE_OMIT		module_invoke
#endif
#endif


int bindfile (dynamic_t current_DYNAMIC)
/*
 * on entry:
 *	current_DYNAMIC = DYNAMIC structure of file to bind
 * returns:
 *	non-zero normally
 *	zero on error
 *	(on error, system may be in an inconsistent state)
 * on exit:
 *	errno is set
 * effects:
 *	binds linkage table and performs dynamic relocation
#ifndef	ELABORATOR
 *	if library is specified as BIND_DEFERRED, set up for shl_bor() only
#endif
 */
{
	char *db, *enddb;

#if	DEBUG & 32
	rec_level = 0;
	highest_rec = 0;
#endif
#ifndef	ELABORATOR
	/* handle deferred binding */
	current_DYNAMIC->bor.displacement = (ulong)&shl_bor - (ulong)&(current_DYNAMIC->bor.displacement);
#endif
	switch (_setjmp(except))
	{
		case 0:
			break;
		case SYMBOL_ERR:
#ifdef	BIND_FLAGS
			bind_immediate = (exec_DYNAMIC->shl->bind & BIND_IMMEDIATE);
#endif
			errno = ENOSYM;
			return(0);
		case DRELOC_ERR:
#pragma	BBA_IGNORE
#ifdef	BIND_FLAGS
			bind_immediate = (exec_DYNAMIC->shl->bind & BIND_IMMEDIATE);
#endif
			badsym = "bad relocation";
			errno = ENOEXEC;
			return(0);
		default:
#pragma	BBA_IGNORE
#ifdef	BIND_FLAGS
			bind_immediate = (exec_DYNAMIC->shl->bind & BIND_IMMEDIATE);
#endif
			errno = ENOEXEC;
			return(0);
	}
	if (current_DYNAMIC == exec_DYNAMIC)
	{
#ifdef	BIND_FLAGS
		if (current_DYNAMIC->shl->bind & BIND_IMMEDIATE)
#else
		if (current_DYNAMIC->shl->bind == BIND_IMMEDIATE)
#endif
		{
			bind_immediate = -1;
			bind_plt(current_DYNAMIC);
		}
		dreloc(current_DYNAMIC);
#ifdef	FLUSH_CACHE
		cpu = __sysconf(_SC_CPU_VERSION);
#endif
	}
	else if (current_DYNAMIC->shl->name == -1)
	{
		/* this is an shl_load() library */
#ifdef	BIND_FLAGS
		if (current_DYNAMIC->shl->bind & BIND_DEFERRED)
#else
		if (current_DYNAMIC->shl->bind == BIND_DEFERRED)
#endif
		{
			bind_immediate = 0;
			return(-1);
		}
		bind_immediate = -1;
		enddb = (char *)current_DYNAMIC->bound;
		for (db = current_DYNAMIC->dmodule; db < enddb; ++db)
			*db = -1;
		bind_plt(current_DYNAMIC);
		bind_dlt(current_DYNAMIC);
#pragma	HP_INLINE_OMIT		dreloc
		dreloc(current_DYNAMIC);
#pragma	HP_INLINE_DEFAULT	dreloc
#ifdef	BIND_FLAGS
		bind_immediate = (exec_DYNAMIC->shl->bind & BIND_IMMEDIATE);
#else
		bind_immediate = (exec_DYNAMIC->shl->bind == BIND_IMMEDIATE);
#endif
#ifdef	ELABORATOR
		if (current_DYNAMIC->status & INITIALIZE)
			module_invoke(current_DYNAMIC,0,-1);
#endif
	}
#if	DEBUG & 32
	printf("highest recurse level for %X was %d\n",current_DYNAMIC,highest_rec);
#endif
	return(-1);
}


static void bind_plt (dynamic_t current_DYNAMIC)
/*
 * on entry:
 *	current_DYNAMIC = DYNAMIC structure of file to bind
 * on exit:
 *	takes exception on error
 *	errno is set
 * effects:
 *	binds all plt entries
 *	if any symbol is not found, its binding is deferred
 * requires:
 *	assumes no plt entries have yet been bound
 *	called only for a.out's or shl_load() libraries with BIND_IMMEDIATE
 */
{
	plt_t plt;
	int plt_size, i;
	import_t import;
	dynamic_t target_DYNAMIC;
	export_t target_export;

#if	DEBUG & 8
	printf("entering bind_plt\n");
#endif
	import = ADDR(import) + (current_DYNAMIC->a_dlt - current_DYNAMIC->dlt);
	plt = current_DYNAMIC->plt;
	plt_size = (plt_t)current_DYNAMIC - plt;
	/* bind each entry */
	for (i = 0; i < plt_size; ++i)
	{
		/* find definition */
		if (findsym(import+i,current_DYNAMIC,IGNORE_SHL_PROC,&target_DYNAMIC,&target_export))
		{
#pragma	HP_INLINE_OMIT		resolve
			plt[i].displacement = resolve(target_DYNAMIC,target_export) - (ulong)&(plt[i].displacement);
#pragma	HP_INLINE_DEFAULT	resolve
			plt[i].opcode = BRA;
		}
		else
#ifdef	BIND_FLAGS
		if (!(current_DYNAMIC->shl->bind & BIND_NONFATAL))
#endif
		{
			badsym = ADDR(string)+(import+i)->name;
			_longjmp(except,SYMBOL_ERR);
		}
	}
#if	DEBUG & 8
	printf("exiting bind_plt\n");
#endif
	return;
}


static void bind_dlt (dynamic_t current_DYNAMIC)
/*
 * on entry:
 *	current_DYNAMIC = DYNAMIC structure of file to bind
 * on exit:
 *	takes exception on error
 *	errno is set
 * effects:
 *	binds all dlt entries
 *	if any symbol is not found, it is a fatal error
 * requires:
 *	assumes no dlt entries have yet been bound
 *	called only for shl_load() libraries with BIND_IMMEDIATE
 */
{
	dlt_t dlt;
	int dlt_size, i;
	import_t import;
	ulong text;
	dynamic_t target_DYNAMIC;
	export_t target_export;

#if	DEBUG & 8
	printf("entering bind_dlt\n");
#endif
	dlt = current_DYNAMIC->dlt;
	dlt_size = current_DYNAMIC->a_dlt - dlt;
	import = ADDR(import);
	/* bind each entry */
#if	DEBUG & 8
	printf("binding dlt entries (%d of them)\n",dlt_size);
#endif
	for (i = 0; i < dlt_size; ++i)
	{
		/* find definition */
		if (findsym(import+i,current_DYNAMIC,0,&target_DYNAMIC,&target_export))
#pragma	HP_INLINE_OMIT		resolve
			dlt[i].address = (char *)resolve(target_DYNAMIC,target_export);
#pragma	HP_INLINE_DEFAULT	resolve
		else
		{
			badsym = ADDR(string)+(import+i)->name;
			_longjmp(except,SYMBOL_ERR);
		}
	}
	if (text = current_DYNAMIC->ptext)
	{
		dlt = current_DYNAMIC->a_dlt;
		dlt_size = (dlt_t)current_DYNAMIC->plt - dlt;
#if	DEBUG & 8
		printf("binding adlt entries (%d of them)\n",dlt_size);
#endif
		for (i = 0; i < dlt_size; ++i)
		{
			/* non-imported symbol */
			dlt[i].address += text;
		}
	}
#ifndef	__lint
	else
#pragma	BBA_IGNORE
		;
#endif
#if	DEBUG & 8
	printf("exiting bind_dlt\n");
#endif
	return;
}


void smart_bind (dynamic_t current_DYNAMIC, dmodule_t dm)
/*
 * on entry:
 *	current_DYNAMIC = DYNAMIC structure of file to bind
 *	dm = dmodule entry of module to bind
 * on exit:
 *	takes exception on error
 *	errno is set
 * effects:
 *	binds all dlt and dreloc entries needed by this module
 *	if bind_immediate is in effect, binds plt entries as well
 */
{
#if	DEBUG & 32
	if (++rec_level > highest_rec)
		highest_rec = rec_level;
#endif
#if	DEBUG & 4
	printf("entering smart_bind\n");
#endif
	if (dm->a_dlt < (dm+1)->a_dlt)
		module_bind_adlt(current_DYNAMIC,dm->a_dlt,(dm+1)->a_dlt);
	if (dm->dreloc < (dm+1)->dreloc)
		module_dreloc(current_DYNAMIC,dm->dreloc,(dm+1)->dreloc);
#if	! defined(ELABORATOR)
	if ((dm->flags) || (bind_immediate && dm->module_imports != -1))
#else
	if ((dm->flags & DM_DLTS) || (bind_immediate && dm->module_imports != -1))
#endif
		smart_bind_lts(current_DYNAMIC,dm);
#ifdef	ELABORATOR
	if (dm->flags & DM_INVOKES)
		module_invoke(current_DYNAMIC,dm->dreloc,(dm+1)->dreloc);
#endif
#if	DEBUG & 4
	printf("exiting smart_bind\n");
#endif
#if	DEBUG & 32
	--rec_level;
#endif
}


static void smart_bind_lts (dynamic_t current_DYNAMIC, dmodule_t dmodule)
/*
 * on entry:
 *	current_DYNAMIC = DYNAMIC structure of file to bind
 *	dmodule = dmodule entry of module to bind
 * on exit:
 *	takes exception on error
 *	errno is set
 * effects:
 *	binds all dlt entries needed by this module
 *	if bind_immediate is in effect, binds plt entries as well
 *	this process is recursive
 */
{
	long index, dlt_size, last_plt;
	import_t import;
	module_t mi;
	dynamic_t target_DYNAMIC;
	export_t target_export;
	dlt_t dlt;
	plt_t plt;

#if	DEBUG & 4
	printf("entering smart_bind_lts\n");
#endif
	import = ADDR(import);
	mi = ADDR(module) + dmodule->module_imports;
	dlt = current_DYNAMIC->dlt;
	/* do DLT's */
	while (mi->import & MODULE_DLT_FLAG)
	{
		index = mi->import & MODULE_IMPORT_MASK;
		if (!current_DYNAMIC->bound[index])
		{
			current_DYNAMIC->bound[index] = -1;
			if (!findsym(import+index,current_DYNAMIC,0,&target_DYNAMIC,&target_export))
			{
				badsym = ADDR(string)+(import+index)->name;
				_longjmp(except,SYMBOL_ERR);
			}
			dlt[index].address = (char *)resolve(target_DYNAMIC,target_export);
		}
		if (mi->import & MODULE_END_FLAG)
			break;
		++mi;
	}
	/* do PLT's if bind_immediate specified, and PLT's present */
	if (bind_immediate && !(mi->import & MODULE_DLT_FLAG))
	{
		plt = current_DYNAMIC->plt;
		dlt_size = current_DYNAMIC->a_dlt - dlt;
		last_plt = ((plt_t)current_DYNAMIC - plt) + dlt_size;
		while (1)
		{
#pragma	BBA_IGNORE_ALWAYS_EXECUTED
			index = mi->import & MODULE_IMPORT_MASK;
			if (index < last_plt && !current_DYNAMIC->bound[index])
			{
				current_DYNAMIC->bound[index] = -1;
				if (!findsym(import+index,current_DYNAMIC,IGNORE_SHL_PROC,&target_DYNAMIC,&target_export)
#ifdef	BIND_FLAGS
				    && !(current_DYNAMIC->shl->bind & BIND_NONFATAL)
#endif
				   )
				{
					badsym = ADDR(string)+(import+index)->name;
					_longjmp(except,SYMBOL_ERR);
				}
				index -= dlt_size;
				plt[index].displacement = resolve(target_DYNAMIC,target_export) - (ulong)&(plt[index].displacement);
				plt[index].opcode = BRA;
			}
#ifdef	PLTS_BEFORE_DRELOCS
			else if (index >= last_plt)
				break;
#endif
			if ((mi->import & MODULE_END_FLAG))
				break;
			++mi;
		}
	}
#if	DEBUG & 4
	printf("exiting smart_bind_lts\n");
#endif
}


static void module_bind_adlt (dynamic_t current_DYNAMIC, long start, long end)
/*
 * on entry:
 *	current_DYNAMIC = DYNAMIC structure of file to bind
 *	start = index of first adlt to bind
 *	end = index of last dlt to bind
 * effects:
 *	binds all adlt entries needed by this module
 */
{
	ulong text;
	dlt_t dlt, edlt;

#if	DEBUG & 4
	printf("doing adlt entries %d through %d\n",start,end);
#endif
	if (text = current_DYNAMIC->ptext)
	{
		edlt = current_DYNAMIC->a_dlt + end;
		for (dlt = current_DYNAMIC->a_dlt + start; dlt < edlt; ++dlt)
			dlt->address += text;
	}
#ifndef	__lint
	else
#pragma	BBA_IGNORE
		;
#endif
}


static void module_dreloc (dynamic_t current_DYNAMIC, long start, long end)
/*
 * on entry:
 *	current_DYNAMIC = DYNAMIC structure of file to bind
 *	start = index of first dreloc to bind
 *	end = index of last dreloc to bind
 * on exit:
 *	takes exception on error
 *	errno is set
 * effects:
 *	binds all dreloc entries needed by this module
 */
{
	dreloc_t dr, dreloct;

#if	DEBUG & 4
	printf("entering module_dreloc - doing records %d through %d\n",start,end);
#endif
	dreloct = ADDR(dreloc);
	for (dr = dreloct + start; dr < dreloct + end; ++dr)
		dreloc_one(current_DYNAMIC,dr);
#if	DEBUG & 4
	printf("exiting module_dreloc\n");
#endif
}


static void dreloc (dynamic_t current_DYNAMIC)
/*
 * on entry:
 *	current_DYNAMIC = DYNAMIC structure of file to bind
 * on exit:
 *	takes exception on error
 *	errno is set
 * effects:
 *	performs dynamic relocation for entire file
 *	if any symbol is not found, it is a fatal error
 * requires:
 *	assumes no dreloc entries have yet been bound
 */
{
	dreloc_t dr;

#if	DEBUG & 4
	printf("entering dreloc - current_DYNAMIC = %X\n",current_DYNAMIC);
#endif
	dr = ADDR(dreloc);
	/* patch each record */
	while (dr->type != DR_END)
	{
		dreloc_one(current_DYNAMIC,dr);
		++dr;
	}
#if	DEBUG & 4
	printf("exiting dreloc\n");
#endif
}


static void dreloc_one (dynamic_t current_DYNAMIC, dreloc_t dr)
{
	ulong text, value;

#if	DEBUG & 4
	printf("entering dreloc_one\n");
#endif
	text = current_DYNAMIC->ptext;
	/* handle of each type of relocation record */
	switch (dr->type)
	{
		case DR_FILE:
			value = text;
			break;
		case DR_EXT:
			value = dr_ext(dr,current_DYNAMIC);
			break;
		case DR_PROPAGATE:
			dr_propagate(dr,current_DYNAMIC);
			return;
#ifdef	ELABORATOR
		case DR_INVOKE:
			return;
#endif
		case DR_NOOP:
			return;
		default:
#pragma	BBA_IGNORE
			_longjmp(except,DRELOC_ERR);
	}
	if (!patch((ulong)(dr->address+text),dr->length,value))
#pragma	BBA_IGNORE
		_longjmp(except,DRELOC_ERR);
#if	DEBUG & 4
	printf("exiting dreloc_one\n");
#endif
}


#ifdef	ELABORATOR
static void module_invoke (dynamic_t current_DYNAMIC, long start, long end)
/*
 * on entry:
 *	current_DYNAMIC = DYNAMIC structure of file containing relocations
 *	start = index of first dreloc to process
 *	end = index of last dreloc to process
 * effects:
 *	invokes appropriate elaborators
 */
{
	int i;
	dreloc_t dr;

#if	DEBUG & 64
printf("entering module_invoke: doing records %d through %d\n",start,end);
#endif
	dr = ADDR(dreloc);
	for (i = start; i != end; ++i)
	{
		switch ((dr+i)->type)
		{
			case DR_INVOKE:
				(*(int (*)())current_DYNAMIC->elaborator)((dr+i)->address+current_DYNAMIC->ptext,(dr+i)->symbol,current_DYNAMIC);
				break;
			case DR_END:
				return;
			default:
#pragma	BBA_IGNORE
				break;
		}
	}
}
#endif


static ulong dr_ext (dreloc_t dr, dynamic_t current_DYNAMIC)
/*
 * on entry:
 *	dr = dynamic relocation record to resolve
 *	current_DYNAMIC = DYNAMIC structure of file containing relocation
 * returns:
 *	value to patch
 * on exit:
 *	takes exception on error
 * effects:
 *	calculates value for DR_EXT relocation record
 */
{
	int dlt_flag = 0;
	ulong address;
	dynamic_t target_DYNAMIC;
	export_t target_export;
	import_t import;

#if	DEBUG & 4
	printf("entering dr_ext\n");
#endif
	import = ADDR(import) + dr->symbol;
	if (dr->symbol < current_DYNAMIC->a_dlt - current_DYNAMIC->dlt && !current_DYNAMIC->bound[dr->symbol])
	{
		dlt_flag = -1;
		current_DYNAMIC->bound[dr->symbol] = -1;
#if	DEBUG & 4
		printf("gonna bind me a dlt, too\n");
#endif
	}
	if (!findsym(import,current_DYNAMIC,0,&target_DYNAMIC,&target_export))
	{
		badsym = ADDR(string) + import->name;
		_longjmp(except,SYMBOL_ERR);
	}
	address = resolve(target_DYNAMIC,target_export);
	if (dlt_flag)
		current_DYNAMIC->dlt[dr->symbol].address = (char *)address;
#if	DEBUG & 4
	printf("exiting dr_ext\n");
#endif
	return(address);
}


static void dr_propagate (dreloc_t dr, dynamic_t current_DYNAMIC)
/*
 * on entry:
 *	dr = dynamic relocation record to propagate
 *	current_DYNAMIC = DYNAMIC structure of file containing relocation
 * on exit:
 *	takes exception on error
 * effects:
 *	handles DR_PROPAGATE dynamic relocation record
 *	applies all associated dynamic relocation records from shared library
 *	to object in this (incomplete executable) file
 */
{
	import_t import;
	dynamic_t target_DYNAMIC;
	export_t target_export;
	dreloc_t pdr;
	ulong addr, paddr, symaddr, value;

#if	DEBUG & 64
	printf("entering dr_propagate - current_DYNAMIC = %X\n",current_DYNAMIC);
#endif
	/* apply patches obtained from elsewhere */
	import = ADDR(import) + dr->symbol;
	if (!findsym(import,current_DYNAMIC,IGNORE_EXEC,&target_DYNAMIC,&target_export))
	{
		badsym = ADDR(string) + import->name;
		_longjmp(except,SYMBOL_ERR);
	}
	symaddr = resolve(target_DYNAMIC,target_export);
	while ((pdr = propagate(target_export,target_DYNAMIC)) != NULL)
	{
		paddr = pdr->address + target_DYNAMIC->ptext;
		addr = dr->address + current_DYNAMIC->ptext + (paddr - symaddr);
#if	DEBUG & 64
		printf("patching %X with contents of %X\n",addr,paddr);
#endif
		switch (pdr->type)
		{
			case DR_FILE:
			case DR_EXT:
				switch (pdr->length)
				{
					case DR_BYTE:
						value = *(uchar *)paddr;
						*(uchar *)addr = (uchar)value;
						break;
					case DR_WORD:
						value = *(ushort *)paddr;
						*(ushort *)addr = (ushort)value;
						break;
					case DR_LONG:
						value = *(ulong *)paddr;
						*(ulong *)addr = (ulong)value;
						break;
					default:
#pragma	BBA_IGNORE
						_longjmp(except,DRELOC_ERR);
				}
				break;
			case DR_NOOP:
#pragma	BBA_IGNORE
				break;
			default:
#pragma	BBA_IGNORE
				_longjmp(except,DRELOC_ERR);
		}
	}
#if	DEBUG & 64
	printf("exiting dr_propagate\n");
#endif
}


static dreloc_t propagate (export_t export, dynamic_t current_DYNAMIC)
/*
 * on entry:
 *	export = export entry of symbol to propagate
 *	current_DYNAMIC = DYNAMIC structure of file containing definition
 * returns:
 *	next dynamic relocation record for symbol
 *	NULL when done
 * effects:
 *	this routine acts as a coroutine
 *	on the first call for a given symbol,
 *	it returns the first dynamic relocation record for the symbol,
 *	and on successive calls, it returns successive records
 */
{
	static dreloc_t pdr = NULL;
	static ulong size = 0;
	shl_export_t shl_export;

#if	DEBUG & 64
	printf("entering propagate - pdr = %X\n",pdr);
	printf("current_DYNAMIC = %X\n",current_DYNAMIC);
#endif
	if (pdr == NULL)
	{
		/* first call for symbol */
		shl_export = ADDR(shl_export) + (export - ADDR(export));
		if (shl_export->dreloc == -1)
			return(NULL);
		pdr = ADDR(dreloc) + shl_export->dreloc;
		size = export->size;
	}
	else
	{
		/* successive calls */
		if ((++pdr)->type == DR_END || pdr->address >= export->value + size)
			pdr = NULL;
	}
#if	DEBUG & 64
	printf("exiting propagate - return value is %X\n",pdr);
#endif
	return(pdr);
}


int patch (ulong address, char length, ulong value)
/*
 * on entry:
 *	address = address to patch
 *	length = number of bytes to patch
 *	value = offset to add to contents of address
 * returns:
 *	non-zero normally
 *	zero on error
 * on exit:
 *	errno is set
 * effects:
 *	patches contents of address with value
 */
{
	if (length == DR_LONG)
	{
		*(char *)(address) = 0;
		*(ulong *)address += value;
	}
	else if (length == DR_WORD)
		*(ushort *)address += (ushort)value;
	else if (length == DR_BYTE)
		*(uchar *)address += (uchar)value;
	else
#pragma	BBA_IGNORE
		return(0);
	return(-1);
}


ulong resolve (dynamic_t target_DYNAMIC, export_t target_export)
/*
 * on entry:
 *	target_DYNAMIC = DYNAMIC structure of file containing definition
 *	target_export = export entry of definition
 * returns:
 *	value of symbol
 */
{
	if ((target_export->type) == TYPE_ABSOLUTE)
		return(target_export->value);
	return(target_export->value+target_DYNAMIC->ptext);
}


char *bor (ulong plt_entry_addr)
/*
 * on entry:
 *	plt_entry_addr = return address from call that caused the trap
 * returns:
 *	address of target symbol
 * effects:
 *	determines which file caused the trap
 *	determines which symbol to bind
 *	binds the symbol
 *	fixes up PLT entry
 */
{
	int index;
	dynamic_t current_DYNAMIC;
	import_t import;
	ulong addr;
	dynamic_t target_DYNAMIC;
	export_t target_export;
	struct plt_entry *this;
#ifdef	HOOKS
	static int in_hook = 0;
#endif

	plt_entry_addr -= offsetof(struct plt_entry,spare1);
#if	DEBUG & 512
	printf("bor from 0x%08X\n",plt_entry_addr);
#endif
	this = (struct plt_entry *)plt_entry_addr;
#ifndef	CHEAT_IN_ASM
#ifndef	PURGE_CACHE
	/* if it was already fixed, blame it on the I-cache */
	if (this->opcode == BRA)
		return((char *)this->displacement+(ulong)&this->displacement);
#endif
#endif
	switch (_setjmp(except))
	{
		case 0:
			break;
		case SYMBOL_ERR:
			fatal(ENOSYM,badsym);
		case DRELOC_ERR:
#pragma	BBA_IGNORE
			fatal(ENOEXEC,"bad relocation");
		default:
#pragma	BBA_IGNORE
			fatal(ENOEXEC,badsym);
	}
#ifdef	SEARCH_ORDER
	current_DYNAMIC = head_DYNAMIC;
#else
	current_DYNAMIC = exec_DYNAMIC;
#endif
	/* find library from which reference came */
	do
	{
		if ((ulong)current_DYNAMIC->plt <= plt_entry_addr && plt_entry_addr < (ulong)current_DYNAMIC)
			break;
		current_DYNAMIC = current_DYNAMIC->next_shl;
	} while (current_DYNAMIC != NULL);
	if (current_DYNAMIC == NULL)
	{
#if	DEBUG & 256
		printf("plt_entry_addr = 0x%08X\n",plt_entry_addr);
#endif
#pragma	BBA_IGNORE
		fatal(ENOEXEC,"bad reference");
	}

	/* bind plt entry */
	import = ADDR(import);
	index = (current_DYNAMIC->a_dlt - current_DYNAMIC->dlt) + (this - current_DYNAMIC->plt);
	if (current_DYNAMIC != exec_DYNAMIC)
		current_DYNAMIC->bound[index] = -1;
#if	DEBUG & 256
	if (bind_immediate)
	{
		ulong *rts = NULL;
		ulong *oldsp;

		oldsp = (ulong *)*(&plt_entry_addr + 3);
		rts = oldsp + 3;
		printf("bor() access to %s from library 0x%08X; rts = 0x%08X\n",import->name+ADDR(string),current_DYNAMIC,*rts);
	}
#endif
	if (!findsym(import+index,current_DYNAMIC,IGNORE_SHL_PROC,&target_DYNAMIC,&target_export))
		fatal(ENOSYM,ADDR(string)+import[index].name);
#ifdef	HOOKS
	addr = resolve(target_DYNAMIC,target_export);
	if (!(dld_flags & DLD_NOFIXPLT))
	{
		this->displacement = addr - (ulong)&this->displacement;
#else
		this->displacement = (addr = resolve(target_DYNAMIC,target_export)) - (ulong)&this->displacement;
#endif
		this->opcode = BRA;
#ifdef	FLUSH_CACHE
		flush_cache(this,cpu);
#endif
#ifdef	PURGE_CACHE
		/* purge instruction cache so patched plt is recognized */
		(void)cachectl(CC_IPURGE,NULL,0);
#endif
#ifdef  HOOKS
	}
	if (dld_hook != NULL && !in_hook)
	{
		in_hook = 1;
		(*dld_hook)(SHL_BOR,ADDR(string)+import[index].name,addr,(shl_t)target_DYNAMIC);
		in_hook = 0;
	}
#endif
	return((char *)addr);
}
