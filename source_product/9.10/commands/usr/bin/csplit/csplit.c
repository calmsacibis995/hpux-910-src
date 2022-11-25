static char *HPUX_ID = "@(#) $Revision: 70.2.1.2 $";
/*
*	csplit - Context or line file splitter
*	Compile: cc -O -s -o csplit csplit.c
*/

 /* READ THIS about message catalog numbers:
  *    Since it is too expensive to change message catalogs (even to
  *    add messages), we satisfy ourselves with adding a new message
  *    only to the C source file.  We still use the catgets() call,
  *    in case the messages get added some day.  So, for a new message
  *    you must pick a message number that has not been used yet.
  *    Here is the list:
  *	 998 "Cannot write to temporary file %s\n"
  *	 999 "Cannot open temporary file %s\n"
  */

 /* For DSDe412184, a different algorithm was added to support input
  * from stdin.  This algorithm uses an arbitrary depth stack of temp
  * files that remembers lines from stdin.  This permits simulation
  * of the ftell/fseek as used by the algorithm that receives input
  * from files.  See the dts report for a description of the changes.
  */

#include <stdio.h>
#include <signal.h>
#include <locale.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <nl_types.h>
#include <sys/dir.h>		/* DSDe412184 (for MAXNAMLEN) */

#define LAST	0L
#define ERR	-1
#define FALSE	0
#define TRUE	1
#define EXPMODE	2
#define LINMODE	3
#define EXPSIZ	128
#define	LINSIZ	256
#define PATH_SIZE 1024

	/* Globals */

char linbuf[LINSIZ];		/* Input line buffer */
char rexlinbuf[LINSIZ];		/* Input line buffer for Regular EXpressions */
char expbuf[EXPSIZ];		/* Compiled expression buffer */
char *file;			/* File name buffer */
char *targ;			/* Arg ptr for error messages */
FILE *infile, *outfile;		/* I/O file streams */
int silent, keep, create;	/* Flags: -s(ilent), -k(eep), (create) */
long line_offset;		/* Regular expression offset value; DSDe412184*/
long curline;			/* Current line in input file */

				/* ************** DSDe412184 *****************/
FILE *tempfile;			/* I/O file stream used when input is stdin */
int temp_file_open = 0;		/* We're ready to write input lines to temp */
int input_is_stdin = 0;		/* "-" arg used: buffer input in temp  file */
char *temp_file_name;		/* Name of temp file, from tempnam(3S)	    */
struct t_file_stack {		/* Stack of files (user original + temps)   */
    FILE *input_file;		/*    that provide input.		    */
    char *file_name;
    struct t_file_stack *next;
} *file_stack;
				/* ******************************************/

int	fflag = 0;			/* 1=file name supplied */
int	maxfiles = 100;		/* maximum number of output files to create */
int	suf_size = 2;		/* number of char. to use in forming the suffix */

/* nls definitions */
#define NL_SETN 1
nl_catd		catd;

#ifndef NLS
#define catgets(cat, set, msg, s )  s
#endif NLS


/*
*	These defines are needed for regexp handling (see regexp(7))
*/
#define INIT		register char *ptr = ++instring;
#define GETC()		(*ptr++)
#define PEEKC()		(*ptr)
#define UNGETC(c)	(--ptr)
#define RETURN()	return;
#define ERROR()		fatal((catgets(catd,NL_SETN,1, "%s: Illegal Regular Expression\n")),targ);

#include <regexp.h>

/*********************************************************************
This function checks to see if the output file names will be
truncated when they are created.
*/
void
check_name( fname, suf_size )
char *fname;		/* file name */
int suf_size;		/* size of suffix to append to file name. */
{

char dirpath[ PATH_SIZE ];
if ( strrchr ( fname, '/' ) == NULL )       /* no leading dir */
    strcpy ( dirpath, "." );
else {
    strcpy ( dirpath, fname );
    *( strrchr ( dirpath, '/' ) ) = '\0';   /* remove basename */
    }

if ( strlen ( fname ) + suf_size + 1  > (int) pathconf(dirpath,_PC_NAME_MAX)){
    fputs((catgets(catd,NL_SETN,2, "Output file name too long.\n")), stderr);
    exit ( 1 );
    }
}

/**********************************************************************/
main(argc,argv)
int argc;
char **argv;
{
	FILE *getfile();
	int ch, mode, sig(), i;
	char *getline();
	long findline();

	/* the following are used by getopt(3) */
	extern char *optarg;
	extern int optind, opterr;
	char *optstring = "f:skn:";
	
	char *fsave;	/* save the filename prefix from the -f argument */

#	ifdef NLS
		if (!setlocale(LC_ALL,"")) {
			fputs(_errlocale("csplit"), stderr);
			putenv("LANG=");
			catd = (nl_catd)-1;
			}
		else
			catd = catopen("csplit", 0);
#	endif NLS

	/* Option handling */
	opterr = 0;		/* local reporting of getopt errors */
	while ((ch = getopt(argc, argv, optstring)) != EOF) {
		switch(ch) {
		case 'f':
			fflag++;
			fsave = optarg;
			break;
		case 'n':	
			suf_size = atoi ( optarg );
			if ( suf_size < 1 || suf_size > 7 ) {
				fprintf(stderr, (catgets(catd,NL_SETN,3, "suffix size not valid\n")));
				exit ( 1 );
				}
			for (i = 2; i < suf_size; i++)
				maxfiles *= 10;
			break;

		case 's':
			silent++;
			break;
		case 'k':
			keep++;
			break;
		default:
			fatal((catgets(catd,NL_SETN,4, "illegal option string: %s\n")),*argv);
		}
	}
	for(++argv, optind--; optind; optind--) {
		--argc;
		++argv;
	}
	if(argc <= 2)
		fatal((catgets(catd,NL_SETN,5, "Usage: csplit [-s] [-k] [-f prefix] [ -n number ] file args ...\n")),NULL);
	if(strcmp(*argv,"-") == 0) {
		infile = stdin;
		input_is_stdin = 1;			/* DSDe412184 */
		/* Initialize the stack of file pointers.
		 * The base block contains stdin's pointer.  Temp files
		 * nest on top of that.
		 */
		file_stack = malloc(sizeof(struct t_file_stack));
		file_stack->input_file = stdin;
		file_stack->file_name = NULL;
		file_stack->next = NULL;
	}
	else
		if((infile = fopen(*argv,"r")) == NULL)
			fatal((catgets(catd,NL_SETN,6, "Cannot open %s\n")),*argv);

	--argc; ++argv;
	curline = 1L;
	signal(SIGINT,sig);

	if ( ! fflag ) {	/* use the default output file name name of xx */
	    if ((file = malloc( 3 + suf_size)) == NULL) {  
			fprintf(stderr, (catgets(catd,NL_SETN,7, "Malloc failed\n")));
			exit(1);
	    	}
	    	strcpy(file, "xx");
		}
	else {		/* use the user supplied name pointed to by fsave */
		if ( (file = malloc ( strlen ( fsave ) + 1 + suf_size ) ) == NULL ) {
			fprintf(stderr, (catgets(catd,NL_SETN,7, "Malloc failed\n")));
			exit(1);
	    	}
		strcpy ( file, fsave );
		}

	check_name ( file, suf_size );
			

	/*
	*	The following for loop handles the different argument types.
	*	A switch is performed on the first character of the argument
	*	and each case calls the appropriate argument handling routine.
	*/

	for(; *argv; ++argv) {
		targ = *argv;
		switch(**argv) {
		case '/':
			mode = EXPMODE;
			create = TRUE;
			re_arg(*argv);
			break;
		case '%':
			mode = EXPMODE;
			create = FALSE;
			re_arg(*argv);
			break;
		case '{':
			num_arg(*argv,mode);
			mode = FALSE;
			break;
		default:
			mode = LINMODE;
			create = TRUE;
			line_arg(*argv);
			break;
		}
	}
	create = TRUE;
	to_line(LAST);
}

/****************************************************************************
*	Atol takes an ascii argument (str) and converts it to a long (plc)
*	It returns ERR if an illegal character.  The reason that atol
*	does not return an answer (long) is that any value for the long
*	is legal, and this version of atol detects error strings.
*/

atol(str,plc)
register char *str;
long *plc;
{
	register int f;
	*plc = 0;
	f = 0;
	for(;;str++) {
		switch(*str) {
		case ' ':
		case '\t':
			continue;
		case '-':
			f++;
		case '+':
			str++;
		}
		break;
	}
	for(; *str != NULL; str++)
		if(*str >= '0' && *str <= '9')
			*plc = *plc * 10 + *str - '0';
		else
			return(ERR);
	if(f)
		*plc = -(*plc);
	return(TRUE);	/* not error */
}

/*
*	Closefile prints the byte count of the file created, (via fseek
*	and ftell), if the create flag is on and the silent flag is not on.
*	If the create flag is on closefile then closes the file (fclose).
*/

closefile()
{
	long ftell();

	if(!silent && create) {
		fseek(outfile,0L,2);
		fprintf(stdout,"%ld\n",ftell(outfile));
	}
	if(create)
		fclose(outfile);
}

/*
*	Fatal handles error messages and cleanup.
*	Because "arg" can be the global file, and the cleanup processing
*	uses the global file, the error message is printed first.  If the
*	"keep" flag is not set, fatal unlinks all created files.  If the
*	"keep" flag is set, fatal closes the current file (if there is one).
*	Fatal exits with a value of 1.
*/

fatal(string,arg)
char *string, *arg;
{
	register char *fls;
	register int num;

	fprintf(stderr,string,arg);
	if(!keep) {
		if(outfile) {
			fclose(outfile);
			for(fls=file; *fls != NULL; fls++);
			fls -= 2;
			for(num=atoi(fls); num >= 0; num--) {
				sprintf(fls,"%.02d",num);
				unlink(file);
			}
		}
	} else
		if(outfile)
			closefile();
	exit(1);
}

/****************************************************************************
	Findline returns the line number referenced by the
	current argument.  Its arguments are a pointer to the
	compiled regular expression (expr), and an offset (oset).
	The variable lncnt is used to count the number of lines
	searched.  
	
	If the input is not from stdin:
	   First the current stream location is saved via
	   ftell(), and getline is called so that R.E.  searching
	   starts at the line after the previously referenced
	   line.  The while loop checks that there are more lines
	   (error if none), bumps the line count, and checks for
	   the R.E.  on each line.  If the R.E.  matches on one
	   of the lines the old stream location is restored, and
	   the line number referenced by the R.E.  and the offset
	   is returned.

	The stdin case is similar except that temp files are used
	   instead of ftell/seek.  This routine opens the output
	   temp file.  As we pass stdin lines getline writes them
	   to the current output temp file.  After we are done
	   scanning for the sought-after line, we close the temp
	   file, add it to the stack of temp files that contain
	   input lines, and reopen it for reading.  We read range
	   of lines that needs to be written to the current ouput
	   file from these temp files.

*/

long findline(expr,oset)
register char *expr;
long oset;
{
	static int benhere;
	long lncnt = 0, saveloc, ftell();

	if (! input_is_stdin)			/* DSDe412184 */
	   saveloc = ftell(infile);
	else {
	   struct t_file_stack *fsp;
	   /*
	    * Open a new temp file for writing to.
	    */
	   temp_file_name = tempnam( getenv("TMPDIR"), "csplt" );
	   if ((tempfile = fopen( temp_file_name, "w" )) == NULL)
	      fatal((catgets(catd,NL_SETN,999, 
		 "Cannot open temporary file %s\n")),temp_file_name);

	}
	/* When starting a search, we don't search the first line that
	 * would be included in the set.  That way, you can search for
	 * successive lines with "foo" (e.g. /foo/ {13}) and not have
	 * it keep finding "foo" on the same line and producing a bunch
	 * of empty files.
	 */
	if(curline != 1L || benhere)		/* If first line, first time, */
		getline(FALSE);			/* then don't skip */
	else
		lncnt--;
	benhere = 1;
	while(getline(FALSE) != NULL) {
		lncnt++;
		if(step(rexlinbuf,expr)) {
			if (! input_is_stdin)	/* DSDe412184 */
			   fseek(infile,saveloc,0L);
			return(curline+lncnt+oset);
		}
	}
	if (! input_is_stdin)			/* DSDe412184 */
	   fseek(infile,saveloc,0L);
	return(curline+lncnt+oset+2);
}

/*
*	Flush uses fputs to put lines on the output file stream (outfile)
*	Since fputs does its own buffering, flush doesn't need to.
*	Flush does nothing if the create flag is not set.
*/

flush()
{
	if(create)
		fputs(linbuf,outfile);
}

/****************************************************************************
*	Getfile does nothing if the create flag is not set.  If the
*	create flag is set, getfile positions the file pointer (fptr) at
*	the end of the file name prefix on the first call (fptr=0).
*	Next the file counter (ctr) is tested for the maximum number of 
*	output files, fatal if too
*	many file creations are attempted.  Then the file counter is
*	stored in the file name and incremented.  If the subsequent
*	fopen fails, the file name is copied to tfile for the error
*	message, the previous file name is restored for cleanup, and
*	fatal is called.  If the fopen succecedes, the stream (opfil)
*	is returned.
*/

FILE *getfile()
{
	static char *fptr;
	static int ctr;
	FILE *opfil;

	if(create) {
		if(fptr == 0)
			for(fptr = file; *fptr != NULL; fptr++);

		if(ctr >= maxfiles)
			fatal((catgets(catd,NL_SETN,8, "reached limit on number of output files at arg %s\n")),targ);

		sprintf(fptr,"%.0*d", suf_size, ctr++);

		if((opfil = fopen(file,"w")) == NULL) {
			fprintf(stderr,(catgets(catd,NL_SETN,9, "Cannot create %s\n")), file);
			sprintf(fptr,"%.0*d", suf_size, (ctr-2));
			fatal("",0); /* already printed message, just clean up */
		}
		return(opfil);
	}
	return(NULL);
}

/****************************************************************************
*	Getline gets a line via fgets from the input stream "infile".
*	If the input comes from a file named on the command line, then
*	the input always comes from that file.  If the input comes from
*	stdin then it comes from that file or from one of a stack of
*	temp files that are used to remember the lines we've read
*	from stdin.
*	The line is put into linbuf and may not be larger than LINSIZ.
*	If getline is called with a non-zero value, the current line
*	is bumped, otherwise it is not (for R.E. searching).
*	If input is from stdin and getline is called with a non-zero 
*	value the we're doing a "real" read of the virtual input file
*	and we simply copy the next input line to the output file.
*	If input is from stdin and getline is called with a zero 
*	value then we're just scanning for the next pattern/line, so
*	we also copy the line to the output temp file so we can reread
*	it later when we're ready to do the "real" reads.
*	
*/

char *fix_EOL(ret)
char *ret;
{
	short i;
	/* fgets returns the line feed '\012' which causes regular 
 	 * expression evaluations to faile. They expect the line buffer 
	 * to end with a null. This little bit of code finds a line feed 
	 * or carriage return which precedes a null terminator. It then 
	 * changes the line feed or carriage return to a null.
	 *
	 * DSDe601483 - "csplit(1) doesn't recognize $ in regular expressions."
	 *
	 */
	for(i=0; ret[i] != NULL; i++) {		/* move the end of the line */

		/* as we move through the line buffer copy the standard 
		 * linbuf contents into the regular expres line buffer.
	 	 */
		rexlinbuf[i] = ret[i];

		/* char == linefeed/carriage return and next char is a null?
		 */
		if(((ret[i] == '\012') || (ret[i] == '\015')) && 
		    (ret[i+1] == '\0')) {
			rexlinbuf[i] = '\0';	/* change LF or CR to null */
			break;
		}
	}	
	return(ret);
}


char *getline(bumpcur)
int bumpcur;
{
	char 	*ret;
	struct t_file_stack *fsp;
	int looking_for_rec = 1;

	if(bumpcur)
		curline++;

	/* Read the next input line, possibly from a temp file.
	 * In the input_is_stdin case:
	 *    Since we may hit EOF on an arbitrary number of temp files,
	 *    we need to loop until we've read a record or hit EOF on
	 *    the original file.  DSDe412184
	 */
	while (looking_for_rec) {
	   ret=fgets(linbuf,LINSIZ,infile);
	   if (! input_is_stdin)
	      return( fix_EOL(ret) );
	   else {					/* DSDe412184 */
	      if (ret == NULL) {
		 /* EOF.  If it was a temp file then close it and move
		  * to the next temp/original file.
		  */
		 if (file_stack->input_file == stdin)
		    /* EOF in user's original file -- *really* EOF. */
		    return(ret);
		 else {
		    char rm_command[MAXNAMLEN+4];
		    /* Go to the next temp file. */
		    fsp = file_stack;
		    file_stack = file_stack->next;
		    fclose(infile);
		    /* We must remove, rather than unlink.  The premise
		     * for making this change is that the input is too
		     * large to keep a copy on disk.
		     */
		    unlink(fsp->file_name);
		    free(fsp->file_name);
		    free(fsp);
		    infile = file_stack->input_file;
		 }
	      }
	      else {
		 /* Not EOF.  Now write the line read out to the the temp 
		  * file, iff we're in the findline() scanning phase 
		  * (! bumpcur). If we're in the to_line() copying phase then 
		  * we just want to read lines from whatever input file we're 
		  * using and * write them to the user's xx?? file and not 
		  * write them to any more temp files.
		  */
		 if (! bumpcur)
		    if (fputs(linbuf,tempfile) == EOF)
	               fatal((catgets(catd,NL_SETN,998, 
		          "Cannot write to temporary file %s\n")),
			  temp_file_name);
		       ;
		 looking_for_rec = 0;
	      }
	   }
	}

	return( fix_EOL(ret) );
}
	

/****************************************************************************
*	Line_arg handles line number arguments.
*	line_arg takes as its argument a pointer to a character string
*	(assumed to be a line number).  If that character string can be
*	converted to a number (long), to_line is called with that number,
*	otherwise error.
*/

line_arg(line)
char *line;
{
	long to;

	if(atol(line,&to) == ERR)
		fatal((catgets(catd,NL_SETN,10, "%s: bad line number\n")),line);
	to_line(to);
}

/*
*	Num_arg handles repeat arguments.
*	Num_arg copies the numeric argument to "rep" (error if number is
*	larger than 11 characters or } is left off).  Num_arg then converts
*	the number and checks for validity.  Next num_arg checks the mode
*	of the previous argument, and applys the argument the correct number
*	of times. If the mode is not set properly its an error.
*/

num_arg(arg,md)
register char *arg;
int md;
{
	long repeat, toline;
	char rep[12];
	register char *ptr;

	ptr = rep;
	for(++arg; *arg != '}'; arg++) {
		if(ptr == &rep[11])
			fatal((catgets(catd,NL_SETN,11, "%s: Repeat count too large\n")),targ);
		if(*arg == NULL)
			fatal((catgets(catd,NL_SETN,12, "%s: missing '}'\n")),targ);
		*ptr++ = *arg;
	}
	*ptr = NULL;
	if((atol(rep,&repeat) == ERR) || repeat < 0L)
		fatal((catgets(catd,NL_SETN,13, "Illegal repeat count: %s\n")),targ);
	if(md == LINMODE) {
		toline = line_offset = curline;
		for(;repeat > 0L; repeat--) {
			toline += line_offset;
			to_line(toline);
		}
	} else	if(md == EXPMODE)
			for(;repeat > 0L; repeat--)
				to_line(findline(expbuf,line_offset));
		else
			fatal((catgets(catd,NL_SETN,14, "No operation for %s\n")),targ);
}


/****************************************************************************
*	Re_arg handles regular expression arguments.
*	Re_arg takes a csplit regular expression argument.  It checks for
*	delimiter balance, computes any offset, and compiles the regular
*	expression.  Findline is called with the compiled expression and
*	offset, and returns the corresponding line number, which is used
*	as input to the to_line function.
*/

re_arg(string)
char *string;
{
	register char *ptr;
	register char ch;

	ch = *string;
	ptr = string;
	while(*(++ptr) != ch) {
		if(*ptr == '\\')
			++ptr;
		if(*ptr == NULL)
			fatal((catgets(catd,NL_SETN,15, "%s: missing delimiter\n")),targ);
	}
	if(atol(++ptr,&line_offset) == ERR)
		fatal((catgets(catd,NL_SETN,16, "%s: illegal offset\n")),string);
	compile(string, expbuf, &expbuf[EXPSIZ], ch);
	to_line(findline(expbuf,line_offset));
}

/*
*	Sig handles breaks.  When a break occurs the signal is reset,
*	and fatal is called to clean up and print the argument which
*	was being processed at the time the interrupt occured.
*/

sig()
{
	signal(SIGINT,sig);
	fatal((catgets(catd,NL_SETN,17, "Interrupt - program aborted at arg '%s'\n")),targ);
}

/****************************************************************************
*	To_line creates split files.
*	To_line gets as its argument the line which the current argument
*	referenced.  To_line calls getfile for a new output stream, which
*	does nothing if create is False.  If to_line's argument is not LAST
*	it checks that the current line is not greater than its argument.
*	While the current line is less than the desired line to_line gets
*	lines and flushes (error if EOF is reached).
*	If to_line's argument is LAST, it checks for more lines, and gets
*	and flushes lines till the end of file.
*	If input is from stdin then we close out the current output temp
*	file that was created by findline() and add it to the top of
*	input temp file stack.  That will be the first file getline()
*	reads records from.
*	Finally, to_line calls closefile to close the output stream.
*/

to_line(ln)
long ln;
{
	struct t_file_stack *fsp;
	/* Switch to getting input from temp file, if appropriate.  DSDe412184
	 */
	if (input_is_stdin) {
	   /*
	    * Convert the current write-to temp file (if any) into 
	    * one used for reading.  There would be one if we just
	    * did a regular expression search.  There wouldn't be 
	    * one if we're doing a line line_number (man page term) 
	    * search.  Repeat arguments are an inconsequential 
	    * embelishment on r.e. and line_number searches.
	    */
	   if (tempfile != NULL) {
	      fclose(tempfile);
	      tempfile = fopen(temp_file_name, "r");
	      /*
	       * Push a file stack node for this temp file.
	       * Make it the current input file.
	       */
	      fsp  = malloc(sizeof(struct t_file_stack));
	      fsp->next  	     = file_stack;
	      file_stack 	     = fsp;
	      file_stack->input_file = tempfile;
	      file_stack->file_name  = temp_file_name;
	      infile 		     = tempfile;
	      tempfile 		     = NULL;
	   }
	}

	outfile = getfile();
	if(ln != LAST) {
		if(curline > ln)
			fatal((catgets(catd,NL_SETN,18, "%s - out of range\n")),targ);
		while(curline < ln) {
			if(getline(TRUE) == NULL)
				fatal((catgets(catd,NL_SETN,18, "%s - out of range\n")),targ);
			flush();
		}
	} else		/* last file */
		if(getline(TRUE) != NULL) {
			flush();
			while(TRUE) {
				if(getline(TRUE) == NULL)
					break;
				flush();
			}
		} else
			fatal((catgets(catd,NL_SETN,18, "%s - out of range\n")),targ);
	closefile();
}
