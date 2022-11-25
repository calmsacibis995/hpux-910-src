/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/dio2.c,v $
 * $Revision: 1.6.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:11:25 $
 */
/* HPUX_ID: @(#)dio2.c	55.1		88/12/23 */

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


#include "../h/types.h"
#include "../h/param.h"
#include "../h/pregion.h"
#include "../h/vas.h"
#include "../h/mman.h"
#include "../h/vmmac.h"

extern vas_t kernvas;
extern preg_t *io_map();

preg_t *io_page_map_prp = NULL;

caddr_t
io_page_map(physaddr)
	caddr_t physaddr;
{
	if (io_page_map_prp == NULL) {
		/* allocate virtual address space, map in page */
		io_page_map_prp = io_map((space_t)&kernvas, NULL,
					 physaddr, 1, PROT_URW);
		if (io_page_map_prp == NULL)
			return((caddr_t)0);
	} else {
		io_deletetrans(io_page_map_prp);
		io_page_map_prp->p_hdl.p_physpfn = btop(physaddr);
		io_addtrans(io_page_map_prp);
	}

	/* Return fixed logical address */
	return((caddr_t)io_page_map_prp->p_vaddr);
}

/*
** F_AREA is a pointer to a long which contains the address of the
** lowest byte of usable physical ram as determined by the boot rom.
** See the Briarpatch boot rom manual for details.
*/
#define F_AREA 0xfffffed4

/*
** Declare dio2_debug so driver writers can do debugging.
**
** dio2_debug values:
** 0 -> turn off all debug info
** 1 -> print an error message on all errors
** 2 -> print parameters on entry and return values on exit
** 3 -> painfully verbose mode
**
** Larger values imply lower ones.  For example if dio2_debug = 2
** then all messages for dio2_debug = 1 are also printed.
*/
int dio2_debug = 0;

/*
** dio2_map_page() maps physical_addr into kernel
** logical address space. This routine returns the
** kernel logical address to be used when accessing
** physical_addr.  Only one page is mapped in.  This 
** routine is used by kernel_initialize() to scan 
** the DIOII backplane for plug in external DIOII 
** interfaces.  See s200io/kernel.c for an example
** of how this routine is used.
*/

caddr_t dio2_map_page(physical_addr)
caddr_t physical_addr;
{
	register caddr_t vaddr;
	extern caddr_t io_page_map();

	if (dio2_debug > 1)
		printf("entered dio2_map_page: physical_addr = 0x%x\n", 
                        physical_addr);

	vaddr = io_page_map(physical_addr);
	if (dio2_debug > 1)
		printf("exiting dio2_map_page: returning logical_addr = 0x%x\n",
                        vaddr);

	return(vaddr);
}


/*
** Map nbytes beginning at physical_addr into kernel
** logical address space.  This routine returns the
** kernel logical address to be used when accessing
** this address space.  This routine is used by 
** kernel_initialize() to scan the DIOII backplane 
** for plug in external DIOII interfaces.  This
** routine is also available for driver writers
** to map in the address space of interfaces using
** the Stealth VME bus adapter or third party custom
** DIOII cards .  See s200io/kernel.c for an example 
** of how this routine is used.
*/

caddr_t dio2_map_mem(physical_addr, nbytes)
register caddr_t physical_addr;
register unsigned int nbytes;
{
	register preg_t *prp;
	register int pages;

	if (dio2_debug > 1)
		printf("entered dio2_map_mem: physical_addr = 0x%x nbytes = 0x%x\n", physical_addr, nbytes);

	/* check to see if the parameters are reasonable */
	if (((unsigned int)physical_addr + nbytes) >
			    (*(unsigned int *)F_AREA)) {
	    	/* request overlays physical ram */
		if (dio2_debug)
    printf("dio2_map_mem: request overlays physical ram!\n");
		if (dio2_debug > 1)
			printf("exiting dio2_map_mem: returning 0\n");
		return((caddr_t)0);
	}

	if (((unsigned int)physical_addr + nbytes) <
				(unsigned int)physical_addr) {
	    	/* physical_addr + nbytes overflows 4GB */
		if (dio2_debug)
			printf("dio2_map_mem: request is too large\n");
		if (dio2_debug > 1)
			printf("exiting dio2_map_mem: returning 0\n");
		return((caddr_t)0);
	}

	if (nbytes == 0) {
	    	/* no bytes requested  - a real corner case */
		if (dio2_debug)
			printf("dio2_map_mem: nbytes requested is zero\n");
		if (dio2_debug > 1)
			printf("exiting dio2_map_mem: returning 0\n");
		return((caddr_t)0);
	}

	/*
	** calculate the total number of pages needed to map in nbytes.
	*/
	pages = ((nbytes - 1) / NBPG) + 1;

	/* if physical_addr is not page aligned then we need one more */
	if (((unsigned int)physical_addr % NBPG) != 0) {
		pages++;
		if (dio2_debug > 2)
printf("dio2_map_mem: physical_addr is not page aligned - bumping pages\n");
	}

	if (dio2_debug > 2)
		printf("dio2_map_mem: pages = %d\n", pages);

	/* allocate virtual address space, map in pages */
	if ( (prp = io_map((space_t)&kernvas, NULL,
			   physical_addr, pages, PROT_URW)) ==	NULL) {
		if (dio2_debug)
			printf("dio2_map_mem: rmalloc failed\n");
		if (dio2_debug > 1)
			printf("exiting dio2_map_mem: returning 0\n");
		return((caddr_t)0);
	}

	if (dio2_debug > 1)
		printf("exiting dio2_map_mem: logical_addr = 0x%x\n",
					prp->p_vaddr);

	/* return the kernel logical address */
	return(prp->p_vaddr);
}

/*
** This routine releases any resources allocated 
** with a call to dio2_map_mem.
*/

dio2_unmap_mem(logical_addr, nbytes)
register caddr_t logical_addr;
register unsigned int nbytes;
{
	register preg_t *prp;

	if (dio2_debug > 1)
    printf("entered dio2_unmap_mem: logical_addr = 0x%x nbytes = 0x%x\n",
				logical_addr, nbytes);

	/* Get the pregion this view was through */
	if ( (prp = findprp(&kernvas, (space_t)&kernvas, logical_addr)) ==
					NULL) {
		panic("dio2_unmap_mem on bad logical_addr");
	}

	if (dio2_debug > 2)
		printf("dio2_unmap_mem: pages = %d\n", prp->p_count);

	/* now cleanup the mess */
	io_unmap(prp);

	if (dio2_debug > 1)
		printf("exiting dio2_unmap_mem\n");
}
