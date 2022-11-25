/* @(#) $Revision: 70.2 $ */

/*
 *	load.c
 *
 *	Series 300 dynamic loader
 *
 *	code to attach shared libraries to a process
 *
 */


#include	"dld.h"

#include	<fcntl.h>
#include	<sys/unistd.h>
#include	<sys/stat.h>


#ifdef SHLIB_PATH
static char shl_name[MAXPATHLEN+1];
#endif

static int open_shl (shlt_t target_shl);
#ifdef SHLIB_PATH
static char *multi_lookup (char *buffer, shlt_t target_shl);
#endif
static int read_shl (int fd, struct exec *filhdr);
static int map_shl (int fd, struct exec *filhdr, caddr_t addr, ulong *target_text, ulong *target_ptext, dl_header_t *target_DL_HEADER);
#pragma	HP_INLINE_NOCODE	open_shl, read_shl, map_shl


struct dynamic *load (shlt_t target_shl, caddr_t addr)
/*
 * on entry:
 *	target_shl = shl entry for library to load
 *	addr = address at which library should be loaded or 0
 * returns:
 *	pointer to DYNAMIC structure of loaded library
 *	NULL on error
 * on exit:
 *	errno is set
 * effects:
 *	maps library text, data, and bss into process
 *	updates DYNAMIC structure
 */
{
	int fd;
	struct exec filhdr;
	ulong target_text, target_ptext;
	dynamic_t current_DYNAMIC;
	dl_header_t current_DL_HEADER;
	char *p;

	/* load library */
	if ((fd = open_shl(target_shl)) == -1)
		return(NULL);
	if (!read_shl(fd,&filhdr))
		return(NULL);
	if (filhdr.a_highwater < target_shl->highwater)
	{
#ifdef	PERROR
		errno = ENOSYS;
#else
		errno = ENOEXEC;
#endif
		return(NULL);
	}
	if (!map_shl(fd,&filhdr,addr,&target_text,&target_ptext,&current_DL_HEADER))
		return(NULL);
	if (close(fd) == -1)
#pragma	BBA_IGNORE
		return(NULL);

	/* initialize data structures */
	current_DYNAMIC = (dynamic_t)(current_DL_HEADER->e_spec.dl_header.dynamic + target_text);
	current_DYNAMIC->dl_header = current_DL_HEADER;
	current_DYNAMIC->shl = target_shl;
	current_DYNAMIC->next_shl = NULL;
	if (current_DYNAMIC->ptext = target_ptext)
	{
		current_DYNAMIC->text = target_ptext;
		current_DYNAMIC->data += target_ptext;
		current_DYNAMIC->bss += target_ptext;
		current_DYNAMIC->end += target_ptext;
		current_DYNAMIC->dlt = (dlt_t)((ulong)current_DYNAMIC->dlt + target_ptext);
		current_DYNAMIC->a_dlt = (dlt_t)((ulong)current_DYNAMIC->a_dlt + target_ptext);
		current_DYNAMIC->plt = (plt_t)((ulong)current_DYNAMIC->plt + target_ptext);
		current_DYNAMIC->bound += target_ptext;
		current_DYNAMIC->dmodule += target_ptext;
#ifdef SHLIB_PATH
		if (current_DYNAMIC->shlib_path != NULL)
		{
			current_DYNAMIC->shlib_path += target_ptext;
			if (target_shl->load == LOAD_LIB)
			{
				strncpy(current_DYNAMIC->shlib_path,shl_name,SHLIB_PATH_LEN-1);
				current_DYNAMIC->shlib_path[SHLIB_PATH_LEN-1] = 0;
#if DEBUG & 1
				printf("just stored %s as name\n",shl_name);
#endif
			}
		}
#endif
	}
#ifndef	__lint
	else
#pragma	BBA_IGNORE
		;
#endif
#ifdef ELABORATOR
	/* handle deferred binding */
	current_DYNAMIC->bor.displacement = (ulong)&shl_bor - (ulong)&(current_DYNAMIC->bor.displacement);
	initializers |= (current_DYNAMIC->status & INITIALIZE);
#endif
	/* don't have a COW, man */
	for (p = (char *)current_DYNAMIC->dmodule; p < (char *)current_DYNAMIC->bound; p += EXEC_PAGESIZE)
		*p = 0;
	for (p = (char *)current_DYNAMIC->bound; p < (char *)current_DYNAMIC->dlt; p += EXEC_PAGESIZE)
		*p = 0;
	for (p = (char *)current_DYNAMIC->a_dlt; p < (char *)current_DYNAMIC->plt; p += EXEC_PAGESIZE)
		*p = 0;
	return(current_DYNAMIC);
}


static int open_shl (shlt_t target_shl)
/*
 * on entry:
 *	target_shl = shl entry for library to load
 * returns:
 *	file handle of shared library
 *	-1 on error
 * on exit:
 *	errno is set
 * effects:
 *	locates and opens file specified by target_shl
 */
{
	dynamic_t current_DYNAMIC;
#ifdef SHLIB_PATH
	const char *shl_name_p;

	if (target_shl->load == LOAD_LIB)
		shl_name_p = multi_lookup(shl_name,target_shl);
	else
	{
		current_DYNAMIC = exec_DYNAMIC;
		shl_name_p = ADDR(string) + target_shl->name;
	}
	return(open(shl_name_p,O_RDONLY));
#else
	if (target_shl->load != LOAD_PATH)
	{
#pragma	BBA_IGNORE
		errno = ENOEXEC;
		return(-1);
	}
	current_DYNAMIC = exec_DYNAMIC;
	return(open(ADDR(string)+target_shl->name,O_RDONLY));
#endif
}


#ifdef SHLIB_PATH
static char *multi_lookup (char *buffer, shlt_t target_shl)
/*
 * on entry:
 *	buffer = buffer to hold path/filename
 *	target_shl = entry for library to find
 * effects:
 *	searches embedded and/or dynamic paths as appropriate for library
 * on exit:
 *	buffer = name of library if found, or original pathname if not found
 * returns:
 *	buffer
 */
{
	dynamic_t current_DYNAMIC;
	const char *original_path;
	const char *basename;

	current_DYNAMIC = exec_DYNAMIC;
	original_path = ADDR(string)+target_shl->name;
	if ((basename = strrchr(original_path,'/')) == NULL)
		basename = original_path;
	else
		++basename;
#if DEBUG & 1
	printf("in multi_lookup - original path = %s, basename = %s\n",original_path,basename);
#endif
	if (current_DYNAMIC->status & DYNAMIC_FIRST)
	{
		if (path_lookup(current_DYNAMIC->shlib_path,buffer,basename,original_path))
			return buffer;
	}
	if (current_DYNAMIC->embed_path)
	{
		if (path_lookup(ADDR(string)+current_DYNAMIC->embed_path,buffer,basename,original_path))
			return buffer;
	}
	if (current_DYNAMIC->status & DYNAMIC_LAST)
	{
		if (path_lookup(current_DYNAMIC->shlib_path,buffer,basename,original_path))
			return buffer;
	}
	strncpy(buffer,original_path,MAXPATHLEN);
	buffer[MAXPATHLEN] = 0;
	return buffer;
}


static int path_lookup (const char *path, char *buffer, const char *basename, const char *original_path)
/*
 * on entry:
 *	path = path to search
 *	buffer = buffer to hold path/filename
 *	basename = basename of library
 *	original path = original pathname of library (for "::" in path)
 * effects:
 *	searches path for library
 * on exit:
 *	buffer = name of library if found
 * returns:
 *	non-zero if found
 *	zero otherwise
 */
{
	int length;
	const char *start, *end;
	struct stat buf;

#if DEBUG & 1
	printf("in path_lookup - path = %s, basename = %s\n",path,basename);
#endif
	start = path;
	length = strlen(path);
	while (start < path + length)
	{
		if ((end = strchr(start,':')) == NULL)
		{
			strncpy(buffer,start,MAXPATHLEN);
			buffer[MAXPATHLEN] = 0;
			strncat(buffer,"/",MAXPATHLEN-strlen(buffer));
			strncat(buffer,basename,MAXPATHLEN-strlen(buffer));
		}
		else if (end == start)
			strncpy(buffer,original_path,MAXPATHLEN);
		else
		{
			int len = (end-start>=MAXPATHLEN)?MAXPATHLEN:end-start;

			strncpy(buffer,start,len);
			buffer[len] = 0;
			strncat(buffer,"/",MAXPATHLEN-strlen(buffer));
			strncat(buffer,basename,MAXPATHLEN-strlen(buffer));
		}
		buffer[MAXPATHLEN] = 0;
#if DEBUG & 1
		printf("in path_lookup - about to stat %s\n",buffer);
#endif
		if (!stat(buffer,&buf))
			return -1;
#if DEBUG & 1
		printf("in path_lookup - darn, that didn't work\n");
#endif
		if (end == NULL)
			break;
		else
			start = end + 1;
	}
	return 0;
}
#endif


static int read_shl (int fd, struct exec *filhdr)
/*
 * on entry:
 *	fd = file descriptor for shared library
 *	filhdr = pointer to allocated buffer
 * returns:
 *	non-zero normally
 *	zero on error
 * on exit:
 *	*filhdr is shared library file header
 *	errno is set
 * effects:
 *	reads shared library file header into *filhdr
 */
{
	int size;

	if ((size = read(fd,(char *)filhdr,sizeof(struct exec))) == -1)
#pragma	BBA_IGNORE
		return(0);
	if (size != sizeof(struct exec))
	{
		errno = ENOEXEC;
		return(0);
	}
	switch (filhdr->a_magic.file_type)
	{
		case SHL_MAGIC:
		case DL_MAGIC:
			break;
		default:
			errno = ENOEXEC;
			return(0);
	}
	return(-1);
}


#ifdef		DEBUG
#define		TEXT_PROT	(PROT_READ|PROT_WRITE|PROT_EXECUTE)
#define		TEXT_MAP	(MAP_PRIVATE)
#else		/* DEBUG */
#define		TEXT_PROT	(PROT_READ|PROT_EXECUTE)
#define		TEXT_MAP	(MAP_SHARED)
#endif		/* DEBUG */

#define		ZERO_MAP	(MAP_PRIVATE|MAP_ANONYMOUS)
#define		BSS_MAP		(MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS)

#define		ZERO_PROT	(PROT_NONE)
#define		DATA_PROT	(PROT_READ|PROT_WRITE|PROT_EXECUTE)
#define		BSS_PROT	(PROT_READ|PROT_WRITE|PROT_EXECUTE)

#define		DATA_MAP	(MAP_PRIVATE|MAP_FIXED)

#ifdef	TEXT_OFFSET
#undef	TEXT_OFFSET
#define	TEXT_OFFSET(hdr)	(EXEC_ALIGN(sizeof(hdr)))
#endif
#ifdef	DATA_OFFSET
#undef	DATA_OFFSET
#define	DATA_OFFSET(hdr)	(EXEC_ALIGN(sizeof(hdr))+\
				 EXEC_ALIGN((hdr).a_text))
#endif


static int map_shl (int fd, struct exec *filhdr, caddr_t addr, ulong *target_text, ulong *target_ptext, dl_header_t *target_DL_HEADER)
/*
 * on entry:
 *	fd = file descriptor for shared library
 *	filhdr = pointer to shared library file header
 *	addr = address at which library should be loaded or 0
 * returns:
 *	non-zero normally
 *	zero on error
 * on exit:
 *	target_text = text of loaded library
 *	target_ptext = pseudo "file text" of loaded library
 *	target_DL_HEADER = DL_HEADER of loaded library
 *	errno is set
 * effects:
 *	maps text, data, and bss segments contiguously
 *	text is mapped shared read only
 *	data and bss are mapped private read/write
 *	DL_HEADER and related sections are mapped shared read only
 */
{
	int tsize, dsize, bsize;
	int zmap = ZERO_MAP;
	int tmap = TEXT_MAP;
	int tprot = TEXT_PROT;

	tsize = EXEC_ALIGN(filhdr->a_text);
	dsize = EXEC_ALIGN(filhdr->a_data);
	bsize = EXEC_ALIGN(filhdr->a_bss);
	if (filhdr->a_magic.file_type == DL_MAGIC
#ifdef	HOOKS
	    || (dld_flags & DLD_PRIVATE)
#endif
	   )
	{
		tprot |= PROT_WRITE;
		tmap &= ~(MAP_SHARED);
		tmap |= MAP_PRIVATE;
	}
	if (!addr)
		addr = (caddr_t)filhdr->a_entry;
	if (addr)
	{
		zmap |= MAP_FIXED;
		tmap |= MAP_FIXED;
	}

	if (risky_load)
	{
#if	DEBUG & 1
		printf("about to \"risky load\" text segment\n");
#endif
		if ((long)(addr = mmap(addr,(int)filhdr->a_text,tprot,tmap,fd,(off_t)TEXT_OFFSET(*filhdr))) == -1)
#if	DEBUG & 1
		{
			printf("sorry - addr = %08X, errno = %08X\n",addr,errno);
#endif
			return(0);
#if	DEBUG & 1
		}
		printf("succeeded - addr = %08X\n",addr);
#endif
	}
	else
	{
#if	DEBUG & 1
		printf("no \"risky load\" - about to load big block\n");
#endif
		/* first map everything */
		if ((long)(addr = mmap(addr,tsize+dsize+bsize,ZERO_PROT,zmap,zero_fd,(off_t)0)) == -1)
			return(0);
#if	DEBUG & 1
		printf("succeeded - addr = %08X\n",addr);
#endif
		/* now replace text */
#ifdef MAP_REPLACE_SUPPORTED
		tmap |= (MAP_REPLACE|MAP_FIXED);
#else
		if (munmap(addr,tsize+dsize+bsize) == -1)
			return 0;
#endif
		if (mmap(addr,(int)filhdr->a_text,tprot,tmap,fd,(off_t)TEXT_OFFSET(*filhdr)) != addr)
			return(0);
	}
	/* next map data */
	if (mmap(addr+tsize,(int)filhdr->a_data,DATA_PROT,DATA_MAP,fd,(off_t)DATA_OFFSET(*filhdr)) != addr + tsize)
#pragma	BBA_IGNORE
		return(0);
	/* finally map bss */
	if (bsize && mmap(addr+tsize+dsize,(int)filhdr->a_bss,BSS_PROT,BSS_MAP,zero_fd,(off_t)0) != addr + tsize + dsize)
#pragma	BBA_IGNORE
		return(0);
	*target_text = (ulong)addr;
	if (filhdr->a_entry)
#pragma	BBA_IGNORE
		*target_ptext = 0;
	else
		*target_ptext = (ulong)addr;
	*target_DL_HEADER = (dl_header_t)addr;
	return(-1);
}
