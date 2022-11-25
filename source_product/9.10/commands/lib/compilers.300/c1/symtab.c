/* file symtab.c */
/* @(#) $Revision: 70.1 $ */
/* KLEENIX_ID @(#)symtab.c	16.2 90/11/09 */

# include "c1.h"

# define HASHU_INIT_SZ	100
# define HASHU_INCR	50


HASHU **symtab;
HASHU **hashsymtab;
long	maxhashsym;		/* size of hashsymtab */
long 	maxsym;			/* size of symtab */
long	lastexternsym;		/* index of last external symbol in symtab */
long	lastfilledsym;		/* index of last filled entry in symtab */

int comtsize = COMTABINCR;
int fargtsize = COMTABINCR;
int ptrtsize = COMTABINCR;
int lastcom;
int lastfarg;
int lastptr;
unsigned *comtab;		/* the COMMON table */
unsigned *fargtab;		/* the formal argument table */
unsigned *ptrtab;		/* pointer targets table */
flag f_find_array;		/* flag to find() to look for XN, X6N */
flag f_find_common;		/* flag to find() to look for CN */
flag f_find_extern;		/* flag to find() that we're adding an extern */
flag f_find_struct;		/* flag to find() to look for SN, S6N */
flag f_find_pass1_symtab;	/* flag to find() that we're inserting because
				 * of a C1NAME or C1OREG entry
				 */
flag f_do_not_insert_symbol = NO;/* flag to find() if not found do not add */
flag f_definite_insert = NO;    /* flag to look_harder() this is definitely
				 * an insertion into the symbol table so
				 * do not look for possible overlapping 
				 * entries
				 */
flag disable_look_harder = NO;  /* flag to find() to avoid calling 
				 * look_harder() if it is not necessary.
				 */


long primes[] = { 127, 191, 293, 443, 661, 991, 1499, 2243, 3391, 4903,
		7411, 10007, 0
		};

HASHU *free_hashu_list;

COMM_NAME  *com_equiv_check = (COMM_NAME *) 0;

      void add_arrayelement();
      void add_common_region();
      void add_comelement();
      void add_farg();
      void add_pointer_target();
      void add_structelement();
LOCAL HASHU *alloc_hashu();
      void complete_ptrtab();
LOCAL void expand_symtab();
LOCAL void expand_hashsymtab();
      void file_init_symtab();
      unsigned find();
LOCAL void free_hashu();
      flag locate();
      HASHU *location();
LOCAL unsigned look_harder();
      void proc_init_symtab();
LOCAL void rehash_symtab();
      void symtab_insert();
LOCAL void make_com_equiv_field_entry();
LOCAL void make_com_entry();
      void check_com_entries();
LOCAL void free_com_entries();
LOCAL int compute_size();

#ifdef DEBUGGING
      void dumpdublock();
      void dumpsym();
      void dumpsymtab();
#endif

/*****************************************************************************
 *
 *			SYMTAB ORGANIZATION
 *
 *   hashsymtab                symtab                     HASHU structures
 *   (hash table)         (sequential table)              (disconnected nodes)
 *   (sparse)
 *   +----------+           +-----------+
 *   |          |           |     *->-->|->------------+
 *   |          |           |-----------|              |     +------------+
 *   |          |           |           |              +---->| -8(%a6)    |
 *   |          |           |           |                    | "foo"      |
 *   |          |           |           |<-- lastexternsym   |            |
 *   |          |           |           |    (index)         +------------+
 *   |          |           |           |                    
 *   |          |           |           |
 *   |          |           |-----------|
 *   |          |           |     *->-->|-->----------------------->+
 *   |          |           |-----------|<-----------<-------- +    |
 *   |          |           |           |      (an.symtabindex)|    |
 *   |          |           |           |                      |    |
 *   |          |           |           |<-- lastfilledsym     |   \ /
 *   |          |           |           |    (index)           |    |
 *   |          |           |           |                      |    |
 *   |          |           |           |<-- maxsym           /|\   |
 *   |          |           +-----------+                      |    |
 *   |----------|                                              |   \|/
 *   |    *-->->|--------->--------------------------------> +------------+
 *   |----------|                                            | _comm+0x8  |
 *   |          |<-- maxhashsym                              |  "comvar"  |
 *   +----------+                                            +------------+
 *
 *
 * Variable definitions:
 *    hashsymtab -- address of hashsymtab
 *    maxhashsym -- maximum # of entries in hashsymtab
 *    symtab     -- address of symtab
 *    lastexternsym -- index of last filled external (between procedures)
 *                       symbol
 *    lastfilledsym -- index of last filled symbol (in FORTRAN, lastexternsym
 *                       and lastfilledsym are always the same)
 *    maxsym     --- maximum # of entries in symtab
 *
 * Note that maxhashsym and maxsym are *NOT* the maximum possible index in the
 *    hashsymtab and symtab, respectively.  Because both tables start indexing
 *    with zero, the maximum fillable slot is maxhashsym-1 and maxsym-1.
 *
 * Within the HASHU record, the "symtabindex" field contains the index of
 *    the pointer within the symtab vector.
 *
 *****************************************************************************
 */

/******************************************************************************
*
*	add_arrayelement()
*
*	Description:		add_arrayelement() adds a linked list entry for
*				the element described in symtab[elloc] to the
*				array symtab[comloc].
*
*	Called by:		computed_aref_plus()
*				main()
*				process_name_icon()
*
*	Input parameters:	arrayloc - symtab index of the array
*				elloc - symtab index of the array element
*
*	Output parameters:	none
*
*	Globals referenced:	symtab
*
*	Globals modified:	symtab
*
*	External calls:		cerror()
*				alloc_plink()
*
*******************************************************************************/
void add_arrayelement(arrayloc, elloc)	unsigned arrayloc, elloc;
{
	register CLINK *cp;
	HASHU *hp;

	hp = symtab[arrayloc];
	if ((hp->xn.tag != XN) && (hp->xn.tag != X6N))
#pragma BBA_IGNORE
		cerror("Malformed array node in symbol table.");
	cp = hp->xn.member;
	if (!cp)
		{
		hp->xn.member = cp = (CLINK *) alloc_plink();
		cp->next = (CLINK *) NULL;
		cp->val = elloc;
#ifdef COUNTING
		_n_aryelems++;
#endif
		return;
		}
	while (cp->next)
		{
		if (cp->val == elloc) return;
		cp = cp->next;
		}
	if (cp->val == elloc) return;
	cp->next = (CLINK *) alloc_plink();
	cp->next->next  = (CLINK *) NULL;
	cp->next->val = elloc;
#ifdef COUNTING
	_n_aryelems++;
#endif
	return;
}

/*****************************************************************************
 *
 *  INSERT_ARRAY_ELEMENT() 
 *
 *  Description:  Add first element of an array to symtab.  
 *
 *  Called by:		pre_add_arrayelements()
 *
 *  Input Parameters:	array_loc: symbol table index of an array.
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced: none 
 *
 *  Globals Modified:   none
 *
 *  External Calls:	find()
 *			add_arrayelement()
 *			talloc()
 *
 *****************************************************************************
 */

void insert_array_element(array_loc) int array_loc;
{
	HASHU *sp, *nsp;
	NODE  *np;
        int elemloc;

	f_find_pass1_symtab = YES;
	disable_look_harder = YES;

        sp = symtab[array_loc];
	if (sp->a6n.array && (!sp->a6n.isstruct))
            {
            np = talloc();
	    if (sp->a6n.tag == X6N)
		np->tn.op = OREG;
	    else if (sp->a6n.tag == XN)
		np->tn.op = NAME;
	    else if ((sp->a6n.tag == AN) || (sp->a6n.tag == A6N))
		{ /* AN -- Caused by existance of local static array: ignore 
		     A6N-- Caused by existance of variable farg array: ignore */
		disable_look_harder = NO;
		f_find_pass1_symtab = NO;
		np->in.op = FREE;
		return;
		}
	    else
#pragma BBA_IGNORE
		cerror("Unexpected entrytype for array.");
            np->atn.name = sp->an.ap;
            np->tn.lval = sp->an.offset;
	    allow_insertion = YES;
	    disable_look_harder = YES;
	    elemloc = find(np);
	    disable_look_harder = NO;
	    allow_insertion = NO;
	    np->in.op = FREE;

	    nsp = symtab[elemloc];
	    nsp->allo.attributes = sp->allo.attributes;
            nsp->an.isexternal = NO;
	    nsp->a6n.pass1name = sp->a6n.pass1name;
	    nsp->a6n.type = sp->a6n.type; 
	    nsp->a6n.wholearrayno = array_loc;
	    nsp->a6n.arrayelem = YES;

	    add_arrayelement(array_loc,elemloc);
            }

	disable_look_harder = NO;
	f_find_pass1_symtab = NO;
}

/*****************************************************************************
 *
 *  ADD_COMMON_REGION()
 *
 *  Description:	Make a new entry into the comtab array and link
 *			it to the symtab[comloc] element describing the COMMON
 *			region.
 *
 *  Called by:		symtab_insert()
 *
 *  Input Parameters:	comloc -- symtab index of CN entry for COMMON region
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	comtab
 *			comtsize
 *			lastcom
 *
 *  Globals Modified:	comtab
 *			comtsize
 *			lastcom
 *
 *  External Calls:	cerror()
 *			ckrealloc()
 *
 *****************************************************************************
 */

void add_common_region(comloc)	unsigned comloc;
{
	register unsigned *cp;

	for (cp = &comtab[lastcom]; cp >= comtab; cp--)
		if (*cp == comloc) return;	/* already in the table */
	
	if (++lastcom >= comtsize)
		{
		/* table overflow - reallocate */
		comtsize += COMTABINCR;
		comtab = (unsigned *) ckrealloc(comtab, comtsize * sizeof(unsigned));
		if (!comtab)
#pragma BBA_IGNORE
			cerror("Unable to allocate the comtab.");
		}

	comtab[lastcom] = comloc;
#ifdef COUNTING
	_n_common_regions++;
#endif
	return;
}

/*****************************************************************************
 *
 *  ADD_COMELEMENT()
 *
 *  Description:	Add a linked list entry for the element described in
 *			symtab[elloc] to the common region symtab[comloc].
 *
 *  Called by:		symtab_insert()
 *
 *  Input Parameters:	comloc -- symtab index of common region CN entry
 *			elloc  -- symtab index of item in common
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	alloc_plink()
 *			cerror()
 *
 *****************************************************************************
 */

void add_comelement(comloc, elloc)	unsigned comloc, elloc;
{
	register CLINK *cp;
	HASHU *hp;

	hp = symtab[comloc];
	if (hp->cn.tag != CN)
#pragma BBA_IGNORE
		cerror("Malformed COMMON node in symbol table.");
	cp = hp->cn.member;
	if (!cp)
		{
		hp->cn.member = cp = (CLINK *) alloc_plink();
		cp->next = (CLINK *) NULL;
		cp->val = elloc;
#ifdef COUNTING
		_n_comelems++;
#endif
		return;
		}
	while (cp->next)
		{
		if (cp->val == elloc) return;
		cp = cp->next;
		}
	if (cp->val == elloc) return;
	cp->next = (CLINK *) alloc_plink();
	cp->next->next = (CLINK *) NULL;
	cp->next->val = elloc;
#ifdef COUNTING
	_n_comelems++;
#endif
	return;
}  /* add_comelement */

/*****************************************************************************
 *
 *  ADD_FARG()
 *
 *  Description:	Add formal argument to the fargtab
 *
 *  Called by:		symtab_insert()
 *
 *  Input Parameters:	fargloc -- symtab index of farg
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	fargtab
 *			fargtsize
 *			lastfarg
 *			symtab
 *
 *  Globals Modified:	fargtab
 *			fargtsize
 *			lastfarg
 *
 *  External Calls:	cerror()
 *			ckrealloc()
 *
 *****************************************************************************
 */

void add_farg(fargloc)	unsigned fargloc;
{
	register unsigned *fp;

	for (fp = &fargtab[lastfarg]; fp >= fargtab; fp--)
		if (*fp == fargloc) return;	/* already in the table */
	
	if (++lastfarg >= fargtsize)
		{
		/* table overflow - reallocate */
		fargtsize += COMTABINCR;
		fargtab = (unsigned *) ckrealloc(fargtab, fargtsize * sizeof(unsigned));
		if (!fargtab)
#pragma BBA_IGNORE
			cerror("Unable to allocate the fargtab.");
		}

	fargtab[lastfarg] = fargloc;
	symtab[fargloc]->a6n.farg = YES;
#ifdef COUNTING
	_n_fargs++;
#endif
	return;
}  /* add_farg */

/*****************************************************************************
 *
 *  ADD_POINTER_TARGET()
 *
 *  Description:	Add entry to ptrtab array.  This array contains
 *			symtab indices of items that may be referenced
 *			by a pointer indirection.
 *
 *  Called by:		main()		 -- in c1.c
 *			complete_ptrtab()
 *
 *  Input Parameters:	symloc -- symtab index of item to be added to ptrtab
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastptr
 *			ptrtab
 *			ptrtsize
 *
 *  Globals Modified:	lastptr
 *			ptrtab
 *			ptrtsize
 *
 *  External Calls:	cerror()
 *			ckrealloc()
 *
 *****************************************************************************
 */

void add_pointer_target(symloc)
unsigned symloc;
{
	register unsigned *pp;

	for (pp = &ptrtab[lastptr]; pp >= ptrtab; pp--)
		if (*pp == symloc) return;	/* already in the table */
	
	if (++lastptr >= ptrtsize)
		{
		/* table overflow - reallocate */
		ptrtsize += COMTABINCR;
		ptrtab = (unsigned *) ckrealloc(ptrtab, ptrtsize * sizeof(unsigned));
		if (!ptrtab)
#pragma BBA_IGNORE
			cerror("Unable to allocate the ptrtab.");
		}

	ptrtab[lastptr] = symloc;
#ifdef COUNTING
	_n_ptr_targets++;
#endif
	return;
}  /* add_pointer_target */

/*****************************************************************************
 *
 *  ADD_STRUCTELEMENT()
 *
 *  Description:	Add a linked list entry for the element described in
 *			symtab[elloc] to the struct symtab[comloc].
 *
 *  Called by:		main() -- in c1.c
 *
 *  Input Parameters:	structloc -- symtab index of SN/S6N entry for struct
 *			elloc -- symtab index of item within struct
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	alloc_plink()
 *			cerror()
 *
 *****************************************************************************
 */

void add_structelement(structloc, elloc)	unsigned structloc, elloc;
{
	register CLINK *cp;
	HASHU *hp;

	hp = symtab[structloc];
	if ((hp->xn.tag != SN) && (hp->xn.tag != S6N))
#pragma BBA_IGNORE
		cerror("Malformed struct node in symbol table.");
	cp = hp->xn.member;
	if (!cp)
		{
		hp->xn.member = cp = (CLINK *) alloc_plink();
		cp->next = (CLINK *) NULL;
		cp->val = elloc;
#ifdef COUNTING
		_n_structelems++;
#endif
		return;
		}
	while (cp->next)
		{
		if (cp->val == elloc) return;
		cp = cp->next;
		}
	if (cp->val == elloc) return;
	cp->next = (CLINK *) alloc_plink();
	cp->next->next  = (CLINK *) NULL;
	cp->next->val = elloc;
#ifdef COUNTING
	_n_structelems++;
#endif
	return;
}  /* add_structelement */

/*****************************************************************************
 *
 *  ALLOC_HASHU()
 *
 *  Description:	Allocate a symbol table entry block.
 *
 *  Called by:		find()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	return value -- pointer to zeroed HASHU block
 *
 *  Globals Referenced:	free_hashu_list
 *
 *  Globals Modified:	free_hashu_list
 *
 *  External Calls:	ckalloc();
 *
 *****************************************************************************
 */

LOCAL HASHU *alloc_hashu()
{
    HASHU *retval;
    HASHU *freeblocks;
    register HASHU *hp, *hpend;

    if (free_hashu_list == NULL)
	{
	/* allocate a block of HASHU_INCR more free HASHU blocks */
	freeblocks = (HASHU *) ckalloc(HASHU_INCR * sizeof(HASHU));
	for (hp = freeblocks, hpend = freeblocks + HASHU_INCR-1; hp <= hpend; --hpend)
	    {
	    *((HASHU **)hpend) = free_hashu_list;
	    free_hashu_list = hpend;
	    }
	}

    retval = free_hashu_list;
    free_hashu_list = *((HASHU **) free_hashu_list);
    memset(retval, 0, sizeof(HASHU));

    return(retval);
}  /* alloc_hashu */

/******************************************************************************
*
*	CHECK_COM_ENTRIES()
*
*	Description:		check_com_entries traverses the data 
*				structure used to check for additional
*				equivalence relationships.  As overlaps
*				in COMMON entries are found the equiv
*				bit is set in the respective symbol table
*				entries.
*
*	Called by:		main()
*
*	Input parameters:	none   
*
*	Output parameters:	none
*
*	Globals referenced:	com_equiv_check
*
*	Globals modified:	symtab[]  (equiv bit)
*
*	External calls:		none
*
*******************************************************************************/
void check_com_entries()
{
COMM_NAME *com_list = com_equiv_check;

while (com_list)
  {
  COMM_EQUIV_FIELD  *field = com_list->first_field;
  while (field)
    {
    int range = field->sp->cn.offset + compute_size(field->sp);
    COMM_EQUIV_FIELD  *follow_field = field->next_field;

    while (follow_field)
      {
      if (range > follow_field->sp->cn.offset)
        /* Fields overlap, set equivalence bits. */
        {
        field->sp->cn.equiv = 1; 
        follow_field->sp->cn.equiv = 1; 

        follow_field = follow_field->next_field;
        }
      else
        /* If this follow_field didn't overlap the others won't either. */
        follow_field = (COMM_EQUIV_FIELD *) 0;
      }
   
    field = field->next_field; 
    } 

  com_list = com_list->next_name;
  }
free_com_entries();
}


/*****************************************************************************
 *
 *  COMPLETE_PTRTAB()
 *
 *  Description:	Complete the contents of the ptrtab (possible pointer
 *			targets) vector.  Add all non-constant static items.
 *
 *  Called by:		main() -- in c1.c
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastfilledsym
 *			symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_pointer_target()
 *
 *****************************************************************************
 */

void complete_ptrtab()
{
    register long i;
    register HASHU *sp;
    NODE *np;

    for (i = lastfilledsym; i >= 0; i--)
	{
	sp = symtab[i];
	if (sp->an.seenininput		/* use seen in code */
	 && ((!sp->an.func) || (sp->an.ptr))
	 && ((i <= lastexternsym)	    /* declared outside of proc */
	  || (sp->an.isexternal)	    /* or with "extern" attribute */
	  || (((sp->an.tag == AN) && (!sp->an.isconst)
					    /* or is static, non-constant */
	   || (sp->an.tag == SN) || (sp->an.tag == XN))
# ifdef FTN_POINTERS
		)))
# else
		&& (!fortran || !sp->an.common))))
# endif FTN_POINTERS
					    /* or is static struct or array */
	    {
	    add_pointer_target(i);	/* add symtab into to table */
	    }
	}

     if (lastptr < 0)
	{
	/* The ptrtab is empty. We need at least 1 entry in it to handle
	   array formal arguments. Make a bogus one (offset=0 is normally
	   impossible).
	*/
	np = block(OREG, 0, TMPREG, INT, 0);
	allow_insertion = YES;
	sp = symtab[i = find(np)];
	allow_insertion = NO;
	np->in.op = FREE;
	sp->an.isexternal = YES;
	add_pointer_target(i);
	}
}  /* complete_ptrtab */

/******************************************************************************
*
*	COMPUTE_SIZE()
*
*	Description:		determine the size in bytes of the symbol 
*				table entry refered to by sp.
*
*	Called by:		check_com_entries()
*				look_harder()
*
*	Input parameters:	sp - pointer to a symbol table entry.   
*
*	Output parameters:	return value is size in bytes of *sp
*
*	Globals referenced:	symtab[]
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
#  define MAX_SIMPLE_TYPE 19
   /* various 'simple' types index this array to obtain their size in bytes */
   int  sizetable[MAX_SIMPLE_TYPE] = {0,0,1,2,4,4,4,8,0,0,4,0,1,2,4,4,0,16,1};

LOCAL int compute_size(sp)
  HASHU *sp;
{
if (ISPTR(sp->a6n.type) || sp->a6n.ptr)
  return(4);
else if (sp->a6n.isstruct)
  return(1); /* size information not know for structures */
else if (sp->a6n.array && !sp->a6n.arrayelem)
  return(sp->a6n.array_size);
else if ((int) BTYPE(sp->a6n.type) >= MAX_SIMPLE_TYPE)
  cerror("Unknown base type in compute_size.");
else
  return(sizetable[(int) BTYPE(sp->a6n.type)]);
}


/*****************************************************************************
 *
 *  EXPAND_HASHSYMTAB()
 *
 *  Description:	Enlarge the hashsymtab and rehash all active symtab
 *			entries into it.
 *
 *  Called by:		find()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	hashsymtab
 *			maxhashsym
 *			primes[]
 *
 *  Globals Modified:	hashsymtab
 *			maxhashsym
 *
 *  External Calls:	cerror()
 *			ckrealloc()
 *			rehash_symtab()
 *
 *****************************************************************************
 */

LOCAL void expand_hashsymtab()
{
    register long *i;

    /* find next larger size in the primes[] table */
    for (i = primes; *i != 0; ++i)
	{
	if (*i > maxhashsym)
	    break;
	}
    if (*i == 0)
#pragma BBA_IGNORE
	cerror("hashsymtab too large in expand_hashsymtab()");

    /* reallocate the hashsymtab */
    maxhashsym = *i;
    hashsymtab = (HASHU **) ckrealloc(hashsymtab, maxhashsym * sizeof(HASHU*));

    /* rehash all active symbols into it */
    rehash_symtab();		/* also zeroes out all unused entries */
}  /* expand_hashsymtab */

/*****************************************************************************
 *
 *  EXPAND_SYMTAB()
 *
 *  Description:	Expand the symtab table.
 *
 *  Called by:		find()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	maxsym
 *			symtab
 *
 *  Globals Modified:	maxsym
 *			symtab
 *
 *  External Calls:	ckrealloc()
 *			memset()
 *
 *****************************************************************************
 */

LOCAL void expand_symtab()
{
    long newsize;

    newsize = maxsym + (maxsym >> 1);	/* increase by 50 % */

    symtab = (HASHU **) ckrealloc(symtab, newsize * sizeof(HASHU *));

    /* zero out entries beyond the previous end of the table */
    memset(symtab + maxsym, 0, (newsize - maxsym) * sizeof(HASHU *));

    maxsym = newsize;

}  /* expand_symtab */

/*****************************************************************************
 *
 *  FILE_INIT_SYMTAB()
 *
 *  Description:	Initialize the symbol table data structures.
 *			Called only once -- before processing any of the
 *			input file.
 *
 *  Called by:		main() -- c1.c
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	primes[]
 *
 *  Globals Modified:	free_hashu_list
 *			hashsymtab
 *			lastexternsym
 *			lastfilledsym
 *			maxhashsym
 *			maxsym
 *			symtab
 *
 *  External Calls:	clralloc()
 *
 *****************************************************************************
 */

void file_init_symtab()
{
    HASHU *freeblocks;
    register HASHU *hp;
    register HASHU *hpend;

    maxsym = HASHU_INIT_SZ;		/* start with space for HASHU_INIT_SZ items */
    maxhashsym = primes[0];	/* start with 1st prime (127) */

    symtab = (HASHU **) clralloc(maxsym * sizeof(HASHU *));
    hashsymtab = (HASHU **) clralloc(maxhashsym * sizeof(HASHU *));

    lastexternsym = -1;
    lastfilledsym = -1;

    /* allocate a block of HASHU_INIT_SZ free HASHU blocks */
    free_hashu_list = NULL;
    freeblocks = (HASHU *) ckalloc(HASHU_INIT_SZ * sizeof(HASHU));
    for (hp = freeblocks, hpend = freeblocks + HASHU_INIT_SZ-1; hp <= hpend; --hpend)
	{
	*((HASHU **)hpend) = free_hashu_list;
	free_hashu_list = hpend;
	}

}  /* file_init_symtab */

/******************************************************************************
*
*	FIND()
*
*	Description:		find() is the basic symbol table manipulation
*				routine. If a symbol is found in the symbol
*				table it returns its symtab index. If not,
*				then it adds the symbol to symtab and then
*				returns its symtab index.
*
*	Called by:		computed_aref_unary_mul()
*				computed_aref_plus()
*				main()
*				symtab_insert()
*				assign()
*				operand_index()
*				update_cv()
*				cpnode()
*				location()
*				locate()
*				mktemp()
*				process_call_arg()
*				process_call_array_arg()
*				process_def()
*				process_use()
*				recognize_array()
*
*	Input parameters:	np - NODE pointer to a subtree
*
*	Output parameters:	none
*
*	Globals referenced:	f_find_array
*				f_find_common
*				f_find_extern
*				f_find_struct
*				lastfilledsym
*				maxhashsym
*				maxsym
*				symtab
*
*	Globals modified:	symtab
*
*	External calls:		alloc_hashu()
*				cerror()
*				eprint()
*				expand_hashsymtab()
*				expand_symtab()
*				fwalk()
*				new_set()
*
*******************************************************************************/
unsigned find (np) register NODE *np;
{
	register HASHU **hp;
	register HASHU *sp;
	register char *cp;
	register unsigned hasher;
	HASHU		**hpend;
	flag 		entrytype;
	unsigned xx;

	switch(np->in.op)
		{
		case OREG:
		case FOREG:
			entrytype = f_find_array ? X6N : 
					f_find_struct ? S6N : A6N;
			hasher = (unsigned) (np->tn.lval);
			break;

		case REG:
			entrytype = ON;
			hasher = (unsigned) (np->tn.rval);
			break;

		case NAME:
		case ICON:
			entrytype = f_find_common ? CN : f_find_array ? XN :
					f_find_struct ? SN : AN;
			hasher = 0;
			cp = np->atn.name;
			if (*cp)
				{
				while (*cp)
					hasher += *cp++;
				}
			else goto error;
			hasher += (unsigned) np->tn.lval;
			break;

		default:
error:
# ifdef DEBUGGING
			fwalk(np, eprint,0);
# endif DEBUGGING
			cerror("unknown optype passed to find()");
		}
	hp = hashsymtab + HASHFT(hasher);
	hpend = hashsymtab + maxhashsym;
	for (;;)
		{
		if (*hp == NULL)
		    	{
		    	if (f_do_not_insert_symbol) 
			    return( (unsigned) (-1) );
		        if (!fortran && !disable_look_harder
			 && ((entrytype == A6N) || (entrytype == AN)))
			    {
			    xx = look_harder(np);
		    	    if (xx != (unsigned) (-1))
			        return(xx);
			    }
			/*
			 * Don't allow the insertion in the symbol
			 * table unless it is from a C1NAME, C1OREG
			 * or REG node
			 * or it has a name that begins with 'L' 
			 * when compiling C programs.
			 * (Names beginning with L are front end
			 *  generated lables.)
			 */
			if (!fortran && !allow_insertion && 
			    (entrytype != ON) &&
			    ((np->tn.name == NULL) || (np->tn.name[0] != 'L')))
			  return ((unsigned) (-1));
#ifdef COUNTING
			_n_symtab_entries++;
#endif
			sp = alloc_hashu();
			*hp = sp;
			sp->a6n.tag = entrytype;
			if (entrytype == AN || entrytype == CN
			 || entrytype == XN || entrytype == SN)
				{
				sp->an.ap = np->atn.name;
				sp->an.offset = np->tn.lval;
				}
			else
				sp->a6n.offset = hasher;
			if (defsets_on)
				sp->a6n.defset = new_set(maxdefs);
			sp->a6n.wholearrayno = NO_ARRAY;
			sp->a6n.symtabindex = ++lastfilledsym;
			if (!f_find_pass1_symtab)
				sp->a6n.seenininput = YES;
			if (lastfilledsym >= maxsym)
				expand_symtab();
			symtab[lastfilledsym] = sp;
			if (f_find_extern)
				++lastexternsym;
			if (lastfilledsym >= ((maxhashsym * 4) / 5))
				expand_hashsymtab();
			return (lastfilledsym);
			}
		if ((*hp)->a6n.tag != entrytype) goto next;
		if (entrytype == AN || entrytype == CN || entrytype == XN
		 || entrytype == SN)
			{
			if ( strcmp((*hp)->an.ap, np->atn.name) ||
				(*hp)->an.offset != np->tn.lval )
				goto next;
			}
		else
			{
			if ((*hp)->a6n.offset != hasher) goto next;
			}
		if (!f_find_pass1_symtab)
			(*hp)->a6n.seenininput = YES;
		return ((*hp)->an.symtabindex);

next:		if (++hp >= hpend)
			{
			hp = hashsymtab;
			}
		}
}

/******************************************************************************
*
*	FREE_COM_ENTRIES()
*
*	Description:		free_com_entries frees the data 
*				structure used to check for additional
*				equivalence relationships.
*
*	Called by:		check_com_entries()
*
*	Input parameters:	none   
*
*	Output parameters:	none
*
*	Globals referenced:	com_equiv_check
*
*	Globals modified:	com_equiv_check
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void free_com_entries()
{
COMM_NAME *com_list = com_equiv_check;

while (com_list)
  {
  COMM_NAME *t_com_list = com_list;

  COMM_EQUIV_FIELD  *field = com_list->first_field;
  while (field)
    {
    COMM_EQUIV_FIELD *t_field = field;

    field = field->next_field;
    free(t_field);
    }

  com_list = com_list->next_name;
  free(t_com_list);
  }

com_equiv_check = (COMM_NAME *) 0; 
}


/*****************************************************************************
 *
 *  FREE_HASHU()
 *
 *  Description:	Return a HASHU block to the free pool.
 *
 *  Called by:		proc_init_symtab()
 *
 *  Input Parameters:	hp -- pointer to HASHU block to be free'd
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	free_hashu_list
 *
 *  Globals Modified:	free_hashu_list
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

LOCAL void free_hashu(hp)
HASHU *hp;
{
    *((HASHU **) hp) = free_hashu_list;
    free_hashu_list = hp;
}  /* free_hashu */

/*****************************************************************************
 *
 *  LOCATE()
 *
 *  Description:	Do a conditional call to location() if the node shape
 *			is appropriate, and return YES. Otherwise return NO.
 *			Combine with location() later.
 *
 *  Called by:		add_sinit()		-- loops.c
 *			associated_indvar()	-- loops.c
 *			check_ltypes()		-- loops.c
 *			init_defs()		-- oglobal.c
 *			lisearch1()		-- loops.c
 *			minor_aiv()		-- loops.c
 *
 *  Input Parameters:	np	-- NODE pointer
 *
 *  Output Parameters:	locpp	-- HASHU * symtab entry for node
 *			return value -- YES or NO
 *
 *  Globals Referenced:	symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	find()
 *			location()
 *
 *****************************************************************************
 */

flag locate(np, locpp)	NODE *np; HASHU **locpp;
{
long find_loc;
	
top:
	if (np->in.arrayref)
		{
		*locpp = location(np);
		return (YES);
		}
	else
		switch (np->in.op)
		{
		case ICON:
		case FCON:
			if (!np->atn.name) return (NO);
			/* fall thru */

		case STRING:
		case NAME:
		case REG:
		case OREG:
		case FOREG:
			/* lookup */
			find_loc = find(np);
			if (find_loc != (unsigned) (-1))
			  {
			  *locpp = symtab[find_loc];
			  return (YES);
			  }
			else
			  if (np->tn.arrayrefno != NO_ARRAY)
			    {
			    *locpp = symtab[np->tn.arrayrefno];
			    return (YES);
			    }
			  else
			    return (NO);

		case CM:
			np = np->in.right;
			goto top;

		case UCM:
			np = np->in.left;
			goto top;

		case UNARY MUL:
			if (np->in.left->in.op == OREG)
				{
				*locpp = location(np->in.left);
				return((*locpp)->an.farg /* YES or NO */);
				}
			/* fall thru */

		default:
			/* do nothing */
			return (NO); 
		}
}  /* locate */

/*****************************************************************************
 *
 *  LOCATION()
 *
 *  Description:	Find HASHU * node associated with given node.
 *			Node may be a CM/UCM node (arg lists), in which
 *			case use the appropriate argument.  Return whole
 *			array symtab entry if this is an array reference.
 *
 *  Called by:		assign()		-- dag.c
 *			lisearch1()		-- loops.c 
 *			locate()
 *			strength_reduction()	-- loops.c
 *
 *  Input Parameters:	np -- NODE pointer
 *
 *  Output Parameters:	return value -- HASHU * for symtab entry
 *
 *  Globals Referenced:	symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	find()
 *
 *****************************************************************************
 */

HASHU *location(np)	register NODE *np;
{
	int i;

	if (np->in.op == CM )
		np = np->in.right;
	else if (np->in.op == UCM)
		np = np->in.left;

	i = np->in.arrayref? np->in.arrayrefno : find(np);
	return( symtab[i] );
}  /* location */

/******************************************************************************
*
*	LOOK_HARDER()
*
*	Description:	look_harder() is a companion routine to find.
*			A C program may specify the address of a variable
*			plus some offset (&xyz + 2).  The offset specified
*			is usually folded into the tree by cpass1 making it
*			difficult to identify this expression as a definition
*			or reference to xyz.  When find() is unable to locate
*			an AN or A6N item in the symbol table look_harder()
*			is called.  This routine looks in the vicinity to
*			see if there is a 'simple' variable that covers
*			the item being looked up.  If so, that item in the
*			symbol table is returned.  Arrays and structures are
*			not included in the mechanism because they are not
*			included in register allocation in C and FORTRAN does
*			not have address expressions of this kind.
*
*	Called by:		find	
*
*	Input parameters:	np - NODE pointer to a subtree
*
*	Output parameters:	none
*
*	Return valu:		symbol table index of overlaying variable 
*				or the unsigned equivalent of -1 for not found
*
*	Globals referenced:	f_do_not_insert_symbol
*                               f_definite_insert
*
*	Globals modified:	f_do_not_insert_symbol
*
*	External calls:		find()       
*				compute_size()
*
*******************************************************************************/
   /* Largest data type is DOUBLE and could start 7 bytes earlier */
#  define POSSIBLE_DELTA -7 

LOCAL unsigned look_harder(np)
NODE	*np;
{
	CONSZ	close_enough = -1;
	CONSZ	save_lval = np->tn.lval;
	register unsigned xx;
	unsigned size;
	register HASHU *sp;


	f_do_not_insert_symbol = YES;

	while ((close_enough >= POSSIBLE_DELTA) && (f_definite_insert == NO))
		{
		np->tn.lval = save_lval + close_enough;

		if ( np->tn.lval & 1) /* odd? */
		  {
		  np->tn.lval -=1;
		  if ((close_enough -=1) < POSSIBLE_DELTA)
			break;
		  }

		xx = find(np);
		if (xx != (unsigned) (-1) )
		  {
		  sp = symtab[xx];
		  size = compute_size(sp);

		  if (size > (-close_enough)) /*found something close enough*/
		    {
		    /* do not allow register allocation on this item */
		    sp->a6n.equiv = YES; 
		    f_do_not_insert_symbol = NO;
		    np->tn.lval = save_lval;
		    return(xx);	
		    }
		  }
		close_enough -= 1;
		}
	f_do_not_insert_symbol = NO;
	np->tn.lval = save_lval;
	return( (unsigned) (-1) );	

}


/******************************************************************************
*
*	MAKE_COM_ENTRY()
*
*	Description:		make_com_entry makes an entry in the data 
*				structure used to check for additional
*				equivalence relationships.
*
*	Called by:		symtab_insert()
*
*	Input parameters:	sp - pointer to a symbol table entry.   
*
*	Output parameters:	none
*
*	Globals referenced:	com_equiv_check
*
*	Globals modified:	com_equiv_check
*
*	External calls:		make_com_equiv_field_entry();
*
*******************************************************************************/
LOCAL void make_com_entry(sp)
  HASHU *sp;
{
/* Don't make entries for array elements.  The whole array will be entered. */
if (sp->a6n.array && sp->a6n.arrayelem)
  return;

if (com_equiv_check == (COMM_NAME *) 0) /* first entry */
  {
  com_equiv_check = (COMM_NAME *) ckalloc(sizeof(COMM_NAME));
  com_equiv_check->name = sp->cn.ap; 
  com_equiv_check->next_name = (COMM_NAME *) 0; 
  com_equiv_check->first_field = (COMM_EQUIV_FIELD *) 0;
  make_com_equiv_field_entry(&com_equiv_check->first_field,sp);
  }
else
  {
  short done = 0;
  COMM_NAME *com_list = com_equiv_check;
  while (!done)
    {
    if (!strcmp(com_list->name,sp->cn.ap))
      /* Add to existing comm_name list */
      {
      make_com_equiv_field_entry(&com_list->first_field,sp);
      done = 1; 
      }
    else if (com_list->next_name == (COMM_NAME *) 0) 
      /* Add COMMON name to end of comm_name list */
      { 
      com_list->next_name = (COMM_NAME *) ckalloc(sizeof(COMM_NAME));
      com_list->next_name->name = sp->cn.ap; 
      com_list->next_name->next_name = (COMM_NAME *) 0; 
      com_list->next_name->first_field = (COMM_EQUIV_FIELD *) 0;
      make_com_equiv_field_entry(&com_list->next_name->first_field,sp);
      done = 1; 
      }
    else /* keep searching comm_name list */
      com_list = com_list->next_name;
    }
  }
}


/******************************************************************************
*
*	MAKE_COM_EQUIV_FIELD_ENTRY()
*
*	Description:		make_com_equiv_field_entry makes an entry  
*				to the list of fields associated with a 
*				specific COMMON name.  The list is sorted
*				by offset of the field entries.
*
*	Called by:		make_com_entry()
*
*	Input parameters:	com_node_list - COMMON field list.
*				sp - pointer to a symbol table entry.   
*
*	Output parameters:	none
*
*	Globals referenced:	none           
*
*	Globals modified:	none
*
*	External calls:		none                        
*
*******************************************************************************/
LOCAL void make_com_equiv_field_entry(com_field_list, sp)
  COMM_EQUIV_FIELD **com_field_list;
  HASHU             *sp;
{
short done = 0;
COMM_EQUIV_FIELD *new_entry = 
                         (COMM_EQUIV_FIELD *) ckalloc(sizeof(COMM_EQUIV_FIELD));

new_entry->sp = sp;

while (!done)
  {
  if ((*com_field_list == (COMM_EQUIV_FIELD *) 0) || 
      ((*com_field_list)->sp->cn.offset > sp->cn.offset)) 
    {
    new_entry->next_field = *com_field_list;
    *com_field_list = new_entry;
    done = 1;
    }
  else
    com_field_list = &(*com_field_list)->next_field;
  }
}

/*****************************************************************************
 *
 *  PROC_INIT_SYMTAB()
 *
 *  Description:	Initialize the symbol table structures for the next
 *			procedure.  Zero out entries/flags.  Reset high
 *			water mark.
 *
 *  Called by:		main() -- c1.c
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastexternsym
 *			lastfilledsym
 *			symtab[]
 *
 *  Globals Modified:	lastfilledsym
 *			symtab[]
 *			free_hashu_list
 *
 *  External Calls:	free_hashu()
 *			rehash_symtab()
 *
 *****************************************************************************
 */

void proc_init_symtab()
{
    register long i;
    register HASHU *sp;

    for (i = 0; i <= lastfilledsym; ++i)
	{
	sp = symtab[i];
	if (i <= lastexternsym)
	    {
	    sp->an.seenininput = NO;   /* not in code for next proc */
	    sp->an.dbra_index = NO;
	    sp->xn.member = NULL;
	    sp->an.cvbnum = 0;
	    sp->an.stmtno = 0;
	    sp->an.regno = 0;
	    }
	else
	    {
	    if (sp->an.isexternal)	/* external sym -- move it */
	        {
		/* zero use flag -- not seen in code for next proc yet */
		sp->an.seenininput = NO;
	        sp->an.dbra_index = NO;
	        sp->xn.member = NULL;
		sp->an.cvbnum = 0;
	        sp->an.stmtno = 0;
	        sp->an.regno = 0;
	        if (i != ++lastexternsym)
		    {

		    symtab[lastexternsym] = sp;
		    sp->an.symtabindex = lastexternsym;
		    symtab[i] = NULL;
		    }
	        }
	    else
	        {
	        free_hashu(sp);
	        symtab[i] = NULL;
	        }
	    }
	}

    lastfilledsym = lastexternsym;

    rehash_symtab();	/* clear hashsymtab and rehash entries into it */

}  /* proc_init_symtab */

/*****************************************************************************
 *
 *  REHASH_SYMTAB()
 *
 *  Description:	Clear all entries in the hashsymtab and rehash all
 *			active symtab entries.
 *
 *  Called by:		proc_init_symtab()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	hashsymtab
 *			lastfilledsym
 *			maxhashsym
 *			symtab
 *
 *  Globals Modified:	hashsymtab contents
 *
 *  External Calls:	memset
 *
 *****************************************************************************
 */

LOCAL void rehash_symtab()
{
    register HASHU *sp;
    register long hasher;
    register long i;

    /* clear hashsymtab -- all entries */
    memset(hashsymtab, 0, maxhashsym * sizeof(HASHU*));

    /* rehash active symtab entries */
    for (i = 0; i <= lastfilledsym; ++i)
	{
	sp = symtab[i];

	switch (sp->an.tag)
	    {
	    case AN:
	    case CN:
	    case XN:
	    case SN:
			{
		        register char *cp;
			hasher = 0;
			if (cp = sp->an.ap)
			    {
			    while (*cp)
			        {
			        hasher += *cp++;
			        }
			    }
			hasher += sp->an.offset;
			}
			break;
			
	    case A6N:
	    case ON:
	    case X6N:
	    case S6N:
			hasher = sp->a6n.offset;
			break;
	    }

	{
        register HASHU **hp;
        register HASHU **hpend;
	hp = hashsymtab + HASHFT(hasher);
	hpend = hashsymtab + maxhashsym;
	for (;;)
	    {
	    if (*hp == 0)
		{
		*hp = sp;
		break;
		}
	    if (++hp == hpend)
		hp = hashsymtab;
	    }
	}

	}
}  /* rehash_symtab */

/*****************************************************************************
 *
 *  SYMTAB_INSERT()
 *
 *  Description:	Insert a variable description record into the symbol
 *			table
 *
 *  Called by:		main()	-- c1.c
 *
 *  Input Parameters:	p -- node describing sym
 *			attr -- attribute info from C1OREG/C1NAME record
 *			name -- user-declared name for sym
 *			in_procedure -- YES iff var declared inside proc.
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	hiddenfargchain
 *			symtab
 *
 *  Globals Modified:	f_find_array
 *			f_find_common
 *			f_find_extern
 *			f_find_struct
 *   			f_definite_insert
 *			hiddenfargchain
 *
 *  External Calls:	add_comelement()
 *			add_common_region()
 *			add_farg()
 *			find()
 *
 *****************************************************************************
 */

void symtab_insert(p,attr,name,in_procedure,arraysize)
NODE *p;
SYMTABATTR attr;
char *name;
flag in_procedure;
int arraysize;
{
    unsigned loc;
    unsigned comloc;
    register HASHU *np;
    register HASHU *nnp;
    long value;
    TTYPE type;
    flag structflag = NO;

#ifdef COUNTING
    _n_symtab_infos++;
#endif
	allow_insertion = YES;
    f_definite_insert = YES;    /* flag to look_harder() this is definitely
				 * a symbol table insertion
				 */

    f_find_pass1_symtab = YES;

    if (in_procedure == NO)
	f_find_extern = YES;		/* reset at exit of this procedure */

    if (attr.a.common)
	{
	value = p->tn.lval;
	p->tn.lval = 0;
	f_find_common = YES;  /* flag to find()*/
	comloc = find(p);
	f_find_common = NO;
	p->tn.lval = value;
	add_common_region(comloc);	/* insert common region into symtab */
	}

    if ((attr.a.array) || ISARY(p->tn.type))
	{
	attr.a.array = YES;
	type = decref(p->tn.type);

	while (ISARY(type))
	    type = decref(type);

	if (((type.base & 0xff) == STRTY)
         || ((type.base & 0xff) == UNIONTY))
	    goto structbase;
	f_find_array = YES;  /* flag to find()*/
	loc = find(p);
	f_find_array = NO;
	}
    else if (((p->tn.type.base & 0xff) == STRTY)
          || ((p->tn.type.base & 0xff) == UNIONTY))
	{
structbase:
	f_find_struct = YES;
	loc = find(p);
	f_find_struct = NO;
	structflag = YES;
	}
    else
	{
	loc = find(p);
	}

    if (attr.a.common)
	add_comelement(comloc,loc);

    np = symtab[loc];
    np->a6n.farg = attr.a.farg;
    np->a6n.common = attr.a.common;

    np->a6n.equiv = attr.a.equiv || attr.a.isvolatile
			|| ((p->tn.type.base & 0xff) == UNIONTY);
    np->a6n.array = attr.a.array || ISARY(p->tn.type);
    np->a6n.ptr = attr.a.ptr;
    np->a6n.func = attr.a.func;
    np->a6n.arrayelem = NO;
    np->a6n.complex1 = attr.a.complex;
    np->a6n.complex2 = NO;
    np->a6n.hiddenfarg = attr.a.hiddenfarg;
    np->a6n.inregister = attr.a.inregister;
    np->a6n.isexternal = attr.a.isexternal || f_find_extern;
    np->a6n.type = p->tn.type;
    /* To allow for equivalenced arrays, take the largest in case a symbol
     * has already been inserted at this place in the symbol table. When
     * a symbol table entry is created initially the fields are zeroed.
     * Account for this by not considering the existing value if it is zero.
     */
    if ((np->a6n.array_size == 0) || 
        (np->a6n.array_size < arraysize))
      np->a6n.array_size = arraysize;
    np->a6n.wholearrayno = NO_ARRAY;
    np->a6n.pass1name = name;
    np->a6n.isconst = NO;
    np->a6n.isstruct = structflag;
    np->a6n.isc0temp = attr.a.isc0temp;

    if (attr.a.hiddenfarg)
	{
	np->a6n.nexthiddenfarg = hiddenfargchain;
	hiddenfargchain = loc;
	}

    if (attr.a.common)
	make_com_entry(np);

    if (np->a6n.complex1 && !np->a6n.farg
     && !np->a6n.array && !np->a6n.ptr
     && !np->a6n.func)
	{
	char xname[300];

	/* insert 2nd half of complex into
	   symtab, too
	*/
	p->tn.lval += (BTYPE(np->a6n.type) == FLOAT) ? 4 : 8;
	loc = find(p);
	np->a6n.back_half = loc;

	if (np->a6n.common)
	    add_comelement(comloc,loc);

	nnp = symtab[loc];
	nnp->a6n.type = np->a6n.type;
	nnp->a6n.wholearrayno =
			np->a6n.wholearrayno;
	nnp->allo.attributes =
			np->allo.attributes;
	nnp->a6n.complex1 = NO;
	nnp->a6n.complex2 = YES;
	sprintf(xname, "%s+%d", np->a6n.pass1name,
		(BTYPE(np->a6n.type) == FLOAT) ? 4 : 8);
	nnp->a6n.pass1name = addtreeasciz(xname);
	}

    if (attr.a.farg)
	{
	if (attr.a.array || structflag) 
	    {
	    comloc = find(p);
	    nnp = symtab[comloc];
	    nnp->allo.attributes = np->allo.attributes;
	    nnp->a6n.farg = NO;
	    nnp->a6n.pass1name = np->a6n.pass1name;
	    nnp->a6n.type = np->a6n.type;
	    nnp->a6n.ptr = YES;
	    nnp->a6n.wholearrayno = loc;
	    }
	add_farg(loc);	/* for arrays, this is the X6N record */
	}

    f_find_extern = NO;
    f_find_pass1_symtab = NO;
    f_definite_insert = NO;
	allow_insertion = NO;
}  /* symtab_insert */

#ifdef DEBUGGING
LOCAL void dump_com_entries()
{
COMM_NAME *com_list = com_equiv_check;

while (com_list)
  {

  COMM_EQUIV_FIELD  *field = com_list->first_field;
  
  fprintf(debugp,"COMMON: %s\n",com_list->name);

  while (field)
    {
    fprintf(debugp,"\t %s(%d:%d)\n", field->sp->cn.pass1name,
                   field->sp->cn.offset,compute_size(field->sp));
    field = field->next_field;
    }

  com_list = com_list->next_name;
  }
}

void dumpdublock(dp)
register DUBLOCK *dp;
{
    if (in_reg_alloc_flag)
        fprintf(debugp, "\t    %s  %s  stmt=%d  sav=%d  defno=%d\n  definite=%s\n",
	    (dp->isdef ? "DEF" : "USE"), (dp->ismemory ? "MEM" : "REG"),
	    dp->stmtno, dp->savings, dp->defsetno,
	    (dp->isdefinite ? "YES" : "NO"));
    else
        fprintf(debugp, "\t    %s  %s  stmt=%d  defno=%d  definite=%s  parent=0x%.8x  deleted=%s\n",
	    (dp->isdef ? "DEF" : "USE"), (dp->ismemory ? "MEM" : "REG"),
	    dp->stmtno, dp->defsetno,
	    (dp->isdefinite ? "YES" : "NO"), dp->d.parent,
	    (dp->deleted ? "YES" : "NO"));
}  /* dumpdublock */



void dumpsym(index)
long index;
{
    register HASHU *np = symtab[index];
    register CLINK *cp;
    long modifiers;

    fprintf(debugp,"\n    symtab[%d]:  ", index);
    switch (np->a6n.tag)
	{
	case A6N:	fprintf(debugp, "A6N  %d(%%a6),", np->a6n.offset);
			break;

	case X6N:	fprintf(debugp, "X6N  %d(%%a6),", np->x6n.offset);
			break;

	case S6N:	fprintf(debugp, "S6N  %d(%%a6),", np->x6n.offset);
			break;

	case AN:	if (np->an.ap)
			    fprintf(debugp, "AN  %s+%d,", np->an.ap,
							 np->an.offset);
			else
			    fprintf(debugp, "AN  \"%d\",", np->an.offset);
			break;

	case XN:	fprintf(debugp, "XN  %s+%d,", np->xn.ap, np->xn.offset);
			break;

	case SN:	fprintf(debugp, "SN  %s+%d,", np->xn.ap, np->xn.offset);
			break;

	case CN:	fprintf(debugp, "CN  %s+%d,", np->cn.ap, np->cn.offset);
			break;

	case ON:	fprintf(debugp, "ON  (%d),", np->on.regval);
			break;
	}

    modifiers = np->a6n.type.base & 0xfc0;
    if (TMODS1(np->a6n.type.base))
	modifiers |= np->a6n.type.mods1 & 0xfffff000;

    while (modifiers != 0)
	{
	switch (modifiers & TMASK)
	    {
	    case PTR:
			fprintf(debugp, " PTR");
			break;
	    case FTN:
			fprintf(debugp, " FTN");
			break;
	    case ARY:
			fprintf(debugp, " ARY");
			break;
	    }
	modifiers >>= 2;
	}

    switch (BTYPE(np->a6n.type))
	{
	case UNDEF:	fprintf(debugp, " UNDEF");
			break;

	case FARG:	fprintf(debugp, " FARG");
			break;

	case CHAR:	fprintf(debugp, " CHAR");
			break;

	case SHORT:	fprintf(debugp, " SHORT");
			break;

	case INT:	fprintf(debugp, " INT");
			break;

	case LONG:	fprintf(debugp, " LONG");
			break;

	case FLOAT:	fprintf(debugp, " FLOAT");
			break;

	case DOUBLE:	fprintf(debugp, " DOUBLE");
			break;

	case STRTY:	fprintf(debugp, " STRTY");
			break;

	case UNIONTY:	fprintf(debugp, " UNIONTY");
			break;

	case ENUMTY:	fprintf(debugp, " ENUMTY");
			break;

	case MOETY:	fprintf(debugp, " MOETY");
			break;

	case UCHAR:	fprintf(debugp, " UCHAR");
			break;

	case USHORT:	fprintf(debugp, " USHORT");
			break;

	case UNSIGNED:	fprintf(debugp, " UNSIGNED");
			break;

	case ULONG:	fprintf(debugp, " ULONG");
			break;

	case VOID:	fprintf(debugp, " VOID");
			break;

	case LONGDOUBLE: fprintf(debugp, " LONGDOUBLE");
			break;

	case SCHAR:	fprintf(debugp, " SCHAR");
			break;

	case LABTY:	fprintf(debugp, " LABTY");
			break;

	case TNULL:	fprintf(debugp, " TNULL");
			break;

	case SIGNED:	fprintf(debugp, " SIGNED");
			break;

	case CONST:	fprintf(debugp, " CONST");
			break;

	case VOLATILE:	fprintf(debugp, " VOLATILE");
			break;

	default:	fprintf(debugp, " unknown(%d,0x%,0x%x)",
				np->a6n.type.base,np->a6n.type.mods1,
				np->a6n.type.mods2);
			break;
	}

    fprintf(debugp, "\n\tattr = [");
    if (np->a6n.farg)
        fprintf(debugp, "farg,");
    if (np->a6n.common)
        fprintf(debugp, "common,");
    if (np->a6n.equiv)
        fprintf(debugp, "equiv/volatile,");
    if (np->a6n.ptr)
        fprintf(debugp, "ptr,");
    if (np->a6n.array)
        fprintf(debugp, "array,");
    if (np->a6n.func)
        fprintf(debugp, "func,");
    if (np->a6n.arrayelem)
        fprintf(debugp, "arrayelem,");
    if (np->a6n.complex1)
        fprintf(debugp, "complex1,");
    if (np->a6n.complex2)
        fprintf(debugp, "complex2,");
    if (np->a6n.hiddenfarg)
        fprintf(debugp, "hiddenfarg,");
    if (np->a6n.inregister)
        fprintf(debugp, "inregister,");
    if (np->a6n.isstruct)
        fprintf(debugp, "isstruct,");
    if (np->a6n.isexternal)
        fprintf(debugp, "isexternal,");
    if (np->a6n.isconst)
        fprintf(debugp, "isconst,");
    if (np->a6n.dbra_index)
        fprintf(debugp, "dbra_index,");
    if (np->a6n.no_effects)
        fprintf(debugp, "no_effects");
    fprintf(debugp, "]\n");

    fprintf(debugp, "\t\"%s\", ", np->a6n.pass1name ? np->a6n.pass1name : "");

    fprintf(debugp, "wholearrayno = %d\n", np->a6n.wholearrayno);

    if (sdebug > 1)
	{
	if (np->a6n.tag == CN)
	    {
	    fprintf(debugp, "\tCOMMON MEMBERS:\n");
	    for (cp = np->cn.member; cp; cp = cp->next)
		{
		fprintf(debugp, "\t\t%d\n", cp->val);
		}
	    }
	if (np->a6n.tag == XN || np->a6n.tag == X6N)
	    {
	    fprintf(debugp, "\tARRAY MEMBERS:\n");
	    for (cp = np->xn.member; cp; cp = cp->next)
		{
		fprintf(debugp, "\t\t%d\n", cp->val);
		}
	    }
	if (np->a6n.tag == SN || np->a6n.tag == S6N)
	    {
	    fprintf(debugp, "\tSTRUCT MEMBERS:\n");
	    for (cp = np->xn.member; cp; cp = cp->next)
		{
		fprintf(debugp, "\t\t%d\n", cp->val);
		}
	    }
	}

    if (rudebug || qudebug)	/* dump def-use chains */
	{
	register DUBLOCK *dp;
	register DUBLOCK *enddp;
	fprintf(debugp, "\tDEF-USE CHAIN:\n");
	if (np->a6n.du != NULL)
	    {
	    enddp = dp = np->a6n.du->next;
	    do
		{
	        dumpdublock(dp);
		dp = dp->next;
		}
	    while (dp != enddp);
	    }
	}

     if (gdebug>2 && np->a6n.defset)
	printset(np->a6n.defset);
}



void dumpsymtab()
{
    register long i;

    fprintf(debugp,"\nSymbol table:\n");

    for (i = 0; i <= lastfilledsym; i++)
	{
	dumpsym(i);
	}

    if (sdebug > 1)
	{
	fprintf(debugp,"\n");
	for (i = 0; i <= lastcom; i++)
	    {
	    fprintf(debugp,"    comtab[%d] = %d\n", i, comtab[i]);
	    }

	fprintf(debugp,"\n");
	for (i = 0; i <= lastfarg; i++)
	    {
	    fprintf(debugp,"    fargtab[%d] = %d\n", i, fargtab[i]);
	    }

	fprintf(debugp,"\n");
	for (i = 0; i <= lastptr; i++)
	    {
	    fprintf(debugp,"    ptrtab[%d] = %d\n", i, ptrtab[i]);
	    }
	}
}
#endif DEBUGGING



