/* @(#) $Revision: 62.1 $ */      
/*
 */

# include	"curses.ext"

#ifdef DEBUG
_sputc(c, f)
chtype c;
FILE *f;
{
	int so;

	so = c & A_ATTRIBUTES;
	c &= A_CHARTEXT;
	if (so) {
		putc('<', f);
		fprintf(f, "%o,", so);
	}
	putc(c, f);
	if (so)
		putc('>', f);
}
#endif
