/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/sitemap.c,v $
 * $Revision: 1.6.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:46:07 $
 */

/* HPUX_ID: @(#)sitemap.c	55.1		88/12/23 */

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
 * This file contains the code for keeping track of resources.  There are two
 * primary structures that are used, the sitemap, and the double count.  The
 * sitemap is used at the serving site and is essentially a list of which sites
 * hold what resources.  The double count can be optionally used at a work-
 * station.  It keeps track of both how many times the server thinks the site
 * is using the resource, and how many times the site is actually using the
 * resource.
 */

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../dux/dm.h"
#include "../dux/sitemap.h"
#include "../h/kern_sem.h"

/*
 * The following functions deal with sitemaps.  A sitemap can logically
 * be considered an array of entries.  Each entry lists a site and the
 * number of times that site makes use of the resource.  Physically, in
 * order to save space, the sitemap is constructed slightly differently.
 * It consists of a count of the number of sites in it, a maptype, and a
 * union.  If the maptype specifies S_MAPEMPTY, there are no sites in the
 * map.  If the maptype specifies S_ONESITE, the map contains one site,
 * with the site number and the number of references stored in the union.
 * If the maptype is S_ARRAY, the union is a pointer to an array of site
 * entries.  If necessary, the last entry of this array is a pointer to
 * another array.
 */

/*
 *Wait for the sitemap to be unlocked.  The sitemap is locked while converting
 *to an array or while allocating new arrays because it is possible to sleep
 *while allocating a buffer and to request another buffer concurrently leading
 *to an inconsistancy.
 */
waitlock(mapp)
register struct sitemap *mapp;
{
	while (mapp->s_maptype == S_LOCKED || mapp->s_maptype == S_WAITING)
	{
		mapp->s_maptype = S_WAITING;
		sleep ((caddr_t)mapp, PRIBIO);
	}
}

/*
 *getnewsitearray is used internally to allocate a new site array.  This is
 *called whenever the number of sites in a sitemap increases to 2, or whenever
 *one of the arrays overflows and it is necessary to form a linked list.
 *Getnewsitearray uses getfixedresources to allocate the array.
 */
static
struct sitearray *
getnewsitearray(mapp)
register struct sitemap *mapp;
{
	register struct sitearray *array;

	/* Lock the sitemap */
	mapp->s_maptype = S_LOCKED;
	/*Allocate the array*/
	array = (struct sitearray *) kmem_alloc (sizeof (struct sitearray));
	/*Zero the array, since there are not yet any sites in it*/
	bzero((caddr_t)array,sizeof (struct sitearray));
	/*unlock the sitemap */
	if (mapp->s_maptype == S_WAITING)
		wakeup ((caddr_t)mapp);
	mapp->s_maptype = S_ARRAY;
	return (array);
}

/*
 *getsiteentry returns a pointer to the siteentry field in the specified map
 *for the specified site.  If there is no entry for that site, it will do
 *one of the following:
 *	If create is 0, it will return NULL.
 *	If create is 1, it will allocate a slot, increment the count of valid
 *	sites, and return a pointer to that slot.  The caller must set the
 *	count on the slot to a non zero value.  Failure to do this will result
 *	in an inconsistancy.
 *
 *NOTE: If a site of zero is passed to this function, it is replaced by my_site.
 */

struct siteentry *
getsiteentry(mapp,site,create)
register struct sitemap *mapp;
register site_t site;
int create;
{
	register struct sitearray *array;
	register struct siteentry *entry;
	register int count;

	if (site == 0)
	{
		site = my_site;
	}
	waitlock (mapp);
	switch (mapp->s_maptype)
	{
	case S_MAPEMPTY:
		/* No entries in the map */
		if (create)
		{
			/* we are adding an entry.  Convert to a onesite type */
			mapp->s_count = 1;
			mapp->s_maptype = S_ONESITE;
			mapp->s_onesite.s_count=0;
			mapp->s_onesite.s_site=site;
			return (&mapp->s_onesite);
		}
		else
		{
			/* We tried to find the entry but there was nothing */
			return (NULL);
		}
		/*NOTREACHED*/

	case S_ONESITE:
		/* There was exactly one site in the map */
		if (mapp->s_onesite.s_site == site ||
			(mapp->s_onesite.s_site == 0 && site == my_site))
		{
			/* It was the site we were looking for */
			return (&mapp->s_onesite);
		}
		else if (create)
		{
			/* There was one site in the map, and we need to add
			 * a second.  We need to convert to an array format.
			 */
			/* Get an array */
			array = getnewsitearray(mapp);
			/* Copy the single entry in the map to the first
			 * entry in the array.  We must do this because
			 * The pointer to the array overlays the map entry
			 * in the union.
			 */
			array->s_entries[0] = mapp->s_onesite;
			/* Set up the second entry with the new site */
			array->s_entries[1].s_site = site;
			/* and set up the rest of the fields */
			mapp->s_count = 2;
			mapp->s_maptype = S_ARRAY;
			mapp->s_array = array;
			return (&(array->s_entries[1]));
		}
		else
		{
			/* The entry wasn't there so return NULL */
			return (NULL);
		}
		/*NOTREACHED*/

	case S_ARRAY:
		/* We have a full fledged array to search; possibly even a
		 * linked list of arrays.*/
		entry=NULL;
		array = mapp->s_array;
		/*first check to see if the site is in the array.  While we
		 *are making this check, keep track of the first empty slot
		 *if any, so that we will know where we can insert a new
		 *entry if we need to.
		 */
		while (array != NULL)
		{
			for (count = 0; count < SITEARRAYSIZE; count++)
			{
				if (array->s_entries[count].s_count == 0)
				{
					if (entry == NULL)
						entry = &array->
							s_entries[count];
				}
				else if (array->s_entries[count].s_site==site ||
					(array->s_entries[count].s_site == 0 &&
					site == my_site))
				{
					/* We found it!!! */
					return (&array->s_entries[count]);
				}
			}
			array = array->s_next;
		}
		/*the site was not in the array (If it was we returned it)*/
		if (create)
		{
			/* We need to enter the site */
			mapp->s_count++;
			if (entry)
			{
				/* If we have prevously found an empty slot,
				 * just fill in the site and return it.
				 */
				entry->s_site = site;
				return(entry);
			}
			else
			{
				/* There wasn't an empty slot.  We need
				 * to add a new array in the linked list.
				 */
				array = getnewsitearray(mapp);
				array->s_entries[0].s_site = site;
				/*insert the array in the linked list.
				 *It is easiest to insert it at the beginning*/
				/* it may be easiest, but it's also less
				** efficient to search and should probably be 
				** changed if we ever go to larger clusters.
				** AR
				*/
				array->s_next = mapp->s_array;
				mapp->s_array = array;
				return (&array->s_entries[0]);
			}
		}
		else
		{
			/* we didn't find it, return NULL */
			return (NULL);
		}
		/*NOTREACHED*/
	default:
		panic ("getsiteentry: invalid map type");
	}
	/*NOTREACHED*/
}

/*
 *return the count associated with a specific site in a specific map.
 */
int
getsitecount(mapp,site)
struct sitemap *mapp;
site_t site;
{
	register struct siteentry *entry;

	/* Find the entry */
	entry=getsiteentry(mapp,site,0);
	/* And return the count */
	return(entry?entry->s_count:0);
}

/*
 *update the count associated with a specific site by adding the appropriate
 *value.  value may be negative, but the result should not be negative.
 *Return the final value of the count.
 *
 *	Be sure to update INC_SITECOUNT() and DEC_SITECOUNT() when making
 * changes to updatesitecount(). They are located in sitemap.h
 */
int
updatesitecount(mapp,site,value)
register struct sitemap *mapp;
site_t site;
int value;
{
	register struct siteentry *entry;
	register struct sitearray *array;
	register struct sitearray *next;
	sv_sema_t ss;

	/* Check for a delta value of 0;  If found, log an error and return 0 */
	if (value==0) 
	{
		msg_printf("updatesitecount:  update of 0 attempted for site %d\n",site);
		return(0);
	}

	PXSEMA(&filesys_sema, &ss);
	/* First find the entry.  If we are incrementing the count, and
	 * there was no entry, create a slot.  If we are decrementing the
	 * count and there was no entry, then we are decrementing a count
	 * that wasn't incremented, so panic
	 */
	entry=getsiteentry(mapp,site,value>0);
	if (entry == NULL)
	{
		panic("updatesitecount: decrementing nonexistent site");
		return (0);
	}
	/* Update the site count */
	entry->s_count += value;
	if (entry->s_count < 0)
	{
		panic("updatesitecount: count < 0");
		entry->s_count = 0;
	}
	/* If we have decremented the count associated with the site to zero,
	 * we can decrement the total number of entries in the map.  If this
	 * resets this count to zero, reset the map type to S_MAPEMPTY, and
	 * release the array, if any.  Note that we do not attempt to convert
	 * from an S_ARRAY type to an S_ONESITE type if the count goes from two
	 * to one.  We also don't attempt to release unused array entries if
	 * they become completely empty.  It was not felt to be worth the extra
	 * code and effort to clean up the sitemaps unless the become completely
	 * empty.
	 */
	if (entry->s_count == 0)
	{
		/* The entry is empty, decrement the total count */
		if (--(mapp->s_count) == 0)
		{
			/* There were no more entries in the map.  Clear it*/
			if (mapp->s_maptype == S_ARRAY)
			{
				/* If the map was an array, release the
				 * array[s]
				 */
				array = mapp->s_array;
				while (array != NULL)
				{
					next = array->s_next;
					kmem_free(array, sizeof(struct sitearray));
					array = next;
				}
			}
			/* The map is now empty */
			mapp->s_maptype = S_MAPEMPTY;
		}
		VXSEMA(&filesys_sema, &ss);
		return (0);
	}
	VXSEMA(&filesys_sema, &ss);
	/* Return the new count */
	return (entry->s_count);
}

/*
 *Convert a sitemap to a sitelist for use with multisite messages.  Return
 *a pointer to a buffer containing the list.  If there are no sites, return
 *NULL.  Make sure and exclude the local site.
 */
struct buf *
createsitelist(map, open_flag)
struct sitemap map;
int open_flag;
{
	register struct dm_multisite *mp;
	register struct sitearray *array;
	register int count;
	register site_t site;
	register struct buf *bp;

	/* Get a block to make into the sitelist */
	bp = geteblk(sizeof(struct dm_multisite));
	waitlock (&map);
	/* If there are no sites, return NULL */
	if (map.s_maptype == S_MAPEMPTY)
	{
		brelse(bp);
		return (NULL);
	}
	/* Set a pointer to the start of the data */
	mp = (struct dm_multisite *) bp->b_un.b_addr;
	/* Clear the data */
	bzero(mp,sizeof(struct dm_multisite));
	if (map.s_maptype == S_ONESITE)
	{
		/* If there is only one site in the map, enter it
		 * into the sitelist, unless it is my site.  In that
		 * case, release the buffer and return NULL.
		 */
		site = map.s_onesite.s_site;
		if (site == my_site ||
			site == 0 ||
			(open_flag && site == u.u_site &&
			 map.s_onesite.s_count == 1))
		{
			brelse(bp);
			return(NULL);
		}
		mp->maxsite = map.s_onesite.s_site;
		mp->dm_sites[mp->maxsite].site_is_valid = 1;
	}
	else 
	{
		/* There may be multiple sites.  Traverse the array,
		 * entering the entries in the sitelist.  However,
		 * if we find our own site, skip it.  Keep track of
		 * whether we have found any site (other than our own),
		 * because if we didn't, we will need to release the
		 * buffer and return NULL.
		 */

		register int found = 0;

		array = map.s_array;
		while (array != NULL)
		{
			for (count = 0; count < SITEARRAYSIZE; count++)
			{
				if (array->s_entries[count].s_count != 0)
				{
					site = array->s_entries[count].s_site;
					if (site == my_site || site == 0 ||
					 (open_flag && 
					  site == u.u_site &&
					  array->s_entries[count].s_count == 1))
						continue;
					if (site > mp->maxsite)
						mp->maxsite = site;
					mp->dm_sites[site].site_is_valid = 1;
					found++;
				}
			}
			array = array->s_next;
		}
		/* If we didn't find any entry other than our own site,
		 * release the buffer and return NULL
		 */
		if (!found)
		{
			brelse(bp);
			return(NULL);
		}
	}
	/* The caller must return this buffer! */
	return (bp);
}


/*
 *The following function deals with the double count.  A double count
 *is actually two counts.  One count, the real count reflects the actual
 *value of the count.  The other count, the virtual count,
 * keeps track of what the server thinks the count
 *is.  The latter count is only updated when the request for change comes
 *from the server or when the real count is 0.
 *
 *The function updates the real count by delta (which may be positive or 
 *negative).  If serverinitiated is true, the virtual count is also
 *updated.  Afterwards, a check is made.  If it is necessary to send a
 *message to the server, either because the real count went to zero,
 *or because the virtual count got to large (we want to avoid an overflow),
 *we will decrement the virtual count by an appropriate amount and return that
 *value.  That value should be sent to the server, who should update
 *the appropriate sitemap entry by the same amount.
 */
int
updatedcount(dcountp,delta,serverinitiated)
register struct dcount *dcountp;
int delta;
int serverinitiated;
{
	register int subtract = 0;

	SPINLOCK(v_count_lock);
	/*Update the real count*/
	dcountp->d_rcount += delta;
	/*If appropraite, update the virtual count*/
	if (serverinitiated)
		dcountp->d_vcount += delta;
	/*Have we gone negative?  Impossible!!*/
	if (dcountp->d_rcount < 0 || dcountp->d_vcount < 0)
		panic ("updatedcount: negative result");
	/* If the real count went to zero, the virtual count must go to zero and
	 * we must send a message to the server
	 */
	if (dcountp->d_rcount == 0)
	{
		subtract = dcountp->d_vcount;
		dcountp->d_vcount = 0;
	}
	/* If the virtual count is Zero, we must set this to one and then send a
         * message to the server, so that it is aware of this client's usage.
	 */
	else if (dcountp->d_vcount ==  0)
	{
		subtract = -1;  /* actually an "add" here */
		dcountp->d_vcount = 1;
	}
	/* If the virtual count got to large, we must also send a message to the
	 * server.
	 */
	else if (dcountp->d_vcount > CHECKVCOUNT)
	{
#ifdef OSDEBUG
		msg_printf("updatesitecount:  exceeded CHECKVCOUNT\n");
#endif OSDEBUG
		subtract = dcountp->d_vcount - dcountp->d_rcount;
		dcountp->d_vcount = dcountp->d_rcount;
	}
	/* Return the count that should be decremented at the server.  Unless
	 * the virtual count was decremented, this will be zero.
	 */
	SPINUNLOCK(v_count_lock);
	return (subtract);
}
