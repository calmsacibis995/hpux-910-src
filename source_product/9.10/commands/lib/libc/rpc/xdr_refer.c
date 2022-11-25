/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 12.4 $	$Date: 92/02/07 17:08:42 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/xdr_refer.c,v $
 * $Revision: 12.4 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:08:42 $
 *
 * Revision 12.3  90/08/30  12:01:46  12:01:46  prabha
 * Yellow Pages/YP -> Network Information Services/NIS changes made.
 * 
 * Revision 12.2  90/06/11  08:25:46  08:25:46  dlr (Dominic Ruffatto)
 * Cleaned up ifdefs.
 * 
 * Revision 12.1  90/03/21  10:50:16  10:50:16  dlr (Dominic Ruffatto)
 * Commented out strings following #else and #endif lines (ANSII-C compliance).
 * 
 * Revision 12.0  89/09/25  16:09:47  16:09:47  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.2  89/01/26  13:29:13  13:29:13  dds (Darren Smith)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.9  89/01/26  12:28:06  12:28:06  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.8  89/01/16  15:27:46  15:27:46  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:55:27  14:55:27  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.5  88/08/23  16:18:21  16:18:21  mikey (Mike Shipley)
 * Moved new xdr routines into ifndef KERNEL
 * 
 * Revision 1.1.10.7  88/08/23  15:17:36  15:17:36  cmahon (Christina Mahon)
 * Moved new xdr routines into ifndef KERNEL
 * 
 * Revision 1.1.10.6  88/08/05  13:00:28  13:00:28  cmahon (Christina Mahon)
 * Changed #ifdef NFS_3.2 to NFS3_2 to avoid problem with "." in ifdef.
 * dds
 * 
 * Revision 1.1.10.5  88/08/03  09:22:13  09:22:13  cmahon (Christina Mahon)
 * Added a RPC3.9 routine of xdr_pointer() inside ifdef NFS3_2
 * 
 * Revision 1.1.10.4  88/03/24  17:36:03  17:36:03  cmahon (Christina Mahon)
 * Fix related to DTS CNDdm01106.  Now we use an NLS front end for all NFS
 * libc routines.  This improves NIS performance specially on the s800s.
 * 
 * Revision 1.1.10.3  87/08/07  15:16:55  15:16:55  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.2  87/07/16  22:33:30  22:33:30  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987.
 * 
 * Revision 1.3  86/09/25  14:28:20  14:28:20  vincent (Vincent Hsieh)
 * Removed line "char * mem_alloc()" to please the compiler - NFS port.
 * 
 * Revision 1.3  86/07/28  15:15:04  15:15:04  dacosta (Herve' Da Costa)
 * Noted name change: xdr_reference.c to xdr_refer.c (12 chars)  D. Pan
 * 
 * Revision 1.2  86/07/28  11:46:41  11:46:41  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: xdr_refer.c,v 12.4 92/02/07 17:08:42 indnetwk Exp $ (Hewlett-Packard)";
#endif

/*
 * xdr_refer.c, Generic XDR routines implementation.
 * USED TO BE CALLED xdr_reference.c,   D. Pan  7/25/86
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * These are the "non-trivial" xdr primitives used to serialize and de-serialize
 * "pointers".  See xdr.h for more info on the interface to xdr.
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define catgets 	_catgets
#define fprintf 	_fprintf
#define memset 		_memset
#define xdr_bool 	_xdr_bool
#define xdr_pointer 	_xdr_pointer		/* In this file */
#define xdr_reference 	_xdr_reference		/* In this file */

#endif /* _NAME_SPACE_CLEAN */

#define NL_SETN 20	/* set number */
#include <nl_types.h>
static	nl_catd nlmsg_fd;

#include <rpc/types.h>	/* <> */
#include <rpc/xdr.h>	/* <> */
#include <stdio.h>
char *malloc();
#define LASTUNSIGNED	((u_int)0-1)


/*
 * XDR an indirect pointer
 * xdr_reference is for recursively translating a structure that is
 * referenced by a pointer inside the structure that is currently being
 * translated.  pp references a pointer to storage. If *pp is null
 * the  necessary storage is allocated.
 * size is the sizeof the referneced structure.
 * proc is the routine to handle the referenced structure.
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_reference
#pragma _HP_SECONDARY_DEF _xdr_reference xdr_reference
#define xdr_reference _xdr_reference
#endif

bool_t
xdr_reference(xdrs, pp, size, proc)
	register XDR *xdrs;
	caddr_t *pp;		/* the pointer to work on */
	u_int size;		/* size of the object pointed to */
	xdrproc_t proc;		/* xdr routine to handle the object */
{
	register caddr_t loc = *pp;
	register bool_t stat;

	nlmsg_fd = _nfs_nls_catopen();

	if (loc == NULL)
		switch (xdrs->x_op) {
		case XDR_FREE:
			return (TRUE);

		case XDR_DECODE:
			*pp = loc = mem_alloc(size);
			if (loc == NULL) {
				fprintf(stderr,
				    (catgets(nlmsg_fd,NL_SETN,1, "xdr_reference: out of memory\n")));
				return (FALSE);
			}
			memset(loc, 0, (int)size);
			break;
	}

	stat = (*proc)(xdrs, loc, LASTUNSIGNED);

	if (xdrs->x_op == XDR_FREE) {
		mem_free(loc, size);
		*pp = NULL;
	}
	return (stat);
}


/*
 * xdr_pointer():
 *
 * XDR a pointer to a possibly recursive data structure. This
 * differs with xdr_reference in that it can serialize/deserialiaze
 * trees correctly.
 *
 *  What's sent is actually a union:
 *
 *  union object_pointer switch (boolean b) {
 *  case TRUE: object_data data;
 *  case FALSE: void nothing;
 *  }
 *
 * > objpp: Pointer to the pointer to the object.
 * > obj_size: size of the object.
 * > xdr_obj: routine to XDR an object.
 *
 */

#ifdef _NAMESPACE_CLEAN
#undef xdr_pointer
#pragma _HP_SECONDARY_DEF _xdr_pointer xdr_pointer
#define xdr_pointer _xdr_pointer
#endif

bool_t
xdr_pointer(xdrs, objpp, obj_size, xdr_obj)
        register XDR *xdrs;
        char **objpp;
        u_int obj_size;
        xdrproc_t xdr_obj;
{

        bool_t more_data;

        more_data = (*objpp != NULL);
        if (! xdr_bool(xdrs, &more_data)) {
                return (FALSE);
        }
        if (! more_data) {
                *objpp = NULL;
                return (TRUE);
        }
        return (xdr_reference(xdrs, objpp, obj_size, xdr_obj));
}

