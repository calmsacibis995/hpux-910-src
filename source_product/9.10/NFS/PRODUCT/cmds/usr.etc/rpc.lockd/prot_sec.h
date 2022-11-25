/* @(#)rpc.lockd:	$Revision: 1.7.109.1 $	$Date: 91/11/19 14:17:56 $
 * 
 * $File$ 
 *
 * This file consists of code to make rpc.lockd work on a secure HP-UX system
 */

#ifdef SecureWare
#include <sys/types.h>
#include <sys/security.h>
#include <prot.h>
#endif /* SecureWare */

#if defined(SecureWare) && defined(B1)
extern mask_t  eff_privs[SEC_SPRIVVEC_SIZE];

#define ENABLEPRIV(priv) \
	 { \
          if (ISB1)  \
             if (nfs_enablepriv(priv)) \
		logmsg( \
(catgets(nlmsg_fd,NL_SETN,340, "%s:  needs to be executed with SEC_ALLOWDACACCESS, SEC_REMOTE, SEC_KILL and\n              SEC_ALLOWMACACCESS privileges", progname)) ); \
	 }

#define DISABLEPRIV(priv)       if (ISB1)  nfs_disablepriv(priv);

#else

#define ENABLEPRIV(priv) 	{}
#define DISABLEPRIV(priv)	{}

#endif /* SecureWare && B1 */

