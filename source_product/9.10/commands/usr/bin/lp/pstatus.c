/* @(#) $Revision: 66.2 $ */      
/* routines for manipulating printer status file in spool directory */

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 13					/* set number */
#include <msgbuf.h>
#endif NLS

#include	"lp.h"
#ifdef TRUX
#include <sys/security.h>
#endif

static FILE *rp = NULL;
static FILE *wp = NULL;
static long poffset = 0L;


/* endpent() -- clean up after last call to getpent() */
/*
	Note:	The "wp" stream should be closed first.
		This is because the file lock will be released
		by the close and you want the output (if any)
		to be flushed.  If you close the "rp" stream
		first, another command waiting on the lock may
		not see the change to the file.
*/

endpent()
{
	if(wp != NULL) {
		fclose(wp);
		wp = NULL;
	}
	if(rp != NULL) {
		fclose(rp);
		rp = NULL;
	}
}

/* getpdest(p, dest) -- finds the printer status entry for destination dest
			and returns status info in the supplied structure */

int
getpdest(p, dest)
struct pstat *p;
char *dest;
{
	int ret;

	setpent();
	while((ret = getpent(p)) != EOF && strcmp(p->p_dest, dest) != 0)
		;
	return(ret);
}

/* getpent(p) -- get next entry from printer status file */

int
getpent(p)
struct pstat *p;
{
	if(wp == NULL || rp == NULL)
		setpent();
	poffset = ftell(rp);
	return((fread((char *)p, sizeof(struct pstat), 1, rp)) != 1 ? EOF : 0);
}
/* setpent() -- initialize for subsequent calls to getpent() */

setpent()
{
	extern FILE *lockf_open();

	if((wp == NULL && (wp = lockf_open(PSTATUS , "r+", FALSE)) == NULL) ||
	   (rp == NULL && (rp = lockf_open(PSTATUS, "r", FALSE)) == NULL))
		fatal((nl_msg(2,"can't open printer status file")), 1);
#ifdef SecureWare
	if(ISSECURE)
            lp_change_mode(SPOOL, PSTATUS, 0644, ADMIN, "printer status file");
	else
	    chmod(PSTATUS, 0644);
#else	
	chmod(PSTATUS, 0644);
#endif
	rewind(wp);
	rewind(rp);
	poffset = ftell(rp);
}


/* putpent -- write pstatus entry, overwriting the last record that
	      was returned by getpent() or getpdest() */

putpent(p)
struct pstat *p;
{
	fseek(wp, poffset, 0);
	wrtpent(p, wp);
}

/* addpent -- write pstatus entry to end of pstatus file */

addpent(p)
struct pstat *p;
{
	if(wp == NULL || rp == NULL)
		setpent();
	fseek(wp, 0L, 2);
	wrtpent(p, wp);
}


/* wrtpent -- write an entry in the pstatus file */

wrtpent(p, stream)
struct pstat *p;
FILE *stream;
{
	fwrite((char *)p, sizeof(struct pstat), 1, stream);
}
