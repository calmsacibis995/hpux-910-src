/* @(#) $Revision: 70.1 $ */      
/* Copyright (c) 1981 Regents of the University of California */
#ifdef hpe
#include <fcntl.h>
#endif hpe

#include "ex.h"
#include "ex_temp.h"
#include "ex_vis.h"
#include "ex_tty.h"

#ifdef ED1000
#include "ex_sm.h"
#endif ED1000

/*
 * Editor temporary file routines.
 * Very similar to those of ed, except uses 2 input buffers.
 */
#define	READ	0
#define	WRITE	1
#define MAXTMPPATH	1024

char	tfname[MAXTMPPATH] = {'\0'} ;	/* temporary file name			*/
char	rfname[MAXTMPPATH] = {'\0'} ;	/* temporary file name for registers	*/
int	havetmp;
short	tfile = -1;
short	rfile = -1;

#ifndef NONLS8 /* User messages */
# define	NL_SETN	11	/* set number */
# include	<msgbuf.h>
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

fileinit()
{
	register char *p, *cp ;
	register int i, j;
	struct stat stbuf;
#ifdef hpe
	char	ex_tname[9];
#endif hpe

					     /* also check for new tmp file name */
	if ((tline == INCRMT * (HBLKS+2)) && (strcmp(tfname, svalue(DIRECTORY)) == 0 ))
		return;
	cleanup(0);
	close(tfile);
	tline = INCRMT * (HBLKS+2);
	blocks[0] = HBLKS;
	blocks[1] = HBLKS+1;
	blocks[2] = -1;
	dirtcnt = 0;
	iblock = -1;
	iblock2 = -1;
	oblock = -1;
	if( !(*tfname) ){ 
		/* the first initialization, get the value of TMPDIR	*/
		if ((cp = getenv("TMPDIR")) && *cp ) {
			/* if TMPDIR exceeds the limit, tfname remains 	*/
			/* null and cause the file io error		*/
			if( ( strlen(cp) <= ( MAXTMPPATH - 9 ) ) && ( strlen(cp) < ONMSZ )  ) {
				strcpy( tfname, cp ) ;
				/* override the directory option value	*/
				strcpy( svalue(DIRECTORY), cp ) ;
			}
		} else {
			/* if TMPDIR is null, use "/tmp"	*/
			CP(tfname, svalue(DIRECTORY)) ;
		}
	} else {
		CP(tfname, svalue(DIRECTORY));
	}
#ifndef hpe
	if (stat(tfname, &stbuf))
#else
	if (0)
#endif hpe
	{
dumbness:
		if (setexit() == 0) {
			tline = 0;	/* ensure a new file is built next time thru */
			filioerr(tfname);
		} else
			putNFL();
		cleanup(1);
		exit(1);
	}
#ifndef hpe
	if ((stbuf.st_mode & S_IFMT) != S_IFDIR) {
		errno = ENOTDIR;
		goto dumbness;
	}
#endif hpe
	ichanged = 0;
	ichang2 = 0;
#ifndef hpe
	ignore(strcat(tfname, "/ExXXXXX"));
	for (p = strend(tfname), i = 5, j = getpid(); i > 0; i--, j /= 10)
		*--p = j % 10 | '0';
	tfile = creat(tfname, 0600);
#else 
	strcpy(ex_tname, "ExXXXXXX");
	strcpy(tfname,mktemp(ex_tname));
	tfile = open(tfname,O_RDWR | O_CREAT | O_TRUNC | 020, 0664 ,"b D2");
#endif hpe
	if (tfile < 0)
		goto dumbness;
#ifdef VMUNIX
	{
		extern stilinc;		/* see below */
		stilinc = 0;
	}
#endif
	havetmp = 1;

#ifndef hpe
	close(tfile);
	tfile = open(tfname, 2);
	if (tfile < 0)
		goto dumbness;
/* 	brk((char *)fendcore); */
#endif hpe
}

cleanup(all)
	bool all;
{
#ifdef SIGTSTP
	/*
	 * If we got here because someone is tried to kill a suspended
	 * vi session, then the onsusp() code has already cleaned up
	 * the terminal state and trying to do it again will result in
	 * the process getting blocked on tty output.
	 */
	if (all && !was_suspended) {
#else SIGTSTP
	if (all) {
#endif SIGTSTP
		putpad(exit_ca_mode);
		flush();
		resetterm();
		normtty--;
	}
	if (havetmp)
		unlink(tfname);
	havetmp = 0;
	if (all && rfile >= 0) {
		unlink(rfname);
		close(rfile);
		rfile = -1;
	}

#ifdef ED1000
	if (sm_clean) {
		sm_rmfile();
		sm_clean = 0;
		insm = 0;
	}
#endif ED1000

	if (all == 1)
		exit(0);
}

#ifndef	NLS16
getline(tl)
#else
getline(tl)
	line tl;
{
	GETLINE(tl);
	char_TO_CHAR(LINEBUF, linebuf);
}

GETLINE(tl)
#endif

	line tl;
{
	register char *bp, *lp;
	register int nl;

#ifndef	NLS16
	lp = linebuf;
#else
	lp = LINEBUF;
#endif

	bp = getblock(tl, READ);
	nl = nleft;
	tl &= ~OFFMSK;
	while (*lp++ = *bp++)
		if (--nl == 0) {
			bp = getblock(tl += INCRMT, READ);
			nl = nleft;
		}
}

#ifndef	NLS16
putline()
#else
putline()
{
	CHAR_TO_char(linebuf, LINEBUF);
	return(PUTLINE());
}

PUTLINE()
#endif

{
	register char *bp, *lp;
	register int nl;
	line tl;

	dirtcnt++;

#ifndef	NLS16
	lp = linebuf;
#else
	lp = LINEBUF;
#endif
#if defined NLS || defined NLS16
	if (right_to_left && rl_inflip) {
		if (rl_order == NL_KEY) {
			/* convert key order to multi-line screen order */
			flip_line(lp,0);
		} else {
			/* convert screen order to multi-line screen order */
			if (strlen(lp) > WCOLS)
				flip_line(flip(lp),0);
		}
		rl_inflip = 0;
	}
#endif
	change();
	tl = tline;
	bp = getblock(tl, WRITE);
	nl = nleft;
	tl &= ~OFFMSK;
	while (*bp = *lp++) {
		if (*bp++ == '\n') {
			*--bp = 0;
			linebp = lp;
			break;
		}
		if (--nl == 0) {
			bp = getblock(tl += INCRMT, WRITE);
			nl = nleft;
		}
	}
	tl = tline;

#ifndef	NLS16
	tline += (((lp - linebuf) + BNDRY - 1) >> SHFT) & 077776;
#else
	tline += (((lp - LINEBUF) + BNDRY - 1) >> SHFT) & 077776;
#endif
	return (tl);
}

int	read();
int	write();

char *
getblock(atl, iof)
	line atl;
	int iof;
{
	register int bno, off;
        register char *p1, *p2;
        register int n;
	
	bno = (atl >> OFFBTS) & BLKMSK;
	off = (atl << SHFT) & LBTMSK;
	if (bno >= NMBLKS)
		error((nl_msg(1, " Tmp file too large")));
	nleft = BUFSIZ - off;
	if (bno == iblock) {
		ichanged |= iof;
		hitin2 = 0;
		return (ibuff + off);
	}
	if (bno == iblock2) {
		ichang2 |= iof;
		hitin2 = 1;
		return (ibuff2 + off);
	}
	if (bno == oblock)
		return (obuff + off);
	if (iof == READ) {
		if (hitin2 == 0) {
			if (ichang2) {
/* ========================================================================= */
/*
** CRYPT block 1
*/
#ifdef CRYPT
				if(xtflag)
					crblock(tperm, ibuff2, CRSIZE, (long)0);
#endif CRYPT
/* ========================================================================= */
				blkio(iblock2, ibuff2, write);
			}
			ichang2 = 0;
			iblock2 = bno;
			blkio(bno, ibuff2, read);
/* ========================================================================= */
/*
** CRYPT block 2
*/
#ifdef CRYPT
			if(xtflag)
				crblock(tperm, ibuff2, CRSIZE, (long)0);
#endif CRYPT
/* ========================================================================= */
			hitin2 = 1;
			return (ibuff2 + off);
		}
		hitin2 = 0;
		if (ichanged) {
/* ========================================================================= */
/*
** CRYPT block 3
*/
#ifdef CRYPT
			if(xtflag)
				crblock(tperm, ibuff, CRSIZE, (long)0);
#endif CRYPT
/* ========================================================================= */
			blkio(iblock, ibuff, write);
		}
		ichanged = 0;
		iblock = bno;
		blkio(bno, ibuff, read);
/* ========================================================================= */
/*
** CRYPT block 4
*/
#ifdef CRYPT
		if(xtflag)
			crblock(tperm, ibuff, CRSIZE, (long)0);
#endif CRYPT
/* ========================================================================= */
		return (ibuff + off);
	}
	if (oblock >= 0) {
/* ========================================================================= */
/*
** CRYPT block 5
*/
#ifdef CRYPT
		if(xtflag) {
			/*
			 * Encrypt block before writing, so some devious
			 * person can't look at temp file while editing.
			 */
			p1 = obuff;
			p2 = crbuf;
			n = CRSIZE;
			while(n--)
				*p2++ = *p1++;
			crblock(tperm, crbuf, CRSIZE, (long)0);
			blkio(oblock, crbuf, write);
		} else
#endif CRYPT
/* ========================================================================= */
			blkio(oblock, obuff, write);
	}
	oblock = bno;
	return (obuff + off);
}

#ifdef	VMUNIX
#define	INCORB	64
char	incorb[INCORB+1][BUFSIZ];
#define	pagrnd(a)	((char *)(((int)a)&~(BUFSIZ-1)))
int	stilinc;	/* up to here not written yet */
#endif

blkio(b, buf, iofcn)
	short b;
	char *buf;
	int (*iofcn)();
{

#ifdef VMUNIX
	if (b < INCORB) {
		if (iofcn == read) {
			bcopy(pagrnd(incorb[b+1]), buf, BUFSIZ);
			return;
		}
		bcopy(buf, pagrnd(incorb[b+1]), BUFSIZ);
		if (laste) {
			if (b >= stilinc)
				stilinc = b + 1;
			return;
		}
	} else if (stilinc)
		tflush();
#endif
	lseek(tfile, (long) (unsigned) b * BUFSIZ, 0);
	if ((*iofcn)(tfile, buf, BUFSIZ) != BUFSIZ)
		filioerr(tfname);
}

#ifdef VMUNIX
tlaste()
{

	if (stilinc)
		dirtcnt = 0;
}

tflush()
{
	int i = stilinc;
	
	stilinc = 0;
	lseek(tfile, (long) 0, 0);
	if (write(tfile, pagrnd(incorb[1]), i * BUFSIZ) != (i * BUFSIZ))
		filioerr(tfname);
}
#endif

/*
 * Synchronize the state of the temporary file in case
 * a crash occurs.
 */
synctmp()
{
	register int cnt;
	register line *a;
	register short *bp;
        register char *p1, *p2;
        register int n, i;

#ifdef VMUNIX
	if (stilinc)
		return;
#endif
	if (dol == zero)
		return;
	/*
	 * In theory, we need to encrypt iblock and iblock2 before writing
	 * them out, as well as oblock, but in practice ichanged and ichang2
	 * can never be set, so this isn't really needed.  Likewise, the
	 * code in getblock above for iblock+iblock2 isn't needed.
	 */
	if (ichanged)
		blkio(iblock, ibuff, write);
	ichanged = 0;
	if (ichang2)
		blkio(iblock2, ibuff2, write);
	ichang2 = 0;
	if (oblock != -1)
/* ========================================================================= */
/*
** CRYPT block 6
*/
#ifdef CRYPT
	if(xtflag) {
		/*
		 * Encrypt block before writing, so some devious
		 * person can't look at temp file while editing.
		 */
		p1 = obuff;
		p2 = crbuf;
		n = CRSIZE;
		while(n--)
			*p2++ = *p1++;
		crblock(tperm, crbuf, CRSIZE, (long)0);
		blkio(oblock, crbuf, write);
	} else
#endif CRYPT
/* ========================================================================= */
		blkio(oblock, obuff, write);
	time(&H.Time);
	uid = getuid();
	*zero = (line) H.Time;
	i=0;
	for (a = zero, bp = blocks; a <= dol; a += BUFSIZ / sizeof *a, bp++) {
		/* FSDlj08776: In case of files with many lines (>250000), */
		/* bp will get too big that the contents of some global    */
		/* variables, like kflag, will be overwritten.		   */
		if (i++ >= LBLKS) 
#ifndef NONLS8 
                        error((nl_msg(6, "Warning: Out of memory|Warning: Out of memory saving lines for recovery - try using ed ")));
#else NONLS8
                        error("Warning: Out of memory@saving lines for recovery - try using ed ");
#endif NONLS8
		
		if (*bp < 0) {
			tline = (tline + OFFMSK) &~ OFFMSK;
			*bp = ((tline >> OFFBTS) & BLKMSK);
			if (*bp > NMBLKS)
				error((nl_msg(2, " Tmp file too large")));
			tline += INCRMT;
			oblock = *bp + 1;
			bp[1] = -1;
		}
		lseek(tfile, (long) (unsigned) *bp * BUFSIZ, 0);
		cnt = ((dol - a) + 2) * sizeof (line);
		if (cnt > BUFSIZ)
			cnt = BUFSIZ;
		if (write(tfile, (char *) a, cnt) != cnt) {
oops:
			*zero = 0;
			filioerr(tfname);
		}
		*zero = 0;
	}
	flines = lineDOL();
	lseek(tfile, 0l, 0);
	if (write(tfile, (char *) &H, sizeof H) != sizeof H)
		goto oops;
}

TSYNC()
{

	if (dirtcnt > MAXDIRT) {	/* mjm: 12 --> MAXDIRT */
#ifdef VMUNIX
		if (stilinc)
			tflush();
#endif
		dirtcnt = 0;
		synctmp();
	}
}

/*
 * Named buffer routines.
 * These are implemented differently than the main buffer.
 * Each named buffer has a chain of blocks in the register file.
 * Each block contains roughly 508 chars of text,
 * and a previous and next block number.  We also have information
 * about which blocks came from deletes of multiple partial lines,
 * e.g. deleting a sentence or a LISP object.
 *
 * We maintain a free map for the temp file.  To free the blocks
 * in a register we must read the blocks to find how they are chained
 * together.
 *
 * BUG:		The default savind of deleted lines in numbered
 *		buffers may be rather inefficient; it hasn't been profiled.
 */
struct	strreg {
	short	rg_flags;
	short	rg_nleft;
	short	rg_first;
	short	rg_last;
} strregs[('z'-'a'+1) + ('9'-'0'+1)], *strp;

struct	rbuf {
	short	rb_prev;
	short	rb_next;
	char	rb_text[BUFSIZ - 2 * sizeof (short)];
} *rbuf, KILLrbuf, putrbuf, YANKrbuf, regrbuf;
#ifdef VMUNIX
short	rused[256];
#else
short	rused[32];
#endif
short	rnleft;
short	rblock;
short	rnext;
char	*rbufcp;

regio(b, iofcn)
	short b;
	int (*iofcn)();
{

	if (rfile == -1) {
		CP(rfname, tfname);
		*(strend(rfname) - 7) = 'R';
#ifndef hpe
		rfile = creat(rfname, 0600);
#else
	        rfile = open(rfname,O_RDWR | O_CREAT | O_TRUNC | 020, 0664 ,"b D2");
#endif hpe
		if (rfile < 0)
oops:
			filioerr(rfname);
#ifndef hpe
		close(rfile);
		rfile = open(rfname, 2);
		if (rfile < 0)
			goto oops;
#endif hpe
	}
	lseek(rfile, (long) b * BUFSIZ, 0);
	if ((*iofcn)(rfile, rbuf, BUFSIZ) != BUFSIZ)
		goto oops;
	rblock = b;
}

REGblk()
{
	register int i, j, m;

	for (i = 0; i < sizeof rused / sizeof rused[0]; i++) {
		m = (rused[i] ^ 0177777) & 0177777;
		if (i == 0)
			m &= ~1;
		if (m != 0) {
			j = 0;
			while ((m & 1) == 0)
				j++, m >>= 1;
			rused[i] |= (1 << j);
#ifdef RDEBUG
			printf("allocating block %d\n", i * 16 + j);
#endif
			return (i * 16 + j);
		}
	}
	error((nl_msg(3, "Out of register space (ugh)")));
	/*NOTREACHED*/
}

struct	strreg *
mapreg(c)
	register int c;
{


#ifndef NONLS8 /* 8bit integrity */
	if ((c >= IS_MACRO_LOW_BOUND) && isupper(c & TRIM)) 
		c = tolower(c & TRIM);
	return (((c >= IS_MACRO_LOW_BOUND) && isdigit(c & TRIM)) ? 
	    &strregs[('z'-'a'+1)+(c-'0')] : &strregs[c-'a']);  
#else NONLS8
	if ((c >= IS_MACRO_LOW_BOUND) && isupper(c)) 
		c = tolower(c);
	return (((c >= IS_MACRO_LOW_BOUND) && isdigit(c)) ? 
	    &strregs[('z'-'a'+1)+(c-'0')] : &strregs[c-'a']);
#endif NONLS8

}

int	shread();

KILLreg(c)
	register int c;
{
	register struct strreg *sp;

	rbuf = &KILLrbuf;
	sp = mapreg(c);
	rblock = sp->rg_first;
	sp->rg_first = sp->rg_last = 0;
	sp->rg_flags = sp->rg_nleft = 0;
	while (rblock != 0) {
#ifdef RDEBUG
		printf("freeing block %d\n", rblock);
#endif
		rused[rblock / 16] &= ~(1 << (rblock % 16));
		regio(rblock, shread);
		rblock = rbuf->rb_next;
	}
}

/*VARARGS*/
shread()
{
	struct front { short a; short b; };

	if (read(rfile, (char *) rbuf, sizeof (struct front)) == sizeof (struct front))
		return (sizeof (struct rbuf));
	return (0);
}

int	getREG();

putreg(c)
	char c;
{
	register line *odot = dot;
	register line *odol = dol;
	register int cnt;

	deletenone();
	appendnone();
	rbuf = &putrbuf;
	rnleft = 0;
	rblock = 0;
	rnext = mapreg(c)->rg_first;
	if (rnext == 0) {
		if (inopen) {
			splitw++;
			vclean();
			vgoto(WECHO, 0);
		}
		vreg = -1;
		error((nl_msg(4, "Nothing in register %c")), c);
	}
	if (inopen && partreg(c)) {
		if (!FIXUNDO) {
			splitw++; vclean(); vgoto(WECHO, 0); vreg = -1;
			error((nl_msg(5, "Can't put partial line inside macro")));
		}
		squish();
		addr1 = addr2 = dol;
	}
	cnt = append(getREG, addr2);
	if (inopen && partreg(c)) {
		unddol = dol;
		dol = odol;
		dot = odot;
		pragged(0);
	}
	killcnt(cnt);
	notecnt = cnt;
}

partreg(c)
	char c;
{

	return (mapreg(c)->rg_flags);
}

notpart(c)
	register int c;
{

	if (c)
		mapreg(c)->rg_flags = 0;
}

getREG()
{

#ifndef	NLS16
	register char *lp = linebuf;
#else
	register char *lp = LINEBUF;
#endif

	register int c;

	for (;;) {
		if (rnleft == 0) {
			if (rnext == 0)
				return (EOF);
			regio(rnext, read);
			rnext = rbuf->rb_next;
			rbufcp = rbuf->rb_text;
			rnleft = sizeof rbuf->rb_text;
		}
		c = *rbufcp;
		if (c == 0)
			return (EOF);
		rbufcp++, --rnleft;
		if (c == '\n') {
			*lp++ = 0;

#ifdef	NLS16
			char_TO_CHAR(LINEBUF, linebuf);
#endif

			return (0);
		}
		*lp++ = c;
	}
}

YANKreg(c)
	register int c;
{
	register line *addr;
	register struct strreg *sp;
	CHAR savelb[LBSIZE];

#ifndef NONLS8 /* Character set features */
	if ((c >= IS_MACRO_LOW_BOUND) && isdigit(c & TRIM))
#else NONLS8
	if ((c >= IS_MACRO_LOW_BOUND) && isdigit(c))
#endif NONLS8

		kshift();

#ifndef NONLS8 /* Character set features */
	if ((c >= IS_MACRO_LOW_BOUND) && islower(c & TRIM))
#else NONLS8
	if ((c >= IS_MACRO_LOW_BOUND) && islower(c))
#endif NONLS8

		KILLreg(c);
	strp = sp = mapreg(c);
	sp->rg_flags = inopen && cursor && wcursor;
	rbuf = &YANKrbuf;
	if (sp->rg_last) {
		regio(sp->rg_last, read);
		rnleft = sp->rg_nleft;
		rbufcp = &rbuf->rb_text[sizeof rbuf->rb_text - rnleft];
	} else {
		rblock = 0;
		rnleft = 0;
	}

#ifndef	NLS16
	CP(savelb,linebuf);
#else
	STRCPY(savelb,linebuf);
#endif

	for (addr = addr1; addr <= addr2; addr++) {
		getline(*addr);
		if (sp->rg_flags) {
			if (addr == addr2)
				*wcursor = 0;
			if (addr == addr1)

#ifndef	NLS16
				strcpy(linebuf, cursor);
#else
				STRCPY(linebuf, cursor);
#endif

		}

#ifdef	NLS16
		CHAR_TO_char(linebuf, LINEBUF);
#endif

		YANKline();

	}
	rbflush();
	killed();

#ifndef	NLS16
	CP(linebuf,savelb);
#else
	STRCPY(linebuf,savelb);
#endif

}

kshift()
{
	register int i;

	KILLreg('9');
	for (i = '8'; i >= '0'; i--)
		copy(mapreg(i+1), mapreg(i), sizeof (struct strreg));
}

YANKline()
{

#ifndef	NLS16
	register char *lp = linebuf;
#else
	register char *lp = LINEBUF;
#endif

	register struct rbuf *rp = rbuf;
	register int c;

	do {
		c = *lp++;
		if (c == 0)
			c = '\n';
		if (rnleft == 0) {
			rp->rb_next = REGblk();
			rbflush();
			rblock = rp->rb_next;
			rp->rb_next = 0;
			rp->rb_prev = rblock;
			rnleft = sizeof rp->rb_text;
			rbufcp = rp->rb_text;
		}
		*rbufcp++ = c;
		--rnleft;
	} while (c != '\n');
	if (rnleft)
		*rbufcp = 0;
}

rbflush()
{
	register struct strreg *sp = strp;

	if (rblock == 0)
		return;
	regio(rblock, write);
	if (sp->rg_first == 0)
		sp->rg_first = rblock;
	sp->rg_last = rblock;
	sp->rg_nleft = rnleft;
}

/* Register c to char buffer buf of size buflen */
regbuf(c, buf, buflen)
char c;
char *buf;
int buflen;
{
	register char *p, *lp;

	rbuf = &regrbuf;
	rnleft = 0;
	rblock = 0;
	rnext = mapreg(c)->rg_first;
	if (rnext==0) {
		*buf = 0;
		error((nl_msg(6, "Nothing in register %c")),c);
	}
	p = buf;
	while (getREG()==0) {

#ifndef	NLS16
		for (lp=linebuf; *lp;) {
#else
		for (lp=LINEBUF; *lp;) {
#endif

			if (p >= &buf[buflen])

#ifndef NONLS8 /* User messages */
				error((nl_msg(7, "Register too long|Register too long to fit in memory")));
#else NONLS8
				error("Register too long@to fit in memory");
#endif NONLS8

			*p++ = *lp++;
		}
		*p++ = '\n';
	}
	if (partreg(c)) p--;
	*p = '\0';
	getDOT();
}
