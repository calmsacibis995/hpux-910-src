/* Internal RCS $Header: middle.c,v 70.6 92/05/08 13:31:30 ssa Exp $ */

/* @(#) $Revision: 70.6 $ */      

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

     Command:      Series 200 ld   ( HP -UX link editor )
     File   :      middle.c

     Purpose:      Performs inter-pass processing
		   - assigns common locations
		   - enters 'edata', 'etext' and 'end' in LST
		   - assigns external symbols their final value
		   - creates output file and other temporary files
		     for the final a.out file

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "ld.defs.h"

char *strrchr();

#ifdef ALIGN_DORIGIN_FIX
static int Dused = 0;
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* middle -	Finalize the symbol table by adding the origins of
		each csect to symbols.  Also define the end stuff,
		adjusting the boundaries of the csect depending on
		options.
*/

middle()
{
	register symp sp, sp_end;
	register int i, slot;
	register symp sp2;
	int d = 0, p = 1;
	int psize, gsize;
	char *c, *stralloc();

#ifdef ALIGN_DORIGIN_FIX
	if (dorigin)
		Dused = dorigin;
#endif
#define  FPA_LOC      0xFFF08000          /* Constant on the s300 */
	if( !rflag && (slot = slookup( "fpa_loc" )) != -1 )
	{
		if( (symtab[ slot ].s.n_type & FOURBITS) == UNDEF )
		{
			symtab[ slot ].s.n_value = FPA_LOC;
			symtab[ slot ].s.n_type = EXTERN|ABS;
		}
		else if( (symtab[ slot ].s.n_type & FOURBITS) != ABS ||
				 symtab[ slot ].s.n_value != FPA_LOC )
		{
#pragma		BBA_IGNORE
			error( e54, FPA_LOC );
		}
	}

#ifdef	STACK_ZERO
	if (!rflag && (slot = slookup("___stack_zero")) != -1)
	{
		if ((symtab[slot].s.n_type & FOURBITS) == UNDEF &&
		    symtab[slot].s.n_value == 0)
		{
			symtab[slot].s.n_value = STACK_ZERO;
			symtab[slot].s.n_type = EXTERN|ABS;
		}
	}
#endif

#ifdef	ELABORATOR
	elaborator_index = slookup(elaborator_name);
#endif

	if (entrypt && (entrypt->s.n_type & FOURBITS) == UNDEF && !entrypt->s.n_value)
		error(e7);

	if( !rflag )
	{
	if( shlib_level == SHLIB_NONE )
		symtab[slookup("__DYNAMIC")].s.n_type = EXTERN|ABS;

	/* Allocate the actual module linked list table.
	 */
	if( shlib_level == SHLIB_BUILD && mod_index )
	{
		module_table = (struct module_entry *) 
			           malloc( mod_index * sizeof( struct module_entry ) );
		memset( module_table, 0xff, mod_index * sizeof(struct module_entry) );
	}

#ifdef SHLIB_PATH
	if (shlib_level == SHLIB_A_OUT
	    && (slot = slookup("___dld_embed_path")) != -1
		&& embed_path != NULL)
	{
		strcpy((c=stralloc(strlen(embed_path)+1)),embed_path);
		symtab[slot].s.n_value = c - stringt;
	}
#endif /* SHLIB_PATH */

	/* Go through the symbol table and look up DLT and PLT and Export Entries.
	*/
	shlib_tables();

	if( shlib_level > SHLIB_NONE )
	{

#ifdef SHLIB_PATH
		if (shlib_level == SHLIB_A_OUT || shlib_level == SHLIB_BUILD)
		{
			symtab[slookup("___dld_shlib_path")].s.n_value = dsize;
			dsize += SHLIB_PATH_LEN;
		}
#endif /* SHLIB_PATH */

		/* DBOUND table */
		symtab[slookup("DBOUND")].s.n_value = dsize;
		if (shlib_level == SHLIB_BUILD)
			dsize += (mod_number + 3) & ~3;

		/* BOUND Table */
		symtab[slookup("BOUND")].s.n_value = dsize;
		if (shlib_level == SHLIB_BUILD )
			dsize += (bound_size = ((dltindex + pltindex + rimpindex) * sizeof( char ) + 3) & ~3);
	}

	/* Enter symbols DLT and PLT into the symbol table.
	 * 'middle_dlt' is the offset for all dlt indexes to be moved, so that
	 * the entire 64K area can be used (should help avoid the need for
	 * +Z instead of +z).  If you didn't do this, then only 32K would be
	 * available.  Define DONT_MOVE_MIDDLE_OF_DLT to override this.
	 */
#ifndef		DONT_MOVE_MIDDLE_OF_DLT
 	middle_dlt = ((abs_dltindex + dltindex + local_dlt) / 2) - abs_dltindex; 
#endif
	global_dlt = dltindex;

	gsize = (abs_dltindex+dltindex+local_dlt) * sizeof(struct dlt_entry);
	if( shlib_level == SHLIB_BUILD || shlib_level == SHLIB_DLD )
	{
		symtab[slot = slookup("DLT")].s.n_value = 
			dsize + ((abs_dltindex + middle_dlt) * sizeof( struct dlt_entry ));
		if( symtab[slot].dltindex >= 0 )
			DltSet( symtab[slot].dltindex, symtab[slot].s.n_value, DATA );
		dsize += gsize;
	}
	else
	{
		/* DLT's in a.out's go in the text segment - no need for dld */
		symtab[slot = slookup("DLT")].s.n_value = 
		    tsize + ((abs_dltindex + middle_dlt) * sizeof( struct dlt_entry ));
		if( symtab[slot].dltindex >= 0 )
			DltSet( symtab[slot].dltindex, symtab[slot].s.n_value, TEXT );
		symtab[slot].s.n_type = EXTERN|TEXT;
		tsize += gsize;
	}

	psize = sizeof(struct plt_entry) * pltindex;
	/* align PLT on 8 byte boundary */
	if (pltindex)
	{
		plt_align = ((dsize + 7) & ~07) - dsize;
		dsize += plt_align;
	}
	symtab[slookup("PLT")].s.n_value = dsize;
	dsize += psize;

	/* This is from above, since we now know the address of the item */
	if( shlib_level >= SHLIB_DLD )
	{
		symtab[slot = slookup("__DYNAMIC")].s.n_value = dsize;
		if( symtab[slot].expindex >= 0 )
		    ExpSet( symtab[slot].expindex, dsize, -1, DATA );
		if( symtab[slot].dltindex >= 0 )
			DltSet( symtab[slot].dltindex, dsize, DATA );
		dsize += sizeof(struct dynamic);
	}

#ifdef	HOOKS
	if ((slot = slookup("___dld_flags")) != -1)
	{
		unsigned long dynval;

		dynval = symtab[slookup("__DYNAMIC")].s.n_value;
		symtab[slot].s.n_value = dynval + offsetof(struct dynamic,dld_flags);
		slot = slookup("___dld_hook");
		symtab[slot].s.n_value = dynval + offsetof(struct dynamic,dld_hook);
		slot = slookup("___dld_list");
		symtab[slot].s.n_value = dynval + offsetof(struct dynamic,dld_list);
	}
#ifdef	BBA
	else
#pragma		BBA_IGNORE
		;
#endif
#endif

	/* Fill in the very first shlib entry to point to this file */
	if( shlib_level == SHLIB_A_OUT )
	{
		strcpy( (c = stralloc( 8 )), "<a.out>" );
		shlibs[ 0 ].name = c - stringt;
		shlibs[ 0 ].load = LOAD_PATH;
		shlibs[ 0 ].bind = Bflag;
		shlibs[ 0 ].highwater = 0;
	}
    }

	/*
	 * Assign common locations.
	 */
	csize = 0;
	if (rflag==0)
	{
		tsize = (tsize + 3) & ~03;
		dsize = (dsize + 3) & ~03;
		bsize = (bsize + 3) & ~03;

		if (Aname == 0)
		{
			/* Secondary Defs 
			 */
			slot = slookup("_etext");
			if( symtab[slot].s.n_value==0 && symtab[slot].s.n_type & EXTERN2 )
			{
				symtab[slot].s.n_value = tsize;
				if( symtab[slot].expindex >= 0 )
					ExpSet( symtab[slot].expindex, tsize, -1, TEXT );
			    if( symtab[slot].dltindex >= 0 )
				    DltSet( symtab[slot].dltindex, tsize, TEXT );
			}
			slot = slookup("_edata");
			if( symtab[slot].s.n_value==0 && symtab[slot].s.n_type & EXTERN2 )
			{
				symtab[slot].s.n_value = dsize;
				if( symtab[slot].expindex >= 0 )
					ExpSet( symtab[slot].expindex, dsize, -1, DATA );
			    if( symtab[slot].dltindex >= 0 )
				    DltSet( symtab[slot].dltindex, dsize, DATA );
			}
			slot = slookup("_end");
			if( symtab[slot].s.n_value==0 && symtab[slot].s.n_type & EXTERN2 )
			{
				symtab[slot].s.n_value = bsize;
				if( symtab[slot].expindex >= 0 )
					ExpSet( symtab[slot].expindex, bsize, -1, BSS );
			    if( symtab[slot].dltindex >= 0 )
				    DltSet( symtab[slot].dltindex, bsize, BSS );
			}

			/* Primary Defs
			 */
			if( symtab[slot = slookup("__etext")].s.n_type & EXTERN2 )
			{
				symtab[slot].s.n_value = tsize;
				symtab[slot].s.n_type &= ~EXTERN2;
			    if( symtab[slot].expindex >= 0 )
				    ExpSet( symtab[slot].expindex, tsize, -1, TEXT );
			    if( symtab[slot].dltindex >= 0 )
				    DltSet( symtab[slot].dltindex, tsize, TEXT );
			}
			else
				error( e52, "__etext" );

			if( symtab[ slot = slookup("__edata")].s.n_type & EXTERN2 )
			{
				symtab[slot].s.n_value = dsize;
				symtab[slot].s.n_type &= ~EXTERN2;
			    if( symtab[slot].expindex >= 0 )
				    ExpSet( symtab[slot].expindex, dsize, -1, DATA );
			    if( symtab[slot].dltindex >= 0 )
				    DltSet( symtab[slot].dltindex, dsize, DATA );
			}
			else
				error( e52, "__edata" );

			if( symtab[ slot = slookup("__end")].s.n_type & EXTERN2 )
			{
				symtab[slot].s.n_value = bsize;
				symtab[slot].s.n_type &= ~EXTERN2;
			    if( symtab[slot].expindex >= 0 )
				    ExpSet( symtab[slot].expindex, bsize, -1, BSS );
			    if( symtab[slot].dltindex >= 0 )
				    DltSet( symtab[slot].dltindex, bsize, BSS );
			}
			else
				error( e52, "__end" );
		}
		common();
	}
	else if (dflag) common();
	csize = (csize + 3) & ~03;

	/* Hash the export list */
	if( shlib_level > SHLIB_DLD )
		ExpHash();

	/* Align string table size */
	stringindex = (stringindex + 0x3) & (~0x3);

	if( shlib_level >= SHLIB_DLD )
	{
		/* Allocate space for the dynamic relocation records */
		relocs = (struct relocation_entry *)
				 calloc(relindex * sizeof(struct relocation_entry),1);

		shlibtextsize = sizeof(struct header_extension) +
			relindex * sizeof(struct relocation_entry);
		if( shlib_level > SHLIB_DLD )
		{
			shlibtextsize += (shlib_level == SHLIB_A_OUT ? 
						         (shlindex * sizeof(struct shl_entry)) : 0) +
				((shlib_level == SHLIB_BUILD ? dltindex : 0)
				            +pltindex+rimpindex)*sizeof(struct import_entry) +
				expindex * sizeof(struct export_entry) +
				(shlib_level == SHLIB_BUILD ? 
					     (expindex * sizeof(struct shl_export_entry)) : 0) + 
				ld_exphashsize * sizeof(struct hash_entry) +
				stringindex * sizeof(char) + 
				(shlib_level == SHLIB_BUILD ? 
				         (mod_index * sizeof(struct module_entry)) : 0);
			if (shlib_level == SHLIB_BUILD)
			{
				shlibtextsize += mod_number * sizeof(struct dmodule_entry);
				shlibtextsize += sizeof(struct dmodule_entry);
			}
		}
	}
	else 
		shlibtextsize = 0;
	shlibtextsize = (shlibtextsize + 3) & ~03;

#ifdef N_INCOMPLETE_FIX
	if (shlibtextsize % 8)
	{
		shlibtext_pad = ((shlibtextsize + 7) & ~07) - shlibtextsize;
		shlibtextsize += shlibtext_pad;
	}
#endif

	if (shlib_level == SHLIB_A_OUT )
		entryval = shlibtextsize;

    tsize += shlibtextsize;

	nund = 0;				/* no undefined initially */
	doffset = 0;				/* beginning of data seg */

	/* try to catch DATA items not big enough for their COMM's */
	if (first_sized_data)
	{
		register sdp p;

		sp_end = &symtab[symindex];
		for (sp = (Asymp ? Asymp : symtab); sp < sp_end; ++sp)
		{
			if ((sp->s.n_type & FOURBITS) != DATA)
				continue;
			if (sp->s.n_value <= first_sized_data->sdsp->s.n_value)
				continue;
			for (p = first_sized_data;
			     p->sdnext != NULL
			     && sp->s.n_value > p->sdnext->sdsp->s.n_value;
			     p = p->sdnext)
				;
			if (sp->s.n_value - p->sdsp->s.n_value < p->sdvalue)
			{
				if (!p->sdfollow)
				{
					error("initialized data space not big enough for COMM declaration of %s",asciz(p->sdsp->sname,p->sdsp->s.n_length));
					p->sdfollow = sp->s.n_value;
				}
				else if (sp->s.n_value < p->sdfollow)
					p->sdfollow = sp->s.n_value;
			}
		}
		/* don't forget the last element! */
		for (p = first_sized_data; p->sdnext != NULL; p = p->sdnext)
			;
		if (!p->sdfollow && dsize - p->sdsp->s.n_value < p->sdvalue)
			error("initialized data space not big enough for COMM declaration of %s",asciz(p->sdsp->sname,p->sdsp->s.n_length));
	}

	/* Alignment symbols will not be activated if rflag <> 0. If, however,
	   pass 1 has seen any alignment symbols, do the following:
        */
	if (alptr) {
		findal1(TEXT);
		findal1(DATA);
		findal1(BSS);
	}

	/*
	 * Now set symbols to their final value
	 */
	adjust_sizes(1);

	sp = symtab;
	if (Asymp != 0)
	{	for (; sp<Asymp; sp++)
			if( !(sp->s.n_flags & NLIST_STILL_FUZZY_FLAG) )
				ssize += NLIST_SIZE + sp->s.n_length;
#ifdef	BBA
			else
#pragma		BBA_IGNORE
				;
#endif
	}
	sp_end = &symtab[symindex];

	/* We need to assign PLT first, so that other externals can use this as
	 * a base
	 */
	if( !rflag )
	{
		sp2 = symtab + slookup( "PLT" );
		sym2( sp2 );
		plt_start = sp2->s.n_value;
	}
	else
		sp2 = sp_end;

	/* Skip over PLT (already done, above) */
	for (; sp < sp2; sp++)         
		if( !(sp->s.n_flags & NLIST_STILL_FUZZY_FLAG) )
		   sym2(sp);
#ifdef	BBA
		else
#pragma		BBA_IGNORE
			;
#endif
	for (sp++; sp < sp_end; sp++) 
		if( !(sp->s.n_flags & NLIST_STILL_FUZZY_FLAG) )
		   sym2(sp);

	bsize += csize;
	lestorigin = borigin + bsize;

	if( shlib_level > SHLIB_DLD )
		ExpReloc();
	DltReloc();
#ifdef	VISIBILITY
	if (shlib_level == SHLIB_BUILD)
		hidden_imports();
#endif

#ifdef FAKE_SHLIB_AS_OBJECT
	if (nund && !rflag && Fflag < 10)
#else
	if (nund && !rflag)
#endif
	{
		error(e34);
#ifdef  FTN_IO
		if (ftn_io_mismatch)
			fprintf(stderr,"Possible FORTRAN library version mismatch.  Recompile with FCOPTS=+I300\n");
#endif
		exit_status = (bflag ?
#pragma		BBA_IGNORE
		               0 : 1);
	}

#ifdef	VFIXES
	for (i = 0; i < VFIXES; ++i)
	{
		/* if new version of symbol, be careful */
		/* mark it undefined so we'll trip across it later */
		if ((slot = slookup(vfixes[i].new.name)) != -1)
		{
			sp = symtab + slot;
			vfixes[i].new.location = slot;
			vfixes[i].type = sp->s.n_type;
			vfixes[i].old.location = slookup(vfixes[i].old.name);
			sp->s.n_type = EXTERN+UNDEF;
		}
	}
#endif

}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* common -	Set up the common area.  */

common()
{
	register symp sp, sp2, sp_end = &symtab[symindex];
	register long val;

	csize = 0;
	for (sp = symtab; sp < sp_end; sp++)
		if ((sp->s.n_type & FOURBITS) == UNDEF
		    && (val = sp->s.n_value)
			&& !(sp->s.n_flags & NLIST_STILL_FUZZY_FLAG)
			)
		{
			val = (val + 3) & ~03;	/* long word boundary */
			sp->s.n_value = csize;
			sp->s.n_type = EXTERN+COMM;
			if( sp->expindex == EXP_UNDEF )
			{
				sp->expindex = ExpEnter( sp, sp->s.n_type, sp->s.n_value, 0, val, -1, -1, -1 );
#if	DEBUG & 1
				printf("comm symbol %s given export entry %d\n",sp->sname,sp->expindex);
#endif
			}


			if( sp->s.n_flags & NLIST_DLT_FLAG )                              
			{
				assert( sp->dltindex >= 0 );
				DltSet( sp->dltindex, csize, COMM ); 
			}
			csize += val;

		}
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* sym2 -	Assign external symbols their final value and compute ssize */

sym2(sp)
register symp sp;
{
	static int g = 0, p = 1;

	/* give the dynamic loader a hint about where to find export */
	if (shlib_level == SHLIB_BUILD && sp->expindex >= 0)
	{
#if	DEBUG & 1
		if ((sp->s.n_type & FOURBITS) == COMM)
			printf("providing hints on comm symbol %s: dlt = %d, plt = %d, rimp = %d; exp = %d\n",sp->sname,sp->dltindex,sp->pltindex,sp->rimpindex,sp->expindex);
#endif
		if (sp->dltindex != DLT_UNDEF)
			dltimports[sp->dltindex].shl = sp->expindex;
		if (sp->pltindex != PLT_UNDEF)
			pltimports[sp->pltindex].shl = sp->expindex;
		if (sp->rimpindex != RIMP_UNDEF)
			reloc_imports[sp->rimpindex].shl = sp->expindex;
	}

	ssize += NLIST_SIZE + sp->s.n_length;
	switch (sp->s.n_type)
	{
	case EXTERN+UNDEF:
#ifdef FAKE_SHLIB_AS_OBJECT
		if (rflag == 0 && sp->s.n_value == 0 && (!bflag || vflag) && Fflag < 10)
#else
		if (rflag == 0 && sp->s.n_value == 0 && (!bflag || vflag))
#endif
		{
			if (nund++==0)
				error(e35, bflag ?
#pragma		BBA_IGNORE
				           "(warning) " : "");
			fprintf(stderr,"\t%s\n", asciz(sp->sname,sp->s.n_length));
#ifdef  FTN_IO
			if (ftn_io(asciz(sp->sname,sp->s.n_length)))
				ftn_io_mismatch = 1;
#endif
		}
		break;

	case EXTERN+ABS:
	default:
#ifdef	BBA
#pragma BBA_IGNORE
#endif
		break;

	case EXTERN+TEXT:
	case EXTERN+TEXT+ALIGN:
	case EXTERN+TEXT+EXTERN2:
		if( sp->s.n_flags & NLIST_EXPORT_PLT_FLAG )
		{
#ifdef	SHLPROCFIX
			/* tell DtlReloc this is an shl proc */
			if (sp->dltindex >= 0)
				dltimports[sp->dltindex].type += EXTERN2;
#endif
			sp->s.n_value += plt_start;
		}
		else
			sp->s.n_value += torigin + shlibtextsize + sp -> stincr;
		break;

	case EXTERN+DATA:
	case EXTERN+DATA+ALIGN:
	case EXTERN+DATA+EXTERN2:
		sp->s.n_value += dorigin + sp -> stincr;
		break;

	case EXTERN+BSS:
	case EXTERN+BSS+ALIGN:
	case EXTERN+BSS+EXTERN2:
		sp->s.n_value += borigin + sp -> stincr;
		break;

	case EXTERN+COMM:
		sp->s.n_type = EXTERN+BSS;
		sp->s.n_value += corigin + sp -> stincr;
		break;
	}
#ifdef	EXPORTALIGNFIX
	if (sp->expindex >= 0)
		exports[sp->expindex].value += sp->stincr;
#endif
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* ldrsym -	Force the definition of a symbol */

ldrsym(asp, val, type)
register int 	asp;
long 		val;			/* value of the symbol */
char 		type;			/* its type */
{
        register symp sp;

	if (asp== -1)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
		return;
	sp = &symtab[asp];
	if (sp->s.n_type != EXTERN+UNDEF || sp->s.n_value)
	{
		error(e6, asciz(sp->sname, sp->s.n_length), filename);
		return;
	}
	sp->s.n_type = type;
	sp->s.n_value = val;
	sp->s.n_almod = 0;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* findal1 -	Called as a separate mini-pass only if alignment symbols
		have been seen in pass 1. Once one of these occurs, all
		succeeding symbols in the current section of the output
		file will have to be adjusted downward. Since more than
		one alignment symbol may be encountered during pass 1, the
		effect is cumulative. This routine must juggle the alptr
		linked list of such alignment symbols and the total linked
		list of arguments to include pertinent library members.
		The easiest method of handling multiple alignment
		symbols is to perform a separate mini-pass thru the
		symbol table for each one.

		If files have been previously loaded (using the -r flag),
		excessive white space may be put into the object
		file. Two solutions are possible: a)  Do nothing for 
		alignment symbols whenever -r is specified. i.e. delay
		any action on them until the final load; or b) Have an
		algorithm that first collapses previously built white space
		before doing any construction of white space. I think the
		first option is more reasonable and it is the one that is
		implemented here.
*/

findal1(aligntype)
register unsigned int aligntype;	/* ids the sgmnt the aligner is in */
{
	register symp sp;
	symp sp_end;
	register symp alsp;
	register long alignspot;	/* offset in seg for start of white space */
	register long	incr;		/* size of white space */
	register aligne alp = alptr;

	do {
		/* Symbol adjustments must go in 2 phases. First, it is
		   necessary to determine the size and location of the white
		   space. Then each symbol found to be succeeding the location
		   of the white space must be incremented by this amount.
		*/
		
		alsp = alp->asymp;
		if (aligntype == (alsp->s.n_type & FOURBITS)) {
			adjust_sizes(0);
			alignspot = alsp->s.n_value + alsp->stincr;
			switch (aligntype) {
		case TEXT:
			alignspot += shlibtextsize;
			for (incr=torigin+alignspot;incr++ % alp->modulo;) ;
			incr -= torigin + alignspot + 1;
			tsize += incr;
			break;
		case DATA:
			for (incr=dorigin+alignspot; incr++ % alp->modulo;) ;
			incr -= dorigin + alignspot + 1;
			dsize += incr;
			break;
		case BSS:	/* is this possible? */
			for (incr= borigin+alignspot; incr++ % alp->modulo;) ;
			incr -= borigin + alignspot + 1;
			bsize += incr;
			break;
		}
		
		alsp->sincr = incr;	/* record for later use in load2td */

		/* add increment for this alignment to every symbol that is
			o in the same segment AND
			o not the align symbol AND
			o greater or equal to the alignment symbol
		*/

		sp_end = &symtab[symindex];
		for (sp = symtab; sp < sp_end; sp++) 
			if ((aligntype == (sp->s.n_type & FOURBITS)) &&
			    (sp != alp->asymp) &&
			    (sp->s.n_value >= alsp->s.n_value))
					sp->stincr += incr;

		}
		alp = alp -> nextale;
	} while (alp != NULL);
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
adjust_sizes(final)
int	final;	/* <>0 only the last time this is called to ensure proper
		   alignments of segments and sizes */
{
	if (final) {
		tsize = (tsize + 3) & ~03;
		dsize = (dsize + 3) & ~03;
		bsize = (bsize + 3) & ~03;
		csize = (csize + 3) & ~03;
	}
#ifdef	MISC_OPTS
#ifdef ALIGN_DORIGIN_FIX
	if (!Dused)
#else
	if (!dorigin)
#endif
	{
#endif
	if (nflag || qflag || 
		(shlib_level == SHLIB_BUILD) || (shlib_level == SHLIB_DLD) )
		dorigin = torigin + EXEC_ALIGN(tsize);
	else 
		dorigin = torigin + tsize;
#ifdef	MISC_OPTS
	}
#endif
	real_dorigin = dorigin;
	if (shlib_level == SHLIB_BUILD && EXEC_ALIGN(dsize) < EXEC_ALIGN(dsize+bsize+csize))
		corigin = dorigin + EXEC_ALIGN(dsize);
	else
	{
		corigin = dorigin + dsize;
		if (shlib_level == SHLIB_BUILD || shlib_level == SHLIB_DLD)
			bss2data = 1;
	}
	borigin = corigin + csize;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* setupout -	Set up for output.  Create output file and a temporary
		files for the data area and relocation commands if
		necessary.  Write the header on the output file.
*/

setupout()
{
	register struct exec *f = &filhdr;
	char * dir;	/* directory of output file */
	register unsigned short i;
#ifdef	VFIXES
#if VERSION == 65
	register unsigned short highest_seen;
#else
	register int highest_seen = -1;
#endif
#endif
	
	/* if user has not specified either -n or -q, and 
	   text+data is above a certain size, make a.out
	   demand loaded by default */
	if(!bflag && !iflag && !qflag && nflag==1 && 
	    tsize+dsize >= DEMAND_THRESHOLD)
		qflag = 1;

	if (rflag) f -> a_magic.file_type = RELOC_MAGIC;
	else if (bflag || iflag) f -> a_magic.file_type = SHL_MAGIC;
	else if (qflag) f -> a_magic.file_type = DEMAND_MAGIC;
	else if (nflag) f -> a_magic.file_type = SHARE_MAGIC;
	else f -> a_magic.file_type = EXEC_MAGIC;

	f->a_magic.system_id = HP9000S200_ID;
#ifdef	VFIXES
	if (version_seen[0] && version_seen[3])
	{
		error("(Warning) possible floating point incompatibility - recompile with +O1");
		fprintf(stderr," (see appropriate language reference manual for details)\n");
	}
	for (i = 0; i <= HIGHEST_VERSION; ++i)
		if (version_seen[i])
			highest_seen = i;
	if (rflag && version_seen[0] && highest_seen > 0)
		error("cannot link pre-6.5 modules with new modules using -r");
#if VERSION == 65
	f->a_stamp = highest_seen;
#else
	f->a_stamp = (highest_seen >= 0) ? highest_seen : 2;
#endif
#else
	f->a_stamp = (highest_seen >= 0) ? highest_seen : 2;
#endif
	f->a_miscinfo = M_VALID | 
		            (shlib_level == SHLIB_A_OUT ? M_INCOMPLETE : 0) | 
#ifdef	PICR
					(supsym_gen ? pic : 0 ) |
#endif
				    (f->a_stamp == 3 ? M_FP_SAVE : 0);
	if (Vflag)
		f->a_stamp = stamp;
	f->a_text = tsize;
	f->a_data = dsize;
	f->a_bss = bsize;
	f->a_trsize = (rflag || plus_rflag) ? rtsize: 0;
	f->a_drsize = (rflag || plus_rflag) ? rdsize: 0;
	f->a_pasint = 0;
	f->a_lesyms = sflag? 0:ssize;
	f->a_supsym = (rflag && supsym_gen ? 
				   ((nloc_items+symindex) * sizeof(struct supsym_entry)) : 0 );
	if( shlib_level == SHLIB_BUILD )
		f->a_highwater = high_highwater;
	debugheader2.e_header = DEBUG_HEADER;
	debugheader2.e_extension = 0;
	debugheader2.e_version = 0;
	debugheader2.e_size = 0;

	debugheader2.e_spec.debug_header.header_offset = 
	   sflag? 0 : RDATAPOS + f->a_drsize + sizeof(debugheader2);
	debugheader2.e_spec.debug_header.header_size = sflag? 0:headersize;

	debugheader2.e_spec.debug_header.gntt_offset = 
	   debugheader2.e_spec.debug_header.header_offset +
	   debugheader2.e_spec.debug_header.header_size;
	debugheader2.e_spec.debug_header.gntt_size = sflag? 0:gnttsize;

	debugheader2.e_spec.debug_header.lntt_offset = 
	   debugheader2.e_spec.debug_header.gntt_offset +
	   debugheader2.e_spec.debug_header.gntt_size;
	debugheader2.e_spec.debug_header.lntt_size = sflag? 0:lnttsize;

	debugheader2.e_spec.debug_header.slt_offset = 
	   debugheader2.e_spec.debug_header.lntt_offset +
	   debugheader2.e_spec.debug_header.lntt_size;
	debugheader2.e_spec.debug_header.slt_size = sflag? 0:sltsize;

	debugheader2.e_spec.debug_header.vt_offset = 
	   debugheader2.e_spec.debug_header.slt_offset +
	   debugheader2.e_spec.debug_header.slt_size;
	debugheader2.e_spec.debug_header.vt_size = sflag? 0:vtsize; 

	debugheader2.e_spec.debug_header.xt_offset = 
	   debugheader2.e_spec.debug_header.vt_offset +
	   debugheader2.e_spec.debug_header.vt_size;
	debugheader2.e_spec.debug_header.xt_size = sflag? 0:xtsize;

	if (sflag) {
	   debugoffset = 0;
	}
	else if (headersize || gnttsize || lnttsize || 
		 sltsize    || vtsize   || xtsize) {
	   debugoffset = RDATAPOS + f->a_drsize;
	   f->a_miscinfo |= M_DEBUG;
	}
	else {
	   debugoffset = 0;
	}

	if (plus_rflag)
	{
		f->a_spared = symtab[slookup("DLT")].s.n_value - middle_dlt * sizeof(struct dlt_entry);
		f->a_spares = abs_dltindex + dltindex + local_dlt;
	}
	else
		f->a_spared = f->a_spares = 0;

	f->a_extension = (shlib_level >= SHLIB_DLD ? TEXTPOS : debugoffset);

	if (entrypt)
		f->a_entry = entrypt->s.n_value;
	else f->a_entry = entryval;

	f->a_drelocs = 0;          /* Updated with rewrite_exec_header() */

	tempname = malloc( strlen(ofilename) + 10);
	strcpy(tempname, ofilename);
	dir = strrchr(tempname, '/');
	if (dir == NULL) strcpy(tempname, "./" );
		else *++dir = 0;
	strcat(tempname, "LDXXXXXX" );
	tempname = (char *) mktemp(tempname);
	close ( creat(tempname, 0666));

	if ((tout = fopen(tempname, "w")) == NULL)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
		fatal(e8, tempname);
	if ((dout = fopen(tempname, "w")) == NULL)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
		fatal(e9, tempname);
	setvbuf(tout,NULL,_IOFBF,8192);
	setvbuf(dout,NULL,_IOFBF,8192);

	fseek(dout, DATAPOS, 0);

	if (debugoffset) 
	{	if ((debugextheaderout = fopen(tempname, "w")) == NULL)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			fatal(e9, tempname);
		fseek(debugextheaderout, debugoffset, 0);
	}

	if (debugheader2.e_spec.debug_header.header_size)
	{	if ((headerout = fopen(tempname, "w")) == NULL)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			fatal(e9, tempname);
		fseek(headerout, debugheader2.e_spec.debug_header.header_offset, 0);
	}

	if (debugheader2.e_spec.debug_header.gntt_size)
	{	if ((gnttout = fopen(tempname, "w")) == NULL)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			fatal(e9, tempname);
		fseek(gnttout, debugheader2.e_spec.debug_header.gntt_offset, 0);
	}

	if (debugheader2.e_spec.debug_header.lntt_size)
	{	if ((lnttout = fopen(tempname, "w")) == NULL)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			fatal(e9, tempname);
		fseek(lnttout, debugheader2.e_spec.debug_header.lntt_offset, 0);
	}

	if (debugheader2.e_spec.debug_header.slt_size)
	{	if ((sltout = fopen(tempname, "w")) == NULL)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			fatal(e9, tempname);
		fseek(sltout, debugheader2.e_spec.debug_header.slt_offset, 0);
	}

	if (debugheader2.e_spec.debug_header.vt_size)
	{	if ((vtout = fopen(tempname, "w")) == NULL)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			fatal(e9, tempname);
		fseek(vtout, debugheader2.e_spec.debug_header.vt_offset, 0);
	}

	if (debugheader2.e_spec.debug_header.xt_size)
	{	if ((xtout = fopen(tempname, "w")) == NULL)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			fatal(e9, tempname);
		fseek(xtout, debugheader2.e_spec.debug_header.xt_offset, 0);
	}

	if (rflag || plus_rflag)
	{
		if ((trout = fopen(tempname, "w")) == NULL)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			fatal(e9, tempname);
		fseek(trout, RTEXTPOS, 0); /* start of text relocation */
		if ((drout = fopen(tempname, "w")) == NULL)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			fatal(e9,tempname);
	    { /* This extra scope is here for the BBA_IGNORE pragma */
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		    fseek(drout, RDATAPOS, 0);	/* to data reloc */
	    }
	}
	fwrite(f, sizeof(filhdr), 1, tout);
	save_filhdr = filhdr;

    { /* This extra scope is here for the BBA_IGNORE pragma */
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		fseek(tout,TEXTPOS,0);
	}
}


#ifdef	FTN_IO

ftn_io (char *symbol)
{
	if (!strncmp(symbol,"_F_",3) || !strncmp(symbol,"_Ftn_",5))
		return(1);
	else
		return(0);
}

#endif


#ifdef	VISIBILITY
hidden_imports ()
{
	int i;

	/* At this point the export table has been created,
	   and hidden symbols were duly left out.
	   So now we need to identify imports for hidden symbols
	   (their names will correspond to defined symbols)
	   and hack up their import records. */
	for (i = 0; i < dltindex; ++i)
     	check_import(dltimports+i);
	for (i = 0; i < pltindex; ++i)
     	check_import(pltimports+i);
	for (i = 0; i < rimpindex; ++i)
     	check_import(reloc_imports+i);
}


check_import (struct import_entry *import)
{
	int slot;
	symp sp;

	if (import->type == TYPE_ABSOLUTE)
		return;
	slot = slookup(stringt+import->name);
	if (slot < dont_export_syms)
		return;
	sp = symtab + slot;
	if ((sp->s.n_type & FOURBITS) != UNDEF && sp->expindex < 0)
	{
		import->name = sp->s.n_value;
		import->shl = sp->module;
		import->type = TYPE_INTRA;
	}
}
#endif
