/* @(#) $Revision: 66.1 $ */   
/****************************************************************************

	DEBUGGER - input routines

****************************************************************************/
#include "defs.h"

CHAR		linebuf[LINSIZ];
FILE		*infile;
FILE		*outfile;
CHAR		*lp;
CHAR		lastc = EOR;
int		eof;

/* input routines */

eol(c)
char	c;
{
	return(c == EOR || c == ';');
}

rdc()
{
	do (readchar());
	while (lastc == SPACE || lastc == TB);
	return(lastc);
}

readchar()
{
	if (eof) lastc = NULL;
	else
	{
		if (lp == 0)
		{
			lp = linebuf;

#ifdef KSH
			if (infile == stdin)
			{
				hist_gets(lp);
				while (*lp++);
				*--lp = EOR;
				*++lp = 0;
				lp = linebuf;
			}
			else
#endif KSH
			{
			 do (eof = (fread(lp, 1, 1, infile) != 1));
			 while (eof == 0 && *lp++ != EOR);
			 *lp = 0; lp = linebuf;
			}
		}
		if (lastc = *lp) lp++;
	}
	return(lastc);
}

nextchar()
{
	if (eol(rdc()))
	{
		lp--; return(0);
	}
	else return(lastc);
}

quotchar()
{
	if (readchar() == '\\')	return(readchar());
	else if (lastc == '\'')	return(0);
	else return(lastc);
}

getformat(deformat)
char	*deformat;
{
	register char	*fptr = deformat;	/* MFM */
	int	quote = FALSE;

	while (quote ? readchar() != EOR : !eol(readchar()))
		if ((*fptr++ = lastc) == '"') quote = ~quote;
	lp--;
	if (fptr != deformat) *fptr++ = '\0';
}


