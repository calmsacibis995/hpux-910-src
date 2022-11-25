 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/dmachain.s,v $
 # $Revision: 1.7.84.3 $	$Author: kcs $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/09/17 21:11:53 $

 # HPUX_ID: @(#)dmachain.s	55.1		88/12/23

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


	global	_dma_here
	global	_lev7int
	global	_interrupt
	global	_dmachain
	global	_dbg_addr
	global	_dma32_chan0,_dma32_chan1
	global	_dma_chan0_last_arm,_dma_chan1_last_arm

#define LOCORE
#include "../machine/cpu.h"     /* Get CPU relative numbers. */

         set	DMA_BASE,DMA_CARD_BASE

 #
 # This code, along with the transfer code in hpib_ti.c and hpib_simon.c,
 # 'emulates' a chaining dma controller.  This psuedo controller does
 # array chaining (address, count pairs) and not linked list chaining.
 # Each entry in the chain has the following format:
 #      bytes   0-3:	address of io card register
 #	byte      4:	level at which interrupt should occur
 #	byte      5:	io card byte (used to modify io card state)
 #	bytes   6-9:	address of transfer buffer
 #      bytes 10-11:	transfer count
 #	bytes 12-13:	dma channel arm word
 #
 # The code in dma.c is called by the transfer routines for the
 # simon and TI9914 cards to setup the dma transfer chain.
 # There are two 'registers' associated with this 'controller'.  Each
 # contain the pointer to the transfer chain for its respective dma
 # channel (dmachain+0, dmachain+4). There is also a transfer count 'register'
 # for each channel.  The transfer routines use this register to see
 # if the total requested count was transferred. See the transfer routines
 # in hpib_ti.c and hpib_simon.c
 #
_lev7int:
		addq.l  &1,_interrupt	# charge to interrupt for profiler
		movm.l	%a0-%a2,-(%sp)	#save a0-a2
		tst.l	_dma_here	#is the dma card present?
		beq	nmi		#no!
		mova.l	&DMA_BASE,%a0	#pointer to dma channel 0
		btst	&1,7(%a0)	#is dma channel 0 interrupting?
		beq.w	dma1_next	#if not then try channel 1
		mova.l	_dmachain+0,%a1	#get pointer to channel 0 array
		mova.l	(%a1)+,%a2	#get pointer to i/o card register
 #
 # If this was level 7 (my level) we service it.  If not, the interrupt
 # must be coming from the other channel, so go on to channel 1.  This
 # situation occurs when a low level dma is being requested and both
 # it and a channel 1 chain occur nearly simultaneously.
 #
		cmp.b	(%a1)+,&7
		bne	dma1_next	
 #
 # NOTE: the following instruction will enable level 7 interrupts
 #       to be taken by the cpu
 #
		tst.w	(%a0)		#clear dma interrupt
 #
 # The chain pointer is incremented here because of a potential race
 # condition.  The condition arises because of the fact that the
 # dma controller can steal the total memory bandwidth, thus preventing
 # the processor from executing instructions.
 #
		add.l	&14,_dmachain+0	#increment chain pointer

 #
 # Test to see if we are using 32 bit dma.
 # If so, correct DMA_BASE
		tst.l	_dma32_chan0	#are we using 32 bit?
		beq	not32_0		#no!

		add.l	&0x100,%a0	#adjust pointer to dma32 channel 0
 #
 # NOTE: the following instruction is for the benefit of Simon; it
 #	1) removes the Simon interrupt request associated with DMA done
 #	2) preserves the high-order bits (but only if in 8-bit mode!)
 #
		tst.b	(%a2)		# for simon
		mov.b	(%a1)+,(%a2)	#write i/o card byte
		mov.l	(%a1)+,(%a0)+	#write dma address register

		mov.l	&0,%a2
		mov.w	(%a1)+,%a2
		mov.l	%a2,(%a0)+

		mov.w	(%a1),_dma_chan0_last_arm #save the arm word
		mov.w	(%a1)+,(%a0)+	#arm the channel
		movm.l	(%sp)+,%a0-%a2	#restore a0-a2
		subq.l  &1,_interrupt	# uncharge to interrupt for profiler
		rte

 #
 # NOTE: the following instruction is for the benefit of Simon; it
 #	1) removes the Simon interrupt request associated with DMA done
 #	2) preserves the high-order bits (but only if in 8-bit mode!)
 #
not32_0:	tst.b	(%a2)
		mov.b	(%a1)+,(%a2)	#write i/o card byte
		
		mov.l	(%a1)+,(%a0)+	#write dma address register
 #
 # NOTE: two move.w instructions are used instead of a move.l so that
 #	the order in which the two words are written is guaranteed
 #
		mov.w	(%a1)+,(%a0)+	#write dma count
		mov.w	(%a1),_dma_chan0_last_arm #save the arm word
		mov.w	(%a1)+,(%a0)+	#arm the channel
		movm.l	(%sp)+,%a0-%a2	#restore a0-a2
		subq.l  &1,_interrupt	# uncharge to interrupt for profiler
		rte

dma1_next:	addq.l	&8,%a0		#pointer to dma channel 1
		btst	&1,7(%a0)	#is dma channel 1 interrupting?
		beq.b	nmi		#if not then NMI interrupt
		mova.l	_dmachain+4,%a1	#get pointer to channel 1 array
		mova.l	(%a1)+,%a2	#get pointer to i/o card register
 #
 # If this was level 7 (my level) we service it.  If not, the level 7 
 # interrupt must be coming from an NMI.  We have already checked channel
 # 0, and it is not interrupting at level 7.  This channel is interrupting
 # at a level < 7.  
 #
		cmp.b	(%a1)+,&7
		bne	nmi	
 #
 # NOTE: the following instruction will enable level 7 interrupts
 #       to be taken by the cpu
 #
		tst.w	(%a0)		#clear dma interrupt

 #
 # The chain pointer is incremented here because of a potential race
 # condition.  The condition arises because of the fact that the
 # dma controller can steal the total memory bandwidth, thus preventing
 # the processor from executing instructions.
 #
		add.l	&14,_dmachain+4	#increment chain pointer

 #
 # Test to see if we are using 32 bit dma.  If so,
 # correct DMA_BASE
		tst.l	_dma32_chan1	#are we using 32 bit?
		beq	not32_1		#no!

		add.l	&0x1F8,%a0	#adjust pointer to dma channel 1
 #
 # NOTE: the following instruction is for the benefit of Simon; it
 #	1) removes the Simon interrupt request associated with DMA done
 #	2) preserves the high-order bits (but only if in 8-bit mode!)
 #
		tst.b	(%a2)		# for simon
		mov.b	(%a1)+,(%a2)	#write i/o card byte
		mov.l	(%a1)+,(%a0)+	#write dma address register
		mov.l	&0,%a2
		mov.w	(%a1)+,%a2
		mov.l	%a2,(%a0)+
		mov.w	(%a1),_dma_chan1_last_arm #save the arm word
		mov.w	(%a1)+,(%a0)+	#arm the channel
		movm.l	(%sp)+,%a0-%a2	#restore a0-a2
		subq.l  &1,_interrupt	# uncharge to interrupt for profiler
		rte

 #
 # NOTE: the following instruction is for the benefit of Simon; it
 #	1) removes the Simon interrupt request associated with DMA done
 #	2) preserves the high-order bits (but only if in 8-bit mode!)
 #
not32_1:	tst.b	(%a2)
		mov.b	(%a1)+,(%a2)	#write i/o card byte

		mov.l	(%a1)+,(%a0)+	#write dma address register
 #
 # NOTE: two move.w instructions are used instead of a move.l so that
 #	the order in which the two words are written is guaranteed
 #
		mov.w	(%a1)+,(%a0)+	#write dma count
		mov.w	(%a1),_dma_chan1_last_arm #save the arm word
		mov.w	(%a1)+,(%a0)+	#arm the channel
		movm.l	(%sp)+,%a0-%a2	#restore a0-a2
		subq.l  &1,_interrupt	# uncharge to interrupt for profiler
		rte

 # Nmi interrupts.  Decide what caused the non-maskable interrupt  
 #	and then do something with it.

	global _monitor_on,_panic
	global _nmi_link
	data
_nmi_link:	long	0
	text

 #
 # See if an HIL keyboard did it.
 #
nmi:		
 # Call eisa_nmi routine to check for EISA NMI interrupts.

	movm.l	%d0-%d2,-(%sp)		#save d0-2 (a0-a2 already saved)
	mov.l _eisa_nmi_routine,%a0
	jsr (%a0)
	movm.l	(%sp)+,%d0-%d2		#restore registers

	movm.l	%d0-%d2,-(%sp)		#save d0-2 (a0-a2 already saved)
		jsr	_hil_nmi_isr		#C routine to figure out HIL
		tst.l	%d0			#was it an HIL keyboard?
		movm.l	(%sp)+,%d0-%d2/%a0-%a2	#restore registers
		blt.w	go_debug		#hard reset, call the monitor
		bgt.w	leave_nmi		#we handled it
 #
 # 1 megabyte memory board (98257A) parity error
 #
nmi2:		
	movm.l	%d0-%d2/%a0-%a1,-(%sp)	#save d0-d2,a0-a1
		mov.l	&1,-(%sp)	#width
		mov.l	&(MEM_CTLR_BASE + 1),-(%sp) #address of memory control register +1 byte
		jsr	_testr		#check for bus error
		lea	8(%sp),%sp	#pop args
		tst.l	%d0		#1 = no bus error, 0 = bus error
		bne.w	nmi3		
 #
 # Backplane NMI
 #
		movq	&0,%d1		# clear high half of SR parameter
		mov.w	20(%sp),%d1	# get the status register saved value
		mov.l	22(%sp),%d0	# get the PC register saved value
		movm.l	%d0-%d1,-(%sp)	# and pass to gatherstats
		mov.l	_nmi_link,%d0	# get the profiling routine
		beq.b	nmi_panic	# if none, then panic		
		mov.l	%d0,%a0		# get the profiling routine address
		jsr	(%a0)		# and call gatherstats(PC, SR)
		lea	8(%sp),%sp	# pop 2 parameters
		movm.l	(%sp)+,%d0-%d2/%a0-%a1	#restore registers
leave_nmi:
		subq.l  &1,_interrupt	# uncharge to interrupt for profiler
		rte			#and return to caller

nmi_panic:
		lea	8(%sp),%sp	# pop 2 parameters
		movm.l	(%sp)+,%d0-%d2/%a0-%a1	#restore registers
		subq.l  &1,_interrupt	# uncharge to interrupt for profiler

		tst.l	_monitor_on	#if ram monitor present
		beq	go_panic		
go_debug:	subq.l	&1,_interrupt	# uncharge interrupt for SYSDEBUG
		mov.l	_dbg_addr,-(%sp)
		rts
go_panic:
		pea	backplane_nmi	#Handle Backplane nmis
		jsr	_panic
		jmp	.
 #
 # At this point we know that a parity error was generated.  Need to
 # find out where the error occurred.  First we must clear the current
 # nmi interrupt and then setup to scan memory for the bad location.
 # We take over the level 7 interrupt vector (nmi) and restore it.
 #
		global	_fault,_parity_error,_snooze

nmi3:		cmp.l	pend_parity,&0
		bne.w	nmi_handler
		mov.l	&1,pend_parity
		mov.w	&0,MEM_CTLR_BASE	#clear nmi interrupt
		mov.l	&1,-(%sp)
		jsr	_snooze
		lea	4(%sp),%sp
		mov.w	&1,MEM_CTLR_BASE	#re-enable parity checking
		mov.w	&0x2600,%sr	#drop interrupt level to allow other
 #					level 7 interrupts
 #
 # Get starting physical ram page and calculate total ram pages.
 #

		mov.l	_physmembase,%a0

		mov.l	_dos_mem_byte,%d1	# dos mem in bytes
		add.l	&NBPG-1,%d1		#round bytes to pages
		and.l	&~(NBPG-1),%d1		
		mov.l	&PGSHIFT,%d0
		lsr.l	%d0,%d1			#d1 has dos mem in pages 

		add.l	_physmem,%d1
		addq.l	&1,%d1		#for highpage
		and.w	&0xfffb,0x5f400e+LOG_IO_OFFSET #turn the data cache off
		bra.w	parity_2

parity_1:	mov.l	%a0,%d0
		movq	&PGSHIFT,%d2
		lsl.l	%d2,%d0
		mov.l   %d0,%a1
		mov.l	&NBPG/4,%d0
		bra.w	parity_4
parity_3:	mov.l	(%a1)+,%d2	#read location
parity_4:	dbf	%d0,parity_3
		addq.l	&1,%a0		#bump frame number
parity_2:	dbf	%d1,parity_1
		mov.l	&1,_trans_parity
		bra.w	nmi_handler1
 #
 # Fell through without finding the location of the parity error
 #

nmi_handler:	
	movm.l	(%sp)+,%d0-%d2/%a0-%a1	#restore registers
		addq.l	&8,%sp		#pop off most recent interrupt frame
nmi_handler1:	mov.l	&0,pend_parity
		mov.w	&0,MEM_CTLR_BASE	#clear nmi request
		mov.l	%a0,%d0		#get frame number
		movq	&PGSHIFT,%d1	#generate byte address
		lsl.l	%d1,%d0
		mov.l	%a1,%d1
		and.l	&PGOFSET,%d1
		or.l	%d1,%d0
		subq.l	&4,%d0		#-4 for post increment above
		mov.l	%d0,-(%sp)
		pea	28(%sp)		#push pointer to interrupt frame
		jsr	_parity_error
		lea	8(%sp),%sp
		jsr	_purge_tlb
		movm.l	(%sp)+,%d0-%d2/%a0-%a1	#restore registers
		or.w	&4,CPU_STATUS_REG	#turn cache on
		and.w	&0xf8ff,%sr	#lower priority
		and.w	&0xf000,6(%sp)	#clear vector offset field
		or.w	&RESCHED,6(%sp)	#or in reschedule trap
		mov.w	&1,MEM_CTLR_BASE	#enable parity detection
		jmp	_fault		#allow user process to terminate	

go_monitor:	
	movm.l	(%sp)+,%d0-%d1/%a0-%a1 #Restore registers
		subq.l  &1,_interrupt	# uncharge to interrupt for profiler
		mov.l	_dbg_addr,-(%sp)# address of monitor
		rts			# go to the monitor (will it return?)

		global	_trans_parity
		data
_trans_parity:	long	0
		text
	
		global	_testparity
_testparity:
		mov.w	&2,MEM_CTLR_BASE
		mov.l	4(%sp),%a0
		mov.l	8(%sp),(%a0)
		mov.l	&1,-(%sp)
		jsr	_snooze
		lea	4(%sp),%sp
		mov.w	&0,MEM_CTLR_BASE
		mov.l	&1,-(%sp)
		jsr	_snooze
		lea	4(%sp),%sp
		mov.w	&1,MEM_CTLR_BASE
		rts

		lalign	2
		data
pend_parity:	long	0
backplane_nmi:	byte	"Backplane NMI",0
fhs_nmi:	byte	"FHS NMI",0
