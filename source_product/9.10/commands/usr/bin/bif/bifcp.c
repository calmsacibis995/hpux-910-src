static char *HPUX_ID = "@(#) $Revision: 66.1 $";
/* @(#)$Revision: 66.1 $ */
/*
** Combined mv/cp/ln command:
**	mv file1 file2
**	mv dir1 dir2
**	mv file1 ... filen dir1
*/

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>

#define EQ(x,y)	!strcmp(x,y)
#define	DOT	"."
#define	DOTDOT	".."
#define	DELIM	'/'
#define	MAXN	128
#define MODEBITS 07777

char	*dname();
char	*strrchr();
extern	errno;

struct	stat s1, s2;

char	*cmd, *pname;
int	f_flag = 0;
int	debug = 1;

main(argc, argv)
register char *argv[];
{
	register i, r;

	pname = argv[0];
	if (cmd = strrchr(argv[0], '/'))
		++cmd;
	else
		cmd = argv[0];
	if (strlen(cmd)>2)
		cmd += strlen(cmd)-2;
	if (!EQ(cmd, "cp") && !EQ(cmd, "mv") && !EQ(cmd, "ln")) {
		fprintf(stderr, "%s: command must end in cp|mv|ln--defaults to `cp'\n", pname);
		cmd = "cp";
	}

	if (EQ(argv[1], "-f"))
		++f_flag, --argc, ++argv;

	if (argc < 3)
		usage();
	bfsstat(argv[1], &s1);
			if(EQ(cmd, "cp"))
	if ((s1.st_mode & S_IFMT) == S_IFDIR && !(EQ(cmd,"mv"))) {
		fprintf(stderr, "%s: %s may not be a directory\n",
			pname, argv[1]);
		exit(1);
	}
	setuid(getuid());
	if (argc > 3)
		if (bfsstat(argv[argc-1], &s2) < 0) {
			fprintf(stderr, "%s: %s not found\n", pname, argv[argc-1]);
			exit(2);
		} else if ((s2.st_mode & S_IFMT) != S_IFDIR)
			usage();
	r = 0;
	for (i=1; i<argc-1; i++)
		r += move(argv[i], argv[argc-1]);
	exit(r?2:0);
}

extern char * normalize(), *bfstail();

move(source, target)
char *source, *target;
{
	register c, i;
	char	buf[MAXN];
	register char *sp,*tp;
	char saves,savet;
	register int links;

	source = normalize(source);
	target = normalize(target);
	if (bfsstat(source, &s1) < 0) {
		fprintf(stderr, "%s: cannot access %s\n", pname, source);
		return(1);
	}
	sp = bfstail(source);
	tp = bfstail(target);
	saves = *sp;
	savet = *tp;
	*sp = '\0'; 	/* null terminate for comparison */
	*tp = '\0';
	if ((s1.st_mode & S_IFMT) == S_IFDIR )
	{
	    if(!(EQ(cmd,"mv"))) {
		*sp = saves;
		fprintf(stderr, "%s : <%s> directory\n", pname, source);
		return(1);
	    }
	    if(!(EQ(source,target))) {
		*tp = savet;
		fprintf(stderr, "%s : <%s> directory\n", pname, target);
		return(1);
	    }
	}
	*sp = saves;
	*tp = savet;

	s2.st_mode = S_IFREG;
	if (bfsstat(target, &s2) >= 0) {
		if ((s2.st_mode & S_IFMT) == S_IFDIR) {
			sprintf(buf, "%s/%s", target, dname(source));
			target = buf;
		}
		if (bfsstat(target, &s2) >= 0) {
			if (s1.st_dev==s2.st_dev && s1.st_ino==s2.st_ino) {
				fprintf(stderr,"%s: %s and %s are identical\n",
					pname, source, target);
				return(1);
			}
			if(EQ(cmd, "cp"))
				goto skip;
			if (bfsunlink(target) < 0) {
				fprintf(stderr, "%s: cannot unlink %s\n", pname, target);
				return(1);
			}
		}
	}
skip:
	if (EQ(cmd, "cp") || bfslink(source, target) < 0) {
	/***
	links = bfslink(source,target);
	if (EQ(cmd, "cp") || links < 0) { 
	***/
		int from, to, ct, oflg;
		char fbuf[512];

		if (EQ(cmd, "ln")) {
			if(errno == EXDEV)
				fprintf(stderr, "%s: different file system\n", pname);
			else
				fprintf(stderr, "%s: no permission for %s\n", pname, target);
			return(1);
		}
		if((from = bfsopen(source, 0)) < 0) {
			fprintf(stderr, "%s: cannot open %s\n", pname, source);
			return 1;
		}
		oflg = 0; /* access(target, 0) == -1; */
		if((to = bfsopen(target, 1)) < 0) {
			fprintf(stderr, "%s: cannot create %s\n", pname, target);
			return 1;
		}
		while((ct = bfsread(from, fbuf, 512)) > 0)
			if(ct < 0 || bfswrite(to, fbuf, ct) != ct) {
				fprintf(stderr, "%s: bad copy to %s\n", pname, target);
				bfsclose(from), bfsclose(to);
				if((s2.st_mode & S_IFMT) == S_IFREG)
					unlink(target);
				return 1;
			}
		bfsclose(from), bfsclose(to);
		if (oflg)
			bfschmod(target, s1.st_mode);
	}
	if (!EQ(cmd, "mv"))
		return 0;
	if (bfsunlink(source) < 0) {
s_unlink:
		fprintf(stderr, "%s: cannot unlink %s\n", pname, source);
		return(1);
	}
	return(0);
}


char *
dname(name)
register char *name;
{
	char *strchr();
	register char *p;

	/* Skip the first : */
	p = strchr(name, ':');
	if (p!=NULL)
		name = p;

	/* find the last / that's not at the end of the string */
	p = name;
	while (*p)
		if (*p++ == DELIM && *p)   /* pointers on both sides?? no no */
			name = p;
	return name;
}

usage()
{
	char prefix[20];
	strcpy(prefix, pname);
	prefix[strlen(prefix)-2] = '\0';	/* omit suffix */

	fprintf(stderr, "Usage: %s{mv|cp|ln} f1 f2\n", prefix);
	fprintf(stderr, "       %s{mv|cp|ln} f1 ... fn d1\n", prefix);
	fprintf(stderr, "       %smv d1 d2\n", prefix);
	exit(2);
}
