#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 72.2 $";
#endif

/*
 * mv file1 file2
 */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef NLS
#   define catgets(i, sn,mn,s) (s)
#else /* NLS */
#   define NL_SETN 1	/* set number */
#   include <nl_types.h>
#   include <nl_ctype.h>
#   include <langinfo.h>
#   include <msgbuf.h>
#   include <locale.h>
#endif /* NLS */

#ifndef NLS16
#   define CHARADV(p) (*(p)++)
#endif /* NLS16 */

#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <unistd.h>
#ifdef SHARED_LIBS
#include <sys/magic.h>
#endif /* SHARED_LIBS */

#define	DELIM	'/'
#define MODEBITS 07777

#ifdef ACLS
#define OPACL 	'+'	/* ACL entries on file:  print "+" past mode */
#define NOACL 	' '	/* NO entries on file:   print " "	     */
#endif

/************************************************************************
 * Forward declarations
 */
int	query();
char	*dname();
char	*check_link();
void	error();
void	Perror();
void	Perror2();
void	duplicate_attributes();


/************************************************************************
 * Global variables
 */
struct	stat s1, s2;
int	iflag = 0;	/* interactive mode */
int	fflag = 0;	/* force overwriting */

#ifdef NLS
nl_catd	nls_fd;
char	yesstr[3], nostr[3];
int	yesint, noint;		/* first character of NLS yes/no */
#endif


main(argc, argv)
int argc;
char *argv[];
{
    extern int optind, opterr;
    register int i, r;
    register char *arg;
    char *dest;
    char c;

#if defined(NLS) || defined(NLS16)
    /*
     * initialize to the correct locale
     */
    if (!setlocale(LC_ALL, ""))
    {
	fputs(_errlocale("mv"), stderr);
	putenv("LANG=");
	nls_fd = (nl_catd)-1;
    }
    else
    {
	char *s,*getenv();
	if (((s=getenv("LANG")) && *s) || ((s=getenv("NLSPATH")) && *s))
		nls_fd = catopen("mv", 0);
	else
		nls_fd = (nl_catd)-1;
    }
    {
	char *s;

	s = nl_langinfo(YESSTR);
	yesstr[0] = s[0];
	yesstr[1] = s[1];

	s = nl_langinfo(NOSTR);
	nostr[0] = s[0];
	nostr[1] = s[1];
    }

    yesint = CHARAT(yesstr);
    noint = CHARAT(nostr);
    if (!yesint || !noint)
    {
	yesint = 'y';
	noint = 'n';
    }

    yesstr[FIRSTof2(yesstr[0]) ? 2 : 1] = '\0';
    nostr[FIRSTof2(nostr[0]) ? 2 : 1] = '\0';
#endif /* NLS || NLS16 */

    opterr = 0;
    while ((c = getopt(argc, argv, "if")) != EOF)
	switch (c)
	{
	case 'i':
	    iflag = 1;
	    fflag = 0;
	    break;
	case 'f':
	    fflag = 1;
	    iflag = 0;
	    break;
	default:
	    goto usage;
	}

    if (argc - optind + 1 < 3)
    {
    usage:
	fprintf(stderr,
	    catgets(nls_fd, NL_SETN, 1, "Usage: mv [-f] [-i] f1 f2\n       mv [-f] [-i] f1 ... fn d1\n       mv [-f] [-i] d1 d2\n"));
	return 1;
    }

    /* Ensure that we are setting file modes to a known value
     * later
     */
    (void) umask(0);	

    dest = argv[argc - 1];
    if (stat(dest, &s2) >= 0 && S_ISDIR(s2.st_mode))
    {
	int sl;

	sl=strlen(dest);
	r = 0;
	for (i = optind; i < argc - 1; i++)
	    r |= movewithshortname(argv[i], dest, sl);
	return r;
    }

    if (argc - optind + 1 > 3)
	goto usage;

    return move(argv[optind], argv[optind + 1]);
}

int
movewithshortname(src, dest, strlen_dest)
char *src;
char *dest;
int strlen_dest;
{
    register char *shortname;
    char target[MAXPATHLEN + 1];

    shortname = dname(src);
    if (strlen_dest + strlen(shortname) > MAXPATHLEN - 1)
    {
	fprintf(stderr, "mv: %s/", dest);
	errno = ENAMETOOLONG;
	perror(shortname);
	return 1;
    }
    strcpy(target,dest);
    strcpy(target+strlen_dest,"/");
    strcpy(target+strlen_dest+1,shortname);
    return move(src, target);
}

move(source, target)
char *source;
char *target;
{
    static char noUnlink[] = "cannot unlink"; /* catgets 2 */
    int targetexists;

    if (check_sl(source) != 0)
	return 1;

    if (lstat(source, &s1) < 0)
    {
	Perror2(source, catgets(nls_fd, NL_SETN, 3, "cannot access"));
	return 1;
    }

    /*
     * First, try to rename source to destination.
     * The only reason we continue on failure is if
     * the move is on a nondirectory and not across
     * file systems.
     */
    targetexists = lstat(target, &s2) >= 0;
    if (targetexists)
    {
	if (s1.st_dev == s2.st_dev && s1.st_ino == s2.st_ino)
	{
	    error(
		catgets(nls_fd, NL_SETN, 4, "%s and %s are identical"),
		source, target);
	    return 1;
	}

	if (iflag &&
            query(stdout, catgets(nls_fd, NL_SETN, 5, "remove %s? "),
							  target) == 0)
	    return 1;

	errno = 0;
	if (access(target, W_OK) < 0)
	{
	    if (errno == ETXTBSY)
	    {
		/* cannot overwrite executing executable */
		Perror2(target, "cannot write");
		return 1;
	    }
	    else if (!fflag && isatty(fileno(stdin)))
	    {
#ifndef ACLS
		if (query(stderr, catgets(nls_fd, NL_SETN, 6, "override protection %o for %s? "),
			    s2.st_mode & MODEBITS, target) == 0)
#else /* ACLS */
		if (query(stderr, catgets(nls_fd, NL_SETN, 6, "override protection %o%c for %s? "),
			s2.st_mode & MODEBITS, s2.st_acl ? OPACL : NOACL, target) == 0)
#endif /* ACLS */
			return 1;
	    }
	}
    }

    if (rename(source, target) >= 0)
	return 0;

    if (errno != EXDEV)
    {
	Perror2((errno==EINVAL||(errno==ENOENT && targetexists))?source:target, "rename");
	return 1;
    }

    /* The next step for XPG4 is to verify that we are not attempting 
     * to move a file to an existing directory, or a directory to an 
     * existing file
     */
    if (targetexists && S_ISDIR(s2.st_mode) && !S_ISDIR(s1.st_mode))
    {
	error(catgets(nls_fd, NL_SETN, 12, "cannot move non-directory %s to existing directory %s"),source, target);
	return 1;
    }
    
    if (targetexists && !S_ISDIR(s2.st_mode) && S_ISDIR(s1.st_mode))
    {
	error(catgets(nls_fd, NL_SETN, 13, "cannot move directory %s to existing non-directory %s"),source, target);
	return 1;
    }
    
    /* Per XPG4, we now try and and unlink the target.
     */
    if (targetexists)
    {
	if (S_ISDIR(s2.st_mode))
	{
	    if (rmdir(target) < 0)
	    {
		Perror2(target, catgets(nls_fd, NL_SETN, 2, noUnlink));
		return 1;
	    }
	}
	else
	{
	    if (unlink(target) < 0)
	    {
		Perror2(target, catgets(nls_fd, NL_SETN, 2, noUnlink));
		return 1;
	    }
	}
    }
	    

    /* If the source is a directory hierachy, then move it recursively
     * otherwise recreate the file and unlink the source.
     */
    if (S_ISDIR(s1.st_mode) )
    {
	if (recreate_tree (source, target, &s1) != 0)
	    return 1;
	else
	    return unlink_tree (source);
    }
    else
    {
	if (recreate_file (source, target, &s1) != 0)
	    return 1;

	if (unlink (source) < 0)
	{
	    Perror2(source, catgets(nls_fd, NL_SETN, 2, noUnlink));
	    return 1;
	}
    }
    return 0;
}

recreate_file (source, target, s1)
  char	*source;
  char	*target;
  struct stat *s1;
{
    
    switch (s1->st_mode & S_IFMT)
    {
	
      /* Attempt to recreate a symbolic link */
      case S_IFLNK:
      {
	  register int m;
	  char symln[MAXPATHLEN + 1];

	  m = readlink(source, symln, sizeof (symln)-1);
	  if (m < 0)
	  {
	      Perror(source);
	      return 1;
	  }
	  symln[m] = '\0';

	  m = umask(~(s1->st_mode & MODEBITS));

          /* If root assume identity of file you are moving. */
	  /* If there is an error (not root), go ahead and   */
	  /* move file but the file will have ownership of   */ 
	  /* user moving the file.                           */

          /* create new symbollic link */
	  if (symlink(symln, target) < 0)
	  {
	      Perror(target);
	      return 1;
	  }
#ifdef ACLS
	  /*
	   * If the source file has optional ACL entries, get the ACL
	   * and set it on the target link.
	   */
	  if (s1->st_acl)
	  {
	      if (cpacl(source, target, s1->st_mode, s1->st_uid,
			s1->st_gid, geteuid(), getegid()) < 0)
	      {
		  Perror(target);
		  return 1;
	      }
	  }
#endif /* ACLS */
	  (void)umask(m);

      return 0;
      }


      /* Handle special files and pipes
       */
      case S_IFCHR:
      case S_IFBLK:
      case S_IFIFO:
      {
	  int rc;

	  if (S_ISFIFO(s1->st_mode))
	      rc = mknod(target, s1->st_mode, 0);
	  else
#ifdef CNODE_DEV
	      rc = mkrnod(target, s1->st_mode, s1->st_rdev, s1->st_rcnode);
#else
	      rc = mknod(target, s1->st_mode, s1->st_rdev);
#endif
	  if (rc < 0)
	  {
	      Perror2(target, catgets(nls_fd, NL_SETN, 18, "cannot create"));
	      return 1;
	  }
      }
      break;
      

      /* Recreate plain files. Keep a list of files which are
       * hardlinks within the hierarchy being copied and relink
       * them in the target hierarchy
       */
      case S_IFREG:
      {
	  register int fi;
	  register int fo;
	  register int n;
	  char *path;
	  char buf[MAXBSIZE];

	  if (check_sl(source) != 0)
	      return 1;

	  /* Check if this file was linked to one that we have
	   * already moved. If so, recreate the link as opposed to 
	   * copying the data
	   */
	  if ((path = check_link(s1)) != (char *)0)
	  {
	      if (link(path, target) == -1)
	      {
		  Perror(path);
		  return 1;
	      }
	      return 0;
	  }

	  /* If this file has hardlinks, then save some info about
	   * it so we can recreate the links later.
	   */
	  if (s1->st_nlink > 1)
	  {
	      (void) save_link(s1, target);
	  }


	  /* Attempt to recreate the file
	   */
	  fi = open(source, O_RDONLY);
	  if (fi < 0)
	  {
	      Perror2(source, catgets(nls_fd, NL_SETN, 19, "cannot read"));
	      return 1;
	  }

	  fo = creat(target, 0000);
	  if (fo < 0)
	  {
	      Perror(target);
	      close(fi);
	      return 1;
	  }

	  for (;;)
	  {
	      n = read(fi, buf, sizeof buf);
	      if (n == 0)
	      {
		  break;
	      }
	      else if (n < 0)
	      {
		  Perror2(source, catgets(nls_fd, NL_SETN, 19, "cannot read"));
		  close(fi);
		  close(fo);
		  return 1;
	      }
	      else if (write(fo, buf, n) != n)
	      {
		  Perror2(target, catgets(nls_fd,NL_SETN,17,"cannot write"));
		  close(fi);
		  close(fo);
		  return 1;
	      }
	  }

	  close(fi);
	  if (close(fo) != 0) {
 	     Perror2(target, catgets(nls_fd,NL_SETN,21,"cannot close"));
	     return 1;
	  }
      }
      break;
      

      /* The following file types are not supported for
       * this type of move.
       */
      case S_IFSOCK:
          error(catgets(nls_fd, NL_SETN, 9, "%s: can't move sockets across file systems"), source);
          return 1;

#if defined(RFA) || defined(OLD_RFA)
      case S_IFNWK:
 	  error(catgets(nls_fd, NL_SETN, 10, "%s: can't move network special files across file systems"), source);
	  return 1;
#endif /* RFA || OLD_RFA */

      default:
          error(catgets(nls_fd, NL_SETN, 8, "%s: unknown file type %o"),
		source, s1->st_mode);
          return 1;
    }

    /* Duplicate the 
     *  - file modification and access times.
     *  - user ID and group ID.
     *  - file mode.
     *
     * XPG4 says that if the user ID, group ID, or file mode can't be 
     * duplicated, then the file mode bits S_ISUID and S_ISGID shouldn't
     * be duplicated
     */

    duplicate_attributes(source, target, s1);
    return(0);
}

/* Duplicate the file attributes of the source. XPG4 says if this fails
 * it is not fatal, but we need to issue a diagnostic message
 */
void duplicate_attributes (source, target, s1)
  char  *source;
  char	*target;
  struct stat *s1;
{
    mode_t	mode;
    struct utimbuf tv;

    /* Set the access and modification times 
     */
    tv.actime = s1->st_atime;
    tv.modtime = s1->st_mtime;
    if (utime(target, &tv) < 0)
	Perror2(target, catgets(nls_fd, NL_SETN, 14, "cannot set file modification times"));

    /* Initially mask off the setuid/setgid bits. Later if we are
     * successful in changing the owner, then we'll try and reinstate 
     * them. This leaves the permissions in a reasonable state if 
     * we chown the file to someone else, then can't modify the permissions
     * because we don't own it anymore.
     */
    mode = s1->st_mode & ~(S_ISUID|S_ISGID);
    if (chmod(target, mode) < 0)
	Perror2(target, catgets(nls_fd, NL_SETN, 16, "cannot set file modes"));

    if (chown(target, s1->st_uid, s1->st_gid) < 0)
	Perror2(target, catgets(nls_fd, NL_SETN, 15, "cannot set file owner or group"));
    else
    {
	if (mode != s1->st_mode)
	    if (chmod(target, s1->st_mode) < 0)
		Perror2(target, catgets(nls_fd, NL_SETN, 16, "cannot set file modes"));
    }

#ifdef ACLS
    /*
     * If the source file has optional ACL entries, get them and
     * set them on the target file.
     */
    if (s1->st_acl)
    {
	if (cpacl(source, target, s1->st_mode, s1->st_uid,
		    s1->st_gid, s1->st_uid, s1->st_gid) < 0)
	{
	    Perror(target);
	}
    }
#endif /* ACLS */
}


/* Read the given directory and move each entry to the target location. If
 * another directory is encountered, then call ourself recursively
 */
recreate_tree(source, target, s1)
  char	*source;
  char	*target;
  struct stat *s1;
{
    DIR 	  *dirp;
    struct dirent *dp;
    struct stat   st;
    char   	  from[MAXPATHLEN+1];
    char	  to[MAXPATHLEN+1];

    /* First make a target directory that we can write into. Once 
     * all the files to be moved have been created, then we'll 
     * match up the permissions with the source directory.
     */
    if (mkdir (target, S_IRWXU | s1->st_mode) == -1)
    {
	Perror2(source, catgets(nls_fd, NL_SETN, 18, "cannot create"));
	return(1);
    }
    
    /* Now read the source directory. For each non-directory, build
     * a new source and target path and call recreate_file. For each
     * directory, recursively call ourself
     */
    if ((dirp = opendir (source)) == NULL)
    {
	Perror2(source, catgets(nls_fd, NL_SETN, 19, "cannot open"));
	return(1);
    }
    
    while ((dp = readdir(dirp)) != NULL)
    {
	if (dp->d_ino == 0)
	    continue;
	if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
	    continue;


	/* Ensure that the newly created names don't exceed the
	 * character arrays we intend to put them in.
	 */
	if (strlen(source)+dp->d_namlen+2 > sizeof(from))
	{
	    (void) closedir(dirp);
	    fprintf (stderr, catgets(nls_fd, NL_SETN, 20, "mv: %s/%s: Name too long."), source, dp->d_name);
	    return(1);
	}
	else
	    sprintf (from, "%s/%s", source, dp->d_name);
	
	if (strlen(target)+dp->d_namlen+2 > sizeof(to))
	{
	    (void) closedir(dirp);
	    fprintf (stderr, catgets(nls_fd, NL_SETN, 20, "mv: %s/%s: Name too long."), source, dp->d_name);
	    return(1);
	}
	else
	    sprintf (to, "%s/%s", target, dp->d_name);
	

	/* Determine the file type and recreate it appropriately
	 */
	if (lstat(from, &st) == -1)
	{
	    Perror(from);
	    return(1);
	}
	
	if (S_ISDIR(st.st_mode))
	{
	    if (recreate_tree(from, to, &st) != 0)
	    {
		(void) closedir(dirp);
		return(1);
	    }
	}
	
	else
	{
	    if (recreate_file(from, to, &st) != 0)
	    {
		(void) closedir(dirp);
		return(1);
	    }
	}
    }
    (void) closedir(dirp);
    duplicate_attributes (source, target, s1);
    return(0);
}

/* Use /bin/rm to remove the source directory hierachy
 */
unlink_tree(source)
  char	*source;
{
    switch(fork())
    {
      case 0:
	execl("/bin/rm", "rm", "-rf", source, (char *)0);

      case -1:
	error(catgets(nls_fd, NL_SETN, 2, "%s: Cannot unlink."), source);
	return(1);
	
      default:
	(void) wait((int *)0);
	if (access(source, F_OK) != -1 || errno != ENOENT)
	{
	    error(catgets(nls_fd, NL_SETN, 2, "%s: Cannot unlink."), source);
	    return(1);
	}
    }
    return(0);
}


/*VARARGS*/
int
query(stream, prompt, a1, a2, a3)
FILE *stream;
char *prompt;
char *a1;
char *a2;
char *a3;
{
    register int i;
    register int c;

    fprintf(stream, prompt, a1, a2, a3);
#ifdef NLS
    fprintf(stream, "(%s/%s) ", yesstr, nostr);
    if (FIRSTof2(i = getchar()))
	if (SECof2(c = getchar()))
	    i = (i << 8) | c;
    for (c = i; c != '\n' && c != EOF;)
	c = getchar();
    return i == yesint;
#else
    i = c = getchar();
    while (c != '\n' && c != EOF)
	c = getchar();
    return i == 'y';
#endif /* NLS */
}

char *
dname(name)
register char *name;
{
    register char *p = name;

#ifdef NLS
    if (_CS_SBYTE) {
#endif /* NLS */
    	while (*p)
		if (*(p)++ == DELIM && *p)
	    		name = p;
#ifdef NLS
    } else {
    	while (*p)
		if (CHARADV(p) == DELIM && *p)
	    		name = p;
    }
#endif /* NLS */
    return name;
}

/*VARARGS*/
void
error(fmt, a1, a2)
char *fmt;
char *a1;
char *a2;
{
    fputs("mv: ", stderr);
    fprintf(stderr, fmt, a1, a2);
    fputc('\n', stderr);
}

void
Perror(s)
char *s;
{
    fputs("mv: ", stderr);
    perror(s);
}

void
Perror2(s1, s2)
char *s1;
char *s2;
{
    fprintf(stderr, "mv: %s: ", s1);
    perror(s2);
}

check_sl (source)
  char	*source;
{

#ifdef SHARED_LIBS
    int fd;
    struct magic mymagic;

    if (   !fflag
	&& source[(fd=strlen(source))-1]=='l'
	&& source[fd-2]=='s'
	&& access(source,W_OK)== -1
	&& errno==ETXTBSY
	&& (fd=open(source,O_RDONLY)) != -1
	&& read(fd,&mymagic,sizeof(mymagic))==sizeof(mymagic)
	&& mymagic.file_type == SHL_MAGIC)
    {
	Perror2(source,catgets(nls_fd,NL_SETN,11,"cannot move \"text busy\" shared library"));
	return(1);
    }
#endif /* SHARED_LIBS */

    return 0;
}


typedef struct hardlink
{
   struct hardlink *next;   /* pointer to next inode structure */
   ino_t  old_inode;        /* inode of source file */
   dev_t  old_dev;	    /* device of the source file */
   char  *path;             /* path of associated target file */
};

static struct hardlink *first_inode=(struct hardlink *)0;

/* Check our saved list of inodes which have links. Return the
 * path to the first target created, or 0 if not found.
 */
char *
check_link(s1)
  struct stat *s1;
{
    char *i, *j;
    struct hardlink *inop;

    for (inop=first_inode; inop!=(struct hardlink *)0; inop=inop->next)
    {
	if (s1->st_ino == inop->old_inode && s1->st_rdev == inop->old_dev)
	{
	    return(inop->path);
	}
    }
    return ((char *)0);
}

/* Save this link if it is not already in the list.
 */
save_link(s1, path)
  struct stat *s1;
  char *path;
{
    char *i, *j;
    struct hardlink *inop;

    for (inop=first_inode; inop!=(struct hardlink *)0; inop=inop->next)
    {
	if (s1->st_ino == inop->old_inode && s1->st_rdev == inop->old_dev)
	{
	    return 0;
	}
    }
    inop=(struct hardlink *)malloc(sizeof(struct hardlink));
    inop->next=first_inode;
    first_inode=inop;
    inop->old_inode=s1->st_ino;
    inop->old_dev=s1->st_rdev;
    inop->path=(char *)malloc(MAXPATHLEN);
    for (j=inop->path ;*path!=(char *)0; j++)
	*j=*path++;
    return(0);
}

