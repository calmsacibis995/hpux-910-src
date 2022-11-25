/* @(#) $Revision: 66.1 $ */    
static char *HPUX_ID = "@(#) $Revision: 66.1 $";
#include "s500defs.h"
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>

#define	TRUE	1
#define	FALSE	0

char *pname, *ctime(), *strchr(), *malloc();

int listall = FALSE;
int listinum = FALSE;
int listname = FALSE;
int dironly = FALSE;
int flagtype = FALSE;
int longlist = FALSE;
int passwdf = TRUE;

#ifdef	DEBUG
int debug = FALSE;
#endif	/* DEBUG */

/*
 * The following two macros are defined even though they may not be
 * supported in the current release.  This is  for backwards
 * compatability.
 */
#ifndef	S_IFSRM
#define	S_IFSRM	0150000 /* SRM special */
#endif

#ifndef S_IFNWK
#define S_IFNWK 0110000 /* RFA network special */
#endif

main(argc, argv)
register int argc;
register char **argv;
{
	register char *cp;
	register int uid;

	pname = *argv++;
	if (pname[strlen(pname) - 1] == 'l')
		longlist = TRUE;
	uid = getuid();
	if (uid == 0)
		listall = TRUE;
	while (*argv != NULL && **argv == '-')
		for (--argc, cp = *argv++ +1; *cp; cp++) {
			switch(*cp) {
				case 'a':
				case 'A':
					if (uid)
						listall = TRUE;
					else
						listall = FALSE;
					break;
				case 'd':
					dironly = TRUE;
					break;
				case 'F':
					flagtype = TRUE;
					break;
				case 'i':
					listinum = TRUE;
					break;
				case 'l':
					longlist = TRUE;
					break;
				case 'p':
					passwdf = FALSE;
					break;

#ifdef	DEBUG
				case 'x':
					debug = TRUE;
					break;
#endif	/* DEBUG */

				default:
					usage();
					break;
			}
		}
	if (*argv == (char *) NULL)
		usage();
	if (--argc > 1)
		listname = TRUE;
	for ( ; *argv != (char *) NULL; ++argv)
		enqueue(*argv);
	flushit();
	exit(0);
}

#define BUFSIZE 200
char *ptrbuf[BUFSIZE];
int ptrcount = 0;

enqueue(path)
register char *path;
{
	static int msg = 0;
	register char *p;
	void process();

	if (ptrcount >= BUFSIZE) {
		if (!msg) {
			fprintf(stderr, "%s: too many files to sort\n", pname);
			msg++;
		}
		flushit();
	}

	p = malloc((unsigned) (strlen(path) + 1));
	if (p == NULL) {
		fprintf("%s: can't allocate memory\n");
		process(path);
	}
	strcpy(p, path);
	ptrbuf[ptrcount++] = p;
}

flushit()
{
	char **p;
	int compare();
	void process();

	qsort((char *) ptrbuf, ptrcount, sizeof(ptrbuf[0]), compare);
	for (p = ptrbuf; ptrcount > 0; ptrcount--, p++) {
		process(*p);
		free(*p);
	}
}

usage()
{
	fprintf(stderr, "Usage: %s [-aAdFilp] device:file [...]\n", pname);
	exit(1);
}

void
process(path)
register char *path;
{
	struct stat stbuf, *stp = NULL;
	char pathname[201], *cp, *sdfname();
	register int fd;
	void listdir();
	
	if ((fd = sdfopen(path, 0)) < 0) {
		fprintf(stderr, "%s: %s not found\n", pname, path);
		return;
	}
	if (sdffstat(fd, &stbuf) < 0) {
		fprintf(stderr, "%s: can't stat %s\n", pname, path);
		return;
	}
	cp = strchr(path, ':');
	strncpy(pathname, path, ++cp - path);
	pathname[cp - path] = NULL;
	if (*cp == '/')
		strcat(pathname, cp);
	else {
		strcat(pathname, "/");
		if (*cp != NULL)
			strcat(pathname, cp);
	}
	if ((stbuf.st_mode & S_IFMT) != S_IFDIR || dironly) {
		sdfclose(fd);
		if (longlist || flagtype || listinum)
			stp = &stbuf;
		listfile(sdfname(pathname), stp);
		return;
	}
	if (listname)
		printf("%s:\n", pathname);
		/*printf("%s:\n", sdfname(pathname));*/
	listdir(fd, pathname, stbuf.st_size);
	sdfclose(fd);
	return;
}

compare(s1, s2)
register char **s1, **s2;
{
	return strcmp(*s1, *s2);
}

ascend(d1, d2)
register struct direct *d1, *d2;
{
	return(strncmp(d1->d_name, d2->d_name, sizeof(d1->d_name)));
}

void
listdir(fd, path, dirsize)
register int fd;
register char *path;
register int dirsize;
{
	struct stat stbuf, *stp;
	struct direct *dp, *dindex;
	char filename[17], pathname[256];
	long blocks = 0, files = 0;
	register int i;
	void sdfdir2str();

	if ((dp = (struct direct *) malloc((unsigned) dirsize)) == NULL)
		nomemory();
	if (sdfread(fd, (char *) dp, dirsize) != dirsize) {
		fprintf(stderr, "%s: can't read directory %s\n", pname, path);
		return;
	}
	files = dirsize / (sizeof(struct direct));
	qsort((char *) dp, files, sizeof(struct direct), ascend);
	for (i = 0, dindex = dp; i < files; i++, dindex++) {
		if (dindex->d_ino == 0)
			continue;
		if (dindex->d_name[0] == '.' && !listall)
			continue;
		sdfdir2str(dindex->d_name);
		strncpy(filename, dindex->d_name, sizeof(dindex->d_name));
		strcpy(pathname, path);
		strcat(pathname, "/");
		strcat(pathname, filename);
		if (longlist || flagtype || listinum) {
			if (sdfstat(pathname, &stbuf) < 0) {
				printf("can't stat %s\n", pathname);
				continue;
			}
			stp = &stbuf;
		}
		else
			stp = NULL;		/* NULL if not -l or -F */
		listfile(filename, stp);
	}
	free((char *) dp);
}

listfile(path, stp)
register char *path;
register struct stat *stp;	/* NULL if -l or -F not specified */
{
	struct passwd *usr, *getpwuid();
	struct group *grp, *getgrgid();
	register char *timestr;
	long timenow, timelimit;
	void listftype();

	if (listinum)
		printf("%5d ", stp->st_ino);
	if (longlist) {
		printmodes(stp->st_mode);
		printf(" %3d ", stp->st_nlink);

		if (passwdf && ((usr = getpwuid(stp->st_uid)) != NULL))
			printf("%-8.8s ", usr->pw_name);
		else
			printf("%-8d ", stp->st_uid);

		if (passwdf && ((grp = getgrgid(stp->st_gid)) != NULL))
			printf("%-9.9s", grp->gr_name);
		else
			printf("%-9d", stp->st_gid);

		if ((stp->st_mode & S_IFCHR) == S_IFCHR ||
		    (stp->st_mode & S_IFSRM) == S_IFSRM ||
		    (stp->st_mode & S_IFBLK) == S_IFBLK)
			printf(" %3d 0x%06.6x", major(stp->st_rdev),
			    minor(stp->st_rdev));
		else
			printf("%7d", stp->st_size);

		timenow = time((long *) 0);
		timelimit = timenow - 6L*30L*24L*60L*60L;	/* 6 mos ago */
		timestr = ctime(&stp->st_mtime);

		if (stp->st_mtime > timelimit)
			printf(" %.6s %.5s ", (timestr+4), (timestr+11));
		else
			printf(" %.6s  %.4s ", (timestr+4), (timestr+20));
	}
	printf("%s", path);
	if (flagtype)
		listftype(stp);
	puts("");
}

printmodes(mode)
register int mode;
{
	switch(mode & S_IFMT) {
		case S_IFIFO:
			putchar('p');
			break;
		case S_IFCHR:
			putchar('c');
			break;
		case S_IFDIR:
			putchar('d');
			break;
		case S_IFBLK:
			putchar('b');
			break;
		case S_IFNWK:
			putchar('n');
			break;
		case S_IFSRM:
			putchar('s');
			break;
		case S_IFREG:
			putchar('-');
			break;
		default:
			putchar('?');
			break;
	}
	putchar(mode & 0400 ? 'r' : '-');		/* owner read */
	putchar(mode & 0200 ? 'w' : '-');		/* owner write */
	switch(mode & 04100) {				/* owner exec */
		case 04100:		/* x && s */
			putchar('s');
			break;
		case 04000:		/* !x && s */
			putchar('S');
			break;
		case 00100:		/* x && !s */
			putchar('x');
			break;
		case 00000:		/* !x && !s */
			putchar('-');
			break;
	}
	putchar(mode & 0040 ? 'r' : '-');		/* group read*/
	putchar(mode & 0020 ? 'w' : '-');		/* group write */
	switch(mode & 02010) {				/* group exec */
		case 02010:		/* x && s */
			putchar('s');
			break;
		case 02000:		/* !x && s */
			putchar('S');
			break;
		case 00010:		/* x && !s */
			putchar('x');
			break;
		case 00000:		/* !x && !s */
			putchar('-');
			break;
	}
	putchar(mode & 0004 ? 'r' : '-');		/* world read */
	putchar(mode & 0002 ? 'w' : '-');		/* world write */
	switch(mode & 01001) {				/* world exec */
		case 01001:		/* x && t */
			putchar('t');
			break;
		case 01000:		/* !x && t */
			putchar('T');
			break;
		case 00001:		/* x && !t */
			putchar('x');
			break;
		case 00000:		/* !x && !t */
			putchar('-');
			break;
	}
}

void
listftype(stp)
register struct stat *stp;
{
	register int type, executable;

	type = stp->st_mode & S_IFMT;
	executable = stp->st_mode & 0111;
	if (type == S_IFDIR || type == S_IFNWK || type == S_IFSRM)
		putchar('/');
	else if (executable)
		putchar('*');
	else
		putchar(' ');
	return;
}
