/* new hfs utilities */

/*
 *  This program is the creation of Rob Gardner, and is not for
 *  sale, trade, or barter. It may not be copied for any reason.
 *  It may not even be examined, and as a matter of fact, it is
 *  forbidden for human eyes to see this program. You are probably
 *  violating this decree right now. This program is probably the
 *  property of Hewlett-Packard, since they own me and everything
 *  I have ever done. May the Source be with you.
 */

#include "hfsio.h"

struct opendiskstruct mountab[MAXMOUNT];

/*
 * lookup dev in mount table and return an open file descriptor for the device
 */
int devtofd(dev)
    dev_t dev;
{
    int i;
    
    init_mountab();
    
    for (i=0; i < MAXMOUNT; i++) 
    {
	if (mountab[i].dev == dev) 
	{
	    /* if device not open yet, do it now */
	    if (mountab[i].fd == -1)
		if (opendev(dev) != 0)
		{
		    panic("Could not open the appropriate device.");
		    return(-1);
		}
	    return(mountab[i].fd);
	}
    }
    if (i >= MAXMOUNT) 
	panic("dev not found in mountab (devtofd)");
    return(-1);
}

/*
 * lookup dev in mount table and return the open file descriptor
 * for the raw device
 */
int devtorfd(dev)
    dev_t dev;
{
    int i;
    
    init_mountab();
    
    for (i=0; i < MAXMOUNT; i++) 
    {
	if (mountab[i].dev == dev) 
	{
	    /* if device not open yet, do it now */
	    if (mountab[i].fd == -1)
		if (opendev(dev) != 0)
		{
		    panic("Could not open the appropriate device.");
		    return(-1);
		}
	    return(mountab[i].rfd);
	}
    }
    if (i >= MAXMOUNT) 
	panic("dev not found in mountab (devtorfd)");
    return(-1);
}


/*
 * lookup dev in mount table and return pointer to superblock for the device
 */
struct fs *devtosuper(dev)
    dev_t dev;
{
    int i;
    
    init_mountab();
    for (i=0; i < MAXMOUNT; i++) 
    {
	if (mountab[i].dev == dev) 
	{
	    /* if device not open, do it now */
	    if (mountab[i].fs == NULL) 
		if (opendev(dev) != 0) 
		{
		    panic("Could not open the appropriate device.");
		    return((struct fs *)0);
		}
	    
	    return(mountab[i].fs);
	}
    }
    if (i >= MAXMOUNT) 
	panic("dev not found in mountab (devtosuper)");
    return((struct fs *)0);
}

    
/*
 * open a device in our mount table and read in super block
 */
int opendev(dev)
    dev_t dev;
{
    int blks, i, size, ret, dfd;
    char *space;
    char buf[8192];
    struct fs *fs;
    
    for (i=0; i < MAXMOUNT; i++) 
	if (mountab[i].dev == dev)
	    break;
    if (i >= MAXMOUNT) 
    {
	fprintf(stderr, "dev not found in mountab (opendev)");
	return(-1);
    }
    if (mountab[i].fd != -1)
	return(0);

    mountab[i].rfd = open(mountab[i].rpath, O_RDONLY);
    if (mountab[i].rfd < 0) 
    {
	if (errno == EOPNOTSUPP) 
	{
	    perror(mountab[i].rpath);
	    fprintf(stderr, "   [Probably a non-local filesystem.]\n");
	}
	else
	    perror(mountab[i].rpath);
	return(-1);
    }
    mountab[i].fd = open(mountab[i].path, O_RDONLY);
    if (mountab[i].fd < 0) 
    {
	if (errno == EOPNOTSUPP) 
	{
	    perror(mountab[i].path);
	    fprintf(stderr, "   [Probably a non-local filesystem.]\n");
	}
	else
	    perror(mountab[i].path);
	return(-1);
    }
    
    /* allocate space for super block and read it in */
    mountab[i].fs = (struct fs *) malloc(SBSIZE);
    if (mountab[i].fs == NULL) 
	panic("malloc failed for sbsize in init");

    /* super block is 2nd block on device (boot block is first) */
    lseek(mountab[i].fd, 8192, SEEK_SET);
    if (read(mountab[i].fd, mountab[i].fs, SBSIZE) != SBSIZE) 
    {
	perror("read sb in init");
	exit(19);
    }

    
    /* check to make sure we have a valid super block */
    if (mountab[i].fs->fs_magic != FS_MAGIC
	&& mountab[i].fs->fs_magic != FS_MAGIC_LFN) 
    {
	unsigned int offset;
	int continue_flag = 0;
	
	/* duh, how about getting an alternate superblock?? */

	fprintf(stderr, "Bad super block magic number for %s\n",
		mountab[i].path);
	fprintf(stderr, "Hang on while I look for a spare.");
	fprintf(stderr, " (This could take a while...)\n");
	
	offset = 2*8192;
	
	while (1) 
	{
	    if (lseek(mountab[i].fd, offset, SEEK_SET) < 0)
		perror("lseek");
	    
	    ret = read(mountab[i].fd, mountab[i].fs, SBSIZE);
	    if (ret < 0)
	    {
		perror("read sb in init");
		fprintf(stderr, "Trouble reading block %d; ", offset/8192);
		if (continue_flag) 
		{
		    fprintf(stderr, "Continuing.\n");
		    continue;
		}
		    
		fprintf(stderr, "Continue? ");
		gets(buf);
		if (buf[0] == 'y' && buf[0] == 'Y') 
		{
		    continue_flag = 1;
		    continue;
		}
		else
		    break;
	    }
	    else if (ret != SBSIZE)
		break;
	    
	    if ( (mountab[i].fs->fs_magic != FS_MAGIC) &&
		(mountab[i].fs->fs_magic != FS_MAGIC_LFN) )
		offset += 8192;
	    else 
	    {
		fprintf(stderr, "I found a spare superblock at block %d\n",
			offset/8192);
		goto sb_ok;
	    }
	}
	fprintf(stderr, "I'm sorry, I could not find a spare. Hose city.\n");
	mountab[i].dev = -1;
	close(mountab[i].fd);
	mountab[i].fd = -1;
	mountab[i].fs = NULL;
	return;
    }
sb_ok:
    fs = mountab[i].fs;
    dfd = mountab[i].fd;

    /* read cg summary data */
    
    blks = howmany(fs->fs_cssize, fs->fs_fsize);
    space = (char *) malloc(fs->fs_cssize);
    if (space == NULL) 
    {
	fprintf(stderr, "malloc failed\n");
	errno = ENOMEM;
	return(NULL);
    }

    for (i=0; i < blks; i += fs->fs_frag) 
    {
	size = fs->fs_bsize;
	if (i + fs->fs_frag > blks)
	    size = (blks - i) * fs->fs_fsize;
	lseek(dfd, (fs->fs_csaddr+i)*fs->fs_fsize, SEEK_SET);
	ret = read(dfd, buf, size);
	if (ret != size) 
	{
	    perror("read csum in opendisk");
	    return(NULL);
	}
	memcpy(space, buf, size);
	fs->fs_csp[i/fs->fs_frag] = (struct csum *) space;
	space += size;
    }

    if (mountab[i].fs->fs_clean == FS_NOTOK) 
	fprintf(stderr, "warning: file system not clean\n");
    
    /* duh, for unrm, of course the file system is mounted */
    /*
      if (mountab[i].fs->fs_clean == FS_OK) 
      fprintf(stderr, "warning: file system might be mounted\n");
    */	
    return(0);
}

/*
 * read /etc/mnttab, and create entries in our mount table
 */
int init_mountab() 
{
    FILE *fp;
    char *s, line[200];
    int i, dev;
    static int init_done = 0;
    
    if (init_done)
	return(0);

    init_done = 1;
    
    for (i=0; i<MAXMOUNT; i++) 
    {
	mountab[i].fs = NULL;
	mountab[i].fd = -1;
	strcpy(mountab[i].path, "");
	mountab[i].dev = -1;
    }
    
    fp = fopen("/etc/mnttab", "r");
    if (fp == NULL) 
    {
	perror("/etc/mnttab");
	exit(1);
    }
    
    while (fgets(line, sizeof(line), fp) != NULL) 
    {
	/* maybe we could detect nfs volumes here? */
	if (line[0] != '/')
	    continue;
	s = strchr(line, ' ');
	if (s == NULL)
	    continue;
	*s = '\0';
	
	dev = add_dev(line);
    }

    fclose(fp);
    return(0);
}


	
/*
 * add this block device to our mount table
 */
add_dev(line)	
    char *line;
{
    struct stat foo;
    int i;
    
    init_mountab();
    if (stat(line, &foo) != 0)
    {
	/*perror(line);*/
	return(-1);
    }

    if (!S_ISBLK(foo.st_mode)) 
    {
	fprintf(stderr, "%s is not a block special file\n", line);
	/*return(-1);*/
    }

    for (i=0; i < MAXMOUNT; i++) 
	if (mountab[i].dev == -1) 
	{
	    mountab[i].dev = foo.st_rdev;
	    strcpy(mountab[i].path, line);
	    ffindisk(line, block_to_raw(foo.st_rdev), "/dev/rdsk");
	    strcpy(mountab[i].rpath, line);
	    return(foo.st_rdev);
	}	    

    panic("out of space in mountab");
}


/*
 * read stuff from /dev/kmem and be nice enough to allocate
 * space for the stuff we're reading, then return a pointer to it.
 */
char *kread(addr, size)
    int addr, size;
{
    char *x = (char *) malloc(size);
    static int kmemfd = -1;
    
    if (kmemfd == -1) 
    {
	kmemfd = open("/dev/kmem", O_RDONLY);
	if (kmemfd < 0) 
	{
	    perror("open of /dev/kmem");
	    yourexit(2);
	}
    }    

    if (x == NULL) 
    {
	fprintf(stderr, "malloc failed\n");
	yourexit(92);
    }
    
    if (lseek(kmemfd, addr, SEEK_SET) != addr) 
    {
	perror("lseek");
	yourexit(17);
    }
    if (read(kmemfd, x, size) != size) 
    {
	perror("read");
	fprintf(stderr, "Dumping core...\n");
	abort();
    }
    
    return(x);
}



/*	
 * read specified block from specified device, and return pointer to
 * malloced area containing the data
 */
char *breadn(dev, bn, size, rawflag)
    dev_t dev;
    int bn, rawflag;
{
    char *x;
    int i, j, where, dfd, whichfrag;
    struct fs *fs;
    
    if (rawflag)
	dfd = devtorfd(dev);
    else
	dfd = devtofd(dev);
    fs = devtosuper(dev);
    
    x = (char *) malloc(fs->fs_bsize);
    if (x == NULL) 
	panic("malloc failed!");
    
    whichfrag = bn % fs->fs_frag;
    
    where = (bn - whichfrag) * fs->fs_fsize;
    for (j=0; j<3; j++)
    {
	lseek(dfd, where, SEEK_SET);
	if (read(dfd, x, fs->fs_bsize) == fs->fs_bsize) 
	    return(x + whichfrag*fs->fs_fsize);
	perror("breadn continuing");
    }
    perror("breadn");
    return(NULL);
}    

/*	
 * read specified block from specified device, and return pointer to
 * malloced area containing the data
 */
char *bread(dev, bn, rawflag)
    dev_t dev;
    int bn, rawflag;
{
    struct fs *fs = devtosuper(dev);
    if ( (bn % fs->fs_frag) != 0)
	fprintf(stderr, "bread warning: odd block number %d\n", bn);
    
    return(breadn(dev, bn, fs->fs_bsize, rawflag));
}


/*
 * read cylinder group block, and return pointer to
 * malloced area containing it 
 */
struct cg *read_cg_block(dev, cgnum)
    int cgnum;
{
    struct cg *cgp;
    struct fs *fs = devtosuper(dev);
    
    cgp = (struct cg *) bread(dev, cgtod(fs, cgnum), 0);
    if (cgp == NULL) 
    {
	perror("dread in read_cg_block");
	return(NULL);
    }
    
    if (cgp->cg_magic != CG_MAGIC) 
    {
	fprintf(stderr, "bad cg magic number in read_cg_block\n");
	return(NULL);
    }
    
    return(cgp);
}

panic(str)
    char *str;
{
    fflush(stdout);
    fprintf(stderr, "%s\n", str);
    yourexit(17);
}


int min(x, y)
    int x, y;
{
    if (x < y)
	return(x);
    else
	return(y);
}


char *lsmodes(mode)
int mode;
{
    static char modes[11];
    int i, m;
    
    strcpy(modes, "----------");
    
    switch (mode & IFMT) {
      case IFDIR:
	modes[0] = 'd';
	break;
      case IFCHR:
	modes[0] = 'c';
	break;
      case IFBLK:
	modes[0] = 'b';
	break;
      case IFIFO:
	modes[0] = 'p';
	break;
      case IFLNK:
	modes[0] = 'l';
	break;
      default:
	break;
    }
    
    m = mode;
    
    for (i=7; i>=1 ; i=i-3) 
    {
	if (m & 04)
	    modes[i] = 'r';
	if (m & 02)
	    modes[i+1] = 'w';
	if (m & 01)
	    modes[i+2] = 'x';

	m = m >> 3;
    }
    
    if (mode & S_ISUID) 
	modes[3] = 's';
    
    if (mode & S_ISGID)
	modes[6] = 's';
    
    if (mode & S_ISVTX)
	modes[9] = 't';

    return(modes);
}	



/*
 * produce  'ls -l' style file information, stuff into target
 */
lsinfo(i, target)
    struct dinode *i;
    char *target;
{
    char tmp[100], *tim;
    struct passwd *pw;
    struct group *gr;
    
    *target = '\0';
    strcat(target, lsmodes(i->di_mode));
    sprintf(tmp, " %3d ", i->di_nlink);
    strcat(target, tmp);
    
    pw = getpwuid(i->di_uid);
    if (pw != NULL) 
	sprintf(tmp, "%-8s", pw->pw_name);	
    else
	sprintf(tmp, " %06d ", i->di_uid);
    strcat(target, tmp);

    gr = getgrgid(i->di_gid);
    if (gr != NULL) 
	sprintf(tmp, " %-8s", gr->gr_name);	
    else
	sprintf(tmp, " %06d  ", i->di_gid);
    strcat(target, tmp);

    if ( ((i->di_mode & IFMT) == IFCHR) || ((i->di_mode & IFMT) == IFBLK) )
	sprintf(tmp, "%4d 0x%06x ", major(i->di_rdev), minor(i->di_rdev));
    else
	sprintf(tmp, "%8d ", i->di_size);
    strcat(target, tmp);

    tim = nl_cxtime(&i->di_ctime, "%b %d %H:%M:%S");
    strcat(target, tim);
	   
    return;
}


/*
 * Convert a block major number to a raw major number.
 * This is done by looking up the address of the open routine for
 * the given block major number (in bdevsw[]) and then searching
 * cdevsw[] for an equivalent open routine.
 */
block_to_raw(dev)
    dev_t dev;
{
    struct nlist syms[10];
    struct bdevsw *bdevsw;
    struct cdevsw *cdevsw;
    int *nchrdev, *nblkdev, i;
    
    memset(syms, 0, sizeof(struct nlist) * 10);
    
#ifdef hp9000s300
    syms[0].n_name = "_cdevsw";
    syms[1].n_name = "_bdevsw";
    syms[2].n_name = "_nchrdev";
    syms[3].n_name = "_nblkdev";
#endif    
#ifdef hp9000s800
    syms[0].n_name = "cdevsw";
    syms[1].n_name = "bdevsw";
    syms[2].n_name = "nchrdev";
    syms[3].n_name = "nblkdev";
#endif    

    if (nlist("/hp-ux", syms) != 0)
	fprintf(stderr, "nlist failed in block_to_raw! ha ha ha!\n");
    
    for (i=0; i<3; i++) 
	if (syms[i].n_value == 0) 
	{
	    fprintf(stderr, "could not read %s\n", syms[0].n_name);
	    return(-1);
	}
    nchrdev = (int *)kread(syms[2].n_value, sizeof(int));
    nblkdev = (int *)kread(syms[3].n_value, sizeof(int));
    
    cdevsw = (struct cdevsw *)
		kread(syms[0].n_value, *nchrdev * sizeof(struct cdevsw));
    bdevsw = (struct bdevsw *)
		kread(syms[1].n_value, *nblkdev * sizeof(struct bdevsw));

    if (major(dev) >= *nblkdev) 
    {
	fprintf(stderr, "bad dev %x in block_to_raw\n", dev);
	return(-1);
    }
    
    for (i=0; i<*nchrdev; i++) 
    {
	if (cdevsw[i].d_open == bdevsw[major(dev)].d_open) 
	{
	    /*printf("block %d ---> char %d\n", major(dev), i);*/
	    return( makedev(i, minor(dev)) );
	}
    }
    return(-1);
}


/*
 * Search through /dev/dsk looking for a device file for a specific dev #
 */
findisk(name, devkey)
    char *name;
    int devkey;
{
    ffindisk(name, devkey, "/dev/dsk");
}


ffindisk(name, devkey, dirname)
    char *name, *dirname;
    int devkey;
{
    DIR *devdir;
    struct dirent *dp;
    struct stat dstat;
    

/*
    printf("major(devkey)=%d, minor(devkey)=0x%x\n", 
	   major(devkey), minor(devkey));
*/

    devdir = opendir(dirname);
    if (devdir == NULL) 
    {
	perror(dirname);
	exit(3);
    }
    
    while ( (dp = readdir(devdir)) != NULL ) 
    {
	char path[255];

	sprintf(path, "%s/%s", dirname, dp->d_name);
	if (stat(path, &dstat) != 0) 
	{
	    perror(path);
	    continue;
	}
	
/*
	printf("%s: major=%d, minor=0x%x\n", path,
	   major(dstat.st_rdev), minor(dstat.st_rdev));
*/
/**********
	if (minor(dstat.st_rdev) == minor(devkey))
***********/
	if (dstat.st_rdev == devkey)
	{
	    strcpy(name, path);
	    closedir(devdir);
	    return;
	}
    }

    closedir(devdir);
    return;
}
