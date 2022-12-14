/* @(#) $Revision: 64.1 $ */    
# include	"curses.ext"
# include	<signal.h>

/*
 *	mini.c contains versions of curses routines for minicurses.
 *	They work just like their non-mini counterparts but draw on
 *	std_body rather than stdscr.  This cuts down on overhead but
 *	restricts what you are allowed to do - you can't get stuff back
 *	from the screen and you can't use multiple windows or things
 *	like insert/delete line (the logical ones that affect the screen).
 *	All this but multiple windows could probably be added if somebody
 *	wanted to mess with it.
 *
 */

/*
 *	This routine adds a string starting at (_cury,_curx)
 *
 */
m_addstr(str)
register char	*str;
{
# ifdef DEBUG
	if(outf) fprintf(outf, "M_ADDSTR(\"%s\")\n", str);
# endif
	while (*str)
		if (m_addch((chtype)(unsigned char) *str++) == ERR)
			return ERR;
	return OK;
}
