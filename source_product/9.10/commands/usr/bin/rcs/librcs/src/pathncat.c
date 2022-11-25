/* $Header: pathncat.c,v 1.1 86/04/17 14:10:57 bob Exp $ */

/* Copyright Hewlett Packard Co. 1986 */

/*
 * pathncat() concatenates path name component p2 to p1. The maximum
 * length of the resulting pathname is plen. Returns p1.
 */

#define NULL 0

char *
pathncat(p1, p2, plen)
	register char *p1;
	register char *p2;
	register int plen;
{
	char *sp1;			/* start of p1 */

	sp1 = p1;
	while (*p1)
		{
		p1++;
		plen--;
		}

	if (plen < 2)			/* leave room for "/" */
		return(sp1);

	if (p1 != sp1 && p1[-1] != '/')
		{
		*p1++ = '/';
		plen--;
		}

	while (*p1++ = *p2++)
		{
		if (--plen == 0)
			{
			*--p1 = '\0';
			break;
			}
		}
	return(sp1);
}
