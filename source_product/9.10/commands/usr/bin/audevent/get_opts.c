/*
 * @(#) $Revision: 64.1 $
 */

/*
 * Function Name:	get_opts
 *
 * Abstract:		Parse options.
 *                      Uses NLS message numbers 1-20.
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
get_opts(ac, av)
int *ac;
char **av[];
{
	reg	int		i = 0;
	reg	int		rvalue = 0;
	reg	char *		sp;

	auto	char *		function = "get_opts";


	while ((*ac > 0) && (***av == '-')) {
		sp = **av;
		(*av)++; (*ac)--;
		while (*(++sp)) switch (*sp) {

		    /*
		     * F option
		     */
		    case 'F':
			if (fail == 0) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,1,"only one of -F or -f allowed\n"));
				rvalue = -1;
				break;
			}
			fail = 1;
			break;

		    /*
		     * f option
		     */
		    case 'f':
			if (fail == 1) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,2,"only one of -F or -f allowed\n"));
				rvalue = -1;
				break;
			}
			fail = 0;
			break;

		    /*
		     * P option
		     */
		    case 'P':
			if (pass == 0) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,3,"only one of -P or -p allowed\n"));
				rvalue = -1;
				break;
			}
			pass = 1;
			break;

		    /*
		     * p option
		     */
		    case 'p':
			if (pass == 1) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,4,"only one of -P or -p allowed\n"));
				rvalue = -1;
				break;
			}
			pass = 0;
			break;

		    /*
		     * E option
		     */
		    case 'E':
			for (i = 0; i < EVMAPSIZE; i++) {
				if (mark_ev(i) != 0) {
					(void)error(function,
						catgets(nlmsg_fd,NL_SETN,5,"mark_ev(%d) failed\n"),i);
					rvalue = -1;
				}
			}
			break;

		    /*
		     * e option
		     */
		    case 'e':
			if ((*ac) <= 0) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,6,"'%c': event argument missing\n"),*sp);
				rvalue = -1;
				break;
			}
			if ((i = ev_index(**av)) < 0) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,7,"unknown event: %s\n"),**av);
				rvalue = -1;
			}
			else {
				if (mark_ev(i) != 0) {
					(void)error(function,
						catgets(nlmsg_fd,NL_SETN,8,"mark_ev(%d) failed\n"),i);
					rvalue = -1;
				}
			}
			(*av)++; (*ac)--;
			break;

		    /*
		     * S option
		     */
		    case 'S':
			for (i = 0; i < SCMAPSIZE; i++) {
				if (mark_sc(i) != 0) {
					(void)error(function,
						catgets(nlmsg_fd,NL_SETN,9,"mark_sc(%d) failed\n"),i);
					rvalue = -1;
				}
			}
			break;

		    /*
		     * s option
		     */
		    case 's':
			if ((*ac) <= 0) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,10,"'%c': syscall argument missing\n"),*sp);
				rvalue = -1;
				break;
			}
			if ((i = sc_index(**av)) < 0) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,11,"unknown syscall: %s\n"),**av);
				rvalue = -1;
			}
			else {
				if (mark_sc(i) != 0) {
					(void)error(function,
						catgets(nlmsg_fd,NL_SETN,12,"mark_sc(%d) failed\n"),i);
					rvalue = -1;
				}
			}
			(*av)++; (*ac)--;
			break;

		    /*
		     * unknown option
		     */
		    default:
			(void)error(function,
				catgets(nlmsg_fd,NL_SETN,13,"'%c': unknown option\n"),*sp);
			rvalue = -1;
			break;
		}
	}
	return(rvalue);
}
