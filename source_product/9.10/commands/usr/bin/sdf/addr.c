/* @(#) $Revision: 37.1 $ */     
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                addr.c                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "s500defs.h"
#include <sys/stat.h>
#include <errno.h>
#include "sdf.h"			
#include "bit.h"

#define	TRUE		1
#define	FALSE		0

extern struct dev_fd dev_fd[];
extern int errno;

#ifdef	DEBUG
extern int debug;
#endif	DEBUG

/* --------------------------------------------------- */
/* ino2addr - convert inode number to physical address */
/* --------------------------------------------------- */

ino2addr(sp, inum)
register struct sdfinfo *sp;
register int inum;
{

#ifdef	DEBUG
	bugout("ino2addr(sdfinfo[%d], %d)\n", sdffileno(sp), inum);
#endif	DEBUG

	return(off2addr(sp, FA_INUM, &sp->dp->fainode, (inum * FA_SIZ), READ));
}

/* ----------------------------------------------------- */
/* off2addr - convert logical offset to physical address */
/* ----------------------------------------------------- */

off2addr(sp, inum, ip, off, rdwr)
register struct sdfinfo *sp;
register int inum, off, rdwr;
register struct dinode *ip;
{
	struct em_rec embuf;
	register int i, sum, extent, bnum, lastinum;
	register int bsize = sp->filsys.s_blksz;

#ifdef	DEBUG
	bugout("off2addr(sdfinfo[%d], (i#) %d, 0x%08x, (off) %d, (rdwr) %d)\n",
	  sdffileno(sp), inum, ip, off, rdwr);
#endif	DEBUG

	if (rdwr == WRITE && inum >= FMAP_INUM && inum < sp->validi)
		return(-1);
	if (ip->di_type != R_INODE)
		return(-1);
	if (off > ip->di_size)
		return(-1);
	bnum = off / bsize;
	lastinum = inum;
	for (sum = 0, i = 0; i < ip->di_exnum; i++) {
		sum += ip->di_extent[i].di_numblk;
		if (sum > bnum)
			return(((ip->di_extent[i].di_startblk
		    	+ ip->di_extent[i].di_numblk + bnum 
		    	- sum) * bsize) + (off % bsize));
	}
	extent = ip->di_exmap;
	while (extent != -1) {
 		getino(sp, extent, (struct dinode *) &embuf);
		if (sum == ip->di_blksz) {
			if (inum != lastinum)
				getino(sp, lastinum, (struct dinode *) &embuf);
			break;
		}
		for (i = 0; i < embuf.e_exnum; i++) {
			if (inum == 0 && embuf.e_extent[i].e_numblk == 0)
				break;
			sum += embuf.e_extent[i].e_numblk;
			if (sum > bnum)
				return(((embuf.e_extent[i].e_startblk
				    + embuf.e_extent[i].e_numblk + bnum
				    - sum) * bsize) + (off % bsize));
		}
		lastinum = extent;
		extent = embuf.e_next;
	}

	if (rdwr == READ)
		return(-1);
	if (inum)
		extent = extendino(sp, inum, ip, lastinum, &embuf);
	else
		extent = extendfa(sp, lastinum, &embuf);
	if (extent == -1)
		return(-1);
	return((extent * bsize) + (off % bsize));
}

/* ----------------------------------------- */
/* extendfa - extend the File Attribute file */
/* ----------------------------------------- */

extendfa(sp, inum, emp)
register struct sdfinfo *sp;
register int inum;
register struct em_rec *emp;
{
	register struct dinode *ip = &sp->dp->fainode;
	register int i, lastb, bnum, newi, avail, needed = 0;
	struct em_rec embuf;
	struct dinode ibuf;

#ifdef	DEBUG
	bugout("extendfa(sdfinfo[%d], (i#) %d, 0x%08x)\n",
	  sdffileno(sp), inum, emp);
#endif	DEBUG

	if (inum == FA_INUM) {				/* still in inode 0 */
		i = ip->di_exnum - 1;
		lastb = ip->di_extent[i].di_startblk
		  + ip->di_extent[i].di_numblk - 1;
	}
	else {						/* extent map */
		i = emp->e_exnum - 1;
		lastb = emp->e_extent[i].e_startblk
		  + emp->e_extent[i].e_numblk - 1;
	}
	bnum = allocblk(sp, lastb);
	if (bnum < 0)
		fatal("can't allocate a block for FA file!");
	ip->di_blksz++;

	if (bnum == (lastb + 1)) {	/* same extent? */
		if (inum == FA_INUM)				/* inode 0 */
			ip->di_extent[i].di_numblk++;
		else 						/* extent map */
			emp->e_extent[i].e_numblk++;
	}

	else if ((inum == FA_INUM && i == 3) || (inum != FA_INUM && i == 12)) {
		needed++;
		if (inum == FA_INUM)
			inum = ip->di_exmap;
		else
			inum = emp->e_next;
		getino(sp, inum, (struct dinode *) emp);
		emp->e_exnum = 1;
		emp->e_extent[0].e_startblk = bnum;
		emp->e_extent[0].e_numblk = 1;
		if (inum == FA_INUM)          /* first extent map of inode 0? */
			emp->e_boffset = inobsum(sp, emp->e_inode);
		else
			emp->e_boffset = inobsum(sp, emp->e_last);
	}

	else {
		i++;
		if (inum == FA_INUM) {
			ip->di_exnum++;
			ip->di_extent[i].di_startblk = bnum;
			ip->di_extent[i].di_numblk = 1;
		}
		else {
			emp->e_exnum++;
			emp->e_extent[i].e_startblk = bnum;
			emp->e_extent[i].e_numblk = 1;
		}
	}
	if (inum != FA_INUM)
		if (putino(sp, inum, (struct dinode *) emp) < 0)
			return(-1);
	avail = ip->di_size / FA_SIZ;
	ip->di_size += sp->filsys.s_blksz;
	if (putino(sp, FA_INUM, ip) < 0)
		return(-1);
	for (i = 0; i < (sp->filsys.s_blksz / FA_SIZ); i++) {
		getino(sp, avail + i, &ibuf);
		ibuf.di_type = R_UNUSED;
		if (putino(sp, avail + i, &ibuf) < 0)
			return(-1);
	}
	if (needed && (int) emp->e_next < 0) {
		newi = allocino(sp, sp->validi);
		if (newi == 0)
			fatal("can't allocate extent map for FA file!");
		getino(sp, newi, (struct dinode *) &embuf);
		initem(&embuf);
		embuf.e_last = inum;
		embuf.e_inode = 0;
		if (putino(sp, newi, (struct dinode *) &embuf) < 0)
			return(-1);
		emp->e_next = newi;
		if (putino(sp, inum, (struct dinode *) emp) < 0)
			return(-1);
	}
	return(bnum);
}

/* -------------------------------------------------- */
/* extendino - add a block to the extents in an inode */
/* -------------------------------------------------- */

extendino(sp, originum, ip, inum, emp)
register struct sdfinfo *sp;
register int originum, inum;
register struct dinode *ip;
register struct em_rec *emp;
{
	register int i, lastb, bnum, newi;
	struct em_rec embuf;

#ifdef	DEBUG
	bugout("extendino(sdfinfo[%d], (fa_ptr) 0x%08x, (origi#) %d (i#) %d)\n",
	    sdffileno(sp), emp, originum, inum);
#endif	DEBUG

	if (inum == originum) {					/* inode */
		i = ip->di_exnum - 1;
		lastb = ip->di_extent[i].di_startblk +
		  ip->di_extent[i].di_numblk - 1;
	}
	else if (emp->e_exnum > 0) { 				/* extent map */
		i = emp->e_exnum - 1;
		lastb = emp->e_extent[i].e_startblk +
		  emp->e_extent[i].e_numblk - 1;
	}
	bnum = allocblk(sp, lastb);		/* allocate the block */
	if (bnum < 0)
		return(-1);
	if (bnum == (lastb + 1)) {
		if (inum == originum)
			ip->di_extent[i].di_numblk++;		/* inode */
		else
			emp->e_extent[i].e_numblk++;		/* extent map */
	}

	else if (inum == originum && i < 3) {			/* inode */
		i++;
		ip->di_exnum++;
		ip->di_extent[i].di_startblk = bnum;
		ip->di_extent[i].di_numblk = 1;
	}
	else if (emp->e_type == R_EM && i < 12) {		/* extent map */
		i++;
		emp->e_exnum++;
		emp->e_extent[i].e_startblk = bnum;
		emp->e_extent[i].e_numblk = 1;
	}

	else {
		newi = allocino(sp, sp->pinumber + 1);
		if (newi == 0)
			return(-1);
		if (inum == originum)
			ip->di_exmap = newi;
		else
			emp->e_next = newi;
		getino(sp, newi, (struct dinode *) &embuf);
		initem(&embuf);
		embuf.e_boffset = inobsum(sp, inum);
		embuf.e_inode = originum;
		embuf.e_last = inum;
		embuf.e_next = -1;
		embuf.e_exnum = 1;
		embuf.e_extent[0].e_startblk = bnum;
		embuf.e_extent[0].e_numblk = 1;
		if (putino(sp, newi, (struct dinode *) &embuf) < 0)
			return(-1);
	}
	ip->di_blksz++;
	if (inum != originum)
		if (putino(sp, originum, ip) < 0)
			return(-1);
	if (putino(sp, inum, (struct dinode *) emp) < 0)
		return(-1);
	return(bnum);
}

/* -------------------------------------------------- */
/* trunc - change an existing SDF file to zero length */
/* -------------------------------------------------- */

trunc(sp, rdwr)
register struct sdfinfo *sp;
register int rdwr;
{
	struct em_rec embuf;
	register int i, extent, inum, ret;
	register struct dinode *ip = &sp->inode;

#ifdef	DEBUG
	bugout("trunc(sdfinfo[%d], (rdwr) %d)\n", sdffileno(sp), rdwr);
#endif	DEBUG

	if (sp->inumber == 0)
		return(-1);
	for (i = 0; i < ip->di_exnum; i++) {		/* extents in inode */
		if (ip->di_extent[i].di_startblk < 0)
			break;
		if (i == 0 && rdwr == WRITE) {
			ret = zeroext(sp, ip->di_extent[0].di_startblk + 1, 
			    ip->di_extent[0].di_numblk - 1);
			if (ret < 0)
				return(-1);
			ip->di_extent[0].di_numblk = 1;
			continue;
		}
		ret = zeroext(sp, ip->di_extent[i].di_startblk,
		    ip->di_extent[i].di_numblk);
		if (ret < 0)
			return(-1);
		ip->di_extent[i].di_startblk = -1;
		ip->di_extent[i].di_numblk = 0;
	}
	inum = extent = ip->di_exmap;
	while (extent != -1) {			/* extents in extent map */
		getino(sp, extent, (struct dinode *) &embuf);
		for (i = 0; i < embuf.e_exnum; i++) {
			if (embuf.e_extent[i].e_startblk < 0)
				break;
			ret = zeroext(sp, embuf.e_extent[i].e_startblk,
			    embuf.e_extent[i].e_numblk);
			if (ret < 0)
				return(-1);
		}
		inum = extent;
		extent = embuf.e_next;
		if (freeino(sp, inum, (struct dinode *) &embuf) < 0)
			return(-1);
	}
	if (rdwr == WRITE) {
		ip->di_blksz = 1;
		ip->di_exnum = 1;
	}
	else {
		ip->di_exnum = 0;
		ip->di_blksz = 0;
	}
	ip->di_size = 0;
	if (putino(sp, sp->inumber, ip) < 0)
		return(-1);
	return(0);
}

/* ------------------------------------------------- */
/* zeroext - zero an extent (deallocate disc blocks) */
/* ------------------------------------------------- */

zeroext(sp, start, size)
register struct sdfinfo *sp;
register int start, size;
{
	register int i, *word, bit;
	int map[FA_SIZ / sizeof(int)];

#ifdef	DEBUG
	bugout("zeroext(sdfinfo[%d], (start) %d, (size) %d)\n", sdffileno(sp),
	    start, size);
#endif	DEBUG

	for (i = 0; i < size; i++) {
		getino(sp, ino4blk(start + i), (struct dinode *) map);
		word = &map[((start + i) % 1024) / (FA_SIZ / sizeof(int))];
		bit = (start + i) % BITSPERWORD;
		setbit(*word, bit);
		if (putino(sp, ino4blk(start + i), (struct dinode *) map) < 0)
			return(-1);
		sp->dp->bfree++;
	}
}

/* --------------------------------------------------------------------- */
/* allocblk - allocate a block of memory, try to allocate "startb" first */
/* --------------------------------------------------------------------- */

allocblk(sp, startb)
register struct sdfinfo *sp;
register int startb;
{
	register int inum, nextinum, bnum, *word, bit;
	int map[FA_SIZ / sizeof(int)];

#ifdef	DEBUG
	bugout("allocblk(sdfinfo[%d], (start_b#) %d)\n", sdffileno(sp), startb);
#endif	DEBUG

	if (sp->dp->bfree <= 0) {
		error("no free space on %s", sp->dp->devnm);
		errno = ENOSPC;
		return(-1);
	}
	sp->dp->bfree--;
	if (startb < 0)
		startb = 1;
	for (inum = -1, bnum = startb; ; ) {
		if (bnum > sp->filsys.s_maxblk)
			bnum = 1;
		nextinum = ino4blk(bnum);
		if (nextinum != inum) {
			inum = nextinum;
			getino(sp, inum, (struct dinode *) map);
		}
		word = &map[(bnum % 1024) / (FA_SIZ / sizeof(int))];
		bit = bnum % BITSPERWORD;
		if (bitval(*word, bit)) {
			clrbit(*word, bit);
			if (putino(sp, inum, (struct dinode *) map) < 0)
				return(-1);
			return(bnum);
		}
		bnum++;
	}
}

/* ---------------------------------------------------- */
/* allocino - allocate an inode number from the FA file */
/* ---------------------------------------------------- */

allocino(sp, starti)
register struct sdfinfo *sp;
register int starti;
{
	register int inum, save, avail;
	register struct dinode *ip = &sp->dp->fainode;
	struct dinode ibuf;

#ifdef	DEBUG
	bugout("allocino(sdfinfo[%d], (start_i#) %d)\n", sdffileno(sp), starti);
#endif	DEBUG

	if (starti <= sp->validi)
		starti = sp->validi + 1;
	save = starti;
	for (avail = 0, inum = starti; ; ) {
		if (inum > (sp->dp->fainode.di_size / FA_SIZ) - 1)
			inum = sp->validi;
		getino(sp, inum, &ibuf);
		if (ibuf.di_type == R_UNUSED) {
			return(inum);
		}
		if (++inum == save)
			break;
	}
	avail = sp->dp->fainode.di_size / FA_SIZ;
	if (off2addr(sp, FA_INUM, ip, ip->di_size, WRITE) < 0)
		return(0);
	getino(sp, avail, &ibuf);
	if (ibuf.di_type == R_UNUSED)
		return(avail);
	return(++avail);
}

/* ----------------------------- */
/* freeino - deallocate an inode */
/* ----------------------------- */

freeino(sp, inum, ip)
register struct sdfinfo *sp;
register int inum;
register struct dinode *ip;
{

#ifdef	DEBUG
	bugout("freeino(sdfinfo[%d], (i#) %d, (i_buf) 0x%08x)\n",
	    sdffileno(sp), inum, ip);
#endif	DEBUG

	ip->di_type = R_UNUSED;
	if (putino(sp, inum, ip) < 0)
		return(-1);
	return(0);
}

/* ------------------------------------------------ */
/* inobsum - sum up blocks in extent maps and inode */
/* ------------------------------------------------ */

inobsum(sp, inum)
register struct sdfinfo *sp;
register int inum;
{
	union fa_entry fabuf;
	register int i, bsum = 0, type, numextents;

#ifdef	DEBUG
	bugout("inobsum(sdfinfo[%d], (i#) inum)\n", sdffileno(sp), inum);
#endif	DEBUG

	getino(sp, inum, (struct dinode *) &fabuf);
	type = fabuf.e1.di_type;
	if (type == R_INODE) {
		numextents = fabuf.e1.di_exnum;
		bsum = 0;
	}
	else {
		numextents = fabuf.e2.e_exnum;
		bsum = fabuf.e2.e_boffset;
	}
	for (i = 0; i < numextents; i++)
		if (type == R_INODE)
			bsum += fabuf.e1.di_extent[i].di_numblk;
		else bsum += fabuf.e2.e_extent[i].e_numblk;
	return(bsum);
}

/* --------------------------------- */
/* initem - initialize an extent map */
/* --------------------------------- */

initem(emp)
register struct em_rec *emp;
{
	register int i;

	emp->e_type = R_EM;
	emp->e_next = -1;
	emp->e_last = -1;
	emp->e_inode = -1;
	emp->e_exnum = 0;
	emp->e_boffset = 0;
	emp->e_res1 = 0;
	for (i = 0; i < 13; i++) {
		emp->e_extent[i].e_startblk = -1;
		emp->e_extent[i].e_numblk = 0;
	}
}
