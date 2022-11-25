 /* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/umem.c,v $
 * $Revision: 1.2.84.6 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/11/30 16:29:03 $
 */

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




/* 
 *   issues			XXX
 *
 *	overload of rp->flags field - we record per-page ro/rw info,
 *	though it is a one-per-range structure
 *	
 *	hole in copyout, on both architectures
 */



/*
 * Memory special file driver for user memory
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/uio.h"
#include "../h/malloc.h"
#include "../h/ioctl.h"
#include "../machine/umem.h"
#include "../h/pregion.h"
#ifdef hp9000s300
#include "../s200/psl.h"
#include "../machine/pte.h"
#include "../s200/trap.h"
#else
#include "../machine/save_state.h"
#include "../machine/pde.h"
#include "../h/ptrace.h"
#include "../h/vfd.h"
#include "../h/region.h"
#undef flags
#endif



#ifdef hp9000s300
extern struct pte *tablewalk();
#endif	

int mm_set_range();
int mm_delete_range();
int mm_buserror();
#ifdef hp9000s700
space_t mm_ldusid();
#endif



umem_mmrw(dev, uio, rw, iov)
dev_t dev;
struct uio *uio;
enum uio_rw rw;
struct iovec *iov;
{
#ifdef hp9000s300
	register u_int c;
	int err = 0;
	struct pte *pte;
	u_int kvaddr;
#else
	space_t tracer_sid, tracee_sid;
#endif	
	struct umem_data *umem;
	int i, npages, curr_count, count;
	u_int curr_offset;


	umem = (struct umem_data *) u.u_fp->f_buf;

	/* 
	 * Do we still have the right process
	 * and does it still have it's virtual memory?
	 */
	if (umem->mpid != umem->mp->p_pid ||
	    umem->mp->p_stat == SZOMB)
		return ESRCH;

	npages = btop(uio->uio_offset+iov->iov_len) - btop(uio->uio_offset) + 1;
	if (npages < 1)
		return EINVAL;
	curr_offset = uio->uio_offset & PGOFSET;
	curr_count = iov->iov_len;
	for (i = 0; i < npages; i++) {
		count = min(curr_count, NBPG - curr_offset);
#ifdef hp9000s300
		pte = tablewalk(umem->mp->p_vas->va_hdl.va_seg, uio->uio_offset);
		if (pte == NULL)
			return EFAULT;

		/* 
		 * Convert the users virtual address into a kernel
		 * virtual address through the tt window.
		 */
		kvaddr = ptob(pte->pg_pfnum) | curr_offset;
		/* move the data */
		err = uiomove(kvaddr, count, rw, uio);
#else
		tracee_sid = mm_ldusid(umem->mp, uio->uio_offset);
		tracer_sid = ldusid(iov->iov_base);
		if (rw == UIO_READ) {
			hdl_copy_page(umem->mp->p_vas, tracee_sid, 
				uio->uio_offset, u.u_procp->p_vas, tracer_sid, 
				iov->iov_base, count);
		} else {
			hdl_copy_page(u.u_procp->p_vas, tracer_sid, 
				iov->iov_base, umem->mp->p_vas, tracee_sid, 
				uio->uio_offset, count);
		}
#endif
		curr_count -= count;
		curr_offset = (curr_count + count) & PGOFSET;

#ifdef hp9000s300
		/* return after the first error */
		if (err)
			return err;
#endif	
	}
	return 0;
}



extern int (*umem_exit_hook)();


/*
 *	exit routine - called when tracee exits or tracer exits/closes
 */
umem_exit(up)
struct umem_data *up;
{
	int i;
	

	for (i = 0; i < MAXRANGES; i++)
		if (up->ranges[i].flags)
			mm_delete_range(&up->ranges[i], up);
	if (up->mp && up->mp->p_pid == up->mpid) {
		up->mp->p_flag &= ~SUMEM;
		up->mp->p_cttyfp = NULL;
	}
	wakeup(up);
}


mm_open(dev, mode)
dev_t dev;
int mode;
{
	struct umem_data *umem;


	umem_exit_hook = umem_exit;
	if (minor(dev) != 4)
                return 0;

	umem = (struct umem_data *) kmalloc(UMEMD_SIZE, M_UMEMD, M_WAITOK);
	if (umem == NULL)
                return ENOMEM;
	bzero(umem, UMEMD_SIZE);
	u.u_fp->f_buf = (caddr_t) umem;

	return 0;
}



mm_close(dev)
dev_t dev;
{
	struct umem_data *umem;
	int i, x;

	umem = (struct umem_data *) u.u_fp->f_buf;

	if (minor(dev) != 4)
                return 0;

	if (u.u_fp->f_buf == NULL)
		panic("Something really bad happened!");
	if (umem->mp->p_stat && umem->mp->p_stat != SZOMB && 
	    umem->mp->p_pid == umem->mpid) {
	    	umem_exit(umem);
		wakeup(umem->fault_addr);
		if (umem->mp->p_stat == SSTOP) {
			x = spl6();
			SPINLOCK(sched_lock);
			setrun(umem->mp);
			SPINLOCK(sched_lock);
			splx(x);
		}
			
	}
	
	kfree(u.u_fp->f_buf, M_UMEMD);
	u.u_fp->f_buf = (caddr_t) NULL;
	return 0;
}


/*	XXX    security/robustness - if MM_SET_PID fails, process can still
	do most of the other ioctls, with disastrous results	*/
	

mm_ioctl(dev, cmd, data, flag)
dev_t dev;
int cmd;
int *data;
int flag;
{
	struct proc *p;
	struct umem_data *umem;
	struct addr_range *rp, *rp2;
	struct pte *pte;
	unsigned int addr;
	int x, i, npages;


	if (minor(dev) != 4)
		return ENOTTY;
	umem = (struct umem_data *) u.u_fp->f_buf;

        switch(cmd) {

		/*
		 *      find the PID; if it exists and belongs to this
		 *      user (in one sense or another :-), set access bit
		 */
		case MM_SET_PID:
			p = pfind(*data);
			if (p == 0)
				return(ESRCH);
			if (u.u_uid != p->p_uid && !suser())
				return(EPERM);
			if (p->p_flag & SUMEM)
				return(EALREADY);
				
			umem->mpid = (pid_t) *data;
			umem->mp = p;
			umem->tpid = u.u_procp->p_pid;
			umem->tp = u.u_procp;
			p->p_cttyfp = (struct proc *) umem;
			p->p_flag |= SUMEM;
			break;
	
		case MM_SET_SIG:
			if (*data <= 0 || *data >= _NSIG)
				return EINVAL;
			umem->sig = *data;
			break;
	
		/*
		 *	set up a range to be monitored and actions to
		 *	be taken on access to monitored range
		 */
		case MM_SET_RANGE:
			if (umem->mp == NULL)
				return(ESRCH);
			rp = (struct addr_range *) data;

			/* find an empty slot */
			for (i = 0; i < MAXRANGES && umem->ranges[i].flags; i++)
				;
			if (i >= MAXRANGES)
				return ENOSPC;
			umem->ranges[i] = *rp;
			mm_set_range(&umem->ranges[i], umem);
			break;

		case MM_DELETE_RANGE:
			if (umem->mp == NULL)
				return(ESRCH);
			rp = (struct addr_range *) data;		

			for (i=0,rp2=&umem->ranges[0]; i<MAXRANGES; i++,rp2++) {
				if (!rp2->flags)
					continue;
				if (rp2->start_addr == rp->start_addr &&
				    rp2->end_addr == rp->end_addr)
					break;
			}
			if (i >= MAXRANGES)
				return ENOENT;
			mm_delete_range(rp2, umem);
			break;
	
		case MM_SELECT:
			return(mm_Select(data));
			break;
			
		case MM_RESUME:
			if (umem && umem->mp && umem->mp->p_stat == SSTOP) {
				x = spl6();
				umem->fault_addr = 0;
				SPINLOCK(sched_lock);
				setrun(umem->mp);
				SPINUNLOCK(sched_lock);
				splx(x);
			}
				
			break;
			
		case MM_WAKEUP:
			if (suser())
				wakeup(*data);
			break;
			
		case MM_SUSPEND:
			if (suser())
				sleep(*data, PZERO+1);
			break;
			
		case MM_GET_STRUCT:
			bcopy(umem, data, sizeof(struct umem_data));
			break;
			
		case MM_GET_ADDR:
			if (umem && umem->mp && umem->mp->p_stat == SSTOP &&
			    umem->fault_addr)
				*data = umem->fault_addr;
			else
				return ENOENT;	     /* XXX  best errno???  */
			break;

		case MM_EXIT:
			if (umem->mp == NULL)
				return(ESRCH);
			if (u.u_uid != umem->mp->p_uid && !suser())
				return(EPERM);
			psignal(umem->mp, SIGKILL);
			break;

#ifdef hp9000s300		/*  XXX should this stuff be in here?  */
		case MM_RFPREGS:
			dragon_read_ureg(umem->mp, data);
			break;
			
		case MM_WFPREGS:
			dragon_write_ureg(umem->mp, data);
			break;

		case MM_SINGLE_STEP:
			mm_single(umem);
			/*  fall through  */
#endif			
		case MM_CONTIN:
			if (*data >= NUSERSIG)
				return(EINVAL);
			umem->mp->p_cursig = *data;
			x = spl6();
			SPINLOCK(sched_lock);
			setrun(umem->mp);
			SPINUNLOCK(sched_lock);
			splx(x);
			break;

		case MM_STOP:
			u.u_procp->p_flag2 |= SADOPTIVE;
			umem->mp->p_dptr = u.u_procp;
			umem->mp->p_flag |= STRC;
			mm_stop(umem);
			break;
						
		default:
			return EINVAL;
	}
	return(0);
}




mm_stop(umem)
struct umem_data *umem;
{
	umem->mp->p_stat = SSTOP;
	umem->mp->p_flag &= ~SWTED;
	swtch();
	umem->fault_addr = 0;

}


#ifdef hp9000s300

mm_single(umem)
struct umem_data *umem;
{
	struct pte *pp;
	struct pregion *prp;
	struct user *uap;
	struct exception_stack *regs;	


	pp = vastopte(umem->mp->p_vas, UAREA);
	if (pp == NULL) {
		prp = findprp(umem->mp->p_vas, umem->mp->p_vas, UAREA);
		bring_in_pages(prp, UAREA, 1, 1, 0);
		pp = vastopte(umem->mp->p_vas, UAREA);
		regrele(prp->p_reg);
		if (pp == NULL)
			panic("can't find u area");
	}
	uap = (struct user *) ptob(pp->pg_pfnum);
	regs = (struct exception_stack *) uap->u_ar0;
	regs->e_PS |= PSL_T;
	uap->u_pcb.pcb_flags |= (USER_TRACE_MASK|UMEM_PTRACE_MASK);
}

#endif

/*
 *	This is not a select routine in the sense of select(2), but it
 *	is used in a somewhat similar way - the tracing process does an
 *	    ioctl(fd, MM_SELECT, &addr)
 *	and this will only return when one of the monitored ranges has
 *	been touched - the specific address will be returned in addr
 */
mm_Select(addr)
int *addr;
{
	struct umem_data *up = (struct umem_data *) u.u_fp->f_buf;
	struct addr_range *rp;

	
	while (up->mpid == up->mp->p_pid && up->mp->p_stat != SZOMB) {
		if ((up->mp->p_stat == SSTOP) &&
		    mm_findrange(up->fault_addr, up, &rp) == 1) {
			*addr = (int) up->fault_addr;		
			return(0);
		} else
			sleep(up, PZERO+2);
	}
	return(ESRCH);
}



/*
 *	mm_findrange - look for an address in a given process' list of ranges
 *		return  -1 if address is in a "traced" page but not a range
 *		         0 if not
 *			 1 if address is in a traced range
 * 		*rpp is set if we return -1 or 1
 */
mm_findrange(addr, up, rpp)
unsigned int addr;
struct umem_data *up;
struct addr_range **rpp;
{
	int i;
	int pg_addr;
	int rval;
	

	*rpp = NULL;
	rval = 0;
	pg_addr = btop(addr);
	
	for (i = 0; i < MAXRANGES; i++) {
		if (!up->ranges[i].flags)
			continue;
		if (addr >= up->ranges[i].start_addr && 
                    addr <= up->ranges[i].end_addr) {
                    	*rpp = &up->ranges[i];
			return(1);
		} else if (pg_addr >= btop(up->ranges[i].start_addr) && 
                           pg_addr <= btop(up->ranges[i].end_addr)) {
                           	*rpp = &up->ranges[i];
                           	rval = -1;
                       }
	}

	return rval;
}


#ifdef hp9000s700

#define PDLOCK(eiem, psw)       (PFAIL_DISABLE(psw),                    \
                                        SPINLOCK_USAV(pd_lock, eiem))
#define PDUNLOCK(eiem, psw)     (SPINUNLOCK_USAV(pd_lock, eiem),        \
                                        PFAIL_ENABLE(psw))
#define XPDE_TO_LPDE(x)   ((struct hpde *)((u_int)(x) & ~0x1F))
#define IS_ODD_OFFSET(x) ((u_int)(x) & PDE_SHADOW_BOFFSET)


/*
 * Load User Space Id of Address for a given process, "p".
 */
space_t
mm_ldusid(p, addr)
	struct proc *p;
	caddr_t addr;
{
	preg_t *prp = findpreg(p->p_vas, (space_t)-1, addr, FPRP_MINUS1);

	if (prp)
	    return prp->p_space;
	return -1;
}



/*      		(from vm_pdir.c)
 * Convert an external pde pointer to the virtual address that the pde maps.
 * The xpde encodes the 5 virtual page bits not reflected in the pde
 * entry itself in the xpde's own low 5 bits.  We extract everything but
 * the low 5 bits and use that as a word-pde pointer, get the 15 bits of
 * virtual address from the pde entry itself, then OR in the low 5 bits 
 * of the pde to get the full 20 bits of virtual page information.
 */     

#define XPDE_TO_VADDR_ODD(xpde)                                              \
      ((((((struct whpde *)(((u_int)(xpde)) & (~0x1F)))->wpde_vaddr_o)       \
                  & 0x7FFF0000) << 1) | ((((u_int)xpde) & 0x1F) << PGSHIFT))
                  
#define XPDE_TO_VADDR_EVEN(xpde)                                             \
      ((((((struct whpde *)(((u_int)(xpde)) & (~0x1F)))->wpde_vaddr_e)       \
                  & 0x7FFF0000) << 1) | ((((u_int)xpde) & 0x1F) << PGSHIFT))
                                                     


int mm_ro_write = 0;

mm_set_range(rp, umem)
struct addr_range *rp;
struct umem_data *umem;
{
	int i, bytes, sid, pfn, index;
	register struct hpde *pde;
	register u_int context, psw;
	register hpde_handle_t xpde;
	preg_t *prp;
	reg_t *regp;
	vfd_t *vfd;
	

	bytes = rp->end_addr - rp->start_addr +1;
	sid = mm_ldusid(umem->mp, rp->start_addr);

        /*
         * Ensure that source page is correctly mapped at the desired
         * address, and set appropriate bits depending on what the
         * tracer requested.
         */
        for (i = 0; i < bytes; i += NBPG) {
		int error;

		prp = findpreg(umem->mp->p_vas, (space_t)-1, 
					rp->start_addr + i, FPRP_MINUS1);
                PDLOCK(context, psw);
		xpde = vtopde(sid, rp->start_addr + i);
		pde = XPDE_TO_LPDE(xpde);
                PDUNLOCK(context, psw);

                regp = prp->p_reg;
                reglock(regp);
                index = regindx(prp, rp->start_addr+i);
                vfd = FINDVFD(regp, (int)index);
		

		if (u.u_procp->p_pid != umem->mpid && !vfd->pgm.pg_lock) {
                        error = bring_in_pages(prp, rp->start_addr+i, 1, 1, 1);
                        if (error)
                                printf("bring_in_pages failed: error = %d\n", error);
                }
                regrele(regp);
                PDLOCK(context, psw);
		xpde = vtopde(sid, rp->start_addr + i);
		pde = XPDE_TO_LPDE(xpde);

		if (IS_ODD_OFFSET(rp->start_addr + i)){
			if (!pde->pde_umem_o && !(pde->pde_ar_o & PDE_HASWRITE))
				rp->flags |= MM_WAS_RO;
			if (rp->flags & MM_MONITOR_READ) {
				pde->pde_valid_o = 0;
#ifndef HPUX_901				
				pde->pde_uncache_o = 0;	
#endif				
			}
			if (rp->flags & MM_MONITOR_WRITE) {
				pde->pde_ar_o &= ~PDE_HASWRITE;
				if (rp->flags & MM_WAS_RO)
					mm_ro_write++;
			}
			pde->pde_umem_o = 1;
			ftlbentry(pde->pde_space_o, XPDE_TO_VADDR_ODD(xpde));
		} else {
			if (!pde->pde_umem_e && !(pde->pde_ar_e & PDE_HASWRITE)) {
				rp->flags |= MM_WAS_RO;
				mm_ro_write++;
			}
			if (rp->flags & MM_MONITOR_READ) {
				pde->pde_valid_e = 0;
#ifndef HPUX_901				
				pde->pde_uncache_e = 0;
#endif				
			}
			if (rp->flags & MM_MONITOR_WRITE) {
				pde->pde_ar_e &= ~PDE_HASWRITE;
/*				if (rp->flags & MM_WAS_RO)	*/

			}
			pde->pde_umem_e = 1;
			ftlbentry(pde->pde_space_e, XPDE_TO_VADDR_EVEN(xpde));
		}
                PDUNLOCK(context, psw);			
        }
}


mm_delete_range(rp, umem)
struct addr_range *rp;
struct umem_data *umem;
{
	int i, bytes, sid, pfn, flags;
	register struct hpde *pde;
	register u_int context, psw;
	register hpde_handle_t xpde;
	struct addr_range *rp2;
	

	bytes = rp->end_addr - rp->start_addr +1;
	sid = mm_ldusid(umem->mp, rp->start_addr);

	flags = rp->flags;
	rp->flags = 0;
	
        for (i = 0; i < bytes; i += NBPG) {
		if (mm_findrange(rp->start_addr + i, umem, &rp2))
			continue;
                PDLOCK(context, psw);
		xpde = vtopde(sid, rp->start_addr + i);
		pde = XPDE_TO_LPDE(xpde);
		if (IS_ODD_OFFSET(rp->start_addr + i)){
			if (!(flags & MM_WAS_RO))
				pde->pde_ar_o |= PDE_HASWRITE;
			/*  should restore cachability, validity?  XXX  */
			pde->pde_umem_o = 0;
		} else {
			if (!(flags & MM_WAS_RO))
				pde->pde_ar_e |= PDE_HASWRITE;
			/*  should restore cachability, validity?  XXX  */
			pde->pde_umem_e = 0;
		}
                PDUNLOCK(context, psw);			
		fcacheall();		/*  XXX  ftlbentry?  */
        }
}

int mm_ttrap = 0;
int mm_mtrap = 0;

char *last_addrs[65536];


mm_trace_trap(ssp)
struct save_state *ssp;
{
mm_ttrap++;
	ssp->ss_ipsw &= ~PSW_R;
	if (u.u_pcb.pcb_flags & UMEM_TRACE_MASK) {
                int rval;
		struct addr_range *rp;
		struct umem_data *up =
			(struct umem_data *) u.u_procp->p_cttyfp;
mm_mtrap++;
last_addrs[mm_mtrap & 0xffff] = up->fault_addr;

		rval = mm_findrange(up->fault_addr, up, &rp);
		if (!rp)
			panic("mm_trace_trap: no range pointer");
		up->faults++;
		mm_set_range(rp, up);		

		u.u_pcb.pcb_flags &= ~UMEM_TRACE_MASK;
		if (rval > 0 && (rp->flags & MM_TRAP_ON_ACCESS)) {
			rp->count++;
#ifdef notdef
			if (pcoq != up->pc)
				printf("pcoq = 0x%x, up->pc = 0x%x\n",
					pcoq, up->pc);
#endif
			if (up->sig)
				psignal(up->tp, up->sig);
			wakeup(up);
			if (u.u_procp->p_flag & STRC)			
#ifdef hp9000s300			
				return(0);	/*  let debugger handle  */
#else
				psignal(u.u_procp, SIGTRAP);
#endif				
			else
				mm_stop(up);	
		}
		return MM_IGNORE;
	}
	return 0;
}

int mm_busserr_o = 0;
int mm_busserr_e = 0;
int mm_buserr = 0;
int recov_ctr_value = 0;



mm_buserror(ssp)
struct save_state *ssp;
{
	register hpde_handle_t xpde;
	struct hpde *pde;
	register unsigned int context, psw;
	register struct umem_data *up;


mm_buserr++;
	if ((up = (struct umem_data *) u.u_procp->p_cttyfp) == NULL)
		return(0);	
        PDLOCK(context, psw);
	xpde = vtopde(isr, ior);
	pde = XPDE_TO_LPDE(xpde);

	if ((IS_ODD_OFFSET(ior) && pde && pde->pde_umem_o) ||
	    (!IS_ODD_OFFSET(ior) && pde && pde->pde_umem_e)) {
		/* 
		 *  arrange for a recovery counter trap on the next 
		 *  instruction and give the process temporary
		 *  write access to the page (it will be taken away
		 *  again by mm_trace_trap() when the recovery counter
		 *  goes negative and we trap)
		 */
		ssp->ss_ipsw |= PSW_R;	
		ssp->ss_cr0 = recov_ctr_value;

		u.u_pcb.pcb_flags |= UMEM_TRACE_MASK;
		up->fault_addr = (u_int) ior;
		up->pc = pcoq;

		if (IS_ODD_OFFSET(ior)) {
			pde->pde_ar_o |= PDE_HASWRITE;
			pde->pde_valid_o = 1;
			ftlbentry(pde->pde_space_o, XPDE_TO_VADDR_ODD(xpde));
			mm_busserr_o++;
		} else {
			pde->pde_ar_e |= PDE_HASWRITE;
			pde->pde_valid_e = 1;
			ftlbentry(pde->pde_space_e, XPDE_TO_VADDR_EVEN(xpde));
			mm_busserr_e++;
		}
		PDUNLOCK(context, psw);
		return MM_IGNORE;
	}

	PDUNLOCK(context, psw);
	return 0;
}




umem_copyout(sid, to, size, cmd)
space_t sid;
unsigned long to;
int size;
int cmd;
{
	register hpde_handle_t xpde;
	struct hpde *pde;
	register unsigned int context, psw;


        PDLOCK(context, psw);

	while (size > -NBPG) {
		xpde = vtopde(sid, to);
		pde = XPDE_TO_LPDE(xpde);

		if ((IS_ODD_OFFSET(to) && pde && pde->pde_umem_o) ||
		    (!IS_ODD_OFFSET(to) && pde && pde->pde_umem_e)) {

			if (IS_ODD_OFFSET(to)) {
				if (cmd == 0)
					pde->pde_ar_o |= PDE_HASWRITE;
				else
					pde->pde_ar_o &= ~PDE_HASWRITE;
				ftlbentry(pde->pde_space_o, XPDE_TO_VADDR_ODD(xpde));
			} else {
				if (cmd == 0)
					pde->pde_ar_e |= PDE_HASWRITE;
				else
					pde->pde_ar_e &= ~PDE_HASWRITE;
				ftlbentry(pde->pde_space_e, XPDE_TO_VADDR_EVEN(xpde));
			}
		}
		size -= NBPG;
		to += NBPG;
	}

	PDUNLOCK(context, psw);
	return 0;
}


#else


mm_set_range(rp, umem)
struct addr_range *rp;
struct umem_data *umem;
{
	unsigned int addr;
	int i, npages;
	struct pte *pte;
	struct pregion *prp;
	

	npages = btop(rp->end_addr) - btop(rp->start_addr) +1;
	addr = rp->start_addr & 0xfffff000;	/*  start on page boundary  */
	
	for (i = 0; i < npages; i++) {
		pte = vastopte(umem->mp->p_vas, addr);
		if (pte == NULL || (pte->pg_v == 0 && pte->pg_umem == 0)) {
			prp = findprp(umem->mp->p_vas, umem->mp->p_vas, addr);
			if (prp == NULL) {
				return EFAULT;
			}

			/* bring_in_pages(prp,vaddr,count,break_cow,wire_down)*/
			bring_in_pages(prp,addr,1,1,0);	  /*  XXX  0  */
			pte = vastopte(umem->mp->p_vas, addr);
			regrele(prp->p_reg);	/*  XXX  */
			if (pte == NULL) 
				panic("couldn't bring in pages");
		}

		if (pte->pg_prot && !pte->pg_umem)
			rp->flags |= MM_WAS_RO;
		if (rp->flags & MM_MONITOR_READ) {
			pte->pg_v = 0;
			pte->pg_cm = 0;
			pte->pg_ci = 1;
		}
		if (rp->flags & MM_MONITOR_WRITE) {
			pte->pg_prot = 1;
			pte->pg_cm = 0;
		}
		pte->pg_umem = 1;
		purge_icache();
		purge_dcache_u();
		purge_tlb_user();
		addr += NBPG;
	}
}



mm_delete_range(rp, umem)
struct addr_range *rp;
struct umem_data *umem;
{
	unsigned int addr;
	int i, npages, flags;
	struct pte *pte;
	struct addr_range *rp2;
	

	npages = btop(rp->end_addr) - btop(rp->start_addr) +1;
	addr = rp->start_addr;
	flags = rp->flags;
	rp->flags = 0;
	
	for (i = 0; i < npages; i++, addr += NBPG) {
		if (mm_findrange(addr, umem, &rp2))
			continue;
		pte = vastopte(umem->mp->p_vas, addr);
		if (pte == NULL)
			continue;

		pte->pg_umem = 0;
		pte->pg_v = 1;		/*  XXX  */
		if (!(flags & MM_WAS_RO))
			pte->pg_prot = 0;
		purge_icache();
		purge_dcache();
		purge_dcache_physical();
		purge_tlb();
	}
}



mm_trace_trap()
{
        int rval;
	struct addr_range *rp;
	struct umem_data *up = (struct umem_data *) u.u_procp->p_cttyfp;
	

	if (u.u_pcb.pcb_flags & UMEM_TRACE_MASK) {
		rval = mm_findrange(up->fault_addr, up, &rp);
		if (!rp)
			panic("mm_trace_trap: no range pointer");
		up->faults++;
		mm_set_range(rp, up);

		u.u_pcb.pcb_flags &= ~UMEM_TRACE_MASK;
		if (rval > 0 && (rp->flags & MM_TRAP_ON_ACCESS)) {
			rp->count++;
			if (up->sig)
				psignal(up->tp, up->sig);
			wakeup(up);
			if (u.u_procp->p_flag & STRC)			
				return(0);	/*  let debugger handle  */
			else
				mm_stop(up);	
		}
		return MM_IGNORE;
	} 

	if (u.u_pcb.pcb_flags & UMEM_PTRACE_MASK) {
		u.u_pcb.pcb_flags &= ~UMEM_PTRACE_MASK;
		mm_stop(up);	
		return MM_IGNORE;			
	}

	return 0;
}



mm_buserror(locregs, vas, addr)
struct exception_stack *locregs;
vas_t *vas;
caddr_t addr;
{
	register struct pte *pt;


        pt = vastopte(vas, addr);
        if (pt && pt->pg_umem) {
		int rval;
                struct addr_range *rp;
                register struct umem_data *up;
                

               	up = (struct umem_data *) u.u_procp->p_cttyfp;
                rval = mm_findrange(addr, up, &rp);

		/* arrange for a trace trap on the next instruction */
		locregs->e_PS |= PSL_T;
		u.u_pcb.pcb_flags |= (USER_TRACE_MASK | UMEM_TRACE_MASK);

		up->fault_addr = (u_int) addr;
		up->pc = locregs->e_PC;

/*  XXX  we are letting anybody write here, even if it was supposed to be RO  */
		if (rval && pt->pg_prot) {
			pt->pg_prot = 0;
			purge_tlb_user();
			return MM_IGNORE;
		}
		if (rval && !pt->pg_v)  {
			pt->pg_v = 1;
			purge_tlb_user();
			return MM_IGNORE;
		}
        }
	return 0;
}



umem_bus_return(locregs, vas, addr)
struct exception_stack *locregs;
vas_t *vas;
caddr_t addr;
{
	register struct pte *pt;
        register struct umem_data *up;
	struct addr_range *rp;


        up = (struct umem_data *) u.u_procp->p_cttyfp;
        if (mm_findrange(addr, up, &rp)) {
                pt = vastopte(vas, addr);
                pt->pg_umem = 1;
                mm_buserror(locregs, vas, addr);
        }
}


#endif

