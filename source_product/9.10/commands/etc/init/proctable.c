/*
 * @(#) $Header: proctable.c,v 66.3 90/05/17 15:34:38 rsh Exp $
 *
 * proctbl.c -- routines used to manipulate the process table.
 */
#include <stdio.h>
#include <sys/types.h>
#include <utmp.h>
#include "init.h"
#include "proctable.h"

proctbl_t *proc_table = (proctbl_t *)NULL; /* active processes */
static proctbl_t *free_procs = (proctbl_t *)NULL; /* free list */

#ifdef MEMORY_CHECKS
static int num_entries = 0;
static unsigned long num_alloc_calls = 0;
static unsigned long num_free_calls = 0;
static proctbl_t *all_entries[1024];

/*
 * check_proc_alloc()
 *    Check the length of the process table and free list to make
 *    sure that they match with num_entries.
 */
void
check_proc_alloc()
{
    static time_t error_time = 0;
    register int list_entries = 0;
    register int free_entries = 0;
    register proctbl_t *proc;
    time_t now;

    for (proc = proc_table; proc; proc = proc->next)
	list_entries++;
    for (proc = free_procs; proc; proc = proc->next)
	free_entries++;

#ifdef DEBUG1
    if (list_entries+free_entries != num_entries)
	debug("check_proc_alloc(), Detected bad alloc/free\n");
#endif
    if (list_entries+free_entries != num_entries &&
	(now = time(0)) > error_time + 60)
    {
	FILE *fp = fopen("init.memory", "a");
	error_time = now;

	if (fp != NULL)
	{
	    int i;

	    fprintf(fp, "\nNumber of used proc_table records: %d\n",
		list_entries);
	    fprintf(fp, "Number of free proc_table records: %d\n",
		free_entries);
	    fprintf(fp, "Allocated proc_table records:	    %d\n",
		num_entries);
	    fprintf(fp, "Calls to proc_alloc(): %d\n",
		num_alloc_calls);
	    fprintf(fp, "Calls to proc_free():	%d\n",
		num_free_calls);

	    for (i = 0; i < num_entries; i++)
	    {
		register proctbl_t *match = all_entries[i];

		for (proc = proc_table; proc; proc = proc->next)
		    if (proc == match)
			break;
		if (proc)
		    continue;

		for (proc = free_procs; proc; proc = proc->next)
		    if (proc == match)
			break;

		if (proc)
		    continue;

		fputs("Missing entry:\n", fp);
		fprintf(fp, "	 p_id:	       %-4s\n", match->p_id);
		fprintf(fp, "	 p_pid:	 %10d\n",	match->p_pid);
		fprintf(fp, "	 p_count %10d\n",	match->p_count);
		fprintf(fp, "	 p_time	 %10d\n",	match->p_time);
		fprintf(fp, "	 p_flags     0x%04x\n", match->p_flags);
		fprintf(fp, "	 p_exit	 %10d\n",	match->p_exit);
		fprintf(fp, "	 p_next	     0x%04x\n", match->next);
		fputc('\n', fp);
	    }

	    fclose(fp);
	}
    }
}
#endif

/*
 * proc_alloc() --
 *     allocate a process table entry, either from the free list or
 *     by calling malloc().  If the malloc() fails, we re-use an entry
 *     from the process table for a process that is dead.
 *
 *     It is assumed that all regular process table cleanup has already
 *     been done.
 */
proctbl_t *
proc_alloc()
{
    register proctbl_t *new_p;
    register proctbl_t *prev;

#ifdef MEMORY_CHECKS
    num_alloc_calls++;
#endif
#ifdef DEBUG1
    debug("In proc_alloc()\n");
#   ifdef MEMORY_CHECKS
    check_proc_alloc();
#   endif
#endif
    if (free_procs != (proctbl_t *)NULL)
    {
	new_p = free_procs;
	free_procs = free_procs->next;
    }
    else
    {
	new_p = (proctbl_t *)malloc(sizeof(proctbl_t));
#ifdef MEMORY_CHECKS
	all_entries[num_entries++] = new_p;
#endif
    }

    if (new_p)
    {
	memset(new_p, 0, sizeof(proctbl_t));
	new_p->p_flags = NEWENTRY;
#ifdef DEBUG1
	debug("Leave proc_alloc()\n");
#endif
	return new_p;
    }

    fprintf(stderr, "init: ran out of memory!\n");
    exit(1);
}

/*
 * proc_free() --
 *     return a process table entry to our free list.
 */
void
proc_free(process)
proctbl_t *process;
{
#ifdef MEMORY_CHECKS
    num_free_calls++;
#endif
#ifdef DEBUG1
    debug("In proc_free(), id is %s, pid is %d\n",
	process->p_id, process->p_pid);
#endif
    process->next = free_procs;
    free_procs = process;

#if defined(MEMORY_CHECKS) && defined(DEBUG1)
    check_proc_alloc();
#endif
#ifdef DEBUG1
    debug("Leave proc_free()\n");
#endif
}

/*
 * proc_cleanup() --
 *     Freshen up the proc_table, removing any entries for dead
 *     processes that don't have the NOCLEANUP set.  Perform the
 *     necessary accounting.
 *
 *     Returns the minimum time interval before we need to send a
 *     SIGKILL to any warned processes (0 if no processes to KILL).
 */
time_t
proc_cleanup()
{
    register proctbl_t *proc;
    register proctbl_t *prev;
    register proctbl_t *next;
    time_t min_time = 0;
    /*
     * flag to insure that proc_cleanup is not called
     * more than once to fix proc_cleanup->account->
     * init_update_tty->efork->proc_cleanup infinit loop
     */
    static int proclnup = 0;

#ifdef DEBUG1
    debug("In proc_cleanup()\n");
#endif

    if (proclnup)
    {
#ifdef DEBUG1
	debug("return from proc_cleanup() sw_prclup:\n");
#endif
	return 0;
    }

    proclnup = 1;
    for (proc = proc_table; proc; proc = next)
    {
	next = proc->next;
	if ((proc->p_flags & LIVING) == 0)
	{
#ifdef	DEBUG
	    debug("proc_cleanup- id:%s pid: %d time: %lo %d %o %o\n",
		    C(&proc->p_id[0]), proc->p_pid,
		    proc->p_time, proc->p_count,
		    proc->p_flags, proc->p_exit);
#endif

	    /*
	     * Is this a named process?	 If so, do the necessary
	     * bookkeeping.
	     */
	    if ((proc->p_flags & (NAMED|ACCOUNTED)) == NAMED)
	    {
		account(DEAD_PROCESS, proc, (proctbl_t *)NULL);
		proc->p_flags |= ACCOUNTED;
	    }

	    /*
	     * Remove this entry from the process table and free this
	     * entry for new usage.  We only do this if this process
	     * isn't marked as NOCLEANUP, since we need the process
	     * table entry to stick around (for respawn frequency
	     * tracking).
	     */
	    if ((proc->p_flags & NOCLEANUP) == 0)
	    {
#ifdef DEBUG
		debug("proc_cleanup, freeing id:%s pid: %d time: %lo %d %o %o\n",
			C(&proc->p_id[0]), proc->p_pid,
			proc->p_time, proc->p_count,
			proc->p_flags, proc->p_exit);
#endif
		if (proc == proc_table)
		    proc_table = proc->next;
		else
		    prev->next = proc->next;
		proc_free(proc);
	    }

	    continue;
	}

#ifdef DEBUG
	debug("proc_cleanup, ignore id:%s pid: %d time: %lo %d %o %o\n",
		C(&proc->p_id[0]), proc->p_pid,
		proc->p_time, proc->p_count,
		proc->p_flags, proc->p_exit);
#endif
	if ((proc->p_flags & (LIVING|WARNED|KILLED)) == (LIVING|WARNED))
	    if (min_time == 0 || proc->p_time < min_time)
		min_time = proc->p_time;
	prev = proc;
    }
    proclnup = 0;

#ifdef DEBUG1
    debug("Leave proc_cleanup(), returning %ld\n",
	min_time != 0 ? min_time - time((time_t *)NULL) : 0);
#endif
    return min_time != 0 ? min_time - time((time_t *)NULL) : 0;
}

/*
 * proc_delete() --
 *     Given a process table entry, delete it from the process table
 */
void
proc_delete(process)
proctbl_t *process;
{
    register proctbl_t *prev = (proctbl_t *)NULL;
    register proctbl_t *proc;

    for (proc = proc_table; proc && proc != process;
					 prev = proc, proc = proc->next)
	continue;

    if (proc)
    {
	if (proc == proc_table)
	    proc_table = proc->next;
	else
	    prev->next = proc->next;
	proc_free(proc);
    }
}

/*
 * findslot() --
 *     finds the old slot in the process table for the command with the
 *     same id, or it finds an empty slot.
 */
proctbl_t *
findslot(cmd)
register struct CMD_LINE *cmd;
{
    register proctbl_t *process;

    for (process = proc_table; process; process = process->next)
	if (id_eq(process->p_id, cmd->c_id))
	    return process;

    return proc_alloc();
}
