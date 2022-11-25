/* @(#) $Revision: 27.1 $ */   
#include "curses.ext"

char
erasechar()
{
#ifdef USG
	return cur_term->Ottyb.c_cc[VERASE];
#else
	return cur_term->Ottyb.sg_erase;
#endif
}
