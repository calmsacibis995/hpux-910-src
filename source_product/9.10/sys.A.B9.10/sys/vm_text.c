/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_text.c,v $
 * $Revision: 1.50.83.5 $       $Author: marshall $
 * $State: Exp $        $Locker:  $
 * $Date: 93/12/15 12:19:43 $
 */

#include "../h/types.h"
#include "../h/sema.h"
#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/errno.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/vas.h"
#include "../h/debug.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../h/pregion.h"
#include "../h/file.h"
#include "../ufs/inode.h"
#include "../h/pfdat.h"
#include "../dux/cct.h"
#include "../dux/sitemap.h"
#include "../dux/dux_hooks.h"	/* to stub out dux-only calls */
extern site_t my_site;
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs_hooks.h"

#if NBPG == 4096
#define TRAPNULLREF_CASE	(-1)
#define UNDEFINED_CASE		0
#define ZEROFILL_CASE		1
#endif

void fault_each();

/*
 * Add a text object
 */
int
add_text(vp, off, len, addr)
	struct vnode *vp;
	int off;
	int len;
	int addr;
{
	reg_t *rp;
	preg_t *prp;
	
	/*
	 * If text is page aligned then use a memory mapped concept
	 * where the pregion is based on the vnode's region;
	 * otherwise, create a region and use the buffer cache to
	 * get the data.
	 */
	if (poff(off) == 0) {
		/*
		 * Page aligned.
		 */
		if ((rp = vnodereg(vp)) == NULL)
			panic("add_text: aligned and vnode not mapped");
		reglock(rp);
		/*
		 * The pregion hanging off the vnode is already on the
		 * active chain, so mark this guy PF_NOPAGE to keep the
		 * region from getting scanned twice in vhand.
		 */
		if ((prp = attachreg(u.u_procp->p_vas, rp,
				     PROT_URX, PT_TEXT, 
				     PF_EXACT | PF_NOPAGE, (caddr_t)addr,
				     btop(off), btorp(len))) == NULL) {
			regrele(rp);
			return(1);
		}
		hdl_procattach(&u, u.u_procp->p_vas, prp);
		regrele(rp);
		return(0);
	} else {
		/* 
		 * Not page aligned.
		 */
		if (text_insert(vp, (u_int)off, (reg_t *)NULL, &rp) != 0) {
			/*
			 * Either a region is currently in the hash
			 * but was locked, or none was there and 
			 * this is a fresh start.
			 */
			reg_t *newrp;

			if ((rp = allocreg( (struct vnode *) vp, swapdev_vp,
							     RT_SHARED))==NULL)
				return(1);
			
			if (growreg(rp, (int)btorp(len), DBD_FSTORE) < 0) {
				freereg(rp);
				return(1);
			}

			/*
			 * Set byte offset for this region
			 */
			rp->r_flags |= RF_UNALIGNED;
			rp->r_byte = off;
			rp->r_bytelen = len;

			/*
			 * Attempt to insert the new region, either this
			 * works (in which case everyone will share this
			 * new region) or it does not in which case me
			 * an my descendants will share this region.
			 *
			 * Note: rp is used below we don't care about
			 * the value of newrp!
			 */
			(void)text_insert(vp, (u_int)off, rp, &newrp);
		}
		/*
		 * We now have a region that is set to pagein data 
		 * through the buffer cache.  This region may (or may not)
		 * be hashed.  
		 */
		VASSERT(rp);
		VASSERT(vm_valusema(&rp->r_lock) <= 0);
		VASSERT(rp->r_flags&RF_UNALIGNED);
		VASSERT(rp->r_fstore == vp);
		VASSERT(rp->r_byte == off);
		if ((prp = attachreg(u.u_procp->p_vas, rp, PROT_URX, PT_TEXT, 
				     PF_EXACT, (caddr_t)addr, 0, btorp(len)))==
									NULL) {
			freereg(rp);
			return(1);
		}
		hdl_procattach(&u, u.u_procp->p_vas, prp);
		regrele(rp);
		return(0);
	}
}

#if defined(hp9000s800) && (NBPG == 4096)  
/*
 *  add_text_2k_compat() sets up a text region for an a.out which is
 *  not 4kb-aligned and which might need special handling for page 0.
 *  Most likely the a.out was generated on an PA-RISC 1.0 system on HP-UX
 *  release 7.0 or before. 
 */

add_text_2k_compat(vp, textoff, textsize, textaddr, addnull_to_text)
	struct vnode *vp;
	u_int textoff, textsize;
	caddr_t textaddr;
	int addnull_to_text;
{
	preg_t *prp;

	switch (addnull_to_text) {
	case ZEROFILL_CASE:
		/*
		 * Add a text segment with the first
		 * 2k of the first page always zeroed.
		 */
		return(add_text_with_null(vp, textoff, textsize, textaddr));
		break;	/* NOTREACHED */
	
	case TRAPNULLREF_CASE:
		if (add_text(vp, textoff, textsize, textaddr)){
			return(1);
		}
		/*
		 * For older executables which force the beginning of text and the
		 * null pointer address range to co-reside on
		 * the first page, flag text pregion for
		 * special treatment by PA HDL (in particular
		 * hdl_addtrans() and hdl_pfault()).
		 */
		prp = findpregtype(u.u_procp->p_vas, PT_TEXT);
		prp->p_hdl.hdlflags |= PFHDL_NONULLREFPG;
		break;
	default:
		panic("add_text_2k_compat: illegal case");
	}
	return(0);
}
#else  /* defined(hp9000s800) && NBPG == 4096 */
add_text_2k_compat()
{
	panic("add_text_2k_compat");
}
#endif /* defined(hp9000s800) && NBPG == 4096 */


#if defined(hp9000s800) && NBPG == 4096

/*
 * Add a text object with special treatment of first page to provide
 * correct null pointer semantics.  This version of add_text() always
 * creates a text segment with swap as backing store in order to
 * preserve modifications made to the first page when it is first
 * paged in from the a.out.  This version of add_text will not
 * create multiple instances of a text region in the text cache
 * from the same vnode as it is possible for this routine to
 * set up aligned pagein's and thereby utilize the page cache.
 * In such a case we wouldn't want to have one set of physical
 * pages share multiple mappings.
 */

int
add_text_with_null(vp, off, len, addr)
	struct vnode *vp;
	int off;
	int len;
	int addr;
{
	reg_t *rp;
	preg_t *prp;
	dbd_t *dbdp;
	vfd_t *vfdp;
	reg_t *newrp;

	VASSERT(addr == 0);

try_again:
	switch(text_insert(vp, off, (reg_t *)NULL, &rp)) {

	case 1:
		/*
		 * No such region, so create one.
		 */
		if ((rp = allocreg(vp, swapdev_vp, RT_SHARED))==NULL)
			return(1);
			
		if (growreg(rp, btorp(len), DBD_FSTORE) < 0) {
			freereg(rp);
			return(1);
		}

		/*
		 * Set byte offset for this region
		 */
		if (poff(off)) {
			rp->r_flags |= RF_UNALIGNED;
			rp->r_byte = off;  
			rp->r_bytelen = len;
		} else {
			VASSERT(rp->r_off == 0);
			rp->r_off = btop(off);
			/*
			 * set r_byte even though page aligned since
			 * text cache is keyed off of the byte offset.
			 */
			rp->r_byte = off;  
		}

		/*
		 * Attempt to insert the new region, either this
		 * works (in which case others will share this
		 * new region) or it does not in which case we
		 * assume someone else has created an identical
		 * region since we last checked.  If so delete
		 * the region we've created and repeat the whole
		 * process.  If the region is locked when we
		 * look for it again, then we will create our
		 * own private text; otherwise we will share
		 * it with the creator.
		 */
		if (text_insert(vp, off, rp, &newrp)){
			freereg(rp);
			goto try_again;
		}
		VASSERT(rp == newrp);
		break;
			 
	case 0:
		/*
		 * Region found and locked, so just use it.
		 */
		break;

	case -1:
		/*
		 * Someone else holds region right now, so
		 * don't bother attempting to share it; just
		 * create a private region paged in through
		 * the buffer cache.
		 */
		
		if ((rp = allocreg(vp, swapdev_vp, RT_PRIVATE))==NULL)
			return(1);
	
		if (growreg(rp, btorp(len), DBD_FSTORE) < 0) {
			freereg(rp);
			return(1);
		}

		rp->r_flags |= RF_UNALIGNED;
		rp->r_byte = off;
		rp->r_bytelen = len;
		break;

	default:
		panic("add_text_with_null: invalid case");
		
	} /* end of switch */

	VASSERT(rp);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);
	VASSERT(rp->r_byte == off);
	VASSERT(rp->r_fstore == vp);
	VASSERT(rp->r_bstore == swapdev_vp);

	if ((prp = attachreg(u.u_procp->p_vas, rp, PROT_URX, PT_TEXT, 
			     PF_EXACT, addr, 0, btorp(len)))==NULL) {
		freereg(rp);
		return(1);
	}
	hdl_procattach(&u, u.u_procp->p_vas, prp);
	
	FINDENTRY(rp, (regindx(prp, addr)), &vfdp, &dbdp);
	/*
	 * If the first page (vaddr == 0) has never been paged in before,
	 * fault it in and zero out first 2k of it.
	 */
	if ((dbdp->dbd_type == DBD_FSTORE) && (!vfdp->pgm.pg_v)) {
		make_unswappable();
                if (bring_in_pages(prp,addr,1,1,1)) {
                        make_swappable();
                        detachreg(u.u_procp->p_vas, prp);
                        return(1);
                }
#ifdef OSDEBUG
		FINDENTRY(rp, (regindx(prp, addr)), &vfdp, &dbdp);
		VASSERT(vfdp->pgm.pg_v);
#endif
		hdl_zero_page(prp->p_space, addr, NBPG_PA83);
#ifdef OSDEBUG
		/*
		 * hdl_zero_page should have set the mod bit
		 * on the page; let's make sure.
		 * vfd/dbdp's could have gone stale during virtual_fault(),
		 * so look it up again.
		 */
		pfnlock(vfdp->pgm.pg_pfn);
		VASSERT(hdl_getbits(vfdp->pgm.pg_pfnum) & VPG_MOD);
		pfnunlock(vfdp->pgm.pg_pfn);
#endif /* OSDEBUG */
	}

	regrele(rp);
	return(0);
}

#endif /* hp9000s800 && NBPG == 4096 */

/*
 * Add a null dereference page (always goes at address 0).
 */
int
add_nulldref(useg, vas)
user_t *useg;
vas_t *vas;
{
	static reg_t *globalnullrp = NULL;
	preg_t *prp;

	/*
	 * If needed we will add a null dereference page to the process.
	 * Since few process will ever need to access the page, we have one
	 * region that is grown to a single page and then attached
	 * to different processes.  The page will be thrashed for processes
	 * doing null pointer dereferences.
	 */
	if (globalnullrp) {
		reglock(globalnullrp);
	} else {
		/*
		 * We are the first process to need one -
		 * very likely, we are init.
		 * Make one.
		 */
		if ((globalnullrp = allocreg((struct vnode *)0, 
					     swapdev_vp, RT_SHARED)) == NULL) {
			panic("add_nulldref: can not allocate global null");
		}
		/*
		 * Bump count to guarantee this page stays around
		 */
		globalnullrp->r_refcnt++;
		if (growreg(globalnullrp, 1, DBD_DZERO) < 0) {
			panic("add_nulldref: can't grow nulldref");
		}
	}
	/*
	 * Send the PF_NOPAGE flag to keep vhand from paging this guy.
	 */
	if ((prp = attachreg(vas,
			     globalnullrp, PROT_URX, PT_NULLDREF,
			     PF_EXACT | PF_NOPAGE, (caddr_t)0, 0, 1)) == NULL) {
		regrele(globalnullrp);
		return(1);
	}
	if (useg) 
		hdl_procattach(useg, vas, prp);
	regrele(globalnullrp);
	return(0);
}

/*
 * Add data.
 */
int
add_data(vp, off, len, addr, file_len)
	struct vnode *vp;
	u_int off;
	u_int len;
	caddr_t addr;
	u_int file_len;
{
	register reg_t *rp;
	register reg_t *newrp;
	register preg_t *prp;

	/*
	 * If data is page aligned then use a memory mapped concept
	 * where the pregion is a copy of the vnode's pregion;
	 * otherwise, the data must be mapped private with an offset.
	 */
	if (poff(off) == 0) {
		/*
		 * Page aligned.
		 */
		if ((rp = vnodereg(vp)) == NULL)
			panic("add_data: aligned and vnode not mapped");
		reglock(rp);
		if ((newrp = dupreg(rp, RT_PRIVATE,
				    btorp(off), btorp(len))) == NULL) {
			regrele(rp);
			return(1);
		}
		regrele(rp);
		if ((prp = attachreg(u.u_procp->p_vas, newrp,
				     PROT_URW, PT_DATA, PF_EXACT,
				     addr, 0, btorp(len))) == NULL) {
			freereg(newrp);
			return(1);
		}
		hdl_procattach(&u, u.u_procp->p_vas, prp);
		/*
		 * If data does not end on a page boundary and if the a.out 
		 * extends beyond the end of initialized data make sure we 
		 * zero the unused portion of the last data page.  vfs_pagein()
		 * takes care of the case where data ends at EOF.  This partial 
		 * page could be the beginning of BSS or it could be grown into 
		 * heap, for which we guarantee zero-fill.  We don't need to do 
		 * this for unaligned regions since they are mapped to the 
		 * exact byte length and unaligned_fault() properly zeroes any 
		 * partial pages.
		 */
		if (poff(len) && (file_len > off+len)) {
			vfd_t *vfdp;
			dbd_t *dbdp;
			int pgindx = regindx(prp, addr + len);

			FINDENTRY(newrp, pgindx, &vfdp, &dbdp);
			if ((dbdp->dbd_type == DBD_FSTORE) &&
			    (!vfdp->pgm.pg_v)) {
				make_unswappable();

				if (bring_in_pages(prp,addr + 
					pagedown(len),1,1,1)) {
                                        make_swappable();
                                        detachreg(u.u_procp->p_vas, prp);
                                        return(1);
                                }

				/*
				 * Because virtual_fault can release the region
				 * lock, we technically need to reget the
				 * address of the vfd
				 */
				vfdp = FINDVFD(newrp, pgindx);
				VASSERT(vfdp->pgm.pg_v);
				VASSERT(vfdp->pgm.pg_cw == 0);

				hdl_zero_page(prp->p_space, addr + len,
					      NBPG - poff(len));

				vfdp->pgm.pg_lock = 0;
				make_swappable();
			}
		}
		regrele(newrp);
		return(0);
	} else {
		/*
		 * Not page aligned.
		 */
		if ((rp = allocreg(vp, swapdev_vp, RT_PRIVATE)) == NULL) {
			return(1);
		}
		if (growreg(rp, (int)btorp(len), DBD_FSTORE) == -1) {
			freereg(rp);
			return(1);
		}

		/* Attach view to region */
		if ((prp = attachreg(u.u_procp->p_vas, rp,
				     PROT_URW, PT_DATA, PF_EXACT,
				     addr, 0, btorp(len))) == NULL) {
			freereg(rp);
			return(1);
		}

		hdl_procattach(&u, u.u_procp->p_vas, prp);

		/*
		 * Set byte offset for this region
		 */
		rp->r_flags |= RF_UNALIGNED;
		rp->r_byte = off;
		rp->r_bytelen = len;
		regrele(rp);
		return(0);
	}
}

/*
 * Add uninitialized data part of process address space
 */
int
add_bss(len)
        u_int len;
{
        register preg_t *prp;
        register reg_t *rp;

        /*
         * Allocate bss as demand zero.  Note that we might waste
         * a page if BSS would fit in the last part of the last
         * data page.
         *
         * We attach bss to the end of the data region.
         */
        if ((prp = findpregtype(u.u_procp->p_vas, PT_DATA)) == NULL)
                panic("add_bss: no data region");
        rp = prp->p_reg;
	if (len) {
		reglock(rp);
		if (growpreg(prp, (int)btorp(len), 0, DBD_DZERO, ADJ_REG) < 0) {
			regrele(rp);
			return(1);
		}

                regrele(rp);
        }
        return(0);
}

int
add_stack(len, addr)
	u_int len;
	caddr_t addr;
{
	register preg_t *prp;
	register reg_t *rp;

	/*	
	 * Allocate a user stack and grow it.
	 */
	if ((rp = allocreg((struct vnode *)0,swapdev_vp, RT_PRIVATE)) == NULL) {
		return(1);
	}
#ifdef __hp9000s300
	/*
	 * 300 stacks grow downwards, but regions only grow upwards.  So to
	 * represent the 300 stack we have to use a region whose size
	 * represents the ulimit-imposed max size possible.  But this would
	 * reserve all of that swap space right up front, which is quite
	 * wasteful.  So we set the SWLAZY flag, which then causes us to
	 * allocate stack space only as it's actually needed for this
	 * region's pages.
	 */
	rp->r_flags |= RF_SWLAZY;
	rp->r_swalloc = 0;
#endif
	if (growreg(rp, (int)btorp(len), DBD_DZERO) < 0) {
		freereg(rp);
		return(1);
	}

	if ((prp = attachreg(u.u_procp->p_vas, rp, PROT_URW,
			PT_STACK, PF_EXACT, addr, 0, btorp(len))) == NULL) {
		freereg(rp);
		return(1);
	}

	hdl_procattach(&u, u.u_procp->p_vas, prp);
	regrele(rp);
	return(0);
}


/*
 * Create an EXEC_MAGIC text image.  EXEC_MAGICs are actually created as
 * a writable data segment.  The entire a.out is mapped into the process'
 * address space as a single writable segment.
 */
int
create_execmagic(vp, off, len, addr)
	struct vnode *vp;
	u_int off;
	u_int len;
	caddr_t addr;
{
	register reg_t *rp;
	register preg_t *prp;
	if ((rp = allocreg(vp, swapdev_vp, RT_PRIVATE)) == (reg_t *)NULL)
		return(1);

	if (growreg(rp, (int)btorp(len), DBD_FSTORE) < 0) {
		freereg(rp);
		return(1);
	}

	if ((prp = attachreg(u.u_procp->p_vas, rp, 
			     PROT_URWX, PT_DATA, PF_EXACT, 
			     addr, 0, btorp(len))) == (preg_t *)NULL) {
		freereg(rp);
		return(1);
	}
	hdl_procattach(&u, u.u_procp->p_vas, prp);
	u.u_procp->p_vas->va_flags |= VA_NOTEXT;

	/*
	 * Set byte offset for this region
	 */
	rp->r_flags |= RF_UNALIGNED;
	rp->r_byte = off;
	rp->r_bytelen = len;
	regrele(rp);

	/*
	 * EXEC_MAGICs get paged into memory up front as they are not marked
	 * with VTEXT and another process could write on the image we are
	 * trying to execute on.
	 */
	fault_each(u.u_procp->p_vas, PT_DATA);
	return(0);
}


/*
 * Convert a shared text segment to a private view to protect others
 *  from our writing of breakpoints.
 */
preg_t *
get_private_text(up, vas, prp)
	struct user *up;
	vas_t *vas;
	preg_t *prp;
{
	short prot, pregtype, pregflag;
	caddr_t vaddr;
	size_t off, count;
	vm_sema_state;		/* semaphore save state */
	reg_t *rp = prp->p_reg;
	reg_t *rp2;
#ifdef __hp9000s800
	preg_t *nullprp = NULL;
#endif

	VASSERT(prp->p_type == PT_TEXT);
	VASSERT(rp->r_type == RT_SHARED);
	vmemp_lockx();		/* lock down VM empire */

#ifdef __hp9000s800
	if (nullprp = findpregtype(vas, PT_NULLDREF)) {
		reglock(nullprp->p_reg);
		hdl_procdetach(up, vas, nullprp);
		detachreg(vas, nullprp);
	}
#endif
	/*
	 * Record the values for the old pregion, then throw it & its
	 *  region away.
	 */
	reglock(rp);
	prot = prp->p_prot;
	pregtype = prp->p_type;
	pregflag = (prp->p_flags & ~(PF_ACTIVE|PF_MLOCK));
	vaddr = prp->p_vaddr;
	off = prp->p_off;
	count = prp->p_count;

	/* Duplicate region, bomb out if can't */
	if ((rp2 = dupreg(rp, RT_PRIVATE, off, count)) == NULL) {
		regrele(rp);
		vmemp_returnx(NULL);
	}

	/* Remove the old, shared view of the region */
	hdl_procdetach(up, vas, prp);
	detachreg(vas, prp);

	/* Add a new private view back into the vas */
	if ((prp = attachreg(vas, rp2, prot, pregtype, pregflag,
			vaddr, 0, count)) == NULL) {
		/*
		 * XXX Andy.  The only way for this to fail would be if
		 *  hdl_procattach or chkattach bounced the request.  But
		 *  since we just detached a pregion with the exact same
		 *  parameters this doesn't seem possible.
		 * The real trick is that we have removed the old pregion
		 *  and failed to attach a new one.  We can't just regrele()
		 *  and return because we no longer have a handle on the vnode
		 *  our text was mapped through, so it will never get unmapped.
		 *  We could do it in-line here, but it would be grotty.
		 */
		panic("get_private_text: attachreg for ptrace");
	}

	hdl_procattach(up, vas, prp);

	regrele(rp2);

#ifdef __hp9000s800
	if (nullprp) {
		if (add_nulldref(up, vas))
			panic("get_private_text: could not attach nulldref");
	}
#endif /* hp9000s800 */
	vmemp_unlockx();
	return(prp);
}


#if defined(hp9000s800) && NBPG == 4096
/*
 * GET_UNALIGNED_PRIVATE_TEXT:
 * Convert a shared text segment to a private unaligned text segment
 * to prevent others from seeing our breakpoints.
 * This differs from get_private_text() in that it does not employ
 * copy-on-write to bring the new text pages into existence.
 * Callers: get_text(), and procxmt().
 *
 * NBPG == 4096
 */
preg_t *
get_unaligned_private_text(up, vas, prp)
	struct user *up;
	vas_t *vas;
	preg_t *prp;
{
	short prot, pregtype, pregflag;
	caddr_t vaddr;
	size_t off, count;
	vm_sema_state;		/* semaphore save state */
	reg_t *rp = prp->p_reg;
	reg_t *rp2;

	VASSERT(prp->p_type == PT_TEXT);
	VASSERT(rp->r_type == RT_SHARED);
	vmemp_lockx();		/* lock down VM empire */

	/*
	 * Record the values for the old pregion, then throw it & its
	 * region away.
	 */
	reglock(rp);
	prot = prp->p_prot;
	pregtype = prp->p_type;
	pregflag = (prp->p_flags & ~(PF_ACTIVE|PF_MLOCK));
	vaddr = prp->p_vaddr;
	off = prp->p_off;
	count = prp->p_count;

	if ((rp2 = allocreg(rp->r_fstore, swapdev_vp, RT_PRIVATE)) == NULL) {
		vmemp_returnx(NULL);
	}
	if (growreg(rp2, rp->r_pgsz, DBD_FSTORE) < 0) {
		freereg(rp2);
		vmemp_returnx(NULL);
	}

	/*
	 * Make sure the new region is paged in through the buffer
	 * cache.
	 */
	if (rp->r_flags & RF_UNALIGNED) {
		rp2->r_flags |= RF_UNALIGNED;
		rp2->r_byte = rp->r_byte;
		rp2->r_bytelen = rp->r_bytelen;
	} else {
		/*
		 * Do unaligned anyway even if aligned.
		 */
		rp2->r_flags |= RF_UNALIGNED;
		rp2->r_byte = ptob(off);
		rp2->r_bytelen = ptob(count);
	}

	/* Remove the old, shared view of the region */
	hdl_procdetach(up, vas, prp);
	detachreg(vas, prp);

	/* Add a new private view back into the vas */
	if ((prp = attachreg(vas, rp2, prot, pregtype, pregflag,
			vaddr, 0, count)) == NULL) {
		/*
		 * XXX Andy.  The only way for this to fail would be if
		 *  hdl_procattach or chkattach bounced the request.  But
		 *  since we just detached a pregion with the exact same
		 *  parameters this doesn't seem possible.
		 * The real trick is that we have removed the old pregion
		 *  and failed to attach a new one.  We can't just regrele()
		 *  and return because we no longer have a handle on the vnode
		 *  our text was mapped through, so it will never get unmapped.
		 *  We could do it in-line here, but it would be grotty.
		 */
		panic("get_private_text: attachreg for ptrace");
	}

	hdl_procattach(up, vas, prp);

	regrele(rp2);
	vmemp_unlockx();
	return(prp);
}
#endif /* defined(hp9000s800) && NBPG == 4096 */


/*
 * Attempt remove RF_NOFREE regions and purge page cache of 
 * anything associated with vfsp.  If discless, broadcast this
 * to all other clients.
 */
xumount(vfsp)
	register struct vfs *vfsp;
{
        /* 
	 * There is no such thing as sticky bit in CDFS
	 * BUT XUMOUNT ALSO PURGES THE CACHE (I believe
	 * this is a bug!)  sfk 11/30/89 XXX
	 */
        if (vfsp->vfs_mtype == MOUNT_CDFS)
		return;
	xumount1(vfsp);
	if (my_site_status & CCT_CLUSTERED)
		dux_xumount(vfsp);
}

/*	
 * Attempt to purge all RF_NOFREE regions associated with vfsp 
 * (not implemented yet!).
 * Purge the page cache of all pages associated with vfsp.
 */
xumount1(vfsp)
	register struct vfs *vfsp;
{
#ifdef LATER
	register reg_t		*rp;
	register reg_t		*nrp;
        register u_int          context;
#endif
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */

#ifdef LATER
retry:
	/*
	 * sfk - XXX - 11/30/89
	 * We currently do not create RF_NOFREE regions.  This code is
	 * almost right, but requires a centinal implementation to avoid
	 * an inifinte loop.  Implement later.
	 */

	/*
	 * Walk the active region chain and remove RF_NOFREE regions
	 * for this file system.
	 */
	rlstlock(context);

	for (rp = regactive.r_forw; rp != &regactive; rp = rp->r_forw) {
		if (rp->r_type == RT_SHARED) {
			if (creglock(rp)) {
				if ((rp->r_flag&RF_NOFREE) && 
				    (rp->r_refcnt == 0)) {
					struct vnode *vp = rp->r_fstore;
					if (vp && vp->v_vfsp == vfsp) {
						rlstunlock(context);
						freereg();
 						goto retry;
					}
				}
			}
		}
	}
	rlstunlock(context);
#endif /* LATER */

	/*
	 * Now attempt to purge the page cache.
	 */
	mpurgevfs(vfsp);

	vmemp_unlockx();	/* free up VM empire */
	return;
}


/*
 * Attempt remove RF_NOFREE regions and purge page cache of 
 * anything associated with vp.  If discless, broadcast this
 * to all other clients.
 */
xrele(vp)
	register struct vnode *vp;
{
	site_t tempsite;
	tempsite = u.u_site;

	/*
	 * Local release of sticky text and purge of page cache.
	 */
	u.u_site = my_site;
	xrele1(vp);
        u.u_site = tempsite;

	if (!(my_site_status & CCT_CLUSTERED)) 
		return;

	/*
	 * Broadcast release of sticky text and purge of client 
	 * page caches.
	 */
	if ((vp->v_fstype == VUFS) || (vp->v_fstype == VCDFS)) {
		xrele_send(vp);
	}
}

/*	
 * Attempt to purge all RF_NOFREE regions associated with vp 
 * (not implemented yet!).  Just like xumount but only for
 * one vnode.  Purge the page cache of all pages associated 
 * vp.
 */
xrele1(vp)
	struct vnode *vp;
{
#ifdef LATER
	register reg_t	*rp;
	register reg_t	*nrp;
        register u_int  context;
#endif
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */

#ifdef LATER
retry:
	/*
	 * sfk - XXX - 11/30/89
	 * We currently do not create RF_NOFREE regions.  This code is
	 * almost right, but requires a centinal implementation to avoid
	 * an inifinte loop.  Implement later.
	 */

	/*
	 * Walk the active region chain and remove RF_NOFREE regions
	 * for this file system.
	 */
	rlstlock(context);

	for (rp = regactive.r_forw; rp != &regactive; rp = rp->r_forw) {
		if (rp->r_type == RT_SHARED) {
			if (creglock(rp)) {
				if ((rp->r_flag&RF_NOFREE) && 
				    (rp->r_refcnt == 0)) {
					if (rp->r_fstore == vp) {
						rlstunlock(context);
						freereg();
 						goto retry;
					}
				}
			}
		}
	}
	rlstunlock(context);
#endif /* LATER */

	/*
	 * Now attempt to purge the page cache.
	 */
	mpurge(vp);
	vmemp_unlockx();	/* free up VM empire */
}

/* 
 * Invalidate the text associated with vp.  Purge in core cache of pages
 * associated with vp and kill all active processes.  This code is used when
 * an nfs client has modified the text image we are executing.
 */
xinval(vp)
	register struct vnode *vp;
{
        register u_int  context;
	register reg_t *rp;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */

	/*
         * Make a single pass over the active region chain setting
         * r_zomb on all regions using this vnode so that the next
         * vfault will kill the process.
         *
         * Note: We can check r_fstore without holding the region
         *       lock because r_fstore is not allowed to be modified
         *       once it is on the active chain.
         */
	rlstlock(context);
	for (rp = regactive.r_forw; rp != &regactive; rp = rp->r_forw) {
		if (rp->r_fstore == vp)
			rp->r_zomb = 1;
	}
	rlstunlock(context);

	/*
	 * Purge the cache; so that we don't have corrupted
	 * data.
	 */
	mpurge(vp);
	vmemp_unlockx();	/* free up VM empire */
}

/*
 * Fault each DBD_FSTORE page for the specified
 *  pregion.
 */

void
fault_each(vas, pt)
	vas_t *vas;
	int pt;
{
	preg_t *prp;
	reg_t *rp;
	int i, j;
	dbd_t *dbd;
	vfd_t *vfd;
	pfd_t *pfd;

	/* Get the preg, do nothing if it isn't there */
	if ((prp = findpregtype(vas, pt)) == NULL)
		return;
	rp = prp->p_reg;
	reglock(rp);

	make_unswappable();
	/*
	 * For each DBD, fault it if it's DBD_FSTORE.  Note that we
	 *  specify "write" to break any copy-on-write association.
	 */
	for (i = prp->p_off, j = 0; j < prp->p_count; ++i, ++j) {
		FINDENTRY(rp, i, &vfd, &dbd);

		if (dbd->dbd_type != DBD_FSTORE)
			continue;

		bring_in_pages(prp,prp->p_vaddr + ptob(j),1,1,1);
		FINDENTRY(rp, i, &vfd, &dbd);

		VASSERT(vfd->pgm.pg_v);
		VASSERT(vfd->pgm.pg_cw == 0);

		/*
		 * Make sure the modified page migrates to
		 * back store.  Boo!  Be sure to remove it
		 * from the page cache since we have modified
		 * it from the front store copy.
		 */

		if (dbd->dbd_type == DBD_FSTORE) {
			dbd->dbd_type = DBD_NONE; dbd->dbd_data = 0xfffff13;
			pfd = &pfdat[vfd->pgm.pg_pfn];
                        pfdatlock(pfd);
                        pageremove(pfd);
                        pfdatunlock(pfd);
		}
		vfd->pgm.pg_lock = 0;
	}

	make_swappable();
	regrele(rp);
}

/*
 * Because NFS code sometimes sleeps at interruptible
 * priorities, it is not acceptable to access NFS files
 * from procxmt().  Thus we must be sure to pre-load
 * any demand-loadable pages from a traced process.
 * This routine does just that for the current process.
 * It assumes that the process is demand-loaded (at least
 * that it has shared text.
 *
 * Returns 0 on success, 1 on failure.
 */
int
pre_demand_load(vas)
	register vas_t *vas;
{
	extern preg_t *get_text();

	/* Convert text segment to private */
	if (get_text(vas) == NULL) {
		return(1);
	}

	/* Fault in all pages from the text */
	fault_each(vas, PT_TEXT);

	/* Fault in initialized data */
	fault_each(vas, PT_DATA);
	return(0);
}

/*
 * Local routine to get the "text" segment.  We abstract it to a routine
 *  because for impure-text exexutables the text segment is actually a
 *  part of the data segment.
 */
preg_t *
get_text(vas)
	register vas_t *vas;
{
	register preg_t *prp;
	extern preg_t *get_private_text();
	extern preg_t *get_unaligned_private_text(); /* NBPG == 4096 */

	if (prp = findpregtype(vas, PT_TEXT)) {
		if (prp->p_reg->r_type == RT_SHARED)
#if defined(hp9000s800) && NBPG == 4096
			/*
			 * NBPG == 4096
			 * If we have one of those funky regions that
			 * has the first page mapped unreadable to everyone,
			 * then don't employ copy-on-write
			 * to create the private text region because
			 * doing so will destroy the special access rights
			 * we've set up for that first page.
			 */
			if (prp->p_hdl.hdlflags & PFHDL_NONULLREFPG)
				prp = get_unaligned_private_text(&u,
								 u.u_procp->p_vas,
								 prp);
			else
#endif /* hp9000s800 && NBPG == 4096 */
				prp = get_private_text(&u, u.u_procp->p_vas, prp);
		return(prp);
	}
	if (prp = findpregtype(vas, PT_DATA))
		return(prp);
	return(NULL);
}

/*
 * Inctext() increments the text reference count associated with a vnode
 * vp by 1.  Unlike dectext, the second parameter is not a count, but is a
 * flag.  The flag is only meaningful in the case of DUX.  If
 * serverinitiated is SERVER_INIT, it means that the server is aware that
 * the text count is going to be incremented, and has incremented its
 * local count.  An example of this is the execve() system call, where
 * the vnode is first looked up at the server.  The server increments its
 * count and passes back the vnode to the client.  The client
 * subsequently increments its own vnode count.
 * 
 * If serverinitiated is CLIENT_INIT, it means that the server is not
 * aware that the text count is going to be incremented.  One example of
 * this is a fork.  No vnode has been loked up at the server, and
 * therefore the server has not been involved.  Another example is when
 * memory mapping in a file (for example for shared libraries), since the
 * file was opened with an open rather than with an exec.  In this case,
 * it is the client's responsibility to notify the server, if necessary
 * (although in many cases, notifying the server can be avoided).
 */

int
inctext(vp, serverinitiated)
	struct vnode *vp;
	int serverinitiated;
{
	vnodelock(vp);

	if ((vp->v_tcount > 0) && ((vp->v_flag&VTEXT) == NULL))
		panic("inctext: VTEXT not set and tcount > 0");
	
	switch(vp->v_fstype) {
	case VUFS: {
		struct inode *ip = VTOI(vp);
		updatesitecount(&ip->i_execsites, u.u_site, 1);
		break;
	}
	case VCDFS: {
		struct cdnode *cdp = VTOCD(vp);
		updatesitecount(&cdp->cd_execsites, u.u_site, 1);
		break;
	}
	case VDUX_CDFS: {
		if (update_text_change(vp, 1, serverinitiated) != 0) {
		    vnodeunlock(vp);
		    return 1;
		}
		break;
	}
	case VDUX: {
		struct inode *ip = VTOI(vp);
		/*
		 * If this is the first reference to the text segment and
		 * the inode has changed, invalidate all of the pages
		 * we have saved in the page cache.
		 */
#ifdef OSDEBUG
		if (vp->v_tcount) {
			if ((ip->i_flag & IPAGEVALID) == NULL) {
				panic("inctext: IPAGEVALID no longer set");
			}
		}
#endif
		if ((vp->v_tcount == 0) && !(ip->i_flag & IPAGEVALID)) {
			int i, count;
			count = howmany(ip->i_size, DEV_BSIZE);
			for (i = 0; i < count; i+= NBPG/DEV_BSIZE)
				munhash(vp, (daddr_t)i);
			ip->i_flag |= IPAGEVALID;
		}
		if (update_text_change(vp, 1, serverinitiated) != 0) {
		    vnodeunlock(vp);
		    return 1;
		}
		break;
	} /* end of case VDUX */
	} /* end of case */
	VN_HOLD(vp);
	vp->v_flag |= VTEXT;
	vp->v_tcount++;
	vnodeunlock(vp);
	return 0;
}

/*
 * Dectext() will decrement the reference count associated with the vnode
 * vp by number.  Note that unlike inctext(), dectext() has a count which
 * may be greater than 1.  Values greater than 1 are only used by DUX,
 * primarily for error recovery.  There is also not serverinitiated field
 * for dectext (effectively, it is always CLIENT_INIT).
 */

dectext(vp, number)
	struct vnode *vp;
	int number;
{
	int i;

	vnodelock(vp);

	VASSERT(vp->v_flag&VTEXT);
	VASSERT(vp->v_tcount);

	switch(vp->v_fstype) {
	case VUFS: {
		struct inode *ip = VTOI(vp);
		/*only update vnode if this is the first text reference*/
		updatesitecount(&ip->i_execsites, u.u_site, -number);
		break;
	}
	case VCDFS: {
		register struct cdnode *cdp = VTOCD(vp);
		/*only update vnode if this is the first text reference*/
		updatesitecount(&cdp->cd_execsites, u.u_site, -number);
		break;
	}
	case VDUX_CDFS:
		/* Fall through */
	case VDUX:
		update_text_change(vp, -number, 0);
		break;
	}

	/*
	 * Decrement the number of active holders of VTEXT.
	 */
	VASSERT(vp->v_tcount >= number);
	if ((vp->v_tcount -= number) == 0)
		vp->v_flag &= ~VTEXT;

	/*
	 * Now that VTEXT is cleared or decremented correctlty, we
	 * can free the synchronization lock.
	 */
	vnodeunlock(vp);
	for (i=0; i < number; i++) {
		VN_RELE(vp);
	}
}


/*
 * If someone in the system (or cluster) has this file open for write,
 * return true; otherwise return 0.
 * This concept extends to memory mapped files, which may have the file
 * mapped writable.
 *
 * If 'inc_textref' is non-zero, this function will increment the text
 * reference on the file if the file is not open for write.
 */
openforwrite(vp, inc_textref)
	struct vnode *vp;
	int inc_textref;
{
	/*
	 * No need to check on READ-ONLY file systems.
	 */
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		if (inc_textref)
			(void)inctext(vp, USERINITIATED);
		return(0);
	}

	/*
	 * If it's only us, then clearly no one else has it open for write.
	 */
	if (vp->v_count != 1) {
		/*
		 * If the file is memory mapped, then we can look at
		 * the va_wcount field in its psuedo-vas to see if
		 * there are any writable memory mappings.
		 */
		if (vp->v_flag & VMMF) {
			VASSERT(vp->v_vas != (vas_t *)0);
			if (vp->v_vas->va_wcount != 0)
				return(1);
		}

		/*
		 * We have some shortcuts to checking for mutual
		 * exclusion:
		 *   . If this is a DUX file, we checked (or will
		 *     check) at the server.
		 *   . If this is a UFS file, just check the write
		 *     sitemap.
		 */
		if (vp->v_fstype == VUFS) {
			if (gettotalsites(&((VTOI(vp))->i_writesites)) != 0) {
				return(1);
			}
		}
		else if (vp->v_fstype != VDUX) {
			register struct file *fp;
			for (fp = file; fp < fileNFILE; fp++) {
				if (fp->f_type == DTYPE_VNODE &&
				    (struct vnode *)fp->f_data == vp &&
				    fp->f_count > 0 &&
				    (fp->f_flag&FWRITE)) {
					return(1);
				}
			}
		}
	}

	/*
	 * It does not appear that the file is open for write.  If we
	 * were asked to call inc_text(), then it may be that this is a
	 * DUX file that might be open for write at the server.  We call
	 * inc_text(), which will return non-zero if this the first text
	 * reference to a DUX vnode and the server returns an error from
	 * update_text_change().
	 */
	if (inc_textref && inctext(vp, USERINITIATED) != 0)
		return(1);

	return(0);
}
