static char *HPUX_ID = "@(#) $Revision: 38.1 $";
#include "s500defs.h"
#include <sys/stat.h>
#include "sdf.h"
#include <stdio.h>

char *pname;
int iflag = 0, fflag = 0, rflag = 0;
int errcode;

extern struct sdfinfo sdfinfo[];
int rmdirflag = 0;

#ifdef	DEBUG
int debug = 0;
#endif	DEBUG

main(argc, argv)
register int argc;
char **argv;
{
	register char *arg;
	void rmdir(), rm();

	pname = *argv++;
	if (strcmp(&pname[strlen(pname) - 3], "dir") == 0)
		rmdirflag++;
	if (argc < 2)
		usage();
	if (**argv == '-') {
		if (rmdirflag)
			usage();
		while (*++*argv != NULL)
			switch (**argv) {
				case 'f':
					fflag++;
					break;
				case 'i':
					iflag++;
					break;
				case 'r':
					rflag++;
					break;
#ifdef	DEBUG
				case 'x':
					debug++;
					break;
#endif	DEBUG
				default:
					usage();
			}
		argv++;
		argc--;
	}
	if (argc == 0)
		usage();
	while (--argc > 0)
		if (rmdirflag)
			rmdir(*argv++);
		else
			rm(*argv++);
	exit(errcode ? 2 : 0);	
}

usage()
{
	if (rmdirflag)
		fprintf(stderr, "Usage: %s device:name [...]\n",pname);
	else
		fprintf(stderr, "Usage: %s [-fir] device:name [...]\n", pname);
	exit(1);
}

void
rm(path, cflag)
register char *path;
register int cflag;
{
	struct direct dbuf;
	register int fd, parenti;
	register struct sdfinfo *sp;
	register char *cp;
	char name[100], *sdftail();
	void sdfdir2str(), rmdir();

	fd = sdfopen(path, 0);
	if (fd == -1) {
		if (!fflag) {
			fprintf(stderr, "%s: can't access %s\n", pname, path);
			++errcode;
		}
		return;
	}
	if (!sdffile(fd)) {
		fprintf(stderr, "use 'rm' to remove %s\n", path);
		++errcode;
		return;
	}
	sp = &sdfinfo[fd - SDF_OFFSET];
	parenti = sp->pinumber;
	if ((sp->inode.di_mode & S_IFMT) == S_IFDIR) {
		if (rflag) {
			if (iflag) {
				printf("directory %s: ", path);
				if (!yes())
					return;
			}
			while (sdfread(fd, (char *) &dbuf, D_SIZ) == D_SIZ) {
				if (dbuf.d_ino == 0)
					continue;
				sdfdir2str(dbuf.d_name);
				sprintf(name, "%s/%.16s", path, dbuf.d_name);
				rm(name);
			}
			sdfclose(fd);
			rmdir(path);
		}
		else {
			fprintf(stderr, "%s: %s directory\n", pname, path);
			++errcode;
		}
		return;
	}

	sdfclose(fd);
	if (iflag) {
		printf("%s: ", path);
		if (!yes())
			return;
	}
	if (sdfunlink(path) && (!fflag || iflag)) {
		perror(pname);
		fprintf(stderr, "%s: %s not removed\n", pname, path);
		++errcode;
		return;
	}
	return;
}

void
rmdir(path, cflag)
register char *path;
register int cflag;
{
	register int	fd;
	register char	*np;
	struct	stat	st;
	struct	direct	dir;
	register struct sdfinfo *sp;

	fd = sdfopen(path, 0);
	if (fd == -1) {
		fprintf(stderr, "%s: can't access %s\n", pname, path);
		++errcode;
		return;
	}
	if (!sdffile(fd)) {
		fprintf(stderr, "%s: use rmdir to process %s\n",
			pname, path);
		sdfclose(fd);
		++errcode;
		return;
	}
	sp = &sdfinfo[fd - SDF_OFFSET];
	if((sp->inode.di_mode & S_IFMT) != S_IFDIR) {
		fprintf(stderr, "%s: %s not a directory\n", pname, path);
		++errcode;
		return;
	}
	while(sdfread(fd, (char *)&dir, sizeof dir) == sizeof dir) {
		if(dir.d_ino == 0) continue;
		fprintf(stderr, "%s: %s not empty\n", pname, path);
		++errcode;
		sdfclose(fd);
		return;
	}
	sdfclose(fd);
	if (sdfunlink(path) < 0) {
		fprintf(stderr, "%s: %s not removed\n", pname, path);
		++errcode;
	}
	return;
}

yes()
{
	int i, b;

	i = b = getchar();
	while (b != '\n' && b > 0)
		b = getchar();
	return(i == 'y');

}
