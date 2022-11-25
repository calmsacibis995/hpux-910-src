# @(#) $Revision: 63.1 $     
#setjmp, longjmp
#
# These routines are for System V.3
# compatibility. The don't save and
# restore the signal mask like the
# the ones in libc.a do.
#
#       longjmp(a, v)
#just calls _longjmp to do the
#real work (in setjmputil.s(libc.a))
#
#       setjmp(v)
#just calls _setjmp to do the real
#work (in setjmputil.s(libc.a))
#
#jmp_buf is set up as:
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
#       |  signal mask  | -- not used by this version
#	-----------------

	global   _setjmp,_longjmp
	text	
		
_setjmp:
	ifdef(`PROFILE',`
	mov.l	&p_setjmp,%a0
	jsr	mcount
	')
	jmp     __setjmp

_longjmp:
	ifdef(`PROFILE',`
	mov.l	&p_longjmp,%a0
	jsr	mcount
	')
	jmp     __longjmp

ifdef(`PROFILE',`
		data
p_setjmp:	long	0
p_longjmp:	long	0
	')
