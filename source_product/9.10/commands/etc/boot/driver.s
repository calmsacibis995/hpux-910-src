 # HPUX_ID: @(#)driver.s	30.1     86/02/20

      	set	CRTMSG,0x150
       	set	KBDWCMD,0x164
       	set	READKEY,0x1c4
	set	MREAD,0x400c
	set	MINIT,0x4004
#ifdef SDS
	set     _current_io_base, 0xfffffdf0
#endif /* SDS */

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

#ifdef SDS
#       space for saving current_io_base during bootrom calls (RPC)
        comm    _saved_io_base,4
#endif /* SDS */
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
#ifdef SDS
	mov.l   _current_io_base,_saved_io_base
#endif /* SDS */
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
#ifdef SDS
	mov.l   _saved_io_base,_current_io_base
#endif /* SDS */
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

	global _jump_to_os

_jump_to_os:
	addq.l  &4,%sp                  # pop off return address
	mov.l   (%sp),%a0               # get entry point for os
	mov.l   &2,(%sp)                # replace with "additional parameter"
					# flag
	jmp     (%a0)                   # jump to os

#
# Allocate memory for the STI graphics display routines
#
	global	_connect_sti

##############################################################################
#									     #
#	Flowchart for STI reconnection from secondary loader		     #
#									     #
##############################################################################

	set	STI_MODULE,1		# Module number for STI functions
	set	STI_SIZE,1
	set	DISCONNECT,2		# STI operation codes
	set	DOWNLOAD,3

	set	CONFIG_WORD,0x3ffc	# Bootrom configuration word
	set	VECTOR0,1		# if bit 1 set, cmd processor present

	global	entry

_connect_sti:

	movm.l	%d0-%d7/%a0-%a5,-(%sp)
#
#	Check to see if bootrom has vector 0 command processor
#
	btst	&VECTOR0,CONFIG_WORD
	beq.b	return
#
#	Get size of STI modules; if zero, STI not present
#
	movq	&STI_MODULE,%d0
	movq	&STI_SIZE,%d1
	lea	test_size1,%a4
	jmp	([0])

test_size1:
	beq.b	return
	move.l	%d0,%d2			# Save size in %d2 for comparison
#
#	Allocate %d0.l bytes for modules
#

	lea	entry,%a0		# where our code starts
	sub.l	%d0,%a0			# empty space before our code

#
#	Reconnect STI console routines (%a0 must point to bottom of target)
#
	movq	&STI_MODULE,%d0
	movq	&DOWNLOAD,%d1
	lea	test_size2,%a4
	jmp	([0])

test_size2:
	cmp.l	%d0,%d2			# did it load as much as we asked?
	beq.b	return
#
#	ERROR:  Download size does not match; disconnect STI
#
	movq	&STI_MODULE,%d0
	movq	&DISCONNECT,%d1
	lea	return,%a4
	jmp	([0])

return:
	movm.l	(%sp)+,%d0-%d7/%a0-%a5
	rts
