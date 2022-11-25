 # HPUX_ID: @(#)driver.s	30.1     86/02/20

      	set	CRTMSG,0x150
       	set	KBDWCMD,0x164
       	set	READKEY,0x1c4
	set	MREAD,0x400c
	set	MINIT,0x4004

	global	_crtmsg
_crtmsg:
	mov.l	4(%sp),%d0
	mov.l	8(%sp),%a0
	jsr	CRTMSG
	rts

	global	_kbdwcmd
_kbdwcmd:
	movq	&0,%d0
	mov.l	4(%sp),%d1
	jsr	KBDWCMD
	rts

	global	_readkey
_readkey:
	movq	&0,%d0
	jsr	READKEY
	rts

	comm	_boot_errno,4
	comm	_call_bootrom_regs,4*16

	global	_call_bootrom
_call_bootrom:
	movm.l	%d1-%d7/%a0-%a7,_call_bootrom_regs
	movm.l	4(%sp),%d0-%d5		# load parameters, %d0 = func
					# %d1..%d4 = p1..p4, respectively
	link	%a5,&-10
	pea	_call_bootrom_recover
	mov.l	%sp,-10(%a5)
	mov.w	&-1,-(%sp)		# PWS calling seq: boolean return value
	cmp	%d0,&0x4004
	beq	_call_bootrom_minit
	cmp	%d0,&0x4008
	beq	_call_bootrom_m_fopen
	cmp	%d0,&0x400C
	beq	_call_bootrom_mread
	cmp	%d0,&0x4010
	beq	_call_bootrom_m_fclose
	bra	_call_bootrom_recover2	# return an error - bad call value

_call_bootrom_minit:			# MINIT call
	mov.l	%d1,-(%sp)		# MSUS: integer
	bra	_call_bootrom_50

_call_bootrom_m_fopen:			# M_FOPEN call
	mov.l	%d1,-(%sp)		# VAR filename: STRING255
	mov.l	%d2,-(%sp)		# VAR execution_address: integer
	mov.l	%d3,-(%sp)		# VAR length: integer
	mov.l	%d4,-(%sp)		# VAR ftype: boolean
	bra	_call_bootrom_50

_call_bootrom_mread:			# M_READ call
	mov.l	%d1,-(%sp)		# sector: integer
	mov.l	%d2,-(%sp)		# byte_count: integer
	mov.l	%d3,-(%sp)		# ramaddress: integer
	mov.w	%d4,-(%sp)		# media: boolean
#	bra	_call_bootrom_50

_call_bootrom_m_fclose:			# M_FCLOSE has no parameters (!)

_call_bootrom_50:
	mov.l	&0xffffffff,_boot_errno
	mov.l	%d0,%a2
#ifdef DEBUG
#	jsr	_dump_d_regs
#endif DEBUG
	jsr	(%a2)			# d0..d4 contain input parameters
	tst.b	(%sp)+			# check bootrom return value
	beq	_call_bootrom_recover2
	movq	&0,%d0			# assume success
_call_bootrom_return:
	unlk	%a5
	movm.l	_call_bootrom_regs,%d1-%d7/%a0-%a7
	rts
_call_bootrom_recover:
	mov.w	-2(%a5),_boot_errno
_call_bootrom_recover2:
	movq	&-1,%d0
	bra.b	_call_bootrom_return
