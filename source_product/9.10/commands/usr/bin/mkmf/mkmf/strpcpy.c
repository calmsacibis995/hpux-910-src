/* $Header: strpcpy.c,v 1.1 84/09/14 17:00:27 bob HP_UXRel1 $ */

/*
 * Author: Peter J. Nicklin
 */

/*
 * strpcpy() copies string s2 to s1 and returns a pointer to the
 * next character after s1.
 */
char *
strpcpy(s1, s2)
	register char *s1;
	register char *s2;
{
	while (*s1++ = *s2++)
		continue;
	return(--s1);
}
