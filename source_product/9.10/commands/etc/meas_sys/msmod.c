/* $Source: /misc/source_product/9.10/commands.rcs/etc/meas_sys/msmod.c,v $
 * $Revision: 70.2 $		$Author: ssa $
 * $State: Exp $		$Locker:  $
 * $Date: 92/05/12 15:13:21 $
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

/* $Source: /misc/source_product/9.10/commands.rcs/etc/meas_sys/msmod.c,v $
 * $Revision: 70.2 $		$Author: ssa $
 * $State: Exp $		$Locker:  $
 * $Date: 92/05/12 15:13:21 $
 *
 * $Log:	msmod.c,v $
 * Revision 70.2  92/05/12  15:13:21  15:13:21  ssa
 * Author: alexl@hpucsb2.cup.hp.com
 * Latest source from Dave Holt to fix UCSqm00807.
 * 
 * Revision 4.35  91/10/21  10:22:53  10:22:53  dah (Dave Holt)
 * above fixes 00343DSDa2 aka DSDa200385
 * 
 * Revision 70.1  91/11/06  17:40:40  17:40:40  ssa
 * Author: sma@hpisoe4.cup.hp.com
 * Minor change. Changed "msmod -lw" to report counts using "%u" rather
 * than "%d". With "%d", negative values were being reported.
 * DTS# DSDDa200385
 * 
 * Revision 66.1  90/09/21  17:12:11  17:12:11  egeland
 * Very minor changes to get rid of compiler warnings about missing
 * type specifier.
 * 
 * Revision 64.1  89/04/28  17:00:02  17:00:02  marcs
 * underscore fix for 300. remove -s option to select nlist file. this version
 * will(?) use nlist if meas_drivr is not present.
 * 
 * Revision 4.34  91/10/21  10:22:39  10:22:39  dah (Dave Holt)
 * counts fixed to be reported as %u
 * 
 * Revision 4.33  90/05/29  11:01:29  11:01:29  dah (Dave Holt)
 * add copyright notice
 * 
 * Revision 4.32  89/07/25  10:27:21  10:27:21  dah (Dave Holt)
 * support "p" key to move back to previous instrumentation point
 * 
 * Revision 4.31.1.2  89/07/25  10:08:33  10:08:33  dah (Dave Holt)
 * make sure you can't back up id's before the beginning
 * 
 * Revision 4.31.1.1  89/07/20  15:57:53  15:57:53  dah (Dave Holt)
 * this is a branch to try out the ability to back up in interactive mode
 * 
 * Revision 4.31  89/03/27  15:33:43  15:33:43  dah (Dave Holt)
 * always define UNDERSCORE, unless you're on a s800
 * 
 * Revision 4.30  88/04/13  10:19:12  10:19:12  dah (Dave Holt)
 * pull 4.29.1.2 back into the trunk
 * 
 * Revision 4.29.1.2  88/03/15  15:11:14  15:11:14  dah (Dave Holt)
 * set default nlist_filename correctly
 * (so msmod -l could work even without /dev/meas_drivr)
 * 
 * Revision 4.29.1.1  87/03/04  10:15:29  10:15:29  dah (Dave Holt)
 * pull -s option, since it no longer does anythin
 * 
 * Revision 4.29  87/02/24  10:46:43  10:46:43  dah (Dave Holt)
 * support common.c 's  use of MS_NLIST ioctl
 * 
 * Revision 4.28.1.2  87/02/23  15:29:58  15:29:58  dah (Dave Holt)
 * move open_meas_drivr() to common.c
 * 
 * Revision 4.28.1.1  87/02/23  15:09:28  15:09:28  dah (Dave Holt)
 * don't look up ms_id_flags, ms_bin_id_flags, or ms_flags_rev since
 * they are no longer used
 * 
 * Revision 4.28  87/02/20  12:37:12  12:37:12  dah (Dave Holt)
 * Use new ioctl's to read + manipulate flag arrays
 * 
 * Revision 4.27.1.8  87/02/19  13:49:47  13:49:47  dah (Dave Holt)
 * move grab_status() to be with ms_turn_{on,off} .
 * add open_meas_drivr() to avoid opening the special file too many times
 * (previously had been done for each ms_turned_on() call (oops!)
 * 
 * Revision 4.27.1.7  87/02/19  13:27:34  13:27:34  dah (Dave Holt)
 * make arrays for holding names longer?
 * try handling problem where wizards were told -1 buffer had valid data
 * 
 * Revision 4.27.1.6  87/02/18  13:41:54  13:41:54  dah (Dave Holt)
 * pull unused function set_bytes()
 * 
 * Revision 4.27.1.5  87/02/18  13:38:25  13:38:25  dah (Dave Holt)
 * pulled unused local variable definitions
 * pulled unused funtions: check_flag_rev(), get_flag_addr(), set_int()
 * 
 * Revision 4.27.1.4  87/02/18  13:33:56  13:33:56  dah (Dave Holt)
 * pull unused declarations of local variables.
 * pull unused routines poke_id() and peek_id()
 * 
 * Revision 4.27.1.3  87/02/18  10:17:38  10:17:38  dah (Dave Holt)
 * use DEVMEAS_DRIVR (defined in sys/meas_drivr.c) instead of DEVKT
 * 
 * Revision 4.27.1.2  87/02/18  09:56:56  09:56:56  dah (Dave Holt)
 * handle true-false return from MS_TURNED_ON ioctl
 * 
 * Revision 4.27.1.1  87/02/18  09:25:06  09:25:06  dah (Dave Holt)
 * use new ioctl's for ms_turn_on, _off, turned_on
 * 
 * Revision 4.27  86/07/14  14:44:08  14:44:08  dah (Dave Holt)
 * merge 4.26.1 .  meas_drivr instead of ktest.
 * 
 * Revision 4.26.1.1  86/06/25  10:20:09  10:20:09  dah (Dave Holt)
 * branch to try using meas_drivr instead of ktest
 * 
 * Revision 4.26  86/06/23  10:25:37  10:25:37  dah (Dave Holt)
 * declare kmemf_flg, so common.c can use it (for iscan).
 * 
 * Revision 4.25  86/05/29  14:07:50  14:07:50  dah (Dave Holt)
 * make cc (aka lint) happier
 * 
 * Revision 4.24  86/05/29  13:58:06  13:58:06  dah (Dave Holt)
 * pull check for su.  It's more properly and correctly done in 
 * the kernel and in the protections for /dev/kmem
 * 
 * Revision 4.23  86/05/12  17:38:57  dah (Dave Holt)
 * some of the 'error' returns from -kK options are really chatty.
 * Treat them as such.
 * 
 * Revision 4.22  86/05/12  17:17:23  dah (Dave Holt)
 * change UNDERSCORES to UNDERSCORE
 * 
 * Revision 4.21  86/05/12  16:10:18  dah (Dave Holt)
 * Handle underscore roll.
 * Distinguish normal and bin id's.
 * Make -q effective throughout (except when info was specifically
 * requested).
 * 
 * Revision 4.20  86/02/04  11:23:40  dah (Dave Holt)
 * add some void's to make cc happier
 * 
 * Revision 4.19  86/02/04  10:26:31  dah (Dave Holt)
 * pull \n's from bail_out calls
 * 
 * Revision 4.18  86/02/03  17:47:23  dah (Dave Holt)
 * fix DSDtg00112
 * (var_addr() initializes correctly, and do_nlist() no longer
 * called elsewhere).
 * 
 * Revision 4.17.1.1  86/02/03  13:48:17  dah (Dave Holt)
 * try fixing DSDtg00112
 * 
 * Revision 4.17  86/01/29  16:38:29  dah (Dave Holt)
 * seems to work (at least normal stuff doesn't break)
 * 
 * Revision 4.16.1.1  86/01/28  15:16:26  dah (Dave Holt)
 * Attempt to resolve
 * DSDtg00103 (ioctl(,KTEST_MS_NEWBUF,) should handle -9 return
 * DSDtg00102 msmod: unknown errno messages should report dummy, not errno
 * also handle dummy returns from meas meas sys
 * 
 * Revision 4.16  86/01/27  17:41:33  dah (Dave Holt)
 * reject extra args (DSDtg00042)
 * 
 * Revision 4.15  86/01/06  15:11:43  dah (Dave Holt)
 * appears to work correctly
 * 
 * Revision 4.14.1.2  85/12/18  11:26:48  dah (Dave Holt)
 * Try to handle ioctl error returns appropriately for the
 * way ktest.c is doing them.
 * 
 * Revision 4.14.1.1  85/12/18  09:58:36  dah (Dave Holt)
 * make it more verbose about ioctl returns if debug is set.
 * 
 * Revision 4.14  85/12/18  09:55:50  dah (Dave Holt)
 * merge of 4.13.1.1 after testing
 * 
 * Revision 4.13.1.1  85/12/12  16:35:29  dah (Dave Holt)
 * try printing out more info for wizards when they use -l (ie. -lw)
 * 
 * Revision 4.13  85/12/12  13:48:56  dah (Dave Holt)
 * works
 * 
 * Revision 4.12.1.1  85/12/12  13:38:38  dah (Dave Holt)
 * open file for read iff just_print, else read + write
 * 
 * Revision 4.12  85/12/12  11:14:00  dah (Dave Holt)
 * usage now mentions -q option
 * 
 * Revision 4.11  85/12/12  11:11:44  dah (Dave Holt)
 * Appears to work ok.
 * Now correctly disallows multiple actions on runstring.
 * Also mentions -s option in usage message.
 * 
 * Revision 4.10.1.1  85/12/12  10:55:21  dah (Dave Holt)
 * check for multiple action items on runstring (disallow)
 * 
 * Revision 4.10  85/12/12  10:31:04  dah (Dave Holt)
 * This version works.
 * I'm about to try modifying code when I'm sick, so here is where to
 * roll back to.
 * 
 * Revision 4.9  85/12/11  16:13:08  dah (Dave Holt)
 * tell lint that return value is deterministic
 * 
 * Revision 4.8  85/12/10  15:10:40  dah (Dave Holt)
 * this version works successfully with common.c
 * 
 * Revision 4.7.1.4  85/12/10  14:54:33  dah (Dave Holt)
 * add missing quotes
 * 
 * Revision 4.7.1.3  85/12/10  14:48:02  dah (Dave Holt)
 * pull all the X_FOO junk from msmod as well
 * 
 * Revision 4.7.1.2  85/12/10  14:35:54  dah (Dave Holt)
 * export my_rev for common.c to use
 * 
 * Revision 4.7.1.1  85/12/09  16:01:40  dah (Dave Holt)
 * this version expects common routines to be in common.c
 * 
 * Revision 4.7  85/12/09  15:39:03  dah (Dave Holt)
 * this version appears to work
 * 
 * Revision 4.6.1.3  85/12/09  14:05:09  dah (Dave Holt)
 * add progname
 * 
 * Revision 4.6.1.2  85/12/09  13:59:59  dah (Dave Holt)
 * pull routines that are now in common
 * 
 * Revision 4.6.1.1  85/12/06  17:18:09  dah (Dave Holt)
 * use note.h
 * 
 * Revision 4.6  85/12/03  16:40:07  dah (Dave Holt)
 * added standard firstci header
 * 
 * Revision 4.5  85/11/21  10:23:21  dah (Dave Holt)
 * changed ifdef BELL to ifdef hpux
 * 
 * Revision 4.4  85/11/21  10:22:11  dah (Dave Holt)
 * *** empty log message ***
 * 
 * Revision 4.4  85/09/12  14:34:23  dah (Dave Holt)
 * incorporate changes based on rel8 testing.
 * 
 * Revision 4.2.1.5  85/09/10  15:57:07  dah (Dave Holt)
 * msmod's version of get_bytes doesn't return a value.
 * print id string *after* checking for runstring options
 * 
 * Revision 4.2.1.2  85/09/10  15:05:40  dah (Dave Holt)
 * add chat
 * 
 * Revision 4.2  85/09/10  13:41:06  dah (Dave Holt)
 * This version has the undocumented t option to allow timing the
 * measurement system calls (in ktest.c)
 * 
 * Revision 4.1.1.1  85/09/04  15:19:32  dah (Dave Holt)
 * branch to test KTEST_MS_MEAS
 * 
 * Revision 4.1  85/08/21  15:10:51  dah (Dave Holt)
 * use <sys/meas_sys.h> instead of <h/meas_sys.h>
 * 
 * Revision 4.0  85/08/20  15:43:26  dah (Dave Holt)
 * *** empty log message ***
 * 
 * Revision 1.7  85/08/17  16:32:59  dah (Dave Holt)
 * *** empty log message ***
 * 
 * Revision 1.6.1.13  85/08/17  14:36:58  dah (Dave Holt)
 * Try to correctly interpret errors from ioctl
 * 
 * Revision 1.6.1.12  85/08/17  14:04:03  dah (Dave Holt)
 * print out errno on ioctl failure
 * 
 * Revision 1.6.1.11  85/08/15  15:04:28  dah (Dave Holt)
 * speaks protocol rev 4.
 * runstring indicates that -b needs an argument.
 * msmod -l now prints out status of buffers.
 * 
 * Revision 1.6.1.9  85/08/13  17:26:04  dah (Dave Holt)
 * now checks for being super-user before allowing memory to be
 * modified, or buffers to be allocated or nuked.
 * 
 * Revision 1.6.1.8  85/08/13  14:11:57  dah (Dave Holt)
 * *** empty log message ***
 * 
 * Revision 1.6.1.5  85/08/09  11:18:22  dah (Dave Holt)
 * slightly better error messages on ioctl errors.
 * newline before done
 * 
 * Revision 1.6.1.4  85/08/08  14:49:16  dah (Dave Holt)
 * *** empty log message ***
 * 
 * Revision 1.6.1.1  85/08/08  09:37:21  dah (Dave Holt)
 * branch to control movable buffers
 * 
 * Revision 1.6  85/07/18  13:48:32  dah (Dave Holt)
 * -l should now work by itself.
 * only print "already" message if the user tried to change things.
 * make output more regular.
 * 
 * Revision 1.4  85/07/17  17:40:27  dah (Dave Holt)
 * told getopt that l is now a legal option
 * 
 * Revision 1.3  85/07/17  15:51:36  dah (Dave Holt)
 * added l option
 * better debugging and normal output
 * 
 * Revision 1.1  85/07/17  10:55:10  dah (Dave Holt)
 * Initial revision
 * 
 * Revision 1.24  85/07/16  16:40:33  dah (Dave Holt)
 * this version works.  next one is experimental
 * 
 * Revision 1.23  85/07/15  16:56:49  dah (Dave Holt)
 * Prettied up the output format.
 * 
 * Revision 1.20  85/07/15  15:31:44  dah (Dave Holt)
 * enabled set_bytes()
 * changed format of debug in set/get_int to be hex
 * tried handling bare cr's as input
 * went whole-hog over to BELL on include files, etc.
 * 
 * Revision 1.14  85/07/12  10:21:58  dah (Dave Holt)
 * fix peek and poke_id to check ms_flags_rev (not it's address as
 * previously!)
 * 
 * Revision 1.13  85/07/11  10:44:53  dah (Dave Holt)
 * added a bit more debugging
 * enabled set_int()
 * 
 * Revision 1.12  85/07/10  17:02:59  dah (Dave Holt)
 * *** empty log message ***
 * 
 * Revision 1.5  85/07/01  18:16:10  dah (Dave Holt)
 * cleaned up some lint errors
 * still very rough
 * 
 * Revision 1.2  85/07/01  15:02:34  dah (Dave Holt)
 * a start
 * 
 */

#include <sys/types.h>
#include <stdio.h>
#include <nlist.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/meas_drivr.h>

#ifdef hpux
#include <string.h>
#include <fcntl.h>
#else
#include <strings.h>
#include <sys/file.h>
#endif

#include <sys/meas_sys.h>

/*
 * protocol rev for reading the kernel 
 * This describes the organization of the kernel data that
 * the code of this program expects.
 */
#define MY_REV		4
int my_rev = MY_REV;		/* for common.c to use */

int    ms_rev;

#ifdef hp9000s800
#undef UNDERSCORE
#else
#define UNDERSCORE 1
#endif

#ifdef UNDERSCORE
struct nlist nlst[] = {
	{ "_ms_id" },
    	{ "_ms_bin_id" },
    	{ "_ms_rev" },
    	{ "_ms_include_rev" },
    	{ "_ms_id_str_ptrs" },
    	{ "_ms_bin_id_str_ptrs" },
    	{ "_ms_buf_siz" },
    	{ "_ms_old_buf_siz" },
    	{ "_ms_buf_num" },
    	{ "_ms_old_buf_num" },
    	{ "_ms_buf_cnt" },
	{ "_ms_buf_ptr" },
	{ "_ms_old_buf_ptr" },
	{ "_ms_old_buf_cnt" },
    	{ 0 },
};
#else !UNDERSCORE
struct nlist nlst[] = {
	{ "ms_id" },
    	{ "ms_bin_id" },
    	{ "ms_rev" },
    	{ "ms_include_rev" },
    	{ "ms_id_str_ptrs" },
    	{ "ms_bin_id_str_ptrs" },
    	{ "ms_buf_siz" },
    	{ "ms_old_buf_siz" },
    	{ "ms_buf_num" },
    	{ "ms_old_buf_num" },
    	{ "ms_buf_cnt" },
	{ "ms_buf_ptr" },
	{ "ms_old_buf_ptr" },
	{ "ms_old_buf_cnt" },
    	{ 0 },
};
#endif UNDERSCORE

char	*nlist_filename = "/hp-ux";

#include "note.h"
int debug = 0;
int wizard = 0;
int all_on = 0;
int all_off = 0;
char *on_names[1000];
char *off_names[1000];
int on_name_cnt = 0;
int off_name_cnt = 0;
int runstring_commands = 0;
int just_print = 0;
int some_changed = 0;
int soft_nuke = 0;
int testing = 0;
int test_count = 0;

int kmem;
char    *kmemf = "/dev/kmem";
int kill_old_buffer = 0;
int hard_nuke = 0;
int new_buffer = 0;
int new_buffer_siz;
int chatty = 1;

int ms_id, ms_bin_id;

typedef unsigned long KMEM_OFFSET;

/* these belong in an include file shared with meas_sys.c */
#define UNALLOCATED 0
#define ON  	    1
#define OFF 	    2

extern int errno;

int kmemf_flg = 0;	/* used in common.c */
char *progname = "msmod";
void do_it_interactively(), do_it_from_runstring();

main(argc,argv)
    int argc;
    char **argv;
{
    int i;

    setup (argc, argv);
    dbg("finished setup()");

    chat("$Header: msmod.c,v 70.2 92/05/12 15:13:21 ssa Exp $\n");

    if (runstring_commands) {
	if (kill_old_buffer) {
	    nuke_it(hard_nuke);
	    clean_up(0);
	}
	
	if (new_buffer) {
	    grab_new_buffer();
	    clean_up(0);
	}
	
	if (just_print) {
	    print_buf_info();
	    printf("Normal id's\n");
	}
	
    	dbg("ms_id = %d", ms_id);
       	for (i = MS_MIN_NORMAL_ID; i < ms_id; i++) {   /* for each normal id */
       	    do_it_from_runstring(i);
	}
       	dbg("normal id's done");

	if (just_print) {
	    printf("Bin id's\n");
	}

       	dbg("ms_bin_id = %d", ms_bin_id);
       	for (i = MS_MIN_BIN_ID; i < ms_bin_id; i++) {    /* for each bin id */
	    do_it_from_runstring(i);
       	}
       	dbg("bin id's done");
    } else {
    	printf("\nUser interface is likely to change.\n");
    	printf("This version is preliminary.\n");
    	printf("Comments and suggestions are encouraged.\n\n");

    	instructions();
    	dbg("finished instructions");

    	dbg("ms_id = %d", ms_id);
    	for (i = MS_MIN_NORMAL_ID; i < ms_id; i++) {   /* for each normal id */
	    	do_it_interactively(&i);
    	}
    	dbg("normal id's done");

    	dbg("ms_bin_id = %d", ms_bin_id);
    	for (i = MS_MIN_BIN_ID; i < ms_bin_id; i++) {    /* for each bin id */
	    	do_it_interactively(&i);
    	}
    	dbg("bin id's done");
    }

    clean_up(0);	/* done */
    /*NOTREACHED*/
}

/* initialize */
setup(argc,argv)
int argc;
char **argv;
{
    char *malloc();
    int oflag;

    handle_runstring(argc, argv);
    dbg("runstring handled");
    
    if (testing) {
	test_it(test_count);
	clean_up(0);
    }

    /*  open kernel memory for reading or read/write as appropriate */
    if (just_print) {
	oflag = O_RDONLY;
    } else {
	oflag = O_RDWR;
    }
    if ((kmem = open (kmemf, oflag)) < 0) {
    	perror_bail_out(kmemf);
    }
    dbg("opened kernel (%s) successfully", kmemf);
    
    /* do_nlist() now done in var_addr() */

    check_ms_rev();
    dbg("kernel rev ok");

    check_include_rev();
    dbg("include file rev ok");
    
    ms_id = get_int_3("ms_id");
    ms_bin_id = get_int_3("ms_bin_id");
}

handle_runstring(argc, argv)
    int argc;
    char **argv;
{
    int c;
    extern char *optarg;
    extern int optind;

    while ((c = getopt(argc, argv, "wkKb:dnqp:m:lPMt:")) != EOF) {
    	switch (c) {
	    case 'w':
	    	wizard = 1; 	/* for measurement system wizards only */
		break;
	    case 'k':
		kill_old_buffer++;
		soft_nuke++;
	    	break;
	    case 'K':
		kill_old_buffer++;
		hard_nuke++;
		break;
	    case 'b':
		new_buffer++;
		if (sscanf(optarg, "%d", &new_buffer_siz) != 1) {
		    bail_out("bad param to -b: %s", optarg);
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
	    case 'p':
		some_changed = 1;
	    	on_names[on_name_cnt++] = optarg;
		dbg("new on name: %s", optarg);
		break;
	    case 'm':
		some_changed = 1;
	    	off_names[off_name_cnt++] = optarg;
		dbg("new off name: %s", optarg);
		break;
	    case 'l':
		just_print++;
	    	dbg("just print");
		break;
	    case 'P':
		all_on++;
	    	dbg("all on");
		break;
	    case 'M':
		all_off++;
	    	dbg("all off");
		break;
	    case 't':
		testing++;
		if (sscanf(optarg,"%d",&test_count) != 1)
		    bail_out("bad arg to t: %s",optarg);
		break;
	    case '?':
	    	bail_out("Usage: msmod -kKdnlq -p \"id string\" -m \"id string\" -b buf_siz");
    	    	break;
	    default:
	    	bail_out("getopt returned %c", c);
	}
    }
    dbg("arguments parsed");

    runstring_commands = some_changed + soft_nuke + hard_nuke +
      just_print + new_buffer + all_on + all_off + testing;

    if (runstring_commands > 1)
      bail_out("Only one of p/m, k, K, l, or b allowed");

    if (argc > optind) {
	bail_out("extra arg: %s", argv[optind]);
    }
}

void
do_it_from_runstring(id)
    int id;
{
    char name[1000];	/* excessively large */
    int old_status, desired;
    int in_on_names, in_off_names;
    char *stat_name, *status_name();

    grab_name(id, name);    /* name is set by this call */

    stat_name = status_name(id);
    dbg("runstr: %s %s", stat_name, name);

    if (just_print) {
	if (wizard) {
	    printf("id %d, %s  %s\n", id, stat_name, name);
	} else {
	    printf("%s  %s\n", stat_name, name);
	}
	return;
    }
    
    old_status = grab_status(id);
    desired = old_status;   /* initial value */

    /* handle -P and -M first */
    if (all_on)
    	desired = ON;
    if (all_off)
    	desired = OFF;
    
    in_on_names = str_match(name, on_names, on_name_cnt);
    dbg("in_on_names = %d", in_on_names);
    in_off_names = str_match(name, off_names, off_name_cnt);
    dbg("in_off_names = %d", in_off_names);
    if (in_on_names) {
	if (in_off_names) {
	    bail_out("error: %s turned on *and* off", name);
	} else {
	    desired = ON;
	}
    } else {
	if (in_off_names) {
	    desired = OFF;
	} else {
	    /* leave alone */
	}
    }

    if (desired != old_status) {    /* only if it needs changing */
	if (desired == ON) {
	    turn_on(id);
	} else {
	    turn_off(id);
	}
	stat_name = status_name(id);
	chat("\"%s\" turned %s.\n", name, stat_name);
    } else {
	if (in_on_names || in_off_names || all_on || all_off) {
	    chat("\"%s\" already %s.\n", name, stat_name);
	}
    }
}

int str_match(name, name_array, cnt)
    char *name;
    char *name_array[];
    int cnt;
{
    int i;

    for (i = 0; i < cnt; i++) {
	if (strcmp(name, name_array[i]) == 0)
	    return(1);
    }
    return(0);
}

void
do_it_interactively(idp)
    int *idp;
{
    char name[1000];	/* excessively large */
    char *stat_name, *status_name();
    char c, prompt_for_char();
    
    grab_name(*idp, name);    /* name is set by this call */
    
    stat_name = status_name(*idp);
    
    while (1) {
	c = prompt_for_char("%s	%s ? ", stat_name, name);
       	switch (c) {
	  case '+':
	    if (sure())
	    turn_on(*idp);
	    return;
	  case '-':
	    if (sure())
	    turn_off(*idp);
	    return;
	  case '\n':
	  case ' ':
	  case '\0':		/* gets returns empty str on bare cr */
	    return;
	    break;
	  case 'p':
	    if (*idp == MS_MIN_NORMAL_ID) {
		    /* don't backup before first */
		    *idp -= 1;	/* main loop will undo this */
		    return;
	    }
	    if (*idp == MS_MIN_BIN_ID) {
		    /* don't backup before first */
		    *idp -= 1;	/* main loop will undo this */
		    return;
	    }
	    *idp -= 2;		/* back up to hit previous id */
	    return;
	  case 'q':
	    printf("quitting\n");
	    clean_up(0);
	  default:
	    printf("unknown option: %c\n", c);
	    instructions();
	}
    }
}


char *
status_name(id)
    int id;
{
    char *stat_name;
    int status;

    status = grab_status(id);
    
    switch (status) {
	case UNALLOCATED:
	    bail_out("bad status read from kernel. id %d, status %d", 
	    	id, status);
	    break;
	case ON:
	    stat_name = "<on>";
	    break;
	case OFF:
	    stat_name = "<off>";
	    break;
	default:
	    bail_out("bad status read from kernel. id %d, status %d", 
	    	id, status);
    }
    return(stat_name);
}

int
sure()
{
    char c, prompt_for_char();
    
    c = prompt_for_char("Are you sure? ");
    switch (c) {
	case 'y':
	case 'Y':
	    return(1);
	default:
	    return(0);
    }
}
    
/************************************************************************/
/* Take advantage of new kernel ioctls */
/************************************************************************/

turn_on(id)
    int id;
{
/*    poke_id(id, ON); */
    int fd, err;
    int dummy;

    must_be_su();
    
    fd = open_meas_drivr();

    dummy = id;	/* ioctl can change its arg! */
    err = ioctl(fd, MS_TURN_ON, &dummy);
    dbg("ioctl MS_TURN_ON returned %d.  dummy is %d.  errno is %d",
	err, dummy, errno);
    if (err != 0) {		/* handle system error */
	perror_bail_out("ioctl");
    }
    if (dummy != 0) {		/* handle meas sys error */
    	bail_out("Unknown return %d from ioctl MS_TURN_ON!!!", dummy);
    }
}

turn_off(id)
    int id;
{
/*    poke_id(id, OFF); */
    int fd, err;
    int dummy;

    must_be_su();
    
    fd = open_meas_drivr();

    dummy = id;	/* ioctl can change its arg! */
    err = ioctl(fd, MS_TURN_OFF, &dummy);
    dbg("ioctl MS_TURN_OFF returned %d.  dummy is %d.  errno is %d",
	err, dummy, errno);
    if (err != 0) {		/* handle system error */
	perror_bail_out("ioctl");
    }
    if (dummy != 0) {		/* handle meas sys error */
    	bail_out("Unknown return %d from ioctl MS_TURN_OFF!!!", dummy);
    }
}

/*
 * Find out if the measurement is turned on.
 * Return ON or OFF .
 */
int 
grab_status(id)
    int id;
{
/*    return(peek_id(id)); */
    int fd, err;
    int dummy;

    must_be_su();
    
    fd = open_meas_drivr();

    dummy = id;	/* ioctl can change its arg! */
    err = ioctl(fd, MS_TURNED_ON, &dummy);
    dbg("ioctl MS_TURNED_ON returned %d.  dummy is %d.  errno is %d",
	err, dummy, errno);
    if (err != 0) {		/* handle system error */
	perror_bail_out("ioctl");
    }
    if (dummy) {
	return(ON);
    } else {
	return(OFF);
    }
}

/************************************************************************/
/* */
/************************************************************************/

/************************************************************************/
/* utility routines */
/************************************************************************/

instructions()
{
    printf("instructions:\n");
    printf(" type <space> or <return> to leave alone. \n");
    printf(" type + to enable measurement.\n");
    printf(" type - to disable measurement.\n");
    printf(" type q to quit msmod.\n");
    printf(" type p to back up to previous measurement.\n");
}
    
/*VARARGS1*/
/* 
 * Later, when there's time to do it right, this routine should just do
 * single-char reads, outputting a newline afterwards.
 */
char
prompt_for_char(fmt, a, b, c, d)
    char *fmt;
{
    char in[80];
    char *gets();

    printf(fmt, a, b, c, d);
    (void) gets(in);
    return(in[0]);
}

/*
 * Use measurements of meas sys in meas_drivr.c .
 * Don't use too often, or you'll run out of bin id's.
 */
test_it(count)
     int count;
{
    int fd, err, dummy;

    must_be_su();

    fd = open_meas_drivr();

    dbg("test_it(%d)", count);
    dummy = count;
    err = ioctl(fd, MS_MEAS, &dummy);
    dbg("ioctl MS_MEAS returned %d.  dummy is %d  errno is %d",
	err, dummy, errno);
    if (err != 0) {		/* handle system error */
	perror_bail_out("ioctl");
    }
    if (dummy != 0) {		/* handle meas sys error */
	switch (dummy) {
	  case -1:
	    bail_out("mms: ms_grab_bin_id failed (1)");
	  case -2:
	    bail_out("mms: ms_usecdiff reported negative delta");
	  case -3:
	    bail_out("mms: ms_grab_id failed");
	  case -4:
	    bail_out("mms: ms_grab_bin_id failed (2)");
	  case -5:
	    bail_out("mms: ms_bin failed");
	  default:
	    bail_out("Unknown return %d from ioctl MS_MEAS!!!", 
		dummy);
	}
    }
    clean_up(0);
}

/* 
 * hard == 1 implies remove it in all cases
 * hard == 0 means remove it only if it is no longer useful
 */
nuke_it(hard)
    int hard;
{
    int fd, err, dummy;
    int ms_old_buf_num, ms_old_buf_siz;

    must_be_su();
    
    ms_old_buf_num = get_int_3("ms_old_buf_num");
    if (ms_old_buf_num == -1) {
	/*
	 * Scripts might do this to be 'safe', so call it chatting
	 * rather than a warning.
	 */
	chat("There is no old buffer\n");
    } else {
	ms_old_buf_siz = get_int_3("ms_old_buf_siz");
	chat("Old buf #%d (size %d words) being nuked\n",
	    ms_old_buf_num, ms_old_buf_siz);
    }
    
    fd = open_meas_drivr();

    dbg("nuke_it: hard = %d", hard);
    dummy = hard;
    err = ioctl(fd, MS_FLUSHBUF, &dummy);
    dbg("ioctl MS_FLUSHBUF returned %d.  dummy is %d  errno is %d",
	err, dummy, errno);
    if (err != 0) {		/* handle system error */
	perror_bail_out("ioctl");
    }
    if (dummy != 0) {		/* handle meas sys error */
	switch (dummy) {
	    case -1:
		/* maybe that's ok */
	    	die("There is no old buffer!"); 
	    case -8:
		/* maybe that's ok */
	    	die("Old buffer still useful, not killed.");
	    case -7:
	    	bail_out("Only one K option allowed.");
		break; 	/* should not be needed */
	    default:
	    	bail_out("Unknown return %d from ioctl MS_FLUSHBUF!!!",
		    dummy);
	}
    }
    chat("old buffer successfully removed\n");    
}

grab_new_buffer()
{
    int fd, err;
    int ms_buf_num, ms_buf_siz, dummy;

    must_be_su();
    
    ms_buf_num = get_int_3("ms_buf_num");
    ms_buf_siz = get_int_3("ms_buf_siz");
    chat("current buf #%d is %d words\n", ms_buf_num, ms_buf_siz);
    chat("trying to allocate a new buffer of %d words\n", new_buffer_siz);
    
    fd = open_meas_drivr();

    dummy = new_buffer_siz;	/* ioctl can change its arg! */
    err = ioctl(fd, MS_NEWBUF, &dummy);
    dbg("ioctl MS_NEWBUF returned %d.  dummy is %d.  errno is %d",
	err, dummy, errno);
    if (err != 0) {		/* handle system error */
	perror_bail_out("ioctl");
    }
    if (dummy != 0) {		/* handle meas sys error */
	switch (dummy) {
	    case -1:
	    	bail_out("Two buffers in use.  Wait or use -K");
	    case -4:
	    	bail_out("Size must be a power of 2");
	    case -2:
	    	bail_out("Can't have more than half of available memory");
	    case -5:
	    	bail_out("Another superuser is trying to grab space.");
	    case -6:
	    	bail_out("There isn't that much lockable memory!");
	    case -9:
		bail_out("wmemall() failed.  Memory unavailable.");
	    default:
	    	bail_out("Unknown return %d from ioctl MS_NEWBUF!!!",
		    dummy);
	}
    }
    
    chat("new buffer successfully allocated\n");
    ms_buf_siz = get_int_3("ms_buf_siz");   /* be sure it worked */
    chat("new size is %d\n", ms_buf_siz);
}

must_be_su()
{
    /*
     * suser() check in meas_drivr ioctl and protections on /dev/kmem
     * should be adequate
     */
}

print_buf_info()
{
    int ms_buf_cnt, ms_buf_num, ms_old_buf_num, ms_buf_siz, ms_old_buf_siz;
    KMEM_OFFSET ms_buf_ptr, ms_old_buf_ptr;
    int ms_old_buf_cnt;
    int ms_id, ms_bin_id;
    
    ms_buf_cnt = get_int_3("ms_buf_cnt");
    ms_buf_num = get_int_3("ms_buf_num");
    ms_old_buf_num = get_int_3("ms_old_buf_num");
    ms_buf_siz = get_int_3("ms_buf_siz");
    ms_old_buf_siz = get_int_3("ms_old_buf_siz");
    
    if (wizard) {
	ms_id = get_int_3("ms_id");
	printf("ms_id = %d\n", ms_id);
	ms_bin_id = get_int_3("ms_bin_id");
	printf("ms_bin_id = %d\n", ms_bin_id);
	ms_buf_ptr = get_int_3("ms_buf_ptr");
	printf("ms_buf_ptr = 0x%x\n", ms_buf_ptr);
	ms_old_buf_ptr = get_int_3("ms_old_buf_ptr");
	printf("ms_old_buf_ptr = 0x%x\n", ms_old_buf_ptr);
	ms_buf_cnt = get_int_3("ms_buf_cnt");
	printf("ms_buf_cnt = %u\n", ms_buf_cnt);
	ms_old_buf_cnt = get_int_3("ms_old_buf_cnt");
	printf("ms_old_buf_cnt = %u\n", ms_old_buf_cnt);
    }

    printf("Current buffer (#%d) is %d words.\n",
    	ms_buf_num, ms_buf_siz);

    if (ms_old_buf_num == -1 && !wizard) {
	printf("There is no old buffer.\n");
    } else {
	printf("Old buffer (#%d) is %d words.\n",
	    ms_old_buf_num, ms_old_buf_siz);
	if (ms_buf_cnt <= ms_buf_siz && ms_old_buf_num != -1) {
	    printf("It still contains potentially useful information.\n");
	} else {
	    printf("It contains out-of-date information.\n");
	}
    }
}
