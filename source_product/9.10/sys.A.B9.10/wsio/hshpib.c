/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/hshpib.c,v $
 * $Revision: 1.3.83.8 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/12/22 13:20:40 $
 */
/* HPUX_ID: @(#)hshpib.c	55.1		90/10/22 */

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
** HPIB_MED (HP98625/HP25560 card) High speed HPIB interface driver
*/
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/malloc.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../wsio/eisa.h"
#include "../wsio/hshpib.h"
#include "../h/user.h"
#include "../h/proc.h"
#ifdef __hp9000s700
#include "../wsio/dma_s700.h"
#else
#include "../s200io/dma.h"
#endif
#include "../wsio/intrpt.h"
#include "../wsio/iobuf.h"
#include "../wsio/tryrec.h"
#include "../wsio/dil.h"
#include "../wsio/hpib.h"
#include "../wsio/dilio.h"

#ifdef __hp9000s700
#include "../wsio/diag1.h"
#include "../wsio/iodiag1.h"
#endif

#ifdef __hp9000s300
#define MAXPHYS (64 * 1024)
#endif

/* simon defines for "Hidden Mode" registers */

#define sim_switches sim_id

#define S_INT_LEVEL             0x30
#define S_SYSTEM_CTLR           0x08
#define S_SPEED                 0x04
#define S_ENABLE_SIMONC         0x02
#define S_ENABLE_DMA32          0x01

struct isc_table_type *hshpib_selectcodes[HSHPIB_MAX_CARDS];
int want_hshpib_poll = 0;
int number_of_hshpib;

#ifdef __hp9000s700
struct python_data python_data[HSHPIB_MAX_CARDS];
#endif /* __hp9000s700 */

#define	MED_INTR	0
#define	MED_IMSK	1
#define	MED_FIFO	2
#define	MED_STATUS	3
#define	MED_CTRL	4
#define	MED_ADDRESS	5
#define	MED_PPMSK	6
#define	MED_PPSNS	7

static unsigned short simon_med_regoff[NUM_MED_REGS]={0x11,0x13,0x15,0x17,
						     0x19,0x1b,0x1d,0x1f};

static unsigned short python_med_regoff[NUM_MED_REGS]={0x400,0x401,0x402,0x403,
						     0x404,0x405,0x406,0x407};

#define HSHPIB_ENABLE_INTRPT(sc) \
{\
if((sc)->card_type==HP98625){\
	register HSHPIB *cardp=(HSHPIB *)(sc)->card_ptr;\
	cardp->s.sim_ctrl = S_ENAB;\
}\
else if(sc->card_type==HP25560)	{\
	register HSHPIB *cardp=(HSHPIB *)(sc)->if_reg_ptr;\
	cardp->p.med_intr = 0x40; /*clear M_PROC_HSK_ABORT*/\
	cardp->p.pyt_intrcfg |= P_INTR_ENB;\
}\
}

#define HSHPIB_DISABLE_INTRPT(sc) \
{\
if((sc)->card_type==HP98625){\
	register HSHPIB *cardp=(HSHPIB *)sc->card_ptr;\
	cardp->s.sim_ctrl = ZERO;\
}\
else if((sc)->card_type==HP25560)	{\
	register HSHPIB *cardp=(HSHPIB *)(sc)->if_reg_ptr;\
	cardp->p.med_intr=0x40;/*clear M_PROC_HSK_ABORT*/\
	cardp->p.pyt_intrcfg &= ~P_INTR_ENB;\
}\
}


/******************************hshpib_reg_access******************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (IO_REG_ACCESS) via dil_utility (dil_action)
**		via enqueue (b_action)
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_reg_access(bp)
register struct buf *bp;
{
	bp->b_error = EINVAL;
	bp->b_flags |= B_ERROR;
}

int ZERO = 0;	/* used to keep compiler from generating clrs */


/******************************routine name******************************
** [short description of routine]
** Set the High speed HPIB card to a known reset state
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
do_hshpib_ifc(sc)
register struct isc_table_type *sc;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ?
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_ctrl = &cp->r[regoff[MED_CTRL]];

	*med_ctrl ^= (M_IFC | M_REN);
	*med_ctrl |= M_INIT_FIFO;/*only safe while IFC assertd*/
	snooze(100);		/* delay for bus protocol: 100 micros */
	*med_ctrl ^= (M_IFC | M_REN);
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
do_hshpib_reset(sc)
register struct isc_table_type *sc;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	volatile register unsigned char *med_ctrl = &cp->r[regoff[MED_CTRL]];
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_address = &cp->r[regoff[MED_ADDRESS]];

	if(sc->card_type==HP98625) {
		cp->s.sim_reset = 0x80;	/* reset Simon and the PHI/ABI */
	}
	else if(sc->card_type==HP25560)	{
		register struct python_data *if_data=
			(struct python_data *)sc->if_drv_data;
		cp->p.eisa_ctrl=E_IOCHKRST; /* reset EISA HPIB */
		snooze(1); /* wait at least 500ns for proper reset */
		cp->p.eisa_ctrl=ZERO;

	    	/* Restore the python card's power up/EEPROM stuff */
	    	cp->p.pyt_cardcfg = if_data->pyt_cardcfg;
	    	cp->p.pyt_romcfg = if_data->pyt_romcfg;
	    	cp->p.pyt_dmacfg = if_data->pyt_dmacfg;
	    	cp->p.pyt_intrcfg = if_data->pyt_intrcfg;
	    	cp->p.med_imsk = if_data->med_imsk;
	    	cp->p.med_ctrl = if_data->med_ctrl;
	    	cp->p.med_address = if_data->med_address;
	    	sc->my_address = if_data->med_address;
	    	cp->p.med_ppmsk = if_data->med_ppmsk;
	    	cp->p.med_ppsns = if_data->med_ppsns;
	    	cp->p.eisa_ctrl = if_data->eisa_ctrl;
	}

	/* Reset Medusa chip set to powerup state */
	*med_status = ZERO;	/* unnecessary, but in good form */
	*med_ctrl = M_8BIT_PROC | M_REN;
	*med_ctrl = M_8BIT_PROC | M_REN;  /* guarantees high-order bits */

	/* setup standard interrupt mask */
	*med_status = M_INT_ENAB;
	*med_imsk = M_PROC_ABRT | M_POLL_RESP;
	
#ifdef __hp9000s700
	*med_status = M_CRCE|M_EPAR;  /* CRC, parity on ABI only */
#else
	*med_status = M_CRCE;  /* CRC on ABI only */
#endif

	*med_address |= M_ONL | sc->my_address;

	/* only system controllers can do interface clear */
	if(sc->card_type==HP98625) {
		if (*med_status & M_SYST_CTRL) {
			sc->state |= SYSTEM_CONTROLLER;
			do_hshpib_ifc(sc);	/* become active controller */
		}
		if (*med_status & M_HPIB_CTRL)
			sc->state |= ACTIVE_CONTROLLER;
		else
			sc->state &= ~ACTIVE_CONTROLLER;
	}
	else if(sc->card_type==HP25560)	{
		if(cp->p.pyt_cardcfg & P_SYS_CNTRL) {
			*med_status |= M_SYST_CTRL;
			sc->state |= ACTIVE_CONTROLLER|SYSTEM_CONTROLLER;
			do_hshpib_ifc(sc);
		}
		else	{
			sc->state &= ~ACTIVE_CONTROLLER;
		}
	}
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
preserving_enables_do_reset(sc)
register struct isc_table_type *sc;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	register char temp = cp->r[regoff[MED_IMSK]];
	char enabled  = cp->r[regoff[MED_STATUS]] & M_INT_ENAB;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	volatile register unsigned char *med_ppmsk = &cp->r[regoff[MED_PPMSK]];
	volatile register unsigned char *med_ppsns = &cp->r[regoff[MED_PPSNS]];
	char ppmsk = *med_ppmsk;
	char ppsns = *med_ppsns;
	
	do_hshpib_reset(sc);

	*med_ppmsk = ppmsk;
	*med_ppsns = ppsns;
	if (!enabled) {
		temp = *med_imsk;
		*med_status&= ~M_INT_ENAB;
		*med_imsk= temp;
	}
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_init(sc)
register struct isc_table_type *sc;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;

	/* initialize for selectcode locking */
	sc->locking_pid = 0;
	sc->locks_pending = 0;

	if(sc->card_type==HP98625)	{
#ifdef __hp9000s300
		int level = ((cp->s.sim_stat & S_LEVMSK) >> S_LEVSHF) + 3;
		int simon_isr(), clkrupt();
		/*
		** simon doesn't have HPIB bus address switches on the card
		** so we will set the card's bus address to it's select code
		** as a default ....arrrgggg!
		*/
		sc->my_address = sc->my_isc;
	        do_hshpib_reset(sc);

		isrlink(simon_isr, level,
				&cp->s.sim_stat, (S_ENAB | S_PEND), (S_ENAB | S_PEND),
				0, (int)sc);

		hshpib_selectcodes[++number_of_hshpib] = sc;

		/*
		** if simon A, check for dedicated interrupt level (clock excepted)
		*/
		if (sc->state & F_SIMONA) {
			register struct interrupt **rupt = &rupttable[level];
			if ((*rupt)->isr == clkrupt)
				rupt = &(*rupt)->next;
			if ((*rupt)->regaddr != (char *)&cp->s.sim_stat ||
		    	(*rupt)->next != NULL)
				panic("HP98625A card on non-dedicated interrupt level");
		}
		HSHPIB_ENABLE_INTRPT(sc);
#endif /* hp9000s300 */
	}
	else if(sc->card_type==HP25560)	{
		static int irqs[8]={3,4,5,9,10,11,12,15};
		int irq_line=irqs[((struct python_data *)(sc->if_drv_data))->pyt_intrcfg&P_IRQ_MASK];
		int python_isr();
		sc->my_address=((struct python_data *)(sc->if_drv_data))->med_address;
		do_hshpib_reset(sc);
		eisa_isrlink((int)sc, python_isr, irq_line,0);
		HSHPIB_ENABLE_INTRPT(sc);
	}
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:   hshpib_fakeisr via sw_trigger	
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_sw(sc)
register struct isc_table_type *sc;
{
	sc->int_flags |= HSHPIB_FAKEISR;
	hshpib_do_isr(sc);
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_fakeisr(sc)
register struct isc_table_type *sc;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	char imsk = cp->r[regoff[MED_IMSK]];
	int x;

	/* turn off medusa interrupts */
	*med_status &= ~M_INT_ENAB;
	*med_imsk = imsk;

	/* level 7 in case simon A is at level 6 */
	x = spl7();
	sw_trigger(&sc->intloc2, hshpib_sw, sc, sc->int_lvl, 0);
	splx(x);
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_fakeisr_poll()
{
	register struct isc_table_type *sc;
	register HSHPIB *cp;
	register int i;
	unsigned short *regoff; 
	volatile register unsigned char *med_status;

	/* for all select codes:
	**   1) if it is busy then ignore it for now
	**   2) if we are doing a transfer then forget it
	*/
	if (want_hshpib_poll == 0)
		return;
	i = 0;
	do {
		if (((sc = hshpib_selectcodes[i++])->active == 0) && 
		    (sc->transfer == NO_TFR)) {
			cp = (sc->card_type==HP98625) ? 
			(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
			regoff = (sc->card_type==HP98625) ? 
				simon_med_regoff : python_med_regoff;
			med_status = &cp->r[regoff[MED_STATUS]];
			if (*med_status & M_HPIB_CTRL)
				if (sc->intr_wait & (WAIT_CTLR | INTR_CTLR))
					hshpib_fakeisr(sc);
			if (*med_status & 0x04)
				if (sc->intr_wait & (WAIT_TALK | INTR_TALK))
					hshpib_fakeisr(sc);
			if (*med_status & M_LTN)
				if (sc->intr_wait & (WAIT_LISTEN | INTR_LISTEN))
					hshpib_fakeisr(sc);
		}
	} while (i <= number_of_hshpib);
	timeout(hshpib_fakeisr_poll, 1, 1, NULL);
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_fakeisr_check()
{
	register struct isc_table_type *isc;
	register int i;
	register int still_polling = 0;

	for (i = 0; i <= number_of_hshpib; i++) {
		isc = hshpib_selectcodes[i];
		if (isc->intr_wait & (WAIT_TALK | INTR_TALK | 
                                      WAIT_LISTEN | INTR_LISTEN |
				      WAIT_CTLR | INTR_CTLR))
			still_polling = 1;
	}
	if ( ! still_polling)
		want_hshpib_poll = 0;
}

/******************************python_isr******************************
** [short description of routine]
**
**      Called by:   interrupt service
**
**      Parameters:     [description, if needed]
**
**      Return value:
**
**      Kernel routines called:
**
**      Global variables/fields referenced:
**
**      Global vars/fields modified:
**
**      Registers modified:
**
**      *** comment local variable usage ***
**
**      Algorithm:  [?? if needed]
************************************************************************/
python_isr(sc,arg)
struct isc_table_type *sc;
int arg;        /* not used */
{
        register HSHPIB *cp = (HSHPIB *)sc->if_reg_ptr;
        register char tmp;

	/* Better make sure this int is for us!!!!!!!!!! */
	if(!(sc->card_type==HP25560))
		return(FALSE);

	tmp=cp->p.pyt_intrcfg;
	if(!(tmp&(P_MED_INTR|P_DMA_INTR|P_GET_INTR|P_IFC_INTR)&&(tmp&P_INTR_ENB)))
		return(FALSE);

	cp->p.pyt_intrcfg &= ~P_INTR_ENB; /* disable further ints */

        sc->intcopy = cp->p.med_status;
        tmp = cp->p.med_ctrl | M_8BIT_PROC;
        cp->p.med_status = ZERO;
        cp->p.med_ctrl = tmp;

        /* disable interrupt */
        tmp = cp->p.med_imsk;
        cp->p.med_status &= ~M_INT_ENAB;
        cp->p.med_imsk = tmp;

        /* Simon driver used to call sw_trigger to adjust interrupt level
         * here, but the int_level is always 5 for EISA cards.
         */
        hshpib_do_isr(sc);
}

/*
**  Note: Simon_isr is now an assembly language routine.  There are two reasons:
**
**	1)  Because of simon's outrageous limitation that it mustn't be touched
**	    by the CPU during a dma output transfer until "simon says", dma
**	    chaining MUST be performed by SIMON's isr, as opposed to a dma isr.
**	    The assembly language routine simon_isr will do this chaining if
**	    appropriate, or it will "call" the C language routine simon_C_isr.
**
**	2)  To still allow internal support for MAC discs, simon may now be
**	    placed on either level 4 OR level 6.  Level 4 is the supported
**	    configuration for customers, and has the desirable characteristic
**	    that dma re-arming occurs only after level 6 & 5 interrupts have
**	    been serviced, resulting in fewer dropped characters with serial
**	    cards.  However, since this behavior would completely blow away
**	    the MAC discs, simon CAN be put at level 6, resulting in chaining
**	    behavior comparable to re-arming it at level 7, as done previously.
**	    However, normal (non-chaining) isr activity should still occur at
**	    level 4, so if the hardware interrupt level is 6, simon_isr will
**	    software trigger simon_C_isr down at level 4, instead of calling
**	    simon_C_isr directly.
**
**
**  Other notes regarding simon_isr:
**
**	1)  Before calling simon_C_isr, simon_isr always clears the M_INT_ENAB
**	    bit in the med_imsk register, so that simon_isr won't be
**	    accidentally re-entered by side-effects when simon_C_isr calls
**	    b_action or HPIB_call_isr.
**	2)  Because of 1), Simon_C_isr must usually set the M_INT_ENAB bit
**	    after it finishes its work.  With DMA transfers, however, we are
**	    once again confronted with the problem of not touching simon in
**	    any way while the (output) transfer is in progress.  For this case,
**	    the routine that initiates the transfer sets the M_INT_ENAB bit
**	    at a safe time, and simon_C_isr leaves it alone.  Re-entrancy 
**	    will not be a problem since all other interrupts are disabled
**	    while the DMA transfer is in progress.
**	3)  As a side-effect of 1), M_8BIT_PROC is always already set when
**	    simon_C_isr is called.
**
*/

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
simon_C_isr(inf)
struct interrupt *inf;
{
	register struct isc_table_type *sc = (struct isc_table_type *)inf->temp;

	hshpib_do_isr(sc);
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_do_isr(sc)
struct isc_table_type *sc;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	register struct buf *cur_buf, *next_buf;
	register struct dil_info *dil_pkt;
	register int completed_transfer;
	register int this_event = 0;
	register int interesting_events = sc->intr_wait;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_intr = &cp->r[regoff[MED_INTR]];
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	int interrupting_condition;

	/* no fakeisr's during transfers */
	if ((sc->transfer != NO_TFR) && (sc->int_flags & HSHPIB_FAKEISR)) {
		sc->int_flags &=  ~HSHPIB_FAKEISR;
		return;
	}

	/*
	** handle any transfer in progess
	*/
	switch(sc->transfer) {
	case FHS_TFR:
		completed_transfer = hshpib_intr_isr(sc);
		break;
	case INTR_TFR:
		completed_transfer = hshpib_pattern_isr(sc);
		break;
	case DMA_TFR:
#ifdef __hp9000s300
		if(sc->card_type==HP98625)
			dmaend_isr(sc);      /* clear up the DMA channel */
#endif /* __hp9000s300 */
		completed_transfer = hshpib_dmaisr(sc);
		break;
	case BO_END_TFR:
		if (!(*med_intr & M_FIFO_IDLE))
			panic("hshpib_C_isr - BO_END_TFR");
		completed_transfer = TRUE;
		break;
	case NO_TFR:
		completed_transfer = FALSE;
		break;
	default:
		panic("hshpib_C_isr - sc->transfer");
	}

	/*
	** Check for a completed transfer.
	*/
#ifdef __hp9000s700
	if (completed_transfer) {
		*med_imsk &= ZERO;
		sc->transfer = NO_TFR;
		*med_imsk|= M_PROC_ABRT | M_POLL_RESP | M_SERV_RQST | M_DEV_CLR;
		(*sc->owner->b_action)(sc->owner);
	}
	HSHPIB_ENABLE_INTRPT(sc);
#else
	if (completed_transfer) {
		*med_imsk &= ZERO;
		*med_imsk |= M_PROC_ABRT | M_POLL_RESP | M_SERV_RQST | M_DEV_CLR;
	        HSHPIB_ENABLE_INTRPT(sc);
		sc->transfer = NO_TFR;
		(*sc->owner->b_action)(sc->owner);
	}
#endif  /* hp9000s700 */

	/*
	** check for an interrupting condition; not possible during a transfer
	*/
	if (sc->transfer == NO_TFR) {
		sc->int_flags &=  ~HSHPIB_FAKEISR;
		if (*med_intr & M_PROC_ABRT)
			panic("hshpib_C_isr - M_PROC_ABRT");

		if (*med_status & M_HPIB_CTRL) {
			if (sc->intr_wait & WAIT_CTLR) {
				sc->int_flags |= INT_CTLR;
				sc->intr_wait &= ~WAIT_CTLR;
			}
			if (sc->intr_wait & INTR_CTLR) {
				this_event |= INTR_CTLR;
				sc->intr_wait &= ~INTR_CTLR;
			}
		}

		if (*med_status & M_TLK) {
			if (sc->intr_wait & WAIT_TALK) {
				sc->int_flags |= INT_TADS;
				sc->intr_wait &= ~WAIT_TALK;
			}
			if (sc->intr_wait & INTR_TALK) {
				this_event |= INTR_TALK;
				sc->intr_wait &= ~INTR_TALK;
			}
		}

		if (*med_status & M_LTN) {
			if (sc->intr_wait & WAIT_LISTEN) {
				sc->int_flags |= INT_LADS;
				sc->intr_wait &= ~WAIT_LISTEN;
			}
			if (sc->intr_wait & INTR_LISTEN) {
				this_event |= INTR_LISTEN;
				sc->intr_wait &= ~INTR_LISTEN;
			}
		}

		/* did we have an srq interrupt? */
		if (*med_intr & M_SERV_RQST) {
			if (interesting_events & WAIT_SRQ) {
				sc->int_flags |= INT_SRQ;
				sc->intr_wait &= ~WAIT_SRQ;
			}
			if (interesting_events & INTR_SRQ) {
				this_event |= INTR_SRQ;
				sc->intr_wait &= ~INTR_SRQ;
			}
			*med_imsk &= ~M_SERV_RQST;
		}

		/* did we have a device clear interrupt? */
		if (*med_intr & M_DEV_CLR) {
			if (interesting_events & INTR_DCL) {
				this_event |= INTR_DCL;
				sc->intr_wait &= ~INTR_DCL;
			}
			*med_intr |= M_DEV_CLR;
			*med_imsk &= ~M_DEV_CLR;
		}

		if (this_event)
			for (cur_buf = sc->event_f; cur_buf != NULL; cur_buf = next_buf) {
				dil_pkt = (struct dil_info *) cur_buf->dil_packet;
				next_buf = dil_pkt->dil_forw;
				if (this_event & dil_pkt->intr_wait) {
					dil_pkt->event |= this_event & dil_pkt->intr_wait;
					dil_pkt->intr_wait &= ~this_event;
					deliver_dil_interrupt(cur_buf);
				}
			}
		
		hshpib_fakeisr_check();

		if (*med_intr & M_POLL_RESP)
			sc->int_flags |= INT_PPL;
		if (interrupting_condition = sc->int_flags)
			HPIB_call_isr(sc);
	} else
		interrupting_condition = FALSE;

	/*
	** dequeue if appropriate
	*/
	if (completed_transfer || interrupting_condition)
		do ;
		while (selcode_dequeue(sc) 
#ifdef __hp9000s300
		| dma_dequeue()
#endif /* __hp9000s300 */
		);


	/*
	** re-enable medusa interrupts
	*/
	if (sc->transfer != DMA_TFR) {
		char imsk = *med_imsk;
		*med_status |= M_INT_ENAB;
		*med_imsk = imsk;
	}
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_clear_wopp(bp)		/* clear without ppoll */
struct buf *bp;
{
	hshpib_clear(bp);
	return 0;	/* no ppoll */
}

#ifdef __hp9000s700
#define MEDUSA_WAIT(x) while((cp->r[regoff[MED_INTR]]&(x))==0) { \
				if(bp->b_queue->timeflag)	{ \
					escape(TIMED_OUT); \
				} \
		       } 
#else
#define MEDUSA_WAIT(x) while((cp->r[regoff[MED_INTR]]&(x))==0)
#endif


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_clear(bp)
struct buf *bp;
{
	register HSHPIB *cp = (bp->b_sc->card_type==HP98625) ? 
		(HSHPIB *)bp->b_sc->card_ptr : (HSHPIB *)bp->b_sc->if_reg_ptr;
	unsigned short *regoff =  (bp->b_sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];

	/* any ppolls/srq's are ignored till done */

	START_POLL;
	HSHPIB_DISABLE_INTRPT(bp->b_sc);
	*med_imsk |= M_FIFO_IDLE;

	MEDUSA_WAIT(M_FIFO_IDLE);	/* its now idle, 8 bytes can go out */
	*med_status = M_FIFO_ATN;
	*med_fifo = UNL;			/* 1 */
	*med_fifo = TAG_BASE + MA;		/* 2 */
	*med_fifo = LAG_BASE + bp->b_ba;	/* 3 */
	*med_fifo = SCG_BASE + 16;		/* 4 */
	*med_status = M_FIFO_EOI;
	*med_fifo = ZERO;			/* 5 */
	*med_status = M_FIFO_ATN;
	*med_fifo = SDC;			/* 6 */
	*med_fifo = UNL;			/* 7 */

	MEDUSA_WAIT(M_FIFO_IDLE);
	*med_imsk &= ~M_FIFO_IDLE;
	HSHPIB_ENABLE_INTRPT(bp->b_sc);
	END_POLL;

	return 1; /* yes, I want a parallel poll */
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_identify(bp)
struct buf *bp;
{
	register HSHPIB *cp = (bp->b_sc->card_type==HP98625) ? 
		(HSHPIB *)bp->b_sc->card_ptr : (HSHPIB *)bp->b_sc->if_reg_ptr;
	struct iobuf *iob = bp->b_queue;
	unsigned short *regoff = (bp->b_sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];

	START_POLL;
	HSHPIB_DISABLE_INTRPT(bp->b_sc);
	*med_imsk |= (M_FIFO_BYTE | M_FIFO_IDLE);

	MEDUSA_WAIT(M_FIFO_IDLE);	/* its idle, up to 8 bytes out */
	*med_status = M_FIFO_ATN;
	*med_fifo = UNL;			/* 1 */
	*med_fifo = LAG_BASE + MA;		/* 2 */
	*med_fifo = TAG_BASE + 31;		/* 3 */
	*med_fifo = SCG_BASE + bp->b_ba;	/* 4 */

	/* set up a 2-byte counted transfer, no lf sensitivity */
	*med_status = M_FIFO_LF_INH;
	*med_fifo = 2;

	MEDUSA_WAIT(M_FIFO_BYTE);
	if ((iob->b_xaddr[0] = *med_fifo), *med_status & M_FIFO_EOI) {
		END_POLL;
		preserving_enables_do_reset(bp->b_sc);
		HSHPIB_ENABLE_INTRPT(bp->b_sc);
		bp->b_error = EIO;
		escape(1);
	}

	MEDUSA_WAIT(M_FIFO_BYTE);
	iob->b_xaddr[1] = *med_fifo;

	/* fifo must be empty to get here */
	*med_status = M_FIFO_ATN;
	*med_fifo = TAG_BASE + MA;

	MEDUSA_WAIT(M_FIFO_IDLE);
	*med_imsk &= ~(M_FIFO_BYTE | M_FIFO_IDLE);
	HSHPIB_ENABLE_INTRPT(bp->b_sc);
	END_POLL;

	return 0; /* forget the parallel poll */
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_ppoll(sc)
struct isc_table_type *sc;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;

	return cp->r[regoff[MED_FIFO]];
}


/******************************hshpib_spoll******************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_SPOLL)  via dil_util (dil_action) 
**		via enqueue (b_action)
**
**	Parameters:	bp 
**
**	Return value:
**
**	Global variables/fields referenced:  bp->b_sc, b_sc->card_ptr
**
**	Global vars/fields modified:  bp->return_data, sc->state, 
**		sc->spoll_byte
**
**	Registers modified:  sim_ctrl, med_imsk, med_status, med_fifo
**
**  Comment:
**	TI9914_spoll only needs bp to pass to TI9914_bo (asm routine),
**	is it needed?  Can we get rid of bp as a parameter to this 
**	routine?
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_spoll(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];

	START_POLL;
	HSHPIB_DISABLE_INTRPT(sc);
	*med_imsk |= (M_FIFO_BYTE | M_FIFO_IDLE);
	MEDUSA_WAIT(M_FIFO_IDLE);	/* room for 8 bytes after this */
	*med_status = M_FIFO_ATN;
	*med_fifo = UNL;			/* 1 */
	*med_fifo = LAG_BASE + MA;		/* 2 */
	*med_fifo = TAG_BASE + bp->b_ba;	/* 3 */
	*med_fifo = SPE;			/* 4 */
	*med_status = M_FIFO_LF_INH;
	*med_fifo = 1;			/* 5 */
	MEDUSA_WAIT(M_FIFO_BYTE);
	sc->spoll_byte = *med_fifo;
	*med_status = M_FIFO_ATN;
	*med_fifo = SPD;
	*med_fifo = UNT;
	MEDUSA_WAIT(M_FIFO_IDLE);
	*med_imsk &= ~(M_FIFO_BYTE | M_FIFO_IDLE);
	HSHPIB_ENABLE_INTRPT(sc);
	END_POLL;

	bp->return_data = sc->spoll_byte;
	return (sc->spoll_byte & RSV_MASK) != 0;

}

/******************************routine name******************************
** [short description of routine]
**	   send/receive one message consisting of an unlisten,
**	   bus addressing, a secondary (optional), a message sent
**	   or received, and an untalk/unlisten
**	   The preamble and postamble to this routine are also
**	   available as separate routines so that a transfer could
**	   be jammed in the middle.
**
**	Called by:  
**
**	Parameters:	
**	   type: contains 2 flags: read/write (write is set, read is off)
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
**	   EOI: on output: send last byte with EOI
**		on input: EOI is acceptable as termination of message.
**			  (if not, an escape occurs with EIO set)
************************************************************************/
hshpib_mesg(bp, type, sec, bfr, count)
register struct buf *bp;
char type, sec;
register char *bfr;
register short count;
{
	register HSHPIB *cp = (bp->b_sc->card_type==HP98625) ? 
		(HSHPIB *)bp->b_sc->card_ptr : (HSHPIB *)bp->b_sc->if_reg_ptr;
	register int temp;
	unsigned short *regoff = (bp->b_sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];

	START_POLL;
	/* enable fifo room and byte flags in interrupt reg (but no ints) */
	/* any ppolls/srq's are ignored till done */
	HSHPIB_DISABLE_INTRPT(bp->b_sc);
	*med_imsk |= (M_FIFO_ROOM | M_FIFO_BYTE | M_FIFO_IDLE);

	MEDUSA_WAIT(M_FIFO_IDLE);	/* Room for up to 8 bytes */
	*med_status = M_FIFO_ATN;
	*med_fifo = UNL;		/* 1 */

	if (type & T_WRITE) {
		*med_fifo = TAG_BASE + MA;		/* 2 */
		*med_fifo = LAG_BASE + bp->b_ba;	/* 3 */
	} else {
		*med_fifo = LAG_BASE + MA;		/* 2 */
		*med_fifo = TAG_BASE + bp->b_ba;	/* 3 */
	}
	if (sec)
		*med_fifo = sec;			/* 4 */

	if (count > 0) {
		if (type & T_WRITE) {
			temp = count-1;
			while (temp--) {
				MEDUSA_WAIT(M_FIFO_ROOM);
				*med_status = ZERO;
				*med_fifo = *bfr++;
			}
			MEDUSA_WAIT(M_FIFO_ROOM);
			*med_status = (type & T_EOI) ? M_FIFO_EOI : 0;
			*med_fifo = *bfr++;
		} else {
			/* set up counted transfer, no lf sensitivity */
			*med_status = M_FIFO_LF_INH;
			*med_fifo = count;
			temp = count;	
			for(;;) {
				MEDUSA_WAIT(M_FIFO_BYTE);
				*bfr++ = *med_fifo;

				if (--temp == 0) break;

				if (*med_status & M_FIFO_EOI) {
					if (type & T_EOI) break;
					else {
						END_POLL;
						bp->b_error = EIO;
						escape(1);
					}
				}
			};
			count = count-temp;   /* return count left unused */
		}
	}

	MEDUSA_WAIT(M_FIFO_ROOM);
	*med_status = M_FIFO_ATN;
	if (type & T_WRITE) {
		if ((sec&0177) == 0162) {
			END_POLL;
			snooze(150);
			/* this is a workaround for the DMD CS80 transparent
			   message bug.  Symptom: occasional message length
			   errors on transparent messages for no good reason.
			   This may be fixed on newer discs.
			*/
			START_POLL;
		}
		*med_fifo = UNL;
	} else {
		*med_fifo = UNT;
	}

	MEDUSA_WAIT(M_FIFO_IDLE);
	*med_imsk &= ~(M_FIFO_ROOM | M_FIFO_BYTE | M_FIFO_IDLE);
	HSHPIB_ENABLE_INTRPT(bp->b_sc);
	END_POLL;
	return count;
}

/******************************routine name******************************
** [short description of routine]
**	   send a message preamble consisting of an unlisten,
**	   bus addressing, and a secondary (optional)
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_preamb(bp, sec)
register struct buf *bp;
char sec;
{
	register HSHPIB *cp = (bp->b_sc->card_type==HP98625) ? 
		(HSHPIB *)bp->b_sc->card_ptr : (HSHPIB *)bp->b_sc->if_reg_ptr;
	unsigned short *regoff = (bp->b_sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	
	START_POLL;
	/* any ppolls/srq's are ignored till done */

	HSHPIB_DISABLE_INTRPT(bp->b_sc);
	*med_imsk |= M_FIFO_IDLE;

	MEDUSA_WAIT(M_FIFO_IDLE);	/* room for 8 bytes after this */
	*med_status = M_FIFO_ATN;
	*med_fifo = UNL;				/* 1 */

	if (bp->b_flags & B_READ) {
		*med_fifo = LAG_BASE + MA;		/* 2 */
		*med_fifo = TAG_BASE + bp->b_ba;	/* 3 */
	} else {
		*med_fifo = TAG_BASE + MA;		/* 2 */
		*med_fifo = LAG_BASE + bp->b_ba;	/* 3 */
	}

	if (sec)
		*med_fifo = sec;			/* 4 */

	MEDUSA_WAIT(M_FIFO_IDLE);
	*med_imsk &= ~M_FIFO_IDLE;
	hshpib_save_state(bp->b_sc);
	END_POLL;
	/* leave with chip interrupts enabled; card interrupts disabled */
}

/******************************routine name******************************
** [short description of routine]
**	   send a message preamble consisting of an unlisten
**	   and bus addressing.
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_preamb_intr(bp)
register struct buf *bp;
{
	hshpib_preamb(bp, 0);
}

/******************************routine name******************************
** [short description of routine]
**	   send an untalk/unlisten at the end of a message.
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_postamb(bp)
register struct buf *bp;
{
	register HSHPIB *cp = (bp->b_sc->card_type==HP98625) ? 
		(HSHPIB *)bp->b_sc->card_ptr : (HSHPIB *)bp->b_sc->if_reg_ptr;
	unsigned short *regoff =  (bp->b_sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];

	START_POLL;
	HSHPIB_DISABLE_INTRPT(bp->b_sc);
	*med_imsk |= M_FIFO_ROOM | M_FIFO_IDLE;
	 
	MEDUSA_WAIT(M_FIFO_ROOM);
	*med_status = M_FIFO_ATN;
	*med_fifo = (bp->b_flags & B_READ) ? UNT : UNL;
	MEDUSA_WAIT(M_FIFO_IDLE);  /* wait for the byte to be handshaked out */

	/* restore normal interrupt conditions */
	hshpib_restore_state(bp->b_sc);
	END_POLL;
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_transfer(transfer_request, bp, proc)
enum transfer_request_type transfer_request;
register struct buf *bp;
int (*proc)();
{
	register struct isc_table_type *sc = bp->b_sc;
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	struct iobuf *iob = bp->b_queue;	/* to get b_xcount and b_addr */
	register long count = iob->b_xcount;
	register int reading = bp->b_flags & B_READ;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	volatile register unsigned char *med_ctrl = &cp->r[regoff[MED_CTRL]];
	int hshpib_wakeup();
	int dmaresfail=0;

	/* Disable all interrupts during transfer type setup */
	HSHPIB_DISABLE_INTRPT(sc);
	*med_imsk &= ZERO;

	/*
	** decide upon the exact actual transfer type to use
	*/
	if (bp->b_flags & B_DIL) {
		switch(transfer_request) {
		case MUST_INTR:
			if (reading)
				sc->transfer = INTR_TFR;
			else 
				sc->transfer = FHS_TFR;
			break;
		case MAX_OVERLAP:
			/*
			** Non-active controller inbound transfers must use
			** INTR_TFR since FHS_TFR and DMA_TFR cannot terminate
			** on premature EOI.
			*/
			if (reading && ((sc->state & ACTIVE_CONTROLLER) == 0)) {
				sc->transfer = INTR_TFR;
				break;
			}
			sc->transfer = !(count == 1 || !reading && count <= 8) &&
#ifdef __hp9000s700
			(sc->card_type==HP25560) ? DMA_TFR : FHS_TFR;
#else
			(sc->card_type!=HP25560 ? try_dma(sc, transfer_request):1) ? DMA_TFR : FHS_TFR;
#endif /* __hp9000s700 */
			break;
		default:
			panic("hshpib_transfer - DIL transfer_request");
		}
	} else {
		switch(transfer_request) {
		/*
		** hshpib's "interrupt burst" routine handles both FHS and INTR
		*/
		case MUST_FHS:
		case MUST_INTR:
			sc->transfer = FHS_TFR;
			break;
		/*
		** Added for diagnostic commands HPIB_INPUT / HPIB_OUTPUT
		*/
#ifdef __hp9000s700
		case MUST_DMA:
			sc->transfer = DMA_TFR;
			break;
#endif
		/*
		** reads of one byte or writes of eight bytes or less are handled
		** completely by fifo's anyway, so dma isn't even worth attempting
		*/
		case MAX_OVERLAP:
		case MAX_SPEED:
			sc->transfer = !(count == 1 || !reading && count <= 8) &&
#ifdef __hp9000s700
			(sc->card_type==HP25560) ? DMA_TFR : FHS_TFR;
#else
			(sc->card_type!=HP25560 ? try_dma(sc, transfer_request):1) ? DMA_TFR : FHS_TFR;
#endif /* __hp9000s700 */
			break;
		default:
			panic("hshpib_transfer - transfer_request");
		}
	}

	/*
	** preparation common for all transfer types
	*/
	bp->b_action = proc;    /* next activity to perform */
	sc->buffer = iob->b_xaddr;
	sc->resid = count;
	sc->tfr_control = 0;

retrytrans:

	if (reading) {
		if (*med_status & M_HPIB_CTRL) {
			if (count <= 256) {	/* perform a counted transfer */
				*med_status = M_FIFO_LF_INH;
				*med_fifo = count;  /* writes a 0 for 256 */
			} else {	     /* perform an uncounted transfer */
				*med_status = M_FIFO_UCT_XFR;
				*med_fifo = ZERO;
			}
		}
	}

	/*
	** kick-off the actual transfer
	*/
	switch(sc->transfer) {
	case FHS_TFR:
	case INTR_TFR:
	        if (bp->b_flags & B_DIL) {
                        /* If dma is reserved and we're here, we decided
                           not to use it, so give it back */
                        if (iob->b_flags & RESERVED_DMA)
                                drop_dma(sc->owner);
                }

		if (*med_status & M_HPIB_CTRL) {
			*med_imsk |= reading ? M_FIFO_IDLE | M_FIFO_BYTE :
						  M_FIFO_IDLE;
		} else {
			*med_imsk |= reading ? M_FIFO_BYTE : M_FIFO_IDLE;
		}
		HSHPIB_ENABLE_INTRPT(sc);
		/* Go !!!!!!!!!!!!! */
		break;
	case DMA_TFR:
		if(sc->card_type==HP25560) {
		struct dma_parms *dma_parms;
		register unsigned char card_ctrl = cp->p.pyt_dmacfg&P_CH_SEL;
		int ret;

		if (bp->b_flags & B_DIL) {
			MALLOC(bp->b_queue->io_s1,int,sizeof(struct dma_parms),M_DMA,M_NOWAIT);
			dma_parms = (struct dma_parms *)bp->b_queue->io_s1;
		}
		else	{
			MALLOC(bp->b_s2,int,sizeof(struct dma_parms),M_DMA,M_NOWAIT);
			dma_parms = (struct dma_parms *)bp->b_s2;
		}

		if(dma_parms == NULL)	{
			sc->transfer=FHS_TFR;
			goto retrytrans;
		}
		sc->owner = bp;

		/*
		** decide between byte and word-mode DMA
		*/
		if (!(bp->b_flags & B_DIL) && !((int)sc->buffer & 1) && !((count) & 1)) {
			dma_parms->dma_options = DMA_BURST | DMA_DEMAND | DMA_16BYTE;
			card_ctrl &= ~P_BYTE_DMA; /* clear half-word bit */
			sc->state |= F_WORD_DMA;
		} else {
			dma_parms->dma_options = DMA_BURST | DMA_DEMAND | DMA_8BYTE;
			card_ctrl |= P_BYTE_DMA; /* set byte mode */
			sc->state &= ~F_WORD_DMA;
		}

		if (reading) {
			dma_parms->dma_options |= DMA_READ;
			card_ctrl &= ~(P_DMA_DIR|P_AUTO_ENB);
			card_ctrl |= P_FULL_WORD;
		} else {
			dma_parms->dma_options |= DMA_WRITE;
			card_ctrl |= P_DMA_DIR|P_AUTO_ENB;
			card_ctrl &= ~P_FULL_WORD;
		}

		/* Set up DMA */
		dma_parms->channel = ((struct python_data *)(sc->if_drv_data))->pyt_dmacfg&P_CH_SEL;
		dma_parms->flags = NO_CHECK;
		dma_parms->drv_routine = hshpib_wakeup;
		dma_parms->drv_arg = (int)sc;
		dma_parms->addr = bp->b_un.b_addr;
		dma_parms->spaddr = bp->b_spaddr;
		dma_parms->count = bp->b_bcount;

		while (ret = eisa_dma_setup(sc, dma_parms)) {
			if (ret == RESOURCE_UNAVAILABLE) {
				FREE(dma_parms,M_DMA);
				sc->transfer=FHS_TFR;
				goto retrytrans;
			}
			else if ( ret != 0 ){
				FREE(dma_parms,M_DMA);
				sc->transfer=FHS_TFR;
				goto retrytrans;
			}
		}

		if (reading) {
			*med_ctrl &= ~M_FIFO_SEL;
			*med_imsk |= M_FIFO_IDLE;
		} else {
			*med_ctrl |= M_FIFO_SEL;
		}

		if ((bp->b_flags & B_DIL) &&
		     ((bp->b_queue->dil_state & EOI_CONTROL) == 0))
			*med_ctrl |= M_8BIT_PROC;
		else
			*med_ctrl &= ~M_8BIT_PROC;

		cp->p.pyt_dmacfg = card_ctrl;
		cp->p.pyt_intrcfg |= (P_DMA_INTR|P_INTR_ENB);/*enable ints*/
		cp->p.pyt_dmacfg |= P_DMA_ENB;
		/* Go ! */
		}

		else if(sc->card_type==HP98625) {
#ifdef __hp9000s300
		struct dma_channel *dma = sc->dma_chan;
		register int card_ctrl;

		/*
		** decide between byte, word, and long word DMA mode
		*/
		sc->state &= ~(F_WORD_DMA | F_LWORD_DMA);

		if (sc->state & F_SIMONC   &&
		    !(bp->b_flags & B_DIL) &&
		    !((int)sc->buffer & 3) &&
		    !(count & 3)) {
			sc->state |= F_LWORD_DMA;
			cp->s.sim_latch = ZERO;   /* clear resid bits */
			enable_hidden_mode(sc->my_isc);
			cp->s.sim_switches |= (S_ENABLE_SIMONC | S_ENABLE_DMA32);
			disable_hidden_mode(sc->my_isc);
		} else {
			if (!(sc->state & F_SIMONA) &&
			    !(bp->b_flags & B_DIL) &&
			    !((int)sc->buffer & 1) &&
			    !(count & 1)) {
				sc->state |= F_WORD_DMA;
				cp->s.sim_latch = ZERO;   /* clear resid bits */
			} else {
				if (sc->state & F_SIMONC) {
					enable_hidden_mode(sc->my_isc);
					cp->s.sim_switches &= ~(S_ENABLE_SIMONC |
							      S_ENABLE_DMA32);
					disable_hidden_mode(sc->my_isc);
				}
			}
		}
		dma_build_chain(bp);	/* build the transfer chain */

		card_ctrl = (dma->card == (struct dma *)DMA_CHANNEL0) ?
					(S_ENAB | S_DMA_0) : (S_ENAB | S_DMA_1);

		if (sc->state & (F_WORD_DMA | F_LWORD_DMA))
			card_ctrl |= S_WORD;

		if (reading) {
			*med_ctrl &= ~M_FIFO_SEL;
			*med_imsk |= M_FIFO_IDLE;
		} else {
			*med_ctrl |= M_FIFO_SEL;
			card_ctrl |= S_WRIT;
		}

		if (!(sc->state & F_SIMONA) &&
		    (bp->b_flags & B_DIL) &&
		    ((bp->b_queue->dil_state & EOI_CONTROL) == 0))
			*med_ctrl |= M_8BIT_PROC;
		else
			*med_ctrl &= ~M_8BIT_PROC;	/* dma_start sets again if Simon A */

		dma_start(dma);			/* arm dma channel */
		cp->s.sim_ctrl = card_ctrl;	/* now blast off! */
#endif /* hp9000s300 */
		}
		break;
	default:
		panic("hshpib_transfer - sc->transfer");
	}
}

static int hshpib_wakeup(sc)
struct isc_table_type *sc;
{
	wakeup(&sc->owner->b_s2);
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
int hshpib_intr_isr(sc)
struct isc_table_type *sc;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	int transfer_complete = FALSE;
	struct dil_info *dil_info = (struct dil_info *)sc->owner->dil_packet;
	register caddr_t buffer = sc->buffer;	
	register caddr_t last_buffer = buffer;
	register long count = sc->resid;
	register short patience = 0;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_intr = &cp->r[regoff[MED_INTR]];		
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];	
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];	
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	register int *spaddr=0;
	int x;

#ifdef __hp9000s700
	if(sc->owner!=NULL)
		spaddr=sc->owner->b_spaddr;
#endif

	if (sc->owner->b_flags & B_READ) {
		while (count) {
			if (*med_intr == M_FIFO_BYTE) {
				*buffer++ = *med_fifo;	
				count--;
				if (count == 0) transfer_complete = TRUE;
			}
			else if (*med_intr & M_FIFO_IDLE) {
				while (*med_intr & M_FIFO_BYTE && count) {

					*buffer++ = *med_fifo;
					--count;
					if (count == 0) transfer_complete = TRUE;
					if ((*med_status & M_FIFO_EOI) && 
						    (*med_status & M_HIGH_1)) {
						sc->tfr_control |= TR_END;
						transfer_complete = TRUE;
						break;
					}
				}
				break;
			}
			else if (!(patience--)) {
				if (last_buffer == buffer)
					break;
				last_buffer = buffer;
				patience = 5;
			}
		}

		if (transfer_complete) {
			if (!(*med_intr & M_FIFO_IDLE))
				do_hshpib_ifc(sc);	 /* init fifo while under IFC */
			while (*med_intr & M_FIFO_BYTE) {
				unsigned char dummy = *med_fifo;  /* clean out inbound fifo */
			}
		}

	} else {
		/* upon entry, med_imsk == fifo_idle, set fifo_room and clear fifo_idle */
		*med_imsk ^= M_FIFO_ROOM | M_FIFO_IDLE;

		/*** section for outputting up to count-1 bytes without EOI */
		while (count > 1) {
			if (*med_intr && M_FIFO_ROOM) {
				*med_fifo = *buffer++;	
				count--;
			}
			else if (!(patience--)) {
				if (last_buffer == buffer)
					break;
				last_buffer = buffer;
				patience = 5;
			}
		}

		/* section for possibly outputting the final byte with EOI */
		if (count == 1) {
			for (patience = 5; patience > 0; patience--) {
				if (*med_intr & M_FIFO_ROOM) {
					if (sc->owner->b_flags & B_DIL) {
						if ((sc->owner->b_queue->dil_state & EOI_CONTROL)
							&& (dil_info->full_tfr_count <= MAXPHYS))
							*med_status = M_FIFO_EOI;

					} else {
						*med_status = M_FIFO_EOI;
					}
					*med_fifo = *buffer++;
					count--;
					sc->transfer = BO_END_TFR;
					break;
				}
			}

		}

		*med_imsk ^= M_FIFO_ROOM | M_FIFO_IDLE;

		/*
		** if final byte placed in fifo, wait at least a short while,
		** hoping to avoid a separate interrupt upon completion
		*/
		if (sc->transfer == BO_END_TFR)
			for (patience = 5; patience > 0; patience--)
				if (*med_intr & M_FIFO_IDLE) {
					transfer_complete = TRUE;
					break;
				}
	
	}

	sc->resid -= buffer - sc->buffer;
	sc->buffer = buffer;
#ifdef __hp9000s700
	if (transfer_complete && (sc->resid == 0))
#else
	if ((sc->owner->b_flags & B_DIL) && transfer_complete && 
		      (sc->resid == 0) && (dil_info->full_tfr_count <= MAXPHYS))
#endif /* hp9000s700 */
		sc->tfr_control |= TR_COUNT;

	return transfer_complete;
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
int hshpib_pattern_isr(sc)
struct isc_table_type *sc;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	int transfer_complete = FALSE;
	struct dil_info *dil_info = (struct dil_info *)sc->owner->dil_packet;
	register count = sc->resid;
	register patience = 0;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_intr = &cp->r[regoff[MED_INTR]];
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	register caddr_t buffer = sc->buffer;
	register caddr_t last_buffer = buffer;

	while (count) {
		if (M_FIFO_BYTE == *med_intr) {  /* Have only FIFO_BYTE set */
			*buffer = *med_fifo;
			count--;
			if (sc->pattern == *buffer++) {
				sc->tfr_control |= TR_MATCH;
				transfer_complete = TRUE;
				if ((*med_status & M_FIFO_EOI) && (*med_status & M_HIGH_1)) {
					sc->tfr_control |= TR_END;
					break;  
				}
				break;
			}
			else if ((*med_status & M_FIFO_EOI) && (*med_status & M_HIGH_1)) {
				sc->tfr_control |= TR_END;
				transfer_complete = TRUE;
				break; 
			}
			if (count == 0) transfer_complete = TRUE;
		}

		/* more than FIFO_BYTE is set or nothing is set */
		else if (M_FIFO_IDLE & *med_intr) {		/* Do we have EOI? */
			/* Have EOI in FIFO, clean out FIFO until EOI, count, or match is hit */
			while (*med_intr & M_FIFO_BYTE && count) {   
				*buffer = *med_fifo;
				count--;
				if (sc->pattern == *buffer++) {
					sc->tfr_control |= TR_MATCH;
					transfer_complete = TRUE;
					if ((*med_status & M_FIFO_EOI) && 
							(*med_status & M_HIGH_1)) {
						sc->tfr_control |= TR_END;
						break;
					}
				}
				if ((*med_status & M_FIFO_EOI) && (*med_status & M_HIGH_1)) {
					sc->tfr_control |= TR_END;
					transfer_complete = TRUE;
					break;
				}
			}
			if (count == 0) transfer_complete = TRUE;
			break;
		}
		/* no FIFO_BYTE, do we still have patience? */
		else if (!(patience--)) { 	/* no patience, have we gotten any new bytes? */
			if (last_buffer == buffer)
				break;   /* No, let's quit */
			/* Yes, let's wait one more round */
			last_buffer = buffer;
			patience = 5;
		}
	}

	if (transfer_complete) {
		if (!(*med_intr & M_FIFO_IDLE))
			do_hshpib_ifc(sc);	 /* init fifo while under IFC */
		while (*med_intr & M_FIFO_BYTE) {
			unsigned char tmp = *med_fifo;  /* clean out inbound fifo */
		}
	}


	sc->resid -= buffer - sc->buffer;
	sc->buffer = buffer;

#ifdef __hp9000s700
	if (transfer_complete && (sc->resid == 0))
#else
	if ((sc->owner->b_flags & B_DIL) && transfer_complete && 
		      (sc->resid == 0) && (dil_info->full_tfr_count <= MAXPHYS))
#endif /* hp9000s700 */
		sc->tfr_control |= TR_COUNT;

	return transfer_complete;
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_dmaisr(sc)
register struct isc_table_type *sc;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_intr = &cp->r[regoff[MED_INTR]];
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];
	struct dil_info *dil_info = (struct dil_info *)sc->owner->dil_packet;
	register int transfer_complete = FALSE;
	unsigned char high_order_bits = (sc->intcopy >> 8) & 0xff;
	if(sc->card_type==HP98625) {
#ifdef __hp9000s300
	cp->s.sim_ctrl &= ~(S_WORD | S_DMA_0 | S_DMA_1);
	drop_dma(sc->owner);

	/* calculate the number of bytes that didn't get transfered */
	if (sc->state & F_LWORD_DMA) {
		sc->resid = sc->resid << 2;
		if (cp->s.sim_latch & 3)
			sc->resid += 4 - (cp->s.sim_latch & 3);
	} else
		if (sc->state & F_WORD_DMA)
			sc->resid = (sc->resid << 1) + (cp->s.sim_latch & 1);
#endif /* __hp9000s300 */
	}
	else if(sc->card_type==HP25560) {
		register char dmacfg=cp->p.pyt_dmacfg;
		cp->p.pyt_dmacfg &= ~(P_DMA_ENB); /* disable DMA */
		HSHPIB_DISABLE_INTRPT(sc);
		cp->p.pyt_intrcfg &= ~P_DMA_INTR; /*disable ints */
	if (sc->owner->b_flags & B_DIL) {
		sc->resid = eisa_dma_cleanup(sc,sc->owner->b_queue->io_s1);
		FREE(sc->owner->b_queue->io_s1,M_DMA);
	}
	else	{
		sc->resid = eisa_dma_cleanup( sc, sc->owner->b_s2 );
		FREE(sc->owner->b_s2,M_DMA);
	}

	if (sc->state&F_WORD_DMA) {
		if(!(dmacfg&P_FULL_WORD)&&(sc->owner->b_flags&B_READ))
			sc->resid += 1;
	}
	}

	if (sc->owner->b_flags & B_READ) {
		register char tmp;
		/* check for EOI */
		if ((high_order_bits & M_HIGH_0) && (high_order_bits & M_HIGH_1)) 
			sc->tfr_control |= TR_END;
		else
			do_hshpib_ifc(sc);	/* init fifo while under IFC */

#ifdef __hp9000s700
		if (!sc->resid)
#else
		if ((sc->owner->b_flags & B_DIL) && (sc->resid == 0) && 
					(dil_info->full_tfr_count <= MAXPHYS))
#endif  /* hp9000s700 */
			sc->tfr_control |= TR_COUNT;

		*med_imsk |= M_FIFO_BYTE;
		while (*med_intr & M_FIFO_BYTE) 
			tmp = *med_fifo;	/* clean out inbound fifo */
		*med_imsk &= ~M_FIFO_BYTE;
		transfer_complete = TRUE;
	} else {
		*med_imsk |= M_FIFO_IDLE;
		if (!(transfer_complete = *med_intr & M_FIFO_IDLE)) {
			sc->transfer = BO_END_TFR;
			HSHPIB_ENABLE_INTRPT(sc); /* enable interrupts */
		}
	}

	return transfer_complete;
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_pplset(sc, mask, sense, enab)
register struct isc_table_type *sc;
int mask;
int sense;
int enab;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_ppsns = &cp->r[regoff[MED_PPSNS]];
	volatile register unsigned char *med_ppmsk = &cp->r[regoff[MED_PPMSK]];

	/* turn off interrupts until everythings set up */
	HSHPIB_DISABLE_INTRPT(sc);

	sc->transfer = NO_TFR;
	
	sc->ppoll_mask |= mask;
	if (sc->owner->b_flags & B_DIL)
		sc->ppoll_sense = (sc->ppoll_sense & (~mask)) | (sense & mask);
	else 
		if (sense == 1)
			sc->ppoll_sense &= ~mask;
		else
			sc->ppoll_sense |= mask;

	*med_ppsns = sc->ppoll_sense;
	*med_ppmsk = sc->ppoll_mask;

	/* do we want interrupts or not? */
	if (enab) {
		HSHPIB_ENABLE_INTRPT(sc);
	}
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	sc,
**		mask -- unused here, used by 9914 (iod_pplclr)
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_pplclr(sc, mask)
register struct isc_table_type *sc;
char mask;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_ppmsk = &cp->r[regoff[MED_PPMSK]];
	register struct buf *cur_buf, *next_buf;
	register struct dil_info *dil_pkt;
	register int ppoll_rupts = 0;

	/* recompute the cumulative select code mask */
	sc->ppoll_mask = 0;
	for (cur_buf = sc->ppoll_f; cur_buf != NULL; cur_buf = next_buf) {
		next_buf = cur_buf->av_forw;
		if (cur_buf->b_flags & B_DIL)
			sc->ppoll_mask |= cur_buf->b_ba;
		else
			sc->ppoll_mask |= 0x80 >> cur_buf->b_ba;
	}
	for (cur_buf = sc->event_f; cur_buf != NULL; cur_buf = next_buf) {
		dil_pkt = (struct dil_info *) cur_buf->dil_packet;
		next_buf = dil_pkt->dil_forw;
		if (dil_pkt->intr_wait & INTR_PPOLL) {
			sc->ppoll_mask |= dil_pkt->ppl_mask;
			ppoll_rupts = 1;
		}
	}
	*med_ppmsk = sc->ppoll_mask;

	if (!ppoll_rupts)
		sc->intr_wait &= ~INTR_PPOLL;

	HSHPIB_ENABLE_INTRPT(sc);
}


/******************************hshpib_pass_control***********************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_PASS_CONTROL) via dil_util (dil_action)
**		via enqueue (b_action)
**
**	Parameters:  bp -- only needs cp, pass_data1
**
**	Global variables/fields referenced:  b_sc->card_ptr, bp->pass_data1
**
**	Registers modified: sim_ctrl, med_imsk, med_status, med_fifo
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_pass_control(bp)
register struct buf *bp;
{
	register HSHPIB *cp = (bp->b_sc->card_type==HP98625) ? 
		(HSHPIB *)bp->b_sc->card_ptr : (HSHPIB *)bp->b_sc->if_reg_ptr;
	unsigned short *regoff = (bp->b_sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];

	START_POLL;
	HSHPIB_DISABLE_INTRPT(bp->b_sc);
	*med_imsk |= M_FIFO_IDLE;

	MEDUSA_WAIT(M_FIFO_IDLE);	/* room for 8 bytes after this */
	*med_status = M_FIFO_ATN;
	*med_fifo = TAG_BASE + (bp->pass_data1 & 0x1f);	/* 1 */
	*med_fifo = TCT;					/* 2 */
	MEDUSA_WAIT(M_FIFO_IDLE);
	*med_imsk &= ~M_FIFO_IDLE;
	HSHPIB_ENABLE_INTRPT(bp->b_sc);
	END_POLL;

	return;

}




/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_save_state(sc)
register struct isc_table_type *sc;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];

	if (sc->state & STATE_SAVED)
		return;

	HSHPIB_DISABLE_INTRPT(sc);
	sc->intmskcpy = *med_imsk;
	*med_status = M_INT_ENAB;
	*med_imsk = ZERO;

	sc->state |= STATE_SAVED;
}

/*************************hshpib_restore_state***************************
** [short description of routine]
**
**	Called by:  hpib_transfer via iosw,
**		    hshpib_abort_io,
**		    hshpib_postamb
**
**	Parameters:	sc
**
**	Global variables/fields referenced: sc->card_ptr, sc->state, 
**		sc->intmskcpy
**
**	Global vars/fields modified:  sc->state
**
**	Registers modified:  sim_ctrl, med_imsk
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_restore_state(sc)
register struct isc_table_type *sc;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];

	char dummy;  /* value read from card, never used */

	if ((sc->state & STATE_SAVED) == 0)
		return;

	HSHPIB_DISABLE_INTRPT(sc);
	dummy = *med_imsk;
	*med_imsk = (unsigned char) sc->intmskcpy;
	sc->state &= ~STATE_SAVED;
	HSHPIB_ENABLE_INTRPT(sc);
}


/******************************hshpib_ren******************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_REN) via dil_utility (dil_action) 
**		via enqueue (b_action)
**
**	Parameters:  bp
**
**	Global variables/fields referenced:  b_sc->card_ptr, bp->pass_data1
**
**	Registers modified:  med_ctrl
**
**	*** comment local variable usage ***
**
**  Comment:
**	TI9914_ren needs bp for call to TI9914_postamb and iob->dil_state
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_ren(bp)
register struct buf *bp;
{
	register HSHPIB *cp = (bp->b_sc->card_type==HP98625) ? 
		(HSHPIB *)bp->b_sc->card_ptr : (HSHPIB *)bp->b_sc->if_reg_ptr;
	unsigned short *regoff = (bp->b_sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_ctrl = &cp->r[regoff[MED_CTRL]];

	if (bp->pass_data1)
		*med_ctrl |= M_REN;
	else
		*med_ctrl &= ~M_REN;
}

/*************************hshpib_set_ppoll_resp*************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_PPOLL_IST and HPIB_PPOLL_RESP) 
**		via dil_utility (dil_action) via enqueue (b_action)
**
**	Parameters:  bp (only needs sc, so does 9914)
**
**	Global variables/fields referenced:  bp->b_sc, sc->card_ptr,
**		sc->ppoll_resp 
**
**	Registers modified:  med_status, sim_ctrl, med_imsk, med_fifo
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_set_ppoll_resp(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];
	unsigned char ppoll_byte;


	if ((*med_status & M_HPIB_CTRL) == 0)
		return;

	if (sc->state & IST)
		ppoll_byte = sc->ppoll_resp >> 8;
	else
		ppoll_byte = (unsigned char)sc->ppoll_resp;

	if (ppoll_byte == 0)
		return;
	if (ppoll_byte == 0xff)
		ppoll_byte = 0;

	START_POLL;
	/* any ppolls/srq's are ignored till done */
	HSHPIB_DISABLE_INTRPT(sc);
	*med_imsk |= M_FIFO_IDLE;

	MEDUSA_WAIT(M_FIFO_IDLE);	/* Room for up to 8 bytes */
	*med_status = M_FIFO_ATN;
	*med_fifo = TAG_BASE + MA;		/* 1 */
	*med_fifo = LAG_BASE + MA;		/* 2 */
	*med_fifo = PPC;			/* 3 */
	*med_fifo = SCG_BASE | ppoll_byte;	/* 4 */
	MEDUSA_WAIT(M_FIFO_IDLE);
	*med_imsk &= ~M_FIFO_IDLE;
	HSHPIB_ENABLE_INTRPT(sc);
	END_POLL;
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_iocheck(bp)
register struct buf *bp;
{
	register struct iobuf *iob = bp->b_queue;
	register struct isc_table_type *sc = bp->b_sc;
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];
	volatile register unsigned char *med_intr = &cp->r[regoff[MED_INTR]];
	register struct dil_info *dil_info = (struct dil_info *)bp->dil_packet;
	register int state = iob->dil_state & 0xff;

	/* always look at our current status */
	if (*med_status & M_HPIB_CTRL)
		sc->state |= ACTIVE_CONTROLLER;
	else
		sc->state &= ~ACTIVE_CONTROLLER;

	if ( (!(state & D_RAW_CHAN)) && 
	     (!(sc->state & ACTIVE_CONTROLLER))) {
		bp->b_error = EINVAL;	/* can't do */
		bp->b_flags |= B_ERROR;
		return TRANSFER_ERROR;
	}
	if (state & D_RAW_CHAN) {
		if ((bp->b_flags & B_READ) && (!(*med_status & M_LTN))) {
			*med_imsk |= M_FIFO_BYTE;
			if (*med_intr & M_FIFO_BYTE) {
				*med_imsk &= ~M_FIFO_BYTE;
				return TRANSFER_READY;
			}
			*med_imsk &= ~M_FIFO_BYTE;
			if (sc->state & ACTIVE_CONTROLLER) {
				bp->b_error = EINVAL;	/* can't do */
				bp->b_flags |= B_ERROR;
				return TRANSFER_ERROR;
			} else {
				/* wait until addressed to listen */
				sc->intr_wait |= WAIT_LISTEN;
				dil_info->intr_wait |= WAIT_LISTEN;
				if ( ! want_hshpib_poll) {
					timeout(hshpib_fakeisr_poll, 1, 1, NULL);
					want_hshpib_poll++;
				}
				return TRANSFER_WAIT;
			}
		}
		if ((!(bp->b_flags & B_READ)) && (!(*med_status & M_TLK))) {
			if (sc->state & ACTIVE_CONTROLLER) {
				bp->b_error = EINVAL;	/* can't do */
				bp->b_flags |= B_ERROR;
				return TRANSFER_ERROR;
			} else {
				/* wait until addressed to talk */
				sc->intr_wait |= WAIT_TALK;
				dil_info->intr_wait |= WAIT_TALK;
				if ( ! want_hshpib_poll) {
					timeout(hshpib_fakeisr_poll, 1, 1, NULL);
					want_hshpib_poll++;
				}
				return TRANSFER_WAIT;
			}
		}
	}
	return TRANSFER_READY;
}

extern int pct_flag;

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_SEND_CMD) and do_hpib_io via 
**		dil_util (dil_action) via enqueue (b_action)
**
**	Parameters:  bp (needs sc, pass_data*)
**
**	Global variables/fields referenced:  b_sc->card_ptr, bp->pass_data1,
**		bp->pass_data2
**
**	Global vars/fields modified:  pct_flag
**
**	Registers modified:  sim_ctrl, med_imsk, med_status, med_fifo
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_send_cmd(bp)
register struct buf *bp;
{
	register HSHPIB *cp = (bp->b_sc->card_type==HP98625) ? 
		(HSHPIB *)bp->b_sc->card_ptr : (HSHPIB *)bp->b_sc->if_reg_ptr;
	unsigned short *regoff = (bp->b_sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];
	register char *bfr = (char *) bp->pass_data2;  /* data to send as commands */
	register int count = bp->pass_data1;	/* number of commands */

	pct_flag = 1;		/* global variable */
	START_POLL;
	/* any ppolls/srq's are ignored till done */
	HSHPIB_DISABLE_INTRPT(bp->b_sc);
	*med_imsk |= (M_FIFO_ROOM | M_FIFO_IDLE);

	MEDUSA_WAIT(M_FIFO_IDLE);	/* Room for up to 8 bytes */
	while (count-- > 0) {
		MEDUSA_WAIT(M_FIFO_ROOM);
		*med_status = M_FIFO_ATN;
		*med_fifo = *bfr++;
	}

	MEDUSA_WAIT(M_FIFO_IDLE);
	*med_imsk &= ~(M_FIFO_ROOM | M_FIFO_IDLE);
	HSHPIB_ENABLE_INTRPT(bp->b_sc);

	END_POLL;
	pct_flag = 1;
}

/******************************hshpib_dil_abort******************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_RESET) via dil_utility (dil_action)
**		via enqueue (b_action)
**
**	Parameters:  bp (only needs cp, so does 9914)
**
**	Kernel routines called:  do_hshpib_ifc
**
**	Global variables/fields referenced:  bp->b_sc->card_ptr
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_dil_abort(bp)
register struct buf *bp;
{
	do_hshpib_ifc(bp->b_sc);	/* init fifo while under IFC */
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_wait_set(sc, active_waits, request)
register struct isc_table_type *sc;
register int *active_waits;
register int request;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_intr = &cp->r[regoff[MED_INTR]];
	register int status = 0;
	unsigned char med_imask;

	/* always look at our current status */
	if (*med_status & M_HPIB_CTRL)
		sc->state |= ACTIVE_CONTROLLER;
	else
		sc->state &= ~ACTIVE_CONTROLLER;

	if (request & STATE_ACTIVE_CTLR) {
		if (sc->state & ACTIVE_CONTROLLER)
			status |= STATE_ACTIVE_CTLR;
		else {
			sc->intr_wait |= WAIT_CTLR;
			*active_waits |= WAIT_CTLR;
			if ( ! want_hshpib_poll) {
				timeout(hshpib_fakeisr_poll, 1, 1, NULL);
				want_hshpib_poll++;
			}
		}
	}
	if (request & STATE_SRQ) {
		*med_imsk |= M_SERV_RQST;
		if (*med_intr & M_SERV_RQST) {
			status |= STATE_SRQ;
			*med_imsk &= ~M_SERV_RQST;
		} else if (status == 0) {
			/* wait til an SRQ interrupt happens */
			sc->intr_wait |= WAIT_SRQ;
			*active_waits |= WAIT_SRQ;
			med_imask = *med_imsk;
			*med_status = M_INT_ENAB;
			*med_imsk = med_imask | M_SERV_RQST;
			HSHPIB_ENABLE_INTRPT(sc);
		}
	}
	if (request & STATE_TALK) {
		if (*med_status & M_TLK)
			status |= STATE_TALK;
		else if (status == 0) {
			sc->intr_wait |= WAIT_TALK;
			*active_waits |= WAIT_TALK;
			if ( ! want_hshpib_poll) {
				timeout(hshpib_fakeisr_poll, 1, 1, NULL);
				want_hshpib_poll++;
			}
		}
	}
	if (request & STATE_LISTEN) {
		if (*med_status & M_LTN)
			status |= STATE_LISTEN;
		else if (status == 0) {
			sc->intr_wait |= WAIT_LISTEN;
			*active_waits |= WAIT_LISTEN;
			if ( ! want_hshpib_poll) {
				timeout(hshpib_fakeisr_poll, 1, 1, NULL);
				want_hshpib_poll++;
			}
		}
	}
	return(status);
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  HPIB_status_clear via iosw (iod_wait_clr)
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:  hshpib_fakeisr_check
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_wait_clear(sc)
register struct isc_table_type *sc;
{
/*** Code needs to be added here to disable SRQ interrupts
     if there isn't anybody else waiting for them.  This code
     was supposed to do that, but doesn't.....

	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_intr = &cp->r[regoff[MED_INTR]];
	if ((sc->intr_wait & WAIT_SRQ) == 0)
		*med_imsk &= ~M_SERV_RQST;
***/

	hshpib_fakeisr_check();
}

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_intr_set(sc, active_waits, request)
register struct isc_table_type *sc;
register int *active_waits;
register int request;
{
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	unsigned char med_imask;

	*active_waits = 0;
	sc->intr_wait |= request;

	if (request & INTR_DCL) {
		*active_waits |= INTR_DCL;
		med_imask = *med_imsk;
		*med_status = M_INT_ENAB;
		*med_imsk = med_imask | M_DEV_CLR;
		HSHPIB_ENABLE_INTRPT(sc);
	}
	if (request & INTR_IFC) 
		*active_waits |= INTR_IFC;
	if (request & INTR_GET)
		*active_waits |= INTR_GET;
	if (request & INTR_REN)
		*active_waits |= INTR_REN;

	if (request & INTR_CTLR) {
		*active_waits |= INTR_CTLR;
		if ( ! want_hshpib_poll) {
			timeout(hshpib_fakeisr_poll, 1, 1, NULL);
			want_hshpib_poll++;
		}
	}
	if (request & INTR_SRQ) {
		*active_waits |= INTR_SRQ;
		med_imask = *med_imsk;
		*med_status = M_INT_ENAB;
		*med_imsk = med_imask | M_SERV_RQST;
		HSHPIB_ENABLE_INTRPT(sc);
	}
	if (request & INTR_TALK) {
		*active_waits |= INTR_TALK;
		if ( ! want_hshpib_poll) {
			timeout(hshpib_fakeisr_poll, 1, 1, NULL);
			want_hshpib_poll++;
		}
	}
	if (request & INTR_LISTEN) {
		*active_waits |= INTR_LISTEN;
		if ( ! want_hshpib_poll) {
			timeout(hshpib_fakeisr_poll, 1, 1, NULL);
			want_hshpib_poll++;
		}
	}
}


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_abort_io(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;

	if (sc->owner != bp)
		return;

	HSHPIB_DISABLE_INTRPT(sc);

	switch(sc->transfer) {
	case NO_TFR:
	case INTR_TFR:
	case FHS_TFR:
	case BO_END_TFR:
			/* assume high level will call poll clear, which
			   also sets mask registers up */
			break;
	case DMA_TFR:
#ifdef __hp9000s300
			if(sc->card_type==HP98625)
				dmaend_isr(sc);
#endif /* hp9000s300 */
			hshpib_dmaisr(sc);
			break;
	default:
			panic("hshpib_abort");
	}

	sc->transfer = NO_TFR;

	preserving_enables_do_reset(sc);

	if (sc->state & STATE_SAVED)
		hshpib_restore_state(sc);

	HSHPIB_ENABLE_INTRPT(sc);
}

/******************************hshpib_srq******************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_SRQ) via dil_utility (dil_action)
**		via enqueue (b_action)
**
**	Parameters:  bp (needs cp, pass_data1, so does 9914)
**
**	Global variables/fields referenced: bp->b_sc->card_ptr, bp->pass_data1
**
**	Registers modified: med_imsk, med_ctrl
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_srq(bp)
register struct buf *bp;
{
	register HSHPIB *cp = (bp->b_sc->card_type==HP98625) ? 
		(HSHPIB *)bp->b_sc->card_ptr : (HSHPIB *)bp->b_sc->if_reg_ptr;
	unsigned short *regoff = (bp->b_sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_ctrl = &cp->r[regoff[MED_CTRL]];
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];

	*med_imsk &= ~M_SERV_RQST;
	*med_ctrl &= ~M_RQST_SRVC;
	if (bp->pass_data1 & 64)
		*med_ctrl |= M_RQST_SRVC;
}

/***************************hshpib_dil_reset****************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (IO_RESET) via dil_utility (dil_action)
**		via enqueue (b_action)
**
**	Parameters:  bp (needs sc)
**
**	Kernel routines called:  do_hshpib_reset
**
**	Global variables/fields referenced:  bp->b_sc
**
**	Global vars/fields modified:  sc->state, sc->ppoll_resp, sc->transfer
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_dil_reset(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;

	sc->transfer = NO_TFR;
	do_hshpib_reset(sc);
	sc->state &= ~IST;
	sc->ppoll_resp = 0;
	/*
	sc->ppoll_f = NULL;
	sc->event_f = NULL;
	sc->status_f = NULL;
	*/
}

/******************************hshpib_status*****************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_BUS_STATUS) thru iosw (iod_status)
**		and via dil_utility (dil_action) via enqueue (b_action)
**
**	Parameters: bp
**
**	Global variables/fields referenced:  b_sc->card_ptr, b_sc->state,
**		bp->pass_data1
**
**	Global vars/fields modified:  bp->b_error, bp->b_flags, bp->return_data
**
**	Registers modified: sim_ctrl, med_imsk
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_status(bp)
register struct buf *bp;
{
	register HSHPIB *cp = (bp->b_sc->card_type==HP98625) ? 
		(HSHPIB *)bp->b_sc->card_ptr : (HSHPIB *)bp->b_sc->if_reg_ptr;
	unsigned short *regoff = (bp->b_sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_intr = &cp->r[regoff[MED_INTR]];
	register int status;		/* register var for performance */
	register int status_result;		/* register var for performance */
	register ushort state = bp->b_sc->state;	/* register var for performance */

	status = bp->pass_data1;
	status_result = 0;
	HSHPIB_ENABLE_INTRPT(bp->b_sc);

	if (status & STATE_REN)
		if (*med_status & M_REM)
			status_result |= STATE_REN;
	if (status & STATE_SRQ) {
		*med_imsk |= M_SERV_RQST;
		if (*med_intr & M_SERV_RQST)
			status_result |= STATE_SRQ;
		*med_imsk &= ~M_SERV_RQST;
	}
	if (status & STATE_NDAC)
		bp->b_error = EINVAL;
	if (status & STATE_SYSTEM_CTLR)
		if (state & SYSTEM_CONTROLLER)
			status_result |= STATE_SYSTEM_CTLR;
	if (status & STATE_ACTIVE_CTLR)
		if (state & ACTIVE_CONTROLLER)
			status_result |= STATE_ACTIVE_CTLR;
	if (status & STATE_TALK)
		if (*med_status & M_TLK)
			status_result |= STATE_TALK;
	if (status & STATE_LISTEN)
		if (*med_status & M_LTN)
			status_result |= STATE_LISTEN;
	if ((status & ~STATUS_BITS) || bp->b_error) { /* check for bad value */
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
	} else
		/* pass back value */
		bp->return_data = status_result;

	HSHPIB_ENABLE_INTRPT(bp->b_sc);
}

/**************************hshpib_ctlr_status****************************
** [short description of routine]
**
**	Called by:  hpib_ioctl and do_hpib_io via dil_utility (dil_action)
**		via enqueue (b_action) and by do_hpib_io thru iosw (iod_ctlr_status)
**
**	Parameters:  bp (needs cp, sc->state, so does 9914)
**
**	Global variables/fields referenced:  bp->b_sc, b_sc->card_ptr
**
**	Global vars/fields modified: sc->state
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_ctlr_status(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	
	if (*med_status & M_HPIB_CTRL)
		sc->state |= ACTIVE_CONTROLLER;
	else
		sc->state &= ~ACTIVE_CONTROLLER;

	if (*med_status & M_SYST_CTRL)
		sc->state |= SYSTEM_CONTROLLER;
	else
		sc->state &= ~SYSTEM_CONTROLLER;
}

#ifdef __hp9000s300
/******************************routine name******************************
** [short description of routine]
**	This routine contains little new.  It is mostly hshpib_mesg,
**	hshpib_preamb, and hshpib_dma all stuck together in one piece.
**	Its intent is the support of the MAC discs at high speeds.
**	They require the unbuffered read/write request and the
**	send/receive data messages to be sent more quickly than can
**	be done under 'normal' circumstances, thus we have this routine.
**
**	Differences from the normal mesg/preamb/transfer scenario:
**	1) the calling routine pre-allocates (gets) the dma channel!!!
**	2) dma_build_chain is done before the request & transfer messages
**	3) interrupts are locked out during the critical timing section
**	4) parallel polling after secondaries & between messages eliminated
**	5) duplicate UNL between the request & transfer messages eliminated
**	6) intermediate procedure call overhead eliminated
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_highspeed(bp, sec1, sec2, bfr, count, proc)
register struct buf *bp;
char sec1, sec2;
register char *bfr;
register short count;
int (*proc)();
{
	register struct isc_table_type *sc = bp->b_sc;
	struct dma_channel *dma = sc->dma_chan;
	struct iobuf *iob = bp->b_queue;
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	volatile register unsigned char *med_ctrl = &cp->r[regoff[MED_CTRL]];
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	register int card_ctrl;
	int x;

	if (sc->owner != bp)
		panic("hshpib_highspeed - select code");

	/*
	** decide between byte, word, and long word DMA mode
	*/

	sc->state &= ~(F_WORD_DMA | F_LWORD_DMA);

	if (sc->state & F_SIMONC && 
	    !((int)iob->b_xaddr & 3) && 
	    !(iob->b_xcount & 3)) {
		sc->state |= F_LWORD_DMA;
		cp->s.sim_latch = ZERO;   /* clear resid bits */
		enable_hidden_mode(sc->my_isc);
		cp->s.sim_switches |= (S_ENABLE_SIMONC | S_ENABLE_DMA32);
		disable_hidden_mode(sc->my_isc);
	} else
		if (!(sc->state & F_SIMONA) &&
		    !((int)iob->b_xaddr & 1) &&
		    !(iob->b_xcount & 1)) {
			sc->state |= F_WORD_DMA;
			cp->s.sim_latch = ZERO;   /* clear resid bits */
		} else
			if (sc->state & F_SIMONC) {
				enable_hidden_mode(sc->my_isc);
				cp->s.sim_switches &= ~(S_ENABLE_SIMONC |
						      S_ENABLE_DMA32);
				disable_hidden_mode(sc->my_isc);
			}

	dma_build_chain(bp);	/* build the transfer chain */

	bp->b_action = proc;

	card_ctrl = (dma->card == (struct dma *)DMA_CHANNEL0) ?
					(S_ENAB | S_DMA_0) : (S_ENAB | S_DMA_1);
	if (sc->state & (F_WORD_DMA | F_LWORD_DMA))
		card_ctrl |= S_WORD;

	/* any ppolls/srq's are ignored till done (if any should come in 
	   in here, disaster is guaranteed) */

	START_POLL;
	HSHPIB_DISABLE_INTRPT(sc);
	sc->transfer = DMA_TFR;  /* set transfer type when interrupts are off */
	
	*med_imsk |= (M_FIFO_ROOM | M_FIFO_IDLE);

	MEDUSA_WAIT(M_FIFO_IDLE);	/* Room for up to 8 bytes */
	*med_status = M_FIFO_ATN;
	*med_fifo = UNL;				/* 1 */
	*med_fifo = TAG_BASE + MA;			/* 2 */
	*med_fifo = LAG_BASE + bp->b_ba;		/* 3 */
	*med_fifo = sec1;				/* 4 */
	
	while (--count) {
		MEDUSA_WAIT(M_FIFO_ROOM);
		*med_status = ZERO;
		*med_fifo = *bfr++;
	}

	MEDUSA_WAIT(M_FIFO_IDLE);	/* room for 8 bytes after this */
	*med_status = M_INT_ENAB;
	*med_imsk = ZERO;
	END_POLL;

	x = spl6();		/* critical timing section starts here */

	*med_status = M_FIFO_EOI;
	*med_fifo = *bfr++;				/* 1 */

	*med_status = M_FIFO_ATN;
	*med_fifo = UNL;				/* 2 */

	if (bp->b_flags & B_READ) {
		*med_fifo = LAG_BASE + MA;		/* 3 */
		*med_fifo = TAG_BASE + bp->b_ba;	/* 4 */
		*med_fifo = sec2;			/* 5 */
		*med_status = M_FIFO_UCT_XFR;
		*med_fifo = ZERO;			/* 6 */
		*med_ctrl &= ~M_FIFO_SEL;
		*med_imsk |= M_FIFO_IDLE;
	} else {
		*med_fifo = TAG_BASE + MA;		/* 3 */
		*med_fifo = LAG_BASE + bp->b_ba;	/* 4 */
		*med_fifo = sec2;			/* 5 */
		*med_ctrl |= M_FIFO_SEL;
		card_ctrl |= S_WRIT;
	}

	*med_ctrl &= ~M_8BIT_PROC;	/* dma_start sets again if Simon A */

	dma_start(dma);			/* arm the dma channel */
	cp->s.sim_ctrl = card_ctrl;	/* now let Simon blast off! */

	splx(x);		     /* critical timing section ends here */
}
#endif /* hp9000s300 */


/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
**	 sc->transfer will be NO_TFR unless there is a transfer in progress.
**	 The masks are set up so that only transfer termination can cause
**	 an interrupt on transfer.
************************************************************************/
hshpib_abort(bp)
struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;

	if (sc->owner != bp)
		return;

	HSHPIB_DISABLE_INTRPT(sc);

	switch(sc->transfer) {
	case NO_TFR:
	case FHS_TFR:
	case INTR_TFR:
	case BO_END_TFR:
			/* assume high level will call poll clear, which
			   also sets mask registers up */
			break;
	case DMA_TFR:
#ifdef __hp9000s300
			if(sc->card_type==HP98625)
				dmaend_isr(sc);
#endif /* hp9000s300 */
			hshpib_dmaisr(sc);
			break;
	default:
			panic("hshpib_abort");
	}

	sc->transfer = NO_TFR;

	preserving_enables_do_reset(sc);
	HSHPIB_ENABLE_INTRPT(sc);
}

#ifdef __hp9000s300
/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
struct dma_chain *simon_dma_setup(bp, dma_chain, entry)
struct buf *bp;
register struct dma_chain *dma_chain;
int entry;
{
	register HSHPIB *cp = (HSHPIB *)bp->b_sc->card_ptr;
	register state = bp->b_sc->state;
	register unsigned int dil_state = bp->b_queue->dil_state;
	struct dil_info *dil_info = (struct dil_info *)bp->dil_packet;

	switch (entry) {
	case FIRST_ENTRIES:
		if (!(state & F_SIMONA)) {
			dma_chain->arm = DMA_ENABLE | DMA_LVL7;
			dma_chain->card_byte = S2_DISABLE_DONE;
			dma_chain->card_reg = (caddr_t) &cp->s.sim_ctrl2;
		} else {
			dma_chain->arm = 0;  /* dma channel itself mustn't interrupt */
			dma_chain->card_byte = M_8BIT_PROC | M_REN;
			if ((bp->b_flags & B_READ) == 0)
				dma_chain->card_byte |= M_FIFO_SEL;
			dma_chain->card_reg = (caddr_t) &cp->s.med_ctrl;
		}
		break;
	case LAST_ENTRY:
		if (!(state & F_SIMONA)) {
			dma_chain->arm &= ~DMA_ENABLE;
			dma_chain->card_byte = 0;  /* enable DMA_DONE sensing */
		} else
			/* if not DIL or if EOI is on then send EOI on last byte */
			if (((bp->b_flags & B_DIL) == 0) || ((dil_state & EOI_CONTROL)
				&& (dil_info->full_tfr_count <= MAXPHYS))) 
				dma_chain->card_byte &= ~M_8BIT_PROC;
		break;
	default:
		panic("simon_dma_setup: unknown dma setup entry");
	}
	return(dma_chain);
}
#endif /* hp9000s300 */

/****************************hshpib_set_addr****************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_SET_ADDR) via dil_utility (dil_action)
**		via enqueue (b_action)
**
**	Parameters:  bp (needs sc, pass_data1, so does 9914)
**
**	Global variables/fields referenced:  bp->pass_data1, bp->b_sc,
**		sc->card_ptr
**
**	Global vars/fields modified:  sc->my_address
**
**	Registers modified:  med_address
************************************************************************/
hshpib_set_addr(bp)
register struct buf *bp;
{
	register struct isc_table_type *sc = bp->b_sc;
	register HSHPIB *cp = (sc->card_type==HP98625) ? 
		(HSHPIB *)sc->card_ptr : (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff =  (sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_address = &cp->r[regoff[MED_ADDRESS]];

	sc->my_address = bp->pass_data1 & 0x1F;
	*med_address = M_ONL | sc->my_address;

}

/******************************hshpib_atn_ctl******************************
** [short description of routine]
**
**	Called by:  hpib_ioctl (HPIB_ATN) via dil_utility (dil_action)
**		via enqueue (b_action)
**
**	Parameters:  bp (needs sc, pass_data1, so does 9914)
**
**	Global variables/fields referenced:  bp->pass_data1, bp->b_sc->card_ptr
**
**	Registers modified: sim_ctrl, med_imsk
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_atn_ctl(bp)
register struct buf *bp;
{
	register HSHPIB *cp = (bp->b_sc->card_type==HP98625) ? 
		(HSHPIB *)bp->b_sc->card_ptr : (HSHPIB *)bp->b_sc->if_reg_ptr;
	unsigned short *regoff = (bp->b_sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];

	HSHPIB_DISABLE_INTRPT(bp->b_sc);
	if (bp->pass_data1 == 0) {
		/* drop atn */
		*med_imsk &= ~M_POLL_RESP;
	} else {
		/* set atn */
		*med_imsk |= M_POLL_RESP;
	}
	HSHPIB_ENABLE_INTRPT(bp->b_sc);
}

#ifdef __hp9000s300
simon_savecore_xfer (bp)
struct buf *bp;
{
	register HSHPIB *cp = (HSHPIB *)bp->b_sc->card_ptr;
	register int zero = 0;	/* Keep compiler from generating clrs */
	register caddr_t addr;
	register int i;
	extern int physmembase, dumpsize;
	extern caddr_t dio2_map_page();
	long page, pagecnt = 0;
	unsigned short *regoff = (bp->b_sc->card_type==HP98625) ? 
		simon_med_regoff : python_med_regoff;
	volatile register unsigned char *med_imsk = &cp->r[regoff[MED_IMSK]];
	volatile register unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	volatile register unsigned char *med_fifo = &cp->r[regoff[MED_FIFO]];

	cp->s.sim_ctrl = zero;
	*med_imsk |= (M_FIFO_ROOM | M_FIFO_BYTE | M_FIFO_IDLE);
	MEDUSA_WAIT(M_FIFO_IDLE);

	for (page = physmembase; pagecnt < dumpsize; 
	     pagecnt++, page++) {
/*
 * Map in the physical page of memory into a pte so it can be dereferenced
 * Do every byte in each page except for the last byte in the last page
 */
		for (addr = dio2_map_page((caddr_t)(page << PGSHIFT)),
		     i = 0; i < (NBPG - (pagecnt == (dumpsize - 1))); i++) {

			MEDUSA_WAIT(M_FIFO_ROOM);
			*med_status = zero;
			*med_fifo = *addr++;
		}
	}

/*
 * Do the last byte with EOI
 */
	*med_status = M_FIFO_EOI;
	*med_fifo = *addr++;
	MEDUSA_WAIT(M_FIFO_IDLE); /* Wait for FIFO outbound buffer to empty */
	*med_imsk &= ~(M_FIFO_ROOM | M_FIFO_BYTE | M_FIFO_IDLE);

}
#endif /* __hp9000s300 */

hshpib_nop()
{	
	return(0);
}

struct drv_table_type hshpib_iosw = {
	hshpib_init,
	hshpib_identify,
	hshpib_clear_wopp,
	hshpib_clear,
	hshpib_nop,		/* used to be hshpib_ifc */
	hshpib_nop,		/* used to be hshpib_isr */
	hshpib_transfer,
	hshpib_pplset,
	hshpib_pplclr,
	hshpib_ppoll,
	hshpib_spoll,
	hshpib_preamb,
	hshpib_postamb,
	hshpib_mesg,
#ifdef __hp9000s300
	hshpib_highspeed,
#else
	hshpib_nop,		
#endif
	hshpib_abort,
	hshpib_ren,
	hshpib_send_cmd,
	hshpib_dil_abort,
	hshpib_ctlr_status,
	hshpib_set_ppoll_resp,
	hshpib_srq,
	hshpib_save_state,
	hshpib_restore_state,
	hshpib_iocheck,
	hshpib_dil_reset,
	hshpib_status,
	hshpib_pass_control,
	hshpib_abort_io,
	hshpib_nop,		
	hshpib_wait_set,
	hshpib_wait_clear,
	hshpib_intr_set,
#ifdef __hp9000s300
	simon_dma_setup,
#else
	hshpib_nop,		
#endif
	hshpib_set_addr,
	hshpib_atn_ctl,
	hshpib_reg_access,
	hshpib_preamb_intr,
	hshpib_nop,
#ifdef __hp9000s300
	simon_savecore_xfer	/* for savecore 300 */
#endif /* __hp9000s300 */
};


/*
** linking and inititialization routines
*/

#ifdef __hp9000s300
extern int (*make_entry)();
int (*simon_saved_make_entry)();

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
simon_make_entry(id, isc)
int id;
struct isc_table_type *isc;
{
	HSHPIB *cp = (HSHPIB *)isc->card_ptr;
	int req_lvl = isc->int_lvl;  /* actual hardware interrupt level */
	int simon_A = 0;
        int x;
        char cardstr[80];
        extern char *strcpy();
        extern char *strcat();

	if ((id != 8) && (id != 72))
		return (*simon_saved_make_entry)(id, isc);

	strcpy(cardstr, "HP98625  High-Speed HP-IB Interface - ");

	isc->iosw = &hshpib_iosw;
	isc->card_type = HP98625;

	/*
	** Simon C is distinguished by the existance of hidden mode
	*/
	if (hidden_mode_exists(isc->my_isc)) {
		isc->state |= F_SIMONC;
		cardstr[7] = 'C';
		
	} else {
		/*
		** Simon B distinguished from Simon A by writable word mode bit
		*/
		cp->s.sim_ctrl = S_WORD;
		if ((cp->s.sim_ctrl & S_WORD) != 0) {
			isc->state |= F_SIMONB;
			cardstr[7] = 'B';
		} else{	
			simon_A = TRUE;
			isc->state |= F_SIMONA;
			cardstr[7] = 'A';
		}
	}

	/*
	** Find out if simon is system controller
	*/
	x = spl6();
	cp->s.med_address |= M_ONL;
	if (!cp->s.med_status & M_SYST_CTRL)
		strcat(cardstr,  "non-");
	strcat(cardstr,  "system controller");

	cp->s.med_address &= ~M_ONL;
	splx(x);

        /*
        ** if Simon B or C, allow int level 3 in addition to 4
        ** if Simon A, allow int level 6 in addition to 4, but give disclaimer
        */
        if (!simon_A && req_lvl != 3 || simon_A && req_lvl != 6)
                req_lvl = 4;

        if (!io_inform(cardstr, isc, req_lvl))
                return FALSE;
	if (req_lvl == 6) {
printf("  WARNING: HP98625A card interrupt level at 6 instead of 4.\n");
printf("  This configuration is for HP internal usage only.\n");
		isc->int_lvl = 4;  /* psuedo interrupt level */
	}

	return TRUE;
}
#endif /* hp9000s300 */

#ifdef __hp9000s700

/****************************************************************************
*****************************************************************************

  This section contains the diagnostic support code for the hshpib interface
  driver.
  The diagnostic entry point is the routine hshpib_diag().  It is called by
  the strategy routine of the diagnostic device driver to execute a single
  diagnostic command.


   The following diagram shows the relationship of the data structures which
   include the isc_table[] entry, the diag_isc_type data structure and the
   bp buffer passed into the hshpib_diag() routine.  The parameter "bp",
   passed into hshpib_diag() is a part of the structure diag_isc_type.
   The bp structure has a pointer referencing the parent isc_table entry
   (bp->b_sc).  The isc table entry inturn points back to the diag_isc_type 
   through the pointer chain ( sc->g_drv_data->diag_drv ).

   The field diag_msg of the diag_isc_type is a structure of type
   diag1_msg_type.  It contains the current diagnostic message to execute.

      isc_table[]
	    ------------------------------------------
	      |                       |
	      |                       |
	      |                       |
	      |  g_drv_data ----------------
	    ----------------^--------------|----------
			    |              |
		            |       -------V--------
			    |       |              | struct gdd
			    |       |  diag_drv -- |
			    |       |            | |
		            |       -------------|--
			    |            	 |
		            |           	 |  struct diag_isc_type
			    |        ------------V-----------
			    |        | struct buf     |     |
			    |        |                |     | 
			    -----------*b_sc          |     |
				 ------*b_queue       |     |
				 |   |                |     |
				 |   |-----------------     |
				 |   |                      |
				 |   |-----------------     |
				 |   | struct iobuf   |     |
				 ---->                |     |
				     |                |     |
				     |-----------------     |
				     |-----------------     |
				     |                |     |
				     | struct diag1_msg_type|
				     |                |     |
				     |                |     |
				     ------------------------

*****************************************************************************
*****************************************************************************
*/


int hshpib_diag_timedout();

#define CALL_DELAYED 1
#define CALL_DIRECT  2
#define DIAG1_TIMEDOUT 3
#define DIAG_DMA_TIMEDOUT (10 * HZ)

/**************************************************************
**
**  NAME	: hshpib_diag_reset
**
**  DESCRIPTION	:  This function is called by hshpib_diag()
**  when the current command is a DIAG_RESET.  A reset command
**  always succeeds.
**
**  PARAMETERS	:
**
**	bp - a pointer to the diagnostic buffer.
**
***************************************************************
*/
hshpib_diag_reset(bp)
struct buf *bp;
{
struct isc_table_type *sc = bp->b_sc;

     do_hshpib_reset(sc);

     sc->state &= ~IST;
     sc->ppoll_resp = 0;
     sc->transfer = NO_TFR;

     bp->b_s0 = DIAG_CMD_COMPLETE;

}  /* hshpib_diag_reset */



/**************************************************************
**
**  NAME	: hshpib_diag_identify
**
**  DESCRIPTION	: This function is called by hshpib_diag() when the
**  command is a DIAG_RESET.  It reads the four EISA_PROD_ID registers.
**
**  PARAMETERS	:
**
**	bp - a pointer to the diagnostic buffer.
**
***************************************************************
*/
hshpib_diag_identify(bp)
struct buf *bp;
{
int buf;
int *id_ptr;
int valid;
int tmpint;
int x;
HSHPIB *reg_ptr;
struct isc_table_type *isc_ptr = bp->b_sc;
struct diag_isc_type *dp       = (struct diag_isc_type *)bp;
int  data_len                  = dp->diag_msg.request_data.data_len;
space_t usr_sid, this_sid;
caddr_t this_vba, usr_vba;


	if( data_len < 4 )
	     valid = FALSE;
	else 
	     valid = TRUE;

	if( !valid )
	{
	  bp->b_s0 = DIAG_CMD_DENIED;
	}
	else
	{
	     buf = read_eisa_cardid(isc_ptr->if_reg_ptr);

	     /*
	        copy 4 bytes of id data to user space : (buf --> *data_ptr)
	     */
	     usr_sid = bvtospace( bp, bp->b_un.b_addr);
	     usr_vba = bp->b_un.b_addr + dp->diag_msg.request_data.data_vba;

	     this_sid = ldsid( (caddr_t)&buf );
	     this_vba = (caddr_t)&buf;

	     privlbcopy( this_sid, (caddr_t)this_vba, usr_sid, usr_vba, 4 );

	     tmpint = 4;
	     this_vba = (caddr_t)&tmpint;

	     usr_vba = (caddr_t)&(dp->diag_msg.trans_count);
	     privlbcopy(this_sid,(caddr_t)this_vba, KERNELSPACE, usr_vba, 4);


	     bp->b_s0 = DIAG_CMD_COMPLETE;
	}

}   /* hshpib_diag_identify */



/**************************************************************
**
**  NAME	: hshpib_diag_rw_reg
**
**  DESCRIPTION	: This function is called by hshpib_diag() to
**  perform a register read or write.  It checks to make sure
**  that the register number of the current commmand is valid
**  and then either writes the value to the register or reads
**  the value from the register.
**
**  PARAMETERS	:
**
**	bp - a pointer to the diagnostic buffer.
**
***************************************************************
*/
hshpib_diag_rw_reg(bp)
struct buf *bp;
{

char buf;
int tmpint;
int valid;
HSHPIB *reg_ptr;
struct isc_table_type *isc_ptr = bp->b_sc;
struct diag_isc_type *dp       = (struct diag_isc_type *)bp;
int  data_len                  = dp->diag_msg.request_data.data_len;
caddr_t usr_vba;
short reg                      = dp->diag_msg.cmd_option;
short cmd                      = dp->diag_msg.cmd;
caddr_t trans_count            = (caddr_t)&(dp->diag_msg.trans_count);
int x;
space_t usr_sid, this_sid;


	usr_vba = (bp->b_un.b_addr + dp->diag_msg.request_data.data_vba);

	if( ((reg >= IO_CARD_CONFIG) && (reg <= IO_HPIB_STATUS)) ||
	    ((reg >= MEDUSA_INTR_CON) && (reg <= MEDUSA_PPOLL_SENSE)) ||
	    (reg == EISA_CONTROL) )
		    valid = 1;
	else
		    valid = 0;

	if( isc_ptr->card_type == HP25560)
	     reg_ptr = (HSHPIB *)isc_ptr->if_reg_ptr;
	else
	     valid = 0;

	if( data_len < 1)
	     valid = 0;

	usr_sid = bvtospace( bp, bp->b_un.b_addr);
	this_sid = ldsid( (caddr_t)&buf );

	if( valid )
	{

	     if( cmd == HPIB_READ_REG )
	     {
	          buf = reg_ptr->r[ reg ];
	          privlbcopy( this_sid, (caddr_t)&buf, usr_sid, usr_vba, 1);

	     }
	     else  /* cmd == HPIB_WRITE_REG */
	     {
	         privlbcopy( usr_sid, usr_vba, this_sid, (caddr_t)&buf, 1 );
		 reg_ptr->r[ reg ] = buf;
	     }

	    /* 
	      one byte transferred.
	    */
	    tmpint = 1;
	    bp->b_s0 = DIAG_CMD_COMPLETE;
	    dp->diag_msg.cmd_status = DIAG_SUCCESSFUL;
	}
	else
	{
	    /* 
	      no bytes transferred.
	    */
	    tmpint = 0;
	    bp->b_s0 = DIAG_CMD_DENIED;
	    dp->diag_msg.cmd_status = DIAG_UNSUCCESSFUL;
	}

	privlbcopy(this_sid, (caddr_t)&tmpint, KERNELSPACE, trans_count, 4);

}  /* hshpib_diag_rw_reg */
		   



hshpib_diag_check_io( cmd, sc )
short cmd;
struct isc_table_type *sc;
{
	HSHPIB *cp = (HSHPIB *)sc->if_reg_ptr;
	unsigned short *regoff = python_med_regoff;
	unsigned char *med_status = &cp->r[regoff[MED_STATUS]];
	int valid;

	if( !(*med_status & M_HPIB_CTRL) )
		valid = 0;
	else
	{
		valid = 1;
		sc->state |= ACTIVE_CONTROLLER;
	}

	return valid; 

} /* hshpib_diag_check_io */



enum hshpib_diag_state {diag_hshpib_initial = 0,
			diag_hshpib_do_cmd,
			diag_hshpib_do_transfer,
			diag_hshpib_end_transfer,
			diag_hshpib_timedout,
			diag_hshpib_finish,
			diag_hshpib_oh_no };
		

/**************************************************************
**
**  NAME	: hshpib_diag
**
**  DESCRIPTION	:   This function is called by the diagnostic device driver
**  to execute a diagnostic command.  It is a finite state machine.  The job
**  of the initial state is to insure that the driver has control of the card.
**  by checking if the card is locked.  If the card is not locked, it queues
**  up the bp in the select code queue.  Later when the bp becomes owner of the
**  card this routine is reinvoked with the command state as the next state.
**  If the card is locked, (bp is owner of the card), then it proceeds 
**  directly to the command state.  The command state is the state that
**  executes the command.
**  The following diagnostic commands are supported:
**
**	DIAG_LOCK
**	DIAG_UNLOCK
**	DIAG_IDENIFY
**	DIAG_RESET
**	HPIB_READ_REG
**	HPIB_WRITE_REG
**	HPIB_INPUT
**	HPIB_OUTPUT
**
**  Some commands can be completed before this routine returns to the caller
**  (diag1_strategy).  Other commands may be queued up for completion at a
**  later time.  If a command cannot be completed when hshpib_diag() returns
**  then the diagnostic device driver must wait until either the command 
**  completes or is denied.  It does this by calling sleep.  Later when
**  the command completes hshpib_diag() must call wakeup to awaken the sleeping
**  diagnostic device driver.
**       The fields bp->b_s0, bp->b_s4 and bp->b_action2 are used to syncronize
**  the diagnostic device driver with the interface driver.  The field 
**  bp->b_s0 is used by the hshpib_diag() routine to communicate directly with
**  diagnostic device driver.  It reports the status of the command to the
**  driver.  If the command is queued up for completion at a later time then
**  it is set to DIAG_CMD_PENDING.  When the driver see this value it sleeps.
**  Later when the command completes, the field is set by hshpib_diag() to 
**  either DIAG_CMD_COMPLETE or DIAG_CMD_DENIED.
**       The field bp->b_s4 is used internally by the hshpib_diag() routine to
**  determine the status of the diagnostic command.  If the command is posted
**  as DIAG_CMD_PENDING, then bp->b_s4 is set to CALL_DELAYED.  This means that
**  the diagnostic device driver will sleep, waiting for command completion.
**  When the command completes, hshpib_diag() checks the value of bp->b_s4, if
**  bp->b_s4 == CALL_DELAYED then it calls (*bp->b_action2)() to wakeup the
**  sleeping diagnostic device driver.
**  bp->b_action2 is a pointer to the diagnostic device driver wakeup routine.
**
**       Every command has a timeout interval specified by the field "timeout"
**  in the current message.  If the command does not complete in the specified
**  time it is timed out.  The timeout is started in the initial state and
**  terminated in the last state (diag_hshpib_finish) using the macros
**  START_TIME and END_TIME.
**
**  The field bp->b_s1 is used to indicate whether or not the card is locked.
**  It is initialized by diag1_strategy() before calling hshpib_diag().
**
**
**  PARAMETERS	:
**      bp -  pointer to the diagnostic buffer.
**
***************************************************************
*/
hshpib_diag( bp )
struct buf *bp;

{

struct isc_table_type *sc    = bp->b_sc;
struct iobuf *iob            = bp->b_queue;
struct diag_isc_type *dp     = (struct diag_isc_type *)sc->g_drv_data->diag_drv;
struct buf *diag_bp            = (struct buf *)&(dp->diag_buf);
struct diag1_msg_type *cur_msg = (struct diag1_msg_type *)&(dp->diag_msg);
short  cmd		       = cur_msg->cmd;
short  timedout		       = cur_msg->timeout;
int locked;
struct buf *tmp_bp;
int transfer_valid;
int count, x, tmpint;
space_t usr_sid, this_sid;
caddr_t usr_vba, trans_count;

if( sc->card_type != HP25560)
{
	diag_bp->b_s0       = DIAG_CMD_DENIED;
	/*
	cur_msg->cmd_status = DIAG_UNKNW_CARD;
	*/
	return( DIAG_UNSUCCESSFUL );
}


try
	START_FSM;

re_switch:

switch( (enum hshpib_diag_state)iob->b_state )
{
case diag_hshpib_initial:

	iob->b_state = diag_hshpib_do_cmd;


	if( ( diag_bp->b_s1 == DIAG_UNLOCK ) && ( cmd != DIAG_UNLOCK ) )
	{

		locked = diag_get_selcode( diag_bp, hshpib_diag );


 		if( !locked )
		{

		  START_TIME( hshpib_diag_timedout, timedout);

		     diag_bp->b_s4   = CALL_DELAYED;
		     diag_bp->b_s0   = DIAG_CMD_PENDING;

		     return(DIAG_SUCCESSFUL);
		}

	}
		diag_bp->b_s4   = CALL_DIRECT;
		goto re_switch;
	break;

case diag_hshpib_do_cmd:

     if( diag_bp->b_s4  == CALL_DELAYED )
     {
      END_TIME
     }

        /*
	   I not sure that this condition will happen but lets check for it
	   any how.
	*/
	if( ( diag_bp != sc->owner ) && ( cmd != DIAG_UNLOCK ) )
	{
		iob->b_state    = diag_hshpib_finish;
		diag_bp->b_s0   = DIAG_CMD_DENIED;
		diag_bp->b_s1   = DIAG_UNLOCK;
		goto re_switch;
	}
	switch (cmd)
	{
	    case DIAG_CHAIN_LOCK:
	    case DIAG_LOCK:
			iob->b_state    = diag_hshpib_finish;
			diag_bp->b_s1   = DIAG_LOCK;
			diag_bp->b_s0   = DIAG_CMD_COMPLETE;

			goto re_switch;

	    case DIAG_CHAIN_UNLOCK:
	      		hshpib_diag_reset( diag_bp );
	    case DIAG_UNLOCK:
			iob->b_state    = diag_hshpib_finish;
			diag_bp->b_s1   = DIAG_UNLOCK;
			diag_bp->b_s0   = DIAG_CMD_COMPLETE;

			goto re_switch;

	    case DIAG_RESET:
			iob->b_state = diag_hshpib_finish;
	      		hshpib_diag_reset( diag_bp );

			goto re_switch;

	    case DIAG_IDENTIFY:
			iob->b_state = diag_hshpib_finish;
			hshpib_diag_identify( diag_bp );

			goto re_switch;

	    case HPIB_READ_REG:
	    case HPIB_WRITE_REG:
			iob->b_state = diag_hshpib_finish;
			hshpib_diag_rw_reg( diag_bp );

			goto re_switch;

	    case HPIB_INPUT:
	    case HPIB_OUTPUT:

			transfer_valid = hshpib_diag_check_io( cmd, sc );
			count = cur_msg->request_data.data_len;

			if( ( !transfer_valid ) || (count > 8) || (count <= 0) )
			{
			     bp->b_s0     = DIAG_CMD_DENIED;
			     iob->b_state = diag_hshpib_finish;
			     cur_msg->cmd_status = DIAG_UNSUCCESSFUL;

			}
			else
			{
			     iob->b_state = diag_hshpib_do_transfer;
			}

			goto re_switch;

	    default:
			diag_bp->b_s0       = DIAG_CMD_DENIED;
			/*
			cur_msg->cmd_status = DIAG_UNKNW_CMD;
			*/
			return( DIAG_UNSUCCESSFUL );

	}  /* switch cmd */

case diag_hshpib_do_transfer:

	dp->bp_ptr = geteblk(count);
	if(dp->bp_ptr == NULL)
	{
	     bp->b_s0     = DIAG_CMD_DENIED;
	     iob->b_state = diag_hshpib_finish;
	     cur_msg->cmd_status = DIAG_ER_KALLOC;
	}
	else
	{
	     x = spl6();
	     sc->owner = dp->bp_ptr;
	     splx(x);

	     dp->bp_ptr->b_flags  = 0;
	     dp->bp_ptr->b_error  = 0;
	     dp->bp_ptr->b_dev    = 0;
	     dp->bp_ptr->b_offset = 0;
	     dp->bp_ptr->b_spaddr = KERNELSPACE;
	     dp->bp_ptr->b_sc     = sc;
	     dp->bp_ptr->b_queue  = diag_bp->b_queue;
	     dp->bp_ptr->b_bcount = count;

	     iob->b_xcount        = count;
	     iob->b_xaddr         = dp->bp_ptr->b_un.b_addr;
	     iob->b_state         = diag_hshpib_end_transfer;

	     sc->resid            = count;
	     sc->tfr_control      = USE_DMA;

	     if( cmd == HPIB_INPUT )
	          dp->bp_ptr->b_flags |= B_READ;

	     if( cmd == HPIB_OUTPUT)
	     {
	          usr_sid = bvtospace( diag_bp, diag_bp->b_un.b_addr);
	          usr_vba =(caddr_t)( diag_bp->b_un.b_addr + cur_msg->request_data.data_vba);
	          privlbcopy( usr_sid, usr_vba, KERNELSPACE, iob->b_xaddr, count);
	     }

	     diag_bp->b_s0 = DIAG_CMD_PENDING;
	     diag_bp->b_s4   = CALL_DELAYED;

	     hshpib_save_state( sc );

	     START_TIME( hshpib_diag_timedout, DIAG_DMA_TIMEDOUT);
	     hshpib_transfer( MUST_DMA, dp->bp_ptr, hshpib_diag);
        }

     	break;
case diag_hshpib_end_transfer:

        END_TIME

	if( dp->bp_ptr->b_error )
	{
	     diag_bp->b_error = EIO;
	     diag_bp->b_s0    = DIAG_CMD_DENIED;
	     cur_msg->cmd_status = DIAG_UNSUCCESSFUL;
	     tmpint = 0;
	}
	else
	{
	     diag_bp->b_s0    = DIAG_CMD_COMPLETE;
	     cur_msg->cmd_status = DIAG_SUCCESSFUL;
	     count = cur_msg->request_data.data_len;
	     tmpint = count;

	     if( cmd == HPIB_INPUT )  /* copy data back to diag buffer */
	     {
	          usr_sid = bvtospace( diag_bp, diag_bp->b_un.b_addr);
	          usr_vba =(caddr_t)( diag_bp->b_un.b_addr + cur_msg->request_data.data_vba);
	          privlbcopy( KERNELSPACE, iob->b_xaddr, usr_sid, usr_vba, count);
	     }
	}
	this_sid = ldsid( (caddr_t)&tmpint );
        trans_count = (caddr_t)&(dp->diag_msg.trans_count);
	privlbcopy(this_sid, (caddr_t)&tmpint, KERNELSPACE, trans_count, 4);

	x = spl6();
	sc->owner = diag_bp;
	splx(x);

	brelse( dp->bp_ptr );
	dp->bp_ptr = NULL;

	hshpib_restore_state( sc );
	iob->b_state = diag_hshpib_finish;

	goto re_switch;

case diag_hshpib_timedout:

	cur_msg->cmd_status = DIAG_TIMEOUT;
	escape( TIMED_OUT );

case diag_hshpib_finish:

	iob->b_state = diag_hshpib_oh_no;

	if( diag_bp->b_s4 == CALL_DELAYED )  /* if this command was delayed */
	{
		(*diag_bp->b_action2)(diag_bp); /* call diag wakeup()       */
	}

	if( diag_bp->b_s1 == DIAG_UNLOCK )  
	{
	      drop_selcode(diag_bp);
	      selcode_dequeue( sc );
	}

	break;

case diag_hshpib_oh_no      :  /* shouldn't reach this state   */
	panic ("bad diag1 hpib state");
	break;
default:
	     panic ("bad diag1 hpib state");

}  /* switch state */

END_FSM;
recover {

	ABORT_TIME;

	if( dp->bp_ptr != NULL )
	{
		x = spl6();
		sc->owner = diag_bp;
		splx(x);
		brelse( dp->bp_ptr );
	}

	dp->bp_ptr = NULL;

	diag_bp->b_s0   = DIAG_CMD_DENIED;

        if( diag_bp->b_s4 == CALL_DELAYED ) 
	        (*diag_bp->b_action2)(diag_bp);

	if( diag_bp->b_s1 == DIAG_UNLOCK )  
	{
	      drop_selcode(diag_bp);
	      selcode_dequeue( sc );
	}
}

}   /* hshpib_diag */



/**************************************************************
**
**  NAME	: hshpib_diag_timedout
**
**  DESCRIPTION	: This is the function that gets called when a
**  timeout occurs.  The timeout is set in the function hshpib_diag.
**
**  PARAMETERS	:
**
**	bp - a pointer to the diagnostic buffer.
**
***************************************************************
*/
hshpib_diag_timedout( bp )
struct buf *bp;
{
	TIMEOUT_BODY(iob->intloc, hshpib_diag, bp->b_sc->int_lvl, 0,
			diag_hshpib_timedout );

}   /* hshpib_diag_timedout */




/*
** linking and inititialization routines for the HP25560A EISA based HPIB card
*/

extern int (*eisa_attach)();
int (*python_saved_attach)();


/******************************python_attach******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
python_attach(id, sc)
int id;
struct isc_table_type *sc;
{
	unsigned char sys_controller = 0;
	struct python_data *if_data;
	register HSHPIB *cp=sc->if_reg_ptr;

	if ( (id&0xfffffff0) == PYTHON_ID ){/*mask off REV_CODE from EISA ID*/
	    hshpib_selectcodes[++number_of_hshpib] = sc;
	    if_data=sc->if_drv_data=(caddr_t)&python_data[number_of_hshpib];
	    /* initialize iosw, gfsw, ifsw */
	    sc->ifsw = sc->iosw = &hshpib_iosw;
	    /* set init routine pointer in isc_table */
	    sc->gfsw->init = hshpib_init;
	    sc->gfsw->diag = hshpib_diag;
		
	    /* Save away the python card's power up/EEPROM stuff */
	    if_data->pyt_cardcfg = cp->p.pyt_cardcfg;
	    if_data->pyt_romcfg = cp->p.pyt_romcfg;
	    if_data->pyt_dmacfg = cp->p.pyt_dmacfg;
	    if_data->pyt_intrcfg = cp->p.pyt_intrcfg;
	    if_data->med_imsk = cp->p.med_imsk;
	    if_data->med_ctrl = cp->p.med_ctrl;
	    if_data->med_address = cp->p.med_address;
	    if_data->med_ppmsk = cp->p.med_ppmsk;
	    if_data->med_ppsns = cp->p.med_ppsns;
	    if_data->eisa_ctrl = cp->p.eisa_ctrl;

	    sc->int_lvl = PYTHON_INT_LVL;	/* interrupt level is always 5 for EISA cards */

	    sys_controller = if_data->pyt_cardcfg & 1;
	    if (sys_controller) {
		if (!io_inform(
		"HP25560 High-Speed HP-IB Interface - system controller",
							sc, sc->int_lvl))
			return FALSE;
	    } else {
		if (!io_inform(
		"HP25560 High-Speed HP-IB Interface - non-system controller",
							sc, sc->int_lvl))
			return FALSE;
	    }

	    /* Initialize card type */
	    sc->card_type = HP25560;

	    ((struct eisa_if_info *)(sc->if_info))->flags = INITIALIZED;
	}
#ifdef EISA_TEST
	eisa_test_cd = (struct eisa_test_card *)sc->if_reg_ptr;
	sc->if_reg_ptr = (caddr_t)&python_dummy_reg;
	eisa_test_cd->ememsel = 0x81;	/* enable 0x1000000 to 0x1ffffff, 16M to32M */
	p_eisa_mem_ptr = (caddr_t)0x1000000;
	v_eisa_mem_ptr = (caddr_t)map_mem_to_host(sc,0x1000000,0x1000000);
	eisa_test_cd->dmablen = 64;	/* maximum dma transfer */
	eisa_test_cd->eisa_cntl = 0x1;	/* enable card */
	eisa_test_cd->eirqen = 0x800;	/* enable IRQ3 for edge trigger */

	/* initialize BMIC */
	eisa_test_cd->iosel = 0xa0;
	v_bmic = (struct bmic_type *)map_isa_address(sc,0x100);
	v_bmic->local_index = 0x8;
	v_bmic->local_data = 2;
#endif
	return (*python_saved_attach)(id, sc);
}

#endif  /* hp90000s700 */

/******************************routine name******************************
** [short description of routine]
**
**	Called by:  
**
**	Parameters:	[description, if needed]
**
**	Return value:
**
**	Kernel routines called:
**
**	Global variables/fields referenced:
**
**	Global vars/fields modified:
**
**	Registers modified:
**
**	*** comment local variable usage ***
**
**	Algorithm:  [?? if needed]
************************************************************************/
hshpib_link()
{
#ifdef __hp9000s700
	python_saved_attach = eisa_attach;
	eisa_attach = python_attach;
#else
	simon_saved_make_entry = make_entry;
	make_entry = simon_make_entry;
#endif /* hp9000s700 */

	number_of_hshpib = -1;
}
