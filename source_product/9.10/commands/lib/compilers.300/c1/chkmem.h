/* file chkmem.h */
/* @(#) $Revision: 66.6 $ */
/* KLEENIX_ID @(#)chkmem.h	16.1 90/11/05 */

#ifndef    NOCHKMEM
#define    MALLOC_TYPE       char
#define    malloc(a)	     chk_malloc(a,__FILE__,__LINE__)
#define    calloc(a,b)	     chk_calloc(a,b,__FILE__,__LINE__)
#define    realloc(a,b)	     chk_realloc(a,b,__FILE__,__LINE__)
#define    free(a)	         chk_free(a,__FILE__,__LINE__)
MALLOC_TYPE *chk_malloc(), *chk_calloc(), *chk_realloc();
#endif     /* NOCHKMEM */

#ifdef    CHKMEM_COMPILE

#define   SIZE_TYPE            long

static char *side = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#define   MEM_ALIGN            4

#include  <signal.h>
#define   ABORT                kill(getpid(),SIGQUIT)

#define   START_FREE_SIZE      1000

int chk_sides = 1;

#else    /* CHKMEM_COMPILE */

extern int chk_sides;

#endif   /* CHKMEM_COMPILE */
