# @(#) $Revision: 66.2 $
#setjmp, longjmp
#
#	longjmp(a, v)
#restores the signal mask and
#then calls _longjmp to do the
#real work (in setjmputil.s)
#
#	setjmp(v)
#saves the signal mask and then
#calls _setjmp to do the real
#work (in setjmputil.s)
#
#jmp_buf is shown in setjmputil.s
#

	global	  ____setjmp,____longjmp
ifdef(`_NAMESPACE_CLEAN',`
	global	  ___setjmp,___longjmp
	sglobal	  _setjmp,_longjmp',`
	global	 _setjmp,_longjmp
')
	text
	version 2

ifdef(`_NAMESPACE_CLEAN',`
___setjmp:
')
_setjmp:
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
	mov.l	(4,%sp),%a0		#pointer to jmp_buf
	mov.l	%d0,(52,%a0)		#store signal mask
    ifdef(`PIC',`
	bra.l	__setjmp',`
	jmp	__setjmp
    ')

ifdef(`_NAMESPACE_CLEAN',`
___longjmp:
')
_longjmp:
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
	mov.l	(4,%sp),%a0		#pointer to jmp_buf
	mov.l	(52,%a0),-(%sp)		#get signal mask
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
	bra.l	__longjmp		#restore signal mask',`
	jmp	__longjmp		#restore signal mask
    ')

ifdef(`PROFILE',`
		data
p_setjmp:	long	0
p_longjmp:	long	0
')
