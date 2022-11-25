/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 70.1 $	$Date: 94/10/04 10:57:28 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/auth_unix.c,v $
 * $Revision: 70.1 $	$Author: hmgr $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/10/04 10:57:28 $
 *
 * Revision 12.2  90/08/30  11:51:35  11:51:35  prabha
 * Yellow Pages/YP -> Network Information Services/NIS changes made.
 * 
 * Revision 12.1  90/03/20  14:43:06  14:43:06  dlr (Dominic Ruffatto)
 * Commented out strings following #else and #endif lines (ANSII-C compliance).
 * 
 * Revision 12.0  89/09/25  16:08:12  16:08:12  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.2  89/01/26  12:49:24  12:49:24  dds (Darren Smith)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.10  89/01/26  11:48:11  11:48:11  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.9  89/01/16  15:03:19  15:03:19  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:47:54  14:47:54  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.2  88/03/24  14:31:37  14:31:37  chm (Cristina H. Mahon)
 * Fix related to DTS CNDdm01106.  Now the NLS routines are only called once
 *  This improves NIS performance on the s800.
 * 
 * Revision 1.1.10.8  88/03/24  16:28:43  16:28:43  cmahon (Christina Mahon)
 * Fix related to DTS CNDdm01106.  Now the NLS routines are only called once
 *  This improves NIS performance on the s800.
 * 
 * Revision 1.1.10.7  87/09/25  13:54:32  13:54:32  cmahon (Christina Mahon)
 * New error message should have message catalog number of 5 instead of 4,
 * so it does not overwrite another message.
 * 
 * Revision 1.1.10.6  87/09/25  13:11:31  13:11:31  cmahon (Christina Mahon)
 * change abort() if authunix_parms() fails to print an error and return NULL;
 * also changed error message if marshall_new_auth fails (deleted auth_none.c).
 * 
 * Revision 1.1.10.5  87/09/21  11:19:14  11:19:14  cmahon (Christina Mahon)
 * Changed the call to getgroup to call it with NGROUPS (20) instead of 
 * NGRPS (8) (nfs groups).  That way if you belong to more then 8 groups I 
 * will only sent along the first 8 groups and ignore the rest instead of
 * aborting like it did before.  The protocol specifies that you can only
 * use 8 groups through NFS.
 * 
 * Revision 1.1.10.4  87/08/07  14:25:48  14:25:48  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.3  87/07/16  22:01:18  22:01:18  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987
 * 
 * Revision 1.2  86/07/28  11:37:22  11:37:22  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: auth_unix.c,v 70.1 94/10/04 10:57:28 hmgr Exp $ (Hewlett-Packard)";
#endif

/*
 * auth_unix.c, Implements UNIX style authentication parameters. 
 *  
 * Copyright (C) 1984, Sun Microsystems, Inc. 
 *
 * The system is very weak.  The client uses no encryption for it's
 * credentials and only sends null verifiers.  The server sends backs
 * null verifiers or optionally a verifier that suggests a new short hand
 * for the credentials.
 *
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define authunix_create 	  _authunix_create	/* In this file. */
#define authunix_create_default   _authunix_create_default  /* In this file */
#define abort 			  _abort
#define catgets 		  _catgets
#define fprintf 		  _fprintf
#define getegid 		  _getegid
#define geteuid 		  _geteuid
#define getgroups 		  _getgroups
#define gethostname 		  _gethostname
#define gettimeofday 		  _gettimeofday
#define memcpy 		  	  _memcpy
#define perror 			  _perror
#define xdr_authunix_parms 	  _xdr_authunix_parms
#define xdr_opaque_auth 	  _xdr_opaque_auth
#define xdrmem_create 		  _xdrmem_create

#endif /* _NAME_SPACE_CLEAN */

#define NL_SETN 4	/* set number */
#include <nl_types.h>
static nl_catd nlmsg_fd;

#include <stdio.h>
#include <sys/param.h>		/* Needed for NGROUPS */
#include <rpc/types.h>	/* <> */
#include <time.h>
#include <rpc/xdr.h>	/* <> */
#include <rpc/auth.h>	/* <> */
#include <rpc/auth_unix.h>	/* <> */
char *malloc();

/*
 * Unix authenticator operations vector
 */
static void	authunix_nextverf();
static bool_t	authunix_marshal();
static bool_t	authunix_validate();
static bool_t	authunix_refresh();
static void	authunix_destroy();

static struct auth_ops auth_unix_ops = {
	authunix_nextverf,
	authunix_marshal,
	authunix_validate,
	authunix_refresh,
	authunix_destroy
};

/*
 * This struct is pointed to by the ah_private field of an auth_handle.
 */
struct audata {
	struct opaque_auth	au_origcred;	/* original credentials */
	struct opaque_auth	au_shcred;	/* short hand cred */
	u_long			au_shfaults;	/* short hand cache faults */
	char			au_marshed[MAX_AUTH_BYTES];
	u_int			au_mpos;	/* xdr pos at end of marshed */
};
#define	AUTH_PRIVATE(auth)	((struct audata *)auth->ah_private)

static bool_t marshal_new_auth();


/*
 * Create a unix style authenticator.
 * Returns an auth handle with the given stuff in it.
 */
#ifdef _NAMESPACE_CLEAN
#undef authunix_create
#pragma _HP_SECONDARY_DEF _authunix_create authunix_create
#define authunix_create _authunix_create
#endif
AUTH *
authunix_create(machname, uid, gid, len, aup_gids)
	char *machname;
	int uid;
	int gid;
	register int len;
	int *aup_gids;
{
	struct authunix_parms aup;
	char mymem[MAX_AUTH_BYTES];
	struct timeval now;
	XDR xdrs;
	register AUTH *auth;
	register struct audata *au;

	nlmsg_fd = _nfs_nls_catopen();

 /* this takes care of a UID security hole. If you pass this routine
    a certain number greater than 65536 for the uid and gid it can turn
    the user into root. We check for the uid and gid to be in the valid
    range. If it is not, we make the user into nobody, nogroup.
 */

	if (uid < -32768 || uid >65535 ) {
	    uid = -2;
	    }

	if (gid < -32768 || gid >65535 ) {
	   gid = -2;
	   }

	/*
	 * Allocate and set up auth handle
	 */
	auth = (AUTH *)mem_alloc(sizeof(*auth));
	if (auth == NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "authunix_create: out of memory\n")));
		return (NULL);
	}
	au = (struct audata *)mem_alloc(sizeof(*au));
	if (au == NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "authunix_create: out of memory\n")));
		return (NULL);
	}
	auth->ah_ops = &auth_unix_ops;
	auth->ah_private = (caddr_t)au;
	auth->ah_verf = au->au_shcred = _null_auth;
	au->au_shfaults = 0;

	/*
	 * fill in param struct from the given params
	 */
	(void)gettimeofday(&now,  (struct timezone *)0);
	aup.aup_time = now.tv_sec;
	aup.aup_machname = machname;
	aup.aup_uid = uid;
	aup.aup_gid = gid;
	aup.aup_len = (u_int)len;
	aup.aup_gids = aup_gids;

	/*
	 * Serialize the parameters into origcred
	 */
	xdrmem_create(&xdrs, mymem, MAX_AUTH_BYTES, XDR_ENCODE);
	if (! xdr_authunix_parms(&xdrs, &aup)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "authunix_create: cannot XDR authunix parameters\n")));
		return(NULL);
	}
	au->au_origcred.oa_length = len = XDR_GETPOS(&xdrs);
	au->au_origcred.oa_flavor = AUTH_UNIX;
	if ((au->au_origcred.oa_base = mem_alloc(len)) == NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "authunix_create: out of memory\n")));
		return (NULL);
	}
	memcpy(au->au_origcred.oa_base, mymem, (u_int)len);

	/*
	 * set auth handle to reflect new cred.
	 */
	auth->ah_cred = au->au_origcred;
	marshal_new_auth(auth);
	return (auth);
}

/*
 * Returns an auth handle with parameters determined by doing lots of
 * syscalls.
 */

#ifdef _NAMESPACE_CLEAN
#undef authunix_create_default
#pragma _HP_SECONDARY_DEF _authunix_create_default authunix_create_default
#define authunix_create_default _authunix_create_default
#endif

AUTH *
authunix_create_default()
{
	register int len;
	char machname[MAX_MACHINE_NAME + 1];
	register int uid;
	register int gid;
	int gids[NGROUPS];

	if (gethostname(machname, MAX_MACHINE_NAME) == -1)
		abort();
	machname[MAX_MACHINE_NAME] = 0;
	uid = geteuid();
	gid = getegid();

/* HPNFS  Changed this portion of the code so that instead of aborting   */
/* 	  if receiving a -1 from getgroups (which happened when we asked */
/*	  for NGRPS groups and there were more than NGRPS available) I   */
/* 	  now ask for NGROUPS (NGROUPS > NGRPS) and use only NGRPS.      */
/*  	  Also, changed gids to be an array of NGROUPS instead of NGRPS  */
/*	  and added include <sys/param.h> to define NGROUPS.		 */

	len = getgroups(NGROUPS, gids);
	if (len > NGRPS)
		len = NGRPS;
	return (authunix_create(machname, uid, gid, len, gids));
}

/*
 * authunix operations
 */

static void
authunix_nextverf(auth)
	AUTH *auth;
{
	/* no action necessary */
}

static bool_t
authunix_marshal(auth, xdrs)
	AUTH *auth;
	XDR *xdrs;
{
	register struct audata *au = AUTH_PRIVATE(auth);

	return (XDR_PUTBYTES(xdrs, au->au_marshed, au->au_mpos));
}

static bool_t
authunix_validate(auth, verf)
	register AUTH *auth;
	struct opaque_auth verf;
{
	register struct audata *au;
	XDR xdrs;

	if (verf.oa_flavor == AUTH_SHORT) {
		au = AUTH_PRIVATE(auth);
		xdrmem_create(&xdrs, verf.oa_base, verf.oa_length, XDR_DECODE);

		if (au->au_shcred.oa_base != NULL) {
			mem_free(au->au_shcred.oa_base,
			    au->au_shcred.oa_length);
			au->au_shcred.oa_base = NULL;
		}
		if (xdr_opaque_auth(&xdrs, &au->au_shcred)) {
			auth->ah_cred = au->au_shcred;
		} else {
			xdrs.x_op = XDR_FREE;
			(void)xdr_opaque_auth(&xdrs, &au->au_shcred);
			au->au_shcred.oa_base = NULL;
			auth->ah_cred = au->au_origcred;
		}
		marshal_new_auth(auth);
	}
	return (TRUE);
}

static bool_t
authunix_refresh(auth)
	register AUTH *auth;
{
	register struct audata *au = AUTH_PRIVATE(auth);
	struct authunix_parms aup;
	struct timeval now;
	XDR xdrs;
	register int stat;

	if (auth->ah_cred.oa_base == au->au_origcred.oa_base) {
		/* there is no hope.  Punt */
		return (FALSE);
	}
	au->au_shfaults ++;

	/* first deserialize the creds back into a struct authunix_parms */
	aup.aup_machname = NULL;
	aup.aup_gids = (int *)NULL;
	xdrmem_create(&xdrs, au->au_origcred.oa_base,
	    au->au_origcred.oa_length, XDR_DECODE);
	stat = xdr_authunix_parms(&xdrs, &aup);
	if (! stat) 
		goto done;

	/* update the time and serialize in place */
	(void)gettimeofday(&now, (struct timezone *)0);
	aup.aup_time = now.tv_sec;
	xdrs.x_op = XDR_ENCODE;
	XDR_SETPOS(&xdrs, 0);
	stat = xdr_authunix_parms(&xdrs, &aup);
	if (! stat)
		goto done;
	auth->ah_cred = au->au_origcred;
	marshal_new_auth(auth);
done:
	/* free the struct authunix_parms created by deserializing */
	xdrs.x_op = XDR_FREE;
	(void)xdr_authunix_parms(&xdrs, &aup);
	XDR_DESTROY(&xdrs);
	return (stat);
}

static void
authunix_destroy(auth)
	register AUTH *auth;
{
	register struct audata *au = AUTH_PRIVATE(auth);

	mem_free(au->au_origcred.oa_base, au->au_origcred.oa_length);

	if (au->au_shcred.oa_base != NULL)
		mem_free(au->au_shcred.oa_base, au->au_shcred.oa_length);

	mem_free(auth->ah_private, sizeof(struct audata));

	if (auth->ah_verf.oa_base != NULL)
		mem_free(auth->ah_verf.oa_base, auth->ah_verf.oa_length);

	mem_free((caddr_t)auth, sizeof(*auth));
}

/*
 * Marshals (pre-serializes) an auth struct.
 * sets private data, au_marshed and au_mpos
 */
static bool_t
marshal_new_auth(auth)
	register AUTH *auth;
{
	XDR		xdr_stream;
	register XDR	*xdrs = &xdr_stream;
	register struct audata *au = AUTH_PRIVATE(auth);

	nlmsg_fd = _nfs_nls_catopen();

	xdrmem_create(xdrs, au->au_marshed, MAX_AUTH_BYTES, XDR_ENCODE);
	if ((! xdr_opaque_auth(xdrs, &(auth->ah_cred))) ||
	    (! xdr_opaque_auth(xdrs, &(auth->ah_verf)))) {
		perror((catgets(nlmsg_fd,NL_SETN,4, "marshall_new_auth: Fatal marshalling problem")));
	} else {
		au->au_mpos = XDR_GETPOS(xdrs);
	}
	XDR_DESTROY(xdrs);
}
