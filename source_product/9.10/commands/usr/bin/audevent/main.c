/*
 * @(#) $Revision: 64.1 $
 */

/*
 * Function Name:	main
 *
 * Abstract:		Audevent entry and exit point.
 *			Uses NLS message numbers 21-30.
 *
 * Exit Value:		exits with 0 on success, 1 on failure
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

#include <sys/types.h>
#include <sys/audit.h>
#include <stdio.h>
#include "define.h"
#include "extern.h"
#include "global.h"

#define	reg		register

#define	ARGUMENTS	"[-P|-p] [-F|-f] [-E] [-e event] ... [-S] [-s syscall] ..."

int
main(ac, av)
int ac;
char *av[];
{
	reg	int		i = 0;

	auto	char *		arguments = ARGUMENTS;
	auto	char *		command = basename(*av);
	auto	char *		function = "main";


	av++; ac--;

#ifdef NLS
	nlmsg_fd = catopen("audevent", 0);
#endif /* NLS */

	/*
	 * first check for omnipotence
	 */
	if (getuid() && geteuid()) {
		(void)fprintf(stderr,
			(catgets(nlmsg_fd,NL_SETN,21, "Not superuser.\n")));
#ifdef NLS
		(void)catclose(nlmsg_fd);
#endif /* NLS */
		exit(1);
	}

	/*
	 * this command is self-auditing
	 */
	if (audswitch(AUD_SUSPEND) != 0) {
		(void)error(function,
			catgets(nlmsg_fd,NL_SETN,22,"audswitch(AUD_SUSPEND) failed: %s\n"),_perror());
#ifdef NLS
		(void)catclose(nlmsg_fd);
#endif /* NLS */
		exit(1);
	}

	/*
	 * if no options or arguments exist, -E is implied
	 */
	if (ac <= 0) {
		for (i = 0; i < EVMAPSIZE; i++) {
			if (mark_ev(i) != 0) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,23,"mark_ev(%d) failed\n"),i);
#ifdef NLS
				(void)catclose(nlmsg_fd);
#endif /* NLS */
				exit(1);
			}
		}
	}
	else {
		/*
		 * get options
		 */
		if (get_opts(&ac, &av) < 0) {
			usage(command, arguments);
#ifdef NLS
			(void)catclose(nlmsg_fd);
#endif /* NLS */
			exit(1);
		}
	}

	/*
	 * if arguments exist, give user a clue
	 */
	if (ac > 0) {
		(void)error(function,
			catgets(nlmsg_fd,NL_SETN,24,"unknown argument: %s\n"),*av);
		usage(command, arguments);
#ifdef NLS
		(void)catclose(nlmsg_fd);
#endif /* NLS */
		exit(1);
	}

	/*
	 * if neither pass nor fail is set, then default action
	 * is to show events and/or syscalls  being audited
	 */
	if ((pass == -1) && (fail == -1)) {
		if (show_audit() != 0) {
			(void)error(function,
				catgets(nlmsg_fd,NL_SETN,25,"show_audit() failed\n"));
#ifdef NLS
			(void)catclose(nlmsg_fd);
#endif /* NLS */
			exit(1);
		}
	}
	/*
	 * otherwise, set events and/or syscalls to audit
	 */
	else {
		if (set_audit() != 0) {
			(void)error(function,
				catgets(nlmsg_fd,NL_SETN,26,"set_audit() failed\n"));
#ifdef NLS
			(void)catclose(nlmsg_fd);
#endif /* NLS */
			exit(1);
		}
	}
#ifdef NLS
	(void)catclose(nlmsg_fd);
#endif /* NLS */
	exit(0);
}
