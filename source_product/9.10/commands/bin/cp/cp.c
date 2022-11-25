/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
    static char *HPUX_ID = "@(#) $Revision: 72.4 $";
#endif

/*
 * cp
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <ndir.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
static int errnosav;

#ifndef NLS
#   define catgets(i, sn,mn,s) (s)
#else
#   define NL_SETN 1
#   include <nl_types.h>
#   include <locale.h>
#   include <langinfo.h>
#endif

#ifdef NLS16
#   include <nl_ctype.h>
#endif

#define BLKSIZE 65536
#define DELIM '/'

int	iflag=0,fflag=0,pflag=0,rflag=0,Rflag=0;
struct stat s1, s2;

char	*strrchr();
uid_t	geteuid();
uid_t	getegid();

#ifdef NLS
nl_catd	nlmsg_fd;
char	yesstr[3], nostr[3];
int	yesint, noint;		/* first character of NLS yes/no */
#endif

typedef struct hardlink
{
   struct hardlink *next;   /* pointer to next inode structure */
   ino_t  old_inode;        /* inode of source file */
   char  *path;             /* path of associated target file */
};

/*
 * Yes this is ugly, but it has to be on one line for findmsg(1) to work
 * correctly!
 */
char USAGE[] =
/* catgets 1 */ "Usage:\tcp [-f|-i] [-p] source_file target_file\n\tcp [-f|-i] [-p] source_file ... target_directory\n\tcp [-f|-i] [-p] -R|-r source_directory ... target_directory\n";
char CANT_OPEN[]     = /* catgets 10 */ "cp: cannot open ";
char CANT_MKRNOD[]   = /* catgets 16 */ "cp: cannot mkrnod ";
char CANT_SYMLINK[]  = /* catgets 17 */ "cp: cannot symlink ";
char CANT_READLINK[] = /* catgets 18 */ "cp: cannot readlink ";

main(argc, argv)
	int argc;
	char **argv;
{
	struct stat stb;
	int rc, i;
	char c;
	extern char *optarg;
	extern int optind,opterr;	/* opterr=0 for no errors */
	int targetIsDir=0;              /* Assume target is NOT a directory */

#if defined(NLS) || defined(NLS16)	/* initialize to the right language */
	if (!setlocale(LC_ALL,"")) {
		fputs(_errlocale("cp"),stderr);
		nlmsg_fd = (nl_catd)-1;
	}
	else
		nlmsg_fd = catopen("cp",0);

	sprintf(yesstr, "%.2s", nl_langinfo(YESSTR));
	sprintf(nostr, "%.2s", nl_langinfo(NOSTR));
	yesint = CHARAT(yesstr);
	noint = CHARAT(nostr);
	if (!yesint || !noint) {
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
#endif /* NLS || NLS16 */

	/*
	 *   XPG4 draft 4 allows -R and -r to differ in how they handle
	 *   special files.  -R must recreate the file hierarchy and
	 *   thus needs to "copy" over the special files.  -r will be
	 *   left unchanged (even though the standard allow them to be
	 *   the same) to provide backward compatability.
	 */
	while ((c=getopt(argc,argv,"fiprR"))!=EOF)
		switch(c) {
			case 'i': iflag++; break;
			case 'f': fflag++; break;
			case 'p': pflag++; break;
			case 'R': Rflag++; break;
			case 'r': rflag++; break;
			default: goto usage;
		}
	if (iflag && fflag) iflag=0;	/* -f overrides -i option */
	if (argc-optind < 2)
		goto usage;
	/*
	 *   Are there more than two operands in the run string?
	 */
	if (argc-optind > 2) {
	    /*
	     *   YES, does the "target" exist?
	     */
	    if (stat(argv[argc-1], &stb) >= 0) {
	        /*
	         *   YES, is it a directory?  
	         */
	        if ((stb.st_mode&S_IFMT) != S_IFDIR) {
		    /*   NO, error   */
		    fprintf(stderr,catgets(nlmsg_fd, NL_SETN, 26,"cp: %s: not a directory\n"),argv[argc-1]);
		    exit(1);
	        }
	    }
	    else {
	        /* 
	         *  NO,  was -R or -r specified?
	         */
	        if (Rflag | rflag) {
	           /*  YES, then treat it as a directory */
	              targetIsDir=1;
	        }
	        else {
		    /*  NO, error  */
		/*
		 *  Back up for release 9.0.  The NLS work is already done.
		 *  Put in at 9.2.
		fprintf(stderr,catgets(nlmsg_fd, NL_SETN, 30,"cp: %s: does not exist. -R required.\n"),argv[argc-1]);
		    */
		    /*  NO, error  */
		    errnosav=errno;
		    fprintf(stderr,"cp: ");
		    errno=errnosav;
		    perror(argv[argc-1]);
		    exit(1);
	        }
	    }
	}
	/* remove extraneous trailing slashes from the last and */
	/* second to last arguments.                            */
	for (i=strlen(argv[argc-1])-1;argv[argc-1][i]=='/'&&i>0;i--)
		argv[argc-1][i]='\0';
	for (i=strlen(argv[argc-2])-1;argv[argc-2][i]=='/'&&i>0;i--)
		argv[argc-2][i]='\0';
	rc = 0;
	for (i = optind; i < argc-1; i++) {
		rc |= copy(argv[i], argv[argc-1], &targetIsDir);
	}
	exit(rc);
usage:
	fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 1, USAGE));
	exit(1);
}

char *dname();

/*
 *  The target does not already exist.  We need to
 *  create the target directory.  XPG4 requires S_IRWXU 
 *  always be set on a directory which is created.
 *
 *   if -p is NOT specified then the permissions for
 *   target are ((s1.st_mode modified by the file creation 
 *   mask) | S_IRWXU) of the user.
 *
 *   if -p is specified then the permissions for
 *   target are (s1.st_mode|S_IRWXU).
 *
 *  Create the target directory using the permissions
 *  from the source directory.  mkdir will modify the
 *  permission based on the users file creation mask.
 */
make_directory(target,source,ents)
	char * target, * source;
	int ents;
{
	mode_t mode;
	int errnosav;

	/*  
	 *  s1 is a global 
	 *    It is set up at the beginning of the copy routine.
	 *    Mkdir sets up the permission bits modified by the users umask.
	 */
	if ((mkdir(target, (int)s1.st_mode) < 0))
	{
		errnosav=errno;
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,9, "cp: cannot create %s: ")), target);
		errno=errnosav;
		perror("");
		return (1);
	}
	/*
	 *   XPG4 draft 4 requires the permissions be inclusive OR'ed with
	 *     S_IRWXU.
	 *
	 *   if -p specified then set the permissions to match the 
	 *      source permissions unmodified otherwise use the
	 *      permissions setup by mkdir.
	 */
	if (stat(target, &s2) < 0)
	{
	   if (errno != ENOENT)
	   {
		errnosav=errno;
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,21, "cp: cannot stat %s: ")), target);
		errno=errnosav;
		perror("");
		return (1);
	   }
	}
	/*
	 *  Fix the directory so we can write into it.
	 */
	mode=S_IRWXU|s2.st_mode;
	if (chmod(target,mode) < 0)
	{
		   errnosav=errno;
		   fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,14, "cp: cannot set the permission bits on %s: ")), target);
		   errno=errnosav;
		   perror("");
		   return (1);
	}

	if (s1.st_acl) {
		/* Optional ACL entries need to be copied */
		if (cpacl(source, target, s1.st_mode, s1.st_uid,
			 s1.st_gid, geteuid(), getegid()) < 0) {
		    	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,22, "cp: Warning: cannot cpacl to %s:\n")), target);
			if (chmod(target,s1.st_mode) < 0)
	    		{
		   		errnosav=errno;
		   		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,14, "cp: cannot set the permission bits on %s: ")), target);
		   		errno=errnosav;
		   		perror("");
		   		return (1);
	    		}
		}
	} 
}

/*
 *  linked will check to see if we have already seen the inode
 *  of the source.  If we have then it will return the
 *  path to the name of the first target which was associated 
 *  with this inode.  Otherwise, it will add the source inode 
 *  and target name to the list of known inodes and return zero.
 */
char *
linked(inode,path)
	ino_t inode;
	char *path;
{
	static struct hardlink *first_inode=(struct hardlink *)0;
	static struct hardlink *inode_list=(struct hardlink *)0;
	char *j;

	for (inode_list=first_inode;
	     inode_list!=(struct hardlink *)0;
	     inode_list=inode_list->next)
	{
	    if (inode == inode_list->old_inode)
	    {
		return(inode_list->path);
	    }
	}
	inode_list=(struct hardlink *)malloc(sizeof(struct hardlink));
	inode_list->next=first_inode;
	first_inode=inode_list;
	inode_list->old_inode=inode;
	inode_list->path=(char *)malloc(strlen(path) + 1);

	/* Copy the path to the inode list and terminate it with NULL. */
	for (j=inode_list->path ; *path != '\0'; j++)
	    *j=*path++;
	*j = '\0';

	return(0);

}

copy(source, target, targetIsDir)
	char *source, *target;
	int *targetIsDir;
{
	int c, rtnval;
	char buf[MAXPATHLEN + 1];
	char *last;
	int from, to, ct, oflg;
	static char fbuf[BLKSIZE];  /* static so we don't take up stack space */
	struct utimbuf	utbuf;
	int ents=0;  /* this is only used if Secureware or B1 is defined */
	char *path;

	if (((Rflag | rflag) ? lstat(source, &s1) : stat(source, &s1)) < 0) {
		errnosav=errno;
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "cp: cannot access %s: ")), source);
		errno=errnosav;
		perror("");
		return (1);
	}
	if ((s1.st_mode & S_IFMT) == S_IFSOCK) {
		fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 24, "cp: %s: Operation not supported on socket\n"), source);
		return (1);
	}
	/*
	 *    can not copy directories if one of the 
	 *    recursive flags were not specified.
	 */
	if (((s1.st_mode & S_IFMT) == S_IFDIR) && !(Rflag | rflag)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "cp: %s: is a directory.  Need \"-R\" option.\n")), source);
		return (1);
	}
	/*
	 *   Assume the target is a file.
	 */
	s2.st_mode = S_IFREG;
	/*
	 *   Does the target already exist??
	 *   If the target exists:
	 *     if target is a directory, then we assume the
	 *     source is a file and not a directory.  Thus,
	 *     the real target is target/basename(source).
	 */
	if (stat(target, &s2) >= 0) {
		/*
		 *   YES.  Is it a directroy?
		 */
		if ((s2.st_mode & S_IFMT) == S_IFDIR) {
			/*   I don't know why this line is here */
			if (stat(target, &s2) >= 0) {
				if (s1.st_dev==s2.st_dev && s1.st_ino==s2.st_ino) {
					fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "cp: %s and %s are identical\n")),
				    	source, target);
					return(1);
				}
			}
			/*  Construct the full path name for the target */
			last = dname(source);
			if (strlen(target) + strlen(last) >= MAXPATHLEN - 1) {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "cp: %s/%s: Name too long\n")),
				    target, last);
				return (1);
			}
			if ((*target == '/') && (strlen(target) == 1))
			   sprintf(buf, "%s%s", target, last);
			else
			   sprintf(buf, "%s/%s", target, last);
			target = buf;
		}
		/*  
		 *  Does the target exist?  
		 *    We check again because we may have just built the
		 *    path to a target file.
		 */
		if (stat(target, &s2) >= 0) {
			if (s1.st_dev==s2.st_dev && s1.st_ino==s2.st_ino) {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "cp: %s and %s are identical\n")),
				    source, target);
				return(1);
			}
		}
	}
	/*
	 *   Are we doing a recursive copy of a source directory?
	 */
	if ((Rflag|rflag) && ((s1.st_mode & S_IFMT) == S_IFDIR))
	{
		/*  YES, does the target exist?  */
		if (stat(target, &s2) == 0) {
			/*  YES. Is it a directory?  */
			if ((s2.st_mode & S_IFMT) != S_IFDIR) {
				/*  NO, error.  */
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,7, "cp: %s: Not a directory.\n")),
				    target);
				return (1);
			}
		}
		else 
		{
		   /*
		    *  The target does not already exist.  We need to
		    *  create the target directory.  
		    */
		    if (make_directory(target,source,ents) != 0)
		       return(1);
		}
		rtnval=rcopy(source, target);	/* copy directory contents */
	        /*
		 *   Do we need to fix up the ownership and access time?
		 */
		if (pflag) {
			/*
			 *   s1 is global and the call to copy changed it.
			 */
	                if (stat(source, &s1) < 0) {
		             errnosav=errno;
		             fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "cp: cannot access %s: ")), source);
		             errno=errnosav;
		             perror("");
		             return (1);
	                }
			/*
			 *  Save the directories permissions.  Chown may
			 *  destroy them.
			 */
	                if (stat(target, &s2) < 0) {
		             errnosav=errno;
		             fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "cp: cannot access %s: ")), source);
		             errno=errnosav;
		             perror("");
		             return (1);
	                }
		        utbuf.actime=s1.st_atime;
		        utbuf.modtime=s1.st_mtime;
		        if (utime(target,&utbuf) < 0)
			{
		             errnosav=errno;
		             fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,19, "cp: utime failed %s: ")), source);
		             errno=errnosav;
		             perror("");
		             return (1);
			}
			/*
			 *   Put the source permissions.
			 */
			if (chmod(target, s1.st_mode) < 0)
	        	{
		   	    errnosav=errno;
		   	    fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,14, "cp: cannot set the permission bits on %s: ")), target);
		   	    errno=errnosav;
		   	    perror("");
		   	    return (1);
	        	}
			/*
			 *   POSIX REQUIRES chown to strip the suid and guid 
			 *   bits.  If we own the file or we're root
			 *   these bit are not preserved by chown and we need
			 *   to chmod to set the bits correctly.
			 */
			 /*  Have to move chown() after chmod(), because
			     after chown to someone else, chmod() fails.
			     It is better to lose the suid and guid bits than
			     totally losing the modes.  EC.  3/11/93. 
			     Fix for dts #DSDe409193.
			  */
		        if ((chown(target,s1.st_uid,s1.st_gid) < 0) &&
		            (errno != EPERM))
			{
		             errnosav=errno;
		             fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,20, "cp: chown failed %s: ")), source);
		             errno=errnosav;
		             perror("");
		             return (1);
			}
	        }
		return(rtnval);
	}
	/*
	 *   The source was NOT a directory.
	 *   If there were multiple operands on the command line than the 
	 *   target is a directory.
	 */
	if ((Rflag|rflag) && (*targetIsDir))
	{
	    /*
	     *  Rflag, rflag, and targetIsDir are set in the main.
	     *  targetIsDir is set if there are more than 2 operands 
	     *  in the run string.  Once the directory at the top of
	     *  the directory tree is created we need to set targetIsDir
	     *  to "0" so each "target" at the same "level" in the source
	     *  tree as the target directory will be treated as the type
	     *  they really are.
	     */
	    *targetIsDir=0;
	    if (make_directory(target,source,ents) < 0)
		       return(1);
	    if (pflag) 
	    {
		/*
		 *   s1 is global and the call to copy changed it.
		 */
	        if (stat(source, &s1) < 0) {
	             errnosav=errno;
	             fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "cp: cannot access %s: ")), source);
	             errno=errnosav;
	             perror("");
	             return (1);
	         }
		/*
		 *  Save the directories permissions.  Chown may
		 *  destroy them.
		 */
	        if (stat(target, &s2) < 0) {
	             errnosav=errno;
	             fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "cp: cannot access %s: ")), source);
	             errno=errnosav;
	             perror("");
	             return (1);
	        }
	        utbuf.actime=s1.st_atime;
	        utbuf.modtime=s1.st_mtime;
	        if (utime(target,&utbuf) < 0)
		{
	             errnosav=errno;
	             fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,19, "cp: utime failed %s: ")), source);
	             errno=errnosav;
	             perror("");
	             return (1);
		}

		/*
		 *   POSIX REQUIRES chown to strip the suid and guid 
		 *   bits.  If we own the file or we're root
		 *   these bit are not preserved by chown and we need
		 *   to chmod to set the bits correctly.
		 */
	        if ((chown(target,s1.st_uid,s1.st_gid) < 0) &&
		    (errno != EPERM))
		{
	             errnosav=errno;
	             fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,20, "cp: chown failed %s: ")), source);
	             errno=errnosav;
	             perror("");
	             return (1);
		}
		/*
		 *   Put the permissions back.
		 */
		if (chmod(target, s2.st_mode) < 0)
	       	{
	   	    errnosav=errno;
	   	    fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,14, "cp: cannot set the permission bits on %s: ")), target);
	   	    errno=errnosav;
	   	    perror("");
	   	    return (1);
	       	}
	    }
	    /*  Construct the full path name for the target */
	    last = dname(source);
	    if (strlen(target) + strlen(last) >= MAXPATHLEN - 1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "cp: %s/%s: Name too long\n")),
		    target, last);
		return (1);
	    }
	    if ((*target == '/') && (strlen(target) == 1))
	       sprintf(buf, "%s%s", target, last);
	    else
	       sprintf(buf, "%s/%s", target, last);
	    target = buf;
	}
	/*
	 *   Copy over special files.
	 *
	 *   XPG4 draft 4 doesn't specify what should happen with special
	 *   files if the neither the -r nor the -R option is specified in 
	 *   the run string.  For backward compatability (and to let
	 *   "cp /dev/null /foo" do what is expected) only replicate the
	 *   special files if either "-r" or "-R" is specified.
	 */
	if (Rflag|rflag)
	{
           switch (s1.st_mode & S_IFMT) {
              case S_IFIFO:
              case S_IFCHR:
              case S_IFBLK:
	        if (mkrnod(target,s1.st_mode,s1.st_rdev,s1.st_cnode) < 0)
	        {
		    errnosav=errno;
		    fputs(catgets(nlmsg_fd,NL_SETN,16, CANT_MKRNOD), stderr);
		    errno=errnosav;
		    perror(source);
		    return(1);
	        }
	        return(0);
#ifdef SYMLINKS
              case S_IFLNK:
	      {
	        int pathlen;
	        char pathbuf[MAXPATHLEN+1];
   
	        if ((pathlen=readlink(source,pathbuf,MAXPATHLEN)) < 0)
	        {
		    errnosav=errno;
		    fputs(catgets(nlmsg_fd,NL_SETN,18, CANT_READLINK), stderr);
		    errno=errnosav;
		    perror(source);
		    return(1);
	        }
	        pathbuf[pathlen]='\0';
	        if (symlink(pathbuf,target) < 0)
	        {
		    errnosav=errno;
		    fputs(catgets(nlmsg_fd,NL_SETN,17, CANT_SYMLINK), stderr);
		    errno=errnosav;
		    perror(source);
		    return(1);
	        }
	        return(0);
              }
#endif
#ifdef S_IFSOCK
              case S_IFSOCK:
	        fprintf(stderr, catgets(nlmsg_fd, NL_SETN, 21, "warning: skipped socket: %s\n"), source);

	        return(0);
#endif
              default:
		break;
           }
        }
        /*
         *   It's a regular file.
         *   Is the source hard linked to another file?
         */
        if (s1.st_nlink > 1)
        {
   	   /*
   	    *   Have we already seen this inode number?
   	    *
   	    *  linked will check to see if we have already sen the inode
   	    *  of the source.  If we have then it will return 
   	    *  path to the name of the first target which was associated 
   	    *  with this inode.  Otherwise, it will add the source inode 
   	    *  and target name to the list of known inodes and return zero.
   	    */
   	   if ((int)(path=linked(s1.st_ino,target)) > 0) 
   	   {
   	       /*
   	        *   YES, make a hard link to the target
   	        */
   	       if (link(path,target) == -1)
   	       {
   	          errnosav=errno;
   	          fputs(catgets(nlmsg_fd,NL_SETN,24,"link failed: "),stderr);
   	          errno=errnosav;
   	          perror(source);
   	          return(1);
   	       }
   	       return(0);
   	   }
        }
        if ((from = open(source, 0)) < 0) {
   	 errnosav=errno;
   	 fputs(catgets(nlmsg_fd,NL_SETN,10, CANT_OPEN), stderr);
   	 errno=errnosav;
   	 perror(source);
   	 return(1);
        }
        if (fflag) {
   	   unlink(target);
        }
        /*
         *   oflg will be TRUE if target does NOT exist.
         */
        oflg = access(target, F_OK) == -1;
        if (iflag && !oflg && query(stdout,(catgets(nlmsg_fd, NL_SETN,25,"overwrite %s? ")), target) == 0) {
   	 close(from);
   	 return(0);
        }
        /*
         * Note that the mode is set to 0 so that there is no window that
         * leaves the file open.
         */
        if ((to = creat(target, 0000)) < 0) {
   	 errnosav=errno;
   	 fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,11, "cp: cannot create %s: ")), target);
   	 errno=errnosav;
   	 perror("");
   	 close(from);
   	 return(1);
        }
        while ((ct = read(from, fbuf, sizeof(fbuf))) != 0)
   	 if (ct < 0 || write(to, fbuf, ct) != ct) {
   	     errnosav=errno;
   	     fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,12, "cp: bad copy to %s: ")), target);
   	     errno=errnosav;
   	     perror(ct < 0 ? "read" : "write");
   	     if ((s2.st_mode & S_IFMT) == S_IFREG)
   	         if (unlink(target) != 0) {
   		     errnosav=errno;
   		     fprintf(stderr, catgets(nlmsg_fd,NL_SETN,2, "cp: couldn't unlink %s: "), target);
   		     errno=errnosav;
   		     perror("");
   	     }
   	 return(1);
        }
        close(from);
        if (close(to) != 0) {
            errnosav=errno;
            fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,13, "cp: cannot close %s: ")), target);
            errno=errnosav;
            perror("");
            return(1);
        }
	/*
	 *    If the -p option was specified, XPG4 draft 4 requires
	 *    the duplication of 
	 *     (1) the time of last modification and the time of last access.
	 *     (2) the user ID and group ID.
	 */
	 /*  Move step (2) to the end, after all the chmod is done, otherwise
	     chmod can fail if the source is not owned by caller. EC. 3/11/93
	     To fix defect #DSDe409193. */
	if (pflag) {
		utbuf.actime=s1.st_atime;
		utbuf.modtime=s1.st_mtime;
		if (utime(target,&utbuf) < 0)
		{
		     errnosav=errno;
		     fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,19, "cp: utime failed %s: ")), source);
		     errno=errnosav;
		     perror("");
		     return (1);
		}
	}
	/*
	 *   s1 is global and the call to copy changed it.
	 */
        if (stat(source, &s1) < 0) {
             errnosav=errno;
             fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "cp: cannot access %s: ")), source);
             errno=errnosav;
             perror("");
             return (1);
        }
	if (oflg || pflag) {
		if (s1.st_acl)
		{
			/*
			 * Source has optional ACL entries which need to be
			 * copied.  Cpacl could be used on files without
			 * optional entries, but is less efficient than chmod.
			 * If this fails print a warning and try to chmod.
			 */
			if (cpacl(source, target, s1.st_mode, s1.st_uid,
			    s1.st_gid, geteuid(), getegid()) < 0) {
			    	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,23, "cp: Warning: cannot cpacl to %s:\n")), target);
				if (chmod(target, s1.st_mode) < 0)
	    			{
		   			errnosav=errno;
		   			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,14, "cp: cannot set the permission bits on %s: ")), target);
		   			errno=errnosav;
		   			perror("");
		   			return (1);
	    			}
			}
		}
		else
		{
		   /*
	            * Source does not have optional ACL entries,
		    * so just chmod.
		    */
	           if (pflag)
		   {
		      if (chmod(target, s1.st_mode) < 0)
	    	      {
		   	      errnosav=errno;
		   	      fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,14, "cp: cannot set the permission bits on %s: ")), target);
		   	      errno=errnosav;
		   	      perror("");
		   	      return (1);
	    	      }
		   }
                   else
	           {
	              /*
	               *  XPG4  requires the target get created with 
		       *  the permissions of the source modified by 
		       *  the users file creation mask.  Since the file 
		       *  was created with a "0" mode in the create and 
		       *  since chmod doesn't use the umask value we get 
		       *  to do it.
		       *
		       *  For security reasons XPG4 says the suid and guid 
		       *  bits are to be removed if the -p option wasn't used.
	   	       */
	              mode_t mask;
	      
	              mask=umask(0);
	              (void)umask(mask);
	              /*
	               *  If we are root or we own the source file then we copy
	               *  the permission bits modified by the umask.  If we do 
		       *  not own the source file we must also strip the suid 
		       *  and guid bits.
	               */
	              if ((geteuid() != 0) && 
	                 ((geteuid() != s1.st_uid) || (getegid() != s1.st_gid)))
	                 mask=~mask & (s1.st_mode&~(S_ISUID|S_ISGID));
	              else
	                 mask=~mask & s1.st_mode;
	   	      if (chmod(target, mask) < 0)
	    	      {
		   	      errnosav=errno;
		   	      fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,14, "cp: cannot set the permission bits on %s: ")), target);
		   	      errno=errnosav;
		   	      perror("");
		   	      return (1);
	    	      }
		   }
		}
	}
	if (pflag) {
		if ((chown(target,s1.st_uid,s1.st_gid) < 0) &&
		    (errno != EPERM))
		{
	             errnosav=errno;
	             fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,20, "cp: chown failed %s: ")), source);
	             errno=errnosav;
	             perror("");
	             return (1);
		}
		/* Put back the suid, guid bits if necessary.  Don't bother
		   with error checking.  This is done, to preserve the suid,
		   guid bits from the source when -p is specified.  If we
		   don't own the source then, this will fail, but it is
		   up to the implementor to report an error or not.  In
		   this case we are not reporting */
		/* If we have acls, copy them, otherwise just copy the mode. */
		if (s1.st_acl)
			cpacl(source, target, s1.st_mode, s1.st_uid,
			    s1.st_gid, geteuid(), getegid());
		else
			chmod (target, s1.st_mode);
	}
	return 0;
}

rcopy(from, to)
	char *from, *to;
{
	DIR *fold = opendir(from);
	struct direct *dp;
	int errs = 0;
	char fromname[MAXPATHLEN + 1];

	if (fold == 0) {
		errnosav=errno;
		fputs(catgets(nlmsg_fd,NL_SETN,10, CANT_OPEN), stderr);
		errno=errnosav;
		perror(from);
		return (1);
	}
	for (;;) {
		dp = readdir(fold);
		if (dp == 0) {
			closedir(fold);
			return (errs);
		}
		if (dp->d_ino == 0)
			continue;
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (strlen(from)+1+strlen(dp->d_name) >= sizeof fromname - 1) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,15, "cp: %s/%s: Name too long.\n")),
			    from, dp->d_name);
			errs++;
			continue;
		}
		(void) sprintf(fromname, "%s/%s", from, dp->d_name);
		errs += copy(fromname, to, 0);
	}
}


char *
dname(name)
char *name;
{
	char *p;

	if ((p = strrchr(name, DELIM)) != NULL)
	    return p+1;
	return name;
}

#ifdef NLS16
#define CNULL (char *)0

char *
strrchr(sp, c)
register char *sp;
register unsigned int c;
{
	register char *r;

	if (sp == CNULL)
		return(CNULL);

	r = CNULL;
	do {
		if(CHARAT(sp) == c)
			r = sp;
	} while(CHARADV(sp));
	return(r);
}
#endif /* NLS16 */


query(stream, prompt, a1)
	FILE *stream;
	char *prompt, *a1;
{
	register int i, c;

	fprintf(stream, prompt, a1);
#ifdef NLS
	fprintf(stream, "(%s/%s) ", yesstr, nostr);
	if (FIRSTof2(i=getchar()))
		if (SECof2(c = getchar()))
			i = (i<<8) | c;
	for (c=i; c!='\n' && c != EOF; )
		c = getchar();
	return (i == yesint);
#else
	i = c = getchar();
	while (c != '\n' && c != EOF)
		c = getchar();
	return (i == 'y');
#endif /* NLS */
}
