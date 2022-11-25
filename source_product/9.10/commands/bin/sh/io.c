/* @(#) $Revision: 66.2 $ */    
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	"dup.h"
#include	<fcntl.h>


short topfd;
extern struct fdsave *fdmap;

/* ========	input output and file copying ======== */

initf(fd)
int	fd;
{
	register struct fileblk *f = standin;

	f->fdes = fd;
	f->fsiz = ((flags & oneflg) == 0 ? BUFSIZ : 1);
	f->fnxt = f->fend = f->fbuf;
	f->feval = 0;
	f->flin = 1;
	f->feof = FALSE;
}

estabf(s)
register tchar *s; 	
{
	register struct fileblk *f;

	(f = standin)->fdes = -1;
	f->fend = tlength(s) + (f->fnxt = s);	
	f->flin = 1;
	return(f->feof = (s == 0));
}

push(af)
struct fileblk *af;
{
	register struct fileblk *f;

	(f = af)->fstak = standin;
	f->feof = 0;
	f->feval = 0;
	standin = f;
}

pop()
{
	register struct fileblk *f;

	if ((f = standin)->fstak)
	{
		if (f->fdes >= 0)
			close(f->fdes);
		standin = f->fstak;
		return(TRUE);
	}
	else
		return(FALSE);
}

struct tempblk *tmpfptr;

pushtemp(fd,tb)
	int fd;
	struct tempblk *tb;
{
	tb->fdes = fd;
	tb->fstak = tmpfptr;
	tmpfptr = tb;
}

poptemp()
{
	if (tmpfptr)
	{
		close(tmpfptr->fdes);
		tmpfptr = tmpfptr->fstak;
		return(TRUE);
	}
	else
		return(FALSE);
}
	
chkpipe(pv)
int	*pv;
{
	if (pipe(pv) < 0 || pv[INPIPE] < 0 || pv[OTPIPE] < 0)
		error(nl_msg(615,piperr));
}

chkopen(t_idf)
tchar *t_idf;
{
	register int	rc;

	if ((rc = open(to_char(t_idf), 0)) < 0)	
		tfailed(t_idf, nl_msg(616,badopen));
	return(rc);
}

rename(f1, f2)
register int	f1, f2;
{
#ifdef RES
	if (f1 != f2)
	{
		dup(f1 | DUPFLG, f2);
		close(f1);
		if (f2 == 0)
			ioset |= 1;
	}
#else
	int	fs;

	if (f1 != f2)
	{
		fs = fcntl(f2, 1, 0);
		close(f2);
		fcntl(f1, 0, f2);
		close(f1);
		if (fs == 1)
			fcntl(f2, 2, 1);
		if (f2 == 0)
			ioset |= 1;
	}
#endif
}

create(s)
tchar *s;
{
	register int	rc;

	if ((rc = creat(to_char(s), 0666)) < 0)
		tfailed(s, nl_msg(611,badcreate));
	return(rc);
}

tmpfil(tb)
	struct tempblk *tb;
{
	int fd;

	itos(serial++);
	movstr(to_char(numbuf), tmpnam);
	fd = create(to_tchar(tmpout));
	pushtemp(fd,tb);
	return(fd);
}

/*
 * set by trim
 */
extern BOOL		nosubst;
#define			CPYSIZ		512


copy(ioparg)
struct ionod	*ioparg;
{
	register tchar	*cline;
	register tchar	*clinep;
	register struct ionod	*iop;
	tchar	c;
	tchar	*ends;
	tchar	*start;
#ifdef NLS
	char	*c_start;	/* local varible for non-tchar copy NLS */
	tchar	*leftoff = 0;
	int	startlen;
	tchar	*savstart;
#endif NLS
	int		fd;
	int		i;
	int		stripflg;
	

	if (iop = ioparg)
	{
		struct tempblk tb;

		copy(iop->iolst);
		ends = mactrim(iop->ioname);
		stripflg = iop->iofile & IOSTRIP;
		if (nosubst)
			iop->iofile &= ~IODOC;
		fd = tmpfil(&tb);

		if (fndef)
			iop->ioname = make(to_tchar(tmpout));
		else
			iop->ioname = cpystak(to_tchar(tmpout));

		iop->iolst = iotemp;
		iotemp = iop;

		cline = clinep = start = locstak();
		if (stripflg)
		{
			iop->iofile &= ~IOSTRIP;
			while (*ends == '\t')
				ends++;
		}
		for (;;)
		{
			chkpr();
			if (nosubst)
			{
				c = readc();
				if (stripflg)
					while (c == '\t')
						c = readc();

				while (!eolchar(c))
				{
#ifndef NLS
					sizechk(clinep);
#else NLS
					sizechk(clinep + CHARSIZE);
#endif NLS
					*clinep++ = c;
					c = readc();
				}
			}
			else
			{
				c = nextc(*ends);
				if (stripflg)
					while (c == '\t')
						c = nextc(*ends);
				
				while (!eolchar(c))
				{
#ifndef NLS
					sizechk(clinep);
#else NLS
					sizechk(clinep + CHARSIZE);
#endif NLS
					*clinep++ = c;
					c = nextc(*ends);
				}
			}

			*clinep = 0;
			if (eof || eqtt(cline, ends))
			{
				if ((i = cline - start) > 0){
#ifndef NLS
					write(fd, start, i);
#else NLS
					extern char *to_charm(); 
					*cline = 0;
					savstart = start;
					startlen = -1;

				    while (startlen < 0) {
				    c_start = to_charm(start, &leftoff, &startlen);
				    write(fd, c_start, abs(startlen));
				    if (leftoff != 0)
					start = leftoff;
				}
				start = savstart;   /* restore orig start */
#endif NLS
				}
				break;
			}
			else
			{
#ifndef NLS
				sizechk(clinep);
#else NLS
				sizechk(clinep + CHARSIZE);
#endif NLS
				*clinep++ = NL;
			}

			if ((i = clinep - start) < CPYSIZ)
				cline = clinep;
			else
			{
#ifndef NLS
				write(fd, start, i);
#else NLS
				extern char *to_charm(); 
				*clinep = 0;
				savstart = start;
				startlen = -1;
				while (startlen < 0) {
				    c_start = to_charm(start, &leftoff, &startlen);
				    write(fd, c_start, abs(startlen));
				    if (leftoff != 0)
					    start = leftoff;
				}
				start = savstart;   /* restore orig start */
#endif NLS
				cline = clinep = start;
			}
		}

		poptemp();		/* pushed in tmpfil -- bug fix for problem
					   deleting in-line scripts */
	}
}


link_iodocs(i)
	struct ionod	*i;
{
	char l_s[260];	/* local var to hold non-tchar NLS */

	while(i)
	{
		free(i->iolink);

		itos(serial++);
		movstr(to_char(numbuf), tmpnam);
		i->iolink = make(to_tchar(tmpout));
		link((sto_char(i->ioname, l_s)), to_char(i->iolink));

		i = i->iolst;
	}
}


swap_iodoc_nm(i)
	struct ionod	*i;
{
	while(i)
	{
		free(i->ioname);
		i->ioname = i->iolink;
		i->iolink = 0;

		i = i->iolst;
	}
}


savefd(fd)
	int fd;
{
	register int	f;

	f = fcntl(fd, F_DUPFD, 10);
	return(f);
}


restore(last)
	register int	last;
{
	register int 	i;
	register int	dupfd;

	for (i = topfd - 1; i >= last; i--)
	{
		if ((dupfd = fdmap[i].dup_fd) > 0)
			rename(dupfd, fdmap[i].org_fd);
		else
			close(fdmap[i].org_fd);
	}
	topfd = last;
}

