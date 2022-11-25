/* @(#) $Revision: 72.1 $ */

#ifdef EUC
/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define mblen _mblen
#define mbtowc _mbtowc
#define wctomb _wctomb
#define mbstowcs _mbstowcs
#define wcstombs _wcstombs
#endif

#include	<stdlib.h>
#include	<limits.h>
#include	<setlocale.h>
#include	<nl_ctype.h>

#define	_XOPEN_SOURCE	/* Needed to get defn of EILSEQ from <sys/errno.h> */
#include	<errno.h>

#ifndef	NULL
#define	NULL		0
#endif
#define	STATE		1
#define	NO_STATE	0

#define SS2			0x8e				/* escape to code set 2 */
#define SS3			0x8f				/* escape to code set 3 */
#define ECSET(c)		(*(__e_cset+(c)))		/* EUC code set number */

static template[] =	{	0x00000000,			/* process code template */
				0x00008080,
				0x00000080,
				0x00008000  };


/*
** The mblen() function determines the number of bytes comprising
** the multibyte character pointed to by s.
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef mblen
#pragma _HP_SECONDARY_DEF _mblen mblen
#define mblen _mblen
#endif

int mblen(s, n)
register const unsigned char *s;
register size_t n;
{
	if (s == NULL)						/* s is a null pointer */
		return (NO_STATE);

	if (n < 1)						/* invalid n */
		return (-1);

	if (*s == '\0') {					/* s points to the null char */
		return (0);					/* special case--return len=0 */
	}

	if (__cs_SBYTE) {					/* single byte code set */
		return (1);					/* return len=1 */
	}

	if (__cs_HP15) {					/* HP15 code set */
		if (*(_1kanji+(*s))) {
			if (*(_2kanji+(*(s+1))) && n >= 2) {
				return (2);			/* valid firstof2 & secof2 char */
			} else {
				errno = EILSEQ;
				return (-1);			/* invalid secof2 char */
			}
		} else {
			return (1);				/* one byte char */
		}
	}

	if (__cs_EUC) {					/* EUC code set */
		register unsigned int c = *s;
		register int cs, size, len;

		cs = ECSET(c);					/* get code set number */

		if (cs == 0) {					/* ASCII character */
			return (1);				/* return len=1 */
		}

		len = size = C_MBLEN(c);

		if (cs > 1) {					/* if CS2 or CS3, eat the SS2/SS3 byte */
			c = *++s;
			size--;
		}

		if (!(c & 0x80)) {				/* high bit must be on */
			errno = EILSEQ;
			return (-1);
		}

		while (--size) {				/* check each byte of the char */
			if (!(*++s & 0x80)) {			/* high bit must be on */
				errno = EILSEQ;
				return (-1);
			}
		}

		return len <= n ? len : -1;
	}
}


/*
** The mbtowc() function determines the number of bytes comprising
** the multibyte character pointed to by s, determines the code for
** value of type wchar_t that corresponds to that multibyte character,
** and then stores the code in the object pointed to by pwc.
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef mbtowc
#pragma _HP_SECONDARY_DEF _mbtowc mbtowc
#define mbtowc _mbtowc
#endif

int mbtowc(pwc, s, n)
register wchar_t *pwc;
register const unsigned char *s;
register size_t n;
{
	if (s == NULL)						/* s is a null pointer */
		return (NO_STATE);

	if (n < 1)						/* invalid n */
		return (-1);

	if (*s == '\0') {					/* s points to the null char */
		if (pwc)
			*pwc = (wchar_t)0;
		return (0);					/* special case--return len=0 */
	}

	if (__cs_SBYTE) {					/* single byte code set */
		if (pwc)
			*pwc = (wchar_t)(*s);
		return (1);					/* return len=1 */
	}

	if (__cs_HP15) {					/* HP15 code set */
		if (*(_1kanji+(*s))) {
			if (*(_2kanji+(*(s+1))) && n >= 2) {
				if (pwc)			/* valid firstof2 & secof2 char */
					*pwc = (wchar_t)((*s)<<8 | *(s+1));
				return (2);
			} else {
				errno = EILSEQ;
				return (-1);			/* invalid secof2 char */
			}
		} else {
			if (pwc)				/* one byte char */
				*pwc = (wchar_t)(*s);
			return (1);
		}
	}

	if (__cs_EUC) {					/* EUC code set */
		register unsigned int c = *s;
		register int cs, size, len;

		cs = ECSET(c);					/* get code set number */

		if (cs == 0) {					/* ASCII character */
			if (pwc)
				*pwc = (wchar_t)c;
			return (1);				/* return len=1 */
		}

		len = size = C_MBLEN(c);

		if (len > n)					/* can't look at more than n bytes */
			return (-1);

		if (cs > 1) {					/* if CS2 or CS3, eat the SS2/SS3 byte */
			c = *++s;
			size--;
		}

		if (!(c & 0x80)) {				/* high bit must be on */
			errno = EILSEQ;
			return (-1);
		}

		c &= 0x7f;					/* strip high bit off 1st element */

		while (--size) {				/* check each byte of the char */
			if (!(*++s & 0x80)) {			/* high bit must be on */
				errno = EILSEQ;
				return (-1);
			}
			c = (c<<8) | (*s & 0x7f);		/* shift left previous elements */
								/* and OR in next element */
		}

		c |= template[cs];				/* OR in template for this code set */

		if (pwc)					/* if given a location	*/
			*pwc = (wchar_t)c;			/* store the value	*/

		return len;
	}
}


/*
** The wctomb() function determines the number of bytes needed to
** represent the multibyte character corresponding to the code whose
** value is wchar and stores the multibyte character representation
** in the character array pointed to by s.
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef wctomb
#pragma _HP_SECONDARY_DEF _wctomb wctomb
#define wctomb _wctomb
#endif

int wctomb(s, wchar)
register unsigned char *s;
register wchar_t wchar;
{
	unsigned char c1, c2;

	if (s == NULL)						/* s is a null pointer */
		return (NO_STATE);

	if (__cs_SBYTE) {					/* single byte code set */
		if (wchar & 0xffffff00) {			/* upper 24 bits should all be 0 */
			errno = EILSEQ;
			return (-1);
		}
		*s = (unsigned char)wchar;
		return (1);
	}

	if (__cs_HP15) {					/* HP15 code set */
		if (wchar & 0xffff0000) {			/* upper 16 bits should all be 0 */
			errno = EILSEQ;
			return (-1);
		}

		if (wchar & 0xff00) {				/* two byte char */
			c1 = wchar>>8;
			c2 = wchar;
			if (*(_1kanji+(c1)) && *(_2kanji+(c2))) {	/* valid */
				*s++ = c1;
				*s = c2;
				return (2);
			}
			else {					/* invalid */
				errno = EILSEQ;
				return (-1);
			}
		} else {					/* one byte char */
			*s = (unsigned char)wchar;
			return (1);
		}
	}

	if (__cs_EUC) {					/* EUC code set */
		register int cs, bcs, size, len, i;

		cs = wchar & 0x80808080;			/* extract just the flags specifying the code set */

		if (cs == 0) {					/* code set 0 (ASCII) */
			if (wchar & 0xffffff00) {		/* upper 24 bits should all be 0 */
				errno = EILSEQ;
				return (-1);
			}
			*s = (unsigned char)wchar;
			return (1);
		}

		if (cs == template[1]){				/* convert from template flag to code set number */
			bcs = (wchar & 0xff00)>>8;		/* extract  */
			if (bcs == SS2 || bcs == SS3) {	/* not an EUC wchar */
				errno = EILSEQ;
				return (-1);
			}
			size = __in_csize[1];
			cs = 1;
		} else if (cs == template[2]) {
			size = __in_csize[2];
			if ((size == 1)||
			    (size == 2 && (wchar & 0xffffff00))||
			    (size == 3 && (wchar & 0xffff0000))) {
				errno = EILSEQ;
				return (-1);
			}
			cs = 2;
			*s++ = SS2;				/* also write SS2/SS3 tokens as required */
		} else if (cs == template[3]){
			size = __in_csize[3];
			if ((size == 1)||
			    (size == 2 && (wchar & 0xffffff00) != 0x8000)||
			    (size == 3 && (wchar & 0xffff0000))) {
				errno = EILSEQ;
				return (-1);
			}
			cs = 3;
			*s++ = SS3;
		} else {
			errno = EILSEQ;
			return (-1);				/* not an EUC wchar */
		}

		len = size;					/* get number of bytes in character */

		for (i=4; i-size; i--)				/* unused upper bits must be all 0 */
			if ((unsigned char)(wchar>>(8*i))) {
				errno = EILSEQ;
				return (-1);
			}

		if (cs > 1)					/* don't use SS2/SS3 byte again */
			size--;

		while (size--)					/* extract each byte */
			*s++ = ((unsigned char) (wchar>>(8*size))) | 0x80;	/* and write it out */

		return len;
	}
}


/*
** The mbstowcs function converts a sequence of multibyte characters from
** the array pointed to by s into a sequence of corresponding codes and
** stores these codes into the array pointed to by pwcs, stopping after
** n codes are stored or a code with value zero (a converted null character)
** is stored.  If pwcs is NULL, then n is effectively ignored by setting it
** to a huge value, no codes are stored and the required length is returned.
** This allows the caller to determine how much space will be needed for the
** buffer passed to a future call.
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef mbstowcs
#pragma _HP_SECONDARY_DEF _mbstowcs mbstowcs
#define mbstowcs _mbstowcs
#endif

size_t mbstowcs(pwcs, s, n)
register wchar_t *pwcs;
register const unsigned char *s;
register size_t n;
{
	register size_t cnt = 0;

	if (s == NULL)						/* s is a null pointer */
		return (0);					/* no element modified */

	if (!pwcs)						/* set n high if not storing */
		n = SSIZE_MAX;					/* (no SIZE_MAX, this is smaller) */

	if (__cs_SBYTE) {					/* single byte code set */
		while (*s && (cnt < n)) {
			if (pwcs)				/* only store if not NULL */
				*pwcs++ = (wchar_t)*s;
			s++;
			cnt++;
		}
		if (pwcs && cnt < n)				/* storing and less than n element converted */
			*pwcs = (wchar_t) 0;			/* zero-terminate wchar_t array */
		return (cnt);
	}

	if (__cs_HP15) {					/* HP15 code set */
		while (*s && (cnt < n)) {
			if (*(_1kanji+(*s))) {
				if (*(_2kanji+(*(s+1)))) {
					if (pwcs)		/* only store if not NULL */
						*pwcs++ = (wchar_t)((*s)<<8 | *(s+1));	/* valid firstof2 & secof2 char */
					s += 2;
				} else {
					errno = EILSEQ;
					return ((size_t)-1);	/* invalid secof2 char */
				}
			} else {
				if (pwcs)			/* only store if not NULL */
					*pwcs++ = (wchar_t)*s;	/* one byte char */
				s++;

			}
			cnt++;
		}
		if (pwcs && cnt < n)				/* storing and less than n element converted */
			*pwcs = (wchar_t) 0;			/* zero-terminate wchar_t array */
		return (cnt);
	}

	if (__cs_EUC) {					/* EUC code set */
		register unsigned int c;
		register int cs, size;

		while (*s && (cnt < n)) {

			c = *s++;				/* first byte of character */
			cs = ECSET(c);				/* get code set number */

			if (cs == 0) {				/* ASCII character */
				if (pwcs)			/* only store if not NULL */
					*pwcs++ = (wchar_t)c;
				cnt++;
				continue;
			}

			size = C_MBLEN(c);

			if (cs > 1) {				/* if CS2 or CS3, eat the SS2/SS3 byte */
				c = *s++;
				size--;
			}

			if (!(c & 0x80)) {			/* high bit must be on */
				errno = EILSEQ;
				return (-1);
			}

			c &= 0x7f;				/* strip high bit off 1st element */

			while (--size) {			/* check each byte of the char */
				c = (c<<8) | (*s & 0x7f);	/* shift left previous elements */
								/* and OR in next element */
				if (!(*s++ & 0x80)) {		/* high bit must be on */
					errno = EILSEQ;
					return (-1);
				}
			}

			c |= template[cs];			/* OR in template for this code set */

			if (pwcs)				/* only store if not NULL */
				*pwcs++ = (wchar_t)c;
			cnt++;
		}
		if (pwcs && cnt < n)				/* storing and less than n element converted */
			*pwcs = (wchar_t) 0;			/* zero-terminate wchar_t array */
		return (cnt);
	}
}


/*
** The wcstombs function converts a sequence of codes that correspond to
** multibyte characters from the array pointed to by pwcs into a sequence
** of multibyte characters and stores these multibyte characters into
** the array pointed to by s, stopping if a multibyte character would
** exceed the limits of n total bytes or if a null character is stored.
** If s is NULL, then n is effectively ignored by setting it to a huge value,
** no codes are stored and the required length is returned.  This allows the
** caller to determine how much space will be needed for the buffer passed to
** a future call.
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef wcstombs
#pragma _HP_SECONDARY_DEF _wcstombs wcstombs
#define wcstombs _wcstombs
#endif

size_t wcstombs(s, pwcs, n)
register unsigned char *s;
register const wchar_t *pwcs;
register size_t n;
{
	register wchar_t wchar;
	register size_t cnt = 0;

	if (pwcs == NULL)					/* pwcs is a null pointer */
		return (0);					/* no element modified */

	if (!s)							/* set n high if not storing */
		n = SSIZE_MAX;					/* (no SIZE_MAX, this is smaller) */

	if (__cs_SBYTE) {					/* single byte code set */
		while ((wchar = *pwcs++) && (cnt < n)) {
			cnt++;
			if (wchar & 0xffffff00) {		/* upper 24 bits should all be 0 */
				errno = EILSEQ;
				return (-1);
			}
			if (s)					/* only store if not NULL */
				*s++ = (unsigned char)wchar;
		}
		if (s && cnt < n)				/* storing and less than n elements converted */
			*s = (unsigned char)0;
		return cnt;
	}

	if (__cs_HP15) {					/* HP15 code set */
		register unsigned char c1, c2;
		register room = 1;

		while ((wchar = *pwcs++) && (cnt < n)) {
			cnt++;
			if (wchar & 0xffff0000) {		/* upper 16 bits should all be 0 */
				errno = EILSEQ;
				return (-1);
			}

			if (wchar & 0xff00) {			/* two byte char */
				c1 = wchar>>8;
				c2 = wchar;
				if (*(_1kanji+(c1)) && *(_2kanji+(c2))) {	/* valid */
					if (++cnt > n) {	/* need room for 2 bytes */
						cnt -= 2;
						room = 0;
						break;
					}
					if (s) {		/* only store if not NULL */
						*s++ = c1;
						*s++ = c2;
					}
				}
				else {				/* invalid */
					errno = EILSEQ;
					return (-1);
				}
			} else {				/* one byte char */
				if (s)				/* only store if not NULL */
					*s++ = (unsigned char)wchar;
			}
		}
		if (s && room && cnt < n)			/* storing and enough room and less than n elements converted */
			*s = (unsigned char)0;
		return cnt;
	}

	if (__cs_EUC) {					/* EUC code set */
		register int cs, bcs, size, i;
		register room = 1;

		while ((wchar = *pwcs++) && (cnt < n)) {

			cs = wchar & 0x80808080;		/* extract just the flags specifying the code set */

			if (cs == 0) {				/* code set 0 (ASCII) */
				if (wchar & 0xffffff00) {	/* upper 24 bits should all be 0 */
					errno = EILSEQ;
					return (-1);
				}
				cnt++;
				if (s)				/* only store if not NULL */
					*s++ = (unsigned char)wchar;
				continue;
			}

			if (cs == template[1]){			/* convert from template flag to code set number */
				bcs = (wchar & 0xff00)>>8;
				if (bcs == SS2 || bcs == SS3) {
					errno = EILSEQ;
					return (-1);
				}
				size = __in_csize[1];
				cs = 1;
			} else if (cs == template[2]){
				size = __in_csize[2];
				if((size == 1)||
				   (size == 2 && (wchar & 0xffffff00))||
				   (size == 3 && (wchar & 0xffff0000))) {
					errno = EILSEQ;
					return (-1);
				}
				cs = 2;
			} else if (cs == template[3]){
				size = __in_csize[3];
				if((size == 1)||
				   (size == 2 && (wchar & 0xffffff00) != 0x8000)||
				   (size == 3 && (wchar & 0xffff0000))) {
					errno = EILSEQ;
					return (-1);
				}
				cs = 3;
			} else {
				errno = EILSEQ;
				return (-1);			/* not an EUC wchar */
			}

			cnt += size;
			if (cnt > n) {				/* need room for size bytes */
				cnt -= size;
				room = 0;
				break;
			}

			for (i=4; i-size; i--)			/* unused upper bits must be all 0 */
				if ((unsigned char)(wchar>>(8*i))) {
					errno = EILSEQ;
					return (-1);
				}

			if (cs > 1) {				/* write SS2/SS3 tokens as required */
				if (s)				/* only store if not NULL */
					*s++ = (cs == 2) ? SS2 : SS3;
				size--;
			}

			while (size--)				/* extract each byte */
				if (s)				/* only store if not NULL */
					*s++ = ((unsigned char) (wchar>>(8*size))) | 0x80;	/* and write it out */

		}

		if (s && room && cnt < n)			/* storing and enough room and less than n elements converted */
			*s = (unsigned char)0;
		return cnt;
	}
}
#else /* EUC */
/* @(#) $Revision: 72.1 $ */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define _1kanji __1kanji
#define _2kanji __2kanji
#define mblen _mblen
#define mbtowc _mbtowc
#define wctomb _wctomb
#define mbstowcs _mbstowcs
#define wcstombs _wcstombs
#endif

#include	<stdlib.h>

#define	_XOPEN_SOURCE	/* Needed to get defn of EILSEQ from <sys/errno.h> */
#include	<errno.h>

#ifndef	NULL
#define	NULL		0
#endif
#define	STATE		1
#define	NO_STATE	0

extern unsigned char *_1kanji;
extern unsigned char *_2kanji;

/*
** The mblen() function determines the number of bytes comprising
** the multibyte character pointed to by s.
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef mblen
#pragma _HP_SECONDARY_DEF _mblen mblen
#define mblen _mblen
#endif

int mblen(s, n)
register const unsigned char *s;
register size_t n;
{
	if (s == NULL)			/* s is a null pointer */
		return(NO_STATE);
	if (n < 1)			/* invalid n */
		return(-1);
	if (*s == '\0')			/* s points to the null char */
		return(0);
	if (*(_1kanji+(*s)))
		if (*(_2kanji+(*(s+1))) && n >= 2)
			return(2);	/* valid firstof2 & secof2 char */
		else {
			errno = EILSEQ;
			return(-1);	/* invalid secof2 char */
		}
	else			
		return(1);		/* one byte char */
}

/*
** The mbtowc() function determines the number of bytes comprising
** the multibyte character pointed to by s, determines the code for
** value of type wchar_t that corresponds to that multibyte character,
** and then stores the code in the object pointed to by pwc.
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef mbtowc
#pragma _HP_SECONDARY_DEF _mbtowc mbtowc
#define mbtowc _mbtowc
#endif

int mbtowc(pwc, s, n)
register wchar_t *pwc;
register const unsigned char *s;
register size_t n;
{
	switch (mblen(s,n)) {
		case 0:			/* null char */
			if (s && pwc)	/* store only if s & pwc isn't null */
				*pwc = (wchar_t)0;
			return(0);
		case 1:			/* one byte char */
			if (pwc)	/* store only if pwc isn't null */
				*pwc = (wchar_t)(*s);
			return(1);
		case 2:			/* two byte char */
			if (pwc)	/* store only if pwc isn't null */
				*pwc = (wchar_t)((*s)<<8 | *(s+1));
			return(2);
		case -1:		/* invalid char */
			errno = EILSEQ;
		default:
			return(-1);
	}
}

/*
** The wctomb() function determines the number of bytes needed to
** represent the multibyte character corresponding to the code whose
** value is wchar and stores the multibyte character representation
** in the character array pointed to by s.
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef wctomb
#pragma _HP_SECONDARY_DEF _wctomb wctomb
#define wctomb _wctomb
#endif

int wctomb(s, wchar)
register unsigned char *s;
register wchar_t wchar;
{
	unsigned char c1, c2;

	if (s == NULL)			/* s is a null pointer */
		return(NO_STATE);
	if (wchar & 0xff00) {		/* two byte char */
		c1 = wchar>>8;
		c2 = wchar;
		if (*(_1kanji+(c1)) && *(_2kanji+(c2))) {	/* valid */
			*s = c1;
			*(s+1) = c2;
			return(2);
		}
		else {						/* invalid */
			errno = EILSEQ;
			return(-1);
		}
	} else {			/* one byte char */
		*s = (unsigned char)(wchar);
		return(1);
	}
}

/*
** The mbstowcs function converts a sequence of multibyte characters from
** the array pointed to by s into a sequence of corresponding codes and
** stores these codes into the array pointed to by pwcs, stopping after
** n codes are stored or a code with value zero (a converted null character)
** is stored.
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef mbstowcs
#pragma _HP_SECONDARY_DEF _mbstowcs mbstowcs
#define mbstowcs _mbstowcs
#endif

size_t mbstowcs(pwcs, s, n)
register wchar_t *pwcs;
register const unsigned char *s;
register size_t n;
{
	register int ret;
	register size_t cnt = 0;

	if (s == NULL || pwcs == NULL)	/* s or pwcs is a null pointer */
		return(0);		/* no element modified */

	while (*s && (cnt < n)) {
		ret = mbtowc(pwcs, s, MB_CUR_MAX);
		if (ret == -1) {	/* invalid multibyte char */
			errno = EILSEQ;
			return((size_t) -1);
		}
		else {			/* one element converted */
			cnt++;
			s += ret;
			pwcs++;
		}
	}
	if (cnt < n)			/* less than n element converted */
		*pwcs = (wchar_t) 0;	/* zero-terminate wchar_t array */
	return(cnt);
}

/*
** The wcstombs function converts a sequence of codes that correspond to
** multibyte characters from the array pointed to by pwcs into a sequence
** of multibyte characters and stores these multibyte characters into
** the array pointed to by s, stopping if a multibyte character would
** exceed the limits of n total bytes or if a null character is stored.
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef wcstombs
#pragma _HP_SECONDARY_DEF _wcstombs wcstombs
#define wcstombs _wcstombs
#endif

size_t wcstombs(s, pwcs, n)
register unsigned char *s;
register const wchar_t *pwcs;
register size_t n;
{
	register int ret;
	register size_t cnt = 0;

	if (s == NULL || pwcs == NULL)	/* s or pwcs is a null pointer */
		return(0);		/* no element modified */
	if (n == 0)			/* n == 0 */
		return(0);		/* no element modified */
	while (*pwcs) {
		ret = wctomb(s, *pwcs);
		if (ret == -1) {	/* invalid wchar_t char */
			errno = EILSEQ;
			return((size_t) -1);
		}
		else {			/* one element converted */
			cnt += ret;
			if (cnt > n) {	/* exceed n bytes, stop */
				cnt -= ret;
				*s = (char) 0;
				return(cnt);
			}
			if (cnt == n)	/* exactly n bytes, stop */
				return(cnt);
			pwcs++;		/* continue to convert */
			s += ret;
		}
	}
	*s = (char) 0;			/* less than n bytes */
	return (cnt);
}
#endif /* EUC */
