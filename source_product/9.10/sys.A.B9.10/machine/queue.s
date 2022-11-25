 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/queue.s,v $
 # $Revision: 1.4.84.3 $	$Author: kcs $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/09/17 21:07:45 $

 # $Header: queue.s,v 1.4.84.3 93/09/17 21:07:45 kcs Exp $

 #
 # VAX-11 Queue Instruction Emulation
 #
 #	"Another fine product of the Sirius Cybernetics Corporation"
 #

	set	next,0
	set	prev,4

 #
 # Insert Queue Element
 #

	global	_insque

_insque:
	move.l	%a2,-(%sp)
	move.l	8(%sp),%a0
	move.l	12(%sp),%a1

	move.l	next(%a1),%a2
	move.l	%a0,next(%a1)
	move.l	%a1,prev(%a0)
	move.l	%a2,next(%a0)
	move.l	%a0,prev(%a2)

	move.l	(%sp)+,%a2
	rts

 #
 # Remove Queue Element
 #

	global	_remque

_remque:
	move.l	%a2,-(%sp)

	move.l	8(%sp),%a0
	move.l	next(%a0),%a1
	move.l	prev(%a0),%a2
	move.l	%a1,next(%a2)
	move.l	%a2,prev(%a1)

	move.l	(%sp)+,%a2
	rts
