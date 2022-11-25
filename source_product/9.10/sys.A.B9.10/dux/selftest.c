/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/selftest.c,v $
 * $Revision: 1.9.83.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/15 12:02:19 $
 */

/* HPUX_ID: @(#)selftest.c	55.1		88/12/23 */

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


/*
 *Perform a site selftest.  This test is used to determine that the site
 *is correctly performing certain operations necessary to be a member of the
 *cluster in good standing.  If the operations cannot be performed, we
 *will disconnect ourselves from the cluster.  The following operations are
 *tested:
 *
 *	Regular buffer allocation:  Can we allocate a disc buffer?
 *
 *	Network buffer allocation:  Can we allocate a network buffer
 *	under interrupt?
 *
 *	DM message allocation:  Can we allocate a dm_message?
 *
 *	NSP invocation:  Can we invoke an NSP?
 *
 *Note that these operations will be happening as the system runs anyway.
 *Rather than continuously performing these operations, we will only
 *perform them if they haven't been performed anyway.
 */

#include "../h/param.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../dux/dm.h"
#include "../dux/selftest.h"
#include "../dux/nsp.h"
#include "../dux/cct.h"


#ifdef __hp9000s300
#include "../wsio/timeout.h"

#include  "../s200io/lnatypes.h"

struct sw_intloc selftest_intloc;
struct sw_intloc forceselftest_intloc;
#endif /* hp9000s300 */
int dux_force_selftest();
int force_selftest();
int schedule_selftest();
/* 
** timeout must be undefined here so that the correct timeout mechanism,
** the one with *three* parameters, is used
*/
#undef timeout

#ifndef VOLATILE_TUNE
/*
 * If VOLATILE_TUNE isn't defined, define 'volatile' so it expands to
 * the empty string.
 */
#define volatile
#endif /* VOLATILE_TUNE */


extern int selftest_period;	/* seconds between selftest execution */
#define RETRYSELFTEST_PERIOD 2 /* seconds between retrys of forced selftest */

/*
** The allocation of space for this global is in init_main.c. It can
** be initialized in any place that is kernel resident. Necessary for
** configuring out this routine for a standalone kernel.
*/
extern volatile int selftest_passed;
int failed_once;
int force_selftest_running;

/*
 *The selftest routine itself.  This routine is called by timeout every 
 *selftest_period seconds.  The algorithm is:
 *
 *	Check to see if all the tests have already passed as part of normal
 *	operation.  If so, just clear the flags and schedule the next test.
 *
 *	If the tests have not all been passed, call failed_selftest,which 
 *	checks to see if this is the first failure. If it is not the first
 *	failure, then compare this failure to the last and indicate which
 *	tests failed.  If it is the first failure, then set flags and
 *	continue.  In either case return to selftest which schedules
 *	the next selftest and the forced selftest.  
 */
selftest()
{
	int s;

	/* Need to call this routine every once in a while, might as well
	** do it here. I was seeing 100 old serving array entries, but the
	** serving array didn't fill so they weren't getting cleaned up.
	** This will help to get rid of them earlier instead of having them
	** waste system resources. cwb
	*/
	s = spl6();
	remove_old_serving_entries();
	splx(s);

	if (selftest_passed == ST_ALL)
	{
		/*We have passed all the tests*/
		selftest_passed = 0;
		failed_once = 0;
	}
	else 
	{
		/* At least one test was not executed - chk for failure */
		/* tests forced and flags reset from failed_selftest */
		failed_selftest();
	}
	/* Reschedule the next iteration */
	timeout (schedule_selftest, (caddr_t)0, selftest_period*hz);
}

/*
 * schedule the first selftest. This function is called by the cluster()
 * system call and by ws_cluster().
 */
schedule_selftest()
{
/* 
** If selftest_period is zero, then we don't want to run it at all,
** so just return.
*/
	if (selftest_period==0) return;
/*
** With the changes to accomodate the S300 Convergence LAN driver,
** the call to selftest must be sw_triggered down to prevent
** problems with the dux_mbuf allocation/deallocation code.
*/
#ifdef __hp9000s300
	sw_trigger(&selftest_intloc, selftest, 0,
		LAN_PROC_LEVEL, LAN_PROC_SUBLEVEL + 1 );
#else
	selftest();
#endif
}


dux_force_selftest()
{
#ifdef __hp9000s300
	sw_trigger(&forceselftest_intloc, force_selftest, 0,
		LAN_PROC_LEVEL, LAN_PROC_SUBLEVEL + 1 );
#else
	force_selftest();
#endif /* hp9000s300 */
}


/*
 *Force the steps of the selftest.  The steps are as follows:
 *
 *	If we haven't already gotten a network buffer, get one and release it.
 *	If this completes all the tests we need, we can quit right here.
 *
 *	Allocate a dm_message (regardless of whether that test has already
 *	been passed.
 *
 *	If either of the above tests fail (remember that this is under
 *	interrupt, so they can return NULL) schedule another forced test
 *	in a second and give up.
 *
 *	Give the dm_message to an NSP.  This NSP will be told to call
 *	nsp_selftest, which will test the disc buffer allocation.
 */
force_selftest()
{
	extern nsp_selftest();		/* forward */
	dm_message mp;

	/*First test the network buffers if necessary*/

/*	The 800 implementation of syncgeteblk cannot be called under
*	interrupt. Let's test this from an nsp.
*/
#ifdef _WSIO
	if (!(selftest_passed&ST_NETBUF))
	{
		register struct buf *bp;

		bp = syncgeteblk(MAXDUXBSIZE);

		/*
		** syncgeteblk can return either 0 -or- -1 as an error
		** depending upon whether it couldn't get a buf or a 
		** buf header.
		*/
		if ((bp == NULL) || (bp == ((struct buf *)-1) ))
		{
			/*we failed, reschedule*/
			timeout(dux_force_selftest,(caddr_t)0, RETRYSELFTEST_PERIOD*hz);
			return;
		}
		else
		{
			/*we succeeded.  Release the buffer*/
			brelse(bp);
		}
	}
#endif _WSIO

	/*If there are no other tests needed, quit.  There are two ways that
	 *this can happen.  One is if the only test we needed to force was
	 *the network buffer test.  In this case, having performed that test
	 *we would have set the appropriate flag and all flags will be set.
	 *The other way would be if the forced test failed the first time,
	 *but before the timeout could take effect all the operations had
	 *been performed.  In either case we can quit right here.
	 */
	if (selftest_passed == ST_ALL) {

		/* Clear the flag that keeps more
		 * force_selftest()s from being started
		 */
		force_selftest_running = 0;
		return;
	}
	/*Now allocate a dm_message.  We do this even if we have passed that
	 *test already because we need it to create an NSP, which we in turn
	 *need to get a disc block.
	 *We choose the size of the message large enough to require an MCLUSTER.
	 */
	mp = dm_alloc(DM_MIN_CLUSTER, DONTWAIT);
	if (mp == NULL)
	{
		/*we failed, reschedule*/
		timeout (dux_force_selftest, (caddr_t)0, RETRYSELFTEST_PERIOD*hz);
		return;
	}
	/* If we haven't allocated an mbuf, we certainly haven't scheduled
	 * an NSP.  So don't bother to test, just give the message to an NSP
	 */

	/* 
	** Scenario:
	**	Limited csp in invoked for something else since last selftest
	** was executed, so ST_NSP bit is set.  However, no disc buffer's been
	** allocated, so we have to  invoke a csp to get one.  However, the
	** last time the lcsp was invoked, it hung and we don't have anymore.
	** Result is that we get a message saying we didn't get a disc buffer,
	** when the real problem was that we couldn't invoke a csp.  Sooo,
	** the kludge work around to none of these selftest flags being
	** incredibly accurate, is to clear the ST_NSP bit before we allocate
	** a csp.  This will still give us the erroneous disc buffer message,
	** but will, at least, also give us the correct csp message
	*/
	selftest_passed &= ~ST_NSP;
	DM_SOURCE(mp) = 0;
	if(  (invoke_nsp (mp, nsp_selftest, 1)) != 0 )
	{
		dm_release(mp,1);
		timeout(dux_force_selftest,(caddr_t)0,RETRYSELFTEST_PERIOD*hz);
	}
	else {

		/* We succeeded!!  Clear the flag that keeps more
		 * force_selftest()s from being started
		 */
		force_selftest_running = 0;
	}
}

/*
 *The portion of the selftest that is executed by the NSP.  Half the test
 *is already done just by getting here.  All that is left is allocating
 *a disc buffer, if needed.
 */
nsp_selftest(mp)
dm_message *mp;
{
	register struct buf *bp;

	dm_release(mp,0);	/*release the request*/

	if (!(selftest_passed&ST_BUFFER))
	{

		bp = geteblk(MAXDUXBSIZE);
		/*Note, this should never return NULL.  If there is a problem,
		 *it will just hang
		 */
		if (bp != NULL)
		{
			brelse(bp);
		}
	}

#ifndef _WSIO
/*
 * Test network buffers here, since cannot do it under interrupt
 */

	if (!(selftest_passed&ST_NETBUF))
	{

		bp = syncgeteblk(MAXDUXBSIZE);
		if ((bp != NULL) && (bp != ((struct buf *)-1) ))
		{
			brelse(bp);
		}
	}
#endif _WSIO
}

failed_selftest()
{
static int last_failure = ST_ALL;  /* For 1st pass last = no failures */
register int failure = ((~last_failure) & (~selftest_passed));

if (failed_once) {
    /* unwarranted paranoia: only print msg if some error occured */
    if (ST_ALL & failure) printf ("Failed kernel selftest.  Failures:\n");
#ifdef _WSIO
    if (failure & ST_NETBUF)
	    printf ("Cannot allocate fs_bufs\n");
#endif
    if (failure & ST_DM_MESSAGE)
	    printf ("Cannot allocate kernel message buffers\n");
    if (failure & ST_NSP)
	    printf ("System process not running\n");
    else if (failure & ST_BUFFER)
	    printf ("Cannot allocate file system buffers\n");
    /* reset flags and try again */
    failed_once = 0;
    last_failure = selftest_passed; /* save which tests failed */
    selftest_passed = 0; 
} else {  /* didn't fail last time or was reset */
    failed_once++;
    last_failure = selftest_passed; /* save which tests failed */
    /*
     * Make sure we don't get multiple dux_force_selftest()s on the
     * timeout queue. Flag is cleared when the selftest succeeds in
     * force_selftest()
     */
    if (!force_selftest_running) {
	force_selftest_running++;
    	dux_force_selftest();	/* schedule force selftests */
    }
}
}
