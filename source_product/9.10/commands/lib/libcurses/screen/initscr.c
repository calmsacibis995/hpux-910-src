/* @(#) $Revision: 66.1 $ */     
# include	"curses.ext"
# include	<signal.h>
# include	<nl_ctype.h>
# include	<locale.h>

char	*calloc();
char	*malloc();
extern	char	*getenv();

extern	WINDOW	*makenew();

/*
 *	This routine initializes the current and standard screen.
 *
 */

WINDOW *
initscr()
{
	register char *sp;
	struct screen *scp;
	extern char *_c_why_not;
	
# ifdef DEBUG
	if (outf == NULL) {
		if( ( outf = fopen("trace", "w") ) == NULL)
		{
			perror("trace");
			exit(-1);
		}
	}
#endif

	if( ( sp = getenv( "TERM" ) ) == NULL )
	{
		sp = Def_term;
	}

	setlocale(LC_ALL, "");		/* set locale to user's defined */

# ifdef DEBUG
	if(outf) fprintf(outf, "INITSCR: term = %s\n", sp);
# endif
	if( ( scp = newterm( sp, stdout, stdin ) ) == NULL )
	{
		_ec_quit(_c_why_not, sp);
	}
	return stdscr;
}
