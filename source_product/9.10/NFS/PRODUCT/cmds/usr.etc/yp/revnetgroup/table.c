/* @(#)table.c	$Revision: 1.18.109.1 $	$Date: 91/11/19 14:22:31 $  
table.c	2.1 86/04/16 NFSSRC 
static  char sccsid[] = "table.c 1.1 86/02/05 (C) 1985 Sun Microsystems, Inc.";
*/

#include <ctype.h>
#include "util.h"
#include "table.h"



/*
 * Hash table manager. Store/lookup strings, keyed by string
 */

/*
 * Generate the key into the table using the first two letters
 * of "str".  The table is alphabetized, with no distinction between
 * upper and lower case.  Non-letters are given least significance.
 */
int
tablekey(str)
	register char *str;
{
#	define TOLOWER(c) (islower(c) ? c : \
							(isupper(c) ? tolower(c) : ('a'+NUMLETTERS-1)))

	register int c1,c2;

	c1 = *str++;
	c2 = *str;
	if (c1 == EOS) {
		c2 = EOS;	/* just in case */
	}
	c1 = TOLOWER(c1) - 'a';
	c2 = TOLOWER(c2) - 'a';
	return (c1*NUMLETTERS + c2);
}


void
store(table,key,datum)
	stringtable table;
	char *key;
	char *datum;
{
	int index;
	tablelist cur,new;

	index = tablekey(key);
	cur = table[index];	

	new = MALLOC(tablenode);
	new->key = key;
	new->datum = datum;
	new->next = cur;
	table[index] = new;
}
	
	
char *
lookup(table,key)
	stringtable table;
	char *key;
{
	tablelist cur;

	cur = table[tablekey(key)];
	while (cur && strcmp(cur->key,key)) {
		cur = cur->next;
	} 
	if (cur) {
		return(cur->datum);
	} else {
		return(NULL);
	}
}
		
	
