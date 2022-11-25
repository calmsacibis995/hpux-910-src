 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/simons.s,v $
 # $Revision: 1.3.84.3 $	$Author: kcs $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/09/17 21:18:30 $
 # HPUX_ID: @(#)simons.s	55.1		88/12/23 


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
 #  simons.s  98625 card driver
 #


 #
 #  ref's
 #
		global	_dmachain,_sw_trigger,_simon_C_isr

		# these are unsigned shorts
		global	_dma_chan0_last_arm,_dma_chan1_last_arm


 #
 #  def's
 #
		global	_simon_isr

 #
 #  address register usage:
 #
 #	a0  (struct interrupt *)inf
 #	a1  (struct isc_table_type *)sc
 #	a2  (struct dma_channel *)dmatemps | (struct simon *)cp
 #	a3  (struct dma *)dmacard
 #	a4  (struct dma_chain *)dmachain[]
 #	a5  (struct dma_chain *)chain
 #


 #
 #  register setup
 #
_simon_isr:	mova.l	4(%sp),%a0		#inf

		movm.l	%a2-%a5,-(%sp)		#save registers
 #
 #  dma transfer in progress?
 #
		mova.l	TMP_OFFSET(%a0),%a1	#sc = inf->temp
		cmpi.w	TRANSFER+2(%a1),&DMA_TFR	#sc->transfer == DMA_TFR ?
		bne.w	normal			#branch if not

 #
 #  are we still chaining?
 #
		mova.l	DMA_CHAN(%a1),%a2		#dmatemps = sc->dma_chan

		mova.l	CARD(%a2),%a3		#dmacard = dmatemps->card

		lea	_dmachain,%a4		#&dmachain[0]
		lea	_dma_chan0_last_arm,%a1 # where to save the arm word
		mov.w	%a3,%d0			#are we using channel 0?
		beq.w	simon_br		#branch if so
		lea	_dma_chan1_last_arm,%a1 # where to save arm the word
		addq	&4,%a4			#&dmachain[1]
simon_br:	mova.l	(%a4),%a5			#chain

		btst	&0,7(%a3)		#dma channel armed?
		bne.w	normal			#branch if so

		cmpa.l	%a5,EXTENT(%a2)		#chain > dmatemps->extent ?
		bhi.w	normal			#branch if so

 #
 #  section for chaining  (see dmachain.s for detailed comments)
 #
		mova.l	(%a5)+,%a2		#i/o card register pointer

		addq	&1,%a5			#skip over the int level field

		mov.b	(%a5)+,(%a2)		#write the i/o card byte

		mov.l	(%a5)+,(%a3)+		#write dma address register

		mov.w	(%a5)+,(%a3)+		#write dma count
		mov.w	(%a5),(%a1)		#save the arm word
		mov.w	(%a5)+,(%a3)+		#arm the channel
		
		mov.l	%a5,(%a4)			#update chain pointer

end_isr:		movm.l	(%sp)+,%a2-%a5		#restore registers
		rts				#return


 #
 #  section for "calling" simon_C_isr:  do we software trigger or jsr?
 #
normal:		mova.l	TMP_OFFSET(%a0),%a1		#sc = inf->temp
		mova.l	CARD_PTR(%a1),%a2		#cp = sc->card_ptr

		mov.b 	MED_STATUS(%a2),INTCOPY(%a1)  	#save high order bits 
		mov	&M_8BIT_PROC,%d0		#prepare to...
		or.b	MED_CTRL(%a2),%d0		#set 8-bit processor mode bit
		clr.b 	MED_STATUS(%a2)  		#clear high-order bits
		mov.b	%d0,MED_CTRL(%a2)		#set control register
		mov.b	%d0,MED_CTRL(%a2)		#once again; set high-order bits

		mov.b	MED_IMSK(%a2),%d0		#read interrupt mask
		andi.b 	&~M_INT_ENAB,MED_STATUS(%a2)  	#clear interrupt enable
		mov.b	%d0,MED_IMSK(%a2)		#write back interrupt mask

		mov	%sr,%d0				#status register
		lsr	&8,%d0				#position the int level bits
		andi	&0x07,%d0			#isolate them
		cmp.b	%d0,INT_LVL(%a1)		#at or below the "normal" level?
		ble.w	jsr_it				#branch if so

		movq	&0,%d0
		mov.l	%d0,-(%sp)		#push software level
		mov.b	INT_LVL(%a1),%d0	#sc->int_lvl
		mov.l	%d0,-(%sp)		#push hardware level
		mov.l	%a0,-(%sp)		#push inf, arg to triggered rout
		pea	_simon_C_isr		#push routine to trigger
		pea	INTLOC(%a1)		#push &intloc
		jsr	_sw_trigger		#software trigger it
		add	&20,%sp			#pop arguments
		bra.w	end_isr			#terminate

jsr_it:		mov.l	%a0,-(%sp)		#push inf, arg to routine
		jsr	_simon_C_isr		#call it directly
		addq	&4,%sp			#pop argument

		bra.w	end_isr			#terminate
