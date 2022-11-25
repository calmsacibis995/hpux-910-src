/*
 * @(#) $Revision: 64.1 $
 */

/*
 * Function Name:	mark_ev
 *
 * Abstract:		Set ev_map mark flag for "index" and mark
 *			associated syscalls with same event type.
 *                      Uses NLS message numbers 31-40.
 *
 * Return Value:	returns 0 on success, -1 on failure
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
extern nl_catd nlmsg_fd;
#endif NLS

#include <sys/types.h>
#include <sys/audit.h>
#include "define.h"
#include "extern.h"

#define	reg		register

int
mark_ev(index)
int index;
{
	reg	int		i = 0;

	auto	char *		function = "mark_ev";


	/*
	 * first mark event table
	 */
	ev_map[index].ev_mark = 1;

	/*
	 * now mark all syscalls with same type as event
	 */
	for (i = 0; i < SCMAPSIZE; i++) {
		if (sc_map[i].sc_type == ev_map[index].ev_type) {
			if (mark_sc(i) != 0) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,31,"mark_sc(%d) failed\n"),i);
				return(-1);
			}
		}
	}
	return(0);
}
