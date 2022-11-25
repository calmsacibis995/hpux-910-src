static char *HPUX_ID = "@(#) $Revision: 70.1 $";
/*
 * put tabs into a file replacing blanks
 */
#include <stdio.h>
#include <string.h>
#include <nl_types.h>

#define NUM_TABS 100		/* number of tab positions available */
#define NL_SETN 1			/* message catalog number */

char	genbuf[BUFSIZ];
char	linebuf[BUFSIZ];
int		all;

extern char	*optarg;
extern int 	optind, opterr;

nl_catd	catd;		/* message catalog file */

/****************************************************************************
Function tabify.  This function adds tabs to the output line where needed.
The tab locations are stored in the tabs array.

All input char are copied to the output string.  If a sequence of spaces
and/or backspaces is encountered the starting position is remembered.  If
that sequence could be replaced by a tab char. it is replaced when the
tab position is reached.  In other words the output string is patched
with a tab character if needed.  Any backspace char. in the sequence are
put in the output string before the tab.  
*/

void
tabify( inline, tabs, all, outline)
char *inline;		/* the input line */
int tabs[];			/* the tabs array */
int all;			/* -a flag */
char * outline;		/* the output line */

{

int		ocolumn = 0;		/* virtual output column */
char 	*iptr = inline;		/* input char. pointer */
int		sp_count = 0;		/* number of consecutive tabs & backspace char */
int		write_tab = 0;		/* boolean, 1= plan on writing a tab  */
int 	tab_pos = 0;		/* index in outline where to put tab */
int 	oindex = 0;			/* index in outline */
int		back_sp = 0;		/* number of backspace char. to write */
int		tabindex = 0;		/* index into the tabs array */
int		non_sp = 0;			/* number of non space or backsp char found */


while ( *iptr != '\0' ) {
	outline [ oindex ] = *iptr;

	if ( *iptr == ' ' || *iptr == '\b' || *iptr == '\t') {
		sp_count++;

		if ( ! write_tab ) {
			write_tab = 1;
			tab_pos = oindex;
			}

		if ( *iptr == '\b' ) {
			back_sp++;
			ocolumn--;
			if ( ocolumn < 1 )		/* should this be zero */
				ocolumn = 1;	
			}
		else
			ocolumn++;
		}
	else {
		write_tab = 0;
		sp_count = 0;
		back_sp+ 0;
		ocolumn++;
		non_sp++;
		}

	if ( *iptr == '\t' ) 
		ocolumn = tabs [ tabindex ];
	
	if ( ocolumn == tabs [ tabindex ] ) {
		tabindex++;
		if ( write_tab && (sp_count > 1) ) {		/* write \b and tab */
			for ( oindex = tab_pos; back_sp != 0; oindex++ ) {
				outline [ oindex ] = '\b';
				back_sp--;
				}
			
			outline [ oindex ] = '\t';
			write_tab = 0;
			sp_count = 0;
			back_sp = 0;
			}

		if ( ( ! all && non_sp ) || ( tabs [ tabindex ] == 0 ) ) {
			/* convert only leading blanks, or, done if out of tab stops */
			strcpy ( &outline [ oindex + 1 ], ++iptr );
			return;
			}
		}

	oindex++;
	iptr++;
	}	/* while *iptr != 0 */

outline [ oindex ] = '\0';
}
		

/************************************************************************
Function load_tabs.  This function reads the tab stops from the input
string and puts the values into the tabs array.  If only a single 
number is specified tabs are set up at n, 2*n, 3*n,,,.
*/

void load_tabs( arg, tabs )
char *arg;
int tabs[];
{
int	x = 0;
char *ptr;
int	last = 0;

ptr = strtok ( arg, " ," );

do {
	tabs [ x ] = atoi ( ptr );
	if ( tabs [ x ] <= 0 ) {
		fputs ( (catgets(catd,NL_SETN,1, "bad tab specification\n")), stderr );
		exit ( 1 );
		}

	if ( last >= tabs [ x ] ) {
		fputs ( (catgets(catd,NL_SETN,2, "tabs out of order\n")), stderr );
		exit ( 1 );
		}
	else {
		last = tabs [ x ];
		if ( x == NUM_TABS - 1 ) {
			fputs ( (catgets(catd,NL_SETN,3, "too many tabs specified\n")), stderr );
			exit ( 1 );
			}
		}		

	x++;
	ptr = strtok ( NULL, " ," );
	}
while ( ptr != NULL );


/* terminate tabs array with zero and fill with succeeding tabs if only a 
   single tab was specified.
*/
if ( x > 1 ) 
	tabs [ x ] = 0;
else {
	for ( x = 1; x < NUM_TABS - 1 ; x++ )
		tabs [ x ] = tabs [ 0 ] * ( x + 1 );
	tabs [ NUM_TABS - 1 ] = 0;
	}
}


/************************************************************************/
void 
usage()
{
fputs((catgets(catd,NL_SETN,4, "usage: unexpand [ -a ] [ -t tablist ] file ...\n")), stderr);
exit(1);
}
/************************************************************************/

main(argc, argv)
	int argc;
	char *argv[];
{
	register char *cp;

	int optc;
	int tabstops [ NUM_TABS ];
	int t = 0;		/* set to 1 when -t option specified */

	catd = catopen ( "unexpand", 0 );	/* open nls message catalog */

	while (( optc = getopt ( argc, argv, "at:" ) ) != EOF )
		switch ( optc ) {
			case 'a':	all++;
						break;
			case 't':	all++;
						t++;
						load_tabs ( optarg, tabstops );
						break;
			default:	usage();
			}

	if ( ! t )		/* use default of 8 if -t not specified */
		load_tabs ( "8", tabstops );

	
	do {
		if ( optind < argc ) {
			if (freopen( argv[ optind ], "r", stdin ) == NULL) {
				perror(argv[optind]);
				exit(1);
				}
			optind++;
			}

		while (fgets(genbuf, BUFSIZ, stdin) != NULL) {
			tabify( genbuf, tabstops, all, linebuf );
			fputs(linebuf, stdout);
			}
		}
	while ( optind < argc );

	exit(0);
}

		
