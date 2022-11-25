	text

	global ___mult32
___mult32:
	link.l	%a6,&-12
	clr.b	-12(%a6)
	movem.l	%d2-%d4,(%sp)
	movem.l	(8,%a6),%a0-%a1
	move.w	(%a1)+,%d0
	add.w	%d0,(%a0)+
	andi.w	&0x9FFF,(%a0)
	tst.w	(%a1)+
	bpl.w	mult32A
	bchg	&7,(%a0)
mult32A:
	movem.w	(2,%a0),%d0-%d1
	movem.w	(%a1),%d2-%d3
	move.w	%d3,%d4
	mulu	%d1,%d3
	mulu	%d2,%d1
	mulu	%d0,%d4
	mulu	%d2,%d0
	add.l	%d4,%d1
	move.w	%d1,%d4
	clr.w	%d1
	addx.b	%d1,%d1
	swap	%d1
	swap	%d4
	clr.w	%d4
	add.l	%d4,%d3
	addx.l	%d1,%d0
	bmi.w	mult32B
	subq.w	&1,(-2,%a0)
	add.l	%d3,%d3
	addx.l	%d0,%d0
mult32B:
	add.l	%d3,%d3
	sne	%d2
	roxr.b	&1,%d2
	lsr.b	&1,%d2
	or.b	%d2,(%a0)
	move.l	%d0,(2,%a0)
	movem.l (%sp),%d2-%d4
	unlk	%a6
	rts

	global ___mult64
___mult64:
	link.l	%a6,&-40
	clr.b	-40(%a6)
	movem.l	(8,%a6),%a0-%a1		# pointers to arguments
	move.w	(%a1)+,%d0		# exponent of second
	add.w	%d0,(%a0)+		# update exponent
	andi.w	&0x9FFF,(%a0)		# clear round and sticky bits
	tst.w	(%a1)+			# is second arg negative?
	bpl.w	mult64A			# if so, change sign of first
	bchg	&7,(%a0)
mult64A:
	movem.l	(%a1),%d0-%d1		# get second argument
	move.l	%a6,%a1
	movem.l	%d2-%d7,-(%a1)		# save registers
	move.l	%d0,%d6
	move.l	%d1,%d7
	movem.l	(2,%a0),%d4-%d5		# get first argument
	clr.w	-(%a1)			# lower 16 bits of partial product
	swap	%d6			# (b2)(b3)
	swap	%d7			# (b0)(b1)
	move.w	%d4,%d0			# move a2
	mulu	%d6,%d0			# a2*b3
	move.w	%d4,%d1			# move a2
	mulu	%d7,%d1			# a2*b1
	move.w	%d5,%d2			# move a0
	mulu	%d6,%d2			# a0*b3
	move.w	%d5,%d3			# move a0
	mulu	%d7,%d3			# a0*b1
	add.l	%d2,%d1			# a2*b1+a0*b3
	negx.l	%d0			# propagate carry
	neg.l	%d0
	swap	%d4			# (a2)(a3)
	swap	%d5			# (a0)(a1)
	swap	%d6			# (b3)(b2)
	swap	%d7			# (b1)(b0)
	move.w	%d5,%d2			# move a1
	mulu	%d7,%d2			# a1*b0
	add.l	%d3,%d2			# a1*b0+a0+b1
	moveq	&0,%d3			# for carry
	addx.l	%d3,%d1			# propagate carry
	addx.l	%d3,%d0			# propagate carry
	move.l	%d2,-(%a1)		# save partial result
	move.w	%d5,%d2			# move a1
	mulu	%d6,%d2			# a1*b2
	add.l	%d2,%d1			# a2*b1+a0*b3+a1*b2
	addx.l	%d3,%d0			# propagate carry
	move.w	%d4,%d2			# move a3
	mulu	%d7,%d2			# a3*b0
	add.l	%d2,%d1			# a2*b1+a0*b3+a1*b2+a3*b0
	addx.l	%d3,%d0			# propagate carry
	move.w	%d4,%d2			# move a3
	mulu	%d6,%d2			# a3*b2
	add.l	%d2,%d0			# a2*b3+a3*b2
	addx.b	%d3,%d3			# catch carry
	move.l	%d1,-(%a1)		# save partial product
	move.l	%d0,-(%a1)
	move.w	%d3,-(%a1)
	swap	%d6			# (b2)(b3)
	swap	%d7			# (b0)(b1)
	move.w	%d4,%d0			# move a3
	mulu	%d6,%d0			# a3*b3
	move.w	%d4,%d1			# move a3
	mulu	%d7,%d1			# a3*b1
	move.w	%d5,%d3			# move a1
	mulu	%d6,%d3			# a1*b3
	move.w	%d5,%d2			# move a1
	mulu	%d7,%d2			# a1*b1
	add.l	%d3,%d1			# a3*b1+a1*b3
	negx.l	%d0			# propagate carry
	neg.l	%d0
	swap	%d4			# (a3)(a2)
	swap	%d5			# (a1)(a0)
	swap	%d6			# (b3)(b2)
	swap	%d7			# (b1)(b0)
	move.w	%d5,%d3			# move a0
	mulu	%d7,%d3			# a0*b0
	mulu	%d6,%d5			# a0*b2
	mulu	%d4,%d7			# a2*b0
	mulu	%d6,%d4			# a2*b2
	moveq	&0,%d6			# for carry propagation
	add.l	%d5,%d7			# a2*b0+a0*b2
	addx.l	%d4,%d1			# a3*b1+a1*b3+a2*b2
	addx.l	%d6,%d0			# add carry
	add.l	%d7,%d2			# a1*b1+a2*b0+a0*b2
	addx.l	%d6,%d1			# add carry
	addx.l	%d6,%d0			# add carry
	movem.l	(%a1)+,%d4-%d7		# get other partial product
	add.l	%d3,%d7			# add partial products
	addx.l	%d2,%d6
	addx.l	%d1,%d5
	addx.l	%d0,%d4
	bmi.w	mult64B			# normalized?
	subq.w	&1,(-2,%a0)		# no: decrement exponent
	add.l	%d6,%d6			# shift result
	addx.l	%d5,%d5
	addx.l	%d4,%d4
mult64B:
	add.l	%d6,%d6			# round bit goes to extend bit
	or.l	%d7,%d6			# all of d6 is now sticky bit
	sne	%d7			# d7.b becomes sticky bit
	roxr.b	&1,%d7			# bit 7 is round; bit 6 is sticky
	lsr.b	&1,%d7			# bit 6 is round; bit 5 is sticky
	or.b	%d7,(%a0)		# put into result area
	movem.l	%d4-%d5,(2,%a0)		# save result
	movem.l	(%a1)+,%d2-%d7		# restore registers
	unlk	%a6
	rts

	global ___mult80
___mult80:
	link.l	%a6,&-60
	clr.b	-60(%a6)
	movem.l	%d2-%d7/%a2-%a5,(-40,%a6)
	movem.l	(8,%a6),%a0-%a1
	move.w	(%a1)+,%d0
	add.w	%d0,(%a0)+
	andi.w	&0x9FFF,(%a0)
	movem.l	(%a0),%d0-%d2
	movem.l	(%a1),%d3-%d5
	tst.l	%d3
	bpl.w	mult80A
	bchg	&7,(%a0)
mult80A:
	move.l	%d0,%a0
	move.l	%d1,%a1
	move.l	%d2,%a2
	move.l	%d3,%a3
	move.l	%d4,%a4
	move.l	%d5,%a5
	move.w	%d5,%d6
	mulu	%d2,%d6
	move.l	%d6,(-44,%a6)
	move.w	%d5,%d6
	mulu	%d1,%d6
	move.w	%d4,%d7
	mulu	%d2,%d7
	add.l	%d6,%d7
	mulu	%d0,%d5
	move.w	%d4,%d6
	mulu	%d1,%d6
	addx.l	%d5,%d6
	mulu	%d3,%d1
	mulu	%d0,%d4
	addx.l	%d1,%d4
	mulu	%d3,%d0
	moveq	&0,%d5
	addx.l	%d5,%d0
	mulu	%d2,%d3
	move.l	%a5,%d1
	swap	%d1
	swap	%d2
	mulu	%d2,%d1
	add.l	%d1,%d7
	addx.l	%d3,%d6
	addx.l	%d5,%d4
	addx.l	%d5,%d0
	move.l	%a1,%d1
	move.l	%a4,%d3
	swap	%d3
	swap	%d1
	mulu	%d3,%d2
	mulu	%d3,%d1
	add.l	%d2,%d6
	addx.l	%d1,%d4
	addx.l	%d5,%d0
	move.l	%a1,%d1
	move.l	%a5,%d2
	swap	%d1
	swap	%d2
	mulu	%d2,%d1
	add.l	%d1,%d6
	addx.l	%d5,%d4
	addx.l	%d5,%d0
	movem.l	%d0/%d4/%d6-%d7,(%sp)
	move.w	%d3,%d6
	move.w	%d2,%d7
	move.l	%a2,%d0
	mulu	%d0,%d7
	mulu	%d0,%d6
	swap	%d0
	move.l	%a4,%d1
	mulu	%d0,%d1
	move.l	%a5,%d4
	mulu	%d0,%d4
	add.l	%d4,%d7
	addx.l	%d1,%d6
	move.w	%a1,%d4
	mulu	%d3,%d4
	move.w	%a0,%d0
	mulu	%d2,%d0
	addx.l	%d0,%d4
	move.w	%a0,%d0
	mulu	%d0,%d3
	move.l	%a1,%d1
	swap	%d1
	move.w	%a3,%d0
	mulu	%d0,%d1
	addx.l	%d1,%d3
	addx.b	%d5,%d5
	move.w	%a1,%d1
	mulu	%d2,%d1
	swap	%d2
	move.l	%a1,%d0
	swap	%d0
	mulu	%d0,%d2
	add.l	%d1,%d2
	move.w	%a4,%d1
	mulu	%d0,%d1
	exg	%d7,%a3
	move.l	%a2,%d0
	swap	%d0
	mulu	%d7,%d0
	addx.l	%d0,%d1
	moveq	&0,%d7
	addx.l	%d7,%d3
	addx.b	%d7,%d5
	add.l	%d2,%d6
	addx.l	%d1,%d4
	addx.l	%d7,%d3
	addx.b	%d7,%d5
	move.w	(%sp)+,%a0
	movem.l	(%sp)+,%d0-%d2/%d7
	add.l	%a3,%d7
	addx.l	%d6,%d2
	addx.l	%d4,%d1
	addx.l	%d3,%d0
	move.w	%a0,%d3
	move.l	(8,%a6),%a0
	addx.w	%d3,%d5
	bmi.w	mult80B
	subq.w	&1,(%a0)
	add.l	%d2,%d2
	addx.l	%d1,%d1
	addx.l	%d0,%d0
	addx.w	%d5,%d5
mult80B:
	add.l	%d2,%d2
	or.w	(%sp)+,%d7
	or.l	%d7,%d2
	sne	%d2
	roxr.b	&1,%d2
	lsr.b	&1,%d2
	or.b	%d2,(2,%a0)
	addq.w	&4,%a0
	move.w	%d5,(%a0)+
	move.l	%d0,(%a0)+
	move.l	%d1,(%a0)
	movem.l	(%sp)+,%d2-%d7/%a2-%a5
	unlk	%a6
	rts
