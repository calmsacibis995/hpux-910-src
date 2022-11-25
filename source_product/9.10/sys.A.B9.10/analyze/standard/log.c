/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/log.c,v $
 * $Revision: 1.35.83.4 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/17 16:48:47 $
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


/* Logswbuf, logs an entry and sees if it has been used before */
logswbuf(bp,type)
	struct buf *bp;
	int type;
{
	int indx;

	indx = bp - swbuf;
	if (swbufblk[indx].type != BFREE){

		fprintf_err();
		fprintf(outf,"   Duplicate swap buf allocated \n");
		fprintf(outf,"   Previous use was in chain:");
		fprintf(outf, swpnames[swbufblk[indx].type]);
		return(1);
		}
	swbufblk[indx].type = type;
	swbufblk[indx].bp = bp;
	return(0);

}



/* Logbuf, logs an entry and checks for various consistency checks. */
logbuf(bp,vbp,type,list)
	struct buf *bp;
	int type,list;

{
register struct bufblk *pblk, *blk;

	pblk =BUFBLKHASH(vbp, bp->b_blkno);	
	blk = pblk->next;
	
	while (blk != NULL) {
		if (blk->vbp == vbp)
			break;
		pblk=blk;
		blk = blk->next;
	}
	if (blk == NULL) {	/* we did not find it */
		blk = (struct bufblk *)calloc(1 , sizeof (struct bufblk));
		blk->next = pblk->next;
		pblk->next = blk;
	}
	if (list == BFREELIST){
		/* check that we have not had an entry before */
		if (blk->freetype != BFREE){

			fprintf_err();
			fprintf(outf,"   Duplicate buf entry in chain\n");
			fprintf(outf,"   Previous use was on ");
			fprintf(outf, bufnames[blk->freetype]);
			fprintf(outf,"\n");
			/* Return something speacial if looping */
			return(1);
		}

		/* check that b_busy flag is not set */
		if (bp->b_flags & B_BUSY){

			fprintf_err();
			fprintf(outf,"   Busy flag set on freelist entry\n\n");
		}

		blk->freetype = type;
		blk->vbp = vbp;
	}

	if (list == BHASH){
		if (blk->hashtype != BFREE){

			fprintf_err();
			fprintf(outf,"   Duplicate buffer on hash list\n");
			fprintf(outf,"   Previous entry was ");
			if (blk->hashtype != BFREE){
				fprintf(outf, bufnames[blk->hashtype]);
				return(1);

			} else {
				fprintf(outf, bufnames[blk->freetype]);
				return(2);
			}
		}
		blk->hashtype = type;
		blk->vbp = vbp;
	}
	return(0);


}


/* Loginode logs an entry and sees if it has been used before */
loginode(ip,list)
	struct inode *ip;
	int list;
{
	register int indx, imode, count, i;
	extern dumpinode();

	indx = ip - inode;
	if ((inodeblk[indx].list != IEMPTY) && (inodeblk[indx].list == list)){

		fprintf_err();
		fprintf(outf,"   Duplicate incore inode entry \n");
		dumpinode(" Chain looping",ip,inode,vinode);
		return(1);
		}
	/* Do a few consistency checks */
	if (list == ICHAIN){
		/* do any checks here */
	}
	count = 0;
	/* Lets check the mutually exclusive flags */
	imode = ip->i_mode;
	/* These are no longer exclusive, so we have removed this */
	/* code */



	/* Log entry */
	inodeblk[indx].list = list;
	inodeblk[indx].ip = ip;
	return(0);

}

/* min function */
my_min(a, b)
	int a,b;
{
	return (a < b ? a : b);
}





/* REG300, must remove protid, stuff, and pdir references */


#ifdef hp9000s800
/* MEMSTATS */
#define DLD_SP_START (64*1024)
preg_t *slastprp =0;
int sfirstvirt =0;
int slastspace =0;
int stopstack =0;
int svsize =0;
int stopstack_dld =0;
int sdldstack = USRSTACK + DLD_SP_START;
int stackbuf[NBPG/4];
int scount = 0;
int user_crossed =0;
#endif


/* log pregion */
/* type 0, ZSYS means system, ZFREE */
/* Must supply a dummy vfd with page frame if page is to be logged */
logvfd(p, vfd, db, type,virtual,prp,status)
	struct proc *p;
	register vfd_t *vfd;
	register dbd_t *db;
	int type,virtual,status;
	preg_t *prp;
{
	register int pfnum = vfd->pgm.pg_pfnum;
	register struct paginfo *zp = &paginfo[pfnum];
	unsigned int space = 0;
	vfd_t vfd1;
	unsigned int 	protid = 0;
	short pid = 0;
	struct pfdat *pf;
/* MEMSTATS */
	int shighwater, i;
	int mapped;


	vfd1.pgi.pg_vfd = 0;

	/* Set default type */
	if (type == 0)
		type = pttozpt(prp->p_type);

	/* Pages logged, but outside the range of pfdat */
	if (type == ZSYS)
		goto record;

	/* MEMSTATS */
	mapped = ltor(prp->p_space, virtual);

	/* If we say we are not mapped, then check this */
	if (!(vfd->pgm.pg_v)){
		/* MEMSTATS */
		if (mapped){

			fprintf_err();
			fprintf(outf," Region vfd not valid, but page still mapped in pdir\n");
			fprintf(outf," Space: 0x%x  Virtual: 0x%x  Vfd: 0x%x\n",
			prp->p_space, virtual, vfd->pgi.pg_vfd);
		} else
		if (db != (dbd_t *)0 && db->dbd_type == DBD_NONE) {
			fprintf_err();
			fprintf(outf," Non-valid vfd 0x%x has dbd_type of NONE\n", *(int *)vfd);
			fprintf(outf," Virtual address that this vfd correspond to is 0x%x\n", virtual);
		}

		/* Empty vfd so return, since nothing to log */
		return(0);
	}

	pfnum = vfd->pgm.pg_pfnum;
#ifdef  hp9000s300
	if (pfnum > (maxfree - firstfree + 1)) {
		fprintf_err();
		fprintf(outf," invalid pfnum with pid %d\n",p->p_pid);
		return(0);
	}
#else
	if ( pfnum < firstfree || pfnum > (maxfree + firstfree)) {
		fprintf_err();
		fprintf(outf," invalid pfnum with pid %d\n",p->p_pid);
		return(0);
	}
#endif

	pf = &pfdat[pfnum];

	/* Check that if page is valid, then it is not also marked free */
	if (vfd->pgm.pg_v){
		if (pf->pf_flags & P_QUEUE){

			fprintf_err();
			fprintf(outf,"  page marked valid, but has P_QUEUE set\n");
			fprintf(outf,"  virtual address == 0x%08x\n", virtual);
			dumppfdat(" bad P_QUEUE ",pf, pfdat, vpfdat);
		}
		if (pf->pf_flags & P_BAD){

			fprintf_err();
			fprintf(outf,"  page marked valid, but has P_BAD set,");
			fprintf(outf,"  virtual address == 0x%08x\n", virtual);
			dumppfdat(" bad P_BAD ",pf, pfdat, vpfdat);
		}
		if ((pf->pf_flags & P_HASH) && (pf->pf_data == 0)){
			fprintf(outf,"  Page on hash chain has pf_data of 0.");
			fprintf(outf,"  virtual address == 0x%08x\n", virtual);
			dumppfdat(" bad P_HASH ",pf, pfdat, vpfdat);
		}
	}

#ifdef hp9000s800
	/* MEMSTATS */
	/* If it is a stack page lets look how big we grew */

	if (type == ZSTACK){
		if (prp != slastprp){
			/* reset when we cross */
			stopstack = 0;
			stopstack_dld = 0;
			scount = 0;
			sfirstvirt = virtual;
			slastspace = prp->p_space;
			slastprp = prp;
			svsize = prp->p_count;
			user_crossed =0;
		}

		/* Only scan if it is mapped */
		if (mapped){
		    if (!(getchunk(prp->p_space, virtual, stackbuf, NBPG, "logvfd"))){
			shighwater = 0;
			/* Scan for highest nonzero */
			for (i = 0; i < (NBPG/4) ; i++){
				if (stackbuf[i] != 0)
					shighwater = i;
			}
			if (scount < (DLD_SP_START/NBPG)){
				if (shighwater != 0)
					stopstack = virtual + shighwater*4;
			} else {
				if (shighwater != 0)
					stopstack_dld = virtual + shighwater*4;
				if (user_crossed)
					stopstack = virtual + shighwater*4;
			}
			/* Has the user stack crossed into dld stack?
			 * (1/2 way into last page before dld stack ) */
			if ((scount == (DLD_SP_START/NBPG -1)) && (shighwater >
				(NBPG/8)))
					user_crossed =1;

		    }
		}
		/* We have scanned another stack page */
		scount++;


	}
#endif

#ifdef PFDAT32
	if (pfnum != (pf-pfdat)){
#else
	if (pfnum != pf->pf_pfn){
#endif
			fprintf_err();
			fprintf(outf,"  pf_pfn in pfdat != vfd->pf_pfnum\n");
#ifdef PFDAT32
			fprintf(outf,"  0x%08x != 0x%08x\n",pf-pfdat,pfnum);
#else
			fprintf(outf,"  0x%08x != 0x%08x\n",pf->pf_pfn,pfnum);
#endif
			dumppfdat(" bad pf_pfn ",pf, pfdat, vpfdat);
	}



	/* Account for page if not already in use, and report if */
	/* already used and not a TEXT, MMAP, or SHARED PAGE */

	if (bad(zp, type)) {
		/* Add Copy on access check here */
		/* MEMSTATS */
		/* If copy on access then duplication is ok */
		if (vfd->pgm.pg_cw){
			/* If we REALLY OWN IT then change the log */
			goto copyonw;
		}


		fprintf_err();
		fprintf(outf,"dup page vfd %x\n", *(int *)vfd);
		dumppfdat("",pf, pfdat, vpfdat);
		fprintf(outf,"\n");
		dump(zp);
		fprintf(outf,"   Claimed now as %s in pid %d\n",typepg[type], p->p_pid);
		fprintf(outf,"\n\n");
		return;
	}

copyonw:

	if (type != ZFREE){
#ifdef  hp9000s800
		protid = prp->p_hdl.hdlprot;
#endif
		pid = p->p_pid;
		vfd1 = *vfd;
		space = prp->p_space;
	}
record:
	if (mapped){
		/* Only record the actual mapping, not copy on write handles */
		zp->z_protid = protid;
		zp->z_pid = pid;
		zp->z_vfd = vfd1;
		zp->z_space = space;
		zp->z_virtual = btop(virtual);
		zp->z_type = type;
		zp->z_status = status;
	}
	zp->z_count++;

	/* MEMSTATS */
	/* Count the types of pages only once */
	if (zp->z_count < 2){
		if (type < ZMAX)
			pfdat_types[type]++;
		else
			pfdat_types[ZMAX]++;
	}

}


/* Bad reports bad if the page has been used and was not TEXT */
/* or SHARED MEMORY */
/* MEMSTATS */
bad(zp, type)
	struct paginfo *zp;
{
	if (zp->z_type != 0 && zp->z_type != type){
		fprintf_err();
		fprintf(outf,"Page type changed\n");
		fprintf(outf,"   Claimed now as %s\n",typepg[type]);
		return (-1);
	}

	if (type == ZTEXT) {
		return (0);
	}
	if (type == ZNULLDREF){
		return (0);
	}
	if (type == ZLIBTXT){
		return (0);
	}
	if (type == ZSHMEM){
		return (0);
	}
	if (type == ZMMAP){
		return (0);
	}
	return (zp->z_count);
}


/* Store away usage info on disc map entry */
/* Do a minor check on ranges */
sysmapuse(first, size)
long    first;
int     size;

{
	register struct sysblks *sp;

	sp = &sblks[nsblks];
	if (++nsblks > SYSMAPSIZE) {

		fprintf_err();
		fprintf(stderr, "too many sysmap entries\n");
	}
	sp->s_first = first;
	sp->s_size = size;
}


/* log dbd */


struct pfdat *
pffinder(devvp, blkno)
	struct vnode *devvp;
	daddr_t	blkno;
{
	register pfd_t		*pfd;


	/*
	 *	hash the page on the physical block number
	 *	which matches
	 */

	for (pfd = phash[PF_HASH(blkno)&phashmask]; pfd != NULL; pfd = pfd->pf_hchain) {
		pfd = localizer(pfd, pfdat, vpfdat);
		if((pfd->pf_data == blkno) && (pfd->pf_devvp == devvp)) {
			if (pfd->pf_flags & P_BAD)
				continue;
			return(pfd);
		}
	}
	return(0);
}
