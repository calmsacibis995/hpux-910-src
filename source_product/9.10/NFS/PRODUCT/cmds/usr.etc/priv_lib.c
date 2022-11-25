#ifndef lint
static char rcsid[] = "@(#)$File$        $Revision: 1.2.109.1 $      $Date: 91/11/19 14:11:03 $";
#endif
/* @(#)$File$    $Revision: 1.2.109.1 $      $Date: 91/11/19 14:11:03 $ */

/*
 * Copyright (c) 1990 Hewlett-Packard, Inc.
 */

#include <sys/types.h>
#include <sys/security.h>
#include <mandatory.h>   
#include <prot.h>
#include <protcmd.h>

mask_t 	eff_privs[SEC_SPRIVVEC_SIZE];

nfs_initpriv()
{
	return(getpriv(SEC_EFFECTIVE_PRIV, eff_privs));	
}

/* 
 * Turn on the privilege.  If there is a problem such as there is no
 * authorization for it, an return of -1 will be done.
 */
int nfs_enablepriv(priv)
int	priv;
{
	int ret = 0;

	if (!ISBITSET(eff_privs, priv)) {
		ADDBIT(eff_privs, priv);
		if ((ret = setpriv(SEC_EFFECTIVE_PRIV, eff_privs)) < 0)
			RMBIT(eff_privs, priv);
	}
	return(ret);
}


/*
 * Turn off the privilege
 */

int nfs_disablepriv(priv)
int	priv;
{
	int ret = 0;

	if (ISBITSET(eff_privs, priv)) {
		RMBIT(eff_privs, priv);
		ret = setpriv(SEC_EFFECTIVE_PRIV, eff_privs);
	}
	return(ret);
}
