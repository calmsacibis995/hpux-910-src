/*
 * @(#)sitemap.h: $Revision: 1.5.83.3 $ $Date: 93/09/17 16:46:13 $
 * $Locker:  $
 */

/* HPUX_ID: @(#)sitemap.h	51.1		88/01/20 */

/* 
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/


#ifndef _SYS_SITEMAP_INCLUDED
#define _SYS_SITEMAP_INCLUDED

/* This file describes the sitemap and the double count */

/*
 * A sitemap is used for keeping track of resource usage.  A sitemap can
 * logically be considered an array of entries.  Each entry lists a site and the
 * number of times that site makes use of the resource.  Physically, in
 * order to save space, the sitemap is constructed slightly differently.
 * It consists of a count of the number of sites in it, a maptype, and a
 * union.  If the maptype specifies S_MAPEMPTY, there are no sites in the
 * map.  If the maptype specifies S_ONESITE, the map contains one site,
 * with the site number and the number of references stored in the union.
 * These two, the site number and the reference count, form a siteentry.
 * If the maptype is S_ARRAY, the union is a pointer to an array of site-
 * entries.  If necessary, the last entry of this array is a pointer to
 * another array.
 */

#define SITEARRAYSIZE 7	/*number of entries in a sitearray.  Best if 1 less
			 *than a power of 2*/
#define SITEBUFSIZE   2048 

/* There is one site entry for each site referencing the resource.  An entry
 * is free if the count is zero.
 */
struct siteentry
{
	site_t s_site;	/*site referencing file*/
	short s_count;	/*number of references*/
};

/* The site array is used if there are multiple sites.  Each array
 * a list of siteentries.  If necessary, the s_next field points to
 * another array.
 */
struct sitearray
{
	struct siteentry s_entries[SITEARRAYSIZE];
	struct sitearray *s_next;
};

/* The sitemap itself*/
struct sitemap
{
	short s_count;			/*total number of sites*/
	short s_maptype;		/*see below*/
	union
	{
		struct siteentry su_onesite;	/*single site*/
		struct sitearray *su_array;	/*pointer to array for
						 *multiple sites*/
	} s_union;
#define s_onesite s_union.su_onesite
#define s_array s_union.su_array
};

/*map types*/
#define S_MAPEMPTY	0	/*there is no map*/
#define S_ONESITE	1	/*use the s_onesite field (only one site)*/
#define S_ARRAY		2	/*use the s_array field (there are probably
				 *multiple sites, although all but one could
				 *have been released*/
#define S_LOCKED	3	/*temporary type for use while allocating
				 *arrays*/
#define S_WAITING	4	/*like S_LOCKED, except that someone is also
				 *waiting on the lock*/

/*macro functions*/

/*Get the total number of sites in a map */
#define gettotalsites(mapp) ((mapp)->s_count)

/*Initialize a map to zero.  This should not be used to reset a
 *map that was previously used, because it does not release any preallocated
 *arrays.  It should only be used to initialize garbage.
 */
#define clearmap(mapp) { \
	(mapp)->s_maptype = S_MAPEMPTY ; \
	(mapp)->s_count = 0 ; \
}

/*The following definitions define a double count.  It may be used at
 *the client site to eliminate extra messages.  The real count is
 *always updated.  The virtual count is only updated when initiated
 *by the server.  If the real count ever goes to zero, the virtual
 *count is also set to zero and a message is sent to the server.
 */

struct dcount
{
	int d_rcount;	/*real count*/
	int d_vcount;	/*virtual count*/
};

/*macro definitions, constants, etc...*/
#define SERVERINITIATED 1
#define USERINITIATED 0

#define CHECKVCOUNT 256   /*if virtual count > this, decriment both counts*/
			  /* 256*266 < FFFF prevents overflow of inode.vcount */

/*macro functions*/

/*Return the real count.  Note that no application routine should ever
 *access the virtual count directly.
 */
#define getrealcount(dcountp) ((dcountp)->d_rcount)
#define GET_REAL_EXEC_DCOUNT(count, vp) \
	if((vp)->v_fstype == VDUX) { \
		count = getrealcount((struct dcount *)&(VTOI(vp)->i_execdcount)); \
	} \
	else if((vp)->v_fstype == VDUX_CDFS) { \
		count = getrealcount((struct dcount *)&(VTOCD(vp)->cd_execdcount)); \
	} \
	else panic("GET_REAL_EXEC_DCOUNT: illegal fs type"); 

/*
 * Be sure that parameters passed into INC_SITECOUNT() and DEC_SITECOUNT()
 * are local variables so that the macros do not have to do extra memory
 * references with every parameter use. IE. don't do this:
 *	INC_SITECOUNT(a->b->c->mapp, a->b->site);
 * Caution: don't call INC_SITECOUNT with a parameter name of inc_v_site.
 * Caution: don't call DEC_SITECOUNT with a parameter name of dec_v_site.
 */
extern site_t	my_site;
#define INC_SITECOUNT(mapp, site)			\
{							\
 sv_sema_t	ss;					\
 int		maptype;				\
 site_t		inc_v_site;				\
 PXSEMA(&filesys_sema, &ss);				\
 inc_v_site = ((site)==0?my_site:(site));		\
 if ((maptype=(mapp)->s_maptype) == S_ONESITE) {	\
	if ((mapp)->s_onesite.s_site == inc_v_site ) { 	\
		(mapp)->s_onesite.s_count++;		\
		VXSEMA(&filesys_sema, &ss);		\
	} else {					\
		VXSEMA(&filesys_sema, &ss);		\
		updatesitecount(mapp, site, 1);		\
	}						\
 } else if (maptype == S_MAPEMPTY) {			\
	(mapp)->s_count = 1;				\
	(mapp)->s_maptype = S_ONESITE;			\
	(mapp)->s_onesite.s_site = inc_v_site;		\
	(mapp)->s_onesite.s_count = 1;			\
	VXSEMA(&filesys_sema, &ss);			\
 } else {						\
	VXSEMA(&filesys_sema, &ss);			\
	updatesitecount(mapp, site, 1);			\
 }							\
}
	
		
#define DEC_SITECOUNT(mapp, site)			\
{							\
 sv_sema_t	ss;					\
 site_t		dec_v_site=((site)==0?my_site:(site));	\
 PXSEMA(&filesys_sema, &ss);				\
 if ((mapp)->s_maptype == S_ONESITE &&			\
     (mapp)->s_onesite.s_site == dec_v_site) {		\
	if (--(mapp)->s_onesite.s_count == 0) {		\
			(mapp)->s_count--;		\
			(mapp)->s_maptype = S_MAPEMPTY; \
		}					\
		VXSEMA(&filesys_sema, &ss);		\
 } else {						\
		VXSEMA(&filesys_sema, &ss);		\
		updatesitecount(mapp, site, -1);	\
 }							\
}
#endif /* not _SYS_SITEMAP_INCLUDED */
