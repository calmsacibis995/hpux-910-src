/* @(#) $Revision: 70.3 $ */
#ifndef _SEARCH_INCLUDED
#define _SEARCH_INCLUDED	/* Allows search.h to be multiply included */

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_XOPEN_SOURCE

#  ifndef _SIZE_T
#    define _SIZE_T
     typedef unsigned int size_t;
#  endif /* _SIZE_T */

/* HSEARCH(3C) */
#  ifdef _XPG4
     typedef struct entry { char *key; void *data; } ENTRY;
#  else /* _XPG4 */
     typedef struct entry { char *key, *data; } ENTRY;
#  endif   /* _XPG4 */

   typedef enum { FIND, ENTER } ACTION;

/* TSEARCH(3C) */
   typedef enum { preorder, postorder, endorder, leaf } VISIT;

#  ifdef _PROTOTYPES 
#    ifdef _XPG4
       extern int hcreate(size_t);
#    else /* _XPG4 */
       extern int hcreate(unsigned);
#    endif /* _XPG4 */

     extern void hdestroy(void);
     extern ENTRY *hsearch(ENTRY, ACTION);
#  ifndef _INCLUDE_AES_SOURCE
     extern void twalk(const void *, void(*)(const void *, VISIT, int));
#  endif /* _INCLUDE_AES_SOURCE */
#  else /* _PROTOTYPES */
     extern int hcreate();
     extern void hdestroy();
     extern ENTRY *hsearch();
     extern void twalk();
#  endif /* _PROTOTYPES */

#  ifdef _CLASSIC_XOPEN_TYPES
     extern char *lfind();
     extern char *lsearch();
     extern char *tsearch();
     extern char *tfind();
     extern char *tdelete();
#  else
#    ifdef _PROTOTYPES
#      ifndef _INCLUDE_AES_SOURCE
          extern void *lfind(const void *, const void *, size_t *, size_t, int(*)(const void *, const void *));
          extern void *lsearch(const void *, void *, size_t *, size_t, int(*)(const void *, const void *));
          extern void *tsearch(const void *, void **, int(*)(const void *, const void *));
          extern void *tfind(const void *, void * const *, int(*)(const void *, const void *));
          extern void *tdelete(const void *, void **, int(*)(const void *, const void *));
#      endif /* not _INCLUDE_AES_SOURCE */
#    else /* _PROTOTYPES */
       extern void *lfind();
       extern void *lsearch();
       extern void *tsearch();
       extern void *tfind();
       extern void *tdelete();
#    endif /* _PROTOTYPES */
#  endif
#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef _INCLUDE_AES_SOURCE
#  ifdef _PROTOTYPES
     extern void twalk(const void **, void(*)(const void *, VISIT, int));
     extern void *lfind(const void *, const void *, size_t *, size_t, int(*)(const void *, const void *));
     extern void *lsearch(const void *, const void *, size_t *, size_t, int(*)(const void *, const void *));
     extern void *tsearch(const void *, const void **, int(*)(const void *, const void *));
     extern void *tfind(const void *, const void **, int(*)(const void *, const void *));
     extern void *tdelete(const void *, const void **, int(*)(const void *, const void *));
#  endif /* _PROTOTYPES */
#endif /* _INCLUDE_AES_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _SEARCH_INCLUDED */
