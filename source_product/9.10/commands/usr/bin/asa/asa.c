static char *HPUX_ID = "@(#) $Revision: 66.1 $";

/*
 *	asa - interpret asa carriage control characters
 *
 *	This program is designed to make sense out of the output
 *	of fortran programs whose authors have used asa carriage
 *	control characters.  "asa" processes either the files
 *	whose names are given as arguments or the standard input
 *	if no file names are given.  "asa" processes each input files
 *	and produces the results to the standard output.
 *	The first character of each line is assumed to be a control
 *	character: the meanings of the control characters are:
 *
 *	' '	single-space before printing
 *	'0'	double-space before printing
 *	'-'	triple-space before printing
 *	'1'	new page before printing
 *	'+'	do not space at all before printing
 *
 *	A line beginning with '+' will overprint the previous line.
 *
 *	Lines beginning with other than the above characters are
 *	treated as if they began with ' '.  The first characters
 *	of such lines will be replaced by ' '.
 *	If any such lines appear,
 *	an appropriate diagnostic will appear on the standard error
 *	file after processing each file.  The return code reflects
 *	the error conditions after processing:
 *		0	if no error
 *		-1	output error
 *		>0	It shows there is no output error and
 *			there are some input files which cannot
 *			be opened. The return code is the total number
 *			of input files which cannot be opened.
 *
 *	The program forces the first line of each file to
 *	start on a new page.  This program requires the printer
 *	be able to perform the formfeed character.
 *
 *	example:
 *		asa [-s] [files]
 *	-s will suppress the error messages.
 *	if there are no files the input file defaults to stdin.
 */
#include <stdio.h>

char *pgmname;          /* program name, for diagnostics */
long badlines = 0;      /* count of lines with bad control characters */
int retcode = 0;        /* number of errors detected */
int prnt = 1;           /* mask for error message */
FILE *f;

main (argc, argv)
int argc;
char *argv[];
{
    extern char *strrchr();
    static char *stdin_name = "standard input";
    register int i;
    char c;
    extern int optind;

    if ((pgmname = strrchr(argv[0], "/")) == NULL)
	pgmname = argv[0];
    else
	pgmname++;

    while ((c=getopt(argc,argv,"s"))!=EOF)
	switch(c) {
	    case 's':prnt=0;break;
	}

    /* no arguments? */
    if (optind == argc) {
	f=stdin;
	dofile(stdin_name);
    } else {
	/*
	 * one iteration per input file
	 */
	i=optind;
	for (; i < argc; i++) {
	    if (strcmp(argv[i],"-")==0) {
		f=stdin;
		dofile(stdin_name);
	    } else if ((f=fopen(argv[i], "r")) == NULL) {
		fputs(pgmname, stderr);
		fputs(": cannot open ", stderr);
		fputs(argv[i], stderr);
		fputc('\n', stderr);
		retcode++;
	    }
	    else
		dofile(argv[i]);
	}
    }

    /*
     * Check if any error on output stream
     */
    if (ferror(stdout)) {
	fputs(pgmname, stderr);
	fputs(": output error\n", stderr);
	retcode = -1;
    }

    exit(retcode);
}

/*
 * dofile - process the standard input.
 *
 *  This routine is called once for each input file.
 *  The "fname" argument is used
 *  for writing diagnostic messages only.
 */
dofile (fname)
char *fname;
{
    register int c;
    register int firstchar;	/* flag to show first char in a line */

    badlines = 0;
    firstchar = 1;
lff:
    putchar('\f');		/* start with a new page */
    c = getc(f);
    while (c != EOF)
    {
	if (firstchar)
	{
	    firstchar = 0;
	    switch (c)
	    {
		/* new page */
	    case '\f': 
	    case '1': 
		putchar('\n');
		putchar('\f');
		break;

		/* triple space */
		/* this is not in the spec! */
	    case '-': 
		putchar('\n');

		/* double space */
	    case '0': 
		putchar('\n');

		/* single space */
	    case ' ': 
		putchar('\n');
		break;
	    case '\n': 
		firstchar = 1;
		putchar('\n');
		break;

		/* no space at all */
	    case '+': 
		putchar('\r');
		break;
	    default: 
		badlines++;
		putchar('\n');
	    }
	}
	else
	{
	    switch (c)
	    {
	    case '\f': 
		putchar('\n');
		putchar('\f');
		break;
	    case '\n': 
		firstchar = 1;
		break;
	    default: 
		putchar(c);
		break;
	    }
	}
	c = getc(f);
    }
    putchar('\n');
    if (f != stdin) /* don't want to close stdin! */
    	fclose(f);

    /*
     * report invalid input lines -- dofile increments badlines
     */
    if (badlines && prnt)
    {
	extern char *ltoa();

	fputs(pgmname, stderr);
	fputs(": ", stderr);
	fputs(ltoa(badlines), stderr);
	fputs(" invalid input lines in ", stderr);
	fputs(fname, stderr);
	fputc('\n', stderr);
    }
}
