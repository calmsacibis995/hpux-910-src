# @(#) $Revision: 70.1 $      
#
	version 2
ifdef(`_NAMESPACE_CLEAN',`
	global	__ldexp,__frexp,__modf
	sglobal	_ldexp,_frexp,_modf',`
	global	_ldexp,_frexp,_modf
')
		
       		set	negHUGE,0xffefffff
       		set	posHUGE,0x7fefffff
       		set	lowHUGE,0xffffffff
      		set	ERANGE,34
		set	EDOM,33
         	set     minuszero,0x80000000

#******************************************************************************
#
#  Procedure _frexp
#
#  Author: Paul Beiser   3/2/82
#
#  Description:
#       Return a real X into a mantissa M such that 0 <= M < 1, and an exponent
#       E such that X = M * 2^E.
#
#  Parameters:
#       (d0,d1)  - the real to be separated.
#       (a0)     - pointer to location of E.
#
#  The result mantissa in returned in (d0,d1).  The result exponent is 
#  stored indirectly through (a0).
#
#  Error conditions: None
#
#  Special considerations: If X is zero, then both M and E will be 0.
#  
#  This routine modifed by Paul Beiser (1/10/84) to check for a -0
#
ifdef(`_NAMESPACE_CLEAN',`
__frexp:')
_frexp:	
ifdef(`PIC',`
        mov.l   &DLT,%a1
        lea.l   -6(%pc,%a1.l),%a1
')
ifdef(`PROFILE',`
	ifdef(`PIC',`
	mov.l	p_frexp(%a1),%a0
	bsr.l	mcount',`
	mov.l	&p_frexp,%a0
	jsr	mcount
	')
')
	link	%a6,&0
#	trap	#2
#	dc.w	-4
	move.l	%d7,-(%sp)
	movem.l	(8,%a6),&0x3	#Put argument (flting pt number) in d0 & d1
	move.l	(16,%a6),%a0
	moveq	&0,%d7		#for the long move later
	cmp.l   %d0,&minuszero	#check for a -0
	beq.w     retzero
	swap	%d0		#get exp in bottom word
	beq.w	retzero		#branch if X is 0
	move.w	%d0,%d7		#d7 will hold exp
	and.w	&0x7ff0,%d7	#mask out sign and mantissa bits, leave exp.
	cmp.w	%d7,&0x7ff0
	beq.w	naninf		#branch if X is NaN of Inf
	lsr.w	&4,%d7
	sub.w	&1022,%d7	#remove bias - 1
	ext.l	%d7
	move.l	%d7,(%a0)	#store the exponent
#
#  The exponent has been stored. Now do the mantissa.
#
	and.w	&0x800f,%d0	#remove exponent
	add.w	&0x3fe0,%d0	#exponent of -1
	swap	%d0		#correct order
	move.l	(%sp)+,%d7
	unlk	%a6
	rts
#
#  X had a value of zero.
#
retzero:
	move.l	%d7,(%a0)		#place zero exponent
	move.l	(%sp)+,%d7
	unlk	%a6
	rts	
#
# X is Nan or Inf
#
naninf:
	move.l	&0x7fffffff,%d0		#Return NaN
	move.l	&0xffffffff,%d1
	movq    &EDOM,%d7		#NaN or Inf; errno = EDOM
ifdef(`PIC',`
        mov.l   _errno(%a1),%a1
        mov.l   %d7,(%a1)',`
        mov.l   %d7,_errno
')
	move.l	(%sp)+,%d7
	unlk	%a6
	rts

#******************************************************************************
#
#  Procedure _ldexp
#
#  Author: Paul Beiser   3/2/82
#
#  Description:
#       Produce a real X from a real M and an integer E such that 0 <= M < 1. 
#	The real is formed by X = M * 2^E.
#
#  Parameters:
#       (d0,d1)  - M
#       (d7)     - exponent increment
#
#  The result is returned in (d0,d1).
#
#  Error conditions:
#       Overflow returns a large number; underflow returns 0. _errno
#       is set on these 2 error conditions.
#
#  This routine modifed by Paul Beiser (1/10/84) to check for a -0
#  Completely rewritten by Mark McDowell (code was becoming patches on patches)
#
ifdef(`_NAMESPACE_CLEAN',`
__ldexp:')
_ldexp: 
ifdef(`PIC',`
	mov.l	&DLT,%a1
	lea.l	-6(%pc,%a1.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_ldexp(%a1),%a0
	bsr.l	mcount
	')',`
	ifdef(`PROFILE',`
	mov.l	&p_ldexp,%a0
	jsr	mcount
	')
')
	mov.l	4(%sp),%a0		# get number
	mov.l	%a0,%d0
	swap	%d0
	and.w	&0x7FF0,%d0
	cmp.w	%d0,&0x7FF0		# check for NaN or Inf
	beq.w	ldnaninf
	mov.l	%a0,%d0
	mov.l	8(%sp),%d1		# get number
	add.l	%d0,%d0			# remove sign
	or.l	%d0,%d1			# check for +0 or -0
	bne.b	ldexp1			# is it?
	mov.l	%a0,%d0		# yes: restore number
	rts
ldexp1: clr.w	%d0			# isolate exponent
	swap	%d0
	lsr.w	&5,%d0
	add.l	12(%sp),%d0		# increment exponent
	bpl.b	ldexp3
	tst.b	12(%sp)			# overflow or underflow?
	bpl.b	ldexp4
ldexp2: movq	&ERANGE,%d0		# underflow: errno = ERANGE
ifdef(`PIC',`
	mov.l	_errno(%a1),%a1
	mov.l	%d0,(%a1)',`
	mov.l	%d0,_errno
')
	movq	&0,%d0			# return 0.0
	movq	&0,%d1
	rts
ldexp3: beq.b	ldexp2			# underflow here too
	cmp.l	%d0,&0x7FE		# exponent too big?
	bgt.b	ldexp4
	lsl.w	&4,%d0			# move exponent into place
	swap	%d0
	mov.l	%a0,%d1
	and.l	&0x800FFFFF,%d1		# exponent mask
	or.l	%d1,%d0			# combine in new exponent
	mov.l	8(%sp),%d1		# get lower half of number
	rts
ldexp4:	movq	&ERANGE,%d0		# overflow: errno = ERANGE
ifdef(`PIC',`
	mov.l	_errno(%a1),%a1
	mov.l	%d0,(%a1)',`
	mov.l	%d0,_errno
')
	mov.l	&0x7FEFFFFF,%d0		# return +HUGE or -HUGE
	mov.l	%a0,%d1			# was original number positive?
	bpl.b	ldexp5
	bchg	%d0,%d0			# -HUGE
ldexp5:	movq	&0xFFFFFFFF,%d1
	rts
ldnaninf:
	mov.l   %a0,%d0
	and.l	&0x000FFFFF,%d0
	bne.l	ldnan
	mov.l	%a0,%d0
	mov.l   8(%sp),%d1
	bne.l	ldnan
	rts
ldnan:
	mov.l	&EDOM,%d0
ifdef(`PIC',`
        mov.l   _errno(%a1),%a1
        mov.l   %d0,(%a1)',`
        mov.l   %d0,_errno
')
	mov.l   %a0,%d0
	mov.l   8(%sp),%d1
	rts

#******************************************************************************
#
#  Procedure _modf
#
#  Author: Paul Beiser   3/2/82
#
#  Description:
#       Return the positive fractional part of real X and the whole part of X;
#       e.g. If X=-3.7, return -3 and .7.
#
#  Parameters:
#       (d0,d1) - X
#
#  External references:
#       lntrel, rsbt, rellnt, rndzero
#
#  The fractional part is returned in (d0,d1). The whole part is returned 
#  through a0.
#
#  Error conditions: 
#       Procedure rellnt can error with a real too large in magnitude.
#
ifdef(`_NAMESPACE_CLEAN',`
__modf:
')
_modf:
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_modf(%a1),%a0
	bsr.l	mcount
	')
	mov.l   flag_68881(%a1),%a0
	tst.w   (%a0)           # 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_modf,%a0
	jsr	mcount
	')
	tst.w   flag_68881	# 68881 present?
')
	bne.b	modf881
	link	%a6,&0
#	trap	#2
#	dc.w	-28
	movem.l	&0x3F00,-(%sp)
	move.l	(16,%a6),%a0
	movem.l	(8,%a6),&0x3
	move.l	%d0,%d2		#save real in (d2,d3)
	move.l	%d1,%d3
ifdef(`PIC',`
	bsr.l	rndzero	#make	whole',`
	jsr	rndzero	#make	whole
')
	movem.l	&0x3,(%a0)	#return the whole part
ifdef(`PIC',`
	bsr.l	rsbt	#get	the	fractional	part',`
	jsr	rsbt	#get	the	fractional	part
')
	tst.l	%d0		#if result is zero, don't change bit
	beq.w	modf11S
	   bchg	  &31,%d0	   #make positive
modf11S: movem.l (%sp)+,&0xFC
	unlk	%a6
	rts			#positive frac in (d0,d1)

modf881:
	fmov.l	%fpcr,%d0
	fmov.l	&0x90,%fpcr
	fmov.d	4(%sp),%fp0
	fint	%fp0,%fp1
	fsub	%fp1,%fp0
	mov.l	12(%sp),%a0
	fmov.d	%fp1,(%a0)
	fmov.d	%fp0,-(%sp)
	fmov.l	%d0,%fpcr
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	rts

ifdef(`PROFILE',`
		data
p_ldexp:	long	0
p_frexp:	long	0
p_modf:		long	0
	')
