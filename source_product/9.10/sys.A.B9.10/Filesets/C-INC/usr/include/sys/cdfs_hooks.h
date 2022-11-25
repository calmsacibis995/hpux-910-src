/*
 * @(#)cdfs_hooks.h: $Revision: 1.5.83.3 $ $Date: 93/09/17 16:36:49 $
 * $Locker:  $
 */

#ifndef _SYS_CDFS_HOOKS_INCLUDED
#define _SYS_CDFS_HOOKS_INCLUDED

/*
** This header is used solely for configuring out portions
** of the CDFS code. The macro CDFSCALL(XX)() is used in several
** places in the permanently resident portion of the kernel.
** The entire cdfsproc[] array of pointers to functions is 
** initialized to nop functions (cdfs_nop(){}), and is then
** filled in at boot time as appropriate. The idea is that
** all undefined externals are able to be resolved in this 
** manner. 
*/


extern int(*cdfsproc[])();
/*
** usage: CDFSCALL(function)(parm1, parm2, ...) 
*/
#define CDFSCALL(function) (*cdfsproc[function])


/*
** The following definitions are used as indices
** into the cdfsproc table.
*/

#define CDFIND				0
#define CDGET 				1
#define CDUNLOCK_PROC			2
#define PKRMCD				3
#define STPRMCD				4
#define SEND_REF_UPDATE_CD		5
#define SEND_REF_UPDATE_NO_CDNO		6
#define CDHINIT				7
#define DUXCD_PSEUDO_INACTIVE		8
#define CLOSE_SEND_NO_CDNO		9
#define CLOSECD_SERVE   		10
#define ECDGET 				11
#define CDFLUSH				12
#define CDFS_GETATTR			13


/* 
 * Added for a test 
 */
#define SERVESTRATREADCD		14
#define CDNO_CLEANUP			15
#define CDFS_STRATEGY			16
#define DUXCD_STRATEGY			17
#define FSCTLCD_SERVE			18


#endif /* not _SYS_CDFS_HOOKS_INCLUDED */
