/* @(#) $Revision: 37.1 $ */   
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                 io.c                                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "s500defs.h"
#include "sdf.h"

#define min(x, y) 	((x) < (y) ? (x) : (y))

extern struct dev_fd dev_fd[];
static int cachetime;
static int cachemax = CACHEMAX;

#ifdef	DEBUG
#include <stdio.h>
extern int debug;
#endif	DEBUG

#define	CACHEINIT	{ 0, 0, 0, 0L, (char *) NULL }
struct cache cache[CACHEMAX] = {
	CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT,
	CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT,
	CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT,
	CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT,
	CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT, CACHEINIT
};

/* ------------------------------------- */
/* get - routine to set up physical read */
/* ------------------------------------- */

get(sp, addr, buf, size)
register struct sdfinfo *sp;
register int addr, size;
register char *buf;
{
	register char *dest;
	register int i, bnum, bsize = sp->filsys.s_blksz;
	char *getblock();

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "get(sdfinfo[%d], (addr) %d, (buf) 0x%08x, %d)\n",
		sdffileno(sp), addr, buf, size);
}
#endif	DEBUG

	if (size < 0 || size > bsize)
		fatal("get(): bad request size %d", size);
	bnum = addr / bsize;
	dest = getblock(sp, bnum);
	if (dest == NULL)
		return(-1);
	for (i = size, dest += (addr % bsize); i > 0; i--)
		*buf++ = *dest++;
	return(size);
}

/* --------------------------------------- */
/* getblock - lowest level I/O (real read) */
/* --------------------------------------- */

char *
getblock(sp, bnum)
register struct sdfinfo *sp;
register int bnum;
{
	register struct cache *p, *best;
	register int bsize = sp->filsys.s_blksz;
	char *malloc();
	long lseek();

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "getblock(sdfinfo[%d], (block #) %d)\n",
		sdffileno(sp), bnum);
}
#endif	DEBUG

	cachetime++;
	for (p = cache; p < &cache[cachemax]; p++) {
		if (p->valid && p->fd == sp->fd && p->bnum == bnum) {
			p->atime = cachetime;
			return(p->block);
		}
	}
/*
 * the following if should NEVER execute, but just in case...
 */
retry:
	if (cachemax == 0)
		nomemory();
	for (p = best = cache; p < &cache[cachemax]; p++) {
		if (!p->valid) {
			best = p;
			break;
		}
		if (p->atime < best->atime)
			best = p;
	}
	if (best->block == (char *) NULL) {
		best->block = malloc((unsigned) bsize);
		if (best->block == NULL) {
			cachemax = best - cache;
			goto retry;
		}
	}
	best->valid++;
	best->atime = cachetime;
	best->fd = sp->fd;
	best->bnum = bnum;
	if (lseek(sp->fd, (long) (bnum * bsize), 0) < 0)
		return((char *) NULL);
	if (read(sp->fd, best->block, (unsigned) bsize) != bsize)
		return((char *) NULL);
	return(best->block);
}

/* -------------------------------------- */
/* put - routine to set up physical write */
/* -------------------------------------- */

put(sp, addr, buf, size)
register struct sdfinfo *sp;
register int addr, size;
register char *buf;
{
	register char *bp, *to;
	register int i, bnum, bsize = sp->filsys.s_blksz;

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "put(sdfinfo[%d], (addr) %d, (buf) 0x%08x, %d)\n",
		sdffileno(sp), addr, buf, size);
}
#endif	DEBUG

	if (size < 0 || size > bsize)
		fatal("put(): bad request size %d", size);
	bnum = addr / bsize;
	bp = getblock(sp, bnum);
	for (i = size, to = bp + (addr % bsize); i > 0; i--)
		*to++ = *buf++;
	if (putblock(sp, bnum, bp) < 0)
		return(-1);
	return(size);
}

/* ---------------------------------------- */
/* putblock - lowest level I/O (real write) */
/* ---------------------------------------- */

putblock(sp, bnum, buf)
register struct sdfinfo *sp;
register int bnum;
register char *buf;
{
	register int bsize = sp->filsys.s_blksz;
	register struct cache *p;
	long lseek();

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "putblock(sdfinfo[%d], (block #) %d, (buf) 0x%08x)\n",
		sdffileno(sp), bnum, buf);
}
#endif	DEBUG

	cachetime++;
	for (p = cache; p < &cache[cachemax]; p++)
		if (p->valid && p->fd == sp->fd && p->bnum == bnum)
			bcopy(p->block, buf, bsize);
	if (lseek(sp->fd, (long) (bnum * bsize), 0) < 0)
		return(-1);
	if (write(sp->fd, buf, (unsigned) bsize) != bsize)
		return(-1);
	if (!sp->written)
		sp->written = 1;
	return(0);
}

/* ------------------------- */
/* inoread - read from inode */
/* ------------------------- */

inoread(sp, inum, ip, offset, buf, size)
register struct sdfinfo *sp;
register int inum, offset, size;
register struct dinode *ip;
register char *buf;
{
	register int readlen = 0, fragment, addr;
	register char *cp;
	register int bsize = sp->filsys.s_blksz;

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "inoread(sdfinfo[%d], (i#) %d, (off) %d, (buf) 0x%08x, %d)\n",
		sdffileno(sp), inum, offset, buf, size);
}
#endif	DEBUG

	readlen = 0;
	cp = buf;
	addr = off2addr(sp, inum, ip, offset, READ);
	if (addr == -1)
		return(-1);
	if (addr % bsize) {					/* 1st part */
		fragment = min(size, bsize - (addr % bsize));
		readlen += get(sp, addr, cp, fragment);
		cp += fragment;
		offset += fragment;
		size -= fragment;
		if (size) {			/* any left? */
			addr = off2addr(sp, inum, ip, offset, READ);
			if (addr == -1)
				return(-1);
		}
	}
	while (size >= bsize) {					/* 2nd part */
		readlen += get(sp, addr, cp, bsize);
		cp += bsize;
		offset += bsize;
		size -= bsize;
		if (size) {		/* any left? */
			addr = off2addr(sp, inum, ip, offset, READ);
			if (addr == -1)
				return(-1);
		}
	}
	if (size)						/* 3rd part */
		readlen += get(sp, addr, cp, size);
	return(readlen);
}

/* ------------------------- */
/* inowrite - write to inode */
/* ------------------------- */

inowrite(sp, inum, ip, offset, buf, size)
register struct sdfinfo *sp;
register int inum, offset, size;
register struct dinode *ip;
register char *buf;
{
	int writelen = 0, fragment, addr, sizechange = 0;
	register char *cp;
	register int bsize = sp->filsys.s_blksz;

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "inowrite(sdfinfo[%d], (i#) %d, (off) %d, (buf) 0x%08x, %d)\n",
		sdffileno(sp), inum, offset, buf, size);
}
#endif	DEBUG

	writelen = 0;
	cp = buf;
	if (ip->di_size + size > ip->di_max)
		size = ip->di_max - ip->di_size + 1;
	addr = off2addr(sp, inum, ip, offset, WRITE);
	if (addr == -1)
		return(-1);
	if (addr % bsize) {					/* 1st part */
		fragment = min(size, bsize - (addr % bsize));
		writelen += put(sp, addr, cp, fragment);
		if (offset + fragment > ip->di_size) {
			ip->di_size = offset + fragment;
			sizechange++;
		}
		cp += fragment;
		offset += fragment;
		size -= fragment;
		if (size) {		/* any left ? */
			addr = off2addr(sp, inum, ip, offset, WRITE);
			if (addr == -1)
				return(-1);
		}
	}
	while (size >= bsize) {					/* 2nd part */
		writelen += put(sp, addr, cp, bsize);
		if (offset + size > ip->di_size) {
			ip->di_size = offset + size;
			sizechange++;
		}
		cp += bsize;
		offset += bsize;
		size -= bsize;
		if (size) {		/* any left? */
			addr = off2addr(sp, inum, ip, offset, WRITE);
			if (addr == -1)
				return(-1);
		}
	}
	if (size) {						/* 3rd part */
		writelen += put(sp, addr, cp, size);
		if (offset + size > ip->di_size) {
			ip->di_size = offset + size;
			sizechange++;
		}
	}
	if (sizechange)
		if (putino(sp, inum, ip) < 0)
			return(-1);
	return(writelen);
}

/* ------------------------------- */
/* getino - grab inode information */
/* ------------------------------- */

getino(sp, inum, ip)
register struct sdfinfo *sp;
register int inum;
register struct dinode *ip;
{
	register int addr;

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "getino(sdfinfo[%d], (i#) %d, (i_buf) 0x%08x)\n", sdffileno(sp),
		inum, ip);
}
#endif	DEBUG

	if (inum == 0) {
		bcopy((char *) ip, (char *) &sp->dp->fainode, FA_SIZ);
		return(0);
	}
	addr = ino2addr(sp, inum);
	if (addr == -1)
		return(-1);
	if (get(sp, addr, (char *) ip, FA_SIZ) != FA_SIZ)
		return(-1);
	return(inum);
}

/* -------------------------------- */
/* putino - write inode on the disc */
/* -------------------------------- */

putino(sp, inum, ip)
register struct sdfinfo *sp;
register int inum;
register struct dinode *ip;
{
	register int addr;

#ifdef	DEBUG
if (debug) {
	fprintf(stderr, "putino(sdfinfo[%d], (i#) %d, (i_buf) 0x%08x)\n",
		sdffileno(sp), inum, ip);
}
#endif	DEBUG

	addr = ino2addr(sp, inum);
	if (addr == -1)
		return(-1);
	if (put(sp, addr, (char *) ip, FA_SIZ) != FA_SIZ)
		return(-1);
	return(inum);
}
