s,@(#)asm.sed	4.1		7/25/83,@(#)asm.sed	4.1		7/25/83,
s/calls	$3,_bcopy/movc3	8(sp),*(sp),*4(sp)\
	addl2	$12,sp/
s/calls	$3,_bcmp/cmpc3	8(sp),*(sp),*4(sp)\
	addl2	$12,sp/
s/calls	$2,_bzero/movc5	$0,(r0),$0,4(sp),*(sp)\
	addl2	$8,sp/
s/calls	$1,_ffs/ffs	$0,$32,(sp)+,r0 \
	bneq	1f \
	mnegl	$1,r0 \
1: \
	incl	r0/
s/calls	$1,_htons/rotl	$8,(sp),r0\
	movb	1(sp),r0\
	movzwl	r0,r0\
	addl2	$4,sp/
s/calls	$1,_ntohs/rotl	$8,(sp),r0\
	movb	1(sp),r0\
	movzwl	r0,r0\
	addl2	$4,sp/
s/calls	$1,_htonl/rotl	$-8,(sp),r0\
	insv	r0,$16,$8,r0\
	movb	3(sp),r0\
	addl2	$4,sp/
s/calls	$1,_ntohl/rotl	$-8,(sp),r0\
	insv	r0,$16,$8,r0\
	movb	3(sp),r0\
	addl2	$4,sp/
s/calls	$2,__insque/insque	*(sp)+,*(sp)+/
s/calls	$1,__remque/remque	*(sp)+,r0/
s/calls	$2,__queue/movl	(sp)+,r0\
	movl	(sp)+,r1\
	insque	r1,*4(r0)/
s/calls	$1,__dequeue/movl	(sp)+,r0\
	remque	*(r0),r0/
