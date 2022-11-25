/* $Source: /misc/source_product/9.10/commands.rcs/etc/meas_sys/common.c,v $
 * $Revision: 66.1 $	$Author: ssa $
 * $State: Exp $   	$Locker:  $
 * $Date: 91/05/09 10:44:00 $
 *
 * $Log:	common.c,v $
 * Revision 66.1  91/05/09  10:44:00  10:44:00  ssa
 * Author: sartin@hpisqb.cup.hp.com
 * Reading core files not supported on PA89.
 * 
 * To deal with 8.02 release problems
 * 
 * Revision 1.4  91/05/07  11:33:29  11:33:29  dah (Dave Holt)
 * try ifdefing out the corefile reading code for 8.02
 * 
 * Revision 1.3  91/05/07  11:19:29  11:19:29  dah (Dave Holt)
 * back to base version
 * 
 * Revision 64.2  89/05/01  16:21:23  16:21:23  marcs
 * adjust to fact that S300 will now have a meas_drivr device driver.
 * 
 * Revision 1.42  89/05/01  15:32:01  15:32:01  dah (Dave Holt)
 * meas_drivr should now appear on s300's.  Make sure that msmod can
 * be built there.
 * 
 * Revision 1.41  89/03/30  14:15:49  14:15:49  dah (Dave Holt)
 * try handling UNDERSCORE properly on s300
 * 
 * Revision 1.40  87/03/24  15:54:55  15:54:55  dah (Dave Holt)
 * finish changes begun in 1.39 (thanks to pcn for fixes)
 * 
 * Revision 1.39  87/03/23  16:43:52  16:43:52  dah
 * Modified common.c so hp9000s200 doesn't use /dev/meas_drivr .
 * Pcn says they don't need the driver because:
 * - They don't care about the nlist speedup
 * - They always have the kernel file (/hp-ux) available on the running
 *     system (no rdb capability)
 * - Their kernel is not tracking ours - it doesn't have the process or
 *     disk instrumentation
 * - Their SE's and customers are not allowed to have kernel sources, hence
 *     they cannot add measurements of their own.
 * Sigh.
 * 
 * Revision 1.38  87/03/16  15:09:58  15:09:58  dah (Dave Holt)
 * fix nlist termination test.
 * 
 * Revision 1.37  87/03/16  14:29:12  14:29:12  dah (Dave Holt)
 * merge in s200 changes.
 * works on s840, untested on s200.
 * 
 * Revision 1.36.1.2  87/03/16  14:04:29  14:04:29  dah (Dave Holt)
 * try merging in CND's changes
 * 
 * Revision 1.36.1.1  87/03/16  14:04:06  14:04:06  dah (Dave Holt)
 * *** empty log message ***
 * 
 * Revision 1.36  87/03/16  09:38:31  09:38:31  dah (Dave Holt)
 * we don't need to be able to write /dev/kmem (or even read, for that
 * matter, but there isn't an O_IOCTLONLY)
 *
 * Revision 1.31.3.1  87/03/16  10:57:57  10:57:57  dah (Dave Holt)
 * CND version (works on s300's)
 * 
 * Revision 1.35  87/03/13  17:57:39  17:57:39  dah (Dave Holt)
 * seems to work now with 1.0 or 1.1 !
 * 
 * Revision 1.34.1.2  87/03/13  17:56:24  17:56:24  dah (Dave Holt)
 * change warning about 1.0 to a chat
 * 
 * Revision 1.34.1.1  87/03/13  17:50:00  17:50:00  dah (Dave Holt)
 * branch to try handling 1.0 or 1.1 transparently
 * 
 * Revision 1.34  87/03/13  16:57:24  16:57:24  dah (Dave Holt)
 * Only define kmemf_flg once
 * 
 * Revision 1.33  87/02/24  10:44:59  10:44:59  dah (Dave Holt)
 * use MS_NLIST ioctl instead of nlist() where appropriate
 * 
 * Revision 1.32.1.9  87/02/23  15:30:21  15:30:21  dah (Dave Holt)
 * move open_meas_drivr() here from msmod.c, so it is also usable
 * by iscan.
 * include <fcntl.h> so it works.
 * 
 * Revision 1.32.1.8  87/02/23  15:20:18  15:20:18  dah (Dave Holt)
 * pull excess debugging
 * 
 * Revision 1.32.1.7  87/02/23  10:52:52  10:52:52  dah (Dave Holt)
 * add missing paren
 * 
 * Revision 1.32.1.6  87/02/23  10:51:04  10:51:04  dah (Dave Holt)
 * dbg is failing. try printf
 * 
 * Revision 1.32.1.5  87/02/23  10:43:56  10:43:56  dah (Dave Holt)
 * add more debugging
 * 
 * Revision 1.32.1.4  87/02/23  10:33:51  10:33:51  dah (Dave Holt)
 * fix a bug + add a cast
 * 
 * Revision 1.32.1.3  87/02/20  17:33:04  17:33:04  dah (Dave Holt)
 * include sys/ioctl.h
 * 
 * Revision 1.32.1.2  87/02/20  17:30:36  17:30:36  dah (Dave Holt)
 * declare a couple of variables
 * 
 * Revision 1.32.1.1  87/02/20  17:07:23  17:07:23  dah (Dave Holt)
 * try using MS_NLIST ioctl when appropriate
 * 
 * Revision 1.32  87/02/20  12:36:09  12:36:09  dah (Dave Holt)
 * handle longer id strings
 * 
 * Revision 1.31.2.1  87/02/19  14:08:26  14:08:26  dah (Dave Holt)
 * grab_name now permits longer id strings
 * 
 * Revision 1.31  86/06/24  14:47:36  14:47:36  dah (Dave Holt)
 * fix DSDtg00317 (read from core files correctly)
 * 
 * Revision 1.30.1.10  86/06/24  13:16:32  13:16:32  dah (Dave Holt)
 * fix up lint
 * 
 * Revision 1.30.1.9  86/06/24  11:30:43  11:30:43  dah (Dave Holt)
 * try doing page reads of kmem file (if not /dev/kmem)
 * 
 * Revision 1.30.1.8  86/06/23  14:56:48  14:56:48  dah (Dave Holt)
 * allow use of assert
 * 
 * Revision 1.30.1.7  86/06/23  14:49:43  14:49:43  dah (Dave Holt)
 * add more declarations
 * 
 * Revision 1.30.1.6  86/06/23  14:32:29  14:32:29  dah (Dave Holt)
 * add necessary include files.
 * declare kmemf_flag as external.
 * fix typo
 * 
 * Revision 1.30.1.5  86/06/23  11:17:06  11:17:06  dah (Dave Holt)
 * call get_maps when needed.
 * 
 * Revision 1.30.1.4  86/06/23  11:15:17  11:15:17  dah (Dave Holt)
 * use var_addr instead of var_value, since it's ok
 * 
 * Revision 1.30.1.3  86/06/23  11:13:17  11:13:17  dah (Dave Holt)
 * add comments
 * 
 * Revision 1.30.1.2  86/06/23  10:38:21  10:38:21  dah (Dave Holt)
 * mostly coded
 * 
 * Revision 1.30.1.1  86/06/23  10:27:57  10:27:57  dah (Dave Holt)
 * branch to try integrating gr's core file code
 * 
 * Revision 1.30  86/05/12  17:18:14  dah (Dave Holt)
 * change UNDERSCORES to UNDERSCORE
 * 
 * Revision 1.29  86/05/12  15:46:06  dah (Dave Holt)
 * Add comment headers to document the differences between the various
 * printing and exiting routines.
 * Change die to exit with a value of 0.
 * Change clean_up to use chat, instead of fprintf.
 * Change var_addr to support the underscore roll.
 * 
 * Revision 1.28  86/03/25  16:16:19  dah (Dave Holt)
 * resolve DSDtg00041.
 * Iscan now will have a separate message for <sysfile> not found
 * 
 * Revision 1.27.1.2  86/03/25  16:12:56  dah (Dave Holt)
 * more improvement needed
 * 
 * Revision 1.27.1.1  86/03/25  16:09:47  dah (Dave Holt)
 * try reporting a better error messageif the nlist filename isn't there
 * 
 * Revision 1.27  86/02/10  14:17:32  dah (Dave Holt)
 * add ifdef to handle older meas_sys.h (as in the 9.0 release - sigh)
 * 
 * Revision 1.26  86/02/04  10:59:20  dah (Dave Holt)
 * die is now quiet if chatty is disabled.
 * DSDtg00037
 * 
 * Revision 1.25  86/02/04  10:27:14  dah (Dave Holt)
 * bail_out now prints program name.
 * 
 * Revision 1.24  86/02/03  17:45:53  dah (Dave Holt)
 * fix DSDtg00112
 * (var_addr() initializes correctly, and do_nlist() no longer
 * called elsewhere).
 * 
 * Revision 1.23.1.1  86/01/31  16:34:28  dah (Dave Holt)
 * try fixing DSDtg00112
 * 
 * Revision 1.23  86/01/27  16:41:53  dah (Dave Holt)
 * make common.o ident'able
 * 
 * Revision 1.22  86/01/27  15:48:47  dah (Dave Holt)
 * handle new meas_sys.h (which defines MS_INCLUDE_REV instead of
 * declaring ms_include_rev)
 * 
 * Revision 1.21  86/01/27  15:44:54  dah (Dave Holt)
 * 1.20.1 seems fine, check it in.
 * 
 * Revision 1.20.1.1  85/12/12  16:58:29  dah (Dave Holt)
 * branch to give more info to wizards
 * 
 * Revision 1.20  85/12/12  16:43:52  dah (Dave Holt)
 * better dbg info in get_int*
 * 
 * Revision 1.19  85/12/11  15:25:31  dah (Dave Holt)
 * Even though the message says:
 * File common.c is unchanged with respect to revision 1.18
 * >> ckin anyway? [ny](n): y
 * It really has changed (though only the rcs log).
 * Amazingly, the previous rcs comment in the rcs log confused lint.
 * when varargs is in caps, anywhere in any comment it appears to
 * apply to the next function encountered.
 * 
 * Revision 1.18  85/12/11  15:15:35  dah (Dave Holt)
 * change varargs to varargs1 where appropriate
 * 
 * Revision 1.17  85/12/10  16:37:01  dah (Dave Holt)
 * make lint happy
 * 
 * Revision 1.16  85/12/10  15:16:24  dah (Dave Holt)
 * change "iscan" to progname
 * 
 * Revision 1.15  85/12/10  15:04:40  dah (Dave Holt)
 * bin_id() and normal_id() shouldn't be static in common.c
 * 
 * Revision 1.14  85/12/10  14:49:39  dah (Dave Holt)
 * pull get_int_2, since I shouldn't be using it anymore
 * 
 * Revision 1.13  85/12/10  14:35:24  dah (Dave Holt)
 * fix more problems
 * 
 * Revision 1.12  85/12/10  14:26:51  dah (Dave Holt)
 * include meas_sys.c and declare some externals.
 * 
 * Revision 1.11  85/12/10  14:21:05  dah (Dave Holt)
 * add typedef for KMEM_OFFSET (cc was dumping core, what a quality product!)
 * 
 * Revision 1.10  85/12/10  14:14:50  dah (Dave Holt)
 * include nlist.h, and declare nlst as external
 * 
 * Revision 1.8  85/12/10  14:08:09  dah (Dave Holt)
 * add var_addr() and get_int_3 to avoid all those X_FOO constants
 * being used.
 * 
 * Revision 1.7  85/12/09  15:57:31  dah (Dave Holt)
 * this version grammed the common stuff from iscan + msmod (iscan
 * invariably had better versions)
 * 
 * Revision 1.6  85/12/09  15:01:26  dah (Dave Holt)
 * clean up braindamaged code .
 * 
 * Revision 1.5  85/12/06  16:54:36  dah (Dave Holt)
 * try handling note.h
 * 
 * Revision 1.4  85/12/06  16:36:07  dah (Dave Holt)
 * include stdio.h and declare necessary externals
 * 
 * Revision 1.3  85/12/06  16:22:38  dah (Dave Holt)
 * clean up.
 * mention progname, rather than "iscan"
 * 
 * Revision 1.2  85/12/06  16:13:49  dah (Dave Holt)
 * Header added.  
 * 
 * $Endlog$
 */

/*LINTLIBRARY*/

#include <stdio.h>
#include <nlist.h>

#include <sys/sysmacros.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/vmmac.h>
#ifndef hp9000s200
#include <machine/pde.h>
#endif
#include <ctype.h>

#include "note.h"
#include <sys/meas_sys.h>
#include <assert.h>

#include <sys/ioctl.h>
#include <sys/meas_drivr.h>
#include <fcntl.h>
#include <errno.h>

#ifdef hp9000s800
#undef UNDERSCORE
#else
#define UNDERSCORE 1
#endif

extern int errno;
/*
 * MS_INCLUDE_REV *will* be defined (unless we have an out-of-date
 * meas_sys.h (as in rel9.0 - sigh))
 */
#ifdef MS_INCLUDE_REV
char ms_include_rev[] = MS_INCLUDE_REV;
#endif MS_INCLUDE_REV

typedef unsigned long KMEM_OFFSET;

extern char *progname;
extern int debug, chatty, wizard;
extern struct nlist nlst[];
extern int kmem, ms_id, ms_bin_id, my_rev, kmemf_flg;
extern char *nlist_filename;

#define sign21ext(foo) \
	(((foo) & 0x100000) ? ((foo) | 0xffe00000) : (foo))

#ifndef hp9000s200
struct  pde *pdir, *iopdir, *apdir, *aiopdir, *vpdir, *viopdir;
struct  hte *htbl, *ahtbl, *vhtbl;
int	nhtbl, npdir, niopdir, uptr, npids, niopids;
#endif


/*  return a char 0..9A..V that's the log(2) of n */
/*  I think negatives will look like 0 */
char lg(n)
int n;
{
    int i;
    
    for (i = 0; (i < 32) && (n > 0); i++, n>>=1)
        ;
    return( i < 10 ? '0' + i : 'A' + (i-10));
}

/*
 * Used for non-fatal, but important error messages.
 */
/*VARARGS1*/
warn(fmt, a0, a1, a2, a3, a4, a5, a6, a7)
    char *fmt;
{
    fprintf(stderr, "%s: ", progname);
    fprintf(stderr, fmt, a0, a1, a2, a3, a4, a5, a6, a7);
}

/*
 * Exit "normally".
 */ 
/*VARARGS1*/
die(s, a, b, c, d)
     char *s;
{
    (void) "$Header: common.c,v 66.1 91/05/09 10:44:00 ssa Exp $";
    /* Identify our .o file; Here is as good a place as any. */

    if (chatty) {
	fprintf(stderr, "\n%s: ", progname);
	fprintf(stderr, s, a, b, c, d);
	fprintf(stderr, "\n");
    }
    post_note();
    clean_up(0);
}

/*
 * Use for handling errors in system or library calls
 * that set the external errno.
 */
perror_bail_out(s)
    char *s;
{
    fprintf(stderr, "\n%s: ", progname);
    perror(s);			/* perror() writes a trailing newline */
    clean_up(1);
}

/*
 * Used in die(), perror_bail_out(), and bail_out() .
 * Can handle normal and abnormal termination.
 */
clean_up(exit_code)
    int exit_code;
{
    chat("%s done\n", progname);
    exit(exit_code);
}

/*VARARGS1*/
/*
 * Abnormal termination, but errno isn't set.
 * 
 * bail_out writes a newline before and after the given string.
 */
bail_out(fmt, a0, a1, a2, a3, a4, a5, a6, a7)
    char *fmt;
{
    fprintf(stderr,"\n%s: ", progname);
    fprintf(stderr, fmt, a0, a1, a2, a3, a4, a5, a6, a7);
    fprintf(stderr,"\n");
    clean_up(1);
}

/*
 * Used for debugging info.  Controlled by the global variable 'debug'.
 */
/*VARARGS1*/
dbg(s, a, b, c, d)
    char *s;
{
    if (debug) {
	fprintf(stderr, "%s: ", progname);
	fprintf(stderr, s, a, b, c, d);
	fprintf(stderr, "\n");
    }
}

/*
 * Used for friendly, but potentially annoying messages.
 * Controlled by the global variable 'chatty'.
 */
/*VARARGS1*/
chat(s, a, b, c, d)
    char *s;
{
    if (chatty) {
	fprintf(stderr, s, a, b, c, d);
    }
}

/*
 * Note that unless specified otherwise, addresses in kmem are virtual.
 * This is the way kmem is usually addressed, but is different than
 * the way /dev/mem or corefiles need to be addressed.  The distinction
 * is buried in get_bytes and below.
 */

/*
 * read data from kmem at the given address.
 * All reads from kmem should go through here.
 * (note that get does an end run).
 */
int
get_bytes(addr, buf, len)
    KMEM_OFFSET	addr;
    char *buf;
    int len;
{
    int last_read, last_byte, pgsize, mask, bytes_to_do;

#ifndef hp9000s200
    if (!kmemf_flg) {
#endif
        lseek_kmem(addr);
        if ( read(kmem, buf, len) != len ) {
            fprintf(stderr,"%s: read failed in get_bytes(0x%x, 0x%x, %d)\n",
                progname, addr, buf, len);
            perror("read");
            return(-1);
        }
#ifndef hp9000s200
    } else {
	/*
	 * Never read more than a page at a time (maps may change
	 * across page boundaries.
	 */
	last_byte = addr + len -1;
	pgsize = 1 << PGSHIFT;
	mask = pgsize -1;
	last_read = 0;

        while (!last_read) {
            if ((addr & ~mask) != (last_byte & ~mask)) { 
                /* if it crosses a page boundary */
                bytes_to_do = pgsize - (addr & mask);
            } else {
                bytes_to_do = last_byte - addr +1;
                last_read = 1;
            }
            lseek_kmem(getphyaddr(addr));
            if ( read(kmem, buf, bytes_to_do) != bytes_to_do ) {
                fprintf(stderr,"%s: read failed in get_bytes(0x%x, 0x%x, %d)\n",
                        progname, addr, buf, bytes_to_do);
                perror("read");
                return(-1);
            }
            addr += bytes_to_do;
            buf += bytes_to_do;
        }
    }
#endif not hp9000s200
    return(0);
}

/*
 * Build a table which maps strings into offsets in kmem.
 */
do_nlist()
{
    int i;
    
    dbg("starting do_nlist");
    
    if (nlist(nlist_filename, nlst) != 0) {
	perror_bail_out(nlist_filename);
    }
    
    /*
     * Use both loop tests, since Sys V says termination is a null name,
     * while HP-UX B.1 std says null name pointer.  This code will
     * correctly handle either usage.
     */
    for (i = 0; 
         (nlst[i].n_name != (char *)0) &&(nlst[i].n_name[0] != '\0'); 
         i++) {
	/* this test is compatible with V and 4.2 */
	if (nlst[i].n_value == 0) { 	
	    bail_out("%s does not have an entry for %s",
	        nlist_filename, nlst[i].n_name);
	}
	dbg("lookup of %s gave 0x%x",
	    nlst[i].n_name, nlst[i].n_value);
    }
    dbg("do_nlist() finished");
}    

/*
 * Build a table which maps strings into offsets in kmem.
 */
int
do_nlist_equiv()
{
    int i;
    struct ms_nlist mnl;
    int fd, err;
    
    dbg("starting do_nlist_equiv");
    
    fd = open_meas_drivr();
    if (fd < 0) {
	return(-1);
    }

    for (i = 0; (nlst[i].n_name[0] != '\0'); i++) {
	/* there's got to be an easier way (that's safe) */
	strncpy(mnl.name, nlst[i].n_name, sizeof(mnl.name) -1);
	mnl.name[sizeof(mnl.name) -1] = '\0'; /* guarranty terminating null */

	err = ioctl(fd, MS_NLIST, &mnl);
	if (err != 0) {
	    if (errno == ENXIO) { /* happens on older (1.0) kernels */
		chat("Kernel is Release 1.0, consider updating\n");
		return(-1);
	    } else {
		perror_bail_out("ioctl");
	    }
	}
	if (mnl.address == 0) {
	    bail_out("Kernel (ioctl) does not have an entry for %s",
		     nlst[i].n_name);
	}
	nlst[i].n_value = (unsigned long) mnl.address;
	dbg("do_nlist_equiv: lookup of %s gave 0x%x", mnl.name, nlst[i].n_value);
    }
    dbg("do_nlist_equiv() finished");
    return(0);
}    

/*
 * Check that rev of measurement system in kernel is one we can handle.
 * If this is wrong, the system probably won't work, but let wizards
 * try anyway.
 */
check_ms_rev()
{    
    int ms_rev;
    
    /* grab ms_rev from kernel memory */
    ms_rev = get_int_3("ms_rev");
    
    if (wizard) {
	printf("ms_rev = %d, my_rev = %d\n", ms_rev, my_rev);
    }
    if (ms_rev != my_rev) {
	if (wizard) {
	    warn("kernel protocol rev is %d.  I only speak %d\n",
	    	ms_rev, my_rev);
	    warn("Proceeding against my better judgement...\n");
	} else {
	    warn("kernel protocol rev is %d.  I only speak %d\n",
	    	ms_rev, my_rev);
	    warn("This is a serious difference.\n");
	    bail_out("Update kernel or this prog as appropriate.");
	}
    }
}

check_include_rev()
{
    KMEM_OFFSET addr, var_addr();
    char kernel_include_rev[80];
    int err;
    
    /*  check that revs of include file for kernel + this prog agree */
    addr = var_addr("ms_include_rev");
    err = get_bytes(addr, kernel_include_rev, sizeof(kernel_include_rev));
    if (err)  {
	perror_bail_out("check_include_rev");
    }
    if (wizard) {
	printf("kernel's include rev = \"%s\"\n", kernel_include_rev);
	printf("my include rev = \"%s\"\n", ms_include_rev);
    }
    if (strcmp(kernel_include_rev, ms_include_rev) != 0) {
        chat("include revisions disagree.\n");
	chat("mine: %s\n",ms_include_rev);
	chat("kernels: %s\n",(char *) kernel_include_rev);
	chat("With luck, this is not a major problem\n");
	chat("Recompile kernel or this prog (as appropriate) to be safe\n");
    }
}

/*
 * Do an lseek on kmem, using an unsigned long as the offset
 * Nlist returns the value as an unsigned long, but lseek requires 
 * a long.
 * I doubt it will ever make a difference, but at least it's done right.
 */
lseek_kmem(offset)
    KMEM_OFFSET offset;
{
/* Bell wants you to use numbers??? */    
#ifndef L_SET
#define L_SET	0
#define L_INCR	1
#endif    
    
    long lseek();
    unsigned long max_long = (1 << (8 * sizeof(long) -1)) -1;

    if (offset < max_long) {
	if (lseek(kmem, (long) offset, L_SET) == -1)
	    perror_bail_out("lseek error");
    } else {
	if (lseek(kmem, (long) max_long, L_SET) == -1)
	    perror_bail_out("lseek error");
	if (lseek(kmem, (long) (offset - max_long), L_INCR) == -1)
	    perror_bail_out("lseek error");
    }
}

/* 
 * get integer from given address in kmem.
 */
int
get_int(addr)
    KMEM_OFFSET addr;
{
    int	value, err;

    err = get_bytes(addr, (char *) &value, sizeof(value));
    if (err != 0) {
	bail_out("get_int(0x%x) failed", addr);
    }
    dbg("get_int(0x%x) = %d", addr, value);
    return(value);    
}

/*
 * Specialized routine: grab the nth allocated id string from the kernel
 * 
 * Name should be at least about 1000 bytes.
 *
 * It would be nice to merge common code from here and get_flag_addr()
 * but it doesn't seem worthwhile.
 */
grab_name(id, name)
    int id;
    char *name;
{
    KMEM_OFFSET array_base_addr, ptr_addr, ptr, get_kmem_offset(), var_addr();
    int byte_offset, err;    
    
    if (normal_id(id)) {
	array_base_addr = var_addr("ms_id_str_ptrs");
	byte_offset = (id-MS_MIN_NORMAL_ID) * sizeof(char *);
	ptr_addr =array_base_addr + byte_offset;
	ptr = get_kmem_offset(ptr_addr);
	err = get_bytes(ptr, name, 1000);
	if (err != 0) {
	    bail_out("grab_name: get_bytes failed");
	}
    } else if (bin_id(id)) {
	array_base_addr = var_addr("ms_bin_id_str_ptrs");
	byte_offset = (id-MS_MIN_BIN_ID) * sizeof(char *);
	ptr_addr =array_base_addr + byte_offset;
	ptr = get_kmem_offset(ptr_addr);
	err = get_bytes(ptr, name, 1000);
	if (err != 0) {
	    bail_out("grab_name: get_bytes failed");
	}
    } else {
	bail_out("grab_name, bad id %d", id);
    }
}

/*
 * Is id a legally allocated normal id?
 */
int
normal_id(id)
    int id;
{
    return((MS_MIN_NORMAL_ID <= id) &&
    	   (id < ms_id));
}

/*
 * Is id a legally allocated bin id?
 */
int
bin_id(id)
    int id;
{
    return((MS_MIN_BIN_ID <= id) &&
    	   (id < ms_bin_id));
}

/*
 * Like get_int, but returns an addr in kmem, rather than an int.
 */
KMEM_OFFSET
get_kmem_offset(addr)
    KMEM_OFFSET addr;
{
    KMEM_OFFSET	foo;
    int err;

    err = get_bytes(addr, (char *) &foo, sizeof(foo));
    if (err != 0) {
	bail_out("get_bytes failed in get_kmem_offset");
    }
    return(foo);    
}

/*
 * Use nlist() results to find the address in kmem corresponding
 * to a given string name.
 *
 * Added 851210 to facilitate modularization  - dah 
 */

/*
 * 910507 - reading core files has become increasingly complex.
 * From two separate formats (s300 and s800) we now have 5:
 *
 * s300 without regions (pre-8.0)
 * s300 with regions
 * s800 without regions (pre-8.0)
 * s800 with regions
 * s800 with sparse_pdir (PA 1.1)
 *
 * All of them require ugly, unsuppored (unsupportable?) code.
 * The sparse_pdir stuff doesn't even compile (8.02).
 * The 8.0 versions compile hasn't been tested but might work.
 * Overall, the -f option to iscan should be considered obsolescent
 * until a supported/supportable routine for doing ltor() is available.
 * (I tried extracting code from analyze, but it was over 750 lines
 * and still only working on 3 of the 5 cases above.
 *                                                  - dah
 */

KMEM_OFFSET 
var_addr(str)
     char *str;
{
    int i;
    static int first_time = 1;

    /* if nlist() hasn't happened yet, do it */
    if (first_time) {
	first_time = 0;
	if (kmemf_flg) {
	    do_nlist();
#ifdef hp9000s800	    
	    get_maps();
#endif hp9000s800
	} else {
	    if (do_nlist_equiv() == -1) { /* use ioctl equivalent */
		do_nlist();	/* ioctl failed, try nlist */
            }	 
	}
    }

    /* look through the table for a match */
    /* sometime recode this using a pointer */
    for (i = 0; (nlst[i].n_name[0] != '\0'); i++) {
#ifdef UNDERSCORE
        /* we have "foo", nlst[] has "_foo" */
	if (strcmp(str, &(nlst[i].n_name[1])) == 0) {
#else !UNDERSCORE
        /* we have "foo", nlst[] has "foo" */
	if (strcmp(str, &(nlst[i].n_name[0])) == 0) {
#endif UNDERSCORE
	    return(nlst[i].n_value);
	}
    }
    bail_out("var_addr: %s not in nlst", str);
    /*NOTREACHED*/
}

int
get_int_3(var_name)
     char *var_name;
{
    KMEM_OFFSET addr, var_addr();
    int value;

    addr = var_addr(var_name);
    value = get_int(addr);
    dbg("get_int_3(0x%x) \"_%s\" = %d", addr, var_name, value);
    return(value);
}

/* return the file descriptor for the opened special file */
int
open_meas_drivr()
{
    static int fd = -17;

    if (fd == -17) {		/* first time only */
	if ((fd = open(DEVMEAS_DRIVR, O_RDONLY, 0)) < 0) {
	    chat("Can't open %s\n", DEVMEAS_DRIVR);
	    perror("open");
	    chat("Do 'mknod /dev/meas_drivr c 41 0' to fix\n");
	    return(-1);
	}
    }
    return(fd);
}

#ifndef hp9000s200
#ifdef SPARSE_PDIR
get_maps()
{
    fprintf(stderr, "Reading core files not supported on PA89.  Sorry.\n");
    exit(1);
}
#else

/*
 * The following code is from gr
 */

get_maps()
{

	vpdir = apdir = (struct pde *)get(var_addr("pdir"));
	npdir = get(var_addr("npdir"));
	viopdir = aiopdir = (struct pde *)get(var_addr("iopdir"));
	niopdir = get(var_addr("niopdir"));
	vhtbl = ahtbl = (struct hte *)get(var_addr("htbl"));
	nhtbl = get(var_addr("nhtbl"));

	/* Get pdir and hash table space and addesses */
	iopdir = (struct pde *)malloc((niopdir +npdir) * sizeof (struct pde));
	/* pdir is an offset into iopdir */
	pdir = iopdir + niopdir;
	htbl = (struct hte *)malloc(nhtbl * sizeof (struct hte));

	/* read in pdir */
	lseek(kmem, (long)(aiopdir), 0);
	if (read(kmem, (char *)iopdir, (niopdir+npdir) * sizeof (struct pde))
	  !=  (niopdir + npdir) * sizeof (struct pde)) {
		perror("pdir read");
		exit(1);
	}

	/* read in hash table */
	lseek(kmem, (long)(ahtbl), 0);
	if (read(kmem, (char *)htbl, nhtbl * sizeof (struct hte))
	    !=  nhtbl * sizeof (struct hte)) {
	 	perror("htbl read");
		exit(1);
	}

}
#endif /* not SPARSE_PDIR */
#endif not hp9000s200

/* Gets the word at the PHYSICAL location specified */
get(loc)
unsigned loc;
{
	int x;
	
	if (loc == 0){
		fprintf(stderr," get: loc zero\n");
		return(0);
	}
	lseek(kmem, (long)(loc), 0);
	if (read(kmem, (char *)&x, sizeof (int)) != sizeof (int)) {
		perror("read");
		fprintf(stderr, "get failed on %x\n", (loc));
		return (0);
	}
	return (x);
}

#ifndef hp9000s200

#ifdef SPARSE_PDIR

getphyaddr(vaddr)
	unsigned vaddr;
{
    fprintf(stderr, "Reading core files not supported on PA89.  Sorry.\n");
    exit(1);
}

#else /* not SPARSE_PDIR */

getphyaddr(vaddr)
	unsigned vaddr;
{
	unsigned raddr;

	raddr = ltor(0,vaddr);
	return (raddr);

}

/* Translate the virtual address to a real address, looking for both
 * valid and prevalid addresses.
 */
ltor(space, offset)
	space_t space;
	caddr_t offset;
{
	struct pde *pde;
	u_int page;
	caddr_t raddr;


	 /* Basically this is ripped out of vm_machdep.c. It of course
	  * cannot use the lpa instruction, so that piece has been 
	  * removed. It also compensates for htbl, and pdir being
	  * local structures, but the internal pointers are still
	  * relative to the core files pdir and hash table.
	  *
	  *          pdir to be a pointer to a local copy of the pdir.
	  *          htbl to be a pointer to a local copy of the hash table.
	  */
	page = btop(offset);
	pde = (struct pde *) &htbl[
		pdirhash(space, (u_int) ptob(page)) & (nhtbl - 1)
		];
	

	/* Pass up the valid portion of the chain.  We know it
	 * is not there.
	 */
	for (;;) {
		if (pde->pde_next == 0) {
			return((caddr_t)0);
		}

		pde = &pdir[sign21ext(pde->pde_next)];

		/* Check for bum pointers */
		if ((pde < iopdir) || (pde > &pdir[npdir])){
			fprintf(stderr," Warning bad pdir next field, translation terminated!!\n");
			fprintf(stderr," pde_next was 0x%x. Returning 0 not found\n");
			return((caddr_t)0);
		}

		if (pde->pde_space == space
			&& pde->pde_page == page)
			break;
	}

	raddr = (caddr_t) ptob(pdetopg(pde));
	raddr = (caddr_t) (u_int) raddr +
			( (u_int) offset & ( (1 << PGSHIFT) -1));

	return (raddr);
}
#endif /* not SPARSE_PDIR */
#endif not hp9000s200
