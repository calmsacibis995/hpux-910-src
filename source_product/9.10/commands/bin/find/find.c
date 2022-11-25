/* @(#) $Revision: 70.2 $ */

/* AW: 14 Feb 1992
 * POSIX.2 Compliance. Specifically removing pathname limits
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>
#include <fcntl.h>

#ifdef RT
#   include <rt/types.h>
#   include <rt/dir.h>
#   include <rt/stat.h>
#else
#   include <sys/types.h>
#   include <ndir.h>
#   include <sys/stat.h>
#   include <ctype.h>
#endif /* RT */

#include <sys/mount.h>
#include <pwd.h>
#include <grp.h>

#ifdef ACLS
#   include <sys/acl.h>
#   include <acllib.h>
#endif /* ACLS */

#if defined(DUX) || defined(CNODE_DEV)
#   include <ctype.h>
#   include <cluster.h>
#endif /* DUX/CNODE_DEV */

#ifndef NLS
#   define catgets(cd,sn,mn,ds) (ds)
#else
#   define NL_SETN 1		/* set number */
#   include <msgbuf.h>
#   include <locale.h>
#   include <nl_ctype.h>
#   include <langinfo.h>
#endif /* NLS */

#ifdef NLS16
#   include <nl_ctype.h>
#   include <locale.h>
#endif /* NLS16 */

#define	UID	1
#define	GID	2

/*
 * Define MOUNT_CDFS if it isn't already defined
 */
#ifndef MOUNT_CDFS
#   define MOUNT_CDFS 2
#endif

int stat();
#ifdef SYMLINKS
int lstat();
#endif /* SYMLINKS */
char *strrchr();

#include "walkfs.h"
#ifdef RCSTOYS
#   include "rcstoys.h"
#endif

#define A_DAY	86400L	/* a day full of seconds */
#define EQ(x, y)  (strcmp(x, y)==0)
#define BUFSIZE	512	/* In u370 I can't use BUFSIZ nor BSIZE */

/*
 * Stuff for -cpio and -ncpio
 */
#include "libcpio.h"
extern CPIO *cpio_open();
extern void cpio_close();

#if defined(DUX) || defined(DISKLESS)
/*
 * Buffer for cpio path name, only necessary when we are finding
 * CDFs.
 */
char cpio_path[MAXPATHLEN + (MAXPATHLEN / 2)];
static char *cpio_end;
#endif /* DUX || DISKLESS */

#ifdef ACLS
#define APSIZE          sizeof(struct acl_entry_patt)
#define ATSIZE          sizeof(struct acl_entry)
#endif /* ACLS */

#if defined(NLS) || defined(NLS16)
    nl_catd	catd;
    extern nl_catd catopen();
    int yeschar;		/* first character of NLS "yes" */
#else
#   define yeschar  ('y')	/* first character of "yes" */
#endif /* NLS */


static int Randlast;
static char Pathbuf[MAXPATHLEN];

/*
 * Nodes are allocated out of a buffer that initially points
 * to a fixed buffer.  If we run out of nodes, a new buffer is
 * allocated using malloc().
 */
#define MAXNODES 85      /* Max that will fit in 1020 bytes */
static struct anode
{
    int (*F)();
    struct anode *L, *R;
} node_buff[MAXNODES];
typedef struct anode ANODE;
static ANODE *Node = node_buff;
static int Nn;			/* index of next available node */

static char *Fname;
static time_t Now;
static int Argc;
static int Ai;
static int Pi;
static char **Argv;

static int cpiof = 0;		/* True if -cpio or -ncpio */
static int depthf = 0;		/* True if -depth, -cpio or -ncpio */

#if defined(DUX) || defined(DISKLESS)
static int hflag = 0;		/* True if -hidden or -type H */
#endif /* DUX || DISKLESS */

#ifdef SYMLINKS
static int followflag = 0;	/* True if -follow */
#endif /* SYMLINKS */

static int mountstopflag = 0;	/* True if -xdev */

/*
 * fsonlyval -- a bit mask that specifies which filesystem types to
 *              visit.  Special case of 0 indicates to visit all.
 */
#define ONLY_HFS   (1 << MOUNT_UFS)
#ifdef HP_NFS
#define ONLY_NFS   (1 << MOUNT_NFS)
#endif /* HP_NFS */
#define ONLY_CDFS  (1 << MOUNT_CDFS)
static unsigned int fsonlyval = 0;

#define DOFS(st) (fsonlyval == 0 || \
		  ((1 << (st).st_fstype) & fsonlyval) != 0)

#define     NNEW    63  /* Fits in 252 bytes */
static int Nnewer;
static time_t newer_buff[NNEW];
static time_t *Newer = newer_buff;

static ANODE *exp(), *e1(), *e2(), *e3(), *mk();
static char *nxtarg();
static int home_fd = 0;  	/* descriptor for path to home */
static int usefulAction = 0;  /* we assume -print if they didn't have a
			         -print, -exec, -ok, -cpio or -ncpio */

static ANODE *exlist;
static int Return;
static int Returndefault = WALKFS_OK;

char *Pathname;
struct stat Statb;
struct walkfs_info *Info;

#ifdef RCSTOYS
int Rcs_Newfile;
#endif

static int
dofile(info, flag)
struct walkfs_info *info;
int flag;
{
    int forceSkip = 0;
    int visit = 1;

    Info = info;
    Pathname = info->relpath;
    Fname = info->basep;
    if (!(*Fname))
	Fname =  "/";
    Statb = info->st;
#ifdef RCSTOYS
    Rcs_Newfile = 1;
#endif

    if (flag == WALKFS_NOCHDIR)
    {
	/*
	 * walkfs couldn't get back to where it wanted to be.  Fix
	 * up fullpath name so it looks good and print an error
	 * message.
	 */
	char *endp = info->fullpath + strlen(info->fullpath) - 1;

	/*
	 * fullpath might end in extra "/./" but we don't want to print
	 * this bogus stuff.
	 */
	if (*endp == '/' && endp != info->fullpath)
	{
	    *endp-- = '\0';
	    if (*endp == '.' && endp != info->fullpath)
	    {
		*endp-- = '\0';
		if (*endp == '/' && endp != info->fullpath)
		    *endp = '\0';
	    }
	}

	fprintf(stderr,
	   catgets(catd,NL_SETN,60, "find: can't chdir back to %s\n"),
	   info->fullpath);
	exit(1);
    }

    if ((flag & WALKFS_NOSTAT) != 0)
    {
	fprintf(stderr,
		catgets(catd,NL_SETN,14, "find: cannot stat %s\n"), info->relpath);
	return WALKFS_OK;
    }

    /*
     * If we got a WALKFS_POPDIR we must be doing -depth, so call the
     * user function and return WALKFS_OK.
     */
    if (flag == WALKFS_POPDIR)
    {
#if defined(DUX) || defined(DISKLESS)
	/*
	 * Terminate the cpio path at the correct spot for the
	 * current path.
	 */
	if (cpiof)
	{
	    /*
	     * Delete the '/' that is at the end of the current
	     * cpio_path.
	     */
	    *--cpio_end = '\0';

	    /*
	     * Since "/" has a basename of "", we must treat it
	     * as a special case.  If cpio_path is now the null
	     * string, we must be trying to do "/", so we fix up
	     * cpio_path so that it is "/".
	     */
	    if (cpio_end == cpio_path)
	    {
		cpio_path[0] = '/';
		cpio_path[1] = '\0';
		cpio_end = &cpio_path[1];
	    }
	}
#endif /* DUX || DISKLESS */

	(*exlist->F)(exlist);

#if defined(DUX) || defined(DISKLESS)
	/*
	 * Now back up cpio_end one level in the hierarchy
	 */
	if (cpiof)
	{
	    --cpio_end;
	    /*
	     * Delete the extra '/' after a CDF, if appropriate
	     */
	    if (cpio_end > cpio_path && *cpio_end == '/')
		--cpio_end;

	    /*
	     * Now back up to just after the previous '/'
	     */
	    while (cpio_end > cpio_path && *cpio_end != '/')
		--cpio_end;
	    cpio_end++;
	}
#endif /* DUX || DISKLESS */
	return WALKFS_OK;
    }

    /*
     * If this is a retry (of a directory) we see if we can process
     * this directory.  If we can, just return WALKFS_OK.  If we
     * can't, we print an error message.
     *
     * We also enter this code if we are doing a -depth and the
     * first attempt at the directory failed (and print an error
     * message).  In this case we print the error message then
     * set forceSkip to TRUE and call the user function on it.
     */
    if (flag & WALKFS_RETRY ||
	(depthf && (flag & (WALKFS_NOREAD|WALKFS_NOSEARCH)) != 0))
    {
	if ((flag & ~WALKFS_RETRY) == WALKFS_DIR)
	    return WALKFS_OK;
	fprintf(stderr,
		catgets(catd,NL_SETN,15, "find: cannot %s %s\n"),
			(flag & WALKFS_NOREAD) ?
			    /* catgets 57 */ "open" :
			    /* catgets 58 */ "search",
			    info->relpath);
	if (flag & WALKFS_RETRY)
	    return WALKFS_SKIP;
	forceSkip = 1;
    }

    if ((flag & ~WALKFS_RETRY) == WALKFS_CYCLE)
    {
	fprintf(stderr,
		catgets(catd,NL_SETN,63, "find: %s creates a cycle\n"),
		info->relpath);
	Return = WALKFS_OK;
    }
    else
	if (Returndefault == WALKFS_SKIP &&
	    (Statb.st_mode & S_IFMT) == S_IFDIR)
	    Return = WALKFS_SKIP;
	else
	    Return = WALKFS_OK;

    /*
     * force a skip of this entry if we have said -mountstop and
     * this is a mountpoint and this is not a starting point.
     * (this is the starting point if info->parent is NULL)
     */
    if (mountstopflag && info->ismountpoint && info->parent != NULL)
	 forceSkip = 1;

    /*
     * Also force a skip if the type of filesystem that this file
     * is is not one specified with -fsonly.
     * We visit mount points unless the parent fstype should be
     * skipped too (except that we always visit a start point)
     */
    if (!DOFS(info->st))
    {
	 forceSkip = 1;
	 visit = info->ismountpoint &&
		     (info->parent == NULL || DOFS(info->parent->st));
    }

#if defined(DUX) || defined(DISKLESS)
    if (cpiof)
    {
	strcpy(cpio_end, info->basep);

	/*
	 * cpio uses an extra "/" following a cdf+ path to
	 * indicate that it is a CDF.
	 */
	if (S_ISCDF(Statb.st_mode))
	    strcat(cpio_end, "/");

	/*
	 * If this is a directory, tack on a '/' and advance cpio_end
	 * to the end of the string.
	 */
	if (S_ISDIR(Statb.st_mode))
	{
	    cpio_end += strlen(cpio_end);
	    *cpio_end++ = '/';
	}
    }
#endif /* DUX || DISKLESS */

    if (visit)
    {
	/*
	 * In depth first mode we don't call the user function on a
	 * directory until we get a WALKFS_POPDIR or if we are
	 * going to force a skip.
	 */
	if (depthf &&
	    (forceSkip || (flag & ~WALKFS_RETRY) == WALKFS_DIR))
	    return forceSkip ? WALKFS_SKIP : WALKFS_OK;
	else
	    (*exlist->F)(exlist);
    }

    if (forceSkip)
	return ((flag & ~WALKFS_RETRY) == WALKFS_DIR) ? WALKFS_SKIP :
							WALKFS_OK;

    /*
     * Return WALKFS_OK unless we had an error on a directory in which
     * case we want to do a retry.
     */
    if (Return == WALKFS_OK)
	return (flag & (WALKFS_NOREAD|WALKFS_NOSEARCH)) ? WALKFS_RETRY :
							  WALKFS_OK;
    else
	return Return;
}

main(argc, argv)
int argc;
char *argv[];
{
    int paths;
    register char *ep, *cp, *sp = 0;
    FILE *pwd, *popen();
    int walkfs_flags = WALKFS_LEAVEDOT;
    int walkfs_file_limit;

#ifdef NLS
    if (!setlocale(LC_ALL, ""))
    {
        fputs(_errlocale("find"), stderr);
	fputc('\n', stderr);
        putenv("LANG=");
        catd = (nl_catd)-1;
    }
    else
        catd = catopen("find", 0);
#endif /* NLS */

    time(&Now);

    Argc = argc;
    Argv = argv;
    if (argc < 2)
    {
    usage:
	fputs(catgets(catd,NL_SETN,1, "Usage: find path-list [predicate-list]\n"),
	      stderr);
	exit(1);
    }
    for (Ai = paths = 1; Ai < argc; ++Ai, ++paths)
	if (*Argv[Ai] == '-' || EQ(Argv[Ai], "(") || EQ(Argv[Ai], "!"))
	    break;

    if (paths == 1)		/* no path-list */
	goto usage;

    if (Ai == argc)      /* no predicates */
	exlist = (ANODE *)NULL;
    else
	if (!(exlist = exp()))	/* parse and compile the arguments */
	{
	    fputs(catgets(catd,NL_SETN,3, "find: parsing error\n"), stderr);
	    exit(1);
	}

    if (Ai < argc)
    {
	fputs(catgets(catd,NL_SETN,4, "find: missing conjunction\n"), stderr);
	exit(1);
    }

    /*
     * If they didn't specify a useful action, assume they want -print.
     * If there is an expression, "and" it with "-print".  If there is
     * no expression, make it a simple "-print".
     *
     * This is required for POSIX 1003.2 compliance.
     */
    if (!usefulAction)
    {
	int and(), print();
	ANODE *p;

	p = mk(print, (ANODE *)0, (ANODE *)0);
	if (exlist)
	    exlist = mk(and, exlist, p);
	else
	    exlist = p;
    }

    if ((home_fd = open(".", O_RDONLY)) < 0)
    {
	fputs(catgets(catd,NL_SETN,2, "find: cannot get 'pwd'\n"), stderr);
	exit(2);
    }

#ifdef SYMLINKS
    if (!followflag)
	walkfs_flags |= WALKFS_LSTAT;
#endif /* SYMLINKS */
#if defined(DUX) || defined(DISKLESS)
    if (hflag)
	walkfs_flags |= WALKFS_DOCDF;
#endif /* defined(DUX) || defined(DISKLESS) */
    if (depthf)
	walkfs_flags |= WALKFS_TELLPOPDIR;

#ifdef GT_64_FDS
    walkfs_file_limit = (sysconf(_SC_OPEN_MAX) - getnumfds()) - 7;
#else
    walkfs_file_limit = (sysconf(_SC_OPEN_MAX) - 3) - 7;
#endif

    for (Pi = 1; Pi < paths; ++Pi)
    {
	sp = "\0";
	strcpy(Pathname = Pathbuf, Argv[Pi]);

	/*
	 * Remove all redundant trailing '/'s
	 */
	ep = &Pathname[strlen(Pathname)];
	while (--ep > Pathname && *ep == '/')
	    *ep = '\0';

#if defined(DUX) || defined(DISKLESS)
	/*
	 * If we are creating a cpio archive, we must add an extra
	 * '/' character to the cpio_path name after each CDF.  We
	 * must also remove all redundant '/' characters from the
	 * cpio_path.
	 */
	if (cpiof)
	{
	    char *s = Pathname;

	    cpio_end = cpio_path;
	    while (*s)
	    {
		while (*s && *s != '/')
		    *cpio_end++ = *s++;

		/*
		 * Add an extra '/' after any CDFs
		 */
		if (s > Pathname && *(s-1) == '+')
		{
		    *cpio_end = '\0';
		    if ((followflag ?
			 *stat:*lstat)(cpio_path, &Statb) != -1 &&
			S_ISCDF(Statb.st_mode))
			*cpio_end++ = '/';
		}

		/*
		 * Copy the '/' or '\0'.
		 */
		*cpio_end++ = *s++;

		/*
		 * Skip any redundant '/' characters
		 */
		while (*s && *s == '/')
		    s++;
	    }
	    --cpio_end;

	    /*
	     * For now, we want cpio_end to point to the last valid
	     * character in cpio_path[].
	     */
	    if (*cpio_end == '\0')
		cpio_end--;

	    /*
	     * Now we must back up one, since dofile() will add
	     * basename(Pathname) on its first invocation.
	     *
	     * First -- skip all trailing '/' characters.
	     */
	    while (cpio_end > cpio_path && *cpio_end == '/')
		--cpio_end;

	    /*
	     * If there is a basename part (i.e. the path isn't
	     * simply "/"), we back up to the byte just after the
	     * '/'.
	     */
	    if (cpio_end > cpio_path)
	    {
		while (cpio_end > cpio_path && *cpio_end != '/')
		    --cpio_end;
		if (*cpio_end == '/')
		    cpio_end++;
	    }

	    *cpio_end = '\0';
	}
#endif /* DUX || DISKLESS */
	walkfs(Pathname, dofile, walkfs_file_limit, walkfs_flags);
    }

    if (cpiof)
	cpio_close();

    exit(0);
}

/* compile time functions:  priority is  exp()<e1()<e2()<e3()  */

static ANODE *
exp()
{				/* parse ALTERNATION (-o)  */
    int or();
    register ANODE *p1;

    p1 = e1() /* get left operand */ ;
    if (EQ(nxtarg(), "-o"))
    {
	Randlast--;
	return mk(or, p1, exp());
    }
    else if (Ai <= Argc)
	--Ai;
    return p1;
}

static ANODE *
e1()
{				/* parse CONCATENATION (formerly -a) */
    int and();
    register ANODE *p1;
    register char *a;

    p1 = e2();
    a = nxtarg();
    if (EQ(a, "-a"))
    {
    And:
	Randlast--;
	return mk(and, p1, e1());
    }
    else if (EQ(a, "(") || EQ(a, "!") || (*a == '-' && !EQ(a, "-o")))
    {
	--Ai;
	goto And;
    }
    else if (Ai <= Argc)
	--Ai;
    return p1;
}

static ANODE *
e2()
{				/* parse NOT (!) */
    int not();

    if (Randlast)
    {
	fputs(catgets(catd,NL_SETN,5, "find: operand follows operand\n"), stderr);
	exit(1);
    }
    Randlast++;
    if (EQ(nxtarg(), "!"))
	return mk(not, e3(), (ANODE *)0);
    else if (Ai <= Argc)
	--Ai;
    return e3();
}

#define NO_ARGUMENT	0
#define ONE_ARGUMENT	1
#define SIGNED_ARGUMENT 2

#define CMD_atime	 1
#define CMD_cpio	 2
#define CMD_ctime	 3
#define CMD_depth	 4
#define CMD_exec	 5
#define CMD_fsonly	 6
#define CMD_fstype	 7
#define CMD_group	 8
#define CMD_inum	 9
#define CMD_links	10
#define CMD_mtime	11
#define CMD_name	12
#define CMD_ncpio	13
#define CMD_ok		14
#define CMD_only	15
#define CMD_path	16
#define CMD_perm	17
#define CMD_print	18
#define CMD_prune	19
#define CMD_quit	20
#define CMD_size	21
#define CMD_skip	22
#define CMD_type	23
#define CMD_user	24
#define CMD_xdev	25
#define CMD_newer	26

#ifdef SYMLINKS
#define CMD_follow	27
#endif

#if defined(DUX) || defined(DISKLESS)
#define CMD_hidden	28
#if defined(CNODE_DEV)
#define CMD_devcid	29
#endif
#endif /* DUX || DISKLESS */

#ifdef ACLS
#define CMD_acl		30
#endif /* ACLS */

#define CMD_linkedto	31
#define CMD_nouser	32
#define CMD_nogroup	33
#if (defined(DUX) || defined(DISKLESS)) && defined(CNODE_DEV)
#define CMD_nodevcid	34
#endif /* (DUX || DISKLESS) && CNODE_DEV */

#ifdef RCSTOYS
#define CMD_rcsaccess	41
#define CMD_rcslocked	42
#define CMD_rcsprint	43
#define CMD_rcsprintf	44
#define CMD_rcsrev	45
#define CMD_rcsstrict	46
#endif /* RCSTOYS */

static struct cmd_table
{
    char *cmd_name;
    short cmd_index;
    short cmd_arg;
} cmd_table[] =
{
    { "print",	 	CMD_print,	NO_ARGUMENT },
    { "name",		CMD_name,	ONE_ARGUMENT },
    { "path",		CMD_path,	ONE_ARGUMENT },
    { "type",		CMD_type,	ONE_ARGUMENT },
    { "size",		CMD_size, 	SIGNED_ARGUMENT },
    { "atime",		CMD_atime, 	SIGNED_ARGUMENT },
    { "mtime",		CMD_mtime, 	SIGNED_ARGUMENT },
    { "ctime",		CMD_ctime, 	SIGNED_ARGUMENT },
    { "user",		CMD_user, 	SIGNED_ARGUMENT },
    { "group",		CMD_group, 	SIGNED_ARGUMENT },
    { "links",		CMD_links, 	SIGNED_ARGUMENT },
    { "perm",		CMD_perm, 	SIGNED_ARGUMENT },
    { "inum",		CMD_inum, 	SIGNED_ARGUMENT },
    { "linkedto",	CMD_linkedto, 	ONE_ARGUMENT },
    { "depth",	 	CMD_depth,	NO_ARGUMENT },
    { "fsonly",		CMD_fsonly,	ONE_ARGUMENT },
    { "fstype",		CMD_fstype,	ONE_ARGUMENT },
    { "xdev",		CMD_xdev,	NO_ARGUMENT },
    { "prune",	 	CMD_prune,	NO_ARGUMENT },
    { "skip",		CMD_skip,	NO_ARGUMENT },
    { "only",		CMD_only,	NO_ARGUMENT },
    { "exec",		CMD_exec,	ONE_ARGUMENT }, /* at least */
    { "ok",		CMD_ok,		ONE_ARGUMENT }, /* at least */
    { "cpio",		CMD_cpio,	ONE_ARGUMENT },
    { "ncpio",		CMD_ncpio,	ONE_ARGUMENT },
    { "mountstop",	CMD_xdev,	NO_ARGUMENT },
    { "quit",		CMD_quit,	NO_ARGUMENT },
    { "newer",		CMD_newer,	ONE_ARGUMENT },
#ifdef SYMLINKS
    { "follow",	 	CMD_follow,	NO_ARGUMENT },
#endif
#if defined(DUX) || defined(DISKLESS)
    { "hidden",	 	CMD_hidden,	NO_ARGUMENT },
#if defined(CNODE_DEV)
    { "devcid",		CMD_devcid, 	SIGNED_ARGUMENT },
    { "nodevcid",	CMD_nodevcid, 	NO_ARGUMENT },
#endif
#endif
    { "nouser",		CMD_nouser, 	NO_ARGUMENT },
    { "nogroup",	CMD_nogroup, 	NO_ARGUMENT },
#ifdef ACLS
    { "acl",		CMD_acl,	ONE_ARGUMENT },
#endif
#ifdef RCSTOYS
    { "rcsaccess",	CMD_rcsaccess,	ONE_ARGUMENT },
    { "rcslocked",	CMD_rcslocked,	ONE_ARGUMENT },
    { "rcsprint",	CMD_rcsprint,	NO_ARGUMENT },
    { "rcsprintf",	CMD_rcsprintf,	ONE_ARGUMENT },
    { "rcsrev",		CMD_rcsrev,	ONE_ARGUMENT },
    { "rcsstrict",	CMD_rcsstrict,	NO_ARGUMENT },
    { "rcstrict",	CMD_rcsstrict,	NO_ARGUMENT },
#endif
    { NULL, 0, 0 }
};

static ANODE *
e3()
{				/* parse parens and predicates */
    extern int parse_mode();
    int exeq(), ok(),
	name(), path(),
	tst_mtime(), tst_atime(), tst_ctime(),
	user(), group(),
	size(), csize(),
	perm(), links(), print(),
        prune(), skip(), only(), mountstop(),
        fstype(), fsonly(),
#if defined(DUX) || defined(DISKLESS)
        hidden(),
#if defined(CNODE_DEV)
        devcid(), nodevcid(),
#endif /* CNODE_DEV */
#endif /* DUX || DISKLESS */
#ifdef SYMLINKS
        follow(),
#endif /* SYMLINKS */
	quit(),
#ifdef ACLS
	aclmatch(), aclinclude(), aclopt(), staerror(),
#endif /* ACLS */
#ifdef RCSTOYS
	rcsaccess(), rcslocked(), rcsprint(), rcsprintf(),
	rcsrev(), rcsstrict(), revopen(),
#endif
	nouser(), nogroup(),
        type(), ismount(), ino(), is_link(), depth(), cpio(), newer();

    ANODE *p1;
    ANODE *mkret;
    register char *a, *b, s;
    int i, j;
    struct cmd_table *cmd;
#ifdef ACLS
    int ninpatt;	/* Number of entries in inpatt ACL */
    struct acl_entry_patt *inpatt;
#endif /* ACLS */

    a = nxtarg();
    if (EQ(a, "("))
    {
	Randlast--;
	p1 = exp();
	a = nxtarg();
	if (!EQ(a, ")"))
	    goto err;
	return p1;
    }

    /*
     * Lookup this primary in the command table
     */
    for (cmd = cmd_table, b = a+1; cmd->cmd_name != NULL; cmd++)
    {
	if (cmd->cmd_index == CMD_newer)
	{
	    /*
	     * -newer can also be -newer[acm[acm]].  Verify that
	     * it is one of these permutations.
	     */
	    if (strncmp("newer", b, 5) == 0)
	    {
		if (!(b[5] == '\0' ||
		    ((b[5] == 'a' ||
		      b[5] == 'c' ||
		      b[5] == 'm')) &&
		      (b[6] == '\0' ||
		       (b[7] == '\0' && (b[6] == 'a' ||
					 b[6] == 'c' ||
					 b[6] == 'm')))))
		    goto err;
		break;
	    }
	}
	else
	    if (EQ(b, cmd->cmd_name))
		break;
    }

    if (cmd->cmd_name == NULL)
    {
    err:
	fprintf(stderr,
	    catgets(catd,NL_SETN,10, "find: bad option %s\n"), a);
	exit(1);
    }

    /*
     * If this takes an argument, set 'b' to be the operand.
     */
    if (cmd->cmd_arg != NO_ARGUMENT)
    {
	if (Ai >= Argc) /* Make sure that there is one. */
	{
	    fprintf(stderr,
		catgets(catd,NL_SETN,25, "find: %s requires an argument\n"), a);
	    exit(1);
	}

	b = nxtarg();

	/*
	 * If the argument may be signed, set 's' to the sign and
	 * point 'b' to the rest of the argument.
	 */
	if (cmd->cmd_arg == SIGNED_ARGUMENT)
	{
	    s = *b;
	    if (s == '+' || s == '-')
		b++;
	}
    }

    switch (cmd->cmd_index)
    {
    case CMD_print:
	usefulAction++;
	return mk(print, (ANODE *)0, (ANODE *)0);

    case CMD_depth:
	depthf = 1;
	return mk(depth, (ANODE *)0, (ANODE *)0);

    case CMD_xdev:
	mountstopflag = 1;
	return mk(mountstop, (ANODE *)0, (ANODE *)0);

    case CMD_only:
	Returndefault = WALKFS_SKIP;
	return mk(only, (ANODE *)0, (ANODE *)0);

    case CMD_prune:
	return mk(prune, (ANODE *)0, (ANODE *)0);

    case CMD_skip:
	return mk(skip, (ANODE *)0, (ANODE *)0);

    case CMD_quit:
	return mk(quit, (ANODE *)0, (ANODE *)0);

    case CMD_name:
	return mk(name, (ANODE *)b, (ANODE *)0);

    case CMD_path:
	return mk(path, (ANODE *)b, (ANODE *)0);

    case CMD_mtime:
	return mk(tst_mtime, (ANODE *)atoi(b), (ANODE *)s);

    case CMD_atime:
	return mk(tst_atime, (ANODE *)atoi(b), (ANODE *)s);

    case CMD_ctime:
	return mk(tst_ctime, (ANODE *)atoi(b), (ANODE *)s);

    case CMD_links:
	return mk(links, (ANODE *)atoi(b), (ANODE *)s);

     case CMD_size:
	mkret = mk(size, (struct anode *)atoi(b), (struct anode *)s);
	while (isdigit(*b))
	    b++;
	if (*b == 'c')
	    mkret->F = csize;
	return mkret;

    case CMD_perm:
	if ((i = parse_mode(b, 0)) == -1)
	{
	    fprintf(stderr,
		catgets(catd,NL_SETN,28, "find: invalid mode %s\n"), b);
		exit(1);
	}
	return mk(perm, (struct anode *)i, (struct anode *)s);

    case CMD_user:
	if ((i = getunum(UID, b)) == -1)
	{
	    char *sb = b;

	    while (*sb >= '0' && *sb <= '9')
		sb++;

	    if (*sb != '\0' || (i = atoi(b)) < 0 || i > 65536)
	    {
		fprintf(stderr,
		    catgets(catd,NL_SETN,6, "find: cannot find -user %s\n"), b);
		exit(1);
	    }
	}
	return mk(user, (ANODE *)i, (ANODE *)s);

    case CMD_nouser:
	return mk(nouser, (ANODE *)0, (ANODE *)0);

    case CMD_group:
	if ((i = getunum(GID, b)) == -1)
	{
	    char *sb = b;

	    while (*sb >= '0' && *sb <= '9')
		sb++;

	    if (*sb != '\0' || (i = atoi(b)) < 0 || i > 65536)
	    {
		fprintf(stderr,
		    catgets(catd,NL_SETN,7, "find: cannot find -group %s\n"), b);
		exit(1);
	    }
	}
	return mk(group, (ANODE *)i, (ANODE *)s);

    case CMD_nogroup:
	return mk(nogroup, (ANODE *)0, (ANODE *)0);

    case CMD_type:
	j = 0;
	switch (b[1] == '\0' ? *b:0) /* force error if not 1 char */
	{
	case 'd':
	    i = S_IFDIR;
	    break;
	case 'b':
	    i = S_IFBLK;
	    break;
	case 'c':
	    i = S_IFCHR;
	    break;
	case 'p':
	    i = S_IFIFO;
	    break;
	case 'f':
	    i = S_IFREG;
	    break;
	case 'M':
	    return mk(ismount, (ANODE *)0, (ANODE *)0);
#ifdef S_IFSOCK
	case 's':
	    i = S_IFSOCK;
	    break;
#endif /* S_IFSOCK */
#ifdef SYMLINKS
	case 'l':
	    i = S_IFLNK;
	    break;
#endif /* SYMLINKS */
#if defined(DUX) || defined(DISKLESS)
	case 'H': /* -type H means check for a dir with setuid */
	    i = S_IFDIR;
	    j = S_ISUID;
	    ++hflag;
	    break;
#endif /* defined(DUX) || defined(DISKLESS) */
#if defined(RFA) || defined(OLD_RFA)
	case 'n':
	    i = S_IFNWK;
	    break;
#endif /* RFA */
#ifdef RT
	case 'r':
	    i = S_IFREC;
	    break;
	case 'm':
	    i = S_IFEXT;
	    break;
	case '1':
	    i = S_IF1EXT;
	    break;
#endif /* RT */
#ifdef RCSTOYS
	case 'R':
	    return mk(revopen, (ANODE *)0, (ANODE *)0);
#endif /* RCSTOYS */
	default:
	    fprintf(stderr,
		catgets(catd,NL_SETN,23, "find: bad -type %s\n"),
		b);
	    exit(1);
	}
	return mk(type, (ANODE *)i, (ANODE *)j);

    case CMD_fstype:
	{
	    int val;

	    if (EQ(b, "hfs"))
		val = MOUNT_UFS;
#if defined(NFS) || defined(HP_NFS)
	    else if (EQ(b, "nfs"))
		val = MOUNT_NFS;
#endif
	    else if (EQ(b, "cdfs"))
		val = MOUNT_CDFS;
	    else
	    {
	    bad_fstype:
		fprintf(stderr,
		    catgets(catd,NL_SETN,56, "find: invalid file system type %s\n"), b);
		exit(1);
	    }
	    return mk(fstype, (ANODE *)val, (ANODE *)0);
	}

    case CMD_fsonly:
	{
	    if (EQ(b, "hfs"))
		fsonlyval |= ONLY_HFS;
#if defined(NFS) || defined(HP_NFS)
	    else if (EQ(b, "nfs"))
		fsonlyval |= ONLY_NFS;
#endif
	    else if (EQ(b, "cdfs"))
		fsonlyval |= ONLY_CDFS;
	    else
		goto bad_fstype;
	    return mk(fsonly, (ANODE *)0, (ANODE *)0);
	}

    case CMD_exec:
    case CMD_ok:
	{
	    /*
	     * Make sure we have at least something to exec
	     */
	    if (b[0] == ';' && b[1] == '\0')
	    {
		fprintf(stderr,
		    catgets(catd,NL_SETN,27, "find: %s must specify a command to run\n"), a);
		exit(1);
	    }

	    usefulAction++;
	    i = Ai - 1;

#if defined(NLS) || defined(NLS16)
	    if (yeschar == 0)  /* need to initialize yeschar */
	    {
		unsigned char yesstr[3];

		strncpy(yesstr, nl_langinfo(YESSTR), 2);
		if (FIRSTof2(yesstr[0]))    /* two-byte yes char */
		    yesstr[2] = 0;
		else                        /* one-byte yes char */
		    yesstr[1] = 0;

		yeschar = CHARAT(yesstr);
		if (yeschar == 0)
		    yeschar = 'y';
	    }
#endif /* NLS || NLS16 */

	    /*
	     * Scan argument list until we find a terminating ';'
	     * note: Ai is a global that is modified by nxtarg().
	     */
	    while (Ai < Argc)
		if ((b = nxtarg())[0] == ';' && b[1] == '\0')
		    return mk(cmd->cmd_index == CMD_exec ? exeq : ok,
			      (ANODE *)i, (ANODE *)0);

	    /*
	     * Ate up all the args, but didn't find a ';'
	     */
	    fprintf(stderr,
		catgets(catd,NL_SETN,26, "find: %s not terminated with ';'\n"), a);
	    exit(1);
	}

    case CMD_cpio:
    case CMD_ncpio:
	usefulAction++;
	cpiof = depthf = 1;
#if defined(DUX) || defined(DISKLESS)
	cpio_path[0] = '\0';
	cpio_end = cpio_path;
#endif /* DUX || DISKLESS */

	{
	    CPIO *desc = cpio_open(b,
			     cmd->cmd_index==CMD_ncpio? CPIO_ASCII : 0);
#ifdef RT
	    setio(-1, 1);		/* turn on physio */
#endif
	    return mk(cpio, (ANODE *)desc, (ANODE *)0);
	}

    case CMD_newer:
	{
	    char *p = a + 6;
	    time_t *t1p, *t2p;

#ifdef SYMLINKS
	    if ((followflag ? *stat : *lstat)(b, &Statb) < 0)
#else
	    if (stat(b, &Statb) < 0)
#endif /* SYMLINKS */
	    {
		fprintf(stderr,
		    catgets(catd,NL_SETN,9, "find: cannot access %s\n"),
		    b);
		exit(1);
	    }

	    if (Nnewer >= NNEW)
	    {
		Newer = (time_t *)malloc(sizeof(time_t) * NNEW);
		Nnewer = 0;
	    }

	    t1p = t2p = &(Statb.st_mtime);
	    switch (*p)
	    {
	    case '\0':
		break;
	    case 'm':
		p++;
		break;
	    case 'a':
		t1p = &(Statb.st_atime);
		p++;
		break;
	    case 'c':
		t1p = &(Statb.st_ctime);
		p++;
		break;
	    }

	    switch (*p)
	    {
	    case '\0':
		break;
	    case 'm':
		p++;
		break;
	    case 'a':
		t2p = &(Statb.st_atime);
		p++;
		break;
	    case 'c':
		t2p = &(Statb.st_ctime);
		p++;
		break;
	    }

	    Newer[Nnewer] = *t2p;
	    return mk(newer, (ANODE *)t1p, (ANODE *)(&Newer[Nnewer++]));
	}

    case CMD_inum:
	{
	    unsigned long num;
	    char *ptr = NULL;

	    if (isdigit(*b))
		num = strtoul(b, &ptr, 10);

	    /*
	     * If the argument isn't a number, print an error message.
	     */
	    if (ptr == NULL || *ptr != '\0')
	    {
		fprintf(stderr,
		    catgets(catd,NL_SETN,24, "find: bad -inum value %s\n"), b);
		    exit(1);
	    }
	    return mk(ino, (ANODE *)num, (ANODE *)s);
	}

    case CMD_linkedto:
	{
	    struct stat statb;

#ifdef SYMLINKS
	    if ((followflag ? *stat : *lstat)(b, &statb) < 0)
#else
	    if (stat(b, &Statb) < 0)
#endif /* SYMLINKS */
	    {
		fprintf(stderr,
		    catgets(catd,NL_SETN,9, "find: cannot access %s\n"),
		    b);
		exit(1);
	    }

	    return mk(is_link,
		      (ANODE *)statb.st_dev, (ANODE *)statb.st_ino);
	}
#ifdef SYMLINKS
    case CMD_follow:
	followflag = 1;
	return mk(follow, (ANODE *)0, (ANODE *)0);
#endif /* SYMLINKS */

#if defined(DUX) || defined(DISKLESS)
    case CMD_hidden:
	hflag = 1;
	return mk(hidden, (ANODE *)0, (ANODE *)0);

#if defined(CNODE_DEV)
    case CMD_devcid:
    {
	int n;

        if (isdigit(*b))
	    n = atoi(b);
	else
	{
	    struct cct_entry *centry = getccnam(b);

	    if (centry)
		n = centry->cnode_id;
	    else
	    {
		fprintf(stderr,
		    catgets(catd,NL_SETN,59, "find: cannot find -devcid %s\n"), b);
		exit(1);
	    }
	}
	return mk(devcid, (ANODE *)n, (ANODE *)s);
    }

    case CMD_nodevcid:
	return mk(nodevcid, (ANODE *)0, (ANODE *)0);

#endif /* CNODE_DEV */
#endif /* DUX || DISKLESS */
#ifdef ACLS
    case CMD_acl:
	if (*b == '=')
	{
	    /*
	     * looking for an exact match
	     */
	    b++;
	    inpatt = (struct acl_entry_patt *)malloc(APSIZE * NACLENTRIES);
	    ninpatt = strtoaclpatt(b, NACLENTRIES, inpatt);
	    if (ninpatt < 0)
	    {
		staerror(-ninpatt);
		exit(1);
	    }
	    return mk(aclmatch, (ANODE *)inpatt, (ANODE *)ninpatt);
	}
	else
	{
	    /*
	     * looking for existance of optional entries
	     */
	    if (EQ(b, "opt"))
		return mk(aclopt, (ANODE *)0, (ANODE *)0);
	    else
	    {
		/*
		 * looking for inclusion of specified entries
		 */
		inpatt = (struct acl_entry_patt *)malloc(APSIZE * NACLENTRIES);
		/* make sure the ACL pattern is valid */
		ninpatt = strtoaclpatt(b, NACLENTRIES, inpatt);
		if (ninpatt <= 0)
		{
		    staerror(-ninpatt);
		    exit(1);
		}
		return mk(aclinclude,
			  (ANODE *)inpatt, (ANODE *)ninpatt);
	    }
	}
#endif /* ACLS */

#ifdef RCSTOYS
    case CMD_rcsprint:
	usefulAction++;
	return mk(rcsprint, (ANODE *)0, (ANODE *)0);

    case CMD_rcsprintf:
	usefulAction++;
	return mk(rcsprintf, (ANODE *)b, (ANODE *)0);

    case CMD_rcsstrict:
	return mk(rcsstrict, (ANODE *)0, (ANODE *)0);

    case CMD_rcslocked:
	return mk(rcslocked, (ANODE *)b, (ANODE *)0);

    case CMD_rcsaccess:
	return mk(rcsaccess, (ANODE *)b, (ANODE *)0);

    case CMD_rcsrev:
	{
	    static struct optbl_t
	    {
		char *name;
		int value;
	    } optbl[] =
	    {
		{ "<",   RCS_LT }, { "lt",  RCS_LT },
		{ "<=",  RCS_LE }, { "le",  RCS_LE },
		{ "==",  RCS_EQ }, { "=",   RCS_EQ }, { "eq",  RCS_EQ },
		{ ">=",  RCS_GE }, { "ge",  RCS_GE },
		{ ">",   RCS_GT }, { "gt",  RCS_GT },
		{ "!=",  RCS_NE }, { "ne",  RCS_NE },
		{ NULL,  -1 }
	    };
	    char *op_str = nxtarg();
	    int i;
	    struct rcs_arg *arg =
		       (struct rcs_arg *)malloc(sizeof(struct rcs_arg));

	    arg->tag1 = b;
	    arg->tag2 = nxtarg();

	    for (i = 0; optbl[i].name != NULL; i++)
		if (EQ(op_str, optbl[i].name))
		    break;

	    if (optbl[i].name == NULL)
	    {
		fprintf(stderr, "find: invalid operator '%s'\n",
		    op_str);
		exit(1);
	    }
	    return mk(rcsrev, (ANODE *)arg, (ANODE *)(optbl[i].value));
	}
#endif /* RCSTOYS */
    }
}

static ANODE *
mk(f, l, r)
int (*f)();
ANODE *l, *r;
{
    /*
     * If we ran out of space in our node buffer chunk, allocate
     * a new buffer chunk.
     */
    if (Nn >= MAXNODES)
    {
	Node = (ANODE *)malloc(sizeof(ANODE) * MAXNODES);
	Nn = 0;
    }
    Node[Nn].F = f;
    Node[Nn].L = l;
    Node[Nn].R = r;
    return &(Node[Nn++]);
}

static char *
nxtarg()
{				/* get next arg from command line */
    static strikes = 0;

    if (strikes == 3)
    {
	fputs(catgets(catd,NL_SETN,11, "find: incomplete statement\n"), stderr);
	exit(1);
    }
    if (Ai >= Argc)
    {
	strikes++;
	Ai = Argc + 1;
	return "";
    }
    return Argv[Ai++];
}

/* execution time functions */
static int
and(p)
register ANODE *p;
{
    return ((*p->L->F)(p->L)) && ((*p->R->F)(p->R)) ? 1 : 0;
}

static int
or(p)
register ANODE *p;
{
    return ((*p->L->F)(p->L)) || ((*p->R->F)(p->R)) ? 1 : 0;
}

static int
not(p)
register ANODE *p;
{
    return !((*p->L->F)(p->L));
}

#if defined(DUX) || defined(DISKLESS)
static int
hidden()
{
    return 1;			/* -hidden is always true */
}
#endif /* DUX || DISKLESS */

static int
name(p)
register struct { int f; char *pattern; } *p;
{
    return (fnmatch(p->pattern, Fname, 0) == 0) ? 1 : 0;
}

static int
path(p)
register struct { int f; char *pattern; } *p;
{
    return (fnmatch(p->pattern, Pathname, 0) == 0) ? 1 : 0;
}

static int
print()
{
    puts(Pathname);
    return 1;
}

static int
prune()
{
    /*
     * -prune only cause a WALKFS_SKIP if the current entry is
     * a directory.
     */
    if ((Statb.st_mode & S_IFMT) == S_IFDIR)
	Return = WALKFS_SKIP;
    return 1;
}

static int
skip()
{
    /*
     * -skip is like -prune, but it always causes a WALKFS_SKIP
     * even if the current entry is not a directory.
     */
    Return = WALKFS_SKIP;
    return 1;
}

static int
only()
{
    /*
     * -only reverses the WALKFS_SKIP that is the Returndefault
     */
    Return = WALKFS_OK;
    return 1;
}

static int
mountstop()
{
    return 1;
}

#ifdef SYMLINKS
static int
follow()
{
    return 1;
}
#endif /* SYMLINKS */

static int
quit()
{
    exit(3);
}

static int
tst_mtime(p)
register struct { int f, t, s; } *p;
{
    return scomp((int)((Now - Statb.st_mtime) / A_DAY), p->t, p->s);
}

static int
tst_atime(p)
register struct { int f, t, s; } *p;
{
    return scomp((int)((Now - Statb.st_atime) / A_DAY), p->t, p->s);
}

static int
tst_ctime(p)
register struct { int f, t, s; } *p;
{
    return scomp((int)((Now - Statb.st_ctime) / A_DAY), p->t, p->s);
}

static int
user(p)
register struct { int f, u, s; } *p;
{
    return scomp(Statb.st_uid, p->u, p->s);
}

static int
ino(p)
register struct { int f, u, s; } *p;
{
    return scomp((int)Statb.st_ino, p->u, p->s);
}

static int
is_link(p)
register struct { int f; dev_t d; ino_t i; } *p;
{
    return Statb.st_ino == p->i && Statb.st_dev == p->d;
}

static int
group(p)
register struct { int f, u; } *p;
{
    return p->u == Statb.st_gid;
}

static int
links(p)
register struct { int f, link, s; } *p;
{
    return scomp(Statb.st_nlink, p->link, p->s);
}

static int
size(p)
register struct { int f, sz, s; } *p;
{
    return scomp((int)((Statb.st_size + (BUFSIZE - 1)) / BUFSIZE), p->sz, p->s);
}

static int
csize(p)
register struct { int f, sz, s; } *p;
{
    return scomp((int)Statb.st_size, p->sz, p->s);
}

static int
perm(p)
register struct { int f, per, s; } *p;
{
    register i;
    i = (p->s == '-') ? p->per : 07777;	/* '-' means only arg bits */
    return (Statb.st_mode & i & 07777) == p->per;
}

static int
type(p)
register struct { int f, per, s; } *p;
{
    return ((Statb.st_mode & S_IFMT) == p->per) &&
	    (p->s ? (Statb.st_mode & p->s) : 1);
}

#if defined(DUX) || defined(CNODE_DEV)
/*
 * devcid -- return TRUE if this is a block or character special
 *          file with the correct st_rcnode value.
 */
static int
devcid(p)
register struct { int f; int rcnode; int sign; } *p;
{
    if ((Statb.st_mode & S_IFMT) == S_IFCHR ||
	(Statb.st_mode & S_IFMT) == S_IFBLK)
	return scomp(Statb.st_rcnode, p->rcnode, p->sign);
    else
	return 0;
}
#endif /* DUX/CNODE_DEV */

static int
ismount()
{
    return Info->ismountpoint;
}

static int
fstype(p)
register struct { int f; unsigned int fstype; } *p;
{
    return Info->st.st_fstype == p->fstype;
}

static int
fsonly(p)
register struct { int f; int fstype; } *p;
{
    return 1;
}

static int
exeq(p)
register struct { int f, com; } *p;
{
    fflush(stdout);		/* to flush possible `-print' */
    return doex(p->com);
}

static int
ok(p)
struct { int f, com; } *p;
{
    int i, b;

    fflush(stdout);		/* to flush possible `-print' */
    fprintf(stderr, "< %s ... %s >?   ", Argv[p->com], Pathname);
    fflush(stderr);

#if defined(NLS) || defined(NLS16)
    if (FIRSTof2(i = getchar()))
        if (SECof2(b = getchar()))
	    i = (i << 8) | b;
    b = i;
#else
    i = b = getchar();
#endif /* NLS || NLS16 */

    /*
     * Skip up to a newline, abort if ^D
     */
    while (b != '\n')
	if (b == EOF)
	{
	    fputc('\n', stderr);
	    exit(2);
	}
	else
	    b = getchar();

    return (i == yeschar) ? doex(p->com) : 0;
}

static int
depth()
{
    return 1;
}

#ifdef ACLS
/*
 * aclmatch -
 * Find if a file's entries exactly match the input entries.
 * Passed p, which is a type anode and contains a pointer to
 * the ACL to match in the left and a pointer to the number
 * of entries in that ACL in the right.
 */
static int
aclmatch(p)
ANODE	*p;
{
    struct acl_entry_patt *inpatt;  /* acl from input string (in p->L) */
    struct acl_entry facl[NACLENTRIES];	/* acl on file */
    int isat,		/* is there an @ in the input? */
        match,		/* flag to keep track of match or not */
        i,		/* counter for input entries */
        f,		/* counter for file entries */
        ninpatt,	/* number of entries in input string (p->R) */
        nfacl;			/* number of entries in the file's ACL */
    int matches[NACLENTRIES];	/* keep track of each entry matched */

    inpatt = (struct acl_entry_patt *)p->L;
    ninpatt = (int)p->R;
    match = 0;
    memset(matches, 0, sizeof (matches));

    /* get the file's ACL */
    nfacl = getacl(Info->shortpath, NACLENTRIES, facl);
    if (nfacl < 0)		/* this will skip remote files */
	return 0;

    /*
     * For each pattern on the input line (inpatt) there must
     * be a matching entry in the file's acl (facl).  For each
     * entry in the file's acl there must be an input entry
     * which matches it.
     */
    for (i = 0; i < ninpatt; i++)
    {
	for (f = 0; f < nfacl; f++)
	    if ((aclcmp(&inpatt[i], &facl[f])) == 1)
		matches[f] = match = 1;

	/* found a match for this input pattern? */
	if (match)
	    match = 0;
	else
	    return 0;
    }

    /* Make sure all of file's entries were matched */
    for (f = 0; f < nfacl; f++)
	if (!matches[f])
	    return 0;

    /* If not all were matched return 0, otherwise return 1 */
    return 1;
}

/*
 *   aclinclude -
 *   Find if a file's entries include the input entries.
 *	p->L contains ACL pattern that was input.  p->R contains
 *	the number of entries in the input.
 */
static int
aclinclude(p)
ANODE *p;
{
    struct acl_entry facl[NACLENTRIES];	/* file's ACL */
    struct acl_entry_patt *inpatt;  /* Input ACL (p->L) */
    int ninpatt,	/* number of entries in input ACL (p->R) */
        nfacl,		/* number of entries in file's ACL */
        match,		/* flag - did we find a match? */
        i,
        f;

    match = 0;
    inpatt = (struct acl_entry_patt *)p->L;
    ninpatt = (int)p->R;

    /* get the file's ACL */
    nfacl = getacl(Info->shortpath, NACLENTRIES, facl);
    if (nfacl < 0)		/* this will skip remote files */
	return 0;

    /*
     * for each entry pattern input (inpatt) there should be a matching
     * entry in the file's acl (facl).  If so, we have a true condition.
     */
    for (i = 0; i < ninpatt; i++)
    {
	for (f = 0; f < nfacl; f++)
	    if ((aclcmp(&inpatt[i], &facl[f])) == 1)
	    {
		match = 1;
		break;
	    }

	if (match)
	    match = 0;
	else
	    return 0;		/* doesn't include all input entries */
    }
    return 1;			/* includes all input entries */
}

/*
 *  aclopt -
 *  Look for existance of optional entries.  Check the st_acl field in
 *  the stat structure to find out.  This returns 0 ("no") on
 *  remote files (NFS or RFA).
 */
static int
aclopt()
{
    return Info->st.st_acl ? 1 : 0;
}
#endif /* ACLS */

static int
newer(p)
register struct { int f; time_t *t1, *t2; } *p;
{
    return *(p->t1) > *(p->t2);
}

/* support functions */
static int
scomp(a, b, s)			/* funny signed compare */
register a, b;
register char s;
{
    if (s == '+')
	return a > b;
    if (s == '-')
	return a < b;
    return (a == b);
}

static int
doex(com)
{
    register np;
    register char *na;
    static char *nargv[50];
    static ccode;
    static pid;

    ccode = np = 0;
    while (na = Argv[com++])
    {
	if (strcmp(na, ";") == 0)
	    break;
	if (strcmp(na, "{}") == 0)
	    nargv[np++] = Pathname;
	else
	    nargv[np++] = na;
    }
    nargv[np] = 0;
    if (np == 0)
	return 9;
    if (pid = fork())
	while (wait(&ccode) != pid);
    else
    {				/* child */
	fchdir(home_fd);
	execvp(nargv[0], nargv);
	exit(1);
    }
    return ccode ? 0 : 1;
}

static int
getunum(t, s)
int t;
char *s;
{
    register i;
    struct passwd *getpwnam(), *pw;
    struct group *getgrnam(), *gr;

    i = -1;
    if (t == UID)
    {
	if (((pw = getpwnam(s)) != (struct passwd *)NULL) && pw != (struct passwd *)EOF)
	    i = pw->pw_uid;
    }
    else
    {
	if (((gr = getgrnam(s)) != (struct group *)NULL) && gr != (struct group *)EOF)
	    i = gr->gr_gid;
    }
    return i;
}

/**********************************************************************/
/*	This strrchr routine will search for 16-bit character rather  */
/*	then an 8-bit character.				      */
/**********************************************************************/

#ifdef NLS16
#define CNULL (unsigned char *)0

char *
strrchr(sp, c)
register unsigned char *sp;
register unsigned c;
{
    register unsigned char *r;

    if (sp == CNULL)
	return (char *)CNULL;

    r = CNULL;
    do
    {
	if (CHARAT(sp) == c)
	    r = sp;
    } while (CHARADV(sp));
    return (char *)r;
}
#endif /* NLS16 */

#ifdef ACLS
/*
 * aclcmp - compare the acls
 *	inpatt - is input and may contain "don't care" characters.
 *	facl - is a file's acl and may NOT contain "don't care".
 * returns  1 if matches, 0 if no match.
 */

static int
aclcmp(inpatt,facl)
struct acl_entry_patt  *inpatt;
struct acl_entry *facl;
{
    int uid,
        gid;

    /*
     * set up uid and gid to be that of the owner of the file if
     * a special character was given.  Otherwise set it to the
     * input user or group.
     */
    if (inpatt->uid == ACL_FILEOWNER)
	uid = Info->st.st_uid;
    else
	uid = inpatt->uid;
    if (inpatt->gid == ACL_FILEGROUP)
	gid = Info->st.st_gid;
    else
	gid = inpatt->gid;

    /*
     * check to see if the input uid matches the file's uid or
     * if the input is a "don't care" special character.  Then
     * check the same for the group and look for a matching mode.
     */
    if (((uid == facl->uid) || (inpatt->uid == ACL_ANYUSER)) &&
	    ((gid == facl->gid) || (inpatt->gid == ACL_ANYGROUP)) &&
    /*
     * make sure the entry to match has the
     * proper modes turned on and off.
     */
	    ((((facl->mode & inpatt->onmode) == inpatt->onmode))
		&& ((~facl->mode & inpatt->offmode) == inpatt->offmode)))
	return 1;
    return 0;			/* not found. */
}


/*
 * MESSAGES FOR ERRORS FROM strtoaclpatt():
 *
 * These are the mappings of error values returned from strtoaclpatt.
 * ERROR_OFFSET is the base message number for staerror().
 */

#define	ERROR_OFFSET 40		/* offset so NLS error numbers don't conflict */
#define	ERROR_MAX 10		/* highest value, except last two */

static char *error_sta[] = {
/* catgets 40 */  /*  0 */  "(no error)",
/* catgets 41 */  /*  1 */  "entry doesn't start with \"(\" in short form: ",
/* catgets 42 */  /*  2 */  "entry doesn't end with \")\" in short form: ",
/* catgets 43 */  /*  3 */  "user name not terminated with dot in entry: ",
/* catgets 44 */  /*  4 */  "group name not terminated correctly in entry: ",
/* catgets 45 */  /*  5 */  "user name is null in entry: ",
/* catgets 46 */  /*  6 */  "group name is null in entry: ",
/* catgets 47 */  /*  7 */  "invalid user name in entry: ",
/* catgets 48 */  /*  8 */  "invalid group name in entry: ",
/* catgets 49 */  /*  9 */  "invalid mode in entry: ",
/* catgets 40 */  /* 10 */  "more than 16 entries at entry: ",
/* catgets 51 */  /* 11 */  "unknown error from strtoaclpatt(): ",
};

/*
 * staerror -
 * Print an english version of a strtoaclpatt-error.
 * Given an error number (positive value = (-return from strtoaclpatt),
 * report error from  strtoaclpatt() using global error_sta[].
 */

static
staerror(errnum)
int	errnum;
{
    extern char *aclentrystart[];   /* for error report	 */
    register char chsave;	    /* saved character	 */
    int nl_errnum;

    /*
     * Translate meaning of error and handle case where error is
     * unknown.
     */
    if (errnum > ERROR_MAX)	/* unknown value */
	errnum = ERROR_MAX + 1;	/* use special	 */
    nl_errnum = errnum + ERROR_OFFSET;

    /*
     * save this value so we can put it back and not alter the input
     * string.
     */
    chsave = aclentrystart[1][0];
    aclentrystart[1][0] = '\0';	/* temporarily end string */

    /*
     * print the message
     */
    fputs(catgets(catd,NL_SETN,55, "find: "), stderr);
    fputs(catgets(catd,NL_SETN,nl_errnum, error_sta[errnum]), stderr);
    fputs(*aclentrystart, stderr);
    fputc('\n', stderr);

    aclentrystart[1][0] = chsave;   /* repair ACL string */
}
#endif /* ACLS */
