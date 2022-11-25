/*
 * @(#) $Revision: 63.1 $
 */

/*
 * Function Name:	ev_index
 *
 * Abstract:		Return index of "name" in sc_map.
 *
 * Return Value:	returns >= 0 on success, -1 on failure
 */

#include <sys/types.h>
#include <sys/audit.h>
#include "define.h"
#include "extern.h"

#define	reg		register

int
sc_index(name)
char *name;
{
	reg	int		i = 0;


	for (i = 0; i < SCMAPSIZE; i++) {
		if (strcmp(name, sc_map[i].sc_name) == 0) {
			return(i);
		}
	}
	return(-1);
}
