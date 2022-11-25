/* @(#)rpc.statd:	$Revision: 1.4.109.1 $	$Date: 91/11/19 14:19:19 $
 * 
 * $File$ 
 *
 * This file consists of code to make rpc.statd work on a secure HP-UX system
 */

#ifdef SecureWare
#include <sys/types.h>
#include <sys/security.h>
#include <prot.h>
#endif /* SecureWare */

#if defined(SecureWare) && defined(B1)

#define ENABLEPRIV(priv) \
	 { \
          if (ISB1)  \
             if (nfs_enablepriv(priv)) \
		logmsg( \
(catgets(nlmsg_fd,NL_SETN,90, "statd:  needs to be executed with SEC_ALLOWDACACCESS, SEC_ALLOWMACACCESS\n           and SEC_CHOWN privileges")) ); \
	 }

#define DISABLEPRIV(priv)       if (ISB1)  nfs_disablepriv(priv);

#else

#define ENABLEPRIV(priv) 	{}
#define DISABLEPRIV(priv)	{}

#endif /* SecureWare && B1 */


