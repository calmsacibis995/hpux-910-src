/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/qfs.c,v $
 * $Revision: 1.4.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:31:37 $
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


#ifdef QFS  /* extends to end of file */

#include "inc.h"
#include "defs.h"
#include "types.h"
#include "externs.h"

/*
 * Display info for output formatting
 */
#define HEX 1
#define DEC 2
int strwidth = 12;	/* width of name field */
int strbetween = 2;	/* number of blanks between name-value pairs */

display(str, val, how)
	char *str;
{
	int i;

	fprintf(outf, "%-*s ", strwidth, str);
	if (how == HEX)
		fprintf(outf, "0x%08x", val);
	else	fprintf(outf, "%10d", val);
	for (i=0; i<strbetween; i++)
		putc(' ', outf);
}

/*
 * Tables for printing flags and value names
 */

struct nameval {
	char *name;
	int val;
};

struct nameval nv_qtype[] = {
	{ "QL_HEAD", 		QL_HEAD },
	{ "QL_INODE", 		QL_INODE },
	{ "QL_BUF", 		QL_BUF },
	{ "QL_FLOAT", 		QL_FLOAT },
	{ "QL_TRHEADER",	QL_TRHEADER },
	{ "QL_PLINK", 		QL_PLINK },
	{ "QL_DEFOP", 		QL_DEFOP },
	{ "", 			0 } };

struct nameval nv_l_flags[] = {
	{ "L_SLEEP", 		L_SLEEP },
	{ "L_ALTHEAD", 		L_ALTHEAD },
	{ "L_LOCKED", 		L_LOCKED },
	{ "", 			0 }};

struct nameval nv_res_type[] = {
	{ "RES_LOGSPACE", 	RES_LOGSPACE },
	{ "", 			0 }};

struct nameval nv_lp_flags[] = {
	{ "LH_HEAD", 		LH_HEAD },
	{ "LH_ALTHEAD", 	LH_ALTHEAD },
	{ "", 			0 }};

struct nameval nv_eh_flags[] = {
	{ "E_FLOATING", 	E_FLOATING },
	{ "E_DEFERRED", 	E_DEFERRED },
	{ "", 			0 }};

/*
 * Given a flags word and a nameval array with the names of all the flags,
 * print the names of the flags present.
 */
qfs_flags(x, nvp)
	struct nameval *nvp;
{
	int flag_printed = 0;

	/* if 0, just print it and go */
	if (x == 0)
	{
		fprintf(outf, "0x%08x", x);
		return;
	}

	/* else print each flag name that's present */
	while (nvp->name[0])
	{
		if (x & nvp->val)
		{
			if (flag_printed)
				putc('|', outf);
			fprintf(outf, "%s", nvp->name);
			++flag_printed;
		}
		x &= ~nvp->val;
		++nvp;
	}

	/* and then any values not accounted for */
	if (x)
	{
		if (flag_printed)
			putc('|', outf);
		fprintf(outf, "0x%08x", x);
	}
}

/*
 * Given a value and a nameval array with value names,
 * print the name of the value we've got.
 */
qfs_value(x, nvp)
	struct nameval *nvp;
{
	while (nvp->name[0])
	{
		if (nvp->val == x)
		{
			fprintf(outf, "%s", nvp->name);
			return;
		}
		++nvp;
	}
	fprintf(outf, "%d", x);
}

/*
 * Print string describing queue link.
 * qp is local addr, vqp is virtual addr.
 */
qfs_qlink(name, qp, vqp)
	qlinkP qp, vqp;
{
	fprintf(outf, "%s: addr 0x%08x, next 0x%08x, prev 0x%08x, type ",
		name, vqp, qp->ql_next, qp->ql_prev);
		qfs_value(qp->ql_type, nv_qtype);
		putc('\n', outf);
}

/*
 * Print qfs items in u structure.
 */
qfs_ustuff()
{
	int i;

	fprintf(outf," u_trptr 0x%08x  u_lcount %d\n",
		u.u_trptr, u.u_lcount);
	fprintf(outf," u_lck_keys: ");
	for (i=0; i<LOCK_TRACK_MAX; i++)
	{
		/* print 5 numbers per line */
		if (i && (i % 5) == 0)
			fprintf(outf, "\n             ");
		fprintf(outf, "0x%08x ", u.u_lck_keys[i]);
	}
}

/*
 * malloc some space and return error-free.
 */
char *
get_space(size)
{
	char * local;

	local = (char *) malloc(size);
	if (local == NULL)
	{
		fprintf(stderr, "out of space\n");
		exit(1);
	}
	return local;
}

/*
 * Get size bytes from the physaddr provided, reading into localaddr.
 */
read_bytes(paddr, size, localaddr)
	caddr_t paddr;
	char *localaddr;
{
	int res;
	res = lseek(fcore, paddr, 0);
	if (res == -1)
	{
		perror("read_bytes lseek");
		exit(1);
	}
	res = read(fcore, localaddr, size);
	if (res != size)
	{
		perror("read_bytes read");
		exit(1);
	}
}

/*
 * Given a virtual addr, read in size bytes,
 * allocating space for it.
 */
#ifdef	PA89
#define PAGESIZE (4*1024)
#else	/* PA89 */
#define PAGESIZE (2*1024)
#endif	/* PA89 */
#define numpg(x)  (((x) + PAGESIZE - 1)/PAGESIZE)
#define min(a,b)  ((a) < (b)? (a): (b))

caddr_t
newstruct(vaddr, size)
	caddr_t vaddr;
{
	caddr_t paddr, local, savelocal;
	int pg, npg;

	savelocal = local = get_space(size);
	npg = numpg(size);
	for (pg = 0; pg < npg; ++pg)
	{
		read_bytes(getphyaddr(vaddr), min(size, PAGESIZE), local);
		vaddr += PAGESIZE;
		local += PAGESIZE;
		size -= PAGESIZE;
	}
	return savelocal;

}

/*
 * Print string describing resource structure.
 */
qfs_resP(r)
	resP r;
{
	fprintf(outf, "type ");
	qfs_value(r->res_type, nv_res_type);
	fprintf(outf, ", amount %d", r->res_amount);
}

/*
 * Keep track of log pages so we can come back and get them later.
 */
#define MAXLP 100
struct lprecord {
	caddr_t vaddr;
	int npages;
} lprecord[MAXLP];
int lpi, lpimax;

record_log_pages(vaddr, npages)
{
	if (lpi == MAXLP)
	{
		fprintf(stderr, "QFS ERROR: too many log pages\n");
		return;
	}
	lprecord[lpi].vaddr = vaddr;
	lprecord[lpi].npages = npages;
	++lpi;
}

/*
 * Prepare to (re)read stored log page info.
 */
reset_log_pages()
{
	if (lpimax == 0)
		lpimax = lpi;
	lpi = 0;
}

/*
 * Return next piece of info about recorded log pages.
 */
retrieve_log_pages(addrp, nump)
	caddr_t *addrp;
	int *nump;
{
	/* return next entry */
	if (lpi < lpimax)
	{
		*addrp = lprecord[lpi].vaddr;
		*nump = lprecord[lpi].npages;
		++lpi;
		return 1;
	}
	else	return 0;
}


/*
 * Print out info about a log hdr.
 */
qfs_loghdr(vlhp)
	log_hdrP vlhp;
{
	log_hdrP lhp;

	lhp = (log_hdrP) newstruct(vlhp, sizeof(log_hdrT));
	fprintf(outf,"QFS log header:\n");

	fprintf(outf,"   ");
	display("l_size", 	lhp->l_size, 		DEC);
	display("l_pgsize", 	lhp->l_pgsize, 		DEC);
	display("l_npages", 	lhp->l_npages, 		DEC);
	putc('\n', outf);

	fprintf(outf,"   ");
	display("l_dev", 	lhp->l_dev, 		HEX);
	display("l_vp", 	lhp->l_vp, 		HEX);
	display("l_sblknum", 	lhp->l_sblknum, 	DEC);
	putc('\n', outf);

	fprintf(outf,"   ");
	display("l_tail", 	lhp->l_tail, 		HEX);
	display("l_dhead", 	lhp->l_dhead, 		HEX);
	display("l_head", 	lhp->l_head,	 	HEX);
	putc('\n', outf);

	fprintf(outf,"   ");
	display("l_althead", 	lhp->l_althead, 	HEX);
	display("l_curhptr", 	lhp->l_curhptr, 	HEX);
	display("l_hswbytes", 	lhp->l_hswbytes, 	DEC);
	putc('\n', outf);

	fprintf(outf,"   ");
	display("l_hrbytes", 	lhp->l_hrbytes,		DEC);
	display("l_cur_trnum", 	lhp->l_cur_trnum, 	DEC);
	display("l_seqnum", 	lhp->l_seqnum, 		DEC);
	putc('\n', outf);

	fprintf(outf,"   ");
	display("l_nbfree", 	lhp->l_nbfree, 		DEC);
	display("l_nbfmin", 	lhp->l_nbfmin, 		DEC);
	display("l_nbdirty", 	lhp->l_nbdirty, 	DEC);
	putc('\n', outf);

	fprintf(outf,"   ");
	display("l_maxdirty", 	lhp->l_maxdirty, 	DEC);
	display("l_nbfloats", 	lhp->l_nbfloats, 	DEC);
	display("l_pid", 	lhp->l_pid, 		DEC);
	putc('\n', outf);

	fprintf(outf,"   ");
	display("l_blktable", 	lhp->l_blktable, 	HEX);
	display("l_locklevel", 	lhp->l_locklevel,	DEC);
	display("l_nscancount",	lhp->l_nscancount, 	DEC);
	putc('\n', outf);

	fprintf(outf,"   l_spaceres: ");
		qfs_resP(&lhp->l_spaceres);
		putc('\n', outf);
	fprintf(outf,"   l_flags ");
		qfs_flags(lhp->l_flags, nv_l_flags);
		putc('\n', outf);

	/* record the log pages so we can find them later */
	record_log_pages(lhp->l_blktable, lhp->l_npages);

	free(lhp);

}

banner(s)
	char *s;
{
	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                 SCANNING %-36s       *\n", s);
	fprintf(outf,"***********************************************************************\n\n");
}


/*
 * Call func on each log page.
 * This is used to dump log pages as well as floating queues.
 */
scan_logpages(func)
	void (*func)();
{
	caddr_t vaddr;
	int numpages;

	reset_log_pages();
	while (retrieve_log_pages(&vaddr, &numpages))
	{
		log_phdrP lpp;
		int i;

		lpp = (log_phdrP) newstruct(vaddr, numpages*sizeof(log_phdrT));
		for (i=0; i<numpages; i++)
			(*func)(lpp+i, lpp, vaddr, numpages);
		free(lpp);
	}
}

/*
 * Dump a single log page structure.
 * lpp is the thing to dump.
 * lb is the local block (lpp points into this).
 * vb is the virtual block.
 */
dumplogpage(lpp, lb, vb, numpages)
	log_phdrP lpp, lb, vb;
{
	int entry;

	entry = lpp - lb;
	fprintf(outf,"   logpage addr 0x%08x  blktable[%3d] of %3d     ",
		vb + entry, entry, numpages);
	display("blktable",	vb,			HEX);
	putc('\n', outf);

	fprintf(outf,"   ");
	display("lp_blkno",	lpp->lp_blkno,		DEC);
	display("lp_bp",	lpp->lp_bp,		HEX);
	display("lp_fentries",	lpp->lp_fentries,	DEC);
	putc('\n', outf);

	fprintf(outf,"   ");
	display("lp_dirtyrefs",	lpp->lp_dirtyrefs,	DEC);
	display("lp_activetr",	lpp->lp_activetr,	DEC);
	display("lp_intransit",	lpp->lp_intransit,	DEC);
	putc('\n', outf);

	fprintf(outf,"   lp_flags: ");
		qfs_flags(lpp->lp_flags, nv_lp_flags);
		putc('\n', outf);

	fprintf(outf,"   ");
	qfs_qlink("lp_dirtyq", &lpp->lp_dirtyq, &(vb+entry)->lp_dirtyq);

	fprintf(outf,"   ");
	qfs_qlink("lp_floatq", &lpp->lp_floatq, &(vb+entry)->lp_floatq);
	putc('\n', outf);
}

/*
 * Dump log pages.
 * Print a header, then cycle through log page blocks.
 */
qfs_logpages()
{
	banner("QFS LOGPAGE HDRS");
	scan_logpages(dumplogpage);
}

/*
 * Dump a floatq entry.
 * We treat the glinkT and the log entry as a unit, and print them
 * both, since (at least currently) log entries don't appear in
 * other contexts.
 * g is the floatq entry, and vg is its virtual address.
 */
dumpfloatq(g, vg)
	glinkP g, vg;
{
	log_entry_headerT *lep;

	fprintf(outf, "   ");
	qfs_qlink("gl_qlink", &g->gl_qlink, &vg->gl_qlink);
	fprintf(outf, "   gl_ptr 0x%08x (log entry follows)\n", g->gl_ptr);

	lep = (log_entry_headerT *) newstruct(g->gl_ptr,
			sizeof(log_entry_headerT));
	fprintf(outf, "   ");
	display("eh_seqnum",	lep->eh_seqnum,		DEC);
	display("eh_magic",	lep->eh_magic,		HEX);
	display("eh_size",	lep->eh_size,		DEC);
	putc('\n', outf);

	fprintf(outf, "   ");
	display("eh_trnum", 	lep->eh_trnum,		DEC);
	display("eh_opid", 	lep->eh_opid,		DEC);
	putc('\n', outf);

	fprintf(outf, "   eh_flags ");
		qfs_flags(lep->eh_flags, nv_eh_flags);
		putc('\n', outf);

	putc('\n', outf);

	free(lep);
}

/*
 * Given a log page, pull out the float queue and run through it,
 * calling dumpfloatq on each item.
 * lpp is the log page.
 * lb is the local logpage block (lpp points into this).
 * vb is the virtual logpage block.
 */
scan_floatq(lpp, lb, vb)
	log_phdrP lpp, lb, vb;
{
	qlinkP q, vq, vhead;

	/* head of queue, virtual addr */
	vhead = &(vb + (lpp-lb))->lp_floatq;

	/* current entry, virtual addr */
	vq = lpp->lp_floatq.ql_next;

	scan_list(vq, vhead, sizeof(glinkT), dumpfloatq, NULL,
		&((glinkP)0)->gl_qlink);
}

/*
 * Dump the floating queues.
 * We pull the queue head off of each log page.
 * scan_floatq runs along each queue (one per log page).
 * dumpfloatq dumps a single element of the queue.
 */
qfs_floatq()
{
	banner("QFS FLOAT QUEUES");
	scan_logpages(scan_floatq);
}

/*
 * Dump a single page link descriptor.
 * plp is the data, vplp the virtual address.
 */
dumppglink(name, plp, vplp)
	char *name;
	TrPageLinkP plp, vplp;
{
	fprintf(outf, "  ");
	qfs_qlink("qlink", &plp->tpl_qlink, &vplp->tpl_qlink);
	fprintf(outf, "  ");
	display("tpl_pageptr", plp->tpl_pageptr, HEX);
	fprintf(outf, "\n\n");
}

/*
 * Dump a transaction.
 * name names the queue.  tp is the local ptr, vtp is the virtual.
 * Each transaction has a queue of page link descriptors (pointing
 * to log page headers), which we print along with it.
 */
dumpxaction(name, tp, vtp)
	char *name;
	TrHeaderP tp, vtp;
{
	fprintf(outf, "  %s entry 0x%08x\n", name, vtp);

	fprintf(outf, "  ");
	display("th_trnumber", 		tp->th_trnumber, 	DEC);
	display("th_trlevel", 		tp->th_trlevel, 	DEC);
	display("th_logptr",		tp->th_logptr,		HEX);
	putc('\n', outf);

	fprintf(outf, "  ");
	display("th_bytesused",		tp->th_bytesused,	DEC);
	putc('\n', outf);

	fprintf(outf, "  th_spaceres ");
		qfs_resP(&tp->th_spaceres);
		putc('\n', outf);

	fprintf(outf, "  ");
	qfs_qlink("trqlink", &tp->th_trqlink, &vtp->th_trqlink);

	fprintf(outf, "  ");
	qfs_qlink("pgqlink", &tp->th_pgqlink, &vtp->th_pgqlink);
	putc('\n', outf);

	/* page links meaningless for free queue */
	if (strcmp(name, "TrFreeq") == 0)
		return;

	fprintf(outf, "  page link descriptors:\n\n");
	scan_list(tp->th_pgqlink.ql_next, &vtp->th_pgqlink,
		sizeof(TrPageLinkT), dumppglink, NULL,
		&((TrPageLinkP)0)->tpl_qlink);

	putc('\n', outf);

}

/*
 * Scan the transaction queue with the given name.
 * vhead is the virtual addr of the head of the queue.
 * Call dumpxaction on each transaction entry in queue.
 */
scan_xactionq(name, vhead)
	char *name;
	qlinkP vhead;
{
	qlinkP headp, vqp;
	TrHeaderP vtp;

	/* vhead 0 means nlist failed, somehow */
	if (vhead == 0)
	{
		fprintf(outf, "Couldn't find value for %s\n", name);
		return;
	}

	/* print list header */
	headp = (qlinkP) newstruct(vhead, sizeof(qlinkT));
	fprintf(outf, "%s\n", name);
	qfs_qlink("  head", headp, vhead);
	putc('\n', outf);

	vqp = headp->ql_next;

	scan_list(vqp, vhead, sizeof(TrHeaderT), dumpxaction, name,
		&((TrHeaderP)0)->th_trqlink);

	free(headp);
}

/*
 * Print the transaction queues.
 * We print a banner, then move through each of the three queues.
 * With each transaction, we print the queue of log page ptrs
 * attached to that transaction.
 */
qfs_xactionq()
{
	banner("QFS TRANSACTION QUEUES");
	scan_xactionq("TrFreeq", nl[X_TRFREEQ].n_value);
	scan_xactionq("TrUncommittedq", nl[X_TRUNCOMMITTEDQ].n_value);
	scan_xactionq("TrCommittedq", nl[X_TRCOMMITTEDQ].n_value);
}

/*
 * The top-level printout routine
 * QFS printout is triggered by -J ("journaled")
 * We want it optional because the code is not very bulletproof --
 * a bad address can cause analyze to stop.
 */
qfscheck()
{
	if (!Jflg)
		return;
	qfs_logpages();
	qfs_floatq();
	qfs_xactionq();
}

/*
 * Scan a circular list of objects tied together by qlinkT.
 * vqp is the virtual address of the first object.
 * vhead is the vaddr of the head of the list.
 * If vqp == vhead, the list is empty.
 * size is the size of the list item.
 * func is the function to call on each item.
 * arg is a miscellaneous argument to that function.
 * offset is the byte offset in the object of the qlinkT.
 */
scan_list(vqp, vhead, size, func, arg, offset)
	char *vqp, *vhead;
	void (*func)();
{
	char *qp;

	while (vqp != vhead)
	{
		/* escape if error */
		if (vqp == NULL)
			return;

		/* back up to real address */
		vqp -= offset;

		qp = (char *) newstruct(vqp, size);
		(*func)(arg, qp, vqp);

		/* link is offset bytes into struct */
		vqp = (char *) ((qlinkP)(qp + offset))->ql_next;

		free(qp);
	}
}



#endif /* QFS */
