/* @(#) $Revision: 70.1 $ */    

#include <stdio.h>
#include <string.h>
#include "symbols.h"

# ifdef SEQ68K
# include "seq.a.out.h"
# else
# include <a.out.h>
# endif

#include "adrmode.h"
#include "sdopt.h"

/*
 *	"symbols.c" is a file containing functions for accessing the
 *	symbol tables. 
 *	The following functions are provided:
 *
 *	Symbol entry and lookup routines:
 *
 *	newent(string)
 *		Creates a new symbol table entry for the symbol with name
 *		"string".  The type of the symbol is set to undefined.
 *		All other fields of the entry are set to zero.
 *
 *	hash(string)
 *		Use the first HASHL characters of "string" to
 *		calculate an integer-valued hash function.  Used
 *		for symbol table lookup.
 *
 *	lookup(string,install_flag,tag)
 *		Looks up the symbol whose name is "string" in the symbol
 *		table and returns a pointer to the entry for that symbol.
 *		"Hash" defines the hashing algorithm, and collisions are
 *		handled by chaining (at the head of the list).
 *		Tag is used to determine what hashtable to search from.
 *		Currently, two lookup hash tables are maintained
 *		one for the instruction names, and one for regnames,
 *		special syms, and user symbols.
 *		Install_flag is INSTALL if symbol is to be installed,
 *		N_INSTALL if it is only to be looked up.
 *
 *	addstr(string)
 *		Enters the "string" into the string table.  Called by
 *		newent() and returns a char * pointer where the string
 *		was stored.  Space for the string table is initially
 *		allocated as array strtab0[].  If "string" would exceed
 *		the available space, then additional string table space
 *		is malloc()-ed.
 *		The current strategy uses a fragmented string table,
 *		wasting space at the end of each fragment, but saving the
 *		time required to copy the table if realloc() were used.
 *		(Note that a realloc strategy would require using malloc
 *		to allocate the initial string table.)
 *		** this requires that symbol entries store pointers to
 *		the name strings, not offsets into the string table.
 *
 *	Symbol table initilization routines:
 *
 *	symtables_init()
 *		Main entry point for initialization; calls all the individual
 *		initialization routines.
 *
 *	hash_inst_mnemonics()
 *		Link the instruction mnemonics found in instab[] into
 *		the symbol table hash chains.
 *	hash_regsymbols()
 *		Link the register symbol names into the hash chains.
 *	hash_special_symbols()
 *		Link the predefined special names (size suffixes) into
 *		the hash chains.
 *
 *
 *	Post-Pass1 Symbol Processing Routines:
 *	
 *	fixstab(addr,incr,type)	This  function  "fixes" elements of the
 *			symbol table by adding  "incr"  to  all  values
 *			of symbols of type "type" that are greater than
 *			"addr".   This  is necessary when the length of
 *			of an instruction is increased.
 *			**** NOT CURRENTLY USED. Will need for sdi later.
 *
 *	fixsyms()  Fixup the values of symbols of type SDATA and SBSS
 *			by adding "txtsiz" to every SDATA symbol, and
 *			"txtsiz + datsiz" to every SBSS symbol.
 *		   This is a "relic" from the old as/ld interface: where
 *			a .s file with no external references was assembled 
 *			into an a.out file that could be executed directly
 *			without linking. This is now just a waste of time;
 *			the linker is going to redo all this anyway.
 *			The as/ld interface should be redefined, but
 *			this would break all existing .o files.
 *		Also, mark any undefined symbol as SEXTERN, so it will
 *		go into the symbol table, and count the number of entries
 *		that will be going into the linker symbol table, and the
 *		space that will be needed for the linker symbol table.
 *
 *	dump_linker_symtab()
 *		Generate the linker symbol table entries.
 *
 *	linker_sym_type(type)
 *		Map the assembler's internal symbol types to linker symbol
 *		types.
 */

/* allocate space for the hash tables, and initialize the structures that
 * point to them.
 */
upsymins shashtab[NSHASH];	/* for user symbols */
upsymins ihashtab[NIHASH];	/* for instructions, predefined names */
static hashtable  shash_table = {NSHASH, shashtab};
static hashtable  ihash_table = {NIHASH, ihashtab};

char	*malloc();
char	*addstr();

long	symcnt = 0;
struct	usersym_blk usersyms0;
struct	usersym_blk * first_usersym_blk = &usersyms0;
struct	usersym_blk * cur_usersym_blk = &usersyms0;

#ifdef PIC
extern short pic_flag;
extern short shlib_flag;
long supsize;
#endif



/*******************************************************************************
 * newent
 *	Create a new symbol table entry and return a pointer to the new
 *	entry.
 *	Save the name in the string table.
 *	Initialize the type to SUNDEF.  Set all the other fields to "null"
 *	values.
 *	Symbol table overflow is detected as a fatal error.
 */

symbol *
newent(strptr)
	register char *strptr;
{
	register symbol *symptr;
	register char *ptr1;

	if (cur_usersym_blk->symblkcnt >= NSYMS_PER_BLK) {
	   /* try to allocate a new symbol block */
	   if ((cur_usersym_blk->next_usersym_blk =
	     (struct usersym_blk *) malloc(sizeof(struct usersym_blk))) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("unable to allocate more space for symbol table");
	   cur_usersym_blk = cur_usersym_blk->next_usersym_blk;
	   cur_usersym_blk->next_usersym_blk = NULL;
	   cur_usersym_blk->symblkcnt = 0;
	   }

	symcnt++;
	symptr = &(cur_usersym_blk->usersym[cur_usersym_blk->symblkcnt++]);
	symptr->snamep = addstr(strptr);
	
	symptr->hashlink = NULL;
	symptr->stype = SUNDEF;
	symptr->svalue = 0L;
	symptr->stag = 0;
	symptr->slink = NULL;
	symptr->slstindex = -1;
	symptr->salmod = 0;
	symptr->ssdiinfo = 0;
#ifdef PIC
	symptr->got = 0;
	symptr->plt = 0;
	symptr->ext = 0;
	symptr->pc = 0;
	symptr->endoflist = 1;
	symptr->internal = 0;
	symptr->size = 0;
#endif
	return(symptr);
}  /* newent */

#if DEBUG

unsigned numcalls,	/* number of calls to lookup */
	numids,		/* number of ids installed */
	numcoll;	/* number of collisions */
#endif


/******************************************************************************
 * hash
 *	Hashing routine for Symbol and Instruction hash tables.
 *	HASHL is the maximum number of chars used in computing the hash value.
 */

# define HASHL  8

long
hash(htp,s)
	hashtable * htp;
	register unsigned char *s;
{
	register int i;
	register int j;

	for ( i=0, j=0; *s; s++ ) {
	    i = (i<<2) + *s;
	    if (++j >= HASHL) break;
	}
	/* This will not have overflowed since HASHL is small relative to
	 * the size of an int.
	 */
	 return( i % htp->htsize);
}  /* hash */


/*******************************************************************************
 * lookup
 *	Lookup symbol with name "string".  Tag is used to determine which symbol
 *	table to search. Currently two hash tables are maintained: one for
 *	user symbols, and one for all the predefined symbols (instrunction
 *	names, register names, size suffixes.
 *	"Install" tells whether to create a new symbol entry if the name
 *	is not found.
 *	A pointer is returned to the matching symbol table entry if found
 *	or created, else NULL is returned.
 *
 *	The HASHDEBUG flag turns on code to print info about the number
 *	of links followed in a search: useful for debugging/tuning a
 *	hash function.
 *	DEBUG turns on statical counts for lookup, printed at end of 
 *	assembly.
 */

usymins *
lookup(sptr,install,tag)
	char *sptr;
	int install;
	int tag;
{
	long hval;
	register hashed_symbol * hsp;
	register hashtable * htp;
# if HASHDEBUG
	int count;
# endif
# if DEBUG
	numcalls++;
# endif

	/* the hash table to use will depend on the tag. */
	switch(tag) {
		case MNEMON:
		case REGSYM:
			htp = &ihash_table;
			break;

		case SPSYM:
		default:
			htp = &shash_table;
			break;
		}

	/* calculate the hash index */
	hval = hash(htp,sptr);

	/* set up pointer 'hsp' to traverse the correct chain */
	hsp = htp->htchain[hval].hsp;
# if HASHDEBUG
	count = 0;
# endif
	while (hsp != NULL) {
		/* see if the name matches. */
# if HASHDEBUG
		count++;
# endif
		if (strcmp(hsp->namep,sptr) == 0) {
			/* found it */
# if HASHDEBUG
			printf("%d	%d	%s\n",count,hval,sptr);
# endif
			return((usymins *) hsp);
			}
		else {
			/* move down chain */
# if DEBUG
			numcoll++;
# endif
			hsp = hsp->hashlink.hsp;
			}

		}

	/* if reach here, no match was found: based on "install" flag,
	 * we either created a new symbol or just return NULL.
	 */
# ifdef HASHDEBUG
	 printf("%d	%d	%s\n",count,hval,sptr);
# endif
	if (install == INSTALL) {
		register symbol * newsymp;
# if DEBUG
		numids++;
# endif
		newsymp = newent(sptr);
		newsymp->hashlink = htp->htchain[hval].stp;
		htp->htchain[hval].stp = newsymp;
		return ( (usymins *) newsymp );
		}
	else
		return(NULL);
	
}  /* lookup */


/*****************************************************************************
 * Defines and variable allocations for the string table.
 *	STRTABSIZE0	- size of the initial string table.
 *	STRTABSIZE_INCR	- number of bytes to allocate when string table
 *			  overflows.
 *	strtab0[]	- the initial string table.
 *	strtabsize	- the size of the current string table fragment.
 *	strtab		- pointer to the base of the current string table
 *			  fragment.
 *	strtabindex	- index (off strtab) of the next free location in
 *			  the current string table fragment.
 *
 * Note: these variables are currently declared as "static" because they
 *	do not need to be accessed outside this file.  However, that does
 *	cause zeros in the executable file, so might want to consider
 *	removing the "static".
 */

# define STRTABSIZE0	4096
# define STRTABSIZE_INCR	4096
static long strtabsize =  STRTABSIZE0;
char strtab0[STRTABSIZE0];
static char	*strtab = strtab0;
static long	strtabindex = 0;


/*****************************************************************************
 * addstr
 *	Add a string to the string table, returning a pointer to the
 *	head of the string.
 *	If the string will not fit in the current string table fragment,
 *	attempt to allocate a new string table fragment of size
 *	STRTABSIZE_INCR.
 */

char *
addstr(strptr)
	char	*strptr;
{
	register int	length;
	char * strtab_ptr;

	length = strlen(strptr);
	if (length + strtabindex >= strtabsize) {
		/* the string won't fit in the remaining space of this
		 * string table fragment.  Try to allocate a new
		 * string table fragment.
		 */
# if DEBUG
		printf("string table overflow. slength = %d waste = %d\n",
		  length, strtabsize-strtabindex);
# endif
		if ((strtab = malloc(strtabsize = STRTABSIZE_INCR)) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			aerror("cannot malloc more space for string table");
		strtabindex = 0;
		}
		
	strcpy(strtab_ptr = &strtab[strtabindex],strptr); /* includes the null*/
	strtabindex += length + 1;
	return(strtab_ptr);
}	/* addstr(strptr) */




/*******************************************************************************
 * Initialization routines for the hash and symbol tables 
 */

symtables_init()
{
# ifdef DEBUG
	hashtables_init();
# endif
	hash_inst_mnemonics();
	hash_regsymbols();
	hash_special_symbols();
}

/* don't really need this initialization, since the gloabal space will
 * initialize to zeroes anyway.
 */
# if DEBUG
hashtables_init()
{
	register upsymins * hp, *limit;

	limit = &shash_table.htchain[NSHASH];
	for (hp = &shash_table.htchain[0]; hp<limit; hp++)
		hp->hsp = NULL;

	limit = &ihash_table.htchain[NIHASH];
	for (hp = &ihash_table.htchain[0]; hp<limit; hp++)
		hp->hsp = NULL;
}
# endif


extern instr instab[];

hash_inst_mnemonics()
{
	register instr *ip;
        register usymins * hp;
	register long 	hval;

	for (ip = instab; ip->inamep != '\0'; ++ip) {
#if DEBUG
		hp = lookup(ip->inamep,N_INSTALL,MNEMON);
		if (hp != NULL)
			aerror("Duplicate instruction table name (%s)", ip->inamep);

#endif
		hval = hash(&ihash_table,ip->inamep);
		ip->hashlink = ihash_table.htchain[hval].itp;
		ihash_table.htchain[hval].itp = ip;
		/* printf("%d	%s\n",hval,ip->inamep);*/
	} /* for */
}

extern regsymbol regsym[];

hash_regsymbols()
{
	register regsymbol *rp;
        register usymins * hp;
	register long 	hval;

	for (rp = regsym; rp->rnamep != '\0'; ++rp) {
#if DEBUG
		hp = lookup(rp->rnamep,N_INSTALL,REGSYM);
		if (hp != NULL)
			aerror("Duplicate register symbol name (%s)", rp->rnamep);

#endif
		hval = hash(&ihash_table,rp->rnamep);
		rp->hashlink = (symbol *) ihash_table.htchain[hval].rsp;
		ihash_table.htchain[hval].rsp = rp;
	} /* for */
}

extern symbol spsym[];

hash_special_symbols()
{
	register symbol *sp;
        register usymins * hp;
	register long 	hval;

	for (sp = spsym; sp->snamep != '\0'; ++sp) {
#if DEBUG
		hp = lookup(sp->snamep,N_INSTALL,SPSYM);
		if (hp != NULL)
			aerror("Duplicate special symbol name (%s)", sp->snamep);

#endif
		hval = hash(&shash_table,sp->snamep);
		sp->hashlink = shash_table.htchain[hval].stp;
		shash_table.htchain[hval].stp = sp;
	} /* for */
}

extern symbol usersym[];

hash_predef_symbols()
{
	register symbol *sp;
        register usymins * hp;
	register long 	hval;

	for (sp = usersym; sp->snamep != '\0'; ++sp) {
		if (hp = lookup(sp->snamep,N_INSTALL,USRNAME)) {
			hp->usersym.stag = sp->stag;
			hp->usersym.stype &= ~STYPE;
			hp->usersym.stype |= sp->stype;
			hp->usersym.svalue = sp->svalue;
		}
	} /* for */
}


/***************************************************************************
 * Post-Pass1 Symbol Processing Rountines **********************************
 */

# if FUTURE
 /* conditionally remove for now */
/***************************************************************************
 * fixstab
 *	"fix" elements of the symbol table by adding "incr" to the value
 *	of all symbols of type "type" with a value greater than "addr".
 */

fixstab(addr,incr,type)
	long addr;
	long incr;
	short type;
{
  register symbol * sym;
  register long i;
  register struct usersym_blk * symblk;
  
  for (symblk = first_usersym_blk; symblk!=NULL; symblk=symblk->next_usersym_blk)
     {
  	for(sym = &symblk->usersym[0], i=symblk->symblkcnt; i>0;  sym++, i--) {
	   if (((sym->stype & STYPE) == type) &&
		(sym->svalue >= addr))
		sym->svalue += incr;
	   }
     }
}
# endif

/*****************************************************************************
 * fixsyms
 *	Fixup the values of symbols of type SDATA and SBSS by adding
 *	"txtsiz" to every SDATA symbol, and "txtsiz + datsiz" to every
 *	SBSS symbol.
 *	NOTE: When span-dependent optimization is on, the value for each STEXT 
 *	label symbol was updated at the end of the span-dependent pass
 *	(in sdo_cleanup).
 *	Also, mark any undefined symbol as SEXTERN, so it will go into
 *	the symbol table, and count the number of entries that will be
 *	going into the linker symbol table, and the space that will be
 *	needed for the linker symbol table.
 *	If the -L option was used, put all symbols into LST, even the
 *	locals.
 *	The strategy for how many symbols should be written to the linker
 *	symbol table is defined here.  Currently, only symbols with the
 *	SEXTERN bit set go into the LST.
 */
extern long txtsiz, datsiz;
extern long lstcnt;
extern long lstsiz;
extern short Lflag;

/* NOTE: for a cross assembler will have to set this to a constant.  And may
 * need to define local structures for those from s300 a.out.h to get
 * correct sizes and alignments.
 */
# ifdef SEQ68K
# define NLISTSZ sizeof(struct sym)
# else
# define NLISTSZ /* sizeof(struct nlist_) */  10
# endif

fixsyms()
{
  register symbol * sym;
  register struct usersym_blk * symblk;
  register long i;
  register txtdatsiz = txtsiz + datsiz;
  register Lflag1 = (Lflag==1);
  register Lflag2 = (Lflag==2);

  lstcnt = 0;
  lstsiz = 0;

  for (symblk = first_usersym_blk; symblk!=NULL; symblk=symblk->next_usersym_blk)
     {
  	for(sym = &symblk->usersym[0], i=symblk->symblkcnt; i>0;  sym++, i--) {
	   switch(sym->stype & STYPE) {
		default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			aerror("unknown type in fixsyms");
			break;
		case SABS:
			break;
		case STEXT:
			break;

		case SGNTT:  case SLNTT:  case SSLT:  case SVT:  case SXT:
		       /* never put these into LST */
			continue;
		case SDATA:
			sym->svalue += txtsiz;
			break;
		case SBSS:
			sym->svalue += txtdatsiz;
			break;
		case SUNDEF:
			sym->stype |= SEXTERN;
			break;
		}

	   /* Mark symbols that go into LST:
	    *	 all external symbols always go into LST
	    *	 if Lflag==1, all symbols go into LST
	    *	 if Lflag==2, all symbols that don't start with 'L'
	    */

#ifdef PIC
	   if (Lflag1 || (sym->stype & (SEXTERN|SEXTERN2)) ||
	       (Lflag2 && (*(sym->snamep)!='L') && (*(sym->snamep)!='.')) ||
	       ( pic_flag && (sym->got || sym->plt || sym->pc || sym->ext) 
	       && (sym->stype != SABS))) {
#else
	   if (Lflag1 || (sym->stype & (SEXTERN|SEXTERN2)) ||
	       (Lflag2 && (*(sym->snamep)!='L') && (*(sym->snamep)!='.')) ) {
#endif
	    sym->slstindex = lstcnt++;
#ifdef SEQ68K
	    lstsiz += NLISTSZ + strlen(sym->snamep) + 1;
#else
	    lstsiz += NLISTSZ + strlen(sym->snamep);
#endif
	    }
	}
     }
#ifdef PIC
     if (shlib_flag) {
        supsize = lstcnt * sizeof(struct supsym_entry);
     }
     else {
        supsize = 0;
     }
#endif
}


/*****************************************************************************
 * dump_linker_symtab
 *	Traverse the user symbol table, generating linker symbol table
 *	entries.  A symbol should be dumped if it slstindex field was
 *	set to a positive value ( in fixsyms ).
 */

#ifdef SEQ68K
struct sym nlist;
#else
struct nlist_ nlist;
#endif
unsigned linker_sym_type();

extern FILE * fdout;

dump_linker_symtab() {
  register symbol * sym;
  register struct usersym_blk * symblk;
  register long i;
  register long symlen;
#ifdef SEQ68K
  int namelength;
#endif

  /* cnt, siz used for integrity checks */
  long siz = 0, cnt = 0;

  for (symblk = first_usersym_blk; symblk!=NULL; symblk=symblk->next_usersym_blk)
     {
  	for(sym = &symblk->usersym[0], i=symblk->symblkcnt; i>0;  sym++, i--) {
	   if (sym->slstindex != -1) {
#ifdef SEQ68K
		nlist.stype  = linker_sym_type(sym->stype);
		nlist.sother = 0;
		nlist.sdesc  = 0;
		nlist.svalue = sym->svalue;
		namelength = strlen(sym->snamep) + 1;

		fwrite(&nlist, NLISTSZ, 1, fdout);
		fwrite(sym->snamep, namelength, 1, fdout);
#else
		nlist.n_value = sym->svalue;
		nlist.n_type  = linker_sym_type(sym->stype);
		nlist.n_length = strlen(sym->snamep);
		nlist.n_almod =  sym->salmod;
		nlist.n_unused = 0;
#ifdef PIC
		nlist.n_dlt = sym->got;
		nlist.n_plt = sym->plt;
		nlist.n_dreloc = sym->ext;
		/*
		 * while this symbol points to a symbol that is not
		 * going to be dumped (has an sltstindex of -1) then
		 * change the current symbol to contain the info
		 * of the symbol it points to
		 */
		while (!sym->endoflist &&
	            (((symbol *) sym->size)->slstindex == -1)) {
		    sym->endoflist = ((symbol *) sym->size)->endoflist;
		    sym->size = ((symbol *) sym->size)->size;
		    if (!sym->endoflist && !sym->size) {
			aerror("supsym list error in Pass 2");
		    }
		}
		nlist.n_list = sym->endoflist;
#endif

		fwrite(&nlist, NLISTSZ, 1, fdout);
		fwrite(sym->snamep, nlist.n_length, 1, fdout);
#endif

		if (cnt != sym->slstindex)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			aerror("lst-indexing error in Pass 2");
		cnt++;
#ifdef SEQ68K
		siz += NLISTSZ + namelength;
#else
		siz += NLISTSZ + nlist.n_length;
#endif
		}
	}
     }
  if ( (cnt!=lstcnt) || (siz != lstsiz) )
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("lstsize error in Pass 2");
}

#ifndef SEQ68K
dump_symbol_sizes()
{
   register symbol *sym;
   register struct usersym_blk *symblk;
   register long  i;
   struct supsym_entry sup;
   sup.b = 0;
   for (symblk = first_usersym_blk; 
        symblk!=NULL; 
        symblk=symblk->next_usersym_blk) {
      for(sym = &symblk->usersym[0], i=symblk->symblkcnt; i > 0; sym++, i--) {
	 if (sym->slstindex != -1) {
	    if (sym->endoflist) {
	       sup.a.size = sym->size;
	    }
	    else {
	       sup.a.next = ((symbol *) sym->size)->slstindex;
	    }
            fwrite (&sup, sizeof(struct supsym_entry), 1, fdout);
         }
      }
   }
}
#endif


/**************************************************************************
 * linker_sym_type
 *	Map the assembler's internal symbol types to linker symbol types
 */
unsigned linker_sym_type(type)
  register unsigned long type;
{ register unsigned long ltype;

  switch(type&STYPE) {
	default:
		uerror("illegal symbol type for LST");
		ltype = 0;
		break;
	case SUNDEF:
		ltype = UNDEF;
		break;
	case SABS:
		ltype = ABS;
		break;
	case STEXT:
		ltype = TEXT;
		break;
	case SDATA:
		ltype = DATA;
		break;
	case SBSS:
		ltype = BSS;
		break;
	}

  if (type&SEXTERN) ltype |= EXTERN;
#ifndef SEQ68K
  if (type&SEXTERN2) ltype |= EXTERN2;
  if (type&SALIGN)  ltype |= ALIGN;
#endif

  return(ltype);
}
