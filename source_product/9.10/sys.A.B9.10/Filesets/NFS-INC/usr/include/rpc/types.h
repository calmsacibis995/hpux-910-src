/* $Header: types.h,v 1.9.83.3 93/09/17 19:28:39 kcs Exp $ */

#ifndef _RPC_TYPES_INCLUDED
#define _RPC_TYPES_INCLUDED

/*
 * RPC types, in addition to those in <sys/types.h>
 *
 * (c) Copyright 1987 Hewlett-Packard Company
 * (c) Copyright 1984 Sun Microsystems, Inc.
 */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#  include "../h/types.h"	/* pull in the standard types.h defs */
#else  /* ! _KERNEL_BUILD */
#  include <sys/types.h>	/* pull in the standard types.h defs */
#endif /* _KERNEL_BUILD */

#  define bool_t	int
#  define enum_t	int

#  ifndef FALSE
#    define FALSE	0
#  endif /* FALSE */

#  ifndef TRUE
#    define TRUE	1
#  endif /* TRUE */

#  define __dontcare__	(-1)

#ifndef _KERNEL
#  define mem_alloc(bsize)	_rpc_malloc(bsize)
#  define mem_free(ptr, bsize)	_rpc_free(ptr)
   char *_rpc_malloc();
   void _rpc_free();
#else
#  define mem_alloc(bsize)	kmem_alloc((u_int)bsize)
#  define mem_free(ptr, bsize)	kmem_free((caddr_t)(ptr), (u_int)(bsize))
#endif

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* not _RPC_TYPES_INCLUDED */
