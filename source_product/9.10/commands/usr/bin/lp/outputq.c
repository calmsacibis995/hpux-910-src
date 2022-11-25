/* @(#) $Revision: 70.1 $ */      
/* routines for manipulating output queue in spool directory */

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 12					/* set number */
#include <msgbuf.h>
#endif NLS

#include	"lp.h"
#ifdef TRUX
#include <sys/security.h>
#endif

static FILE *ro = NULL;
static FILE *wo = NULL;
static long ooffset = 0L;
static int n_del = 0;		/* number of O_DEL entries in outputq */
# define MAXDEL 100		/* threshhold to trigger outputq compression */

/* endoent() -- clean up after last call to getoent() */
/*
	Note:	The "wo" stream should be closed first.
		This is because the file lock will be released
		by the close and you want the output (if any)
		to be flushed.  If you close the "ro" stream
		first, another command waiting on the lock may
		not see the change to the file.
*/

endoent()
{
	if(wo != NULL) {
		fclose(wo);
		wo = NULL;
	}
	if(ro != NULL) {
		fclose(ro);
		ro = NULL;
	}
}

/* getodest(o, dest) -- finds the next output queue entry for destination dest
			and returns status info in the supplied structure */

int
getodest(o, dest)
struct outq *o;
char *dest;
{
	int ret;

	setoent();
	while((ret = getoent(o)) != EOF && strcmp(o->o_dest, dest) != 0)
		;
	return(ret);
}

/* getoid(o, dest, seqno) -- finds the output queue entry for request id
			and returns status info in the supplied structure */

int
getoid(o, dest, seqno)
struct outq *o;
char *dest;
int seqno;
{
	setoent();
	while(getoent(o) != EOF)
		if(o->o_seqno == seqno && strcmp(o->o_dest, dest) == 0)
			return(0);
	return(EOF);
}

/* getoent(o) -- get next entry from output queue */

int
getoent(o)
struct outq *o;
{
	if(wo == NULL || ro == NULL)
		setoent();

	ooffset = ftell(ro);
        fflush (ro);

	while(fread((char *)o, sizeof(struct outq), 1, ro) == 1) {
	    if(o->o_flags & O_DEL) {	/* skip deleted records */
		n_del++;
		ooffset = ftell(ro);
	    } else {
		return 0;
	    }
	}
	return EOF;
}

/* setoent() -- initialize for subsequent calls to getoent() */

setoent()
{
	extern FILE *lockf_open();

	if((wo == NULL && (wo = lockf_open(OUTPUTQ, "r+", TRUE)) == NULL) ||
	   (ro == NULL && (ro = lockf_open(OUTPUTQ, "r", FALSE)) == NULL))
		fatal((nl_msg(2,"can't open output queue file")), 1);
#ifdef SecureWare
	if(ISSECURE)
        	lp_change_mode(SPOOL, OUTPUTQ, 0644, ADMIN,
                       "output queue file for printer");
	else
		chmod(OUTPUTQ, 0644);
#else
	chmod(OUTPUTQ, 0644);
#endif
	rewind(wo);
	rewind(ro);
	if (n_del > MAXDEL) {
/* copy outputq over itself, eliminating O_DEL entries, then truncate */
/* reads from ro and writes to wo */
	    struct outq qitem;
	    while (getoent(&qitem) != EOF)
		wrtoent(&qitem, wo);
	    fflush(wo);
	    ftruncate(fileno(wo), ftell(wo));
	    rewind(wo);
	    rewind(ro);
	}
	n_del = 0;
	ooffset = ftell(ro);
}


/* putoent -- write output queue entry, overwriting the last record that
	      was returned by getoent() or getoid() */

putoent(o)
struct outq *o;
{
	fseek(wo, ooffset, 0);
	wrtoent(o, wo);
	if (o->o_flags & O_DEL)
	    n_del++;
}

/* addoent -- write output queue entry to end of output queue file */
/* Compress the queue if its getting too many O_DEL entries */

addoent(o)
struct outq *o;
{
	if(wo == NULL || ro == NULL)
		setoent();
	fseek(wo, 0l, 2);
	wrtoent(o, wo);
}


/* wrtoent -- write an output queue entry to the named stream */

wrtoent(o, stream)
struct outq *o;
FILE *stream;
{
	fwrite((char *)o, sizeof(struct outq), 1, stream);
}

#ifdef REMOTE
/* getodestuser(o, dest, user) -- finds the output queue entry for
			the specified destination and user
			and returns status info in the supplied structure */

int
getodestuser(o, dest, user)
struct outq *o;
char *dest;
char *user;
{
	setoent();
	while(getoent(o) != EOF)
		if((strcmp(o->o_logname,user) == 0) && (strcmp(o->o_dest, dest) == 0))
			return(0);
	return(EOF);
}


/* getoidhost(o, dest, seqno, host) --	finds the output queue entry for the
					destination, request id and host 
					and returns status info in the
					supplied structure */

int
getoidhost(o, dest, seqno, host)
struct outq *o;
char *dest;
int seqno;
char *host;
{
	setoent();
	while(getoent(o) != EOF)
		if(o->o_seqno == seqno && strcmp(o->o_dest, dest) == 0 &&
			strcmp(o->o_host,host) == 0)
			return(0);
	return(EOF);
}
#endif REMOTE
