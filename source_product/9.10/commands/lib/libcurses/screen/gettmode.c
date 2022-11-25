/* @(#) $Revision: 27.1 $ */    
# include	"curses.ext"
# include	<signal.h>

char	*calloc();
char	*malloc();
extern	char	*getenv();

extern	WINDOW	*makenew();

gettmode()
{
	/* No-op included only for upward compatibility. */
}
