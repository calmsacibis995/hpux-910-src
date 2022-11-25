/* @(#) $Revision: 72.2 $ */

#ifdef _NAMESPACE_CLEAN
#define access	_access
#define sysconf	_sysconf
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#ifdef _NAMESPACE_CLEAN
#undef sysconf
#pragma _HP_SECONDARY_DEF _sysconf sysconf
#define sysconf	_sysconf
#endif

long
sysconf(name)
int name;
{
    extern long __sysconf();	/* the kernel entry point */

    switch (name)
    {
    case _SC_CPU_VERSION:
	{
	    /*
	     * The cpu version remains constant, and we may want to
	     * get it alot.  So, we cache the value returned from
	     * the first call.
	     */
	    static long cpu_version = -1;

	    if (cpu_version == -1)
		cpu_version = __sysconf(_SC_CPU_VERSION);
	    return cpu_version;
	}
    case _SC_IO_TYPE:
	{
	    /*
	     * The io type remains constant, and we may want to
	     * get it alot.  So, we cache the value returned from
	     * the first call.
	     */
	    static long io_type = -1;

	    if (io_type == -1)
		io_type = __sysconf(_SC_IO_TYPE);
	    return io_type;
	}
#ifdef _SC_SECURITY_CLASS
    case _SC_SECURITY_CLASS:
	{
	    /*
	     * The SC_SECURITY_CLASS is known by the kernel, but never
	     * changes.  We cache the value in a local static variable
	     * so that we can avoid the overhead of a kernel call.
	     *
	     * If __sysconf() fails, we force the security class to be
	     * a non-secure system for safety.
	     */
	    static long security_class = -1;
	    int errno_save = errno;

	    if (security_class == -1)
	    {
		security_class = __sysconf(_SC_SECURITY_CLASS);
		if (security_class < 0)
		{
		    errno = errno_save;
		    security_class = SEC_CLASS_NONE; /* force it */
		}
	    }
	    return security_class;
	}
#endif /* _SC_SECURITY_CLASS */

#ifdef _INCLUDE_POSIX_SOURCE || _INCLUDE_POSIX2_SOURCE
    case _SC_JOB_CONTROL:  	return(_POSIX_JOB_CONTROL);
    case _SC_SAVED_IDS:  	return(_POSIX_SAVED_IDS);

    /*
     * _SC_VERSION should be #defined to either _SC_1_VERSION_88 or
     * _SC_1_VERSION_90 when the user program compiles unistd.h.  Therefore,
     * we return the correct value using _...88 or _...90, and
     * sysconf(_SC_VERSION) will return the correct value at run time
     * depending on the posix version the user has specified.
     */

    case _SC_1_VERSION_88:      return(_POSIX1_VERSION_88);
    case _SC_1_VERSION_90:      return(_POSIX1_VERSION_90);

    case _SC_BC_BASE_MAX:  	return(BC_BASE_MAX);
    case _SC_BC_DIM_MAX:  	return(BC_DIM_MAX);
    case _SC_BC_SCALE_MAX:  	return(BC_SCALE_MAX);
    case _SC_BC_STRING_MAX:  	return(_POSIX2_BC_STRING_MAX);
    case _SC_COLL_WEIGHTS_MAX: 	return(_POSIX2_COLL_WEIGHTS_MAX);
    case _SC_EXPR_NEST_MAX:  	return(EXPR_NEST_MAX);
    case _SC_LINE_MAX:  	return(LINE_MAX);
    case _SC_RE_DUP_MAX:  	return(RE_DUP_MAX);
    case _SC_TZNAME_MAX:	return(TZNAME_MAX);
    case _SC_2_VERSION:  	return(_POSIX2_VERSION);
    case _SC_2_C_BIND:  	return(_POSIX2_C_BIND);
    case _SC_2_LOCALEDEF: 	return(_POSIX2_LOCALEDEF);
    case _SC_2_C_VERSION:	return(_POSIX2_C_VERSION);
    case _SC_2_CHAR_TERM:	return(_POSIX2_CHAR_TERM);
    case _SC_2_UPE:		return(-1);	/* NOT supported yet */
    case _SC_2_C_DEV:
	{
	    /*
	     * check for critical pieces for C language
	     * development tools
	     */
	    int errno_save = errno;

#	ifdef V4FS
	    if (access("/usr/ccs/bin/c89",  X_OK) != 0 ||
		access("/usr/ccs/bin/lex",  X_OK) != 0 ||
		access("/usr/ccs/bin/yacc", X_OK) != 0 ||
		access("/usr/lib/libc.a",   F_OK) != 0)
#	else
	    if (access("/bin/c89",      X_OK) != 0 ||
		access("/usr/bin/lex",  X_OK) != 0 ||
		access("/usr/bin/yacc", X_OK) != 0 ||
		access("/lib/libc.a",   F_OK) != 0)
#	endif	/* V4FS */
	    {
		errno = errno_save;	/* return inherited errno */
		return -1;
	    }
	    return 1;		/* all is ok, return 1 */
	}

    case _SC_2_FORT_DEV:
	{
	    /*
	     * check for critical pieces for Fortran language
	     * development tools
	     */
	    int errno_save = errno;

	    if (access("/usr/bin/fort77", X_OK) != 0 ||
	        access("/usr/bin/asa",    X_OK) != 0)
	    {
		errno = errno_save;	/* return inherited errno */
		return -1;
	    }
	    return 1;		/* all is ok, return 1 */
	}
    case _SC_2_FORT_RUN:
	{
	    /*
	     * check for critical pieces for Fortran language
	     * runtime environment
	     */
	    int errno_save = errno;

	    if (access("/usr/bin/asa",    X_OK) != 0)
	    {
		errno = errno_save;	/* return inherited errno */
		return -1;
	    }
	    return 1;		/* all is ok, return 1 */
	}
    case _SC_2_SW_DEV:
	{
	    /*
	     * check for critical pieces for software
	     * development tools
	     */
	    int errno_save = errno;

#	ifdef V4FS
	    if (access("/usr/ccs/bin/ar",    X_OK) != 0 ||
	        access("/usr/ccs/bin/make",  X_OK) != 0 ||
	        access("/usr/ccs/bin/strip", X_OK) != 0)
#	else
	    if (access("/bin/ar",    X_OK) != 0 ||
	        access("/bin/make",  X_OK) != 0 ||
	        access("/bin/strip", X_OK) != 0)
#	endif	/* V4FS */
	    {
		errno = errno_save;	/* return inherited errno */
		return(-1);
	    }
	    return 1;		/* all is ok, return 1 */
	}

    /*
     * _SC_XOPEN_VERSION should be #defined to either _SC_XPG3_VERSION or
     * _SC_XPG4_VERSION when the user program compiles unistd.h.  Therefore,
     * we return the correct value using _SC_XPG3... or _SC_XPG4..., and
     * sysconf(_SC_XOPEN_VERSION) will return the correct value at run time
     * depending on the posix version the user has specified.
     */
    case _SC_XPG3_VERSION:      return(3);
    case _SC_XPG4_VERSION:      return(4);

    case _SC_XOPEN_CRYPT:	return(_XOPEN_CRYPT);
    case _SC_XOPEN_ENH_I18N:	return(-1);	/* NOT supported yet */
    /*
     * Note that we let __sysconf handle _SC_XOPEN_SHM, since it can be
     * de-configured by the customer; we can't make a judgement here.
     */
    case _SC_XOPEN_SHM:
#if defined(__hp9000s700) || defined(__hp9000s300)
	return 1;		/* Shared memory not configurable on s300/700 */
#else /* ! defined(__hp9000s700) || defined(__hp9000s300) */
	return __sysconf(_SC_XOPEN_SHM);	/* ask kernel */
#endif /* defined(__hp9000s700) || defined(__hp9000s300) */

    case _SC_PASS_MAX:  	return(PASS_MAX);

#endif /* _INCLUDE_POSIX_SOURCE || _INCLUDE_POSIX2_SOURCE */

#ifdef AES
    case _SC_ATEXIT_MAX:        return(ATEXIT_MAX);
    /*
     * At this time, the s[347]00 conforms to AES and the s800 does not.
     * We use _SC_IO_TYPE to determine if we are running on a workstation
     * and return the appropriate value.
     */
    case _SC_AES_OS_VERSION: 
	{
	    if (__sysconf(_SC_IO_TYPE) == IO_TYPE_WSIO)
    	        return (_AES_OS_VERSION);	/* s[347]00: AES Conformance */
	    else
    	        return (0);			/* s800: No AES Conformance */
	}
#endif

    /*
     * The following are understood by the kernel, so we let it
     * do the work.  Even if they gave us something bogus, let the
     * kernel return the error.
     */
    case _SC_PAGE_SIZE:
	{
	    /*
	     * The page size remains constant, and we may want to
	     * get it alot.  So, we cache the value returned from
	     * the first call.
	     */
	    static long page_size = -1;

	    if (page_size == -1)
		page_size = __sysconf(_SC_PAGE_SIZE);
	    return page_size;
	}
    case _SC_STREAM_MAX:	return __sysconf(_SC_OPEN_MAX);
    case _SC_ARG_MAX:
    case _SC_CHILD_MAX:
    case _SC_CLK_TCK:
    case _SC_NGROUPS_MAX:
    case _SC_OPEN_MAX:
    default:
	return __sysconf(name);
    }
}
