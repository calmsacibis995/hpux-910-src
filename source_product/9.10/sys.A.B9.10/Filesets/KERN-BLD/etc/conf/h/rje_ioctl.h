/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/h/RCS/rje_ioctl.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:01:34 $
 */
/* @(#) $Revision: 1.3.84.3 $ */    
#ifndef _SYS_RJE_IOCTL_INCLUDED /* allows multiple inclusion */
#define _SYS_RJE_IOCTL_INCLUDED
#ifdef __hp9000s300

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

typedef struct {
      unsigned char	req_code;
      unsigned char     req_subf;
}rje_cmd;

struct rje_stat{
      unsigned char	ret_stat[7];
};

struct rje_err{
      unsigned char	ret_int_err[55];
};

#define		RJE_READ	_IOW('j',50,rje_cmd)
#define		RJE_WRITE	_IOW('j',51,rje_cmd)
#define		RJE_CONT_LINK	_IOW('j',3,rje_cmd)
#define		RJE_CONT_PDI	_IOW('j',6,rje_cmd)
#define		RJE_CONT_TRACE	_IOW('j',9,rje_cmd)
#define		RJE_REP_TRANS	_IOWR('j',21,struct rje_stat)
#define		RJE_REP_ERROR	_IOR('j',31,struct rje_err)
#define		RJE_SELF_TEST	_IOR('j',30,struct rje_stat)

#endif /* __hp9000s300 */
#endif /* _SYS_RJE_IOCTL_INCLUDED */
