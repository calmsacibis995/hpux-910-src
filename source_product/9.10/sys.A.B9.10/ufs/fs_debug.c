/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/fs_debug.c,v $
 * $Revision: 1.4.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:20:16 $
 */

/* HPUX_ID: @(#)nfs_debug.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
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


#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../ufs/fsdir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/vm.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/acct.h"
#include "../h/conf.h"
#include "../ufs/fs.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../nfs/nfs.h"
#include "../h/mount.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"
#define CDNOHSZ  63
#if     ((CDNOHSZ&(CDNOHSZ-1)) == 0)
#define        CDNOHASH(dev,cdno)        (((dev)+(cdno))&(CDNOHSZ-1))
#else
#define CDNOHASH(dev,cdno)        (((unsigned)((dev)+(cdno)))%CDNOHSZ)
#endif
union cdhead {
        union  cdhead *cdh_head[2];
        struct cdnode *cdh_chain[2];
} cdhead[CDNOHSZ];
extern int (*test_printf)(); /* defined in trap.c */
#define	printf (*test_printf)

#define INOHSZ  63
#if     ((INOHSZ&(INOHSZ-1)) == 0)
#define        INOHASH(dev,ino)        (((dev)+(ino))&(INOHSZ-1))
#else
#define INOHASH(dev,ino)        (((unsigned)((dev)+(ino)))%INOHSZ)
#endif

int	buf_list[0x300]; 		/*0x300 is current maxbufs*/


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
		if (nbuf > 0x300) {
			printf("Please recompile nfs_debug.c after increase buf_list size\n");
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

kao_inode(i_number)
ino_t i_number;
  { register union ihead *ih;
    register struct inode *ip;

    for(ih = ihead; ih < &(ihead[INOHSZ]); ih++)
      for(ip = ih->ih_chain[0]; ip != (struct inode *) ih; ip = ip->i_forw) {
	if((ip->i_number == i_number) || (i_number == 0)) {
		printf("inode at 0x%x (%x, %d)\n",
			ip, ip->i_dev, ip->i_number);
	}
      }
  }
kao_cdnode(cd_number)
cdno_t cd_number;
  { register union cdhead *cdh;
    register struct cdnode *cdp;

    for(cdh = cdhead; cdh < &(cdhead[CDNOHSZ]); cdh++)
      for(cdp = cdh->cdh_chain[0]; cdp != (struct cdnode *) cdh; 
	  cdp = cdp->cd_forw) {
	if((cdp->cd_num == cd_number) || (cd_number == 0)) {
		printf("cdnode at 0x%x (%x, %d)\n",
			cdp, cdp->cd_dev, cdp->cd_num);
	}
      }
  }
who_has_lockf()
  { register union ihead *ih;
    register struct inode *ip;

    for(ih = ihead; ih < &(ihead[INOHSZ]); ih++)
      for(ip = ih->ih_chain[0]; ip != (struct inode *) ih; ip = ip->i_forw) {
	if(ip->i_locklist) {
		printf("inode at 0x%x (%x, %d)\n",
			ip, ip->i_dev, ip->i_number);
	}
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

ipcheck(ip)
        struct inode *ip;
{
        printf("ip = %x\n", ip);
        printf("number = %d\n", ip->i_number);
	printf("dev = 0x%x\n", ip->i_dev);
        printf("flag (octal) = %o\n", ip->i_flag);
        printf("mode (octal) = %o\n", ip->i_mode);
} 

