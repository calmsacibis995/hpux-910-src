/* @(#) $Revision: 70.3 $ */   
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */


#include	"defs.h"
#include	<errno.h>

#ifdef NLS
#define NL_SETN 1
#endif


#define ARGMK	01
#define NULL 0

extern int	gsort();
extern char	*sysmsg[];
extern int	nsysmsg;
extern short topfd;
extern struct fdsave *fdmap;
extern int (*oldsigwinch)();

/*
 * service routines for `execute'
 */
initio(iop, save)
	struct ionod	*iop;
	int		save;
{
	register tchar	*ion;
	register int	iof, fd;
	int		ioufd;
	short	lastfd;

	lastfd = topfd;
	while (iop)
	{
		iof = iop->iofile;
		ion = mactrim(iop->ioname);
		ioufd = iof & IOUFD;

		if (*ion && (flags&noexec) == 0)
		{
			if (save)
			{
				fdmap[topfd].org_fd = ioufd;
				fdmap[topfd++].dup_fd = savefd(ioufd);
			}

			if (iof & IODOC)
			{
				struct tempblk tb;

				subst(chkopen(ion), (fd = tmpfil(&tb)));

				poptemp();	/* pushed in tmpfil() --
						   bug fix for problem with
						   in-line scripts
						*/

				fd = chkopen(to_tchar(tmpout));
				unlink(tmpout);
			}
			else if (iof & IOMOV)
			{
				if (eqtt(minus, ion))
				{
					fd = -1;
					close(ioufd);
				}
				else if ((fd = stoi(ion)) >= USERIO)
					tfailed(ion, nl_msg(623,badfile));
				else
					fd = dup(fd);
			}
			else if ((iof & IOPUT) == 0)
				fd = chkopen(ion);
			else if (flags & rshflg)
				tfailed(ion, nl_msg(614,restricted));
			else if (iof & IOAPP && (fd = open(to_char(ion), 1)) >= 0)
				lseek(fd, 0L, 2);
			else
				fd = create(ion);
			if (fd >= 0)
				rename(fd, ioufd);
		}

		iop = iop->ionxt;
	}
	return(lastfd);
}

tchar *
simple(s)
tchar	*s;
{
	tchar	*sname;

	sname = s;
	while (1)
	{
		if (any('/', sname))
			while (*sname++ != '/')
				;
		else
			return(sname);
	}
}

tchar *	
getpath(s)
	tchar	*s;
{
	register tchar	*path;

	if (any('/', s) || any(('/' | QUOTE), s))
	{
		if (flags & rshflg)
			tfailed(s, nl_msg(614,restricted));
		else
			return(nullstr);
	}
	else if ((path = pathnod.namval) == 0)
		return(defpath);
	else
		return(cpystak(path));
}

pathopen(path, name)
register tchar *path, *name;
{
	register int	f;
	char *open_str;		

	do
	{
		path = catpath(path, name);
		open_str = to_char(curstak());
	} while ((f = open(open_str, 0)) < 0 && path);
	return(f);
}

tchar *
catpath(path, name)
register tchar	*path;
tchar	*name;
{
	/*
	 * leaves result on top of stack
	 */
	register tchar	*scanp = path; 
	register tchar	*argp = locstak();

	sizechk(argp + tlength(path) + tlength(name));
	while (*scanp && *scanp != COLON)
		*argp++ = *scanp++;
	if (scanp != path && *(argp-1) != '/')
		*argp++ = '/';
	if (*scanp == COLON)
		scanp++;
	path = (*scanp ? scanp : 0);
	scanp = name;
	while ((*argp++ = *scanp++))
		;
	return(path);
}

tchar *	
nextpath(path)
	register tchar	*path;
{
	register tchar	*scanp = path;

	while (*scanp && *scanp != COLON)
		scanp++;

	if (*scanp == COLON)
		scanp++;

	return(*scanp ? scanp : 0);
}

static char	**xecenv, *xecmsg = NULL;

int	execa(at, pos)
	tchar	*at[];
	short pos;
{
	register tchar	*path;
	register tchar	**t = at;
	int		cnt;
	int		len;

#ifndef NLS 
#define c_t	t	
#else NLS 
	char **		c_t;
	cnt = blklen(t);
	len = (cnt + 1)*sizeof(tchar *);
	for (cnt--; cnt >= 0; cnt--)
		len += (tlength(t[cnt])+1)*sizeof(tchar);
	c_t = (char **)alloc(len);
	blk_to_char(t, c_t);
#endif NLS
	if ((flags & noexec) == 0)
	{
		path = getpath(*t);
		xecenv = setenv();

		if (pos > 0)
		{
			cnt = 1;
			while (cnt != pos)
			{
				++cnt;
				path = nextpath(path);
			}
			execs(path, t, c_t);
			path = getpath(*t);
		}
		while (path = execs(path, t, c_t))
			;
#ifdef DEBUG
		printf((nl_msg(801, "execa failed, t=%s\n")), *c_t);
#endif DEBUG
		if (xecmsg == NULL)
			tfailed(*t, nl_msg(622,notfound));
	        else
			tfailed(*t, xecmsg);
	}
}

#ifndef NLS
/*VARARGS2*/
static char *
execs(ap, t)
char	*ap;
register char	*t[];
{
	register char *p, *prefix;

	prefix = catpath(ap, t[0]);
	trim(p = curstak());
	sigchk();

	execve(p, t, xecenv);
	switch (errno)
	{
	case ENOEXEC:		/* could be a shell script */
		funcnt = 0;
		flags = 0;
		*flagadr = 0;
		comdiv = 0;
		ioset = 0;
		clearup();	/* remove open files and for loop junk */
		if (input)
			close(input);

		input = chkopen(p);
	
#ifdef ACCT
		preacct(p);	/* reset accounting */
#endif

		/*
		 * set up new args
		 */
		
		setargs(t);
		longjmp(subshell, 1);

	case EINVAL:
		tfailed(p, nl_msg(660,incomp_bin));

	case ENOMEM:
		tfailed(p, nl_msg(620,toobig));

	case E2BIG:
		tfailed(p, nl_msg(618,arglist));

	case ETXTBSY:
		tfailed(p, nl_msg(619,txtbsy));

	default:
		xecmsg = nl_msg(621,badexec);
	case ENOENT:
		return(prefix);
	}
}
#else NLS
static tchar *
execs(ap, t, c_t)
tchar	*ap;
register tchar	*t[];
register char	*c_t[];
{
	register tchar *p, *prefix;

	prefix = catpath(ap, t[0]);
	trim(p = curstak());
	sigchk();

	execve(to_char(p), c_t, xecenv);
	switch (errno)
	{
	case ENOEXEC:		/* could be a shell script */
		funcnt = 0;
		flags = 0;
		*flagadr = 0;
		comdiv = 0;
		ioset = 0;
		clearup();	/* remove open files and for loop junk */
		if (input)
			close(input);

		input = chkopen(p);
	
#ifdef ACCT
		preacct(p);	/* reset accounting */
#endif

		/*
		 * set up new args
		 */
		
#if 1
		free(c_t);
#endif
		setargs(t);
		longjmp(subshell, 1);

	case EINVAL:
		tfailed(p, nl_msg(660,incomp_bin));

	case ENOMEM:
		tfailed(p, nl_msg(620,toobig));

	case E2BIG:
		tfailed(p, nl_msg(618,arglist));

	case ETXTBSY:
		tfailed(p, nl_msg(619,txtbsy));

	default:
		xecmsg = (char *)nl_msg(621,badexec);
	case ENOENT:
		return(prefix);
	}
}
#endif NLS


/*
 * for processes to be waited for
 */
#define MAXP 20
static int	pwlist[MAXP];
static int	pwc;

postclr()
{
	register int	*pw = pwlist;

	while (pw <= &pwlist[pwc])
		*pw++ = 0;
	pwc = 0;
}

post(pcsid)
int	pcsid;
{
	register int	*pw = pwlist;

	if (pcsid)
	{
		while (*pw)
			pw++;
		if (pwc >= MAXP - 1)
			pw--;
		else
			pwc++;
		*pw = pcsid;
	}
}

await(i, bckg)
int	i, bckg;
{
	int	rc = 0, wx = 0;
	int	w;
	int	ipwc = pwc;

	if (i != 0 && ((kill(i,0) == -1) && (errno == ESRCH))){
					/* wait returns if process no longer */
		exitval = rc;		/* exists or was a bad pid. Bell bug */
		exitset();		/* now behaves the same as ksh       */
		return;
	}

	post(i);
	while (pwc)
	{
		register int	p;
		register int	sig;
		int		w_hi;
		int	found = 0;

		{
			register int	*pw = pwlist;

			p = wait(&w);
			if (wasintr)
			{
				wasintr = 0;
				if (bckg)
				{
					break;
				}
			}
			while (pw <= &pwlist[ipwc])
			{
				if (*pw == p)
				{
					*pw = 0;
					pwc--;
					found++;
				}
				else
					pw++;
			}
		}
		if (p == -1)
		{
			if (bckg)
			{
				register int *pw = pwlist;

				while (pw <= &pwlist[ipwc] && i != *pw)
					pw++;
				if (i == *pw)
				{
					*pw = 0;
					pwc--;
				}
			}
			continue;
		}
		w_hi = (w >> 8) & LOBYTE;
		if (sig = w & 0177)
		{
			if (sig == 0177)	/* ptrace! return */
			{
				prs((nl_msg(802, "ptrace: ")));
				sig = w_hi;
			}
			if (sig >= nsysmsg)
			{
 				prs(nl_msg(606,badsig));
 				prs(": ");
 				prn(sig);
			}
			else if (sysmsg[sig])
			{
				if (i != p || (flags & prompt) == 0)
				{
					prp();
					prn(p);
					blank();
				}
				prs(sysmsg[sig]);
				if (w & 0200)
					prs(nl_msg(617,coredump));
			}
			newline();
		}
		if (rc == 0 && found != 0)
			rc = (sig ? sig | SIGFLG : w_hi);
		wx |= w;
		if (p == i)
		{
			break;
		}
	}
	/* reinstate signal handler previoused saved from SIGWINCH */
        signal (SIGWINCH, oldsigwinch);
	if (wx && flags & errflg)
		exitsh(rc);
	flags |= eflag;
	exitval = rc;
	exitset();
}

BOOL		nosubst;
trim(at)
tchar	*at;
{
	register tchar	*p; 
	register tchar 	*ptr;
	register tchar	c;
	register tchar	q = 0;

	if (p = at)
	{
		ptr = p;
		while (c = *p++)
		{
			if (*ptr = c & STRIP)
				++ptr;
			q |= c;
		}

		*ptr = 0;
	}
	nosubst = q & QUOTE;
}

tchar *	
mactrim(s)
tchar	*s;
{
	register tchar	*t = macro(s);

	trim(t);
	return(t);
}


tchar **
scan(argn)
int	argn;
{
	register struct argnod *argp = (struct argnod *)(Rcheat(gchain) & ~ARGMK);
	register tchar **comargn, **comargm; 

	comargn = (tchar **)getstak(sizeof(tchar *) * argn + sizeof(tchar *));
	comargm = comargn += argn;
	*comargn = ENDARGS;
	while (argp)
	{
		*--comargn = argp->argval;

		trim(*comargn);
		argp = argp->argnxt;

		if (argp == 0 || Rcheat(argp) & ARGMK)
		{
			gsort(comargn, comargm);
			comargm = comargn;
		}
		/* Lcheat(argp) &= ~ARGMK; */
		argp = (struct argnod *)(Rcheat(argp) & ~ARGMK);
	}
	return(comargn);
}

static int
gsort(from, to)
tchar	*from[], *to[];
{
	int	k, m, n;
	register int	i, j;

	if ((n = to - from) <= 1)
		return;
	for (j = 1; j <= n; j *= 2)
		;
	for (m = 2 * j - 1; m /= 2; )
	{
		k = n - m;
		for (j = 0; j < k; j++)
		{
			for (i = j; i >= 0; i -= m)
			{
				register tchar **fromi;
				register char tmp[1024];

				fromi = &from[i];
				strcpy(tmp, to_char(fromi[m]));
				if (strcoll(tmp, to_char(fromi[0])) > 0)
				{
					break;
				}
				else
				{
					tchar *s;

					s = fromi[m];
					fromi[m] = fromi[0];
					fromi[0] = s;
				}
			}
		}
	}
}

/*
 * Argument list generation
 */
getarg(ac)
struct comnod	*ac;
{
	register struct argnod	*argp;
	register int		count = 0;
	register struct comnod	*c;

	if (c = ac)
	{
		argp = c->comarg;
		while (argp)
		{
			count += split(macro(argp->argval));
			argp = argp->argnxt;
		}
	}
	return(count);
}

static int
split(s)		/* blank interpretation routine */
register tchar	*s;
{
	register tchar	*argp;
	register int	c;
	int		count = 0;

	for (;;)
	{
		sigchk();
		argp = (tchar *)(Rcheat(locstak()) + BYTESPERWORD);
		sizechk(argp + tlength(s));
		while ((c = *s++, !any(c, ifsnod.namval) && c))
			*argp++ = c;
		if (argp == (tchar *)(Rcheat(staktop) + BYTESPERWORD))
		{
			if (c)
			{
				continue;
			}
			else
			{
				return(count);
			}
		}
		else if (c == 0)
			s--;
		/*
		 * file name generation
		 */

		argp = endstak(argp);

		if ((flags & nofngflg) == 0 && 
			(c = expand(((struct argnod *)argp)->argval, 0)))
			count += c;
		else
		{
			makearg(argp);
			count++;
		}
		gchain = (struct argnod *)((int)gchain | ARGMK);
	}
}

#ifdef ACCT
#include	<sys/types.h>
#include	"acctdef.h"
#include	<sys/acct.h>
#include 	<sys/times.h>

struct acct sabuf;
struct tms buffer;
extern clock_t times();
static clock_t before;
static int shaccton;	/* 0 implies do not write record on exit
			   1 implies write acct record on exit
			*/


/*
 *	suspend accounting until turned on by preacct()
 */

suspacct()
{
	shaccton = 0;
}

preacct(cmdadr)
	tchar *cmdadr;
{
	tchar *simple();

	if (acctnod.namval && *acctnod.namval)
	{
		sabuf.ac_btime = time((long *)0);
		before = times(&buffer);
		sabuf.ac_uid = getuid();
		sabuf.ac_gid = getgid();
		cmovstrn(to_char(simple(cmdadr)), sabuf.ac_comm, sizeof(sabuf.ac_comm));
		shaccton = 1;
	}
}

#ifndef O_NDELAY
#include	<fcntl.h>		/* include only if necessary, mn */
#endif

doacct()
{
	int fd;
	clock_t after;

	if (shaccton)
	{
		after = times(&buffer);
		sabuf.ac_utime = compress(buffer.tms_utime + buffer.tms_cutime);
		sabuf.ac_stime = compress(buffer.tms_stime + buffer.tms_cstime);
		sabuf.ac_etime = compress(after - before);

		if ((fd = open(to_char(acctnod.namval), O_WRONLY | O_APPEND | O_CREAT, 0666)) != -1)
		{
			write(fd, &sabuf, sizeof(sabuf));
			close(fd);
		}
	}
}

/*
 *	Produce a pseudo-floating point representation
 *	with 3 bits base-8 exponent, 13 bits fraction
 */

compress(t)
	register time_t t;
{
	register exp = 0;
	register rund = 0;

	while (t >= 8192)
	{
		exp++;
		rund = t & 04;
		t >>= 3;
	}

	if (rund)
	{
		t++;
		if (t >= 8192)
		{
			t >>= 3;
			exp++;
		}
	}

	return((exp << 13) + t);
}
#endif


#ifdef NLS   /* from here to end of file is only needed for NLS */

#ifndef NLS16
#undef ADVANCE
#define SCHARAT(s) (*(s)&0xff)
#define ADVANCE(s) (s++)
#else NLS16

#ifdef EUC
#define SCHARAT(s) (_CHARAT(s)&STRIP)
#else  EUC
#define SCHARAT(s) (CHARAT(s)&STRIP)
#endif EUC

#endif NLS16

#ifdef NLS
#define MCHARAT(s) ((*(s)&0xff)==0x80 ? (0x8000|(*(s+1)&0xff)) : SCHARAT(s))
#define MADVANCE(s) ((*(s)&0xff)==0x80 ? s+=2 : ADVANCE(s))
#endif NLS

tchar *
sto_tchar(s,tptr)
char *s;
tchar tptr[1024];
{
	register tchar *n;

	if (s) {
		n = tptr;
		while (*n++ = MCHARAT(s))
			MADVANCE(s);
		return(tptr);
	}
	else return ((tchar *) 0);
}


tchar *
to_tchar(s)
char *s;
{
	register tchar *n;
	static tchar m[1024];

	if (s) {
		n = m;
		/* properly convert kanji strings */
		while (*n++ = MCHARAT(s))
			MADVANCE(s);
		return(m);
	}
	else return ((tchar *) 0);
}

char *
sto_char(s,cptr)
tchar *s;
char cptr[1024];
{
	register char *n;

	if (s) {
		n = cptr;
		/* properly convert kanji strings */
		while (*s) {
			if (*s > 0377) {
				*n++ = (*s >> 8) | 0200; 
				*n++ = *s++ & 0377; 
			}
			else *n++ = *s++;
		}
		*n = 0;		/* null terminate */
		return(cptr);
	}
	else return ((char *) 0);
}

char *
to_char(s)
tchar *s;
{
	register char *n;
	static char m[2048];

	if (s) {
		n = m;
		while (*s) {
			if (n >= &m[2048-1])
				error((nl_msg(803, "sh internal 2K buffer overflow")));
			if (*s > 0377) {
				*n++ = (*s >> 8) | 0200; 
				*n++ = *s++ & 0377; 
			}
			else *n++ = *s++;
		}
		*n = 0;		/* null terminate */
		return(m);
	}
	else return ((char *) 0);
}

tchar **
blk_to_tchar(c, newv)
register char **c;
register tchar **newv;
{
	register int len = 0;
	tchar **onewv;
	register tchar *alloc_ptr;
	register char **c1;

	c1 = c;
	while (*c1++) len++;
	onewv = newv;
	alloc_ptr = (tchar *)(newv + len + 1);
	while (*c) {
		*newv = sto_tchar(*c, alloc_ptr);
		alloc_ptr += tlength(*newv);
		newv++;
		c++;
	}
	*newv = 0;
	return (onewv);
}

char **
blk_to_char(c, newv)
register tchar **c;
register char **newv;
{
	register int len = blklen(c) + 1;
	char **onewv;
	register char *alloc_ptr;

	onewv = newv;
	alloc_ptr = (char *)(newv + len);
	while (*c) {
		*newv = sto_char(*c, alloc_ptr);
		alloc_ptr += length(*newv);
		newv++;
		c++;
	}
	*newv = 0;
	return (onewv);
}

blklen(av)
	register tchar **av;
{
	register int i = 0;

	while (*av++)
		i++;
	return (i);
}

numchar(as, endptr)	/* NLS new routine returns actual number of bytes */
tchar *as, *endptr;
{
	int clen = 0;

	while (as < endptr) 
		if (*as++ > 0377)
			clen += 2;
		else
			clen++;
	return(clen);
}

tchar *
to_tcharn(s,clen)
char *s;
int *clen;
{
	register tchar *n;
	static tchar m[1024];
	char *olds;

	if (*clen >= 1024)
		error((nl_msg(804, "to_tcharn buffer overflow")));
	(n = m)[0] = 0;
	while (*clen > 0) {
		/* properly convert kanji strings */
		*n++ = MCHARAT(s);
		olds = s;
		MADVANCE(s);
		*clen -= s - olds;
	}
	*clen = n-m;
	return(m);
}

char *
to_charm(s, leftoff, len)
tchar *s, **leftoff;
int *len;
{
	register char *n;
	static char m[1024];

	if (s) {
		n = m;
		while (*s) {
			if (n >= m + 1024-1) {
			/* negative length to show continuation needed */
				*len = m - n;  
				*leftoff = s;
				return(m);
			}
			if (*s > 0377) {
				*n++ = (*s >> 8) | 0200; 
				*n++ = *s++ & 0377; 
			}
			else *n++ = *s++;
		}
		*n = 0;			/* null terminate */
		*len = n - m;  
		*leftoff = (tchar *)0;
		return(m);
	}
	else return ((char *) 0);
}

/* This function will concatenate a character onto a tchar string.
 * This is used for concatentating the alternate space character onto
 * the white space string.
 */

tchar *
cfcat(s1,s2)
register tchar *s1;
int s2;
{
	register tchar *os1;

	if (s2 == NULL)
		return(s1);
	os1 = s1;
	while (*s1++);
	--s1;
	*s1++ = (tchar) s2;
	*s1++ = 0;
	return(os1);
}



#endif NLS 		/* closes the ifdef for NLS only routines */
