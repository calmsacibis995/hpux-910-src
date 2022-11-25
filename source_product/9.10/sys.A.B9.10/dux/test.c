/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/test.c,v $
 * $Revision: 1.7.83.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/15 12:02:25 $
 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

#ifdef __hp9000s300

/*****************************************************************************
 * Prior to my modifications to this file, it did not compile on either 
 * system.  It used to have ifdefs so that it would compile on S800's back
 * when it did compile, but lots of things don't seem like they would work:
 * Most notibly the dump*stack* routines, which are what I really wanted
 * to fix.  So, I have ifdefed the entire file for S300 to make sure that
 * I don't break anything on the S800 by accident
 * 	Bret Mckee 12/4/90
 *****************************************************************************/

/*
 *	11-11-93   DKM removed the dump_stack stuff and put it in 
 *		       subr_xxx.c so it will work even when DUX is
 *		       configured out of someone's kernel
 */
 
#include	"../h/param.h"
#include	"../h/systm.h"
#include	"../dux/dm.h"
#include	"../dux/protocol.h"
#include	"../h/map.h"
#include	"../h/user.h"
#include	"../dux/rmswap.h"
#include	"../dux/cct.h"
/*
#include	"../h/dux_mbuf.h"
*/
#include	"../wsio/timeout.h"
#include	"../dux/nsp.h"
#include	"../h/proc.h"
#undef timeout

#define	MAXARG	31	/* longest string accepted */

extern int printf(), uprintf(), msg_printf(), (*kdb_printf)();
extern int (*test_printf)(); /* defined in trap.c */

/* return values from calls to switch printf destination */
#define	PRINTF		0
#define	UPRINTF		1
#define	MSG_PRINTF	2
#define	KDB_PRINTF	3

#define	printf (*test_printf)

extern giveup_clusters();
extern giveup_mbufs();


extern	int	vm_debug;
extern	struct	timeval	time;

extern int num_nsps;
extern int free_nsps;
extern int nsps_to_invoke;
extern int nsps_started;
extern int nsp_first_try;
extern int max_nsp;
extern dm_message nsp_head;
extern dm_message nsp_tail;
extern dm_message limited_head;
extern dm_message limited_tail;

extern struct serving_entry serving_array[];
extern int serving_array_size;
extern struct using_entry using_array[];
extern int using_array_size;

int mbufs_dm_alloced = 0;
int bufs_alloced = 0;
int clusters_dm_alloced = 0;
#define	MAXMBUFS      320
#define	MAXBUFS       200
struct dux_mbuf  *reqp[MAXMBUFS];
struct buf *bufp[MAXBUFS];
int really_giveup_mbufs();
int really_giveup_bufs();
int giveup_bufs();
struct sw_intloc free_mbuf_intloc;
struct sw_intloc free_buf_intloc;
struct sw_intloc free_cbuf_intloc;
extern LanDelaySum, LanDelayN, LanDelayAvg;	/* clocksync LAN delay */
extern syncN, syncMax, syncAvg, syncMin, syncSum;	/* clocksync stats */

char dump_ground[4095];		/* A data dumping ground, when you need to 
				write something somewhere! */

test()
{
	struct a {
		int	argc;
		char	**argv;
	} *uap = (struct a *)u.u_ap;


	char	*EAT_MBUFS = "eat_mbufs";
	char	*EAT_FSBUFS = "eat_fsbufs";
	char	*GIVEUP_MBUFS = "giveup_mbufs";
	char	*EAT_CLUSTERS = "eat_clusters";
	char	*GIVEUP_CLUSTERS = "giveup_clusters";
	char	*NSPINFO = "nspinfo";
	char	*NSPS = "nsps";
	char 	*DUX_MBSTAT= "dux_mbstat_info";
	char	*DUX_MBUF_INFO = "mbuf_info";
	char	*LENGTHOF_DUXQ = "lengthof_duxQ";
	char	*FIELDSOF_DUXQ = "fieldsof_duxQ";
	char	*RAW_MBUF_INFO = "raw_mbuf_info";
	char	*DUP_SERVING_MBUFS = "dup_serving_mbufs";
	char 	*SERVING_ARRAY_INFO = "serving_array_info";
	char	*SWAPEXPAND = "swapexpand";
	char	*SWAPCONTRACT = "swapcontract";

	char	*RMALLOC = "rmalloc";
	char	*RMFREE = "rmfree";
	char	*SWAPMAP = "swapmap";
	char	*ARGMAP = "argmap";

	char	*PRINTSWDEVT = "printswdevt";
	char	*PRINTSWAPMAP = "printswapmap";
	char	*PRINTARGMAP = "printargmap";
	char	*PRINTVMMESG = "printvmmesg";
	char	*SCR = "scr";

	char	*RECOVERY_ON = "recovery_on";
	char	*RECOVERY_OFF = "recovery_off";

        char	*PRINTCLKSTATS = "printclkstats";
        char	*RESETCLKSTATS = "resetclkstats";
        char	*RESETCLKDELAY = "resetclkdelay";

	char	*TESTPRINT_CONSOLE = "testprint_console";
	char	*TESTPRINT_USER = "testprint_user";
	char	*TESTPRINT_DMESG = "testprint_dmesg";
	char	*TESTPRINT_DEBUG = "testprint_debug";

	char	*TESTR = "testr";
	char	*TESTW = "testw";
	char	*INIT68881 = "initialize_68881";
	char	*BCOPY = "bcopy_prot";
	char	*SCOPY = "scopy_prot";
	char	*LCOPY = "lcopy_prot";

	char	temp[MAXARG+1];
	char	temp1[MAXARG+1];
	char	temp2[MAXARG+1];
	char	temp3[MAXARG+1];
	int	addr, num, howlong, c, from, to, s, sec, size, usec;
	int	(*func)();

	struct	dux_vmmesg	*vmmesg;
	struct	dux_timemesg	*timemesg;
	struct	timeval	*tp;


	/* point to the function name */
	uap->argv++;

	/* set temp to the function name */
	getargu(uap->argv++,temp);

#ifdef LATER
	if( !stringcmp(SWAPEXPAND,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp);
		/* convert the string to hex */
		if ( (size = atox(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		if (my_site_status & CCT_SLWS) {
			while (!chunk_table_lock());
			u.u_r.r_val1 = swapexpand(size);
			chunk_table_unlock();
		} else
			u.u_r.r_val1 = swapexpand(size);
/* 
   The next six (6) are for giving access to several kernel routines which
   are armored against bus faults.  This allows them to be tested from a
   user land program.  This was done to allow some 7.0 White Box testing.
*/
	} else	if (!stringcmp(TESTR,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp);
		/* convert the string to int */
		if ( (to = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* get the second argument */
		getargu(uap->argv++,temp);
		/* convert the string to int */
		if ( (size = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		u.u_r.r_val1 = testr((caddr_t)to,size);
		return;

	} else	if (!stringcmp(TESTW,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp);
		/* convert the string to int */
		if ( (to = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* get the second argument */
		getargu(uap->argv++,temp);
		/* convert the string to int */
		if ( (size = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		u.u_r.r_val1 = testw((caddr_t)to,size);
		return;

	} else	if (!stringcmp(BCOPY,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp);
		/* convert the string to int */
		if ( (from = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* get the second argument */
		getargu(uap->argv++,temp);
		/* convert the string to int */
		if ( (to = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* get the third argument */
		getargu(uap->argv++,temp);
		/* convert the string to int */
		if ( (size = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		u.u_r.r_val1 = bcopy_prot((caddr_t)from,(caddr_t)to,size);
		return;

	} else	if (!stringcmp(SCOPY,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp);
		/* convert the string to int */
		if ( (from = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* get the second argument */
		getargu(uap->argv++,temp);
		/* convert the string to int */
		if ( (to = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* get the third argument */
		getargu(uap->argv++,temp);
		/* convert the string to int */
		if ( (size = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		u.u_r.r_val1 = scopy_prot((caddr_t)from,(caddr_t)to,size);
		return;

	} else	if (!stringcmp(LCOPY,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp);
		/* convert the string to int */
		if ( (from = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* get the second argument */
		getargu(uap->argv++,temp);
		/* convert the string to int */
		if ( (to = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* get the third argument */
		getargu(uap->argv++,temp);
		/* convert the string to int */
		if ( (size = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		u.u_r.r_val1 = lcopy_prot((caddr_t)from,(caddr_t)to,size);
		return;
	} else if (!stringcmp(INIT68881,temp)) {
#else
	if (!stringcmp(INIT68881,temp)) {
#endif
		u.u_r.r_val1 = initialize_68881();
		return;

	} else	if (!stringcmp(NSPINFO,temp)) {
		u.u_r.r_val1 = nspinfo();
		return;
	} else	if (!stringcmp(NSPS,temp)) {
		u.u_r.r_val1 = nsps();
		return;
#ifdef LATER
	} else	if (!stringcmp(DUX_MBSTAT,temp)) {
		u.u_r.r_val1 = dux_mbstat_info();
		return;
	} else	if (!stringcmp(LENGTHOF_DUXQ,temp)) {
		u.u_r.r_val1 = lengthof_duxQ();
		return;
	} else	if (!stringcmp(RAW_MBUF_INFO,temp)) {
		u.u_r.r_val1 = raw_mbuf_info();
		return;
	} else	if (!stringcmp(DUP_SERVING_MBUFS,temp)) {
		u.u_r.r_val1 = dup_serving_mbufs();
		return;
	} else	if (!stringcmp(SERVING_ARRAY_INFO,temp)) {
		u.u_r.r_val1 = serving_array_info();
		return;
	} else	if (!stringcmp(FIELDSOF_DUXQ,temp)) {
		u.u_r.r_val1 = fieldsof_duxQ();
		return;
	} else	if (!stringcmp(EAT_MBUFS,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp);
		/* convert the string to hex */
		if ( (num = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* get the second argument*/
		getargu(uap->argv++,temp1);
		/* convert the string to hex */
		if ( (howlong = atoi(temp1)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		u.u_r.r_val1 = eat_mbufs(num, howlong);
		return;
	} else	if (!stringcmp(EAT_FSBUFS,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp);
		/* convert the string to hex */
		if ( (num = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* get the second argument*/
		getargu(uap->argv++,temp1);
		/* convert the string to hex */
		if ( (howlong = atoi(temp1)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		u.u_r.r_val1 = eat_fsbufs(num, howlong);
		return;
	} else	if (!stringcmp(GIVEUP_MBUFS,temp)) {
		u.u_r.r_val1 = giveup_mbufs();
		return;
	} else	if (!stringcmp(EAT_CLUSTERS,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp);
		/* convert the string to hex */
		if ( (num = atoi(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* get the second argument*/
		getargu(uap->argv++,temp1);
		/* convert the string to hex */
		if ( (howlong = atoi(temp1)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		u.u_r.r_val1 = eat_clusters(num, howlong);
		return;
	} else	if (!stringcmp(GIVEUP_CLUSTERS,temp)) {
		u.u_r.r_val1 = giveup_clusters();
		return;
	} else	if (!stringcmp(DUX_MBUF_INFO,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp);
		/* convert the string to hex */
		if ( (c = atox(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		u.u_r.r_val1 = mbuf_info( c );
#endif
	} else	if (!stringcmp(SWAPCONTRACT,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp);
		/* convert the string to hex */
		if ( (c = atox(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* call the function and set the return value */
		u.u_r.r_val1 = swapcontract(c);
#ifdef LATER
	} else	if (!stringcmp(RMALLOC,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp1);
		/* get the second argument*/
		getargu(uap->argv++,temp);
		/* convert the string to hex */
		if ( (size = atox(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* call the function and set the return value */
		if (!stringcmp(SWAPMAP, temp1)) {
			u.u_r.r_val1 = rmalloc(swapmap, size);
		} else if (!stringcmp(ARGMAP, temp1)){
			u.u_r.r_val1 = rmalloc(argmap, size);
		} else
			printf("test : undefined map \n");
	} else	if (!stringcmp(RMFREE,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp1);

		/* get the second argument*/
		getargu(uap->argv++,temp);
		/* convert the string to hex */
		if ( (size = atox(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* get the third argument*/
		getargu(uap->argv++,temp);
		/* convert the string to hex */
		if ( (addr = atox(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* call the function and set the return value */
		if (!stringcmp(SWAPMAP, temp1)) {
			u.u_r.r_val1 = rmfree(swapmap, size, addr);
		} else if (!stringcmp(ARGMAP, temp1)){
			u.u_r.r_val1 = rmfree(argmap, size, addr);
		} else
			printf("test : undefined map\n");

	} else	if (!stringcmp(PRINTSWDEVT,temp)) {
		/* call the function and set the return value */
		u.u_r.r_val1 = printswdevt();

	} else	if (!stringcmp(PRINTSWAPMAP,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp);
		/* convert the string to hex */
		if ( (from = atox(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* get the second argument*/
		getargu(uap->argv++,temp);
		/* convert the string to hex */
		if ( (to = atox(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* call the function and set the return value */
		u.u_r.r_val1 = printswapmap(from, to);

	} else	if (!stringcmp(PRINTARGMAP,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp);
		/* convert the string to hex */
		if ( (from = atox(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* get the second argument*/
		getargu(uap->argv++,temp);
		/* convert the string to hex */
		if ( (to = atox(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* call the function and set the return value */
		u.u_r.r_val1 = printargmap(from, to);

	} else	if (!stringcmp(PRINTVMMESG,temp)) {
		/* get the first argument*/
		getargu(uap->argv++,temp);
		/* convert the string to hex */
		if ( (vmmesg = (struct dux_vmmesg *) atox(temp)) < 0) {
			u.u_r.r_val1 = -1;
			return;
		}
		/* call the function and set the return value */
		u.u_r.r_val1 = printvmmesg(vmmesg);

	} else	if (!stringcmp(SCR,temp)) {
		/* call the function and set the return value */
		u.u_r.r_val1 = sched_chunk_release();

#endif
	} else	if (!stringcmp(RECOVERY_ON,temp)) {
		/* call the function and set the return value */
		u.u_r.r_val1 = recovery_on();

	} else	if (!stringcmp(RECOVERY_OFF,temp)) {
		/* call the function and set the return value */
		u.u_r.r_val1 = recovery_off();

        } else  if (!stringcmp(PRINTCLKSTATS,temp)) {
                copyout((caddr_t) &LanDelayN, (caddr_t)uap->argv,4*6);  

        } else  if (!stringcmp(RESETCLKSTATS,temp)) {
                syncMin = 0x7fffffff;
                syncMax = 0x80000000;
                syncSum = 0;
                syncN   = 0;

        } else  if (!stringcmp(RESETCLKDELAY,temp)) {
                LanDelayN = 0;
                LanDelaySum = 0;

	} else	if (!stringcmp(TESTPRINT_CONSOLE,temp)) {
		u.u_r.r_val1 = testprint_console();
	} else	if (!stringcmp(TESTPRINT_USER ,temp)) {
		u.u_r.r_val1 = testprint_user();
	} else	if (!stringcmp(TESTPRINT_DMESG ,temp)) {
		u.u_r.r_val1 = testprint_dmesg();
	} else	if (!stringcmp(TESTPRINT_DEBUG ,temp)) {
		u.u_r.r_val1 = testprint_debug();

	} else
		printf("Undefined function\n");
}

getargu(upptr,temp)
char	**upptr;
char	*temp;
{
	register int i;
	char	*ustrptr;

	ustrptr = (char *)fuword(upptr);

	i = 0;
	while( ( (i < MAXARG) && (*temp++=fubyte(ustrptr++)) != '\0') ) i++;
	if (i == MAXARG)
		*temp = '\0';
}

stringcmp(s, t)
char	*s, *t;
{
	for(;*s==*t; s++,t++)
		if(*s =='\0')
			return(0);
	return(*s -*t);
}


atox (ptr)
	char	*ptr;
{
	int	val;

	for (val=0; *ptr != '\0' ; *ptr++) {
		if (*ptr >= '0' && *ptr <= '9') {
			val = val * 16 + *ptr - '0';
		} else if (*ptr >= 'a' && *ptr <= 'f') {
			val = val * 16 + *ptr - 'a' + 10;
		} else
			return(-1);
	}
	return(val);
}



/*
** This function is used for testing purposes only.
** If prints out important variables specific to nsps.
** It is called from user land via the "generic" test
** system call.
*/
nspinfo()
{
	printf("\n-------------------------- nsp info --------------------\n");
	printf("size of CSP array  	    = ncsp           = %d\n",ncsp);
	printf("no. of CSPs alive  	    = num_nsps       = %d\n",num_nsps);
	printf("no. of CSPS to fork off     = nsps_to_invoke = %d\n", nsps_to_invoke);
	printf("total no. of CSPs           = nsps_started   = %d\n", nsps_started);
	printf("no. of non-busy CSPs 	    = free_nsps      = %d\n",free_nsps);
	printf("first CSP to try allocating = nsp_first_try  = %d\n", nsp_first_try);
	printf("max nsp slot in use         = max_nsp        = %d\n",max_nsp);
	printf("pointer to nsp_head         = nsp_head       = %x\n",nsp_head);
	printf("pointer to nsp_tail         = nsp_tail       = %x\n",nsp_tail);
	printf("pointer to limited_head     = limited_head   = %x\n",limited_head);
	printf("pointer to limited_tail     = limited_tail   = %x\n",limited_tail);
	printf("length of limited q: current = %d, max = %d\n",
		csp_stats.limitedq_curlen, csp_stats.limitedq_maxlen);
	printf("length of general q: current = %d, max = %d\n",
		csp_stats.generalq_curlen, csp_stats.generalq_maxlen);
	printf("minimum general CSPs free    = %d\n", csp_stats.min_gen_free);
	printf("limited requests:    total   = %d, max time = %d ticks\n",
		csp_stats.requests[CSPSTAT_LIMITED], csp_stats.max_lim_time);
	printf("general requests (by timeout): %5d short, %5d med, %5d long\n",
		csp_stats.requests[CSPSTAT_SHRT],
		csp_stats.requests[CSPSTAT_MED],
		csp_stats.requests[CSPSTAT_LONG]);
	printf("timed out requests:            %5d short, %5d med, %5d long\n",
		csp_stats.timeouts[CSPSTAT_SHRT],
		csp_stats.timeouts[CSPSTAT_MED],
		csp_stats.timeouts[CSPSTAT_LONG]);
	printf("---------------------- end nsp info --------------------\n");
	return(0);
}

/*
** This function is used for testing purposes only.
** If prints out information for each NSP.
** It is called from user land via the "generic" test
** system call.
*/
nsps()
{
	register int i;
	register struct nsp *nspp;

	printf("\n-------------------------- nsps ------------------------------\n");
	printf("no. of CSPs alive  	    = num_nsps       = %d\n",num_nsps);
	printf("SLOT   NSPP  FLAG    PID    PROCP CNODE  RPID      RID   WCHAN\n");
	for (i = 0, nspp = nsp; i < ncsp; i++, nspp++)
		if (nspp->nsp_flags & NSP_VALID)
			printf ("%3d  %6x  %04x  %5d  %6x  %3d  %5d 0x%06x  %6x\n",
				i, nspp, nspp->nsp_flags, nspp->nsp_proc->p_pid,
				nspp->nsp_proc, nspp->nsp_site,
				nspp->nsp_pid, nspp->nsp_rid,
				nspp->nsp_proc->p_wchan);
	printf("------------------------- end nsps ---------------------------\n");
	return(0);
}


#ifdef LATER
/*
** This function is used for testing purposes only.
** If prints out important variables specific to dux mbufs.
** It is called from user land via the "generic" test
** system call.
*/
dux_mbstat_info()
{
	printf("\n------------------------ dux_mbstat --------------------\n");
	printf("struct dux_mbstat{\n");
	printf("  mbuf pool                  = %d\n",dux_mbstat.m_mbufs);
	printf("  mbufs on free list         = %d\n",dux_mbstat.m_mbfree);
	printf("  total mbufs used           = %d\n",dux_mbstat.m_mbtotal);

	printf("  cluster pool               = %d\n",dux_mbstat.m_clusters);
	printf("  clusters on free list      = %d\n",dux_mbstat.m_clfree);
	printf("  total clusters used        = %d\n",dux_mbstat.m_cltotal);
	printf("  clusters from page pool    = %d\n",dux_mbstat.pagepool_clust);
	printf("}\n");
	printf("Warning: start/end addresses are for last expansion only\n");
	printf("struct mbuf_xtra_stats{\n");
	printf("  # of mbuf expans    = 0x%x\n",mbuf_xtra_stats.m_expand);
	printf("  mbuf start address  = 0x%x\n",mbuf_xtra_stats.dux_mbuf_start);
	printf("  mbuf end address    = 0x%x\n",mbuf_xtra_stats.dux_mbuf_end);
	printf("  cluster start addr  = 0x%x\n",mbuf_xtra_stats.dux_mcluster_start);
	printf("  cluster end addr    = 0x%x\n",mbuf_xtra_stats.dux_mcluster_end);
	printf("}\n");
}
#endif


/*
** This needs to be sw_triggered. Doesn't work correctly.
*/

eat_clusters( number, howlong)
int number;
int howlong;
{
register int i;
extern struct dux_mbuf *reqp[];
extern int clusters_dm_alloced;

	if ( number < 0 )
	{
		number = 0;
	}

	printf("\n-------------------- eat_clusters --------------------\n");

	printf("No. of clusters to eat=%d, for %d seconds\n", number, howlong);
	for( i = 0; i < number; i++ )
	{
		reqp[i] = (struct dux_mbuf *)dm_alloc(200, 1);
		if( reqp[i] != NULL)
		{
			clusters_dm_alloced++;
		}
	}

	if( (number > 0) && (howlong > 0))
	{
		timeout(giveup_clusters, (caddr_t)0, howlong * HZ);
		return(0);
	}
	else
	{
		return(-1);
	}

}

giveup_clusters( )
{
register int i;
extern struct dux_mbuf *reqp[];
extern clusters_dm_alloced;

	printf("\n-------------------- giveup_clusters --------------------\n");

	if (clusters_dm_alloced < 0)
	{
		clusters_dm_alloced = 0;
	}

	printf("Number of clusters to give back = %d\n", clusters_dm_alloced);
	for( i = 0; i < clusters_dm_alloced; i++ )
	{
		dm_release( (struct dux_mbuf *)reqp[i], 0);	
	}
	clusters_dm_alloced = 0;

}




#define FOUR_K  0x1000
eat_fsbufs( number, howlong)
int number;
int howlong;
{
register int i;
extern struct buf *bufp[]; /* MAXBUFS in size */
register struct buf *err;


	printf("\n------------------------ eat_fsbufs------------------\n");

	if ( number <= 0 )
	{
		printf("Error: number to obtain is <= 0\n");
		return(-1);
	}

	if ( howlong <= 0 )
	{
		printf("Warning: delay is <= 0\n");
		howlong = 1;
	}

	printf("No. of fsbufs to eat = %d, for %d seconds\n", number, howlong);
	if( number > MAXBUFS )
	{
		printf("Warning: Max to eat is %d fsbufs\n", MAXBUFS);
		number = MAXBUFS;
	}

	for( i = 0; i < number; i++ )
	{
		err = syncgeteblk(FOUR_K); /* Results in a page */

		if( (err == NULL) || (err == ((struct buf *)-1)) )
		{
			printf("Out of fsbufs to allocate; i = %d\n", i);
			goto setup_delay;
		}
		else
		{
			bufp[i] = err;
			bufs_alloced++;
		}
	}

setup_delay:

		timeout(giveup_bufs, (caddr_t)0, howlong * HZ);
		return(0);
}



giveup_bufs()
{
	sw_trigger( &free_buf_intloc, really_giveup_bufs, 0, 0, 1);
}

really_giveup_bufs( )
{
register int i;
int s;
extern struct buf *bufp[];
extern int bufs_alloced;

	printf("\n-----------------------giveup_bufs --------------------\n");

	if ( bufs_alloced <= 0 )
	{
		bufs_alloced = 0;
		printf("Error: no fsbufs to give up\n");
		return(-1);
	}

	printf("Number of fsbufs to give back = %d\n", bufs_alloced);

	for( i = 0; i < bufs_alloced; i++ )
	{
		if( bufp[i] != NULL )
		{
			brelse( (struct buf *)bufp[i] );
			bufp[i] = NULL;
		}
	}
	bufs_alloced = 0;

}






eat_mbufs( number, howlong)
int number;
int howlong;
{
register int i;
extern struct dux_mbuf *reqp[]; /* MAXMBUFS in size, equals 3 pages of mbufs */


	printf("\n------------------------ eat_mbufs --------------------\n");

	if ( number <= 0 )
	{
		printf("Error: number to obtain is <= 0\n");
		return(-1);
	}

	if ( howlong <= 0 )
	{
		printf("Warning: delay is = 0\n");
		howlong = 1;
	}

	printf("No. of mbufs to eat = %d, for %d seconds\n", number, howlong);
	if( number > MAXMBUFS )
	{
		printf("Warning: Max to eat is %d mbufs\n", MAXMBUFS);
		number = MAXMBUFS;
	}

	for( i = 0; i < number; i++ )
	{
		reqp[i] = (struct dux_mbuf *)dm_alloc(0, 1);
		mbufs_dm_alloced++;
	}

	timeout(giveup_mbufs, (caddr_t)0, howlong * HZ);
	return(0);
}	


giveup_mbufs()
{
	sw_trigger( &free_mbuf_intloc, really_giveup_mbufs, 0, 0, 1);
}

really_giveup_mbufs( )
{
register int i;
int s;
extern struct dux_mbuf *reqp[];
extern int mbufs_dm_alloced;

	printf("\n-----------------------giveup_mbufs --------------------\n");

	if ( mbufs_dm_alloced <= 0 )
	{
		mbufs_dm_alloced = 0;
		printf("Error: no mbufs to give up\n");
		return(-1);
	}

	printf("Number of mbufs to give back = %d\n", mbufs_dm_alloced);

	for( i = 0; i < mbufs_dm_alloced; i++ )
	{
		if( reqp[i] != NULL )
		{
			dm_release( (struct dux_mbuf *)reqp[i], 0);	
			reqp[i] = NULL;
		}
	}
	mbufs_dm_alloced = 0;

}


/*
** DUP_SERVING_MBUFS
**   This code doesn't work....................
*/
dup_serving_mbufs()
{
register struct serving_entry *serving_entry;
register struct serving_entry *serving_entry1;
register int i, j;


	serving_entry = serving_array;
	serving_entry1 = serving_array;
	printf("serving_array_size = %d\n", serving_array_size);
	printf("----------------------------------------------\n");


	for(i = 0; i < serving_array_size; i++, serving_entry++ )
	{
		for( j = 0; j < serving_array_size; j++, serving_entry1++ )
		{
	         if( serving_entry->msg_mbuf ==  serving_entry1->msg_mbuf)
		 {
		  printf("ERROR: serving_entries #%d and #%d have same mbuf\n",
				 i, j);
		  printf("	serving_entry[%d]->msg_mbuf = %x\n", 
				i, serving_entry->msg_mbuf);
		  printf("	serving_entry[%d]->msg_mbuf = %x\n", 
				j, serving_entry1->msg_mbuf);
		 }
		}
	}
	printf("--------------------DONE---------------------\n");
}


/*
** SERVING_ARRAY_INFO
*/

serving_array_info()
{
register struct serving_entry *serving_entry;
register struct dm_header *dmp;
register struct proto_header *ph;
register int i;

	printf("serving_array_size = %d\n", serving_array_size);
	printf("----------------------------------------------\n");

	for(i = 0; i < serving_array_size; i++ )
	{
	serving_entry = (struct serving_entry *)&serving_array[i];

	dmp = DM_HEADER(serving_entry->msg_mbuf);
	ph  = &(dmp->dm_ph);

	printf("------- serving_entry number %d ----- \n", i);
	printf("flags       = %x\n", serving_entry->flags);
	printf("req_index   = %x\n",serving_entry->req_index);
	printf("rid         = %x\n", serving_entry->rid);
	printf("state       = %x\n", serving_entry->state);
	printf("byte_recved = %x\n", serving_entry->byte_recved);
	printf("no_retried  = %x\n", serving_entry->no_retried);
	printf("msg_mbuf    = %x\n", serving_entry->msg_mbuf);
	printf("	--- msg_mbuf fields ---\n");
	printf("	ph->p_flags   = %x\n", ph->p_flags);
	printf("	ph->p_srcsite = %x\n", ph->p_srcsite);
	printf("	ph->p_rid     = %x\n", ph->p_rid);
	printf("	ph->p_byte_no = %x\n", ph->p_byte_no);
	printf("	ph->p_dmmsg_length = %x\n", ph->p_dmmsg_length);
	printf("	ph->p_data_length = %x\n", ph->p_data_length);
	printf("	ph->p_data_offset = %x\n", ph->p_data_offset);
	printf("	ph->p_req_index = %x\n", ph->p_req_index);
	printf("	ph->p_rep_index = %x\n", ph->p_rep_index);
	printf("	dmp->dm_op      = %x\n", dmp->dm_op);
	printf("	dmp->dm_pid     = %x\n", dmp->dm_pid);
	printf("	dmp->dm_rc      = %x\n", dmp->dm_rc);

	}
}

#ifdef LATER
/*
** Scan DUX xmitQ and return the length
*/
lengthof_duxQ()
{
extern struct dux_mbuf *duxQhead;
extern struct dux_mbuf *duxQtail;

dm_message  p, q;
int Qlength = 0;

	if( duxQhead == NULL )
	{
		printf("DUXQ_LENGTH == NULL\n");
		return(0);
	}

	if( duxQhead->m_act == NULL )
	{
		printf("DUXQ_LENGTH == 1\n");
		return(0);
	}


	Qlength++;
	q = duxQhead;
	while(  ( p = q->m_act) && ( p != NULL) )
	{
		Qlength++;
		q = p;
	}
	printf("DUXQ_LENGTH == %d\n",Qlength);
	return(0);
	
}
#endif

#ifdef LATER

fieldsof_duxQ()
{
extern struct dux_mbuf *duxQhead;
extern struct dux_mbuf *duxQtail;
struct dm_header *dmp;
struct proto_header *ph;

dm_message  p, q;
int Qlength = 0;

	if( duxQhead == NULL )
	{
		printf("DUXQ_LENGTH == NULL\n");
		return(0);
	}

	if( duxQhead->m_act == NULL )
	{
		printf("DUXQ_LENGTH == 1, look at manually\n");
		return(0);
	}


	Qlength++;
	q = duxQhead;
	while(  ( p = q->m_act) && ( p != NULL) )
	{
		dmp = DM_HEADER(p);
		ph  = &(dmp->dm_ph);

		printf("------------------------------\n");
		printf("duxQ element #%d\n", Qlength++);
printf("p_plags	dm_dest	p_dmmsg_length 	p_data_length	p_byte_no  dm_op dm_pid\n");
printf("%x	%x	%x	%x	%x	%x\n",
ph->p_flags, dmp->dm_dest, ph->p_dmmsg_length, ph->p_data_length, 
ph->p_byte_no, dmp->dm_op, dmp->dm_pid);

		q = p;
	}

	return(0);
	
}
#endif

#ifdef LATER

/*
** This function is used for testing purposes only.
** If prints out important variables specific to dux mbufs.
** It is called from user land via the "generic" test
** system call.
*/
mbuf_info( mbuf_ptr )
struct dux_mbuf *mbuf_ptr;
{

	printf("\n------------------- dux_mbuf_info--------------------\n\n");
	printf("mbuf start addr   =0x%x\n",mbuf_xtra_stats.dux_mbuf_start);
	printf("mbuf end addr     =0x%x\n",mbuf_xtra_stats.dux_mbuf_end);
	printf("cluster start addr=0x%x\n",mbuf_xtra_stats.dux_mcluster_start);
	printf("cluster end addr  =0x%x\n",mbuf_xtra_stats.dux_mcluster_end);

	if(  mbuf_ptr != NULL )
	{
	 printf(" \nSupplied mbuf ptr = 0x%x\n\n", mbuf_ptr);
	 printf(" struct{			\n");
	 printf("             *m_next  = 0x%x\n", mbuf_ptr->m_next);
	 printf("             m_len    = 0x%x  = %d (decimal)\n", mbuf_ptr->m_len, mbuf_ptr->m_len);
	 if( mbuf_ptr->m_type ==  DUX_FREE )
	 	printf("             m_type   = 0x%x = DUX_FREE\n", mbuf_ptr->m_type);
	 else
	 if( mbuf_ptr->m_type ==  DUX_DATA )
	 	printf("             m_type   = 0x%x = DUX_DATA\n", mbuf_ptr->m_type);
	 else
	 printf("             m_type   = 0x%x = ??? Could be a mcluster\n", mbuf_ptr->m_type);
	 printf("             dux_m_dat[0] = 0x%x  = %d (decimal)\n", mbuf_ptr->dux_m_dat[0]);
	 printf("             dux_m_dat[1] = 0x%x  = %d (decimal)\n", mbuf_ptr->dux_m_dat[1]);
	 printf("             *m_act   = 0x%x\n", mbuf_ptr->m_act);
	 printf("          }\n\n");
	}
	else
	 printf(" Supplied mbuf ptr = NULL \n");

	printf("\n--------------- dux_mbuf_info end--------------------\n\n");
}
#endif


dump_csp_stacks()
{
	register struct nsp *nspp;

	for (nspp = nsp; nspp < nspNCSP; nspp++)
		if (nspp->nsp_flags & NSP_VALID)
			dump_stack(nspp->nsp_proc);
}


#endif  /*  hp9000s200  */
