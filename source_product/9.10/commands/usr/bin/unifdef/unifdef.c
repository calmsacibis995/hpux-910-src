/* @(#) $Revision: 70.1 $ */

/*
 * unifdef - remove ifdef'ed lines
 *
 * unifdef [-ltc] [[-Dsym] [-Usym] [-iDsym] [-iUsym]]... [file]
 *
 *    -l     convert rejected lines to blank lines (normally deleted)
 *    -t     indicates file is plain text (C comments are not special)
 *    -c     only include rejected lines (complement)
 *
 *    -Dsym  define "sym"
 *    -Usym  undefine "sym"
 *    -iDsym ignore any defines of "sym"
 *    -iUsym ignore any undefines of "sym"
 *
 *    At least one arg from [-D -U -iD -iU] is required
 *
 * Exit status:
 *    0  -- normal
 *    1  -- output is different from input
 *    2  -- error in processing
 */

#ifdef ALL_SYMS
/*
 *  Additional options include:
 *
 *    -u      treat all symbols that are not explicitly Defined as
 *            if they were explicitly Undefined
 *
 *    -d      treat all symbols that are not explicitly Undefined as
 *            if they were explicitly Defined
 *
 *    At least one arg from [ -D -U -iD -iU -d -u ] is required
 */
#endif /* ALL_SYMS */

#include "unifdef.h"

static void flushline();
static void pfile();
static void prname();

#ifdef CCOPTS
static void parse_ccopts();
#endif

int exitstat = EXIT_NO_CHANGE;
int nsaved = 0;

#ifdef ALL_SYMS
/*
 * For -u and -d
 */
int assume_defined   = NO;
int assume_undefined = NO;
#endif /* ALL_SYMS */

main(argc, argv)
int argc;
char **argv;
{
    extern char *strrchr();

    char **curarg;
    register char *cp;
    register char *cp1;
    char ignorethis;
    extern int yydebug;
    int stdin_specified = NO;

    if (argv[0] == NULL || argv[0][0] == '\0')
	progname = "unifdef";
    else if ((progname = strrchr(progname, '/')) != NULL)
	progname++;
    else
	progname = argv[0];

    if (progname[0] == 'D')
	yydebug = 1;

    for (curarg = &argv[1]; --argc > 0 && !stdin_specified; curarg++)
    {
	if (*(cp1 = cp = *curarg) != '-')
	    break;

	if (strcmp(curarg, "--") == 0)
	{
	    curarg++;
	    break;
	}

	if (*++cp1 == 'i')
	{
	    ignorethis = YES;
	    cp1++;
	}
	else
	    ignorethis = NO;

	if ((*cp1 == 'D' || *cp1 == 'U') && cp1[1] != '\0')
	{
	    if (nsyms >= MAXSYMS)
	    {
		prname();
		fprintf(stderr, "too many symbols.\n");
		return EXIT_ERROR;
	    }
	    ignore[nsyms] = ignorethis;
	    true[nsyms] = *cp1 == 'D' ? YES : NO;
	    sym[nsyms++] = &cp1[1];
	}
	else if (ignorethis)
	{
	    prname();
	    fprintf(stderr, "unrecognized option: %s\n", cp);
	    goto usage;
	}
	else
	{
	    char *o = &cp[1];

	    do {
		switch(*o)
		{
		case '\0':
		    stdin_specified = YES;
		    break;
		case 'c':
		    complement = YES;
		    break;

		case 'l':
		    lnblank = YES;
		    break;

		case 't':
		    text = YES;
		    break;

#ifdef ALL_SYMS
		case 'd':
		    if (assume_undefined == YES)
		    {
			prname();
			fprintf(stderr,
			    "-d and -u are mutually exclusive\n");
			goto usage;
		    }
		    assume_defined = YES;
		    break;

		case 'u':
		    if (assume_defined == YES)
		    {
			prname();
			fprintf(stderr,
			    "-u and -d are mutually exclusive\n");
			goto usage;
		    }
		    assume_undefined = YES;
		    break;
#endif /* ALL_SYMS */

#ifdef CCOPTS

		case 'E':
		    parse_ccopts();
		    break;
#endif /* CCOPTS */

		default:
		unrec:
		    prname();
		    fprintf(stderr, "unrecognized option: %c\n", *o);
		    goto usage;
		}
	    } while (!stdin_specified && *++o != '\0');
	}
    }

#ifdef ALL_SYMS
    if (nsyms == 0 && assume_defined == NO && assume_undefined == NO)
    {
    usage:
	fprintf(stderr, "\
Usage: %s [-clt] [-d|-u]\n\
	       [[-Dsym] [-Usym] [-iDsym] [-iUsym]]... [file]\n\
    At least one arg from [-D -U -iD -iU -d -u] is required\n",
	    progname);
	return EXIT_ERROR;
    }
#else /* ! ALL_SYMS */
    if (nsyms == 0)
    {
    usage:
	fprintf(stderr, "\
Usage: %s [-clt] [[-Dsym] [-Usym] [-iDsym] [-iUsym]]... [file]\n\
    At least one arg from [-D -U -iD -iU] is required\n",
	    progname);
	return EXIT_ERROR;
    }
#endif /* ALL_SYMS */

    if (argc > 1 || (argc == 1 && stdin_specified))
    {
	prname();
	fprintf(stderr, "can only do one file.\n");
	return EXIT_ERROR;
    }

    /*
     * Open the input file, none or "-" assumes stdin.
     */
    if (argc == 1 && (curarg[0][0] != '-' && curarg[0][1] != '\0'))
    {
	filename = *curarg;
	if ((input = fopen(filename, "r")) == NULL)
	{
	    prname();
	    perror(*curarg);
	    return EXIT_ERROR;
	}
    }
    else
    {
	filename = "[stdin]";
	input = stdin;
    }

    pfile();
    fclose(input);
    fflush(stdout);

    /*
     * Only report status if asked for.
     */
    return exitstat;
}

#ifdef CCOPTS
/*
 * parse_ccopts() --
 *    Look at the CCOPTS environment variable (if set).
 *    Extract all -D and -U settings and act as if they were put on
 *    the command line.
 */
static void
parse_ccopts()
{
    extern char *getenv();
    char *cp;
    
    if ((cp = getenv("CCOPTS")) == NULL)
	return;

    while (*cp)
    {
	while (isspace(*cp))
	    cp++;

	if (*cp == '-' &&
	    (*++cp == 'D' || *cp == 'U') &&
	    (isalpha(*(cp+1)) || *(cp+1) == '_'))
	{
	    char *cpe;
	    char *endcp;

	    /* 
	     * Extract the word, we use the memory directly
	     * from the environment since we don't care what
	     * is in it after we're done.
	     */
	    for (cpe = cp + 1;
		 isalpha(*cpe) || *cpe == '_' || isdigit(*cpe);
		 cpe++)
		continue;

	    /*
	     * Put a '\0' at the end of the string and set endcp
	     * to the next character to process.
	     */
	    endcp = *cpe == '\0' ? cpe : cpe + 1;
	    *cpe = '\0';

	    if (cpe != cp + 1)	/* ignore "-D" and "-U" */
	    {
		if (nsyms >= MAXSYMS)
		{
		    prname();
		    fprintf(stderr,
			    "too many symbols in environment.\n");
		    exit(EXIT_ERROR);
		}

		ignore[nsyms] = NO;
		true[nsyms] = *cp == 'D' ? YES : NO;
		sym[nsyms++] = cp + 1;
	    }
	    cp = endcp;
	}
	else
	    while (*cp && !isspace(*cp))
		cp++;
    }
}
#endif /* CCOPTS */

int stqcline BSS;   /* start of current comment or quote */
char *errs[] = {
#define NO_ERR      0
			"",
#define END_ERR     1
			"",
#define ELSE_ERR    2
			"Inappropriate else",
#define ENDIF_ERR   3
			"Inappropriate endif",
#define IEOF_ERR    4
			"Premature EOF in ifdef",
#define CEOF_ERR    5
			"Premature EOF in comment",
#define Q1EOF_ERR   6
			"Premature EOF in quoted character",
#define Q2EOF_ERR   7
			"Premature EOF in quoted string"
};

static void
pfile()
{
    reject = 0;
    doif(-1, NO, reject, 0);
}

int if_sym_index = MAXSYMS;

doif(thissym, inif, prevreject, depth)
register int thissym;   /* index of the symbol that was last ifdef'ed */
int inif;               /* YES or NO we are inside an ifdef */
int prevreject;         /* previous value of reject */
int depth;              /* depth of ifdef's */
{
    register int lineval;
    register int thisreject;
    int doret;		/* tmp return value of doif */
    static int cursym = -1;	/*
				 * index of the symbol returned by checkline;
				 * made static so that cursym is retained
				 * across recursive calls to doif();
				 * initialized to -1 to differentiate
				 * from 0, which is a valid symbol index
				 */
    int stline;		/* line number when called this time */
    int save_cursym, save_insym;	/* for pushing & popping insym */

    stline = linenum;
    for (;;)
    {
	switch (lineval = checkline(&cursym))
	{
	case PLAIN:
	    flushline(YES);
	    break;

	case TRUE:
	case FALSE:
	    thisreject = reject;
	    /*
	     *  To handle nested if(n)def's of same symbol (cpp can),
	     *  keep a count of nesting in the insym[] array.
	     */
	    save_cursym = cursym;
	    save_insym = insym[cursym];

	    if (lineval == TRUE)
		insym[cursym] += 1;
	    else
	    {
		if (reject < 2)
		    reject = ignore[cursym] ? 1 : 2;
		insym[cursym] -= 1;
	    }
	    if (ignore[cursym])
		flushline(YES);
	    else
	    {
		exitstat = EXIT_CHANGED;
		flushline(NO);
	    }
	    doret = doif(cursym, YES, thisreject, depth + 1);
	    insym[save_cursym] = save_insym;
	    if (doret != NO_ERR)
		return error(doret, stline, depth);
	    break;

	case OTHER:
	    flushline(YES);
	    goto not_changed;

	case PARTIAL:
	    nsaved = 0;
	    exitstat = EXIT_CHANGED;
	not_changed:
	    if ((doret = doif(-1, YES, reject, depth + 1)) != NO_ERR)
		return error(doret, stline, depth);
	    break;

	case ELSE:
	    if (inif != 1)
		return error(ELSE_ERR, linenum, depth);
	    inif = 2;
	    if (thissym >= 0)
	    {
		if ((insym[thissym] = -insym[thissym]) < 0)
		    reject = ignore[thissym] ? 1 : 2;
		else
		    reject = prevreject;
		if (!ignore[thissym])
		{
		    flushline(NO);
		    break;
		}
	    }
	    flushline(YES);
	    break;

	case ENDIF:
	    if (inif == 0)
		return error(ENDIF_ERR, linenum, depth);
	    if (thissym >= 0)
	    {
		reject = prevreject;
		if (thissym > MAXSYMS)
		{		/* a #if */
		    if_sym_index--;
		    if (thissym != if_sym_index)
			fprintf(stderr, "error in handling #if nesting !!!\n");
		}
		if (!ignore[thissym])
		{
		    flushline(NO);
		    return NO_ERR;
		}
	    }
	    flushline(YES);
	    return NO_ERR;

	case LEOF:
	    {
		int err;
		err = incomment
		    ? CEOF_ERR
		    : inquote[QUOTE1]
		    ? Q1EOF_ERR
		    : inquote[QUOTE2]
		    ? Q2EOF_ERR
		    : NO_ERR;
		if (inif)
		{
		    if (err != NO_ERR)
			error(err, stqcline, depth);
		    return error(IEOF_ERR, stline, depth);
		}
		else if (err != NO_ERR)
		    return error(err, stqcline, depth);
		else
		    return NO_ERR;
	    }
	}
    }
}

#define endsym(c) (!isalpha (c) && !isdigit (c) && c != '_')

#define MAXLINE 256
char tline[MAXLINE] BSS;

int
checkline(cursym)
int *cursym;
{
    register char *cp;
    register char *symp;
    register char chr;
    int retval;
    int symind;
#   define KWSIZE 8
    char keyword[KWSIZE+1];
    int eatit = NO;

    linenum++;
    if (getlin(tline, sizeof tline, input, NO) == EOF)
	return LEOF;

    retval = PLAIN;
    if (*(cp = tline) != '#' ||
	incomment ||
	inquote[QUOTE1] ||
	inquote[QUOTE2])
	goto eol;

    nsaved = 0;
    cp = eatcomment(++cp);
    symp = keyword;
    while (!endsym(*cp))
    {
	*symp = *cp++;
	if (++symp >= &keyword[KWSIZE])
	    goto eol;
    }
    *symp = '\0';

    if (strcmp(keyword, "ifdef") == 0)
    {
	retval = YES;
	goto ifdef;
    }
    else if (strcmp(keyword, "ifndef") == 0)
    {
	char *scp;

	retval = NO;
    ifdef:
	eatit = YES;
	scp = cp = eatcomment(++cp);
	for (symind = 0;;)
	{
	    /*
	     *  Used to be a test "if (insym[symind] == 0)" here that
	     *  surrounded the text from here to XXX comment below.
	     *  It was removed to allow correct handling of nesting
	     *  of same symbol.
	     */
	    for (symp = sym[symind], cp = scp;
		 *symp && *cp == *symp;
		 cp++, symp++)
		continue;

	    chr = *cp;

	    /*
	     *  Did we match a known symbol?
	     */
	    if (*symp == '\0' && endsym(chr))
	    {
		/*
		 *  Yes, we matched a known symbol.  Change cursym to
		 *  the matched symbol index if:
		 *	- current symbol is not defined (< 0)	OR
		 *	- current symbol is defined *but*
		 *	  not ignoring it			OR
		 *	- current symbol is defined and
		 *	  it is to be ignored, *but*
		 *	  it is not to be ignored in this context
		 *	  (for instance - -iDIGNORE_THIS specified
		 *	   but within the context of an
		 * 	   #ifndef IGNORE_THIS)
		 */
		if (*cursym < 0 ||
		    ignore[*cursym] == 0 ||
		    insym[*cursym] <= 0)
		{
		    *cursym = symind;
		    retval = (retval ^ true[symind]) ? FALSE : TRUE;
		}
		else
		    retval = OTHER;

		break;
	    }

	    /* XXX */

	    if (++symind >= nsyms)
	    {
#ifdef ALL_SYMS
		/*
		 * If we are assuming that things are defined or
		 * undefined, we must make an entry for this in our
		 * sym[], ignore[] and true[] arrays.  Then we return
		 * the value just as as if it was specified on the
		 * command line.
		 */
		if (assume_defined || assume_undefined)
		{
		    *cursym = add_symbol(scp);
		    retval = (retval ^ true[*cursym]) ? FALSE : TRUE;
		}
		else
#endif /* ALL_SYMS */
		    retval = OTHER;
		break;
	    }
	}
    }
    else if (strcmp(keyword, "if") == 0)
    {
	cp = eatcomment(++cp);
	retval = parse_if(tline, cp);

	if (retval == TRUE || retval == FALSE)
	{
	    if ((*cursym = if_sym_index++) >= TOTAL_SYM_SLOTS)
	    {
		fprintf(stderr, "excessive nesting of #if's\n");
		exit(EXIT_ERROR);
	    }
	}
#ifdef TESTING
fprintf (stderr, "checkline returning %d from #if\n", retval);
#endif /* TESTING */
    }
    else if (strcmp(keyword, "else") == 0)
    {
	retval = ELSE;
	eatit = YES;
    }
    else if (strcmp(keyword, "endif") == 0)
    {
	retval = ENDIF;
	eatit = YES;
    }

eol:
    if (!text && !reject)
	for (; *cp;)
	{
	    if (incomment)
		cp = skipcomment(cp);
	    else if (inquote[QUOTE1])
		cp = skipquote(cp, QUOTE1);
	    else if (inquote[QUOTE2])
		cp = skipquote(cp, QUOTE2);
	    else if (*cp == '/' && cp[1] == '*')
		if (eatit)
		    cp = eatcomment(cp);
		else
		    cp = skipcomment(cp);
	    else if (*cp == '\'')
		cp = skipquote(cp, QUOTE1);
	    else if (*cp == '"')
		cp = skipquote(cp, QUOTE2);
	    else
		cp++;
	}
    return retval;
}

/*
 * Skip over comments and stop at the next character
 * position that is not whitespace.
 */
char *
skipcomment(cp)
register char *cp;
{
    if (incomment)
	goto inside;
    for (;; cp++)
    {
	while (*cp == ' ' || *cp == '\t')
	    cp++;
	if (text)
	    return cp;
	if (cp[0] != '/' || cp[1] != '*')
	    return cp;
	cp += 2;
	if (!incomment)
	{
	    incomment = YES;
	    stqcline = linenum;
	}
    inside:
	for (;;)
	{
	    for (; *cp != '*'; cp++)
		if (*cp == '\0')
		    return cp;
	    if (*++cp == '/')
		break;
	}
	incomment = NO;
    }
}

/*
 * Skip over comments and stop at the next character
 * position that is not whitespace.  This routine will span
 * lines, saving all but the current line in a buffer using
 * more_input().
 */
char *
eatcomment(cp)
register char *cp;
{
    do
    {
	cp = skipcomment(cp);
	if (incomment && (cp = more_input()) == NULL)
	{
	    error(CEOF_ERR, stqcline, 0);
	    exit(EXIT_ERROR);
	}
    } while (incomment);
}

/*
 * Skip over a quoted string or character and stop at the next
 * character position that is not whitespace.
 */
char *
skipquote (cp, type)
register char *cp;
register int type;
{
    register char qchar;

    qchar = type == QUOTE1 ? '\'' : '"';

    if (inquote[type])
	goto inside;

    for (;; cp++)
    {
	if (*cp != qchar)
	    return cp;
	cp++;
	if (!inquote[type])
	{
	    inquote[type] = YES;
	    stqcline = linenum;
	}
    inside:
	for (;; cp++)
	{
	    if (*cp == qchar)
		break;
	    if (*cp == '\0' || *cp == '\\' && *++cp == '\0')
		return cp;
	}
	inquote[type] = NO;
    }
}

/*
 *  special getlin - treats form-feed as an end-of-line
 *                   and expands tabs if asked for
 *
 */
int
getlin(line, maxline, inp, expandtabs)
register char *line;
int maxline;
FILE *inp;
int expandtabs;
{
    int tmp;
    register int num;
    register int chr;
#ifdef FFSPECIAL
    static char havechar = NO;	/* have leftover char from last time */
    static char svchar BSS;
#endif

    num = 0;
#ifdef FFSPECIAL
    if (havechar)
    {
	havechar = NO;
	chr = svchar;
	goto ent;
    }
#endif
    while (num + 8 < maxline)
    {				/* leave room for tab */
	chr = getc(inp);
	if (isprint(chr))
	{
#ifdef FFSPECIAL
	ent:
#endif
	    *line++ = chr;
	    num++;
	}
	else
	    switch (chr)
	    {
	    case EOF:
		return EOF;

	    case '\t':
		if (expandtabs)
		{
		    num += tmp = 8 - (num & 7);
		    do
			*line++ = ' ';
		    while (--tmp);
		    break;
		}
	    default:
		*line++ = chr;
		num++;
		break;

	    case '\n':
		*line = '\n';
		num++;
		goto end;

#ifdef FFSPECIAL
	    case '\f':
		if (++num == 1)
		    *line = '\f';
		else
		{
		    *line = '\n';
		    havechar = YES;
		    svchar = chr;
		}
		goto end;
#endif
	    }
    }
end:
    *++line = '\0';
    return num;
}

/*
 * save_line() --
 *   save the current line at the end of 'entire_expr_text'
 */
void
save_line(line)
char *line;
{
    static int buf_siz = 0;
    int line_len = strlen(line);
    int text_len;

    /*
     * First call, allocate space, copy the line and return.
     */
    if (buf_siz == 0)
    {
	buf_siz = line_len > MAXLINE - 20 ? line_len + 20 : MAXLINE;
	entire_expr_text = safe_malloc(buf_siz);
	strcpy(entire_expr_text, line);
	nsaved = 1;
	return;
    }

    /*
     * Copy line to end of existing buffer, growing the buffer first
     * if necessary.
     */
    if (nsaved == 0)
    {
	entire_expr_text[0] = '\0';
	text_len = 0;
    }
    else
	text_len = strlen(entire_expr_text);

    if (text_len + line_len >= buf_siz)
    {
	buf_siz = text_len + line_len + 20;
	entire_expr_text = safe_realloc(entire_expr_text, buf_siz);
    }

    /*
     * Keep track of the number of saved lines, so that flushline()
     * can print the right number of blank lines [-l feature].
     */
    nsaved++;

    strcpy(entire_expr_text + text_len, line);
}

char *
more_input()
{
    save_line(tline);

    if (getlin(tline, sizeof tline, input, NO) == EOF)
	return NULL;

    return tline;
}

static void
flushline(keep)
{
    if ((keep && reject < 2) ^ complement)
    {
	if (nsaved > 0)
	    fputs(entire_expr_text, stdout);
	fputs(tline, stdout);
    }
    else if (lnblank)
    {
	int i;

	for (i = nsaved; i >= 0; i--)
	    putchar('\n');
    }
    nsaved = 0;
}

static void
prname()
{
    fputs(progname, stderr);
    fputs(": ", stderr);
}


int
error(err, line, depth)
{
    if (err == END_ERR)
	return err;

    prname();

#ifdef TESTING
    fprintf(stderr, "Error in %s line %d: %s. ",
	    filename, line, errs[err]);
    fprintf(stderr, "ifdef depth: %d\n", depth);
#else
    fprintf(stderr, "Error in %s line %d: %s.\n",
	    filename, line, errs[err]);
#endif

    exitstat = EXIT_ERROR;
    return depth > 1 ? IEOF_ERR : END_ERR;
}

#ifdef ALL_SYMS
/*
 * add_symbol() --
 *    Add a symbol to the sym[], ignore[] and true[] arrays, assuming
 *    that it is either YES or NO (based on assume_defined and
 *    assume_undefined).
 */
int
add_symbol(scp)
char *scp;
{
    char *cp;
    int length;

    if (nsyms >= MAXSYMS)
    {
	prname();
	fprintf(stderr, "too many symbols in file.\n");
	return EXIT_ERROR;
    }
    ignore[nsyms] = NO;
    true[nsyms] = assume_defined ? YES : NO;

    /*
     * Find the end of the symbol name
     */
    for (cp = scp; *cp && !endsym(*cp); cp++)
	continue;

    /*
     * Make a copy of the symbol name
     */
    length = cp - scp;
    sym[nsyms] = (char *)malloc(length + 1);
    strncpy(sym[nsyms], scp, length);
    sym[nsyms][length] = '\0';

    return nsyms++;
}
#endif /* ALL_SYMS */

/*
 * Check if an identifier is known - returns YES, NO, or MAYBE
 */
int
lookup(id)
char *id;
{
    register int i;
    register char *c1, *c2;

    for (i = 0; i < nsyms; i++)
	for (c1 = id, c2 = sym[i]; *c1 == *c2; c1++, c2++)
	    if (*c1 == '\0')
		return true[i];

    /*
     * Not explicitly -Defined or -Undefined, return MAYBE ...
     */

#ifdef ALL_SYMS
    /*
     *  ... unless -u or -d have been specified.  In that case,
     *  add the symbol to the list and return non-zero or zero according
     *  to whether the assumption is everything defined or not defined,
     *  respectively.
     */
    if (assume_defined || assume_undefined)
	return true[add_symbol(id)];
#endif /* ALL_SYMS */

    return MAYBE;
}
