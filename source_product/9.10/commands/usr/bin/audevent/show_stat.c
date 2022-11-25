/*
 * @(#) $Revision: 63.1 $
 */

/*
 * Function Name:	show_status
 *
 * Abstract:		Print status of event or syscall.
 *
 * Return Value:	none
 */

#include <sys/types.h>
#include <sys/audit.h>

void
show_status(class, name, status)
char *class;
char *name;
int status;
{
	switch (status) {
	    case NONE:
		/*
		 * ignore
		 */
		break;
	    case PASS:
		printf("%10s: %16s: success        \n", class, name);
		break;
	    case FAIL:
		printf("%10s: %16s:         failure\n", class, name);
		break;
	    case BOTH:
		printf("%10s: %16s: success failure\n", class, name);
		break;
	}
}
