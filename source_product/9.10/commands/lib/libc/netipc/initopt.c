/* @(#) $Revision: 66.1 $ */

#include "nipc.h"

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _initopt initopt
#define initopt _initopt
#endif /* _NAMESPACE_CLEAN */

void
initopt( opt, num_entries, result )
	char	*opt;
	short	num_entries;
	short	*result;

{			/***** initopt *****/
	struct opthead	*head;


	/* check for a bad options pointer */
	if (opt == 0)
		ERR_RETURN(NSR_ADDR_OPT);

	if (num_entries < 0)
		ERR_RETURN(NSR_OPT_TOTAL);

	/* cast the head of the buffer */
	head = (struct opthead *)opt;

	/* initialize the fields of the header */
	head->opt_length = num_entries * sizeof( struct optentry);
	head->opt_entries = num_entries;

	ERR_RETURN(NSR_NO_ERROR);
}			/***** initopt *****/
