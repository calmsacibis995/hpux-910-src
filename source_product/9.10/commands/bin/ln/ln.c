/*
 * ln -- link files
 */
#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 72.1 $";
#endif

#ifndef NLS
#   define catgets(i, sn,mn,s) (s)
#else
#   define NL_SETN 1	/* set number */
#   include <nl_types.h>
#   include <nl_ctype.h>
#   include <langinfo.h>
#   include <locale.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>

#ifndef MAXPATHLEN
#   define MAXPATHLEN 1024
#endif

#define MODEBITS 07777
#define DELIM	'/'

#ifdef ACLS
#define OPACL 	'+'	/* optional ACL entries on file:  print "+" past mode */
#define NOACL 	' '	/* NO optional entries on file:  print " "	      */
#endif /* ACLS */

struct	stat s1, s2;

int	fflag = 0;		/* force         flag set? */
int	iflag = 0;		/* interactive   flag set? */
int	sflag = 0;              /* symbolic link flag set? */

extern	int errno;
extern	int optind, opterr;	/* for getopt */

#ifdef NLS
nl_catd	nlmsg_fd;
char	yesstr[3], nostr[3];
int	yesint, noint;		/* first character of NLS yes/no */
#endif

main(argc, argv)
int argc;
register char *argv[];
{
    int i, r, c;

#ifdef NLS || NLS16		/* initialize to the correct locale */
    if (!setlocale(LC_ALL, ""))
    {
	fputs(_errlocale("ln"), stderr);
	putenv("LANG=");
	nlmsg_fd = (nl_catd)-1;
    }
    else
	nlmsg_fd = catopen("ln", 0);

    sprintf(yesstr, "%.2s", nl_langinfo(YESSTR));
    sprintf(nostr, "%.2s", nl_langinfo(NOSTR));
    yesint = CHARAT(yesstr);
    noint = CHARAT(nostr);
    if (!yesint || !noint)
    {
	yesint = 'y';
	noint = 'n';
    }

    if (FIRSTof2(yesstr[0]))
	yesstr[2] = 0;
    else
	yesstr[1] = 0;
    if (FIRSTof2(nostr[0]))
	nostr[2] = 0;
    else
	nostr[1] = 0;
#endif	/* NLS || NLS16 */

#ifdef SYMLINKS
    while ((c = getopt(argc, argv, "fis")) != EOF)
#else
    while ((c = getopt(argc, argv, "fi")) != EOF)
#endif
	switch (c)
	{
	case 'f':
	    fflag++;
	    break;
	case 'i':
	    iflag++;
	    break;
#ifdef SYMLINKS
	case 's':
	    sflag++;
	    break;
#endif
	case '?':
	    goto usage;
	}

    if (argc - optind < 2)
	goto usage;

    if (argc - optind > 2)
    {
	if (stat(argv[argc - 1], &s1) < 0)
	    goto usage;
	if ((s1.st_mode & S_IFMT) != S_IFDIR)
	    goto usage;
    }
    r = 0;
    for (i = optind; i < argc - 1; i++)
	r |= linkit(argv[i], argv[argc - 1]);
    exit(r);

usage:
#ifdef SYMLINKS
    fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 1, "Usage: ln [-f] [-i] [-s] f1 f2\n       ln [-f] [-i] [-s] f1 ... fn d1\n")));
#else
    fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 2, "Usage: ln [-f] [-i] f1 f2\n       ln [-f] [-i] f1 ... fn d1\n")));
#endif
    exit(2);
}

int	link();
#ifdef SYMLINKS
int symlink();
#endif
char *dname();

linkit(source, target)
char *source;
char *target;
{
    char *last;
#ifdef SYMLINKS
    int (*linkf)() = sflag ? symlink : link;
#else
    int (*linkf)() = link;
#endif
    int c, i;
    char buf[MAXPATHLEN];

    if (!sflag && stat(source, &s1) < 0)
    {
	fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 3, "ln: cannot access %s\n")), source);
	return 1;
    }
    else
	if (sflag)
	    memset(&s1, 0, sizeof s1);

    if (!sflag && (s1.st_mode & S_IFMT) == S_IFDIR)
    {
	fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 4, "ln : <%s> directory\n")), source);
	return 1;
    }

    s2.st_mode = S_IFREG;
    if (lstat(target, &s2) >= 0)
    {
	if ((s2.st_mode & S_IFMT) == S_IFDIR)
	{
	    last = dname(source);
	    if (strlen(target) + strlen(last) >= MAXPATHLEN - 1)
	    {
		fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 5, "ln: %s/%s: Name too long\n")),
			target, last);
		return 1;
	    }
	    sprintf(buf, "%s/%s", target, last);
	    target = buf;
	}

	if (lstat(target, &s2) >= 0)
	{

	    if (s1.st_dev == s2.st_dev && s1.st_ino == s2.st_ino)
	    {
		fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 6, "ln: %s and %s are identical\n")),
			source, target);
		return 1;
	    }

            /*
             * If the target exists, the -f flag must have been given.  If
             * not, complain.  This change is conformant with Posix.2
             */

	    if (!fflag)
	    {
		fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 12, "ln: %s exists\n")),
			target);
		return 1;
	    }

	    /*
	     * Check to see if the files are on the same file system
	     * BEFORE we unlink the target.  Do this for hard links
	     * only.
	     * Normal and NFS file systems just check st_dev.
	     */
#ifdef RFA
	     /*
	      * RFA file systems also check st_remote and st_netdev.
	      */
	    if (!sflag && (s1.st_dev != s2.st_dev ||
			(!s1.st_remote && !s2.st_remote &&
			    s1.st_netdev != s2.st_netdev)))
#else
	    if (!sflag && s1.st_dev != s2.st_dev)
#endif /* RFA */
	    {
		fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 9, "ln: different file system\n")));
		exit(1);
	    }

	    /* if our second try at a target is a dir, exit */
	    if ((s2.st_mode & S_IFMT) == S_IFDIR)
	    {
		fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 4, "ln: <%s> directory\n")), target);
		return 1;
	    }
	    if (isatty(fileno(stdin))
	        && ((access(target, 2) < 0 && !fflag)
	            || iflag))
	    {
#ifdef NLS
#ifndef ACLS
		fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 7, "ln: %s: %o mode (%s/%s) ")),
			target, s2.st_mode & MODEBITS,
			yesstr, nostr);
#else				/* ACLS */
		fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 7, "ln: %s: %o%c mode (%s/%s) ")),
			target, s2.st_mode & MODEBITS,
			s2.st_acl ? OPACL : NOACL, yesstr, nostr);
#endif	/* ACLS */
		if (FIRSTof2(i = getchar()))
		    if (SECof2(c = getchar()))
			i = (i << 8) | c; /* combine into a 16 bit character */
		for (c = i; c != '\n' && c != EOF;)
		    c = getchar();
		if (i != yesint)
		    return 1;
#else
#ifndef ACLS
		fprintf(stderr, "ln: %s: %o mode (%s/%s) ", target,
			s2.st_mode & MODEBITS, "y", "n");
#else				/* ACLS */
		fprintf(stderr, "ln: %s: %o%c mode (%s/%s) ", target,
			s2.st_mode & MODEBITS,
			s2.st_acl ? OPACL : NOACL, "y", "n");
#endif				/* ACLS */
		i = c = getchar();
		while (c != '\n' && c != EOF)
		    c = getchar();
		if (i != 'y')
		    return 1;
#endif
	    }
	    if (unlink(target) < 0)
	    {
		fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 8, "ln: cannot unlink %s\n")), target);
		return 1;
	    }
	}
    }

    if ((*linkf)(source, target) < 0)
    {
	switch (errno)
	{
	case EXDEV:
	    fputs(catgets(nlmsg_fd, NL_SETN, 9, "ln: different file system\n"), stderr);
	    break;
	case EMLINK:
	    fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 11, "ln: too many links to %s\n")), source);
	    break;
	case EACCES:
	    fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 10, "ln: no permission for %s\n")), target);
	    break;
	default:
	    fputs("ln: ", stderr);
	    perror(target);
	    break;
	}
	return 1;
    }
    return 0;
}

/*
 *  We don't want to return a trailing DELIM
 *  so Strrchr returns the righmost DELIM that is
 *  not the last character in the string.
 */
#ifndef NLS16

char *
Strrchr(sp, c)
register char *sp;
register unsigned    c;
{
    register char *r;

    if (sp == NULL)
	return (NULL);

    r = NULL;
    do
    {
	if (*sp == c && *(sp + 1) != NULL)
	    r = sp;
    } while (*sp++);
    return (r);
}
#else /* NLS16 */
#define CNULL (char *)0

char *
Strrchr(sp, c)
register char *sp;
register unsigned    c;
{
    register char *r;

    if (sp == CNULL)
	return (CNULL);

    r = CNULL;
    do
    {
	if (CHARAT(sp) == c)
	    r = sp;
    } while (CHARADV(sp));
    return (r);
}
#endif /* NLS16 */

char *
dname(name)
char *name;
{
    char *p;

    p = Strrchr(name, DELIM);
    if (p)
	name = p;
    return name;
}
