/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/REG.300/RCS/swapconf.c,v $
 * $Revision: 1.5.84.5 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/11/03 09:42:58 $
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


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/conf.h"
#include "../h/swap.h"
#include "../dux/rmswap.h"
#include "../h/buf.h"
#include "../h/file.h"

#include "../h/vmmac.h"		/* for ptob() */
#include "../h/vnode.h"
#include "../ufs/inode.h"	/* for IFBLK */
#include "../dux/cct.h"		/* for CCT_SLWS */
extern	u_int	my_site_status;
extern  long    dumplo;         /* offset into dumpdev */
extern  int     dumpsize;       /* size of dump in NBPG pages */
extern  int     physmembase;    /* start location for dump */
extern  int     dumpmag;        /* magic number for savecore, 0x8fca0101 */
extern  dev_t   dumpdev;        /* device ident for savecore */

int dumpdevsize = 0;	/* Actual size of the dump device in blocks */

/*
 * Configure dump device.
 */
dumpconf() {
    int fsblocks = 0;
    extern int install, noswap;

    /*
     * dumpsize is the number of pages for the number of bytes from 
     * the address that physmembase represents, to the end of memory.  
     */
    dumpsize = btop(-(physmembase*NBPG));

    /*
     * Just as we opened the swap device(s), let's open the dump device,
     * and get its size.
     */

    /*
     * Don't try to open NODEV dump device because opend() will
     * successfully open the root device because NODEV is identical
     * the pseudo-root device.  Yuch.
     * Also, don't close the dump device once it's successfully
     * opened because the CS80 driver dump routine, unlike the S800
     * drivers, will complain if the device isn't open.
     */
    if ((dumpdev != NODEV) && (opend(&dumpdev, IFBLK, FREAD, 0) == 0)) {
	if (bdevsw[major(dumpdev)].d_psize)
	    dumpdevsize = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
	if ((fsblocks = fs_size(dumpdev)) > 0)
		printf("Warning: file system present on dump device\n");
    } else {
	    /* unspecified or bogus dump device -- use primary swap */
	    if (((noswap || (my_site_status & CCT_SLWS)) && !install)
		    || dumpdev != NODEV)
#ifndef XTERMINAL
		printf("Warning: unable to configure dump device\n");
#endif
	    if (noswap || (my_site_status & CCT_SLWS)) {
		dumpdev = NODEV;
		return;
	    }
	    msg_printf("Using primary swap device for crashdump.\n");
	    dumpdev = swdevt[0].sw_dev;
	    dumpdevsize = swdevt[0].sw_nblks;
	    if (dumpdevsize < (dumpsize << (PGSHIFT - DEV_BSHIFT)))
	    	dumpsize = btop(dumpdevsize*DEV_BSIZE);
	    dumplo = swdevt[0].sw_start + swdevt[0].sw_nblks - 
			(dumpsize << (PGSHIFT - DEV_BSHIFT)); /* in 1k blocks */
    }
    if (dumplo == 0) {
	dumplo = dumpdevsize - ((int)ptob(dumpsize) / DEV_BSIZE);
	dumpdevsize -= fsblocks;
    }		    
    if (dumpdevsize < ((int)ptob(physmem) / DEV_BSIZE)) {
	printf("\nWarning: insufficient space on dump device to save full crashdump.\n");
	printf("Should a crashdump be necessary only %d of %d bytes will be saved.\n\n",
	dumpdevsize * DEV_BSIZE, ptob(physmem));
    }
    msg_printf("Core image of %d pages will be saved at block %d on device 0x%x\n",
		 dumpsize, dumplo, dumpdev);

    dumpmag = 0x8fca0101;		/* magic number for savecore file */
}
