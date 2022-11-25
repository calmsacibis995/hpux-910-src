# @(#) $Revision: 66.5 $
#_setjmp, _longjmp
#
#	_longjmp(a, v)
#causes a "return(v)" from the
#last call to
#
#	_setjmp(v)
#by restoring all the registers and
#adjusting the stack
#
#These routines assume that 'v'
#points to data space containing
#at least enough room to store the
#following:
#
#	----------------- <----	 0
#	|	pc	|
#	-----------------
#	|	d2	|
#	-----------------
#	|	...	|
#	-----------------
#	|	d7	|
#	-----------------
#	|	a2	|   +  13 x 4
#	-----------------
#	|	...	|
#	-----------------
#	|	a7	|
#	----------------- <---- 52
#	|   mask	|   +	 1   x 4	NOTE: for sigsetjmp savemask
#	----------------- <---- 56  68881/2 Floating Point regs
#	|      fp2	|
#	-----------------
#	|      ...	|   +	18 x 4
#	-----------------
#	|      fp7	|
#	----------------- <---- 128  fpa accelerator regs
#	|     fpa3	|
#	-----------------
#	|     ...	|   +	26 x 4
#	-----------------
#	|     fpa15	|
#	-----------------
#IMPORTANT NOTE:
# POSIX 1003.1 sigsetjmp/siglongjmp will store signal mask with offset
# of 100	    !!!! any changes will effect this function
#		    !!!! SO BEWARE!!!!!!
#	----------------- <----- 232 sigset_t structure [0]
#	|      [0]	|
#	-----------------
#	|     .....	|    + 8 x 4
#	-----------------
#	|      [7]	|
#	----------------- <----- 264
#
#
	global	 __setjmp,__longjmp,_____setjmp,_____longjmp
	text
	version 2
__setjmp:
	mov.l	(4,%sp),%a0		#pointer to context storage
	mov.l	(%sp),(%a0)		#pc
	movem.l %d2-%d7/%a2-%a7,(4,%a0) #d2-d7, a2-a7
# FP save/restore conventions as of 7.0:
# flag_68881 = 0 if MC68881 not present
#	     = 1 if present *and* save/restore required ("version 3")
#	     = 2 if present but save/restore *not* required
# flag_fpa =  0 if dragon not present
#	   = -1 if present *and* save/restore required ("version 3")
#	   = -2 if present but save/restore *not* required
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	flag_68881(%a0),%a1
	cmp.w	(%a1),&1		#check if 68881 FP save reqd',`
	cmp.w	flag_68881,&1		#check if 68881 FP save reqd
    ')
	bne.b	__store_fpa		#if not, check FPA
	mov.l	(4,%sp),%a1		#pointer to context storage
	fmovem	%fp2-%fp7,56(%a1)	#store FP2-FP7
__store_fpa:
    ifdef(`PIC',`
	mov.l	flag_fpa(%a0),%a1
	cmp.w	(%a1),&-1		#check if FPA save required',`
	cmp.w	flag_fpa,&-1		#check if FPA save required
    ')
	bne.b	__setjmp2s
#
# Note: the fp* macros use register a2, so we must save a2 before
#	we load it up with a new value.
#
	mov.l	%a2,%a1			#save a2, fp* uses it
	mov.l	&fpa_loc,%a2		#set base reg for fpa
	mov.l	(4,%sp),%a0		#pointer to context storage
	add	&128,%a0		#set to correct address
	fpmov.d %fpa3,(%a0)+		#store FPA3 -- FPA15
	fpmov.d %fpa4,(%a0)+
	fpmov.d %fpa5,(%a0)+
	fpmov.d %fpa6,(%a0)+
	fpmov.d %fpa7,(%a0)+
	fpmov.d %fpa8,(%a0)+
	fpmov.d %fpa9,(%a0)+
	fpmov.d %fpa10,(%a0)+
	fpmov.d %fpa11,(%a0)+
	fpmov.d %fpa12,(%a0)+
	fpmov.d %fpa13,(%a0)+
	fpmov.d %fpa14,(%a0)+
	fpmov.d %fpa15,(%a0)
	mov.l	%a1,%a2			#restore original value of a2
__setjmp2s:
	clr.l	%d0			#return 0
	rts

__longjmp:
    ifdef(`PIC',`
	mov.l	&DLT,%a1
	lea.l	-6(%pc,%a1.l),%a1
	mov.l	flag_68881(%a1),%a0
	cmp.w	(%a0),&1		#check if 68881 restore reqd',`
	cmp.w	flag_68881,&1		#check if 68881 restore reqd
    ')
	bne.b	__restore_fpa		#if not, check FPA
	mov.l	(4,%sp),%a0		#pointer to context storage
	add	&56,%a0			#set correct address in buf
	fmovem	(%a0),%fp2-%fp7		#load FP2-FP7
__restore_fpa:
    ifdef(`PIC',`
	mov.l	flag_fpa(%a1),%a0
	cmp.w	(%a0),&-1		#check if FPA restore reqd',`
	cmp.w	flag_fpa,&-1		#check if FPA restore reqd
    ')
	bne.b	__longjmp2S
#
# Note, although the fp* macros use register a2, we are going
# to restore the registers right before we return, so we can
# use a2 without saving it's previous value.
#
	mov.l	&fpa_loc,%a2		#set base reg for fpa
	mov.l	(4,%sp),%a0		#pointer to context storage
	add	&128,%a0		#set correct address
	fpmov.d (%a0)+,%fpa3		#load FPA3 --- FPA15
	fpmov.d (%a0)+,%fpa4
	fpmov.d (%a0)+,%fpa5
	fpmov.d (%a0)+,%fpa6
	fpmov.d (%a0)+,%fpa7
	fpmov.d (%a0)+,%fpa8
	fpmov.d (%a0)+,%fpa9
	fpmov.d (%a0)+,%fpa10
	fpmov.d (%a0)+,%fpa11
	fpmov.d (%a0)+,%fpa12
	fpmov.d (%a0)+,%fpa13
	fpmov.d (%a0)+,%fpa14
	fpmov.d (%a0),%fpa15
__longjmp2S:
	mov.l	(4,%sp),%a0		#pointer to context storage
	mov.l	(8,%sp),%d0		#value returned
	movem.l (4,%a0),%d2-%d7/%a2-%a7 #restore d2-d7, a2-a7
	mov.l	(%a0),(%sp)		#restore pc of call to setjmp to stack
	tst.l	%d0
	bne.w	__longjmp1S
	moveq	&1,%d0			#can never!! return 0
__longjmp1S:
	rts
