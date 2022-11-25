/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/atl.c,v $
 * $Revision: 1.6.84.5 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/06/15 13:32:28 $
 */


#include "../mach.300/atl.h"
#include "../mach.300/pte.h"
#include "../h/malloc.h" 
#include "../h/pregion.h"
#include "../h/pfdat.h"
#include "../h/vas.h"
#include "../h/vdma.h"


#define NOT_USED (unsigned short)-1

#ifdef OSDEBUG
#define ATL_PROFILE
#endif

#ifdef ATL_PROFILE
#define ATL_PFD_PROF_SIZE 32
#define ATL_PRP_PROF_SIZE 1024

int atl_pfd_cur[ATL_PFD_PROF_SIZE] = {0};
int atl_pfd_max[ATL_PFD_PROF_SIZE] = {0};
int atl_prp_used=0;
int atl_prp_vacant=0;
int atl_prp_realloc=0;
int atl_prp_realloc_size[ATL_PRP_PROF_SIZE] = {0};
int atl_prp_alloc_size[ATL_PRP_PROF_SIZE] = {0};
#endif


/* 
 * perform atl initializations.  
 */
atl_init()
{
}

/************************************************************************
 * 	The next several routines manupulate the atl data associated 
 * 	with the pfd
 ************************************************************************/

struct pfd_atl_entry *
malloc_pfd_entries(count)
int count;
{
	struct pfd_atl_entry *ret;
	int bytes = count * sizeof(struct pfd_atl_entry);


	ret = (struct pfd_atl_entry *)kmalloc(bytes, M_ATL, M_WAITOK);
	if (ret == NULL)
		panic("malloc_pfd_entries: unable to get space");
	return ret;
}


pfd_atl_grow(pfd_atlp)
struct pfd_atl *pfd_atlp;
{
	struct pfd_atl_entry * new_data;
	int new_index;

	do {
	    new_index = pfd_atlp->max_index+ATL_PFD_CHUNK_SIZE;
	    new_data = malloc_pfd_entries(new_index);

	    /* We could have gone to sleep in malloc_pfd_entries() and had
	       the attach list grown for us by another process.  Check to 
	       see if it still needs to grow.   Also beware that it could
	       have gotten bigger, but still need to be grown.
	    */
	    if (   (new_index > pfd_atlp->max_index)
	        && (pfd_atlp->max_index == pfd_atlp->cur_index))
	    {
		    /* We have succeeded.  Put the old data into the new 
		     * buffer 
		    */
		    if (pfd_atlp->atl_data) {
		    	bcopy(pfd_atlp->atl_data, new_data,
			    pfd_atlp->max_index * sizeof(struct pfd_atl_entry));
		    	kfree(pfd_atlp->atl_data,M_ATL);
		    }
		    pfd_atlp->atl_data = new_data;
		    pfd_atlp->max_index = new_index;
	    } else {
		    /* Some other process grew the attach list while we
		     * were asleep.  We may still need to grow it, but
		     * check for that later.
		    */
		    kfree(new_data,M_ATL);
	    }
	} while  (pfd_atlp->max_index == pfd_atlp->cur_index);

	return;
}

/* 
 * pfd_atl_shrink actually does not shrink, it deallocates and then reallocates
 * an entry.
 */

pfd_atl_shrink(pfd_atl)
struct pfd_atl *pfd_atl;
{
	struct pfd_atl_entry * new_data;
	int new_index;

#ifdef ATL_PROFILE
	{
		int idx;

		idx = (pfd_atl->cur_index >= ATL_PFD_PROF_SIZE?
			ATL_PFD_PROF_SIZE-1:pfd_atl->cur_index);
		atl_pfd_cur[idx]++;
		idx = (pfd_atl->max_index >= ATL_PFD_PROF_SIZE?
			ATL_PFD_PROF_SIZE-1:pfd_atl->max_index);
		atl_pfd_max[idx]++;

	}

#endif

	pfd_atl->cur_index = 0;
	if (pfd_atl->max_index > ATL_PFD_CHUNK_SIZE) 
	{
		new_index = ATL_PFD_CHUNK_SIZE;
		new_data = malloc_pfd_entries(new_index);
		/* We could have slept in malloc_pfd_entries.  Can't blow
		   this away if some other process now has an entry.
		*/
		if (    (pfd_atl->cur_index == 0)  
		    &&  (pfd_atl->max_index > ATL_PFD_CHUNK_SIZE))
		{

			VASSERT(pfd_atl->atl_data);
			kfree(pfd_atl->atl_data, M_ATL);
			pfd_atl->max_index = new_index;
			pfd_atl->atl_data = new_data;
		} else 
			kfree(new_data, M_ATL);
	}
	return;
}

add_to_pfdat_list(pfn, pt, prp, idx)
int pfn;
struct pte *pt;
struct pregion *prp;
int idx;
{
	struct pfd_atl *pfd_atl = &pfdat[pfn].pf_hdl.pf_atl;
	register struct pfd_atl_entry *pfd_atle;

	VASSERT(pfd_atl);

	if (pfd_atl->max_index == pfd_atl->cur_index) 
	{
		/* grow the list */
		pfd_atl_grow(pfd_atl);
	}

	VASSERT(pfd_atl->cur_index < pfd_atl->max_index);
	pfd_atle = pfd_atl->atl_data+pfd_atl->cur_index;
	pfd_atle->pte = pt;
	pfd_atle->prp = prp;
	pfd_atle->index = idx;

	return pfd_atl->cur_index++;
}

pfdat_atl_remove(pfn, element)
int pfn, element;
{
	struct pfd_atl *pfd_atl;
	struct pfd_atl_entry *pfd_atle;
	struct pregion_atl_entry *prp_atle;

	VASSERT(pfn != NOT_USED);

	pfd_atl = &pfdat[pfn].pf_hdl.pf_atl;
	pfd_atle = pfd_atl->atl_data+element;

	VASSERT(pfd_atl);

	VASSERT(pfd_atl->cur_index);
	VASSERT(pfd_atl->cur_index > element);

	*pfd_atle = pfd_atl->atl_data[--pfd_atl->cur_index];
	if (pfd_atl->cur_index)
	{
		prp_atle = pfd_atle->prp->p_hdl.p_atl->atl_data + pfd_atle->index;
		VASSERT(prp_atle->pfd_index == pfn);
		VASSERT(prp_atle->entry_number == pfd_atl->cur_index);
		prp_atle->entry_number = element;
	}
#ifdef OSDEBUG
	pfd_atl->atl_data[pfd_atl->cur_index].index = -1;
#endif

	/* if compaction was desired, it could be done here */
#ifdef PFDAT32	
	/*
	 *  It is desired :-)  This is because we are now sharing the
	 *  space in the pfdat between ATLs, IPTEs, and DPTEs.  Given this,
	 *  it is no longer acceptable for the ATL code to hold malloc'ed
	 *  memory across a freepfd()/allocpfd() pair; it needs to give up
	 *  the memory when the page goes away and get the memory back 
	 *  later if/when it is needed.  Otherwise, we have a fast leak
	 *  of kernel RAM!
	 *						-- DKM 6-15-94
	 */
	if (pfd_atl->cur_index < 1 && pfd_atl->max_index > 0 && 
	    pfd_atl->atl_data) {
		kfree(pfd_atl->atl_data, M_ATL);
		pfd_atl->max_index = 0;	
		pfd_atl->cur_index = 0;		
		pfd_atl->atl_data = (struct pfd_atl_entry *) NULL;
	}
#endif

	return;
}

/****************************************************************************
 *	The next group of routines manipulate the prp attach list 
 ****************************************************************************/

struct pregion_atl_entry *
malloc_prp_entries(entries)
int entries;
{
	struct pregion_atl_entry *ret;
	int bytes;

	bytes = entries*sizeof(struct pregion_atl_entry);

	ret=(struct pregion_atl_entry *)kmalloc(bytes, M_ATL, M_WAITOK);

	if (ret == NULL)
		panic("pregion_atl_entry: unable to get space for entries");

	return ret;
}

atl_procattach(prp)
preg_t *prp;
{
	struct pregion_atl *prp_atl;
	register struct pregion_atl_entry *prp_atle;
	register struct pregion_atl_entry *end;

	VASSERT(prp->p_hdl.p_ntran == 0);
	VASSERT(prp->p_hdl.p_atl == NULL);

	prp_atl=(struct pregion_atl *)kmalloc(sizeof(*prp_atl), M_ATL, M_WAITOK);
	if (prp_atl == NULL)
		panic("atl_procattach: unable to get space for prp_atl");


	prp_atl->count=prp->p_count;
	prp_atl->lowest=prp_atl->count;

	/*
	 * Aack.  Due to regions design  problem where regions only grow up,
	 * the S300 stack is allocated big, and it is 
	 * set up to be a "good" size already.
	 * And besides, it cannot grow upwards so don't give it
	 * any extra room.
	 */


	if (prp->p_type != PT_STACK)
		prp_atl->count += ATL_PRP_EXTRA;


	prp_atl->atl_data = malloc_prp_entries(prp_atl->count);

	end = prp_atl->atl_data+prp_atl->count;
	for(prp_atle=prp_atl->atl_data;
	    prp_atle != end;prp_atle++)
	{
		prp_atle->pfd_index = NOT_USED;
#ifdef OSDEBUG
		prp_atle->entry_number = 0xabcd;
#endif
	}

#ifdef ATL_PROFILE
	{
		int idx;
		idx = (prp_atl->count >= ATL_PRP_PROF_SIZE?
			ATL_PFD_PROF_SIZE-1:prp_atl->count);
		atl_prp_alloc_size[idx]++;
	}
#endif

	prp->p_hdl.p_atl = prp_atl;

	VASSERT(prp->p_hdl.p_atl != NULL);
	VASSERT(prp->p_hdl.p_atl->count >= prp->p_count);

}

prp_atl_grow(prp, prp_atl, change)
preg_t *prp;
struct pregion_atl *prp_atl;
int change;
{
	struct pregion_atl_entry *hold, *end, *prp_atle;
	int new_size, old_size;

	VASSERT(change > 0);

	new_size = prp_atl->count + change;
	old_size = MIN(prp->p_count, prp_atl->count);
	hold = prp_atl->atl_data;
	VASSERT(old_size <= prp_atl->count);

	if (new_size > prp_atl->count)
	{
		VASSERT(new_size < 0x10000);
		prp_atl->atl_data = malloc_prp_entries(new_size);

		if (hold)
		{
#ifdef ATL_PROFILE
			atl_prp_realloc++;
			{
				int idx;
				int change = new_size-old_size;

				idx = (change >= ATL_PRP_PROF_SIZE?
					ATL_PFD_PROF_SIZE-1:change);
				atl_prp_realloc_size[idx]++;
			}
#endif
			bcopy(hold, prp_atl->atl_data,
				 old_size * sizeof(struct pregion_atl_entry));

			kfree(hold,M_ATL);
		}
	}

	/* ouch !! zero this out */

	end = prp_atl->atl_data+new_size;
	for(prp_atle=prp_atl->atl_data+old_size;
	    prp_atle != end;prp_atle++)
	{
		prp_atle->pfd_index = NOT_USED;
#ifdef OSDEBUG
		prp_atle->entry_number = 0xbcde;
#endif
	}

	prp_atl->count = new_size;

	VASSERT(prp->p_hdl.p_atl->count >= prp->p_count + change);
}


prp_atl_destroy(prp)
preg_t *prp;
{
	struct pregion_atl *prp_atl;
	register struct pregion_atl_entry *prp_atle;
	register struct pfd_atl_entry *pfd_atlep;
	int x;


	x = CRIT();

	if ((prp_atl = prp->p_hdl.p_atl) == NULL) {
	    UNCRIT(x);
	    return;
	}

	VASSERT(prp->p_count <= prp_atl->count);

	for(prp_atle = prp_atl->atl_data + prp_atl->lowest;
	    prp->p_hdl.p_ntran; prp_atle++)
	{
		VASSERT(prp_atle < prp_atl->atl_data + prp->p_count);
		if (prp_atle->pfd_index != NOT_USED)
		{
#ifdef ATL_PROFILE
			atl_prp_used ++;
#endif
			prp->p_hdl.p_ntran--;
			VASSERT(prp->p_hdl.p_ntran >= 0);

			pfd_atlep = &pfdat[prp_atle->pfd_index].pf_hdl.pf_atl.atl_data[prp_atle->entry_number];
			VASSERT(pfd_atlep->prp == prp);

			if (pfd_atlep->pte->pg_notify)
				do_vdma_notify(VDMA_UNVIRTUALIZE, prp,
					pfd_atlep->index, pfd_atlep->pte, 0);

			pfdat_atl_remove(prp_atle->pfd_index, 
					 prp_atle->entry_number);
#ifdef OSDEBUG
			prp_atle->pfd_index = 0xbeef;
#endif
		} 
#ifdef ATL_PROFILE
		else
			atl_prp_vacant++;
#endif
	}

	if (prp_atl->atl_data)
		kfree(prp_atl->atl_data, M_ATL);

	kfree(prp_atl,M_ATL);

	prp->p_hdl.p_atl = NULL;

	UNCRIT(x);
}

/****************************************************************************
 *	The rest of the file contains the real atl routines
 ****************************************************************************/

/* 
 * Notify the VDMA hardware if this pte is going away 
 */
do_vdma_notify(func,prp,index, pt, flags)
int func;
preg_t *prp;
int index;
struct pte *pt;
int flags;
{
		int return_val;

		struct vnotify ns;
                ns.v_type = func;
                ns.v_vas = prp->p_vas;
                ns.v_space = prp->p_space;
                ns.v_vaddr = prp->p_vaddr + ptob(index);
                ns.v_cnt = 1;
                ns.v_hil_pfn = pfntohil(pt->pg_pfnum);

		/* 
		 * only for VDMA_UNSETBITS does the flags 
		 * matter 
		 */

		if (func == VDMA_UNSETBITS)
			ns.v_flags = flags;
		else
			ns.v_flags = pt->pg_prot;

                return vdma_notify(&ns);
}

/* 
 * add atl entry for *pfd (which is pte *pt) into prp
 */

atl_addtrans(prp, pfn, p, vaddr)
preg_t *prp;
int pfn;
struct pte *p;
caddr_t vaddr;
{
	struct pregion_atl *prp_atl;
	struct pregion_atl_entry *prp_atle;
	int kernel_preg;
	int idx;
	int x;
	extern preg_t *kvm_preg;
#ifdef PFDAT32
	struct pfdat *pfd;
	
	VASSERT(pfd_is_locked(&pfdat[pfn]));
#else	
	VASSERT(vm_valusema(&pfdat[pfn].pf_lock) <= 0);
#endif	

	x=CRIT();

	kernel_preg = (prp->p_vas == &kernvas && prp != kvm_preg);

	VASSERT(kernel_preg || prp->p_type == PT_IO || prp->p_hdl.p_atl != NULL);
	idx=btop(vaddr - prp->p_vaddr);
#ifdef PFDAT32
	pfd = pfdat + pfn;
#endif	
	VASSERT(prp);

#ifdef PFDAT32
	if (prp->p_type == PT_IO || kernel_preg || (pfd->pf_flags & P_SYS)) 
#else	
	if (prp->p_type == PT_IO || kernel_preg || (pfdat[pfn].pf_flags & P_SYS)) 
#endif	
	{
		UNCRIT(x);
		return ; /* these dont need any of this */
	}

#ifdef PFDAT32
	pfd->pf_hdl.pf_bits |= PFHDL_ATLS;
#endif	
	prp_atl = prp->p_hdl.p_atl;

	VASSERT(prp_atl);
	VASSERT(prp_atl->count >= prp->p_count);
	VASSERT(prp_atl->atl_data);
	VASSERT(idx < prp_atl->count);

	prp_atle = prp_atl->atl_data + idx; 

	if (idx < prp_atl->lowest)
		prp_atl->lowest = idx;

	if (prp_atle->pfd_index == NOT_USED)
	{
		prp_atle->pfd_index = pfn;

		/* fill in the pfdat list */

		prp_atle->entry_number = add_to_pfdat_list(pfn, p, prp, idx);
		prp->p_hdl.p_ntran++;
	}
	else
		if (prp_atle->pfd_index != pfn)
			panic("atl_addtrans: clobbering translation");

	VASSERT(prp->p_hdl.p_ntran>0);
	VASSERT(prp->p_hdl.p_ntran <= prp->p_count);
	VASSERT(prp_atle->pfd_index == pfn);

	UNCRIT(x);

	return ;
}


/* 
 * delete the translation from the atl
 */

atl_deletetrans(prp, space, vaddr, purge_tlb)
preg_t *prp;
space_t space;
caddr_t vaddr;
int purge_tlb;	/* purge tlb? */
{
	int idx;
	struct pregion_atl_entry *prp_atle;
	struct pte *p;
	pfd_t *pfd;
	int kernel_preg;
	int x;
	int pfn;
	extern preg_t *kvm_preg;

	x=CRIT();
	
	kernel_preg = (prp->p_vas == &kernvas && prp != kvm_preg);

	VASSERT(prp->p_type != PT_IO);

	p = vtopte(prp,vaddr-prp->p_vaddr);

	/* clear out the pte if not already clear */

	if (p && p->pg_v) 
	{

	/* is there any point in doing a cache purge here ?? */

		idx=btop(vaddr - prp->p_vaddr);
		prp_atle = prp->p_hdl.p_atl->atl_data+idx;

		VASSERT(kernel_preg || prp->p_hdl.p_ntran == 0 || 
			prp->p_hdl.p_atl != NULL);
		pfn = pfntohil(p->pg_pfnum);
		pfd = pfdat + pfn;

		VASSERT((pfd->pf_flags&P_SYS) || kernel_preg || 
			 prp->p_hdl.p_ntran == 0 ||
			 idx < prp->p_hdl.p_atl->count);

		VASSERT(kernel_preg || (pfd->pf_flags&P_SYS) || 
			prp->p_hdl.p_ntran == 0 || 
			prp_atle->pfd_index == NOT_USED || 
			prp_atle->pfd_index == pfn);

		if (p->pg_notify)
		{
			if (p->pg_m == 0 || p->pg_ref==0)
				*(int *)p |=
					do_vdma_notify(VDMA_GETBITS, prp, idx, p, 0);

			do_vdma_notify(VDMA_DELETETRANS, prp, idx, p, 0);
		}
		/* propagate reference/modified bits */

		/* 
		 * The PFHDL_PROTECTED/PFHDL_UNSET weirdness comes from us
		 * not having attach list entries for system translations
		 * gotten via user_protect.
		 */

		if (p->pg_m)
			if (((pfd->pf_hdl.pf_bits & PFHDL_PROTECTED) == 0) ||
			    ((pfd->pf_hdl.pf_bits & PFHDL_UNSET_MOD) == 0)) 
				pfd->pf_hdl.pf_bits |= VPG_MOD;

		if (p->pg_ref)
			if (((pfd->pf_hdl.pf_bits & PFHDL_PROTECTED) == 0) ||
			    ((pfd->pf_hdl.pf_bits & PFHDL_UNSET_REF) == 0)) 
				pfd->pf_hdl.pf_bits |= VPG_REF;

		*(int *)p = 0; /* blast this translation */

		/* delete this translation from the caches */

		if (purge_tlb)
			if (space == KERNELSPACE)
			{
				purge_dcache_s();
				purge_tlb_super();
			}
			else
			{
				purge_dcache_u();
				purge_tlb_user();
			}
	}
	else
		pfd = (pfd_t *)NULL;
	

	if (kernel_preg || pfd==NULL || (pfd->pf_flags&P_SYS) ||
		prp->p_hdl.p_atl == NULL || prp_atle->pfd_index == NOT_USED)
	{
		UNCRIT(x);
		return ; /* there is no translation on the attach list */
	}

	/*
	 * now the translation is gone.  Remove all records of it
	 * from the linked lists off the prp and the pfd 
	 */

	VASSERT(prp->p_hdl.p_atl != NULL);
	VASSERT(idx < prp->p_hdl.p_atl->count);
	VASSERT(prp_atle->pfd_index == pfn);

	pfdat_atl_remove(prp_atle->pfd_index,prp_atle->entry_number);
	prp_atle->pfd_index = NOT_USED;
#ifdef OSDEBUG
	prp_atle->entry_number = 0xdead;
#endif
	prp->p_hdl.p_ntran--;

	VASSERT(prp->p_hdl.p_ntran>=0);
	
	UNCRIT(x);

	return ;
}


/* 
 * unvirtualize a page.  Remove it from both the pfdat list and the pregion
 * list.
 */

atl_unvirtualize(pfn)
int pfn;
{
	struct pfd_atl *pfd_atl;
	register struct pfd_atl_entry *pfd_atle;
	struct pregion_atl *prp_atl;
	struct pregion_atl_entry *prp_atle;
	register int i;
	int x;

	x=CRIT();

	pfd_atl = &pfdat[pfn].pf_hdl.pf_atl;

	VASSERT(pfd_atl);

	pfd_atle = pfd_atl->atl_data;

	for(i=0;i<pfd_atl->cur_index;i++,pfd_atle++)
	{
		if (pfd_atle->pte->pg_notify)
			do_vdma_notify(VDMA_UNVIRTUALIZE, pfd_atle->prp, 
				pfd_atle->index, pfd_atle->pte, 0);
		*(int *)pfd_atle->pte = 0; /* remove access */

		/* clean up the prp pointers */
		prp_atl = pfd_atle->prp->p_hdl.p_atl;
		prp_atle = prp_atl->atl_data + pfd_atle->index;
		prp_atle->pfd_index = NOT_USED;
		pfd_atle->prp->p_hdl.p_ntran--;
		VASSERT(pfd_atle->prp->p_hdl.p_ntran>=0);
#ifdef OSDEBUG
		prp_atle->entry_number = 0;
#endif
			/* mark it as unused */
		pfd_atle->prp = NULL;
	}

	pfd_atl_shrink(pfd_atl); /* This is now empty -- make it smaller */

#ifdef PFDAT32
	pfdat[pfn].pf_hdl.pf_bits &= 0x3f8c;	/*  don't clear type of PTE  */
#else	
	pfdat[pfn].pf_hdl.pf_bits = 0;
#endif	
	purge_tlb();

	UNCRIT(x);

	return;
}

atl_prot(pfd_atlep, allow)
struct pfd_atl_entry *pfd_atlep;
int allow;
{
	int vdma_func;

	VASSERT(pfd_atlep);

	if (allow)
	{
		pfd_atlep->pte->pg_v = 1; /* mark the page as valid */
		vdma_func = VDMA_USERUNPROTECT;
	}
	else
	{
		pfd_atlep->pte->pg_v = 0; /* mark the page as invalid */
		vdma_func = VDMA_USERPROTECT;
	}

	if (pfd_atlep->pte->pg_notify)
		do_vdma_notify(vdma_func, pfd_atlep->prp, pfd_atlep->index, 
			    pfd_atlep->pte, 0);

}

atl_steal(pfn)
int pfn;
{
	atl_unvirtualize(pfn);
}

atl_cw(pfn)
int pfn;
{
	int x;
	pfd_t *pfd;
	struct pfd_atl *pfd_atl;
	register struct pfd_atl_entry *pfd_atlep;
	register struct pte *pt;
	register int i, bits;

	x=CRIT();

	pfd = pfdat+pfn;
	pfd_atl = &pfdat[pfn].pf_hdl.pf_atl;
	VASSERT(pfd_atl);
	pfd_atlep = pfd_atl->atl_data;
	bits = 0;

	for(i=0;i<pfd_atl->cur_index;i++,pfd_atlep++)
	{
		pt = pfd_atlep->pte;
		bits |= *(unsigned *)pt;
		if (pt->pg_notify)
		{
			if ((bits & (PG_M|PG_REF)) != (PG_M|PG_REF)) {
				bits |= vdma_notify(VDMA_GETBITS,
					    pfd_atlep->prp,
					    pfd_atlep->index,
					    pt, 0);
			}
			do_vdma_notify(VDMA_READONLY,
					    pfd_atlep->prp,
					    pfd_atlep->index,
					    pt, 0);
		}
		pt->pg_prot = 1; /* disable write access */
	}


       /* Propagate ref & mod into master copy in pfdat entry */
        if (bits & PG_M)
                pfd->pf_hdl.pf_bits |= VPG_MOD;
        if (bits & PG_REF)
                pfd->pf_hdl.pf_bits |= VPG_REF;

        purge_tlb_user();

	UNCRIT(x);

	return ;
}

atl_pfdat_do(pfn,func,arg)
int pfn;
int (*func)();
int arg;
{
	struct pfd_atl *pfd_atl;
	register struct pfd_atl_entry *pfd_atlep;
	register struct pfd_atl_entry *end;
	int x;

	x = CRIT();

	pfd_atl = &pfdat[pfn].pf_hdl.pf_atl;

	VASSERT(pfd_atl);
	pfd_atlep = pfd_atl->atl_data;
	end = pfd_atl->atl_data + pfd_atl->cur_index;

	for(pfd_atlep = pfd_atl->atl_data; pfd_atlep != end; pfd_atlep++)
		(*func)(pfd_atlep,arg);

	UNCRIT(x);
	return ;
}

atl_promote_write(pfd_atlep)
struct pfd_atl_entry *pfd_atlep;
{
	pfd_atlep->pte->pg_prot = 0;
}


/* 
 * walk all the vfds in region rp starting at byte offset idx and
 * for count entries.  Add translations for all the valid pages.
 */
int
do_addatlsc(rp, idx, vd, count, prp)
reg_t *rp;
int idx;
struct vfddbd *vd;
int count;
preg_t *prp; 
{
	struct pte *pt=NULL;
	caddr_t vaddr = prp->p_vaddr + ptob(idx - prp->p_off);

	for (; count--; vd++,vaddr += NBPG) 
	{
		vfd_t *vfd = &(vd->c_vfd);
		int pfn = vfd->pgm.pg_pfn;

		if ((pt == NULL) || (END_PT_PAGE(pt))) 
			pt = vastopte((vas_t *)prp->p_space,  vaddr);
		if (pt == NULL)
			continue;

		if (vfd->pgm.pg_v && pt->pg_v && pfn == pfntohil(pt->pg_pfnum))
		{
			pfd_t *pfd = pfdat+pfn;
			pfdatlock(pfd);
			atl_addtrans(prp, pfn, pt, vaddr);
			pfdatunlock(pfd);
		}
		else
			pt->pg_v = 0; /* it is not valid */

		pt++;
	}
}

atl_getbits(pfn)
int pfn;
{
	struct pfd_atl *pfd_atl;
	pfd_t *pfd;
	register struct pfd_atl_entry *pfd_atlep;
	register struct pfd_atl_entry *end;
	register int bits=0;
	int x;

	x = CRIT();

	pfd = pfdat+pfn;
	pfd_atl = &pfd->pf_hdl.pf_atl;

	VASSERT(pfd_atl);
	pfd_atlep = pfd_atl->atl_data;
	end = pfd_atl->atl_data + pfd_atl->cur_index;

        for(pfd_atlep = pfd_atl->atl_data; pfd_atlep != end; pfd_atlep++)
	{
		if (pfd_atlep->prp->p_type == PT_UAREA)
		{
			bits=(PG_M|PG_REF);
			break;
		}
				
                bits |= *(unsigned *)pfd_atlep->pte;

		if (pfd_atlep->pte->pg_notify && 
		    ((bits & (PG_M|PG_REF)) != (PG_M|PG_REF)))
			bits |= do_vdma_notify(VDMA_GETBITS, pfd_atlep->prp, 
				               pfd_atlep->index, pfd_atlep->pte, 0);
	}

      /* Propagate ref & mod into master copy in pfdat entry */
        if (bits & PG_M)
                pfd->pf_hdl.pf_bits |= VPG_MOD;
        if (bits & PG_REF)
                pfd->pf_hdl.pf_bits |= VPG_REF;
 
	UNCRIT(x);
}

atl_unsetbits(pfn,bits)
int pfn;
int bits;
{
	struct pfd_atl *pfd_atl;
	pfd_t *pfd;
	register struct pfd_atl_entry *pfd_atlep;
	register struct pfd_atl_entry *end;
	int x;

	x = CRIT();

	pfd = pfdat+pfn;
	pfd_atl = &pfd->pf_hdl.pf_atl;
	VASSERT(pfd_atl);
	pfd_atlep = pfd_atl->atl_data;
	end = pfd_atl->atl_data + pfd_atl->cur_index;

	if (pfd->pf_hdl.pf_bits & PFHDL_PROTECTED)
	{
		if (bits&VPG_MOD)
			pfd->pf_hdl.pf_bits |= PFHDL_UNSET_MOD;

		if (bits&VPG_REF) 
			pfd->pf_hdl.pf_bits |= PFHDL_UNSET_REF;
	}
	for(pfd_atlep = pfd_atl->atl_data; pfd_atlep != end; pfd_atlep++)
	{
		
		VASSERT(pfd_atlep->pte);

		if (pfd_atlep->pte->pg_notify)
		{ 
			int vdma_flags;

			vdma_flags = 0;
			if (bits & VPG_MOD)
				vdma_flags |= PG_M;
			if (bits & VPG_REF)
				vdma_flags |= PG_REF;
			do_vdma_notify(VDMA_UNSETBITS, pfd_atlep->prp, 
				    pfd_atlep->index, pfd_atlep->pte, vdma_flags);
		}
		if (bits&VPG_MOD)
			pfd_atlep->pte->pg_m = 0;
		if (bits&VPG_REF)
			pfd_atlep->pte->pg_ref = 0;

	}

	UNCRIT(x);
}

atl_grow(prp,change)
register preg_t *prp;
int change;
{
	struct pregion_atl *prp_atl;
	register struct pregion_atl_entry *prp_atle;
	register struct pregion_atl_entry *end;
	register space_t space;
	register caddr_t vaddr;

	prp_atl = prp->p_hdl.p_atl;
	VASSERT(prp_atl);

	VASSERT(prp->p_count < 0x10000);

	end = prp_atl->atl_data+prp->p_count;
	prp_atle = prp_atl->atl_data;

	if (change < 0)
	{	/* shrink the region */
		if (prp->p_count+change < prp_atl->lowest)
			prp_atle += prp_atl->lowest;
		else
			prp_atle += prp->p_count+change;

		if (prp_atl->lowest > prp->p_count+change)
			prp_atl->lowest = prp->p_count+change;

		space =	prp->p_space;
		vaddr = prp->p_vaddr + ptob(prp_atle - prp_atl->atl_data);

		for(;prp_atle != end; prp_atle++)
		{
			VASSERT(vaddr < prp->p_vaddr + ptob(prp->p_count));
			atl_deletetrans(prp, space, vaddr, 0);
			vaddr += NBPG;
		}
		if (space == KERNELSPACE)
		{
			purge_dcache_s();
			purge_tlb_super();
		}
		else
		{
			purge_dcache_u();
			purge_tlb_user();
		}

		/* if compaction was desired, it would go here */
	}
	else /* grow the region */
	{
		VASSERT(prp_atl);
		VASSERT(prp->p_count <= prp_atl->count);

		prp_atl_grow(prp, prp_atl, change);
	}

	VASSERT(prp_atl->count >= prp->p_count+change);
}

atl_swapi(prp)
preg_t *prp;
{
	atl_procattach(prp);
}

atl_swapo(prp)
preg_t *prp;
{
	prp_atl_destroy(prp);
}

atl_initkvm(prp)
preg_t *prp;
{
	atl_procattach(prp);
}
