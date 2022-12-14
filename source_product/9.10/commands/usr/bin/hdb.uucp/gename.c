#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1
#include <nl_types.h>
#endif NLS
/*	@(#) $Revision: 51.3 $	*/
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	/sccs/src/cmd/uucp/s.gename.c
	gename.c	1.1	7/29/85 16:32:57
*/
#include "uucp.h"

static struct {
	char	sys[NAMESIZE];
	int	job;
	int	subjob;
} syslst[30];		/* no more than 30 systems per job */

static int nsys = 0;

 /* generate file name
  *	pre	-> file prefix
  *	sys	-> system name
  *	grade	-> service grade 
  *	file	-> buffer to return filename must be of size DIRSIZ+1
  * return:
  *	none
  */
gename(pre, sys, grade, file)
char pre, *sys, grade, *file;
{
	int	n;

	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,506, "gename(%c, ")), pre);
	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,507, "%s, ")), sys);
	DEBUG(9, (catgets(nlmsg_fd,NL_SETN,508, "%c)\n")), grade);
	if (*sys == '\0') {
		sys = Myname;
		DEBUG(9, (catgets(nlmsg_fd,NL_SETN,509, "null sys -> %s\n")), sys);
	}
	n = sysseq(sys);
	if (pre == CMDPRE || pre == XQTPRE) {
		(void) sprintf(file, "%c.%.*s%c%.4x",
			pre, SYSNSIZE, sys, grade, syslst[n].job); 
	} else
		(void) sprintf(file, "%c.%.5s%.4x%.3x",
			pre, sys, syslst[n].job & 0xffff,
				++syslst[n].subjob & 0xfff); 
	DEBUG(4, (catgets(nlmsg_fd,NL_SETN,510, "file - %s\n")), file);
	return;
}


#define SLOCKTIME 10
#define SLOCKTRIES 5
#define SEQLEN 4

 /*
  * get next sequence number
  * returns:  
  *	number between 1 and 0xffff
  *
  * sequence number 0 is reserved for polling
  */
static int
getseq(sys)
char	*sys;
{
	register FILE *fp;
	register int i;
	int n;
	int	seed;
	char seqlock[MAXFULLNAME], seqfile[MAXFULLNAME];

	ASSERT(nsys < sizeof (syslst)/ sizeof (syslst[0]),
	     "SYSLST OVERFLOW", "", sizeof (syslst));

	(void) time(&seed);	/* crank up the sequence initializer */
	srand(seed);

	(void) sprintf(seqlock, "%s%s", SEQLOCK, sys);
	BASENAME(seqlock, '/')[MAXBASENAME] = '\0';
	for (i = 1; i < SLOCKTRIES; i++) {
		if (!ulockf(seqlock, (time_t) SLOCKTIME))
			break;
		sleep(5);
	}

	ASSERT(i < SLOCKTRIES, Ct_LOCK, seqlock, 0);

	(void) sprintf(seqfile, "%s/%s", SEQDIR, sys);
	if ((fp = fopen(seqfile, "r")) != NULL) {
		/* read sequence number file */
		if (fscanf(fp, "%4x", &n) != 1) {
		    n = rand();
		    clearerr(fp);
		}
		fp = freopen(seqfile, "w", fp);
		ASSERT(fp != NULL, Ct_OPEN, seqfile, errno);
		(void) chmod(seqfile, 0666);
	} else {
		/* can not read file - create a new one */
		ASSERT((fp = fopen(seqfile, "w")) != NULL,
		    Ct_CREATE, seqfile, errno);
		(void) chmod(seqfile, 0666);
		n = rand();
	}

	n++;
	n &= 0xffff;	/* 4 byte sequence numbers */
	(void) fprintf(fp, "%.4x\n", n);
	ASSERT(ferror(fp) == 0, Ct_WRITE, seqfile, errno);
	(void) fclose(fp);
	ASSERT(ferror(fp) == 0, Ct_CLOSE, seqfile, errno);
	rmlock(seqlock);
	DEBUG(6, (catgets(nlmsg_fd,NL_SETN,512, "%s seq ")), sys);
        DEBUG(6, (catgets(nlmsg_fd,NL_SETN,513, "now %x\n")), n);
	(void) strcpy(syslst[nsys].sys, sys);
	syslst[nsys].job = n;
	syslst[nsys].subjob = rand() & 0xfff;	/* random initial value */
	return(nsys++);
}

static int
sysseq(sys)
char	*sys;
{
	int	i;

	for (i = 0; i < nsys; i++)
		if (strncmp(syslst[i].sys, sys, SYSNSIZE) == SAME)
			return(i);

	return(getseq(sys));
}

/*
 *	initSeq() exists because it is sometimes important to forget any
 *	cached work files.  for example, when processing a bunch of spooled X.
 *	files, we must not re-use any C. files used to send back output.
 */
void
initSeq()
{
	nsys = 0;
}
