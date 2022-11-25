/* @(#) $Revision: 37.1 $ */     
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                open.c                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "s500defs.h"
#include <sys/stat.h>
#include "sdf.h"			
#include <fcntl.h>
#include <errno.h>
#include <ustat.h>
#include <signal.h>

#define	ONE_K		1024
#define	SDF_FORMAT	0x700
#define	LIF_REGTYPE	0xea61
#define	MAXF_LENGTH	2147483647

#define	SDF_LOCK	"/tmp/SDF..LCK"

#define min(x, y) 	((x) < (y) ? (x) : (y))

extern struct cache cache[];

char *ctime(), *strcpy(), *strncpy(), *strchr(), *malloc();
long lseek();

struct dev_fd dev_fd[DEVFDMAX];
struct sdfinfo sdfinfo[SDFMAX];
extern int errno;

#ifdef	DEBUG
extern int debug;
#endif	DEBUG

/* -------------------------------------- */
/* sdfopen - open a file on an SDF device */
/* -------------------------------------- */

sdfopen(path, rdwr)		
register char *path;		
register int rdwr;
{
	register struct sdfinfo *sp;
	register struct dev_fd *dp;
	register struct filsys *f;
	register char *cp, *name;
	struct stat stbuf;
	struct ustat ustbuf;
	char devname[FNAMEMAX * 2], filename[FNAMEMAX * 2], buffer[ONE_K];
	static int first = 1;

#ifdef	DEBUG
	bugout("sdfopen(%s, (rdwr) %d)\n", path, rdwr);
#endif	DEBUG

	if (first) {
		for (sp = sdfinfo; sp < &sdfinfo[SDFMAX]; sp++)
			sp->opened = 0;
		for (dp = dev_fd; dp < &dev_fd[DEVFDMAX]; dp++)
			dp->count = 0;
		first = 0;
	}
	if (!sdfpath(path))
		return(-1);
	for (sp = sdfinfo; sp < &sdfinfo[SDFMAX]; sp++)
		if (!sp->opened)
			break;
	if (sp >= &sdfinfo[SDFMAX]) {
		errno = ENFILE;
		return(-1);
	}
	for (name = devname, cp = path; *cp != ':'; )
		*name++ = *cp++;
	*name = NULL;
	for (name = filename, cp++; *cp != NULL; )
		*name++ = *cp++;
	*name = NULL;
	sp->fd = opendev(sp, devname, (rdwr == WRITE ? O_RDWR : O_RDONLY));
	if (sp->fd < 0) {
		unlink(SDF_LOCK);
		return(-1);
	}
	lseek(sp->fd, SUPERB, 0);
	if (read(sp->fd, buffer, ONE_K) != ONE_K) {
		error("can't get super block for %s", devname);
		closedev(sp, sp->fd);
		return(-1);
	}
	f = &sp->filsys;
	bcopy((char *) f, buffer, FILSYSSIZ);
	if (f->s_format != SDF_FORMAT) {
		closedev(sp, sp->fd);
		error("%s: not an SDF device (0x%04x)", devname, f->s_format);
		return(-1);
	}
	if (f->s_corrupt != 0) {
		closedev(sp, sp->fd);
		error("%s: corrupt bit set in superblock", devname);
		return(-1);
	}
	sp->validi = FMAP_INUM+((f->s_maxblk+1+((8*FA_SIZ)-1))/(8*FA_SIZ)); 
	if (fstat(sp->fd, &stbuf) < 0) {
		closedev(sp, sp->fd);
		return(-1);
	}
	get(sp, f->s_fa * f->s_blksz, (char *) &sp->dp->fainode, FA_SIZ);
	sp->dev = stbuf.st_rdev;
	if (rdwr == WRITE && sp->dp->bfree < 0) {
		sdfustat(devname, &ustbuf);
		sp->dp->bfree = ustbuf.f_tfree;
	}
	sp->inumber = findino(sp, filename, rdwr);
	if (sp->inumber == -1) {
		closedev(sp, sp->fd);
		return(-1);
	}
	if (rdwr == WRITE && (sp->inode.di_mode & S_IFMT) == S_IFDIR) {
		errno = EISDIR;
		closedev(sp, sp->fd);
		return(-1);
	}
	sp->offset = 0;
	sp->written = 0;
	sp->opened++;
	if (rdwr == WRITE)
		if (trunc(sp, WRITE) < 0) {
			closedev(sp, sp->fd);
			return(-1);
		}
	return(sp - sdfinfo + SDF_OFFSET);
}

/* ----------------------------------------------- */
/* sdfclose - free SDF file descriptor table entry */
/* ----------------------------------------------- */

sdfclose(fd)
register int fd;
{
	register int ret;
	register struct sdfinfo *sp;

#ifdef	DEBUG
	bugout("sdfclose(%d)\n", fd);
#endif	DEBUG

	if (!sdffile(fd))
		return(close(fd));
	sp = &sdfinfo[fd - SDF_OFFSET];
	sp->opened--;
	ret = closedev(sp, sp->fd);
	return(ret);
}

/* -------------------------------------- */
/* opendev - set up internal device table */
/* -------------------------------------- */

opendev(sp, name, mode)
register struct sdfinfo *sp;
register char *name;
register int mode;
{
	register int fd;
	register struct dev_fd *p, *empty;
	int rmlock();
	int pid, ret;

#ifdef	DEBUG
	bugout("opendev(sdfinfo[%d], %s, (mode) %d)\n", sdffileno(sp), name,
	  mode);
#endif	DEBUG

	signal(SIGQUIT, rmlock);
	signal(SIGTERM, rmlock);
	signal(SIGPIPE, rmlock);
	signal(SIGINT, rmlock);
	signal(SIGHUP, rmlock);
	while (1) {
		if (access(SDF_LOCK, 0) < 0) {
			fd = open(SDF_LOCK, O_WRONLY|O_EXCL|O_CREAT, 0666);
			if (fd < 0)
				continue;
			pid = getpid();
			write(fd, (char *) &pid, sizeof(int));
			close(fd);
			break;
		}
		else {
			fd = open(SDF_LOCK, O_RDONLY);
			if (fd < 0)
				continue;
			ret = read(fd, (char *) &pid, sizeof(int));
			close(fd);
			if (ret == 0)
				continue;
			if (pid == getpid() || pid == getppid())
				break;
			if (kill(pid, 0) < 0 && errno == ESRCH) {
				unlink(SDF_LOCK);
				continue;
			}
			sleep(2);
		}
	}
	empty = NULL;
	for (p = dev_fd; p < &dev_fd[DEVFDMAX]; p++) {
		if (p->count && strncmp(name, p->devnm, FNAMEMAX) == 0) {
			p->count++;
			sp->dp = p;
			return(p->fd);
		}
		if (empty == NULL && p->count == 0)
			empty = p;
	}
	fd = open(name, O_RDWR);
	if (fd < 0 && mode != O_RDWR)
		fd = open(name, mode);
	if (fd < 0)
		return(-1);
	if (empty != NULL) {
		empty->fd = fd;
		empty->count = 1;
		strncpy(empty->devnm, name, sizeof(empty->devnm));
		empty->bfree = -1;
		sp->dp = empty;
	}
	return(fd);
}

/* --------------------------------------------- */
/* closedev - remove internal device table entry */
/* --------------------------------------------- */

closedev(sp, fd)
register struct sdfinfo *sp;
register int fd;
{
	extern struct cache cache[];
	register struct cache *chp;
	register struct dev_fd *p;

#ifdef	DEBUG
	bugout("closedev(sdfinfo[%d], %d)\n", sdffileno(sp), fd);
#endif	DEBUG

	for (p = dev_fd; p < &dev_fd[DEVFDMAX]; p++)
		if (p->count && p->fd == fd) {
			p->count--;
			if (p->count == 0) {
				for (chp = cache; chp < &cache[CACHEMAX]; chp++)
					if (chp->fd == fd && chp->valid) {
						chp->valid = 0;
						free(chp->block);
						chp->block = (char *) NULL;
						chp->atime = 0;
					}
				unlink(SDF_LOCK);
				return(close(fd));
			}
			else
				return(0);
		}
	return(close(fd));
}

/* ----------------------------------- */
/* findino - find inode of an SDF file */
/* ----------------------------------- */

findino(sp, path, rdwr)
register struct sdfinfo *sp;
register char *path;
register int rdwr;
{
	register int inum;
	register struct dinode *ip = &sp->inode;
	register char *cp;
	char fname[FNAMEMAX];

#ifdef	DEBUG
	bugout("findino(sdfinfo[%d], %s, (rdwr) %d)\n", sdffileno(sp), path,
	    rdwr);
#endif	DEBUG

	inum = getino(sp, ROOT_INUM, ip);
	if (inum != ROOT_INUM)
		fatal("can't get root inode");
	sp->inumber = sp->pinumber = inum;
	strcpy(fname, "/");
	for (;;) {
		while (*path == '/')
			path++;
		if (*path == NULL)
			return(inum);
		for (cp = fname; *path != NULL && *path != '/'; )
			*cp++ = *path++;
		*cp = NULL;
		if ((ip->di_mode & S_IFMT) != S_IFDIR) {
			errno = ENOENT;
			return(-1);
		}
		inum = getdirent(sp, ip, fname,
		    (rdwr == WRITE && *path == NULL));
		if (inum == -1) {
			errno = ENOENT;
			return(-1);
		}
		sp->pinumber = sp->inumber;
		sp->inumber = inum;
		getino(sp, inum, ip);
	}
}

/* ----------------------------------------------- */
/* getdirent - translate file name to inode number */
/* ----------------------------------------------- */

getdirent(sp, ip, fname, rdwr)
register struct sdfinfo *sp;
register struct dinode *ip;
register char *fname;
register int rdwr;
{
	register int offset, newblock, empty = -1;
	struct direct dbuf;
	struct dinode ibuf;
	void sdfdir2str();

#ifdef	DEBUG
	bugout("getdirent(sdfinfo[%d], i_buf(0x%08x), %s, (rdwr) %d)\n",
	    sdffileno(sp), ip, fname, rdwr);
#endif	DEBUG

	for (offset = 0; offset < ip->di_size; offset += D_SIZ) {
		inoread(sp, sp->inumber, &sp->inode, offset, (char *) &dbuf,
		  D_SIZ);
		if (dbuf.d_ino == 0) {
			if (empty < 0)
				empty = offset;
			continue;
		}
		sdfdir2str(dbuf.d_name);
		if (strncmp(fname, dbuf.d_name, DIRSIZ) == 0) {
			sp->diraddr = off2addr(sp, sp->inumber, &sp->inode,
			  offset, READ);
			return(dbuf.d_ino);	/* found it */
		}
	}
	if (rdwr == READ)
		return(-1);
	dbuf.d_ino = allocino(sp, sp->inumber + 1);
	if (dbuf.d_ino == 0)
		return(-1);
	getino(sp, dbuf.d_ino, &ibuf);
	initino(&ibuf);
	strncpy(dbuf.d_name, "                ", DIRSIZ + 2);
	strncpy(dbuf.d_name, fname, min(strlen(fname), DIRSIZ));
	newblock = allocblk(sp, 1);
	if (newblock == -1)
		return(-1);
	ibuf.di_extent[0].di_startblk = newblock;
	ibuf.di_extent[0].di_numblk = 1;
	ibuf.di_exnum = 1;
	ibuf.di_blksz = 1;
	if (empty < 0)
		offset = sp->inode.di_size;
	else
		offset = empty;
	sp->diraddr = off2addr(sp, sp->inumber, &sp->inode, offset, READ);
	dbuf.d_object_type = 0;
	dbuf.d_file_code = 0;
	if (inowrite(sp, sp->inumber, &sp->inode, offset, (char *) &dbuf,
	    D_SIZ) != D_SIZ)
		fatal("can't make directory entry for %s", fname);
	if (putino(sp, sp->inumber, &sp->inode) < 0)
		return(-1);
	if (putino(sp, dbuf.d_ino, &ibuf) < 0)
		return(-1);
	return(dbuf.d_ino);
}

/* ------------------------------ */
/* initino - initialize and inode */
/* ------------------------------ */

initino(ip)
register struct dinode *ip;
{
	register int i, mask;
	long now, time();

	now = time((long *) NULL);
	mask = umask(0);
	umask(mask);
	unix2ios(now, &ip->di_ctime);
	ip->di_type = R_INODE;
	ip->di_ftype = 0;
	ip->di_count = 1;
	ip->di_uftype = LIF_REGTYPE;
	ip->di_other = (unsigned) 0xffffffff;
	ip->di_protect = -1;
	ip->di_label = -1;
	ip->di_max = MAXF_LENGTH;
	ip->di_blksz = 0;
	ip->di_exsz = 1;
	ip->di_exnum = 0;
	for (i = 1; i < 4; i++) {
		ip->di_extent[i].di_startblk = -1;
		ip->di_extent[i].di_numblk = 0;
	}
	ip->di_exmap = -1;
	ip->di_size = 0;
	ip->di_parent = 0;
	strncpy(ip->di_name, "                ", DIRSIZ + 2);
	ip->di_atime = now;
	unix2ios(now, &ip->di_mtime);
	ip->di_recsz = 0;
	ip->di_uid = geteuid();
	ip->di_gid = getegid();
	ip->di_mode = S_IFREG | (0666 & ~mask);
	ip->di_res2[0] = 0;
	ip->di_res2[1] = 0;
	ip->di_dev = 0;
	return(0);
}

/* ---------------------------------------------------------- */
/* unix2ios - convert UNIX time to IOS time for creation time */
/* ---------------------------------------------------------- */

unix2ios(unixtime, iostime)
register long unixtime;
register time_ios *iostime;
{
	iostime->ti_date = unixtime / 86400 + 2440588;
	iostime->ti_time = unixtime % 86400;
}

/* --------------------------------------------------------- */
/* rmlock - remove device lock file upon receipt of a signal */
/* --------------------------------------------------------- */

/*ARGSUSED*/
rmlock(sig)
int sig;
{
	register struct sdfinfo *sp;
	register int fd;
	int pid;

	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	fd = open(SDF_LOCK, O_RDONLY);
	if (fd < 0)
		exit(-1);
	pid = -1;
	read(fd, (char *) &pid, sizeof(int));
	close(fd);
	if (pid == getpid())
		if (unlink(SDF_LOCK) < 0)
			error("can't remove lockfile %s!", SDF_LOCK);
	for (sp = sdfinfo; sp < &sdfinfo[SDFMAX]; sp++) {
		if (!sp->opened)
			continue;
		if (sp->written) {
			error("%s may have been corrupted; use sdffsck",
			    sp->dp->devnm);
		}
	}
	exit(-1);
}
