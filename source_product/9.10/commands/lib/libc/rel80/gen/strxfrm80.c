/* @(#) $Revision: 70.1 $ */     
/* LINTLIBRARY */
/*
 *	strxfrm()
 */
#ifdef _NAMESPACE_CLEAN
#define strxfrm _strxfrm
#endif

#include <string.h>					/* for size_t */
#include <nl_ctype.h>					/* for CHARADV() */
#include <setlocale.h>
#include <collate.h>

/*
 * Strxfrm transforms the string pointed to by s2 and places the result
 * into the array pointed to by s1.  The transformation is such that if the
 * strcmp function is applied to two transformed strings, it returns a value
 * greater than, equal to, or less than zero, corresponding to the result of
 * the strcoll function applied to the same two original strings.  No more
 * than n characters are placed into the resulting array pointed to by s1,
 * including the null character.  Strxfrm returns the length of the transformed
 * string (not including the terminating null character).  If the value is n
 * or more, the contents of the array pointed to by s1 are indeterminate.
 */
#ifdef _NAMESPACE_CLEAN
#undef strxfrm
#pragma _HP_SECONDARY_DEF _strxfrm strxfrm
#define strxfrm _strxfrm
#endif

size_t strxfrm( s1, s2, n )
register unsigned char *s1;
register const unsigned char *s2;
register size_t n;
{
	register int i = 0;				/* to count size of result */
	register int c, pri;				/* character and its priority */
	register unsigned char *p0, *p1;		/* where to store priority numbers */
	register int index;				/* index into 1-to-2, 2-to-1 tables */

	if (s2 == NULL)					/* done if nothing to transform */
		return(0);

	if (s1 == NULL)					/* flag no output string desired */
		n = 0;
	
	/* 7/8-bit language with machine collation */
	if (!_nl_collate_on) {
		
		/*
		 * The transformed string is just a copy of the original.
		 */

		for (; *s2; s2++) {
			if (i++ < n)
				*s1++ = *s2;
		}
		if (i < n)
			*s1 = '\0';
		return(i);
	}

	/* 16-bit language with machine collation */
	if (_nl_mb_collate) {

		/*
		 * Each character, whether single or multibyte, of the original
		 * string is converted to 3 bytes in the transformed string as
		 * follows:
		 *           original 16-bit char:    aaaaaaab bbbbbbcc
		 *           transformed:             aaaaaaa1 bbbbbbb1 000001cc
		 *
		 *           original 8-bit char:     bbbbbbcc
		 *           transformed:             00000001 0bbbbbb1 000001cc
		 *
		 * This guarantees no null bytes and that all 8-bit characters
		 * will collated before all 16-bit characters.
		 */

		while (c = CHARADV(s2))
			if ((i += 3) < n) {
				*s1++ = ((c & 0xfe00) >> 8) + 1;
				*s1++ = ((c & 0x01fc) >> 1) + 1;
				*s1++ = (c & 0x0003) + 4;
			}
		if (i < n)
			*s1 = '\0';
		return(i);
	}

	/* 8-bit language with collation table (sequence numbers only) */
	if (_nl_onlyseq) {

		/*
		 * The possible sequence numbers for this case include zero so
		 * the sequence number must be transformed into two bytes:
		 *
		 *           original 8-bit char:     aaaabbbb
		 *           transformed:             aaaa0001 0001bbbb
		 */

		while (c = *s2++)
			if ((i += 2) < n) {
				c = (int)_seqtab[c];		/* char becomes its sequence number */
				*s1++ = (c & 0xf0) + 1;		/* store in two bytes /w one bit always */
				*s1++ = (c & 0x0f) + 0x10;	/*    set to guarantee no zero bytes    */
			}
		if (i < n)
			*s1 = '\0';
		return(i);
	}

	/* 8-bit language with collation table (sequence & priority numbers) */

		/*
		 * In the most simple case, the original (On) characters are
		 * transformed into sequence (Sn) bytes and priority (Pn) bytes
		 * as follows:
		 *
		 *            original string:   O1 O2 O3 O4 O5 \0
		 *            transformed:       S1 S2 S3 S4 S5 P1 P2 P3 P4 P5 \0
		 *
		 * For 2-to-1 original characters, On and On+1, a single pair of
		 * transformed bytes, Sn and Pn, are produced.
		 *
		 * For a 1-to-2 original character, On, two pairs of transformed
		 * bytes, Sn Sn+1 and Pn Pn+1, are produced.
		 *
		 * For a "don't care" original character, no transformed bytes
		 * are produced.
		 *
		 * All transformed bytes must be non-zero (automatic for sequence
		 * numbers, must add 1 to priority numbers) and the lowest sequence
		 * number must be greater than the highest priority number (guaranteed
		 * by buildlang(1m) when it creates collation tables).
		 */

	p0 = p1 = s1 + (n/2);				/* start storing priority numbers halfway down */

	while (c = *s2++) {

		pri = (int)_pritab[c];			/* use table to get priority */
		c = (int)_seqtab[c];			/* and sequence number */
							/* (char becomes its sequence number) */

		switch (pri >> 6) {			/* high 2 bits indicate char type */

		case 0:					/* 1 to 1 */
			break;

		case 1:					/* 2 to 1 */
			index = pri & MASK077;
			while (_tab21[index].ch1 != ENDTABLE) {	/* check each possible 2nd char */
				if (*s2 == _tab21[index].ch2) {	/* against actual next char */
					s2++;			/* got a match */
					c = _tab21[index].seqnum;
					break;
				}
				index++;
			}
			/* whether we got a match or not, where we ended up has the priority */
			pri = _tab21[index].priority & MASK077;
			break;

		case 2:					/* 1 to 2 */
			index = pri & MASK077;
			pri = _tab12[index].priority;		/* priority for both seq numbers */
			if ((i += 2) < n) {
				*s1++ = c;			/* store 1st set of sequence number */
				*p1++ = pri + 1;		/* and priority number (offset from zero) */
			}
			c = _tab12[index].seqnum;		/* 2nd sequence number */
			break;

		case 3:					/* don't_care_character */
			continue;				/* skip to next character */
		}

		if ((i += 2) < n) {
			*s1++ = c;			/* store sequence number */
			*p1++ = pri + 1;		/* and priority number (offset from zero) */
		}
	}
	if (i < n) {
		if (p0 > s1)				/* gap between end of seq and start of pri #'s ? */
			while (p0 < p1)			/* if so, slide pri #'s down to fill in gap */
				*s1++ = *p0++;
		*s1 = '\0';
	}
	return(i);
}
