/* @(#) $Revision: 66.8 $ */    
/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"

#ifdef hpe
# include <fcntl.h>
# define _TTYIN (char *) myctermid()
#endif hpe

/*
 * File input/output, source, preserve and recover
 */

/*
 * Following remember where . was in the previous file for return
 * on file switching.
 */
int	altdot;
int	oldadot;
bool	wasalt;
short	isalt;

long	cntch;			/* Count of characters on unit io */
#ifndef VMUNIX
short	cntln;			/* Count of lines " */
#else
int	cntln;
#endif
long	cntnull;		/* Count of nulls " */

#ifdef NONLS8 /* 8bit integrity */
long	cntodd;			/* Count of non-ascii characters " */
#endif NONLS8

#ifndef NONLS8 /* User messages */
# define	NL_SETN	7	/* set number */
# include	<msgbuf.h>
# undef	getchar
# undef	putchar
#else NONLS8
# define	nl_msg(i, s)	(s)
#endif NONLS8

/*
 * Parse file name for command encoded by comm.
 * If comm is E then command is doomed and we are
 * parsing just so user won't have to retype the name.
 */
filename(comm)
	int comm;
{
	register int c = comm, d;
	register int i;

	d = getchar();
	if (endcmd(d)) {
		if (savedfile[0] == 0 && comm != 'f')
			error((nl_msg(1, "No file|No current filename")));
		CP(file, savedfile);
		wasalt = (isalt > 0) ? isalt-1 : 0;
		isalt = 0;
		oldadot = altdot;
		if (c == 'e' || c == 'E')
			altdot = lineDOT();
		if (d == EOF)
			ungetchar(d);
	} else {
		ungetchar(d);
		getone();
		eol();
		if (savedfile[0] == 0 && c != 'E' && c != 'e') {
			c = 'e';
			edited = 0;
		}
		wasalt = strcmp(file, altfile) == 0;
		oldadot = altdot;
		switch (c) {

		case 'f':
			edited = 0;
			/* fall into ... */

		case 'e':
			if (savedfile[0]) {
				altdot = lineDOT();
				CP(altfile, savedfile);
			}
			CP(savedfile, file);
			break;

		default:
			if (file[0]) {
				if (c != 'E')
					altdot = lineDOT();
				CP(altfile, file);
			}
			break;
		}
	}
	if (hush && comm != 'f' || comm == 'E')
		return;
#if defined NLS || defined NLS16
	RL_OKEY
#endif
	if (file[0] != 0) {
		lprintf("\"%s\"", file);
#if defined NLS || defined NLS16
		if (right_to_left && rl_mode == NL_NONLATIN) {
			printf("%c",alt_space);
		}
#endif
		if (comm == 'f') {
			if (value(READONLY))
				printf((nl_msg(2, " [Read only]")));
			if (!edited)
				printf((nl_msg(3, " [Not edited]")));
			if (tchng)
				printf((nl_msg(4, " [Modified]")));
		}
		flush();
	} else
		printf((nl_msg(5, "No file ")));
	if (comm == 'f') {
		if (!(i = lineDOL()))
			i++;
		printf((nl_msg(6, " line %d of %d --%ld%%--")), lineDOT(), lineDOL(),
		    (long) 100 * lineDOT() / i);
	}
#if defined NLS || defined NLS16
	RL_OSCREEN
#endif
}

/*
#ifndef	NLS16
 * Get the argument words for a command into genbuf
#else
 * Get the argument words for a command into GENBUF
#endif
 * expanding # and %.
 */
getargs()
{
	register int c;
	register char *cp, *fp;
	static char fpatbuf[32];	/* hence limit on :next +/pat */

	pastwh();
	if (peekchar() == '+') {
		for (cp = fpatbuf;;) {
			c = *cp++ = getchar();
			if (cp >= &fpatbuf[sizeof(fpatbuf)])
				error((nl_msg(7, "Pattern too long")));
			
#ifndef NONLS8 /* Character set features */
			if (c == '\\' && (peekchar() >= IS_MACRO_LOW_BOUND) && _isspace(peekchar() & TRIM))
#else NONLS8
			if (c == '\\' && (peekchar() >= IS_MACRO_LOW_BOUND) && isspace(peekchar()))
#endif NONLS8

				c = getchar();

#ifndef NONLS8 /* Character set features */
			if (c == EOF || ((c >= IS_MACRO_LOW_BOUND) && _isspace(c & TRIM))) {
#else NONLS8
			if (c == EOF || ((c >= IS_MACRO_LOW_BOUND) && isspace(c))) {
#endif NONLS8

				ungetchar(c);
				*--cp = 0;
				firstpat = &fpatbuf[1];
				break;
			}
		}
	}
	if (skipend())
		return (0);

#ifndef	NLS16
	CP(genbuf, "echo "); cp = &genbuf[5];
#else
	CP(GENBUF, "echo "); cp = &GENBUF[5];
#endif

	for (;;) {
		c = getchar();
		if (endcmd(c)) {
			ungetchar(c);
			break;
		}
		switch (c) {

		case '\\':
			if (any(peekchar(), "#%|"))
				c = getchar();
			/* fall into... */

		default:

#ifndef	NLS16
			if (cp > &genbuf[LBSIZE - 2])
#else
			if (cp > &GENBUF[LBSIZE - 2])
#endif

flong:
				error((nl_msg(8, "Argument buffer overflow")));
			*cp++ = c;
			break;

		case '#':
			fp = altfile;
			if (*fp == 0)

#ifndef NONLS8 /* User messages */
				error((nl_msg(9, "No alternate filename|No alternate filename to substitute for #")));
#else NONLS8
				error("No alternate filename@to substitute for #");
#endif NONLS8

			goto filexp;

		case '%':
			fp = savedfile;
			if (*fp == 0)

#ifndef NONLS8 /* User messages */
				error((nl_msg(10, "No current filename|No current filename to substitute for %%")));
#else NONLS8
				error("No current filename@to substitute for %%");
#endif NONLS8

filexp:
			while (*fp) {

#ifndef	NLS16
				if (cp > &genbuf[LBSIZE - 2])
#else
				if (cp > &GENBUF[LBSIZE - 2])
#endif

					goto flong;
				*cp++ = *fp++;
			}
			break;
		}
	}
	*cp = 0;
	return (1);
}

/*
#ifndef	NLS16
 * Glob the argument words in genbuf, or if no globbing
#else
 * Glob the argument words in GENBUF, or if no globbing
#endif
 * is implied, just split them up directly.
 */
glob(gp)
	struct glob *gp;
{
	int pvec[2];
	register char **argv = gp->argv;
	register char *cp = gp->argspac;
	register int c;
	char ch;
	int nleft = NCARGS;

	gp->argc0 = 0;
	if (gscan() == 0) {

#ifndef	NLS16
		register char *v = genbuf + 5;		/* strlen("echo ") */
#else
		register char *v = GENBUF + 5;		/* strlen("echo ") */
#endif

		for (;;) {

#ifndef NONLS8 /* Character set features */
			while ((*v >= IS_MACRO_LOW_BOUND) && _isspace(*v & TRIM))
#else NONLS8
			while ((*v >= IS_MACRO_LOW_BOUND) && isspace(*v))
#endif NONLS8

				v++;
			if (!*v)
				break;
			*argv++ = cp;

#ifndef NONLS8 /* Character set features */
			while (*v && ((*v < IS_MACRO_LOW_BOUND) || !_isspace(*v & TRIM)))
#else NONLS8
			while (*v && ((*v < IS_MACRO_LOW_BOUND) || !isspace(*v)))
#endif NONLS8

				*cp++ = *v++;
			*cp++ = 0;
			gp->argc0++;
		}
		*argv = 0;
		return;
	}
	if (pipe(pvec) < 0)
		error((nl_msg(11, "Can't make pipe to glob")));
	pid = fork();
	io = pvec[0];
	if (pid < 0) {
		close(pvec[1]);
		error((nl_msg(12, "Can't fork to do glob")));
	}
	if (pid == 0) {
		int oerrno;

		close(1);
		dup(pvec[1]);
		close(pvec[0]);
		close(2);	/* so errors don't mess up the screen */
		open("/dev/null", 1);

#ifndef	NLS16
		execl(svalue(SHELL), "sh", "-c", genbuf, 0);
#else
		execl(svalue(SHELL), "sh", "-c", GENBUF, 0);
#endif

		oerrno = errno; close(1); dup(2); errno = oerrno;
		filioerr(svalue(SHELL));
	}
	close(pvec[1]);
	do {
		*argv = cp;
		for (;;) {
			if (read(io, &ch, 1) != 1) {
				close(io);
				c = -1;
			} else
				c = ch & TRIM;

#ifndef NONLS8 /* Character set features */
			if (c <= 0 || _isspace(c & TRIM))
#else NONLS8
			if (c <=  0 || isspace(c))
#endif NONLS8

				break;
			*cp++ = c;
			if (--nleft <= 0)
				error((nl_msg(13, "Arg list too long")));
		}
		if (cp != *argv) {
			--nleft;
			*cp++ = 0;
			gp->argc0++;
			if (gp->argc0 >= NARGS)
				error((nl_msg(14, "Arg list too long")));
			argv++;
		}
	} while (c >= 0);
	waitfor();
	if (gp->argc0 == 0)
		error((nl_msg(15, "No match")));
}

/*
 * Scan genbuf for shell metacharacters.
 * Set is union of v7 shell and csh metas.
 */
gscan()
{
	register char *cp;

#ifndef	NLS16
	for (cp = genbuf; *cp; cp++)
		if (any(*cp, "~{[*?$`'\"\\"))
#else
	for (cp = GENBUF; *cp; cp++)
		/* prevent mistaking 2nd byte of 16-bit character for a ANK. */
		if (FIRSTof2((*cp)&TRIM) && SECONDof2((*(cp+1))&TRIM))
			cp++;
		else if (any(*cp, "~{[*?$`'\"\\"))
#endif

			return (1);
	return (0);
}

/*
 * Parse one filename into file.
 */
struct glob G;
getone()
{
	register char *str;

	if (getargs() == 0)
		error((nl_msg(16, "Missing filename")));
	glob(&G);
	if (G.argc0 > 1)
		error((nl_msg(17, "Ambiguous|Too many file names")));
	str = G.argv[G.argc0 - 1];
	if (strlen(str) > FNSIZE - 4)
		error((nl_msg(18, "Filename too long")));
samef:
	CP(file, str);
}

/*
 * Read a file from the world.
 * C is command, 'e' if this really an edit (or a recover).
 */
rop(c)
	int c;
{
	register int i;
	struct stat stbuf;
	unsigned short magic;
	static int ovro;	/* old value(READONLY) */
	static int denied;	/* 1 if READONLY was set due to file permissions */

	io = open(file, 0);
	if (io < 0) {
		if (c == 'e' && errno == ENOENT) {
			edited++;
			/*
			 * If the user just did "ex foo" he is probably
			 * creating a new file.  Don't be an error, since
			 * this is ugly, and it screws up the + option.
			 */
			if (!seenprompt) {
#if defined NLS || defined NLS16
				RL_OKEY
#endif
				printf((nl_msg(19, " [New file]")));
#if defined NLS || defined NLS16
				RL_OSCREEN
#endif
				noonl();
				if (laste) {
#ifdef VMUNIX
					tlaste();
#endif
					laste = 0;
					sync();
				}
				return;
			}
		}
		syserror(0);
	}
	if (fstat(io, &stbuf))
		syserror(0);
#ifdef hpe
	/* Set the file mode to ordinary read/write by all */
	stbuf.st_mode = 0100666;
#endif hpe
	switch (stbuf.st_mode & S_IFMT) {

	case S_IFBLK:
		error((nl_msg(20, " Block special file")));

	case S_IFCHR:
		if (isatty(io))
			error((nl_msg(21, " Teletype")));
		if (samei(&stbuf, "/dev/null"))
			break;
		error((nl_msg(22, " Character special file")));

	case S_IFDIR:
		error((nl_msg(23, " Directory")));

	case S_IFREG:
/* ========================================================================= */
/*
** CRYPT block 1
*/
#ifdef CRYPT
		if (xflag)
			break;
#endif CRYPT
/* ========================================================================= */
#ifdef hpux
		lseek(io, 2l, 0);	/* skip hp sys id */
#endif
		i = read(io, (char *) &magic, sizeof(magic));
		lseek(io, 0l, 0);
		if (i != sizeof(magic))
			break;
#ifdef hpe
		close(io);
		io = open(file,O_RDONLY | 020, 0, "T");
#endif hpe
		switch (magic) {

		case 0405:	/* data overlay on exec */
		case 0407:	/* unshared */
		case 0410:	/* shared text */
		case 0411:	/* separate I/D */
		case 0413:	/* VM/Unix demand paged */
		case 0430:	/* PDP-11 Overlay shared */
		case 0431:	/* PDP-11 Overlay sep I/D */
			error((nl_msg(24, " Executable")));

		case 0406:	/* relocatable only */
			error((nl_msg(25, " Relocatable file")));

		/*
		 * We do not forbid the editing of portable archives
		 * because it is reasonable to edit them, especially
		 * if they are archives of text files.  This is
		 * especially useful if you archive source files together
		 * and copy them to another system with ~%take, since
		 * the files sometimes show up munged and must be fixed.
		 */
		case 0177545:
		case 0177555:
			error((nl_msg(26, " Archive")));

		default:

#ifdef NONLS8 /* 8bit integrity */
# ifdef mbb
			/* C/70 has a 10 bit byte */
			if (magic & 03401600)
# else
			/* Everybody else has an 8 bit byte */
			if (magic & 0100200)
# endif
				error(" Non-ascii file");
#endif NONLS8

			break;
		}
	}
	if (c != 'r') {
		if (value(READONLY) && denied) {
			value(READONLY) = ovro;
			denied = 0;
		}
		if ((stbuf.st_mode & 0222) == 0 
#ifndef hpe
					/* Stub access due to failure if
					   file is already opened	*/
					|| access(file, 2) < 0
#endif hpe
								) {
			ovro = value(READONLY);
			denied = 1;
			value(READONLY) = 1;
		}
	}
	if (hush == 0 && value(READONLY)) {
		printf((nl_msg(27, " [Read only]")));
		flush();
	}
	if (c == 'r')
		setdot();
	else
		setall();
	if (FIXUNDO && inopen && c == 'r')
#ifdef ED1000
		undap1 = undap2 = dot + 1;
#else
		undap1 = undap2 = addr2 + 1;
#endif ED1000

	rop2();
	rop3(c);
}

rop2()
{
	line *first, *last, *a;

	deletenone();
	clrstats();
	first = addr2 + 1;
	ignore(append(getfile, addr2));

	/*
	 * If we had a dummy line in an empty buffer and
	 * we were able to read some real line(s) into
	 * the buffer, then shift dol so that the dummy
	 * empty line is moved from the current set of
	 * lines to the undo save area.  If the user
	 * undo's this ":read" then the buffer will once
	 * again contain the blank line.  Unfortunately,
	 * after the undo the blank line will no longer
	 * be flagged as not real (via real_empty).  Few
	 * users should either notice or care about this
	 * shortcoming and it would be difficult if not
	 * impossible to fix in the current algorithm.
	 */
	if (oreal_empty && (dol > one)) {
		dol -= 1;
		dot = one;	/* force cursor to 1st line of :read lines */
	}
	oreal_empty = 0;

	if (!value(MODELINES))
		return;
	last = dot;
	for (a=first; a<=last; a++) {
		if (a==first+5 && last-first > 10)
			a = last - 4;

#ifndef	NLS16
		getline(*a);
		checkmodeline(linebuf);
#else
		getline(*a);
		checkmodeline(LINEBUF);
#endif

	}
}

rop3(c)
	int c;
{

	if (iostats() == 0 && c == 'e')
		edited++;
	if (c == 'e') {
		if (wasalt || firstpat) {
			register line *addr = zero + oldadot;

			if (addr > dol)
				addr = dol;
			if (firstpat) {
				if (dol > one)		/* needed if nowrapscan set by user */
					dot = one;
				globp = (*firstpat) ? firstpat : "$";
				commands(1,1);
				firstpat = 0;
			} else if (addr >= one) {
				if (inopen)
					dot = addr;
				markpr(addr);
			} else
				goto other;
		} else
other:
			if (dol > zero) {
				if (inopen)
					dot = one;
				markpr(one);
			}
		if(FIXUNDO)
			undkind = UNDNONE;
		if (inopen) {
			vcline = 0;
			vreplace(0, lines, lineDOL());
		}
	}
	if (laste) {
#ifdef VMUNIX
		tlaste();
#endif
		laste = 0;
		sync();
	}
}

/*
 * Are these two really the same inode?
 */
samei(sp, cp)
	struct stat *sp;
	char *cp;
{
	struct stat stb;

	if (stat(cp, &stb) < 0 || sp->st_dev != stb.st_dev)
		return (0);
	return (sp->st_ino == stb.st_ino);
}

/* Returns from edited() */
#define	EDF	0		/* Edited file */
#define	NOTEDF	-1		/* Not edited file */
#define	PARTBUF	1		/* Write of partial buffer to Edited file */

/*
 * Write a file.
 */
wop(dofname)
bool dofname;	/* if 1 call filename, else use savedfile */
{
	register int c, exclam, nonexist;
	line *saddr1, *saddr2;
	struct stat stbuf;
#ifdef hpe
	char tmpfil[128];
#endif hpe	

	c = 0;
	exclam = 0;
	if (dofname) {
		if (peekchar() == '!')
			exclam++, ignchar();
		ignore(skipwh());
		while (peekchar() == '>')
			ignchar(), c++, ignore(skipwh());
		if (c != 0 && c != 2)
			error((nl_msg(28, "Write forms are 'w' and 'w>>'")));
		filename('w');
	} else {
		if (savedfile[0] == 0)
			error((nl_msg(29, "No file|No current filename")));
		saddr1=addr1;
		saddr2=addr2;
		addr1=one;
		addr2=dol;
		CP(file, savedfile);
		if (inopen) {
			vclrech(0);
			splitw++;
		}
#if defined NLS || defined NLS16
		RL_OKEY
#endif
		lprintf("\"%s\"", file);
#if defined NLS || defined NLS16
		RL_OSCREEN
#endif
	}
	nonexist = stat(file, &stbuf);
	switch (c) {

	case 0:
		if (!exclam && (!value(WRITEANY) || value(READONLY)))
		switch (edfile()) {
		
		case NOTEDF:
			if (nonexist)
				break;
			if ((stbuf.st_mode & S_IFMT) == S_IFCHR) {
				if (samei(&stbuf, "/dev/null"))
					break;
				if (samei(&stbuf, "/dev/tty"))
					break;
			}
			io = open(file, 1);
			if (io < 0)
				syserror(0);
			if (!isatty(io))
#ifdef ED1000
				serror((nl_msg(101, " File exists| File exists - use \"wr or er %s\" to overwrite")), file);
#else
				serror((nl_msg(30, " File exists| File exists - use \"w! %s\" to overwrite")), file);
#endif ED1000
			close(io);
			break;

		case EDF:
			if (value(READONLY))
				error((nl_msg(31, " File is read only")));
			break;

		case PARTBUF:
			if (value(READONLY))
				error((nl_msg(32, " File is read only")));
#ifdef ED1000
			error((nl_msg(102, " Use \"wr\" to write partial buffer")));
#else
			error((nl_msg(33, " Use \"w!\" to write partial buffer")));
#endif ED1000
		}
cre:
/*
		synctmp();
*/
#ifdef hpe
		{
			char namebuf[128], *template = "ViXXXXXX";

			strcpy(namebuf,template);
			strcpy(tmpfil,mktemp(namebuf));

			io = open (tmpfil, O_WRONLY | O_CREAT | 020, 0666, "R128 d1 D2");
		}
#else
		io = creat(file, 0666);
#endif hpe
		if (io < 0)
			syserror(0);
		writing = 1;
#if defined NLS || defined NLS16
		RL_OKEY
#endif
		if (hush == 0)
			if (nonexist)
				printf((nl_msg(34, " [New file]")));
			else if (value(WRITEANY) && edfile() != EDF)
				printf((nl_msg(35, " [Existing file]")));
#if defined NLS || defined NLS16
		RL_OSCREEN
#endif
		break;

	case 2:
		io = open(file, 1);
		if (io < 0) {
			if (exclam || value(WRITEANY))
				goto cre;
			syserror(0);
		}
		lseek(io, 0l, 2);
		break;
	}
	putfile(0);
	ignore(iostats());
#ifdef hpe
	{
		int fd;

		unlink(file);
		fd = open(tmpfil,O_RDONLY | 020,0,"D1");
		close (fd);
		link(tmpfil,file);
		unlink(tmpfil);
	}
#endif hpe
	if (c != 2 && addr1 == one && addr2 == dol) {
		if (eq(file, savedfile))
			edited = 1;
		sync();
	}
	if (!dofname) {
		addr1 = saddr1;
		addr2 = saddr2;
	}
	writing = 0;
}

/*
 * Is file the edited file?
 * Work here is that it is not considered edited
 * if this is a partial buffer, and distinguish
 * all cases.
 */
edfile()
{

	if (!edited || !eq(file, savedfile))
		return (NOTEDF);
	return (addr1 == one && addr2 == dol ? EDF : PARTBUF);
}

/*
 * Extract the next line from the io stream.
 */
char *nextip;

getfile()
{
	register short c;
	register char *lp, *fp;

#ifndef	NLS16
	lp = linebuf;
#else
	lp = LINEBUF;
#endif

#if defined NLS || defined NLS16
	rl_inflip = 1;
#endif
	fp = nextip;
	do {
		if (--ninbuf < 0) {

#ifndef	NLS16
			ninbuf = read(io, genbuf, LBSIZE) - 1;
#else
			ninbuf = read(io, GENBUF, LBSIZE) - 1;
#endif

			if (ninbuf < 0) {

#ifndef	NLS16
				if (lp != linebuf) {
#else
				if (lp != LINEBUF) {
#endif

					lp++;
					printf((nl_msg(36, " [Incomplete last line]")));
					break;
				}
#if defined NLS || defined NLS16
				rl_inflip = 0;
#endif
				return (EOF);
			}
/* ========================================================================= */
/*
** CRYPT block 2
*/
#ifdef CRYPT
#ifndef NLS16
			fp = genbuf;
			while(fp < &genbuf[ninbuf]) {
#else
			fp = GENBUF;
			while(fp < &GENBUF[ninbuf]) {
#endif
				if (*fp++ & 0200) {
					if (kflag)
#ifndef NLS16
						crblock(perm, genbuf, ninbuf+1,
#else
						crblock(perm, GENBUF, ninbuf+1,
#endif
cntch);
					break;
				}
			}
#endif CRYPT
/* ========================================================================= */

#ifndef	NLS16
			fp = genbuf;
#else
			fp = GENBUF;
#endif

			cntch += ninbuf+1;
		}

		/***********
		* FSDlj06757
		* vi has problems (core dumps) when dealing with lines
		* greater than 1022 characters.  There seems to be an 
		* "internal limit" of allowing only 1022 chars per line,
		* so for now, put that limit on the input lines as well.
		***********/
#ifndef	NLS16
		if (lp >= &linebuf[LBSIZE-2]) {
#else
		if (lp >= &LINEBUF[LBSIZE-2]) {
#endif

			error((nl_msg(37, " Line too long")));
		}
		c = *fp++;
		if (c == 0) {
			cntnull++;
			continue;
		}

#ifdef NONLS8 /* 8bit integrity */
		if (c & QUOTE) {
			cntodd++;
			c &= TRIM;
			if (c == 0)
				continue;
		}
#endif NONLS8

		*lp++ = c;
	} while (c != '\n');
	*--lp = 0;
	nextip = fp;
	cntln++;

#ifdef	NLS16
	char_TO_CHAR(LINEBUF, linebuf);
#endif

	return (0);
}

/*
 * Write a range onto the io stream.
 */
putfile(isfilter)
int isfilter;
{
	line *a1;
	register char *fp, *lp;
	register int nib;

	a1 = addr1;
	clrstats();
	cntln = addr2 - a1 + 1;
	if (cntln == 0)
		return;
	nib = BUFSIZ;

#ifndef	NLS16
	fp = genbuf;
#else
	fp = GENBUF;
#endif

	do {

#ifndef	NLS16
		getline(*a1++);
		lp = linebuf;
#else
		GETLINE(*a1++);
		lp = LINEBUF;
#endif
#if defined NLS || defined NLS16
		if (right_to_left) {
			/* convert multi-line screen to key order */
			flip_line(lp,0);
			if (rl_order == NL_SCREEN) {
				/* convert key order to screen order */
				flip(lp);
			}
		}
#endif
		for (;;) {
			if (--nib < 0) {

#ifndef	NLS16
				nib = fp - genbuf;
#else
				nib = fp - GENBUF;
#endif

/* ========================================================================= */
/*
** CRYPT block 3
*/
#ifdef CRYPT
                		if(kflag && !isfilter)
#ifndef	NLS16
                                        crblock(perm, genbuf, nib, cntch);
#else
                                        crblock(perm, GENBUF, nib, cntch);
#endif
#endif CRYPT
/* ========================================================================= */

#ifndef	NLS16
				if (write(io, genbuf, nib) != nib) {
#else
				if (write(io, GENBUF, nib) != nib) {
#endif

					wrerror();
				}
				cntch += nib;
				nib = BUFSIZ - 1;

#ifndef	NLS16
				fp = genbuf;
#else
				fp = GENBUF;
#endif

			}
			if ((*fp++ = *lp++) == 0) {
				fp[-1] = '\n';
				break;
			}
		}
	} while (a1 <= addr2);

#ifndef	NLS16
	nib = fp - genbuf;
#else
	nib = fp - GENBUF;
#endif

/* ========================================================================= */
/*
** CRYPT block 4
*/
#ifdef CRYPT
	if(kflag && !isfilter)
#ifndef	NLS16
		crblock(perm, genbuf, nib, cntch);
#else
		crblock(perm, GENBUF, nib, cntch);
#endif
#endif CRYPT
/* ========================================================================= */

#ifndef	NLS16
	if (write(io, genbuf, nib) != nib) {
#else
	if (write(io, GENBUF, nib) != nib) {
#endif

		wrerror();
	}
	cntch += nib;
#ifdef hpux	/* fsync(2) does not exist on the AT&T 3B2 */
	fsync(io);
#endif hpux
}

/*
 * A write error has occurred;  if the file being written was
 * the edited file then we consider it to have changed since it is
 * now likely scrambled.
 */
wrerror()
{

	if (eq(file, savedfile) && edited)
		change();
	syserror(1);
}

/*
 * Source command, handles nested sources.
 * Traps errors since it mungs unit 0 during the source.
 */
short slevel;
short ttyindes;

source(fil, okfail)
	char *fil;
	bool okfail;
{
	jmp_buf osetexit;
	register int saveinp, ointty, oerrno;
	char *saveglobp;
	short savepeekc;

	signal(SIGINT, SIG_IGN);
#ifdef hpe
	saveinp = 47;
#else
	saveinp = dup(0);
#endif hpe
	savepeekc = peekc;
	saveglobp = globp;
	peekc = 0; globp = 0;
	if (saveinp < 0)
		error((nl_msg(38, "Too many nested sources")));
	if (slevel <= 0)
		ttyindes = saveinp;
	close(0);
#ifdef hpe
	if (open(fil, 0 | 020, 0, "T") < 0) {
#else
	if (open(fil, 0) < 0) {
#endif hpe
		oerrno = errno;
		setrupt();
#ifdef hpe
		open(_TTYIN,0);
		sTTY(0);
#else
		dup(saveinp);
		close(saveinp);
#endif hpe
		errno = oerrno;
		if (!okfail)
			filioerr(fil);
		return;
	}
	slevel++;
	ointty = intty;
	intty = isatty(0);
	oprompt = value(PROMPT);
	value(PROMPT) &= intty;
	getexit(osetexit);
	setrupt();
	if (setexit() == 0)
		commands(1, 1);
	else if (slevel > 1) {
		close(0);
#ifdef hpe
		open(_TTYIN,0);
		sTTY(0);
#else
		dup(saveinp);
		close(saveinp);
#endif hpe
		slevel--;
		resexit(osetexit);
		reset();
	}
	intty = ointty;
	value(PROMPT) = oprompt;
	close(0);
#ifdef hpe
	open(_TTYIN,0);
	sTTY(0);
#else
	dup(saveinp);
	close(saveinp);
#endif hpe
	globp = saveglobp;
	peekc = savepeekc;
	slevel--;
	resexit(osetexit);
}

/*
 * Clear io statistics before a read or write.
 */
clrstats()
{

	ninbuf = 0;
	cntch = 0;
	cntln = 0;
	cntnull = 0;

#ifdef NONLS8 /* 8bit integrity */
	cntodd = 0;
#endif NONLS8

}

/*
 * Io is finished, close the unit and print statistics.
 */
iostats()
{

#if defined NLS || defined NLS16
	RL_OKEY
#endif
/*FSDlj09532: 8.0 and before write(2) can't detect NFS filesystem full
  until close(2). The wrerror() below is meant to be a workaround for
  write(2). It is hope that write(2) will be fixed in 9.0 */
	if (close(io) < 0)
		wrerror();
	io = -1;
	if (hush == 0) {
		if (value(TERSE))
			printf(" %d/%D", cntln, cntch);
		else

#ifndef NONLS8 /* User messages */
			printf((nl_msg(39, " %d lines, %D characters")), cntln, cntch);
#else NONLS8
			printf(" %d line%s, %D character%s", cntln, plural((long) cntln),
			    cntch, plural(cntch));
#endif NONLS8

#ifndef NONLS8 /* 8bit integrity & User messages */
		if (cntnull)
			printf((nl_msg(40, " (%D null)")), cntnull);
#else NONLS8
		if (cntnull || cntodd) {
			printf(" (");
			if (cntnull) {
				printf("%D null", cntnull);
				if (cntodd)
					printf(", ");
			}
			if (cntodd)
				printf("%D non-ASCII", cntodd);
			putchar(')');
		}
#endif NONLS8

		noonl();
		flush();
	}

#if defined NLS || defined NLS16
	RL_OSCREEN
#endif

#ifndef NONLS8 /* 8bit integrity */
	return (cntnull != 0);
#else NONLS8
	return (cntnull != 0 || cntodd != 0);
#endif NONLS8

}

#ifndef	NLS16
#ifdef USG
/* It's so wonderful how we all speak the same language... */
# define index strchr
# define rindex strrchr
#endif
#else	/* revise index() and rindex() to prevent mistaking 2nd byte
	** of 16-bit character for a ANK character.
	*/
/* returns a pointer to the first occurance of  ANK character c */
char	*index(str, c)
register char	*str;
int	c;
{
	for ( ; *str; str++)
		if (FIRSTof2(*str) && SECONDof2(*(str+1)))
			str++;
		else if (*str == c)
			return(str);
	return(NULL);
}

/* returns a pointer to the last occurance of ANK character c */
char	*rindex(str, c)
register char	*str;
int	c;
{
	register char	*r;

	for (r = NULL; *str; str++)
		if (FIRSTof2(*str) && SECONDof2(*(str+1)))
			str++;
		else if (*str == c)
			r = str;
	return(r);
}
#endif

checkmodeline(line)
char *line;
{
        char *beg, *end, *scanptr;
        int newcmd, squote, dquote;
	char cmdbuf[1024];
	char *index(), *rindex();
	short opeekc;
	bool olaste;

	beg = index(line, ':');
	if (beg == NULL)
		return;
	if ( (beg-2) < line ) return;	/* don't index below line start */
    	else if (!(beg[-2] == 'e' && beg[-1] == 'x' ||
	           beg[-2] == 'v' && beg[-1] == 'i')) return;

	/* for root, scan for unallowed leading "!" shell escape */
	/* Ref. SR#5003080234 */
	if (!getuid())
		for (scanptr=beg+1,newcmd=1,squote=dquote=0; *scanptr != 0; scanptr++)
		  switch (*scanptr) 
		  {
		    case '\'': {		/* single quote string */
		      if (squote) squote=0;
		      else if (!dquote) squote=1;
		      break;
		      }
		    case '"': {			/* double quote string */
		      if (dquote) dquote=0;
		      else if (!squote) dquote=1;
		      break;
		      }
		    case ' ':     		/* skip over white space */
		    case '\t':		
		      break;
		    case '|':			/* join for new compound command */
		      if ( (!squote) && (!dquote) ) newcmd=1;
		      break;
		    case '!':			/* lead ! is unallowed shell escape */
		      if ( (!squote) && (!dquote) && (newcmd) ) return;
		      break;
		    default:			/* all else treated as ex command */
		      newcmd=0;
		      break;
		    }

	strncpy(cmdbuf, beg+1, sizeof cmdbuf);
	end = rindex(cmdbuf, ':');
	if (end == NULL)
		return;
	*end = 0;
	globp = cmdbuf;
	olaste = laste;
	opeekc = peekc;
	peekc = 0;		/* fix to force processing of globp */
	commands(1, 1);
	peekc = opeekc;
	laste = olaste;		/* fix to ensure can't undo initial file read */
}
