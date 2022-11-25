/* @(#) $Revision: 66.3 $ */      
#include "rcv.h"
#include <sys/stat.h>
#include <errno.h>

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 8	/* set number */
#endif NLS

static int freadintr;
static int had_NULL_char = 0;   /* greater-than-zero if we've seen
				 * a NULL character in mail file.
				 */

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * File I/O.
 */


/*
 * Set up the input pointers while copying the mail file into
 * /tmp.
 */

setptr(ibuf)
	FILE *ibuf;
{
	register int c;
	register char *cp, *cp2;
	register int count, l;
	register long s;
	off_t offset;
	char linebuf[LINESIZE];
	char wbuf[LINESIZE];
	int maybe, mestmp, flag, inhead;
	struct message this;
	extern char tempSet[];
	char fbuf[256];

	if ((mestmp = opentemp(tempSet)) < 0)
		exit(1);
	msgCount = 0;
	offset = 0;
	s = 0L;
	l = 0;
	maybe = 1;
	flag = MUSED|MNEW;

	if (fread(fbuf, 1, FRWRDLEN, ibuf) == FRWRDLEN &&
		strncmp(fbuf, FORWARD, FRWRDLEN) == 0) {
		fgets(fbuf, sizeof(fbuf), ibuf);
		printf((catgets(nl_fn,NL_SETN,1, "Your mail is being forwarded to %s\n")), fbuf);
		if(getc(ibuf) == EOF) 
			return;
		printf((catgets(nl_fn,NL_SETN,2, "and your mailbox contains extra stuff\n")));
	}
	fseek(ibuf, (long)0, 0);

	for (;;) {
		cp = linebuf;
		c = getc(ibuf);
		while (c != EOF && c != '\n') {
			if (c == 0) {
			    /* skip over NULL character: */
			    had_NULL_char++;
			    c = getc(ibuf); /* get next character */
			    continue;
			}
			if (cp - linebuf >= LINESIZE - 1) {
				ungetc(c, ibuf);
				*cp = 0;
				break;
			}
			*cp++ = c;
			c = getc(ibuf);
		}

		if ( had_NULL_char ) {
		    skip_message(had_NULL_char);
		    had_NULL_char = 0;
		}

		*cp = 0;
		if (cp == linebuf && c == EOF) {
			this.m_flag = flag;
			flag = MUSED|MNEW;
			this.m_offset = offsetof(offset);
			this.m_block = blockof(offset);
			this.m_size = s;
			this.m_lines = l;
			if (append(&this, mestmp)) {
				perror(tempSet);
				exit(1);
			}
			fclose(ibuf);
			makemessage(mestmp);
			close(mestmp);
			return;
		}
		count = cp - linebuf + 1;
		for (cp = linebuf; *cp;)
			putc(*cp++, otf);
		putc('\n', otf);
		if (ferror(otf)) {
			perror("/tmp");
			exit(1);
		}
		if (maybe && linebuf[0] == 'F' && ishead(linebuf)) {
			msgCount++;
			this.m_flag = flag;
			flag = MUSED|MNEW;
			inhead = 1;
			this.m_block = blockof(offset);
			this.m_offset = offsetof(offset);
			this.m_size = s;
			this.m_lines = l;
			s = 0L;
			l = 0;
			if (append(&this, mestmp)) {
				perror(tempSet);
				exit(1);
			}
		}
		if (linebuf[0] == 0)
			inhead = 0;
		if (inhead && index(linebuf, ':')) {
			cp = linebuf;
			cp2 = wbuf;
			while (isalpha(*cp))
				*cp2++ = *cp++;
			*cp2 = 0;
			if (icequal(wbuf, "status")) {
				cp = index(linebuf, ':');
				if (index(cp, 'R'))
					flag |= MREAD;
				if (index(cp, 'O'))
					flag &= ~MNEW;
				inhead = 0;
			}
		}
		offset += count;
		s += (long)count;
		l++;
		maybe = 0;
		if (linebuf[0] == 0)
			maybe = 1;
	}
}

/*
 * Drop the passed line onto the passed output buffer.
 * If a write error occurs, return -1, else the count of
 * characters written, including the newline.
 */

putline(obuf, linebuf)
	FILE *obuf;
	char *linebuf;
{
	register int c;

	c = strlen(linebuf);
	fputs(linebuf, obuf);
	putc('\n', obuf);
	if (ferror(obuf))
		return(-1);
	return(c+1);
}

/*
 * Read up a line from the specified input into the line
 * buffer.  Return the number of characters read.  Do not
 * include the newline at the end.
 */

readline(ibuf, linebuf)
	FILE *ibuf;
	char *linebuf;
{
	register char *cp;
	register int c;
	void (*istat) ();
	int readintr();
	extern void intack(), collintsig();

	/* added signal to catch SIGINT locally in this read routine
	 * should help "smooth" the expected response.
	 * 4/1/84 - ml
	 */
	if ((istat = signal(SIGINT, SIG_IGN)) != SIG_IGN) {
		signal(SIGINT, readintr);
	}

	/* however, reset if this particular handler is specified */
	if (istat == collintsig) signal(SIGINT, istat); 
	if (istat == intack    ) signal(SIGINT, istat); 

	do {
		freadintr = 0;
		clearerr(ibuf);
		c = getc(ibuf);
		for (cp = linebuf; c != '\n' && c != EOF; c = getc(ibuf)) {
			if (freadintr == 1) {
				signal(SIGINT, istat);
				return(0);
			}
			if (c == 0) {
			    /* skip over NULL character: */
			    had_NULL_char++;
			    continue;
			}
			if (cp - linebuf < LINESIZE-4)
				*cp++ = c;
		}
	} while (ferror(ibuf) && ibuf == stdin);

	if ( had_NULL_char ) {
	    skip_message(had_NULL_char);
	    had_NULL_char = 0;
	}

	signal(SIGINT, istat);
	*cp = 0;
	if (c == EOF && cp == linebuf)
		return(0);
	return(cp - linebuf + 1);
}

/*    
 * routine readintr grafted in to handle problems with SIGINTS in the
 * above routine
 */
readintr() {

	freadintr = 1;
	signal(SIGINT, readintr);
	return;
}

/*
 * Return a file buffer all ready to read up the
 * passed message pointer.
 */

FILE *
setinput(mp)
	register struct message *mp;
{
	off_t off;

	fflush(otf);
	off = mp->m_block;
	off <<= 9;
	off += mp->m_offset;
	if (fseek(itf, off, 0) < 0) {
		perror("fseek");
		panic((catgets(nl_fn,NL_SETN,3, "temporary file seek")));
	}
	return(itf);
}

/*
 * Take the data out of the passed ghost file and toss it into
 * a dynamically allocated message structure.
 */

makemessage(f)
{
	register struct message *m;
	register char *mp;
	register count;

	mp = calloc((unsigned) (msgCount + 1), sizeof *m);
	if (mp == NOSTR) {
		printf((catgets(nl_fn,NL_SETN,4, "Insufficient memory for %d messages\n")), msgCount);
		exit(1);
	}
	if (message != (struct message *) 0)
		cfree((char *) message);
	message = (struct message *) mp;
	dot = message;
	lseek(f, 0L, 0);
	if (read(f, mp, (msgCount+1) * sizeof *m) != (msgCount+1) * sizeof *m) {
		printf((catgets(nl_fn,NL_SETN,5, "Message ghost file read error\n")));
		exit(1);
	}
	for (m = &message[0]; m < &message[msgCount]; m++) {
		m->m_size = (m+1)->m_size;
		m->m_lines = (m+1)->m_lines;
		m->m_flag = (m+1)->m_flag;
	}
	message[msgCount].m_size = 0L;
	message[msgCount].m_lines = 0;
}

/*
 * Append the passed message descriptor onto the temp file.
 * If the write fails, return 1, else 0
 */

append(mp, f)
	struct message *mp;
{
	if (write(f, (char *) mp, sizeof *mp) != sizeof *mp)
		return(1);
	return(0);
}

/*
 * Delete a file, but only if the file is a plain file.
 */

remove(name)
	char name[];
{
	struct stat statb;
	extern int errno;

	if (stat(name, &statb) < 0)
		return(-1);
	if ((statb.st_mode & S_IFMT) != S_IFREG) {
		errno = EISDIR;
		return(-1);
	}
	return(unlink(name));
}

/*
 * Terminate an editing session by attempting to write out the user's
 * file from the temporary.  Save any new stuff appended to the file.
 */

edstop()
{
	register int gotcha, c;
	register struct message *mp;
	FILE *obuf, *ibuf, *readstat;
	struct stat statb;
	char tempname[30], *id;
	int (*sigs[3])();

	if (readonly)
		return;
	holdsigs();
	if (Tflag != NOSTR) {
		if ((readstat = fopen(Tflag, "w")) == NULL)
			Tflag = NOSTR;
	}
	for (mp = &message[0], gotcha = 0; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW) {
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & (MODIFY|MDELETED|MSTATUS))
			gotcha++;
		if (Tflag != NOSTR && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			if ((id = hfield("article-id", mp)) != NOSTR)
				fprintf(readstat, "%s\n", id);
		}
	}
	if (Tflag != NOSTR)
		fclose(readstat);
	if (!gotcha || Tflag != NOSTR)
		goto done;
	ibuf = NULL;
	if (stat(editfile, &statb) >= 0 && statb.st_size > mailsize) {
		strcpy(tempname, "/tmp/mboxXXXXXX");
		mktemp(tempname);
		if ((obuf = fopen(tempname, "w")) == NULL) {
			perror(tempname);
			relsesigs();
			reset(0);
		}
		if ((ibuf = fopen(editfile, "r")) == NULL) {
			perror(editfile);
			fclose(obuf);
			remove(tempname);
			relsesigs();
			reset(0);
		}
		fseek(ibuf, mailsize, 0);
		while ((c = getc(ibuf)) != EOF)
			putc(c, obuf);
		fclose(ibuf);
		fclose(obuf);
		if ((ibuf = fopen(tempname, "r")) == NULL) {
			perror(tempname);
			remove(tempname);
			relsesigs();
			reset(0);
		}
		remove(tempname);
	}
	printf("\"%s\" ", editfile);
	flush();
	if ((obuf = fopen(editfile, TRUNCMODE)) == NULL) {
		perror(editfile);
		relsesigs();
		reset(0);
	}
	trunc(obuf);
	c = 0;
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
		if ((mp->m_flag & MDELETED) != 0)
			continue;
		c++;
		if (send(mp, obuf, 0) < 0) {
			perror(editfile);
			relsesigs();
			reset(0);
		}
	}
	gotcha = (c == 0 && ibuf == NULL);
	if (ibuf != NULL) {
		while ((c = getc(ibuf)) != EOF)
			putc(c, obuf);
		fclose(ibuf);
	}
	fflush(obuf);
	if (ferror(obuf)) {
		perror(editfile);
		relsesigs();
		reset(0);
	}
	fclose(obuf);
	if (gotcha) {
		remove(editfile);
		mailsize = 0;
		msgCount = 0;
		printf((catgets(nl_fn,NL_SETN,6, "removed\n")));
	}
	else
		printf((catgets(nl_fn,NL_SETN,7, "complete\n")));
	flush();

done:
	relsesigs();
}

static int sigdepth = 0;                /* depth of holdsigs() */
static int sigmsk = 0;
/*
 * Hold signals SIGHUP - SIGQUIT.
 */
holdsigs()
{
	register int i;

	if (sigdepth++ == 0)
#ifdef VMUNIX
		sigmsk = sigblock(mask(SIGHUP)|mask(SIGINT)|mask(SIGQUIT));
#else VMUNIX
		for (i = SIGHUP; i <= SIGQUIT; i++)
			sighold(i);
#endif VMUNIX
}

/*
 * Release signals SIGHUP - SIGQUIT
 */
relsesigs()
{
	register int i;

	if (--sigdepth == 0)
#ifdef VMUNIX
		sigsetmask(sigmsk);
#else
		for (i = SIGHUP; i <= SIGQUIT; i++)
			sigrelse(i);
#endif VMUNIX
}

/*
 * Empty the output buffer.
 */

clrbuf(buf)
	register FILE *buf;
{

	buf = stdout;
	buf->_ptr = buf->_base;
	buf->_cnt = BUFSIZ;
}

/*
 * Open a temp file by creating, closing, unlinking, and
 * reopening.  Return the open file descriptor.
 */

opentemp(file)
	char file[];
{
	register int f;

	if ((f = creat(file, 0600)) < 0) {
		perror(file);
		return(-1);
	}
	close(f);
	if ((f = open(file, 2)) < 0) {
		perror(file);
		remove(file);
		return(-1);
	}
	remove(file);
	return(f);
}

/*
 * Flush the standard output.
 */

flush()
{
	fflush(stdout);
	fflush(stderr);
}

/*
 * Determine the size of the file possessed by
 * the passed buffer.
 */

off_t
fsize(iob)
	FILE *iob;
{
	register int f;
	struct stat sbuf;

	f = fileno(iob);
	if (fstat(f, &sbuf) < 0)
		return(0);
	return(sbuf.st_size);
}

/*
 * Take a file name, possibly with shell meta characters
 * in it and expand it by using "csh -c echo filename"
 * Return the file name as a dynamic string.
 */

char *
expand(name)
	char name[];
{
	register char *cp;
	register int pid, l, rc;
	char xname[BUFSIZ];
	char cmdbuf[BUFSIZ];
	char *Shell = "/bin/csh";
	char *option = "-cf";
	int t, s, pivec[2];
	void (*sigint)();
	struct stat sbuf;

	if (debug) fprintf(stderr, "expand(%s)=", name);
	if (name[0] == '+' && getfold(cmdbuf) >= 0) {
		sprintf(xname, "%s/%s", cmdbuf, name + 1);
		return(expand(savestr(xname)));
	}
	if (!anyof(name, "~{[*?$`'\"\\")) {
		if (debug) fprintf(stderr, "%s\n", name);
		return(name);
	}
	if (pipe(pivec) < 0) {
		perror("pipe");
		return(name);
	}
	sprintf(cmdbuf, "echo %s", name);

	if ((pid = fork()) == 0) {
		sigchild();
		close(pivec[0]);
		close(1);
		dup(pivec[1]);
		close(pivec[1]);
		close(2);
		execlp(Shell, Shell, option, cmdbuf, 0);
		_exit(1);
	}
	if (pid == -1) {
		perror("fork of shell failed");
		close(pivec[0]);
		close(pivec[1]);
		return(NOSTR);
	}
	close(pivec[1]);
	l = read(pivec[0], xname, BUFSIZ);
	close(pivec[0]);
	while (wait(&s) != pid);
		;
	s &= 0377;
	if (s != 0 && s != SIGPIPE) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,8, "\"Echo\" failed\n")));
		goto err;
	}
	if (l < 0) {
		perror("read");
		goto err;
	}
	if (l == 0) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,9, "\"%s\": No match\n")), name);
		goto err;
	}
	if (l == BUFSIZ) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,10, "Buffer overflow expanding \"%s\"\n")), name);
		goto err;
	}
	xname[l] = 0;
	for (cp = &xname[l-1]; *cp == '\n' && cp > xname; cp--)
		;
	*++cp = '\0';
	if (any(' ', xname) && stat(xname, &sbuf) < 0) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,11, "\"%s\": Ambiguous\n")), name);
		goto err;
	}
	if (debug)
		fprintf(stderr,"pipe read returned %d, [%s]\n", l, xname);
	return(savestr(xname));

err:
	printf("\n");
	return(NOSTR);
}

/*
 * Determine the current folder directory name.
 */
getfold(name)
	char *name;
{
	char *folder;

	if ((folder = value("folder")) == NOSTR)
		return(-1);
	if (*folder == '/')
		strcpy(name, folder);
	else
		sprintf(name, "%s/%s", homedir, folder);
	return(0);
}

/*
 * A nicer version of Fdopen, which allows us to fclose
 * without losing the open file.
 */

FILE *
Fdopen(fildes, mode)
	char *mode;
{
	register int f;
	FILE *fdopen();

	f = dup(fildes);
	if (f < 0) {
		perror("dup");
		return(NULL);
	}
	return(fdopen(f, mode));
}

/*
 * return the filename associated with "s".  This function always
 * returns a non-null string (no error checking is done on the receiving end)
 */
char *
Getf(s)
register char *s;
{
	register char *cp;
	static char defbuf[PATHSIZE];

	if ((cp = value(s)) && *cp) {
		return(cp);
	} else if (strcmp(s, "MBOX")==0) {
		strcpy(defbuf, Getf("HOME"));
		strcat(defbuf, "/");
		strcat(defbuf, "mbox");
		return(defbuf);
	} else if (strcmp(s, "DEAD")==0) {
		strcpy(defbuf, Getf("HOME"));
		strcat(defbuf, "/");
		strcat(defbuf, "dead.letter");
		return(defbuf);
	} else if (strcmp(s, "MAILRC")==0) {
		strcpy(defbuf, Getf("HOME"));
		strcat(defbuf, "/");
		strcat(defbuf, ".mailrc");
		return(defbuf);
	} else if (strcmp(s, "HOME")==0) {
		/* no recursion allowed! */
		return(".");
	}
	return("DEAD");	/* "cannot happen" */
}

    static
skip_message(num)
    int num;
{
    static char prbuf[512];         /* buffer for printing error messages */

    switch (num) {
	case 0:
	    return;

	case 1:
	    prs((catgets(nl_fn,NL_SETN,12, "mailx: deleted 1 NULL character in mail file\n")));
	    return;

	default:
	    sprintf(prbuf, (catgets(nl_fn,NL_SETN,13, "mailx: deleted %d NULL characters in mail file\n")), num);
	    prs(prbuf);
	    return;
    }
}
