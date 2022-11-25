/* @(#) $Revision: 66.1 $ */    
static char *HPUX_ID = "@(#) $Revision: 66.1 $";
#include <stdio.h>
#include "s500defs.h"
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#ifdef	DEBUG
int debug = 0;
#endif	DEBUG

char *cmd, *pname, *dname(), *strrchr();
extern int errno, errinfo;

struct stat s1, s2;

int cpflag = 0, mvflag = 0, lnflag = 0, maketarget;

main(argc, argv)
register int argc;
register char *argv[];
{
	register int i, r;

#ifdef	DEBUG
	setbuf(stdout, (char *) NULL);
#endif	DEBUG

	pname = argv[0];
	if (cmd = strrchr(pname, '/'))
		cmd++;
	else cmd = pname;
	if (strlen(cmd) > 2)
		cmd += strlen(cmd) - 2;
	if (strcmp(cmd, "cp") == 0)
		cpflag++;
	else if (strcmp(cmd, "mv") == 0)
		mvflag++;
	else if (strcmp(cmd, "ln") == 0)
		lnflag++;
	else {
		error("%s: not a valid link to this program", pname);
		exit(1);
	}

#ifdef	DEBUG
	if (strcmp(argv[1], "-x") == 0) {
		debug++;
		argc--;
		argv++;
	}
#endif	DEBUG

	if (argc < 3)
		usage();
	if (argc == 3 && strcmp(argv[1], "-") == 0
	    && strcmp(argv[2], "-") == 0) {
		error("use \"cat\" to copy stdin to stdout");
		exit(3);
	}
	if (argc > 3) {
		for (i = 1; i < (argc - 2); i++)
			if (strcmp(argv[i], "-") == 0) {
				error("'-' makes no sense here.");
				usage();
			}
		if (strcmp(argv[argc-1], "-") != 0) {
			if (sdfstat(argv[argc-1], &s2) < 0) {
				error("%s: %s not found", pname, argv[argc-1]);
				exit(1);
			}
			if ((s2.st_mode & S_IFMT) != S_IFDIR)
				usage();
		}
	}
	for (r = 0, i = 1; i < (argc - 1); i++)
		r += process(argv[i], argv[argc - 1]);
	exit(r ? 2 : 0);
}

process(source, target)
register char *source, *target;
{
	register int infd, outfd;
	char buf[128];

	if (!sdfpath(source) && !sdfpath(target)) {
		error("%s and %s are both HP-UX files; use \"%s\".", source,
		  target, cmd);
		return(1);
	}
	if (strcmp(source, "-") != 0 && sdfstat(source, &s1) < 0) {
		error("%s: %s not found", pname, source);
		return(1);
	}
	if ((s1.st_mode & S_IFMT) == S_IFDIR) {
		error("%s: source (%s) cannot be a directory",
		    pname, source);
		return(1);
	}
	s2.st_mode = S_IFREG;
	if (sdfstat(target, &s2) >= 0)
		if ((s2.st_mode & S_IFMT) == S_IFDIR) {
			sprintf(buf, "%s/%s", target, dname(source));
			target = buf;
			if (sdfstat(target, &s2) >= 0) {
				if (s1.st_dev == s2.st_dev
				  && s1.st_ino == s2.st_ino) {
					error("%s: %s and %s are identical",
					  pname, source, target);
					return(1);
				}
				if (!cpflag)
					if (sdfunlink(target) < 0) {
						error("%s: cannot unlink %s",
						  pname, target);
						return(1);
					}
			}
			else
				maketarget = 1;
		}
	if (lnflag)
		return(ln(source, target));
	else if (mvflag)
		return(mv(source, target));
	else
		return(cp(source, target));
}

mv(source, target)
register char *source, *target;
{
	register int val;

	if (sdflink(source, target))
		if (errno == EXDEV || errno == ENODEV)
			cp(source, target);
		else
			return(-1);
	if (sdfunlink(source))
		return(1);
	return(0);
}

ln(source, target)
register char *source, *target;
{
	if (sdflink(source, target) < 0) {
		switch (errno) {
			case EXDEV:
				error("%s: cannot link across file systems",
				  pname);
				break;
			default:
				error("%s: no permission for %s", pname,
				  target);
				break;
		}
		return(1);
	}
	return(0);
}

cp(source, target)
register char *source, *target;
{
	register int bread, infd, outfd;
	char buf[1024];

/*
 * Check to see if standard input is the input to the copy function... if it
 *     is, it's already open.  Otherwise, see if the pathname is a valid SDF
 *     pathname; if so, call the special open for SDF files; otherwise, call
 *     the normal open system call...
 */

	if (strcmp(source, "-") == 0)
		infd = 0;
	else if (sdfpath(source))
		infd = sdfopen(source, 0);
	else 
		infd = open(source, O_RDONLY);
	if (infd < 0) {
		fprintf(stderr, "%s: can't open %s\n", pname, source);
		return(1);
	}

/*
 * Check to see if standard output is the output to the copy function... if it
 *     is, it's already open.  Otherwise, see if the pathname is a valid SDF
 *     pathname; if so, call the special open for SDF files; otherwise, see if
 *     the file exists.  If not, call creat() to create it, else call the normal
 *     open system call...
 */

	if (strcmp(target, "-") == 0)
		outfd = 1;
	else if (sdfpath(target))
		outfd = sdfopen(target, 1);
	else if (access(target, 0) < 0)
		outfd = creat(target, 0666);
	else
		outfd = open(target, O_WRONLY);
	if (outfd < 0) {
		if (infd != 0)
			sdfclose(infd);
		fprintf(stderr, "%s: can't open %s\n", pname, target);
		return(1);
	}
	while ((bread = sdfread(infd, buf, 1024)) > 0) {
		if (sdfwrite(outfd, buf, bread) != bread) {
			error("%s: bad copy to %s", pname, target);
			error("%s: unlinking %s", pname, target);
			sdffstat(outfd, &s2);
			if (infd != 0)
				sdfclose(infd);
			if (outfd != 1)
				sdfclose(outfd);
			if ((s2.st_mode & S_IFMT) == S_IFREG) {
				if (sdfpath(target))
					sdfunlink(target);
				else
					unlink(target);
			}
			return(1);
		}
	}
	if (infd > 0) {
		if (maketarget)
			sdffstat(infd, &s1);
		sdfclose(infd);
	}
	if (outfd > 0 && outfd != 1)
		sdfclose(outfd);
	if (maketarget)
		sdfchmod(target, s1.st_mode & 07777);
	return(0);
}

char *
dname(name)
register char *name;
{
	char *strchr();
	register char *cp;

	cp = strchr(name, ':');
	if (cp != NULL)
		name = cp;
	cp = name;
	while (*cp)
		if (*cp++ == '/' && *cp != NULL)
			name = cp;
	return(name);
}

usage()
{
	char prefix[256];

	strcpy(prefix, pname);
	prefix[strlen(prefix) - 2] = NULL;
	error("Usage: %s{mv|cp|ln} {f1|-} f2", prefix);
	error("       %s{mv|cp|ln} f1 ... fn {dir|-}", prefix);
	if (mvflag)
		error("       %smv dir1 dir2", prefix);
	exit(2);
}
