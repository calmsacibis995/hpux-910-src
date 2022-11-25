/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/scan1.c,v $
 * $Revision: 1.37.83.3 $       $Author: root $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 16:31:57 $
 */

/*
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/



#include "inc.h"
#include "defs.h"
#include "types.h"
#include "externs.h"



#ifdef hp9000s800
/*MEMSTATS */
extern int sdldstack, svsize, stopstack, stopstack_dld;
#endif

/* REGION */


sproc_text()
{
	register struct proc *p, *unloc_p;
	register int i;
	preg_t *prp, *unloc_prp;
	vas_t *vas;

	if (suppress_default)
		return 0;
	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                     SCANNING PROC TABLE                             *\n");
	fprintf(outf,"***********************************************************************\n\n");
	for (p = &proc[0]; p < proc+nproc; p++) {

#ifdef	hp9000s800
		stopstack = 0;
		stopstack_dld = 0;
#endif

		/* break out of loop on sigint */
		if (got_sigint)
			break;

		if (p->p_stat == 0)
			continue;
#ifdef  hp9000s800
		/* Since proc[0] is treated a little differently, no
		   virtual to physical addr is needed */
		proc0 = ( p->p_pid == 0 ) ? 1 : 0;
#endif
		fprintf(outf,"\n");
		dumpproc(" ",p,proc,vproc);

		if ((p->p_flag & SLOAD) == 0) {
			fprintf(outf," swapped \n");
		} else {
			fprintf(outf," u. loaded:");
			if (getu(p)){
				fprintf_err();
				fprintf(outf," Error in u_area prevents further scan of this proc\n");
			}
		}

		unloc_p = unlocalizer(p, proc, vproc);

		/* Go down pregions */
		if (p->p_vas == 0) {
			fprintf_err();
			fprintf(outf," Process has no pregions!\n");
		} else {
			vas = GETBYTES(vas_t *, p->p_vas, sizeof(vas_t));
			if (vas == NULL) {
				fprintf(outf,"sproc_text: Localizing vas failed\n");
				return;
			}

			/*
			 * First display the vas.
			 */
			if (Pflg)
				dumpvas("sproc", p->p_vas, vas);

			/*
			 * Now display all of the pregions and regions.
			 */
			unloc_prp = vas->va_next;
			while(unloc_prp != (preg_t *)p->p_vas) {
				if (unloc_prp == (preg_t *) 0) {
					fprintf_err();
					fprintf(outf," Null pregion pointer\n");
					fprintf(outf," Terminating scan of pregions.\n");
					break;
				}
				prp = GETBYTES(preg_t *,unloc_prp,sizeof(preg_t));
				if (prp == NULL) {
					fprintf(outf,"sproc_text: Localizing prp failed\n");
					fprintf(outf," Terminating scan of pregions.\n");
					break;
				}
				if (prp->p_vas != p->p_vas) {
					fprintf(outf, "pregion at 0x%08x is garbage\n", unloc_prp);
					fprintf(outf, "p_vas is 0x%08x, should be 0x%08x\n",
					    prp->p_vas, p->p_vas);
					fprintf(outf," Terminating scan of pregions.\n");
					break;
				}
				tracepregion(p, prp, unloc_prp);
				unloc_prp = prp->p_next;
			}
		}


#ifdef hp9000s800
		/* MEMSTATS */
		if (stopstack !=0){
			fprintf(outf,"\n BaseStack 0x%x  Topstack 0x%x  Active size 0x%x  (virt 0x%x)\n", USRSTACK, stopstack, stopstack - USRSTACK, svsize*NBPG);
			if (stopstack < stopstack_dld)
				fprintf(outf," DldBaseStack 0x%X   TopDld stack 0x%x  Active dld size 0x%x\n", sdldstack, stopstack_dld, stopstack_dld - sdldstack);
			fprintf(outf,"\n");
		}
#endif

skip:
		if(vflg)fprintf(outf,"\n\n");
	}


}


scan_hashchains()
{
	register struct pfdat *pf;
	register int  j;

	if (suppress_default)
		return 0;
	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                     SCANNING HASH CHAINS                            *\n");
	fprintf(outf,"***********************************************************************\n\n");

	for(j = 0; j < phashmask+1; j++){
		/* scan down each hash chain. */

		pf = *(phash + j);
		if (pf == (struct pfdat *)0)
			continue;
		if (Hflg)
			fprintf(outf,"phash[0x%04x]:\n",j);

		for ( ; pf; pf = pf->pf_hchain) {

			pf = localizer(pf, pfdat, vpfdat);
			if ((pf  < &pfdat[firstfree]) || (pf > &pfdat[maxfree])){

				fprintf_err();
				fprintf(outf,"Bad pf_hchain field 0x%x, terminating scan\n",
					(pf - pfdat) + vpfdat);
				break;
			}
			if (Hflg){
				fprintf(outf,"      pfdat[0x%04x] devvp 0x%04x  blk 0x%08x\n",
					pf - pfdat, pf->pf_devvp, pf->pf_data);
			}
			if ((pf->pf_flags & P_HASH) == 0){

				fprintf_err();
				fprintf(outf,"P_HASH flag not set on page in hash chain.  phash[0x%04x]  pfdat[0x%04x]\n", j, pf - pfdat);
			}
		}


		if (Hflg) fprintf(outf,"\n");
	}
}

/* pr is local virtual address of the pregion */
/* preg is not localized pregion ptr */
tracepregion(p,pr,preg)
struct proc *p;
struct pregion *pr, *preg;
{
	struct region *r;

	allow_sigint = 1;

	if (Pflg)
		dumppreg("p_region",pr, preg);

	/* Localize the pointer */
	/*r = GETBYTES(reg_t *, pr->p_reg, sizeof(reg_t ));*/

	traceregion(p, pr->p_reg, pr);

	allow_sigint = 0;

	return(0);

}


/* r not localized yet */
traceregion(p, r, pr)
struct proc *p;
struct region *r;
struct pregion *pr;
{

	struct region *rp;
	int i, type, numvfds;
	u_long virt;
	vfd_t *vfd;
	dbd_t *db;


	rp = GETBYTES(reg_t *, r, sizeof(reg_t ));
	if (Pflg)
		dumpreg(" region", rp, r);

	/* if vfds and dbds are not swapped */
#ifdef  hp9000s800
	if (proc0)
		return(0);
#endif
	if (rp->r_dbd == 0 ) {
		/* num of vfds for pregion */
		numvfds = pr->p_count+pr->p_off;
		/* account for vfd and dbd resources */
		for (i = pr->p_off; i < numvfds; i++) {
			/* Sanity check */
			if (numvfds > 0x1000){
				fprintf(outf,"traceregion error: numvfds > 0x1000 ( 0x%x )\n",numvfds);
				fprintf(outf,"p_off 0x%x  p_count 0x%x\n",pr->p_count, pr->p_off);
				break;
			}
			virt = (u_long)(pr->p_vaddr + ptob(i - pr->p_off));
			vfd = (vfd_t *)findvfd(rp, i);
			if (vfd != NULL) {
				db = (dbd_t *)finddbd(rp, i);
				if (db != NULL)
					logvfd(p, vfd, db, 0, virt, pr, 0);
			}
		}
	}

	return(0);
}

followpregion(p,pr)
struct proc *p;
struct pregion *pr;
{
	struct region *r;
	struct pregion *npregptr, *pregptr;

	/* Go down pregions */
	npregptr = pr;
	do{
		pregptr = GETBYTES(preg_t *, npregptr, sizeof(preg_t));
		if (pregptr == NULL) {
			fprintf(outf, "followpregion: localizing pregion failed\n");
			return(1);
		}
		dumppreg("p_region", pregptr, npregptr);
		if (pregptr->p_reg == NULL) {
			fprintf(outf,"followpregion: no region is associated with this pregion.\n");
			return(1);
		}
		r = GETBYTES(reg_t *, pregptr->p_reg, sizeof(reg_t));
		if (r == NULL) {
			fprintf(outf, "followpregion: localizing region failed\n");
			return(1);
		}
		dumpreg(" region",r, pregptr->p_reg);
		npregptr = pregptr->p_ll.lle_next;
	} while ((preg_t *)p->p_vas != npregptr);

	fprintf(outf,"\n\n");
	return(0);
}

/* r is the local virtual address of the region */

followregion(p, r, pr)
struct proc *p;
struct region *r;
struct pregion *pr;
{

	int i, type, virt;
	vfd_t *vfd;
	dbd_t *db;

	fprintf(outf,"\n");
	dumpreg(" region",r);

	return(0);
}

/* REG300, must convert these to visr_freemem if using get() , careful on
   reads Must remove pdir stuff */
#ifdef  hp9000s800
/* isrfreepool reports on page inconsistencies in the ISR Free Memory
 * page pool.
 */
isrfreepool()
{
	struct paginfo *zp;
	int i, space, virt, skip, pagenum;
	struct pfdat isr_memlist, *cur, *next, *prev;
	int isr_freemem;
	int aisr_freemem = lookup("isr_freemem");
	int aisr_memlist = lookup("isr_memlist");

	if (suppress_default)
		return 0;
	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                       SCANNING ISR FREE MEMORY POOL                 *\n");
	fprintf(outf,"***********************************************************************\n\n");
	if (!aisr_freemem) {
		fprintf_err();
		fprintf(outf,"Can't find isr_freemem in symbol table\n");
		return;
	}
	if (!aisr_memlist) {
		fprintf_err();
		fprintf(outf,"Can't find isr_memlist in symbol table\n");
		return;
	}

		fprintf(outf,"Can't scan isr_freemem \n");

}
#else
isrfreepool()
{}
#endif

/* REG300, must remove pdir refs */

/* Coresummary reports on pages inconsistencies in the pfdat, and dumps the
 * entire map if the Cflg is set. (use to be summary)
 */
#ifdef  hp9000s800
coresummary()
{
	register int i;
	register struct paginfo *zp;
	register int csys, skip;
	register  int lostcount;
	register struct pfdat;
	int virt, space;
	struct pfdat *pf;
	struct hpde *pde, *pd;
	extern	struct hpde *base_pdir;
	int is_odd;
	int cpd_bits;

	if (suppress_default)
		return 0;
	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                       SCANNING PFDAT                                *\n");
	fprintf(outf,"***********************************************************************\n\n");
	lostcount = 0;
	for (i = firstfree ; i <= maxfree; i+= 1) {
		skip = 0;
		csys = 0;
		zp = &paginfo[i];
		/* Page unaccounted for */
		if (zp->z_type == ZLOST){
			pd = pgtopde_table[i];

			if (base_pdir == NULL) {
				cpd_bits = ((((int)pd) & 0x1E0) >> 4);
			} else {
				cpd_bits = ((((int)pd) & 0x1E));
			}

			/* See if odd and convert to pointer */
			if ((int)pd && 0x1){
				is_odd = 1;
			} else {
				is_odd = 0;
			}
			pd = (struct hpde *)((int)pd & ~0x1F);

			if ((pd >= vhtbl) && (pd < &vhtbl[nhtbl])){
				pd = &htbl[pd - vhtbl];
			} else if ((pd >= vpdir) && (pd < &vpdir[nhtbl])){
				pd = &pdir[pd - vpdir];
			} else {
			if (vflg)
				fprintf(outf,"pfdat scan: page 0x%x not mapped in pgtopde_table\n", i);
				pd = 0;
			}
			if (pd){
				if (is_odd) {
					virt=ptob(((pd->pde_page_o<<5)|cpd_bits|1));
					space = pd->pde_space_o;
				}
				else {
					virt=ptob((pd->pde_page_e<<5)|cpd_bits);
					space = pd->pde_space_e;
				}


				/* If its mapped to the kernel we will accept
				 * that . It use to also require it to be equiv
				 * mapped.
				 */
				if ((virt < 0x40000000) && (space== KERNELSPACE)
					&& (ltor(space, virt) != 0))
					csys = 1;
			}
			if((csys == 0) || (Cflg)){
				lostcount++;
				skip++;

				dumppfdat("lost", &pfdat[i], pfdat, vpfdat);
				fprintf(outf,"      WARNING:\n");
				fprintf(outf,"      pfdat page not claimed by anyone (unaccounted for)\n");
				fprintf(outf,"      could be page from kdalloc\n");
			}
		}
		/* Check for pages that are claimed, but not mapped by the
		 * pdir (note pages must not be in the freelist waiting to
		 * be reclaimed).
		 * This goes away with regions because a copy on write page
		 * could suffer this circumstance if the parent exited without
		 * touching the page and the child has not touched the page
		 * either.
		 */

		/*
		if (zp->z_type != ZLOST && zp->z_type != ZISRFREE
				&& zp->z_pdirtype == ZINVALID) {

			if (((pfdat +  i)->pf_flags & P_QUEUE) == 0){
				skip++;

				fprintf_err();
				dumppfdat("invalid",&pfdat[i], pfdat, vpfdat);

				fprintf(outf,"      pfdat claimed, but not mapped by pdir\n");
			}
		}*/

		/* Check that all P_HASH pages are on a hash queue */

		pf = &pfdat[i];
		if (pf->pf_flags & P_HASH) {

			if (pffinder(pf->pf_devvp, pf->pf_data) != pf) {
				skip++;

				fprintf_err();
				dumppfdat("invalid P_HASH", pf, pfdat, vpfdat);
				fprintf(outf,"      pfdat has P_HASH, but is not in hash chains\n");
			}
		}

		/* NO EQUIVALENT YET
		if (cmap[pfnum].c_lock && cmap[pfnum].c_type != CSYS){
			skip++;
			dumpcm("locked", i);
			fprintf (outf,"      core map page locked, but does not belong to CSYS\n");
		}
		*/

		/* Only write the page out once */
		if ((Cflg) && (skip == 0)){
			skip++;
			dumppfdat("mem",&pfdat[i], pfdat, vpfdat);
		}
		if (skip)
			fprintf(outf,"\n");
	}
	if (lostcount > 0){
		fprintf(outf," WARNING lost pages (%d) > 0 !! (use C option)\n",lostcount);
	}

	/* MEMSTATS */
	fprintf(outf,"\n Summary of pages which could be accounted for\n");
	for (i = 0 ; i <= ZMAX; i ++){
		fprintf(outf," Page count 0x%04x (0x%x) for type %s \n",
			pfdat_types[i], pfdat_types[i] * NBPG , typepg[i]);
	}
	fprintf(outf,"\n\n");
}
#else
coresummary()
{
}
#endif


/* freelist scans down the freelist of the pfdat, printing out
 * inconsistencies
 */
freelist()
{
	register int i, skip, pagenum;
	register struct pfdat *next, *prev, *head, *cur;


	if (suppress_default)
		return 0;
	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                     SCANNING FREELIST                               *\n");
	fprintf(outf,"***********************************************************************\n\n");


	/* head = localizer(vphead, pfdat, vpfdat); */
	head = &phead;
	cur = head;
	/* check for empty freelist up front */
	if (cur->pf_next  == vphead) {
		fprintf(outf,"Warning: \n");
		fprintf(outf,"\n   Empty freelist!! Scan terminated.\n");
		return;
	}
	else if (cur->pf_next  == (struct pfdat *)0) {
		fprintf(outf,"Warning: \n");
		fprintf(outf,"\n   Pfdat pf_next field null!! Scan terminated.\n");
		return;
	}
	next = localizer(phead.pf_next, pfdat, vpfdat);
	for (i=freemem; ; i--) {
		skip = 0;
		prev = cur;
		cur = next;
		if ( cur->pf_next  == vphead ) {
			break;	/* We have gone around once; end of list */
		}
		else if (cur->pf_next  == (struct pfdat *)0) {
			fprintf(outf,"Warning: \n");
			fprintf(outf,"\n   Pfdat pf_next field null!! Scan terminated.\n");
			break;
		}
		next = localizer(cur->pf_next,pfdat, vpfdat);

		/* if (i == 0) {
			fprintf_err();
			fprintf(outf,"\n   Too many entries on freelist!!\n");
		} */

		if (i < -10000) {
			fprintf_err();
			fprintf(outf,"Infinite loop in freelist? Giving up!\n");
			break;	/* infinite loop? */
		}

		/* pftopg */
		pagenum = cur - pfdat;
		if ((pagenum < firstfree) || (pagenum > maxfree)) {
			skip++;

			fprintf_err();
			fprintf(outf,"free list link out of range: \n");
			dumppfdat("bad link (pf_next) in", prev, pfdat, vpfdat);
			fprintf(outf,"   Scan of freelist terminating early");
			break;
		}
		if ((cur->pf_flags & P_SYS)) {
			skip++;

			fprintf_err();
			fprintf(outf,"kdalloced page not kdfreed before put on freelist:\n" );
			dumppfdat("non-freed page in", prev, pfdat, vpfdat);
		}

		/* pftopg */
		if ((cur->pf_flags & P_QUEUE) == 0){
			skip++;

			fprintf_err();
			fprintf(outf,"link to non free block:\n" );
			dumppfdat("bad link (pf_next) in", prev, pfdat, vpfdat);
			dumppfdat("to non free block", cur, pfdat, vpfdat);
		}
		if (cur->pf_use != 0) {
			skip++;

			fprintf_err();
			fprintf(outf,"free list entry has nonzero use field\n");
			dumppfdat("bad use ", cur, pfdat, vpfdat);
		}

		paginfo[cur - pfdat].z_type = ZFREE;
		if ((Fflg) && (skip == 0)){
			skip++;
			dumppfdat("free", cur, pfdat, vpfdat);
		}
		if (skip)
			fprintf(outf,"\n");
	}

	if (skip)
		fprintf(outf,"\n");

	if (i > 0){

		fprintf(outf,"Warning: \n");
		fprintf(outf,"\n   Scan terminated early!!\n");
		fprintf(outf,"   Number of entries on freelist < freemem. freemem=%d  entries=%d\n", freemem, freemem - i);
	}

	if (i < 0){

		fprintf_err();
		fprintf(outf,"\n   Too many entries on freelist!!\n");
		fprintf(outf,"   Number of entries on freelist > freemem. freemem=%d  entries=%d\n", freemem, freemem - i);
	}
}

dmcheck()
{
	struct swaptab *chunk;
	int i, j;
	int free_cnt;
	int nxt;
	int bad_swapmap;
	struct swapmap swapmap[NPGCHUNK], *aswpmp;
	struct swdevt *addr, *st_dev_h, *st_dev_t;
	int length = NPGCHUNK * sizeof(struct swapmap);

	if (!(dflg))
		return(0);

	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                     SCANNING SWAP SPACE                             *\n");
	fprintf(outf,"***********************************************************************\n");

	/*
	 * We are checking three things in swaptab:
	 * 	1. st_nfpgs in a swapchunk should match with number of
	 *         free pages in swapmap.
	 *      2. st_free should be the index of the first free page.
	 *      3. use count of each free page on the free list in
	 *         swapmap should be 0.
	 */

	for (i = 0; i < MAXSWAPCHUNKS; i++) {
		int rmchunk;

		/*
		 * Print the data for this swap chunk.
		 */
		chunk = &swaptab[i];
		if (chunk->st_site != my_site &&
		    chunk->st_free == -1 && chunk->st_nfpgs == 0) {
		    fprintf(outf,
			"\n swaptab[%d]: (allocated to cnode %d)\n",
			i, chunk->st_site);
		    rmchunk = 1;
		}
		else {
		    fprintf(outf, "\n swaptab[%d]:\n", i);
		    rmchunk = 0;
		}

		fprintf(outf,
		    "  st_free  %10d  st_next  %10d  st_nfpgs %10d\n",
		    chunk->st_free,  chunk->st_next, chunk->st_nfpgs);
		fprintf(outf,
		    "  st_site  %10d  st_dev   0x%08x  st_fsp   0x%08x\n",
		    chunk->st_site, chunk->st_dev, chunk->st_fsp);
		fprintf(outf,
		    "  st_vnode 0x%08x  st_swpmp 0x%08x\n",
		    chunk->st_vnode, chunk->st_swpmp);
		fprintf(outf,
		    "  st_start/st_swptab 0x%08x",
		    chunk->st_union.st_start);

#define ST_ELSE (~(ST_FREE|ST_INDEL))

		if (chunk->st_flags & ST_ELSE)
			fprintf(outf, " ST_0x%08x", chunk->st_flags & ST_ELSE);
		if (chunk->st_flags & ST_FREE)
			fprintf(outf, " ST_FREE");
		if (chunk->st_flags & ST_INDEL)
			fprintf(outf, " ST_INDEL");
		fprintf(outf, "\n");

		/*
		 * Now perform consistency checks...
		 */
		if (chunk->st_swpmp == 0) {
			if (chunk->st_nfpgs == 0)
				continue;
			else {
				fprintf(outf,
				    "dmcheck: swaptab[0x%x] is zero but not its free page count!\n",
				    i);
				return(0);
			}
		}

		aswpmp = (struct swapmap *)ltor(0, chunk->st_swpmp);
		if (aswpmp == 0) {
			fprintf(outf, "dmcheck: ltor failed on swapmap\n");
			return(0);
		}
		if (longlseek(fcore, (long)aswpmp, 0) == -1) {
			fprintf(outf, "dmcheck: lseek failed\n");
			return(0);
		}
		if (longread(fcore, &swapmap[0], length) != length) {
			fprintf(outf, "dmcheck: longread failed\n");
			return(0);
		}

		/*
		 * walk thru the free list in swapmap.  Make sure that
		 * we do not cause an infinite loop by bounding the
		 * traversal using NPGCHUNK.
		 */
		bad_swapmap = 0;
		nxt = chunk->st_free;
		free_cnt = 0;
		while (nxt != -1 && free_cnt <= NPGCHUNK) {
			if (swapmap[nxt].sm_ucnt == 0) {
				free_cnt++;
				nxt = swapmap[nxt].sm_next;
			}
			else {
				fprintf(outf, "\n  inconsistency found in swapmap[%d]\n",
					nxt);
				bad_swapmap = 1;
				break;
			}
		}

		/*
		 * Verify free count.
		 */
		if (free_cnt != chunk->st_nfpgs) {
			fprintf(outf,
			    "\n  free count mismatch on swaptab[%d].st_nfpgs: %d != %d\n",
			    i, chunk->st_nfpgs, free_cnt);
			bad_swapmap = 1;
		}

		/*
		 * Verify that (st_free == first free in swapmap)
		 * NOTE Exception:
		 *   If we are a swap server, this chunk may be in use
		 *   by another diskless client.  If so, then the
		 *   st_free must be -1 and st_nfpgs must be 0.
		 *   (see above)
		 */
		if (!rmchunk) {
			for (j = 0; j < NPGCHUNK; j++) {
				if (swapmap[j].sm_ucnt == 0)
					break;
			}
			if (j == NPGCHUNK)
				j = -1;
			if (j != chunk->st_free) {
				fprintf(outf,
					"\n  first free mismatch on swaptab[%d].st_free: %d != %d\n",
					i, chunk->st_free, j);
				    bad_swapmap = 1;
			}
		}

		/*
		 * If we found an inconsistency, print out the swapmap.
		 */
		if (bad_swapmap) {
			for (j = 0; j < NPGCHUNK; j++) {
			    if ((j % 8) == 0)
				fprintf(outf, "\n  map[%3d]:", j);
			    fprintf(outf, " %3d.%3d",
				swapmap[j].sm_ucnt, swapmap[j].sm_next);
			}
			fprintf(outf, "\n");
		}
	}
	fprintf(outf, "\n");

	/*
	 * We are checking three things in swdevt:
	 * 	1. address of swdevt[i] should equal to
	 *	   swaptab[sw_head].st_dev, and also equal to
	 *         swaptab[sw_tail].st_dev.
	 *      2. sum of st_nfpgs for this device should equal to
	 *         sw_nfpgs.
	 *      3. swaptab[sw_tail].next should = to -1.
	 *
	 */
	for (i = 0; i < NSWAPDEV; i++) {
		if (swdevt[i].sw_dev == -1)
			continue;
		addr = localizer(&swdevt[i], vswdevt, &swdevt[0]);
		st_dev_h = swaptab[swdevt[i].sw_head].st_dev;
		st_dev_t = swaptab[swdevt[i].sw_tail].st_dev;
		if (addr != st_dev_h || addr != st_dev_t ) {
			fprintf(outf,"&swdevt[0x%x] != st_devs in swaptab 0x%08x 0x%08x\n",
			i, st_dev_h, st_dev_t);
		} else {
			j = swdevt[i].sw_head;
			free_cnt = 0;
			while ( j >= 0 && (j <= swdevt[i].sw_tail) ) {
				free_cnt += swaptab[j].st_nfpgs;
				if ( swaptab[j].st_next == -1  &&
				     j < swdevt[i].sw_tail ) {
					fprintf(outf,"swdevt[0x%x].sw_tail doesn't agree with swaptab\n", i);
					break;
				} else
					j = swaptab[j].st_next;

			}
			if (free_cnt != swdevt[i].sw_nfpgs) {
				fprintf(outf,"sum of free pages in swdevt[0x%x] doesn't agree with swaptab", i);
				fprintf(outf," 0x%08x != 0x%08x\n", swdevt[i].sw_nfpgs, free_cnt);
			}
		}
	}
}


shmem_table()
{
	register struct shmid_ds *sp;
	register struct region *r;
	register struct pregion *prp;

	if (suppress_default)
		return 0;
	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                     SCANNING SHMEM TABLE                            *\n");
	fprintf(outf,"***********************************************************************\n\n");
	for (sp = shmem; sp < &shmem[shminfo.shmmni]; sp++){
		vas_t *vas;

		if ((sp->shm_perm.mode & IPC_ALLOC) == IPC_ALLOC){
			if (Sflg)
				dumpshmid(" ",sp,shmem,vshmem);

			vas = GETBYTES(vas_t *, sp->shm_vas, sizeof(vas_t));
			prp = GETBYTES(preg_t *, vas->va_next, sizeof(preg_t));
			/* r = GETBYTES(reg_t *, prp->p_reg, sizeof(reg_t)); */
			traceregion(&proc[0], prp->p_reg, 0);

		}
	}
}

/* This is the sorting algorithm given to qsort in sysmapcheck */
syssort(d, e)
	register struct sysblks *d, *e;
{

	return (e->s_first - d->s_first);
}

/* Account for all free blocks left in sysmap (not yet allocated).
 * Sort all entries, then search for missing pieces, and overlaps.
 * Note the current rmfree() routine in the kernel can drop pieces
 */
sysmapcheck()
{
	register struct mapent *smp;
	register struct sysblks *d, *e;
	int s_total;

	if (!(dflg))
		return(0);

	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                     SCANNING SYSMAP                                 *\n");
	fprintf(outf,"***********************************************************************\n\n");
	/* Fill in remaining entries */
	/* The first entry is the limit and name */
	smp = (struct mapent *)&sysmap[0];
	/*
	if (Dflg)
		fprintf(outf," m_limit 0x%x \n", smp->m_addr);
	*/
	nsblks = 0;	/* clear sblks[] for sysmapuse() */
	for (smp = (struct mapent *)&sysmap[1]; smp->m_size; smp++){
		if (smp > (struct mapent *)&sysmap[SYSMAPSIZE]){
			fprintf(outf," sysmap runs off the end\n");
			break;
		}
		/*
		if (Dflg)
			fprintf(outf," m_addr 0x%x m_size 0x%x \n",
				smp->m_addr, smp->m_size);
		*/
		sysmapuse(smp->m_addr, smp->m_size, SFREE, 0);
	}

	s_total = 0;
	fprintf(outf,"Number of free chunks in sysmap = 0x%x\n\n",nsblks);
	qsort(sblks, nsblks, sizeof (struct sysblks), syssort);
	d = &sblks[nsblks - 1];
	if (d->s_first > 1){
		fprintf(outf,"sysmap HOLE:\t\t\t\t  start 0x%04x size 0x%04x\n",
			1, d->s_first);
	}
	for (;; d--) {
		if (Dflg) {
			fprintf(outf,"sysmap chunk: start 0x%04x size 0x%04x\n",
				d->s_first, d->s_size);
		}
		e = d - 1;
		s_total += d->s_size;

		/* see if time to abort the loop */
		if (e < (struct sysblks *) &sblks[0])
			break;

		if (d->s_first + d->s_size > e->s_first) {

			fprintf_err();
			fprintf(outf,"overlap in sysmap mappings:\n");

		} else if (d->s_first + d->s_size < e->s_first) {

			fprintf(outf,"sysmap HOLE:\t\t\t\t  start 0x%04x size 0x%04x\n",
			    d->s_first + d->s_size,
			    e->s_first - (d->s_first + d->s_size));
		}
	}

	fprintf(outf,"\nTotal virtual space contained in sysmap = 0x%x\n",s_total);

}
