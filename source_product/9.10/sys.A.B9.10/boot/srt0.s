 # HPUX_ID: @(#)srt0.s	30.1     86/02/20

 #	Boot Rom entry addresses

     	set	MINIT,0x4004		#initialize boot rom mass storage driver

 #	Miscellaneous constants

       	set	HIGHRAM,0xfffffac0	#last usable byte of high ram
        set     F_AREA,0xfffffed4	#pointer to the base of rom driver temps

        set	REMOTE_ADR,0x9a
        set	R_INTLVL,0x3
        set	BOOT_ID,0x3ffe
        set	NO_CARD,0x800000
     	set	BOOT3,0x3


 #
 # Startup code for standalone system
 # Non-relocating version -- for programs which are loaded by boot
 #

	global	_edata
	global	_end
	global	_main
	global	__rtt
	global	entry
	global	_func_table
	text
entry:
	bra.b	entry0
# this jump table is used by the C-level kernel debugger.
# it must be at 0xffff0802, otherwise CKDB jumps into hyperspace.
_func_table:		
	long	_open
	long	_close
	long	_read
	long	_open_CDF
	long	_lseek
entry0:	
	cmp.w	BOOT_ID,&BOOT3		#must be 3.0 boot rom
	bne.b	skip
	mova.l	F_AREA,%a0		#check if remote
	mov.l	REMOTE_ADR(%a0),%a0
	cmpa.l	%a0,&NO_CARD
	beq.b	skip
	clr.b	R_INTLVL		#disable card
skip:	mov.w	&0x2100,%sr		#allow interrupts (for mini floppy)
	mov.l	&HIGHRAM,%sp		#init stack pointer
start:
	mov.l	&_edata,%a0
	mov.l	&_end,%d0
	sub.l	&_edata,%d0

clr:	clr.l	(%a0)+
	subq.l	&4,%d0
	bne.w	clr

	jsr	_main

__rtt:
	bra.w	__rtt
