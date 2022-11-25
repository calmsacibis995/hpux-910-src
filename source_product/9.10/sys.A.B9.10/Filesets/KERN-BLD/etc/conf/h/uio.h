/* $Header: uio.h,v 1.18.83.5 93/12/09 12:40:52 marshall Exp $ */    

#ifndef _SYS_UIO_INCLUDED
#define _SYS_UIO_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

   /* Types */

#  ifndef _CADDR_T
#    define _CADDR_T
       typedef char *caddr_t;	/* same as in types.h */
#  endif /* _CADDR_T */

#  ifndef _SSIZE_T
#     define _SSIZE_T
      typedef int ssize_t;	/* Signed version of size_t */
#  endif /* _SSIZE_T */


   /* Structures */

#  ifdef _KERNEL
#  ifdef __hp9000s800
   struct ioops {
	   int		(*ioo_freebuf)();       /* free buffer */
	   int		(*ioo_copyto)();        /* copy data to buffer */
	   int		(*ioo_copyfrom)();      /* copy data from buffer */
	   int		(*ioo_physaddr)();      /* return physical address */
	   int		(*ioo_flushcache)();    /* flush buffer from cache */
	   int		(*ioo_purgecache)();    /* purge buffer from cache */
   };
#  endif /* __hp9000s800 */
#  endif /* _KERNEL */

   struct iovec {
	   caddr_t	iov_base;
	   int		iov_len;
   };

   struct uio {
	   struct iovec *uio_iov;
	   int		uio_iovcnt;
	   int		uio_offset;
	   int		uio_seg;
	   int		uio_resid;
	   int		uio_fpflags;
   };


   /* Function protoypes */

#  ifndef _KERNEL
#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */

#  ifdef _PROTOTYPES
       extern ssize_t readv(int, const struct iovec *, size_t);
       extern ssize_t writev(int, const struct iovec *, size_t);
#  else /* not _PROTOTYPES */
#    ifdef _CLASSIC_TYPES
       extern int readv();
       extern int writev();
#    else /* not _CLASSIC_TYPES */
       extern ssize_t readv();
       extern ssize_t writev();
#    endif /* not _CLASSIC_TYPES */
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */
#  endif /* not _KERNEL */


   /* Constants */

#  define MAXIOV 16		/* Maximum number of iov's on readv/writev */

   enum uio_rw { UIO_READ = 0, UIO_WRITE = 1 };
#  define UIO_RW		1
#  define UIO_AVOID_COPY	2


   /* segments */
#  define UIOSEG_USER		0	/* User space */
#  define UIOSEG_KERNEL		1	/* Kernel space */
#  define UIOSEG_INSTR		2	/* Instructions */
#  define UIOSEG_LBCOPY		3	/* Intraspace move */

#  ifdef __hp9000s800
#    define UIOSEG_KFLUSH	4	/* Kernel space - flush data cache */
#    define UIOSEG_LBFLUSH	5	/* Intraspace move - flush data cache */
#  endif /* __hp9000s800 */

#  define UIOSEG_PAGEIN		6

#  ifdef  __hp9000s300
#    define UIO_NOACCESS	0
#    define UIO_KERNACCESS	1
#    define UIO_KERNWRITE	2
#  endif /* __hp9000s300 */

#  ifdef _KERNEL
#  ifdef __hp9000s800
#    define IOO_FREEBUF(ioo,arg,flag) /* returns status */            \
	((*(ioo)->ioo_freebuf)((arg),(flag)))
#    define IOO_COPYTO(ioo,arg,off,len,from) /* returns status */     \
	((*(ioo)->ioo_copyto)((arg),(off),(len),(from)))
#    define IOO_COPYFROM(ioo,arg,off,len,to) /* returns status */     \
	((*(ioo)->ioo_copyfrom)((arg),(off),(len),(to)))
#    define IOO_PHYSADDR(ioo,arg,off) /* returns physical addr */     \
	((*(ioo)->ioo_physaddr)((arg),(off)))
#    define IOO_FLUSHCACHE(ioo,arg,off,len,cond) /* returns status */ \
	((*(ioo)->ioo_flushcache)((arg),(off),(len),(cond)))
#    define IOO_PURGECACHE(ioo,arg,off,len,cond) /* returns status */ \
	((*(ioo)->ioo_purgecache)((arg),(off),(len),(cond)))
#  endif /* __hp9000s800 */
#  endif /* _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_UIO_INCLUDED */
