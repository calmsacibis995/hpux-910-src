static char *HPUX_ID = "@(#) $Revision: 70.4 $";
/*
 *	rm [-f|-i] [-Rr] file ...
 *      rmdir [-f|-i] [-ps] directory ...
 */

#include	<sys/param.h>		/* for MAXPATHLEN */
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<ndir.h>
#include	<sys/errno.h>
#include	<unistd.h>

#ifdef NLS
#    define NL_SETN 1		/* set number */
#    include	<msgbuf.h>
#    include	<locale.h>
#    include	<nl_ctype.h>
#    include	<langinfo.h>
     unsigned char yesstr[3], nostr[3];
     int  yesint, noint;	/* first character of NLS yes/no */
#else
#   define nl_msg(i, s) (s)
#   define nl_strrchr   strrchr
    char yesstr[] = "y";
    char nostr[]  = "n";
#endif /* NLS */
#if defined(SecureWare) && defined(B1)
#include <sys/security.h>
#endif

#define TRUE   1
#define FALSE  0

#ifdef ACLS
#    define OPACL '+'  /* optional ACL entries:  print "+" after mode */
#    define NOACL ' '  /* NO optional entries:   print " " after mode */
#endif

#ifdef SYMLINKS
#if defined(SecureWare) && defined(B1)
#   define STAT  do_lstat
#else
#   define STAT  lstat
#endif
#else
#   define STAT  stat
#endif

extern int errno;

void rm();
void too_long();
char *nl_strrchr();
extern int rmdir();

int rmdir_flag;         /* rmdir instead of rm */
int fflg;    		/* -f */
int iflg;    		/* -i */
int rflg;    		/* -r (rm only) */
int pflg;    		/* -p (rmdir only) */
#ifdef VERBOSE_RMDIR_P
int sflg;    		/* -s (rmdir only) */
#endif /* VERBOSE_RMDIR_P */
int errflg;  		/* an error occured */
int nfds;    		/* number of open file descriptors */
char path[MAXPATHLEN];  /* Path to current thing */
struct stat st;         /* stat() values for current thing */
int maxfds;

/*
 * This assumes that ISATTY is always called with the same value
 * for 'fd' (which is true for our purposes).
 */
int ttyflag = -1;
#define ISATTY(fd) (ttyflag == -1 ? (ttyflag = isatty(fd)) : ttyflag)

main(argc, argv)
int argc;
char *argv[];
{
    extern int optind;
    int c;
#ifdef SecureWare && defined (B1)
    int mflg=0;	/* Is the -M option set ? */

    if (ISB1)
	rm_init(argc, argv);
#endif

    {
	char *sb = nl_strrchr(argv[0], '/');

	if (sb)
	    sb++;
	else
	    sb = argv[0];
	rmdir_flag = (strncmp(sb, "rmdir", 5) == 0);
    }
#ifdef GT_64_FDS
    maxfds = sysconf(_SC_OPEN_MAX)-1;
#else
    maxfds = _NFILE - 1;
#endif

#ifdef NLS || NLS16		/* initialize to the current locale */
    if (!setlocale(LC_ALL, ""))
    {
	fputs(_errlocale(rmdir_flag ? "rmdir" : "rm"), stderr);
	putenv("LANG=");
    }
    nl_catopen("rm");

    strncpy(yesstr, nl_langinfo(YESSTR), 2);
    strncpy(nostr, nl_langinfo(NOSTR), 2);
    if (FIRSTof2(yesstr[0]))	/* two-byte yes char */
	yesstr[2] = 0;
    else			/* one-byte yes char */
	yesstr[1] = 0;
    if (FIRSTof2(nostr[0]))	/* two-byte no char */
	nostr[2] = 0;
    else			/* one-byte no char */
	nostr[1] = 0;

    yesint = CHARAT(yesstr);
    noint = CHARAT(nostr);
    if (!yesint || !nostr)
    {
	yesint = 'y';
	noint = 'n';
    }
#endif /* NLS || NLS16 */

#ifdef VERBOSE_RMDIR_P
    fflg = iflg = rflg = pflg = sflg = errflg = FALSE;
#else
    fflg = iflg = rflg = pflg = errflg = FALSE;
#endif /* VERBOSE_RMDIR_P */

#ifdef VERBOSE_RMDIR_P
#ifdef SecureWare && defined (B1)
    while ((c = getopt(argc, argv, rmdir_flag? "fips" : "MRfir")) != EOF)
#else
    while ((c = getopt(argc, argv, rmdir_flag? "fips" : "Rfir")) != EOF)
#endif
#else
#ifdef SecureWare && defined (B1)
    while ((c = getopt(argc, argv, rmdir_flag? "fip" : "MRfir")) != EOF)
#else
    while ((c = getopt(argc, argv, rmdir_flag? "fip" : "Rfir")) != EOF)
#endif
#endif /* VERBOSE_RMDIR_P */
	switch (c)
	{
	case 'f':
	    fflg = TRUE;
	    break;
	case 'i':
	    iflg = TRUE;
	    break;
	case 'r':
	case 'R':
	    rflg = TRUE;
	    break;
	case 'p':
	    pflg = TRUE;
	    break;
#ifdef VERBOSE_RMDIR_P
	case 's':
	    sflg = TRUE;
	    break;
#endif /* VERBOSE_RMDIR_P */
#ifdef SecureWare && defined (B1)
	case 'M':
		if (ISB1)
			mflg++;
		else	
	    		errflg = TRUE;
		break;
#endif /* SecureWare && B1 */
	case '?':
	    errflg = TRUE;
	    break;
	}
#ifdef SecureWare && defined (B1)
	/* Raise mld if (in kernel and -M option) or (in base set) */
	if (ISB1)
		rm_raise_mld (mflg);
#endif /* SecureWare && B1 */

    if (fflg)
    {
	/*
	 * disable -i if they also said -f
	 */
        if (iflg)
	    iflg = FALSE;

#ifdef VERBOSE_RMDIR_P
	/*
	 * Enable -s if both -p and -f were used.
	 */
	if (pflg)
	    sflg = TRUE;
#endif /* VERBOSE_RMDIR_P */
    }
    
    argc -= optind;
    argv = &argv[optind];

    if (errflg || (argc < 1 && !fflg))
    {
	if (rmdir_flag)
#ifdef VERBOSE_RMDIR_P
	    fputs(nl_msg(17, "Usage: rmdir [-fips] dirname ...\n"),
		stderr);
#else
	    fputs(nl_msg(17, "Usage: rmdir [-fip] dirname ...\n"),
		stderr);
#endif /* VERBOSE_RMDIR_P */
	else
#if defined(SecureWare) && defined(B1)
	    if (ISB1)
	       fputs(nl_msg(1, "Usage: rm [-MRfir] file ...\n"), stderr);
	    else
	       fputs(nl_msg(1, "Usage: rm [-Rfir] file ...\n"), stderr);
#else
	    fputs(nl_msg(1, "Usage: rm [-Rfir] file ...\n"), stderr);
#endif
	exit(2);
    }

    while (argc-- > 0)
    {
	int len;
	char *sb;

	/*
	 * Check for "." or "..", can't remove these things.
	 */
	if (strcmp(*argv, "..") == 0 ||
	    ((sb = nl_strrchr(*argv, '/')) &&
	     strcmp(sb + 1, "..") == 0))
	{
	    fprintf(stderr, nl_msg(2, "%s: cannot remove ..\n"),
		rmdir_flag ? "rmdir" : "rm");
	    errflg = TRUE;
	    goto Continue;
	}

	if (sb && strcmp(sb + 1, ".") == 0)
	{
	    fprintf(stderr, nl_msg(2, "%s: cannot remove .\n"),
		rmdir_flag ? "rmdir" : "rm");
	    errflg = TRUE;
	    goto Continue;
	}

	/*
	 * If this command is "rmdir", just call rmDir on the given
	 * path.
	 */
	if (rmdir_flag)
	{
	    /*
	     * If we might need to print the whole path name of
	     * what we are removing, save it in path (since rmDir
	     * modifies its argument as it goes).
	     */
#ifdef VERBOSE_RMDIR_P
	    if (pflg && !sflg && !fflg)
		strcpy(path, *argv);
#else
	    if (pflg && !fflg)
		strcpy(path, *argv);
#endif /* VERBOSE_RMDIR_P */

	    if (rmDir(*argv, TRUE) == -1)
		errflg = TRUE;
	    goto Continue;
	}

	/*
	 * This command is "rm", do the (possibly recursive) remove.
	 */
	if ((len = strlen(*argv)) >= MAXPATHLEN)
	{
	    too_long(*argv, NULL);
	    goto Continue;
	}

	strcpy(path, *argv);
	if (STAT(path, &st) == -1)
	{
	    if (errno != ENOENT)
	    {
		fputs(nl_msg(21, "rm: cannot stat "), stderr);
		perror(path);
		errflg = TRUE;
	    }
	    else if (fflg == 0)
	    {
		fprintf(stderr, nl_msg(3, "rm: %s non-existent\n"),
		    path);
		errflg = TRUE;
	    }
	}
	else
	{
#ifdef GT_64_FDS
	    nfds = getnumfds(); /* files open, (stdin,stdout,stderr) */
#else
	    nfds = 3; /* stdin, stdout, stderr */
#endif
	    rm(&path[len]);
	}
    Continue:
	argv++;
    }

    exit(errflg ? 2 : 0);
}

/*
 * rm() -- remove the file whose path name is in the global buffer
 *         "path".  endp points to the NULL character at the end
 *         of the buffer and is used to append extra stuff to the
 *         end of path when/if we call ourselves.  The global
 *         stat struct, "st" is already filled in.
 */
void
rm(endp)
char *endp;
{
    register DIR *dirp;
    register struct direct *dp;

    if ((st.st_mode & S_IFMT) == S_IFDIR)
    {
	int pathlen;

	if (!rflg)
	{
	    fprintf(stderr, nl_msg(6, "rm: %s directory\n"), path);
	    errflg = TRUE;
	    return;
	}

	pathlen = (endp - path) + 1;
	if (pathlen >= MAXPATHLEN)
	{
	    too_long(path, NULL);
	    return;
	}

	if (iflg) 
	{
		if ( !dotname(path)) {
	    		fprintf(stderr, nl_msg(4, "directory %s: ? "), path);
	    		fprintf(stderr, "(%s/%s) ", yesstr, nostr);
	    		if (!yes())
				return;
		}
	}
	else
	if (!fflg)  
	{
		if (access(path, 02) == -1 && ISATTY(fileno(stdin)))
		{
#ifdef ACLS
			fprintf(stderr, nl_msg(7, "%s: %o%c mode ? "), path,
		    		st.st_mode & 0777, st.st_acl ? OPACL : NOACL);
#else				/* no ACLS */
			fprintf(stderr, nl_msg(7, "%s: %o mode ? "),
		    		path, st.st_mode & 0777);
#endif /* ACLS */
	    		fprintf(stderr, "(%s/%s) ", yesstr, nostr);
	    		if (!yes())
				return;
		}
	}
	if ((dirp = opendir(path)) == NULL)
	{
	    fputs(nl_msg(5, "rm: cannot read "), stderr);
	    perror(path);
	    errflg = TRUE;
	    return;
	}

	nfds++;
	*endp++ = '/';
	*endp = '\0';

	while ((dp = readdir(dirp)) != NULL)
	{
	    int rc;
	    int namlen = dp->d_namlen;

	    if (dp->d_ino == 0 || dotname(dp->d_name))
		continue;

	    if (pathlen + namlen >= MAXPATHLEN)
	    {
		too_long(path, dp->d_name);
		return;
	    }

	    strcpy(endp, dp->d_name);
	    rc = STAT(path, &st);

#if defined(DUX) || defined(DISKLESS)
	    /*
	     * If the stat failed or we got an inode number that
	     * we didn't expect, we add a '+' and stat again.
	     * If that stat fails or we don't get a CDF, we remove
	     * the '+' and stat the file one more time.
	     */
	    if (rc == -1 || st.st_ino != dp->d_ino)
	    {
		if (pathlen + namlen >= (MAXPATHLEN - 1))
		{
		    too_long(path, dp->d_name);
		    return;
		}

		endp[namlen++] = '+';
		endp[namlen] = '\0';
		if ((rc = STAT(path, &st)) == -1 ||
			(st.st_mode & (S_IFMT | S_CDF)) != (S_IFDIR | S_CDF))
		{
		    endp[--namlen] = '\0';
		    rc = STAT(path, &st);
		}
	    }
#endif /* DUX || DISKLESS */

	    if (rc == -1)
	    {
		if (errno != ENOENT)
		{
		    fputs(nl_msg(21, "rm: cannot stat "), stderr);
		    perror(path);
		    errflg = TRUE;
		}
		else if (!fflg)
		{
		    fprintf(stderr, nl_msg(3, "rm: %s non-existent\n"),
			path);
		    errflg = TRUE;
		}
	    }
	    else
	    {
		/*
		 * Do a recursive call to remove a file or directory.
		 * If the thing we want to remove is a directory, we
		 * first see if we have run out of file descriptors.
		 * If we have, we save our place in the current
		 * directory and close it.  When we get back from the
		 * recursie call, we reopen the current dir (if we
		 * closed it before)
		 */
		long here = -1;

		if (nfds >= maxfds && (st.st_mode & S_IFMT) == S_IFDIR)
		{
		    if ((here = telldir(dirp)) != -1)
			closedir(dirp);
		}

		rm(&endp[namlen], st);

		if (here != -1)
		{
		    *endp = '\0';
		    if ((dirp = opendir(path)) == NULL)
		    {
			fputs(nl_msg(5, "rm : cannot read "), stderr);
			perror(path);
			exit(2);
		    }
		    seekdir(dirp, here);
		}
	    }
	}

	closedir(dirp);
	nfds--;
	*--endp = '\0';
	if (rmDir(path, TRUE) == -1)
	    errflg = TRUE;
	return;
    }

    if (iflg)
    {
	fprintf(stderr, "%s: ? (%s/%s) ", path, yesstr, nostr);
	if (!yes())
	    return;
    }
    else if (!fflg)
    {
	if (
#ifdef SYMLINKS
		((st.st_mode & S_IFMT) != S_IFLNK) &&
#endif
		access(path, 02) == -1 && ISATTY(fileno(stdin)))
	{
#ifdef ACLS
	    fprintf(stderr, nl_msg(7, "%s: %o%c mode ? "), path,
		    st.st_mode & 0777, st.st_acl ? OPACL : NOACL);
#else				/* no ACLS */
	    fprintf(stderr, nl_msg(7, "%s: %o mode ? "),
		    path, st.st_mode & 0777);
#endif /* ACLS */
	    fprintf(stderr, "(%s/%s) ", yesstr, nostr);
	    if (!yes())
		return;
	}
    }

    if (unlink(path) == -1)
    {
	fprintf(stderr, nl_msg(8, "rm: %s not removed.  "), path);
	perror("");
	errflg = TRUE;
    }
}

int
dotname(s)
char *s;
{
    return (s[0] & 0177) == '.' &&
	    (s[1] == '\0' || ((s[1] & 0177) == '.' && s[2] == '\0'));
}

/*
 * isempty() -- Return TRUE if the specified directory is empty.
 *              Returns FALSE if path is not empty or is not a
 *              directory (cannot be opendir-ed).
 */
int
isempty(path)
char *path;
{
    register DIR *dirp;
    register struct direct *dp;

    if ((dirp = opendir(path)) == NULL)
	return FALSE;

    while ((dp = readdir(dirp)) != NULL)
    {
	if (dp->d_ino == 0 || dotname(dp->d_name))
	    continue;
	closedir(dirp);
	return FALSE;
    }
    closedir(dirp);
    return TRUE;
}

void
my_perror(errval)
int errval;
{
    extern int errno;

    switch (errval)
    {
    case EBUSY:
	fputs(nl_msg(13, "Cannot remove mountable directory\n"),
	    stderr);
	break;
    case EEXIST:
	fputs(nl_msg(14, "Directory not empty\n"), stderr);
	break;
    case EINVAL:
	fputs(nl_msg(15, "Cannot remove current directory\n"), stderr);
	break;
    case ENAMETOOLONG:
	fputs(nl_msg(16, "Directory name too long\n"), stderr);
	break;
    default:
	{
	    int save_errno = errno;

	    errno = errval;
	    perror("");
	    errno = save_errno;
	}
    }
}

/*
 * rmDir() --
 *   Delete a directory.  If the global pflg is TRUE, then the parent
 *   of the directory is also removed (if it is empty).
 */
int
rmDir(f, top_level)
char *f;
int top_level;
{
    if (dotname(f))
	return 0;

    if (iflg)
    {
	fprintf(stderr, "%s: ? (%s/%s) ", f, yesstr, nostr);
	if (!yes())
	    return pflg ? -2 : 0;
    }

    if (rmdir(f) == -1)
	if (!top_level || (fflg && errno == ENOENT))
	    return -1;
	else
	{
	    if (rmdir_flag)
		fprintf(stderr, nl_msg(18, "rmdir: %s: "), f);
	    else
		fprintf(stderr,
		    nl_msg(12, "rm: directory %s not removed.  "), f);

	    my_perror(errno);
	    return -1;
	}

    /*
     * If we are doing "rmdir -p path", strip the last component from
     * the path and then try to remove it if it is empty.
     */
    if (pflg)
    {
	char *sb = nl_strrchr(f, '/');
	int rval;

	if (sb == NULL)
	    return 0;
	else
	{
	    if (sb == f) /* don't rmDir '/' */
		return 0;
	    *sb = '\0';
	}

	/*
	 * Only remove the directory if it isn't empty.  We do an
	 * explicit check so that we don't prompt about removing
	 * a non-empty directory (-i option) and so that we can
	 * correctly report other errors about removing the directory.
	 *
	 * We can avoid the explicit check if the "-f" option has
	 * been specified as long as there was no -i specified too.
	 */
	if ((fflg && !iflg) || isempty(f))
	    rval = rmDir(f, FALSE);
	else
	{
	    errno = EEXIST;
	    rval = -1;
	}

	/*
	 * If the rmdir failed, we print a message that the rm failed
	 * (unless we've already printed it, which is marked by
	 * setting path[0] to '\0')
	 *
	 * The -f (fflg) only suppresses ENOENT errors.
	 */
	if ((rval == -1 || rval == -2) &&
	    !(fflg && errno == ENOENT) && path[0] != '\0')
	{
	    if (rval == -1)
	    {
		fprintf(stderr,
		    nl_msg(19, "rmdir: %s: %s not removed; "), path, f);

		my_perror(errno);
	    }
#ifdef VERBOSE_RMDIR_P
	    else if (!sflg)
		fprintf(stderr,
		    nl_msg(22, "rmdir: %s: %s not removed\n"), path, f);
#endif /* VERBOSE_RMDIR_P */

	    path[0] = '\0'; /* mark that we have printed the error */
	}

	if (sb != f)
	    *sb = '/';  /* restore '/' character */

#ifdef VERBOSE_RMDIR_P
	if (rval == 0 && top_level && !sflg && !fflg && path[0] != '\0')
	    fprintf(stderr,
		nl_msg(20, "rmdir: %s: Whole path removed.\n"), f);
#endif /* VERBOSE_RMDIR_P */

	return rval;
    }
    else
	return 0;
}

yes()
{
    int i, b;

#if defined(NLS) || defined(NLS16)
    if (FIRSTof2(i = getchar()))
	if (SECof2(b = getchar()))
	    i = (i << 8) | b;
    for (b = i; b != '\n' && b > 0;)
	b = getchar();
    return i == yesint;
#else
    i = b = getchar();
    while (b != '\n' && b > 0)
	b = getchar();
    return i == 'y';
#endif /* NLS || NLS16 */
}

void
too_long(s1, s2)
char *s1;
char *s2;
{
    if (s2)
	fprintf(stderr, nl_msg(11, "rm: %s/%s: Name too long\n"),
	    s1, s2);
    else
	fprintf(stderr, nl_msg(10, "rm: %s: Name too long\n"), s1);
    errflg = TRUE;
}

#ifdef NLS16
#define CNULL (char *)0

/*
 * nl_strrchr() --
 *   NLS-ed version of strrchr
 */
char *
nl_strrchr(sp, c)
register unsigned char *sp;
register unsigned int c;
{
    register unsigned char *r;

    if (sp == '\0')
	return (char *)NULL;

    r = (unsigned char *)NULL;
    do
    {
	if (CHARAT(sp) == c)
	    r = sp;
    } while (CHARADV(sp));
    return (char *)r;
}
#endif /* NLS16 */
