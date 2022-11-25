/*
 * @(#) $Revision: 63.1 $
 */

/*
 * Function Name:	usage
 *
 * Abstract:		Print usage message.
 *			Uses NLS message numbers 61-70.
 *
 * Return Value:	none
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
extern nl_catd nlmsg_fd;
#endif NLS

#include <stdio.h>

void
usage(command, arguments)
char *command;
char *arguments;
{
	(void)fprintf(stderr,
		(catgets(nlmsg_fd,NL_SETN,61, "usage: %s")), command);
	if (arguments != NULL) {
		(void)fprintf(stderr, " %s", arguments);
	}
	(void)fputc('\n', stderr);
	return;
}
