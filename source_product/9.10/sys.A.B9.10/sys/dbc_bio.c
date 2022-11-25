/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/dbc_bio.c,v $
 * $Revision: 1.2.83.9 $      $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 94/12/06 11:09:04 $
 */

/* 
   This code estimates the number pages that should be removed from
   the buffer cache.  It is done in a manner analogous to the "aging" and 
   "stealing" of regions.  Regions are "aged" by clearing reference bits 
   for a subset of the pages in the region.  Some time is allowed to go by, 
   then the pages are stolen if they remain unreferenced. 

   The buffer cache is aged by associating a timestamp and a counter with
   the beginning of an age period.  The counter is incremented for each unique
   page referenced during that age interval.  Each age inverval is associated
   with a subset of the pages in the buffer cache.  When it comes time to
   steal pages, the counter is used to estimate the number of pages that have
   been referenced in that subset of pages.  This estimate assumes a uniform 
   reference pattern on the pages in the buffer cache.

   Each age interval represents a fraction of the buffer cache, and is the 
   same fraction of a region that is aged for each lap of the age hand. Two
   algorithms are implemented in this code.  One algorithm maintains a separate
   counter for each age interval and is turned on by defining DBC_ONEBUCKET.  
   The other algorithm keeps one counter for all intervals, and estimates the 
   number of counts in the interval under the steal hand.

   Do we really need to be concerned about wraparound on timestamps?  If the 
   buffer cache is aged 20 times a second, it would take more than 2400 days 
   before the time stamp wraps around.  Since this is such a rare event, we 
   won't spend a any effort minimizing the burp that will occur at that time.

   If the customer has specified a target size for the buffer cache, we 
   could turn off the aging,  (and as a side effect, the referencing).  The
   dbc_steal() routine could always steal 1/16 as long as the size of the 
   buffer cache did not drop below what the customer specified.  HP REVISIT

   It is not clear when buffers should be considered referenced.  Should 
   IO that is initiated to sync dirty buffers be considered a reference? 
   There also exists the possibility of under counting the references due
   to the existence of B_BUSY buffers at the time dbc_steal() is called. If
   the reference counts are incremented at getblk() time, then any aging 
   after the getblk() and before the brelse() won't include the reference.
   if the reference counts are incremented at brelse() time, then B_BUSY
   buffers won't have yet been counted when dbc_steal() is called.  We
   will consider B_BUSY buffers to be the equivalent of locked pages, and
   skip over a portion of "them" instead of stealing "them". HP REVISIT 

   Dependencies.

     1)	Rate at which the buffer cache is aged, will affect rate at which
	timestamp overflows and burp occurs.

     2) An array of reference counts are allocated on the stack in dbc_age.
        If AGEFRACTION gets large, then may want to MALLOC tempory memory 
        instead of using stack.  This is only an issue if DBC_ONEBUCKET is
	not defined.
*/

#include "../h/types.h"
#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/debug.h"
#include "../h/malloc.h"
#include "../h/vmsystm.h"


#define POINT9GB_PGS (((1024 * 1024 * 1024) / NBPG) * 0.9)

#define DBC_ONEBUCKET		/* We use one bucket to collect the reference
				 * counts for all acitve age intervals.
				*/

#define MINNBUF      32
#define MINBUFPAGES  64		/* If the customer has not specified a minimum
                                 * buffer cache size, we use this as the 
                                 * minimum.  Choose wisely to prevent deadlock.
			        */

extern int parolemem, stealvisited, maxpendpageouts, freemem, lotsfree;
extern int noswap;


/* The difference between the agetime and the stealtime is the number of 
 * intervals the age hand has moved in front of the steal hand.
*/
char fixed_size_cache;
static unsigned int agetime;
static unsigned int stealtime;
static int dbc_unreferenced = 0;	/* number of pages left to be stolen
					 * from the initial age interval.
					*/

#ifdef DBC_ONEBUCKET
static int refcounts;			/* Running counter of page references */
#else					/* One counter for all intervals */
static int refcounts[AGEFRACTION];	/* Separate counter for each age */
#endif /* DBC_ONEBUCKET */		/* interval.			 */

int dbc_parolemem = 0;
unsigned int dbc_stealavg;
struct  buf *dbc_hdr = 0;
int orignbuf;			/* specified by customer */
int origbufpages;		/* specified by customer */
int dbc_ceiling = 0;		/* maximum number of buffer pages */
extern int dbc_min_pct;		/* the 10.0 way of tuning the size of the */
extern int dbc_max_pct;		/* buffer cache - lowest precedence  */
int dbc_nbuf;			/* minimum number of buffer headers */
int dbc_bufpages;		/* minimum number of buffer pages */
u_int dbc_vhandcredit;		/* number of pages not malloc'd in */
				/* anticipation of the next stealing */
int dbc_steal_factor = 48;	/* This can be used to adjust the rate at which
				** pages will be taken from the buffer cache.
				** A value of 16 is neutral. 
				*/ 


#define DBC_CEILING (physmem/2)

/* Right now dbc_ceiling is only set once.  Never reset to a new value.
 * Customer can adb the kernel to set dbc_ceiling to some value other than
 * zero and that that value will stick.
 */

dbc_set_ceiling()
{
  if (dbc_ceiling != DBC_CEILING) 
  	return;
  if (fixed_size_cache) 
	dbc_ceiling = dbc_bufpages;
  else  
  	dbc_ceiling = (physmem * NBPG) / 10 / NBPG;
}


/*
 * Description:
 *   System initialization for dynamic buffer cache.
 */
dbc_init()
{
  /* New buffer headers start with a timestamp of zero, but that is taken 
   * care of in ??? 
   */
  agetime = 0;
  stealtime = 0;
  dbc_stealavg = 0;
  dbc_vhandcredit = 0;

  orignbuf = nbuf;
  origbufpages = bufpages;
  
  /*  
   *	patch 3-30-93 (PHKL_2333) DKM   and again 7-21-93 (PHKL_2892)
   *
   *	This used to be 
   *       if (dbc_ceiling == 0) dbc_ceiling = physmem;
   *    
   *	The problem is that we never really want the buffer cache taking all
   *	of RAM; it does indeed try to do this in stock 9.0, so we are patching
   *	things up in the following way: dbc_ceiling will be set to physmem/2
   *	(not necessarily right, but lots better than physmem!), and a new
   *	variable "dbc_cushion" will be introduced that will be the amount of
   *	free RAM that must exist for the DBC to be allowed to take more. 
   *	We'll default this to 2 MB, but the customer can adb it if needed.
   *
   *	For the next release, should all of this stuff be tunable?
   *
   *	For 9.1 we have the 10.0 scheme (dbc_min_pct, dbc_max_pct) and
   *	dbc_cushion is tunable  --  DKM 12-6-94
   */

  if (dbc_ceiling == 0) 
  	dbc_ceiling = DBC_CEILING;

  fixed_size_cache = (orignbuf != 0) || (origbufpages != 0);
  if (fixed_size_cache) {       /* customer has specified some limits */
        if (nbuf == 0) 
                nbuf = bufpages/2;	
        else if (bufpages == 0) 
#ifdef __hp9000s800		/*  historical difference...  */
                bufpages = nbuf*2;
#else                
                bufpages = nbuf;
#endif

        /* Check lower limits */
        if (bufpages < MINBUFPAGES) 
        	bufpages = MINBUFPAGES;
        if (nbuf < MINNBUF) 
        	nbuf = MINNBUF;

        /* Check upper limits.
         * There is a virtual limit set on the buffer cache of .9 gigabytes.
         * Because of this the value, the number of buffer pages is also
         * limited to .9 gigabytes.  Users configuring their buffer cache
         * greater than .9 gigabytes will be trimmed back.  See the 9.A VM
         * Design document for details.  HP REVISIT
         */
        if (bufpages > POINT9GB_PGS) {
                bufpages = POINT9GB_PGS;
                printf("Warning: Buffer cache physical size greater than .9 gigabyte maximum, trimming back buffer pages to (%d)\n", bufpages);
        }

        if (bufpages > nbuf * (MAXBSIZE / NBPG))
                bufpages = nbuf * (MAXBSIZE / NBPG);
        if (nbuf > bufpages) 
        	nbuf = bufpages;

        dbc_nbuf = nbuf;
        dbc_bufpages = bufpages;
  } else {
  	/*
  	 *  Use the 10.0 scheme, which sets minimum and maximum percentages
  	 *  of RAM to use.  The minimum defaults to 5 and the maximum to 50,
  	 *  but these are tunable parameters so the user can specify most
  	 *  anything.  First sanity-check these values, and then use them...
  	 */
  	if (dbc_min_pct < 2)
  		dbc_min_pct = 2;
  	if (dbc_max_pct < 2)
  		dbc_max_pct = 2;
	if (dbc_min_pct > 90)  		
  		dbc_min_pct = 90;
	if (dbc_max_pct > 90)  		
  		dbc_max_pct = 90;
	dbc_bufpages = dbc_min_pct*physmem/100;
	if (dbc_bufpages < MINBUFPAGES)
	        dbc_bufpages = MINBUFPAGES;
        dbc_nbuf = dbc_bufpages/2;
        if (dbc_ceiling == DBC_CEILING)
        	dbc_ceiling = dbc_max_pct*physmem/100;
  }
  if (noswap || fixed_size_cache) 
  	dbc_set_ceiling();
}


/* Description
 *   dbc_alloc() creates new buffer headers.  It allocates a page of memory and
 *   carves it up into an integral number of buffer headers.  We don't wait
 *   for memory to be allocated so it is possible that this routine will fail.
 *
 * Return Values:
 *	count is the number of buffer headers.
 *	function return value is a pointer to a linked list of buffer headers.
 *      A NULL value is returned if we were unable to allocate any headers.
*/
struct buf *dbc_alloc (count)
int *count;
{
        register struct buf *bp, *last;
        register int n;

        *count = n = NBPG/(sizeof (struct buf));
        VASSERT(n > 0);

        /* Note that S300 requires that buffer headers are initialized to
         * zero when allocated.  S700 does not seem to have the same
         * requirement.
        */
        bp = (struct buf *) kalloc (1, KALLOC_NOWAIT);
        if (bp == NULL) {
		*count = 0;
	        return (NULL);
	}
	last=NULL;
        while (n-- > 0) {
		bp->b_dev = NODEV;	
                bp->b_bcount = 0;	/* neccessary??? */
                bp->b_nexthdr = last;
		last = bp;
		bp++;
        }
	
	/* count has already been set */
        return (last);
}


/*
 * Description:
 *   Create another age interval for the buffer cache.  This initiates 
 *   a new counter for the number of unique pages referenced 
 *   since this age point.  Most of the code is for handling the rare 
 *   event of a overflow of the agetime.
 *
 * Return value:
 *	The number of pages actually aged.  This is always the same fraction
 *      of the total pages in the buffer cache.
*/
int
agebuffers ()
{
  int diff;			/* intervals between the age and steal hands */

  diff = agetime-stealtime; 
  /* VASSERT(diff <= AGEFRACTION);  */
  if (diff > AGEFRACTION) {	
#ifdef OSDEBUG
	printf ("excessive buffer cache laps\n");  
#endif
	return (0);
  }

  /* Have we wrapped around on the timestamps?  If so, the time is reset.
   * Has not been made MP safe.
  */
  agetime++;
  if (agetime <= 0) {  	
    int i,j;
    struct buf *bp;

    /* temporarily reset our agetime
    */
    agetime = stealtime + diff;

#ifndef DBC_ONEBUCKET
    {
      /* Save the counts for existing age intervals, then move to new
       * locations. 
      */
      int oldrefcounts[AGEFRACTION];

      for (i=0; i<AGEFRACTION; i++)
        oldrefcounts[i] = refcounts[i];
    
      for (i=1, j=stealtime+1; j<=agetime; i++, j++)
        refcounts[i%AGEFRACTION] = oldrefcounts[j%AGEFRACTION];
    }
#endif /* DBC_ONEBUCKET */
	 
    /* Adjust the timestamps in the buffer headers.  Buffers with timestamps
     * less than or equal to stealtime have not been referenced since the
     * buffer cache was aged.
    */
    for (bp = dbc_hdr; bp != NULL; bp=bp->b_nexthdr) {
      VASSERT(bp->b_timestamp <= agetime);
      if (bp->b_timestamp  < stealtime)
           bp->b_timestamp = 0;
      else bp->b_timestamp -= stealtime; 
    }
    stealtime = 0;
    agetime = diff+1;			/* Move to next unused age slot */
  }

#ifdef DBC_ONEBUCKET
  /* only absolutely neccessary at system initialization. */
  if (diff == 0) refcounts = 0;		
#else
  refcounts [agetime % AGEFRACTION] = 0;
#endif /* DBC_ONEBUCKET */

  return bufpages / AGEFRACTION;
}

/*
 * Description:
     The steal routine should not schedule more IO than the devices (?) can
     handle.  This could choke off other useful IO.  When enough IO has been 
     scheduled, the steal routine returns the number of pages that were 
     scheduled.  The steal routine is called later to finish stealing the 
     pages in the current interval.  Need to signal to vhand that either all 
     pages were stolen, or that there are some left.

 * Assumptions:

 * Returns:
	The number of pages actually stolen.  Will set finished to zero if 
	no more pages left to steal in this interval.  May return less 
        than pagestosteal if it comes accross roadblock.  The roadblock may 
        go away if other processes are given the chance to run.  May return 
	slightly more than max if we overshoot by some number of pages in 
	a buffer.

 * Notes:
     1) With ONEBUCKET defined, there is one bucket containing the counts
        for all age intervals.  Some fraction of the counts belong to the
        interval under the age hand.  We know that the count for any given
        interval is greater than or equal to any later interval.  Two 
	possibilities are:
		a) Divide the counts equally amoung all buckets.  Since 
		   later intervals (closer to age hand) have had less time
		   for page references, this will bias the reference counts 
		   to those buckets.  More pages will be released sooner.
		b) Assume that aging occurs at uniform intervals in time 
		   (not neccessarily true), and that the older buckets
		   have a larger share of hits.  If we have N intervals, then
		   assume that the fraction of counts under the steal hand 
		   is
				N  / (sum of 1 to N)
		   or		N / (N * (N + 1) / 2)		
		   or		2 / (N + 1)
		
     2) On any exit from this routine:  If dbc_referenced is zero, then
	*finished should be 1.  This signals vhand to go on to the next
	region.
*/
int 
stealbuffers (pagestosteal, laps, finished)
int pagestosteal;/* Don't need to free up more than this number of pages */
int *finished;   /* set to 0 when finished with this interval */
int laps;	 /* Don't want to get closer to age hand than laps. */
{
  int count = 0;			/* number of pages actually stolen 
					 * during this invocation. 
                                        */
  int pages;
  register struct buf *bp;
  register struct buf *blist;
  register struct bufqhead *endq;
  int q;
  int s;
  int i;

  VASSERT(laps >= 0 && laps < 16);
  VASSERT(pagestosteal >= 0);

    /* Go to next steal interval if we have exhausted the current one. 
    */
    if (dbc_unreferenced <= 0) {		

      /* Don't get too close (or pass) the age hand.
      */
      if ((agetime - stealtime) < (laps + 1)) {
	printf ("inconsistent buffer cache laps or vhand laps\n");
	*finished = 0;
	return (0);
      }
      stealtime++;

#ifdef DBC_ONEBUCKET
      {
	int partialcount;

	/* Attribute some fraction of the counts to the interval we are 
         * Currently stealing from.  See Note number 1 in proceedure heading.
         * Note that stealtime has been incremented above.
         */
        VASSERT(refcounts >= 0);
	partialcount = (2*refcounts)/(agetime - stealtime + 2);
        dbc_unreferenced = (bufpages - partialcount) / AGEFRACTION;
	refcounts -= partialcount;
      }
#else
      {
        /* The fractional part of the division is lost (round down) */ 
        int stealindex;
        stealindex = stealtime % AGEFRACTION;

        VASSERT(refcounts[stealindex] >= 0);
        dbc_unreferenced = (bufpages - refcounts[stealindex]) / AGEFRACTION;
      }
#endif /* DBC_ONEBUCKET */

      /* If the customer has specified a target size for the buffer cache,
      ** we want to quickly shrink down to that target.  Later code ensures
      ** we won't go below the target.
      */
      if (fixed_size_cache) {	
        dbc_unreferenced = (bufpages) / AGEFRACTION;
      }
	
      /* Having this factor lets us "tune" the rate at which pages leave
      ** the buffer cache.   Though it has not been shown by any performance 
      ** tests,  A number of people feel that there should be some bias 
      ** against the buffer cache, in favor of other VM objects.  
      */
      dbc_unreferenced = (dbc_unreferenced * dbc_steal_factor) / 16;

      /* Since bufpages can change over time, don't exceed reasonable limits.  
      */ 
      if (dbc_unreferenced < 0) dbc_unreferenced = 0;

      /* Also don't want to shrink buffer cache too much since too few buffers 
      ** can create deadlock conditions.  
      ** When a buffer is taken, all the pages are taken, so even if 
      ** unreferenced is 1, up to btop(MAXBSIZE) pages may leave the cache.
      ** Note that dbc_parolemem can change under interrupt!!!  It can only
      ** get smaller under interrupt, so that won't hurt us here.
      */
      i = (bufpages - dbc_parolemem) - dbc_bufpages;
      if (dbc_unreferenced > i) dbc_unreferenced = i;

      /* An average of the steal counts is kept and used as a target for 
      ** voluntary page reduction.  See allocbuf().
      */
      dbc_stealavg = (dbc_stealavg + dbc_unreferenced)/2;

      /* Do we have any credit for pages already given up to the VM system.
       * This can happen if we voluntarily give up pages due to memory 
       * pressure.  See allocbuf().
      */
      VASSERT(dbc_vhandcredit >= 0);
      if (dbc_unreferenced < dbc_vhandcredit) {
        dbc_vhandcredit -= dbc_unreferenced;
        dbc_unreferenced = 0;
      } else {
        dbc_unreferenced -= dbc_vhandcredit;
        dbc_vhandcredit = 0;
      }


      if (dbc_unreferenced <= 0) {
	*finished = 0;
	return (0);
      }
    }


    pages = dbc_steal_free_pages (dbc_unreferenced);
    count += pages;
    dbc_unreferenced -= pages;


     
    /* Want to keep this independent of the queue balancing code.  Skip over 
     * empty queues.  We could release the critical region after removing
     * each buffer from the available list. But...  It should not take us 
     * long to assemble the buffers, releasing the critical region to 
     * process each buffer makes the code more complex.
    */
    blist = NULL;
    q = BQ_AGE;
    endq = &bfreelist[q];
    s = CRIT();
    while (   (dbc_unreferenced > 0) 
	   && (parolemem < maxpendpageouts)
	   && (freemem + count <= lotsfree)) {

      bp = endq->av_forw;
      if (bp == (struct buf *)endq) {
        if (q == BQ_AGE)  {                  /* any more queues to process */
          dbc_unreferenced = 0;
        } else {
          q--;
          endq = &bfreelist[q];
        }
      } else {

        /* When we steal pages from a buffer, we steal all the pages.   This 
         * might run us over our target a bit (as much as 15 pages), but that 
         * should not be a problem.  We have compensated above to ensure a 
         * minimum sized buffer cache.
        */
        VASSERT(bp->b_bufsize>0);
        pages = btop(bp->b_bufsize);
        count += pages;
        dbc_unreferenced -= pages;
        /* bufpages is adjusted when pages are actually let go. */

        bp->b_flags |= (B_BUSY | B_PAGEOUT);
        bremfree (bp);
	bp->av_forw = blist;
	blist = bp;
	if (bp->b_flags & B_DELWRI) {
	  parolemem += pages;
	  dbc_parolemem += pages;
	}
      }
    }

    UNCRIT(s);

  stealvisited += count;
  while (blist != NULL) {
    bp = blist;
    blist = blist->av_forw;
    if (bp->b_flags & B_DELWRI) {
      bp->b_flags |= B_ASYNC;  
      bwrite(bp);
    } else {
      struct bufqhead *flist;
      bremhash(bp);				/* remove from hash chain */
      flist = &bfreelist[BQ_AGE];
      binshash(bp, flist);
      brelse (bp);
    }
  }

  if (dbc_unreferenced <= 0) *finished = 0;
  return (count);
}


 
/*
 * Description:
 *   When a buffer is referenced, we may need to update the reference counts 
 *   for the active age intervals.  Since we are counting unique references
 *   we only update the counts for age intervals which were aged after the
 *   last reference to this buffer.
 *
 * Assumptions:
 *   Buffer pointer is not NULL.
 *
 * Notes:
 *   Performance is a key concern here, since buffer references occur more
 *   frequently than aging and stealing.  This concern is the primary 
 *   reason behind the DBC_ONEBUCKET algorithm.  If the age hand and steal
 *   hand stay far apart, several buckets may have to be incremented on 
 *   each use of a buffer.
*/
void dbc_reference (bp)
register struct buf *bp;
{
  register unsigned int time;

  /* Do we have any active age intervals? */
  if (agetime > stealtime) {
    time = bp->b_timestamp;
    if (time < stealtime) time = stealtime;
    bp->b_timestamp = agetime;
#ifdef DBC_ONEBUCKET
    refcounts += (agetime - time) * btop(bp->b_bufsize);
#else
    while ( ++time <= agetime ) 
      refcounts [time % AGEFRACTION]++;   
#endif /* DBC_ONEBUCKET */
  }
}

/* TUNE  How much buffer space are we willing to let diskless consume.
         Only used when local mounts are present.  Note that we are
         also talking about the memory pressure put on by diskless
	 that was not there in 8.0
	 Need at least some minimum number of buffers available to diskless,
	 and this should be factored into dbc_bufpages at initialization time.
	HP REVISIT
*/
dbc_dux_limits_exceeded (cur_dux_req_bufs, 
			 cur_dux_req_bufmem, 
			 max_dux_req_percent)
int cur_dux_req_bufs; 
int cur_dux_req_bufmem;
int max_dux_req_percent;
{
  int limit;
  limit = (bufpages * max_dux_req_percent)/100;
  if (limit < 16) 
	limit = 16;
  if (btop(cur_dux_req_bufmem) > limit)
	 return(1);
  else   return (0);
}
