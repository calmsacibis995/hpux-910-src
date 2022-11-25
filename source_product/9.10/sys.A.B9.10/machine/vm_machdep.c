/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/vm_machdep.c,v $
 * $Revision: 1.9.84.7 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/11/03 16:06:52 $
 */

/* HPUX_ID: @(#)vm_machdep.c	55.1		88/12/23 */

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

#include "../h/debug.h"
#include "../s200/pte.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../nfs/nfs.h"
#include "../h/mount.h"
#include "../h/vm.h"
#include "../h/pfdat.h"
#include "../h/pregion.h"
#include "../h/uio.h"
#include "../h/sar.h"
#include "../h/rtprio.h"
#include "../h/cache.h"

extern int maxssiz;
extern int maxdsiz;
extern int maxtsiz;

extern vas_t kernvas;

#ifdef notdef
/*ARGSUSED*/
mapout(pte, size)
	register struct pte *pte;
	int size;
{

	panic("mapout");
}
#endif

#ifdef NEVER_CALLED
/*
 * Check for valid program size
 *  XXX Andy  I'm not sure this needs to exist at all any more
 */
chksize(ts, ds, ss, chkshm)
	unsigned ts, ds, ss, chkshm;
{
	if (ts > maxtsiz || ds > maxdsiz || ss > maxssiz) {
		u.u_error = ENOMEM;
		return (1);
	}

	return (0);
}

newptes(v, size)
	u_int v;
	register int size;
{
	register caddr_t a = (caddr_t)ptob(v);
	if (size >= 8) {
		purge_tlb_user();
		return;
	}
	while (size-- > 0) {
		purge_tlb_select_user(a);
		a += NBPG;
	}
}
#endif /* NEVER_CALLED */

/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLSIZE.
 */
pagemove(from, to, size)
	register caddr_t from, to;
	int size;
{
	register struct pte *fpte, *tpte;
#ifdef PFDAT32
	struct pfdat *pfd;
#endif	

	if (size % NBPG)
		panic("pagemove: bad size");
	while (size > 0) {
		fpte = vastopte(&kernvas, from);
		tpte = vastopte(&kernvas, to);
#ifdef PFDAT32
		pfd = pfdat + hdl_vpfn(KERNELSPACE, from);
		if (pfd->pf_hdl.pf_kpp == fpte)
			pfd->pf_hdl.pf_kpp = tpte;
		else
			panic("pagemove");
#endif
		from += NBPG;
		to += NBPG;
		*tpte = *fpte;
		*(int *)fpte = INVALID;
		size -= NBPG;
	}
	purge_tlb_super();
}


#ifdef REQ_MERGING

/*
 * Copy pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLSIZE.
 */

pagecopy(from, to, size)
	register caddr_t from, to;
	int size;
{
	register struct pte *fpte, *tpte;
	unsigned int fake_pte;

	while (size > 0) {
		if (from >= (caddr_t)tt_region_addr) {
			fake_pte = ((unsigned int)from & 0xfffff000) | 1;
			fpte = (struct pte *) &fake_pte;
		} else
			fpte = vastopte(&kernvas, from);
		tpte = (struct pte *) vm_alloc_tables(KERNVAS, to);
		from += NBPG;
		to += NBPG;
		*tpte = *fpte;
		size -= NBPG;
	}
	purge_tlb_super();
	purge_dcache_s();
}


/*
 * Remove mappings for pages that live in Sysmap
 * size must be a multiple of CLSIZE.
 */
pagerm(addr, size)
register caddr_t addr;
int size;
{
	register struct pte *tpte;

	while (size > 0) {
		tpte = vastopte(&kernvas, addr);
		addr += NBPG;
		*(int *)tpte = INVALID;
		size -= NBPG;
	}
	purge_tlb_super();
}

#endif

int kernacc_sendmsg = 1;

/*
** kernacc -- check a range of kernel logical addresses
**	      for accessibility.  All pte's in the range
**	      must be valid and if writing and the write
**	      protect bit is set then remove write protection.
*/
int
kernacc(vaddr, length, mode)
    register caddr_t vaddr;
    int length;
    int mode;
{
	register caddr_t limit = vaddr+length;
	register struct pte *pte;
	register int writing = (mode == B_WRITE);
	caddr_t startaddr = vaddr;
	int modified_ptes = 0;

	while (vaddr < limit) {
		if (vaddr < (caddr_t)tt_region_addr) {
			if ((pte = vastopte(&kernvas, vaddr)) == NULL)
				return(UIO_NOACCESS);
			if (!pte->pg_v)
				return(UIO_NOACCESS);
			if (writing && (pte->pg_prot)) {
				pte->pg_prot = 0;
				modified_ptes = 1;
				if (kernacc_sendmsg) {
					msg_printf("User has written to kernel text\n");
					kernacc_sendmsg = 0;
				}
			}
		}
		if ((vaddr += NBPG) < startaddr)
			break;
	}
	if (modified_ptes) {
		/* Flush the caches */
		purge_dcache();
		purge_tlb();
		return(UIO_KERNWRITE);
	}
	return(UIO_KERNACCESS);
}

/*
** kernprot -- write protect a range of kernel logical addresses
*/
int
kernprot(vaddr, length)
    register caddr_t vaddr;
    int length;
{
	register caddr_t limit = vaddr+length;
	register struct pte *pte;
	caddr_t startaddr = vaddr;

	while (vaddr < limit) {
		if (vaddr < (caddr_t)tt_region_addr) {
			pte = vastopte(&kernvas, vaddr);
			VASSERT(pte != NULL);
			VASSERT(pte->pg_v);
			pte->pg_prot = 1;
		}
		if ((vaddr += NBPG) < startaddr)
			break;
	}
	/* Flush the caches */
	purge_dcache();
	purge_tlb();;
}
/*
 * Tell if the user may access a given range of virtual addresses
 */
useracc(base, length, mode)
    register int base;
    int length;
    register int mode;
{
    register caddr_t start, end;
    vas_t *vas = u.u_procp->p_vas;
    register preg_t *prp;

    if (u.u_pcb.pcb_flags & MULTIPLE_MAP_MASK)
	if ((base & 0xf0000000) != 0xf0000000)
	    base &= 0x0fffffff;

    start = (caddr_t)pagedown(base);
    prp = searchprp(vas, (space_t)vas, start);
    if (prp == (preg_t *)0)
	return(0);

    end = (caddr_t)pageup(base + length);
    if (end > prp->p_vaddr + ptob(prp->p_count))
	return(0);

    if (!IS_MPROTECTED(prp)) {
	/*
	 * This pregion has not been mprotect()ed, just look
	 * at the pregion protection flags.
	 */
	if (mode == B_WRITE && !(prp->p_prot & PROT_WRITE))
	    return(0);

	return(1);
    }
    else {
	/*
	 * This pregion has been mprotect()ed, we must look
	 * at the protection mode for all of the pages in
	 * the range [base, base + length).
	 */
	while (start < end) {
	    u_long mprot = hdl_page_mprot(prp, start);

	    switch (mprot) {
	    case MPROT_UNMAPPED:
	    case MPROT_NONE:
		/*
		 * Page is either marked PROT_NONE or has been unmapped.
		 * Thus, the user cannot access this page.
		 */
		return(0);
	    
	    case MPROT_RO:
		/*
		 * A read-only page, if they want to write to it, this
		 * is an error, otherwise they can access the page.
		 */
		if (mode == B_WRITE)
		    return(0);
		break;
	    
	    case MPROT_RW:
		/*
		 * This page is read/write, they can do whatever they
		 * want to it.
		 */
		break;
	    }
	    start += NBPG;
	}
	return(1);
    }
    /* NOTREACHED */
}

#ifdef __hp9000s800
fdcache( space, addr, count )
    int space, addr, count;
{
    if( space == KERNELSPACE )
	purge_dcache_s();
    else
	purge_dcache_u();
}
#endif /* __hp9000s800 */


#ifdef NEVER_CALLED
/*
 * Validate the kernel map for size ptes which
 * start at ppte in the sysmap, and which map
 * kernel virtual addresses starting with vaddr.
 */
vmaccess(ppte, vaddr, size)
	register struct pte *ppte;
	register caddr_t vaddr;
	register int size;
{
	register int temp = 0;
	while (size--) {
		ppte->pg_v = PG_V;
		ppte->pg_prot = PG_RW;
		*(int *)ppte |= PG_REF;
		purge_tlb_select(vaddr);
		ppte++;
		vaddr += NBPG;
	}


}
#endif /* NEVER_CALLED */

    /* Simulate sys_memall and sys_memfree for the 300 drivers */
caddr_t
sys_memall(bytes)
    int bytes;
{
    extern caddr_t kdalloc();

    return( kdalloc(btorp(bytes)) );
}
sys_memfree(p, bytes)
    caddr_t p;
    int bytes;
{
    kdfree(p, btorp(bytes));
}

#ifndef WSIO
display_activity(nrun)
{
}
#endif /* WSIO  */
/*
 * Raw I/O. The arguments are
 *	The strategy routine for the device
 *	A buffer
 *	The device number
 *	Read/write flag
 * There are two important, different paths through physio:
 *	- Kernel addresses, in which case we pretty much just let the
 *		I/O happen.
 *	- User addresses, in which case we must fault in the memory, wire
 *		down the pages, and create a temporary kernel translation
 *		for the duration of the I/O.  Of course, we have to undo
 *		all this when the I/O's over, too.
 */
physio(strat, bp, dev, rw, mincnt, uio)
	int (*strat)();
	struct buf *bp;
	dev_t dev;
	int rw;
	unsigned (*mincnt)();
	struct uio *uio;
{
	struct iovec *iov;
	int c, s, error = 0, ccount;
	char *a;
	struct buf *save_bp = bp;
	preg_t *prp;
	space_t kspace;
	caddr_t kvaddr;
	struct proc *p = u.u_procp;
	extern struct buf *bswalloc();

	/* if caller did not have buf >> allocate one for him */
	if (bp == NULL)
		bp = bswalloc();

nextiov:
	iov = uio->uio_iov;
	if (uio->uio_iovcnt == 0)
		goto physio_exit;

	/*  The uio data may be in kernel space, so don't check access if so */
	if (uio->uio_seg != UIOSEG_KERNEL)
		if (useracc(iov->iov_base, (u_int)iov->iov_len,
				(rw == B_READ) ? B_WRITE : B_READ) == NULL) {
			error = EFAULT;
			goto physio_exit;
		}

	syswait.physio++;

	s = spl6();
	while (bp->b_flags&B_BUSY) {
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO+1);
	}
	splx(s);

	bp->b_error = 0;
	bp->b_proc = p;
	bp->b_un.b_addr = iov->iov_base;
#ifdef FSD_KI
	/*
	 * Set the b_apid/b_upid fields to the pid (this process' pid)
	 * that last allocated/used this buffer.
	 */
	bp->b_apid = bp->b_upid = u.u_procp->p_pid;	

 	/* Identify this buffer for KI */
	bp->b_bptype = B_physio|B_phybf;

 	/* Save site(cnode) that last used this buffer */
	bp->b_site = u.u_procp->p_faddr; 
#endif /* FSD_KI */
	bsetprio(bp);
	while (iov->iov_len > 0) {
		/*
		** preserve B_SCRACH bits if bp is passed in by caller --
		** don't want B_BUSY to get cleared (even momentarily)
		** in case of interrupt
		*/
		bp->b_flags |= B_BUSY | B_PHYS | rw;
		bp->b_flags &= B_SCRACH1|B_SCRACH2|B_SCRACH3|B_SCRACH4
				|B_SCRACH5|B_SCRACH6|B_BUSY|B_PHYS|rw;
		bp->b_dev = dev;
		bp->b_blkno = btodb(uio->uio_offset);
		bp->b_offset = uio->uio_offset;
		bp->b_bcount = iov->iov_len;
		(*mincnt)(bp);
		c = bp->b_bcount;

		if(uio->uio_seg == UIOSEG_KERNEL) {
			/*
			 * If the uio data is in kernel space,
			 * don't go through the mapping
			 */
			bp->b_un.b_addr = iov->iov_base;
		} else {
			/*
			 * Otherwise create a temporary mapping in
			 * KERNELSPACE
			 */

			vas_t *vas = p->p_vas;

			/*	
			 * Pick up the prp pointer here because vslock will 
			 * get pfdat locks and doing the searchprp holding
			 * pfdat locks could deadlock us when searchprp tries
			 * to get the vas lock.
			 */
			a = bp->b_un.b_addr;
			prp = searchprp(vas, (space_t)vas, a);
			if (prp == NULL) {
				error = EFAULT;
				syswait.physio--;
				goto physio_exit;
			}

			/* Lock down the user pages */
			p->p_flag |= SPHYSIO;
			if (!vslock(a, c, &prp)) {
				p->p_flag &= ~SPHYSIO;
				error = EFAULT;
				syswait.physio--;
				goto physio_exit;
			}

			/* Map the user pages into kernel address space */
			ccount = pagesinrange(a, c);
			hdl_user_protect(prp, prp->p_space,
					(uint)a & ~POFFMASK, ccount,
				&kspace, &kvaddr, 0);

			VASSERT(kspace == KERNELSPACE);

			/* Fill in buf header w. kernel address generated */
			bp->b_spaddr = kspace;
			bp->b_un.b_addr = kvaddr + poff(a);
		}

		/* It is now definitely in KERNELSPACE */
		bp->b_spaddr = KERNELSPACE;

		/* Do the I/O */
		physstrat(bp, strat, PRIBIO);

		(void) spl6();
		if(uio->uio_seg != UIOSEG_KERNEL) {

			/*
			 * Clean up temporary kernel translation if
			 * it wasn't in KERNELSPACE
			 */
			hdl_user_unprotect(kspace, kvaddr, ccount, 0);

			/* Unlock user pages */
			vsunlock(a, c, rw, prp);
			p->p_flag &= ~SPHYSIO;

			/* Restore user address to buf header */
			bp->b_un.b_addr = a;
		}

		/* Wake up any waiters */
		if (bp->b_flags & B_WANTED)
			wakeup((caddr_t)bp);
		splx(s);

		/* Update I/O info */
		c -= bp->b_resid;
		bp->b_un.b_addr += c;
		iov->iov_len -= c;
		uio->uio_resid -= c;
		uio->uio_offset += c;

		/* temp kludge for tape drives */
		if (bp->b_resid || (bp->b_flags & (B_ERROR | B_END_OF_DATA)))
			break;
	}
	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS);
	error = geterror(bp);

	syswait.physio--;

	/* temp kludge for tape drives */
	if (bp->b_resid || error || bp->b_flags & B_END_OF_DATA)
		goto physio_exit;

	uio->uio_iov++;
	uio->uio_iovcnt--;
	goto nextiov;

physio_exit:
	/* if we allocated buf for caller, then deallocate it */
	if (save_bp == NULL)
		bswfree(bp);

	return(error);
}

#ifdef NEVER_CALLED
how_many_processors()
{
    return(1);/*Always uniprocessor, on S300, at least so far...*/
}
#endif /* NEVER_CALLED */


transfer_stack(stackptr)
    unsigned int stackptr;
{
    register int i;
    register int regidx;
    register int nstackpages;
    register struct proc *p = u.u_procp;
    register preg_t *prp = p->p_upreg;
    vfd_t *vfd;
    dbd_t *dbd;
    struct pte *pte;
    unsigned int pfnum;
    static int entered = 0;

    /*
     * We are relying on the fact that hdl_addtrans() should not
     * sleep, due to the fact that all of the page tables should
     * already be allocated for the translation. But, just in case.
     * we make sure by setting a static flag.

    if (entered)
	panic("transfer_stack reentered");

    entered++;

    /* Compute number of stack pages used */

    nstackpages = btorp(KSTACKADDR - stackptr);
    VASSERT(nstackpages > p->p_stackpages);

    /*
     * If we are on last stack page, we panic. It
     * should be impossible for nstack_pages to be
     * greater than KSTACK_PAGES, but just in case,
     * we test for >=.
     */

    if (nstackpages >= KSTACK_PAGES)
	   panic("kernel stack overflow");

#ifdef OSDEBUG
    msg_printf("Increasing stack pages from %d to %d for pid %d\n",
	    p->p_stackpages,nstackpages,p->p_pid);
#endif

    /* give common stack page(s) to user process */

    for (i = p->p_stackpages + 1; i <= nstackpages; i++) {

	pte = (struct pte *)vastopte(&kernvas,(caddr_t)(KSTACKADDR - i * NBPG));
	pfnum = pfntohil(pte->pg_pfnum);
	FINDENTRY(prp->p_reg, prp->p_off + KSTACK_PAGES - i, &vfd, &dbd);
	VASSERT(vfd->pgm.pg_v == 0);
	hdl_addtrans(prp,prp->p_space,prp->p_vaddr + ptob(KSTACK_PAGES - i),
		     vfd->pgm.pg_cw,pfnum);

	vfd->pgm.pg_v = 1;
	vfd->pgm.pg_pfn = pfnum;
	pfnunlock(pfnum);
	prp->p_reg->r_nvalid++;
	dbd->dbd_type = DBD_NONE; dbd->dbd_data = 0xfffff03;
    }

    p->p_stackpages = nstackpages;
    entered = 0;
    return(nstackpages);
}

extern int kstack_reserve;
extern unsigned int kstack_rptes[];

stack_reserve_alloc()
{
    unsigned int addr;
    label_t lqsave;
    int *save_sswap;
    static int entered = 0;

    if (entered)
	return;

    entered++;

    /*
     * alloc_page(), which is called below, may sleep. Since we are
     * called at the end of resume, it is possible that we were
     * returning to sleep, i.e. if alloc_page() calls sleep it may
     * be recursive. This is ok since we have already been woken up,
     * however, we should trap longjmp if alloc_page ever sleeps
     * at an interruptable priority. We don't know what the upper
     * level sleep was sleeping at, so it might not be appropriate
     * to longjmp back to syscall. We just restore u_qsave and
     * return, leaving the stack reserve low (the next caller
     * of resume will try again).
     *
     * We also save u.u_pcb.pcb_sswap so that a forked child will
     * not longjump out of here leaving the entered flag set.
     */

    lqsave = u.u_qsave;
    save_sswap = u.u_pcb.pcb_sswap;
    u.u_pcb.pcb_sswap = 0;
    if (setjmp(&u.u_qsave)) {
	u.u_qsave = lqsave;
	u.u_pcb.pcb_sswap = save_sswap;
	entered = 0;
	return;
    }

    /* Allocate reserve stack pages.
     *
     *      Note: Since alloc_page can sleep, we can't use
     *      a loop variable here, since kstack_reserve can
     *      change value during the sleep.
     */

    while (kstack_reserve < KSTACK_RESERVE) {
	addr = alloc_page(DONT_ZERO_MEM);
	kstack_rptes[kstack_reserve++] = addr | PG_RW | PG_CB | PG_V;
    }

    u.u_qsave = lqsave;
    u.u_pcb.pcb_sswap = save_sswap;
    entered = 0;
    return;
}
