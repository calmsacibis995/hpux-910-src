/* $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libio/io_isc.c,v $
 * $Revision: 70.2 $        $Author: ssa $
 * $State: Exp $        $Locker:  $
 * $Date: 92/04/07 15:00:53 $
 */

/* AUTOBAHN: fix these when files are put in right place */
#include <sys/libio.h>
#include <sys/io.h>
#include <machine/dconfig.h>

/*
 * Library routines to manipulate /dev/config. Included are:
 *
 * io_search_isc    - Search the isc_table for entries matching a criteria.
 */

/* Global variables */

extern int  config_fd;	    /* file descriptor for /dev/config */

/**************************************************************************
 * io_search_isc(search_type, search_key, search_qual, isc_entry)
 **************************************************************************
 *
 * Description:
 *	Search the isc_table for entries matching the search criteria
 *	specified in search_type, search_key, search_qual, and isc_entry,
 *	and return information about the entry in isc_entry.  The space for
 *	the isc_entry must be allocated by the caller.
 *	Requires read permission on /dev/config.
 *
 * Input Parameters:
 *	search_type	Type of search to perform.  One of:
 *		SEARCH_FIRST
 *		    Find the first entry matching the criteria and return it
 *		    in isc_entry along with the number of additional entries
 *		    which match.
 *		SEARCH_NEXT
 *		    Find the next entry matching the criteria and return it
 *		    in isc_entry along with the number of remaining entries
 *		    which match.  There must be a SEARCH_FIRST before any
 *		    calls with SEARCH_NEXT.
 *		SEARCH_SINGLE
 *		    Find the first occurance of any entry matching the
 *		    criteria and return it in isc_entry.  These may be freely
 *		    interspersed with SEARCH_FIRST and SEARCH_NEXT calls.
 *
 *	search_key	The value to match on.  One of:
 *		KEY_HDW_PATH
 *		    Match all entries that have the hardware path specified in
 *		    isc_entry->hdw_path.
 *		KEY_MY_ISC
 *		    Match the entry with the entry id in isc_entry->my_isc
 *		KEY_BUS_TYPE
 *		    Match the entry with the entry id in isc_entry->bus_type.
 *		KEY_IF_ID
 *		    Match the entry with the entry id in isc_entry->if_id.
 *
 *	search_qual	Qualifies the search.  Subsets the entries which match
 *			the seach key.  If no qualifier is desired, set 
 *                      search_qual to QUAL_NONE.
 *			Bits that can be ORed together in any combination:
 *		QUAL_INIT
 *		    Match only entries that are currently initialized.
 *		QUAL_FTN_NO
 *		    Match only entries that have a function number which 
 *                  equals isc_entry->ftn_no.
 *	isc_entry       Pointer to an isc_entry having the fields relevant
 *		    to a particular search_key and search_qual filled in.
 *		    The input fields are only used by SEARCH_FIRST and
 *		    SEARCH_SINGLE; they are ignored for SEARCH_NEXT.
 *
 * Output Parameters:
 *	isc_entry	    Pointer to an isc_entry which is filled in with
 *		    the info from the matched entry.
 *
 * Returns:
 *	>= 0			The number of additional entries matching
 *				the search criteria.
 *   Errors (<0):
 *	INVALID_KEY		The search_key argument is invalid.
 *	INVALID_NODE_DATA	The search_key- or search_qual-specific
 *				input data in isc_entry is invalid.
 *	INVALID_QUAL		The search_qual argument is invalid.
 *	INVALID_TYPE		The search_type argument is invalid.
 *	NO_MATCH		No isc entries match the search criteria.
 *	NO_SEARCH_FIRST		SEARCH_NEXT was used before SEARCH_FIRST.
 *	NOT_OPEN		/dev/config is not open.
 *	OUT_OF_MEMORY		No kernel memory available to do search.
 *	PERMISSION_DENIED	/dev/config is not open read/write.
 *	SYSCALL_ERROR		Non-specific system call error - see errno.
 *
 * Globals Referenced:
 *	config_fd	The /dev/config file descriptor.
 *	errno		System call error number.
 *
 * External Calls:
 *	ioctl(2)
 *
 * Algorithm:
 *	Copy the search_type, search_key, search_qual, and the isc_entry
 *	structure into a config_search_isc_t.
 *	Call ioctl(2) with config_fd, CONFIG_SEARCH_ISC and the
 *	config_search_isc_t.
 *	Fail if we got an error as follows:
 *	    errno	    function return
 *	    ---------------------------------
 *	    EBADF	    NOT_OPEN
 *	    EPERM	    PERMISSION_DENIED
 *	    other	    SYSCALL_ERROR
 *	Else copy the isc_entry structure from the config_search_isc_t to
 *	the output parameter and return the status (INVALID_KEY,
 *	INVALID_NODE_DATA, INVALID_QUAL, INVALID_TYPE, NO_MATCH, or
 *	NO_SEARCH_FIRST) from that structure.
 *
 **************************************************************************/

int
io_search_isc(search_type, search_key, search_qual, isc_entry)
  unsigned int		search_type,
			search_key,
			search_qual;
  isc_entry_type	*isc_entry;
{
    config_search_isc_t    ioctl_data;

    ioctl_data.search_type = search_type;
    ioctl_data.search_key = search_key;
    /*
     *  The field search_qualify is invalid for 300/400 so
     *  lets default to to none.
     */
#ifdef __hp9000s700
    ioctl_data.search_qual = search_qual;
#else
    ioctl_data.search_qual = QUAL_NONE;
#endif
    ioctl_data.isc_entry = *isc_entry;

    if (ioctl(config_fd, CONFIG_SEARCH_ISC, &ioctl_data) == -1)
	switch(errno) {
	    case EBADF:
		return(NOT_OPEN);
	    case EPERM:
		return(PERMISSION_DENIED);
	    default:
		return(SYSCALL_ERROR);
	}

    *isc_entry = ioctl_data.isc_entry;
    /*
    ** At this point need some way to get major numbers based on the
    ** information available in the isc_entry.  Possibly read
    ** /etc/master looking for handle based on if_id?  YUK.
    ** Maybe hardcode major numbers based on if_id? DOUBLE YUK.
    */
    return(ioctl_data.status);
}
