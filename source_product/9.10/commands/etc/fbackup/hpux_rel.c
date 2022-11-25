#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 72.9.1.1 $";
#endif

/*
    ** $Log:	hpux_rel.c,v $
 * Revision 72.9.1.1  94/10/12  15:37:14  15:37:14  hmgr
 * Author: lkc@hpmort3.fc.hp.com
 * added EXABYTE support and resolved defects 
 * DSDe411763 and DSDe412053
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/src.rcs/etc/fbackup/main.c    70.2
 * /hpux/src.rcs/etc/fbackup/search.c    70.4
 * /hpux/src.rcs/etc/fbackup/vdi.c    70.6.1.1
 * /hpux/src.rcs/etc/fbackup/vdi.h    70.4
 * /hpux/src.rcs/etc/fbackup/writer.c    72.2
 * /hpux/src.rcs/etc/fbackup/writer2.c    70.3
 * 
 * Revision 72.9  93/04/07  14:54:12  14:54:12  ssa (shared source admin)
 * Author: shemanin@hpucsb2.cup.hp.com
 * Fixed DSDe410083 and relative defects.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/flist.c    72.2
 * 
 * Revision 72.8  93/03/26  12:54:13  12:54:13  ssa (RCS Manager)
 * Author: shemanin@hpucsb2.cup.hp.com
 * UCSqm00941 fixed. Fbackup couldn't write into files, those names
 * look like /xxxxxx i.e. in root directory.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/parse.c    72.4
 * 
 * Revision 72.7  93/03/17  13:39:53  13:39:53  ssa (RCS Manager)
 * Author: shemanin@hpucsb3.cup.hp.com
 * CSBlm00410 fixed, parsing for graph file was slightly
 * improved.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/parse.c    72.3
 * 
 * Revision 72.6  92/12/17  14:37:16  14:37:16  ssa (RCS Manager)
 * Author: eric@hpucsb2.cup.hp.com
 * Added mods for metrics.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/makefile    72.1
 * 
 * Revision 72.5  92/11/16  10:33:39  10:33:39  ssa (RCS Manager)
 * Author: finz@hpfclj.fc.hp.com
 * Changes for V.4 filesystem layout.
 * Files changed:
 * 	parse.c
 * 	rmt.c
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/parse.c    72.2
 * 
 * Revision 72.4  92/10/08  16:50:56  16:50:56  ssa (RCS Manager)
 * Author: kumaran@hpucsb2.cup.hp.com
 * Fixed DTS # DSDe407937. When a tape error
 * occurs and if the bad tape is unsalvageable
 * writer process will request for a new tape
 * and signal the main process to send the index.
 * The main process was incrementing the volume numbers
 * after updateafter, for each change of tape though
 * the tapes were discarded.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/flist.c    72.1
 * 
 * Revision 72.3  92/10/08  16:49:39  16:49:39  ssa (RCS Manager)
 * Author: kumaran@hpucsb2.cup.hp.com
 * Fixed DTS # DSDe407937.
 * The checkpoint logic is changed to put the proper
 * values for the volume checkpoint. After a bad tape
 * the volume checkpoint is not recomputed.
 * Also the output when vflag is set is changed when there
 * bad tape.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/writer.c    72.1
 * 
 * Revision 72.2  92/10/08  16:48:39  16:48:39  ssa (RCS Manager)
 * Author: kumaran@hpucsb2.cup.hp.com
 * Fixed DTS # DSDe407937.
 * reset() function is changed to skip the exact number of
 * blocks (as blocks/record can vary)
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/reset.c    72.1
 * 
 * Revision 72.1  92/10/08  16:47:31  16:47:31  ssa (RCS Manager)
 * Author: kumaran@hpucsb2.cup.hp.com
 * Fixed DTS # DSDe407704. hostname was null
 * terminated in get_host()
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/rmt.c    72.1
 * 
 * Revision 70.38  92/09/03  13:08:30  13:08:30  ssa (RCS Manager)
 * Author: kumaran@15.0.56.21
 * Fixes DTS #DSDe407421. The fix was done in the
 * routine expanddir(). The code reading the
 * directory entries has to make a check if
 * the entry read is an actual file, because this
 * could happen in the case of a deleted CDF and if
 * -H is not specified.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/search.c    70.3
 * 
 * Revision 70.37  92/08/24  16:07:56  16:07:56  ssa (RCS Manager)
 * Author: kumaran@hpucsb2.cup.hp.com
 * This fixes DTS # DSDe407588 for 300 systems.
 * fbackup was failing to backup the contents of
 * the directory when -i <directory_name> was given
 * in the command line. pathcmp() function of revision
 * 70.1 is modified and used here.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/flist.c    70.4
 * 
 * Revision 70.36  92/08/14  16:43:45  16:43:45  ssa (RCS Manager)
 * Author: tuan@hpucsb2.cup.hp.com
 * Add ifndef for MT_ISQIC for compiling the latest fbackup code on
 * 8.0 systems.  We can keep a single soruce tree for the fb/fr code.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/vdi.c    70.6
 * 
 * Revision 70.35  92/08/14  00:36:09  00:36:09  ssa (RCS Manager)
 * Author: tuan@hpucsb2.cup.hp.com
 * Add logic to check the enforcement lock condition.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/main2.c    70.2
 * /hpux/shared/supp/usr/src/cmd/fbackup/reader.c    70.2
 * /hpux/shared/supp/usr/src/cmd/fbackup/writer.c    70.5
 * 
 * Revision 70.34  92/08/14  00:27:30  00:27:30  ssa (RCS Manager)
 * Author: tuan@hpucsb2.cup.hp.com
 * Add logic to check the enforcement lock condition.
 * 
 * Revision 70.33  92/07/13  15:43:52  15:43:52  ssa
 * Author: venky@hpucsb2.cup.hp.com
 * The volume header information has been changed to have the 
 * correct spelling for a word - 'identification' instead of 
 * the wrong 'indentification'.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/flist.c    70.3
 * 
 * Revision 70.32  92/06/11  18:46:38  18:46:38  ssa (RCS Manager)
 * Author: tuan@hpucsb2.cup.hp.com
 * Add QIC support. (For QIC-525 format only)
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/writer2.c    70.2
 * 
 * Revision 70.31  92/06/11  18:45:40  18:45:40  ssa (RCS Manager)
 * Author: tuan@hpucsb2.cup.hp.com
 * Add QIC support.  (For QIC-525 format only)
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/writer.c    70.4
 * 
 * Revision 70.30  92/06/11  18:44:36  18:44:36  ssa (RCS Manager)
 * Author: tuan@hpucsb2.cup.hp.com
 * Add QIC support.  (For QIC-525 only)
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/vdi.c    70.5
 * 
 * Revision 70.29  92/04/21  11:03:52  11:03:52  ssa (RCS Manager)
 * Author: venky@hpucsb2.cup.hp.com
 * The Code for Default Mag tape selection has been withdrawn for
 * the present since it affects other major numbers and raw devices
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/vdi.c    70.4
 * /hpux/shared/supp/usr/src/cmd/fbackup/vdi.h    70.3
 * /hpux/shared/supp/usr/src/cmd/fbackup/writer.c    70.3
 * 
 * Revision 70.28  92/04/09  14:22:58  14:22:58  ssa (RCS Manager)
 * Author: venky@hpucsb2.cup.hp.com
 * The following changes are contained :
 * A new pathname comparison routine (flist.c), A bug fix for memory release 
 * (search.c),SCSI Mag tape recognition and Code to support default Mag tape
 * (vdi.c,writer.c,vdi.h)
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/search.c    70.2
 * /hpux/shared/supp/usr/src/cmd/fbackup/flist.c    70.2
 * /hpux/shared/supp/usr/src/cmd/fbackup/vdi.c    70.3
 * /hpux/shared/supp/usr/src/cmd/fbackup/writer.c    70.2
 * /hpux/shared/supp/usr/src/cmd/fbackup/vdi.h    70.2
 * 
 * Revision 70.27  92/01/31  15:20:18  15:20:18  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Merged changes to 8.0X patch, and incorporated new REMOTE
 * cases for future releases' further transparancy.  (Same
 * chages were incorporated in frecover for the patch).
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/vdi.c    70.2
 * 
 * Revision 70.26  92/01/29  10:26:06  10:26:06  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Changed the string of patch ID 1
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/flist.c    66.8
 * /hpux/shared/supp/usr/src/cmd/fbackup/main.c    66.9
 * /hpux/shared/supp/usr/src/cmd/fbackup/search.c    66.6
 * 
 * Revision 70.25  92/01/29  10:24:09  10:24:09  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Corrected fbackup problems on NFS mounted files and
 * changed offensive messages to friendlier ones
 * 
 * Revision 70.24  91/12/20  12:17:20  12:17:20  ssa
 * Author: dickermn@hpucsb2.cup.hp.com
 *  Added extra check to classify 400-series computers as 300-series,
 * so that remote backups will recognize the remote machine.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/writer.c    66.9
 * 
 * Revision 70.23  91/12/20  12:16:39  12:16:39  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Added extra check to classify 400-series computers as 300-series,
 * so that remote backups will recognize the remote machine.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/rmt.c    66.4
 * 
 * Revision 70.22  91/11/22  15:13:21  15:13:21  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Added AUTOCH to s700 table, so that MO autochangers are recognized
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/vdi.c    66.3.1.2
 * 
 * Revision 70.21  91/11/22  15:05:55  15:05:55  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Added AUTOCH to s700 table, so that MO autochangers are recognized
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/vdi.c    66.3.1.2
 * 
 * Revision 70.20  91/11/06  23:10:17  23:10:17  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Changes to avoid malloc typcasting warnings on complie and
 * Corrected the GMT_SM (setmark bit) assignment from im_report to
 *   tm2 for getting attributes from the DDS drive
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/vdi.c    66.3.1.1
 * 
 * Revision 70.19  91/11/06  17:55:55  17:55:55  ssa (RCS Manager)
 * Author: dickermn@hpucsb3.cup.hp.com
 * Added updateidx call, as per the ifdefs just removed at last rev
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/flist.c    66.7
 * 
 * Revision 70.18  91/11/06  13:38:18  13:38:18  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Removed references/ifdefs referring to TAI
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/writer.c    66.8
 * /hpux/shared/supp/usr/src/cmd/fbackup/flist.c    66.6
 * /hpux/shared/supp/usr/src/cmd/fbackup/head.h    66.9
 * /hpux/shared/supp/usr/src/cmd/fbackup/makefile    66.4
 * /hpux/shared/supp/usr/src/cmd/fbackup/main.c    66.8
 * /hpux/shared/supp/usr/src/cmd/fbackup/main2.c    66.5
 * /hpux/shared/supp/usr/src/cmd/fbackup/tape.c    66.4
 * /hpux/shared/supp/usr/src/cmd/fbackup/search.c    66.5
 * /hpux/shared/supp/usr/src/cmd/fbackup/inex.c    66.3
 * 
 * Revision 70.17  91/11/05  15:11:14  15:11:14  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/main.c    66.7
 * /hpux/shared/supp/usr/src/cmd/fbackup/main2.c    66.4
 * 
 * Revision 70.16  91/11/05  15:09:10  15:09:10  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/writer2.c    66.3
 * 
 * Revision 70.15  91/11/05  15:08:53  15:08:53  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/writer.c    66.7
 * 
 * Revision 70.14  91/11/05  15:07:41  15:07:41  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/vdi.c    70.2
 * 
 * Revision 70.13  91/11/05  15:07:28  15:07:28  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/util.c    66.3
 * 
 * Revision 70.12  91/11/05  15:06:59  15:06:59  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/tape.c    66.3
 * 
 * Revision 70.11  91/11/05  15:06:07  15:06:07  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/search.c    66.4
 * 
 * Revision 70.10  91/11/05  15:05:35  15:05:35  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/rmt.c    66.3
 * 
 * Revision 70.9  91/11/05  15:05:21  15:05:21  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/reset.c    66.2
 * 
 * Revision 70.8  91/11/05  15:05:07  15:05:07  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/reader.c    66.1
 * 
 * Revision 70.7  91/11/05  15:04:10  15:04:10  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/pwgr.c    66.1
 * 
 * Revision 70.6  91/11/05  15:03:49  15:03:49  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/parse.c    66.6
 * 
 * Revision 70.5  91/11/05  15:03:26  15:03:26  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * no comment
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/main3.c    66.3
 * 
 * Revision 70.4  91/11/05  14:59:24  14:59:24  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Changes to list structures and macros to conserve memory 
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/inex.c    66.2
 * 
 * Revision 70.3  91/11/05  14:58:13  14:58:13  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Changes to macros for string compares and static memory allocated
 * for filenames, and changes to structure dtabentry for hash tables
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/head.h    66.8
 * 
 * Revision 70.2  91/11/05  14:55:11  14:55:11  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Memory allocation more efficient
 * Locatlity of reference to improve searching performance
 * Open to DDS tape changed to avoid error when reading volume header
 *  from fresh (completely blank) tape.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/flist.c    66.5
 * 
 * Revision 70.1  91/09/11  08:21:54  08:21:54  hmgr (History Manager)
 * Author: lkc@hpfclkc.fc.hp.com
 * changed lseek from < 0 to == -1 for SDS.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/vdi.c    70.1
 * 
 * Revision 66.21  91/03/01  08:06:06  08:06:06  ssa (RCS Manager)
 * Author: danm@hpbblc.bbn.hp.com
 * fixes to MO and DAT support
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/flist.c    66.4
 * /hpux/shared/supp/usr/src/cmd/fbackup/head.h    66.7
 * /hpux/shared/supp/usr/src/cmd/fbackup/main2.c    66.3
 * /hpux/shared/supp/usr/src/cmd/fbackup/main3.c    66.2
 * /hpux/shared/supp/usr/src/cmd/fbackup/parse.c    66.5
 * /hpux/shared/supp/usr/src/cmd/fbackup/rmt.c    66.2
 * /hpux/shared/supp/usr/src/cmd/fbackup/search.c    66.3
 * /hpux/shared/supp/usr/src/cmd/fbackup/tape.c    66.2
 * /hpux/shared/supp/usr/src/cmd/fbackup/vdi.c    66.3
 * /hpux/shared/supp/usr/src/cmd/fbackup/writer.c    66.6
 * /hpux/shared/supp/usr/src/cmd/fbackup/writer2.c    66.2
 * 
 * Revision 66.20  91/02/11  16:30:48  16:30:48  ssa (Shared Source Administrator)
 * Author: lkc@hpfclj
 * removed reference to disc - size_info struct.  It is not used and causes
 * fbackup to fail its compile on IF1.
 * 
 * *** The Following Files Were Modified: ***
 * /hpux/shared/supp/usr/src/cmd/fbackup/vdi.c    66.2
 * 
 * Revision 66.19  91/01/17  14:51:25  14:51:25  danm
 * ******* The Following Modules Were Modified *******
 *     vdi.h,v
 * 
 * Revision 66.18  91/01/17  14:44:16  14:44:16  danm (#Dan Matheson)
 * ******* The Following Modules Were Modified *******
 *     vdi.c,v
 * 
 * Revision 66.17  91/01/17  14:36:48  14:36:48  danm (#Dan Matheson)
 * ******* The Following Modules Were Modified *******
 *     writer2.c,v
 *     writer.c,v
 *     tape.c,v
 *     search.c,v
 *     rmt.c,v
 *     parse.c,v
 *     makefile,v
 *     main3.c,v
 *     main2.c,v
 *     main.c,v
 *     inex.c,v
 *     head.h,v
 * 
 * Revision 66.16  91/01/17  13:41:20  13:41:20  danm (#Dan Matheson)
 * ******* The Following Modules Were Modified *******
 *     flist.c,v
 * 
 * Revision 66.15  90/11/30  08:04:24  08:04:24  danm (#Dan Matheson)
 *  cleaned up what string to remove patch words
 * 
 * Revision 66.14  90/10/01  11:29:17  11:29:17  egeland
 * Removed explicit declaration of "sbrk()".  In <unistd.h>, sbrk went
 * from "char *" to "void *".  The routine does not appear to be used
 * ******* The Following Modules Were Modified *******
 *     main.c,v
 * 
 * Revision 66.13  90/10/01  10:26:27  10:26:27  danm
 *  syncing of main branch with patches from 66.1.1.1 branch
 * ******* The Following Modules Were Modified *******
 *     main.c,v
 * 
 * Revision 66.12  90/09/21  16:58:52  16:58:52  egeland
 * Very minor changes to get rid of compiler warnings regarding missing
 * type specifiers.
 * ******* The Following Modules Were Modified *******
 *     reset.c,v
 *     main.c,v
 * 
 * Revision 66.11  90/05/11  10:51:33  10:51:33  egeland (Mark Egeland)
 * Added "fbackup.cat" to clobber target so that "make clobber" would
 * work as advertised.
 * ******* The Following Modules Were Modified *******
 *     makefile,v
 * 
 * Revision 66.10  90/05/11  07:38:47  07:38:47  danm
 * ******* The Following Modules Were Modified *******
 *     parse.c,v
 * 
 * Revision 66.9  90/05/10  08:12:46  08:12:46  danm (#Dan Matheson)
 * ******* The Following Modules Were Modified *******
 *     parse.c,v
 * 
 * Revision 66.8  90/05/09  07:26:55  07:26:55  danm (#Dan Matheson)
 * ******* The Following Modules Were Modified *******
 *     flist.c,v
 * 
 * Revision 66.7  90/05/09  01:49:23  01:49:23  danm (#Dan Matheson)
 * ******* The Following Modules Were Modified *******
 *     parse.c,v
 *     flist.c,v
 *     main.c,v
 * 
 * Revision 66.6  90/04/02  14:26:37  14:26:37  danm (#Dan Matheson)
 * ******* The Following Modules Were Modified *******
 *     util.c,v
 * 
 * Revision 66.5  90/04/01  16:08:44  16:08:44  lkc
 * removed declaration of malloc since malloc.h is now included in head.h
 * This is required to let fbackup compile
 * ******* The Following Modules Were Modified *******
 *     util.c,v
 * 
 * Revision 66.4  90/03/30  18:50:58  18:50:58  danm
 * ******* The Following Modules Were Modified *******
 *     head.h,v
 * 
 * Revision 66.3  90/03/22  15:55:36  15:55:36  danm (#Dan Matheson)
 *  special what string for a patch version
 * 
 * Revision 66.2  90/02/20  18:24:07  18:24:07  danm (#Dan Matheson)
 * ******* The Following Modules Were Modified *******
 *     main.c,v
 *     search.c,v
 *     main2.c,v
 *     head.h,v
 * 
 * Revision 66.1  89/10/26  09:22:40  09:22:40  kamei
 * The following modules were modified:
 *     writer.c,v
 * 
 * Revision 64.15  89/08/11  13:41:54  13:41:54  lkc
 * The following modules were modified:
 *     tape.c,v
 * 
 * Revision 64.14  89/08/11  13:40:03  13:40:03  lkc (Lee Casuto)
 * The following modules were modified:
 *     rmt.c,v
 * 
 * Revision 64.13  89/08/11  13:38:39  13:38:39  lkc (Lee Casuto)
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 64.12  89/07/24  00:08:28  00:08:28  kamei (Masatoshi Kamei)
 * The following modules were modified:
 *     pwgr.c,v
 * 
 * Revision 64.11  89/07/20  03:38:21  03:38:21  takiya (Makoto Takaiya)
 * The following modules were modified:
 *     main.c,v
 * 
 * Revision 64.10  89/07/20  03:18:26  03:18:26  takiya (Makoto Takaiya)
 * The following modules were modified:
 *     flist.c,v
 * 
 * Revision 64.9  89/05/24  01:04:44  01:04:44  kazu (Kazuhisa Yokoto)
 * The following modules were modified:
 *     reader.c,v
 * 
 * Revision 64.8  89/05/19  00:19:21  00:19:21  kazu (Kazuhisa Yokoto)
 * The following modules were modified:
 *     writer2.c,v
 *     util.c,v
 *     main.c,v
 * 
 * Revision 64.7  89/02/18  15:11:19  15:11:19  jh (June Hsueh)
 * The following modules were modified:
 *     writer.c,v
 *     search.c,v
 *     reader.c,v
 * 
 * Revision 64.6  89/02/07  01:52:19  01:52:19  kazu
 * The following modules were modified:
 *     rmt.c,v
 * 
 * Revision 64.5  89/02/06  13:57:34  13:57:34  rsh (R Scott Holbrook)
 * The following modules were modified:
 *     Makefile,v
 * 
 * Revision 64.4  89/02/02  23:24:43  23:24:43  kazu (Kazuhisa Yokoto)
 * The following modules were modified:
 *     writer.c,v
 *     tape.c,v
 *     search.c,v
 *     parse.c,v
 *     makefile,v
 * 
 * Revision 64.3  89/01/24  17:20:21  17:20:21  lkc (Lee Casuto)
 * The following modules were modified:
 *     flist.c,v
 * 
 * Revision 64.2  89/01/19  16:49:21  16:49:21  bkh (Brian Hoyt)
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 64.1  89/01/17  11:07:26  11:07:26  benji (Jeff Benjamin)
 * The following modules were modified:
 *     makefile,v
 * 
 * Revision 63.12  88/11/09  11:04:16  11:04:16  lkc (Lee Casuto)
 * The following modules were modified:
 *     main3.c,v
 * 
 * Revision 63.11  88/10/24  15:52:44  15:52:44  lkc (Lee Casuto)
 * The following modules were modified:
 *     search.c,v
 * 
 * Revision 63.10  88/10/14  15:34:38  15:34:38  lkc (Lee Casuto)
 * The following modules were modified:
 *     main3.c,v
 * 
 * Revision 63.9  88/09/23  15:38:18  15:38:18  lkc (Lee Casuto)
 * The following modules were modified:
 *     tape.c,v
 * 
 * Revision 63.8  88/09/22  15:50:14  15:50:14  lkc (Lee Casuto)
 * The following modules were modified:
 *     head.h,v
 * 
 * Revision 63.7  88/09/22  15:48:22  15:48:22  lkc (Lee Casuto)
 * The following modules were modified:
 *     main3.c,v
 * 
 * Revision 63.6  88/09/22  15:43:43  15:43:43  lkc (Lee Casuto)
 * The following modules were modified:
 *     parse.c,v
 * 
 * Revision 63.5  88/09/19  15:41:38  15:41:38  lkc (Lee Casuto)
 * The following modules were modified:
 *     parse.c,v
 * 
 * Revision 63.4  88/09/19  15:39:19  15:39:19  lkc (Lee Casuto)
 * The following modules were modified:
 *     main.c,v
 * 
 * Revision 63.3  88/09/19  15:35:14  15:35:14  lkc (Lee Casuto)
 * The following modules were modified:
 *     head.h,v
 * 
 * Revision 63.2  88/09/16  16:03:16  16:03:16  lkc (Lee Casuto)
 * The following modules were modified:
 *     main3.c,v
 * 
 * Revision 63.1  88/09/07  17:35:00  17:35:00  lkc (Lee Casuto)
 * The following modules were modified:
 *     fbackup.mk,v
 * 
 * Revision 62.27  88/09/07  17:27:48  17:27:48  lkc (Lee Casuto)
 * The following modules were modified:
 *     writer.c,v
 * 
 * Revision 62.26  88/09/07  17:18:40  17:18:40  lkc (Lee Casuto)
 * The following modules were modified:
 *     reader.c,v
 * 
 * Revision 62.25  88/09/07  17:13:02  17:13:02  lkc (Lee Casuto)
 * The following modules were modified:
 *     search.c,v
 * 
 * Revision 62.24  88/09/07  17:04:17  17:04:17  lkc (Lee Casuto)
 * The following modules were modified:
 *     reader.c,v
 * 
 * Revision 62.23  88/09/07  16:55:59  16:55:59  lkc (Lee Casuto)
 * The following modules were modified:
 *     main.c,v
 * 
 * Revision 62.22  88/09/07  15:53:45  15:53:45  lkc (Lee Casuto)
 * The following modules were modified:
 *     writer.c,v
 * 
 * Revision 62.21  88/09/07  15:51:49  15:51:49  lkc (Lee Casuto)
 * The following modules were modified:
 *     search.c,v
 * 
 * Revision 62.20  88/09/07  15:49:18  15:49:18  lkc (Lee Casuto)
 * The following modules were modified:
 *     reader.c,v
 * 
 * Revision 62.19  88/09/07  15:44:54  15:44:54  lkc (Lee Casuto)
 * The following modules were modified:
 *     main.c,v
 * 
 * Revision 62.18  88/08/24  14:58:41  14:58:41  lkc (Lee Casuto)
 * The following modules were modified:
 *     main2.c,v
 * 
 * Revision 62.17  88/08/16  09:42:12  09:42:12  lkc (Lee Casuto[Ft.Collins])
 * The following modules were modified:
 *     tape.c,v
 * 
 * Revision 62.16  88/08/12  16:21:48  16:21:48  lkc (Lee Casuto[Ft.Collins])
 * The following modules were modified:
 *     search.c,v
 * 
 * Revision 62.15  88/08/12  16:19:09  16:19:09  lkc (Lee Casuto[Ft.Collins])
 * The following modules were modified:
 *     main.c,v
 * 
 * Revision 62.14  88/08/12  16:18:04  16:18:04  lkc (Lee Casuto[Ft.Collins])
 * The following modules were modified:
 *     reader.c,v
 * 
 * Revision 62.13  88/08/12  16:17:13  16:17:13  lkc (Lee Casuto[Ft.Collins])
 * The following modules were modified:
 *     writer.c,v
 * 
 * Revision 62.12  88/08/12  16:16:20  16:16:20  lkc (Lee Casuto[Ft.Collins])
 * The following modules were modified:
 *     fbackup.mk,v
 * 
 * Revision 62.11  88/08/12  16:15:07  16:15:07  lkc (Lee Casuto[Ft.Collins])
 * The following modules were modified:
 *     Makefile,v
 * 
 * Revision 62.10  88/07/26  17:35:50  17:35:50  lkc (Lee Casuto[Ft.Collins])
 * The following modules were modified:
 *     search.c,v
 * 
 * Revision 62.9  88/07/26  17:33:01  17:33:01  lkc (Lee Casuto[Ft.Collins])
 * The following modules were modified:
 *     fbackup.mk,v
 * 
 * Revision 62.8  88/07/19  09:30:17  09:30:17  lkc (Lee Casuto[Ft.Collins])
 * The following modules were modified:
 *     Makefile,v
 * 
 * Revision 62.7  88/07/12  17:01:49  17:01:49  peteru (Peter Uhlemann)
 * The following modules were modified:
 *     parse.c,v
 * 
 * Revision 62.6  88/07/12  16:58:01  16:58:01  peteru (Peter Uhlemann)
 * The following modules were modified:
 *     main.c,v
 * 
 * Revision 62.5  88/04/25  10:39:04  10:39:04  pvs
 * Merged branch deltas back into the main trunk of the tree.
 * The following modules were modified:
 *     head.h	(62.2)
 *     main.c	(62.3)
 *     main2.c	(62.3)
 *     parse.c	(62.3;62.4)
 *     search.c	(62.3)
 *     tape.c	(62.3)
 *     writer.c	(62.4)
 * 
 * Revision 62.4  88/04/05  21:13:24  21:13:24  carolyn
 * The following modules were modified:
 *     inex.c,v
 * 
 * Revision 62.3  88/04/05  21:12:40  21:12:40  carolyn (Carolyn Sims)
 * The following modules were modified:
 *     flist.c,v
 * 
 * Revision 62.2  88/02/08  17:33:43  17:33:43  sandee (Sandee Hoag)
 * Added to a msg to print the errno number from sys/errno.h to track the
 * error of an obscure shmat error.
 * 
 * Revision 62.1  88/02/08  14:29:40  14:29:40  sandee (Sandee Hoag)
 * Added an error catcher to exit gracefully when mymalloc signals
 * that we're out of virtual memory.
 * 
 * Revision 56.5  88/01/13  09:53:40  09:53:40  crook (Jim Crook)
 * Backed out the (erroneous) change which caused all directories to be backed
 * up on incremental dumps (irrespective of their ctime and mtime values).
 * Also split the function restorestate into two functions, restoresession
 * (which is before the writer process is forked) and restoreflist (which is
 * after this fork).  The function s_cmp was removed from the file search.c.
 * It was one extra (and unnecessary) level of function call for the call
 * to qsort.  Some comments were added and updated too.  Two unnecessary case
 * statements were removed from the switch statement in wrtrhandler, in the
 * file writer.c
 * 
 * Revision 56.4  87/12/11  16:17:20  16:17:20  sandee (Sandee Hoag)
 * Added a line in flist.c--add_flistnode to add directories to the flist
 * and changed the tape message to say 'ANSII labels not yet implemented'.
 * 
 * Revision 56.3  87/11/25  15:04:29  15:04:29  crook (Jim Crook)
 * Fbackup now builds does its forks BEFORE building the file-list (flist)
 * so that it doesn't run out of virtual memory.
 * 
 * Revision 56.2  87/11/18  10:34:47  10:34:47  crook (Jim Crook)
 * Refinded search algorithm and made some other enhancements.
 * 
 * Revision 56.1  87/11/05  08:26:19  08:26:19  crook (Jim Crook)
 * Completed next phase of development.
 * 
 * Revision 51.3  87/10/12  18:33:08  18:33:08  crook (Jim Crook)
 * *** empty log message ***
 * 
 * Revision 51.2  87/09/23  16:59:08  16:59:08  crook (Jim Crook)
 * Code now lints and runs on a 300.
 * 
 * Revision 51.1  87/09/15  15:02:43  15:02:43  crook (Jim Crook)
 * Fbackup is NOT finished yet.  It should be soon, but don't be surprised
 * if some functionality is less than optimal.
 * 
*/


/***************************************************************************
****************************************************************************

	hpux_rel.c

    This file is only present to keep RCS information on fbackup.


****************************************************************************
***************************************************************************/
