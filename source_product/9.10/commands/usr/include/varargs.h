/* @(#) $Revision: 70.5 $ */
#ifndef _VARARGS_INCLUDED
#define _VARARGS_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef _STDARG_INCLUDED
#  ifdef __hp9000s300
#    ifndef _VA_LIST
#      define _VA_LIST
       typedef char *va_list;
#    endif  /* _VA_LIST */

#    ifdef __cplusplus
#      ifdef __compile_mode

         extern "C" {
             void __builtin_va_start(va_list, ...);
         }
#        define va_start(__list,__parmN) (__list=0,__builtin_va_start(__list,&__parmN),__list-=4)
#      else  /* ! __compile_mode */
#        define va_start(__list,__parmN) __list = (char *) ((char *)(&__parmN) \
            + sizeof(__parmN))
#      endif /*  __compile_mode */
#    else /* ! __cplusplus */
#      define va_start(__list,__parmN) __list = (char *) ((char *)(&__parmN) \
 	  + sizeof(__parmN))
#    endif /* ! __cplusplus */

#    define va_arg(__list,__mode) ((__mode *)(__list += sizeof(__mode)))[-1]
#    define va_end(__list)
#  endif /* __hp9000s300 */

#  ifdef __hp9000s800

#  ifdef __lint
#    ifndef _VA_LIST
#      define _VA_LIST
       typedef double *va_list;
#    endif /* _VA_LIST */

#    define va_start(__list,__parmN) __list = (double *) ((double *)(&__parmN)\
	+ sizeof(__parmN))
#    define va_arg(__list,__mode) ((__mode *)(__list += sizeof(__mode)))[-1]
#    define va_end(__list)
#  else /* ! __lint */

#    define __WORD_MASK 0xFFFFFFFC
#    define __DW_MASK   0xFFFFFFF8

#    ifndef _VA_LIST
#      define _VA_LIST
       typedef double *va_list;
#    endif /* _VA_LIST */

/* Args > 8 bytes are passed by reference.  Args > 4 and <= 8 are
 * right-justified in 8 bytes.  Args <= 4 are right-justified in
 * 4 bytes.
 */
/* __list is the word-aligned address of the previous argument.
*/
/* If sizeof __mode > 8, address of arg is at __list - 4.  We need to do
 * two indirections:  1 to fetch the address, and 1 to fetch the value.
 * If sizeof __mode <= 8, the word-aligned address of arg is 
 * __list - sizeof __mode, masked to requred alignment (4 or 8 byte).  
 * The real address is the word-aligned address + extra byte offset.
 */

#ifdef __cplusplus
     extern "C" {
	 void __builtin_va_start(va_list, ...);
     }
#    define va_start(__list,__parmN) (__list=0,__builtin_va_start(__list,&__parmN))
#else /* not __cplusplus */
#    define va_start(__list,__parmN) __builtin_va_start (__list, &__parmN)
#endif /* __cplusplus */

#    define va_arg(__list,__mode)					\
	(sizeof(__mode) > 8 ?						\
	  ((__list = (va_list) ((char *)__list - sizeof (int))),	\
	   (*((__mode *) (*((int *) (__list)))))) :			\
	  ((__list =							\
	      (va_list) ((long)((char *)__list - sizeof (__mode))	\
	      & (sizeof(__mode) > 4 ? __DW_MASK : __WORD_MASK))),	\
	   (*((__mode *) ((char *)__list +				\
		((8 - sizeof(__mode)) % 4))))))

#    define va_end(__list)

#  endif /* ! __lint */

#  endif /* __hp9000s800 */

#else /* not _STDARG_INCLUDED */

#  ifdef __hp9000s800
#    ifndef _VA_LIST
#      define _VA_LIST
       typedef double *va_list;
#    endif /* _VA_LIST */

#    define va_dcl long va_alist;
#  else
#    ifndef _VA_LIST
#      define _VA_LIST
       typedef char *va_list;
#    endif /* _VA_LIST */

#    define va_dcl int va_alist;
#  endif /* __hp9000s800 */

#  ifdef hp9000s500
#    define va_start(__list) __list = ((char*) (&va_alist)) + 3
#    define va_arg(__list,__mode) ((__mode *)((__list -= \
		((sizeof(__mode)+3)&(~3))) + sizeof(int)))[0]
#  endif /* hp9000s500 */

#  ifdef __hp9000s300
#    ifdef __cplusplus
#      ifdef __compile_mode
         extern "C" {
             void __builtin_va_start(va_list, ...);
         }
#        define va_start(__list) (__list=0,__builtin_va_start(__list,&va_alist),__list-=4)
#      else /* ! __compile_mode */
#        define va_start(__list) __list = (char *) &va_alist
#      endif /* __compile_mode */
#    else  /* ! __cplusplus */
#      define va_start(__list) __list = (char *) &va_alist
#    endif /*  __cplusplus */

#    define va_arg(__list,__mode) ((__mode *)(__list += sizeof(__mode)))[-1]
#  endif /* __hp9000s300 */

#  ifdef __hp9000s800

#  ifdef __lint
#    define va_start(__list) __list = (double *) &va_alist
#    define va_arg(__list,__mode) ((__mode *)(__list += sizeof(__mode)))[-1]
#  else /* ! __lint */

#    define __WORD_MASK 0xFFFFFFFC
#    define __DW_MASK   0xFFFFFFF8

/* Args > 8 bytes are passed by reference.  Args > 4 and <= 8 are
 * right-justified in 8 bytes.  Args <= 4 are right-justified in
 * 4 bytes.
 */
/* __list is the word-aligned address of the previous argument.
*/
/* If sizeof __mode > 8, address of arg is at __list - 4.  We need to do
 * two indirections:  1 to fetch the address, and 1 to fetch the value.
 * If sizeof __mode <= 8, the word-aligned address of arg is 
 * __list - sizeof __mode, masked to requred alignment (4 or 8 byte).  
 * The real address is the word-aligned address + extra byte offset.
 */

#ifdef __cplusplus
     extern "C" {
	 void __builtin_va_start(va_list, ...);
     }
#    define va_start(__list) (__list=0,__builtin_va_start(__list,&va_alist))
#else /* not __cplusplus */
#    define va_start(__list) __builtin_va_start (__list, &va_alist)
#endif /* __cplusplus */


#    define va_arg(__list,__mode)					\
	(sizeof(__mode) > 8 ?						\
	  ((__list = (va_list) ((char *)__list - sizeof (int))),	\
	   (*((__mode *) (*((int *) (__list)))))) :			\
	  ((__list =							\
	      (va_list) ((long)((char *)__list - sizeof (__mode))	\
	      & (sizeof(__mode) > 4 ? __DW_MASK : __WORD_MASK))),	\
	   (*((__mode *) ((char *)__list +				\
		((8 - sizeof(__mode)) % 4))))))

#  endif /* __lint */

#  endif /* __hp9000s800 */

#  define va_end(__list)

#endif /* else _STDARG_INCLUDED */

#endif /* _VARARGS_INCLUDED */
