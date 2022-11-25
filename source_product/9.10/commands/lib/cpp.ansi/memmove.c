/* @(#) $Revision: 70.2 $ */   
/*LINTLIBRARY*/
/*
 *  memmove: Copy n bytes from s2 to s1 and return s1.
 *           Guaranteed to work even if s1 and s2 overlap.
 *          
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define memmove _memmove
#endif

#include <sys/types.h>

#if !defined(__aix)
#undef  NULL
#define NULL	(unsigned char *)0
#endif

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef memmove
#pragma _HP_SECONDARY_DEF _memmove memmove
#define memmove _memmove
#endif

char *memmove(s1, s2, n)
register unsigned char		*s1;
register unsigned char	*s2;
register size_t			n;
{
	register unsigned char *start_s1;

	if (s2 == NULL)				/* s1 undefined if s2 == NULL		*/
		return((char *)s1);

	start_s1 = s1;				/* save address to be returned		*/

	if (s1 > s2 && s1 < s2+n) {
		s1 += n;			/* target overlaps source ...		*/
		s2 += n;			/* so copy from the end backwards	*/
		while (n--)			/* so as not to clobber any of the	*/
			*--s1 = *--s2;		/* source before its copied		*/
	} else
		while (n--)			/* otherwise do a regular copy		*/
			*s1++ = *s2++;

	return ((char *)start_s1);
}
