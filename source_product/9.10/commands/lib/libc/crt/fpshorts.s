# @(#) $Revision: 66.5 $       
#
	version 2
	global	___ftod,___dtof
	sglobal	_ftod,_dtof
	global	___itofl,___fltoi,___rndfl
	sglobal	_itofl,_fltoi,_rndfl
	global	___fmulf,___fdivf,___faddf,___fsubf
	sglobal	_fmulf,_fdivf,_faddf,_fsubf
	global	___ffadd,___ffsub,___ffmul,___ffdiv
	sglobal	_ffadd,_ffsub,_ffmul,_ffdiv
	global	___afaddf,___afsubf,___afmulf,___afdivf
	sglobal	_afaddf,_afsubf,_afmulf,_afdivf
	global	___fcmpf
	sglobal	_fcmpf

         	set	minuszero,0x80000000	#IEEE -0
       		set	SIGFPE,8		#Floating point exception

#*****************************************************************************
#
#  procedure    _ftod       
#
#  author:      paul beiser
#
#  revisions:   8/19/82
#               Paul Beiser 01/11/83 - Modified to handle -0 as input
#
#  description: convert a 32 bit ieee format floating point number to
#               a 64 bit one.
#
#  inputs:      32 bit floating point number on stack
#
#  outputs:     64 bit floating point number in (d0,d1)
#
#  errors:      none
#
#  errata:      uses only registers d0 and d1; no attempt is made to
#               check for ieee denormalized numbers, -0, infinities,
#               and NaNs.
#
___ftod:
_ftod:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l   &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a0
	mov.l	p_ftod(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_ftod,%a0
	jsr	mcount
')
	')
	link    %a6,&0
	move.l	(8,%a6),%d0	#get number
	move.l	%d0,%d1		#save number for sign test later
	beq.b	len2		#branch if 32 bit number is = 0
	cmp.l   %d0,&minuszero   #check for a -0
	bne.b   ftd_1           #branch if regular number 
           moveq   &0,%d0           #else return 0
	   moveq   &0,%d1
	   unlk    %a6
	   rts
ftd_1:   bclr	&31,%d0		#remove the sign
	lsr.l	&3,%d0		#make room for 11 bit exponent
	add.l	&0x38000000,%d0	#$380 is 3ff-7f (exp bias difference)
	tst.l	%d1		#place sign back in
	bpl.b	len1		#branch if no sign to replace
	bset	&31,%d0
len1:	and.l	&0x00000007,%d1	#extract 3 bits removed for exponent
	ror.l	&3,%d1
len2:	unlk     %a6
	rts			#return

#*****************************************************************************
#
#  procedure    _dtof
#
#  author:      paul beiser
#
#  revisions:   8/19/82
#               Paul Beiser 01/11/83 - Modified to handle -0 as input
#
#  description: convert a 64 bit ieee format floating point number to
#               a 32 bit one.
#
#  inputs:      64 bit number to convert is on the stack
#
#  outputs:     the result 32 bit number is returned in d0
#
#  errors:      Overflow is possible. An underflow returns a value of zero.
#
#  errata:      No attempt is made to check for ieee denormalized numbers, 
#               -0, infinities, and NaNs.
#
___dtof:
_dtof:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l   &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a0
	mov.l	p_dtof(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_dtof,%a0
	jsr	mcount
')
	')
	link    %a6,&0
#	trap	#2		test for stack space
#	dc.w	-4
	move.l	%d7,-(%sp)	#save register
	movem.l	(8,%a6),%d0-%d1	#get operand
	tst.l	%d0		#check for zero first
	beq.b	short3		#if zero, nothing to do
	cmp.l   %d0,&minuszero   #check for a -0
	beq.b   short3          #return a -0
	btst	&28,%d1		#check bit necessary for the round
	beq.b	short2		#no rounding is necessary if zero
	add.l	&0x10000000,%d1	#add 1 in the round bit
	bcc.b	short1		#branch if nothing to propagate
	   addq.l  &1,%d0	   #else propagate the carry
short1:	move.l	%d1,%d7		#check if halfway between 
	and.l	&0xfffffff,%d7	#remove 3 bits for the 32 bit result
	bne.b	short2		#if <> 0, normal round and done
	bclr	&29,%d1		#else have to clear the lsb (to even)
#
#  have the 64 bit number rounded correctly to 32 bits, 3 of which are
#  still in d1. note that the exponent has been incremented (if 
#  necessary) by the "addq.l #1,d0" if the round propagated a carry
#  that caused a mantissa overflow. now check the exponent size to
#  make sure it fits in the 8 bit field.
#
short2:	move.l	%d0,%d7		#extract the exponent
	swap	%d7		#place in low order word
	bclr	&15,%d7		#remove sign
	lsr.w	&4,%d7		#place exp in low 11 bits
	sub.w	&0x3ff-0x7f,%d7	#get the correct 32 bit biased exp
	ble.w  	shortunder	#64 bit number is to small
	cmp.w	%d7,&0xff	#check for too big
	bge.w  	shortover	#64 bit number is too big
#
#  now place the correct exponent into the number. the 3 bits in d1 
#  must be shifted over in d0 also.
#
	move.l	%d0,%d7		#save sign
	sub.l	&0x38000000,%d0	#$380 is 3ff-7f (exp bias difference)
	lsl.l	&3,%d0		#exp move; make room for 3 bits
	and.l	&0xe0000000,%d1	#mask off three bits
	rol.l	&3,%d1		#move to lower position
	or.b	%d1,%d0		#place the 3 bits in d0
	tst.l	%d7		#check sign
	bpl.b	short3		#if +, nothing more to do
	bset	&31,%d0		#else make negative
short3:	move.l	(%sp)+,%d7	#restore register
	unlk    %a6
	rts			#return

#*************************************************************************
#                                                                        *
# _itofl - convert 32 bit integer to 32 bit floating point               *
#                                                                        *
# call: integer is passed on the stack                                   *
# return: 32 bit floating point result is returned in d0                 *
#                                                                        *
#*************************************************************************

___itofl:
_itofl:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l   &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a0
	mov.l	p_itofl(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_itofl,%a0
	jsr	mcount
')
	')
	link    %a6,&0
#	trap	#2		test for stack space
#	dc.w	-12
	movem.l	%d2-%d4,-(%sp)	#save registers
	move.l	(8,%a6),%d0	#get operand
	beq.b	sfloat7		#no work if zero
	move.l	%d0,%d1		#d1 is working register
	bpl.b	sfloat1		#get absolute value
	neg.l	%d1

sfloat1:	moveq	&31,%d3		#need to find msb
	move.l	%d1,%d2		#search by byte
	swap	%d2		#isolate most significant word
	tst.w	%d2		#in most significant bytes?
	beq.b	sfloat2		#check lower bytes if not
	clr.b	%d2		#check most significant byte
	tst.w	%d2		#is it here?
	bne.b	sfloat3		#branch if it is
	moveq	&23,%d3		#must be in second most significant
	bra.b	sfloat3
sfloat2:	moveq	&15,%d3		#msb is in lower word
	move.w	%d1,%d2		#isolate lower word
	clr.b	%d2		#isolate upper byte
	tst.w	%d2		#is it here?
	bne.b	sfloat3		#branch if it is
	moveq	&7,%d3		#must be in lower byte

sfloat3:	btst	%d3,%d1		#now scan bits
	dbne	%d3,sfloat3	#until found

	moveq	&23,%d2		#calculate shift
	sub.w	%d3,%d2		#number of left shifts

	bpl.b	sfloat5		#left or right?

	neg.w	%d2		#number of right shifts
	moveq	&-1,%d4		#mask for sticky bit
	lsl.w	%d2,%d4		#bits to mask
	not.w	%d4
	lsr.w	&1,%d4		#don't save round bit
	and.w	%d1,%d4		#sticky bit
	lsr.l	%d2,%d1		#shift it
	bcc.b	sfloat6		#round bit set?
	addq.l	&1,%d1		#yes: round up
	btst	&24,%d1		#overflow?
	beq.b	sfloat4		#branch if not
	lsr.l	&1,%d1		#adjust mantissa
	addq.w	&1,%d3		#adjust msb number
	bra.b	sfloat6
sfloat4:	tst.w	%d4		#check sticky bit
	bne.b	sfloat6		#should i have rounded?
	bclr	&0,%d1		#maybe not
	bra.b	sfloat6

sfloat5:	beq.b	sfloat6		#shift at all?
	lsl.l	%d2,%d1		#shift it left

sfloat6:	add.w	&0x7e,%d3	#add exponent bias
	add.l	%d0,%d0		#shift off sign
	roxr.b	&1,%d3		#put in sign
	roxl.w	&8,%d3
	swap	%d3		#move into place
	add.l	%d1,%d3		#include the mantissa

	move.l	%d3,%d0		#save the result
sfloat7:	movem.l	(%sp)+,%d2-%d4	#restore registers
	unlk    %a6
	rts

#***************************************************************************
#                                                                          *
#        _fltoi                                                            *
#                                                                          *
#        Convert a 32 bit floating point real to a 32 bit integer with     *
#        truncation                                                        *
#                                                                          *
#        input: 32 bit floating point number is passed on stack            *
#                                                                          *
#        output: 32 bit integer is returned in d0                          *
#									   *
#               Paul Beiser 01/11/83 - Modified to handle -0 as input	   *
#									   *
#***************************************************************************

___fltoi:
_fltoi:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a0
	mov.l	p_fltoi(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_fltoi,%a0
	jsr	mcount
')
	')
	link    %a6,&0
#	trap	#2		test for stack space
#	dc.w	-4
	move.l	%d2,-(%sp)	#save register
	move.l	(8,%a6),%d0	#argument
	beq.b	trunc5		#is it zero?
	cmp.l   %d0,&minuszero   #is it -0?
	bne.b   trunc0
           moveq   &0,%d0           #-0 returns integer value of 0
	   bra.w     trunc5
trunc0:	move.l	%d0,%d1		#save exponent
	move.l	%d0,%d2		#save sign
	and.l	&0x7fffff,%d0	#mantissa
	bset	&23,%d0		#hidden bit
	swap	%d1		#isolate exponent
	add.w	%d1,%d1		#remove sign bit
	lsr.w	&8,%d1		#d1.w <- exponent
	sub.w	&0x7f+23,%d1	#amount to shift left
	beq.b	trunc4		#branch if no shift
	cmp.w	%d1,&-24		#trunc to zero?
	bgt.b	trunc1
	   moveq   &0,%d0	   #result is zero
	   bra.b   trunc5	   #return
trunc1:	cmp.w	%d1,&8		#possible overflow?
	bgt.w  	intgrover
	blt.b	trunc2
	tst.l	%d2		#test sign
	bpl.w     intgrover
	cmp.l	%d0,&0x800000	#is it -2^31?
	bne.w     intgrover
trunc2:	tst.w	%d1		#left or right shift
	bgt.b	trunc3
	neg.w	%d1
	lsr.l	%d1,%d0		#right shift
	bra.b	trunc4		#done
trunc3:	lsl.l	%d1,%d0		#left shift
trunc4:	tst.l	%d2		#set sign
	bpl.b	trunc5
	neg.l	%d0		#make it negative
trunc5:	move.l	(%sp)+,%d2	#restore register
	unlk    %a6
	rts			#return

#***************************************************************************
#                                                                          *
#        _rndfl                                                            *
#                                                                          *
#        Convert a 32 bit floating point real to a 32 bit integer with     *
#        rounding to nearest                                               *
#                                                                          *
#        input: 32 bit floating point number is passed on stack            *
#                                                                          *
#        output: 32 bit integer is returned in d0                          *
#                                                                          *
#               Paul Beiser 01/11/83 - Modified to handle -0 as input      *
#									   *
#***************************************************************************
___rndfl:
_rndfl:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a0
	mov.l	p_rndfl(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_rndfl,%a0
	jsr	mcount
')
	')
	link    %a6,&0
#	trap	#2		test for stack space
#	dc.w	-4
	move.l	%d2,-(%sp)	#save register
	move.l	(8,%a6),%d0	#argument
	beq.b	round5		#is it zero?
	cmp.l   %d0,&minuszero   #is it -0?
	beq.b   round0
       	   moveq   &0,%d0           #-0 returns 0 as a result
	   bra.w     round5
round0:	move.l	%d0,%d1		#save exponent
	move.l	%d0,%d2		#save sign
	and.l	&0x7fffff,%d0	#mantissa
	bset	&23,%d0		#hidden bit
	swap	%d1		#isolate exponent
	add.w	%d1,%d1		#remove sign bit
	lsr.w	&8,%d1		#d1.w <- exponent
	sub.w	&0x7f+23,%d1	#amount to shift right
	beq.b	round4		#branch if no shift
	cmp.w	%d1,&-25		#round to zero?
	bgt.b	round1
	   moveq   &0,%d0	   #result is zero
	   bra.b   round5	   #return
round1:	cmp.w	%d1,&8		#possible overflow?
	bgt.w  	intgrover
	blt.b	round2
	tst.l	%d2		#test sign
	bpl.w  	intgrover
	cmp.l	%d0,&0x800000	#is it -2^31?
	bne.w  	intgrover
round2:	tst.w	%d1		#left or right shift
	blt.b	round3
	lsl.l	%d1,%d0		#left shift
	bra.b	round4		#done
round3:	neg.w	%d1
	lsr.l	%d1,%d0		#right shift
	bcc.b	round4		#rounding needed?
	addq.l	&1,%d0		#yes
round4:	tst.l	%d2		#set sign
	bpl.b	round5
	neg.l	%d0		#make it negative
round5:	move.l	(%sp)+,%d2	#restore register
	unlk    %a6
	rts			#return
#
# Error handlers.
#
intgrover: trap	&SIGFPE         #overflow
	move.l	(%sp)+,%d2	#restore register
	unlk    %a6
	rts

shortover: trap	&SIGFPE		#overflow
	move.l	(%sp)+,%d7	#restore register
	unlk    %a6
	rts

shortunder: moveq  &0,%d0		#underflow
	move.l	(%sp)+,%d7	#restore register
	unlk    %a6
        rts

#***********************************************************************
#                                                                      *
#   floating point multiply subroutine _afmulf                         *
#                                                                      *
#   call: afmulf(a,b) float *a,b;				       *
#   return: result is returned to *a                                   *
#                                                                      *
# Note: Although this function is designed to store its result         *
# indirectly through a pointer, calling code may also require the      *
# result to be found in register d0. Therefore, if any modifications   *
# are made to this code, be certain d0 is correct on exit.	       *
#								       *
#***********************************************************************

# Note: Although this function is designed to store its result indirectly 
# through a pointer, calling code may also require the result to be found 
# in register d0. Therefore, if any modifications are made to this code, 
# be certain d0 is correct on exit.
#
f_afmulf:fmov.l	&0x7400,%fpcr
	mov.l	(4,%sp),%a0
	fmov.s	(%a0),%fp0
	fsglmul.s (8,%sp),%fp0
	fmov.s	%fp0,(%a0)
	mov.l	(%a0),%d0
	rts

___afmulf:
_afmulf:	
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_afmulf(%a1),%a0
	bsr.l	mcount
	')
	mov.l	flag_68881(%a1),%a0
	tst.w	(%a0)			# 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_afmulf,%a0
	jsr	mcount
	')
	tst.w	flag_68881		# 68881 present?
')
	bne.b	f_afmulf
	link	%a6,&0
#	trap	#2
#	dc.w	-16
	move.l	(12,%a6),-(%sp)
	move.l	(8,%a6),%a0
	move.l	(%a0),-(%sp)
ifdef(`PIC',`
	bsr.l	_fmulf',`
	jsr	_fmulf
')
	addq.w	&8,%sp
	move.l	(8,%a6),%a0
	move.l	%d0,(%a0)
	unlk	%a6
	rts

#***********************************************************************
#                                                                      *
#   floating point multiply subroutine _fmulf                          *
#                                                                      *
#   call:   multiplier y and multiplicand x are pushed on stack        *
#   return: result is returned in d0                                   *
#                                                                      *
#   Modified by Paul Beiser 1/11/84 to handle -0 for inputs    	       *
#								       *
#***********************************************************************

f_fmulf:fmov.l	&0x7400,%fpcr
	fmov.s	(4,%sp),%fp0
	fsglmul.s (8,%sp),%fp0
	fmov.s	%fp0,%d0
	rts

rmul0:	moveq	&0,%d0		#answer is zero
	movem.l	(%sp)+,%d2-%d7	#restore registers
	unlk    %a6
	rts

___ffmul:
___fmulf:
_ffmul:
_fmulf:	
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_fmulf(%a1),%a0
	bsr.l	mcount
	')
	mov.l	flag_68881(%a1),%a0
	tst.w	(%a1)			# 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_fmulf,%a0
	jsr	mcount
	')
	tst.w	flag_68881		# 68881 present?
')
	bne.b	f_fmulf
	link    %a6,&0
#	trap	#2		test for stack space
#	dc.w	-24
	movem.l	%d2-%d7,-(%sp)	#save registers
	movem.l	(8,%a6),%d0-%d1	#get arguments
	cmp.l   %d0,&minuszero   #check for a -0
	beq.w     rmul0
	cmp.l   %d1,&minuszero   #check for a -0
	beq.w     rmul0
	move.l	%d0,%d7		#for sign
	beq.b	rmul0
	move.l	%d1,%d6		#for exponent
	beq.b	rmul0
	eor.l	%d1,%d7		#sign of result
	move.l	%d0,%d2		#for exponent
	move.l	&0x7f800000,%d5	#mask
	and.l	%d5,%d2		#exp1
	and.l	%d5,%d6		#exp2
	add.l	%d2,%d6		#d6 <- exponent of result bias 254
	sub.l	&0x3f800000,%d6	#adjust bias
	bcs.w	underflw	#underflow?
	bmi.w	overflw         #overflow?
	move.l	&0x7fffff,%d5	#mantissa mask
	and.l	%d5,%d0		#mant1
	and.l	%d5,%d1		#mant2
	addq.l	&1,%d5		#hidden bit
	or.l	%d5,%d0
	or.l	%d5,%d1
	move.w	%d0,%d2		#d2 <- a0*b0
	mulu	%d1,%d2
	swap	%d1		#d3 <- a0*b1
	move.w	%d1,%d3
	mulu	%d0,%d3
	swap	%d0		#d0 <- a1*b1
	move.w	%d0,%d4		#save
	mulu	%d1,%d0
	swap	%d1		#d1 <- a1*b0
	mulu	%d4,%d1
	add.l	%d3,%d1		#d1 <- a0*b1+a1*b0
	move.l	%d1,%d3		#prepare for addition
	swap	%d1
	clr.w	%d1
	clr.w	%d3
	swap	%d3
	add.l	%d1,%d2		#lower 32 bits
	addx.l	%d3,%d0		#upper 32 bits
	tst.w	%d0		#exponent correct?
	bmi.b	rmul1
	add.l	%d2,%d2		#shift left one
	addx.l	%d0,%d0
	bra.b	rmul2
rmul1:	add.l	%d5,%d6		#exponent adjust
	bmi.w  	overflw		#overflow?
rmul2:	lsl.l	&8,%d0		#normalize
	rol.l	&8,%d2
	move.b	%d2,%d0		#shifted bits
	clr.b	%d2		#clear moved bits
	add.l	%d2,%d2		#rounding needed?
	bcc.b	rmul4
	addq.l	&1,%d0		#round up
	tst.l	%d2		#should it have been?
	bne.b	rmul3
	bclr	%d5,%d0		#maybe not
rmul3:	cmp.l	%d0,&0x1000000	#overflow?
	blt.b	rmul4
	move.l	%d5,%d0		#shift back
	add.l	%d5,%d6		#adjust exponent
rmul4:	add.l	%d6,%d0		#include exponent
	bmi.w  	overflw	        #overflow?
	sub.l	%d5,%d0		#remove hidden bit
	cmp.l	%d0,%d5		#underflow?
	blt.w  	underflw
	tst.l	%d7		#fix sign
	bpl.b	rmul5
	bset	&31,%d0		#negative
rmul5:	movem.l	(%sp)+,%d2-%d7	#restore registers
	unlk    %a6
	rts

#***********************************************************************
#                                                                      *
#   floating point divide subroutine _afdivf			       *
#                                                                      *
#   call: afdivf(a,b) float *a,b;				       *
#   return: result is returned to *a                                   *
#                                                                      *
# Note: Although this function is designed to store its result         *
# indirectly through a pointer, calling code may also require the      *
# result to be found in register d0. Therefore, if any modifications   *
# are made to this code, be certain d0 is correct on exit.	       *
#								       *
#***********************************************************************

# Note: Although this function is designed to store its result indirectly 
# through a pointer, calling code may also require the result to be found 
# in register d0. Therefore, if any modifications are made to this code, 
# be certain d0 is correct on exit.
#
f_afdivf:fmov.l	&0x7400,%fpcr
	mov.l	(4,%sp),%a0
	fmov.s	(%a0),%fp0
	fsgldiv.s (8,%sp),%fp0
	fmov.s	%fp0,(%a0)
	mov.l	(%a0),%d0
	rts

___afdivf:
_afdivf:	
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_afdivf(%a1),%a0
	bsr.l	mcount
	')
	mov.l	flag_68881(%a1),%a0
	tst.w	(%a0)			# 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_afdivf,%a0
	jsr	mcount
	')
	tst.w	flag_68881		# 68881 present?
')
	bne.b	f_afdivf
	link	%a6,&0
#	trap	#2
#	dc.w	-16
	move.l	(12,%a6),-(%sp)
	move.l	(8,%a6),%a0
	move.l	(%a0),-(%sp)
ifdef(`PIC',`
	bsr.l	_fdivf',`
	jsr	_fdivf
')
	addq.w	&8,%sp
	move.l	(8,%a6),%a0
	move.l	%d0,(%a0)
	unlk	%a6
	rts

#***********************************************************************
#                                                                      *
#   floating point divide subroutine _fdivf                            *
#                                                                      *
#   call:   dividend and divisor are pushed on the stack               *
#   return: result is returned in d0                                   *
#                                                                      *
#   Modified by Paul Beiser 1/11/84 to handle -0 for inputs    	       *
#                                                                      *
#***********************************************************************

f_fdivf:fmov.l	&0x7400,%fpcr
	fmov.s	(4,%sp),%fp0
	fsgldiv.s (8,%sp),%fp0
	fmov.s	%fp0,%d0
	rts

___ffdiv:
___fdivf:
_ffdiv:
_fdivf:	
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_fdivf(%a1),%a0
	bsr.l	mcount
	')
	mov.l	flag_68881(%a1),%a0
	tst.w	(%a0)		# 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_fdivf,%a0
	jsr	mcount
	')
	tst.w	flag_68881	# 68881 present?
')
	bne.b	f_fdivf
	link    %a6,&0
#	trap	#2		test for stack space
#	dc.w	-24
	movem.l	%d2-%d7,-(%sp)	#save registers
	movem.l	(8,%a6),%d0/%d2	#get dividend and divisor
	move.l	%d2,%d1		#determine sign
	beq.w  	divzero		#is it division by zero?
	cmp.l   %d2,&minuszero   #check for a -0
	beq.w     divzero
	move.l	%d0,%d3		#to determine exponent
	bne.b	div1		#zero dividend?
divA0:      moveq   &0,%d0	   #yes
	   movem.l (%sp)+,%d2-%d7	   #restore registers
	   unlk    %a6
	   rts			   #done
div1:    cmp.l   %d0,&minuszero   #check dividend for -0
	beq.w     divA0           #branch if return a 0
	eor.l	%d0,%d1		#msb is result sign
	move.l	&0x7f800000,%d5	#exponent mask
	move.l	%d2,%d4		#save exponent
	and.l	%d5,%d3		#exp1
	and.l	%d5,%d4		#exp2
	sub.l	%d4,%d3		#result exponent w/o bias
	add.l	&0x3f800000,%d3	#add in bias
	bvs.w	overflw		#overflow?
	bmi.w	underflw	#underflow?

	move.l	&0x7fffff,%d5	#mantissa mask
	and.l	%d5,%d0		#mant1
	and.l	%d5,%d2		#mant2
	addq.l	&1,%d5		#hidden bit
	or.l	%d5,%d0
	or.l	%d5,%d2

	add.l	%d0,%d0		#adjust to give round bit
	move.l	%d0,%d4		#d4 will have upper quotient
	lsl.l	&8,%d2		#shift divisor
	move.w	%d2,%d3		#need to save lower 8 bits
	clr.w	%d2		#clear upper bits of divisor
	swap	%d2

	divu	%d2,%d4		#first divide (can't overflow)

	move.w	%d2,%d6		#upper two bytes of divisor
	mulu	%d4,%d6		#partial product

	move.w	%d3,%d7		#lower two bytes of divisor
	mulu	%d4,%d7		#another partial product

	move.w	%d7,%d1		#bottom end of product
	clr.w	%d7		#remove it
	swap	%d7		#align to add partials
	add.l	%d7,%d6		#add them

	neg.w	%d1		#subtract from dividend
	subx.l	%d6,%d0

	bpl.b	div3		#is quotient correct?
div2:	subq.w	&1,%d4		#incorrect quotient
	add.w	%d3,%d1		#readjust dividend
	addx.l	%d2,%d0
	bmi.b	div2		#okay yet?

div3:	swap	%d0		#adjust for second divide
	move.w	%d1,%d0
	move.l	%d0,%d5		#d5 will have lower quotient
	beq.b	div7		#shortcut if zero dividend

	divu	%d2,%d5		#second divide
	bvc.b	div4		#was there overflow?
	moveq	&-1,%d5		#quotient is $ff or less
	move.l	%d2,%d6		#need to subtract $10000 times
	swap	%d6		#divisor
	move.w	%d3,%d6
	sub.l	%d6,%d0
	bra.b	div6		#now adjust quotient

div4:	move.w	%d2,%d6		#upper two bytes of divisor
	mulu	%d5,%d6		#partial product

	move.w	%d3,%d7		#lower two bytes of divisor
	mulu	%d5,%d7		#another partial product?

	move.w	%d7,%d1		#bottom end of product
	clr.w	%d7		#remove it
	swap	%d7		#align to add partials
	add.l	%d7,%d6		#add them

	neg.w	%d1		#subtract from dividend
	subx.l	%d6,%d0

	bpl.b	div7		#is quotient correct?
div5:	subq.w	&1,%d5		#incorrect quotient
div6:	add.w	%d3,%d1		#readjust dividend
	addx.l	%d2,%d0
	bmi.b	div5		#okay yet?

div7:	swap	%d4		#combine quotients
	move.w	%d5,%d4

	clr.w	%d3		#fix exponent
	move.l	&0x800000,%d6	#hidden bit
	moveq	&25,%d7		#quotient overflow bit

	btst	%d7,%d4		#determine normalization shift
	beq.b	div8
	lsr.l	&1,%d4		#shift off sticky bit
	addx.b	%d1,%d1		#save it
	bra.b	div9
div8:	sub.l	%d6,%d3		#adjust exponent
	bmi.w  	underflw

div9:	lsr.l	&1,%d4		#remove round bit
	bcc.b	div11
	addq.l	&1,%d4		#round up
	btst	%d7,%d4		#overflow?
	beq.b	div10
	move.l	%d6,%d4		#shift right
	add.l	%d6,%d3		#adjust exponent
	bmi.w  	overflw
div10:	or.w	%d1,%d0		#check sticky bit
	bne.b	div11
	bclr	%d6,%d4		#maybe shouldn't have rounded

div11:	add.l	%d4,%d3		#combine mantissa and exponent
	bvs.w  	overflw		#overflow?
	sub.l	%d6,%d3		#remove hidden bit
	cmp.l	%d3,%d6		#underflow?
	blt.w  	underflw
	tst.l	%d1		#get sign
	bpl.b	div12
	bset	&31,%d3		#make negative

div12:	move.l	%d3,%d0		#result
	movem.l	(%sp)+,%d2-%d7	#restore registers
	unlk    %a6
	rts

#***********************************************************************
#                                                                      *
#   floating point addition subroutine _afaddf			       *
#                                                                      *
#   call: afaddf(a,b) float *a,b;				       *
#   return: result is returned to *a                                   *
#                                                                      *
# Note: Although this function is designed to store its result         *
# indirectly through a pointer, calling code may also require the      *
# result to be found in register d0. Therefore, if any modifications   *
# are made to this code, be certain d0 is correct on exit.	       *
#								       *
#***********************************************************************

# Note: Although this function is designed to store its result indirectly 
# through a pointer, calling code may also require the result to be found 
# in register d0. Therefore, if any modifications are made to this code, 
# be certain d0 is correct on exit.
#
f_afaddf:fmov.l	&0x7400,%fpcr
	mov.l	(4,%sp),%a0
	fmov.s	(%a0),%fp0
	fadd.s	(8,%sp),%fp0
	fmov.s	%fp0,(%a0)
	mov.l	(%a0),%d0
	rts

___afaddf:
_afaddf:	
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_afaddf(%a1),%a0
	bsr.l	mcount
	')
	mov.l	flag_68881(%a1),%a0
	tst.w	(%a0)		# 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_afaddf,%a0
	jsr	mcount
	')
	tst.w	flag_68881	# 68881 present?
')
	bne.b	f_afaddf
	link	%a6,&0
#	trap	#2
#	dc.w	-16
	move.l	(12,%a6),-(%sp)
	move.l	(8,%a6),%a0
	move.l	(%a0),-(%sp)
ifdef(`PIC',`
	bsr.l	_faddf',`
	jsr	_faddf
')
	addq.w	&8,%sp
	move.l	(8,%a6),%a0
	move.l	%d0,(%a0)
	unlk	%a6
	rts

#***********************************************************************
#                                                                      *
#   floating point subtract subroutine _afsubf                         *
#                                                                      *
#   call: afsubf(a,b) float *a,b;				       *
#   return: result is returned to *a                                   *
#                                                                      *
# Note: Although this function is designed to store its result         *
# indirectly through a pointer, calling code may also require the      *
# result to be found in register d0. Therefore, if any modifications   *
# are made to this code, be certain d0 is correct on exit.	       *
#								       *
#***********************************************************************

# Note: Although this function is designed to store its result indirectly 
# through a pointer, calling code may also require the result to be found 
# in register d0. Therefore, if any modifications are made to this code, 
# be certain d0 is correct on exit.
#
f_afsubf:fmov.l	&0x7400,%fpcr
	mov.l	(4,%sp),%a0
	fmov.s	(%a0),%fp0
	fsub.s	(8,%sp),%fp0
	fmov.s	%fp0,(%a0)
	mov.l	(%a0),%d0
	rts

___afsubf:
_afsubf:	
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_afsubf(%a1),%a0
	bsr.l	mcount
	')
	mov.l	flag_68881(%a1),%a0
	tst.w	(%a0)		# 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_afsubf,%a0
	jsr	mcount
	')
	tst.w	flag_68881	# 68881 present?
')
	bne.b	f_afsubf
	link	%a6,&0
#	trap	#2
#	dc.w	-16
	move.l	(12,%a6),-(%sp)
	move.l	(8,%a6),%a0
	move.l	(%a0),-(%sp)
ifdef(`PIC',`
	bsr.l	_fsubf',`
	jsr	_fsubf
')
	addq.w	&8,%sp
	move.l	(8,%a6),%a0
	move.l	%d0,(%a0)
	unlk	%a6
	rts

#***********************************************************************
#                                                                      *
#   floating point addition subroutine _faddf,_fsubf                   *
#                                                                      *
#   call:  augend x and addend y are pushed on stack for add           *
#          minuend x and subtrahend y are pushed on stack for subtract *
#   return: result is returned in d0                                   *
#                                                                      *
#   Modified by Paul Beiser 1/11/84 to handle -0 for inputs    	       *
#                                                                      *
#***********************************************************************

f_fsubf:fmov.l	&0x7400,%fpcr
	fmov.s	(4,%sp),%fp0
	fsub.s	(8,%sp),%fp0
	fmov.s	%fp0,%d0
	rts

f_faddf:fmov.l	&0x7400,%fpcr
	fmov.s	(4,%sp),%fp0
	fadd.s	(8,%sp),%fp0
	fmov.s	%fp0,%d0
	rts

___ffsub:
___fsubf:
_ffsub:
_fsubf:	
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_fsubf(%a1),%a0
	bsr.l	mcount
	')
	mov.l	flag_68881(%a1),%a0
	tst.w	(%a0)		# 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_fsubf,%a0
	jsr	mcount
	')
	tst.w	flag_68881	# 68881 present?
')
	bne.b	f_fsubf
	link    %a6,&0
#	trap	#2		test for stack space
#	dc.w	-24
	movem.l	%d2-%d7,-(%sp)	#save registers
	movem.l	(8,%a6),%d0-%d1	#get operands
	cmp.l	%d1,%d0		#are they the same?
	bne.b	srsub1
	moveq	&0,%d0		#yes: answer is zero
	bra	addsub8
srsub1:  cmp.l   %d1,&minuszero   #check second operand for -0
	beq.w     addsub8         #treat the same as 0
	tst.l	%d1		#second operand zero?
	beq	addsub8
	bchg	&31,%d1		#change sign of second and add
	bra.b	addsub		#all else is the same as add

___ffadd:
___faddf:
_ffadd:
_faddf:	
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_faddf(%a1),%a0
	bsr.l	mcount
	')
	mov.l	flag_68881(%a1),%a0
	tst.w	(%a0)		# 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_faddf,%a0
	jsr	mcount
	')
	tst.w	flag_68881	# 68881 present?
')
	bne.w	f_faddf
	link    %a6,&0
#	trap	#2		test for stack space
#	dc.w	-24
	movem.l	%d2-%d7,-(%sp)	#save registers
	movem.l	(8,%a6),%d0-%d1	#get operands
	cmp.l   %d1,&minuszero   #check second operand for -0
	beq.w     addsub8         #treat the same as 0
	tst.l	%d1		#second operand zero?
	beq	addsub8
	move.l	%d1,%d2		#quick check for zero result
	bchg	&31,%d2
	cmp.l	%d2,%d0		#are they the same?
	bne.b	addsub
	moveq	&0,%d0		#result is zero
	bra	addsub8
addsub:	tst.l	%d0		#first operand zero?
	bne.b	addsub1
addsubA1:   move.l  %d1,%d0	   #answer is second number
	   bra	   addsub8
addsub1: cmp.l   %d0,&minuszero   #check first operand for a -0
	beq.w     addsubA1        #return the second number as the result
	move.l	%d0,%d7		#determine if add or subtract
	eor.l	%d1,%d7		#bit 31 = subtract/not add
#
# TAKE ABSOLUTE VALUE OF ARGUMENTS
#
	move.l	&0x7fffff,%d6	#mantissa mask
	move.l	%d0,%d2		#save mantissa
	move.l	%d1,%d3		#save mantissa
	bclr	%d6,%d2		#abs(arg1)
	bclr	%d6,%d3		#abs(arg2)
#
# SWAP ARGUMENTS IF THE SECOND IS LARGER IN MAGNITUDE
#
	cmp.l	%d2,%d3		#which is larger?
	bge.b	addsub2
	exg	%d0,%d1		#exchange arguments
	exg	%d2,%d3		#exchange absolute values
#
# ISOLATE MANTISSAS
#
addsub2:	and.l	%d6,%d2		#mant1
	and.l	%d6,%d3		#mant2
	addq.l	&1,%d6		#hidden bit
	or.l	%d6,%d2
	or.l	%d6,%d3
#
# ISOLATE EXPONENTS
#
	move.l	&0x7f800000,%d5	#mask
	move.l	%d0,%d4		#save exponent
	and.l	%d5,%d4		#exp1
	and.l	%d1,%d5		#exp2

#
# DETERMINE SHIFT FOR MANTISSA ALIGNMENT
#
	moveq	&0,%d1		#(d1 is for shift overflow)
	sub.l	%d4,%d5		#exponent difference
	beq.b	addsub3		#shortcut if same
	neg.l	%d5		#make positive
	swap	%d5
	lsr.w	&7,%d5		#adjust field
	cmp.w	%d5,&25		#is second operand insignificant?
	bgt	addsub8
#
# ALIGN MANTISSAS
#
	moveq	&-1,%d1		#mask
	lsl.l	%d5,%d1		#bits of overflow
	not.l	%d1		#bits to move
	and.l	%d3,%d1		#move them
	ror.l	%d5,%d1
	lsr.l	%d5,%d3
#
# ADD OR SUBTRACT ?
#
addsub3:	tst.l	%d7		#add or subtract?
	bmi.b	sub1
#
# ADD
#
	add.l	%d3,%d2		#this is it!
	cmp.l	%d2,&0x1000000	#mantissa overflow?
	blt.b	addsub4
	lsr.l	&1,%d2		#realign mantissa
	roxr.l	&1,%d1
	add.l	%d6,%d4		#adjust exponent
	bra.b	addsub4
#
# SUBTRACT
#
sub1:	neg.l	%d1		#lower bits
	subx.l	%d3,%d2		#upper bits

	moveq	&23,%d5		#normalize
	move.l	%d2,%d3		#start at the top
	swap	%d3		#where is that msb?
	tst.b	%d3		#is it here?
	bne.b	sub2
	moveq	&15,%d5		#guess bits 15 - 8
	move.w	%d2,%d3		#check for it
	clr.b	%d3
	tst.w	%d3		#bits 15 - 8?
	bne.b	sub2		#branch if good guess
	moveq	&7,%d5		#bits 7 - 0
sub2:	btst	%d5,%d2		#is this bit set?
	dbne	%d5,sub2

	moveq	&23,%d3		#exponent adjustment
	sub.w	%d5,%d3
	beq.b	addsub4		#shortcut if no adjust
	move.w	%d3,%d5		#shift amount
	lsl.w	&7,%d3		#align with exponent field
	swap	%d3
	sub.l	%d3,%d4		#new exponent
	bmi.w	underflw   	#underflow?

	lsl.l	%d5,%d2		#normalize upper bits
	rol.l	%d5,%d1		#now lower bits
	moveq	&-1,%d7		#mask
	lsl.l	%d5,%d7
	move.l	%d7,%d3		#mask for remainder of d1
	not.l	%d7		#mask for moved bits
	and.l	%d1,%d7		#bits to move
	and.l	%d3,%d1		#remove moved bits
	or.l	%d7,%d2		#insert moved bits
#
# ROUND THE RESULT
#
addsub4:	add.l	%d1,%d1		#pop off round bit
	bcc.b	addsub6
	addq.l	&1,%d2		#round up
	tst.l	%d1		#should i have?
	bne.b	addsub5
	bclr	%d6,%d2		#maybe not
addsub5:	cmp.l	%d2,&0x1000000		#overflow?
	blt.b	addsub6
	move.l	%d6,%d2		#yes
	add.l	%d6,%d4		#adjust exponent
#
# RECOMBINE FIELDS
#
addsub6:	add.l	%d4,%d2		#put in exponent
	bmi.w  	overflw   	#overflow?
	sub.l	%d6,%d2		#remove hidden bit
	cmp.l	%d2,%d6
	blt.w  	underflw   	#underflow?
	tst.l	%d0		#get the sign
	bpl.b	addsub7
	bset	&31,%d2
addsub7:	move.l	%d2,%d0
addsub8:	movem.l	(%sp)+,%d2-%d7	#restore registers
	unlk    %a6
	rts


#
# Code for errors in the +,-,*, and / routines.
#
overflw:  trap	&SIGFPE         #overflow
	movem.l	(%sp)+,%d2-%d7	#restore registers
	unlk    %a6
	rts

underflw: moveq	&0,%d0		#underflow
	movem.l	(%sp)+,%d2-%d7	#restore registers
	unlk    %a6
	rts

divzero:	trap	&SIGFPE		#division by zero
	movem.l	(%sp)+,%d2-%d7	#restore registers
	unlk    %a6
	rts

#******************************************************************************
#
#       Procedure _fcmpf
#      
#       Revision: Paul Beiser 01/11/83 to handle -0 as inputs 
#
#       Description:
#               Compare 2 32 bit real numbers.
#
#       Parameters:
#               4(sp)  - the 2 numbers to be compared
#
#       The result is returned in d0.
#
#
___fcmpf:
_fcmpf:  
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a0
	mov.l	p_fcmpf(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_fcmpf,%a0
	jsr	mcount
')
	')
	link	%a6,&0
	movem.l	(8,%a6),%d0/%d1
	cmp.l   %d0,&minuszero   #check for a -0
	bne.w     fcmpA1          #jump if not a -0
	   moveq   &0,%d0           #else convert to 0
fcmpA1:  cmp.l   %d1,&minuszero   #check for a -0
	bne.w     fcmpA2          #jump if not a -0
	   moveq   &0,%d1
fcmpA2:  tst.l	%d0		#test first for sign of the first operand
	bpl.w	rcomp2		
	tst.l	%d1		#test sign of second operand
	bpl.w	rcomp2
#
	cmp.l	%d1,%d0		#both negative so do test backward
	beq.w	eq
	bgt.w	gt
	bra.w	lt
#
rcomp2:	cmp.l	%d0,%d1		#at least one positive, ordinary test
	beq.w	eq
	bgt.w	gt
lt:	move.l	&-1,%d0
	bra.w	done
gt:	moveq	&1,%d0
	bra.w	done
eq:	moveq	&0,%d0
done:	unlk	%a6
	rts	

ifdef(`PROFILE',`
		data
p_ftod:		long	0
p_dtof:		long	0
p_itofl:	long	0
p_fltoi:	long	0
p_rndfl:	long	0
p_fmulf:	long	0
p_fdivf:	long	0
p_faddf:	long	0
p_fsubf:	long	0
p_afaddf:	long	0
p_afsubf:	long	0
p_afmulf:	long	0
p_afdivf:	long	0
p_fcmpf:	long	0
	')
