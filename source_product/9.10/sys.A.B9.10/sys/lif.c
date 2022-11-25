/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/lif.c,v $
 * $Revision: 1.5.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:07:51 $
 */

/*
 * BEGIN_DESC 
 *
 *   File:
 *	lif.c
 *
 *   Purpose:
 *	Implements reading of the LIF header of a disk to determine
 *	if there is a SWAP entry in the LIF directory, and if there
 *	is, it gets the swap start and size data.
 *
 *   Interface:
 *	read_my_lif(dev, start_p, size_p)
 *
 *   Theory:
 *	This module implements the reading of the SWAP information in
 *	the LIF directory.  
 *	At some later time, it may provide some additional and more
 *	general LIF utilities.
 *
 *
 * END_DESC
 */

#include "../h/param.h"
#include "../h/lifdef.h"
#include "../h/lifglobal.h"
#include "../h/types.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/file.h"		/* for FREAD */
#include "../h/vnode.h"
#include "../ufs/inode.h"

extern site_t my_site;          /* this site's id */

/*
 * BEGIN_DESC 
 *
 * read_my_lif(dev, start_p, size_p)
 *
 * Return Value:
 *	1		Succeeded
 *      0		Unable to find LIF directory entry for swap
 *
 * Globals Referenced:
 *	None
 *
 * Description:
 *	This routine returns a 1 and the start address and the size of the 
 *	swap area on the disk or returns zero if they can't be found.
 *
 * Algorithm:
 *	Attempt to read the LIF data from the device, return zero if can't.
 *	Find the SWAP entry in the LIF directory, return zero if can't.
 *	Return a 1 and the start address and size of the swap area.
 *
 *
 * END_DESC
 */

int 
read_my_lif(dev, start_p, size_p)
	dev_t dev;      /* device to look at */
	int *start_p;   /* Address of variable for swap start address */
	int *size_p;    /* Address of variable for swap size */
{
	int error = 0;  /* unsuccessful */
	struct lvol *lif_vol_p;
	struct dentry *lif_dir_entry_p, *dir_end_p;
	struct vnode *dev_vp;
	struct buf *bp;

#define SHIFT256TODEVBSIZE (DEV_BSHIFT - 8)

#ifdef FULLDUX
#ifdef HPNSE
	if (opend(&dev, IFBLK | IF_MI_DEV, my_site, FREAD, 0, 0, 0)) {
#else  /* not HPNSE */
	if (opend(&dev, IFBLK | IF_MI_DEV, my_site, FREAD, 0)) {
#endif /* HPNSE */
#else  /* not FULLDUX */
#ifdef HPNSE
	if (opend(&dev, IFBLK | IF_MI_DEV, FREAD, 0, 0, 0)) {
#else  /* not HPNSE */
	if (opend(&dev, IFBLK | IF_MI_DEV, FREAD, 0)) {
#endif /* HPNSE */
#endif /* FULLDUX */
		return (error);
	}

	dev_vp = devtovp(dev);
	bp = bread(dev_vp, 0, LIFSIZE, B_unknown);  
	bp->b_flags |= B_NOCACHE; 
	if (bp->b_flags & B_ERROR) {
		goto done;
	}

	lif_vol_p = (lifaddr_t) bp->b_un.b_addr;
	if (lif_vol_p->discid != LIFID) {   /* not a lif vol */
		goto done;
	}

	dir_end_p = (diraddr_t) (((lif_vol_p->dstart + lif_vol_p->dsize)*
		LIFSECTORSIZE) + (char *) lif_vol_p);

	lif_dir_entry_p = (diraddr_t) (lif_vol_p->dstart*LIFSECTORSIZE +
				       (char *) lif_vol_p);
        /*
         *  Verify that the directory starts and ends within valid
         *  addresses for the LIF header, else print out warning & clean up.
         */

        if ((lif_dir_entry_p < (diraddr_t) bp->b_un.b_addr) ||
            (lif_dir_entry_p > (diraddr_t) (bp->b_un.b_addr +
                                            LIFSIZE - DESIZE)) ||
            (dir_end_p < (diraddr_t) ((char *)lif_dir_entry_p + DESIZE)) ||
            (dir_end_p > (diraddr_t) (bp->b_un.b_addr + LIFSIZE - DESIZE))) {
                printf("\nLIF area for this device is corrupted.\n");
                printf("Use lifinit to restore it.\n");
                goto done;
        }

	while ((lif_dir_entry_p < dir_end_p) && 
	       lif_dir_entry_p->ftype != EOD) {

		if (lif_dir_entry_p->ftype == LIFSWAPTYPE) {
			*start_p = (lif_dir_entry_p->start >> 
				    SHIFT256TODEVBSIZE);
			*size_p = (lif_dir_entry_p->size >>
				    SHIFT256TODEVBSIZE);
			error = 1; /* routine succeeded */
			goto done;
		}
		lif_dir_entry_p++;
	}
        /* If exited the while loop, no SWAP entry, so return error flag 0 */
done:
	brelse(bp);
#ifdef FULLDUX
#ifdef  HPNSE
	(void) closed(dev, IFBLK, FREAD, my_site, 0);
#else  /* not HPNSE */
	(void) closed(dev, IFBLK, FREAD, my_site);
#endif /* HPNSE */
#else  /* not FULDUX */
#ifdef  HPNSE
	(void) closed(dev, IFBLK, FREAD, 0);
#else  /* not HPNSE */
	(void) closed(dev, IFBLK, FREAD);
#endif /* HPNSE */
#endif /* FULLDUX */
	return (error);  
}

