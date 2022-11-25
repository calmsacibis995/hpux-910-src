/* un-remove program for hfs */

/*
 *  This program is the creation of Rob Gardner, and is not for
 *  sale, trade, or barter. It may not be copied for any reason.
 *  It may not even be examined, and as a matter of fact, it is
 *  forbidden for human eyes to see this program. You are probably
 *  violating this decree right now. This program is probably the
 *  property of Hewlett-Packard, since they own me and everything
 *  I have ever done. May the Source be with you.
 */


/*
$Compile: cc -v -g -c %f
*/

    
    /* ideas:
	- check to see if we're restoring it on the same filesystem
	  as the deleted one, and print a warning and prompt for confirmation.
	- also check to see if this is the same directory as the original, and 
	  issue a warning.
    */

#include "hfsio.h"

static char *xyzzy = "********   WHAT THE HECK ARE YOU DOING RUNNING STRINGS ON MY PROGRAM?????!!!!";

static char *HPUX_ID = "@(#) $unrm revision: Wed Dec 22 12:32:11 MST 1993 $";

char progname[100];		/* for usage() */

/*
 * list_struct defines a two-dimensional linked list; one dimension is
 * for files, and the other is for blocks within a file.
 */
struct list_struct 
{
    char *data;			/* points to a malloc'd block of data */
    unsigned int blockno;	/* block number (on disk) of data */
    int size;			/* how many bytes in block */
    int tag;			/* entry number in unrm_table */
    struct list_struct *next_block;
    struct list_struct *next_file;
};

struct list_struct *make_block_list();

#define LNULL ((struct list_struct *)0)
#define LIST_ALL -1


/* kernel saved unlink info variables */
int *unrm_table_size, *unrm_table_addr, *unrm_use_next;
struct unrm_table_t *unrm_table = NULL;

/* space to save the kernel info for when we re-read it */
int old_use_next;
struct unrm_table_t *old_table = NULL;


/* various other declarations */
void sigcatch();
char *ask();

/* global debug- set with -d option */
int unrm_debug = 0;



main(argc, argv)
    int argc;
    char *argv[];
{
    int ret, h, ofd, force = 0, listall = 0;
    extern int optind;
    char c, *tag;
    struct list_struct *top = NULL, *blockp, *bp;
    regex_t preg;

    
    strcpy(progname, argv[0]);
    signal(SIGINT, sigcatch);
    
    if ( (getegid() != 3) && (geteuid() != 0) )		/* group sys */
    {
	fprintf(stderr, "%s must be run as group sys.\n", progname);
	exit(3);
    }


    while ( (c = getopt(argc, argv, "hH?lfd")) != EOF)
    {
	switch (c) 
	{
	  case 'd':		/* debug option */
	    unrm_debug++;
	    break;
	    
	  case 'f':		/* don't ask questions */
	    force++;
	    break;
	    
	  case 'l':		/* just list */
	    listall++;
	    break;

	  case '?':		/* usage */
	  case 'h':
	  case 'H':
	    usage();
	    break;

	  default:
	    usage();
	    break;
	}
    }

    ret = read_unrm_data();
    if (ret == 1) 
    {
	fprintf(stderr, "No unrm data in this kernel.\n");
	exit(1);
    }
    if (ret == 2) 
    {
	fprintf(stderr, "No useful unrm data found. If diskless, ");
	fprintf(stderr, "try %s on the rootserver.\n", progname);
	exit(2);
    }
    old_table = unrm_table;
    old_use_next = *unrm_use_next;
    
    if (listall) 
    {
	listrm(LIST_ALL);
	exit(0);
    }
	
    
    if (argc == optind)		/* no args given */
    {
	listrm(geteuid());
	exit(0);
    }
    

    sync();
    sleep(1);

    /* allocate tag table */
    tag = (char *)malloc(*unrm_table_size);
    memset(tag, 0, *unrm_table_size);

    
    /* for each argument, treat it as a regular expression, and compare */
    /* it with each entry in the unrm table, tagging the matches. */
    for (; optind < argc; optind++) 
    {
	char pattern[256];
	int matches = 0;
	
	strcpy(pattern, argv[optind]);
	preprocess(pattern);
	if ( (ret=regcomp(&preg, pattern, 0)) != 0) 
	{
	    fprintf(stderr, "regcomp failed on '%s' with error %d\n",
		    argv[optind], ret);
	    continue;
	}

	/* if an entry in unrm_table matches a pattern, make a mark */
	/* in the tag table to remember the event. */
	for (h=0; h<*unrm_table_size; h++) 
	    if (regexec(&preg, unrm_table[h].fname, 0, 0, 0) == 0) 
	    {
		tag[h] = 1;
		matches++;
	    }
	if (matches == 0)
	    fprintf(stderr, "No match found for '%s'\n", argv[optind]);
    }
    
    if (unrm_debug) 
    {
	fprintf(stderr, "straight matches:\n");
	for (h=0; h<*unrm_table_size; h++) 
	    if (tag[h]) 
	    {
		char ls_line[200];
		lsinfo(&unrm_table[h].din, ls_line);
		fprintf(stderr, "%s  %s\n", ls_line, unrm_table[h].fname);
	    }
    }
    

    /* now go through the table again looking for tagged items; for each */
    /* tagged item, go through the entire table looking for items with */
    /* the same name, and unmark the 'older' entry. in this way, we are */
    /* left with a list of only the most recently deleted files. */
    for (h=0; h<*unrm_table_size; h++) 
	if (tag[h]) 
	{
	    int key = h;
	    int i;
	    
	    for (i=0; i<*unrm_table_size; i++) 
		if (strcmp(unrm_table[key].fname, unrm_table[i].fname) == 0) 
		{
		    if (i == key)
			continue;
		    
    /* files with the same name are ok if they are from a different device */
    /* or a different directory... */
		    if ( (unrm_table[i].dir_dev != unrm_table[key].dir_dev) ||
			(unrm_table[i].dir_inum != unrm_table[key].dir_inum))
			continue;
		    
		    /* remember only the newest file, forget the other */
		    /******** gack, mtime does not reflect file deletion time!
		    ***if (unrm_table[i].din.di_mtime >
		    ***      unrm_table[key].din.di_mtime) 
		    *****************************************************/
		    if (newer(i, key, *unrm_use_next) == i)
		    {
			tag[key] = 0;
			key = i;
		    }
		    else
			tag[i] = 0;
		}
	}
    
    if (unrm_debug) 
    {
	fprintf(stderr, "matches after culling:\n");
	for (h=0; h<*unrm_table_size; h++) 
	    if (tag[h]) 
	    {
		char ls_line[200];
		lsinfo(&unrm_table[h].din, ls_line);
		fprintf(stderr, "%s  %s\n", ls_line, unrm_table[h].fname);
	    }
    }
    
    /* now, for each remaining tagged entry, read the data blocks for that */
    /* file off the disk, and save them in the big 2-D linked list. */
    /* artifact of programming: the order of the files is reversed. */
    top = NULL;
    for (h=0; h<*unrm_table_size; h++) 
	if (tag[h]) 
	{
	    bp = make_block_list(h);
	    if (top == NULL)
		top = bp;
	    else 
	    {
		bp->next_file = top;
		top = bp;
	    }
	}

    
    if (unrm_debug)  
    {
	if (top != NULL)
	    fprintf(stderr, "files to unrm:\n");
	bp = top;
	while (bp != NULL) 
	{
	    fprintf(stderr, "\tname = %s\n", unrm_table[bp->tag].fname);
	    bp = bp->next_file;
	}
    }
    
    /* now re-read the unrm_table to be sure we have the latest data, */
    /* and go through and check all the blocks to see if they have been */
    /* used again, or used again and freed, etc. */
    read_unrm_data();
    
    /* now go through the saved file list, and start restoring them to disk */
    bp = top;
    while (bp != NULL) 
    {
	char *fname = unrm_table[bp->tag].fname;
	char *answer;
	int mode;
	
	if (check_integrity(bp)) 
	{
	    bp = bp->next_file;
	    continue;
	}
	
	mode = unrm_table[bp->tag].din.di_mode;
	
	if (!force) 
	{
	    fprintf(stderr, "%s? ", fname);
	    answer = ask("(y or n): ", "n");
	    if (*answer == 'n') 
	    {
		bp = bp->next_file;
		continue;
	    }
	}
	
	while ( (ofd = open(fname, O_RDWR | O_CREAT | O_EXCL, mode)) < 0)
	{
	    perror(fname);
	    fname = ask("Enter new path (return to skip): ", NULL);
	    if (*fname == '\0')
		break;
	}
	if (*fname == '\0') 
	{
	    bp = bp->next_file;
	    continue;
	}
	
	/* set modes on the file before putting any data into it */
	if (fchown(ofd, unrm_table[bp->tag].din.di_uid,
	       unrm_table[bp->tag].din.di_gid) < 0)
	    /* um, what do we do if the chown fails? can it? */
	    perror("chown");
	
	blockp = bp;
	
	while (blockp != NULL) 
	{
	    /* could check return value of write() here */
	    write(ofd, blockp->data, blockp->size);
	    blockp = blockp->next_block;
	}
	close(ofd);
	fprintf(stderr, "%s: restored.\n", fname);
	bp = bp->next_file;
    }
    exit(0);
}    



usage()
{
    fprintf(stderr, "usage: %s\t\t list your removed files\n", progname);
    fprintf(stderr, "usage: %s -l\t\t list all removed files\n", progname);
    fprintf(stderr, "usage: %s [-f] files... attempt to unrm files\n", progname);
    exit(1);
}


/*
 * just malloc a list_struct and check for errors 
 */
struct list_struct *lalloc()
{
    struct list_struct *l;
    
    l = (struct list_struct *) malloc(sizeof(struct list_struct));
    if (l == LNULL) 
    {
	fprintf(stderr, "malloc failed in lalloc.\n");
	yourexit(8);
    }
    memset(l, 0, sizeof(struct list_struct));
    return(l);
}


/* stolen from kernel, hope nobody minds */
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
		return (NULL);
	}
}


char *ask(question, dfault)
    char *question, *dfault;
{
    static char answer[100];
    
    while (1) 
    {
	printf("%s", question);
	fgets(answer, sizeof(answer), stdin);
	if (answer[strlen(answer)-1] == '\n')
	    answer[strlen(answer)-1] = '\0';
	
	if (answer[0] == '\0' || answer[0] == '\n') 
	    if (dfault && *dfault == '\0')
		printf("Please answer the question.\n");
	    else
		return(dfault);
	else
	    return(answer);
    }
}

/*
 * the reason for all this myexit/yourexit stuff has been lost
 * in the sands of time...
 */
void sigcatch(signo)
    int signo;
{
    yourexit(1);
}

myexit(n)
    int n;
{
    exit(n);
}

yourexit(n)
    int n;
{
    myexit(n);
}



/*
 * This routine actually does the reading of blocks off the disk
 */
struct list_struct *make_block_list(n) /* n is index into unrm_table */
    int n;
{
    struct dinode din;
    char fname[MAXNAMLEN+1];
    dev_t dev;
    int size, i, uncertain;
    struct list_struct *top, *newblock, *last;
    struct fs *fs;
    
    memcpy(&din, &unrm_table[n].din, sizeof(struct dinode));
    memcpy(fname, unrm_table[n].fname, MAXNAMLEN+1);
    dev = unrm_table[n].dir_dev;

    fs = (struct fs *)devtosuper(dev);
    size = din.di_size;
    top = NULL;

    for (i=0; i<NDADDR; i++) 
    {
	int blockno = din.di_db[i];
	
	newblock = lalloc();
	newblock->tag = n;	/* which file it came from */
	if (top == NULL) 
	{
	    top = newblock;
	    last = top;
	}
	else 
	{
	    last->next_block = newblock;
	    last = newblock;
	}
	newblock->blockno = blockno;

	/* the last parm to breadn tells it to read from the raw device */
	newblock->data = (char *)breadn(dev, blockno, min(size,fs->fs_bsize),1);
	newblock->next_block = NULL;

	if (size >= fs->fs_bsize)
	    newblock->size = fs->fs_bsize;
	else 
	{
	    newblock->size = size;
	}
	size -= fs->fs_bsize;

	if (size <= 0)
	    return(top);
    }
    return(top);
}


/*
 * This routine is used to figure out if the data we read from the disk
 * is valid. There are a few things that can screw it up: First, a block
 * could be re-used by another file, and in this case the block does not
 * contain good data, and we cannot give it to the user. Second, a block
 * could be re-used, but then freed. This is more difficult to detect
 * since we have to look at a lot of stuff, but we cannot use the data
 * from that block either. Third, and most annoyingly, indirect blocks
 * get completely zeroed when a large file is removed, and so there
 * could be blocks that look fine superficially, but have been held
 * by large files. So, if there have been any indirect blocks freed
 * since our file was removed, we have no choice but to disallow 
 * restoration of that file. To do otherwise would compromise security.
 *
 */
check_integrity(top)		/* pointer to linked list for file */
    struct list_struct *top;
{
    struct dinode din;
    char fname[MAXNAMLEN+1];
    dev_t dev;
    int size, i, uncertain;
    struct list_struct *newblock, *last;
    struct fs *fs;

    dev = old_table[top->tag].dir_dev;
    fs = (struct fs *)devtosuper(dev);
    memcpy(&din, &old_table[top->tag].din, sizeof(struct dinode));
    memcpy(fname, old_table[top->tag].fname, MAXNAMLEN+1);

    size = din.di_size;
    newblock = top;
    
    for (i=0; i<NDADDR; i++) 
    {
	int blockno = newblock->blockno;
	
	/* problem: block may be used, but frag may not be! */
	/* have to check for that case... */
	if (block_is_used(dev, blockno))
	{
	    char msg[200];

	    if ( (newblock->size >= fs->fs_bsize)
		|| frag_is_used(dev, blockno, newblock->size)) 
	    {
		sprintf(msg, "Warning: block %d of file %s is in use.\n",
			blockno, fname);
		fprintf(stderr, "%s", msg);
		memset(newblock->data, 0, min(size, fs->fs_bsize));
		strcpy(newblock->data, msg);
		newblock->size = strlen(msg);
	    }
	}

	/* check all newer files to see if block has been used again */
	/* the 'uncertain' parameter tells us if indirect blocks have */
	/* been freed. */
	if (block_was_reused(blockno, top->tag, &uncertain))
	{
	    char msg[200];
	    sprintf(msg, "Warning: block %d of file %s was re-used.\n",
		    blockno, fname);
	    fprintf(stderr, "%s", msg);
	    memset(newblock->data, 0, min(size, fs->fs_bsize));
	    strcpy(newblock->data, msg);
	    newblock->size = strlen(msg);
	}

	/* allow root to restore files in this case */
	if (uncertain) 
	{
	    fprintf(stderr, "Indirect blocks have been freed since '%s' was unlinked\n", fname);
	    if (geteuid() != 0) 
	    {
		fprintf(stderr, "For security, only root can restore this file.\n");
		return(1);
	    }
	}
	
	size -= fs->fs_bsize;
	if (size <= 0)
	    return(0);
        newblock = newblock->next_block;
    }
    return(0);
}


/*
 * Read the cool stuff from the kernel. There are three important pieces
 * of information to read: the size of the table, the table itself, and
 * the 'use next' pointer, which tells which entry in the table will be
 * used next.
 */
read_unrm_data()
{
    int ret;
    struct nlist unrm_syms[5];
    
#ifdef hp9000s300
    unrm_syms[0].n_name = "_unrm_table_size";
    unrm_syms[1].n_name = "_unrm_table";
    unrm_syms[2].n_name = "_unrm_use_next";
    unrm_syms[3].n_name = 0;
#endif    
#ifdef hp9000s800
    unrm_syms[0].n_name = "unrm_table_size";
    unrm_syms[1].n_name = "unrm_table";
    unrm_syms[2].n_name = "unrm_use_next";
    unrm_syms[3].n_name = 0;
#endif    
    if (nlist("/hp-ux", unrm_syms) != 0)
	fprintf(stderr, "nlist failed! ha ha ha!\n");

    if (unrm_syms[0].n_value != 0)
    {
	unrm_table_size = (int *) kread(unrm_syms[0].n_value, sizeof(int));
	unrm_table_addr = (int *) kread(unrm_syms[1].n_value, sizeof(int));
	
	if (*unrm_table_addr == NULL) 
	    return(2);

	/* see ufs/ufs_dir.c for an explaination of this junk */
	if (mkfifo("/tmp/foo17.1", 0666) != 0)
	    perror("uh oh couldn't make a fifo");
	unlink("/tmp/foo17.1");
	
	unrm_table = (struct unrm_table_t *) kread(*unrm_table_addr,
			*unrm_table_size * sizeof(struct unrm_table_t));
	unrm_use_next = (int *) kread(unrm_syms[2].n_value, sizeof(int));
	
	/* hack for kernel bug: it increments unrm_use_next, but doesn't */
	/* do the modulo table size arithmetic until the next unlink, */
	/* so there's a chance that unrm_use_next will be equal to the */
	/* table size, and this will cause some calculations to fail */
	if (*unrm_use_next >= *unrm_table_size)
	    *unrm_use_next = 0;
	

	return(0);
    }
    else 
	return(1);
}

/*
 * Show 'ls -l' style list of files from the table
 */
listrm(uid)
    int uid;
{
    int i;
    
    for (i=0; i<*unrm_table_size; i++) 
    {
	char ls_line[200];
	char *f = unrm_table[i].fname;

	if ( *f == '\0' || unrm_table[i].din.di_size == 0 )
	    continue;
	if ( (uid == LIST_ALL) || (uid == unrm_table[i].din.di_uid) ) 
	{
	    lsinfo(&unrm_table[i].din, ls_line);
	    printf("%s  %s\n", ls_line, f);
	}
    }
}

/*
 * Simply check the freemap to see if a block is used
 */
int block_is_used(dev, blockno)
    dev_t dev;
    int blockno;
{
    struct cg *cgp;
    struct fs *fs;
    int frag, cgnum, fragoffset, blkoffset;
    
    fs = (struct fs *)devtosuper(dev);
    cgnum = dtog(fs, blockno);
    cgp = read_cg_block(dev, cgnum);

    fragoffset = dtogd(fs, blockno);
    blkoffset = fragstoblks(fs, fragoffset);

    if (isblock(fs, cgp->cg_free, blkoffset)) /* if block is free */
	return(0);
    else
	return(1);

}

/*
 * Check the freemap to see if frags are used
 */
frag_is_used(dev, blockno, size)
    dev_t dev;
    int blockno;
    int size;
{
    int i, bno;
    struct cg *cgp;
    struct fs *fs;
    
    fs = (struct fs *)devtosuper(dev);
    bno = dtogd(fs, blockno);
    cgp = read_cg_block(dev, dtog(fs, blockno));
    
    for (i = 0; size > 0; i++, size -= fs->fs_fsize)
	if (isclr(cgp->cg_free, bno + i)) {
	    return(1);
	}
    return(0);
}


/*
 * Check all newer (ie, more recently removed) files in unrm_table, and
 * look for blocks that were re-used 
 */
block_was_reused(blockno, tptr, uncertain)
    int blockno, tptr, *uncertain;
{
    int i, j, uncertainties = 0, all_newer = 0;
    struct fs *fs = (struct fs *)devtosuper(old_table[tptr].dir_dev);
    
    /* if our removed file is no longer present in unrm_table, that means */
    /* that every entry in the table is newer than it, and so it's possible */
    /* that the entire table was filled up again, and there were files */
    /* deleted that we have no way of knowing about. if this happens, then */
    /* we also have no way of knowing if those unknown files held indirect */
    /* blocks, so we have to mark this as an uncertainty */
    if ( memcmp(&unrm_table[tptr], &old_table[tptr],
		sizeof(struct unrm_table_t)) != 0) 
	all_newer = 1;
    

    for (i = 0; i<*unrm_table_size; i++)
    {
	/* if devices are different, then it doesn't matter */
	if (unrm_table[i].dir_dev != old_table[tptr].dir_dev)
	    continue;

	/* only interested in newer files */
	/* uh-oh, the inode timestamp does not indicate when */
	/* the file was deleted, but when it was modified! */
#ifdef ohshit
	if (unrm_table[i].din.di_mtime < old_table[tptr].din.di_mtime)
	    continue;
#else
	if (!all_newer)
	    if ( (newer(i, tptr, old_use_next) == tptr) &&
		(newer(i, tptr, *unrm_use_next) == tptr))
		continue;
#endif
	
	/* check to see if we're comparing file with itself. */
	/* can we be absolutely certain of this comparison? */
	/* i think so: the entries must be on the same device, */
	/* must be contained in the same directory, have the same */
	/* name, and the same creation, modification, and access */
	/* times. even if you can create two files in the same directory */
	/* at the same time, when you rename one later, its creation */
	/* time will be updated. */
	if ( (i == tptr) &&  (memcmp(&unrm_table[i],
		&old_table[tptr],  sizeof(struct unrm_table_t)) == 0) )
	    continue;
	

	/* ok, now check the direct blocks in this table entry to see */
	/* if blockno appears anywhere. */
	/* woops #1: should we check di_size to make sure we're doing */
	/* valid comparisons? ie, some direct block pointers could be */
	/* zero or bogus? */
	/* woops #2: a big one- since any indirect blocks associated */
	/* with this file have been zeroed, there is no definite way */
	/* to tell if blockno has been re-used! at least if the size */
	/* of the file indicates that it had no indirect blocks, we can */
	/* be certain that all is ok. */
	for (j=0; j<NDADDR; j++)
	    if (blockno == unrm_table[i].din.di_db[j])
		return(1);
	if (unrm_table[i].din.di_size > NDADDR*fs->fs_bsize) 
	    uncertainties++;
    }
    *uncertain = uncertainties + all_newer;
    return(0);
}


/*
 * This routine is used to convert ed/sed style regular expressions 
 * to shell style regular expressions. I have no idea why I have to
 * do this, but it seems I must in order to allow users to specify
 * 'normal' patterns, ie, *.c, etc.
 *
 * Basically, I put '^' at the beginning of each pattern, change each
 * '.' (dot) to "\.", change each '*' to ".*", change each '?' to
 * '.' (dot), and finally, add '$' to the end!!
 */
preprocess(s)
    char *s;
{
    char copy[200];
    int i = 0;
    
    strcpy(copy, s);
    *s++ = '^';
    
    while (copy[i]) 
    {
	switch (copy[i]) 
	{
	  case '*':
	    *s++ = '.';
	    *s++ = '*';
	    break;

	  case '.':
	    *s++ = '\\';
	    *s++ = '.';
	    break;

	  case '?':
	    *s++ = '.';
	    break;
	  default:
	    *s++ = copy[i];
	}
	i++;
    }
    *s++ = '$';
    *s = '\0';
}

/*
 * This routine tells which unrm_table entry is newer. I had thought this
 * was easy for a long time, since it looked to me like the inode was
 * updated with a new timestamp when the file was deleted. This actually
 * does happen, however, it doesn't happen until after the inode is saved
 * in unrm_table in the kernel. Wooops! So, the only way to tell which
 * entry is newer is to see which was written to unrm_table later.
 * The table is written sequentially, with 'next' pointing to the next
 * entry to use, and wrapping around to zero when it hits unrm_table_size
 */
newer(a, b, next)
    int a, b;
{
    int t;
    
    /* swap them to make sure a < b */
    if (a > b) 
    {
	t = a;
	a = b;
	b = t;
    }

    if (next <= a || next > b)
	return(b);
    else
	return(a);
}
