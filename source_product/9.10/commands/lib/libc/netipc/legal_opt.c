/* @(#) $Revision: 66.2 $ */

#include "nipc.h"

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _legal_option legal_option
#define legal_option _legal_option
#endif /* _NAMESPACE_CLEAN */

legal_option(code)
	int 	code;
{
	static int options[] = {
		NSO_MAX_SEND_SIZE,
		NSO_MAX_RECV_SIZE,
		NSO_MAX_CONN_REQ_BACK,
		NSO_DATA_OFFSET,
		NSO_PROTOCOL_ADDRESS,
		NSO_MIN_BURST_IN,
		NSO_MIN_BURST_OUT
	};
#define MAXOPTIONS (sizeof(options)/sizeof(int))
	int 	i;

	for (i=0; i<MAXOPTIONS; i++)
		if (code == options[i])
			break;

	if (i == MAXOPTIONS)
		return(0);
	else
		return(1);
}
