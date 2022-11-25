/* @(#) $Revision: 64.3 $ */   
#

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Collect input from standard input, handling
 * ~ escapes.
 */


#include "rcv.h"
#include <sys/stat.h>

#ifndef NLS
#define catgets(i,sn,mn,s) (s)
#else NLS
#define NL_SETN 6	/* set number */
#endif NLS

/*
 * Read a message from standard output and return a read file to it
 * or NULL on error.
 */

/*
 * The following hokiness with global variables is so that on
 * receipt of an interrupt signal, the partial message can be salted
 * away on dead.letter.  The output file must be available to flush,
 * and the input to read.  Several open files could be saved all through
 * mailx if stdio allowed simultaneous read/write access.
 */

static	void	(*savesig)();		/* Previous SIGINT value */
static	void	(*savehup)();		/* Previous SIGHUP value */
# ifdef VMUNIX
static	void	(*savecont)();		/* Previous SIGCONT value */
# endif VMUNIX
# ifdef SIGTSTP
static  void	(*savestp)();		/* Previous SIGTSTP value */
# endif SIGTSTP
static	FILE	*newi;			/* File for saving away */
static	FILE	*newo;			/* Output side of same */
static	int	hf;			/* Ignore interrups */
int	hadintr;		/* Have seen one SIGINT so far */

static	jmp_buf	coljmp;			/* To get back to work */
extern	char	tempMail[], tempEdit[];

FILE *
collect(hp)
	struct header *hp;
{
	FILE *ibuf, *fbuf, *obuf;
	int lc, cc, escape, collrub(), intack(), collhup, collcont(), eof, colltstp();
	register int c, t;
	char linebuf[LINESIZE], *cp;
	extern char tempMail[];
	int notify();
	extern collintsig(), collhupsig();

	noreset++;
	ibuf = obuf = NULL;
	if (value("ignore") != NOSTR)
		hf = 1;
	else
		hf = 0;
	hadintr = 0;
# ifdef VMUNIX
	if ((savesig = sigset(SIGINT, SIG_IGN)) != SIG_IGN)
		sigset(SIGINT, hf ? intack : collrub), sigblock(mask(SIGINT));
	if ((savehup = sigset(SIGHUP, SIG_IGN)) != SIG_IGN)
		sigset(SIGHUP, collrub), sigblock(mask(SIGHUP));
	savecont = sigset(SIGCONT, collcont);
# else VMUNIX
	savesig = sigset(SIGINT, SIG_IGN);
	savehup = sigset(SIGHUP, SIG_IGN);
# endif VMUNIX
# ifdef SIGTSTP
	savestp = sigset(SIGTSTP, colltstp);
# endif
	newi = NULL;
	newo = NULL;
	if ((obuf = fopen(tempMail, "w")) == NULL) {
		perror(tempMail);
		goto err;
	}
	newo = obuf;
	if ((ibuf = fopen(tempMail, "r")) == NULL) {
		perror(tempMail);
		newo = NULL;
		fclose(obuf);
		goto err;
	}
	newi = ibuf;
	remove(tempMail);

	/*
	 * If we are going to prompt for a subject,
	 * refrain from printing a newline after
	 * the headers (since some people mind).
	 */

	t = GTO|GSUBJECT|GCC|GNL;
	c = 0;
	if (hp->h_subject == NOSTR) {
		hp->h_subject = sflag;
		sflag = NOSTR;
	}
	if (intty && hp->h_subject == NOSTR && value("asksub"))
		t &= ~GNL, c++;
	if (hp->h_seq != 0) {
		puthead(hp, stdout, t);
		fflush(stdout);
	}
	if (c)
		grabh(hp, GSUBJECT);
	escape = ESCAPE;
	if ((cp = value("escape")) != NOSTR)
		escape = *cp;
	eof = 0;
	for (;;) {
# ifdef VMUNIX
		int omask = sigblock(0) &~ (mask(SIGINT)|mask(SIGHUP));
# endif VMUNIX

		setjmp(coljmp);
# ifdef VMUNIX
		sigsetmask(omask);
# else VMUNIX
		if (savesig != SIG_IGN)
			signal(SIGINT, hf ? intack : collintsig);
		if (savehup != SIG_IGN)
			signal(SIGHUP, collhupsig);
# endif VMUNIX
		flush();
		if (readline(stdin, linebuf) <= 0) {
			if (intty && value("ignoreeof") != NOSTR) {
				if (++eof > 35)
					break;
				printf((catgets(nl_fn,NL_SETN,1, "Use \".\" to terminate letter\n")),
				    escape);
				continue;
			}
			break;
		}
		eof = 0;
		hadintr = 0;
		if (intty && equal(".", linebuf) &&
		    (value("dot") != NOSTR || value("ignoreeof") != NOSTR))
			break;
		if (linebuf[0] != escape || rflag != NOSTR) {
			if ((t = putline(obuf, linebuf)) < 0)
				goto err;
			continue;
		}
		c = linebuf[1];
		switch (c) {
		default:
			/*
			 * On double escape, just send the single one.
			 * Otherwise, it's an error.
			 */

			if (c == escape) {
				if (putline(obuf, &linebuf[1]) < 0)
					goto err;
				else
					break;
			}
			printf((catgets(nl_fn,NL_SETN,2, "Unknown tilde escape.\n")));
			break;

		case 'a':
		case 'A':
			/*
			 * autograph; sign the letter.
			 */

			if (cp = value(c=='a' ? "sign":"Sign")) {
			      cpout( cp, obuf);
			      if (isatty(fileno(stdin)))
				    cpout( cp, stdout);
			}

			break;

		case 'i':
			/*
			 * insert string
			 */
			for (cp = &linebuf[2]; any(*cp, " \t"); cp++)
				;
			if (*cp)
				cp = value(cp);
			if (cp != NOSTR) {
				cpout(cp, obuf);
				if (isatty(fileno(stdout)))
					cpout(cp, stdout);
			}
			break;

		case '!':
			/*
			 * Shell escape, send the balance of the
			 * line to sh -c.
			 */

			shell(&linebuf[2]);
			break;

		case ':':
		case '_':
			/*
			 * Escape to command mode, but be nice!
			 */

			execute(&linebuf[2], 1);
			printf((catgets(nl_fn,NL_SETN,3, "(continue)\n")));
			break;

		case '.':
			/*
			 * Simulate end of file on input.
			 */
			goto eofl;

		case 'q':
		case 'Q':
			/*
			 * Force a quit of sending mail.
			 * Act like an interrupt happened.
			 */

			hadintr++;
			collrub(SIGINT);
			exit(1);

		case 'x':
			xhalt();
			break; 	/* not reached */

		case 'h':
			/*
			 * Grab a bunch of headers.
			 */
			if (!intty || !outtty) {
				printf((catgets(nl_fn,NL_SETN,4, "~h: no can do!?\n")));
				break;
			}
			grabh(hp, GTO|GSUBJECT|GCC|GBCC);
			printf((catgets(nl_fn,NL_SETN,5, "(continue)\n")));
			break;

		case 't':
			/*
			 * Add to the To list.
			 */

			hp->h_to = addto(hp->h_to, &linebuf[2]);
			hp->h_seq++;
			break;

		case 's':
			/*
			 * Set the Subject list.
			 */

			cp = &linebuf[2];
			while (any(*cp, " \t"))
				cp++;
			hp->h_subject = savestr(cp);
			hp->h_seq++;
			break;

/* following code added in to support reply-to field */
		case 'R':
			/*
			 * Set the Reply-To field.
			 */

			cp = &linebuf[2];
			while (any(*cp, " \t"))
				cp++;
			hp->h_replyto = savestr(cp);
			hp->h_seq++;
			break;

		case 'c':
			/*
			 * Add to the CC list.
			 */

			hp->h_cc = addto(hp->h_cc, &linebuf[2]);
			hp->h_seq++;
			break;

		case 'b':
			/*
			 * Add stuff to blind carbon copies list.
			 */
			hp->h_bcc = addto(hp->h_bcc, &linebuf[2]);
			hp->h_seq++;
			break;

		case 'd':
			copy(Getf("DEAD"), &linebuf[2]);
			/* fall into . . . */

		case '<':
		case 'r': {
			int	ispip;
			/*
			 * Invoke a file:
			 * Search for the file name,
			 * then open it and copy the contents to obuf.
			 *
			 * if name begins with '!', read from a command
			 */

			cp = &linebuf[2];
			while (any(*cp, " \t"))
				cp++;
			if (*cp == '\0') {
				printf((catgets(nl_fn,NL_SETN,6, "Interpolate what file?\n")));
				break;
			}
			if (*cp=='!') {
				/* take input from a command */
				ispip = 1;
				if ((fbuf = popen(++cp, "r"))==NULL) {
					perror("");
					break;
				}
			} else {
				ispip = 0;
				cp = expand(cp);
				if (cp == NOSTR)
					break;
				if (isdir(cp)) {
					printf((catgets(nl_fn,NL_SETN,7, "%s: directory\n")));
					break;
				}
				if ((fbuf = fopen(cp, "r")) == NULL) {
					perror(cp);
					break;
				}
			}
			printf("\"%s\" ", cp);
			flush();
			lc = 0;
			cc = 0;
			while (readline(fbuf, linebuf) > 0) {
				lc++;
				if ((t = putline(obuf, linebuf)) < 0) {
					if (ispip)
						pclose(fbuf);
					else
						fclose(fbuf);
					goto err;
				}
				cc += t;
			}
			if (ispip)
				pclose(fbuf);
			else
				fclose(fbuf);
			printf("%d/%d\n", lc, cc);
			break;
			}

		case 'w':
			/*
			 * Write the message on a file.
			 */

			cp = &linebuf[2];
			while (any(*cp, " \t"))
				cp++;
			if (*cp == '\0') {
				fprintf(stderr, (catgets(nl_fn,NL_SETN,8, "Write what file!?\n")));
				break;
			}
			if ((cp = expand(cp)) == NOSTR)
				break;
			fflush(obuf);
			rewind(ibuf);
			/* changed to add append to file rather than write */
			exwrite(cp, ibuf, 1, 1);
			break;

		case 'm':
		case 'f':
			/*
			 * Interpolate the named messages, if we
			 * are in receiving mail mode.  Does the
			 * standard list processing garbage.
			 * If ~f is given, we don't shift over.
			 */

			if (!rcvmode) {
				printf((catgets(nl_fn,NL_SETN,9, "No messages to send from!?!\n")));
				break;
			}
			cp = &linebuf[2];
			while (any(*cp, " \t"))
				cp++;
			if (forward(cp, obuf, c) < 0)
				goto err;
			printf((catgets(nl_fn,NL_SETN,10, "(continue)\n")));
			break;

		case '?':
#ifdef NLS
			if ((fbuf = fopen(NL_THELPFILE, "r")) == NULL &&
			    (fbuf = fopen(THELPFILE, "r")) == NULL) {
				printf((catgets(nl_fn,NL_SETN,11, "No help just now.\n")));
				break;
			}
#else
			if ((fbuf = fopen(THELPFILE, "r")) == NULL) {
				printf((catgets(nl_fn,NL_SETN,11, "No help just now.\n")));
				break;
			}
#endif
			t = getc(fbuf);
			while (t != -1) {
				putchar(t);
				t = getc(fbuf);
			}
			fclose(fbuf);
			break;

		case 'p':
			/*
			 * Print out the current state of the
			 * message without altering anything.
			 */

			fflush(obuf);
			rewind(ibuf);
			printf((catgets(nl_fn,NL_SETN,12, "-------\nMessage contains:\n")));
			puthead(hp, stdout, GTO|GSUBJECT|GCC|GBCC|GREPLYTO|GNL);
			while ((t = getc(ibuf))!=EOF)
				putchar(t);
			printf((catgets(nl_fn,NL_SETN,13, "(continue)\n")));
			break;

		case '^':
		case '|':
			/*
			 * Pipe message through command.
			 * Collect output as new message.
			 */

			obuf = mespipe(ibuf, obuf, &linebuf[2]);
			newo = obuf;
			ibuf = newi;
			newi = ibuf;
			printf((catgets(nl_fn,NL_SETN,14, "(continue)\n")));
			break;

		case 'v':
		case 'e':
			/*
			 * Edit the current message.
			 * 'e' means to use EDITOR
			 * 'v' means to use VISUAL
			 */

			if ((obuf = mesedit(ibuf, obuf, c)) == NULL)
				goto err;
			newo = obuf;
			ibuf = newi;
			printf((catgets(nl_fn,NL_SETN,15, "(continue)\n")));
			break;
			break;
		}
	}
eofl:
	fclose(obuf);
	rewind(ibuf);
	sigset(SIGINT, savesig);
	sigset(SIGHUP, savehup);
# ifdef VMUNIX
	sigset(SIGCONT, savecont);
	sigsetmask(0);
# endif VMUNIX
	noreset = 0;
	return(ibuf);

err:
	if (ibuf != NULL)
		fclose(ibuf);
	if (obuf != NULL)
		fclose(obuf);
	sigset(SIGINT, savesig);
	sigset(SIGHUP, savehup);
# ifdef VMUNIX
	sigset(SIGCONT, savecont);
	sigsetmask(0);
# endif VMUNIX
	noreset = 0;
	return(NULL);
}

/*
 * Write a file, ex-like if f set.
 */

exwrite(name, ibuf, f, appnd)
	char name[];
	FILE *ibuf;
	int f, appnd;
{
	register FILE *of;
	register int c;
	long cc;
	int lc;
	int itexists;
	struct stat junk;

	if (f) {
		printf("\"%s\" ", name);
		fflush(stdout);
	}
	itexists = 0;
	if (stat(name, &junk) >= 0 && (junk.st_mode & S_IFMT) == S_IFREG) {
		if (appnd)
			itexists = 1;
		else {
			if (!f) fprintf(stderr, "%s: ", name);
			fprintf(stderr, (catgets(nl_fn,NL_SETN,16, "File exists\n")), name);
			return(-1);
		}
	}
	if ((of = fopen(name, (appnd ? "a" : "w"))) == NULL) {
		perror("");
		return(-1);
	}
	lc = 0;
	cc = 0;
	while ((c = getc(ibuf)) != EOF) {
		cc++;
		if (c == '\n')
			lc++;
		putc(c, of);
		if (ferror(of)) {
			perror(name);
			fclose(of);
			return(-1);
		}
	}
	fclose(of);
	if (appnd && itexists ) printf((catgets(nl_fn,NL_SETN,17, " [appended] %d/%ld\n")), lc, cc); 
		else printf("%d/%ld\n", lc, cc); 
	fflush(stdout);
	return(0);
}

/*
 * Edit the message being collected on ibuf and obuf.
 * Write the message out onto some poorly-named temp file
 * and point an editor at it.
 *
 * On return, make the edit file the new temp file.
 */

FILE *
mesedit(ibuf, obuf, c)
	FILE *ibuf, *obuf;
{
	int pid, s;
	FILE *fbuf;
	register int t;
	int (*sig)(), (*scont)(), foonly();
	struct stat sbuf;
	register char *edit;
	char ecmd[BUFSIZ];

/*
 * This was ifdefed to fix DTS report DSDrs00150, "AT&T #861:
 * SIGINT is ignored when escaping to shell from vi in mailx".
 * Just define NOSIGINT and you too can see the bug!
 */ 
# ifdef NOSIGINT
	sig = sigset(SIGINT, SIG_IGN);
# endif NOSIGINT
# ifdef VMUNIX
	scont = sigset(SIGCONT, foonly);
# endif VMUNIX
	if (stat(tempEdit, &sbuf) >= 0) {
		printf((catgets(nl_fn,NL_SETN,18, "%s: file exists\n")), tempEdit);
		goto out;
	}
	close(creat(tempEdit, 0600));
	if ((fbuf = fopen(tempEdit, "w")) == NULL) {
		perror(tempEdit);
		goto out;
	}
	fflush(obuf);
	rewind(ibuf);
	t = getc(ibuf);
	while (t != EOF) {
		putc(t, fbuf);
		t = getc(ibuf);
	}
	fflush(fbuf);
	if (ferror(fbuf)) {
		perror(tempEdit);
		remove(tempEdit);
		goto fix;
	}
	fclose(fbuf);
	if ((edit = value(c == 'e' ? "EDITOR" : "VISUAL")) == NOSTR)
		edit = c == 'e' ? EDITOR : VISUAL;
	sprintf(ecmd, "exec %s %s", edit, tempEdit);
	if (system(ecmd) & 0377) {
		printf((catgets(nl_fn,NL_SETN,19, "Fatal error in \"%s\"\n")), edit);
		printf((catgets(nl_fn,NL_SETN,20, "Editor tmp file saved as %s\n")),tempEdit);
		/*remove(tempEdit);*/
		goto out;
	}

	/*
	 * Now switch to new file.
	 */

	if ((fbuf = fopen(tempEdit, "a")) == NULL) {
		perror(tempEdit);
		remove(tempEdit);
		goto out;
	}
	if ((ibuf = fopen(tempEdit, "r")) == NULL) {
		perror(tempEdit);
		fclose(fbuf);
		remove(tempEdit);
		goto out;
	}
	remove(tempEdit);
	fclose(obuf);
	fclose(newi);
	obuf = fbuf;
	goto out;
fix:
	perror(tempEdit);
out:
# ifdef VMUNIX
	sigset(SIGCONT, scont);
# endif VMUNIX
# ifdef NOSIGINT
	sigset(SIGINT, sig);
# endif NOSIGINT
	newi = ibuf;
	return(obuf);
}

/*
 * Currently, Berkeley virtual VAX/UNIX will not let you change the
 * disposition of SIGCONT, except to trap it somewhere new.
 * Hence, sigset(SIGCONT, foonly) is used to ignore continue signals.
 */
foonly() {}

/*
 * Pipe the message through the command.
 * Old message is on stdin of command;
 * New message collected from stdout.
 * Sh -c must return 0 to accept the new message.
 */

FILE *
mespipe(ibuf, obuf, cmd)
	FILE *ibuf, *obuf;
	char cmd[];
{
	register FILE *ni, *no;
	int pid, s;
	void (*savesig)();
	char *Shell;

	newi = ibuf;
	if ((no = fopen(tempEdit, "w")) == NULL) {
		perror(tempEdit);
		return(obuf);
	}
	if ((ni = fopen(tempEdit, "r")) == NULL) {
		perror(tempEdit);
		fclose(no);
		remove(tempEdit);
		return(obuf);
	}
	remove(tempEdit);
	savesig = sigset(SIGINT, SIG_IGN);
	fflush(obuf);
	rewind(ibuf);
	if ((Shell = value("SHELL")) == NULL || *Shell=='\0')
		Shell = SHELL;
	if ((pid = vfork()) == -1) {
		perror("fork");
		goto err;
	}
	if (pid == 0) {
		/*
		 * stdin = current message.
		 * stdout = new message.
		 */

		sigchild();
		close(0);
		dup(fileno(ibuf));
		close(1);
		dup(fileno(no));
		for (s = 4; s < 15; s++)
			close(s);
		execlp(Shell, Shell, "-c", cmd, 0);
		perror(Shell);
		_exit(1);
	}
	while (wait(&s) != pid)
		;
	if (s != 0 || pid == -1) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,21, "\"%s\" failed!?\n")), cmd);
		goto err;
	}
	if (fsize(ni) == 0) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,22, "No bytes from \"%s\" !?\n")), cmd);
		goto err;
	}

	/*
	 * Take new files.
	 */

	newi = ni;
	fclose(ibuf);
	fclose(obuf);
	sigset(SIGINT, savesig);
	return(no);

err:
	fclose(no);
	fclose(ni);
	sigset(SIGINT, savesig);
	return(obuf);
}

/*
 * Interpolate the named messages into the current
 * message, preceding each line with a tab.
 * Return a count of the number of characters now in
 * the message, or -1 if an error is encountered writing
 * the message temporary.  The flag argument is 'm' if we
 * should shift over and 'f' if not.
 */
extern long transmit();

forward(ms, obuf, f)
	char ms[];
	FILE *obuf;
{
	register int *msgvec, *ip;
	extern char tempMail[];

	msgvec = (int *) salloc((msgCount+1) * sizeof *msgvec);
	if (msgvec == (int *) NOSTR)
		return(0);
	if (getmsglist(ms, msgvec, 0) < 0)
		return(0);
	if (*msgvec == NULL) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == NULL) {
			printf((catgets(nl_fn,NL_SETN,23, "No appropriate messages\n")));
			return(0);
		}
		msgvec[1] = NULL;
	}
	printf((catgets(nl_fn,NL_SETN,24, "Interpolating:")));
	for (ip = msgvec; *ip != NULL; ip++) {
		touch(*ip);
		printf(" %d", *ip);
		if (f == 'm') {
			if (transmit(&message[*ip-1], obuf) < 0L) {
				perror(tempMail);
				return(-1);
			}
		} else
			if (send(&message[*ip-1], obuf, 0) < 0) {
				perror(tempMail);
				return(-1);
			}
	}
	printf("\n");
	return(0);
}

/*
 * Send message described by the passed pointer to the
 * passed output buffer.  Insert a tab in front of each
 * line.  Return a count of the characters sent, or -1
 * on error.
 */

	long
transmit(mailp, obuf)
	struct message *mailp;
	FILE *obuf;
{
	register struct message *mp;
	register int ch;
	register long c, n;
	int bol;
	FILE *ibuf;

	mp = mailp;
	ibuf = setinput(mp);
	c = mp->m_size;
	n = c;
	bol = 1;
	while (c-- > 0L) {
		if (bol) {
			bol = 0;
			putc('\t', obuf);
			n++;
			if (ferror(obuf)) {
				perror("/tmp");
				return(-1L);
			}
		}
		ch = getc(ibuf);
		if (ch == '\n')
			bol++;
		putc(ch, obuf);
		if (ferror(obuf)) {
			perror("/tmp");
			return(-1L);
		}
	}
	return(n);
}

#ifdef VMUNIX

/*
 * Print (continue) when continued after ^Z.
 */
collcont(s)
{

	printf((catgets(nl_fn,NL_SETN,25, "(continue)\n")));
	fflush(stdout);
}

#endif

/*
 * On interrupt, go here to save the partial
 * message on ~/dead.letter.
 * Then restore signals and execute the normal
 * signal routine.  We only come here if signals
 * were previously set anyway.
 */

# ifndef VMUNIX
collintsig()
{
	signal(SIGINT, SIG_IGN);
	collrub(SIGINT);
}

collhupsig()
{
	signal(SIGHUP, SIG_IGN);
	collrub(SIGHUP);
}
# endif VMUNIX

collrub(s)
{
	register FILE *dbuf;
	register int c;
	register char *deadletter = Getf("DEAD");

	if (s == SIGINT && hadintr == 0) {
		hadintr++;
		clrbuf(stdout);
		printf((catgets(nl_fn,NL_SETN,26, "\n(Interrupt -- one more to kill letter)\n")));
		longjmp(coljmp, 1);
	}
	fclose(newo);
	rewind(newi);
	if (s == SIGINT && value("save")==NOSTR || fsize(newi) == 0)
		goto done;
	if ((dbuf = fopen(deadletter, "w")) == NULL)
		goto done;
	chmod(deadletter, 0600);
	while ((c = getc(newi)) != EOF)
		putc(c, dbuf);
	fclose(dbuf);

done:
	fclose(newi);
	sigset(SIGINT, savesig);
	sigset(SIGHUP, savehup);
# ifdef VMUNIX
	sigset(SIGCONT, savecont);
# endif VMUNIX
	if (rcvmode) {
		if (s == SIGHUP)
			hangup(SIGHUP);
		else
			stop(s);
	}
	else
		exit(1);
}

/*
 * Acknowledge an interrupt signal from the tty by typing an @
 */

intack(s)
{
	
	puts("@");
	fflush(stdout);
	clearerr(stdin);
	signal(SIGINT, intack);    /* must reset or will get default on next */
				   /* interrupt from the user                */
	longjmp(coljmp,1);
}

# ifdef SIGTSTP
/*
 * Catch the signal when carry out ^Z
 */

colltstp()
{
	flush();

	signal(SIGTSTP, SIG_DFL);
	kill(0, SIGTSTP);

	/* mailx stops here by ^Z */ 

	signal(SIGTSTP, colltstp);
}

# endif

/*
 * Add a string to the end of a header entry field.
 */

char *
addto(hf, news)
	char hf[], news[];
{
	register char *cp, *cp2, *linebuf;

	if (hf == NOSTR)
		hf = "";
	if (*news == '\0')
		return(hf);
	linebuf = salloc(strlen(hf) + strlen(news) + 2);
	for (cp = hf; any(*cp, " \t"); cp++)
		;
	for (cp2 = linebuf; *cp;)
		*cp2++ = *cp++;
	*cp2++ = ' ';
	for (cp = news; any(*cp, " \t"); cp++)
		;
	while (*cp != '\0')
		*cp2++ = *cp++;
	*cp2 = '\0';
	return(linebuf);
}

cpout( str, ofd )

char *str;
FILE *ofd;
{
      register char *cp = str;

      while ( *cp ) {
	    if ( *cp == '\\' ) {
		  switch ( *(cp+1) ) {
			case 'n':
			      putc('\n',ofd);
			      cp++;
			      break;
			case 't':
			      putc('\t',ofd);
			      cp++;
			      break;
			default:
			      putc('\\',ofd);
		  }
	    }
	    else {
		  putc(*cp,ofd);
	    }
	    cp++;
      }
      putc('\n',ofd);
}

xhalt()
{
	fclose(newo);
	fclose(newi);
	sigset(SIGINT, savesig);
	sigset(SIGHUP, savehup);
	if (rcvmode)
		stop(0);
	exit(1);
}
