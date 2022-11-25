/*
 * This file logs changes to the SCCS system
 * $Revision: 66.6.1.2 $
 * $Log:	hpux_rel.c,v $
 * Revision 66.6.1.2  94/12/16  13:27:24  13:27:24  hmgr
 * Author: cbarth@bartman.fc.hp.com
 * Fix corresponding to COSL revision 74.1
 * Fix for DSDe421771. Changed interpretation of date to read 00-69 as
 * 2000-2069 in the year ( yy ) variable.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/src.rcs/usr/bin/sccs/lib/comobj/date_ab.c    70.1
 * 
 * Revision 66.6.1.1  94/12/16  12:57:00  12:57:00  hmgr (History Manager)
 * Author: cbarth@bartman.fc.hp.com
 * Fix corresponding to COSL revision 72.2
 * fixed corrupt header problem in SCCS library when year > 2000. DSDe408392
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/src.rcs/usr/bin/sccs/lib/comobj/date_ba.c    70.1
 * 
 * Revision 66.6  90/06/15  16:44:15  16:44:15  rer
 * DTS FSDlj04054
 * "findmsg misses "nl_msg (47" because of space."
 * removed space!
 * ******* The Following Modules Were Modified *******
 *     cmd/admin.c,v
 * 
 * Revision 66.5  90/05/11  11:19:51  11:19:51  egeland (Mark Egeland)
 * Added "rm -f sccs.msg" and "rm -f sccs.cat" to clean and clobber targets
 * (respectively) so that "make clean" and "make clobber" would work as
 * advertised.
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.4  90/05/07  09:58:23  09:58:23  ec (Edgar_Circenis)
 * Fix for FSDlj03347.  Changed "stuck" entry to reference ERRNO(2) instead
 * of INTRO(2).
 * ******* The Following Modules Were Modified *******
 *     help.d/cmds,v
 * 
 * Revision 66.3  90/05/07  09:49:00  09:49:00  ec (Edgar_Circenis)
 * Changed message for ut12.  It is not due to an intermittent hardware
 * error, but is due to broken pipes.  FSDlj03388
 * ******* The Following Modules Were Modified *******
 *     help.d/ut,v
 * 
 * Revision 66.2  90/05/07  09:22:54  09:22:54  rsh (R Scott Holbrook)
 * Use libBUILD instead of /lib/toolong.o
 * ******* The Following Modules Were Modified *******
 *     cmd/makefile,v
 * 
 * Revision 66.1  90/05/04  13:06:27  13:06:27  rer (Rob Robason)
 * The following files have already been modified for 8.0 at level
 * 66 prior to the creation of this hpux_rel.c file:
 * /hpux/shared/supp/usr/src/cmd/sccs/makefile,v 66.1
 * /hpux/shared/supp/usr/src/cmd/sccs/cmd/val.c,v 66.2
 * /hpux/shared/supp/usr/src/cmd/sccs/cmd/comb.c,v 66.1
 * /hpux/shared/supp/usr/src/cmd/sccs/help.d/cmds,v 66.1
 * /hpux/shared/supp/usr/src/cmd/sccs/help.d/co,v 66.2
 * /hpux/shared/supp/usr/src/cmd/sccs/help.d/val,v 66.1
 * /hpux/shared/supp/usr/src/cmd/sccs/lib/comobj/putline.c,v 66.1
 * 
 */
