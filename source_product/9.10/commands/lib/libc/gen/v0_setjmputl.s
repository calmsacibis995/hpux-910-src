# @(#) $Revision: 64.2 $     
#____setjmp, ____longjmp
#
#       ____longjmp(a, v)
#causes a "return(v)" from the
#last call to
#
#       ____setjmp(v)
#by restoring all the registers and
#adjusting the stack
#
#These routines assume that 'v'
#points to data space containing
#at least enough room to store the
#following:
#
#	_________________
#	|	pc	|
#	-----------------
#	|	d2	|
#	-----------------
#	|	...	|
#	-----------------
#	|	d7	|
#	-----------------
#	|	a2	|
#	-----------------
#	|	...	|
#	-----------------
#	|	a7	|
#	-----------------
#       |       ...     |
#
#
		
	global   _____setjmp,_____longjmp
	text	
	version	2
		
_____setjmp:
	move.l  (4,%sp),%a0             #pointer to context storage
	move.l	(%sp),(%a0)		#pc
	movem.l	&0xFCFC,(4,%a0)		#d2-d7, a2-a7
	clr.l	%d0		#return 0
	rts	
		
_____longjmp:
	move.l  (4,%sp),%a0             #pointer to context storage
	move.l	(8,%sp),%d0		#value returned
	movem.l	(4,%a0),&0xFCFC		#restore d2-d7, a2-a7
	move.l	(%a0),(%sp)		#restore pc of call to setjmp to stack
	tst.l   %d0
	bne.w	__longjmp1S
	moveq   &1,%d0                  #can never!! return 0
__longjmp1S:	rts	
