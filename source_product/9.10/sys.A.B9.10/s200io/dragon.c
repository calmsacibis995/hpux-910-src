/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/dragon.c,v $
 * $Revision: 1.8.84.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/08 12:08:40 $
 */

/* HPUX_ID: @(#)dragon.c	55.1		88/12/23 */

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

#include "../h/param.h"
#include "../s200/pte.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../wsio/intrpt.h"
#include "../s200io/dragon.h"
#include "../h/pregion.h"
#include "../h/mman.h"
extern preg_t *io_map();

#define	HP98248A	DRAGON_ID		/* may need to move this to
						   hpibio.h */
caddr_t dragon_base = (char *) 0x20038000;	/* should this be dynamic ?   */
struct proc *dragon_bank_map[DRAGON_MAXBANK]; 	/* zero means available, 
						   otherwise used by proc */	
struct dragon_mmap_type *dragon_mmap = NULL;	/* only one dragon per system */
extern int dragon_present;


dragon_isr(inf)
struct interrupt *inf;
{

	/* check for right context must be the current one */
	if (u.u_pcb.pcb_dragon_bank != (short)(dragon_mmap->bank_number)) {
		printf("WARNING: FPA MAY GENERATE SPURIOUS INTERRUPT\n");
		printf("WARNING: MAY LOSE SIGFPE FROM FPA\n");
	} else {
		/* highest bit indicates dragon */
		u.u_code = dragon_mmap->statusreg | 0x80000000; 
		
		psignal(u.u_procp, SIGFPE);
	}

	/* clear interrupt request */
	dragon_int_clear();	
	/* reenable interrupt      */
	dragon_int_enable();

}


dragon_init()
{
	struct pte proto;
	register int pfnum, i;
#ifdef	DRAGON_DEBUG
	int	int_lev;
#endif	DRAGON_DEBUG

	dragon_mmap = (struct dragon_mmap_type *)DRAGON_DIO_ADDR;

	/* reset the hardware */
	dragon_reset();
				
#ifdef	DRAGON_DEBUG
	int_lev = (((*(((caddr_t)dragon_mmap)+3) >> 4) & 0x3) + 3);
	if (int_lev != DRAGON_INT_LVL)
		printf("WARNING: DRAGON 's interrupt level is at %d\n",int_lev);
#endif	DRAGON_DEBUG

	/* install our interrupt service routine */
	isrlink(dragon_isr, DRAGON_INT_LVL, &dragon_mmap->intr_control,
		DRAGON_IE | DRAGON_IR, DRAGON_IE | DRAGON_IR, 0, 0);
	dragon_int_enable();
}


dragon_int_enable()
{
	dragon_mmap->intr_control = DRAGON_IE; 
}
dragon_int_clear()
{
	dragon_mmap->intr_control = 0; 	/* zeroes out IE clears IR */
}

dragon_reset()
{
	dragon_mmap->id_reg = 0; 
}

dragon_detach(p)
struct proc *p;
{
	preg_t *prp;

        /* Get the pregion this view was through */
        if ((prp = findprp(p->p_vas, (space_t)(p->p_vas), DRAGON_AREA)) == NULL) {
                panic("dragon_detach: bad logical_addr");
        }

        /* now cleanup the mess */
        io_unmap(prp);
}

dragon_bank_alloc()
{
	int i;

	while (1) {
		for (i=0; i< DRAGON_MAXBANK; i++) {
			if (dragon_bank_map[i] == NULL)
				break;
		}
		if (i >= DRAGON_MAXBANK) {
#ifdef	DRAGON_DEBUG
			printf("DRAGON register banks over flow\n");
#endif	DRAGON_DEBUG
			sleep((caddr_t)dragon_bank_map, PSLEP);
		} else {
			/* found available dragon bank */
			break;
		}
	}
	u.u_pcb.pcb_dragon_sr = DRAGON_STATUS_V;
	u.u_pcb.pcb_dragon_cr = DRAGON_CNTRL_V;
	u.u_pcb.pcb_dragon_bank = i;
	dragon_bank_map[i] = u.u_procp;
}

dragon_bank_free()
{
	dragon_bank_map[u.u_pcb.pcb_dragon_bank] = NULL;
	u.u_pcb.pcb_dragon_bank = -1;
	/* should probably only do this if we know someone is waiting */
	wakeup((caddr_t)dragon_bank_map);
}

dragon_bank_select()
{
	/* printf("dragon_bank_select\n"); */
	if (u.u_pcb.pcb_dragon_bank != -1) {	/* should do this outside 
					   to save a routine call
					*/
		dragon_mmap->bank_number = u.u_pcb.pcb_dragon_bank;
		dragon_mmap->statusreg =   u.u_pcb.pcb_dragon_sr;
		dragon_mmap->controlreg =  u.u_pcb.pcb_dragon_cr;
	}
}

dragon_bank_save()
{
	if (u.u_pcb.pcb_dragon_bank != -1) {
		u.u_pcb.pcb_dragon_sr =	dragon_mmap->statusreg;
		u.u_pcb.pcb_dragon_cr =	dragon_mmap->controlreg;
	}
}



dragon_buserror()
{
	preg_t *prp;

	/* the u.u_pcb.pcb_dragon_sr and u.u_pcb.pcb_dragon_cr should be
	   initialized before u.u_pcb.pcb_dragon_bank cause
	   u.u_pcb.pcb_dragon_bank is used also as flag for other
	   actions 
	*/
	dragon_bank_alloc();
	if ((prp = io_map(u.u_procp->p_vas, DRAGON_AREA,
			  dragon_base, DRAGON_PAGES, PROT_URW)) == NULL)
		return(0);
	dragon_bank_select();
	return(1);
}

/*	Upon entry: saveaddr points to a kernel area of 136 bytes  */
dragon_save_regs(saveaddr)
int *saveaddr;
{
	saveaddr[0] = *((int *)((char *)dragon_mmap + DRAGON_REG0H));
	saveaddr[1] = *((int *)((char *)dragon_mmap + DRAGON_REG0L));
	saveaddr[2] = *((int *)((char *)dragon_mmap + DRAGON_REG1H));
	saveaddr[3] = *((int *)((char *)dragon_mmap + DRAGON_REG1L));
	saveaddr[4] = *((int *)((char *)dragon_mmap + DRAGON_REG2H));
	saveaddr[5] = *((int *)((char *)dragon_mmap + DRAGON_REG2L));
	saveaddr[6] = *((int *)((char *)dragon_mmap + DRAGON_REG3H));
	saveaddr[7] = *((int *)((char *)dragon_mmap + DRAGON_REG3L));
	saveaddr[8] = *((int *)((char *)dragon_mmap + DRAGON_REG4H));
	saveaddr[9] = *((int *)((char *)dragon_mmap + DRAGON_REG4L));
	saveaddr[10] = *((int *)((char *)dragon_mmap + DRAGON_REG5H));
	saveaddr[11] = *((int *)((char *)dragon_mmap + DRAGON_REG5L));
	saveaddr[12] = *((int *)((char *)dragon_mmap + DRAGON_REG6H));
	saveaddr[13] = *((int *)((char *)dragon_mmap + DRAGON_REG6L));
	saveaddr[14] = *((int *)((char *)dragon_mmap + DRAGON_REG7H));
	saveaddr[15] = *((int *)((char *)dragon_mmap + DRAGON_REG7L));
	saveaddr[16] = *((int *)((char *)dragon_mmap + DRAGON_REG8H));
	saveaddr[17] = *((int *)((char *)dragon_mmap + DRAGON_REG8L));
	saveaddr[18] = *((int *)((char *)dragon_mmap + DRAGON_REG9H));
	saveaddr[19] = *((int *)((char *)dragon_mmap + DRAGON_REG9L));
	saveaddr[20] = *((int *)((char *)dragon_mmap + DRAGON_REGAH));
	saveaddr[21] = *((int *)((char *)dragon_mmap + DRAGON_REGAL));
	saveaddr[22] = *((int *)((char *)dragon_mmap + DRAGON_REGBH));
	saveaddr[23] = *((int *)((char *)dragon_mmap + DRAGON_REGBL));
	saveaddr[24] = *((int *)((char *)dragon_mmap + DRAGON_REGCH));
	saveaddr[25] = *((int *)((char *)dragon_mmap + DRAGON_REGCL));
	saveaddr[26] = *((int *)((char *)dragon_mmap + DRAGON_REGDH));
	saveaddr[27] = *((int *)((char *)dragon_mmap + DRAGON_REGDL));
	saveaddr[28] = *((int *)((char *)dragon_mmap + DRAGON_REGEH));
	saveaddr[29] = *((int *)((char *)dragon_mmap + DRAGON_REGEL));
	saveaddr[30] = *((int *)((char *)dragon_mmap + DRAGON_REGFH));
	saveaddr[31] = *((int *)((char *)dragon_mmap + DRAGON_REGFL));
	saveaddr[32] = 		   dragon_mmap->statusreg;
	saveaddr[33] = 		   dragon_mmap->controlreg;
}

/*	Upon entry: saveaddr points to a kernel area of 136 bytes         */
/*	Is the order of restoring general purpose reg relative to         */
/*	restoring control/status reg important? Put it in a different  way*/
/*	Does write to general regs affect status reg ? 		          */
/*	Answer is no (hardware documentation should address this)	  */		
dragon_restore_regs(saveaddr)
int *saveaddr;
{
	*((int *)((char *)dragon_mmap + DRAGON_REG0H)) = saveaddr[0];
	*((int *)((char *)dragon_mmap + DRAGON_REG0L)) = saveaddr[1];
	*((int *)((char *)dragon_mmap + DRAGON_REG1H)) = saveaddr[2];
	*((int *)((char *)dragon_mmap + DRAGON_REG1L)) = saveaddr[3];
	*((int *)((char *)dragon_mmap + DRAGON_REG2H)) = saveaddr[4];
	*((int *)((char *)dragon_mmap + DRAGON_REG2L)) = saveaddr[5];
	*((int *)((char *)dragon_mmap + DRAGON_REG3H)) = saveaddr[6];
	*((int *)((char *)dragon_mmap + DRAGON_REG3L)) = saveaddr[7];
	*((int *)((char *)dragon_mmap + DRAGON_REG4H)) = saveaddr[8];
	*((int *)((char *)dragon_mmap + DRAGON_REG4L)) = saveaddr[9];
	*((int *)((char *)dragon_mmap + DRAGON_REG5H)) = saveaddr[10];
	*((int *)((char *)dragon_mmap + DRAGON_REG5L)) = saveaddr[11];
	*((int *)((char *)dragon_mmap + DRAGON_REG6H)) = saveaddr[12];
	*((int *)((char *)dragon_mmap + DRAGON_REG6L)) = saveaddr[13];
	*((int *)((char *)dragon_mmap + DRAGON_REG7H)) = saveaddr[14];
	*((int *)((char *)dragon_mmap + DRAGON_REG7L)) = saveaddr[15];
	*((int *)((char *)dragon_mmap + DRAGON_REG8H)) = saveaddr[16];
	*((int *)((char *)dragon_mmap + DRAGON_REG8L)) = saveaddr[17];
	*((int *)((char *)dragon_mmap + DRAGON_REG9H)) = saveaddr[18];
	*((int *)((char *)dragon_mmap + DRAGON_REG9L)) = saveaddr[19];
	*((int *)((char *)dragon_mmap + DRAGON_REGAH)) = saveaddr[20];
	*((int *)((char *)dragon_mmap + DRAGON_REGAL)) = saveaddr[21];
	*((int *)((char *)dragon_mmap + DRAGON_REGBH)) = saveaddr[22];
	*((int *)((char *)dragon_mmap + DRAGON_REGBL)) = saveaddr[23];
	*((int *)((char *)dragon_mmap + DRAGON_REGCH)) = saveaddr[24];
	*((int *)((char *)dragon_mmap + DRAGON_REGCL)) = saveaddr[25];
	*((int *)((char *)dragon_mmap + DRAGON_REGDH)) = saveaddr[26];
	*((int *)((char *)dragon_mmap + DRAGON_REGDL)) = saveaddr[27];
	*((int *)((char *)dragon_mmap + DRAGON_REGEH)) = saveaddr[28];
	*((int *)((char *)dragon_mmap + DRAGON_REGEL)) = saveaddr[29];
	*((int *)((char *)dragon_mmap + DRAGON_REGFH)) = saveaddr[30];
	*((int *)((char *)dragon_mmap + DRAGON_REGFL)) = saveaddr[31];
	dragon_mmap->statusreg = saveaddr[32];
	dragon_mmap->controlreg = saveaddr[33];
}

dragon_read_ureg(p, useraddr)
struct proc *p;
caddr_t	*useraddr;
{
	int i;
	int dragonregs[34];	/* 0-31 has fpa0-fpa15 */
				/* 32   has fpasr      */
				/* 33   has fpacr      */ 
				/* should this be a constant in header file? */
	
	for (i=0; i< DRAGON_MAXBANK; i++) {
		if (dragon_bank_map[i] == p)
			break;
	}
	if (i >= DRAGON_MAXBANK) {
#ifdef	DRAGON_DEBUG
		printf("DRAGON_read_ureg: process does not use dragon\n");
#endif	DRAGON_DEBUG
		/* process has not used dragon */
		u.u_error = EIO;	/* consistent with the rest */
		return;			/* of ptrace error number   */
	}
	if (u.u_pcb.pcb_dragon_bank != -1) 
		dragon_bank_save();
	
	dragon_mmap->bank_number = i;	/* select bank i */
	dragon_save_regs(dragonregs);

	/* how does adb know how big is the data area ??? */
	if (copyout(dragonregs, useraddr, sizeof(dragonregs)))
		u.u_error = EFAULT;

	if (u.u_pcb.pcb_dragon_bank != -1) 
		dragon_bank_select();
}

dragon_write_ureg(p, useraddr)
struct proc *p;
caddr_t	*useraddr;
{
	int i;
	int dragonregs[34];	/* 0-31 has fpa0-fpa15 */
				/* 32   has fpasr      */
				/* 33   has fpacr      */ 
				/* should this be a constant in header file? */
	
	for (i=0; i< DRAGON_MAXBANK; i++) {
		if (dragon_bank_map[i] == p)
			break;
	}
	if (i >= DRAGON_MAXBANK) {
#ifdef	DRAGON_DEBUG
		printf("DRAGON_write_ureg: process does not use dragon\n");
#endif	DRAGON_DEBUG
		/* process has not used dragon */
		u.u_error = EIO;	/* consistent with the rest */
		return;			/* of ptrace error number   */
	}
	if (u.u_pcb.pcb_dragon_bank != -1) 
		dragon_bank_save();
	
	dragon_mmap->bank_number = i;	/* select bank i */
	if (copyin(useraddr, dragonregs, sizeof(dragonregs)))
		u.u_error = EFAULT;

	dragon_restore_regs(dragonregs);

	if (u.u_pcb.pcb_dragon_bank != -1) 
		dragon_bank_select();
}
