/* @(#) $Revision: 70.2 $ */

/***************************************************************************
****************************************************************************

	reader.c

    This file contains the code for the reader processes.  After initializing,
    it enters an infinite loop containing only a pause.  This is the quiescent
    state for a reader process.  Whenever it has nothing to do, it sleeps.
    Whenever the main process has something for this particular reader to do,
    it sends a signal (SIGUSR2) to that reader.  The reader then wakes up
    and does what it has been instructed to do.  The task to be done is
    determined by examining shared memory in the "pad" area.

****************************************************************************
***************************************************************************/

#include "head.h"

#ifdef NLS
#include <locale.h>
#define NL_SETN 2	/* message set number */
nl_catd nlmsg_fd;
#endif NLS

char *shmat();

void	rdrhandler(),
	endhandler();
int	reader,
	segsize;
char	*segment,
	*file;
PADTYPE *pad;
RDRTYPE *rdr;
struct sigvec rdr_vec = {rdrhandler,  0, 0},
	      end_vec = {endhandler, 0, 0},
	      int_vec = {SIG_IGN,     0, 0};
#ifdef PFA
    struct sigvec dflsig_vec={SIG_DFL, 0, 0};
#endif PFA





/***************************************************************************
    The main function for reader.
***************************************************************************/
main(argc, argv)  /* argv[0], [1],     [2],    [3]      [4]     [5]      [6] */
int argc;	  /* reader,  shmemid, reader, segsize, (lang), shmaddr, shmflg */
char **argv;
{
    int shmemid, shmflg;
    char *shmaddr;

#ifdef NLS
    setlocale(LC_ALL, argv[4]);
    nlmsg_fd = catopen("fbackup", 0);
#endif NLS

    argc = argc;				/* to keep lint happy */
    shmemid = atoi(argv[1]);			/* get the arguments */
    reader = atoi(argv[2]);
    segsize = atoi(argv[3]);
    shmaddr = atoi(argv[5]);
    shmflg = atoi(argv[6]);
    if ((int)((segment=shmat(shmemid, shmaddr, shmflg))) == -1) {
	msg((catgets(nlmsg_fd,NL_SETN,1, "fbackup(2001): reader %d: shmat failed for memory segment\n")), reader);
	rdrabort();
    }

    pad = (PADTYPE *) ((unsigned)segment + segsize);
    rdr = pad->rdr;

    file = rdr[reader].file;
    (void) sigvector(SIGUSR2, &rdr_vec, (struct sigvec *)0);/* handle SIGUSR2 */
    (void) sigvector(SIGUSR1, &end_vec,(struct sigvec *)0);/* handle SIGUSR1 */
    (void) sigvector(SIGINT,  &int_vec, (struct sigvec *)0);/* ignore SIGINT */

    rdr[reader].status = READY;			/* tell main that I'm ready */
    (void) kill(pad->ppid, SIGUSR2);
    while (1) {
	(void) sigpause(0L);	/* sleep whenever I have nothing to do */
    }
}






/***************************************************************************
    This function is called whenever this reader process receives a signal
    from the main process.  It obtains four bits of information from the
    main process (via shared memory): (1) which file it is to read, (2) at
    what offset it should start reading, (3) how many bytes to read into
    shared memory, and (4) the starting address of where in shared memory
    the data should go.
	Since shared memory is (conceptually) arranged as a "ring", it is
    possible that a read may span the place where the "ring" joins back on
    itself.  (Logically, byte 0 follows byte N-1, for an N byte "ring".)
    In such cases, rdrhandler does two reads, first one for the higher
    addresses in the ring (up to byte N-1), and a second one starting at
    byte 0.  The last task performed is the filling of any unused part of
    the last block of shared memory with zeros.
***************************************************************************/
void
rdrhandler()
{
    unsigned offset, nbytes;
    int index, firstread, secondread, n;
    int fd;
    struct stat statbuf;
    char *ptr;
    long lseek();
	
    if (rdr[reader].status == START) {
	rdr[reader].status = BUSY;
	offset = rdr[reader].offset;
	index = rdr[reader].index;
	nbytes = rdr[reader].nbytes;
	if ((fd = open(file, O_NONBLOCK|O_RDONLY)) < 0) { /* open for nonblock */
	    msg((catgets(nlmsg_fd,NL_SETN,2, "fbackup(2002): reader %d: unable to open file %s\n")), reader, file);
	    fillmem(nbytes, index);
	    return;
	}
	if (lseek(fd, (long)offset, 0) == -1) {
	    msg((catgets(nlmsg_fd,NL_SETN,3, "fbackup(2003): reader %d: unable to lseek file %s\n")), reader, file);
	    (void) close(fd);
	    fillmem(nbytes, index);
	    return;
	}
	firstread = min(nbytes, segsize-index);
	n = read(fd, &segment[index], (unsigned)firstread);
	if ((n == -1) && (errno == EAGAIN)) {    /* check for locking */
            msg((catgets(nlmsg_fd,NL_SETN,8, "fbackup(2008): reader %d: unable to read file %s,\n  file has been locked\n")), reader, file);
	    (void) close(fd);
            fillmem(nbytes, index);
            return;
        }
    

	if (n != firstread) {
	    perror("fbackup(9999)");
	    msg((catgets(nlmsg_fd,NL_SETN,4, "fbackup(2004): reader %d: 1st read error, ")), reader);
	    msg("firstread=%d, n=%d\n", firstread, n);
	    (void) close(fd);
	    fillmem(nbytes, index);
	    return;
	}
	if (firstread < nbytes) {
	    secondread = nbytes-firstread;
	    n = read(fd, segment, (unsigned)(secondread));
	    if ((n == -1) && (errno == EAGAIN)) {  /* Check for locking */ 
               msg((catgets(nlmsg_fd,NL_SETN,8, "fbackup(2008): reader %d: unable to read file %s,\n  file has been locked\n")), reader, file);
	       (void) close(fd);
               fillmem(nbytes, index);
               return;
            }
    
	    if (n != secondread) {
		perror("fbackup(9999)");
		msg((catgets(nlmsg_fd,NL_SETN,5, "fbackup(2005): reader %d: 2nd read error, ")), reader);
		msg("secondread=%d, n=%d\n", secondread, n);
		(void) close(fd);
		fillmem(nbytes, index);
		return;
	    }
	}
	(void) fstat(fd, &statbuf);
	rdr[reader].mtime = statbuf.st_mtime;
	rdr[reader].ctime = statbuf.st_ctime;
	
	(void) close(fd);

	firstread = (index+nbytes) % segsize;
	n = rndup(firstread) - firstread;
	ptr = &segment[firstread];
	while (n--)
	    *ptr++ = FILLCHAR;

	rdr[reader].status = DONE;
	(void) kill(pad->ppid, SIGUSR2);
#ifdef DEBUG
    } else {
	msg((catgets(nlmsg_fd,NL_SETN,6, "fbackup(2006): reader %d: received a sig with no work to do\n")), reader);
#endif DEBUG
    }
}






/***************************************************************************
    In the event that something goes wrong in the function rdrhandler (see
    above), the shared memory must be allowed to keep data which was left
    around from any previous file(s).  It is zeroed out.
***************************************************************************/
fillmem(nbytes, index)
unsigned nbytes;
int index;
{
    int firstchunk, secondchunk, n;
    char *ptr;

    rdr[reader].mtime = MODIFIED;
    firstchunk = min(nbytes, segsize-index);
    n = firstchunk;
    while (n--)
	segment[index++] = FILLCHAR;
    if (firstchunk < nbytes) {
	secondchunk = nbytes-firstchunk;
	n = secondchunk;
	index = 0;
	while (n--)
	    segment[index++] = FILLCHAR;
    }
    
    firstchunk = index % segsize;
    n = rndup(firstchunk) - firstchunk;
    ptr = &segment[firstchunk];
    while (n--)
	*ptr++ = FILLCHAR;

    rdr[reader].status = DONE;
    (void) kill(pad->ppid, SIGUSR2);
}






/***************************************************************************
    This function aborts this reader in the event that the shared memory
    segment cannot be attached (a major catastrophe!).  The main process
    discovers that this process has died by watching for a SIGCLD, then it
    kills any other reader processes (and the writer, if one exists) before
    exiting.
***************************************************************************/
rdrabort()
{
    msg((catgets(nlmsg_fd,NL_SETN,7, "fbackup(2007): reader %d: aborting\n")),reader);
    (void) exit(ERROR_EXIT);
}






void
endhandler()
{
#ifdef PFA
    (void) sigvector(SIGUSR1, &dflsig_vec, (struct sigvec *)0);
    (void) sigvector(SIGUSR2, &dflsig_vec, (struct sigvec *)0);
    (void) sigvector(SIGINT,  &dflsig_vec, (struct sigvec *)0);
    sleep(2*(reader+1));
#endif PFA
    exit(0);
}






/***************************************************************************
    This function is EXACTLY the same as the 'msg' function in util.c.
    Dupicating it here serves two functions: (1) it reduces the run-time
    size of the reader process(es) (none of the other functions in util.c
    are called by readers), and (2), it keeps the PFA results from being
    skewed significantly for the reader process(es).
***************************************************************************/
/* VARARGS */
msg(format, i1, i2, i3, i4)
char *format;
int i1, i2, i3, i4;
{
    (void) fprintf(stderr, format, i1, i2, i3, i4);
    (void) fflush(stderr);
}
