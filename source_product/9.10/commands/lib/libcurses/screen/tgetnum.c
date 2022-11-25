/* @(#) $Revision: 66.1 $ */     
/*
 * Simulation of termcap using terminfo.
 */

#include "curses.ext"


/* Make a 2 letter code into an integer we can switch on easily */
#define	two( s1, s2 )	(s1 + 256 * s2 )
#define	twostr( str )	two( *str, str[ 1 ] )

int
tgetnum(id)
char *id;
{
	int rv;

	switch (twostr(id)) {
	case two('c','o'): rv = columns; break;
	case two('i','t'): rv = init_tabs; break;
	case two('l','i'): rv = lines; break;
	case two('l','m'): rv = lines_of_memory; break;
	case two('s','g'): rv = magic_cookie_glitch; break;
	case two('p','b'): rv = padding_baud_rate; break;
	case two('v','t'): rv = virtual_terminal; break;
	case two('w','s'): rv = width_status_line; break;
 	case two('N','l'): rv = num_labels; break;
 	case two('l','h'): rv = label_height; break;
 	case two('l','w'): rv = label_width; break;
	default: rv = -1;
	}
	return rv;
}
