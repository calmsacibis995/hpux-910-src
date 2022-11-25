/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/get.c,v $
 * $Revision: 1.66.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:31:14 $
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

extern int translate_enable;
extern vas_t *kernvasp;
extern unsigned int tt_region_addr;

u_long ltor();
u_long getphyaddr();

#ifdef __hp9000s800
get_crash_processor_table(addr)
register int addr;
{
	extern int crash_processor_table [];

	getchunk (0, addr, crash_processor_table,
		CRASH_TABLE_SIZE, "get_crash_processor_table");
}

get_crash_event_table(addr)
register int addr;
{
	extern int crash_event_table [];

	getchunk (0, addr, crash_event_table,
		CRASH_EVENT_TABLE_SIZE, "get_crash_event_table");
}

get_rpb_list()
{
	extern int rpb_list [];
	extern int crash_event_table [];
	extern int cet_entries;
	int *rpb_list_ptr;
	int i;
	struct crash_event_table_struct *cet_ptr;

	rpb_list_ptr = rpb_list;
	while (*rpb_list_ptr != 0) rpb_list_ptr++;
	cet_ptr = (struct crash_event_table_struct *)crash_event_table;
	for (i = 0; i < cet_entries; i++) {
		if (cet_ptr->cet_hpa == NULL) {
			break;
		}
		*rpb_list_ptr++ = cet_ptr->cet_savestate;
		cet_ptr++;
	}
}

/*
 * "check_rpb" is passed a pointer to a valid rpb.  It checks whether
 * the rpb is in the rpb list.  If so, it returns silently.  If not,
 * it complains bitterly and adds the rpb to the list.
 */
check_rpb(rpb_addr)
int rpb_addr;
{
	int index;
	int *rpb_list_ptr;
	extern int rpb_list [];

	rpb_list_ptr = rpb_list;
	index = 0;
	while (*rpb_list_ptr && index < MAX_RPBS) {
		if (*rpb_list_ptr == rpb_addr)
			return;
		rpb_list_ptr++;
		index++;
	}
	if (index >= MAX_RPBS) {
		fprintf(outf, "Ran out of rpb pointers.  Lost rpb address 0x%X.\n",
			rpb_addr);
		return;
	}
	*rpb_list_ptr = rpb_addr;
	fprintf(outf, "  RPB addr 0x%X was not represented in the Event list.\n",
		rpb_addr);
}

find_missing_rpbs()
{
	extern int crash_processor_table [];
	struct crash_proc_table_struct *cpt_ptr;
	struct rpb *rp = (struct rpb *)0;
	int index;
	int max_index;
	int flag_offset;
	int addr;

	flag_offset = &rp->rp_flag;
	cpt_ptr = (struct crash_proc_table_struct *)crash_processor_table;
	index = 0;
	max_index = CRASH_TABLE_SIZE / sizeof (struct crash_proc_table_struct);
	for (index = 0; index < max_index; index++) {
		if (cpt_ptr->cpt_hpa == NULL)
			break;
		if ((cpt_ptr->cpt_toc_savestate != NULL) &&
			(getreal (cpt_ptr->cpt_toc_savestate + flag_offset) == 1)) {
			check_rpb (cpt_ptr->cpt_toc_savestate);
		}
		if ((cpt_ptr->cpt_hpmc_savestate != NULL) &&
			(getreal (cpt_ptr->cpt_hpmc_savestate + flag_offset) == 1)) {
			check_rpb (cpt_ptr->cpt_hpmc_savestate);
		}
		cpt_ptr++;
	}

	addr = nl[X_PANIC_RPB].n_value;
	if (addr != 0)
		if (getreal (addr + flag_offset) == 1)
			check_rpb (addr);

	addr = nl[X_PANIC2_RPB].n_value;
	if (addr != 0)
		if (getreal (addr + flag_offset) == 1)
			check_rpb (addr);
}

getrpb(addr)
register int addr;
{
	getchunk(0, addr, rpbbuf, sizeof (struct rpb), "getrpb");
}

gethpmc(addr)
register int addr;
{
	getrpb(addr);
}

#else
getrpb()
{
	/* Nothing for now */
}
#endif

/*
 * Getchunk gets a logical address, either kernel or user, a user
 * address must pass in a non-zero space if s800, or vas address
 * if s300.
 */
getchunk(space, virt, localbuf, size, caller)
register int space;
register int virt;
register char *localbuf;
register int size;
char *caller;
{
	register int bytestomove, offset, bytesleft;
	register u_long phys;
	int errs = 0;

	bytesleft = size;	    /* get starting local buffer */
	offset = (virt & PGOFSET);  /* initial offset into page */

	while (bytesleft) {
		/*
		 * Calculate how many bytes on page to read, if more
		 * then what is left then only read in what we have to.
		 */
		bytestomove = NBPG - offset;
		if (bytestomove > bytesleft)
			bytestomove = bytesleft;

		/* Do address translation */
		phys = (u_long)ltor(space, virt);
		/* make sure its mapped */
		if (phys) {
			longlseek(fcore, phys, 0);
			if (longread(fcore, localbuf,
				bytestomove) != bytestomove) {
				errs++;
				perror("    core getchunk ");
				fprintf(outf, "    caller: %s\n", caller);
			}
		} else {
			fprintf_err();
			fprintf(outf, "  getchunk: page not mapped!!\n");
			fprintf(outf, "  space 0x%x virt 0x%x phy 0x%x\n",
			    space, virt, phys);
			fprintf(outf, "  caller: %s\n", caller);

			errs++;
		}
		/* we are page aligned now so we can set this to zero */
		offset = 0;
		virt += bytestomove;
		localbuf += bytestomove;
		bytesleft -= bytestomove;
	}
	return errs;
}

/* Get icsbase and put it into kstack array */
getics()
{
	int i, errs = 0;
	u_long aicsaddr;

	for (i = 0; i < stacksize; i ++) {
		aicsaddr = (u_long)ltor(0, stackaddr + (NBPG * i));
		if (aicsaddr) {
			/* If stack array mapped then read */
			longlseek(fcore, aicsaddr, 0);
			if (longread(fcore, (char *)&kstack[i][0], NBPG) != NBPG) {
				perror("ics-stack read");
				errs++;
			}
		} else {
			fprintf(outf, " ICS-stack page not mapped \n");
			errs++;
		}
	}
}

/* Gets the word from text, initialized data, or BSS only */
get(loc)
unsigned loc;
{
	int x;

	if (loc == 0) {
		fprintf(outf, " get: loc zero\n");
		return 0;
	}
	lseek(fcore, (long)clear(loc), 0);
	if (read(fcore, (char *)&x, sizeof (int)) != sizeof (int)) {
		perror("read");
		fprintf(outf, "get failed on %x\n", clear(loc));
		return 0;
	}
	return x;
}

/* Gets the short word from text, initialized data, or BSS only */
short
getshort(loc)
unsigned loc;
{
	short x;

	if (loc == 0) {
		fprintf(outf, " get: loc zero\n");
		return 0;
	}
	lseek(fcore, (long)clear(loc), 0);
	if (read(fcore, (char *)&x, sizeof (short)) != sizeof (short)) {
		perror("read");
		fprintf(outf, "getshort failed on %x\n", clear(loc));
		return 0;
	}
	return x;
}

/* Gets the word from physical address */
getreal(loc)
unsigned loc;
{
	int x;

	longlseek(fcore, (long)clear(loc), 0);
	if (longread(fcore, (char *)&x, sizeof (int)) != sizeof (int)) {
		perror("read");
		fprintf(outf, "getreal failed on %x\n", clear(loc));
		return 0;
	}
	return x;
}

/* Gets the short word from physical address */
short
getshortreal(loc)
unsigned loc;
{
	short x;

	longlseek(fcore, (long)clear(loc), 0);
	if (longread(fcore, (char *)&x, sizeof (short)) != sizeof (short)) {
		perror("read");
		fprintf(outf, "getshortreal failed on %x\n", clear(loc));
		return 0;
	}
	return x;
}

#ifdef __hp9000s800

/* Retrieves a pde from an equivalently mapped address */
getepde(loc, pde)
unsigned loc;
struct hpde *pde;
{
	int x;

	longlseek(fcore, (long)clear(loc), 0);
	if (longread(fcore, (char *)pde, sizeof (struct hpde)) !=
		sizeof (struct hpde)) {
		perror("read");
		fprintf(outf, "getepde failed on %x\n", clear(loc));
		return -1;
	}
	return 0;
}

#endif /* __hp9000s800 */

/*
 *	Find the pregion of a particular type.
 */
preg_t *
findpreg(pp, type)
register struct proc *pp;
register int type;
{
	register preg_t *prp, *unloc_prp;
	register struct proc *unloc_pp;
	vas_t *vas;

	unloc_pp = unlocalizer(pp, proc, vproc);

	/* Check for no pregion pointer */
	if (pp->p_vas == 0) {
		fprintf(outf, "\n");
		fprintf_err();
		fprintf(outf, " process has no pregions\n");
		return(0);
	}

	vas = GETBYTES(vas_t *, pp->p_vas, (sizeof (struct vas)));
	if (vas == NULL) {
		fprintf(outf, "findpreg: Localizing vas failed\n");
		return(0);
	}

	unloc_prp = vas->va_next;
	while (unloc_prp != (preg_t *)pp->p_vas) {
		if (unloc_prp == (preg_t *)0) {
			fprintf_err();
			fprintf(outf, " null p_region pointer");
			fprintf(outf, " in findpreg\n");
			return(0);
		}
		prp = GETBYTES(preg_t *, unloc_prp, (sizeof (struct pregion)));
		if (prp == NULL) {
			fprintf(outf, "findpreg: Localizing prp failed\n");
			return(0);
		}
		if ((prp)->p_type == type)
			return(prp);

		unloc_prp = prp->p_next;
	}

	return(0);
}

int pidarray[1024];

/* Get uarea for process */
/* REGION */
getu(p)
register struct proc *p;
{
	register int uvirt;
	register char *localbuf;
	int astackaddr, vpids;
	int diff, j, x;
	struct save_state *ss1;
	struct pregion *pr;
	struct region *r;
	struct pte *pte;
	vfd_t *vfd;
	dbd_t *db;
	dev_t dev;
	int i, w, cc, errs = 0, current = 0;

	pr = GETBYTES(preg_t *, p->p_upreg, sizeof (preg_t));
	if (pr == NULL) {
		fprintf(outf, "\ngetu: Localizing p_upreg failed\n");
		return(++errs);
	}

	/* get starting virtual address */
	uvirt = (int)pr->p_vaddr;
	fprintf(outf, "\n uvaddr 0x%08x:0x%08x ", pr->p_space, uvirt);

	/* Lets get vfds for grins, we might need them later */
	r = GETBYTES(reg_t *, pr->p_reg, sizeof (reg_t));
	if (r == NULL) {
		fprintf(outf, "\ngetu: Localizing p_reg failed!\n");
		return(++errs);
	}

	if (((p->p_flag & SLOAD) == 0) && (r->r_dbd !=0)) {
		fprintf(outf, "\nswapped\n");
		return(errs);
	}

	if ((p->p_flag & SLOAD) && (r->r_dbd !=0) && activeflg) {
		fprintf(outf, "Warning: this process is in a transient state\n");
		return(++errs);
	}

	/*
	 * get uarea
	 */
	if (p->p_flag & SLOAD) {
#ifdef __hp9000s300
		pte = GETBYTES(struct pte *, p->p_addr, sizeof (struct pte));
		if (pte == NULL) {
			fprintf(outf, "\ngetu: Localizing pte failed\n");
			return ++errs;
		}
		if (pte->pg_v & PG_IV)
			pte = GETBYTES(struct pte *, (*(u_long *)pte) & ~3, sizeof (struct pte));

		longlseek(fcore, (long)ptob(pte->pg_pfnum), 0);
		if (longread(fcore, (char *)u_area.buf,
			sizeof (struct user)) != sizeof (struct user)) {
			perror(" getu: u_area");
			errs++;
		}
#else
		errs += getchunk(pr->p_space, uvirt, (char *)u_area.buf,
			sizeof (struct user), "getu: uarea");
#endif /* s300 vs. s800 */
	} else if (fswap >= 0 && r->r_dbd == 0) {
		for (i=0, x=pr->p_off; i < btorp(sizeof (user_t)); i++, x++)  {
			vfd =(vfd_t *)findvfd(r, x);
			if (vfd == (vfd_t *)0) {
				errs++;
				break;
			}

			db = (dbd_t *)finddbd(r, x);
			if (db == (dbd_t *)0) {
				errs++;
				break;
			}

			if (vfd->pgm.pg_v) {
				/* Page still incore, get it there */
				errs += getchunk(pr->p_space, uvirt,
					(char *)u_area.buf[i], NBPG,
					"getu: uarea");
			}
		}
	}

#ifndef MP
	if (p == localizer(currentp, proc, vproc)) {
		fprintf(outf, " CURRENT PROCESS ");
		current=1;
	}
#else
	 /*
	  * Scan thru all the processor's iva info. If noproc=0
	  * then we have a valid current process
	  */
	{
	    int j;
	    for (j = 0; j < MAX_PROCS; j++) {
		if ((mp[j].prochpa == 0) || (mpiva[j].iva_noproc == 1))
		    continue;

		if (p == localizer((struct proc *)mpiva[j].procp, proc, vproc)) {
		    fprintf(outf, " CURRENT PROCESS ON PROCESSOR %d", j);
		    current=1;
	        }
	    }
	}
#endif /* MP vs. not MP */

	if (!errs) {
		if (p->p_flag & SLOAD) {
			if (p == &proc[0]) {
				fprintf(outf, "  U_COMM swapper\n");
			} else if (p == &proc[2]) {
				fprintf(outf, "  U_COMM pageout\n");
			} else if (p == &proc[3]) {
				fprintf(outf, "  U_COMM statdaemon\n");
			} else {
				fprintf(outf, "  U_COMM %.14s\n", u.u_comm);
			}

			/*
			 * print open files
			 */
			{
			    int files_printed = 0;

			    fprintf(outf, " p_cdir:  0x%08x p_rdir: 0x%08x  u_ap: 0x%08x",
				p->p_cdir, p->p_rdir, u.u_ap);
			    if (p->p_ofilep != NULL) {
				struct ofile_t *ofilep[MAXFUPLIM/SFDCHUNK];

				getchunk(KERNELSPACE, p->p_ofilep, &ofilep[0],
					 (sizeof (struct ofile_t *) * NFDCHUNKS(p->p_maxof)), "getu");

				for (i = 0; (ofilep[i] != NULL) && (i < NFDCHUNKS(p->p_maxof)); i++) {
				    int j;
				    struct ofile_t ofile;
				    getchunk(KERNELSPACE, ofilep[i], &ofile, sizeof ofile, "getu");
				    for (j = 0; j < SFDCHUNK; j++) {
					if (ofile.ofile[j]) {
					    if ((files_printed % 6) == 0)
						fprintf(outf, "\n u_ofilep ");
					    files_printed++;
					    fprintf(outf, "0x%08x ", ofile.ofile[j]);
					}
				    }
				}
			    }

			    if (files_printed == 0)
				fprintf(outf, "\n u_ofilep NONE\n");
			    else
				fprintf(outf, "\n");
			}

			fprintf(outf, " u_arg    ");
			for (i = 0; i < 5; i++) {
			    fprintf(outf, "0x%08x ", u.u_arg[i]);
			}
			fprintf(outf, "\n          ");
			for (i = 5; i < 9; i++) {
			    fprintf(outf, "0x%08x ", u.u_arg[i]);
			}
			fprintf(outf, "\n");
#ifdef __hp9000s800
			fprintf(outf, " sr5 : 0x%08x\n", u.u_pcb.pcb_sr5);
#endif /* __hp9000s800 */
			fprintf(outf, " u_cntxp 0x%08x", u.u_cntxp);
			if (DUXFLAG) {
				if (u.u_nsp) {
					fprintf(outf, " u_nsp 0x%08x", u.u_nsp);
				}
				if (u.u_site) {
					fprintf(outf, " u_site 0x%02x", u.u_site);
				}
				if (u.u_request) {
					fprintf(outf, " u_request 0x%08x", u.u_request);
				}
				fprintf(outf, "\n");
				if (u.u_duxflags)
					fprintf(outf, "\n");
				if (u.u_duxflags & DUX_UNSP)
					fprintf(outf, " DUX_UNSP");
				if (u.u_duxflags)
					fprintf(outf, "\n");
			}

			/*
			 * Print the ucred (user credentials) structure
			 */
			{
			    struct ucred cred;

			    if (getchunk(KERNELSPACE, u.u_cred, &cred, sizeof cred, "getu"))
				fprintf(outf, "\n u_cred 0x%08x: can't read u.u_cred!\n",
				    u.u_cred);
			    else
				fprintf(outf, "\n u_cred  0x%08x  cr_uid %2d  cr_gid %2d  cr_ref %2d\n",
				    u.u_cred, cred.cr_uid, cred.cr_gid, cred.cr_ref);
			}
#ifdef QFS
			qfs_ustuff();
#endif /* QFS */
			if (u.u_procp != (vproc + (p - proc))) {
				fprintf(outf, " Bad u_procp 0x%08x != p\n", u.u_procp);
				/* if outside of proc table range then
				 * this is really bad and we will
				 * filter that up
				 */
				if ((u.u_procp < vproc) ||
					(u.u_procp > vproc +nproc))
					errs++;
			}

			/* Print out the rusage fields */
			fprintf(outf, " u_ru fields:\n");
			fprintf(outf,"   ru_utime: %d.%06d ru_stime: %d.%06d\n",
			    u.u_ru.ru_utime.tv_sec, u.u_ru.ru_utime.tv_usec,
			    u.u_ru.ru_stime.tv_sec, u.u_ru.ru_stime.tv_usec);
			fprintf(outf, "   ru_maxrss: %d ru_ixrss: %d ru_idrss: %d ru_isrss: %d\n",
				u.u_ru.ru_maxrss, u.u_ru.ru_ixrss,
				u.u_ru.ru_idrss, u.u_ru.ru_isrss);
			fprintf(outf, "   ru_minflt: %d ru_majflt: %d ru_nswap: %d\n",
				u.u_ru.ru_minflt, u.u_ru.ru_majflt,
				u.u_ru.ru_nswap);
			fprintf(outf, "   ru_inblock: %d ru_oublock: %d ru_ioch: %d\n",
				u.u_ru.ru_inblock, u.u_ru.ru_oublock,
				u.u_ru.ru_ioch);
			fprintf(outf, "   ru_msgsnd: %d ru_msgrcv: %d\n",
				u.u_ru.ru_msgsnd, u.u_ru.ru_msgrcv);
			fprintf(outf, "   ru_nsignals: %d ru_nvcsw: %d ru_nivcsw: %d\n",
				u.u_ru.ru_nsignals,
				u.u_ru.ru_nvcsw, u.u_ru.ru_nivcsw);
		}
	}

	/* get kernel stack for tracing */
#ifdef __hp9000s300
	if (!errs)
#else
	if (!errs && !current)
#endif
	{
		uvirt = f_stackaddr();
		if (p->p_flag & SLOAD) {
#ifdef __hp9000s300
			/*
			 * The pte entries for the stack are contiguous with
			 * the u_area pte.
			 */
			struct pte *pte_addr = p->p_addr - KSTACK_PAGES;
			int i;

			for (i = 0; i < KSTACK_PAGES; i++, pte_addr++) {
				pte = GETBYTES(struct pte *, pte_addr, sizeof (struct pte));
				if (pte == NULL) {
					fprintf(outf,
					    "\ngetu: Localizing kstack pte %d failed\n", i);
					return ++errs;
				}

				if (pte->pg_v == 0) {
					if (i == KSTACK_PAGES-1) {
						fprintf(outf,
							"\ngetu: kstack doesn't have first page\n");
						return ++errs;
					}

					continue;
				}

				if (pte->pg_v & PG_IV)
					pte = GETBYTES(struct pte *, (*(u_long *)pte) & ~3, sizeof (struct pte));

				longlseek(fcore, (long)ptob(pte->pg_pfnum), 0);
				if (longread(fcore, &kstack[i][0], NBPG) != NBPG) {
					perror(" getu: k-stack");
					errs++;
				}
			}
#else
			errs += getchunk(pr->p_space, uvirt,
				(char *)&kstack[0][0],
				KSTACKBYTES, "getu: k-stack");
#endif /* s300 vs. s800 */
		} else if (fswap >= 0 && r->r_dbd) {
			r = GETBYTES(reg_t *, pr->p_reg, sizeof (reg_t));
			if (r == NULL) {
				fprintf(outf, "\ngetu: Localizing p_reg failed\n");
				return(errs++);
			}
			for (i = 0, x=pr->p_off; i < btorp(KSTACKBYTES); i++, x++) {
				vfd = (vfd_t *)findvfd(r, btorp(uvirt) + x);
				if (vfd == (vfd_t *)0) {
					errs++;
					break;
				}

				db = (dbd_t *)finddbd(r, btorp(uvirt) + x);
				if (db == (dbd_t *)0) {
					errs++;
					break;
				}

				if (vfd->pgm.pg_v) {
					/* Page still incore, get it there */
					errs += getchunk(pr->p_space, uvirt,
						(char *)&kstack[i][0], NBPG,
						"getu: k-stack");
				}
			}
		}

		if (!errs) {
			/* Extract the user pc address while we have it */
#ifdef __hp9000s800
			ss1 = (struct save_state *)&kstack[0][0];
			/* Don't do for system processes */
			if (p != &proc[0] && p != &proc[2] && p != &proc[3]) {
				fprintf(outf, "\n User PC queue 0x%08x, ",
					ss1->ss_pcoq_head);
				fprintf(outf, "rp  0x%08x,  sp  0x%08x\n",
					ss1->ss_rp, ss1->ss_sp);
			}
#endif
#ifdef DEBREC
			/* Initialize High level stack data structures */
			if (xdbenable)
				GetState(p);
#endif
			fprintf(outf, "\n Kernel Stack trace:\n\n");
#ifdef __hp9000s800
			/*  OBSOLETE:  stktrc(p); */
			new_stktrc2(0, 0, p);
#else
			stktrc(p);
#endif
			if (!Uflg) {
				/* This is the more compact form */
				for (i = 2 , j= 0; i <= trc[1]+1; i++) {
					if (j++ == 3) {
						j = 1;
						fprintf(outf, "\n         ");
					}
				/* look up closet symbol */
					diff = findsym(trc[i]);
					if (diff == 0xffffffff)
						fprintf(outf, "    0x%04x", trc[i]);
					else
						fprintf(outf, "    %s+0x%04x",
							cursym, diff);
				}
			}
			fprintf(outf, "\n\n");
#ifdef DEBREC
			if (xdbenable) {
			  if (Uflg) {
				fprintf(outf, "\n Attempting detailed Stack trace:\n");
				StackTrace(20, 1); /* Maximum depth 20 */
			  }
			}
#endif /* DEBREC */
		} else {
			/* Try it anyway... */
			fprintf(outf, "\n Kernel Stack trace:\n\n");
#ifdef __hp9000s800
			/*  OBSOLETE:  stktrc(p); */
			new_stktrc2(0, 0, p);
#else
			stktrc(p);
#endif
		}

	}
#ifdef __hp9000s800
	else {
		/* Try it anyway... */
		fprintf(outf, "\n Kernel Stack trace:\n\n");
		/*  OBSOLETE:  stktrc(p); */
		new_stktrc2(0, 0, p);
	}
#endif
	return errs;
}

/* This guy reads in all tables */
snapshot()
{
	int i;


	fprintf(stderr, " Reading in tables...");

#ifdef __hp9000s800
	/* read in pdir */
	longlseek(fcore, (long)(apdir), 0);
	if (longread(fcore, (char *)pdir, (scaled_npdir) * sizeof (struct hpde))
		!= (scaled_npdir) * sizeof (struct hpde)) {
		perror("pdir read");
		exit(1);
	}

	/* read in hash table */
	longlseek(fcore, (long)(ahtbl), 0);
	if (longread(fcore, (char *)htbl, nhtbl * sizeof (struct hpde))
		!= nhtbl * sizeof (struct hpde)) {
		perror("htbl read");
		exit(1);
	}

	/* read in pgtopde_table */
	lseek(fcore, (long)(vpgtopde_table), 0);
	if (read(fcore, (char *)pgtopde_table, (physmem)* sizeof (struct hpde *))
		!= (physmem) * sizeof (struct hpde *)) {
		perror("pgtopde_table read");
		exit(1);
	}
#else
	/* REG300, must update kernel Sysmap */
#endif

#ifdef iostuff
	/* longread io stuff */
	io_snapshot();
#endif

	/* longread proc table */
	longlseek(fcore, (long)clear(aproc), 0);
	if (longread(fcore, (char *)proc, nproc * sizeof (struct proc))
		!= nproc * sizeof (struct proc)) {
		perror("proc longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread file table */
	longlseek(fcore, (long)clear(afile), 0);
	if (longread(fcore, (char *)file, nfile * sizeof (struct file))
		!= nfile * sizeof (struct file)) {
		perror("file longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread run queues */
	longlseek(fcore, (char *)aqs, 0);
	if (longread(fcore, (char *)qs, (NQS * sizeof (struct prochd)))
		!= NQS * sizeof (struct prochd)) {
		perror("qs longread");
		if (!tolerate_error)
			exit(1);
	}

#ifdef __hp9000s800
	/* longread uidhash queues */
	longlseek(fcore, (char *)auidhash, 0);
	if (longread(fcore, (char *)uidhash, (UIDHSZ * sizeof (short)))
		!= UIDHSZ * sizeof (short)) {
		perror("uidhash longread");
		if (!tolerate_error)
			exit(1);
	}
#endif

	/* longread REGION tables */

	/* longread sysmap table */
	longlseek(fcore, (long)asysmap, 0);
	if (longread(fcore, (char *)sysmap, SYSMAPSIZE * sizeof (struct map))
		!= SYSMAPSIZE * sizeof (struct map)) {
		perror("sysmap longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread quad4map table */

	/* longread pfdat table */
	longlseek(fcore, (long)apfdat, 0);
	if (longread(fcore, (char *)pfdat, physmem * sizeof (struct pfdat))
		!= physmem * sizeof (struct pfdat)) {
		perror("pfdat longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread phash table */
	longlseek(fcore, (long)aphash, 0);
	if (longread(fcore, (char *)phash, (phashmask+1)* sizeof (struct pfdat **))
		!= (phashmask+1)* sizeof (struct pfdat **)) {
		perror("phash longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread phead */
	longlseek(fcore, (long)aphead, 0);
	if (longread(fcore, (char *)&phead, sizeof (struct pfdat))
		!= sizeof (struct pfdat)) {
		perror("pfdat head longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread pbad */
	longlseek(fcore, (long)apbad, 0);
	if (longread(fcore, (char *)&pbad, sizeof (struct pfdat))
		!= sizeof (struct pfdat)) {
		perror("pfdat bad longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread region active header */
	longlseek(fcore, (long)aregactive, 0);
	if (longread(fcore, (char *)&regactive, sizeof (struct region))
		!= sizeof (struct region)) {
		perror("region active header longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread region free header */
	longlseek(fcore, (long)aregfree, 0);
	if (longread(fcore, (char *)&regfree, sizeof (struct region))
		!= sizeof (struct region)) {
		perror("region free header longread");
		if (!tolerate_error)
			exit(1);
	}

	/* nextswap = (int)get(nl[X_NEXTSWAP].n_value); */
	swapwant = (int)get(nl[X_SWAPWANT].n_value);

	/* longread swaptab */
	longlseek(fcore, (long)aswaptab, 0);
	if (longread(fcore, (char *)swaptab, MAXSWAPCHUNKS * sizeof (swpt_t))
		!= MAXSWAPCHUNKS * sizeof (swpt_t)) {
		perror("swaptab longread");
		if (!tolerate_error)
			exit(1);
	}
#ifdef notdef /* JCC disabled this code for now */
	else {
		/*
		 * NOTE: This assumes that the swap devices do NOT
		 * change after analyze is started. Snapshot will not
		 * be able to handle that case !!!!
		 */
		for (i= 0; i < MAXSWAPCHUNKS; i++) {
			if ((swaptab[i].st_dev != 0) &&
				(swaptab[i].st_npgs != 0)) {
				longlseek(fcore,
				     (long)getphyaddr(swaptab[i].st_ucnt), 0);
				if (longread(fcore, (char *)usetable[i],
				     swaptab[i].st_npgs * sizeof (use_t)) !=
				     swaptab[i].st_npgs * sizeof (use_t)) {
						perror("usetable longread");
						if (!tolerate_error)
							exit(1);
				}
			}
		}
	}
#endif /* notdef, disabled by JCC */

	/* longread sysinfo */
	longlseek(fcore, (long)asysinfo, 0);
	if (longread(fcore, (char *)&sysinfo, sizeof (struct sysinfo))
		!= sizeof (struct sysinfo)) {
		perror("sysinfo longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread camap */
	longlseek(fcore, (long)acamap, 0);
	if (longread(fcore, (char *)camap, ecamx * sizeof (struct vfd))
		!= ecamx * sizeof (struct vfd)) {
		perror("camap longread");
		if (!tolerate_error)
			exit(1);
	}

#ifdef NETWORK
	if (!tolerate_error) {
		net_gettables();
		mux_gettables();
		pty_gettables();
	}
#endif /* NETWORK */


#ifdef __hp9000s800
	/* longread shm_shmem */
	longlseek(fcore, (long)clear(ashm_shmem), 0);
	if (longread(fcore, (char *)shm_shmem, nproc * shminfo.shmseg * sizeof (struct shmid_ds *))
		!= nproc * shminfo.shmseg * sizeof (struct shmid_ds *)) {
		perror("shm_shmem longread");
		if (!tolerate_error)
			exit(1);
	}
#endif

	/* longread shmem table */
	longlseek(fcore, (long)clear(ashmem), 0);
	if (longread(fcore, (char *)shmem,  shminfo.shmmni * sizeof (struct shmid_ds))
		!= shminfo.shmmni * sizeof (struct shmid_ds)) {
		perror("shmem longread");
		if (!tolerate_error)
			exit(1);
	}

#ifdef MP
	/* longread mpprocinfo */
	longlseek(fcore, (long)(amp), 0);
	if (longread(fcore, (char *)mp, (MAX_PROCS * sizeof (struct mpinfo)))
		!= (MAX_PROCS * sizeof (struct mpinfo))) {
		perror("mpproc_info longread");
		if (!tolerate_error)
			exit(1);
	}

	/* Read in mp iva structures.... */
	for (i = 0 ; i < MAX_PROCS; i++) {
		if (mp[i].prochpa != 0) {
			ampiva = (struct mp_iva *)getphyaddr(mp[i].iva);
			longlseek(fcore, (long)(ampiva - 1), 0);
			if (longread(fcore, (char *)(mpiva + i),
			sizeof (struct mp_iva)) != sizeof (struct mp_iva)) {
				perror("mp_iva longread");
				if (!tolerate_error)
					exit(1);
			}
		}
	}
#endif /* MP */

	/* longread swbufs */
	longlseek(fcore, (long)clear(abswlist), 0);
	if (longread(fcore, (char *)&bswlist, sizeof (struct buf))
		!= sizeof (struct buf)) {
		perror("bswlist longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread bfreelist */
	longlseek(fcore, (long)clear(abfreelist), 0);
	if (longread(fcore, (char *)bfreelist, 
			    BQUEUES * sizeof (struct bufqhead))
		!= BQUEUES * sizeof (struct bufqhead)) {
		perror("bfreelist longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread bufhash */
	longlseek(fcore, (long)clear(abufhash), 0);
	if (longread(fcore, (char *)bufhash, BUFHSZ * sizeof (struct bufhd))
		!= BUFHSZ * sizeof (struct bufhd)) {
		perror("bufhash longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread kmemstats itself */
	longlseek(fcore, (long)clear(akmemstats), 0);
	if (longread(fcore, (char *)kmemstats, M_LAST * sizeof (struct kmemstats))
		!= M_LAST * sizeof (struct kmemstats)) {
		perror("kmemstats longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread swbuf */
	longlseek(fcore, (long)clear(aswbuf), 0);
	if (longread(fcore, (char *)swbuf, nswbuf * sizeof (struct buf))
		!= nswbuf * sizeof (struct buf)) {
		perror("swbuf longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread inodes */
	longlseek(fcore, (long)clear(ainode), 0);
	if (longread(fcore, (char *)inode, ninode * sizeof (struct inode))
		!= ninode * sizeof (struct inode)) {
		perror("inode longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread inode hash table */
	longlseek(fcore, (long)clear(aihead), 0);
	if (longread(fcore, (char *)ihead, INOHSZ * sizeof (struct ihead))
		!= INOHSZ * sizeof (struct ihead)) {
		perror("ihead longread");
		if (!tolerate_error)
			exit(1);
	}

	longlseek(fcore, (long)clear(artable), 0);
	if (longread(fcore, (char *)rtable, RTABLESIZE * sizeof (struct rnode *))
		!= RTABLESIZE * sizeof (struct rnode *)) {
		perror("rtable longread");
		if (!tolerate_error)
			exit(1);
	}

	/* longread in directory name lookup cache */
	longlseek(fcore, (long)clear(ancache), 0);
	if (longread(fcore, (char *)ncache, ninode * sizeof (struct ncache))
		!= ninode * sizeof (struct ncache)) {
		  perror("ncache longread");
		  if (!tolerate_error)
			  exit(1);
	}

	longlseek(fcore, (long)clear(anc_hash), 0);
	if (longread(fcore, (char *)nc_hash, NC_HASH_SIZE*sizeof (struct nc_hash))
		!= NC_HASH_SIZE * sizeof (struct nc_hash)) {
		  perror("nc_hash longread");
		  if (!tolerate_error)
			  exit(1);
	}

	longlseek(fcore, (long)clear(anc_lru), 0);
	if (longread(fcore, (char *)&nc_lru, sizeof (struct nc_lru))
		!= sizeof (struct nc_lru)) {
		  perror("nc_lru longread");
		  if (!tolerate_error)
			  exit(1);
	}

	longlseek(fcore, (long)clear(ancstats), 0);
	if (longread(fcore, (char *)&ncstats, sizeof (struct ncstats))
		!= sizeof (struct ncstats)) {
		  perror("ncstats longread");
		  if (!tolerate_error)
			  exit(1);
	}


	/* longread dqfreelist */
	longlseek(fcore, (long)clear(adqfree), 0);
	if (longread(fcore, (char *)&dqfreelist, sizeof (struct dquot))
		!= (sizeof (struct dquot))) {
	    perror("dqfreelist longread");
	    if (!tolerate_error) exit(1);
	}

	/* longread dqresvlist */
	longlseek(fcore, (long)clear(adqresv), 0);
	if (longread(fcore, (char *)&dqresvlist, sizeof (struct dquot))
		!= (sizeof (struct dquot))) {
	    perror("dqresvlist longread");
	    if (!tolerate_error) exit(1);
	}


	/* longread dqhead */
	longlseek(fcore, (long)clear(adqhead), 0);
	if (longread(fcore,
			(char *)dqhead,
			(NDQHASH * sizeof (struct dqhead)))
	 	!=	(NDQHASH * sizeof (struct dqhead))) {
		perror("dqhead longread");
		if (!tolerate_error) exit(1);
	}


	if (my_site_status & CCT_CLUSTERED) {
		/* I am assuming these variables are equivalently mapped */
		mountlock = get(nl[X_MOUNTLOCK].n_value);
		mount_waiting = get(nl[X_MOUNT_WAITING].n_value);
		my_site_status = get(nl[X_MY_SITE_STATUS].n_value);
		max_nsp = get(nl[X_MAX_NSP].n_value);
		selftest_passed = get(nl[X_SELFTEST].n_value);

                vnet_bchain =  (long)nl[X_NET_BCHAIN].n_value;
                anet_bchain =  getphyaddr((unsigned)vnet_bchain);
                longlseek(fcore, (long) clear(anet_bchain), 0);
                if (longread(fcore, (char *)&net_bchain, 
					sizeof (struct bufqhead))
                        != sizeof (struct bufqhead)) {
                        perror("net_bchain read");
                        if (!tolerate_error)
                                exit(1);
                }

		/* read mbuf stats */
		lseek(fcore, (long)nl[X_DUX_MBSTAT].n_value, 0);
		if (read(fcore, (char *)&dux_mbstat, sizeof (struct dux_mbstat))
			!= sizeof (struct dux_mbstat)) {
			perror("dux_mbstat read");
			if (!tolerate_error)
				exit(1);
		}

		lseek(fcore, (long)nl[X_IN_PROT_STATS].n_value, 0);
		if (read(fcore, (char *)&inbound_opcode_stats, sizeof (struct proto_opcode_stats))
			!= sizeof (struct proto_opcode_stats)) {
			perror("dux_inbound_proto_stats read");
			if (!tolerate_error)
				exit(1);
		}

		lseek(fcore, (long)nl[X_OUT_PROT_STATS].n_value, 0);
		if (read(fcore, (char *)&outbound_opcode_stats, sizeof (struct proto_opcode_stats))
			!= sizeof (struct proto_opcode_stats)) {
			perror("dux_outbound_proto_stats read");
			if (!tolerate_error)
				exit(1);
		}

		/* read nsp table */
		longlseek(fcore, (long)clear(ansp), 0);
		if (longread(fcore, (char *)nsp, ncsp * sizeof (struct nsp))
			!= ncsp * sizeof (struct nsp)) {
			perror("nsp table read");
			if (!tolerate_error)
				exit(1);
		}

		/* read cluster table */
		longlseek(fcore, (long)clear(aclustab), 0);
		if (longread(fcore, (char *)clustab, MAXSITE * sizeof (struct cct))
			!= MAXSITE * sizeof (struct cct)) {
			perror("cluster table read");
			if (!tolerate_error)
				exit(1);
		}

		/* read using array */
		longlseek(fcore, (long)clear(ausing_array), 0);
		if (longread(fcore, (char *)using_array,
			 using_array_size * sizeof (struct using_entry))
			!= using_array_size * sizeof (struct using_entry)) {
			perror("using array read");
			if (!tolerate_error)
				exit(1);
		}

		/* read serving array */
		longlseek(fcore, (long)clear(aserving_array), 0);
		if (longread(fcore, (char *)serving_array,
			 serving_array_size * sizeof (struct serving_entry))
			!= serving_array_size * sizeof (struct serving_entry)) {
			perror("serving array read");
			if (!tolerate_error)
				exit(1);
		}
	}

	fprintf(stderr, "    done\n");
}

/*
 * Get a kernel physical address
 * More for convienence, could do ltor directly
 */
u_long
getphyaddr(vaddr)
u_long vaddr;
{
	u_long raddr;

#ifdef __hp9000s800
	raddr = ltor(0, vaddr);
	return raddr;
#else
	if (vaddr >= tt_region_addr)
		return vaddr;

	raddr = ((u_int)kernel_tablewalk(vaddr) & 0xfffff000) + (vaddr & 0xfff);
	return raddr;
#endif
}

#ifdef __hp9000s800

/* Change for 4K page */
#define COMMON_BTOP(x) (btop(x) & ~0x1)		/* Find common page */
#define IS_ODD_OFFSET(x) ((u_int)(x) & PDE_SHADOW_BOFFSET)
#define IS_ODD_PAGE(x)	 ((u_int)(x) & 0x1)	/* Is it an odd page */
#define IS_ODD_PDE(x)	 ((u_int)(x) & 0x1)	/* Is it an odd pde */

/*
 * Try out the 3 instruction hash, 4K PAGE SIZE CHANGE HERE
 */
/* Original old pdir 2K 5 instr hash */
#define pdirhash0(sid, sof)	\
	((sid << 5) ^ ((unsigned int)(sof) >> 11)  ^ \
	(sid << 13) ^ ((unsigned int)(sof) >> 19))

#ifdef SPARSE_PDIR2K

/* New pdir 2K 3 instr XOR hash */
#define pdirhash1(sid, sof)	((sid << 5) ^ ((unsigned int)(sof) >> 12))

/* New pdir 2K 3 instr ADD hash */
#define pdirhash2(sid, sof)	((sid << 5) + ((unsigned int)(sof) >> 12))

/* New pdir 2K 5 instr hash */
#define pdirhash3(sid, sof)	\
	((sid << 5) ^ ((unsigned int)(sof) >> 12)  ^ \
	(sid << 13) ^ ((unsigned int)(sof) >> 19))

/* New PCXT 3 instr hash */
#define pdirhash4(sid, sof)	((sid << 4) ^ ((unsigned int)(sof) >> 13))
#else

/* New pdir 4K 3 instr XOR hash */
#define pdirhash1(sid, sof)	((sid << 5) ^ ((unsigned int)(sof) >> 13))

/* New pdir 4K 3 instr ADD hash */
#define pdirhash2(sid, sof)	((sid << 5) + ((unsigned int)(sof) >> 13))

/* New pdir 4K 5 instr hash */
#define pdirhash3(sid, sof)	\
	((sid << 5) ^ ((unsigned int)(sof) >> 13)  ^ \
	(sid << 13) ^ ((unsigned int)(sof) >> 19))

/* New PCXT 3 instr hash */
#define pdirhash4(sid, sof)	((sid << 4) ^ ((unsigned int)(sof) >> 13))
#endif

pdirhashproc(sid, off)
unsigned sid, off;
{
	switch (pdirhash_type) {
	case 0:
		return(pdirhash0(sid, off));
	case 1:
		return(pdirhash1(sid, off));
	case 2:
		return(pdirhash2(sid, off));
	case 3:
		return(pdirhash3(sid, off));
	case 4:
		return(pdirhash4(sid, off));

	default: printf("Bad pdirhash_type, default  to 0\n");
		return(pdirhash0(sid, off));

	}
}

struct hpde tmppde;	/* temp storage for dynamic (non-array) pde's */

/*
 * Translate the virtual address to a real address, looking for both
 * valid and prevalid addresses.
 */
u_long
ltor(space, offset)
space_t space;
caddr_t offset;
{
	struct hpde *newpde;
	extern struct hpde *base_pdir;
	u_int page, pde_vpage;
	caddr_t raddr;
	int s;
	int is_odd;

	/*
	 * Basically this is ripped out of vm_machdep.c. It of course
	 * cannot use the lpa instruction, so that piece has been
	 * removed. It also compensates for htbl, and pdir being
	 * local structures, but the internal pointers are still
	 * relative to the core files pdir and hash table.
	 *
	 *     pdir to be a pointer to a local copy of the pdir.
	 *     htbl to be a pointer to a local copy of the hash table.
	 */
	if (translate_enable == 0)
		fprintf(outf, "Trying to translate address prior to tables initialized\n");
	if (vflg) {
		fprintf(outf, " htbl, 0x%x,  pdir 0x%x \n", htbl, pdir);
		fprintf(outf, " space 0x%x  offset 0x%x  ...", space, offset);
	}

        newpde=(struct hpde *)&htbl[pdirhashproc(space,(u_int)offset)&(nhtbl-1)];
	page = btop(offset);
	pde_vpage = VPAGE_TO_PDE_VPAGE(COMMON_BTOP(offset));
	is_odd = IS_ODD_OFFSET(offset);

	if (vflg)
		fprintf(outf, " hash 0x%x , commonpage 0x%x ptob 0x%x\n",
			 pdirhashproc(space, offset), page, ptob(1));

	/* Run down the chain looking for the space, offset pair. */
	for (;newpde;) {
		if (vflg) {
		    fprintf(outf, " newpde_space_e 0x%x , newpde_off_e 0x%x (newpde 0x%x)\n",
			 newpde->pde_space_e, newpde->pde_page_e, newpde);

		    fprintf(outf, " newpde_space_o 0x%x , newpde_off_o 0x%x (newpde 0x%x)\n",
			 newpde->pde_space_o, newpde->pde_page_o, newpde);
		}
		if (is_odd) {
			if (((newpde->pde_space_o) == space)
				&& ((newpde->pde_page_o) == pde_vpage)) {
				break;
			}
		} else {
			if (((newpde->pde_space_e) == space)
				&& ((newpde->pde_page_e) == pde_vpage)) {
				break;
			}
		}
		newpde = newpde->pde_next;

		/* See if end of chain */
		if (newpde == 0)
			break;

		/*
		 * Convert to internal address.
		 */

		if ((newpde < vpdir) || ((newpde - vpdir) > scaled_npdir)) {
			/*
			 * Note: the following is heavily dependent
			 * on HP-PA quadrant usage by HP-UX.  It
			 * assumes kernel data addresses reside
			 * in the first quadrant (0 - 0x3fffffff)
			 * Anything outside of this range must be
			 * a bogus pde pointer.
			 */
			if ((newpde < base_pdir) ||
			    (newpde >= (struct hpde *)0x40000000)) {
				printf("Pdir next out of range 0x%x, ret 0 \n",
				       newpde);
				return(0);
			}

			/*
			 * This must be a dynamically extended
			 * pde from an equivalently mapped
			 * page beyond the static pdir array.
			 * Currently this is only used by
			 * io_map() translations by EISA
			 * drivers on the Series 700.
			 *
			 * Note that we repeat this lookup
			 * every time we encounter a
			 * dynamically extended pde.  This
			 * is inefficient, but preserves the
			 * internal pde pointers in the core file.
			 * If this were not a goal, we could
			 * malloc a pde everytime and insert
			 * it into the pdir hash chain by modifying
			 * the previous entry's pde_next pointer.
			 */
			if (getepde(newpde, &tmppde))
				return(0);
			newpde = &tmppde;
		} else {
			newpde = (struct hpde *)&pdir[newpde - vpdir];
		}
	}

	/* did we find it or its buddy ? */
	if (newpde == 0)
		return(0);

	/* Check correct half */
	if IS_ODD_OFFSET(offset) {
		if (newpde->pde_phys_o == 0)
			return(0);
		raddr = (off_t)ptob(newpde->pde_phys_o);
	} else {
		if (newpde->pde_phys_e == 0)
			return(0);
		raddr = (off_t)ptob(newpde->pde_phys_e);
	}
	raddr = (off_t)(u_int)raddr +
			((u_int)offset & ((1 << PGSHIFT) -1));

	if (vflg) {
	    fprintf(outf, " raddr = 0x%x \n", raddr);
	}
	return (int)raddr;
}

#else /* __hp9000s300 */

/*
 * REG300, this routine uses Syssegtab to retrieve physical address.
 * The first param is left unused for the time being, and it could
 * be used to translate address by going thru the vas route.
 */
u_long
ltor(vas, virt)
struct vas *vas;
unsigned virt;
{
	struct ste  *segent, *segtbl;
	struct pte  *pteent,  *pte;
	struct vas  *lvas;
	int  pteidx, segidx, ret;
	u_int offset, *page, *pageptr;
	caddr_t raddr;
	int valid;
	int pfnum;
	u_int lpte;

	if (translate_enable == 0)
		fprintf(outf, "Trying to translate address prior to tables initialized\n");

	if (vflg)
		fprintf(outf, "ltor input: addr 0x%x  \n", virt);
	offset = (virt & PGOFSET);

	/* Add code to handle transparent translations */
	if (((vas == kernvasp) || !vas) &&
	     ((unsigned int)virt >= tt_region_addr)) {
		/* no pte, tranparently translated */
		valid = 1; /* It's in memory */
		pfnum = virt >> PGSHIFT;
		if (vflg) {
			fprintf(outf,
		       "Current page for virt=%x is transparently translated.\n", virt);
		}
	}
	else {
		if (vas != 0) {
			lvas = GETBYTES(struct vas *, vas, sizeof (struct vas));
			if (lvas == NULL) {
				fprintf(outf, "ltor: Localizing vas failed\n");
				return 0;
			}
			lpte = (u_int)local_vastopte(lvas, virt);
			pteent = (struct pte*)&lpte;
			if (pteent == NULL) {
				if (vflg) {
					fprintf(outf, "ltor: pteent is NULL \n");
				}
				return 0;
			}
			else {
				valid = pteent->pg_v;
				pfnum = pteent->pg_pfnum;
			}
		} else {
			lpte = (u_int)kernel_tablewalk(virt);
			pteent = (struct pte*)&lpte;
			if (pteent == NULL) {
				if (vflg) {
					fprintf(outf, "ltor: pteent is NULL \n");
				}
				return 0;
			} else {
				valid = pteent->pg_v;
				pfnum = pteent->pg_pfnum;
			}
		}
	}

	/* localize physical page */
	if (valid) {
		pageptr = (u_int *)ptob(pfnum);
		raddr = (caddr_t)pageptr + offset;
	} else {
		if (vflg) {
			fprintf(outf, "Warning invalid page\n");
			fprintf(outf, "addr translation failed\n");
		}
		return 0;
	}

	if (vflg) {
		fprintf(outf, "ltor output: raddr = 0x%x\n", raddr);
	}
	return (u_long)raddr;
}

kernel_tablewalk(vaddr)
register u_int vaddr;
{
    u_int ste_addr, bte_addr, pte_addr, steent, bteent, pteent;

    if (three_level_tables) {
	ste_addr = (u_int)segtable + MC68040_SEGOFF(vaddr);
	steent = *(u_int *)ste_addr;
	if (steent == 0) {
	    fprintf(outf, "kernel_tablewalk: Localizing steent failed\n");
	    return NULL;
	}

	bte_addr = (steent & SG3_FRAME) + MC68040_BTOFF(vaddr);
	bteent = getreal(bte_addr);
	if (bteent == NULL) {
	    fprintf(outf, "kernel_tablewalk: Localizing pteent failed\n");
	    return NULL;
	}

	pte_addr = (bteent & BLK_FRAME) + MC68040_PTOFF(vaddr);
	pteent = getreal(pte_addr);
    } else {
	ste_addr = (u_int)segtable + MC68030_SEGOFF(vaddr);
	steent = *(u_int *)ste_addr;

	if (steent == 0) {
	    fprintf(outf, "kernel_tablewalk: Localizing steent failed\n");
	    return NULL;
	}

	pte_addr = (steent & SG_FRAME) + MC68030_PTOFF(vaddr);
	pteent = getreal(pte_addr);
    }

    /*
     * Handle an indirect pte
     */
    if (pteent & PG_IV)
	pteent = getreal(pteent & ~3);

    return pteent;
}

local_tablewalk(segment_table, vaddr)
u_int segment_table;
register u_int vaddr;
{
    u_int ste_addr, bte_addr, pte_addr, steent, bteent, pteent;

    if (three_level_tables) {
	ste_addr = segment_table + MC68040_SEGOFF(vaddr);
	steent = getreal(ste_addr);
	if (steent == 0) {
	    fprintf(outf, "local_tablewalk: Localizing steent failed\n");
	    return NULL;
	}

	bte_addr = (steent & SG3_FRAME) + MC68040_BTOFF(vaddr);
	bteent = getreal(bte_addr);
	if (bteent == NULL) {
	    fprintf(outf, "local_tablewalk: Localizing pteent failed\n");
	    return NULL;
	}

	pte_addr = (bteent & BLK_FRAME) + MC68040_PTOFF(vaddr);
	pteent = getreal(pte_addr);
    } else {
	ste_addr = segment_table + MC68030_SEGOFF(vaddr);
	steent = getreal(ste_addr);

	if (steent == 0) {
	    fprintf(outf, "local_tablewalk: Localizing steent failed\n");
	    return NULL;
	}

	pte_addr = (steent & SG_FRAME) + MC68030_PTOFF(vaddr);
	pteent = getreal(pte_addr);
    }

    /*
     * Handle an indirect pte
     */
    if (pteent & PG_IV)
	pteent = getreal(pteent & ~3);

    return pteent;
}

local_vastopte(vas, vaddr)
vas_t *vas;
register u_int vaddr;
{
    u_int seg;

    seg = (u_int)vas->va_hdl.va_seg;
    if (seg == NULL) {
	fprintf(outf, "local_vastopte: Localizing seg failed\n");
	return NULL;
    }

    return local_tablewalk(seg, vaddr);
}

#endif /* s800 vs. s300 */

longlseek(filedes, loc)
int filedes;
char *loc;
{
	if ((filedes != fcore))
		fprintf(outf, "longlseek bad filedes\n");

	if (activeflg) {
		/*
		 * Reading from /dev/mem, no need to adjust
		 * location.
		 */
		lseek(frealcore, loc, 0);
	} else {
		/*
		 * Reading from a core file, shift down request.
		 */
		lseek(fcore, loc - loadbase, 0);
	}
}

longread(filedes, buf, nbyte)
int filedes;
char *buf;
unsigned nbyte;
{
	if ((filedes != fcore))
		fprintf(outf, "longread bad filedes\n");

	if (activeflg)
		return read(frealcore, buf, nbyte);
	else
		return read(fcore, buf, nbyte);
}

#ifdef DEBREC
/* DEBREC is only for -g debug processing */

#ifdef __hp9000s800
/* get the space from a short pointer */
/* For now the getxxx() are in trace.c, these should move here */
ldsid(adr, p)
unsigned int adr;
unsigned int p;
{
	unsigned int space;

	if (adr < 0x40000000) {
		space = KERNELSPACE ;
	} else if ((adr > 0x3fffffff) && (adr < 0x80000000)) {
		space = getsr5(p) ;
	} else if ((adr > 0x7fffffff) && (adr < 0xc0000000)) {
		space = getsr6(p) ;
	} else {
		space = getsr7(p) ;
	}

	return(space);
}

extern u_int currentrp;
extern u_int currentrp;
extern u_int currentdp;
extern u_int currentpsw;
extern u_int currentpcoq2;
extern u_int currentsr6;
extern u_int currentsr7;

/* Pick these values, either out of he current proc place holders, or the
 * current pcb */
getumrp(p)
struct proc *p;
{
	struct pcb *pcb;
	int umrp;

	/* current umrp is in rpb */
	if (p  == localizer(currentp, proc, vproc)) {
		umrp = currentrp;
	} else {
		/* otherwise look into the pcb */
		pcb = (struct pcb *)&u_area;
		umrp = pcb->pcb_r31;
	}

	return(umrp);
}

getrp(p)
struct proc *p;
{
	struct pcb *pcb;
	int rp;

	/* current rp is in rpb */
	if (p  == localizer(currentp, proc, vproc)) {
		rp = currentrp;
	} else {
		/* otherwise look into the pcb */
		pcb = (struct pcb *)&u_area;
		rp = pcb->pcb_r2;
	}

	return(rp);
}

getsr4(p)
struct proc *p;
{
	struct pregion *pr;

	struct pcb *pcb;
	int sr4;

	/* Note they want the real text SR4 */
	/* The following code is just a guess, must check and then try */
	pr = findpreg(p, PT_TEXT);
	if (pr == (struct pregion *)0) {
		printf("Bad pregion in getsr4\n");
		return(0);
	}
	sr4 = pr->p_space;

	return(sr4);
}

getsr5(p)
struct proc *p;
{
	struct pcb *pcb;
	int sr5;

	/* current sr5 is in rpb */
	/* XXXX look into, make sure they still put it there, may change if
	 * mapped files toggle space .... */
	if (p  == localizer(currentp, proc, vproc)) {
		sr5 = currentsr5;
	} else {
		/* otherwise look into the pcb */
		pcb = (struct pcb *)&u_area;
		sr5 = pcb->pcb_sr5;
	}

	return(sr5);
}

getsr6(p)
struct proc *p;
{
	struct pcb *pcb;
	int sr6;

	/* current sr6 is in rpb */
	if (p  == localizer(currentp, proc, vproc)) {
		sr6 = currentsr6;
	} else {
		/* otherwise look into the proc */
		pcb = (struct pcb *)&u_area;
		sr6 = pcb->pcb_sr6;
	}

	return(sr6);
}

getsr7(p)
struct proc *p;
{
	struct pcb *pcb;
	int sr7;

	/* current sr7 is in rpb */
	if (p  == localizer(currentp, proc, vproc)) {
		sr7 = currentsr7;
	} else {
		/* otherwise look into the pcb */
		pcb = (struct pcb *)&u_area;
		sr7 = pcb->pcb_sr7;
	}

	return(sr7);
}

getdp(p)
struct proc *p;
{
	struct pcb *pcb;
	int dp;

	/* current dp is in dpb */
	if (p  == localizer(currentp, proc, vproc)) {
		dp = currentdp;
	} else {
		/* otherwise look into the pcb */
		pcb = (struct pcb *)&u_area;
		dp = pcb->pcb_dp;
	}

	return(dp);
}

getpsw(p)
struct proc *p;
{
	struct pcb *pcb;
	int psw;

	/* current psw is in pswb */
	if (p  == localizer(currentp, proc, vproc)) {
		psw = currentpsw;
	} else {
		/* otherwise look into the pcb */
		pcb = (struct pcb *)&u_area;
		psw = pcb->pcb_psw;
	}

	return(psw);
}

getpc2(p)
struct proc *p;
{
	struct pcb *pcb;
	int pc;

	/* current sp is in rpb */
	if (p == localizer(currentp, proc, vproc)) {
		/* We are proabably in panic so hard code this to the
		 * routine that sets the sp in the rpb.
		 */
		pc = currentpcoq2;
	} else {
		/* otherwise it has to be resume */
		pcb = (struct pcb *)&u_area;
		pc = lookup("resume") + 8;
#ifdef PREEMPTISR
		/* save is a leaf routine, so we use current sp,
		 * and our callers pc (swtch.. also in rp on stack) */

		pc = lookup("swtch") + 8;
#endif
	}

	/* Check for valid pc */
	if ((unsigned)pc == 0) {
		fprintf(outf, " pc out of range 0x%x\n", pc);
		return(0);
	}

	return(pc);
}

/* Give Real sp not local one */
getusp(p)
struct proc *p;
{
	struct pcb *pcb;
	int ksp;
	int icsbase;

	stackaddr = f_stackaddr();
	stacksize = btorp(KSTACKBYTES);
	/* current sp is in rpb */
	if (p  == localizer(currentp, proc, vproc)) {
		ksp = currentsp;
	} else {
		/* otherwise look into the pcb */
		pcb = (struct pcb *)&u_area;
		ksp = pcb->pcb_sp;
	}

	/* Check for valid sp */
	icsbase = lookup("icsBase");

	/* Change our stackbase to be on the ICS if it is out of range of
	 * the kernel stack.
	 */
	if ((unsigned)ksp < f_stackaddr()) {
		stackaddr = icsbase;
		stacksize = ICS_SIZE;
		/* read in the ics stack instead untop of kstack[][] */
		getics();
	}

	if ((unsigned)ksp < f_stackaddr() &&
	    (ksp > icsbase+0x2800 || ksp < icsbase)) {
		fprintf(outf, " k-stack pointer out of range 0x%x\n", ksp);
		return(0);
	}

	return(ksp);
}

#endif /* s800 */
#endif /* DEBREC */

/*
 * Get bytes out of the core image into a local buffer.  Hash the
 * data once read so that further reads all point to the same
 * place.
 */
caddr_t
getbytes(addr, length)
char *addr;
int length;
{
	char *ptr, *aaddr;
	int  hash;
	static struct hashent {
		char *h_addr;
		struct hashent *h_next;
		char *h_data;
		int   h_len;
	} *hashtable[256];
	register struct hashent **h, *elem;

	aaddr = (!proc0) ? (char *)getphyaddr(addr) : addr;
	if (aaddr == 0)
		return(NULL);

	hash = ((int)aaddr >> 8) & 0xFF;
	h = &hashtable[hash];

	/* Search down the hash collision chain, return value if found */
	while (*h) {
		if ((*h)->h_addr == aaddr && (*h)->h_len == length)
			return((*h)->h_data);
		h = &((*h)->h_next);
	}

	/* Wandered off the end, so create a new element */
	ptr = (char *)malloc(length);
	if (!ptr) {
		fprintf(outf, "malloc failed in getbytes(0x%x, %d)\n",
			aaddr, length);
		exit(1);
	}
	elem = (struct hashent *)malloc(sizeof (struct hashent));
	if (!elem) {
		fprintf("malloc failed in getbytes(0x%x, %d)\n",
			aaddr, length);
		exit(1);
	}
	(*h) = elem;
	elem->h_next = 0;
	elem->h_addr = aaddr;
	elem->h_data = ptr;
	elem->h_len = length;

	/* Pull in the data */
	if (longlseek(fcore, (long)aaddr, 0) == -1) {
		perror("getbytes/lseek");
		exit(1);
	}
	if (longread(fcore, ptr, length) != length) {
		perror("getbytes/longread");
		if (!tolerate_error)
			exit(1);
	}
	return(ptr);
}
