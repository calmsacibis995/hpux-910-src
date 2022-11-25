/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

# ifndef lint
static char rcsid[] = "$Header: stab.c,v 1.8.109.3 95/02/21 16:08:49 mike Exp $";
# 	ifndef hpux
static char sccsid[] = "@(#)stab.c	5.7 (Berkeley) 6/1/90";
# 	endif	/* not hpux */
# endif /* not lint */

# include "sendmail.h"

/*
**  STAB -- manage the symbol table
**
**	Parameters:
**		name -- the name to be looked up or inserted.
**		type -- the type of symbol.
**		op -- what to do:
**			ST_ENTER -- enter the name if not
**				already present.
**			ST_FIND -- find it only.
**
**	Returns:
**		pointer to a STAB entry for this name.
**		NULL if not found and not entered.
**
**	Side Effects:
**		can update the symbol table.
*/

# define STABSIZE	400

static STAB	*SymTab[STABSIZE];

STAB *
stab(name, type, op)
	char *name;
	int type;
	int op;
{
	register STAB *s;
	register STAB **ps;
	register int hfunc;
	register char *p;
	extern char lower();

# ifdef DEBUG
	if (tTd(36, 5))
		printf("STAB: %s %d ", name, type);
#  endif /* DEBUG */

	/*
	**  Compute the hashing function
	**
	**	We could probably do better....
	*/

	hfunc = type;
	for (p = name; *p != '\0'; p++)
		hfunc = (((hfunc << 7) | lower(*p)) & 077777) % STABSIZE;

# ifdef DEBUG
	if (tTd(36, 9))
		printf("(hfunc=%d) ", hfunc);
#  endif /* DEBUG */

	ps = &SymTab[hfunc];
	while ((s = *ps) != NULL && (strcasecmp(name, s->s_name) || s->s_type != type))
		ps = &s->s_next;

	/*
	**  Dispose of the entry.
	*/

	if (s != NULL || op == ST_FIND)
	{
# ifdef DEBUG
		if (tTd(36, 5))
		{
			if (s == NULL)
				printf("not found\n");
			else
			{
				long *lp = (long *) s->s_class;

				printf("type %d val %lx %lx %lx %lx\n",
					s->s_type, lp[0], lp[1], lp[2], lp[3]);
			}
		}
#  endif /* DEBUG */
		return (s);
	}

	/*
	**  Make a new entry and link it in.
	*/

# ifdef DEBUG
	if (tTd(36, 5))
		printf("entered\n");
#  endif /* DEBUG */

	/* make new entry */
	s = (STAB *) xalloc(sizeof *s);
	bzero((char *) s, sizeof *s);
	s->s_name = newstr(name);
	makelower(s->s_name);
	s->s_type = type;

	/* link it in */
	*ps = s;

	return (s);
}
