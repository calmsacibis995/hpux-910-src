/* @(#) $Revision: 66.1 $ */

#include "nipc.h"

/* return the number of bytes needed for the non-data portion of a
 * Netipc option buffer
 */

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _optoverhead optoverhead
#define optoverhead _optoverhead
#endif /* _NAMESPACE_CLEAN */

optoverhead( num_entries, result)
	short	num_entries;
	short	*result;
{
	if (num_entries < 0) {
		*result = NSR_OPT_TOTAL;
		return(0);
	}
	else {
		*result = NSR_NO_ERROR;
		return( num_entries*sizeof(struct optentry)+
			sizeof(struct opthead));
	}

}
