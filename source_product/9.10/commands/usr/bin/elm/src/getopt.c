/**			getopt.c			**/

/*
 *  @(#) $Revision: 64.2 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  starting argument parsing routine... 
 */


#include "defs.h"

#ifndef NULL
# define NULL		0
#endif

#define DONE		0
#define ERROR		-1


char           *optional_arg;	/* optional argument as we go */
int             opt_index = 0;	/* argnum + 1 when we leave   */


/*
 *
 * Typical usage of this routine is exemplified by;
 *
 *	register int c;
 *
 *	while ((c = get_options(argc, argv, "ad:f:")) > 0) {
 *	   switch (c) {
 *	     case 'a' : arrow_cursor++;		break;
 *	     case 'd' : debug = atoi(optional_arg);	break;
 *	     case 'f' : strcpy(infile, optional_arg); 
 *	                mbox_specified = 2;  break;
 *	   }
 *	}
 *
 *	if (c == ERROR) {
 *	  printf("Usage: %s [a] [-d level] [-f file] <names>\n\n", argv[0]);
 *	  exit(1);
 *	}
 *
 */


int             _indx = 1, 
		_argnum = 1;


int
get_options(argc, argv, options)

	int             argc;
	char            *argv[], 
			*options;

{
	/*
	 *  Returns the character argument next, and optionally instantiates 
	 *  "argument" to the argument associated with the particular option 
	 */


	char            *word, 
			*strchr();


	if ( _argnum >= argc ) {		/* quick check first - no arguments! */
		opt_index = argc;
		return ( DONE );
	}

	if ( _indx >= strlen(argv[_argnum]) && _indx > 1 ) {
		_argnum++;
		_indx = 1;			/* zeroeth char is '-' */
	}

	if ( _argnum >= argc ) {
		opt_index = _argnum;		/* no more args */
		return ( DONE );
	}

	if ( argv[_argnum][0] != '-' ) {
		opt_index = _argnum;
		return ( DONE );
	}

	word = strchr( options, argv[_argnum][_indx++] );

	if ( word == NULL )
		return ( ERROR );		/* Sun compatibility */

	if ( word == NULL || strlen(word) == 0 )
		return ( ERROR );

	if ( word[1] == ':' ) {

		/*
		 *  Two possibilities - either tailing end of this argument 
		 *  or the next argument in the list 
		 */

		if ( _indx < strlen(argv[_argnum]) ) {	/* first possibility */
			optional_arg = (char *) (argv[_argnum] + _indx);
			_argnum++;
			_indx = 1;

		} else {				/* second choice     */
			if ( ++_argnum >= argc )
				return ( ERROR );	/* no argument!!     */

			optional_arg = (char *) argv[_argnum++];
			_indx = 1;
		}
	}

	return ( (int) word[0] );
}
