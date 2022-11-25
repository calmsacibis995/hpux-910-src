# @(#) $Revision: 66.2 $
#sigsetjmp, siglongjmp
#
#	siglongjmp(a, v)
#
#causes a "return(v)" from the
#last call to
#
#	sigsetjmp(v, savemask)
#by restoring all the registers and
#adjusting the stack
#for POSIX 1003.1 if savemask != 0 save and restore signal mask
#		  else do not restore signal mask
#
#sigjmp_buf is set up as:
#
#     0 _________________
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
#    52 -----------------
#	|		| savemask
#    56 -----------------
#	    floating
#	    point regs
#
#   232 -----------------
#	|	[0]	| sigset_t structure [0]
#	-----------------
#    +32|	...	|
#	-----------------
#	|	[7]	| sigset_t [7]
#    264-----------------
#
# NOTE: changes to setjmputil.s will effect this functions offsets
#

ifdef(`_NAMESPACE_CLEAN',`
	global	 __sigsetjmp,__siglongjmp
	sglobal	  _sigsetjmp,_siglongjmp',`
	global	 _sigsetjmp,_siglongjmp
 ')

	set	SIG_SETMASK,2
	text

ifdef(`_NAMESPACE_CLEAN',`
__sigsetjmp:
 ')
_sigsetjmp:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_sigsetjmp(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_sigsetjmp,%a0
	jsr	mcount
    ')
')
	mov.l	(4,%sp),%a0
	mov.l	(8,%sp),%d0
	mov.l	%d0,(52,%a0)
	tst.l	%d0
	beq.w	__savemask1S		#d0 = 0 do not save signal mask
	add	&232,%a0
	mov.l	%a0,-(%sp)
ifdef(`_NAMESPACE_CLEAN',`
    ifdef(`PIC',`
	bsr.l	__sigemptyset',`
	jsr	__sigemptyset
    ')',`
    ifdef(`PIC',`
	bsr.l	_sigemptyset',`
	jsr	_sigemptyset
    ')
 ')
	addq	&4,%sp
	mov.l	(4,%sp),%a0
	add	&232,%a0
	mov.l	%a0,-(%sp)		#set parm *oset = sigjmp_buf+52
	pea	0.w			#set parameter *set = 0
	pea	0.w			#set parameter how = 0
ifdef(`_NAMESPACE_CLEAN',`
    ifdef(`PIC',`
	bsr.l	__sigprocmask',`
	jsr	__sigprocmask
    ')',`
    ifdef(`PIC',`
	bsr.l	_sigprocmask',`
	jsr	_sigprocmask
    ')
 ')
	add	&12,%sp
__savemask1S:
    ifdef(`PIC',`
	bra.l	__setjmp',`
	jmp	__setjmp
    ')

ifdef(`_NAMESPACE_CLEAN',`
__siglongjmp:
 ')
_siglongjmp:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_siglongjmp(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_siglongjmp,%a0
	jsr	mcount
    ')
')
	mov.l	(4,%sp),%a0
	mov.l	(52,%a0),%d0		#check whether to save signal mask
	tst.l	%d0			#if savemask = 0 do not restore mask
	beq.w	__restoremask1S
	pea	0.w			#set parm oset = 0
	add	&232,%a0
	mov.l	%a0,-(%sp)
	mov.l	&SIG_SETMASK,-(%sp)	#set parm how = 2
ifdef(`_NAMESPACE_CLEAN',`
    ifdef(`PIC',`
	bsr.l	__sigprocmask',`
	jsr	__sigprocmask
    ')',`
    ifdef(`PIC',`
	bsr.l	_sigprocmask',`
	jsr	_sigprocmask
    ')
 ')
	add	&12,%sp
__restoremask1S:
    ifdef(`PIC',`
	bra.l	__longjmp',`
	jmp	__longjmp
    ')

ifdef(`PROFILE',`
		data
p_sigsetjmp:	long	0
p_siglongjmp:	long	0
	')
