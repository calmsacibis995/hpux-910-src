/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/dos.c,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:12:00 $
 */

/* HPUX_ID: @(#)dos.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


/* Phoenix card driver. 
--	This driver provides addtional OS function for the DOS coprocessor 
	software and hardware (Product numbers 98286A and 98531A).
	
	All functions are supported via the ioctl system call.  Read and write
	system calls will return the ENODEV error.   The driver will only 
	allow one process to have it open at any given time.  Once opened,
	subsequent opens will return the EBUSY error.  This is used  by user
	processes to obtain exclusive access to the hardware.
	
	The following IOCTL calls are supported:
		PCATMEMSIZE:
			Returns a value coded as follows
				0xSSSSSVVV
			0xSSSSS000  is the amount of memory that has been 
			reserved for the DOS coprocessor.  (unit is bytes)
			VVV indicates how memory can be used.  Shared memory
			can be allocated, locked and used only if it can be
			addressed by the 98286A card.  Systems with more
			than 6Mb shared memory may contain pages which are
			outside the address range of the card.  Reserved memory
			(dos_mem_start, dos_mem_byte) must be used in this
			case. The following values are meaningful:
				001 -> indicates that shared memory (shm(1))
					can be used for dos coprocessor.  
				000 -> shared memory can not be used.
		PCATMEMSTART:
			Return physical address of reserved memory (if any). 
			This only has significance if SSSSS (from above) is
			non zero.
		PCATUID:
			Sets the effective and real user id's for this 
			process.  One of real, effective, or saved id must be
			superuser in order to do this.  If either input 
			parameters is -1 then corresponding id is unchanged.
		PCATDMA:
			Return the bus master position of the card.  The card
			must be the last bus master in the chain.  The position 
			is coded in the lower two bits, and is the ones 
			complement of the actual position.  For example:
			    if no other bus masters - return 3 (position 00)
			    if one other bus masters - return 2 (position 01)
		PCATPHYS:
			Return the physical address of a page of memory.  Pages
			are assumed to be 4K in length.
		PCATACOUNT:
			Increment the current  attached count to guarantee that
			a locked shared memory segment wont be swapout(ed) even 
			no process  currently in core is actually attaching to 
			it.  This routine is really a kludge.  It violates
			the shared memory interface.  It understands the inner 
			implementation of shared memory module and takes 
			advantage of a side effect.  This is what we get for 
			adding things after the os has been shipped !!!!!!!!
		PCATCI:
			Turn on the cache inhibit line for the pages in a 
			segment of shared memory.
		PCATVERSION:
			Returns the current version of the driver.  This value
			can only be changed if corresponding DOS coprocessor
			software is changed.
*/

#include "../h/types.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../machine/pte.h"
#include "../machine/cpu.h"
#include "../h/dir.h"
#include "../h/uio.h"
#include "../h/errno.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../s200io/dos.h"

#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/ipc.h"
#include "../h/shm.h"


#define	NUMBUSMASTERS	3

extern int bus_master_count;
/* extern	int	dma_here; */
extern	int	dos_mem_byte;
extern	uint	dos_mem_start;
static	char	card_open = 0;

/* ------------------------------------ */
/* dos_open -                           */
/* ------------------------------------ */
dos_open (dev, flag)
dev_t	dev;
int	flag;
{
	if (card_open) {
		u.u_error = EBUSY;
		return (-1);
	}
	card_open++;
	return (0);
}


/* ------------------------------------ */
/* dos_write -                          */
/* ------------------------------------ */
dos_write (dev)
dev_t	dev;
{
	u.u_error = ENODEV;
	return (ENODEV);
}


/* ------------------------------------ */
/* dos_read -                           */
/* ------------------------------------ */
dos_read (dev)
dev_t	dev;
{
	u.u_error = ENODEV;
	return (ENODEV);
}



/* ------------------------------------ */
/* dos_close -                          */
/* ------------------------------------ */
dos_close (dev)
dev_t		dev;
{
	card_open = 0;
	return (0);
}




/* ------------------------------------ */
/* dos_ioctl -                          */
/* ------------------------------------ */
dos_ioctl (dev, cmd, arg, mode)
dev_t		dev;
int		cmd;
struct pcatargs	*arg;
int		mode;		/* mode not used */
{
	unsigned int value ;
	int limit;

	switch (cmd) {
		case PCATMEMSIZE:
				if (dos_mem_byte <= 0)
					value = 0;
				else   value = dos_mem_byte & (~0xfff);

				switch(machine_model)
				{
			    case MACH_MODEL_310:
			    case MACH_MODEL_320:
				    limit = 0xff800;
				    break;
			    case MACH_MODEL_330:
			    case MACH_MODEL_350:
			    default:
				    limit = 0xffa00;
				    break;
				}

				if (firstfree < limit) 
				     u.u_r.r_val1 =  value | 0x0;
				else u.u_r.r_val1 =  value | 0x1;
				break;
		case PCATMEMSTART:
				u.u_r.r_val1 =  (unsigned int )dos_mem_start;
				break;
		case PCATUID :	pcat_setreuid (arg);
				break;
		case PCATDMA :	
				/* 5.2 implementation
				if (dma_here == 0)
					u.u_r.r_val1 =  3;
				  else	u.u_r.r_val1 =  2;
				*/
				u.u_r.r_val1 = NUMBUSMASTERS - bus_master_count;
				break;	
		case PCATPHYS : u.u_r.r_val1 =  
					pcat_logtophy (arg->log, arg->shmid);
				break;
		case PCATACOUNT:u.u_r.r_val1 = pcat_bumpcatcnt(arg->shmid);
				break;
		case PCATCI:	u.u_r.r_val1 = pcat_cacheoff(arg->shmid);
				break;
		case PCATVERSION:
				u.u_r.r_val1 = PCATVERSIONNO;
				break;
		default : u.u_error = EINVAL;
			  return (EINVAL);
	}
	return (0);
}

/* ------------------------------------------------------------------ */
/* pcat_logtophy-                      				      */
/* returns the physical page on to which logical addr 'log' is mapped */
/* ------------------------------------------------------------------ */
pcat_logtophy (log, shmid)
int	log, shmid;
{
	struct	proc	*p = u.u_procp;
	struct	pte	*pte;
	struct	shmid_ds	*sp;		/* shm header ptr */

        /* check for valid segment */ 

	if ((sp = (struct shmid_ds *)shmconv(shmid, 0)) == NULL) 
		return(-1);
	
	/* segment must be locked and cattcount >= 2 (kludge) at this point 
	 * to guarantee PHYSICALLY locked. 
	 */

	if (ipc_lock(sp))
		return(-1);			/* ipc_lock sets u.u_error */
#ifdef REGION_LATER
	if (!(sp->shm_flag & SHM_NOPAGE) || (sp->shm_cnattch < 2)) {
		ipc_unlock(sp);
		return(-1);
	}
#endif
	ipc_unlock(sp);

#ifdef REGION_LATER
	if ((isashmsv(p, btop(log)) != sp))	/* range check on addr */
		return(-1);
#endif

	panic("pcat_logphy called");
	pte = vtopte(p, btop (log));
	return ((int)pte->pg_pfnum);
}

/************************************************************************
pcat_bumpcatcnt		

Increment the current  attached count to guarantee  that a locked shared
memory segment wont be swapout(ed) even no process  currently in core is
actually attaching to it.  This routine is really a kludge.  It violates
the shared memory interface.  It understands the inner implementation of
shared memory module and takes advantage of a side effect.  This is what
we get for adding things after the os has been shipped !!!!!!!!
************************************************************************/

pcat_bumpcatcnt(shmid)
int shmid;
{
	struct	shmid_ds	*sp;		/* shm header ptr */

	/* check for valid segment */

	if ((sp = (struct shmid_ds *)shmconv(shmid, 0)) == NULL) 
		return(-1);
	
	if (ipc_lock(sp))
		return(-1);			/* ipc_lock sets u.u_error */
	sp->shm_cnattch++;
	ipc_unlock(sp);
	return(0);
}

/*************************************************************************
pcat_cacheoff.

Inhibit the data cache for shared memory segment 'shmid'
*************************************************************************/
pcat_cacheoff(shmid)
int shmid;
{
	struct	shmid_ds	*sp;		/* shm header ptr */

	/* check for valid segment */

	if ((sp = (struct shmid_ds *)shmconv(shmid, 0)) == NULL) 
		return(-1);
	
	if (ipc_lock(sp))
		return(-1);			/* ipc_lock sets u.u_error */
#ifdef REGION_LATER
	shm_cacheoff(sp);
#endif
	ipc_unlock(sp);
	return(0);
}



/*****************/
/* PCAT_SETREUID */
/*****************/
/*	Temporary support until the real setresuid call is supported.
*/
static pcat_setreuid (uap)
struct	arguid *uap;
{
	register int ruid, euid;

	ruid = uap->ruid;
	if (ruid == -1)
		ruid = u.u_ruid;
	if (u.u_ruid != ruid && u.u_uid != ruid && !suser())
		return;
	euid = uap->euid;
	if (euid == -1)
		euid = u.u_uid;
	if (u.u_ruid != euid && u.u_uid != euid && !suser())
		return;
	/*
	 * Everything's okay, do it.
	 */
	u.u_cred = crcopy(u.u_cred);
	/* unlink old uid, and add new into the chain */
	if (!(uremove(u.u_procp)))
		panic("pcat_setreuid: lost uidhx");
	ulink(u.u_procp,ruid);
	u.u_procp->p_uid = ruid;
	u.u_ruid = ruid;
	u.u_uid = euid;
}
