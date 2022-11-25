/* @(#) $Revision: 70.1 $ */      

# ifdef SDOPT

# include "symbols.h"
# include "adrmode.h"
# include "icode.h"
# include "sdopt.h"
# include "ivalues.h"

extern char *	calloc();
extern long	newdot;
extern long	dottxt;
extern short	nerrors;
extern symbol *	dot;
extern long	line;
extern short	pic_flag;

/* Data definitions and algorithms to support span-dependent optimization
 * of branch instructions.  The instructions handled are bCC, bsr, bra.
 * We work down from the top: assume a worst case (largest size), and
 * then shorten where possible.
 */

/* Maximum times to iterate through shortening loop. */
# define SDMAXLOOP	6

/* array that gives the number of bytes for a span-dependent instruction
 * indexed by opcode type and instruction size.
 * ?? if want to optimize PC-relative's too, how does that fit in here.
 * ?? would it be better to keep track of the extra displacement space 
 * ?? required instead of the total instruction size.
 * NOTE: SDEXTRN is shown for completeness, but is not implemented
 *       at this time.
 *	 If SDEXTRN is implemented, then should have SDUNKN==SDEXTRN,
 *	 otherwise, for now, can have SDUNKN==SDLONG.
 */
# ifdef M68010
short sdi_length[3][6] = {
	/*	   SDZERO  SDBYTE  SDWORD  SDLONG  SDUNKN  SDEXTRN
	/* SDBCC */   {	2,	2,	4,	8,	8,	8 },
	/* SDBRA */   {	2,	2,	4,	6,	6,	6 },
	/* SDBSR */   {	4,	2,	4,	6,	6,	6 },
	};
# endif

# ifdef M68020
short sdi_length[4][6] = {
	/*	   SDZERO  SDBYTE  SDWORD  SDLONG  SDUNKN  SDEXTRN
	/* SDBCC */   {	2,	2,	4,	6,	6,	8 },
	/* SDBRA */   {	2,	2,	4,	6,	6,	6 },
	/* SDBSR */   {	4,	2,	4,	6,	6,	6 },
	/* SDCPBCC */ {	4,	4,	4,	6,	6,	6 },
	};
# endif

/* How far from location should the base of the span-size calculation
 * be.
 * Currently, always .+2 so maybe this isn't worth the bother.
 * Would only change if wanted to optimize other instructions
 * that potentially had different base.
 */
short sdi_loc_to_base[] =  { 2, 2, 2,		/* SDBCC, SDBRA, SDBSR */
# ifdef M68020
			 2,			/* SDCPBCC */
# endif
			 };

struct sdi * sdi_listhead = (struct sdi *)&sdi_listhead,
	   * sdi_listtail = (struct sdi *)&sdi_listhead;

long text_idelta = 0;	/* Incremental Delta -- number of bytes change 
			 * each time through loop.
			 */
long text_tdelta = 0;	/* Total size change to the text segment. */


/* Macros to update the current value of the "dot" symbol and of the
 * target node for "dot".  We need to update these as we go along
 * in case the target expression involves "."
 * ?? SET_DOT_TARGET is not currently used ??  Because we don't let
 * spans with targets that use "dot" go into the iteration step.
 */
# define SET_DOT_VALUE(x) dot->svalue=x
# define SET_DOT_TARGET(x) dot->ssdiinfo->sdi_location=x

/* macro to reset the global "line" before an error call.  Needed after
 * pass1 if we want the lineno reported to be valid.
 * Probably really only need to do this for uerrors.
 */
# define SET_LINE(p) line = (p)->sdi_line

/* Create an sdi "span" type node and add it to the end of the linked list of
 * span dependent info.
 * Return a pointer to the node created.
 * This routine will be called during pass1 whenever we see an instruction that
 * is a candidate for span-dependent optimization.
 */
struct sdi * 
makesdi_spannode(sditype, instrp)
   int sditype;
   struct instruction * instrp;
{
   register struct sdi * p;
   register rexpr * target;

   /* An optimization to this algorithm is to see if we can determine
    * the size necessary already at pass1 (it's a SZBYTE or the
    * expression is not simple (ie, it's a TDIFF) ), and not make a
    * node at all.
    */
   
   /* Only create a span node for expressions of the form <sym>+<offset>
    * If the expression is TYPL, then could determine the size right now.
    * ?? In old assembler,iIf the expression is a O_COMPLEX, must use long form
    * to avoid looping. ??But that is not a problem with max->shorter iteration
    * and simple TDIFF's which are either an absolute that could only get
    * smaller, or really <sym>+<offset>.
    * *** 
    * 870529 Have a bug with letting <sym>+<offset> go through.  These could
    * actually become bigger do to sdo, and so worst case should be
    * assumed and no optimization done.
    * 880325 TDIFF's are o.k. because they are either an absolute that could
    * only get smaller, or really <sym>+<offset>.
    * The <sym>+<offset> must be marked as the worst case in setup_sdi_target.
    *
    * THIS ASSUMES WE ONLY OPTIMIZE IN THE TEXT SEGMENT!!!  (NOTE: if do
    * optimization in the DATA segment too (argh!), then must be more 
    * restrictive and must (?should) do optimization of the 2 segments
    * together.)
    * ?? For now, just eval a TDIFF once at sdi_setup time and use that size.
    *    Some optimizations will be lost, but saves the time of reevaluating
    *    the TDIFF's each time through the loop, and compiler doesn't 
    *    generate these anyway  (NOTE: if add optimization of %pc-relative
    *    compiler does do TDIFF's there because of switch table calculations.
    */

   /* The target of the branch is operand[0], known to be A_EXP */

   /* debug print */
# ifdef SDDEBUG
   printf("sdi spannode being created at location %d\n",newdot);
# endif
   p = (struct sdi *) calloc(1,sizeof(struct sdi));
   if (p==0)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("unable to calloc space for span dependent info");

   /* fill in the information (as much as possible) in the structure */
   target = &instrp->opaddr[0].unoperand.soperand.adexpr1;
   p->sdi_location0 = newdot;
   p->sdi_location = newdot;
   p->sdi_exp.symptr = target->symptr;
   p->sdi_exp.expval.lngval = target->expval.lngval;
   p->sdi_exp.exptype = target->exptype;
   p->sdi_target = 0;	/* Fill in later */
   p->sdi_nodetype = SDSPAN;
   p->sdi_optype = sditype;
   p->sdi_size = SDUNKN;
   p->sdi_length = sdi_length[sditype][SDUNKN];
   p->sdi_span = 0;	/* Fill in later */
   p->sdi_tdelta = 0;
   p->sdi_idelta = 0;
   p->sdi_base = newdot + sdi_loc_to_base[sditype];
   p->sdi_line = line;

   /* link it into the list */
   sdi_listtail->sdi_next = p;	/* first time, this is really setting
				 * sdi_listhead */
   p->sdi_next = 0;
   sdi_listtail = p;
   return(p);
   
}


/* Create an sdi "label" type node and add it to the end of the linked list of
 * span dependent info.
 * Return a pointer to the node created.
 * This routine will be called during pass1 whenever we define a label.
 */
struct sdi * 
makesdi_labelnode(labsym)
   symbol * labsym;
{
   register struct sdi * p;

   /* debug print */
# ifdef SDDEBUG
   printf("sdi labelnode being created for <%s>\n", labsym->snamep);
# endif
   p = (struct sdi *) calloc(1,sizeof(struct sdi));
   if (p==0)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("unable to calloc space for span dependent info");

   /* fill in the information (as much as possible) in the structure */
   p->sdi_location0 = 0;	/* will be set later. Right now, we
				 * don't know about the effects of
				 * implicit alignment.
				 */
   p->sdi_location = 0;
   p->sdi_exp.symptr = labsym;
   p->sdi_target = 0;
   p->sdi_nodetype = SDLABEL;
   p->sdi_optype = 0;
   p->sdi_size = 0;
   p->sdi_length = 0;
   p->sdi_span = 0;
   p->sdi_tdelta = 0;
   p->sdi_idelta = 0;
   p->sdi_base = 0;
   p->sdi_line = line;

   /* link it into the list */
   sdi_listtail->sdi_next = p;	/* first time, this is really setting
				 * sdi_listhead */
   p->sdi_next = 0;
   sdi_listtail = p;
   return(p);
}


/* Driver for the mid-pass to do the span dependent optimization. */
sdopass() {
  if (sdi_listhead==(struct sdi *) &sdi_listhead) {
	/* list is null -- nothing to do */
# ifdef SDDEBUG
	printf("sdi list was empty\n");
# endif
	return;
	}
   if (nerrors==0) sdo_setup();
   if (nerrors==0) sdo_iterate();
   if (nerrors==0) sdo_cleanup();
}

/* Get ready to do the span-dependent optimization iterations.
 * During pass1, we set up preliminary nodes for each span-dependent opcode,
 * and for each label.  Before the iteration starts we need to:
 *	- finish filling in fields in the SPAN and LABEL nodes
 */
sdo_setup() {
  register struct sdi * p;
  register symbol * sym;
  register struct sdi * target;

# ifdef SDDEBUG
  printf("starting sdo_setup\n");
# endif
  dot->stype = STEXT;
  dot->svalue = 0;
  for(p = sdi_listhead; p != 0; p=p->sdi_next) {
	sym = p->sdi_exp.symptr;
	switch(p->sdi_nodetype) {
	   default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		SET_LINE(p);
		aerror("unexpected sdi-nodetype in sdo_setup");
		break;
	   case SDSPAN:
# ifdef SDDEBUG
		printf("span node at location %d\n",p->sdi_location);
# endif
		/* Must fill in values for fields left 0 in pass1 */
		setup_sdi_target(p);
		break;

	   case SDLABEL:
		/* The location field was not set in pass1 because
		 * imlicit aligned would not have been done yet.
		 */
		p->sdi_location0 = sym->svalue;
		p->sdi_location = sym->svalue;
# ifdef SDDEBUG
		printf("label node for <%s> at %d\n",sym->snamep,sym->svalue);
# endif
		break;

	   case SDLALIGN:
		break;
	   }
	}

  update_spaninfo();
# ifdef SDDEBUG
  printf("leaving sdo_setup\n");
# endif
}

sdo_iterate() {

  int loopctr = 0;
  register struct sdi * p;

# ifdef SDDEBUG
  printf("starting sdo_iterate\n");
# endif
  while (nerrors==0 && loopctr++ < SDMAXLOOP && text_idelta!=0 ) {
# ifdef SDDEBUG
	printf("starting loop iteration %d\n", loopctr);
# endif
	text_idelta = 0;
  	for(p = sdi_listhead; p != 0; p=p->sdi_next) {
	   /*SET_DOT_TARGET(p->sdi_location);*/
	   if (p->sdi_nodetype == SDSPAN) {
		/* integrity check -- can remove later */
		if (p->sdi_target==dot->ssdiinfo) {
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			SET_LINE(p);
			aerror("dot target referenced in iterate loop");
			}
		/* ?? in calculation below, p->sdi_offset really always zero
		 * since we don't iterate on nonsimple targets.
		 */
		p->sdi_span = (p->sdi_target->sdi_location + p->sdi_offset) - p->sdi_base;
		update_node(p);
		}
	   }
# ifdef SDDEBUG
	printf("ending loop iteration %d, idelta=%d\n", loopctr, text_idelta);
# endif
	update_spaninfo();

	}
# ifdef SDDEBUG
   printf("ending sdo_iterate\n");
# endif
}

/* Final cleanup for the span-dependent middle pass.
 * The main job here is to fix up the lalign's and get them right, and
 * do other adjustments they imply.
 * Make a final pass through the sdi-list and calculate the real size
 * for each lalign now.  Also update the location for each lalign node
 * to reflect the lalignment changes. (Note that SDLALIGN node locations
 * are updated as part of the sdo-iteration process.
 * Also fixup the sdi_location for each label node, and use this to
 * fix the value of the corresponding STEXT symbol table entry.
 *
 * Note that this really changes the sdi_span and sdi_location values of
 * all the span nodes too, but at this point we don't really care
 * anymore.  ?? RIGHT ????  The codegen routines will use the updated
 * symbol values, not the span information.
 *
 * Otherwise there's not alot that can be done right now (except maybe some more
 * integrity info.  If we had the instruction nodes in memory we
 * could go fixup their size info right now, and greatly simplify
 * things in pass2.
 * We could fix all the STEXT symbol table values now by scanning for
 * SDLABEL nodes, but it's more efficient to do this in the fixsyms
 * pass through the symbol table.
 */
sdo_cleanup() {
   register struct sdi * p;
   register long ngap;
   register long ladelta = 0;
   register symbol * sym;

# ifdef SDDEBUG
   printf("starting sdo_cleanup\n");
   printf("total size change to text segment is: %d\n", text_tdelta);
# endif

   for (p = sdi_listhead; p != 0; p=p->sdi_next) {
	p->sdi_location += ladelta;
	switch (p->sdi_nodetype & SDNDTYPE) {
	  default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		line = p->sdi_line;
		aerror("unknown node type <%d> in sdo_cleanup",
			p->sdi_nodetype&SDNDTYPE);
		break;
	  case SDLALIGN:
		{ /* Need to adjust the lalignment.  The alignment value
		   * is in p->sdi_span. In pass1, (lalignval - 2) bytes
		   * of filler were generated as a worst case.
		   */
		  long lalmod = p->sdi_span;
		  long nskip, ngap;

		  /* calculate the size filler needed now to generate
		   * the correct alignment.
		   */
		  ngap = p->sdi_location % lalmod;
		  if (ngap != 0)
			ngap = lalmod - ngap;

		  p->sdi_offset = ngap;
		  p->sdi_tdelta = ngap - (lalmod - 2);
		  ladelta += p->sdi_tdelta;
# ifdef SDDEBUG
		  printf("lalgign at %d for %d, sized from %d to %d\n",
			p->sdi_location, lalmod, lalmod-2, ngap);
# endif
		  break;
		}
	  
	  case SDLABEL:
		{ sym = p->sdi_exp.symptr;
# ifdef SDDEBUG
		  printf("updating symbol <%s> from %d to %d\n",
			sym->snamep, sym->svalue, p->sdi_location);
# endif
		  sym->svalue = p->sdi_location;
		  break;
		
		}
	  case SDSPAN:
		{
# ifdef SDDEBUG
		  printf("span node location updated from <%d> to <%d>\n",
			p->sdi_location-ladelta, p->sdi_location );
# endif
		  break;
		}
	  }
	}

   text_tdelta += ladelta;
   dottxt += text_tdelta;
# ifdef SDDEBUG
   printf("size change in laligns is: %d\n", ladelta);
   printf("total size change to text segment is: %d\n", text_tdelta);
   printf("ending sdo_cleanup\n");
# endif

}

/* Check the target expression for the span node, and turn it into
 * the simplest form now possible:  TYPL, TSYM, TDIFF.
 * ??Don't reduce a TDIFF to absolute unless both symbols are absolute, (or
 * ??are not in the text segment??).
 * ??For now, we just evaluate the TDIFF once, and use this absolute value
 * to determine a size for the branch displacement.
 * Then setup the target/offset info in the spannode.
 *
 * 870529 Bug here with targets like L+<offset>.  The sdo could actually
 * cause the span of such a node to increase, so our initial size pick
 * here could be too small by pass2.  The situtation looks something
 * like:
 *	 L:  xxxxxx
	     <code that could shrink with sdo>
	     bcc L+d

	(L+d):	
 * As code shrinks during sdo, L+d becomes farther away from the bcc
 * instruction.
 * Should just assign the worst case size now. ( is that SZWORD or pseudo
 * SZLONG for the 68010). 
 * For now, compilers never generate these forms, and manual warns user
 * against them.
 *
 * 880325 The code for TDIFF's is o.k.  If the TDIFF reduces to an absolute
 * this respresents a displacement, and the TDIFF value could only shrink.
 * The <sym>+<offset> case must assume the worst case, because of the
 * problems shown above.  The difference is that an absolute is a
 * *displacement*, a symbol (<sym> or <sym>+d) is a *target*.
 * A target of the form <dot>+d is o.k. because it always resolves to
 * a displacement of d-2.  Use the one-time calculation.
 *
 */
setup_sdi_target(span)
  register struct sdi * span;
{ register rexpr * exp;
  long  exptype;
  symbol * lsym, * rsym;

  SET_DOT_VALUE(span->sdi_location);
  SET_LINE(span);
  exp = &span->sdi_exp;
  if (exp->exptype&TDIFF) eval_tdiff(exp);
  exptype = exp->exptype;

  if (exptype==TYPL) {
	/* bCC  <abs>
	 * The <abs> is the displacement.
	 */
	span->sdi_target = 0;
	span->sdi_offset = exp->expval.lngval;
	span->sdi_span = span->sdi_offset;
	span->sdi_nodetype |= SDSPANFXD;
	/* still need to set a size and length, and delta's */
	update_node(span);
	return;
	}

   if (exptype==TSYM && exp->symptr->stype&SABS) {
	/* Just like the TYPL case */
	span->sdi_target = 0;
	span->sdi_offset = exp->symptr->svalue + exp->expval.lngval;
	span->sdi_span = span->sdi_offset;
	span->sdi_nodetype |= SDSPANFXD;
	update_node(span);
	return;
	}

   if (exptype==TSYM) {
	if (!(exp->symptr->stype&STEXT)) {
		/* NOTE: to implement SDEXTRN case, at this point mark the
		 * span node as being of size SDEXTRN, and mark the type
		 * as SDSPANFXD.
		 */
#ifdef PIC
                if (pic_flag) {
	           span->sdi_nodetype |= SDSPANFXD;
		   span->sdi_size = SDEXTN;
		}
		else {
		   uerror("branch target not in text segment");
		}
#else
		uerror("branch target not in text segment");
#endif
		return;
		}
	span->sdi_target = exp->symptr->ssdiinfo;
	span->sdi_offset = exp->expval.lngval;
	span->sdi_span = (exp->symptr->svalue + span->sdi_offset) - span->sdi_base;
	if (exp->symptr==dot || span->sdi_target==0 ) {
		span->sdi_nodetype |= SDSPANFXD;
		update_node(span);
		return;
		}
	else if (exp->expval.lngval!=0 ) {
		/* not simple label -- assume a worst case size  */
		int delta;
# ifdef SDDEBUG
		werror("non simple label -- assume worst case size");
# endif
		span->sdi_nodetype |= SDSPANFXD;
		span->sdi_size = SDLONG;
		/* expect the size to be same for SDLONG and SDUNKN, but
		 * just in case.
		 */
		if ( (delta=sdi_length[span->sdi_optype][SDUNKN] -
		           sdi_length[span->sdi_optype][SDLONG]) ) {
			span->sdi_length = sdi_length[span->sdi_optype][SDLONG];
			span->sdi_tdelta = delta;
			text_idelta += delta;
			text_tdelta += delta;
			}
		return;		/* don't do a call to update_node() */
		}
	else {
		update_node(span);
		return;
		}
	}
   else {
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("bad exptype in check_sdi_target");
	return;
	}
}



/* Update the given span-node, assuming that the sdi_span field is
 * uptodate see if the instruction could be shortened.
 */
update_node(span)
  struct sdi * span;
{  int ssize;
   int adjflg;
   adjflg = !(span->sdi_nodetype&SDSPANFXD);
   ssize = spansize(span->sdi_optype, span->sdi_span, span->sdi_size, adjflg);
   if (ssize != span->sdi_size) {
      int newlength = sdi_length[span->sdi_optype] [ssize];
      int delta = newlength - span->sdi_length;

# ifdef SDDEBUG
      printf("shortening size for span (%x) to %d\n", span, newlength);
      printf("ssize = %d, newlength = %d, delta = %d\n",ssize, newlength, delta);
# endif
      span->sdi_size = ssize;
      span->sdi_length = newlength;
      span->sdi_tdelta += delta;
      span->sdi_idelta += delta;
      text_idelta += delta;
      text_tdelta += delta;	/* could calculate in sdo_cleanup;  putting
				 * it here gives a "running" total we can
				 * look at for debugging.
				 */

      if (ssize==SDBYTE || ssize==SDZERO)
	 span->sdi_nodetype |= SDSPANFXD;
      }

}


/* Determine the SD-size instruction needed based on the span size.
 * Currently the calculation is the same for every instruction type.
 * NOTE:  the upper bounds tests may appear to be too high, but
 * remember that we are coming down from an initial maximum size, so
 * bound test allows for the decrease in the displacement size which
 * is itself part of the span. Otherwise, would would miss the
 * cross-over cases, where the larger displacement itself makes the
 * difference.
 */
 /* sdsize is the current SD... so can account for change in disp
  * part itself for forward displacements.
  */
# define NEWSPAN(size) (adjflg?span-(current_size-sdi_length[sditype][size]):span)

int spansize(sditype, span, sdsize, adjflg) {
   int current_size;		/* current bytes */

   current_size = sdi_length[sditype][sdsize];

   /*if (NEWSPAN(SDZERO)==sdi_length[sditype][SDZERO]-2) return (SDZERO);*/
   if (NEWSPAN(SDBYTE)==0) return (SDZERO);
   else if (span >= -128 && NEWSPAN(SDBYTE) <= 126) return(SDBYTE);
   else if (span >= -32768 && NEWSPAN(SDWORD) <= 32766) return(SDWORD);
   else return (SDLONG);
}


/*  Scan through the span-info list and get the location info 
 *  updated, and ready for the next goround.
 */
update_spaninfo()
{
   register struct sdi * p;
   register long delta = 0;

   for(p = sdi_listhead; p != 0; p=p->sdi_next) {
	p->sdi_location += delta;
	if ((p->sdi_nodetype&SDNDTYPE)==SDSPAN) {
	   p->sdi_base += delta;
	   delta += p->sdi_idelta;
	   p->sdi_idelta = 0;
	   }
	}

   /* Integrity check */
   if (text_idelta != delta)
	/* (line number on error message not useful here anyway) */
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("delta inconsistency in span optimization loop");

}

/* Map a SDsize to an operation size -- for pass2 fixup of the
 * intermediate instructions.
 */
int  map_sdisize(sditype, sdsize)
{
   switch(sdsize) {
	default:
	case SDEXTN:
#ifdef PIC
                if (pic_flag) {
		   return(SZLONG);
		}
		else {
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		   aerror("bad value for sdsize in pass2");
		}
#else
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("bad value for sdsize in pass2");
#endif
		break;
	case SDUNKN:
		/* lineno for this message set in fix_bcc routine */
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("bad value for sdsize in pass2");
		break;
	case SDZERO:
		if (sditype==SDBSR || sditype==SDCPBCC) return(SZWORD);
		else return(SZNULL);
		break;
	case SDBYTE:
		if (sditype==SDCPBCC) return(SZWORD);
		else return(SZBYTE);
		break;
	case SDWORD:
		return(SZWORD);
		break;
	case SDLONG:
		return(SZLONG);
		break;
	}

}


struct sdi *
makesdi_lalignnode(lalmod)
  int lalmod;
{
   struct sdi * p;

   /* debug print */
# ifdef SDDEBUG
   printf("sdi lalign being created location <%d>, size <%d>\n", newdot,
	lalmod);
# endif
   p = (struct sdi *) calloc(1,sizeof(struct sdi));
   if (p==0)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("unable to calloc space for span dependent info");

   /* fill in the information (as much as possible) in the structure.
    * use the sdi_span field to store the lalmod value.
    */
   p->sdi_location0 = newdot;
   p->sdi_location = newdot;
   p->sdi_exp.symptr = 0;
   p->sdi_target = 0;
   p->sdi_nodetype = SDLALIGN;
   p->sdi_optype = 0;
   p->sdi_size = 0;
   p->sdi_length = 0;
   p->sdi_span = lalmod;
   p->sdi_offset = lalmod - 2;	/* initial worst case number of filler bytes */
   p->sdi_tdelta = 0;
   p->sdi_idelta = 0;
   p->sdi_base = 0;
   p->sdi_line = line;

   /* link it into the list */
   sdi_listtail->sdi_next = p;	/* first time, this is really setting
				 * sdi_listhead */
   p->sdi_next = 0;
   sdi_listtail = p;
   return(p);
}


/* sdi_dotadjust:
 *	given a pointer to an sdi node, and an offset, return the adjustment 
 *	to the location counter for all code up through and including 
 *	possibly including this sdi node. The sdi node itself is too be
 *	included if offset>sdi_location0, meaning that the sdi comes
 *	before the offset in question. (For slt uses, this will always be
 *	true;  for the lister use, the lister node is built at the end
 *	of the line processing;  if the line had a span-node, we'll be
 *	pointing to that sdi now, but don't want to include the change in
 *	this sdi node in the dotadjust calculation.).
 *	This routine is called from the pass2 slt handler to determine the 
 *	adjustment needed to the sltoffset after span-dependent optimization.
 *	It is also used by the lister to determine true offsets after
 *	span-dependent optimization.
 */
long sdi_dotadjust(psdi,offset)
  struct sdi * psdi;
  long offset;
{ long sdi_delta = 0;
  if (psdi !=  (struct sdi *)&sdi_listhead) {
	sdi_delta = psdi->sdi_location - psdi->sdi_location0;
	/* also add in the delta of this node if its location0 is
	 * < the offset parameter (meaning that the sdi comes before
	 * the offset in question. (only span and lalign nodes have
	 * a nonzero value)
	 */
	if (psdi->sdi_location0 < offset) sdi_delta += psdi->sdi_tdelta;
	}
  return(sdi_delta);
}

# endif  /* ifdef SDOPT */
