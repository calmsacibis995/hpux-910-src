/* Internal RCS $Header: pass2.c,v 70.4 92/02/05 14:33:53 ssa Exp $ */

/* @(#) $Revision: 70.4 $ */   

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

     Command:      Series 200 ld   ( HP -UX link editor )
     File   :      pass2.c

     Purpose:      driver for pass2
		   - loads the text and data segments performing
		     relocation
                   - complete writing the output file

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "ld.defs.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

#ifdef	BBA
long relext(), relcmd();
#endif

/* extern unsigned long dntt_base, slt_base, vt_base; */
aligne loc_alptr;
long total_text_align_incr;
long total_data_align_incr;
long total_bss_align_incr;
int absolute;	/* flag indicates if previous relocation record was
		   for an absolute symbol */
static int reloc_seen_here;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* load2arg -	Load a named file or an archive */

load2arg(cp, libtype)
char 	*cp;
int libtype;
{
	register arce entry;		/* pointer to current entry */
	register struct shl_data_copy *sptr; 
	struct symbol *sp;

	switch (getfile(cp, libtype))
	{
	case 1 /* FMAGIC */:		/* normal file */
		
/*		dread(&filhdr, sizeof filhdr, 1, text); */
		load2(0L);
		break;
	case 2 /* ARCMAGIC */:		/* archive */
		for(entry=arclist->arc_e_list; entry; entry=entry->arc_e_next)
		{
/*			fseek(text, infilepos = entry->arc_offs, 0); */
/*			dread(&filhdr, sizeof filhdr, 1, text); */
			infilepos = entry->arc_offs;
			load2(infilepos);		/* load the file */
		}
		arclist = arclist->arc_next;
		break;

	case 3 /* SHL_MAGIC */:             /* Shared Library */
 	case 4 /* DL_MAGIC */:              /* DL Library */
		for( sptr = &shldata[ shldata_nidx ]; sptr->type != -1; 
			                                          sptr++, shldata_nidx++ )
		{
			fseek( text, sptr->offset, 0 );
			switch( sptr->type )
			{
			  case TYPE_DATA:
				zout( dout, sptr->align );
				dcopy( text, dout, sptr->size );
				
				/* Create the relocation entry */
				if( sptr->sym >= 0 )
				{
					relocs[ curr_relindex ].address = dorigin + sptr->align;
					sp = &symtab[ sptr->sym ];
					relocs[ curr_relindex ].symbol = 
					  (sp->pltindex != PLT_UNDEF ?
#pragma		BBA_IGNORE
					                   sp->pltindex :
 					                   pltindex + sp->rimpindex);
					relocs[ curr_relindex ].type = DR_PROPAGATE;
					relocs[ curr_relindex++ ].length = 0;
				}

				dorigin += sptr->align + sptr->size;
				doffset += sptr->align + sptr->size;
				break;
#ifdef  CDATA
			  case TYPE_CDATA:
				zout( tout, sptr->align );
				dcopy( text, tout, sptr->size );
				torigin += sptr->align + sptr->size;
				break;
#endif
			  default:
#ifdef   BBA
#pragma  BBA_IGNORE
#endif
				assert( 0 );
				break;
			}
		}
		/* Align segments */
		zout( dout, sptr->align );
		dorigin += sptr->align;
		doffset += sptr->align;

		zout( tout, sptr->size );
		torigin += sptr->size;

		borigin += sptr->offset;
		shldata_nidx++;

		break;

	default:
#ifdef	BBA
#pragma BBA_IGNORE
#endif
		bletch("bad file type on second pass");
	}
	fclose(text);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* load2 -	Actually output the text, performing relocation as necessary.
*/


load2(sloc)
register long sloc;	/* position of filhdr in current input file */
{
	register symp sp;
	register int s;
	register int symno = 0;
	unsigned short type;
	register unsigned short aligned_sym=0;
	register int i;
	register long int x;
	register char * lst;
	register char *p1;
	register char *p2;
	aligne a1,a2,a3;
	long text_align_incr = 0;
	long data_align_incr = 0;
	long bss_align_incr = 0;
	int zero;
	struct r_info *relbuft = NULL;
	struct r_info *relbufd = NULL;
	int rnumbert;
	int rnumberd;

	loc_alptr = NULL;             /* head of linked list of aligned symbols 					 for this file */
	readhdr(sloc);
	readdebugexthdr(sloc);
	ctrel = torigin - total_text_align_incr;
	cdrel += (dorigin - total_data_align_incr);
	cbrel += (borigin - total_bss_align_incr);

	/*
	 * Reread the symbol table, recording the numbering
	 * of symbols for fixing external references.
	 */
  	fseek(text,sloc + LESYMPOS, 0);	/* skip to symbols */
	if( filhdr.a_lesyms > 0 )
	{
		lst = (char *) malloc(filhdr.a_lesyms);
		if ( fread((char *) lst, 1, filhdr.a_lesyms, text)
			!= filhdr.a_lesyms )
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			fatal (e4, filename);
		
		for( x = 0; x < filhdr.a_lesyms; )
		{
			p1 = (char *) &(cursym.s);
			p2 = & lst[x] ;
			for (i = NLIST_SIZE; i > 0; i--)
				*p1 ++ = *p2 ++ ;
  	        x += NLIST_SIZE;
  	        for (i = 0; i < cursym.s.n_length; i++)
				csymbuf[i] = lst[x++] ;
			cursym.sname = csymbuf;
			
			if (++symno > ld_stsize) expand();
			symreloc();
			/* NOTE : Berkeley's loader expands symreloc inline here */
			type = cursym.s.n_type;
			if ((type&EXTERN) == 0)		/* enter local symbols now */
			{
				/* ALL local symbols must be enter()ed since we need the D/PLT
				 * information.  So, the actual stripping of local symbols
				 * is now done in finishout() (with the -x flag)      GSL
				 */
#ifdef	PICR
				enter(-1);
				if(!rflag && (cursym.s.n_flags & NLIST_DLT_FLAG))
#else
				enter(lookup());
				if( cursym.s.n_flags & NLIST_DLT_FLAG )
#endif
				{
					lastsym->dltindex = 
						DltEnter( NULL, lastsym->s.n_type & FOURBITS,
								  lastsym->s.n_value, 0 );
				}
				if( cursym.s.n_flags & NLIST_PLT_FLAG )
				{
#pragma		BBA_IGNORE
					lastsym->pltindex = PLT_LOCAL_RESOLVE;
				}
				local[symno - 1] = lastsym;
				continue;
			}
			if ((s = lookup()) == -1)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
				fatal(e11, asciz(cursym.sname, cursym.s.n_length),filename);
			sp = & symtab[s];
			local[symno - 1] = sp;
			
			if (cursym.s.n_type & ALIGN)
				{
					/* build linked list of aligned symbols for this file */
					if (!rflag)
					{
						aligned_sym = 1;
					
						a1 = (aligne)calloc(1, ALIGN_ENTRY_SIZE);

						a1->modulo = sp->s.n_almod;
						a1->asymp = sp;
						a1->nextale = NULL;
						if (loc_alptr == NULL) loc_alptr = a1;
						else
						{
							for (a2 = loc_alptr, a3 = NULL ;
								 a2 && a2 ->asymp ->s.n_value < sp->s.n_value;
								 a3 = a2, a2=a2->nextale) ;
							a1->nextale = a2;
							if ( a3 ) a3 -> nextale = a1;
							else loc_alptr = a1;
							
						}
					}
				}
			if ((!bflag
#ifndef	MISC_OPTS
			     && !iflag
#endif
			    )
				&& cursym.s.n_type != EXTERN+UNDEF
				&& ((cursym.s.n_type & EXTERN2) == 0)
				&& (cursym.s.n_type != sp->s.n_type
					|| cursym.s.n_value != sp->s.n_value - sp->stincr))
#ifdef	VFIXES
				{
					register int i;
					
					for (i = 0; i < VFIXES; ++i)
					{
						if (sp == symtab + vfixes[i].new.location
							&& cursym.s.n_value == sp->s.n_value - sp->stincr)
							break;
					}
					if (i == VFIXES)
					  error(e6,asciz(cursym.sname,cursym.s.n_length),filename);
				}
#else
			error(e6, asciz(cursym.sname, cursym.s.n_length), filename);
#endif
			if ((cursym.s.n_type & FOURBITS) == UNDEF
				&& cursym.s.n_value > 0)
				{
					if ((sp->s.n_type & FOURBITS) == TEXT
						&& (sp->s.n_type & EXTERN))
							error(e6,asciz(cursym.sname,cursym.s.n_length),
								                                    filename);
				}
		}
		
		if (lst) 
			free (lst);
#ifdef	BBA
		else
#pragma BBA_IGNORE
			;
#endif
	}

	if (funding) return;

	align_seen = aligned_sym;	/* true if any align syms seen */
	last_local=symno;

	/* if any aligned symbols were seen, correct the value of the symbol
	   table entry for all local symbols
	*/
	if (aligned_sym)
	{
		for(i=0;i<symno;i++)
		{
			if ((sp = local[i]) == NULL)
#pragma		BBA_IGNORE
				continue;
			if ((sp->s.n_type & EXTERN) == 0)
				sp->s.n_value = align_inc(sp->s.n_value,sp->s.n_type);
			else /* symbol is external */
				switch (sp -> s.n_type)
				{
				  case TEXT + EXTERN + ALIGN:
					text_align_incr += sp->sincr;
					break;
				  case DATA + EXTERN + ALIGN:
					data_align_incr += sp->sincr;
					break;
				  case BSS + EXTERN + ALIGN:
					bss_align_incr += sp->sincr;
					break;
				  default:
					break;
				}

		}
	}
	/* alloc memory for text r_info */
	relbuft = (filhdr.a_trsize ? 
			  (struct r_info *)malloc(filhdr.a_trsize) : (void *)NULL);

	/* calculate number of r_info entrys */
	rnumbert = filhdr.a_trsize / sizeof(struct r_info);

	/* position file ptr to get r_info from file */
	fseek(text, sloc+RTEXTPOS
#if   VERSION==80
		                      /* This is here for compatability */
	                          + filhdr.a_spared + filhdr.a_spares
#endif
	                                                             , 0);
    dread(relbuft, sizeof(struct r_info), rnumbert, text);

	/* alloc memory for data r_info */
	relbufd = (filhdr.a_drsize ? 
			  (struct r_info *)malloc(filhdr.a_drsize) : (void *)NULL);

	/* calculate number of r_info entrys */
	rnumberd = filhdr.a_drsize / sizeof(struct r_info);

	fseek(text, sloc+RDATAPOS
#if   VERSION==80
		                      /* This is here for compatability */
		                      + filhdr.a_spared + filhdr.a_spares
#endif
	                                                             , 0);
    dread(relbufd, sizeof(struct r_info), rnumberd, text);

	/* now output the final text segment */
    fseek(text, sloc+TEXTPOS, 0);
	i = curr_relindex;
	reloc_seen_here = 0;
	load2td(tout, trout, torigin, filhdr.a_text, rnumbert, relbuft, 0L);
	if (reloc_seen_here)
	{
		error(e42,filename);
		text_reloc_seen = 1;
	}
	
	/* free old r_info memory if necessary */
	if (relbuft) free(relbuft);
#ifdef	BBA
	else
#pragma BBA_IGNORE
		;
#endif

	/* now output the final data segment */
	fseek(text, sloc+DATAPOS, 0);
	load2td(dout, drout, doffset, filhdr.a_data, rnumberd, relbufd,real_dorigin);
	if (shlib_level == SHLIB_BUILD && curr_relindex - i != filhdr.a_drelocs)
#pragma		BBA_IGNORE
		fatal(e55,filename);

	/* free old r_info memory if necessary */
	if (relbufd) free(relbufd);
#ifdef	BBA
	else
#pragma BBA_IGNORE
		;
#endif
	if (!sflag && debugheader.e_spec.debug_header.header_size) {
	   fseek(text, sloc+debugheader.e_spec.debug_header.header_offset, 0);
	   dcopy(text, headerout, debugheader.e_spec.debug_header.header_size);
	}
	if (!sflag && debugheader.e_spec.debug_header.gntt_size) {
	   fix_dntts(sloc,1);
	}
	if (!sflag && debugheader.e_spec.debug_header.lntt_size) {
	   fix_dntts(sloc,0);
	}
	if (!sflag && debugheader.e_spec.debug_header.slt_size) {
	   fseek(text, sloc+debugheader.e_spec.debug_header.slt_offset, 0);
	   dcopy(text, sltout, debugheader.e_spec.debug_header.slt_size);
	}
	if (!sflag && debugheader.e_spec.debug_header.vt_size) {
	   fseek(text, sloc+debugheader.e_spec.debug_header.vt_offset, 0);
	   dcopy(text, vtout, debugheader.e_spec.debug_header.vt_size);
	}
	if (!sflag && debugheader.e_spec.debug_header.xt_size) {
	   fseek(text, sloc+debugheader.e_spec.debug_header.xt_offset, 0);
	   dcopy(text, xtout, debugheader.e_spec.debug_header.xt_size);
	}

	torigin += filhdr.a_text + text_align_incr;
	dorigin += filhdr.a_data + data_align_incr;
	borigin += filhdr.a_bss + bss_align_incr;
	doffset += filhdr.a_data;

	total_text_align_incr += text_align_incr;
	total_data_align_incr += data_align_incr;
	total_bss_align_incr += bss_align_incr;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* load2td - load the text or data section of a file performing relocation */

load2td(outf, outrf, txtstart, txtsize, rsize, relptr, start_pos)
FILE *outf;				/* text or data portion of output file */
FILE *outrf;				/* text or data relocation part of output file */
long txtstart;				/* initial offset of text or data segment */
long txtsize;				/* number of bytes in segment */
register long rsize;			/* number of appropriate relocation data */
register struct r_info *relptr;		/* shortcut ptr to rel */
long start_pos;          /* Position of this section relative to text */
{
	register int size;		/* number of bytes to relocate */
	register long offs;		/* value of offset to use */
	register long pos = 0;		/* current input position */
	register short	incr;		/* sizeof whitespace added by .align */
	int no_align_inc;		/* do not increment for align if it's
					   an external symbol--it's already
					   been done */
#ifdef	PICR
	long here;			/* where relocation is applied -
					   adjusted for start of segment
					   & alignment within segment */
#endif
#ifdef	SHORT_SYMBOLNUM_FIX
	int index = 0;
#endif

	absolute = 0;
	for( ; rsize--; relptr++)	/* for each relocation command */
	{
		if (pos >= txtsize)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			bletch("relocation after end of segment");

		offs = 0;
		no_align_inc = 0;

		switch( relptr->r_length )
		{
		  case RLONG:   size = DR_LONG;          break;
		  case RWORD:   size = DR_WORD;          break;
		  case RBYTE:   size = DR_BYTE;          break;
		  case RALIGN:                           break;
		  default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			fatal( e12, filename );  break;
		}
#ifdef	PICR
		if (align_seen)
			here = align_inc(relptr->r_address+txtstart+start_pos,(start_pos==0)?TEXT:DATA);
		else
			here = relptr->r_address + txtstart + start_pos;
#endif
		switch(relptr->r_segment)
		{
		case REXT:
			if( (shlib_level == SHLIB_BUILD || shlib_level == SHLIB_DLD) &&
			    !Rflag )
			{
			  if( shlib_level == SHLIB_DLD )
			  {
				relocs[ curr_relindex ].address = relptr->r_address+
					                              txtstart+start_pos;
				relocs[ curr_relindex ].symbol = -1;
				relocs[ curr_relindex ].type = DR_FILE;
#ifdef  MISC_OPTS
				if (size != DR_LONG)
					fatal("can't do word or byte relocation with -i");
#endif
				relocs[ curr_relindex++ ].length = size;
			  }
			  else
			  {
				struct symbol *sp;

				sp = local[relptr->r_symbolnum];
				
				/* If this is an absolute relocation, we don't really 
				 * need to produce a dynamic relocation, since we can
				 * go ahead and punch the correct value right here.
				 * We must keep it around so the indexes stay right, though.
				 */
				if( (sp->s.n_type & FOURBITS) == ABS )
				{
					offs = relext( relptr );
					relocs[curr_relindex++].type = DR_NOOP;
					break;        /* ****** Watch the break ******* */
				}

				relocs[ curr_relindex ].address = relptr->r_address+
					                              txtstart+start_pos;
				if( sp->dltindex == DLT_UNDEF 
				    && sp->pltindex == PLT_UNDEF 
				    && sp->rimpindex == RIMP_UNDEF )
#ifdef	REXT_LOCAL_FIX
				{
					/*
					 * This is probably stretching things a bit, but...
					 * Probably this is an REXT reference to a local,
					 * produced from a prior "ld -r -h" run.
					 * Let's just turn it into a file relative relocation, OK?
					 */
					relocs[curr_relindex].address = relptr->r_address + txtstart + start_pos;
					relocs[ curr_relindex ].symbol = -1;
					relocs[ curr_relindex ].length = size;
					relocs[curr_relindex++].type = DR_FILE;
					offs = 0;
					reloc_seen_here = 1;
					break;
				}
#else
#pragma		BBA_IGNORE
					fatal( e45 );
#endif
				relocs[ curr_relindex ].symbol = 
					(sp->dltindex != DLT_UNDEF ? sp->dltindex :
					 (sp->pltindex != PLT_UNDEF ? (sp->pltindex + global_dlt) :
	                                 global_dlt + pltindex + sp->rimpindex));
#ifdef	ELABORATOR
				if (sp - symtab == elaborator_index)
				    relocs[curr_relindex].type = DR_INVOKE;
				else
#endif
					relocs[ curr_relindex ].type = DR_EXT;
				relocs[ curr_relindex++ ].length = size;
				reloc_seen_here = 1;
				offs = 0;
				break;  /* ***** Watch the break ****** */
			  }
			}
			no_align_inc = 1;
			if ((relptr->r_symbolnum < 0 ||
				relptr->r_symbolnum >= ld_stsize) ||
				(local[relptr->r_symbolnum] == NULL))
			{
#pragma BBA_IGNORE
				error(e19, filename);
				offs = 0;
			}
			else if (rflag && (local[relptr->r_symbolnum]->s.n_type & ALIGN))
				goto rflagchk;
			else if( rflag && supsym_gen )
				goto rflagchk;
			else
				offs = relext(relptr);

			break;
		case RPC:
#ifdef	PICR
			if (rflag && global_align_seen)
				goto rflagchk;
#endif
			if (rflag && supsym_gen)
				goto rflagchk;
#ifdef	DONT_RESOLVE_RPC_TO_ABS
			if ((local[relptr->r_symbolnum]->s.n_type & FOURBITS) == ABS)
				goto rflagchk;
#endif
			if ((local[relptr->r_symbolnum]->s.n_type & FOURBITS) == UNDEF)
			{
#ifdef	DL_MAGIC_RPC_FIX
				if (shlib_level == SHLIB_BUILD)
				{
					if (local[relptr->r_symbolnum]->pltindex >= 0)
						offs = plt_start + local[relptr->r_symbolnum]->pltindex * sizeof(struct plt_entry) - (relptr->r_address + txtstart + start_pos);
					else
						fatal("RPC relocation for undefined symbol %s in %s",local[relptr->r_symbolnum]->sname,filename);
				}
				else
#endif
				offs = 0;
			}
			else
			{
#ifndef	PICR
				if (rflag)
#endif
				absolute = 1;
				offs = local[relptr->r_symbolnum]->s.n_value 
#ifdef	PICR
				       - here;
#else
				        - (relptr->r_address + txtstart+start_pos);
#endif
			}
			break;
		case RDLT:
			if (rflag)
				goto rflagchk;
			offs = local[relptr->r_symbolnum]->dltindex;
			assert( offs >= 0 );
			if( offs & ABS_DLT_FLAG )
				/* If in the absolute dlt section, need to do some funky math.
				 */
				offs = abs_dltindex - (offs & (~(ABS_DLT_FLAG))) - middle_dlt;
			else
				offs = offs - middle_dlt;
			offs *= 4;
			if((offs >= 32764 || offs <= -32768) && relptr->r_length == RWORD)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
				fatal( e51, filename );
			break;
		case RPLT:
#ifdef	PICR
			if (rflag && (local[relptr->r_symbolnum]->s.n_type & FOURBITS) != UNDEF && !global_align_seen)
			{
				local[relptr->r_symbolnum]->s.n_flags &= ~NLIST_PLT_FLAG;
				absolute = 1;
				offs = local[relptr->r_symbolnum]->s.n_value 
				        - (relptr->r_address + txtstart+start_pos);
				break;
			}
#endif
			if (rflag)
				goto rflagchk;
			/* it is possible that this code
			   inadvertently fixes the bug in which
			   unresolved pic proc references in an a.out
			   appear to have their PLT indices blown away */
			if( local[relptr->r_symbolnum]->pltindex == PLT_LOCAL_RESOLVE )
			{
				/* Local symbols are resolved directly, no need for 
				 * anonymous PLT's.  This should be like RPC, above.
				 */
				offs = local[relptr->r_symbolnum]->s.n_value - 
#ifdef	PICR
				       here;
#else
					   (relptr->r_address + txtstart+start_pos);
#endif
			}
			else
#ifdef	RESTORE_MOD
			if (local[relptr->r_symbolnum]->pltindex == PLT_UNDEF)
			{
				bletch("PLT ref to symbol %s with no PLT entry?",local[relptr->r_symbolnum]->sname);
				offs = 0;
			}
			else
#endif
			{
				offs = plt_start + local[relptr->r_symbolnum]->pltindex *
					       sizeof(struct plt_entry)
#ifdef	PICR
				       - here;
#else
							   - (relptr->r_address + txtstart + start_pos);
#endif
			}
			break;
		case RTEXT:
			offs += torigin;
			if( (shlib_level == SHLIB_BUILD || shlib_level == SHLIB_DLD) &&
			    !Rflag )
			{
				relocs[ curr_relindex ].address = relptr->r_address+
					                              txtstart+start_pos;
				relocs[ curr_relindex ].symbol = -1;
				relocs[ curr_relindex ].type = DR_FILE;
#ifdef  MISC_OPTS
				if (shlib_level == SHLIB_DLD && size != DR_LONG)
					fatal("can't do word or byte relocation with -i");
#endif
				relocs[ curr_relindex++ ].length = size;
				reloc_seen_here = 1;
			}
			break;
		case RDATA:
			offs += dorigin - filhdr.a_text;
			if( (shlib_level == SHLIB_BUILD || shlib_level == SHLIB_DLD) &&
			    !Rflag )
			{
				relocs[ curr_relindex ].address = relptr->r_address+
					                              txtstart+start_pos;
				relocs[ curr_relindex ].symbol = -1;
				relocs[ curr_relindex ].type = DR_FILE;
#ifdef  MISC_OPTS
				if (shlib_level == SHLIB_DLD && size != DR_LONG)
					fatal("can't do word or byte relocation with -i");
#endif
				relocs[ curr_relindex++ ].length = size;
				reloc_seen_here = 1;
			}
			break;
		case RBSS:
			offs += borigin - (filhdr.a_text + filhdr.a_data);
			if( (shlib_level == SHLIB_BUILD || shlib_level == SHLIB_DLD) &&
			    !Rflag )
			{
				relocs[ curr_relindex ].address = relptr->r_address+
					                              txtstart+start_pos;
				relocs[ curr_relindex ].symbol = -1;
				relocs[ curr_relindex ].type = DR_FILE;
#ifdef  MISC_OPTS
				if (shlib_level == SHLIB_DLD && size != DR_LONG)
					fatal("can't do word or byte relocation with -i");
#endif
				relocs[ curr_relindex++ ].length = size;
				reloc_seen_here = 1;
			}
			break;
		case RNOOP:
			absolute = 1;
			goto rflagchk;
		}

		switch(relptr->r_length)
		{
		case RBYTE:
			size = 1; break;
		case RWORD:
			size = 2; break;
		case RLONG:
			size = 4; break;
		case RALIGN:
			if (relptr->r_address > txtsize)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
				fatal(e13, filename);
			if (relptr->r_address < pos)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
				error(e21, filename);
			else if (!rflag) {
				dcopy(text, outf, relptr->r_address - pos);
				/* now slip in a little white space for momma */
				incr = local[relptr->r_symbolnum]->sincr;
				while (incr-- > 0) 
				{
#ifdef  BBA
#pragma BBA_IGNORE
#endif
					putc(WSPACE, outf);
				}
				pos = relptr->r_address;
			}
			goto rflagchk;
		default:
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			fatal(e12, filename);
		}
		if (relptr->r_address > txtsize)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			fatal(e13, filename);
		else pos = relcmd(pos, relptr->r_address, size, offs, outf,
				relptr->r_segment+2,	/* change segment into 
							appropriate symbol type */
				no_align_inc);
		/* write out relocation commands */
rflagchk:	if (rflag || plus_rflag)
		   if (!absolute)
		   {
			if (plus_rflag)
			{
#pragma		BBA_IGNORE
				switch (relptr->r_segment)
				{
					case RTEXT:
					case RDATA:
					case RBSS:
						break;
					default:
						relptr->r_segment = RNOOP;
				}
			}
		   	relptr->r_address +=  txtstart;
#ifdef	SHORT_SYMBOLNUM_FIX
			switch (relptr->r_segment)
			{
				case REXT:
				case RDLT:
				case RPLT:
				case RPC:
					if ((index = local[relptr->r_symbolnum]->sindex) > 0xffff)
						fatal("too many symbols in output file");
				default:
					relptr->r_symbolnum = index;
			}
#else
		   	relptr->r_symbolnum = (relptr->r_segment == REXT
								   || relptr->r_segment == RDLT || 
								      relptr->r_segment == RPLT || 
								      relptr->r_segment == RPC
								   )?
		   		local[relptr->r_symbolnum]->sindex: 0;
#endif
		   	fwrite(relptr, sizeof(struct r_info), 1, outrf);
			if( relptr->r_segment < 4 )
				dyn_count++;
		   }
		   else 
		   {
		   	relptr->r_address = 0;
		   	relptr->r_symbolnum = 0;
		   	relptr->r_segment = RNOOP;
		   	relptr->r_length = 0;
		   	fwrite(relptr, sizeof( struct r_info), 1, outrf);
		   	absolute = 0;
		   }
	}
	dcopy(text, outf, txtsize - pos);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* relext -	Find the offset of an REXT command and fix up the rel cmd.
*/

long relext(r)
register struct r_info *r;
{
#ifdef	VFIXES
	register int i;
#endif
	register symp sp;		/* pointer to symbol for EXT's */
	register long offs;			/* return value */
	sp = local[r->r_symbolnum];
	offs = sp->s.n_value;
	switch(sp->s.n_type & FOURBITS)
	{
		case TEXT: 
		    /* This is called only when building a.out's.
		       In such case, a ref to TEXT in a shlib means PLT,
		       which is in data. */
		    if( sp->shlib != SHLIB_UNDEF )
				r->r_segment = RDATA;
			else
				r->r_segment = RTEXT;
			break;
		case DATA: r->r_segment = RDATA; break;
		case BSS: r->r_segment = RBSS; break;
		case ABS: if (rflag) absolute = 1; break;
		case UNDEF:
#ifdef	VFIXES
			if (filhdr.a_stamp == 0)
			{
				for (i = 0; i < VFIXES; ++i)
					if (sp == symtab + vfixes[i].new.location)
						break;
				if (i < VFIXES)
				{
					if ((sp = symtab + vfixes[i].old.location) < symtab)
						error("(Warning) unable to fix pre-6.5 call to %s",vfixes[i].new.name);
					else
						offs = sp->s.n_value;
					r->r_segment = vfixes[i].type - 2;
					break;
				}
			}
#endif
			if (rflag && (sp->s.n_type & EXTERN))
			{
				r->r_segment = REXT;
				return 0;
			}		
	}
	return(offs);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* relcmd -	Seek to <position> in file by copying from text to
		outf, then Add an offset to the next <size>
		bytes in the text and write out to outf.
*/

long relcmd(current, position, size, offs, outf, stype, no_align_inc)
long current;		/* current position in file */
long position;		/* where in file to perform relocation */
int size;		/* number of bytes to fix up */
long offs;		/* the number to fix up bye */
register FILE *outf;	/* where to write to */
unsigned int stype; 	/* sym type for this relocation command */
{

	/* Tasks performed:
	   o  copy the input file up to the point referenced by the relocation
	   	command
	   o  get the next size bytes from the input file
	   o  add the offset to this value
	   o  if this file has any align symbols see if the offset must be
		incremented
	   o  copy this new value to the output file
	*/


	register int i, c;
	register int buf = 0;
	register int count;
	register FILE *inf = text;
#ifdef xcomp300_800
        int tmp;   /*  this is needed below for alignment problems on the 800*/
#endif

	count = position - current;

	/*  dcopy(text, outf, position - current);	*/
	/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	/* dcopy -      Copy input to output, checking for errors */

	
	switch (count) {
	  case 4:    
               	putc(getc(inf), outf); 
	  case 3:
#ifdef  BBA             /* Three's and one's are very rare */
#pragma BBA_IGNORE
#endif
               	putc(getc(inf), outf);
   	  case 2:
               	putc(getc(inf), outf);
	  case 1:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
               	if ((c = getc(inf)) == EOF)
               	{
#ifdef	BBA
#pragma BBA_IGNORE
#endif
               	       	if (feof(inf)) fatal(e3, filename);
               	       	if (ferror(outf)) fatal(e4, filename);
               	}
               	putc(c, outf);
	case 0:
		break;
	default:
		if (count < 0)
		{
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			error(e21, filename);
			return(current);
		}
		dcopy(inf, outf, count);
		break;
	}
	/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

	if (size == sizeof buf) {
		/* this nonsence is done because getw is very slow right now */
		if ((inf->_cnt -= 4) < 0) {
			inf->_cnt += 4;
			buf = getw(inf);
		} else {
#ifdef xcomp300_800
  /*  this code is necessary because on an 800, in assignment of long words,	
       the operands must both be on a 4 byte boundary.  If they are not, you
	get a bus error.  this code eliminates that problem.  by using memcpy
	rather than an assignment. */
			memcpy(&tmp,inf->_ptr,4);
			buf = tmp;
			((int *)(inf->_ptr))++;
#else	
			buf = *(int *)(inf->_ptr);
			inf->_ptr = (unsigned char *)((int *)(inf->_ptr) + 1);
#endif
		}

		buf += offs;

		if (align_seen && !no_align_inc)
			buf = align_inc(buf,stype);

		/* this nonsence is done because putw is very slow right now */
		if ((outf->_cnt -= 4) < 0) {
			outf->_cnt += 4;
			putw(buf, outf);
		} else {
#ifdef xcomp300_800  /* see comments above for alignment problems description */
                        tmp = buf; 
			memcpy(outf->_ptr,&tmp,4);
		        ((int *)(outf->_ptr))++;
#else	
			*(int *)(outf->_ptr) = buf;
			outf->_ptr = (unsigned char *)((int *)(outf->_ptr) + 1);
#endif
	
	/*		*((int *)(outf->_ptr))++ = buf; */
		}
	} else {
		for (i = size; i; i--)
		{
			if ((c = getc(inf)) == EOF)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
				fatal(e3, filename);
			buf = (buf << 8) | (c & LOBYTE);
		}
		buf += offs;
		if (align_seen && !no_align_inc)
			buf = align_inc(buf,stype);
		for (i = size - 1; i >= 0; i--)
		{
			c = (buf >> (i * 8)) & LOBYTE;
			putc(c, outf);
		}
	}
	return(position + size);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* align_inc--find the increment due to alias for this value and type in the
   local list of aligned symbols
*/

align_inc(value,stype)
register unsigned long value;
register unsigned int stype;
{	register aligne a1;
	register symp alsp;

	if (loc_alptr == NULL)
#pragma		BBA_IGNORE
		bletch("linked list of aligned symbols missing");
	for(a1=loc_alptr; a1; a1= a1->nextale)
	{
	   alsp = a1->asymp;
	   if (((alsp->s.n_type & FOURBITS) == stype) &&
		   (alsp->s.n_value <= value))
			value += alsp->sincr;
	}
	return value;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* finishout 	Finish off the output file by writing out symbol table and
		other stuff
*/

finishout()
{
	register symp sp, sp_end = &symtab[symindex];
	register short i;
	register char *cp;
	register hide a2;
	int sym_start;
#ifdef	VISIBILITY
	int dont_hide_me;
#endif
	
	if (debugoffset) 
	{
	    fwrite(&debugheader2, sizeof(debugheader2), 1, debugextheaderout);
	}
	
#ifdef	VFIXES
	/* correct types of fixed symbols */
	for (i = 0; i < VFIXES; ++i)
	{
		if (vfixes[i].new.location != -1)
			(symtab+vfixes[i].new.location)->s.n_type = vfixes[i].type;
	}
#endif
	if (qflag || bflag || iflag)
		fseek(tout, EXEC_ALIGN(sizeof filhdr) + EXEC_ALIGN(tsize) + EXEC_ALIGN(dsize) , 0);
	else fseek(tout, sizeof filhdr + tsize + dsize , 0);
	
	if ((qflag || bflag || iflag) &&
	    ((dsize < EXEC_ALIGN(dsize)) || dsize == 0))
	{
		/* If demand-load: then make sure that data ia padded
		   to the next 4K boundary. If there is no data, make
		   sure that text is padded to next 4K boundary.
		   */
		char null =0;
		fseek(tout, -1, 1);
		fwrite( &null, 1, 1, tout);
	}
	if (sflag == 0) 
	{
#ifndef  NDEBUG
		sym_start = ftell( tout );
#endif
		
		if (hidelist == NULL)
		/* hidelist always has special shlib symbols except in "-r" case */
			for (sp = symtab; sp < sp_end; sp++)
			{
				/* Check for non-external symbols, don't write those */
				if( !(sp->s.n_type & EXTERN) )
				{
					if( xflag && !(sp->s.n_flags & NLIST_DLT_FLAG))
						continue;
					sp->s.n_flags |= NLIST_LIST_FLAG;
				}

				/* If it is still a fuzzy symbol, get out.
				 */
				if( sp->s.n_flags & NLIST_STILL_FUZZY_FLAG )
#pragma		BBA_IGNORE
					continue;
				
				if( shlib_level == SHLIB_BUILD && 
				   ((sp->s.n_type & FOURBITS) >> 1) != 0 &&
				   savernum > 0 )
#pragma		BBA_IGNORE
					sp->s.n_value -= savernum;
				
				sp->s.n_flags &= NLIST_REAL_MASK;
				fwrite(&sp->s, NLIST_SIZE, 1, tout);
				if ((i = sp->s.n_length) == 0)
#pragma BBA_IGNORE
					bletch("zero length symbol");
				cp = sp->sname;
				while (i--)
				{
#pragma		BBA_IGNORE_ALWAYS_EXECUTED
					if (*cp <= ' ')
#pragma BBA_IGNORE
						bletch("bad character in symbol %s",
							   asciz(sp->sname, sp->s.n_length));
					putc(*cp++, tout);
				}
				if (ferror(tout))
#pragma BBA_IGNORE
					fatal(e14, ofilename);
			}
		else  /* there is as linked list of hide elements */
			
			for (sp = symtab; sp < sp_end; sp++)
			{
				/* Check for non-external symbols, don't write those */
				if( !(sp->s.n_type & EXTERN) )
				{
					if( xflag && !(sp->s.n_flags & NLIST_DLT_FLAG))
						continue;
					sp->s.n_flags |= NLIST_LIST_FLAG;
				}
				
#ifdef	VISIBILITY
				dont_hide_me = 0;
#endif
				for (a2 = hidelist; a2; a2 = a2 -> hide_next)
					/* Note: cannot use strcmp here since string 
					   is not null terminated */
					if(sp->s.n_length == strlen(a2->hide_name)
					   && !strncmp(sp->sname,a2->hide_name,sp->s.n_length))
					{
#ifdef	VISIBILITY
						if (hide_status != EXPORT_HIDES)
						{
							if ((sp->s.n_type & FOURBITS) == UNDEF)
#else
							if (sp->s.n_type == (EXTERN | UNDEF))
#endif
								error(e26, asciz(sp->sname,sp->s.n_length));
							else
								sp->s.n_type &= ~EXTERN;
							break;
#ifdef	VISIBILITY
						}
						else
						{
							dont_hide_me = 1;
							break;
						}
#endif
					}
#ifdef	VISIBILITY
					if (hide_status == EXPORT_HIDES && !dont_hide_me && (sp->s.n_type & FOURBITS) != UNDEF)
						sp->s.n_type &= ~EXTERN;
#endif
				if( sp->s.n_flags & NLIST_STILL_FUZZY_FLAG )
					continue;
				
				if( shlib_level == SHLIB_BUILD &&
				   ((sp->s.n_type & FOURBITS) >> 1) != 0 &&
				   savernum > 0 )
#pragma		BBA_IGNORE
					sp->s.n_value -= savernum;
				
				sp->s.n_flags &= NLIST_REAL_MASK;
				fwrite(&sp->s, NLIST_SIZE, 1, tout);
				if ((i = sp->s.n_length) == 0)
#pragma		BBA_IGNORE
					bletch("zero length symbol");
				cp = sp->sname;
				while (i--)
				{
#pragma		BBA_IGNORE_ALWAYS_EXECUTED
					if (*cp <= ' ')
#pragma BBA_IGNORE
						bletch("bad character in symbol %s",
							   asciz(sp->sname, sp->s.n_length));
					putc(*cp++, tout);
				}
				if (ferror(tout))
#pragma BBA_IGNORE
					fatal(e14, ofilename);
			}
		
#ifndef  NDEBUG
		if( (ftell( tout ) - sym_start) != ssize )   
			bletch( "symbol table sizes incorrect" );
#endif

		/* Write out the supsym table if one is needed (this is only in
		 * the case of -r with modules which have a supsym table.  We are
		 * guaranteed that all the local symbols will have clear entries,
		 * since we use calloc/memset(0).
		 */
		if( rflag & supsym_gen )
			fwrite( supsym, sizeof( struct supsym_entry ), 
#ifdef	PICR
					save_filhdr.a_supsym/sizeof(struct supsym_entry),
#else
				    symindex - (xflag ? nloc_items : 0),
#endif
			        tout );
		
	}
	
	if (rflag)
	{
		fclose(trout);
		fclose(drout);
	}
	fclose(tout);
	fclose(dout);
	
	if (debugoffset) {
		
		fclose(debugextheaderout);
		
		if (debugheader2.e_spec.debug_header.header_size) {
			fclose(headerout);
		}
#ifdef	BBA
		else
#pragma		BBA_IGNORE
			;
#endif
		
		if (debugheader2.e_spec.debug_header.gntt_size) {
			fclose(gnttout);
		}
#ifdef	BBA
		else
#pragma		BBA_IGNORE
			;
#endif
		
		if (debugheader2.e_spec.debug_header.lntt_size) {
			fclose(lnttout);
		}
#ifdef	BBA
		else
#pragma		BBA_IGNORE
			;
#endif
		
		if (debugheader2.e_spec.debug_header.slt_size) {
			fclose(sltout);
		}
#ifdef	BBA
		else
#pragma		BBA_IGNORE
			;
#endif
		
		if (debugheader2.e_spec.debug_header.vt_size) {
			fclose(vtout);
		}
#ifdef	BBA
		else
#pragma		BBA_IGNORE
			;
#endif
		
		if (debugheader2.e_spec.debug_header.xt_size) {
			fclose(xtout);
		}
#ifdef	BBA
		else
#pragma		BBA_IGNORE
			;
#endif
		
		/* Invoke the debug preprocessor.  Give a warning message if we
		 * could not invoke pxdb and let xdb deal with it.
		 */
	    if (!rflag
#ifndef	HOOKS
		    && !bflag
#endif
		    && !Aname)
	    {
		   char *pxdb, *getenv();
		   int waitstat;

		   if (!(pxdb = getenv("LD_PXDB"))) 
				pxdb = PXDB;

		   if( vflag )
			   printf( "Invoking Debug Preprocessor: %s\n", pxdb );

		   if (fork() == 0) {
#pragma BBA_IGNORE
				execl( pxdb, pxdb, tempname, 0 );
				if( errno != ENOENT )
					error( e49, pxdb );
				if( vflag )
					printf( "Debug Preprocessor FAILED\n" );
				exit( 0 );     /* So that we don't report an error below */
			}
		    else {
				wait(&waitstat);
				if( ((WIFEXITED(waitstat)) && (WEXITSTATUS(waitstat) != 0))
				    || (WIFSIGNALED(waitstat)) )
				{
#pragma		BBA_IGNORE
					error(e49, pxdb);
					if( vflag )
						printf( "Debug Preprocessor FAILED\n" );
				}
			}
		}
	}

	if (rename(tempname, ofilename) < 0)
#pragma BBA_IGNORE
		fatal(e32, tempname, ofilename);

	if (rflag || exit_status) chmod(ofilename, 0666 & ~umask(0));
	else       chmod(ofilename, 0777 & ~umask(0));
	
	if (warn_xg) error (e36);
}
