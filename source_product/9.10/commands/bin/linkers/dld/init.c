/* @(#) $Revision: 70.4 $ */

/*
 *	init.c
 *
 *	Series 300 dynamic loader
 *
 *	intialization code
 *
 */


#include	"dld.h"

#include	<sys/unistd.h>
#include	<sys/signal.h>

#ifdef	PROFILING
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>

char prof_buf[0x10000];
#endif

#include	<sys/cache.h>

#if	DEBUG
#include	<stdio.h>
#endif


#ifdef	PERROR
extern char *sys_errlist[];
#endif


static int bootstrap (dl_header_t my_DL_HEADER, ulong my_text);
#pragma	HP_INLINE_NOCODE	bootstrap
#pragma	HP_INLINE_OMIT		fatal

void shl_term (void);

#ifdef	BBA
extern void _bA_dump (void);
#endif


#ifdef	SEARCH_ORDER
dynamic_t head_DYNAMIC;
#endif
dynamic_t exec_DYNAMIC;
const char *badsym = "<a.out>";
jmp_buf except;
int dld_last_hash;
int g_eindex;
int risky_load;
ulong dirty_sp;

#ifdef	ELABORATOR
int initializers;
#endif

#ifdef	HOOKS
unsigned long dld_flags;
int (*dld_hook)();
#endif

#ifdef	__lint
struct dynamic _DYNAMIC;
void shl_bor (void)
{
	return;
}
#endif


int shl_init (dl_header_t my_DL_HEADER, ulong my_text, dl_header_t main_DL_HEADER, ulong main_text
)
/*
 * on entry:
 *	my_DL_HEADER = DL_HEADER of dld
 *	my_text = text of dld
 *	main_DL_HEADER = DL_HEADER of incomplete executable
 *	main_text = text of incomplete executable
 * returns:
 *	zero normally
 *	non-zero or abort on error
 * on exit:
 *	exec_DYNAMIC = DYNAMIC structure of incomplete executable
 * effects:
 *	bootstraps the dynamic loader
 *	loads required shared libraries
 *	binds all BIND_IMMEDIATE symbols, sets up other for deferred binding
 */
{
	int rc;
	dynamic_t current_DYNAMIC, target_DYNAMIC;
	shlt_t target_shl, end_shl;
#ifdef SHLIB_PATH
	char *shlib_path_p;
#endif

	/* bootstrap dld */
	if (rc = bootstrap(my_DL_HEADER,my_text))
		return(rc);

	dirty_sp = getsp();
#if	DEBUG
	setbuf(stdout,NULL);
#endif
#ifdef	PROFILING
	profil(prof_buf,sizeof(prof_buf),my_text,0177777);
#endif
	errno = 0;
	/* get incomplete executable DYNAMIC */
	exec_DYNAMIC = (dynamic_t)(main_DL_HEADER->e_spec.dl_header.dynamic + main_text);
#ifdef	SEARCH_ORDER
	head_DYNAMIC =
#endif
	current_DYNAMIC = exec_DYNAMIC;
	current_DYNAMIC->dl_header = main_DL_HEADER;
	current_DYNAMIC->text = main_text;
	current_DYNAMIC->ptext = main_text;
	current_DYNAMIC->data += main_text;
	current_DYNAMIC->bss += main_text;
	current_DYNAMIC->end += main_text;
	current_DYNAMIC->dlt = (dlt_t)((ulong)current_DYNAMIC->dlt + main_text);
	current_DYNAMIC->a_dlt = (dlt_t)((ulong)current_DYNAMIC->a_dlt + main_text);
	current_DYNAMIC->plt = (plt_t)((ulong)current_DYNAMIC->plt + main_text);
	current_DYNAMIC->shl = ADDR(shlt);
#ifdef ELABORATOR
	/* handle deferred binding */
	current_DYNAMIC->bor.displacement = (ulong)&shl_bor - (ulong)&(current_DYNAMIC->bor.displacement);
#endif
#ifdef	HOOKS
	dld_flags = current_DYNAMIC->dld_flags;
	dld_hook = (int (*)())current_DYNAMIC->dld_hook;
	exec_DYNAMIC->dld_list = (unsigned long)&head_DYNAMIC;
#endif
#ifdef SHLIB_PATH
	if (((current_DYNAMIC->status & DYNAMIC_FIRST) || (current_DYNAMIC->status & DYNAMIC_LAST)) && current_DYNAMIC->shlib_path != NULL)
	{
		current_DYNAMIC->shlib_path + main_text;
		environ = *(char ***)(current_DYNAMIC->shlib_path);
		if ((shlib_path_p = getenv("SHLIB_PATH")) != NULL)
			strncpy(current_DYNAMIC->shlib_path,shlib_path_p,SHLIB_PATH_LEN);
		else
			*(current_DYNAMIC->shlib_path) = 0;
	}
	else
		current_DYNAMIC->shlib_path = NULL;
#endif

	/* attach each library */
	target_DYNAMIC = current_DYNAMIC;
	target_shl = current_DYNAMIC->shl;
	end_shl = (shlt_t)ADDR(import);
	risky_load = -1;
	while (++target_shl != end_shl)
	{
		/* load library */
		if ((target_DYNAMIC->next_shl = load(target_shl,(caddr_t)0)) == NULL)
		{
#ifdef	PERROR
			fatal(errno,ADDR(string)+target_shl->name);
#else
			fatal(ENOENT,ADDR(string)+target_shl->name);
#endif
		}
		target_DYNAMIC = target_DYNAMIC->next_shl;
	}
	risky_load = 0;

#ifdef	ELABORATOR
	(void)cachectl(CC_FLUSH,NULL,0);
	if (initializers)
	{
		target_DYNAMIC = current_DYNAMIC;
		while ((target_DYNAMIC = target_DYNAMIC->next_shl) != NULL)
		{
			if (target_DYNAMIC->status & INITIALIZE)
			{
				if (!bind_initial(target_DYNAMIC))
					fatal(errno,badsym);
			}
			else
				target_DYNAMIC->initializer = -1;
		}
	}
#endif

	/* bind each file */
	target_DYNAMIC = current_DYNAMIC;
	do
	{
		/* bind file symbols */
		if (!bindfile(target_DYNAMIC))
			fatal(errno,badsym);
	} while ((target_DYNAMIC = target_DYNAMIC->next_shl) != NULL);
#ifndef	ELABORATOR
	(void)cachectl(CC_FLUSH,NULL,0);
#endif

	return(0);
}


static int bootstrap (dl_header_t my_DL_HEADER, ulong my_text)
/*
 * on entry:
 *	my_DL_HEADER = DL_HEADER of dld
 *	my_text = text of dld
 * requires:
 *	dld has no PLT, and no imports for DLT or dynamic relocation
 * returns:
 *	zero normally
 *	non-zero on error
 * effects:
 *	initializes DLT for dld
 *	processes all dynamic relocation records for dld
 */
{
	dynamic_t current_DYNAMIC;
	dlt_t dlt, end_dlt;
	dreloc_t dreloc;

	/* get dld DYNAMIC */
	current_DYNAMIC = (dynamic_t)(my_DL_HEADER->e_spec.dl_header.dynamic + my_text);
	current_DYNAMIC->dl_header = my_DL_HEADER;
	current_DYNAMIC->text = my_text;
	current_DYNAMIC->ptext = my_text;
	current_DYNAMIC->data += my_text;
	current_DYNAMIC->bss += my_text;
	current_DYNAMIC->end += my_text;
	current_DYNAMIC->dlt = (dlt_t)((ulong)current_DYNAMIC->dlt + my_text);
	current_DYNAMIC->a_dlt = (dlt_t)((ulong)current_DYNAMIC->a_dlt + my_text);
	current_DYNAMIC->plt = (plt_t)((ulong)current_DYNAMIC->plt + my_text);
	current_DYNAMIC->shl = NULL;

	/* bind dlt */
	end_dlt = (dlt_t)current_DYNAMIC->plt;
	for (dlt = current_DYNAMIC->a_dlt; dlt != end_dlt; ++dlt)
		dlt->address += my_text;

	/* dynamic relocation */
	dreloc = ADDR(dreloc);
	while (dreloc->type != DR_END)
	{
#ifdef	PERROR
		*(long *)(dreloc->address+my_text) += my_text;
#else
		if (!patch(dreloc->address+my_text,dreloc->length,my_text))
			return(ENOEXEC);
#endif
		++dreloc;
	}
	return(0);
}


#ifdef	ELABORATOR
int bind_initial (dynamic_t current_DYNAMIC)
/*
 * on entry:
 *	current_DYNAMIC = library to initialize
 * returns:
 *	zero on error
 *	non-zero normally
 * effects:
 *	looks up elaborator
 *	looks up and executes initializer
 */
{
	import_t import;
	dynamic_t target_DYNAMIC;
	export_t target_export;

	if (current_DYNAMIC->elaborator != -1)
	{
		import = ADDR(import) + current_DYNAMIC->elaborator;
#if	DEBUG & 512
		printf("finding elaborator for 0x%08X...",current_DYNAMIC);
#endif
		if (!findsym(import,current_DYNAMIC,0,&target_DYNAMIC,&target_export))
		{
			badsym = ADDR(string) + import->name;
			errno = ENOSYM;
			return(0);
		}
		current_DYNAMIC->elaborator = resolve(target_DYNAMIC,target_export);
#if	DEBUG & 512
		printf("it's 0x%08X\n",current_DYNAMIC->elaborator);
#endif
	}
	if (current_DYNAMIC->initializer != -1)
	{
		import = ADDR(import) + current_DYNAMIC->initializer;
		if (!findsym(import,current_DYNAMIC,0,&target_DYNAMIC,&target_export))
		{
			badsym = ADDR(string) + import->name;
			errno = ENOSYM;
			return(0);
		}
		current_DYNAMIC->initializer = resolve(target_DYNAMIC,target_export);
		(*(int (*)())current_DYNAMIC->initializer)(current_DYNAMIC,1);
	}
	return(1);
}
#endif


void fatal (int code, const char *msg)
/*
 * on entry:
 *	code = error code
 *	msg = message to display
 * effects:
 *	prints message
 *	aborts program
 */
{
#ifndef	PERROR
	char *msgn;
	static char msg0[] = "/lib/dld.sl: fatal error - ";
	static char msg1[] = "/lib/dld.sl: unresolved external - ";
	static char msg2[] = "/lib/dld.sl: unable to load library - ";
#endif
	char buf[1024];

#ifdef	PERROR
	(void)strcpy(buf,"/lib/dld.sl: ");
	(void)strcat(buf,sys_errlist[code]);
	(void)strcat(buf," - ");
#else
	switch (code)
	{
		case ENOSYM:
			msgn = msg1;
			break;
		case ENOENT:
			msgn = msg2;
			break;
		case ENOEXEC:
		default:
			msgn = msg0;
	}
	(void)strcpy(buf,msgn);
#endif
	(void)strcat(buf,msg);
	(void)strcat(buf,"\n");
	(void)write(2,buf,strlen(buf));
#if	0
#ifdef	BBA
	_bA_dump();
#endif
#else
	shl_term();
#endif
	(void)raise(SIGABRT);
}


void shl_term (void)
{
#if	0
#else
	static int already_termed = 0;

	already_termed = 1;
	if (already_termed)
		return;
#endif
#ifdef	PROFILING
	int fd;

	profil(NULL,0,0,0);
	if (!(fd = open("/tmp/prof.data",O_WRONLY|O_CREAT|O_APPEND,0666)))
		fatal(ENOEXEC,"unable to open /tmp/prof.data");
	if (write(fd,prof_buf,sizeof(prof_buf)) != sizeof(prof_buf))
		fatal(ENOEXEC,"unable to write /tmp/prof.data");
	close(fd);
#endif
#ifdef	BBA
	_bA_dump();
#endif
}


#ifdef	BBA
#define		CORESIZE	(1024*16)
static char core[CORESIZE];
static char *end = core + CORESIZE;
void *malloc (size_t nbytes)
{
	end -= nbytes;
	return((long)end < (long)core ? NULL : end);
}

void free (void *addr)
{
}
#endif
