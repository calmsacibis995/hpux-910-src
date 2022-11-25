/* Internal RCS $Header: pass1.c,v 70.1 91/11/13 14:43:43 ssa Exp $ */

/* @(#) $Revision: 70.1 $ */   

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

     Command:      Series 200 ld   ( HP-UX link editor )
     File   :      pass1.c

     Purpose:      driver for pass1
		   - creates global LST from all the input files

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */



#include "ld.defs.h"
char *trim();


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* load1arg - scan file to find defined symbols */

load1arg(cp, libtype, bind)
register char 	*cp;
int             libtype, bind;
{
	register arcp 		ap;  	/* ptr to new archive list element */
	register struct ranlib *tp;	/* ptr to ranlib array element */
	int			kind, def;

	kind = getfile(cp, libtype);	/* open file and read 1st word */

	if (tflag) printf( "%s\n", filename );

	switch (kind)
	{
	case 1 /* FMAGIC */:
		if (vflag)
			printf("Loading %s",filename);
		load1(0L, 0);		/* pass1 on regular file */
		break;

	/* regular archive */
	case 2 /* ARCMAGIC */:
		if (vflag)
			printf("Searching %s:\n",filename);
		ap = (arcp)calloc(1, sizeof(*ap));
		ap->arc_next = NULL;
		ap->arc_e_list = NULL;
		if (arclist)
		{
			arclast->arc_next = ap;
			arclast = ap;
		}
		else arclast = arclist = ap;
		fseek(text, SARMAG, 0);	/* skip magic word */
		fread(&archdr, sizeof(archdr), 1, text);

		if (!feof(text))
		{   if (strncmp(archdr.ar_name, DIRNAME,
			    sizeof(archdr.ar_name))!=0)
		    {   /* there is no directory */
			error(e38,filename);
			infilepos = SARMAG;
			fseek(text, SARMAG, 0);
		    }
		    else
		    {
			/*
			It is an up-to-date directory in the first file
			DIRNAME. Read the table of contents and its associated
			string table. Pass thru the library resolving symbols
			until nothing changes for an entire pass (i.e. you can
			get away with backward references when there is a table
			of contents!).
			*/
			dread(&rantnum, sizeof(rantnum), 1, text);
			dread(&rasciisize, sizeof(rasciisize), 1, text);

			if (rantnum == 0x020b && rasciisize >> 16 == 0x0619)
				fatal("%s table of contents is in 800 format",filename);

			tab = (struct ranlib *) malloc(rantnum*sizeof(struct ranlib));
			tabstr = (char *)malloc(rasciisize);

			dread(tabstr, 1, rasciisize, text);
			dread(tab, sizeof(struct ranlib), rantnum, text);
			for (tp=(&tab[rantnum]); --tp >= tab; ) 
			{
				if (tp->ran_un.ran_strx < 0 ||
				   tp->ran_un.ran_strx >= rasciisize)
#pragma BBA_IGNORE
					error(e25, filename);
				tp->ran_un.ran_name = tabstr
							+ tp->ran_un.ran_strx;
			}
			infilepos = ftell(text);
			while (ldrand())
				continue;
			
			/* now that we're finished with the directory, release
			   the space.
			*/
			free((char *)tab);		/* berkeley = cfree */
			free(tabstr);			/* berkeley = cfree */
			break;
		    }
		}
		while (fread(&archdr, sizeof archdr, 1, text) && !feof(text))
		{
			infilepos += sizeof(archdr);
			if (vflag)
				printf("\tLoading %s",trim(archdr.ar_name));
			if (load1(infilepos, 1))		/* arc pass1 */
			{
				register arce ae = (arce)calloc(1, sizeof(*ae));
				
				ae->arc_e_next = NULL;
				ae->arc_offs = infilepos;
				if (arclast->arc_e_list)
				{
					arcelast->arc_e_next = ae;
					arcelast = ae;
				}
				else arclast->arc_e_list = arcelast = ae;
				if (tflag) 
				{
					printf("\t%s\n", trim(archdr.ar_name));
				}
			}
			else if (vflag)
				printf("\t\t%s not loaded (no resolutions)\n",trim(archdr.ar_name));
			infilepos += (atol(archdr.ar_size) + 1)&~1;
			fseek(text, infilepos, 0);
		}
		break;

	/* shared library */
	case 3 /* SHL_MAGIC */:
    case 4 /* DL_MAGIC */:
		if (vflag)
			printf("Searching Shared Library %s:\n",filename);
		/* search the library */
		def = AddShlib(filename, *cp == '-', bind);
		if( shlib_level == SHLIB_NONE )
			shlib_level = SHLIB_A_OUT;
		break;

	default:
		fatal(e5, filename);
	}
	fclose(text);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* trim -       Removes slash and makes the string asciz. */

char * trim(s)
char *s;
{
	register int i;
	for(i = 0; i < 16; i++)
	   if (s[i] == '/') s[i] = '\0';
	return(s);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* step -	Advance to the next archive member, which is at offset infilepos
		in the archive. If the member is useful, record its location
		in the arclist structure for use in pass 2. Mark the end of the
		archive in arclist with a NULL.
*/

step()
{

	int	load1result;
	
	fseek(text, infilepos, 0);
	if (fread(&archdr, sizeof archdr, 1, text) && !feof(text))
	{
		infilepos += sizeof(archdr);
		if (vflag)
			printf("\tLoading %s to satisfy %s",trim(archdr.ar_name),
				                                               cursym.sname);
		if (load1result = load1(infilepos, 1))	/* arc pass1 */
		{
			arce ae = (arce)calloc(1, sizeof(*ae));
			ae->arc_e_next = NULL;
			ae->arc_offs = infilepos;
			if (arclast->arc_e_list)
			{
				arcelast->arc_e_next = ae;
				arcelast = ae;
			}
			else arclast->arc_e_list = arcelast = ae;
			if (tflag) 
			{
				printf("\t%s\n", trim(archdr.ar_name));
			}
		}
		else if (vflag)
			printf("\t\t%s not loaded (no resolutions)\n",trim(archdr.ar_name));
		infilepos += (atol(archdr.ar_size) + 1)&~1;
		return(load1result);
	}
	else
#pragma BBA_IGNORE
		;
	return(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* ldrand - 	One pass over an archive with a table of contents.
		Remember the number of symbols currently defined,
		then call step on members which look promising (i.e.
		that define a symbol which is currently externally
		undefined). Indicate to our caller whether this process
		netted any more symbols.
*/

ldrand()
{
	register int sp;
	register struct ranlib *tp, *tplast;
	register long loc;
	int  nsymt = symindex;	/* berkeley = symx(nextsym) */

	tplast = &tab[rantnum - 1];
	for (tp = tab; tp <= tplast; tp++) {
		/* inline expansion of the pertinent parts of slookup */
		cursym.sname = tp->ran_un.ran_name;
		cursym.s.n_length = strlen(cursym.sname);	/* no trail 0 */
		cursym.s.n_type = EXTERN+UNDEF;
		if ((sp = lookup()) == -1) 
			continue;
		/* replaces
			if ((sp = slookup(tp->ran_un.ran_name)) == NULL)
				continue;
		*/
		if ((symtab[sp].s.n_type & FOURBITS) != UNDEF)
			continue;
		/* if we get here, the symbol is extern and defined */
		infilepos = tp->ran_off;
		step();
		loc = tp->ran_off;
		while (tp < tplast && (tp+1)->ran_off == loc)
			tp++;
	}
	return(symindex != nsymt);	/* i.e. return true iff symindex has
					   been updated by step() */
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* load1 -	Accumulate file sizes and symbols for single file
		or archive member.  Relocate each symbol as it is
		processed to its relative position in its csect.
		If libflg == 1, then file is an archive hence
		throw it away unless it defines some symbols.
		Here we also accumulate .comm symbol values.
*/

load1(sloc, libflg)
int 		libflg;	/* 1 => loading a library, 0 else */
{
	register long x;
	register char *lst = NULL;
	register char *p1;
	register char *p2;
	register int savindex;	/* symbol table index on entry */
	struct supsym_entry *lst_sup = NULL;
	long *lst_slot = NULL, *lst_slot_save = NULL;
	char *lst_used = NULL;
#define   USED_TARGET      1
#define   USED_END         2
#define   USED_LIST_FLAG   4
#define   USED_UNDEF_FLAG  8
	struct symbol *sp;
	long lst_size = 0;
	int savexpindex;
	int savstringindex, savsym_stringindex;
	int savdltindex, savlocal_dlt;
	int savpltindex;
	int savrelindex;
	int savmod_number, savmod_index;
	int y, z, first, last, t;
	unsigned char save_type;
	long save_value;
#ifdef	PICR
	int savpic;
	int savnloc_items;
#endif
	register int ndef;	/* number of symbols defined */
	register int i;
	
	readhdr(sloc);
	readdebugexthdr(sloc);
	ctrel = tsize;
	cdrel += dsize;
	cbrel += bsize;
	ndef = 0;
	nloc = 0;

	if (vflag)
 		printf(" (version %d)\n",filhdr.a_stamp);
	savindex = symindex;
#ifdef	PICR
	savpic = pic;
	savnloc_items = nloc_items;
#endif
	/* Shared library support here deals mainly with the supsym table.
	 * The supsym table comes in to us as a bunch of singly linked lists
	 * which terminate in the size of the object.  We convert this
	 * into cicularly linked lists, and preserve the size in the symtab
	 * entry.
	 *
	 * Also, when building a regular shared library, we need to start 
	 * building the module lists.  This is done here, since this routine
	 * is run on a per-object basis.
	 */

	/* This is just some of the variables that are saved and restored
	 * depending on the need of a particular object.
	 */
	savexpindex    = expindex;
	savstringindex = stringindex;
	savsym_stringindex = sym_stringindex;
	savdltindex = dltindex;
	savlocal_dlt = local_dlt;
	savpltindex = pltindex;
	savrelindex = relindex;
	savmod_index = mod_index;

	/* When building a shared library, we need to build a dmodule list
	 * and a module list.  'module[]' is the array which becomes the
	 * dmodule list in the finished library.
	 */
	if( shlib_level == SHLIB_BUILD )
	{
		savmod_number = mod_number;
		if( mod_number == ld_mod_number )
		{
			ld_mod_number *= 2;
			module = (struct dmodule_entry *)
			     realloc( module, ld_mod_number*sizeof(struct dmodule_entry) );
			ld_module = (struct mod_entry *)
				 realloc( ld_module, ld_mod_number*sizeof(struct mod_entry) );
		}
		module[ mod_number ].module_imports = mod_index;
		ld_module[ mod_number ].first_seen = 
			ld_module[ mod_number ].last_seen = -1;

		module[ mod_number ].dreloc = relindex - 1;
		module[ mod_number ].a_dlt = local_dlt;
		module[ mod_number ].flags = 0;
		mod_number++;
	}
	/* This object number is used to keep the linked list pointers running
	 * through the symbol table straight.
	 */
	object_number++;

#ifdef	PICR
	pic &= (filhdr.a_miscinfo & (M_PIC | M_DL));
#endif

	if( (shlib_level == SHLIB_DLD || shlib_level == SHLIB_BUILD) && 
	    !Rflag )
		relindex += filhdr.a_drelocs;

	fseek(text,sloc + LESYMPOS, 0);	/* skip to symbols */

	if( filhdr.a_lesyms > 0 )
	{
		lst = (char *) malloc(filhdr.a_lesyms);
		if ( fread((char *) lst, 1, filhdr.a_lesyms, text)
			!= filhdr.a_lesyms )
#pragma BBA_IGNORE
			fatal (e4, filename);

		/* Read in the supplemental symbol table, if one is present.
		 * We also malloc() up extra arrays of the same size to hold
		 * symtab[] indices and other information.
		 *
		 *    lst_size       -- size of the symbol table (in elements).
		 *    lst_sup        -- the supsym table as recorded in the
		 *             file itself.
		 *    lst_slot       -- records the slot in the regular 
		 *             symbol table for each symbol in the object's symbol
		 *             table.  Becomes important when we need to map
		 *             between the object's symbol table numbers and the
		 *             regular symbol table slots.
		 *    lst_used       -- this array is used to find the head 
		 *             pointers in the object's sup sym table; as each
		 *             symbol is the target of another pointer in the
		 *             supsym table, an element of this array is set.
		 */
		if( shlib_level == SHLIB_BUILD || (rflag && supsym_gen) )
		{
			if( filhdr.a_supsym > 0 &&
			    filhdr.a_spared == 0 && filhdr.a_spares == 0 )
			{
                lst_size = filhdr.a_supsym / sizeof( struct supsym_entry );
				lst_sup = (struct supsym_entry *) malloc( filhdr.a_supsym );
				lst_slot_save = lst_slot = 
					(long *) malloc( sizeof(long) * lst_size );
				lst_used = (char *) calloc( lst_size, sizeof( char ) );

				if( fread((char *) lst_sup, 1, filhdr.a_supsym, text ) 
				   != filhdr.a_supsym )
#pragma  BBA_IGNORE
					fatal( e4, filename );
			}
			else if( rflag )
				supsym_gen = 0;
			else
				fatal( e44, filename );
		}
	
		/* Pass1 inbolves loading an object module and then determining if
		 * the object was actually needed.  Before shared libs, we didn't
		 * have to do anything speacial to the symbol table, since it
		 * was not altered except for definitions, and any definition would
		 * keep the object module.  However, with shlibs, bits are set
		 * depending on ANY reference in a file (namely, the 
		 * NLIST_DLT/PLT/DRELOC flags are set by ANY file).  Thus, we need
		 * to restore these flags if the file is not kept.  Changed_head
		 * forms a linked list which runs through the symbol table, holding
		 * symbols which need to be converted back.
		 */
		changed_head = -1;
		for( y = 0, x = 0; x < filhdr.a_lesyms; y++ )
		{
			p1 = (char *) &(cursym.s);
			p2 = & lst[x];
			for (i = NLIST_SIZE; i > 0; i--)
				*p1 ++ = *p2 ++ ;
			x += NLIST_SIZE;
			for (i = 0; i < cursym.s.n_length; i ++)
				csymbuf[i] = lst[x++] ;
			cursym.sname = csymbuf;
			if( rflag && supsym_gen )
			{
				/* if end of list (size), mark as end in "used" array, too */
				if( cursym.s.n_flags & NLIST_LIST_FLAG )
					lst_used[ y ] |= USED_LIST_FLAG;
				if ((cursym.s.n_type & FOURBITS) == UNDEF)
					lst_used[y] |= USED_UNDEF_FLAG;
			}
			else if( shlib_level == SHLIB_BUILD )
			{
				/* USED_TARGET mean this is *not* the head of the list -
				   ie, it was pointed to by something else.
				   Thus later we can find the heads of the lists -
				   they are the ones whose USED_TARGET bit is *not* set.
				   USED_END may or may not be useful. */
				if( !(cursym.s.n_flags & NLIST_LIST_FLAG) )
					lst_used[ lst_sup[ y ].a.next ] |= USED_TARGET;
				/* mark undefs as targets, so they don't show up as heads */
				else if( (cursym.s.n_type & FOURBITS) == UNDEF )
					lst_used[ y ] |= USED_TARGET;
				else
					lst_used[ y ] |= USED_END;
			}
			else if( shlib_level == SHLIB_A_OUT || shlib_level == SHLIB_NONE )
			{
				/* you don't really need drelocs in an a.out
				   (propagates are handled later */
				cursym.s.n_flags &= ~(NLIST_DRELOC_FLAG);
			}

			ndef += sym1();	/* process one symbol */
			if( rflag && supsym_gen )
			{
				if( cursym.s.n_type & EXTERN )
					*(lst_slot++) = lastsym->sindex;
				else
					*(lst_slot++) = -1;
			}
			else if( shlib_level == SHLIB_BUILD )
			{
				if( !(cursym.s.n_type & EXTERN) )
					*(lst_slot++) = -1;
				else
				{
					*(lst_slot++) = lastsym->sindex;

					/* if this is an imported reference */
					if( (cursym.s.n_flags & NLIST_DLT_FLAG ||
						 cursym.s.n_flags & NLIST_DRELOC_FLAG ||
					       ((cursym.s.n_type & FOURBITS) == UNDEF &&
					         cursym.s.n_value == 0)) &&
					    lastsym->sindex >= dont_import_syms )
					{
						if( mod_index >= ld_mod_index )
						{
							ld_mod_index *= 2;
							module_next = (long *) realloc( module_next, 
												ld_mod_index * sizeof(long) );
						}
						/* create linked list of module references to symbol
						   use 'shlib' field to point to head */
						module_next[ mod_index ] = lastsym->shlib;
						lastsym->shlib = mod_index++;

						if( lastsym->next_num < object_number 
							&& lastsym->sindex < savindex )
						{
#pragma		BBA_IGNORE
							lastsym->next = changed_head;
							changed_head = lastsym->sindex;
						
							lastsym->flags = lastsym->s.n_flags;
						}
						/* we are not sure this is needed -
						   at this point we know there will be an import
						   so setting this flag won't cause another one */
						lastsym->s.n_flags |= NLIST_DRELOC_FLAG;
					}
				}
			}
			else if( lastsym->s.n_flags & NLIST_STILL_FUZZY_FLAG )
			{
			    /* we had a fuzzy ref/def, and now we have a real (.o) one */
				if( lastsym->next_num < object_number 
				    && lastsym->sindex < savindex )
				{
#pragma		BBA_IGNORE
					lastsym->next = changed_head;
					changed_head = lastsym->sindex;
				
					lastsym->flags = lastsym->s.n_flags;
				}

				/* it is not fuzzy any more cause a .o just ref/def'ed it */
				lastsym->s.n_flags &= ~(NLIST_STILL_FUZZY_FLAG);
				/* we don't understand this one either,
				   but it looks important */
				if( lastsym->shlib == -1 )
					lastsym->s.n_flags &= ~(NLIST_EVER_FUZZY_FLAG);
			}
		}
		/* If none seen, mark this symbol as having no entries */
		if( shlib_level == SHLIB_BUILD && mod_index == savmod_index )
			module[ mod_number-1 ].module_imports = -1;

		if( lst_slot )
			assert( lst_slot - lst_slot_save == lst_size );
	}
	else if (shlib_level == SHLIB_BUILD)
		module[mod_number-1].module_imports = -1;

	if (libflg==0 || ndef) {
		tsize += filhdr.a_text;
		dsize += filhdr.a_data;
		bsize += filhdr.a_bss;
		ssize += nloc;			/* count local symbols */
		rtsize += filhdr.a_trsize;
		rdsize += filhdr.a_drsize;
		headersize += debugheader.e_spec.debug_header.header_size;
		gnttsize += debugheader.e_spec.debug_header.gntt_size;
		lnttsize += debugheader.e_spec.debug_header.lntt_size;
		sltsize += debugheader.e_spec.debug_header.slt_size;
		vtsize += debugheader.e_spec.debug_header.vt_size;
		xtsize += debugheader.e_spec.debug_header.xt_size;

#ifdef	VFIXES
		if ((unsigned)filhdr.a_stamp <= HIGHEST_VERSION)
			version_seen[filhdr.a_stamp] = 1;
		else
#pragma BBA_IGNORE
			error("unknown version number in file %s",filename);
#else
		if (filhdr.a_stamp == 0)
			error("(warning) - old (pre-6.5) file %s may be incompatible with newer files",filename);
		if (filhdr.a_stamp > highest_seen)
			highest_seen = filhdr.a_stamp;
#endif

		if( rflag && supsym_gen )
		{
			/* We need to do a similar thing to what is done below, but
			 * we don't have to worry about an export list.  We need to
			 * generate a supsym table, which is easy, since we know
			 * where each pointer was mapped.  Problem: what to do
			 * about local symbols in the linked lists?  We handle this
			 * case by patching the linked list around the effected
			 * local.  The patching needs to be done before we enter the
			 * main loop where the actual supsym table is generated.
			 */
			for( x = 0; x < lst_size; x++ )
			{
				/* This is the case for locals.  The only case which
				 * really matters is the case where the symbol is 
				 * a target of some other symbol.  
				 */
				if( lst_slot_save[ x ] == -1 )
				{
					/* look for the pointer to this guy. 
					 */
					for (y = 0; y < lst_size && 
#ifdef	PICR
					            (
#endif
					             lst_sup[y].a.next != x
#ifdef	PICR
					             ||
#else
					             &&
#endif
					             (lst_used[ y ] & USED_LIST_FLAG)
#ifdef	PICR
					            )
#endif
					            ;
					     y++)
						;
					/* if y == lst_size, we must have had a "head" -
					   that is, no one pointed to him;
					   in that case don't do anything special to unlink it */
					if( y != lst_size )
					{
						/* unlink the local guy - his "parent" inherits
						   his "next"/"size" field (and LIST flag) */
						lst_used[ y ] = lst_used[ x ];
						lst_sup[ y ].a = lst_sup[ x ].a;
					}
				}
			}

			/* Now that all the locals are gone, go ahead and build the
			 * supsym table.  Note that we reset the NLIST_LIST_FLAG since
			 * this could have been altered from above.
			 */
			for( x = 0; x < lst_size; x++ )
			{
				if (!(lst_used[x] & USED_UNDEF_FLAG)
				    && (y = lst_slot_save[x]) != -1)
				{
					if( lst_used[ x ] & USED_LIST_FLAG )
					{
						supsym[ y ].a.size = lst_sup[ x ].a.size;
						symtab[ y ].s.n_flags |= NLIST_LIST_FLAG;
					}
					else
					{
						supsym[ y ].a.next = 
							lst_slot_save[ lst_sup[ x ].a.next ];
						symtab[ y ].s.n_flags &= ~(NLIST_LIST_FLAG);
					}
				}
			}
		}
		else if( shlib_level == SHLIB_BUILD )
		{
			for( x = 0; x < lst_size; x++ )
			{
				/* This if statement will find the beginning of a linked list
				 * of symbols.  Anything marked with USED_TARGET was pointed
				 * at; thus anything else must be a head pointer.
				 */
				if( !(lst_used[ x ] & USED_TARGET) )
				{
					/* We must now find the end of the list (so we
					 * can recover the size of the symbol).
					 */
					for( y = x; !(lst_used[y] & USED_END); 
						        y = lst_sup[ y ].a.next ) ;

					/* The size should now be in lst_sup[ y ].  Now
					 * we again go through the list, plugging the proper
					 * sizes into the export list.  At this time, there
					 * is no reason to keep the size in the symtab, so this
					 * is not done (but if it were, this would be the
					 * place for it).  Notice that we must be careful here
					 * to insure that each symbol that has an export list
					 * entry is altered (but ones without them are not).
					 */
					last = -1;               
					z = -1;
					do
					{
						z = (z == -1 ? x : lst_sup[ z ].a.next );

						if( lst_slot_save[ z ] != -1 &&
						    (t = symtab[ lst_slot_save[ z ] ].expindex) >= 0 )
						{
							exports[ t ].size = lst_sup[ y ].a.size;
							if( last == -1 )
								first = t;
							else								
								shl_exports[ t ].next_symbol = last;
							last = t;
						}
					}
					while( z != y );
					if( last != -1 )
						shl_exports[ first ].next_symbol = 
							                ( last != first ? last : -1 );
				}
			}
		}

		if( lst_slot_save )  free( lst_slot_save );
		if( lst_used )       free( lst_used );
		if( lst_sup )        free( lst_sup );
		if( lst )  			 free( lst );

		return(1);
	}


	/*
	 * No symbols defined by this library member.
	 * Rip out the hash table entries and reset the symbol table.
	 */
	while (symindex>savindex) 
	{
	  x = symtab[--symindex].shash;
	  hashmap[x] = chain[symindex];
    }

	if (lst) 
		free (lst);
	if( lst_slot_save )
		free( lst_slot_save );
	if( lst_used )
		free( lst_used );
	if( lst_sup )
		free( lst_sup );

	/* 
	 * Go through changed symtab entries, 
     * restoring flags to what they used to be.
	 */
	for( ; changed_head != -1; changed_head = symtab[ changed_head ].next )
	{
		symtab[ changed_head ].s.n_flags = symtab[ changed_head ].flags;
#ifdef	RESTORE_MOD
		symtab[changed_head].shlib = symtab[changed_head].save_shlib;
		symtab[changed_head].expindex = symtab[changed_head].save_expindex;
#endif
	}

	expindex    = savexpindex;
	stringindex = savstringindex;
	sym_stringindex = savsym_stringindex;
	dltindex = savdltindex;
	local_dlt = savlocal_dlt;
	pltindex = savpltindex;
	relindex = savrelindex;
	mod_index = savmod_index;
	mod_number = savmod_number;
#ifdef	PICR
	pic = savpic;
	nloc_items = savnloc_items;
#endif
	return(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* sized_data -	record size of DATA item as required by COMM declaration
		keep a linked list of such items
*/

sized_data (sp, size, offset)
struct symbol *sp;
long size;
long offset;
{
	register sdp p, q;
	sdp back;

	/* find entry for this symbol */
	for (p = first_sized_data; p != NULL; p = p->sdnext)
		if (p->sdsp == sp)
			break;
	/* create new entry if necessary */
	if (p == NULL)
	{
		p = (sdp) malloc(sizeof(*p));

		p->sdfollow = 0;
		p->sdvalue = size;
		p->sdsp = sp;
		/* insert into list ordered by offset */
		for (q = first_sized_data, back = NULL;
		     q && q->sdsp->s.n_value < offset;
		     back = q, q = q->sdnext)
			;
		p->sdnext = q;
		if (back)
			back->sdnext = p;
		else
			first_sized_data = p;
	}
	/* otherwise update old entry */
	else if (size > p->sdvalue)
		p->sdvalue = size;
}

#ifdef	MISC_OPTS
check_ylist (const char *name, int type, int value)
{
	generic p;

	for (p = trace_list; p != NULL; p = p->next)
	{
		if (!strcmp(name,p->name))
		{
			static char typechararry[] = { 'u', 'a', 't', 'd', 'b' };
			char typechar;

			typechar = typechararry[type&FOURBITS];
			if (type & EXTERN)
				typechar = toupper(typechar);
			if (typechar == 'U' && value)
				typechar = 'C';
			printf("%s: %s is %c\n",filename,p->name,typechar);
			break;
		}
	}
}
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* sym1 -	Process pass1 symbol definitions.  This involves flushing
		un-needed symbols, and entering the rest in the symbol table.
		Returns 1 if a symbol was defined and 0 else.
*/

sym1()
{
	register symp sp;
	register unsigned char ctype = cursym.s.n_type; /* type of the current symbol */
	register unsigned char ltype;
	register long cvalue;

#ifdef	MISC_OPTS
	if (trace_list != NULL)
		check_ylist(asciz(cursym.sname,cursym.s.n_length),cursym.s.n_type,cursym.s.n_value);
#endif
	/* if symbol is local, deal with it and get out of here */
	if ((ctype&EXTERN) == 0)
	{
	   if (cursym.s.n_flags & NLIST_DLT_FLAG
#ifdef	PICR
	       && !rflag
#endif
	      )
	       ++local_dlt;
	   if (!xflag || (cursym.s.n_flags & NLIST_DLT_FLAG))
	   {
		   nloc += NLIST_SIZE + cursym.s.n_length;
		   nloc_items++;
	   }
	   return(0);
	}
	symreloc(); 			/* relocate symbol in file */
	/* symreloc modifies cursym.s.n_type and cursym.s.n_value */

	/* install symbol in table */
	if (enter(lookup())) 
	{
#ifdef	VFIXES
		lastsym->version = filhdr.a_stamp;
#endif
		return(0);
	}

	/* If we get here, then symbol was already present.
	   If symbol is already defined then return; multiple
	   definitions are caught later.   If current symbol is
	   not EXTERN e.g. it is local, let the new one override */
	/* local symbols should never have survived this far */

	ltype = (sp = lastsym)->s.n_type;
	ctype = cursym.s.n_type;

	/* both new and old symbols have to be extern at this point */
	if (!(ctype & EXTERN) || !(ltype & EXTERN))
#pragma BBA_IGNORE
		bletch("internal error - local symbol in symbol table\n");

	/*
	 * if current symbol is primary def and the one in the
	 * symbol table is secondary, let the primary override
	 * Assumption: EXTERN2 cannot be set unless EXTERN is set
	 */

	if ((ltype & EXTERN2)
	    && (ctype & FOURBITS) != UNDEF
	    && !(ctype & EXTERN2))
	{
	    sp->s.n_type = ctype;
	    sp->s.n_value = cursym.s.n_value;
#ifdef	VFIXES
	    sp->version = filhdr.a_stamp;
#endif
	    return (1);
	}

	/* keep old entry if it was an external _definition_ */
		/* if old was DATA and this is COMM, keep track of it */
		if ((ltype & FOURBITS) == DATA
		    && (ctype & FOURBITS) == UNDEF
		    && cursym.s.n_value > 0)
			sized_data(sp,cursym.s.n_value,sp->s.n_value);
		if ((ltype & FOURBITS) != UNDEF) return(0);

	/* at this point we know old entry was extern undef */
	cvalue = cursym.s.n_value;
	/* if new entry is also extern undef */
	/* then we want it only if it is comm and is bigger */
	if ((ctype & FOURBITS) == UNDEF)
	{
		/* Check for .comm symbols. They will have a non-zero value. */
		if (cvalue>sp->s.n_value) sp->s.n_value = cvalue;
		/* OK, I lied before.
		   We really need secondary comm to override primary. */
		sp->s.n_type |= ctype;
		return(0);
	}
	/* if this is text, don't use it to satisfy comm */
	if (sp->s.n_value != 0
	    && (ctype & FOURBITS) == TEXT)
		return(0);

	/* in order to guarantee a clean ANSI/POSIX C namespace */
	/* don't use secondary data to satisfy 'secondary' comm */
	if ((ltype & EXTERN2)
	    && (ctype & FOURBITS) == DATA
	    && (ctype & EXTERN2))
		return(0);

	/* so now we know:
		1. old entry was extern undef
		2. new entry is extern _definition_
		3. if old entry was comm, this one is not text or comm
		   keep track of any potential size problems
	   in any case, we have a new definition
	*/
	if (sp->s.n_value > 0
	    && (ctype & FOURBITS) == DATA)
		 sized_data(sp,sp->s.n_value,cvalue);
	sp->s.n_type = ctype;	/* define something new */
	sp->s.n_value = cvalue;
#ifdef	VFIXES
	sp->version = filhdr.a_stamp;
#endif
	return(1);
}

