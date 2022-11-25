/* @(#) $Revision: 70.1 $ */
/*
 * Instruction Scheduler (file 2)
 */


#include "sched.h"


extern int   flops;
extern int   floads;
extern int   fstores;
extern int   iops;
extern int   aloads;

extern int   cndflops;
extern int   cndfloads;
extern int   cndfstores;
extern int   cndiops;

extern o40pipe o40;



/***************************************************
 *
 * B E S T   I N S T   3 0
 *
 * Choose the best instruction to schedule for
 * the 68030 from the root list of the dag
 *
 */
dag *bestinst30(r,l,lf,ft,lck)
dag *r;   /* the root list 		*/
dag *l;   /* the last instrscheduled	*/
dag *lf;  /* the last flop scheduled	*/
int  ft;  /* floating point tail left	*/
int *lck; /* lock value between last 	*/
	  /* instr and best instr	*/
{
	register dag *b,	/* current "best" instruction	*/ 
		     *c, 	/* candidate to replace "best"	*/
		     *d;        /* used to find best fit with c */
	int           block, 	/* interlock between b and last	*/
		      clock;	/* interlock between c and last	*/
	int           bfp, 	/* path length from b to a flop */
		      cfp;	/* path length from c to a flop */
	int           bhead, 	/* head of instr b		*/
		      chead;	/* head of instr c		*/
	int           bfit, 	/* fit of b with the flop tail  */
		      cfit, 	/* fit of c with the flop tail  */
		      dfit;	/* fit of d with the flop tail  */
	int           binsttype,/* type of instr b		*/
		      cinsttype;/* type of instr c		*/

	/* first choice for "best" is the first root */
	b = r;

	/*
	 * if we pick "b" how many ticks 
	 * is it til we can schedule a flop? 
	 */
	bfp = floppath30(b,r);

	/* does "b" interlock with our last choice? */
	if (l) {
		block = lock(l,b);
	}
	else {
		block = lckNULL;
	}

	/* does "b" interlock with out last flop? */
	if (lf) {
		block = max(block,lock(lf,b));
	}

	/*
	 * how well does b fit into the remaining
	 * tail of the last scheduled flop?
	 */
	bfit = ft - b->time.t30.head;

	/*
	 * compare "b" to the rest of the roots
	 * if one of the roots is a better choice
	 * than "b", assign it to "b" and keep
	 * comparing
	 */

	for (c = r; c; c = c->rootn) {

		binsttype = b->insttype;
		cinsttype = c->insttype;
		
		/*
		 * calculate the interlock, min floating point
		 * path of the candidate "c"
		 */

		if (l) {
			clock = lock(l,c);
		}
		else {
			clock = lckNULL;
		}
		if (lf) {
			clock = max(clock,lock(lf,c));
		}

		cfp = floppath30(c,r);

		/*
		 * if one of "b" or "c" has a floating point
		 * interlock with the previous flop then 
		 * choose the one that doesn't
		 */
		if ((block != clock) &&
		    ((block == lckFPR) || (clock == lckFPR))) {
			if (block == lckFPR) {
			    copy:
				b = c;
				bfp = cfp;
				block = clock;
				bfit = ft - c->time.t30.head - 
					((cinsttype != iFLOP) 
						? c->time.t30.body
						: 0
					 );
			}
			continue;
		}

		/*
		 * if there are flops in the dag, but
		 * either there are no flop candidates or 
		 * the FPU is idle choose the instr with 
		 * the shorter path to a flop
		 */
		if (flops && !(cndflops && ft)) {
			if (bfp > cfp) {
				goto copy;
			}
			else if (bfp == cfp) {
				if ((binsttype != iFLOP) && 
				    ((binsttype < cinsttype) ||
				     (cinsttype == iFLOP))) {
					goto copy;
				}
			}
			continue;
		}

		/*
		 * if one of "b" or "c" has an A-register 
		 * interlock with the previous inst then 
		 * choose the one that doesn't
		 */
		if ((block != clock) && 
		    ((block == lckAR) || (clock == lckAR))) {
			if (block == lckAR) {
			    	goto copy;
			}
			continue;
		}

		/*
		 * now we are at of the "obvious" reasons to
		 * favor one instr over the other, so now it
		 * is time to choose the best fit of "b" and
		 * "c"
		 */

		bhead = b->time.t30.head + b->time.t30.body;
		chead = c->time.t30.head + c->time.t30.body;

		switch (cinsttype) {

		case iIOP:
			/*
			 * favor an A-register load over other
			 * IOP's as this will increase the 
			 * chance of avoiding an A-register
			 * interlock
			 */
			if (binsttype == iALOAD) {
				continue;
			}
			goto bestfit;

		case iALOAD:
			if (binsttype == iIOP) {
				b = c;
				bfp = cfp;
				block = clock;
				bfit += bhead - chead;
				continue;
			}

		case iFLOAD:
		case iFSTORE:
			/*
			 * "c" is not a flop, it is an ALOAD,
			 * IOP, IFLOAD or IFSTORE. So pair
			 * "c" with available flops, and find
			 * the best possible fit for "c"
			 */
	       bestfit: cfit = 9999;
			for (d = r; d; d = d->rootn) {
				if (d->insttype == iFLOP) {
					dfit = ft - 
					       (chead + d->time.t30.head + 
						d->time.t30.body);
					if (abs(dfit) < abs(cfit)) {
						cfit = dfit;
					}
				}
			}
			/*
			 * if "c" fits better than "b" than
			 * lets choose "c" over "b"
			 */
			if ((abs(cfit) < abs(bfit)) || (b == c)) {
				b = c;
				bfp = cfp;
				block = clock;
				bfit = cfit;
			}
			break;

		case iFLOP:
			/*
			 * "c" is a flop, see if "c" by itself
			 * is a better fit than "b"
			 */

			if (abs(ft - chead) < abs(bfit)) {
				b = c;
				bfp = cfp;
				block = clock;
				bfit = ft - chead;
			}
			break;

		}

	}
	/* what is in "b" now is our best choice */
	*lck = block;
	return(b);
}


/***************************************************
 *
 * F L O P   P A T H   3 0
 *
 * for the 68030, determine the
 * minimum path length to a flop
 * if we choose instruction "d"
 * This algorithm looks for a flop
 * at a max depth of 3 in the dag
 *
 */
floppath30(d,r)
dag *d;
dag *r;
{
	int   i,i2;
	int   k,k2;
	int   path = 9999;
	dag **p,**p2;
	dag  *d2,*d3;

	k = d->kids;
	p = d->kid;

	/*
	 * if "d" is a flop, then after the head time for
	 * "d" the fpu will start up
	 */
	if (d->insttype == iFLOP) {
		return(d->time.t30.head);
	}

	/* "d" is not a flop so lets check "d"s kids */
 	for (i = 0; i < k; i++,p++) {

		d2 = *p;

		/*
		 * we will only consider those children of "d"
		 * for which all the parents of that child are
		 * in the root list
		 */
		if (d2->parents == d2->rootparents) {
			/*
			 * if "d2" is a flop great, else we
			 * will check the children of "d2",
			 * but that is as far as we go
			 */
			if (d2->insttype == iFLOP) {
				path = min(path,pathlength30(d2,r));
			}
			else {

				k2 = d2->kids;
				p2 = d2->kid;

 				for (i2 = 0; i2 < k2; i2++,p2++) {

					d3 = *p2;

					if (d3->insttype == iFLOP) {
						path = min(
							path,
						        pathlength30(d3,r)
						       );
					}
				}
			}
		}
	}
#ifdef DEBUG
	if (debug_is) {
		printf("\n  path(%x) = %d",d,path);
	}
#endif
	return(path);
}


/***************************************************
 *
 * P A T H   L E N G T H   3 0
 *
 * calculate the path length to the flop "e" 
 *
 */
pathlength30(e,r)
dag *e;
dag *r;
{
	dag  *d,*d2;
	int   i,i2;
	int   k,k2;
	dag **p,**p2;
	int   path = e->time.t30.head;
	int   parents = 0;

	/*
	 * check all paths of length two or less from all
	 * the root nodes, summing up the lengths.  If all
	 * parents of "e" are accounted for than return this
	 * total path length, else return 9999
	 */
	for (d = r; d; d = d->rootn) {

		k = d->kids;
		p = d->kid;

 		for (i = 0; i < k; i++,p++) {
			if (*p == e) {
				parents++;
				path += d->time.t30.head + d->time.t30.body + 
					d->time.t30.tail;
				break;
			}
			else {
				d2 = *p;
				k2 = d2->kids;
				p2 = d2->kid;

 				for (i2 = 0; i2 < k2; i2++,p2++) {
					if (*p2 == e) {
						parents++;
						path += d->time.t30.head + 
							d->time.t30.body + 
							min(
							   d->time.t30.tail,
							   d2->time.t30.head
							) +
							d2->time.t30.body + 
							d2->time.t30.tail;
						break;
					}
				}
			}
		}
	}
	if (parents == e->parents) {
		return(path);
	}
	else {
		return(9999);
	}
}


/***************************************************
 *
 * N E W   T A I L
 *
 * calculate the new flop tail based on the chosen
 * instruction "d" and its interlock level with the
 * previous flop "lk"
 *
 */
newtail(d,lk,ft)
dag *d;
int  lk;
int *ft;
{
	switch (d->insttype) {

	case iFLOAD:
	case iFSTORE:
		if (lk == lckFPR) {
			*ft = 6;
		}
		else {
			*ft -= min(*ft, d->time.t30.head);
		}
		break;

	case iFLOP:
		*ft = d->time.t30.tail;
		break;

	case iALOAD:
	case iIOP:
		*ft -= min(*ft, d->time.t30.head + d->time.t30.body);
		break;

	}
}


/***************************************************
 *
 * B E S T   I N S T   4 0
 *
 * Choose the best instruction to schedule for
 * the 68040 from the root list of the dag
 *
 */
dag *bestinst40(r)
dag *r;   /* the root list */
{
	register dag *b,	/* current "best" instruction	*/ 
		     *c, 	/* candidate to replace "best"	*/
		     *d;        /* used to find best fit with c */
	int           blost, 	/* lost ticks due to interlock	*/
		      clost;	/* lost ticks due to interlock 	*/
	int           bfp, 	/* path length from b to a flop */
		      cfp;	/* path length from c to a flop */
	int           beac, 	/* eac of instr b		*/
		      ceac;	/* eac of instr c		*/
	int           bfit, 	/* fit of b with next flop 	*/
		      cfit, 	/* fit of c with next flop 	*/
		      dfit;	/* fit of d with next flop 	*/
	int           binsttype,/* type of instr b		*/
		      cinsttype;/* type of instr c		*/
	int	      xcuempty;

	xcuempty = flopwanted();

	/* first choice for "best" is the first root */
	b = r;

	/*
	 * if we pick "b" how many ticks 
	 * is it til we can schedule a flop? 
	 */
	bfp = floppath40(b,r);

	/* any interlocks in the pipe for "b"? */
	blost = lostticks(b,bfp);

	/*
	 * how well does b fit into the remaining
	 * tail of the last scheduled flop?
	 */
	bfit = 9999;

	/*
	 * compare "b" to the rest of the roots
	 * if one of the roots is a better choice
	 * than "b", assign it to "b" and keep
	 * comparing
	 */

	for (c = r; c; c = c->rootn) {

		binsttype = b->insttype;
		cinsttype = c->insttype;
		
		/*
		 * calculate the min floating point
		 * path and interlock situation of 
		 * the candidate "c"
		 */

		cfp = floppath40(c,r);

		clost = lostticks(c,cfp);

		/*
		 * if one of "b" or "c"  wastes more time
		 * than the other do to an interlock then
		 * choose the one that doesn't
		 */
		if (blost != clost) {
			if (blost > clost) {
			    copy:
				b = c;
				bfp = cfp;
				blost = clost;
				bfit = cfp - xcuempty;
			}
			continue;
		}

		/*
		 * if there are flops in the dag, but
		 * either there are no flop candidates or 
		 * the fcu will be idle choose the instr with 
		 * the shorter path to a flop
		 */
		if ((flops || floads || fstores) && 
		    (!(cndflops || cndfloads || cndfstores) ||
		     ((bfp > xcuempty) || (cfp > xcuempty)))) {
		     
			if (bfp > cfp) {
				goto copy;
			}
			else if (bfp == cfp) {
				if ((binsttype != iFLOP) && 
				    ((binsttype < cinsttype) ||
				     (cinsttype == iFLOP))) {
					goto copy;
				}
			}
			continue;
		}

		/*
		 * now we are at of the "obvious" reasons to
		 * favor one instr over the other, so now it
		 * is time to choose the best fit of "b" and
		 * "c"
		 */

		if (cndflops || cndfloads || cndfstores) {
			switch (cinsttype) {
	
			case iIOP:
			case iALOAD:
	 			cfit = 9999;
				for (d = r; d; d = d->rootn) {
					if (d->insttype == iFLOP) {
						dfit = intflop(c,d) - xcuempty;

						if (abs(dfit) < abs(cfit)) {
							cfit = dfit;
						}
					}
				}
				/*
				 * if "c" fits better than "b" than
				 * lets choose "c" over "b"
				 */
				if ((abs(cfit) < abs(bfit)) || (b == c)) {
					b = c;
					bfp = cfp;
					blost = clost;
					bfit = cfit;
				}
				break;
	
			case iFLOAD:
			case iFSTORE:
			case iFLOP:
				/*
				 * "c" is a flop, see if "c" by itself
				 * is a better fit than "b"
				 */
	
				cfit = intflop(NULL,c) - xcuempty;
				if (abs(cfit) < abs(bfit)) {
					b = c;
					bfp = cfp;
					blost = clost;
					bfit = cfit;
				}
				break;
	
			}
		}
		else {
			if (o40.eaf.inst != NULL &&
			    (abs(b->time.t40.eac - calceac(b)) > 
			     abs(c->time.t40.eac - calceac(c)))) {
				b = c;
				bfp = cfp;
				blost = clost;
				bfit = cfp;
			}
			else if ((c->insttype == iALOAD) &&
				 (b->insttype != iALOAD)) {
				b = c;
				bfp = cfp;
				blost = clost;
				bfit = cfp;
				
			}
		}

	}
	/* what is in "b" now is our best choice */
	return(b);
}


/***************************************************
 *
 * F L O P  W A N T E D
 *
 * In how many ticks to do we want the next flop
 * to arrive at the fcu
 *
 */
flopwanted()
{
	if (o40.fxu.cu_inst) {
		if (o40.eaf.inst->insttype <= iFSTORE) {
			return(o40.eaf.clks + 
			       max(
				  o40.eaf.inst->time.t40.fcu - 1,
				  o40.fxu.cu_inst->time.t40.fxu
			       ) +
			       max(o40.eaf.inst->time.t40.fxu-2,0));
		}
		else {
			return(
			    max(o40.fxu.cu_clks - 1, o40.fxu.xu_clks) + 
			    max(o40.fxu.cu_inst->time.t40.fxu-2,0)
			);
		}
	}
	else {
		if (o40.eaf.inst->insttype <= iFSTORE) {
			return(o40.eaf.clks + 
			       max(
				  o40.eaf.inst->time.t40.fcu - 1,
				  o40.fxu.xu_clks - o40.eaf.clks
			       ) + max(o40.eaf.inst->time.t40.fxu-2,0)
			);
		}
		else {
			return(0);
		}
	}
}


/***************************************************
 *
 * I N T  F L O P
 *
 * determine the time flop "f" gets to 
 * the fxu if iop "i" is in front of it
 *
 */
intflop(i,f)
dag *i;
dag *f;
{
	int ieac;

	if (!i) {
		return(calceac(f) + f->time.t40.eaf);
	}
	else {
		ieac = calceac(i);

		if (f->time.t40.eac_gets_xu) {
			return(ieac + max(
					max(f->time.t40.eac_gets_xu, i->time.t40.eaf),
					o40.eaf.inst->time.t40.ixu - ieac +
					o40.eaf.clks
				      ) + 
				      i->time.t40.ixu + f->time.t40.eac -
				      f->time.t40.eac_gets_xu + f->time.t40.eaf);
		}
		else {
			return(ieac + max(
					max(f->time.t40.eac,i->time.t40.eaf),
					o40.eaf.inst->time.t40.ixu - ieac +
					o40.eaf.clks
				      ) + f->time.t40.eaf);
		}
	}
}


/***************************************************
 *
 * F L O P   P A T H   4 0
 *
 * determine the minimum path length to
 * a flop if we choose instruction "d"
 * This algorithm looks for a flop at a
 * max depth of 3 in the dag
 *
 */
floppath40(d,r)
dag *d;
dag *r;
{
	int   i,i2;
	int   k,k2;
	int   path = 9999;
	dag **p,**p2;
	dag  *d2,*d3;

	k = d->kids;
	p = d->kid;

	/*
	 * if "d" is a flop, then how many
	 * ticks until "d" gets to the fpu?
	 */
	if (d->insttype <= iFSTORE) {
		return(calceac(d) + d->time.t40.eaf);
	}

	/* 
	 * "d" is not a flop so lets check 
	 * the other root nodes
	 */
	i = calceac(d);

	for (d2 = r; d2; d2 = d2->rootn) {
		if (d2->insttype <= iFSTORE) {
			if (d2->time.t40.eac_gets_xu) {
				path = min(
					path,
					i + max(
						max(d2->time.t40.eac_gets_xu, d->time.t40.eaf),
						o40.eaf.inst->time.t40.ixu - i +
						o40.eaf.clks
					    ) + d->time.t40.ixu + d2->time.t40.eac 
					    - d2->time.t40.eac_gets_xu + d2->time.t40.eaf);
			}
			else {
				path = min(
					path,
					i + max(
						max(d2->time.t40.eac,d->time.t40.eaf),
						o40.eaf.inst->time.t40.ixu - i +
						o40.eaf.clks
					    ) + d2->time.t40.eaf);
			}
		}
	}
	/*
	 * "d" is not a flop an neither are
	 * any of the other root nodes so 
	 * lets check "d"s kids 
	 */
 	for (i = 0; i < k; i++,p++) {

		d2 = *p;

		/*
		 * we will only consider those children of "d"
		 * for which all the parents of that child are
		 * in the root list
		 */
		if (d2->parents == d2->rootparents) {
			/*
			 * if "d2" is a flop great, else we
			 * will check the children of "d2",
			 * but that is as far as we go
			 */
			if (d2->insttype == iFLOP) {
				path = min(path,pathlength40(d2,r));
			}
			else {

				k2 = d2->kids;
				p2 = d2->kid;

 				for (i2 = 0; i2 < k2; i2++,p2++) {

					d3 = *p2;

					if (d3->insttype == iFLOP) {
						path = min(
							path,
						        pathlength40(d3,r)
						       );
					}
				}
			}
		}
	}
#ifdef DEBUG
	if (debug_is) {
		printf("\n  path(%x) = %d",d,path);
		fflush(0);
	}
#endif
	return(path);
}


/***************************************************
 *
 * P A T H   L E N G T H   4 0
 *
 * calculate the path length to the flop "e" 
 *
 */
pathlength40(e,r)
dag *e;
dag *r;
{
	dag  *d,*d2;
	int   i,i2;
	int   k,k2;
	dag **p,**p2;
	int   path = e->time.t40.eac + e->time.t40.eaf;
	int   parents = 0;

	/*
	 * check all paths of length two or less from all
	 * the root nodes, summing up the lengths.  If all
	 * parents of "e" are accounted for than return this
	 * total path length, else return 9999
	 */
	for (d = r; d; d = d->rootn) {

		k = d->kids;
		p = d->kid;

 		for (i = 0; i < k; i++,p++) {
			if (*p == e) {
				parents++;
				path += max(d->time.t40.eac,max(d->time.t40.eaf,d->time.t40.ixu));
				break;
			}
			else {
				d2 = *p;
				k2 = d2->kids;
				p2 = d2->kid;

 				for (i2 = 0; i2 < k2; i2++,p2++) {
					if (*p2 == e) {
						parents++;
						path += max(
							   d->time.t40.eac,
							   max(
							      d->time.t40.eaf,
							      d->time.t40.ixu
							   )
							) +
							max(
							   d2->time.t40.eac,
							   max(
							      d2->time.t40.eaf,
							      d2->time.t40.ixu
							   )
							);
						break;
					}
				}
			}
		}
	}
	if (parents == e->parents) {
		return(path);
	}
	else {
		return(9999);
	}
}


/***************************************************
 *
 *  I N I T   F O R T Y
 *
 * initialize the 68040 pipeline
 *
 */
initforty()
{
	o40.clock = 0;

	o40.eac.inst = NULL;
	o40.eac.clks = 0;

	o40.eaf.inst = NULL;
	o40.eaf.clks = 0;
	
	o40.ixu.inst = NULL;
	o40.ixu.clks = 0;
	
	o40.fxu.cu_inst = NULL;
	o40.fxu.cu_clks = 0;
	o40.fxu.xu_inst = NULL;
	o40.fxu.xu_clks = 0;
	o40.fxu.nu_inst = NULL;
	o40.fxu.nu_clks = 0;
}


/***************************************************
 *
 *  C L E A N   F O R T Y
 *
 * clean up the 68040 pipeline
 *
 */
cleanforty()
{
	freedagnode(o40.eac.inst);
	freedagnode(o40.eaf.inst);
	freedagnode(o40.ixu.inst);
	if (o40.ixu.inst != o40.fxu.cu_inst) {
		freedagnode(o40.fxu.cu_inst);
	}
	if ((o40.ixu.inst != o40.fxu.xu_inst) &&
	    (o40.fxu.cu_inst != o40.fxu.xu_inst)) {
		freedagnode(o40.fxu.xu_inst);
	}
	if (o40.ixu.inst != o40.fxu.nu_inst) {
		freedagnode(o40.fxu.nu_inst);
	}
}


/***************************************************
 *
 * E A C   F R E E
 *
 * time for a new instruction?
 */
eacfree()
{
	return(!o40.eac.inst);
}


/***************************************************
 *
 * T I C K - T O C K
 *
 * calculate the new state after a clock tick
 *
 */
ticktock()
{
	int ticks;
	static int cu_to_xu_early = 0;

#ifdef DEBUG
	if (debug_is) {
		printf("\n\nclock = %d\n",o40.clock);
		printf("                               ___ixu(%d,%5.5x)\n",
		       o40.ixu.clks,o40.ixu.inst
		      );
		printf("eac(%d,%5.5x)___eaf(%d,%5.5x)___/\n",
		       o40.eac.clks,o40.eac.inst,
		       o40.eaf.clks,o40.eaf.inst
		      );
		printf("                              \\___fcu(%d,%5.5x)___fxu(%d,%5.5x)___fnu(%d,%5.5x)\n",
		       o40.fxu.cu_clks,o40.fxu.cu_inst,
		       o40.fxu.xu_clks,o40.fxu.xu_inst,
		       o40.fxu.nu_clks,o40.fxu.nu_inst
		      );
		fflush(0);
	}
#endif

	ticks = 9999;

	if (o40.eac.clks) {
		ticks = min(ticks,o40.eac.clks);
	}

	if (o40.eaf.clks) {
		ticks = min(ticks,o40.eaf.clks);
	}

	if (o40.ixu.clks) {
		ticks = min(ticks,o40.ixu.clks);
	}

	if (o40.fxu.nu_clks) {
		ticks = min(ticks,o40.fxu.nu_clks);
	}

	if (o40.fxu.xu_clks) {
		ticks = min(ticks,o40.fxu.xu_clks);
	}

	if (o40.fxu.cu_clks) {
		if (!o40.fxu.xu_inst || (o40.fxu.cu_clks > o40.fxu.xu_clks)) {
			ticks = min(ticks,max(1,o40.fxu.cu_clks - 1));
		}
		else {
			ticks = min(ticks,o40.fxu.cu_clks);
		}
	}

	if (ticks == 9999) {
		return;
	}

	o40.clock += ticks;

	if (o40.ixu.inst) {
		if (o40.ixu.clks) {
			o40.ixu.clks -= ticks;
		}
		if (!o40.ixu.clks) {
			if (o40.ixu.inst->insttype >= iIOP) {
				/* 
				 * if the inst is a flop, it must
				 * be a Flt->Int conversion. Let
				 * the fxu free up the dag node
				 */
				freedagnode(o40.ixu.inst);
			}
			o40.ixu.inst = NULL;
		}
	}

	/* advance the fnu */
	if (o40.fxu.nu_inst) {
		if (o40.fxu.nu_clks) {
			o40.fxu.nu_clks -= ticks;
		}
		if (!o40.fxu.nu_clks) {
			freedagnode(o40.fxu.nu_inst);
			o40.fxu.nu_inst = NULL;
		}
	}

	/* advance the fxu */
	if (o40.fxu.xu_inst) {
		if (o40.fxu.xu_clks) {
			o40.fxu.xu_clks -= ticks;
		}
	
		if (!o40.fxu.xu_clks) {
			if (!o40.fxu.nu_inst) {
				o40.fxu.nu_inst = o40.fxu.xu_inst;
				o40.fxu.nu_clks = o40.fxu.nu_inst->time.t40.fnu;

				o40.fxu.xu_inst = NULL;
			}
			else {
				internal_error("fxu done, fnu not free");
			}
		}
	}

	/* advance the fcu */
	if (o40.fxu.cu_inst) {
		if (o40.fxu.cu_clks) {
			o40.fxu.cu_clks -= ticks;
		}
	
		if (o40.fxu.cu_clks <= 1) {
			if (cu_to_xu_early) {
				o40.fxu.cu_inst = NULL;
				cu_to_xu_early = 0;
			}
			else {
				if (!o40.fxu.xu_inst) {
					o40.fxu.xu_inst = o40.fxu.cu_inst;
					o40.fxu.xu_clks = max(
							   o40.fxu.cu_inst->time.t40.fxu,
							   o40.fxu.nu_clks
							  );
	
					if (!o40.fxu.cu_clks) {
						o40.fxu.cu_inst = NULL;
					}
					else {
						cu_to_xu_early = 1;
					}
				}
				else if (!o40.fxu.cu_clks) {
					if (o40.fxu.cu_inst == o40.fxu.xu_inst) {
						o40.fxu.cu_inst = NULL;
					}
					else {
						internal_error(
						    "fcu done, fxu not free"
						);
					}
				}
			}
		}
	}

	/* advance the eaf */
	if (o40.eaf.inst) {
		if (o40.eaf.clks) {
			o40.eaf.clks -= ticks;
		}
	
		if (!o40.eaf.clks) {
			if (o40.eaf.inst->insttype <= iFSTORE) {
				if (!o40.fxu.cu_inst) {
					setfcu(o40.eaf.inst);

					if (o40.eaf.inst->time.t40.cvrt == FltInt) {
						if (o40.ixu.inst) {
							internal_error(
								"eaf done, fcu free, ixu not ready for Flt->Int"
							);
						}
						else {
							o40.ixu.inst = 
							    o40.fxu.cu_inst;
							o40.ixu.clks = 
							    o40.fxu.cu_clks;
						}
						
					}

					o40.eaf.inst = NULL;
				}
				else {
					internal_error(
						"eaf done, fcu not free"
					);
				}
			}
			else {
				if (!o40.ixu.inst) {
					o40.ixu.inst = o40.eaf.inst;
					o40.ixu.clks = o40.eaf.inst->time.t40.ixu;

					o40.eaf.inst = NULL;
				}
				else {
					internal_error(
						"eaf done, ixu not free"
					);
				}
			}
		}
	}

	/* advance the eac */
	if (o40.eac.inst) {
		if (o40.eac.clks) {
			o40.eac.clks -= ticks;
		}
	
		if (!o40.eac.clks) {
			if (!o40.eaf.inst) {
				seteaf(o40.eac.inst);

				o40.eac.inst = NULL;
			}
			else {
				internal_error("eac done, eaf not free");
			}
		}
	}
}


/***************************************************
 *
 * S E T   F C U
 *
 * Determine the number of clocks an instruction
 * will be keeping the cu busy. Includes handling
 * type conversions, interlocks and how long 
 * instuctions ahead will block the xu and nu.
 *
 */
setfcu(d)
dag *d;
{
	o40.fxu.cu_inst = d;

	if (d->time.t40.cvrt == IntFlt) {
		o40.fxu.cu_clks = max(o40.fxu.xu_clks,2) + 10;
	}
	else {
		if (lock(o40.fxu.xu_inst,d) == lckFPR) {
			if (d->time.t40.cvrt == FltInt) {
				o40.fxu.cu_clks = o40.fxu.xu_clks + 12;
			}
			else {
				o40.fxu.cu_clks = o40.fxu.xu_clks + d->time.t40.fcu + 2;
			}
		}
		else if (lock(o40.fxu.nu_inst,d) == lckFPR) {
			if (d->time.t40.cvrt == FltInt) {
				o40.fxu.cu_clks = max(
						      o40.fxu.xu_clks,
						      o40.fxu.nu_clks + 2
						  ) + 7;
			}
			else {
				o40.fxu.cu_clks = 
				   max(
				       d->time.t40.fcu + o40.fxu.nu_clks - 1,
				       o40.fxu.xu_clks
				   );
			}
		}
		else {
			if (d->time.t40.cvrt == FltInt) {
				o40.fxu.cu_clks = max( o40.fxu.xu_clks, 3) + 7;
			}
			else {
				o40.fxu.cu_clks = max( d->time.t40.fcu, o40.fxu.xu_clks);
			}
		}
	}
}


/***************************************************
 *
 * S E T   E A F
 *
 * Determine the number of clocks an instruction
 * will be keeping the eaf busy. This is not
 * how many clocks the instruction needs the eaf,
 * but how long it will occupy the eaf.
 *
 */
seteaf(d)
dag *d;
{
	o40.eaf.inst = d;
	if (d->insttype >= iIOP) {
		o40.eaf.clks = max(d->time.t40.eaf,o40.ixu.clks);
	}
	else {
		if (d->time.t40.cvrt == FltInt) {
			o40.eaf.clks = max(d->time.t40.eaf,
					   max(o40.fxu.cu_clks,o40.ixu.clks));
		}
		else {
			o40.eaf.clks = max(d->time.t40.eaf,o40.fxu.cu_clks);
		}
	}
}


/***************************************************
 *
 * S E T   E A C
 *
 * Initialize the eac with instruction 'd'
 *
 */
seteac(d)
dag *d;
{
	int i;

	o40.eac.inst = d;
	o40.eac.clks = calceac(d);
}


/***************************************************
 *
 * C A L C   E A C
 *
 * Determine the number of clocks an instruction
 * will be keeping the eac busy. This is not
 * how many clocks the instruction needs the eac,
 * but how long it will occupy the eac.
 *
 */
int calceac(d)
dag *d;
{
	if (d->time.t40.eac_gets_xu) {
		if (o40.eaf.inst->insttype >= iIOP) {
			return(
			    max(
				d->time.t40.eac_gets_xu,
				o40.eaf.clks + o40.eaf.inst->time.t40.ixu
			     ) + 
			     d->time.t40.eac - d->time.t40.eac_gets_xu
			);
		}
		else {
			return(
			    max(
				d->time.t40.eac_gets_xu,
				max( o40.eaf.clks, o40.ixu.clks)
		     	     ) +
			     d->time.t40.eac - d->time.t40.eac_gets_xu
			);
		}
	}
	else {
		if (lock(o40.eaf.inst,d) == lckAR) {
			return(d->time.t40.eac + o40.eaf.clks + o40.eaf.inst->time.t40.ixu);
		}
		else if (lock(o40.ixu.inst,d) == lckAR) {
			return(max(d->time.t40.eac + o40.ixu.clks, o40.eaf.clks));
		}
		else {
			return(max(d->time.t40.eac,o40.eaf.clks));
		}
	}
}


/***************************************************
 *
 * L O S T   T I C K S
 *
 * Does "d" interlock with another instruction
 * in the pipe such that a stall will result.
 * if so, how many clocks are we wasting?
 *
 */
int lostticks(d,dfp)
dag *d;
int  dfp;
{
	int  l = 0;

	/*
	 * does "d" A-reg interlock with
	 * the instruction in the eaf or ixu?
	 */
	 if (lock(o40.eaf.inst,d) == lckAR) {
		l = o40.eaf.clks + o40.eaf.inst->time.t40.ixu;
	 }

	 if (lock(o40.ixu.inst,d) == lckAR) {
		l = max(l,o40.ixu.clks);
	 }

	/* 
	 * does "d" interlock with a flop that
	 * will not clear the fnu in time?
	 */
	if (d->insttype < iIOP) {
		if (lock(o40.eaf.inst,d) == lckFPR) {
			l = max(
				l,
				(o40.fxu.cu_inst 
				  ? o40.fxu.cu_inst->time.t40.fxu
				  : o40.eaf.inst->time.t40.fcu - 1
				) +
				o40.eaf.inst->time.t40.fxu + 2 - o40.eaf.clks - dfp
			    );
		}
		if (lock(o40.fxu.cu_inst,d) == lckFPR) {
			l = max(
				l,
				o40.fxu.cu_clks + o40.fxu.cu_inst->time.t40.fxu + 
				2 - dfp
			    );
		}
		if (lock(o40.fxu.xu_inst,d) == lckFPR) {
			l = max(
				l,
				o40.fxu.xu_clks + 2 - dfp
			    );
		}
	}
	return(l);
}


/***************************************************
 *
 * L O C K
 *
 * if "d2" is a child of "d1", return the interlock
 * level between them, else return lckNULL
 *
 */
lock(d1,d2)
dag *d1;
dag *d2;
{
	dag **p;
	int   i, k;

	if (!d1) {
		return(lckNULL);
	}

	k = d1->kids;
	p = d1->kid;

 	for (i = 0; i < k; i++,p++) {
		if (*p == d2) {
			return(d1->kidintrlck[i]);
		}
	}
	return(lckNULL);
}
