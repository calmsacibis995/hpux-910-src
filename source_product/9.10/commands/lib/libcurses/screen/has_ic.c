/* @(#) $Revision: 70.2 $ */      
#include "curses.ext"

/*
 * Does it have insert/delete char?
 */
has_ic()
{
	return insert_character || enter_insert_mode || parm_ich || delete_character || enter_delete_mode || parm_dch;
}

