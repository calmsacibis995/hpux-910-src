/* @(#) $Revision: 70.2 $ */   
#

/* The external declaration in <stdio.h> was changed from "_iob" to
   "__iob" for _NAMESPACE_CLEAN reasons.  
*/
#ifdef _NAMESPACE_CLEAN
#define _iob __iob
#endif

/*
 * Define scan_iops() to be ___stdio_unsup_1 which is its "real" name
 * in libc.a.  The name scan_iops() is just a more descriptive name to use.
 */
#define scan_iops ___stdio_unsup_1
#define ANY_BITS_IN_MASK	0

#include "rcv.h"
#include <sys/stat.h>

#ifndef NLS
#define catgets(i,mn,sn, s) (s)
#else NLS
#define NL_SETN 10	/* set number */
#include <nl_ctype.h>
#endif NLS

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Lexical processing of commands.
 */


/*
 * Set up editing on the given file name.
 * If isedit is true, we are considered to be editing the file,
 * otherwise we are reading our mail which has signficance for
 * mbox and so forth.
 */

setfile(name, isedit)
	char *name;
	int isedit;
{
	FILE *ibuf;
	int i;
	static int shudclob;
	static char efile[128];
	char fortest[128];
	extern char tempMesg[];
	struct stat stbuf;

	if (shudclob) {
		holdsigs();
		if (edit)
			edstop();
		else
			quit();
		relsesigs();
	}

	if (isedit) {
		if ((int)(ibuf = fopen(name, "r")) < 0)
			{
			perror(name);
			exit(ibuf);
			}
		}
	else
		if ((int)(ibuf = lock(name, "r", IN)) <= 0)
			{
			perror(name);
			exit(ibuf);
			}

	if ((ibuf == NULL) ||
	    (fstat(fileno(ibuf), &stbuf) < 0)) {
		if (exitflg)
			exit(1);	/* no mail, return error */
		if (isedit)
			perror(name);
		else if (!Hflag) {
			if (rindex(name,'/') == NOSTR)
				fprintf(stderr, (catgets(nl_fn,NL_SETN,1, "No mail.\n")));
			else
				fprintf(stderr, (catgets(nl_fn,NL_SETN,2, "No mail for %s\n")), rindex(name,'/')+1);
		}
		if (ibuf) {
			if (isedit)
				fclose(ibuf);
			else
				unlock(ibuf);
		}
		return(-1);
	}
	if (stbuf.st_size == 0L) {
		if (exitflg)
			exit(1);	/* no mail, return error */
		if (isedit)
			fprintf(stderr, (catgets(nl_fn,NL_SETN,3, "%s: empty file\n")), name);
		else if (!Hflag) {
			if (rindex(name,'/') == NOSTR)
				fprintf(stderr, (catgets(nl_fn,NL_SETN,4, "No mail.\n")));
			else
				fprintf(stderr, (catgets(nl_fn,NL_SETN,5, "No mail for %s\n")), rindex(name,'/')+1);
		if (isedit)
			fclose(ibuf);
		else
			unlock(ibuf);
		}
		return(-1);
	}

	fgets(fortest, sizeof fortest, ibuf);
	fseek(ibuf, 0L, 0);
	if (strncmp(fortest, "Forward to ", 11) == 0) {
		if (exitflg)
			exit(1);	/* no mail, return error */
		fprintf(stderr, (catgets(nl_fn,NL_SETN,6, "Your mail is being forwarded to %s")), fortest+11);
		if (isedit)
			fclose(ibuf);
		else
			unlock(ibuf);
		return (-1);
	}
	if (exitflg)
		exit(0);	/* there is mail, return success */

	/*
	 * Looks like all will be well. Must hold signals
	 * while we are reading the new file, else we will ruin
	 * the message[] data structure.
	 * Copy the messages into /tmp and set pointers.
	 */

	holdsigs();
	readonly = 0;
	if ((i = access(name, 2)) < 0)
		readonly++;
	if (shudclob) {
		fclose(itf);
		fclose(otf);
	}
	shudclob = 1;
	edit = isedit;
	strncpy(efile, name, 128);
	editfile = efile;
	if (name != mailname)
		strcpy(mailname, name);
	mailsize = fsize(ibuf);
	if ((otf = fopen(tempMesg, "w")) == NULL) {
		perror(tempMesg);
		exit(1);
	}
	if ((itf = fopen(tempMesg, "r")) == NULL) {
		perror(tempMesg);
		exit(1);
	}
	remove(tempMesg);
	setptr(ibuf);
	setmsize(msgCount);
	if (isedit)
		fclose(ibuf);
	else
		unlock(ibuf);
	relsesigs();
	sawcom = 0;
	return(0);
}

/*
 * Interpret user commands one by one.  If standard input is not a tty,
 * print no prompt.
 */

int	*msgvec;
int	shudprompt;

commands()
{
	int eofloop, stop(), rexit();
	register int n;
	char linebuf[LINESIZE];
	int hangup(), contin();

# ifdef VMUNIX
	sigset(SIGCONT, SIG_DFL);
# endif VMUNIX
	for (;;) {
		setexit();        /* sets point for longjmp */

		if (rcvmode && !sourcing) {
			if (sigset(SIGINT, SIG_IGN) != SIG_IGN)
				sigset(SIGINT, stop);
			if (sigset(SIGHUP, SIG_IGN) != SIG_IGN)
				sigset(SIGHUP, hangup);
			if (sigset(SIGQUIT, SIG_IGN) != SIG_IGN)
				sigset(SIGQUIT, rexit);
		}

		/*
		 * Print the prompt, if needed.  Clear out
		 * string space, and flush the output.
		 */

		if (!rcvmode && !sourcing)
			return;
		eofloop = 0;
top:
		if (shudprompt = (intty && !sourcing)) {
			if (prompt==NOSTR)
				prompt = "? ";
			printf("%s", prompt);
			flush();
# ifdef VMUNIX
			sigset(SIGCONT, contin);
# endif VMUNIX
		} else
			flush();
		sreset();

		/*
		 * Read a line of commands from the current input
		 * and handle end of file specially.
		 */

		n = 0;
		for (;;) {
			if (readline(input, &linebuf[n]) <= 0) {
				if (n != 0)
					break;
				if (loading)
					return;
				if (sourcing) {
					unstack();
					goto more;
				}
				if (value("ignoreeof") != NOSTR && shudprompt) {
					if (++eofloop < 25) {
						printf((catgets(nl_fn,NL_SETN,7, "Use \"quit\" to quit.\n")));
						goto top;
					}
				}
				if (edit)
					edstop();
				return;
			}
			if ((n = strlen(linebuf)) == 0)
				break;
			n--;
#ifdef NLS
			if (!FIRSTof2(linebuf[n-1])) {
#endif NLS
			if (linebuf[n] != '\\')
				break;
			linebuf[n++] = ' ';
#ifdef NLS
			}
			else
				break;
#endif NLS
		}
# ifdef VMUNIX
		sigset(SIGCONT, SIG_DFL);
# endif VMUNIX
		if (execute(linebuf, 0))
			return;
more:		;
	}
}

/*
 * Execute a single command.  If the command executed
 * is "quit," then return non-zero so that the caller
 * will know to return back to main, if he cares.
 * Contxt is non-zero if called while composing mail.
 */

execute(linebuf, contxt)
	char linebuf[];
{
	char word[LINESIZE];
	char *arglist[MAXARGC];
	struct cmd *com;
	register char *cp, *cp2;
	register int c;
	int muvec[2];
	int edstop(), e;

	/*
	 * Strip the white space away from the beginning
	 * of the command, then scan out a word, which
	 * consists of anything except digits and white space.
	 *
	 * Handle ! escapes differently to get the correct
	 * lexical conventions.
	 */

	cp = linebuf;
	while (any(*cp, " \t"))
		cp++;
	if (*cp == '!') {
		if (sourcing) {
			printf((catgets(nl_fn,NL_SETN,8, "Can't \"!\" while sourcing\n")));
			unstack();
			return(0);
		}
		shell(cp+1);
		return(0);
	}
	cp2 = word;
	while (*cp && !any(*cp, " \t0123456789$^.:/-+*'\""))
		/* '# ' OR '#' can now start comment */
		if((*cp2++ = *cp++) == '#') break;
	*cp2 = '\0';

	/*
	 * Look up the command; if not found, complain.
	 * Normally, a blank command would map to the
	 * first command in the table; while sourcing,
	 * however, we ignore blank lines to eliminate
	 * confusion.
	 */

	if (sourcing && equal(word, ""))
		return(0);
	com = lex(word);
	if (com == NONE) {
		if (sourcing) 
			printf((catgets(nl_fn,NL_SETN,9, "Unknown command in source: \"%s\"\n")), word);
                else	printf((catgets(nl_fn,NL_SETN,10, "Unknown command: \"%s\"\n")), word);
		if (loading)
			return(1);
		if (sourcing)
			unstack();
		return(0);
	}

	/*
	 * See if we should execute the command -- if a conditional
	 * we always execute it, otherwise, check the state of cond.
	 */

	if ((com->c_argtype & F) == 0)
		if (cond == CRCV && !rcvmode || cond == CSEND && rcvmode)
			return(0);

	/*
	 * Special case so that quit causes a return to
	 * main, who will call the quit code directly.
	 * If we are in a source file, just unstack.
	 */

	if (com->c_func == edstop && sourcing) {
		if (loading)
			return(1);
		unstack();
		return(0);
	}
	if (!edit && com->c_func == edstop) {
		sigset(SIGINT, SIG_IGN);
		return(1);
	}

	/*
	 * Process the arguments to the command, depending
	 * on the type he expects.  Default to an error.
	 * If we are sourcing an interactive command, it's
	 * an error.
	 */

	if (!rcvmode && (com->c_argtype & M) == 0) {
		printf((catgets(nl_fn,NL_SETN,11, "May not execute \"%s\" while sending\n")),
		    com->c_name);
		if (loading)
			return(1);
		if (sourcing)
			unstack();
		return(0);
	}
	if (sourcing && com->c_argtype & I) {
		printf((catgets(nl_fn,NL_SETN,12, "May not execute \"%s\" while sourcing\n")),
		    com->c_name);
		if (loading)
			return(1);
		unstack();
		return(0);
	}
	if (readonly && com->c_argtype & W) {
		printf((catgets(nl_fn,NL_SETN,13, "May not execute \"%s\" -- message file is read only\n")),
		   com->c_name);
		if (loading)
			return(1);
		if (sourcing)
			unstack();
		return(0);
	}
	if (contxt && com->c_argtype & R) {
		printf((catgets(nl_fn,NL_SETN,14, "Cannot recursively invoke \"%s\"\n")), com->c_name);
		return(0);
	}
	e = 1;
	switch (com->c_argtype & ~(F|P|I|M|T|W|R)) {
	case MSGLIST:
		/*
		 * A message list defaulting to nearest forward
		 * legal message.
		 */
		if (msgvec == 0) {
			printf((catgets(nl_fn,NL_SETN,15, "Illegal use of \"message list\"\n")));
			return(-1);
		}
		if ((c = getmsglist(cp, msgvec, com->c_msgflag)) < 0)
			break;
		if (c  == 0)
			if (msgCount == 0)
				*msgvec = NULL;
			else {
				*msgvec = first(com->c_msgflag,
					com->c_msgmask);
				msgvec[1] = NULL;
			}
		if (*msgvec == NULL) {
			printf((catgets(nl_fn,NL_SETN,16, "No applicable messages\n")));
			break;
		}
		e = (*com->c_func)(msgvec);
		break;

	case NDMLIST:
		/*
		 * A message list with no defaults, but no error
		 * if none exist.
		 */
		if (msgvec == 0) {
			printf((catgets(nl_fn,NL_SETN,17, "Illegal use of \"message list\"\n")));
			return(-1);
		}
		if (getmsglist(cp, msgvec, com->c_msgflag) < 0)
			break;
		e = (*com->c_func)(msgvec);
		break;

	case STRLIST:
		/*
		 * Just the straight string, with
		 * leading blanks removed.
		 */
		while (any(*cp, " \t"))
			cp++;
		e = (*com->c_func)(cp);
		break;

	case RAWLIST:
		/*
		 * A vector of strings, in shell style.
		 */
		if ((c = getrawlist(cp, com, arglist)) < 0)
			break;
		if (c < com->c_minargs) {
			printf((catgets(nl_fn,NL_SETN,18, "%s requires at least %d arg(s)\n")),
				com->c_name, com->c_minargs);
			break;
		}
		if (c > com->c_maxargs) {
			printf((catgets(nl_fn,NL_SETN,19, "%s takes no more than %d arg(s)\n")),
				com->c_name, com->c_maxargs);
			break;
		}
		e = (*com->c_func)(arglist);
		break;

	case NOLIST:
		/*
		 * Just the constant zero, for exiting,
		 * eg.
		 */
		e = (*com->c_func)(0);
		break;

	default:
		panic((catgets(nl_fn,NL_SETN,20, "Unknown argtype")));
	}

	/*
	 * Exit the current source file on
	 * error.
	 */

	if (e && loading)
		return(1);
	if (e && sourcing)
		unstack();
	if (com->c_func == edstop)
		return(1);
	if (value("autoprint") != NOSTR && com->c_argtype & P)
		if ((dot->m_flag & MDELETED) == 0) {
			muvec[0] = dot - &message[0] + 1;
			muvec[1] = 0;
			type(muvec);
		}
	if (!sourcing && (com->c_argtype & T) == 0)
		sawcom = 1;
	return(0);
}

#ifdef  VMUNIX

/*
 * When we wake up after ^Z, reprint the prompt.
 */
contin(s)
{
	if (shudprompt)
		printf("%s", prompt);
	fflush(stdout);
}
#endif

/*
 * Branch here on hangup signal and simulate quit.
 */
hangup()
{

	holdsigs();
	if (edit) {
		if (setexit())
			exit(0);
		edstop();
	}
	else
		quit();
	exit(0);
}

/*
 * Set the size of the message vector used to construct argument
 * lists to message list functions.
 */
 
setmsize(sz)
{

	if (msgvec != (int *) 0)
		cfree(msgvec);
	if ( sz < 1 )
		sz = 1; /* need at least one cell for terminating 0 */
	msgvec = (int *) calloc((unsigned) (sz + 1), sizeof *msgvec);
}

/*
 * Find the correct command in the command table corresponding
 * to the passed command "word"
 */

struct cmd *
lex(word)
	char word[];
{
	register struct cmd *cp;
	extern struct cmd cmdtab[];

	for (cp = &cmdtab[0]; cp->c_name != NOSTR; cp++)
		if (isprefix(word, cp->c_name))
			return(cp);
	return(NONE);
}

/*
 * Determine if as1 is a valid prefix of as2.
 * Return true if yep.
 */

isprefix(as1, as2)
	char *as1, *as2;
{
	register char *s1, *s2;

	s1 = as1;
	s2 = as2;
	while (*s1++ == *s2)
		if (*s2++ == '\0')
			return(1);
	return(*--s1 == '\0');
}

/*
 * The following gets called on receipt of a rubout.  This is
 * to abort printout of a command, mainly.
 * Dispatching here when command() is inactive crashes rcv.
 * Close all open files except 0, 1, 2, and the temporary.
 * The special call to getuserid() is needed so it won't get
 * annoyed about losing its open file.
 * Also, unstack all source files.
 */

int	inithdr;			/* am printing startup headers */

closefile(fp) /* this gets called from stop() by scan_iops() */
FILE *fp;
{
	if (fp == stdin || fp == stdout)
		return;
	if (fp == itf || fp == otf)
		return;
	if (fp == stderr)
		return;
	if (fp == pipef) {
		pclose(pipef);
		pipef = NULL;
		return;
	}
	fclose(fp);
}

stop(s)
int s;
{
	register FILE *fp;

# ifndef VMUNIX
	s = SIGINT;
# endif VMUNIX
	noreset = 0;
	if (!inithdr)
		sawcom++;
	inithdr = 0;
	while (sourcing)
		unstack();
	getuserid((char *) -1);

	(void)scan_iops(closefile,0xffff, ANY_BITS_IN_MASK); /* close files */

	if (image >= 0) {
		close(image);
		image = -1;
	}
	clrbuf(stdout);
	if (s) {
		printf((catgets(nl_fn,NL_SETN,21, "Interrupt\n")));
# ifndef VMUNIX
		signal(s, stop);
# endif
	}
	reset(0);
}

/*
 * Announce the presence of the current mailx version,
 * give the message count, and print a header listing.
 */

char    *greeting       = "mailx %s  Type ? for help.\n"; /* catgets 22 */
extern char *HPUX_ID;

announce()
{
	int vec[2], mdot;
	extern char *getrev();

	if (!Hflag && value("quiet")==NOSTR)
		printf((catgets(nl_fn,NL_SETN,22, greeting)), getrev());
	mdot = newfileinfo();
	vec[0] = mdot;
	vec[1] = 0;
	dot = &message[mdot - 1];
	if (msgCount > 0 && !noheader) {
		inithdr++;
		headers(vec);
		inithdr = 0;
	}
}

/*
 * Announce information about the file we are editing.
 * Return a likely place to set dot.
 */
newfileinfo()
{
	register struct message *mp;
	register int u, n, mdot, d, s;
	char fname[BUFSIZ], zname[BUFSIZ], *ename;

	if (Hflag)
		return(1);		/* fake it--return message 1 */
	for (mp = &message[0]; mp < &message[msgCount]; mp++)
		if (mp->m_flag & MNEW)
			break;
	if (mp >= &message[msgCount])
		for (mp = &message[0]; mp < &message[msgCount]; mp++)
			if ((mp->m_flag & MREAD) == 0)
				break;
	if (mp < &message[msgCount])
		mdot = mp - &message[0] + 1;
	else
		mdot = 1;
	s = d = 0;
	for (mp = &message[0], n = 0, u = 0; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW)
			n++;
		if ((mp->m_flag & MREAD) == 0)
			u++;
		if (mp->m_flag & MDELETED)
			d++;
		if (mp->m_flag & MSAVED)
			s++;
	}
	ename = mailname;
	if (getfold(fname) >= 0) {
		strcat(fname, "/");
		if (strncmp(fname, mailname, strlen(fname)) == 0) {
			sprintf(zname, "+%s", mailname + strlen(fname));
			ename = zname;
		}
	}
	printf("\"%s\": ", ename);
	if (msgCount == 1)
		printf((catgets(nl_fn,NL_SETN,23, "1 message")));
	else
		printf((catgets(nl_fn,NL_SETN,24, "%d messages")), msgCount);
	if (n > 0)
		printf((catgets(nl_fn,NL_SETN,25, " %d new")), n);
	if (u-n > 0)
		printf((catgets(nl_fn,NL_SETN,26, " %d unread")), u);
	if (d > 0)
		printf((catgets(nl_fn,NL_SETN,27, " %d deleted")), d);
	if (s > 0)
		printf((catgets(nl_fn,NL_SETN,28, " %d saved")), s);
	if (readonly)
		printf((catgets(nl_fn,NL_SETN,29, " [Read only]")));
	printf("\n");
	return(mdot);
}

strace() {}

/*
 * Print the current version number.
 */

pversion(e)
{
	extern char *getrev();

	printf("%s\n", getrev());
}

/*
 * Get revision string.
 */

char *
getrev()
{
char *p;
	/* Change all '$' characters to space (for better look) */
	for (; (p = strrchr(HPUX_ID,'$')) != NULL; *p = ' ')
		;
	if ((p = strchr(HPUX_ID,'R')) != NULL)
		return(p);
	else
		return(HPUX_ID);
}

/*
 * Load a file of user definitions.
 */
load(name)
	char *name;
{
	register FILE *in, *oldin;

	if ((in = fopen(name, "r")) == NULL)
		return;
	oldin = input;
	input = in;
	loading = 1;
	sourcing = 1;
	commands();
	loading = 0;
	sourcing = 0;
	input = oldin;
	fclose(in);
}
