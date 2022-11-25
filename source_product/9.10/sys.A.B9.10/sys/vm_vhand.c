/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_vhand.c,v $
 * $Revision: 1.15.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:19:55 $
 */
#include "../h/debug.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/vm.h"
#include "../h/vmmeter.h"
#include "../h/vmmac.h"
#include "../h/pfdat.h"
#include "../h/pregion.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/swap.h"
#include "../h/sysinfo.h"
#include "../h/buf.h"
#include "../h/vnode.h"
#include "../h/sema.h"
#include "../h/systm.h"

/*
 * Psuedo-random number generator.  Uses a permutation of the numbers between
 * 0 and NUMRAND-1 (inclusive), and cycles through them.  To get a random 
 * number from the range 0 through 2^n-1 with a flat probability, use
 * RAND(n).  Don't use RAND(n) where n > LOGNUMRAND.
 */
#define NUMRAND 128
#define LOGNUMRAND 7
#define RAND(logsup) (((nextrand >= NUMRAND) ? \
			    nextrand=0, randarray[nextrand++] : \
		            randarray[nextrand++]) >> (LOGNUMRAND - logsup))
/*
 * Data for psuedo-random number generator:  a permutation of the numbers 
 * between * 0 and NUMRAND-1 (inclusive).
 */
int nextrand = 0;
unsigned char randarray[NUMRAND] = 
			 {   117,    6,  103,    3,   40,  118,  107,   61, 
			     124,   23,   88,   74,  115,    8,   13,   68, 
			       2,    7,   34,   41,   37,   95,   89,   27, 
			      51,  109,   30,  127,   71,   17,   42,   80, 
			     100,   12,   56,   39,   35,   16,   36,    5, 
			     106,   64,   43,   19,   82,   48,   14,   52, 
			      73,   81,   18,   21,   31,   46,   69,   66, 
			     125,   57,   70,  120,  112,  126,  102,   65, 
			       1,    4,   87,   32,  110,   86,   75,  122, 
			      79,   55,   50,   96,   63,   85,   92,   99, 
			     105,   83,  104,   28,   49,   53,   29,   90, 
			      93,   24,   60,   97,  121,   20,   91,   78, 
			      77,   33,  108,  111,   76,   54,   47,   25, 
			      44,   15,   84,  123,  119,   45,   94,   98, 
			      58,   67,    0,   62,   38,  101,  113,   59, 
			      26,  116,   72,  114,   10,    9,   11,   22  };

extern int gpgslim;
extern int pfdatnumentries;	/* includes dynamic buffer cache */
extern int freemem, parolemem;
extern preg_t *agehand;
extern preg_t *stealhand;
extern preg_t *bufcache_preg;

vm_sema_t vhandsema;	/* for waking up vhand */

typedef struct vhandargs {	
	int phys_count;	/* used for counting physical pages visited */
	int nice;	/* value to use for preferential aging */
} vhandargs_t;			

/* Expansion factor for nice values.  Should not expand values to bigger than 
 * random number value range.
 */
#define NICEMULT 3
#define MINNICE 0
#define MAXNICE (NICEMULT * (2*NZERO - 1))

/* For treating an array as a circular.  E.g., 5 is between 17 and 8 
 * because 5 is after 17 and before 8.  The test is inclusive.  It is also
 * unambiguous because we know which bound is "low" and which is "high".
 */
#define CIRC_BETWEEN(x,l,h) \
    ((h) >= (l) && (x) >= (l) && (x) <= (h) || \
     (h) < (l) && ((x) >= (l) || (x) <= (h)))

/* These macros exponentially adjust values, both with and without a
 * specific target in mind.
 */
#define INCREASE_VALUE(x) ((x) += ((x)+16) >> 4)
#define DECREASE_VALUE(x) ((x) -= ((x)+15) >> 4)
#define CONVERGE_VALUE(x,t) ((x) = (3*(x) + (t)) >> 2)

/* These are the new (potential) tunables. */
int vhandrunrate = 8;		/* number of times per second vhand should run*/
int minpagecpu = 100;	/* tenths of a percent, cpu to spend paging */
int maxpagecpu = 100;
int memzerorate = 30;	/* tenths of seconds, minimum tolerable time between
				freemem hitting zero events */
int maxqueuetime = 10;	/* tenths of seconds, longest queue to try to create
				for I/O */

static int localstop_index;	/* one past where stealhand should stop within 
					the current pregion */
int handlaps;		/* number of laps between the age hand and steal hand,
			    the agehand laps stealhand when it reaches the 
			    stealhand, but the stealhand laps the agehand when 
			    it passes (leaves) the agehand */
static int targetlaps = 2;	/* goal for hand separation (handlaps), will
					adapt at run time */

int hitzerofreemem = 0;	/* number of times system drove freemem to zero */
int maxpendpageouts;	/* number of pageouts to have pending at a time */

int pageoutcnt = 0;		/* recent count of page-outs finished */
int pageoutrate = 75;		/* emperically updated number of pageouts 
					possible per second */
int stealvisited;		/* number pages visited by steal hand */
int coalescerate = 16;		/* how often to coalesce malloc's free memory */
static int agerate = 100;	/* number of pages to age
					per second, adapts at run time */
static int stealrate = 100;	/* number of pages visited by
					stealhand per second */
static int boot_ticks;	/* ticks since boot until this call of vhand */
static int numticks;	/* number of ticks taken by last call of vhand */

int vhandinfoticks = 0;

void update_gpgslim();
void update_pageoutrate();
void update_targetlaps();
void update_agerate();
void bump_stealhand();
void write_vhand_info();

/*	
 * Vhand (the pageout deamon) is awakened by schedpaging every so often
 * to maintain recently referenced pages and to move pages out when memory 
 * is tight.  
 *
 * It is a two-handed clock algorithm implemented at the pregion level.  
 * This is not a natural way to treat pages fairly or keep track of 
 * where the hands are and how far apart they are etc.  But we do it  
 * anyway because it's useful to have the higher level data structures around,
 * and the kernel is already structured around this approach.
 *
 * The basic algorithm is to use two hands, an age hand followed by a steal
 * hand.  The former clears reference bits on in-core pages, the latter
 * steals those pages which still have their reference bits clear (implying
 * the page has not been used since the age hand aged it).  To do this
 * fairly at the pregion level, the hands will go from one pregion to the
 * next, looking at a fixed fraction (e.g. 1/16) of each pregion before
 * going to the next.  Thus it takes 16 laps of the pregion active list
 * to visit each page once.  Both hands may be visiting the same region at 
 * a given time, but unless handlaps == 0 they will really be at different
 * places in the logical "unraveled" page clock.  The steal hand will
 * never pass the agehand and the agehand will never pass the stealhand 
 * (in the logical clock).  This is equivalent to specifying 0 <= handlaps < 16.
 *
 * The hands visit 1/16 of the _virtual_ space of a pregion at a time.  This
 * is really only perfectly fair if physical pages are evenly distributed
 * across virtual pages.  I.e. there is room for improvement here. 
 *
 * Many vhand parameters (listed above) adapt at run time due to the wide
 * range of memory sizes, cpu speeds, and workloads this algorithms is
 * expected to work on.
 */
void
vhand()
{
    int pagestoage;
    int pagestosteal;
    int coalescecnt = 0;

    gpgslim = (lotsfree + 3*desfree) / 4;
    boot_ticks = ticks_since_boot;
loop:
    numticks = ticks_since_boot - boot_ticks;
    sleep(&vhandsema, PSWP);
    boot_ticks = ticks_since_boot;

    if (coalescerate && ++coalescecnt >= coalescerate) {
	kfree_unused();
	coalescecnt = 0;
	if (freemem >= lotsfree)
	    goto loop;
    }

    update_gpgslim();
    update_pageoutrate();
    update_targetlaps();
    update_agerate();
    if (vhandinfoticks)
	write_vhand_info();

    /* Stealing rate is based on how much stealing we need to do 
     * (amount of free memory etc.) and on how much stealing we are
     * capable of doing (device throughput etc.).
     */
    pagestosteal = gpgslim - (freemem + parolemem); 
    maxpendpageouts = pageoutrate * maxqueuetime / 10;
    if (maxpendpageouts < 2)
        maxpendpageouts = 2;

    vmemp_lock();

    /* Do stealing */
    stealvisited = 0; /* incremented by individual pageout functions */
    while (pagestosteal > 0) {
	if (! advance_stealhand())
	    break;
	if (stealhand == bufcache_preg) {
	    pagestosteal -= stealbuffers(pagestosteal, handlaps, 
					    &bufcache_preg->p_stealscan);
	} else {
	    pagestosteal -= stealpages();
	    regrele(stealhand->p_reg);
	}
	if (parolemem >= maxpendpageouts || freemem >= lotsfree)
	    break;
    }

    /* Don't smooth the steal rate (with CONVERGE etc.), because its main
     * use is for setting the agerate, which is already being smoothed.
     * (Nested smooths adapt too slowly.)
     */
    stealrate = vhandrunrate * stealvisited;

    pagestoage = agerate/vhandrunrate;

    /* Do aging */
    while (pagestoage > 0) {
	if (! advance_agehand())
	    break;
	if (agehand == bufcache_preg) {
	    pagestoage -= agebuffers();
	    bufcache_preg->p_ageremain = 0;
	} else {
	    pagestoage -= agepages(pagestoage);
	    regrele(agehand->p_reg);
	}
    }

    vmemp_unlock();
    goto loop;
}

/* Update paging-out threshold based on how often we've been running out of
 * memory.  Currently, this threshold may move between desfree and lotsfree.
 * If we are not running out of memory very often, then whatever is running
 * should have access to almost all physical memory.  If we are running
 * out of memory frequently, raise the threshold so that more free memory
 * is available when it is needed.
 */
void 
update_gpgslim()
{
    static int memcheckbootticks = 0;	/* when we started counting events */
    int targethits;			/* number of hits allowed */

    /* Don't update untill we have enough data. */
    if (10*(boot_ticks - memcheckbootticks) > 2*HZ*memzerorate ||
						    hitzerofreemem > 2) {
	targethits = 10*(boot_ticks - memcheckbootticks)/HZ/memzerorate;
	if (hitzerofreemem < targethits) {
	    if (gpgslim > desfree)
		CONVERGE_VALUE(gpgslim, desfree);
	}
	else if (hitzerofreemem > targethits) {
	    if (gpgslim < lotsfree)
		CONVERGE_VALUE(gpgslim, lotsfree);
	}
	hitzerofreemem = 0;
	memcheckbootticks = boot_ticks;
    }
}

/* Update pageoutrate based on recent paging data.
 */
void 
update_pageoutrate()
{
    static int ioboot_ticks = 0;	/* when we started counting */
    int recentpor;	/* recent page out rate; since last update */

    /* Wait until we have enough data. */
    if (boot_ticks >= ioboot_ticks+2 && pageoutcnt >= 2) {
	recentpor = HZ * pageoutcnt / (boot_ticks - ioboot_ticks);
	if (parolemem > 0) {
	    /* then recentpor is a good estimate of the i/o rate */
	    CONVERGE_VALUE(pageoutrate, recentpor);
	} else {
	    if (recentpor > pageoutrate) {
		/* here recentpor is a lower bound, we definitely could have 
		 * done more, so only increase pageoutrate 
		 */
		CONVERGE_VALUE(pageoutrate, recentpor);
	    } else {
		/* We may be able to do more, but we're not sure, and we
		 * won't know unless we try, so increment just a little.
		 */
		++pageoutrate;
	    }
	}
	pageoutcnt = 0;
	ioboot_ticks = ticks_since_boot;
    }
}

/*
 * Update the number of laps we want between the two hands.  The more laps,
 * the better the LRU approximation, and the more CPU time is taken because
 * the hands have to go faster to keep the same steal rate.
 */
void 
update_targetlaps()
{
    static int tickstart = 0;	/* when we started counting */
    static int runs = 0;	/* number of times vhand has run */
    static int lastticks = 0;	/* previous number of ticks charged to vhand */
    int ticks;			/* current number of ticks charged to vhand */
    int recentticks;		/* number of ticks recently charged to vhand */
    int cpu;			/* vhand's cpu in tenths of a percent */

    ++runs;
    ticks = u.u_procp->p_cptickstotal + u.u_procp->p_cpticks;
    recentticks = ticks - lastticks;
    /* but don't consider modifying targetlaps until we have enough data */ 
    if (boot_ticks > tickstart && (recentticks > 1 || runs > vhandrunrate)) {
	if (runs < vhandrunrate * (boot_ticks - tickstart)/HZ) {
	    /* calculate a cpu percentage extrapolated up from the number 
	     * of times we actually ran to the number of times we
	     * expect to run based on the hand rate:
	     *   100 -> for percent
	     *   recentticks/runs -> actual ticks per actual run
	     *   vhandrunrate/HZ -> total cpu time between expected runs
	     */
	    cpu = 1000*recentticks*vhandrunrate/runs/HZ;
	} else {
	    /* just calculate straight percent cpu */
	    cpu = 1000*recentticks/(boot_ticks - tickstart);
	}

	if (cpu < minpagecpu) {
	    INCREASE_VALUE(targetlaps);
	    if (targetlaps >= AGEFRACTION)
		targetlaps = AGEFRACTION - 1;
	} else if (cpu > maxpagecpu)
	    DECREASE_VALUE(targetlaps);
	lastticks = ticks;
	tickstart = boot_ticks;
	runs = 0;
    }
}

/* Update the aging rate to compensate for a difference between
 * current hand separation (handlaps) and desired hand separation
 * (targetlaps), or just for changes in the steal rate.
 */
void 
update_agerate()
{
    int targetrate;	/* how fast it looks like we should go */

    if (handlaps != targetlaps) {
	targetrate = stealrate + (targetlaps - handlaps) * 
			((pfdatnumentries - freemem) >> LOGAGEFRACTION) / 4;
	if (targetrate < 0)
	    targetrate = 0;
    } else
	targetrate = stealrate;

    CONVERGE_VALUE(agerate, targetrate);
}

/*
 * This is mostly for debug, but can be useful in the field for figuring
 * out paging troubles.  As long as console is available it is better than
 * vmstat in terms of working while the system is thrashing.  (vmstat tends
 * to bog down and work poorly just like any other program)
 */
void
write_vhand_info()
{
    static int lastboot_ticks = 0;
    static int numrows = 0;
    extern int ave_num_fault_procs;

    if (vhandinfoticks < boot_ticks - lastboot_ticks) {
	if (numrows++ % 8 == 0)
	    printf("handlaps targetlaps agerate gpgslim faultprocs parolemem stealrate freemem\n");
	printf("%8d %10d %7d %7d %10d %9d %9d %7d\n", handlaps, targetlaps, 
		agerate, gpgslim, ave_num_fault_procs, parolemem, stealrate, 
		freemem);
	lastboot_ticks = boot_ticks;
    }
}

/*
 * Update stealhand to point to the next region to take pages from.  Note 
 * that stealhand doubles as the pointer to the active list, which is how 
 * it is used in other files.
 *
 * Return 0 if the stealhand is too close to the agehand, meaning no
 * pages should be stolen at this time.  Otherwise return 1.
 */
int
advance_stealhand()
{
    register reg_t *rp;
    preg_t *endloop;

restart:
    plstlock();

    endloop = stealhand;
    do {
	/* If we're on the buffer cache, and we've finished it, skip it,
		otherwise, return it */
	if (stealhand == bufcache_preg) {
	    if (bufcache_preg->p_stealscan == 0)	/* finished with bc */
		goto skip_preg;
	    else {	/* continue stealing from bc */
		plstunlock();
		return 1;
	    }
	}

	/* If region cannot be locked, skip it.  */
	rp = stealhand->p_reg;
	if (!creglock(rp))
	    goto skip_preg;

	/* If region's vfd/dbd's are swapped, skip it.  */
	if (rp->r_dbd)
	    goto rel_and_skip_preg;

	/* If region has no valid pages, consider swapping it, and in any
	 * case, skip it. 
	 */ 
	if (rp->r_nvalid == 0) {
	    if (rp->r_incore == 0 && countvfd(rp)) {
		/* Then this region has no pages but has vfds, and all 
		 * processes using it are swapped, so its vfds can be 
		 * swapped.  The swap out may fail, so make sure we 
		 * make forward progress through the regions independent of 
		 * the effect of vfdswapo().
		 */
		if (handlaps == 0 && stealhand == agehand) {
		    /* wait for agehand to gives us room before even trying */
		    INCREASE_VALUE(agerate);
		    regrele(rp);
		    plstunlock();
		    return 0;
		}
		bump_stealhand();
		plstunlock();
		vfdswapo(rp);
		if (rp->r_dbd == 0) { /* rare, but */
		    regrele(rp);
		    return 0;	     /* without this, we could infinite loop */
		} 
		regrele(rp);
		goto restart;
	    } else
		goto rel_and_skip_preg;
	}

	/* If region is memory locked, skip it.  */
	if (rp->r_mlockcnt != 0)
	    goto rel_and_skip_preg;

	if (stealhand->p_agescan > stealhand->p_count) {
	    /* The pregion has shrunk, and we don't want to try to catch up to 
	     * an agehand that's off in no-where land.
	     */
	    stealhand->p_agescan = stealhand->p_count;
	}

	if (handlaps == 0 && stealhand == agehand &&
	    CIRC_BETWEEN(stealhand->p_stealscan, stealhand->p_agescan-1,
							stealhand->p_agescan)) {
	    /* steal hand too close to age hand */
	    INCREASE_VALUE(agerate);
	    regrele(rp);
	    plstunlock();
	    return 0;
	}
	if (rp->r_incore) {
	    localstop_index = stealhand->p_agescan - 
			    handlaps * (stealhand->p_count >> LOGAGEFRACTION);
	    if (localstop_index == stealhand->p_agescan)
		--localstop_index;
	} else
	    localstop_index = stealhand->p_agescan - 1;
	if (localstop_index <= 0)
	    localstop_index += stealhand->p_count;
	if (CIRC_BETWEEN(stealhand->p_stealscan, localstop_index, 
						    stealhand->p_agescan))
	    goto rel_and_skip_preg;

	VASSERT(stealhand->p_flags & PF_ACTIVE);
	plstunlock();
	return 1;

rel_and_skip_preg:
	regrele(rp);
skip_preg:
	if (handlaps == 0 && stealhand == agehand) {
	    /* steal hand too close to age hand */
	    INCREASE_VALUE(agerate);
	    plstunlock();
	    return 0;
	}
	bump_stealhand();
    } while (stealhand != endloop);

    plstunlock();
    return 0;
}

int nicepaging = 1;	/* 0 to turn off probabilistic aging
			   1 if using adaptive probabilistic aging 
			   2 if using fixed probabilistic aging */
int nicepagelog = 0;	/* log of width of probability distribution */
int nicepageshift = 0;	/* left edge of probability distribution */
int nicepageignore = 2;	/* number of lotsfree pages of low priority to ignore */

static int nicedist[MAXNICE + 1];	/* number of pages at each nice level */

/*
 * Move the stealhand pointer one pregion forward, and do all activities
 * related to tracking and updating system for probabilistic aging.
 */
void 
bump_stealhand()
{
    int n;	/* nice value of pregion */
    static int maxnice = NICEMULT*NZERO;
    static int minnice = NICEMULT*NZERO;
    static int lastmaxnice = NICEMULT*NZERO;
    static int lastminnice = NICEMULT*NZERO;

    if (stealhand == agehand)
	--handlaps;
    stealhand = stealhand->p_forw;

    if (stealhand == bufcache_preg) {
	bufcache_preg->p_stealscan = 1;	/* we just got here, assume work to do*/

	cnt.v_rev++;	/* hands have gone around pregions once */

	/* Back off on maxnice so that it does not include nice levels that
	 * have only a few pages in them.  (It only makes sense to do
	 * probabilistic aging if there are a significant number of pages
	 * at the maxnice end of the spectrum).
	 */
	if (minnice <= maxnice) {
	    int i, sum = 0;
	    int newmax;
	    int threshold = nicepageignore * lotsfree;

	    for (newmax = maxnice; newmax > minnice; --newmax) {
		sum += nicedist[newmax];
		if (sum > threshold)
		    break;
	    }

	    for (i = minnice; i <= maxnice; ++i)
		nicedist[i] = 0;

	    maxnice = newmax;
	}
	    
	/* Update probabilistic aging (even when nicepaging is off,
	 * so that when it is turned on, sane values exist).
	 */
	if (nicepaging < 2 && minnice <= maxnice && 
		(minnice != lastminnice || maxnice != lastmaxnice)) {
	    int t;

	    nicepagelog = 0;
	    t = maxnice - minnice;
	    while (t > 1) {
		++nicepagelog;
		t >>= 1;
	    }
	    if (nicepagelog > LOGNUMRAND)
		nicepagelog = LOGNUMRAND;
	    nicepageshift = minnice - 1;
		
	    lastminnice = minnice;
	    lastmaxnice = maxnice;
	}
	minnice = MAXNICE;
	maxnice = MINNICE;
    } else if (stealhand->p_reg->r_mlockcnt == 0) {
	/* Update nice limits. */
	n = stealhand->p_trend_diff;	/* see update_active_nice() */
	if (n >= 0 && n <= MAXNICE) {
	    if (n < minnice)
		minnice = n;
	    if (n > maxnice)
		maxnice = n;
	    nicedist[n] += stealhand->p_reg->r_nvalid;
	}
    }
}

/*
 * Steal pages from the current stealhand position between p_stealscan
 * and localstop index.  As usual, the pregion is treated as circular,
 * but VOP_PAGEOUT is not, so if the range wraps around the end back
 * to the beginning, two calls to VOP_PAGEOUT need to be made.  Also,
 * localstop_index is treated like other index pointers; it points one
 * past the index actual work is done on.
 */
int
stealpages()
{
    int save_nvalid;

    save_nvalid = stealhand->p_reg->r_nvalid;
    if (stealhand->p_stealscan >= stealhand->p_count)
	stealhand->p_stealscan = 0;
    if (localstop_index > stealhand->p_stealscan) {
	VOP_PAGEOUT(stealhand->p_reg->r_bstore, stealhand, 
		    stealhand->p_stealscan, localstop_index-1,
		    PAGEOUT_VHAND|PAGEOUT_FREE);
    } else if (localstop_index < stealhand->p_stealscan) {
	VOP_PAGEOUT(stealhand->p_reg->r_bstore, stealhand, 
		    stealhand->p_stealscan, stealhand->p_count - 1,
		    PAGEOUT_VHAND|PAGEOUT_FREE);
	if (stealhand->p_stealscan >= stealhand->p_count) {
	    stealhand->p_stealscan = 0;
	    VOP_PAGEOUT(stealhand->p_reg->r_bstore, stealhand, 
			stealhand->p_stealscan, localstop_index-1, 
			PAGEOUT_VHAND|PAGEOUT_FREE);
	} /* otherwise we didn't even finish the first part */
    }

    return save_nvalid - stealhand->p_reg->r_nvalid;
}

/*
 * Update agehand to point to the next region to age.  This may leave agehand
 * unchanged if there is still some work remaining on the current agehand
 * region.  On return, the agehand pregion will have the number of pages to age 
 * stored in p_ageremain, which will be at least 1.
 */
int 
advance_agehand()
{
    register reg_t *rp;
    preg_t *endloop;
    int newregion = 0;
    int agesize;

    plstlock();

    if (agehand->p_ageremain == 0) {
	/* Done with this region, lets go to the next one... */
	if (! bump_agehand())
	    goto return0;
	newregion = 1;
    }

    endloop = agehand;
    do {
	if (agehand == bufcache_preg) {
	    plstunlock();
	    return 1;
	}
	    
	rp = agehand->p_reg;
	if (rp->r_mlockcnt == 0) {	/* if region is not memory locked */
	    if (creglock(rp)) {		/* if we got a lock */
		if (rp->r_dbd == 0) {	/* if region is not swapped out */
		    if (rp->r_nvalid) {	/* if region has valid pages */
			if (rp->r_incore)
			    agesize = agehand->p_count >> LOGAGEFRACTION;
			else
			    agesize = agehand->p_count;
			if (newregion) {
			    if (rp->r_incore && RAND(LOGAGEFRACTION) < 
					(agehand->p_count & AGEFRACTIONMASK))
				++agesize;
			    agehand->p_ageremain = agesize;
			} else if (agehand->p_ageremain > agesize) {
			    /* compensate for any shrinkage */
			    agehand->p_ageremain = agesize;
			}
			if (agehand->p_ageremain > 0) {
			    plstunlock();
			    return 1;
			}
		    }
		}
		regrele(rp);
	    }
	}
	agehand->p_ageremain = 0;
	if (! bump_agehand())
	    goto return0;
	newregion = 1;
    } while (agehand != endloop);

return0:
    plstunlock();
    return 0;
}

int 
bump_agehand()
{
    agehand = agehand->p_forw;
    if (agehand == stealhand) {
	++handlaps;
	if (handlaps >= AGEFRACTION) { /* ran into stealhand */
	    agehand = agehand->p_back; /* WARNING:  THIS IS REWORK! */
	    --handlaps;

	    return 0;
	}
    }
    if (agehand == bufcache_preg)
	bufcache_preg->p_ageremain = 1;

    return 1;
}

/* CLUSTERMASK is used to help prevent probabistic aging from
 * aging a region so chaotically that the attempts made at pageout
 * time to do sequential I/O fail.  Basically it is used to treat
 * groups of pages the same way, so probabilistic aging takes place
 * at a coarser granularity than the page level.
 */
#define CLUSTERMASK 0x10

/*
 * See if the given vfd has been referenced recently, and if so
 * conditionally clear the reference bit.  The reference bit is
 * not always cleared for regions in a process with a better nice value
 * than other processes.  This is helps keep better priority processes
 * in memory.
 *
 * Also counts the number of pages visited, this is used as a count 
 * of valid pages.
 *
 * Return 0 = keep going.
 * Return 1 = scanned enough.
 */
 
/*ARGSUSED*/
int
vhand_vfdcheck(rindex, vfd, dbd, args)
    int rindex;
    vfd_t *vfd;
    dbd_t *dbd;
    vhandargs_t *args;
{
    static int agecluster;
    static int previndex;
    static preg_t *prevpreg = NULL;

    /*
     * If the page is locked, don't go near it.
     */
    if ((!vfd->pgm.pg_lock) && cpfnlock(vfd->pgm.pg_pfn)) {
	if (hdl_getbits((int)vfd->pgm.pg_pfn) & VPG_REF) {
	    if (nicepaging) {
		if ((previndex & CLUSTERMASK) != (rindex & CLUSTERMASK) ||
			prevpreg != agehand) {
		    agecluster = args->nice-nicepageshift > RAND(nicepagelog);
		    prevpreg = agehand;
		} 
		previndex = rindex;
	    } else
		agecluster = 1;
	    if (agecluster)
		hdl_unsetbits((int)vfd->pgm.pg_pfn, VPG_REF);
	}
	pfnunlock(vfd->pgm.pg_pfn);
    }
    ++args->phys_count;

    return(0);
}

/*	
 * Agepages clears reference bits on pages in the pregion currently pointed
 * to by agehand.  p_agescan is the current place to start clearing.  It
 * will not clear more than numtoage pages, nor will it clear more than
 * p_ageremain pages.  It will also stop before crossing the p_stealscan.
 * It will however wrap around the end of the pregion back to the beginning
 * if necessary.  p_ageremain is decremented by the number of pages cleared.
 * Agepages returns the number of virtual pages scannned.
 */
int
agepages(numtoage)
    int numtoage;		/* physical number of pages to age */
{
    extern void foreach_valid();
    int numtoscan;	/* virtual pages */
    int numaged;	/* physical pages */
    vhandargs_t args;
    reg_t *rp;

    rp = agehand->p_reg;

    VASSERT(vm_valusema(&rp->r_lock) <= 0);

    args.nice = agehand->p_trend_diff;	/* see update_active_nice() */
    if (args.nice == -1)	/* put real-time priority at slowest aging */
	args.nice = nicepageshift + 1;
    numaged = 0;
    do {
	if (agehand->p_agescan >= agehand->p_count)
	    agehand->p_agescan = 0;

	/* if we're up against the steal hand, quit aging this region */
	if (agehand->p_agescan+1 == agehand->p_stealscan || 
			agehand->p_agescan+1 >= agehand->p_count && 
			agehand->p_stealscan == 0) {
	    agehand->p_ageremain = 0;
	    break;
	}

	numtoscan = MIN(numtoage, agehand->p_ageremain);
	/* except we can't scan past the pregion's end...  */
	if (numtoscan > (agehand->p_count - agehand->p_agescan))
	    numtoscan = agehand->p_count - agehand->p_agescan;

	/* and make sure we stay behind the steal hand */
	if (agehand->p_agescan < agehand->p_stealscan && 
			agehand->p_stealscan <= agehand->p_agescan + numtoscan)
	    numtoscan = agehand->p_stealscan - agehand->p_agescan - 1;
	if (agehand->p_agescan + numtoscan == agehand->p_count && 
		agehand->p_stealscan == 0)
	    --numtoscan;

	args.phys_count = 0;
	foreach_valid(rp, agehand->p_off + agehand->p_agescan, 
				(int)numtoscan, vhand_vfdcheck, &args);

	agehand->p_agescan += numtoscan;
	numtoage -= args.phys_count;
	numaged += args.phys_count;
	agehand->p_ageremain -= numtoscan;
    } while (numtoage > 0 && agehand->p_ageremain > 0);

    cnt.v_scan += numaged;

    return numaged;
}

/* adb'able tunable for nice bias against mmf data only pregions */
int mmfnicebias = 0;

/*
 * Return the priority associated with a pregion.  Eventually this data
 * should be used so that the ultimate priority assigned a region is a
 * function of the smallest nice value of all the processes using this
 * pregion.  Hence, pseudo pregions return a large priority value so
 * that if no real process is using a region, then we are biased
 * against it.  We also incorporate a bias for MMF's that are used only
 * as data.
 */
int 
get_pregionnice(prp)
    preg_t *prp;
{
    struct proc *p;

    if (prp->p_vas && (p = prp->p_vas->va_proc)) {
	if (p->p_flag & SRTPROC)
	    return -1;	/* as a flag, not a priority */
	else {
	    int n = NICEMULT * p->p_nice;

	    if (prp->p_type == PT_MMAP && !(prp->p_flags & PF_VTEXT)) {
		/* Bias MMF's being used to read/write data */
		n += NICEMULT * mmfnicebias;
		if (n > MAXNICE)
		    n = MAXNICE;
		else if (n < MINNICE)
		    n = MINNICE;
	    }
	    return n;
	}
    }
    else
	return MAXNICE;
}
