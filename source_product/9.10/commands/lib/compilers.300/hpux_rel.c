#ifdef ANSI
static char *HPUX_ID = "@(#) B2371B.08.37  ANSI C  Internal $Revision: 72.24 $";
#else
static char *HPUX_ID = "@(#) B2371B.08.37  C  Internal $Revision: 72.24 $";
#endif
/*
    $Log:	hpux_rel.c,v $
 * Revision 72.24  94/12/13  09:04:07  09:04:07  hmgr
 * Author: dond@hprdbe.rose.hp.com
 * Fix for SWFfc00674. Various problems when braces omitted from aggregate
 * initializers.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/src.rcs/lib/compilers.300/pftn.c    72.6
 * 
 * Revision 72.23  94/11/02  16:46:11  16:46:11  hmgr (History Manager)
 * Author: dond@hprdbe.rose.hp.com
 * Updated version id.
 * 
 * Revision 72.22  94/11/02  16:40:58  16:40:58  hmgr (History Manager)
 * Author: dond@hprdbe.rose.hp.com
 * Fix for SWFfc00726 (SR # 5003-170282).The compiler has a problem with
 * tentative definitions for incomplete types when the type category
 * of the declared type is derived from type "void".
 * 
 * The files changed for this fix:
 * code.c
 * cput.c
 * local.c
 * pftn.c
 * trees.c
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/src.rcs/lib/compilers.300/code.c    70.5
 * /hpux/src.rcs/lib/compilers.300/cput.c    70.4
 * /hpux/src.rcs/lib/compilers.300/local.c    70.4
 * /hpux/src.rcs/lib/compilers.300/pftn.c    72.4
 * /hpux/src.rcs/lib/compilers.300/trees.c    70.10
 * 
 * Revision 72.21  94/11/02  16:23:58  16:23:58  hmgr (History Manager)
 * Author: dond@hprdbe.rose.hp.com
 * Fix for CLL4600018. Confusing error message with duplicate
 * static functions. 
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/src.rcs/lib/compilers.300/pftn.c    72.3
 * 
 * Revision 72.20  94/11/02  15:55:37  15:55:37  hmgr (History Manager)
 * Author: dond@hprdbe.rose.hp.com
 * Fix for SWFfc00992. The ANSI c compiler is not processing const
 * correctly.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/src.rcs/lib/compilers.300/trees.c    70.9
 * 
 * Revision 72.19  94/11/02  15:33:12  15:33:12  hmgr (History Manager)
 * Author: dond@hprdbe.rose.hp.com
 * Fix for SWFfc01090 (SR #1653100826). The user is requesting S700/S800
 * alignment for ints, float word aligned (4 bytes), doubles are
 * double-word (8 bytes) aligned and strucs my be byte aligned depending on
 * the data types used within the structure ( HPUX_NATURAL ).
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/src.rcs/lib/compilers.300/order.c    70.6
 * 
 * Revision 72.18  94/11/02  15:20:02  15:20:02  hmgr (History Manager)
 * Author: dond@hprdbe.rose.hp.com
 * Fix for CLL4600054 (SR #1653038844). Ccom.ansi is getting confused if the
 * name of a source file contains ccom or cpass1.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/src.rcs/lib/compilers.300/code.c    70.4
 * 
 * Revision 72.17  94/11/02  15:06:06  15:06:06  hmgr (History Manager)
 * Author: dond@hprdbe.rose.hp.com
 * fix for SWFfc00653. Struct and field information being placed into the
 * GNTT instead of the LNTT table.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/src.rcs/lib/compilers.300/cdbsyms.c    70.7
 * 
 * Revision 72.16  94/11/02  14:59:49  14:59:49  hmgr (History Manager)
 * Author: dond@hprdbe.rose.hp.com
 * Fix for FSDdt07079. Compiler aborts with an referenced but undefined label
 * in the gntt when the -g option is invoked.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/src.rcs/lib/compilers.300/cdbsyms.c    70.6
 * 
 * Revision 72.15  94/11/02  14:39:08  14:39:08  hmgr (History Manager)
 * Author: dond@hprdbe.rose.hp.com
 * fix for SWFfc00660 (SR #1653071654). The C compiler may report
 * various errors and fail to compile a valid program under the
 * following circumstances:
 *      1.  The -y option is specified (for static analysis information);
 *          and
 *      2.  The program is compiled in ANSI mode; and
 *      3.  The program contains a string literal that is followed by a
 *          macro invocation.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/src.rcs/lib/compilers.300/scan.c    70.13
 * 
 * Revision 72.14  93/12/22  08:18:51  08:18:51  ssa (shared source admin)
 * Author: dond@hprdbe.rose.hp.com
 * Corrects defect SWFfc00722. The problem had to do with a newline not being
 * generated when a include file contained no tokens was processed.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/scan.c    70.12
 * 
 * Revision 72.13  93/11/12  13:46:19  13:46:19  ssa (History Manager)
 * Author: dond@hprdbe.rose.hp.com
 * Rolled version number for CLL4600056 defect fix.
 * 
 * Revision 72.12  93/11/12  13:39:52  13:39:52  ssa (History Manager)
 * Author: dond@hprdbe.rose.hp.com
 * CLL4600056 defect fix. The language for numeric data is now forced to C.
 * This corrects the problem found using the atof functions.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/scan.c    70.11
 * 
 * Revision 72.11  93/11/12  13:22:17  13:22:17  ssa (History Manager)
 * Author: dond@hprdbe.rose.hp.com
 * Code changes made to correctly generate warnings for attempting to
 * promte block level externs to file scope. Corrects previous NIST fix.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/mfile1    70.5
 * 
 * Revision 72.10  93/11/12  13:16:38  13:16:38  ssa (History Manager)
 * Author: dond@hprdbe.rose.hp.com
 * Code changes made to correctly generate warnings for attempting to
 * promte block level externs to file scope. Corrects previous NIST fix.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cgram.y    70.7
 * 
 * Revision 72.9  93/11/12  13:08:46  13:08:46  ssa (History Manager)
 * Author: dond@hprdbe.rose.hp.com
 * Code changes made to correctly generate warnings for attempting to
 * promte block level externs to file scope. Corrects previous NIST fix.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/pftn.c    72.2
 * 
 * Revision 72.8  92/11/18  10:29:49  10:29:49  ssa (History Manager)
 * Author: asok@hpcll46.cup.hp.com
 * No changes
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/K_rel_ccom.c    72.1
 * 
 * Revision 72.7  92/11/18  10:28:29  10:28:29  ssa (RCS Manager)
 * Author: asok@hpcll46.cup.hp.com
 * Version is changed to reflect B8.35 for NIST, ANSI version.
 * 
 * Revision 72.6  92/09/23  11:06:09  11:06:09  ssa (RCS Manager)
 * Author: asok@hpcll46.cup.hp.com
 *  Fix to ANSI conformance problem, DTS#FSDdt08969. (integral promotion)
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/trees.c    70.7
 * 
 * Revision 72.5  92/09/23  10:36:07  10:36:07  ssa (RCS Manager)
 * Author: asok@hpcll46.cup.hp.com
 *  Fix to ANSI conformance problem, DTS#FSDdt08969.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/scan.c    70.10
 * 
 * Revision 72.4  92/09/17  13:56:29  13:56:29  ssa (RCS Manager)
 * Author: lwl@hpucsb2.cup.hp.com
 * This log message is copied from revision 70.143.1.1 - lwl.
 * Fix for CLL4600041, C++ test program fails when compiled ANSI with no
 * optimization.  The translated C program gives signal 11.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/scan.c    70.9
 * 
 * Revision 72.3  92/08/14  09:53:44  09:53:44  ssa (RCS Manager)
 * Author: lkc@hpfclkc.fc.hp.com
 * eliminated /bin references.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/ccom.make    72.1
 * 
 * Revision 72.2  92/08/14  09:48:05  09:48:05  ssa (RCS Manager)
 * Author: lkc@hpfclkc.fc.hp.com
 * eliminated /bin references.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/makefile    72.1
 * 
 * Revision 72.1  92/08/14  09:46:20  09:46:20  ssa (RCS Manager)
 * Author: lkc@hpfclkc.fc.hp.com
 * eliminated /bin references.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/makefile    72.1
 * 
 * Revision 70.143  92/07/23  17:31:21  17:31:21  ssa (RCS Manager)
 * Author: asok@hpcll46.cup.hp.com
 * Fix for DNTT, the SRC, TAG and TYPE are generated in the correct sequence so that pxdb doesn't generate internal errors. DTS#CLL4600021
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cdbsyms.c    70.5
 * 
 * Revision 70.142  92/06/29  17:09:31  17:09:31  ssa (RCS Manager)
 * Author: asok@hpcll46.cup.hp.com
 * Fix for static analysis DTS#FSDdt08978, FSDdt08973.  Problem with
 * #line construct and comments and 
 * comments in character string initialization.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/scan.c    70.8
 * 
 * Revision 70.141  92/06/29  16:43:02  16:43:02  ssa (RCS Manager)
 * Author: asok@hpcll46.cup.hp.com
 * DTS#CLL4600024; Static Array initialization fix(HOT SITE).
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/pftn.c    70.7
 * 
 * Revision 70.140  92/06/16  11:43:14  11:43:14  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * expr's with DIV or MOD ops are not considered to be invariant, so that
 * they will not be hoisted out of a loop and then result in a divide by zero.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/loops.c    70.3
 * 
 * Revision 70.139  92/05/18  07:34:42  07:34:42  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/K_rel_c1.c    70.2
 * 
 * Revision 70.138  92/05/18  07:14:30  07:14:30  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * Rewrote code to see if an empty for loop can be removed
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/loopxforms.c    70.2
 * 
 * Revision 70.137  92/05/18  07:12:06  07:12:06  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * Fixed potential for uninitialiazed variable
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/loops.c    70.2
 * 
 * Revision 70.136  92/05/04  16:35:00  16:35:00  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * In optim(), don't swap subtrees if the node is
 * an array or struct ref
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/misc.c    70.3
 * 
 * Revision 70.135  92/05/04  15:39:13  15:39:13  ssa (RCS Manager)
 * Author: sje@hpsje.fc.hp.com
 * Check in change for inf. loop bug fix (compile time abort)
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/order.c    70.5
 * 
 * Revision 70.134  92/05/04  09:24:14  09:24:14  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * Added code to eliminate regions which could be the target of an
 * assigned goto from being optimized
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/oglobal.c    70.2
 * 
 * Revision 70.133  92/05/01  13:28:36  13:28:36  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * Yet another fix for GNTT enhancement.  With the addition of the K_SRCFILE 
 * dntt entry, a label (e.g. GD7:) was being placed on this line instead of
 * the following K_TAGDEF line where it belonged.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cdbsyms.c    70.4
 * 
 * Revision 70.132  92/04/27  10:55:18  10:55:18  ssa (RCS Manager)
 * Author: sje@hpsje.fc.hp.com
 * Fix compile time error where register is not freed (FLLrt00363)
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/local2.c    70.6
 * 
 * Revision 70.131  92/04/19  11:10:58  11:10:58  ssa (RCS Manager)
 * Author: asok@hpcll46.cup.hp.com
 * Fix for DTS#CLLbs00095. Core dumps for typedef enum{AA;BB}enum_type;
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/pftn.c    70.6
 * 
 * Revision 70.130  92/04/16  12:59:20  12:59:20  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * GNTT fix: in defid(), #ifdef IRIF should be above case TYPEDEF: - not below.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/pftn.c    70.5
 * 
 * Revision 70.129  92/04/03  14:21:27  14:21:27  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cput.c    70.3
 * 
 * Revision 70.128  92/04/03  14:20:48  14:20:48  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/yaccpar    70.3
 * 
 * Revision 70.127  92/04/03  14:20:16  14:20:16  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/xdefs.c    70.3
 * 
 * Revision 70.126  92/04/03  14:19:51  14:19:51  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/vtlib.h    70.3
 * 
 * Revision 70.125  92/04/03  14:19:31  14:19:31  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/vtlib.c    70.3
 * 
 * Revision 70.124  92/04/03  14:19:01  14:19:01  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/trees.c    70.6
 * 
 * Revision 70.123  92/04/03  14:18:27  14:18:27  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/table.c    70.3
 * 
 * Revision 70.122  92/04/03  14:18:00  14:18:00  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/scan.c    70.7
 * 
 * Revision 70.121  92/04/03  14:17:32  14:17:32  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/sa_iface.h    70.3
 * 
 * Revision 70.120  92/04/03  14:17:05  14:17:05  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * ,
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/sa.h    70.3
 * 
 * Revision 70.119  92/04/03  14:16:36  14:16:36  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/sa.c    70.3
 * 
 * Revision 70.118  92/04/03  14:16:01  14:16:01  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/reader.c    70.3
 * 
 * Revision 70.117  92/04/03  14:15:34  14:15:34  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/quad.c    70.3
 * 
 * Revision 70.116  92/04/03  14:14:37  14:14:37  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/pftn.c    70.4
 * 
 * Revision 70.115  92/04/03  14:04:41  14:04:41  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/order.c    70.4
 * 
 * Revision 70.114  92/04/03  14:04:13  14:04:13  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/optim.c    70.3
 * 
 * Revision 70.113  92/04/03  14:03:47  14:03:47  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/opcodes.h    70.4
 * 
 * Revision 70.112  92/04/03  14:03:20  14:03:20  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/mfile2    70.3
 * 
 * Revision 70.111  92/04/03  14:02:59  14:02:59  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/mfile1    70.4
 * 
 * Revision 70.110  92/04/03  14:02:27  14:02:27  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/messages.h    70.3
 * 
 * Revision 70.109  92/04/03  14:02:02  14:02:02  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/messages.c    70.4
 * 
 * Revision 70.108  92/04/03  14:01:35  14:01:35  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/match.c    70.3
 * 
 * Revision 70.107  92/04/03  14:01:07  14:01:07  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/manifest    70.4
 * 
 * Revision 70.106  92/04/03  14:00:41  14:00:41  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/macdefs    70.3
 * 
 * Revision 70.105  92/04/03  14:00:21  14:00:21  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/mac2defs    70.3
 * 
 * Revision 70.104  92/04/03  13:59:36  13:59:36  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/local2.c    70.5
 * 
 * Revision 70.103  92/04/03  13:58:37  13:58:37  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/local.c    70.3
 * 
 * Revision 70.102  92/04/03  13:58:04  13:58:04  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cpass2.c    70.3
 * 
 * Revision 70.101  92/04/03  13:57:32  13:57:32  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/commonb    70.3
 * 
 * Revision 70.100  92/04/03  13:57:09  13:57:09  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/common    70.4
 * 
 * Revision 70.99  92/04/03  13:56:36  13:56:36  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/comm2.c    70.3
 * 
 * Revision 70.98  92/04/03  13:56:09  13:56:09  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/comm1.c    70.3
 * 
 * Revision 70.97  92/04/03  13:55:26  13:55:26  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/code.c    70.3
 * 
 * Revision 70.96  92/04/03  13:54:47  13:54:47  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cgram.y    70.6
 * 
 * Revision 70.95  92/04/03  13:52:35  13:52:35  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cdbsyms.h    70.3
 * 
 * Revision 70.94  92/04/03  13:52:09  13:52:09  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cdbsyms.c    70.3
 * 
 * Revision 70.93  92/04/03  13:51:33  13:51:33  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/backend.c    70.3
 * 
 * Revision 70.92  92/04/03  13:49:01  13:49:01  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/apex_config.c    70.4
 * 
 * Revision 70.91  92/04/03  13:47:54  13:47:54  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/apex.h    70.5
 * 
 * Revision 70.90  92/04/03  13:47:18  13:47:18  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/allo.c    70.3
 * 
 * Revision 70.89  92/04/03  13:46:45  13:46:45  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * File set update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/K_rel_ccom.c    70.3
 * 
 * Revision 70.88  92/04/03  13:41:05  13:41:05  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * no change.  Just a blanket update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/walkf.s    70.3
 * 
 * Revision 70.87  92/04/03  13:40:21  13:40:21  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * no change.  Just a blanket update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/print.s    70.3
 * 
 * Revision 70.86  92/04/03  13:39:21  13:39:21  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * no change.  Just a blanket update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/ccom.make    70.5
 * 
 * Revision 70.85  92/04/03  13:37:23  13:37:23  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * no change. Just doing a blanket update for IF3.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/makefile    70.6
 * 
 * Revision 70.84  92/04/03  13:35:02  13:35:02  ssa (RCS Manager)
 * Author: donj@hpcll46.cup.hp.com
 * updated VUF to B.08.30 for IF3.
 * 
 * Revision 70.83  92/03/24  08:25:52  08:25:52  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * Mask off required bits when doing an SCONV to an unsigned char or short
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/misc.c    70.2
 * 
 * Revision 70.82  92/03/24  07:44:37  07:44:37  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * delete a block of dead code which refers only to itself
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/duopts.c    70.2
 * 
 * Revision 70.81  92/03/24  07:42:25  07:42:25  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * kludge for mixed auto and adjustable arrays w/ ENTRY
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/regdefuse.c    70.3
 * 
 * Revision 70.80  92/03/24  07:39:37  07:39:37  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * kludge for mixed automatic arrays and adjustable arrays w/ ENTRY points
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/regpass2.c    70.2
 * 
 * Revision 70.79  92/03/13  00:01:00  00:01:00  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * REC_PASFILE
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/apex.h    70.4
 * 
 * Revision 70.78  92/03/02  18:53:42  18:53:42  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * Remove unneeded routines to improve BBA
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/apex_config.c    70.3
 * 
 * Revision 70.77  92/02/27  11:40:19  11:40:19  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * In "process_call_hidden_vars", added a def/use for the second
 * half of Fortran complex's
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/regdefuse.c    70.2
 * 
 * Revision 70.76  92/02/27  09:15:51  09:15:51  ssa (RCS Manager)
 * Author: egeland@hpfclj.fc.hp.com
 * Removed spurious -DQUADC definition that was causing the ccom.ansi
 * and cpass1.ansi builds to fail.  Verified that not including -DQUADC
 * in ANSIFLAGS had no affect on the finished product (because it is
 * already turned on via PFLG20_ANSI and SPLITC_ANSI (for ccom.ansi and
 * cpass1.ansi, respectively).
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/makefile    70.5
 * 
 * Revision 70.75  92/02/26  14:39:27  14:39:27  ssa (RCS Manager)
 * Author: donj@hpcll11.cup.hp.com
 * 
 * 
 * 
 * oops - should have been -DQUADC.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/makefile    70.4
 * 
 * Revision 70.74  92/02/26  14:29:05  14:29:05  ssa (RCS Manager)
 * Author: donj@hpcll11.cup.hp.com
 * added -DINST_SCHED to XCFLAGS.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/ccom.make    70.4
 * 
 * Revision 70.73  92/02/26  14:27:38  14:27:38  ssa (RCS Manager)
 * Author: donj@hpcll11.cup.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/makefile    70.3
 * 
 * Revision 70.72  92/02/12  16:09:28  16:09:28  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * Add REC_F77USE
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/apex.h    70.3
 * 
 * Revision 70.71  92/02/10  09:56:58  09:56:58  ssa (RCS Manager)
 * Author: sje@hpsje.fc.hp.com
 * Fix +z +bfpa interaction bug
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/local2.c    70.4
 * 
 * Revision 70.70  92/01/17  11:07:50  11:07:50  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * Allow apex to parse "foo() #options(a0_return) {}"
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cgram.y    70.5
 * 
 * Revision 70.69  92/01/13  12:58:04  12:58:04  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/vfe.c    70.1
 * 
 * Revision 70.68  92/01/13  12:55:49  12:55:49  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/utils.c    70.1
 * 
 * Revision 70.67  92/01/13  12:52:48  12:52:48  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/symtab.c    70.1
 * 
 * Revision 70.66  92/01/13  12:49:41  12:49:41  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/regweb.c    70.1
 * 
 * Revision 70.65  92/01/13  12:46:02  12:46:02  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/regpass2.c    70.1
 * 
 * Revision 70.64  92/01/13  12:44:11  12:44:11  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/regpass1.c    70.1
 * 
 * Revision 70.63  92/01/13  12:41:56  12:41:56  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/register.c    70.1
 * 
 * Revision 70.62  92/01/13  12:39:58  12:39:58  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/regdefuse.c    70.1
 * 
 * Revision 70.61  92/01/13  12:37:04  12:37:04  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/regallo.c    70.1
 * 
 * Revision 70.60  92/01/13  12:34:41  12:34:41  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/p2out.c    70.1
 * 
 * Revision 70.59  92/01/13  12:32:05  12:32:05  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/oglobal.c    70.1
 * 
 * Revision 70.58  92/01/13  12:29:37  12:29:37  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/misc.c    70.1
 * 
 * Revision 70.57  92/01/13  12:26:59  12:26:59  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/mfile2    70.1
 * 
 * Revision 70.56  92/01/13  12:24:28  12:24:28  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/manifest    70.1
 * 
 * Revision 70.55  92/01/13  12:22:58  12:22:58  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/malloc.c    70.1
 * 
 * Revision 70.54  92/01/13  12:20:51  12:20:51  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/macdefs    70.1
 * 
 * Revision 70.53  92/01/13  12:18:30  12:18:30  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/loopxforms.c    70.1
 * 
 * Revision 70.52  92/01/13  09:57:56  09:57:56  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/loops.c    70.1
 * 
 * Revision 70.51  92/01/13  09:55:44  09:55:44  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/duopts.c    70.1
 * 
 * Revision 70.50  92/01/13  09:53:49  09:53:49  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/dag.c    70.1
 * 
 * Revision 70.49  92/01/13  09:48:29  09:48:29  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/c1.h    70.1
 * 
 * Revision 70.48  92/01/13  09:46:48  09:46:48  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * IF3 version
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/c1.c    70.1
 * 
 * Revision 70.47  92/01/13  09:37:47  09:37:47  ssa (RCS Manager)
 * Author: sjl@amber.fc.hp.com
 * Changed version # to B.09.00 for the IF3 release
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/K_rel_c1.c    70.1
 * 
 * Revision 70.46  92/01/10  15:35:45  15:35:45  ssa (RCS Manager)
 * Author: donj@hpcll11.cup.hp.com
 * added test for FCON in error check for CALL in buildtree().
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/code.c    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/common    70.3
 * /hpux/shared/supp/usr/src/cmd/compilers.300/optim.c    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/mfile2    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/mfile1    70.3
 * /hpux/shared/supp/usr/src/cmd/compilers.300/match.c    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/allo.c    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cgram.y    70.4
 * /hpux/shared/supp/usr/src/cmd/compilers.300/ccom.make    70.3
 * /hpux/shared/supp/usr/src/cmd/compilers.300/manifest    70.3
 * /hpux/shared/supp/usr/src/cmd/compilers.300/K_rel_ccom.c    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cdbsyms.c    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/print.s    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/scan.c    70.6
 * /hpux/shared/supp/usr/src/cmd/compilers.300/comm1.c    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/comm2.c    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/local2.c    70.3
 * /hpux/shared/supp/usr/src/cmd/compilers.300/order.c    70.3
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cdbsyms.h    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/xdefs.c    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/trees.c    70.5
 * /hpux/shared/supp/usr/src/cmd/compilers.300/table.c    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/local.c    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/vtlib.h    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/reader.c    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/pftn.c    70.3
 * /hpux/shared/supp/usr/src/cmd/compilers.300/mac2defs    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/macdefs    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/messages.h    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/walkf.s    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.300/sa.h    70.2
 * /hpux/shared/supp/usr/src/cmd/compilers.30
 * 
 * Revision 70.45  92/01/10  15:35:18  15:35:18  ssa (RCS Manager)
 * Author: donj@hpcll11.cup.hp.com
 * added test for FCON in error check for CALL in buildtree().
 * 
 * Revision 70.44  92/01/09  12:59:47  12:59:47  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * Parameter to yylex() is not set unless Xp./yaccpar is used - fix for Domain
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/scan.c    70.5
 * 
 * Revision 70.43  92/01/08  15:27:57  15:27:57  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * The bad_fp signal handler should #ifdef the S800 restart code for
 * !defined(DOMAIN) rather than !defined(APEX) so that S700 apex will not hang
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/trees.c    70.4
 * 
 * Revision 70.42  91/12/20  09:48:55  09:48:55  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * Restore condition "if (ansilint&&hflag)" around "function prototype not
 * visible at point of call"
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cgram.y    70.3
 * 
 * Revision 70.41  91/12/19  09:44:40  09:44:40  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * Remove comment terminator from Revision 70.40 auto-update text
 * 
 * Revision 70.40  91/12/18  13:17:03  13:17:03  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * Fix parsing of APEX HINT [*] -- lxapex() was correctly setting
 * all bits in "stds", then wiping that out with the uninitialized "first"
 * FSDdt09194
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/scan.c    70.4
 * 
 * Revision 70.39  91/12/12  12:40:42  12:40:42  ssa (RCS Manager)
 * Author: sje@hpsje.fc.hp.com
 * Fix +z +ffpa interaction bug in FORTRAN 68k codegen (update here to keep C
 * and FORTRAN identical)
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/local2.c    70.2
 * 
 * Revision 70.38  91/12/11  12:15:23  12:15:23  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * Require that APEX appear first in the comment to be considered a directive.
 * Also, make the parsing of APEX directives more robust.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/scan.c    70.3
 * 
 * Revision 70.37  91/11/30  14:45:23  14:45:23  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * Add -UAPEX to unifdef'ing of cgram.y
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/ccom.make    70.2
 * 
 * Revision 70.36  91/11/30  11:53:15  11:53:15  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * add apex warning for incorrect # args in call
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/trees.c    70.3
 * 
 * Revision 70.35  91/11/30  11:52:06  11:52:06  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * lxapex(), attribute and options parsing
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/scan.c    70.2
 * 
 * Revision 70.34  91/11/30  11:51:13  11:51:13  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * lxapex(), attribute and options parsing
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/pftn.c    70.2
 * 
 * Revision 70.33  91/11/30  11:50:05  11:50:05  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * apex opcodes
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/opcodes.h    70.2
 * 
 * Revision 70.32  91/11/30  11:49:25  11:49:25  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * apex global variable declarations
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/mfile1    70.2
 * 
 * Revision 70.31  91/11/30  11:48:43  11:48:43  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * Add apex messages 241-244
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/messages.c    70.2
 * 
 * Revision 70.30  91/11/30  11:47:35  11:47:35  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * Add F77 types for apex
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/manifest    70.2
 * 
 * Revision 70.29  91/11/30  11:46:51  11:46:51  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * initialize typedef name field in tree nodes for apex
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/common    70.2
 * 
 * Revision 70.28  91/11/30  11:45:39  11:45:39  ssa (RCS Manager)
 * Author: pas@hpfcpas.fc.hp.com
 * Domain extensions for apex
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cgram.y    70.2
 * 
 * Revision 70.27  91/11/27  08:26:32  08:26:32  ssa (RCS Manager)
 * Author: sje@hpsje.fc.hp.com
 * Fix same bug as FSDdt07430 but in +ffpa mode
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/order.c    70.2
 * 
 * Revision 70.26  91/08/28  13:06:45  13:06:45  ssa (RCS Manager)
 * Author: bethke@hpbethke.fc.hp.com
 * Turn off IRIF on the unifdef of cgram.y.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/ccom.make    70.1
 * 
 * Revision 70.25  91/08/28  07:49:57  07:49:57  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * unlock the file
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/makefile    70.1
 * 
 * Revision 70.24  91/08/28  00:05:11  00:05:11  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cgram.y    70.1
 * 
 * Revision 70.23  91/08/28  00:03:19  00:03:19  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * Revision 70.22  91/08/27  23:29:12  23:29:12  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/mfile1    70.1
 * 
 * Revision 70.21  91/08/27  23:28:34  23:28:34  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/messages.h    70.1
 * 
 * Revision 70.20  91/08/27  23:27:56  23:27:56  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/messages.c    70.1
 * 
 * Revision 70.19  91/08/27  23:27:17  23:27:17  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/match.c    70.1
 * 
 * Revision 70.18  91/08/27  23:26:35  23:26:35  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/manifest    70.1
 * 
 * Revision 70.17  91/08/27  23:25:50  23:25:50  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/macdefs    70.1
 * 
 * Revision 70.16  91/08/27  23:25:12  23:25:12  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/mac2defs    70.1
 * 
 * Revision 70.15  91/08/27  23:24:37  23:24:37  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/local2.c    70.1
 * 
 * Revision 70.14  91/08/27  23:23:07  23:23:07  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/local.c    70.1
 * 
 * Revision 70.13  91/08/27  23:22:29  23:22:29  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cput.c    70.1
 * 
 * Revision 70.12  91/08/27  23:21:50  23:21:50  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cpass2.c    70.1
 * 
 * Revision 70.11  91/08/27  23:21:12  23:21:12  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/commonb    70.1
 * 
 * Revision 70.10  91/08/27  23:20:34  23:20:34  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/common    70.1
 * 
 * Revision 70.9  91/08/27  23:19:52  23:19:52  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/comm2.c    70.1
 * 
 * Revision 70.8  91/08/27  23:19:14  23:19:14  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/comm1.c    70.1
 * 
 * Revision 70.7  91/08/27  23:18:32  23:18:32  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/code.c    70.1
 * 
 * Revision 70.6  91/08/27  23:17:42  23:17:42  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cdbsyms.h    70.1
 * 
 * Revision 70.5  91/08/27  23:17:09  23:17:09  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cdbsyms.c    70.1
 * 
 * Revision 70.4  91/08/27  23:15:58  23:15:58  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/backend.c    70.1
 * 
 * Revision 70.3  91/08/27  23:15:12  23:15:12  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/allo.c    70.1
 * 
 * Revision 70.2  91/08/27  23:12:26  23:12:26  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/yaccpar    70.1
 * 
 * Revision 70.1  91/08/27  23:11:31  23:11:31  ssa (Shared Source Administrator)
 * Author: pas@hpfclj.fc.hp.com
 * update to level 63 SCCS
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/K_rel_ccom.c    70.1
 * 
 * Revision 66.205  91/05/01  11:15:25  11:15:25  ssa (Shared Source Administrator)
 * Author: egeland@hpfclj.fc.hp.com
 * Late-breaking 8.0 change to fix optimizer problem.  Changed loops.c.
 * Changes checked in for Greg Lindhorst.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/K_rel_c1.c    66.32
 * 
 * Revision 66.204  91/04/30  21:20:15  21:20:15  ssa (Shared Source Administrator)
 * Author: egeland@hpfclj.fc.hp.com
 * Checked in for Greg Lindhorst to fix late-breaking 8.0 optimizer
 * problem.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/loops.c    66.13
 * 
 * Revision 66.203  91/03/27  11:39:19  11:39:19  ssa (Shared Source Administrator)
 * Author: bethke@hpbethke
 * Fix for debug info array size defect. CRT.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cdbsyms.c    66.15
 * 
 * Revision 66.202  91/03/15  10:05:43  10:05:43  ssa (Shared Source Administrator)
 * Author: brad@hpfcrt
 * Fix of defect FSDdt06664.  Must def ptr targets when a call
 * is encountered.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/dag.c    66.8
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/K_rel_c1.c    66.31
 * 
 * Revision 66.201  91/03/11  11:24:00  11:24:00  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * CRT fix for problem in debug size information.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/cdbsyms.c    66.14
 * 
 * Revision 66.200  91/03/11  11:20:19  11:20:19  ssa (Shared Source Administrator)
 * Author: bethke@hpfclj
 * CRT fix of the nls 16-bit character support bug.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/scan.c    66.25
 * 
 * Revision 66.199  91/03/05  11:14:43  11:14:43  ssa (Shared Source Administrator)
 * Author: mev@hpfclj
 * Hi Mark, expect a CRT for this one.
 * Fix FSDdt06365, auto aggregate initializers incorrect with string initialization for char array.
 * Only occurs in ANSI mode.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/pftn.c    66.22
 * 
 * Revision 66.198  91/02/26  15:41:30  15:41:30  ssa (Shared Source Administrator)
 * Author: brad@hpfcrt
 * loop invariant code motion problem fixed, seen in perf.suite/thtd
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/K_rel_c1.c    66.30
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/loops.c    66.12
 * 
 * Revision 66.197  91/02/04  12:03:59  12:03:59  ssa (Shared Source Administrator)
 * Author: sje@hpsje
 * Fix bug involving double prec. array assignment in return stmnt
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/table.c    66.10
 * 
 * Revision 66.196  91/02/04  12:00:51  12:00:51  ssa (Shared Source Administrator)
 * Author: sje@hpsje
 * Fix bug involving double prec. array assignment in return stmnt
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/local2.c    66.19
 * 
 * Revision 66.195  91/01/31  13:54:09  13:54:09  ssa (Shared Source Administrator)
 * Author: brad@hpfcrt
 * Inlined transcendental call requires PIC base register.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/K_rel_c1.c    66.29
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/regdefuse.c    66.10
 * 
 * Revision 66.194  91/01/21  08:06:57  08:06:57  ssa (Shared Source Administrator)
 * Author: brad@hpfcrt
 * Do not execute cerror() for bogus symbol table entry detection if the name
 * of the symbol table entry begins with an 'L'.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/c1.c    66.18
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/K_rel_c1.c    66.28
 * 
 * Revision 66.193  91/01/08  09:21:35  09:21:35  ssa (Shared Source Administrator)
 * Author: sje@hpsje
 * Fix log handling for 68040
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/local2.c    66.18
 * 
 * Revision 66.192  91/01/08  08:34:35  08:34:35  ssa (Shared Source Administrator)
 * Author: sje@hpsje
 * Fix PIC / 040 apollo emulation routine interaction problem
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/local2.c    66.17
 * 
 * Revision 66.191  91/01/07  08:32:06  08:32:06  ssa (Shared Source Administrator)
 * Author: brad@hpfcrt
 * Fix to avoid changing type in symbol table to CHAR.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/c1.c    66.17
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/K_rel_c1.c    66.27
 * 
 * Revision 66.190  90/12/14  15:11:03  15:11:03  ssa (Shared Source Administrator)
 * Author: brad@hpfcrt
 * Short circuit evaluation of an "if" prevented a pointer from getting
 * a value.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/K_rel_c1.c    66.26
 * /hpux/shared/supp/usr/src/cmd/compilers.300/c1/regdefuse.c    66.9
 * 
 * Revision 66.189  90/12/14  10:54:22  10:54:22  bethke
 * ******* The Following Modules Were Modified *******
 *     pftn.c,v
 * 
 * Revision 66.188  90/12/14  10:52:13  10:52:13  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     trees.c,v
 * 
 * Revision 66.187  90/12/10  09:20:12  09:20:12  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     backend.c,v
 * 
 * Revision 66.186  90/12/05  15:54:03  15:54:03  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     local2.c,v
 * 
 * Revision 66.185  90/11/28  10:45:02  10:45:02  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     c0/K_rel_c0.c,v
 * 
 * Revision 66.184  90/11/28  06:56:57  06:56:57  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     local2.c,v
 * 
 * Revision 66.183  90/11/27  15:28:48  15:28:48  bethke (Robert Bethke)
 * Set what string.
 * 
 * Revision 66.182  90/11/26  12:35:17  12:35:17  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/K_rel_c1.c,v
 *     c1/misc.c,v
 * 
 * Revision 66.181  90/11/26  09:00:14  09:00:14  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     local.c,v
 * 
 * Revision 66.180  90/11/20  11:18:58  11:18:58  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     trees.c,v
 *     table.c,v
 *     scan.c,v
 *     order.c,v
 *     local2.c,v
 *     backend.c,v
 * 
 * Revision 66.179  90/11/20  10:42:11  10:42:11  mike (Mike McNelly)
 * Deltas to ensure consistency between SCCS on hpdcdb and RCS on hpfclj.
 * ******* The Following Modules Were Modified *******
 *     c1/xprint.c,v
 *     c1/vfe.c,v
 *     c1/utils.c,v
 *     c1/symtab.c,v
 *     c1/sets.s,v
 *     c1/sets.c,v
 *     c1/regweb.c,v
 *     c1/regpass2.c,v
 *     c1/regpass1.c,v
 *     c1/register.c,v
 *     c1/regdefuse.c,v
 *     c1/regallo.c,v
 *     c1/p2out.c,v
 *     c1/oglobal.c,v
 *     c1/misc.c,v
 *     c1/mfile2,v
 *     c1/manifest,v
 *     c1/malloc.c,v
 *     c1/makefile,v
 *     c1/macdefs,v
 *     c1/loopxforms.c,v
 *     c1/loops.c,v
 *     c1/duopts.c,v
 *     c1/dag.c,v
 *     c1/chkmem.h,v
 *     c1/chkmem.c,v
 *     c1/c1.h,v
 *     c1/c1.c,v
 *     c1/K_rel_c1.c,v
 * 
 * Revision 66.178  90/11/19  16:08:12  16:08:12  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     c0/inline.c,v
 * 
 * Revision 66.177  90/11/16  08:13:04  08:13:04  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/misc.c,v
 * 
 * Revision 66.176  90/11/15  14:05:30  14:05:30  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/symtab.c,v
 * 
 * Revision 66.175  90/11/15  13:40:43  13:40:43  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/c1.h,v
 *     c1/c1.c,v
 *     c1/misc.c,v
 *     c1/oglobal.c,v
 *     c1/K_rel_c1.c,v
 * 
 * Revision 66.174  90/11/09  14:46:43  14:46:43  mev (Mike Vermeulen)
 * Additional lint support for alignment of struct/union/enum and array types.
 * ******* The Following Modules Were Modified *******
 *     pftn.c,v
 * 
 * Revision 66.173  90/11/07  15:34:09  15:34:09  mev (Mike Vermeulen)
 * Add new improved alignment checking to lint.
 * ******* The Following Modules Were Modified *******
 *     pftn.c,v
 * 
 * Revision 66.172  90/11/06  17:10:16  17:10:16  mev (Mike Vermeulen)
 * Lint: add support for -s option (check alignments).
 * ******* The Following Modules Were Modified *******
 *     xdefs.c,v
 *     pftn.c,v
 *     mfile1,v
 *     messages.h,v
 *     messages.c,v
 * 
 * Revision 66.171  90/10/31  08:35:25  08:35:25  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/symtab.c,v
 *     c1/utils.c,v
 *     c1/K_rel_c1.c,v
 * 
 * Revision 66.170  90/10/30  09:16:11  09:16:11  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     trees.c,v
 * 
 * Revision 66.169  90/10/30  09:15:06  09:15:06  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     scan.c,v
 * 
 * Revision 66.168  90/10/30  09:14:08  09:14:08  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     messages.c,v
 * 
 * Revision 66.167  90/10/30  09:13:15  09:13:15  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     local.c,v
 * 
 * Revision 66.166  90/10/30  09:10:53  09:10:53  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     code.c,v
 * 
 * Revision 66.165  90/10/30  09:10:06  09:10:06  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     cgram.y,v
 * 
 * Revision 66.164  90/10/30  09:09:17  09:09:17  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     cdbsyms.c,v
 * 
 * Revision 66.163  90/10/30  09:08:25  09:08:25  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     backend.c,v
 * 
 * Revision 66.162  90/10/30  09:07:28  09:07:28  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     pftn.c,v
 * 
 * Revision 66.161  90/10/30  08:30:01  08:30:01  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     local2.c,v
 * 
 * Revision 66.160  90/10/29  15:45:40  15:45:40  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/utils.c,v
 *     c1/K_rel_c1.c,v
 * 
 * Revision 66.159  90/10/29  08:29:01  08:29:01  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/duopts.c,v
 *     c1/K_rel_c1.c,v
 * 
 * Revision 66.158  90/10/25  09:40:44  09:40:44  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     ccom.make,v
 * 
 * Revision 66.157  90/10/24  10:49:30  10:49:30  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     table.c,v
 * 
 * Revision 66.156  90/10/23  15:30:04  15:30:04  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     ccom.make,v
 * 
 * Revision 66.155  90/10/23  15:24:41  15:24:41  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     local2.c,v
 * 
 * Revision 66.154  90/10/17  17:36:56  17:36:56  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/c1.c,v
 *     c1/K_rel_c1.c,v
 * 
 * Revision 66.153  90/10/15  09:42:17  09:42:17  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     c0/pass1.c,v
 * 
 * Revision 66.152  90/10/02  16:01:21  16:01:21  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/utils.c,v
 *     c1/K_rel_c1.c,v
 * 
 * Revision 66.151  90/10/01  13:26:39  13:26:39  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/vfe.c,v
 *     c1/utils.c,v
 *     c1/symtab.c,v
 *     c1/loops.c,v
 *     c1/c1.c,v
 *     c1/K_rel_c1.c,v
 * 
 * Revision 66.150  90/09/25  16:12:37  16:12:37  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     local2.c,v
 * 
 * Revision 66.149  90/09/25  16:11:17  16:11:17  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     trees.c,v
 * 
 * Revision 66.148  90/09/25  16:09:07  16:09:07  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     backend.c,v
 * 
 * Revision 66.147  90/09/25  16:08:06  16:08:06  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     common,v
 * 
 * Revision 66.146  90/08/31  15:24:01  15:24:01  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/c1.c,v
 *     c1/K_rel_c1.c,v
 * 
 * Revision 66.145  90/08/30  13:53:48  13:53:48  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/c1.c,v
 *     c1/K_rel_c1.c,v
 * 
 * Revision 66.144  90/08/28  14:03:02  14:03:02  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/K_rel_c1.c,v
 * 
 * Revision 66.143  90/08/26  10:22:36  10:22:36  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     local2.c,v
 * 
 * Revision 66.142  90/08/26  10:20:50  10:20:50  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     reader.c,v
 * 
 * Revision 66.141  90/08/26  10:19:23  10:19:23  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     table.c,v
 * 
 * Revision 66.140  90/08/26  10:17:26  10:17:26  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     common,v
 * 
 * Revision 66.139  90/08/24  12:34:42  12:34:42  mike (Mike McNelly)
 * Fix to ensure proper values for SETREGS records.  Fix to remove call to
 * initialize_definetab() after removing loops.  Fix to remove loops only
 * during the first call to do_def_use_opts().
 * ******* The Following Modules Were Modified *******
 *     c1/utils.c,v
 *     c1/regpass2.c,v
 *     c1/duopts.c,v
 *     c1/c1.h,v
 *     c1/c1.c,v
 *     c1/K_rel_c1.c,v
 *     c1/symtab.c,v
 * 
 * Revision 66.138  90/08/23  19:05:14  19:05:14  rsh (R Scott Holbrook)
 * Removed kerncom.  The new C product structure no longer requires it.
 * ******* The Following Modules Were Modified *******
 *     ccom.make,v
 * 
 * Revision 66.137  90/08/20  14:52:26  14:52:26  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     pftn.c,v
 *     scan.c,v
 * 
 * Revision 66.136  90/08/19  13:10:10  13:10:10  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     yaccpar,v
 *     xdefs.c,v
 *     walkf.s,v
 *     vtlib.h,v
 *     vtlib.c,v
 *     trees.c,v
 *     table.c,v
 *     scan.c,v
 *     sa_iface.h,v
 *     sa.h,v
 *     sa.c,v
 *     reader.c,v
 *     quad.c,v
 *     pftn.c,v
 *     order.c,v
 *     optim.c,v
 *     opcodes.h,v
 *     mfile2,v
 *     mfile1,v
 *     messages.h,v
 *     messages.c,v
 *     match.c,v
 *     manifest,v
 *     macdefs,v
 *     mac2defs,v
 *     local2.c,v
 *     local.c,v
 *     cput.c,v
 *     cpass2.c,v
 *     commonb,v
 *     common,v
 *     comm2.c,v
 *     comm1.c,v
 *     code.c,v
 *     cgram.y,v
 *     cdbsyms.h,v
 *     cdbsyms.c,v
 *     backend.c,v
 *     allo.c,v
 *     K_rel_ccom.c,v
 * 
 * Revision 66.135  90/08/19  13:05:58  13:05:58  bethke (Robert Bethke)
 * Null delta to bump past the level for the HIPER/7_05 tag.
 * 
 * Revision 66.134  90/08/19  10:34:27  10:34:27  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     local2.c,v
 * 
 * Revision 66.133  90/08/17  15:25:48  15:25:48  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     xdefs.c,v
 *     scan.c,v
 *     mfile1,v
 *     common,v
 *     cgram.y,v
 * 
 * Revision 66.132  90/08/17  12:59:56  12:59:56  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     backend.c,v
 * 
 * Revision 66.131  90/08/14  08:41:31  08:41:31  mike (Mike McNelly)
 * Changes to insert the first element of all arrays into the symbol table.
 * ******* The Following Modules Were Modified *******
 *     c1/utils.c,v
 *     c1/symtab.c,v
 *     c1/c1.h,v
 *     c1/c1.c,v
 * 
 * Revision 66.130  90/08/11  09:57:34  09:57:34  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     vtlib.c,v
 *     scan.c,v
 *     sa.c,v
 *     pftn.c,v
 *     common,v
 *     comm2.c,v
 *     comm1.c,v
 *     code.c,v
 * 
 * Revision 66.129  90/08/09  10:20:10  10:20:10  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/xprint.c,v
 * 
 * Revision 66.128  90/08/08  08:46:50  08:46:50  mev (Mike Vermeulen)
 * Change #ifdef LINT to #ifdef LINT_TRY to fix problem with lint building.
 * ******* The Following Modules Were Modified *******
 *     scan.c,v
 * 
 * Revision 66.127  90/08/06  11:34:27  11:34:27  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     scan.c,v
 *     xdefs.c,v
 *     pftn.c,v
 * 
 * Revision 66.126  90/07/25  15:31:18  15:31:18  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     ccom.make,v
 * 
 * Revision 66.125  90/07/25  15:26:42  15:26:42  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     ccom.make,v
 * 
 * Revision 66.124  90/07/25  15:22:24  15:22:24  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     ccom.make,v
 * 
 * Revision 66.123  90/07/24  15:26:20  15:26:20  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     scan.c,v
 * 
 * Revision 66.122  90/07/23  14:05:34  14:05:34  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cgram.y,v
 *     trees.c,v
 * 
 * Revision 66.121  90/07/19  15:13:18  15:13:18  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     trees.c,v
 *     table.c,v
 *     scan.c,v
 *     pftn.c,v
 *     mfile2,v
 *     mfile1,v
 *     manifest,v
 *     mac2defs,v
 *     local2.c,v
 *     local.c,v
 *     cput.c,v
 * 
 * Revision 66.120  90/07/19  15:02:39  15:02:39  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     common,v
 *     code.c,v
 *     cgram.y,v
 *     cdbsyms.c,v
 *     backend.c,v
 * 
 * Revision 66.119  90/07/18  15:58:17  15:58:17  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     allo.c,v
 * 
 * Revision 66.118  90/07/18  15:56:41  15:56:41  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     reader.c,v
 * 
 * Revision 66.117  90/06/29  15:16:52  15:16:52  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     yaccpar,v
 * 
 * Revision 66.116  90/06/29  15:15:40  15:15:40  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     xdefs.c,v
 * 
 * Revision 66.115  90/06/29  15:15:15  15:15:15  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     walkf.s,v
 * 
 * Revision 66.114  90/06/29  15:14:49  15:14:49  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     vtlib.h,v
 * 
 * Revision 66.113  90/06/29  15:14:27  15:14:27  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     vtlib.c,v
 * 
 * Revision 66.112  90/06/29  15:14:09  15:14:09  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     trees.c,v
 * 
 * Revision 66.111  90/06/29  15:13:35  15:13:35  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     table.c,v
 * 
 * Revision 66.110  90/06/29  15:13:00  15:13:00  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     symtab.h,v
 * 
 * Revision 66.109  90/06/29  15:12:31  15:12:31  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     scan.c,v
 * 
 * Revision 66.108  90/06/29  15:11:57  15:11:57  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     sa_iface.h,v
 * 
 * Revision 66.107  90/06/29  15:11:40  15:11:40  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     sa.h,v
 * 
 * Revision 66.106  90/06/29  15:11:20  15:11:20  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     sa.c,v
 * 
 * Revision 66.105  90/06/29  15:10:53  15:10:53  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     reader.c,v
 * 
 * Revision 66.104  90/06/29  15:10:27  15:10:27  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     quad.c,v
 * 
 * Revision 66.103  90/06/29  15:10:03  15:10:03  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     pftn.c,v
 * 
 * Revision 66.102  90/06/29  15:09:24  15:09:24  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     order.c,v
 * 
 * Revision 66.101  90/06/29  15:08:56  15:08:56  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     optim.c,v
 * 
 * Revision 66.100  90/06/29  15:08:21  15:08:21  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     opcodes.h,v
 * 
 * Revision 66.99  90/06/29  15:07:58  15:07:58  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     mfile2,v
 * 
 * Revision 66.98  90/06/29  15:07:35  15:07:35  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     mfile1,v
 * 
 * Revision 66.97  90/06/29  15:07:04  15:07:04  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     messages.h,v
 * 
 * Revision 66.96  90/06/29  15:06:35  15:06:35  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     messages.c,v
 * 
 * Revision 66.95  90/06/29  14:56:29  14:56:29  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     match.c,v
 * 
 * Revision 66.94  90/06/29  14:56:06  14:56:06  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     manifest,v
 * 
 * Revision 66.93  90/06/29  14:55:38  14:55:38  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     macdefs,v
 * 
 * Revision 66.92  90/06/29  14:55:11  14:55:11  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     mac2defs,v
 * 
 * Revision 66.91  90/06/29  14:54:44  14:54:44  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     local2.c,v
 * 
 * Revision 66.90  90/06/29  14:54:07  14:54:07  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     local.c,v
 * 
 * Revision 66.89  90/06/29  14:53:40  14:53:40  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     dnttsizes.h,v
 * 
 * Revision 66.88  90/06/29  14:53:16  14:53:16  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cput.c,v
 * 
 * Revision 66.87  90/06/29  14:52:49  14:52:49  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cpass2.c,v
 * 
 * Revision 66.86  90/06/29  14:52:23  14:52:23  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     commonb,v
 * 
 * Revision 66.85  90/06/29  14:52:00  14:52:00  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     common,v
 * 
 * Revision 66.84  90/06/29  14:51:35  14:51:35  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     comm2.c,v
 * 
 * Revision 66.83  90/06/29  14:51:07  14:51:07  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     comm1.c,v
 * 
 * Revision 66.82  90/06/29  14:50:49  14:50:49  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     code.c,v
 * 
 * Revision 66.81  90/06/29  14:50:27  14:50:27  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cgram.y,v
 * 
 * Revision 66.80  90/06/29  14:49:56  14:49:56  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cdbsyms.h,v
 * 
 * Revision 66.79  90/06/29  14:49:23  14:49:23  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cdbsyms.c,v
 * 
 * Revision 66.78  90/06/29  14:48:47  14:48:47  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     backend.c,v
 * 
 * Revision 66.77  90/06/29  14:48:26  14:48:26  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     allo.c,v
 * 
 * Revision 66.76  90/06/29  14:34:01  14:34:01  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     K_rel_ccom.c,v
 * 
 * Revision 66.75  90/06/22  17:39:04  17:39:04  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.74  90/06/22  15:46:19  15:46:19  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     ccom.make,v
 * 
 * Revision 66.73  90/06/12  14:35:34  14:35:34  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/regpass2.c,v
 * 
 * Revision 66.72  90/06/05  11:02:44  11:02:44  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/K_rel_c1.c,v
 *     c1/loops.c,v
 * 
 * Revision 66.71  90/05/31  12:35:01  12:35:01  rsh (R Scott Holbrook)
 * Major modifications to allow ccom, ccom.ansi, cpass1, cpass1.ansi and cpass2
 * to be built independently in parallel.  The object files for each executable
 * are placed in a seperate sub-directory.
 * In addition to allowing the pieces to be built in parallel, it is now
 * no longer necessary to remove the current ".o" files each time you want
 * to make something.  This means that a simple relink only takes a few seconds
 * instead of 40 minutes.
 * ******* The Following Modules Were Modified *******
 *     ccom.make,v
 *     makefile,v
 * 
 * Revision 66.70  90/05/29  15:54:58  15:54:58  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/regpass2.c,v
 * 
 * Revision 66.69  90/05/25  14:46:55  14:46:55  marc (Marc Sabatella)
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.68  90/05/23  15:50:12  15:50:12  mev (Mike Vermeulen)
 * Remove argument promotion of char->int from lint cases.
 * ******* The Following Modules Were Modified *******
 *     trees.c,v
 * 
 * Revision 66.67  90/05/16  16:14:53  16:14:53  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.66  90/05/16  15:58:31  15:58:31  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     c0/makefile,v
 * 
 * Revision 66.65  90/05/16  15:54:21  15:54:21  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     c0/makefile,v
 * 
 * Revision 66.64  90/05/16  15:50:51  15:50:51  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     c0/makefile,v
 * 
 * Revision 66.63  90/05/16  15:44:13  15:44:13  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     c0/pass2.c,v
 * 
 * Revision 66.62  90/05/16  15:43:51  15:43:51  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     c0/pass1.c,v
 * 
 * Revision 66.61  90/05/16  15:43:24  15:43:24  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     c0/makefile,v
 * 
 * Revision 66.60  90/05/16  15:43:04  15:43:04  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     c0/main.c,v
 * 
 * Revision 66.59  90/05/16  15:42:42  15:42:42  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     c0/inline.c,v
 * 
 * Revision 66.58  90/05/16  15:42:19  15:42:19  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     c0/callgraph.c,v
 * 
 * Revision 66.57  90/05/16  15:41:59  15:41:59  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     c0/c0.h,v
 * 
 * Revision 66.56  90/05/16  15:41:39  15:41:39  sje (Steve Ellcey)
 * ******* The Following Modules Were Modified *******
 *     c0/K_rel_c0.c,v
 * 
 * Revision 66.55  90/05/15  08:30:31  08:30:31  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/c1.c,v
 * 
 * Revision 66.54  90/05/04  16:51:21  16:51:21  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/K_rel_c1.c,v
 *     c1/utils.c,v
 * 
 * Revision 66.53  90/04/30  11:52:54  11:52:54  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     common,v
 * 
 * Revision 66.52  90/04/30  11:47:39  11:47:39  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     vtlib.c,v
 * 
 * Revision 66.51  90/04/30  11:46:43  11:46:43  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     trees.c,v
 * 
 * Revision 66.50  90/04/30  11:45:01  11:45:01  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     scan.c,v
 * 
 * Revision 66.49  90/04/30  11:43:32  11:43:32  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     sa.c,v
 * 
 * Revision 66.48  90/04/30  11:42:27  11:42:27  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     pftn.c,v
 * 
 * Revision 66.47  90/04/30  11:40:45  11:40:45  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     order.c,v
 * 
 * Revision 66.46  90/04/30  11:39:15  11:39:15  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     optim.c,v
 * 
 * Revision 66.45  90/04/30  11:38:09  11:38:09  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     mfile1,v
 * 
 * Revision 66.44  90/04/30  11:36:46  11:36:46  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     manifest,v
 * 
 * Revision 66.43  90/04/30  11:35:40  11:35:40  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     local2.c,v
 * 
 * Revision 66.42  90/04/30  11:33:39  11:33:39  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     local.c,v
 * 
 * Revision 66.41  90/04/30  11:31:57  11:31:57  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cput.c,v
 * 
 * Revision 66.40  90/04/30  11:30:39  11:30:39  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     common,v
 * 
 * Revision 66.39  90/04/30  11:29:10  11:29:10  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     comm1.c,v
 * 
 * Revision 66.38  90/04/30  11:27:39  11:27:39  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     code.c,v
 * 
 * Revision 66.37  90/04/30  11:26:19  11:26:19  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cdbsyms.c,v
 * 
 * Revision 66.36  90/04/30  11:24:28  11:24:28  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     backend.c,v
 * 
 * Revision 66.35  90/04/30  11:23:06  11:23:06  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     allo.c,v
 * 
 * Revision 66.34  90/04/18  15:18:25  15:18:25  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/K_rel_c1.c,v
 *     c1/misc.c,v
 *     c1/c1.c,v
 * 
 * Revision 66.33  90/04/06  15:34:08  15:34:08  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/K_rel_c1.c,v
 *     c1/oglobal.c,v
 *     c1/misc.c,v
 *     c1/loops.c,v
 *     c1/c1.h,v
 *     c1/c1.c,v
 * 
 * Revision 66.32  90/04/03  09:58:29  09:58:29  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     local.c,v
 * 
 * Revision 66.31  90/03/28  15:25:14  15:25:14  mike (Mike McNelly)
 * ******* The Following Modules Were Modified *******
 *     c1/K_rel_c1.c,v
 *     c1/p2out.c,v
 *     c1/misc.c,v
 *     c1/loops.c,v
 *     c1/dag.c,v
 * 
 * Revision 66.30  90/03/15  09:07:16  09:07:16  mike (Mike McNelly)
 * changed typo in makefile from -Drel8.0 to -Drel8_0.
 * ******* The Following Modules Were Modified *******
 *     c1/makefile,v
 * 
 * Revision 66.29  90/03/13  15:44:59  15:44:59  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     scan.c,v
 * 
 * Revision 66.28  90/03/13  15:42:04  15:42:04  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     pftn.c,v
 * 
 * Revision 66.27  90/03/13  15:38:29  15:38:29  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     code.c,v
 * 
 * Revision 66.26  90/03/13  15:36:41  15:36:41  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cdbsyms.c,v
 * 
 * Revision 66.25  90/03/07  14:08:07  14:08:07  mike (Mike McNelly)
 * updates to 8.0 and C++ release. - mfm
 * ******* The Following Modules Were Modified *******
 *     c1/xprint.c,v
 *     c1/vfe.c,v
 *     c1/utils.c,v
 *     c1/symtab.c,v
 *     c1/sets.s,v
 *     c1/sets.c,v
 *     c1/regweb.c,v
 *     c1/regpass2.c,v
 *     c1/regpass1.c,v
 *     c1/register.c,v
 *     c1/regdefuse.c,v
 *     c1/regallo.c,v
 *     c1/p2out.c,v
 *     c1/oglobal.c,v
 *     c1/misc.c,v
 *     c1/mfile2,v
 *     c1/manifest,v
 *     c1/malloc.c,v
 *     c1/makefile,v
 *     c1/macdefs,v
 *     c1/loopxforms.c,v
 *     c1/loops.c,v
 *     c1/duopts.c,v
 *     c1/dag.c,v
 *     c1/chkmem.h,v
 *     c1/chkmem.c,v
 *     c1/c1.h,v
 *     c1/c1.c,v
 *     c1/K_rel_c1.c,v
 * 
 * Revision 66.24  90/03/06  12:12:27  12:12:27  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     sa_iface.h,v
 * 
 * Revision 66.23  90/03/06  12:10:59  12:10:59  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     sa.h,v
 * 
 * Revision 66.22  90/03/06  12:09:32  12:09:32  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     sa.c,v
 * 
 * Revision 66.21  90/03/06  12:07:06  12:07:06  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     scan.c,v
 * 
 * Revision 66.20  90/03/06  12:05:17  12:05:17  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     trees.c,v
 * 
 * Revision 66.19  90/03/06  12:02:03  12:02:03  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     reader.c,v
 * 
 * Revision 66.18  90/03/06  12:00:21  12:00:21  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     pftn.c,v
 * 
 * Revision 66.17  90/03/06  11:56:31  11:56:31  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     mfile2,v
 * 
 * Revision 66.16  90/03/06  11:54:34  11:54:34  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     mfile1,v
 * 
 * Revision 66.15  90/03/06  11:51:30  11:51:30  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     order.c,v
 * 
 * Revision 66.14  90/03/06  11:47:42  11:47:42  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     match.c,v
 * 
 * Revision 66.13  90/03/06  11:26:13  11:26:13  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     local2.c,v
 * 
 * Revision 66.12  90/03/06  11:23:39  11:23:39  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     local.c,v
 * 
 * Revision 66.11  90/03/06  11:21:34  11:21:34  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cput.c,v
 * 
 * Revision 66.10  90/03/06  11:18:24  11:18:24  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     commonb,v
 * 
 * Revision 66.9  90/03/06  11:17:01  11:17:01  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     common,v
 * 
 * Revision 66.8  90/03/06  11:11:38  11:11:38  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     code.c,v
 * 
 * Revision 66.7  90/03/06  11:08:35  11:08:35  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     vtlib.c,v
 * 
 * Revision 66.6  90/03/06  11:04:14  11:04:14  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cgram.y,v
 * 
 * Revision 66.5  90/03/06  11:01:59  11:01:59  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cdbsyms.h,v
 * 
 * Revision 66.4  90/03/06  11:00:24  11:00:24  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     cdbsyms.c,v
 * 
 * Revision 66.3  90/03/06  10:51:19  10:51:19  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     backend.c,v
 * 
 * Revision 66.2  90/03/01  13:36:21  13:36:21  bethke (Robert Bethke)
 * ******* The Following Modules Were Modified *******
 *     ccom.make,v
 * 
 * Revision 66.1  90/03/01  13:25:55  13:25:55  bethke (Robert Bethke)
 * Initial version of the new hpux_rel.c
 * 
 */
