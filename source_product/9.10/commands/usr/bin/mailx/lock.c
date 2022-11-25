/* @(#) $Revision: 66.1 $ */     
#

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Stuff to do version 7 style locking.
 */

#include "rcv.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#ifndef NLS
#define catgets(i,sn,mn,s) (s)
#else NLS
#define NL_SETN 22       /* set number */
#include <msgbuf.h>
#endif NLS

#ifdef USG
char	*maillock	= ".lock";		/* Lock suffix for mailname */
#else
char	*maillock	= ".mail";		/* Lock suffix for mailname */
#endif
static char		curlock[50];		/* Last used name of lock */
static	int		locked;			/* To note that we locked it */

/*
 * Lock the specified mail file by setting the file mailfile.lock.
 * We must, of course, be careful to remove the lock file by a call
 * to unlock before we stop.  The algorithm used here is to see if
 * the lock exists, and if it does, to check its modify time.  If it
 * is older than 5 minutes, we assume error and set our own file.
 * Otherwise, we wait for 5 seconds and try again.
 *
 * If there's no lock file for the mail file, check whether the file 
 * is locked  by lockf() * or not. When the file is not locked by 
 * lockf(), lock the mail file by lockf(). lock() returns the file
 * pointer for the mail file.
 *
 *		Added 01/22/90	
 */

FILE *
lock(file, mode, direction)
char *file;
char *mode;
int  direction;
{
	register int f, fd;
	struct stat sbuf;
	int count, count_lock, iteration, print_out;
	long curtime;
	FILE *fp;

	count = 1;
	count_lock =  0;
	print_out = 0;
	iteration = 0;
	if (file == NOSTR) {
		return( (FILE *) -1);
	}
	if (access(file, 0) != 0)
		return( (FILE *)0);
	if ((fd = open(file, O_RDWR)) < 0)
		return( (FILE *)-1);
	strcpy(curlock, file);
	strcat(curlock, maillock);
	for (;;) {
		if (stat(curlock, &sbuf) >= 0) {
			if (iteration == 0)
				printf((catgets(nl_fn,NL_SETN,1, "Lock file exists!!  Waiting .")));
			else if (iteration >= MAX_COUNT)
				if (direction == IN)
					goto leave_in;
				else
					goto leave_out;
			else
				printf((catgets(nl_fn,NL_SETN,2, ".")));
			iteration++;
			fflush( stdout );
			sleep(5);
			continue;
		} 
		break;
	}

	for (;;) {
		if (lockf(fd, F_TLOCK, 0) != 0) {
			print_out++;
			if ((errno == EAGAIN)||(errno == EACCES)) {
				if (count_lock == 0)
					printf((catgets(nl_fn,NL_SETN,3, "System Mailbox has been locked!!  Waiting .")));
				else if (count_lock >= MAX_COUNT) {
					iteration = count_lock;
					if (direction == IN)
						goto leave_in;
					else
						goto leave_out;
				} else
					printf((catgets(nl_fn,NL_SETN,4, ".")));
				count_lock++;
				fflush( stdout );
				sleep(5);
				continue;
			} else if (errno == EINTR) {
				count++;
				if ((count % 100) == 0){
					printf((catgets(nl_fn,NL_SETN,5,"lockf has been interrupted %d times\n"), count));
					close(fd);
					exit(1);
				}	
				continue;
			} else {	
				printf((catgets(nl_fn,NL_SETN,6, "Error encountered attempting to create lock file\n")));
				close(fd);
				exit(1);
			}
		} else {
			if ((fp = fdopen(fd, mode)) == NULL) {
				close(fd);
				exit(1);
			}
			if(print_out)
				printf("\n");
			return(fp);
		}
	}
leave_in:
 	printf((catgets(nl_fn,NL_SETN,7, "\n\nmailx: can't lock %s file after %d tries\n")), file, iteration);
 	printf((catgets(nl_fn,NL_SETN,8, "\nPlease try to read your mail again in a few minutes\n")));
	exit(1);
leave_out:
 	printf((catgets(nl_fn,NL_SETN,9, "\n\nmailx: can't lock %s file after %d tries\n")), file, iteration);
 	printf((catgets(nl_fn,NL_SETN,10, "\nEmergency Exit!!\n")));
	exit(1);
}

/*
 * Remove the mail lock, and note that we no longer 
 * have it locked.
 */

unlock(fp)
 FILE  *fp;
{
	int  fd;

		fd = fileno(fp);
		lockf(fd, F_ULOCK, 0);
		fclose(fp);
}
