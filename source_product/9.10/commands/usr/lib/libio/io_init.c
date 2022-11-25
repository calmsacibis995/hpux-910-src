/* $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libio/io_init.c,v $
 * $Revision: 70.2 $        $Author: ssa $
 * $State: Exp $        $Locker:  $
 * $Date: 92/04/07 15:00:49 $
 */

/* AUTOBAHN: fix these when files are put in right place */
#include <sys/libio.h>
#include <machine/dconfig.h>

/*
 * Library routines to manipulate /dev/config. Included are:
 *
 * io_init - Initialize the libio(3A) library.
 * io_end  - End a session using the libio(3A) library.
 */

/* Global variables */

int	    config_fd = -1;	/* file descriptor for /dev/config */
extern int  errno;		/* system call error number */

/**************************************************************************
 * io_init(flag)
 **************************************************************************
 *
 * Description:
 *	Initialize the libio(3A) library.
 *
 * Input Parameters:
 *	flag    Specifies how to open /dev/config.  One of:
 *	    O_RDONLY		Open for reading
 *	    O_WRONLY		Open for writing
 *	    O_RDWR		Open for reading and writing
 *
 * Output Parameters: None
 *
 * Returns:
 *	SUCCESS (0)		Success.
 *	ALREADY_OPEN		/dev/config is already open.
 *	INVALID_FLAG		The flag argument is invalid.
 *	NO_SUCH_FILE		/dev/config does not exist.
 *	PERMISSION_DENIED	Cannot open /dev/config with the
 *				specified flag.
 *	SYSCALL_ERROR		Non-specific system call error - see errno.
 *
 * Globals Referenced:
 *	config_fd	Set to the file descriptor returned by open(2).
 *	errno		System call error number.
 *
 * External Calls:
 *	open(2)
 *
 * Algorithm:
 * AUTOBAHN:  could we count on open failing with EINVAL instead?
 *	Check the "flag" parameter and fail with INVALID_FLAG if not valid.
 *	Do an open(2) of /dev/config.
 *	Fail if we got an error as follows:
 *	    errno	    function return
 *	    ---------------------------------
 *	    EACCES	    PERMISSION_DENIED
 *	    EBUSY	    ALREADY_OPEN
 *	    ENOENT	    NO_SUCH_FILE
 *	    other	    SYSCALL_ERROR
 *	Else set the global "config_fd" to the result of the open(2) and
 *	return SUCCESS.
 *
 **************************************************************************/

int
io_init(flag)
  int	flag;
{
    int fd;	/* used so that open failures don't change config_fd */

    if (!(flag == O_RDONLY || flag == O_WRONLY || flag == O_RDWR))
	return(INVALID_FLAG);

    if ((fd = open(DEVCONFIG_FILE, flag)) == -1)
	switch (errno) {
	case EACCES:
	    return(PERMISSION_DENIED);
	case EBUSY:
	    return(ALREADY_OPEN);
	case ENOENT:
	    return(NO_SUCH_FILE);
	default:
	    return(SYSCALL_ERROR);
	}

    config_fd = fd;
    return(SUCCESS);
}

/**************************************************************************
 * io_end()
 **************************************************************************
 *
 * Description:
 *	End a session using the libio(3A) library.
 *
 * Input Parameters: None
 *
 * Output Parameters: None
 *
 * Returns: None
 *
 * Globals Referenced:
 *	config_fd	The /dev/config file descriptor.
 *
 * External Calls:
 *	close(2)
 *
 * Algorithm:
 *	Call close(2) on config_fd.
 *	Set config_fd to -1 so any subsequent calls to library functions
 *	will return NOT_OPEN.
 *
 **************************************************************************/

void
io_end()
{
    (void)close(config_fd);
    config_fd = -1;
}
