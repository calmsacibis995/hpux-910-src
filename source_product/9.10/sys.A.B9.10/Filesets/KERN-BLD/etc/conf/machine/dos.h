/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/dos.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:12:06 $
 */
/* HPUX_ID: @(#)dos.h	46.1		86/12/18 */
#ifndef _MACHINE_DOS_INCLUDED /* allows multiple inclusion */
#define _MACHINE_DOS_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

struct	arguid {
	int	ruid;
	int	euid;
};
struct	pcatargs {
	int	shmid;
	int	log;
};

/* physical memory mapping ioctl command */
#define	PCATPHYS	_IOW('M',1,struct pcatargs)
#define	PCATDMA		_IO('M',2)
#define	PCATVERSION	_IO('M',3)
#define	PCATACOUNT	_IOW('M',4,struct pcatargs)
#define	PCATCI		_IOW('M',5,struct pcatargs)
#define	PCATUID		_IOW('M',6,struct arguid)
#define	PCATMEMSIZE	_IO('M',7)
#define	PCATMEMSTART	_IO('M',8)

#define PCATVERSIONNO	200
#endif /* _MACHINE_DOS_INCLUDED */
