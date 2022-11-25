/*
 * @(#)duxfs.h: $Revision: 1.4.83.3 $ $Date: 93/09/17 16:43:25 $
 * $Locker:  $
 */

/* information from superblock needed on client */

struct duxfs
{
	int	dfs_structsize;		/* actual size of this structure */
	long	dfs_bsize;		/* size of basic blocks in fs */
	long	dfs_fsize;		/* size of frag blocks in fs */
	long	dfs_bmask;		/* ``blkoff'' calc of blk offsets */
	long	dfs_fmask;		/* ``fragoff'' calc of frag offsets */
	long	dfs_bshift;		/* ``lblkno'' calc of logical blkno */
	long	dfs_fshift;		/* ``numfrags'' calc number of frags */
	char	dfs_fsmnt[MAXMNTLEN];	/* name mounted on (actually shorter)*/
	long	spare1;
	long	spare2;
	long	spare3;
	long	spare4;
	long	spare5;
};

/* The following macros are based in those in fs.h, but use a duxfs */

#define duxblkoff(dfs, loc)	/* calculates (loc % dfs->dfs_bsize) */ \
	((loc) & ~(dfs)->dfs_bmask)
#define duxlblkno(dfs, loc)	/* calculates (loc / dfs->dfs_bsize) */ \
	((loc) >> (dfs)->dfs_bshift)

/*Determine the size of a block.  This is equal to either fs_bsize, or the
 *size to the end of the inode, whichever is smaller
 */
#define dux_blksize(dfs, ip, lbn) \
	(((ip)->i_size >= (((lbn) + 1) << (dfs)->dfs_bshift)) \
		? (dfs)->dfs_bsize \
		: ((ip)->i_size - ((lbn) << (dfs)->dfs_bshift)))
