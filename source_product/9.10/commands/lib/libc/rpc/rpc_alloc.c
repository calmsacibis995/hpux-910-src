/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 12.2 $	$Date: 91/04/26 09:08:42 $  */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define catopen _catopen

#ifdef _ANSIC_CLEAN
#define free 			_free
#define malloc 			_malloc
#endif /* _ANSIC_CLEAN */

#endif /* _NAME_SPACE_CLEAN */

/*
 * The following set of routines was developed as a front end to
 * malloc and/or free for the RPC/XDR libraries.  They were developed
 * due to a problem with free.  Namely, free does not check the pointer
 * given it to make sure that it was a pointer allocated to it by
 * malloc(3).  Because of the structure of the XDR routines, we need to
 * have this condition checked.  For example, in the routine xdr_string(),
 * in the XDR_DECODE case.  If xdr_string is given a NULL pointer, it 
 * mem_alloc()'s space for the string, but ONLY if given a NULL pointer.
 * Otherwise, it assumes that the pointer is to some valid place in memory
 * to put the string.  However, in the XDR_FREE case, it has no way of
 * knowing whether the pointer given to it is one that it should free
 * or not.  NOTE: On Sun and/or 4.[23]BSD systems, the malloc()/free() calls
 * do this check for you, thus this was not a problem for them.  On
 * HP-UX systems, handing free() an unknown pointer is asking for trouble.
 * 
 * These routines should be referenced in the RPC/XDR code via the
 * mem_alloc() and mem_free() macros.  This way if the real malloc() and/or
 * free() routines are fixed to handle this, these routines can go away
 * and the macros adjusted to use the real routines.  See <rpc/types.h>
 *
 * NOTE:  These routines are not intended to be used by the KERNEL.
 *
 * Created by Darren Smith, April 3, 1987 for the NFS project.
 *
 */

 /*
  * To keep track of what we have allocated, create a linked list.  To be
  * at least reasonable efficient with our calls to malloc, we simply tack
  * the data structure on the front of what is allocated (allocating extra
  * space for it), and have the data portion line up with the space we are
  * giving back to the calling routine.  By having the data portion be an
  * array, we can reference linked_list->data WITHOUT a subscript and get
  * the address of the array, i.e. the address of the space allocated to
  * the calling routine.  This is somewhat obscure, but prevents us from
  * having to do multiple malloc() calls or keep extra data around.
  * FYI: this is actually the same trick malloc() uses for its structures.
  */
/* To avoid including all of stdio.h, just define NULL as 0 */
#define NULL 0

struct linked_list {
	struct linked_list *next;
#if defined(hp9000s700) || defined(hp9000s800)
	long placeholder; /* make sure that data is 8 byte aligned! */
#endif
	char data[1];
};

static struct linked_list *head = NULL;

char *malloc();
void free();


char *
_rpc_malloc( size )
int size;
{
	struct linked_list *current;
	int alloc_size;

	alloc_size = size + sizeof(struct linked_list);
	current = (struct linked_list *) malloc( alloc_size );

	if ( current == NULL )
		return (NULL);

	current->next = head;
	head = current;

	return(current->data);
}

void
_rpc_free(ptr)
char *ptr;
{
	struct linked_list *current;
	struct linked_list *previous=NULL;	/* used for removing from list*/

	if ( ptr == NULL ) 
		return;

	/* search list for ptr.  Remove and free if found */
	for( current=head; current != NULL; current=current->next ) {
		if ( current->data == ptr ) {
			if ( previous != NULL )	/* curr not at head of list */
				previous->next = current->next;
			else
				head = current->next;
			free( current );
			break;
		}
		previous = current;
	}
}

/* This routine was developed as a front end to the NLS processing for 	*/
/* all the NFS routines in libc.					*/

#include <nl_types.h>
#include <rpc/rpc.h>

/* File descriptor for the NFS libc message catalog */
static nl_catd nfs_nlmsg_fd;

/* When calling library routines multiple times we do not want to   */
/* open the message catalog more than once.  Trying to open the     */
/* catalog only once speeds up the NLS portion of the routines.	    */
static bool_t first_time = TRUE;

nl_catd
_nfs_nls_catopen()
{
	if (first_time)
	{
		nfs_nlmsg_fd = catopen("libc_nfs",0);
		first_time = FALSE;
	}
	return(nfs_nlmsg_fd);
}

