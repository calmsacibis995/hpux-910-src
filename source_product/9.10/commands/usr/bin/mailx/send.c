/* @(#) $Revision: 70.2 $ */      
#

#include "rcv.h"
#ifdef VMUNIX
#include <wait.h>
#endif
#include <ctype.h>

#ifndef NLS
#define catgets(i,sn,mn,s) (s)
#else NLS
#define NL_SETN 15	/* set number */
#include <locale.h>
extern int __nl_langid[];
#endif NLS


int	warn = 0; 

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Mail to others.
 */


/*
 * Send message described by the passed pointer to the
 * passed output buffer.  Return -1 on error, but normally
 * the number of lines written.  Adjust the status: field
 * if need be.  If doign is set, suppress ignored header fields.
 */
send(mailp, obuf, doign)
	struct message *mailp;
	FILE *obuf;
{
	register struct message *mp;
	register int t;
	long c;
	FILE *ibuf;
	char line[LINESIZE], field[BUFSIZ];
	int lc, ishead, infld, fline, dostat;
	char *cp, *cp2;
	int oldign = 0;	/* previous line was ignored */
#ifdef NLS
	char *nl_cxtime(), *nldp, nldate[LINESIZE];
	extern long mailx_getdate(); /* converts ASCII date string to long */
	long fromdate;
#endif NLS

	mp = mailp;
	ibuf = setinput(mp);
	c = mp->m_size;
	ishead = 1;
	dostat = 1;
	infld = 0;
	fline = 1;
	lc = 0;
	clearerr(obuf);
	while (c > 0L) {
		fgets(line, LINESIZE, ibuf);
		c -= (long)strlen(line);
		lc++;
		if (ishead) {
			/* 
			 * First line is the From line, so no headers
			 * there to worry about
			 */
			if (fline) {
#ifdef NLS
				if ((__nl_langid[LC_TIME] != 0 && __nl_langid[LC_TIME] != 99) && (doign & PBIT))  {
				/* The PBIT is set by the print()
				 * function only.  This keeps us from
				 * munging the "From " line in all
				 * other cases when send() is called.
				 */
					nldp = strchr(line, ' ');
					nldp = strchr(++nldp, ' ');
					/* Copy ASCII date string
					 * to nldate for conversion
					 */
					strcpy(nldate,++nldp);
					/* Terminate "From " line
					 * at space after username
					 */
					*nldp = '\0';
					fromdate = mailx_getdate(nldate, NULL);
					if (fromdate != -1)   /* fix for DSDe414531 */
					   strcpy(nldate, nl_cxtime(&fromdate, ""));
					/* nldate now contains the new
					 * NLS-formatted date
					 */
					strcat(nldate, "\n");
					strcat(line, nldate);
				}
				doign &= ~PBIT; /* Unset PBIT */
#endif NLS
				fline = 0;
				goto writeit;
			}
			/*
			 * If line is blank, we've reached end of
			 * headers, so force out status: field
			 * and note that we are no longer in header
			 * fields
			 */
			if (line[0] == '\n') {
				if (dostat) {
					statusput(mailp, obuf, doign);
					dostat = 0;
				}
				ishead = 0;
				goto writeit;
			}
			/*
			 * If this line is a continuation
			 * of a previous header field, just echo it.
			 */
			if (isspace(line[0]) && infld)
				if (oldign)
					continue;
				else
					goto writeit;
			infld = 0;
			/*
			 * If we are no longer looking at real
			 * header lines, force out status:
			 * This happens in uucp style mail where
			 * there are no headers at all.
			 */
			if (!headerp(line)) {
				if (dostat) {
					statusput(mailp, obuf, doign);
					dostat = 0;
				}
				putc('\n', obuf);
				ishead = 0;
				goto writeit;
			}
			infld++;
			/*
			 * Pick up the header field.
			 * If it is an ignored field and
			 * we care about such things, skip it.
			 */
			cp = line;
			cp2 = field;
			while (*cp && *cp != ':' && !isspace(*cp))
				*cp2++ = *cp++;
			*cp2 = 0;
			oldign = doign && isign(field);
			if (oldign)
				continue;
			/*
			 * If the field is "status," go compute and print the
			 * real Status: field
			 */
			if (icequal(field, "status")) {
				if (dostat) {
					statusput(mailp, obuf, doign);
					dostat = 0;
				}
				continue;
			}
		}
writeit:
		fputs(line, obuf);
		if (ferror(obuf))
			return(-1);
	}
	if (ferror(obuf))
		return(-1);
	if (ishead && (mailp->m_flag & MSTATUS))
		printf((catgets(nl_fn,NL_SETN,1, "failed to fix up status field\n")));
	return(lc);
}

/*
 * Test if the passed line is a header line, RFC 733 style.
 */
headerp(line)
	register char *line;
{
	register char *cp = line;

	if (*cp=='>' && strncmp(cp+1, "From", 4)==0)
		return(1);
	while (*cp && !isspace(*cp) && *cp != ':')
		cp++;
	while (*cp && isspace(*cp))
		cp++;
	return(*cp == ':');
}

/*
 * Output a reasonable looking status field.
 * But if "status" is ignored and doign, forget it.
 */
statusput(mp, obuf, doign)
	register struct message *mp;
	register FILE *obuf;
{
	char statout[3];

	if (doign && isign("status"))
		return;
	if ((mp->m_flag & (MNEW|MREAD)) == MNEW)
		return;
	if (mp->m_flag & MREAD)
		strcpy(statout, "R");
	else
		strcpy(statout, "");
	if ((mp->m_flag & MNEW) == 0)
		strcat(statout, "O");
	fprintf(obuf, "Status: %s\n", statout);
}


/*
 * Interface between the argument list and the mail1 routine
 * which does all the dirty work.
 */

mail(people)
	char **people;
{
	register char *cp2;
	register int s;
	char *buf, **ap;
	char *person;
	struct header head;
	char *unuucp();
	char recfile[128];

	for (s = 0, ap = people; *ap != (char *) -1; ap++)
		s += strlen(*ap) + 1;
	buf = salloc(s+1);
	cp2 = buf;
	for (ap = people; *ap != (char *) -1; ap++) {
		cp2 = copy(*ap, cp2);
		*cp2++ = ' ';
	}
	if (cp2 != buf)
		cp2--;
	*cp2 = '\0';
	head.h_to = buf;
	if (Fflag) {
		char *tp, *bp, tbuf[128];
		struct name *to;
		/*
		 * peel off the first recipient and expand a possible alias
		 */
		for (tp=tbuf, bp=buf; *bp && *bp!=' '; bp++, tp++)
			*tp = *bp;
		*tp = '\0';
		if (debug)
			fprintf(stderr, "First recipient is '%s'\n", tbuf);
		if ((to = usermap(extract(tbuf, GTO)))==NIL) {
			printf((catgets(nl_fn,NL_SETN,2, "No recipients specified\n")));
			return(1);
		}
		getrecf(to->n_name, recfile, Fflag);
	} else
		getrecf(buf, recfile, Fflag);
	head.h_subject = NOSTR;
	head.h_cc = NOSTR;
	head.h_bcc = NOSTR;
	head.h_replyto = NOSTR;
	head.h_seq = 0;
	mail1(&head, recfile);
	return(0);
}


/*
 * Send mail to a bunch of user names.  The interface is through
 * the mail routine below.
 */

sendmail(str)
	char *str;
{
	register char **ap;
	char *bufp;
	register int t;
	struct header head;
	char recfile[128];

	if (blankline(str))
		head.h_to = NOSTR;
	else
		head.h_to = str;
	head.h_subject = NOSTR;
	head.h_cc = NOSTR;
	head.h_bcc = NOSTR;
	head.h_replyto = NOSTR;
	head.h_seq = 0;
	/*
	   Do proper folder expansion if necessary.  This fixes the
	   record variables bug (DSDrs00156).  Now we get same
	   behavior from mailx command line and from the shell.
	 */
	getrecf(0, recfile, 0);
	mail1(&head, recfile);
	return(0);
}

/*
 * Mail a message on standard input to the people indicated
 * in the passed header.  (Internal interface).
 */

mail1(hp, recfile)
	struct header *hp;
	char *recfile;
{
	register char *cp;
	struct name *to, *np;
	FILE *mtf;
	int pid, i, s, p, gotcha;
	int remote = rflag != NOSTR || rmail;
	int k = 0;
	int ask = 0;
	int prwarn();
	char **namelist, *deliver;
	char **t;
	char *deadletter = Getf("DEAD");

	/*
	 * Collect user's mail from standard input.
	 * Get the result as mtf.
	 */

	pid = -1;
	if (hp->h_replyto == NOSTR && (cp = value("replyto")) != NOSTR)
		hp->h_replyto = cp;
	if ((mtf = collect(hp)) == NULL)
		return(-1);
	hp->h_seq = 1;
	if (hp->h_subject == NOSTR)
		hp->h_subject = sflag;
	if (fsize(mtf) == 0 && hp->h_subject == NOSTR) {
		printf((catgets(nl_fn,NL_SETN,3, "No message !?!\n")));
		goto out;
	}
	if (intty && value("askcc") != NOSTR) {
		void (*int_hd)();
		int_hd = sigset(SIGINT, SIG_IGN);
		grabh(hp, GCC);
		ask++;
		sigset(SIGINT, int_hd);
	}
	if (intty && value("askbcc") != NOSTR) {
		void (*int_hd1)();
		int_hd1 = sigset(SIGINT, SIG_IGN);
		grabh(hp, GBCC);
		ask++;
		sigset(SIGINT, int_hd1);
	}
	else if (intty && !ask) {
		ask = 0;
		printf((catgets(nl_fn,NL_SETN,4, "EOT\n")));
		flush();
	}

	
	to = cat(extract(skin(hp->h_bcc), GBCC),
 	        cat(extract(skin(hp->h_to), GTO), extract(skin(hp->h_cc), GCC)));

	/*
	 * If it's rmail, step on any file or pipes in the
	 * list, it's a security hole otherwise. Do this before
	 * alias processing, thus allowing local aliases to be
	 * files or pipes.
	 */

	if (rmail) {
		nofiles(to);
		if (to == NIL) {
			printf((catgets(nl_fn,NL_SETN,5, "No recipients specified\n")));
			goto topdog;
		}
	}

	/*
	 * Now, take the user names from the combined
	 * to and cc lists and do all the alias
	 * processing.
	 */

	senderr = 0;
	to = usermap(to);
	if (to == NIL) {
		printf((catgets(nl_fn,NL_SETN,6, "No recipients specified\n")));
		goto topdog;
	}

	/*
	 * Look through the recipient list for names with /'s
	 * in them which we write to as files directly.
	 */

	to = outof(to, mtf, hp);
	rewind(mtf);
	to = verify(to);
	if (senderr && !remote) {
topdog:

		if (fsize(mtf) != 0) {
			remove(deadletter);
			exwrite(deadletter, mtf, 1, 0);
			rewind(mtf);
		}
	}
	for (gotcha = 0, np = to; np != NIL; np = np->n_flink)
		if ((np->n_type & GDEL) == 0) {
			gotcha++;
			np->n_name = unuucp(np->n_name);
		}
	if (!gotcha)
		goto out;
	to = elide(to);
	mechk(to);
	if (count(to) > 1)
		hp->h_seq++;
#ifndef NOCOMMA
	fixhead(hp, to);
#endif
	if (hp->h_seq > 0 && !remote) {
#ifdef NOCOMMA
		fixhead(hp, to);
#endif
		if (fsize(mtf) == 0)
		    if (hp->h_subject == NOSTR)
			printf((catgets(nl_fn,NL_SETN,7, "No message, no subject; hope that's ok\n")));
		    else
			printf((catgets(nl_fn,NL_SETN,8, "Null message body; hope that's ok\n")));
		if ((mtf = infix(hp, mtf)) == NULL) {
			fprintf(stderr, (catgets(nl_fn,NL_SETN,9, ". . . message lost, sorry.\n")));
			return(-1);
		}
	}
	namelist = unpack(to);
	if (debug) {
		fprintf(stderr, "Recipients of message:\n");
		for (t = namelist; *t != NOSTR; t++)
			fprintf(stderr, " \"%s\"", *t);
		fprintf(stderr, "\n");
	}
	if (recfile != NOSTR && *recfile)
		savemail(expand(recfile), hp, mtf);

	/*
	 * Wait, to absorb a potential zombie, then
	 * fork, set up the temporary mail file as standard
	 * input for "mail" and exec with the user list we generated
	 * far above. Return the process id to caller in case he
	 * wants to await the completion of mail.
	 */

	signal(SIGALRM, prwarn);
	alarm(1);
#ifdef VMUNIX
	while (wait3(&s, WNOHANG, 0) > 0)
		;
#else
	if (wait(&s) == -1)
		wait(&s);
#endif  
	alarm(0);
	if (warn) {
		while (k != 51) {
		 	printf("\b");
			k++;
		}
		while (k != 0) {
			printf(" ");
			k--;
		}
		while (k != 51) {
	 		printf("\b");
			k++;
		}
		warn = 0;
	}	
	signal(SIGALRM, SIG_DFL);
	rewind(mtf);
	pid = fork();
	if (pid == -1) {
		perror("fork");
		remove(deadletter);
		exwrite(deadletter, mtf, 1, 0);
		goto out;
	}
	if (pid == 0) {
		sigchild();
#ifdef SIGTSTP
		if (remote == 0) {
			sigset(SIGTSTP, SIG_IGN);
			sigset(SIGTTIN, SIG_IGN);
			sigset(SIGTTOU, SIG_IGN);
		}
#endif
		sigignore(SIGHUP);
		sigignore(SIGINT);
		sigignore(SIGQUIT);
		s = fileno(mtf);
		for (i = 3; i < 15; i++)
			if (i != s)
				close(i);
		close(0);
		dup(s);
		close(s);
#ifdef CC
		submit(getpid());
#endif CC
		if ((deliver = value("sendmail")) == NOSTR)
#ifdef DELIVERMAIL
			deliver = DELIVERMAIL;
#else
		{
			deliver = MAIL;
			namelist[0] = "rmail";
		}
#endif DELIVERMAIL
		execvp(deliver, namelist);

		/* drop thru to the ordinary mailer if sendmail wasn't found.
		   Since the Bell mailer is dumb, the options have to be
		   stripped off again.  Either we need to bring /bin/mail
		   up to ucb in this area, or keep this code here.
		   If SENDMAIL isn't defined, then this is just extra work,
		   but its simpler this way. */

		for (t = &namelist[1]; *t != NOSTR; ) {
			if (*t[0] != '-') break;
			if (strcmp(*t, "-h") == 0) t++; /* -h <arg> */
			t++;
		}
		t[-1] = namelist[0];
		namelist = &t[-1];

		if (debug) {
			printf("Arguments to execv(MAIL,namelist):\n");
			for (t = namelist; *t != NOSTR; t++)
				printf(" \"%s\"", *t);
			printf("\n");
			fflush( stdout );
		}

		namelist[0] = "rmail"; /* reset argv[0] to "rmail" */
		execvp(MAIL, namelist);
		perror(MAIL);
		exit(1);
	}

	if (value("sendwait")!=NOSTR)
		remote++;
out:
	if (remote) {
		while ((p = wait(&s)) != pid && p != -1)
			;
		if (s != 0)
			senderr++;
		pid = 0;
	}
	fclose(mtf);
	return(pid);
}

/*
 * Fix the header by glopping all of the expanded names from
 * the distribution list into the appropriate fields.
 * If there are any ARPA net recipients in the message,
 * we must insert commas, alas.
 */

fixhead(hp, tolist)
	struct header *hp;
	struct name *tolist;
{
	register struct name *nlist;
	register int f;
	register struct name *np;

#ifdef NOCOMMA
	for (f = 0, np = tolist; np != NIL; np = np->n_flink)
		if ( any('@', np->n_name) || any('!', np->n_name) ) {
			f |= GCOMMA;
			break;
		}
#else
	f = GCOMMA;
#endif NOCOMMA

	if (debug && (f & GCOMMA))
		fprintf(stderr, "Should be inserting commas in recip lists\n");
	hp->h_to = detract(tolist, GTO|f);
	hp->h_cc = detract(tolist, GCC|f);
}

/*
 * Prepend a header in front of the collected stuff
 * and return the new file.
 */

FILE *
infix(hp, fi)
	struct header *hp;
	FILE *fi;
{
	extern char tempMail[];
	register FILE *nfo, *nfi;
	register int c;
	register char line[LINESIZE];
	register char *cp = line;

	rewind(fi);
	if ((nfo = fopen(tempMail, "w")) == NULL) {
		perror(tempMail);
		return(fi);
	}
	if ((nfi = fopen(tempMail, "r")) == NULL) {
		perror(tempMail);
		fclose(nfo);
		return(fi);
	}
	remove(tempMail);
	puthead(hp, nfo, GTO|GSUBJECT|GCC|GNL|GREPLYTO);
	while (readline(fi, line) > 0) {
		if (strncmp(cp, "From", 4) == 0) {
			putc('>', nfo);
			fputs(line, nfo);
		} else
			fputs(line, nfo);
		putc('\n', nfo);
	}
	if (ferror(fi)) {
		perror("read");
		return(fi);
	}
	fflush(nfo);
	if (ferror(nfo)) {
		perror(tempMail);
		fclose(nfo);
		fclose(nfi);
		return(fi);
	}
	fclose(nfo);
	fclose(fi);
	rewind(nfi);
	return(nfi);
}

/*
 * Dump the to, subject, cc header on the
 * passed file buffer.
 */

puthead(hp, fo, w)
	struct header *hp;
	FILE *fo;
{
	register int gotcha;

	gotcha = 0;
	if (hp->h_to != NOSTR && w & GTO)
		fprintf(fo, "To: "), fmt(hp->h_to, fo), gotcha++;
	if (w & GSUBJECT)
		if (hp->h_subject != NOSTR && *hp->h_subject)
			fprintf(fo, "Subject: %s\n", hp->h_subject), gotcha++;
		else
			if (sflag && *sflag)
				fprintf(fo, "Subject: %s\n", sflag), gotcha++;
	if (hp->h_cc != NOSTR && w & GCC)
		fprintf(fo, "Cc: "), fmt(hp->h_cc, fo), gotcha++;
	if (hp->h_bcc != NOSTR && w & GBCC)
		fprintf(fo, "Bcc: "), fmt(hp->h_bcc, fo), gotcha++;
	if (hp->h_replyto != NOSTR && (w & GREPLYTO))
		fprintf(fo, "Reply-to: "), fmt(hp->h_replyto, fo), gotcha++;
	if (gotcha && w & GNL)
		putc('\n', fo);
	return(0);
}

/*
 * Format the given text to not exceed 72 characters.
 */

fmt(str, fo)
	register char *str;
	register FILE *fo;
{
	register int col;
	register char *cp;
	void shiftpipe();

	cp = str;
	/* Check for pipe character in string */
	if (index(cp, '|'))
		shiftpipe(cp);
	col = 0;
	while (*cp) {
		if (*cp == ' ' && col > 65) {
			fprintf(fo, "\n    ");
			col = 4;
			cp++;
			continue;
		}
		putc(*cp++, fo);
		col++;
	}
	putc('\n', fo);
}

/*
 * Break the string down into space or comma separated tokens and
 * shift the pipe character and it's following command to the
 * start of the token.  For example: "hpda!|cmd" becomes "|cmd"
 */

void
shiftpipe(s1)
	char *s1;
 {
	char *ptr1, *ptr2;
	char *s2;

	s2 = (char *)malloc(strlen(s1) + 1); /* Get space for work area */

	/* Get the first token */
	ptr1 = (char *)strtok(s1, " ,");
	ptr2 = ptr1;
	/*
	 * If there's a pipe character, make sure it's at the start
	 * of the token
	 */
	if (ptr1 = index(ptr1, '|'))
		ptr2 = ptr1;
	(void) strcpy(s2, ptr2);

	/* Get other tokens in the string */
	while (ptr1 = (char *)strtok(NULL, " ,")) {
		ptr2 = ptr1;
		/*
	 	 * If there's a pipe character, make sure it's at the
		 * start of the token and add it to the end of the
		 * string after a space.
	 	 */
		if (ptr1 = index(ptr1, '|'))
			ptr2 = ptr1;
		(void) strcat(s2, " ");
		(void) strcat(s2, ptr2);
	}
	(void) strcpy(s1, s2);
	free(s2);
}

/*
 * Save the outgoing mail on the passed file.
 */

savemail(name, hp, fi)
	char name[];
	struct header *hp;
	FILE *fi;
{
	register FILE *fo;
	register int c;
	long now;
	char *n;

	if (debug)
		fprintf(stderr, "save in '%s'\n", name);
	if ((fo = fopen(name, "a")) == NULL) {
		perror(name);
		return(-1);
	}
	time(&now);
	n = rflag;
	if (n == NOSTR)
		n = myname;
	fprintf(fo, "From %s %s", n, ctime(&now));
	rewind(fi);
	for (c = getc(fi); c != EOF; c = getc(fi))
		putc(c, fo);
	fprintf(fo, "\n");
	fflush(fo);
	if (ferror(fo))
		perror(name);
	fclose(fo);
	return(0);
}

	/*
         * print out the reason of the waiting
	 */

prwarn()
{
	if (rcvmode) {
		printf("mailx: waiting for sending previous mail completely");
		flush();
		warn = 1;
	} 
}
