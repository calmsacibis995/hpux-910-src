#ifndef lint
static char HPUX_ID[] = "@(#) $Revision: 66.5 $";
#endif
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <cluster.h>
#include <string.h>
#include <errno.h>
#ifdef ACLS
#include <sys/acl.h>
#endif /* ACLS */
#include <dirent.h>
#include <unistd.h>

#ifdef NLS || NLS16
#   include <nl_ctype.h>
    unsigned char *nl_strrchr();
#else
#   define ADVANCE(p)           (++p)
#   define CHARAT(p)            (*p)
#   define CHARADV(p)           (*p++)
#   define FIRSTof2(x)          0
#   define nl_strrchr(x, y)     ((unsigned char *)strrchr(x, y))
#endif

#define equal(s1,s2)	(strcmp((s1), (s2)) == 0)
#define ISREG(st)	(((st).st_mode & S_IFMT) == S_IFREG)
#define ISDIR(st)	(((st).st_mode & S_IFMT) == S_IFDIR)
#define ISCDF(st)	(ISDIR(st) && ((st).st_mode & S_ISUID))

#ifdef SYMLINKS
#   define STAT lstat
#else
#   define STAT stat
#endif

int num_c;
int cflag;
int dflag;
int fflag;
int errflg;

char *malloc();
char *dirname();

#ifndef TRUE
#   define TRUE  1
#   define FALSE 0
#endif

uid_t myuid;
gid_t mygid;

char *names[MAXCNODE+1];
char default_name[MAXNAMLEN+1];
char message[MAXNAMLEN+1];   /* for printing error messages */

main(argc, argv)
int argc;
char    **argv;
{
    extern uid_t geteuid();
    extern gid_t getegid();
    extern int optind;
    extern char *optarg;
    int c;
    int procerr = 0;

    myuid = geteuid();
    mygid = getegid();

    while ((c = getopt(argc, argv, "d:f:c:")) != EOF)
	switch (c)
	{
	case 'd':
	    if (dflag)
		errflg++;
	    else
	    {
		if (fflag)
		    errflg++;
		dflag++;
		strcpy(default_name, optarg);
	    }
	    break;
	case 'f':
	    if (fflag)
		errflg++;
	    else
	    {
		if (dflag)
		    errflg++;
		fflag++;
		strcpy(default_name, optarg);
	    }
	    break;
	case 'c':
	    names[num_c] = malloc(strlen(optarg) + 1);
	    if (names[num_c] == NULL)
	    {
		fprintf(stderr, "makecdf: malloc error\n");
		exit(1);
	    }
	    strcpy(names[num_c++], optarg);
	    cflag++;
	    break;
	case '?':
	    errflg++;
	    break;
	}

    if (optind >= argc || errflg)
	usage();

    if (num_c == 0) /* no -c options */
    {
	read_clusterconf();
	if (num_c == 0)
	{
	    fprintf(stderr,
"makecdf: you must use at least one \"-c context\" on standalone systems\n");
	    exit(1);
	}
    }

    for (; optind < argc; optind++)
	procerr |= process(argv[optind]);
    return procerr;
}

#if defined(NLS) || defined(NLS16)
/*
 * nl_strrchr() --
 *    strrchr() that works 16 bit strings and characters
 */
unsigned char
*nl_strrchr(s, c)
unsigned char *s;
int c;
{
    unsigned char *spot = (unsigned char *)NULL;

    for (; *s; ADVANCE(s))
        if (CHARAT(s) == c)
            spot = s;

    return spot;
}
#endif /* NLS || NLS16 */

read_clusterconf()
{
    struct cct_entry *cct_entry;

    setccent();
    while (cct_entry = getccent())
    {
	names[num_c] = malloc(strlen(cct_entry->cnode_name) + 1);
	if (names[num_c] == NULL)
	{
	    fprintf(stderr, "makecdf: malloc error\n");
	    exit(1);
	}
	strcpy(names[num_c++], cct_entry->cnode_name);
    }
    endccent();
}

struct stat statbuf;
mode_t parent_mode;
uid_t parent_uid;
gid_t parent_gid;
#ifdef ACLS
unsigned int parent_acl;
#endif /* ACLS */

process(file)
char    *file;
{
    char file_plus[MAXPATHLEN + 1];
    char newname[MAXPATHLEN + 1];
    char master[MAXPATHLEN + 1];
    char parent[MAXPATHLEN + 1];
    int i, retval;
    int file_exists = 0;	/* if 'file' exists but is not a CDF */
    int cdf = 0;		/* if 'file' is already a CDF */
    int src_file = 0;		/* if there is a source to copy */
#ifdef ACLS
    int mode;			/* access mode of file */
    int npacl;			/* # of entries in parent file's ACL */
    struct acl_entry pacl[NACLENTRIES];	/* parent file's ACL */
    int nmacl;			/* # of entries in master file's ACL */
    struct acl_entry macl[NACLENTRIES];	/* Master file's ACL */
#endif /* ACLS */

    /* Collect vital stats on parent for later use */
    dirname(file, parent);
    if (stat(parent, &statbuf))
    {
	sprintf(message, "makecdf: parent directory %s", parent);
	perror(message);
	return 1;
    }
    else
    {
	parent_uid = statbuf.st_uid;
	parent_gid = statbuf.st_gid;
	parent_mode = statbuf.st_mode;
#ifdef ACLS
	parent_acl = statbuf.st_acl;
#endif /* ACLS */
	if (eaccess(parent, W_OK))
	{			/* verify that parent is accessible */
	    sprintf(message, "makecdf: %s", parent);
	    perror(message);
	    return 1;
	}
#ifdef ACLS
	/*
	 * If there are optional entries, get the ACL to set later
	 * and set mode to be st_basemode.
	 * This will silently remove opt entries from remote files.
	 */
	if (statbuf.st_acl)
	{
	    if ((npacl = getacl(parent, NACLENTRIES, pacl)) < 0)
	    {
		parent_mode = statbuf.st_mode;
		sprintf(message, "makecdf: WARNING: Could not getacl %s; using st_mode value", parent);
		perror(message);
	    }
	    else
	    {
		parent_mode = statbuf.st_basemode;
	    }
	}
#endif /* ACLS */
    }

    /* setup the CDF name, determine if it's already a CDF */
    sprintf(file_plus, "%s+", file);
    if (STAT(file_plus, &statbuf) == 0)
    {
	/*
	 * file is already a CDF
	 * check for -d flag
	 */
	if (ISCDF(statbuf))
	{
	    if (!(dflag || fflag))
	    {
		fprintf(stderr,
		    "makecdf: %s is a CDF, no -d or -f was specified\n",
		    file);
		return 1;
	    }
	    file_exists = 0;
	    cdf = 1;
	}
	else
	{
	    /*
	     * "foo+" could exist because it is really "foo" and the
	     * max name length for this file system causes the extra
	     * character to be ignored.  So, we need to figure out
	     * if this is why foo+ exists.
	     */
	    if (toolong(file_plus))
		file_exists = 1;
	    else
	    {
		fprintf(stderr, "makecdf: %s exists and is not a CDF\n",
		    file_plus);
		return 1;
	    }
	}
    }
    else
    {
	if (STAT(file, &statbuf) == 0)
	    file_exists = 1;
	else
	    file_exists = 0;
    }

    /* verify that file is accessible */
    if (myuid)
    {
	/*
	 * if file is a directory, it must be writable for rename
	 * to work
	 */
	if (file_exists && ISDIR(statbuf) && eaccess(file, W_OK))
	{
	    sprintf(message, "makecdf: %s directory must be writable",
		file);
	    perror(message);
	    return 1;
	}

	if ((cdf || file_exists) && (myuid != statbuf.st_uid))
	{
	    sprintf(message, "makecdf: %s", cdf ? file_plus : file);
	    errno = EPERM;
	    perror(message);
	    return 1;
	}
    }

    /* determine where to get data for elements of CDF */
    if (file_exists)
    {
	make_temp_name(file, master);
	if (rename(file, master) != 0)
	{
	    /*
	     * If the errno is EXDEV or the source file was '/',
	     * "file" must be a mount point.  Print a more descriptive
	     * error message to fix FSDlj05895.
	     */
	    if (errno == EXDEV || (file[0] == '/' && file[1] == '\0'))
		fprintf(stderr, "makecdf: %s is a mount point\n", file);
	    else
	    {
		sprintf(message, "makecdf: rename(%s, %s)",
		    file, master);
		perror(message);
	    }
	    return 1;
	}
    }
    else if (fflag)
	strcpy(master, default_name);
    else if (cdf && dflag)
	sprintf(master, "%s/%s", file_plus, default_name);
    else
	master[0] = '\0';

    /* stat the master file */
    if (master[0] == '\0' || equal(master, "-"))
    {
	/*
	 * there is no data for the elements, make some up
	 */
	statbuf.st_mode = (0666 | S_IFREG);
	statbuf.st_uid = -1;
	statbuf.st_gid = -1;
#ifdef ACLS
	statbuf.st_acl = 0;
#endif /* ACLS */
	if (master[0] != '\0')
	    src_file++;
    }
    else
    {				/* use the master file */
	if (STAT(master, &statbuf))
	{
	    sprintf(message, "makecdf: stat(%s)", master);
	    perror(message);
	    return 1;
	}
	else
#ifdef ACLS
	{
	    if (statbuf.st_acl)
		if ((nmacl = getacl(master, NACLENTRIES, macl)) < 0)
		{
		    sprintf(message, "makecdf: getacl(%s)", master);
		    perror(message);
		}
	    src_file++;
	}
#else /* no ACLS */
	    src_file++;
#endif /* ACLS */
    }

    /* create the CDF if it does not already exist */
    if (!cdf)
    {
	if (mkdir(file, 0700))
	{
	    sprintf(message, "makecdf: mkdir(%s)", file);
	    perror(message);
	    if (file_exists && rename(master, file))
	    {
		fprintf(stderr, "makecdf: WARNING: could not fixup %s,  original file left in %s\n", file,
			master);
	    }
	    return 1;
	}

	/*
	 * turn it into a CDF. Permissions/ownership are set later,
	 * after the elements have been created.
	 */
	if (chmod(file, (0700 | S_ISUID)))
	{
	    sprintf(message, "makecdf: chmod(%s)", file);
	    perror(message);
	    if (rmdir(file) || (file_exists && rename(master, file)))
		fprintf(stderr, "makecdf: WARNING: could not fixup %s,  original left in %s\n", file, master);
	    return 1;
	}
    }

    /*
     * we have the CDF, now copy the master file into it
     */
    if (src_file || cflag)
    {
	for (i = 0; i < num_c; i++)
	{
	    sprintf(newname, "%s/%s", file_plus, names[i]);
	    if (i == 0 && file_exists &&
		(ISREG(statbuf) || ISDIR(statbuf)))
	    {
		/*
		 * we need to rename the temp file to copy all
		 * the subdirectories to the first context
		 * element.
		 */
		if (rename(master, newname))
		{
		    sprintf(message,
			    "makecdf: couldn't rename directory(%s, %s)",
			    master, newname);
		    perror(message);
		    fprintf(stderr, "makecdf: original left in %s\n", master);
		    return 1;
		}
		/*
		 * We need to remember the new name of the master file
		 * (only if it is a file and we have more context
		 * elements).
		 */
		if (num_c > 0 && ISREG(statbuf))
		    strcpy(master, newname);
	    }
	    else
	    {
#ifdef CNODE_DEV
#ifdef ACLS
		retval = copy(master, newname, &statbuf, i, nmacl, macl);
#else /* CNODE_DEV and NO ACLS */
		retval = copy(master, newname, &statbuf, i);
#endif /* ACLS */
#else /* no CNODE_DEV */
#ifdef ACLS
		retval = copy(master, newname, &statbuf, nmacl, macl);
#else /* NO CNODE_DEV and NO ACLS */
		retval = copy(master, newname, &statbuf);
#endif /* ACLS */
#endif /* CNODE_DEV */
		/*
		 * Handle error conditions, we ignore an EEXIST error.
		 */
		if (retval && retval != EEXIST)
		{
		    if (i != 0)
		    {
			/*
			 * this is the Nth element, report but keep CDF
			 */
			sprintf(message,
			    "makecdf: WARNING: copy(%s, %s)",
			    file, master);
			perror(message);
			return 0;
		    }
		    else
		    {
			/*
			 * this is the 0th element, blow away the CDF
			 *
			 * NOTE: We never execute this code for files
			 * or directories.
			 */
			sprintf(message, "makecdf: copy(%s, %s)",
			    file, master);
			perror(message);
			sprintf(message, "/bin/rm -rf %s", file_plus);
			system(message);
			if (!eaccess(file_plus, F_OK))
			{	/* CDF still there */
			    fprintf(stderr,
				    "makecdf: could not remove %s, original left in %s\n",
				    file_plus, master);
			    return 1;
			}
			if (file_exists && rename(master, file))
			{
			    fprintf(stderr,
				    "makecdf: WARNING: could not fixup %s,  original left in %s\n",
				    file, master);
			    return 1;
			}
		    }
		}
	    }
	}
    }

    if (file_exists && !ISREG(statbuf) && !ISDIR(statbuf) &&
	unlink(master))
    {
	fprintf(stderr,
	    "makecdf: WARNING: couldn't remove temporary file %s\n",
	    master);
    }

    /*
     * finish up CDF by setting permissions and ownership to be that of
     * the parent directory.
     */
    if (!cdf)
    {
#ifdef ACLS
	if (parent_acl)
	{
	    /*
	     * If the parent has optional entries.
	     * The file_plus was just created with default ownership
	     * from mkdir.  We need to chmod to get the special bits
	     * set.  Then we need to chownacl the parent acl to
	     * translate ownership to the effective uid and gid so that
	     * the acl will be appropriate for a setacl on the
	     * file_plus.  Do a chown later to take care of the
	     * ownerships (this will translate the ACL again).
	     */
	    if (chmod(file_plus, (parent_mode & 07000 | S_ISUID)))
	    {
		sprintf(message,
		    "makecdf: WARNING: Could not chmod CDF %s",
		    file_plus);
		perror(message);
	    }
	    chownacl(npacl, pacl, parent_uid, parent_gid, myuid, mygid);
	    if (setacl(file_plus, npacl, pacl) < 0)
	    {
		sprintf(message,
		    "makecdf: WARNING: Could not setacl CDF %s",
		    file_plus);
		perror(message);
	    }
	}
	else
	{
	    /*
	     * if the parent has no optional entries we just need to
	     * chmod.
	     */
	    if (chmod(file_plus, (parent_mode | S_ISUID)))
	    {
		sprintf(message,
		    "makecdf: WARNING: Could not chmod CDF %s",
		    file_plus);
		perror(message);
	    }
	}
#else /* no ACLS */
	if (chmod(file_plus, (parent_mode | S_ISUID)))
	{
	    sprintf(message,
		"makecdf: WARNING: Could not chmod CDF %s",
		file_plus);
	    perror(message);
	}
#endif /* ACLS */
	if (chown(file_plus, parent_uid, parent_gid))
	{
	    sprintf(message,
		"makecdf: WARNING: Could not chown CDF %s",
		file_plus);
	    perror(message);
	}
    }
    return 0;
}

/*
 * copy the file (normal, directory, device file, etc) from "from"
 * to "to" preserving the mode, etc.
 */
#ifdef CNODE_DEV
#ifdef ACLS
copy(from, to, statbuf, nameindex, nfromacl, fromacl)
char    *from, *to;
struct stat *statbuf;
int	nameindex;		/* index into the name array */
int	nfromacl;		/* number entries in fromacl */
struct acl_entry *fromacl;	/* ACL of from file.  */
#else /* CNODE_DEV and NO ACLS */
copy(from, to, statbuf, nameindex)
char    *from, *to;
struct stat *statbuf;
int	nameindex;		/* index into the name array */
#endif /* ACLS */
#else /* no CNODE_DEV */
#ifdef ACLS
copy(from, to, statbuf, nfromacl, fromacl)
char    *from, *to;
struct stat *statbuf;
int	nfromacl;		/* number entries in fromacl */
struct acl_entry *fromacl;	/* ACL of from file.  */
#else /* NO CNODE_DEV and NO ACLS */
copy(from, to, statbuf)
char    *from, *to;
struct stat *statbuf;
#endif /* ACLS */
#endif /* CNODE_DEV */

{
    char buf[BUFSIZ];
    int cc;
#ifdef CNODE_DEV
    int cnode_id;
    struct cct_entry *ccentry;
#endif /* CNODE_DEV */

    errno = 0;
    switch (statbuf->st_mode & S_IFMT)
    {
    case S_IFCHR:
    case S_IFBLK:
    case S_IFIFO:
#ifdef CNODE_DEV
	/*
	 * get the entry from clusterconf and use the cnode ID to call
	 * mkrnod.  if the entry isn't found, copy the st_rcnode from
	 * the from file.
	 * set errno to 0 in either case since getccnam seems to set it.
	 */
	if ((ccentry = getccnam(names[nameindex])) == 0)
	{
	    errno = 0;
	    cnode_id = statbuf->st_rcnode;
	}
	else
	{
	    errno = 0;
	    cnode_id = ccentry->cnode_id;
	}
	if (mkrnod(to, statbuf->st_mode, statbuf->st_rdev, cnode_id))
	    return errno;
#else /* no CNODE_DEV */
	if (mknod(to, statbuf->st_mode, statbuf->st_rdev))
	    return errno;
#endif /* CNODE_DEV */
#ifdef ACLS
	/*
	 *  If there are optional entries, copy them to the new file.
	 */
	if (statbuf->st_acl)
	{
	    if (cpacl(from, to, statbuf->st_mode, statbuf->st_uid,
			statbuf->st_gid, myuid, mygid))
		break;
	}
#endif /* ACLS */
	else if (chmod(to, statbuf->st_mode))
	    break;
	if (chown(to, statbuf->st_uid, statbuf->st_gid))
	    break;
	break;
    case S_IFDIR:
	if (mkdir(to, statbuf->st_mode & 07000))
	    return errno;
#ifdef ACLS
	/*
	 * Can't use cpacl since the from file may not exist.
	 * Must copy the acl by hand with what has been passed in
	 * fromacl.
	 */
	else if (statbuf->st_acl && (nfromacl > 0))
	{
	    /*
	     * Translate the base ACL entries to this uid so that the
	     * ACL will be proper after the chown.
	     */
	    chownacl(nfromacl, fromacl,
		     statbuf->st_uid, statbuf->st_gid, myuid, mygid);
	    /*
	     * Put the ACL on the file.  Print a message and do a chmod
	     * if it fails.  This is not disasterous.
	     */
	    if (setacl(to, nfromacl, fromacl))
	    {
		sprintf(message, "makecdf: setacl(%s)", to);
		perror(message);
		if (chmod(to, statbuf->st_mode))
		    break;
	    }
	    if (chown(to, statbuf->st_uid, statbuf->st_gid))
		break;
	    break;
	}
	else if (chmod(to, statbuf->st_mode))
	    break;
#else /* no ACLS */
	else if (chmod(to, statbuf->st_mode))
	    break;
#endif /* ACLS */
	else if (chown(to, statbuf->st_uid, statbuf->st_gid))
	    break;
	break;
    case S_IFREG:
	if (errno = copyfile(from, to))
	    return errno;
#ifdef ACLS
	/*
	 *  If there are optional entries, copy them to the new file.
	 */
	if (statbuf->st_acl)
	{
	    if (cpacl(from, to, statbuf->st_mode, statbuf->st_uid,
			statbuf->st_gid, myuid, mygid))
		break;
	}
#endif /* ACLS */
	else if (chmod(to, statbuf->st_mode))
	    break;
	if (chown(to, statbuf->st_uid, statbuf->st_gid))
	    break;
	break;
    case S_IFLNK:
	if ((cc = readlink(from, buf, BUFSIZ)) == -1)
	    return errno;
	buf[cc] = 0;
	if (symlink(buf, to))
	    return errno;
	break;
    default:
	fprintf(stderr, "Can't do that type of file ... yet\n");
	return 1;
    }
    if (errno)
    {
	/* print a warning, though it's not catastrophic */
	sprintf(message, "makecdf: WARNING: chmod/chown %s", to);
	perror(message);
    }
    return 0;
}

copyfile(from, to)
char    *from, *to;
{
    int ffd, tfd;
    char buf[BUFSIZ];
    int chars;

    if (!eaccess(to, F_OK)) /* don't overwrite an existing context */
	return 0;
    if (equal(from, "-") || equal(from, ""))
    {
	ffd = 0;		/* stdin */
    }
    else if ((ffd = open(from, 0)) < 0)
    {
	fprintf(stderr, "makecdf: can't open %s\n", from);
	return errno;
    }
    if ((tfd = creat(to, 000)) < 0)
    {
	fprintf(stderr, "makecdf: cannot create %s\n", to);
	if (ffd)
	    close(ffd);
	return errno;
    }
    if (equal(from, "")) /* there is no data */
    {
	close(tfd);
	return 0;
    }

    while ((chars = read(ffd, buf, BUFSIZ)) != 0)
	if (chars < 0 || write(tfd, buf, chars) != chars)
	{
	    fprintf(stderr, "makecdf: bad copy from %s to %s\n",
		from, to);
	    if (ffd)
		close(ffd);
	    close(tfd);
	    unlink(to);
	    return errno;
	}

    close(ffd);
    if (close(tfd) != 0)
    {
	fprintf(stderr, "makecdf: cannot close %s\n", to);
	return errno;
    }
    if (equal(from, "-"))
    {
	/* don't try to read stdin twice, use the new 'to' file */
	strcpy(from, to);
    }
    return 0;
}

char *
dirname(file, parent)
unsigned char *file;
unsigned char *parent;
{
    unsigned char *ptr;
    int len;

    if ((ptr = nl_strrchr(file, '/')) == NULL)
	strcpy(parent, ".");
    else if (ptr == file)
	strcpy(parent, "/");
    else
    {
	len = ptr - file;
	strncpy(parent, file, len);
	parent[len] = NULL;
    }
    return (char *)parent;
}


make_temp_name(file, temp)
unsigned char *file;
unsigned char *temp;
{
    unsigned char *p;

    strcpy(temp, file);
    if ((p = nl_strrchr(temp, '/')) == NULL)
	p = temp;
    else
	p++;
    *p = '\0';
    strcat(temp, "cdfXXXXXX");
    if (mktemp(temp) == NULL)
    {
	fprintf(stderr, "makecdf: mktemp failed\n");
	exit(1);
    }
}

usage()
{
    fprintf(stderr,
	"usage: makecdf [[ -d default_name]|[ -f src_file ]] [[-c context] ... ] file ...\n");
    exit(1);
}

/*
 * toolong() --
 *   return TRUE if (the last component of) a given path name is too
 *   long for the filesystem that it is on.  The given path must
 *   exist.
 */
int
toolong(path)
char *path;
{
    char *basename;
    char *dirname = path;
    char *spot = "/";		/* pos of / between dirname/basename */
    int len;			/* length returned by pathconf() */
    DIR *dirp;			/* used if pathconf() fails */
    struct dirent *dp;		/* a directory entry */

    if (eaccess(path, F_OK) != 0)
	return -1;

    /*
     * Split path into its dirname and basename components.
     */
    if ((basename = (char *)nl_strrchr(path, '/')) == NULL)
    {
	basename = path;
	dirname = ".";
    }
    else
    {
	spot = basename;
	basename++;
	if (spot == path)
	    dirname = "/";
    }

    *spot = '\0';

    /*
     * Simple case.  If pathconf() works, we can just return the
     * comparison of what it says and the length of basename.
     */
    if ((len = pathconf(dirname, _PC_NAME_MAX)) != -1)
    {
	*spot = '/';
	return strlen(basename) > len;
    }

    /*
     * pathconf() failed (NFS) so we must do things the hard
     * way.
     */
    dirp = opendir(dirname);
    *spot = '/';

    if (dirp == NULL)
	return -1; /* dont know */

    while ((dp = readdir(dirp)) != (struct dirent *)0)
	if (strcmp(dp->d_name, basename) == 0)
	{
	    /*
	     * Found basename in the directory, it must not be too
	     * long.
	     */
	    closedir(dirp);
	    return FALSE;
	}

    /*
     * Didn't find 'basename' in the directory, yet it was accessible
     * so it must be too long.
     */
    closedir(dirp);
    return TRUE;
}

/*
 * The following routine is ifdef-ed in case getaccess() ever
 * goes away.
 */
#ifdef hpux
#include <sys/getaccess.h>

int
eaccess(path, mode)
char *path;
int mode;
{
    int val;

    if (mode == F_OK)
	return access(path, F_OK);

    val = getaccess(path, UID_EUID, NGROUPS_EGID_SUPP,
	      (int *)0, (void *)0, (void *)0);

    /*
     * If getaccess() failed, return -1 (errno is already set to
     * the approprate error by getaccess()).
     */
    if (val < 0)
	return -1;

    /*
     * If they have permission, just return 0
     */
    if (val & mode)
	return 0;

    /*
     * They don't have permission, set errno to EACCES and return -1
     */
    errno = EACCES;
    return -1;
}
#else /* not hpux, no getaccess() */

#include <sys/fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int
eaccess(path, mode)
char *path;
int mode;
{
    uid_t ruid, euid;
    gid_t rgid, egid;
    int ret;
    int save_errno;
    int bad_errno;

    if (mode == F_OK)
	return access(path, F_OK);

    save_errno = errno;
    ruid = getuid();
    euid = geteuid();
    rgid = getgid();
    egid = getegid();

    if (ruid != euid)
	(void)setresuid(euid, ruid, -1);

    if (rgid != egid)
	(void)setresuid(egid, rgid, -1);

    ret = access(path, mode);
    bad_errno = errno;

    if (ruid != euid)
	(void)setresuid(ruid, euid, -1);
    if (rgid != egid)
	(void)setresgid(rgid, egid, -1);

    /*
     * Restore errno if call succeeded, since setres[ui]id may haved
     * changed it.
     */
    if (ret == 0)
	errno = (ret == 0) ? save_errno : bad_errno;
    return ret;
}
#endif /* hpux vs. non-hpux */
