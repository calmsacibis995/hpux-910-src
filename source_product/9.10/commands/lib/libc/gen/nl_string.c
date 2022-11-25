/* @(#) $Revision: 70.1 $ */     
#ifdef PIC
#pragma HP_SHLIB_VERSION "4/92"
#endif
/* LINTLIBRARY */
/* 
    strcmp8, strncmp8, strcmp16, strncmp16
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define _1kanji __1kanji
#define _2kanji __2kanji
#define strcmp8 _strcmp8
#define strncmp8 _strncmp8
#define idtolang _idtolang
#define langinit _langinit
#define strcmp16 _strcmp16
#define strncmp16 _strncmp16
#define strncmp _strncmp
#endif

#include <values.h>	/* for MAXINT */
#include <nl_ctype.h>
#include <setlocale.h>
#include <collate.h>
#include <langinfo.h>
#include <locale.h>

#ifndef NULL
#   define NULL		(unsigned char *)0
#endif

/*
 * strcmp8 compares two strings and returns an integer greater than,
 * equal to, or less than 0, according to the native collating sequence
 * table specified by langid if string1 is greater than, equal to,
 * or less than string2.  Trailing blanks in both strings are ignored.
 * A status variable is set according to difficulties encountered in
 * loading the specified language.
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef strcmp8
#pragma _HP_SECONDARY_DEF _strcmp8 strcmp8
#define strcmp8 _strcmp8
#endif

strcmp8(s1, s2, langid, status)
unsigned const char *s1, *s2;
int langid, *status;
{
	return(strncmp8(s1, s2, MAXINT, langid, status));
}

/*
 * strncmp8 compares two strings up to n characters and returns an
 * integer greater than, equal to, or less than 0, according to the
 * native collating sequence table specified by langid if string1 is
 * greater than, equal to, or less than string2.  Trailing blanks in
 * both strings are ignored.  A status variable is set according to
 * difficulties encountered in loading the specified language.
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef strncmp8
#pragma _HP_SECONDARY_DEF _strncmp8 strncmp8
#define strncmp8 _strncmp8
#endif

strncmp8(s1, s2, n, langid, status)
register const unsigned char *s1, *s2;
int n, langid, *status;
{
	register int	n1 = n, n2 = n;					/* individually count down each string */
	register int	c1, c2;
	register int	index;
	register int	pri1 = 0, pri2 = 0, priority = 0;
	register int	peekc1 = 0, peekc2 = 0;
	extern int	_nl_errno;

	/* load language if not already loaded */
	if (langid != __nl_langid[LC_COLLATE]) {
		if (langinit(idtolang(langid)))
			_nl_errno = ENOLFILE;				/* collating table inaccessible */
		*status = _nl_errno;					/* idtolang() may have set ENOCFFILE or ENOCONV */
	} else
		*status = 0;

	if (s1 == s2 || n <= 0)						/* nothing to compare?			*/
		return(0);
	else if (s1 == NULL)						/* comparing nothing against something?	*/
		return(0-*s2);
	else if (s2 == NULL)						/* comparing something against nothing?	*/
		return(*s1);

	while ( n1 && n2 ) {
		/*
		**  Get the sequence number and priority of the next
		**  character in the first string.
		*/
		if (c1 = peekc1) {					/* if 2nd char of a 1 to 2 is waiting */
			n1--;						/*    use it */
			peekc1 = 0;
		} else
			while (n1 && (c1 = CHARADV(s1))) {		/* otherwise, get a new character */
				n1--;

				/* no table lookup for C/n-computer or multibyte languages */
				if (!_nl_collate_on || _nl_mb_collate)
					break;

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
			while (n2 && (c2 = CHARADV(s2))) {		/* otherwise, get a new character */
				n2--;

				/* no table lookup for C/n-computer or multibyte languages */
				if (!_nl_collate_on || _nl_mb_collate)
					break;

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
		if (c1 != c2) {						/* sequence #'s differ for these chars */
			if (c1 == '\0' && *(s2-1) == ' ') {			/* if at end of s1 */
				while (n2 && (c2 = *s2) && c2 == ' ') {		/* see if deleting blanks from s2 */
					s2++; n2--;				/* will bring us to the end of s2 */
				}
				if (!n2 || !c2)
					return (priority);			/* equal except for priority */
				else
					return (0-c2);				/* s2 is longer */
			} else if (c2 == '\0' && *(s1-1) == ' ') {		/* if at end of s2 */
				while (n1 && (c1 = *s1) && c1 == ' ') {		/* see if deleting blanks from s1 */
					s1++; n1--;				/* will bring us to the end of s1 */
				}
				if (!n1 || !c1)
					return (priority);			/* equal except for priority */
				else
					return (c1);				/* s1 is longer */
			} else
				return(c1 - c2);				/* true difference between the strings */
		}
		if (c1 == '\0')						/* both strings ended with all seq #'s the same */
			return(priority);
		if (!priority)						/* priority tie breaker is set at 1st opportunity */
			priority = pri1 - pri2;
	}
	/*
	**  Hit the number limit on one or both of the strings.
	**  First bypass any trailing blanks on each string, then:
	**  - If both counts expired, the only difference between the strings can be their priorities.
	**  - The same is also true if one count expired and the pointer to the other string has
	**    reached the end of the string.
	**  - In all other cases, the shorter string collates earlier.
	*/
	while (n1 && (c1 = *s1) && c1 == ' ') {				/* bypass trailing blanks on s1 */
		s1++; n1--;
	}
	while (n2 && (c2 = *s2) && c2 == ' ') {				/* bypass trailing blanks on s2 */
		s2++; n2--;
	}

	return ((n1 == n2) ? priority					/* both expired */
			   : (n1 == 0) ? (*s2 == '\0') ? priority	/* s1 expired, s2 at string end */
						       : 0-*s2		/* s2 is longer */
				       : (*s1 == '\0') ? priority	/* s2 expired, s1 at string end */
						       : *s1 );		/* s1 is longer */
}


/*
 *=====================================================================
 * 16-bit string comparison routines
 *=====================================================================
 */


/*
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef strcmp16
#pragma _HP_SECONDARY_DEF _strcmp16 strcmp16
#define strcmp16 _strcmp16
#endif

strcmp16(s1, s2, file, err)
unsigned const char *s1, *s2, *file;
int *err;
{
	return (strncmp16(s1, s2, MAXINT, file, err));
}

/*
 * Compare strings (at most n characters)
 *	returns: s1>s2; >0  s1==s2; 0  s1<s2; <0
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef strncmp16
#pragma _HP_SECONDARY_DEF _strncmp16 strncmp16
#define strncmp16 _strncmp16
#endif

strncmp16(s1, s2, n, file, err)
register const unsigned char *s1, *s2;
unsigned const char *file;
register int n;
int *err;
{
	register int c1, c2;

	*err = 0;

	if (s1 == s2 || n <= 0)						/* nothing to compare?			*/
		return(0);
	else if (s1 == NULL)						/* comparing nothing against something?	*/
		return(0-*s2);
	else if (s2 == NULL)						/* comparing something against nothing?	*/
		return(*s1);
	
	while (n-- && (c1 = CHARADV(s1)) == (c2 = CHARADV(s2)))		/* skip down until a difference is found*/
		if (c1 == 0)
			return(0);
	
	/* got a difference or n expired */
	return (c1 - c2);						/* the char with lower value goes first	*/
}
