/* @(#) $Revision: 64.1 $ */      
#

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * String allocation routines.
 * Strings handed out here are reclaimed at the top of the command
 * loop each time, so they need not be freed.
 */

#include "rcv.h"

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 16	/* set number */
#endif NLS


/*
 * Allocate size more bytes of space and return the address of the
 * first byte to the caller.  An even number of bytes are always
 * allocated so that the space will always be on a word boundary.
 * The string spaces are of exponentially increasing size, to satisfy
 * the occasional user with enormous string size requests.
 */

char *
salloc(size)
{
	register char *t;
	register int s;
	register struct strings *sp;
	int index;
	physadr l_physadr;

	s = size;
#ifdef	u3b
	s += 3;		/* 3b's need alignment on quad boundary */
	s &= ~03;
#else
	/* insure alignment on address boundary */
	s += (sizeof l_physadr) -1;
	s &= ~((sizeof l_physadr) - 1);
#endif	u3b
	index = 0;
	for (sp = &stringdope[0]; sp <= &stringdope[NSPACE-1]; sp++) {
		if (sp->s_topFree == NOSTR && (STRINGSIZE << index) >= s)
			break;
		if (sp->s_nleft >= s)
			break;
		index++;
	}
	if (sp > &stringdope[NSPACE-1])
		panic((catgets(nl_fn,NL_SETN,1, "String too large")));
	if (sp->s_topFree == NOSTR) {
		index = sp - &stringdope[0];
		sp->s_topFree = (char *) calloc(STRINGSIZE << index,
		    (unsigned) 1);
		if (sp->s_topFree == NOSTR) {
			fprintf(stderr, (catgets(nl_fn,NL_SETN,2, "No room for space %d\n")), index);
			panic((catgets(nl_fn,NL_SETN,3, "Internal error")));
		}
		sp->s_nextFree = sp->s_topFree;
		sp->s_nleft = STRINGSIZE << index;
	}
	sp->s_nleft -= s;
	t = sp->s_nextFree;
	sp->s_nextFree += s;
	return(t);
}

/*
 * Reset the string area to be empty.
 * Called to free all strings allocated
 * since last reset.
 */

sreset()
{
	register struct strings *sp;
	register int index;

	if (noreset)
		return;
	minit();
	index = 0;
	for (sp = &stringdope[0]; sp <= &stringdope[NSPACE-1]; sp++) {
		if (sp->s_topFree == NOSTR)
			continue;
		sp->s_nextFree = sp->s_topFree;
		sp->s_nleft = STRINGSIZE << index;
		index++;
	}
}
