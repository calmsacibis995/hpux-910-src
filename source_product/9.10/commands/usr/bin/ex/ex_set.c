/* @(#) $Revision: 66.2 $ */    
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_temp.h"
#include "ex_tty.h"

/*
 * Set command.
 */
char	optname[ONMSZ];

#ifndef NONLS8 /* User messages */
# define	NL_SETN	9	/* set number */
# include	<msgbuf.h>
# undef	getchar
# undef	putchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

set()
{
	register char *cp;
	register struct option *op;
	register int c;
	bool no;
	extern short ospeed;
	bool prev_dblesc = value(DOUBLEESCAPE);

	setnoaddr();
	if (skipend()) {
		if (peekchar() != EOF)
			ignchar();
		propts();
		return;
	}
	do {
		cp = optname;
		do {

#ifndef	NLS16
			if (cp < &optname[ONMSZ - 2])
				*cp++ = getchar();
#else
			/*
			 * At least, a Kanji character (2 bytes) should
			 * be saved in cp.
			 */
			if (cp < &optname[ONMSZ - 2])	{
				*cp++ = getchar();
				if (IS_SECOND(peekchar()))
					*cp++ = getchar();
			}
#endif

#ifndef NONLS8 /* Character set features */
		} while ((peekchar() >= IS_MACRO_LOW_BOUND) && isalnum(peekchar() & TRIM));
#else NONLS8
		} while ((peekchar() >= IS_MACRO_LOW_BOUND) && isalnum(peekchar()));
#endif NONLS8

		*cp = 0;
		cp = optname;
		if (eq("all", cp)) {
			if (inopen)
				pofix();
			prall();
			goto next;
		}
		no = 0;
		if (cp[0] == 'n' && cp[1] == 'o' && strcmp(cp, "novice")) {
			cp += 2;
			no++;
		}

		/*
		 * '!' suffix distinguishes between command mode "keyboardedit" and
		 * input mode "keyboardedit!"
		 */
		if (!strcmp(cp, options[KEYBOARDEDIT].oname) && peekchar() == '!') {
			*(cp+12) = getchar();
			*(cp+13) = 0;
		}
		/*
		 * If the abbreviations "ke" and "ke!" are approved someday ...
		else if (!strcmp(cp, options[KEYBOARDEDIT].oabbrev) && peekchar() == '!') {
			*(cp+2) = getchar();
			*(cp+3) = 0;
		}
		 *
		 */

		/* Implement w300, w1200, and w9600 specially */
		if (eq(cp, "w300")) {
			if (ospeed >= B1200) {
dontset:
				ignore(getchar());	/* = */
				ignore(getnum());	/* value */
				continue;
			}
			cp = "window";
		} else if (eq(cp, "w1200")) {
			if (ospeed < B1200 || ospeed >= B2400)
				goto dontset;
			cp = "window";
		} else if (eq(cp, "w9600")) {
			if (ospeed < B2400)
				goto dontset;
			cp = "window";
		}
		for (op = options; op < &options[NOPTS]; op++)
			if (eq(op->oname, cp) || op->oabbrev && eq(op->oabbrev, cp))
				break;
		if (op->oname == 0)

#ifndef NONLS8 /* User messages */
			serror((nl_msg(1, "%s: No such option|%s: No such option - 'set all' gives all option values")), cp);
#else NONLS8
			serror("%s: No such option@- 'set all' gives all option values", cp);
#endif NONLS8

		c = skipwh();
		if (peekchar() == '?') {
			ignchar();
printone:
			propt(op);
			noonl();
			goto next;
		}
		if (op->otype == ONOFF) {
			op->ovalue = 1 - no;
			if (op == &options[PROMPT])
				oprompt = 1 - no;
			goto next;
		}
		if (no)
			serror((nl_msg(2, "Option %s is not a toggle")), op->oname);
		if (c != 0 || setend())
			goto printone;
		if (getchar() != '=')

#ifndef NONLS8 /* User messages */
			serror((nl_msg(3, "Missing =|Missing = in assignment to option %s")), op->oname);
#else NONLS8
			serror("Missing =@in assignment to option %s", op->oname);
#endif NONLS8

		switch (op->otype) {

		case NUMERIC:

#ifndef NONLS8 /* Character set features */
			if ((peekchar() < IS_MACRO_LOW_BOUND) || !isdigit(peekchar() & TRIM))
#else NONLS8
			if ((peekchar() < IS_MACRO_LOW_BOUND) || !isdigit(peekchar()))
#endif NONLS8


#ifndef NONLS8 /* User messages */
				error((nl_msg(4, "Digits required|Digits required after =")));
#else NONLS8
				error("Digits required@after =");
#endif NONLS8

			op->ovalue = getnum();
			if (value(TABSTOP) <= 0)
				value(TABSTOP) = TABS;
#ifndef ED1000
			if (value(HARDTABS) <= 0)
				value(HARDTABS) = 8;  /*can't set to 0. If so,
							set to default value*/
#endif ED1000

			if (value(TIMEOUTLEN) <= 0)
				value(TIMEOUTLEN) = 1;	/* has to be greater than zero */

			if (op == &options[WINDOW]) {
				if (value(WINDOW) >= lines)
					value(WINDOW) = lines-1;
				vsetsiz(value(WINDOW));
			}
			break;

		case STRING:
		case OTERM:
			cp = optname;
			while (!setend()) {
				if (cp >= &optname[ONMSZ])

#ifndef NONLS8 /* User messages */
					error((nl_msg(5, "String too long|String too long in option assignment")));
#else NONLS8
					error("String too long@in option assignment");
#endif NONLS8

				/* adb change:  allow whitepace in strings */

#ifndef	NLS16
				if( (*cp = getchar()) == '\\')
					if( peekchar() != EOF)
						*cp = getchar();
				cp++;
#else
				/* prevent mistaking 2nd byte of 16-bit character
				** for a ANK character.
				*/
				if((c = getchar()) == '\\')
					if(peekchar() != EOF)
						c = getchar();
				*cp++ = c;
#endif

			}
			*cp = 0;
			if (op->otype == OTERM) {
/*
 * At first glance it seems like we shouldn't care if the terminal type
 * is changed inside visual mode, as long as we assume the screen is
 * a mess and redraw it. However, it's a much harder problem than that.
 * If you happen to change from 1 crt to another that both have the same
 * size screen, it's OK. But if the screen size if different, the stuff
 * that gets initialized in vop() will be wrong. This could be overcome
 * by redoing the initialization, e.g. making the first 90% of vop into
 * a subroutine. However, the most useful case is where you forgot to do
 * a setenv before you went into the editor and it thinks you're on a dumb
 * terminal. Ex treats this like hardcopy and goes into HARDOPEN mode.
 * This loses because the first part of vop calls oop in this case.
 * The problem is so hard I gave up. I'm not saying it can't be done,
 * but I am saying it probably isn't worth the effort.
 */
				if (inopen)
error((nl_msg(6, "Can't change type of terminal from within open/visual")));
				setterm(optname);
			} else {
				CP(op->osvalue, optname);
				op->odefault = 1;
			}
			if (dir_chg && op == &options[DIRECTORY])
				init();		/* init again if user changing dir during initializations */
			break;
		}
next:
		flush();
	} while (!skipend());

	/*
	 * If the DOUBLEESCAPE option changed value, update the terminfo mappings
	 */
	if (prev_dblesc != value(DOUBLEESCAPE)) {
		char *p, *q;
		int x, y;
		if (value(DOUBLEESCAPE)) {
			/* add an extra escape to each mapping */
			p = doublespace;
			for (x=0; x<immacs_start; x++) {
				q = p;
				for (y=0; immacs[x].mapto[y]; y++) {
					if (immacs[x].mapto[y] == ESCAPE)
						*q++ = ESCAPE;
					*q++ = immacs[x].mapto[y];
				}
				*q = '\0';
				immacs[x].mapto = p;
				p = q + 1;
			}
		} else {
			/* remove the extra escape from each mapping */
			p = allocspace;
			for (x=0; x<immacs_start; x++) {
				q = p;
				for (y=0; immacs[x].mapto[y]; y++) {
					if (immacs[x].mapto[y] == ESCAPE)
						y++;
					*q++ = immacs[x].mapto[y];
				}
				*q = '\0';
				immacs[x].mapto = p;
				p = q + 1;
			}
		}
	}
	
	eol();
}

setend()
{

	return (iswhite(peekchar()) || endcmd(peekchar()));
}

prall()
{
	register int incr = (NOPTS + 2) / 3;
	register int rows = incr;
	register struct option *op = options;

	for (; rows; rows--, op++) {
		propt(op);
		gotab(20);
		propt(&op[incr]);
		if (&op[2*incr] < &options[NOPTS]) {
			gotab(56);
			propt(&op[2 * incr]);
		}
		putNFL();
	}
}

propts()
{
	register struct option *op;

	for (op = options; op < &options[NOPTS]; op++) {
		if (op == &options[TTYTYPE])
			continue;
		switch (op->otype) {

		case ONOFF:
		case NUMERIC:
			if (op->ovalue == op->odefault)
				continue;
			break;

		case STRING:
			if (op->odefault == 0)
				continue;
			break;
		}
		propt(op);

#if defined NLS || defined NLS16
	RL_OKEY
#endif
		putchar(' ');
#if defined NLS || defined NLS16
	flush();
	RL_OSCREEN
#endif

	}
	noonl();
	flush();
}

propt(op)
	register struct option *op;
{
	register char *name;
	
	name = op->oname;

#if defined NLS || defined NLS16
	RL_OKEY
#endif

	switch (op->otype) {

	case ONOFF:
		printf("%s%s", op->ovalue ? "" : "no", name);
		break;

	case NUMERIC:
		printf("%s=%d", name, op->ovalue);
		break;

	case STRING:
	case OTERM:
		printf("%s=%s", name, op->osvalue);
		break;
	}

#if defined NLS || defined NLS16
	flush();
	RL_OSCREEN
#endif

}
