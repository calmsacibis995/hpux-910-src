/* @(#) $Revision: 70.1 $ */     

#include <dirent.h>
#ifdef SecureWare
#include <sys/security.h>
#include <sys/audit.h>
#include <prot.h>
#include <protcmd.h>
#endif

#define FALSE		0
#define TRUE		1
#define MINUTE		60L
#define HOUR		60L*60L
#define DAY		24L*60L*60L
#define	NQUEUE		26		/* number of queues available */
#define	ATEVENT		0
#define BATCHEVENT	1
#define CRONEVENT	2

#define ADD		'a'
#define DELETE		'd'
#define	AT		'a'
#define CRON		'c'
#define ADDNOTIFY       'y'       /* POSIX draft 7: notify user job completed */

#define	QUE(x)		('a'+(x))
#define RCODE(x)	(((x)>>8)&0377)
#define TSTAT(x)	((x)&0377)

#define	FLEN	(MAXNAMLEN + 1)
#define	LLEN	9

/* structure used for passing messages from the
   at and crontab commands to the cron			*/

struct	message {
	char	etype;
	char	action;
	char	fname[FLEN];
	char	logname[LLEN];
#if defined(SecureWare) && defined(B1)
        mand_ir_t mlevel;
#endif
};

/* anything below here can be changed */

#define CRONDIR		"/usr/spool/cron/crontabs"
#define ATDIR		"/usr/spool/cron/atjobs"
#define ACCTFILE	"/usr/lib/cron/log"
#define CRONALLOW	"/usr/lib/cron/cron.allow"
#define CRONDENY	"/usr/lib/cron/cron.deny"
#define ATALLOW		"/usr/lib/cron/at.allow"
#define ATDENY		"/usr/lib/cron/at.deny"
#define PROTO		"/usr/lib/cron/.proto"
#define	QUEDEFS		"/usr/lib/cron/queuedefs"
#define	FIFO		"/usr/lib/cron/FIFO"

#ifdef AUDIT
#define	CRONAIDS	"/usr/spool/cron/.cronaids"
#define	ATAIDS		"/usr/spool/cron/.ataids"
#endif /* AUDIT */

#define SHELL		"/bin/sh"	/* shell to execute */

#define CTLINESIZE	1000	/* max chars in a crontab line */
#define UNAMESIZE	20	/* max chars in a user name */
