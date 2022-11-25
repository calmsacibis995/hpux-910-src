/* @(#) $Revision: 27.1 $ */      
#include "curses.ext"

/*
 * Queries: does the terminal have insert/delete line?
 */
has_il()
{
	return insert_line && delete_line || change_scroll_region;
}
