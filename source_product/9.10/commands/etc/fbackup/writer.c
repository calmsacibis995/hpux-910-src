/* @(#) $Revision: 72.2 $ */

/***************************************************************************
****************************************************************************

	writer.c

    This file contains routines which set up the writer process, and then
    handles interrupts which indicate that there is something for the writer
    to do.  Like the reader(s), the writer process sleeps whenever it has
    nothing to do.

****************************************************************************
***************************************************************************/

#include "head.h"
#include <fbackup.h>
#include <fcntl.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

void newvol(), callupdateidx();
extern int errno;
extern int qicflag;     /* For QIC support */
extern int is3480;      /* For 3480 support */
extern int is8mm;       /* For 8MM support */

#ifdef NLS
#include <locale.h>
#define NL_SETN 3	/* message set number */
nl_catd nlmsg_fd;
char *nl_langinfo();    /* add for msg 3310 */
#include <langinfo.h>   /* add for msg 3310 */
#endif NLS

void do_mrec(), writerec();
char *shmat();

void wrtrhandler(),
     endhandler();
struct sigvec wrtr_vec = {wrtrhandler, 0, 0},
	      end_vec  = {endhandler,  0, 0},
	      int_vec  = {SIG_IGN,     0, 0};
#ifdef PFA
    struct sigvec dflsig_vec={SIG_DFL, 0, 0};
#endif PFA

char	*segment,
	*recbuf;

struct fn_list *temp;
struct fn_list *outfile_head;

PADTYPE	*pad;
RECTYPE *rec;

int mttype;

char	*blkstatus,
	*lang,
        *outfptr;

int	segsize,
	recsize,
	nblks,
	outfiletype,
	n_outfiles,
	filenum,
	ckptfreq,
        fsmfreq,
	pipefd;


CKPTTYPE v_ckpt,
	 r_ckpt;

off_t	*datasizes;

int	resumeflag = FALSE,
	outfd = -1,
	volfirstrec = 0,
	trecnum = 0,
        fsmcounter = 0,
	retrynum = 0;

extern int semaid;

int machine_type;         /* machine type where the tape device is, rmt.c
			     can overwrite */

int SuccessfulAutoLoad = 0;
	/* This is the return value of load_next_reel.  */
	/* It is set to 1 after a successful auto load. */
	/* on the 3480.					*/


/***************************************************************************
    This function sets up the signal handler, shared memory, etc.  It also
    grabs all the values needed to communicate with the main process either
    from the command line, or through shared memory (after the shmat has been
    done).  It then simply waits in an infinite pause loop for the next signal
    telling it that there is some writer work to be done.
***************************************************************************/
    /* argv[0], [1],     [2],     [3]     [4]   [5]      [6]     [7]      ... [N+2]    */
    /* writer   shmemid, segsize, pipefd, lang, shmaddr, shmflg, outfile1 ... outfileN */
main(argc, argv)
int argc;
char **argv;
{
    int shmemid, shmflg;
    char *shmaddr;
    int dummyvar;
    unsigned recbufsize;
    int i;
    struct fn_list *tail;
    
    struct utsname name;

    lang = argv[4];
#ifdef NLS
    setlocale(LC_ALL, lang);
    nlmsg_fd = catopen("fbackup", 0);
#endif NLS

#ifdef DEBUG
for (i = 0; i < argc; i++) {
  fprintf(stderr, "writer: main(): argv[%d] = %s\n", i, argv[i]);
}
#endif

    shmemid = atoi(argv[1]);
    segsize = atoi(argv[2]);
    pipefd = atoi(argv[3]);
    shmaddr = atoi(argv[5]);
    shmflg = atoi(argv[6]);
    n_outfiles = argc - 7;

#ifdef DEBUG
fprintf(stderr, "writer: main(): argc = %d\n", argc);
fprintf(stderr, "writer: main(): n_outfiles = %d\n", n_outfiles);
#endif

    temp = (struct fn_list *)NULL;
    tail = (struct fn_list *)NULL;
    outfile_head = (struct fn_list *)NULL;
    for (i = 0; i < n_outfiles; i++) {
      temp = malloc(sizeof(struct fn_list));
      strcpy(temp->name, argv[7+i]);
      temp->next = (struct fn_list *)NULL;
      if (outfile_head == (struct fn_list *)NULL) {
	outfile_head = temp;
	outfile_head->next = temp;
	tail = temp;
      }
      else {
	tail->next = temp;
	tail = temp;
	tail->next = outfile_head;
      }
#ifdef DEBUG
fprintf(stderr, "writer: main(): build fn_list loop i = %d\n", i);
fprintf(stderr, "writer: main(): build fn_list loop outfile_head = %d\n", outfile_head);
fprintf(stderr, "writer: main(): build fn_list loop temp = %d\n", temp);
fprintf(stderr, "writer: main(): build fn_list loop temp->name = %s\n", temp->name);
fprintf(stderr, "writer: main(): build fn_list loop temp->next = %d\n", temp->next);
#endif
    }
    temp = outfile_head;
    outfptr = NULL;

/*  since series 700 machines come with 8.0 and mtget structs are converged
 *  for 8.0 it should be OK to let 700 machines default to 800
 */
    uname(&name);
    if (strncmp(name.machine, "9000/3", 6) == 0) {
      machine_type = 300;
    } else if (strncmp(name.machine, "9000/4", 6) == 0) {
      machine_type = 300;
    } else if (strncmp(name.machine, "9000/7", 6) == 0) {
      machine_type = 700;
    } else {
      machine_type = 800;
    }

#ifdef DEBUG_D
fprintf(stderr, "writer: machine_type = %d\n", machine_type);
#endif /* DEBUG_D */

    errno = 0;
    if ((int)((segment=shmat(shmemid, shmaddr, shmflg))) == -1) {
	msg((catgets(nlmsg_fd,NL_SETN,1, "fbackup(3001): shmat failed for the memory segment for the writer; errno: %d\n")), errno);
	wrtrabort();
    }

    pad = (PADTYPE*)((unsigned)segment + segsize);

    rec = pad->rec;
    semaid = pad->semaid;
    recsize = pad->recsize;
    nblks = segsize/BLOCKSIZE;
    ckptfreq = pad->ckptfreq;
    fsmfreq = pad->fsmfreq;
    filenum = pad->startfno;

    datasizes = (off_t*)((unsigned)pad + sizeof(PADTYPE));
    blkstatus = (char*)((unsigned)datasizes + nblks*sizeof(off_t));

    dummyvar = 0;	/* dummyvar is only here to keep lint happy */
    recbufsize = (sizeof(VHDRTYPE)+dummyvar > sizeof(LABELTYPE)) ?
			sizeof(VHDRTYPE)+1 : sizeof(LABELTYPE)+1;

    recbufsize = (recbufsize > recsize) ? recbufsize : recsize;
    recbuf = (char*)mymalloc(recbufsize);
    if (recbuf == (char*)NULL)  {
	msg((catgets(nlmsg_fd,NL_SETN,2, "fbackup(3002): out of virtual memory\n")));
	wrtrabort();
    }

    (void) sigvector(SIGUSR1, &wrtr_vec, (struct sigvec *)0);
    (void) sigvector(SIGUSR2, &end_vec,  (struct sigvec *)0);
    (void) sigvector(SIGINT,  &int_vec,  (struct sigvec *)0);

    pad->wrtrstatus = READY;		     /* tell the main proc I'm ready */
    (void) kill(pad->ppid, SIGUSR1);
    while (1) {
	(void) sigpause(0L);			/* wait for a signal */
    }
}






/***************************************************************************
    This function actually determines what must be done whenever the writer
    process wakes up.
***************************************************************************/
void
wrtrhandler()
{
    static int mrecord = 0;
    FHDRTYPE eob_buf;
    FHDRTYPE *eob_buf_ptr;
    char eob_blk[BLOCKSIZE];
    int size;
    int i;
    char *to;
    char *from;

    switch (pad->wrtrstatus) {
	case READY:			/* normal case, write a shmem record */
	    while (rec[mrecord].status == COMPLETE) {
		do_mrec(mrecord);
		if (resumeflag) {	/* there has been a tape write error */
		    pad->wrtrstatus = RESET;
		    (void) kill(pad->ppid, SIGUSR1);
		    break;
		} else {
		    rec[mrecord].status = FREE;		/* free this record */
		    mrecord = (mrecord+1)%pad->nrecs;
		    atomicadd(recsize);
		    (void) kill(pad->ppid, SIGUSR1);
		}
	    }
	break;
	case RESET:	    /* do nothing while main gets ready to go again */
	    (void) kill(pad->ppid, SIGUSR1);
	break;
	case RESUME:		/* get (writer) all ready to resume operation */
	    mrecord = 0;
	    resumeflag = FALSE;
	    pad->wrtrstatus = READY;
	    (void) kill(pad->ppid, SIGUSR1);
	break;
	case DONE:
  
/* clean up and tell main that I'm done 
   8.0 and later, ceate EOB (End Of Backup) header record and
   write to output before closing output file
*/

	    (void) strcpy(eob_buf.com.type, EOB);
	    (void) strcpy(eob_buf.com.last, EOH);
	    (void) strcpy(eob_buf.com.magic, MAGICSTR);
	    (void) sprintf(eob_buf.com.checksum, INTSTRFMT, 0);
	    (void) sprintf(eob_buf.id.ppid, INTSTRFMT, pad->pid);
	    (void) sprintf(eob_buf.id.time, INTSTRFMT, pad->begtime);
	    (void) sprintf(eob_buf.filenum, INTSTRFMT, -1);
	    size = sizeof(eob_buf);
	    eob_buf_ptr = &eob_buf;
	    
	    to = eob_blk;
	    for (i=0; i<BLOCKSIZE; i++, to++)
	      *to = FILLCHAR;

	    to = eob_blk;
	    from = eob_buf_ptr;
	    for (i=0; i<size; i++, to++, from++)
	      *to = *from;
	    
	    switch (outfiletype) {
	    case VDI_REMOTE:
#ifdef DEBUG_FSM
fprintf(stderr, "wrtrhandler : before remote eob writerec\n");
#endif	
	      writerec(eob_blk, (unsigned)BLOCKSIZE);
	      rmt_close(outfd);
	      break;
	    case VDI_MAGTAPE:
	    case VDI_DAT:
	    case VDI_DAT_FS:
	    case VDI_MO:
	    case VDI_DISK:
	    case VDI_STDOUT:
	    case VDI_REGFILE:
#ifdef DEBUG_FSM
fprintf(stderr, "wrtrhandler : before eob writerec\n");
#endif	
	      writerec(eob_blk, (unsigned)BLOCKSIZE);
	      if (qicflag)       /* To avoid single EOF in QIC */
		 vdi_ops(outfiletype, outfd, VDIOP_WTM1, 1); 
	      vdi_close(outfiletype, outfptr, &outfd, mttype);
	      break;
	    }
	    pad->wrtrstatus = EXIT;
	    (void) kill(pad->ppid, SIGUSR1);
	break;
	case EXIT:		/* do nothing; main will (eventually) kill me */
	break;
#ifdef DEBUG
	default:
	    msg("/W:default, wrtrstatus=%d\\\n", pad->wrtrstatus);
	    (void) kill(pad->ppid, SIGUSR1);
	break;
#else
       default:
       break;
#endif /* DEBUG */
    }
}






/***************************************************************************
    This function is called whenever an EOT condition occurs.  It closes the
    closes the old output file, updates the index, attempts to execute the
    change volume script (if one is has been specified), and calls newvol to
    get the next output file.
***************************************************************************/
nexttape(afterfile)
     int afterfile;
{
    msg((catgets(nlmsg_fd,NL_SETN,3, "fbackup(3003): normal EOT\n")));
    switch (outfiletype) {
    case VDI_REMOTE:
      rmt_close(outfd);
      break;
    case VDI_REGFILE:
    case VDI_MAGTAPE:
    	if (is3480) {
	    SuccessfulAutoLoad = load_next_reel(outfd);
    	}
    case VDI_DAT:
    case VDI_DAT_FS:
    case VDI_MO:
    case VDI_DISK:
    case VDI_STDOUT:
      if (qicflag)       /* To avoid single EOF in QIC */
          vdi_ops(outfiletype, outfd, VDIOP_WTM1, 1);	  
      vdi_close(outfiletype, outfptr, &outfd, mttype);
      break;
    }
    callupdateidx(afterfile);
    (pad->vol)++;
    if (pad->chgvol_file != (char*)NULL)
	(void) system(pad->chgvol_file);
    newvol();
}






/***************************************************************************
    This function is called whenever the writer process discovers that an
    unrecoverable error has occurred.  The main process discovers that this
    process has died by watching for a SIGCLD, then it kills any reader
    processes before exiting.
***************************************************************************/
wrtrabort()
{
    msg((catgets(nlmsg_fd,NL_SETN,4, "fbackup(3004): writer aborting\n")));
    exit(ERROR_EXIT);
}






		    /* These variables are used by both getfname and do_mrec. */
static char *fchptr, file[MAXPATHLEN];

/***************************************************************************
    This function extracts the filename from the header block(s).
    If necessary, it gets long file names (from multiple header blocks)
    from subsequent header blocks.
***************************************************************************/
getfname(chptr, lim)
char *chptr;
int lim;
{
    int i = 0;
    while (i < lim) {
	*fchptr++ = *chptr;
	if (*chptr++ == '\0')
	    return(FALSE);
	i++;
    }
    return(TRUE);
}






/* These variables are used by both do_mrec and writerec. */

static int  readingfname,
	    afterfile,
	    ht_ctr = 1,
	    d_ctr = 0,
	    max_d_ctr,
	    tapeerrflag = FALSE,
	     ckptfilenum,
	     ckptretrynum,
	     ckptblknum;
static off_t ckptdatasize;
static char ckptblktype;

static off_t tmpdatasize;
static INTCKPTTYPE r_internalckpt, v_internalckpt;

/***************************************************************************
    This function examines the data in one record of the  (shared memory)
    "ring" to see that any file(s) (partially or completely) contained in it
    were not active.  Each block of the ring has an associated type value
    (char blkstatus[]) and a data size (integer datasize[]) associated with it.
    The blkstatus values are valid for every block.  The datasize values are
    only valid for blocks of type HDRFIRST.  There is one byte in the trailer
    block which indicates that a file is active.
    There are 3 cases:
    1.) No active files, and no write error will happen (normal case): writerec
	is called to write this entire record.
    2.) An active file is present, but no write error will happen: writerec is
	called to write only the part of the record up to and including the
	trailer block for this record.  This function then sets the flags and
	values required to have both the main process and this (writer) process
	stop what they are presently doing, re-initialize, and then resume
	at the active file.
    3.) A write error will happen on the call to writerec: writerec is called
	to write all or part of this record (depending on whether there is an
	active file in this record or not).  writerec sets the flags and values
	required to have both the main process and this (writer) process
	either A) save the good data on this volume (if possible) and continue,
	or B) rewrite this volume from the beginning.  In either case, an new
	volume will be mounted and both the main process and this (writer)
	process will be restarted as in case 2 above.
***************************************************************************/
void
do_mrec(mrecord)
int mrecord;
{
    PBLOCKTYPE st_blk, blk;
    int lim, fno, st_blkno, blkno, count;
    int start_ch;     /* address of first character in hdr block */
    int start_blkno;  /* block number of hdr block */
    int num_blk;      /* number of blocks to write out */

    st_blk.ch = blk.ch = &segment[mrecord*recsize];
    st_blkno = blkno = mrecord * (recsize/BLOCKSIZE);

    if ((ckptblktype=blkstatus[st_blkno]) == HDRFIRST)
	afterfile = filenum - 1;
    else
	afterfile = filenum;

    if ((trecnum-volfirstrec)%ckptfreq == 0) {	/* save internal ckpt data */
	if (trecnum == 0 && outfd == -1)
	    newvol();
	r_internalckpt.readingfname = readingfname;
	r_internalckpt.tmpdatasize = tmpdatasize;
	r_internalckpt.ht_ctr = ht_ctr;
	r_internalckpt.d_ctr = d_ctr;
	if (trecnum == volfirstrec)
	    v_internalckpt = r_internalckpt;

	ckptfilenum = filenum;
	ckptretrynum = retrynum;
	switch (ckptblktype) {
	    case HDRFIRST: ckptblknum = 1; ckptdatasize = datasizes[st_blkno];
	    break;
	    case TRAILER:  ckptblknum = 1;        ckptdatasize = tmpdatasize;
	    break;
	    case HDRCONT:  ckptblknum = ht_ctr+1; ckptdatasize = tmpdatasize;
	    break;
	    case DATA:     ckptblknum = d_ctr+1;  ckptdatasize = tmpdatasize;
	    break;
	}
    }

/*
  The do_mrec_datfs function is special for dat fast search.  Since
  we need to put fast search marks every fsmfreq files and since a
  record could contain more that 1 file, we need to step through the
  record block by block.  When a hdr block is seen write a fast search
  mark if needed.  Then check how much of the remaining record can be
  written out.
*/

    count = rec[mrecord].count;
    lim = count / BLOCKSIZE;
    start_blkno = blkno;
    start_ch = st_blk.ch;
    num_blk = 0;

    while (lim--) {
      switch (blkstatus[blkno]) {
      case HDRFIRST: 
	if ((outfiletype == VDI_DAT) || (outfiletype == VDI_DAT_FS)) {
	  fsmcounter++;
	  if ((fsmcounter%fsmfreq) == 0) {  /* time to do a setmark */
	    if (num_blk) { /* write out what we have seen in this record */
#ifdef DEBUG_FSM
fprintf(stderr, "do_mrec : before writerec pre vdi_ops size = %d\n", ((num_blk)*BLOCKSIZE));
fprintf(stderr, "do_mrec : before writerec pre vdi_ops fsmcounter = %d\n", fsmcounter);
#endif	
              writerec(start_ch, (unsigned)(num_blk * BLOCKSIZE));
           }	
	    if ((vdi_ops(outfiletype, outfd, VDIOP_WTM2, 1)) < 0) {
	      tapeerrflag = TRUE;
	      return;
	    }
	    /* reset where we are in this record */
	    start_blkno = blkno;
	    start_ch = blk.ch;
	    num_blk =0;
	  }
	} /* if DAT */
	d_ctr = 0; 
	ht_ctr = 1;
	(void) sscanf(blk.hdr->filenum, "%d", &fno);
	if (filenum < fno) {
	  while (filenum < fno)
	    msg((catgets(nlmsg_fd,NL_SETN,5, "fbackup(3005): WARNING: file number %d was NOT backed up\n")), filenum++);
	} else if (filenum > fno) {
	  msg((catgets(nlmsg_fd,NL_SETN,6, "fbackup(3006): writer lost synchronization, fno=%d, filenum=%d\n")), fno, filenum);
	  filenum = fno;
	}
	tmpdatasize = datasizes[blkno];

#ifdef DEBUG_FILE
msg("(fno=%d,fn#=%d,rt=%d,dsz=%d,", fno, filenum, retrynum, tmpdatasize);
#endif

	fchptr = file;
	readingfname = getfname(blk.ch+sizeof(FHDRTYPE), BLOCKSIZE-sizeof(FHDRTYPE));
	break;

      case HDRCONT: ht_ctr++;
	if (readingfname)
	  readingfname = getfname(blk.ch+sizeof(BLKID), BLOCKSIZE-sizeof(BLKID));
	break;

      case TRAILER: 
	max_d_ctr = d_ctr; 
	d_ctr = 0; 
	ht_ctr = 0;
	(void) sscanf(blk.trl->filenum, "%d", &fno);
#ifdef DEBUG_FILE
msg("%d)(%s)",fno, (blk.trl->status));
#endif 
	if (pad->vflag && !tapeerrflag)
	  msg("%5d: %s\n", fno, file);

	if (strcmp(blk.trl->status, ASCIIGOOD)) {
	  if (tapeerrflag || ((retrynum < pad->maxretries) &&
			      (retrynum*max_d_ctr < pad->retrylim))) {

	    if (!tapeerrflag) {
	      msg((catgets(nlmsg_fd,NL_SETN,7, "fbackup(3007): WARNING: File number %d (%s)\n")), filenum, file);
	      msg((catgets(nlmsg_fd,NL_SETN,8, "	was active during attempt number %d\n")), retrynum+1);

#ifdef DEBUG
	    } else {
	      msg("fbackup: reset: nulling file %d\n", filenum);
#endif
	    }

#ifdef DEBUG_FSM
fprintf(stderr, "do_mrec : before trailer writerec\n");
#endif	
	    writerec(st_blk.ch, (unsigned) ((blkno-st_blkno+1)*BLOCKSIZE));
	    if (!tapeerrflag)
	      retrynum++;

	    if (!resumeflag) {	/* if no wrt err on this rec */
	      resumeflag = TRUE;/* resume @ beg this file */
	      pad->reset.filenum = filenum;
	      pad->reset.blktype = HDRFIRST;
	      tapeerrflag = FALSE;
	    }
	    return;
	  } else {
	    msg((catgets(nlmsg_fd,NL_SETN,9, "fbackup(3009): WARNING: File number %d (%s)\n")), filenum, file);
	    msg((catgets(nlmsg_fd,NL_SETN,10, "	was not successfully backed up\n")));
	  }
	}
	filenum++;
	retrynum = 0;
	tapeerrflag = FALSE;
	break;

      case DATA: 
	d_ctr++;
	break;
      }  /* end switch(block) */
      blkno = (blkno+1) % nblks;
      blk.ch += BLOCKSIZE;
      num_blk++;
    }  /* end while */

    if ((outfiletype == VDI_DAT) || (outfiletype == VDI_DAT_FS)) {
#ifdef DEBUG_FSM
fprintf(stderr, "do_mrec DAT : before end writerec = %d\n", (num_blk * BLOCKSIZE));
#endif	
      writerec(start_ch, (unsigned)(num_blk * BLOCKSIZE));		  
    } else {
#ifdef DEBUG_FSM
fprintf(stderr, "do_mrec : before end writerec\n");
#endif	
      writerec(st_blk.ch, (unsigned)count);
    } 
}  /* end do_mrec */



/***************************************************************************
    Writerec is called to write a buffer of data to the output file.  If this
    file is a regular file or stdout, it simply tries the write.  If it fails,
    a message is printed and the writer aborts.  If the output file is a
    magtape drive, many more possibilites exist.
	First, it checks to see if a checkpoint record belongs before the
    record which is about to be written.  If it does, the checkpoint record
    is built (at the beginning of a volume, a "volume checkpoint" record is
    built before the "record checkpoint").  It then writes an EOF mark to the
    tape.  If this fails, tapeerr is called to handle the error condition.
    (Note that an EOT condition cannot happen while writeing an EOF mark.)
    It then writes the approprate checkpoint record, and again, if it fails,
    tapeerr is called.  However, since this is a write of a data buffer, an
    EOT condition may occur here.  If it does, the next volume is requested,
    and we try to write this record again.  (Note that if the next volume is
    not a magtape, no more checkpointing is appropriate, and isn't done.)
	Whether or not we wrote a checkpoint record or not, we finally get to
    the point where we need to write the buffer we originally wanted to write.
    Again, if the write fails, we call tapeerr to handle the error.  If an
    ordinary EOT is encountered, again, a new tape is requested and we try
    writing this buffer again.

    The switch statement in writerec is used to control the format of
    the media.  Different media have different formats.
***************************************************************************/
void
writerec(addr, size)
char *addr;
unsigned size;
{
    int n;
    CKPTTYPE oldr_ckpt;
    struct mtget mtget_buf;
    int eot_flag = 0;
    struct vdi_gatt status;

    if (resumeflag)
	return;

    do {
newvolaftertapeerr:		/* when a new volume is mounted, resume here */
      switch (outfiletype) {
      case VDI_MAGTAPE:
	/* checkpoint record logic */
	if ((trecnum-volfirstrec)%ckptfreq == 0) {
	  if (trecnum == volfirstrec) {
	    if (!tapeerrflag) {
	    (void) sprintf(v_ckpt.filenum, INTSTRFMT, filenum);
	    (void) sprintf(v_ckpt.trecnum, INTSTRFMT, trecnum);
	    (void) sprintf(v_ckpt.retrynum, INTSTRFMT,retrynum);
	    (void) sprintf(v_ckpt.datasize, INTSTRFMT,tmpdatasize);
	    (void) sprintf(v_ckpt.blknum, INTSTRFMT, d_ctr);
	    }
	    v_ckpt.blktype = ckptblktype;
	    oldr_ckpt = r_ckpt = v_ckpt;
	  } else {
	    oldr_ckpt = r_ckpt;
	    (void) sprintf(r_ckpt.filenum, INTSTRFMT, ckptfilenum);
	    (void) sprintf(r_ckpt.trecnum, INTSTRFMT, trecnum);
	    (void) sprintf(r_ckpt.retrynum, INTSTRFMT,ckptretrynum);
	    (void) sprintf(r_ckpt.datasize, INTSTRFMT,ckptdatasize);
	    (void) sprintf(r_ckpt.blknum, INTSTRFMT, ckptblknum);
	    r_ckpt.blktype = ckptblktype;
	  }
	  if ((vdi_ops(outfiletype, outfd, VDIOP_WTM1, 1)) != 0) {
	    msg((catgets(nlmsg_fd,NL_SETN,11, "fbackup(3011): WRITE ERROR while writing a checkpoint EOF,")));
	    tapeerrflag = TRUE;
	    tapeerr(outfiletype, outfptr, &readingfname, &ht_ctr, &d_ctr,
		    &tmpdatasize, &r_internalckpt, &v_internalckpt);
	    return;
	  }
	  
	  n = vdi_write(outfiletype, outfd, (char *)&r_ckpt, sizeof(CKPTTYPE));
	  if (n != sizeof(CKPTTYPE)) {
	    if (vdi_get_att(outfiletype, outfd, VDIGAT_ISEOM, &status) != 0) {
	      msg((catgets(nlmsg_fd,NL_SETN,26, "fbackup(3026): ioctl error, can't query outfile\n")));
	      tapeerrflag = TRUE;
	      tapeerr(outfiletype, outfptr, &readingfname, &ht_ctr, &d_ctr, &tmpdatasize, &r_internalckpt, &v_internalckpt);
	      return;
	    }
	    if (status.eom) {
	      nexttape(afterfile);		/* normal EOT */
	      goto newvolaftertapeerr;
	    } else {
	      r_ckpt = oldr_ckpt;	/* write error */
	      (void) vdi_ops(outfiletype, outfd, VDIOP_BSF, 1);
	      msg((catgets(nlmsg_fd,NL_SETN,12, "fbackup(3012): WRITE ERROR while writing checkpoint record,")));
	      tapeerrflag = TRUE;
	      tapeerr(outfiletype, outfptr, &readingfname, &ht_ctr, &d_ctr, &tmpdatasize, &r_internalckpt, &v_internalckpt);
	      return;
	    }
	  }
	}
	
#ifdef DEBUG_MAG
fprintf(stderr, "writerec MAG: size before write = %d\n", size);
#endif	
	if ((n=vdi_write(outfiletype, outfd, addr, size)) != size) {
#ifdef DEBUG_MAG
fprintf(stderr, "writerec MAG: n after write = %d\n", n);
#endif	
	  if (vdi_get_att(outfiletype, outfd, VDIGAT_ISEOM, &status) != 0) {
	    msg((catgets(nlmsg_fd,NL_SETN,27, "fbackup(3027): ioctl error, can't query outfile\n")));
	    tapeerrflag = TRUE;
	    tapeerr(outfiletype, outfptr, &readingfname, &ht_ctr, &d_ctr, &tmpdatasize, &r_internalckpt, &v_internalckpt);
	    return;
	  }
	  if (status.eom) {
	    nexttape(afterfile);		/* normal EOT */
	    goto newvolaftertapeerr;
	  } else {
	    msg((catgets(nlmsg_fd,NL_SETN,13, "fbackup(3013): WRITE ERROR while writing data record,")));
	    tapeerrflag = TRUE;
	    tapeerr(outfiletype, outfptr, &readingfname, &ht_ctr, &d_ctr, &tmpdatasize, &r_internalckpt, &v_internalckpt);
	    return;
	  }
	}
	break;
      case VDI_REGFILE:
      case VDI_STDOUT:
	if ((n=vdi_write(outfiletype, outfd, addr, size)) != size) {
	  msg((catgets(nlmsg_fd,NL_SETN,14, "fbackup(3014): WRITE ERROR: could not write to the output file\n")));
	  wrtrabort();
	}
	break;
      case VDI_MO:
      case VDI_DISK:
	if ((n=vdi_write(outfiletype, outfd, addr, size)) != size) {
	  status.eom = FALSE;
	  if (n >= 0) {  /* no error, but since write not full assume at EOM */
	    status.eom = TRUE;
	  } 	
#ifdef _WSIO  /* 300 and 700 MO handling */
	  if ((n < 0) && (vdi_errno == ENOSPC)) {
	    status.eom = TRUE;
	  } 
#else  /* 800 MO handling */
	  if ((n < 0) && (vdi_errno == ENXIO)) {
	    status.eom = TRUE;
	  } 
#endif
	  if (status.eom) {
	    nexttape(afterfile);		/* normal EOT */
	    goto newvolaftertapeerr;
	  } else {
	    msg((catgets(nlmsg_fd,NL_SETN,43, "fbackup(3043): WRITE ERROR while writing data record,")));
	    wrtrabort();
	  }
	}
	break;
      case VDI_DAT:
      case VDI_DAT_FS:
	/* checkpoint record logic */
	if ((trecnum-volfirstrec)%ckptfreq == 0) {
	  if (trecnum == volfirstrec) {
	    if (!tapeerrflag) {
	    (void) sprintf(v_ckpt.filenum, INTSTRFMT, filenum);
	    (void) sprintf(v_ckpt.trecnum, INTSTRFMT, trecnum);
	    (void) sprintf(v_ckpt.retrynum, INTSTRFMT,retrynum);
	    (void) sprintf(v_ckpt.datasize, INTSTRFMT,tmpdatasize);
	    (void) sprintf(v_ckpt.blknum, INTSTRFMT, d_ctr);
	    }
	    v_ckpt.blktype = ckptblktype;
	    oldr_ckpt = r_ckpt = v_ckpt;
	  } else {
	    oldr_ckpt = r_ckpt;
	    (void) sprintf(r_ckpt.filenum, INTSTRFMT, ckptfilenum);
	    (void) sprintf(r_ckpt.trecnum, INTSTRFMT, trecnum);
	    (void) sprintf(r_ckpt.retrynum, INTSTRFMT,ckptretrynum);
	    (void) sprintf(r_ckpt.datasize, INTSTRFMT,ckptdatasize);
	    (void) sprintf(r_ckpt.blknum, INTSTRFMT, ckptblknum);
	    r_ckpt.blktype = ckptblktype;
	  }
	  if ((vdi_ops(outfiletype, outfd, VDIOP_WTM1, 1)) != 0) {
	    msg((catgets(nlmsg_fd,NL_SETN,11, "fbackup(3011): WRITE ERROR while writing a checkpoint EOF,")));
	    tapeerrflag = TRUE;
	    tapeerr(outfiletype, outfptr, &readingfname, &ht_ctr, &d_ctr,
		    &tmpdatasize, &r_internalckpt, &v_internalckpt);
	    return;
	  }
	  
	  n = vdi_write(outfiletype, outfd, (char *)&r_ckpt, sizeof(CKPTTYPE));
	  if (n != sizeof(CKPTTYPE)) {
	    if (vdi_get_att(outfiletype, outfd, VDIGAT_ISEOM, &status) != 0) {
	      msg((catgets(nlmsg_fd,NL_SETN,26, "fbackup(3026): ioctl error, can't query outfile\n")));
	      tapeerrflag = TRUE;
	      tapeerr(outfiletype, outfptr, &readingfname, &ht_ctr, &d_ctr, &tmpdatasize, &r_internalckpt, &v_internalckpt);
	      return;
	    }
	    if (status.eom) {
	      nexttape(afterfile);		/* normal EOT */
	      goto newvolaftertapeerr;
	    } else {
	      r_ckpt = oldr_ckpt;	/* write error */
	      (void) vdi_ops(outfiletype, outfd, VDIOP_BSF, 1);
	      msg((catgets(nlmsg_fd,NL_SETN,12, "fbackup(3012): WRITE ERROR while writing checkpoint record,")));
	      tapeerrflag = TRUE;
	      tapeerr(outfiletype, outfptr, &readingfname, &ht_ctr, &d_ctr, &tmpdatasize, &r_internalckpt, &v_internalckpt);
	      return;
	    }
	  }
	}
	
#ifdef DEBUG_FSM
fprintf(stderr, "writerec DAT: size before write = %d\n", size);
#endif	
	if ((n=vdi_write(outfiletype, outfd, addr, size)) != size) {
#ifdef DEBUG_FSM
fprintf(stderr, "writerec DAT: n after write = %d\n", n);
#endif	
	  if (vdi_get_att(outfiletype, outfd, VDIGAT_ISEOM, &status) != 0) {
	    msg((catgets(nlmsg_fd,NL_SETN,44, "fbackup(3044): ioctl error, can't query outfile\n")));
	    tapeerrflag = TRUE;
	    tapeerr(outfiletype, outfptr, &readingfname, &ht_ctr, &d_ctr, &tmpdatasize, &r_internalckpt, &v_internalckpt);
	    return;
	  }
	  if (status.eom) {
	    nexttape(afterfile);		/* normal EOT */
	    goto newvolaftertapeerr;
	  } else {
	    msg((catgets(nlmsg_fd,NL_SETN,45, "fbackup(3045): WRITE ERROR while writing data record,")));
	    tapeerrflag = TRUE;
	    tapeerr(outfiletype, outfptr, &readingfname, &ht_ctr, &d_ctr, &tmpdatasize, &r_internalckpt, &v_internalckpt);
	    return;
	  }
	}
	break;
      case VDI_REMOTE:
	if ((trecnum-volfirstrec)%ckptfreq == 0) {
	  if (trecnum == volfirstrec) {
	    if (!tapeerrflag) {
	    (void) sprintf(v_ckpt.filenum, INTSTRFMT, filenum);
	    (void) sprintf(v_ckpt.trecnum, INTSTRFMT, trecnum);
	    (void) sprintf(v_ckpt.retrynum, INTSTRFMT,retrynum);
	    (void) sprintf(v_ckpt.datasize, INTSTRFMT,tmpdatasize);
	    (void) sprintf(v_ckpt.blknum, INTSTRFMT, d_ctr);
	    }
	    v_ckpt.blktype = ckptblktype;
	    oldr_ckpt = r_ckpt = v_ckpt;
	  } else {
	    oldr_ckpt = r_ckpt;
	    (void) sprintf(r_ckpt.filenum, INTSTRFMT, ckptfilenum);
	    (void) sprintf(r_ckpt.trecnum, INTSTRFMT, trecnum);
	    (void) sprintf(r_ckpt.retrynum, INTSTRFMT,ckptretrynum);
	    (void) sprintf(r_ckpt.datasize, INTSTRFMT,ckptdatasize);
	    (void) sprintf(r_ckpt.blknum, INTSTRFMT, ckptblknum);
	    r_ckpt.blktype = ckptblktype;
	  }
	  if (!mtoper(outfd, MTWEOF, 1)) {
	    msg((catgets(nlmsg_fd,NL_SETN,28, "fbackup(3028): WRITE ERROR while writing a checkpoint EOF,")));
	    tapeerrflag = TRUE;
	    tapeerr(outfiletype, outfptr, &readingfname, &ht_ctr, &d_ctr,
		    &tmpdatasize, &r_internalckpt, &v_internalckpt);
	    return;
	  }
	  
	  n = rmt_write(outfd, (char *)&r_ckpt, sizeof(CKPTTYPE));
	  if (n != sizeof(CKPTTYPE)) {
	    getstat(outfd, &mtget_buf);
#ifdef DEBUG_D
	    fprintf(stderr, "EOT machine_type = %d\n", machine_type);
	    fprintf(stderr, "EOT mtget_buf.mt_type = %#lx\n", mtget_buf.mt_type);
	    fprintf(stderr, "EOT mtget_buf.mt_resid = %#lx\n", mtget_buf.mt_resid);
	    fprintf(stderr, "EOT mtget_buf.mt_dsreg1 = %#lx\n", mtget_buf.mt_dsreg1);
	    fprintf(stderr, "EOT mtget_buf.mt_dsreg2 = %#lx\n", mtget_buf.mt_dsreg2);
	    fprintf(stderr, "EOT mtget_buf.mt_gstat = %#lx\n", mtget_buf.mt_gstat);
	    fprintf(stderr, "EOT mtget_buf.mt_errg = %#lx\n", mtget_buf.mt_erreg);
#endif /* DEBUG_D */
	    if (machine_type == 300) {
	      eot_flag = GMT_300_EOT(mtget_buf.mt_dsreg1);
	    }
	    else {
	      eot_flag = GMT_800_EOT(mtget_buf.mt_gstat);
	    }
#ifdef DEBUG_D
	    fprintf(stderr, "EOT eot_flag = %d\n", eot_flag);
#endif /* DEBUG_D */
	    if (eot_flag) {
	      nexttape(afterfile);		/* normal EOT */
	      goto newvolaftertapeerr;
	    } else {
	      r_ckpt = oldr_ckpt;	/* write error */
	      (void) mtoper(outfd, MTBSF, 1);
	      msg((catgets(nlmsg_fd,NL_SETN,29, "fbackup(3029): WRITE ERROR while writing checkpoint record,")));
	      tapeerrflag = TRUE;
	      tapeerr(outfiletype, outfptr, &readingfname, &ht_ctr, &d_ctr, &tmpdatasize, &r_internalckpt, &v_internalckpt);
	      return;
	    }
	  }
	}

	if ((n=rmt_write(outfd, addr, size)) != size) {
	  getstat(outfd, &mtget_buf);
	  if (machine_type == 300) {
	    eot_flag = GMT_300_EOT(mtget_buf.mt_dsreg1);
	  }
	  else {
	    eot_flag = GMT_800_EOT(mtget_buf.mt_gstat);
	  }
	  if (eot_flag) {
	    nexttape(afterfile);		/* normal EOT */
	    goto newvolaftertapeerr;
	  } else {
	    msg((catgets(nlmsg_fd,NL_SETN,30, "fbackup(3030): WRITE ERROR while writing data record,")));
	    tapeerrflag = TRUE;
	    tapeerr(outfiletype, outfptr, &readingfname, &ht_ctr, &d_ctr, &tmpdatasize, &r_internalckpt, &v_internalckpt);
	    return;
	  }
	}
	break;
      }
    } while (n != size);
    trecnum++;
}  /* end writerec */


void
endhandler()
{
#ifdef PFA
    (void) sigvector(SIGUSR1, &dflsig_vec, (struct sigvec *)0);
    (void) sigvector(SIGUSR2, &dflsig_vec, (struct sigvec *)0);
    (void) sigvector(SIGINT,  &dflsig_vec, (struct sigvec *)0);
#endif PFA
    exit(0);
}



/***************************************************************************
    This function is called whenever it is necessary to mount a new volume.
    Note that the new volume may or may not be magnetic tape, but whenever
    a volume is not a magtape, an end of volume is an unrecoverable error.
    That is, only magnetic tapes are allowed to end before the data is
    exhausted.
	This function first checks to see if the new volume is the stdout.
    If it is, it sets the output to go there.  If it isn't, newvol stats
    the new filename and then goes through a battery of checks and tests.
    First it ensures that the new file isn't a block special device.  After
    that, it determines if it is a regular file or a character special device.
    If it is the latter, it checks to see that it is one of the two legal
    types of raw magtape devices.  It then ensures that the tape is at BOT,
    and calls a routine to see if the tape we mounted is OK (ie, hasn't been
    used too many times or is another reel of this session).
	If there is some problem opening the output file, the user is given
    the opportunity to specify a new output file (and/or abort), and then
    try all over again.  When a file is finally OK, functions are called to 
    write the label, volume header, and the index.
***************************************************************************/
void
newvol()
{
    struct stat statbuf;
    struct mtget mtget_buf;
    struct vdi_gatt vdigattbuf;
    char str[MAXPATHLEN], *oldoutfptr;
    int s, n_uses=0;
    int bot_flag = 0;
    int begerrflag;
    int i;
    char   nostr[16], yesstr[16];  /* add for msg 3310 */

    outfd = CLOSED;
    if (outfptr == NULL) {
      oldoutfptr = outfptr;
      outfptr = outfile_head->name;
    }
    else {
/* If the auto load succeeds in the 3480, there is no need to scan the
 * list of output devices. i.e., SuccessfulAutoLoad == 1.
 */
      if (!SuccessfulAutoLoad) {
      temp = temp->next;
      oldoutfptr = outfptr;
      outfptr = temp->name;
    }
    }
    volfirstrec = trecnum;

    while (outfd == CLOSED) {
      is3480 = 0;      /* reset is3480 */
      is8mm = 0;      /* reset is8mm */
      (void) vdi_identify(outfptr, &outfiletype);

#ifdef DEBUG
fprintf(stderr, "writer.c:newvol: outfiletype is %d\n", outfiletype);
fprintf(stderr, "writer.c:newvol: oldoufptr is %s\n", oldoutfptr);
fprintf(stderr, "writer.c:newvol: oufptr is %s\n", outfptr);
#endif 

      switch (outfiletype) {
      case VDI_REGFILE:
	outfiletype = VDI_REGFILE;
	if ((outfd = vdi_open(outfiletype, outfptr, (O_RDWR | O_CREAT))) < 0) {
	  msg((catgets(nlmsg_fd,NL_SETN,54, "fbackup(3054): could not open output file %s\n")), outfptr);
	  outfd = CLOSED;
	}
	break;
      case VDI_STDOUT:
	outfd = 1;
	break;
      case VDI_REMOTE:  /* deal directly with rmt routines */
	if (rmt_stat(outfptr, &statbuf) != -1) {	/* could stat it */
	  if ((statbuf.st_mode & S_IFMT) == S_IFBLK) {
	    msg((catgets(nlmsg_fd,NL_SETN,15, "fbackup(3015): block special output devices aren't supported\n")));
	  } else 
	    if ((statbuf.st_mode & S_IFMT) == S_IFCHR) {
	      if (!strcmp(oldoutfptr, outfptr)) {
		msg((catgets(nlmsg_fd,NL_SETN,16, "fbackup(3016): hit return when volume %d is ready on %s?\n")), pad->vol, outfptr);
		(void) fgets(str, MAXPATHLEN, stdin);
	      }
	      if (openoutfd(outfptr, &outfd) && isamagtape(outfd)) {
		if ((mttype=getmttype(&statbuf)) == ILLEGALMT) {
		  close_magtape(outfptr, &outfd, mttype);
		} else {
		  s = getstat(outfd, &mtget_buf);
#ifdef DEBUG_D
  fprintf(stderr, "BOT machine_type = %d\n", machine_type);
  fprintf(stderr, "BOT mtget_buf.mt_type = %#lx\n", mtget_buf.mt_type);
  fprintf(stderr, "BOT mtget_buf.mt_resid = %#lx\n", mtget_buf.mt_resid);
  fprintf(stderr, "BOT mtget_buf.mt_dsreg1 = %#lx\n", mtget_buf.mt_dsreg1);
  fprintf(stderr, "BOT mtget_buf.mt_dsreg2 = %#lx\n", mtget_buf.mt_dsreg2);
  fprintf(stderr, "BOT mtget_buf.mt_gstat = %#lx\n", mtget_buf.mt_gstat);
  fprintf(stderr, "BOT mtget_buf.mt_errg = %#lx\n", mtget_buf.mt_erreg);
#endif /* DEBUG_D */
		  if ((machine_type == 300) || (machine_type == 700)) {
		    bot_flag = GMT_300_BOT(mtget_buf.mt_dsreg1);
		  }
		  else {
		    bot_flag = GMT_800_BOT(mtget_buf.mt_gstat);
		  }
#ifdef DEBUG_D
		      fprintf(stderr, "BOT bot_flag = %d\n", bot_flag);
#endif /* DEBUG_D */
			
		  if (!bot_flag) {
		    msg((catgets(nlmsg_fd,NL_SETN,17, "fbackup(3017): the tape is not at the beginning, rewinding it\n")));
		    (void) mtoper(outfd, MTREW, 1);
		  }
		  if (!rmt_newtapeok(outfd, &n_uses)) {    /* reads the tape at the */
		    close_magtape(outfptr, &outfd, mttype);  /* current density. If this */
		  }				     /* is different than the specified */
		  else {				  /* density, the tape is incorrectly */
		    close_magtape(outfptr, &outfd, mttype);  /* written at the wrong density. The */
		    (void) openoutfd(outfptr, &outfd);	  /* close and open force the specified */
		  }					/* density! */
	      } /* end else getmttype */ 
	    } /* end if openoufd && isamagtape */
	  } else if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
	    (void) creatoutfd(outfptr, &outfd);
  } else {
	    msg((catgets(nlmsg_fd,NL_SETN,18, "fbackup(3018): output file %s is an illegal type\n")), outfptr);
	  }
	} 
	else {  /* could not stat file so create it */
	  (void) creatoutfd(outfptr, &outfd);
	} /* if rmt_stat */
	break;
      case VDI_MAGTAPE:
	stat(outfptr, &statbuf);
	if (!strcmp(oldoutfptr, outfptr) && !SuccessfulAutoLoad) {
/* Leave the following two statements as comments because the localized catalog
   had passed the change deadline.  These two statements should be removed
   at next release.   Charlie Tuan 6/11/92
*/
/*	  msg((catgets(nlmsg_fd,NL_SETN,31, "fbackup(3031): hit return when volume %d is ready on %s?\n")), pad->vol, outfptr);
	  (void) fgets(str, MAXPATHLEN, stdin);
*/  
#ifdef NLS
          (void) strcpy(yesstr, nl_langinfo(YESSTR));
          (void) strcpy(nostr,  nl_langinfo(NOSTR));
#else NLS
          (void) strcpy(yesstr, "yes");
          (void) strcpy(nostr,  "no");
#endif NLS
	  if (!query(msg((catgets(nlmsg_fd,NL_SETN,310, "fbackup(3310): enter '%s' when volume %d is ready on %s,\n or '%s' to discontinue:\n")), yesstr, pad->vol, outfptr, nostr))) {
	    vdi_close(outfiletype, outfptr, &outfd, mttype);
	    break;
	  } 
	}
	
	if ((outfd = vdi_open(outfiletype, outfptr, O_RDWR)) < 0) {
	  msg((catgets(nlmsg_fd,NL_SETN,32, "fbackup(3032): could not open output file %s\n")), outfptr);
	  break;
	}	

	if ((mttype = getmttype(&statbuf)) == ILLEGALMT) {
	  msg((catgets(nlmsg_fd,NL_SETN,33, "fbackup(3033): This output file is an illegal type; please specify a ")));
	  msg((catgets(nlmsg_fd,NL_SETN,34, "UCB mode device\n	with no-rewind, or an AT&T mode device with ")));
	  msg((catgets(nlmsg_fd,NL_SETN,35, "rewind set\n")));
	  vdi_close(outfiletype, outfptr, &outfd, mttype);
	  break;
	}
	
	vdi_get_att(outfiletype, outfd, VDIGAT_ISBOM, &vdigattbuf);
	if (!vdigattbuf.bom) {
	  if (vdi_ops(outfiletype, outfd, VDIOP_REW, 1) != 0) {
	    msg((catgets(nlmsg_fd,NL_SETN,36, "fbackup(3036): could not position media %s to beginning\n")), outfptr);
	    vdi_close(outfiletype, outfptr, &outfd, mttype);
	    break;
	  }
	}
	if (!newtapeok(outfiletype, outfd, &n_uses)) {
	  vdi_close(outfiletype, outfptr, &outfd, mttype);
	}
	break;
      case VDI_DAT:
      case VDI_DAT_FS:
	stat(outfptr, &statbuf);
	if (!strcmp(oldoutfptr, outfptr)) {
	  msg((catgets(nlmsg_fd,NL_SETN,46, "fbackup(3046): hit return when volume %d is ready on %s?\n")), pad->vol, outfptr);
	  (void) fgets(str, MAXPATHLEN, stdin);
	}
	
	if ((outfd = vdi_open(outfiletype, outfptr, O_RDWR)) < 0) {
	  msg((catgets(nlmsg_fd,NL_SETN,47, "fbackup(3047): could not open output file %s\n")), outfptr);
	  break;
	}	
	vdi_get_att(outfiletype, outfd, VDIGAT_ISBOM, &vdigattbuf);
	if (!vdigattbuf.bom) {
	  if (vdi_ops(outfiletype, outfd, VDIOP_REW, 1) != 0) {
	    msg((catgets(nlmsg_fd,NL_SETN,48, "fbackup(3048): could not position media %s to beginning\n")), outfptr);
	    vdi_close(outfiletype, outfptr, &outfd, mttype);
	    break;
	  }
	}
	if (!newtapeok(outfiletype, outfd, &n_uses)) {
	  vdi_close(outfiletype, outfptr, &outfd, mttype);
	}
/*
 * If the DAT has nothing on it (ie. it's new) we return -1 in n_uses.
 * We need to close and reopen the device or writes will fail with ENOSPC.
 * Perhaps this is a bug in the DAT driver.
 */
	if (n_uses == -1) {
	  vdi_close(outfiletype, outfptr, &outfd, mttype);

	  if ((outfd = vdi_open(outfiletype, outfptr, O_RDWR)) < 0) {
	    msg((catgets(nlmsg_fd,NL_SETN,47, "fbackup(3047): could not open output file %s\n")), outfptr);
	  }
	}
	break;
      case VDI_MO:
	stat(outfptr, &statbuf);
	if (!strcmp(oldoutfptr, outfptr)) {
	  msg((catgets(nlmsg_fd,NL_SETN,49, "fbackup(3049): hit return when volume %d is ready on %s?\n")), pad->vol, outfptr);
	  (void) fgets(str, MAXPATHLEN, stdin);
	}
	
	if ((outfd = vdi_open(outfiletype, outfptr, O_RDWR)) < 0) {
	  msg((catgets(nlmsg_fd,NL_SETN,50, "fbackup(3050): could not open output file %s\n")), outfptr);
	  break;
	}	
	if (!newtapeok(outfiletype, outfd, &n_uses)) {
	  vdi_close(outfiletype, outfptr, &outfd, mttype);
	} 
	else {  /* since newtapeok moves file pointer we need to close and open to reposition */
	  vdi_close(outfiletype, outfptr, &outfd, 0);
	  outfd = vdi_open(outfiletype, outfptr, O_RDWR);
	}
	break;
      case VDI_DISK:
	stat(outfptr, &statbuf);
	if (!strcmp(oldoutfptr, outfptr)) {
	  msg((catgets(nlmsg_fd,NL_SETN,51, "fbackup(3051): hit return when volume %d is ready on %s?\n")), pad->vol, outfptr);
	  (void) fgets(str, MAXPATHLEN, stdin);
	}
	
	if ((outfd = vdi_open(outfiletype, outfptr, O_RDWR)) < 0) {
	  msg((catgets(nlmsg_fd,NL_SETN,52, "fbackup(3052): could not open output file %s\n")), outfptr);
	  break;
	}	
	if (!newtapeok(outfiletype, outfd, &n_uses)) {
	  vdi_close(outfiletype, outfptr, &outfd, mttype);
	} 
	else {  /* since newtapeok moves file pointer we need to close and open to reposition */
	  vdi_close(outfiletype, outfptr, &outfd, 0);
	  outfd = vdi_open(outfiletype, outfptr, O_RDWR);
	}
	break;
      case VDI_UNKNOWN:
	outfiletype = VDI_REGFILE;
	if ((outfd = vdi_open(outfiletype, outfptr, (O_RDWR | O_CREAT))) < 0) {
	  msg((catgets(nlmsg_fd,NL_SETN,37, "fbackup(3037): could not open output file %s\n")), outfptr);
	  outfd = CLOSED;
	}
	stat(outfptr, &statbuf);
	break;
      }  /* switch outfiletype */

      if (outfd == CLOSED) {

	if (query((catgets(nlmsg_fd,NL_SETN,19, "fbackup(3019): would you like to enter a new output file?\n")))) {
	  msg((catgets(nlmsg_fd,NL_SETN,20, "fbackup(3020): please enter the new file's name:\n")));
	  (void) fgets(str, MAXPATHLEN, stdin);
	  str[strlen(str)-1] = '\0';		/* strip off '\n' */
	  outfptr = (char*)mymalloc((unsigned)(strlen(str)+1));
	  if (outfptr == (char*)NULL) {
	    msg((catgets(nlmsg_fd,NL_SETN,21, "fbackup(3021): out of virtual memory\n")));
	    wrtrabort();
	  }
	  (void) strcpy(outfptr, str);
	  strcpy(temp->name, outfptr);
	} 
	else {
	  if (query((catgets(nlmsg_fd,NL_SETN,22, "fbackup(3022): would you like to continue this session?\n"))))
	  msg((catgets(nlmsg_fd,NL_SETN,23, "fbackup(3023): please ensure that volume %d is ready on %s\n")), pad->vol, outfptr);
	  else
	  wrtrabort();
	}
      } 
      else {
	begerrflag = FALSE;
	msg((catgets(nlmsg_fd,NL_SETN,24, "fbackup(3024): writing volume %d to the output file %s\n")), pad->vol, outfptr);
	for (i = 0; i <= 2; i++) {
	  if (begerrflag) {
	    break;
	  }
	  switch (i) {
	  case 0:
	    begerrflag = writelabel(outfiletype);
	    break;
	  case 1:
	    begerrflag = writevolhdr(outfiletype, n_uses);
	    break;
	  case 2:
	    begerrflag = writeidx(outfiletype);
	    break;
	  }
	}
	if (begerrflag) {
	  msg((catgets(nlmsg_fd,NL_SETN,25, "fbackup(3025): write error at the beginning of the volume\n")));
	  if (outfiletype == VDI_REMOTE) {
	    rmt_close(outfd);
	  }
	  else {
	    vdi_close(outfiletype, outfptr, &outfd, mttype);
	  }
	  oldoutfptr = outfptr;
	}
      }
    }  /* while outfd == closed */
} /* end newvol */



/***************************************************************************
    This function is called to write the (ANSI standard?) label to the output
    file.
    This was moved from writer2.c to fix a POTM (Phase of the Moon) bug
    on 300 8.0.  I don't know why this works but it does. -- danm
***************************************************************************/
int
writelabel(filetype)
     int filetype;
{
  LABELTYPE label;
  int n;
  int size;
  char *type = "ANSII label";

  (void) strncpy(label.data,  "ANSII standard label not yet implemented",
		 sizeof(label.data));
  size = sizeof(LABELTYPE);
  
  switch (filetype) {
  case VDI_REMOTE:
    n = rmt_write(outfd, (char*)&label, size);
    if (n != size) {
      perror("fbackup(9999)");
      msg((catgets(nlmsg_fd,NL_SETN,122, "fbackup(3122): write error on a record in the %s\n")), type);
      return(TRUE);
    }

#ifdef DEBUG_WRTR
    fprintf(stderr, "Remote writelabel successful\n");
#endif

    if (!mtoper(outfd, MTWEOF, 1)) {
      msg((catgets(nlmsg_fd,NL_SETN,107, "fbackup(3107): write error on a EOF in the %s\n")), type);
      return(TRUE);
    }

#ifdef DEBUG_WRTR
    fprintf(stderr, "Remote writelabel EOF successful\n");
#endif

    break;
  case VDI_MAGTAPE:
    n = vdi_write(filetype, outfd, (char*)&label, size);
    if (n != size) {
      perror("fbackup(9999)");
      msg((catgets(nlmsg_fd,NL_SETN,108, "fbackup(3108): write error on a record in the %s\n")), type);
      return(TRUE);
    }
/* test comment for compiler problem */
#ifdef DEBUG_WRTR
    fprintf(stderr, "MAG writelabel successful\n");
#endif

    if ((vdi_ops(filetype, outfd, VDIOP_WTM1, 1)) != 0) {
      msg((catgets(nlmsg_fd,NL_SETN,109, "fbackup(3109): write error on an EOF in the %s\n")), type);
      return(TRUE);
    }
/* test comment for compiler problem */

#ifdef DEBUG_WRTR
    fprintf(stderr, "MAG writelabel EOF successful\n");
#endif

    break;
  case VDI_DAT:
  case VDI_DAT_FS:
    n = vdi_write(filetype, outfd, (char*)&label, size);
    if (n != size) {
      perror("fbackup(9999)");
      msg((catgets(nlmsg_fd,NL_SETN,124, "fbackup(3124): write error on a record in the %s\n")), type);
      return(TRUE);
    }
/*
 * for 8MM devices check if the format allows setmarks. The 8MM tape driver
 * allows the determination of the format only after a read/write to the media
 * hence the re-identification is done here.
 */
    if (is8mm && !setmarks_allowed_8mm(outfd)) {
	/*
	 * format does not support setmarks.
	 */
        outfiletype = VDI_MAGTAPE; /* reset global outfiletype */
        filetype = VDI_MAGTAPE;    /* reset local filetype */

    if ((vdi_ops(filetype, outfd, VDIOP_WTM1, 1)) != 0) {
      msg((catgets(nlmsg_fd,NL_SETN,109, "fbackup(3109): write error on an EOF in the %s\n")), type);
      return(TRUE);
    }
    break;
    }

#ifdef DEBUG_WRTR
fprintf(stderr, "DAT FS writelabel successful\n");
#endif

    if ((vdi_ops(filetype, outfd, VDIOP_WTM2, 1)) != 0) {

#ifdef DEBUG_WRTR
fprintf(stderr, "DAT FS write FSM problem vdi_errno = %d\n", vdi_errno);
#endif

      msg((catgets(nlmsg_fd,NL_SETN,125, "fbackup(3125): write error on an FSM in the %s\n")), type);

#ifdef DEBUG_WRTR
      fprintf(stderr, "errno from vdi_ops call is %d", vdi_errno);
#endif      

      return(TRUE);
    }

#ifdef DEBUG_WRTR
    fprintf(stderr, "DAT FS writelabel FSM successful\n");
#endif

    break;
  case VDI_MO:
  case VDI_DISK:
    n = vdi_write(filetype, outfd, (char*)&label, size);
    if (n != size) {
      perror("fbackup(9999)");
      msg((catgets(nlmsg_fd,NL_SETN,126, "fbackup(3126): write error on a record in the %s\n")), type);
      return(TRUE);
    }

#ifdef DEBUG_WRTR
    fprintf(stderr, "MO writelabel successful\n");
#endif

    break;
  case VDI_REGFILE:
  case VDI_STDOUT:
    n = vdi_write(filetype, outfd, (char*)&label, size);
    if (n != size) {
      perror("fbackup(9999)");
      if ((outfd == 1) && ((vdi_errno == EPIPE) || (vdi_errno == ENOSPC)))
        wrtrabort();
      msg((catgets(nlmsg_fd,NL_SETN,110, "fbackup(3110): write error on a record in the %s\n")), type);
      return(TRUE);
    }

#ifdef DEBUG_WRTR
    fprintf(stderr, "FILE writelabel successful\n");
#endif

    break;
  default:

#ifdef DEBUG_WRTR
    fprintf(stderr, "writelabel NOT DONE\n");
#endif

    break;
  } /* end switch */

    return(FALSE);
    
}  /* end writelabel */



/* The following code is added for 3480 support. */

/* load_next_reel() function attempts to load the next cartridge.
 * It returns 1 after sucessful completion and 0 on failure.
 * I assumes current operation is writing to a 3480 device.
 */
load_next_reel(mt)
int mt;
{
	int f;
	struct mtop mt_com;

	mt_com.mt_op = MTWEOF;
	mt_com.mt_count = 2;
	if (ioctl(mt, MTIOCTOP, &mt_com) < 0) {
		msg(catgets(nlmsg_fd,NL_SETN,315, "fbackup(3315): ioctl to write filemarks failed (%d). aborting...\n"),errno);
		wrtrabort();
	}

	mt_com.mt_op = MTOFFL;
	mt_com.mt_count = 1;
        if ( ioctl(mt, MTIOCTOP, &mt_com) < 0 ) {
		msg(catgets(nlmsg_fd,NL_SETN,311, "fbackup(3311): ioctl to offline device failed.\n"));
		wrtrabort();
	}  
/*
 * The previous ioctl may put the 3480 device offline in the following cases:
 * 1. End of Magazine condition in auto mode.
 * 2. Device in manual/system mode.
 */

/* Check if cartridge is loaded to the drive. */
	if (DetectTape(mt)) {
		msg(catgets(nlmsg_fd,NL_SETN,312,"fbackup(3312): auto loaded next media.\n"));
		return(1);
	} else {
		msg(catgets(nlmsg_fd,NL_SETN,313,"fbackup(3313): unable to auto load next media.\n"));
		return(0);
	}
}

/* This function checks if a cartridge is loaded into the 3480 drive. */
DetectTape(mt)
{
	struct mtget mtget;

/*	NOTE:  The code below is commented out due to the 3480 driver
 *	returning an error if the device is offile instead of allowing
 *	ioctl to return an OFFLINE indication. (DLM 7/21/93)
 *
 *	if ( ioctl(mt, MTIOCGET, &mtget) < 0 ) {
 *		msg(catgets(nlmsg_fd,NL_SETN,314, "fbackup(3314): ioctl to determine device online failed.\n"));
 *		wrtrabort();
 *	}
 */

	if ((ioctl(mt, MTIOCGET, &mtget) < 0) ||
	    (!GMT_ONLINE(mtget.mt_gstat))) return(0);
	else return(1);
}
