/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/printswap.c,v $
 * $Revision: 1.5.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:44:57 $
 */

/* HPUX_ID: @(#)printswap.c	55.1		88/12/23 */

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


#include	"../h/param.h"
#include	"../h/user.h"
#include	"../h/proc.h"
#include	"../h/conf.h"
#include	"../h/map.h"
#include	"../dux/rmswap.h"


printswdevt() 
{
	u_int	c;
	struct swdevt *swp;

	uprintf("\n");
	uprintf("               **** SWAP DEVICE TABLE ****\n");
	for (swp = swdevt, c=0; swp->sw_dev; swp++,c++) {
		uprintf("--------------------------");
		uprintf("--------------------------\n");
		uprintf("0x%x|dev=0x%x|freed=0x%x|nblks=0x%x|start=0x%x|\n", c,
			swp->sw_dev,swp->sw_freed,swp->sw_nblks,swp->sw_start);
	}
	uprintf("--------------------------");
	uprintf("--------------------------\n");
}
	
printswapmap (from, to) 
	u_int	from, to;
{
	struct mapent *bp, *bp1;
	u_int	c;

	bp1 = (struct mapent *)(swapmap+1+from);
	uprintf("\n");
	uprintf("        **** SWAP MAP ****\n");
	bp = bp1;
	c = from;
	do {
		uprintf("------------------------------------------\n");
		uprintf("0x%x|addr=0x%x|size=0x%x|\n",
			c,bp->m_addr,bp->m_size);
		bp++;
		c++;
	} while ( c <= to);
	uprintf("------------------------------------------\n");
}

printargmap (from, to) 
	u_int	from, to;
{
	struct mapent *bp, *bp1;
	u_int	c;

	bp1 = (struct mapent *)(argmap+1+from);
	uprintf("\n");
	uprintf("          **** ARG MAP ****\n");
	bp = bp1;
	c = from;
	do {
	uprintf("------------------------------------------\n");
		uprintf("0x%x|addr=0x%x|size=0x%x|\n",
			c,bp->m_addr,bp->m_size);
		bp++;
		c++;
	} while ( c <= to);
	uprintf("------------------------------------------\n");
}

printvmmesg(vmmesg)
	struct dux_vmmesg *vmmesg;
{
	uprintf("\n");
	uprintf("      **** VM MESSAGE ****\n");
		uprintf("------------------------------\n");
		uprintf("|size=0x%x|frame=0x%x|\n",
			vmmesg->ch_size,vmmesg->ch_frame);
	uprintf("------------------------------\n");
}

pu(uptr)
	struct user *uptr;
{
	register int i;

	uprintf("\n");
	uprintf(" *** Memory Management Information from proc ***");
	uprintf("\n");
	uprintf("pid=0x%x  ",uptr->u_procp->p_pid);
	uprintf("p_swdarr=0x%x  ",uptr->u_procp->p_swaddr);
	uprintf("p_tsize=0x%x  ",uptr->u_procp->p_tsize);
	uprintf("p_dsize=0x%x  ",uptr->u_procp->p_dsize);
	uprintf("p_ssize=0x%x  ",uptr->u_procp->p_ssize);
	uprintf("\n");
	uprintf(" *** Memory Management Information from u ***");
	uprintf("\n");
	uprintf("u_tsize=0x%x  ", uptr->u_tsize);
	uprintf("u_dsize=0x%x  ", uptr->u_dsize);
	uprintf("u_ssize=0x%x  ", uptr->u_ssize);
	uprintf("\n");
	uprintf("\n");
	uprintf("u_dmap : ");
	uprintf("dm_size=0x%x ", uptr->u_dmap.dm_size);
	uprintf("dm_alloced=0x%x", uptr->u_dmap.dm_alloced);
	uprintf("\n");
	for(i=0;i < NDMAP; i++) 
		uprintf("[0x%x]=0x%x ",i,uptr->u_dmap.dm_map[i]);
	uprintf("\n");
	uprintf("u_smap : ");
	uprintf("dm_size=0x%x ", uptr->u_smap.dm_size);
	uprintf("dm_alloced=0x%x", uptr->u_smap.dm_alloced);
	uprintf("\n");
	for(i=0;i < NDMAP; i++) 
		uprintf("[0x%x]=0x%x ",i,uptr->u_smap.dm_map[i]);
	uprintf("\n");
	uprintf("u_cdmap : ");
	uprintf("dm_size=0x%x ", uptr->u_cdmap.dm_size);
	uprintf("dm_alloced=0x%x", uptr->u_cdmap.dm_alloced);
	uprintf("\n");
	for(i=0;i < NDMAP; i++) 
		uprintf("[0x%x]=0x%x ",i,uptr->u_cdmap.dm_map[i]);
	uprintf("\n");
	uprintf("u_csmap : ");
	uprintf("dm_size=0x%x ", uptr->u_csmap.dm_size);
	uprintf("dm_alloced=0x%x", uptr->u_csmap.dm_alloced);
	uprintf("\n");
	for(i=0;i < NDMAP; i++) 
		uprintf("[0x%x]=0x%x ",i,uptr->u_csmap.dm_map[i]);
	uprintf("\n");
}
