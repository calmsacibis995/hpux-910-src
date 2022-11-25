/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 12.5 $	$Date: 92/03/09 10:15:11 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/svc_simple.c,v $
 * $Revision: 12.5 $	$Author: gomathi $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/03/09 10:15:11 $
 *
 * Revision 12.2  90/08/30  12:03:00  12:03:00  prabha
 * Yellow Pages/YP -> Network Information Services/NIS changes made.
 * 
 * Revision 12.1  90/03/21  10:43:53  10:43:53  dlr (Dominic Ruffatto)
 * Commented out strings following #else and #endif lines (ANSII-C compliance).
 * 
 * Revision 12.0  89/09/25  16:09:20  16:09:20  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.3  89/02/13  15:06:26  15:06:26  dds (Darren Smith)
 * Changed ifdef around malloc/free in NAME SPACE defines to use #ifdef
 * _ANSIC_CLEAN.  Also changed uses of BUFSIZ to use their own define of 1024
 * because in most cases we don't really need 8K buffers. dds
 * 
 * Revision 1.1.10.12  89/02/13  14:05:33  14:05:33  cmahon (Christina Mahon)
 * Changed ifdef around malloc/free in NAME SPACE defines to use #ifdef
 * _ANSIC_CLEAN.  Also changed uses of BUFSIZ to use their own define of 1024
 * because in most cases we don't really need 8K buffers. dds
 * 
 * Revision 1.1.10.11  89/01/26  12:11:39  12:11:39  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.10  89/01/16  15:21:55  15:21:55  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:54:39  14:54:39  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.2  88/03/24  15:14:57  15:14:57  chm (Cristina H. Mahon)
 * Fix related to DTS CNDdm01106.  Now we use an NLS front end for all
 * NFS libc routines.  This improves NIS performance on the s800s.
 * 
 * Revision 1.1.10.9  88/03/24  17:13:39  17:13:39  cmahon (Christina Mahon)
 * Fix related to DTS CNDdm01106.  Now we use an NLS front end for all
 * NFS libc routines.  This improves NIS performance on the s800s.
 * 
 * Revision 1.1.10.8  87/09/25  13:44:39  13:44:39  cmahon (Christina Mahon)
 * Added message catalog support for a message that did not have it.
 * 
 * Revision 1.1.10.7  87/09/25  13:19:15  13:19:15  cmahon (Christina Mahon)
 * fixed DTS bugs CNOdm00479 -- NULL inproc or outproc cause errors
 * and CNOdm00390 -- multiple versions of prog,proc don't work...
 * added code to save and check version number of registered programs,
 * which calls the exact version, if matched, or the LAST version registered
 * if there is no exact match (same as the old behavior except for exact match).
 * 
 * Revision 1.1.10.6  87/09/22  15:52:51  15:52:51  cmahon (Christina Mahon)
 * readded fix that was done before support for NLS messages was added but
 * lost inbetween
 * 
 * Revision 1.1.10.5  87/09/22  13:14:11  13:14:11  cmahon (Christina Mahon)
 * Added support for NLS to messages that did not have it already.
 * 
 * Revision 1.1.10.4  87/08/07  15:06:12  15:06:12  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.3  87/08/07  11:07:18  11:07:18  cmahon (Christina Mahon)
 * Merge to latest 300 version.  Added defines for prognum, versnum, procum
 * as u_long and add check for a NULL procedure name.
 * 
 * Revision 1.1.10.2  87/07/16  22:24:47  22:24:47  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987.
 * 
 * Revision 1.2  86/07/28  14:52:11  14:52:11  dacosta (Herve' Da Costa)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: svc_simple.c,v 12.5 92/03/09 10:15:11 gomathi Exp $ (Hewlett-Packard)";
#endif

/* 
 * svc_simple.c
 * Simplified front end to rpc.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define catgets 		_catgets
#define fprintf 		_fprintf
#define memset 			_memset
#define nl_fprintf 		_nl_fprintf
#define pmap_unset 		_pmap_unset
#define registerrpc 		_registerrpc		/* In this file */
#define svc_register 		_svc_register
#define svc_sendreply 		_svc_sendreply
#define svcerr_decode 		_svcerr_decode
#define svcudp_create 		_svcudp_create

/* #define xdr_void 		_xdr_void      Removed by RB DTS 10181INDaa  */

#ifdef _ANSIC_CLEAN
#define malloc 			_malloc
#endif /* _ANSIC_CLEAN */

#endif /* _NAME_SPACE_CLEAN */

#define NL_SETN 14	/* set number */
#include <nl_types.h>

#include <stdio.h>
#include <rpc/rpc.h>
extern bool_t   _xdr_void();    /*RB DTS 10181INDaa */
#include <sys/socket.h>
#include <time.h>
#include <netdb.h>

/*	HPNFS	jad	87.09.23
**	added p_versnum to fix bug of only being able to have one dispatch
**	routine for any prog,proc pair (can't serve multiple versions with
**	different functions).  Also use u_long for all declarations!
*/
static struct proglst {
	char *(*p_progname)();
	u_long	p_prognum;
	u_long	p_versnum;
	u_long	p_procnum;
	xdrproc_t p_inproc, p_outproc;
	struct proglst *p_nxt;
} *proglst;

int universal();
static SVCXPRT *transp;
static madetransp;
static struct proglst *pl;
static nl_catd nlmsg_fd;

#ifdef _NAMESPACE_CLEAN
#undef registerrpc
#pragma _HP_SECONDARY_DEF _registerrpc registerrpc
#define registerrpc _registerrpc
#endif

registerrpc(prognum, versnum, procnum, progname, inproc, outproc)
	u_long prognum, versnum, procnum;
	char *(*progname)();
	xdrproc_t inproc, outproc;
{
	
	nlmsg_fd = _nfs_nls_catopen();
	/*	HPNFS	jad	87.09.23
	**	added more error checking for NULLPROC and NULL XDR procs
	*/
	if ((char *)progname == NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,8, "can't register with a NULL procedure name\n")));
		return (-1);
	}
	if (inproc == NULL_xdrproc_t  ||  outproc == NULL_xdrproc_t) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,9, "can't register with a NULL XDR procedure\n")));
		return(-1);
	}
	if (procnum == NULLPROC) {
		fprintf(stderr,
		    (catgets(nlmsg_fd,NL_SETN,1, "can't reassign procedure number %ld\n")), NULLPROC);
		return (-1);
	}

	if (!madetransp) {
		madetransp = 1;
		transp = svcudp_create(RPC_ANYSOCK);
		if (transp == NULL) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "couldn't create an rpc server\n")));
			return (-1);
		}
	}
	pmap_unset(prognum, versnum);
	if (!svc_register(transp, prognum, versnum, universal, IPPROTO_UDP)) {
	    	nl_fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "couldn't register prog %1$d vers %2$d\n")),
		    prognum, versnum);
		return (-1);
	}
	pl = (struct proglst *)malloc(sizeof(struct proglst));
	if (pl == NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "registerrpc: out of memory\n")));
		return (-1);
	}
	pl->p_progname = progname;
	pl->p_prognum = prognum;
	pl->p_versnum = versnum;
	pl->p_procnum = procnum;
	pl->p_inproc = inproc;
	pl->p_outproc = outproc;
	pl->p_nxt = proglst;
	proglst = pl;
	return (0);
}

static
universal(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	int i, prog_registered=FALSE;
	char *outdata;
	char xdrbuf[UDPMSGSIZE];
	u_long prog, proc, vers;
	struct proglst *pl, *pl_found;

	nlmsg_fd = _nfs_nls_catopen();

	/* 
	 * enforce "procnum 0 is echo" convention
	 */
	if (rqstp->rq_proc == NULLPROC) {
		if (svc_sendreply(transp, _xdr_void, 0) == FALSE) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "universal: could not sendreply\n")));
		}
		return;
	}
	prog = rqstp->rq_prog;
	vers = rqstp->rq_vers;
	proc = rqstp->rq_proc;
	pl_found = NULL;
	for (pl = proglst; pl != NULL; pl = pl->p_nxt)
		if (pl->p_prognum == prog) {
			/*	HPNFS	jad	87.09.23
			**	we found a matching program number; if we also
			**	match the procedure and version numbers, then
			**	break from the search and call it.
			*/
			prog_registered = TRUE;
			if (pl->p_procnum == proc) {
				if (pl->p_versnum == vers) {
					pl_found = pl;
					break;
				} else
					/*	HPNFS	jad	87.09.25
					**	if we have matched the program
					**	and procedure numbers, but not
					**	version number, then make sure
					**	we only save the FIRST match.
					**	progs are added to the head of
					**	the proglst, so it's the LAST
					**	one that was registered; this
					**	is the old behavior (compat).
					*/
					if (pl_found == NULL)
						pl_found = pl;
			}
		}

	if (! prog_registered)
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,7, "never registered prog %ld\n")), prog);
	else if (pl_found == NULL)
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,8, "never registered proc %ld\n")), proc);
	else {
		pl = pl_found;	/* since the rest of the code uses pl! */
		/*	HPNFS	jad	87.09.23
		**	Note -- we could have a version mismatch!  For
		**	backwards compatibility, we allow this.  The
		**	service itself must refuse to serve unknown versions.
		##	NOTE: the version will either match, or be the last
		##	version we registered (for backwards compatibility)
		*/
		/* decode arguments into a CLEAN buffer */
		memset(xdrbuf, 0, sizeof(xdrbuf)); /* required ! */
		if (!svc_getargs(transp, pl->p_inproc, xdrbuf)) {
			svcerr_decode(transp);
			return;
		}
		outdata = (*(pl->p_progname))(xdrbuf);
		if ( (outdata == NULL) && 
                     ((pl->p_outproc != _xdr_void) && 
                            (pl->p_outproc != xdr_void)))
			/* there was an error, return silently(?!?) */
			return;
		if (!svc_sendreply(transp, pl->p_outproc, outdata)) {
			fprintf(stderr,
				(catgets(nlmsg_fd,NL_SETN,6, "universal: trouble replying to prog %ld\n")),
				pl->p_prognum);
			/* free the decoded arguments */
			svc_freeargs(transp, pl->p_inproc, xdrbuf);
		}
		return;
	}
}
