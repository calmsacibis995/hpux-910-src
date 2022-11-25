static char *HPUX_ID = "@(#) $Revision: 66.1 $";
/*
 * Do simple formatting of paragraphs of text.
 * See the manual entry for details.
 * Designed for speed, at some cost in complexity and redundancy.
 *
 * Compile with -DDEBUG to include ShowSort(), useful for debugging
 * the Justify() routine.
 *
 * There is a minor undocumented bug in Dump().  It calls Justify() in the
 * case where the last word of the last line of a paragraph is an end-of-
 * sentence word (typical) and ends just one blank before the margin
 * (rare).  This results in one blank being inserted in the line when it's
 * not necessary.  It happens because the two trailing blanks after the
 * word cause an "ordinary" line dump before Fill() sees the next line and
 * knows it has an end of paragraph.  WARNING:  The situation would be
 * aggravated if FillWord() ever set blanks to an even larger number.
 */


#include <stdio.h>
#include <ctype.h>


/************************************************************************
 * MISC GLOBALS:
 */

#define	proc				/* null; easy to grep for procs */
#define	chNull	   ('\0')
#define	sbNull	   ((char *) NULL)
#define	fileNull   ((FILE *) NULL)
#define	false	   0
#define	true	   1

char	*myname;				/* how command invoked	*/
int	bflag	= false;			/* -b (blanks) option	*/
int	cflag	= false;			/* -c (center) option	*/
int	jflag	= false;			/* -j (justify) option	*/
int	m0flag	= false;			/* -m0 (automargin)	*/
int	rflag	= false;			/* -r (right justify)	*/

char	*defaultargs[] = {"-", sbNull};		/* read stdin by default */

#define	PrintErr(part1,part2) \
		fprintf (stderr, "%s: %s %s\n", myname, part1, part2);


/************************************************************************
 * FORMATTING CONTROL:
 */

#define	LINESIZE BUFSIZ			/* how big a line can be */

int	tabsize	= 8;			/* tab size in use	*/
#define	MAXTABSIZE 100			/* max legal tabsize	*/

long	fillmargin   = 72;		/* right margin for fill */
long	centermiddle = 40;		/* middle col for center */

#if defined NLS || defined NLS16
#include <locale.h>
extern int _nl_space_alt;
#endif /* NLS || NLS16 */


/************************************************************************
 * M A I N
 *
 * Initialize, check arguments, open and read files, and pass a series
 * of lines to lower-level routines.
 */

proc main (argc, argv)
	int	argc;
	char	**argv;
{
	extern	int	optind;			/* for getopt() */
	extern	char	*optarg;
	int	optchar;
	long	maxmargin;			/* max allowed	*/
register FILE	*filep;				/* file to read	*/
	int	retval = 0;			/* return value	*/

	long	atol();
	char	*fgets();

	myname = *argv;

/*
 * INITIALIZE FOR NLS
 */

#if defined NLS || defined NLS16
	if (!setlocale(LC_ALL,"")) 
		fputs(_errlocale(),stderr);
#endif /* NLS || NLS16 */


/*
 * CHECK ARGUMENTS:
 */

	while ((optchar = getopt (argc, argv, "bcjm:rt:")) != EOF)
	    switch (optchar)
	    {
	    case 'b':	bflag   = true;				break;
	    case 'c':	cflag   = true;				break;
	    case 'j':	jflag   = true;				break;
	    case 'm':	fillmargin = centermiddle = atol (optarg);
			m0flag  = (fillmargin == 0L);		break;
	    case 'r':	rflag   = true;				break;
	    case 't':	tabsize = atoi (optarg);		break;
	    default:	Usage();
	    }

	if (cflag + jflag + rflag > 1)		/* mutually exclusive */
	{
	    PrintErr ("only one of -cjr is allowed", "");
	    Usage ();
	}

	maxmargin = cflag? (LINESIZE / 2) : LINESIZE;

	if ((fillmargin < 0L) || (fillmargin > maxmargin))
	{
	    fprintf (stderr, "%s: %s must be in range 0-%d\n",
		     myname, (cflag ? "middle" : "margin"), maxmargin);
	    Usage ();
	}

	if ((tabsize < 1) || (tabsize > MAXTABSIZE))
	{
	    fprintf (stderr, "%s: tab size must be in range 1-%d\n",
		     myname, MAXTABSIZE);
	    Usage ();
	}

	argc -= optind;
	argv += optind;

	if (argc < 1)				/* use default arguments */
	    argv = defaultargs;

/*
 * OPEN AND READ INPUT FILES:
 *
 * Be careful to keep stdin open for filenames of "-".
 */

	for ( ; *argv != sbNull; argv++)		/* each argument */
	{
	    if (strcmp (*argv, "-") == 0)		/* read stdin */
		filep = stdin;

	    else if ((filep = fopen (*argv, "r")) == fileNull)
	    {
		PrintErr ("can't open", *argv);
		retval = 1;
		continue;
	    }

	    while (DoLine (filep))
		;

	    if (filep != stdin)			/* keep stdin open for reuse */
		fclose (filep);
	}

/*
 * DUMP LAST LINE AND EXIT:
 */

	if (! cflag)				/* not centering	*/
	    Dump (true);			/* force end paragraph	*/

	exit (retval);

} /* main */


/************************************************************************
 * U S A G E
 *
 * Tell correct usage and exit.
 */

proc Usage ()
{
	fprintf (stderr,
	    "usage: %s [-bcrj] [-m column] [-t tabsize] [files...]\n", myname);
	exit (1);
} /* Usage */


/************************************************************************
 * D O   L I N E
 *
 * Read one input line and do generic processing (read chars from input
 * to inline, crunching backspaces).  Return true if successful, else
 * false at end of input file.  This must be done before expanding tabs;
 * pass half-processed line to Center() or Fill() for more work.
 *
 * Assumes getc() is efficient.
 * inline[] is static so it is only allocated once.
 */

proc int DoLine (filep)
register FILE	*filep;				/* open file to read	*/
{
static	 char	inline [LINESIZE];		/* after reading in	*/
register char	*incp	 = inline;		/* place in inline	*/
register char	*incplim = inline + LINESIZE;	/* limit of inline	*/
register int	c;				/* temp hold of input	*/

/*
 * READ LINE WITH BACKSPACE CRUNCHING:
 */

	while (incp < incplim)			/* buffer not full */
	{
	    switch (c = getc (filep))		/* get and save one char */
	    {
		case EOF:
		    if (incp == inline)		/* nothing read yet */
			return (false);
						/* else fall through	 */
		case chNull:			/* fall through 	 */
		case '\n':			/* end of line		 */
		    incplim = incp;		/* set limit (quit loop) */
		    break;

		case '\b':			/* backspace	*/
		    if (incp > inline)		/* not at start	*/
			incp--;			/* just back up	*/
		    break;

		default:			/* any other char */
		    *incp++ = c;
		    break;
	    }
	}
	/* now incplim is correctly set to last char + 1 */

/*
 * PASS LINE ON FOR MORE WORK:
 */

	if (cflag)	Center (inline, incplim);
	else		Fill   (inline, incplim);

	return (true);				/* maybe more to read */

} /* DoLine */


/************************************************************************
 * C E N T E R
 *
 * Center text (starting at inline, ending before inlinelim).  First copy
 * chars from inline to outline skipping nonprintables and expanding tabs.
 * For efficiency (at the cost of simplicity), note and skip indentation
 * at the same time.  Then, print outline with indentation to center it.
 *
 * outline[] is static so it is only allocated once; startpara so it is
 * used for successive lines.
 */

proc Center (inline, inlinelim)
	 char	*inline;			/* start of input line	*/
register char	*inlinelim;			/* end of line + 1	*/
{
register char	*incp;				  /* pointer in inline	*/
static	 char	outline [LINESIZE + MAXTABSIZE];  /* after generic work	*/
register char	*outcp      = outline;		  /* place in outline	*/
register char	*outlinelim = outline + LINESIZE; /* limit of outline	*/

static	 int	startpara = true;		/* expect para. start?	*/
	 int	haveword  = false;		/* hit word in inline?	*/
register unsigned char	ch;			/* current working char	*/
register int	needsp;				/* spaces need for tab	*/
register int	indent = 0;			/* size of indentation	*/
	 int	textwidth;			/* size of text part	*/

/*
 * SCAN INPUT LINE:
 */

	for (incp = inline; (incp < inlinelim) && (outcp < outlinelim); incp++)
	{
	    ch = *(unsigned char *)incp;

/*
 * SKIP WHITE SPACE:
 */

#if defined NLS || defined NLS16
	    if ((ch == '\t') || (ch == ' ') || ch == (_nl_space_alt))
#else
	    if ((ch == '\t') || (ch == ' '))
#endif /* NLS || NLS16 */
	    {
#if defined NLS || defined NLS16
		needsp = ((ch == ' ') || (ch == _nl_space_alt)) ?
#else
		needsp = (ch == ' ') ?
#endif /* NLS || NLS16 */
			 1 : tabsize - ((indent + outcp - outline) % tabsize);

		if (! haveword)				/* indentation */
		    indent += needsp;
		else					/* past text */
		    for ( ; needsp > 0; needsp--)
			*outcp++ = ' ';			/* save blanks */
	    }

/*
 * CARRY OVER PRINTABLE CHARS AND NOTE INDENTATION:
 */

	    else if (isprint (ch))			/* note: ch != ' ' */
	    {
		*outcp++ = ch;				/* just copy it over */
		haveword = true;
	    }

	    /* else do nothing, which tosses other chars */

	} /* while */

	/* now outcp is the new outlinelim */

/*
 * HANDLE BLANK LINE (no text):
 */

	if (! haveword)
	{
	    putchar ('\n');			/* "eat" any whitespace */
	    startpara = true;			/* expect new paragraph */
	}

/*
 * EAT TRAILING BLANKS, SET TEXTWIDTH:
 */

	else
	{
#if defined NLS || defined NLS16
	    while ((outcp [-1] == ' ') || ((int)(unsigned char)outcp [-1] == _nl_space_alt))
	    	/* note that outcp > outline */
#else
	    while (outcp [-1] == ' ')		/* note that outcp > outline */
#endif /* NLS || NLS16 */
		outcp--;

	    textwidth = (outcp - outline);	/* thus textwidth > 0 */

/*
 * SET AUTOMARGIN AND PRINT CENTERED LINE:
 *
 * The equations used depend on truncating division:
 *
 *		eqn. 1		eqn. 2
 *	Margin	Middle	Width	Results
 *	odd	exact	odd	exact centering
 *	odd	exact	even	extra char to right
 *	even	left	odd	extra char to left
 *	even	left	even	exact centering (due to "extra" char to right)
 *
 * When centermiddle is default or given by user, it is the same as the
 * first two lines above (middle exactly specified).
 */

	    if (startpara)			/* start of paragraph */
	    {
		startpara = false;

		if (m0flag)			/* set automargin */
/* 1 */		    centermiddle = (indent + textwidth + 1) / 2;
	    }

/* 2 */	    PrintIndent (centermiddle - ((textwidth + 1) / 2));
	    printf ("%.*s\n", textwidth, outline);

	} /* else */

} /* Center */


/************************************************************************
 * WORD DATA STRUCTURES USED TO TRACK WORDS WHILE FILLING:
 *
 * This complicated scheme is used to minimize the actual data moving
 * and manipulation needed to do the job.
 *
 * Words are kept in wordbuf[] without trailing nulls.  It is large enough
 * to accomodate two lines' worth of words plus wrap around to start a new
 * word (which might be as big as a line) without overwriting old words
 * not yet dumped.  wordcp -> next free location in wordbuf[];
 * wordcpwrap -> last place a new word is allowed to start.
 *
 * Words are pointed to and described by word[] structures.  The array is
 * big enough to accomodate two lines' worth of words, assuming each word
 * takes at least two characters (including separator).  wordbase and
 * wordlimit are the bounds of the array.  wordfirst remembers the first
 * word in the array not yet printed.  wordat is the word being worked on
 * after wordnext is advanced (and maybe wrapped around).  New elements
 * are "allocated" using wordnext, a pointer that is wrapped around as
 * needed.
 *
 * inlinenum and incolumn track the current input line and column per
 * paragraph.  outlinenum tracks the next line to print.  All are base 1,
 * but inlinenum is preset to zero at the start of each paragraph.
 *
 * pendchars tracks the number of dumpable characters accrued so far, to
 * trigger dumping.  indent1 and indent2 remember the indentations seen
 * for the first two lines of each paragraph.
 */

	char	wordbuf [LINESIZE * 3];	/* see above for discussion */
	char	*wordcp	    = wordbuf;
	char	*wordcpwrap = wordbuf + (LINESIZE * 2);

typedef struct {			/* describe one word		*/
	char	*cp;			/* word location in buffer	*/
	int	inlinenum;		/* word start line in input	*/
	int	incolumn;		/* word start column in input	*/
	int	length;			/* size of word			*/
	int	score;			/* word + next for jflag	*/
	int	blanks;			/* output trailing blanks need	*/
} WORD;

#define	WORDMAX LINESIZE		/* see above for discussion */
	WORD	word [WORDMAX];
	WORD	*wordbase  = & word [0];
	WORD	*wordlimit = & word [WORDMAX];
#define WORDNEXT(at) { if (++at == wordlimit) at = wordbase; }	/* statement */
#define WORDPREV(at) (((at == wordbase) ? wordlimit : at) - 1)	/* function  */
	WORD	*wordfirst = & word [0];
	WORD	*wordat;
	WORD	*wordnext  = & word [0];

	int	inlinenum  = 0;		/* see above for discussion */
	int	incolumn;
	int	outlinenum = 1;

	int	pendchars = 0;
	int	indent1, indent2;


/************************************************************************
 * F I L L
 *
 * Parse an input line (inline to inlinelim) into words and trigger
 * FillWord() as needed to finish each word, which in turn can call
 * Dump() to dump output lines.  This routine sacrifices simplicity and
 * clarity for efficiency.  It uses shared global word data except
 * outlinenum and pendchars.
 */

proc Fill (inline, inlinelim)
	 char	*inline;		/* start of input line	*/
register char	*inlinelim;		/* end of line + 1	*/
{
register char	*incp;			/* place in inline	*/
register unsigned  char	ch;		/* current working char	*/
	 int	haveword = false;	/* hit word in inline?	*/
register int	inword	 = false;	/* currently in a word?	*/

/*
 * SCAN INPUT LINE:
 */

	inlinenum++;
	incolumn = 1;				/* starting a line */

	for (incp = inline; incp < inlinelim; incp++)
	{
	    ch = *(unsigned char *)incp;

/*
 * AT WHITE SPACE; FINISH PREVIOUS WORD if any, then skip white space:
 */

#if defined NLS || defined NLS16
	    if ((ch == '\t') || (ch == ' ') || (ch == _nl_space_alt))
#else
	    if ((ch == '\t') || (ch == ' '))
#endif /* NLS || NLS16 */
	    {
		if (inword)			/* was skipping a word */
		{
		    inword = false;
		    FillWord();
		}

#if defined NLS || defined NLS16
		incolumn += ((ch == ' ') || (ch == _nl_space_alt)) ?
#else
		incolumn += (ch == ' ') ?
#endif /* NLS || NLS16 */
			    1 : tabsize - ((incolumn - 1) % tabsize);
	    }

/*
 * AT PART OF WORD; START NEW ONE IF NEEDED:
 */

	    else if (isprint (ch))	/* note: ch != ' ' */
	    {
		if (! inword)				/* start new word */
		{
		    inword = true;

		    if (wordcp > wordcpwrap)		/* past wrap point   */
			wordcp = wordbuf;		/* wraparound buffer */

		    wordat = wordnext;
		    WORDNEXT (wordnext);

		    wordat->cp	      = wordcp;		/* note start of word */
		    wordat->inlinenum = inlinenum;	/* note input line    */
		    wordat->incolumn  = incolumn;	/* note input column  */

		    if (! haveword)
		    {
			haveword = true;

			switch (inlinenum)		/* note indentations */
			{
			    case 1:	indent1 = incolumn - 1; break;
			    case 2:	indent2 = incolumn - 1; break;
			}
		    }
		}

/*
 * SAVE ONE CHAR OF WORD:
 */

		*wordcp++ = ch;
		incolumn++;

	    } /* else */

	    /* else skip other chars by doing nothing */

	} /* for */

/*
 * END OF LINE; HANDLE BLANK LINE, OR FINISH WORD AND AUTOMARGIN:
 */

	if (! haveword)				/* no text on line */
	{
	    inlinenum--;			/* don't count empty lines */
	    Dump (true);			/* force end paragraph */
	    putchar ('\n');			/* put this empty line */
	    inlinenum = 0;			/* start new paragraph */
	}
	else					/* have text on line */
	{
	    if (inword)				/* line ends in word */
		FillWord();

	    if (m0flag && (inlinenum == 1))	/* need	to note right margin */
		fillmargin = wordat->incolumn + wordat->length - 1;
	}
} /* Fill */


/************************************************************************
 * F I L L   W O R D
 *
 * Save values for the word just finished and dump the output line if
 * needed.  Uses shared global word values, but does not touch outlinenum,
 * and only increments pendchars.
 *
 * Trailing blanks (1 or 2) needed after a word are figured here.
 * wordlen is the total length (nonzero) and wordcp points to the char
 * past the end of the word.  Two blanks are needed after words ending in:
 *
 *	<terminal>[<quote>][<close>]
 *
 * where <terminal> is any of	. : ? !
 *	 <quote>    is any of	' "
 *	 <close>    is any of	) ] }
 *
 * For efficiency, this routine avoids calling others to do character
 * matching; it does them in line.
 */

proc FillWord ()
{
	 int	wordlen;		/* length of word to fill */
	 int	blanks = 1;		/* trailing blanks needed */
register char	ch1, ch2, ch3;		/* last chars of word	  */

	wordlen = wordat->length = (incolumn - wordat->incolumn);

/*
 * CHECK FOR SPECIAL END OF WORD:
 */

	ch3 = wordcp [-1];

	if ((ch3 == '.') || (ch3 == ':') || (ch3 == '?') || (ch3 == '!'))
	{
	    blanks = 2;			/* <terminal> */
	}
	else if (wordlen >= 2)
	{
	    ch2 = wordcp [-2];

	    if (((ch2 == '.')  || (ch2 == ':') || (ch2 == '?') || (ch2 == '!'))
	    &&  ((ch3 == '\'') || (ch3 == '"')
	      || (ch3 == ')')  || (ch3 == ']') || (ch3 == '}')))
	    {
		blanks = 2;		/* <terminal><quote or close> */
	    }
	    else if (wordlen >= 3)
	    {
		ch1 = wordcp [-3];

		if (((ch1 == '.')  || (ch1 == ':') || (ch1 == '?')
		  || (ch1 == '!'))
		&&  ((ch2 == '\'') || (ch2 == '"'))
		&&  ((ch3 == ')')  || (ch3 == ']') || (ch3 == '}')))
		{
		    blanks = 2;		/* <terminal><quote><close> */
		}
	    }
	} /* else */

/*
 * SAVE VALUES AND CHECK IF NEED TO DUMP OUTPUT LINE:
 *
 * The check ignores indentation, e.g. might be able to dump sooner
 * than we actually do, but it's no problem to wait.
 */

	pendchars += wordlen + (wordat->blanks = blanks);

	if (pendchars > fillmargin)		/* time to dump line	*/
	    Dump (false);			/* not end of paragraph	*/

} /* FillWord */


/************************************************************************
 * D U M P
 *
 * Print output line(s), with all necessary indentation and formatting,
 * if required (at end of paragraph) or if permissible.  If required,
 * indent1 is used if indent2 is not set yet (not on second line of
 * input).  Otherwise, if not at end of paragraph, dumping is only
 * permissible after beginning the second input line, so fillmargin and
 * indent2 are known, so tagged paragraphs are done right.
 *
 * Whenever dumping, all "full" lines are dumped, which means more than
 * just one may be printed per call.  jflag or rflag formatting is
 * applied to all lines, except that jflag is ignored for the last line
 * of each paragraph.
 *
 * Uses shared global word data, but does not touch inlinenum, incolumn,
 * indent1, or indent2.  
 */

proc Dump (endpara)
	 int	endpara;			/* end of paragraph? */
{
	 int	haveindent2 = (inlinenum >= 2);	/* indent2 known?	*/
	 int	startpara;			/* start of paragraph?	*/
	 int	normal;				/* non-tagged line?	*/
register WORD	*wordpast = wordfirst;		/* past last to dump	*/
register int	wordlen;			/* length of one word	*/
	 int	indent;				/* local value		*/
register int	outneed;			/* chars need to dump	*/
register int	outchars;			/* chars found to dump	*/

/*
 * IF DUMPING NEEDED, DUMP LINES TILL DONE:
 */

	if (! (endpara || haveindent2))			/* not time to dump */
	    return;

	while ((pendchars > fillmargin)			/* line is full */
	||     (endpara && (wordfirst != wordnext)))	/* more to dump */
	{
	    startpara = (outlinenum < 2);
	    indent    = (startpara || (! haveindent2)) ? indent1 : indent2;

/*
 * CHECK FOR TAGGED PARAGRAPH if needed:
 */

	    normal = true;				/* default == no tag */

	    if (startpara && haveindent2 && (indent1 < indent2))
	    {
		int	incol2 = indent2 + 1;		/* column needed */

		while ((wordpast != wordnext)		/* more words  */
		&&     (wordpast->inlinenum == 1))	/* from line 1 */
		{
		    if (wordpast->incolumn == incol2)	/* bingo */
		    {
			normal = false;
			break;
		    }
		    WORDNEXT (wordpast);
		}

		if (normal)
		    wordpast = wordfirst;		/* reset value */

/*
 * PRINT TAG PART OF TAGGED PARAGRAPH:
 */

		else
		{
		    WORD *wordat = wordfirst;	/* local value */

		    while (wordat != wordpast)	/* decrement pendchars */
		    {
			pendchars -= wordat->length + wordat->blanks;
			WORDNEXT (wordat);
		    }

		    PrintIndent (indent);
		    PrintTag  (wordpast);	/* preceding words   */
		    wordfirst = wordpast;	/* do rest of line   */
		    indent    = indent2;	/* as if second line */
		}
	    } /* if */

/*
 * FIND WORDS WHICH FIT ON [REST OF] LINE:
 */

	    if (indent >= fillmargin)		/* don't over indent */
		indent  = fillmargin - 1;

	    outneed   = fillmargin - indent;	/* always greater than zero */
	    outchars  = 0;
	    wordlen   = wordpast->length;

	    do {				/* always consume one */
		outchars += wordlen + wordpast->blanks;
		WORDNEXT (wordpast);
	    }
	    while ((wordpast != wordnext)	/* not last word */
	    &&     (outchars + (wordlen = wordpast->length) <= outneed));
						/* next will fit */

	    pendchars -= outchars;		/* pendchars to consume */

	    /* from now on, don't include trailing blanks on last word */
	    outchars -= (WORDPREV (wordpast) -> blanks);

	    /* now wordfirst and wordpast specify the words to dump */

/*
 * PRINT INDENTATION AND PREPARE JUSTIFICATION:
 */

	    if (rflag)				/* right-justify only */
	    {
		if (normal)			/* nothing printed yet */
		    PrintIndent (fillmargin - outchars);
		else				/* indent + tag printed */
		{
		    int blanks = fillmargin - outchars - indent;

		    while (blanks-- > 0)	/* can't use PrintIndent() */
			putchar (' ');
		}
	    }
	    else
	    {
		if (normal)			/* not already done */
		    PrintIndent (indent);

		if (jflag && ! (endpara && (wordpast == wordnext)))
		    Justify (outneed - outchars, wordpast);
	    }

/*
 * PRINT REST OF LINE:
 */

	    PrintWords (wordpast);
	    putchar ('\n');
	    wordfirst = wordpast;
	    outlinenum++;				/* affects startpara */

	} /* while */

	if (endpara)
	    outlinenum = 1;

} /* Dump */


/************************************************************************
 * P R I N T   I N D E N T
 *
 * Print line indentation (if > 0), optionally using tabs where possible.
 * Does not print a newline.
 */

proc PrintIndent (indent)
	int	indent;		/* leading indentation */
{
	if (indent > 0)				/* indentation needed */
	{
	    if (! bflag)			/* unexpand leading blanks */
	    {
		while (indent >= tabsize)
		{
		    putchar ('\t');
		    indent -= tabsize;
		}
	    }
	    printf ("%*s", indent, "");		/* [remaining] blanks */
	}
} /* PrintIndent */


/************************************************************************
 * P R I N T   T A G
 *
 * Print paragraph tag words from word[] array beginning with (global)
 * wordfirst and ending before (parameter) wordpast, using input column
 * positions in each word's data.  Assumes indentation of indent1 was
 * already printed on the line.  Assumes *wordpast is the next word on
 * the line and its column position is valid, and appends spaces up to
 * the start of that word.   Doesn't print a newline.
 *
 * Line indentation must already be done, as this routine doesn't know
 * how to print leading tabs, only blanks.
 */

proc PrintTag (wordpast)
	 WORD	*wordpast;		/* past last to print */
{
register WORD	*wordat = wordfirst;	/* local value	  */
register int	outcol = indent1 + 1;	/* next column	  */
	 int	wordcol;		/* desired column */
register char	*wordcp;		/* place in word  */
	 char	*wordcplim;		/* limit of word  */

	while (true)				/* till break */
	{
	    wordcol = wordat->incolumn;

	    while (outcol < wordcol)		/* space over to word */
	    {
		putchar (' ');
		outcol++;
	    }

	    if (wordat == wordpast)		/* past last word */
		break;				/* quit the loop  */

	    wordcp    = wordat->cp;
	    wordcplim = wordcp + wordat->length;

	    while (wordcp < wordcplim)		/* print word */
		putchar (*wordcp++);

	    outcol += wordat->length;
	    WORDNEXT (wordat);
	}
} /* PrintTag */


/************************************************************************
 * P R I N T   W O R D S
 *
 * Print words from word[] array beginning with (global) wordfirst and
 * ending before (parameter) wordpast, using word sizes and trailing
 * blanks (except for last word on line).  Doesn't print a newline.
 */

proc PrintWords (wordpast)
	 WORD	*wordpast;		/* past last to print */
{
register WORD	*wordat = wordfirst;	/* local value   */
register char	*wordcp;		/* place in word */
	 char	*wordcplim;		/* limit of word */
register int	blanks;			/* after a word	 */

	while (true)				/* till break */
	{
	    wordcp    = wordat->cp;
	    wordcplim = wordcp + wordat->length;
	    blanks    = wordat->blanks;		/* set before do WORDNEXT() */

	    while (wordcp < wordcplim)		/* print word */
		putchar (*wordcp++);

	    WORDNEXT (wordat);

	    if (wordat == wordpast)		/* just did last word */
		break;

	    while (blanks-- > 0)		/* print trailing blanks */
		putchar (' ');
	}
} /* PrintWords */


/************************************************************************
 * J U S T I F Y
 *
 * Do left/right justification of [part of] a line in the word[] array
 * beginning with (global) wordfirst and ending before (parameter)
 * wordpast, by figuring where to insert blanks.
 *
 * Gives each word (except the last on the line) a score based on its
 * size plus the size of the next word.  Quicksorts word indices into
 * order of current trailing blanks (least first), then score (most
 * first).  Cycles through this list adding trailing blanks to word[]
 * entries such that words will space out nicely when printed.
 *
 * sort[] and words are global so they are accessible to a debug routine;
 * also, sort[] is only allocated once.
 */

WORD	*sort [WORDMAX];		/* sorted pointers */
int	words;				/* words in sort[] */

proc Justify (blanks, wordpast)
register int	blanks;			/* blanks to insert   */
	 WORD	*wordpast;		/* past last to print */
{
register WORD	*wordat;		/* local value		*/
register int	sortat;			/* place in sort[]	*/
register int	wordlen;		/* size of this word	*/
register int	nextlen;		/* size of next word	*/
	 int	level;			/* current blanks level	*/
	 int	CompareWords();		/* for qsort()		*/

	wordat = WORDPREV (wordpast);		/* last word on line */

	if ((blanks < 1) || (wordat == wordfirst))
	    return;				/* can't do anything */

/*
 * COMPUTE SCORES FOR WORDS AND SORT INDICES:
 *
 * March backwards through the words on line, starting with next to last.
 */

	words	= 0;
	nextlen	= wordat->length;		/* length of last word */

	do {					/* always at least one */
	    wordat  = WORDPREV (wordat);
	    wordlen = wordat->length;
	    wordat->score = wordlen + nextlen;	/* this plus next */
	    nextlen = wordlen;
	    sort [words++] = wordat;		/* prepare for sorting */
	}
	while (wordat != wordfirst);

	qsort ((char *) sort, words, sizeof (WORD *), CompareWords);

/*
 * ADD TRAILING BLANKS TO PAD OUT WORDS:
 *
 * Each pass through the sorted list adds one trailing blank to each word
 * not already past the current level.  Thus all one-blank words are brought
 * up to two; then all twos up to three; etc.
 */

	level = 0;

	while (true)					/* till return */
	{
	    level++;

	    for (sortat = 0; sortat < words; sortat++)
	    {
		wordat = sort [sortat];
		if (wordat->blanks > level)		/* end of this level */
		    break;				/* start next level  */

		wordat->blanks++;
		if (--blanks <= 0)			/* no more needed */
		    return;
	    }
	}
} /* Justify */


/************************************************************************
 * C O M P A R E   W O R D S
 *
 * Compare two word[] entries based on pointers to (WORD *), as called
 * from qsort(3).  Tell which entry has priority for receiving inserted
 * blanks (least trailing blanks first, then highest scores), by returning
 * -1 for the first entry, +1 for second entry, or 0 if no difference.
 * (-1 literally means first entry < second entry, so it sorts first.)
 */

proc int CompareWords (wordp1, wordp2)
	WORD	**wordp1, **wordp2;	/* pointers to (WORD *) */
{
	WORD	*word1	= *wordp1;		/* (WORD *) pointers */
	WORD	*word2	= *wordp2;
	int	blanks1	= word1->blanks;	/* trailing blanks */
	int	blanks2	= word2->blanks;

	if (blanks1 == blanks2)			/* commonest case */
	{
	    int	score1 = word1->score;		/* word scores */
	    int	score2 = word2->score;

	    if	    (score1 > score2)	return (-1);	/* word1 has priority */
	    else if (score1 < score2)	return ( 1);
	    else			return (0);
	}
	else if (blanks1 < blanks2)	return (-1);	/* word1 has priority */
	else				return ( 1);

} /* CompareWords */


#ifdef DEBUG

/************************************************************************
 * S H O W    S O R T
 *
 * Show data from the Justify() sort[] array, using shared globals sort[]
 * and words.   Intended to be called from a debugger before sorting,
 * after sorting, and/or after inserting blanks.
 */

proc ShowSort()
{
	int	sortat;			/* where in sort[]	*/
	WORD	*wordat;		/* where in word[]	*/
	int	wordlen;		/* size of one word	*/
	int	totlen = 0;		/* total words + blanks	*/

	printf ("\nsortat  wordat  blanks  score  length  word\n");

	for (sortat = 0; sortat < words; sortat++)
	{
	    wordat  = sort [sortat];
	    wordlen = wordat->length;
	    totlen += wordlen + wordat->blanks;

	    printf ("%6d  %6d  %6d  %5d  %6d  %*.*s\n", sortat, wordat,
		wordat->blanks, wordat->score, wordlen,
		wordlen, wordlen, wordat->cp);
	}

	printf ("Total length + blanks = %d\n", totlen);

} /* ShowSort */

#endif /* DEBUG */
