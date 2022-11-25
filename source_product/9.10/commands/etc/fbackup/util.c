/* @(#) $Revision: 70.1 $ */

/***************************************************************************
****************************************************************************

	util.c

    This file contains functions which are common to at least two of the 
    three types of processes (main, reader(s), and writer).

****************************************************************************
***************************************************************************/

#include "head.h"
#include <time.h>

#ifdef NLS
#define NL_SETN 4	/* message set number */
nl_catd nlmsg_fd;
char *nl_langinfo();
#include <langinfo.h>
#endif NLS

/***************************************************************************
    This function writes to stderr and flushes it.  I takes up to 5 arguments,
    a format string, and up to 4 values for the format string.
    NOTE: MSG TAKES ONLY 4 DATA ARGUMENTS, this will bite you if you try and
    give it more.
***************************************************************************/
/* VARARGS */
msg(format, i1, i2, i3, i4)
char *format;
int i1, i2, i3, i4;
{
    (void) fprintf(stderr, format, i1, i2, i3, i4);
    (void) fflush(stderr);
}






/***************************************************************************
    This function is called to ask a yes/no question and won't let the user
    out until it gets one of those answers.  It takes the question as its
    argument, and returns TRUE for affirmative and FALSE for negative
    responses.  If NLS is compiled in, it expects the native language answers.
    If the -y option has been specified (yflag = TRUE), a message that the
    question has automatically been answered affirmatively, and it behaves
    accordingly.
    If fgets() gets EOF, this function returns EOF.
***************************************************************************/
extern PADTYPE *pad;
#define MAXANSLEN 32
#define MAXQUESLEN 256

xquery(inputstr)
char *inputstr;
{
    char question[MAXQUESLEN], ans[MAXANSLEN], nostr[16], yesstr[16];

#ifdef NLS
    (void) strcpy(yesstr, nl_langinfo(YESSTR));
    (void) strcpy(nostr,  nl_langinfo(NOSTR));
#else NLS
    (void) strcpy(yesstr, "yes");
    (void) strcpy(nostr,  "no");
#endif NLS

    (void) strncpy(question, inputstr, MAXQUESLEN);
    for (;;) {
	msg(question);
	if (pad->yflag) {
	    msg((catgets(nlmsg_fd,NL_SETN,1, "fbackup(4001): automatic '%s'\n")), yesstr);
	    return(TRUE);
	} else {
	    if (fgets(ans, MAXANSLEN, stdin) == NULL) /* avoid loop for EOF */
		return (EOF);
	    ans[strlen(ans)-1] = '\0';			/* strip off '\n' */
	    if (!strcmp(yesstr, ans))
		return(TRUE);
	    else if (!strcmp(nostr, ans))
		return(FALSE);
	    else
		msg((catgets(nlmsg_fd,NL_SETN,2, "fbackup(4002): please respond with: '%s' or '%s'\n")), yesstr, nostr);
	}
    }
}






#define GET    (-1)		/* binary semaphore operations */
#define RELEASE (1)

int semaid;
static struct sembuf sops = {0, 0, 0};

/***************************************************************************
    This function performs semaphore operations (GET and RELEASE) on the one-
    and-only semaphore in this applicaton, the one which ensures exclusive
    access to the variable pad->avail, the amount of available shared memory.
    If the semop call is interrupted, which happens infrequently, it is
    tried again until is succeeds.
***************************************************************************/
atomicadd(value)
int value;
{
    int r, op;

		/* This for loop ALWAYS iterates twice, and the variable op
		   is assigned one of two values: on pass 1, op=GET,
		   and on pass 2 op=RELEASE.  (Note: GET != RELEASE.)
		   It may be confusing, but it's fast.
		*/
    for (op=GET; op!=RELEASE+RELEASE-GET; op+=(RELEASE-GET)) {
	sops.sem_op = op;
	do {
	    r = semop(semaid, &sops, 1);
	    if ((r < 0) && (errno != EINTR)) {
		perror("fbackup(9999)");
		msg((catgets(nlmsg_fd,NL_SETN,3, "fbackup(4003): semaphore op failed: op=%d\n")), op);
	    }
	} while ((r < 0) && (errno == EINTR));
	if (op == GET)
	    pad->avail += value;
    }
}






/***************************************************************************
    Mymalloc does one thing that ordinary malloc doesn't; it prints an error
    message indicating that there wasn't any memory left.
***************************************************************************/
long *
mymalloc(size)
unsigned size;
{
    char *ptr;

    if ((ptr=(char*)malloc(size+sizeof(int))) == (char*)NULL)
	msg((catgets(nlmsg_fd,NL_SETN,4, "fbackup(4004): function mymalloc could not allocate space\n")));
    return(ptr);
}   






/***************************************************************************
    Myctime calls nl_ctime if NLS is defined and ctime if it isn't.  There
    is a slight difference in output between them, ctime returns a string
    with a '\n' at the end, and this must be stripped off for compatibility
    so that the code that calls myctime behaves the same in either case.
***************************************************************************/
char *
myctime(time)
long *time;
{
#ifdef NLS
    char *nl_cxtime();
    return(nl_cxtime(time, (char*)NULL));
#else NLS
    char *timestr;
    timestr = ctime(time);
    *(timestr+strlen(timestr)-1) = '\0';	/* strip off the '\n' */
    return(timestr);
#endif NLS
}
