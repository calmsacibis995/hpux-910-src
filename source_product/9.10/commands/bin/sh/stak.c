/* @(#) $Revision: 72.1 $ */      
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"


/* ========	storage allocation	======== */

tchar *	
getstak(asize)			/* allocate requested stack */
int	asize;
{
	register tchar	*oldstak;
	register int	size;

	size = round(asize, BYTESPERWORD);
	oldstak = stakbot;
	staktop = stakbot += size;
	sizechk(staktop);
	return(oldstak);
}

#if defined(hp9000s200)
growstak(x)
tchar	*x;
{
	while ( brkend <= (x+1) )
		if (setbrk(brkincr) == -1)
			error(nl_msg(604,nostack));
	if (brkincr < BRKMAX)
		brkincr += 256;
}
#endif hp9000s200

/*
 * set up stack for local use
 * should be followed by `endstak'
 */
tchar *	
locstak()
{
#if defined(hp9000s200)
	/*
	 * Careful!	sizechk(x) is defined to <null>
	 *         	on most systems, so use it only
	 *		if it makes sense.
	 */
	sizechk(stakbot + BRKINCR);
#else
	if (brkend - stakbot < BRKINCR)
	{
		if (setbrk(brkincr) == -1)
			error(nl_msg(604,nostack));
		if (brkincr < BRKMAX)
			brkincr += 256;
	}
#endif hp9000s200
	return(stakbot);
}

tchar *	
savstak()
{
	assert(staktop == stakbot);
	return(stakbot);
}

tchar *
endstak(argp)		/* tidy up after `locstak' */
register tchar	*argp;
{
	register tchar	*oldstak;

	*argp++ = 0;
	oldstak = stakbot;
	stakbot = staktop = (tchar *)round(argp, BYTESPERWORD);
	return(oldstak);
}

tdystak(x)		/* try to bring stack back to x */
register tchar	*x;
{
	while ((tchar *)(stakbsy) > (tchar *)(x))
	{
		free(stakbsy);
		stakbsy = stakbsy->word;
	}
	staktop = stakbot = max((tchar *)(x), (tchar *)(stakbas)); 
	rmtemp(x);
}

stakchk()
{
	while ((brkend - stakbas) > BRKINCR + BRKINCR)
		setbrk(-BRKINCR);
}

tchar *		
cpystak(x)
tchar	*x;
{
	sizechk(stakbot + tlength(x));	
	return(endstak(tmovstr(x, locstak())));
}
