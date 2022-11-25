# KLEENIX_ID @(#)walkf.s	63.1 90/05/11 
#
# Hand written version of walkf in common
#	Written by Mark McDowell
#
# -------------------------------------------------------------------------
#/* walkf() is a postfix tree traversal routine that traverses the tree in the
#   order L - R - P.
#*/
#walkf( t, f ) register NODE *t;  int (*f)(); {
#	register short opty;
#
#	opty = optype(t->in.op);
#
#	if( opty != LTYPE ) walkf( t->in.left, f );
#	if( opty == BITYPE ) walkf( t->in.right, f );
#	(*f)( t );
#	}
#
# -------------------------------------------------------------------------

# after CPP it looks thus:
# -------------------------------------------------------------------------
#	walkf( t, f ) register NODE *t;  int (*f)(); {
#		register short opty;
#	
#		opty = (dope[t->in.op]&016);
#	
#		if( opty != 02 ) walkf( t->in.left, f );
#		if( opty == 010 ) walkf( t->in.right, f );
#		(*f)( t );
#		}
#	
# -------------------------------------------------------------------------

	global	_walkf
_walkf:
	mov.l	4(%sp),%a0	#move t to a0
	pea	(%a0)		#push t on the stack for final call to f
	mov.w	(%a0),%d0	#t->in.op to d0
	lsl.w	&2,%d0
	lea	_dope,%a1	#base address of array
	movq	&14,%d1		#TYFLG to d1
	and.w	2(%a1,%d0.w),%d1
	mov.l	12(%sp),%a1	#move f to a1
	subq.w	&8,%d1		#check if opty is BITYPE
	bne.b	L1		#do not walk the right side at L1
	pea	(%a1)		#push f for right traversal
	mov.l	30(%a0),-(%sp)	#push t->in.right for right traversal
	pea	(%a1)		#push f for left traversal
	mov.l	26(%a0),-(%sp)	#push t->in.left for left traversal
	bsr.b	_walkf		#walk left side of tree
	addq.w	&8,%sp		#remove parameters
	bsr.b	_walkf		#walk right side of tree
	addq.w	&4,%sp		#remove only t->in.right, leaving f on the stack
	mov.l	(%sp)+,%a0	#move f to a0
	jsr	(%a0)		#call f
	addq.w	&4,%sp		#remove t
	rts			#done
L1:
	addq.w	&6,%d1		#check if opty is LTYPE
	beq.b	L3		#do not walk either side of the tree at L3
	pea	(%a1)		#push f for left traversal
	mov.l	26(%a0),-(%sp)	#push t->in.left for left traversal
	bsr.b	_walkf		#walk left side of tree
	addq.w	&4,%sp		#remove only t->in.left, leaving f on the stack
	mov.l	(%sp)+,%a0	#move f to a0
	jsr	(%a0)		#call f
	addq.w	&4,%sp		#remove t
	rts			#done
L3:
	jsr	(%a1)		#since no procedure call as yet, a1 contains f
	addq.w	&4,%sp		#remove t
	rts			#done
