/* @(#) $Revision: 64.1 $ */      
/* $Source: /misc/source_product/9.10/commands.rcs/etc/fsck/ufs_subr.c,v $
 * $Revision: 64.1 $	$Author: lkc $
 * $State: Exp $   	$Locker:  $
 * $Date: 89/05/04 16:25:58 $
 *
 * $Log:	ufs_subr.c,v $
 * Revision 64.1  89/05/04  16:25:58  16:25:58  lkc
 * I did an unifdef -UKERNEL to remove the kernel specific code.  This is 
 * in response to DTS FSDlj04301 which was complaining about NMOUNT, a define
 * only used in the kernel portion of the file.  By the way, fsck is not built
 * with a -DKERNEL so this is essentially a harmless change.
 * 
 * Revision 56.2  87/04/21  08:08:29  08:08:29  rer (Rob Robason)
 * removed include of vnode.h, which is now included by inode.h
 * 
 * Revision 56.1  87/03/30  11:01:10  11:01:10  ems
 * Changes for new checklist file format and DUX.
 * 
 * Revision 37.1  85/08/15  09:38:19  09:38:19  nm (Naomi Munekawa)
 * Update to match kernel's ufs_subr.c
 * 
 * Revision 1.6  85/08/12  17:53:18  jcm (J. C. Mercier)
 * Fixed up includes for users, so that fsck can
 * do its job and FSD can smile.
 * 
 * Revision 1.5  85/05/21  19:10:38  jcm (J. C. Mercier)
 * Deleted inclusion of <sys/user.h>.
 * Added inclusion of <sys/types.h>.
 * This is for non-KERNEL code, i. e. fsck(8).
 * 
 * Revision 1.4  85/05/10  09:56:40  sol (Sol Kavy)
 * moved over preemption points from root.rt
 * 
 * Revision 1.3  85/03/07  17:19:07  sol (Sol Kavy)
 * Modified code to fully support SYNCIO.  Code is from Debbie Bartlett.
 * 
 * Revision 1.2  84/01/18  14:51:54  kusmer (Steve Kusmer)
 * Header added.  SCCS original: ufs_subr.c 6.1 83/07/29
 * 
 * $Endlog$
 */

/*
** NOTE: The header files have been juggled for Early Release of DUX/NFS.
**	 This should be cleaned once the header files are straightened out.
*/
#ifdef hpux
#include <sys/types.h>
#endif
#include <sys/param.h>
#include <sys/systm.h>
#undef NFS
#include <sys/mount.h>
#define NFS
#ifdef	hpux
#include <sys/signal.h>
#endif
#include <sys/fs.h>
#include <sys/conf.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/inode.h>
#ifndef	hpux
#include <sys/quota.h>
#endif


extern	int around[9];
extern	int inside[9];
extern	u_char *fragtbl[];

/*
 * Update the frsum fields to reflect addition or deletion 
 * of some frags.
 */
fragacct(fs, fragmap, fraglist, cnt)
	struct fs *fs;
	int fragmap;
	long fraglist[];
	int cnt;
{
	int inblk;
	register int field, subfield;
	register int siz, pos;

	inblk = (int)(fragtbl[fs->fs_frag][fragmap]) << 1;
	fragmap <<= 1;
	for (siz = 1; siz < fs->fs_frag; siz++) {
		if ((inblk & (1 << (siz + (fs->fs_frag % NBBY)))) == 0)
			continue;
		field = around[siz];
		subfield = inside[siz];
		for (pos = siz; pos <= fs->fs_frag; pos++) {
			if ((fragmap & field) == subfield) {
				fraglist[siz] += cnt;
				pos += siz;
				field <<= siz;
				subfield <<= siz;
			}
			field <<= 1;
			subfield <<= 1;
		}
	}
}


/*
 * block operations
 *
 * check if a block is available
 */
isblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	daddr_t h;
{
	unsigned char mask;

	switch (fs->fs_frag) {
	case 8:
		return (cp[h] == 0xff);
	case 4:
		mask = 0x0f << ((h & 0x1) << 2);
		return ((cp[h >> 1] & mask) == mask);
	case 2:
		mask = 0x03 << ((h & 0x3) << 1);
		return ((cp[h >> 2] & mask) == mask);
	case 1:
		mask = 0x01 << (h & 0x7);
		return ((cp[h >> 3] & mask) == mask);
	default:
		panic("isblock");
		return (NULL);
	}
}

/*
 * take a block out of the map
 */
clrblock(fs, cp, h)
	struct fs *fs;
	u_char *cp;
	daddr_t h;
{

	switch ((fs)->fs_frag) {
	case 8:
		cp[h] = 0;
		return;
	case 4:
		cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] &= ~(0x01 << (h & 0x7));
		return;
	default:
		panic("clrblock");
	}
}

/*
 * put a block into the map
 */
setblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	daddr_t h;
{

	switch (fs->fs_frag) {

	case 8:
		cp[h] = 0xff;
		return;
	case 4:
		cp[h >> 1] |= (0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] |= (0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] |= (0x01 << (h & 0x7));
		return;
	default:
		panic("setblock");
	}
}

