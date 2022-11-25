/* Internal RCS $Header: main.c,v 70.4 92/04/22 10:37:36 ssa Exp $ */

/* @(#) $Revision: 70.4 $ */   

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

     Command:      Series 200 ld   ( HP -UX link editor )
     File   :      main.c

     Purpose:      overall driver for the link process
		   - does command line parsing
		   - supervises pass1, middle and pass2
		   - allocates space for symbol table data structures
		   - handles incrememtal load (-A option)

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define  LD_MAIN
#include "ld.defs.h"
#include <signal.h>
#include <ctype.h>
#include <string.h>
#ifdef   TIME_LD
#include <sys/types.h>
#include <sys/param.h>
#include <sys/times.h>
#endif

/* global variables */
undef libdirlist;			/* linked list of -L dirs */
int libdirsiz = 0;			/* size of names in libdirlist */

int argc;         /* For global versions of the parameters to main() */
char **argv;
static char *next_arg;
#ifdef	MISC_OPTS
static FILE *opt_file = NULL;
#endif

#ifdef	BBA
long atox();
#endif
void 	delexit();

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* main -	The link editor works in two passes.  In pass 1, symbols
		are defined.  If alignment specifying symbols are present,
		an additional operation in the middle is performed to
		determine how much total white space is to be included in
		the output file. In pass 2, the actual text and data will
		be output along with any relocation commands and symbols.
*/

main(ac, av)
int ac;
char **av;
{
	register arg argp;	/* current argument */
	register undef u_ptr;	/* undef list ptr */
	register undef u_ptr1;	/* undef item ptr used in free'ing list */
	predef p_ptr;
	predef p_ptr1;
	register int  i;
	int old_bsize;

	/* replace (default) signal handling */
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
#pragma BBA_IGNORE
		signal(SIGINT, delexit);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
#pragma BBA_IGNORE
		signal(SIGTERM, delexit);

	torigin = LADDR;	/* reset with -R flag */

	/* process arguments */
	argc = ac;
	argv = av;
	procargs();
	if(arglist == NULL && !rflag) fatal(e28);
	entryval = torigin;

	/* These are the main memory areas used by the shlib routines.
	 * Most are malloc()'s, since we don't need them to be cleared.
	 * Notice that we assume that procargs() has already been run, such
	 * that shlib_level is set.
	 */
	exports = (struct export_entry *) 
		      malloc( ld_expsize * sizeof(struct export_entry) );
	if( shlib_level == SHLIB_BUILD )
		shl_exports = (struct shl_export_entry *)
		          malloc( ld_expsize * sizeof(struct shl_export_entry) );

	stringt = (char *) malloc( ld_stringsize * sizeof(char) );
	sym_stringt = (char *) malloc( ld_sym_stringsize * sizeof(char) );

	dltimports = (struct import_entry *)
		         malloc( ld_dltsize * sizeof(struct import_entry) );
	dlt = (struct dlt_entry *) 
		  malloc( ld_dltsize * sizeof(struct dlt_entry) );
	abs_dlt = (struct dlt_entry *)
		      malloc( ld_abs_dltsize * sizeof(struct dlt_entry) );

	pltimports = (struct import_entry *)
		         malloc( ld_pltsize * sizeof(struct import_entry) );
	pltchain = (long *)
		       malloc( ld_pltsize * sizeof(long) );

	if( shlib_level == SHLIB_NONE || shlib_level == SHLIB_A_OUT )
	{
		shlibs = (struct shl_entry *)
			     malloc( ld_shlsize * sizeof(struct shl_entry) );
		shldata = (struct shl_data_copy *)
		          malloc( shldata_size * sizeof( struct shl_data_copy) );
	    fuzzy_shl = (struct fuzzy_shl_copy *)
		            malloc( fuzzy_shl_size * sizeof( struct fuzzy_shl_copy) );
	}

	reloc_imports = (struct import_entry *)
		            malloc( ld_rimpsize * sizeof(struct import_entry) );

	if( shlib_level == SHLIB_BUILD )
	{
		module_next = (long *)
		              malloc( ld_mod_index * sizeof( long ) );
		module = (struct dmodule_entry *)
			     malloc( ld_mod_number * sizeof( struct dmodule_entry ) );
		ld_module = (struct mod_entry *)
			        malloc( ld_mod_number * sizeof( struct mod_entry ) );
	}

	symtab = (struct symbol *) calloc( ld_stsize, SYMBOLSIZE );
	if( rflag )
		supsym = (struct supsym_entry *)
			     calloc( ld_stsize, sizeof( struct supsym_entry ) );
	local = (symp *) calloc( ld_stsize, sizeof(symp) );
	hashmap = (int *) malloc( ld_sthashsize * sizeof(int) );
	chain = (int *) calloc( ld_stsize, sizeof(int) );

        /* initialize the hash chains to be empty lists */
	for( i = ld_sthashsize; i > 0; )
		hashmap[ --i ] = 0x80000000 ;
    hash_cycle = ld_sthashsize;
#ifdef	VFIXES
	/* initialize vfixes list */
	for (i = 0; i < VFIXES; ++i)
	{
		vfixes[i].new.location = -1;
		vfixes[i].old.location = -1;
	}
	strcpy(vfixes[0].new.name,"_ldiv");
	strcpy(vfixes[0].old.name,"___ldiv");
	strcpy(vfixes[1].new.name,"_setjmp");
	strcpy(vfixes[1].old.name,"____setjmp");
	strcpy(vfixes[2].new.name,"__setjmp");
	strcpy(vfixes[2].old.name,"_____setjmp");
	strcpy(vfixes[3].new.name,"_longjmp");
	strcpy(vfixes[3].old.name,"____longjmp");
	strcpy(vfixes[4].new.name,"__longjmp");
	strcpy(vfixes[4].old.name,"_____longjmp");
#endif


	/* set-up library search path - lpath */
	lpath = (char *) getenv("LPATH");
	if (!lpath || (strlen(lpath) == 0))
#ifdef xcomp300_800
	lpath = "/usr/local/300comp/lib:/usr/local/300comp/usr/lib";
#else
	lpath = STANDARD_LPATH;
#endif
	if (libdirlist != NULL)
	{
		char *x;
		x = (char *) malloc(strlen(lpath)+libdirsiz+2);

		x[0] = '\0';
		for (u_ptr=libdirlist; u_ptr; u_ptr=u_ptr->undef_next)
		{
			strcat(x,u_ptr->undef_name);
			strcat(x,":");
		}
		strcat(x,lpath);
		lpath =x;
	}
#ifdef SHLIB_PATH
	if (embed_path != NULL && !strcmp(embed_path,":"))
		embed_path = lpath;
#endif /* SHLIB_PATH */
	if (vflag)
		printf("LPATH is : %s\n",lpath);
	
	/* pass 1 */
	if (Aname != NULL)
	{
		/* produces an un-shared a.out */
		nflag = 0; qflag = 0;
		funding = 1;
		load1arg(Aname, EITHER, BIND_IMMEDIATE);
		Asymp = &symtab[symindex];
		if (savernum==-1) 
		/* no -R option use old value of _end */
		{
			   if (filhdr.a_magic.file_type==SHARE_MAGIC ||
			       filhdr.a_magic.file_type==DEMAND_MAGIC )
				   tsize = EXEC_ALIGN(tsize);
			   torigin = tsize+dsize+bsize;
		}
		rtsize = 0; rdsize = 0; tsize = 0; dsize = 0; bsize = 0;

		headersize = 0; 
		gnttsize = 0; 
		lnttsize = 0; 
		sltsize = 0; 
		vtsize= 0;
		xtsize= 0;

		ctrel = 0; cdrel = 0; cbrel = 0;
		funding = 0;
	}
	/* enter -u symbols in the symbol table */
	if (undeflist != NULL)
	{
		for (u_ptr=undeflist; u_ptr;)
		{
			enter(slookup(u_ptr->undef_name));
			u_ptr1 = u_ptr;
			u_ptr = u_ptr->undef_next;
			free ((char *) u_ptr1);
		}
	}
	/* enter -P symbols, too */
	if (predeflist != NULL)
	{
#pragma		BBA_IGNORE
		for (p_ptr = predeflist; p_ptr; )
		{
			enter(slookup(p_ptr->predef_name));
			ldrsym(slookup(p_ptr->predef_name),p_ptr->predef_value,EXTERN|ABS);
			p_ptr1 = p_ptr;
			p_ptr = p_ptr->predef_next;
			free(p_ptr1);
		}
	}
	if (e_name)
	{	enter(slookup(e_name));
		entrypt = lastsym;
	}

	/* Certain symbols need to be defined here so that they are not 
	 * found in a shared library. (bad).  The values given are mainly
     * dummy values, which will get properly resolved in middle().
	 */
	if( !rflag )
	{
		enter(slookup("PLT")); 
		ldrsym(i=slookup("PLT"),0,EXTERN|DATA);
		symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef	VISIBILITY
		if (hide_status != EXPORT_HIDES)
#endif
			newhide("PLT");

		enter(slookup("DLT"));     
		ldrsym(i=slookup("DLT"),0,EXTERN|DATA);
		symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef	VISIBILITY
		if (hide_status != EXPORT_HIDES)
#endif
			newhide("DLT");

		enter(slookup("BOUND"));
		ldrsym(i=slookup("BOUND"),0,EXTERN|DATA);
		symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef	VISIBILITY
		if (hide_status != EXPORT_HIDES)
#endif
			newhide("BOUND");

		enter(slookup("DBOUND"));
		ldrsym(i=slookup("DBOUND"),0,EXTERN|DATA);
		symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef	VISIBILITY
		if (hide_status != EXPORT_HIDES)
#endif
			newhide("DBOUND");

#ifdef  HOOKS
		enter(slookup("___dld_flags"));
		ldrsym(i=slookup("___dld_flags"),0,EXTERN|DATA);
		symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef	VISIBILITY
		if (hide_status != EXPORT_HIDES)
#endif
			newhide("___dld_flags");
		enter(slookup("___dld_hook"));
		ldrsym(i=slookup("___dld_hook"),0,EXTERN|DATA);
		symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef	VISIBILITY
		if (hide_status != EXPORT_HIDES)
#endif
			newhide("___dld_hook");
		enter(slookup("___dld_list"));
		ldrsym(i=slookup("___dld_list"),0,EXTERN|DATA);
		symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef	VISIBILITY
		if (hide_status != EXPORT_HIDES)
#endif
			newhide("___dld_list");
#endif

#ifdef SHLIB_PATH
		enter(slookup("___dld_shlib_path"));
		ldrsym(i=slookup("___dld_shlib_path"),0,EXTERN|DATA);
		symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef VISIBILITY
		if (hide_status != EXPORT_HIDES)
#endif /* VISIBILITY */
			newhide("___dld_shlib_path");
		enter(slookup("___dld_embed_path"));
		ldrsym(i=slookup("___dld_embed_path"),0,EXTERN|ABS);
		symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef VISIBILITY
		if (hide_status != EXPORT_HIDES)
#endif /* VISIBILITY */
			newhide("___dld_embed_path");
#endif /* SHLIB_PATH */

		dont_import_syms = symindex;

		enter(slookup("__DYNAMIC")); 
		ldrsym(slookup("__DYNAMIC"),0,EXTERN|DATA);
		if (shlib_level == SHLIB_BUILD || shlib_level == SHLIB_DLD)
			symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef	VISIBILITY
		if (hide_status != EXPORT_HIDES)
#endif
			newhide("__DYNAMIC");

	    if (Aname == 0
		   ) {
		    enter(slookup("__etext")); 
		        ldrsym(i=slookup("__etext"),0,EXTERN2|EXTERN|TEXT);
		        if( bflag ) symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef EXPORT_MORE
				else symtab[i].expindex = EXP_UNDEF;
#endif
		    enter(slookup("_etext"));   
		        ldrsym(i=slookup("_etext"),0,EXTERN2|EXTERN|TEXT);
		        if( bflag ) symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef EXPORT_MORE
				else symtab[i].expindex = EXP_UNDEF;
#endif
		    enter(slookup("__edata"));  
		        ldrsym(i=slookup("__edata"),0,EXTERN2|EXTERN|DATA);
		        if( bflag ) symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef EXPORT_MORE
				else symtab[i].expindex = EXP_UNDEF;
#endif
		    enter(slookup("_edata"));
		        ldrsym(i=slookup("_edata"),0,EXTERN2|EXTERN|DATA);
		        if( bflag ) symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef EXPORT_MORE
				else symtab[i].expindex = EXP_UNDEF;
#endif
		    enter(slookup("__end"));   
		        ldrsym(i=slookup("__end"),0,EXTERN2|EXTERN|BSS);
		        if( bflag ) symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef EXPORT_MORE
				else symtab[i].expindex = EXP_UNDEF;
#endif
		    enter(slookup("_end"));   
		        ldrsym(i=slookup("_end"),0,EXTERN2|EXTERN|BSS);
		        if( bflag ) symtab[i].expindex = EXP_DONT_EXPORT;
#ifdef EXPORT_MORE
				else symtab[i].expindex = EXP_UNDEF;
#endif
		}
#ifdef	VISIBILITY
		dont_export_syms = symindex;
#endif
	}

	for (argp = arglist; argp; argp = argp->arg_next)
		load1arg(filename = argp->arg_name, argp->lib, argp->bind);
	
	middle();

	if (bss2data)
	{
		old_bsize = bsize;
		dsize += bsize;
		bsize = 0;
	}
	
	/* pass 2 */
	setupout();

#if   CHKMEM
	chk_sides();
#endif

	FillInShlibStuff();
	WriteShlibText();

	if (Aname != NULL)
	{	funding = 1;
		load2arg(Aname,EITHER);
		funding = 0;
	}

#if   CHKMEM
	chk_sides();
#endif
	
	for (argp = arglist; argp; argp = argp->arg_next)
		load2arg(filename = argp->arg_name, argp->lib);

	if( shlib_level > SHLIB_NONE && fuzzy_head != -1 )
			write_fuzzy_data();
	WriteShlibData();
	if (bss2data)
		zout(dout,old_bsize);
	WriteDynReloc();   

	if( rflag )                   /* To catch changes to .a_drelocs */
		rewrite_exec_header();

	finishout();

#if   CHKMEM
	chk_sides();
#endif

#ifdef TIME_LD
    {
		struct tms timeb;
		FILE *fp;

	    times( &timeb );
		if( (fp = fopen( TIME_LD, "a" )) != NULL )
		{
			fprintf( fp, "%ld + %ld = %ld\n",
					 timeb.tms_utime, timeb.tms_stime, 
					 timeb.tms_utime + timeb.tms_stime );
			fclose( fp );
		}
		else
			fprintf( stderr, "bad\n" );
	}
#endif

	exit(exit_status);
}

#pragma BBA_IGNORE
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* delexit - 	exit routine for processing a KILL or BREAK signal */

#ifdef  __STDC__
void delexit( sig )
int sig;
#else
void delexit()
#endif
{
	fatal(e16);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* procargs -	Process command arguments */

procargs()
{
	while( get_next_arg( 0 ) )    /* for each arg */
	{
		if (*next_arg == '-') minus_procflags( next_arg );
		else if( *next_arg == '+' ) plus_procflags( next_arg );
		else newarg( next_arg );	                 /* build linked list */
	}

	/* check for conflicting options */
	if (rflag && sflag)
	{
	   error(e30);
	   sflag = 0;
        }
	if (rflag)
	{
	   nflag = 0;
	   if (Rflag) error(e29,'R','r','R');
	   Rflag = 0;
#ifdef	MISC_OPTS
	   if (dorigin) error(e29,'D','r','D');
	   dorigin = 0;
#endif
	   if (qflag) error(e29,'q','r','q');
	   qflag = 0;
	   if( bflag ) error(e29,'b','r','b');
	   bflag = 0;
	   if( iflag ) error(e29,'i','r','i');
	   iflag = 0;
	   shlib_level = SHLIB_NONE;
	}

	if (Rflag) torigin = savernum;
	/* if output is to be stripped, no need to store locals */
	if (sflag) xflag++;

	/* Print LDOPTS if necessary */
	if ( vflag && getenv("LDOPTS") )
		printf( "LDOPTS is : %s\n", getenv("LDOPTS") );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* get_next_arg - Get arguments from the environment variable 'LDOPTS' and
 * then from the command line.  One argument, specifing whether the end 
 * of arguments should be ok (0) or an error (anything else).  This argument
 * should be the letter of the option which was expecting another argument.
 */
get_next_arg( flag )
char flag;
{
	static char *environ = NULL;
	static int argnum = 0;
	char *top;

	if( !argnum )
	{
		if( !environ )
		{
			top = (char *) getenv("LDOPTS");
			if( top == NULL )
				argnum = 1;
			else
			{
				environ = malloc( (strlen( top ) + 1) * sizeof(char) );
				assert( environ );
				strcpy( environ, top );
			}
		}
		
		if( environ )
		{
			for( ; *environ && isspace( *environ ); environ++ ) ;
			if( *environ )
			{
				top = environ;
				for( ; *environ && !isspace( *environ ); environ++ ) ;
				if( *environ )
					*(environ++) = '\0';
				else
					argnum = 1;

				next_arg = top;
				return( 1 );
			}
			else
#pragma		BBA_IGNORE
				;
			argnum = 1;
		}
	}

#ifdef	MISC_OPTS
	if (opt_file != NULL)
	{
		char arg_buf[512];

		if (fgetword(opt_file,arg_buf) != NULL)
		{
			next_arg = strdup(arg_buf);
#ifdef	DEBUG
			printf("file option = %s\n",next_arg);
#endif
			return 1;
		}
		else
		{
			fclose(opt_file);
			opt_file = NULL;
		}
	}
#endif

	/* Read arguments from command line */
	if( argnum )
	{
		if( argnum == argc )
		{
			if( flag != '\0' )
				error( e1, flag );
			return( 0 );
		}
		next_arg = argv[ argnum++ ];
		return( 1 );
	}
#ifdef	BBA
	else
#pragma		BBA_IGNORE
		;
#endif
	
	/* Should never have gotten this far */
	assert( 0 );
}


#ifdef	MISC_OPTS
insert_args (file)
const char *file;
{
	if ((opt_file = fopen(file,"r")) == NULL)
		error("unable to open option file %s",file);
}

/* the following code was stolen from the 800 linker */
/* Fgetword gets the next word from the file f and places it in buf. */

#pragma		BBA_IGNORE
int fgetword(f, buf)
FILE *f;
char *buf;
    {
    int new_word = TRUE;
    int c, lastc;

    while ((c = getc(f)) != EOF)  {
	if (!isspace(c) && c != ',' && c != ';')
	    break;
	}

    if (c == EOF)
	return (FALSE);
       
    lastc = c;
    *buf++ = c;
    new_word = TRUE;

    while ((c = getc(f)) != EOF) {
        if ((lastc == '#') && new_word) {

            /* get rid of the last character which is a # */
            buf = buf - 1;

    	    if (c == '#')  {       /* ## is taken as a single # */
	        new_word = FALSE;
		lastc = NULL;
		}

	    else  {                /* we have a comment */
	       /* add check to make sure that we are only skipping characters
                  for the current comment file. If the user just specify  
                  #\n for the comment line, we used to skip the entire next 
                  line as comment.       */
                if (c !='\n') {
                   /* move to a new line */
                   while (((c = getc(f)) != EOF) && (c != '\n'))   
	               ;

		   if (c == EOF)
		       return (FALSE);
                   }
		/* no longer a comment - skip white space */
		while ((c = getc(f)) != EOF) {
		    if (!isspace(c) && c != ',' && c != ';')
			break;
			}

		if (c == EOF)
		    return (FALSE);

	        lastc = c;
	        new_word = TRUE;
	        }
	    }
	else { /* still traversing a single token */
	    new_word = FALSE;
	    if (isspace(c) || c == ',' || c == ';')
	        break;
  	    }

        *buf++ = c;

        } /* while */


    if (c == EOF)
	return (FALSE);

    *buf++ = 0;
    return (TRUE);
    }
#endif


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* newarg -	Create a new member of linked list of arguments */

newarg(name)
register char *name;
{
	register arg a1, a2;

	a1 = (arg)calloc(1, sizeof(*a1));
	a1->arg_name = name;
	a1->arg_next = NULL;
    a1->lib = aflag;
	a1->bind = Bflag;
	if (arglist == NULL) arglist = a1;
	else	/* link new one on end */
	{
		for (a2 = arglist; a2->arg_next; a2 = a2->arg_next);
		a2->arg_next = a1;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* newhide -	Create a new member of linked list of hide elements */

newhide(name)
register char *name;
{
	register hide a1, a2;
	a1 = (hide)calloc(1, sizeof(*a1));
	a1->hide_name = name;
	a1->hide_next = NULL;
	if (hidelist == NULL) hidelist = a1;
	else	/* link new one on end */
	{
		for (a2 = hidelist; a2->hide_next; a2 = a2->hide_next);
		a2->hide_next = a1;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* newundef -	Create a new member of linked list of undef elements */

newundef(name)
register char *name;
{
	register undef a1, a2;
	a1 = (undef)calloc(1, sizeof(*a1));
	a1->undef_name = name;
	a1->undef_next = NULL;
	if (undeflist == NULL) undeflist = a1;
	else	/* link new one on end */
	{
		for (a2 = undeflist; a2->undef_next; a2 = a2->undef_next);
		a2->undef_next = a1;
	}
}

#ifdef	MISC_OPTS
newgeneric (name, list)
register char *name;
generic *list;
{
	register generic a1, a2;

	a1 = (generic)calloc(1,sizeof(*a1));
	a1->name = name;
	a1->next = NULL;
	if (*list == NULL)
		*list = a1;
	else
	{
		for (a2 = *list; a2->next != NULL; a2 = a2->next)
			;
		a2->next = a1;
	}
}
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* newpredef -	Create a new member of linked list of predef elements */

#pragma		BBA_IGNORE
newpredef (name)
register char *name;
{
	register predef a1, a2;

	a1 = (predef)calloc(1,sizeof(*a1));
	a1->predef_name = strtok(name,"=");
	a1->predef_value = atox(strtok(NULL," "));
	a1->predef_next = NULL;
	if (predeflist == NULL) predeflist = a1;
	else	/* link new one on end */
	{
		for (a2 = predeflist; a2->predef_next; a2 = a2->predef_next);
		a2->predef_next = a1;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* newlibdir -	Create a new member of linked list of -L dirs */

newlibdir(name)
register char *name;
{
	register undef a1, a2;
	a1 = (undef)calloc(1, sizeof(*a1));
	a1->undef_name = name;
	a1->undef_next = NULL;
	libdirsiz += strlen(name)+1 ;
	if (libdirlist == NULL) libdirlist = a1;
	else	/* link new one on end */
	{
		for (a2 = libdirlist; a2->undef_next; a2 = a2->undef_next);
		a2->undef_next = a1;
	}
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* procflags -	Process flag arguments.  */

plus_procflags(a)
char *a;
{
	register char *flagp = a+1;
	char c;
	
	while( c = *flagp++ )
#pragma		BBA_IGNORE_ALWAYS_EXECUTED
	switch( c )
	{
	  /* Place options here, something like: (see below also)
	   *  case 'E':
	   *  	plus_Eflag = 1;
	   *	break;
	   */
	  case 'r':
#pragma		BBA_IGNORE
		plus_rflag = 1;
		break;
#ifdef	VISIBILITY
	  case 'e':
		if (get_next_arg(c))
		{
			if (hide_status == HIDE_HIDES)
				fatal("can't mix -h and +e");
			newhide(next_arg);
			hide_status = EXPORT_HIDES;
		}
		break;
#endif
#ifdef	ELABORATOR
	  case 'E':
		if (get_next_arg(c))
			elaborator_name = next_arg;
		break;
	  case 'I':
		if (get_next_arg(c))
			initializer_name = next_arg;
		break;
#endif
#ifdef	HOOKS
	  case 'H':
		if (get_next_arg(c))
			hook_name = next_arg;
		break;
#endif
#ifdef SHLIB_PATH
	  case 's':
		enable_shlib_path = (embed_path == NULL) ? 1 : -1;
		break;
	  case 'b':
		if (get_next_arg(c))
			embed_path = next_arg;
		break;
#endif /* SHLIB_PATH */
	  default:
		error( e2, '+', c );
		break;
	}
}	

minus_procflags(a)
char *a;
{
	register char *flagp = a+1;
	char    *s;
	char 	c;
	static unsigned int Qflag = 0;	/* did user EXPLICITLY ask for -Q */

	while(c = *flagp++)
#pragma		BBA_IGNORE_ALWAYS_EXECUTED
	switch(c)
	{
#ifdef SHLIB_AS_OBJECT_OPTION
	  case 'g':
		gflag = 1;
		break;
#endif
#ifdef FAKE_SHLIB_AS_OBJECT
	  case 'g':
		Fflag = 10;
		break;
#endif
	  case 'o':
		if( get_next_arg(c) )
			ofilename = next_arg;
		break;
	  case 'u':
		if( get_next_arg(c) )
			newundef( next_arg );
		break;
	  case 'e':
		if( get_next_arg(c) )
			e_name = next_arg;
		break;
	  case 'h':
		if( get_next_arg(c) )
#ifdef	VISIBILITY
		{
			if (hide_status == EXPORT_HIDES)
				fatal("can't mix -h and +e");
#endif
			newhide( next_arg );
#ifdef	VISIBILITY
			hide_status = HIDE_HIDES;
		}
#endif
		break;
	  case 'R':	/* -r will override -R */
		if( get_next_arg(c) )
		{
			savernum = atox( next_arg );
			Rflag++;
		}
		break;
	  case 'V':
		if( get_next_arg(c) )
		{
			char *tmp;

			stamp = strtol( next_arg, &tmp, 10 );
			if( tmp == next_arg )
			{
				error( e17 );
				stamp = 0;
			}
			else
				Vflag++;
		}
		break;
	  case 'a':
		if( get_next_arg(c) )
		{
#ifdef	MISC_OPTS
			if (!strncmp(next_arg,"default",strlen(next_arg))) {
#else
			if (next_arg[0] == 'd') {                /* "default" */
#endif
				aflag = EITHER;
			}
#ifdef	MISC_OPTS
			else if (!strncmp(next_arg,"archive",strlen(next_arg))) {
#else
			else if (next_arg[0] == 'a') {           /* "archive" */
#endif
				aflag = ARCHIVE;
			}
#ifdef	MISC_OPTS
			else if (!strncmp(next_arg,"shared",strlen(next_arg))) {
#else
			else if (next_arg[0] == 's') {           /* "shared" */
#endif
				aflag = SHARED;
			}
			else  {
				error(e41,next_arg,c);
			}
		}
		break;

	  case 'b':
		bflag++;
		shlib_level = SHLIB_BUILD;
		aflag = ARCHIVE;
		break;

	  case 'B':
		if( get_next_arg(c) )
		{
#ifdef	MISC_OPTS
			if (!strncmp(next_arg,"immediate",strlen(next_arg))) {
#else
			if( next_arg[0] == 'i' ) {          /* "immediate" */
#endif
				Bflag = BIND_IMMEDIATE;
			}
#ifdef	MISC_OPTS
			else if (!strncmp(next_arg,"deferred",strlen(next_arg))) {
#else
			else if( next_arg[0] == 'd' ) {     /* "deferred" */
#endif
				Bflag = BIND_DEFERRED;
			}
#ifdef	BIND_FLAGS
			else if (!strncmp(next_arg,"nonfatal",strlen(next_arg)))
				Bflag |= BIND_NONFATAL;
#endif
			else {
				error( e41,next_arg,c );
			}
		}
		break;

#ifdef	MISC_OPTS
	case 'D':
		if (get_next_arg(c))
			dorigin = atox(next_arg);
		break;

	case 'c':
		if (get_next_arg(c))
			insert_args(next_arg);
		break;
	case 'y':
		if (get_next_arg(c))
			newgeneric(next_arg,&trace_list);
		break;
#endif

	  case 'i':
		iflag++;  
		shlib_level = SHLIB_DLD;
		aflag = ARCHIVE;
		break;

	  case 'E':
		Fflag++;
		break;

	  case 'l': 
		newarg(a); 
		return;
#if VERSION == 65
	  case 'm': /* keep it for one release to make transition easier */
#endif
	  case 't':
		if (vflag) break;
		tflag++; break;
	  case 'x': xflag++; break;
	  case 'r': rflag++; 
		  torigin = 0;
		  supsym_gen = 1;
		  break;
	  case 's': sflag++; break;
	  case 'd': dflag++; break;
	  case 'n':
		if (nflag < 1) error(e29, 'N', 'n', 'N'); 
		nflag = 2;
                break;
	  case 'N':
		if (nflag > 1) error(e29, 'n', 'N', 'n'); 
		if (qflag) error(e29, 'q', 'N', 'q'); 
		nflag = 0;
		qflag = 0;
                break;
	  case 'A':
		if (Aname != NULL) fatal(e29, 'A', 'A', 'A');
		else if( get_next_arg(c) )
			Aname = next_arg;
		break;
	  case 'L':
#ifdef	MISC_OPTS
		if (*flagp)
		{
			newlibdir(flagp);
			flagp += strlen(flagp);
		}
		else
#endif
		if( get_next_arg(c) )
			newlibdir( next_arg );
		break;
	  case 'X':
		if( get_next_arg(c) )
		{
		   ld_stsize = make_prime(atol( next_arg ));
		   ld_sthashsize = make_prime(ld_stsize*2);
           ld_expsize = 2 * ld_stsize;
           ld_dltsize = ld_stsize;
           ld_pltsize = ld_stsize;
		   ld_stringsize = 10 * ld_stsize;
	    }
		break;
	  case 'q': 
		if (Qflag) error(e29, 'Q', 'q', 'Q');
		Qflag = 0;
		qflag++; 
		if (!nflag) error(e29, 'N', 'q', 'N');
		break;
	  case 'Q':
		if (qflag) error(e29, 'q', 'Q', 'q'); 
		qflag = 0;
		Qflag = 1;
                break;
	  case 'v':
		/* -v option prints lots of nifty things; who needs -t? */
		vflag = 1;
		tflag = 0;
		break;
	  case 'P':
#pragma		BBA_IGNORE
		/* "-P name=value" analogous to "-D" to "cc" */
		if (get_next_arg(c))
			newpredef(next_arg);
		break;
	  default:	error(e2, '-', c);
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
long atox(s)
register char *s;
{
	register long result = 0;
	for (; *s != 0; s++)
	  if (*s >= '0' && *s <= '9') result = (result<<4) + *s-'0'; else
	  if (*s >= 'a' && *s <= 'f') result = (result<<4) + *s-'a'+10; else
	  if (*s >= 'A' && *s <= 'F') result = (result<<4) + *s-'A'+10; else
	  error(e10,*s);
	return(result);
}

/* Memory Routines.
 *  These routines are a wrapper around realloc, calloc, and malloc to insure
 *  that no bad pointers are returned.  Chkmem can also be used in these
 *  routines.
 */
#undef   realloc
#undef   calloc
#undef   malloc
char *malloc(), *calloc(), *realloc();
#if CHKMEM
#include "chkmem.h"
#endif

#pragma		BBA_IGNORE
#if CHKMEM
char *ld_realloc( a, b, f, l )
char *f;
long l;
#else
char *ld_realloc( a, b )
#endif
void *a;
long b;
{
	char *r;

#if CHKMEM
	r = chk_realloc( a, b, f, l );
#else
	r = realloc( a, b );
#endif
	if( r == NULL )	
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		fatal( e18 );

	return( r );
}

#pragma		BBA_IGNORE
#if CHKMEM
char *ld_calloc( a, b, f, l )
char *f;
long l;
#else
char *ld_calloc( a, b )
#endif
long a;
long b;
{
	char *r;

#if CHKMEM
	r = chk_calloc( a, b, f, l );
#else
	r = calloc( a, b );
#endif
	if( r == NULL ) 
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		fatal( e18 );

	return( r );
}

#pragma		BBA_IGNORE
#if CHKMEM
char *ld_malloc( a, f, l )
char *f;
long l;
#else
char *ld_malloc( a )
#endif
long a;
{
	char *r;

#if CHKMEM
	r = chk_malloc( a, f, l );
#else
	r = malloc( a );
#endif
	if( r == NULL ) 
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		fatal( e18 );

	return( r );
}
