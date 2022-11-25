/* @(#)rpc.statd:	$Revision: 1.3.109.1 $	$Date: 91/11/19 14:19:30 $
*/
/* (#)sm_statd.h	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)sm_statd.h	1.2 86/12/30 NFSSRC */
/*
 * (#)sm_statd.h 1.1 86/09/24
 */

struct stat_chge {
	char *name;
	int state;
};
typedef struct stat_chge stat_chge;

#define SM_NOTIFY 6



