/* @(#) $Revision: 64.4 $ */   
/*LINTLIBRARY*/
/*
 * Tree search algorithm, generalized from Knuth (6.2.2) Algorithm T.
 *
 */

#include <search.h>
typedef char *POINTER;
typedef struct node { POINTER key; struct node *llink, *rlink; } NODE;

#define	NULL	0


/*	tfind - find a node, or return 0	*/
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _tfind tfind
#define tfind _tfind
#endif
char *
tfind(key, rootp, compar)
POINTER	key;			/* Key to be located */
register NODE	**rootp;	/* Address of the root of the tree */
int	(*compar)();		/* Comparison function */
{
	if (rootp == NULL)
		return (NULL);
	while (*rootp != NULL) {			/* T1: */
		int r = (*compar)(key, (*rootp)->key);	/* T2: */
		if (r == 0)
			return ((char *)*rootp);	/* Key found */
		rootp = (r < 0) ?
		    &(*rootp)->llink :		/* T3: Take left branch */
		    &(*rootp)->rlink;		/* T4: Take right branch */
	}
	return (NULL);
}
