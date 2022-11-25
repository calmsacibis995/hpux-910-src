/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/fixed_rmap.c,v $
 * $Revision: 1.3.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:43:49 $
 */

/* HPUX_ID: @(#)fixed_rmap.c	55.1		88/12/23 */

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

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/systm.h"
/*
 *routines for allocating a fixed sized and small resource, of which an
 *indeterminate number of entries will be allocated.  The routines will
 *if necessary allocate a disc buffer and subdivide it into fixed chunks of
 *the appropriate size, which it will link together.  It will then
 *allocate out of that list until it again needs a new buffer.
 *This package is somewhat of a kludge and could be replaced by a better memory
 *allocation scheme.
 */

/*
 *The following constants are used to lock a map while a new buffer is being
 *allocated to prevent concurrent buffer allocation.  The constants should not
 *be valid pointer addresses.
 */
#define BUFFER_LOCKED ((caddr_t)(-1))
#define BUFFER_WAITING ((caddr_t)(-3))
/*
 *Increase the size of the allocated resource map by getting another
 *buffer.  Note that once this buffer has been allocated it will never be
 *returned.   Therefore these routines should not be used with a
 *resource which is likely to grow very large for a short amount of
 *time.
 *Growmap is only called internally from getfixedresources.
 */
static
growmap(resourcesize,mapp,buffersize)
register int resourcesize;	/*the size of the resource.  Must always
				 *be the same for a given map.
			 	 */
register caddr_t *mapp;		/*the address of a map pointer.  Map must
				 *initially be NULL.
			 	 */
register int buffersize;	/*the size of the buffer to be allocated
				 *eachtime.
				 */
{
	register caddr_t bufp;
	register caddr_t lastbuf;
	register struct buf *bp;

	/* Lock the map */
	*mapp = BUFFER_LOCKED;
	/* Make sure that the resource is at least big enough to hold a
	 * pointer */
	if (resourcesize < sizeof(caddr_t))
		resourcesize = sizeof(caddr_t);
	/* Get a disc buffer */
	bp = geteblk(buffersize);
	/* wakeup any sleepers */
	if (*mapp == BUFFER_WAITING)
		wakeup (mapp);
	/* Set the passed map to point to the start of the data */
	*mapp = bufp = bp->b_un.b_addr;
	/* Now carve up the buffer onto blocks of the appropriate size,
	 * forming them into a linked list.
	 */
	lastbuf = bufp + buffersize - resourcesize - resourcesize;
	while (bufp <= lastbuf)
	{
		*(caddr_t *)bufp = bufp + resourcesize;
		bufp += resourcesize;
	}
	*(caddr_t *)bufp = NULL;
}

/*
 *Allocate a resource.  First try to take it from the linked list specified
 *by mapp.  If that fails, get a disc buffer and carve it up.
 */
caddr_t 
getfixedresources(resourcesize,mapp,buffersize)
int resourcesize;	/*the size of the resource.  Must always be the same
			 *for a given map.
			 */
register caddr_t *mapp;	/*the address of a map pointer.  Map must initially be
			 *NULL.
			 */
int buffersize;		/*the size of the buffer to be allocated eachtime.*/
{
	register caddr_t chosen;	/*the chosen resource*/

	/* If the map is locked, wait */
	while (*mapp == BUFFER_LOCKED || *mapp == BUFFER_WAITING)
	{
		*mapp = BUFFER_WAITING;
		sleep(mapp,PRIBIO);
	}
	/*If the linked list is empty, get a new buffer*/
	if (*mapp == NULL)
		growmap(resourcesize,mapp,buffersize);
	/*at this point, map will always point to the first element on the
	 *linked list.  Return it, and set mapp to point to the second element.
	 */
	chosen = *mapp;
	*mapp = *(caddr_t *)*mapp;
	return (chosen);
}

/* Place an element back in the linked list pointed at by mapp */
releasefixedresources(mapp,resource)
caddr_t *mapp;		/*the address of a map pointer.*/
caddr_t resource;	/*the resource being returned.*/
{
	*(caddr_t *)resource = *mapp;
	*mapp = resource;
}
