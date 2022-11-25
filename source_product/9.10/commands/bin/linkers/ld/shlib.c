/*======================================================================
 *
 * File: shlib.c                        (Shared library support for LD)
 *
 * Description:
 *    This file contains all of the shared library specific routines.
 *    Of course, there are hooks and such in the other routines of LD,
 *    but the actual routines are contained here.
 *
 *    You will notice the use of assert() throughout the code.  This is
 *    intended only for debuging versions, and should not be included
 *    in the production version (by #define'ing NDEBUG).  Any really
 *    possible error conditions are handled through error() or bletch().
 *
 */

/* Internal RCS $Header: shlib.c,v 70.6 93/10/26 10:10:13 ssa Exp $ */

#include "ld.defs.h"
#include <sys/param.h>     /* For MAXPATHLEN definition */

#ifdef	BBA
char *stralloc();
long foffset();
#endif

/*----------------------------------------------------------------------
 * Globals:
 *   These symbols are required for sharing of data between various 
 *   routines within this file, but not needed elsewhere; thus, they
 *   are here, and marked static.
 */

/* The following globals are used by AddShlib() and the various procedures
 * in the tree of enter_shlib_symbol -> fuzzy_define -> real_define, etc.
 * These are mostly pointers to various parts of the shared library's tables
 * which are read into memory by AddShlib()
 */
static struct import_entry *ie_table;
static struct export_entry *ee_table;
static struct shl_export_entry *see_table;
static char *ee_done;
static long hash_size;
static struct hash_entry *hash_table;
static char *string_table;
static struct module_entry *mod_table;   
static struct dmodule_entry *dmodule_table;
static long *dmodule_next;
#define  DMODULE_END    -2   /* Any negative value will do here */
#define  DMODULE_UNDEF  -1   /* HAS TO BE THIS VALUE (see memset(), below) */
static int fuzzy_align_dsize;

/* These two are used to seek back for the synamic relocation records
 * in WriteShlibText() and WriteDynReloc() only
 */
static long dyn_seek;    /* Contains file position for dynamic relocations */
static long exp_seek;    /* Contains file posotion for export list */


/*----------------------------------------------------------------------
 *
 * Procedure: enter_string_table()   (Place string into shlib string table)
 *
 * Description:
 *    This routine is called when it is realized that a certain name will
 *    be needed by the shlib code, in the form of an export list entry,
 *    an import list entry, etc.  The old string is left where it is
 *    (because it would be too hard to get space back from the ld
 *    string table).
 * 
 */
enter_string_table( sp )
struct symbol *sp;      
{
    char *str;

    if( !(sp->s.n_flags & NLIST_STRING_TABLE_FLAG) )
    {
	str = stralloc( sp->s.n_length + 1 );
	strncpy( str, sp->sname, sp->s.n_length );
	str[ sp->s.n_length ] = '\0';

	sp->sname = str;

	sp->s.n_flags |= NLIST_STRING_TABLE_FLAG;
    }
}

/*----------------------------------------------------------------------
 * 
 * Procedure: stralloc()          (allocate space in shlib string table)
 *
 * Description:
 *    This procedure will allocate space in the shlib string table for 
 *    a given string.  Most symbol table entries start out by calling
 *    sym_stralloc() for their space, which is ld's internal string table.
 *    At some point, it may become clear that the string needs to be
 *    in this space, at which time, space is allocated here and the
 *    original name is copied (no attempt is made to recover the space
 *    for sym_stralloc().
 */
char *stralloc(bytes)
int bytes;
{
    register struct symbol *sp, *sp_end;
    register long diff;
    char *old_stringt;

    /* If there is not enough space in the current malloc()ed area, call
     * realloc() and fix up any pointers in the symbol table.
     */
    if( stringindex + bytes >= ld_stringsize )    
    {
	ld_stringsize *= 2;
	old_stringt = stringt;
	stringt = (char *) realloc(stringt, ld_stringsize);

	/* Change all the pointers in the symbol table if the realloc'ed
	 * area has indeed moved.  The loop tries to be as quick as possible,
	 * since the symbol table can be quite large.
	 */
	if( (diff = stringt - old_stringt) != 0 )
	{
	    for( sp_end = symtab + symindex, sp = symtab; sp < sp_end; sp++ )
	    {
		if( sp->s.n_flags & NLIST_STRING_TABLE_FLAG )
		    sp->sname += diff;
	    }
	}
    }

    stringindex += bytes;
    return( stringt + (stringindex - bytes) );
}

/*----------------------------------------------------------------------
 *
 * Procedure: DltEnter()                 (Make a DLT entry for a symbol)
 *
 * Description:
 *    When shlib_tables() does its scan of the symbol table, it may find
 *    that a symbol requires a DLT.  If this is the case, DltEnter creates
 *    Dlt entry in the DLT table.  It returns the index into the DLT table
 *    that will be placed into the symbol table (under .dltindex).
 *    This also creates the import list entry for the DLT, so DltReloc() 
 *    can do it's thing, and so dld.sl can as well.
 */
DltEnter( sp, seg, val, highwater)
struct symbol *sp;
int    seg;
long   val;
int    highwater;
{
   register struct import_entry *impp;

   /* If the symbol is an absolte symbol, it's DLT is special (since we
    * don't want dld.sl to touch it at run time).  These DLT's get placed
    * before the regular DLT's in the dlt table.  This is only a problem
    * for dld itself, since it just adds start of text to all of its
    * own DLT's (and has no information on which ones are ABS).
    */
   if( (seg == ABS) && (shlib_level == SHLIB_DLD) )
   {
       /* Our usual realloc code 
	*/
       if( abs_dltindex == ld_abs_dltsize ) {
	   ld_abs_dltsize *= 2;
	   abs_dlt = (struct dlt_entry *)
	       realloc(abs_dlt, ld_abs_dltsize * sizeof(struct dlt_entry));
       }

       abs_dlt[ abs_dltindex ].address = (char *) val;
       return( ABS_DLT_FLAG | (abs_dltindex++) );         /**************/
   }
   else
   /* In the normal case (non-ABS), we place the DLT in the normal DLT
    * table, and record information about what type this value is, since
    * we must do relocation later (after the middle pass; see DltReloc())
    */
   {
       /* Our usual realloc code (multiply by 2, etc.
	*/
       if (dltindex == ld_dltsize) {
	   ld_dltsize *= 2;
	   dltimports = (struct import_entry *) 
	       realloc(dltimports, ld_dltsize * sizeof(struct import_entry));
	   dlt = (struct dlt_entry *) 
	       realloc(dlt, ld_dltsize * sizeof(struct dlt_entry));
       }

       impp = dltimports + dltindex;
   /* Record the name, type and shlib  of the DLT entry,
	* for the import list (also used in DltReloc()).
	*/
       if( sp == NULL )
	   impp->name = 0;
       else
       {
	   enter_string_table( sp );
	   impp->name = sp->sname - stringt;
       }
       impp->type = seg;
       impp->shl = -1;

   /* Record the value of the DLT itself 
	*/
       dlt[ dltindex ].address = (char *) val;

       return( dltindex++ );
   }
}

/*----------------------------------------------------------------------
 *
 * Procedure: DltSet()            (Clobber a DLT entry, with new values)
 *
 * Description:
 *   This little routine simply sets the values of a DLT entry.  This 
 *   is useful in middle() and common(), since DLT values are being 
 *   reassigned in these routines.  Obviously, this is meant to work
 *   on a DLT that has already been allocated through DltEnter().
 */
DltSet( dltindex, val, seg )     
int dltindex, val, seg;
{
    assert( dltindex >= 0 );

    dlt[ dltindex ].address = (char *) val;
    dltimports[ dltindex ].type = seg;
}

/*----------------------------------------------------------------------
 *
 * Procedure: PltEnter()                           (Create a PLT entry)
 * 
 * Description:
 *    This procedure will create a PLT entry for a symbol, in much the
 *    same way that DltEnter creates a DLT entry.  The return value is 
 *    the index into the PLT table for the symbol given (presumed to 
 *    get put into .pltindex of the symbol table).  
 * 
 *    Unlike DltEnter, this procedure does not actually produce a PLT
 *    table.  Rather, it records the needed information for the import
 *    list and records the symbol number in 'pltchain' so that it can
 *    get the value of the procedure back again.  With this information,
 *    WriteShlibData() can create the PLT table on the fly.
 *
 */
PltEnter(sp, shl)
struct symbol *sp;
int    shl;
{
    register struct import_entry *impp;

    if (pltindex == ld_pltsize) {
	/* Bet you've seen this code alot by now...
	 */
	ld_pltsize *= 2;
	pltimports = (struct import_entry *) 
	    realloc(pltimports, ld_pltsize * sizeof(struct import_entry));
	pltchain = (long *) 
	    realloc(pltchain, ld_pltsize * sizeof(long));
    }

    enter_string_table( sp );

    /* Record information about this symbol in the import list entry
     */
    impp = pltimports + pltindex;
    impp->name = sp->sname - stringt;
    impp->shl = shl;
    impp->type = TYPE_PROCEDURE;

    /* Keep track of the symbols index, so that we can get its value
     * back later.
     */
    pltchain[pltindex] = sp->sindex;

    return( pltindex++ );
}

/*----------------------------------------------------------------------
 *
 * Procedure: DltReloc()              (Relocate values in the DLT table)
 *
 * Description:
 *    The DLT table is created at the end of pass1, by shlib_tables().
 *    Since final addresses are not computed until middle() is finished,
 *    the DLT's will need to relocated to their final location within the
 *    file, as opposed to their location within their respective segments.
 *    This is the task of this procedure.
 * 
 *    Another problem DltReloc() solves is that the types in the 
 *    export/import lists are not the same as those in the symbol table.
 *    Thus, we need to convert the types of the import list entries as 
 *    well.
 */
DltReloc()
{
    register int i;

    for (i = 0; i < dltindex; i++) {
	switch (dltimports[i].type) {

	  case UNDEF:
	    dltimports[i].type = TYPE_UNDEFINED;
	    dlt[i].address = 0;
	    break;

	  case ABS:
	    dltimports[i].type = TYPE_ABSOLUTE;
	    break;

	  case TEXT:
	    dltimports[i].type = TYPE_PROCEDURE;
	    dlt[i].address += (torigin + shlibtextsize);
	    break;

#ifdef	SHLPROCFIX
	  case EXTERN2+TEXT:
	    dltimports[i].type = TYPE_PROCEDURE;
	    dlt[i].address += plt_start;
	    break;
#endif

	  case DATA:
	    dltimports[i].type = TYPE_DATA;
	    dlt[i].address += dorigin;
	    break;

	  case BSS:
	    dltimports[i].type = TYPE_BSS;
	    dlt[i].address += borigin;
	    break;

	  case COMM:
	    dltimports[i].type = TYPE_COMMON;
	    dlt[i].address += corigin;
	    break;

	  default:
#pragma BBA_IGNORE
	    bletch( "Strange type in DLT list" );
	    break;
      }
   }
}

/*----------------------------------------------------------------------
 * 
 * Procedure: AddShlib()                 (Link against a Shared Library)
 * 
 * Description:
 *    This procedure will link against a shared library.  This is the
 *    shared library equivalent of pass1().
 *    
 */
int AddShlib(name, minus_l, bind)
char *name;
int   minus_l, bind;
{
    char *getcwd();
    long i, i_end, slot;
    char *c;
    struct export_entry *ee2;
    struct header_extension exthdr;
    struct dynamic dy;
    struct _dl_header *dl;
    int def = 0;
    char cwd[ MAXPATHLEN ];
    long dmodule_head;

    /* If we are building a shared library or building DLD, we can't include
     * a shared library in the link.
     */
    if( shlib_level == SHLIB_BUILD || shlib_level == SHLIB_DLD )
	fatal( e46, filename );

    /* Obviously, it doesn't make much sense to link against a shared library
     * while trying to use -r on some object modules.
     */
    if( rflag )
	fatal( e50, filename );

    /* Read in the exec header, get to the extension header, read it,
     * put 'dl' to point to the juicy dynamic load structure within the
     * extension header.
     */
    readhdr( 0L );
    if( filhdr.a_extension == 0L )
#pragma		BBA_IGNORE
	fatal( e53, filename );
    fseek( text, filhdr.a_extension, 0 );
    dread( &exthdr, 1, sizeof( struct header_extension ), text );
    dl = &exthdr.e_spec.dl_header;

    /* Sections that we need from the extension header.  These are used
     * mainly in enter_shlib_symbol() (below), but are actually malloced
     * and freed here.  Notice that one big malloc is done, one big read,
     * and then the individual pointers are added to the start of the
     * import table.
     */
    fseek( text, filhdr.a_extension + dl->import, 0 );
    ie_table = (struct import_entry *) malloc( dl->dreloc - dl->import );
    dread( ie_table, 1, dl->dreloc - dl->import, text );

    ee_table = (struct export_entry *) 
		     ((long)ie_table + (dl->export - dl->import));
    see_table = (struct shl_export_entry *)
		     ((long)ie_table + (dl->shl_export - dl->import));
    hash_table = (struct hash_entry *)
		     ((long)ie_table + (dl->hash - dl->import));
    string_table = (char *)
		     ((long)ie_table + (dl->string - dl->import));
    mod_table = (struct module_entry *)
		     ((long)ie_table + (dl->module - dl->import));
    dmodule_table = (struct dmodule_entry *)
		     ((long)ie_table + (dl->dmodule - dl->import));

    ee_done = (char *) calloc( dl->shl_export - dl->export, sizeof(char) );
    hash_size = (dl->string - dl->hash) / sizeof( struct hash_entry );

    /* Sections that we need from the DYNAMIC area.
     */
    fseek( text, dl->dynamic + TEXTPOS, 0 );
    dread( &dy, sizeof( struct dynamic ), 1, text );


    /* Enter_shlib_symbol() is an inherently recursive procedure.  You 
     * find one symbol that you need, and its module list tells you that
     * you need another symbol, and if it is found, the procedure continues.
     * The 'dmodule_next' array is used to keep track of which dmodule
     * list should be traversed next.  Thus, the recursion is kept in this
     * array, and not done directly.
     */
    i = ((dl->dreloc - dl->dmodule) / sizeof(struct dmodule_entry)) * sizeof(long);
    dmodule_next = (long *) malloc( i );
    memset( dmodule_next, 0xff, i );

#ifdef	ELABORATOR
	/* make sure initializer shows up as an undefine,
	   and that it is needed by a shared library */
	if ((dy.status & INITIALIZE) && (dy.initializer != -1))
	{
		if ((slot = slookup(string_table+ie_table[dy.initializer].name)) == -1)
		{
			enter(-1);
			slot = lastsym - symtab;
			symtab[slot].s.n_flags |= NLIST_STILL_FUZZY_FLAG;
		}
		symtab[slot].expindex = EXP_UNDEF;
	}
#endif
#ifdef SHLIB_AS_OBJECT
	if (1)
	{
		int isize, esize;

		isize = (dl->export - dl->import) / sizeof(struct import_entry);
		esize = (dl->shl_export - dl->export) / sizeof(struct export_entry);
		/* first enter all definitions */
		for (i = 0; i < esize; ++i)
		{
			slot = slookup(string_table + ee_table[i].name);
			if (slot == -1)
			{
				enter(-1);
				fuzzy_define(ee_table+i,see_table+i,lastsym-symtab);
			}
			else if (symtab[slot].s.n_flags & NLIST_STILL_FUZZY_FLAG)
				fuzzy_define(ee_table+i,see_table+i,slot);
			else
				real_define(ee_table+i,see_table+i,slot);
		}
#if 0
		for (i = 0; i < isize; ++i)
		{
			enter(slookup(string_table + ie_table[i].name));
			slot = lastsym - symtab;
			symtab[slot].s.n_flags |= NLIST_EVER_FUZZY_FLAG|NLIST_STILL_FUZZY_FLAG;
			symtab[slot].shlib = -1;
		}
#endif
	}
	else
	{
#endif
    /* Go through the entire symbol table looking for things which can
     * be resolved by things in the shared library.  This code could have 
     * also been written to use the export list in the same way (HELP)
     */
    for( i = 0, i_end = symindex;
#ifdef	EVIL_FIX
	     i < symindex;
#else
	     i < i_end;
#endif
	     i++ )
    {
#ifdef	FOLLOW_VERSION
	if (((symtab[i].s.n_type & FOURBITS) == UNDEF
	     || (symtab[i].s.n_flags & NLIST_EVER_FUZZY_FLAG)
	     || (symtab[i].shlib != SHLIB_UNDEF))
	    && ExpLookup(symtab[i].sname,-1,&ee2,string_table,ee_table,hash_table,hash_size)
	    && !ee_done[ee2-ee_table])
#else
	if( (symtab[i].s.n_type & FOURBITS) == UNDEF &&
	    ExpLookup( symtab[i].sname, -1, &ee2, string_table, ee_table, 
		       hash_table, hash_size ) &&
	   !ee_done[ ee2 - ee_table ] )
#endif
	{ 
	    enter_shlib_symbol( ee2 - ee_table, symtab[i].sindex );
	}
    }
#ifdef SHLIB_AS_OBJECT
	}
#endif

    /* Terminator for data copy list for this shared library,
     * and place segment alignment information in align and size fields.
     */
    shldata[ shldata_idx ].type = -1;
    shldata[ shldata_idx ].align = (4 - (dsize & 0x3)) & 0x3;
    dsize += shldata[ shldata_idx ].align;
    shldata[ shldata_idx ].size = (4 - (tsize & 0x3)) & 0x3;
    tsize += shldata[ shldata_idx ].size;
    shldata[ shldata_idx ].offset = (4 - (bsize & 0x3)) & 0x3;
    bsize += shldata[ shldata_idx ].offset;

    shldata_idx++;

    /* Free this shlib's tables 
     */
    free( ie_table );
    free( ee_done );

    /* If it is a relative pathname, we need to convert it to a full absolute
     * pathname (mainly, for security reasons).  
     */
#ifndef   DONT_GETCWD
    if( name[0] != '/' )
    {
	c = getcwd( cwd, MAXPATHLEN );
	if( c == NULL )
#pragma		BBA_IGNORE
	    bletch( "getcwd() failed" );
	strcat( cwd, "/" );
	strcat( cwd, name );
	name = cwd;
    }
#endif

    /* Place this shared library on the shared library list 
     * (even if it was not helpful above).
     */
    if (shlindex >= ld_shlsize) {
	ld_shlsize *= 2;
	shlibs = (struct shl_entry *) 
		 realloc(shlibs, ld_shlsize * sizeof (struct shl_entry));
    }
    c = stralloc(strlen(name)+1);
    strcpy(c,name);
    shlibs[shlindex].name = c - stringt;
    shlibs[shlindex].bind = bind;

#ifdef SHLIB_PATH
	if (embed_path != NULL || enable_shlib_path)
	    shlibs[shlindex].load = minus_l ? LOAD_LIB : LOAD_PATH;
	else
#endif
    /* This should be able to deal with the 'minus_l' parameter in the
     * future, but for right now, we will just do these with LOAD_PATH.
     * It is a secutiry hole otherwise.
     */
	    shlibs[shlindex].load = LOAD_PATH;
    shlibs[shlindex].highwater = filhdr.a_highwater;

    shlindex++;

    return( def );
}

/*----------------------------------------------------------------------
 * 
 * Procedure: enter_shlib_symbol()   (Enter shlib symbol into ld symtab)
 *
 * Description:
 *    This is the first routine in a series of routines meant to deal 
 *    with entering a symbol into the ld symbol table from a shared 
 *    library.  These routines are so complex because they attempt to
 *    determine all linkages between libraries and objects.  At present,
 *    on the series 800, they are not doing any of this neat stuff.
 *
 *    First some terminology:
 *
 *       A "Real Define" is the standard definition of a symbol that
 *       one would have expected from the normal workings of ld.  In this
 *       case, ld has found a definition for one of the symbols that
 *       had previously been undefined (from earlier object modules).
 *     
 *       A "Fuzzy Define" is used for symbols which are needed by symbols
 *       from a shared library.  These are entered into the symbol table
 *       but are marked as not there (NLIST_STILL_FUZZY_FLAG) so that
 *       ld will not find them when outputing the symbol table.  
 *
 *    An example.  Assume that ld already has "_qsort" in the symbol 
 *    table as an undefine.  We scan libc.sl, and find _qsort.  But,
 *    also find that _qsort calls _howdy.  If _howdy is also in libc.sl,
 *    we recurse and make it a real define.  If not, we enter _howdy as
 *    a fuzzy define (actually undefine).  If we see a _howdy later in 
 *    either an archive or shared library, we will know to bring it in.
 *
 *    Another example.  Assume that we call "_malloc" in libmalloc.sl.
 *    After this is resolved, we also need _exit out of libc.sl (assume
 *    that libmalloc.sl is on the ld command line before libc.sl).  
 *    Let's say that _exit requires _free.  Which _free should it get,
 *    libc's or libmalloc's?  It had better get libmalloc's, or bad 
 *    things will occur.  So, when _malloc is encountered, _free is
 *    fuzzy defined.  Thus, when libc is scanned, it's free will NOT be
 *    pulled in.
 *        
 *    This is the basis for all of these routines: trying to keep the
 *    references straight between libraries and objects.
 */
enter_shlib_symbol( eeidx, slot )
long eeidx, slot;
{
    register struct export_entry *ee = &ee_table[ eeidx ];
    register struct shl_export_entry *see = &see_table[ eeidx ];
    register struct symbol *sp = &symtab[ slot ];
#ifdef SAVE_SLOT
	int save_slot = slot;
#endif
    long j, i, t;
    struct export_entry *ee2;
    long dmodule_head, dmodule_tail;

    /* ee_done[] is used to mark export list entries which have been 
     * finished completely.  It keeps us from doing work over again.
     * In some cases, it is important that we do NOT do the same export
     * list entry twice.  This array prevents it.
     */
    ee_done[ eeidx ] = 1;

    /* stop here if we don't really need to do anything */
    if ((symtab[slot].s.n_type & FOURBITS) == UNDEF
        && symtab[slot].s.n_value)
    {
	/* procedures don't resolve common */
	if (ee->type & TYPE_PROCEDURE)
	    return;
	/* secondary common is almost as good as a "real" definition */
	else if ((symtab[slot].s.n_type & EXTERN2)
	         && (ee->type & TYPE_EXTERN2))
	    return;
    }

    /* If the symbol was already fuzzy, then the definition should remain
     * fuzzy.  This means that we saw a reference in another shared library
     * and are resolving it in this shared library.  References between
     * shared libraries are no concern of the a.out file, thus it is
     * simply marked as fuzzy, for the next library to look at.
     * Note the reference must have been in a different shared library,
     * or we would have caught this symbol during the "recursive"
     * module list traversal.
     *
     * In the other case, the symbol has yet to be defined at all.  This
     * means that object modules want the symbol, so it becomes a full
     * define.
     */
    if( sp->s.n_flags & NLIST_STILL_FUZZY_FLAG )
	i = fuzzy_define( ee, see, slot );
    else
	i = real_define( ee, see, slot );
#ifdef SAVE_SLOT
	/* above action could conceivably expand symtab */
	sp = symtab + save_slot;
#endif

    /* Either fuzzy_define or real_define may find that they don't need
     * to enter the symbol.  Such cases are common symbol definitions.
     * In those cases, we return before doing any of the module stuff.
     */
    if( i == 0 )
	return;

    /* Now for symbols in the same module as the one defined above (this
     * information comes from the module lists).  This is done in two
     * steps:
     *  
     *  1) If a symbol is already undefined in the symbol table, and we find
     *     one of these definitions for it, we enter the symbol in the normal
     *     fashion (as above).
     *
     *  2) If the symbol is not present, then it will be entered as a fuzzy
     *     define.  If a similar definition is attempted later, it is rejected.
     *     If this symbol is ever referenced, then it should get, then this
     *     fuzzy definition turns into a real definition.  If it is a piece
     *     of bss/data/cdata, it will be copied in at that time.  If it was
     *     a piece of text, then a PLT will have to be created.
     *
     * The reason we make two passes at this is because the first may 
     * do some symbol defining.  For example, assume that _a and _as are
     * two symbols on the same object.  If _a is already undefined, we
     * will catch it in part 1.  But, we will also enter _as during
     * enter_other_shlib_names().  Thus, part 2 won't need to deal with _as.
     */
	for (j = see->next_module; j != ee - ee_table; j = see_table[j].next_module)
    {
		if	(!ee_done[j])
		{
#ifdef	SKIP_OLD_VERSIONS
			if (ExpLookup(string_table+ee_table[j].name,-1,&ee2,string_table,ee_table,hash_table,hash_size) && ee2 - ee_table != j)
				continue;
#endif
			if ((slot = slookup(string_table+ee_table[j].name)) != -1
			    && (symtab[slot].s.n_type & FOURBITS) == UNDEF)
			{
				if (symtab[slot].s.n_flags & NLIST_STILL_FUZZY_FLAG)
					fuzzy_define(&ee_table[j],&see_table[j],slot);
				else
					real_define(&ee_table[j],&see_table[j],slot);
#ifdef SAVE_SLOT
				/* above action could conceivably expand symtab */
				sp = symtab + save_slot;
#endif
			}
			else if (slot != -1 && !(ee_table[j].type & TYPE_EXTERN2) && !(symtab[slot].s.n_type & EXTERN2))
#ifdef	SKIP_OLD_VERSIONS
			{
				int old_type[] = { EXTERN|UNDEF, EXTERN|DATA, EXTERN|TEXT, EXTERN|BSS, EXTERN|BSS, EXTERN|TEXT, EXTERN|ABS, EXTERN|TEXT };

				if (trace_list != NULL)
					check_ylist(string_table+ee_table[j].name,old_type[ee_table[j].type],0);
#endif
				error(e6,symtab[slot].sname,filename);
#ifdef	SKIP_OLD_VERSIONS
			}
#endif
		}
    }

    for( j=see->next_module; j != (ee-ee_table); j=see_table[j].next_module )
    {
	if( !ee_done[ j ] &&
	    (slot = slookup( string_table + ee_table[j].name )) == -1 )
	{
#ifdef	SKIP_OLD_VERSIONS
		if (ExpLookup(string_table+ee_table[j].name,-1,&ee2,string_table,ee_table,hash_table,hash_size) && ee2 - ee_table != j)
			continue;
#endif
	    slot = sp->sindex;       /* sp is not preserved over an enter() */
	    enter( -1 );
	    sp = &symtab[slot];
	    slot = slookup( string_table + ee_table[j].name );

	    fuzzy_define( &ee_table[j], &see_table[j], slot );
#ifdef SAVE_SLOT
		/* above action could conceivably expand symtab */
		sp = symtab + save_slot;
#endif
	}
    }

    /* This rather large piece of bulky code handles symbols which are
     * NEEDED by the module that this symbol was defined in.  Each module
     * is done in turn, using a linked list of modules.  A new module is
     * added to the list if a symbol that is needed is found in another
     * module of the shared lib. 
     */
    if( see->dmodule != -1 && 
	dmodule_table[ see->dmodule ].module_imports != -1 )
    {
	/* Recursion is handled through a linked list of dmodule_next
	 * entries.  DMODULE_END is used to mark the end of the list,
	 * and new entries are added to the end of the list, by using
	 * 'dmodule_tail'.
	 */
	dmodule_next[ see->dmodule ] = DMODULE_END;
	for( dmodule_head = dmodule_tail = see->dmodule; 
	     dmodule_head != DMODULE_END; 
	     dmodule_head = dmodule_next[ dmodule_head ] )
	{
	    /* This loop handles this particular module.  Each iteration
	     * goes through a different import list entry which is 
	     * needed.
	     */
#ifdef	RECURSE_FIX
	    if ((j = dmodule_table[dmodule_head].module_imports) == -1)
		continue;
#else
	    j = dmodule_table[ dmodule_head ].module_imports;
#endif
	    do
	    {
		i = mod_table[ j ].import & MODULE_IMPORT_MASK;
#ifdef	VISIBILITY
		/* if this is an "intra" (anonymous) import,
		   just put the target module on the list and continue */
		if (ie_table[i].type == TYPE_INTRA)
		{
#ifndef	DONT_TRACE
			t = ie_table[i].shl;
#ifdef	RECURSE_FIX
			if (dmodule_next[t] == DMODULE_UNDEF)
			{
#endif	/* RECURSE_FIX */
				dmodule_next[t] = DMODULE_END;
				dmodule_next[dmodule_tail] = t;
				dmodule_tail = t;
#ifdef	RECURSE_FIX
			}
#endif	/* RECURSE_FIX */
#endif	/* DONT_TRACE */
			continue;
		}
#endif	/* VISIBILITY */

		/* If the symbol is alredy defined in the symtab, we don't
		 * need to do anything else (except mark if for exporting,
		 * which is done below).  In the other case, a fuzzy 
		 * define must be made for it (may be a fuzzy undefine). 
		 */
		if( (slot = slookup(string_table+ie_table[i].name)) == -1 )
		{
		    enter( -1 );
#ifdef SAVE_SLOT
			/* above action could conceivably expand symtab */
			sp = symtab + save_slot;
#endif
		    slot = slookup( string_table+ie_table[i].name );

#ifndef	EVIL_FIX
		    /* The check against ee_done is probably superfluous
		       since the symbol isn't even in the symtab yet */
		    if( ExpLookup( symtab[ slot ].sname, -1, &ee2, 
				   string_table, ee_table,
				   hash_table, hash_size ) &&
			!ee_done[ ee2 - ee_table ] )
		    {
			/* The symbol that we need is in the shlib that
			 * we are currently on.  Thus, we do a fuzzy define,
			 * and add the new module to the list of modules
			 * that we need to consider.
			 */
			fuzzy_define(ee2, &see_table[ee2-ee_table], slot);
#ifdef SAVE_SLOT
			/* above action could conceivably expand symtab */
			sp = symtab + save_slot;
#endif
			ee_done[ ee2 - ee_table ] = 1;
			if( (t = see_table[ee2-ee_table].dmodule) != -1 &&
			    dmodule_next[t] == DMODULE_UNDEF && 
			    dmodule_table[t].module_imports != -1 )
			{
			    t = see_table[ee2-ee_table].dmodule;
			    dmodule_next[t] = DMODULE_END;
			    dmodule_next[ dmodule_tail ] = t;
			    dmodule_tail = t;
			}
		    }
		    /* If there ever was a case where the above check against
		       ee_done was relevant, this may be broken, since this
		       clause may be hit if the symbol was already in the lib
		       and already had meaningful flags and shlib fields */
		    else
#endif
		    {
			/* The symbol was not in the shlib, thus we make
			 * it a fuzzy undefine.  This is fuzzy because if 
			 * it is defined within another shared library,
			 * we don't want to bother with it.  Shlib_tables()
			 * will check for fuzzy undefines, which become
			 * errors at that point.
			 */
			symtab[ slot ].s.n_flags |= 
			    NLIST_EVER_FUZZY_FLAG|NLIST_STILL_FUZZY_FLAG;
			symtab[ slot ].shlib = -1;
		    }
		}

#ifdef	MISC_OPTS
		if (trace_list != NULL)
			check_ylist(symtab[slot].sname,EXTERN|UNDEF,0);
#endif

		/* In any case, the shlib is going to need this symbol,
		 * thus we MUST export it.
		 */
		symtab[ slot ].expindex = EXP_UNDEF;
	    }
	    while( !(mod_table[ j++ ].import & MODULE_END_FLAG) );
	}

	/* Mark this dmodule as finished (so we don't do loops on
	 * ourself (wouldn't kill us, but it is inefficient)).
	 */
	dmodule_table[ see->dmodule ].module_imports = -1;
    }
}

/*----------------------------------------------------------------------
 *
 * Procedure: real_define()              (define a shlib symbol, really)
 *
 * Description:
 *    If we made it here, we know that we have a symbol that we really 
 *    want to be in the a.out file.  Since we are playing with shlibs,
 *    this is a little different than the archive case:
 *  
 *        Text: Text symbols don't need to be copied (which is the whole
 *              point for shlibs).  Instead a PLT entry is created by
 *              shlib_tables(), we just mark it here for this purpose.
 * 
 *        BSS:  These are resolved directlly, by allocating them to BSS
 *              at this time.  Alignment issues are considered.
 *
 *        Data/CData: These actually need to be copied from the shlib.
 *              At this point, the size, position within the file, and
 *              type are recorded.  Again, pass2() takes care of the
 *              work for us.
 *
 *        Common: If we find a common with a bigger value than the
 *              one we have, then we keep the largest.  As far as 
 *              commons between shared libraries, dld.sl insures that
 *              the largest is found.  However, if the a.out also has
 *              a common definition, the largest of all the libraries
 *              and the .o's must be in the a.out (and exported).
 *
 *    As simple as that all sounds, there are a few issues to consider.
 *    First, alignment of data/cdata/bss items must be insured (text is
 *    not a problem, since those are always aligned on a two byte 
 *    boundary).  Thus, alignment information is carried around as well.
 *
 *    After a symbol is defined, then all the other names that it was
 *    under inside the shlib are also entered into the symbol table.
 *    This is handled by enter_other_shlib_names(). 
 */
real_define( ee, see, slot )
struct export_entry *ee;
struct shl_export_entry *see;
long slot;
{
#ifdef SAVE_SLOT
	int save_slot = slot;
#endif
    int j;
    unsigned long flags = 0;
    register struct symbol *sp = &symtab[slot];

    /* We know that the symbol is undefined, that we have now 
     * found it.  We will first find out it's type and some
     * other useful things.
     */
    switch( ee->type & TYPE_MASK )
    {
      case TYPE_UNDEFINED:   
#pragma BBA_IGNORE
	fatal( e53, filename );
	break;

      case TYPE_CDATA:
#pragma BBA_IGNORE
      case TYPE_DATA:        
	/* We will need to copy the data for this data item to
	 * the incomplete executable, so place in the list.
	 * '+1' is for the terminator which is needed (see below).
	 */
	if( (shldata_idx+1) >= shldata_size )
	{
	    shldata_size *= 2;
	    shldata = (struct shl_data_copy *)
		realloc( shldata, shldata_size *
			 sizeof( struct shl_data_copy ) );
	}

	/* Copy type, offset within file, alignment fixup, and size
	 * into the shldata array.  This will get read back out during
	 * pass2().
	 */
	shldata[ shldata_idx ].type = ee->type & TYPE_MASK;
	shldata[ shldata_idx ].offset = foffset( ee->value );
	j = (ee->value & 3) - (dsize & 3);
	if( j < 0 )
#pragma		BBA_IGNORE
		j += 4;
	shldata[ shldata_idx ].align = j;
	shldata[ shldata_idx ].size = ee->size;

	/* Patch the value for the newly defined entry within symtab.
	 */
	sp->s.n_type = (((ee->type & TYPE_MASK) == TYPE_DATA ?
			 DATA : TEXT) | EXTERN);  
	sp->s.n_value = dsize + j;
	dsize += j + ee->size;

	/* Make the dynamic relocation entry for this item.  Since this item
	 * is being copied from the shlib, we need a PROPAGATE relocation
	 * record in the a.out file (only if there are dynamic relocations
	 * in the shlib).
	 */
#ifdef	ELABORATOR
	if (see->dreloc != -1 || (see->dmodule != -1 && (dmodule_table[see->dmodule].flags & DM_INVOKES)))
#else
	if( see->dreloc != -1 )
#endif
	{
	    relindex++;  
	    shldata[ shldata_idx ].sym = slot;
	    sp->s.n_flags |= NLIST_DRELOC_FLAG;
	}
	else
	    shldata[ shldata_idx ].sym = -1;

	shldata_idx++;
#ifdef EXPORT_MORE
	sp->expindex = EXP_UNDEF;
#endif
	break;

      case TYPE_PROCEDURE:   
	/* If we see a definition for a procedure in the shared
	 * library, after it has already been declared as a common
	 * block, ignore the definition in the shlib (old archive 
	 * behavior mimicked here).
	 */
#ifdef	FOLLOW_VERSION
	if ((sp->s.n_type & FOURBITS) == UNDEF)
	{
#endif
		if( sp->s.n_value > 0 )
		    return( 0 );                        /**************/
#ifdef	FOLLOW_VERSION
	}
	/* otherwise, I guess we've already done the work; let's get out of here */
	else
		break;
#endif

	/* We need to make a PLT for this reference 
	 */
	sp->pltindex = PltEnter( sp, shlindex );
	sp->s.n_value = sp->pltindex * sizeof( struct plt_entry );

	/* Patch the type within the symbol table.  The NLIST_EXPORT_PLT_FLAG
	 * tells shlib_tables() to export the PLT entry.
	 */
	flags = NLIST_EXPORT_PLT_FLAG;
	sp->s.n_flags |= flags;
	sp->s.n_type = (TEXT | EXTERN);

	break;

      case TYPE_COMMON:
	/* Collect the biggest possible defintion for the
	 * item.  If it grew because of the library, up the size.
	 * This mainly models the archive case.
	 */
	if( sp->s.n_value < ee->size )
	    sp->s.n_value = ee->size;

	/* We will need to export this guy so that the shlib can find it
	 * (we only want one common of the same name to be used)
	 */
	sp->expindex = EXP_UNDEF;

	/* Inform our caller that this was common and that we need
	 * to do no more with it (common's do not have any kind of
	 * a module affiliation, they could be in many .o's within the
	 * shlib).
	 */
	return( 0 );                 /****************/
	break;

      case TYPE_BSS:         
	/* Patch the type, and add to the bsize.  Notice that we fix
	 * the alignment stuff here as well.
	 */
	sp->s.n_type = (BSS | EXTERN);
	j = (ee->value & 3) - (bsize & 3);
	if( j < 0 )
#pragma		BBA_IGNORE
	    j += 4;
	sp->s.n_value = bsize + j;
	bsize += j + ee->size;

	break;

      case TYPE_ABSOLUTE:    
	/* Absolutes are just taken on faith.  Nothing more to do later.
	 */
	sp->s.n_type = (ABS | EXTERN);
	sp->s.n_value = ee->value;
	break;

      default:
#pragma  BBA_IGNORE
	fatal( e53, filename );
	break;
    }

#ifdef	MISC_OPTS
	if (trace_list != NULL)
		check_ylist(sp->sname,sp->s.n_type,sp->s.n_value);
#endif

    /* Record the shared library number in the ld symbol table.
     */
    sp->shlib = shlindex;
    sp->size = ee->size;

    /* Now, go through all symbols of this type, and enter them
     * both in the export table, and in the normal symbol table 
     */
    enter_other_shlib_names( ee, see, sp, flags, flags, FALSE );
#ifdef SAVE_SLOT
	sp = symtab + save_slot;
#endif

    /* Now, we can or in the EXTERN2 flag into sp.  We couldn't do
     * this before the enter_other_shlib_names(), since every symbol
     * defined there would have been EXTERN2.
     */
    sp->s.n_type |= ((ee->type & TYPE_EXTERN2) ? EXTERN2 : 0);

    /* Everything cool, continue to work on this shlib module.
     */
    return( 1 );
}

/*----------------------------------------------------------------------
 *
 * Procedure: fuzzy_define()              (define a shlib symbol, sorta)
 *
 * Description:
 *    This procedure is very much like real_define(), but in a fuzzy
 *    kind of a way.  Different strutures are used, and different
 *    flags, but all the same info is kept around.  The big difference
 *    is that fuzzy_define() will not add anything to the a.out file
 *    (although this may happen later; see write_fuzzy_data()).
 *
 *    What do we mean by a fuzzy define?  A symbol is entered fuzzy when
 *    it appears in the same module within the shlib as a symbol which 
 *    is given a full define.  These symbols are technically part of 
 *    the a.out that is being produced, but only as place holders.  When
 *    the a.out file is written out, they will not be included in the 
 *    symbol table.  
 *
 *    An example: a .o file references _malloc and _exit.  libmalloc.sl
 *    is scanned and _malloc is found, _free is fuzzy defined. 
 *    Next, libc.sl is scanned, and _exit is found, which needs _free.
 *    Since _free is already fuzzy defined, it will not try and define
 *    the _free which is in libc.sl (which is what we want).
 *
 *    Another example: libmalloc.sl references _printf.  We enter it as
 *    a fuzzy UNDEF (these are legal too).  When libc.sl is scanned, 
 *    _printf will satisfy this UNDEF, but nothing else will happen. 
 *    No PLT will be created for _printf, and none should be (dld.sl
 *    will take care of this for us).  Now, let's say that a .o file
 *    contains _printf.  We then want it to be brought in, and in this
 *    case, the fuzzy UNDEF turns into a regular ld define (no fuzziness).
 *
 *    One more example:  libmalloc.sl defines _value as a data item
 *    in the same module as _malloc.  We enter _value as a fuzzy define
 *    and record file location info, etc.  Later, we see a .o file which
 *    needs _value.  At that time, we make _value a real define, and
 *    copy the data into the a.out.  If we had never seen the reference
 *    by that last .o file, we would have just left _value in the shlib.
 *
 *    Get the idea?  There are many, many cases where fuzzy defines are
 *    needed; listed above are just a few.  
 */
fuzzy_define( ee, see, slot )
struct export_entry *ee;
struct shl_export_entry *see;
long slot;
{
    unsigned int i;
    unsigned long flags = 0;

    ee_done[ ee-ee_table ] = 1;

    /* Find the typr that we are to take care of, and do some crude
     * work on the symbol.
     */
    switch( ee->type & TYPE_MASK )
    {
      case TYPE_DATA:        i = DATA;     break;

      case TYPE_PROCEDURE:   
	i = TEXT;
	flags |= NLIST_EXPORT_PLT_FLAG;
	break;

      case TYPE_BSS:         i = BSS;      break;

      case TYPE_ABSOLUTE:    i = ABS;      break;

      case TYPE_COMMON:      
	if( symtab[ slot ].s.n_value < ee->size )
	    symtab[ slot ].s.n_value = ee->size;
	i = UNDEF;
	break;

      default:
#pragma BBA_IGNORE
		fatal( e53, filename );  break;
    }
    symtab[ slot ].s.n_type = (i | EXTERN);

#ifdef	MISC_OPTS
	if (trace_list != NULL)
	{
		symp sp = symtab + slot;

		check_ylist(sp->sname,sp->s.n_type,sp->s.n_value);
	}
#endif
    /* These two flags make this symbol fuzzy.  Later, if we need the
     * symbol, we can clear NLIST_STILL_FUZZY_FLAG, buut we should never
     * touch NLIST_EVER_FUZZY_FLAG (except when it is OK to do so!)
     */
    symtab[ slot ].s.n_flags |= NLIST_STILL_FUZZY_FLAG|NLIST_EVER_FUZZY_FLAG;

    /* Record where our information structure is, and size of this item.
     */
    /* yet another use of shlib - this time to index the fuzzy data array */
    symtab[ slot ].shlib = fuzzy_shl_idx;
    symtab[ slot ].size = ee->size;

    /* Enter other shlib_names for this symbol.  Also, these will be
     * fuzzy for the time being.  Shlib_tables() will change all of them
     * at the same time.
     */
    /* Note that flags1 (the case where the other symbol was already an undef
     * in the symbol table) does not have STILL_FUZZY turned on, since we
     * do not wish to make it fuzzy if it isn't.  But it does have EVER_FUZZY,
     * since we want to be sure that later, in shlib_tables, these get
     * allocated together.  See, if we didn't make it EVER_FUZZY, then
     * it would be a "real" define, and we would have immediately note it
     * needs to be copied; later, we wouldn't know we had already arranged for
     * the alias to be copied, and we'd arrange for the original symbol to be 
     * copied, too.
     */
    i = enter_other_shlib_names( ee, see, &symtab[ slot ], 
			 NLIST_EVER_FUZZY_FLAG|flags,
			 NLIST_STILL_FUZZY_FLAG|NLIST_EVER_FUZZY_FLAG|flags,
			 TRUE );

    /* enter_other_shlib_names() will return to us a circular linked list
     * of names which were aliased to this symbol.  We record that list
     * in the size portion of the symtab entry.  We will recover it in
     * shlib_tables().
     */
    if( i != -1 )
	symtab[ slot ].alias = i;
    else
	symtab[ slot ].alias = slot;

    /* As with real_define, we enter the EXTERN2 bit after entering
     * all other shlib names.
     */
    if( ee->type & TYPE_EXTERN2 )
	symtab[ slot ].s.n_type |= EXTERN2;

    /* Now, fill in the information structure, which will be needed by
     * write_fuzzy_data().  We will need offset, an alignment info
     * (notice that for now, we simply record the alignment in the shlib),
     * and shared library number (corresponding the shlib structures).
     */
    if( fuzzy_shl_idx >= fuzzy_shl_size )
    {
	fuzzy_shl_size *= 2;
	fuzzy_shl = (struct fuzzy_shl_copy *) 
		    realloc( fuzzy_shl,
			     fuzzy_shl_size * sizeof(struct fuzzy_shl_copy) );
    }           
    fuzzy_shl[ fuzzy_shl_idx ].size = ee->size;
    if( (ee->type & TYPE_MASK) == TYPE_ABSOLUTE )
	fuzzy_shl[ fuzzy_shl_idx ].offset = ee->value;
    else
	fuzzy_shl[ fuzzy_shl_idx ].offset = foffset( ee->value );
    fuzzy_shl[ fuzzy_shl_idx ].align = ee->value & 3;
    fuzzy_shl[ fuzzy_shl_idx ].shlib = shlindex;
    fuzzy_shl[ fuzzy_shl_idx ].sym = -1;

    /* It may be necessary for us to have a propogate reloc record for this
     * item if it turns into a real define.  In that case, we will
     * need to know this little tid bit of information.
     */
#ifdef	ELABORATOR
	if (see->dreloc != -1 || (see->dmodule != -1 && (dmodule_table[see->dmodule].flags & DM_INVOKES)))
#else
    if( see->dreloc != -1 )
#endif
	fuzzy_shl[ fuzzy_shl_idx ].flags |= FUZZY_PROPOGATE_DRELOC;

    fuzzy_shl_idx++;

    return( 1 );
}

/*----------------------------------------------------------------------
 *
 * Procedure: enter_other_shlib_names()           (enter secondary defs)
 *
 * Description:
 *    Shared libraries have the ability to have multiple labels on the
 *    same piece of data or on the same routine.  One special case of 
 *    this are secondary defs.  This routine will take an export
 *    list entry, and enter all the aliases for it into the normal
 *    ld symbol table.
 *
 *    In the case of fuzzy defines, we need to run a linked list through
 *    all of these entries.  If the 'linked' parameter is true, it 
 *    will produce this type of a linked list, with the .size field 
 *    of the symbol table entries as the pointers.  It will return the
 *    tail of the linked list to the caller.
 */
enter_other_shlib_names( ee, see, sp, flags1, flags2, linked )
struct export_entry *ee;
struct shl_export_entry *see;
struct symbol *sp;
unsigned flags1, flags2;
int linked;
{
    long j, slot;
    long last = -1, first;

    /* First we check for the existance of any aliases.
     */
    if( see->next_symbol != -1 )
    {
	/* For each symbol which is an alias...
	 */
	for( j = see->next_symbol; j != (ee-ee_table);
					     j = see_table[j].next_symbol )
	{
	    /* If we have already dealt with this symbol, ignore this
	     * definition, otherwise mark it as being finished.
	     */
	    if( ee_done[ j ] )
		continue;
	    else
		ee_done[ j ] = 1;

#ifdef	MISC_OPTS
		if (trace_list != NULL)
			check_ylist(string_table+ee_table[j].name,sp->s.n_type,sp->s.n_value);
#endif
	    /* Next we see if the symbol is already in the symbol table.
	     * If it is, we will have to do some checks to see if it is
	     * a definition, or a secondary def, or other combinations.
	     */
	    if( (slot = slookup( string_table+ee_table[j].name )) != -1 )
	    {
		/* If there already is a primary definition, and this
		 * is a primary def, then we have a clash.
		 */
		if ((symtab[slot].s.n_type & FOURBITS) != UNDEF 
		    && !(ee_table[j].type & TYPE_EXTERN2) 
		    && !(symtab[slot].s.n_type & EXTERN2))
		{
			error(e6,string_table+ee_table[j].name,filename);
		}
		/* if symbol in table is common */
		else if ((symtab[slot].s.n_type & FOURBITS) == UNDEF
		         && symtab[slot].s.n_value)
		{
			/* don't let text resolve */
			if (ee_table[j].type & TYPE_PROCEDURE)
				continue;
			/* don't let secondary data resolve secondary common */
			else if ((symtab[slot].s.n_type & EXTERN2)
			         && (ee_table[j].type & TYPE_EXTERN2))
				continue;
			/* otherwise accept the new definition */
			symtab[slot].s.n_type = sp->s.n_type | ((ee_table[j].type & TYPE_EXTERN2) ? EXTERN2 : 0);
			symtab[slot].s.n_value = sp->s.n_value;
		}
		/* If the definition in the symbol table is an UNDEF, or
		 * is a secondary def (and the library version is not), 
		 * then we can enter the primary definition for it.
		 */
		else if( (symtab[slot].s.n_type & FOURBITS) == UNDEF
			 || (!(ee_table[j].type & TYPE_EXTERN2) 
			     && symtab[slot].s.n_type & EXTERN2) )
		{
		    symtab[ slot ].s.n_type = sp->s.n_type |
			((ee_table[j].type & TYPE_EXTERN2) ? 
			 EXTERN2 : 0);
		    symtab[ slot ].s.n_value = sp->s.n_value;
		}
		else
		{
		    /* No action needed, since none of the conditions
		     * above were taken.  We will ignore this definition.
		     */
		    continue;
		}

		/* Or in the 'flags1' variable at this point.  'flags2'
		 * is ored in below (both won't be ored into the same
		 * symbol).  Both are taken from the caller directly.
		 */
		symtab[ slot ].s.n_flags |= flags1;
#ifdef	FUZZY2FIX
		/* if this is an alias for a "real" define, make alias "real" too */
		/* it will still only be exported if needed */
		if (!linked)
			symtab[slot].s.n_flags &= ~(NLIST_EVER_FUZZY_FLAG|NLIST_STILL_FUZZY_FLAG);
#endif
	    }           
	    else
	    {
		/* Since it is not in the symtab, go ahead and enter it,
		 * with all the information from the library.
		 */
		slot = sp->sindex;   /* Saved, since enter could move symtab */
		enter( slookup( string_table+ee_table[j].name ));
		sp = &symtab[ slot ];

		ldrsym(slot = slookup( string_table + ee_table[j].name ), 
		       sp->s.n_value,
		       sp->s.n_type | 
		       ((ee_table[j].type & TYPE_EXTERN2) ? 
			EXTERN2 : 0));
		symtab[ slot ].s.n_flags |= flags2; 

		/* This alias is marked as not to be exported.  The reason
		 * this is not in the earlier case for this if() is that
		 * in the case above, this is not needed, since it was
		 * already entered into the symbol table and EXP_DONT_EXPORT
		 * was already set (or it may not be set anymore, since 
		 * a shlib may have asked for it already).  A shlib
		 * (including the current one) may ask for this symbol later,
		 * and this flag will be reset.
		 */
		symtab[ slot ].expindex = EXP_DONT_EXPORT;
	    }

	    /* Record some information about this symbol in the symtab.
	     */
	    symtab[slot].shlib = sp->shlib;
	    symtab[slot].pltindex = sp->pltindex;

	    /* Build our linked list of symbols, if it was requested.
	     */
	    if( linked )
	    {
		if( last == -1 )
		    symtab[ slot ].alias = sp->sindex;
		else
		    symtab[ slot ].alias = last;
		last = slot;
	    }
	    symtab[ slot ].size = sp->size;
	}
    }

    /* Return the tail of the linked list to the caller 
     */
    return( last );
}

/*----------------------------------------------------------------------
 * 
 * Procedure: foffset()                (Return file offset for pointer)
 *
 * Description:
 *    This procedure converts a pointer to a file offset.  For example,
 *    you know that a certain item is at address 0x0f0f0f when the
 *    file is exec()ed, and you want to know what offset within the
 *    file (while it sits on the disk) that address corresponds to.
 *    It uses the current filhdr structure to get the magic number.
 */
long foffset( off )
long off;
{
    switch( filhdr.a_magic.file_type )
    {
      case SHL_MAGIC:
      case DL_MAGIC:
	    off = (off - filhdr.a_entry) + TEXTPOS;
	break;
#if 0     
    /* These are here only for completeness.  This code was taken
     * from diss which can handle these extra kinds as well.  We
     * never use them.  If called for these, the default should 
     * catch them and give an error.
     */
      case SHARE_MAGIC:
	off = (off - EXEC_ALIGN( filhdr.a_text )) + filhdr.a_text + TEXTPOS;
	break;
      case DEMAND_MAGIC:
	off += TEXTPOS;
	break;
      case RELOC_MAGIC:
      case EXEC_MAGIC:
	bletch( "reloc and exec not supported for dl information\n" );
	break;
#endif

      default:
#pragma BBA_IGNORE
	error( e53, filename );
	break;
    }

    return( off );
}

/*----------------------------------------------------------------------
 *
 * Procedure: FillInShlibStuff()              (Fill in shlib structures)
 *
 * Description:
 *    This routine fills in the extension header for the a.out file
 *    and the DYNAMIC structure.  Most of the values are filled in 
 *    directly from symbol table values.  There are many terinary 
 *    operators, for the various configurations.
 */
FillInShlibStuff()
{
#ifdef	HOOKS
    int slot;
#endif

    /* General extension header fields
     */
    extheader.e_header = DL_HEADER;
    extheader.e_version = 1;
    extheader.e_size = shlibtextsize;
    extheader.e_extension = debugoffset;

    /* General dl_header fields
     */
    extheader.e_spec.dl_header.zero = 0;
    extheader.e_spec.dl_header.dynamic = 
	symtab[slookup("__DYNAMIC")].s.n_value - (savernum>=0 ? savernum : 0);
    extheader.e_spec.dl_header.spare1[0] = 0;
    extheader.e_spec.dl_header.spare1[1] = 0;

    /* Shlib specific extension header fields.  Notice that for a 
     * complete executable (which should never get here), and for DLD, all
     * of these values are sipmly the end of the extension header (that's
     * the else part of the following if()). 
     */
    if( shlib_level > SHLIB_DLD )
    {
	extheader.e_spec.dl_header.shlt = 
	    sizeof(struct header_extension); 
	extheader.e_spec.dl_header.import = 
	    extheader.e_spec.dl_header.shlt +
		( shlib_level == SHLIB_A_OUT ?
		  (shlindex * sizeof(struct shl_entry)) : 0 );
	extheader.e_spec.dl_header.export = 
	    extheader.e_spec.dl_header.import +
		((shlib_level == SHLIB_BUILD ? dltindex : 0)
		 + pltindex + rimpindex) * sizeof(struct import_entry);
	extheader.e_spec.dl_header.shl_export = 
	    extheader.e_spec.dl_header.export + 
		expindex * sizeof( struct export_entry );
	extheader.e_spec.dl_header.hash = 
	    extheader.e_spec.dl_header.shl_export + 
		(shlib_level == SHLIB_BUILD ? 
		 (expindex * sizeof(struct shl_export_entry)) : 0);
	extheader.e_spec.dl_header.string = 
	    extheader.e_spec.dl_header.hash + 
		ld_exphashsize * sizeof(struct hash_entry);
	extheader.e_spec.dl_header.module =
	    extheader.e_spec.dl_header.string + 
		stringindex * sizeof(char);
	extheader.e_spec.dl_header.dmodule =
	    extheader.e_spec.dl_header.module +
		(shlib_level == SHLIB_BUILD ?
		 (mod_index * sizeof(struct module_entry)) : 0);
	extheader.e_spec.dl_header.dreloc = 
	    extheader.e_spec.dl_header.dmodule + 
		(shlib_level == SHLIB_BUILD ?
		 ((mod_number+1) * sizeof(struct dmodule_entry)) : 0);
    }
    else
    {
	extheader.e_spec.dl_header.shlt = 
	    extheader.e_spec.dl_header.import = 
		extheader.e_spec.dl_header.export = 
		    extheader.e_spec.dl_header.shl_export =
			extheader.e_spec.dl_header.hash = 
			    extheader.e_spec.dl_header.string =
				extheader.e_spec.dl_header.module = 
				  extheader.e_spec.dl_header.dmodule =
				    extheader.e_spec.dl_header.dreloc =
					sizeof(struct header_extension);
    }

    /* Dynamic header fields.  Most of these are taken from the values
     * of symbols, others are based on the shlib object being built
     * (for complete executables, DLT's are in the text section, etc.).
     */
    dynheader.next_shl = 0;
    dynheader.ptext = 0;
    dynheader.text = torigin;
    dynheader.data = dorigin;
    dynheader.bss = corigin;
    dynheader.end = corigin + bsize;

    dynheader.bound = (unsigned char *) symtab[ slookup( "BOUND" ) ].s.n_value;
    dynheader.dmodule = (char *)symtab[slookup("DBOUND")].s.n_value;

    dynheader.plt = (struct plt_entry *) symtab[ slookup( "PLT" ) ].s.n_value;
    if( (shlib_level == SHLIB_BUILD && savernum < 0) 
	|| shlib_level == SHLIB_DLD )
    {
	/* In this one, we need the beginning of the DLT's, which is 
	 * based on where we moved the middle_dlt to.
	 */
	dynheader.dlt = (struct dlt_entry *)                           
	    (symtab[ slookup( "DLT" ) ].s.n_value -
	     (middle_dlt * sizeof( struct dlt_entry )));

	if( shlib_level == SHLIB_DLD )
	{
	    dynheader.a_dlt = dynheader.dlt;
	}
	else
	{
	    dynheader.a_dlt = (struct dlt_entry *) ((long)dynheader.dlt +
			      (sizeof(struct dlt_entry) * global_dlt));
	}
    }
    else
    {
	dynheader.dlt = dynheader.a_dlt = (struct dlt_entry *) dynheader.plt;
    }

    dynheader.bor.opcode = BRA;
    dynheader.bor.size = LONG;
    dynheader.bor.spare1 = 0;
    dynheader.bor.displacement = 0;
    dynheader.dl_header = 0;
    dynheader.status = 0;
    dynheader.spare1 = 0;
#ifdef	HOOKS
	if ((slot = slookup(hook_name)) != -1)
		dynheader.dld_hook = symtab[slot].s.n_value;
	else
		dynheader.dld_hook = 0;
#endif
#ifdef	ELABORATOR
	if (shlib_level == SHLIB_BUILD)
	{
		int i;

		if (elaborator_index != -1)
		{
			dynheader.elaborator = global_dlt + pltindex + symtab[elaborator_index].rimpindex;
			dynheader.status |= INITIALIZE;
		}
		else
		dynheader.elaborator = -1;
		if ((i = slookup(initializer_name)) != -1)
		{
			dynheader.initializer = global_dlt + pltindex + symtab[i].rimpindex;
			dynheader.status |= INITIALIZE;
		}
		else
			dynheader.initializer = -1;
	}
#endif
#ifdef SHLIB_PATH
	if (shlib_level == SHLIB_A_OUT)
	{
		if (enable_shlib_path < 0)
			dynheader.status |= DYNAMIC_LAST;
		if (enable_shlib_path > 0)
			dynheader.status |= DYNAMIC_FIRST;
		dynheader.shlib_path = (char *)symtab[slookup("___dld_shlib_path")].s.n_value;
		dynheader.embed_path = symtab[slookup("___dld_embed_path")].s.n_value;
	}
	else if (shlib_level == SHLIB_BUILD)
		dynheader.shlib_path = (char *)symtab[slookup("___dld_shlib_path")].s.n_value;
#endif /* SHLIB_PATH */
}

/*----------------------------------------------------------------------
 *
 * Procedure: WriteShlibText()          (Write Text Section, Shlib Info)
 *
 * Description:
 *    This procedure writes out the text section shlib information: the
 *    extension header and its related sections.  WriteShlibData() handles
 *    the data section parts.  As with FillInShlibStuff(), most of these
 *    branches are because of the various configurations.
 *
 *    Note that WriteDynReloc() will rewrite the dynamic relocation 
 *    information (later).
 */
WriteShlibText()
{
    if( shlib_level >= SHLIB_DLD )
    {
	fwrite(&extheader, sizeof(struct header_extension), 1, tout);
	if( shlib_level > SHLIB_DLD )
	{
	    if( shlib_level == SHLIB_A_OUT )
		fwrite(shlibs, sizeof(struct shl_entry), shlindex, tout);
	    if( shlib_level == SHLIB_BUILD )
		fwrite(dltimports, sizeof(struct import_entry), dltindex,tout);
	    fwrite(pltimports, sizeof(struct import_entry), pltindex, tout);
	    fwrite(reloc_imports,sizeof(struct import_entry),rimpindex,tout);
	    fwrite(exports, sizeof(struct export_entry), expindex, tout);
	    if( shlib_level == SHLIB_BUILD )
	    {
		exp_seek = ftell( tout );
		fwrite( shl_exports, sizeof(struct shl_export_entry), 
			expindex, tout );
	    }
	    fwrite(exphash, sizeof(struct hash_entry), ld_exphashsize, tout);
	    fwrite(stringt, sizeof(char), stringindex, tout);
	    if( shlib_level == SHLIB_BUILD && mod_index )
	    {
		fix_module_lists();

		fwrite( module_table, sizeof( struct module_entry ), 
			mod_index, tout );
	    }
	    if (shlib_level == SHLIB_BUILD)
	    {
		struct dmodule_entry dm;

		fwrite(module,sizeof(struct dmodule_entry),mod_number,tout);
		dm.flags = DM_END;
		dm.dreloc = relindex - 1;
		dm.a_dlt = local_dlt;
		dm.module_imports = 0;      /* Dummy Value */
		fwrite(&dm,sizeof(struct dmodule_entry),1,tout);
	    }
	}
	dyn_seek = ftell( tout );
	fwrite( relocs, sizeof(struct relocation_entry), relindex, tout );
    }

    torigin += shlibtextsize;
#ifdef N_INCOMPLETE_FIX
	if (shlibtext_pad)
		zout(tout,shlibtext_pad);
#endif
}

/*----------------------------------------------------------------------
 *
 * Procedure: fix_module_lists()                   (fixup module tables)
 *
 * Description:
 *    The module tables, as produced by shlib_tables() are very crude,
 *    and are not null terminated.  Also, the final number of dlt's is
 *    not known until after shlib_tables() is finished, we must add
 *    the dltindex into some of these entries (same reason we need to
 *    add the pltindex into some of these items).  Obviously, we only
 *    need to do this if we are building a shared library.
 */
fix_module_lists()
{
    register int t, n;
    int last, first, seen, old;
#ifdef	ELABORATOR
    int set_invokes = 0;
    int elaborator_import = -1;
#endif

#ifdef	ELABORATOR
    if (elaborator_index != -1)
	elaborator_import = global_dlt + pltindex + symtab[elaborator_index].rimpindex;
#endif
    /* We need to HELP */
    for( n = 0; n < mod_number && module[ n ].module_imports < 0; n++ ) ;
    last = n;
    for( ; n < mod_number && module[ n ].module_imports <= 0; n++ ) ;

    for( seen = 0, first = -1, t = 0; t < mod_index; t++ )
    {
	assert( module_table[ t ].import >= 0 );

	if( module_table[ t ].import & MODULE_ADD_DLTINDEX )
	{
	    module_table[ t ].import = global_dlt +
		(module_table[t].import & MODULE_IMPORT_MASK);
	    if( first == -1 ) 
		first = t;
	}
	else if( module_table[ t ].import & MODULE_ADD_BOTH )
	{
	    module_table[ t ].import = global_dlt + pltindex + 
		(module_table[t].import & MODULE_IMPORT_MASK);
	    if( first == -1 )
		first = t;
	}
	else
	{
	    seen = 1;
	    if( first == -1 )
		module_table[ t ].import |= MODULE_DLT_FLAG;
	    else
	    {
		old = module_table[ first ].import;
		module_table[ first ].import = 
		    module_table[ t ].import | MODULE_DLT_FLAG;
		module_table[ t ].import = old;
		while( (module_table[++first].import & MODULE_DLT_FLAG)
							   && first < t ) ;
	    }
	}
#ifdef	ELABORATOR
	if ((module_table[t].import & MODULE_IMPORT_MASK) == elaborator_import)
	    set_invokes = 1;
#endif
	if( (module[ n ].module_imports - 1) == t )
	{
		module_table[ t ].import |= MODULE_END_FLAG;
		first = -1;
		if( seen )
			module[ last ].flags |= DM_DLTS;
#ifdef	ELABORATOR
		if (set_invokes)
			module[last].flags |= DM_INVOKES;
		set_invokes = 0;
#endif
	    last = n;
	    while( n < mod_number && 
		  module[ ++n ].module_imports-1 <= t ) ;
	    seen = 0;
	}
	}
	if( seen )
		module[ last ].flags |= DM_DLTS;
#ifdef	ELABORATOR
	if (set_invokes)
		module[last].flags |= DM_INVOKES;
#endif
	module_table[ mod_index-1 ].import |= MODULE_END_FLAG;
}

/*----------------------------------------------------------------------
 * 
 * Procedure: rewrite_exec_header()            (replace the exec header)
 *
 * Description:
 *    Thie routine simply rewrites the exec header, after some modifications.
 *    The main exec header is written out in the middle pass, before we
 *    know the number of dynamic relocations, or that a shared library
 *    needs to be a DL_MAGIC shared library.  So, in some cases, we need
 *    to go back and redo it.
 */
rewrite_exec_header()
{
    save_filhdr.a_drelocs = dyn_count;
    fseek( tout, 0L, 0 );
    fwrite( &save_filhdr, sizeof( struct exec ), 1, tout );
}

/*----------------------------------------------------------------------
 *
 * Procedure: WriteDynReloc()                (Write Dynamic Relocations)
 *
 * Description:
 *    Since the shlib text section is written out during the middle pass,
 *    and since we have no information on relocation records at that time,
 *    we will have to write the dynamic relocation records after the
 *    regular relocation entries have been gone through.
 */
WriteDynReloc()
{
    int i, k;
    struct export_entry *eptr;
    struct relocation_entry *rptr;

    if( shlib_level >= SHLIB_DLD )
    {
	/* If there are dynamic relocation in the text segment, we mark the
	 * library as DL_MAGIC, so we need to re-write the main header.
	 */
	if( text_reloc_seen )
	{
	    error( e43 );
	    save_filhdr.a_magic.file_type = DL_MAGIC;
	    rewrite_exec_header();
	}
    }

    if( shlib_level == SHLIB_BUILD )
    {
	/* We need to fill in information about where the first dynamic 
	 * relocation entry is for each exported entry.  The algorithm used
	 * is somewhat ugly, but since this code is only used when building
	 * a shared library, it should not cause too much grief.
	 */
	for( eptr = exports, i = 0; i < expindex; eptr++, i++ )
	{
	    if( eptr->size > 0 )
	    {
		for( rptr = relocs, k = 0; k < curr_relindex && 
		    (eptr->value > rptr->address || rptr->address < tsize);
		    rptr++, k++ ) ;
		if( k != curr_relindex && 
				(eptr->value+eptr->size) > rptr->address )
		    shl_exports[ i ].dreloc = k;
		else
		    shl_exports[ i ].dreloc = -1;
	    }
	    else
		shl_exports[ i ].dreloc = -1;
	}
	fseek( tout, exp_seek, 0 );
	fwrite( shl_exports, sizeof(struct shl_export_entry), expindex, tout );
    }

    /* Finally, output the actual dynamic relocations.
     */
    if( shlib_level > SHLIB_NONE && relindex )
    {
	fseek( tout, dyn_seek, 0 );
	if( curr_relindex+1 != relindex )
#pragma		BBA_IGNORE
	    fatal( e55, "(unknown)" );
	assert( relocs );
	fwrite( relocs, sizeof(struct relocation_entry), relindex, tout );
    }
}

/*----------------------------------------------------------------------
 * 
 * Procedure: WriteShlibData()           (Write data section shlib info)
 *
 * Description: 
 *    Here, we write out the Data section tables (DLT's, PLT's, Dmodule,
 *    DYNAMIC, etc.; the Text section was written out by WriteShlibText()).
 *    Note that in some cases, DLT's will appear in the text segment.
 */
WriteShlibData()
{
   int              i;
   struct plt_entry p;

#ifdef SHLIB_PATH
	/* Pay no attention to the next comment.  SHLIB_PATH space comes first */
	if (shlib_level == SHLIB_A_OUT || shlib_level == SHLIB_BUILD)
		zout(dout,SHLIB_PATH_LEN);
#endif
	
   /* First is the DBOUND array.  Trust me. */
   if (shlib_level == SHLIB_BUILD)
	   zout(dout,(mod_number+3)&~3);

   /* Next is the BOUND array. */
   if (shlib_level == SHLIB_BUILD)
	   zout(dout,bound_size);

   /* Next come the dlt's, including ABS_DLT's and regular DLT's
    */
   if( abs_dltindex )
   {
       assert( shlib_level == SHLIB_DLD );
       fwrite( abs_dlt, sizeof( struct dlt_entry ), abs_dltindex, dout );
   }
   assert( local_dlt + global_dlt == dltindex );
#ifdef	PICR
   if (!rflag)
#endif
   fwrite( dlt, sizeof( struct dlt_entry ), dltindex, 
      (shlib_level == SHLIB_BUILD || shlib_level == SHLIB_DLD) ? dout : tout );

   /* PLT's come now.  It used to be that we would output a BRA for pic
    * complete executables (or for DLD), but this has been fixed so that
    * branches go directly where they should (no more need for a PLT).
    */
   if (pltindex) 
   {
       assert( shlib_level > SHLIB_DLD );

       /* align PLT on 8 byte boundary */
       zout(dout,plt_align);
       p.opcode = BSR;
       p.size = 0xFF;
       p.spare1 = 0;
       for (i = 0; i < pltindex; i++)
       {
	   p.displacement = ((long)&dynheader.bor - (long)&dynheader) + 
		dynheader.text + extheader.e_spec.dl_header.dynamic -
		(int)dynheader.plt - (sizeof(struct plt_entry) * i) - 2;
	   fwrite(&p, sizeof(struct plt_entry), 1, dout);
       }
   }

   /* Write out a DYNAMIC structure (the only reason for us not to have
    * one is if we have a complete PIC executable).
    */
   if( shlib_level >= SHLIB_DLD )
       fwrite(&dynheader, sizeof(struct dynamic), 1, dout);
}

/*----------------------------------------------------------------------
 *
 * Procedure: shlib_tables()      (Generates shlib tables in middle pass)
 *
 * Description:
 *    This procedure handles the middle pass for shlib information.
 *    This routine takes one full pass through the linker symbol table,
 *    while performing the following tasks (in order):
 *
 *         1) If a symbol needs to be converted from FUZZY to Non-Fuzzy,
 *            this is handled first.
 *         2) If a symbol is UNDEF, it is assumed to be a TEXT symbol,
 *            and a PLT is created.
 *         3) If a PLT is needed, one is created.
 *         4) If a symbol needs to be EXPORTED, and export entry is built.
 *         5) If a DLT is needed, one is created.
 *         6) If a symbol still does not have an IMPORT entry, and it's 
 *            .n_list flag is set (indicating dynamic relocation), an
 *            import list entry is created.
 * 
 */       
shlib_tables()
{
    register symp sp, sp2;
    register int t;
    register int seen;
    long flag, f, temp, i;
    long fuzzy_tail;

    /* Initialize the fuzzy_data linked list.  'fuzzy_head' is a static
     * global variable.
     */
    fuzzy_head = fuzzy_tail = -1;

#ifdef	VISIBILITY
	if (!bflag && hide_status)
	{
		int slot;
		hide p;

		for (p = hidelist; p != NULL; p = p->hide_next)
		{
			if ((slot = slookup(p->hide_name)) == -1)
			{
				error("(warning) no such symbol %s",p->hide_name);
				continue;
			}
			if (hide_status == EXPORT_HIDES)
				symtab[slot].expindex = EXP_UNDEF;
			else if (hide_status == HIDE_HIDES)
				symtab[slot].expindex = EXP_DONT_EXPORT;
#ifdef	BBA
			else
#pragma	BBA_IGNORE
				;
#endif
		}
	}
#endif

    /* Scan through the entire symbol table.
     */
    for( sp = symtab, i = flag = seen = 0; i < symindex; sp++, i++, seen = 0 )
    {
	/* If undefined, make a warning, go ahead and assume TYPE_PROC
	 * (for the import list), and create an SHL_PROC
	 * We also need to catch fuzzy undefines as well (which are just
	 * as bad as pure undefines), but don't make an SHL_PROC
	 */
	if( (sp->s.n_type & FOURBITS) == UNDEF && sp->s.n_value == 0 &&
#ifdef	DL_MAGIC_RPC_FIX
		(!(pic & M_PIC)))
#else
	    shlib_level == SHLIB_A_OUT )
#endif
	{
	    if( !flag )
	    {
#ifdef	DL_MAGIC_RPC_FIX
		if (shlib_level == SHLIB_BUILD)
			error(e35, "(warning) ");
		else
#endif
#ifdef FAKE_SHLIB_AS_OBJECT
		if (Fflag < 10)
#endif
			error(e35, "");
		flag = 1;
	    }
#ifdef FAKE_SHLIB_AS_OBJECT
	    if (Fflag < 10)
#endif
	    fprintf(stderr,"\t%s\n",asciz( sp->sname,sp->s.n_length));
#ifdef	FTN_IO
	    if (ftn_io(asciz(sp->sname,sp->s.n_length)))
		ftn_io_mismatch = 1;
#endif
#ifdef	DL_MAGIC_RPC_FIX
		if (shlib_level == SHLIB_BUILD)
			sp->s.n_flags |= NLIST_PLT_FLAG;
		else
#endif
	    if (!(sp->s.n_flags & NLIST_STILL_FUZZY_FLAG))
	    {
		sp->s.n_type = EXTERN | TEXT;
		sp->s.n_flags = (sp->s.n_flags & 
			~(NLIST_STILL_FUZZY_FLAG|NLIST_EVER_FUZZY_FLAG)) 
			| NLIST_EXPORT_PLT_FLAG | NLIST_PLT_FLAG;
		sp->pltindex = PltEnter( sp, -1 );
		sp->s.n_value = sp->pltindex * sizeof( struct plt_entry );
		if( shlib_level == SHLIB_A_OUT )
		    sp->expindex = EXP_UNDEF;
		else
		    sp->expindex = EXP_DONT_EXPORT;
	    }
	}

	/* If it was a fuzzy define, and somebody needed it, then 
	 * the NLIST_STILL_FUZZY_FLAG == 0 and NLIST_EVER_FUZZY_FLAG == 1.
	 * In this case, allocate space for it in the appropriate space,
	 * assign the appropriate values to the symbol.  We will also need
	 * to create a special PLT if it is defined here.
	 */
	if( sp->s.n_flags & NLIST_EVER_FUZZY_FLAG )
	{
	    if( sp->s.n_flags & NLIST_STILL_FUZZY_FLAG )
	    {
		continue;   /* *********** watch the continue ********* */
	    }
	    else
	    {
		/* In the following case, the symbol has already been
		 * changed from fuzzy to non-fuzzy, so we only need to 
		 * copy over value, pltindex, etc. stuff from the 
		 * defining symbol.
		 */
		/* this happens if it was an "other def" of a previous sym */
		if( fuzzy_shl[ f = sp->shlib ].sym >= 0 )
		{
		    sp2 = &symtab[ fuzzy_shl[ f ].sym ];
		    sp->s.n_value = sp2->s.n_value;
		    sp->shlib = fuzzy_shl[ f ].shlib;
		    if( sp2->pltindex != PLT_UNDEF )
		    {
			sp->pltindex = sp2->pltindex;
			sp->s.n_flags |= NLIST_EXPORT_PLT_FLAG;
		    }
		}
		else
		/* This guy is still very fuzzy.  We need to do all the
		 * work of transfering it from a fuzzy def to a regular
		 * definition.
		 */
		{
		    fuzzy_shl[ f ].sym = sp->sindex;

		    /* For all aliased symbols, we need to mark them 
		     * as requiring the conversion (when they are seen
		     * in the course of our grand scan through the
		     * symbol table).  Note that this breaks the linked
		     * list of symbols (which we no longer need).
		     */
		    t = sp->sindex;
		    do
		    {
			symtab[temp=t].s.n_flags &= ~(NLIST_STILL_FUZZY_FLAG);
			t = symtab[ temp ].alias;
			symtab[temp].size = fuzzy_shl[ f ].size;
		    }
		    while( t != sp->sindex );

		    sp->shlib = fuzzy_shl[ f ].shlib;

		    switch( sp->s.n_type & FOURBITS )
		    {
		      case DATA:
			/* Data symbols are the most fun.  We must add a
			 * record so that copy_fuzzy_data() can copy the
			 * data from the shlib into our a.out file.  We
			 * also must make sure that these guys are added
			 * at the end, since they are allocated in numberical
			 * order inside the data segment.
			 */
			if( fuzzy_tail == -1 )
			    fuzzy_head = fuzzy_tail = f;
			else
			{
			    fuzzy_shl[ fuzzy_tail ].next = f;
			    fuzzy_tail = f;
			}
			fuzzy_shl[ f ].next = -1;

			/* Figure out the alignment for the data when it 
			 * gets copied over into the a.out file.
			 */
			t = fuzzy_shl[ f ].align - (dsize & 3);
			if( t < 0 )
#pragma		BBA_IGNORE
				t += 4;
			fuzzy_shl[ f ].align = t;

			/* Now, if we expect to have dynamic relocation,
			 * we will need to create a space for the relocation
			 * record.
			 */
			if( fuzzy_shl[ f ].flags & FUZZY_PROPOGATE_DRELOC )
			{
			    relindex++;
			    sp->s.n_flags |= NLIST_DRELOC_FLAG;
			}

			/* Allocate it in data space, up the size.
			 */
			sp->s.n_value = dsize + t;
			dsize += t + fuzzy_shl[ f ].size;
#ifdef EXPORT_MORE
			sp->expindex = EXP_UNDEF;
#endif

			break;

		      case TEXT:
			/* Text symbols are easy.  Create a PLT, and insure
			 * that it will be exported.
			 */
			sp->pltindex = PltEnter( sp, sp->shlib );
			sp->s.n_value = sp->pltindex*sizeof(struct plt_entry);
			sp->s.n_flags |= NLIST_EXPORT_PLT_FLAG;
			break;

		      case BSS:
			/* BSS symbols also need to be aligned, and 
			 * allocated within the BSS section of the a.out.
			 */
			sp->s.n_type = (BSS | EXTERN);
			t = fuzzy_shl[ f ].align - (bsize & 3);
			if( t < 0 )    t += 4;
			fuzzy_shl[ f ].align = t;

			sp->s.n_value = bsize + t;
			bsize += t + fuzzy_shl[ f ].size;

			break;

		      case UNDEF:
			/* We should not have gotten this far (undef's
			 * are handled above).  But, these will be handled in
			 * common(), or above if the symbol's value is zero.
			 */
#pragma		BBA_IGNORE
			break;   

		      case ABS:
			/* Absolutes are pretty easy too. 
			 */
			sp->s.n_value = fuzzy_shl[ f ].offset;
			break;

		      default:
#pragma  BBA_IGNORE
			assert( 0 );
			break;
		    }
		}
	    }
	}


	/* If this item was defined in a shared library, then it will 
	 * already have a plt.
	 */
	if( sp->shlib == SHLIB_UNDEF || shlib_level == SHLIB_BUILD )
	{
	    /* Make plt/dlt entries as well as needed reloc_import entries.
	     */
	    if( sp->s.n_flags & NLIST_PLT_FLAG ) 
	    {
		if( shlib_level == SHLIB_BUILD )
		{
		    sp->pltindex = PltEnter( sp, -1 );
#ifdef	DL_MAGIC_RPC_FIX
			if ((sp->s.n_type & FOURBITS) == UNDEF && !(pic & M_PIC))
				pltimports[sp->pltindex].type = TYPE_UNDEFINED;
#endif
		    seen = 1;

		    if( (t = sp->shlib) != SHLIB_UNDEF )
		    {
			do
			{
			    module_table[ t ].import = sp->pltindex | 
						       MODULE_ADD_DLTINDEX;
			    t = module_next[ t ];
			}
			while( t != SHLIB_UNDEF );
		    }

		    if( sp->s.n_flags & NLIST_EXPORT_PLT_FLAG )
			sp->s.n_value = sp->pltindex *
					sizeof( struct plt_entry );
		}
		/* This appears to be a bug - this case could be hit
		   by an unresolved pic proc ref in an a.out,
		   in which case we do need a PLT.
		   Luckily, the PLT_LOCAL_RESOLVE code will resolve the ref
		   to the PLT, so everything works. */
		else
		{
		    /* If we are resolving localy within anything but a shlib
		     * proper, we can go ahead and patch the text segment 
		     * directly to point to the proper place.  
		     * No need for a PLT.
		     */
		    sp->pltindex = PLT_LOCAL_RESOLVE;
		}
	    }
	}

	/* Enter all symbols which are defined.
	 * Exports for shared libraries are entered
	 * in enter() since there may be multiple versions.  These are not
	 * entered there because a shared library may not need a symbol.
	 */
	if( (sp->s.n_type & FOURBITS) != UNDEF
	    && shlib_level == SHLIB_A_OUT && sp->expindex == EXP_UNDEF )
	{
	    sp->expindex = ExpEnter( sp, sp->s.n_type, sp->s.n_value, 0, 
				     sp->size, -1, -1, -1 );
	}

	if( sp->s.n_flags & NLIST_DLT_FLAG && !rflag )
	{
	    sp->dltindex = DltEnter( sp, sp->s.n_type & FOURBITS,
				     sp->s.n_value, 0 );
	    if( shlib_level == SHLIB_BUILD )
	    {
		seen = 1;

		if( (t = sp->shlib) != SHLIB_UNDEF )
		{
		    do
		    {
			module_table[ t ].import = sp->dltindex;
			t = module_next[ t ];
		    }
		    while( t != SHLIB_UNDEF );
		}
	    }
	}

	/* If we still have not created an import list entry for this
	 * item, we need to create one for dynamic relocation records.
	 * Also, if an item is undefined and not used by the shared library
	 * in any way, we still need an import list entry for the
	 * module import lists to reference.
	 */
	if( !seen && (sp->s.n_flags & NLIST_DRELOC_FLAG)
	    && shlib_level != SHLIB_NONE )
	{
	    if (rimpindex == ld_rimpsize) {
		ld_rimpsize *= 2;
		reloc_imports = (struct import_entry *) 
		    realloc(reloc_imports, 
			    ld_rimpsize * sizeof(struct import_entry));
	    }

	    sp->rimpindex = rimpindex;

	    enter_string_table( sp );
	    reloc_imports[ rimpindex ].name = sp->sname - stringt;

	    reloc_imports[ rimpindex ].shl = (shlib_level == SHLIB_A_OUT ?
					      sp->shlib : -1);
	    switch( sp->s.n_type & FOURBITS )
	    {
	      case UNDEF:     t = TYPE_UNDEFINED;   break;
	      case DATA:      t = TYPE_DATA;        break;
	      case TEXT:      t = TYPE_PROCEDURE;   break;
	      case COMM:      t = TYPE_COMMON;      break;
	      case BSS:       t = TYPE_BSS;         break;
	      case ABS:       t = TYPE_ABSOLUTE;    break;
	      default:
#pragma BBA_IGNORE
			assert( 0 );          break;
	    }
	    reloc_imports[ rimpindex++ ].type = t;

	    if( shlib_level == SHLIB_BUILD && (t = sp->shlib) != SHLIB_UNDEF )
	    {
		do
		{
		    module_table[ t ].import =
				    sp->rimpindex | MODULE_ADD_BOTH;
		    t = module_next[ t ];
		}
		while( t != SHLIB_UNDEF );
	    }
	}
    }
#ifdef FAKE_SHLIB_AS_OBJECT
    if (flag && Fflag < 10)
#else
    if (flag)
#endif
    {
#ifdef	DL_MAGIC_RPC_FIX
	if (shlib_level != SHLIB_BUILD)
#endif
		exit_status = 1;
	error(e34);
#ifdef	RESTORE_MOD
	if (shlib_level == SHLIB_A_OUT || (shlib_level == SHLIB_BUILD && !(pic & M_PIC)))
		error(e47);
#endif
#ifdef	FTN_IO
	if (ftn_io_mismatch)
	    fprintf(stderr,"Possible FORTRAN library version mismatch.  Recompile with FCOPTS=+I300\n");
#endif
    }

    /* If we ended up defining things as fuzzy, we need to align the
     * data and bss sections to 4 bytes so that PLT, etc. will be aligned
     * in the proper way.
     */
    fuzzy_align_dsize = ((dsize + 3) & ~03) - dsize;
    dsize += fuzzy_align_dsize;
    bsize = (bsize + 3) & ~03;
}

/*----------------------------------------------------------------------
 *
 * Procedure: write_fuzzy_data()            (Write out shlib fuzzy data)
 *
 * Description:
 *   During shlib_tables(), it may be found that a piece of fuzzy data
 *   is indeed needed by the a.out file.  In this case, the fuzzy define
 *   changes into a real define.  If it was data, that data needs to be
 *   copied into the a.out file from the shlib.  This routine will do
 *   this copying, at the end of the a.out file (we can't place the needed
 *   data where the shlib was loaded).  Thus, all these fuzzy rejects
 *   are collected here, and written out here.
 *  
 */
/* Note - this whole charade could have been eliminated had we collected the
   fuzzy data during pass2 (into a temp file), and then we could have just
   appended that file here. */
write_fuzzy_data()
{
    FILE *in = NULL;
    struct symbol *sp;
    long last = -1;

    /* Shlib_tables() built us a linked list of fuzzy items which need
     * to be converted to real items.  We will traverse this linked list
     * and copy each item from the shlib into the a.out file (doing
     * alognments as we go).
     */
    for( ; fuzzy_head != -1; fuzzy_head = fuzzy_shl[ fuzzy_head ].next )
    {
	/* If this item is from the same file, we don't need to close it 
	 * and reopen it again.
	 */
	if( last != fuzzy_shl[ fuzzy_head ].shlib )
	{
	    if( in != NULL )
		fclose( in );
	    if( (in = fopen(shlibs[fuzzy_shl[fuzzy_head].shlib].name+stringt,
			    "r")) == NULL )
	    {
#pragma		BBA_IGNORE
		fatal( e15, shlibs[fuzzy_shl[fuzzy_head].shlib].name+stringt);
	    }
	    last = fuzzy_shl[ fuzzy_head ].shlib;
	}

	/* Go to the correct place in the shlib, do what ever alignment 
	 * we need to do for the a.out file, and finally, copy the data
	 * over into the a.out file.
	 */
	fseek( in, fuzzy_shl[fuzzy_head].offset, 0 );
	zout( dout, fuzzy_shl[fuzzy_head].align );
	dcopy( in, dout, fuzzy_shl[fuzzy_head].size );

	/* Like any data copied over from the shared library, we need to
	 * have a PROPOGATE relocation record in the dynamic relocation
	 * section (not really, only if there are dynamic relocations for
	 * the given item).
	 */
	if( fuzzy_shl[fuzzy_head].flags & FUZZY_PROPOGATE_DRELOC )
	{
	    relocs[ curr_relindex ].address = dorigin + 
					      fuzzy_shl[fuzzy_head].align;
	    sp = &symtab[ fuzzy_shl[fuzzy_head].sym ];
	    relocs[ curr_relindex ].symbol = 
		(sp->pltindex != PLT_UNDEF ? sp->pltindex :
					     pltindex + sp->rimpindex);
	    relocs[ curr_relindex ].type = DR_PROPAGATE;
	    relocs[ curr_relindex++ ].length = 0;
	}

	/* Update where we are in the a.out file.
	 */
	dorigin += fuzzy_shl[fuzzy_head].align + fuzzy_shl[fuzzy_head].size;
	doffset += fuzzy_shl[fuzzy_head].align + fuzzy_shl[fuzzy_head].size;
    }

    /* We need to close that last file.
     */
    if( in != NULL )
	fclose( in );

    free( fuzzy_shl );     /* No longer needed */

    /* These values are computed in shlib_tables().
     */
    dorigin += fuzzy_align_dsize;
    doffset += fuzzy_align_dsize;
    zout(dout,fuzzy_align_dsize);
}

/* <<<<<<<<<<<<<<<<<<<<<<<<<<<< end of shlib.c >>>>>>>>>>>>>>>>>>>>>>>>>>> */

