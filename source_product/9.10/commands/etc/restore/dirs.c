/* @(#)  $Revision: 66.4 $ */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "restore.h"
#ifdef	hpux
#  include "dumprestore.h"
#else	hpux
#  include <protocols/dumprestore.h>
#endif	hpux
#include <sys/file.h>

/*
 * Symbol table of directories read from tape.
 */

#if !defined(TRUX) && !defined(B1) 	/* B1 needs this to be global */
#define HASHSIZE	1000
#define INOHASH(val) (val % HASHSIZE)
struct inotab {
	struct inotab *t_next;
	ino_t	t_ino;
	daddr_t	t_seekpt;
	long t_size;
};
static struct inotab *inotab[HASHSIZE];
extern struct inotab *inotablookup();
extern struct inotab *allocinotab();
#else /* TRUX && B1 */
char  *plus;
#endif /* TRUX && B1 */


/*
 * Information retained about directories.
 */
struct modeinfo {
	ino_t ino;
	time_t timep[2];
	short mode;
	short uid;
	short gid;
};

/*
 * Global variables for this file.
 */
static daddr_t	seekpt;
static FILE	*df, *mf;
static DIR	*dirp;
static char	dirfile[32] = "#";	/* No file */
static char	modefile[32] = "#";	/* No file */
extern ino_t	search();
struct direct 	*rst_readdir();
extern void 	rst_seekdir();

/*
 * Format of old style directories.
 */
#define ODIRSIZ 14
struct odirect {
	u_short	d_ino;
	char	d_name[ODIRSIZ];
};

/*
 *	Extract directory contents, building up a directory structure
 *	on disk for extraction by name.
 *	If genmode is requested, save mode, owner, and times for all
 *	directories on the tape.
 */
extractdirs(genmode)
	int genmode;
{
	register int i, ts;
	register struct dinode *ip;
	struct inotab *itp;
	struct direct nulldir;
	int putdir(), null();

#if defined(TRUX) && defined(B1)
/* set the proclvl syslo. Will look for them at syslo */
	mand_copy_ir(mand_syslo, ir);
	if (setslabel(ir) < 0)
		panic("cannot change proclevel. Dir modes may not be set. \n");
#endif /* TRUX && B1 */

	vprintf(stdout, "Extract directories from tape\n");
	(void) sprintf(dirfile, "/tmp/rstdir%d", dumpdate);
	df = fopen(dirfile, "w");
	if (df == 0) {
		fprintf(stderr,
		    "restore: %s - cannot create directory temporary\n",
		    dirfile);
		perror("fopen");
		done(1);
	}
	if (genmode != 0) {
		(void) sprintf(modefile, "/tmp/rstmode%d", dumpdate);
		mf = fopen(modefile, "w");
		if (mf == 0) {
			fprintf(stderr,
			    "restore: %s - cannot create modefile \n",
			    modefile);
			perror("fopen");
			done(1);
		}
	}
	nulldir.d_ino = 0;
	nulldir.d_namlen = 1;
	(void) strcpy(nulldir.d_name, "/");
#ifdef	SHORT_NAMES_ONLY
	nulldir.d_reclen = sizeof(struct direct);
#else	SHORT_NAMES_ONLY
	nulldir.d_reclen = DIRSIZ(&nulldir);
#endif	SHORT_NAMES_ONLY
	for (;;) {
		curfile.name = "<directory file - name unknown>";
		curfile.action = USING;
		ip = curfile.dip;
		ts = curfile.ts;
		if (ts != TS_END && ts != TS_INODE
#if defined(TRUX) && defined(B1)
						&& ts != TS_SEC_INODE
#endif /* TRUX && B1 */
						) {
			getfile(null, null); /*Do the file extraction w/f1,&f2*/
			continue;
		}
#if defined(TRUX) && defined(B1)
		if (((ts == TS_INODE || ts == TS_SEC_INODE) && (ip->di_mode & IFMT) != IFDIR)
#else /* TRUX && B1 */
		if ((ts == TS_INODE && (ip->di_mode & IFMT) != IFDIR)
#endif /* TRUX && B1 */
		  || (ts == TS_END)
		   ) {
			(void) fclose(df);
			dirp = opendir(dirfile);
			if (dirp == NULL)
				perror("opendir");
			if (mf != NULL)
				(void) fclose(mf);
			i = dirlookup(".");
			if (i == 0)
				panic("Root directory is not on tape\n");
			return;
		}
		itp = allocinotab(curfile.ino, ip, seekpt);
		getfile(putdir, null);
		putent(&nulldir);   /* add a new directory entry to a file */
		flushent();         /* flush out a directory that is finished */
		itp->t_size = seekpt - itp->t_seekpt;
	}
}

/*
 * skip over all the directories on the tape
 */
skipdirs()
{

	while ((curfile.dip->di_mode & IFMT) == IFDIR) {
		skipfile();
	}
}

/*
 *	Recursively find names and inumbers of all files in subtree 
 *	pname and pass them off to be processed.
 */
treescan(pname, ino, todo)
	char *pname;
	ino_t ino;
	long (*todo)();
{
	register struct inotab *itp;
	register struct direct *dp;
	register struct entry *np;
	int namelen;
	daddr_t bpt;
	char locname[MAXPATHLEN + 1];

	itp = inotablookup(ino);
	if (itp == NULL) {
		/*
		 * Pname is name of a simple file or an unchanged directory.
		 */
		(void) (*todo)(pname, ino, LEAF);
		return;
	}
	/*
	 * Pname is a dumped directory name.
	 */
	if ((*todo)(pname, ino, NODE) == FAIL)
		return;
	/*
	 * begin search through the directory
	 * skipping over "." and ".."
	 */
	(void) strncpy(locname, pname, MAXPATHLEN);
	(void) strncat(locname, "/", MAXPATHLEN);
	namelen = strlen(locname);
	rst_seekdir(dirp, itp->t_seekpt, itp->t_seekpt);
	dp = rst_readdir(dirp); /* "." */
	if (dp != NULL && strcmp(dp->d_name, ".") == 0)
		dp = rst_readdir(dirp); /* ".." */
	else
		fprintf(stderr, "Warning: `.' missing from directory %s\n",
			pname);
	if (dp != NULL && strcmp(dp->d_name, "..") == 0)
		dp = rst_readdir(dirp); /* first real entry */
	else
		fprintf(stderr, "Warning: `..' missing from directory %s\n",
			pname);
	bpt = telldir(dirp);
	/*
	 * a zero inode signals end of directory
	 */
	while (dp != NULL && dp->d_ino != 0) {
		locname[namelen] = '\0';
		if (namelen + dp->d_namlen >= MAXPATHLEN) {
			fprintf(stderr, "%s%s: name exceeds %d char\n",
				locname, dp->d_name, MAXPATHLEN);
		} else {
			(void) strncat(locname, dp->d_name, (int)dp->d_namlen);
			treescan(locname, dp->d_ino, todo);
			rst_seekdir(dirp, bpt, itp->t_seekpt);
		}
		dp = rst_readdir(dirp);
		bpt = telldir(dirp);
	}
	if (dp == NULL)
		fprintf(stderr, "corrupted directory: %s.\n", locname);
}

/*
 * Search the directory tree rooted at inode ROOTINO
 * for the path pointed at by n
 */
ino_t
psearch(n)
	char	*n;
{
	register char *cp, *cp1;
	ino_t ino;
	char c;

	ino = ROOTINO;
	if (*(cp = n) == '/')
		cp++;
next:
	cp1 = cp + 1;
	while (*cp1 != '/' && *cp1)
		cp1++;
	c = *cp1;
	*cp1 = 0;
	ino = search(ino, cp);
	if (ino == 0) {
#if defined(TRUX) && defined(B1) 
/* maybe it's a cdf and it doesnot expect a cdf+ name so remove the last + */
/* This could be a problem only when combining cdf and mld? */
		if ((plus= strrchr(cp, '+'))!= 0 && (*(plus+1) == 0)){
			*plus = 0;
			ino = search(ino, cp);
			if (ino == 0) {
				*cp1 = c;
				return(0);
			}
		}
#else /* TRUX && B1 */
		*cp1 = c;
		return(0);
#endif /* TRUX && B1 */
	}
	*cp1 = c;
	if (c == '/') {
		cp = cp1+1;
		goto next;
	}
	return(ino);
}

/*
 * search the directory inode ino
 * looking for entry cp
 */
ino_t
search(inum, cp)
	ino_t	inum;
	char	*cp;
{
	register struct direct *dp;
	register struct inotab *itp;
	int len;

	itp = inotablookup(inum);
	if (itp == NULL)
		return(0);
	rst_seekdir(dirp, itp->t_seekpt, itp->t_seekpt);
	len = strlen(cp);
	do {
		dp = rst_readdir(dirp);
		if (dp == NULL || dp->d_ino == 0)
			return (0);
	} while (dp->d_namlen != len || strncmp(dp->d_name, cp, len) != 0);
	return(dp->d_ino);
}

/*
 * Put the directory entries in the directory file
 */
putdir(buf, size)
	char *buf;
	int size;
{
	struct direct cvtbuf;
	register struct odirect *odp;
	struct odirect *eodp;
	register struct direct *dp;
	long loc, i;
	extern int Bcvt;

	if (cvtflag) {
		eodp = (struct odirect *)&buf[size];
		for (odp = (struct odirect *)buf; odp < eodp; odp++)
			if (odp->d_ino != 0) {
				dcvt(odp, &cvtbuf);
				putent(&cvtbuf);
			}
	} else {
		for (loc = 0; loc < size; ) {
#if defined(TRUX) && defined(B1) && defined(B1DEBUG)
			msg_b1("putdir:ino=%d i=%d loc=%d name=%s\n",
			dp->d_ino, i, loc, dp->d_name);
#endif
			dp = (struct direct *)(buf + loc);
			if (Bcvt) {
				swabst("l2s", (char *) dp);
			}
			i = DIRBLKSIZ - (loc & (DIRBLKSIZ - 1));
			if (dp->d_reclen == 0 || dp->d_reclen > i) {
				loc += i;
				continue;
			}
			loc += dp->d_reclen;
			if (dp->d_ino != 0) {
				putent(dp);
			}
		}
	}
}

/*
 * These variables are "local" to the following two functions.
 */
char dirbuf[DIRBLKSIZ];
long dirloc = 0;
long prev = 0;

/*
 * add a new directory entry to a file.
 */
putent(dp)
	struct direct *dp;
{
#ifdef	SHORT_NAMES_ONLY
	dp->d_reclen = sizeof(struct direct);
#else	SHORT_NAMES_ONLY
	dp->d_reclen = DIRSIZ(dp);
#endif	SHORT_NAMES_ONLY
	if (dirloc + dp->d_reclen > DIRBLKSIZ) {
		((struct direct *)(dirbuf + prev))->d_reclen =
		    DIRBLKSIZ - prev;
		(void) fwrite(dirbuf, 1, DIRBLKSIZ, df);
		dirloc = 0;
	}
	bcopy((char *)dp, dirbuf + dirloc, (long)dp->d_reclen);
	prev = dirloc;
	dirloc += dp->d_reclen;
}

/*
 * flush out a directory that is finished.
 */
flushent()
{

	((struct direct *)(dirbuf + prev))->d_reclen = DIRBLKSIZ - prev;
	(void) fwrite(dirbuf, (int)dirloc, 1, df);
	seekpt = ftell(df);
	dirloc = 0;
}

dcvt(odp, ndp)
	register struct odirect *odp;
	register struct direct *ndp;
{

	bzero((char *)ndp, (long)(sizeof *ndp));
	ndp->d_ino =  odp->d_ino;
	(void) strncpy(ndp->d_name, odp->d_name, ODIRSIZ);
	ndp->d_namlen = strlen(ndp->d_name);
#ifdef	SHORT_NAMES_ONLY
	ndp->d_reclen = sizeof(struct direct);
#else	SHORT_NAMES_ONLY
	ndp->d_reclen = DIRSIZ(ndp);
#endif	SHORT_NAMES_ONLY
}

/*
 * Seek to an entry in a directory.
 * Only values returned by ``telldir'' should be passed to rst_seekdir.
 * This routine handles many directories in a single file.
 * It takes the base of the directory in the file, plus
 * the desired seek offset into it.
 */
void
rst_seekdir(dirp, loc, base)
	register DIR *dirp;
	daddr_t loc, base;
{

	if (loc == telldir(dirp))
		return;
	loc -= base;
	if (loc < 0)
		fprintf(stderr, "bad seek pointer to rst_seekdir %d/%d\n", 
			loc, base);
	(void) lseek(dirp->dd_fd, base + (loc & ~(DIRBLKSIZ - 1)), 0);
	dirp->dd_loc = loc & (DIRBLKSIZ - 1);
	if (dirp->dd_loc != 0)
		dirp->dd_size = read(dirp->dd_fd, dirp->dd_buf, DIRBLKSIZ);
}

/*
 * get next entry in a directory.
 */
struct direct *
rst_readdir(dirp)
	register DIR *dirp;
{
	register struct direct *dp;

	for (;;) {
		if (dirp->dd_loc == 0) {
			dirp->dd_size = read(dirp->dd_fd, dirp->dd_buf, 
			    DIRBLKSIZ);
			if (dirp->dd_size <= 0) {
				dprintf(stdout, "error reading directory\n");
				return NULL;
			}
		}
		if (dirp->dd_loc >= dirp->dd_size) {
			dirp->dd_loc = 0;
			continue;
		}
		dp = (struct direct *)(dirp->dd_buf + dirp->dd_loc);
		if (dp->d_reclen == 0 ||
		    dp->d_reclen > DIRBLKSIZ + 1 - dirp->dd_loc) {
			dprintf(stdout, "corrupted directory: bad reclen %d\n",
				dp->d_reclen);
			return NULL;
		}
		dirp->dd_loc += dp->d_reclen;
		if (dp->d_ino == 0 && strcmp(dp->d_name, "/") != 0)
			continue;
		if (dp->d_ino >= maxino) {
			dprintf(stdout, "corrupted directory: bad inum %d\n",
				dp->d_ino);
			continue;
		}
		return (dp);
	}
}

/*
 * Simulate the opening of a directory
 */
DIR *
rst_opendir(name)
	char *name;
{
	struct inotab *itp;
	ino_t ino;

	if ((ino = dirlookup(name)) > 0 &&
	    (itp = inotablookup(ino)) != NULL) {
		rst_seekdir(dirp, itp->t_seekpt, itp->t_seekpt);
		return (dirp);
	}
	return (0);
}

/*
 * Set the mode, owner, and times for all new or changed directories
 */
setdirmodes()
{
	FILE *mf; 
	struct modeinfo node;
	struct entry *ep;
	char *cp;
#if defined(DUX) || defined(DISKLESS)
        char tempbuffer[MAXNAMLEN+1];
#endif

#if defined(TRUX) && defined(B1)
/* set the proclvl to the level the files were created at i.e. syslo */
	mand_copy_ir(mand_syslo, ir);
	if (setslabel(ir) < 0)
		panic("cannot change proclevel. Dir modes may not be set. \n");
#endif /* TRUX && B1 */
	
	vprintf(stdout, "Set directory mode, owner, and times.\n");
	(void) sprintf(modefile, "/tmp/rstmode%d", dumpdate);
	mf = fopen(modefile, "r");
	if (mf == NULL) {
		perror("fopen");
		fprintf(stderr, "cannot open mode file %s\n", modefile);
		fprintf(stderr, "directory mode, owner, and times not set\n");
		return;
	}
	clearerr(mf);
	for (;;) {
		(void) fread((char *)&node, 1, sizeof(struct modeinfo), mf);
		if (feof(mf))
			break;
		ep = lookupino(node.ino);
		if (command == 'i' || command == 'x') {
			if (ep == NIL)
				continue;
			if (ep->e_flags & EXISTED) {
				ep->e_flags &= ~NEW;
				continue;
			}
			if (node.ino == ROOTINO &&
		   	    reply("set owner/mode for '.'") == FAIL)
				continue;
		}
		if (ep == NIL)
			panic("cannot find directory inode %d\n", node.ino);
		cp = myname(ep);
		if (! (ep->e_flags & EXISTED)) {
			(void) chown(cp, node.uid, node.gid);
			utime(cp, node.timep);
			(void) chmod(cp, node.mode);
		}
#if defined(DUX) || defined(DISKLESS)
		ep->e_CDF = C_UNKNOWN;
		(void)myname(ep);
#endif
		ep->e_flags &= ~NEW;
	}
	if (ferror(mf))
		panic("error setting directory modes\n");
	(void) fclose(mf);
}

/*
 * Generate a literal copy of a directory.
 */
genliteraldir(name, ino)
	char *name;
	ino_t ino;
{
	register struct inotab *itp;
	int ofile, dp, i, size;
	char buf[BUFSIZ];

	itp = inotablookup(ino);
	if (itp == NULL)
		panic("Cannot find directory inode %d named %s\n", ino, name);
	if ((ofile = creat(name, 0666)) < 0) {
		fprintf(stderr, "%s: ", name);
		(void) fflush(stderr);
		perror("cannot create file");
		return (FAIL);
	}
	rst_seekdir(dirp, itp->t_seekpt, itp->t_seekpt);
	dp = dup(dirp->dd_fd);
	for (i = itp->t_size; i > 0; i -= BUFSIZ) {
		size = i < BUFSIZ ? i : BUFSIZ;
		if (read(dp, buf, (int) size) == -1) {
			fprintf(stderr,
				"read error extracting inode %d, name %s\n",
				curfile.ino, curfile.name);
			perror("read");
			done(1);
		}
		if (write(ofile, buf, (int) size) == -1) {
			fprintf(stderr,
				"write error extracting inode %d, name %s\n",
				curfile.ino, curfile.name);
			perror("write");
			done(1);
		}
	}
	(void) close(dp);
	(void) close(ofile);
	return (GOOD);
}

/*
 * Determine the type of an inode
 */
inodetype(ino)
	ino_t ino;
{
	struct inotab *itp;

	itp = inotablookup(ino);
	if (itp == NULL)
		return (LEAF);
	return (NODE);
}

/*
 * Allocate and initialize a directory inode entry.
 * If requested, save its pertinent mode, owner, and time info.
 */
struct inotab *
allocinotab(ino, dip, seekpt)
	ino_t ino;
	struct dinode *dip;
	daddr_t seekpt;
{
#if defined(TRUX) && defined(B1)
	int i;
	struct sec_dinode *sdip = dip;
#endif
	register struct inotab	*itp;
	struct modeinfo node;

	itp = (struct inotab *)calloc(1, sizeof(struct inotab));
	if (itp == 0)
		panic("no memory directory table\n");
	itp->t_next = inotab[INOHASH(ino)];
	inotab[INOHASH(ino)] = itp;
	itp->t_ino = ino;
	itp->t_seekpt = seekpt;
#if defined(TRUX) && defined(B1)
	if (it_is_b1fs){
        	for (i=0; i< SEC_SPRIVVEC_SIZE; i++){
                	itp->secinfo.di_gpriv[i]= sdip->di_gpriv[i];
                	itp->secinfo.di_ppriv[i]= sdip->di_ppriv[i];
        	}
        	for (i=0; i< SEC_TAG_COUNT; i++)
                	itp->secinfo.di_tag[i]= sdip->di_tag[i];
        	itp->secinfo.di_type_flags = sdip->di_type_flags;
        	itp->secinfo.di_checksum = sdip->di_checksum; /* always = 0 */
	}
#endif /* TRUX && B1 */
	if (mf == NULL)
		return(itp);
	node.ino = ino;
	node.timep[0] = dip->di_atime;
	node.timep[1] = dip->di_mtime;
	node.mode = dip->di_mode;
	node.uid = dip->di_uid;
	node.gid = dip->di_gid;

	(void) fwrite((char *)&node, 1, sizeof(struct modeinfo), mf);
	return(itp);
}

/*
 * Look up an inode in the table of directories
 */
struct inotab *
inotablookup(ino)
	ino_t	ino;
{
	register struct inotab *itp;

	for (itp = inotab[INOHASH(ino)]; itp != NULL; itp = itp->t_next)
		if (itp->t_ino == ino)
			return(itp);
	return ((struct inotab *)0);
}

/*
 * Clean up and exit
 */
done(exitcode)
	int exitcode;
{

	closemt();
	if (modefile[0] != '#')
		(void) unlink(modefile);
	if (dirfile[0] != '#')
		(void) unlink(dirfile);
	exit(exitcode);
}

#if defined(TRUX) && defined(B1)


static int mand_tag = 1; 	/* hardcoded, change later */
static int acl_tag = 0;		/* hardcoded, change later */

sec_chmod(path, sec, exists)
        char *path;
        struct sec_attr sec;
	int exists; 	/* whether dir existed or not, if yes won't setlabels */
{
	char acl_buff[1024];
	tag_t *ditag;
	priv_t *dippriv, digpriv;
	int acl_size;
	obj_t obj;

    if (it_is_b1fs){
	obj.o_file = path;
	if (sec.di_type_flags == SEC_I_MLDCHILD)
		return; /* don't change the mld child. OS should've */

	if (sec.di_tag[acl_tag] != SEC_WILDCARD_TAG_VALUE){
		/* otherwise we'll get a NULL acl instead of wildcard */

	   if ((acl_size = acl_tag_to_ir(&(sec.di_tag[acl_tag]), acl_buff)) == -1){
		fprintf(stderr,"ERROR: Can not get an acl_ir from the tag, %s\n",path);
	   }else
		if (chacl (path,acl_buff, acl_size) == -1) 
			fprintf(stderr,"ERROR: Can not set acl on %s\n",path);
	}

	/* SET LABELS */	


	/* SET LABELS only on empty dir */	

	if (mand_tag_to_ir(&(sec.di_tag[mand_tag]), ir) == 0) {
	   if(sec.di_tag[mand_tag] == SEC_WILDCARD_TAG_VALUE)
                   fprintf(stderr,"ERROR: Directory label cannot be a WILDCARD\n");
                   fprintf(stderr, "ERROR: Cannot get ir from tag(%d) of %s. Wil l set to SYSHI\n",
                   sec.di_tag[mand_tag], path);
                   mand_copy_ir(mand_syshi, ir);
	}
	if (chslabel(path, ir) == -1){
		if (exists)
                      fprintf(stderr,"Warning: Can not change label on nonempty Dir.\n");
		else{
                      fprintf(stderr,"ERROR: Can not set label(tag=%d) on %s. Will set to SYSHI\n",
                      sec.di_tag[mand_tag], path);
                      mand_copy_ir(mand_syshi, ir);
                      if (chslabel(path, ir) == -1)
                         fprintf(stderr,"ERROR: Can not set %s to SYSHI\n",path);
		}
       }
    }
}
#endif /* TRUX && B1 */
