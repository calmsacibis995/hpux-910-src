/* @(#) $Revision: 66.2 $ */   
/* lpshut -- shut the line printer scheduler
	All busy printers will stop printing,
	but no requests will be cancelled because of this.
*/

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 10					/* set number */
#include <msgbuf.h>
#endif NLS

#include	"lp.h"
#ifdef TRUX
#include <sys/security.h>
#endif

main(argc, argv)
int argc;
char *argv[];
{
	extern char *f_name;

#ifdef SecureWare
	if(ISSECURE){
            set_auth_parameters(argc, argv);
#ifdef B1
	    if(ISB1){
        	initprivs();
        	(void) forcepriv(SEC_ALLOWMACACCESS);
	    }
#endif
	}
#endif  
#ifdef NLS
	nl_catopen("lp");
#endif NLS

	f_name = argv[0];

	if(! ISADMIN)
		fatal(ADMINMSG, 1);

	if(chdir(SPOOL) == -1)
		fatal((nl_msg(1, "spool directory non-existent")), 1);
	if(enqueue(F_QUIT, "") == 0) {
		/* Scheduler is running */
		printf((nl_msg(2, "scheduler stopped\n")));
		exit(0);
	}
	else {
		/* Scheduler is not running -- remove the FIFO and SCHEDLOCK */
		unlink(FIFO);
		unlink(SCHEDLOCK);
		fatal((nl_msg(3, "scheduler not running")), 1);
	}
}
