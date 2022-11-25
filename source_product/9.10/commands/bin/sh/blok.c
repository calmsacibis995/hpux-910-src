/* @(#) $Revision: 66.1 $ */      
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */


#include	"defs.h"

#ifdef NLS
#define NL_SETN 1
#endif


/*
 *	storage allocator
 *	(circular first fit strategy)
 */

#define BUSY 01
#define busy(x)	(Rcheat((x)->word) & BUSY)

unsigned	brkincr = BRKINCR;
struct blk *blokp;			/*current search pointer*/
struct blk *bloktop;		/* top of arena (last blok) */

char		*brkbegin;
char		*setbrk();

tchar *				
alloc(nbytes)
	unsigned nbytes;
{
	register unsigned rbytes = round(nbytes+BYTESPERWORD, BYTESPERWORD);


	for (;;)
	{
		int	c = 0;
		register struct blk *p = blokp;
		register struct blk *q;
		do
		{
			if (!busy(p))
			{
				while (!busy(q = p->word))
					p->word = q->word;
				if ((char *)q - (char *)p >= rbytes)
				{
					blokp = (struct blk *)((char *)p + rbytes);
					if (q > blokp)
						blokp->word = p->word;
					p->word = (struct blk *)(Rcheat(blokp) | BUSY);
					return((tchar *)(p + 1));
				}
			}
			q = p;
			p = (struct blk *)(Rcheat(p->word) & ~BUSY);
		} while (p > q || (c++) == 0);
		addblok(rbytes);
	}
}

addblok(reqd)
	unsigned reqd;
{
	if (stakbot == 0)
	{
		brkbegin = setbrk(3 * BRKINCR);
		bloktop = (struct blk *)brkbegin;
	}

	if (stakbas != staktop)
	{
		register char *rndstak;
		register struct blk *blokstak;

		pushstak(0);
		rndstak = (char *)round(staktop, BYTESPERWORD);
		blokstak = (struct blk *)(stakbas) - 1;
		blokstak->word = stakbsy;
		stakbsy = blokstak;
		bloktop->word = (struct blk *)(Rcheat(rndstak) | BUSY);
		bloktop = (struct blk *)(rndstak);
	}
	reqd += brkincr;
	reqd &= ~(brkincr - 1);
	blokp = bloktop;
	bloktop = bloktop->word = (struct blk *)(Rcheat(bloktop) + reqd);
#ifndef NLS
	sizechk((char *)bloktop + sizeof(struct blk));
#else NLS
	sizechk((tchar *)(&bloktop[1]));
#endif NLS
	bloktop->word = (struct blk *)(brkbegin + 1);
	{
		register tchar *stakadr = (tchar *)(bloktop + 2);

		if (stakbot != staktop)
		{
#ifndef NLS
			sizechk(stakadr + length(stakbot));
#else NLS
			/*
			 * We don't know if it's a char * or a tchar *
			 * so ... pass the max of length(stakbot)
			 * and tlength((tchar *)stakbot)
			 */
			int c_len, t_len;

			c_len = length(stakbot);
			t_len = tlength(stakbot);
			sizechk(stakadr + (c_len > t_len ? c_len : t_len));
#endif NLS
			staktop = tmovstr(stakbot, stakadr);
		}
		else
		{
			sizechk(stakadr);
			staktop = stakadr;
		}

		stakbas = stakbot = stakadr;
	}
}

tchar *				
realloc(ptr, nbytes)
	tchar *ptr;
	unsigned int nbytes;
{
	register unsigned int size;
	register struct blk *p;
	register struct blk *q;
	tchar *cp;
	unsigned int osize;

	/*
	 * Make p point to the "control byte" of the memory chunk.
	 * If this is an allocated block, then free it and set the
	 * "current block" pointer to the newly free-ed block.
	 */
	p = ((struct blk *)ptr) - 1;
	if (busy(p))
	{
		free(ptr);
		blokp = p;
	}

	/*
	 * Calculate the size of the block and allocate a chunk of
	 * the new size.  We must get "osize" BEFORE the malloc, since
	 * the malloc may re-use the same block and would modify
	 * p->word.
	 */
	osize = (char *)p->word - (char *)ptr;
	cp = malloc(nbytes);

	/*
	 * If we succeeded in allocating a chunk, and the chunk moved
	 * in memory, we need to move the bytes from the old place
	 * to the new.
	 */
	if (cp && cp != ptr)
	{
	    p = (struct blk *)ptr;
	    q = (struct blk *)cp;

	    /*
	     * Determine how much memory to copy, it is the minimum
	     * of the old size and the new size (rounded up to a
	     * BYTESPERWORD boundry.
	     */
	    size = round(nbytes, BYTESPERWORD);
	    if (osize < size)
		    size = osize;
	    
	    /*
	     * Since p and q are pointers to blocks and size is in
	     * bytes, divide size by the ratio between the size of a
	     * (struct blk) and the size of a (char) to get the
	     * actual number of copies that we need to perform.
	     */
	    size /= (sizeof(struct blk) / sizeof(char));
	    while (size-- > 0)
		    *q++ = *p++;
	}
	return cp;
}

free(ap)
	struct blk *ap;
{
	register struct blk *p;

#ifdef hp9000s800
	/*
	 * NLS added hp9000s800 code.
	 * Check for non-word aligned values
	 * We don't care about these, since we couldn't have
	 * allocated them.
	 */
	if (Rcheat(ap) & (BYTESPERWORD-1))
		return;
#endif
	if ((p = ap) && p < bloktop)
	{
#ifdef DEBUG
		chkbptr(p);
#else
		if (p < (struct blk *)brkbegin)
			return;
#endif
		--p;
		p->word = (struct blk *)(Rcheat(p->word) & ~BUSY);
	}


}

#ifdef DEBUG

chkbptr(ptr)
	struct blk *ptr;
{
	int	exf = 0;
	register struct blk *p = (struct blk *)brkbegin;
	register struct blk *q;
	int	us = 0, un = 0;

	for (;;)
	{
		q = (struct blk *)(Rcheat(p->word) & ~BUSY);

		if (p+1 == ptr)
			exf++;

		if (q < (struct blk *)brkbegin || q > bloktop) {
			abort(3);
		}

		if (p == bloktop)
			break;

		if (busy(p))
			us += q - p;
		else
			un += q - p;

		if (p >= q) {
			abort(4);
		}

		p = q;
	}
	if (exf == 0) {
		abort(1);
	}
}


chkmem()
{
	register struct blk *p = (struct blk *)brkbegin;
	register struct blk *q;
	int	us = 0, un = 0;

	for (;;)
	{
		q = (struct blk *)(Rcheat(p->word) & ~BUSY);

		if (q < (struct blk *)brkbegin || q > bloktop) {
			abort(3);
		}

		if (p == bloktop)
			break;

		if (busy(p))
			us += q - p;
		else
			un += q - p;

		if (p >= q) {
			abort(4);
		}

		p = q;
	}

	prs((nl_msg(1, "un/used/avail ")));
	prn(un);
	blank();
	prn(us);
	blank();
	prn((char *)bloktop - brkbegin - (un + us));
	newline();

}
#endif

