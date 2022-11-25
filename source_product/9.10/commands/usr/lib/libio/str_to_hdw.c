/* $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libio/str_to_hdw.c,v $
 * $Revision: 70.1 $        $Author: ssa $
 * $State: Exp $        $Locker:  $
 * $Date: 92/03/31 15:36:02 $
 */

#include <sys/libio.h>
#include <string.h>

#define TRUE	1
#define FALSE	0

/*
 * Library routine to support other libio.a functions.  Included is:
 *
 * string_to_hdw_path - Convert a hardware path string to a hdw_path_type
 *			structure.
 *
 * WARNING: This routine is provided "as is", solely for use by
 *	    insf(1m), ioscan(1m), mksf(1m), and rmsf(1m).
 */

/**************************************************************************
 * string_to_hdw_path(hdw_path, string)
 **************************************************************************
 *
 * Description:
 *	Convert an input hardware path string into a hdw_path_type structure.
 *
 * Input Parameters:
 *	string	    The string to convert.
 *
 * Output Parameters:
 *	hdw_path    Pointer to a hdw_path_type structure which is filled
 *		    in with the hardware path..
 *
 * Returns:
 *	SUCCESS (0)		Success.
 *   Errors (<0):
 *	BAD_ADDRESS_FORMAT	Invalid address found in the input string.
 *	LENGTH_OUT_OF_RANGE	The hardware path length is invalid.  A valid
 *				length is between 1 and MAX_IO_PATH_ELEMENTS.
 *
 * Globals Referenced:	None.
 *
 * External Calls:
 *	strtol(3C)
 *
 * Algorithm:
 *	Set hdw_path->num_elements to 0 and the "saw_dot" flag to FALSE.
 *	Do the following loop (at least once):
 *	    If num_elements exceeds MAX_IO_PATH_ELEMENTS, return
 *	    LENGTH_OUT_OF_RANGE.
 *	    Call strtol() to convert the next address element and store it
 *	    in the next available slot in hdw_path->addr.
 *	    If the element is invalid, return BAD_ADDRESS_FORMAT.
 *	    An invalid element is indicated by any of:
 *		1) strtol() returns with the terminating character pointer
 *		   (ptr) equal to the input string pointer (string) (meaning
 *		   that no integer could be formed),
 *		2) the character pointed to by ptr is not the NULL ('\0'),
 *		   a dot ('.'), or a slash ('/') (slash is allowed
 *		   only if we have not yet seen a dot),
 *		3) the address is < 0,
 *		4) the address corresponds to native hardware (!saw_dot)
 *		   and it's >= NUM_MODS_PER_BUS.
 *	    Increment num_elements.
 *	    If the terminating character was a dot, set saw_dot to TRUE.
 *	    Set string to ptr + 1 to prepare to convert the next element.
 *	While we haven't reached the terminating NULL character.
 *	Return SUCCESS (all errors returned earlier).
 *
 **************************************************************************/

int
string_to_hdw_path(hdw_path, string)
  hdw_path_type	*hdw_path;
  char		*string;
{
    int		saw_dot = FALSE;
    char	*ptr;
    extern long	strtol();

    hdw_path->num_elements = 0;

    do {
	if (hdw_path->num_elements >= MAX_IO_PATH_ELEMENTS)
	    return(LENGTH_OUT_OF_RANGE);

	hdw_path->addr[hdw_path->num_elements] = (int)strtol(string, &ptr, 0);

#ifdef __hp9000s700
	if (ptr == string ||
		!(*ptr == '\0' || *ptr == '.' || (!saw_dot && *ptr == '/')) ||
		hdw_path->addr[hdw_path->num_elements] < 0 ||
		(!saw_dot &&
		hdw_path->addr[hdw_path->num_elements] >= NUM_MODS_PER_BUS))
#else
	if( (ptr == string ) ||
            ((*ptr != '\0') && (*ptr != '.')) ||
	    ( hdw_path->addr[hdw_path->num_elements] < 0 ) )
#endif
	    return(BAD_ADDRESS_FORMAT);

	hdw_path->num_elements++;

#ifdef __hp9000s700
	if (*ptr == '.')
	    saw_dot = TRUE;
#endif

	string = ptr + 1;
    } while (*ptr != '\0');

    return(SUCCESS);
}
