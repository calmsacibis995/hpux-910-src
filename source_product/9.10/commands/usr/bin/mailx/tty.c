/* @(#) $Revision: 66.1 $ */   
/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Generally useful tty stuff.
 */


#include "rcv.h"

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 18	/* set number */
#endif NLS

extern int hadintr;

#ifdef	USG_TTY

#include <nl_ctype.h>

#define	CTRL(x)	(('x')&037)		/* control character */

static	int	c_erase;		/* Current erase char */
static	int	c_kill;			/* Current kill char */
static	int	c_intr;			/* interrupt char */
static	int	c_quit;			/* quit character */
static	int	c_word;			/* Current word erase char */
static	int	Col;			/* current output column */
static	int	Pcol;			/* end column of prompt string */
static  int     Fcol;			/* end column of flag buffer */
static	int	Out;			/* file descriptor of stdout */
static	struct termio savtty, ttybuf;
static	char canonb[BUFSIZ];		/* canonical buffer for input */
					/* processing */
static  int canonf[BUFSIZ];		/* canonical flag buffer */
static	int	erasing;		/* we are erasing characters */
static  int     escape = 0;             /* put in the escape character */
  
/*
 * Read all relevant header fields.
 */

grabh(hp, gflags)
register struct header *hp;
{
	void (*savesigs[2])();
	register int s;

	Out = fileno(stdout);
	if(ioctl(fileno(stdin), TCGETA, &savtty) < 0)
	{	perror("ioctl");
		return(-1);
	}
	c_erase = savtty.c_cc[VERASE];
	c_kill = savtty.c_cc[VKILL];
	c_intr = savtty.c_cc[VINTR];
	c_quit = savtty.c_cc[VQUIT];
	for (s = SIGINT; s <= SIGQUIT; s++)
		if ((savesigs[s-SIGINT] = sigset(s, SIG_IGN)) == SIG_DFL)
			sigset(s, SIG_DFL);
	if (gflags & GTO) {
		hp->h_to = readtty("To: ", hp->h_to);
		if (hp->h_to != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GSUBJECT) {
		hp->h_subject = readtty("Subject: ", hp->h_subject);
		if (hp->h_subject != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GCC) {
		hp->h_cc = readtty("Cc: ", hp->h_cc);
		if (hp->h_cc != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GBCC) {
		hp->h_bcc = readtty("Bcc: ", hp->h_bcc);
		if (hp->h_bcc != NOSTR)
			hp->h_seq++;
	}
	for (s = SIGINT; s <= SIGQUIT; s++)
		sigset(s, savesigs[s-SIGINT]);
	return(0);
}

/*
 * Read up a header from standard input.
 * The source string has the preliminary contents to
 * be read.
 *
 */


char *
readtty(pr, src)
	char pr[], src[];
{
	int c;
	register char *cp, *cp2;

	erasing = 0;
	c_word = CTRL(W);	/* erase word character */
	fflush(stdout);
	Col = 0;
	write(Out, pr, strlen(pr));
	Col = strlen(pr);
	Pcol = Col;
	Fcol = Col;
	if (src != NOSTR && strlen(src) > BUFSIZ - 1) {
		printf((catgets(nl_fn,NL_SETN,1, "too long to edit\n")));
		return(src);
	}
	if (setty(Out))
		return(src);
	cp2 = src==NOSTR ? "" : src;
	for (cp=canonb; *cp2; cp++, cp2++)
		*cp = *cp2;
	*cp = '\0';
	outstr(canonb);

	for (;;) {
#ifdef NLS
		c = getc(stdin);
#else
		c = getc(stdin) & 0177;
#endif NLS

		if (c==c_erase && !escape) {
			if (cp > canonb) {
#ifdef NLS
					if (canonf[Fcol-1] == 2) {
						rubout(--cp);
						rubout(--cp);
					} else
						rubout(--cp);
#else NLS
					rubout(--cp);
#endif NLS
				}
		} else if (c==c_kill && !escape) {
			write(Out, " \b", 2);
			while (cp > canonb) {
				rubout(--cp);
			}
		} else if (c==c_word) {
			if (cp > canonb) {
				while (--cp >= canonb)
					if (isalnum(*cp))
						break;
					else
						rubout(cp);
					while (cp >= canonb)
						if (isalnum(*cp))
							rubout(cp--);
						else
							break;
					if (cp < canonb)
						cp = canonb;
					else if (*cp)
						cp++;
				}
		} else if (c==EOF || ferror(stdin) || c==c_intr || c==c_quit) {
			resetty(Out);
			write(Out, "\n", 1);
			hadintr = 1;
			collrub(2);
		} else switch (c) {
			case '\n':
			case '\r':
				resetty(Out);
				write(Out, "\n", 1);
				if (canonb[0]=='\0')
					return(NOSTR);
				else
					return(savestr(canonb));
				break; /* not reached */
			case '\\':
				if (escape) {
					*cp++ = c;
					*cp = '\0';
					Echo(c);
					escape = 0;
				} else {
					write(Out, "\\\b", 2);
					escape = 1;
				}
 				break;           
			default:
				erasing = 1;
				escape = 0;
#ifdef NLS
				if (canonf[Fcol-1] == 1) {
					if (SECof2(c))
						canonf[Fcol++] = 2;
					else {
						canonf[Fcol-1] = 0;
						canonf[Fcol++] = 0;
					}
				} else {
					if (FIRSTof2(c))
						canonf[Fcol++] = 1;
					else 
						canonf[Fcol++] = 0;
				}
#endif NLS
				if (Fcol == BUFSIZ-1 && canonf[Fcol-1] == 1) {
					putchar((char) 007);
					Fcol--;
					break;
				}
					
				if (Fcol > BUFSIZ-1) {
					putchar((char) 007);	/* BEEP ! */
					Fcol--;
					break;
				}

				*cp++ = c;
				*cp = '\0';
				Echo(c);
		}
	}
}

setty(f)
{
	if (ioctl(f, TCGETA, &savtty) < 0) {
		perror("ioctl");
		return(-1);
	}
	ttybuf = savtty;
#ifdef	u370
	ttybuf.c_cflag &= ~PARENB;	/* disable parity */
	ttybuf.c_cflag |= CS8;		/* character size = 8 */
#endif	u370
	ttybuf.c_cc[VMIN] = 01;
	ttybuf.c_cc[VINTR] = 0;
	ttybuf.c_cc[VQUIT] = 0;
	ttybuf.c_lflag &= ~(ICANON|ECHO);
	if (ioctl(f, TCSETA, &ttybuf) < 0) {
		perror("ioctl");
		return(-1);
	}
	return(0);
}

resetty(f)
{
	if (ioctl(f, TCSETA, &savtty) < 0)
		perror("ioctl");
}

outstr(s)
register char *s;
{
	Fcol = 0;
	while (*s)
		Echo(*s++);
}

rubout(cp)
register char *cp;
{
	register int oldcol;
	register int c = *cp;

	*cp = '\0';
	switch (c) {
	case '\t':
		oldcol = countcol();
		do
			write(Out, "\b", 1);
		while (--Col > oldcol);
		Fcol-- ;
		break;
	case '\b':
#ifdef NLS
		if (canonf[Fcol-2] == 2) {
			write(Out, "\b", 1);
			write(Out, cp-2, 2);
			Col++;
			Fcol--;
			break;
		}
#endif NLS 
		if (isprint(cp[-1]))
			write(Out, cp-1, 1);
		else
			write(Out, " ", 1);
		Col++;
		canonf[Fcol++] = 0;
		break;
	default:
#ifdef NLS
		if (isprint(c) || FIRSTof2(c) || SECof2(c)) {
			Fcol--;
#else NLS
		if (isprint(c)) {
#endif NLS
			write(Out, "\b \b", 3);
			Col--;
		}
	}
}

countcol()
{
	register int col;
	register char *s;

	for (col=Pcol, s=canonb; *s; s++)
		switch (*s) {
		case '\t':
			while (++col % 8)
				;
			break;
		case '\b':
			col--;
			break;
		default:
#ifdef NLS
			if (isprint(*s) || FIRSTof2(*s) || SECof2(*s)) 
#else NLS
			if (isprint(*s))
#endif NLS
				col++;
		}
	return(col);
}

Echo(cc)
{
#ifdef NLS
	unsigned char c = cc;
#else not NLS
	char c = cc;
#endif NLS

	switch (c) {
	case '\t':
		do
			write(Out, " ", 1);
		while (++Col % 8);
		if (!erasing) 
			canonf[Fcol++] = 0;
		break;
	case '\b':
		if (Col > 0) {
			write(Out, " \b\b", 3);
			Col--;
		}
		break;
	case '\r':
	case '\n':
		Col = 0;
		write(Out, "\r\n", 2);
		break;
	default:
#ifdef NLS
		if (!erasing) {
			if (canonf[Fcol-1] == 1) {
				if (SECof2(c))
					canonf[Fcol++] = 2;
				else {
					canonf[Fcol-1] = 0;
					canonf[Fcol++] = 0;
				}
			} else {
				if (FIRSTof2(c))
					canonf[Fcol++] = 1;
				else 
					canonf[Fcol++] = 0;
			}
		} else
			erasing = 0;

		if (isprint(c) || FIRSTof2(c) || SECof2(c)) {
#else NLS
		if (isprint(c)) {
#endif NLS
			Col++;
			write(Out, &c, 1);
		}
	}
}
#else

static	int	c_erase;		/* Current erase char */
static	int	c_kill;			/* Current kill char */
static	int	hadcont;		/* Saw continue signal */
static	jmp_buf	rewrite;		/* Place to go when continued */
#ifndef TIOCSTI
static	int	ttyset;			/* We must now do erase/kill */
#endif

/*
 * Read all relevant header fields.
 */

grabh(hp, gflags)
	struct header *hp;
{
	struct sgttyb ttybuf;
	int ttycont(), signull();
#ifndef TIOCSTI
	int (*savesigs[2])();
#endif
	int (*savecont)();
	register int s;
	int errs;

# ifdef VMUNIX
	savecont = sigset(SIGCONT, signull);
# endif VMUNIX
	errs = 0;
#ifndef TIOCSTI
	ttyset = 0;
#endif
	if (gtty(fileno(stdin), &ttybuf) < 0) {
		perror("gtty");
		return(-1);
	}
	c_erase = ttybuf.sg_erase;
	c_kill = ttybuf.sg_kill;
#ifndef TIOCSTI
	ttybuf.sg_erase = 0;
	ttybuf.sg_kill = 0;
	for (s = SIGINT; s <= SIGQUIT; s++)
		if ((savesigs[s-SIGINT] = sigset(s, SIG_IGN)) == SIG_DFL)
			sigset(s, SIG_DFL);
#endif
	if (gflags & GTO) {
#ifndef TIOCSTI
		if (!ttyset && hp->h_to != NOSTR)
			ttyset++, stty(fileno(stdin), &ttybuf);
#endif
		hp->h_to = readtty("To: ", hp->h_to);
		if (hp->h_to != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GSUBJECT) {
#ifndef TIOCSTI
		if (!ttyset && hp->h_subject != NOSTR)
			ttyset++, stty(fileno(stdin), &ttybuf);
#endif
		hp->h_subject = readtty("Subject: ", hp->h_subject);
		if (hp->h_subject != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GCC) {
#ifndef TIOCSTI
		if (!ttyset && hp->h_cc != NOSTR)
			ttyset++, stty(fileno(stdin), &ttybuf);
#endif
		hp->h_cc = readtty("Cc: ", hp->h_cc);
		if (hp->h_cc != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GBCC) {
#ifndef TIOCSTI
		if (!ttyset && hp->h_bcc != NOSTR)
			ttyset++, stty(fileno(stdin), &ttybuf);
#endif
		hp->h_bcc = readtty("Bcc: ", hp->h_bcc);
		if (hp->h_bcc != NOSTR)
			hp->h_seq++;
	}
# ifdef VMUNIX
	sigset(SIGCONT, savecont);
# endif VMUNIX
#ifndef TIOCSTI
	ttybuf.sg_erase = c_erase;
	ttybuf.sg_kill = c_kill;
	if (ttyset)
		stty(fileno(stdin), &ttybuf);
	for (s = SIGINT; s <= SIGQUIT; s++)
		sigset(s, savesigs[s-SIGINT]);
#endif
	return(errs);
}

/*
 * Read up a header from standard input.
 * The source string has the preliminary contents to
 * be read.
 *
 */

char *
readtty(pr, src)
	char pr[], src[];
{
	char canonb[BUFSIZ];
	int c, ch, signull();
	register char *cp, *cp2;

	fputs(pr, stdout);
	fflush(stdout);
	if (src != NOSTR && strlen(src) > BUFSIZ - 2) {
		printf((catgets(nl_fn,NL_SETN,3, "too long to edit\n")));
		return(src);
	}
#ifndef TIOCSTI
	if (src != NOSTR)
		cp = copy(src, canonb);
	else
		cp = copy("", canonb);
	fputs(canonb, stdout);
	fflush(stdout);
#else
	cp = src == NOSTR ? "" : src;
	while (c = *cp++) {
		if (c == c_erase || c == c_kill) {
			ch = '\\';
			ioctl(0, TIOCSTI, &ch);
		}
		ioctl(0, TIOCSTI, &c);
	}
	cp = canonb;
	*cp = 0;
#endif
	cp2 = cp;
	while (cp2 < canonb + BUFSIZ)
		*cp2++ = 0;
	cp2 = cp;
	if (setjmp(rewrite))
		goto redo;
# ifdef VMUNIX
	sigset(SIGCONT, ttycont);
# endif VMUNIX
	clearerr(stdin);
	while (cp2 < canonb + BUFSIZ) {
		c = getc(stdin);
		if (c == EOF || c == '\n')
			break;
		*cp2++ = c;
	}
	*cp2 = 0;
# ifdef VMUNIX
	sigset(SIGCONT, signull);
# endif VMUNIX
	if (c == EOF && ferror(stdin) && hadcont) {
redo:
		hadcont = 0;
		cp = strlen(canonb) > 0 ? canonb : NOSTR;
		clearerr(stdin);
		return(readtty(pr, cp));
	}
#ifndef TIOCSTI
	if (cp == NOSTR || *cp == '\0')
		return(src);
	cp2 = cp;
	if (!ttyset)
		return(strlen(canonb) > 0 ? savestr(canonb) : NOSTR);
	while (*cp != '\0') {
		c = *cp++;
		if (c == c_erase) {
			if (cp2 == canonb)
				continue;
			if (cp2[-1] == '\\') {
				cp2[-1] = c;
				continue;
			}
			cp2--;
			continue;
		}
		if (c == c_kill) {
			if (cp2 == canonb)
				continue;
			if (cp2[-1] == '\\') {
				cp2[-1] = c;
				continue;
			}
			cp2 = canonb;
			continue;
		}
		*cp2++ = c;
	}
	*cp2 = '\0';
#endif
	if (equal("", canonb))
		return(NOSTR);
	return(savestr(canonb));
}

# ifdef VMUNIX
/*
 * Receipt continuation.
 */
ttycont(s)
{

	hadcont++;
	longjmp(rewrite, 1);
}
# endif VMUNIX

/*
 * Null routine to satisfy
 * silly system bug that denies us holding SIGCONT
 */
signull(s)
{}
#endif	USG_TTY
