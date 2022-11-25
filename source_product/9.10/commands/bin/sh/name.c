/* @(#) $Revision: 72.1 $ */      
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	<sys/termio.h>

#ifdef NLS || NLS16
#include	<locale.h>
#endif NLS || NLS16

extern BOOL	chkid();
extern char	*simple();		/* not used.  mn */
extern int	mailchk;

struct tnamnod ps2nod =
{
	(struct tnamnod *)NIL,
	&acctnod,
	ps2name
};
struct tnamnod cdpnod = 
{
	(struct tnamnod *)NIL,
	(struct tnamnod *)NIL,
	cdpname
};
struct tnamnod pathnod =
{
	&mailpnod,
	(struct tnamnod *)NIL,
	pathname
};
struct tnamnod ifsnod =
{
	&homenod,
	&mailnod,
	ifsname
};
struct tnamnod ps1nod =
{
	&pathnod,
	&ps2nod,
	ps1name
};
struct tnamnod homenod =
{
	&cdpnod,
	(struct tnamnod *)NIL,
	homename
};
struct tnamnod mailnod =
{
	(struct tnamnod *)NIL,
	(struct tnamnod *)NIL,
	mailname
};
struct tnamnod mchknod =
{
	&ifsnod,
	&ps1nod,
	mchkname
};
struct tnamnod acctnod =
{
	(struct tnamnod *)NIL,
	(struct tnamnod *)NIL,
	acctname
};
struct tnamnod mailpnod =
{
	(struct tnamnod *)NIL,
	(struct tnamnod *)NIL,
	mailpname
};


struct tnamnod *namep = &mchknod;

/* ========	variable and string handling	======== */

syslook(w, syswds, n)
	register tchar *w;
	register struct sysnod syswds[];
	int n;
{
	int	low;
	int	high;
	int	mid;
	register int cond;

	if (w == 0 || *w == 0)
		return(0);

	low = 0;
	high = n - 1;

	while (low <= high)
	{
		mid = (low + high) / 2;

		if ((cond = cf(w, syswds[mid].sysnam)) < 0)
			high = mid - 1;
		else if (cond > 0)
			low = mid + 1;
		else
			return(syswds[mid].sysval);
	}
	return(0);
}

setlist(arg, xp)
register struct argnod *arg;
int	xp;
{
	if (flags & exportflg)
		xp |= N_EXPORT;

	while (arg)
	{
		register tchar *s = mactrim(arg->argval);
		setname(s, xp);
		arg = arg->argnxt;
		if (flags & execpr)
		{
			prst(s);
			if (arg)
				blank();
			else
				newline();
		}
	}
}


setname(argi, xp)	/* does parameter assignments */
tchar	*argi;		
int	xp;
{
#ifdef NLS16
	char *langname;
#endif NLS16
	register tchar *argscan = argi;		
	register struct tnamnod *n;	

	if (letter(*argscan)) 
	{
		while (alphanum(*argscan)) 
			argscan++;

		if (*argscan == '=')
		{
			*argscan = 0;	/* make name a cohesive string */

			n = lookup(argi);
			*argscan++ = '=';
			attrib(n, xp);
			if (xp & N_ENVNAM)
				n->namenv = n->namval = argscan;
			else
				assign(n, argscan);
			return;
		}
	}
	tfailed(argi, nl_msg(628,notid));
}

replace(a, v)
register tchar	**a;
tchar	*v;
{
	free(*a);
	*a = make(v);
}

dfault(n, v)
struct tnamnod *n;
tchar	*v;		
{
	if (n->namval == 0)
		assign(n, v);
}

assign(n, v)
struct tnamnod *n;
tchar	*v;
{

#ifdef NLS16

/* If LANG or LC_* environment variable is being assigned, execute
   a setlocale to update the shells international environment */

	char *langname;

	if (eqtt(n->namid,langvar)) {
		langname=to_char(v);
		setlocale(LC_ALL, langname);
		sprintf(langpath,"%s%s/sh.cat",NLSDIR,langname);
	}
	if (eqtt(n->namid,lccollate))
		setlocale(LC_COLLATE, to_char(v));
	if (eqtt(n->namid,lcctype))
		setlocale(LC_CTYPE, to_char(v));
	if (eqtt(n->namid,lcmonetary))
		setlocale(LC_MONETARY, to_char(v));
	if (eqtt(n->namid,lcnumeric))
		setlocale(LC_NUMERIC, to_char(v));
	if (eqtt(n->namid,lctime))
		setlocale(LC_TIME, to_char(v));
#endif
	if (n->namflg & N_RDONLY)
		tfailed(n->namid, nl_msg(627,wtfailed));

#ifndef RES

	else if (flags & rshflg)
	{
		if (n == &pathnod || eqtt(n->namid, shell))
			tfailed(n->namid, nl_msg(614,restricted));
	}

#endif

	else if (n->namflg & N_FUNCTN)
	{
		func_unhash(n->namid);
		freefunc(n);

		n->namenv = 0;
		n->namflg = N_DEFAULT;
	}

	if (n == &mchknod)
	{
		mailchk = stoi(v);
	}
		
	replace(&n->namval, v);
	attrib(n, N_ENVCHG);

	if (n == &pathnod)
	{
		zaphash();
		set_dotpath();
		return;
	}
	
	if (flags & prompt)
	{
		if ((n == &mailpnod) || (n == &mailnod && mailpnod.namflg == N_DEFAULT))
			setmail(n->namval);
	}
}

readvar(names)
tchar	**names;
{
	struct fileblk	fb;
	register struct fileblk *f = &fb;
	register tchar	c;
	register int	rc = 0;
	int	nbytes = 0;		/* NLS  for number of bytes */
	struct tnamnod *n = lookup(*names++);	/* done now to avoid storage mess */
	char	*rel = (char *)relstak();

	push(f);
	initf(dup(0));

	if (lseek(0, 0L, 1) == -1)
		f->fsiz = 1;

	/*
	 * strip leading IFS characters
	 */
	while ((any((c = nextc(0)), ifsnod.namval)) && !(eolchar(c))) 
			;
	for (;;)
	{
		if ((*names && any(c, ifsnod.namval)) || eolchar(c))
		{
			zerostak();
			assign(n, absstak(rel));
			setstak(rel);
			if (*names)	
				n = lookup(*names++);
			else
				n = 0;
			if (eolchar(c))
			{
				break;
			}
			else		/* strip imbedded IFS characters */
			{
				while ((any((c = nextc(0)), ifsnod.namval)) &&
					!(eolchar(c)))
					;
			}
		}
		else
		{
			pushstak(c);
			c = nextc(0);

			if (eolchar(c))
			{
				tchar *top = staktop;
			
				while (any(*(--top), ifsnod.namval))
					;
				staktop = top + 1;
			}


		}
	}
	while (n)
	{
		assign(n, nullstr);
		if (*names)	
			n = lookup(*names++);
		else
			n = 0;
	}

	if (eof)
		rc = 1;
#ifndef NLS
	lseek(0, (long)(f->fnxt - f->fend), 1);
#else NLS
	nbytes = numchar(f->fnxt, f->fend);
	lseek(0, (long)0-nbytes, 1);
#endif NLS
	pop();
	return(rc);
}

assnum(p, i)
tchar	**p;
int	i;
{
	itos(i);
	replace(p, numbuf);
}

tchar *
make(v)	
tchar	*v;
{
	register tchar	*p;

	if (v)
	{
		tmovstr(v, p = alloc(CHARSIZE*tlength(v)));
		return(p);
	}
	else
		return(0);
}


struct tnamnod *
lookup(nam)
	register tchar	*nam;
{
	register struct tnamnod *nscan = namep;
	register struct tnamnod **prev;
	int		LR;

	if (!chkid(nam))
		tfailed(nam, nl_msg(628,notid));
	while (nscan)
	{
		if ((LR = cf(nam, nscan->namid)) == 0)
			return(nscan);

		else if (LR < 0)
			prev = &(nscan->namlft);
		else
			prev = &(nscan->namrgt);
		nscan = *prev;
	}
	/*
	 * add name node
	 */
	nscan = (struct tnamnod *)alloc(sizeof *nscan);
	nscan->namlft = nscan->namrgt = (struct tnamnod *)NIL;
	nscan->namid = make(nam);
	nscan->namval = 0;
	nscan->namflg = N_DEFAULT;
	nscan->namenv = 0;

	return(*prev = nscan);
}

BOOL
chkid(nam)
tchar	*nam;
{
	register tchar *cp = nam;

	if (cp == 0 || *cp == 0)
		return(FALSE);

	if (!letter(*cp))
		return(FALSE);
	else
	{
		while (*++cp)
		{
			if (!alphanum(*cp))
				return(FALSE);
		}
	}
	return(TRUE);
}

static int (*namfn)();
namscan(fn)
	int	(*fn)();
{
	namfn = fn;
	namwalk(namep);
}

static int
namwalk(np)
register struct tnamnod *np;
{
	if (np)
	{
		namwalk(np->namlft);
		(*namfn)(np);
		namwalk(np->namrgt);
	}
}

printnam(n)
struct tnamnod *n;
{
	register tchar	*s;

	sigchk();

	if (n->namflg & N_FUNCTN)
	{
		prst_buff(n->namid);
		prs_buff("(){\n");
		prf(n->namenv);	
		prs_buff("\n}\n");
	}
	else if (s = n->namval)
	{
		prst_buff(n->namid);
		prc_buff('=');
		prst_buff(s);
		prc_buff(NL);
	}
}

static char *
staknam(n)
register struct tnamnod *n;
{
	register char	*p;
/*
 * namval can be a very long string (greater than 1K), we may need to write 
 * in several pieces.  So now use to_charm the multiple version of to_char.
 */
	extern char *to_charm(); 
	tchar	*l_namval = n->namval;	/* local varible for n->namval NLS */
	tchar	*leftoff = 0;
	int	startlen = -1;

#ifndef NLS
	sizechk(staktop + strlen(n->namid)+strlen(n->namval) + 2);
#else NLS

	/*
	 * Allocate enough space for the character translated version
	 * of namid and namval.  Thus we divide by sizeof tchar.
	 */
	sizechk(staktop + (tlength(n->namid)+tlength(n->namval)+2)/CHARSIZE);
#endif NLS
	p = movstr(to_char(n->namid), (char *)staktop);
	p = movstr("=",p);
#ifndef NLS
	p = movstr(l_namval,p);
#else NLS
	while (l_namval && startlen < 0) {
		p=movstr(to_charm(l_namval, &leftoff, &startlen),p);
		if (leftoff != 0)
			l_namval = leftoff;
	}
#endif NLS
	return((char *)getstak(p + 1 - (char *)stakbot));
}

static int namec;

exname(n)
	register struct tnamnod *n;
{
	register int 	flg = n->namflg;

	if (flg & N_ENVCHG)
	{

		if (flg & N_EXPORT)
		{
			free(n->namenv);
			n->namenv = make(n->namval);
		}
		else
		{
			free(n->namval);
			n->namval = make(n->namenv);
		}
	}

	
	if (!(flg & N_FUNCTN))
		n->namflg = N_DEFAULT;

	if (n->namval)
		namec++;

}

printro(n)
register struct tnamnod *n;
{
	if (n->namflg & N_RDONLY)
	{
		prs_buff(readonly);
		prc_buff(SP);
		prst_buff(n->namid);
		prc_buff(NL);
	}
}

printexp(n)
register struct tnamnod *n;
{
	if (n->namflg & N_EXPORT)
	{
		prs_buff(export);
		prc_buff(SP);
		prst_buff(n->namid);
		prc_buff(NL);
	}
}

setup_env(e)
register tchar **e;
{
	register int i;	

	if (e)
		for (i = 0;e[i]; i++) {
			setname(e[i], N_ENVNAM);
		}

}


static char **argnam;

pushnam(n)
struct tnamnod *n;
{
	if (n->namval)
		*argnam++ = (char *)staknam(n);
}

char **
setenv()
{
	register char	**er;

	namec = 0;
	namscan(exname);

	argnam = er = (char **)getstak(namec * BYTESPERWORD + BYTESPERWORD);
	namscan(pushnam);
	*argnam++ = 0;
	return(er);
}

struct tnamnod *
findnam(nam)
	register tchar	*nam;
{
	register struct tnamnod *nscan = namep;
	int		LR;

	if (!chkid(nam))
		return(0);
	while (nscan)
	{
		if ((LR = cf(nam, nscan->namid)) == 0)
			return(nscan);
		else if (LR < 0)
			nscan = nscan->namlft;
		else
			nscan = nscan->namrgt;
	}
	return(0); 
}


unset_name(name)
	register tchar 	*name;
{
	register struct tnamnod	*n;

	if (n = findnam(name))
	{
		if (n->namflg & N_RDONLY)
			tfailed(name, nl_msg(627,wtfailed));

		if (n == &pathnod ||
		    n == &ifsnod ||
		    n == &ps1nod ||
		    n == &ps2nod ||
		    n == &mchknod)
		{
			tfailed(name, nl_msg(632,badunset));
		}

#ifndef RES

		if ((flags & rshflg) && eqtt(name, shell))
			tfailed(name, nl_msg(614,restricted));

#endif

		if (n->namflg & N_FUNCTN)
		{
			func_unhash(name);
			freefunc(n);
		}
		else
		{
			free(n->namval);
			free(n->namenv);
		}

		n->namval = n->namenv = 0;
		n->namflg = N_DEFAULT;

		if (flags & prompt)
		{
			if (n == &mailpnod)
				setmail(mailnod.namval);
			else if (n == &mailnod && mailpnod.namflg == N_DEFAULT)
				setmail(0);
		}
		if (eqtt(n->namid,langvar)) {
			setlocale(LC_ALL,"C");
			sprintf(langpath,"%s%s/sh.cat",NLSDIR,"C");
		}
		if (eqtt(n->namid,lccollate))
			setlocale(LC_COLLATE,"C");
		if (eqtt(n->namid,lcctype))
			setlocale(LC_CTYPE,"C");
		if (eqtt(n->namid,lcmonetary))
			setlocale(LC_MONETARY,"C");
		if (eqtt(n->namid,lcnumeric))
			setlocale(LC_NUMERIC,"C");
		if (eqtt(n->namid,lctime))
			setlocale(LC_TIME,"C");
	}
}

tchar   lines_str[] = { 'L', 'I', 'N', 'E', 'S', 0 };
tchar   cols_str[]  = { 'C', 'O', 'L', 'U', 'M', 'N', 'S', 0 };
struct winsize cur_wsize;

void
set_l_and_c(lval, cval)
int lval, cval;
{
	struct tnamnod *n, *findnam(), *lookup();

	if(cur_wsize.ws_row != lval) {
		/* update the LINES env variable, even if it was unset */
        	if((n = findnam(lines_str)) == (struct tnamnod *) 0)
			n = lookup(lines_str);	/* add name to env */
		itos(lval);
		assign(n, numbuf);
		attrib(n, N_EXPORT);
		cur_wsize.ws_row = lval;
        }

	if(cur_wsize.ws_col != cval) {
		/* update the COLUMNS env variable even if it was unset */
		if((n = findnam(cols_str)) == (struct tnamnod *)0 )
			n = lookup(cols_str);	/* add name to env */
		itos(cval);
		assign(n, numbuf);
		attrib(n, N_EXPORT);
		cur_wsize.ws_col = cval;
        }
}

/* Debugging routine which I'm commenting out until I need.
   Bill Gates   7/17/91

void
print_string(s)
tchar *s;
{
    int i;
    char c;

    for(i=0; s[i] != 0; i++) {
        c = s[i];
        write(1, &c, 1);
    }
    write(1, "\n", 1);
}*/
