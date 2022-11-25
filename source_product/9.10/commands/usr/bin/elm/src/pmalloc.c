/**			pmalloc.c			**/

/*
 *  @(#) $Revision: 70.2 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *  This module provides a centralized set of routines from which to allocate
 *  data and to provide a common "recovery" behavior.
 */

#include "headers.h"

#ifdef NLS
#   include <nl_types.h>
#   define NL_SETN  31
    extern nl_catd  nl_fd;
#else
#   define catgets(i,sn,mn,s) (s)
#endif

static char *malloc_panic_str="\n\r\n\rCould not allocate %d bytes for %s!!!\n\r\n\r";

void *
my_malloc( size, for_what )
    size_t size;
    char *for_what;
{
    void *p;

    p = malloc(size);
    if (!p) {
	fprintf(stderr, catgets(nl_fd, NL_SETN, 1, malloc_panic_str),
		size, for_what );
	dprint( 1, (debugfile,
		    "ERROR -- my_malloc(): could not malloc(%d) for %s\n",
		    size, for_what) );
	leave( 1 );
    }

    return(p);
}

/* This routine was created as a generalized way to calloc memory
   and handle the error condition.  It was used only by initialize.c
   and introduced a defect where the softkeys weren't being 
   displayed.  

   Its usage was backed out of initialize.c in rel 70.4.
   This function is no longer being used and is commented out.

void *
my_calloc( nelem, elsize, for_what )
    size_t nelem, elsize;
    char *for_what;
{
    void *p;

    p = calloc(nelem, elsize);
    if (!p) {
	fprintf(stderr, catgets(nl_fd, NL_SETN, 1, malloc_panic_str),
		nelem*elsize, for_what );
	dprint( 1, (debugfile,
		    "ERROR -- my_calloc(): could not calloc(%d,%d) for %s\n",
		    nelem, elsize, for_what) );
	leave( 1 );
    }

    return(p);
}

*/  /* Commented out my_calloc() function, retired from service */

void *
my_realloc( oldp, size, for_what )
    void *oldp;
    size_t size;
    char *for_what;
{
    void *p;

    p = realloc(oldp, size);

    if (!p) {
	fprintf(stderr, catgets(nl_fd, NL_SETN, 1, malloc_panic_str),
		size, for_what );
	dprint( 1, (debugfile,
		    "ERROR -- my_realloc(): could not realloc(0x%x,%d) for %s\n",
		    oldp, size, for_what) );
	leave( 1 );
    }

    return(p);
}
