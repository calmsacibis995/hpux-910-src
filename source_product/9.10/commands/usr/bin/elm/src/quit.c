/**			quit.c				**/

/*
 *  @(#) $Revision: 70.1 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  quit: leave the current mailbox and quit the program.
 */


#include "headers.h"


long            bytes();


int
quit()

{
	/*
	 * a wonderfully short routine!! 
	 */

	if ( leave_mbox(1) == -1 )
		return;			/* new mail!  (damn it)  resync */
					/* or cancel operation */
	leave( 0 );
}


int
resync()

{
	int rc;
	/*
	 *  Resync on the current mailbox... This is simple: simply call
	 * 'newmbox' to read the file in again, set the size (to avoid
	 *  confusion in the main loop) and refresh the screen!
	 */

	if ((rc = leave_mbox(0)) == -1)	/* new mail or cancel */
		return;			/* can't resync yet!  */

	if (rc == 2) {
		/* mail file was deleted so revert to default file */
		infile[0] = '\0';
		mbox_specified = 0;
		last_header_page = -1;
		mailfile_size = 0;
		setresgid(-1, egroupid, egroupid);
		newmbox(1, FALSE, TRUE);
	} else
		newmbox(3, TRUE, TRUE );
	showscreen();
}
