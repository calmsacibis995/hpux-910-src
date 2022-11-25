/* @(#) $Revision: 64.1 $ */    
#include "curses.ext"

extern	struct	term _first_term;

del_curterm(oterm)
struct	term	*oterm;
{
	if( !(oterm == &_first_term) )
	{
		cfree((struct term *) oterm);
	}
}
