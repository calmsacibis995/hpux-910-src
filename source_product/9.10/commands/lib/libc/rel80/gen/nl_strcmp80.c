/* @(#) $Revision: 70.1 $ */     
/* LINTLIBRARY */
/*
 *	nl_strcmp() and nl_strncmp()
 */

#ifdef _NAMESPACE_CLEAN
#define nl_strcmp _nl_strcmp
#define nl_strncmp _nl_strncmp
#define strcmp _strcmp
#define strncmp _strncmp
#endif /* _NAMESPACE_CLEAN */

#include <values.h>	/* for MAXINT */
#include <nl_ctype.h>
#include <setlocale.h>
#include <collate.h>

#ifndef NULL
#define NULL		(unsigned char *)0
#endif

/*
 * nl_strcmp compares two strings and returns an integer greater than,
 * equal to, or less than 0, according to the native collating sequence
 * table if string1 is greater than, equal to, or less than string2.
 */

#ifdef _NAMESPACE_CLEAN
#undef nl_strcmp
#pragma _HP_SECONDARY_DEF _nl_strcmp nl_strcmp
#define nl_strcmp _nl_strcmp
#endif /* _NAMESPACE_CLEAN */

nl_strcmp(s1, s2)
const unsigned char *s1, *s2;
{
	if (!_nl_collate_on)
		return(strcmp((char *)s1, (char *)s2));			/* "C" processing */
	else
		return(nl_strncmp(s1, s2, MAXINT));			/* NLS processing */
}

/*
 * nl_strncmp compares two strings up to n characters and returns an
 * integer greater than, equal to, or less than 0, according to the
 * native collating sequence table if string1 is greater than, equal
 * to, or less than string2.
 */

#ifdef _NAMESPACE_CLEAN
#undef nl_strncmp
#pragma _HP_SECONDARY_DEF _nl_strncmp nl_strncmp
#define nl_strncmp _nl_strncmp
#endif /* _NAMESPACE_CLEAN */

nl_strncmp(s1, s2, n1)
register const unsigned char *s1, *s2;
register size_t n1;
{
	register int	n2;						/* individually count down each string */
	register int	c1, c2;
	register int	index;
	register int	pri1, pri2, priority;
	register int	peekc1, peekc2;

	if (!_nl_collate_on)						/* "C" processing */
		return(strncmp((char *)s1, (char *)s2, n1));

	if (s1 == s2 || n1 == 0)					/* nothing to compare?				*/
		return(0);
	else if (s1 == NULL)						/* comparing nothing against something?	*/
		return(0-*s2);
	else if (s2 == NULL)						/* comparing something against nothing?	*/
		return(*s1);

	/* no collation table yet for Kanji */
	if (_nl_mb_collate) {
		while (n1-- && (c1 = CHARADV(s1)) == (c2 = CHARADV(s2)))
			if(c1 == '\0')
				return(0);
		return(c1 - c2);
	}

	/*
	** If not a language with 2-to-1 mappings, quickly skip past identical
	** leading characters before starting slower collate table processing.
	*/
	if (!_nl_map21 && *s1 == *s2) {
		do {
			if (*s1 == '\0')
				return(0);
		} while (--n1 && *++s1 == *++s2);

		if (n1 == 0)
			return(0);
	}

	n2 = n1;
	priority = peekc1 = peekc2 = 0;

	/* perform rest of compare using the collation table */
	while ( n1 && n2 ) {
		/*
		**  Get the sequence number and priority of the next
		**  character in the first string.
		*/
		if (c1 = peekc1) {					/* if 2nd char of a 1 to 2 is waiting */
			n1--;						/*    use it */
			peekc1 = 0;
		} else
			while (n1 && (c1 = *s1++)) {			/* otherwise, get a new character */
				n1--;

				pri1 = (int)_pritab[c1];		/* use table to get priority */
				c1 = (int)_seqtab[c1] + 1;		/* and sequence number */
									/* (char becomes its sequence number) */
									/* (+1 so seq #'s are offset from '\0') */

				switch (pri1 >> 6) {			/* high 2 bits indicate char type */

				case 0:					/* 1 to 1 */
					break;

				case 1:					/* 2 to 1 */
					index = pri1 & MASK077;
					while (_tab21[index].ch1 != ENDTABLE) {	/* check each possible 2nd char */
						if (n1 && *s1 == _tab21[index].ch2) {	/* against actual next char */
							s1++; n1--;		/* got a match */
							c1 = _tab21[index].seqnum + 1;
							break;
						}
						index ++;
					}
					/* whether we got a match or not, where we ended up has the priority */
					pri1 = _tab21[index].priority & MASK077;
					break;

				case 2:					/* 1 to 2 */
					index = pri1 & MASK077;
					peekc1 = _tab12[index].seqnum + 1;	/* hold on to the 2nd char */
					pri1 = _tab12[index].priority;		/* combined priority */
					n1++;					/* now have 1 more char to process */
					break;

				case 3:					/* don't_care_character */
					c1 = 0;					/* in case there are no more chars */
					continue;				/* skip to next character */
				}
				break;
			}
		/*
		**  Get the sequence number and priority of the next
		**  character in the second string.
		*/
		if (c2 = peekc2) {					/* if 2nd char of a 1 to 2 is waiting */
			n2--;						/*    use it */
			peekc2 = 0;
		} else
			while (n2 && (c2 = *s2++)) {			/* otherwise, get a new character */
				n2--;

				pri2 = (int)_pritab[c2];		/* use table to get priority */
				c2 = (int)_seqtab[c2] + 1;		/* and sequence number */
									/* (char becomes its sequence number) */
									/* (+1 so seq #'s are offset from '\0') */

				switch (pri2 >> 6) {			/* high 2 bits indicate char type */

				case 0:					/* 1 to 1 */
					break;

				case 1:					/* 2 to 1 */
					index = pri2 & MASK077;
					while (_tab21[index].ch1 != ENDTABLE) {	/* check each possible 2nd char */
						if (n2 && *s2 == _tab21[index].ch2) {	/* against actual next char */
							s2++; n2--;		/* got a match */
							c2 = _tab21[index].seqnum + 1;
							break;
						}
						index ++;
					}
					/* whether we got a match or not, where we ended up has the priority */
					pri2 = _tab21[index].priority & MASK077;
					break;

				case 2:					/* 1 to 2 */
					index = pri2 & MASK077;
					peekc2 = _tab12[index].seqnum + 1;	/* hold on to the 2nd char */
					pri2 = _tab12[index].priority;		/* combined priority */
					n2++;					/* now have 1 more char to process */
					break;

				case 3:					/* don't_care_character */
					c2 = 0;					/* in case there are no more chars */
					continue;				/* skip to next character */
				}
				break;
			}
		if (c1 != c2)						/* sequence #'s differ for these chars */
			return(c1 - c2);
		if (c1 == '\0')						/* both strings ended with all seq #'s the same */
			return(priority);
		if (!priority)						/* priority tie breaker is set at 1st opportunity */
			priority = pri1 - pri2;
	}
	/*
	**  Hit the number limit on one or both of the strings.
	**  - If both counts expired, the only difference between the strings can be their priorities.
	**  - The same is also true if one count expired and the pointer to the other string has
	**    reached the end of the string.
	**  - In all other cases, the shorter string collates earlier.
	*/
	return ((n1 == n2) ? priority					/* both expired */
			   : (n1 == 0) ? (*s2 == '\0') ? priority	/* s1 expired, s2 at string end */
						       : 0-*s2		/* s2 is longer */
				       : (*s1 == '\0') ? priority	/* s2 expired, s1 at string end */
						       : *s1 );		/* s1 is longer */
}
