/* @(#) $Revision: 66.2 $ */

#ifdef EUC
/*
 *  The multibyte character parsing macros found in <nl_ctype.h> use
 *  the following routines to support the EUC code set scheme.
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define _2kanji __2kanji
#endif

#include <stddef.h>						/* for wchar_t typedef */
#include <nl_ctype.h>						/* for multibyte macros */
#include <setlocale.h>						/* for multibyte externals */

#define SS2			0x8e				/* escape to code set 2 */
#define SS3			0x8f				/* escape to code set 3 */
#define ECSET(c)		(*(__e_cset+(c)))		/* EUC code set number */


/*
 *  __euc_cs()
 *
 *  Return the EUC code set number of the multibyte character which
 *  corresponds to the wchar_t argument.
 */
unsigned char __euc_cs(wc)
register wchar_t wc;
{
	wc &= 0x80808080;					/* extract just the flags specifying the code set */
								/* (assumes 32-bit wchar_t) */

	return (wc == 0
			? 0					/* code set 0 */
			: wc == __euc_template[1]
				? 1				/* code set 1 */
				: wc == __euc_template[2]
					? 2			/* code set 2 */
					: 3 );			/* code set 3 (or an invalid character) */
}




/*
 *  __get_euc()
 *
 *  Get one logical character from a multibyte string containing
 *  characters coded in an EUC code set.  The output is the wchar_t
 *  process code set value for the first character in the multibyte
 *  string.  The multibyte string is assumed to be valid EUC format
 *  data.
 */
wchar_t __get_euc(p)
register const unsigned char *p;
{
	register unsigned int c = *p;
	register int cs, size;

	cs = ECSET(c);						/* get code set number */

	if (cs == 0)
		return c;					/* done if all we have is an ASCII character */

	size = C_MBLEN(c);

	if (cs > 1) {						/* if CS2 or CS3, eat the SS2/SS3 byte */
		c = *++p;
		size--;
	}

	c &= 0x7f;						/* strip high bit off 1st element */

	while (--size)						/* shift left previous elements */
		c = (c<<8) | (*++p & 0x7f);			/* and OR in next element */

	c |= __euc_template[cs];				/* OR in template for this code set */

	return c;
}




/*
 *  __put_euc()
 *
 *  Convert a wchar_t value into the corresponding EUC character and
 *  write the EUC byte(s) at the location specified by the pointer.
 *  The return value is the original wchar_t value.
 */
wchar_t __put_euc(wc, p)
register wchar_t wc;
register unsigned char *p;
{
	register int cs, size;

	cs = wc & 0x80808080;					/* extract just the flags specifying the code set */

	if (cs == 0) {						/* code set 0 (ASCII) */
		*p = (unsigned char) wc;
		return wc;
	}

	if (cs == __euc_template[1])				/* convert from template flag to code set number */
		cs = 1;
	else if (cs == __euc_template[2]) {
		cs = 2;
		*p++ = SS2;					/* also write SS2/SS3 tokens as required */
	} else {
		cs = 3;
		*p++ = SS3;
	}

	size = __in_csize[cs];					/* get number of bytes in character */
	if (cs > 1)						/* less the SS2/SS3 byte, if any */
		size--;

	while (size--)						/* extract each byte */
		*p++ = ((unsigned char) (wc>>(8*size))) | 0x80;	/* and write it out */

	return wc;
}




/*
 *  __put_adv_euc()
 *
 *  Same as __put_euc() above but also advance the multibyte pointer
 *  to point past the extracted EUC character.
 */

unsigned char *__put_adv_euc(wc, p)
register wchar_t wc;
register unsigned char *p;
{
	register int cs, size;

	cs = wc & 0x80808080;					/* extract just the flags specifying the code set */

	if (cs == 0) {						/* code set 0 (ASCII) */
		*p++ = (unsigned char) wc;
		return (p);
	}

	if (cs == __euc_template[1])				/* convert from template flag to code set number */
		cs = 1;
	else if (cs == __euc_template[2]) {
		cs = 2;
		*p++ = SS2;					/* also write SS2/SS3 tokens as required */
	} else {
		cs = 3;
		*p++ = SS3;
	}

	size = __in_csize[cs];					/* get number of bytes in character */
	if (cs > 1)						/* less the SS2/SS3 byte, if any */
		size--;

	while (size--)						/* extract each byte */
		*p++ = ((unsigned char) (wc>>(8*size))) | 0x80;	/* and write it out */

	return (p);
}

#else /* EUC */
extern int errno;
#endif /* EUC */
