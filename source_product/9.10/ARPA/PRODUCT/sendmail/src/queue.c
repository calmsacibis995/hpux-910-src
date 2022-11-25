/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

# ifndef lint
static char rcsid[] = "$Header: queue.c,v 1.21.109.11 95/03/22 17:38:15 mike Exp $";
# 	ifndef hpux
# 		ifdef QUEUE
static char sccsid[] = "@(#)queue.c	5.30 (Berkeley) 6/1/90 (with queueing)";
# 		else	/* ! QUEUE */
static char sccsid[] = "@(#)queue.c	5.30 (Berkeley) 6/1/90 (without queueing)";
# 		endif	/* QUEUE */
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_5384="@(#) PATCH_9.X: queue.o $Revision: 1.21.109.11 $ 94/03/24 PHNE_5384";
# endif	/* PATCH_STRING */

# include "sendmail.h"
# include <sys/stat.h>
# include <dirent.h>
# include <sys/file.h>
# include <sys/unistd.h>
# include <signal.h>
# include <errno.h>
# include <pwd.h>

# ifdef QUEUE

static void setctluser();
static void clrctluser();

/*
**  Work queue.
*/

struct work
{
	char		*w_name;	/* name of control file */
	long		w_pri;		/* priority of message, see below */
	time_t		w_ctime;	/* creation time of message */
	struct work	*w_next;	/* next in queue */
};

typedef struct work	WORK;

WORK	*WorkQ;			/* queue of things to be done */
/*
**  QUEUEUP -- queue a message up for future transmission.
**
**	Parameters:
**		e -- the envelope to queue up.
**		queueall -- if TRUE, queue all addresses, rather than
**			just those with the QQUEUEUP flag set.
**		announce -- if TRUE, tell when you are queueing up.
**		dorename -- if TRUE, rename the temp file to the queue file.
**
**	Returns:
**		locked FILE* to q file
**
**	Side Effects:
**		The current request are saved in a control file.
*/

FILE *
queueup(e, queueall, announce, dorename)
	register ENVELOPE *e;
	bool queueall;
	bool announce;
	bool dorename;
{
	register FILE *tfp;
	register HDR *h;
	register ADDRESS *q;
	register char *p;
	char *qf;
	int fd;
	bool newid;
	MAILER nullmailer;
	char buf[MAXLINE], tf[MAXLINE];

	/*
	**  Create control file.
	*/

	newid = (e->e_id == NULL) || !bitset(EF_INQUEUE, e->e_flags);

	/* if newid, queuename will create a locked qf file in e->lockfp */
	strcpy(tf, queuename(e, 't'));
	tfp = e->e_lockfp;
	if (tfp == NULL)
		newid = FALSE;

	/* if newid, just write the qf file directly (instead of tf file) */
	if (!newid)
	{
		/* get a locked tf file */
		do {
			fd = open(tf, O_CREAT|O_WRONLY|O_EXCL, FileMode);
			if (fd < 0) {
				syserr("queueup: Cannot create temp file %s", tf);
				return NULL;
			} else {
				if (lockf(fd, F_TLOCK, 0) < 0) {
					if (errno != EACCES && errno != EAGAIN)
						syserr("Cannot lockf(%s)", tf);
					close(fd);
					fd = -1;
				}
			}
		} while (fd < 0);

		tfp = fdopen(fd, "w");
	}

# 	ifdef DEBUG
	if (tTd(40, 1))
		printf("\n>>>>> queueing %s%s >>>>>\n", e->e_id,
			newid ? " (new id)" : "");
# 	endif /* DEBUG */

	/*
	**  If there is no data file yet, create one.
	*/

	if (e->e_df == NULL)
	{
		register FILE *dfp;

		e->e_df = queuename(e, 'd');
		e->e_df = newstr(e->e_df);
		fd = open(e->e_df, O_WRONLY|O_CREAT, FileMode);
		if (fd < 0 || (dfp = fdopen(fd, "w")) == NULL)
		{
			syserr("queueup: cannot create data temp file %s, uid=%d",
			       e->e_df, geteuid());
			(void) fclose(tfp);
			return NULL;
		}
		(*e->e_putbody)(dfp, ProgMailer, e);
		(void) fclose(dfp);
		e->e_putbody = putbody;
	}

	/*
	**  Output future work requests.
	**	Priority and creation time should be first, since
	**	they are required by orderq.
	*/

	/* output message priority */
	fprintf(tfp, "P%ld\n", e->e_msgpriority);

	/* output creation time */
	fprintf(tfp, "T%ld\n", e->e_ctime);

	/* output SMTP size of content (body only) */
	if (e->e_msgsize)
	    fprintf(tfp, "Z%ld\n", e->e_msgsize);

	/* output body type and name of data file */
	if (e->e_bodytype != NULL)
		fprintf(tfp, "B%s\n", e->e_bodytype);
	fprintf(tfp, "D%s\n", e->e_df);

	/* message from envelope, if it exists */
	if (e->e_message != NULL)
		fprintf(tfp, "M%s\n", denlstring(e->e_message, TRUE, FALSE));

	/* send various flag bits through */
	p = buf;
	if (bitset(EF_RESPONSE, e->e_flags))
		*p++ = 'r';
	*p++ = '\0';
	if (buf[0] != '\0')
		fprintf(tfp, "F%s\n", buf);

	/* output name of sender */
	fprintf(tfp, "S%s\n", denlstring(e->e_from.q_paddr, TRUE, FALSE));

	/* output list of recipient addresses */
	for (q = e->e_sendqueue; q != NULL; q = q->q_next)
	{
		if (bitset(QQUEUEUP, q->q_flags) ||
		    (queueall && !bitset(QDONTSEND|QBADADDR|QSENT, q->q_flags)))
		{
			char *ctluser, *getctluser();

			if ((ctluser = getctluser(q)) != NULL)
				fprintf(tfp, "C%s\n", ctluser);
			fprintf(tfp, "R%s\n", denlstring(q->q_paddr, TRUE, FALSE));
			if (announce)
			{
				e->e_to = q->q_paddr;
				message("queued");
				if (LogLevel > 4)
					logdelivery("queued", e);
				e->e_to = NULL;
			}
# 	ifdef DEBUG
			if (tTd(40, 1))
			{
				printf("queueing ");
				printaddr(q, FALSE);
			}
# 	endif /* DEBUG */
		}
	}

	/* output list of error recipients */
	for (q = e->e_errorqueue; q != NULL; q = q->q_next)
	{
		if (!bitset(QDONTSEND, q->q_flags))
		{
			char *ctluser, *getctluser();

			if ((ctluser = getctluser(q)) != NULL)
				fprintf(tfp, "C%s\n", ctluser);
			fprintf(tfp, "E%s\n", denlstring(q->q_paddr, TRUE, FALSE));
		}
	}

	/*
	**  Output headers for this message.
	**	Expand macros completely here.  Queue run will deal with
	**	everything as absolute headers.
	**		All headers that must be relative to the recipient
	**		can be cracked later.
	**	We set up a "null mailer" -- i.e., a mailer that will have
	**	no effect on the addresses as they are output.
	*/

	bzero((char *) &nullmailer, sizeof nullmailer);
	nullmailer.m_r_rwset = nullmailer.m_s_rwset = -1;
	nullmailer.m_eol = "\n";

	define('g', "\001f", e);
	for (h = e->e_header; h != NULL; h = h->h_link)
	{
		extern bool bitzerop();

		/* don't output null headers */
		if (h->h_value == NULL || h->h_value[0] == '\0')
			continue;

		/* don't output resent headers on non-resent messages */
		if (bitset(H_RESENT, h->h_flags) && !bitset(EF_RESENT, e->e_flags))
			continue;

		/* output this header */
		fprintf(tfp, "H");

		/* if conditional, output the set of conditions */
		if (!bitzerop(h->h_mflags) && bitset(H_CHECK|H_ACHECK, h->h_flags))
		{
			int j;

			(void) putc('?', tfp);
			for (j = '\0'; j <= '\177'; j++)
				if (bitnset(j, h->h_mflags))
					(void) putc(j, tfp);
			(void) putc('?', tfp);
		}

		/* output the header: expand macros, convert addresses */
		if (bitset(H_DEFAULT, h->h_flags))
		{
			expand(h->h_value, buf, &buf[sizeof buf], e);
			fprintf(tfp, "%s: %s\n", h->h_field, buf);
		}
		else if (bitset(H_FROM|H_RCPT, h->h_flags))
		{
			bool oldstyle = bitset(EF_OLDSTYLE, e->e_flags);

			if (bitset(H_FROM, h->h_flags))
				oldstyle = FALSE;

			commaize(h, h->h_value, tfp, oldstyle,
				 &nullmailer, e);
		}
		else
			fprintf(tfp, "%s: %s\n", h->h_field, h->h_value);
	}

	/*
	**  Clean up.
	*/

	fflush(tfp);

	if (!newid)
	{
		if (dorename) {
			qf = queuename(e, 'q');
			if (rename(tf, qf) < 0)
				syserr("Cannot rename(%s, %s), df=%s, uid=%d",
				       tf, qf, e->e_df, geteuid());

			/* close and unlock old (locked) qf */
			if (e->e_lockfp != NULL)
				(void) fclose(e->e_lockfp);
			e->e_lockfp = tfp;
		}
	}
	else
		qf = tf;
	errno = 0;
	e->e_flags |= EF_INQUEUE;

# 	ifdef LOG
	/* save log info */
	if (LogLevel > 15)
		syslog(LOG_DEBUG, "%s: queueup, qf=%s, df=%s\n", e->e_id, qf, e->e_df);
# 	endif /* LOG */
	
# 	ifdef DEBUG
	if (tTd(40, 1))
		printf("<<<<< done queueing %s <<<<<\n\n", e->e_id);
# 	endif /* DEBUG */
	return tfp;
}
/*
**  RUNQUEUE -- run the jobs in the queue.
**
**	Gets the stuff out of the queue in some presumably logical
**	order and processes them.
**
**	Parameters:
**		forkflag -- TRUE if the queue scanning should be done in
**			a child process.  We ignore SIGCLD so we don't
**			have to clean up after it.
**
**	Returns:
**		none.
**
**	Side Effects:
**		runs things in the mail queue.
*/

ENVELOPE	QueueEnvelope;		/* the queue run envelope */

runqueue(forkflag)
	bool forkflag;
{
	register ENVELOPE *e;
	extern ENVELOPE BlankEnvelope;

	/*
	**  If no work will ever be selected, don't even bother reading
	**  the queue.
	*/

	CurrentLA = getla();	/* get load average */

	if (shouldqueue(-100000000L))
	{
		if (Verbose)
			printf("Skipping queue run -- load average too high\n");
# 	ifdef LOG
		if (LogLevel > 11)
			syslog(LOG_DEBUG, "load average = %.2f, skipping queue run",
			       (double)CurrentLA);
# 	endif /* LOG */
		if (forkflag && QueueIntvl != 0)
			(void) setevent(QueueIntvl, runqueue, TRUE);
		if (forkflag)
			return;
		finis();
	}

	/*
	**  See if we want to go off and do other useful work.
	*/

	if (forkflag)
	{
		int pid;

		pid = dofork();
		if (pid != 0)
		{
			/* ignoring SIGCLD obviates zombies */
			signal(SIGCLD, SIG_IGN);

			if (QueueIntvl != 0)
				(void) setevent(QueueIntvl, runqueue, TRUE);
			return;
		}
		signal(SIGCLD, SIG_DFL);
	}

	setproctitle("running queue: %s", QueueDir);

# 	ifdef LOG
	if (LogLevel > 11)
		syslog(LOG_DEBUG, "runqueue %s, pid=%d, forkflag=%d",
			QueueDir, getpid(), forkflag);
# 	endif /* LOG */

	/*
	**  Release any resources used by the daemon code.
	*/

# 	ifdef DAEMON
	clrdaemon();
# 	endif /* DAEMON */

	/*
	**  Create ourselves an envelope
	*/

	CurEnv = &QueueEnvelope;
	e = newenvelope(&QueueEnvelope, CurEnv);
	e->e_flags = BlankEnvelope.e_flags;

	/*
	**  Make sure the alias databases are open.
	*/

	if (AliasDB == NULL)
		AliasDB = initaliases(AliasFile, FALSE, e);
	if (ReverseAliasFile && (ReverseAliasDB == NULL))
		ReverseAliasDB = initaliases(ReverseAliasFile, FALSE, e);

	/*
	**  Start making passes through the queue.
	**	First, read and sort the entire queue.
	**	Then, process the work in that order.
	**		But if you take too long, start over.
	*/

	/* order the existing work requests */
	(void) orderq(FALSE);

	/* process them once at a time */
	while (WorkQ != NULL)
	{
		WORK *w = WorkQ;

		WorkQ = WorkQ->w_next;

		/*
		**  Ignore jobs that are too expensive for the moment.
		*/

		if (shouldqueue(w->w_pri))
		{
			if (Verbose)
				printf("\nSkipping %s\n", w->w_name + 2);
		}
		else
		{
			pid_t pid;
			extern pid_t dowork();

			pid = dowork(w->w_name + 2, ForkQueueRuns, FALSE, e);
			errno = 0;
			(void) waitfor(pid);
		}
		free(w->w_name);
		free((char *) w);
	}

	/* exit without the usual cleanup */
	exit(ExitStat);
}
/*
**  ORDERQ -- order the work queue.
**
**	Parameters:
**		doall -- if set, include everything in the queue (even
**			the jobs that cannot be run because the load
**			average is too high).  Otherwise, exclude those
**			jobs.
**
**	Returns:
**		The number of request in the queue (not necessarily
**		the number of requests in WorkQ however).
**
**	Side Effects:
**		Sets WorkQ to the queue of available work, in order.
*/

# 	define NEED_P		001
# 	define NEED_T		002

orderq(doall)
	bool doall;
{
	register struct dirent *d;
	register WORK *w;
	register int i;
	DIR *f;
	WORK wlist[QUEUESIZE+1];
	int wn = -1;
	extern workcmpf();

	/* clear out old WorkQ */
	for (w = WorkQ; w != NULL; )
	{
		register WORK *nw = w->w_next;

		WorkQ = nw;
		free(w->w_name);
		free((char *) w);
		w = nw;
	}

	/* open the queue directory */
	f = opendir(".");
	if (f == NULL)
	{
		syserr("orderq: cannot open \"%s\" as \".\"", QueueDir);
		return (0);
	}

	/*
	**  Read the work directory.
	*/

	while ((d = readdir(f)) != NULL)
	{
		FILE *cf;
		char lbuf[MAXNAME];

		/* is this an interesting entry? */
		if (d->d_name[0] != 'q' || d->d_name[1] != 'f')
			continue;

		/* yes -- open control file (if not too many files) */
		if (++wn >= QUEUESIZE)
			continue;

		cf = fopen(d->d_name, "r");
		if (cf == NULL)
		{
			/* this may be some random person sending hir msgs */
			/* syserr("orderq: cannot open %s", cbuf); */
# 	ifdef DEBUG
			if (tTd(41, 2))
				printf("orderq: cannot open %s (%d)\n",
					d->d_name, errno);
# 	endif /* DEBUG */
			errno = 0;
			wn--;
			continue;
		}
		w = &wlist[wn];
		w->w_name = newstr(d->d_name);

		/* make sure jobs in creation don't clog queue */
		w->w_pri = 0x7fffffff;
		w->w_ctime = 0;

		/* extract useful information */
		i = NEED_P | NEED_T;
		while (i != 0 && fgets(lbuf, sizeof lbuf, cf) != NULL)
		{
			extern long atol();

			switch (lbuf[0])
			{
			  case 'P':
				w->w_pri = atol(&lbuf[1]);
				i &= ~NEED_P;
				break;

			  case 'T':
				w->w_ctime = atol(&lbuf[1]);
				i &= ~NEED_T;
				break;
			}
		}
		(void) fclose(cf);

# 	ifdef NOTDEF
		/*
		**  Why do this here and just drop
		**  it on the floor?  It's done again
		**  more gracefully in dowork().
		*/
		if (!doall && shouldqueue(w->w_pri))
		{
			/* don't even bother sorting this job in */
			wn--;
		}
# 	endif /* NOTDEF */
	}
	(void) closedir(f);
	wn++;

	/*
	**  Sort the work directory.
	*/

	qsort((char *) wlist, min(wn, QUEUESIZE), sizeof *wlist, workcmpf);

	/*
	**  Convert the work list into canonical form.
	**	Should be turning it into a list of envelopes here perhaps.
	*/

	WorkQ = NULL;
	for (i = min(wn, QUEUESIZE); --i >= 0; )
	{
		w = (WORK *) xalloc(sizeof *w);
		w->w_name = wlist[i].w_name;
		w->w_pri = wlist[i].w_pri;
		w->w_ctime = wlist[i].w_ctime;
		w->w_next = WorkQ;
		WorkQ = w;
	}

# 	ifdef DEBUG
	if (tTd(40, 1))
	{
		for (w = WorkQ; w != NULL; w = w->w_next)
			printf("%32s: pri=%ld\n", w->w_name, w->w_pri);
	}
# 	endif /* DEBUG */

	return (wn);
}
/*
**  WORKCMPF -- compare function for ordering work.
**
**	Parameters:
**		a -- the first argument.
**		b -- the second argument.
**
**	Returns:
**		-1 if a < b
**		 0 if a == b
**		+1 if a > b
**
**	Side Effects:
**		none.
*/

workcmpf(a, b)
	register WORK *a;
	register WORK *b;
{
	long pa = a->w_pri + a->w_ctime;
	long pb = b->w_pri + b->w_ctime;

	if (pa == pb)
		return (0);
	else if (pa > pb)
		return (1);
	else
		return (-1);
}
/*
**  DOWORK -- do a work request.
**
**	Parameters:
**		id -- the ID of the job to run.
**		forkflag -- if set, run this in background.
**		requeueflag -- if set, reinstantiate the queue quickly.
**			This is used when expanding aliases in the queue.
**			If forkflag is also set, it doesn't wait for the
**			child.
**		e - the envelope in which to run it.
**
**	Returns:
**		process id of process that is running the queue job.
**
**	Side Effects:
**		The work request is satisfied if possible.
*/

pid_t
dowork(id, forkflag, requeueflag, e)
	char *id;
	bool forkflag;
	bool requeueflag;
	register ENVELOPE *e;
{
	register pid_t pid;

# 	ifdef DEBUG
	if (tTd(40, 1))
		printf("dowork(%s)\n", id);
# 	endif /* DEBUG */

	/*
	**  Fork for work.
	*/

	if (forkflag)
	{
		pid = dofork();
		if (pid < 0)
		{
			syserr("dowork: cannot fork");
			return 0;
		}
	}
	else
	{
		pid = 0;
	}

	if (pid == 0)
	{
		FILE *qflock, *readqf();
		/*
		**  CHILD
		**	Lock the control file to avoid duplicate deliveries.
		**		Then run the file as though we had just read it.
		**	We save an idea of the temporary name so we
		**		can recover on interrupt.
		*/

		/* set basic modes, etc. */
		(void) alarm(0);
		clearenvelope(e, FALSE);
		QueueRun = TRUE;
		e->e_errormode = EM_MAIL;
		e->e_id = id;
		if (forkflag)
		{
			disconnect(TRUE, e);
			OpMode = MD_DELIVER;
		}
# 	ifdef LOG
		if (LogLevel > 11)
			syslog(LOG_DEBUG, "%s: dowork, pid=%d", e->e_id,
			       getpid());
# 	endif /* LOG */

		/* don't use the headers from sendmail.cf... */
		e->e_header = NULL;

		/* read the queue control file */
		/*  and lock the control file during processing */
		if ((qflock=readqf(e, TRUE)) == NULL)
		{
			if (forkflag)
				exit(EX_OK);
			else
				return;
		}

		e->e_flags |= EF_INQUEUE;
		eatheader(e, requeueflag);

		if (requeueflag)
			queueup(e, TRUE, FALSE, TRUE);

		/* do the delivery */
		if (!bitset(EF_FATALERRS, e->e_flags))
			sendall(e, SM_DELIVER);

		/* finish up and exit */
		if (forkflag)
			finis();
		else
			dropenvelope(e);
		fclose(qflock);
	}
	e->e_id = NULL;
	return pid;
}
/*
**  READQF -- read queue file and set up environment.
**
**	Parameters:
**		e -- the envelope of the job to run.
**		full -- if set, read in all information.  Otherwise just
**			read in info needed for a queue print.
**
**	Returns:
**		FILE * pointing to lockf()ed fd so it can be closed
**		after the mail is delivered
**
**	Side Effects:
**		cf is read and created as the current job, as though
**		we had been invoked by argument.
*/

FILE *
readqf(e, full)
	register ENVELOPE *e;
	bool full;
{
	register FILE *qfp;
	char qf[MAXNAME], Qf[MAXNAME];
	struct stat st;
	char buf[MAXFIELD];
	extern char *fgetfolded();
	extern long atol();
	int gotctluser = 0;

	/*
	**  Read and process the file.
	*/

	strcpy(qf, queuename(e, 'q'));
	qfp = fopen(qf, "r+");
	if (qfp == NULL)
	{
		if (tTd(40, 8))
			printf("readqf(%s): fopen failure (%s)\n",
				qf, errstring(errno));
		if (errno != ENOENT)
			syserr("readqf: no control file %s", qf);
		return NULL;
	}

	if (lockf(fileno(qfp), F_TLOCK, 0) < 0)
	{
		/* being processed by another queuer */
		if (tTd(40, 8))
			printf("readqf(%s): locked\n", qf);
		if (Verbose)
			printf("%s: locked\n", e->e_id);
# 	ifdef LOG
		if (LogLevel > 11)
			syslog(LOG_DEBUG, "unable to lock %s\n", qf);
# 	endif /* LOG */
		(void) fclose(qfp);
		return NULL;
	}

	/*
	**  Check the queue file for plausibility to avoid attacks.
	*/

	if (fstat(fileno(qfp), &st) < 0)
	{
		/* must have been being processed by someone else */
		if (tTd(40, 8))
			printf("readqf(%s): fstat failure (%s)\n",
				qf, errstring(errno));
		fclose(qfp);
		return NULL;
	}

	if (st.st_uid != geteuid())
	{
# 	ifdef LOG
		if (LogLevel > 0)
		{
			syslog(LOG_ALERT, "%s: bogus queue file, uid=%d, mode=%o",
				e->e_id, st.st_uid, st.st_mode);
		}
# 	endif /* LOG */
		if (tTd(40, 8))
			printf("readqf(%s): bogus file\n", qf);
		strcpy(Qf, queuename(e, 'Q'));
		if (rename(qf, Qf) < 0)
			syserr("Cannot rename (%s, %s), uid=%d", qf, Qf, geteuid());
		fclose(qfp);
		return NULL;
	}

	if (st.st_size == 0)
	{
		/* must be a bogus file -- just remove it */
		(void) unlink(qf);
		fclose(qfp);
		return NULL;
	}

	if (st.st_nlink == 0)
	{
		/*
		**  Race condition -- we got a file just as it was being
		**  unlinked.  Just assume it is zero length.
		*/

		fclose(qfp);
		return NULL;
	}

	/* good file -- save this lock */
	e->e_lockfp = qfp;

	/* do basic system initialization */
	initsys(e);

	FileName = qf;
	LineNumber = 0;
	if (Verbose && full)
		printf("\nRunning %s\n", e->e_id);
	while (fgetfolded(buf, sizeof buf, qfp) != NULL)
	{
		register char *p;
		ADDRESS *q;
# 	ifdef DEBUG
		if (tTd(40, 4))
			printf("+++++ %s\n", buf);
# 	endif /* DEBUG */
		switch (buf[0])
		{
		  case 'C':		/* specify controlling user */
			setctluser(&buf[1]);
			gotctluser = 1;
			break;

		  case 'R':		/* specify recipient */
			q = parseaddr(&buf[1], (ADDRESS *) NULL, 1, '\0', e);
			if (q != NULL)
				{
				q->q_flags |= QPRIMARY;
				(void) recipient(q, &e->e_sendqueue, e);
				}
			break;

		  case 'E':		/* specify error recipient */
			sendtolist(&buf[1], (ADDRESS *) NULL, &e->e_errorqueue, e);
			break;

		  case 'H':		/* header */
			if (full)
				(void) chompheader(&buf[1], FALSE, e);
			break;

		  case 'M':		/* message */
			e->e_message = newstr(&buf[1]);
			break;

		  case 'S':		/* sender */
			setsender(newstr(&buf[1]), e);
			break;

		  case 'B':		/* body type */
			e->e_bodytype = newstr(&buf[1]);
			break;

		  case 'D':		/* data file name */
			if (!full)
				break;
			e->e_df = newstr(&buf[1]);
			e->e_dfp = fopen(e->e_df, "r");
			if (e->e_dfp == NULL)
				syserr("readqf: Cannot open %s", e->e_df);
			break;

		  case 'T':		/* init time */
			e->e_ctime = atol(&buf[1]);
			break;

		  case 'P':		/* message priority */
			e->e_msgpriority = atol(&buf[1]) + WkTimeFact;
			break;

		  case 'F':		/* flag bits */
			for (p = &buf[1]; *p != '\0'; p++)
			{
				switch (*p)
				{
				  case 'r':	/* response */
					e->e_flags |= EF_RESPONSE;
					break;
				}
			}
			break;

		  case 'Z':
			e->e_msgsize = atol(&buf[1]);
			break;

		  case '\0':		/* blank line; ignore */
			break;

		  default:
			syserr("readqf(%s:%d): bad line \"%s\"", e->e_id,
				LineNumber, buf);
			break;
		}
		/*
		**  The `C' queue file command operates on the next line,
		**  so we use "gotctluser" to maintain state as follows:
		**      0 - no controlling user,
		**      1 - controlling user has been set but not used,
		**      2 - controlling user must be used on next iteration.
		*/
		if (gotctluser == 1)
			gotctluser++;
		else if (gotctluser == 2)
		{
			clrctluser();
			gotctluser = 0;
		}
	}

	/* clear controlling user in case we break out prematurely */
	clrctluser();

	FileName = NULL;

	/*
	**  If we haven't read any lines, this queue file is empty.
	**  Arrange to remove it without referencing any null pointers.
	*/

	if (LineNumber == 0)
	{
		errno = 0;
		e->e_flags |= EF_CLRQUEUE | EF_FATALERRS | EF_RESPONSE;
	}
	return qfp;
}
/*
**  PRINTQUEUE -- print out a representation of the mail queue
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Prints a listing of the mail queue on the standard output.
*/

void printqueue()
{
	register WORK *w;
	FILE *f;
	int nrequests;
	char buf[MAXLINE];
	char cbuf[MAXLINE];

	/*
	**  Check for permission to print the queue
	*/

	if (bitset(PRIV_RESTRICTMAILQ, PrivacyFlags) && RealUid != 0)
	{
		struct stat st;
# 	ifdef NGROUPS
		int n;
		GIDSET_T gidset[NGROUPS];
# 	endif	/* NGROUPS */

		if (stat(QueueDir, &st) < 0)
		{
			syserr("Cannot stat %s", QueueDir);
			return;
		}
# 	ifdef NGROUPS
		n = getgroups(NGROUPS, gidset);
		while (--n >= 0)
		{
			if (gidset[n] == st.st_gid)
				break;
		}
		if (n < 0)
# 	else	/* ! NGROUPS */
		if (RealGid != st.st_gid)
# 	endif	/* NGROUPS */
		{
			usrerr("510 You are not permitted to see the queue");
			setstat(EX_NOPERM);
			return;
		}
	}

	/*
	**  Read and order the queue.
	*/

	nrequests = orderq(TRUE);

	/*
	**  Print the work list that we have read.
	*/

	/* first see if there is anything */
	if (nrequests <= 0)
	{
		printf("Mail queue is empty\n");
		return;
	}

	CurrentLA = getla();	/* get load average */

	printf("\t\tMail Queue (%d request%s", nrequests, nrequests == 1 ? "" : "s");
	if (nrequests > QUEUESIZE)
		printf(", only %d printed", QUEUESIZE);
	if (Verbose)
		printf(")\n----QID----- --Size-- -Priority- ---Q-Time--- ---------Sender/Recipient--------\n");
	else
		printf(")\n----QID----- --Size-- -----Q-Time----- ------------Sender/Recipient------------\n");
	for (w = WorkQ; w != NULL; w = w->w_next)
	{
		struct stat st;
		auto time_t submittime = 0;
		long dfsize = -1;
		char message[MAXLINE];
		char bodytype[MAXNAME];

		f = fopen(w->w_name, "r+");
		if (f == NULL)
		{
			errno = 0;
			continue;
		}
		printf("%11s", w->w_name + 2);
		if (lockf(fileno(f), F_TEST, 0) < 0)
			printf("*");
		else if (shouldqueue(w->w_pri))
			printf("X");
		else
			printf(" ");
		errno = 0;

		message[0] = bodytype[0] = '\0';
		cbuf[0] = '\0';
		while (fgets(buf, sizeof buf, f) != NULL)
		{
			fixcrlf(buf, TRUE);
			switch (buf[0])
			{
			  case 'M':	/* error message */
				(void) strcpy(message, &buf[1]);
				break;

			  case 'B':	/* body type */
				(void) strcpy(bodytype, &buf[1]);
				break;

			  case 'S':	/* sender name */
				if (Verbose)
					printf(" %8ld %10ld %.12s %.38s",
					    dfsize,
					    w->w_pri,
					    ctime(&submittime) + 4,
					    &buf[1]);
				else
					printf(" %8ld %.16s %.45s", dfsize,
					    ctime(&submittime), &buf[1]);
				if (message[0] != '\0' || bodytype[0] != '\0')
				{
					printf("\n    %10.10s", bodytype);
					if (message[0] != '\0')
						printf("   (%.60s)", message);
				}
				break;

			  case 'C':	/* controlling user */
				if (strlen(buf) < MAXLINE-3)	/* sanity */
					(void) strcat(buf, ") ");
				cbuf[0] = cbuf[1] = '(';
				(void) strncpy(&cbuf[2], &buf[1], MAXLINE-1);
				cbuf[MAXLINE-1] = '\0';
				break;

			  case 'R':	/* recipient name */
				if (cbuf[0] != '\0') {
					/* prepend controlling user to `buf' */
					(void) strncat(cbuf, &buf[1],
					              MAXLINE-strlen(cbuf));
					cbuf[MAXLINE-1] = '\0';
					(void) strcpy(buf, cbuf);
					cbuf[0] = '\0';
				}
				if (Verbose)
					printf("\n\t\t\t\t\t      %.38s", &buf[1]);
				else
					printf("\n\t\t\t\t       %.45s", &buf[1]);
				break;

			  case 'T':	/* creation time */
				submittime = atol(&buf[1]);
				break;

			  case 'D':	/* data file name */
				if (stat(&buf[1], &st) >= 0)
					dfsize = st.st_size;
				break;
			}
		}
		if (submittime == (time_t) 0)
			printf(" (no control file)");
		printf("\n");
		(void) fclose(f);
	}
}

# endif /* QUEUE */
/*
**  QUEUENAME -- build a file name in the queue directory for this envelope.
**
**	Assigns an id code if one does not already exist.
**	This code is very careful to avoid trashing existing files
**	under any circumstances.
**
**	Parameters:
**		e -- envelope to build it in/from.
**		type -- the file type, used as the first character
**			of the file name.
**
**	Returns:
**		a pointer to the new file name (in a static buffer).
**
**	Side Effects:
**		Will create the qf file if no id code is
**		already assigned.  This will cause the envelope
**		to be modified.
*/

char *
queuename(e, type)
	register ENVELOPE *e;
	char type;
{
	static char buf[MAXNAME];
	static int pid = -1;
	char c1 = 'A';
	char c2 = 'A';

	if (e->e_id == NULL)
	{
		char qf[20];

		/* find a unique id */
		if (pid != getpid())
		{
			/* new process -- start back at "AA" */
			pid = getpid();
			c1 = 'A';
			c2 = 'A' - 1;
		}
		(void) sprintf(qf, "qfAA%05d%04d", pid, (int) time(NULL) % 10000);

		while (c1 < '~' || c2 < 'Z')
		{
			int i;

			if (c2 >= 'Z')
			{
				c1++;
				c2 = 'A' - 1;
			}
			qf[2] = c1;
			qf[3] = ++c2;
# ifdef DEBUG
			if (tTd(7, 20))
				printf("queuename: trying \"%s\"\n", qf);
# endif /* DEBUG */
			i = open(qf, O_WRONLY|O_CREAT|O_EXCL, FileMode);
			if (i < 0) {
				if (errno != EEXIST) {
					syserr("queuename: Cannot create \"%s\" in \"%s\" (euid=%d)",
						qf, QueueDir, geteuid());
					exit(EX_UNAVAILABLE);
				}
			} else {
				(void) close(i);
				break;
			}
		}
		if (c1 >= '~' && c2 >= 'Z')
		{
			syserr("queuename: Cannot create \"%s\" in \"%s\" (euid=%d)",
				qf, QueueDir, geteuid());
			exit(EX_OSERR);
		}
		e->e_id = newstr(&qf[2]);
		define('i', e->e_id, e);
# ifdef DEBUG
		if (tTd(7, 1))
			printf("queuename: assigned id %s, env=%x\n", e->e_id, e);
# endif /* DEBUG */
# ifdef LOG
		if (LogLevel > 16)
			syslog(LOG_DEBUG, "%s: assigned id", e->e_id);
# endif /* LOG */
	}

	if (type == '\0')
		return (NULL);
	(void) sprintf(buf, "%cf%s", type, e->e_id);
# ifdef DEBUG
	if (tTd(7, 2))
		printf("queuename: %s\n", buf);
# endif /* DEBUG */
	return (buf);
}
/*
**  UNLOCKQUEUE -- unlock the queue entry for a specified envelope
**
**	Parameters:
**		e -- the envelope to unlock.
**
**	Returns:
**		none
**
**	Side Effects:
**		unlocks the queue for `e'.
*/

void unlockqueue(e)
	ENVELOPE *e;
{
	if (tTd(51, 4))
		printf("unlockqueue(%s)\n", e->e_id);

	/* if there is a lock file in the envelope, close it */
	if (e->e_lockfp != NULL)
		fclose(e->e_lockfp);
	e->e_lockfp = NULL;

	/* don't create a queue id if we don't already have one */
	if (e->e_id == NULL)
		return;

	/* remove the transcript */
# ifdef LOG
	if (LogLevel > 19)
		syslog(LOG_DEBUG, "%s: unlock", e->e_id);
# endif /* LOG */
	if (!tTd(51, 104))
		xunlink(queuename(e, 'x'));
}
/*
**  GETCTLUSER -- return controlling user if mailing to prog or file
**
**	Check for a "|" or "/" at the beginning of the address.  If
**	found, return a controlling username.
**
**	Parameters:
**		a - the address to check out
**
**	Returns:
**		Either NULL, if we werent mailing to a program or file,
**		or a controlling user name (possibly in getpwuid's
**		static buffer).
**
**	Side Effects:
**		none.
*/

char *
getctluser(a)
	ADDRESS *a;
{
	extern ADDRESS *getctladdr();
	struct passwd *pw;
	char *retstr;

	/*
	**  Get unquoted user for file, program or user.name check.
	**  N.B. remove this code block to always emit controlling
	**  addresses (at the expense of backward compatibility).
	*/

	{
		char buf[MAXNAME];
		(void) strncpy(buf, a->q_paddr, MAXNAME);
		buf[MAXNAME-1] = '\0';
		stripquotes(buf);

		if (buf[0] != '|' && buf[0] != '/')
			return((char *)NULL);
	}

	a = getctladdr(a);		/* find controlling address */

	if (a != NULL && a->q_uid != 0 && (pw = getpwuid(a->q_uid)) != NULL)
		retstr = pw->pw_name;
	else				/* use default user */
		retstr = DefUser;

# ifdef DEBUG
	if (tTd(40, 5))
		printf("Set controlling user for `%s' to `%s'\n",
		       (a == NULL)? "<null>": a->q_paddr, retstr);
# endif /* DEBUG */

	return(denlstring(retstr, TRUE, FALSE));
}
/*
**  SETCTLUSER - sets `CtlUser' to controlling user
**  CLRCTLUSER - clears controlling user (no params, nothing returned)
**
**	These routines manipulate `CtlUser'.
**
**	Parameters:
**		str  - controlling user as passed to setctluser()
**
**	Returns:
**		None.
**
**	Side Effects:
**		`CtlUser' is changed.
*/

static char CtlUser[MAXNAME];

static void setctluser(str)
register char *str;
{
	(void) strncpy(CtlUser, str, MAXNAME);
	CtlUser[MAXNAME-1] = '\0';
}

static void clrctluser()
{
	CtlUser[0] = '\0';
}

/*
**  SETCTLADDR -- create a controlling address
**
**	If global variable `CtlUser' is set and we are given a valid
**	address, make that address a controlling address; change the
**	`q_uid', `q_gid', and `q_ruser' fields and set QGOODUID.
**
**	Parameters:
**		a - address for which control uid/gid info may apply
**
**	Returns:
**		None.	
**
**	Side Effects:
**		Fills in uid/gid fields in address and sets QGOODUID
**		flag if appropriate.
*/

void setctladdr(a)
	ADDRESS *a;
{
	struct passwd *pw;

	/*
	**  If there is no current controlling user, or we were passed a
	**  NULL addr ptr or we already have a controlling user, return.
	*/

	if (CtlUser[0] == '\0' || a == NULL || a->q_ruser)
		return;

	/*
	**  Set up addr fields for controlling user.  If `CtlUser' is no
	**  longer valid, use the default user/group.
	*/

	if ((pw = getpwnam(CtlUser)) != NULL)
	{
		if (a->q_home)
			free(a->q_home);
		a->q_home = newstr(pw->pw_dir);
		a->q_uid = pw->pw_uid;
		a->q_gid = pw->pw_gid;
		a->q_ruser = newstr(CtlUser);
	}
	else
	{
		a->q_uid = DefUid;
		a->q_gid = DefGid;
		a->q_ruser = newstr(DefUser);
	}

	a->q_flags |= QGOODUID;		/* flag as a "ctladdr"  */

# ifdef DEBUG
	if (tTd(40, 5))
		printf("Restored controlling user for `%s' to `%s'\n",
		       a->q_paddr, a->q_ruser);
# endif /* DEBUG */
}
