/* $Source: /misc/source_product/9.10/commands.rcs/etc/meas_sys/note.h,v $
 * $Revision: 1.5 $	$Author: dah $
 * $State: Exp $   	$Locker:  $
 * $Date: 86/01/27 17:00:18 $
 *
 * $Log:	note.h,v $
 * Revision 1.5  86/01/27  17:00:18  17:00:18  dah
 * make it more ident'able
 * 
 * Revision 1.4  85/12/06  16:56:12  dah (Dave Holt)
 * try to get it all to work together
 * 
 * Revision 1.3  85/12/06  16:42:29  dah (Dave Holt)
 * macros needed to use note.c
 * 
 * Revision 1.2  85/12/06  16:42:05  dah (Dave Holt)
 * Header added.  
 * 
 * $Endlog$
 */

/*
 * $Header: note.h,v 1.5 86/01/27 17:00:18 dah Exp $
 */
extern int noting;

#define note(x) if (noting) note_time(x)
#define post_note() if (noting) note_posts();

