
/* LINTLIBRARY */

#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "bif.h"
#include <signal.h>

#define CACHE 1			/* Should we do cacheing? */
#define NOWRITETHROUGH 1	/* Buffer cache is *not* write-through */

char *ctime(), *strcpy(), *strncpy(), *getenv();
long lseek();
long time();			/* a long time */
char *get_block();

#define OFFSET	 1000		/* differentiate bfs fd's from native fd's */
#define INFOSIZE 10		/* how many bfs files we can have */
#define eq(a,b) (strcmp((a),(b))==0)
#define max(a,b) (((a)>(b)) ? (a) : (b))
#define min(a,b) (((a)<(b)) ? (a) : (b))

#define readit (false)
#define writeit (true)

#define BIF_LOCK	"/tmp/BIF..LCK"

extern int errno;
extern boolean debug;
extern char *pname;

struct info {
	int fd;			/* native file descriptor of device	*/
	boolean opened;		/* currently active?			*/
	struct filsys filsys;	/* super-block for this device		*/
	int inode_number;	/* address of inode on disc		*/
	struct dinode inode;	/* inode for current file		*/
	int dir_addr;		/* directory entry that contains us	*/
	int fp;			/* offset within current file		*/
} info[INFOSIZE];


char zeroblock[BSIZE];		/* a block of zeros */

/*
 * bfsopen(path, writing)
 *
 * path:	device:path	(like /dev/hd:alpha/beta)
 *		or just path	(like gamma/zulu)
 *
 * writing:	boolean, true if opening for writing
 *
 * returns pseudo file descriptor:
 *
 *	-1:	error occured, look at errno
 *	<1000:	native system file descriptor	(for native file)
 *	>=1000:	index+1000 into info array	(for bfs file)
 */

bfsopen(path, writing)
register char *path;
register boolean writing;
{
	register struct info *dp;		/* pointer into info array */
	char device[100];			/* device name (/dev/hd) */
	register char *p;
	register int ret, mode;

	if(strcmp(path,"-") == 0)		/* talking to stdout ?? */
		return(1);
	/* Get device name */
	for (p=device; *path!=':' && *path!='\0'; )
		*p++ = *path++;
	*p = '\0';				/* terminate device */
	if (*path=='\0') {			/* if only a filename: */
		mode = writing ? O_WRONLY|O_CREAT|O_TRUNC : O_RDONLY;
		return open(device, mode, 0666);
	}

	path++;					/* skip colon */

	/* find a place to store this information */
	for (dp=info; dp<&info[INFOSIZE]; dp++)	/* look through info array */
		if (!dp->opened)		/* is this one unused? */
			break;			/* goot! */

	/* Did we find one or run out? */
	if (dp>=&info[INFOSIZE]) {		/* if no room left: */
		errno = ENFILE;			/* too many files */
		return -1;			/* return an error */
	}

	/* Open the device itself */
	dp->fd = open_dev(device, writing ? O_RDWR : O_RDONLY);
	if (dp->fd == -1)			/* if it failed: */
		return -1;			/* return in disgrace */

	/* get super-block into info structure */
	get(dp->fd, SUPERB*BSIZE, (char *) &dp->filsys, sizeof(dp->filsys));

	/* Find our file within the file system */
	ret = find_inode(dp, path, writing);
	if (ret==-1)
		return ret;

	/* opening a directory for writing? */
	if (writing && (dp->inode.di_mode & S_IFMT)==S_IFDIR) {
		errno = EISDIR;
		return -1;
	}

	dp->fp=0;				/* at start of file */

	dp->opened=true;			/* file is now opened */
	if (writing)
		truncate(dp);
	return OFFSET+(dp-info);		/* OFFSET marks it as bfs */
}


/*
 * find_inode(dp, path, writing)
 *
 * Find the inode on device fd corresponding to path.
 *
 * returns:
 *	-1 if error
 *	0 and info structure updated if found.
 */

find_inode(dp, path, writing)
register struct info *dp;
register char *path;
register boolean writing;
{
	register int fd = dp->fd;
	register int ii;
	register struct dinode *inode = &dp->inode;

	register char *p;
	char fname[20];

	dp->inode_number = 2;			/* root's i-number */
	get_ino(fd, 2, inode);			/* Get root inode */

	strcpy(fname, "/");

	for (;;) {
		while (*path=='/') path++;
		if (*path=='\0')
			return 0;

		/* Get next component into fname */
		for (p=fname; *path!=0 && *path!='/'; )
			*p++ = *path++;
		*p='\0';				/* terminate it */

		/* We'd better have a directory here */
		if ((inode->di_mode & S_IFMT)!=S_IFDIR) {
			errno = ENOENT;
			return -1;
		}

		/* Look through directory for file */
		ii = find_entry(dp, inode, fname, writing && *path=='\0');
		if (ii==-1) {
			errno = ENOENT;		/* wasn't in directory */
			return -1;		/* return in disgrace */
		}
		get_ino(fd, ii, inode);		/* down to next inode */
		dp->inode_number = ii;
	}
}


find_entry(dp, inode, fname, writing)
register struct info *dp;
register struct dinode *inode;
register char *fname;
register boolean writing;
{
	register int offset, empty_addr, i;
	struct direct direct;

	/* Look for it */
	empty_addr = 0;
	for (offset=0; offset<inode->di_size; offset+=sizeof(direct)) {
		register int addr;

		/* get the block */
		addr = offset_to_addr(offset, dp, readit);
		if (addr==0)			/* missing block */
			continue;		/* won't find it here */
		get(dp->fd, addr, (char *) &direct, sizeof(direct));

		if (direct.d_ino==0) {			 /* if empty slot: */
			if (empty_addr==0)		 /* if first empty: */
				empty_addr=direct.d_ino; /* remember it */
			continue;
		}

		if (strncmp(fname, direct.d_name, sizeof(direct.d_name))==0) {
			dp->dir_addr = addr;	/* remember where this is */
			return direct.d_ino;
		}
	}

	/* We couldn't find it, let's see if we have to extend the directory */
	if (writing && empty_addr==0) {
		empty_addr = offset_to_addr(dp->inode.di_size, dp, writeit);
		dp->inode.di_size += sizeof(direct);
		put_ino(dp->fd, dp->inode_number, &dp->inode);   /* write ino */
	}

	/* Use the hole we found or created at the end */
	if (writing && empty_addr!=0) {
		static struct dinode ino;	/* filled with zeros */
		register int ii;
		register long now;

		ii = alloc_ino(dp);
		if (ii==-1)
			return -1;

		ino.di_mode = S_IFREG | 0664;	/* regular file, default mode */
		ino.di_nlink = 1;
		ino.di_uid = geteuid();
		ino.di_gid = getegid();
		now = time((long *) 0);
		ino.di_atime = now;
		ino.di_mtime = now;
		ino.di_ctime = now;

		put_ino(dp->fd, ii, &ino);		/* write new inode */

		direct.d_ino = ii;			/* new i-number */
		for (i=0; i<sizeof(direct.d_name); i++)
			direct.d_name[i]='\0';		/* null out name */
		strncpy(direct.d_name, fname, sizeof(direct.d_name));
		put(dp->fd, empty_addr, (char *) &direct, sizeof(direct));
		dp->dir_addr = empty_addr;	/* remember where this is */
		return ii;
	}

	return -1;		/* failure */
}


bfsread(dp, buf, len)
register int dp;
register char *buf;
register int len;
{
	register int addr, rlen;
	register struct info *p;

	if (!bfsfile(dp))			/* if native file: */
		return read(dp, buf, len);	/* do a native read */
	p = &info[dp-OFFSET];
	if (len+p->fp > p->inode.di_size)	/* if want more than's there: */
		len = p->inode.di_size - p->fp;	/* truncate to maximum */

	/*
	 * We don't want to seek past the end of the file,
	 * so just return a zero length read right now.
	 */
	if (len==0)
		return 0;

	addr = offset_to_addr(p->fp, p, readit);
	if (addr==0) {				/* implied block */
		int i;
		for (i=0; i<0; i++)
			buf[i] = '\0';
		rlen = len;
	} else
		rlen = get(p->fd, addr, buf, len);
	if (rlen==-1)
		return rlen;
	if (rlen!=len)
		fatal("bfsread: wanted %d got %d", len, rlen);
	p->fp += rlen;				/* update that file pointer */
	return rlen;				/* return length read */
}


bfschmod(path, mode)
register char *path;
{
	register int fd;
	register struct info *dp;

	if (!bfspath(path)) {
		errno = ENODEV;
		return -1;
	}

	fd = bfsopen(path, 0);
	if (fd==-1)
		return -1;
	
	dp = &info[fd-OFFSET];		/* pointer to data structure */

	dp->inode.di_mode &= ~07777;		/* clear old bits */
	dp->inode.di_mode |= (mode & 07777);
	put_ino(dp->fd, dp->inode_number, &dp->inode);
	bfsclose(fd);
	return 0;
}

bfsutime(path, t)
register char *path;
register long *t;
{
	register int fd;
	register struct info *dp;

	if (!bfspath(path)) {
		errno = ENODEV;
		return -1;
	}

	fd = bfsopen(path, 0);
	if (fd==-1)
		return -1;
	
	dp = &info[fd-OFFSET];			/* pointer to data structure */

	if (t==NULL) {				/* if no times given: */
		register long now;

		now = time((char *) 0);		/* time right now */

		dp->inode.di_atime = now;	/* access time to now */
		dp->inode.di_mtime = now;	/* modification time to now */
	} else {
		dp->inode.di_atime = t[0];	/* set access time */
		dp->inode.di_mtime = t[1];	/* set modification time */
	}

	put_ino(dp->fd, dp->inode_number, &dp->inode);
	bfsclose(fd);
	return 0;
}


bfschown(path, owner, group)
register char *path;
{
	register int fd;
	register struct info *dp;

	if (!bfspath(path)) {
		errno = ENODEV;
		return -1;
	}

	fd = bfsopen(path, 0);
	if (fd==-1)
		return -1;
	
	dp = &info[fd-OFFSET];		/* pointer to data structure */

	dp->inode.di_uid = owner;
	dp->inode.di_gid = group;

	put_ino(dp->fd, dp->inode_number, &dp->inode);
	bfsclose(fd);
	return 0;
}


bfslink(oldpath, newpath)
register char *oldpath;
register char *newpath;
{
	register int oldfd, newfd;
	register struct info *olddp, *newdp;
	register struct dinode *ino, ti;
	struct direct direct;

/*	bugout("bfslink('%s','%s')", oldpath, newpath); */

	if (!bfspath(oldpath) || !bfspath(newpath)) {
		errno = ENODEV;
		return -1;
	}

	if (exist(newpath)) {
		errno = EEXIST;
		return -1;
	}

	newfd = bfsopen(newpath, 1);
	if (newfd==-1)
		return -1;
	newdp = &info[newfd-OFFSET];

	oldfd = bfsopen(oldpath, 0);
	if (oldfd==-1) {
		bfsclose(newfd);
		return -1;
	}
	olddp = &info[oldfd-OFFSET];

	/* different file systems? */
	if (olddp->fd != newdp->fd) {
		bfsclose(oldfd);
		bfsclose(newfd);
		errno = EXDEV;
		return -1;
	}

	/* increment the link count in the inode */
	ino = &olddp->inode;
	ino->di_nlink++;				/* one more link */
	put_ino(olddp->fd, olddp->inode_number, ino);	/* write it */

	/* change the new directory entry */
	get(newdp->fd, newdp->dir_addr, (char *) &direct, sizeof(direct));
	free_ino(newdp, direct.d_ino);		/* free old inode */
	direct.d_ino = olddp->inode_number;	/* replace the i-number */
	put(newdp->fd, newdp->dir_addr, (char *) &direct, sizeof(direct));

	bfsclose(oldfd);
	bfsclose(newfd);

	return 0;
}


bfsunlink(path)
register char *path;
{
	register int fd;
	register struct info *dp;
	static struct direct direct;

	if (!bfspath(path)) {
		errno = ENODEV;
		return -1;
	}

	fd = bfsopen(path, 0);
	if (fd==-1)
		return -1;
	
	dp = &info[fd-OFFSET];		/* pointer to data structure */

	if (dp->inode_number == 2) {		/* the root inode?? */
		bfsclose(fd);
		errno = EPERM;
		return -1;
	}

	/* write empty directory entry */
	put(dp->fd, dp->dir_addr, (char *) &direct, sizeof(direct));

	dp->inode.di_nlink--;			/* one less link to the data */
	if (dp->inode.di_nlink==0) {		/* no links left? */
		truncate(dp);			/* get rid of data */
		free_ino(dp, dp->inode_number);  /* free that inode */
	} else
		put_ino(dp->fd, dp->inode_number, &dp->inode);

	bfsclose(fd);
	return 0;
}

bfsmknod(path, mode, dev)
register char *path;
register int mode, dev;
{
	register int fd;
	register struct info *dp;

	if (!bfspath(path)) {
		errno = ENODEV;
		return -1;
	}

	if (exist(path)) {			/* Does it already exist? */
		errno = EEXIST;			/* we don't like that */
		return -1;			/* tell him so */
	}

	/* Create it */
	fd = bfsopen(path, 1);			/* should work */
	if (fd==-1)				/* didn't work (no space?) */
		return -1;
	dp = &info[fd-OFFSET];			/* pointer to data structure */

	dp->inode.di_mode = mode;		/* mode information */
	dp->inode.di_rdev = dev;		/* device information */
	put_ino(dp->fd, dp->inode_number, &dp->inode);

	bfsclose(fd);
	return 0;
}





/*
 * truncate(dp)
 *
 * Truncate the file to zero size.
 */
truncate(dp)
register struct info *dp;
{
#define p256(n) (1 << ((n)*8))
#define triple(p) (((p)[0]<<16) + ((p)[1]<<8) + (p)[2])

	register struct dinode *ino = &dp->inode;
	register int i, fsize, bnum;

	fsize = (ino->di_size+BSIZE-1) >> BSHIFT;     /* block count round up */

	/* Free direct blocks */
	for (bnum=0; bnum<=9 && fsize>0; bnum++, fsize--)
		handle(dp, triple((unsigned char *) &ino->di_addr[bnum*3]),
			0, 1);

	/* Handle indirect blocks */
	for (i=1; i<=3 && fsize>0; i++) {
		bnum = triple((unsigned char *) &ino->di_addr[(9+i)*3]);
		handle(dp, bnum, i, min(fsize, p256(i)));
		fsize -= p256(i);
	}

	if (fsize>0)
		fatal("truncate: fsize %d still positive?", fsize);

	/* Clear out the pointers themselves */
	for (i=0; i<13*3; i++)
		ino->di_addr[i]=0;

	ino->di_size = 0;

	put_ino(dp->fd, dp->inode_number, ino);
}


handle(dp, bnum, depth, count)
register struct info *dp;
register int bnum;		/* the block of interest */
register int depth;
register int count;		/* how many blocks hanging off this one */
{
	if (bnum==0)				/* no block at all */
		return;				/* nothing to do */

	if (depth>0) {
		int *bp, his_size;

		bp = (int *) get_block(dp->fd, bnum);

		for (; count>0; count-=his_size) {
			his_size = min(p256(depth-1), count);
			handle(dp, *bp++, depth-1, his_size);
		}
	}
	free_block(dp, bnum);			/* free the block itself */
}



clear_block(bp)
register char *bp;
{
	register long *start, *finish;

	start = (long *) bp;
	finish = (long *) (bp+BSIZE);

	while (start<finish)
		*start++ = 0;
}


alloc_block(dp)
register struct info *dp;
{
	register struct filsys *fs = &dp->filsys;
	register int bnum, i, *bp;

	if (fs->s_tfree==0)			/* nothing left */
		return 0;			/* error */

	fs->s_tfree--;				/* one less total free */
	bnum = fs->s_free[--fs->s_nfree];
	if (bnum==0)
		fatal("alloc_block: bnum=%d s_tfree=%d?", bnum, fs->s_tfree);

	if (fs->s_nfree==0) {
		bp = (int *) get_block(dp->fd, bnum);
		fs->s_nfree = bp[0];
		for (i=0; i<NICFREE; i++)
			fs->s_free[i] = bp[i+1];
	}

	put(dp->fd, SUPERB*BSIZE, (char *)fs, sizeof(*fs));

	return bnum;
}



free_block(dp, block_number)
register struct info *dp;
register block_number;
{
	register struct filsys *fs = &dp->filsys;


	if (fs->s_nfree==NICFREE) {		/* if free list cache full: */
		int block[BSIZE/4], i;

		block[0] = fs->s_nfree;
		for (i=0; i<NICFREE; i++)
			block[i+1] = fs->s_free[i];
		put_block(dp->fd, block_number, (char *) block);
	}
	fs->s_free[fs->s_nfree++] = block_number;

	fs->s_tfree++;

	put(dp->fd, SUPERB*BSIZE, (char *)fs, sizeof(*fs));
}



alloc_ino(dp)
register struct info *dp;
{
	register int n;

	struct filsys *fs = &dp->filsys;

	if (fs->s_ninode==0) {
		struct dinode ino;
		
		/* Look through the i-node list for free i-nodes */
		for (n=ROOTINO+1; n<=(fs->s_isize-2)*INOPB
				  && fs->s_ninode<NICINOD; n++) {
			get_ino(dp->fd, n, &ino);
			if (ino.di_mode==0)		/* if free: */
				fs->s_inode[fs->s_ninode++] = n;
		}
	}

	if (fs->s_ninode==0) {
		/* Didn't change the super block, don't have to write it. */
		return -1;
	}

	n = fs->s_inode[-- fs->s_ninode];
	fs->s_tinode--;

	put(dp->fd, SUPERB*BSIZE, (char *)fs, sizeof(*fs));

	return n;
}




free_ino(dp, inumber)
register struct info *dp;
register int inumber;
{
	register struct filsys *fs = &dp->filsys;  /* pointer to super-block */
	static struct dinode ino;	   	   /* an unallocated i-node */

	put_ino(dp->fd, inumber, &ino);		/* write out zero i-node */

	if (fs->s_ninode < NICINOD)			/* if space in cache */
		fs->s_inode[fs->s_ninode++] = inumber;	/* enter i-number */
	fs->s_tinode++;					/* one more free */
	put(dp->fd, SUPERB*BSIZE, (char *)fs, sizeof(*fs));
}



/*
 * offset_to_addr(off, dp, writing)
 *
 * given offset <off> in info dp
 * return the disk address of that offset.
 */
offset_to_addr(off, dp, writing)
register int off;
register struct info *dp;
register boolean writing;
{

	register unsigned char *addrs;
	register int bnum, addr, depth;
	register struct dinode *ino = &dp->inode;

	bnum = off/BSIZE;			/* block number */

	if (bnum<0)
		fatal("Can't read to block %d!", bnum);

	if (bnum<10)
		depth=0;
	else if ((bnum-=10)<256)
		depth=1;
	else if ((bnum-=256)<256*256)
		depth=2;
	else if ((bnum-=256*256)<256*256*256)
		depth=3;
	else
		fatal("Indirection too much");

	/* get address first block */
	addrs = (unsigned char *) &ino->di_addr[(depth==0 ? bnum : 9+depth)*3];
	addr = triple(addrs);
	if (addr==0) {
		if (writing) {
			addr = alloc_block(dp);
			put_block(dp->fd, addr, zeroblock);
			addrs[0] = (addr>>16) & 0xff;
			addrs[1] = (addr>>8) & 0xff;
			addrs[2] = (addr>>0) & 0xff;
			put_ino(dp->fd, dp->inode_number, ino);
		} else {
			return 0;
		}
	}

	while (depth>0) {
		int newaddr;
		int *bp;

		bp = (int *) get_block(dp->fd, addr);
		depth--;
		newaddr = bp[(bnum>>(depth*8)) & 0xff];
		if (newaddr==0) {
			if (writing) {
				newaddr = alloc_block(dp);
				put_block(dp->fd, newaddr, zeroblock);
				bp[(bnum>>(depth*8)) & 0xff] = newaddr;
				put_block(dp->fd, addr, (char *) bp);
			} else {
				return 0;
			}
		}
		addr = newaddr;
	}

	return addr*BSIZE			/* addr of block */
	       + (off & BMASK);			/* offset in block */
}



bfswrite(dp, buf, len)
register int dp;
register char *buf;
register len;
{
	register int addr, rlen;
	register struct info *p;

	if (!bfsfile(dp))			/* if native file: */
		return write(dp, buf, len);	/* do a native write */

 	p = &info[dp-OFFSET];
	addr = offset_to_addr(p->fp, p, writeit);
	rlen = put(p->fd, addr, buf, len);
	if (rlen==-1)
		return rlen;
	if (rlen!=len)
		fatal("bfswrite: wanted %d got %d", len, rlen);
	p->fp += rlen;				/* update that file pointer */
	if (p->fp > p->inode.di_size) {		/* if want more than's there: */
		p->inode.di_size = p->fp;	/* change file size */
		put_ino(p->fd, p->inode_number, &p->inode);
	}

	return rlen;				/* return length read */
}


bfsclose(dp)
register int dp;
{
	if (!bfsfile(dp))
		return close(dp);

	dp -= OFFSET;

	info[dp].opened = false;
	return close_dev(info[dp].fd);
}


#ifdef CACHE
struct dev_fd {
	char dev_name[50];
	int fd;
	int count;
} dev_fd[20];
#endif

int rmlock();

open_dev(name, mode)
register char *name;
register int mode;
{
#ifdef CACHE
	register struct dev_fd *p, *empty;
#endif
	register int fd;
	register int ret;
	int pid;

	signal(SIGQUIT, rmlock);
	signal(SIGTERM, rmlock);
	signal(SIGPIPE, rmlock);
	signal(SIGINT, rmlock);
	signal(SIGHUP, rmlock);
	while(1) 
	{
		if(access(BIF_LOCK,0) < 0)  /* lock file doesn't exist */
		{
		    fd = open(BIF_LOCK, O_WRONLY|O_EXCL|O_CREAT, 0666);
		    if(fd < 0)
			continue;
		    pid = getpid();
		    write(fd, (char *) &pid, sizeof(int));
		    close(fd);
		    break;
		}
		else				/* lock file does exist */
		{
		    fd = open(BIF_LOCK, O_RDONLY);
		    if(fd < 0)
			continue;
		    ret = read(fd, (char *) &pid, sizeof(int));
		    close(fd);
		    if(ret == 0)
			continue;
		    if(pid == getpid() || pid == getppid() )
			break;
		    if( kill(pid,0) < 0 && errno == ESRCH)
		    {
			unlink(BIF_LOCK);
			continue;
		    }
		    sleep(2);	/* wait for things to settle ? */
		}
	}	/* end while lock loop */

	/* Substitute environment variable for null name */
	if (name[0]=='\0') {			/* if no name given */
		name = getenv("BIFDEVICE");	/* substitute env variable */
		if (name==NULL)			/* if no environment variable */
			name="";		/* return to original state */
	}

#ifdef CACHE
	empty = NULL;					/* no empty spot yet */

	/* Look for it in the table */
	for (p=dev_fd; p<&dev_fd[20]; p++) {
		if (p->count && eq(p->dev_name, name)) {/* same device? */
			p->count++;			/* bump usage */
			return p->fd;			/* re-use this fd */
		}
		if (empty==NULL && p->count==0)		/* empty slot? */
			empty = p;			/* remember it */
	}

#endif
	fd = open(name, O_RDWR);		/* try to open rw */
	if (fd==-1 && mode!=O_RDWR)		/* requested different? */
		fd = open(name, mode);		/* open as requested */
	if (fd==-1)				/* if it still failed: */
		return fd;			/* return an error */
#ifdef CACHE
	if (empty!=NULL) {			/* if any spots */
		empty->fd = fd;			/* use it for our own */
		empty->count = 1;		/* we are the first user */
		strcpy(empty->dev_name, name);	/* save device name */
	}
#endif
	return fd;
}


close_dev(fd)
register int fd;
{
#ifdef CACHE
	register struct dev_fd *p;

	bfssync();	/* update dirty buffers */

	/* Look for it in the table */
	for (p=dev_fd; p<&dev_fd[20]; p++) {
		if (p->count && p->fd == fd) {		/* found it */
			p->count--;			/* one less usage */
			if (p->count)			/* if uses left: */
				return 0;		/* don't close it */
			else				/* last usage */
			{
				unlink(BIF_LOCK);
				return close(fd);	/* close it */
			}
		}
	}

#endif
	unlink(BIF_LOCK);
	return close(fd);		/* not in table, just close it */
}



get_ino(fd, inumber, ino)
register int fd;
register int inumber;
register struct dinode *ino;
{
	register int addr;

	addr = (inumber-1)*sizeof(struct dinode)+2*BSIZE;
	get(fd, addr, (char *)ino, sizeof(*ino));
}


put_ino(fd, inumber, ino)
register int fd;
register int inumber;
register struct dinode *ino;
{
	register int addr;

	addr = (inumber-1)*sizeof(struct dinode)+2*BSIZE;
	put(fd, addr, (char *)ino, sizeof(*ino));
}


get(fd, addr, buffer, size)
register int fd;
register int addr;
register char *buffer;
register int size;
{
	register int i, bnum;
	register char *bp;

	if (size<0 || size>BSIZE)
		fatal("get: trying to read at address %d; size=%d", addr, size);

	bnum = addr/BSIZE;
	bp = get_block(fd, bnum);		/* get pointer to block */

	for (i=size, bp+=(addr%BSIZE); i>0; i--)
		*buffer++ = *bp++;

	return size;
}


put(fd, addr, buffer, size)
register int fd;
register int addr;
register char *buffer;
register int size;
{
	register int i, bnum;
	register char *bp, *to;

	if (size<0 || size>BSIZE)
		fatal("put: trying to read at address %d; size=%d", addr, size);

	bnum = addr/BSIZE;
	bp = get_block(fd, bnum);		/* get pointer to block */

	for (i=size, to=bp+(addr%BSIZE); i>0; i--)
		*to++ = *buffer++;
	put_block(fd, bnum, bp);

	return size;
}


#ifdef CACHE
#define CACHESIZE 20

struct cache {
	boolean valid;
	boolean dirty;
	int when_accessed;
	int fd;
	int bnum;
	char block[BSIZE];
} cache[CACHESIZE];

static int cache_time = 0;
#endif


char *
get_block(fd, bnum)
register int fd;
register int bnum;
{

#ifdef CACHE
	register struct cache *p, *best;
	register char *bp;

	/***
	for (p=cache; p<&cache[CACHESIZE]; p++)
		if (p->valid)
			bugout("cache[%d]: access=%d fd=%d bnum=%d",
				p-cache, p->when_accessed, p->fd, p->bnum);
	***/

	cache_time++;
	/* try to find it in the cache */
	for (p=cache; p<&cache[CACHESIZE]; p++) {
		if (p->valid && p->fd==fd && p->bnum==bnum) {
			/* bugout("cached at location %d", p-cache); */
			p->when_accessed = cache_time;
			return p->block;
		}
	}

	/* find an empty spot for it in the cache */
	for (p=best=cache; p<&cache[CACHESIZE]; p++) {
		if (!(p->valid)) {
			best=p;
			break;
		}
		if (p->when_accessed < best->when_accessed)
			best = p;
	}

#ifdef NOWRITETHROUGH
	/* write out the old buffer if it's dirty */
	if (best->valid && best->dirty) {
		/* bugout("get_block: writing dirty block %d", best->bnum); */

		if (lseek(best->fd, (long) best->bnum*BSIZE, 0) == -1) {
			perror("get_block: can't seek");
			exit(3);
		}

		if (write(best->fd, best->block, BSIZE) != BSIZE) {
			perror("get_block: can't write");
			exit(4);
		}
	}
#endif

	/* bugout("Reading it into cache[%d]", best-cache); */
	bp=best->block;				/* our new block */

	best->valid = true;
	best->dirty = false;
	best->when_accessed = cache_time;
	best->fd = fd;
	best->bnum = bnum;
#else
	static char bp[BSIZE];
#endif


	if (lseek(fd, (long) bnum*BSIZE, 0) == -1) {
		perror("get_block: can't seek");
		exit(3);
	}

	if (read(fd, bp, BSIZE) != BSIZE) {
		perror("get_block: can't read");
		exit(4);
	}

	return bp;
}


put_block(fd, bnum, block)
register int fd;
register int bnum;
register char *block;
{
#ifdef CACHE
	register struct cache *p, *best;

	cache_time++;

	/*
	 * If it's in the cache, update that cache entry.
	 */
	for (p=cache; p<&cache[CACHESIZE]; p++)
		if (p->valid && p->fd==fd && p->bnum==bnum) {
			if (p->block != block)		/* update cache */
				bcopy(p->block, block);
#ifdef NOWRITETHROUGH
			p->dirty = true;
			p->when_accessed = cache_time;
			return;
#endif
			}

#ifdef NOWRITETHROUGH
	/* find an empty spot for it in the cache */
	for (p=best=cache; p<&cache[CACHESIZE]; p++) {
		if (!(p->valid)) {
			best=p;
			break;
		}
		if (p->when_accessed < best->when_accessed)
			best = p;
	}

	/* write out the old buffer if it's dirty */
	if (best->valid && best->dirty) {
		/* bugout("put_block: writing dirty block %d", best->bnum); */

		if (lseek(best->fd, (long) best->bnum*BSIZE, 0) == -1) {
			perror("put_block: can't seek");
			exit(3);
		}

		if (write(best->fd, best->block, BSIZE) != BSIZE) {
			perror("put_block: can't write");
			exit(4);
		}
	}

	best->valid = true;
	best->dirty = true;
	best->when_accessed = cache_time;
	best->fd = fd;
	best->bnum = bnum;
	bcopy(best->block, block);
	return;
#endif
#endif

	/* bugout("put_block: writing block %d", bnum); */

	if (lseek(fd, (long) bnum*BSIZE, 0) == -1) {
		perror("put_block: can't seek");
		exit(3);
	}

	if (write(fd, block, BSIZE) != BSIZE) {
		perror("put_block: can't write");
		exit(4);
	}
}

bfssync()
{
#ifdef CACHE
#ifdef NOWRITETHROUGH
	register struct cache *p;

	for (p=cache; p<&cache[CACHESIZE]; p++) {
		if (!p->valid || !p->dirty)
			continue;

		/* bugout("bfssync: writing dirty block %d", p->bnum); */

		if (lseek(p->fd, (long) p->bnum*BSIZE, 0) == -1) {
			perror("bfssync: can't seek");
			exit(3);
		}

		if (write(p->fd, p->block, BSIZE) != BSIZE) {
			perror("bfssync: can't write");
			exit(4);
		}

		p->dirty = false;
	}
#endif
#endif
}


bfsstat(path, buf)
register char *path;
register struct stat *buf;
{
	register int fd;

	if (!bfspath(path))
		return stat(path, buf);

	fd = bfsopen(path, 0);
	if (fd==-1)
		return -1;
	bfsfstat(fd, buf);
	bfsclose(fd);
	return 0;
}



bfsfstat(fd, buf)
register int fd;
register struct stat *buf;
{
	register struct info *dp;
	register struct dinode *p;

	if (!bfsfile(fd))
		return fstat(fd, buf);

	dp = &info[fd-OFFSET];
	p = &dp->inode;

	buf->st_dev	= p->di_rdev;
	buf->st_ino	= dp->inode_number;
	buf->st_mode	= p->di_mode;
	buf->st_nlink	= p->di_nlink;
	buf->st_uid	= p->di_uid;
	buf->st_gid	= p->di_gid;
	buf->st_rdev	= -1;
	buf->st_size	= p->di_size;
	buf->st_atime	= p->di_atime;
	buf->st_mtime	= p->di_mtime;
	buf->st_ctime	= p->di_ctime;

	return 0;
}

exist(path)
register char *path;
{
	int fd;

	if (!bfspath(path))
		return access(path,0);

	fd = bfsopen(path, 0);		/* Try to open it. */
	if (fd==-1)			/* If we couldn't open it */
		return false;		/* then it doesn't exist. */
	else {				/* If it did exist */
		bfsclose(fd);		/* then close it */
		return true;		/* tell them that it did exist */
	}
}

check(msg, a,b,c,d,e,f)
char *msg;
{
	int ret;

	if (!debug)
		return;

	bfssync();

	bugout(msg,a,b,c,d,e,f);
	ret = system("exec sh");
	if (ret!=0)
		exit(ret);
		
}

rmlock()	/* remove device lock upon receipt of signal */
{
	register int fd;
	int pid;

	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	bfssync();	/* update dirty buffers */
	fd = open(BIF_LOCK, O_RDONLY);
	if(fd < 0)
	     exit(-1);	/* proper behavior ? */
	read(fd, (char *) &pid, sizeof(int));
	if(pid == getpid())
		if(unlink(BIF_LOCK) < 0)
			printf("can't remove lockfile %s!\n",BIF_LOCK);
	exit(-1);
}
