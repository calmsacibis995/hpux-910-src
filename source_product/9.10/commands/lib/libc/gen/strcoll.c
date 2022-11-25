/* @(#) $Revision: 70.2 $ */     
#ifdef PIC
#pragma HP_SHLIB_VERSION "4/92"
#endif
/* LINTLIBRARY */
/*
 *	strcoll()
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define strcmp _strcmp
#define strcoll _strcoll
#endif

#include <nl_ctype.h>
#include <setlocale.h>
#include <collate.h>

#ifndef NULL
#define NULL		(unsigned char *)0
#endif

/*
 * strcoll compares two strings and returns an integer greater than,
 * equal to, or less than 0, according to the native collating sequence
 * table if string1 is greater than, equal to, or less than string2.
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef strcoll
#pragma _HP_SECONDARY_DEF _strcoll strcoll
#define strcoll _strcoll
#endif

strcoll(s1, s2)
register const unsigned char *s1, *s2;
{

	if (!_nl_collate_on)
	    return(strcmp((char *)s1, (char *)s2));	/* "C" processing */
	else
	    return(docmp(s1, s2));			/* NLS processing */
}

/*
 * strcoll was broken up this way to maximize the performance of the
 * "C" language case.  With most of the code removed from strcoll
 * itself above, fewer registers need to be saved and restored prior
 * to the call to strcmp.  Of course, the increase in "C" language
 * performance comes at the expense of the performance of the other
 * languages.  But "C" has the most users (currently anyway) and
 * that's the one most (all?) benchmarks are run in.
 */
static docmp(s1, s2)
register const unsigned char *s1, *s2;
{
	register int	c1, c2;
	register int	index;
	register int	pri1, pri2, priority;
	register int	peekc1, peekc2;

	if (s1 == s2)	    		    /* nothing to compare?				*/
	    return(0);
	else if (s1 == NULL)	  /* comparing nothing against something? */
	    return(0-*s2);
	else if (s2 == NULL)	    /* comparing something against nothing?*/
	    return(*s1);

	/* no collation table yet for Kanji */
	if (_nl_mb_collate) {
	    for (;;) {
	    	c1 = CHARADV(s1);
	    	c2 = CHARADV(s2);
	    	if (c1 != c2)
	    		break;
	    	if(c1 == '\0')
	    		return(0);
	    }
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
	    } while (*++s1 == *++s2);
	}

	priority = peekc1 = peekc2 = 0;

	/* perform rest of compare using the collation table */
	while ( 1 ) {
	    /*
	    **  Get the sequence number and priority of the next
	    **  character in the first string.
	    */
	    if (c1 = peekc1) {	    /* if 2nd char of a 1 to 2 is waiting */
	    	peekc1 = 0;		/*    use it */
	    } else
	    	/* 
		 * otherwise, get a new character 
		 * use table to get priority 
		 * and sequence number 
		 */
		while (c1 = *s1++) {	
		    pri1 = (int)_pritab[c1];

		    /* 
		     * (char becomes its sequence number+1 so seq #'s 
		     * are offset from '\0') 
		     */
		    c1 = (int)_seqtab[c1] + 1;

		    /* high 2 bits indicate char type */
		    switch (pri1 >> 6) {	

		    case 0:			/* 1 to 1 */
		    break;

		    case 1:			/* 2 to 1 */
		    index = pri1 & MASK077;

		    /* 
		     * check each possible 2nd char 
		     * against actual next char 
		     */
		    while (_tab21[index].ch1 != ENDTABLE) {	
		        if (*s1 == _tab21[index].ch2) {	
		    		s1++;		/* got a match */
		    		c1 = _tab21[index].seqnum + 1;
		    		break;
		    	}
		    	index ++;
		    }

		    /* 
		     * whether we got a match or not, where we ended up 
		     * has the priority 
		     */
		    pri1 = _tab21[index].priority & MASK077;
		    break;

		    case 2:			/* 1 to 2 */
		    index = pri1 & MASK077;

		    /* hold on to the 2nd char */
			peekc1 = _tab12[index].seqnum + 1;	

		    /* combined priority */
			pri1 = _tab12[index].priority;		
		    break;

		    case 3:		/* don't_care_character */
		    continue;		/* skip to next character */
		    }
		    break;
	    	}
	    /*
	    **  Get the sequence number and priority of the next
	    **  character in the second string.
	    */
	    if (c2 = peekc2) {		/* if 2nd char of a 1-2 is waiting */
	    	peekc2 = 0;		/*    use it */
	    } else
	    	while (c2 = *s2++) {	/* otherwise, get a new character */

		    /* 
		     * use table to get priority 
		     * and sequence number 
		     */
		    pri2 = (int)_pritab[c2];	

		    /* 
		     * (char becomes its sequence number+1 
		     * so seq #'s are offset from '\0') 
		     */
		    c2 = (int)_seqtab[c2] + 1;	
		    	
		    /* high 2 bits indicate char type */
		    switch (pri2 >> 6) {	

		    case 0:			/* 1 to 1 */
		    break;

		    case 1:			/* 2 to 1 */
		    index = pri2 & MASK077;

		    /* 
		     * check each possible 2nd char
		     * against actual next char 
		     */
		    while (_tab21[index].ch1 != ENDTABLE) {	
		    	if (*s2 == _tab21[index].ch2) {	
		    		s2++;		/* got a match */
		    		c2 = _tab21[index].seqnum + 1;
		    		break;
		    	}
		    	index ++;
		    }

		    /* 
		     * whether we got a match or not, where we ended up 
		     * has the priority 
		     */
		    pri2 = _tab21[index].priority & MASK077;
		    break;

		    case 2:				/* 1 to 2 */
		    index = pri2 & MASK077;

		    /* hold on to the 2nd char */
		    peekc2 = _tab12[index].seqnum + 1;	
		    pri2 = _tab12[index].priority;	/* combined priority */
		    break;

		    case 3:		/* don't_care_character */
		    continue;		/* skip to next character */
		    }
		    break;
	    	}

	    /* sequence #'s differ for these chars */
	    if (c1 != c2)		
	    	return(c1 - c2);

	    /* both strings ended with all seq #'s the same */
	    if (c1 == '\0')	
	    	return(priority);

	    /* priority tie breaker is set at 1st opportunity */
	    if (!priority)	
	    	priority = pri1 - pri2;
	}
}
