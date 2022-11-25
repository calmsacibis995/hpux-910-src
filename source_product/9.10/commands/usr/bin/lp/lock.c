/* $Revision: 66.4 $ */
/* Now
/* @(#) $Pre Revision: 62.1 $ */    
#include	"lp.h"
#include        <unistd.h>
#ifdef TRUX
#include <sys/security.h>
#endif


/* lockf_open() first checks to see if the file exists. If it does not    */
/* and createflag == FALSE, then a null file pointer is returned. Other-  */
/* wise we create the file. If the file exists then we open it for        */
/* read/write access. lockf_open() then calls lockf() to lock the file.   */
/* Finally, lockf_open() calls fdopen to get a stdio file pointer.        */
/* This procedure is necessary for portability since if we were to use    */
/* fopen() and the fileno() macro for lockf(), we would not be            */
/* guaranteed be positioned at the same point in the file.                */

FILE *
lockf_open(filename,mode,createflag)
char	*filename;
char	*mode;
int	createflag;
{
	int	count;
	int	fd;
	char	tmp_buf[FILEMAX];

#ifdef SecureWare
    privvec_t	save_privs;
    (void) forceprivs(privvec(SEC_ALLOWMACACCESS, SEC_ALLOWDACACCESS, -1),
		      save_privs);

    if (((ISSECURE) && (GET_ACCESS(filename,0) != 0)) ||
	((!ISSECURE) && (access(filename,0) != 0)))
#else
	if (access(filename,0) != 0) 
#endif
	{
	if (createflag == FALSE)
		return( (FILE *)0);

		if ((fd = open(filename,(O_RDWR|O_CREAT|O_EXCL),0644)) < 0)
			return( (FILE *)0);
	}
	else
	{
	if ((fd = open(filename,O_RDWR)) < 0)
#ifdef SecureWare
		{
			seteffprivs(save_privs, NULL);
			return( (FILE *)0);
		}
#else
		return( (FILE *)0);
#endif
	}

	count = 0;
loop:
	if (lockf(fd,F_LOCK,0) != 0){
		if (errno == EINTR){
			count++;
			if ((count % 100) == 0){
				sprintf(tmp_buf, "lockf has been interrupted %d times\n", count);
				fatal(tmp_buf, 0);
			}
			goto loop;
		}
		sprintf(tmp_buf, "Error %d from lockf call on file %s\n", errno, filename);
		fatal(tmp_buf, 0);
#ifdef SecureWare
		seteffprivs(save_privs, NULL);
#endif
		return( (FILE *)0);
	}
#ifdef SecureWare
	seteffprivs(save_privs, NULL);
#endif
	return(fdopen(fd,mode));
}

/* unlock_file -- unlock the locked file but not close it. */

unlock_file(file)
FILE	*file;
{
	fseek(file, 0, 0);	/* rewind the file pointer */
    again:
	if (lockf(fileno(file), F_ULOCK, 0) != 0){
		if (errno == EINTR){
			goto again;
		}
		return(-1);
	}

	return(0);
}
