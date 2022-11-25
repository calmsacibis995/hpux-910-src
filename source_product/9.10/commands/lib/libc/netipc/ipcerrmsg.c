/* @(#) $Revision: 66.2 $ */

#include "nipc.h"

#ifdef _NAMESPACE_CLEAN
#define ipcerrstring _ipcerrstring
#define memcpy _memcpy
#define strlen _strlen
#endif /* _NAMESPACE_CLEAN */

/*
 * this routine assumes that speed in mapping error is less important
 * than code space.
 *
 *	Note, len is ignored on input because that's what the 3000 does.
 */

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _ipcerrmsg ipcerrmsg
#define ipcerrmsg _ipcerrmsg
#endif /* _NAMESPACE_CLEAN */

void
ipcerrmsg( error , buffer , len , result )
	int error, *len, *result;
	char *buffer;
{
	char	*cp=0;
	char	*ipcerrstring();
	int 	length;

	if (!buffer) {
		ERR_RETURN(NSR_BOUNDS_VIO);
	}

	if (cp = ipcerrstring(error)) {
		length= MIN(strlen(cp)+1, *len);
		memcpy(buffer, cp, length);
		buffer[length] = 0;
		*len = length;
		ERR_RETURN(NSR_NO_ERROR);
	}
	else
		/* error number not found */
		ERR_RETURN(NSR_ERRNUM);
}
