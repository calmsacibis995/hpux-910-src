static char *HPUX_ID = "@(#) $Revision: 70.5 $";

/* ar: UNIX Archive Maintainer */

/* This code is taken from the Bell V.2 source. The major difference
 * is in the way the archive symbol table (list of symbols and the offset
 * of the archive member they are defined in, old __.SYMDEF file created
 * by ranlib) is added to the archive. In the Bell code, it is done in
 * ar. In HP-UX we have different archive symbol table formats (ranlib.h)
 * for s500 and s200. So mksymtab() forks a call to ranlib, now located
 * in /usr/lib. This version of ranlib places the archive symbol table 
 * in a temporary file in the /usr/tmp, if there are any symbols
 * in the archive. mksymtab() gets the __.SYMDEF file and copies it 
 * as the first file of the archive, naming it "/" following the V.2
 * approach. Unlike Bell, our archive symbol table is NOT portable. Bell
 * uses sgetl() and sputl() to manipulate the archive symbol table ( it
 * is in char format). The reasons for leaving the archive symbol table
 * in a machine - dependent format are : (i) we don't have COFF, so an
 * archive containing object files would not really be "portable" (ii) it
 * would slow down the linker, since it would have to convert all the
 * offsets from characters to integers.
 */



#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ar.h>
#include <string.h>

#ifndef P_tmpdir
#define P_tmpdir "/usr/tmp"
#endif

#ifdef NLS || NLS16
#include <locale.h>
#include <nl_types.h>
#define NL_SETN 1
#else
#define catgets(i,mn,sn,s) (s)
#endif /* NLS || NLS16 */

#define	SKIP	1
#define	IODD	2
#define	OODD	4
#define	HEAD	8

#define	SUID	04000
#define	SGID	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000

#define SYMDIRNAME	"/               "	/* symbol directory filename */

#define	IO_BUFSIZ	(32*BUFSIZ)

struct stat	stbuf;

struct ar_hdr	ariobuf;       /* input/output copy of archive member header */

struct		/* usable copy of archive file member header */
{
	char	ar_name[16];
	long	ar_date;
	int	ar_uid;
	int	ar_gid;
	long	ar_mode;
	long	ar_size;
} arbuf;

char	*man	=	{ "mrxtdpq" };
#ifdef LONGFILENAMES
char	*opt	=	{ "uvcbailsf" };
#else
char	*opt	=	{ "uvcbails" };
#endif

int signum[] = {SIGHUP, SIGINT, SIGQUIT, SIGTERM, 0};
char	flg[26];
char	**namv;
int	namc;
char	*arnam;
char	*ponam;
char	*tmpdir = ""; /* default temp file directory to environment */
char	*tfnam;
char	*tf1nam;
char	*tf2nam;
char	*file;
char	name[16];
int	af;
int	tf;
int	tf1;
int	tf2;
int	qf;

int	bastate;

int	update;		/* was archive written or updated */
long	mem_ptr;	/* position of archive member in the archive */

#ifdef ACLS
int aclflag = 0;	/* display warning messages */
#endif

int	m1[] = { 1, ROWN, 'r', '-' };
int	m2[] = { 1, WOWN, 'w', '-' };
int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int	m4[] = { 1, RGRP, 'r', '-' };
int	m5[] = { 1, WGRP, 'w', '-' };
int	m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int	m7[] = { 1, ROTH, 'r', '-' };
int	m8[] = { 1, WOTH, 'w', '-' };
int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};


	/* declare all archive functions */
	
int	setcom(),	rcmd(),		dcmd(),		xcmd(),
	pcmd(),		mcmd(),		tcmd(),		qcmd(),
	init(),		getaf(),	getqf(),	usage(),
	noar(),		sigdone(),	done(),		notfound(),
	morefil(),	cleanup(),	install(),	movefil(),
	stats(),	copyfil(),	getdir(),	match(),
	bamatch(),	phserr(),	mesg(),		longt(),
	pmode(),	select(),	wrerr(),	mksymtab();
	
char	*trim(),	*trimslash();

char	*tempnam();
char	*ctime();
long	time();

int	(*comfun)();

#ifdef NLS || NLS16
nl_catd nl_fd;
nl_catd nl_tfd;		/* message catalogue - depends on LC_TIME */
struct locale_data *ld;
#endif


main(argc, argv)
	int argc;
	char **argv;
{
	register int i;
	register char *cp;
	char tmplang[SL_NAME_SIZE];

	for (i = 0; signum[i]; i++)
		if (signal(signum[i], SIG_IGN) != SIG_IGN)
			signal(signum[i], sigdone);

#ifdef NLS || NLS16
	if (!setlocale(LC_ALL,"")) {
		fputs(_errlocale(),stderr);
		nl_fd=nl_tfd=(nl_catd)-1;
	}
	else {
		nl_fd=catopen("ar",0);
		ld=getlocale(LOCALE_STATUS);
		if (strcmp(ld->LC_ALL_D,ld->LC_TIME_D)) {
			sprintf(tmplang,"LANG=%s",ld->LC_TIME_D);		
			putenv(tmplang);
			nl_tfd=catopen("ar",0);
			sprintf(tmplang,"LANG=%s",ld->LC_ALL_D);		
			putenv(tmplang);
		}
		else 
			nl_tfd=nl_fd;
	}
#endif /* NLS || NLS16 */

	if (argc < 3)
		usage();
	cp = argv[1];
	if (*cp == '-')	/* skip a '-', make ar more like other commands */
		cp++;
	for (; *cp; cp++)
	{
		switch (*cp)
		{
		case 'l':
		case 'v':
		case 'u':
		case 'a':
		case 'b':
		case 'c':
		case 'i':
		case 's':
#ifdef LONGFILENAMES
		case 'f':
#endif
			flg[*cp - 'a']++;
			continue;
		case 'r':
			setcom(rcmd);
			flg[*cp - 'a']++;
			continue;
		case 'd':
			setcom(dcmd);
			flg[*cp - 'a']++;
			continue;
		case 'x':
			setcom(xcmd);
			flg[*cp - 'a']++;
			continue;
		case 't':
			setcom(tcmd);
			flg[*cp - 'a']++;
			continue;
		case 'p':
			setcom(pcmd);
			flg[*cp - 'a']++;
			continue;
		case 'm':
			setcom(mcmd);
			flg[*cp - 'a']++;
			continue;
		case 'q':
			setcom(qcmd);
			flg[*cp - 'a']++;
			continue;
#ifdef ACLS
		case 'A':
			aclflag = 1;
			continue;
#endif
		default:
			fprintf(stderr, (catgets(nl_fd,NL_SETN,2, "ar: bad option `%c'\n")), *cp);
			done(1);
		}
	}
	if (flg['l' - 'a'])
		tmpdir = ".";	/* use local directory for temp files */
	if (flg['i' - 'a'])
		flg['b' - 'a']++;
	if (flg['a' - 'a'] || flg['b' - 'a'])
	{
		bastate = 1;
		ponam = trim(argv[2]);
		argv++;
		argc--;
		if (argc < 3)
			usage();
	}
	arnam = argv[2];
	namv = argv + 3;
	namc = argc - 3;
	if (comfun == 0)
	{
		if (flg['u' - 'a'] == 0)
		{
			fprintf(stderr,
				(catgets(nl_fd,NL_SETN,3, "ar: one of [%s] must be specified\n")), man);
			done(1);
		}
		setcom(rcmd);
	}
	update = (flg['d' - 'a'] | flg['q' - 'a'] | flg['m' - 'a'] |
		flg['r' - 'a'] | flg['u' - 'a'] | flg['s' - 'a']);
	(*comfun)();
	if (update)	/* make archive symbol table */
		mksymtab();
	done(notfound());
}



setcom(fun)
	int (*fun)();
{

	if (comfun != 0)
	{
		fprintf(stderr, (catgets(nl_fd,NL_SETN,4, "ar: only one of [%s] allowed\n")), man);
		done(1);
	}
	comfun = fun;
}



rcmd()
{
	register int f;

	init();
	getaf();
	while (!getdir())
	{
		bamatch();
		if (namc == 0 || match())
		{
			f = stats();
			if (f < 0)
			{
				if (namc)
					fprintf(stderr,
						(catgets(nl_fd,NL_SETN,5, "ar: cannot open %s\n")), file);
				goto cp;
			}
			if (flg['u' - 'a'])
				if (stbuf.st_mtime <= arbuf.ar_date)
				{
					close(f);
					goto cp;
				}
			mesg('r');
			copyfil(af, -1, IODD + SKIP);
			movefil(f);
			continue;
		}
	cp:
		mesg('c');
		copyfil(af, tf, IODD + OODD + HEAD);
	}
	cleanup();
}




dcmd()
{
	init();
	if (getaf())
		noar();
	while (!getdir())
	{
		if (match())
		{
			mesg('d');
			copyfil(af, -1, IODD + SKIP);
			continue;
		}
		mesg('c');
		copyfil(af, tf, IODD + OODD + HEAD);
	}
	install();
}




xcmd()
{
	register int f;

	if (getaf())
		noar();
	while (!getdir())
	{
		if (namc == 0 || match())
		{
			f = creat(file, arbuf.ar_mode & 0777);
			if (f < 0)
			{
				fprintf(stderr, (catgets(nl_fd,NL_SETN,6, "ar: %s cannot create\n")), file);
				goto sk;
			}
			mesg('x');
			copyfil(af, f, IODD);
			close(f);
			continue;
		}
	sk:
		mesg('c');
		copyfil(af, -1, IODD + SKIP);
		if (namc > 0  &&  !morefil())
			done(0);
	}
}




pcmd()
{
	if (getaf())
		noar();
	while (!getdir())
	{
		if (namc == 0 || match())
		{
			if (flg['v' - 'a'])
			{
				fprintf(stdout, "\n<%s>\n\n", file);
				fflush(stdout);
			}
			copyfil(af, 1, IODD);
			continue;
		}
		copyfil(af, -1, IODD + SKIP);
	}
}




mcmd()
{
	init();
	if (getaf())
		noar();
	tf2nam = tempnam(tmpdir, "ar");
	close(creat(tf2nam, 0600));
	tf2 = open(tf2nam, 2);
	if (tf2 < 0)
	{
		fprintf(stderr, (catgets(nl_fd,NL_SETN,7, "ar: cannot create third temp\n")));
		done(1);
	}
	while (!getdir())
	{
		bamatch();
		if (match())
		{
			mesg('m');
			copyfil(af, tf2, IODD + OODD + HEAD);
			continue;
		}
		mesg('c');
		copyfil(af, tf, IODD + OODD + HEAD);
	}
	install();
}




tcmd()
{
	if (getaf())
		noar();
	while (!getdir())
	{
		if (namc == 0 || match())
		{
			if (flg['v' - 'a'])
				longt();
			fprintf(stdout, "%s\n", trim(file));
		}
		copyfil(af, -1, IODD + SKIP);
	}
}




qcmd()
{
	register int i, f;

	if (flg['a' - 'a'] || flg['b' - 'a'])
	{
		fprintf(stderr, (catgets(nl_fd,NL_SETN,8, "ar: abi not allowed with q\n")));
		done(1);
	}
	getqf();
	for (i = 0; signum[i]; i++)
		signal(signum[i], SIG_IGN);
	lseek(qf, 0l, 2);
	for (i = 0; i < namc; i++)
	{
		file = namv[i];
		if (file == 0)
			continue;
		namv[i] = 0;
		mesg('q');
		f = stats();
		if (f < 0)
		{
			fprintf(stderr, (catgets(nl_fd,NL_SETN,9, "ar: %s cannot open\n")), file);
			continue;
		}
		tf = qf;
		movefil(f);
		qf = tf;
	}
}




init()
{
	tfnam = tempnam(tmpdir, "ar");
	close(creat(tfnam, 0600));
	tf = open(tfnam, 2);
	if (tf < 0)
	{
		fprintf(stderr, (catgets(nl_fd,NL_SETN,10, "ar: cannot create temp file\n")));
		done(1);
	}
	if (write(tf, ARMAG, sizeof(char) * SARMAG) != sizeof(char) * SARMAG)
		wrerr();
}




getaf()
{
	char buf[SARMAG + 1];
	long home;

	af = open(arnam, 0);
	if (af < 0)
		return (1);
	if (read(af, buf, sizeof(char) * SARMAG) != sizeof(char) * SARMAG ||
		strncmp(buf, ARMAG, SARMAG))
	{
		fprintf(stderr, (catgets(nl_fd,NL_SETN,11, "ar: %s not in archive format\n")), arnam);
		done(1);
	}
	/*
	* If the first entry is the symbol directory, skip it
	*/
	home = lseek(af, 0L, 1);
	if (!getdir() && file[0] == '\0')
	{
		copyfil(af, -1, IODD + SKIP);
		return (0);
	}
	/*
	* Otherwise, get back to beginning of first file header
	*/
	if (lseek(af, home, 0) == -1)
	{
		fprintf(stderr, (catgets(nl_fd,NL_SETN,12, "ar: %s cannot seek\n")), arnam);
		done(1);
	}
	return (0);
}




getqf()
{
	char buf[SARMAG + 1];

	if ((qf = open(arnam, 2)) < 0)		/* a new archive */
	{
		if (!flg['c' - 'a'])
			fprintf(stderr, (catgets(nl_fd,NL_SETN,13, "ar: creating %s\n")), arnam);
		close(creat(arnam, 0666));
		if ((qf = open(arnam, 2)) < 0)
		{
			fprintf(stderr, (catgets(nl_fd,NL_SETN,14, "ar: cannot create %s\n")), arnam);
			done(1);
		}
		if (write(qf, ARMAG, sizeof(char) * SARMAG) !=
			sizeof(char) * SARMAG)
		{
			wrerr();
		}
	}
	else if (read(qf, buf, sizeof(char) * SARMAG) !=
		sizeof(char) * SARMAG || strncmp(buf, ARMAG, SARMAG))
	{
		fprintf(stderr, (catgets(nl_fd,NL_SETN,15, "ar: %s not in archive format\n")), arnam);
		done(1);
	}
	return (0);
}




usage()
{
	fprintf(stderr, (catgets(nl_fd,NL_SETN,16, "usage: ar [%s][%s] [posname] archive files ...\n")),
		man, opt);
	done(1);
}




noar()
{
	fprintf(stderr, (catgets(nl_fd,NL_SETN,17, "ar: %s does not exist\n")), arnam);
	done(1);
}


sigdone(sig)
int sig;
{
	int i;

	/* ignore signals */
	for (i = 0; signum[i]; i++)
		signal(signum[i], SIG_IGN);

	rmfiles();

	/* set signals to default values */
	for (i = 0; signum[i]; i++)
		signal(signum[i], SIG_DFL); 

	kill(0, sig);
}


done(c)
	int c;
{
	rmfiles();
	exit(c);
}

rmfiles()
{
	if (tfnam)
		unlink(tfnam);
	if (tf1nam)
		unlink(tf1nam);
	if (tf2nam)
		unlink(tf2nam);
}


notfound()
{
	register int i, n;

	n = 0;
	for (i = 0; i < namc; i++)
		if (namv[i])
		{
			fprintf(stderr, (catgets(nl_fd,NL_SETN,18, "ar: %s not found\n")), namv[i]);
			n++;
		}
	return (n);
}




morefil()
{
	register int i, n;

	n = 0;
	for (i = 0; i < namc; i++)
		if (namv[i])
			n++;
	return (n);
}




cleanup()
{
	register int i, f;

	for (i = 0; i < namc; i++)
	{
		file = namv[i];
		if (file == 0)
			continue;
		namv[i] = 0;
		mesg('a');
		f = stats();
		if (f < 0)
		{
			fprintf(stderr, (catgets(nl_fd,NL_SETN,19, "ar: %s cannot open\n")), file);
			continue;
		}
		movefil(f);
	}
	install();
}




install()
{
	register int i;
	char	buf[IO_BUFSIZ];

	for (i = 0; signum[i]; i++)
		signal(signum[i], SIG_IGN);
	if (af < 0)
	{
		if (!flg['c' - 'a'])
			fprintf(stderr, (catgets(nl_fd,NL_SETN,20, "ar: creating %s\n")), arnam);
	}
	close(af);
	af = creat(arnam, 0666);
	if (af < 0)
	{
		fprintf(stderr, (catgets(nl_fd,NL_SETN,21, "ar: cannot create %s\n")), arnam);
		done(1);
	}
	if (tfnam)
	{
		lseek(tf, 0l, 0);
		while ((i = read(tf, buf, IO_BUFSIZ)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
	if (tf2nam)
	{
		lseek(tf2, 0l, 0);
		while ((i = read(tf2, buf, IO_BUFSIZ)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
	if (tf1nam)
	{
		lseek(tf1, 0l, 0);
		while ((i = read(tf1, buf, IO_BUFSIZ)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
}



/*
* insert the file 'file' into the temporary file
*/
movefil(f)
	int f;
{
	(void)strncpy(arbuf.ar_name, trim(file), sizeof(arbuf.ar_name));
	arbuf.ar_size = stbuf.st_size;
	arbuf.ar_date = stbuf.st_mtime;
	arbuf.ar_uid = stbuf.st_uid;
	arbuf.ar_gid = stbuf.st_gid;
	arbuf.ar_mode = stbuf.st_mode;
	copyfil(f, tf, OODD + HEAD);
	close(f);
}



stats()
{
	register int f;

	f = open(file, 0);
	if (f < 0)
		return(f);
	if (fstat(f, &stbuf) < 0)
	{
		close(f);
		return(-1);
	}
#ifdef ACLS
	if(!aclflag) {
	    if(stbuf.st_acl)
	        fprintf(stderr, (catgets(nl_fd,NL_SETN,22, "optional acl entries for %s not archived\n")),file);
	}
#endif
	return (f);
}




/*
* copy next file size given in arbuf
*/
copyfil(fin, fout, flag)
	int fin, fout, flag;
{
	char	buf[IO_BUFSIZ];
	register int i, o;
	int pe;

	if (flag & HEAD)
	{
		char buf[sizeof(ariobuf) + 1];
		char *fname;

		if (strncmp(file, SYMDIRNAME, 16))
		       fname = trimslash(file);
		else   fname = file;

		if (sprintf(buf, "%-16s%-12ld%-6u%-6u%-8o%-10ld%-2s",
			fname, arbuf.ar_date, arbuf.ar_uid,
			arbuf.ar_gid, arbuf.ar_mode, arbuf.ar_size,
			ARFMAG) != sizeof(ariobuf))
		{
			fprintf(stderr,
				(catgets(nl_fd,NL_SETN,23, "ar: %s internal header generation error\n")),
				arnam);
			done(1);
		}
		(void)strncpy((char *)&ariobuf, buf, sizeof(ariobuf));
		if (write(fout, &ariobuf, sizeof(ariobuf)) != sizeof(ariobuf))
			wrerr();
	}
	pe = 0;
	while (arbuf.ar_size > 0)
	{
		i = o = IO_BUFSIZ;
		if (arbuf.ar_size < i)
		{
			i = o = arbuf.ar_size;
			if (i & 1)
			{
				buf[i] = '\n';
				if (flag & IODD)
					i++;
				if (flag & OODD)
					o++;
			}
		}
		if (read(fin, buf, i) != i)
			pe++;
		if ((flag & SKIP) == 0)
			if (write(fout, buf, o) != o)
				wrerr();
		arbuf.ar_size -= IO_BUFSIZ;
	}
	if (pe)
		phserr();
}




getdir()
{
	register char *cp;
	register int i;

	i = read(af, (char *)&ariobuf, sizeof(ariobuf));
	if (i != sizeof(ariobuf))
	{
		if (tf1nam)
		{
			i = tf;
			tf = tf1;
			tf1 = i;
		}
		return (1);
	}
	if (strncmp(ariobuf.ar_fmag, ARFMAG, sizeof(ariobuf.ar_fmag)))
	{
		fprintf(stderr, (catgets(nl_fd,NL_SETN,24, "ar: malformed archive (at %ld)\n")),
			lseek(af, 0L, 1));
		done(1);
	}
	cp = ariobuf.ar_name + sizeof(ariobuf.ar_name);
	while (*--cp == ' ')
		;
	if (*cp == '/')
		*cp = '\0';
	else
		*++cp = '\0';
	file = strncpy(name, ariobuf.ar_name, sizeof(name));
	(void)strncpy(arbuf.ar_name, name, sizeof(arbuf.ar_name));
	if (sscanf(ariobuf.ar_date, "%ld", &arbuf.ar_date) != 1 ||
		sscanf(ariobuf.ar_uid, "%d", &arbuf.ar_uid) != 1 ||
		sscanf(ariobuf.ar_gid, "%d", &arbuf.ar_gid) != 1 ||
		sscanf(ariobuf.ar_mode, "%o", &arbuf.ar_mode) != 1 ||
		sscanf(ariobuf.ar_size, "%ld", &arbuf.ar_size) != 1)
	{
		fprintf(stderr, (catgets(nl_fd,NL_SETN,25, "ar: %s bad header layout for %s\n")),
			arnam, name);
		done(1);
	}
	return (0);
}




match()
{
	register int i;

	for (i = 0; i < namc; i++)
	{
		if (namv[i] == 0)
			continue;
#ifdef LONGFILENAMES
		if (strcmp(trim(namv[i]),file) == 0 ||
		    (flg['f'-'a'] && strncmp(trim(namv[i]),file,14) == 0))
#else
		if (strcmp(trim(namv[i]), file) == 0)
#endif
		{
			file = namv[i];
			namv[i] = 0;
			return (1);
		}
	}
	return (0);
}




bamatch()
{
	register int f;

	switch (bastate)
	{
	case 1:
		if (strcmp(file, ponam) != 0)
			return;
		bastate = 2;
		if (flg['a' - 'a'])
			return;
	case 2:
		bastate = 0;
		tf1nam = tempnam(tmpdir, "ar");
		close(creat(tf1nam, 0600));
		f = open(tf1nam, 2);
		if (f < 0)
		{
			fprintf(stderr, (catgets(nl_fd,NL_SETN,26, "ar: cannot create second temp\n")));
			return;
		}
		tf1 = tf;
		tf = f;
	}
}




phserr()
{
	fprintf(stderr, (catgets(nl_fd,NL_SETN,27, "ar: phase error on %s\n")), file);
	done(1);
}




mesg(c)
	int c;
{
	if (flg['v' - 'a'])
		if (c != 'c' || flg['v' - 'a'] > 1)
			fprintf(stdout, "%c - %s\n", c, file);
}




char *
trimslash(s)
	char *s;
{
	static char buf[sizeof(arbuf.ar_name)];

	(void)strncpy(buf, trim(s), sizeof(arbuf.ar_name) - 2);
	buf[sizeof(arbuf.ar_name) - 2] = '\0';
	return (strcat(buf, "/"));
}


char *
trim(s)
	char *s;
{
	register char *p1, *p2;

	for (p1 = s; *p1; p1++)
		;
	while (p1 > s)
	{
		if (*--p1 != '/')
			break;
		*p1 = 0;
	}
	p2 = s;
	for (p1 = s; *p1; p1++)
		if (*p1 == '/')
			p2 = p1 + 1;
	return (p2);
}




longt()
{
	register char *cp;

#ifdef NLS
	extern int __nl_langid[];
#endif
	pmode();
	fprintf(stdout, "%6d/%6d", arbuf.ar_uid, arbuf.ar_gid);
	fprintf(stdout, "%7ld", arbuf.ar_size);
#ifdef NLS || NLS16
	if (__nl_langid[LC_TIME] == 0 || __nl_langid[LC_TIME] == 99) {
		cp=ctime(&arbuf.ar_date);
		fprintf(stdout, " %-12.12s %-4.4s ", cp + 4, cp + 20);
	}
	else {
		cp=nl_cxtime(&arbuf.ar_date,catgets(nl_tfd,NL_SETN,1,"%4h %2d %H:%M 19%y"));
		fprintf(stdout," %s ",cp); 
	}
#else 
	cp=ctime(&arbuf.ar_date);
	fprintf(stdout, " %-12.12s %-4.4s ", cp + 4, cp + 20);
#endif /* NLS || NLS16 */
}




pmode()
{
	register int **mp;

	for (mp = &m[0]; mp < &m[9];)
		select(*mp++);
}




select(pairp)
	int *pairp;
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n >= 0 && (arbuf.ar_mode & *ap++) == 0)
		ap++;
	putchar(*ap);
}




wrerr()
{
	perror((catgets(nl_fd,NL_SETN,28, "ar write error")));
	done(1);
}


#define max(A,B) (((A) < (B)) ? (B) : (A))

extern char	*getenv(),	*mktemp();
extern int	access();
static char	*pcopy(),	*seed="AAA";



char *
tempnam(dir, pfx)
	char *dir;		/* use this directory please (if non-NULL) */
	char *pfx;		/* use this (if non-NULL) as filename prefix */
{
	register char *p, *q, *tdir;
	int x = 0, y = 0, z;

	z = strlen(P_tmpdir);
	if ((tdir = getenv("TMPDIR")) != NULL)
		x = strlen(tdir);
	if (dir != NULL)
		y = strlen(dir);
	if ((p = malloc((unsigned)(max(max(x, y), z) + 16))) == NULL)
		return (NULL);

        /* the order of the following four conditionals determines the
	   tmp directory priority */

	if (y > 0 && access(pcopy(p, dir), 3) == 0)
		goto OK;
	if (x > 0 && access(pcopy(p, tdir), 3) == 0)
		goto OK;
	if (access(pcopy(p, P_tmpdir), 3) == 0)
		goto OK;
	if (access(pcopy(p, "/tmp"), 3) != 0)
		return (NULL);
OK:
	(void)strcat(p, "/");
	if (pfx)
	{
		*(p + strlen(p) + 5) = '\0';
		(void)strncat(p, pfx, 5);
	}
	(void)strcat(p, seed);
	(void)strcat(p, "XXXXXX");
	q = seed;
	while (*q == 'Z')
		*q++ = 'A';
	++*q;
	if (*mktemp(p) == '\0')
		return (NULL);
	return (p);
}



static char *
pcopy(space, arg)
	char *space, *arg;
{
	char *p;

	if (arg)
	{
		(void)strcpy(space, arg);
		p = space - 1 + strlen(space);
		if (*p == '/')
			*p = '\0';
	}
	return (space);
}

/*********************** Start 300 symbol table routines ****************/

#ifdef __hp9000s300

#include	<sys/wait.h>

mksymtab()
{
        int f, stat;
	char * symtabfile;
	char * failmsg = "ar: \"/usr/lib/ar.ranlib %s %s\" failed\n";

	symtabfile = tempnam(tmpdir, "ar");
	if (vfork())
	{
		if (wait(&stat) == -1)
			fprintf(stderr, failmsg, arnam, symtabfile);
		else if (WIFSIGNALED(stat))
			kill(0,WTERMSIG(stat));
		else if (!(WIFEXITED(stat)) || WEXITSTATUS(stat) != 0)
			fprintf(stderr, failmsg, arnam, symtabfile);
	}
	else
	{
		execl("/usr/lib/ar.ranlib","ar.ranlib",arnam,symtabfile,NULL);
		fprintf(stderr, failmsg, arnam, symtabfile);
	}

 	/*
 	* clean up the garbage that may have been left around
 	*/
 	unlink(tfnam);
 	tfnam = 0;
 	unlink(tf1nam);
 	tf1nam = 0;
 	unlink(tf2nam);
 	tf2nam = 0;
 	init();		/* rewrite the archive header */
 	
	/* get the __.SYMDEF file and copy it as first file of the 
           archive.
        */

        file = symtabfile;
        f = stats();
	/* if no symbols in archive, ranlib will not produce a __.SYMDEF file */
        if (f < 0) return ;
        file = SYMDIRNAME;
        movefil(f);
        unlink(symtabfile);

 	/*
 	* copy the rest of the archive to finish up
 	*/
 	getaf();		/* skip past the old header */
 	while (!getdir())
 		copyfil(af, tf, IODD + OODD + HEAD);
 	install();
 
 
 
}

#endif 	/* __hp9000s300 */

/*********************** End   300 symbol table routines ****************/

/*********************** Start 800 symbol table routines ****************/

#ifdef __hp9000s800

#include "filehdr.h"
#include "lst.h"
#include "syms.h"

/* Declarations for the symbol directory (LST) */

#define HASHSIZE	31
#define MAXSOMS		1024

#define TABSZ   4096
#define NSEG    128
#define PAD_VAL 20

FILHDR  ml_filhdr;      /* COFF machine language file header */
int     nsyms;          /* nbr of symbol directory entries */
long    mem_skip;       /* adjustment to add to mem_ptr for "real" position */

int     filenum = 0;    /* incremented by ml_file() for getname()'s benefit */

char    *str_base,      /* start of string table for names */
	*str_top;       /* pointer to next available location */

int 	ml_file(),	getname();
unsigned int		gethashkey();

struct lst_symbol_record *sym[NSEG];	/* symbol directory */
struct som_entry *somdir[NSEG];		/* SOM directory */
int hash_table[HASHSIZE];		/* symbol hash table */
struct lst_header lsthdr;		/* LST header */
int som_offset;				/* offset of som within file */

mksymtab()
{
	int i, j, offset, nsoms, nsegs, somdirseg, somdiroff;
	int *p;
	struct lst_symbol_record *s, *seg;
	char buf[sizeof(ariobuf) + 1];

	for (i = 0; i < NSEG; i++)
		sym[i] = NULL;

	if (getaf())
	{
		fprintf(stderr, "ar: cannot make symbol directory\n");
		done(1);
	}
	nsoms = 0;
	nsegs = 0;
	nsyms = 0;
	mem_ptr = sizeof(char) * SARMAG;
	mem_skip = lseek(af, 0L, 1) - mem_ptr;
	while (!getdir())
	{
		/*
		* scan machine language file members for symbols
		*/
		if ((i = ml_file()) >= 0)  /* read the symbol table */
		{
			if (nsoms > MAXSOMS*NSEG)
			{
				fprintf(stderr, "ar:  too many object files\n");
				done(1);
			}
			somdirseg = nsoms / MAXSOMS;
			somdiroff = nsoms % MAXSOMS;
			while (somdirseg >= nsegs) {
				somdir[nsegs] =
				    malloc(MAXSOMS * sizeof(struct som_entry));
				if (somdir[nsegs] == NULL) {
					fprintf(stderr, "ar: out of memory\n");
					done(1);
				}
				nsegs++;
			}
			/* relocate location field later */
			somdir[somdirseg][somdiroff].location = mem_ptr;
			somdir[somdirseg][somdiroff].length = arbuf.ar_size;

			do {	/* loop for each som in file */
			    static SYMENT *sym_array = NULL;
			    static int sym_array_size = 0;
			    SYMENT *symbol;  /* working ptr into sym_array */
			    int sym_type;

			    if ((sym_array == NULL) || (sym_array_size < i)) {
				/* allocate larger buffer */
				sym_array_size = i + PAD_VAL + (i >> 3); /* 12.5% pad */
				if (sym_array != NULL)
				    free(sym_array);
				sym_array = (SYMENT *) malloc(SYMESZ*sym_array_size);
				if (sym_array == NULL) {
				    fprintf(stderr, "ar: out of memory\n");
				    done(1);
				    }
				}

			    if (read(af, (char *)sym_array, SYMESZ*i) != SYMESZ*i)
				{
					fprintf(stderr, "ar: internal error, archive %s out of order!\n",
						arnam);
					done(1);
				}

			    while (i > 0)  /* loop for each symbol in som */
			    {
				i--;
				symbol = &sym_array[i];

				/*
				* check out symbol, is it
				* exported or storage request?
				*/
				sym_type = symbol->symbol_type;
				if ((sym_type != ST_NULL &&
				     sym_type != ST_SYM_EXT &&
				     sym_type != ST_ARG_EXT &&
				     symbol->symbol_scope == SS_UNIVERSAL) ||
				    sym_type == ST_STORAGE)
				{
					if (nsyms >= TABSZ * NSEG) {
						fprintf(stderr,
					   "ar: too many external symbols\n");
						done(1);
					}
					seg = sym[nsyms/TABSZ];
					if (seg == NULL) {
						seg = malloc(TABSZ * LSTSYMESZ);
						if (seg == NULL) {
							fprintf(stderr,
						       "ar: out of memory\n");
							done(1);
						}
						sym[nsyms/TABSZ] = seg;
					}
					s = &seg[nsyms%TABSZ];
					*(SYMENT *)s = *symbol;
					for (j = SYMESZ/sizeof(int);
					     j < LSTSYMESZ/sizeof(int); j++)
						((int *)s)[j] = 0;
					s->som_index = nsoms;
					s->name.n_strx = 
					    getname(s->name.n_strx);
					s->qualifier_name.n_strx = 
					    getname(s->qualifier_name.n_strx);
					s->symbol_key = gethashkey(s);
					nsyms++;
				}
			    }
			} while ((i = next_som()) >= 0);

			nsoms++;
		}
		/*
		* Be careful with odd length .o files (now possible)
		*/
		mem_ptr += ((arbuf.ar_size + 1) & ~01) + sizeof(struct ar_hdr);
		/*
		* Skip string table (if any)
		*/
		if (lseek(af, mem_ptr + mem_skip, 0) == -1)
		{
			fprintf(stderr,
				"ar: %s cannot skip string table for %s\n",
				arnam, file);
			done(1);
		}
	}
	/*
	* rewrite the archive to include the symbol directory
	*/
	close(af);
	/*
	* clean up the garbage that may have been left around
	*/
	unlink(tfnam);
	tfnam = 0;
	unlink(tf1nam);
	tf1nam = 0;
	unlink(tf2nam);
	tf2nam = 0;

	/* Write the archive header */
	init();

	/* Write the LST if there were any symbols */
	if (nsoms > 0)
	{
		/* Initialize the LST header */
		lsthdr.system_id = HP9000S800_ID;
		lsthdr.a_magic = LIBMAGIC;
		lsthdr.version_id = VERSION_ID;
		lsthdr.file_time.secs = 0;
		lsthdr.file_time.nanosecs = 0;
		lsthdr.hash_loc = sizeof(lsthdr);
		lsthdr.hash_size = HASHSIZE;
		lsthdr.module_count = nsoms;
		lsthdr.module_limit = nsoms;
		lsthdr.dir_loc = lsthdr.hash_loc + sizeof(hash_table);
		lsthdr.export_loc = lsthdr.dir_loc +
				nsoms * sizeof(struct som_entry);
		lsthdr.export_count = nsyms;
		lsthdr.import_loc = 0;
		lsthdr.aux_loc = 0;
		lsthdr.aux_size = 0;
		lsthdr.string_loc = lsthdr.export_loc + nsyms * LSTSYMESZ;
		lsthdr.string_size = str_top - str_base;
		lsthdr.free_list = 0;
		lsthdr.file_end = 0;	/* computed later */
		lsthdr.checksum = 0;	/* computed later */

		/* Build the hash table */
		for (i = 0; i < HASHSIZE; i++)
			hash_table[i] = 0;
		offset = lsthdr.export_loc;
		for (i = 0; i < nsyms; i++)
		{
			s = &sym[i/TABSZ][i%TABSZ];
			j = s->symbol_key % HASHSIZE;
			s->next_entry = hash_table[j];
			hash_table[j] = offset;
			offset += LSTSYMESZ;
		}

		/* Fill in the SOM location fields in somdir.             */
		/* SOM pointers are absolute file offsets; include the    */
		/* size of the archive headers.				  */
		offset = 2 * sizeof(ariobuf) + 
			 lsthdr.string_loc + lsthdr.string_size;
		for (i = 0; i < nsoms; i++)
			somdir[i/MAXSOMS][i%MAXSOMS].location += offset;

		/* Set the file_end field and compute the checksum */
		lsthdr.file_end = mem_ptr - sizeof(char) * SARMAG +
			lsthdr.string_loc + lsthdr.string_size;
		p = (int *)&lsthdr;
		for (i = 0; i < (SLSTHDR/sizeof(int))-1; i++)
			lsthdr.checksum ^= *p++;

		/* Write out the archive header for the LST */
		sprintf(buf, "%-16s%-12ld%-6u%-6u%-8o%-10ld%-2s",
			SYMDIRNAME, time(0), 0, 0, 0,
			lsthdr.string_loc + lsthdr.string_size,
			ARFMAG);
		if (strlen(buf) != sizeof(ariobuf))
		{
			fprintf(stderr,
				"ar: %s internal header generation error\n",
				arnam);
			done(1);
		}
		if (write(tf, buf, sizeof(ariobuf)) != sizeof(ariobuf))
			wrerr();

		/* Write out the LST header */
		if (write(tf, (char *)&lsthdr, SLSTHDR) != SLSTHDR)
			wrerr();

		/* Write out the hash table */
		if (write(tf, (char *)hash_table, sizeof(hash_table)) != sizeof(hash_table))
			wrerr();

		/* Write out the SOM directory */
		for (i = 0; i < nsoms; i += MAXSOMS) {
			somdirseg = i/MAXSOMS;
			j = nsoms - i;
			if (j > MAXSOMS)
				j = MAXSOMS;
			if (write(tf, (char *)somdir[somdirseg],
					j * sizeof(struct som_entry)) !=
					j * sizeof(struct som_entry))
				wrerr();
			free(somdir[somdirseg]);
		}

		/* Write out the symbol records */
		for (i = 0; i < nsyms; i += TABSZ) {
			seg = sym[i/TABSZ];
			j = nsyms - i;
			if (j > TABSZ)
				j = TABSZ;
			if (write(tf, (char *)seg, j * LSTSYMESZ) !=
					j * LSTSYMESZ)
				wrerr();
			free(seg);
		}

		/* Write out the string table */
		if (write(tf, str_base, str_top - str_base) != str_top - str_base)
			wrerr();

	}
	/*
	* copy the rest of the archive to finish up
	*/
	getaf();		/* skip past the old header */
	while (!getdir())
		copyfil(af, tf, IODD + OODD + HEAD);
	install();
}


ml_file()
{
	long save_size;

	/*
	* is this a recognizable machine language file
	* if so, then skip down to the beginning of the symbol table
	* function returns the number of symbol table entries
	* if not, then skip to the next file
	*/
	som_offset = 0;
	filenum++;
	if (arbuf.ar_size <= sizeof(ml_filhdr))		/* don't bother */
	{
		save_size = arbuf.ar_size;
		copyfil(af, -1, IODD + SKIP);
		arbuf.ar_size = save_size;
		return (-1);
	}
	/*
	* check the "magic" number
	*/
	if (read(af, (char *)&ml_filhdr, sizeof(ml_filhdr))
		!= sizeof(ml_filhdr))
	{
		fprintf(stderr,
			"ar: internal error, archive %s out of order!\n",arnam);
		done(1);
	}
	switch (ml_filhdr.a_magic)
	{
	default:	/* skip to the next archive file member */
	skip_it:;
		save_size = arbuf.ar_size;
		arbuf.ar_size -= sizeof(ml_filhdr);
		copyfil(af, -1, IODD + SKIP);
		arbuf.ar_size = save_size;
		return (-1);

	case RELOC_MAGIC:
	case EXEC_MAGIC:
	case SHARE_MAGIC:
	case DEMAND_MAGIC:

		if (!_PA_RISC_ID(ml_filhdr.system_id))
  		{
			/* non SOM format, do not add symbols to table */
                        goto skip_it;
                }

		/*
		* skip to the beginning of the symbol table and return the
		* number of symbols in the table
		*/
		save_size = arbuf.ar_size;
		if (ml_filhdr.symbol_location < 0 || 
		    ml_filhdr.symbol_location > save_size)
		{
			fprintf(stderr, "ar: (warning) file %.16s pretends to be an object file\n",
				arbuf.ar_name);
			goto skip_it;
		}
		arbuf.ar_size = ml_filhdr.symbol_location - sizeof(ml_filhdr);
		copyfil(af, -1, IODD + SKIP);
		arbuf.ar_size = save_size;
		return (ml_filhdr.symbol_total); /* return the nbr of entries */
	}
}

next_som()
{
	long save_size;

	/*
	 * If there is another som in the file, then skip
	 * to the beginning of its symbol table.  If not,
	 * just return -1.
	 */
	som_offset += ml_filhdr.som_length;
	if (som_offset >= arbuf.ar_size)
		return (-1);

	if (lseek(af,
		  mem_ptr + mem_skip + som_offset + sizeof(struct ar_hdr),
		  0) == -1)
	{
		fprintf(stderr, "ar: %s cannot seek to next module in %s\n",
			arnam, file);
		done(1);
	}

	/*
	* function returns the number of symbol table entries
	* if not an object module, then skip to the next file
	*/
	filenum++;
	if (arbuf.ar_size-som_offset <= sizeof(ml_filhdr))  /* don't bother */
	{
		save_size = arbuf.ar_size;
		arbuf.ar_size -= som_offset;
		copyfil(af, -1, IODD + SKIP);
		arbuf.ar_size = save_size;
		return (-1);
	}
	/*
	* check the "magic" number
	*/
	if (read(af, (char *)&ml_filhdr, sizeof(ml_filhdr))
		!= sizeof(ml_filhdr))
	{
		fprintf(stderr,
			"ar: internal error, archive %s out of order!\n",arnam);
		done(1);
	}
	switch (ml_filhdr.a_magic)
	{
	default:	/* skip to the next archive file member */
		fprintf(stderr, "ar: warning: object file %s contains garbage",
			file);
	skip_it:;
		save_size = arbuf.ar_size;
		arbuf.ar_size -= sizeof(ml_filhdr) + som_offset;
		copyfil(af, -1, IODD + SKIP);
		arbuf.ar_size = save_size;
		return (-1);

	case RELOC_MAGIC:
	case EXEC_MAGIC:
	case SHARE_MAGIC:
	case DEMAND_MAGIC:
		/*
		* skip to the beginning of the symbol table and return the
		* number of symbols in the table
		*/
		save_size = arbuf.ar_size;
		if (ml_filhdr.symbol_location < 0 || 
		    ml_filhdr.symbol_location > save_size)
		{
			fprintf(stderr, "ar: (warning) file %.16s pretends to be an object file\n",
				arbuf.ar_name);
			goto skip_it;
		}
		arbuf.ar_size = ml_filhdr.symbol_location - sizeof(ml_filhdr);
		copyfil(af, -1, IODD + SKIP);
		arbuf.ar_size = save_size;
		return (ml_filhdr.symbol_total); /* return the nbr of entries */
	}
}


getname(name)
	unsigned int name;
{
	char *p;
	register int i;
	static int str_length = BUFSIZ * 5;
	static int lastfilenum = 0;
	static char *strtab = NULL;
	static long strtablen = 0;

	if (name <= 0)
		return (name);
	if (str_base == (char *)0)	/* no space allocated yet */
	{
		if ((str_base = malloc((unsigned)str_length)) == NULL)
		{
			fprintf(stderr,
				"ar: %s cannot get string table space\n",
				arnam);
			done(1);
		}
		str_top = str_base;
	}

	if (lastfilenum != filenum || strtab == NULL)	/* read it in */
	{
		long home = lseek(af, 0L, 1);
		long len;

		if (strtab != NULL)
			free(strtab);
		len = ml_filhdr.symbol_strings_location;
		strtablen = ml_filhdr.symbol_strings_size;
		if (strtablen < 4L)	/* room for string table */
		{
			fprintf(stderr,
				"ar: %s missing string table for %s\n",
				arnam, file);
			done(1);
		}
		len += sizeof(struct ar_hdr) + mem_ptr + mem_skip + som_offset;
		if ((strtab = malloc((unsigned)strtablen)) == NULL ||
			lseek(af, len, 0) == -1 ||
			read(af, strtab, strtablen) != strtablen ||
			lseek(af, home, 0) == -1 ||
			strtab[strtablen - 1] != '\0')
		{
			fprintf(stderr,
				"ar: %s bad strings table for %s\n",
				arnam, file);
			done(1);
		}
		lastfilenum = filenum;
	}
	if (name < 4 || name >= strtablen)
	{
		fprintf(stderr,
			"ar: %s bad string table offset for %s\n",
			arnam, file);
		done(1);
	}
	p = str_top;
	/* Make sure that str_top is always full-word aligned */
	str_top += (strlen(strtab + name) + 2*sizeof(int)) &
		   ~(sizeof(int)-1);
	/* Reallocate the string area if necessary */
	if (str_top > str_base + str_length)
	{
		char *old_base = str_base;
		int diff;

		str_length += BUFSIZ * 2;
		if ((str_base = realloc(str_base, str_length)) == NULL)
		{
			fprintf(stderr,
				"ar: %s cannot grow string table\n",
				arnam);
			done(1);
		}
		/*
		* Re-adjust other pointers
		*/
		diff = str_base - old_base;
		p += diff;
		str_top += diff;
	}

	/* Put the length of the string out just before the string itself */
	*(int *)p = strlen(strtab + name);
	p += sizeof(int);
	(void)strcpy(p, strtab + name);
	return (p-str_base);
}

unsigned int
gethashkey(symbol)
struct lst_symbol_record *symbol;
{
	char *s;
	register unsigned int len, key, masklen;

	/* assumes getname has just been called */
	s = str_base + symbol->name.n_strx ;
	len = *(int *)(s - sizeof(int));
	masklen = len & 0x7f;

	switch (len) {
	    case 0:
		fprintf(stderr, "ar: zero length string name\n");
		done(1);
		break;
	    case 1:
		key = (1 << 24) | (1 << 8) | (s[0] << 16) | s[0];
		break;
	    default:
		key = (masklen<<24) | (s[1]<<16) | (s[len-2]<<8) | s[len-1];
		break;
	}
	return (key);
}


#endif 	/* __hp9000s800 */

/*********************** End   800 symbol table routines ****************/


