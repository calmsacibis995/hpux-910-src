/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/drv_init.c,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:12:26 $
 */

/*
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1990. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */
 
#if  defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) drv_init.c $Revision: 1.5.84.3 $ $Date: 93/09/17 21:12:26 $";
#endif


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vnode.h"   /* added for 6.0 DUX Vnode kernel jtg */
#include "../ufs/inode.h"
#include "../h/stat.h"

#include "../wsio/timeout.h"	/* for lan_link routines -- driver stuff */
#include "../wsio/hpibio.h"	/* for lan_link routines -- driver stuff */
#include "../h/conf.h"		/* for lan_msus_for_boot -- cdevsw struct */
#include "../s200io/bootrom.h"  /* for lan_msus_for_boot -- msus struct */

extern int lanift_init();    /* PEKING change */
/*
 * LAN Driver iosw table, we use only the first entry which is reserved for
 * initialization routine
 */    /* PEKING change */

struct drv_table_type lan_iosw[] = {
	lanift_init,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
	(int (*)())NULL,
};

/*
 *	linking and initialization routines.
 *
 *	An entry for lan_link is placed in a table of drivers to be called
 *	upon boot time.	 When lan_link is called, it places the function,
 *	"lan_make_entry" on the top of a linked list of driver initialization
 *	routines.  The linked list has at its head, the entry in the extern
 *	variable, "make_entry".	 The global (to LAN) variable, "lan_saved_
 *	make_entry", is the link to the next routine.
 *
 *	As the system encounters i/o cards, it calls the top function on the
 *	list.  If the card is for its driver, it initializes (and tests) it and
 *	returns TRUE (typically), so the system knows that the driver is there.
 *	We do not return TRUE, because the card is not tested and powered up
 *	until npowerup is called.  If the card is not for the driver routine
 *	that has been called, it will call the next routine on the linked list
 *	through its saved entry.  This is done for all i/o cards in the system.
 *
 *	Because LAN consists of 2 major numbers, lan_link will be called twice.
 *	The second time nothing is done.
 *
 */

extern	int	(*make_entry)();
	
int	(*lan_saved_make_entry)();
	
lan_make_entry(id, isc)
	int id;
	struct	isc_table_type	*isc;
{
	if ( id == 21) {
		isc->card_type = HP98643;
		/* 
		 * force kernel_initialize() to call lan_init() through
		 * lan_iosw[] table
		 */
		isc->iosw = lan_iosw;
		return io_inform("HP98643", isc, 5);

	} else
		return(*lan_saved_make_entry)( id, isc);
}

struct msus (*lan_saved_msus_for_boot)();
/*
**  code for converting a dev_t to a boot ROM msus for reboot to lan device
*/
struct msus lan_msus_for_boot(blocked, dev)
char blocked;
dev_t dev;
{
	extern struct cdevsw cdevsw[];
	extern int lla_open();

	int maj = major(dev);
	struct msus my_msus;

	/*
	**  does this driver not handle the specified device?
	*/
	if (blocked || 
            (!blocked && (maj >= nchrdev || cdevsw[maj].d_open != lla_open))) 
		return (*lan_saved_msus_for_boot)(blocked, dev);

	/*
	** construct the boot ROM msus
	*/
	my_msus.dir_format = 7;		/* Special */
	my_msus.device_type = 2;	/* LAN */
	my_msus.sc = m_selcode(dev);
	my_msus.ba = m_busaddr(dev);	/* don't care for LAN */
	my_msus.unit = m_unit(dev);	/* don't care for LAN */
	my_msus.vol = m_volume(dev);	/* don't care for LAN */
	return my_msus;
}
	
lan_link()
{
	extern struct msus (*msus_for_boot)();
	static int done = 0;

	if (done == 0) {
		lan_saved_make_entry = make_entry;
		make_entry = lan_make_entry;

		/* put msus routine in list for reboot-from-lan capability */
		lan_saved_msus_for_boot = msus_for_boot;
		msus_for_boot = lan_msus_for_boot;

		done = 1;
	}
}
