static char *HPUX_ID = "@(#) $Revision: 66.6 $";

/*
 *	du -- summarize disk usage
 *		du [-ars] [-t type] [name ...]
 */

#include	<stdio.h>
#include        <errno.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<ndir.h>
#include	<sys/inode.h>
#include        <sys/mount.h>
#ifdef SWFS
#include	<sys/swap.h>
#include	<sys/vfs.h>
#endif /* SWFS */

#define EQ(x,y)	(strcmp(x,y)==0)

#define RSIZE   512             /* reporting block size         */
#define UFS_NBLOCKS(s) \
    howmany(dbtob((s)->st_blocks), RSIZE)
#define XXX_NBLOCKS(s) \
    howmany((((s)->st_size + (DEV_BSIZE-1))/DEV_BSIZE)*DEV_BSIZE, RSIZE)

#ifdef SYMLINKS
#    define STAT lstat
#else
#    define STAT stat
#endif

/*
 * fsonlyval -- a bit mask that specifies which filesystem types to
 *              visit.  Special case of 0 indicates to visit all.
 */
#define ONLY_HFS   (1 << MOUNT_UFS)
#ifdef HP_NFS
#define ONLY_NFS   (1 << MOUNT_NFS)
#endif
#ifdef CDROM
#define ONLY_CDFS  (1 << MOUNT_CDFS)
#endif

unsigned int fsonlyval = 0;
#define DOFS(st) (fsonlyval == 0 || \
                  ((1 << (st).st_fstype) & fsonlyval) != 0)

struct stat Statb;
char path[MAXPATHLEN];

int aflag = 0;
#ifdef SWFS
int bflag = 0;
#endif /* SWFS */
int rflag = 0;
int sflag = 0;
int mountstop = 0;
dev_t lastdev;
int descended;
long descend();

#ifdef SWFS
#   define USAGE "usage: du [-abrsx] [-t type] [name ...]\n"
#else
#   define USAGE "usage: du [-arsx] [-t type] [name ...]\n"
#endif /* SWFS */

/*
 * printit() -- print the name of an entry along with its size.
 */
void
printit(n, name)
register long n;
char *name;
{
    extern char *ultoa();

    if (n == -1)
	n = 0;

    fputs(n == -1 ? "0" : ultoa(n), stdout);
    fputc('\t', stdout);
    fputs(name, stdout);
    fputc('\n', stdout);
}

main(argc, argv)
int argc;
char **argv;
{
    extern char *optarg;
    extern int optind;
    int c;
    long blocks = 0;

#ifdef SWFS
    while ((c = getopt(argc, argv, "abrsxt:")) != EOF)
#else
    while ((c = getopt(argc, argv, "arsxt:")) != EOF)
#endif
	switch (c)
	{
	case 'a': 
	    aflag = 1;
	    break;
#ifdef SWFS
	case 'b': 
	    bflag = 1;
	    break;
#endif
	case 'r': 
	    rflag = 1;
	    break;
	case 's': 
	    sflag = 1;
	    break;
	case 'x': 
	    mountstop = 1;
	    break;
	case 't': 
	    if (EQ(optarg, "hfs"))
		fsonlyval |= ONLY_HFS;
#ifdef HP_NFS
	    else if (EQ(optarg, "nfs"))
		fsonlyval |= ONLY_NFS;
#endif
#ifdef CDROM
	    else if (EQ(optarg, "cdfs"))
		fsonlyval |= ONLY_CDFS;
#endif
	    else
	    {
		fprintf(stderr,
			"du: invalid file system type %s\n", optarg);
		exit(1);
	    }
	    break;
	default: 
	Usage: 
	    fputs(USAGE, stderr);
	    exit(2);
	}

    argc -= optind;
    argv += (optind-1);

    if (argc == 0)
    {
	argc = 1;
	argv[1] = ".";
    }

    while (argc--)
    {
	lastdev = -1;
	strcpy(path, *++argv);
#ifdef SWFS
	if (bflag)
	    dofsswap(path);
#endif				/* SWFS */
	descended = 0;
#if defined(DUX) || defined(DISKLESS)
	blocks = descend(&path[strlen(path)], (ino_t)0);
#else
	blocks = descend(&path[strlen(path)]);
#endif
	if (!aflag && (sflag || (blocks != -1 && !descended)))
	    printit(blocks, path);
    }

#ifdef SWFS
    if (bflag)
	dofsswap(0);
#endif				/* SWFS */

    return 0;
}

#if defined(DUX) || defined(DISKLESS)
long descend(endp, ino)
char *endp;
ino_t ino;
#else
long descend(endp)
char *endp;
#endif
{
    DIR *dirp;
    struct direct *dir;
    long blocks;
    int rc;

    rc = STAT(path, &Statb);
#if defined (DUX) || defined(DISKLESS)
    /*
     * The previous descend passes in the inode that we expect
     * (or 0 if it dosen't know).  If we didn't get that
     * from stat(path) or the previous stat failed, it might be
     * a CDF so we add a '+' on the end and stat() it again.
     * If that stat fails or does not return a CDF, it might
     * be a mount point so we stat the entry again (without the +).
     * If that stat fails, we print a "bad status" message and
     * return -1.
     */
    if (rc == -1 || Statb.st_ino != ino)
    {
	*endp++ = '+';
	*endp   = '\0';
	if (STAT(path, &Statb) < 0 ||
	    !(Statb.st_mode & S_IFDIR) || !(Statb.st_mode & S_ISUID))
	{
            *--endp = '\0';
	    rc = STAT(path, &Statb);
	}
	else
	    rc = 0;
    }
#endif

    /*
     * If the stat fails, we print an error message if:
     *   o) This was a start point
     *   o) This was not a start point, they specified -r and errno
     *      isn't ENOENT (we ignore ENOENT errors because the file
     *      might have been unlinked after we read the directory).
     */
    if (rc < 0)
    {
	if (!descended && errno == ENOENT)
	{
	    fputs("du: ", stderr);
	    perror(path);
	    return -1;
	}
	if (!descended || (rflag && errno != ENOENT))
	    fprintf(stderr, "du: bad status < %s >\n", path);
	return -1;
    }

    /*
     * If mountstop is enabled, simply ignore this entry if it is
     * a mount point.
     */
    if (mountstop && lastdev != -1 && Statb.st_dev != lastdev)
	return -1;
    lastdev = Statb.st_dev;

    /*
     * If we don't want to do this filesystem type, we print an error
     * only if this is a start point and -r was specified.  Otherwise,
     * just ignore this thing.
     */
    if (!DOFS(Statb))
    {
	if (!descended && rflag)
	    fprintf(stderr,
		"du: < %s > not of specified file system type\n",
		path);
	return -1;
    }

    if (Statb.st_nlink > 1 && (Statb.st_mode & S_IFMT) != S_IFDIR &&
	was_counted(Statb.st_dev, Statb.st_ino))
	return 0;

    /*
     * Fix for FSDlj05896 -- for other file system types (especially
     * NFS) we don't know what their blocksize is, so we must use
     * the size field to guess at the number of blocks.
     * The only file system type we know about is UFS, so we only
     * use the block algorithm for those.
     */
    if (Statb.st_fstype == MOUNT_UFS)
	blocks = UFS_NBLOCKS(&Statb);
    else
	blocks = XXX_NBLOCKS(&Statb);

    if ((Statb.st_mode & S_IFMT) != S_IFDIR)
    {
	if (aflag)
	    printit(blocks, path);
	return blocks;
    }

    if ((dirp = opendir(path)) == NULL)
    {
	if (errno == EMFILE)
	    fputs("du: directory too deep\n", stderr);
	else
	    if (rflag)
		fprintf(stderr, "du: cannot open < %s >\n", path);
	return -1;
    }

    if ((dir = readdir(dirp)) == NULL)
    {
	if (rflag)
	    fprintf(stderr, "du: cannot read < %s >\n", path);
	closedir(dirp);
	return -1;
    }

    for (; dir != NULL; dir = readdir(dirp))
    {
	register char *c1, *c2;
	int i;
	long tmp_blocks;

	if (dir->d_ino == 0 || dir->d_ino == -1 ||
	    EQ(dir->d_name, ".") || EQ(dir->d_name, ".."))
	    continue;

	/*
	 * Avoid an infinite loop if the directory entry is corrupted.
	 * This should never happen, but...
	 */
	c2 = dir->d_name;
	if (c2 == NULL || *c2 == '\0')
	{
	    *endp = '\0';
	    fprintf(stderr, "du: bad dir entry in < %s >\n", path);
	    continue;
	}

	/*
	 * Copy this name to the end of the global "path".
	 */
	c1 = endp;
	if (*(c1 - 1) != '/')	/* do not print // */
	    *c1++ = '/';

	for (i = 0; i < MAXNAMLEN && c1 < path + sizeof path; i++)
	    if (*c2)
		*c1++ = *c2++;
	    else
		break;

	/*
	 * If we would have overflowed our path buffer, we print an
	 * error message for this entry and keep processing
	 */
	if (c1 >= path + sizeof path)
	{
	    *endp = '\0';
	    fprintf(stderr, "du: path name too long < %s/%.*s >\n",
		path, MAXNAMLEN, dir->d_name);
	    continue;
	}
	*c1 = '\0';
#if defined(DUX) || defined(DISKLESS)
	if ((tmp_blocks = descend(c1, dir->d_ino)) != -1)
#else
	if ((tmp_blocks = descend(c1)) != -1)
#endif
	    blocks += tmp_blocks;
    }

    (void)closedir(dirp);
    *endp = '\0';
    if (!sflag)
	printit(blocks, path);
    descended = 1;
    return blocks;
}

/*
 * was_counted() -- see if we have already counted (dev,ino).  Return
 *                  TRUE if we have; Add to a hash table and return
 *                  FALSE if we haven't.
 */
int
was_counted(dev, ino)
dev_t dev;
ino_t ino;
{
#   define MAXHASH 237
#   define ML	85
    static struct hash_tb
    {
	dev_t dev;
	ino_t ino;
	struct hash_tb *next;
    } ml[ML];
    static struct hash_tb *table[MAXHASH]; /* = (struct hash_tb *)0 */
    static struct hash_tb *mlbuff = ml;
    static int linkc = 0;
    register struct hash_tb *p;
    int hash_index;

    for (p = table[hash_index=(int)(ino % MAXHASH)]; p; p = p->next)
	if (p->ino == ino && p->dev == dev)
	    break;
    if (p)
	return 1;

    if (linkc >= ML)
    {
	mlbuff = (struct hash_tb *)
		    malloc(sizeof(struct hash_tb) * ML);
	linkc = 0;
	if (!mlbuff)
	{
	    fputs("du: link table overflow.\n", stderr);
	    exit(1);
	}
    }

    mlbuff[linkc].dev = dev;
    mlbuff[linkc].ino = ino;
    mlbuff[linkc].next = table[hash_index];
    table[hash_index] = &mlbuff[linkc];
    linkc++;
    return 0;
}

#ifdef SWFS
dofsswap(path)
char *path;
{
    static struct swapfs_info swapfs_buf1;
    static struct swapfs_info swapfs_buf2;
    static struct swapfs_info *swp1 = &swapfs_buf1;
    static struct swapfs_info *swp2 = &swapfs_buf2;
    static int got_swapinfo = 0;
    static int print_swapinfo = 0;

    if ((path == (char *)0) && got_swapinfo)
    {
	print_swapinfo = 1;
    }
    else if (!got_swapinfo)
    {
	struct statfs sfs;

	if (swapfs(path, swp1) == 0 && statfs(path, &sfs) == 0)
	{
	    /*
	     * Adjust sw_binuse to be in RSIZE chunks.
	     * To be consistent, sw_bavail and sw_breserve would need
	     * to be adjusted too, but we currently don't use them.
	     */
	    swp1->sw_binuse = (swp1->sw_binuse * sfs.f_bsize) / RSIZE;
	    got_swapinfo = 1;
	}
    }
    else
    {
	if (swapfs(path, swp2) == 0)
	{
	    if (strcmp(swp1->sw_mntpoint, swp2->sw_mntpoint) != 0)
	        print_swapinfo = 1;
	    else
	        print_swapinfo = 0;
	}
	else
	{
	    print_swapinfo = 1;
	    got_swapinfo = 0;
	}
    }

    if (print_swapinfo)
    {
	char buf[sizeof "Swapping to " + sizeof swp1->sw_mntpoint];
	struct swapfs_info *swp_tmp;

	strcpy(buf, "Swapping to ");
	strcat(buf, swp1->sw_mntpoint);
	printit(swp1->sw_binuse, buf);
	swp_tmp = swp1;
	swp1 = swp2;
	swp2 = swp_tmp;
	print_swapinfo = 0;
    }
}
#endif /* SWFS */
