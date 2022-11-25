/* @(#) $Revision: 70.1 $ */   
/*LINTLIBRARY*/
/*
 *  memmove: Copy n bytes from s2 to s1 and return s1.
 *           Guaranteed to work even if s1 and s2 overlap.
 *          
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define memmove _memmove
#define memcpy  _memcpy
#endif

#include <sys/types.h>
extern char* memcpy();

#define NULL	(unsigned char *)0

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef memmove
#pragma _HP_SECONDARY_DEF _memmove memmove
#define memmove _memmove
#endif

char *memmove(s1, s2, n)
register unsigned char		*s1;
register const unsigned char	*s2;
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
		return ((char *)start_s1);
	} else
		return memcpy(s1,s2,n);
}
