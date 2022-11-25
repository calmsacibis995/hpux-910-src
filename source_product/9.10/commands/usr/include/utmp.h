/* @(#) $Revision: 66.5 $ */

#ifndef _UTMP_INCLUDED  /* allows multiple inclusion */
#define _UTMP_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

/*	<sys/types.h> must be included.					*/

#define	UTMP_FILE	"/etc/utmp"
#define	WTMP_FILE	"/etc/wtmp"
#define	BTMP_FILE	"/etc/btmp"
#define	ut_name	ut_user


struct utmp
  {
	char ut_user[8] ;		/* User login name */
	char ut_id[4] ; 		/* /etc/lines id(usually line #) */
	char ut_line[12] ;		/* device name (console, lnxx) */
#ifdef _CLASSIC_ID_TYPES
	unsigned short ut_filler_pid ;
	short ut_pid ;
#else
	pid_t ut_pid ;			/* process id */
#endif
	short ut_type ; 		/* type of entry */
	struct exit_status
	  {
	    short e_termination ;	/* Process termination status */
	    short e_exit ;		/* Process exit status */
	  }
	ut_exit ;			/* The exit status of a process
					 * marked as DEAD_PROCESS.
					 */
	unsigned short ut_reserved1 ;	/* Reserved for future use */
	time_t ut_time ;		/* time entry was made */
	char ut_host[16] ;		/* host name, if remote; 
				           NOT SUPPORTED */
	unsigned long ut_addr ;		/* Internet addr of host, if remote */
  } ;

/*	Definitions for ut_type						*/

#define	EMPTY		0
#define	RUN_LVL		1
#define	BOOT_TIME	2
#define	OLD_TIME	3
#define	NEW_TIME	4
#define	INIT_PROCESS	5	/* Process spawned by "init" */
#define	LOGIN_PROCESS	6	/* A "getty" process waiting for login */
#define	USER_PROCESS	7	/* A user process */
#define	DEAD_PROCESS	8
#define	ACCOUNTING	9
#define	UTMAXTYPE	ACCOUNTING   /* Largest legal value of ut_type */


/*	Special strings or formats used in the "ut_line" field when	*/
/*	accounting for something other than a process.			*/
/*	No string for the ut_line field can be more than 11 chars +	*/
/*	a NULL in length.						*/

#define	RUNLVL_MSG	"run-level %c"
#define	BOOT_MSG	"system boot"
#define	OTIME_MSG	"old time"
#define	NTIME_MSG	"new time"

#  if defined(__STDC__) || defined(__cplusplus)
   extern struct utmp *getutent(void);
   extern struct utmp *getutid(struct utmp *);
   extern struct utmp *getutline(struct utmp *);
   extern void pututline(struct utmp *);
   extern struct utmp *_pututline(struct utmp *);
   extern void setutent(void);
   extern void endutent(void);
#else /* __STDC__ || __cplusplus */
   extern struct utmp *getutent();
   extern struct utmp *getutid();
   extern struct utmp *getutline();
   extern void pututline();
   extern struct utmp *_pututline();
   extern void setutent();
   extern void endutent();
#endif /* __STDC__ || __cplusplus */

#ifdef __cplusplus
}
#endif

#endif /* _UTMP_INCLUDED */
