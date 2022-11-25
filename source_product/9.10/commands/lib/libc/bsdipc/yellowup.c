/* @(#)$Header: yellowup.c,v 64.3 89/01/26 12:21:07 jmc Exp $ */


/*	yellowup.c	--	determine whether or not the Yellow Pages are up
****
**	used by several modules (gethostent.c, getnetent.c, getrpcent.c,
**	getservent.c, gprotoent.c, gtnetgrent.c, innetgr.c) to determine
**	whether or not the Yellow Pages facility is running; they use the
**	usingyellow variable as an extern now, and call yellowup()
*/

#ifdef HP_NFS

#ifdef _NAMESPACE_CLEAN
#define yp_bind _yp_bind
#define getdomainname _getdomainname
#define exit _exit
#define usingyellow _usingyellow
#endif

#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>

extern int	usingyellow;		/* are the Yellow Pages up?	*/

/* 
**	yellowup()	--	determine whether or not the Yellow Pages are up
**		Check to see if yellow pages are up in the given domain, and
**		store that fact in usingyellow.  This check is performed once
**		at startup and thereafter if the flag parameter is set.
**
**		yellowup() returns the current value of usingyellow regardless.
*/

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _yellowup yellowup
#define yellowup _yellowup
#endif

int
yellowup(flag, domain)
int	flag;
char	*domain;
{
	static int firsttime = 1;

	if (firsttime || flag) {
		firsttime = 0;
		if (domain[0] == 0)
			if (getdomainname(domain, YPMAXDOMAIN+1) < 0)
				exit(1);
		usingyellow = !yp_bind(domain);
	}	
	return (usingyellow);
}

#endif HP_NFS
