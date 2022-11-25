# @(#) $Revision: 66.3 $      
#
	version 2
	global	___float,___fix
	sglobal	_float,_fix
	global	___fadd,___fsub,___fmul,___fdiv
	sglobal	_fadd,_fsub,_fmul,_fdiv
	global	___afadd,___afsub,___afmul,___afdiv
	sglobal	_afadd,_afsub,_afmul,_afdiv
	global	___fcmp
	sglobal	_fcmp

         	set	minuszero,0x80000000	#top part of IEEE -0

___float:
_float:  
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_float(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_float,%a0
	jsr	mcount
')
	')
	link	%a6,&0
#	trap	#2
#	dc.w	-20
	movem.l	%d4-%d7,-(%sp)
	move.l	(8,%a6),%d0
ifdef(`PIC',`
	bsr.l	lntrel',`
	jsr	lntrel
')
	movem.l	(%sp)+,%d4-%d7
	unlk	%a6
	rts

___fix:
_fix:    
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_fix(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_fix,%a0
	jsr	mcount
')
	')
	link	%a6,&0
#	trap	#2
#	dc.w	-20
	movem.l	%d4-%d7,-(%sp)
	move.l	(8,%a6),%d0
	move.l	(12,%a6),%d1
ifdef(`PIC',`
	bsr.l	Atrunc',`
	jsr	Atrunc
')
	movem.l	(%sp)+,%d4-%d7
	unlk	%a6
	rts
#
#
#

f_fadd:	fmov.l	&0x7400,%fpcr
	fmov.d	(4,%sp),%fp0
	fadd.d	(12,%sp),%fp0
	fmov.d	%fp0,-(%sp)
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	rts

___fadd:
_fadd:	
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_fadd(%a1),%a0
	bsr.l	mcount
	')
	mov.l	flag_68881(%a1),%a0
	tst.w	(%a0)		# 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_fadd,%a0
	jsr	mcount
	')
	tst.w	flag_68881	# 68881 present?
')
	bne.b	f_fadd
	link	%a6,&0
#	trap	#2
#	dc.w	-28
	movem.l	&0x3F00,-(%sp)
	movem.l	(8,%a6),&0xF
ifdef(`PIC',`
	bsr.l	radd',`
	jsr	radd
')
	movem.l	(%sp)+,&0xFC
	unlk	%a6
	rts	

f_fsub:	fmov.l	&0x7400,%fpcr
	fmov.d	(4,%sp),%fp0
	fsub.d	(12,%sp),%fp0
	fmov.d	%fp0,-(%sp)
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	rts

___fsub:
_fsub:	
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_fsub(%a1),%a0
	bsr.l	mcount
	')
	mov.l   flag_68881(%a1),%a0
	tst.w   (%a0)           # 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_fsub,%a0
	jsr	mcount
	')
	tst.w   flag_68881	# 68881 present?
')
	bne.b	f_fsub
	link	%a6,&0
#	trap	#2
#	dc.w	-28
	movem.l	&0x3F00,-(%sp)
	movem.l	(8,%a6),&0xF
ifdef(`PIC',`
	bsr.l	rsbt',`
	jsr	rsbt
')
	movem.l	(%sp)+,&0xFC
	unlk	%a6
	rts	

f_fmul:	fmov.l	&0x7400,%fpcr
	fmov.d	(4,%sp),%fp0
	fmul.d	(12,%sp),%fp0
	fmov.d	%fp0,-(%sp)
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	rts

___fmul:
_fmul:	
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_fmul(%a1),%a0
	bsr.l	mcount
	')
	mov.l   flag_68881(%a1),%a0
	tst.w   (%a0)           # 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_fmul,%a0
	jsr	mcount
	')
	tst.w   flag_68881	# 68881 present?
')
	bne.b	f_fmul
	link	%a6,&0
#	trap	#2
#	dc.w	-28
	movem.l	&0x3F00,-(%sp)
	movem.l	(8,%a6),&0xF
ifdef(`PIC',`
	bsr.l	rmul',`
	jsr	rmul
')
	movem.l	(%sp)+,&0xFC
	unlk	%a6
	rts	

f_fdiv:	fmov.l	&0x7400,%fpcr
	fmov.d	(4,%sp),%fp0
	fdiv.d	(12,%sp),%fp0
	fmov.d	%fp0,-(%sp)
	mov.l	(%sp)+,%d0
	mov.l	(%sp)+,%d1
	rts

___fdiv:
_fdiv:	
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_fdiv(%a1),%a0
	bsr.l	mcount
	')
	mov.l   flag_68881(%a1),%a0
	tst.w   (%a0)           # 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_fdiv,%a0
	jsr	mcount
	')
	tst.w   flag_68881	# 68881 present?
')
	bne.b	f_fdiv
	link	%a6,&0
#	trap	#2
#	dc.w	-40
	movem.l	&0x3F00,-(%sp)
	movem.l	(8,%a6),&0xF
ifdef(`PIC',`
	bsr.l	rdvd',`
	jsr	rdvd
')
	movem.l	(%sp)+,&0xFC
	unlk	%a6
	rts	
#
#
# Note: Although this function is designed to store its result indirectly 
# through a pointer, calling code may also require the result to be found 
# in registers d0 and d1. Therefore, if any modifications are made to this 
# code, be certain d0 and d1 are correct on exit.
#
#
f_afadd:fmov.l	&0x7400,%fpcr
	mov.l	(4,%sp),%a0
	fmov.d	(%a0),%fp0
	fadd.d	(8,%sp),%fp0
	fmov.d	%fp0,(%a0)
	movm.l	(%a0),%d0-%d1
	rts

___afadd:
_afadd:	
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_afadd(%a1),%a0
	bsr.l	mcount
	')
	mov.l   flag_68881(%a1),%a0
	tst.w   (%a0)           # 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_afadd,%a0
	jsr	mcount
	')
	tst.w   flag_68881	# 68881 present?
')
	bne.b	f_afadd
	link	%a6,&0
#	trap	#2
#	dc.w	-32
	movem.l	&0x3F08,-(%sp)
	move.l	(8,%a6),%a4
	movem.l	(%a4),&0x3
	movem.l	(12,%a6),&0xC
ifdef(`PIC',`
	bsr.l	radd',`
	jsr	radd
')
	movem.l	&0x3,(%a4)
	movem.l	(%sp)+,&0x10FC
	unlk	%a6
	rts	

# Note: Although this function is designed to store its result indirectly 
# through a pointer, calling code may also require the result to be found 
# in registers d0 and d1. Therefore, if any modifications are made to this 
# code, be certain d0 and d1 are correct on exit.
#
f_afsub:fmov.l	&0x7400,%fpcr
	mov.l	(4,%sp),%a0
	fmov.d	(%a0),%fp0
	fsub.d	(8,%sp),%fp0
	fmov.d	%fp0,(%a0)
	movm.l	(%a0),%d0-%d1
	rts

___afsub:
_afsub:	
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_afsub(%a1),%a0
	bsr.l	mcount
	')
	mov.l   flag_68881(%a1),%a0
	tst.w   (%a0)           # 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_afsub,%a0
	jsr	mcount
	')
	tst.w   flag_68881	# 68881 present?
')
	bne.b	f_afsub
	link	%a6,&0
#	trap	#2
#	dc.w	-32
	movem.l	&0x3F08,-(%sp)
	move.l	(8,%a6),%a4
	movem.l	(%a4),&0x3
	movem.l	(12,%a6),&0xC
ifdef(`PIC',`
	bsr.l	rsbt',`
	jsr	rsbt
')
	movem.l	&0x3,(%a4)
	movem.l	(%sp)+,&0x10FC
	unlk	%a6
	rts	

# Note: Although this function is designed to store its result indirectly 
# through a pointer, calling code may also require the result to be found 
# in registers d0 and d1. Therefore, if any modifications are made to this 
# code, be certain d0 and d1 are correct on exit.
#
f_afmul:fmov.l	&0x7400,%fpcr
	mov.l	(4,%sp),%a0
	fmov.d	(%a0),%fp0
	fmul.d	(8,%sp),%fp0
	fmov.d	%fp0,(%a0)
	movm.l	(%a0),%d0-%d1
	rts

___afmul:
_afmul:	
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_afmul(%a1),%a0
	bsr.l	mcount
	')
	mov.l   flag_68881(%a1),%a0
	tst.w   (%a0)           # 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_afmul,%a0
	jsr	mcount
	')
	tst.w   flag_68881	# 68881 present?
')
	bne.b	f_afmul
	link	%a6,&0
#	trap	#2
#	dc.w	-32
	movem.l	&0x3F08,-(%sp)
	move.l	(8,%a6),%a4
	movem.l	(%a4),&0x3
	movem.l	(12,%a6),&0xC
ifdef(`PIC',`
	bsr.l	rmul',`
	jsr	rmul
')
	movem.l	&0x3,(%a4)
	movem.l	(%sp)+,&0x10FC
	unlk	%a6
	rts	

# Note: Although this function is designed to store its result indirectly 
# through a pointer, calling code may also require the result to be found 
# in registers d0 and d1. Therefore, if any modifications are made to this 
# code, be certain d0 and d1 are correct on exit.
#
f_afdiv:fmov.l	&0x7400,%fpcr
	mov.l	(4,%sp),%a0
	fmov.d	(%a0),%fp0
	fdiv.d	(8,%sp),%fp0
	fmov.d	%fp0,(%a0)
	movm.l	(%a0),%d0-%d1
	rts

___afdiv:
_afdiv:	
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a1
	ifdef(`PROFILE',`
	mov.l	p_afdiv(%a1),%a0
	bsr.l	mcount
	')
	mov.l   flag_68881(%a1),%a0
	tst.w   (%a0)           # 68881 present?',`
	ifdef(`PROFILE',`
	mov.l	&p_afdiv,%a0
	jsr	mcount
	')
	tst.w   flag_68881	# 68881 present?
')
	bne.b	f_afdiv
	link	%a6,&0
#	trap	#2
#	dc.w	-44
	movem.l	&0x3F08,-(%sp)
	move.l	(8,%a6),%a4
	movem.l	(%a4),&0x3
	movem.l	(12,%a6),&0xC
ifdef(`PIC',`
	bsr.l	rdvd',`
	jsr	rdvd
')
	movem.l	&0x3,(%a4)
	movem.l	(%sp)+,&0x10FC
	unlk	%a6
	rts	

#******************************************************************************
#
#       Procedure  : _fcmp
#
#       Description: Compare operand 1 with operand 2. Both operands are
#                    64 bit floating point reals.
#
#       Author     : Paul Beiser
#
#       Revisions  : 1.0  06/01/81
#                    2.0  09/01/83  For -0 as valid input
#
#       Parameters : Both reals are on the stack.
#
#       Result     : Returned in d0 : -1 -> lt,  0 -> eq,  1 -> gt
#
#       Misc	   : Invalid IEEE numbers are not checked for.
#
#******************************************************************************

___fcmp:
_fcmp:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_fcmp(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_fcmp,%a0
	jsr	mcount
')
	')
	link	%a6,&0
#	trap	#2
#	dc.w	-8
	movem.l	%d2/%d3,-(%sp)
	movem.l	(8,%a6),%d0-%d3
	cmp.l   %d0,&minuszero   #check first operand for -0
	bne.w     mz1
	   moveq   &0,%d0           #convert to a +0
mz1:     cmp.l   %d2,&minuszero   #check second operand for -0
	bne.w     mz2
	   moveq   &0,%d2           #convert to a +0
mz2:     tst.l	%d0		#test first for sign of the first operand
	bpl.w	rcomp2		
	tst.l	%d2		#test sign of second operand
	bpl.w	rcomp2
#
	cmp.l	%d2,%d0		#both negative so do test backward
	beq.w	mz29S
	bgt.w	gt
	bra.w	lt
mz29S:	cmp.l	%d3,%d1
	beq.w	eq
	bhi.w	gt
	bra.w	lt
rcomp2:	cmp.l	%d0,%d2		#at least one positive, ordinary test
	beq.w	rcomp29S
	bgt.w	gt
	bra.w	lt
rcomp29S:	cmp.l	%d1,%d3
	beq.w	eq
	bhi.w	gt
lt:	move.l	&-1,%d0
	bra.w	done
gt:	moveq	&1,%d0
	bra.w	done
eq:	moveq	&0,%d0
done:	movem.l	(%sp)+,%d2/%d3
	unlk	%a6
	rts	

ifdef(`PROFILE',`
		data
p_float:	long	0
p_fix:		long	0
p_fadd:		long	0
p_fsub:		long	0
p_fmul:		long	0
p_fdiv:		long	0
p_afadd:	long	0
p_afsub:	long	0
p_afmul:	long	0
p_afdiv:	long	0
p_fcmp:		long	0
	')
