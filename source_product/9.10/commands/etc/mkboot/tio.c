/* $Source: /misc/source_product/9.10/commands.rcs/etc/mkboot/tio.c,v $
 * $Revision: 70.1 $        $Author: ssa $
 * $State: Exp $        $Locker:  $
 * $Date: 92/05/28 13:27:14 $
 */

/*
 * Efficient tape access routines. Designed primarily
 * for use by applications accessing LIF files.
 *
 *	The following code allows random access to sequential
 * or random access devices. It attempts to accomplish this
 * efficiently through a simple buffer cache. The buffers in
 * the cache are round robin reclaimed. Buffers for block 0 and
 * 1 are exceptions to this. Since the LIF header and directory
 * are special, buffers for block 0 and 1 are always resident.
 * This code also supports regular file access. However, it
 * is not cached. 
 * 
 * External procedures include...
 *	int fd = topen(char *filename, int flags)
 *	tclose(int fd)
 *	int bytes_read = tread(int fd, char *buffer, int count)
 *	int bytes_written = twrite(int fd, char *buffer, int count)
 *	int new_offset = tlseek(int fd, int offset, int unused)
 *
 *	Return values indicating errors are equivalent to those
 * 	of open, close, read, write, lseek.
 *
 * Limitations include...
 * 	1) Will not work correctly with tape media having blocksize
 *	   equal or greater than MAXCACHESIZE. It will assume they
 *         are disks.
 *	2) May not work if tape media block size is less than 
 * 	   MINCACHELINESIZE.
 *	3) Will return errors if the caller has more than
 *	   MAXTOPENS+3 currently open files. This includes console
 *	   access on descriptors 0, 1, and 2.
 *	4) May not work correctly if given a block device file for
 *	   tapes. Character interface is recommended for all media.
 *	5) tlseek does not use the last parameter. All seeks are assumed
 *	   from the beginning of the media.
 *	6) Assumes at least 2 blocks are available on tapes.
 *	7) Accesses to regular files are not cached.
 *	8) Writes (twrite) to tape devices will return an error.
 *         This is not a supported activity.
 */
#include <sys/param.h>
#include <sys/mtio.h>		/* these are for mag-tape skip-counts */
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TAG_EV	0x1	/* Entry valid */
#define TAG_DV	0x2	/* Data valid */
#define TAG_DF	0x4	/* Data fragment */

/*
 * Cache line size is equal to the device record
 * size. So, it'll increase with increasing record size.
 * However, the total space for cache should stay constant.
 * This means the number of cache lines decreases with
 * increasing record size.
 */
#define MAXCACHELINES		32
#define MINCACHELINESIZE	2048
#define MAXCACHESIZE		MAXCACHELINES * MINCACHELINESIZE

struct bcache {
	unsigned short int lastfilled;
	unsigned short int tagsused;
	unsigned int cfrag[MAXCACHELINES];
	unsigned int ctag[MAXCACHELINES];
	unsigned char data[MAXCACHESIZE];
};

#define F_IS_OPEN	0x1
#define F_IS_TAPE	0x2
#define F_IS_FILE	0x4

struct devinfo {
	unsigned short int flags;
	unsigned int recsize;
	unsigned int ldevloc;
	unsigned int pdevloc;
	struct bcache *cache;
};

/*
 * There's alot of devinfo structures cause we're gonna
 * assume a one to one correspondence between normal open
 * fd's and our returned fd's. So, the real fd will be the
 * index into our fd table. It's not efficient but it
 * saves having to do fd allocation code and will be more
 * flexible/clear to the caller. Note fd 0,1,2 are unused.
 */
#define MAXTOPENS	9
struct devinfo ddesc[MAXTOPENS+3]; 
#define INVALIDFD(fd) ((fd < 0) || (fd >= MAXTOPENS + 3))
#define VALIDFD(fd) (!INVALIDFD(fd))

/* For lint.... */
extern void *malloc(); /* Annoying!... what's a pointer to nothing??? */
extern void sync(); 
extern off_t lseek();


topen(devfile, flags)
char *devfile;
int flags;
{
	int tmpfd, st;
	int cnt1, cnt2;
	struct mtop op;
	struct stat stb;

	tmpfd = open(devfile, flags);
	if (INVALIDFD(tmpfd)) 
		return(-1);
	ddesc[tmpfd].flags = 0; /* Make sure flags are clear */

	/* 
	 * What is it???
	 */
	if (fstat(tmpfd, &stb) == -1) {
		(void)close(tmpfd);
		return(-1);
	}
	switch (stb.st_mode & S_IFMT) {
	case S_IFREG:
			/*
			 * Regular files are uncached by this code.
			 */
			ddesc[tmpfd].flags = (F_IS_FILE | F_IS_OPEN);
			return(tmpfd);
	case S_IFBLK:
	case S_IFCHR:
			/*
			 * These devices are round-robin cached.
			 */
			break;
	default:
			/*
			 * Huh???
			 */
			(void)close(tmpfd);
			return(-1);
	};

	/*
	 * Attempt rewind just in case it's a tape
	 * If it fails, big deal.
	 */
	op.mt_op = MTREW;
	op.mt_count = 1;
	st = ioctl(tmpfd, MTIOCTOP, &op);
	
	/*
	 * Get space for the cache for this device.
	 */
	ddesc[tmpfd].cache = (struct bcache *)
				malloc(sizeof(struct bcache));
	if(ddesc[tmpfd].cache == (struct bcache *)0){
		(void)close(tmpfd);
		return(-1);
	}
	/*
	 * Attempt to read first 2 records into 
	 * cache. These records will always be
	 * cache resident.
	 */
	cnt1 = read(tmpfd, ddesc[tmpfd].cache->data, MAXCACHESIZE);
	if( cnt1 <= 0 ){
		(void)close(tmpfd);
		return(-1);
	}
	ddesc[tmpfd].cache->ctag[0] = ( 0 | TAG_DV | TAG_EV);
	ddesc[tmpfd].ldevloc = 0;
	ddesc[tmpfd].pdevloc = cnt1;

	if( cnt1 == MAXCACHESIZE ){
		/*
		 * This is either a disk or a tape with
		 * a BIG block size. Assume a disk.
		 * Treat it as if it were a record oriented
		 * device yielding MAXCACHELINES cachelines. 
		 */
		ddesc[tmpfd].recsize = MINCACHELINESIZE;
		ddesc[tmpfd].cache->tagsused = MAXCACHELINES;
		ddesc[tmpfd].cache->lastfilled = MAXCACHELINES - 1;
		for(st=1; st < MAXCACHELINES; st++){
			ddesc[tmpfd].cache->ctag[st] = 
				((st*MINCACHELINESIZE) | TAG_DV | TAG_EV);
		}
		ddesc[tmpfd].flags &= ~F_IS_TAPE;
		ddesc[tmpfd].flags |= F_IS_OPEN;
		return(tmpfd);
	}
	ddesc[tmpfd].cache->tagsused = MAXCACHESIZE/cnt1;
	ddesc[tmpfd].recsize = cnt1;

	cnt2 = read(tmpfd, ddesc[tmpfd].cache->data + cnt1, 
				MAXCACHESIZE - cnt1);
	if( cnt2 <= 0 ){
		(void)close(tmpfd);
		return(-1);
	}
	if(cnt1 != cnt2){
		/*
		 * Unequal record sizes.... Forget it.
		 */
		(void)close(tmpfd);
		return(-1);
	}

	ddesc[tmpfd].cache->ctag[1] = 
		(ddesc[tmpfd].pdevloc | TAG_DV | TAG_EV);
	ddesc[tmpfd].cache->lastfilled = 1;
	ddesc[tmpfd].pdevloc += cnt2;

	ddesc[tmpfd].flags |= F_IS_TAPE;
	ddesc[tmpfd].flags |= F_IS_OPEN; /* Done. */
	return(tmpfd);
}

tclose(fd)
int fd;
{
	if(INVALIDFD(fd))
		return(-1);

	ddesc[fd].flags = 0;
	(void)close(fd);
	sync();
	return(0);
}

static int
fcachecopy(fd, cl, dboffset, cnt, ubuf)
int fd, cl; /* cl = cache line index */
unsigned int dboffset; 
int cnt;
char *ubuf;
{
	int i, dav; /* dav = data available */
	char *cldp; /* Cache line data pointer */
	
	if(ddesc[fd].cache->ctag[cl] & TAG_DF)
		dav = ddesc[fd].cache->cfrag[cl];
	else 
		dav = ddesc[fd].recsize;
	cldp = (char *)
		(ddesc[fd].cache->data + (cl * ddesc[fd].recsize));

	for(i=dboffset; cnt; i++){
		if(i >= dav)
			break;
		*ubuf++ = cldp[i];
		cnt--;
	}

	if(cnt > 0)
		cnt = dav - dboffset;
	else
		cnt = i - dboffset;
	return(cnt);
}

static int
tcachecopy(fd, cl, dboffset, cnt, ubuf)
int fd, cl; /* cl = cache line index */
unsigned int dboffset; 
int cnt;
char *ubuf;
{
	int i; 
	char *cldp; /* Cache line data pointer */
	
	cldp = (char *)
		(ddesc[fd].cache->data + (cl * ddesc[fd].recsize));

	for(i=dboffset; cnt; i++){
		if(i >= ddesc[fd].recsize)
			break;
		cldp[i] = *ubuf++;
		cnt--;
	}

	if(cnt > 0)
		cnt = ddesc[fd].recsize - dboffset;
	else
		cnt = i - dboffset;
	return(cnt);
}

static int chits = 0;
static int cmiss = 0;

static int
cachehit(cp, l)
struct bcache *cp;
unsigned int l;
{
	int i;

	for(i=0; i < MAXCACHELINES; i++){
		if(!(cp->ctag[i] & TAG_EV))
			continue; /* No valid entry */
		if(!(cp->ctag[i] & TAG_DV))
			continue; /* No valid data */
		if((l & cp->ctag[i]) == l)
			break; /* address part matches */
	}
	if(i < MAXCACHELINES){
		chits++;
	} else {
		cmiss++;
		i = -1;
	}
	return(i);
}

static int drew = 0;
static int dfwd = 0;

static void
cachefill(fd, dbaddr)
int fd;
unsigned int dbaddr;
{
	struct mtop op;
	int st, cl;
	unsigned char *dp;

	if(ddesc[fd].pdevloc > dbaddr){
		/*
		 * Oooops. Block is behind our current 
		 * position.
		 */
		drew++;
		if(ddesc[fd].flags & F_IS_TAPE){
	         	/* Rewind */
			op.mt_op = MTREW;
			op.mt_count = 1;
			st = ioctl(fd, MTIOCTOP, &op);
		} 
		/*
		 * Tape is rewound. Disk doesn't matter.
		 * Block is now forward. Let's ask ourself
		 * to get it.
		 */
		ddesc[fd].pdevloc = 0;
		cachefill(fd, dbaddr);
	} else if(ddesc[fd].pdevloc < dbaddr){
		/*
		 * Block is ahead of our current
		 * position.
		 */
		dfwd++;
		if(ddesc[fd].flags & F_IS_TAPE){
			while(ddesc[fd].pdevloc < dbaddr){
				cachefill(fd, ddesc[fd].pdevloc);
			}
		} else {
			ddesc[fd].pdevloc = dbaddr;
			(void) lseek(fd, (off_t)dbaddr, 0);
		}
		/*
		 * Device is positioned. Now let's ask
		 * ourself to get the right block.
		 */
		cachefill(fd, dbaddr);
	} else {
		/*
		 * Good. We're positioned correctly.
		 * Just read the block into a cache line.
		 */
		cl = getcacheline(ddesc[fd].cache);
		dp = ddesc[fd].cache->data + (cl * ddesc[fd].recsize);
		st = read(fd, dp, ddesc[fd].recsize);
		ddesc[fd].cache->ctag[cl] = dbaddr | TAG_EV | TAG_DV;
		if(st != ddesc[fd].recsize) {
			ddesc[fd].cache->ctag[cl] |= TAG_DF;
			ddesc[fd].cache->cfrag[cl] |= st;
		}
		ddesc[fd].pdevloc += ddesc[fd].recsize;
	}

}

static int
getcacheline(cp)
struct bcache *cp;
{
	/*
	 * Formula keeps cachelines 0 and 1 from being
	 * reclaimed. All others are round robin allocated.
	 */
	cp->lastfilled = (cp->lastfilled + 1) % cp->tagsused;
	if(cp->lastfilled < 2)
		cp->lastfilled = 2;
	return(cp->lastfilled);
}

cachelinesync(fd, cl)
int fd, cl; /* cl = cache line index */
{
	char *cldp; /* Cache line data pointer */
	unsigned int baddr;
	int cnt;
	
	baddr = ddesc[fd].cache->ctag[cl] & ~0xff; /* Clear flags */
	cldp = (char *)
		(ddesc[fd].cache->data + (cl * ddesc[fd].recsize));

	(void)lseek(fd, (off_t)baddr, 0); /* Set the device location */
	cnt = write(fd, cldp, ddesc[fd].recsize);
	
	ddesc[fd].pdevloc = baddr + ddesc[fd].recsize;
	return(cnt);
}

tread(fd, ubuf, cnt)
int fd;
char *ubuf;
int cnt;
{
	unsigned int loc = ddesc[fd].ldevloc;
	struct bcache *cp = ddesc[fd].cache;
	int xfrd, cl; /* cl = cache line index */
	unsigned int dbaddr, dboffset; 

	if(INVALIDFD(fd))
		return(-1);
	
	if(ddesc[fd].flags & F_IS_FILE) {
		/* No cache */
		(void)lseek(fd, (off_t)ddesc[fd].ldevloc, 0);
		ddesc[fd].ldevloc += cnt;
		return(read(fd, ubuf, cnt));
	}

	while(cnt > 0) {
		dbaddr = loc & ~(ddesc[fd].recsize - 1);
		dboffset = loc & (ddesc[fd].recsize - 1);
		if((cl = cachehit(cp, dbaddr)) < 0){
			/* Cache miss */
			cachefill(fd, dbaddr);
			/* Now try again */
		} else {
			/* Cache hit */
			xfrd = fcachecopy(fd, cl, dboffset, cnt, ubuf);
			if(xfrd <= 0)
				break;
			loc += xfrd;
			ubuf += xfrd;
			cnt -= xfrd;
		
		}
	}
	cnt = loc - ddesc[fd].ldevloc;
	ddesc[fd].ldevloc = loc;

	return(cnt);
}

twrite(fd, ubuf, cnt)
int fd;
char *ubuf;
int cnt;
{
	unsigned int loc = ddesc[fd].ldevloc;
	struct bcache *cp = ddesc[fd].cache;
	int xfrd, cl; /* cl = cache line index */
	unsigned int dbaddr, dboffset; 

	if(INVALIDFD(fd))
		return(-1);

	/*
	 * No tape writes allowed.
	 */
	if(ddesc[fd].flags & F_IS_TAPE)
		return(-1);

	if(ddesc[fd].flags & F_IS_FILE) {
		/* No cache */
		(void)lseek(fd, (off_t)ddesc[fd].ldevloc, 0);
		ddesc[fd].ldevloc += cnt;
		return(write(fd, ubuf, cnt));
	}

	while(cnt > 0) {
		dbaddr = loc & ~(ddesc[fd].recsize - 1);
		dboffset = loc & (ddesc[fd].recsize - 1);
		if((cl = cachehit(cp, dbaddr)) < 0){
			/* Cache miss */
			cachefill(fd, dbaddr);
			/* Now try again */
		} else {
			/* Cache hit */
			xfrd = tcachecopy(fd, cl, dboffset, cnt, ubuf);
			if(xfrd <= 0)
				break;
			if(cachelinesync(fd, cl) != ddesc[fd].recsize)
				return(-1);
			loc += xfrd;
			ubuf += xfrd;
			cnt -= xfrd;
		
		}
	}
	cnt = loc - ddesc[fd].ldevloc;
	ddesc[fd].ldevloc = loc;

	return(cnt);
}

tlseek(fd, l, unused)
int fd;
unsigned int l, unused;
{
	if(INVALIDFD(fd))
		return(-1);

	return(ddesc[fd].ldevloc = l);
}
