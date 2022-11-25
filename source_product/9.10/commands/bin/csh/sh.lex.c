/* @(#) $Revision: 72.3 $ */    
/************************************************************************
 * These lexical routines read input and form lists of words.
 * There is some involved processing here, because of the complications
 * of input buffering, and especially because of history substitution.
 * This file has newbgetc to support tenex.
 *************************************************************************/

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 10	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

#ifdef NLS16
#include <nl_ctype.h>
#endif
#include "sh.h"

#ifdef NLS
extern int _nl_space_alt;
#define	ALT_SP	(_nl_space_alt & TRIM)
#endif

/*  Csh doesn't normally use stdio.h.
*/
#ifdef DEBUG_FILE_LEX
#include <stdio.h>
FILE *debugStream;
#endif

CHAR	*word();

/*
 * Peekc is a peek characer for getC, peekread for readc.
 * There is a subtlety here in many places... history routines
 * will read ahead and then insert stuff into the input stream.
 * If they push back a character then they must push it behind
 * the text substituted by the history substitution.  On the other
 * hand in several places we need 2 peek characters.  To make this
 * all work, the history routines read with getC, and make use both
 * of ungetC and unreadc.  The key observation is that the state
 * of getC at the call of a history reference is such that calls
 * to getC from the history routines will always yield calls of
 * readc, unless this peeking is involved.  That is to say that during
 * getexcl the variables lap, exclp, and exclnxt are all zero.
 *
 * Getdol invokes history substitution, hence the extra peek, peekd,
 * which it can ungetD to be before history substitutions.
 */
CHAR	peekc, peekd;
CHAR	peekread;

CHAR	*exclp;			/* (Tail of) current word from ! subst */
struct	wordent *exclnxt;	/* The rest of the ! subst words */
int	exclc;			/* Count of remainig words in ! subst */
CHAR	*alvecp;		/* "Globp" for alias resubstitution */

int	halfkj = 0;

/*
 * Lex returns to its caller not only a wordlist (as a "var" parameter)
 * but also whether a history substitution occurred.  This is used in
 * the main (process) routine to determine whether to echo, and also
 * when called by the alias routine to determine whether to keep the
 * argument list.
 */
bool	hadhist;

#define	ungetC(c)	peekc = c
#define	ungetD(c)	peekd = c

/*  Called by:
	process ()
	backeval ()
	asyn3 ()
*/
/**********************************************************************/
lex(hp)
	register struct wordent *hp;
/**********************************************************************/
{
	register struct wordent *wdp;
	int c;

/*  Since lex can be called from a child whose stdin and stdout have been
    redirected to a pipe, and all other descriptors closed, the only safe
    thing to do with output is to open another file and write to it.
    This situation occurs, for example, when backquote evaluation happens.
    Since the parent closed all file descriptors, this routine has to 
    re-open the debug file.  This is done for appending so that nothing 
    gets overwritten.
*/
#if defined (DEBUG_FILE_LEX) || defined (DEBUG_READC) || defined (DEBUG_BGETC )
  fclose (debugStream);
  debugStream = fopen ("debugLex", "a");
#endif

	lineloc = btell();
	hp->next = hp->prev = hp;
	hp->word = nullstr;
	alvecp = 0, hadhist = 0;
	do
		c = readc(0);
#ifndef NLS
	while (c == ' ' || c == '\t');
#else
	while (c == ' ' || c == '\t' || c == ALT_SP);
#endif
	if (c == HISTSUB && intty)
	  {
#ifdef DEBUG_LEX_HIST
  fprintf (debugStream, "lex (1): %d, Calling getexcl()\n", getpid ());
  fflush (debugStream);
#endif
		/* ^lef^rit	from tty is short !:s^lef^rit */
		getexcl(c);
	  }
	else
		unreadc(c);
	wdp = hp;
	/*
	 * The following loop is written so that the links needed
	 * by freelex will be ready and rarin to go even if it is
	 * interrupted.
	 */
	do {
		register struct wordent *new = (struct wordent *) calloc(1, sizeof *wdp);

		new->prev = wdp;
		new->next = hp;
		wdp->next = new;
		wdp = new;
		wdp->word = word();

#ifdef DEBUG_FILE_LEX
  if (debugStream != 0)
    {
      fprintf (debugStream, "lex (2): %d, word: %s\n", getpid (), 
	       to_char (wdp -> word));
      fflush (debugStream);
    }
#endif

	} while (wdp->word[0] != '\n');
	hp->prev = wdp;
	return (hadhist);
}

/*  Called by:
	process ()
	phist ()
*/
/**********************************************************************/
prlex(sp0)
	struct wordent *sp0;
/**********************************************************************/
{
	register struct wordent *sp = sp0->next;

	for (;;) {
		printf("%s", to_char(sp->word));
		sp = sp->next;
		if (sp == sp0)
			break;
		printf(" ");
	}
}

/*  Called by:
	enthist ()
*/
/**********************************************************************/
copylex(hp, fp)
	register struct wordent *hp;
	struct wordent *fp;
/**********************************************************************/
{
	register struct wordent *wdp;

	wdp = hp;
	fp = fp->next;
	do {
		register struct wordent *new = (struct wordent *) calloc(1, sizeof *wdp);

		new->prev = wdp;
		new->next = hp;
		wdp->next = new;
		wdp = new;
		wdp->word = savestr(fp->word);
		fp = fp->next;
	} while (wdp->word[0] != '\n');
	hp->prev = wdp;
}

/*  Called by:
	process ()
	evalav ()
	hfree ()
	asyn3 ()
*/
/**********************************************************************/
freelex(vp)
	register struct wordent *vp;
/**********************************************************************/
{
	register struct wordent *fp;

	while (vp->next != vp) {
		fp = vp->next;
		vp->next = fp->next;
		xfree(fp->word);
		xfree((CHAR *)fp);
	}
	vp->prev = vp;
}

char	*WORDMETA =	"# '`\"\t;&<>()|\n";

int haveread; 

/*  Called by:
	lex ()
*/
/**********************************************************************/
CHAR *
word()
/**********************************************************************/
{
	register CHAR c, c1;
	register CHAR *wp;
	CHAR wbuf[WRDSIZ];
	register bool dolflg;
	register int i;

	wp = wbuf, haveread = 0;
	i = WRDSIZ - 4;
loop:
	c = getC(DOALL);
#ifdef NLS
	if (c == ALT_SP)
		goto loop;
#endif

#ifdef DEBUG_FILE_LEX
  fprintf (debugStream, "word (1): %d, character: %lo\n", getpid (), (long ) c);
  fflush (debugStream);
#endif

	switch (c) {

	case ' ':
	case '\t':
		goto loop;

	case '`':
	case '\'':
	case '"':
		*wp++ = c, --i, c1 = c, haveread++;
		dolflg = c == '"' ? DOALL : DOEXCL;

		for (;;) {
			c = getC(dolflg);

			if (c == c1)
				break;
			if (c == '\n') {
				seterrc((catgets(nlmsg_fd,NL_SETN,1, "Unmatched ")), c1);
				ungetC(c);
				goto ret;
			}
			if (c == '\\') {
				c = getC(0);
				if (c == HIST)
					c |= QUOTE;
				else {
/*  This check quoted everything except a newline that occurred in backquotes.

					if (c == '\n' && c1 != '`')

    In response to a defect reported against csh where it wouldn't allow a
    backslashed newline in backquotes this has been changed to allow a quoted
    newline in all cases.
*/
					if (c == '\n')
						c |= QUOTE;
					ungetC(c), c = '\\';
				}
			}
			if (--i <= 0)
				goto toochars;
			*wp++ = c, haveread++;
		}
		*wp++ = c, --i, haveread++;
		goto pack;

	case '&':
	case '|':
	case '<':
	case '>':
		*wp++ = c, haveread++;
		c1 = getC(DOALL);
		if (c1 == c)
			*wp++ = c1;
		else
			ungetC(c1);
		goto ret;

	case '#':
		if (intty)
			break;
		if (wp != wbuf) {
			ungetC(c);
			goto ret;
		}
		c = 0;
		do {
			c1 = c;
			c = getC(0);
		} while (c != '\n');
		if (c1 == '\\')
			goto loop;
		/* fall into ... */

	case ';':
	case '(':
	case ')':
	case '\n':
		*wp++ = c, haveread++;
		goto ret;

	case '\\':
		c = getC(0);
		if (c == '\n') {
			if (onelflg == 1)
				onelflg = 2;
			goto loop;
		}
		if (c != HIST)
			*wp++ = '\\', --i, haveread++;
		c |= QUOTE;
		break;
	}
	ungetC(c);
pack:
	for (;;) {
		c = getC(DOALL);
		if (c == '\\') {
			c = getC(0);
			if (c == '\n') {
				if (onelflg == 1)
					onelflg = 2;
				goto ret;
			}
			if (c != HIST)
				*wp++ = '\\', --i, haveread++;
			c |= QUOTE;
		}
#ifndef NLS
		if (any(c, WORDMETA + intty)) {
#else
		if (any(c, WORDMETA + intty) || c == ALT_SP) {
#endif
			ungetC(c);
			if (any(c, "\"'`"))
				goto loop;
			goto ret;
		}
		if (--i <= 0)
			goto toochars;
		*wp++ = c, haveread++;
	}
toochars:
	seterr((catgets(nlmsg_fd,NL_SETN,2, "Word too long")));
	wp = &wbuf[1];
ret:
	*wp = 0, haveread = 0;
	return (savestr(wbuf));
}

/*  Called by:
	word ()
	getdol ()
	getexcl ()
	getsub ()
	getsel ()
	gethent ()
*/
/**********************************************************************/
getC(flag)
	register int flag;
/**********************************************************************/
{
	register CHAR c;

top:
	if (c = peekc) {
		peekc = 0;
		return (c);
	}
	if (lap) {
		c = *lap++;
		if (c == 0) {
			lap = 0;
			goto top;
		}
#ifndef NLS
		if (any(c, WORDMETA + intty))
		  {
#else
		if (any(c, WORDMETA + intty) || c == ALT_SP)
		  {
#endif
			c |= QUOTE;
		  }
		return (c);
	}
	if (c = peekd) {
		peekd = 0;
		return (c);
	}
	if (exclp) {
		if (c = *exclp++)
			return (c);
		if (exclnxt && --exclc >= 0) {
			exclnxt = exclnxt->next;
			setexclp(exclnxt->word);
			return (' ');
		}
		exclp = 0;
		exclnxt = 0;
	}
	if (exclnxt) {
		exclnxt = exclnxt->next;
		if (--exclc < 0)
			exclnxt = 0;
		else
			setexclp(exclnxt->word);
		goto top;
	}
	c = readc(0);

#ifdef DEBUG_FILE_LEX
  fprintf (debugStream, "getC (1): %d, character: %lo\n", getpid (), (long ) c);
  fflush (debugStream);
#endif
	if (c == '$' && (flag & DODOL)) {
		getdol();
		goto top;
	}
	if (c == HIST && (flag & DOEXCL)) {

#ifdef DEBUG_LEX_HIST
  fprintf (debugStream, "getC (1): %d, Calling getexcl(0)\n", getpid ());
  fflush (debugStream);
#endif
		getexcl(0);
		goto top;
	}
	return (c);
}

/*  Called by:
	getc ()
*/
/**********************************************************************/
getdol()
/**********************************************************************/
{
	register CHAR *np;
	CHAR name[164];
	
	/* changed from	CHAR name[40];   DSDe408290, DSDe414890 */
	
	register int c;
	int sc;
	bool special = 0;

	np = name, *np++ = '$';
	c = sc = getC(DOEXCL);
#ifndef NLS
	if (any(c, "\t \n")) {
#else
	if (any(c, "\t \n") || c == ALT_SP) {
#endif
		ungetD(c);
		ungetC('$' | QUOTE);
		return;
	}
	if (c == '{')
		*np++ = c, c = getC(DOEXCL);
	if (c == '#' || c == '?')
		special++, *np++ = c, c = getC(DOEXCL);
	*np++ = c;
	switch (c) {
	
	case '<':
	case '$':
		if (special)
			goto vsyn;
		goto ret;

	case '\n':
		ungetD(c);
		np--;
		goto vsyn;

	case '*':
		if (special)
			goto vsyn;
		goto ret;

	default:
		if (digit(c)) {
/*
 * let $?0 pass for now
			if (special)
				goto vsyn;
*/
			while (digit(c = getC(DOEXCL))) {
				if (np < &name[sizeof name / sizeof (CHAR) / 2])
					*np++ = c;
			}
		} else if (letter(c))
			while (alnum(c = getC(DOEXCL))) {
				if (np < &name[sizeof name / sizeof (CHAR) / 2])
					*np++ = c;
			}
		else
			goto vsyn;
	}
	if (c == '[') {
		*np++ = c;
		do {
			c = getC(DOEXCL);
			if (c == '\n') {
				ungetD(c);
				np--;
				goto vsyn;
			}
			if (np >= &name[sizeof name / sizeof (CHAR) - 8])
				goto vsyn;
			*np++ = c;
		} while (c != ']');
		c = getC(DOEXCL);
	}
	if (c == ':') {
		*np++ = c, c = getC(DOEXCL);
		if (c == 'g')
			*np++ = c, c = getC(DOEXCL);
		*np++ = c;
		if (!any(c, "htrqxe"))
			goto vsyn;
	} else
		ungetD(c);
	if (sc == '{') {
		c = getC(DOEXCL);
		if (c != '}') {
			ungetC(c);
			goto vsyn;
		}
		*np++ = c;
	}
ret:
	*np = 0;
	addla(name);
	return;

vsyn:
	seterr((catgets(nlmsg_fd,NL_SETN,3, "Variable syntax")));
	goto ret;
}

/*  Called by:
	Dgetdol ()
	getDolp ()
	getdol ()
*/
/**********************************************************************/
addla(cp)
	CHAR *cp;
/**********************************************************************/
{
	CHAR buf[WRDSIZ];

	if (lap != 0 && Strlen(cp) + Strlen(lap) >= sizeof (labuf) / sizeof (CHAR) - 4) {
		seterr((catgets(nlmsg_fd,NL_SETN,4, "Expansion buf ovflo")));
		return;
	}
	if (lap)
		Strcpy(buf, lap);
	Strcpy(labuf, cp);
	if (lap)
		Strcat(labuf, buf);
	lap = labuf;
}

CHAR	lhsb[32];
CHAR	slhs[32];
CHAR	rhsb[64];
int	quesarg;

/*  Called by:
	lex ()
	getC ()
*/
/**********************************************************************/
getexcl(sc)
	CHAR sc;
/**********************************************************************/
{
	register struct wordent *hp, *ip;
	int left, right, dol;
	register int c;

#ifdef DEBUG_LEX_HIST
  fprintf (debugStream, "getexecl (1): %d, Called with : %c\n", getpid (),
	   to_char (sc));
  fflush (debugStream);
#endif
	if (sc == 0) {
		sc = getC(0);
		if (sc != '{') {
			ungetC(sc);
			sc = 0;
		}
	}
	quesarg = -1;
	lastev = eventno;
	hp = gethent(sc);
	if (hp == 0)
		return;
	hadhist = 1;
	dol = -1;
	if (hp == alhistp)
		for (ip = hp->next; ip != alhistt; ip = ip->next)
			dol++;
	else
		for (ip = hp->next; ip->next != hp; ip = ip->next)
			dol++;
	left = 0, right = dol;
	if (sc == HISTSUB) {
		ungetC('s'), unreadc(HISTSUB), c = ':';
		goto subst;
	}
	c = getC(0);
	if (!any(c, ":^$*-%"))
		goto subst;
	left = right = -1;
	if (c == ':') {
		c = getC(0);
		unreadc(c);
		if (letter(c) || c == '&') {
			c = ':';
			left = 0, right = dol;
			goto subst;
		}
	} else
		ungetC(c);
	if (!getsel(&left, &right, dol))
		return;
	c = getC(0);
	if (c == '*')
		ungetC(c), c = '-';
	if (c == '-') {
		if (!getsel(&left, &right, dol))
			return;
		c = getC(0);
	}
subst:
	exclc = right - left + 1;
	while (--left >= 0)
		hp = hp->next;
	if (sc == HISTSUB || c == ':') {
		do {
			hp = getsub(hp);
			c = getC(0);
		} while (c == ':');
	}
	unreadc(c);
	if (sc == '{') {
		c = getC(0);
		if (c != '}')
			seterr((catgets(nlmsg_fd,NL_SETN,5, "Bad ! form")));
	}
	exclnxt = hp;
}

/*  Called by:
	getexcl ()
*/
/**********************************************************************/
struct wordent *
getsub(en)
	struct wordent *en;
/**********************************************************************/
{
	register CHAR *cp;
	int delim;
	register int c;
	int sc;
	bool global = 0;
	CHAR orhsb[sizeof rhsb / sizeof (CHAR)];

#ifdef DEBUG_LEX_HIST
  fprintf (debugStream, "getsub (1): %d\n", getpid ());
  fflush (debugStream);
#endif
        delim = 0;
	exclnxt = 0;
	sc = c = getC(0);
	if (c == 'g')
		global++,  sc = c = getC(0);
	switch (c) {

	case 'p':
		justpr++;
		goto ret;

	case 'x':
	case 'q':
		global++;
		/* fall into ... */

	case 'h':
	case 'r':
	case 't':
	case 'e':
		break;

	case '&':
		if (slhs[0] == 0) {
			seterr((catgets(nlmsg_fd,NL_SETN,6, "No prev sub")));
			goto ret;
		}
		Strcpy(lhsb, slhs);
		break;

/*
	case '~':
		if (lhsb[0] == 0)
			goto badlhs;
		break;
*/

	case 's':
		delim = getC(0);
#ifndef NLS
		if (letter(delim) || digit(delim) || any(delim, " \t\n")) {
#else
		if (letter(delim) || digit(delim) || any(delim, " \t\n") || delim == ALT_SP) {
#endif
			unreadc(delim);
bads:
			lhsb[0] = 0;
			seterr((catgets(nlmsg_fd,NL_SETN,7, "Bad substitute")));
			goto ret;
		}
		cp = lhsb;
		for (;;) {
			c = getC(0);
			if (c == '\n') {
				unreadc(c);
				goto bads;
			}
			if (c == delim)
				break;
			if (cp > &lhsb[sizeof lhsb / sizeof (CHAR) - 2])
				goto bads;
			if (c == '\\') {
				c = getC(0);
				if (c != delim && c != '\\')
					*cp++ = '\\';
			}
			*cp++ = c;
		}
		if (cp != lhsb)
			*cp++ = 0;
		else if (lhsb[0] == 0) {
/*badlhs:*/
			seterr((catgets(nlmsg_fd,NL_SETN,8, "No prev lhs")));
			goto ret;
		}
		cp = rhsb;
		Strcpy(orhsb, cp);
		for (;;) {
			c = getC(0);
			if (c == '\n') {
				unreadc(c);
				break;
			}
			if (c == delim)
				break;
/*
			if (c == '~') {
				if (&cp[Strlen(orhsb)] > &rhsb[sizeof rhsb / sizeof (CHAR) - 2])
					goto toorhs;
				Strcpy(cp, orhsb);
				cp = strend(cp);
				continue;
			}
*/
			if (cp > &rhsb[sizeof rhsb / sizeof (CHAR) - 2]) {
/*toorhs:*/
				seterr((catgets(nlmsg_fd,NL_SETN,9, "Rhs too long")));
				goto ret;
			}
			if (c == '\\') {
				c = getC(0);
				if (c != delim /* && c != '~' */)
					*cp++ = '\\';
			}
			*cp++ = c;
		}
		*cp++ = 0;
		break;

	default:
		if (c == '\n')
			unreadc(c);
		seterrc((catgets(nlmsg_fd,NL_SETN,10, "Bad ! modifier: ")), c);
		goto ret;
	}
	Strcpy(slhs, lhsb);
	if (exclc)
		en = dosub(sc, en, global);
ret:
	return (en);
}

/*  Called by:
	getsub ()
*/
/**********************************************************************/
struct wordent *
dosub(sc, en, global)
	int sc;
	struct wordent *en;
	bool global;
/**********************************************************************/
{
	struct wordent lex;
	bool didsub = 0;
	struct wordent *hp = &lex;
	register struct wordent *wdp;
	register int i = exclc;

#ifdef DEBUG_LEX_HIST
  fprintf (debugStream, "dosub (1): %d\n", getpid ());
  fflush (debugStream);
#endif
	wdp = hp;
	while (--i >= 0) {
		register struct wordent *new = (struct wordent *) calloc(1, sizeof *wdp);

		new->prev = wdp;
		new->next = hp;
		wdp->next = new;
		wdp = new;
		en = en->next;
		wdp->word = global || didsub == 0 ?
		    subword(en->word, sc, &didsub) : savestr(en->word);
	}
	if (didsub == 0)
		seterr((catgets(nlmsg_fd,NL_SETN,11, "Modifier failed")));
	hp->prev = wdp;
	return (&enthist(-1000, &lex, 0)->Hlex);
}

/*  Called by:
	dosub ()
*/
/**********************************************************************/
CHAR *
subword(cp, type, adid)
	CHAR *cp;
	int type;
	bool *adid;
/**********************************************************************/
{
	CHAR wbuf[WRDSIZ];
	register CHAR *wp, *mp, *np;
	register int i;

#ifdef DEBUG_LEX_HIST
  fprintf (debugStream, "subword (1): %d\n", getpid ());
  fflush (debugStream);
#endif
	switch (type) {

	case 'r':
	case 'e':
	case 'h':
	case 't':
	case 'q':
	case 'x':
		wp = domod(cp, type, 1);
		if (wp == 0)
			return (savestr(cp));
		*adid = 1;
		return (wp);

	default:
		wp = wbuf;
		i = WRDSIZ - 4;
		for (mp = cp; *mp; mp++)
			if (matchs(mp, lhsb)) {
				for (np = cp; np < mp;)
					*wp++ = *np++, --i;
				for (np = rhsb; *np; np++) switch (*np) {

				case '\\':
					if (np[1] == '&')
						np++;
					/* fall into ... */

				default:
					if (--i < 0)
						goto ovflo;
					*wp++ = *np;
					continue;

				case '&':
					i -= Strlen(lhsb);
					if (i < 0)
						goto ovflo;
					*wp = 0;
					Strcat(wp, lhsb);
					wp = strend(wp);
					continue;
				}
				mp += Strlen(lhsb);
				i -= Strlen(mp);
				if (i < 0) {
ovflo:
					seterr((catgets(nlmsg_fd,NL_SETN,12, "Subst buf ovflo")));
					return (nullstr);
				}
				*wp = 0;
				Strcat(wp, mp);
				*adid = 1;
				return (savestr(wbuf));
			}
		return (savestr(cp));
	}
}

/*  Called by:
	setDolp ()
	subword ()
*/
/**********************************************************************/
CHAR *
domod(cp, type, histsub)
	CHAR *cp;
	int type, histsub;
/**********************************************************************/
{
	register CHAR *wp, *xp;
	register int c;

#ifdef DEBUG_LEX_HIST
  fprintf (debugStream, "domod (1): %d\n", getpid ());
  fflush (debugStream);
#endif
	switch (type) {

	case 'x':
	case 'q':
		wp = savestr(cp);
		for (xp = wp; c = *xp; xp++)
#ifndef NLS
			if ((c != ' ' && c != '\t') || type == 'q')
#else
			if ((c != ' ' && c != '\t' && c != ALT_SP) || type == 'q')
#endif
				*xp |= QUOTE;
		return (wp);

	case 'h':
	case 't':
		if (!Any('/', cp))	/* what if :h :t are both the same? */
			return (0);
		wp = strend(cp);
		while (*--wp != '/')
			continue;
		if (type == 'h')
			xp = savestr(cp), xp[wp - cp] = 0;
		else
			xp = savestr(wp + 1);
		return (xp);

	case 'e':
	case 'r':
		if (histsub && !Any('.', cp))
			return (0);
		wp = strend(cp);
		for (wp--; wp >= cp && *wp != '/'; wp--)
			if (*wp == '.') {
				if (type == 'e')
					xp = savestr(wp + 1);
				else
					xp = savestr(cp), xp[wp - cp] = 0;
				return (xp);
			}
		return(savestr(type == 'e' ? nullstr : cp));
	}
	return (0);
}

/*  Called by:
	subword ()
	matchev ()
*/
/**********************************************************************/
matchs(str, pat)
	register CHAR *str, *pat;
/**********************************************************************/
{

#ifdef DEBUG_LEX_HIST
  fprintf (debugStream, "matchs (1): %d\n", getpid ());
  fflush (debugStream);
#endif
	while (*str && *pat && *str == *pat)
		str++, pat++;
	return (*pat == 0);
}

/*  Called by:
	getexcl ()
*/
/**********************************************************************/
getsel(al, ar, dol)
	register int *al, *ar;
	int dol;
/**********************************************************************/
{
	register int c = getC(0);
	register int i;
	bool first = *al < 0;

#ifdef DEBUG_LEX_HIST
  fprintf (debugStream, "getsel (1): %d\n", getpid ());
  fflush (debugStream);
#endif
	switch (c) {

	case '%':
		if (quesarg == -1)
			goto bad;
		if (*al < 0)
			*al = quesarg;
		*ar = quesarg;
		break;

	case '-':
		if (*al < 0) {
			*al = 0;
			*ar = dol - 1;
			unreadc(c);
		}
		return (1);

	case '^':
		if (*al < 0)
			*al = 1;
		*ar = 1;
		break;

	case '$':
		if (*al < 0)
			*al = dol;
		*ar = dol;
		break;

	case '*':
		if (*al < 0)
			*al = 1;
		*ar = dol;
		if (*ar < *al) {
			*ar = 0;
			*al = 1;
			return (1);
		}
		break;

	default:
		if (digit(c)) {
			i = 0;
			while (digit(c)) {
				i = i * 10 + c - '0';
				c = getC(0);
			}
			if (i < 0)
				i = dol + 1;
			if (*al < 0)
				*al = i;
			*ar = i;
		} else
			if (*al < 0)
				*al = 0, *ar = dol;
			else
				*ar = dol - 1;
		unreadc(c);
		break;
	}
	if (first) {
		c = getC(0);
		unreadc(c);
		if (any(c, "-$*"))
			return (1);
	}
	if (*al > *ar || *ar > dol) {
bad:
		seterr((catgets(nlmsg_fd,NL_SETN,13, "Bad ! arg selector")));
		return (0);
	}
	return (1);

}

/*  Called by:
	getexcl ()
*/
/**********************************************************************/
struct wordent *
gethent(sc)
	int sc;
/**********************************************************************/
{
	register struct Hist *hp;
	register CHAR *np;
	register int c;
	int event;
	bool back = 0;

#ifdef DEBUG_LEX_HIST
  fprintf (debugStream, "gethent (1): %d\n", getpid ());
  fflush (debugStream);
#endif
	c = sc == HISTSUB ? HIST : getC(0);
	if (c == HIST) {
		if (alhistp)
			return (alhistp);
		event = eventno;
		goto skip;
	}
	switch (c) {

	case ':':
	case '^':
	case '$':
	case '*':
	case '%':
		ungetC(c);
		if (lastev == eventno && alhistp)
			return (alhistp);
		event = lastev;
		break;

	case '-':
		back = 1;
		c = getC(0);
		goto number;

	case '#':			/* !# is command being typed in (mrh) */
		return(&paraml);

	default:
		if (any(c, "(=~")) {
			unreadc(c);
			ungetC(HIST);
			return (0);
		}
		if (digit(c))
			goto number;
		np = lhsb;
#ifndef NLS
		while (!any(c, ": \t\\\n}")) {
#else
		while (!any(c, ": \t\\\n}") && c != ALT_SP) {
#endif
			if (np < &lhsb[sizeof lhsb / sizeof (CHAR) - 2])
				*np++ = c;
			c = getC(0);
		}
		unreadc(c);
		if (np == lhsb) {
			ungetC(HIST);
			return (0);
		}
		*np++ = 0;

/*  The string must be the first argument.
*/
		hp = findev(lhsb, 0);

/*  Return the word list from the history.
*/
		if (hp)
			lastev = hp->Hnum;
		return (&hp->Hlex);

	case '?':
		np = lhsb;
		for (;;) {
			c = getC(0);
			if (c == '\n') {
				unreadc(c);
				break;
			}
			if (c == '?')
				break;
			if (np < &lhsb[sizeof lhsb / sizeof (CHAR) - 2])
				*np++ = c;
		}
		if (np == lhsb) {
			if (lhsb[0] == 0) {
				seterr((catgets(nlmsg_fd,NL_SETN,14, "No prev search")));
				return (0);
			}
		} else
			*np++ = 0;

/*  The string can be any argument.
*/
		hp = findev(lhsb, 1);

/*  Return the word list from the history.
*/
		if (hp)
			lastev = hp->Hnum;
		return (&hp->Hlex);

	number:
		event = 0;
		while (digit(c)) {
			event = event * 10 + c - '0';
			c = getC(0);
		}
		if (back)
			event = eventno + (alhistp == 0) - (event ? event : 0);
		unreadc(c);
		break;
	}

/*  Looking for a particular history entry.  If found then return the
    wordlist.
*/

skip:
	for (hp = Histlist.Hnext; hp; hp = hp->Hnext)
		if (hp->Hnum == event) {
			hp->Href = eventno;
			lastev = hp->Hnum;
			return (&hp->Hlex);
		}
	np = putn(event);
	noev(np);
	return (0);
}

/*  Called by:
	gethent ()
*/
/*  Purpose:  Return a whole structure if a particular event is found.
*/
/**********************************************************************/
struct Hist *
findev(cp, anyarg)
	CHAR *cp;
	bool anyarg;
/**********************************************************************/
{
	register struct Hist *hp;

#ifdef DEBUG_LEX_HIST
  fprintf (debugStream, "findev (1): %d\n", getpid ());
  fflush (debugStream);
#endif
	for (hp = Histlist.Hnext; hp; hp = hp->Hnext)
		if (matchev(hp, cp, anyarg))
			return (hp);
	noev(cp);
	return (0);
}

/*  Called by:
	gethent ()
	findev ()
*/
/**********************************************************************/
noev(cp)
	CHAR *cp;
/**********************************************************************/
{

	seterr2(to_char(cp), (catgets(nlmsg_fd,NL_SETN,15, ": Event not found")));
}

/*  Called by:
	findev ()
*/
/*  Purpose:  Search a wordlist of history for a string match.
*/
/**********************************************************************/
matchev(hp, cp, anyarg)
	register struct Hist *hp;
	CHAR *cp;
	bool anyarg;
/**********************************************************************/
{
	register CHAR *dp;
	struct wordent *lp = &hp->Hlex;
	int argno = 0;

	for (;;) {
		lp = lp->next;
		if (lp->word[0] == '\n')
			return (0);

/*  Look for a match of the 1st argument (this will loop through all the
    arguments in some cases).  If a match is found and 'anyarg' is TRUE then 
    save the argument number and return TRUE.  If no match occurs and 'anyarg' 
    is FALSE then return FALSE.  If no match occurs and 'anyarg' is TRUE, try 
    the next argument.
*/
		for (dp = lp->word; *dp; dp++) {
			if (matchs(dp, cp)) {
				if (anyarg)
					quesarg = argno;
				return (1);
			}
			if (!anyarg)
				return (0);
		}
		argno++;
	}
}

/*  Called by:
	getC ()
*/
/**********************************************************************/
setexclp(cp)
	register CHAR *cp;
/**********************************************************************/
{

#ifdef DEBUG_LEX_HIST
  fprintf (debugStream, "setexeclp (1): %d\n", getpid ());
  fflush (debugStream);
#endif

	if (cp[0] == '\n')
		return;
	exclp = cp;
}

/*  Called by:
	getword ()
	gethent ()
	getexcl ()
	readc ()
	getsub ()
	getsel ()
*/
/**********************************************************************/
unreadc(c)
	CHAR c;
/**********************************************************************/
{

	peekread = c;
}

/*  Called by:
	heredoc ()
	getword ()
	lex ()
	getC ()
*/
/**********************************************************************/
readc(wanteof)
	bool wanteof;
/**********************************************************************/
{
	register int c;
	static sincereal = 0;

	if (c = peekread) {
		peekread = 0;
		sincereal = 0;
		return (c);
	}
top:
	if (alvecp) {
		if (c = *alvecp++)
		  {
			sincereal = 0;
			return (c);
		  }
		if (*alvec) {
			alvecp = *alvec++;
			sincereal = 0;
			return (' ');
		}
	}
	if (alvec) {
		if (alvecp = *alvec) {
			alvec++;
			goto top;
		}
		/* Infinite source! */
		sincereal = 0;
		return ('\n');
	}
	if (evalp) {
		if (c = *evalp++)
		  {
			sincereal = 0;
			return (c);
		  }
		if (*evalvec) {
			evalp = *evalvec++;
			sincereal = 0;
			return (' ');
		}
		evalp = 0;
	}
	if (evalvec) {
		if (evalvec == (CHAR **)1) {
			doneinp = 1;
			reset();
		}
		if (evalp = *evalvec) {
			evalvec++;
			goto top;
		}
		evalvec = (CHAR **)1;
		sincereal = 0;
		return ('\n');
	}

/*  This loop works on arginp if it is set.  It is set in the case of a 
    backquote evaluation.  It turns out that backeval() copies the string
    between backquotes into arginp and then calls lex() on it.  That 
    eventually leads to this loop.
*/
	do {
		if (arginp == (CHAR *) 1 || onelflg == 1) {
			if (wanteof)
			  {
				sincereal = 0;
				return (-1);
			  }
			exitstat();
		}
		if (arginp) {
			if ((c = *arginp++) == 0) {
				arginp = (CHAR *) 1;
				sincereal = 0;
#ifdef DEBUG_READC
  fprintf (debugStream, "readc (1): %d, arginp ch: %lo\n", getpid (),(long ) c);
  fflush (debugStream);
#endif
				return ('\n');
			}
			sincereal = 0;
#ifdef DEBUG_READC
  fprintf (debugStream, "readc (2): %d, arginp ch: %lo\n", getpid (),(long ) c);
  fflush (debugStream);
#endif
			return (c);
		}
#ifdef SIGTSTP
reread:
#endif
		c = bgetc();

#ifdef DEBUG_READC
  fprintf (debugStream, "readc (3): %d, char: %lo\n", getpid (), (long ) c);
  fflush (debugStream);
#endif
		if (c == -1) {
#include <termio.h>
			struct termio tty;

#ifdef DEBUG_READC
  fprintf (debugStream, "readc (4): %d, Got a -1 back from bgetc\n", getpid ());
  fprintf (debugStream, "\twanteof: %d (if !=0 will return -1)\n", wanteof);
  fflush (debugStream);
#endif

			if (wanteof)
			  {
				sincereal = 0;
				return (-1);
			  }
			if (haveread) {
				haveread = 0;
				unreadc(0);
				sincereal = 0;
				return ('\n');
			}

#ifdef DEBUG_READC
  {
    struct termio debugTty;
    int debugReturnIoctl;

    fprintf (debugStream, "readc (5): %d, Checking TCGETA = 0 && ICANON != 0\n",
	     getpid ());

    debugReturnIoctl = ioctl(SHIN, TCGETA, &debugTty);
    fprintf (debugStream, "\tIoctl return: %d, errno: %d\n", debugReturnIoctl, 
	     errno);
    if (debugReturnIoctl == 0)
      fprintf (debugStream, "\tlocal mode: %o\n", debugTty.c_lflag);
    
    fflush (debugStream);
  }
#endif
			/* was isatty but raw with ignoreeof yields problems */
			if (ioctl(SHIN, TCGETA, &tty)==0 && (tty.c_lflag & ICANON) != 0) {
#ifdef SIGTSTP
				int ctpgrp;
				int tmp;
#endif

				if (++sincereal > 25)
				  {
#ifdef DEBUG_READC
  fprintf (debugStream, "readc (6): %d, sincereal: %d\n", getpid (), sincereal);
  fflush (debugStream);
#endif
					goto oops;
				  }
#ifdef SIGTSTP
				tmp = ioctl(FSHTTY, TIOCGPGRP, &ctpgrp);
				if (tpgrp != -1 && tmp == 0 && tpgrp != ctpgrp) {
					ioctl(FSHTTY, TIOCSPGRP, &tpgrp);
					killpg(ctpgrp, SIGHUP);
printf((catgets(nlmsg_fd,NL_SETN,16, "Reset tty pgrp from %d to %d\n")), ctpgrp, tpgrp);
					goto reread;
				}
				if (tpgrp != -1 && tmp == -1) {
					ioctl(FSHTTY, TIOCSPGRP, &tpgrp);
printf((catgets(nlmsg_fd,NL_SETN,19, "Reset tty pgrp to %d\n")), tpgrp);
					goto reread;
				}
#endif

#ifdef DEBUG_READC
  {
    struct varent *debugAdrof;

    debugAdrof = adrof (CH_ignoreeof);
    fprintf (debugStream, "readc (7): %d, Return from adrof: %ld\n", getpid (), 
	     (long) debugAdrof);
    fflush (debugStream);
  }
#endif
				if (adrof(CH_ignoreeof)) {

#ifdef DEBUG_READC
  fprintf (debugStream, "readc (8): %d, adrof found CH_ignoreeof,doing reset\n",
	   getpid ());
  fflush (debugStream);
#endif

					if (loginsh)
						printf((catgets(nlmsg_fd,NL_SETN,17, "\nUse \"logout\" to logout.\n")));
					else
						printf((catgets(nlmsg_fd,NL_SETN,18, "\nUse \"exit\" to leave csh.\n")));
					reset();
				}
				if (chkstop == 0)
					panystop(1);
			}
oops:
			doneinp = 1;
			reset();
		}
		sincereal = 0;
		if (c == '\n' && onelflg)
			onelflg--;
	} while (c == 0);
	sincereal = 0;
	return (c);
}

/*  Called by:
	readc ()
*/
/*  Purpose:  Return 1 character from the input buffer.
*/
/**********************************************************************/
bgetc()
/**********************************************************************/
{
	register int buf, off, c;
	char ttyline[BUFSIZ];
	register int numleft = 0, roomleft;

	buf = off = c = 0;
#ifdef TELL
	if (cantell) {
		if (fseekp < fbobp || fseekp > feobp) {
			fbobp = feobp = fseekp;
			lseek(SHIN, fseekp, 0);
		}

/*  Read more data from the input stream.
*/
		if (fseekp == feobp) {
			fbobp = feobp;
			do
			    if (!halfkj)
			    	c = read(SHIN, fbuf[0], BUFSIZ);
			    else {
			    	c = read(SHIN, fbuf[0] + 1, BUFSIZ - 1) + 1;
				fbuf[0][0] = halfkj;
				halfkj = 0;
			    }
			while (c < 0 && errno == EINTR);
			if (c <= 0)
			  {
#ifdef DEBUG_BGETC
  fprintf (debugStream, "bgetc (1): %d, char: %ld\n", getpid (), (long ) c);
  fflush (debugStream);
#endif
				return (-1);
			  }
			feobp += c;
		}
       		halfkj = 0; /* Need to reset the halfkj flag here */
		c = fbuf[0][fseekp - fbobp];
		fseekp++;
#ifdef NLS16
		/* Must return a whole character; ie both bytes of a kanji */
		if (FIRSTof2(c)) {
			if ((fseekp%BUFSIZ) == 0) { /* last byte of buf */
				halfkj = c;
				c = bgetc();
#ifdef DEBUG_BGETC
  fprintf (debugStream, "bgetc (2): %d, char: %lo\n", getpid (), (long ) c);
  fflush (debugStream);
#endif
				return (c);
			}
			else if (SECof2(fbuf[0][fseekp - fbobp] & 0377)) {
				c = ((c << 8) | (fbuf[0][fseekp - fbobp] & 0377)) & TRIM;
				fseekp++;
			}
		}
#endif

#ifdef DEBUG_BGETC
  fprintf (debugStream, "bgetc (3): %d, char: %lo\n", getpid (), (long ) c);
  fflush (debugStream);
#endif
		return (c);
	}
#endif
again:
	buf = (int) fseekp / BUFSIZ;

/*  Get another input buffer (?)
*/
	if (buf >= fblocks) {
		register char **nfbuf = (char **) calloc((unsigned) (fblocks+2), sizeof (char **));

		if (fbuf) {
#ifndef NONLS
			/* blkcpy now work on CHAR, but we want to copy bytes here */
			(void) b_blkcpy(nfbuf, fbuf);
#else
			(void) blkcpy(nfbuf, fbuf);
#endif
			xfree((char *) fbuf);
		}
		fbuf = nfbuf;
		fbuf[fblocks] = (char *)calloc(BUFSIZ, sizeof (char));
		fblocks++;
		goto again;
	}
	if (fseekp >= feobp) {
		buf = (int) feobp / BUFSIZ;
		off = (int) feobp % BUFSIZ;
		roomleft = BUFSIZ - off;
		do
		  {
		    if (intty && tenexflag)     /* then use tenex routine */
		    {
			c = numleft ? numleft : tenex(ttyline, BUFSIZ);
			if (c > roomleft)	/* No room in this buffer? */
			{
			    /* start with fresh buffer */
			    feobp = fseekp = fblocks * BUFSIZ;
			    numleft = c;
			    goto again;
			}
			if (c > 0)
#ifndef NONLS
			   /* copy now work on CHAR, but we want to copy bytes here */
			    b_copy (fbuf[buf] + off, ttyline, c);
#else
			    copy (fbuf[buf] + off, ttyline, c);
#endif
			numleft = 0;
		    }
		    else 
			if (!halfkj)
				c = read(SHIN, fbuf[buf] + off, roomleft);
			else {
				c = read(SHIN, fbuf[buf] + off + 1, roomleft - 1) + 1;
				fbuf[buf][off] = halfkj;
			/*	halfkj = 0; should be reset even if read does
			 * not take place. Moved this to outside the if */
			}
		  }
		while (c < 0 && errno == EINTR);
		if (c <= 0)
		  {

#ifdef DEBUG_BGETC
  fprintf (debugStream, "bgetc (4): %d, char: %ld\n", getpid (), (long ) c);
  fflush (debugStream);
#endif
			return (-1);
                  }

		feobp += c;
		if (!intty || !tenexflag)
		    goto again;
	}
       	halfkj = 0; /* Need to reset the halfkj flag here */
	c = (fbuf[buf][(int) fseekp % BUFSIZ]) & 0377;
	fseekp++;
#ifdef NLS16
	/* get a whole character */
	if (FIRSTof2(c)) {
		if ((fseekp%BUFSIZ) == 0) { /* last byte of buf */
			halfkj = c;
			goto again;
		}
		else if (SECof2(fbuf[buf][(int)fseekp % BUFSIZ] & 0377)) {
			c = ((c << 8) | (fbuf[buf][(int)fseekp % BUFSIZ] & 0377)) & TRIM;
			fseekp++;
		}
	}
#endif

#ifdef DEBUG_FILE_LEX
  fprintf (debugStream, "bgetc (5): %d, char: %lo\n", getpid (), (long ) c);
  fflush (debugStream);
#endif
	return (c);
}

/* Called by:
	btoeof ()
*/
/**********************************************************************/
bfree()
/**********************************************************************/
{
	register int sb, i;

#ifdef TELL
	if (cantell)
		return;
#endif
	if (whyles)
		return;

/*  BUFSIZ is defined to be 512.  This does a 'div' so that if the input
    buffer pointer is less than 513, sb will be 0.  This seems to be the
    number of input buffers has been used (?).  Then the routine frees them
    if more than one exists.
*/
	sb = (int) (fseekp - 1) / BUFSIZ;
	if (sb > 0) {
		for (i = 0; i < sb; i++)
			xfree(fbuf[i]);

/*  Copy from >sb down to 0
*/
#ifndef NONLS
		(void) b_blkcpy(fbuf, &fbuf[sb]);
#else
		(void) blkcpy(fbuf, &fbuf[sb]);
#endif
		fseekp -= BUFSIZ * sb;
		feobp -= BUFSIZ * sb;
		fblocks -= sb;
	}
}

/* Called by:
	dogoto ()
	doagain ()
	search ()
	toend ()
*/
/**********************************************************************/
bseek(l)
	long l;
/**********************************************************************/
{
	register struct whyle *wp;

	fseekp = l;
#ifdef TELL
	if (!cantell) {
#endif
		if (!whyles)
			return;

/*  Go through while loops
*/
		for (wp = whyles; wp->w_next; wp = wp->w_next)
			continue;

/*  'l' is used as an address, points to the "restart loop".
*/
		if (wp->w_start > l)
			l = wp->w_start;
#ifdef TELL
	}
#endif
}

/* any similarity to bell telephone is purely accidental */
/*  Called by:
	dogoto ()
	wfree ()
	doforeach ()
	lex ()
	preread ()
	doend ()
	toend ()
*/
/**********************************************************************/
long
btell()
/**********************************************************************/
{

	return (fseekp);
}

/*  Called by:
	error ()
	doexit ()
*/
/**********************************************************************/
btoeof()
/**********************************************************************/
{

/*  Move the pointer to the size of the file plus the offset (01) since
    the 'whence' field is 2.
*/
    /* Bad fix made for FSDlj08532. cflag not needed. see sh.err.c for fix */
    /* if (!cflag) */
	lseek(SHIN, 0l, 2);

/*  Set the seed pointer to the seek pointer at the end of the buffers.
    'fseekp' = B.Bfseekp (seek pointer)
    'feobp' = B.Bfeobp (seek pointer of end of buffers)
*/
	fseekp = feobp;

/*  Free memory for while loops, then free memory from input buffers if
    there is more than 1 buffer.
*/
	wfree();
	bfree();
}

#ifdef TELL
/**********************************************************************/
settell()
/**********************************************************************/
{

	cantell = 0;
	if (arginp || onelflg || intty)
		return;
	if (lseek(SHIN, 0l, 1) == -1 || errno == ESPIPE)
		return;
	fbuf = (char **) calloc(2, sizeof (char **));
	fblocks = 1;
	fbuf[0] = calloc(BUFSIZ, sizeof (char));
	fseekp = fbobp = feobp = lseek(SHIN,0l,1);/* cfi */
	cantell = 1;
}
#endif
