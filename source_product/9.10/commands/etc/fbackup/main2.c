/* @(#) $Revision: 70.2 $ */

/***************************************************************************
****************************************************************************

	main2.c

    This file contains the functions which copy the contents of files and
    directories to shared memory, when there is data to be copied (which
    is the case for non-empty regular files and directories).  In addition,
    these functions invoke other functions which build the header and trialer
    blocks.  The functions which handle hard links to non-directories are
    also present.
	A note on the pending file table:
    The pending file table is a data structure which holds all the relevant
    info on files until they are finished being processed.  Because of the
    nature of fbackup (space in shared memeory is allocated for a file's
    data, and then it is asynchronously read in), such a structure is
    required.  This means that for a particular file, say file N, the space
    may be allocated, and then the space for several files after N may
    also be allocated before all the data for file N is read in.  In fact,
    all the data for zero or more files following N may be completely read
    in BEFORE any of the data for file N is read in.  Since this is a non-
    determanistic process, information must be maintained on files which are
    "pending".  For a given number, R, of readers, there can be no more than
    R files pending at any given time.  For this reason, there are R+1
    entries in the pending file table (so the main process never has to wait
    for a pending file table entry).

****************************************************************************
***************************************************************************/

#include "head.h"

#ifdef NLS
#define NL_SETN 1	/* message set number */
extern nl_catd nlmsg_fd;
#endif NLS

void free(), cantbackup(), buildhdr(), buildtrl(), markempty(), markfull();
DIR *opendir();
struct dirent *readdir();

extern int segsize,
	   segindex,
	   nrdrs,
	   rdrpid[];

extern char	*segment;
extern PADTYPE	*pad;
extern RDRTYPE	*rdr;
extern RECTYPE	*rec;
extern PENDFTYPE pend_file[],
		 *pend_ptr;







/***************************************************************************
    This function is called to copy all the data necessary to recover a file
    into shared memory.  First, the file name (and what it is hard linked to,
    if applicable) is obtained.  Then the file is stat-ed.  If either of
    these operations fails, dofile exits prematurely.  Otherwise, the pending
    file table is searched to find a FREE entry, and it is initialized.
    Depending on which kind of file we are dealing with, the appropriate
    action is taken.  After processing for the file is finished, the pending
    file status entry is set back to FREE.
***************************************************************************/
dofile(filenum)
int filenum;
{
    struct stat statbuf;
    int reader;
    int filesize;
    int nbytes;
    int avail_tmp;
    int pend_fidx;
    int blkrndup;
    char *file, *linkto, *dotdot;
    int fwdlinkflag;
    int fd;
    long savemask;
    int bytesread = 0;
    struct flock flockbuf;
    int lockretries = 0;   /* add for # of retries for file locked condition */

#ifdef DEBUG
fprintf(stderr, "entered dofile with filenum: %d\n", filenum);
#endif    

    if (!getfile(filenum, &fwdlinkflag, &file, &linkto, &dotdot)) {
	msg((catgets(nlmsg_fd,NL_SETN,101, "fbackup(1101): unexpectedly ran out of files to backup\n")));
	cantbackup();
	return;
    }

    if (statcall(file, &statbuf) == -1) {
	msg((catgets(nlmsg_fd,NL_SETN,102, "fbackup(1102): unable to stat file %s\n")), file);
	cantbackup();
	return;
    }

    for (pend_fidx=0; pend_fidx<=nrdrs; pend_fidx++) {	/* get a free entry */
	pend_ptr = &pend_file[pend_fidx];
	if (pend_ptr->status == FREE) {		/* status is one of: FREE, */
	    pend_ptr->status = TRLNOTINMEM;	/* TRLNOTINMEM, or TRLINMEM */
/*
 * We need to save the filename in our structure because it is returned
 * in a static buffer.
 */
	    file = strcpy(pend_ptr->file, file);
	    pend_ptr->chunks = 0;
	    pend_ptr->chunksfull = 0;
	    pend_ptr->modflag = FALSE;
	    pend_ptr->atime = statbuf.st_atime;	/* save the mod times */
	    pend_ptr->mtime = statbuf.st_mtime;
	    pend_ptr->ctime = statbuf.st_ctime;
	    break;
	}
    }

    if (linkto != (char*)NULL) {	    /* it's a link to some other file */
	statbuf.st_size = 0;		    /* back it up as such (no data) */
	buildhdr(file, linkto, dotdot, filenum, fwdlinkflag, &statbuf);
	buildtrl(filenum, &statbuf);
	pend_ptr->status = FREE;
	return;
    }

    switch (statbuf.st_mode&S_IFMT) {

    case S_IFREG:					/* regular file */
    /* case S_IFNWK:			       obsolete network special */

	if ((fd = open(file, 0)) < 0) {
	    msg((catgets(nlmsg_fd,NL_SETN,103, "fbackup(1103): unable to open file %s\n")), file);
	    pend_ptr->status = FREE;	/* return if we can't open it */
	    cantbackup();
	    return;
	}
        
        if ((statbuf.st_mode & S_ENFMT) &&
	    !(statbuf.st_mode & S_IXGRP)) {  /*check for enforcement lock */
	   lockretries = 0;
           while (lockretries < pad->maxretries)  {
#ifdef DEBUG
fprintf(stderr, "lockretries : %d\n", lockretries);
#endif    
             flockbuf.l_type = F_RDLCK;
             flockbuf.l_whence = SEEK_SET;
             flockbuf.l_start = 0;
             flockbuf.l_len = 0;
	     if (fcntl(fd, F_GETLK, &flockbuf) == -1) {
#ifdef DEBUG
fprintf(stderr, "fcntl retuan -1, errno = %d\n", errno);
#endif    
               msg((catgets(nlmsg_fd,NL_SETN,107, "fbackup(1107): unable to get file control information for file %s\n")), file);
               pend_ptr->status = FREE;    /* return if we can't fcntl it */
               cantbackup();
               return;
             }
             if (flockbuf.l_type == F_WRLCK) {
	       lockretries++;
             }
	     else  { 
	       break;
             }
           }
	   if (lockretries == pad->maxretries) {
              msg((catgets(nlmsg_fd,NL_SETN,108, "fbackup(1108): file %s has been locked,\n and will not be backed up\n")), file); 
              pend_ptr->status = FREE;    /* return if file is locked */
              cantbackup();
              return;
           }
        }
	buildhdr(file, (char*)NULL, (char*)NULL, filenum, FALSE, &statbuf);

		    /* While there is still data left to copy and the
		       writer is still ready, bite off chunks of this file
		       as large as possible, and assign reader(s) to copy
		       them into shared memory.
		    */
	filesize = statbuf.st_size;
	while ((bytesread < filesize) && (pad->wrtrstatus == READY)) {
	    savemask = sigblock(WRTRSIG);

		    /* while there's no shared memory avaliable, wait
		    */
	    while ((avail_tmp=pad->avail) < BLOCKSIZE) {
		if (pad->wrtrstatus != READY) {
		    (void) sigsetmask(savemask);
		    return;
		}
		(void) sigpause(savemask);
	    }
	    (void) sigsetmask(savemask);

	    reader = getrdr();			/* get a free reader */
	    (void) strcpy(rdr[reader].file, file);	/* set the reader up */
	    rdr[reader].offset = bytesread;
	    rdr[reader].index = segindex;
	    rdr[reader].pend_fidx = pend_fidx;
	    pend_ptr->chunks++;		/* bump the chunks counter */

		    /* If there is more data left to read in than available
		       shared memory, read the available amount, otherwise,
		       read the rest of the file, and round this amount up
		       so that we always read multiples of BLOCKSIZE bytes.
		    */
	    if (filesize-bytesread > avail_tmp) {
		nbytes = blkrndup = avail_tmp;
	    } else {
		nbytes = filesize-bytesread;
		blkrndup = rndup(nbytes);
	    }

		    /* finish initializing the reader, mark this section
		       of "ring" shared memory as empty, decrement the
		       amount of available shared memory, adjust the
		       shared memory index, and start the reader going.
		    */
	    rdr[reader].nbytes = nbytes;
	    markempty(segindex, blkrndup, DATA, 0); 
	    atomicadd(-blkrndup);
	    bytesread += nbytes;
	    segindex = (segindex+blkrndup) % segsize;
	    rdr[reader].status = START;
	    (void) kill(rdrpid[reader], SIGUSR2);
	}
	(void) close(fd);

		    /* At this point, the entire file has been divided
		       into chunks and assigned readers to bring it into
		       shared memory.  So the file has been closed, and
		       the trailer block must be built.
		       Note: these functions (dofile and buildtrl) MAY BE
		       INTERRUPTED.  It is very important to know what can
		       happen in the function rdrsignal, which CANNOT be
		       interrupted, as it is an interrupt handler.

		       Buildtrl builds the trailer block and changes the
		       pendfile status for this file from TRLNOTINMEM to
		       TRLINMEM.
		    */
	buildtrl(filenum, &statbuf);

		    /*
		       Whenever the reader(s) have filled all the chunks
		       before the call to buildtrl has marked that the
		       trailer for this file is in memory (which happens,
		       although infrequently), the status of this file (in
		       the pending file table) will have been marked as
		       FREE by rdrsignal.  In this case, we need not reset
		       the times for this file or mark its pendfile status
		       as FREE, as both of these things would have already
		       been done by rdrsignal.
		    */
	savemask = sigblock(RDRSIG);    /* reader(s) cannot interrupt now */

	if ((pend_ptr->status != FREE) &&
			    (pend_ptr->chunks == pend_ptr->chunksfull)) {
	    if (!pend_ptr->modflag)
		resettimes(pend_ptr);
	    pend_ptr->status = FREE;
	}
	(void) sigsetmask(savemask);	/* enable reader interrupts */
	break;

    case S_IFDIR:					/* directory */
	dodir(file, dotdot, filenum, &statbuf);
	break;

			/* for block, character, and network special files,
			   fifos, and sockets, there will be no data blocks
			   needed; all the information necessary to recover
			   them is contained in the header and trailer blocks.
			   Note: there is no need to check for files of
			   these type being active.
			*/
    case S_IFCHR: case S_IFBLK: case S_IFIFO: case S_IFSOCK:
	buildhdr(file, (char*)NULL, (char*)NULL, filenum, FALSE, &statbuf);
	buildtrl(filenum, &statbuf);
	pend_ptr->status = FREE;
	break;

#ifdef SYMLINKS
			/* 
			   Symbolic link:
			   No data blocks are required to recover symbolic
			   links.  The link-to name is put in the header.
			*/
    case S_IFLNK:
	linkto = (char*)mymalloc((unsigned)statbuf.st_size+1);	/* get space */
        if (linkto == (char*)NULL)
    	    errorcleanx(catgets(nlmsg_fd,NL_SETN,104, "fbackup(1104): out of virtual memory\n"));
	readlink(file, linkto, statbuf.st_size);	/* get linkto name */
	*(linkto+statbuf.st_size) = '\0';		/* terminate it */
	statbuf.st_size = 0;		    /* tell frecover there's no data */
	buildhdr(file, linkto, (char*)NULL, filenum, FALSE, &statbuf);
	free(linkto);					/* free the space */
	buildtrl(filenum, &statbuf);
	pend_ptr->status = FREE;
	break;
#endif /* SYMLINKS */
    }
}






/***************************************************************************
    This function copies the information necessary to recover a directory
    to shared memory.  For frecover, this information consists of the names
    of all the entries in this directory, except the two which are always
    present, ".", and "..".  Directories exhibit the unfortunate property
    that the "size" (st_size) does not necessarily have much to do with the
    number of bytes which will have to be copied.  The only relationship
    between the two is that st_size will always be larger, unless the
    directory is active, and new entries are added to it during the time we
    are trying to read it.  If this happens, dodir marks this directory
    as being active, and exits prematurely.  If the directory was active,
    but it's new size did not exceed the original st_size, there is no
    premature exit, it is simply marked active after processing the entire
    directory.  Normally, of course, neither of these cases occurs.
    Because of these unfortunate properties, it is impossible to allocate
    the correct amount of shared memory space, and assign reader processes
    to fill it.  Hence, the main process must process all directory enties.
***************************************************************************/
dodir(file, dotdot, filenum, statbuf)
char *file, *dotdot;
int filenum;
struct stat *statbuf;
{
    char *cp;
    struct dirent *dp;
    DIR	*dirp;
    time_t oldmtime, oldctime;
    int lim, len, minn, n, oldsegidx;
    int blkidx = BLOCKSIZE, n_total = 0;

    if ((dirp = opendir(file)) == NULL) {	/* open the directory */
      perror("fbackup(9999)");
      msg((catgets(nlmsg_fd,NL_SETN,105, "fbackup(1105): could not open directory %s\n")), file);
      pend_ptr->status = FREE;
      return; 
    }

    oldmtime = statbuf->st_mtime;	/* save the original m&c times */
    oldctime = statbuf->st_ctime;
    buildhdr(file, (char*)NULL, dotdot, filenum, FALSE, statbuf);

    while ((dp = readdir(dirp)) != NULL) {	/* get a directory entry */
	cp = dp->d_name;

				    /* if it's not "." or "..", process it */
	if ((*cp != '.') || ((*(cp+1) != '\0') &&
				    ((*(cp+1) != '.') || (*(cp+2) != '\0')))) {
	    n = 0;
	    len = strlen(cp)+1;

		    /* If enough new entries are added so that the total size
		       of their (null terminated) names exceeds the st_size
		       for this directory, then mark this directory as being
		       active.  This will cause the writer to catch it and
		       tell the main process to reset and then resume.
		       */
	    if (n_total + len > statbuf->st_size) {
		oldmtime = MODIFIED;
		break;
	    }
			/* Otherwise, process each entry normally.  */
	    while (n < len) {
		if (blkidx >= BLOCKSIZE) {
		    oldsegidx = segindex;
		    if (getshmemblk()) {
			closedir(dirp);
			pend_ptr->status = FREE;
			return;
		    } else {
			blkidx = 0;
		    }
		}
		minn = min(len-n, BLOCKSIZE-blkidx);
		bcopy(&segment[oldsegidx+blkidx], cp+n, minn);
		n += minn;
		n_total += minn;
		blkidx += minn;
		if (blkidx >= BLOCKSIZE)
		    markfull(oldsegidx, BLOCKSIZE);
	    }
	}
    }

		    /* For any remaining (empty) part of the last block, fill
		       it with null characters.
		    */
    lim = BLOCKSIZE-blkidx;
    if (lim) {
	cp = &segment[oldsegidx+blkidx];
	n_total += lim;
	while (lim--)
	    *cp++ = '\0';
	markfull(oldsegidx, BLOCKSIZE);
    }
    closedir(dirp);				/* close the directory */

		    /* If st_size is so large that there are entire block(s)
		       which were not used, fill them with null characters.
		    */
    while (n_total < statbuf->st_size) {
	oldsegidx = segindex;
	(void) getshmemblk();
	cp = &segment[oldsegidx];
	lim = BLOCKSIZE;
	while (lim--)
	    *cp++ = '\0';
	markfull(oldsegidx, BLOCKSIZE);
	n_total += BLOCKSIZE;
    }

		    /* If this stat call fails or if this directory has been
		       modified since when we started reading it, mark it
		       as being active.  Otherwise, reset its times.
		    */
    if ((statcall(file, statbuf) == -1) || (statbuf->st_mtime != oldmtime) ||
					(statbuf->st_ctime != oldctime))
	pend_ptr->modflag = TRUE;
    else 
	resettimes(pend_ptr);

		    /* Build its trailer and free the pendfile entry */
    buildtrl(filenum, statbuf);
    pend_ptr->status = FREE;
}






/***************************************************************************
    This function waits for at least BLOCKSIZE bytes of "ring" shared memory
    to become available, and then marks this block empty, updates the
    amount of available shared memory and the shared memory segment index,
    and then returns.  It returns a value of true if the writer process
    is not READY.  This is necessary to avoid deadlock in the event of the
    writer discovering an active file or a tape write error.
***************************************************************************/
int
getshmemblk()
{
    long savemask;

    savemask = sigblock(WRTRSIG);
    while (pad->avail < BLOCKSIZE) {
	if (pad->wrtrstatus != READY) {
	    (void) sigsetmask(savemask);
	    return(TRUE);
	}
	(void) sigpause(savemask);
    }
    (void) sigsetmask(savemask);

    markempty(segindex, BLOCKSIZE, DATA, 0);
    atomicadd(-BLOCKSIZE);
    segindex = (segindex+BLOCKSIZE) % segsize;
    return(FALSE);
}






typedef struct flinknode {
    ino_t inode;
    dev_t device;
    FLISTNODE *linkdata;
    struct flinknode *next;
} FLINKNODE;

static FLINKNODE *head = (FLINKNODE *) NULL;

/***************************************************************************
    This function is called whenever a hard link to a non-directory is found.
    This is indicated by its link count being > 1.  The list of such files
    is searched to see if this inode, device number pair has been encountered
    before.  If this is the first instance of a link to this inode, it is
    added to the list, otherwise, the matching list entry (another link to
    the same inode) is found.  In either case, a pointer to the appropriate 
    element in this list is returned.  Note that this is a separate list of
    only hard links to non-directories, not the flist.
***************************************************************************/
FLISTNODE *
hardflink(flistptr, statbuf)
FLISTNODE *flistptr;
struct stat *statbuf;
{
    FLINKNODE *node = head, *prev = (FLINKNODE *) NULL;
    ino_t inode = statbuf->st_ino;
    dev_t device = statbuf->st_dev;

    while (node != (FLINKNODE *) NULL) {
	if ((node->inode == inode) && (node->device == device))
	{
	    return(node->linkdata);
	}
	if (node->inode > inode)
	    break;
	prev = node;
	node = node->next;
    }
    addflink(inode, device, flistptr, prev, node);
    return((FLISTNODE *) NULL);
}






/***************************************************************************
    This function is called by hardflink to actually add elements to the
    list of hard links to non-directories.
***************************************************************************/
addflink(inode, device, flistptr, prev, node)
ino_t inode;
dev_t device;
FLISTNODE *flistptr;
FLINKNODE *prev, *node;
{
    FLINKNODE *new;

    new = (FLINKNODE*)new_malloc(sizeof(FLINKNODE));
    if (new == (FLINKNODE*)NULL)
	errorcleanx(catgets(nlmsg_fd,NL_SETN,106, "fbackup(1106): out of virtual memory\n"));
    new->inode = inode;
    new->device = device;
    new->linkdata = flistptr;

    if (head == (FLINKNODE *) NULL) {		/* first on list */
	head = new;
	new->next = (FLINKNODE *) NULL;
    } else if (prev == (FLINKNODE *) NULL) {	/* add to the beginning */
	head = new;
	new->next = node;
    } else if (node != (FLINKNODE *) NULL) {	/* add in the middle */
	prev->next = new;
	new->next = node;
    } else {					/* add to the end */
	prev->next = new;
	new->next = (FLINKNODE *) NULL;
    }
}
