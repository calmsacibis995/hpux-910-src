/* @(#) $Revision: 66.3 $ */    

#ifdef _NAMESPACE_CLEAN
#define getmsg _getmsg
#define msglenzero _msglenzero
#define lseek _lseek
#define read _read
#endif

#include <errno.h>
#include <msgcat.h>

#define	NULL		0	/* Null pointer address */

extern int	errno;

static int	msglzero = FALSE;
static int	set_exists = FALSE;
static char	savebuf[DIRSIZE];

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getmsg
#pragma _HP_SECONDARY_DEF _getmsg getmsg
#define getmsg _getmsg
#endif

char *
getmsg(fd, setnum, msgnum, buf, buflen)
	int	fd, setnum, msgnum, buflen;
	char	*buf;
/*
 * This procedure reads the message line specified by `setnum'
 * and `msgnum' from the file `fd'.  In this procedure, no
 * substitution will be done.
 */
{
	DIR	*msgdir,
		*search_dir();
	int	msglen,
		lseek(),
		read();

	msglzero = FALSE;
	if (fd < 0) {
		errno = EBADF;
		*buf = NULL;
		return(buf);
	}
	if (buflen <= 1) {
		/* There is no enough space */
		errno = EINVAL;
		return("");
	}
	/*
	 * First search the directory specified by setnum
	 * and msgnum.
	 */
	if ((msgdir = search_dir(fd, setnum, msgnum)) == NULL) {
		/*
		 * Some error occurred in search_dir procedure.
		 * Errno must be set in the search_dir procedure.
		 * Returns the empty string.
		 */
		*buf = NULL;
		return(buf);
	}

	/*
	 * Set the file pointer to the message.
	 */
	if (lseek(fd, msgdir->addr, L_SET) == ERROR) {
		/*
		 * Probably file system error occurred.
		 * Errno must be set in the lseek procedure.
		 * Returns the empty string.
		 */
		*buf = NULL;
		return(buf);
	}

	/*
	 * Check the buffer length.
	 */
	if (msgdir->length > buflen - 1) {
		msglen = buflen - 1;
		errno = ERANGE;
	}
	else
		msglen = msgdir->length;

	if(!msglen)
		msglzero = TRUE;
	else
		msglzero = FALSE;

	/*
	 * Read the message from the file.
	 */
	if (read(fd, buf, msglen) != msglen) {
		/*
		 * Probably file system error occurred.
		 * Errno must be set in the read procedure.
		 * Returns the empty string.
		 */
		*buf = NULL;
		return(buf);
	}
	buf[msglen] = '\0';
	return(buf);
}

static DIR *
search_dir(fd, setnum, msgnum)
	int fd, setnum, msgnum;
/*
 * This procedure search the directory area against the message
 * directory specified by setnum and msgnum, and returns the
 * directory.
 */
{
	long	dir_left,
		numdirs,
		readlen;
	DIR	*result,
		*search_msg();
	char	readbuf[BUFLEN],
		*ptr;
	int	lseek(),
		read(),
		bufncpy();

	/*
	 * Check the set number and message number.
	 */
	if (setnum < 1 || setnum > MAX_SETNUM) {
		errno = EINVAL;		/* E_ILLEGAL_SET */
		return(NULL);
	}
	if (msgnum < 1 || msgnum > MAX_MSGNUM) {
		errno = EINVAL;		/* E_ILLEGAL_MSG */
		return(NULL);
	}
	/*
	 * Set the file pointer at the top of the file.
	 */
	if (lseek(fd, 0, L_SET) == ERROR)
		/*
		 * Probably file system error occurred.
		 * Errno must be set in the lseek procedure.
		 */
		return(NULL);
	/*
	 * Read the first part of the file.
	 */
	if ((readlen = read(fd, readbuf, BUFLEN)) == ERROR)
		/*
		 * Probably file system error occurred.
		 * Errno must be set in the lseek procedure.
		 */
		return(NULL);
	else if (readlen < OVERHEAD) {	/* wrong file */
		errno = EINVAL;
		return(NULL);
	}
	/* Calculate the number of directories in the buffer */
	numdirs = (readlen - OVERHEAD) / DIRSIZE;
	ptr = &readbuf[DIR_START_POS];

	/* Get the number of messages */
	dir_left = *(long *)(&readbuf[NUM_MSG_POS]);
	if (numdirs > dir_left)
		numdirs = dir_left;
	for (;;) {
		/*
		if ((result = search_msg(ptr, numdirs, setnum, msgnum)) != NULL
		     && result != ERROR)
		*/
		result = search_msg(ptr, numdirs, setnum, msgnum);
		if ((int)result != NULL && (int)result != ERROR)
			/* Directory was found. */
			break;

		if ((dir_left -= numdirs) <= 0 || (int)result == ERROR) {
			/*
			 * The directory we desire does not exist
			 * int the file.
			 * Check whether the set exists or not.
			 */
			if (set_exists)
				errno = E_NO_MSG;
			else
				errno = E_NO_SET;
			return(NULL);
		}
		else {
			/*
			 * The desired directory may be after this
			 * position.  Read next buffer.
			 */
			if ((readlen = read(fd, readbuf, BUFLEN)) == ERROR)
				/*
				 * Probably file system error occurred.
				 * Errno must be set in the read procedure.
				 */
				return(NULL);
			else if (readlen == 0) {
				/*
				 * Unexpected EOF found.
				 */
				errno = EINVAL;
				return(NULL);
			}
			/* Calculate the number of dirs in the buffer */
			numdirs = readlen / DIRSIZE;
			if (numdirs > dir_left)
				numdirs = dir_left;
			ptr = readbuf;
		}
	}
	bufncpy(savebuf, result, DIRSIZE);
	return((DIR *)savebuf);
}

static
bufncpy(to, from, len)
	int len;
	char *to, *from;
/*
 * This procedure copies `n' bytes regardless null characters
 * from `from' buffer to `to' buffer.
 */
{
	while (len--)
		*to++ = *from++;
}

static DIR *
search_msg(buf, numdirs, setnum, msgnum)
	char *buf;
	long numdirs;
	int setnum, msgnum;
{
	DIR	*dirptr,
		*getdir();
	long	start,
		mid,
		end;

	/*
	 * Check the last directory in the buffer whether the
	 * desired directory is in the buffer.
	 */
	dirptr = getdir(buf, numdirs - 1);
	if (setnum > dirptr->setnum ||
	    setnum == dirptr->setnum && msgnum > dirptr->msgnum)
		/* The desired directory may be after this buffer */
		return(NULL);

	/*
	 * The desired directory must be in the buffer.  Search it
	 * using the binary search.
	 */
	start = 0;
	end = numdirs - 1;
	set_exists = FALSE;
	while (start <= end) {
		mid = (start + end) / 2;
		dirptr = getdir(buf, mid);
		if (dirptr->setnum == setnum) {
			set_exists = TRUE;
			if (dirptr->msgnum == msgnum) {
				/*
				 * Directory was found.
				 * Return the address.
				 */
				return(dirptr);
			}
			if (dirptr->msgnum > msgnum)
				end = mid - 1;
			else
				start = mid + 1;
		}
		else if (dirptr->setnum > setnum)
			end = mid - 1;
		else
			start = mid + 1;
	}
	/*
	 * Directory was not found.  It does not exist
	 * in this file.
	 */
	return((DIR *)ERROR);
}

static DIR *
getdir(buf, num)
	char *buf;
	long num;
{
	return((DIR *)&buf[num * DIRSIZE]);
}

int
msglenzero()
{
return(msglzero);
}
