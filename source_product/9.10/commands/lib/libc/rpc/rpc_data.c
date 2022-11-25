
/*
 * rpc_data.c
 *
 * Copyright, 1989, Hewlett Packard Company.
 */

/* 
 * This file was created as part of the name space clean up effort for
 * ANSI-C/POSIX.  Variables that are visible to the user, but not in
 * the ANSI/POSIX standard, are changed to have names that begin with
 * an underscore (e.g. _marshalled_client instead of marshalled_client),
 * and a "secondary definition" is added to resolve references to the
 * original name if the program does not have a conflicting name.  
 * Unfortunately, secondary definitions will ONLY work on INITIALIZED
 * data.  Thus, we have to initialize any variables that we want to
 * use with secondary references.  Worse yet, if a global variable needs
 * to be initialized in a ".o" file, the ENTIRE ".o" file is included by
 * the loader,  if the variable is referenced in the program.  This
 * means that all unresolved references of that .o then pull in other ".o"
 * files, etc.  Thus, to avoid pulling all of libc into every loaded file,
 * this file was created as a place where we can initialize RPC variables
 * as necessary.  If in doubt, put the global variable in this file.
 */

#ifdef _NAMESPACE_CLEAN

#define rpc_createerr		_rpc_createerr		/* In this file */

#endif

#include <rpc/rpc.h>

/*
 * From clnt_perr.c:
 *
 * rpc_createerr hold information about RPC failures during client creation.
 * Moved from clnt_perr.c because some programs use rpc_createerr but don't
 * ever call any of the functions in clnt_perr.c
 */

#ifdef _NAMESPACE_CLEAN
#undef rpc_createerr
#pragma _HP_SECONDARY_DEF _rpc_createerr rpc_createerr
#define rpc_createerr _rpc_createerr
#endif

struct rpc_createerr rpc_createerr = {
	(enum clnt_stat) 0
};
