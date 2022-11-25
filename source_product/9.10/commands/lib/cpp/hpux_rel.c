#ifdef __hp9000s300
static char *HPUX_ID = "@(#) B2371B.08.00  C/ANSI C  Internal $Revision: 70.11.1.1 $";
#else
static char *HPUX_ID = "@(#) $Revision: 70.11.1.1 $";
#endif
/* This is an extra file added to multi-file commands and libraries to
   provide Unicorn version control
*/

/*
    $Log:	hpux_rel.c,v $
 * Revision 70.11.1.1  93/11/29  15:44:04  15:44:04  ssa
 * Author: dond@hprdbe.rose.hp.com
 * Rolled version number for 9.03 release
 * 
 * Revision 70.11  92/06/05  17:45:38  17:45:38  ssa (History Manager)
 * Author: klee@hpclbis.cup.hp.com
 * Modified dodef() to issue more informative message if having 'too much defi-
 * ning' or 'too many defines'. Some customers don't know -H option when they
 * see 'too much defining'.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.11
 * 
 * Revision 70.10  92/01/21  18:47:56  18:47:56  ssa (RCS Manager)
 * Author: klee@hpclbis.cup.hp.com
 * Fix for DTS # CLLca01825. cpp barfs on macros with more than 31 formal
 * parameters. We bump max number of formals/actuals to a macro from 31 to 127.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.10
 * 
 * Revision 70.9  92/01/15  17:23:03  17:23:03  ssa (RCS Manager)
 * Author: klee@hpclbis.cup.hp.com
 * A change for the fix for dts CLLca01948. Allocate one more byte ( for null
 * character ) for the warning message.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.9
 * 
 * Revision 70.8  92/01/15  14:00:22  14:00:22  ssa (RCS Manager)
 * Author: klee@hpclbis.cup.hp.com
 * Fix for dts CLLca01948. Enhancement request to put "warning" in front of
 * warning messages. This is consistent with cpp.ansi.
 * Also, made a slightly change to the fix for dts CLLca01923. Now, we issue
 * a warning message if an invalid directive is ifdef'ed out.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.8
 * 
 * Revision 70.7  92/01/14  19:36:53  19:36:53  ssa (RCS Manager)
 * Author: klee@hpclbis.cup.hp.com
 * Fix for dts CLLca01923. Check if flslvl is zero before issue the error
 * message "undefined preprocessor directive" if the directive is ifdef'ed out.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.7
 * 
 * Revision 70.6  91/10/29  19:15:46  19:15:46  ssa (RCS Manager)
 * Author: jbc@hpcllca.cup.hp.com
 * Fix for dts CLLca01945.  Illegal SecOf2 char (following a valid FirstOf2)
 * in a comment or string should generate a warning, not an error.  After
 * all, it is in a comment/string and should be ignored.
 * This is fixed.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.6
 * 
 * Revision 70.5  91/09/12  16:10:23  16:10:23  hmgr (History Manager)
 * Author: curtw@hpisoe4.cup.hp.com
 * Corrected catalog message numbers for the -y and -p options. -klee
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.5
 * 
 * Revision 70.4  91/09/11  12:54:57  12:54:57  hmgr (History Manager)
 * Author: jbc@hpcllca.cup.hp.com
 * Fix for dts CLLca01888.  Added more informative error messages when the
 * fopen fails for the input file.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.4
 * 
 * Revision 70.3  91/09/10  10:22:11  10:22:11  ssa (RCS Manager)
 * Author: curtw@hpcllca.cup.hp.com
 * Two changes for OSF:  (1) '$' no longer alphabetic by default (now like
 * HP-UX); requires "-$" to make it so; (2) Added "-v" option to print out
 * the version string and exit.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.3
 * 
 * Revision 70.2  91/08/21  20:27:55  20:27:55  ssa (Shared Source Administrator)
 * Author: klee@hpcllca.cup.hp.com
 * Added -M, -y, -p, and -h options to support OSF. Code is ifdef'ed. klee
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.2
 * 
 * Revision 70.1  91/08/20  15:15:08  15:15:08  ssa (Shared Source Administrator)
 * Author: curtw@hpisoe4.cup.hp.com
 * Change the LINEBUFSIZE from 2K to 4K to handle large strings.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.1
 * 
 * Revision 66.41  91/04/02  11:31:04  11:31:04  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Modify targets for OSF and PAXDEV.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/make.osf    66.6
 * 
 * Revision 66.40  91/04/02  11:29:05  11:29:05  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Allow #ident only under PAXDEV env.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    66.24
 * 
 * Revision 66.39  91/04/01  16:14:03  16:14:03  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Changed name to cpp.trad; added #ident.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    66.23
 * 
 * Revision 66.38  91/03/28  18:01:48  18:01:48  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Fix the clean operation.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/make.osf    66.5
 * 
 * Revision 66.37  91/03/28  17:47:16  17:47:16  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Add target for OSF and paXdev.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/make.osf    66.4
 * 
 * Revision 66.36  91/03/27  16:45:15  16:45:15  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Modified to make the cmpexe cpp.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/make.osf    66.3
 * 
 * Revision 66.35  91/03/26  16:35:53  16:35:53  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Added support for elif.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    66.22
 * 
 * Revision 66.34  91/03/21  18:01:51  18:01:51  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * Port to OSF.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    66.21
 * 
 * Revision 66.33  91/03/21  17:58:56  17:58:56  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * Port to OSF.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/yylex.c    66.5
 * 
 * Revision 66.32  91/03/21  17:55:09  17:55:09  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * Cleanup on port to OSF.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/make.osf    66.2
 * 
 * Revision 66.31  91/03/21  17:52:55  17:52:55  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * Cleanup on port to OSF.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/makefile    66.11
 * 
 * Revision 66.30  91/03/15  18:04:12  18:04:12  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * Changes for port to OSF.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/yylex.c    66.4
 * 
 * Revision 66.29  91/03/15  18:00:42  18:00:42  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * Changes for port to OSF.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    66.20
 * 
 * Revision 66.28  91/03/15  17:52:46  17:52:46  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * Changes for the port to OSF.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/makefile    66.10
 * 
 * Revision 66.27  90/11/27  16:02:03  16:02:03  bethke
 * Set what string.
 * 
 * Revision 66.26  90/11/13  17:29:55  17:29:55  choang
 * Ifdefed out "extern int[short] passcom;" for appropriate platforms.
 * ******* The Following Modules Were Modified *******
 *     yylex.c,v
 * 
 * Revision 66.25  90/08/23  19:01:20  19:01:20  rsh (R. Scott Holbrook[Ft. Collins])
 * Removed kernpp.  The new C product structure for 8.0 doesn't need it
 * anymore.
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.24  90/08/15  11:29:30  11:29:30  lkc (Lee Casuto)
 * fixed the linking of cpp to use cflags
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.23  90/08/15  11:19:41  11:19:41  lkc (Lee Casuto)
 * commented out CFLAGS since we need to specify it in the database and not
 * have it overwritten.
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.22  90/08/10  09:49:37  09:49:37  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.21  90/08/08  17:28:39  17:28:39  choang
 * Change cpp.c to build a table with $ in it only if -$ option is specified.
 * Otherwise, build the table the "old" way.
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.20  90/07/24  15:43:58  15:43:58  bethke
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.19  90/06/22  14:35:23  14:35:23  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.18  90/06/22  14:06:44  14:06:44  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.17  90/06/04  14:45:42  14:45:42  choang
 * Fixed 2 bugs:
 *  1. include file name padded with garbage when closing " is missing.
 *     (CLLca01567).
 *  2. no warning messages when closing > or " is missing. (CLLca01568).
 * 
 * Revision 66.16  90/03/26  15:32:38  15:32:38  bethke
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.15  90/02/21  14:36:16  14:36:16  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.14  90/02/15  14:55:41  14:55:41  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.13  90/02/13  14:26:17  14:26:17  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.12  90/02/06  18:32:34  18:32:34  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.11  90/01/30  14:57:41  14:57:41  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.10  90/01/15  14:45:44  14:45:44  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     yylex.c,v
 * 
 * Revision 66.9  89/12/15  17:32:15  17:32:15  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.8  89/12/15  17:25:55  17:25:55  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     yylex.c,v
 * 
 * Revision 66.7  89/12/11  16:01:15  16:01:15  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.6  89/09/21  14:36:07  14:36:07  choang
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 66.5  89/09/21  13:54:01  13:54:01  bethke
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 66.4  89/08/29  14:15:09  14:15:09  bethke (Robert Bethke)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 66.3  89/08/29  09:30:02  09:30:02  choang
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 66.2  89/08/28  12:15:55  12:15:55  choang (Christopher Hoang)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 66.1  89/08/08  14:44:32  14:44:32  kah
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 64.32  89/08/01  10:57:08  10:57:08  bethke (Robert Bethke)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.31  89/07/21  09:38:51  09:38:51  bethke (Robert Bethke)
 * The following modules were modified:
 *     yylex.c,v
 * 
 * Revision 64.30  89/07/17  09:17:29  09:17:29  bethke (Robert Bethke)
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 64.29  89/07/12  16:59:11  16:59:11  bethke (Robert Bethke)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.28  89/07/07  12:19:19  12:19:19  bethke (Robert Bethke)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.27  89/06/30  11:29:38  11:29:38  bethke (Robert Bethke)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.26  89/06/30  11:23:32  11:23:32  bethke (Robert Bethke)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.25  89/05/04  10:09:42  10:09:42  choang
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.24  89/04/27  13:57:40  13:57:40  egeland
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 64.23  89/04/21  13:57:48  13:57:48  jimb (Jim Bigelow)
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 64.22  89/04/17  11:10:41  11:10:41  choang
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 64.21  89/04/14  15:28:19  15:28:19  choang (Christopher Hoang)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.20  89/03/27  16:55:52  16:55:52  jimb
 * The following modules were modified:
 *     cpy.y,v
 * 
 * Revision 64.19  89/03/02  17:52:03  17:52:03  choang
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.18  89/02/22  12:44:32  12:44:32  georg
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 64.17  89/02/09  13:30:19  13:30:19  jimb (Jim Bigelow)
 * The following modules were modified:
 *     cpp.mk,v
 * 
 * Revision 64.16  89/02/09  13:11:19  13:11:19  jimb (Jim Bigelow)
 * The following modules were modified:
 *     cpp.c  ( added pragma for 300 series optimizer )
 * 
 * Revision 64.15  89/02/09  12:44:05  12:44:05  jimb (Jim Bigelow)
 * The following modules were modified:
 *     cpp.mk,v
 * 
 * Revision 64.14  89/02/09  10:25:29  10:25:29  jimb (Jim Bigelow)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.13  89/02/06  15:08:01  15:08:01  claudia (Claudia Luzzi)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.12  89/02/06  14:06:01  14:06:01  jimb
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.11  89/02/06  14:03:16  14:03:16  jimb (Jim Bigelow)
 * The following modules were modified:
 *     cpp.mk,v
 * 
 * Revision 64.10  89/02/02  11:38:41  11:38:41  jimb (Jim Bigelow)
 * added $Log:	hpux_rel.c,v $
 * Revision 70.11.1.1  93/11/29  15:44:04  15:44:04  ssa
 * Author: dond@hprdbe.rose.hp.com
 * Rolled version number for 9.03 release
 * 
 * Revision 70.11  92/06/05  17:45:38  17:45:38  ssa (History Manager)
 * Author: klee@hpclbis.cup.hp.com
 * Modified dodef() to issue more informative message if having 'too much defi-
 * ning' or 'too many defines'. Some customers don't know -H option when they
 * see 'too much defining'.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.11
 * 
 * Revision 70.10  92/01/21  18:47:56  18:47:56  ssa (RCS Manager)
 * Author: klee@hpclbis.cup.hp.com
 * Fix for DTS # CLLca01825. cpp barfs on macros with more than 31 formal
 * parameters. We bump max number of formals/actuals to a macro from 31 to 127.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.10
 * 
 * Revision 70.9  92/01/15  17:23:03  17:23:03  ssa (RCS Manager)
 * Author: klee@hpclbis.cup.hp.com
 * A change for the fix for dts CLLca01948. Allocate one more byte ( for null
 * character ) for the warning message.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.9
 * 
 * Revision 70.8  92/01/15  14:00:22  14:00:22  ssa (RCS Manager)
 * Author: klee@hpclbis.cup.hp.com
 * Fix for dts CLLca01948. Enhancement request to put "warning" in front of
 * warning messages. This is consistent with cpp.ansi.
 * Also, made a slightly change to the fix for dts CLLca01923. Now, we issue
 * a warning message if an invalid directive is ifdef'ed out.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.8
 * 
 * Revision 70.7  92/01/14  19:36:53  19:36:53  ssa (RCS Manager)
 * Author: klee@hpclbis.cup.hp.com
 * Fix for dts CLLca01923. Check if flslvl is zero before issue the error
 * message "undefined preprocessor directive" if the directive is ifdef'ed out.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.7
 * 
 * Revision 70.6  91/10/29  19:15:46  19:15:46  ssa (RCS Manager)
 * Author: jbc@hpcllca.cup.hp.com
 * Fix for dts CLLca01945.  Illegal SecOf2 char (following a valid FirstOf2)
 * in a comment or string should generate a warning, not an error.  After
 * all, it is in a comment/string and should be ignored.
 * This is fixed.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.6
 * 
 * Revision 70.5  91/09/12  16:10:23  16:10:23  hmgr (History Manager)
 * Author: curtw@hpisoe4.cup.hp.com
 * Corrected catalog message numbers for the -y and -p options. -klee
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.5
 * 
 * Revision 70.4  91/09/11  12:54:57  12:54:57  hmgr (History Manager)
 * Author: jbc@hpcllca.cup.hp.com
 * Fix for dts CLLca01888.  Added more informative error messages when the
 * fopen fails for the input file.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.4
 * 
 * Revision 70.3  91/09/10  10:22:11  10:22:11  ssa (RCS Manager)
 * Author: curtw@hpcllca.cup.hp.com
 * Two changes for OSF:  (1) '$' no longer alphabetic by default (now like
 * HP-UX); requires "-$" to make it so; (2) Added "-v" option to print out
 * the version string and exit.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.3
 * 
 * Revision 70.2  91/08/21  20:27:55  20:27:55  ssa (Shared Source Administrator)
 * Author: klee@hpcllca.cup.hp.com
 * Added -M, -y, -p, and -h options to support OSF. Code is ifdef'ed. klee
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.2
 * 
 * Revision 70.1  91/08/20  15:15:08  15:15:08  ssa (Shared Source Administrator)
 * Author: curtw@hpisoe4.cup.hp.com
 * Change the LINEBUFSIZE from 2K to 4K to handle large strings.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    70.1
 * 
 * Revision 66.41  91/04/02  11:31:04  11:31:04  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Modify targets for OSF and PAXDEV.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/make.osf    66.6
 * 
 * Revision 66.40  91/04/02  11:29:05  11:29:05  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Allow #ident only under PAXDEV env.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    66.24
 * 
 * Revision 66.39  91/04/01  16:14:03  16:14:03  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Changed name to cpp.trad; added #ident.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    66.23
 * 
 * Revision 66.38  91/03/28  18:01:48  18:01:48  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Fix the clean operation.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/make.osf    66.5
 * 
 * Revision 66.37  91/03/28  17:47:16  17:47:16  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Add target for OSF and paXdev.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/make.osf    66.4
 * 
 * Revision 66.36  91/03/27  16:45:15  16:45:15  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Modified to make the cmpexe cpp.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/make.osf    66.3
 * 
 * Revision 66.35  91/03/26  16:35:53  16:35:53  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Added support for elif.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    66.22
 * 
 * Revision 66.34  91/03/21  18:01:51  18:01:51  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * Port to OSF.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    66.21
 * 
 * Revision 66.33  91/03/21  17:58:56  17:58:56  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * Port to OSF.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/yylex.c    66.5
 * 
 * Revision 66.32  91/03/21  17:55:09  17:55:09  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * Cleanup on port to OSF.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/make.osf    66.2
 * 
 * Revision 66.31  91/03/21  17:52:55  17:52:55  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * Cleanup on port to OSF.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/makefile    66.11
 * 
 * Revision 66.30  91/03/15  18:04:12  18:04:12  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * Changes for port to OSF.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/yylex.c    66.4
 * 
 * Revision 66.29  91/03/15  18:00:42  18:00:42  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * Changes for port to OSF.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/cpp.c    66.20
 * 
 * Revision 66.28  91/03/15  17:52:46  17:52:46  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * Changes for the port to OSF.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/cpp/makefile    66.10
 * 
 * Revision 66.27  90/11/27  16:02:03  16:02:03  bethke
 * Set what string.
 * 
 * Revision 66.26  90/11/13  17:29:55  17:29:55  choang
 * Ifdefed out "extern int[short] passcom;" for appropriate platforms.
 * ******* The Following Modules Were Modified *******
 *     yylex.c,v
 * 
 * Revision 66.25  90/08/23  19:01:20  19:01:20  rsh (R. Scott Holbrook[Ft. Collins])
 * Removed kernpp.  The new C product structure for 8.0 doesn't need it
 * anymore.
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.24  90/08/15  11:29:30  11:29:30  lkc (Lee Casuto)
 * fixed the linking of cpp to use cflags
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.23  90/08/15  11:19:41  11:19:41  lkc (Lee Casuto)
 * commented out CFLAGS since we need to specify it in the database and not
 * have it overwritten.
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.22  90/08/10  09:49:37  09:49:37  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.21  90/08/08  17:28:39  17:28:39  choang
 * Change cpp.c to build a table with $ in it only if -$ option is specified.
 * Otherwise, build the table the "old" way.
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.20  90/07/24  15:43:58  15:43:58  bethke
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.19  90/06/22  14:35:23  14:35:23  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.18  90/06/22  14:06:44  14:06:44  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.17  90/06/04  14:45:42  14:45:42  choang
 * Fixed 2 bugs:
 *  1. include file name padded with garbage when closing " is missing.
 *     (CLLca01567).
 *  2. no warning messages when closing > or " is missing. (CLLca01568).
 * 
 * Revision 66.16  90/03/26  15:32:38  15:32:38  bethke
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.15  90/02/21  14:36:16  14:36:16  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.14  90/02/15  14:55:41  14:55:41  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.13  90/02/13  14:26:17  14:26:17  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.12  90/02/06  18:32:34  18:32:34  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.11  90/01/30  14:57:41  14:57:41  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.10  90/01/15  14:45:44  14:45:44  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     yylex.c,v
 * 
 * Revision 66.9  89/12/15  17:32:15  17:32:15  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.8  89/12/15  17:25:55  17:25:55  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     yylex.c,v
 * 
 * Revision 66.7  89/12/11  16:01:15  16:01:15  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cpp.c,v
 * 
 * Revision 66.6  89/09/21  14:36:07  14:36:07  choang
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 66.5  89/09/21  13:54:01  13:54:01  bethke
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 66.4  89/08/29  14:15:09  14:15:09  bethke (Robert Bethke)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 66.3  89/08/29  09:30:02  09:30:02  choang
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 66.2  89/08/28  12:15:55  12:15:55  choang (Christopher Hoang)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 66.1  89/08/08  14:44:32  14:44:32  kah
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 64.32  89/08/01  10:57:08  10:57:08  bethke (Robert Bethke)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.31  89/07/21  09:38:51  09:38:51  bethke (Robert Bethke)
 * The following modules were modified:
 *     yylex.c,v
 * 
 * Revision 64.30  89/07/17  09:17:29  09:17:29  bethke (Robert Bethke)
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 64.29  89/07/12  16:59:11  16:59:11  bethke (Robert Bethke)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.28  89/07/07  12:19:19  12:19:19  bethke (Robert Bethke)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.27  89/06/30  11:29:38  11:29:38  bethke (Robert Bethke)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.26  89/06/30  11:23:32  11:23:32  bethke (Robert Bethke)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.25  89/05/04  10:09:42  10:09:42  choang
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.24  89/04/27  13:57:40  13:57:40  egeland
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 64.23  89/04/21  13:57:48  13:57:48  jimb (Jim Bigelow)
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 64.22  89/04/17  11:10:41  11:10:41  choang
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 64.21  89/04/14  15:28:19  15:28:19  choang (Christopher Hoang)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.20  89/03/27  16:55:52  16:55:52  jimb
 * The following modules were modified:
 *     cpy.y,v
 * 
 * Revision 64.19  89/03/02  17:52:03  17:52:03  choang
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.18  89/02/22  12:44:32  12:44:32  georg
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 64.17  89/02/09  13:30:19  13:30:19  jimb (Jim Bigelow)
 * The following modules were modified:
 *     cpp.mk,v
 * 
 * Revision 64.16  89/02/09  13:11:19  13:11:19  jimb (Jim Bigelow)
 * The following modules were modified:
 *     cpp.c  ( added pragma for 300 series optimizer )
 * 
 * Revision 64.15  89/02/09  12:44:05  12:44:05  jimb (Jim Bigelow)
 * The following modules were modified:
 *     cpp.mk,v
 * 
 * Revision 64.14  89/02/09  10:25:29  10:25:29  jimb (Jim Bigelow)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.13  89/02/06  15:08:01  15:08:01  claudia (Claudia Luzzi)
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.12  89/02/06  14:06:01  14:06:01  jimb
 * The following modules were modified:
 *     cpp.c,v
 * 
 * Revision 64.11  89/02/06  14:03:16  14:03:16  jimb (Jim Bigelow)
 * The following modules were modified:
 *     cpp.mk,v
 *  rcs symbol to file so that it will contain a record of what's
 * gone on.
 * added -A for POSIX name space, added new predefines for 7.0, updated #ifdefs
 * to #if defines, added new predefines to add hase out of non-POSIX defines
 * 
 */
