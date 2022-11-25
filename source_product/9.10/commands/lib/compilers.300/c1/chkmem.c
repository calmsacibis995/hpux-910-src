/* file chkmem.c */
/* @(#) $Revision: 66.6 $ */
/* KLEENIX_ID @(#)chkmem.c	16.1 90/11/05 */
/*======================================================================
 *
 * Package: chkmem.c                         (Checks Memory Allocations)
 * By: Greg Lindhorst, June 1989
 *
 * Description:
 *   This package (chkmem.c and chkmem.h) are designed to aid in 
 *   finding pesky malloc() problems and stray pointers. 
 *
 */

#include   <stdio.h>
#include   <assert.h>

#define    CHKMEM_COMPILE   
#include   "chkmem.h"       
#undef     malloc            /* We don't want to call ourselves recursevily */
#undef     realloc
#undef     calloc
#undef     free

/* Error message types.  Both fatal and internal do not return */
#define    warning( ptr, msg )         error_msg( 0, ptr, msg )
#define    fatal( ptr, msg )           error_msg( 1, ptr, msg )
#define    internal( ptr, msg )        error_msg( 2, ptr, msg )

struct mem {
	MALLOC_TYPE     *m, *um;             /* Internal and User Pointers */
	SIZE_TYPE       size, usize;         /* Internal and User Sizes */
	char            *afile, *ffile;      /* Allocated and Freed File Names */
	unsigned int    aline, fline;        /* Allocated and Freed Line Numbers */
	int             status;              /* Status word, see STATUS_* below */
	struct mem      *next;               /* Next Pointers */
    };

#define   STATUS_INUSE         1
#define   STATUS_FREED         2
#define   STATUS_REALLOC       3
#define   STATUS_BOGUS         100

static struct mem *free_ptr, *alloc_ptr, start_free[ START_FREE_SIZE ];
static int inited, side_size;
static char *_proc, *_file;
static int _line;

MALLOC_TYPE *malloc(), *realloc(), *calloc();
static struct mem *alloc_mem(), *check_mem();
static void free_mem(), write_sides(), internal_check_sides();
void check_sides();

MALLOC_TYPE *chk_malloc( size, file, line )
SIZE_TYPE size;
char *file;
unsigned int line;
{
	struct mem *ptr;
	MALLOC_TYPE *s;

	_proc = "malloc";
	_file = file;
	_line = line;
	if( !inited ) init();
	if( chk_sides ) check_sides();

	if( size == 0 )
	{
		warning( NULL, "Unusual Size Requested (size == 0)" );
		return( malloc( size ) );
	}

	if( (s = malloc( size+(2*side_size) )) == NULL )
	{
		warning( NULL, "No More Memory (NULL Returned)" );
		return( NULL );
	}

	if( !(ptr = check_mem( s )) )
		ptr = alloc_mem();
	ptr->m = s;
	ptr->um = (MALLOC_TYPE *) ((char *)s+side_size);
	ptr->size = size+(2*side_size);
	ptr->usize = size;
	ptr->afile = file;
	ptr->aline = line;
	ptr->status = STATUS_INUSE;
	write_sides( ptr );

	return( ptr->um );
}

MALLOC_TYPE *chk_calloc( size, nelem, file, line )
SIZE_TYPE size;
SIZE_TYPE nelem;
char *file;
unsigned int line;
{
	register struct mem *ptr;
	MALLOC_TYPE *s;
	SIZE_TYPE new_nelem;

	_proc = "calloc";
	_file = file;
	_line = line;
	if( !inited ) init();
	if( chk_sides ) check_sides();

	if( size == 0 || nelem == 0 )
	{
		warning( NULL, "Unusual Size Requesed (size == 0)" );
		return( calloc( size, nelem ) );
	}

	new_nelem = nelem + (((2*side_size)+size)/size);
	if( (s = calloc( size, new_nelem )) == NULL )
	{
		warning( NULL, "No More Memory (NULL Returned)" );
		return( NULL );
	}

	if( !(ptr = check_mem( s )) )
		ptr = alloc_mem();
	ptr->m = s;
	ptr->um = (MALLOC_TYPE *) ((char *)s+side_size);
	ptr->size = size * new_nelem;
	ptr->usize = size * nelem;
	ptr->afile = file;
	ptr->aline = line;
	ptr->status = STATUS_INUSE;
	write_sides( ptr );

	return( ptr->um );
}
	
MALLOC_TYPE *chk_realloc( um, size, file, line )
MALLOC_TYPE *um;
SIZE_TYPE size;
char *file;
unsigned int line;
{
	register struct mem *ptr, *ptr2;
	MALLOC_TYPE *sp;

	_proc = "realloc";
	_file = file;
	_line = line;
	if( !inited ) init();
	if( chk_sides ) check_sides();

	if( size == 0 )
		fatal( NULL, "Unusual Size Requested (size == 0)" );

	for( ptr = alloc_ptr; ptr && ptr->um != um; ptr = ptr->next ) ;
	if( ptr == NULL )
		fatal(NULL, "Can't find the original chkmem entry for this pointer");
	internal_check_sides( ptr );

	if( ptr->size == size )
		warning( ptr,"Realloc with the same size as the original allocation" );
		
	if( (sp = realloc( ptr->m, size + (2*side_size) )) == NULL )
		fatal( ptr, "Realloc Returned a NULL pointer" );

	ptr->status = STATUS_REALLOC;
	if( (ptr2 = check_mem( sp )) != NULL )
		free_mem( ptr2 );
	
	ptr->m = sp;
	ptr->um = (MALLOC_TYPE *) ((char *)sp+side_size);
	ptr->size = size + (2*side_size);
	ptr->usize = size;
	ptr->afile = file;
	ptr->aline = line;
	ptr->status = STATUS_INUSE;
	write_sides( ptr );
	
	return( ptr->um );
}

void chk_free( sp, file, line )
MALLOC_TYPE *sp;
char *file;
unsigned int line;
{
	register struct mem *ptr;

	_proc = "free";
	_file = file;
	_line = line;
	if( !inited ) init();
	if( chk_sides ) check_sides();

	for( ptr = alloc_ptr; ptr && ptr->um != sp; ptr = ptr->next ) ;
	if( ptr == NULL )
		fatal( NULL, "Free of a non-malloced pointer" );
	if( ptr->status == STATUS_FREED )
		fatal( ptr, "Free of an already freed pointer" );

	internal_check_sides( ptr );
	free( ptr->m );
	ptr->status = STATUS_FREED;
	ptr->ffile = file;
	ptr->fline = line;
}

static struct mem *check_mem( s )
MALLOC_TYPE *s;
{
	register struct mem *ptr, *found;

	for( ptr = alloc_ptr, found = NULL; ptr; ptr = ptr->next )
	{
		if( ptr->m == s )
		{
			switch( ptr->status )
			{
			  case STATUS_INUSE:
				fatal( ptr, "A non-freed pointer was returned" );
				break;
			  case STATUS_FREED: 
				if( found == NULL )
					found = ptr;
				else
					internal( ptr, "Multiple Entries in Chkmem Table" );
			  case STATUS_REALLOC:
				break;
			  default:
				internal( ptr, "Internal Error in Chkmem" );
				break;
			}
		}
		else if( s > ptr->m && s < (ptr->m+ptr->size) )
		{
			switch( ptr->status )
			{
			  case STATUS_INUSE:
				fatal( ptr, "Pointer returned is within another memory area" );
				break;
			  case STATUS_FREED:
			  case STATUS_REALLOC:
				break;
			  default:
				internal( ptr, "Internal Error in Chkmem" );
				break;
			}
		}
	}

	return( found );
}
			
static struct mem *alloc_mem()
{
	register struct mem *ptr;
	struct mem temp_mem;
	int i;
	char *save_proc;

	if( free_ptr == NULL )
	{
		warning( NULL, "Chkmem free list is empty, allocating some more" );

		/* We need one entry so that the following chk_malloc() will not
         * fail.  We copy this entry over when we return into one of the
         * newly allocated slots.
         */
		free_ptr = &temp_mem;
		temp_mem.next = NULL;
		temp_mem.status = STATUS_BOGUS;
		
		save_proc = _proc;
		_proc = "INTERNAL alloc_mem";
		if( (ptr = (struct mem *) 
			       chk_malloc(START_FREE_SIZE*sizeof(struct mem),
					                                _file,_line)) == NULL )
		{
			fatal( NULL, "Out of memory in alloc_mem()" );
		}
		_proc = save_proc;

		for( free_ptr = ptr, i = 1; i < START_FREE_SIZE; i++, ptr++ )
			ptr->next = ptr+1;
		ptr->next = NULL;

		/* Now copy over what is in temp_mem into a newly allocated slot.
         * Also, allocate this new slot and place it on our main linked list
         */
		if( temp_mem.status == STATUS_INUSE )
		{
			alloc_ptr = free_ptr;
			free_ptr = free_ptr->next;
			memcpy( alloc_ptr, &temp_mem, sizeof( struct mem ) );
		}
	}

	ptr = free_ptr;
	free_ptr = free_ptr->next;

	ptr->next = alloc_ptr;
	alloc_ptr = ptr;

	ptr->m = ptr->um = NULL;
	ptr->afile = ptr->ffile = NULL;
	ptr->size = ptr->usize = ptr->aline = ptr->fline = 0;
	ptr->status = STATUS_BOGUS;

	return( ptr );
}

static void free_mem( ptr )
struct mem *ptr;
{
	register struct mem *ptr2, *ptr3;

	for( ptr2 = alloc_ptr, ptr3 = NULL; ptr2 && ptr2 != ptr;
		                                    ptr3 = ptr2, ptr2 = ptr2->next ) ;
	if( ptr2 == ptr )
	{
		if( ptr3 == NULL )
			alloc_ptr = ptr->next;
		else
			ptr3->next = ptr->next;
	}
	else
		internal( ptr, "free_mem: cannot find this pointer in alloced list" );

	ptr->next = free_ptr;
	free_ptr = ptr;
}

static void write_sides( ptr )
struct mem *ptr;
{
	if( side_size )
	{
		strncpy( ptr->um-side_size, side, side_size );
		strncpy( ptr->um+ptr->usize, side, side_size );
	}
}

static void internal_check_sides( ptr )
struct mem *ptr;
{
	if( strncmp( ptr->um-side_size, side, side_size ) )
		fatal( ptr, "check_sides: pre sides corrupt" );
	if( strncmp( ptr->um+ptr->usize, side, side_size ) )
		fatal( ptr, "check_sides: post sides corrupt" );
}

void check_sides()
{
	register struct mem *ptr;

	if( !inited ) init();
	
	for( ptr = alloc_ptr; ptr; ptr = ptr->next )
		if( ptr->status != STATUS_FREED )
			internal_check_sides( ptr );
}

static error_msg( type, ptr, format )
int type;
struct mem *ptr;
char *format;
{
	FILE *fp;

#ifdef   LOGFILE
	if( (fp = fopen( LOGFILE, "a" )) == NULL )
	{
		fprintf(stderr,"chkmem: error: unable to open log '%s'\n",LOGFILE);
		ABORT;
	}
#else
	fp = stderr;
#endif

	fprintf( fp, "%schkmem: %s: %s\n", (type == 2 ? "INTERNAL " : ""), 
	                                   (_proc == NULL ? "" : _proc),
			                           format );
	if( _file != NULL )
	    fprintf(fp, "\t   callee: %s, line %d\n", _file, _line );
	if( ptr != NULL )
	{
		if( ptr->afile != NULL )
			fprintf(fp, "\tallocated: %s, line %d\n", ptr->afile, ptr->aline);
		if( ptr->ffile != NULL )
			fprintf(fp, "\t    freed: %s, line %d\n", ptr->ffile, ptr->fline);
	}

	fflush( fp );
#ifdef  LOGFILE
	fclose( fp );
#endif

	if( type != 0 )
		ABORT;
}	
	
static init()
{
	register int i;
	register struct mem *ptr;

	for( ptr = free_ptr = start_free, i = 1; i < START_FREE_SIZE; i++,ptr++ )
		ptr->next = ptr+1;
	ptr->next = NULL;

	if( side == NULL )
		side_size = 0;
	else
	{
		side_size = strlen( side );
		if( side_size % MEM_ALIGN )
			side_size = side_size + (MEM_ALIGN - (side_size % MEM_ALIGN));
	}

	inited = 1;
}

	

	
