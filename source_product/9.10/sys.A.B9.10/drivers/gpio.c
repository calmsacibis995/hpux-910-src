/* HPUX_ID       @(#)gpio.c	1.1     88/04/13	*/

/* stuff kept in buf structure */
#define	pass_data1		b_bufsize	/* put temp parameter info */
#define	pass_data2		b_pfcent	/* put temp parameter info */
#define	return_data		b_bufsize	/* put temp return info */
#define	wait_event		b_s0	/* wait for bus status */
#define ppoll_data		b_s1	/* put termination reason stuff here */
#define	gpio_lines		b_s1	/* control/status lines */
#define	dil_packet		b_s2

/* stuff kept in iobuf structure */
#define	read_pattern		io_nreg	/* hold read terminating pattern */
#define term_reason		io_s0	/* put termination reason stuff here */
#define	activity_timeout	io_s1	/* for use during any activity */
#define	dil_timeout		timeo	/* for all timeouts */
#define	dil_trigger		intloc	/* to drop priorty level */
#define	dil_state		b_errcnt	/* keep dil info (size char) */

#define locking_pid		tty_routine

/* buf.h uses 0x40000000 for B_DIL to show that this is a dil buffer */

/*
 * These flags are kept in dil_state.
 */
/* DIL state stuff */
#define D_CHAN_LOCKED	0x01	/* this process has the channel locked */
#define D_16_BIT 	0x02	/* transfer width is 16 bit */
#define D_RAW_CHAN 	0x04	/* this is a raw open */
#define D_MAPPED 	0x08	/* this was mapped into user space */


/* transfer information for select code in dil state */
#define TFR_MASK	0xf0
#define USE_DMA		0x10
#define USE_INTR	0x20
#define READ_PATTERN	0x40
#define EOI_CONTROL	0x80

/* extra bit to see if process can touch card */
#define NO_PERMISSION	0x0100

/* extra bit in term_reason to keep track of SRQ event */
#define TERM_SRQ	0x80

#define OWNER		0x02	/* this is the owner(see ioctl's) */

#define NO_ADDRESS	31	/* invalid address for the bus*/

#define	DEFAULT_TIMEOUT	HZ*15	/* default timeout value */

#define DILPRI	PZERO+3

/* iov generic structure */
struct diliovec {
	int	*buffer;
	int	count;
	int	command;
};


/*
**	info for 98622 card 
*/
#define MAX_GPIO 6	/* max number of 98622's */

enum gpio_states { get = 0, get2, transfer, drop, timedout, defaul};

/* memory layout of the card */
struct GPIO {				/* address */
	unsigned char ready;		/*   0	   */
	unsigned char id_reg;		/*   1 	   */
	unsigned char intr_mask;	/*   2 	   */
	unsigned char intr_control;	/*   3 	   */
	union {				/*   4,5   */
		struct {
			unsigned char upper;
			unsigned char lower;
		} byte;
		unsigned short word;
	} data;
	unsigned char nop1;
	unsigned char p_status;		/*   7	   */
};

#define set_pctl ready
#define	reset_gpio id_reg
#define	intr_status intr_control
#define p_control p_status

/* (addr 2) ready/external interrupt control assignments */
#define	GP_EIR1 	0x01
#define	GP_EIR0 	0x00
#define	GP_RDYEN1	0x02
#define	GP_RDYEN0	0x00

/* (addr 3) interrupt/dma control assignments */
#define	GP_DMA0		0x01
#define	GP_DMA1		0x02
#define	GP_WORD		0x04
#define	GP_BURST	0x08
#define	GP_ENAB		0x80

/* (addr 7) status register */
#define	GP_INT_EIR	0x04
#define	GP_PSTS		0x08


#define NO_TIMEOUT	B_SCRACH3

#define LOCK_OWNER_ONLY if ((bp->b_sc->state & LOCKED) && \
	                    ((short)bp->b_sc->locking_pid != \
			     ((struct dil_info *)bp->dil_packet)->dil_procp->p_pid)) { \
				bp->b_error = EIO; \
				bp->b_flags |= B_ERROR | B_DONE; \
				break; \
			}

#define RAW_CHAN_ONLY   if (!(state & D_RAW_CHAN)) { \
				bp->b_error = ENOTTY; \
				bp->b_flags |= B_ERROR | B_DONE; \
				break; \
			}

#define HPIB_CHAN_ONLY   if ((sc->card_type != INTERNAL_HPIB) && \
                             (sc->card_type != HP98624) && \
                             (sc->card_type != HP98625)) { \
				bp->b_error = EINVAL; \
				bp->b_flags |= B_ERROR | B_DONE; \
				break; \
			}

#define GPIO_CHAN_ONLY   if (sc->card_type != HP98622) { \
				bp->b_error = EINVAL; \
				bp->b_flags |= B_ERROR | B_DONE; \
				break; \
			}

#define ACT_CTLR_ONLY   if (!(sc->state & ACTIVE_CONTROLLER)) { \
				bp->b_error = EIO; \
				bp->b_flags |= B_ERROR | B_DONE; \
				break; \
			}

#define SYS_CTLR_ONLY   if (!(sc->state & SYSTEM_CONTROLLER)) { \
				bp->b_error = EIO; \
				bp->b_flags |= B_ERROR | B_DONE; \
				break; \
			}

#define DIL_START_TIME(proc)                                             \
			if (!(iob->b_flags & NO_TIMEOUT)) {              \
				START_TIME(proc, iob->activity_timeout); \
			}  


#define DIL_RECOVERY                              \
		{                                 \
		ABORT_TIME;                       \
		iob->b_state = (int) dil_defaul;  \
		if (escapecode == TIMED_OUT)      \
			bp->b_error = EIO;        \
		(*sc->iosw->iod_abort_io)(bp);    \
		dil_drop_selcode(bp);             \
		queuedone(bp);                    \
		dil_dequeue(bp);                  \
		}


#define crit()		{ 						    \
			asm("	subq.l	#4,sp		* allocate space"); \
			asm("	move	sr,(sp)		* save old sr	"); \
			asm("	ori	#0x0700,sr	* Superman (7!)	"); \
			}

#define uncrit()	{						    \
			asm("	move	(sp),sr		* Clark Kent	"); \
			asm("	addq.l	#4,sp		* return space  "); \
			}


/* sc->intr_wait and sc->intr_event info */
#define WAIT_CTLR	0x0001
#define WAIT_SRQ	0x0002
#define WAIT_TALK	0x0004
#define WAIT_LISTEN	0x0008
#define WAIT_READ	0x0020
#define WAIT_WRITE	0x0040
#define	WAIT_MASK	0x006f



/* user interrupt event info */

#define	INTR_DCL	0x00100
#define	INTR_IFC	0x00200
#define	INTR_GET	0x00400
#define	INTR_GPIO_EIR	0x00400
#define	INTR_LISTEN	0x00800
#define	INTR_TALK	0x01000
#define	INTR_SRQ	0x02000
#define	INTR_REN	0x04000
#define	INTR_CTLR	0x08000
#define	INTR_PPOLL	0x10000
#define	INTR_MASK	0x1ff00

#define NO_RUPT_HANDLER	B_SCRACH2

/* dil info packet */
struct dil_info {
	struct buf * dil_forw;
	struct buf * dil_back;
	struct buf * event_forw;
	struct buf * event_back;
	int eid;
	int (*handler)();
	int event;
	int cause;
	int ppl_mask;
	struct proc *dil_procp;
	int (*dil_action)();
	int (*dil_timeout_proc)();
	int intr_wait;
	int dil_retry_count;
};

#define	CONTROL_LINES		1
#define	STATUS_LINES		2

#include <param.h>
#include <pte.h>
#include <buf.h>
#include <dma.h>
#include <dir.h>
#include <user.h>
#include <timeout.h>
#include <hpibio.h>
#include <simon.h>
#include <ti9914.h>
#include <proc.h>
#include <iobuf.h>
#include <systm.h>
#include <vm.h>
#include <intrpt.h>
#include <tryrec.h>
#include <file.h>
#include <dil.h>	/* may want to move these to h */
#include <gpio.h>
#include <dilio.h>

#define	locking_pid	tty_routine
#define	dil_procp		block_timeout

int gpio_do_isr();
int gpio_strategy();
int gpio_transfer();
int gpio_dma();
int gpio_abort();
int gpio_isr();
int dil_getsc();
int lock_timeout();
int dil_io_lock();
struct buf * get_dilbuf();


gpio_open(dev, flag, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct isc_table_type *sc;
	register struct buf *bp;
	register struct iobuf *iob;
	register int sc_num, ba;

	sc_num = m_selcode(dev);
	ba = m_busaddr(dev);

	/* check for valid selectcodes */
	if (sc_num > 31)
		return(ENXIO);

	if ((sc = isc_table[sc_num]) == NULL)		/* no card */
		return(ENXIO);

	/* init the dil buffer and put on list for ioctl open call */
	if (sc->card_type != HP98622) 	/* GPIO card */
		return(ENXIO);

	/* assign a dil buffer to this open request */
	if ((bp = get_dilbuf()) == NULL) {
		return(ENFILE);
	}
	iob = bp->b_queue;	/* get iobuffer */

	/* always a raw channel */
	iob->dil_state = D_RAW_CHAN;

	bp->b_sc = sc;		/* save selectcode this will be using */

	/* set the dil buffer to defaults */
		/* init to defaults */
	iob->dil_state |= USE_DMA;
	iob->activity_timeout = DEFAULT_TIMEOUT;

	/*
	iob->block_timeout = DEFAULT_TIMEOUT;
	*/
	/* save the process id for io_lock/unlock */
	iob->dil_procp = (int) u.u_procp;

	iob->term_reason = TR_ABNORMAL;
	bp->gpio_lines = 0;

	/* save buffer in the open file structure */
	u.u_fp->f_buf = (caddr_t) bp; 	/* save buffer */

	return 0;
}


gpio_close(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct buf *bp;
	register struct iobuf *iob;

	bp = (struct buf *) u.u_fp->f_buf;	/* get buffer */
	iob = bp->b_queue;

	/* check for remaining lock */
	if (((struct proc *)iob->dil_procp)->p_pid == (short)bp->b_sc->locking_pid) {
		iob->dil_state &= ~D_CHAN_LOCKED;
	}

	release_dilbuf(bp);
	unlock_close_check(bp);
}


gpio_read(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct buf *bp; 	/* get buffer */

	bp = (struct buf *) u.u_fp->f_buf;	/* get buffer */
	return(physio(gpio_strategy, bp, dev, B_READ,minphys,uio));
}


gpio_write(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct buf *bp; 	/* get buffer */

	bp = (struct buf *) u.u_fp->f_buf;	/* get buffer */
	return(physio(gpio_strategy, bp, dev, B_WRITE, minphys, uio));
}


gpio_strategy(bp, uio)
	register struct buf *bp;
	struct uio *uio;
{
	/* this routine will be called from char or block devices */
	register struct iobuf *iob;
	register int s;

	iob = bp->b_queue;

	/* set up any buffer stuff */
	bp->b_resid = bp->b_bcount;
	iob->b_xaddr = bp->b_un.b_addr;
	iob->b_xcount = bp->b_bcount;

	/* check for pid also */
	if ((bp->b_sc->state & LOCKED) && 
            ((short)bp->b_sc->locking_pid != ((struct proc *)iob->dil_procp)->p_pid ))
	{
		/* this process has no permission to touch channel */
		bp->b_flags |= (B_ERROR | B_DONE);
		bp->b_error = EIO;
		return;	/* did not get it */
	}
	/*
	**  check this transfer info?
	*/
	if (iocheck(bp)) {
		bp->b_flags |= (B_ERROR | B_DONE);
		return;
	}
	bp->b_action = gpio_transfer;	/* for queue to call */
	enqueue(iob, bp);
}


gpio_ioctl(dev, cmd, arg, mode, uio)
	dev_t dev;
	register struct ioctl_type *arg;
	struct uio *uio;
{
	register struct buf *bp = (struct buf *) u.u_fp->f_buf;	/* get buffer */
	register struct iobuf *iob = bp->b_queue; 	/* get dil iobuf*/
	struct isc_table_type *sc = bp->b_sc;
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;
	register int state;
	register int status;
	register int status_result;
	register int temp;
	register unsigned char data;
	register unsigned char data1;
	int x;

	acquire_buf(bp);
	bp->b_flags = B_DIL;	/* mark the buffer */
	bp->b_error = 0;	/* clear errors */

	state = iob->dil_state;
	/* check for pid also */
	if ((bp->b_sc->state & LOCKED) && 
            ((short)bp->b_sc->locking_pid != ((struct proc *)iob->dil_procp)->p_pid ))
		/* this process has no permission to touch channel */
		state |= NO_PERMISSION;

	switch( cmd ) {
	case GPIO_CONTROL:
	   switch( arg->type ) {
		case CONTROL_LINES:	/* set control lines */
			if ( state & NO_PERMISSION){
				bp->b_error = EIO;
				break;
			}
			/* make sure we get the lower bits */
			data = arg->data[0] & 0x03;
			cp->p_control = data;
			break;
		default:
			bp->b_error = EINVAL;	/* bad value */
			break;
	   	}	/* control */
		break;
	case GPIO_STATUS:	/* status */
	   switch( arg->type ) {
		case STATUS_LINES:	/* get status lines */
			if ( state & NO_PERMISSION){
				bp->b_error = EIO;
				break;
			}
			arg->data[0] = cp->p_status;
			break;
		default:
			bp->b_error = EINVAL;	/* bad value */
			break;
	   	}	/* status */
		break;
	case IO_CONTROL:
	   switch( arg->type ) {
	      case IO_LOCK:
		      switch (arg->data[0]) {
		        case LOCK:
				if (((struct proc *)iob->dil_procp)->p_pid == (short)sc->locking_pid)
				{	/* we already have it locked */
					arg->data[0] = ++sc->lock_count;
					break;
				}
				bp->b_action = dil_io_lock;
				enqueue(iob, bp);
				iowait(bp);
				arg->data[0] = bp->return_data;
				break;
		        case UNLOCK:
				if (state & NO_PERMISSION)
				{	/* someone else has it locked */
					bp->b_error = EIO;
					break;
				}
				if ((int)sc->locking_pid == 0)
				{	/* no one has it locked */
					arg->data[0] = 0;
					sc->lock_count = 0;
					break;
				}
				if ((--sc->lock_count) == 0)
				{	iob->dil_state &= ~D_CHAN_LOCKED;
					sc->state &= ~LOCKED;
					sc->locking_pid = 0;
					arg->data[0] = 0;
					dil_drop_selcode(sc->owner);
					dil_dequeue(bp);
				} else
					arg->data[0] = sc->lock_count;
				break;
		      }
		      break;

		case IO_TIMEOUT:	/* set the activity timeout */
			temp = arg->data[0];
			bp->b_clock_ticks = temp;	/* save this one */
			if (temp > 0)
				temp = ((temp / 1000) * HZ) / 1000 + 1;
			else temp = 0;
			iob->activity_timeout = temp;

			/* set block timeout */
			/*
			temp = arg->data[1];
			if (temp > 0)
				temp = ((temp / 1000) * HZ) / 1000 + 1;
			else temp = 0;
			iob->block_timeout = temp;
			*/

			break;
		case IO_READ_PATTERN:
			if (arg->data[0] != 0) {
				iob->dil_state |= READ_PATTERN;
				iob->read_pattern = arg->data[1] & 
					((iob->dil_state & D_16_BIT) ? 0xffff : 0xff);
			}
			else {
				iob->dil_state &= ~READ_PATTERN;
				iob->read_pattern = 0;
			}
			break;
		case IO_WIDTH:
			if (arg->data[0] == 16)
				iob->dil_state |= D_16_BIT;
			else if (arg->data[0] == 8)
				iob->dil_state &= ~D_16_BIT;
			else
				bp->b_error = EINVAL;	/* bad value */
			break;
	      	case IO_RESET:
			if ( ! (state & D_RAW_CHAN)) {
				bp->b_error = EINVAL;	/* can't do */
				break;
			}
			if ( state & NO_PERMISSION){
				bp->b_error = EIO;
				break;
			}
			/* do a hard reset to the card */
			gpio_reset(sc);		/* reset the card */
			break;
		case IO_SPEED:	/* set the transfer speed */
			temp = arg->data[0];
			bp->dil_speed = temp;
			iob->dil_state &= ~(USE_DMA|USE_INTR);
			if (temp < 7)
				iob->dil_state |= USE_INTR;
			else
				iob->dil_state |= USE_DMA;
			break;
		default:
			bp->b_error = ENXIO;
			break;
		} /* IO_CONTROL switch */
		break;
	case IO_STATUS:
	   switch( arg->type ) {
		case IO_TERM_REASON:
			arg->data[0] = iob->term_reason;
			break;
		case IO_TIMEOUT:
			arg->data[0] = bp->b_clock_ticks;
			/*
			arg->data[1] = iob->block_timeout;
			*/
			break;
		case IO_WIDTH:
			/* look at flag */
			if (iob->dil_state & D_16_BIT)
				arg->data[0] = 16;
			else 
				arg->data[0] = 8;
			break;
		case IO_SPEED:
			arg->data[0] = bp->dil_speed;
			break;
		case IO_READ_PATTERN:
			if (state & READ_PATTERN)
				arg->data[0] = iob->read_pattern;
			else
				arg->data[0] = -1;
			break;
		case CHANNEL_TYPE:
			arg->data[0] = GPIO_CHAN;
			break;
		}	/* IO_STATUS switch */
		break;
	default:
		bp->b_error = ENXIO;
	} /* switch */

	release_buf(bp);
	return(bp->b_error);
}

gpio_timeout();

gpio_transfer(bp)
register struct buf *bp;
{
	register struct iobuf *iob;

	iob = bp->b_queue;
	try
	START_FSM
	reswitch:
		switch((enum gpio_states) iob->b_state) {
		case get:
			iob->b_state = (int) transfer;
			if (dil_get_selcode(bp, gpio_transfer))
				goto reswitch;	/* got it right away*/
			break;
		case transfer:
			iob->b_state = (int) drop;
			iob->b_xcount = bp->b_resid;
			iob->b_xaddr = bp->b_un.b_addr;	/* get from user land */
			if (iob->activity_timeout > 0)
				START_TIME(gpio_timeout, iob->activity_timeout);
			gpio_driver(bp);
			break;
		case drop:
			bp->b_resid = bp->b_sc->resid;
			iob->term_reason = bp->b_sc->tfr_control;
			iob->b_state = (int) defaul;
			dil_drop_selcode(bp);
			END_TIME
			queuedone(bp);
			break;

		case timedout:
			escape(TIMED_OUT);

		default:
			panic("GPIO: bad transfer state\n");
		} /* switch */
		END_FSM
	recover {
		iob->b_state = (int) defaul;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		gpio_abort(bp);
		iob->term_reason = TR_ABNORMAL;
		dil_drop_selcode(bp);
		queuedone(bp);
		dil_dequeue(bp);
	}
}


gpio_timeout(bp)
struct buf *bp;
{
	TIMEOUT_BODY(bp->b_queue->intloc, gpio_transfer,
			bp->b_sc->int_lvl, 0, timedout);
}

/*
**	This routine is responsible for the following:
**		1) selecting the type of transfer
**		2) kicking off the transfer
*/
gpio_driver(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	register struct isc_table_type *sc = bp->b_sc;
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;
	register int state = iob->dil_state;
	register int temp;

	if (!(cp->p_status & GP_PSTS))
	{	sc->tfr_control = TR_END;
		sc->resid = bp->b_bcount;
		gpio_transfer(bp);
		return;
	}
	/* set up other control info here */
	sc->tfr_control = state;	/* transfer info */

	/*
	**  For speed during interrupt transfers the following fields
	**  are used:
	**	sc->intcopy  => iob->dil_state
	**	sc->intmsksav  => gp->termination_pattern
	**	sc->intmskcpy  => bp->b_flags
	*/
	sc->intcopy = iob->dil_state;
	sc->intmsksav = iob->read_pattern;
	sc->intmskcpy = bp->b_flags;

	/* selcect type of transfer */
	if ((bp->b_bcount == 1) ||
	    ((state & D_16_BIT) && (bp->b_bcount == 2)) ||
	    (state & READ_PATTERN) ||
	    (state & USE_INTR) ||
	    (! (try_dma(bp, MAX_SPEED))))  
		sc->transfer = INTR_TFR;	/* do interrupt transfer */
	else
		sc->transfer = DMA_TFR;		/* Otherwise do dma */

	sc->buffer = bp->b_queue->b_xaddr;
	sc->count = bp->b_queue->b_xcount;
	switch(sc->transfer) {
		case DMA_TFR:
			if (iob->dil_state & D_16_BIT) {
				sc->state |= F_WORD_DMA;	/* 16 bit */
				sc->buffer = (caddr_t)((int)sc->buffer + sc->count - 2);
			}
			else {
				sc->state &= ~F_WORD_DMA;	/* 8 bit */
				sc->buffer = (caddr_t)((int)sc->buffer + sc->count - 1);
			}
			sc->count = 1;

			if (cp->intr_control & GP_BURST)
				sc->state |= F_PRIORITY_DMA;	/* do burst */
			else
				sc->state &= ~F_PRIORITY_DMA;

			gpio_dma(bp);
			break;
		case INTR_TFR:
			gpio_intr(bp);
			break;
		case FHS_TFR:
		case BURST_TFR:
		case NO_TFR:
		default:
			panic("GPIO: transfer of unknown type\n");
	}
}


/*
**	do a dma transfer
**
**	assume:
**		- selectcode already owned (by bp)
**		- dma channel already owned (by sc)
**		- transfer chain set up for 8 or 16 bit transfers
*/
gpio_dma(bp)
register struct buf *bp;
{
	struct isc_table_type *sc = bp->b_sc;
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;
	register struct dma_channel *dma = sc->dma_chan;
	register unsigned char chan_flag;
	register struct dma_chain *chain;
	struct iobuf *iob = bp->b_queue;
	register char temp;

	dma_build_chain(bp);

	bp->b_action = gpio_transfer;		/* who to call */

	cp->intr_mask = GP_EIR1;	/* disable ready interrupt */

	/* we want the dmacard to call us when it is finished */
	dma->isr = gpio_do_isr;
	dma->arg = (int) sc;

	/* prepare the gpio card for an input transfer */
	if (bp->b_flags & B_READ) {
		temp = cp->data.byte.lower;	/* do a read */
		cp->set_pctl = temp;		/* set pctl on card */
	}
	/* start the dma transfer */
	dma_start(dma);
}


gpio_intr(bp)
register struct buf *bp;
{
	register char temp;	/* used for kicking card */
	register struct isc_table_type *sc = bp->b_sc;
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;

	bp->b_action = gpio_transfer;		/* who to call */
	if (sc->tfr_control & D_16_BIT) /* we have a 16 bit transfer */
		sc->count = sc->count / 2;	/* adjust count */

	cp->intr_mask = GP_RDYEN1 | GP_EIR1;	/* enable on ready */

	if (bp->b_flags & B_READ) {
		temp = cp->data.byte.lower;	/* set IO line */
		cp->set_pctl = temp;		/* set PCTL */
	}
	cp->intr_control = GP_ENAB;	/* enable card */
}


gpio_intr_isr(sc, cp, ext_intr)
register struct isc_table_type *sc;
register struct GPIO *cp;
int ext_intr;
{
	char temp;
	register unsigned char control;
	register unsigned short data = 0;
	register unsigned short status;

	if (ext_intr) {		/* terminate the transfer */
		gpio_resid(sc);		/* find out how much is left */
		cp->intr_mask = 0;	/* disable interrupts */
		cp->intr_control = 0;
		sc->tfr_control = TR_END;
		sc->transfer = NO_TFR;
		gpio_reset(sc);		/* now clear the EIR line */
		return 0;
	}
	status = sc->intcopy;		/* for fast access */
	control = sc->tfr_control;	/* for fast access */

	if (sc->intmskcpy & B_READ) {	/* input transfers */
		if (status & D_16_BIT)	{ /* word transfers */
			data = cp->data.word;
			*(unsigned short *)sc->buffer = data;
			sc->buffer += 2;
		}
		else {	/* byte transfer */
			data = cp->data.byte.lower;
			*sc->buffer++ = data;
		}
		if (control & READ_PATTERN) {
			if (data == sc->intmsksav) {
				sc->tfr_control = TR_MATCH;
				sc->resid = --sc->count;
				if (status & D_16_BIT)
					sc->resid += sc->resid;
				if (sc->resid == 0)
					sc->tfr_control |= TR_COUNT;
				cp->intr_mask = 0;	/* disable interrupts */
				cp->intr_control = 0;
				sc->transfer = NO_TFR;
				return 0;
			}
		}
		if (--sc->count) {	/* still more to go */
			cp->set_pctl = temp;	/* set PCTL */
			return 1;	/* keep going */
		}
		else {	/* finished */
			/* don't want to reset PCTL now */
			gpio_resid(sc);		/* find out how much is left */
			cp->intr_mask = 0;	/* disable interrupts */
			cp->intr_control = 0;
			sc->tfr_control = TR_COUNT;
			sc->transfer = NO_TFR;
			return 0;
		}
	}
	else {	/* output transfers */
		if (status & D_16_BIT) {/* word transfers */
			cp->data.word = *(unsigned short *)sc->buffer;
			sc->buffer += 2;
		}
		else	/* byte transfer */
			cp->data.byte.lower = *sc->buffer++;
		if (--sc->count) {	/* still more to go */
			cp->set_pctl = temp;
			return 1;	/* keep going */
		}
		else {	/* finished */
			gpio_resid(sc);		/* find out how much is left */
			cp->intr_mask = 0;	/* disable interrupts */
			cp->intr_control = 0;
			sc->tfr_control = TR_COUNT;
			sc->transfer = NO_TFR;
			cp->set_pctl = temp;	/* set PCTL for last byte */
			return 0;
		}
	}
}


gpio_dma_isr(sc, cp, ext_intr)
register struct isc_table_type *sc;
register struct GPIO *cp;
register int ext_intr;
{
	register unsigned char chan_flag;

	dmaend_isr(sc);		/* clear up dma channel */
	cp->intr_control &= ~(GP_DMA0 | GP_DMA1);  /* disable DMA for GPIO */
	drop_dma(sc->owner);	/* get rid of channel */

	if (ext_intr) {		/* terminate the transfer */
		if (cp->intr_control & GP_WORD) 	/* adjust resid from dmaend */
			sc->resid += sc->resid;		/* double it */
		cp->intr_mask = 0;	/* disable interrupts */
		cp->intr_control = 0;
		sc->tfr_control = TR_END;
		sc->transfer = NO_TFR;
		gpio_reset(sc);		/* now clear the EIR lint */
		return 0;
	}
	if (sc->owner->b_flags & B_READ) {	/* input transfer */
		/* we must do interrupt transfer for the last byte */
		cp->intr_control = 0;	/* turn off for now */
		sc->transfer = INTR_TFR;
		cp->intr_mask = GP_RDYEN1 | GP_EIR1;	/* enable on ready */
		cp->intr_control = GP_ENAB;	/* enable the card */
		return 1;	/* finish last next isr */
	}
	sc->transfer = NO_TFR;
	if (cp->intr_control & GP_WORD) 	/* adjust resid from dmaend */
		sc->resid += sc->resid;		/* double it */
	sc->tfr_control = TR_COUNT;
	cp->intr_control = 0;	/* disable gpio from dma */
	return 0;	/* yes we are finished */
}


gpio_isr(inf)
struct interrupt *inf;
{
	register struct isc_table_type *sc = (struct isc_table_type *)inf->temp;

	gpio_do_isr(sc);
}


gpio_do_isr(sc)
register struct isc_table_type *sc;
{
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;
	int i;
	register int ext_int = 0;

	if (cp->p_status & GP_INT_EIR)	/* external interrupt */
		ext_int= 1;

	switch(sc->transfer) {
		case DMA_TFR:
				if (gpio_dma_isr(sc, cp, ext_int)) return 1;
				(sc->owner->b_action)(sc->owner);
				break;
		case INTR_TFR:
				if (gpio_intr_isr(sc, cp, ext_int)) return 1;
				(sc->owner->b_action)(sc->owner);
				break;
		case NO_TFR:
				break;
		case FHS_TFR:
		case BURST_TFR:
		default:
			panic("Unrecognized GPIO transfer type\n");
	}
	/* interrupts should be off now.  Only active during transfer */

	do {
		i = selcode_dequeue(sc);
		i += dma_dequeue();
	}
	while (i);
	return 1;
}


/* this is used by any transfer routine other than DMA */
gpio_resid(sc)
register struct isc_table_type *sc;
{
	register struct GPIO *cp = (struct GPIO *)sc->card_ptr;
	register int count = 0;

	sc->resid = sc->count;
	if (sc->tfr_control & D_16_BIT)
		sc->resid += sc->resid;
}


gpio_init(sc)
register struct isc_table_type *sc;
{
	struct GPIO *cp = (struct GPIO *)sc->card_ptr;

	gpio_reset(sc);

	isrlink(gpio_isr, sc->int_lvl, &cp->intr_control, 0xc0, 0xc0,
								0, (int)sc);
}


gpio_reset(sc)
register struct isc_table_type *sc;
{
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;

	cp->reset_gpio = 1;
	snooze(15);	/* wait 15 usecs */
}


gpio_abort(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register struct GPIO *cp = (struct GPIO *) sc->card_ptr;
	register int i;

	cp->intr_mask = 0;		/* disable interrupts */
	cp->intr_control = 0;
	if (sc->transfer == DMA_TFR) 
	{	dmaend_isr(sc);
		drop_dma(sc->owner);
	}
	sc->transfer = NO_TFR;
	gpio_reset(bp->b_sc);		/* now clear the EIR line */
}


/*
** linking and inititialization routines
*/

struct drv_table_type gpio_iosw = {
	gpio_init,
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
	(int (*)())NULL
};

extern int (*make_entry)();

int (*gpio_saved_make_entry)();


gpio_make_entry(id, isc)
int id;
struct isc_table_type *isc;
{
	if (id != 3)
		return (*gpio_saved_make_entry)(id, isc);

	isc->iosw = &gpio_iosw;
	isc->card_type = HP98622;
	return io_inform("HP98622", isc, 3);
}


gpio_link()
{
	gpio_saved_make_entry = make_entry;
	make_entry = gpio_make_entry;
}

