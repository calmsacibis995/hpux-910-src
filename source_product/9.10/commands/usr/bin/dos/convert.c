static char *HPUX_ID = "@(#)convert.c	1.2 10/02/86 11:23:40";

/*
 * Convert text to HP-UX format or to DOS format.
 *
 * Accepts a list of filenames to convert.  All output goes to stdout.
 * If a file name is specified as "-" or not filenames are given, it
 * will convert stdin.
 *
 * If the last file processed cannot be opened, the return value will be
 * 2, otherwise the return value will be 0.
 *
 * Input text can be in either DOS or HP-UX format.  Carriage returns
 * (ascii 13) are skipped.  Control-Z (ascii 26) terminates processing
 * that file.  If converting to DOS format, new-lines (ascii 10) are
 * changed to new-line followed by carriage return (ascii 13), and
 * control-Z (ascii 26) is appended to the end of the file.
 *
 * REVISION HISTORY
 * -------- -------
 * 861002 Larry Fenske	fix for HP-UX
 *			changed program names in comments
 *			changed FROMUX to TODOS
 * 860924 Larry Fenske	originally coded
 */

/*
 * In HP-UX:
 *   for "dos2ux" compile with "cc         -O convert.c -o dos2ux"
 *   for "ux2dos" compile with "cc -DTODOS -O convert.c -o ux2dos"
 * In DOS the Lattice C compiler is required because of the method of
 *   opening files in raw mode.
 *   for "dos2ux" compile with "lc         -odos2ux.obj convert.c"
 *     link with "link \lc\s\c dos2ux,dos2ux,,\lc\s\lcm+\lc\s\lc"
 *   for "ux2dos" compile with "lc -dTODOS -oux2dos.obj convert.c"
 *     link with "link \lc\s\c ux2dos,ux2dos,,\lc\s\lcm+\lc\s\lc"
 */

#include "stdio.h"

int _fmode = 0x8000;			/* LC: do all opens in raw mode */

int retvalue;	/* return value of main: 0=success, 2=failure of last file */

void translate(fp)
FILE *fp;
{
    int c;

    while ( (c = getc(fp)) != EOF ) {
	if      (c == '\r')	/* CR */
	    ;			/* ignore CR's */
#ifdef TODOS
	else if (c == '\n') {	/* end of line */
	    putchar('\r');	/* add CR and */
	    putchar('\n');	/*   LF */
	}
#endif
	else if (c == 26)	/* control Z */
	    break;		/* exit loop */
	else			/* anything else */
	    putchar(c);		/* pass on through */
    }
    retvalue = 0;
}


int main (argc, argv)
int argc;
char *argv[];
{
    FILE *fp;
    char *progname;

    progname = *argv;
    retvalue = 2;
    if (argc == 1) translate(stdin);	/* only arg is program name */
    else {
	argc--;			/* don't count program name */
	do {
	    argv++;		/* first time skip past program name */
	    if (strcmp(*argv, "-") == 0) {
		translate(stdin);
	    } else {
		if ( (fp = fopen(*argv, "r")) == NULL) {
		    fputs(progname, stderr);
		    fputs(": cannot open ", stderr);
		    fputs(*argv, stderr);
		    fputs("\n", stderr);
		    retvalue = 2;
		}
		else translate(fp);
	    }
	} while (--argc);
    }
#ifdef TODOS
    putchar(26);	/* append control Z to the end */
#endif
    return (retvalue);
}
