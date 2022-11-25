/* @(#) $Revision: 72.1 $ */

/***************************************************************************
****************************************************************************

	reset.c

    Occasionally, due to an active file or a write error to a tape drive,
    the writer process informs the main process that it needs to halt,
    resynchronize, and then resume at an agreed-upon location.  For the
    main process, the first thing to be done is to stop assigning any new
    tasks (eg, stop assigning chunks of files to be read by reader processes).
    This happens even if the main process is in the middle of copying a
    very large file.  In the normal course of backing up files which are on
    the flist, fbackup checks to see that everything is right with the
    writer process.
	After discovering that it should not initiate any new tasks, the
    main process calls the functions in this file which wait for all
    tasks which were assigned before to complete (and no new tasks are
    initiated).  When the main process has reached this quiescent state, it
    informs the writer process, and the two of them get back into sync.

****************************************************************************
***************************************************************************/

#include "head.h"
#include <sys/mtio.h>
#include <sys/ioctl.h>

#ifdef NLS
#define NL_SETN 1	/* message set number */
extern nl_catd nlmsg_fd;
#endif NLS

void makeresetblk(), beforefork(), blk_cksum(), markempty(), markfull();

extern PADTYPE	*pad;
extern RDRTYPE	*rdr;
extern int	segsize,
		segindex,
		wrtrpid,
		rdrpid[],
		recsize,
		idxfd,
		filenum;
		blksperrec;
extern char	idxrec[],
		*segment;


/***************************************************************************
    This function is called after it has been discovered that no new tasks
    should be initiated, but before all the tasks previously started have
    (necessarily) finished.  Reset first waits for all readers to become
    quiescent.  It then waits for the writer process to become READY, and
    does a bit if re-initialization.  At this point one of two cases may
    occur: (1) the writer wants to resume at the beginning of a file, as is
    the case when an active file is detected.  In this case, reset simply
    returns the file number supplied by the writer.  Case (2) occurs when
    there is a write error to a tape drive.  The odds are that the writer
    will require that the main process finish writing the rest of the file
    the writer was writing when the error occurred (otherwise, it is handled
    as a case 1).  In this second case, the main process fills the requisite
    amount of shared memory with zeros, and then returns this same file
    number so that a "resume" will occur at this file.

***************************************************************************/
int
reset()
{
    long savemask;
    int lim;

    waitforrdrs();
    pad->wrtrstatus = RESUME;
    (void) kill(wrtrpid, SIGUSR1);

    savemask = sigblock(WRTRSIG);
    while (pad->wrtrstatus != READY) {
	(void) sigpause(savemask);
    }
    (void) sigsetmask(savemask);

#ifdef DEBUG
    msg("\nRESET: filenum=%d, blktype=%c, blknum=%d, datasize=%d\n",
			pad->reset.filenum, pad->reset.blktype,
			pad->reset.blknum, pad->reset.datasize);
#endif DEBUG

    beforefork();

    if (pad->reset.blktype != HDRFIRST) {

		    /* We zero out all of the "ring" (of shared mem).  This
		       isn't necessary, but it doesn't hurt anything; it's
		       more difficult to decide exactly what must be zeroed.
		    */
	lim = 0;
	while (lim < segsize)
	    segment[lim++] = '\0';

	if (pad->reset.blktype == HDRCONT) {
	    makeresetblk(HDRCONT);
	    skipover(0);
	} else if (pad->reset.blktype == DATA) {
	    skipover((pad->reset.blknum - blksperrec)*BLOCKSIZE);
	}
	makeresetblk(TRAILER);
    }
    msg((catgets(nlmsg_fd,NL_SETN,301, "fbackup(1301): resuming at file %d\n")), pad->reset.filenum);
    return(pad->reset.filenum);
}






/***************************************************************************
    This function gets two data from the writer process: how big the file
    of interest was (at least at the time it was stated by the main process
    just before it was copied to shared memory), and how much of the file
    got put on the tape before the write error occurred (before the
    checkpoint).  The remainder of the file is skipped over (filled with
    zeros).
***************************************************************************/
skipover(bytesread)
int bytesread;
{
    int filesize;
    int nbytes;
    int avail_tmp;
    int blkrndup;
    long savemask;

    filesize = pad->reset.datasize;
    while ((bytesread < filesize) && (pad->wrtrstatus == READY)) {
	savemask = sigblock(WRTRSIG);
	while (((avail_tmp=pad->avail)<BLOCKSIZE) && (pad->wrtrstatus==READY)) {
	    (void) sigpause(savemask);
	}
	(void) sigsetmask(savemask);

	if (filesize-bytesread > avail_tmp) {
	    nbytes = (avail_tmp/BLOCKSIZE)*BLOCKSIZE;
	    blkrndup = nbytes;
	} else {
	    nbytes = filesize-bytesread;
	    blkrndup = rndup(nbytes);
	}
	
	markempty(segindex, blkrndup, DATA, 0);
	atomicadd(-blkrndup);
	bytesread += nbytes;

	markfull(segindex, blkrndup);
	segindex = (segindex+blkrndup) % segsize;
    }
}






/***************************************************************************
    This function is called to make a dummy header or trailer block.  In
    either case, (a header continuation block or a trailer block), the block
    doen't contain real data.  It only fills the required space, and, in the
    case of a trailer block, is marked active (so the writer will cause the
    main process to resume at (the beginning of) this file once again.
    Note that makeresetblk is NEVER called to make a first header block).
***************************************************************************/
void
makeresetblk(type)
char type;
{
    long savemask;
    int lim;
    PBLOCKTYPE ptr;
    char *chptr;
    char fnumstr[INTSTRSIZE+1];

    savemask = sigblock(WRTRSIG);
    while ((pad->avail < BLOCKSIZE) && (pad->wrtrstatus == READY)) {
	(void) sigpause(savemask);
    }
    (void) sigsetmask(savemask);

    atomicadd(-BLOCKSIZE);
    markempty(segindex, BLOCKSIZE, type, 0);

    lim = BLOCKSIZE;
    chptr = ptr.ch = &segment[segindex];
    while (lim--)			/* fill with null chars */
	*chptr++ = FILLCHAR;

    if (type == HDRCONT) {
	(void) strcpy(ptr.id->type, COH);
	(void) strcpy(ptr.id->last, EOH);
    } else {	/* TRAILER */
	(void) strcpy(ptr.id->type, BOT);
	(void) strcpy(ptr.id->last, EOT);
	(void) sprintf(fnumstr, INTSTRFMT, pad->reset.filenum);
	(void) strcpy(ptr.trl->filenum, fnumstr);
	(void) strcpy(ptr.trl->status, ASCIIBAD);
    }
    (void) strcpy(ptr.id->magic, MAGICSTR);
    blk_cksum(ptr);

    markfull(segindex, BLOCKSIZE);
    segindex = (segindex+BLOCKSIZE) % segsize;
}
