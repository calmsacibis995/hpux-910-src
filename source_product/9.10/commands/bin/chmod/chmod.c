static char *HPUX_ID = "@(#) $Revision: 72.1 $";
/*
 * chmod [-R] [ugoa]+-=[rwxXstHugo][,...] file ...
 * chmod [-A] [-R] [ugoa]+-=[rwxXstHugo][,...] file ... (if ACLS defined)
 *  change mode of files
 *
 * OBSOLETE FORM:
 * chmod [-R] absolute-mode file ...
 * chmod [-A] [-R] absolute-mode file ... (if ACLS defined)
 *
 * AW:
 * 2 Jan 1992:  Modified for POSIX.2 draft 11.2 conformance
 * Changed parse_mode()
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include "walkfs.h"
int	walkfs_fn();			/* walkfs function */

#if defined(NLS) || defined(NLS16)
#include <locale.h>
#include <setlocale.h>
#endif

#ifdef ACLS
#   include <sys/acl.h>
#   ifdef NFS
#	include <sys/mount.h> /* for MOUNT_NFS */
#   endif
#endif /* ACLS */

#ifndef NLS		/* NLS must be defined */
#   define catgets(i,sn,mn,s) (s)
#   define open_catalog()
#else /* NLS */
#   define NL_SETN 1	/* set number */
    nl_catd nlmsg_fd;
#endif /* NLS */

#ifdef ACLS
#define USER	    05700	/* user's bits */
#define GROUP	    02070	/* group's bits */
#define OTHER	    00007	/* other's bits */
#define BASE	    00777	/* all possible for setacl */
#define USERSHIFT	6	/* number to shift for user bits */
#define GROUPSHIFT	3	/* number to shift for group bits */
#endif /* ACLS */

#define ALLBITS	07777		/* all bits */

extern int optind, opterr;
extern char *optarg;

int	status = 0;			/* status of subroutine calls */
int	Rflag = 0;			/* recursively descend directories */
int	Aflag = 0;
char	abuf[100];			/* argument buffer */
int	errno_save = 0;			/* for preserving errno value */
#if defined(DUX) || defined(DISKLESS)
char	*mstr = "ugoa=+-rwxsXHt,01234567";	/* all valid options */
#else
char	*mstr = "ugoa=+-rwxsXt,01234567";	/* all valid options */
#endif

/* NOTE: The message catalog numbers for each variant of the usage message must
 * be unique -- even though only one will ever be used for a given version of
 * chmod.  The reason: "findmsg", which processes this C code in order to build
 * the message catalog, does not honor #ifdef directives.
 */
#if defined(DUX) || defined(DISKLESS)
#  ifdef ACLS
      static char usage[] =
      "Usage: chmod [-A] [-R] [ugoa]+-=[rwxXstHugo][,...] file ...\n";	/* catgets 101 */
#     define USAGE_NUM	101
#  else					/* no ACLS */
      static char usage[] =
      "Usage: chmod [-R] [ugoa]+-=[rwxXstHugo][,...] file ...\n";	/* catgets 102 */
#     define USAGE_NUM	102
#  endif /* ACLS */
#else /* no DUX || DISKLESS */
#  ifdef ACLS
      static char usage[] =
      "Usage: chmod [-A] [-R] [ugoa]+-=[rwxXstugo][,...] file ...\n";	/* catgets 103 */
#     define USAGE_NUM	103
#  else /* no ACLS */
      static char usage[] =
      "Usage: chmod [-R] [ugoa]+-=[rwxXstugo][,...] file ...\n";	/* catgets 104 */
#     define USAGE_NUM	104
#  endif /* ACLS */
#endif /* defined(DUX) || defined(DISKLESS) */

static char cant_change[] =
    "chmod: can't change ";					/* catgets 3 */

#if defined(ACLS) && (defined(RFA) || defined(NFS))
static char no_remote[] =
    "chmod: -A option not allowed on remote file: %s\n";	/* catgets 5 */
#endif /* ACLS && (RFA || NFS) */

#ifdef NLS
void
open_catalog()
{
    static int called = 0;

    if (!called)
    {
	if(!setlocale(LC_ALL, "")) {
		fputs(_errlocale("chmod"), stderr);
		putenv("LANG=");
		nlmsg_fd = (nl_catd)(-1);
	} else
		nlmsg_fd = catopen("chmod", 0);
	called = 1;
    }
}
#endif /* NLS */

main(argc,argv)
int  argc;
char **argv;
{
register int i;
int j, c;
int fflag = 0;
char *pstr;
struct stat sbuf;
int cur_ind;

	if (argc < 3)
	{
		open_catalog();
		fputs(catgets(nlmsg_fd, NL_SETN, USAGE_NUM, usage), stderr);
		return 255;	/* exit with 255 for backward compatability */
	}

	cur_ind = 1;		/* initial optind value */
	opterr = 0;
	/* get options -A or -R from command line.
	 */
	while ((c = getopt(argc, argv, "AR")) != -1) {
		switch (c) {
			case 'A':
				Aflag = 1;
				break;
			case 'R':
				Rflag = 1;
				break;
			case '?':
				/* Obsolescent forms are to be allowed.
				 * illegal multi-char options like -z87
				 * will come here without optind incremented.
				 */
				if(optind != cur_ind) optind--;
				break;
			default:
				userr(usage, USAGE_NUM);
		} /* switch (c) */
		if (c == '?') break;
		cur_ind = optind;
	}

#ifndef ACLS
	if(Aflag) userr(usage, USAGE_NUM);
#endif /* ACLS */

	if(optind >= argc) userr(usage, USAGE_NUM);

	/* collect all arguments into buffer */
	abuf[0] = '\0';			/* Initialize buffer to empty */
	pstr = abuf;			/* abuf can hold only 100 bytes */

	i = optind;  /* mode list here */
	for(j = 0; argv[i][j]; j++) {
		if(valid(argv[i][j], mstr)) continue;
		fflag = 1;
		break;
	}
	if(!fflag)
		for(j = 0; argv[i][j]; j++) *pstr++ = argv[i][j];
	i = optind+1;	/* next argument should be file */

	/* 
	 * Make sure that we have at least one valid mode before continuing,
	 * and determine if modes are valid before going through the arguments
	 */
	if ((strlen(abuf) == 0) || (parse_mode(abuf,0) < 0))
	{
		open_catalog();
		fputs(catgets(nlmsg_fd, NL_SETN, 4, "chmod: invalid mode\n"),
	    		stderr);
		return 255;
	}

    for (; i < argc; i++)
    {				/* i is set to first file */
	if (Rflag)
	{			/* recursively descend */
	    walkfs(argv[i], walkfs_fn, _NFILE - 3,
		   WALKFS_DOCDF);			/* removed WALKFS_LSTAT. This propagated the
							 * symlink's perms to the original file
							 * Defect No.:UCSqm00681  */
	}
	else
	{
	    struct walkfs_info single_info;
	    unsigned int flag = WALKFS_NONDIR;

	    single_info.shortpath = argv[i];
	    if (stat(argv[i], &(single_info.st)) == -1)
		flag = WALKFS_NOSTAT;
	    walkfs_fn(&single_info, flag);
	}
    }
    return status;
}

#ifdef ACLS
int
doacl(pst, p,nm)
struct stat *pst;
char *p;
int nm;
{
    struct acl_entry acl[NACLENTRIES];
    int nacl;
    int err;

    /*
     * get the original ACL.
     */
    if ((nacl = getacl(p, NACLENTRIES, acl)) < 0)
    {
	if (errno == EOPNOTSUPP)
	{
	    open_catalog();
	    fprintf(stderr,
		catgets(nlmsg_fd, NL_SETN, 5, no_remote), p);
	    return 1;
	}
	else
	{
	    errno_save = errno;		/* preserve for later perror() use */
	    open_catalog();
	    fputs(catgets(nlmsg_fd, NL_SETN, 3, cant_change), stderr);
	    errno = errno_save;		/* restore */
	    perror(p);
	    return 1;
	}
    }

    /*
     * change to the new modes
     */
    if (chmod(p, nm) < 0)
    {
	errno_save = errno;		/* preserve for later perror() use */
	open_catalog();
	fputs(catgets(nlmsg_fd, NL_SETN, 3, cant_change), stderr);
	errno = errno_save;		/* restore */
	perror(p);
	return 1;
    }

    /*
     * apply the new modes to the original ACL.
     */
    applyacl(pst, acl, nacl, nm);

    /*
     * set the ACL with the new modes.
     * If this fails, we may leave the modes in a strange state.
     */
    if (setacl(p, nacl, acl) < 0)
    {
	errno_save = errno;		/* preserve for later perror() use */
	open_catalog();
	fputs(catgets(nlmsg_fd, NL_SETN, 3, cant_change), stderr);
	errno = errno_save;		/* restore */
	perror(p);
	return 1;
    }
    return 0;
}

/*
 * applyacl-
 *	Apply the modes in nm to the base mode bits in the original acl
 */
applyacl(pst, acl,nacl,nm)
struct stat *pst;
struct acl_entry  *acl;
int  nacl;
unsigned int nm;
{
    int i;
    int user;
    int base;

    /*
     * Walk through each entry in the ACL
     */
    for (i = 0; i < nacl; i++)
    {
	if (acl[i].uid == pst->st_uid && acl[i].gid == ACL_NSGROUP)
	{
	    /*
	     * Found the base entry for user.
	     * Put new mode into acl after masking special bits
	     */
	    acl[i].mode = (nm & USER & BASE) >> USERSHIFT;
	}

	if (acl[i].uid == ACL_NSUSER)
	{
	    if (acl[i].gid == pst->st_gid)
	    {
		/*
		 * found the base entry for group
		 * Put new mode into acl after masking special bits
		 */
		acl[i].mode = (nm & GROUP & BASE) >> GROUPSHIFT;
	    }
	    else if (acl[i].gid == ACL_NSGROUP)
	    {
		/*
		 * found the base entry for other
		 * Put new mode into acl
		 */
		acl[i].mode = (nm & OTHER);
	    }
	}
    }
}
#endif /* ACLS */

walkfs_fn(info, flag)
	struct walkfs_info *info;
	unsigned int flag;
{
    register char *p = info->shortpath;
    struct stat *pst = &(info->st);
    unsigned int nm;

    if ((flag & WALKFS_NOSTAT) != 0)
    {
	open_catalog();
	fprintf(stderr,
	    catgets(nlmsg_fd, NL_SETN, 2, "chmod: can't access %s\n"), p);
	status++;
	return WALKFS_OK;
    }

    if ((flag & (WALKFS_NOREAD | WALKFS_NOSEARCH)) != 0)
    {
	open_catalog();
	fprintf(stderr,
	    catgets(nlmsg_fd, NL_SETN, 6, "chmod: can't traverse %s\n"), p);
	status++;
	return WALKFS_OK;
    }

#ifdef ACLS
    if (Aflag)			/* -A - apply mode to st_basemode */
	nm = parse_mode(abuf, (pst->st_mode & S_IFMT) | pst->st_basemode);
    else
#endif /* ACLS */
	nm = parse_mode(abuf, pst->st_mode);

#if defined(DUX) || defined(DISKLESS)
    /*
     * parse_mode() can only fail now if they did a +H
     * and 'p' is not a directory.  [we already checked
     * that the mode was otherwise valid.
     */
    if (nm == -1)
    {
	open_catalog();
	fprintf(stderr,
	    catgets(nlmsg_fd, NL_SETN, 7, "chmod: %s not a directory\n"), p);
	status++;
	return WALKFS_OK;
    }
#endif /* defined(DUX) || defined(DISKLESS) */
#ifdef ACLS
    /*
     * Try to retain optional entries
     * Flag error if file is remote.
     */
    if (Aflag)
    {
#if defined(RFA) || defined(NFS)
#if !defined(RFA) /* just NFS */
	if (pst->st_fstype == MOUNT_NFS)
#else
#if !defined(NFS) /* just RFA */
	if (pst->st_remote != 0)
#else /* both */
	if (pst->st_fstype == MOUNT_NFS || pst->st_remote != 0)
#endif
#endif
	{
	    open_catalog();
	    fprintf(stderr,
		catgets(nlmsg_fd, NL_SETN, 5, no_remote), p);
	    status++;
	    return WALKFS_OK;
	}
#endif /* RFA or NFS */
	if (pst->st_acl)
	{
	    if (doacl(pst, p, nm & ~S_IFMT) != 0)
	    {
		status++;
		return WALKFS_OK;
	    }
	    return WALKFS_OK;
	}
    }
#endif /* ACLS */

    if (chmod(p, nm & ~S_IFMT) < 0)
    {				/* call CHMOD */
	errno_save = errno;		/* preserve for later perror() use */
	open_catalog();
	fputs(catgets(nlmsg_fd, NL_SETN, 3, cant_change), stderr);
	errno = errno_save;		/* restore */
	perror(p);
	status++;
    }
    return WALKFS_OK;
}

userr(s, num)
char *s;
int num;
{
	open_catalog();
	fputs(catgets(nlmsg_fd, NL_SETN, num, s),stderr);
	exit(1);
}

valid(c, s)
char c;
char *s;
{
	/* check if it is a valid character belonging to "s" */
	while(*s) if(c == *s++) return(1);
	return(0);
}
