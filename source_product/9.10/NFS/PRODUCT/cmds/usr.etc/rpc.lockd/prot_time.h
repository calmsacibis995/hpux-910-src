/* @(#)rpc.lockd:	$Revision: 1.6.109.1 $	$Date: 91/11/19 14:18:05 $
*/
/* (#)prot_time.h	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)prot_time.h	1.2 86/12/30 NFSSRC */
/*
 * (#)prot_time.h 1.1 86/09/24
 *
 * This file consists of all timeout definition used by rpc.lockd
 */

#define MAX_LM_TIMEOUT_COUNT	1
#define OLDMSG			30		/* counter to throw away old msg */
#define LM_TIMEOUT_DEFAULT 	10
#define LM_GRACE_DEFAULT 	5
int 	LM_TIMEOUT;
int 	LM_GRACE;
#ifndef hpux
int	grace_period;
#endif hpux
