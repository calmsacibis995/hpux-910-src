/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/scan.c,v $
 * $Revision: 1.43.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:31:49 $
 */


/*
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/


#include "inc.h"
#include "defs.h"
#include "types.h"
#include "externs.h"

bfreelist_check ()
{
	register struct buf *vbp, *pbp;
	register int i,skip,indx;
	extern dumpbuf();
	extern int logbuf();
	unsigned bfreelistCnt[BQUEUES];
	struct buf dummy;
	struct buf *bp = &dummy;
	unsigned absaddr;

	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                   SCANNING BUFFER FREELIST                          *\n");
	fprintf(outf,"***********************************************************************\n\n");
	for (i = 0; i < BQUEUES; i++){
		bfreelistCnt[i]=0;
		fprintf(outf,"\n\nscanning ");
		fprintf(outf, bufnames[i+1]);

		vbp = bfreelist[i].av_forw;
		pbp = vbfreelist + i;
        	absaddr = getphyaddr ((unsigned) vbp);
        	longlseek(fcore, (long)clear(absaddr), 0);
        	if (longread(fcore, (char *)bp, sizeof (struct buf))
                	!= sizeof (struct buf)) {
            		perror("dqfree longread");
            		break;
		}

		/* check for empty list */
		if (vbp == pbp){
			fprintf(outf,"... ");
			fprintf(outf,bufnames[i+1]);
			fprintf(outf," empty \n");
			continue;
		}
		fprintf(outf,"\n");
		/* Check for bad pointers */

		if (bp->av_back != pbp){

			fprintf_err();
			fprintf(outf,"   bad av_back link\n");
			dumpbuf(" bad av_back ",bp, vbp);
			fprintf(outf,"   Scan terminating early\n");
			continue;
		}
		for (; bp;){
			skip = 0;
			if (Bflg){
				dumpbuf(" bfreelist ",bp, vbp);
				skip++;
			}
			bfreelistCnt[i]++;

			/* log use of buffer */
			if (logbuf(bp,vbp,i+1,BFREELIST)){
				fprintf(outf,"   scan terminating early\n");
				break;
			}

			/* Check for end of chain */
			if (bp->av_forw == (vbfreelist + i))
				break;

			/* get next entry */
			pbp = vbp;
			vbp = bp->av_forw;

        		absaddr = getphyaddr ((unsigned) vbp);
        		longlseek(fcore, (long)clear(absaddr), 0);
        		if (longread(fcore, (char *)bp, sizeof (struct buf))
                		!= sizeof (struct buf)) {
            			perror("dqfree longread");
            			break;
			}

			/* Check pointers */
			if (bp->av_back != pbp){

				fprintf_err();
				fprintf(outf,"   av_back pointer bogus\n");
				dumpbuf(" bad av_back ", bp, vbp);
				fprintf(outf,"   Scan terminating early\n");
				break;

			}

			if (skip)
				fprintf(outf,"\n");
		}
	}
	fprintf(outf, "\n\n-- freelist totals --\n");
	for(i=0;i < BQUEUES;i++) {
		fprintf(outf, "%8s: %5.5d entr%s.\n",
			bufnames[i+1],
			bfreelistCnt[i],
			bfreelistCnt[i] == 1 ? "y" : "ies");

	}

}



bhash_check()
{
	register struct buf *vbp, *pbp, *valid_headr;
	register int i,skip,indx;
	extern dumpbuf();
	extern int logbuf();
	unsigned bfreelistCnt[BQUEUES];
	struct buf dummy;
	struct buf *bp = &dummy;
	unsigned absaddr;
	unsigned bufhashCnt;

	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                     SCANNING BUFFER HASHLIST                        *\n");
	fprintf(outf,"***********************************************************************\n\n");
	bufhash[BUFHSZ].b_forw = bfreelist[BAGE-1].b_forw ;
	bufhash[BUFHSZ].b_back = bfreelist[BAGE-1].b_back ;
	bufhash[BUFHSZ+1].b_forw = bfreelist[BEMPTY-1].b_forw;
	bufhash[BUFHSZ+1].b_back = bfreelist[BEMPTY-1].b_back;
	for (i = 0; i < NBUFHASH; i++){
		fprintf(outf,"\n\nscanning ");
		fprintf(outf, bufnames[BHASH]);
		if (i == BUFHSZ){
			fprintf(outf, " bfreelist[BAGE] ");
		} else if (i == (BUFHSZ + 1)){
			fprintf(outf, " bfreelist[BEMPTY] ");
		} 
		vbp = bufhash[i].b_forw;
		pbp = vbufhash + i;

		if (i == BUFHSZ){
			valid_headr = (struct buf *)(vbfreelist + (BAGE-1));
		} else if (i == (BUFHSZ + 1)){
			valid_headr = (struct buf *)(vbfreelist + (BEMPTY-1));
		} else {
			valid_headr = (struct buf *)(vbufhash + i);

		}

		/* check for empty list */
		if (bufhash[i].b_forw == valid_headr){
			fprintf(outf," %d...  empty \n",i);
			continue;
		}
		fprintf(outf," %d \n",i);
		/* Check for bad pointers */

        	absaddr = getphyaddr ((unsigned) vbp);
        	longlseek(fcore, (long)clear(absaddr), 0);
        	if (longread(fcore, (char *)bp, sizeof (struct buf))
               		!= sizeof (struct buf)) {
            		perror("dqfree longread");
            		break;
		}
		if (bp->b_back != valid_headr){

			fprintf_err();
			fprintf(outf,"   bad b_back link\n");
			dumpbuf(" bad b_back ",bp, vbp);
			fprintf(outf,"   Scan terminating early\n");
			continue;
		}
		bufhashCnt = 0;
		for (; bp;){
			skip = 0;
			if (Bflg){
				dumpbuf(" bufhash  ",bp,vbp);
				skip++;
			}
			bufhashCnt++;

			/* log use of buffer */
			if (logbuf(bp,vbp,BHASH,BHASH)){

				fprintf(outf,"   Scan terminating early\n");
				break;
			}
			/* Check for end of chain */
			if (bp->b_forw == valid_headr)
				break;

			/* get next entry */
			pbp = vbp;
			vbp = bp->b_forw;
        		absaddr = getphyaddr ((unsigned) vbp);
        		longlseek(fcore, (long)clear(absaddr), 0);
        		if (longread(fcore, (char *)bp, sizeof (struct buf))
               			!= sizeof (struct buf)) {
            			perror("dqfree longread");
            			break;
			}
			/* Check pointers */
			if (bp->b_back != pbp){

				fprintf_err();
				fprintf(outf,"   b_back pointer bogus\n");
				dumpbuf(" bad b_back ", bp, vbp);
				fprintf(outf,"   Scan terminating early\n");
				break;

			}

			if (skip)
				fprintf(outf,"\n");
		}
		fprintf(outf, "  %d entr%s.\n", bufhashCnt,
			bufhashCnt == 1 ? "y" : "ies");
	}

}

/*  Check buffers for consistency in chains, and optionally to print out
 *  the contents
 */
bufcheck()
{
	register struct buf *bp, *pbp;
	register int i,skip,indx;
	extern dumpbuf();
	extern int logswbuf(), logbuf();

	if (!(bflg))
		return(0);


	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                  SCANNING PAGING SWAP HEADRS                        *\n");
	fprintf(outf,"***********************************************************************\n\n");

	if (bswlist.av_forw == NULL){
		fprintf(outf,"   Swap headr list empty\n");
	} else {
	bp = &swbuf[bswlist.av_forw - vswbuf];
	if (((bp < swbuf) || (bp > &swbuf[nswbuf]))&&(bp != NULL)){

		fprintf_err();
		fprintf(outf,"   bswlist has bogus pointer!! %x\n",bswlist.av_forw);
	} else {
		for (;bp;){
			skip = 0;
			if (Bflg){
				dumpbuf(" bswlist ",bp, (bp-swbuf)+vswbuf);
				skip++;
			}

			/* Log swbuf, and check for duplictaes */
			if (logswbuf(bp,BSWAP)){

				fprintf(outf,"   Scan terminating early\n");
				break;
			}

			/* Check for end of chain */
			if (bp->av_forw == NULL)
				break;

			pbp = bp;

			/* get next entry */
			bp = &swbuf[bp->av_forw - vswbuf];

			/* check for valid pointer */
			if ( (bp < swbuf) || (bp > &swbuf[nswbuf])){

				fprintf_err();
				fprintf(outf,"   swap buffer av_forw pointer out of range\n");
				if (!(Bflg))
					dumpbuf(" Bad pointer ",pbp, (pbp-swbuf)+vswbuf);

				fprintf(outf,"   Bswlist scan terminated early\n");
				break;

			}
			if (skip)
				fprintf(outf,"\n");
		}
	}

	}
	/* List out busy buffers */
	fprintf(outf,"\n\n   LISTING UNLINKED SWBUF BUFFERS\n");

	skip = 0;
	for (bp = swbuf; bp < &swbuf[nswbuf]; bp++){
		indx = bp - swbuf;
		if (swbufblk[indx].type == BFREE){
			dumpbuf(" Unlinked ", bp, (bp-swbuf)+ vswbuf);
			skip++;
		}
	}
	if (skip == 0)
		fprintf(outf,"   None found\n");

	bfreelist_check ();
	bhash_check ();

}


dqcheck ()
{
register int i;
register struct dqhead *dhp;
register struct dquot *dqp;
int count;
struct dquot dummy;
unsigned absaddr;

    if (!qflg) return;

    fprintf(outf,"\n\n\n***********************************************");
    fprintf(outf,"\n*	SCANNING INCORE DQUOTA HASH CHAINS    *");
    fprintf(outf,"\n***********************************************");


    for (i=0; i < NDQHASH; i++){
	dhp = &dqhead[i];

	fprintf(outf,"\nscanning dqhead[%d]",i);
	/* check for empty list */

	dqp = dhp->dqh_forw;
	if (dqp == (struct dquot *)(dhp - dqhead + vdqhead)){
	    fprintf(outf,"... ");
	    fprintf(outf," empty \n");
	    continue;
	}

	fprintf(outf,"\n");
	while (dqp != (struct dquot *)(dhp - dqhead + vdqhead)) {
	    absaddr = getphyaddr ((unsigned) dqp);
	    longlseek(fcore, (long)clear(absaddr), 0);
	    if (longread(fcore, (char *)&dummy, sizeof (struct dquot))
		!= sizeof (struct dquot)) {
	    	perror("dqhash longread");
	    	break;
	    }

	    dumpdquota (&dummy, dqp);
	    dqp = dummy.dq_forw;
	}

    }
    fprintf(outf,"\n\n\n***********************************************");
    fprintf(outf,"\n*           SCANNING DQUOTA FREELIST          *");
    fprintf(outf,"\n***********************************************\n");

    count=0;
    fprintf(outf,"Free list at:0x%08x freef:0x%08x freeb:0x%08x\n",
    	vdqfree, dqfreelist.dq_freef, dqfreelist.dq_freeb);
    dqp = dqfreelist.dq_freef;

    while (dqp != vdqfree && (count++ < 16)) {
	absaddr = getphyaddr ((unsigned) dqp);
	longlseek(fcore, (long)clear(absaddr), 0);
	if (longread(fcore, (char *)&dummy, sizeof (struct dquot))
		!= sizeof (struct dquot)) {
	    perror("dqfree longread");
	    break;
	}
	dumpdquota (&dummy, dqp);
	dqp = dummy.dq_freef;
    }


    fprintf(outf,"\n\n\n***********************************************");
    fprintf(outf,"\n*          SCANNING DQUOTA RESERVED LIST      *");
    fprintf(outf,"\n***********************************************\n");

    count=0;

    fprintf (outf,"Reserved list at: 0x%08x freef: 0x%08x freeb: 0x%08x\n",
    	vdqresv, dqresvlist.dq_freef, dqresvlist.dq_freeb);
    dqp = dqresvlist.dq_freef;
    while (dqp != vdqresv && (count++ < 16)) {
	absaddr = getphyaddr ((unsigned) dqp);
	longlseek(fcore, (long)clear(absaddr), 0);
	if (longread(fcore, (char *)&dummy, sizeof (struct dquot))
		!= sizeof (struct dquot)) {
	    perror("dqfree longread");
	    break;
	}
	dumpdquota (&dummy, dqp);
	dqp = dummy.dq_freef;
    }
}



/* icheck, scan in core inode table looking for chain consistency, and
 * other various checks . (see loginode)
 */
icheck()
{
	register struct inode *ip, *pip, *valid_headr;
	register int i, skip, indx;
	extern dumpinode();
	extern int loginode();
	unsigned ifreelistCnt;
	unsigned ihashCnt;
	struct vnode *vp;
	if (!(iflg))
		return(0);

	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*               SCANNING INCORE INODE HASH CHAINS                     *\n");
	fprintf(outf,"***********************************************************************\n\n");
	for (i = 0; i < INOHSZ; i++){
		fprintf(outf,"\n\nscanning ihead[%d]",i);
		ip = &inode[(ihead[i].ih_chain[0] - vinode)];

		valid_headr = (struct inode *)(vihead + i);


		/* check for empty list */
		if (ihead[i].ih_chain[0] == valid_headr){
			fprintf(outf,"... ");
			fprintf(outf," empty \n");
			continue;
		}
		fprintf(outf,"\n");
		/* Check for bad pointers */

		if ((ip < inode) || (ip > &inode[ninode])){

			fprintf_err();
			fprintf(outf,"   inode headr bogus pointer 0x%08x\n",ihead[i].ih_chain);
			continue;
		}

		if (ip->i_chain[1] != valid_headr){

			fprintf_err();
			fprintf(outf,"   bad backward i_chain[1] link\n");
			dumpinode(" bad i_chain[1]",ip,inode,vinode);
			fprintf(outf,"   Scan terminating early\n");
			continue;
		}
		ihashCnt = 0;
		for (; ip;){
			skip = 0;
			if (Iflg){
				dumpinode(" hash ",ip,inode,vinode);
				skip++;
			}
			ihashCnt++;
			/* log use of buffer */
			if (loginode(ip,ICHAIN)){

				fprintf(outf,"   Scan terminating early\n");
				break;
			}
			/* Check for end of chain */
			if (ip->i_chain[0] == valid_headr)
				break;

#ifdef notdef
			/* Inode/Vnode cross-checks */
			vp = ITOV(ip);
			if ((vp->v_data != vinode + (ip - inode)) ||
			    (vp->v_v_op != nl[X_UFS_VNODEOPS].n_value)
			    (vp->v_rdev != ip->i_rdev) ||
#endif
			pip = ip;
			/* get next entry */
			ip = &inode[ip->i_chain[0] - vinode];
			/* Check pointers */
			if ((ip < inode) || (ip > &inode[ninode])){

				fprintf_err();
				fprintf(outf,"   forward i_chain bogus\n");
				if (!(Iflg))
					dumpinode(" bad forw i_chain[0]",pip,inode,vinode);
				fprintf(outf,"   Scan terminating early\n");
				break;
			} else if (ip->i_chain[1] != vinode + (pip - inode)){

				fprintf_err();
				fprintf(outf,"   i_chain[1] pointer bogus\n");
				dumpinode(" bad back i_chain[1]",ip,inode,vinode);
				fprintf(outf,"   Scan terminating early\n");
				break;

			}

			if (skip)
				fprintf(outf,"\n");
		}
		fprintf(outf, "  %d entr%s.\n", ihashCnt,
			ihashCnt == 1 ? "y" : "ies");
	}



	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*               SCANNING INCORE INODE FREELIST                        *\n");
	fprintf(outf,"***********************************************************************\n\n");
	ip = &inode[(ifreeh - vinode)];
	if (ifreeh == NULL){
		fprintf(outf,"... ");
		fprintf(outf," empty \n");
	} else {
		ifreelistCnt = 0;
		fprintf(outf,"\n");
		for (;ip;){
			/* Check for bad pointers */
			if ((ip < inode) || (ip > &inode[ninode])){

				fprintf_err();
				fprintf(outf,"   inode ifreeh bogus pointer 0x%08x\n",ifreeh);
				break;
			}

			skip = 0;
			if (Iflg){
				dumpinode(" free ",ip,inode,vinode);
				skip++;
			}
			ifreelistCnt++;

			/* log use of buffer */
			if (loginode(ip,IFREE)){
				fprintf(outf,"   Scan terminating early\n");
				break;
			}

			/* check for end */
			if (ip->i_freef == NULL)
				break;

			pip = ip;
			/* get next entry */
			ip = &inode[ip->i_freef - vinode];

			/* Check pointers */
			if ((ip < inode) || (ip > &inode[ninode])){
				fprintf(outf,"   forward i_free bogus\n");

					fprintf_err();
					if (!(Iflg))
						dumpinode(" bad freef ",pip,inode,vinode);
					fprintf(outf,"   Scan terminating early\n");
					break;
			 } else if (ip->i_freeb != &((vinode + (pip - inode))->i_freef)){

					fprintf_err();
					fprintf(outf,"   i_freeb pointer bogus\n");
					dumpinode(" bad freeb ",ip,inode,vinode);
					fprintf(outf,"   Scan terminating early\n");
					break;

			}

			if (skip)
				fprintf(outf,"\n");
		}
		fprintf(outf, "  %d entr%s.\n", ifreelistCnt,
			ifreelistCnt == 1 ? "y" : "ies");
	}

	/* list out unlinked inodes */
	fprintf(outf,"\n\n   LISTING UNLINKED INODE ENTRIES\n");
	skip = 0;
	for (ip = inode; ip < &inode[ninode]; ip++){
		indx = ip - inode;
		if (inodeblk[indx].list == IEMPTY){

			fprintf_err();
			dumpinode(" Unlinked ",ip,inode,vinode);
			skip++;
		}
	}
	if (skip == 0)
		fprintf(outf,"   None found\n");



}






#define RTABLE_FLAG Iflg

/*
 * rcheck, scan rnode hash table and rnode free list.
 */
rcheck()
{
	register struct rnode *vrp, *valid_headr;
	register int i;
	struct rnode rnode;
	extern dumprnode();
	extern int logrnode();
	unsigned rfreelistCnt;
	unsigned rhashCnt, TotRHashCnt = 0;

	if (!(RTABLE_FLAG))
		return(0);

	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*               SCANNING RNODE HASH TABLE                             *\n");
	fprintf(outf,"***********************************************************************\n\n");
	for (i = 0; i < RTABLESIZE; i++){
		fprintf(outf,"\n\nscanning rtable[%d]",i);
		vrp = rtable[i];

		/* check for empty list */
		if (vrp == NULL){
			fprintf(outf,"...  empty \n");
			continue;
		}
		fprintf(outf,"\n");

		rhashCnt = 0;
		for (; vrp; vrp = rnode.r_next) {
			if (getchunk(KERNELSPACE,vrp,&rnode,
				     sizeof(struct rnode),"rcheck")) {
				fprintf_err();
				fprintf(outf,"Aborting scan of this chain\n");
				break;
			}
			dumprnode(" hash ",&rnode,vrp);
			rhashCnt++;
			fprintf(outf,"\n");
		}
		fprintf(outf, "  %d entr%s.\n", rhashCnt,
			rhashCnt == 1 ? "y" : "ies");
		TotRHashCnt += rhashCnt;
	    }
	fprintf(outf,"\n  %d total hash entries.\n", TotRHashCnt);

	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*               SCANNING RNODE FREELIST                               *\n");
	fprintf(outf,"***********************************************************************\n\n");
	vrp = rpfreelist;
	if (vrp == NULL){
		fprintf(outf,"...  empty \n");
	} else {
		rfreelistCnt = 0;
		fprintf(outf,"\n");
		for (;vrp; vrp = rnode.r_next) {
			if (getchunk(KERNELSPACE,vrp,&rnode,sizeof (struct rnode),
				 "rcheck")) {
				fprintf_err();
				fprintf(outf,"Aborting scan of freelist.\n");
				break;
			}
			dumprnode(" free ",&rnode, vrp);
			rfreelistCnt++;
			fprintf(outf,"\n");
		}
		fprintf(outf, "  %d entr%s.\n", rfreelistCnt,
			rfreelistCnt == 1 ? "y" : "ies");
	}
}

#ifdef dont_do_for_now
/*
 * Rnode/vnode cross-checks and vnode validity check.
 */

rvcheck(vrp,rp)
struct rnode *vrp, *rp;
{
  register struct vnode *vp;

	vp = &(rp)->r_vnode;
	if (((vp->v_data) != vrp) ||
	    (vp->v_type & (VBLK|VCHR|VBAD|VNON|VFIFO|VFNWK)) ||
	    (vp->v_fstype != VNFS) ||
	    (!vp->v_count)){
		fprintf_err();
		dumprnode("Bad vnode in rnode", rp, vrp);
	}
}
#endif


dump_dnlcstats()
{
	fprintf(outf,"DNLC stats:\n");
	fprintf(outf,
		"  hits   0x%08x   purges     0x%08x  lru_empty  0x%08x\n",
		ncstats.hits, ncstats.purges, ncstats.lru_empty);
	fprintf(outf,
		"  misses 0x%08x   long_look  0x%08x  long_enter 0x%08x\n",
		ncstats.misses, ncstats.long_look, ncstats.long_enter);
#ifdef notdef
	fprintf(outf,
		"  enters 0x%08x   long_enter 0x%08x  dbl_enters 0x%08x\n",
		ncstats.enters, ncstats.long_enter, ncstats.dbl_enters);
#endif notdef
}

dnlc_check()
{
	register struct ncache *ncp, *nhp, *vncp;
	struct nc_hash *vhncp;
	struct nc_lru *vhhncp;
	int i, nchashCnt, TotNCHashCnt = 0, nclruCnt = 0;

	if (!(DNLC_FLAG))
		return(0);
	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*               SCANNING DNLC HASH TABLE                              *\n");
	fprintf(outf,"***********************************************************************\n\n");

	dump_dnlcstats();
	fprintf(outf, "\n\n");
	for (i = 0, vhncp = vnc_hash; i < NC_HASH_SIZE;
	     i++, vhncp++) {
		fprintf(outf, "scanning nc_hash[%d]", i);
		vncp = nc_hash[i].hash_next;
		if (nc_hash[i].hash_next == (struct ncache *)vhncp) {
			fprintf(outf, "...  empty\n");
			continue;
		}
		fprintf(outf, "\n");

		nchashCnt = 0;
		for (ncp = &ncache[vncp - vncache];
		     vncp != (struct ncache *)vhncp;
		     vncp = ncp->hash_next, ncp = &ncache[vncp - vncache]) {
			dumpncache("", ncp, vncp);
			nchashCnt++;
			fprintf(outf,"\n");
		}
		fprintf(outf, "  %d entr%s.\n\n", nchashCnt,
			nchashCnt == 1 ? "y" : "ies");
		TotNCHashCnt += nchashCnt;
	}
	fprintf(outf,"\n  %d total hash entries.\n", TotNCHashCnt);

	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*               SCANNING DNLC LRU LIST                                *\n");
	fprintf(outf,"***********************************************************************\n\n");

	vhhncp = vnc_lru;
	if (nc_lru.lru_next == (struct ncache *)vhhncp)
		fprintf(outf, "... empty\n");
	else {
		vncp = nc_lru.lru_next;
		for (ncp = &ncache[vncp - vncache];
		     vncp != (struct ncache *)vhhncp;
		     vncp = ncp->lru_next, ncp = &ncache[vncp - vncache]) {
			dumpncache("", ncp, vncp);
			nclruCnt++;
			fprintf(outf,"\n");
		}
		fprintf(outf, "  %d entr%s.\n", nclruCnt,
			nclruCnt == 1 ? "y" : "ies");
	}
}




#ifdef hp9000s800
/* For our purposes here, we're just trying to go from a physical page
 * to the virtual bits that are not contained within the PDE.  For a
 * non-IO page, that's easy---just return the pgtopde_table entry, and
 * you have the complete xpde, including the virtual bits in the low-order
 * 5 bits.  If it's an I/O page, we could return the entire xpde by doing
 * a look-up in the pde table and following down the chain.  But we're
 * really only interested virtual index bits, which are self-contained in
 * the page address.  Just return those.
 */
int
phys_page_to_xpde (phys)
	unsigned int phys;
{
	int xpde;

	if (ptob(phys) < 0xF0000000) {
		xpde = (int) pgtopde_table [phys];
	} else {
		xpde = phys & 0x1F;
	}
	return (xpde);
}

/* pdircheck, checks the consistency of the pdir chains */
pdircheck()
{
	register struct hpde *ht;
	register struct hpde *pd, *nextpd, *prevpd;
	register struct paginfo *zp;
	register int chaintype, error;
	extern struct hpde *base_pdir;
	extern struct hpde *max_pdir;
	/* MEMSTATS */
	int full_entry, half_entry;
	extern dump(), dumppde();
	int pde_vpage;
	int xpde_virt_bits;

	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                     SCANNING PDIR HASH CHAINS                       *\n");
	fprintf(outf,"***********************************************************************\n\n");

	/* MEMSTATS */
	full_entry = 0;
	half_entry = 0;

	/* Scan down hash table */
	for (ht = htbl; ht < &htbl[nhtbl]; ht++){
		/* MEMSTATS */
		if (vflg)
			fprintf(outf,"Scanning pdir hash chain htbl[%d]\n",ht - htbl);
		/* The beginning of the chain is always the valid portion */
		chaintype = ZVALID;

		pd = ht;

		/* MEMSTATS */

		while (1){
				      /* MEMSTATS */
		    if ((pd->pde_os & 3) != 0){
			  /* MEMSTATS */
			   /* Count full vs half full entries */
			   if ((pd->pde_os & 3) == 3)
				full_entry++;
			   else
				half_entry++;

			   xpde_virt_bits =
			       (int)phys_page_to_xpde(pd->pde_phys_e) & 0x1F;
			   pde_vpage = (pd->pde_page_e << 5) | xpde_virt_bits;

			   /* LOOK AT EVEN PAGE */
			   /* Separate the iopages from regular pages */
			   if ((ptob(pd->pde_phys_e) < 0xf0000000) && (pd->pde_phys_e > 0)){
				loghtbl1(pd->pde_space_e, pde_vpage);
				loghtbl2(pd->pde_space_e, pde_vpage);

				zp = &paginfo[pd->pde_phys_e];

				/* Ok we should only hit a physical page once */
				if (zp->z_pdirtype != ZINVALID){

					fprintf_err();
					fprintf(outf,"  Hash chains collide with previous entry\n");
					fprintf(outf,"  Your pdir chains are corrupt! \n");
					dump(zp);
					dumppde("pdir",pd,ht - htbl);
					fprintf(outf,"\n");
					break;
				}
				error = 0;

				/* Look for stranded buddy bit */
				if (((pd->pde_os & 3) != 3) && pd->pde_shadow_e){
					fprintf_err();
					fprintf(outf,"  Buddy set with no buddy\n");
					dump(zp);
					dumppde("pdir",pd,ht - htbl);
					fprintf(outf,"\n");
				}

				if (zp->z_virtual != 0){
					/* Check for address match */
					if (zp->z_space != pd->pde_space_e){

						fprintf(outf,"  Space mismatch in pdir\n");
						error++;
					}
					if (zp->z_virtual != pde_vpage) {

						fprintf(outf,"  Virtual page mismatch in pdir\n");
						fprintf(outf,"  zp->z_virtual 0x%X; pde_vpage 0x%X.  Even.\n",
						    zp->z_virtual, pde_vpage);
						error++;
					}
					if (zp->z_protid != (short)pd->pde_protid_e){

						fprintf(outf,"  Protid mismatch in pdir\n");
						error++;
					}


					if (error){

						fprintf_err();
						dump(zp);
						dumppde("pdir",pd,ht - htbl);
						fprintf(outf,"\n");
					}
				} else {
					/* No previous entry to compare..
					 * so record new info.
					 */
					zp->z_space = pd->pde_space_e;
					zp->z_virtual = pde_vpage;
					zp->z_protid = (short)pd->pde_protid_e;
				}
				zp->z_pdirtype = chaintype;
				zp->z_hashndx = ht - htbl;

			}

			xpde_virt_bits =
			    (int)phys_page_to_xpde(pd->pde_phys_o) & 0x1F;
			pde_vpage = ((pd->pde_page_o << 5) | xpde_virt_bits);

			/* LOOK AT ODD PAGE */
			/* Separate the iopages from regular pages */
			if ((ptob(pd->pde_phys_o) < 0xf0000000) && (pd->pde_phys_o > 0)){
				/* Only account fo this once */
				if ((pd->pde_os & 3) != 3){
					loghtbl1(pd->pde_space_o, pde_vpage);
					loghtbl2(pd->pde_space_o, pde_vpage);
				}

				zp = &paginfo[pd->pde_phys_o];
				/* Ok we should only hit a physical page once */
				if (zp->z_pdirtype != ZINVALID){

					fprintf_err();
					fprintf(outf,"  Hash chains collide with previous entry\n");
					fprintf(outf,"  Your pdir chains are corrupt! \n");
					dump(zp);
					dumppde("pdir",pd,ht - htbl);
					fprintf(outf,"\n");
					break;
				}
				error = 0;
				if (zp->z_virtual != 0){
					/* Check for address match */
					if (zp->z_space != pd->pde_space_o){

						fprintf(outf,"  Space mismatch in pdir\n");
						error++;
					}
					if (zp->z_virtual != pde_vpage) {

						fprintf(outf,"  Virtual page mismatch in pdir\n");
						fprintf(outf,"  zp->z_virtual 0x%X; pde_vpage 0x%X.  Odd.\n", zp->z_virtual, pde_vpage);
						error++;
					}
					if (zp->z_protid != (short)pd->pde_protid_o){

						fprintf(outf,"  Protid mismatch in pdir\n");
						error++;
					}


					if (error){

						fprintf_err();
						dump(zp);
						dumppde("pdir",pd,ht - htbl);
						fprintf(outf,"\n");
					}
				} else {
					/* No previous entry to compare..
					 * so record new info.
					 */
					zp->z_space = pd->pde_space_o;
					zp->z_virtual = pde_vpage;
					zp->z_protid = (short)pd->pde_protid_o;
				}
				zp->z_pdirtype = chaintype;
				zp->z_hashndx = ht - htbl;


			}
		    }


		    /* Just for grinns lets look down the rest of the
		     * chain looking for a duplicate space, vaddr tuple.
		     * That would be very bad, and we don't like
		     * to see that .
		     */

		     prevpd = pd;
		     nextpd = pd->pde_next;

		     while (nextpd) {
				/*
				 * Convert to internal address.
				 */
				if (nextpd < base_pdir || nextpd >= max_pdir) {
						fprintf_err();
						fprintf(outf, "Bad next %x\n",nextpd);
						fprintf(outf," Scan for duplicate mappings of same virtual address terminated\n");
						dumppde(" Bad next ",prevpd,ht - htbl);
						fprintf(outf,"\n");
						break;
				}
				if (getepde(nextpd, &tmppde)) {
					fprintf_err();
					fprintf(outf, "pdircheck: getedpe failed\n on 0x%x\n", nextpd);
					break;
				}
				nextpd = &tmppde;

				if (nextpd->pde_page_e == pd->pde_page_e &&
				    nextpd->pde_space_e == pd->pde_space_e &&
				    nextpd->pde_valid_e && pd->pde_valid_e) {
					fprintf_err();
					fprintf(outf,"  Same virtual address mapped to more then one pde (address aliasing)\n");
					dumppde("current pdir",pd,ht-htbl);
					dumppde("dup pdir",nextpd,ht-htbl);
					fprintf(outf,"\n");
				}
				prevpd = nextpd;
				nextpd = nextpd->pde_next;
		    }


		    /* Get next entry in chain */
		    prevpd=pd;
		    pd = pd->pde_next;
		    if (pd == 0)
			    break;

		    if (pd < base_pdir || pd >= max_pdir) {
			    fprintf_err();
			    fprintf(outf, "Bad next %x\n",pd);
			    fprintf(outf," Scan for duplicate mappings of same virtual address terminated\n");
			    dumppde(" Bad next ",prevpd,ht - htbl);
			    fprintf(outf,"\n");
			    break;
		    }
		    if (getepde(nextpd, &tmppde)) {
			    fprintf_err();
			    fprintf(outf, "pdircheck: getedpe failed\n on 0x%x\n", pd);
			    break;
		    }
		    pd = &tmppde;
		}
	}

	/* MEMSTATS */

	fprintf(outf,"\n PDIR CHAIN STATISTICS \n");
	fprintf(outf," Full entries= %d,  half full entries= %d \n\n",
		full_entry, half_entry);
	dumphtbl1_stats();
	dumphtbl2_stats();
}

loghtbl1(space, page)
unsigned int space, page;
{
	htbl1[(pdirhash(space, ptob(page)) & (nhtbl -1))]++;
}

loghtbl2(space, page)
unsigned int space, page;
{
	htbl2[(pdirhash(space, ptob(page)) & (nhtbl2 -1))]++;
}

#define MAXHIST 10

int hist2[MAXHIST];

int hist1[MAXHIST];
dumphtbl1_stats()
{
	int i;
	int total_entries;
	int total_buckets;
	int maxchain;

	total_entries = 0;
	total_buckets = 0;
	maxchain = 0;


	for (i = 0; i < nhtbl; i++){

		/* Create histogram */
		if (htbl1[i] <= MAXHIST)
			hist1[htbl1[i]]++;
		else
			hist1[MAXHIST]++;

		if (htbl1[i] == 0)
			continue;

		total_buckets++;
		total_entries = total_entries + htbl1[i];
		if (htbl1[i] > maxchain)
			maxchain = htbl1[i];
	}
	fprintf(outf, " Maxchain= %d  ave chain= %f  total_entries= %d  total_buckets= %d\n",
	    maxchain, ((float)total_entries)/((float)total_buckets), total_entries, total_buckets);

	for (i = 0; i < MAXHIST; i++){
		fprintf(outf," htbl length %d is 0x%x\n", i , hist1[i]);
	}
	fprintf(outf," htbl length >= %d is 0x%x\n\n", MAXHIST , hist1[MAXHIST]);

}

dumphtbl2_stats()
{
	int i;
	int total_entries;
	int total_buckets;
	int maxchain;

	total_entries = 0;
	total_buckets = 0;
	maxchain = 0;


	for (i = 0; i < nhtbl2; i++){

		/* Create histogram */
		if (htbl2[i] <= MAXHIST)
			hist2[htbl2[i]]++;
		else
			hist2[MAXHIST]++;

		if (htbl2[i] == 0)
			continue;

		total_buckets++;
		total_entries = total_entries + htbl2[i];
		if (htbl2[i] > maxchain)
			maxchain = htbl2[i];
	}
	fprintf(outf, " If hash table were 1/2 nhtbl we would have\n");
	fprintf(outf, " Maxchain= %d  ave chain= %f  total_entries= %d  total_buckets= %d\n",
		maxchain, ((float)total_entries)/((float)total_buckets),
		total_entries, total_buckets);

	for (i = 0; i < MAXHIST; i++){
		fprintf(outf," htbl length %d is 0x%x\n", i , hist2[i]);
	}
	fprintf(outf," htbl length >= %d is 0x%x\n\n", MAXHIST , hist2[MAXHIST]);

}
#endif hp9000s800

runqueues()
{

	if (!(Qflg))
		return(0);
	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                     SCANNING RUN QUEUES                             *\n");
	fprintf(outf,"***********************************************************************\n\n");

	dumpqs(qs);

}

#ifdef hp9000s800
uidhashchains()
{

	if (!(Xflg))
		return(0);
	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                     SCANNING UID HASH QUEUES                        *\n");
	fprintf(outf,"***********************************************************************\n\n");

	dumpuidhx();

}
#endif


dev_table()
{
    int i;
    dtaddr_t *devhash = (dtaddr_t *)lookup("devhash");
    int hash_lengths[DEVHSZ];

    fprintf(outf,"\n\n\n***********************************************************************\n");
    fprintf(outf,"*                        SCANNING DEVICE TABLE                        *\n");
    fprintf(outf,"***********************************************************************\n\n");

    if (devhash == 0)
    {
	fprintf(outf, " devhash: <symbol not found>\n");
	return;
    }

    for (i = 0; i < DEVHSZ; i++)
    {
	dtaddr_t *dtp = (dtaddr_t)get(devhash + i);

	hash_lengths[i] = 0;
	while (dtp != (dtaddr_t)0)
	{
	    dtable_t dt;

	    hash_lengths[i]++;
	    if (getchunk(KERNELSPACE, dtp, &dt, sizeof dt, "dev_table"))
	    {
		fprintf(outf, " can't read dtable_t entry at 0x%08x\n",
		    dtp);
		break;
	    }

	    dump_device_table("", i, dtp, &dt);
	    dtp = dt.next;
	}
    }

    fprintf(outf, "\n Hash lengths:\n");
    for (i = 0; i < DEVHSZ; i++)
	fprintf(outf, "     devhash[%2d] %8d\n", i, hash_lengths[i]);
}

/* vfs_list, print out mounted vfs */
vfs_list()
{
	struct vfs vfs_entry, *vvfs;
	struct vnode vnode;
	extern dumpvfs();

	if (!(Mflg))
		return(0);

	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                     SCANNING MOUNTED VFS LIST                       *\n");
	fprintf(outf,"***********************************************************************\n\n");

	fprintf(outf, " mountlock:      %d\n", mountlock);
	fprintf(outf,"  mount_waiting:  %d\n\n", mount_waiting);

	vfs_entry.vfs_next = rootvfs;
	fprintf(outf,"ROOTVFS:\n");
	while (vvfs = vfs_entry.vfs_next) {
		if (getchunk(KERNELSPACE, vvfs, &vfs_entry, sizeof (struct vfs), "vfs_list"))
			continue;
		dumpvfs(" ", &vfs_entry, vvfs);
		if (vfs_entry.vfs_mtype == MOUNT_UFS)
		{
			struct mount mt;
			if (getchunk(KERNELSPACE, vfs_entry.vfs_data, &mt,
				     sizeof(struct mount), "vfs_list"))
			    continue;
			dumpmt("UFS Mount entry : ", &mt, vfs_entry.vfs_data);
		}
		else if (vfs_entry.vfs_mtype == MOUNT_NFS)
			dumpmntinfo(" ", vfs_entry.vfs_data);
		if (vfs_entry.vfs_vnodecovered) {
			/* get vnode covered by this vfs */
			getchunk(KERNELSPACE, vfs_entry.vfs_vnodecovered,
				 &vnode, sizeof (struct vnode),"vfs_list");
			if (vnode.v_vfsmountedhere != vvfs){
				fprintf_err();
				dumpvnode("Mounted on bad vnode", &vnode,
					  vfs_entry.vfs_vnodecovered);
			}
		}
		fprintf(outf, "\n");
	}
}



/* file_table, print out file table */
file_table()
{
	register struct file *ft;
	extern dumpft();

	if (!(Mflg))
		return(0);
	fprintf(outf,"\n\n\n***********************************************************************\n");
	fprintf(outf,"*                     SCANNING FILE TABLE                            *\n");
	fprintf(outf,"***********************************************************************\n\n");
	for (ft = file; ft < &file[nfile]; ft++){
		dumpft(" ",ft,file,vfile);
		fprintf(outf,"\n");
	}
}
