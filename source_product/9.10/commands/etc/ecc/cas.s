
# @(#) $Revision: 1.1 $

# This program is passed an address.  It will read the value
# stored at that address and then CAS the value back. This 
# allows a value to be rewritten to restore the ECC check bits.

global _cas

_cas:
	link.l %a6,&0
	mov.l 12(%a6),%d0
	mov.l  8(%a6),%a0	# Use a0 to hold the pointer
loop:	
	move.l (%a0),%d1	# get the old value
	cas.l %d1,%d1,(%a0)+	# set the value back, unless it changed.
	subi.l &1,%d0
	bne loop
	unlk %a6
	rts
