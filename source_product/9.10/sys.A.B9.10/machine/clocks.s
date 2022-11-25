 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/clocks.s,v $
 # $Revision: 1.7.84.8 $	$Author: drew $
 # $State: Exp $   	$Locker:  $
 # $Date: 94/10/12 07:26:54 $

 # HPUX_ID: @(#)clocks.s	55.1		88/12/23

 #(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
 #(c) Copyright 1979 The Regents of the University of Colorado,a body corporate 
 #(c) Copyright 1979, 1980, 1983 The Regents of the University of California
 #(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
 #The contents of this software are proprietary and confidential to the Hewlett-
 #Packard Company, and are limited in distribution to those with a direct need
 #to know.  Individuals having access to this software are responsible for main-
 #taining the confidentiality of the content and for keeping the software secure
 #when not in use.  Transfer to any party is strictly forbidden other than as
 #expressly permitted in writing by Hewlett-Packard Company. Unauthorized trans-
 #fer to or possession by any unauthorized party may be a criminal offense.
 #
 #                    RESTRICTED RIGHTS LEGEND
 #
 #          Use,  duplication,  or disclosure by the Government  is
 #          subject to restrictions as set forth in subdivision (b)
 #          (3)  (ii)  of the Rights in Technical Data and Computer
 #          Software clause at 52.227-7013.
 #
 #                     HEWLETT-PACKARD COMPANY
 #                        3000 Hanover St.
 #                      Palo Alto, CA  94304

 #
 # The following are the address assignments for the clock chip:
 # 0x5f8001	control 3/1
 # 0x5f8003	control 2 / status
 #
 # 0x5f8005	counter 1 msb
 # 0x5f8007	counter 1 lsb
 #
 # 0x5f8009	counter 2 msb
 # 0x5f800B	counter 2 lsb
 #
 # 0x5f800D	counter 3 msb
 # 0x5f800F	counter 3 lsb
 #
 # output of counter 3 clocks counter 2
 #
 # counter 1 is set to continuous 16 bit mode and used for an interval timer
 # counter 2 is set to continuous 16 bit mode and used for a lost tick counter
 # counter 3 is set to dual 8 bit mode and used for a periodic timer
 # only counters 1 and 3 are allowed to interrupt (counter 1 only interrupts
 # when it is used to time an interval)
 #
 # control register definitions:
 #
 # control register 1 (write location 0x5f8001 and control register 1 selected)
 #	bit 7	output enable
 #	bit 6	interrupt enable for counter 1
 #	bit 5-3 mode bits
 #	bit 2   8/16 bit operation
 #	bit 1	clock source for counter 1
 #	bit 0	1 = chip reset
 # control register 2 (write location 0x5f8003)
 #	bit 7	output enable
 #	bit 6	interrupt enable for counter 2
 #	bit 5-3 mode bits
 #	bit 2   8/16 bit operation
 #	bit 1	clock source for counter 2
 #	bit 0	control 3/1 access 1=cr1 0=cr3
 # control register 3 (write location 0x5f8001 and control register 3 selected)
 #	bit 7	output enable
 #	bit 6	interrupt enable for counter 3
 #	bit 5-3 mode bits
 #	bit 2   clk source
 #	bit 1	clock source for counter 3
 #	bit 0	counter 3 prescale
 #
 # status register (read of location 0x5f8003)
 #	bit 7 	composite interrupt flag
 #		this bit is set as follows:
 #		INT = (i1 & CR1_6) | (i2 & CR2_6) | (i3 & CR3_6)
 #		where INT = composite interrupt flag
 #		i1 = timer #1 interrupt flag
 #		i2 = timer #2 interrupt flag
 #		i3 = timer #3 interrupt flag
 # Interrupts are cleared by a timer reset condition or
 # by a timer read counter command provided that the status register has
 # previously been read while the interrupt flag was set.
 #
 # Timers may be read at any time.  A write to the timer sets the preload
 # value (value to be counted down).

#define LOCORE
#include "../machine/cpu.h"     /* Get CPU relative numbers. */

         	set CLK_BASE,CPU_TIMER_REG	#start of clock registers

             	set CONT_REG1_OFF,0x0001	#control register 1 (write) (if selected by CR2)

             	set CONT_REG2_OFF,0x0003	#control register 2 (write)

             	set CONT_REG3_OFF,0x0001	#control register 3 (write) (if selected by CR2)

            	set STAT_REG_OFF,0x0003	#status register (read)

          	set TIMER1_OFF,0x0005	#timer1 counter latch (read)
            	set PRELOAD1_OFF,0x0005	#timer1 preload latch (write)

          	set TIMER2_OFF,0x0009	#timer2 counter latch (read)
            	set PRELOAD2_OFF,0x0009	#timer2 preload latch (write)

          	set TIMER3_OFF,0x000D	#timer3 counter latch (read)
            	set PRELOAD3_OFF,0x000D	#timer3 preload latch (write)

            	set	SELECT_CONT1,0x01	#stored into bit 0 of CR2 to select CR1
            	set	SELECT_CONT3,0x00	#stored into bit 0 of CR2 to select CR3

           	set	START_RESET,0x01	#stored into bit 0 of CR1 to start reset
         	set	END_RESET,0x00	#stored into bit 0 of CR1 to stop reset


 # TIMER 1 is enabled as follows:
 #	output disabled
 #	interrupts disabled
 #	continuous operation
 #	16 bit mode
 #
         	set	TIMER1_EN,0x00

 # TIMER 2 is enabled as follows:
 #	output disabled
 #	interrupts disabled
 #	continuous operation
 #	16 bit mode
 #	select control register 3
 #
         	set	TIMER2_EN,0x00

 # TIMER 3 is enabled as follows:
 #	output enabled
 #	interrupts enabled
 #	continuous operation
 #	dual 8 bit mode
 #
         	set	TIMER3_EN,0xc4

          	set	TIMER1_DIS,0x00		#disable from interrupting
          	set	TIMER2_DIS,0x00		#disable from interrupting
          	set	TIMER3_DIS,0x84		#disable from interrupting

            	set	TIMER3_PERIOD,0x13f9		#20ms (timer 3 is periodic timer)
                set     YO_TIMER3_PERIOD,0x1309            # 20ms / 25

	global	_clkstart,_clkrupt,_startrtclock,_tick_count,_get_precise_time
	global	_time,_inittodr

 #
 # _clkstart will:
 #	1) call isrlink to link in _clkrupt <NOTE: now moved to snoozeinit!>
 #	2) initialize the preload counter for timer 2 to 0
 #	3) initialize the "previous value" of timer 2 counter to 0
 #	4) initialize the preload counter for timer 3
 #	5) set the operating conditions for timers 2 and 3
 #	6) terminate the reset sequence started by _clkreset
 #  Known by both its Kleenix name and its 4.2 name
_clkstart:
_startrtclock:
	lea	CLK_BASE,%a0
	movq	&-1,%d0
	movp.w	%d0,PRELOAD2_OFF(%a0)		#set preload for timer 2
	movp.w	%d0,PRELOAD1_OFF(%a0)		#set preload for timer 1
	mov.w	%d0,_tick_count			#initialize the "previous value"
	mov.w	%d0,_YO_prev

	mov.l %a0,-(%sp)
	mov.l %a1,-(%sp)
	mov.l %d1,-(%sp)
	jsr	_YO_setup  # returns with desired timer 3 period in %d0
	mov.l (%sp)+,%d1
	mov.l (%sp)+,%a1
	mov.l (%sp)+,%a0

	movp.w	%d0,PRELOAD3_OFF(%a0)		#set preload for timer 3

	mov.b	&SELECT_CONT1,CONT_REG2_OFF(%a0)	#select control register 1
	mov.b	&END_RESET,CONT_REG1_OFF(%a0)	#terminate reset sequence

	mov.b	&SELECT_CONT1,CONT_REG2_OFF(%a0)	#select control register 1
	mov.b	&TIMER1_EN,CONT_REG1_OFF(%a0)	#enable timer1 (again)

	mov.b	&TIMER2_EN,CONT_REG2_OFF(%a0)	#enable timer2 (and select CR3)
	mov.b	&TIMER3_EN,CONT_REG3_OFF(%a0)	#enable timer3
	mov.l	&0,-(%sp)
	jsr	_inittodr
	addq.l  &4,%sp                  # pop the stack
	rts


 # YO setup sets up the system to use "YO" interrupts if it is necessary for
 # this machine.

	global _YO_setup

_YO_setup:
	tst.l _YO_never
	bne _YO_not_needed	# allow YO override

	tst.l _YO_always
	bne _YO_needed		# allow this to be forced

	cmp.l _processor,&3	# 3 means 68040 (ARGHHH!!!)
	bne _YO_not_needed


_YO_needed:
	# This code changes the interrupt vector for level 6 interrupts
	# to be YO_handler.  To do this it must un-writeproctect the pte
	# for the vector table to change the vector.  

	pea.l   _level_six_vec		# push logical address
	pea.l   _Syssegtab      	# push segment table pointer
	jsr     _tablewalk              # get the pte pointer
	addq.l  &8,%sp                  # pop the stack
	mov.l   %d0,%a0                 # put the pte ptr into %a0

	mov.l (%a0),%d0			# save away the pte
	and.l &NOT_PG_PROT,(%a0)        # clear write protect bit
	jsr _purge_tlb

	# now change the handler

	mov.l &_YO_handler,_level_six_vec

	mov.l %d0,(%a0) 		# restore the original pte
	jsr _purge_tlb

	# now we need to change the dio buserror timeout on bernie so that
	# the YO interrupt will occur before a buserror occurs.

	set BUS_ERROR_REGISTER,0x400007+LOG_IO_OFFSET

	mov.b &0xff,BUS_ERROR_REGISTER

	# Now set the YO_active flag and return the timer value

	mov.l	&1,_YO_active
	mov.w	&YO_TIMER3_PERIOD,%d0 	# return the fast YO time.

	rts

_YO_not_needed:
	clr.l   _YO_active
	mov.w	&TIMER3_PERIOD,%d0
	rts

 # YO handler.  This routine takes the very fast level 6 "Yo" interrupts
 # that will prevent cimmaron death.  
 # It does look odd, but trust me.  Before you consider removing it, 
 # get in touch with one of:
 #
 # 	Jim Brokish
 #	Brian Strope
 #	Dave Dahms
 # 	Bret Mckee
 #	Bruce Bigler
 #
 # Do not remove it if you don't understand why it was put here, 
 # no matter how stupid it looks and no matter how much testing you do
 # of the system without it.

      global  _YO_handler

      global _YO_always, _YO_never
      global _YO_count, _YO_active

      set YO_DIVISOR,25

      data
      lalign  4
_YO_count:    long  YO_DIVISOR
_YO_active:   long 0
_YO_always:   long 0
_YO_never:    long 0

      text
_YO_handler:

      btst    &2,CLK_BASE+STAT_REG_OFF           # test timer3 interrupt flag
      beq _not_YO # not us - handle it like usual

 # it was a YO interrupt - deal with it

      subq.l  &1,_YO_count
      beq _YO_cycle_done
      tst.b CLK_BASE+TIMER3_OFF #read counter to clear interrupt
      rte 

_YO_cycle_done:
      mov.l &YO_DIVISOR, _YO_count

      # fall through to the normal interrupt handler
      # because it is time for the system to see a clock tick

_not_YO:
      jmp _Interrupt    # do this like usual

 #
 # Clock isr.
 #
 # commented out:
 #*  Check to see if TIMER1 (the interval timer) is interrupting.
 #*  If it is then call the interval service routine.
 #*  Check for TIMER3 interrupt.
 #
 # Only invoked by TIMER3 interrupt
 # Save the value of the TIMER1 counter for use by _get_precise_time
 # Prepare a call to the following routines:
 #    hardclock(ticks, ps)
 #    softclock(ticks, ps)
 # Note - we only push the parameters once, under the assumption that no one 
 #        modifies these parameters during any of these calls
 #
 #  The following values are on the stack at the specified offsets
 #  when this is invoked.  Note that those offsets grow as this isr
 #  pushes values on the stack.
         	set	pushed_pc,42
         	set	pushed_ps,40


	text
_clkrupt:
	lea	CLK_BASE,%a0

 #
 #	btst	#0,STAT_REG_OFF(a0)		test timer1 interrupt flag
 #	jeq	_clkrupt1
 #	tst.b	timer1				rupt flag set. Do we care?
 #	jeq	_clkrupt1
 #	jsr	_clkinterval			yes we care!
 #_clkrupt1
 #	btst	#2,STAT_REG_OFF(a0)		test timer3 interrupt flag
 #	jeq	_clkrupt3			was it set?
	mov.b	&SELECT_CONT3,CONT_REG2_OFF(%a0)	#select control register 3
	mov.b	&TIMER3_DIS,CONT_REG3_OFF(%a0)	#disable timer 3
	movp.w	TIMER3_OFF(%a0),%d0		#read counter to clear interrupt
	movp.w	TIMER1_OFF(%a0),%d1		#read counter to save

	ori	&0x0700,%sr			#for fast level 6 dma chaining...
	andi	&0xfdff,%sr			#do the real clock work @ level 5
	mov.w	%d1,save_timer1			#save value for get_precise_time
	movp.w	TIMER2_OFF(%a0),%d1		#read tick counter

	tst.l	_YO_active
	beq	no_YO

 # we need to fix up a the vaule in %d1 to be in 20 ms ticks, negating
 # the YO effect.  This costs a couple of duplicated instructions but allows
 # things that groped _tick_count out of /dev/kmem to continue working
 # even with the YO effect active

	movq	&0,%d0				#clear the register
	mov.w _YO_prev,%d0			# get last TIMER_2 value
	sub.w %d1,%d0				# subtract it from current

	# %d0 now contains the number of YO ticks  - normalize it

	divu.w	&YO_DIVISOR,%d0 	# divide by YO_divisor 
	and.l	&0xffff,%d0		# ditch remainder
	mov.l 	%d0,%d1			# copy the answer to %d1

	# now we need to set _YO_prev to indicate the last value of 
	# TIMER_2 that was used - this prevents lost ticks

	mulu.w  &YO_DIVISOR,%d1
	sub.w	%d1,_YO_prev	# _YO_prev -= YO_DIVISOR*(TIMER_2/YO_DIVISOR)
				# remeber the counters count DOWN

	# %d0 contains the number of 20 ms ticks elapsed
	# now fix it up so it looks like we read it from a 20 ms TIMER_2

	mov.w _tick_count,%d1
	sub.w  %d0,%d1

no_YO:
	movq	&0,%d0				#clear the register
	mov.w	_tick_count,%d0			#get the previous value
	mov.w	%d1,_tick_count			#save it away
	sub.w	%d1,%d0				#get the difference
	mov.l	%d0,-(%sp)			#push tick count (parameter 3)
	movq	&0,%d1				#clear register
	mov.w	pushed_ps+4(%sp),%d1		#build 32-bit parameter from PS
	mov.l	%d1,-(%sp)			#push PS (parameter 2)
	mov.l	pushed_pc+8(%sp),-(%sp)		#push PC (parameter 1)
	jsr	_hardclock
	jsr	_softclock
	adda	&12,%sp				#pop 3 parameters

#ifdef DDB
	mov.l   _ddb_boot,%d1
	cmpi.l  %d1,&1				# ddb_boot == TRUE?
	bne.b   skipddb1
	jsr	_ddb_poll_ss			#check for DDB command
skipddb1:
#endif DDB

	ori	&0x0700,%sr			#get back to level 6 in case the 
	andi	&0xfeff,%sr			#clock wants to interrupt again.
 #						must prevent stack overflow.
	mov.b	&TIMER3_EN,CLK_BASE+CONT_REG3_OFF  #re-enable timer3
 #_clkrupt3
	rts

 #
 # set_interval, clkinterval are low level interfaces for interval timing.
 # It is assumed that a higher level mechanism will be built on
 # top of this (if needed).
 #
 #	globl	_set_interval,_clkinterval

 #
 # set_interval(interval)
 #	interval is in units of microseconds
 #
 #_set_interval
 #	rts

 #_clkinterval
 #	rts


 #	snooze and friends are used for short timing loops so that the
 #	time taken is independent of the processor speed.  Since snooze
 #	is used very early in the boot, it needs to turn on timer 1, but
 #	not the other ones.  All three timers are re-initialized when
 #	clkstart is called.

 #	snoozeinit, not clkstart, now links in the clock isr, to
 #	guarantee that its put at the head of the polling chain.
 #	This allows support of the 98625 card on interrupt level 6.

	global	_snooze,_snoozeinit

_snoozeinit:

 # call isrlink(isr,level,regaddr,mask,value,misc,tmp)
 #	isr = _clkrupt
 #	level = 6
 #	regaddr = STAT_REG
 #	mask = 0x80	bit 7 (composite interrupt flag)
 #	value = 0x80	(composite interrupt flag set)
 #	misc, tmp = 0	(not used by clock)
 #
	pea	0				#push tmp
	pea	0				#push misc
	pea	0x80				#push value
	pea	0x80				#push mask
	pea	CLK_BASE+STAT_REG_OFF		#push regaddr
	pea	6				#push interrupt level
	pea	_clkrupt			#push address of isr
	jsr	_isrlink			#link it in
	add.w	&28,%sp				#pop args to isrlink

	lea	CLK_BASE,%a0

	movq	&-1,%d0
	movp.w	%d0,PRELOAD1_OFF(%a0)		#set preload for timer 1

	mov.b	&SELECT_CONT1,CONT_REG2_OFF(%a0)	#select control register 1
	mov.b	&END_RESET+TIMER1_EN,CONT_REG1_OFF(%a0)	#terminate reset and go
	rts

_snooze:
	mov.l	%d2,%a1				#save d2
	lea	CLK_BASE,%a0
	clr.l	%d1
	movp.w	TIMER1_OFF(%a0),%d1		#initialize last counter value

 #	Initialize d0 as counter of 4usec ticks left to wait
	mov.l	4(%sp),%d0			#get argument
	addq.l	&3,%d0				#round up
	lsr.l	&2,%d0				#unsigned divide by 4

_snooze1S:	movp.w	TIMER1_OFF(%a0),%d2		#read current counter value
	sub.w	%d2,%d1				#how many ticks since last time?
	sub.l	%d1,%d0				#decrement counter
	bls.w	snooze_done
	mov.w	%d2,%d1				#save last counter value
	bra.w	_snooze1S

snooze_done:
	mov.l	%a1,%d2				#restore d2
	rts

 #
 #	get_precise_time -
 #
 #	called by gettimeofday() to supply precise time
 #	parameter is a pointer to a struct timeval (int sec; int usec;)
 #	Note - this will have problems if timer1 has wrapped all the way
 #	       around between two invocations of the clock ISR (about
 #	       1/4 second).  Given that this routine is only invoked from
 #	       gettimeofday system call (at level 0), this should be a
 #	       non-problem.  Hopefully 1/4 second hold-offs at level 6
 #	       are impossible as well.
 #

_get_precise_time:
	lea	CLK_BASE,%a0
	mov.l	4(%a7),%a1			#argument - ptr. to result
	clr.l	%d0
	mov.w	%sr,-(%sp)			#save status register
	mov.w	&0x2600,%sr			#start critical region
 #						get values from last interrupt:
	mov.l	_time,(%a1)+				#seconds
	mov.l	_time+4,(%a1)				#microseconds
	mov.w	save_timer1,%d0				#counter value
	movp.w	TIMER1_OFF(%a0),%d1		#get current counter value
	mov	(%sp)+,%sr			#end critical region
	sub.w	%d1,%d0				#get elapsed time from interrupt
 #						overflow handled correctly
	asl.l	&2,%d0				#convert from units of 4us to us
	add.l	%d0,(%a1)				#add in usec since last tick
	cmp.l	(%a1),&1000000			#check for overflow
	blt.w	no_overflow
	sub.l	&1000000,(%a1)			#correct microseconds
	addq.l	&1,-(%a1)			#correct seconds
no_overflow:
	rts

	bss
	lalign 2
_YO_prev:	space	2*(1)
	lalign	2
_tick_count:	space	2*(1)			#previous timer2 count
	lalign	2
save_timer1:	space	2*(1)			#timer1 count from last interrupt
 #	data
 #timer1		dc.b	0			expecting timer1 interrupts
#ifdef	FSD_KI
	data
	lalign	4
	text

 # time_now = get_4_usec_tick(); /* return 4 usec freerunning clock */

	global _get_4_usec_tick
	global _mfctl_CR_IT
	global _read_adjusted_itmr

_mfctl_CR_IT:
_read_adjusted_itmr:
_get_4_usec_tick:
	lea	CLK_BASE,%a0		# pointer to hardware clock registers
	movq	&0,%d0			# get ready for short extention
	movp.w	TIMER1_OFF(%a0),%d0	# get free running 16 bit 4 uS timer
	neg.w	%d0			# make forward counting clock
	rts

#endif	FSD_KI
