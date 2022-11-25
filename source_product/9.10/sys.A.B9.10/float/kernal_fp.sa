# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/kernal_fp.sa,v $
# $Revision: 1.5.84.3 $	$Author: kcs $
# $State: Exp $   	$Locker:  $
# $Date: 93/09/17 20:49:03 $

# This file contains rounties for debugging floating point under KDB.
# This is not user code, and is not used except through KDB.
#
	text

jt:	long	m0
	long	m1
	long	m2
	long	m3
	long	m4
	long	m5
	long	m6
	long	m7

	global	_fpall
_fpall:
	fmove.l	%fpsr,%d0
	move.l	%d0,-(%sp)	
	fmove.l	%fpcr,%d0
	move.l	%d0,-(%sp)
	pea		fpscr
	move.l	_kdb_printf,%a0
	jsr		(%a0)
	add.l	&12,%sp
	move.l	&0,-(%sp)
	jsr		_fp
	move.l	&1,(%sp)
	jsr		_fp
	move.l	&2,(%sp)
	jsr		_fp
	move.l	&3,(%sp)
	jsr		_fp
	move.l	&4,(%sp)
	jsr		_fp
	move.l	&5,(%sp)
	jsr		_fp
	move.l	&6,(%sp)
	jsr		_fp
	move.l	&7,(%sp)
	jsr		_fp
	addq	&4,%sp
	rts

fpscr:	byte	"fpcr = %08x, fpsr = %08x",10,0

	global	_fp0
_fp0:
	move.l	&0,-(%sp)
	jsr		_fp
	addq	&4,%sp
	rts
	global	_fp1
_fp1:
	move.l	&1,-(%sp)
	jsr		_fp
	addq	&4,%sp
	rts
	global	_fp2
_fp2:
	move.l	&2,-(%sp)
	jsr		_fp
	addq	&4,%sp
	rts
	global	_fp3
_fp3:
	move.l	&3,-(%sp)
	jsr		_fp
	addq	&4,%sp
	rts
	global	_fp4
_fp4:
	move.l	&4,-(%sp)
	jsr		_fp
	addq	&4,%sp
	rts
	global	_fp5
_fp5:
	move.l	&5,-(%sp)
	jsr		_fp
	addq	&4,%sp
	rts
	global	_fp6
_fp6:
	move.l	&6,-(%sp)
	jsr		_fp
	addq	&4,%sp
	rts
	global	_fp7
_fp7:
	move.l	&7,-(%sp)
	jsr		_fp
	addq	&4,%sp
	rts
	
	global	_fp
_fp:
	fmovem.x	%fp0/%fp1/%fp2,-(%sp)
	movem.l		%a0/%a1/%d0/%d1/%a2/%d2,-(%sp)
	mov.l	64(%sp),%d2
	lea.l	jt,%a2
	move.l	(%a2,%d2.w*4),%a2
	jmp		(%a2)
	
m0:	fmovem.x	%fp0,-(%sp)
	jmp		fp_out
m1:	fmovem.x	%fp1,-(%sp)
	jmp		fp_out
m2:	fmovem.x	%fp2,-(%sp)
	jmp		fp_out
m3:	fmovem.x	%fp3,-(%sp)
	jmp		fp_out
m4:	fmovem.x	%fp4,-(%sp)
	jmp		fp_out
m5:	fmovem.x	%fp5,-(%sp)
	jmp		fp_out
m6:	fmovem.x	%fp6,-(%sp)
	jmp		fp_out
m7:	fmovem.x	%fp7,-(%sp)
	jmp		fp_out

fp_out:
	mov.l	%d2,-(%sp)
	pea		string
	move.l	_kdb_printf,%a0
	jsr		(%a0)
	add.l	&20,%sp
	movem.l		(%sp)+,%a0/%a1/%d0/%d1/%a2/%d2
	fmovem.x	(%sp)+,%fp0/%fp1/%fp2
	rts

string:		byte	"fp%d = 0x%08x 0x%08x 0x%08x",10,0

	global	cache_none
cache_none: 
	move.l	%d0,-(%sp)
	move.l	&0,%d0
	movec	%d0,%cacr
	move.l	(%sp)+,%d0
	rts

	global	cache_i
cache_i:
	move.l	%d0,-(%sp)
	move.l	&0x8000,%d0
	movec	%d0,%cacr
	move.l	(%sp)+,%d0
	rts

	global	cache_d
cache_d:
	move.l	%d0,-(%sp)
	move.l	&0x80000000,%d0
	movec	%d0,%cacr
	move.l	(%sp)+,%d0
	rts

	global	cache_id
cache_id:
	global	cache_di
cache_di:
	move.l	%d0,-(%sp)
	move.l	&0x80008000,%d0
	movec	%d0,%cacr
	move.l	(%sp)+,%d0
	rts

	version		3


