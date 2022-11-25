# HPUX_ID: @(#) $Revision: 70.1 $

#	stack.s
#
#	Series 300 dynamic loader
#
#	data structures and routines to manipulate private stack


	global	_getsp
	text
_getsp:
	mov.l	%sp,%d0
	rts
