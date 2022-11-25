/* @(#)  $Revision: 72.8 $ */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef	PURDUE_EE
extern	filepass;
#endif	PURDUE_EE

#include "dump.h"
#ifdef	hpux
#  include <sys/sysmacros.h>
#endif	hpux

extern int Cascade_disk;
extern int lgblk;

pass(fn, map)
	register int (*fn)();
	register char *map;
{
	register int bits;
	ino_t maxino;

	maxino = sblock->fs_ipg * sblock->fs_ncg - 1;
	for (ino = 0; ino < maxino; ) {
		if ((ino % NBBY) == 0) {
			bits = ~0;
			if (map != NULL)
				bits = *map++;
		}
		ino++;
		if (bits & 1)
			(*fn)(getino(ino));
		bits >>= 1;
	}
}

#if defined(TRUX) && defined(B1)
static int q_asked = 0;
#endif 


mark(ip)
	struct dinode *ip;
{
	register int f;
	extern int anydskipped;

#ifdef ACLS
        if(ip->di_contin)
            aclcount++;
#endif ACLS

	f = ip->di_mode & IFMT;
	if (f == 0)
#if defined(TRUX) && defined(B1) 
        {
                msg_b1("mark: di_mode was NO good, ino %d\n",ino);
                return;
        }
#else /*(TRUX) && (B1) */

                return;
#endif /*(TRUX) && (B1) */


#if defined(TRUX) && defined(B1)
	if (it_is_b1fs & !pipeout) { 
		if (f == IFDIR) 
			msg_b1("mark: ino was a DIR ino %d\n",ino);
		else
			msg_b1("mark: ino was a FILE ino %d\n",ino);
		/* Check to see that the level of this inode falls 
                 * within the tape range. If not, don't put it on the bitmap */ 
  		if (!remote_tape) {
			if (check_range(ip) == 0 ){
				msg_b1("mark: out of range ip %x ino %d\n",ip,ino);
				if (!q_asked)
if (!query("Some files will not be exported. Check the device. Do you want to continue?"))
					dumpabort();
				else
					msg("continuing with dump ...\n");
					
				q_asked = 1; /* so the Q? is asked only once */
	  			return;	/* Skip this inode, go for the next */
			}
		} else
		if(compare_irs(ip) == 0){
			msg_b1("mark: Remote out of range ip %x ino %d\n",ip, ino);
			return;		/* Skip this inode, go for the next */
		}
	}
	/* if not a b1 inode, then check to see at what level it is mounted */
#endif /*(TRUX) && (B1) */

	BIS(ino, clrmap);
	if (f == IFDIR)
		BIS(ino, dirmap);
	if ((ip->di_mtime >= spcl.c_ddate || ip->di_ctime >= spcl.c_ddate) &&
	    !BIT(ino, nodmap)) {
#ifdef	PURDUE_EE
		if (ip->di_mtime >= spcl.c_date || ip->di_ctime >= spcl.c_date){
			if (f != IFDIR && f != IFLNK && f != IFCHR && f != IFBLK) {
				return;
			}
		}
#endif	PURDUE_EE
		BIS(ino, nodmap);
		if (f != IFREG && f != IFDIR && f != IFLNK && f != IFNWK) {
			esize += 1;
			return;
		}
		est(ip);
	} else if (f == IFDIR)
		anydskipped = 1;
}

add(ip)
	register struct dinode *ip;
{
	register int i;
	long filesize;

#if defined(TRUX) && defined(B1)
	msg_b1("add: ino %d\n", ino);
	if(BIT(ino, nodmap)){
		msg_b1("add: returned here, ino %d\n", ino);
		return;
	}
#else
	if(BIT(ino, nodmap))
		return;
#endif
	nsubdir = 0;
	dadded = 0;
	filesize = ip->di_size;
	for (i = 0; i < NDADDR; i++) {
		if (ip->di_db[i] != 0)
			dsrch(ip->di_db[i], dblksize(sblock, ip, i), filesize);
		filesize -= sblock->fs_bsize;
	}
	for (i = 0; i < NIADDR; i++) {
		if (ip->di_ib[i] != 0)
			indir(ip->di_ib[i], i, &filesize);
	}
	if(dadded) {
		nadded++;
		if (!BIT(ino, nodmap)) {
#if defined(TRUX) && defined(B1)

			if (it_is_b1fs & !pipeout) { 
				msg_b1("add: dadded %d, ino %d\n", dadded, ino);
/* Check to see that the level of this inode falls 
 * within the tape range. If not, don't put it on the bitmap
 */
  				if (!remote_tape) {
					if (check_range(ip) == 0 ){
			msg_b1("add: out of range ip %x ino %d\n", ip, ino);
	  					return;	/* Skip this inode */
					}
				} else  
					if(compare_irs(ip) == 0){
			msg_b1("add: Remote out of range ip %x ino %d\n", ip, ino);
						return;
					}
/* if not a b1 inode, check what level it is mounted at */
			}
#endif /*(TRUX) && (B1) */
			BIS(ino, nodmap);
			est(ip);
		}
	}
	if(nsubdir == 0)
		if(!BIT(ino, nodmap))
			BIC(ino, dirmap);
}

indir(d, n, filesize)
	daddr_t d;
	int n, *filesize;
{
	register i;
	daddr_t	idblk[MAXNINDIR];

#if defined(TRUX) && defined(B1)
	msg_b1("indir: d %x n %d filesize %d\n", d, n, *filesize);
#endif
	bread(fsbtodb(sblock, d), (char *)idblk, sblock->fs_bsize);
	if(n <= 0) {
		for(i=0; i < NINDIR(sblock); i++) {
			d = idblk[i];
			if(d != 0)
				dsrch(d, sblock->fs_bsize, *filesize);
			*filesize -= sblock->fs_bsize;
		}
	} else {
		n--;
		for(i=0; i < NINDIR(sblock); i++) {
			d = idblk[i];
			if(d != 0)
				indir(d, n, filesize);
		}
	}
}

dirdump(ip)
	
	struct dinode *ip;
{ 
#if defined(TRUX) && defined(B1)
	msg_b1("dirdump: ino %d\n", ino);
#endif
	/* watchout for dir inodes deleted and maybe reallocated */
	if ((ip->di_mode & IFMT) != IFDIR)
		return;
	dump(ip);
}

dump(ip)
	
	struct dinode *ip;
{
	register int i;
	long size;

	if(newtape) {
		newtape = 0;
		bitmap(nodmap, TS_BITS);
	}
	BIC(ino, nodmap);

#if defined(TRUX) && defined(B1)
	msg_b1("dump: newtape %d ino %d\n", newtape, ino);
	if (it_is_b1fs){
		spcl.udino.s_dinode = *(struct sec_dinode *)ip;
		spcl.c_type = TS_SEC_INODE;
	}
	else{
		spcl.udino.c_dinode = *ip;
		spcl.c_type = TS_INODE;
	}
#else /* TRUX & B1 */
	spcl.c_dinode = *ip;
	spcl.c_type = TS_INODE;
#endif /* TRUX & B1 */
	spcl.c_count = 0;
	i = ip->di_mode & IFMT;
	if (i == 0) /* free inode */
		return;
#ifdef	PURDUE_EE
	if (ip->di_mtime >= spcl.c_date || ip->di_ctime >= spcl.c_date) {
		if (filepass) {
			if (i != IFDIR && i != IFLNK && i != IFCHR && i != IFBLK) {
				return;
			}
		}
	}
#endif	PURDUE_EE
	if ((i != IFDIR && i != IFREG && i != IFLNK && i != IFNWK) || ip->di_size == 0) {
		spclrec();
		return;
	}
#ifdef IC_FASTLINK
	/*
	 * Check to see if this is a "fast" symbolic link.  These
	 * have the pathname of the link stored in the space that
	 * is usually used for the direct and indirect block pointers.
	 *
	 * If this is a fast symlink, we write data to the tape so
	 * that the result looks just like a "normal" symlink.  This
	 * means that no changes are required to restore(1m) to support
	 * fast symlinks.
	 */
	if (i == IFLNK && (ip->di_flags & IC_FASTLINK) != 0) {
		char tbuf[TP_BSIZE];

		/*
		 * copy the data to a tape block and write out the
		 * block.
		 */
		bzero(tbuf, sizeof tbuf);
		strncpy(tbuf, ip->di_symlink, sizeof ip->di_symlink);

		/*
		 * Mark that the data block pointer is valid and
		 * write out the TS_INODE/TS_ADDR record.
		 * Finally, write the data block for this symlink
		 * path.
		 */
		spcl.c_addr[0] = 1;
		spcl.c_count = 1;
		spclrec();

		taprec(tbuf);
		return;
	}
#endif /* IC_FASTLINK */
	if (ip->di_size > NDADDR * sblock->fs_bsize)
		i = NDADDR * sblock->fs_frag;
	else
		i = howmany(ip->di_size, sblock->fs_fsize);
	blksout(&ip->di_db[0], i);
	size = ip->di_size - NDADDR * sblock->fs_bsize;
	if (size <= 0)
		return;
	for (i = 0; i < NIADDR; i++) {
		dmpindir(ip->di_ib[i], i, &size);
		if (size <= 0)
			return;
	}
}

dmpindir(blk, lvl, size)
	daddr_t blk;
	int lvl;
	long *size;
{
	int i, cnt;
	daddr_t idblk[MAXNINDIR];

#if defined(TRUX) && defined(B1)
	msg_b1("dmpindir: blk %x lvl %d size %l\n", blk, lvl, *size);
#endif
	if (blk != 0)
		bread(fsbtodb(sblock, blk), (char *)idblk, sblock->fs_bsize);
	else
		bzero(idblk, sblock->fs_bsize);
	if (lvl <= 0) {
		if (*size < NINDIR(sblock) * sblock->fs_bsize)
			cnt = howmany(*size, sblock->fs_fsize);
		else
			cnt = NINDIR(sblock) * sblock->fs_frag;
		*size -= NINDIR(sblock) * sblock->fs_bsize;
		blksout(&idblk[0], cnt);
		return;
	}
	lvl--;
	for (i = 0; i < NINDIR(sblock); i++) {
		dmpindir(idblk[i], lvl, size);
		if (*size <= 0)
			return;
	}
}

blksout(blkp, frags)
	daddr_t *blkp;
	int frags;
{
	int i, j, count, blks, tbperdb;

	blks = howmany(frags * sblock->fs_fsize, TP_BSIZE);
	tbperdb = sblock->fs_bsize / TP_BSIZE;
	for (i = 0; i < blks; i += TP_NINDIR) {
		if (i + TP_NINDIR > blks)
			count = blks;
		else
			count = i + TP_NINDIR;
		for (j = i; j < count; j++)
			if (blkp[j / tbperdb] != 0)
				spcl.c_addr[j - i] = 1;
			else
				spcl.c_addr[j - i] = 0;
		spcl.c_count = count - i;
		spclrec();
		for (j = i; j < count; j += tbperdb)
			if (blkp[j / tbperdb] != 0)
				if (j + tbperdb <= count)
				{
					dmpblk(blkp[j / tbperdb],
					    sblock->fs_bsize);
					    }
				else
				{
					dmpblk(blkp[j / tbperdb],
					    (count - j) * TP_BSIZE);
					    }
		spcl.c_type = TS_ADDR;
	}
}

bitmap(map, typ)
	char *map;
{
	register i;
	char *cp;

	spcl.c_type = typ;
	spcl.c_count = howmany(msiz * sizeof(map[0]), TP_BSIZE);
	spclrec();
	for (i = 0, cp = map; i < spcl.c_count; i++, cp += TP_BSIZE)
		taprec(cp);
}

spclrec()
{
	register int s, i, *ip;

	spcl.c_inumber = ino;
#if defined(TRUX) && defined(B1)
	if (it_is_b1fs)
		spcl.c_magic = BLS_B1_MAGIC;
	else
		spcl.c_magic = BLS_C2_MAGIC;
	msg_b1("spclrec: spcl.c_type %d ino %d\n", spcl.c_type, ino);
#else  /*(TRUX) && (B1)*/
	spcl.c_magic = NFS_MAGIC;
#endif /*(TRUX) && (B1)*/
	spcl.c_checksum = 0;
	ip = (int *)&spcl;
	s = 0;
	i = sizeof(union u_spcl) / (4*sizeof(int));
	while (--i >= 0) {
		s += *ip++; s += *ip++;
		s += *ip++; s += *ip++;
	}
	spcl.c_checksum = CHECKSUM - s;
	taprec((char *)&spcl);
}

dsrch(d, size, filesize)
	daddr_t d;
	int size, filesize;
{
#if defined(TRUX) && defined(B1)
	register struct dirent *dp;
#else /*defined(TRUX) && defined(B1)*/
	register struct direct *dp;
#endif /*defined(TRUX) && defined(B1)*/
	long loc;
	char dblk[MAXBSIZE];

#if defined(TRUX) && defined(B1) 
	msg_b1("dsrch:d=%x size=%d filesize=%d dadded=%d nsubdir=%d\n",
   		d, size, filesize, dadded, nsubdir);
#endif
	if(dadded)
		return;
	if (filesize > size)
		filesize = size;
#ifdef	hpux
	/* 
	 * hpux needs a little help here, bread MUST be called with a count
	 * that is a multiple of DEV_BSIZE but no more than MAXBSIZE
	 */
	bread(fsbtodb(sblock, d), dblk, (filesize ==  MAXBSIZE ? MAXBSIZE : (((filesize / DEV_BSIZE) + 1) * DEV_BSIZE)));
#else	hpux
	bread(fsbtodb(sblock, d), dblk, filesize);
#endif	hpux
	for (loc = 0; loc < filesize; ) {
		dp = (struct direct *)(dblk + loc);
		if (dp->d_reclen == 0) {
			msg("corrupted directory, inumber %d\n", ino);
			break;
		}
		loc += dp->d_reclen;
		if(dp->d_ino == 0)
			continue;
		if(dp->d_name[0] == '.') {
			if(dp->d_name[1] == '\0')
				continue;
			if(dp->d_name[1] == '.' && dp->d_name[2] == '\0')
				continue;
		}
		if(BIT(dp->d_ino, nodmap)) {
			dadded++;
#if defined(TRUX) && defined(B1) 
			msg_b1("dsrch:dadded now=%d d_ino=%d\n", dadded,dp->d_ino);
#endif
			return;
		}
		if(BIT(dp->d_ino, dirmap))
			nsubdir++;
#if defined(TRUX) && defined(B1) 
		msg_b1("dsrch:nsubdir=%d d_ino=%d\n", nsubdir, dp->d_ino);
#endif
	}
}
struct dinode *
getino(ino)
	daddr_t ino;
{

	static struct dinode itab[MAXINOPB];
#if defined(ACLS) || defined (CNODE_DEV) 	/* ACLS and B1 are Mexclusive */
	struct dinode *p;			/* temp pointer to inode */
#endif

#if defined(TRUX) && defined(B1)
/* getino() is a kludge since SecureWare did not put the inode in a consecutive
 * order in each inode block, starting in the second inode group !? I donno why.
 * Instead they put them in according to itoo() offset. Hence this code 
 * looks a lot messier than it should.
 */
	static daddr_t minino[(MAXIPG/10)];  /* should be plenty */
	static daddr_t maxino[(MAXIPG/10)];
	static int cg,i; 	/* cylgrp#. So min/maxino not recalculated everytime*/
	static struct sec_dinode secitab[B1MAXINOPB];
	int j, maxi;

	if ((sblock->fs_ipg % INOPB(sblock)) == 0) /*!0 for SecureWare*/
		maxi = (sblock->fs_ipg/INOPB(sblock))-1; 
	else
		maxi = sblock->fs_ipg/INOPB(sblock); 

	if (ino >= minino[i] && ino < maxino[i]) {

#else /* TRUX & B1 */
	static daddr_t minino, maxino;

	if (ino >= minino && ino < maxino) {
#endif /* TRUX & B1 */
#ifdef ACLS
                p = &itab[ino - minino];        /* mark cont inode so that */
                if(p->di_contin)                /* ACLS will not be confused */
                    p->di_contin = -1;          /* with a pointer to data */
#endif ACLS

#ifdef CNODE_DEV
#if defined(TRUX) && defined(B1)
		if (ino >= sblock->fs_ipg) 	
			p = (it_is_b1fs ? &secitab[itoo(sblock,ino)] : &itab[itoo(sblock,ino)]);	
		else				/* if cg = 0 */
			p = (it_is_b1fs ? &secitab[ino - minino[i]] : &itab[ino - minino[i]]);	
		msg_b1("getino: offset %d c %d mode %o ino %d\n",
		 itoo(sblock,ino), itog(sblock,ino), 
		 (it_is_b1fs ? secitab[itoo(sblock,ino)].di_node.di_mode : itab[itoo(sblock,ino)].di_mode), ino);
#else /* TRUX & B1 */
		p = &itab[ino - minino];	/* mark CSD */
#endif /* TRUX & B1 */
		if(((p->di_mode & IFMT) == IFCHR) ||
		   ((p->di_mode & IFMT) == IFBLK)) {
		    p->di_rsite += CSD_MAGIC;	/* 0 - 255 are legal values */
		}
#endif /* CNODE_DEV */

#if defined(TRUX) && defined(B1)
		if (ino >= sblock->fs_ipg) 	
			return(it_is_b1fs ? &secitab[itoo(sblock,ino)] : &itab[itoo(sblock,ino)]);	
		else				/* if cg = 0 */
			return(it_is_b1fs ? &secitab[ino - minino[i]] : &itab[ino - minino[i]]);
	}
	bread(fsbtodb(sblock, itod(sblock, ino)), (it_is_b1fs?secitab:itab), sblock->fs_bsize);

/* calculate the minino and maxino arrays */
  {
  /* getino starts with ino=1. If this changes I'll BREAK. Or change below */
	if (itog(sblock,ino) != cg || ino == 1){ 
		cg = itog(sblock,ino);

		minino[0] = (sblock->fs_ipg * cg);
		maxino[maxi] = (sblock->fs_ipg * (cg+1));
		for (j=0 ; j<  maxi ; j++){
			maxino[j] = minino[j] + INOPB(sblock);
			minino[j+1] = maxino[j];
		}
	}
  }
	for (j=0; j<=maxi; j++)
		if (ino < maxino[j])
			break;
	i = (j > maxi) ? maxi : j;
	msg_b1("getino: i %d maxino= %d minino= %d\n", i, maxino[i], minino[i]);
#else 	/* TRUX & B1 */
		return (&itab[ino - minino]);
	}
	bread(fsbtodb(sblock, itod(sblock, ino)), itab, sblock->fs_bsize);
	minino = ino - (ino % INOPB(sblock));
	maxino = minino + INOPB(sblock);
#endif 	/* TRUX & B1 */


#ifdef ACLS
        p = &itab[ino - minino];
        if(p->di_contin)
            p->di_contin = -1;
#endif ACLS
#ifdef CNODE_DEV
#if defined(TRUX) && defined(B1)
	if (ino >= sblock->fs_ipg) 	
		p = (it_is_b1fs ? &secitab[itoo(sblock,ino)] : &itab[itoo(sblock,ino)]);	
	else				/* if cg = 0 */
		p = (it_is_b1fs ? &secitab[ino - minino[i]] : &itab[ino - minino[i]]);	
#else /* TRUX & B1 */
	p = &itab[ino - minino];	/* mark CSD */
#endif /* TRUX & B1 */
        if(((p->di_mode & IFMT) == IFCHR) ||
           ((p->di_mode & IFMT) == IFBLK)) {
            p->di_rsite += CSD_MAGIC;
        }
#endif /* CNODE_DEV */
#if defined(TRUX) && defined(B1)
	msg_b1("getino: offset %d c %d mode %o ino %d\n",
	 itoo(sblock,ino), itog(sblock,ino), 
	 (it_is_b1fs ? secitab[itoo(sblock,ino)].di_node.di_mode : itab[itoo(sblock,ino)].di_mode), ino);
	if (ino >= sblock->fs_ipg) 	
		return(it_is_b1fs ? &secitab[itoo(sblock,ino)] : &itab[itoo(sblock,ino)]);	
	else				/* if cg = 0 */
		return(it_is_b1fs ? &secitab[ino - minino[i]] : &itab[ino - minino[i]]);
#else /* TRUX & B1 */
		return (&itab[ino - minino]);
#endif /* TRUX & B1 */
}

int	breaderrors = 0;		
#define	BREADEMAX 32

#define  I_BUFSIZE 8*DEV_BSIZE  /* 8 K bytes */


bread(da, ba, cnt)
	daddr_t da;
	char *ba;
	int	cnt;	
{
	int n = 0;

if (Cascade_disk) {
	int cnt1 = 0;
	int cnt2 = 0;
	int cnt3 = 0;
	daddr_t da2 = 0;
	daddr_t da3 = 0;

	char localBuf[I_BUFSIZE];

	cnt1 = cnt/DEV_BSIZE;
	cnt2 = cnt1%lgblk;
	cnt3 = cnt1/lgblk;

	da2 = da%lgblk;
	da3 = da/lgblk;

	if (lseek(fi, (long)(da3*lgblk*DEV_BSIZE), 0) == -1){
		msg("bread: lseek fails\n");
	}

	if (da2) {
		if (cnt2 > (lgblk-da2)) {
			n = read(fi, localBuf, lgblk*DEV_BSIZE);
			memcpy(ba, localBuf+da2*DEV_BSIZE,
					(lgblk-da2)*DEV_BSIZE);
			n = read(fi, ba+(lgblk-da2)*DEV_BSIZE,
					cnt3*lgblk*DEV_BSIZE);
			n = read(fi, localBuf, lgblk*DEV_BSIZE);
			memcpy(ba+((cnt3+1)*lgblk-da2)*DEV_BSIZE,
				localBuf, (cnt2+da2-lgblk)*DEV_BSIZE);
		} else if (cnt2 == (lgblk-da2)) {
			n = read(fi, localBuf, lgblk*DEV_BSIZE);
			memcpy(ba, localBuf+da2*DEV_BSIZE, cnt2*DEV_BSIZE);
			n = read(fi, ba+cnt2*DEV_BSIZE, cnt3*lgblk*DEV_BSIZE);
		} else /* cnt2 < (lgblk-da2) */
			if(cnt3) {
				n = read(fi, localBuf, lgblk*DEV_BSIZE);
				memcpy(ba, localBuf+da2*DEV_BSIZE,
					(lgblk-da2)*DEV_BSIZE);
				n = read(fi, ba+(lgblk-da2)*DEV_BSIZE,
					(cnt3-1)*lgblk*DEV_BSIZE);
				n = read(fi, localBuf, lgblk*DEV_BSIZE);
				memcpy(ba+(cnt3*lgblk-da2)*DEV_BSIZE,
					localBuf, (cnt2+da2)*DEV_BSIZE);
			} else
				if (cnt2) {
					n = read(fi, localBuf,
							lgblk*DEV_BSIZE);
					memcpy(ba, localBuf+da2*DEV_BSIZE,
							cnt2*DEV_BSIZE);
				}
	} else {
		n = read(fi, ba, cnt3*lgblk*DEV_BSIZE);
		if(cnt2) {
			n = read(fi, localBuf, lgblk*DEV_BSIZE);
			memcpy(ba+cnt3*lgblk*DEV_BSIZE, localBuf,
							cnt2*DEV_BSIZE);
		}
	}
	if ( n == -1 ) {
		broadcast("DUMP IS AILING!\n");
		msg("Cascade disk read error.\n");
		msg("This is an unrecoverable error.\n");
		dumpabort();
	}
	return;
}

loop:
	if (lseek(fi, (long)(da * DEV_BSIZE), 0) == -1){
		msg("bread: lseek fails\n");
	}
	n = read(fi, ba, cnt);
#if defined(TRUX) && defined(B1) 
	msg_b1("bread:from %s [block %d]: count=%d, got=%d\n",disk, da, cnt, n);
	msg_b1("bread:itog=%d, cgbase=%d, cgstart=%d\n\t, cgimin=%d, itod= %d\n"
	, itog(sblock,ino),cgbase(sblock,itog(sblock,ino)), cgstart(sblock,itog(sblock,ino)), cgimin(sblock,itog(sblock,ino)), itod(sblock,ino));
#endif /* TRUX && B1 */
	if (n == cnt)
		return;
	if (da + (cnt / DEV_BSIZE) > fsbtodb(sblock, sblock->fs_size)) {
#if defined(TRUX) && defined(B1) 
	msg_b1("bread:cnt!=n, from %s [block %d]: count=%d, got=%d\n",disk, da, cnt, n);
#endif
		/*
		 * Trying to read the final fragment.
		 *
		 * NB - dump only works in TP_BSIZE blocks, hence
		 * rounds DEV_BSIZE fragments up to TP_BSIZE pieces.
		 * It should be smarter about not actually trying to
		 * read more than it can get, but for the time being
		 * we punt and scale back the read only when it gets
		 * us into trouble. (mkm 9/25/83)
		 */
		cnt -= DEV_BSIZE;
		goto loop;
	}
	msg("(This should not happen)bread from %s [block %d]: count=%d, got=%d\n",
		disk, da, cnt, n);
	if (++breaderrors > BREADEMAX){
		msg("More than %d block read errors from %s\n",
			BREADEMAX, disk);
		broadcast("DUMP IS AILING!\n");
		msg("This is an unrecoverable error.\n");
		if (!query("Do you want to attempt to continue?")){
			dumpabort();
			/*NOTREACHED*/
		} else
			breaderrors = 0;
	}
}

#if defined(TRUX) && defined(B1)

int mand_tag=1; 	/* hard coded, change later */

/* get from the ip to the file's ir, determine whether this file should be
 * exported or not. Returns 1, YES (export it). 0, NO.
 */
check_range(ip)
struct sec_dinode *ip;
{

/* Enhance: the b1buff has to be parsed , it is hardcoded for now*/

/*      if (!mand_tag_to_ir(ip->di_tag[b1buff->first_obj_tag], fir)){ */

        if (ip->di_tag[mand_tag] == SEC_WILDCARD_TAG_VALUE)   /* == 0, so export it */
                {mand_free_ir(fir); return(1);}
        if (mand_tag_to_ir(&(ip->di_tag[mand_tag]), fir) == 0){
                mand_free_ir(fir);
                dumpabort();    /* Returned 0; Error couldn't get the IR */
	}
	if (cmp_dev_file_sl(fir) == 1)/* 0 means don't export this file */
		{mand_free_ir(fir); return(1);} 
	else if (cmp_dev_file_sl(fir) == 0)
		{mand_free_ir(fir); return(0);}
	dumpabort();
}

/*
 * comare device SL with the file SL. The file SL has to be between the 
 * device SL in order to export the file. 
 * 1 Yes it can be exported
 * 0 No 
 * 2 Something went wrong.
 */
cmp_dev_file_sl(file_ir)	
mand_ir_t *file_ir;
{
	mand_ir_t *tape_ir;
	int comp1, comp2;
	static mand_ir_t *tape_max_ir;
	static mand_ir_t *tape_min_ir;
	static int is_single = 0;


	/*********************************************
	 * Check the device sensitivity label range.
	 ********************************************/

	/* Is it a multi-level device? */

	if (ISBITSET(td->ufld.fd_assign,AUTH_DEV_MULTI)) {
	/* Can only assign it one way. */
		if (ISBITSET(td->ufld.fd_assign,AUTH_DEV_SINGLE))

                	return (2); /* or any number but 0 or 1. for error */

		/* uflg, flags assoc with this user */
                if (td->uflg.fg_max_sl)
                        tape_max_ir = td->ufld.fd_max_sl;

		/* sflg, flags assoc with system */
                else if (td->sflg.fg_max_sl) {
                        tape_max_ir = td->sfld.fd_max_sl;
                }
                else
			return (2); /* this will eventually cause a dumpabort */

                if (td->uflg.fg_min_sl)
                        tape_min_ir = td->ufld.fd_min_sl;
                else if (td->sflg.fg_min_sl) {
                        tape_min_ir = td->sfld.fd_min_sl;
                }
                else
			return (2);
	}
	/* Is it a single level device? */

	else if (ISBITSET(td->ufld.fd_assign,AUTH_DEV_SINGLE)) {
        	if (td->uflg.fg_cur_sl) {
                	tape_ir = td->ufld.fd_cur_sl;
                        is_single = 1;
                }
		/* Is it a system default single level? */

                else if (td->sflg.fg_cur_sl) {
                        tape_ir = td->sfld.fd_cur_sl;
                        is_single = 1;
		}
		else
			return (2);
	}

          /* The SSO must specify the device in one of the options above, else
           * we have an error, so notify the user and exit.
           */
		else
                	return (2);

          /* If single-level, only files that match the designated level
           * exactly can be exported.  Restrict the min/max_ir to the
           * designated device level.
           */
	if (is_single) {
        	tape_max_ir = tape_ir;
		tape_min_ir = tape_ir;
	}
	/*
	 * Now make sure that the specified sensitivity range is valid.
	 */ 
	 /* No wildcards allowed. */

	if(tape_max_ir == (mand_ir_t *)0)
		return (2);
	if(tape_min_ir == (mand_ir_t *)0)
		return (2);

        /* Determine the relationship between the file clearance and
         * the device.
         */

	comp1 = mand_ir_relationship(file_ir,tape_max_ir);

	/* SINGLE LEVEL case. See if file equals the dump tape. */
	if (is_single && !(comp1 & MAND_EQUAL))
		return(0); /* not the same level as the the single level tape */
	else if (is_single && (comp1 & MAND_EQUAL)) 
		return(1); /* ok file the same level as the tape */
	     else if(is_single)	
			return(2);
	/* done single level */

	/* See if dump tape maximum dominates the file. */

	if (!(comp1 & (MAND_EQUAL | MAND_ODOM)))
		return(0);
	else 
		return(1);

	/* See if dump tape minimum is dominated by the file. */

	comp2 = mand_ir_relationship(file_ir,tape_min_ir);
	if (!(comp2 & (MAND_EQUAL | MAND_SDOM)))
		return(0);
	else 
		return(1);

}
/* get from the ip to the file's ir, determine whether this file should be
 * exported or not. Returns 1, YES (export it). 0, NO.
 */
compare_irs(ip)
struct sec_dinode *ip;
{
	register int decision1;
	register int decision2;

	if (!remote_tape){
	if (mand_tag_to_ir(&(ip->di_tag[mand_tag]), fir) == 0)
		dumpabort(); 	/* Returned 0; Error couldn't get the IR */

	decision1 = mand_ir_relationship(rmttape_max_ir, fir);
	decision2 = mand_ir_relationship(rmttape_min_ir, fir);

	if (((decision1 & MAND_EQUAL) != 0 ) || ((decision1 & MAND_SDOM) != 0)){
	msg_b1("compare_irs: subject (tape) dominates object (file) or they are equal.\n");
		return(1);
	}
	if (((decision1 & MAND_ODOM) != 0 ) || ((decision1 & MAND_INCOMP)!= 0)){
	msg_b1("compare_irs: object (file) dominates subject (tape) or they are incomparable.\n");
		return(0);
	}

	if (((decision2 & MAND_EQUAL) != 0 ) || ((decision2 & MAND_ODOM) != 0)){
	msg_b1("compare_irs: object (file) dominates subject (tape) or they are equal.\n");
		return(1);
	}
	if (((decision2 & MAND_SDOM) != 0 ) || ((decision2 & MAND_INCOMP)!= 0)){
	msg_b1("compare_irs: subject (tape) dominates object (file) or they are incomparable.\n");
		return(0);
	}
	} else return(1);
}
#endif /*(TRUX) && (B1) */
