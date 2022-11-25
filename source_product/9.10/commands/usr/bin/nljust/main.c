/* @(#) $Revision: 70.2 $ */   

/*
**************************************************************************
** Include Files
**************************************************************************
*/

#include <nl_types.h>
#include <nl_ctype.h>
#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <string.h>
#include "justify.h"
#include "extern.h"

/*
**************************************************************************
** External references
**************************************************************************
*/

extern char *malloc();			/* get memory */
extern void free();			/* free memory */
extern char *getenv();			/* get environment variable */
extern nl_catd catopen();		/* open catalog */
extern nl_direct _nl_direct;		/* direction (R-to-L, L-to-R) flag */
extern nl_mode _nl_mode;		/* mode (Latin, non-Latin) flag */
extern nl_order _nl_order;		/* order (Key, Screen) flag */
extern int _nl_space_alt;		/* alternative space flag */
extern int _nl_context;			/* context analysis flag */
extern char *optarg;            	/* pointer to start of option arg */
extern int optind;              	/* argv index of next arg */
extern int opterr;              	/* error message indicator */

extern void justify();			/* justify lines */
extern void shift();			/* put in shift-in shift-out */
extern void shapes();			/* arabic context analysis */
extern void put_line();			/* place line on stdout */
extern void put_message();		/* place message on stderr */

/*
**************************************************************************
** Forward references
**************************************************************************
*/

extern void start_up();			/* initialize program */
extern void dofile();			/* process files */
extern char *get_basename();		/* command base name */

/*
**************************************************************************
** Driver Program
**************************************************************************
*/

main(argc,argv)
int argc;		/* argument count */
char **argv;		/* pointer to arguments */
{
	UCHAR fill1[FILL]; 		/* spaces for context analysis */
	UCHAR buf1[MAX_LINE];		/* input or output buffer */
	UCHAR fill2[FILL];		/* spaces for context analysis */
	UCHAR buf2[MAX_LINE];		/* input or output buffer */
	UCHAR fill3[FILL];		/* spaces for context analysis */
	UCHAR buf3[MAX_LINE];		/* only wrap buffer */

	register int retval = SUCCESSFUL;	/* assume a sucessful return */

	/* parse cmd line options, etc */
	start_up(argc,argv,buf3,fill1,fill2,fill3);
	
	/* open and process input files one at a time */
	for ( ; *Filename ; Filename++) {
		if (!strcmp(*Filename,"-")) {	/* open input file */
			Input = stdin;
		} else if (!(Input = fopen(*Filename,"r"))) {
			put_message(WARN,BAD_INPUT,*Filename);
			retval = UNSUCCESSFUL;	/* ... get next file */
			continue;		/* ... if can't open */
		}
		if (JustFile) {			/* process the file */
			dofile(buf1,buf2);
		} else {			/* copy the file */
			while (fgets(buf1,MAX_LINE,Input) != NULL)
				(void) fputs(buf1,stdout);
		}
		if (Input != stdin)		/* unless it's stdin, */
			(void) fclose(Input);	/* ... close input file */
	}

	/* finish up */
	(void) catclose(Catd);		/* close the message catalog */
	if (Escape) free(Primary);	/* free up memory for font esc seq */
	return retval;			/* and leave */
}

/*
**************************************************************************
** Process File Routine
**************************************************************************
*/

void
dofile(buf1,buf2)
UCHAR *buf1;				/* input & output buffers */
UCHAR *buf2;				/* input & output buffers */
{
	while (get_line(buf1,buf2)) {	/* get a line */
		if (empty(InBuf)) {	/* if its empty just print it */
			(void) fputs(InBuf,stdout);
			(void) fputc(LastChar,stdout);
			HaveWrap = FALSE;
		} else {		/* otherwise, process the line */
			justify();		/* do the right (left) just */
			if (Lang == ARABIC) {
				shift();	/* primary & secondary lang */
				shapes();	/* arabic context analysis */
			}
			put_line();		/* put print line on stdout */
		}
	}
}

/*
**************************************************************************
** Initialization  Routine
**************************************************************************
*/

void
start_up(argc,argv,buf3,fill1,fill2,fill3)
int argc;			/* argument count */
char **argv;			/* pointer to arguments */
UCHAR *buf3;			/* pointer to wrap buffer */
UCHAR *fill1;			/* space filler for context analysis */
UCHAR *fill2;			/* space filler for context analysis */
UCHAR *fill3;			/* space filler for context analysis */
{
	register int optchar;		/* option character */
	register int i;			/* loop counter */
	register int num;		/* command line number */
	register char *lang;		/* points to language name */
	static char *deffiles[] = { "-", (char*)NULL };
					/* default arguments */

	/* set up wrap buffer pointer */
	WrapBuf = buf3;

	/* set up nulls for fill buffers (used for context analysis) */
	for (i=0 ; i<FILL ; i++) fill1[i] = fill2[i] = fill3[i] = EOL;

	/* get the command name */
	Cmd = get_basename( argv[0]);

	/* initialize the locale */
	if (! setlocale( LC_ALL, "")) {
		/* bad initialization */
		(void) fputs( _errlocale(), stderr);
		Catd = (nl_catd) -1;
		(void) putenv( "LANG=");       /* for perror */
	}
	else {
		/* good initialization: open message catalog,
		   ... keep on going if it isn't there */
		Catd = catopen( Cmd, 0);
	}

	/* setlocale mode, order, alt space, direction and context analysis */
	Mode = _nl_mode;
	Order = _nl_order;
	AltSpace = _nl_space_alt;
	if (JustFile = _nl_direct == NL_RTL ? TRUE : FALSE) {
		/* have a right-to-left language */
		/* Assumption: either hebrew or arabic */
		if (_nl_context == 1) {
			/* have arabic context analysis */
			Lang = ARABIC;
			Primary = ARAB_PRI;
		}
		else {
			/* do not have arabic context analysis */
			Lang = HEBREW;
			Primary = HEBR_PRI;
		}
	}
	else {
		/* not a right-to-left language */
		if(*(lang = getenv("LANG")) == '\0') {
			lang = DEFAULT;
		}
		Lang = OTHER;
		Primary = ROMN_PRI;
		put_message(WARN,BAD_LANG,lang);
	}

	/* parse command line options */

	opterr = 0;		/* disable getopt error message */
	while ((optchar = getopt(argc,argv,"j:m:o:r:w:x:e:aclnt")) != EOF) {
		switch (optchar) {
		case 'j':	/* justification */
			if (!strcmp(optarg,"l")) {
				Just = LEFT;
			} else if (!strcmp(optarg,"r")) {
				Just = RIGHT;
			} else {
				put_message(WARN,BAD_JUST,optarg);
			}
			break;
		case 'm':	/* mode */
			if (!strcmp(optarg,"l")) {
				Mode = NL_LATIN;
			} else if (!strcmp(optarg,"n")) {
				Mode = NL_NONLATIN;
			} else {
				put_message(WARN,BAD_MODE,optarg);
			}
			break;
		case 'o':	/* order */
			if (!strcmp(optarg,"k")) {
				Order = NL_KEY;
			} else if (!strcmp(optarg,"s")) {
				Order = NL_SCREEN;
			} else {
				put_message(WARN,BAD_ORDER,optarg);
			}
			break;
		case 'r':	/* wrap margin */
			if ((num = atoi(optarg)) > MAX_LINE-1) {
				put_message(WARN,BAD_1MAR,optarg);
			} else if (!num) {
				put_message(WARN,BAD_2MAR,optarg);
			} else {
				Margin = num;
			}
			break;
		case 'w':	/* printer width */
			if ((num = atoi(optarg)) > MAX_LINE-1) {
				put_message(WARN,BAD_1WIDTH,optarg);
			} else if (!num) {
				put_message(WARN,BAD_2WIDTH,optarg);
			} else {
				Width = num;
			}
			break;
		case 'x':	/* expand tabs */
			if (!isdigit(*optarg & 0377)) {
				Tab.tab = *optarg++;
			}
			if ((num = atoi(optarg)) > MAX_LINE-1) {
				put_message(WARN,BAD_1TAB,optarg);
			} else if (!num) {
				if (*optarg) put_message(WARN,BAD_2TAB,optarg);
			} else {
				Tab.stop = num;
			}
			break;
		case 'e':	/* primary escape sequence */
			Escape = TRUE;
			if (Primary = malloc((unsigned)strlen(optarg)+1)) {
				*Primary = ESC;
				(void) strcpy(Primary+1,optarg);
			} else {
				Escape = FALSE;
				put_message(WARN,NO_MEMORY);
			}
			break;
		case 'a':	/* justify no matter what the language */
			JustFile = TRUE;
			break;
		case 'c':	/* use enhanced printer shapes */
			Enhanced = TRUE;
			break;
		case 'l':	/* leading blanks */
			LBlanks = TRUE;
			break;
		case 'n':	/* don't output newlines */
			NewLine = FALSE;
			break;
		case 't':	/* truncate don't wrap */
			End = TRUNC;
			break;
		case '?':	/* unrecognized option */
			put_message(FATAL,BAD_USAGE);
		}
	}

	/* set up opposite language character mask */
	Mask = Mode == NL_LATIN ? L_MASK : NL_MASK;
	
	/* be sure wrap margin does not exceed print width */
	if (Margin > Width) {
		(void) sprintf(WrapBuf,"%d",Width);
		put_message(WARN,BAD_3MAR,WrapBuf);
		Margin = Width;
	}

	/* must be correspondence between file mode & justification */
	if (Lang != OTHER) {
		if (Mode == NL_NONLATIN && Just == LEFT) {
			put_message(WARN,BAD_LEFT);
		} else if (Mode == NL_LATIN && Just == RIGHT) {
			put_message(WARN,BAD_RIGHT);
		}
	}

	/* set up input file arguments */
	Filename = ((argc - optind) < 1) ? deffiles : argv + optind ;
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

/*
**************************************************************************
** Empty Line Routine
**************************************************************************
*/

int
empty(line)
UCHAR *line;
{
	/* Changed "do{ .. }while" to "while{ .. }"; so lines starting 
	   with null character do not get processed.
	   For more info see gets(3S) and ctype(3C)            */

	while (*line){
		/* FSDlj09516 fix; changed isgraph */
		if (!(isspace((int)*line++))) return FALSE;
	} 

	return TRUE;
}
