/* @(#) $Revision: 70.1 $ */      

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include "symbols.h"
# include "adrmode.h"
# include "sdopt.h"

#ifdef YYDEBUG
long yy_rtable[];
#endif

/*******************************************************************************
 * pass1.c
 *	aspass1() is the driver for the first pass of the assembler.
 *	The following things are done by this function (or by subfunctions
 *	called from aspass1):
 *
 *	1. Open files required by pass1:  the input file, and the
 *	   temporary files for text and data.
 *
 *	2. Scanner and Symbol Table Initialization. 
 *		- Setup signal handling to catch interrupt signals.
 *		- Initilization of scanner tables (lxinit).
 *	   	- Initialization of the symbol table with mnemonics for
 *	   	  instructions and pseudos-ops.
 *
 *	3. Definition of predefined usernames.
 *		- Initialization of the predefined username '.' which
 *		  refers to the location counter.
 *		- Initialization of the predefined names .text, .data, .bss.
 *		  These names are of type STEXT, SDATA, SBSS, respectively,
 *		  and each with value 0L.  They can be referenced as labels to
 *		  the start of each section.  In a COFF enviornment, 
 *		  these names are used for relocation records, but our
 *		  current a.out definition uses segment relative relocation.
 *
 *	4. "yyparse" is called to do the actual first pass processing.
 *	   This is followed by a call to "cgsect". Normally this
 *	   function is used to change the section into which code
 *	   is generated. In this case, it is only called to make
 *	   sure that "dottxt", "dotdat", and "dotbss" contain the
 *	   proper values for the program counters for the respective
 *	   sections.
 *
 *	5. The temporary files are flushed and the input files is
 *	   is closed.
 *
 */

extern char *file;

extern char *filenames[];

extern unsigned int  line;


#if DEBUG
extern unsigned numcalls;
extern unsigned numids;
extern unsigned numcoll;
#endif

extern short nerrors;

extern int aerror();
extern int onintr();
extern int fpe_catch();

FILE *fderr;

extern usymins *lookup();

extern long dottxt;
extern long dotdat;
extern long dotbss;
extern long dotgntt;
extern long dotlntt;
extern long dotslt;
extern long dotvt;
extern long dotxt;

extern short rflag;

long newdot;
symbol *dot;

FILE	*fdin;
FILE	*fdtext;
FILE	*fddata;
FILE	*fdgntt;
FILE	*fdlntt;
FILE	*fddntt;	/* for the dntt_include pseudo */
FILE	*fdslt;
FILE	*fdvt = NULL;
FILE	*fdxt;
FILE	*fdtextfixup;
FILE	*fddatafixup;
FILE	*fdmod;
FILE	*fdcsect;	/* FILE descriptor for current segment */
FILE	*fdcsectfixup;	/* FILE descriptor for current segment fixup info */
FILE	*fdlistinfo;	/* info used to generate a listing after code gen */
#if DEBUG
FILE	*perfile;	/* performance data file descriptor */
#endif

extern int gfiles_open;
extern short listflag;

#if DEBUG
/*
 *	Performance data structure
 */
	long	ttime;
	struct	tbuffer {
		long	proc_user_time;
		long	proc_system_time;
		long	child_user_time;
		long	child_system_time;
		} ptimes;
	extern	long	times();

#endif

/*******************************************************************************
 * aspass1
 *	Main driver for pass1 of the assembler.
 */

aspass1()
{
	register short i;
	
	setup_signal_handling();
	
	fderr = stderr;

#if DEBUG
/*	Performance data collected	*/
	ttime = times(&ptimes);
#endif

	/* open the input file */
	if ( strcmp(file,"-")==0 ) fdin = stdin;
	else if ((fdin = fopen(file, "r")) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("Unable to open input file");

	open_intermediate_files();

	line = 1;
	lxinit();
	symtables_init();
	initialize_dot_symbols();
	cgsect(STEXT);

# ifdef DRAGON
	dragon_setup();
# endif

#ifdef YYDEBUG
	yy_init_rtable("/users/xdb/sjl/as/gram/gram.data", yy_rtable);
#endif

	yyparse();	/* pass 1 */

#ifdef YYDEBUG
	yy_update_rfile("/users/xdb/sjl/as/gram/gram.data", yy_rtable);
#endif
	
	/* this fclose moved to pass0.c, so lister can reread file without
	 * reopening. */
	/*fclose(fdin);*/

	cgsect(STEXT);
	
	flush_intermediate_files();

	cgsect(STEXT); /* update "dottxt" */



#if DEBUG
	if (tstlookup) {
		printf("Number of calls to lookup: %u\n",numcalls);
		printf("Number of identifiers: %u\n",numids);
		printf("Number of identifier collisions: %u\n",numcoll);
		fflush(stdout);
	}
/*
 *	Performance data collected and written out here
 */

	ttime = times(&ptimes) - ttime;
	if ((perfile = fopen("as.info", "r")) != NULL ) {
		fclose(perfile);
		if ((perfile = fopen("as.info", "a")) != NULL ) {
			fprintf(perfile,
			   "as1\t%07ld\t%07ld\t%07ld\t%07ld\t%07ld\tpass 1\n",
			    ttime, ptimes);
			fclose(perfile);
		}
	}

#endif

}

/*****************************************************************************
 * setup_signal_handling
 *	Call "signal" to catch interrupt singlas for hang-up, break,
 *	terminate, and floating point exception.
 */

setup_signal_handling()
{
  if (signal(SIGHUP,SIG_IGN) == SIG_DFL)
	signal(SIGHUP,onintr);
  if (signal(SIGINT,SIG_IGN) == SIG_DFL)
	signal(SIGINT,onintr);
  if (signal(SIGTERM,SIG_IGN) == SIG_DFL)
	signal(SIGTERM,onintr);
  if (signal(SIGFPE,SIG_IGN) == SIG_DFL)
	signal(SIGFPE, fpe_catch);
}


/*******************************************************************************
 * open_intermediate_files()
 *	Open the intermediate files for text (fdtext) and data (fddata).
 *	The intermediate files required for cdb-support (fdgntt, fdlntt, fdslt, 
 *	fdvt, fdxt) are NOT opened here.  The cdb-support routines (debug.c)
 *	will open these IF cdb pseudo-ops are seen.
 */

open_intermediate_files()
{
  if ((fdtext = fopen(filenames[2], "w+")) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("Unable to open temporary (text) file");
  if ((fddata = fopen(filenames[3], "w+")) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("Unable to open temporary (data) file");
  if ((fdtextfixup = fopen(filenames[7], "w+")) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("Unable to open temporary (text fixup) file");
  if ((fddatafixup = fopen(filenames[8], "w+")) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("Unable to open temporary (data fixup) file");
# ifdef LISTER2
  if (listflag)
	if ((fdlistinfo = fopen(filenames[9], "w+")) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	   aerror("Unable to open temporary (lister info) file");
# endif
  if ((fdmod = fopen(filenames[11], "w+")) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("Unable to open temporary (module info) file");

  /* unlink the files so they automatically go away if the assembler
   * dies or is killed.
   */
  unlink(filenames[2]);
  unlink(filenames[3]);
  unlink(filenames[7]);
  unlink(filenames[8]);
  unlink(filenames[11]);
# if defined(LISTER2) && !defined(LISTER_DEBUG)
  if (listflag) unlink(filenames[9]);
# endif
	
  /* For more efficient writing, use a larger buffer for the fdtext
   * intermediate file.
   */
  setvbuf(fdtext, 0, _IOFBF, 8192);
  setvbuf(fddata, 0, _IOFBF, 4096);
  setvbuf(fdtextfixup, 0, _IOFBF, 8192);
  setvbuf(fddatafixup, 0, _IOFBF, 1024);
  setvbuf(fdin, 0, _IOFBF, 8192);
# ifdef LISTER2
  if (listflag) setvbuf(fdlistinfo, 0, _IOFBF, 4096);
# endif
}


/*****************************************************************************
 * flush_intermediate_files
 *	Fflush the buffers for all the intermediate files.
 */

flush_intermediate_files()
{
  fflush(fdtext);
  if (ferror(fdtext))
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("trouble writing; probably out of temp-file space");
  
  fflush(fddata);
  if (ferror(fddata))
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("trouble writing; probably out of temp-file space");

  fflush(fdtextfixup);
  if (ferror(fdtextfixup))
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("trouble writing; probably out of temp-file space");
  
  fflush(fddatafixup);
  if (ferror(fddatafixup))
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("trouble writing; probably out of temp-file space");

  if (gfiles_open) {
	fflush(fdgntt);
	if (ferror(fdgntt))
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	   aerror("trouble writing gntt; probably out of temp-file space");

	fflush(fdlntt);
	if (ferror(fdlntt))
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	   aerror("trouble writing lntt; probably out of temp-file space");

	fflush(fdslt);
	if (ferror(fdslt))
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	   aerror("trouble writing slt; probably out of temp-file space");

	if (fdvt)
	{
	    fflush(fdvt);
	    if (ferror(fdvt))
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("trouble writing vt; probably out of temp-file space");
        }
	
	fflush(fdxt);
	if (ferror(fdxt))
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	   aerror("trouble writing xt; probably out of temp-file space");
	}
#ifdef LISTER2
  if (listflag)
	fflush(fdlistinfo);
#endif
	fflush(fdmod);
	if (ferror(fdmod))
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	   aerror("trouble writing mod info; probably out of temp-file space");
}


/******************************************************************************* * intitialize_dot_symbols
 *	Add the '.' (dot) symbol to the symbol table and initialize it.
 *	Also add .text, .data, .bss as symbols that reference the start
 *	of each of these sections.
 *	We don't need these (in COFF all relocation is done as symbol
 *	relative, so relocations relative to .text, .data and .bss replace
 *	segment relative relocations).  However, the presence of these
 *	symbols do give a user feature:  the ability to reference things
 *	like 
 *		.data+10
 *	so I'm defining these names for compatibilty.
 *	This routine is called before parsing the user program, to avoid
 *	confusion with a user attempt to use these names. (The VAX
 *	assembler made the call after the parse, which would silently
 *	redefine the names.
 *	COFF assemblers also define (text), etc. as the end of each
 *	section, but these names are not user addressable and so are
 *	not duplicated here.
 */

initialize_dot_symbols()
{
  register symbol * ptr;

  /* initialize the '.' (dot) symbol */
  dot = (symbol *) lookup(".",INSTALL, USRNAME);
  dot->stype = STEXT;
  dot->svalue = newdot = 0L;
# ifdef SDOPT
  if (sdopt_flag)
	dot->ssdiinfo =  makesdi_labelnode(dot);
# endif

  /* start of section symbols */
  ptr = (symbol *) lookup(".text", INSTALL, USRNAME);
  ptr->stype = STEXT;
  ptr->svalue = 0L;
  ptr = (symbol *) lookup(".data", INSTALL, USRNAME);
  ptr->stype = SDATA;
  ptr->svalue = 0L;
  ptr = (symbol *) lookup(".bss", INSTALL, USRNAME);
  ptr->stype = SBSS;
  ptr->svalue = 0L;


}
