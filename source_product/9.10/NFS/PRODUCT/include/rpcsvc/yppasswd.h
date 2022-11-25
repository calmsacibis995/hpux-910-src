/*	@(#)yppasswd.h	$Revision: 1.16.109.1 $	$Date: 91/11/19 14:43:28 $  */

/*      (c) Copyright 1987 Hewlett-Packard Company  */
/*      (c) Copyright 1985 Sun Microsystems, Inc.   */

#ifndef _RPCSVC_YPPASSWD_INCLUDED
#define _RPCSVC_YPPASSWD_INCLUDED

#define YPPASSWDPROG 100009
#define YPPASSWDPROC_UPDATE 1
#define YPPASSWDVERS_ORIG 1
#define YPPASSWDVERS 1

struct yppasswd {
	char *oldpass;		/* old (unencrypted) password */
	struct passwd newpw;	/* new pw structure */
};

int xdr_yppasswd();

#endif /* _RPCSVC_YPPASSWD_INCLUDED */
