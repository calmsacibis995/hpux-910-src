/* @(#) $Revision: 27.1 $ */   
#include "curses.ext"

/*
 * Set the file descriptor for typeahead checks to fd.  fd can be -1
 * to disable the checking.
 */
typeahead(fd)
int fd;
{
	SP->check_fd = fd;
}
