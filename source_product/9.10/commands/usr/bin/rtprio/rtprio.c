static char *HPUX_ID = "@(#) $Revision: 64.1 $";
/*
 *	rtprio executes the command with a realtime priority, or
 *	changes the realtime priority of currently executing process
 *	pid.  Realtime priorities range from zero (highest) to 127 (lowest).
 *	Realtime processes are  not subject to priority degradation and 
 *	are all of greater (scheduling) importance than non-realtime
 *	processes.
 *
 *	Command will not be scheduled, pid's realtime priority will not
 *	be changed, if it is not a member of a group having MAG_RTPRIO
 *	access and is not the super-user.  When changing the realtime
 *	priority of a currently executing process, the effective user ID 
 *	of the calling process must be superuser, or the real or effective
 *	user ID must match the real or effective user ID of the process to
 *	be modified.
 *	
 *	If -t is specified instead of a realtime priority then
 *	rtprio executes command with a timeshare (non-realtime) priority
 *	or charges the currently executing process pid from a possibly
 *	realtime priority to a timeshare priority.  The former is useful
 *	to spawn a timeshare priority command from a realtime priority 
 *	shell.
 *
 *	usage: rtprio priority command [ arguments ]
 *	       rtprio priority -pid
 *	       rtprio -t command [ arguments ]
 *	       rtprio -t -pid
 *
 *
 *	rtprio exits with 0, upon sucessfully completition.
 *	                  1, if command is not excutable or
 *	                     pid does not exists.
 *	                  2, if command (pid) lacks realtime
 *	                     capability, or the invoker's
 *	                     effective user ID is not superuser
 *	                     or effective user ID does not match   
 *	                     the real or effective user ID of the
 *	                     process to be changed.
 *
 *	author: B.C.
 *
 *
 */
      

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <sys/rtprio.h>

extern int errno;

main(argc, argv)
int argc;
char *argv[];
 
{
extern char *sys_errlist[];

int priority, pid;
char *ptr;

     if (argc < 3) usage();

     if ( argv[1][0] == '-' && argv[1][1] == 't') {

	priority = RTPRIO_RTOFF;

	}
     else {

	priority = strtol(argv[1], &ptr, 10);

        if (priority < 0 || priority > 127 || (*ptr != NULL) ) {
		fputs("rtprio: priority ranges from 0 to 127\n", stderr);
		exit(1);
	}


     }

     /* see if it is a process id (pid) */
     if (argv[2][0] == '-') {
	argv[2][0] = ' ';
	pid = strtol(argv[2], &ptr, 10);
	if (*ptr) {
		fputs("rtprio: No such process\n", stderr);
		exit(1);
	}
	pid = (unsigned) pid;

	/* do the real time priority call with process id
		  and the priority                         */

	do_rtprio(pid,priority);
	exit(0);

        }

     else {


	/* do the real time priority call with process id 0,
		  and the priority                         */

	do_rtprio(0,priority);
	execvp(argv[2], &argv[2]);

	fputs(argv[2], stderr);
	fputs(": ", stderr);
	fputs(sys_errlist[errno], stderr);
	exit(2);
     }


}

usage()
{
    fputs("usage: rtprio priority command [ arguments ]\n", stderr);
    fputs("       rtprio priority -pid\n", stderr);
    fputs("       rtptio -t command [ arguments ]\n", stderr);
    fputs("       rtprio -t -pid\n", stderr);
    exit(1);
}

do_rtprio(pid,priority)

int pid,priority;

{

        if ( rtprio(pid,priority) == -1 ) {
		switch (errno) {

		/* priority is not RTPRIO_NOCHG or in the
		   range 0 to 127                          */

		case EINVAL:
			perror("rtprio");
			exit(1);

		/*  No process can be found corresponding
		    to that specified by pid              */

		case ESRCH:
			perror("rtprio");
			exit(1);

		/*  The sending process is not the super-user
		    and neither its real or effective user-id 
		    match the real or effective user-id of the
		    receiving process.

		    The sending process is not the super-user
		    and is not a member of a group having
		    PRIV-RTPRIO capability.                    */

		case EPERM:
			perror("rtprio");
			exit(2);

		default:
			perror("rtprio");
			exit(5);
		}
	}


}

