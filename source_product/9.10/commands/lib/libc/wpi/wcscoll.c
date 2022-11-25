/* static char *HPUX_ID = "@(#) wcscoll.c $Revision: 70.3 $"; */

/*LINTLIBRARY*/


#define _HPUX_SOURCE	/* Needed for _nl_* enums indirect in <setlocale.h> */
#include <setlocale.h>
#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef wcscoll
#    pragma _HP_SECONDARY_DEF _wcscoll wcscoll
#    define wcscoll _wcscoll

#    define wcscmp _wcscmp
#endif


/*
 * wcscoll compares two wide strings and returns an integer greater than,
 * equal to, or less than 0, according to the native collating sequence
 * table if string1 is greater than, equal to, or less than string2.
 * wcscoll is based upon strcoll.  If the LC_COLLATE locale is a multibyte
 * locale, then we use machine order (wcscmp).  In the future, when full
 * collation tables are added, this will need to change.
 */

int wcscoll(const wchar_t *ws1, const wchar_t *ws2)

{

	int	c1, c2;
	int	index;
	int	pri1, pri2, priority;
	int	peekc1, peekc2;


	/* For "C" locale and all multibyte languages, use machine order
	   for collation. */
	if (!_nl_collate_on || _nl_mb_collate)
	    return(wcscmp(ws1, ws2));

	if (ws1 == ws2)	    	    /* nothing to compare? */
	    return(0);
	else if (ws1 == NULL)	  /* comparing nothing against something? */
	    return(0-*ws2);
	else if (ws2 == NULL)	    /* comparing something against nothing?*/
	    return(*ws1);

	/*
	** If not a language with 2-to-1 mappings, quickly skip past identical
	** leading characters before starting slower collate table processing.
	*/
	if (!_nl_map21 && *ws1 == *ws2) {
	    do {
	    	if (*ws1 == L'\0')
	    		return(0);
	    } while (*++ws1 == *++ws2);
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
		while (c1 = *ws1++) {	
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
		        if (*ws1 == (wchar_t)_tab21[index].ch2) {	
		    		ws1++;		/* got a match */
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
	    	while (c2 = *ws2++) {	/* otherwise, get a new character */

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
		    	if (*ws2 == (wchar_t)_tab21[index].ch2) {	
		    		ws2++;		/* got a match */
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
