/* HPUX_ID       @(#)snalink.c	1.1     88/04/13	*/

/*
 * SDLC driver
 *	This document contains preliminary unsupported information.
 */
#include <param.h>
#include <dir.h>
#include <signal.h>
#include <systm.h>
#include <user.h>		/* user structure */
#include <tty.h>		/* user structure */
#include <hpibio.h>		/* isc_table_type structure */
#include <errno.h>	
#include <proc.h>
#include <uio.h>
#include <sysmacros.h>
#include <intrpt.h>
#include <pdi.h>

extern struct isc_table_type *isc_table[];

    /*
     * This is a symbol in the kernel--but the user version of "user.h"
     *	doesn't declare it, so we do it ourselves.
     */
extern struct user u;

#define Z80_FAM 52		/* Device ID we will own */
#define SDLC_ID 252		/*  Sub-ID for the 98628 family */
#define CARD_SIZE 1024		/* # bytes in a block from this card */


    /*
     * Open the sdlc device. This routine is called on each open() system
     *	call invoked by a user process.
     */
snalink_open(dev, flag)
    int dev, flag;
{
    register struct isc_table_type *isc;
    register char *card;
    register int unit;
    int old_level;

    unit = m_selcode(dev);
    if ((unit < 0) || (unit > 31))		/* bad minor number */
	return(ENXIO);

    if ((isc = isc_table[unit]) == NULL)	/* no device at selcode */
	return(ENXIO);
    
	/*
	 * Allow only one open at a time
	 */
    old_level = spl4();
    if( isc->active ){
	splx(old_level);
	return(EACCES);
    }
    isc->active = 1;
    splx(old_level);

	/*
	 * Save the PID, for signalling on interrupts
	 */
    isc->ppoll_f = (struct buf *)u.u_procp;

    return( 0 );
}

    /*
     * Close the device. This routine is called on the last close of
     *	the device.
     */
snalink_close(dev)
    int dev;
{
    register struct isc_table_type *isc;
    char *card;

    isc = isc_table[m_selcode(dev)];
    card = (char *)(isc->card_ptr);

	/*
	 * The following sequence should make the card shut up &
	 *	go to sleep.
	 */
    card[pdi_reset] = 0x80;

	/*
	 * Nobody's using the card, so disable its signals
	 */
    isc->ppoll_f = 0;

	/*
	 * Release our hold of the interlock
	 */
    isc->active = 0;

    return(0);
}

    /*
     * Interrupt service routine. Called when our card generates
     *	an interrupt.
     */
snalink_isr(info)
    struct interrupt *info;
{
    register int parm1;
    register struct isc_table_type *isc = isc_table[info->temp];
    register char *card = (char *)isc->card_ptr;
    register struct proc *our_proc = (struct proc *)isc->ppoll_f;

	/*
	 * Is this interrupt for us?
	 */
    if( (card[3] & 0x40) == 0 )
	return;

	/*
	 *  The reference to location 0x4001 clears
	     *	the interrupt on that card.
	     */
    parm1 = card[0x4001];

	/*
	 * Turn the interrupt into a signal for our opener
	 */
    if( our_proc != 0 )
	psignal( our_proc, SIGIOT );
}

extern int (*make_entry)();		/* Pointer to linked list of drivers */
int (*snalink_saved_make_entry)();		/* Pointer to driver before us */

    /*
     * snalink_make_entry()
     *	At power-up time, each device is associated with a driver. The driver's
     *	  powerup routine will either recognize the device & claim it, or else
     *	  reject it. If the device is rejected, it will be passed on to
     *	  the other drivers on the system.
     */
snalink_make_entry(id, isc)
    int id;
    struct isc_table_type *isc;
{
    register unsigned char *card = (unsigned char *)isc->card_ptr;

	/*
	 * If we recognize the ID, then this device belongs with us.
	 *	Otherwise, we refer it to the next driver in the list.
	 */
    if( id != Z80_FAM )
	return( (* snalink_saved_make_entry)(id,isc) );
    if( card[0x402F] != SDLC_ID )
	return( (* snalink_saved_make_entry)(id,isc) );

	/*
	 * Arrange for interrupts to go to our service routine
	 * isrlink(isr, level, regvalue, compare-value,
	 *		parm1, parm2);
	 */
    isrlink(snalink_isr, isc->int_lvl, (int) isc->card_ptr + 3, 0xc0, 0xc0,
		0, ((int)isc->card_ptr >> 16) & 0x1f);
    isc->ppoll_f = 0;
    isc->active = 0;

	/*
	 * Tell the user his card has been found & accepted
	 */
    io_inform("SDLC card", isc, isc->int_lvl);
    
	/*
	 * Do anything you want to your new card here
	 */
    /*
	card[n] = ....
     */
    
	/*
	 * The isc struct pointer we've been using is not the permanent one.
	 *	If we return with TRUE, what we fill into this struct will
	 *	be copied into an allocated permanent struct. If we return
	 *	FALSE, we will have to calloc() the isc struct ourselves.
	 */
    return( TRUE );
}

    /*
     * snalink_link()
     *	This is the very first code called in a driver. All it does is
     *	  link itself into a list of ALL drivers.
     */
snalink_link(){
    snalink_saved_make_entry = make_entry;
    make_entry = snalink_make_entry;
}

