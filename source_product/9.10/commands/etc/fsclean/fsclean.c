 static char HPUX_ID[] = "@(#) $Revision: 70.1 $";

/*  determine shutdown status of specified file system 
 *	Synopsis:  fsclean [ -v ] [ special ... ]
 */
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <ustat.h>
#include <sys/param.h>
#include <sys/fs.h>
#include <sys/inode.h>
#include <stdio.h>
#include <fcntl.h>
#include <mntent.h>
#include <sys/signal.h>
#include <sys/errno.h>

#define EQ(x,y) (strcmp(x,y)==0)

extern int errno;
char *cmd;
int verbose;
int dirty;
#ifdef QUOTA
int quota, qdirty;
char *getcdf();
#endif /* QUOTA */
int errors;
int err_is_tty;
struct mntent *mntent;

void checkit();

main(argc, argv)
int argc;
char **argv;
{
    int opt, i;
    FILE *fp;
    extern char *optarg;
    extern int optind;
    int checklist;

    cmd = argv[0];
#ifdef QUOTA
    while ((opt = getopt(argc, argv, "qv")) != EOF)
#else /* not QUOTA */
    while ((opt = getopt(argc, argv, "v")) != EOF)
#endif /* QUOTA */
	switch (opt)
	{
	case 'v': 		/* be verbose */
	    verbose++;
	    break;
#ifdef QUOTA
	case 'q': 		/* check FS_QFLAG (quota) */
	    quota++;
	    break;
#endif /* QUOTA */
	case '?': 
	    usage();
	    break;
	}

    err_is_tty = isatty(2);
    dirty = errors = 0;

    if ((argc - optind) == 0 || optind == 0)
    {
	fp = setmntent(MNT_CHECKLIST, "r");
	while ((mntent = getmntent(fp)) != NULL)
	{
	    if (EQ(mntent->mnt_type, MNTTYPE_HFS) &&
		    (hasmntopt(mntent, MNTOPT_RW) ||
			hasmntopt(mntent, MNTOPT_RO) ||
			hasmntopt(mntent, MNTOPT_DEFAULTS)))
#ifdef QUOTA
		if (!quota)
		    checkit(mntent->mnt_fsname);
		else if(hasmntopt(mntent, MNTOPT_QUOTA) &&
			(hasmntopt(mntent, MNTOPT_RW) ||
			hasmntopt(mntent, MNTOPT_DEFAULTS)))
		    checkit(mntent->mnt_fsname);
#else /* not QUOTA */
		checkit(mntent->mnt_fsname);
#endif /* QUOTA */
	}
    }
    else
	for (i = optind; !EQ(argv[i], ""); i++)
	    checkit(argv[i]);

    exit(dirty ? (errors ? 3 : 1)
	       : (errors ? 2 : 0));
}
			
int got_interrupt;

void
nothing()
{
    /*
     * This fuction does nothing, it is just here so we have a
     * signal handler for SIGINT.
     */
    got_interrupt = 1;
    return;
}

/*
 * In case we are trying a 7935 or 7937, we wait up to 6 minutes for
 * it to spinup.
 */
#define TRY_TIME   360
#define SLEEP_TIME  10
#define MSG_TIME    60

#if ((MSG_TIME % SLEEP_TIME) != 0)
    SLEEP_TIME must be a factor of MSG_TIME!
#endif

void
checkit(path)
char *path;
{
    int fd;
    int ret;
    char sblock[SBSIZE];
    struct fs *super;
    struct stat stdev, stslash;
    struct ustat ustdev;
    int root;
    int try_time = 0;
    struct sigaction action;

    if (stat(path, &stdev) == -1)
    {
	errors++;
	fprintf(stderr, "%s: cannot stat ", cmd);
	perror(path);
	return;
    }

    if ((stdev.st_mode & S_IFMT) != S_IFCHR &&
        (stdev.st_mode & S_IFMT) != S_IFBLK)
    {
	errors++;
	fprintf(stderr, "%s: %s: block or character device required\n",
	    cmd, path);
	return;
    }

    /*
     * Set up an handler for SIGINT, so that the user can interrupt
     * the retry of a device.
     */
    got_interrupt = 0;
    sigfillset(&action.sa_mask);
    action.sa_handler = nothing;
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);

    /*
     * Keep trying to open the device.  This is in case we are talking
     * to a disk that is still spinning up.  Wait for 6 minutes max,
     * trying every 15 seconds.
     * A message is printed every 60 seconds while we are waiting.
     * If an INTERRUPT signal is generated, we stop waiting and give
     * up on this device.
     */
    while ((fd = open(path, O_RDONLY)) == -1 &&
	    errno == ENXIO && try_time < TRY_TIME)
    {
	if (err_is_tty ||
	    try_time % MSG_TIME == 0 ||
	    (TRY_TIME - try_time) <= MSG_TIME)
	{
	    if (try_time == 0)
	    {
		fprintf(stderr, "%s: device %s not responding\n",
		    cmd, path);
		fputs("\tHit INTERRUPT to cancel retry and continue\n",
		    stderr);
	    }

	    fprintf(stderr, "\r\tRe-trying for %d more seconds...",
		TRY_TIME - try_time);
	    if (err_is_tty)
	       fputs("          \b\b\b\b\b\b\b\b\b\b", stderr);
	    else
	       fputc('\n', stderr);
	}

	sleep(SLEEP_TIME);
	if (got_interrupt)
	    try_time = TRY_TIME;
	else
	    try_time += SLEEP_TIME;
    }
    action.sa_handler = SIG_DFL;
    sigaction(SIGINT, &action, NULL);

    /*
     * If we printed any "retrying" messages to a terminal, we need to
     * terminate them with a newline.
     */
    if (err_is_tty && try_time != 0)
	fputc('\n', stderr);

    if (got_interrupt)
	errno = ENXIO;

    if (fd == -1)
    {
	errors++;
	fprintf(stderr, "%s: cannot open ", cmd);
	perror(path);
	return;
    }

    super = (struct fs *)sblock;
    if (bread(fd, super, SBLOCK, SBSIZE) == -1)
    {
	int save_errno = errno;

	errors++;
	fprintf(stderr, "%s: Could not read superblock of ", cmd);
	errno = save_errno;
	perror(path);
	close(fd);
	return;
    }

#if defined(FD_FSMAGIC)
    if ((super->fs_magic != FS_MAGIC) && (super->fs_magic != FS_MAGIC_LFN)
		&& (super->fs_magic != FD_FSMAGIC))
#else /* not new magic number */
#ifdef LONGFILENAMES
    if ((super->fs_magic != FS_MAGIC) && (super->fs_magic != FS_MAGIC_LFN))
#else				/* not LONGFILENAMES */
	if (super->fs_magic != FS_MAGIC)
#endif				/* not LONGFILENAMES */
#endif /* new magic number */
	{
	    errors++;
	    fprintf(stderr,
		    "%s: Incorrect magic number in superblock of %s\n",
		    cmd, path);
	    close(fd);
	    return;
	}

    /* determine if root device */
    root = 0;
    stat("/", &stslash);
    if ((stdev.st_rdev == -1) ||
	(minor(stslash.st_dev) == minor(stdev.st_rdev)))
	root++;

#ifdef QUOTA
    if (quota) { /* check the FS_QFLAG byte */
	int quotas_on = 0;
	FILE *mnttab;
	struct mntent *mnttab_ent;
#ifdef LOCAL_DISK
	char cdf[MNTMAXSTR + 1];
#else /* LOCAL_DISK */
	char *cdf;
#endif /* LOCAL_DISK */

	switch (FS_QFLAG(super))
	{
	case FS_QCLEAN: 
	    if (verbose)
		printf("%s: %s quotas clean\n", cmd, path);
	    break;
	case FS_QOK: 
	    /* determine if quotas are enabled */
	    /* FS_QOK && quotas_on == TRUE is expected */
#ifdef GETMOUNT
	    update_mnttab();
#endif /* GETMOUNT */
	    if ((mnttab=setmntent(MNT_MNTTAB, "r")) == NULL) {
		    fprintf(stderr,
			"%s: unable to open %s for quota check\n",
			cmd, MNT_MNTTAB);
		    errors++;
	    } else {
		/* find a matching entry in mnttab */
#ifdef LOCAL_DISK
		(void) getcdf(path, cdf, MNTMAXSTR);
#else /* LOCAL_DISK */
		cdf = path;
#endif /* LOCAL_DISK */
		while (mnttab_ent=getmntent(mnttab)) {
		    if (EQ(cdf, mnttab_ent->mnt_fsname)) {
			/* found a match */
			break;
		    }
		}
		(void)endmntent(mnttab);
		if (mnttab_ent && hasmntopt(mnttab_ent, MNTOPT_QUOTA)) {
		    quotas_on++;
		}
	    }
	    if (quotas_on) {
		if (verbose)
		    printf("%s: %s (quotas on) ok\n", cmd, path);
	    } else {
		dirty = 1;
		if (verbose)
		    printf("%s: %s quotas not clean, run quotacheck\n", cmd, path);
	    }
	    break;
	case FS_QNOTOK: 
	default: 	/* virgin file system, never had quotas */
	    dirty = 1;
	    if (verbose)
		printf("%s: %s quotas not ok, run quotacheck\n", cmd, path);
	    break;
	}
	close(fd);
	return;
    }
#endif /* QUOTA */

    switch (super->fs_clean)
    {
    case FS_CLEAN: 
	if (verbose)
	    printf("%s: %s clean\n", cmd, path);
	break;
    case FS_OK: 
	switch (root)
	{
	case 1: 
	    if (verbose)
		printf("%s: %s (root device) ok\n", cmd, path);
	    break;
	case 0: 
	    /* 
	     * Decide if the disk is mounted.
	     * We need the blk spec compliment for ustat
	     */
	    if ((stdev.st_mode & S_IFMT) == S_IFCHR)
	    {
		dirty = 1;
		if (verbose)
		    printf("%s: %s not clean, ok if mounted, otherwise run fsck\n",
			    cmd, path);
		break;
	    }

	    /* 
	     ** This ustat() call won't work correctly.
	     ** No cnode information is included, therefore
	     ** we may be stat'ing the wrong device.
	     */

	    if (ustat(stdev.st_rdev, &ustdev) == -1)
	    {
		/* not mounted */
		dirty = 1;
		if (verbose)
		    printf("%s: %s not clean, run fsck\n", cmd, path);
		break;
	    }
	    else
	    {
		/* mounted */
		if (verbose)
		    printf("%s: %s (mounted) ok\n", cmd, path);
		break;
	    }
	}
	break;
    case FS_NOTOK: 
	dirty = 1;
	if (verbose)
	    printf("%s: %s not ok, run fsck\n", cmd, path);
	break;
    default: 
	fprintf(stderr, "%s: unexpected value for fs_clean on %s\n",
	    cmd, path);
	errors++;
	break;
    }
    close(fd);
}

bread(fd, buf, blk, size)
int fd;
char *buf;
daddr_t blk;
long size;
{
#ifdef DEBUG
    int save_errno;
#endif
    int amount;

    if (lseek(fd, (long)dbtob(blk), 0) == -1)
    {
#ifdef DEBUG
	save_errno = errno;
	fprintf(stderr, "seek error, block = %d\n", blk);
	errno = save_errno;
#endif DEBUG
	return -1;
    }

    if ((amount = read(fd, buf, (int)size)) == size)
	return 0;

#ifdef DEBUG
    save_errno = errno;
    fprintf(stderr, "block read error, block = %d\n", blk);
    fprintf(stderr, "amount read = %d\n", amount);
    errno = save_errno;
#endif DEBUG

    return -1;
}

usage()
{
#ifdef QUOTA
	fprintf(stderr, "\nusage: fsclean [ -qv ] [ device_file ... ]\n");
	fprintf(stderr, "\t-q - report quota information validity\n");
#else
	fprintf(stderr, "\nusage: fsclean [ -v ] [ device_file ... ]\n");
#endif
	fprintf(stderr, "\t-v - be verbose about activity\n");
	fprintf(stderr, "\t   - (no device_file arguments) use /etc/checklist for filenames\n");
	exit(2);
}
