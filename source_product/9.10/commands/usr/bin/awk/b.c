/* $Revision: 70.2 $ */
/*
Copyright (c) 1984, 19885, 1986, 1987 AT&T
	All Rights Reserved

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.

The copyright notice above does not evidence any
actual or intended publication of such source code.
*/

#define	DEBUG

#include <regex.h>
#include "awk.h"
#include <ctype.h>
#include <stdio.h>
#include "y.tab.h"


uchar	*patbeg;
int	patlen;
int	nosub = 0;

#define	NFA	513	/* cache this many dynamic fa's;NLS increased from 20 */
fa	*fatab[NFA];
int	nfatab	= 0;/* entries in fatab */
fa	*mkdfa();

fa *makedfa(s)	/* returns compiled nfa for reg expr s */
	uchar *s;
{
	int i, use, nuse;
	fa *pfa;

	if (compile_time)	/* a constant for sure */
		return mkdfa(s);
	for (i = 0; i < nfatab; i++)	/* is it there already? */
		if (strcmp(fatab[i]->restr,s) == 0) {
			fatab[i]->use++;
			return fatab[i];
	}
	pfa = mkdfa(s);
	if (nfatab < NFA) {	/* room for another */
		fatab[nfatab] = pfa;
		fatab[nfatab]->use = 1;
		nfatab++;
		return pfa;
	}
	use = fatab[0]->use;	/* replace least-recently used */
	nuse = 0;
	for (i = 1; i < nfatab; i++)
		if (fatab[i]->use < use) {
			use = fatab[i]->use;
			nuse = i;
		}
	freefa(fatab[nuse]);
	fatab[nuse] = pfa;
	pfa->use = 1;
	return pfa;
}

fa *mkdfa(s)	/* does the real work of making a dfa */
		/* actually makes an nfa through regex routines */
uchar *s;
{
	fa *f;
	int err;

	if ((f = (fa *) malloc(sizeof(fa))) == NULL)
		overflo("no room for fa");
	f->restr = tostring(s);
	if (err=regcomp(&(f->re),s,REG_EXTENDED|REG_NEWLINE|_REG_C_ESC|(REG_NOSUB*nosub)))
		rxerror(err);
	nosub=0;
	return f;
}


overflo(s)
	uchar *s;
{
	error(FATAL,ERR73, s);
}


member(c, s)	/* is c in s? */
	register uchar c, *s;
{
	if (_CS_SBYTE)
		return (strchr(s,c)?1:0);
	else { /* don't look inside kanji for single byte chars */
		while (*s) {
			if (FIRSTof2(*s)){
				s++;
				if (!iscntrl(*s) && !isspace(*s))
					s++;
			} else if (c == *s++)
				return(1);
		}
		return(0);
	}
}


match(f, p)
	register fa *f;
	register uchar *p;
{
	int i;

	i=xmatch(f,p,0);
	patbeg=(uchar *)0;
	patlen=0;
	return(i);
}

pmatch(f, p, eflags)
	register fa *f;
	register uchar *p;
	register int eflags;
{
	return(xmatch(f,p,eflags));
}

nematch(f, p)
	register fa *f;
	register uchar *p;
{
	return(xmatch(f,p,0));
}

xmatch(f, p, eflags)
	register fa *f;
	register uchar *p;
	register int eflags;
{
	regmatch_t	match[1];

	f->use++;
	if (regexec(&(f->re),p,1,match,eflags)==0) {
		patbeg=(uchar *)((int)p+match[0].rm_so);
		patlen=match[0].rm_eo-match[0].rm_so;
		return(1);
	}
	patbeg=p-1;
	patlen=0;
	return(0);
}


freefa(f)
	struct fa *f;
{
	if (f == NULL)
		return;
	regfree(&(f->re));
	free(f->restr);
	free(f);
}
