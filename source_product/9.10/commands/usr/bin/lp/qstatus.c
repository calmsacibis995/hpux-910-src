/* @(#) $Revision: 66.2 $ */      
/* routines for manipulating qstatus file in spool directory */

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 14					/* set number */
#include <msgbuf.h>
#endif NLS

#include	"lp.h"
#ifdef TRUX
#include <sys/security.h>
#endif

static FILE *rq = NULL;
static FILE *wq = NULL;
static long qoffset = 0L;

/* endqent() -- clean up after last call to getqent() */
/*
	Note:	The "wq" stream should be closed first.
		This is because the file lock will be released
		by the close and you want the output (if any)
		to be flushed.  If you close the "rq" stream
		first, another command waiting on the lock may
		not see the change to the file.
*/

endqent()
{
	if(wq != NULL) {
		fclose(wq);
		wq = NULL;
	}
	if(rq != NULL) {
		fclose(rq);
		rq = NULL;
	}
}

/* getqdest(q, dest) -- finds the acceptance status entry for destination dest
			and returns status info in the supplied structure */

int
getqdest(q, dest)
struct qstat *q;
char *dest;
{
	int ret;

	setqent();
	while((ret = getqent(q)) != EOF && strcmp(q->q_dest, dest) != 0)
		;
	return(ret);
}

/* getqent(q) -- get next entry from acceptance status file */

int
getqent(q)
struct qstat *q;
{
	if(wq == NULL || rq == NULL)
		setqent();
	qoffset = ftell(rq);
	return(fread((char *)q, sizeof(struct qstat), 1, rq) != 1 ? EOF : 0);
}
/* setqent() -- initialize for subsequent calls to getqent() */

setqent()
{
	extern FILE *lockf_open();

	if((wq == NULL && (wq = lockf_open(QSTATUS , "r+", FALSE)) == NULL) ||
	   (rq == NULL && (rq = lockf_open(QSTATUS, "r", FALSE)) == NULL))
		fatal((nl_msg(2,"can't open acceptance status file")), 1);
#ifdef SecureWare
	if(ISSECURE)
            lp_change_mode(SPOOL, QSTATUS, 0644, ADMIN,
                       "printer acceptance status file");
	else
	    chmod(QSTATUS, 0644);
#else
	chmod(QSTATUS, 0644);
#endif
	rewind(wq);
	rewind(rq);
	qoffset = ftell(rq);
}


/* putqent -- write qstatus entry, overwriting the last record that
	      was returned by getqent() or getqdest() */

putqent(q)
struct qstat *q;
{
	fseek(wq, qoffset, 0);
	wrtqent(q, wq);
}

/* addqent -- write qstatus entry to end of qstatus file */

addqent(q)
struct qstat *q;
{
	if(wq == NULL || rq == NULL)
		setqent();
	fseek(wq, 0L, 2);
	wrtqent(q, wq);
}


/* wrtqent -- write an entry in the qstatus file */

wrtqent(q, stream)
struct qstat *q;
FILE *stream;
{
	fwrite((char *)q, sizeof(struct qstat), 1, stream);
}
