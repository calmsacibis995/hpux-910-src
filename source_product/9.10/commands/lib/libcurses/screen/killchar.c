/* @(#) $Revision: 27.1 $ */    
#include "curses.ext"

char
killchar()
{
#ifdef USG
	return cur_term->Ottyb.c_cc[VKILL];
#else
	return cur_term->Ottyb.sg_kill;
#endif
}
