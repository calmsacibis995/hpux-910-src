static char *HPUX_ID = "@(#) $Revision: 64.2 $";

/*
**	1.1 Data Order Conversion
**
**	The forder command converts the order of characters in a
**	file from screen to keyboard or vice versa.  If the file to
**	be converted is a Latin mode file, then all non-Latin
**	strings are reversed.  Similarly, if the file to be
**	converted is a non-Latin mode file, then all Latin strings
**	are reversed.  Hindi numbers always have a left-to-right
**	orientation.
**
**	1.2 Latin and non-Latin Strings
**
**	Latin and non-Latin strings are identified by the most
**	significant bit of their characters.  The most significant
**	bit is off for a Latin character and on for a non-Latin
**	character.
**
**	Space characters are part of (and do not delimit) Latin and
**	non-Latin strings.  Space characters include Latin spaces
**	(0x20) and non-Latin spaces (0xa0).
**
**	Regular Ascii control codes (0x00 - 0x1f) may terminate
**	Latin and non-Latin strings.  Latin strings are delimited by
**	non-Latin characters or Ascii control characters.  Non-Latin
**	strings are delimited Latin characters or Ascii control
**	characters.
**
**	1.3 Data Conversion Example
**
**	Assume the following code:
**
**	1. Ai : Arabic letters.
**	2. Hi : Hindi digits.
**	3. Li : Latin letters.
**	4. Di : Latin digits.
**	5. t : tab (0x09).
**	6. b : Latin space (0x20).
**	7. B : Non-Latin space (0xa0).
**
**	The following examples summarize the data conversion:
**
**	1. Latin Mode: Phonetic to Screen
**
**	File Contents:
**
**	A1 A2 A3 B B H1 H2 H3 t A4 A5 A6 t t L1 L2 L3 t L4 L5 L6 b D1 D2 D3
**
**	Result of Conversion:
**
**	H1 H2 H3 B B A3 A2 A1 t A6 A5 A4 t t L1 L2 L3 t L4 L5 L6 b D1 D2 D3
**
**	Screen to Phonetic conversion results in the opposite mapping.
**
**	2. Non-Latin Mode: Phonetic to Screen
**
**	File Contents:
**
**	A1 A2 A3 B B H1 H2 H3 t A4 A5 A6 L1 L2 L3 t t L4 L5 L6 b D1 D2 D3 t
**
**	Result of Conversion:
**
**	A1 A2 A3 B B H3 H2 H1 t A4 A5 A6 L3 L2 L1 t t D3 D2 D1 b L6 L5 L4 t
**
**	Screen to Phonetic conversion results in the opposite mapping.
**
**	1.4 Mode Information
**
**	The command converts data based on mode information
**	contained in the LANGOPTS environment variable
**	[mode][_order][.outdigit].  If the mode part of LANGOPTS is
**	set to l, then the file is assumed to have been created in
**	Latin mode.  If the mode part of LANGOPTS is set to n or is
**	undefined, then the file is assumed to have been created in
**	non-Latin mode.  Mode information is set up using nl_init(3c).
**
**	1.5 Command Line Options
**
**	Mode information can be overridden from the command line.
**
**	1. -l: Assume the file has been created in Latin mode.
**	2. -n: Assume the file has been created in non-Latin mode.
**	3. -a: Do the data order conversion no matter what the language.
**	   By default, only right-to-left languages are converted.
**	4. Forder reads the concatenation of input files
**	   (or standard input if none are given) and produces
**	   on standard output the converted version of its input.
**	   If "-" appears as an input file name, forder reads
**	   standard input at that point.  You can use "--" to
**	   delimit the end of options.
*/

#include <stdio.h>
#include <nl_types.h>
#include <nl_ctype.h>
#include <locale.h>

/* constants */

#define SUCCESSFUL 	0
#define UNSUCCESSFUL 	1
#define NL_SETN 	1

/* some messy error messages put in macros so they don't clutter up the code */

#define USAGE		{ (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,1, "usage: %s [-l | -n] [-a] [files ... ]\n")),myname);\
			(void) catclose(nlmsg_fd);\
			return UNSUCCESSFUL; }

#define BAD_INPUT	{ (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,2, "%s: can't open input file \"%s\" \n")), myname,*argv); }

#define NOT_RTL		(void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,3,"%s: \"%s\" not a right-to-left language\n")), myname,langname)

main(argc, argv)
int argc;
char *argv[];
{	
	extern char *getenv();		/* get environment variables */
	extern char *strord();		/* the data conversion routine */
	extern nl_catd catopen();	/* open message catalog */
	extern int catclose();		/* close message catalog */
	extern char *catgets();		/* get message */
	extern char *get_basename();	/* get basename of command name */
	extern nl_mode _nl_mode;	/* mode info from nl_init(3c) */
	extern nl_direct _nl_direct;	/* direction info from nl_init(3c) */
	extern int optind;		/* argv index of next argument */
	extern int opterr;		/* getopt() error flag */

	char oldstr[BUFSIZ];		/* buffer to store input string */
	char newstr[BUFSIZ];		/* buffer to store output string */  
	register char *pold = oldstr;	/* reg ptr to input string for speed */
	register char *pnew = newstr;	/* reg ptr to output string for speed */
	register nl_mode mode;		/* mode of file */
	register nl_catd nlmsg_fd;	/* message catalogue file descriptor */
	register FILE *fpin;		/* input file pointer */
	register char *myname;		/* pointer to command name */
	register char *langname;	/* pointer to language name */
	register int notlatmode = 0;	/* -n option for non-Latin */
	register int latmode = 0;	/* -l option for Latin mode */
	register int all_lang = 0;	/* -a option for all languages */
	register int optchar;		/* option character */

	static char *defargs[] = { "-", (char*)NULL };	/* default arguments */

	register int value = SUCCESSFUL;	/* assume a sucessful return */

	/* get the program base name */
	myname = get_basename( *argv);

	/* initialize things including mode information */
	if (! setlocale( LC_ALL, "")) {
		/* bad initialization */
		(void) fputs( _errlocale(), stderr);
		nlmsg_fd = (nl_catd) -1;
		putenv( "LANG=");       /* for perror */
	}
	else {
		/* good initialization: open message catalog,
		   ... keep on going if it isn't there */
		nlmsg_fd = catopen( myname, 0);
	}

	/* get language name (used in error messages) */
	if(*(langname = getenv("LANG")) == '\0') {
		langname = "C";
	}

	mode = _nl_mode;

	/* parse command line options */
	opterr = 0;		/* disable getopt error message */
	while ((optchar = getopt(argc,argv,"aln")) != EOF) {
		switch (optchar) {
		case 'l':	/* latin mode */
			latmode++;
			mode = NL_LATIN;
			break;
		case 'n':	/* non-latin mode */
			notlatmode++;
			mode = NL_NONLATIN;
			break;
		case 'a':	/* convert no matter what the language */
			all_lang++;
			break;
		case '?':	/* unrecognized option */
			USAGE;
		}
	}

	/* can only have one mode option */
	if (notlatmode + latmode > 1) USAGE;

	/* set up input file arguments */
	argc -= optind; argv += optind;
	if (argc < 1) argv = defargs;
	
	/* process input files one at a time */
	for ( ; *argv ; argv++) {

		if (!strcmp(*argv,"-")) {	/* open input file */
			fpin = stdin;
		} else if (!(fpin = fopen(*argv,"r"))) {
			BAD_INPUT;
			value = UNSUCCESSFUL;	/* ... get next file */
			continue;		/* ... if can't open */
		}

		/* do the actual work: get a string from input file,  */
		/* ... run in thru strord(3c) and print it on stdout. */

		if (_nl_direct == NL_RTL) {	/* have rtl language */
			while ((fgets(pold,BUFSIZ,fpin)) != NULL)
				(void) fputs(strord(pnew,pold,mode),stdout);
		} else if (all_lang) {		/* no rtl but convert anyway */
			NOT_RTL;
			while ((fgets(pold,BUFSIZ,fpin)) != NULL)
				(void) fputs(strord(pnew,pold,mode),stdout);
		} else {			/* no rtl and no conversion */
			NOT_RTL;
			while ((fgets(pold,BUFSIZ,fpin)) != NULL)
				(void) fputs(pold,stdout);
		}

		if (fpin != stdin)		/* close input file ... */
			(void) fclose(fpin);	/* ... unless it's stdin */
	}

	/* finish up */
	(void) catclose(nlmsg_fd);
	return value;
}

/*
**************************************************************************
** get_basename() 
**
** description:
**	get the basename of the command
**
** return value:
**	ptr to start of base name
**************************************************************************
*/

static char *
get_basename( p)
char *p;			/* ptr to start of command name */
{
	char *slash;		/* pointer to char after slash */

	for (slash = p ; *p ; ADVANCE( p)) {
		if (CHARAT( p) == '/') {
			slash = p + 1;
		}
	}

	return slash;
}
