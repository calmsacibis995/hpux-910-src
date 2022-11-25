/*
 * proctable.h -- definitions having to do with process table
 *                entries.
 */

#ifndef _PROCTABLE_INCLUDED
#define _PROCTABLE_INCLUDED

#include <sys/types.h>

#ifndef _GID_T  /* KLUDGE to get to compile on 6.5 system */
typedef long pid_t;
typedef long gid_t;
typedef long uid_t;
#define _GID_T
#define _UID_T
#endif

/*
 * The process table.  This table is implemented as a linked list
 * of structures.
 */
typedef struct PROC_TABLE proctbl_t;
struct PROC_TABLE
{
    char p_id[4];	   /* Four letter unique id of process */
    pid_t p_pid;  	   /* Process id */
    short p_count;	   /* Number of respawns in current series. */
    long p_time;	   /* Start time for a series of respawns */
    short p_flags;         /* flags describing status of this entry */
    short p_exit;	   /* Exit status of a process which died */
    proctbl_t *next;       /* next entry in list */
};

#define	NULLPROC ((proctbl_t *)0)

/*
 * Flags for the "p_flags" word of a proc_table entry:
 *
 * ACCOUNTED            Accounting has been done for this process
 *
 * LIVING		Process is alive.
 *
 * NOCLEANUP		"efork()" is not allowed to cleanup this entry
 *                      even if process is dead.
 *
 * NAMED		This process has a name, i.e. came from
 *                      /etc/inittab.
 *
 * DEMANDREQUEST	Process started by a "telinit [abc]" command.
 *                      Processes formed this way are respawnable and
 *                      immune to level changes as long as their entry
 *                      exists in inittab.
 *
 * TOUCHED		Flag used by "remove" to determine whether it
 *                      has looked at an entry while checking for
 *			processes to be killed.
 *
 * WARNED		Flag used by "remove" to mark processes that
 *                      have been sent the SIGTERM signal.  If they
 *                      don't die in 20 seconds, they will be sent the
 *                      SIGKILL signal.
 *
 * KILLED		Flag used by "remove" to say that a process has
 *                      been sent both kill signals.  Such processes
 *			should die immediately, but in case they don't,
 *                      this prevents "init" from trying to kill it
 *			again and again, and hogging the process table
 *                      of the operating system.
 *
 * DEFERACCT		FLAG used to defer posting of acctg record
 *                      during BOOTWAIT operations.  This protects the
 *			file system until bcheckrc runs fsck.
 *                      Added by rer.
 *
 * NEWENTRY             A new entry, not currently in process table
 */
#define ACCOUNTED       00001
#define	LIVING		00002
#define	NOCLEANUP	00004
#define	NAMED		00010
#define	DEMANDREQUEST	00020
#define	TOUCHED		00040
#define	WARNED		00100
#define	KILLED		00200
#define	DEFERACCT	00400
#define	NEWENTRY 	01000

extern proctbl_t *proc_alloc();
extern void proc_free();
extern time_t proc_cleanup();
extern void proc_delete();
extern proctbl_t *findslot();

/*
 * Process table --
 *    This is a linked list of entries that are malloc-ed as needed.
 *    We don't free() the memory, just put it on the "free_procs" list.
 */
extern proctbl_t *proc_table;	/* Table of active processes */
extern proctbl_t *free_procs;   /* free list of entries */

#endif /* _PROCTABLE_INCLUDED */
