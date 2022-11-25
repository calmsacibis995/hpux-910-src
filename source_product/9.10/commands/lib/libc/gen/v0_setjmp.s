# @(#) $Revision: 66.2 $
#___setjmp, ___longjmp
#
#	___longjmp(a, v)
#restores the signal mask and
#then calls ____longjmp to do the
#real work (in v0_setjmputil.s)
#
#	setjmp(v)
#saves the signal mask and then
#calls ____setjmp to do the real
#work (in v0_setjmputil.s)
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
#	|  signal mask	|
#	-----------------

	global	 ____setjmp,____longjmp
	text
	version 2

____setjmp:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_setjmp(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_setjmp,%a0
	jsr	mcount
    ')
')
	pea	0.w			#d0 = sigblock(0)
ifdef(`_NAMESPACE_CLEAN',`
    ifdef(`PIC',`
	bsr.l	___sigblock',`
	jsr	___sigblock
    ')',`
    ifdef(`PIC',`
	bsr.l	_sigblock',`
	jsr	_sigblock
    ')
')
	addq	&4,%sp
	move.l	(4,%sp),%a0		#pointer to jmp_buf
	move.l	%d0,(52,%a0)		#store signal mask
    ifdef(`PIC',`
	bra.l	_____setjmp',`
	jmp	_____setjmp
    ')

____longjmp:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_longjmp(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_longjmp,%a0
	jsr	mcount
    ')
')
	move.l	(4,%sp),%a0		#pointer to jmp_buf
	move.l	(52,%a0),-(%sp)		#get signal mask
ifdef(`_NAMESPACE_CLEAN',`
    ifdef(`PIC',`
	bsr.l	___sigsetmask		#restore signal mask',`
	jsr	___sigsetmask		#restore signal mask
    ')',`
    ifdef(`PIC',`
	bsr.l	_sigsetmask		#restore signal mask',`
	jsr	_sigsetmask		#restore signal mask
    ')
')
	addq	&4,%sp
    ifdef(`PIC',`
	bra.l	_____longjmp',`
	jmp	_____longjmp
    ')

ifdef(`PROFILE',`
		data
p_setjmp:	long	0
p_longjmp:	long	0
')
