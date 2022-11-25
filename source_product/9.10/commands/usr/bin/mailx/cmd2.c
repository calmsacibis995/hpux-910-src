/* @(#) $Revision: 66.1 $ */      
#

#include "rcv.h"
#include <sys/stat.h>

#ifndef NLS
#define catgets(i,sn,mn,s) (s)
#else NLS
#define NL_SETN 3	/* set number */
#include <nl_ctype.h>
#endif NLS

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * More user commands.
 */


/*
 * If any arguments were given, go to the next applicable argument
 * following dot, otherwise, go to the next applicable message.
 * If given as first command with no arguments, print first message.
 */

next(msgvec)
	int *msgvec;
{
	register struct message *mp;
	register int *ip, *ip2;
	int list[2], mdot;

	if (*msgvec != NULL) {

		/*
		 * If some messages were supplied, find the 
		 * first applicable one following dot using
		 * wrap around.
		 */

		mdot = dot - &message[0] + 1;

		/*
		 * Find the first message in the supplied
		 * message list which follows dot.
		 */

		for (ip = msgvec; *ip != NULL; ip++)
			if (*ip > mdot)
				break;
		if (*ip == NULL)
			ip = msgvec;
		ip2 = ip;
		do {
			mp = &message[*ip2 - 1];
			if ((mp->m_flag & MDELETED) == 0) {
				dot = mp;
				goto hitit;
			}
			if (*ip2 != NULL)
				ip2++;
			if (*ip2 == NULL)
				ip2 = msgvec;
		} while (ip2 != ip);
		printf((catgets(nl_fn,NL_SETN,1, "No messages applicable\n")));
		return(1);
	}

	/*
	 * If this is the first command, select message 1.
	 * Note that this must exist for us to get here at all.
	 */

	if (!sawcom)
		goto hitit;

	/*
	 * Just find the next good message after dot, no
	 * wraparound.
	 */

	for (mp = dot+1; mp < &message[msgCount]; mp++)
		if ((mp->m_flag & (MDELETED|MSAVED)) == 0)
			break;
	if (mp >= &message[msgCount]) {
		printf((catgets(nl_fn,NL_SETN,2, "At EOF\n")));
		return(0);
	}
	dot = mp;
hitit:
	/*
	 * Print dot.
	 */

	list[0] = dot - &message[0] + 1;
	list[1] = NULL;
	return(type(list));
}

/*
 * Save a message in a file.  Mark the message as saved
 * so we can discard when the user quits.
 */
save(str)
	char str[];
{

	return(save1(str, 1));
}

/*
 * Copy a message to a file without affected its saved-ness
 */
copycmd(str)
	char str[];
{

	return(save1(str, 0));
}

/*
 * Save/copy the indicated messages at the end of the passed file name.
 * If mark is true, mark the message "saved."
 */
save1(str, mark)
	char str[];
{
	register int *ip, mesg;
	register struct message *mp;
	char *file, *disp;
	int f, *msgvec, lc, cc, t;
	FILE *obuf;
	struct stat statb;
	int ispipe;

	msgvec = (int *) salloc((msgCount + 2) * sizeof *msgvec);
	if ((file = snarf(str, &f, 0, &ispipe)) == NOSTR)
		file = Getf("MBOX");
	if (f==-1)
		return(1);
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == NULL) {
#ifndef NLS
			printf("No messages to %s.\n", mark ? "save" : "copy");
#else
			if (mark)
				printf((catgets(nl_fn,NL_SETN,3, "No messages to save.\n")));
			else
				printf((catgets(nl_fn,NL_SETN,4, "No messages to copy.\n")));
#endif
			return(1);
		}
		msgvec[1] = NULL;
	}
	if (f && getmsglist(str, msgvec, 0) < 0)
		return(1);
	if (!ispipe && (file = expand(file)) == NOSTR)
		return(1);
	savemsglist(file, msgvec, mark, ispipe);
	return(0);
}

Save(msgvec)
int *msgvec;
{
	return(Save1(msgvec, 1));
}

Copy(msgvec)
int *msgvec;
{
	return(Save1(msgvec, 0));
}

/*
 * save/copy the indicated messages at the end of a file named
 * by the sender of the first message in the msglist.
 */
Save1(msgvec, mark)
int *msgvec;
{
	char file[128];
	char *from;

	from = nameof(&message[*msgvec-1], 0);
	getrecf(from, file, 1);
	savemsglist(expand(file), msgvec, mark, 0);
	return(0);
}

/*
 * save a message list in a file
 */
jmp_buf pipe_break;

savemsglist(file, msgvec, mark, ispipe)
char *file;
int *msgvec;
int ispipe;
{
	register int *ip, mesg;
	register struct message *mp;
	char *disp;
	FILE *obuf;
	struct stat statb;
	int lc, t;
	long cc;
	int break_pipe();

	printf("\"%s\" ", file);
	flush();
	if (ispipe)
		disp = (catgets(nl_fn,NL_SETN,5, "[Piped]"));
	else if (stat(file, &statb) >= 0)
		disp = (catgets(nl_fn,NL_SETN,6, "[Appended]"));
	else
		disp = (catgets(nl_fn,NL_SETN,7, "[New file]"));
	if (ispipe) {
		printf("\n");
		obuf = popen(file, "w");
	} else
		obuf = fopen(file, "a");
	if (setjmp(pipe_break)) {
		pclose(obuf);
		goto pipe0;
	}
	if (obuf == NULL) {
		perror("");
		return(1);
	} else signal(SIGPIPE, break_pipe);
	cc = 0L;
	lc = 0;
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		touch(mesg);
		mp = &message[mesg-1];
		if ((t = send(mp, obuf, 0)) < 0) {
			perror(file);
			fclose(obuf);
			return(1);
		}
		lc += t;
		cc += mp->m_size;
		if (mark)
			mp->m_flag |= MSAVED;
	}
	fflush(obuf);
	if (ferror(obuf))
		perror(file);
	if (ispipe) {
		if ((t = pclose(obuf)) != 0) {
			printf((catgets(nl_fn,NL_SETN,8, "\nExit status = %d.\n")),t>>8);
			return(1);
		}
	}
	else
		fclose(obuf);
	printf("%s %d/%ld\n", disp, lc, cc);
pipe0:
	signal(SIGPIPE, SIG_DFL);
}

/*
 * Write the indicated messages at the end of the passed
 * file name, minus header and trailing blank line.
 */

jmp_buf	pipe_stop;

swrite(str)
	char str[];
{
	register int *ip, mesg;
	register struct message *mp;
	register char *file, *disp;
	char linebuf[BUFSIZ];
	int f, *msgvec, lc, t;
	long cc;
	FILE *obuf, *mesf;
	struct stat statb;
	int ispipe;
	int stop_pipe();

	obuf = NULL;
	if (setjmp(pipe_stop)) {
		pclose(obuf);
		goto stop0;
	}
	msgvec = (int *) salloc((msgCount + 2) * sizeof *msgvec);
	if ((file = snarf(str, &f, 1, &ispipe)) == NOSTR)
		return(1);
	if (f==-1)
		return(1);
	if (!ispipe && (file = expand(file)) == NOSTR)
		return(1);
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == NULL) {
			printf((catgets(nl_fn,NL_SETN,9, "No messages to write.\n")));
			return(1);
		}
		msgvec[1] = NULL;
	}
	if (f && getmsglist(str, msgvec, 0) < 0)
		return(1);
	printf("\"%s\" ", file);
	flush();
	if (ispipe)
		disp = (catgets(nl_fn,NL_SETN,10, "[Piped]"));
	else if (stat(file, &statb) >= 0)
		disp = (catgets(nl_fn,NL_SETN,11, "[Appended]"));
	else
		disp = (catgets(nl_fn,NL_SETN,12, "[New file]"));
	if (ispipe)
		obuf = popen(file, "w");
	else
		obuf = fopen(file, "a");
	if (obuf == NULL) {
		perror("");
		return(1);
	} else signal(SIGPIPE, stop_pipe);
	cc = 0L;
	lc = 0;
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		touch(mesg);
		mp = &message[mesg-1];
		mesf = setinput(mp);
		t = mp->m_lines - 2;
		readline(mesf, linebuf);
		while (t-- > 0) {
			fgets(linebuf, BUFSIZ, mesf);
			fputs(linebuf, obuf);
			cc += (long)strlen(linebuf);
		}
		lc += mp->m_lines - 2;
		mp->m_flag |= MSAVED;
	}
	fflush(obuf);
	if (ferror(obuf))
		perror(file);
	if (ispipe) {
		if ((t = pclose(obuf)) != 0) {
			printf((catgets(nl_fn,NL_SETN,13, "\nExit status = %d.\n")),t>>8);
			return(1);
		}
	}
	else
		fclose(obuf);
	printf("%s %d/%ld\n", disp, lc, cc);
	return(0);
stop0:
	signal(SIGPIPE, SIG_DFL);
}

/*
 * Snarf the file from the end of the command line and
 * return a pointer to it.  If there is no file attached,
 * just return NOSTR.  Put a null in front of the file
 * name so that the message list processing won't see it,
 * unless the file name is the only thing on the line, in
 * which case, return 0 in the reference flag variable.
 * A file name can also be a pipe; in that case everything
 * to the right of the pipe symbol ('|') is taken as the name.
 */

char *
snarf(linebuf, flag, erf, ispipe)
	char linebuf[];
	int *flag;
	int *ispipe;
{
	register char *cp, *tp;
	char end;

	cp = strlen(linebuf) + linebuf - 1;
	*flag = 1;

	/* 
	 * first look left to right for '|'
	 */
	tp = linebuf;
	while (tp <= cp && *tp != '|')
#ifdef NLS
		ADVANCE(tp);
#else
		tp++;
#endif NLS
	if ((*ispipe = (*tp == '|'))) {
		*tp = '\0';
		*flag = (tp > linebuf);
		return (++tp);
	}
	/* 
	 * now look right to left for an ordinary name
	 */

	/*
	 * Strip away trailing blanks.
	 */

	while (*cp == ' ' && cp > linebuf)
		cp--;
	*++cp = 0;

	/*
	 * Now see if string is quoted
	 */
	if (cp > linebuf && any(cp[-1], "'\"")) {
		end = *--cp;
		*cp = '\0';
		while (*cp != end && cp > linebuf)
			cp--;
		if (*cp != end) {
			printf((catgets(nl_fn,NL_SETN,14, "Syntax error: missing %c.\n")), end);
			*flag = -1;
			return(NOSTR);
		}
		if (cp==linebuf)
			*flag = 0;
		*cp++ = '\0';
		return(cp);
	}

	/*
	 * Now search for the beginning of the file name.
	 */

	while (cp > linebuf && !any(*cp, "\t "))
		cp--;
	if (*cp == '\0') {
		if (erf)
			printf((catgets(nl_fn,NL_SETN,15, "No file specified.\n")));
		*flag = 0;
		return(NOSTR);
	}
	if (any(*cp, " \t"))
		*cp++ = 0;
	else
		*flag = 0;
	return(cp);
}

/*
 * Delete messages.
 */

delete(msgvec)
	int msgvec[];
{
	return(delm(msgvec));
}

/*
 * Delete messages, then type the new dot.
 */

deltype(msgvec)
	int msgvec[];
{
	int list[2];
	int lastdot;

	lastdot = dot - &message[0] + 1;
	if (delm(msgvec) >= 0) {
		list[0] = dot - &message[0];
		list[0]++;
		if (list[0] > lastdot) {
			touch(list[0]);
			list[1] = NULL;
			return(type(list));
		}
		printf((catgets(nl_fn,NL_SETN,16, "At EOF\n")));
		return(0);
	}
	else {
		printf((catgets(nl_fn,NL_SETN,17, "No more messages\n")));
		return(0);
	}
}

/*
 * Delete the indicated messages.
 * Set dot to some nice place afterwards.
 * Internal interface.
 */

delm(msgvec)
	int *msgvec;
{
	register struct message *mp;
	register *ip, mesg;
	int last;

	last = NULL;
	for (ip = msgvec; *ip != NULL; ip++) {
		mesg = *ip;
		touch(mesg);
		mp = &message[mesg-1];
		mp->m_flag |= MDELETED|MTOUCH;
		mp->m_flag &= ~(MPRESERVE|MSAVED|MBOX);
		last = mesg;
	}
	if (last != NULL) {
		dot = &message[last-1];
		last = first(0, MDELETED);
		if (last != NULL) {
			dot = &message[last-1];
			return(0);
		}
		else {
			dot = &message[0];
			return(-1);
		}
	}

	/*
	 * Following can't happen -- it keeps lint happy
	 */

	return(-1);
}

/*
 * Undelete the indicated messages.
 */

undelete(msgvec)
	int *msgvec;
{
	register struct message *mp;
	register *ip, mesg;

	for (ip = msgvec; ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		if (mesg == 0)
			return;
		touch(mesg);
		mp = &message[mesg-1];
		dot = mp;
		mp->m_flag &= ~MDELETED;
	}
}

/*
 * Add the given header fields to the ignored list.
 * If no arguments, print the current list of ignored fields.
 */
igfield(list)
	char *list[];
{
	char field[BUFSIZ];
	register int h;
	register struct ignore *igp;
	char **ap;

	if (argcount(list) == 0)
		return(igshow());
	for (ap = list; *ap != 0; ap++) {
		if (isign(*ap))
			continue;
		istrcpy(field, *ap);
		h = hash(field);
		igp = (struct ignore *) calloc(1, sizeof (struct ignore));
		igp->i_field = calloc(strlen(field) + 1, sizeof (char));
		strcpy(igp->i_field, field);
		igp->i_link = ignore[h];
		ignore[h] = igp;
	}
	return(0);
}

/*
 * Print out all currently ignored fields.
 */
igshow()
{
	register int h, count;
	struct ignore *igp;
	char **ap, **ring;
	int igcomp();

	count = 0;
	for (h = 0; h < HSHSIZE; h++)
		for (igp = ignore[h]; igp != 0; igp = igp->i_link)
			count++;
	if (count == 0) {
		printf((catgets(nl_fn,NL_SETN,18, "No fields currently being ignored.\n")));
		return(0);
	}
	ring = (char **) salloc((count + 1) * sizeof (char *));
	ap = ring;
	for (h = 0; h < HSHSIZE; h++)
		for (igp = ignore[h]; igp != 0; igp = igp->i_link)
			*ap++ = igp->i_field;
	*ap = 0;
	qsort(ring, count, sizeof (char *), igcomp);
	for (ap = ring; *ap != 0; ap++)
		printf("%s\n", *ap);
	return(0);
}

/*
 * Compare two names for sorting ignored field list.
 */
igcomp(l, r)
	char **l, **r;
{

	return(strcmp(*l, *r));
}


/*
 * Is ch any of the characters in str?
 */

any(ch, str)
	char *str;
	char ch;
{
	register char *f;
	register char c;

	if (str == NOSTR) return(0);
	f = str;
	c = ch;
	while (*f)
#ifdef NLS
		if (c == CHARADV(f))
#else
		if (c == *f++)
#endif NLS
			return(1);
	return(0);
}

/*
 * Respond to a broken pipe signal
 */

break_pipe()
{
# ifndef VMUNIX
	signal(SIGPIPE, pipe_break);
# endif
	longjmp(pipe_break, 1);
}

stop_pipe()
{
# ifndef VMUNIX
	signal(SIGPIPE, stop_pipe);
# endif
	longjmp(pipe_stop, 1);
}
