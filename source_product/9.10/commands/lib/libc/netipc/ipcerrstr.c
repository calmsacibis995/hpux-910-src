/* @(#) $Revision: 66.2 $ */

#include "nipc.h"

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF __ipc_result_map ipc_result_map
#define ipc_result_map __ipc_result_map
#endif /* _NAMESPACE_CLEAN */

struct result_map ipc_result_map[] = {
	0, "no error occurred",
	3, "parameter bounds violation",
	4, "network is down",
	5, "illegal socket type",
	6, "illegal protocol",
	7, "illegal flags",
	8, "illegal option",
	9, "protocol not active",
	10, "socket type/protocol mismatch",
	11, "no memory",
	14, "illegal protocol address in opt array",
	15, "no file table entries available",
	18, "error in opt array syntax",
	21, "duplicate option in opt array",
	24, "error in maximum connection queued option",
	28, "illegal name length",
	29, "illegal descriptor",
	30, "cannot name vc socket",
	31, "duplicate name",
	36, "name table is full",
	37, "name not found",
	38, "not owner",
	39, "illegal node name syntax",
	40, "node does not exist",
	43, "can not send lookup request",
	44, "no response from remote registry",
	45, "aborted due to signal",
	46, "can not interpret path report",
	47, "received bad registry msg",
	50, "illegal data length value",
	51, "illegal destination descriptor",
	52, "source and destination have different protocols",
	53, "source and destination have different socket types",
	54, "illegal socket descriptor",
	56, "would block",
	59, "timed out",
	60, "socket limit",
	62, "must call ipcrecv",
	64, "remote abort",
	65, "local abort",
	66, "not a connection descriptor",
	74, "illegal request",
	76, "illegal timeout value",
	99, "illegal vector data length",
	100, "too many vectored data descriptors",
	106, "address in use",
	109, "graceful release; can not receive",
	116, "destination unreachable",
	118, "version number mismatch",
	124, "bad opt entry number",
	125, "bad opt entry length",
	126, "bad option total",
	127, "can not read option",
	1002, "bad threshold value",
	2003, "not allowed",
	2004, "message size too big",
	2005, "address not available",
};

#define	NRESULTS	sizeof(ipc_result_map)/sizeof(struct result_map)


/* return a pointer to an error string for the error or 0 if not found */
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _ipcerrstring ipcerrstring
#define ipcerrstring _ipcerrstring
#endif /* _NAMESPACE_CLEAN */

char *
ipcerrstring(error)
int	error;
{
	extern struct result_map ipc_result_map[];

	int i;
	for (i=0; i<NRESULTS; i++)
		if(ipc_result_map[i].res_code == error)
			return(ipc_result_map[i].res_string);
	return(0);
}
