/*
 * @(#) $Revision: 63.1 $
 */

/*
 * Function Name:	mark_sc
 *
 * Abstract:		Set sc_map mark flag for "index;" it is a trivial
 *			function, but may be extended in the future.
 *
 * Return Value:	0
 */

#include <sys/types.h>
#include <sys/audit.h>
#include "define.h"
#include "extern.h"

int
mark_sc(index)
int index;
{
	sc_map[index].sc_mark = 1;
	return(0);
}
