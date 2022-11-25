/* @(#) $Revision: 66.1 $ */    
/******************************************************************
 * Perform aliasing on the word list lex
 * Do a (very rudimentary) parse to separate into commands.
 * If word 0 of a command has an alias, do it.
 * Repeat a maximum of 20 times.
 ******************************************************************/

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 12	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

#include "sh.h"

alias(lex)
	register struct wordent *lex;
{
	static int aleft;
	jmp_buf osetexit;

	aleft = 21;
	getexit(osetexit);
	setexit();

#ifdef DEBUG_ALIAS
  printf ("alias (1): %d, aleft: %d\n", getpid (), aleft);
#endif
	if (haderr) {
		resexit(osetexit);
		reset();
	}
	if (--aleft == 0)
	  {

#ifdef DEBUG_ALIAS
  printf ("alias (2): %d, aleft went to 0\n", getpid ());
#endif
		error((catgets(nlmsg_fd,NL_SETN,1, "Alias loop")));
	  }

#ifdef DEBUG_ALIAS
  if (to_char (lex -> next -> word) == "")
    {
      printf ("alias (3): %d, Calling asyntax with NULL", getpid ());

      if (to_char (lex -> word) == "")
        printf (" and NULL\n");
      
      else
        printf (" and %s\n", to_char (lex -> word));
    }

  else
    {
      printf ("alias (3): %d, Calling asyntax with %s", getpid (),
	      to_char (lex -> next -> word));

      if (to_char (lex -> word) == "")
        printf (" and NULL\n");
  
      else
        printf (" and %s\n", to_char (lex -> word));
    }

#endif
	asyntax(lex->next, lex);
	resexit(osetexit);
}

asyntax(p1, p2)
	register struct wordent *p1, *p2;
{

	while (p1 != p2)
		if (any(p1->word[0], ";&\n"))
			p1 = p1->next;
		else {

#ifdef DEBUG_ALIAS
  if (to_char (p1 -> word) == "")
    {
      printf ("asyntax (1): %d, Calling asyn0 with NULL", getpid ());

      if (to_char (p2 -> word) == "")
        printf (" and NULL\n");
      
      else
        printf (" and %s\n", to_char (p2 -> word));
    }

  else
    {
      printf ("asyntax (1): %d, Calling asyn0 with %s", getpid (),
	      to_char (p1 -> word));

      if (to_char (p2 -> word) == "")
        printf (" and NULL\n");
  
      else
        printf (" and %s\n", to_char (p2 -> word));
    }
#endif
			asyn0(p1, p2);
			return;
		}
}

asyn0(p1, p2)
	struct wordent *p1;
	register struct wordent *p2;
{
	register struct wordent *p;
	register int l = 0;

	for (p = p1; p != p2; p = p->next)
		switch (p->word[0]) {

		case '(':
			l++;
			continue;

		case ')':
			l--;
			if (l < 0)
				error((catgets(nlmsg_fd,NL_SETN,2, "Too many )'s")));
			continue;

		case '>':
			if (p->next != p2 && eq(p->next->word, "&"))
				p = p->next;
			continue;

		case '&':
		case '|':
		case ';':
		case '\n':
			if (l != 0)
				continue;
#ifdef DEBUG_ALIAS
  if (to_char (p1 -> word) == "")
    {
      printf ("asyn0 (1): %d, Calling asyn3 with NULL", getpid ());

      if (to_char (p -> word) == "")
        printf (" and NULL\n");
      
      else
        printf (" and %s\n", to_char (p -> word));
    }

  else
    {
      printf ("asyn0 (1): %d, Calling asyn3 with %s", getpid (),
	      to_char (p1 -> word));

      if (to_char (p -> word) == "")
        printf (" and NULL\n");
  
      else
        printf (" and %s\n", to_char (p -> word));
    }
#endif
			asyn3(p1, p);

#ifdef DEBUG_ALIAS
  if (to_char (p -> next -> word) == "")
    {
      printf ("asyn0 (2): %d, Calling asyntax with NULL", getpid ());

      if (to_char (p2 -> word) == "")
        printf (" and NULL\n");
      
      else
        printf (" and %s\n", to_char (p2 -> word));
    }

  else
    {
      printf ("asyn0 (2): %d, Calling asyntax with %s", getpid (),
	      to_char (p -> next -> word));

      if (to_char (p2-> word) == "")
        printf (" and NULL\n");
  
      else
        printf (" and %s\n", to_char (p2-> word));
    }
#endif
			asyntax(p->next, p2);
			return;
		}
	if (l == 0)
          {

#ifdef DEBUG_ALIAS
  if (to_char (p1 -> word) == "")
    {
      printf ("asyn0 (3): %d, Calling asyn3 with NULL", getpid ());

      if (to_char (p2 -> word) == "")
        printf (" and NULL\n");
      
      else
        printf (" and %s\n", to_char (p2 -> word));
    }

  else
    {
      printf ("asyn0 (3): %d, Calling asyn3 with %s", getpid (),
	      to_char (p1 -> word));

      if (to_char (p2 -> word) == "")
        printf (" and NULL\n");
  
      else
        printf (" and %s\n", to_char (p2 -> word));
    }
#endif
		asyn3(p1, p2);
          }
}

#ifndef NONLS
CHAR CH_quote[] = { 0100000, 0};	/* this is for asyn3 which precedes */
					/* command with a quote character */
#else
#define CH_quote	"\200"
#endif

asyn3(p1, p2)
	struct wordent *p1;
	register struct wordent *p2;
{
	register struct varent *ap;
	struct wordent alout;
	register bool redid;

	if (p1 == p2)
		return;
	if (p1->word[0] == '(') {
		for (p2 = p2->prev; p2->word[0] != ')'; p2 = p2->prev)
			if (p2 == p1)
				return;
		if (p2 == p1->next)
			return;
		asyn0(p1->next, p2);
		return;
	}
	ap = adrof1(p1->word, &aliases);
	if (ap == 0)
		return;
	alhistp = p1->prev;
	alhistt = p2;
	alvec = ap->vec;
	redid = lex(&alout);
	alhistp = alhistt = 0;
	alvec = 0;
	if (err) {
		freelex(&alout);
		error(err);
	}
	if (p1->word[0] && Strcmp(p1->word, alout.next->word) == 0) {
		CHAR *cp = alout.next->word;

		alout.next->word = Strspl(CH_quote, cp); 
		xfree(cp);
	}
	p1 = freenod(p1, redid ? p2 : p1->next);
	if (alout.next != &alout) {
		p1->next->prev = alout.prev->prev;
		alout.prev->prev->next = p1->next;
		alout.next->prev = p1;
		p1->next = alout.next;

#ifdef DEBUG_ALIAS
  if (to_char (p1 -> word) == "")
    {
      printf ("asyn3 (1): %d, p1 is NULL", getpid ());

      if (to_char (p1 -> next -> word) == "")
        printf (" and NULL\n");
      
      else
        printf (" and p1 -> next is %s\n", to_char (p1 -> next -> word));
    }

  else
    {
      printf ("asyn3 (1): %d, p1 is %s", getpid (),
	      to_char (p1 -> word));

      if (to_char (p1 -> next -> word) == "")
        printf (" and NULL\n");
  
      else
        printf (" and p1 -> next is %s\n", to_char (p1 -> next -> word));
    }
#endif
		xfree(alout.prev->word);
		xfree((CHAR *)(alout.prev));
	}

#ifdef DEBUG_ALIAS
  printf ("asyn3 (2): %d, doing a longjmp\n", getpid ());
#endif
	reset();		/* throw! */
}

struct wordent *
freenod(p1, p2)
	register struct wordent *p1, *p2;
{
	register struct wordent *retp = p1->prev;

	while (p1 != p2) {
		xfree(p1->word);
		p1 = p1->next;
		xfree((CHAR *)(p1->prev));
	}
	retp->next = p2;
	p2->prev = retp;
	return (retp);
}

#define	PHERE	1
#define	PIN	2
#define	POUT	4
#define	PDIAG	8

/*
 * syntax
 *	empty
 *	syn0
 */
struct command *
syntax(p1, p2, flags)
	register struct wordent *p1, *p2;
	int flags;
{

	while (p1 != p2)
		if (any(p1->word[0], ";&\n"))
			p1 = p1->next;
		else
			return (syn0(p1, p2, flags));
	return (0);
}

/*
 * syn0
 *	syn1
 *	syn1 & syntax
 */
struct command *
syn0(p1, p2, flags)
	struct wordent *p1, *p2;
	int flags;
{
	register struct wordent *p;
	register struct command *t, *t1;
	int l;

	l = 0;
	for (p = p1; p != p2; p = p->next)
		switch (p->word[0]) {

		case '(':
			l++;
			continue;

		case ')':
			l--;
			if (l < 0)
				seterr((catgets(nlmsg_fd,NL_SETN,3, "Too many )'s")));
			continue;

		case '|':
			if (p->word[1] == '|')
				continue;
			/* fall into ... */

		case '>':
			if (p->next != p2 && eq(p->next->word, "&"))
				p = p->next;
			continue;

		case '&':
			if (l != 0)
				break;
			if (p->word[1] == '&')
				continue;
			t1 = syn1(p1, p, flags);
			if (t1->t_dtyp == TLST) {
				t = (struct command *) calloc(1, sizeof (*t));
				t->t_dtyp = TPAR;
				t->t_dflg = FAND|FINT;
				t->t_dspr = t1;
				t1 = t;
			} else
				t1->t_dflg |= FAND|FINT;
			t = (struct command *) calloc(1, sizeof (*t));
			t->t_dtyp = TLST;
			t->t_dflg = 0;
			t->t_dcar = t1;
			t->t_dcdr = syntax(p, p2, flags);
			return(t);
		}
	if (l == 0)
		return (syn1(p1, p2, flags));
	seterr((catgets(nlmsg_fd,NL_SETN,4, "Too many ('s")));
	return (0);
}

/*
 * syn1
 *	syn1a
 *	syn1a ; syntax
 */
struct command *
syn1(p1, p2, flags)
	struct wordent *p1, *p2;
	int flags;
{
	register struct wordent *p;
	register struct command *t;
	int l;

	l = 0;
	for (p = p1; p != p2; p = p->next)
		switch (p->word[0]) {

		case '(':
			l++;
			continue;

		case ')':
			l--;
			continue;

		case ';':
		case '\n':
			if (l != 0)
				break;
			t = (struct command *) calloc(1, sizeof (*t));
			t->t_dtyp = TLST;
			t->t_dcar = syn1a(p1, p, flags);
			t->t_dcdr = syntax(p->next, p2, flags);
			if (t->t_dcdr == 0)
				t->t_dcdr = t->t_dcar, t->t_dcar = 0;
			return (t);
		}
	return (syn1a(p1, p2, flags));
}

/*
 * syn1a
 *	syn1b
 *	syn1b || syn1a
 */
struct command *
syn1a(p1, p2, flags)
	struct wordent *p1, *p2;
	int flags;
{
	register struct wordent *p;
	register struct command *t;
	register int l = 0;

	for (p = p1; p != p2; p = p->next)
		switch (p->word[0]) {

		case '(':
			l++;
			continue;

		case ')':
			l--;
			continue;

		case '|':
			if (p->word[1] != '|')
				continue;
			if (l == 0) {
				t = (struct command *) calloc(1, sizeof (*t));
				t->t_dtyp = TOR;
				t->t_dcar = syn1b(p1, p, flags);
				t->t_dcdr = syn1a(p->next, p2, flags);
				t->t_dflg = 0;
				return (t);
			}
			continue;
		}
	return (syn1b(p1, p2, flags));
}

/*
 * syn1b
 *	syn2
 *	syn2 && syn1b
 */
struct command *
syn1b(p1, p2, flags)
	struct wordent *p1, *p2;
	int flags;
{
	register struct wordent *p;
	register struct command *t;
	register int l = 0;

	for (p = p1; p != p2; p = p->next)
		switch (p->word[0]) {

		case '(':
			l++;
			continue;

		case ')':
			l--;
			continue;

		case '&':
			if (p->word[1] == '&' && l == 0) {
				t = (struct command *) calloc(1, sizeof (*t));
				t->t_dtyp = TAND;
				t->t_dcar = syn2(p1, p, flags);
				t->t_dcdr = syn1b(p->next, p2, flags);
				t->t_dflg = 0;
				return (t);
			}
			continue;
		}
	return (syn2(p1, p2, flags));
}

/*
 * syn2
 *	syn3
 *	syn3 | syn2
 *	syn3 |& syn2
 */
struct command *
syn2(p1, p2, flags)
	struct wordent *p1, *p2;
	int flags;
{
	register struct wordent *p, *pn;
	register struct command *t;
	register int l = 0;
	int f;

	f = 0;  /* initialize auto variable */
	for (p = p1; p != p2; p = p->next)
		switch (p->word[0]) {

		case '(':
			l++;
			continue;

		case ')':
			l--;
			continue;

		case '|':
			if (l != 0)
				continue;
			t = (struct command *) calloc(1, sizeof (*t));
			f = flags | POUT;
			pn = p->next;
			if (pn != p2 && pn->word[0] == '&') {
				f |= PDIAG;
				t->t_dflg |= FDIAG;
			}
			t->t_dtyp = TFIL;
			t->t_dcar = syn3(p1, p, f);
			if (pn != p2 && pn->word[0] == '&')
				p = pn;
			t->t_dcdr = syn2(p->next, p2, flags | PIN);
			return (t);
		}
	return (syn3(p1, p2, flags));
}

char	*RELPAR =	"<>()";

/*
 * syn3
 *	( syn0 ) [ < in  ] [ > out ]
 *	word word* [ < in ] [ > out ]
 *	KEYWORD ( word* ) word* [ < in ] [ > out ]
 *
 *	KEYWORD = (@ exit foreach if set switch test while)
 */
struct command *
syn3(p1, p2, flags)
	struct wordent *p1, *p2;
	int flags;
{
	register struct wordent *p;
	struct wordent *lp, *rp;
	register struct command *t;
	register int l;
	CHAR **av;
	int n, c;
	bool specp = 0;

	if (p1 != p2) {
		p = p1;
again:
		switch (srchx(p->word)) {

		case ZELSE:
			p = p->next;
			if (p != p2)
				goto again;
			break;

		case ZEXIT:
		case ZFOREACH:
		case ZIF:
		case ZLET:
		case ZSET:
		case ZSWITCH:
		case ZWHILE:
			specp = 1;
			break;
		}
	}
	n = 0;
	l = 0;
	for (p = p1; p != p2; p = p->next)
		switch (p->word[0]) {

		case '(':
			if (specp)
				n++;
			l++;
			continue;

		case ')':
			if (specp)
				n++;
			l--;
			continue;

		case '>':
		case '<':
			if (l != 0) {
				if (specp)
					n++;
				continue;
			}
			if (p->next == p2)
				continue;
			if (any(p->next->word[0], RELPAR))
				continue;
			n--;
			continue;

		default:
			if (!specp && l != 0)
				continue;
			n++;
			continue;
		}
	if (n < 0)
		n = 0;
	t = (struct command *) calloc(1, sizeof (*t));
	av = (CHAR **) calloc((unsigned) (n + 1), sizeof (CHAR **));
	t->t_dcom = av;
	n = 0;
	if (p2->word[0] == ')')
		t->t_dflg = FPAR;
	lp = 0;
	rp = 0;
	l = 0;
	for (p = p1; p != p2; p = p->next) {
		c = p->word[0];
		switch (c) {

		case '(':
			if (l == 0) {
				if (lp != 0 && !specp)
					seterr((catgets(nlmsg_fd,NL_SETN,5, "Badly placed (")));
				lp = p->next;
			}
			l++;
			goto savep;

		case ')':
			l--;
			if (l == 0)
				rp = p;
			goto savep;

		case '>':
			if (l != 0)
				goto savep;
			if (p->word[1] == '>')
				t->t_dflg |= FCAT;
			if (p->next != p2 && eq(p->next->word, "&")) {
				t->t_dflg |= FDIAG, p = p->next;
				if (flags & (POUT|PDIAG))
					goto badout;
			}
			if (p->next != p2 && eq(p->next->word, "!"))
				t->t_dflg |= FANY, p = p->next;
			if (p->next == p2) {
missfile:
				seterr((catgets(nlmsg_fd,NL_SETN,6, "Missing name for redirect")));
				continue;
			}
			p = p->next;
			if (any(p->word[0], RELPAR))
				goto missfile;
			if ((flags & POUT) && (flags & PDIAG) == 0 || t->t_drit)
badout:
				seterr((catgets(nlmsg_fd,NL_SETN,7, "Ambiguous output redirect")));
			else
				t->t_drit = savestr(p->word);
			continue;

		case '<':
			if (l != 0)
				goto savep;
			if (p->word[1] == '<')
				t->t_dflg |= FHERE;
			if (p->next == p2)
				goto missfile;
			p = p->next;
			if (any(p->word[0], RELPAR))
				goto missfile;
			if ((flags & PHERE) && (t->t_dflg & FHERE))
				seterr((catgets(nlmsg_fd,NL_SETN,8, "Can't << within ()'s")));
			else if ((flags & PIN) || t->t_dlef)
				seterr((catgets(nlmsg_fd,NL_SETN,9, "Ambiguous input redirect")));
			else
				t->t_dlef = savestr(p->word);
			continue;

savep:
			if (!specp)
				continue;
		default:
			if (l != 0 && !specp)
				continue;
			if (err == 0)
				av[n] = savestr(p->word);
			n++;
			continue;
		}
	}
	if (lp != 0 && !specp) {
		if (n != 0)
			seterr((catgets(nlmsg_fd,NL_SETN,10, "Badly placed ()'s")));
		t->t_dtyp = TPAR;
		t->t_dspr = syn0(lp, rp, PHERE);
	} else {
		if (n == 0)
			seterr((catgets(nlmsg_fd,NL_SETN,11, "Invalid null command")));
		t->t_dtyp = TCOM;
	}
	return (t);
}

freesyn(t)
	register struct command *t;
{
	register CHAR **v;

	if (t == 0)
		return;
	switch (t->t_dtyp) {

	case TCOM:
		for (v = t->t_dcom; *v; v++)
			xfree(*v);
		xfree((CHAR *)(t->t_dcom));
		goto lr;

	case TPAR:
		freesyn(t->t_dspr);
		/* fall into ... */

lr:
		xfree(t->t_dlef), xfree(t->t_drit);
		break;

	case TAND:
	case TOR:
	case TFIL:
	case TLST:
		freesyn(t->t_dcar), freesyn(t->t_dcdr);
		break;
	}
	xfree((CHAR *)t);
}
