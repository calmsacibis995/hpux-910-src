static char *HPUX_ID = "@(#) $Revision: 64.1 $";
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <ndir.h>

#ifndef NLS
#define catgets(i,sn,mn,s) (s)
#else NLS
#define NL_SETN 2	/* set number */
#include <locale.h>
#include <nl_types.h>
#include <nl_ctype.h>
nl_catd nlmsg_fd;
#endif NLS


#ifndef NONLS
#define NLSMANDIR	"/usr/man/%s/man"
#endif

static char *bindirs[] = {
	"/etc",
	"/bin",
	"/usr/bin",
	"/usr/games",
	"/lib",
	"/usr/lib",
	"/usr/local/bin",
	"/usr/local/games",
	"/usr/local/include",
	"/usr/local/lib",
	"/usr/contrib/bin",
	"/usr/contrib/games",
	"/usr/contrib/include",
	"/usr/contrib/lib",
	0
};
static char *mandirs[] = {
	"/usr/man/man",
	"/usr/local/man/man",
	"/usr/contrib/man/man",
	0
};
static char *zmandirs[] = {
	"/usr/man/man1.Z",
	"/usr/man/man1m.Z",
	"/usr/man/man2.Z",
	"/usr/man/man3.Z",
	"/usr/man/man4.Z",
	"/usr/man/man5.Z",
	"/usr/man/man6.Z",
	"/usr/man/man7.Z",
	"/usr/man/man8.Z",
	"/usr/man/man9.Z",
	"/usr/local/man/man1.Z",
	"/usr/local/man/man1m.Z",
	"/usr/local/man/man2.Z",
	"/usr/local/man/man3.Z",
	"/usr/local/man/man4.Z",
	"/usr/local/man/man5.Z",
	"/usr/local/man/man6.Z",
	"/usr/local/man/man7.Z",
	"/usr/local/man/man8.Z",
	"/usr/local/man/man9.Z",
	"/usr/contrib/man/man1.Z",
	"/usr/contrib/man/man1m.Z",
	"/usr/contrib/man/man2.Z",
	"/usr/contrib/man/man3.Z",
	"/usr/contrib/man/man4.Z",
	"/usr/contrib/man/man5.Z",
	"/usr/contrib/man/man6.Z",
	"/usr/contrib/man/man7.Z",
	"/usr/contrib/man/man8.Z",
	"/usr/contrib/man/man9.Z",
};
static char *suffix[] = {
	"1", "1m", "2", "3", "4", "5", "6", "7", "8", "9", "l", "n", "p", 0
};
static char *srcdirs[]  = {
	"/usr/src/games",
	"/usr/src/local",
	"/usr/src/head",
	"/usr/src/lib",
	"/usr/src/lib/libc/gen",
	"/usr/src/lib/libc/stdio",
	"/usr/src/lib/libc/sys",
	"/usr/src/lib/libPW",
	"/usr/src/lib/libc/net/inet",
	"/usr/src/lib/libc/net/misc",
	"/usr/src/lib/libc/net/common",
	"/usr/src/undoc",
	0
};

char	sflag = 1;
char	bflag = 1;
char	mflag = 1;
char	**Sflag;
int	Scnt;
char	**Bflag;
int	Bcnt;
char	**Mflag;
int	Mcnt;
char	uflag;
/*
 * whereis name
 * look for source, documentation and binaries
 */
main(argc, argv)
	int argc;
	char *argv[];
{

#if defined NLS || defined NLS16	/* initialize to the right language */
	if (!setlocale(LC_ALL,"")) {
		fputs(_errlocale("whereis"),stderr);
		nlmsg_fd = (nl_catd)-1;
	}
	else
		nlmsg_fd = catopen("man");
#endif NLS || NLS16

	argc--, argv++;
	if (argc == 0) {
usage:
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "whereis [ -sbmu ] [ -SBM dir ... -f ] name...\n")));
		exit(1);
	}
	do
		if (argv[0][0] == '-') {
			register char *cp = argv[0] + 1;
			while (*cp) switch (*cp++) {

			case 'f':
				break;

			case 'S':
				getlist(&argc, &argv, &Sflag, &Scnt);
				break;

			case 'B':
				getlist(&argc, &argv, &Bflag, &Bcnt);
				break;

			case 'M':
				getlist(&argc, &argv, &Mflag, &Mcnt);
				break;

			case 's':
				zerof();
				sflag++;
				continue;

			case 'u':
				uflag++;
				continue;

			case 'b':
				zerof();
				bflag++;
				continue;

			case 'm':
				zerof();
				mflag++;
				continue;

			default:
				goto usage;
			}
			argv++;
		} else
			lookup(*argv++);
	while (--argc > 0);
	exit (0);
}

getlist(argcp, argvp, flagp, cntp)
	char ***argvp;
	int *argcp;
	char ***flagp;
	int *cntp;
{

	(*argvp)++;
	*flagp = *argvp;
	*cntp = 0;
	for ((*argcp)--; *argcp > 0 && (*argvp)[0][0] != '-'; (*argcp)--)
		(*cntp)++, (*argvp)++;
	(*argcp)++;
	(*argvp)--;
}


zerof()
{

	if (sflag && bflag && mflag)
		sflag = bflag = mflag = 0;
}
int	count;
int	print;


lookup(cp)
	register char *cp;
{
	register char *dp;

#ifdef NLS16
	register char *ep = NULL;

	for (dp = cp; *dp; ADVANCE(dp))
		if (*dp == '.')
			ep = dp;
	if (ep)
		*ep = 0;

	for (dp = cp; *dp; ADVANCE(dp))
		if (*dp == '/')
			cp = dp + 1;
#else
	for (dp = cp; *dp; dp++)
		continue;
	for (; dp > cp; dp--) {
		if (*dp == '.') {
			*dp = 0;
			break;
		}
	}
	for (dp = cp; *dp; dp++)
		if (*dp == '/')
			cp = dp + 1;
#endif
	if (uflag) {
		print = 0;
		count = 0;
	} else
		print = 1;
again:
	if (print)
		printf("%s:", cp);
	if (sflag) {
		looksrc(cp);
		if (uflag && print == 0 && count != 1) {
			print = 1;
			goto again;
		}
	}
	count = 0;
	if (bflag) {
		lookbin(cp);
		if (uflag && print == 0 && count != 1) {
			print = 1;
			goto again;
		}
	}
	count = 0;
	if (mflag) {
		lookman(cp);
		if (uflag && print == 0 && count != 1) {
			print = 1;
			goto again;
		}
	}
	if (print)
		printf("\n");
}

looksrc(cp)
	char *cp;
{
	if (Sflag == 0) {
		find(srcdirs, cp);
	} else
		findv(Sflag, Scnt, cp);
}

lookbin(cp)
	char *cp;
{
	if (Bflag == 0)
		find(bindirs, cp);
	else
		findv(Bflag, Bcnt, cp);
}

lookman(cp)
	char *cp;
{
	if (Mflag == 0) {
		findx(mandirs, cp);
		find(zmandirs, cp);
	} else
		findv(Mflag, Mcnt, cp);
}

findv(dirv, dirc, cp)
	char **dirv;
	int dirc;
	char *cp;
{

	while (dirc > 0)
		findin(*dirv++, cp), dirc--;
}

find(dirs, cp)
	char **dirs;
	char *cp;
{

	while (*dirs)
		findin(*dirs++, cp);
}

findx(dirs, cp)
	char **dirs;
	char *cp;
{
	char	**p;
	char	buf[40];

#ifndef NONLS
	char	*lang;

	if ((lang = getenv("LANG")) != 0) {
		p = suffix;
		while (*p) {
			sprintf(buf, NLSMANDIR, lang);
			strcat(buf, *p++);
			findin(buf, cp);
		}
	}
#endif
	while (*dirs) {
		p = suffix;
		while (*p) {
			strcpy(buf, *dirs);
			strcat(buf, *p++);
			findin(buf, cp);
		}
		*dirs++;
	}
}

findin(dir, cp)
	char *dir, *cp;
{
	register DIR *d;
	struct direct *direct;

	d = opendir(dir);
	if (d == NULL)
		return;
	while ((direct = readdir(d)) != (struct direct *)NULL) {
		if (direct->d_ino == 0)
			continue;
		if (itsit(cp, direct->d_name)) {
			count++;
			if (print)
				printf(" %s/%s", dir, direct->d_name);
		}
	}
	closedir(d);
}

itsit(cp, dp)
	register char *cp, *dp;
{
	register int i = MAXNAMLEN;

	if (dp[0] == 's' && dp[1] == '.' && itsit(cp, dp+2))
		return (1);
	while (*cp && *dp && *cp == *dp)
		cp++, dp++, i--;
	if (*cp == 0 && *dp == 0)
		return (1);
	while (isdigit(*dp))
		dp++;
	if (*cp == 0 && *dp++ == '.') {
		--i;
		while (i > 0 && *dp)
			if (--i, *dp++ == '.')
				return (*dp++ == 'C' && *dp++ == 0);
		return (1);
	}
	return (0);
}
