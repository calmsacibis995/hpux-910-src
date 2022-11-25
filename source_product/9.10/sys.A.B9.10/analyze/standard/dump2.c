/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/dump2.c,v $
 * $Revision: 1.45.83.4 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/13 11:04:46 $
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

extern struct vfddbd * findentry();
extern caddr_t getbytes();




/* dump() dumps a paginfo page out */
dump(zp)
	struct paginfo *zp;
{
	fprintf(outf,"   Old info: core page 0x%08x  type %s  pid %d  vfd 0x%08x\n",
		zp - paginfo, typepg[zp->z_type], zp->z_pid, zp->z_vfd);

	fprintf(outf,"	     space     0x%08x  virt_page 0x%08x  protid 0x%06x\n",
		zp->z_space,zp->z_virtual,zp->z_protid);
#ifdef hp9000s800
	fprintf(outf,"	     pdirtype    %08s  hashndx 0x%08x\n",
		typepdir[zp->z_pdirtype],zp->z_hashndx);
#endif
}


dumpaddress()
{
	register struct nlist *np;
	int i = 1;

	fprintf(outf, "\n");
	fprintf(outf, " Address:\n");
	for (np = nl; np->n_name && *np->n_name; np++, i++) {
		if (strcmp(np->n_name, "UNUSED") == 0) {
			i--;
			continue;
		}
#ifdef MP
	        if (check_obsolete(np->n_name)) {
			i--;
			continue;
		}
#endif
		fprintf(outf, " %8.8s: 0x%08x  ", np->n_name, np->n_value);
		if (i == 3) {
			i = 0;
			fprintf(outf, "\n");
		}
	}
#ifdef NETWORK
	i = ptydump_address(i);
	i = netdump_address(i);
#endif NETWORK
	fprintf(outf, "\n");
}

#ifdef MP
check_obsolete(name)
char *name;
{
       /* Check if this location has been obsolete in MP */
       /* Actually, we could do this by carefully ifdef the nlist
	  table in al_nlist.h. But then we will also need to change
	  the indices in al_nldefs.h.  This may become messy very
	  quick */

       if (!strcmp(name,"rpb")) return(1);
       if (!strcmp(name,"spl_word")) return(1);
       if (!strcmp(name,"spl_lock")) return(1);
       if (!strcmp(name,"semaphore_log")) return(1);
       if (!strcmp(name,"u")) return(1);
       if (!strcmp(name,"cur_proc")) return(1);
       if (!strcmp(name,"crash_processor_table")) return(1);
       if (!strcmp(name,"crash_event_table")) return(1);
       if (!strcmp(name,"panic_save_state")) return(1);
       if (!strcmp(name,"panic_second_save_state")) return(1);
       return(0);
}
#endif

dumpqs(qptr)
struct prochd *qptr;
{
	register int i;
	struct proc *procptr;

	if (!Qflg)
		return;
	fprintf(outf," Run queues: ");
	for (i = 0; i < NQS; i++, qptr++){
		fprintf(outf," qs[%3d]  0x%07x  0x%07x  ",i,qs[i].ph_link,
		qs[i].ph_rlink);
		if ((int)(qs[i].ph_rlink) == (int)(aqs + i)){
			fprintf(outf, " empty ");
			fprintf(outf,"\n");
		} else {
			procptr = localizer(qs[i].ph_link, proc, vproc);
			for (;;){
				fprintf(outf," proc[%d]:p_pri 0x%x ", procptr - proc, procptr->p_pri);
				if ( (int)(procptr->p_link) ==
					(int)(aqs + i)){
					break;
				}
				if ( (int)(procptr->p_link) ==  0){
					fprintf(outf," p_link == 0 ");
					break;
				}

				procptr = localizer(procptr->p_link,proc,vproc);
			}
			fprintf(outf,"\n");
		}

	}
	fprintf(outf,"\n");
}

#ifdef hp9000s800
dumpuidhx()
{
	register int i, format;
	struct proc *procptr;

	if (!Xflg)
		return;
	for (i = 0; i < UIDHSZ; i++){
		fprintf(outf,"uidhash[%2d] %3d ",i,uidhash[i]);
		if (uidhash[i] == 0){
			fprintf(outf, "empty ");
			fprintf(outf,"\n");
		} else {
			procptr = &proc[ uidhash[i] ];
			for (format=0;;format++){
				if ((format % 3) == 0) fprintf(outf,"\n   ");
				fprintf(outf," proc[%3d]:p_uidhx %3d  ",
					procptr - proc, procptr->p_uidhx);
				if ((int)(procptr->p_uidhx) ==  0) {
					break;
				}

				if ((procptr->p_uidhx < 0) || (procptr->p_uidhx
					> nproc)){
						fprintf(outf," p_uidx out of range\n");
						break;
				}
				procptr = &proc[ procptr->p_uidhx ];
			}
			fprintf(outf,"\n\n");
		}

	}
	fprintf(outf,"\n");
}
#endif



/* hexdump was stolen from the networking trace formatter */
/* BEGIN_IMS hexdump *
 ********************************************************************
 ****
 ****			hexdump( buf, size, start, offset )
 ****
 ********************************************************************
 * Input Parameters
 *	buf			buffer containing the data to drop
 *	size			number of bytes of data to dump (- start)
 *	start			byte offset in data to start dumping
 *	offset			offset for address
 * Output Parameters
 *	none
 * Return Value
 *	none
 * Description
 *	print out a buffer in hexadecimal, with printable ASCII
 *	equivalent on the side.  See "format_packet" for a picture.
 *
 * External Calls
 *	printf
 * Called By (optional)
 *	format_packet
 ********************************************************************
 * END_IMS hexdump */

#define LINESIZE	16

hexdump(buf, size, start, offset)
unsigned char *buf;
int  	size;
int	start;
int	offset;
{
    register int i;
    register char charbuf[LINESIZE+1];		/* holds ASCII equivalent */
    register int pos;

    charbuf[LINESIZE] = '\0';
    i = start;
    while ( i < size ) {
	pos = (i-start) % LINESIZE;

	if (pos == 0)
	    fprintf(outf,"  %4d:  ", offset + (i - start));

	fprintf(outf,"%02x ", buf[i]);
	if (buf[i] < ' ' || buf[i] > '~')
	    charbuf[pos] = '.';
	else
	    charbuf[pos] = buf[i];

	if (pos == LINESIZE-1)
	    fprintf(outf,"    %s\n", charbuf );
	i++;
    }
    if (pos != LINESIZE-1) {
	for (pos = (i-start) % LINESIZE; pos < LINESIZE; pos++) {
	    fprintf(outf,"-- ");
	    charbuf[pos] = '.';
	}
	fprintf(outf,"    %s\n", charbuf );
    }
}			/***** hexdump *****/



dumphex(space, addr, length)
int space;
int *addr;
int length;
{
	register int phyaddr, i;
	int wbuf[4], offset;

	for (offset = i = 0; i < length; i++){
		if ((i % 4) == 0) {
		    phyaddr = ltor(space, addr++);
		    if (phyaddr == 0) {
			fprintf(outf," 0x%x address not mapped\n",addr - 1);
			break;
		    }
		    wbuf[(i % 16) / 4] = getreal(phyaddr);
		}
		if ((i+1) % 16 == 0) {
			hexdump((char *)wbuf, 16, 0, offset);
			offset += 16;
		} else if (i+1 == length)
			hexdump((char *)wbuf, i-offset+1, 0, offset);
	}
}

dumpsysmap(cp, sp, ls, vs)
	char *cp;
	struct mapent *sp, *ls, *vs;

{
	fprintf(outf,"entry[ %03d]   m_addr 0x%08x  m_size 0x%08x\n",
	sp - ls, sp->m_addr , sp->m_size);

}

#ifdef LATER
dumpseglist(cp, sp, ls, vs)
	char *cp;
	struct seglist *sp, *ls, *vs;
{
	fprintf(outf," [%03d] flgs 0x%04x  at 0x%08x  size 0x%06x  next 0x%06x last 0x%06x\n",
	sp - ls, sp->flags, sp->atpoint, sp->size, sp->next , sp->last);
}
#endif LATER


dumpshmid(cp, sp, ls, vs)
	char *cp;
	struct shmid_ds *sp, *ls, *vs;

{
	fprintf(outf,"Shared memory entry : %s   shmem address 0x%08x\n",
		cp,( vs + (sp-ls)));
	fprintf(outf,"   ipc_mode 0x%04.4x  ipc_key 0x%08x  ipc_cuid 0x%04.4x\n",
		sp->shm_perm.mode ,sp->shm_perm.key, sp->shm_perm.cuid);
	fprintf(outf,"   shm_segsz 0x%08x  shm_lpid 0x%08x  nattch 0x%04.4x cnattch 0x%04.4x\n",
		sp->shm_segsz, sp->shm_lpid,sp->shm_nattch,sp->shm_cnattch);
	fprintf(outf,"   shm_vas   0x%08x  shm_cpid 0x%08x\n",
		sp->shm_vas, sp->shm_cpid);

}

/*
 * dumpvas(cp, vas_addr, vas) --
 *    Display a vas, vas_addr is the real address, vas is the
 *    copy in our data space.
 */
dumpvas(cp, vas_addr, vas)
	char *cp;
	vas_t *vas_addr;
	vas_t *vas;
{
	fprintf(outf,"\nVas:  vas address is 0x%08x\n", vas_addr);
	fprintf(outf, "   va_next    0x%08x va_prev   0x%08x va_refcnt %10d\n",
		vas->va_next, vas->va_prev, vas->va_refcnt);
	fprintf(outf, "   va_rss     %10d va_prss   %10d va_swprss %10d\n",
		vas->va_rss, vas->va_prss, vas->va_swprss);
	fprintf(outf, "   va_flags   0x%08x va_fp     0x%08x va_wcount %10d\n",
		vas->va_flags, vas->va_fp, vas->va_wcount);
#ifdef __hp9000s800
	fprintf(outf, "   va_proc    0x%08x v_hdlflags    0x%04x hdl_q2sid 0x%08x\n",
		vas->va_proc, vas->va_hdl.v_hdlflags, vas->va_hdl.hdl_q2sid);
#else
	fprintf(outf, "   va_proc    0x%08x va_seg    0x%08x va_vsegs  0x%08x\n",
		vas->va_proc, vas->va_hdl.va_seg, vas->va_hdl.va_vsegs);
	fprintf(outf, "   va_attach_hint  0x%08x\n",
		vas->va_hdl.va_attach_hint);
#endif

#define VA_ELSE (~(VA_HOLES|VA_IOMAP|VA_NOTEXT))

	if (vas->va_flags & VA_ELSE)
		fprintf(outf," VA_0x%08x", vas->va_flags & VA_ELSE);

	if (vas->va_flags & VA_HOLES)
		fprintf(outf," VA_HOLES");
	if (vas->va_flags & VA_IOMAP)
		fprintf(outf," VA_IOMAP");
	if (vas->va_flags & VA_NOTEXT)
		fprintf(outf," VA_NOTEXT");

	fprintf(outf,"\n");
}

/* dump pregion */
/* pr is localized and preg is not */
dumppreg(cp, pr, preg )
	char *cp;
	struct pregion *pr, *preg;
{
	char *str;

	fprintf(outf,"\nPregion:  pregion address is 0x%08x\n", preg);
	fprintf(outf, "   p_next     0x%08x p_prev    0x%08x p_flags   0x%08x\n",
		pr->p_next, pr->p_prev, pr->p_flags);
	fprintf(outf, "   p_type     0x%08x p_reg     0x%08x p_space   0x%08x\n",
		pr->p_type, pr->p_reg, pr->p_space);
	fprintf(outf, "   p_vaddr    0x%08x p_off     %10d p_count   %10d\n",
		pr->p_vaddr, pr->p_off, pr->p_count);
	fprintf(outf, "   p_prot     0x%08x\n", pr->p_prot);
	fprintf(outf, "   p_ageremain  %8d p_agescan %10d p_stealscan %8d\n",
		pr->p_ageremain, pr->p_agescan, pr->p_stealscan);
	fprintf(outf, "   p_vas      0x%08x p_forw    0x%08x p_back    0x%08x\n",
		pr->p_vas, pr->p_forw, pr->p_back);
	fprintf(outf, "   p_prpnext  0x%08x p_prpprev 0x%08x\n",
		pr->p_prpnext, pr->p_prpprev);
#ifdef  hp9000s800
	fprintf(outf, "   p_hdlflags 0x%08x p_hdlar   0x%08x p_hdlprot 0x%08x\n",
		pr->p_hdl.hdlflags, pr->p_hdl.hdlar, pr->p_hdl.hdlprot);
#else
	fprintf(outf, "   p_hdlflags 0x%08x p_ntran   0x%08x p_physpfn 0x%08x\n",
		pr->p_hdl.p_hdlflags, pr->p_hdl.p_ntran, pr->p_hdl.p_physpfn);
#endif

	/*
	 * Dump out mprotect data for this pregion.
	 */
	dump_mprotect(pr->p_hdl.p_spreg);

	switch (pr->p_type) {
	case PT_UNUSED:
		str = "PT_UNUSED";	break;
	case PT_UAREA:
		str = "PT_UAREA";	break;
	case PT_TEXT:
		str = "PT_TEXT";	break;
	case PT_DATA:
		str = "PT_DATA";	break;
	case PT_STACK:
		str = "PT_STACK";	break;
	case PT_SHMEM:
		str = "PT_SHMEM";	break;
	case PT_NULLDREF:
		str = "PT_NULLDREF";	break;
	case PT_LIBTXT:
		str = "PT_LIBTXT";	break;
	case PT_LIBDAT:
		str = "PT_LIBDAT";	break;
	case PT_SIGSTACK:
		str = "PT_SIGSTACK";	break;
	case PT_IO:
		str = "PT_IO";		break;
	case PT_MMAP:
		str = "PT_MMAP";	break;
	case PT_GRAFLOCKPG:
		str = "PT_GRAFLOCKPG";	break;
	default:
		str = NULL;		break;
	}

	if (str == NULL)
		fprintf(outf," PT_0x%08x ", pr->p_type);
	else
		fprintf(outf," %s ", str);

#define PF_ELSE (~(PF_ALLOC|PF_MLOCK|PF_EXACT|PF_ACTIVE|	\
		   PF_NOPAGE|PF_NOMAP|PF_PUBLIC|PF_DAEMON|	\
		   PF_WRITABLE|PF_INHERIT|PF_VTEXT|PF_MMFATTACH))

	if(pr->p_flags & PF_ELSE)
		fprintf(outf," PF_0x%04x", pr->p_flags & PF_ELSE);

	if(pr->p_flags & PF_ALLOC)
		fprintf(outf," PF_ALLOC");
	if(pr->p_flags & PF_MLOCK)
		fprintf(outf," PF_MLOCK");
	if(pr->p_flags & PF_EXACT)
		fprintf(outf," PF_EXACT");
	if(pr->p_flags & PF_ACTIVE)
		fprintf(outf," PF_ACTIVE");
	if(pr->p_flags & PF_NOPAGE)
		fprintf(outf," PF_NOPAGE");
	if(pr->p_flags & PF_NOMAP)
		fprintf(outf," PF_NOMAP");
	if(pr->p_flags & PF_PUBLIC)
		fprintf(outf," PF_PUBLIC");
	if(pr->p_flags & PF_DAEMON)
		fprintf(outf," PF_DAEMON");
	if(pr->p_flags & PF_WRITABLE)
		fprintf(outf," PF_WRITABLE");
	if(pr->p_flags & PF_INHERIT)
		fprintf(outf," PF_INHERIT");
	if(pr->p_flags & PF_VTEXT)
		fprintf(outf," PF_VTEXT");
	if(pr->p_flags & PF_MMFATTACH)
		fprintf(outf," PF_MMFATTACH");

	fprintf(outf, "  ");
	if(pr->p_prot == PROT_NONE)
		fprintf(outf, "PROT_NONE");
	else {
	    int x = 1;
	    if(pr->p_prot&PROT_USER) {
		    fprintf(outf, x + "|PROT_USER");
		    x = 0;
	    }
	    if(pr->p_prot&PROT_KERNEL) {
		    fprintf(outf, x + "|PROT_KERNEL");
		    x = 0;
	    }
	    if(pr->p_prot&PROT_READ) {
		    fprintf(outf, x + "|PROT_READ");
		    x = 0;
	    }
	    if(pr->p_prot&PROT_WRITE) {
		    fprintf(outf, x + "|PROT_WRITE");
		    x = 0;
	    }
	    if(pr->p_prot&PROT_EXECUTE) {
		    fprintf(outf, x + "|PROT_EXECUTE");
		    x = 0;
	    }
	}

	fprintf(outf,"\n");
}

#ifdef __hp9000s800
/*
 * dump_mprotect() --
 *    print out the mprotect data for a pregion and region.
 */
dump_mprotect(p_spreg)
hdl_subpreg_t *p_spreg;
{
    static char me[] = "dump_mprotect";
    static char *mprot_names[] = {
	"UNMAPPED", "NONE", "RO", "RW"
    };

    hdl_subpreg_t subpreg;
    hdl_subpreg_t *psubpreg;
    hdl_subreg_t *psubreg;
    int i;
    int count;
    int size;

    if (p_spreg == (hdl_subpreg_t *)0)
	return;

    fprintf(outf, "\n   mprotect subPregion data (0x%08x):\n", p_spreg);

    if (getchunk(KERNELSPACE, p_spreg, &subpreg, sizeof subpreg, me))
	return;

    count = sizeof subpreg.mode / sizeof subpreg.mode[0];
    if (subpreg.nelements > count) {
	/*
	 * The structure is bigger than the default.  We malloc one of
	 * the correct size and re-read the data.
	 */
	size = sizeof subpreg - sizeof subpreg.mode;
	size += subpreg.nelements * sizeof subpreg.mode[0];

	psubpreg = malloc(size);
	if (psubpreg == (hdl_subpreg_t *)0)
	    goto finish;

	if (getchunk(KERNELSPACE, p_spreg, psubpreg, size, me))
	    goto finish;
    }
    else
	psubpreg = &subpreg;
    
    fprintf(outf, "     p_subreg   0x%08x  nelements %4d\n",
	psubpreg->p_subreg, psubpreg->nelements);

    /*
     * Get the subregion data.
     */
    size = sizeof *psubreg - sizeof psubreg->range;
    size += sizeof psubreg->range[0] * psubpreg->nelements;
    psubreg = malloc(size);
    if (psubreg == (hdl_subreg_t *)0)
	goto finish;
    if (getchunk(KERNELSPACE, psubpreg->p_subreg, psubreg, size, me))
	goto finish;

    fprintf(outf, "   mprotect subRegion data (0x%08x):\n", psubpreg->p_subreg);
    fprintf(outf, "     nused     %4d nelements %4d hint      %4d refcnt    %4d\n",
	psubreg->nused, psubreg->nelements, psubreg->hint, psubreg->refcnt);

    fprintf(outf, "\n");
    for (i = 0; i < psubreg->nused; i++) {
	fprintf(outf, "     [%2d]  idx %4d  protid  0x%04x  flags   0x%04x",
	    i, psubreg->range[i].idx, psubreg->range[i].protid,
	    psubreg->range[i].flags);

	if (psubpreg->mode[i] > 0x3 /* MPROT_RW */) {
	    fprintf(outf, "  MPROT_%d\n", psubpreg->mode[i]);
	}
	else {
	    fprintf(outf, "  MPROT_%s\n", mprot_names[psubpreg->mode[i]]);
	}
    }
    fprintf(outf, "\n");

finish:
    if (psubpreg != &subpreg && psubpreg != (hdl_subpreg_t *)0)
	free(psubpreg);
    if (psubreg != (hdl_subreg_t *)0)
	free(psubreg);
}
#endif /* __hp9000s800 */

#ifdef __hp9000s300
/*
 * dump_mprotect() --
 *    print out the mprotect data for a pregion and region.
 */
dump_mprotect(p_spreg)
hdl_subpreg_t *p_spreg;
{
    /* need to write this for s300 */
}
#endif /* __hp9000s800 */

/* dump region */
/* r is localized, and reg is not */
dumpreg(cp, r, reg)
	char *cp;
	struct region *r, *reg;
{
	register int i;
	char *regaddress;

	fprintf(outf, "\nRegion:  region address is 0x%08x\n", reg);
	fprintf(outf, "   r_flags     0x%08x r_type     0x%08x r_pgsz    %10d\n",
		r->r_flags, r->r_type, r->r_pgsz);
	fprintf(outf, "   r_nvalid    %10d r_swnvalid %10d r_swalloc %10d\n",
		r->r_nvalid, r->r_swnvalid, r->r_swalloc);
	fprintf(outf, "   r_refcnt    %10d r_off      %10d r_incore  %10d\n",
		r->r_refcnt, r->r_off, r->r_incore);
	fprintf(outf, "   r_mlockcnt  %10d r_dbd      0x%08x\n",
		r->r_mlockcnt, r->r_dbd);
	fprintf(outf, "   r_fstore    0x%08x r_bstore   0x%08x\n",
		r->r_fstore, r->r_bstore);
	fprintf(outf, "   r_forw      0x%08x r_back     0x%08x r_zomb        0x%04x\n",
		r->r_forw, r->r_back, r->r_zomb);
	fprintf(outf, "   r_hchain    0x%08x r_byte     0x%08x r_bytelen 0x%08x\n",
		r->r_hchain, r->r_byte, r->r_bytelen);
	fprintf(outf, "   r_lock.b_lock %8d r_lock.order %8d r_lock.owner  0x%08x\n",
		r->r_lock.b_lock, r->r_lock.order, r->r_lock.owner);
	fprintf(outf, "   r_mlock.b_lock %7d r_mlock.order %7d r_mlock.owner 0x%08x\n",
		r->r_mlock.b_lock, r->r_mlock.order, r->r_mlock.owner);
/* MEMSTATS */
	fprintf(outf, "   r_poip      %10d r_root     0x%08x r_key    0x%08x\n",
		r->r_poip, r->r_root, r->r_key);
	fprintf(outf, "   r_chunk     0x%08x\n",
		r->r_chunk);

	fprintf(outf, "   r_next      0x%08x r_prev     0x%08x r_pregs   0x%08x\n",
		r->r_next, r->r_prev, r->r_pregs);

#if defined(__hp9000s700) || defined(__hp9000s800)
	fprintf(outf, "   r_hdl.r_space      0x%08x r_hdl.r_vaddr   0x%08x\n",
	    r->r_hdl.r_space, r->r_hdl.r_vaddr);
	fprintf(outf, "   r_hdl.r_allocsize  0x%08x r_hdl.r_hdlflags    0x%04x\n",
	    r->r_hdl.r_allocsize, r->r_hdl.r_hdlflags);
	
#define RHDL_ELSE   (~(RHDL_MMAP_ATTACHED|RHDL_MMAP_PUBLIC|	\
		       RHDL_SID_ALLOC))

	if (r->r_hdl.r_hdlflags & RHDL_ELSE)
	    fprintf(outf, " RHDL_0x%04x",
		r->r_hdl.r_hdlflags & RHDL_ELSE);
	if (r->r_hdl.r_hdlflags & RHDL_MMAP_ATTACHED)
	    fprintf(outf, " RHDL_MMAP_ATTACHED");
	if (r->r_hdl.r_hdlflags & RHDL_MMAP_PUBLIC)
	    fprintf(outf, " RHDL_MMAP_PUBLIC");
	if (r->r_hdl.r_hdlflags & RHDL_SID_ALLOC)
	    fprintf(outf, " RHDL_SID_ALLOC");
	fprintf(outf,"\n");
#else
	fprintf(outf, " r_hdl.r_flags      0x%08x\n",
	    r->r_hdl.r_flags);
#endif

#define RF_ELSE (~(RF_NOFREE|RF_ALLOC|RF_MLOCKING|RF_ZOMB|	 \
		   RF_UNALIGNED|RF_SWLAZY|RF_WANTLOCK|RF_HASHED| \
		   RF_EVERSWP|RF_NOWSWP|RF_DAEMON|RF_UNMAP))

	fprintf(outf,"  ");
	if (r->r_flags & RF_ELSE)
		fprintf(outf, " RF_0x%08x", r->r_flags & RF_ELSE);
	if (r->r_flags & RF_NOFREE)
		fprintf(outf, " RF_NOFREE");
	if (r->r_flags & RF_ALLOC)
		fprintf(outf, " RF_ALLOC");
	if (r->r_flags & RF_MLOCKING)
		fprintf(outf, " RF_MLOCKING");
	if (r->r_flags & RF_ZOMB)
		fprintf(outf, " RF_ZOMB");
	if (r->r_flags & RF_UNALIGNED)
		fprintf(outf, " RF_UNALIGNED");
	if (r->r_flags & RF_SWLAZY)
		fprintf(outf, " RF_SWLAZY");
	if (r->r_flags & RF_WANTLOCK)
		fprintf(outf, " RF_WANTLOCK");
	if (r->r_flags & RF_HASHED)
		fprintf(outf, " RF_HASHED");
	if (r->r_flags & RF_EVERSWP)
		fprintf(outf, " RF_EVERSWP");
	if (r->r_flags & RF_NOWSWP)
		fprintf(outf, " RF_NOWSWP");
	if (r->r_flags & RF_DAEMON)
		fprintf(outf, " RF_DAEMON");
	if (r->r_flags & RF_UNMAP)
		fprintf(outf, " RF_UNMAP");
	fprintf(outf, "\n   ");

	switch (r->r_type) {
	case RT_UNUSED:
		fprintf(outf, "RT_UNUSED");
		break;
	case RT_PRIVATE:
		fprintf(outf, "RT_PRIVATE");
		break;
	case RT_SHARED:
		fprintf(outf, "RT_SHARED");
		break;
	default:
		fprintf(outf, "RT_0x%04x", r->r_type);
		break;
	}
	fprintf(outf, "\n");

	if (Eflg && r->r_dbd == 0) {
		dumpregvfd(r);
	}
}

/* dump pfdat structure */
dumppfdat(cp, pf, lpf, vpf)
	char *cp;
	struct pfdat *pf, *lpf, *vpf;
{
	char *pfaddress;
	if (pf == &phead){
		/* map others to the header */
		lpf = &phead;
		vpf = vphead;
		fprintf(outf,"Pfdat is freelist head\n");
	}

	pfaddress = (char *)(vpf + (pf - lpf));


	fprintf(outf,"Pfdat entry : %s   Pfdat headr address 0x%08x\n",cp,
	pfaddress);

	fprintf(outf,"  pf_data  0x%08x  pf_flags  0x%08x  pf_use    0x%08x\n",
		pf->pf_data, pf->pf_flags, pf->pf_use);
#ifdef PFDAT32
	fprintf(outf,"  pf_devvp  0x%08x  pf_pfn    0x%08x  pf_wanted 0x%08x\n",
		pf->pf_devvp, pf-pfdat, pf->pf_wanted);
#else
	fprintf(outf,"  pf_devvp  0x%08x  pf_pfn    0x%08x  pf_fill   0x%08x\n",
		pf->pf_devvp, pf->pf_pfn, pf->pf_fill);
#endif
	fprintf(outf,"  pf_next   0x%08x  pf_prev   0x%08x\n",
		pf->pf_next, pf->pf_prev);
	fprintf(outf,"  pf_hchan  0x%08x  pf_lock   0x%08x\n",
#ifdef PFDAT32
		pf->pf_hchain, pf->pf_locked);
#else		
		pf->pf_hchain, pf->pf_lock);
#endif		
#if defined(__hp9000s700) || defined(__hp9000s800)
	fprintf(outf,"  hdlpf_flags 0x%08x  \n", pf->pf_hdl.hdlpf_flags);
#else
	fprintf(outf,"  pf_bits  0x%08x  \n", (u_long)pf->pf_hdl.pf_bits);
#endif

	fprintf(outf,"\n   ");

#ifdef hp9000s800
#define P_ELSE (~(P_QUEUE|P_BAD|P_HASH|P_SYS|P_HDL|P_DMEM|P_LCOW))
#else
#define P_ELSE (~(P_QUEUE|P_BAD|P_HASH|P_SYS|P_HDL))
#endif

	if (pf->pf_flags & P_ELSE)
		fprintf(outf," P_0x%08x", pf->pf_flags & P_ELSE);
	if (pf->pf_flags & P_QUEUE)
		fprintf(outf," P_QUEUE");
	if (pf->pf_flags & P_BAD)
		fprintf(outf," P_BAD");
	if (pf->pf_flags & P_HASH)
		fprintf(outf," P_HASH");
	if (pf->pf_flags & P_SYS)
		fprintf(outf," P_SYS");
	if (pf->pf_flags & P_HDL)
		fprintf(outf," P_HDL");
#ifdef hp9000s800		
	if (pf->pf_flags & P_DMEM)
		fprintf(outf," P_DMEM");
	if (pf->pf_flags & P_LCOW)
		fprintf(outf," P_LCOW");
#endif		
	fprintf(outf,"\n");
}

dumpregvfd(rp)
   register reg_t *rp;
{
   register int i,end;
   register vfd_t **vfd;
   register int index = 0;

	end = rp->r_pgsz;

	fprintf(outf, "\n");
	fprintf(outf, "\t    Vfd     flgs    pfn      type     data     index\n");
	fprintf(outf, "\t==========  ====  ========  ======  =========  =====\n");

	for (i = 0; i < end; i++) {
		dumpvfddbd(rp, i);
	}
}

dumpvfddbd(rp,index)
register reg_t *rp;
register int index;
{
	char dash[4];
	struct vfd  *vfd;
	dbd_t	*dbd;
	int i;
	char *str;

	dash[0] = dash[1] = dash[2] = '-';
	dash[3]= '\0';

	if (findentry(rp,index,&vfd,&dbd) != NULL) {
		if (vfd->pg_v)
			dash[0]= 'V';
		if (vfd->pg_cw)
			dash[1]= 'C';
		if (vfd->pg_lock)
			dash[2]= 'L';
		fprintf(outf, "\t0x%08x  %s   ", *(int *)vfd, dash);
		fprintf(outf, "0x%06x  ", vfd->pg_pfnum);
		switch (dbd->dbd_type) {
		case DBD_NONE:
			str = "NONE";	break;
		case DBD_FSTORE:
			str = "FSTORE";	break;
		case DBD_BSTORE:
			str = "BSTORE";	break;
		case DBD_DZERO:
			str = "DZERO";	break;
		case DBD_DFILL:
			str = "DFILL";	break;
		case DBD_HOLE:
			str = "HOLE";	break;
		default:
			str = NULL;	break;
		}

		if (str != NULL)
			fprintf(outf, "%-6s  ", str);
		else
			fprintf(outf, "DBD_0x%1x ", dbd->dbd_type);
		fprintf(outf, "0x%07x  [%3d]\n", dbd->dbd_data, index);

		/* check swap page consistency */
		if (dflg && rp->r_bstore == swapdev_vp) {
			struct swpdbd *swndx;
			swpm_t *smap_ptr,*swp_ent;

			swndx = (struct swpdbd *)dbd;
			swp_ent = swaptab[swndx->dbd_swptb].st_swpmp +
				  (swndx->dbd_swpmp * sizeof(swpm_t));
			smap_ptr = GETBYTES(swpm_t *,
				swp_ent, sizeof(swpm_t));
			if (smap_ptr == 0) {
				fprintf(outf, "dumpvfddbd: localizing smap_ptr failed\n");
				return(NULL);
			}
			if ((smap_ptr->sm_ucnt > 0 ) &&
			    (swaptab[swndx->dbd_swptb].st_dev || swaptab[swndx->dbd_swptb].st_fsp))
			    /* nothing */;
			else {
				fprintf_err();
				fprintf(outf, " use count mismatch on swaptab[0x%x] & swapmap[0x%x]\n",
					swndx->dbd_swptb, swndx->dbd_swpmp);
			}
		}
	}
}

/* dump dbd */
dumpdbd(cp, dbd)
	char *cp;
	dbd_t *dbd;
{
	char *str;

	fprintf(outf,"   dbd_type 0x%1x   dbd_data 0x%07x",
		dbd->dbd_type, dbd->dbd_data);
	switch (dbd->dbd_type) {
	case DBD_NONE:
		str = "NONE";	break;
	case DBD_FSTORE:
		str = "FSTORE";	break;
	case DBD_BSTORE:
		str = "BSTORE";	break;
	case DBD_DZERO:
		str = "DZERO";	break;
	case DBD_DFILL:
		str = "DFILL";	break;
	case DBD_HOLE:
		str = "HOLE";	break;
	default:
		str = NULL;	break;
	}

	if (str == NULL)
		fprintf(outf,"   DBD_0x%1x\n", dbd->dbd_type);
	else
		fprintf(outf,"   DBD_%s\n", str);

}


/* dump vfd */
dumpvfd(cp, vfd)
	char *cp;
	struct vfd *vfd;
{
	fprintf(outf,"   pg_pfnum 0x%06x", vfd->pg_pfnum);
	fprintf(outf,"   ");
	if(vfd->pg_v)
		fprintf(outf," PG_V");
	if(vfd->pg_cw)
		fprintf(outf," PG_CW");
	if(vfd->pg_lock)
		fprintf(outf," PG_LOCK");
	fprintf(outf,"  :0x%08x \n",*(int *)vfd);

}

/* dump swaptab */
dumpswaptab(cp, st, lst, vst)
	char *cp;
	swpt_t *st, *lst, *vst;
{

	fswdev_t   *fsp;
	swdev_t    *dev;

	fprintf(outf,"Swaptab entry : %s    address 0x%08x\n",cp,
	( vst + (st - lst)));

	fprintf(outf,"   st_free   0x%08x  st_next 0x%08x  st_flags   0x%08x\n",
		st->st_free, st->st_next, st->st_flags);
	fprintf(outf,"   st_dev    0x%08x  st_fsp  0x%08x  st_vnode   0x%08x\n",
		st->st_dev, st->st_fsp, st->st_vnode);
	fprintf(outf,"   st_nfpgs  0x%08x  st_swpmp 0x%08x st_site    0x%08x\n",
		st->st_nfpgs, st->st_swpmp, st->st_site);
	fprintf(outf,"   st_swptab 0x%08x\n", st->st_union.st_swptab);

	if(st->st_flags == ST_INDEL)
		fprintf(outf," ST_INDEL");
	else
		fprintf(outf," ST_???");

	fprintf(outf,"\n");

	/* dump the swap device structure */
	if (st->st_fsp) {
		fsp = GETBYTES(fswdev_t *, st->st_fsp, sizeof(fswdev_t));
		if (fsp == 0) {
			fprintf(outf,"dumpswaptab: localizing fsp failed\n");
			return(NULL);
		}
		dumpfswdevt(fsp);
	} else
		if (st->st_dev) {
			dev = GETBYTES(swdev_t *, st->st_dev, sizeof(swdev_t));
			if (dev == 0) {
				fprintf(outf,"dumpswaptab: localizing dev failed\n");
				return(NULL);
			}

			dumpswdevt(dev);
		}
}

dumpfswdevt(fp)
	fswdev_t *fp;
{
	fprintf(outf,"fswdevt:\n");
	fprintf(outf,"   fsw_next       0x%08x  fsw_enable  0x%08x  fsw_nfpgs  0x%08x\n",
		fp->fsw_next, fp->fsw_enable, fp->fsw_nfpgs);
	fprintf(outf,"   fsw_allocated  0x%08x  fsw_min     0x%08x  fsw_limit  0x%08x\n",
		fp->fsw_allocated, fp->fsw_min, fp->fsw_limit);
	fprintf(outf,"   fsw_priority   0x%08x  fsw_reserve 0x%08x  fsw_vnode  0x%08x\n",
		fp->fsw_priority, fp->fsw_reserve, fp->fsw_vnode);
	fprintf(outf,"   fsw_mntpoint   0x%08x  fsw_head    0x%08x  fsw_tail   0x%08x\n",
		fp->fsw_mntpoint, fp->fsw_head, fp->fsw_tail);

	fprintf(outf,"\n");

}

dumpswdevt(dp)
	swdev_t  *dp;
{
	fprintf(outf,"swdevt:\n");
	fprintf(outf,"   sw_dev       0x%08x  sw_enable  0x%08x  sw_nfpgs  0x%08x\n",
		dp->sw_dev, dp->sw_enable, dp->sw_nfpgs);
	fprintf(outf,"   sw_priority  0x%08x  sw_nblks   0x%08x  sw_head   0x%08x\n",
		dp->sw_priority, dp->sw_nblks, dp->sw_head);
	fprintf(outf,"   sw_tail      0x%08x  sw_next    0x%08x  sw_start  0x%08x\n",
		dp->sw_tail, dp->sw_next, dp->sw_start);

	fprintf(outf,"\n");

}

dumpvmstats()
{
	int stacksize = btorp(KSTACKBYTES);
	int upages = UPAGES;
	/* int maxvfd = MAXVFD;
	int vfd_noindir = VFD_NOINDIR;
	int vfd_1indir = VFD_1INDIR;
	int vfd_2indir = VFD_2INDIR;
	int vfd_dbds = VFD_DBDS;
	int vfd_nindir = VFD_NINDIR; */
	int i;

	fprintf(outf,"\n");
	fprintf(outf," VM stats:\n");
	fprintf(outf," maxfree    0x%07x   lotsfree  0x%07x   desfree  0x%07x\n",
		maxfree, lotsfree, desfree);
	fprintf(outf," minfree    0x%07x   stack     0x%07x   upages   0x%07x\n",
		minfree, stacksize, upages);
	/* MEMSTATS */
	fprintf(outf," physmem    0x%07x   freemem   0x%07x   bufpages 0x%07x  (0x%x)\n",
		physmem, freemem, bufpages, bufpages * NBPG);


	fprintf(outf,"\n Dumping KMEMSTATS info \n");

	/* Dump kmemstat info */
	for (i = 0; i < M_LAST; i++){
		dumpkmemstat("kmemstat",&kmemstats[i], i);
	}
	fprintf(outf,"\n\n");
}

dumpvmaddr()
{
	/* MEMSTATS */
	fprintf(outf,"\n");
	fprintf(outf," VM tables addresses:\n");
	/* fprintf(outf," region   = 0x%08x - 0x%08x  size = 0x%04x entries = 0x%04x\n",
	vregion, &vregion[v.v_region], sizeof(struct region), v.v_region); */
	fprintf(outf," buf      = 0x%08x - 0x%08x  size = 0x%04x entries = 0x%04x (0x%x)\n",
	0, 0, sizeof(struct buf), nbuf, sizeof(struct buf) * nbuf);
	fprintf(outf," swbuf    = 0x%08x - 0x%08x  size = 0x%04x entries = 0x%04x (0x%x)\n",
	vswbuf, &vswbuf[nswbuf], sizeof(struct buf), nswbuf,sizeof(struct buf) * nswbuf);
	fprintf(outf," inode    = 0x%08x - 0x%08x  size = 0x%04x entries = 0x%04x (0x%x)\n",
	vinode, &vinode[ninode], sizeof(struct inode), ninode, sizeof(struct inode) * ninode);
	fprintf(outf," file     = 0x%08x - 0x%08x  size = 0x%04x entries = 0x%04x (0x%x)\n",
	vfile, &vfile[nfile], sizeof(struct file), nfile, sizeof(struct file) * nfile);
	fprintf(outf," proc     = 0x%08x - 0x%08x  size = 0x%04x entries = 0x%04x (0x%x)\n",
	vproc, &vproc[nproc], sizeof(struct proc), nproc, sizeof(struct proc) * nproc);
#ifndef REGION
	fprintf(outf," text     = 0x%08x - 0x%08x  size = 0x%04x entries = 0x%04x\n",
	vtext, &vtext[ntext], sizeof(struct text), ntext);
#endif
	fprintf(outf," sysmap   = 0x%08x - 0x%08x  size = 0x%04x entries = 0x%04x (0x%x)\n",
	vsysmap, &vsysmap[SYSMAPSIZE], sizeof(struct map), SYSMAPSIZE, sizeof(struct map) * SYSMAPSIZE);
	/* fprintf(outf," pregion  = 0x%08x - 0x%08x  size = 0x%04x entries = 0x%04x\n",
	vprp, &vprp[npblks], sizeof(struct pregion), npblks); */
	fprintf(outf," phash    = 0x%08x - 0x%08x  size = 0x%04x entries = 0x%04x (0x%x)\n",
	vphash, &vphash[phashmask+1], sizeof(struct pfdat **), phashmask+1, sizeof(struct pfdat **) * (phashmask+1));
	fprintf(outf," pfdat    = 0x%08x - 0x%08x  size = 0x%04x entries = 0x%04x (0x%x)\n",
	vpfdat, &vpfdat[physmem], sizeof(struct pfdat), physmem, sizeof(struct pfdat) * physmem);
	fprintf(outf," clist    = 0x%08x - 0x%08x  size = 0x%04x entries = 0x%04x (0x%x)\n",
	vcfree, &vcfree[nclist], sizeof(struct cblock), nclist, sizeof(struct cblock) * nclist);

	fprintf(outf,"\n");
#ifdef hp9000s800
	fprintf(outf," VM values:\n");
	fprintf(outf," pdir    0x%08x   scaled_npdir0x%08x (0x%x)\n",
		vpdir,scaled_npdir, sizeof(struct hpde) * scaled_npdir);
	fprintf(outf," htbl    0x%08x   nhtbl       0x%08x (0x%x)\n",
		vhtbl,nhtbl,sizeof(struct hpde) * nhtbl);
#endif
	fprintf(outf," ubase   0x%08x   \n",ubase);
	fprintf(outf,"\n");
}

dumpuptr()
{
#ifndef MP
	fprintf(outf," Current process uptr = 0x%08x  size u = 0x%08x\n",
		uptr,sizeof(struct user));
	fprintf(outf," Current process ptr = 0x%08x\n",currentp);
#endif
}

dumpsysinfo()
{
	fprintf(outf,"\n");
	fprintf(outf," sysinfo values:\n");
	fprintf(outf,"Not yet supported\n:");

	fprintf(outf,"\n");
}

#include "h/msgbuf.h"

#ifdef __hp9000s300
#define msgbuf_name "Msgbuf"
#else
#define msgbuf_name "msgbuf"
#endif

void
dumpmsgbuf()
{
    u_long x;
    char *cp;
    char *cp_end;
    struct msgbuf msg;

    if ((x = lookup(msgbuf_name)) == 0)
    {
	fprintf(outf, "dumpmsgbuf: can't find symbol \"%s\"\n",
	    msgbuf_name);
	return;
    }

    if (getchunk(KERNELSPACE, x, &msg, sizeof msg, "dumpmsgbuf"))
	return;

    fprintf(outf, "\n\n");
    fprintf(outf, "***********************************************************************\n");
    fprintf(outf, "*                           MESSAGE BUFFER                            *\n");
    fprintf(outf, "***********************************************************************\n\n");

    /*
     * If the buffer has wrapped, find the beginning of a complete
     * line before beginning to print, otherwise just start at
     * the beginning of the buffer.
     */
    cp = &msg.msg_bufc[msg.msg_bufx];
    if (*cp == '\0')
    {
	/*
	 * Buffer has not wrapped...
	 */
	cp = &msg.msg_bufc[0];
    }
    else
    {
	/*
	 * Buffer has wrapped, find beginning of first complete
	 * line.
	 */
	fprintf(outf, "...");

	cp_end = &msg.msg_bufc[MSG_BSIZE];
	while (cp < cp_end && *cp != '\n')
	    cp++;

	/*
	 * Handle wrap-around
	 */
	if (cp == cp_end)
	{
	    cp = &msg.msg_bufc[0];

	    while (cp < cp_end && *cp != '\n')
		cp++;
	}
    }

    /*
     * Now, print all data between cp and the end of the buffer (but
     * remember it is circular buffer, so it could wrap).
     */
    cp_end = &msg.msg_bufc[msg.msg_bufx];

    if (cp_end > cp)
	fwrite(cp, 1, cp_end - cp, outf);
    else
    {
	fwrite(cp, 1, &msg.msg_bufc[MSG_BSIZE] - cp, outf);
	fwrite(&msg.msg_bufc[0], 1, cp_end - &msg.msg_bufc[0], outf);
    }

    fprintf(outf, "\n\n");
    fprintf(outf, "***********************************************************************\n\n");
}

void
dumpdquota (dq, vm)
struct dquot *dq;
unsigned vm;
{
    int dqelse;

    fprintf(outf,"dquot at: 0x%08x \n", vm);
    fprintf(outf,"dq_forw  0x%08x dq_back  0x%08x ", 
		dq->dq_forw, dq->dq_back);
    fprintf(outf,"dq_freef 0x%08x dq_freeb 0x%08x\n", 
		dq->dq_freef, dq->dq_freeb);
    fprintf(outf,"dq_flag  0x%08x dq_cnt   0x%08x ",
		dq->dq_flags, dq->dq_cnt);
    fprintf(outf,"dq_uid   0x%08x dq_mp    0x%08x\n",
		dq->dq_uid, dq->dq_mp);
    fprintf(outf,"bhard    0x%08x bsoft    0x%08x ",
		dq->dq_dqb.dqb_bhardlimit, dq->dq_dqb.dqb_bsoftlimit);
    fprintf(outf,"curblks  0x%08x fhard    0x%08x\n",
		dq->dq_dqb.dqb_curblocks, dq->dq_dqb.dqb_fhardlimit);
    fprintf(outf,"fsoft    0x%08x curfiles 0x%08x ",
		dq->dq_dqb.dqb_fsoftlimit, dq->dq_dqb.dqb_curfiles);
    fprintf(outf,"timeblks 0x%08x timefils 0x%08x\n",
		dq->dq_dqb.dqb_btimelimit, dq->dq_dqb.dqb_ftimelimit);


    dqelse = ~( DQ_LOCKED |  DQ_WANT | DQ_MOD | DQ_BLKS | DQ_FILES | 
		DQ_SOFT_FILES | DQ_HARD_FILES | DQ_TIME_FILES | 
		DQ_SOFT_BLKS | DQ_HARD_BLKS | DQ_TIME_BLKS | DQ_RESERVED |
		DQ_FREE);

    if (dq->dq_flags &  DQ_LOCKED) 
      fprintf(outf," DQ_LOCKED ");
    if (dq->dq_flags &  DQ_WANT )
      fprintf(outf," DQ_WANT ");
    if (dq->dq_flags &  DQ_MOD )
      fprintf(outf," DQ_MOD ");
    if (dq->dq_flags &  DQ_BLKS )
      fprintf(outf," DQ_BLKS ");
    if (dq->dq_flags &  DQ_FILES )
      fprintf(outf," DQ_FILES ");
    if (dq->dq_flags &  DQ_SOFT_FILES)
      fprintf(outf," DQ_SOFT_FILES");
    if (dq->dq_flags &  DQ_HARD_FILES )
      fprintf(outf," DQ_HARD_FILES ");
    if (dq->dq_flags &  DQ_TIME_FILES )
      fprintf(outf," DQ_TIME_FILES ");
    if (dq->dq_flags &  DQ_SOFT_BLKS )
      fprintf(outf," DQ_SOFT_BLKS ");
    if (dq->dq_flags &  DQ_HARD_BLKS) 
      fprintf(outf," DQ_HARD_BLKS ");
    if (dq->dq_flags & DQ_TIME_BLKS)
      fprintf(outf," DQ_TIME_BLKS");
    if (dq->dq_flags &  DQ_RESERVED )
      fprintf(outf," DQ_RESERVED ");
    if (dq->dq_flags &  DQ_FREE)
      fprintf(outf," DQ_FREE");

    if (dqelse)
      fprintf(outf,"** DQ_ELSE ** 0x%08x", dqelse);
    fprintf(outf,"\n\n");


}

#include "h/callout.h"

void
dumpcallout()
{
    u_long addr;
    int ncallout;
    struct callout calltodo;
    struct callout *callbuf;
    u_long offset;
    int i;
    struct callout *p1;
    int t = 0;

    fprintf(outf,"\n\n\n***********************************************************************\n");
    fprintf(outf,"*                     SCANNING CALLOUT TABLE                          *\n");
    fprintf(outf,"***********************************************************************\n\n");

    /*
     * read in all the variables and data structures we will
     * need.
     *
     * ncallout -- the number of elements in the callout pool.
     */
    if ((addr = lookup("ncallout")) == 0)
    {
	fprintf(outf, "dumpcallout: can't find symbol \"ncallout\"\n");
	return;
    }
    ncallout = get(addr);
    fprintf(outf, "ncallout        = %10d\n", ncallout);

    /*
     * calltodo -- the head of the active callout chain.
     */
    if ((addr = lookup("calltodo")) == 0)
    {
	fprintf(outf, "dumpcallout: can't find symbol \"calltodo\"\n");
	return;
    }
    callbuf = GETBYTES(struct callout *, addr, sizeof (struct callout));
    if (callbuf == (struct callout *)0)
    {
	fprintf(outf, "dumpcallout: can't read \"calltodo\"\n");
	return;
    }
    memcpy(&calltodo, callbuf, sizeof (struct callout));
    fprintf(outf, "calltodo.c_time = %10d\n", calltodo.c_time);
    fprintf(outf, "calltodo.c_next = 0x%08x\n", calltodo.c_next);

    /*
     * callbuf -- an array of elements comprising the callout pool.
     */
    if ((addr = lookup("callout")) == 0)
    {
	fprintf(outf, "dumpcallout: can't find symbol \"callout\"\n");
	return;
    }
    addr = get(addr);
    callbuf = GETBYTES(struct callout *, addr, ncallout*sizeof (struct callout));
    fprintf(outf, "callout         = 0x%08x\n\n", addr);

    /*
     * Now, localize the pointers in the callout structures so we can
     * traverse them normally.
     */
    offset = (u_long)callbuf - addr;
    if (calltodo.c_next != (struct callout *)0)
	calltodo.c_next = (struct callout *)((u_long)calltodo.c_next + offset);

    for (i = 0; i < ncallout; i++)
    {
	if (callbuf[i].c_next != (struct callout *)0)
	    callbuf[i].c_next = (struct callout *)((u_long)callbuf[i].c_next + offset);
    }

    i = 0;
    for (p1 = calltodo.c_next; p1 != (struct callout *)0; p1 = p1->c_next)
    {
	u_long diff;
	char buf[80];

	t += p1->c_time;
	sprintf(buf, "now+%d", t);
	fprintf(outf, "  0x%08x: %-14s ", p1 - offset, buf);

	diff = findsym(p1->c_func);
	if (p1->c_arg == 0)
	{
	    if (diff == 0xffffffff)
		sprintf(buf, "0x%08x(0)", p1->c_func);
	    else if (diff)
		sprintf(buf, "%s+0x%04x(0)", cursym, diff);
	    else
		sprintf(buf, "%s(0)", cursym);
	}
	else
	{
	    if (diff == 0xffffffff)
		sprintf(buf, "0x%08x(0x%x)", p1->c_func, p1->c_arg);
	    else if (diff)
		sprintf(buf, "%s+0x%04x(0x%x)", cursym, diff, p1->c_arg);
	    else
		sprintf(buf, "%s(0x%x)", cursym, p1->c_arg);
	}

#if defined(__hp9000s800) && defined(_WSIO)
	if (p1->flag)
	    fprintf(outf, "%-30s flag = %d\n", buf, p1->flag);
	else
#endif
	    fprintf(outf, "%s\n", buf);

	if (++i > ncallout)
	{
	    fprintf(outf, "ERROR: more than ncallout entries in list!\n");
	    break;
	}
    }
}
