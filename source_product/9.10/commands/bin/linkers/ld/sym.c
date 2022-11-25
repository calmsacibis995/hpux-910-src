/* Internal RCS $Header: sym.c,v 70.1 91/11/13 14:44:11 ssa Exp $ */

/* @(#) $Revision: 70.1 $ */     

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

     Command:      Series 200 ld   ( HP -UX link editor )
     File   :      sym.c

     Purpose:      Handles symbol table operations and has other
                   utility routines.

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "ld.defs.h"

#define     CHAIN_TAG      0
#define     HASH_TAG       1
#define     LINK(tag,next) ((tag << 31) | next)
#define     HASH_KEY_SIZE  8
#define     LST_XF         500

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* getfile -    Open an input file as text.  Returns an indicator of the
                magic number of the file but leaves the file pointer at zero.

                Return value:   0 - error (incorrect magic number)
                                1 - RELOC_MAGIC
                                2 - AR_MAGIC
                                3 - SHL_MAGIC
*/

getfile(cp, libtype)
register char *cp;      /* name of the file to open */
int            libtype;
{
        register short c;
	MAGIC *fmagic;
        char arcmag[SARMAG+1];
        static char libname[512];
        int  i, w, type;

        filename = cp;
        text = NULL;
        if (cp[0] == '-' && cp[1] == 'l') 
		{
			if(cp[2] == '\0') cp = "-lc";

			for (i = 0; i < strlen(lpath); i += w + 1)
			{
				w = strcspn(&(lpath[i]), ":");
				strncpy(libname, &(lpath[i]), w);
				libname[w] = '\0';
				strcat(libname, "/lib");
				strcat(libname, &(cp[2]));
				switch (libtype)
				{
					case EITHER:
					case SHARED:
						strcat(libname,".sl");
						if ((text = fopen(libname,"r")) != NULL ||
						    libtype == SHARED)
							break;
						libname[strlen(libname)-3] = '\0';
					case ARCHIVE:
						strcat(libname,".a");
						if ((text = fopen(libname,"r")) != NULL)
							break;
				}
				if (text != NULL)
					break;
			}

			if (text == NULL) 
				fatal(e31, cp);
			filename = libname;
		}
        else if ((text = fopen(filename, "r")) == NULL)
			fatal(e15, filename);

        setvbuf(text,NULL,_IOFBF,8192);
        dread(arcmag, SARMAG, 1, text);
        if (strncmp(arcmag, ARMAG, SARMAG) == 0) return 2;
	fmagic = (MAGIC *)arcmag;

        if (fmagic->system_id != HP9000S200_ID) return 0;
        if (fmagic->file_type == RELOC_MAGIC) return 1;
        if (fmagic->file_type == SHL_MAGIC) return 3;
        if (fmagic->file_type == DL_MAGIC) return 4;
        if (fmagic->file_type == AR_MAGIC)
#pragma BBA_IGNORE
                fatal(e37, filename);
        if (strcmp(cp,Aname) == 0) return 1;
        return 0;

}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* lookup -     Returns the index into symtab of a symbol with the
                same name cursym.sname or -1 if none.
*/

int lasthash;   /* saves the result of the last hash call */

lookup()
{
        register int hash;
        register unsigned char *cp = (unsigned char *) cursym.sname;
        register symln = cursym.s.n_length;
        register int slot;
        register symp p;
		register unsigned g;

		/* This has function is taken from the Dragon book, pg 436.
		 * The old hashing function is included below.
		 */
		for( hash = 0, g = 0, slot = symln; slot; slot-- )
		{
			hash = (hash << 4) + *cp++;
			if( (g = hash & 0xF0000000) )
			{
				hash ^= (g >> 24);
				hash ^= g;
			}
		}	
		
		/* For now, zero length symbols really don't happen too often,
		 * and are checked for below.
		 */

		/* We don't need the "&~(0x8000000)" as below since this value
		 * cannot possibly grow to larger than 0x1000000.
		 */
		hash %= hash_cycle;

        lasthash = (signed) hash;

        if( (cursym.s.n_type & EXTERN) && symln )
		{
			for( slot = hashmap[ hash ]; slot >= 0; slot = chain[ slot ] ) 
			{
				p = &symtab[slot];
				
				if( (p->s.n_length == symln) &&
				    (p->s.n_type & EXTERN) &&
				    !strncmp( p->sname, cursym.sname, symln ) )
				         return( slot );
			}
		}

		return( -1 );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* slookup -    Like lookup but with an arbitrary string argument */

slookup(s)
char *s;
{
        cursym.sname = s;
        cursym.s.n_length = strlen(s);  /* no trailing 0 */
        cursym.s.n_type = EXTERN+UNDEF;
        cursym.s.n_value = 0;
		cursym.s.n_flags = 0;
        return(lookup());
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* enter -      Make sure that cursym is installed in symbol table.
                Called with an index into the symbol table for a symbol
                with the same name or  -1 if lookup failed.  Returns 1
                if the symbol was new, or 0 if it was already present.
*/

enter(slot)
register int slot;
{
        register symp sp;
        aligne a1,a2,a3;
        char *stralloc(), *sym_stralloc();
#ifdef	VISIBILITY
	int hide_me = 0;
#endif

#ifdef  VISIBILITY
	if (hide_status)
	{
		hide p;

		/* assume symbol not on list */
		hide_me = (hide_status == EXPORT_HIDES);
		for (p = hidelist; p != NULL; p = p->hide_next)
		{
			if (cursym.s.n_length == strlen(p->hide_name) && !strncmp(cursym.sname,p->hide_name,cursym.s.n_length))
			{
				hide_me = (hide_status == HIDE_HIDES);
				break;
			}
		}
	}
#endif

        if (slot == -1)
        {
                if (symindex >= ld_stsize) expand();
                lastsym = sp = &symtab[symindex];
                chain[symindex] = hashmap[lasthash];
                hashmap[lasthash] = LINK(CHAIN_TAG, symindex);
                sp->sname = sym_stralloc(cursym.s.n_length + 1);
                strncpy(sp->sname, cursym.sname, cursym.s.n_length);
                sp->sname[cursym.s.n_length] = NULL;
                if (*(sp->sname) == 0)
#pragma BBA_IGNORE
                        bletch("null symbol entered");
                sp->s.n_length = cursym.s.n_length;
                sp->s.n_type = cursym.s.n_type;
                sp->s.n_value = cursym.s.n_value;
				sp->next_num = 0;
#ifdef  VISIBILITY
				if (Fflag)
					sp->expindex = EXP_UNDEF;
				else if (bflag)
				{
					if (hide_status && hide_me)
						sp->expindex = EXP_DONT_EXPORT;
					else
						sp->expindex = EXP_UNDEF;
				}
				else
					sp->expindex = EXP_DONT_EXPORT;
#else
                sp->expindex = ( (Fflag || bflag) ? EXP_UNDEF :
                                                         EXP_DONT_EXPORT );
#endif
                sp->dltindex = DLT_UNDEF;
                sp->pltindex = PLT_UNDEF;
				sp->rimpindex = RIMP_UNDEF;
				sp->shlib = SHLIB_UNDEF;
				sp->s.n_flags = cursym.s.n_flags;
                sp->shash = lasthash;
                sp->sindex = symindex++;

                /* if it is an align symbol enter it in the linked list
                   of align symbols. Note that this linked list is kept
                   ordered w.r.t. the symbol value, so that when we add
                   white space we do it in the same order.              */
#ifdef	PICR
                if (sp->s.n_type & ALIGN)
					global_align_seen = 1;
#endif
                if ((sp->s.n_type&ALIGN) && !rflag)
                {
                        a1 = (aligne) calloc(ALIGN_ENTRY_SIZE, 1);
                        a1->modulo = cursym.s.n_almod;
#ifdef	ZERO_MODULO_FIX
						if (a1->modulo == 0)
							bletch("align symbol with zero modulo");
#endif
                        a1->asymp = sp;
                        a1->nextale = NULL;
                        if (alptr == NULL) alptr = a1;
                        else
                        {
                           for (a2 = alptr, a3 = NULL ;
                                a2 && a2->asymp->s.n_value < cursym.s.n_value;
                                a3 = a2, a2=a2->nextale) ;
                           a1->nextale = a2;
                           if ( a3 ) a3 -> nextale = a1;
                           else alptr = a1;
                        }
                }
                sp->s.n_almod = (rflag) ? cursym.s.n_almod : 0;

				/* If this is a definition for the symbol, we create an
				 * export list entry for it.  All links are created through
				 * the export list at this point except for the list of the
				 * same symbols with this value.  These are done in pass1.c
				 * after it has been determined that this module will indeed
				 * be included.
				 */
				if( shlib_level == SHLIB_BUILD && 
				    (sp->s.n_type & FOURBITS) != UNDEF &&
				    (sp->s.n_type & EXTERN) )
				{
#ifdef	VISIBILITY
					sp->module = mod_number - 1;
				    if ((!hide_status) || (!hide_me))
					{
#endif
					sp->expindex = 
						ExpEnter( sp, sp->s.n_type, sp->s.n_value,
								  highwater, -1, -1, mod_number-1, 
							      ld_module[mod_number-1].first_seen );
					if( ld_module[mod_number-1].first_seen == -1 )
						ld_module[mod_number-1].first_seen = sp->expindex;
					else
						shl_exports[ld_module[mod_number-1].last_seen]
							.next_module = sp->expindex;
					ld_module[ mod_number-1 ].last_seen = sp->expindex;
#ifdef	VISIBILITY
					}
#endif
				}

                return(1);
        }
        else 
        {
			    /* Save the old flags, place new flags in, place on linked
				 * list of changed entries.
				 */
			    lastsym = &symtab[ slot ];
				if( lastsym->next_num < object_number )
				{
					lastsym->next = changed_head;
					changed_head = slot;
				
					lastsym->flags = lastsym->s.n_flags;
#ifdef	RESTORE_MOD
					/* save away old shlib field (used for module table) */
					lastsym->save_shlib = lastsym->shlib;
					lastsym->save_expindex = lastsym->expindex;
#endif
					lastsym->next_num = object_number;
				}

				lastsym->s.n_flags |= cursym.s.n_flags & ~NLIST_LIST_FLAG;

#ifdef	PICR
                if (cursym.s.n_type & ALIGN)
					global_align_seen = 1;
                if ((cursym.s.n_type & ALIGN) && !(lastsym->s.n_type & ALIGN) && !rflag)
                {
                        a1 = (aligne) calloc(ALIGN_ENTRY_SIZE, 1);
                        a1->modulo = cursym.s.n_almod;
#ifdef	ZERO_MODULO_FIX
						if (a1->modulo == 0)
							bletch("align symbol with zero modulo\n");
#endif
                        a1->asymp = lastsym;
                        a1->nextale = NULL;
                        if (alptr == NULL) alptr = a1;
                        else
                        {
                           for (a2 = alptr, a3 = NULL ;
                                a2 && a2->asymp->s.n_value < cursym.s.n_value;
                                a3 = a2, a2=a2->nextale) ;
                           a1->nextale = a2;
                           if ( a3 ) a3 -> nextale = a1;
                           else alptr = a1;
                        }
                }
                lastsym->s.n_almod = (rflag) ? cursym.s.n_almod : 0;
#endif

				/* If this is a definition for the symbol, we create an
				 * export list entry for it.  All links are created through
				 * the export list at this point except for the list of the
				 * same symbols with this value.  These are done in pass1.c
				 * after it has been determined that this module will indeed
				 * be included.
				 */
				if( shlib_level == SHLIB_BUILD &&
				    (cursym.s.n_type & FOURBITS) != UNDEF &&
				    (cursym.s.n_type & EXTERN)
#ifdef	VISIBILITY
				  )
				{
					lastsym->module = mod_number - 1;
				    if (((!hide_status) || (!hide_me)) && (lastsym->expindex != EXP_DONT_EXPORT))
					{
#else
				    && (lastsym->expindex != EXP_DONT_EXPORT) )
				{
#endif
					lastsym->expindex = 
						ExpEnter( lastsym, cursym.s.n_type, cursym.s.n_value,
								  highwater, -1, -1, mod_number-1, 
							      ld_module[ mod_number-1 ].first_seen );
					if( ld_module[ mod_number-1 ].first_seen == -1 )
						ld_module[mod_number-1].first_seen = lastsym->expindex;
					else
						shl_exports[ld_module[mod_number-1].last_seen]
							.next_module = lastsym->expindex;
					ld_module[ mod_number-1 ].last_seen = lastsym->expindex;
#ifdef	VISIBILITY
					}
#endif
				}

                return(0);
        }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* expand - Expand linker symbol table and associated data structures.
            Since there are pointers to the old symtab in local, Asymp,
            alptr and entrypt, these have to be updated. Also , since
            realloc does not initialize the expanded space, this is
            done for symtab, local and chain.
*/

expand()
{
	int           osymtab;          /* old value of symtab */
	aligne        a1;
	int           i;
	int           diff;             /* diff between old and new symtab */
	int           temp;
	sdp           p;

	/* Up the size of the symbol table, while keeping around the old
	 * pointer so we can calculate differences (below).  We also need
	 * to realloc the local and chain arrays, since these are the same
	 * size as the regular symbol table.
	 */
	osymtab = (int) symtab;
	ld_stsize += LST_XF;
	symtab = (struct symbol *) realloc( symtab, ld_stsize * SYMBOLSIZE );
	if( rflag && supsym_gen )
		supsym = (struct supsym_entry *)
			     realloc( supsym, ld_stsize * sizeof( struct supsym_entry ) );
	local = (symp *) realloc( local, ld_stsize * sizeof(symp) );
	chain = (int *) realloc( chain, ld_stsize * sizeof(int) );

	/* Since the symbol table should be '0' before we use it (we used calloc
	 * in main.c), we need to clear out the added space.
	 */
	memset( &symtab[symindex], 0, (ld_stsize - symindex) * SYMBOLSIZE );
	if( rflag && supsym_gen )
		memset( &supsym[symindex], 0, (ld_stsize - symindex) *
			                          sizeof( struct supsym_entry ) );
	memset( &local[symindex], 0, (ld_stsize - symindex) * sizeof(symp) );
	memset( &chain[symindex], 0, (ld_stsize - symindex) * sizeof(int) );

	/* A few pointers actually pointed into the symbol table (not
	 * by using indices, those are fine).  We need to add the difference
	 * of the old area and the new area to such pointers.
	 */
	diff = ((int) symtab) - osymtab;
	if (Asymp != 0) Asymp = (symp) ((int)Asymp + diff);
	if (entrypt) entrypt = (symp) ((int)entrypt + diff);
	for (p = first_sized_data; p != NULL; p = p->sdnext)
		p->sdsp = (symp) ((int)p->sdsp + diff);
	if (alptr) 
		for(a1 = alptr; a1; a1 = a1 -> nextale)
			a1 -> asymp = (symp) ((int)a1->asymp + diff);

	for (i = 0; i < ld_stsize - LST_XF; i++ )
		if ( local[i] )
            local[i] = (symp) ((long)local[i] + diff);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* symreloc -   Perform partial relocation of symbol.  Each symbol is
                relocated twice.  The first time, it is adjusted to
                is relative position within its segment.  The second
                time, it is adjusted by the start of the final segment
                to which that symbol refers.  This routine only
                performs the first relocation.
*/

symreloc()
{

        if (funding) return;
        switch (cursym.s.n_type & FOURBITS) {
        case TEXT:
                cursym.s.n_value += ctrel;
                return;

        case DATA:
                cursym.s.n_value += cdrel;
                return;

        case BSS:
                cursym.s.n_value += cbrel;
                return;

        case UNDEF:
                return;
#ifdef  BBA
        default:
                ;
#endif
        }
        if (cursym.s.n_type&EXTERN)
                cursym.s.n_type = EXTERN+ABS;
}

/*----------------------------------------------------------------------
 *
 * Procedure: readhdr()         (Read in a file header, adjusting stuff)
 * Description: 
 *    Reads in the 'exec' header from the open input file (stream 'text')
 *    and checks magic numbers and file types.  It also adjusts text,
 *    data, and bss sizes, version stamps, and high water marks.
 */
readhdr(pos)
long pos;
{
	register long st, sd;
        
	fseek( text, pos, 0 );
	dread( &filhdr, sizeof filhdr, 1, text );
	if( N_BADMACH(filhdr) || (filhdr.a_magic.file_type != RELOC_MAGIC &&
							  filhdr.a_magic.file_type != SHL_MAGIC &&
							  filhdr.a_magic.file_type != DL_MAGIC) 
	                      && strcmp( Aname, filename ) ) 
		error(e5, filename);

	st = (filhdr.a_text+1) & ~1;
	filhdr.a_text = st;
	cdrel = -st;
	sd = (filhdr.a_data+1) & ~1;
	cbrel = -(st + sd);
	filhdr.a_bss = (filhdr.a_bss+1) & ~1;

	this_version = filhdr.a_stamp;
	highwater = filhdr.a_highwater;
	if( high_highwater < highwater )
		high_highwater = highwater;
}       

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 * readdebugexthdr - Read in a debug extension header,
 */

readdebugexthdr(pos)
long pos;
{
        register long extoffset = pos + filhdr.a_extension;
        
	while (extoffset != pos) {
           fseek(text, extoffset, 0);
           dread(&debugheader, sizeof debugheader, 1, text);
	   if (debugheader.e_header == DEBUG_HEADER) {
	      break;
	   }
	   extoffset = pos + debugheader.e_extension;
	}
	if (extoffset == pos) {
	   debugheader.e_spec.debug_header.header_size = 0;
	   debugheader.e_spec.debug_header.gntt_size = 0;
	   debugheader.e_spec.debug_header.lntt_size = 0;
	   debugheader.e_spec.debug_header.slt_size = 0;
	   debugheader.e_spec.debug_header.vt_size = 0;
	   debugheader.e_spec.debug_header.xt_size = 0;
	}
}       

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* error -      Print out error messages and give up if they are severe. */

/*VARARGS1*/
error(fmt, a1, a2, a3, a4, a5)
char *fmt;
{
        fprintf(stderr,"ld: ");
        fprintf(stderr,fmt, a1,a2,a3,a4,a5);
        fprintf(stderr,"\n");
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* fatal -      Print error message and exit */

/*VARARGS1*/
fatal(fmt, a1, a2, a3, a4, a5)
char *fmt;
{
        error(fmt, a1, a2, a3, a4, a5);
        error(e33);
        unlink(tempname);
        exit(1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* bletch -     Print out a message and abort.  Used for internal errors. */

#ifdef  BBA
#pragma BBA_IGNORE
#endif
/*VARARGS1*/
bletch(fmt, a1, a2, a3, a4, a5)
char *fmt;
{
        error(fmt, a1, a2, a3, a4, a5);
        abort();
}

/*----------------------------------------------------------------------
 * 
 * Procedure: dcopy()               (Copy input to output, quickly)
 * Description:
 *    dcopy() copies from one stream to another, in a fast manner and
 *    checking for errors as we go.  This piece of code is highly 
 *    specific to HPUX, as it takes advantages of internal buffering
 *    and other nasties, to be as quick as possible.
 */
dcopy(in, out, count)
register FILE *in, *out;
register long count;
{
	register mincnt;
	register int c;

	assert( count >= 0 );
	while(count--)
	{
		if ((c = getc(in)) == EOF)
		{
#pragma BBA_IGNORE
			if (feof(in)) fatal(e3, filename);
			if (ferror(out)) fatal(e4, filename);
		}

		putc(c, out);

		if ((mincnt = (in->_cnt <= out->_cnt) ? in->_cnt : out->_cnt) <= 0)
			continue;

		mincnt = count <= mincnt ? count : mincnt;

		memcpy(out->_ptr, in->_ptr, mincnt);

		in->_cnt  -= mincnt;
		in->_ptr  += mincnt;
		out->_cnt -= mincnt;
		out->_ptr += mincnt;
		count     -= mincnt;
	}
}

/*----------------------------------------------------------------------
 *
 * Procedure: zout()             (output a bunch of zeroes to a stream)
 * Description:
 *    zout() will push 'count' number of zeroes on to the given stream.
 */
zout(stream, count)
FILE *stream;
register int count;
{
	while( count-- )
		putc( 0, stream );
}

/*----------------------------------------------------------------------
 * 
 * Procedure: dread()        (Like fread, but checks for errors and eof)
 * Description:
 *    dread() does and fread() with the exact same parameters, and then
 *    chacks for any errors or eof, and produces the proper fatal
 *    errors if either of these are seen.
 */
dread(pos, size, count, file)
void *pos;
int size;
int count;
register FILE *file;
{
        if (fread(pos, size, count, file) == size * count) return;
        if (feof(file))
#pragma BBA_IGNORE
                fatal(e3, filename);
        if (ferror(file))
#pragma BBA_IGNORE
                fatal(e4, filename);
}

/*----------------------------------------------------------------------
 *
 * Procedure: asciiz()               (terminate a non-null string)
 * Description:
 *    asciiz() will convert a non-null terminated string (which is passed
 *    as a string and a length) into a normal null terminated string,
 *    and will return this new string to the caller.
 */
char *asciz(sp, l)
register char *sp;
register int l;
{
	register char *asp;
	static char ascizs[SYMLENGTH];

	assert( l > 0 );
	for( asp = ascizs; l > 0; l-- )
		*(asp++) = *(sp++);
	*asp = '\0';

	return( ascizs );
}

/*----------------------------------------------------------------------
 *
 * Procedure: make_prime()           (find a prime number close to num)
 * Description:
 *    This procedure will return a prime number close to the argument
 *    given to it (actually less than it).  If it can't find a suitable
 *    prime (it has a fixed table of primes that it looks in), it will
 *    return the input parameter back to the caller.
 */
make_prime(num)
register int num;
{
	register int *p;
	static int primes[] = {
        503,        1009,        1511,        2003,        2503,
        3001,       3511,        4003,        4507,        5003,
        6007,       7001,        8009,        9001,       10007,
        12503,     15013,       20011,       25013,       30029,
		35401,     40039,		45181,		 50087,		  55021,
		60017,	   65053,		70003,		 75169,		  80021,
		85093,     90001,		95021,		 99991,           0
		};

	for( p = primes; num > *p && *p != 0; p++ ) ;
	return( *p ? *p : num );
}

#ifdef junk
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* acopy - copies ascii names from cursym.sname (no null terminator). */

acopy(sp)
register char *sp;
{
 register unsigned char i = cursym.s.n_length;
 register char *ss = cursym.sname;

 for (; i>0; i--) *sp++ = *ss++;
 return;
}

dump_sym_tab()
{
        register int i;

        fprintf(stderr,"Dumping Symbol Table\n");
        for(i=0;i<symindex;i++)
                dump_sym(&symtab[i]);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
dump_sym(p)
register symp p;
{
        if (p)
        {       fprintf(stderr,"\nsymbol %s:\n",p->sname);
                fprintf(stderr,"value is %X, stincr is %X\n",p->s.n_value,
                        p->stincr);
                fprintf(stderr,"sincr is %X\n",p->sincr);
        }
        else    fprintf(stderr,"NULL symbol ptr\n");
}
#endif

/*----------------------------------------------------------------------------
 *
 * This is stolen from shlib.c (stralloc) 
 */
char *sym_stralloc(bytes)
int bytes;
{
	register struct symbol *sp, *sp_end;
	register long diff;
	char *old_stringt;

	if (sym_stringindex + bytes >= ld_sym_stringsize) 
	{
		ld_sym_stringsize *= 2;
		old_stringt = sym_stringt;
		sym_stringt = (char *) realloc(sym_stringt, ld_sym_stringsize);

		/* Change all the pointers in the symbol table if the realloc'ed
		 * area has indeed moved.
		 */
		if( (diff = sym_stringt - old_stringt) != 0 )
		{
			for( sp_end = symtab + symindex, sp = symtab; sp < sp_end; sp++ )
			{
				if( !(sp->s.n_flags & NLIST_STRING_TABLE_FLAG) )
					sp->sname += diff;
			}
		}
	}
   
	sym_stringindex += bytes;
	return(sym_stringt + sym_stringindex - bytes);
}
