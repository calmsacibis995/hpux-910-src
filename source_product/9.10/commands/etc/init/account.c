/* @(#) $Revision: 66.6 $ */
/*
 * $Header: account.c,v 66.6 90/08/27 11:26:12 raf Exp $
 *
 * account.c -- accounting routines used by "init"
 */
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <utmp.h>
#include <fcntl.h>
#ifdef AUDIT
#   include <sys/audit.h>
#endif
#ifdef TRUX
#   include <sys/security.h>
#endif
#include "init.h"
#include "proctable.h"

#ifdef UDEBUG
#   undef  UTMP_FILE
#   define UTMP_FILE      "utmp"
#   undef  WTMP_FILE
#   define WTMP_FILE      "wtmp"
#endif

/*
 * account() --
 *    update entries in /etc/utmp and append new entries to the end
 *    of /etc/wtmp (assuming /etc/wtmp exists).
 */
account(state, process, program)
int state;
register proctbl_t *process;
char *program;	/* Name of program in the case of INIT_PROCESSes
		 * otherwise NULL.
		 */
{
    static int defering = FALSE; /* TRUE if accounting BOOTWAIT procs */
    register struct utmp *u, *oldu;
    char c;
    struct utmp utmpbuf;
#ifdef AUDIT
    struct self_audit_rec audrec;
    char *txtptr;
#endif AUDIT

#ifdef	ACCTDEBUG
    debug("** account ** state: %d id:%s\n",
	state, C(process->p_id));
#endif

    /*
     * Set up the prototype for the utmp structure we want to write.
     */
    u = &utmpbuf;
    memset(u, 0, sizeof (struct utmp));

    /*
     * Fill in the various fields of the utmp structure.
     */
    u->ut_id[0] = process->p_id[0];
    u->ut_id[1] = process->p_id[1];
    u->ut_id[2] = process->p_id[2];
    u->ut_id[3] = process->p_id[3];
    u->ut_pid = process->p_pid;

    /*
     * Fill the "ut_exit" structure.
     */
    u->ut_exit.e_termination = (process->p_exit & 0xff);
    u->ut_exit.e_exit = ((process->p_exit >> 8) & 0xff);
    u->ut_type = state;
    time(&u->ut_time);

    /*
     * See if there already is such an entry in the "utmp" file.
     * Only do this if we are NOT deferring accounting.  If we
     * are deferring accounting, we assume that the utmp file is
     * empty.
     */
    if (!(process->p_flags & DEFERACCT))
    {
	setutent();		/* Start at beginning of utmp file. */
	if ((oldu = getutid(u)) != NULL)
	{
	    /*
	     * Copy in the old "user" and "line" fields to our new
	     * structure.
	     */
	    memcpy(u->ut_user, oldu->ut_user, sizeof (u->ut_user));
	    memcpy(u->ut_line, oldu->ut_line, sizeof (u->ut_line));
#ifdef ACCTDEBUG
	    debug("Replacing old entry in utmp file.\n");
	}
	else
	{
	    debug("New entry in utmp file.\n");
#endif
	}
    }

    /*
     * Preform special accounting. Insert the special string into the
     * ut_line array. For INIT_PROCESSes put in the name of the
     * program in the "ut_user" field.
     */
    switch (state)
    {
    case RUN_LVL:
	u->ut_exit.e_termination = level(cur_state);
	u->ut_exit.e_exit = level(prior_state);
	u->ut_pid = n_prev[cur_state];
	c = level(cur_state);
	sprintf(u->ut_line, RUNLVL_MSG, c);
#ifdef AUDIT
	/* Construct an audit record */
	txtptr = (char *)audrec.aud_body.text;
	strcpy(txtptr, "Run level is changed to ");
	strncat(txtptr, &c, 1);
	audrec.aud_head.ah_pid = getpid();
	audrec.aud_head.ah_error = 0;
	audrec.aud_head.ah_event = EN_RUNSTATE;
	audrec.aud_head.ah_len = strlen(txtptr);
	/* Write the audit record */
	setaudproc(1);		/* so the record will get written */
	audwrite(&audrec);
	setaudproc(0);		/* so we don't get unwanted records */
#endif AUDIT
	break;
    case BOOT_TIME:
	sprintf(u->ut_line, "%.12s", BOOT_MSG);
	break;
    case INIT_PROCESS:
	strncpy(u->ut_user, program, sizeof (u->ut_user));
	break;
#ifdef AUDIT
    case DEAD_PROCESS:
	/* Construct an audit record */
	txtptr = (char *)audrec.aud_body.text;
	strcpy(txtptr, "Dead process: ");
	strcat(txtptr, ltoa(process->p_pid));
	audrec.aud_head.ah_pid = process->p_pid;
	audrec.aud_head.ah_error = process->p_exit;
	audrec.aud_head.ah_event = EN_LOGOUT;
	audrec.aud_head.ah_len = strlen(txtptr);
	/* Write the audit record */
	setaudproc(1);		/* so the record will get written */
	audwrite(&audrec);
	setaudproc(0);		/* clear flag again */
	break;
#endif AUDIT
#ifdef SecureWare
	/* NOTE: SecureWare and AUDIT are mutually exclusive */
	case DEAD_PROCESS:
		if (ISSECURE)
		    init_update_tty(oldu);
		break;
#endif
    default:
	break;
    }

    if (process->p_flags & DEFERACCT) /* defering acctg during boot */
    {
	/* these must all be up front in inittab to be effective */
	defering = TRUE;
	defacct(u);
    }
    else
    {
	if (defering)
	{
	    /* we've been defering posting, now post everything held */
	    defering = FALSE;
	    postacct();
	}

	/*
	 * Write out the updated entry to utmp file.
	 */
	if (_pututline(u) == (struct utmp *)NULL)
	    console("failed write of utmp entry: \"%-4.4s\"\n",
		u->ut_id);

	/*
	 * Now attempt to add to the end of the wtmp file.  Do not
	 * create if it doesn't already exist.
	 */
	{
	    int fd;

	    if ((fd = open(WTMP_FILE, O_WRONLY|O_APPEND)) != -1)
	    {
		write(fd, u, sizeof (struct utmp));
		close(fd);
	    }
	}
    }
}

/**********************************/
/****    Defered accounting    ****/
/**********************************/

/*
 * The following routines are added to provide for defered accounting
 * at boot
 * Added 10/17/85 by rer
 * Modified 04/26/89 by R. Scott Holbrook --
 *    Allocate 64 array elements at a time, instead of NPROC, since
 *    we no longer have an NPROC constant, and all size limits are
 *    being removed.
 */

static int defer_cnt = 0;     /* number of defered records held */
static int post_cnt = 0;      /* number of calls to postacct() */
static struct utmp **def_ut;  /* array of pointers to held entrys */
static int def_arrsize = 0;   /* size of array of pointers */
#define DEF_ARRINC 64         /* allocate in chunks of 64 elements */

/***********************/
/****    defacct    ****/
/***********************/

/*
 * "defacct" keeps track of utmp entries whose posting is defered
 * until after the "BOOTWAIT" entries have completed executing.
 * This allows keeping accurate records of boot time and level
 * transitions ala SVID without risking file system corruption
 * caused by posting accounting records before bcheckrc has a
 * chance to run fsck after a crash. "postacct" posts these later
 */
void
defacct(u)
struct utmp *u;	/* utmp entry to be held */
{
#ifdef ACCTDEBUG
    debug("In defacct()\n");
#endif
    /*
     * If this is the first time through the routine, we must malloc
     * some space for the accounting records
     */
    if (defer_cnt == 0)
    {
#ifdef ACCTDEBUG
	debug("defacct: malloc'ing **def_ut, %d\n",
	    DEF_ARRINC * sizeof (struct utmp *));
#endif
	/* allocate space for the pointer array */
	def_ut = (struct utmp **)malloc(DEF_ARRINC * sizeof (struct utmp *));
	def_arrsize = DEF_ARRINC;
    }
    else
	if (defer_cnt >= def_arrsize)
	{
	    def_arrsize += DEF_ARRINC;
	    def_ut = (struct utmp **)realloc(def_ut, def_arrsize * sizeof (struct utmp *));
	}

#ifdef ACCTDEBUG
    debug("defacct: malloc'ing def_ut[%d], %d\n",
	    defer_cnt, sizeof (struct utmp));
    debug("defacct: copying u to  def_ut[%d], %s\n",
	    defer_cnt, u->ut_id);
#endif
    def_ut[defer_cnt] = (struct utmp *)malloc(sizeof (struct utmp));
    memcpy(def_ut[defer_cnt], u, sizeof (struct utmp));

    /*
     * If this is the DEAD_PROCESS accounting of the above
     * then copy its ut_user and ut_line into mine
     */
    if (defer_cnt &&
	u->ut_type != RUN_LVL &&
	u->ut_type != BOOT_TIME &&
	u->ut_type != OLD_TIME &&
	u->ut_type != NEW_TIME &&
	id_eq(def_ut[defer_cnt]->ut_id, def_ut[defer_cnt - 1]->ut_id))
    {
#ifdef ACCTDEBUG
	debug("defacct: copying def_ut[%d]->ut_user to  def_ut[%d]->ut_user, %s\n",
		defer_cnt - 1, defer_cnt, def_ut[defer_cnt - 1]->ut_user);
#endif
	strncpy(def_ut[defer_cnt]->ut_user,
		def_ut[defer_cnt - 1]->ut_user,
		sizeof def_ut[0]->ut_user);
#ifdef ACCTDEBUG
	debug("defacct: copying def_ut[%d]->ut_line to  def_ut[%d]->ut_line, %s\n",
		defer_cnt - 1, defer_cnt, def_ut[defer_cnt - 1]->ut_line);
#endif
	strncpy(def_ut[defer_cnt]->ut_line,
		def_ut[defer_cnt - 1]->ut_line,
		sizeof def_ut[0]->ut_line);
    }
    defer_cnt++;
#ifdef ACCTDEBUG
    debug("Leave defacct()\n");
#endif
}

/************************/
/****    postacct    ****/
/************************/

/*
 * "postacct" posts entries to utmp and wtmp which were previously
 * defered.  After posting everything it frees the memory space
 * allocated by defacct.
 */
void
postacct()
{
    int index;

#ifdef ACCTDEBUG
    debug("In postacct()\n");
#endif
    if (post_cnt == 0)
    {		/* no posting has occured yet */
	char utmplock[sizeof UTMP_FILE + 5];
	int fd;

	/*
	 * Initialize (truncate) the "utmp" file
	 * Set the umask so that the utmp file is created 644.
	 */
#ifdef SecureWare
	if (ISSECURE)
	    init_secure_mask();
	else
	    umask(0);	/* Allow files to be created normally. */
#else
	umask(0);	/* Allow files to be created normally. */
#endif
	endutent();

	sprintf(utmplock, "%s.lck", UTMP_FILE);
	unlink(utmplock);

	if ((fd = open(UTMP_FILE, O_WRONLY|O_CREAT|O_TRUNC, 0644)) == -1)
	{
	    /*
	     * Without "utmp" file default to single user mode.
	     */
	    console("Cannot create %s\n", UTMP_FILE, 0);
	    cur_state = SINGLE_USER;
	}
	else
	    close(fd);
    }

#ifdef ACCTDEBUG
    debug("In postacct(), about to post records\n");
#endif
    /*
     * post all the defered utmp entries FIFO
     */
    for (index = 0; index < defer_cnt; index++)
    {
#ifdef ACCTDEBUG
	debug("postacct: _pututline(def_ut[%d]), %s\n",
	    index, def_ut[index]->ut_id);
#endif
	/* Write out the updated entry to utmp file. */
	if (_pututline(def_ut[index]) == (struct utmp *)NULL)
	    console("failed write of utmp entry: \"%-4.4s\"\n",
		def_ut[index]->ut_id);

	/*
	 * Now attempt to add to the end of the wtmp file.  Do not
	 * create if it doesn't already exist.
	 */
	{
	    int fd;

#ifdef ACCTDEBUG
	    debug("postacct: wtmp'ing def_ut[%d], %s\n", index, def_ut[index]->ut_id);
#endif
	    if ((fd = open(WTMP_FILE, O_WRONLY|O_APPEND)) != -1)
	    {
		write(fd, def_ut[index], sizeof (struct utmp));
		close(fd);
	    }
	}
#ifdef ACCTDEBUG
	debug("postacct: free'ing def_ut[%d]\n", index);
#endif
	free(def_ut[index]);
    }
#ifdef ACCTDEBUG
    debug("postacct: free'ing **def_ut\n");
#endif
    free(def_ut);
    defer_cnt = 0;
    post_cnt++;
#ifdef ACCTDEBUG
    debug("Leave postacct()\n");
#endif
}
