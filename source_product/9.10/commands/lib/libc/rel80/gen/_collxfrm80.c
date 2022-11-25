/* @(#) $Revision: 70.1 $ */     

/* LINTLIBRARY */

/*
 *	_collxfrm()
 */

#include <setlocale.h>

/*
 * Collxfrm transforms the characters c1 and c2 into an ordinal value in
 * the collation sequence.  If c1 plus c2 form a 2-to-1 the ordinal value
 * returned is for the 2-to-1 otherwise the value returned is only for
 * c1.  If the two_to_1 argument is a non-null pointer, it is set to true
 * if c1 plus c2 form a 2-to-1 and false otherwise.
 */

unsigned _collxfrm( c1, c2, equiv, two_to_1 )
register unsigned int	c1,		/* 8/16-bit value of 1-to-1 or 1st char of possible 2-to-1 */
			c2;		/* 8-bit value of 2nd char of possible 2-to-1 */
register int		equiv;		/* -1 if want low char of equiv class, +1 if hi char, 0 if actual char */
int			*two_to_1;	/* will set non-zero if c1 + c2 constitute a 2-to-1 collation element */
{
	register int pri;					/* priority number character */

	/* presume we do not have a 2-to-1 pair */
	if (two_to_1)
		*two_to_1 = 0;

	/* if a collation table exists, replace char with its seq/pri numbers */
	if (_seqtab) {

		/* 8-bit language with sequence numbers only collation table */
		if (_nl_onlyseq)
			c1 = (unsigned int)_seqtab[c1];		/* char becomes its sequence number */

		/* 8-bit language with sequence & priority numbers collation table */
		else {

			pri = (int)_pritab[c1];			/* use table to get priority */
			c1 = (unsigned int)_seqtab[c1];		/* and sequence number */
								/* (char becomes its sequence number) */

			switch (pri >> 6) {			/* high 2 bits indicate char type */

			case 0:					/* 1 to 1 */
				break;

			case 1:					/* 2 to 1 */
				pri &= MASK077;				/* pri becomes index */
				while (_tab21[pri].ch1 != ENDTABLE) {	/* check each possible 2nd char */
					if (c2 == _tab21[pri].ch2) {	/* against actual next char */
						c1 = _tab21[pri].seqnum;	/* got a match */
						if (two_to_1)
							(*two_to_1)++;		/* flag c1/c2 is a 2-to-1 */
						break;
					}
					pri++;
				}
				/* whether we got a match or not, where we ended up has the priority */
				pri = _tab21[pri].priority & MASK077;
				break;

			case 2:					/* 1 to 2 */
				pri = _tab12[pri & MASK077].priority;	/* priority for both seq numbers */
				break;

			case 3:					/* don't_care_character */
				return (0);				/* return "this char doesn't collate" */
			}

			/*
			 * Check if actually want endpoint of an equivalence class
			 * rather than the actual character given.
			 */
			if (equiv < 0)
				pri = 0;			/* low endpoint */
			else if (equiv > 0)
				pri = 255;			/* high endpoint */

			c1 = (c1 << 8) + pri;			/* seq # in upper 8 bits, pri # in low bits */
		}
	}

	return (c1);
}
