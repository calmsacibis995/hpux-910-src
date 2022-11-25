static char *HPUX_ID = "@(#) $Revision: 70.1 $";

/*  This program does not report info about swap devices
 *  This program can be shared
 */
#include <sys/param.h>		/* for MAXPATHLEN */
#include <stdio.h>
#include <mntent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ndir.h>
#include <string.h>

typedef struct fst
{
    char *filesys;
    dev_t dev;
    struct fst *next;
} filesystemlist;


/************************************************************************
 * MAIN
 *
 *   Algorithm is to first read and stat all the entries in mnttab to
 *   obtain device ID's. Then, for each pathname given on the command line,
 *   first compare against special device files read from mnttab and if
 *   no match is found, then search the /dev directory. Most of the time
 *   the match will come from mnttab as syncer(1m) will keep it up-to-date
 *   with the kernel.
 *
 */

main(argc, argv)
  int	argc;
  char	*argv[];
{
    FILE *mnttab;
    struct mntent *mnt;
    filesystemlist *head = NULL, *tail;
    char fname[MAXPATHLEN];
    struct stat sbuf;
    dev_t fno;
    short found;
    char *malloc();
    char *scan();


/* READ MNTTAB ENTRIES */
    
    mnttab = setmntent(MNT_MNTTAB, "r");	
    
    if (mnttab != NULL) 
    {
	while ((mnt = getmntent(mnttab)) != NULL) {
	    if (stat(mnt->mnt_dir, &sbuf) < 0)
		continue;
	    
	    if (head == NULL)
		head = tail =
		    (filesystemlist *)malloc(sizeof(filesystemlist));
	    else {
		tail->next =
		    (filesystemlist *)malloc(sizeof(filesystemlist));
		tail = tail->next;
	    }
	    
	    tail->next = NULL;
	    tail->filesys = malloc(strlen(mnt->mnt_fsname) + 1);
	    (void) strcpy(tail->filesys, mnt->mnt_fsname);
	    tail->dev = sbuf.st_dev;
	}
    }


/* PROCESS PATHNAMES PASSED ON THE COMMAND LINE */
    
    while(--argc) {
	found = 0;
	if (stat(*++argv, &sbuf) == -1) {
	    fputs("devnm: ", stderr);
	    perror(*argv);
	    continue;
	}
	
	fno = sbuf.st_dev;
	
	/*
	 * Compare against entries in mnttab
	 */
	for (tail = head; tail != NULL; tail = tail->next)
	{
	    if (fno == tail->dev)
	    {
		found = 1;
		fputs(tail->filesys, stdout);
		fputc(' ', stdout);
		fputs(*argv, stdout);
		fputc('\n', stdout);
		break;
	    }
	}
	
	/*
	 * If not found in mnttab, then search all the files
	 * in /dev and its subdirectories (excluding tty* and pty*
         * files).
	 */
	if (!found)
	{
	    if (scan ("/dev", fno, fname, sizeof(fname)) != NULL)
	    {
		fputs(fname, stdout);
		fputc(' ', stdout);
		fputs(*argv, stdout);
		fputc('\n', stdout);
	    }
	    else
	    {
		fputs("Could not find device for ", stderr);
		fputs(*argv, stderr);
		fputc('\n', stderr);
	    }
	}
    }
    exit(0);
}

/************************************************************************
 * SCAN
 *
 *    Scan through a directory looking for a special device file with
 *    a device id which matches the supplied id. The assumption is that
 *    we are looking for a mountable device on which a file could reside.
 *    Thus, in order to speed the performance along, we don't even bother
 *    looking at files which start with "pty" or "tty". 
 */

char *scan (dir, id, buf, len)
  char	*dir;
  dev_t	id;
  char	*buf;
  size_t len;
{
    DIR		*dirp;
    char   	fname[MAXPATHLEN], *p;
    struct direct *dbuf;
    struct stat sbuf;

    if ((dirp = opendir(dir)) == NULL)
	return NULL;
    
    while ((dbuf = readdir(dirp)) != NULL) 
    {
	if (!dbuf->d_ino) 
	    continue;

	/* Assumes that what we are looking for cannot be named
	 * pty* or tty*  
	 */
	if (strncmp (dbuf->d_name, "tty", 3) == 0  ||
	    strncmp (dbuf->d_name, "pty", 3) == 0)
	    continue;
	
	strcpy(fname, dir);
	strcat(fname, "/");
	strcat(fname, dbuf->d_name);
	if (stat(fname, &sbuf) == -1) {
#if defined(DISKLESS) || defined(DUX)
	    (void) strcat(fname, "+");
	    if (stat(fname, &sbuf) == -1)
#endif defined(DISKLESS) || defined(DUX)
		fputs ("/dev stat error\n", stderr);
	    continue;
	}

	/* Found a match!
	 */
	if ((id == sbuf.st_rdev) && ((sbuf.st_mode & S_IFMT) == S_IFBLK))
	{
	    closedir(dirp);
	    return strncpy (buf, fname, len);
	}    
	    
	/* Check to see if this is a directory which must be searched
	 */
	if (((sbuf.st_mode & S_IFMT) == S_IFDIR) &&
	    (strcmp (dbuf->d_name, ".") != 0) &&
	    (strcmp (dbuf->d_name, "..") != 0))
	{
	    if ((p = scan (fname, id, buf, len)) != NULL)
	    {
		closedir(dirp);
		return (p);
	    }
	}
	
    }
    closedir(dirp);
    return NULL;
}
