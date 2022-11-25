/* @(#) $Revision: 1.11.83.5 $ */       
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/vm.h,v $
 * $Revision: 1.11.83.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 12:41:03 $
 */
#ifndef _SYS_VM_INCLUDED /* allows multiple inclusion */
#define _SYS_VM_INCLUDED

/*
 *	#include "../h/vm.h"
 * or	#include <vm.h>		 in a user program
 * is a quick way to include all the vm header files.
 */
#ifdef _KERNEL_BUILD
#include "../h/vmparam.h"
#include "../h/vmmac.h"
#include "../h/vmmeter.h"
#include "../h/vmsystm.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/vmparam.h>
#include <sys/vmmac.h>
#include <sys/vmmeter.h>
#include <sys/vmsystm.h>
#endif /* _KERNEL_BUILD */
#endif /* _SYS_VM_INCLUDED */
