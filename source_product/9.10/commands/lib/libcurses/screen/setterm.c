/* @(#) $Revision: 27.1 $ */     
# include	"curses.ext"
# include	<signal.h>

char	*calloc();
char	*malloc();
extern	char	*getenv();

extern	WINDOW	*makenew();

/*
 * Low level interface, for compatibility with old curses.
 */
setterm(type)
char *type;
{
	setupterm(type, 1, 0);
}
