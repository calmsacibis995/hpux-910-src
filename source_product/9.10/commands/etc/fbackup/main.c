/* @(#) $Revision: 70.2 $ */

/***************************************************************************
****************************************************************************

	main.c

    For an explanation of the overall macroscopic view of how fbackup works,
    see the file README, in the same directory where the fbackup sources
    are kept.  There is a file called tape_fmt which provides a description
    of what fbackup output files look like; it forms the agreement between
    fbackup and frecover.  This document is also very helpful for
    understanding how checkpointing is implemented.

****************************************************************************
***************************************************************************/

#include "head.h"

#define LANG_DFLT "C"

#ifdef NLS
#include <locale.h>
#include <stdlib.h>
#define NL_SETN 1	/* message set number */
nl_catd nlmsg_fd;
#else 
#define catgets(i, sn,mn,s) (s)
#endif NLS

void beforefork(), parse_cmdline(), inithdrtrl(), initpwgr(), make_flist(),
	make_tables();
char *getcwd(), *shmat(), *shmaddr;

char	chgvol_file[MAXPATHLEN],
	err_file[MAXPATHLEN],
	*startdir;		/* directory where we started */

int	n_outfiles = 0,
	nrdrs = NRDRS_DFLT,
	blksperrec = BLKSPERREC_DFLT,
	nrecs = NRECS_DFLT,
	maxretries = MAXRETRIES_DFLT,
	retrylim = RETRYLIM_DFLT,
	ckptfreq = CKPTFREQ_DFLT,
	fsmfreq = FSMFREQ_DFLT,
	maxvoluses = MAXVOLUSES_DFLT,
	idxsize = 0,
	yflag = FALSE,
	uflag = FALSE,
	vflag = FALSE,
	Rflag = FALSE,
	Hflag = FALSE,
	nflag = FALSE,
	Zflag = FALSE,
        sflag = FALSE,
	savestateflag = FALSE,
        shmflg,
	flistbuiltflag = FALSE;

struct fn_list *temp;
struct fn_list *outfile_head;

int	shmemid,
	nblks,
	recsize,
	segsize,
	segindex,
	rdrpid[MAXRDRS],
	wrtrpid,
	filenum=1, 
	nfiles=0,
	pipefd[2];		/* used for sinding indices to writer proc */

void	rdrsignal(),		/* the 5 signal handling routines */
	wrtrsignal(),
	intsignal(),
	cldsignal(),
	trmsignal();

int	ignsigs [] = {		/* All these signals will be ignored */
	    SIGHUP, SIGQUIT, SIGILL, SIGTRAP, SIGIOT, SIGEMT, SIGFPE,
	    SIGBUS, SIGSEGV, SIGSYS, SIGPIPE, SIGALRM, SIGPWR, SIGVTALRM,
	    SIGPROF, SIGIO, SIGWINDOW, SIGURG,
				/* but these will be assigned handlers later */
	    SIGUSR1, SIGUSR2, SIGCLD, SIGTERM
	};

struct sigvec rdrsig_vec={rdrsignal, 0, 0}, wrtrsig_vec={wrtrsignal, 0, 0},
	      intsig_vec={intsignal, 0, 0}, cldsig_vec ={cldsignal,  0, 0},
	      ignsig_vec={SIG_IGN,   0, 0}, trmsig_vec ={trmsignal,  0, 0};
#ifdef PFA
    struct sigvec dflsig_vec={SIG_DFL, 0, 0};
#endif PFA

char	*segment,
	*blkstatus,
	*envlang = (char*)NULL,
	*lang;

off_t	*datasizes;

PADTYPE *pad;
RDRTYPE *rdr;
RECTYPE *rec;
PENDFTYPE pend_file[MAXRDRS+1],
	  *pend_ptr;

extern int semaid;

#ifdef AUDIT
extern int glob_level;			/* from parse.c */
#endif


#ifdef DEBUG_T
time_t debug_start_time;
time_t debug_end_time;
time_t debug_tape_time;
time_t debug_interval;
time_t debug_time;
#endif DEBUG_T


/***************************************************************************
    This is the main function of fbackup.
***************************************************************************/

main(argc, argv)
int argc;
char **argv;
{
    long starttime, endtime, time();

#ifdef NLS || NLS16			/* initialize to the current locale */
    if (!setlocale(LC_ALL, "")) {
	fputs(_errlocale("fbackup(1001)"), stderr);
	putenv("LANG=C");
	putenv("LC_COLLATE=C");
    }
    nlmsg_fd = catopen("fbackup", 0);
    if ((envlang=getenv("LC_COLLATE")) == (char*)NULL) {
	if ((envlang=getenv("LANG")) == (char*)NULL) {
	    lang = (char*)mymalloc((unsigned)strlen(LANG_DFLT)+1);
            if (lang == (char*)NULL)
 	        errorcleanx(catgets(nlmsg_fd,NL_SETN,1, "fbackup(1001): out of virtual memory\n"));
    	    (void) strcpy(lang, LANG_DFLT);
        }
	else {
	    lang = envlang;
	}
    } else {
	lang = envlang;
    }
#else NLS || NLS16
    lang = (char*)mymalloc((unsigned)strlen(LANG_DFLT)+1);
    if (lang == (char*)NULL)
	errorcleanx(catgets(nlmsg_fd,NL_SETN,3, "fbackup(1003): out of virtual memory\n"));
    (void) strcpy(lang, LANG_DFLT);
#endif NLS || NLS16

    (void) time(&starttime);

#ifdef DEBUG_T
    debug_start_time = starttime;
#endif DEBUG_T

    parse_cmdline(argc, argv);

#ifdef AUDIT
    /* turn off auditing here for self auditing */
    errno = 0;
    audswitch(AUD_SUSPEND);
#endif

    msg((catgets(nlmsg_fd,NL_SETN,4, "fbackup(1004): session begins on %s\n")), myctime(&starttime));
    setup(starttime);

#ifdef DEBUG_T
    (void) time(&debug_tape_time);
    debug_interval = debug_tape_time - debug_start_time;
    msg("Debug Time (sec): initialization = %ld\n", debug_interval);
#endif DEBUG_T

    work();

#ifdef DEBUG_T
    (void) time(&debug_end_time);
    debug_interval = debug_end_time - debug_tape_time;
    msg("Debug Time (sec): tape writing = %ld\n", debug_interval);
    debug_interval = debug_end_time - debug_start_time;
    msg("Debug Time (sec): tota time = %ld\n", debug_interval);
#endif DEBUG_T


    (void) time(&endtime);

	    /* To ensure that the last few files don't have their ctimes set to
	       the same value as endtime, it is incremented.  This line should
	       go away when ctimes can be reset to their real (original) values.
	    */
    endtime++;

    if (!savestateflag) {
	onlineidx();
	volheader_print();
	putdates(endtime);
    }

    if (vflag)
	msg((catgets(nlmsg_fd,NL_SETN,5, "fbackup(1005): run time: %d seconds\n")), endtime-starttime);

    cleanup();
    if (!savestateflag)
	return(NORMAL_EXIT);
    else
	return(RESTART_EXIT);
}






/***************************************************************************
    This function is called by the main process to initialize everything.
    It ignores all signals except the ones which are used by the program,
    and sets up handlers for those.  Routines are called to make the
    (both the file and directory) exclude tables, and then use these to
    build the flist.  It gets the shared memory segment (used by all
    processes) and semaphore (used by the main and writer processes).
    It also initializes the values in shared memory which require it.
    The last thing this function does is to fork first the reader process(es)
    and then the ONE writer process.  After the forks, reader and writer
    processes are exec-ed.  The main (parent) process does a bit more init-
    ialization, and then waits for the writer process to signal that it's
    ready before returning.
***************************************************************************/
setup(starttime)
long starttime;
{
    ushort semarray[1];
    int i, reader, warg;
    char shmemidstr[20], readerstr[20], segsizestr[20], shmflgstr[20], shmaddrstr[20];
    char *wrtrargv[MAXOUTFILES+8];
    int size;
    long savemask;
    int init_findpath();

    init_dtable();

#ifdef DEBUG_T
    (void) time(&debug_time);
    msg("Debug Time: start of setup = %ld\n", debug_time);
#endif DEBUG_T

    size = sizeof(ignsigs)/sizeof(int);
    for (i=0; i<size; i++)
	(void) sigvector(ignsigs[i], &ignsig_vec, (struct sigvec *)0);

    (void) sigvector(SIGTERM, &trmsig_vec,  (struct sigvec *)0);

    parse_config();
    if (nrdrs > MAXRDRS)
	errorx((catgets(nlmsg_fd,NL_SETN,6, "fbackup(1006): too many readers specified\n")));
    if (nrecs > MAXRECS)
	errorx((catgets(nlmsg_fd,NL_SETN,7, "fbackup(1007): too many memory records specified\n")));
    nblks =      nrecs * blksperrec;
    recsize =    blksperrec * BLOCKSIZE;
    segsize =    nrecs * recsize;

    if ((startdir=getcwd((char *)NULL, MAXPATHLEN+2)) == NULL)
	errorx((catgets(nlmsg_fd,NL_SETN,8, "fbackup(1008): could not get the current working directory\n")));

#if defined(DUX) || defined(DISKLESS)
    /*
     * Initialize findpath() routine (used for CDFs).
     *
     * Note: init_findpath() requires the "startdir" variable.
     */

    init_findpath();
#endif /* DUX || DISKLESS */

    (void) sigvector(SIGUSR2, &rdrsig_vec,  (struct sigvec *)0);
    (void) sigvector(SIGUSR1, &wrtrsig_vec, (struct sigvec *)0);
    (void) sigvector(SIGCLD,  &cldsig_vec,  (struct sigvec *)0);
	/* ignore SIGINT when SAM special option is used */
	if (Zflag)
		(void) sigvector(SIGINT, &ignsig_vec, (struct sigvec *)0);
	else
		(void) sigvector(SIGINT,  &intsig_vec,  (struct sigvec *)0);

    size = segsize + sizeof(PADTYPE) + nblks*sizeof(long) + nblks*sizeof(char);
    if ((shmemid=shmget(IPC_PRIVATE, size, PROTECT)) == -1)
	errorx((catgets(nlmsg_fd,NL_SETN,9, "fbackup(1009): cannnot get the specified shared memory segment\n")));
    (void) sprintf(shmemidstr, "%d", shmemid);
    (void) sprintf(segsizestr, "%d", segsize);

    if ((semaid=semget(IPC_PRIVATE, 1, PROTECT)) == -1)
	errorcleanx((catgets(nlmsg_fd,NL_SETN,10, "fbackup(1010): semget failed for the semaphore\n")));

#ifdef hp9000s300
#define SHMOFFSET 64000000
#define S310OFFSET      12000000
    shmaddr = (char *)SHMOFFSET;
    shmflg = SHM_RND;
    if ((int)((segment=shmat(shmemid, shmaddr, shmflg))) == -1) {
	shmaddr = (char *)S310OFFSET;
	segment=shmat(shmemid, shmaddr, shmflg);
    }
#else
    shmaddr = 0;
    shmflg = 0;
    segment=shmat(shmemid, shmaddr, shmflg);
#endif

    (void) sprintf(shmflgstr, "%d", shmflg);
    (void) sprintf(shmaddrstr, "%d", shmaddr);

    if ((int)segment == -1)
	errorcleanx((catgets(nlmsg_fd,NL_SETN,11, "fbackup(1011): shmat failed for the shared memory segment\n")));

    pad = (PADTYPE*)((unsigned)segment + segsize);
    datasizes = (off_t*)((unsigned)pad + sizeof(PADTYPE));
    blkstatus = (char*)((unsigned)datasizes + nblks*sizeof(off_t));

    rdr = pad->rdr;
    rec = pad->rec;
    pad->semaid = semaid;
    pad->nrecs = nrecs;
    pad->recsize = recsize;
    pad->ppid = getpid();
    pad->begtime = starttime;

    pad->maxretries = maxretries;
    pad->retrylim = retrylim;
    pad->ckptfreq = ckptfreq;
    pad->fsmfreq = fsmfreq;
    pad->maxvoluses = maxvoluses;
    pad->yflag = yflag;
    pad->vflag = vflag;
    pad->idxcmd = NOACTION;
    (void) strcpy(pad->envlang, envlang);
    (void) strcpy(pad->chgvol_file, chgvol_file);
    (void) strcpy(pad->err_file, err_file);

    if (!Rflag) {		/* not restarting */
	pad->startfno = 1;
	pad->vol = FIRSTVOL;
	pad->pid = pad->ppid;
    } else if (restoresession(&filenum)) {	    /* restarting (-R option) */
	msg((catgets(nlmsg_fd,NL_SETN,12, "fbackup(1012): successfully restarted the session\n")));
    } else {
	errorcleanx((catgets(nlmsg_fd,NL_SETN,13, "fbackup(1013): unable to restart the session\n")));
    }

#ifdef DEBUG || DEBUG_T
    (void) fprintf(stderr,
	   "nrdrs=%d blksperrec=%d nrecs=%d nblks=%d recsize=%d segsize=%d\n",
	   nrdrs, blksperrec, nrecs, nblks, recsize, segsize);
    (void) fflush(stderr);
#endif DEBUG

    pad->wrtrstatus = BUSY;
    beforefork();
    for (reader=0; reader<nrdrs; reader++) {
	rdr[reader].status = BUSY;
    }

		/* fork and then exec the reader process(es) */
    for (reader=0; reader<nrdrs; reader++) {
	(void) sprintf(readerstr, "%d", reader);
#ifdef PFA
	pfa_dump();
#endif PFA
	if ((rdrpid[reader]=fork()) < 0)		/* fork error */
	    errorcleanx((catgets(nlmsg_fd,NL_SETN,14, "fbackup(1014): fork failed for reader %d\n")), reader);
	if (rdrpid[reader] == 0) {		/* reader process */
	    (void) execl(READER,READER,shmemidstr,readerstr,segsizestr,lang,shmaddrstr, shmflgstr,0);
	    msg((catgets(nlmsg_fd,NL_SETN,15, "fbackup(1015): reached after reader execl\n")));
	    (void) exit(ERROR_EXIT);
	}
    }

    (void) pipe(pipefd);	/* setup pipe from main to writer */

				/* setup semaphore for main and writer */
    semarray[0]=1;
    if (semctl(semaid, 0, SETALL, semarray) == -1)
	errorcleanx((catgets(nlmsg_fd,NL_SETN,16, "fbackup(1016): semctl failed\n")));


		/* fork and then exec the writer process */
#ifdef PFA
    pfa_dump();
#endif PFA
    if ((wrtrpid=fork()) < 0)				/* fork error */
	errorcleanx((catgets(nlmsg_fd,NL_SETN,17, "fbackup(1017): fork failed for writer\n")));
    if (wrtrpid == 0) {					/* writer process */
	(void) close(pipefd[1]);
	wrtrargv[0] = WRITER;
	wrtrargv[1] = shmemidstr;
	wrtrargv[2] = segsizestr;
	(void) sprintf(readerstr, "%d", pipefd[0]);/* nothing to do with rdrs */
	wrtrargv[3] = readerstr;		   /* just using this string */
	wrtrargv[4] = lang;
	wrtrargv[5] = shmaddrstr;
	wrtrargv[6] = shmflgstr;
	temp = outfile_head;
	for (warg=0; warg<n_outfiles; warg++) {
#ifdef DEBUG
fprintf(stderr, "fbackup: setup() wrtr exec: temp name = %s\n", temp->name);
#endif
	    wrtrargv[warg+7] = temp->name;
	    temp = temp->next;
	}
	wrtrargv[n_outfiles+7] = (char *)0;
#ifdef DEBUG
for (warg = 0; warg < (n_outfiles+7); warg++) {
fprintf(stderr, "fbackup: setup() wrtr exec: wrtrargv[%d] = %s\n", warg, wrtrargv[warg]);
}
#endif
	(void) execv(WRITER, wrtrargv);
	msg((catgets(nlmsg_fd,NL_SETN,18, "fbackup(1018): reached after writer execl\n")));
	(void) exit(ERROR_EXIT);
    }
							/* parent process */
    (void) close(pipefd[0]);

    if (!Rflag) {		/* if this is not a "restart" (-R) */

	make_tables();

#ifdef DEBUG_T
    (void) time(&debug_time);
    msg("Debug Time: start of make flist = %ld\n", debug_time);
#endif DEBUG_T

	make_flist();

#ifdef DEBUG_T
    (void) time(&debug_end_time);
    debug_interval = debug_end_time - debug_time;
    msg("Debug Time: for making flist = %ld\n", debug_interval);
#endif DEBUG_T

	if (nfiles == 0) {
	    msg((catgets(nlmsg_fd,NL_SETN,19, "fbackup(1019): warning: none of the specified files needed to be backed up\n")));
	    if (!uflag)
		errorcleanx((catgets(nlmsg_fd,NL_SETN,20, "fbackup(1020): Unexpected exit\n")));
	}
    } else if (restoreflist(filenum)) {	    /* restarting (-R option) */
	msg((catgets(nlmsg_fd,NL_SETN,21, "fbackup(1021): recovered the file list\n")));
    } else {
	errorcleanx((catgets(nlmsg_fd,NL_SETN,22, "fbackup(1022): unable to recover the file list\n")));
    }

    flistbuiltflag = TRUE;
    pad->nfiles = nfiles;
    pad->idxsize = idxsize;

#ifdef DEBUG
    msg("Debug Data: number of files = %d\n", nfiles);
    msg("Debug Data: size of index = %d\n", idxsize);
#endif 

    inithdrtrl();
    initpwgr();

    savemask = sigblock(WRTRSIG);
    while (pad->wrtrstatus != READY) {	/* wait for writer to get ready */
	(void) sigpause(savemask);
    }
    (void) sigsetmask(savemask);
    waitforrdrs();		/* wait for all the readers to get ready */
}






/***************************************************************************
    This function is called both at initialization time and when a "resume"
    must be done (eg, after the writer process discovers a file was active
    while it was being backed up or when there is a write error to a tape).
    It sets the status of all the shared memory "records" to FREE,
    sets the status of each entry in the pending file table to FREE, and
    sets the amount of available "ring" shared memory to the size of
    the ring (no ring memory currently in use).
***************************************************************************/
void
beforefork()
{
    int record, pend_fidx;

    segindex = 0;
    for (record=0; record<nrecs; record++) {
	rec[record].status = FREE;
    }
    for (pend_fidx=0; pend_fidx<=nrdrs; pend_fidx++) {
	pend_file[pend_fidx].status = FREE;
    }
    pad->avail = segsize;
}






/***************************************************************************
    This is the part of the main process that sequences through the files
    in the flist and calls a function to copy their information into shared
    memory.  The reasons for all the apparently unneccesary complications
    in this function have to do with: active files and write errors causing
    a "resume" to occur, an interrupt causing a premature exit, or a timing
    problem.  It is possible for the writer process to discover an active
    file either before or after the main process thinks it is finished;
    hence, there must be a way to get back to the "resume" label even after
    all the input files have been copied into shared memory.  The only
    remaining task for this function is to tell the writer process that it
    is done and then handle the saving of state for this session in case of
    a premature exit due to an interrupt.
***************************************************************************/
work()
{
    int active_wrtr, record;
    long savemask;

#ifdef DEBUG || DEBUG_T
    msg("%d total files\n", nfiles);
#endif DEBUG

resume:			/* start/resume at the start of file no filenum */

#ifdef DEBUG
    msg("(RE)SUMING at fno=%d\n", filenum);
#endif DEBUG

    while (filenum <= nfiles) {
	if (savestateflag)		/* if an interrupt has occurred */
	    break;
	dofile(filenum);
	if (pad->wrtrstatus == RESET) {	/* an active file or a tape write err */
	    filenum = reset();
	    goto resume;
	}
	filenum++;
#ifdef DEBUG_T
	if ((filenum % 100) == 0) {
	  (void) time(&debug_time);
	  debug_interval = debug_time - debug_start_time;
	  msg("Debug Time: to do %d files = %ld\n", filenum, debug_interval);
	}
#endif DEBUG_T
    }

    waitforrdrs();			/* wait for all readers to finish */

    active_wrtr = TRUE;
    while (active_wrtr) {		/* wait for the writer to finish */
	if (pad->wrtrstatus == RESET) {
	    filenum = reset();		/* if there has been an active file */
	    goto resume;		/* or a tape write error, resume */
	}		
	active_wrtr = FALSE;		/* for any active "ring" records: */
	for (record=0; record<nrecs; record++) {
	    switch (rec[record].status) {
		case PARTIAL:			/* handle the last partial */
		    active_wrtr = TRUE;		/* record, if there is one */
		    if (rec[record].count == 0) {
			rec[record].count = segindex%recsize;
			rec[record].status = COMPLETE;
			(void) kill(wrtrpid, SIGUSR1);
		    }
		    break;
		case COMPLETE:
		    active_wrtr = TRUE;
		    break;
		case FREE:
		    break;
#ifdef DEBUG
		default: msg("error in work, hit default\n"); break;
#endif DEBUG
	    }
	}
    }
    if (pad->wrtrstatus == RESET) {	/* if there was an active file or */
	filenum = reset();		/* a tape write error on one of the */
	goto resume;			/* last few records */
    }
    pad->wrtrstatus = DONE;		/* tell the writer to finish up */
    (void) kill(wrtrpid, SIGUSR1);

    savemask = sigblock(WRTRSIG);
    while (pad->wrtrstatus != EXIT)	/* wait for it to finish up */
	(void) sigpause(savemask);
    (void) sigsetmask(savemask);

		    /* If the main process has been an interrupted, the
		       savestateflag will be set.  If this is the case, the
		       user is prompted to see if s/he wants to be able to
		       restart later.  If so, savestate is called.   If not,
		       a call to errorcleanx cleans up and exits.
		    */
    if (savestateflag) {
	if (query((catgets(nlmsg_fd,NL_SETN,23, "fbackup(1023): will you restart this session later?\n"))))
	    if (savestate(filenum))
		msg((catgets(nlmsg_fd,NL_SETN,24, "fbackup(1024): this session's state has been saved\n")));
	    else
		errorcleanx((catgets(nlmsg_fd,NL_SETN,25, "fbackup(1025): did not save this session's state\n")));
	else
	    errorcleanx((catgets(nlmsg_fd,NL_SETN,26, "fbackup(1026): exiting without saving this session's state\n")));
    }
}  /* end work */






/***************************************************************************
    This function waits until the status of ALL the reader process(es) is
    READY.  It is called whenever it necessary for all reader activity to
    come to a stop before proceeding.
***************************************************************************/
waitforrdrs()
{
    int active_rdr, reader;
    long savemask;

    active_rdr = TRUE;
    while (active_rdr) {
	active_rdr = FALSE;
	savemask = sigblock(RDRSIG);
	for (reader=0; reader<nrdrs; reader++) {
	    if (rdr[reader].status != READY) {
		active_rdr = TRUE;
		break;
	    }
	}
	if (active_rdr) {
	    (void) sigpause(savemask);
	}
	(void) sigsetmask(savemask);
    }
}






/***************************************************************************
    This function is called when a reader process is needed to read part (or
    all) of a regular file into shared memory.  (Note: directories, special
    files, and symbolic links are all handled by the main process, so it's
    not necessary to call getrdr for them.)   This function keeps looking at
    all the reader(s) until it discovers one that is READY.  The number
    (where readers are numbered from 0 to n-1, for n readers) of the first
    one that is READY is returned by this function.
    An attempt to re-assign the same reader that was last assigned is made,
    using the static variable, cur_rdr.
***************************************************************************/
int
getrdr()
{
    int reader;
    static int cur_rdr = 0;
    long savemask;

    while (1) {
	reader = cur_rdr;
	savemask = sigblock(RDRSIG);
	do {
	    if (rdr[reader].status == READY) {
		cur_rdr = reader;
		(void) sigsetmask(savemask);
		return(reader);
	    } else {
		reader = (reader+1)%nrdrs;
	    }
	} while (reader != cur_rdr);
#ifdef DEBUG_RDR
	(void) putc('r', stderr); (void) fflush(stderr);
	for (reader=0; reader < nrdrs; reader++) {
	    printf("rdr[%d].status = %d\n", reader, rdr[reader].status);
	    fflush(stdout);
	}
#endif /* DEBUG_RDR */
	(void) sigpause(savemask);
	(void) sigsetmask(savemask);
    }
}






/***************************************************************************
    This function is called to update the status of "ring" shared memory
    "records".  Whenever a region of this memory is allocated by the main
    process, this function is called to keep track of what that region is
    being used for.  There are two priciple structures that markempty updates.
    First, the block status array in shared memory (blkstatus).  There is
    one byte for each block of shared memory in the "ring".  Each time
    markempty is called, it is passed a type argument.  This type may be
    one of H, h, d, or T (for first header block, non-first header block,
    data block, or trailer block, respectively).  For each block in the 
    region specified, blkstatus for that block is set to this type value.
    The second structure updated is "rec" section of shared memory.
    Markempty is called to mark the region specified as being empty, but
    it will be filled by some process.  The process which will fill this
    particular region may be either one of the reader process(es) or the
    main process, but that is of no concern here.
***************************************************************************/
void
markempty(index, nbytes, type, datasize)
int index, nbytes;
char type;
off_t datasize;
{
    int x, y, end, b, block, r, record;
    long savemask;

    x = index/BLOCKSIZE;
    y = x + nbytes/BLOCKSIZE;
    datasizes[x%nblks] = datasize;
    for (b=x; b<y; b++) {
	block = b%nblks;
	blkstatus[block] = type;
    }
    x = index/recsize;
    end = index + nbytes - 1;
    y = end/recsize;

    savemask = sigblock(RDRSIG|WRTRSIG);
    for (r=x; r<=y; r++) {
	record = r%nrecs;
	switch (rec[record].status) {
	    case FREE:
		rec[record].count = 1;
		if (end >= (r+1)*recsize - 1) /* takes last of this rec? */
		    rec[record].status = ALLOCATED;	/* yes */
		else
		    rec[record].status = PARTIAL;	/* no */
		break;
	    case PARTIAL:
		rec[record].count++;
		if (end >= (r+1)*recsize - 1) /* takes last of this rec? */
		    rec[record].status = ALLOCATED;	/* yes */
		break;
#ifdef DEBUG
	    case ALLOCATED: msg("error in markempty, hit ALLOCATED\n");break;
	    case COMPLETE: msg("error in markempty, hit COMPLETE\n"); break;
	    default: msg("error in markempty, hit DEFAULT\n"); break;
#endif DEBUG
	}
    }
    (void) sigsetmask(savemask);
}






/***************************************************************************
    This function is the mate to markempty, above.  Whenever a region of
    "ring" shared memory has been filled (either by one of the reader
    process(es) or by the main process), markfull is called to update the
    status of the rec data structure.

    The two functions markempty and markfull are responsible for the status
    of records making the transitions from FREE, to PARTIAL (sometimes
    omitted), to ALLOCATED, to COMPLETE.  The writer process accepts COMPLETE
    records, writes them, and set the status back to FREE, and the cycle
    repeats.
***************************************************************************/
void
markfull(index, nbytes)
int index, nbytes;
{
    int record, a, b, r, end;
    long savemask;

    if (pad->wrtrstatus != READY)
	return;
    a = index/recsize;
    end = index + nbytes -1;
    b = end/recsize;

    savemask = sigblock(RDRSIG|WRTRSIG);
    for (r=a; r<=b; r++) {
	record = r%nrecs;
	switch (rec[record].status) {
#ifdef DEBUG
	    case FREE: msg("error in markfull, hit FREE\n");
		break;
#endif DEBUG
	    case PARTIAL:
		rec[record].count--;
		break;
	    case ALLOCATED:
		rec[record].count--;
		if (rec[record].count == 0) {
		    rec[record].count = recsize;
		    rec[record].status = COMPLETE;
		    (void) kill(wrtrpid, SIGUSR1);
		}
		break;
#ifdef DEBUG
	    case COMPLETE: msg("error in markfull, hit COMPLETE\n"); break;
	    default: msg("error in markfull, hit DEFAULT\n"); break;
#endif DEBUG
	}
    }
    (void) sigsetmask(savemask);
}






/***************************************************************************
    This function is called whenever a signal is received from one of the
    reader process(es).  Such signals only occur when a reader is finished
    doing whatever it had been assigned to do, and has set its status to
    DONE.  Hence, rdrsignal looks through all the readers to see which one(s)
    are DONE, and then takes the appropriate action for that reader.
    Two counters are maintained in the pending file table, chunks and
    chunksfull.  Chunks is the number of sections of a regular file is
    devided into while being read (note that this number is not necessarily
    the same, even for the same size file every time fbackup is run, but
    this does not matter - chunks is the number of sections the file have
    been assigned for THIS particular session).  Chunksfull is the number
    of these chunks that have been filled by reader process(es).
    Each time a chunk has been filled (ie, a reader process finishes that 
    chunk and signals the main process, rdrsignal is called.

    Note: this is an interrupt handler, and as such, may not be interrupted
    before it completes.  This code is intimately related to that in the
    function dofile, especially in the case of regular files (see
    case S_IFREG: in that function.
***************************************************************************/
void
rdrsignal()
{
    int reader;
    PENDFTYPE *p_ptr;

#ifdef DEBUG_RDR
fprintf(stderr, "fbackup: entered rdrsignal()\n");
#endif /* DEBUG */

    for (reader=0; reader<nrdrs; reader++) {
#ifdef DEBUG_RDR
fprintf(stderr, "fbackup: status of reader #%d: ", reader);
switch (rdr[reader].status) {
    case READY: fprintf(stderr, "READY\n"); break;
    case START: fprintf(stderr, "START\n"); break;
    case BUSY:  fprintf(stderr, "BUSY\n"); break;
    case DONE:  fprintf(stderr, "DONE\n"); break;
    default:    fprintf(stderr, "???\n"); break;
}
#endif /* DEBUG */

	if (rdr[reader].status == DONE) {
	    p_ptr = &pend_file[rdr[reader].pend_fidx];
	    p_ptr->chunksfull++;

			/* If the mtime and/or the ctime are changed, mark it
			   active.  If the trailer is already in shared memory,
			   update the status for this file.
		        */
	    if ((rdr[reader].mtime != p_ptr->mtime) ||
					(rdr[reader].ctime != p_ptr->ctime)) {
		p_ptr->modflag = TRUE;
		if (p_ptr->status == TRLINMEM) {
		    (void) strcpy(p_ptr->statusidx, ASCIIBAD);

/* Here, we have to recalculate the checksum for the trailer block,
 * because, the checksum would have changed as the status field in the
 * trailer block has been modified from ASCIIGOOD to ASCIIBAD and when the
 * trailer was built the checksum calculation was done with the assumption
 * that the status will be ASCIIGOOD.
 *
 * There doesn't seem to be direct way of accessing the beginning of the
 * trailer block. However, It can be indirectly done as follows:
 * p_ptr->statusidx is pointing to status field of the trailer block,
 * p_ptr->statusidx-INTSTRSIZE-sizeof(BLKID) should give the address of
 * beginning of the trailer - given,
 *
 * FTRLTYPE as defined in include<fbackup.h>:
 * i.e.,
 * typedef struct {
 *    BLKID com;
 *    char filenum[INTSTRSIZE];
 *    char status[2];
 * } FTRLTYPE;
 *
 */
                blk_cksum(p_ptr->statusidx-INTSTRSIZE-sizeof(BLKID));
              }
	    }

			/* If the trailer is already in shared memory, AND
			   chunks == chunksfull, then the file has been 
			   completely read in.  We may then check if it has
			   been modified (mtime or ctime has been changed).
			   If it isn't active, reset the i-node times.
			*/
	    if ((p_ptr->status == TRLINMEM) &&
					(p_ptr->chunks == p_ptr->chunksfull)) {
		if (!p_ptr->modflag)
		    resettimes(p_ptr);
		p_ptr->status = FREE;	/* FREE this pending file record */
	    }

			/* Mark this region of "ring" shared memory full, and
			   change this reader's status from DONE to READY.
			*/
	    markfull(rdr[reader].index, rndup(rdr[reader].nbytes));
	    rdr[reader].status = READY;
#ifdef DEBUG_RDR
fprintf(stderr, "fbackup: changed status of reader #%d to READY.\n", reader);
#endif /* DEBUG_RDR */
	}
    }
}






/***************************************************************************
    This function does very little real work.  It is mostly around for
    debugging purposes in case a problem needs to be tracked down.  
    It's two important tasks are: (1) to serve as a means of waking up the
    main process whenever it's sleeping when the writer process sends it a
    signal, and (2), it checks to see if the writer process wants to have
    the main process update the volume numbers in the index, or send it the
    index (via the pipe opened previously).
***************************************************************************/
void
wrtrsignal()
{
#ifdef DEBUG
    switch (pad->wrtrstatus) {
	case READY:
	    /* msg("<M>"); */
	break;
	case RESET:
	    msg("<M:RESET>");
	    msg("av=%d", pad->avail);
	break;
	case RESUME:
	    msg("<M:RESUME>");
	    msg("av=%d", pad->avail);
	break;
	case DONE:
	    msg("<M:DONE>");
	break;
	case EXIT:
	    msg("<M:EXIT>");
	break;
	case BUSY:
	    msg("<M:BUSY>");
	break;
	default:
	    msg("<M:default>");
	break;
    }
#endif DEBUG
    switch (pad->idxcmd) {
	case NOACTION:
	break;
	case UPDATEIDX:
	    updateidx();
	break;
	case SENDIDX:
	    sendidx();
	break;
#ifdef DEBUG
	default:
	    msg("main.c: writer signal handler, hit default\n");
	break;
#endif DEBUG
    }
}






/***************************************************************************
    This function only catches interrupt signals, and sets the savestate
    flag.
***************************************************************************/
void
intsignal()
{
    if (!flistbuiltflag)
	errorcleanx("");
    if (filenum < nfiles)
	savestateflag = TRUE;
}






/***************************************************************************
    If one of the children processes dies unexpectedly, this function 
    cleans up and exits in a (somewhat) graceful manner.
***************************************************************************/
void
cldsignal()
{
    int status, pid, i;

    /* Did one of our reader or writer processes die?
     * If so, exit gracefully with "errorcleanx",
     * otherwise continue.
     */

    pid = wait(&status);
    if ( pid != -1 ) {
	for (i=0; i < nrdrs; i++) {
	    if ( rdrpid[i] != 0 && pid == rdrpid[i] )
		errorcleanx((catgets(nlmsg_fd,NL_SETN,27, "fbackup(1027): Backup did not complete : Reader or Writer process exit \n")));
	}

	if ( wrtrpid != 0 && pid == wrtrpid )
	    errorcleanx((catgets(nlmsg_fd,NL_SETN,2, "fbackup(1002): Backup did not complete : Reader or Writer process exit \n")));
    }

#ifdef DEBUG
    if ( pid == rdrpid[0] || pid == rdrpid[1] || pid == wrtrpid ) {
	msg("fbackup: wait returned %d, high-byte: 0x%x, low-byte: 0x%x\n", pid, (status >> 8)&0xff, status&0xff);
	msg("reader pids: %d, %d; writer pid: %d\n", rdrpid[0], rdrpid[1], wrtrpid);
    }
#endif /* DEBUG */

    return;
}






/***************************************************************************
    This function cleans up and exits in a (somewhat) graceful manner
    whenever a SIGTERM signal is received.  It is provided for the sole
    purpose of terminating an fbackup session when something has gone awry.
***************************************************************************/
void
trmsignal()
{
    errorcleanx((catgets(nlmsg_fd,NL_SETN,28, "fbackup(1028): received a SIGTERM; cleaning up and exiting\n")));
}






/***************************************************************************
    This function is called whenever fbackup is terminated after the shared
    memory segment is obtained.  Cleanup first ignores SIGCLD, so the children
    won't interrupt this (main) process as they die.  Then the shared memory
    segment and semaphore are released.  All reader process(es) and then the
    writer process are killed.
***************************************************************************/
cleanup()
{
    int reader;
#ifdef DEBUG
msg("fbackup: called clean up routine\n");
#endif /* DEBUG */

					/* normal termination, ignore SIGCLD */
    (void) sigvector(SIGCLD, &ignsig_vec, (struct sigvec *)0);

    (void) shmctl(shmemid, IPC_RMID, (struct shmid_ds *)0);
    (void) semctl(semaid, 0, IPC_RMID, 0);

    for(reader=0; reader<nrdrs; reader++)
	if (rdrpid[reader] != 0) {
#ifdef DEBUG
	    msg("fbackup: trying to send SIGUSR1 to pid=%d\n", rdrpid[reader]);
#endif /* DEBUG */
	    (void) kill(rdrpid[reader], SIGUSR1);
	}

    if ( wrtrpid != 0 ) {
#ifdef DEBUG
	msg("fbackup: trying to send SIGUSR2 to pid=%d\n", wrtrpid);
#endif /* DEBUG */
	(void) kill(wrtrpid, SIGUSR2);
    }

    while (wait((int*)0) > 0)
	;
#ifdef PFA
    for (reader=1; reader<SIGARRAYSIZE; reader++) /* reader = signal number */
	(void) sigvector(reader, &dflsig_vec, (struct sigvec *)0);
    sleep(1);
#endif PFA

#ifdef AUDIT
    /* write audit record and turn auditing back on */
    {
	static char *inc = "Perform an Incremental Backup Online";
	static char *full = "Perform a Full Backup Online";
	char *audit_text;
	int len;
	struct self_audit_rec self;

	if(glob_level == 0) {
	    self.aud_head.ah_event = EN_SAMFBACKUP;
	    audit_text = full;
	}
	else {
	    self.aud_head.ah_event = EN_SAMIBACKUP;
	    audit_text = inc;
	}
	self.aud_head.ah_error = errno;
	self.aud_head.ah_len = len = strlen(audit_text);
	len = len >= (MAX_AUD_TEXT-1) ? MAX_AUD_TEXT-1 : len;
	strncpy(self.aud_body.text, audit_text, len);
	self.aud_body.text[len] = '\0';
	audwrite(&self);
	audswitch(AUD_RESUME);
    }
#endif
}






/***************************************************************************
    This function just prints its message argument and exits with an exit
    code set to indicate that an error occurred.
***************************************************************************/
/* VARARGS */
errorx(format, str)
char *format, *str;
{
    msg(format, str);		/* remember: copy "format" if any calls to */
    (void) exit(ERROR_EXIT);	/* catgets are added to this function, */
}				/* otherwise "format" will be overwritten */






/***************************************************************************
    This function prints its message argument, calls cleanup to keep from
    leaving shared memory, semaphores, or processes hanging about.  It then
    exits with an exit code set to indicate that an error occurred.
***************************************************************************/
/* VARARGS */
errorcleanx(format, str)
char *format, *str;
{
    msg(format, str);		/* remember: copy "format" if any calls to */
    cleanup();			/* catgets are added to this function, */
    (void) exit(ERROR_EXIT);	/* otherwise "format" will be overwritten */
}



/***************************************************************************
    This function calls xquery function with same argumet.  If xquery returns
    EOF, fbackup will abort.  Otherwise this function returns the value of
    xquery function.
***************************************************************************/

query(inputstr)
char *inputstr;
{
    int retvalue = xquery(inputstr);

    if (retvalue == EOF)
	errorcleanx((catgets(nlmsg_fd,NL_SETN,29, "fbackup(1029): Unexpected exit\n")));

    return (retvalue);
}


int statcall(path, buf)
     char *path;
     struct stat *buf;
{
  if (sflag) {
    return(stat(path, buf));
  }
  return(lstat(path, buf));
}  /* end statcall() */
