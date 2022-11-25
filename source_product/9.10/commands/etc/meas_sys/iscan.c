/* $Source: /misc/source_product/9.10/commands.rcs/etc/meas_sys/iscan.c,v $
 * $Revision: 70.1 $		$Author: ssa $
 * $State: Exp $		$Locker:  $
 * $Date: 92/05/12 15:12:46 $
 */
/*
 * Copyright (c) 1990 Hewlett Packard Co.a
 * All Rights Reserved.
 *
 * This is unpublished proprietary source code of Hewlett Packard Co.
 * The copyright notice above does not evidence any actual
 * or intended publication of such source code.
 *
 * ================================================================
 *
 * This is code is for internal HP use.
 * It is not supported for normal customer use.
 * Please send bug reports and enhancement requests to dah@hpda.hp.com
 *
 * WARNING:  iscan is a very complicated program.  It's distribution
 * is restricted even within HP.  Please do not attempt
 * to change it.  You will very likely break it and no one will be
 * interested in helping you with a modified version.
 *
 * Other files (msmod, translate, untranslate) are more tractable.
 * Also be sure to look at programs such as xray3 for examples
 * of more modern analysis (e.g. faster MS record parsing).
 */

/*
 * $Source: /misc/source_product/9.10/commands.rcs/etc/meas_sys/iscan.c,v $
 * $Revision: 70.1 $		$Author: ssa $
 * $State: Exp $		$Locker:  $
 * $Date: 92/05/12 15:12:46 $
 *
 * $Log:	iscan.c,v $
 * Revision 70.1  92/05/12  15:12:46  15:12:46  ssa
 * Author: alexl@hpucsb2.cup.hp.com
 * Latest source from Dave Holt to fix UCSqm00807.
 * 
 *
 * Revision 64.1  89/04/28  16:54:13  16:54:13  marcs
 *  underscore fix for 300.
 * 
 * Revision 4.47  91/07/12  16:54:34  16:54:34  dah (Dave Holt)
 * 8.02 looks ok as is.  Apparently the kernel had more symbols in it
 * than 8.05 did.
 * 
 * Revision 4.46  91/07/08  10:59:38  10:59:38  dah (Dave Holt)
 * Now it should not only compile, but also run!
 * (nlst was failing on snakes (8.02 also?) because it was
 * looking for symbols it wasn't going to use anyway!
 * 
 * Revision 4.45  90/05/29  11:01:18  11:01:18  dah (Dave Holt)
 * add copyright notice
 * 
 * Revision 4.44  89/12/12  12:46:49  12:46:49  dah (Dave Holt)
 * correct usage message (-W needs an argument)
 * 
 * Revision 4.43  89/12/08  16:26:34  16:26:34  dah (Dave Holt)
 * make default max write size magtape compatible
 * 
 * Revision 4.42  89/12/08  16:25:39  16:25:39  dah (Dave Holt)
 * merge in 4.41.1 branch (support writes to magtape)
 * 
 * Revision 4.41.1.3  89/12/04  16:29:57  16:29:57  dah (Dave Holt)
 * still buggyy .  add more diagnostics
 * 
 * Revision 4.41.1.2  89/12/04  16:11:14  16:11:14  dah (Dave Holt)
 * fix mixup between bytes and words
 * 
 * Revision 4.41.1.1  89/12/04  15:43:04  15:43:04  dah (Dave Holt)
 * try new -W option to default max write size (magtape was having
 * problems)
 * 
 * Revision 4.41  89/03/27  15:30:46  15:30:46  dah (Dave Holt)
 * make there be underscores unless you're on a s800 (spectrum)
 * 
 * Revision 4.40  88/07/25  13:41:16  13:41:16  dah (Dave Holt)
 * made complaints about lost buffer (but not lost data) chatty messages
 * (turned off with -q)
 * 
 * Revision 4.39  88/07/25  13:29:48  13:29:48  dah (Dave Holt)
 * add -m option to memory lock
 * 
 * Revision 4.38  88/07/22  17:02:26  17:02:26  dah (Dave Holt)
 * Fix bug where -e caused data to be read into my_buf, then grab_old_id_strings()
 * caused it to be slightly trashed before it was written.
 * Fix was by doing grab_old_id_strings before any data is read from the kernel
 * For additional security, changed malloc_if_needed to use realloc when
 * appropriate (which maintains data integrity).
 * 
 * Revision 4.37  87/03/16  11:05:54  11:05:54  dah (Dave Holt)
 * changes to support s300.
 * pcn)
 * ifndef out nlist entries not used on the s300
 * 
 * Revision 4.36  87/03/10  14:22:04  14:22:04  dah (Dave Holt)
 * fix bug in grab_old_id_strings where if last non-nul char of string
 * ended on word boundary, nul didn't get posted.
 * 
 * Revision 4.35  87/02/24  12:07:46  12:07:46  dah (Dave Holt)
 * handle new common.c
 * 
 * Revision 4.34.1.1  87/02/24  12:05:13  12:05:13  dah (Dave Holt)
 * try fixing bug relating to new grab_name
 * 
 * Revision 4.34.2.1  87/03/16  10:54:07  10:54:07  dah (Dave Holt)
 * CND version (works on s300)
 * 
 * Revision 4.34  86/08/28  14:01:12  14:01:12  dah (Dave Holt)
 * check in after testing branch.
 * handle counts in kernel > 2048 mega words.
 * print better debugging info out if i find a corrupt record.
 * 
 * Revision 4.33.1.7  86/08/26  16:58:02  16:58:02  dah (Dave Holt)
 * seems to dump useful data when I poke it to cause the error with
 * xdb.
 * 
 * Revision 4.33.1.6  86/08/26  16:48:51  16:48:51  dah (Dave Holt)
 * also print out data in hex
 * 
 * Revision 4.33.1.5  86/08/26  16:34:18  16:34:18  dah (Dave Holt)
 * fix typo
 * 
 * Revision 4.33.1.4  86/08/26  16:29:55  16:29:55  dah (Dave Holt)
 * try handling corrupt data better by dumping state info
 * 
 * Revision 4.33.1.3  86/08/12  17:00:44  17:00:44  dah (Dave Holt)
 * handle get_int_3 and read_buffer()
 * 
 * Revision 4.33.1.2  86/08/12  16:54:12  16:54:12  dah (Dave Holt)
 * try first cut an unsigning things
 * 
 * Revision 4.33.1.1  86/08/12  16:43:21  16:43:21  dah (Dave Holt)
 * branch to try moving to unsigned counters
 * 
 * Revision 4.33  86/06/20  09:51:00  09:51:00  dah (Dave Holt)
 * modified to support gr's fixes for reading core files
 * (real work is in common.c).
 * 
 * Revision 4.32  86/06/18  16:27:56  16:27:56  dah (Dave Holt)
 * only block signals across write if I'm going to do something
 * more useful than exit with them
 * 
 * Revision 4.31  86/06/18  15:47:58  15:47:58  dah (Dave Holt)
 * fix typo.
 * 
 * Revision 4.30  86/06/18  15:30:07  15:30:07  dah (Dave Holt)
 * mask SIGINT and SIGTERM during our write.
 * 
 * Revision 4.29  86/06/18  11:14:11  11:14:11  dah (Dave Holt)
 * Fix DSDtg00308 .
 * update iscan_ms_buf_cnt after calls to do_read, rather than inside it.
 * Had been messing up other functions, because iscan_ms_buf_cnt had
 * been incremented before checking that the read was ok.
 * Removed the variable 'save' from read_backwards(), since no longer needed.
 * 
 * Revision 4.28  86/06/17  16:33:42  16:33:42  dah (Dave Holt)
 * handle multiple kill signals.
 * (IND was looping sending signals, this kept resetting killed = 1).
 * 
 * Revision 4.27  86/06/17  15:20:52  15:20:52  dah (Dave Holt)
 * fix bug in read_backwards (previously could address off end of
 * array if kernel writing very quickly).
 * 
 * Revision 4.26  86/05/27  13:19:55  13:19:55  dah (Dave Holt)
 * pull dbg line that can't happen (since handle_runstring)
 * hasn't had a chance to set debug =1 yet).
 * 
 * Revision 4.25  86/05/27  12:54:50  12:54:50  dah (Dave Holt)
 * make sure id strings get posted in -o or -a case too.
 * 
 * Revision 4.24  86/05/27  12:26:28  12:26:28  dah (Dave Holt)
 * declare some functions void to please cc (funny, I thought that was
 * what lint(1) was for...)
 * 
 * Revision 4.23  86/05/27  12:22:14  12:22:14  dah (Dave Holt)
 * add assertion for safety in shuffle_and_write
 * 
 * Revision 4.22  86/05/27  12:11:20  12:11:20  dah (Dave Holt)
 * attempt to do better in read_backwards - still confusing.
 * 
 * Revision 4.21  86/05/27  10:57:14  10:57:14  dah (Dave Holt)
 * add missing parens
 * 
 * Revision 4.20  86/05/12  17:16:14  dah (Dave Holt)
 * change UNDERSCORES to UNDERSCORE
 * 
 * Revision 4.19  86/05/12  15:51:19  dah (Dave Holt)
 * Handle underscore roll.
 * 
 * Revision 4.18  86/03/25  16:28:49  dah (Dave Holt)
 * seems to work.
 * Now has -o, -a, and -f options
 * 
 * Revision 4.17.1.14  86/03/25  13:29:08  dah (Dave Holt)
 * !rcp
 * fg
 * 
 * Revision 4.17.1.13  86/03/25  11:29:32  dah (Dave Holt)
 * remove erroneous assertion
 * 
 * Revision 4.17.1.12  86/03/25  10:47:18  dah (Dave Holt)
 * fix fencepost (hopefully)
 * 
 * Revision 4.17.1.11  86/03/25  10:15:59  dah (Dave Holt)
 * fix check_data_valid
 * 
 * Revision 4.17.1.10  86/03/25  08:40:14  dah (Dave Holt)
 * mroe debugging
 * 
 * Revision 4.17.1.9  86/03/25  08:32:29  dah (Dave Holt)
 * add more debugging code
 * 
 * Revision 4.17.1.8  86/03/25  08:25:25  dah (Dave Holt)
 * fix shuffle and write to not complain when first != 0
 * 
 * Revision 4.17.1.7  86/03/25  08:10:50  dah (Dave Holt)
 * fix logic error
 * 
 * Revision 4.17.1.6  86/03/25  07:55:18  dah (Dave Holt)
 * fix syntax errors
 * 
 * Revision 4.17.1.5  86/03/24  17:39:07  dah (Dave Holt)
 * try adding real code to read backwards.
 * Not yet tested!
 * 
 * Revision 4.17.1.4  86/03/21  16:39:36  dah (Dave Holt)
 * generalize shuffle_and_write.
 * 
 * Alter existing calls to it to use new syntax.
 * 
 * Revision 4.17.1.3  86/03/21  14:48:05  dah (Dave Holt)
 * fix usage string
 * 
 * Revision 4.17.1.2  86/03/21  14:14:23  dah (Dave Holt)
 * 
 * try adding runstring handling
 * 
 * Revision 4.17.1.1  86/03/21  13:46:59  dah (Dave Holt)
 * branch to add features:
 * 	-o to read old stuff and exit
 * 	-a to read old stuff and continue
 * 	-f to specify file to read (e.g. /dev/kmem), probably a coredump
 * 
 * Revision 4.17  86/02/04  11:12:11  dah (Dave Holt)
 * add some void's to make cc happier (what ever happened to lint?)
 * 
 * Revision 4.16  86/02/04  10:25:27  dah (Dave Holt)
 * pull \n's from bail_out calls
 * 
 * Revision 4.15  86/02/03  17:47:01  dah (Dave Holt)
 * fix DSDtg00112
 * (var_addr() initializes correctly, and do_nlist() no longer
 * called elsewhere).
 * 
 * Revision 4.14.1.1  86/02/03  13:49:16  dah (Dave Holt)
 * try fixing DSDtg000112
 * 
 * Revision 4.14  86/01/31  10:02:50  dah (Dave Holt)
 * changed dbg message to bail_out. (corrupt kernel data)
 * DSDtg00055
 * 
 * Revision 4.13  86/01/30  14:39:57  dah (Dave Holt)
 * iscan will now immediately acknowledge a signal, even with -e .
 * DSDtg00068
 * 
 * Revision 4.12  86/01/27  17:33:43  dah (Dave Holt)
 * reject extra args (DSDtg00042)
 * 
 * Revision 4.11  85/12/11  16:52:46  dah (Dave Holt)
 * 
 * clean up more for lint
 * 
 * Revision 4.10  85/12/10  15:38:37  dah (Dave Holt)
 * this version seems to work with 1.16 common.c
 * 
 * Revision 4.9.1.2  85/12/10  15:29:24  dah (Dave Holt)
 * define my_rev for common.c
 * change get_int_2's to get_int_3's
 * 
 * Revision 4.9.1.1  85/12/10  15:19:24  dah (Dave Holt)
 * pulled out common stuff that is now in common.c
 * 
 * Revision 4.9  85/12/09  15:36:59  dah (Dave Holt)
 * this version appears to work
 * 
 * Revision 4.8.1.5  85/12/06  16:54:56  dah (Dave Holt)
 * try handling note.h
 * 
 * Revision 4.8.1.4  85/12/06  16:34:13  dah (Dave Holt)
 * clean up declaration/initialization of chatty
 * 
 * Revision 4.8  85/12/06  14:33:11  dah (Dave Holt)
 * merge of 4.7.1.1
 * seems to work correctly.
 * 
 * Revision 4.7.1.1  85/12/06  14:03:20  dah (Dave Holt)
 * change bail_out() to do newline at end.
 * make calls to bail_out() consistent in expecting it to do newlines
 * at end.
 * 
 * Revision 4.7  85/12/06  10:58:02  dah (Dave Holt)
 * merge of 4.6.1.2 .
 * This fixes bug dts bug DSDtg00087
 * 
 * Revision 4.6.1.2  85/12/05  16:53:13  dah (Dave Holt)
 * This version works! (it ignores ^C when in the background, but
 * pays attention when it's in the foreground
 * 
 * Revision 4.6.1.1  85/12/05  15:49:25  dah (Dave Holt)
 * attempt to handle ^C in the background correctly.
 * Also document signal handling better.
 * 
 * Revision 4.6  85/12/05  10:26:49  dah (Dave Holt)
 * merge of 4.5.1.5 .  No problems have turned up in IND's use, so check
 * it back into the trunk
 * 
 * Revision 4.5.1.5  85/12/03  16:45:57  dah (Dave Holt)
 * added standard firstci headers
 * 
 * Revision 4.5.1.4  85/11/24  16:39:22  dah (Dave Holt)
 * kill signals shouldn't need to mask out SIGALRM any more
 * (even though they weren't doing it anyway (>> should have been << - oops).
 * 
 * Revision 4.5.1.1  85/11/24  15:35:57  dah (Dave Holt)
 * branch to add reliable signals
 * 
 * Revision 4.5  85/11/21  10:20:48  dah (Dave Holt)
 * changed ifdef BELL to ifdef hpux
 * 
 * Revision 4.4  85/11/21  10:19:23  dah (Dave Holt)
 * *** empty log message ***
 * 
 * Revision 4.4  85/09/16  11:04:18  dah (Dave Holt)
 * e option seems to work, so check it in.
 * 
 * Revision 4.3.1.2  85/09/14  16:59:18  dah (Dave Holt)
 * passes lint
 * 
 * Revision 4.3.1.1  85/09/14  15:54:31  dah (Dave Holt)
 * branch to add -e option (start data capture here, and
 * when killed, read the kernel before exiting).
 * 
 * Revision 4.3  85/09/12  14:33:48  dah (Dave Holt)
 * incorporate changes based on rel8 testing.
 * 
 * Revision 4.1.1.2  85/09/10  16:05:50  dah (Dave Holt)
 * check for runstring options before printing identifier
 * 
 * Revision 4.1  85/08/21  15:11:57  dah (Dave Holt)
 * use <sys/meas_sys.h> instead of <h/meas_sys.h>
 * 
 * Revision 4.0.1.1  85/08/21  14:37:02  dah (Dave Holt)
 * branch to try <sys/meas_sys.h> instead of <h/meas_sys.h>
 * 
 * Revision 4.0  85/08/20  15:41:11  dah (Dave Holt)
 * *** empty log message ***
 * 
 * Revision 3.23  85/08/20  14:50:31  dah (Dave Holt)
 * *** empty log message ***
 * 
 * Revision 3.22.1.14  85/08/13  15:55:51  dah (Dave Holt)
 * pretty much works!!
 * 
 * Revision 3.22.1.13  85/08/13  14:33:21  dah (Dave Holt)
 * added a bunch of fixes relating to words_read
 * 
 * Revision 3.22.1.10  85/08/12  17:47:12  dah (Dave Holt)
 * point to the kernel for meas_sys.h
 * 
 * Revision 3.22.1.9  85/08/12  17:18:53  dah (Dave Holt)
 * cleaned up code in shuffle_and_write
 * 
 * Revision 3.22.1.6  85/08/12  13:34:41  dah (Dave Holt)
 * fixed bug in malloc_if_needed
 * 
 * Revision 3.22.1.4  85/08/12  10:06:33  dah (Dave Holt)
 * added fixes found during debugging
 * 
 * Revision 3.22.1.2  85/08/07  18:13:28  dah (Dave Holt)
 * fixed to work with bell software
 * 
 * Revision 3.22.1.1  85/08/07  17:33:31  dah (Dave Holt)
 * branch for modifications to support movable buffers in kernel
 * 
 * Revision 1.10  85/08/07  17:25:16  dah (Dave Holt)
 * "full functionality"
 * 
 * Revision 1.5  85/08/06  18:03:18  dah (Dave Holt)
 * *** empty log message ***
 * 
 * Revision 1.3  85/08/06  14:50:06  dah (Dave Holt)
 * *** empty log message ***
 * 
 * Revision 1.1  85/08/05  18:41:09  dah (Dave Holt)
 * Initial revision
 * 
 */
/************************************************************************/
/* new code for movable buffers	*/
/************************************************************************/

#include <stdio.h>
#include <nlist.h>
#include <assert.h>
#include <signal.h>
#include <sys/lock.h>
 
#ifdef hpux
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#include <sys/meas_sys.h>
#include "note.h"

typedef unsigned long KMEM_OFFSET;
typedef int BOOL;

/* read from kernel */
int ms_buf_num,
    ms_old_buf_num,
    ms_buf_siz,
    ms_old_buf_siz;
unsigned int ms_buf_cnt,
    ms_old_buf_cnt;
KMEM_OFFSET ms_buf_ptr,
    ms_old_buf_ptr;

#ifdef hp9000s800
#undef UNDERSCORE
#else
#define UNDERSCORE 1
#endif

#ifdef UNDERSCORE
struct nlist nlst[] = {
    { "_ms_buf_num" },
    { "_ms_old_buf_num" },
    { "_ms_buf_siz" },
    { "_ms_old_buf_siz" },
    { "_ms_buf_cnt" },
    { "_ms_old_buf_cnt" },
    { "_ms_buf_ptr" },
    { "_ms_old_buf_ptr" },
    { "_ms_rev" },
    { "_ms_include_rev" },
    { "_ms_id" },
    { "_ms_bin_id" },
    { "_ms_id_str_ptrs" },
    { "_ms_bin_id_str_ptrs" },
#ifndef hp9000s200
    { "_htbl" },
    { "_nhtbl" },
#ifndef SPARSE_PDIR
    { "_pdir" },
    { "_npdir" },
    { "_iopdir" },
    { "_niopdir" },
#endif /* not SPARSE_PDIR */
#endif /* not hp9000s200 */
    { 0 },
};    
#else !UNDERSCORE
struct nlist nlst[] = {
    { "ms_buf_num" },
    { "ms_old_buf_num" },
    { "ms_buf_siz" },
    { "ms_old_buf_siz" },
    { "ms_buf_cnt" },
    { "ms_old_buf_cnt" },
    { "ms_buf_ptr" },
    { "ms_old_buf_ptr" },
    { "ms_rev" },
    { "ms_include_rev" },
    { "ms_id" },
    { "ms_bin_id" },
    { "ms_id_str_ptrs" },
    { "ms_bin_id_str_ptrs" },
#ifndef hp9000s200
    { "htbl" },
    { "nhtbl" },
#ifndef SPARSE_PDIR
    { "pdir" },
    { "npdir" },
    { "iopdir" },
    { "niopdir" },
#endif /* not SPARSE_PDIR */
#endif /* not hp9000s200 */
    { 0 },
};    
#endif UNDERSCORE
    
/* protocol rev for reading the kernel */
#define MY_REV	    	    4
int my_rev = MY_REV;		/* for use in common.c */

/* state of iscan */
int iscan_ms_buf_num,
    iscan_ms_buf_siz = 0,
    words_read,
    state;
unsigned int iscan_ms_buf_cnt;
    
/* possible states */
#define CURRENT_BUF 	    1
#define OLD_BUF	    	    2
#define LOST_DATA   	    3
    
#define	STDOUT 1    

int *my_buf;
char *naptime_str;
float naptime;	    /* would like to be "float naptime = 5.0;" */
int debug = 0;
int wizard = 0;
char *nlist_filename = "/hp-ux";
int kmem;
char    *kmemf = "/dev/kmem";
int kmemf_flg;	/* true iff reading from a core dump */
int chatty = 1;
static int experiment = 0;
static int killed = 0;
static int grab_old_only = 0;
static int grab_all = 0;

int max_write_size = 32768;	/* reasonable default, more sane than 60000 */

/* read from kernel at start */
int ms_id, ms_bin_id;

char *progname = "iscan";

void do_read(), check_data_valid(), do_write();

main(argc, argv)
    int argc;
    char **argv;
{
    setup(argc, argv);

    chat("iscan: $Header: iscan.c,v 70.1 92/05/12 15:12:46 ssa Exp $\n");
    chat("iscan: pid = %d\n", getpid());
        
    while (1) {
	read_data_structures();
	
	check_buf_num();
	
	check_buf_cnt();
	
	do_read();
	
	read_data_structures();
	
	check_data_valid();
	
	do_write();
	
	possibly_sleep();
    }    
}

setup(argc, argv)
    int argc;
    char **argv;
{
    int handle_int(), handle_term();
    struct sigvec vec, ovec;
    
    naptime = 5.0;  /* work around compiler bug */
    
    handle_runstring(argc, argv);    
    
    /*
     * Here, we are using HP reliable signals.
     * The most important reference is the sigvector(OS) man page.
     *
     * SIGINT is normally sent by ^C.  This normally happens when the
     * user is running iscan in the forground.
     *
     * SIGTERM is for when the user runs iscan in the background.
     * It is what is sent when he says 'kill <pid>'.
     *
     * Under normal circumstances we could just handle both of them
     * in a straightforward manner.  Unfortunately, since HP-UX is
     * following Bell in lack of job control, it uses the same
     * crude method of protecting background processes from ^C's.
     * When a program is put into the background, the shell merely
     * sets its signal handler for SIGINT to SIG_IGN, rather than
     * putting it in a separate process group where it belongs.
     * (The Korn shell may do this better than csh or sh).
     *
     * Anyway, as a result, we won't arm a signal handler for SIGINT
     * if its current value is SIG_IGN.
     */

    /* First, pick up current value. */
    if (sigvector(SIGINT, (struct sigvec *) 0, &ovec) != 0) {
	perror("sigvector");
	exit(1);
    }
    if (ovec.sv_handler != SIG_IGN) { /* If it's not currently ignored, */
	vec.sv_handler = handle_int; /* then go ahead and arm our handler. */
	vec.sv_mask = 0;
	vec.sv_onstack = 0;
	if (sigvector(SIGINT, &vec, (struct sigvec *) 0) != 0) {
	    perror("sigvector");
	    exit(1);
	}
    }

    /* In any case, arm the handler for SIGTERM */
    vec.sv_handler = handle_term;
    vec.sv_mask = 0;
    vec.sv_onstack = 0;
    if (sigvector(SIGTERM, &vec, (struct sigvec *) 0) != 0) {
	perror("sigvector");
	exit(1);
    }

    dbg("iscan: new signal handlers armed");

    /*  open kernel memory for reading */
    if ((kmem = open (kmemf, O_RDONLY)) < 0) {
    	perror(kmemf);
	exit(1);
    }
    dbg("opened kernel (%s) successfully", kmemf);
    
    /* do_nlist() now done in var_addr() */

    check_ms_rev();
    dbg("kernel rev ok");

    check_include_rev();
    dbg("include file rev ok");

    if (experiment) {
	/* start reading here */
	read_data_structures();
	iscan_ms_buf_num = ms_buf_num;
	iscan_ms_buf_cnt = ms_buf_cnt;
    } else {
	iscan_ms_buf_num = 0;
	iscan_ms_buf_cnt = 0;
    }

    /* This may not be strictly necessary, but it's better to have
     * the id strings come out twice, than trash the data
     * (as happened previously in some cases)!
     */
    grab_old_id_strings();

    if (grab_old_only || grab_all) {
	read_backwards();
    }
    if (grab_old_only) {
	die("Old stuff grabbed\n");
    }
}

read_data_structures()
{
    dbg("starting read_data_structures");
    note(0);
    
    /* later fix this to be one bigger read */
    ms_buf_num =    	get_int_3("ms_buf_num");
    ms_old_buf_num = 	get_int_3("ms_old_buf_num");
    ms_buf_siz =    	get_int_3("ms_buf_siz");
    ms_old_buf_siz = 	get_int_3("ms_old_buf_siz");
    ms_buf_ptr =    	(KMEM_OFFSET) get_int_3("ms_buf_ptr");
    ms_old_buf_ptr = 	(KMEM_OFFSET) get_int_3("ms_old_buf_ptr");
    ms_buf_cnt =    	(unsigned int) get_int_3("ms_buf_cnt");
    ms_old_buf_cnt = 	(unsigned int) get_int_3("ms_old_buf_cnt");
    
    dbg("ms_buf_num = %d", ms_buf_num);
    dbg("ms_old_buf_num = %d", ms_old_buf_num);
    dbg("ms_buf_siz = %d", ms_buf_siz);
    dbg("ms_old_buf_siz = %d", ms_old_buf_siz);
    dbg("ms_buf_ptr = 0x%x", ms_buf_ptr);         
    dbg("ms_old_buf_ptr = 0x%x", ms_old_buf_ptr);     
    dbg("ms_buf_cnt = %u", ms_buf_cnt);         
    dbg("ms_old_buf_cnt = %u", ms_old_buf_cnt);     
}

check_buf_num()
{
    dbg("starting check_buf_num");
    note(1);
    
    if (current_buf()) {
	state = CURRENT_BUF;
    } else if (old_buf()) {
	state = OLD_BUF;
    } else {
	move_to_next_buffer();
    }
}

check_buf_cnt()
{
    dbg("starting check_buf_cnt");
    note(2);
    
    switch (state) {
	case LOST_DATA:
	    break;
	case CURRENT_BUF:
	    if (current_overflow()) {
		resynch_current();
	    }
	    break;
	case OLD_BUF:
	    if (old_overflow()) {
		move_to_next_buffer();
	    }
	    break;
	default:
	    bail_out("bad case");
    }
}

void 
do_read()
{
    int err;
    long words_in_current_buf(), words_in_old_buf();
    char lg();

    dbg("starting do_read");
    note(3);
    
    switch (state) {
	case CURRENT_BUF:
	    if (current_empty()) {
		words_read = 0;
		chat(".");
		return;
	    }
	    malloc_if_needed(ms_buf_siz);
	    err = read_buffer(ms_buf_ptr, ms_buf_siz,
	    	    	      iscan_ms_buf_cnt, ms_buf_cnt);
	    words_read = words_in_current_buf();
	    chat("%c", lg(words_read));
	    if (err) {
		resynch_current();
	    }
	    break;
	case OLD_BUF:
	    if (old_empty()) {
		words_read = 0;
		chat(".");
		return;
	    }
	    malloc_if_needed(ms_old_buf_siz);
	    err = read_buffer(ms_old_buf_ptr, ms_old_buf_siz,
	    	    	      iscan_ms_buf_cnt, ms_old_buf_cnt);
            words_read = words_in_old_buf();
	    chat("%c", lg(words_read));
	    if (err) {
		move_to_next_buffer();
	    }
	    break;
	case LOST_DATA:
	    words_read = 0;
	    break;
	default:
	    bail_out("bad case");
    }
}

void 
check_data_valid()
{
    dbg("starting check_data_valid");
    note(4);
    
    switch (state) {
	case LOST_DATA:
	    break;
	case OLD_BUF:
	    if (!old_buf()) {
		move_to_next_buffer();
	    }
	    assert (!old_overflow());
	    move_to_next_buffer_happily();
	    state = OLD_BUF;
	    break;
	case CURRENT_BUF:
	    if (current_buf()) {
		if (current_overflow()) {
		    resynch_current();
		} else {
		    iscan_ms_buf_cnt += words_read;
		}
	    } else if (old_buf()) {
		if (old_overflow()) {
		    move_to_next_buffer();
		} else {
		    iscan_ms_buf_cnt += words_read;
		}
	    } else {
		move_to_next_buffer();
	    }
	    break;
	default:
	    bail_out("bad case");
    }
}

void
do_write()
{
    long words_in_current_buf(), words_in_old_buf();

    dbg("starting do_write");
    note(5);
    
    /* if nothing to do */
    if (words_read == 0) {
	return;
    }
    
    switch (state) {
	case CURRENT_BUF:
	case OLD_BUF:
	    shuffle_and_write((long) 0, (long) (words_read -1));
	    break;
	case LOST_DATA:
	    break;
	default:
	    bail_out("bad case");
    }
}

possibly_sleep()
{
    dbg("starting possibly_sleep");
    note(6);
    
    if (state == CURRENT_BUF) {
	if (!killed) {
	    dbg("actually sleeping");
	    nap(naptime);
	} else {
	    /*
	     * When killed, exit only on the second time sleeping here.
	     * If we did it the first time we could have the following
	     * race condition:
	     *     iscan figures out what to read
	     *     more data gets posted in the kernel
	     *     iscan reads data (missing new stuff)
	     *     iscan is killed (with -e option), and sets killed = 1
	     *     iscan writes data
	     *     iscan starts to sleep, but notices it's killed so it dies
	     *        without grabbing the rest of the data.
	     */
	    killed++;
	    if (killed > 2) {
		die("exiting at experiment end\n");
	    }
	}
    } else {
	dbg("not really sleeping");
    }
}

/*
 * Grab data from as far back in the current buffer as we can manage.
 * Since it is still being written from, the oldest data may get,
 * written on as we read it so be careful.  If conditions are favorable,
 * we will try to read stuff from an old buffer as well, but if we cannot
 * get it we will not worry.
 *
 * The error reporting philosophy of this routine is not well defined.
 *
 * Watch out for fencepost problems in shuffle_and_write().	xxx
 */
read_backwards()
{
	int wrote_old_stuff = 0;
	int lost_words;

	read_data_structures();

	/*
	 * If there is an allocated old buffer and it is continuous 
	 * with the current current buffer (current buffer has not
	 * wrapped), then handle the old buffer.
	 */
	if ((ms_old_buf_num >= 0) && (ms_buf_cnt <= ms_buf_siz)) {
		state = OLD_BUF;
		iscan_ms_buf_num = ms_old_buf_num;
		/* Try reading a full buffers worth... */
		/* unless there is not that much to read. */
		if (ms_old_buf_cnt < ms_old_buf_siz) {
			iscan_ms_buf_cnt = 0;
		} else {
			iscan_ms_buf_cnt = ms_old_buf_cnt - ms_old_buf_siz;
		}
		do_read();
		read_data_structures();
		/* Old buffers, by definition, are not being actively
		 * written to.  Therefore we do not need to check for
		 * wrap.  We do, however have to watch out for the 
		 * buffer being deallocated out from under us.
		 */
		assert(!old_overflow());
		if (ms_old_buf_num >= 0) {
			shuffle_and_write(0, words_read -1);
			wrote_old_stuff = 1;
		} else {
			/* 
			 * Do not complain if it did not work;  We will try
			 * the current buffer in a moment.
			 */
		}
		/* the following unconditionally sets iscan_ms_buf_cnt */
		move_to_next_buffer_happily();
		while (!current_buf()) {
		    /*
		     * Only complain if we have already written data
		     * (in this case a gap before we write anything is
		     * ok, but one in the middle of data is always
		     * unacceptable)
		     */
		    if (wrote_old_stuff) {
			move_to_next_buffer();
		    } else {
			move_to_next_buffer_happily();
		    }
		}
		if (wrote_old_stuff && current_overflow()) {
		    lost_data();
		}
	} else {
	    iscan_ms_buf_num = ms_buf_num;
	}

	assert(current_buf());
	state = CURRENT_BUF;

	/* Try reading a full buffers worth... */
	/* unless there is not that much to read. */
	if (ms_buf_cnt < ms_buf_siz) {
		iscan_ms_buf_cnt = 0;
	} else {
		iscan_ms_buf_cnt = ms_buf_cnt - ms_buf_siz;
	}

	do_read();
	read_data_structures();
	/* 
	 * If the distance from where we started to here is bigger than 
	 * the buffer, then we lost some data.  
	 * Do not consider writing out the lost part.
	 */
	lost_words = (ms_buf_cnt - iscan_ms_buf_cnt) - ms_buf_siz;
	if (lost_words < 0) {
		lost_words = 0;
	}
	if (wrote_old_stuff && (lost_words > 0)) {
	    lost_data();
	}
	shuffle_and_write(lost_words, words_read -1);
	iscan_ms_buf_cnt += words_read;
}

/************************************************************************/
/* end of top level routines */
/************************************************************************/

resynch_current()
{
    dbg("starting resynch_current");
    
    lost_data();
    iscan_ms_buf_cnt = ms_buf_cnt;
    state = LOST_DATA;
}

move_to_next_buffer()
{
    dbg("starting move_to_next_buffer");
    
    lost_buffer();
    iscan_ms_buf_num++;
    iscan_ms_buf_cnt = 0;
    state = LOST_DATA;
}

move_to_next_buffer_happily()
{
    dbg("starting move_to_next_buffer_happily");
    
    iscan_ms_buf_num++;
    iscan_ms_buf_cnt = 0;
}

/*
 * Since the size of the buffer in kmem can change dynamically, the size of 
 * the buffer we use
 * to copy into may also need to change.  When necessary, this routine
 * frees our old buffer (if we had one), and grabs a new one of the
 * necessary size.
 *
 * Note that the buffer obtained is one word larger than the kernels.
 * Also, my_buf is pointed to the second word (number 1) of the buffer.
 * This allows us to safely access my_buf[-1] in shuffle_and_write().
 *
 * Changed to use realloc() so any data in the buffer isn't trashed.
 * (rarely occurs).
 */
malloc_if_needed(siz)	/* size is in ints */
    int siz;
{
    char *cp, *malloc(), *realloc();
    
    /* my_buf will point to the first (not zeroth) word we allocate */
    if (siz > iscan_ms_buf_siz) {  /* if I need more space */
	dbg("needed to malloc (%d > %d)", siz, iscan_ms_buf_siz);
	if (iscan_ms_buf_siz > 0) { /* if I have a buffer, do realloc */
	    dbg("doing realloc");
	    cp = realloc((char *) &my_buf[-1],
			 (unsigned) (sizeof(int) * (siz +1)));
	} else {
	    dbg("doing original malloc");
	    cp = malloc( (unsigned) (sizeof(int) * (siz +1)));
	}
 	if (cp == NULL) {
	    bail_out("malloc failed on an argument of %d", siz);
    	}
	my_buf = (int *) (cp + sizeof(int));
	iscan_ms_buf_siz = siz;
	dbg("buffer malloc-ed at 0x%x", my_buf);
    }
}

/*
 * Read from either the current or the old buffer in kmem into my_buf.
 * Correctly handle the possibility that the data is wrapped around the
 * end of the buffer.
 */
int
read_buffer(ptr, siz, old, cur)
    KMEM_OFFSET ptr;
    int siz;
    unsigned int old, cur;
{
    KMEM_OFFSET start;
    int total_words, total_bytes;
    int first_words, first_bytes;
    int *second_dest, second_bytes;
    int err;

    start = ptr + ((old % siz) * sizeof(int));
    total_words =  (cur - old);
    total_bytes =  sizeof(int) * total_words;
    
    if ((old/siz) == ((cur -1)/siz)) {
	/* buffer doesnt wrap around end, so read in one chunk */
	err = get_bytes(start, (char *) my_buf, total_bytes);
	dbg("read_buffer: does not wrap");
	dbg("read_buffer: got bytes(0x%x, 0x%x, %d)",
	    start, (char *) my_buf, total_bytes);
    } else {
	/* buffer wraps around end, so read in two chunks */
	first_words = siz - (old % siz);
	first_bytes = sizeof(int) * first_words;
	err = get_bytes(start, (char *) my_buf, first_bytes);
	dbg("read_buffer: does wrap");
	dbg("read_buffer: got bytes(0x%x, 0x%x, %d)",
	    start, (char *) my_buf, first_bytes);
	if (err != 0) {
	    return(err);
	}
	
	second_bytes = total_bytes - first_bytes;
	second_dest = &my_buf[first_words];
	err = get_bytes(ptr, (char *) second_dest, second_bytes);
	dbg("read_buffer: got bytes(0x%x, 0x%x, %d)",
	    ptr, (char *) second_dest, second_bytes);
    }
    return(err);
}

/*
 * This routine outputs the words in my_buf from first to last to standard out.
 * Note that in kmem, the length follows the data, while in iscans output,
 * it precedes it.  (Earlier versions of iscan wrote the length both before
 * and after - it seemed like a good idea at the time :-{  )
 */
shuffle_and_write(first, last)
     long first, last;
{
    int here, tmp, save, byte_cnt, recs;
    long cnt, old_mask;
    int i;
    FILE *panic_file;

    dbg("starting shuffle_and_write: first=%d, last=%d", first, last);
    
    /* note that I safely access my_buf[-1] because of hackery with malloc */
    save = 0;	/* really does not matter */
    my_buf[first -1] = 1; /* anything positive is ok */
    recs = 0;	/* just for information sake */
    for (here = last; here >= (first -1); here -= save) {
	tmp = my_buf[here];
	/* Since this seems to happen occasionally, replace
	 * the assert with some more meaningful diagnostics
	 */
	if (tmp <= 0) {
		warn("tmp <= 0 (corrupt record)\n");
		warn("first = %d, last = %d\n", first, last);
		warn("here = %d, tmp = %d\n", here, tmp);
		warn("save = %d, recs = %d\n", save, recs);
		warn("iscan_ms_buf_siz = %d\n",iscan_ms_buf_siz);
		warn("about to try dumping my_buf to /tmp/iscan.my_buf\n");
		if ((panic_file = fopen("/tmp/iscan.my_buf", "w")) == NULL) {
			perror_bail_out("Unable to open /tmp/iscan.my_buf");
		}
		fprintf(panic_file, "tmp <= 0 (corrupt record)\n");
		fprintf(panic_file, "first = %d, last = %d\n", first, last);
		fprintf(panic_file, "here = %d, tmp = %d\n", here, tmp);
		fprintf(panic_file, "save = %d, recs = %d\n", save, recs);
		fprintf(panic_file, "iscan_ms_buf_siz = %d\n",iscan_ms_buf_siz);
		fprintf(panic_file, "my_buf (from -1 to iscan_ms_buf_siz):\n");
		for (i = -1; i < iscan_ms_buf_siz; i++) {
			fprintf(panic_file, "my_buf[%7d] = %12d, (0x%8x)\n", 
				i, my_buf[i], my_buf[i]);
		}
		fprintf(panic_file, "end of my_buf\n");
		if (fclose(panic_file) != 0) {
			perror_bail_out("Unable to close /tmp/iscan.my_buf\n");
		}
		bail_out("finished dumping my_buf please forward to hpda!dah\n");
	}
	my_buf[here] = save;
	save = tmp;
	recs++;
    }
    recs--;
    /* it is ok to modify local copies (call by value) */
    first = here + save;	/* where VALID data starts */
    last--;			/* no longer using trailing len */
    cnt = 1 + last - first;	/* first to last inclusive */
    
    dbg("shuffle_and_write: writing %d records (%d words)", recs, cnt);
    /* I cannot think of a meaningful assertion for here */

    byte_cnt = cnt * sizeof(int);
    if (experiment) {
        /* block our signal handlers during the write */
        old_mask = sigblock(((1L << (SIGINT-1)) | (1L << (SIGTERM-1))));
    }
    while (1) {
	if (byte_cnt > max_write_size) {
	    dbg("s_a_w1: writing %6d bytes\n", max_write_size);
	    if (write(STDOUT, (char *) &my_buf[first], max_write_size) !=
		max_write_size) {
		perror_bail_out("write");
	    }    
	    first += (max_write_size/sizeof(int)); /* my_buf is in words */
	    byte_cnt -= max_write_size;	/* byte_cnt is in bytes */
	} else {
	    dbg("s_a_w2: writing %6d bytes\n", byte_cnt);
	    if (write(STDOUT, (char *) &my_buf[first], byte_cnt) != byte_cnt) {
		perror_bail_out("write");
	    }    
	    break;
	}
    }
    dbg("s_a_w3 done\n");
    if (experiment) {
        (void) sigsetmask(old_mask);
    }
}

handle_runstring(argc, argv)
    int argc;
    char **argv;
{
    int c;
    extern char *optarg;
    extern int optind;

    while ((c = getopt(argc, argv, "dnqs:i:weoaf:mW:")) != EOF) {
    	switch (c) {
	  case 's':
	    nlist_filename = optarg;
	    break;
	  case 'i':
	    naptime_str = optarg;
	    if (sscanf(naptime_str, "%f", &naptime) != 1) {
		bail_out("interval didn't parse");
	    }
	    break;
	  case 'd':
	    debug++;
	    break;
	  case 'n':
	    noting++;
	    break;
	  case 'q':
	    chatty = 0;
	    break;
	  case 'w':
	    wizard = 1; 	/* for measurement system wizards only */
	    break;		
	  case 'e':
	    experiment = 1;
	    break;
	  case 'o':
	    grab_old_only = 1;
	    break;
	  case 'a':
	    grab_all = 1;
	    break;
	  case 'W':
	    max_write_size = atoi(optarg);
	    break;
	  case 'f':
	    kmemf = optarg;
	    kmemf_flg = 1;
	    break;
	  case 'm':
	    if (plock(PROCLOCK) != 0) {	/* will expand at later malloc()s */
		perror_bail_out("plock");
	    }
	    break;
	  case '?':
	    bail_out("Usage: iscan -dnqeoam -W max_wrt_siz -s system -i interval -f file");
	    break;
	  default:
	    bail_out("getopt returned %c", c);
	}
    }
    dbg("arguments parsed");
    dbg("optarg = 0x%x\n", optarg);
    dbg("optind = %d\n", optind);
    dbg("argc = %d, argv = 0x%x\n", argc, argv);

    /* empirical coding :-( */
    if (argc > optind) {
	bail_out("extra arg: %s", argv[optind]);
    }

    if (grab_old_only && grab_all) {
	bail_out("may not specify both -a and -o");
    }

    if (grab_old_only && experiment) {
	bail_out("may not specify both -o and -e");
    }
}

/*
 * We have lost data (like the old buffer overrun error).  Post all the
 * relevant info.  This happens when the buffer iscan is looking at
 * more than completely fills itself between the times iscan sees it.
 */
lost_data()
{
    int i;
    int buf1[20];

    dbg("starting lost_data");
    
    fprintf(stderr, "iscan: lost data. my buf#=%d.  knls#= %d. old knls#=%d\n",
    	iscan_ms_buf_num, ms_buf_num, ms_old_buf_num);
    fprintf(stderr, "iscan: ---- ----. my cnt=%u.  knls=%u.  old knls=%u\n",
    	iscan_ms_buf_cnt, ms_buf_cnt, ms_old_buf_cnt);

    i = 0;
    buf1[i++] = iscan_ms_buf_num;		
    buf1[i++] = ms_buf_num;			
    buf1[i++] = ms_old_buf_num;		
    buf1[i++] = iscan_ms_buf_cnt;		
    buf1[i++] = ms_buf_cnt;			
    buf1[i++] = ms_old_buf_cnt;		
    
    fake_record(-1, MS_LOST_DATA, buf1, i);
}

/*
 * We have lost a buffer, so create an error record of the relevant info.
 * Losing a buffer happens when the buffer is changed on us, and we aren't
 * able to catch up.
 */
lost_buffer()
{
    int i;
    int buf1[20];

    dbg("starting lost_buffer");
    
    chat("iscan: lost buffer. my buf#=%d. knls#= %d. old knls#=%d\n",
    	iscan_ms_buf_num, ms_buf_num, ms_old_buf_num);
    chat("iscan: ---- ------. my cnt=%u.  knls=%u.  old knls=%u\n",
    	iscan_ms_buf_cnt, ms_buf_cnt, ms_old_buf_cnt);
    
    i = 0;
    buf1[i++] = iscan_ms_buf_num;		
    buf1[i++] = ms_buf_num;			
    buf1[i++] = ms_old_buf_num;		
    buf1[i++] = iscan_ms_buf_cnt;
    buf1[i++] = ms_buf_cnt;			
    buf1[i++] = ms_old_buf_cnt;		
    
    fake_record(-1, MS_LOST_BUFFER, buf1, i);
}

/*
 * routines to abstractly manipulate pointers and cnts etc.
 */
BOOL
current_empty()
{
    return(ms_buf_cnt == iscan_ms_buf_cnt);
}

int
old_empty()
{
    return(ms_old_buf_cnt == iscan_ms_buf_cnt);
}

int
current_buf()
{
    return(ms_buf_num == iscan_ms_buf_num);
}

int
old_buf()
{
    return(ms_old_buf_num == iscan_ms_buf_num);
}

int
current_overflow()
{
    return(ms_buf_cnt - iscan_ms_buf_cnt > ms_buf_siz);
}

int
old_overflow()
{
    return(ms_old_buf_cnt - iscan_ms_buf_cnt > ms_old_buf_siz);
}

long
words_in_old_buf()
{
    return(ms_old_buf_cnt - iscan_ms_buf_cnt);
}

long
words_in_current_buf()
{
    return(ms_buf_cnt - iscan_ms_buf_cnt);
}

/* if terminated, clean up and leave */
int 
handle_term()
{
    if (experiment) {
	if (!killed) {
	    killed = 1;
	}
	chat("SIGTERM noted.  Reading remaining data from kmem...");
    } else {
	die("exiting due to SIGTERM\n");
    }
}

/* if interrupted, clean up and leave */
int 
handle_int()
{
    if (experiment) {
	if (!killed) {
	    killed = 1;
	}
	chat("SIGINT noted.  Reading remaining data from kmem...");
    } else {
	die("exiting due to SIGINT\n");
    }
}

grab_old_id_strings()
{
    int  i, len_in_words;
    char name[1004];

    dbg("starting grab_old_id_strings");
    
    /*  do both ms_id and ms_bin_id */
    ms_id = get_int_3("ms_id");
    if (ms_id > MS_MAX_NORMAL_ID +1) {
	bail_out("grab_old_id_strings: bad ms_id: %d", ms_id);
    }
    for (i = MS_MIN_NORMAL_ID; i < ms_id; i++) {
	grab_name(i, name);
	/* take length of string (plus one for nul) and round up to word */
	len_in_words = (strlen(name) +1 + sizeof(int) -1) / sizeof(int);
	fake_record(i, MS_GRAB_ID, (int *) name, len_in_words);
    }
    ms_bin_id = get_int_3("ms_bin_id");
    if (ms_bin_id > MS_MAX_BIN_ID +1) {
	bail_out("grab_old_id_strings: bad ms_bin_id: %d", ms_bin_id);
    }
    for (i = MS_MIN_BIN_ID; i < ms_bin_id; i++) {
	grab_name(i, name);
	/* take length of string (plus one for nul) and round up to word */
	len_in_words = (strlen(name) +1 + sizeof(int) -1) / sizeof(int);
	fake_record(i, MS_GRAB_BIN_ID, (int *) name, len_in_words);
    }
}

fake_record(id, type, data, len)
    int id, type, *data, len;
{
    int cnt, i;

    dbg("starting fake_record");
    
    malloc_if_needed(len + 3);

    cnt = 0;
    my_buf[cnt++] = id;
    my_buf[cnt++] = type;
    for (i = 0; i < len; i++) {
	my_buf[cnt++] = data[i];
    }
    my_buf[cnt++] = len + 3;
    shuffle_and_write((long) 0, (long) (len + 3 -1));
}

