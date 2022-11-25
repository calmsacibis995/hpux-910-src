/* @(#) $Revision: 72.1 $ */

#ifndef _WORDEXP_INCLUDED
#  define _WORDEXP_INCLUDED

#  ifndef _SYS_STDSYMS_INCLUDED
#    include <sys/stdsyms.h>
#  endif /* _SYS_STDSYMS_INCLUDED */

#  ifdef __cplusplus
     extern "C" {
#  endif

#  if defined( _XPG4 ) || defined ( _INCLUDE_POSIX2_SOURCE )

#    ifndef _SIZE_T
#       define _SIZE_T
        typedef unsigned int size_t;
#    endif /* _SIZE_T */

     typedef struct {
        size_t   we_wordc;        /* Count of words matched by words. */
        char**   we_wordv;        /* Pointer to list of expanded words. */
        size_t   we_offs;         /* Slots to reserve at the beginning of 
                                     we_wordv. */
        } wordexp_t;

/*   flags values */
#    define WRDE_APPEND 0x01 /* Append words generated to the ones from a 
                                previous call to wordexp(). */

#    define WRDE_DOOFFS 0x02 /* Make use of we_offs.  If this flag is set, 
                                we_offs is used to specify how many null 
                                pointers to add to the beginning of we_wordv.
                                In other words, we_wordv will point to 
                                we_offs null pointers, followed by we_wordc 
                                word pointers, followed by a null pointer. */

#    define WRDE_NOCMD  0x04 /* Fail if command substitution, as specified in 
                                X/Open Commands and Utilities, Section 3.x, 
                                Command Substitution, is requested. */

#    define WRDE_REUSE  0x08 /* The pwordexp argument was passed to a previous 
                                successful call to wordexp(), and has not been 
                                passed to wordfree().  The result will be the 
                                same as if the application had called 
                                wordfree() and then called wordexp() without 
                                WRDE_REUSE. */

#    define WRDE_SHOWERR 0x10 /* Do not redirect stderr to /dev/null */
#    define WRDE_UNDEF   0x20 /* Report error on an attempt to expand an 
                                 undefined shell variable. */

/*   Error return values */
#    define WRDE_BADCHAR 1 /* One of the unquoted characters |,&,;,<,>,(),{}
                              appears in words in an inappropriate context. */

#    define WRDE_BADVAL  2 /* Reference to undefined shell variable when 
                              WRDE_UNDEF is set in flags. */

#    define WRDE_CMDSUB  3 /* Command substitution requested when WRDE_NOCMD 
                              was set in flags. */

#    define WRDE_NOSPACE 4 /* Attempt to allocate memory failed. */
#    define WRDE_SYNTAX  5 /* Shell syntax error, such as unbalanced 
                              parentheses or unterminated string. */
#    define WRDE_INTERNAL 6 /* internal errors */
#    define WRDE_NOSYS  -1 /* Return value when function is not supported */

#    ifdef _PROTOTYPES
       extern int wordexp ( const char *, wordexp_t *, int ); 
       extern void wordfree ( wordexp_t * );
#    else /* _PROTOTYPES */
       extern int wordexp();
       extern void wordfree();
#    endif  /* _PROTOTYPES */

#  endif /* defined( _XPG4 ) || defined ( _INCLUDE_POSIX2_SOURCE ) */


#  ifdef __cplusplus
     }
#  endif    /* __cplusplus */


#endif  /* _WORDEXP_INCLUDED */ 
