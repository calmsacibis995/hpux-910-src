/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/trace.c,v $
 * $Revision: 1.34.83.3 $       $Author: root $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 16:32:15 $
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
#ifdef hp9000s800
#include <machine/frame.h>
#include <machine/save_state.h>
#else
#include <machine/trap.h>
#endif
#include "unwind.h"
#include "defs.h"
#include "types.h"
#include "externs.h"


#ifdef hp9000s800

#define KERNEL_SWITCH 1
int Reference_error = 0;
int Ignore_Bad_Stack = 0;

/* This function is really awful and needs to be changed when someone
 * has time.  The intent is to verify that the stack pointer given is
 * within the bounds of one of the known stacks---it should either
 * be within the bounds of one of the ICS stacks (you get bonus points
 * if it checks the processor and checks that it's on that processor's
 * ICS), or it should be on the crash_monarch_stack or the crash_serf_stack,
 * or it should be on this process's kernel stack.  The current simple-
 * minded check just verifies that it is non-zero, within the bounds
 * of the physical memory if it's a quadrant zero stack, or within the
 * bounds of our VM kernel stacks (and this bound is defined very loosely)
 * if it's a quadrant 2 stack.
 */
check_priv0_stack (ptr)
	unsigned int ptr;
{
	int bad;
	int space;

	bad = 0;
	space = ((ptr >> 30) & 0x3);
	if (space == 0 && (ptr < 0x8000 || ptr > 0x800000)) {
		bad = 1;
	}
	if (space == 2 && (ptr < 0x68000000 || ptr > 0x6800C000)) {
		bad = 1;
	}
	if (bad) {
		fprintf (outf, "The stack pointer value 0x%X does not seem valid.\n",
			ptr);
		fprintf (outf, "If you want to print it anyway, you should set the\n");
		fprintf (outf, "'analyze' internal variable Ignore_Bad_Stack to 1.\n");
		if (! Ignore_Bad_Stack)
			return (0);
	}
	return (1);
}

unsigned int
Localize (value) {
	return (value);
}

unsigned int
Unlocalize (value) {
	return (value);
}

/* Routine to localize pointers to analyzes data space */

#define localize(x)	((x - stackaddr) + (int)&kstack[0][0])
#define unlocalize(x)   ((x + stackaddr) - (int)&kstack[0][0])


/* set_SRs_from_proc_ptr --- We've been given a process pointer, and
 * we're supposed to set up the space registers based on this proc ptr.
 * If this is a currently running process on one of the processors,
 * get the space registers from that processor and set the registers
 * accordingly.  If it is not a currently running process, but is
 * an in-memory process, set up the registers from the PCB structure
 * in the u-area.  If it is not in core, forget it.
 */
int
set_SRs_from_proc_ptr (p)
	struct proc *p;
{
	extern int Last_rpb_sr4;
	extern int Last_rpb_sr5;
	extern int Last_rpb_sr6;
	extern int Last_rpb_sr7;
	int hpa;
	int i;
	int icsbase;
	int curProc;
	int j;
	int rpb_ptr;
	struct pcb *pcb;
	struct rpb *rrpb_ptr;
	extern int cet_entries;
	extern struct crash_event_table_struct crash_event_table [];

#ifdef MP
	curProc = 0;
	for (i = 0; i < MAX_PROCS; i++) {
		if ( mpiva[i].procp == p ) {
			int mpindex;

			curProc = i + 1;
			icsbase = mpiva[curProc - 1].ibase;
			mpindex = (mpiva[curProc - 1].procindex);
			hpa = (int) (mp[mpindex].prochpa);
			rpb_ptr = 0;
			for (j = 0; j < cet_entries; j++) {
				if (crash_event_table [j].cet_hpa == hpa) {
					rpb_ptr = crash_event_table [j].cet_savestate;
					break;
				}
				if (crash_event_table [j].cet_hpa == 0) {
					break;
				}
			}
			if (rpb_ptr != 0) {
				getrpb(rpb_ptr);
			} else {
				fprintf(outf, "Can't find RPB for HPA == 0x%X!\n", hpa);
			}
			rrpb_ptr = (struct rpb *) rpbbuf;
			Last_rpb_sr4 = rrpb_ptr->rp_sr4;
			Last_rpb_sr5 = rrpb_ptr->rp_sr5;
			Last_rpb_sr6 = rrpb_ptr->rp_sr6;
			Last_rpb_sr7 = rrpb_ptr->rp_sr7;
			break;
		}
	}
	if ( !curProc ) {
		/* When does u_area get set? */
		pcb = (struct pcb *)&u_area;
		Last_rpb_sr4 = pcb->pcb_sr4;
		Last_rpb_sr5 = pcb->pcb_sr5;
		Last_rpb_sr6 = pcb->pcb_sr6;
		Last_rpb_sr7 = pcb->pcb_sr7;
	}
#else  MP
	/* This is broken.  Don't know if or when it will be fixed. */
#endif MP
}

unsigned int
Reference (ptr)
	register int ptr;
{
	register int space;
	register int space_val;
	int local_value;
	extern int Last_rpb_sr4;
	extern int Last_rpb_sr5;
	extern int Last_rpb_sr6;
	extern int Last_rpb_sr7;

	space = ((ptr >> 30) & 0x3);
	switch (space) {
		case 0:
			space_val = Last_rpb_sr4;
			break;

		case 1:
			space_val = Last_rpb_sr5;
			break;

		case 2:
			space_val = Last_rpb_sr6;
			break;

		case 3:
			space_val = Last_rpb_sr7;
			break;
	}

	if (getchunk (space_val, ptr, &local_value, 4, "Reference")) {
		Reference_error = 1;
		fprintf(outf, "Stack trace Reference Error.\n");
		return (-1);
	} else {
		return (local_value);
	}
}


/*===========================NEW VERSION==================================*/
int *
new_stktrc2 (ipc, isp, p)
	int ipc;
	int isp;
	struct proc *p;
{
	/*
	 *
	 * NOTE that integer overflow can occur while calculating the
	 *      hash.  This is acceptable and it is assumed that this
	 *      will NOT cause a processor exception.
	 */

/* define OVRHD 2	array also contains trace hash & track length  */
/* define MAXSTKTRC 30	only trace the stack this far (look in defs.h) */

/* ADDPC adds the integer pc value to the stack trace begin collected */
#define ADDPC(pc) \
	trc[count] = pc; \
	trc[0] = (trc[0] << 1) + trc[count];	/* calculate hash */ \
	count++

	register int count;
	int cntval = 30;	/* limit unwind to 30 deep */
	struct frame_marker *fmp;
	struct save_state *svsp;
	struct unwindDesc unwdesc;
	unsigned    rp,pc, sp, n, i, diff;
	unsigned    tmp;
	unsigned    prev_sp;
	struct proc *real_p;
	int *args;

	trc[0] = 0;
	count = OVRHD;


	real_p = (struct proc *) unlocalizer(p,proc,vproc);

	if (ipc == 0 && isp == 0 && p != NULL) {
		/* Get initial starting values */
		pc = getpc(real_p);
		sp = unlocalize (getsp(real_p));
		set_SRs_from_proc_ptr (real_p);
	} else {
		pc = ipc;
		sp = isp;
	}

	if (!(check_priv0_stack (sp)))
		goto out;

	/* Get the initial frame marker */
	fmp = sptofm(sp);

	if (Uflg)
		fprintf(outf,"  starting sp=0x%x\n", Unlocalize(sp));


	/* get the unwind descriptor (if its the first time read in the
	 * entire unwind table)
	 */
	if (!getUnwEntry(pc,&unwdesc)) goto out;

	/*
	 * roll back sp if we're not at entry point
	 * (Analyze will never be at the start of a routine.)
	 */

#ifdef KERNEL_SWITCH
	if (unwdesc.start_ofs != (pc & ~0x3)) {
#else
	if (unwdesc.start_ofs != pc) {
#endif
		/* If we saved the psp in the frame use it, else roll
		  * back this frame.
		  */
		if (unwdesc.save_sp)
			sp = Reference(&fmp->fm_psp);
		 else sp = sp - (unwdesc.frame_size << 3);
	 }


	prev_sp = 0xFFFF0000;
	while (cntval--) {
		if (Reference_error) {
			Reference_error = 0;
			fprintf(outf, "  Stack trace error, terminating.\n");
			goto out;
		}

		if (sp == prev_sp) {
			fprintf(outf, "  Stack pointer did not change from last iteration of loop---terminating.\n");
			goto out;
		}
		prev_sp = sp;

		if (Uflg){
			/* Arguments are stored -36 off sp */
			args = (int *)(sp - 36);
			diff = findsym(pc);
			if (diff == 0xffffffff)
				fprintf(outf,"  0x%04x( ",pc);
			else
				fprintf(outf,"  %s+0x%x ( ",cursym,
					diff);
			if ((unwdesc.args_stored) &&
				(unwdesc.start_ofs != pc)) {
				/* dump arguments */

				for (i = 0; i < 4; i++){
					if (i != 3)
					     fprintf(outf,"0x%08x ,",
						Reference((args -i)));
					else
					      fprintf(outf,"0x%08x )\n",
						Reference((args -i)));
				}


			} else{
				fprintf(outf,"arguments not stored )\n");
			}

			if (Uflg){
				fprintf(outf, "        pc=0x%x, pfmp=0x%x, psp=0x%x\n",
				pc,  sptofm(Unlocalize(sp)),
				Unlocalize(sp));
			}
		}

		/* Check for termination condition */
		if (Unlocalize(sp) == 0)
			break;
		else {
			/* Log pc in trace buffer */
			ADDPC(pc);
		}

		/* Sanity always prevails, look for problems now */
		/* HAHHH !  - JFB  */
		if (!(check_priv0_stack (sp)))
			goto out;

		fmp = sptofm(sp);
#ifdef KERNEL_SWITCH
		if (unwdesc.start_ofs == (pc & ~0x3)){
			pc = Reference((sp - 20) & ~3);
#else
		if (unwdesc.start_ofs == pc){
			/* If this were not the kernel
			* pc = *(int *)((sp - 20) & ~3);
			*/
			fprintf(outf," pc at start of routine??\n");
#endif

		/* Analyze should not see this, if in  */
		/* frame it could be picked up from sp???     */
		} else if (unwdesc.save_mrp_in_frame) {
			/* if (!(pc=get(sp,DSP) & ~3)){ */
			   pc = Reference(sp);
			   if (!pc || pc == 0xFFFFFFFF) {
				printf("can't unwind -- save_mrp_in_frame at 0x%X\n", sp);
				goto out;
			}
		} else if (unwdesc.save_rp) {
			pc = Reference(&fmp->fm_crp);
			if (!pc || pc == 0xFFFFFFFF) {
				printf("can't unwind -- save_rp at 0x%X\n",
					Unlocalize(((int)&fmp->fm_crp)));
				goto out;
			 }
		} else {
			printf(" Analyze crossing trap marker\n");
		}



		/* we have a new pc, so get the next descriptor */
		if (!getUnwEntry(pc,&unwdesc)) goto out;


		/* Check for interrupt linkage */
		if (unwdesc.hpux_int){
			/* Also round to quad boundary */
			svsp = sp -
				((((unwdesc.frame_size << 3) +15) /16) * 16);
			if (Uflg)
				fprintf(outf,"  trap marker save state 0x%x  sp 0x%x framesize 0x%x \n",
				Unlocalize((unsigned)svsp),Unlocalize(sp),
				 ((((unwdesc.frame_size << 3) +15)/16)*16));
			tmp = Reference (&svsp->ss_flags);
			if (!(tmp & SS_PSPKERNEL))
				break;
			if (svsp < (struct save_state *)Localize(stackaddr))
				break;
			/* log interrupt frame */
			ADDPC(pc);

			/* get faulting pc, and sp */
			pc = Reference (&svsp->ss_pcoq_head) & ~3;
			rp = Reference (&svsp->ss_rp) & ~3;
			sp = Localize(Reference(&svsp->ss_sp));

			if (vflg){
				fprintf(outf," save state pc 0x%x rp 0x%x sp 0x%x\n",pc, rp, sp);
			}

			/* we have a new pc, so get the next descriptor */
			if (!getUnwEntry(pc,&unwdesc)) goto out;

			if ((unwdesc.frame_size == 0) ||
				(unwdesc.start_ofs == pc)){


				/* Arguments are stored -36 off sp */
				args = (int *)(sp - 36);
				diff = findsym(pc);
				if (diff == 0xffffffff)
					fprintf(outf,"  0x%04x( ",pc);
				else
					fprintf(outf,"  %s+0x%x ( ",cursym,
						diff);
				if ((unwdesc.args_stored) &&
					(unwdesc.start_ofs != pc)) {
					/* dump arguments */

					for (i = 0; i < 4; i++){
						if (i != 3)
						fprintf(outf,"0x%08x ,",
							Reference((args -i)));
						else
						fprintf(outf,"0x%08x )\n",
							Reference((args -i)));
					}


				} else{
					fprintf(outf,"arguments not stored )\n");
				}

				/* We are a leaf routine, so push us past */
				if (vflg)
					fprintf(outf, "    leaf routine pc=0x%x\n", pc);
				ADDPC(pc);
				pc = Reference(&svsp->ss_rp);
				if (!getUnwEntry(pc,&unwdesc)) goto out;
			}
			sp = sp - (unwdesc.frame_size << 3);

		/* Check if sp saved in frame, if so grab it */
		} else if (unwdesc.save_sp){
			sp = Localize(Reference(&fmp->fm_psp));
		} else {
			sp = sp - (unwdesc.frame_size << 3);
		}
	}
out:
	trc[1] = count - OVRHD; /* trace nesting depth */
	return(0);

}

arb_stktrc (pc_space, pc, sp_space, sp)
	int pc_space;
	int pc;
	int sp_space;
	int sp;
{
	extern int Last_rpb_sr4;
	extern int Last_rpb_sr5;
	extern int Last_rpb_sr6;
	extern int Last_rpb_sr7;
	int space;
	int oldUflg;

	/*DEBUG*/fprintf(outf,"DEBUG: arbitrary stack trace on pc = 0x%X.0x%X, sp = 0x%X.0x%X.\n", pc_space, pc, sp_space, sp);

	oldUflg = Uflg;
	Uflg = 1;
	space = ((pc >> 30) & 0x3);
	switch (space) {
		case 0:
			Last_rpb_sr4 = pc_space;
			break;

		case 1:
			Last_rpb_sr5 = pc_space;
			break;

		case 2:
			Last_rpb_sr6 = pc_space;
			break;

		case 3:
			Last_rpb_sr7 = pc_space;
			break;
	}

	space = ((sp >> 30) & 0x3);
	switch (space) {
		case 0:
			Last_rpb_sr4 = sp_space;
			break;

		case 1:
			Last_rpb_sr5 = sp_space;
			break;

		case 2:
			Last_rpb_sr6 = sp_space;
			break;

		case 3:
			Last_rpb_sr7 = sp_space;
			break;
	}
	new_stktrc2 (pc, sp, 0);
	Uflg = oldUflg;
}


/*===========================NEW VERSION==================================*/

/*
 *
 * STKTRC - trace the kernel stack and return the trace.
 *
 * Returns an array containing pc addresses in the stack frames in the
 * kernel stack, exclusive of the current frame.
 *
 * The integer array contains:
 *	array[0] = hash of stack trace
 *	array[1] = depth of stack trace
 *	array[2] = deepest (most recent) pc addr in trace
 *	   .
 *	   .
 *	array[array[1]+1] = highest (least recent) pc addr in trace
 *
 * Note that the length of the returned array is (array[1] + 2).
 *
 * Side Effects:  Returned array is overwritten by subsequent calls.
 *
 */

int *
stktrc(p)
struct proc *p;
{
	/*
	 *
	 * NOTE that integer overflow can occur while calculating the
	 *      hash.  This is acceptable and it is assumed that this
	 *      will NOT cause a processor exception.
	 */

/* define OVRHD 2	array also contains trace hash & track length  */
/* define MAXSTKTRC 30	only trace the stack this far (look in defs.h) */

/* ADDPC adds the integer pc value to the stack trace begin collected */
#define ADDPC(pc) \
	trc[count] = pc; \
	trc[0] = (trc[0] << 1) + trc[count];	/* calculate hash */ \
	count++

	register int count;
	int adrval = 0;
	int cntval = 30;	/* limit unwind to 30 deep */
	struct frame_marker *fmp;
	struct save_state *svsp;
	struct unwindDesc unwdesc;
	unsigned    rp,pc, sp, n, i, diff;
	unsigned    prev_sp;
	int *args;

	trc[0] = 0;
	count = OVRHD;


	if (adrval) {
		/* Currently analyze does not support user supplied values */
		pc = (unsigned)-1;	/* don't know */
		sp = adrval;
	} else {
		/* Get initial starting values */
		pc = getpc(p);
		sp = getsp(p);
	}

	if ((sp < &kstack[0][0]) || (sp > &kstack[stacksize][NBPG])){
		fprintf(outf," k-stack pointer out of range 0x%x\n",sp);
		goto out;
	}

	/* Get the initial frame marker */
	fmp = sptofm(sp);

	if (Uflg)
		fprintf(outf,"  starting sp=0x%x\n", unlocalize(sp));


	/* get the unwind descriptor (if its the first time read in the
	 * entire unwind table)
	 */
	if (!getUnwEntry(pc,&unwdesc)) goto out;

	/*
	 * roll back sp if we're not at entry point
	 * (Analyze will never be at the start of a routine.)
	 */

	if (unwdesc.start_ofs != pc) {
		/* If we saved the psp in the frame use it, else roll
		  * back this frame.
		  */
		if (unwdesc.save_sp)
			sp = localize(fmp->fm_psp);
		 else sp = sp - (unwdesc.frame_size << 3);
	 }


	prev_sp = 0xFFFF0000;
	while (cntval--) {

		if (sp == prev_sp) {
			fprintf(outf, "  Stack pointer did not change from last iteration of loop---terminating.\n");
			goto out;
		}
		prev_sp = sp;

		if (Uflg){
			/* Arguments are stored -36 off sp */
			args = (int *)(sp - 36);
			diff = findsym(pc);
			if (diff == 0xffffffff)
				fprintf(outf,"  0x%04x( ",pc);
			else
				fprintf(outf,"  %s+0x%x ( ",cursym,
					diff);
			if ((unwdesc.args_stored) &&
				(unwdesc.start_ofs != pc)) {
				/* dump arguments */

				for (i = 0; i < 4; i++){
					if (i != 3)
					     fprintf(outf,"0x%08x ,",
						*(args -i));
					else
					      fprintf(outf,"0x%08x )\n",
						*(args -i));
				}


			} else{
				fprintf(outf,"arguments not stored )\n");
			}

			if (Uflg){
				fprintf(outf, "        pc=0x%x, pfmp=0x%x, psp=0x%x\n",
				pc,  sptofm(unlocalize(sp)),
				unlocalize(sp));
			}
		}

		/* Check for termination condition */
		if (unlocalize(sp) == 0)
			break;
		else {
			/* Log pc in trace buffer */
			ADDPC(pc);
		}

		/* Sanity always prevails, look for problems now */
		if ((sp < &kstack[0][0]) || (sp > &kstack[stacksize][NBPG])){
			fprintf(outf," stack pointer out of range 0x%x\n",sp);
			fprintf(outf," possible cross-over from interrupt stack\n");
			goto out;
		}

		fmp = sptofm(sp);
		if (unwdesc.start_ofs == pc){
			/* If this were not the kernel
			pc = *(int *)((sp - 20) & ~3);
			*/
			fprintf(outf," pc at start of routine??\n");

		/* Analyze should not see this, if in  */
		/* frame it could be picked up from sp???     */
		} else if (unwdesc.save_mrp_in_frame) {
			/* if (!(pc=get(sp,DSP) & ~3)){ */
			   if (!(pc = *(int *)sp)){
				printf("can't unwind -- save_mrp_in_frame at 0x%X\n", sp);
				goto out;
			}
		} else if (unwdesc.save_rp) {
			if (!(pc=fmp->fm_crp)) {
				printf("can't unwind -- save_rp at 0x%X\n",
					unlocalize(((int)&fmp->fm_crp)));
				goto out;
			 }
		} else {
			printf(" Analyze crossing trap marker\n");
		}



		/* we have a new pc, so get the next descriptor */
		if (!getUnwEntry(pc,&unwdesc)) goto out;


		/* Check for interrupt linkage */
		if (unwdesc.hpux_int){
			/* Also round to quad boundary */
			svsp = sp -
				((((unwdesc.frame_size << 3) +15) /16) * 16);
			if (Uflg)
				fprintf(outf,"  trap marker save state 0x%x  sp 0x%x framesize 0x%x \n",
				unlocalize((unsigned)svsp),unlocalize(sp),
				 ((((unwdesc.frame_size << 3) +15)/16)*16));
			if (!(svsp->ss_flags & SS_PSPKERNEL))
				break;
			if (svsp < (struct save_state *)localize(stackaddr))
				break;
			/* log interrupt frame */
			ADDPC(pc);

			/* get faulting pc, and sp */
			pc = svsp->ss_pcoq_head & ~3;
			rp = svsp->ss_rp & ~3;
			sp = localize(svsp->ss_sp);

			if (vflg){
				fprintf(outf," save state pc 0x%x rp 0x%x sp 0x%x\n",pc, rp, sp);
			}

			/* we have a new pc, so get the next descriptor */
			if (!getUnwEntry(pc,&unwdesc)) goto out;

			if ((unwdesc.frame_size == 0) ||
				(unwdesc.start_ofs == pc)){


				/* Arguments are stored -36 off sp */
				args = (int *)(sp - 36);
				diff = findsym(pc);
				if (diff == 0xffffffff)
					fprintf(outf,"  0x%04x( ",pc);
				else
					fprintf(outf,"  %s+0x%x ( ",cursym,
						diff);
				if ((unwdesc.args_stored) &&
					(unwdesc.start_ofs != pc)) {
					/* dump arguments */

					for (i = 0; i < 4; i++){
						if (i != 3)
						fprintf(outf,"0x%08x ,",
							*(args -i));
						else
						fprintf(outf,"0x%08x )\n",
							*(args -i));
					}


				} else{
					fprintf(outf,"arguments not stored )\n");
				}

				/* We are a leaf routine, so push us past */
				if (vflg)
					fprintf(outf, "    leaf routine pc=0x%x\n", pc);
				ADDPC(pc);
				pc = svsp->ss_rp;
				if (!getUnwEntry(pc,&unwdesc)) goto out;
			}
			sp = sp - (unwdesc.frame_size << 3);

		/* Check if sp saved in frame, if so grab it */
		} else if (unwdesc.save_sp){
			sp = localize(fmp->fm_psp);
		} else {
			sp = sp - (unwdesc.frame_size << 3);
		}
	}
out:
	trc[1] = count - OVRHD; /* trace nesting depth */
	return(0);
}


/*
 * getUnwEntry - Lookup entry in unwind table for passed location.
 */
getUnwEntry(pc,unwdesc)
unsigned int pc;
struct unwindDesc *unwdesc;
{
	register struct unwindDesc *uwp;
	u_int tableofs;
	static struct unwindDesc *cache = (struct unwindDesc *) -1;
	u_int endofs;

	if (cache == (struct unwindDesc *) -1) {
		if (!(tableofs = lookup("$UNWIND_START")) ||
		    !(endofs = lookup("$UNWIND_END"))) {
			printf("\n\ncan't unwind -- no_unwind_table\n");
			return(0);
		}
		if (!(cache = (struct unwindDesc *)malloc(endofs-tableofs+8))){
			fprintf(stderr,"Couldn't get space for unwind descriptors\n");
			return(0);
		}

		lseek(fcore, (long)tableofs,0);
		if (read(fcore, cache, endofs-tableofs) < endofs-tableofs){
			fprintf(stderr,"Trouble reading unwind desc, errno = %d\n", errno);
			return(0);
		}
		uwp = &cache[(endofs-tableofs)/sizeof(struct unwindDesc)];
		uwp->start_ofs = 0;
	}
	for (uwp=(struct unwindDesc *)cache; uwp->start_ofs; uwp++)
		if (pc >= uwp->start_ofs && pc <= uwp->end_ofs) {
			*unwdesc=*uwp;
			return(1);
		}
	printf("\n\ncan't unwind -- no_entry for pc 0x%x in unwind table\n",
		pc);
	return(0);
}



#define RPB_SP 32
#define RPB_PCOQ 52

getsp(p)
struct proc *p;
{
	struct pcb *pcb;
	int ksp;
	int icsbase;
	int i, curProc = 0;
	int rpb_ptr;
	int hpa;
	int j;
	extern int cet_entries;
	extern struct crash_event_table_struct crash_event_table [];
	struct rpb *rrpb_ptr;

	stackaddr = f_stackaddr();
	stacksize = btorp(KSTACKBYTES);
	/* current sp is in rpb */
#ifdef	MP
	for (i = 0; i < MAX_PROCS; i++) {
		if ( mpiva[i].procp == p ) {
			int mpindex;

			curProc = i + 1;
			mpindex = (mpiva[curProc - 1].procindex);
			icsbase = mpiva[curProc - 1].ibase;
			hpa = (int) (mp[mpindex].prochpa);
			rpb_ptr = 0;
			for (j = 0; j < cet_entries; j++) {
				if (crash_event_table [j].cet_hpa == hpa) {
					rpb_ptr = crash_event_table [j].cet_savestate;
					break;
				}
				if (crash_event_table [j].cet_hpa == 0) {
					break;
				}
			}
			if (rpb_ptr != 0) {
				getrpb(rpb_ptr);
			} else {
				fprintf(outf, "Can't find RPB for HPA == 0x%X!\n", hpa);
			}
			rrpb_ptr = (struct rpb *)rpbbuf;
			if (rrpb_ptr->rp_crash_type == CT_PANIC) {
				ksp = rrpb_ptr->sp;
			} else {
				ksp = rrpb_ptr->rp_sp;
			}
			break;
		}
	}
	if ( !curProc ) {
		pcb = (struct pcb *)&u_area;
		ksp = pcb->pcb_sp;
	}
#else	! MP
	if (p  == localizer(currentp,proc,vproc)){
		ksp = currentsp;
	} else {
		/* otherwise look into the pcb */
		pcb = (struct pcb *)&u_area;
		ksp = pcb->pcb_sp;
	}

	/* Check for valid sp */
	icsbase = lookup("icsBase");
#endif	! MP

	/* Change our stackbase to be on the ICS if it is out of range of
	 * the kernel stack.
	 */
	if( (unsigned)ksp < f_stackaddr()){
		stackaddr = icsbase;
		stacksize = ICS_SIZE;
		/* read in the ics stack instead untop of kstack[][] */
		getics();
	}

	if (((unsigned)ksp < f_stackaddr()) && ((ksp > icsbase+(ICS_SIZE*NBPG))
	     || ( ksp < icsbase ))){
		fprintf(outf," k-stack pointer out of range 0x%x\n",ksp);
		return(0);
	}

	return(localize(ksp));
}


getpc(p)
struct proc *p;
{
	struct pcb *pcb;
	int pc;
	int i, curProc = 0;
	int j;
	int hpa;
	int rpb_ptr;
	extern int cet_entries;
	extern struct crash_event_table_struct crash_event_table [];
	struct rpb *rrpb_ptr;

	/* current sp is in rpb */
#ifdef	MP
	for (i = 0; i < MAX_PROCS; i++) {
		if ( mpiva[i].procp == p ) {
			int mpindex;

			curProc = i + 1;
			mpindex = (mpiva[curProc - 1].procindex);
			hpa = (int) (mp[mpindex].prochpa);
			rpb_ptr = 0;
			for (j = 0; j < cet_entries; j++) {
				if (crash_event_table [j].cet_hpa == hpa) {
					rpb_ptr = crash_event_table [j].cet_savestate;
					break;
				}
				if (crash_event_table [j].cet_hpa == 0) {
					break;
				}
			}
			if (rpb_ptr != 0) {
				getrpb(rpb_ptr);
			} else {
				fprintf(outf, "Can't find RPB for HPA == 0x%X!\n", hpa);
			}
			rrpb_ptr = (struct rpb *)rpbbuf;
			if (rrpb_ptr->rp_crash_type == CT_PANIC) {
				pc = rrpb_ptr->rp_rp;
			} else {
				pc = rrpb_ptr->rp_pcoq_head;
			}
			break;
		}
	}
	if ( ! curProc ) {
#else	! MP
	if (p == localizer(currentp,proc,vproc)){
		/* We are proabably in panic so hard code this to the
		 * routine that sets the sp in the rpb.
		 */
		pc = currentpcoq;
	} else {
#endif	! MP
		/* otherwise it has to be resume */
		pcb = (struct pcb *)&u_area;

#ifdef	MP
#define	DEFAULT_RTN	"save"
#else	/* ! MP */
#define	DEFAULT_RTN	"resume"
#endif	/* ! MP */
#ifdef DEBREC
		pc = lookup(DEFAULT_RTN) + 60;
#else
#ifdef KERNEL_SWITCH
		pc = lookup(DEFAULT_RTN);
#else
		pc = lookup(DEFAULT_RTN) + 4;
#endif
#endif
	}

	/* Check for valid pc */
	if ((unsigned)pc == 0) {
		fprintf(outf," pc out of range 0x%x\n",pc );
		return(0);
	}

	return(pc);
}
#endif


#ifdef __hp9000s300

#define localize(x)	((x - stackaddr) + (int)&kstack[0][0])
#define unlocalize(x)	((x + stackaddr) - (int)&kstack[0][0])

#define CALL2	0x4E90		/* jsr a0@	*/
#define CALL6	0x4EB9		/* jsr <adr>	*/
#define ADDQL	0x500F		/* addql #x,sp	*/
#define ADDL	0xDEFC		/* addl #X,sp	*/
#define LEA	0x4fef		/* lea X(sp),sp */
#define ISYM	2
#undef BSR
#define BSR	0x6100
#undef BSRL
#define BSRL    0x61FF          /* bsr address w/32 bit displacement */


#define ISP 1
#define DSP 0

arb_stktrc(pc_space, pc, sp_space, sp)
	int pc_space;
	int pc;
	int sp_space;
	int sp;
{
	fprintf(outf,"Arbitrary stack traces not implemented on the S300.\n");
}

int *
stktrc(pp)
struct proc *pp;
{
	/*
	 * NOTE that integer overflow can occur while calculating the
	 *	hash.  This is acceptable and it is assumed that this
	 *	will NOT cause a processor exception.
	 */

/* ADDPC adds the integer pc value to the stack trace begin collected */
#define ADDPC(pc) \
	trc[count] = pc; \
	trc[0] = (trc[0] << 1) + trc[count];	/* calculate hash */ \
	count++

	register int count;
	int adrval = 0;
	int cntval = 30;	/* limit unwind to 30 deep */
	unsigned    rp, pc, sp, link, n, i, diff;
	int *args;

	register long	rtn, stkptr, inst;
	long		calladr, entadr;
	int		argn;

	trc[0] = 0;
	count = OVRHD;

	if (adrval)
	{
		/*
		 * Currently analyze does not support user supplied
		 * values
		 */
		pc = (unsigned)-1;	/* don't know */
		sp = adrval;
	}
	else
	{
		/*
		 * Get initial starting values, stack values
		 * are localized
		 */
		pc = getpc(pp);
		sp = getsp(pp);
		link = getlink(pp);
	}

	/* Compare with local uarea */
	if (sp < (u_int)&kstack[0][0] ||
	    sp >= (u_int)&kstack[stacksize][0] + ptob(stacksize))
	{
		fprintf(outf, "\tk-stack pointer out of range 0x%x\n", sp);
		goto out;
	}

	/* Compare with local uarea */
	if (link < (u_int)&kstack[0][0] ||
	    link >= (u_int)&kstack[stacksize][0] + ptob(stacksize))
	{
		fprintf(outf, "\tlink pointer out of range 0x%x\n", link);
		goto out;
	}

	if (vflg)
		fprintf(outf, "\tStarting linkptr 0x%x\n\n", unlocalize(link));

	while (cntval--)
	{
		/* Stk ptr to link value */
		stkptr = link; calladr = -1; entadr = -1;

		/* Fetch link value, and localize */
		link = fetch(stkptr, DSP, 4);
		link = localize(link);

		/* Increment stack pointer to point to return value */
		stkptr += 4;
		if (link == 0)
		{
			fprintf(outf, "\tno subroutine on stack\n");
			break;
		}

		/* Fetch rp value */
		rtn = fetch(stkptr, DSP, 4);

		/*
		 * Try fetching the instr at various offsets,
		 * in order: -6, -4, -2, determining the type of
		 * instruction.
		 */
		if (fetch(rtn - 6, ISP, 2) == CALL6)
		{
			calladr = rtn - 6;
			entadr = fetch(rtn - 4, ISP, 4);
		}
		else if (fetch(rtn - 6, ISP, 2) == BSRL)
		{
			calladr = rtn - 6;
			entadr = fetch(rtn-4, ISP, 4) + (rtn - 4);
		}
		else if ((fetch(rtn - 4, ISP, 2) & ~0xffL) == BSR)
		{
			calladr = rtn - 4;
			entadr = (short)(fetch(rtn-2, ISP, 2)) + rtn - 2;
		}
		else if ((fetch(rtn - 2, ISP, 2) & ~0xffL) == BSR)
		{
			calladr = rtn - 2;
			entadr = (char)(fetch(rtn-2, ISP, 2)) + rtn;
		}
		else if (fetch(rtn - 2, ISP, 2) == CALL2)
		{
			calladr = rtn - 2;
		}

		/*
		 * Try fetching the instr at rtn,
		 * determine # of arguments
		 */
		inst = fetch(rtn, ISP, 2);
		if ((inst & 0xF13F) == ADDQL)
		{
			argn = (inst>>9) & 07;
			if (argn == 0)
				argn = 8;
		}
		else if ((inst & 0xFEFF) == ADDL)
		{
			argn = fetch(rtn + 2, ISP, inst & 0x100 ? 4 : 2);
		}
		else if (inst == LEA)
		{
			argn = fetch(rtn + 2, ISP, 2);
		}
		else
		{
			argn = 0;
		}

		if (argn && (argn % 4))
			argn = (argn/4) + 1;
		else
			argn /= 4;

		ADDPC(calladr);

		fprintf(outf, "\t");
		if (calladr != -1)
			psymoff(calladr, ISYM, ":  ");
		else
			fprintf(outf, "???:  ");

		if (entadr != -1)
			psymoff(entadr, ISYM, "");
		else
			fprintf(outf, "???");
		fprintf(outf, "(");

		/* Fetch arguments off stack */
		if (argn)
			fprintf(outf, "0x%x", fetch(stkptr += 4, DSP, 4));

		for (i = 1; i < argn; i++)
			fprintf(outf, ", 0x%x", fetch(stkptr += 4, DSP, 4));

		if (vflg)
		    fprintf(outf, ")  prev linkptr 0x%x\n", unlocalize(link));
		else
		    fprintf(outf, ")\n");

		/* Do not walk off either end of the stack */
		if (link < (u_int)&kstack[0][0] ||
		    link >= (u_int)&kstack[0][0] + ptob(stacksize))
			break;
	}

out:
	trc[1] = count - OVRHD; /* trace nesting depth */
	return 0;
}

/* Space is DSP for data or ISP for instr */
fetch(adr, space, size)
int * adr;
struct proc * space;
int size;
{
	long data;

	if (space == DSP) {
		/* Its in our local kstack */
		data = *(adr);
	} else {
		/* Its in the instruction stream, long way */
		getchunk(0, adr, &data, 4, "fetch");
	}

	if (size != 4) {
		data >>= 16;
		data &= 0xffff;
	}
	return data;
}

#define KERNEL_TUNE_DLB 1

#ifdef KERNEL_TUNE_DLB
/* pc, d2-d7, a2-a7, saved in u_rsave label_t, rset in pcb */
#define LABEL_PC 0
#define LABEL_D2 4
#define LABEL_A6 44
#define LABEL_A7 48
#else
/* pc, sfc, dfc, d2-d7, a2-a7 */
#define LABEL_PC 0
#define LABEL_D2 12
#define LABEL_A6 52
#define LABEL_A7 56
#endif

getsp(p)
struct proc *p;
{
	u_int ksp;
	u_int icsbase;
	char * uaddr;

	uaddr = (char *)KUAREABASE;
	stackaddr = f_stackaddr();
	stacksize = KSTACK_PAGES;

	/* otherwise look into the pcb */
	ksp = *(int *)((int)(&(u.u_rsave)) + LABEL_A7);

	/* Check for valid sp */
	icsbase = lookup("dmastk_start"); /* Base at dmastack */

	/*
	 * Change our stackbase to be on the ICS if it is out of range
	 * of the kernel stack.
	 */
	if (ksp >= icsbase && ksp < icsbase + DMASTACKSIZE) {
		stackaddr = icsbase;
		stacksize = DMASTACKSIZE;
		/* read in the ics stack instead untop of kstack[][] */
		getics();
	}

	return localize(ksp);
}

getpc(p)
struct proc *p;
{
	int pc;

	pc = *(int *)((int)(&(u.u_rsave)) + LABEL_PC);

	/* Check for valid pc */
	if ((unsigned)pc == 0) {
		fprintf(outf, " pc out of range 0x%x\n", pc);
		return 0;
	}

	return pc;
}

getlink(p)
struct proc *p;
{
	u_int link;

	link = *(int *)((int)(&(u.u_rsave)) + LABEL_A6);
	return localize(link);
}
#endif /* __hp9000s300 */
