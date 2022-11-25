 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/ti9914s.s,v $
 # $Revision: 1.3.84.3 $	$Author: kcs $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/09/17 21:19:18 $
 # HPUX_ID: @(#)ti9914s.s	55.1		88/12/23


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
 #	Fast Handshake Transfer for the ti9914
 #
 #	ASSUMPTIONS
 #		1) Addressing has taken place.
 #		2) Hold off is ON.
 #		3) ATN is on.
 #		4) The device is ready to send/recieve data.
 #		5) No DMA is allocated to this selcode.
 #		6) sc->buffer has transfer buffer address and
 #		   sc->count has transfer count.
 #
 #	ENTRY
 #		Stack	address of internal status copy
 #			ternination address
 #			start of chain address
 #		   -->  return address
 #
 #	REGISTERS USED
 #		d7 Transfer count sc->count
 #		d6 int0stat value
 #		d5 BI/BO test value ($20/$10)
 #
 #		a5 pointer to select code
 #		a4 pointer to trasnfer buffer
 #		a3 pointer to cp->intstat0
 #		a2 pointer to cp->datain/dataout
 #
 #
 #	EXIT
 #		d0 count of transfer or remaining bytes
 #
 #**************************************************************

	text
	global _fhs_in,_fhs_out		#| entry points for in and out routines
	global _fhs_timeout_exit	#|

	global _odd_parity		#| entry point for odd parity generator
	global _TI9914_fakeisr		#| highlevel routine to handle isrs
	global _TI9914_RHDF		#| entry point for release holdoff routine

 #
 #	register assignments
 





 



 
 #	offset to card registers
 #
       		set	OFF_AUX,6	#| offset from intstat 0 to auxcmd
 #
 #	offset to select code table
 #
      		set	INTCPY,INTCOPY+1
 #
 #	test values
 #
        	set	BI_VALUE,0x20
        	set	BO_VALUE,0x10
      		set	BI_BIT,5
      		set	BO_BIT,4
       		set	EOI_BIT,3
 #
 #	auxillery commands
 #
      		set	H_RHDF,0x02	#| release holdoff
       		set	H_HDFA0,0x03	#| turn off holdoff
       		set	H_HDFA1,0x83	#| turn on holdoff
       		set	H_HDFE0,0x04	#| off holdoff on end
       		set	H_HDFE1,0x84	#| holdoff on end
      		set	H_FEOI,0x08	#| assert EOI with byte
     		set	H_GTS,0x0B	#| off ATN
 #
 #	set up the registers as specified above
 #
setup:
	mov.l	36(%sp),%a5		#| pointer to select code
	mov.l  CARD_PTR(%a5),%a0	#| get card address
	lea	INTSTAT0(%a0),%a3	#| get intstat0 pointer
	lea	DATAIN(%a0),%a2		#| get datain/out pointer
	mov.l	BUFFER(%a5),%a4		#| get transfer buffer
	mov.l	COUNT(%a5),%d7		#| get transfer count
	clr.l	%d6			#| clear register
	clr.l	%d5			#|   "     "
	clr.l	%d0			#| clear for return value
	mov.b	&H_GTS,OFF_AUX(%a3)	#| off ATN
	rts				#| return
 #
 #	release holdoff routine
 #
_TI9914_RHDF:
	movm.l	%a3-%a5,-(%sp)			#| save registers
	mov.l	16(%sp),%a5			#| pointer to select code
	bclr	&3,STATE+2(%a5)			#| should we releasee holdoff?
	beq.b	RHDF_END			#| if not skip
	mov.l	CARD_PTR(%a5),%a4		#| get card address
	lea	INTSTAT0(%a4),%a3		#| get intstat0 pointer
	mov.b	&H_RHDF,OFF_AUX(%a3)		#| release it
RHDF_END:
	movm.l	(%sp)+,%a3-%a5			#| restore registers
	rts					#| return
 #
 #	fast handshake in routine
 #
_fhs_in:
	movm.l	%d5-%d7/%a2-%a5,-(%sp)		#| save registers
	jsr	setup				#| set up registers for transfer
	movq	&BI_VALUE,%d5			#| set up test values
	bclr	&EOI_BIT,INTCPY(%a5)		#| clear eoi flag if present
	mov.b	&H_HDFA0,OFF_AUX(%a3)		#| turn off holdoff
	mov.b	&H_HDFE1,OFF_AUX(%a3)		#| holdoff on end
	bclr	&3,STATE+2(%a5)			#| should we release holdoff?
	beq.b	fhs_start			#| if not skip
	mov.b	&H_RHDF,OFF_AUX(%a3)		#| release holdoff
 #
fhs_start:
 #
 # START_POLL
 #
	mova.l	OWNER(%a5),%a0			#| get the bp
	mova.l	B_QUEUE(%a0),%a0		#| get the iob
	mov.l	%sp,%d0				#| have to mark the stack
	subq.l	&6,%d0				#| adjust for rts
	mov.l	%d0,MARKSTACK(%a0)		#| put this in iob->markstack
	tst.b	TIMEFLAG(%a0)			#| has a timeout occured?
	bne	_fhs_timeout_exit		#| yes, get out of here

	subq.l	&2,%d7				#| count - 2
	bge	fti_i1				#| test BI (n-1 loop)
	bra	fti_i2				#| test BI (one byte)
 #
 #	high speed for n-1 bytes
 #
fti_w1:

	mov.b	(%a3),%d6			#| get card status
	beq.b	fti_w1				#| loop till we get something
	cmp.b	%d5,%d6				#| BI status only
	bne.b	fti_s1				#| no, process other conditions

fti_t1:

	mov.b	(%a2),(%a4)+			#| get data byte
	dbra	%d7,fti_w1			#| loop until lower count
 #
 # 	last byte handling
 #
fti_w2:

	mov.b	(%a3),%d6			#| get card status
	beq.b	fti_w2				#| loop until we get something
	or.b	%d6,INTCPY(%a5)			#| save instat0 value
	cmp.b	%d5,%d6				#| BI status only?
	beq.b	fti_t2				#| yes, get the byte
	mov.b	%d6,%d0				#| make copy
	and.b	&0xc0,%d0			#| check for INT0/INT1
	beq.b	fti_i2				#| no interrupt, check for other

 #
 # END_POLL
 #
	mova.l	OWNER(%a5),%a0			#| get the bp
	mova.l	B_QUEUE(%a0),%a0		#| get the iob
	clr.l	MARKSTACK(%a0)			#| clear out iob->markstack

	jsr	fakeisr				#| else, process other status
 #
 # START_POLL
 #
	mova.l	OWNER(%a5),%a0			#| get the bp
	mova.l	B_QUEUE(%a0),%a0		#| get the iob
	mov.l	%sp,%d0				#| have to mark the stack
	subq.l	&6,%d0				#| adjust for rts
	mov.l	%d0,MARKSTACK(%a0)		#| put this in iob->markstack
	tst.b	TIMEFLAG(%a0)			#| has a timeout occured?
	bne	_fhs_timeout_exit		#| yes, get out of here

fti_i2:
	bclr	&BI_BIT,INTCPY(%a5)		#| see if BI was logged
	beq.b	fti_w2				#| no, keep waiting

fti_t2:
	mov.b	&H_HDFA1,OFF_AUX(%a3)		#| turn on holdoff
	mov.b	&H_HDFE0,OFF_AUX(%a3)		#| off end holdoff
	bset	&3,STATE+2(%a5)			#| we are in holdoff
	mov.b	(%a2),(%a4)+			#| get last byte
 #
 #	if we get here it was normal termination
 #
	movq	&0,%d7				#| a good termination
	or.b	&TR_COUNT,TFR_CONTROL(%a5)	#| set termination reason
	bclr	&EOI_BIT,INTCPY(%a5)		#| BI set, was EOI?
	beq	terminate			#| no, we are done
	or.b	&TR_END,TFR_CONTROL(%a5)	#| set termination reason
	bra	terminate			#| Yes, leave
 #
 #	special status handling n-l loop
 #
fti_s1:
	or.b	%d6,INTCPY(%a5)			#| save instat0 value
	mov.b	%d6,%d0				#| make copy
	and.b	&0xc0,%d0			#| check for INT0/INT1
	beq.b	fti_i1				#| no interrupt, check for other
 #
 # END_POLL
 #
	mova.l	OWNER(%a5),%a0			#| get the bp
	mova.l	B_QUEUE(%a0),%a0		#| get the iob
	clr.l	MARKSTACK(%a0)			#| clear out iob->markstack

	jsr	fakeisr				#| process other conditions
 #
 # START_POLL
 #
	mova.l	OWNER(%a5),%a0			#| get the bp
	mova.l	B_QUEUE(%a0),%a0		#| get the iob
	mov.l	%sp,%d0				#| have to mark the stack
	subq.l	&6,%d0				#| adjust for rts
	mov.l	%d0,MARKSTACK(%a0)		#| put this in iob->markstack
	tst.b	TIMEFLAG(%a0)			#| has a timeout occured?
	bne	_fhs_timeout_exit		#| yes, get out of here
fti_i1:
	bclr	&BI_BIT,INTCPY(%a5)		#| BI logged?
	beq	fti_w1				#| if not, keep testing status
	bclr	&EOI_BIT,INTCPY(%a5)		#| BI set, was EOI?
	beq	fti_t1				#| no, go transfer the byte
	mov.b	(%a2),(%a4)			#| get last byte
	addq.l	&1,%d7				#| adjust transfer count
	mov.b	&H_HDFA1,OFF_AUX(%a3)		#| turn on holdoff
	mov.b	&H_HDFE0,OFF_AUX(%a3)		#| off end holdoff
	or.b	&TR_END,TFR_CONTROL(%a5)	#| set termination reason
	bset	&3,STATE+2(%a5)			#| we are in holdoff
	bra	terminate			#| leave
 #
 #  FAKEISR
 #		need to restore registers after this
 #
fakeisr:
	or.b	%d6,INTCPY(%a5)			#| save instat0 value
	mov.l	%a5,-(%sp)			#| push sc on stack
	jsr	_TI9914_fakeisr			#| call isr
	addq.l	&4,%sp				#| adjust sp
	or.b	INTCPY(%a5),%d6			#| save instat0 value
	rts
 #
 #
 #	fast handshake out routine
 #
_fhs_out:
	movm.l	%d5-%d7/%a2-%a5,-(%sp)		#| save registers
	jsr	setup				#| set up registers for transfer
	movq	&BO_VALUE,%d5			#| set up test values
 #
 #
 # START_POLL
 #
	mova.l	OWNER(%a5),%a0			#| get the bp
	mova.l	B_QUEUE(%a0),%a0		#| get the iob
	mov.l	%sp,%d0				#| have to mark the stack
	subq.l	&6,%d0				#| adjust for rts
	mov.l	%d0,MARKSTACK(%a0)		#| put this in iob->markstack
	tst.b	TIMEFLAG(%a0)			#| has a timeout occured?
	bne	_fhs_timeout_exit		#| yes, get out of here

	subq.l	&2,%d7				#| count - 2
	bge.b	fto_t1				#| loop n-1 times
	bra	fto_last			#| send last byte
 #
 #	high speed loop for n-1 bytes
 #
fto_w1:
	mov.b	(%a3),%d6			#| get card status
	beq.b	fto_w1				#| loop untill we get something
	cmp.b	%d5,%d6				#| BO status only?
	bne	fto_s1				#| no, process other conditions
fto_t1:
	mov.b	(%a4)+,(%a2)			#| transfer byte out
	dbra	%d7,fto_w1			#| loop until lower count out
fto_w2:
	mov.b	(%a3),%d6			#| get card status
	beq.b	fto_w2				#| loop until we get something
	cmp.b	%d5,%d6				#| BO status only?
	beq.b	fto_last			#| yes, check if finished
	mov.b	%d6,%d0				#| make copy
	and.b	&0xc0,%d0			#| check for INT0/INT1
	beq.b	fto_i2				#| no interrupt, check for other

 #
 # END_POLL
 #
	mova.l	OWNER(%a5),%a0			#| get the bp
	mova.l	B_QUEUE(%a0),%a0		#| get the iob
	clr.l	MARKSTACK(%a0)			#| clear out iob->markstack

	jsr	fakeisr				#| process the other conditions
 #
 # START_POLL
 #
	mova.l	OWNER(%a5),%a0			#| get the bp
	mova.l	B_QUEUE(%a0),%a0		#| get the iob
	mov.l	%sp,%d0				#| have to mark the stack
	subq.l	&6,%d0				#| adjust for rts
	mov.l	%d0,MARKSTACK(%a0)		#| put this in iob->markstack
	tst.b	TIMEFLAG(%a0)			#| has a timeout occured?
	bne	_fhs_timeout_exit		#| yes, get out of here
fto_i2:
	bclr	&BO_BIT,INTCPY(%a5)		#| BO logged?
	beq.b	fto_w2				#| no, keep waiting
fto_last:
 #
 #	last byte handling
 #
 # check for EOI control here
 #
	mov.b	&H_FEOI,OFF_AUX(%a3)		#| send EOI with this byte
no_eoi:
	mov.b	(%a4)+,(%a2)			#| transfer last byte out
fto_w3:
	mov.b	(%a3),%d6			#| get card status
	beq.b	fto_w3				#| loop until we get something
	cmp.b	%d5,%d6				#| BO status only?
	beq.b	fto_out				#| yes, check if finished
	mov.b	%d6,%d0				#| make copy
	and.b	&0xc0,%d0			#| check for INT0/INT1
	beq.b	fto_i3				#| no interrupt, check for other

 #
 # END_POLL
 #
	mova.l	OWNER(%a5),%a0			#| get the bp
	mova.l	B_QUEUE(%a0),%a0		#| get the iob
	clr.l	MARKSTACK(%a0)			#| clear out iob->markstack

	jsr	fakeisr				#| process the other conditions
 #
 # START_POLL
 #
	mova.l	OWNER(%a5),%a0			#| get the bp
	mova.l	B_QUEUE(%a0),%a0		#| get the iob
	mov.l	%sp,%d0				#| have to mark the stack
	subq.l	&6,%d0				#| adjust for rts
	mov.l	%d0,MARKSTACK(%a0)		#| put this in iob->markstack
	tst.b	TIMEFLAG(%a0)			#| has a timeout occured?
	bne	_fhs_timeout_exit		#| yes, get out of here
fto_i3:
	bclr	&BO_BIT,INTCPY(%a5)		#| BO logged?
	beq.b	fto_w3				#| no, keep waiting
fto_out:
	movq	&0,%d7				#| a good termination
	bra.b	term_out
 #
 #	special status handling n-1 loop
 #
fto_s1:
	mov.b	%d6,%d0				#| make copy
	and.b	&0xc0,%d0			#| check for INT0/INT1
	beq.b	fto_i1				#| no interrupt, check for other

 #
 # END_POLL
 #
	mova.l	OWNER(%a5),%a0			#| get the bp
	mova.l	B_QUEUE(%a0),%a0		#| get the iob
	clr.l	MARKSTACK(%a0)			#| clear out iob->markstack

	jsr	fakeisr				#| process other conditions
 #
 # START_POLL
 #
	mova.l	OWNER(%a5),%a0			#| get the bp
	mova.l	B_QUEUE(%a0),%a0		#| get the iob
	mov.l	%sp,%d0				#| have to mark the stack
	subq.l	&6,%d0				#| adjust for rts
	mov.l	%d0,MARKSTACK(%a0)		#| put this in iob->markstack
	tst.b	TIMEFLAG(%a0)			#| has a timeout occured?
	bne	_fhs_timeout_exit		#| yes, get out of here
fto_i1:
	bclr	&BO_BIT,INTCPY(%a5)		#| BO logged?
	bne	fto_t1				#| yes, transfer the byte
	bra	fto_w1				#| else, keep waiting
 #
 #	FHS transfer termination
 #
term_out:
	or.b	&0x10,INTCPY(%a5)		#| reset BO so not lost
	mov.b	&0,TFR_CONTROL(%a5)		#| clear termination reason
	or.b	&TR_COUNT,TFR_CONTROL(%a5)	#| set termination reason
terminate:
	andi.b	&0xdf,INTCPY(%a5)		#| clear BI
 #	move.b	#h_dai0,OFF_AUX(a3)		#| re-enable interrupts
	mov.l	%d7,%d0				#| return value

_fhs_timeout_exit:
 #
 # END_POLL
 #
	mova.l	OWNER(%a5),%a0			#| get the bp
	mova.l	B_QUEUE(%a0),%a0		#| get the iob
	clr.l	MARKSTACK(%a0)			#| clear out iob->markstack

	movm.l	(%sp)+,%d5-%d7/%a2-%a5		#| restore registers
	rts					#| return
 #***********************************************************
 #	fast BO and BI routines
 #***********************************************************
 #
 # TI9914_bo(bp);	
 #
	text
	global _TI9914_bo	#| entry points for byte out routine
	global _TI9914_bi	#| byte in routine
	global _TI9914_fakeisr	#| highlevel routine to handle isrs

_TI9914_bo:
	movm.l	%d7/%a3-%a5,-(%sp)		#| save registers
	mova.l	20(%sp),%a5		#| get bp
	mova.l B_SC(%a5),%a4		#| get select code
	mova.l CARD_PTR(%a4),%a3		#| get card address
 #
 # try for a bo before we have to loop
 #
	movq	&0,%d7
 # sure we get the right part of INTCOPY
	mov.b	INTCPY(%a4),%d7		#| get current status
	or.b	17(%a3),%d7		#| get card status
	or.b	%d7,INTCPY(%a4)		#| update copy
	and.b	&0xd0,%d7		#| clear out other garbage
	cmpi.b	%d7,&0x10		#| BO status only?
	beq.b	got_bo			#| we got a bo on our first try
 # try again
	or.b	17(%a3),%d7		#| get card status
	or.b	%d7,INTCPY(%a4)		#| update copy
	and.b	&0xd0,%d7		#| clear out other garbage
	cmpi.b	%d7,&0x10		#| BO status only?
	beq.b	got_bo			#| we got a bo on our first try
 #
 # We will have to loop now for a while
 #
	mova.l	B_QUEUE(%a5),%a5		#| get iob
	mov.l	%sp,%d0			#| have to mark the stack
	subq.l	&6,%d0			#| adjust for rts
	mov.l	%d0,MARKSTACK(%a5)	#| put this in iob->markstack
	tst.b	TIMEFLAG(%a5)		#| has timeout occured?
	bne	time			#| yes, get out of here

bo_loop:
	or.b	INTCPY(%a4),%d7		#| get current status
	or.b	17(%a3),%d7		#| get card status
	beq.b	bo_loop			#| nothing there, wait
	or.b	%d7,INTCPY(%a4)		#| update copy
	cmpi.b	%d7,&0x10		#| BO status?
	beq.b	got_bo_l		#| Yes, leave
	mov.b	%d7,%d0			#| copy it
	and.b	&0xC0,%d0		#| INT0 or INT1 status?
	bne.b	fake_bo			#| Yes, take care of it
	btst	&4,%d7			#| Is BO set?
	bne.b	got_bo_l		#| Yes, leave
	bra.b	bo_loop
fake_bo:
	jsr	b_fake_isr		#| we got a isr
	bra.b	bo_loop

got_bo_l:
	clr.l	MARKSTACK(%a5)		#| clear out iob->markstack
got_bo:
	and.b	&0xed,INTCPY(%a4)	#| clear out bo
	movm.l	(%sp)+,%d7/%a3-%a5	#| restore registers
	rts

b_fake_isr:
	or.b	%d7,INTCPY(%a4)		#| save current status
	clr.l	MARKSTACK(%a5)		#| clear out iob->markstack
	pea	(%a4)			#| push select code
	jsr	_TI9914_fakeisr		#| call isr
	addq.l	&4,%sp			#| adjust sp
	mov.l	%sp,%d0			#| have to mark the stack
	subq.l	&6,%d0			#| adjust for rts
	mov.l	%d0,MARKSTACK(%a5)	#| put this in iob->markstack
	tst.b	TIMEFLAG(%a5)		#| has timeout occured?
	bne	time			#| yes, get out of here
	mov.b	INTCPY(%a4),%d7		#| get current status
	rts
 #
 # TI9914_bi routine
 #
_TI9914_bi:
	movm.l	%d7/%a3-%a5,-(%sp)		#| save registers
	mova.l	20(%sp),%a5		#| get bp
	mova.l B_SC(%a5),%a4		#| get select code
	mova.l CARD_PTR(%a4),%a3		#| get card address
 #
 # try for a bo before we have to loop
 #
	movq	&0,%d7
 # sure we get the right part of INTCOPY
	mov.b	INTCPY(%a4),%d7		#| get current status
	or.b	17(%a3),%d7		#| get card status
	or.b	%d7,INTCPY(%a4)		#| update copy
	and.b	&0xe0,%d7		#| clear out other garbage
	cmpi.b	%d7,&0x20		#| BI status only?
	beq.b	got_bi			#| we got a bi on our first try
 # try again
	or.b	17(%a3),%d7		#| get card status
	or.b	%d7,INTCPY(%a4)		#| update copy
	and.b	&0xe0,%d7		#| clear out other garbage
	cmpi.b	%d7,&0x20		#| BI status only?
	beq.b	got_bi			#| we got a bi on our second try
 #
 # We will have to loop now for a while
 #
	mova.l	B_QUEUE(%a5),%a5		#| get iob
	mov.l	%sp,%d0			#| have to mark the stack
	subq.l	&6,%d0			#| adjust for rts
	mov.l	%d0,MARKSTACK(%a5)	#| put this in iob->markstack
	tst.b	TIMEFLAG(%a5)		#| has timeout occured?
	bne.b	time			#| yes, get out of here

bi_loop:
	or.b	INTCPY(%a4),%d7		#| get current status
	or.b	17(%a3),%d7		#| get card status
	beq.b	bi_loop			#| nothing there, loop
	or.b	%d7,INTCPY(%a4)		#| update copy
	cmpi.b	%d7,&0x20		#| BO status?
	beq.b	got_bi_l		#| NO, try again
	mov.b	%d7,%d0			#| copy it
	and.b	&0xC0,%d0		#| INT0 or INT1 status?
	bne.b	fake_bi			#| NO, try again
	btst	&5,%d7			#| Is BO set?
	bne.b	got_bi_l		#| Yes, leave
	bra.b	bi_loop			#| nothing there, loop
fake_bi:
	jsr	b_fake_isr		#| we got a isr
	bra.b	bi_loop

got_bi_l:
	clr.l	MARKSTACK(%a5)		#| clear out iob->markstack
got_bi:
	and.b	&0xdd,INTCPY(%a4)	#| clear out bo
	bset	&3,STATE+2(%a4)		#| we are in holdoff
	movm.l	(%sp)+,%d7/%a3-%a5		#| restore registers
	rts

time:
	mov.l	&2,-(%sp)		#| put timeout value on stack
	jsr	_escape			#| leave, wont return ***************************************************************


 #***********************************************************
 #	fast BO and BI routines
 #***********************************************************
 #
 # TI9914_bo_intr(bp);	
 #
	text
	global _TI9914_bo_intr	#| entry points for byte out routine
	global _TI9914_bi_intr	#| byte in routine
	global _TI9914_ruptmask	#| high level enable interrupts routine

_TI9914_bo_intr:
	movm.l	%d6-%d7/%a3-%a5,-(%sp)	#| save registers
	mova.l	24(%sp),%a5		#| get bp
	mova.l B_SC(%a5),%a4		#| get select code
	mova.l CARD_PTR(%a4),%a3	#| get card address
 #
 # try for a bo before we have to loop
 #
	movq	&0,%d7
 # sure we get the right part of INTCOPY
	mov.b	INTCPY(%a4),%d7		#| get current status
	or.b	17(%a3),%d7		#| get card status
	or.b	%d7,INTCPY(%a4)		#| update copy
	and.b	&0xd0,%d7		#| clear out other garbage
	cmpi.b	%d7,&0x10		#| BO status only?
	beq	n_got_bo		#| we got a bo on our first try
 # try again
	or.b	17(%a3),%d7		#| get card status
	or.b	%d7,INTCPY(%a4)		#| update copy
	and.b	&0xd0,%d7		#| clear out other garbage
	cmpi.b	%d7,&0x10		#| BO status only?
	beq	n_got_bo			#| we got a bo on our first try
 #
 # We will have to loop now for a while
 #
	mov.l	&0x1000,%d6		#| patience
	mova.l	B_QUEUE(%a5),%a5	#| get iob
	mov.l	%sp,%d0			#| have to mark the stack
	subq.l	&6,%d0			#| adjust for rts
	mov.l	%d0,MARKSTACK(%a5)	#| put this in iob->markstack
	tst.b	TIMEFLAG(%a5)		#| has timeout occured?
	bne	n_time			#| yes, get out of here

n_bo_loop:
	or.b	INTCPY(%a4),%d7		#| get current status
	or.b	17(%a3),%d7		#| get card status
	and.b	&0xd0,%d7		#| clear out other garbage
	dbne	%d6,n_bo_loop		#| nothing there, wait until no patience
	or.b	%d7,INTCPY(%a4)		#| update copy
	cmpi.b	%d7,&0x10		#| BO status?
	beq.b	n_got_bo_l		#| Yes, leave
	mov.b	%d7,%d0			#| copy it
	and.b	&0xC0,%d0		#| INT0 or INT1 status?
	bne.b	n_fake_bo		#| Yes, take care of it
	btst	&4,%d7			#| Is BO set?
	bne.b	n_got_bo_l		#| Yes, leave

	cmpi.w	%d6,&0xffff		#| any patience left?
	bne	n_bo_loop			#| branch if so
	or.b	%d7,INTCPY(%a4)		#| save current status
	clr.l	MARKSTACK(%a5)		#| clear out iob->markstack
	or.l	&0x1,STATE(%a4)		#| we will wait for an interrupt
	pea     0x1.w			#| arg INT_ON for ruptmask
	pea     0x1000.w		#| arg TI_S_BO for ruptmask
	pea     (%a4)			#| arg sc for ruptmask
	jsr     _TI9914_ruptmask
	lea	0xc(%sp),%sp		#| pop args

	mov.l	&0,%d0			#| didnt get a bo - wait for interrupt
	movm.l	(%sp)+,%d6-%d7/%a3-%a5	#| restore registers
	rts

n_fake_bo:
	jsr	n_b_fake_isr		#| we got a isr
	bra.b	n_bo_loop

n_got_bo_l:
	clr.l	MARKSTACK(%a5)		#| clear out iob->markstack
n_got_bo:
	and.b	&0xed,INTCPY(%a4)	#| clear out bo
	mov.l	&1,%d0			#| got a byte out
	movm.l	(%sp)+,%d6-%d7/%a3-%a5	#| restore registers
	rts

n_b_fake_isr:
	or.b	%d7,INTCPY(%a4)		#| save current status
	clr.l	MARKSTACK(%a5)		#| clear out iob->markstack
	pea	(%a4)			#| push select code
	jsr	_TI9914_fakeisr		#| call isr
	addq.l	&4,%sp			#| adjust sp
	mov.l	%sp,%d0			#| have to mark the stack
	subq.l	&6,%d0			#| adjust for rts
	mov.l	%d0,MARKSTACK(%a5)	#| put this in iob->markstack
	tst.b	TIMEFLAG(%a5)		#| has timeout occured?
	bne	n_time			#| yes, get out of here
	mov.b	INTCPY(%a4),%d7		#| get current status
	rts
 #
 # TI9914_bi routine
 #
_TI9914_bi_intr:
	movm.l	%d6-%d7/%a3-%a5,-(%sp)	#| save registers
	mova.l	24(%sp),%a5		#| get bp
	mova.l B_SC(%a5),%a4		#| get select code
	mova.l CARD_PTR(%a4),%a3	#| get card address
 #
 # try for a bo before we have to loop
 #
	movq	&0,%d7
 # sure we get the right part of INTCOPY
	mov.b	INTCPY(%a4),%d7		#| get current status
	or.b	17(%a3),%d7		#| get card status
	or.b	%d7,INTCPY(%a4)		#| update copy
	and.b	&0xe0,%d7		#| clear out other garbage
	cmpi.b	%d7,&0x20		#| BI status only?
	beq	n_got_bi			#| we got a bi on our first try
 # try again
	or.b	17(%a3),%d7		#| get card status
	or.b	%d7,INTCPY(%a4)		#| update copy
	and.b	&0xe0,%d7		#| clear out other garbage
	cmpi.b	%d7,&0x20		#| BI status only?
	beq	n_got_bi			#| we got a bi on our second try
 #
 # We will have to loop now for a while
 #
	mov.l	&0x1000,%d6		#| patience
	mova.l	B_QUEUE(%a5),%a5	#| get iob
	mov.l	%sp,%d0			#| have to mark the stack
	subq.l	&6,%d0			#| adjust for rts
	mov.l	%d0,MARKSTACK(%a5)	#| put this in iob->markstack
	tst.b	TIMEFLAG(%a5)		#| has timeout occured?
	bne.b	n_time			#| yes, get out of here

n_bi_loop:
	or.b	INTCPY(%a4),%d7		#| get current status
	or.b	17(%a3),%d7		#| get card status
	and.b	&0xe0,%d7		#| clear out other garbage
	dbne	%d6,n_bi_loop		#| nothing there, loop while patient
	or.b	%d7,INTCPY(%a4)		#| update copy
	cmpi.b	%d7,&0x20		#| BI status?
	beq.b	n_got_bi_l		#| NO, try again
	mov.b	%d7,%d0			#| copy it
	and.b	&0xC0,%d0		#| INT0 or INT1 status?
	bne.b	n_fake_bi			#| NO, try again
	btst	&5,%d7			#| Is BO set?
	bne.b	n_got_bi_l		#| Yes, leave

	cmp.w	%d6,&0xffff		#| any patience left?
	bne	n_bi_loop			#| branch if so
	or.b	%d7,INTCPY(%a4)		#| save current status
	clr.l	MARKSTACK(%a5)		#| clear out iob->markstack
	or.l	&0x2,STATE(%a4)		#| we will wait for an interrupt
	pea     0x1.w			#| arg INT_ON for ruptmask
	pea     0x2000.w		#| arg TI_S_BI for ruptmask
	pea     (%a4)			#| arg sc for ruptmask
	jsr     _TI9914_ruptmask	#| enable BI interrupt
	lea	0xc(%sp),%sp		#| pop args
	mov.l	&0,%d0			#| didnt get a bi - wait for interrupt
	movm.l	(%sp)+,%d6-%d7/%a3-%a5	#| restore registers
	rts

n_fake_bi:
	jsr	n_b_fake_isr		#| we got a isr
	bra.b	n_bi_loop

n_got_bi_l:
	clr.l	MARKSTACK(%a5)		#| clear out iob->markstack
n_got_bi:
	and.b	&0xdd,INTCPY(%a4)	#| clear out bo
	bset	&3,STATE+2(%a4)		#| we are in holdoff
	mov.l	&1,%d0			#| got a byte in
	movm.l	(%sp)+,%d6-%d7/%a3-%a5	#| restore registers
	rts

n_time:
	mov.l	&2,-(%sp)		#| put timeout value on stack
	jsr	_escape			#| leave, wont return *******


 #
 #
 # used by ti9914 routines to talk with abi chips
 #
 # function odd_parity(i: integer): integer;
 #
_odd_parity:
	movm.l	%d1-%d2,-(%sp)	#save d1 and d2
	mov.l	12(%sp),%d0	#pop the argument

	mov.b	%d0,%d1		#7 6 5 4 3 2 1 0

	mov.b	%d1,%d2
	lsl.b	&1,%d2
	eor.b	%d2,%d1		#76 65 54 43 32 21 10 0

	mov.b	%d1,%d2
	lsl.b	&2,%d2
	eor.b	%d2,%d1		#7654 6543 5432 4321 3210 210 10 0

	mov.b	%d1,%d2
	lsl.b	&4,%d2
	eor.b	%d2,%d1		#76543210 6543210 543210 43210 3210 210 10 0

	andi.b	&0x80,%d1	#isolate the logical eight-bit difference
	eor.b	%d1,%d0		#generate even parity
	eor.b	&0x80,%d0	#switch it to odd parity
	movm.l	(%sp)+,%d1-%d2	#restore d1 and d2
	rts			#return

 #*********************************************************************
 #
 # hpib_send(bp, buffer, count);	
 #
	global _hpib_send	#| entry points for byte out routine
	global _odd_partab	#| table to generate odd parity

      	set	H_TON1,0x8a		#| enable to talk
      	set	H_TON0,0x0a		#| disable to talk
      	set	H_LON1,0x89		#| enable to listen
      	set	H_LON0,0x09		#| disable to listen
     	set	H_RLC,0x12		#| release control

   	set	UNT,0x5f		#| un-talk
   	set	UNL,0x3f		#| un-listen
   	set	TCT,0x09		#| take control

_hpib_send:
	movm.l	%d4-%d7/%a2-%a5,-(%sp)
	mova.l 36(%sp),%a5		#| get bp
	mova.l	B_SC(%a5),%a4		#| get selcode
	mova.l	B_QUEUE(%a5),%a2	#| get iob
	mov.b	MY_ADDRESS(%a4),%d5	#| get my address
	mova.l CARD_PTR(%a4),%a4	#| now put card address here
	mova.l	40(%sp),%a3		#| get buffer pointer
	mov.l	44(%sp),%d7		#| get count
 #
 # loop for the data
 #
loop:
	mov.b	(%a3)+,%d4		#| get data from buffer
	btst	&3,(%a2)		#| check for parity control
	beq	no_parity
	and.w	&0x7f,%d4		#| mask out parity
	mova.l	&_odd_partab,%a0	#| get odd parity table
	adda.w	%d4,%a0			#| get pointer to byte
	mov.b	(%a0),31(%a4)		#| write out the byte with odd parity
	bra	check_command
no_parity:
	mov.b	%d4,31(%a4)		#| write out the byte with no parity
check_command:
	mov.b	%d5,%d6			#| make copy of my address
	or.b	&0x20,%d6		#| make listen address
	cmp.b	%d4,%d6			#| see if my listen address
	bne.b	talker			#| no, try talker
	mov.b	&H_LON1,23(%a4)		#| tell dumb chip its a listener
	bra.b	leave			#| now get out of here
talker:
	eor.b	&0x60,%d6		#| change my addressing
	cmp.b	%d4,%d6			#| see if my talk address
	bne.b	unlisten		#| no, try unlisten
	mov.b	&H_TON1,23(%a4)		#| tell dumb chip its a talker
	bra.b	leave			#| now get out of here
unlisten:
	cmpi.b	%d4,&UNL		#| see if UNL
	bne.b	untalk			#| no, try untalk
	mov.b	&H_LON0,23(%a4)		#| tell dumb chip to unlisten
	bra.b	leave			#| now get out of here
untalk:
	cmpi.b	%d4,&UNT		#| see if UNT
	bne.b	tct			#| no, try TCT
	mov.b	&H_TON0,23(%a4)		#| tell dumb chip to untalk
	bra.b	leave			#| now get out of here
tct:
	cmpi.b	%d4,&TCT		#| see if TCT
	bne.b	otalk			#| no, try other talk
	mov.b	21(%a4),%d0		#| get address state
	btst	&1,%d0			#| in talk state?
	bne.b	otalk			#| no, try other talk address
	pea	(%a5)			#| push bp on stack
	jsr	_TI9914_bo		#| wait for BO
	addq.w	&4,%sp			#| adjust stack
	mov.b	&H_RLC,23(%a4)		#| tell chip to release control
	bra.b	bottom_loop		#| now get out of here
otalk:
	andi.b	&0x60,%d4		#| mask control bits
	cmpi.b	%d4,&0x40		#| other talk address?
	bne.b	leave			#| no, lets stop this
	mov.b	&H_TON0,23(%a4)		#| tell dumb chip its not a talker
leave:
	pea	(%a5)			#| push bp on stack
	jsr	_TI9914_bo		#| wait for BO
	addq.w	&4,%sp			#| adjust stack
	subq.l	&1,%d7			#| --count
	bne	loop			#| keep going
bottom_loop:
	movm.l	(%sp)+,%d4-%d7/%a2-%a5	#| restore registers
	rts

