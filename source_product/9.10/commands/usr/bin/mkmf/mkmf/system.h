/* $Header: system.h,v 66.2 90/05/22 14:57:42 nicklin Exp $ */

/*
 * System-dependent definitions
 *
 * Author: Peter J. Nicklin
 */
#ifndef SYSTEM_H
#define SYSTEM_H

#if defined(hpux) || defined(CRAY)
#  ifndef SYSV
#    define SYSV
#  endif
#endif

#if defined(sun) || defined(apollo) || defined(vax)
# ifndef BSD4
#    define BSD4
#  endif
#endif

#if defined(M_XENIX)
#  ifndef XENIX
#    define XENIX
#  endif
#endif

#if !defined(SYSV) && !defined(BSD4) && !defined(XENIX)
#  define BSD4			/* default */
#endif

#if defined(BSD4) || defined(hpux)
#  define OPEN(name,flags,mode) open(name,flags,mode)
#  define RENAME(from,to)	rename(from,to)
#else
#  define OPEN(name,flags,mode) open(name,flags)
#  define RENAME(from,to)	unlink(to); link(from,to); unlink(from)
#endif

#ifdef BSD4
#  define strchr		index
#  define strrchr		rindex
#endif

#define FILEXIST(file)		((access(file,0) == 0) ? 1 : 0)
#define FILEWRITE(file)		((access(file,6) == 0) ? 1 : 0)

#endif /* SYSTEM_H */
