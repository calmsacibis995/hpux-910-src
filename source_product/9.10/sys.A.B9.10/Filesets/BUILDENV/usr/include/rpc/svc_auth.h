#ifdef MODULE_ID
/*
 * @(#)svc_auth.h: $Revision: 1.6.83.3 $ $Date: 93/09/17 19:28:19 $
 * $Locker:  $
 */
#endif /* MODULE_ID */
/*
 * REVISION: @(#)10.1
 */

/*
 * svc_auth.h, Service side of rpc authentication.
 * 
 * (c) Copyright 1987 Hewlett-Packard Company
 * (c) Copyright 1984 Sun Microsystems, Inc.
 */


/*
 * Server side authenticator
 */
extern enum auth_stat _authenticate();
