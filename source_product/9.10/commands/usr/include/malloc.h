/* @(#) $Revision: 70.2 $ */       
#ifndef _MALLOC_INCLUDED
#define _MALLOC_INCLUDED /* allows multiple inclusion */

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#include <sys/types.h> /* for size_t type def */

#ifdef __cplusplus
extern "C" {
#endif

/*
	Constants defining mallopt operations
*/
#define M_MXFAST	1	/* set size of blocks to be fast */
#define M_NLBLKS	2	/* set number of block in a holding block */
#define M_GRAIN		3	/* set number of sizes mapped to one, for
				   small blocks */
#define M_KEEP		4	/* retain contents of block after a free until
				   another allocation */
#define M_BLOCK		5	/* enable signal blocking in all malloc
				   related routines */
#define M_UBLOCK	6	/* disable signal blocking set by M_BLOCK */

#ifndef _STRUCT_MALLINFO
#  define _STRUCT_MALLINFO

   /* structure filled by */
   struct mallinfo  {
	int arena;	/* total space in arena */
	int ordblks;	/* number of ordinary blocks */
	int smblks;	/* number of small blocks */
	int hblks;	/* number of holding blocks */
	int hblkhd;	/* space in holding block headers */
	int usmblks;	/* space in small blocks in use */
	int fsmblks;	/* space in free small blocks */
	int uordblks;	/* space in ordinary blocks in use */
	int fordblks;	/* space in free ordinary blocks */
	int keepcost;	/* cost of enabling keep option */
   };	
#endif /* _STRUCT_MALLINFO */

#if defined(__STDC__) || defined(__cplusplus)
   void free(void *);
   int mallopt(int, int);
   struct mallinfo mallinfo(void);
   void memorymap(int);
#else /* __STDC__ */
   void free();
   int mallopt();
   struct mallinfo mallinfo();
   void memorymap();
#endif /* __STDC__ */

#ifdef _CLASSIC_ANSI_TYPES
   char *calloc();
   char *malloc();
   char *realloc();
#else
#  if defined(__STDC__) || defined(__cplusplus)
     void *calloc(size_t, size_t);
     void *malloc(size_t);
     void *realloc(void *, size_t);
#  else /* not __STDC__ || __cplusplus */
     void *calloc();
     void *malloc();
     void *realloc();
#  endif /* else not __STDC__ || __cplusplus */
#endif

#ifdef __cplusplus
}
#endif

#endif /* _MALLOC_INCLUDED */
