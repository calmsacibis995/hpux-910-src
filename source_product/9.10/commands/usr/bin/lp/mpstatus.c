/* @(#) $Revision: 66.2 $ */      
/* routines for manipulating printer status file in spool directory */

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 17					/* set number */
#include <msgbuf.h>
#endif NLS

#include	"lp.h"
#ifdef TRUX
#include <sys/security.h>
#endif

static FILE *rp = NULL;
static	char	*dest_buf_ptr = NULL;
static	char	*tmp_ptr	= NULL;
static	char	*end_ptr	= NULL;
static	int	file_closed	= TRUE;
static	int	buffer_empty	= TRUE;

/* endmpent() -- clean up after last call to getmpent() */

endmpent()
{
	if (dest_buf_ptr != NULL){
		(void) free(dest_buf_ptr);
	}
	file_closed	= TRUE;
	buffer_empty	= TRUE;
}

/* getmpdest(p, dest) -- finds the printer status entry for destination dest
			and returns status info in the supplied structure */

int
getmpdest(p, dest)
struct pstat *p;
char *dest;
{
	int ret;

	setmpent();
	while((ret = getmpent(p)) != EOF && strcmp(p->p_dest, dest) != 0)
		;
	return(ret);
}

/* getmpent(p) -- get next entry from printer status file */

int
getmpent(p)
struct pstat *p;
{
	if (file_closed)
		setmpent();
	return((structread((char *)p, sizeof(struct pstat), 1)) != 1 ? EOF : 0);
}
/* setmpent() -- initialize for subsequent calls to getmpent() */

setmpent()
{
	extern	FILE *lockf_open();

	if ((buffer_empty && (rp = lockf_open(PSTATUS, "r", FALSE)) == NULL))
		fatal((nl_msg(2,"can't open printer status file")), 1);

#ifdef SecureWare
	if(ISSECURE)
	    lp_change_mode(SPOOL, PSTATUS, 0644, ADMIN,
			"begin lookup in printer status file");
	else
	    chmod(PSTATUS, 0644);
#else
	chmod(PSTATUS, 0644);
#endif
	if (buffer_empty)
		rewind(rp);
	tmp_ptr	= dest_buf_ptr;
	file_closed = FALSE;
}
/* structread() --	returns num_to_read records from the buffer
			that holds the printer status file.
			returns 0 if EOF is reached
*/
int
structread(buffer, buffer_size, num_to_read)
char	*buffer;
int	buffer_size;
int	num_to_read;
{
	int	ii;
	int	total_length;
	unsigned	length;
	struct	stat	buf;
	extern	char	*malloc();
	extern	char	errmsg[];

	if (buffer_empty){
		if (stat(PSTATUS, &buf) == -1){
			sprintf(errmsg,"Unable to stat the printer status file");
			fatal(errmsg,1);
		}
		length = (unsigned) buf.st_size;
		if ((dest_buf_ptr = malloc(length)) == NULL){
			fatal("Unable to malloc enough memory to read the printer status file",1);
		}
		tmp_ptr = dest_buf_ptr;
		end_ptr = dest_buf_ptr + length;
		ii = fread(dest_buf_ptr, length, 1, rp);
		fclose(rp);
		if (ii == 0){
			return(0);
		}
		buffer_empty	= FALSE;
	}
	total_length = buffer_size * num_to_read;
	if ((tmp_ptr + total_length) > end_ptr){
		return(0);
	}
	memcpy(buffer, tmp_ptr, total_length);
	tmp_ptr += total_length;
	return(num_to_read);
}
