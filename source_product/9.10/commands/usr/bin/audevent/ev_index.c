/*
 * @(#) $Revision: 63.1 $
 */

/*
 * Function Name:	ev_index
 *
 * Abstract:		Return index of "name" in ev_map.
 *
 * Return Value:	returns >= 0 on success, -1 on failure
 */

#include <sys/types.h>
#include <sys/audit.h>
#include "define.h"
#include "extern.h"

#define	reg		register

int
ev_index(name)
char *name;
{
	reg	int		i = 0;


	for (i = 0; i < EVMAPSIZE; i++) {
		if (strcmp(name, ev_map[i].ev_name) == 0) {
			return(i);
		}
	}
	return(-1);
}
