/* HPUX_ID       @(#)vme.skel.c	1.1     88/04/13	*/

#include <errno.h>
#include <param.h>
#include <buf.h>
#include <dir.h>
#include <signal.h>
#include <timeout.h>	
#include <hpibio.h>
#include <uio.h>
#include <skel.h>
#include <ioctl.h>
#include <param.h>
#include <user.h>
#include <vme2.h>

/* global data structure declarations */

struct skelregs *skel=(struct skelregs *) 0x791000;   /* board address */
struct buf skelbuf;        /* io buffer */

skel_first_open=1;
int skelopened;            /* handler has been opened */
int skel_strategy();
int skel_isr();

#define SKELPRI 10                /* sleep priority */
#define SKELIRQ  2                /* interrupt level */

/*

/*
 * SKEL_OPEN
 *      Perform initialization on first open.
 *      Allow only one open at a time
 */
skel_init() { }

skel_open(dev, flag) 
dev_t dev;
int flag;
{

int vec_num;

	if (skel_first_open) {
	/* This is the first call to open since boot.
	   Complete power-up initialization		*/

		skel_first_open = 0;

		/* Initialize global data structures here */
		/* Initialize hardware here	*/

		/* set up hardware vector address */
		if ( (vec_num = set_vector(skel_isr) ) == -1)
			panic("out of interrupt vectors");
		card_ptr->vector_number = vec_num;
	}
	/* The following implements exclusive open */
	if (skelopened)
		u.u_error = EACCES;
	skelopened++;
	return(0);
}

skel_close(dev) 
dev_t dev;
{
	/* The following implements exclusive open of the device */
	skelopened = 0;
	return(0);
}

/*
 * SKEL_READ
 * Read up to cnt bytes of data from the device into buf. 
 */

skel_read(dev, uio) 
dev_t dev;
struct uio *uio;
{
	/* The read function is implemented through the strategy routine */
	return (physio(skel_strategy, &skelbuf, dev, B_READ, minphys, uio) );
}

/*
 * SKEL_WRITE
 * Write cnt bytes of data from buf to the device.  
 */

skel_write(dev, uio)
dev_t dev;
struct uio *uio; 
{
	/* The write function is implemented through the strategy routine */
	return(physio(skel_strategy, &skelbuf, dev, B_WRITE, minphys, uio) );
}

/*
 * SKEL_STRATEGY routine:
 * The strategy routine is responsible for setting the
 * hardware up for a transfer and starting the transfer
 */

skel_strategy(bp) 
struct buf *bp;
{

int      pri;
register caddr_t dmaddr;
short   cnt;

	pri= spl4();

	dmaddr = bp->b_un.b_addr;
	cnt = bp->b_bcount;

	if (bp->b_flags & B_READ)
	{
	/* prepare card for read transfer */
	}
	else
	{
	/* prepare card for write transfer */
	}

	/* start transfer of data */


	splx(pri);
}

/*
 * SKEL_IOCTL
 *      The ioctl routine is used to implement the remaining functions.
 *      Since the functions require at most one argument the third
 *      address is taken to be an argument passed by value.
 *
 */
#define SKEL_IOCTL1   _IO(V,1)
#define SKEL_IOCTL2   _IO(V,2)
#define SKEL_IOCTL3   _IO(V,3)
#define SKEL_IOCTL4   _IO(V,4)
#define SKEL_IOCTL5   _IO(V,5)
#define SKEL_IOCTL6   _IO(V,6)
#define SKEL_IOCTL7  _IO(V,7)
#define SKEL_IOCTL8   _IO(V,8)

skel_ioctl(dev,code,v) {
	switch(code) {
		case SKEL_IOCTL1:
			/* Call routine or insert code to implement first ioctl */
			break;
		case SKEL_IOCTL2:
			/* Call routine or insert code to implement second ioctl */
			break;
		case SKEL_IOCTL3:
			/* Call routine or insert code to implement third ioctl */
			break;
		case SKEL_IOCTL4:
			/* Call routine or insert code to implement fourth ioctl */
			break;
		case SKEL_IOCTL5:
			/* Call routine or insert code to implement fifth ioctl */
			break;
		case SKEL_IOCTL6:
			/* Call routine or insert code to implement sixth ioctl */
			break;
		case SKEL_IOCTL7:
			/* Call routine or insert code to implement seventh ioctl */
			break;
		case SKEL_IOCTL8:
			/* Call routine or insert code to implement eighth ioctl */
	}
	return(0);
}

/*
 * SKEL_ISR
 */
int skel_isr() {
	BEGIN_ISR;

	 /* determine if board caused interrupt */

	/* reset card here */
	/* clear interrupt on card */

	if (card indicates transfer is complete)
		iodone(&skelbuf);
	else    wakeup(&skelbuf);

	END_ISR;
}

