/* @(#) $Revision: 72.1 $ */      

/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	"sym.h"

#ifdef NLS
extern	int	_nl_space_alt;
#define	ALT_SP	(_nl_space_alt & STRIP)
#endif


/* ========	character handling for command lines	========*/

word()
{
	register tchar	c, d;
	struct argnod	*arg = (struct argnod *)locstak();
	register tchar	*argp = arg->argval;
	int		alpha = 1;

	wdnum = 0;
	wdset = 0;

	while (1)
	{
#ifndef NLS
		while (c = nextc(0), space(c))		/* skipc() */
#else
		while (c = nextc(0), space(c) || c == ALT_SP)		/* skipc() */
#endif
			;

		if (c == COMCHAR)
		{
			while ((c = readc()) != NL && c != EOF);
			peekc = c;
		}
		else
		{
			break;	/* out of comment - white space loop */
		}
	}
#ifndef NLS
	if (!eofmeta(c))
#else
	if (!eofmeta(c) && c != ALT_SP)
#endif
	{
		do
		{
			if (c == LITERAL)
			{
				sizechk(argp+1);
				*argp++ = (DQUOTE);
				while ((c = readc()) && c != LITERAL)
				{
					*argp++ = (c | QUOTE);
					if (c == NL)
						chkpr();
					sizechk(argp);
				}
				*argp++ = (DQUOTE);
			}
			else
			{
				sizechk(argp+1);
				*argp++ = (c);
				if (c == '=')
					wdset |= alpha;
				if (!alphanum(c))
					alpha = 0;
				if (qotchar(c))
				{
					d = c;
					while ((*argp++ = (c = nextc(d))) && c != d)
					{
						if (c == NL)
							chkpr();
						sizechk(argp);
					}
				}
			}
#ifndef NLS
		} while ((c = nextc(0), !eofmeta(c)));
#else
		} while ((c = nextc(0), !eofmeta(c) && c != ALT_SP));
#endif
		sizechk(argp);
		argp = endstak(argp);
		if (!letter(arg->argval[0]))
			wdset = 0;

		peekn = c | MARK;
		if (arg->argval[1] == 0 && 
		    (d = arg->argval[0], digit(d)) && 
		    (c == '>' || c == '<'))
		{
			word();
			wdnum = d - '0';
		}
		else		/*check for reserved words*/
		{
			if (reserv == FALSE || (wdval = syslook(arg->argval,reserved, no_reserved)) == 0)
			{
				wdarg = arg;
				wdval = 0;
			}
		}
	}
	else if (dipchar(c))
	{
		if ((d = nextc(0)) == c) 
		{
			wdval = c | SYMREP;
			if (c == '<')
			{
				if ((d = nextc(0)) == '-') 
					wdnum |= IOSTRIP;
				else {
					peekn = d | MARK;
				}
			}
		}
		else
		{
			peekn = d | MARK;
			wdval = c;
		}
	}
	else
	{
		if ((wdval = c) == EOF)
			wdval = EOFSYM;
		if (iopend && eolchar(c))
		{
			copy(iopend);
			iopend = 0;
		}
	}
	reserv = FALSE;
	return(wdval);
}

skipc()
{
	register tchar c;

#ifndef NLS
	while (c = nextc(0), space(c))
#else
	while (c = nextc(0), space(c) || c == ALT_SP)
#endif
		;
	return(c);
}

nextc(quote)
tchar	quote;		
{
	register tchar	c, d;

retry:
	if ((d = readc()) == ESCAPE)
	{
		if ((c = readc()) == NL)
		{
			chkpr();
			goto retry;
		}
		else if (quote && c != quote && !escchar(c))
			peekc = c | MARK;
		else
			d = c | QUOTE;
	}
	return(d);
}


readc()
{
	register tchar	c;
	register int	len;
	register struct fileblk *f;

	if (peekn)
	{
		peekc = peekn;
		peekn = 0;
	}
	if (peekc)
	{
		c = peekc;
		peekc = 0;
		return(c);
	}
	f = standin;
retry:
	if (f->fnxt != f->fend)
	{
 		if ((c = (tchar)*f->fnxt++) == 0) 
		{
			if (f->feval)
			{
				if (estabf(*f->feval++))
					c = EOF;
				else
					c = SP;
			}
			else
				goto retry;	/* = c = readc(); */
		}
		if (flags & readpr && standin->fstak == 0)
			prct(c);
		if (c == NL)
			f->flin++;
	}
	else if (f->feof || f->fdes < 0)
	{
		c = EOF;
		f->feof++;
	}
	else if ((len = readb()) <= 0)
	{
		close(f->fdes);
		f->fdes = -1;
		c = EOF;
		f->feof++;
	}
	else
	{
		f->fend = (f->fnxt = f->fbuf) + len;
		goto retry;
	}
	return(c);
}

static
readb()
{
	register struct fileblk *f = standin;
	int	len;
#ifdef NLS
	tchar *tp;
#endif NLS

	/* Fix for boundary conditions. Cannot lseek on a pipe, so read
	 * lesser number of bytes :DSDe410617:
	 */
	f->fsiz = (f->fsiz == BUFSIZ) ? BUFSIZ - 1 : f->fsiz;
	do
	{
		if (trapnote & SIGSET)
		{
			if(!(flags & noprompt) && !(flags & sigwtrap))
			    newline();
			sigchk();
		}
		else if ((trapnote & TRAPSET) && (rwait > 0))
		{
			newline();
			chktrap();
			clearup();
		}
	} while ((len = read(f->fdes, (char *)f->fbuf, f->fsiz)) < 0 && trapnote);
#ifdef NLS
	/*
	 * Must be to_short here, we can't have any allocation, as word
	 * will reset the stakbot so that stakbot < stakbas.  DISASTER!
	 */
    {
	extern tchar *to_tcharn();
	unsigned char *trash = (unsigned char *)f->fbuf;
	int i=0;
	if (len == -1) return(len);
	/*
	 * if boundary char is a quote char need to leave it in the 
	 * input stream  
	 * if boundary char is a 2nd byte of a 2 byte character back
	 * up 1 byte --
	 * DSDe410617: dont back up instead read one more byte!! cannot back
	 * up on a pipe!
	 */
	if (len == f->fsiz) {	/* DSDe410617: check against size */
		/* check if last character is a 2-byte character 
		 * To do this we need to check all chars in buffer - there
		 * is no way to find if a particular character is First_of_2
		 * unless we start from the beginning!!
		 */
		while (i < len) {
			if (FIRSTof2(trash[i]))
				i+=2;
			else
				i++;
		}
/*		if (i>len) {		DSDe410617: cannot lseek on a pipe.
			len--;
			lseek(f->fdes,-1L,1);
		}
*/
		if (i > len) {
			/* read one more byte! -assume 2-byte chars */
			if(read(f->fdes, &trash[len], 1) < 0)
				return (-1);	/* No SECof2: settle for EOF */
			len++;
			f->fsiz++;
		}
		/* this makes no sense - so i'am commenting it out!
		 * should have been (trash[len-1] & 0x80) perhaps - see
		 * rev 51.2 and 56.1   changed at DSDe410617
		while (len != 0 && (trash[len-1] == 0x80)) {
			len--;
			lseek(f->fdes, -1L, 1);
		}
		*/
	}
	/*
	 * The other condition to check is when we are reading from a device
	 * on which we cannot seek (f->fsiz should be 1), and we have read
	 * the first of a 2-byte character.  We have to read the second
	 * byte here, otherwise it may not be picked up properly at the next
	 * call (e.g. the case where SECof2 is "\".)
	 * This fixes defect STARS# 5000-681858.
	 */
	else if (len==1 && f->fsiz==1 && FIRSTof2(trash[0])) {
		if ((len=read(f->fdes,&trash[1],1)) == -1)
			return (-1);	/* No SECof2: settle for EOF */
		len=2;
	}
	trash[len] = '\0';
	tp = to_tcharn((char *)f->fbuf,&len);
	memcpy((char *)f->fbuf, (char *)tp, len*CHARSIZE);
    }
#endif NLS
	return(len);
}

