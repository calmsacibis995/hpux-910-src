/* $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libio/hdw_to_str.c,v $
 * $Revision: 70.1 $        $Author: ssa $
 * $State: Exp $        $Locker:  $
 * $Date: 92/03/31 15:33:24 $
 */

#include <sys/libio.h>
#include <string.h>

/*
 * Library routine to support other libio.a functions.  Included is:
 *
 * hdw_path_to_string - Convert a hdw_path_type structure to a hardware path
 *			string.
 *
 * WARNING: This routine is provided "as is", solely for use by
 *	    insf(1m), ioscan(1m), mksf(1m), and rmsf(1m).
 */

#define MAX_INT_LEN 10	/* maximum number of chars in an int */

/**************************************************************************
 * hdw_path_to_string(hdw_path, num_native)
 **************************************************************************
 *
 * Description:
 *	Convert an input hdw_path_type structure to a hardware path string.
 *	The num_native parameter specifies how many of the hardware
 *	elements are native modules and should be preceded by '/' instead
 *	of '.' in the string.
 *
 *	WARNING:  The return value points to static data whose content is
 *		  overwritten by each call.
 *
 * Input Parameters:
 *	hdw_path	The hdw_path_type to convert.
 *
 *	num_native	How many of the path elements correspond to
 *			native hardware modules.  Specifying 0 or 1 gives
 *			a string with no '/' separators in it.
 *
 * Output Parameters:	None.
 *
 * Returns:
 *	NULL		The hdw_path had num_elements <= 0.
 *   	otherwise	A pointer to a static buffer containing the
 *			converted string.
 *
 * Globals Referenced:	None.
 *
 * External Calls:
 *	sprintf(3C)
 *
 * Algorithm:
 *	If hdw_path->num_elements is <= 0, return NULL.
 *	Otherwise, use sprintf() to convert the first path element to
 *	a string and store it in the static hdw_buf array.  Set n to the
 *	length of the string (returned by sprintf()) so we can sprintf()
 *	the next element onto the end of the string.
 *	For each remaining path element (1 to num_elements-1):
 *	    If this element is less than num_native, sprintf() '/' and
 *	    the string for that element into hdw_buf at position "n", and
 *	    increment n by the length of the string (returned by sprintf()).
 *	    Otherwise (past num_native), sprintf() '.' and the number.
 *	Return a pointer to hdw_buf.
 *
 **************************************************************************/

char *
hdw_path_to_string(hdw_path, num_native)
  hdw_path_type	*hdw_path;
  int		num_native;
{
    int		i, n;
    static char hdw_buf[MAX_IO_PATH_ELEMENTS*(MAX_INT_LEN+1)+1];

    if (hdw_path->num_elements <= 0)
	return((char *)0);

    n = sprintf(hdw_buf, "%d", hdw_path->addr[0]);

    for (i = 1; i < hdw_path->num_elements; i++)
	n += sprintf(&hdw_buf[n], "%c%d", (i < num_native) ? '/' : '.',
	    hdw_path->addr[i]);

    return(hdw_buf);
}
