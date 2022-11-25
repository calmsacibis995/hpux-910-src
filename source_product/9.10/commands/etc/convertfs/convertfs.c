
static char *HPUX_ID = "@(#) $Revision: 70.3 $";

/*
 * convertfs: convert existing HFS file system to allow long file names.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/fs.h>
#include <stdio.h>
#include <sys/reboot.h>
#include <sys/sysmacros.h>
#include <checklist.h>

union {
	struct	fs sb;
	char pad[MAXBSIZE];
} sbun;
#define	sblock sbun.sb

int fi, hotroot = 0;
int silent=0;
struct fslist
{
	char *fsname;
	struct fslist *next;
};
char *rootdev;
struct fslist *filesyslist, *tmp;
struct stat stslash;

#define NO	0
#define	YES	1

char banner1[] =
"*****************************************************************\n\
*****                                                       *****\n\
***** Short file name to long file name conversion utility  *****\n\
*****                                                       *****\n\
*****************************************************************\n\n\
This utility converts standard HP-UX file systems with\n\
14-character maximum file names to support long file names\n\
up to 255 characters long.\n\n\
Warning: Conversion to long file names is irreversible and\n\
certain programs may not work with long file names.\n\
(See System Administrator's Manual for details.)\n\
\nConverting the root file system will cause a system reboot.\n\
The system should be shut down into single user state\n\
and all non-root file systems should be unmounted\n\
before this utility is run.\n\n";

char banner2[] =
"\nThis utility can convert all file systems in /etc/checklist\n\
or can allow you to pick and choose file systems.\n";

int convertfs();

main(argc, argv)
int argc;
char *argv[];
{
    int reply();
    char *rawname();
    int interactive = 1;
    int i = 0;
    char strbuf[BUFSIZ + 1];
    FILE *checklist;
    struct mntent *mnt;
    int errors = 0;

    if (argc > 1 && (!strcmp(argv[1], "-q")))
    {
	argc--;
	argv++;
	silent = 1;
    }

    if (!silent)
    {
	fputs(banner1, stdout);

	if (reply("Do you wish to continue [y/n]? ") == NO)
	{
	    printf("File systems unchanged.\n");
	    exit(1);
	}
    }				/* silent */

    chdir("/");
    if (stat("/", &stslash) < 0)
    {
	if (!silent)
	    fprintf(stderr, "Can't stat root\n");
	exit(11);
    }

    if (argc <= 1)
    {				/* no arguments specified */
	if (!silent)
	{
	    fputs(banner2, stdout);

	    if (reply("Do you wish to convert ALL file systems to allow long file names [y/n]? ") == YES)
		interactive = 0;
	}
	else
	    interactive = 0;

	if ((checklist = setmntent(MNT_CHECKLIST, "r")) == 0)
	{
	    if (!silent)
	    {
		perror(MNT_CHECKLIST);
		return NULL;
	    }
	    else
		exit(10);
	}

	filesyslist = (struct fslist *)malloc(sizeof (struct fslist));
	tmp = filesyslist;
	while ((mnt = getmntent(checklist)) != 0)
	{
	    if (strcmp(mnt->mnt_type, MNTTYPE_HFS) != 0)
		continue;

	    /* 
	     * Special case root file system for last.
	     */
	    if ((strlen(mnt->mnt_dir) == 1) && (mnt->mnt_dir[0] == '/'))
	    {
		rootdev = (char *)malloc(strlen(mnt->mnt_fsname) + 1);
		strcpy(rootdev, mnt->mnt_fsname);
		continue;
	    }

	    if (interactive)
	    {
		sprintf(strbuf,
		    "Convert %s usually mounted on %s [y/n]? ",
		    mnt->mnt_fsname, mnt->mnt_dir);
		if (reply(strbuf) == NO)
		    continue;
	    }

	    /*
	     * check raw device for non-root fs
	     */
	    filesyslist->fsname =
			    (char *)malloc(strlen(mnt->mnt_fsname) + 2);
	    strcpy(filesyslist->fsname, rawname(mnt->mnt_fsname));
	    filesyslist->next =
			(struct fslist *)malloc(sizeof (struct fslist));
	    filesyslist = filesyslist->next;
	}

	if (interactive)
	{
	    sprintf(strbuf, "Convert root file system %s [y/n]? ",
		rootdev);
	    if (reply(strbuf) == YES)
	    {
		filesyslist->fsname = rootdev;
		filesyslist->next =
			(struct fslist *)malloc(sizeof (struct fslist));
		filesyslist = filesyslist->next;
	    }
	}
	else
	{
	    filesyslist->fsname = rootdev;
	    filesyslist->next =
			(struct fslist *)malloc(sizeof (struct fslist));
	    filesyslist = filesyslist->next;
	}

	filesyslist->fsname = (char *)malloc(sizeof (int));
	strcpy(filesyslist->fsname, "");
	endmntent(checklist);
	while (strcmp(tmp->fsname, "") != 0)
	{
	    if (convertfs(tmp->fsname) == -1)
		errors++;
	    tmp = tmp->next;
	}
    }
    else
    {
	/* 
	 * File system to be converted have been passed in
	 * as argument to command.
	 */
	if (argc > 2)
	{
	    if (!silent)
	    {
		fprintf(stderr, "Usage: %s [special_file]\n", argv[0]);
		exit(1);
	    }
	    exit(2);
	}
	if (convertfs(argv[1]) == -1)
	    errors++;
    }
    return errors ? 9 : 0;
}

int
convertfs(special)
char *special;
{
    struct stat st;
    int i, status;
    char cmdbuf[MAXPATHLEN + 1];

    hotroot = 0;
    if (stat(special, &st) < 0)
    {
	if (!silent)
	{
	    perror(special);
	    fprintf(stderr, "Skipping %s\n", special);
	}
	else
	    fprintf(stderr,
	     "Skipped %s (Cannot obtain information on this device.)\n",
	     special);
	return -1;
    }
    if ((st.st_mode & S_IFMT) != S_IFBLK &&
	    (st.st_mode & S_IFMT) != S_IFCHR)
    {
	fprintf(stderr, "%s is not a block or character device",
	    special);
	return -1;
    }

    getsb(&sblock, special);
    if (stslash.st_dev == st.st_rdev)
    {
	hotroot++;
	sync();
	sleep(2);
	getsb(&sblock, special);
    }

    if (hotroot)
    {
	if (geteuid() != 0)
	{
	    fputs("Must be root to change root filesystem.\n", stderr);
	    fputs("Skipping root device.\n", stderr);
	    hotroot = 0;
	    return -1;
	}

	if (rootdev && strcmp(rootdev, special))
	{
	    fputs("Root file system in /etc/checklist is incorrect.\n",
		stderr);
	    fputs("Skipping root device.\n", stderr);
	    hotroot = 0;
	    return -1;
	}

	sync();		/* this marks in-core super-block clean */
	getsb(&sblock, special); /* re-read */
    }

    if (sblock.fs_magic == FS_MAGIC_LFN)
    {
	fprintf(stderr,
	    "Filesystem %s already allows long file names.\n", special);
	return 0;
    }

    if ((sblock.fs_clean != FS_CLEAN) && (hotroot == 0))
    {
	fprintf(stderr, "Filesystem %s is dirty.\n", special);
	fputs("Use convertfs only on clean, unmounted filesystem.\n",
	    stderr);
	return -1;
    }

    sblock.fs_magic = FS_MAGIC_LFN;
    if (hotroot == 0)
	sblock.fs_clean = FS_NOTOK;

    if (!silent)
	fprintf(stderr, "Converting %s\n", special);

    bwrite(SBLOCK, (char *)&sblock, SBSIZE);
    for (i = 0; i < sblock.fs_ncg; i++)
	bwrite(fsbtodb(&sblock, cgsblock(&sblock, i)),
		(char *)&sblock, SBSIZE);

    /* write all superblocks (except in-core) out to disk */
    sync();
    sleep(2);

    if (hotroot == 0)
    {
	/* fsck to compact . and ..  in directories into new format */
	if (!silent)
	{
	    fprintf(stderr, "Please wait, running fsck on %s ...\n",
		special);
	    fflush(stderr);
	}

	strcpy(cmdbuf, "/etc/fsck -p -q ");
	strcat(cmdbuf, special);
	status = system(cmdbuf);
	fflush(stdout);

	switch (status)
	{
	case 0: 
	    if (!silent)
		fprintf(stderr, "%s successfully converted.\n",
		    special);
	    return 0;
	case 12: 
	    if (!silent)
		fputs("Fsck interrupted, re-run fsck -p\n", stderr);
	    else
		fprintf(stderr,
		    "Fsck interrupted, please re-run fsck -p on %s\n",
		    special);
	    return -1;
	default: 
	    if (!silent)
		fputs("Fsck error, re-run fsck manually.", stderr);
	    else
		fprintf(stderr,
		    "Fsck interrupted, please re-run fsck -p on %s\n",
		    special);
	    return -1;
	}
    }
    else
    {				/* mounted root file system */
	if (!silent)
	{
	    fprintf(stderr,
		"Please wait, running fsck on root device %s\n",
		special);
	    fflush(stderr);
	}

	strcpy(cmdbuf, "/etc/fsck -p -q ");
	strcat(cmdbuf, special);
	status = system(cmdbuf);
	fflush(stdout);

	switch (status)
	{
	case 0: 
	    if (!silent)
		fprintf(stderr, "%s successfully converted.\n",
		    special);
	    return 0;
	case 12: 
	    if (!silent)
		fputs("\n\n\nfsck INTERRUPTED\n\n", stderr);
	case 8: 
	    fputs("\n\n\nfsck -P failed, RUN FSCK MANUALLY\n", stderr);
	    fputs("HALTING SYSTEM; REBOOT IN SINGLE USER MODE\n\n",
		stderr);
	    fflush(stderr);
	    sleep(2);
	    reboot(RB_NOSYNC | RB_HALT, "", "");
	case 4: 
	default: 
	    fputs("\007Rebooting system\n", stderr);
	    fflush(stderr);
	    sleep(2);
	    reboot(RB_NOSYNC, "", "");
	}
    }
}

getsb(fs, file)
register struct fs *fs;
char *file;
{

    fi = open(file, 2);
    if (fi < 0)
    {
	if (!silent)
	{
	    fprintf(stderr, "cannot open ");
	    perror(file);
	}
	exit(3);
    }

    if (bread(SBLOCK, (char *)fs, SBSIZE))
    {
	if (!silent)
	{
	    fprintf(stderr, "bad super block ");
	    perror(file);
	}
	exit(4);
    }

    if ((fs->fs_magic != FS_MAGIC) && (fs->fs_magic != FS_MAGIC_LFN))
    {
	if (!silent)
	    fprintf(stderr, "%s: bad magic number\n", file);
	exit(5);
    }
}

bwrite(blk, buf, size)
char *buf;
daddr_t blk;
register size;
{
    if (lseek(fi, blk * DEV_BSIZE, 0) == -1)
    {
	if (!silent)
	{
	    perror("FS SEEK");
	}
	exit(6);
    }
    if (write(fi, buf, size) != size)
    {
	if (!silent)
	{
	    perror("FS WRITE");
	}
	exit(7);
    }
}

bread(bno, buf, cnt)
daddr_t bno;
char *buf;
{
    register i;

    if (lseek(fi, bno * DEV_BSIZE, 0) == -1)
    {
	if (!silent)
	    exit(1);
	else
	    exit(6);
    }
    if ((i = read(fi, buf, cnt)) != cnt)
    {
	for (i = 0; i < sblock.fs_bsize; i++)
	    buf[i] = 0;
	return 1;
    }
    return 0;
}

reply(s)
char *s;
{
    char response[80];

    for (;;)
    {
	fputs(s, stdout);
	if (fgets(response, 80, stdin) == NULL)
	{
	    putchar('\n');
	    exit(1);
	}
	if ((response[0] == 'n' || response[0] == 'N'))
	    return 0;
	if ((response[0] == 'y' || response[0] == 'Y'))
	    return 1;
	else
	    fputs("Please answer 'y' or 'n'.\n", stdout);
    }
}

char *
rawname(cp)
char *cp;
{
    extern char *strrchr();
    static char rawbuf[32];
    char *dp;

    struct stat if_lvm;

    dp = strrchr(cp, '/');
    if (dp == 0)
	 return NULL;

    if (stat(cp, &if_lvm) < 0) {
    	 printf("Cannot access device parameters %s \n", cp);
	 return NULL;
    }
    else if(S_ISCHR(if_lvm.st_mode)) {
    	 (void)strcpy(rawbuf, cp);
      return rawbuf;
	 }
    else if(S_ISBLK(if_lvm.st_mode)) {
    	 while (dp >= cp && *--dp != '/');

    	/*  Check for AT&T Sys V disk naming convention
    	 *    (i.e. /dev/dsk/c0d0s3).
    	 *  or for the auto changer convention
    	 *    (i.e. /dev/ac/c0d1as2 or /dev/ac/1a)
    	 *  If it doesn't fit either of these,
    	 *    assume the LVM device naming convention
    	 *    /dev/vg00/lvol1 -> /dev/vg00/rlvol1
    	 *    or the old 200/300 naming
    	 *    convention (i.e. /dev/hd).
    	 */
    	if ((strncmp(dp, "/dsk", 4) != 0) && (strncmp(dp, "/ac", 3) != 0)) {
    	  dp = strrchr(cp, '/');
    	}

    	*dp = 0;
    	(void)strcpy(rawbuf, cp);
    	*dp = '/';
    	(void)strcat(rawbuf, "/r");
    	(void)strcat(rawbuf, dp+1);
    	return (rawbuf);
    }
    else {
	 printf("%s is neither a character nor a block device.\n", cp);
	 return NULL;
	 }
}
