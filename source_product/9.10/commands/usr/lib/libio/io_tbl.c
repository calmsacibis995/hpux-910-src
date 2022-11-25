/* $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libio/io_tbl.c,v $
 * $Revision: 70.2 $        $Author: ssa $
 * $State: Exp $        $Locker:  $
 * $Date: 92/04/07 15:01:02 $
 */

/* AUTOBAHN: fix these when files are put in right place */
#include <sys/libio.h>
#include <malloc.h>
#include <machine/dconfig.h>

/*
 * Library routines to manipulate /dev/config. Included are:
 *
 * io_get_table	    - Get an I/O system table.
 * io_free_table    - Free memory from io_get_table.
 */

/* Global variables */

extern int  config_fd;	    /* file descriptor for /dev/config */

/**************************************************************************
 * io_get_table(which_table, table)
 **************************************************************************
 *
 * Description:
 *	Return the I/O system table specified by which_table.
 *	The space needed for the table is allocated using malloc(3C).
 *	Requires read permission on /dev/config.
 *
 * S800
 * Input Parameters:
 *	which_table	Which table to return.  One of:
 *	    T_IO_CLASS_TABLE	    I/O Device Class Table
 *	    T_IO_KERN_DEV_TABLE	    I/O Kernel Device Table
 *	    T_IO_MGR_TABLE	    I/O Manager Table
 *	    T_IO_SW_MOD_TABLE	    I/O Software Module Table
 *	    T_IO_MOD_TABLE	    Native I/O Module Table
 * S700
 * Input Parameters:
 *	which_table	Which table to return.  One of:
 *	    T_IO_MOD_TABLE          VSC module Table
 *	    T_MEM_DATA_TABLE	    Page zero memory configuration info
 *
 * Output Parameters:
 *	table		Set to point to the list of names.
 *
 * Returns:
 *	>= 0			The number of elements in the table.
 *   Errors (<0):
 *	INVALID_TYPE		The which_table argument is invalid.
 *	NOT_OPEN		/dev/config is not open.
 *	OUT_OF_MEMORY		There is no available memory - malloc failed.
 *	PERMISSION_DENIED	/dev/config is not open read/write.
 *	SYSCALL_ERROR		Non-specific system call error - see errno.
 * S800
 *	STRUCTURES_CHANGED	Kernel data structures keep changing.
 *
 * Globals Referenced:
 *	config_fd	The /dev/config file descriptor.
 *	errno		System call error number.
 *
 * External Calls:
 *	ioctl(2)  read(2)  malloc(3C)  free(3C)
 *
 * Algorithm:
 * S800
 *	Copy "which_table" into a config_get_table_t.
 *	Call ioctl(2) with config_fd, CONFIG_GET_TABLE and the
 *	config_get_table_t.
 *	Fail if we got an error as follows:
 *	    errno	    function return
 *	    ---------------------------------
 *	    EBADF	    NOT_OPEN
 *	    EPERM	    PERMISSION_DENIED
 *	    other	    SYSCALL_ERROR
 *	Else check the status from the config_get_table_t and return
 *	it if negative (INVALID_TYPE), or 0 (table is empty).
 *	If the status is >0, it indicates the number of table elements,
 *	so malloc(3C) space for them and set table to point to the space.
 *	If the malloc failed, return OUT_OF_MEMORY.
 *	Do a read(2) to get the table.
 *	If the read failed with EAGAIN, the kernel structures have changed
 *	since the ioctl, so go back and try again (at most MAX_RETRIES
 *	times).  If the read failed with any other error, return SYSCALL_ERROR.
 *	If the read returned less data then expected, retry the ioctl & read.
 *	Return the status from the config_get_table_t (the number of
 *	elements in table).
 * S700
 * Algorithm:
 *	Copy "which_table" into a config_get_table_t.
 *	Call ioctl(2) with config_fd, CONFIG_GET_TABLE and the
 *	config_get_table_t.
 *	Fail if we got an error as follows:
 *	    errno	    function return
 *	    ---------------------------------
 *	    EBADF	    NOT_OPEN
 *	    EPERM	    PERMISSION_DENIED
 *	    other	    SYSCALL_ERROR
 *	Else check the status from the config_get_table_t and return
 *	it if negative (INVALID_TYPE), or 0 (table is empty).
 *	If the status is >0, it indicates the number of table elements,
 *	so malloc(3C) space for them and set table to point to the space.
 *	If the malloc failed, return OUT_OF_MEMORY.
 *	Do a read(2) to get the table.
 *	If the read failed with EAGAIN, the kernel structures have changed
 *	since the ioctl, so go back and try again (at most MAX_RETRIES
 *	times).  If the read failed with any other error, return SYSCALL_ERROR.
 *	If the read returned less data then expected, retry the ioctl & read.
 *	Return the status from the config_get_table_t (the number of
 *	elements in table).
 *
 **************************************************************************/

int
io_get_table(which_table, table)
  unsigned int	which_table;
  void		**table;
{
    config_get_table_t	ioctl_data;
    int try;	    /* counts the number of ioctl/read retries */
    int ret;

    *table = (void *)0;	   /* just to be safe in the event of an error */
    ioctl_data.which_table = which_table;

#ifndef _WSIO
    for (try = 0; try < MAX_TRIES; try++) {
	if (ioctl(config_fd, CONFIG_GET_TABLE, &ioctl_data) == -1)
	    switch(errno) {
		case EBADF:
		    return(NOT_OPEN);
		case EPERM:
	            return(PERMISSION_DENIED);
	        default:
		    return(SYSCALL_ERROR);
	    }

	if (ioctl_data.status <= 0)	/* error or empty table */
	    return(ioctl_data.status);

	if ((*table = malloc(ioctl_data.size)) == (void *)0)
	    return(OUT_OF_MEMORY);

	if ((ret = read(config_fd, (char *)*table, ioctl_data.size)) ==
		    ioctl_data.size)
	    return(ioctl_data.status);
	else if (ret == -1)
	    switch(errno) {
	    case EAGAIN:	    /* structures changed since ioctl */
		free(*table);
		continue;	    /* try ioctl & read again */
	    default:
		free(*table);
	        return(SYSCALL_ERROR);
	    }
	else {		    /* read returned less than expected? */
	    free(*table);
	    continue;	    /* try ioctl & read again? */
        }
    }

    return(STRUCTURES_CHANGED);

#else /* not _WSIO */
    if (ioctl(config_fd, CONFIG_GET_TABLE, &ioctl_data) == -1)
        switch(errno) {
        case EBADF:
            return(NOT_OPEN);
        case EPERM:
            return(PERMISSION_DENIED);
        default:
            return(SYSCALL_ERROR);
        }

    if (ioctl_data.status <= 0)	/* error or empty table */
        return(ioctl_data.status);

    if ((*table = malloc(ioctl_data.size)) == (void *)0)
        return(OUT_OF_MEMORY);

    if ((ret = read(config_fd, (char *)*table, ioctl_data.size)) == -1) 
    {
        free(*table);
	return(SYSCALL_ERROR);
    }
    else
    {
	switch (which_table)
	{
	case T_IO_MOD_TABLE:
	    return(ret / sizeof(return_vsc_mod_entry));
	    break;
	case T_MEM_DATA_TABLE:
	    return(32);	/* 32 memory table entries always */
	    break;
	}
    }
#endif /* not _WSIO */
}

/**************************************************************************
 * io_free_table(which_table, table)
 **************************************************************************
 *
 * Description:
 *	Free the space (pointed to by table) allocated in a previous
 *	call to io_get_table().
 *
 * Input Parameters:
 *	which_table	Which type of table space corresponds to. Ignored.
 *	table		Address of the pointer to the space to free.
 *
 * Output Parameters: None
 *
 * Returns: None
 *
 * Globals Referenced: None
 *
 * External Calls:
 *	free(3C)
 *
 * Algorithm:
 *	Call free(3C) with table as the argument.
 *	Zero out the user's pointer so any (erroneous) subsequent use will
 *	result in a null pointer dereference.
 *
 **************************************************************************/

/*ARGSUSED*/
void
io_free_table(which_table, table)
  unsigned int	which_table;
  void		**table;
{
    free(*table);
    *table = (void *)0;	   /* keep user from using this pointer anymore */
}
