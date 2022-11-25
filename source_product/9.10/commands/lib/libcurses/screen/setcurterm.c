/* @(#) $Revision: 64.1 $ */    
#include "curses.ext"

extern	struct	term *cur_term;

set_curterm(nterm)
struct	term	*nterm;
{
	cur_term = nterm;
}
