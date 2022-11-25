/* @(#)rpc.lockd:	$Revision: 1.4.109.1 $	$Date: 91/11/19 14:18:22 $
*/
/* (#)sm_res.h	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)sm_res.h	1.2 86/12/30 NFSSRC */
/*
 * (#)sm_res.h 1.1 86/09/24
 */

struct stat_res{
	res res_stat;
	union {
		sm_stat_res stat;
		int rpc_err;
	}u;
};
#define sm_stat 	u.stat.res_stat
#define sm_state 	u.stat.state

