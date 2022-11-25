
/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/sys/ulimit.c,v $
 * $Revision: 70.1 $	$Author: ssa $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/03/23 19:59:32 $
 *
 * $Log:	ulimit.c,v $
 * Revision 70.1  93/03/23  19:59:32  19:59:32  ssa
 * *** empty log message ***
 * 
 * Revision 72.1  92/09/30  14:00:23  14:00:23  ssa (RCS Manager)
 * Author: stratton@hpucsb3.cup.hp.com
 * Initial revision
 * 
 * Revision 64.2  89/01/19  16:29:14  16:29:14  marklin (Mark Lin)
 * Namespace cleanup using _NAMESPACE_CLEAN flag.
 * 
 * Revision 1.4  86/09/25  07:54:58  07:54:58  lkc (Lee Casuto)
 * only sets global flag on a SET command so as not to affect performance
 * 
 * Revision 1.3  86/09/24  10:46:32  10:46:32  lkc (Lee Casuto)
 * new file created in sys to hold global variable _ulimit_called;
 * this was done so that the svvs would pass
 * 
 * Revision 1.2  84/10/02  23:54:29  23:54:29  wallace (Kevin Wallace)
 * Header added.  
 * 
 * $Endlog$
 */

#define	SET	2
int _ulimit_called = -1;

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF __ulimit ulimit
#define ulimit __ulimit
#endif

ulimit(cmd, newlimit)
	int cmd;
	long newlimit;
{
	long _ulimit();

	if(cmd == SET)
		_ulimit_called = 0;		/* arm flag	*/
	return(_ulimit(cmd, newlimit));
}
