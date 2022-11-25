/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_debug.c,v $
 * $Revision: 1.5.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:40:28 $
 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
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

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


#ifdef hp9000s800

/*
 * SERIES 800 dux_debug.c
 */

#include "../h/param.h"
#include "../h/types.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/buf.h"
#include "../h/vmmac.h"
#include "../h/mbuf.h"
#include "../dux/dm.h"
#include "../dux/protocol.h"

struct buf *debug_bp;
struct inode *debug_ip;
struct vnode *debug_vp;
caddr_t debug_chan;
dm_message debug_dmp;
struct dm_header *debug_hp;
struct serving_entry *debug_se;
struct using_entry *debug_ue;

debug_sleep()
{}

osps()
{
	register struct proc *p;
	int i;
	struct user *myup;
	extern int rdb_printf_enb;

	for (p=proc; p < procNPROC; p++)
	{
		if (p->p_stat == NULL)
			continue;
		printf ("pid=%d, ppid=%d, pgrp=%d, wchan=0x%x, status=%d, flag=0x%x",p->p_pid, p->p_ppid, p->p_pgrp, p->p_wchan, p->p_stat, p->p_flag);
		if (p->p_flags & SLOAD) {
			myup = uvadd(p);
			printf("\tu_comm %s", myup->u_comm);
		}
		printf("\n");
		if (!rdb_printf_enb)
			for (i=0; i<100000; i++);
	}
}

int
debug_traverse_freelist(freelist)
register struct buf *freelist;
{
	register struct buf *bp;
	register struct buf *lastb = freelist;
	register n=0;

	for (bp=freelist->av_forw; bp != freelist; bp = bp->av_forw)
	{
		if (bp->av_back != lastb)
		{
			printf ("forw != back last:0x%x next:0x%x\n",lastb,bp);
			return (-1);
		}
		n++;
		lastb = bp;
	}
	return (n);
}

int
debug_traverse_hashchain(hashchain)
register struct buf *hashchain;
{
	register struct buf *bp;
	register struct buf *lastb = hashchain;
	register n=0;

	for (bp=hashchain->b_forw; bp != hashchain; bp = bp->b_forw)
	{
		if (bp->b_back != lastb)
		{
			printf ("forw != back last:0x%x next:0x%x\n",lastb,bp);
			return (n);
		}
		n++;
		lastb = bp;
	}
	return (n);
}

debug_bufs()
{
	int i;
	
	printf ("nbuf=%d\n",nbuf);
	printf ("freelists:\n");
	printf ("\tlocked:  %d\n",debug_traverse_freelist(bfreelist[BQ_LOCKED]));
	printf ("\tlru:  %d\n",debug_traverse_freelist(bfreelist[BQ_LRU]));
	printf ("\tage:  %d\n",debug_traverse_freelist(bfreelist[BQ_AGE]));
	printf ("\tempty:  %d\n",debug_traverse_freelist(bfreelist[BQ_EMPTY]));
		for (i=0; i<100000; i++)
			;
	printf ("hashchains:\n");
	for (i=0; i<BUFHSZ; i++)
		printf ("%d: %d; ",i,debug_traverse_hashchain(bufhash[i]));
	printf ("\n");
		for (i=0; i<100000; i++)
			;
}
		
struct dm_header *
debug_dm(dmp)
dm_message dmp;
{
	debug_dmp = dmp;
	if (dmp->m_off == 0)
	{
		printf ("empty message");
		debug_hp = NULL;
	}
	else
		debug_hp = DM_HEADER(dmp);
	return (debug_hp);
}

extern struct	using_entry	using_array[];
extern int	using_array_size;	/* using_array[using_array_size]     */
extern struct	serving_entry	serving_array[];
extern int	serving_array_size;	/* serving_array[serving_array_size] */

struct serving_entry *
debug_finds(rid)
register rid;
{
	register i;

	for (i=0; i<serving_array_size; i++)
	{
		if (serving_array[i].rid == rid)
		{
			debug_se = &serving_array[i];
			return (debug_se);
		}
	}
	printf ("not found\n");
	return (0);
}

struct using_entry *
debug_findu(rid)
register rid;
{
	register i;

	for (i=0; i<using_array_size; i++)
	{
		if (using_array[i].rid == rid)
		{
			debug_ue = &using_array[i];
			return (debug_ue);
		}
	}
	printf ("not found\n");
	return (0);
}


#else /* not hp9000s800 */

/*
 *  SERIES 300 dux_debug.c
 */


#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/inode.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/nami.h"
#include "../h/acct.h"
#include "../h/conf.h"
#include "../h/fs.h"
#include "../dux/dm.h"
#include "../dux/duxscalls.h"
#include "../dux/dmmsgtype.h"
#include "../dux/dux_dev.h"
#include "../h/mount.h"

#define INOHSZ  63
#if     ((INOHSZ&(INOHSZ-1)) == 0)
#define        INOHASH(dev,ino)        (((dev)+(ino))&(INOHSZ-1))
#else
#define INOHASH(dev,ino)        (((unsigned)((dev)+(ino)))%INOHSZ)
#endif

int	buf_list[128]; 		/*128 is current maxbufs*/


union ihead {                           /* inode LRU cache, Chris Maltby */
        union  ihead *ih_head[2];
        struct inode *ih_chain[2];
} ihead[INOHSZ];

/* struct inode *ifreeh, **ifreet; */
kaobcheck(type, arg)
int	type;
struct	buf	*arg;
{
	int	s, i, mask, *ip;
	unsigned j, k;
	struct	buf	*bufp1, *bufp2;
	struct	bufhd	*bufhdp1, *bufhdp2;

	s = spl6();
	switch(type) {
	case 1:
		mask = 1;
		if (nbuf > 128) {
			printf("Please recompile dux_debug.c after increase buf_list size\n");
			splx(s);
			return(1);
		}
		for (i = 0; i < nbuf; i++) buf_list[i]=0;
		for(bufp1 = bfreelist,i= 0; i <= BQ_EMPTY; i++, bufp1++) {
			bufp2=bufp1;
			while((bufp2=bufp2->av_forw) != bufp1) {
				if((j = bufp2 -buf) > nbuf) {
					printf("bad buf freelist link\n");
					splx(s);
					return(1);
				}
				buf_list[j] |= mask;
			}
			mask <<= 1;
		}
		k = mask;
		for(bufhdp1 = bufhash, i= 0; i < BUFHSZ; i++,bufhdp1++) {
			bufp2 = bufhdp1 -> b_forw;
			while ((bufp2=bufp2->b_forw) != (struct buf *)bufhdp1) {
				if((j = bufp2 -buf) > nbuf) {
					printf("bad buf hash link\n");
					splx(s);
					return(1);
				}
				buf_list[j] += mask;
			}
			mask += k;
		}
		printf("Missing list:");
		for (i= 0, ip=buf_list; i < nbuf; ip++, i++) {
			if(!*ip) printf("%d, ", i);
		}
		printf("\n");
		break;
	case 2:
		for(bufhdp1 = bufhash, i= 0; i < BUFHSZ; i++,bufhdp1++) {
			if (arg) {
				bufhdp1 = (struct bufhd *)arg;
			} 
			bufp2 = (struct buf *)bufhdp1;
			j=0xffff;
			while ((bufp2=bufp2->b_forw) != (struct buf *)bufhdp1) {
				if(arg) i=bufp2-buf;
				if (j >nbuf) printf("chain %d: ", i);
				if((j = bufp2 -buf) > nbuf) {
					printf("bad buf hash link\n");
					splx(s);
					return(1);
				}
				printf("0x%x(%d) -> ", bufp2, j);
			}
			if(j<nbuf) printf("0x%x(back to head)\n", bufhdp1);
			if (arg) break;
		}
		break;
	case 3:
		for(bufp1 = bfreelist,i= 0; i <= BQ_EMPTY; i++, bufp1++) {
			bufp2=bufp1;
			while((bufp2=bufp2->av_forw) != bufp1) {
				if((j = bufp2 -buf) > nbuf) {
					printf("bad buf freelist link\n");
					splx(s);
					return(1);
				}
				printf("0x%x(%d) -> ", bufp2, j);
			}
			printf("0x%x(back to head)\n", bufp1);
		}
		break;
	default:
		printf("invalid action\n");
		break;
	}
	splx(s);
	return(0);
}

osps()
{
	register struct proc *p;

	for (p=proc; p < procNPROC; p++)
	{
		if (p->p_stat == NULL)
			continue;
		printf ("pid=%d, ppid=%d, pgrp=%d, wchan=0x%x, status=%d, flag=0x%x\n",p->p_pid, p->p_ppid, p->p_pgrp, p->p_wchan, p->p_stat, p->p_flag);
	}
}

check_ilist()
  { register union ihead *ih;
    register struct inode *ip;

    for(ih = ihead; ih < &(ihead[INOHSZ]); ih++)
      for(ip = ih->ih_chain[0]; ip != (struct inode *) ih; ip = ip->i_forw)
	if(ip->i_forw != (struct inode *) ih && (ip->i_forw < inode || ip->i_forw >= inodeNINODE))
	  if(ip->i_forw >= (struct inode *) ihead && ip->i_forw < (struct inode *) &(ihead[INOHSZ]))
	    { printf("inode at 0x%x (%x, %d) on hash 0x%x points to 0x%x\n",
			ip, ip->i_dev, ip->i_number, ih, ip->i_forw);
	      break;
	    }
	  else
	    { printf("inode at 0x%x (%x, %d) on hash 0x%x points to hyperspace 0x%x\n",
			ip, ip->i_dev, ip->i_number, ih, ip->i_forw);
	      break;
	    }
  }

bcheck()
{
        register struct buf *bp, *hp, *flist;
        int s;

        s = spl6();
        for (flist = bfreelist; flist < &bfreelist[BQ_EMPTY]; flist++)
        {
                for (hp = flist, bp = hp->av_forw;
                     bp != flist; 
                     bp = bp->av_forw, 
                     hp = hp->av_forw ) 
                {
                        if ( bp->av_back == hp) continue;
                       else {
                                printf("chain %d: (%x)-->(%x)  (???)<--(%x)\n",
                                (flist - bfreelist), hp, bp, bp->av_back);
                                splx(s);
                                return;
                        }
                }
                if (bp->av_back != hp) {
                        printf("end of chain %d: (%x)-->(%x)  (???)<--(%x)\n",
                        (flist - bfreelist), hp, bp, bp->av_back);
                        splx(s);
                        return;
                }
        
        }
        printf("bcheck OK\n");
        splx(s);
} 

ipcheck(ip, site)
        struct inode *ip;
        site_t site;
{
        printf("ip = %x\n", ip);
        printf("number = %d\n", ip->i_number);
        printf("count = %d\n", ip->i_count);
        printf("flag (octal) = %o\n", ip->i_flag);
        printf("mode (octal) = %o\n", ip->i_mode);
        printf("rcount = %d\n", ip->i_execdcount.d_rcount);
        printf("vcount = %d\n", ip->i_execdcount.d_vcount);
        printf("total exec sites = %d\n", gettotalsites(&(ip->i_execsites)));
        printf("total write sites = %d\n", gettotalsites(&(ip->i_writesites)));
        printf("total namei sites = %d\n", gettotalsites(&(ip->i_nameisites)));
        printf("site %d namei ref count = %d\n", site, 
                getsitecount(&(ip->i_nameisites), site));
} 

inocheck(dev, ino, site)
        dev_t dev;
        ino_t ino;
        site_t site;
{
        struct inode *ip;

        ip = ifind(dev, ino);
        if (ip == NULL) return;
        ipcheck(ip, site);
}

printlocks(lp)
  register struct locklist *lp;
  { while(lp != NULL)
      { printf("  lock at 0x%x, ", lp);

	if(lp->ll_flags & L_REMOTE)
	  printf("held by PID %d on site %d: ", lp->ll_pid, lp->ll_psite);
 	else
	  printf("held by local procees %d: ", lp->ll_proc->p_pid);

	printf("%d to %d\n", lp->ll_start, lp->ll_end);

	lp = lp->ll_link;
      }
  }

printilocks(ip)
  struct inode *ip;
  { printf("inode at 0x%x\n", ip);

    if(ip->i_locklist == NULL)
      printf("  No locks\n");
    else
      printlocks(ip->i_locklist);
  }

dmcheck(msg)
dm_message msg;
{
	struct dm_header *hp = DM_HEADER(msg);

	printf ("dm message @ 0x%x\n",msg);
	printf ("header @ 0x%x (%s)\n",hp,
		(((int)hp) - ((int)msg)) == 12 ? "dux_mbuf": "dux_mcluster");
	printf ("p_flags:  0x%x\n",hp->dm_ph.p_flags);
	printf ("p_rid:  0x%x\n",hp->dm_ph.p_rid);
	printf ("dm_flags:  0x%x\n",hp->dm_flags);
	printf ("dm_func:  0x%x\n",hp->dm_func);
	printf ("dm_bufp:  0x%x\n",hp->dm_bufp);
	printf ("dm_bufoffset:  0x%x\n",hp->dm_bufoffset);
	printf ("dm_mid:  0x%x\n",hp->dm_mid);
	printf ("dm_dest:  0x%x\n",hp->dm_dest);
	printf ("dm_tflags:  0x%x\n",hp->dm_tflags);
	printf ("dm_headerlen:  0x%x\n",hp->dm_headerlen);
	printf ("dm_datalen:  0x%x\n",hp->dm_datalen);
	printf ("dm_srcsite:  0x%x\n",hp->dm_srcsite);
	printf ("dm_op/dm_rc:  0x%x\n",hp->dm_op);
	printf ("dm_pid/dm_eosys:  0x%x\n",hp->dm_pid);
}

#endif
