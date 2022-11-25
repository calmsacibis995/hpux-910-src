/* @(#) $Revision: 64.1 $ */    
/***********************************************************************
        C SHELL
 ***********************************************************************/

#include "sh.h"

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 7	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

#define IGNORE	1	/* in ignore, it means to ignore value, just parse */
#define NOGLOB	2	/* in ignore, it means not to globone */

#define	ADDOP	1
#define	MULOP	2
#define	EQOP	4
#define	RELOP	8
#define	RESTOP	16
#define	ANYOP	31

#define	EQEQ	1
#define	GTR	2
#define	LSS	4
#define	NOTEQ	6
#define EQMATCH 7
#define NOTEQMATCH 8

exp(vp)
	register CHAR ***vp;
{

	return (exp0(vp, 0));
}

exp0(vp, ignore)
	register CHAR ***vp;
	bool ignore;
{
	register int p1 = exp1(vp, ignore);
	
#ifdef EDEBUG
	etraci("exp0 p1", p1, vp);
#endif
	if (**vp && eq(**vp, "||")) {
		register int p2;

		(*vp)++;
		p2 = exp0(vp, (ignore&IGNORE) || p1);
#ifdef EDEBUG
		etraci("exp0 p2", p2, vp);
#endif
		return (p1 || p2);
	}
	return (p1);
}

exp1(vp, ignore)
	register CHAR ***vp;
{
	register int p1 = exp2(vp, ignore);

#ifdef EDEBUG
	etraci("exp1 p1", p1, vp);
#endif
	if (**vp && eq(**vp, "&&")) {
		register int p2;

		(*vp)++;
		p2 = exp1(vp, (ignore&IGNORE) || !p1);
#ifdef EDEBUG
		etraci("exp1 p2", p2, vp);
#endif
		return (p1 && p2);
	}
	return (p1);
}

exp2(vp, ignore)
	register CHAR ***vp;
	bool ignore;
{
	register int p1 = exp2a(vp, ignore);

#ifdef EDEBUG
	etraci("exp3 p1", p1, vp);
#endif
	if (**vp && eq(**vp, "|")) {
		register int p2;

		(*vp)++;
		p2 = exp2(vp, ignore);
#ifdef EDEBUG
		etraci("exp3 p2", p2, vp);
#endif
		return (p1 | p2);
	}
	return (p1);
}

exp2a(vp, ignore)
	register CHAR ***vp;
	bool ignore;
{
	register int p1 = exp2b(vp, ignore);

#ifdef EDEBUG
	etraci("exp2a p1", p1, vp);
#endif
	if (**vp && eq(**vp, "^")) {
		register int p2;

		(*vp)++;
		p2 = exp2a(vp, ignore);
#ifdef EDEBUG
		etraci("exp2a p2", p2, vp);
#endif
		return (p1 ^ p2);
	}
	return (p1);
}

exp2b(vp, ignore)
	register CHAR ***vp;
	bool ignore;
{
	register int p1 = exp2c(vp, ignore);

#ifdef EDEBUG
	etraci("exp2b p1", p1, vp);
#endif
	if (**vp && eq(**vp, "&")) {
		register int p2;

		(*vp)++;
		p2 = exp2b(vp, ignore);
#ifdef EDEBUG
		etraci("exp2b p2", p2, vp);
#endif
		return (p1 & p2);
	}
	return (p1);
}

exp2c(vp, ignore)
	register CHAR ***vp;
	bool ignore;
{
	register CHAR *p1 = exp3(vp, ignore);
	register CHAR *p2, *tmp;
	register int i;

#ifdef EDEBUG
	etracc("exp2c p1", p1, vp);
#endif
	if (i = isa(**vp, EQOP)) {
		(*vp)++;
		if (i == EQMATCH || i == NOTEQMATCH)
			ignore |= NOGLOB;
		p2 = exp3(vp, ignore);
#ifdef EDEBUG
		etracc("exp2c p2", p2, vp);
#endif
		if (!(ignore&IGNORE)) switch (i) {

		case EQEQ:
			i = Eq(p1, p2);
			break;

		case NOTEQ:
			i = !Eq(p1, p2);
			break;

		case EQMATCH:
			i = Gmatch(p1, p2);
			break;

		case NOTEQMATCH:
			i = !Gmatch(p1, p2);
			break;
		}
		xfree(p1), xfree(p2);
		return (i);
	}
	i = egetn(p1);
	xfree(p1);
	return (i);
}

CHAR *
exp3(vp, ignore)
	register CHAR ***vp;
	bool ignore;
{
	register CHAR *p1, *p2;
	register int i;

	p1 = exp3a(vp, ignore);
#ifdef EDEBUG
	etracc("exp3 p1", p1, vp);
#endif
	if (i = isa(**vp, RELOP)) {
		(*vp)++;
		if (**vp && eq(**vp, "="))
			i |= 1, (*vp)++;
		p2 = exp3(vp, ignore);
#ifdef EDEBUG
		etracc("exp3 p2", p2, vp);
#endif
		if (!(ignore&IGNORE)) switch (i) {

		case GTR:
			i = egetn(p1) > egetn(p2);
			break;

		case GTR|1:
			i = egetn(p1) >= egetn(p2);
			break;

		case LSS:
			i = egetn(p1) < egetn(p2);
			break;

		case LSS|1:
			i = egetn(p1) <= egetn(p2);
			break;
		}
		xfree(p1), xfree(p2);
		return (putn(i));
	}
	return (p1);
}

CHAR *
exp3a(vp, ignore)
	register CHAR ***vp;
	bool ignore;
{
	register CHAR *p1, *p2, *op;
	register int i;

	p1 = exp4(vp, ignore);
#ifdef EDEBUG
	etracc("exp3a p1", p1, vp);
#endif
	op = **vp;
	if (op && any(op[0], "<>") && op[0] == op[1]) {
		(*vp)++;
		p2 = exp3a(vp, ignore);
#ifdef EDEBUG
		etracc("exp3a p2", p2, vp);
#endif
		if (op[0] == '<')
			i = egetn(p1) << egetn(p2);
		else
			i = egetn(p1) >> egetn(p2);
		xfree(p1), xfree(p2);
		return (putn(i));
	}
	return (p1);
}

CHAR *
exp4(vp, ignore)
	register CHAR ***vp;
	bool ignore;
{
	register CHAR *p1, *p2;
	register int i = 0;

	p1 = exp5(vp, ignore);
#ifdef EDEBUG
	etracc("exp4 p1", p1, vp);
#endif
	if (isa(**vp, ADDOP)) {
		register CHAR *op = *(*vp)++;

		p2 = exp4(vp, ignore);
#ifdef EDEBUG
		etracc("exp4 p2", p2, vp);
#endif
		if (!(ignore&IGNORE)) switch (op[0]) {

		case '+':
			i = egetn(p1) + egetn(p2);
			break;

		case '-':
			i = egetn(p1) - egetn(p2);
			break;
		}
		xfree(p1), xfree(p2);
		return (putn(i));
	}
	return (p1);
}

CHAR *
exp5(vp, ignore)
	register CHAR ***vp;
	bool ignore;
{
	register CHAR *p1, *p2;
	register int i = 0;

	p1 = exp6(vp, ignore);
#ifdef EDEBUG
	etracc("exp5 p1", p1, vp);
#endif
	if (isa(**vp, MULOP)) {
		register CHAR *op = *(*vp)++;

		p2 = exp5(vp, ignore);
#ifdef EDEBUG
		etracc("exp5 p2", p2, vp);
#endif
		if (!(ignore&IGNORE)) switch (op[0]) {

		case '*':
			i = egetn(p1) * egetn(p2);
			break;

		case '/':
			i = egetn(p2);
			if (i == 0)
				error((catgets(nlmsg_fd,NL_SETN,1, "Divide by 0")));
			i = egetn(p1) / i;
			break;

		case '%':
			i = egetn(p2);
			if (i == 0)
				error((catgets(nlmsg_fd,NL_SETN,2, "Mod by 0")));
			i = egetn(p1) % i;
			break;
		}
		xfree(p1), xfree(p2);
		return (putn(i));
	}
	return (p1);
}

#ifndef NONLS
CHAR CH_braces[] = {'{',' ','.','.','.',' ','}',0};
#else
#define CH_braces	"{ ... }"
#endif

CHAR *
exp6(vp, ignore)
	register CHAR ***vp;
{
	int ccode, i;
	register CHAR *cp, *dp, *ep;

	i = 0;    /* initialize auto variable */
	if (eq(**vp, "!")) {
		(*vp)++;
		cp = exp6(vp, ignore);
#ifdef EDEBUG
		etracc("exp6 ! cp", cp, vp);
#endif
		i = egetn(cp);
		xfree(cp);
		return (putn(!i));
	}
	if (eq(**vp, "~")) {
		(*vp)++;
		cp = exp6(vp, ignore);
#ifdef EDEBUG
		etracc("exp6 ~ cp", cp, vp);
#endif
		i = egetn(cp);
		xfree(cp);
		return (putn(~i));
	}
	if (eq(**vp, "(")) {
		(*vp)++;
		ccode = exp0(vp, ignore);
#ifdef EDEBUG
		etraci("exp6 () ccode", ccode, vp);
#endif
		if (*vp == 0 || **vp == 0 || ***vp != ')')
			bferr((catgets(nlmsg_fd,NL_SETN,3, "Expression syntax")));
		(*vp)++;
		return (putn(ccode));
	}
	if (eq(**vp, "{")) {
		register CHAR **v;
		struct command faket;
		CHAR *fakecom[2];

		faket.t_dtyp = TCOM;
		faket.t_dflg = 0;
		faket.t_dcar = faket.t_dcdr = faket.t_dspr = (struct command *)0;
		faket.t_dcom = fakecom;
		fakecom[0] = CH_braces;
		fakecom[1] = NOSTR;
		(*vp)++;
		v = *vp;
		for (;;) {
			if (!**vp)
				bferr((catgets(nlmsg_fd,NL_SETN,4, "Missing }")));
			if (eq(*(*vp)++, "}"))
				break;
		}
		if (ignore&IGNORE)
			return (nullstr);
		psavejob();
		if (pfork(&faket, -1) == 0) {
			*--(*vp) = 0;
			evalav(v);
			exitstat();
		}
		pwait();
		prestjob();
#ifdef EDEBUG
		etraci("exp6 {} status", egetn(value(CH_status)), vp);
#endif
		return (putn(egetn(value(CH_status)) == 0));
	}
	if (isa(**vp, ANYOP))
		return (nullstr);
	cp = *(*vp)++;
	if (*cp == '-' && any(cp[1], "erwxfdzo")) {
		struct stat stb;

		if (isa(**vp, ANYOP))
			bferr((catgets(nlmsg_fd,NL_SETN,5, "Missing file name")));
		dp = *(*vp)++;
		if (ignore&IGNORE)
			return (nullstr);
		ep = globone(dp);
		switch (cp[1]) {

		case 'r':
			i = !access(to_char(ep), 4);
			break;

		case 'w':
			i = !access(to_char(ep), 2);
			break;

		case 'x':
			i = !access(to_char(ep), 1);
			break;

		default:
			if (stat(to_char(ep), &stb)) {
				xfree(ep);
				return (CH_zero);
			}
			switch (cp[1]) {

			case 'f':
				i = (stb.st_mode & S_IFMT) == S_IFREG;
				break;

			case 'd':
				i = (stb.st_mode & S_IFMT) == S_IFDIR;
				break;

			case 'z':
				i = stb.st_size == 0;
				break;

			case 'e':
				i = 1;
				break;

			case 'o':
				i = stb.st_uid == uid;
				break;
			}
		}
#ifdef EDEBUG
		etraci("exp6 -? i", i, vp);
#endif
		xfree(ep);
		return (putn(i));
	}
#ifdef EDEBUG
	etracc("exp6 default", cp, vp);
#endif
	return (ignore&NOGLOB ? savestr(cp) : globone(cp));
}

evalav(v)
	register CHAR **v;
{
	struct wordent paraml;
	register struct wordent *hp = &paraml;
	struct command *t;
	register struct wordent *wdp = hp;
	
	set(CH_status, CH_zero);
	hp->prev = hp->next = hp;
	hp->word = nullstr;
	while (*v) {
		register struct wordent *new = (struct wordent *) calloc(1, sizeof *wdp);

		new->prev = wdp;
		new->next = hp;
		wdp->next = new;
		wdp = new;
		wdp->word = savestr(*v++);
	}
	hp->prev = wdp;
	alias(&paraml);
	t = syntax(paraml.next, &paraml, 0);
	if (err)
		error(err);
	execute(t, -1);
	freelex(&paraml), freesyn(t);
}

isa(cp, what)
	register CHAR *cp;
	register int what;
{

	if (cp == 0)
		return ((what & RESTOP) != 0);
	if (cp[1] == 0) {
		if ((what & ADDOP) && any(cp[0], "+-"))
			return (1);
		if ((what & MULOP) && any(cp[0], "*/%"))
			return (1);
		if ((what & RESTOP) && any(cp[0], "()!~^"))
			return (1);
	}
	if ((what & RESTOP) && (any(cp[0], "|&") || eq(cp, "<<") || eq(cp, ">>")))
		return (1);
	if (what & EQOP) {
		if (eq(cp, "=="))
			return (EQEQ);
		if (eq(cp, "!="))
			return (NOTEQ);
		if (eq(cp, "=~"))
			return (EQMATCH);
		if (eq(cp, "!~"))
			return (NOTEQMATCH);
	}
	if (!(what & RELOP))
		return (0);
	if (*cp == '<')
		return (LSS);
	if (*cp == '>')
		return (GTR);
	return (0);
}

egetn(cp)
	register CHAR *cp;
{

	if (*cp && *cp != '-' && !digit(*cp))
		bferr((catgets(nlmsg_fd,NL_SETN,6, "Expression syntax")));
	return (getn(cp));
}

/* Phew! */

#ifdef EDEBUG
etraci(str, i, vp)
	char *str;
	int i;
	CHAR ***vp;
{

	printf("%s=%d\t", str, i);
	blkpr(*vp);
	printf("\n");
}

etracc(str, cp, vp)
	char *str;
	CHAR *cp;
	CHAR ***vp;
{

	printf("%s=%s\t", str, to_char(cp));
	blkpr(*vp);
	printf("\n");
}
#endif
