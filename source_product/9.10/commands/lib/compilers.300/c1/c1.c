/* file c1.c */
/* @(#) $Revision: 70.2 $ */
/* KLEENIX_ID @(#)c1.c	16.3.1.2 91/01/18 */

# include "c1.h"

# define NSTACKSZ 250
# define SAFEVAL(x)	(flag)(((x)>>12)&01)


void		process_name_icon();
void		process_oreg_foreg();
NODE		*basic_node();

int  		pseudolbl = PSL;/* locally generated label number counter */
int 		nlabs;		/* number of labels for latest computed goto */
BBLOCK 		*topblock;	/* ptr to first basic block in the function */
BBLOCK 		*lastblock;	/* ptr to last active block in table */
LBLOCK		*lastlbl;	/* ptr to last active label in table. */
CGU 		*asgtp;		/* array of labels for ficonlabs */
CGU 		*llp;		/* array of labels for latest computed goto */
NODE	   	*lastnode;	/* ptr to last used node in current block. */
int 		ficonlabsz;	/* # active elements in asgtp */
int 		ficonlabmax;	/* size of asgtp array */
char		*procname;
char		lcrbuf[NCHNAM+1]; /* a temporary name building buffer */
char		filename[FTITLESZ];  	/* the name of the file */
unsigned char 	lastop;		/* history why new block was needed */
unsigned char 	breakop;	/* history why new block was needed */
flag		structflag;
flag		isstructprimary;
flag		arrayflag;
flag		arrayelement;
flag 		need_block;
flag 		postponed_nb;	/* delay new block for 1 FEXPR */
flag		assigned_goto_seen;	/* ASSIGNED GOTO seen in this func. */
flag 		need_ep;
flag		asm_flag;	/* in procedure with C "asm" statement? */
flag		in_procedure;	/* in procedure now ? */
	flag		reading_input = YES;
# ifdef DEBUGGING
	char		*initial_sbrk;		/* for determining heap use */
# endif DEBUGGING
	flag		allow_insertion = NO;

LOCAL NODE 	*fstack[NSTACKSZ];
LOCAL NODE	**structnode;
LOCAL long 	maxstructnode;
LOCAL long	laststructnode;
LOCAL long	tysize[] = {4, 4, 1, 2, 4, 4, 4, 8, 4, 4, 4, 4, 1, 2, 4, 4, 4,
				4, 4};   /* size of each type, in bytes */

LOCAL NODE      *rewrite_c_array();
LOCAL long	is_in_namelist();
LOCAL void	propagate_flags();
# ifdef FTN_POINTERS
LOCAL void	find_fortran_pointer_targets();
# endif FTN_POINTERS
LOCAL flag	c0tempflag;
LOCAL flag	disable_procedure; /* Found reason not to optimize procedure */

# ifndef NOLREAD
/******************************************************************************
*
*	lread()
*
*	Description:		reads 1 4-byte record while checking for a read
*				error 
*
*	Called by:		main()
*				do_hiddenvars()
*				basic_node()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	lrd
*
*	Globals modified:	none
*
*	External calls:		fread()
*				cerror()
*
*******************************************************************************/
int lread(){
	static int x;
	if( fread( (char *) &x, 4, 1, lrd ) <= 0 )
#pragma BBA_IGNORE
		cerror( "intermediate file read error" );
	return( x );
	}
# endif NOLREAD


/*****************************************************************************
 *
 *  EXTRACT_CONSTANT_SUBTREE()
 *
 *  Description:	Extract constants from a array reference (may be in
 *			a structure).  Represent the constant offset
 *			explicitly.
 *
 *                              +               +
 *                             / \    ==>      / \
 *                         base   subs        +   ICON
 *                                           / \
 *                                       base'  subs'
 *
 *
 *  Called by:
 *			computed_aref_plus()
 *			computed_aref_unary_mul()
 *			computed_ptrindir()
 *			computed_structref()
 *			fix_array_form()	(misc.c)
 *			node_constant_fold()	(misc.c)
 *
 *  Input Parameters:	p -- top PLUS node of tree
 *			baseoffset -- base address of array/structure
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	bcon()			(misc.c)
 *			offset_from_subtree()	(regpass2.c)
 *			talloc()
 *
 *****************************************************************************
 */

void extract_constant_subtree(p, baseoffset)
NODE *p;
long baseoffset;
{
    long offset;
    NODE *newsubtree;
    NODE *t;

    /* extract any constants from the base address */
    offset = p->in.left->tn.lval - baseoffset;
	
    /* If the base of an array is an OREG node, the offset must
     * be consistant with the value in baseoffset.  If not issue
     * an error because an invalid transformation is about to occur.
     */
    if ((offset != 0) && p->in.left->in.op == OREG)
#pragma BBA_IGNORE
	cerror("extract_constant_subtree: Invalid OREG as arraybase");

    if (p->in.left->in.op == REG) /* turn REG node into an FOREG node */
	{
	p->in.left->in.op = FOREG;
	p->in.left->tn.lval = baseoffset;
	}
    else
	p->in.left->tn.lval = baseoffset;

    /* add constants from the subscripts */
    offset += offset_from_subtree(p->in.right, &newsubtree);

    if (offset == 0)
	{
	if (newsubtree == NULL)
	    {
	    /* shrink tree to single node -- the base address */
	    t = p->in.left;
	    /* copy needed flags */
	    t->tn.structref = p->tn.structref;
	    t->tn.arrayref = p->tn.arrayref;
	    t->tn.arrayrefno = p->tn.arrayrefno;
	    t->tn.isarrayform = p->tn.isarrayform;
	    *p = *t;
	    t->in.op = FREE;
	    change_parent(p /*node*/, p /*old parent*/);
	    }
	else
	    {
	    p->in.right = newsubtree;
	    }
	}
    else
	{
	if (newsubtree == NULL)
	    {
	    /* subscripts are the constant */
	    p->in.right = bcon(offset, INT);
	    }
	else
	    {
	    /* add constant and PLUS node to tree */
	    t = talloc();
	    *t = *p;
	    t->in.right = newsubtree;
	    t->in.left = p->in.left;
	    t->allo.flagfiller = 0;
	    p->in.right = bcon(offset, INT);
	    p->in.left = t;
	    }
	}
}   /* extract_constant_subtree */

/******************************************************************************
*
*	computed_aref_unary_mul()
*
*	Description:		computed_aref_unary_mul() handles arrays with
*				indices determined dynamically.  These are
*				typically topped with a UNARY MUL node and have
*				an ultimate offset calculation tree as children.
*				Common is particularly troublesome.
*
*	Called by:		main()
*
*	Input parameters:	p -- pointer to UNARY MUL on top of tree
*				aoffset -- array base address
*
*	Output parameters:	none
*
*	Globals referenced:	arrayelement
*				f_find_array
*				fortran
*				symtab
*
*	Globals modified:	arrayelement
*				f_find_array
*
*	External calls:		cerror()
*				decref()
*				extract_constant_subtree()
*				find()
*				process_name_icon()
*				rewrite_c_array()
*				talloc()
*
*******************************************************************************/
LOCAL void computed_aref_unary_mul(p, aoffset)
	register NODE *p;
	int aoffset;
{
	register NODE *t;
	register NODE *pleft = p->in.left;
	register HASHU *sp;

	if (pleft->in.op == OREG)
		goto dynamic_array;

	if ( !fortran && (pleft->in.op == ICON) )
		{
		/* C does not optimize constant array refs away like Fortran */

		*p = *pleft;
		pleft->in.op = FREE;
		p->in.op = NAME;
		p->tn.type = decref(p->tn.type);
		arrayelement = YES;
		process_name_icon(p, aoffset, 0);
		return;
		}

	if (! fortran && (pleft->in.op == UNARY MUL))
	    {
	    p->in.isptrindir = YES;
            if ((pleft->in.op == UNARY MUL) &&
                (pleft->in.left->in.op == ICON))
              { /* Convert global var a[0][0] into *a[0] form */
              p->in.isarrayform = YES;
              pleft = pleft->in.left;
	      *p->in.left = *pleft;
	      pleft->in.op = FREE;
	      p->in.left->in.op = NAME;
	      p->in.left->tn.type = decref(p->in.left->tn.type);
	      arrayelement = YES;
	      process_name_icon(p->in.left, aoffset, 0);
	      return;
              }
            else
	      do  {
		  p = pleft;
		  pleft = p->in.left;
		  } while (pleft->in.op == UNARY MUL);
	    }

	if (pleft->in.op != PLUS)
#pragma BBA_IGNORE
		{
		if (!fortran)
			{
			werror("Malformed array reference. Discontinuing optimization.");
			disable_procedure = YES;
			return;
			}
		goto common_error;
		}

	if (! fortran)
	    {
	    t = rewrite_c_array(pleft);
	    if (t != pleft)
		{
		p->in.isptrindir = YES;
		switch(t->in.op)
		    {
		    case OREG:
			arrayelement = YES;
			process_oreg_foreg(t, aoffset, 0);
			return;
		    case NAME:
			arrayelement = YES;
			process_name_icon(t, aoffset, 0);
			return;
		    case REG:
			return;
		    default:
			p = t;		/* process true array subtree */
			pleft = p->in.left;
			break;
		    }
		}
	    }

	if (pleft->in.left->in.op == OREG)
		{
		/* this is ambiguous -- could be an farg array or a static
		   array.
		 */
		if ((pleft->in.right->in.op == ICON)
		 && (pleft->in.right->atn.name != NULL))
		    goto static_array;
		else
		    goto dynamic_array;
		}

	if (pleft->in.left->in.op == REG)
		{
		if ((pleft->in.right->in.op == PLUS)
		 || (pleft->in.right->in.op == MINUS))
		    {
		    t = pleft->in.right->in.right;
		    if ((t->in.op != ICON) || t->atn.name)
#pragma BBA_IGNORE
		        cerror("expecting ICON in computed_aref_unary_mul()");
		    pleft->in.left->in.op = FOREG;
		    pleft->in.left->tn.lval = (pleft->in.right->in.op == PLUS) ?
					t->tn.lval : - t->tn.lval;
		    t->in.op = FREE;
		    t = pleft->in.right;
		    pleft->in.right = t->in.left;
		    t->in.op = FREE;
		    }
		else if ((pleft->in.right->in.op != OREG)
		      && ((pleft->in.right->in.op != SCONV)
		       || (pleft->in.right->in.left->in.op != OREG)))
#pragma BBA_IGNORE
		    cerror("expecting PLUS/MINUS/OREG in computed_aref_unary_mul()");

		/* This must be a computed array ref on the stack */
dynamic_array:
		if ((((t = pleft)->in.op == OREG)
		  || ((t = pleft->in.left)->in.op == OREG))
		 && t->tn.isc0temp)
		    {
		    p->in.arrayrefno = symtab[find(t)]->a6n.wholearrayno;
		    }
		else
		    {
		    t = talloc();
		    *t = *p;
		    t->in.op = OREG;
		    t->tn.lval = aoffset;

		    f_find_array = YES;
		    p->in.arrayrefno = find(t);
		    f_find_array = NO;

		    t->in.op = FREE;
		    }

		p->in.arrayref = YES;
		p->in.arrayelem = arrayelement;
		p->in.isarrayform = YES;

		sp = symtab[p->in.arrayrefno];

		if (sp->xn.common)
		    p->in.common_base = YES;
		if (sp->x6n.equiv)
		    p->tn.equivref = YES;

		if (((t = pleft)->in.op == OREG)
		  || ((t = pleft->in.left)->in.op == FOREG)
		  || (t->in.op == OREG))
		    {
		    t->in.arraybaseaddr = YES;
		    t->in.arrayrefno = p->in.arrayrefno;
		    }
                /* Check for a[0][0] and convert to *a[0] */
                if ((!fortran) && (pleft->in.op == OREG)) 
                    {
                    p->in.arrayref = NO;
                    p->in.isptrindir = YES;
                    pleft->in.arrayref = YES;
                    }
		}
	else
		{
		/* This must be a static computed array ref. */
static_array:
		t = talloc();
		*t = *p;
		t->in.op = ICON;
		p->in.arrayref = YES;
		p->in.isarrayform = YES;
		p->in.arrayelem = arrayelement;
		if ((pleft->in.right->in.op != ICON)
		 || (! pleft->in.right->atn.name))
#pragma BBA_IGNORE
			goto common_error;
		t->tn.lval = aoffset;
		t->atn.name = pleft->in.right->atn.name;
		f_find_array = YES;
		p->in.arrayrefno = find(t);
		f_find_array = NO;
		pleft->in.right->in.arraybaseaddr = YES;
		pleft->in.right->in.arrayrefno = p->in.arrayrefno;

		sp = symtab[p->in.arrayrefno];
		if (sp->xn.common)
		    p->in.common_base = YES;
		if (sp->xn.equiv)
		    p->tn.equivref = YES;
		t->in.op = FREE;

		/* swap sides -- base address on left */
		t = pleft->in.right;
		pleft->in.right = pleft->in.left;
		pleft->in.left = t;
		}
	
	/* extract constant subscripts */
	if (pleft->in.op == PLUS)
	    {
	    /* A bogus symbol table entry may have been created. */
            if (((pleft->in.left->in.op == ICON) && 
                 (pleft->in.left->atn.name != NULL)) &&
                (pleft->in.left->tn.lval != aoffset) &&
		!fortran)
	      {
	      int sym_index;

	      sym_index = find(pleft->in.left);
	      if ((sym_index != (unsigned) (-1)) &&
		  !symtab[sym_index]->an.array &&
		  (symtab[sym_index]->an.pass1name != NULL) &&
		  (symtab[sym_index]->an.pass1name[0] != 'L'))
		cerror("A bogus symbol table entry has been detected.");
              }

	    extract_constant_subtree(pleft, aoffset);
	
	    /* Check if we can fold down to simple NAME or OREG node */
	    if (((pleft->in.op == FOREG) || (pleft->in.op == ICON))
	     && (p->in.op == UNARY MUL))
	        node_constant_collapse(p);
	    }
	return;
common_error:
	cerror("strange shape for array node in computed_aref_unary_mul()");
}

/******************************************************************************
*
*	computed_aref_plus()
*
*	Description:		computed_aref_plus() handles arrays with indices
*				determined dynamically.  These are typically
*				topped with a PLUS node and have an ultimate
*				offset calculation tree as children. Common is
*				particularly troublesome.
*
*	Called by:		main()
*
*	Input parameters:	pp -- pointer to pointer to top PLUS node of
*					tree
*				arrayoffset -- array base offset
*
*	Output parameters:	none
*
*	Globals referenced:	arrayelement
*				f_find_array
*				fortran
*				symtab[]
*
*	Globals modified:	f_find_array
*
*	External calls:		cerror()
*				extract_constant_subtree()
*				find()
*				rewrite_c_array()
*				talloc()
*
*******************************************************************************/

LOCAL void computed_aref_plus(p, arrayoffset)
register NODE *p;
long arrayoffset;
{
    register HASHU *ap;
    register NODE *q;		/* array base node for symtab find */
    register NODE *t;
    NODE *topp;

    if (p->in.op == FOREG)
	{
#pragma BBA_IGNORE
	cerror("FOREG in computed_aref_plus");
	}
    else
	{
	topp = p;
	if (! fortran)
	    {
	    /* find top of true array ref in C tree */
	    t = rewrite_c_array(p);
	    if (t != p)
		{
		switch(t->in.op)
		    {
		    case OREG:
			arrayelement = YES;
			process_oreg_foreg(t, arrayoffset, 0);
			p->in.arrayrefno = t->in.arrayrefno;
			return;
		    case NAME:
			arrayelement = YES;
			process_name_icon(t, arrayoffset, 0);
			p->in.arrayrefno = t->in.arrayrefno;
			return;
		    case REG:
			return;
		    default:
			p = t->in.left;		/* process true array subtree */
			break;
		    }
		}
	    }
	t = p->in.left;			/* array base address */
	q = talloc();			/* array base node for symtab find */
	if (t->in.op == OREG)
	    {
	    if ((p->in.right->in.op == ICON) && (p->in.right->atn.name != NULL))
	        q->in.op = NAME;	/* static array -- base on rhs */
	    else
		{
	        q->in.op = OREG;	/* dynamic array */
		if (t->tn.isc0temp)
		    q->tn.isc0temp = YES;
		}
	    }
        else if (t->in.op == REG)	/* dynamic array */
	    {
	    if ((p->in.right->in.op == PLUS)
	      || (p->in.right->in.op == MINUS))
		{
	        /* rewrite as FOREG:

                          +                         +
                         / \                       / \
                      REG   +         ==>     FOREG   subs
                           / \
                       subs   ICON

		*/
		
	        t = p->in.right->in.right;
	        if ((t->in.op != ICON) || t->atn.name)
#pragma BBA_IGNORE
		    cerror("expecting ICON in computed_aref_plus()");
	        p->in.left->in.op = FOREG;
	        p->in.left->tn.lval = (p->in.right->in.op == PLUS) ?
					t->tn.lval : - t->tn.lval;
	        t->in.op = FREE;
	        t = p->in.right;
	        p->in.right = t->in.left;
	        t->in.op = FREE;
		}
	    else if (p->in.right->in.op != OREG)
#pragma BBA_IGNORE
		cerror("expecting PLUS/MINUS/OREG in computed_aref_plus()");

	    q->in.op = OREG;	/* dynamic array */
	    }
        else if (t->in.op == FOREG)
	    q->in.op = OREG;	/* dynamic array */
        else
	    q->in.op = NAME;	/* static array */
	}
    q->in.type = p->in.type;
    p->in.arrayref = YES;
    p->in.isarrayform = YES;
    p->in.arrayelem = arrayelement;
    q->tn.lval = arrayoffset;
    if (q->in.op == NAME)
	{
	if ((p->in.right->in.op != ICON) || (p->in.right->atn.name == NULL))
#pragma BBA_IGNORE
	    cerror("strange array shape in computed_aref_plus()");
        q->atn.name = p->in.right->atn.name;

	/* swap sides -- base address always on left */
	t = p->in.right;
	p->in.right = p->in.left;
	p->in.left = t;
	}

    /* find the array base in the symtab */
    if (q->in.isc0temp)
	{
        topp->in.arrayrefno =
		p->in.arrayrefno = symtab[find(q)]->a6n.wholearrayno;
	}
    else
	{
	f_find_array = YES;
	topp->in.arrayrefno = 
		p->in.arrayrefno = find(q);
	f_find_array = NO;
	}

    ap = symtab[p->in.arrayrefno];

    /* copy array attributes to the tree node */
    if (ap->xn.equiv)
        p->in.equivref = YES;
    if (ap->xn.common)
        p->in.common_base = YES;

    if ((p->in.op != FOREG) && ((t = p->in.left)->in.op != REG))
	{
	t->in.arraybaseaddr = YES;
	t->in.arrayrefno = p->in.arrayrefno;
	}

    q->in.op = FREE;	/* throw it away */

    /* extract constants from subscript tree */
    if (p->in.op == PLUS)
	/* A bogus symbol table entry may have been created.
	 * If this is the case, set its type to CHAR so that
	 * it will not have any affect on look_harder().     */
        if (((p->in.left->in.op == ICON) && 
             (p->in.left->atn.name != NULL)) &&
            (p->in.left->tn.lval != arrayoffset))
	  {
	  int sym_index;

	  sym_index = find(p->in.left);
	  if ((sym_index != (unsigned) (-1)) && !symtab[sym_index]->an.array)
            {	
	    symtab[sym_index]->allo.type.base = CHAR; 
	    symtab[sym_index]->an.ptr = NO;
	    }
          }

        extract_constant_subtree(p, arrayoffset);

    return;

}  /* computed_aref_plus */

/*****************************************************************************
 *
 *  REWRITE_C_ARRAY()
 *
 *  Description:	Re-write a C array into the corresponding FORTRAN form.
 *
 *  Called by:		computed_aref_plus()
 *			computed_aref_unary_mul()
 *
 *  Input Parameters:	top -- pointer to top PLUS node
 *
 *  Output Parameters:	top -- node modified -- new tree
 *			return value - top pure array node in tree
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	block()
 *			cerror()
 *			decref()
 *			incref()
 *			talloc()
 *
 *****************************************************************************
 */

LOCAL NODE *rewrite_c_array(top)
NODE *top;
{
    NODE *base;		/* base address node */
    NODE *subs;		/* subscript tree */
    NODE *p;		/* current PLUS node in tree */
    NODE *lp;		/* left subtree of p */
    NODE *rp;		/* right subtree of p */
    NODE *arytop;	/* top of true array subtree */

    /* find top of true array subtree */
    arytop = top;
    p = top;

    while (1)
	{
	lp = p->in.left;
	rp = p->in.right;

	/* process right subtree */
	if ((rp->in.op == ICON) && (rp->atn.name != NULL)
	 && (ISPTR(rp->atn.type)))
	    {
	    /* right subtree is base address */
	    /* left subtree must be more subscripts */
	    break;	/* exit loop */
	    }

	/* process left subtree */
	if (lp->in.op == FOREG)
	    {
	    break;	/* exit loop */
	    }
	else if (lp->in.op == UNARY MUL)
	    {
	    p = lp->in.left;
	    switch(p->in.op)
		{
		case PLUS:
		    arytop = lp;
		    break;

		case ICON:
		    *lp = *p;
		    p->in.op = FREE;
		    lp->tn.type = decref(lp->tn.type);
		    lp->in.op = NAME;
		    return(lp);
/*		    goto exitloop1;
*/

		default:
#pragma BBA_IGNORE
	            cerror("expecting PLUS/ICON in rewrite_c_array()");
		    break;
		}
	    }
	else if ((lp->in.op == ICON) && (lp->atn.name != NULL)
		 && (ISPTR(lp->atn.type)))
	    {
	    break;	/* exit loop */
	    }
	else if (lp->in.op == PLUS)
	    {
	    p = lp;	/* recurse on left subtree */
	    }
	else if (((lp->in.op == OREG) || (lp->in.op == REG))
	      && ISPTR(lp->atn.type))
	    {
	    return(lp);
	    }
	else if ((rp->in.op == ICON) && (rp->atn.name != NULL))
	    {
	    rp->atn.type = incref(rp->atn.type, PTR, 0);
	    break;
	    }
	else
	    {
	    cerror("expecting PLUS in rewrite_c_array()");
	    }
	}

exitloop1:

    base = NULL;
    subs = NULL;
    if (arytop != top)
	top = arytop->in.left;
    p = top;

    /* collect base and subscripts */
    while (1)
	{
	lp = p->in.left;
	rp = p->in.right;
	if (p != top)	/* throw it away */
	    p->in.op = FREE;

	/* process right subtree */
	if ((rp->in.op == ICON) && (rp->atn.name != NULL)
	 && (ISPTR(rp->atn.type)))
	    {
	    base = rp;

	    /* left subtree must be more subscripts */
	    if (subs == NULL)
		subs = lp;
	    else
		subs = block( PLUS, lp, subs, INT);
	    break;	/* exit loop */
	    }
	else  /* must be more subscripts */
	    {
	    if (subs == NULL)
		subs = rp;
	    else
		subs = block( PLUS, rp, subs, INT);
	    }

	/* process left subtree */
	if ((lp->in.op == FOREG)
	 || ((lp->in.op == ICON) && (lp->atn.name != NULL)
	 && (ISPTR(lp->atn.type))))
	    {
	    base = lp;
	    break;	/* exit loop */
	    }
	else if (lp->in.op != PLUS)
	    {
#pragma BBA_IGNORE
	    cerror("expecting PLUS in rewrite_c_array()");
	    }
	else
	    {
	    p = lp;	/* recurse on left subtree */
	    }
	}

    /* write the base and subscripts in a FORTRAN form */
    if (base->in.op == ICON)
	{
	top->in.right = base;
	top->in.left = subs;
	}
    else	/* base is FOREG */
	{
	p = talloc();
	p->in.op = ICON;
	p->in.type.base = INT;
	p->in.type.mods1 = 0;
	p->in.type.mods2 = 0;
	p->atn.lval = base->tn.lval;
	top->in.right = block( PLUS, subs, p, INT);
	top->in.left = base;
	base->in.op = REG;
	base->tn.lval = 0;
	}

    return(arytop);

}  /* rewrite_c_array */

/*****************************************************************************
 *
 *  COMPUTED_PTRINDIR()
 *
 *  Description:	Put pointer indirection into array form, if possible.
 *
 *  Called by:		main()
 *
 *  Input Parameters:	np -- pointer to top UNARY MUL on reference
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	block()
 *			extract_constant_subtree()
 *
 *****************************************************************************
 */

void computed_ptrindir(np)
NODE *np;			/* points to top UNARY MUL */
{
    register NODE *p;
    register NODE *lp;
    register NODE *rp;
    NODE *base;
    NODE *subs;

    np->in.isarrayform = NO;	/* default */

    /* First descend tree, verifying form is OK for array reference */
    p = np->in.left;		/* top PLUS ?? */

    if ((p->in.op == NAME) || (p->in.op == OREG))
	{
	np->in.isarrayform = YES;
	return;
	}
    if ((p->in.op != PLUS) || !ISPTR(p->in.type))
	return;		/* no good */

    while (1)
	{
	lp = p->in.left;
	rp = p->in.right;
	if (ISPTR(lp->in.type) && ((lp->in.op == NAME) || (lp->in.op == OREG)))
	    {
	    break;
	    }
	if (ISPTR(rp->in.type) && ((rp->in.op == NAME) || (rp->in.op == OREG)))
	    {
	    /* canonicalize */
	    p->in.right = lp;
	    p->in.left = rp;
	    break;
	    }
	if ((lp->in.op == PLUS) && ISPTR(lp->in.type))
	    {
	    p = lp;
	    }
	else if ((rp->in.op == PLUS) && ISPTR(rp->in.type))
	    {
	    /* canonicalize */
	    p->in.right = lp;
	    p->in.left = rp;
	    p = rp;
	    }
	else
	    return;		/* no good */
	}

    /* collect base and subscripts */
    base = NULL;
    subs = NULL;

    p = np->in.left;

    while (1)
	{
	lp = p->in.left;
	rp = p->in.right;
	if (p != np->in.left)
	    p->in.op = FREE;	/* throw it away */

	if (subs == NULL)
	    subs = rp;
	else
	    subs = block( PLUS, rp, subs, INT);
	if (ISPTR(lp->in.type) && ((lp->in.op == NAME) || (lp->in.op == OREG)))
	    {
	    base = lp;
	    break;
	    }
	else
	    p = lp;
	}

    /* write it as a tree */
    p = np->in.left;
    p->in.left = base;
    p->in.right = subs;

    /* extract constant subscripts */
    extract_constant_subtree(p, base->tn.lval);

    np->in.isarrayform = YES;


}  /* computed_ptrindir */

/*****************************************************************************
 *
 *  COMPUTED_STRUCTREF()
 *
 *  Description:	Process a multi-node structure reference
 *
 *  Called by:		main()
 *
 *  Input Parameters:	ptopnode -- pointer to pointer to top node
 *			basenode -- pointer to structref base node (flagged by
 *				secondary STRUCTREF in input file)
 *			structoffset -- structure base address (from
 *				STRUCTREF record)
 *
 *  Output Parameters:	modified tree
 *
 *  Globals Referenced:	f_find_struct
 *			fortran
 *
 *  Globals Modified:	f_find_struct
 *
 *  External Calls:	block()
 *			decref()
 *			extract_constant_subtree()
 *			find()
 *
 *****************************************************************************
 */

LOCAL void computed_structref(ttopnode, basenode, structoffset)
NODE *ttopnode;
NODE *basenode;
long structoffset;
{
    register NODE *p;
    register NODE *lp;
    register NODE *rp;
    NODE *base;
    NODE *subs;
    NODE *topnode;
    long save;

    allow_insertion = YES;
    if (ttopnode->in.op == UNARY MUL)
	{
        topnode = ttopnode->in.left;		/* top PLUS ?? */
	while (topnode->in.op == UNARY MUL)
	    {
	    ttopnode->tn.isptrindir = YES;
	    ttopnode = topnode;
	    topnode = topnode->in.left;
	    }
	}
    else
	topnode = ttopnode;

    p = topnode;

    /* Re-write FORTRAN     +     form to    +
                           / \              / \
                        REG   +        FOREG   sub
                             / \
                          sub   ICON
     */

    if (fortran && (basenode->in.op == REG))
	{
	if (p->in.op != PLUS)
#pragma BBA_IGNORE
	    cerror("expecting FORTRAN PLUS in computed_structref() - 1");
	rp = p->in.right;
	if (rp->in.op == PLUS)
	    {
	    rp = rp->in.right;
	    if ((rp->in.op != ICON) || rp->atn.name)
#pragma BBA_IGNORE
	        cerror("expecting FORTRAN ICON in computed_structref()");
	    basenode->in.op = FOREG;
	    basenode->tn.lval = rp->tn.lval;
	    rp->in.op = FREE;
	    rp = p->in.right;
	    p->in.right = rp->in.left;
	    rp->in.op = FREE;
	    }
	}

    if ((p->in.op == ICON) || (p->in.op == FOREG))
	{
	ttopnode->in.structref = YES;

	/*
               U*      U*
               |  or   |     ==>   NAME or OREG
             ICON    FOREG
	*/

	if (ttopnode->in.op == UNARY MUL)
	    {
	    *ttopnode = *p;
	    ttopnode->in.op = (p->in.op == ICON) ? NAME : OREG;
	    ttopnode->tn.type = decref(ttopnode->tn.type);
	    p->tn.lval = structoffset;

	    /* insert the base into the symbol table */
	    f_find_struct = YES;
	    ttopnode->tn.arrayrefno = find(p);
	    f_find_struct = NO;
	    p->in.op = FREE;
	    }
	else
#pragma BBA_IGNORE
	    cerror("expecting UNARY MUL above ICON/FOREG in computed_structref()");
	return;
	}
    else if ((p->in.op == OREG) && (ttopnode->in.op == UNARY MUL))
	{
	if (p != basenode)
#pragma BBA_IGNORE
	    cerror("confusing OREG in computed_structref()");

	if (p->tn.isc0temp)
	    {
	    /* insert the base into the symbol table */
	    ttopnode->tn.structref = YES;
	    ttopnode->tn.isarrayform = YES;
            ttopnode->tn.arrayrefno = symtab[find(p)]->a6n.wholearrayno;
	    }
	else
	    {
	    /* insert the base into the symbol table */
	    ttopnode->tn.isptrindir = YES;
	    ttopnode->tn.isarrayform = YES;
	    p->tn.structref = YES;
	    save = p->tn.lval;
	    p->tn.lval = structoffset;
	    f_find_struct = YES;
	    p->tn.arrayrefno = find(p);
	    f_find_struct = NO;
	    p->tn.lval = save;
	    }
	return;
	}
    else if ((p->in.op == NAME) && (ttopnode->in.op == UNARY MUL))
	{
	if (p != basenode)
#pragma BBA_IGNORE
	    cerror("confusing NAME in computed_structref()");

	/* insert the base into the symbol table */
	save = p->tn.lval;
	p->tn.lval = structoffset;
	f_find_struct = YES;
	p->tn.arrayrefno = find(p);
	f_find_struct = NO;
	p->tn.lval = save;
	p->tn.structref = YES;
	ttopnode->tn.isptrindir = YES;
	ttopnode->tn.isarrayform = YES;
	return;
	}
    if (((p->in.op != PLUS) && (p->in.op != MINUS)) || !ISPTR(p->in.type))
#pragma BBA_IGNORE
	cerror("unexpected form in computed_structref()");	/* no good */

    /* Descend tree, verifying form is OK for array/struct reference */
    while (1)
	{
	/* assumption -- p is always PLUS node at this point */
	lp = p->in.left;
	rp = p->in.right;
	if (ISPTR(lp->in.type))
	    {
	    if ((lp->in.op == ICON) || (lp->in.op == FOREG)
	     || (fortran && ((lp->in.op == OREG) || (lp->in.op == REG))))
	        {
	        if (lp != basenode)
#pragma BBA_IGNORE
		    cerror("confusing ICON/FOREG/OREG/REG in computed_structref() - 1");
	        break;
		}
	    else if ((lp->in.op == NAME) || (lp->in.op == OREG))
		{
	        if (lp != basenode)
#pragma BBA_IGNORE
		    cerror("confusing NAME/OREG in computed_structref()");
		if (ttopnode->in.op == UNARY MUL)
		    ttopnode->in.isptrindir = YES;

		lp->tn.structref = YES;
		save = lp->tn.lval;
		lp->tn.lval = structoffset;
		f_find_struct = YES;
		lp->tn.arrayrefno = find(lp);
		f_find_struct = NO;
		lp->tn.lval = save;
		return;
		}
	    else if (lp->in.op == PLUS)
	        {
	        p = lp;
	        }
	    else if (lp->in.op == UNARY MUL)
	        {
	        if (ttopnode->in.op == UNARY MUL)
		    ttopnode->in.isptrindir = YES;
	        ttopnode = lp;
	        topnode = lp->in.left;
	        while (topnode->in.op == UNARY MUL)
	            {
	            ttopnode->tn.isptrindir = YES;
	            ttopnode = topnode;
	            topnode = topnode->in.left;
	            }
	        p = topnode;
	    
	        switch (p->in.op)
		    {
		    case ICON:
			if (basenode != p)
#pragma BBA_IGNORE
			    cerror("confusing ICON in computed_structref()");
			*ttopnode = *p;
			p->in.op = FREE;
			ttopnode->tn.type = decref(ttopnode->tn.type);
			ttopnode->in.op = NAME;
			ttopnode->tn.structref = YES;
			
			save = ttopnode->tn.lval;
			ttopnode->tn.lval = structoffset;
			f_find_struct = YES;
			ttopnode->tn.arrayrefno = find(ttopnode);
			f_find_struct = NO;
			ttopnode->tn.lval = save;
			return;

		    case PLUS:
			break;

		    default:
			cerror("expecting PLUS/ICON in computed_structref()");
			break;
		    }
	        }
	    else
#pragma BBA_IGNORE
	        cerror("unexpected form in computed_structref()");
	    }
	else if (ISPTR(rp->in.type))
	    {
	    if ((rp->in.op == ICON) || (rp->in.op == FOREG)
	     || (fortran && ((rp->in.op == OREG) || (rp->in.op == REG))))
	        {
	        if (rp != basenode)
		    cerror("confusing ICON/FOREG/OREG/REG in computed_structref() - 2");

	        /* canonicalize */
	        p->in.right = lp;
	        p->in.left = rp;
	        break;
	        }
	    else if (rp->in.op == PLUS)
	        {
	        /* canonicalize */
	        p->in.right = lp;
	        p->in.left = rp;
	        p = rp;
	        }
	    else if ((rp->in.op == NAME) || (rp->in.op == OREG))
	        {
	        /* swap */
	        p->in.left = rp;
	        p->in.right = lp;
		/* go around again */
	        }
	    else if ((rp->in.op == UNARY MUL) && ISPTR(rp->in.type))
	        {
	        /* swap */
	        p->in.left = rp;
	        p->in.right = lp;
		/* go around again */
	        }
	    else
	        cerror("unexpected form in computed_structref()");
	    }
	else
	    cerror("unexpected form in computed_structref()");
	}

    /* collect base and subscripts */
    base = NULL;
    subs = NULL;

    p = topnode;

    while (1)
	{
	lp = p->in.left;
	rp = p->in.right;
	if (p != topnode)
	    p->in.op = FREE;	/* throw it away */

	if (subs == NULL)
	    subs = rp;
	else
	    subs = block( PLUS, rp, subs, INT);
	if (ISPTR(lp->in.type)
	 && ((lp->in.op == ICON) || (lp->in.op == FOREG)
	  || (fortran && ((lp->in.op == OREG) || lp->in.op == REG))))
	    {
	    base = lp;
	    break;
	    }
	else
	    p = lp;
	}

    /* write it as a tree */
    p = topnode;
    p->in.left = base;
    p->in.right = subs;

    /* insert the base into the symbol table */
    if (base->tn.isc0temp)
	{
        base->tn.arrayrefno = symtab[find(base)]->a6n.wholearrayno;
	}
    else if (base->in.op == REG)
	{
	base->tn.lval = structoffset;
	base->in.op = OREG;
	f_find_struct = YES;
	base->tn.arrayrefno = find(base);
	f_find_struct = NO;
	base->tn.lval = 0;
	base->in.op = REG;
	}
    else
	{
	save = base->tn.lval;
	base->tn.lval = structoffset;
	f_find_struct = YES;
	base->tn.arrayrefno = find(base);
	f_find_struct = NO;
	base->tn.lval = save;
	}

    ttopnode->in.structref = YES;
    ttopnode->in.isarrayform = YES;
    ttopnode->in.arrayrefno = base->tn.arrayrefno;

    /* extract constant subscripts */
    if (base->in.op != REG)
	{
	extract_constant_subtree(p, structoffset);
	
	/* Check if we can fold down to simple NAME or OREG node */
	if (((p->in.op == FOREG) || (p->in.op == ICON))
	 && (ttopnode->in.op == UNARY MUL))
	    node_constant_collapse(ttopnode);
	}
    allow_insertion = NO;

}  /* computed_structref */

/******************************************************************************
*
*	MAIN()
*
*	Description:		obvious
*
*	Called by:		obvious
*
*	Input parameters:	argv - traditional
*				argc - traditional
*
*	Output parameters:	none
*
*	Globals referenced:	qq
*
*	Globals modified:	qq
*
*	External calls:		add_pointer_target()
*				addexpression()
*				addexternasciz()
*				addtreeasciz()
*				basic_node()
*				bfree()
*				cerror()
*				ckalloc()
*				ckrealloc()
*				clean_flowgraph()
*				coalesce_blocks()
*				complete_ptrtab()
*				computed_aref_plus()
*				computed_aref_unary_mul()
*				computed_ptrindir()
*				computed_structref()
*				copy_in_to_out()
*				do_hiddenvars()
*				enterlblock()
*				exit()
*				file_init_storage()
*				file_init_symtab()
*				final_reg_cleanup()
*				find()
*				find_or_add_vfe()
*				flags_and_files()
*				free()		(FREEIT)
*				free_global_data()
*				funcinit()
*				fwalk()
*				fwrite()
*				global_def_use_opts()
*				global_optimize()
*				gofs()
*				inittaz()
*				is_in_namelist()
*				lccopy()
*				lcread()
*				local_optimizations()
*				lread()
*				lstrread()
*				p2flush()
*				p2name()
*				p2pass()
*				p2word()
*				pass_procedure()
*				prmaxlabblank()
*				prmaxlab()
*				proc_init_symtab()
*				process_name_icon()
*				process_oreg_foreg()
*				process_vfes()
*				pushlbl()
*				register_allocation()
*				rewrite_comops()
*				setup_global_data()
*				strcpy()
*				strlen()
*				symtab_insert()
*				talloc()
*				tinit()
*				mallopt()
*				oreg2plus()
*				locate()
*
*******************************************************************************/

/* # pragma OPT_LEVEL 1 */
main( argc, argv ) char *argv[]; 
{
	register NODE ** fsp;  /* points to next free position on the stack */
	register int x;
	register NODE *p;
	register HASHU *sp;		/* ptr to symtab[symloc] */
	HASHU *lsp;			/* Not in a register */
	int arrayoffset;
	int structoffset;
	int a;
	int xbuf[4];	/* for writing/rewriting tmpoff to outfile */
			/* x[0] for FLBRAC op */
			/* x[1] for max temp float registers */
			/* x[2] for original tempoff */
			/* x[3] for updated tempoff */
	LBLOCK *lp;
	LCRACKER lx;
	flag in_vfe_ref;	/* in tree for call to vfe "thunk" */
	NODE *vfe_lastnode;
	HIDDENVARS *hiddenvars;
	SYMTABATTR attr;
	CGU *xllp;

# ifdef DEBUGGING
	initial_sbrk = sbrk(0);		/* for later determining heap use */
# endif DEBUGGING

# ifdef fast_malloc
	mallopt(M_MXFAST,0); 		/* tuning for fast malloc calls */
	mallopt(M_NLBLKS,2);
# endif fast_malloc

	if (x = flags_and_files(argc, argv))
		return(x);	/* x here is number of errors */

	if ((pass_flag == YES)
	 && (last_optz_procname == -1))
		{
		copy_in_to_out();
		return(0);
		}
	inittaz();
	file_init_symtab();		/* misc.c -- init symtab storage */
	file_init_storage();		/* regweb.c -- init's reg alloc stg */
	tinit();			/*misc.c Initializes expression trees */
	prmaxlabblank();
	fsp = fstack;
	structflag = NO;
	arrayflag = NO;
	arrayelement = NO;
	in_vfe_ref = NO;
	in_procedure = NO;
	c0tempflag = NO;
	hiddenvars = NULL;
	maxstructnode = 10;
	structnode = (NODE **) ckalloc(maxstructnode * sizeof (NODE *));
	laststructnode = -1;

	for(;;){
		/* read nodes, and go to work... */
		x = lread();

#ifdef DEBUGGING
	if( xdebug ) fprintf( debugp, "\nop is %s, val = %d., rest = 0x%x\n",
			xfop(FOP(x)), VAL(x), (int)REST(x) );
#endif	DEBUGGING
		switch( FOP(x) ){  /* switch on opcode */

		case 0:
#ifdef DEBUGGING
			fprintf( debugp, "null opcode ignored\n" );
#else
			fprintf( stderr, "null opcode ignored\n" );
#endif DEBUGGING
			break;

		case PAREN:
			if (paren_enable) 
			  goto def;
			else
			  break; /* ignore (strip out) PAREN nodes */

		case C0TEMPASG:
			c0tempflag = YES;
			break;

		case ARRAYREF:
			arrayflag = YES;
			arrayelement = VAL(x);	/* YES iff the reference
						   is to a specific element
						*/
			arrayoffset = lread();
#ifdef DEBUGGING
			if (xdebug)
				fprintf(debugp, "\tarrayoffset = 0x%x, element = %d\n",
					arrayoffset, arrayelement);
#endif DEBUGGING
			break;

		case STRUCTREF:
			structflag = YES;
			isstructprimary = VAL(x);
			if (isstructprimary)
			    structoffset = lread();
			else
			    structoffset = 0;
#ifdef DEBUGGING
			if (xdebug)
			    fprintf(debugp,
					"\tstructoffset = 0x%x, primary = %d\n",
					structoffset, isstructprimary);
#endif DEBUGGING
			break;

		case FLD:
		case STARG:
			p = basic_node(x);
			p->sin.stalign = VAL(x);
			p->sin.stsize = lread();	
#ifdef DEBUGGING
			if (xdebug)
			    fprintf(debugp, "\tstsize = 0x%x\n", p->sin.stsize);
#endif DEBUGGING
			p->sin.left = *--fsp;
			if ((FOP(x) == FLD) && structflag)
			    {
			    NODE *t;
			    NODE *pp;

			    if (laststructnode < 0)
#pragma BBA_IGNORE
				cerror("bad structnode stack -- FLD");
			    else
			        t = structnode[laststructnode--];

			    pp = p->sin.left;
			    switch (pp->in.op)
				{
				case UNARY MUL:
				case PLUS:
					computed_structref(pp, t, structoffset);
					p->sin.left = pp;
					if (pp->in.op == OREG)
					    goto fld_oreg;
					else if (pp->in.op == NAME)
					    goto fld_name;
					break;			
					
				case FOREG:
				case OREG:
fld_oreg:
				        pp->tn.structref = YES;
					process_oreg_foreg(pp, 0, structoffset);
					break;

				case ICON:
				case NAME:
fld_name:
				        pp->tn.structref = YES;
					process_name_icon(pp, 0, structoffset);
					break;
				}
			    }
			structflag = NO;
			goto bump;

		case STASG:
			p = basic_node(x);
			p->sin.stalign = VAL(x);
			p->sin.stsize = lread();	
			p->sin.right = *--fsp;
			p->sin.left = *--fsp;
			structflag = NO;
			if (p->sin.left->in.op == OREG) {
			    long symloc;
			    flag flagsave = f_do_not_insert_symbol;
			    f_do_not_insert_symbol = YES;
			    symloc = find(p->sin.left);
			    f_do_not_insert_symbol = flagsave;
			    if (symloc != -1)
			        symtab[symloc]->a6n.equiv = YES;
			}
			goto bump;

		case C1HIDDENVARS:
			hiddenvars = (HIDDENVARS *)
			 	      ckalloc(sizeof(ushort) * (REST(x) + 1));
			hiddenvars->nitems = REST(x);
			do_hiddenvars(hiddenvars);
			break;

		case FENTRY:
			/* Alternate entry point code */
			if (++lastep >= maxepsz)
				{
				maxepsz += EPSZ;
				ep = (EPB *)ckrealloc(ep, maxepsz*sizeof(EPB));
				}
			/* ep[lastep] will be filled in insert_dummy_block
			*/
			lstrread();
			strcpy(ep[lastep].b.name, lcrbuf);
			need_ep = YES;
			insert_dummy_block(); 
			break;

		case FICONLAB:
			/* The following ICON is a label in disguise */
			lx.x = x;
			lp = enterlblock(lx.s.l);
			lp->preferred = YES;

			/* Now fill asgtp in case ASSIGNED GOTO stmts occur */
			if (ficonlabsz >= ficonlabmax)
				{
				ficonlabmax += FICONSZ;
				asgtp = (CGU *)ckrealloc(asgtp,
							ficonlabmax*sizeof(CGU));
				}
			asgtp[ficonlabsz].val = lx.s.l;
			asgtp[ficonlabsz].nonexec = YES;/* initialization */
			ficonlabsz++;
			break;

		case FCOMPGOTO:
		case SWTCH:
			/* FCOMPGOTO needs one more label spot for "fall-thru"
			   default label.  SWTCH doesn't need it.  Use
			   "fortran" flag to distinguish between the two.
			*/
			nlabs = REST(x);
			if (llp)
				{
				xllp = llp;
				while (xllp->next)
					xllp = xllp->next;
				xllp = xllp->next = (CGU *) ckalloc( (fortran?
					(nlabs + 1) : nlabs)*sizeof(CGU));
				}
			else
				llp = xllp = (CGU *) ckalloc( (fortran?
					(nlabs + 1) : nlabs)*sizeof(CGU));
			p = talloc();
			lastop = FCOMPGOTO;
			p->cbn.op = FCOMPGOTO;
			p->cbn.ll = xllp;
			for (a = 0; a < nlabs; a++)
				{
				if (! fortran)  /* SWTCH */
					xllp[a].caseval = lread();
				xllp[a].val = lread();
				xllp[a].lp = enterlblock(xllp[a].val);
				xllp[a].lp->preferred = YES;
				}
			xllp->next = (CGU *) NULL;
			if (fortran)
				nlabs++;
			else	/* SWTCH */
				p->cbn.switch_flags = VAL(x);
			xllp->nlabs = nlabs;
			p->cbn.nlabs = nlabs;
			*fsp++ = p;
			if( fsp >= &fstack[NSTACKSZ] )
#pragma BBA_IGNORE
				uerror( "expression depth exceeded" );
			postponed_nb = YES;
			goto fexpr;	/* pass1 forgot to put it out */

		case NAME:			/* 2 from manifest */
			p = basic_node(x);
			if( VAL(x) ) 
				{
				a = lread();
#ifdef DEBUGGING
				if (xdebug) fprintf(debugp, "\tp->tn.lval = 0x%x",
					a);
#endif	DEBUGGING
				}
			else a = 0;
			p->tn.rval = 0;
			lstrread();	/* fills lcrbuf */
			p->atn.name = addtreeasciz(lcrbuf);
			p->tn.lval = a;
			process_name_icon(p, arrayoffset, structoffset);
			goto bump;

		case ICON:			/* 4 from manifest */
			p = basic_node(x);
			p->tn.rval = 0;
			a = lread();
#ifdef DEBUGGING
			if (xdebug) fprintf(debugp, "\tp->tn.lval = 0x%x\n",a);
#endif	DEBUGGING
			if( VAL(x) )
				{
				lstrread( );
				p->atn.name = addtreeasciz(lcrbuf);
				p->tn.lval = a;
				process_name_icon(p, arrayoffset, structoffset);
				}
			else
				{
				p->tn.lval = a;
				p->atn.name = 0;
				}

			structflag = NO;
			arrayflag = NO;
			arrayelement = NO;
			goto bump;

		case GOTO:			/* 0x25 = 37. */
			postponed_nb = YES;
			if( VAL(x) )
				{
			  	/* unconditional branch */
				p = talloc();
				*fsp++ = p;
				if( fsp >= &fstack[NSTACKSZ] )
#pragma BBA_IGNORE
					uerror( "expression depth exceeded" );
				lastop = LGOTO;
				p->in.op = LGOTO;
				p->bn.label = lread();
				}
			else
				{
				/* ASSIGNED GOTO */
				assigned_goto_seen = YES;
				lastop = GOTO;
				goto def;
				}
			break;

		case CBRANCH:
			lastop = CBRANCH;
			postponed_nb = YES;
			x &= 0xffff00ff;	/* zero out VAL field */
			goto def;

		case REG:			/* 0x5E = 94. from manifest */
			p = basic_node(x);
			p->tn.rval = VAL(x);
			p->tn.lval = 0;
			p->atn.name = 0;
			if (structflag)
			    {
			    if (isstructprimary)
#pragma BBA_IGNORE
				cerror("structprimary on REG");
			    else
				{
				/* push secondary STRUCTREF onto the structref
				   stack
				 */
				laststructnode++;
				if (laststructnode >= maxstructnode)
				    {
				    maxstructnode <<= 2;
				    structnode = (NODE **) ckrealloc(structnode,
						maxstructnode * sizeof(NODE *));
				    }
				structnode[laststructnode] = p;
				structflag = NO;
				}
			    }
			goto bump;

		case UNARY MUL:
			p = basic_node(x);
			p->in.left = *--fsp;
			if (arrayflag)
				{
				computed_aref_unary_mul(p, arrayoffset);
				arrayflag = NO;
				arrayelement = NO;
				}
			else
			    {
			    if (structflag)
				{
				NODE *t;
				if (laststructnode < 0)
#pragma BBA_IGNORE
				    cerror("bad structnode stack -- UNARY MUL");
				else
				    t = structnode[laststructnode--];

				/* insert the base into the symtab */
				computed_structref(p, t, structoffset);

				if (p->in.op == OREG)
				    process_oreg_foreg(p, 0, structoffset);
				else if (p->in.op == NAME)
				    process_name_icon(p, 0, structoffset);

				structflag = NO;
				}
			    else if (! fortran)	/* arbitrary pointer indir */
				{
				p->in.isptrindir = YES;
				computed_ptrindir(p);
				}
# ifdef FTN_POINTERS
			    else /* fortran */
				{
				if ((p->in.left->in.op != OREG) ||
				    !(symtab[find(p->in.left)]->a6n.farg))
				  {
				  p->in.isptrindir = YES;
				  computed_ptrindir(p);
				  }
				}
# endif FTN_POINTERS
			    }
			if (optype( p->in.op ) == LTYPE)
			  goto bump;
                        else
			  goto unary;

		case OREG:			/* 0x5F = 95. from manifest */
			p = basic_node(x);
			p->tn.rval = VAL(x);
			p->tn.lval = lread();
			p->atn.name = NULL;
#ifdef DEBUGGING
			if (xdebug)
				{
				fprintf(debugp, "\tp->tn.lval = 0x%x", p->tn.lval);
				fprintf(debugp, "\tp->atn.name = %s\n", p->atn.name);
				}
#endif	DEBUGGING
			process_oreg_foreg(p, arrayoffset, structoffset);
			goto bump;

		case CALL:
		case UNARY CALL:
			p = basic_node(x);
			p->fn.side_effects = VAL(x);
                        if (!fortran)
			  p->fn.side_effects = 0;
			ncalls++;
			if (p->in.op == CALL)
			    {
			    p->in.right = *--fsp;
			    p->in.safe_call = (fortran && SAFEVAL(x));
			    }
			p->in.left = *--fsp;
			p->in.hiddenvars = hiddenvars;
			hiddenvars = NULL;
			if (p->in.left->in.op == ICON)
			    if (locate(p->in.left, &lsp))
				{
			    	lsp->an.func = YES;
			    	if (!fortran)
			        	{
			        	if (lsp->an.no_effects)
			            	p->fn.side_effects = NO_DEFS;
			        	}
				}
			    else p->fn.side_effects = YES; /* Assume the worst */
			if (fortran)
			    {
			    if (NO_SIDE_EFFECTS)
			        p->fn.side_effects |= NO_DEFS;
			    if (p->in.op == CALL)
				propagate_flags(p);
			    }
			p->in.parm_types_matched = (PARM_TYPES_MATCHED != 0);
			goto bump;

		case STCALL:
		case UNARY STCALL:
			p = basic_node(x);
			p->sin.stalign = VAL(x);
			p->sin.stsize = lread();	
			ncalls++;
			if (p->in.op == STCALL)
			    {
			    p->in.right = *--fsp;
			    }
			p->in.left = *--fsp;
			p->in.hiddenvars = hiddenvars;
			hiddenvars = NULL;
			if (p->in.left->in.op == ICON)
			    if (locate(p->in.left, &lsp))
				{
			    	lsp->an.func = YES;
			    	if (!fortran)
			        	{
			        	if (lsp->an.no_effects)
			            	p->fn.side_effects = NO_DEFS;
			        	}
				}
			    else p->fn.side_effects = YES; /* Assume the worst */
			if (fortran)
			    {
			    if (NO_SIDE_EFFECTS)
			        p->fn.side_effects |= NO_DEFS;
			    if (p->in.op == STCALL)
				propagate_flags(p);
			    }
			p->in.parm_types_matched = (PARM_TYPES_MATCHED != 0);
			goto bump;


		case FTEXT:			/* 0xC8 = 200. */
			read_comment( x );	/* buffer all comments */
			break;

		case FEXPR:			/* 0xC9 = 201. */
			lineno = REST(x);
			{
			int size = VAL(x);
			if( size ) {
				lcread( size );
				strcpy(filename, lcrbuf);
				}
			if( fsp == fstack ) /* filename only */
				break;
			}
fexpr:
			if( --fsp != fstack )
				{
#pragma BBA_IGNORE
#ifdef DEBUGGING
				if (xdebug)
				fprintf(debugp, "\tafter processing fstack-fsp= %d.\n",
					fstack-fsp);
#endif	DEBUGGING
				cerror( "expression poorly formed" );
				}

			if ( laststructnode != -1 )
				{
#pragma BBA_IGNORE
				cerror( "non-empty structnode stack" );
				}

			p = fstack[0];
#ifdef DEBUGGING
			if( edebug ) fwalk( p, eprint, 0 );
#endif	DEBUGGING

			if (c0tempflag)
			    {
			    sp = symtab[find(p->in.left)];
			    if (p->in.right->in.arrayref
			     || p->in.right->in.structref)
				sp->a6n.wholearrayno =
					p->in.right->in.arrayrefno;
			    else
				sp->a6n.wholearrayno = NO_ARRAY;
			    c0tempflag = NO;
			    }

	/* put p in canonical form */
# 	ifdef DEBUGGING
			if (xdebug>1) {
				fprintf(debugp, "canon entry: 0x%x\n", p );
				fwalk( p, eprint, 0 );
				}
# 	endif	/* DEBUGGING */
			/* look for and create OREG nodes (plus some more) */
			oreg2plus(p, YES, NO);

# 	ifdef DEBUGGING
			if (xdebug>2) {
				fprintf(debugp, "after canon: 0x%x\n", p );
				fwalk( p, eprint, 0 );
				}
# 	endif	/* DEBUGGING */

			nexprs++;
			addexpression(p, lineno);
			if (in_vfe_ref)		/* reference to vfe ? */
			    {
			    VFEREF *vp;

			    in_vfe_ref = NO;
			    vp = (VFEREF *) ckalloc(sizeof(VFEREF));
			    vp->np = p;
			    vp->bp = lastblock - topblock;

			    if ((p->in.op == UNARY CALL) &&
				    (p->in.left->in.op == ICON))
				{	/*not anonymous call */
				x = find_or_add_vfe(p->in.left->tn.name);
				vp->next = vfe_thunks[x].refs;
				vfe_thunks[x].refs = vp;
				}
			    else	/* anonymous call */
				{
				vp->next = vfe_anon_refs;
				vfe_anon_refs = vp;
				}
			    }
# ifdef FTN_POINTERS
			if (fortran)
			  find_fortran_pointer_targets(p);
# endif FTN_POINTERS

			p = (NODE *) 0;
			breakop = lastop;
			lastop = FREE;
			break;

		case FLBRAC:			/* 0xCB = 203. */
			/* output all FTEXTs that came before the FLBRAC node */
			write_comments(comment_head); 
			comment_head = NULL;
			
			p = 0;
			in_procedure = YES;
			disable_procedure = NO;
			xbuf[0] = x;
			xbuf[1] = lread();
			xbuf[2] = lread();
			lread();		/* spacer only */
			baseoff = tmpoff = xbuf[2] >> 3; /* in bytes */
#ifdef DEBUGGING
			if (xdebug) fprintf(debugp, "\ttmpoff = baseoff = 0x%x\n",tmpoff);
#endif	DEBUGGING
			if( ftnno != REST(x) ){
				/* beginning of function */
				ftnno = REST(x);
				funcinit();
				}
			else {
				if( baseoff > maxoff ) maxoff = baseoff;
				/* maxoff at end of ftn is max of autos and temps 
				   over all blocks in the function */
				cerror("don't know how to handle inner block");
				}

			lstrread();
			procname = addtreeasciz(lcrbuf);
# ifdef DEBUGGING
			if (emitname)
				fprintf(debugp, "C1 in %s():\n", procname);
# endif DEBUGGING

			xbuf[3] = xbuf[2];
			/* asm_flag is used:
			 *	- by C to indicate assemble stmts in source
			 *      - by FORTRAN to indicate the occurance of %loc
			 * No optimizations are performed when this flag is on.
			 */
			asm_flag = ((x) >> 15) & 1;

			if ((pass_flag && !is_in_namelist(procname,
					    optz_procnames, last_optz_procname))
			 || is_in_namelist(procname, pass_procnames,
					  last_pass_procname)
			 || asm_flag)     /* C "asm" stmts */
				{
				p2word(xbuf[0]);
				p2word(xbuf[1]);
				p2word(xbuf[2]);
				p2word(xbuf[3]);
				p2name(procname);
				pass_procedure();
#ifdef COUNTING
				_n_procs_passed++;
#endif
				}
			break;

		case FRBRAC:			/* 0xCC = 204. */
			{
#ifdef COUNTING
				_n_procs_optzd++;
#endif

			reading_input = NO;
			/* insert symbol table entries for array elements */
			pre_add_arrayelements();

			SETOFF( maxoff, ALSTACK );
			/* cleanup last block */
			addexpression(p? 0 : (p->in.op==FREE? 0 : p), 0);
			/* assume: last block always falls thru to epilog */
			lastblock->l.breakop = EXITBRANCH;

			/* check for additional equivalence relationships */
			if (fortran)
			  check_com_entries();

			/* complete the asgtp table for ASSIGNED GOTOs */
			complete_asgtp_table();

			/* inline any vfe's */
			process_vfes();

			/* complete the pointer target table -- add all
			 * non-constant static items.
			 */
			complete_ptrtab();

# ifdef DEBUGGING
			/* dump the symbol table, comtab, and fargtab */
			if (sdebug)
				dumpsymtab();
# endif DEBUGGING

			/* clean up the LBLOCK-BBLOCK tables */
			clean_flowgraph();

			if (!disable_procedure)
			  {

			  if (fortran && lastep==0 && lastfarg >= 0)
				rm_farg_aliases();

			  check_tail_recursion();

			  setup_global_data();

			  if (reducible &&
			    !(deadstore_disable && copyprop_disable
			      && constprop_disable
			      && disable_unreached_loop_deletion
			      && disable_empty_loop_deletion && dbra_disable))
				{
				find_doms();
				global_def_use_opts(1);
				}

			  if (reducible && !coalesce_disable)
			        coalesce_blocks();

			  if (!dagdisable)
				/*optimize trees for block-wide optimizations */
				local_optimizations();

			  if (reducible && !global_disable && gosf())
				/* do global optimizations */
				global_optimize();

# ifdef DEBUGGING
			  if (!comops_disable)
# endif DEBUGGING
				rewrite_comops();
# ifdef DEBUGGING
			  if (bdebug)
				{
				fprintf(debugp,"after rewrite_comops():\n");
				dumpblockpool(1, 1);
				}
# endif DEBUGGING

			  if (reducible)
			    {
			    if (!dom)
				find_doms();

			    if (!global_disable
			      && !(deadstore_disable && copyprop_disable
			      && constprop_disable
			      && disable_unreached_loop_deletion
			      && disable_empty_loop_deletion && dbra_disable))
			        global_def_use_opts(2);		/* duopts.c */

			    if ( ! reg_disable )
				/* do register allocation */
				{
				register_allocation();

# 	ifdef DEBUGGING
				if (bdebug)
					{
					fprintf(debugp,"after register_allocate():\n");
					dumpblockpool(1, 1);
					}
# 	endif DEBUGGING
				}
			    free_global_data();
			    }
			  }

			/* write out tmpoff for pass 2 */
			xbuf[0] |= (7 << 8);	  /* all D regs avail */
			xbuf[1] = (15 << 16) | 7; /* all FPA & 881 regs avail */
			/* The bit corresponding to the value 256 is used
			 * to indicate that +ffpa was specified but there
			 * are no FPA operations in this routine.
			 */
			if (fpaflag && !saw_dragon_access)
			  xbuf[1] += 256;
			/* The bit corresponding to the value 512 is used
			 * to indicate that PIC was specified but there
			 * are no global variable accesses in this routine.
			 */
			if (pic_flag && !saw_global_access) 
			  xbuf[1] += 512;
			xbuf[3] = tmpoff << 3;
			p2word(xbuf[0]);
			p2word(xbuf[1]);
			p2word(xbuf[2]);
			p2word(xbuf[3]);
			p2name(procname);	/* procedure name */

			/* output all FTEXTs between FLBRAC and FRBRAC nodes */
			write_comments(comment_head); 
			comment_head = NULL;
			
			/* new use for xbuf follows */
			if (VAL(x))
				{
				/* structured valued function needs more info */
				xbuf[0] = lread();
				xbuf[1] = lread();
				}
			/* then write it out */
			p2pass(x, xbuf);	/* put it all out to outfile */

			/* now free it */
			bfree();
			if ( !disable_procedure && !reg_disable && reducible)
			    final_reg_cleanup();
# ifdef DEBUGGING
			tcheck();
# endif	DEBUGGING

			FREEIT(asgtp);
			in_procedure = NO;

			proc_init_symtab();	/* remove local items from
						 * symtab */

			reading_input = YES;

			break;
			}

		case FEOF:			/* 0xCD = 205. */
			/* output all FTEXTs since the last FRBRAC node */
			write_comments(comment_head); 
# ifdef DEBUGGING
			tcheck();
# endif	DEBUGGING
			a = x;
			p2flush();
			if (fwrite(&a, sizeof(a), 1, outfile ) != 1)
				goto write_error;
			prmaxlab(pseudolbl+1);
# ifdef COUNTING
			if (print_counts)
				write_counts();
# endif COUNTING
			return( nerrors );

		case LABEL:			/* 0xCF = 207. */
			{
			int ii;

			lx.x = x;
			ii = lx.s.l;
			pushlbl( ii );
			enterlblock(ii);
			need_block = YES;
			break;
			}

		case FMAXLAB:		/* 211 */
			pseudolbl = lread() + 1;
			break;

		case C1SYMTAB:		/* 216 */
			if (VAL(x) == 1)
			    {
			    farg_high = lread();
			    farg_low = lread();
			    farg_diff = lread() + (farg_high - 3);
			    }
			break;

		case C1OREG:
			/* A C1SYMTAB entry describing an OREG ... not 
			   a tree node.
			*/
			{
			char *name;
			int arrayloc;
			int arraysize;

			p = basic_node(x);
			p->tn.op = OREG;
			p->tn.type.base = REST(x);

			p->tn.rval = VAL(x);
			p->tn.lval = lread();
			attr.l = lread();
			if (attr.a.array)
			  arraysize = lread();
			else
			  arraysize = -1;
#ifdef DEBUGGING
			if (xdebug)
			    {
			    fprintf(debugp, "\toffset = 0x%x\n",p->tn.lval);
			    fprintf(debugp, "\tattributes = 0x%x\n",attr.l);
			    }
#endif
			lstrread();	 /* pass1 name */

			if (lcrbuf[0])	/* name != null */
			   name = addtreeasciz(lcrbuf);
			else
			   name = (char *) 0;

			disable_look_harder = YES;
			symtab_insert(p,attr.l,name,in_procedure,arraysize);
			disable_look_harder = NO;
			p->in.op = FREE;	/* return to free pool */
			}
			break;

		case C1NAME:
			/* A C1SYMTAB entry describing a NAME ... not 
			   a tree node.
			*/
			{
			char *name;
			int arrayloc;
			int arraysize;

			p = basic_node(x);
			p->tn.op = NAME;
			p->tn.type.base = REST(x);
			p->tn.rval = 0;
			if (VAL(x))
			    p->tn.lval = lread();
			else
			    p->tn.lval = 0;
			lstrread();
			attr.l = lread();
			if (attr.a.array)
			  arraysize = lread();
			else
			  arraysize = -1;
#ifdef DEBUGGING
			if (xdebug)
			    {
			    fprintf(debugp, "\toffset = 0x%x\n",p->tn.lval);
			    fprintf(debugp, "\tattributes = 0x%x\n",attr.l);
			    }
#endif
			p->atn.name = (in_procedure && !attr.a.isexternal) ?
				addtreeasciz(lcrbuf) : addexternasciz(lcrbuf);
			lstrread();

			if (lcrbuf[0])	/* name != null */
			    name = (in_procedure && !attr.a.isexternal) ?
				addtreeasciz(lcrbuf) : addexternasciz(lcrbuf);
			else
			    name = (char *) 0;

			disable_look_harder = YES;
			symtab_insert(p,attr.l,name,in_procedure,arraysize);
			disable_look_harder = NO;

			p->in.op = FREE;	/* return to free pool */
			}
			break;

		case C1OPTIONS:
			/* set optimization level and set of assumptions */
			olevel = VAL(x);
			assumptions = REST(x);
			if (olevel < 2)
				{
				/* mimic FLBRAC for this procedure */
				p2word(xbuf[0]);
				p2word(xbuf[1]);
				p2word(xbuf[2]);
				p2word(xbuf[3]);
				p2name(procname);
				pass_procedure();	/* reads rest of proc */
#ifdef COUNTING
				_n_procs_passed++;
#endif
				}
			break;

		case NOEFFECTS:		/* #pragma NOEFFECTS  functionname */
			if (in_procedure)
#pragma BBA_IGNORE
				cerror("NOEFFECTS within procedure");
			p = talloc();
			p->tn.op = ICON;
			p->tn.type.base = INT | (FTN << TSHIFT) | PTR;
			lstrread( );
			p->atn.name = addexternasciz(lcrbuf);
			allow_insertion = YES;
			sp = symtab[find(p)];
			allow_insertion = NO;
			if ((BTYPE(sp->an.type) != UNDEF) && !sp->an.func)
#pragma BBA_IGNORE
				cerror("NOEFFECTS name not a procedure");
			sp->an.type = p->tn.type;
			sp->an.func = YES;
			sp->an.no_effects = YES;
			p->tn.op = FREE;
			break;

		case VAREXPRFMTDEF:
			/* read and store the "thunk" name */
			vfe_lastnode = lastnode;	/* save GOTO ptr */
			x = lread();
			if (FOP(x) != FTEXT)
#pragma BBA_IGNORE
				cerror("no FTEXT after VAREXPRFMTDEF");
			lcread(VAL(x));
			lcrbuf[VAL(x) * 4] = '\0';	/*in case all words filled*/
			a = strlen(lcrbuf);
			lcrbuf[a-2] = '\0';  /* erase the ':\n' */
			a = find_or_add_vfe(lcrbuf);
			vfe_thunks[a].startblock = lastblock;
			vfe_thunks[a].startlabel = lastlbl;
			break;


		case VAREXPRFMTEND:
			if (VAL(x))	/* empty FORMAT */
			    {
			    vfe_thunks[curr_vfe_thunk].thunk = (VFECODE*) NULL;
			    }
			else
			    {
			    register struct vfethunk *vp;
			    BBLOCK *vbp;
			    LBLOCK *vlp;
			    VFECODE **vcplast;

			    vp = &(vfe_thunks[curr_vfe_thunk]);
			    vp->thunkvars = (ushort *) NULL;
			    vp->asg_fmt_target = NO;

			    if ((vp->nblocks = lastblock - vp->startblock) > 1)
				uerror("Optimizer does not support complex Variable Format Expressions.");
			    if ((lastlbl - vp->startlabel) != vp->nblocks)
#pragma BBA_IGNORE
				cerror("vfe label/block mismatch in main");

			    vbp = vp->startblock;
			    vlp = vp->startlabel;
			    vp->thunk = NULL;
			    vcplast = &(vp->thunk);
			    while (vbp != lastblock)
				{
				VFECODE *vcp;
				vbp = vbp + 1;
				vlp = vlp + 1;
				vcp = (VFECODE *) ckalloc(sizeof(VFECODE));
				*vcplast = vcp;
				vcp->np = vbp->l.treep;
				vcp->labval = vlp->val;
				vcp->next = NULL;
				vcplast = &(vcp->next);
				}
			    non_empty_vfe_seen = YES;
			    /* now make it look like the last block was the
				GOTO to the next statement (i.e., pretend that
				that the "thunk" was never seen */
			    lastblock = vp->startblock;
			    lastlbl = vp->startlabel;
			    lastop = FREE;
			    breakop = LGOTO;
			    lastnode = vfe_lastnode;  /* GOTO */
			    postponed_nb = NO;
			    need_block = YES;
			    }
			break;


		case VAREXPRFMTREF:
			in_vfe_ref = YES;	/* we'll handle it in FEXPR */
			break;

		case QUEST:
#pragma BBA_IGNORE
		case COLON:
#pragma BBA_IGNORE
		case ANDAND:
#pragma BBA_IGNORE
		case OROR:
#pragma BBA_IGNORE
			cerror("lazy evaluation op seen in input stream");
			break;

		case SETREGS:			/* 0xD0 = 208 */
                        {
                        register int y;
                        /* Only allow SETREGS with NO registers specified */

                        y = lread();
                        if ( (x != SETREGS) || (y != 0 )) 
			  /* throw it away */
#pragma BBA_IGNORE
 		          cerror("SETREGS op seen in input stream");
		        break;
                        }
		default:
def:
			p = basic_node(x);

			switch( optype( p->in.op ) )
			{
			case BITYPE:			/* 0x8 in manifest */
				p->in.right = *--fsp;
				p->in.left = *--fsp;

				if (asgop(p->in.op) && 
				    p->in.left->in.op == PAREN)
          			  {
	  			  p->in.left->in.op = FREE;
	  			  p->in.left = p->in.left->in.left;
          			  }

				/* mark lhs array refs for dag use later */
				if ( asgop(p->in.op) 
					&& (p->in.left->allo.flagfiller &
						(AREF|SREF|ISPTRINDIR)) )
				    p->in.left->in.lhsiref = YES;
				else if ((p->in.op == PLUS)
				      || (p->in.op == MINUS))
				    {
				    if ((p->in.left->in.op == REG) &&
					(p->in.left->tn.rval == A6) &&
					(p->in.right->in.op == ICON) &&
					(p->in.right->atn.name == NULL))
					{
					NODE *rp = p->in.right;

					p->in.left->in.op = FREE;
					p->tn.rval = A6;
					p->tn.lval = (p->in.op == PLUS) ?
						      rp->tn.lval :
						      - rp->tn.lval;
					rp->in.op = FREE;
					p->in.op = FOREG;
					process_oreg_foreg(p, arrayoffset,
								structoffset);

					if (! fortran)
					    {
					    if (p->tn.arrayref)
					        add_pointer_target(p->tn.arrayrefno);
					    else if (p->tn.structref)
						{
					        if (isstructprimary)
					            add_pointer_target(p->tn.arrayrefno);
						}
					    else
						{
						int loc_p;
						loc_p = find(p);
						if (loc_p != (unsigned) (-1))
					          add_pointer_target(loc_p);
						}
					    }
					}
				    else
					{
				        if (arrayflag)
					    {
					    computed_aref_plus(p, arrayoffset);
					    if (! fortran)
					        add_pointer_target(p->tn.arrayrefno);
					    }
				        else if (structflag)
					    {

					    if (isstructprimary)
					        {
					        NODE *t;

					        if (laststructnode < 0)
#pragma BBA_IGNORE
						cerror("bad structnode stack -- PLUS");
					        t = structnode[laststructnode--];
						computed_structref(p, t, structoffset);
					        if (p->tn.structref && !fortran)
					            add_pointer_target(p->tn.arrayrefno);
					        }

					    else
					        {
					        p->tn.structref = YES;
					        laststructnode++;
					        if (laststructnode >= maxstructnode)
						    {
						    maxstructnode <<= 2;
						    structnode =
						    (NODE **) ckrealloc(structnode,
						    maxstructnode * sizeof(NODE *));
						    }
					        structnode[laststructnode] = p;
					        }
					    }
					}
				    }
				arrayflag = NO;
				arrayelement = NO;
				structflag = NO;
				goto bump;

			case UTYPE:			/* 0x4 in manifest */
				p->in.left = *--fsp;
unary:
				p->tn.rval = 0;
				arrayflag = NO;
				arrayelement = NO;
				structflag = NO;
				goto bump;

			case LTYPE:			/* 0x2 in manifest */
#pragma BBA_IGNORE
				cerror( "illegal leaf node: %d", p->in.op );
				exit( 1 );
				}
bump:
			*fsp++ = p;
			if( fsp >= &fstack[NSTACKSZ] )
#pragma BBA_IGNORE
				uerror( "expression depth exceeded" );
				}	/* switch on opcode */

#ifdef DEBUGGING
			if (xdebug)
				fprintf(debugp, "\tafter processing fstack-fsp= %d.\n",
					fstack-fsp);
#endif	DEBUGGING
			}
		/* cannot fall thru */
write_error:
		cerror("output file error in main()");
}
# pragma OPT_LEVEL 2


/******************************************************************************
*
*	basic_node()
*
*	Description:		sets the p->in.type and op according to the
*				expanded definition of type.
*
*	Called by:		main()
*
*	Input parameters:	x - the op+rest+val packed int
*
*	Output parameters:	return value -- the tree node
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		talloc()
*
*******************************************************************************/
NODE *basic_node(x) register int x;
{
	register NODE *np;

	np = talloc();
	np->tn.op = FOP(x);
	np->tn.type.base = REST(x);
	if ( TMODS1(np->tn.type.base) )
		{
		np->tn.type.mods1 = lread();
		if ( TMODS2(np->tn.type.base) )
			{
			x = lread();
			np->tn.type.mods2 = FMODS2(x);
			}
		}
	return(np);
}

/******************************************************************************
*
*	FIND_FORTRAN_POINTER_TARGETS()
*
*	Description:		Search the tree pointed to by "np" and make
*				appropriate entries in the pointer table.
*				Fortran differs from C in this process 
*				because passing a variable as a parameter
*				is only considered to be taking the address
*				of that variable if the Fortran source
*				directive ASSUME_NO_HIDDEN_POINTER_ALIASING
*				is set to OFF.
*
*	Called by:		main()
*
*	Input parameters:	np - a NODE ptr to an expression tree.
*
*	Output parameters:	none
*
*	Globals referenced:	NO_HIDDEN_POINTER_ALIASING
*
*	Globals modified:	ptrtab[]
*
*	External calls:		add_pointer_target()
*
*******************************************************************************/
LOCAL void find_fortran_pointer_targets(np)	register NODE *np;
{
if ((np->in.op == PLUS) || (np->in.op == MINUS))
  {
  if (np->tn.arrayref)
    add_pointer_target(np->tn.arrayrefno);
  return;
  }
else if (np->in.op == FOREG)
  {
  int loc_p;
  
  if (np->in.arrayref)
     loc_p = np->in.arrayrefno;
  else
     loc_p = find(np);
  if (loc_p != (unsigned) (-1))
    if (symtab[loc_p]->xn.arrayelem)
       add_pointer_target(symtab[loc_p]->xn.wholearrayno);
    else
       add_pointer_target(loc_p);
  return;
  }
else if (((np->in.op == CALL)   || (np->in.op == STCALL)) &&
         NO_HIDDEN_POINTER_ALIASING) 
  /* Don't consider parameter passing as address taking. */
  {
  find_fortran_pointer_targets(np->in.left);  /* routine being called */
  np = np->in.right;
  while (np->in.op == CM)
    {
    if (np->in.right->in.op == COMOP) 
    /* don't search right op as that is an FOREG 
     * that doesn't go in the pointer table.
     */
      find_fortran_pointer_targets(np->in.right->in.left);
    else if (optype(np->in.right->in.op) == BITYPE)
      {
      find_fortran_pointer_targets(np->in.right->in.left);
      find_fortran_pointer_targets(np->in.right->in.right);
      }
    else if (optype(np->in.op) == UTYPE)
      find_fortran_pointer_targets(np->in.right->in.left);
    np = np->in.left;
    }
  if (np->in.op == UCM)
    np = np->in.left;
  /* Fall through to handle last (or only) parameter. */
  }
if (np == NULL) 
  return;
if (optype(np->in.op) == BITYPE)
  {
  find_fortran_pointer_targets(np->in.left);
  find_fortran_pointer_targets(np->in.right);
  }
else if (optype(np->in.op) == UTYPE)
  find_fortran_pointer_targets(np->in.left);
}
/******************************************************************************
*
*	propagate_flags()
*
*	Description:		Propagates the no_arg_defs flags downwards from
*				the CALL node to the CM or UCM nodes underneath.
*
*	Called by:		main()
*
*	Input parameters:	np - a NODE ptr to a callop node.
*
*	Output parameters:	none
*
*	Globals referenced:	fortran
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void propagate_flags(np)	register NODE *np;
{
	if (np->in.no_arg_defs)
		{
		np = np->in.right;
		while (np->in.op == CM || np->in.op == UCM)
			{
			np->in.no_arg_defs = YES;
			np = np->in.left;
			}
		}
}

/*****************************************************************************
 *
 *  TYPES_DIFFER()
 *
 *  Description:	Compare two type descriptor words for equality.
 *
 *  Called by:		process_name_icon()
 *			process_oreg_foreg()
 *
 *  Input Parameters:	pt1 -- pointer to first type word
 *			pt2 -- pointer to second type word
 *
 *  Output Parameters:	return value -- 0 == types are same
 *					1 == types are different
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

LOCAL long types_differ(pt1, pt2)
TTYPE *pt1;
TTYPE *pt2;
{
    register long mod;
    register long mod1count;
    register long mod2count;

    /* count modifiers */
    mod = (pt1->base & 0xfc0) >> 6;
    if (pt1->base & MODS1MASK)
	mod |= (pt1->mods1 & 0xfffff000) >> 6;
    mod1count = 0;
    while (mod & 0x3)
	{
	++mod1count;
	mod >>= 2;
	}

    mod = (pt2->base & 0xfc0) >> 6;
    if (pt2->base & MODS1MASK)
	mod |= (pt2->mods1 & 0xfffff000) >> 6;
    mod2count = 0;
    while (mod & 0x3)
	{
	++mod2count;
	mod >>= 2;
	}

    if (mod2count != mod1count)
	return(YES);
    else
	return(NO);
}  /* types_differ */

/*****************************************************************************
 *
 *  PROCESS_NAME_ICON()
 *
 *  Description:	Process a NAME or ICON node.  Set flags in node.  Make
 *			necessary symtab entries.
 *
 *  Called by:		computed_aref_unary_mul()
 *			main()
 *			node_constant_collapse()
 *
 *  Input Parameters:	p -- the NAME or ICON node
 *			aoffset -- array base offset, if array ref
 *			soffset -- structure base offset, if structref
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	arrayelement
 *			arrayflag
 *			f_find_array
 *			fortran
 *			isstructprimary
 *			laststructnode
 *			maxstructnode
 *			structflag
 *			structnode[]
 *			symtab[]
 *			tysize[]
 *
 *  Globals Modified:	arrayelement
 *			arrayflag
 *			f_find_array
 *			laststructnode
 *			maxstructnode
 *			structflag
 *			structnode[]
 *
 *  External Calls:	add_arrayelement()
 *			addtreeasciz()
 *			ckrealloc()
 *			decref()
 *			find()
 *			sprintf()
 *			tcopy()
 *			tfree()
 *			types_differ()
 *			werror()
 *			strcpy()
 *			uerror()
 *			strcat()
 *
 *****************************************************************************
 */

void process_name_icon(p, aoffset, soffset) register NODE *p;
{
	register HASHU *sp;
	register HASHU *ap;
	NODE *t;
	HASHU *fp;
	unsigned symloc;
	unsigned symloc2;
	int x;
	long a;
	allow_insertion = YES;
	a = p->tn.lval;		/* save value */

	if (arrayflag)
		{
		p->tn.arrayref = YES;
		p->tn.lval = aoffset;
		/* insert the base into the symtab */
		f_find_array = YES;	/* flag to find() */
		p->tn.arrayrefno = find(p);
		f_find_array = NO;

		if (!fortran)
		    {
		    /* C doesn't tell us whether it's an array element
		     * reference -- so we guess.
		     */
		    if ((a != aoffset) || (p->tn.op == NAME)
		     || (!ISPTR(p->tn.type) && !ISARY(p->tn.type)))
			arrayelement = YES;
		    if (!arrayelement
		     && types_differ(&(symtab[p->tn.arrayrefno]->a6n.type),
				     &(p->tn.type)))
			arrayelement = YES;
		    }
		p->tn.arrayelem = arrayelement;
		}

	if (structflag)
		{
		p->tn.structref = YES;
		if (isstructprimary)
		    {
		    p->tn.lval = soffset;
		    /* insert the base into the symtab */
		    f_find_struct = YES;	/* flag to find() */
		    p->tn.arrayrefno = find(p);
		    f_find_struct = NO;
		    }
		else
		    {
		    /* push secondary STRUCTREF's onto the structref stack */
		    laststructnode++;
		    if (laststructnode >= maxstructnode)
			{
			maxstructnode <<= 2;
			structnode = (NODE **) ckrealloc(structnode,
						maxstructnode * sizeof(NODE *));
			}
		    structnode[laststructnode] = p;
		    }
		}

	p->tn.lval = a;
	allow_insertion = NO;
	if (arrayflag && !arrayelement)
	  sp = symtab[p->tn.arrayrefno];
	else
	  {
	  if (!fortran && arrayelement) /* don't put C arrayelems in sym tab */
	    goto exit;
	  if (structflag)
	    allow_insertion = YES; 
	  symloc = find(p);   /* insert it into symtab */
	  allow_insertion = NO; 
	  if (symloc == (unsigned) (-1))
	    goto exit;
	  sp = symtab[symloc];
  
	  if (arrayelement && (sp->an.wholearrayno == NO_ARRAY))
				  /* new entry in symtab */
	      {
	      char name[300];
	      long elnum;
	      long elsize;
  
	      ap = symtab[p->tn.arrayrefno];
  
	      sp->an.wholearrayno = p->tn.arrayrefno;
	      sp->allo.attributes = ap->allo.attributes;
	      sp->an.isexternal = NO;	/* lives only within proc */
	      sp->an.type = ap->an.type;
	      sp->an.arrayelem = YES;
	      add_arrayelement(p->tn.arrayrefno, symloc);
	      if (ap->an.complex1)  /* complex array */
				    /* is this elem 1st or 2nd */
				    /* half ? */
		  {
	          elsize = (BTYPE(ap->x6n.type) == FLOAT) ? 8 : 16;
	          elnum = (p->tn.lval - aoffset) / elsize;
	          if ((p->tn.lval - aoffset) % elsize)
	              {
	              sp->an.complex1 = NO; 
	              sp->an.complex2 = YES; 
		      sprintf(name, "%s(%d)+%d", ap->an.pass1name, elnum+1,
			  (elsize == 8) ? 4 : 8);
	              sp->an.pass1name = addtreeasciz(name);
		      }
		  else	/* first half -- add second half */
		      {
		      sprintf(name, "%s(%d)", ap->an.pass1name, elnum+1);
	              sp->an.pass1name = addtreeasciz(name);
  
		      t  = tcopy(p);
		      t->tn.lval += (ap->an.type.base == FLOAT) ? 4 :8;
	allow_insertion = YES;
		      symloc2 = find(t);
	allow_insertion = NO;
		      fp = symtab[symloc2];
		      fp->an.wholearrayno = p->tn.arrayrefno;
		      fp->allo.attributes = ap->allo.attributes;
		      fp->an.isexternal = NO;
		      fp->an.type = ap->an.type;
		      fp->an.complex1 = NO; 
		      fp->an.complex2 = YES; 
		      fp->an.arrayelem = YES;
		      sprintf(name, "%s(%d)+%d", ap->an.pass1name, elnum+1,
			  (elsize == 8) ? 4 : 8);
	              fp->an.pass1name = addtreeasciz(name);
		      add_arrayelement(p->tn.arrayrefno, symloc2);
		      tfree(t);
		      sp->a6n.back_half = symloc2;
		      }
		  }
	      else	/* not complex array */
	          {
	          TTYPE tw;
  
	          tw = ap->x6n.type;
	          while (ISARY(tw))
		      tw = decref(tw);
	          if (ISPTR(tw) || ISFTN(tw))
		      elsize = 4;
	          else
	              elsize = tysize[BTYPE(tw)];
	          elnum = (p->tn.lval - aoffset) / elsize;
	          if (fortran)
	              sprintf(name, "%s(%d)", ap->a6n.pass1name, elnum+1);
	          else
	              sprintf(name, "%s[%d]", ap->a6n.pass1name, elnum);
	          sp->a6n.pass1name = addtreeasciz(name);
	          }
	      if ( (p->tn.lval < aoffset) || 
	           ( (ap->allo.array_size >= 0) &&
                     (p->tn.lval >= (aoffset + ap->allo.array_size)) ) )
	        {
		char *s;
		/* allocate enough room for the message to be written */
		s = (char *) ckalloc(strlen(sp->a6n.pass1name)+75);

		strcpy(s,"Element ");
#	if 1
		if (0)	/* C syms are never added to the symbol table anyway */
#	else
		if (!fortran && (p->in.op == ICON)) 
# 	endif 0
		  /* Possibly not a problem, just issue a warning */
		  {
	          if ( (p->tn.lval < aoffset) || 
	               ( (ap->allo.array_size >= 0) &&
                         (p->tn.lval > (aoffset + ap->allo.array_size)) ) )

			 /* The previous test has >=, in this one it is just >.
			  * This allows the address of an array element one
		          * beyond the end of an array to be taken without
			  * any warning being given.
			  */
	            werror(strcat(strcat(s,sp->a6n.pass1name)," outside array bounds.")); 
		  }
		else if (reading_input)
		  {
		  /* May corrupt symbol table, stop optimizing this routine. */
	          werror(strcat(strcat(s,sp->a6n.pass1name)," outside array bounds, procedure not optimized.")); 
	          disable_procedure = YES;
		  }
		else
		  {
		  /* Invalid optimizations may have occured. Abort compile. */
		  int stmtno = find_node_statement(p);
		  if (stmtno == 0) 
	            uerror(strcat(strcat(s,sp->a6n.pass1name)," outside array bounds, cannot continue.")); 
		  else
		    {
		    char stemp[12];

		    sprintf(stemp,"%d",stmtno);
	            uerror(
			strcat(
			  strcat(
			    strcat(
			      strcat(s,sp->a6n.pass1name),
			      " outside array bounds, cannot continue. (Line"),
			    stemp),
			  ")")); 
		    }
		  }
		FREEIT(s);
	        }

	      }

	  if (structflag && isstructprimary && (! sp->an.isstruct)
	   && (sp->an.wholearrayno == NO_STRUCT))
				  /* new entry in symtab */
	      {
	      ap = symtab[p->tn.arrayrefno];
	      sp->an.wholearrayno = p->tn.arrayrefno;
	      sp->allo.attributes = ap->allo.attributes;
	      sp->an.isexternal = NO;
	      sp->an.isstruct = YES;
	      add_structelement(p->tn.arrayrefno, symloc);
	      }

	  }
	if (sp->an.type.base == UNDEF)	/* new entry */
	    {
	    if ((sp->an.ap[0] == 'L')   /* L<digit>... */
	     && ((x = sp->an.ap[1]) >= '0')
	     && (x <= '9'))
		sp->an.isconst = YES;
	    sp->an.type = p->tn.type;
	    sp->an.ptr = ISPTR(p->tn.type);
	    if (sp->an.isconst && sp->an.ptr && (p->in.op == ICON))
		{
	        sp->an.type = decref(sp->an.type);
	        sp->an.ptr = ISPTR(sp->an.type);
		}
	    sp->an.func = ISFTN(p->tn.type);
	    }
	else if (!fortran && !structflag && !arrayflag
	      && !(sp->an.type.base & TMASK)	/* no modifiers */
	      && (BTYPE(sp->an.type) != BTYPE(p->tn.type)))
	    {
	    /* types don't match -- maybe due to some wierd casting.  Better
	     * keep our hands off this one.
	     */
	    sp->an.equiv = YES;
	    }

	if (sp->an.equiv)
	    p->tn.equivref = YES;
	if (sp->an.common)
	    p->tn.common_base = YES;

exit:
	structflag = NO;
	arrayflag = NO;
	arrayelement = NO;
}

/*****************************************************************************
 *
 *  PROCESS_OREG_FOREG()
 *
 *  Description:	Process an OREG or FOREG node.  Set flags in node.
 *			Make necessary symtab entries.
 *
 *  Called by:		main()
 *			node_constant_collapse()
 *
 *  Input Parameters:	p -- the NAME or ICON node
 *			arrayoffset -- array base offset, if array ref
 *			structoffset -- structure base offset, if structref
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	arrayelement
 *			arrayflag
 *			f_find_array
 *			fortran
 *			isstructprimary
 *			laststructnode
 *			maxstructnode
 *			structflag
 *			structnode[]
 *			symtab[]
 *			tysize[]
 *
 *  Globals Modified:	arrayelement
 *			arrayflag
 *			f_find_array
 *			f_find_struct
 *			laststructnode
 *			maxstructnode
 *			structflag
 *			structnode[]
 *
 *  External Calls:	add_arrayelement()
 *			add_structelement()
 *			addtreeasciz()
 *			ckrealloc()
 *			decref()
 *			find()
 *			sprintf()
 *			tcopy()
 *			tfree()
 *			types_differ()
 *			werror()
 *			strcpy()
 *			uerror()
 *			strcat()
 *			find_node_statement()
 *
 *****************************************************************************
 */

void process_oreg_foreg(p, arrayoffset, structoffset)
NODE *p;
long arrayoffset;
long structoffset;
{
    long a;
    long symloc;
    HASHU *sp;

	/* Find out if this is a C0 temporary node */
    f_do_not_insert_symbol = YES;
    
    symloc = find(p);

    f_do_not_insert_symbol = NO; /* restore */

    if ((!(symloc == (unsigned) (-1))) && 
        ((sp=symtab[symloc])->a6n.isc0temp))
      {
      /* Don't do array or structure things with c0 temp in the initial
       * assignment.
       */
      if (!c0tempflag)
	{
        if (arrayflag)
	  {
	  p->tn.arrayref = YES;
	  p->tn.arrayrefno = sp->a6n.wholearrayno;
	  p->tn.arrayelem = arrayelement;
          }
        if (structflag)
	  {
	  p->tn.structref = YES;
	  if (isstructprimary)
	      p->tn.arrayrefno = sp->a6n.wholearrayno;
          else
	    {
	    laststructnode++;
	    if (laststructnode >= maxstructnode)	/* table full? */
		{
		maxstructnode <<= 2;
		structnode =
			(NODE **) ckrealloc(structnode,
			maxstructnode * sizeof(NODE *));
		}
	    structnode[laststructnode] = p;
	    }
          }
	}
      p->tn.isc0temp = YES;
      structflag = NO;
      arrayflag = NO;
      arrayelement = NO;
      return;
      }
    allow_insertion = YES;

    a = p->tn.lval;		/* save offset */

    if (arrayflag)
	{
	p->tn.arrayref = YES;
	    {
	    p->tn.lval = arrayoffset;

	    /* insert the base into the symtab */
	    f_find_array = YES; /* flag to find() */
	    p->tn.arrayrefno = find(p);
	    f_find_array = NO;

	    if (!fortran)
	        {
	        /* C doesn't tell whether it's an array element reference -- we
	         * have to guess.
	         */
	        if ((a != arrayoffset) || (p->in.op == OREG)
	         || (!ISPTR(p->tn.type) && !ISARY(p->tn.type)))
	            arrayelement = YES;
	        if (!arrayelement
	         && types_differ(&(symtab[p->tn.arrayrefno]->a6n.type),
			     &(p->tn.type)))
		    arrayelement = YES;
	        }
	    }
	p->tn.arrayelem = arrayelement;
	}

    if (structflag)
	{
	p->tn.structref = YES;
	if (isstructprimary)
	    {
		{
	        p->tn.lval = structoffset;

	        /* insert the base into the symtab */
	        f_find_struct = YES; /* flag to find() */
	        p->tn.arrayrefno = find(p);
	        f_find_struct = NO;
		}
	    }
	else
	    {
	    laststructnode++;
	    if (laststructnode >= maxstructnode)	/* table full? */
		{
		maxstructnode <<= 2;
		structnode =
			(NODE **) ckrealloc(structnode,
			maxstructnode * sizeof(NODE *));
		}
	    structnode[laststructnode] = p;
	    }
	}

    p->tn.lval = a;
    allow_insertion = NO;

    if (arrayflag && !arrayelement)
      sp = symtab[p->tn.arrayrefno];
    else
      {
      if (!fortran && arrayelement) /* don't put C arrayelems in sym tab */
        goto exit;
      if (structflag)
	allow_insertion = YES; 
      symloc = find(p);   /* insert it into symtab */
      allow_insertion = NO; 
      if (symloc == (unsigned) (-1))
	goto exit;	
      sp = symtab[symloc];
  
      if (arrayelement && (! sp->a6n.farg)
       && (sp->a6n.wholearrayno == NO_ARRAY))
			  /* new entry in symtab */
          {
	  char name[300];
	  long elnum;
	  long elsize;
          register HASHU *ap= symtab[p->tn.arrayrefno];
  
          sp->a6n.wholearrayno = p->tn.arrayrefno;
          sp->allo.attributes = ap->allo.attributes;
          sp->a6n.isexternal = NO;
          sp->a6n.type = ap->x6n.type;
          sp->a6n.arrayelem = YES;
          add_arrayelement(p->tn.arrayrefno, symloc);
          if (ap->x6n.complex1)  /* complex array */
			        /* is this elem 1st or 2nd half ? */
	      {
	      elsize = (BTYPE(ap->x6n.type) == FLOAT) ? 8 : 16;
	      elnum = (p->tn.lval - arrayoffset) / elsize;
	      if ((p->tn.lval - arrayoffset) % elsize)
	          {
	          sp->a6n.complex1 = NO; 
	          sp->a6n.complex2 = YES; 
		  sprintf(name, "%s(%d)+%d", ap->a6n.pass1name, elnum+1,
			  (elsize == 8) ? 4 : 8);
	          sp->a6n.pass1name = addtreeasciz(name);
	          }
	      else	/* first half -- add second half */
	          {
	          NODE *t;
	          unsigned symloc2;
	          HASHU *fp;
  
		  sprintf(name, "%s(%d)", ap->a6n.pass1name, elnum+1);
	          sp->a6n.pass1name = addtreeasciz(name);
  
	          t  = tcopy(p);
	          t->tn.lval += (BTYPE(ap->a6n.type) == FLOAT)? 4:8;
    allow_insertion = YES;
	          symloc2 = find(t);
    allow_insertion = NO;
	          fp = symtab[symloc2];
	          fp->a6n.wholearrayno = p->tn.arrayrefno;
	          fp->allo.attributes = ap->allo.attributes;
	          fp->a6n.isexternal = NO;
	          fp->a6n.type = ap->an.type;
	          fp->a6n.arrayelem = YES;
	          fp->a6n.complex1 = NO; 
	          fp->a6n.complex2 = YES; 
		  sprintf(name, "%s(%d)+%d", ap->a6n.pass1name, elnum+1,
			  (elsize == 8) ? 4 : 8);
	          fp->a6n.pass1name = addtreeasciz(name);
	          add_arrayelement(p->tn.arrayrefno, symloc2);
	          tfree(t);
		  sp->a6n.back_half = symloc2;
	          }
	      }
	  else	/* not complex array */
	      {
	      TTYPE tw;
  
	      tw = ap->x6n.type;
	      while (ISARY(tw))
		  tw = decref(tw);
	      if (ISPTR(tw) || ISFTN(tw))
		  elsize = 4;
	      else
	          elsize = tysize[BTYPE(tw)];
	      elnum = (p->tn.lval - arrayoffset) / elsize;
	      if (fortran)
	          sprintf(name, "%s(%d)", ap->a6n.pass1name, elnum+1);
	      else
	          sprintf(name, "%s[%d]", ap->a6n.pass1name, elnum);
	      sp->a6n.pass1name = addtreeasciz(name);
	      }
	  if ( (p->tn.lval < arrayoffset) || 
	       ( (ap->allo.array_size >= 0) &&
                 (p->tn.lval >= (arrayoffset + ap->allo.array_size)) ) )
	    {
	    char *s;
  
	    /* allocate enough room for the message to be written */
	    s = (char *) ckalloc(strlen(sp->a6n.pass1name)+75);
	    strcpy(s,"Element ");
	    if (reading_input)
	      werror(strcat(strcat(s,sp->a6n.pass1name)," outside array bounds, procedure not optimized.")); 
	    else
	      {
	      int stmtno = find_node_statement(p);
	      if (stmtno == 0) 
	        uerror(strcat(strcat(s,sp->a6n.pass1name)," outside array bounds, cannot continue.")); 
	      else
	        {
		char stemp[12];

		sprintf(stemp,"%d",stmtno);
	        uerror(
		  strcat(
		    strcat(
		      strcat(
		        strcat(s,sp->a6n.pass1name),
			  " outside array bounds, cannot continue. (Line"),
			  stemp),
			")")); 
		}
	      }
	    FREEIT(s);
	    disable_procedure = YES;
	    }

          }
  
      if (structflag && isstructprimary && !sp->a6n.isstruct && !sp->an.farg
       && (sp->a6n.wholearrayno == NO_STRUCT))
			  /* new entry in symtab */
          {
          register HASHU *ap= symtab[p->tn.arrayrefno];
  
          sp->a6n.wholearrayno = p->tn.arrayrefno;
          sp->allo.attributes = ap->allo.attributes;
          sp->a6n.isstruct = YES;
          sp->a6n.isexternal = NO;
          add_structelement(p->tn.arrayrefno, symloc);
          }
  
      }
    if (sp->a6n.type.base == UNDEF)
        {
	if (p->in.op == FOREG)
	    sp->a6n.type = decref(p->tn.type);
	else
            sp->a6n.type = p->tn.type;
        sp->a6n.ptr |= ISPTR(sp->a6n.type);
        sp->a6n.func |= ISFTN(sp->a6n.type);
        }
    else if (!fortran && !structflag && !arrayflag
          && !(sp->an.type.base & TMASK)	/* no modifiers */
	  && (BTYPE(sp->an.type) != BTYPE(p->tn.type)))
        {
        /* types don't match -- maybe due to some wierd casting.  Better
         * keep our hands off this one.
         */
        sp->an.equiv = YES;
        }

    if (sp->a6n.equiv)
	p->tn.equivref = YES;

exit:
    structflag = NO;
    arrayflag = NO;
    arrayelement = NO;
}  /* process_oreg_foreg */

/*****************************************************************************
 *
 *  IS_IN_NAMELIST()
 *
 *  Description:	Check whether given name is in the list of names
 *
 *  Called by:		main()
 *
 *  Input Parameters:	name -- input name
 *			namelist -- vector of pointers to names
 *			lastname -- index of last pointer in vector
 *
 *  Output Parameters:	return value -- YES iff name is in list, NO otherwise
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	strcmp()
 *
 *****************************************************************************
 */

LOCAL long is_in_namelist(name, namelist, lastname)
char	*name;
char	**namelist;
long	lastname;
{
    register long i;

    for (i = 0; i <= lastname; ++i)
	{
	if (!strcmp(name, namelist[i]))
	    return(YES);
	}
    return(NO);
}  /* is_in_namelist */
