 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/exceptions.s,v $
 # $Revision: 1.6.84.3 $	$Author: kcs $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/09/17 21:04:06 $

 # HPUX_ID: @(#)exceptions.s	55.1		88/12/23 

 #(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
 #(c) Copyright 1979 The Regents of the University of Colorado,a body corporate 
 #(c) Copyright 1979, 1980, 1983 The Regents of the University of California
 #(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
 #The contents of this software are proprietary and confidential to the Hewlett-
 #Packard Company, and are limited in distribution to those with a direct need
 #to know.  Individuals having access to this software are responsible for main-
 #taining the confidentiality of the content and for keeping the software secure
 #when not in use.  Transfer to any party is strictly forbidden other than as
 #expressly permitted in writing by Hewlett-Packard Company. Unauthorized trans-
 #fer to or possession by any unauthorized party may be a criminal offense.
 #
 #                    RESTRICTED RIGHTS LEGEND
 #
 #          Use,  duplication,  or disclosure by the Government  is
 #          subject to restrictions as set forth in subdivision (b)
 #          (3)  (ii)  of the Rights in Technical Data and Computer
 #          Software clause at 52.227-7013.
 #
 #                     HEWLETT-PACKARD COMPANY
 #                        3000 Hanover St.
 #                      Palo Alto, CA  94304

#define LOCORE
#include "../machine/cpu.h"     /* Get CPU relative numbers. */

                set LOW,0x2000          # interrupt level 0
                set HIGH,0x2600         # total disable

 # XXX this define should go in proc.h and be printed by genassym.c
 #define S268040_FP  0x00040000  /* Flag for 68040 fp emulation */
		set S268040_FP,0x40000


 ###############################################################################
 # MC680X0 EXCEPTION VECTOR TABLE
 #
 # The following is the initialization of the exception vectors for 
 # the 680X0.
 #
 ###############################################################################

	global	_vectors, _fault, _busvec, _cnt
	global  _bsun, _inex, _dz, _unfl, _operr, _ovfl, _snan
	global	_Interrupt
	global _level_six_vec
	text

 # ALL INTERRUPT VECTORS (except lev7int) POINT TO THE SAME PLACE!!!!!!!

_vectors:
	long	0		# reset vector SSP
	long	0		# reset vector PC
_busvec:	
	long	_xbuserror	# bus error
	long	_fault		# address error
	long	_fault		# illegal instruction
	long	_fault		# zero divide
	long	_fault		# check instruction 
	long	_fault		# trapv instruction
	long	_fault		# privilege violation
	long	_fault		# trace trap
	long	_fault		# line 1010 emulator
	long	_fline		# line 1111 emulator
	long	0		# (unassigned,reserved)
 	long	_fault		# coprocessor protocol violation
 	long	_fault		# format error
	long	0		# (unassigned,reserved)
	long	0		# (unassigned,reserved)
	long	0		# (unassigned,reserved)
	long	0		# (unassigned,reserved)
	long	0		# (unassigned,reserved)
	long	0		# (unassigned,reserved)
	long	0		# (unassigned,reserved)
	long	0		# (unassigned,reserved)
	long	0		# (unassigned,reserved)
	long	_fault		# spurious interrupt
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler
	long	_Interrupt	# generic interrupt handler
	long	_Interrupt	# generic interrupt handler
	long	_Interrupt	# generic interrupt handler
_level_six_vec:
	long	_Interrupt	# generic interrupt handler
	long	_lev7int	# level 7 io interrupt
	long	_xsyscall	# trap 0 instruction
	long	_fault		# trap 1 instruction
	long	_fault		# trap 2 instruction
	long	_fault		# trap 3 instruction
	long	_fault		# trap 4 instruction
#ifdef	ICA_300
	long	xica_handler	# trap 5 instruction
#else
	long	_fault		# trap 5 instruction
#endif
	long	_fault		# trap 6 instruction
	long	_fault		# trap 7 instruction
	long	_fault		# trap 8 instruction
	long	_fault		# trap 9 instruction
	long	_fault		# trap 10 instruction
	long    _lw_call        # trap 11 instruction
	long    _cachectl       # trap 12 instruction
	long	_fault		# trap 13 instruction
	long	_fault		# trap 14 instruction
	long	_fault		# trap 15 instruction
 	long	_bsun		# 68881 branch/set byte on unordered condition
 	long	_inex		# 68881 inexact result
 	long	_dz		# 68881 divide by zero
 	long	_unfl		# 68881 underflow
 	long	_operr		# 68881 operand error
 	long	_ovfl		# 68881 overflow
 	long	_snan		# 68881 signalling NAN
 	long	_unsupp		# 68040 FP unimplemented data type

 	space	4*(8)		# skip vectors 56 - 63

 #	64 vectors for VME
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
 #	16 vectors for EISA
	long	_Interrupt	# generic interrupt handler 0x200
	long	_Interrupt	# generic interrupt handler 0x204
	long	_Interrupt	# generic interrupt handler 0x208
	long	_Interrupt	# generic interrupt handler 0x20c
	long	_Interrupt	# generic interrupt handler 0x210
	long	_Interrupt	# generic interrupt handler 0x214
	long	_Interrupt	# generic interrupt handler 0x218
	long	_Interrupt	# generic interrupt handler 0x21c
	long	_Interrupt	# generic interrupt handler 0x220
	long	_Interrupt	# generic interrupt handler 0x224
	long	_Interrupt	# generic interrupt handler 0x228
	long	_Interrupt	# generic interrupt handler 0x22c
	long	_Interrupt	# generic interrupt handler 0x230
	long	_Interrupt	# generic interrupt handler 0x234
	long	_Interrupt	# generic interrupt handler 0x238
	long	_Interrupt	# generic interrupt handler 0x23c
 #  	128-16=112  vectors for future ?
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 
	long	_Interrupt	# generic interrupt handler 

#ifdef	ICA_300
xica_handler:
	global	_ica_handler
	movm.l	%d0-%d7/%a0-%a7,-(%sp)	# save all registers
	mov.l	%usp,%a0		# save the user stack ptr
	mov.l	%a0,60(%sp)
	jsr	_ica_handler		# call ica handler in C
	mov.l	60(%sp),%a0
	mov.l	%a0,%usp		# restore usr stack ptr
	movm.l	(%sp)+,%d0-%d7/%a0-%a6	# restore all other registers
	addq.l	&4,%sp			# and pop off usp
	rte
#endif


 ###############################################################################
 ###############################################################################
 ##
 ## Exception Handling Routine
 ##
 ###############################################################################
 ###############################################################################
	global	_fault, _trap

_fault:
	movm.l	%d0-%d7/%a0-%a7,-(%sp)	# save all registers
	mov.l	%usp,%a0		# save the user stack ptr
	mov.l	%a0,60(%sp)
#ifdef	FSD_KI
	jsr	_ki_accum_push_TOS_trap	# accum clocks
#endif	FSD_KI
 	jsr	_trap			# C handler for traps and faults
	bra	xreturn

kstack_overflow_msg:
        byte     "kernel stack overflow",0
        lalign  4

	global	_xbuserror, _buserror, _bus_trial, _LAN_return_addr

_xbuserror:

 # 	There are a number of routines in the kernel (testr/testw and the
 # *copy_prot routines).  which must be able to recover from a bus error.
 # This used to be done by having them replace address 0x8, the bus error
 # vector, with the address of a recovery routine while they were running.
 # They would restore 0x8 when they had completed.  Due to historical reasons,
 # the LAN driver intialization code, hw_int_test in s200io/drvst.s also 
 # used this feature to survive bus error.
 #
 # 	All have now been changed to make use of a flag which indicates that
 # a "trial" is underway.  The current bus error might be the result of a
 # failed trial.  If so, the stack should be cleaned up and we should return
 # without further processing of this bus error.
 #
	tst.l	_bus_trial	# if 0, a bus trial is not active.
	beq.w	no_bus_trial	# if == 0, treat as normal bus error.

	cmp.l	_bus_trial,&1	# 1 means a protected routine trial was
	beq.b	is_bus_trial	# executing and has caused a bus error.

	cmp.l	_bus_trial,&2	# 2 means the networking initialization
	beq.b   is_bus_trial	# trial caused a bus error.
				# if neither a 0, 1 nor a 2, something is WRONG!
	mov.l	&busmsg,-(%sp) 	# panic if trial flag is unexpected value.
	jsr	_panic
is_bus_trial:			# Figure out the type of exception frame and
	mov.w	6(%sp),%d0	# clean up the stack.  See the MC68020 book.
	and.l	&0xf000,%d0
	cmp.w	%d0,&0x7000
	beq.w	buser4S
	cmp.w	%d0,&0x8000
	bne.w	buser1S
	add.w	&58,%sp
	bra.b	buser3S
buser1S:
	cmp.w	%d0,&0xa000
	bne.w	buser2S
	add.w	&32,%sp
	bra.b	buser3S
buser2S:
	add.w	&92,%sp
	bra.b	buser3S
buser4S:
	add.w	&0x3c,%sp

buser3S:
	cmp.l	_bus_trial,&1	# Was it a testr/testw or *copy_prot type trial?
	beq.b	is_prot_routine	# if so, do return for a protected routine trial


	# The routine hw_int_test in s200io/drvst.s is also protected  from
	# bus  errors.   It  sets the _bus_trial flag == 2 before it begins
	# initializing the LAN card.  Like type 1 trials, if  a  bus  error
	# does  occur,  xbuserror will catch it, and clean up the stack, as
	# above.
	# 
	# While stack clean up is the same for  both  type  1  and  type  2
	# trials,  the  setup  for  the  trial  and  the  return  to caller
	# activities are  very  different.  For  those  reasons,  the  full
	# recovery  for  type 1 trials is done here in _xbuserror, but only
	# stack clean up is done in _xbuserror for type 2 trials.
	# 
	# A type 2 trial should be from  the  LAN  driver,  and  we  simply
	# return  to  an address given to us. Where ever we jump back to is
	# responsible for restoring the old value of  _bus_trial  (probably
	# 0).   The recover portion of hw_int_ test must not expect %d0 and
	# %a0 be the same after bus error as they  were  before,  since  we
	# used them to clean the stack and for the jmp.
	# 
	# To be careful, if the address is zero, we know that  it  can  not
	# possibly  be a valid place to jump back to.  (Zero is its initial
	# value).

	tst.l	_LAN_return_addr	# If zero, bad news.  A valid return
	beq.b	cant_go_back		# address was not set by hw_int_test.

	mov.l	_LAN_return_addr,%a0	# Is non-zero, assume we can jump there
	jmp	(%a0)			# and be back in the LAN driver code.

cant_go_back:
	mov.l	&no_addrmsg,-(%sp) 	# panic if return address is zero.
	jsr	_panic

is_prot_routine:
	movq	&0,%d0			# return 0 in %d0 to mean failure.
	mov.l	%d1,_bus_trial		# put previous value of the flag back.
	rts				# return to caller

no_bus_trial:

	movm.l	%d0-%d7/%a0-%a7,-(%sp)	# save all registers
	mov.l	%usp,%a0
	mov.l	%a0,60(%sp)		# save usr stack ptr
	tst.l   _mainentered            # Panic if main not entered
	bne.w   xbuserror0

 # u is not mapped before main is entered. If we continue from here
 # we will recursively enter this routine until we double bus fault.

	pea     no_main_err
	jsr     _panic
no_main_err:
	byte "xbuserror: bus error before main entered",0,0

xbuserror0:
        mov.l   &KSTACKBASE,%a0         # Check to make sure that we have
        add.l   stack_death,%a0         #   at least stack_death bytes
        cmp.l   %sp,%a0                 #   (initially 2K) left on the stack
        bcc.b   xbuserror1S		# ok, continue
        mov.l   &0,stack_death          # give panic room to breath
        pea     kstack_overflow_msg     # message
        jsr     _panic                  # print warning
        addq.l  &4,%sp                  # possible continue 

xbuserror1S:
	tst.l	_smart_poll		# check if smart polling active
	beq.b	xbuserror0S		# no, continue
	jsr	_do_smart_poll		# brain damaged hardware
xbuserror0S:
#ifdef	FSD_KI
	jsr	_ki_accum_push_TOS_vfault # accum clocks
#endif	FSD_KI
        mov.l   &_processor,%a0         # get variable address
        cmpi.l  (%a0),&M68040           # is this an MC68040?
        bne.w   not_mc68040_buserr      # branch if not

 ##############################################################################
 # Begin 68040 Mask xxd43b Bug #1238 Workaround
 #
 # Psuedo Code from Motorola:
 #
 # 1. int_fault_pc = [A7+2]
 #
 # 2. if ((int_fault_pc != xxxxxff8) && (int_fault_pc != xxxxxffa)) 
 #         {goto NOFIX}
 #
 # 3. if (resident(int_fault_pc) == FAULT) {goto NOFIX}
 #
 # 4. opcode = [int_fault_pc]
 #    if (int_fault_pc == xxxxxff8) {
 #        if( (opcode && fffffc00) != f2394400 ) {goto NOFIX}
 #        else if (resident(int_fault_pc + $10) != FAULT) {goto NOFIX}
 #    }
 #    else if (int_fault_pc == xxxxxffa) {
 #        if( ((opcode && fffffc00) != f2394400) &&
 #            ((opcode && fff8fc00) != f2284400) ) {goto NOFIX}
 #        else if (resident(int_fault_pc + $10) != FAULT) {goto NOFIX}
 #
 # 5. fsave
 #    if ((int_fault_pc != fpiarcu) || (bug_flag_procIDxxxx == SET)) 
 #       { goto NOFIX_FRESTORE }
 #
 # 6. if (fsave_format_version != $40 ) {goto NOFIX_FRESTORE}
 #            /* This is done much earlier, for efficiency */
 #
 # 7. if !(E3_flag)  {goto NOFIX_FRESTORE}
 #    if (cupc == 0) {goto NOFIX_FRESTORE}
 #    if (cmdreg1b[15:10] != 010001) {goto NOFIX_FRESTORE}
 #    if (cmdreg1b[9:7] != cmdreg3b[9:7]) {goto NOFIX_FRESTORE}
 #
 # 8. cupc = 0
 #
 # 9. NOFIX_FRESTORE: frestore
 #
 # 10. NOFIX: continue with tlb fault handling
 #
 ##############################################################################
 		cmpi.l	_fsave_version,&0x40	# 6. Check for bad chip
 		bne.w	NOFIX

		mov.l 	66(%sp),%d7		# 1. Get User PC
		mov.l	%d7,%d6
		and.l	&0xfff,%d6              # 2. Check for end of page PC
		cmp.l	%d6,&0xff8
		beq.b	step_3
		cmp.l	%d6,&0xffa
		bne.w	NOFIX

step_3:	mov.l	&2,-(%sp)			# 3. check for PC page present
		mov.l	%d7,-(%sp)
		jsr		_mc68040_ptest
		add.l	&8,%sp
		btst	&0,%d0
		beq.w	NOFIX

		mov.l	%d7,-(%sp)		# 4. Get opcode
		jsr		_fuword
		add.l	&4,%sp
		and.l	&0xfffffc00,%d0
		cmp.l	%d0,&0xf2394400
		beq.b	check_page
		cmp.l	%d6,&0xff8
		beq.w	NOFIX
		and.l	&0xfff8fc00,%d0
		cmp.l	%d0,&0xf2284400
		bne.w	NOFIX
check_page:
		mov.l	&2,-(%sp)
		mov.l	%d7,%d0
		add.l	&0x10,%d0
		mov.l	%d0,-(%sp)
		jsr		_mc68040_ptest
		add.l	&8,%sp
		btst	&0,%d0
		bne.b	NOFIX

		link	%a6,&0
		fsave	-(%sp)			# 5. FSAVE and check conditions
		cmpi.w	(%sp),&0x4060		# Check for 4060 frame out
		beq.b   cont_bugfix	
	 	cmpi.b	1(%sp),&0
		beq.b	NOFIX_UNLNK
		bra.b	NOFIX_FRESTORE	
cont_bugfix:	fmove.l	%fpiar,%d0
		cmp.l	%d0,%d7
		bne.b	NOFIX_FRESTORE
		mov.l	_u+U_PROCP,%a0
		mov.l	P_FLAG2(%a0),%d0
		and.l	&S268040_FP,%d0
		bne.b	NOFIX_FRESTORE

 # Some constants, from the end of the fsave state frame (where %a6 is)
		set		CMDREG1B,-36
		set		CMDREG3B,-48
		set		CU_SAVEPC,-92
		set		E_BYTE,-28
		set		E3,1

		btst.b	&E3,E_BYTE(%a6)
		beq.b	NOFIX_FRESTORE		
		move.b	CU_SAVEPC(%a6),%d0
		andi.b	&0xFE,%d0
		beq.b	NOFIX_FRESTORE
		move.w	CMDREG1B(%a6),%d0
		andi.w	&0xfc00,%d0
		cmpi.w	%d0,&0x4400
		bne.b	NOFIX_FRESTORE
		bfextu	CMDREG3B(%a6){&6:&3},%d1
		bfextu	CMDREG1B(%a6){&6:&3},%d0
		cmp.b	%d0,%d1
		beq.b	NOFIX_FRESTORE

		clr.w	CU_SAVEPC(%a6)

NOFIX_FRESTORE:
		frestore  (%sp)+
NOFIX_UNLNK:
		unlk	%a6
		
NOFIX:
 ##############################################################################
 # End 68040 Mask xxd43b Bug #1238 Workaround
 ##############################################################################

        jsr     _mc68040_access_error	# call C handler
        bra     xreturn

not_mc68040_buserr:
	tst.l	_pmmu_exist
	beq.b	notpmmuberr
	jsr	_pmmubuserror
	bra	xreturn

notpmmuberr:
	jsr	_buserror		# C bus error handler
	bra	xreturn

 ###############################################################################
 #
 #     writeback(fault_address, data, size, function_code)
 #     int fault_address, data, size, function_code;
 #
 ###############################################################################

	global	_writeback,_wb_error

_writeback:

	mov.l	_u+PROBE_ADDR,%a1

	mov.l	&_wb_error,_u+PROBE_ADDR # setup return in case of error

	mov.l   4(%sp),%a0    		# get address
	mov.l   8(%sp),%d0    	 	# get data
	mov.l	12(%sp),%d1		# get the size
	cmpi.l	16(%sp),&1		# check fc - check if user space
	beq.b	writeback_usr
	cmpi.l	16(%sp),&5		# check fc - check if kernel space
	beq.b	writeback_sys

        mov.l   &wb_msg2,-(%sp)  	# panic on unknown fc
        jsr     _panic

writeback_usr:
	tst.l	%d1
	bne.b	not_long_usr		# branch if size != 0 (0 means long)

	movs.l  %d0,(%a0)       	# store the data
	nop				# empty pipe before the movs executes
	bra.b	wb_cleanup

not_long_usr:
	subq	&1,%d1			# size = short?
	bne.b	not_byte_usr
	
	movs.b  %d0,(%a0)       	# store the data
	nop				# empty pipe before the movs executes
	bra.b	wb_cleanup

not_byte_usr:
	subq	&1,%d1			# size = long?
	bne.b	not_word

	movs.w  %d0,(%a0)       	# store the data
	nop				# empty pipe before the movs executes
	bra.b	wb_cleanup

writeback_sys:
	tst.l	%d1
	bne.b	not_long_sys		# branch if size != 0 (0 means long)

	mov.l  %d0,(%a0)	       	# store the data
	nop				# empty pipe before the mov executes
	bra.b	wb_cleanup

not_long_sys:
	subq	&1,%d1			# size = short?
	bne.b	not_byte_sys
	
	mov.b  %d0,(%a0) 	      	# store the data
	nop				# empty pipe before the mov executes
	bra.b	wb_cleanup

not_byte_sys:
	subq	&1,%d1			# size = long?
	bne.b	not_word

	mov.w  %d0,(%a0)   	    	# store the data
	nop				# empty pipe before the mov executes
	bra.b	wb_cleanup

not_word:
        mov.l   &wb_msg1,-(%sp)  	# panic on unknown size
        jsr     _panic

wb_cleanup:
	mov.l	%a1,_u+PROBE_ADDR	# restore probe return
	movq	&0,%d0			# initialize success return value
	rts

_wb_error: 
	mov.l	%a1,_u+PROBE_ADDR	# restore probe return
	mov.l	&-1,%d0			# return error
	rts

	# declare panic message for writeback
	data
stack_death:            long    2048    # Die on buserror if only 2K left
wb_msg1:
        byte    "_writeback: Unexpected size in writeback",0
wb_msg2:
        byte    "_writeback: Unexpected function code in writeback",0
	text

 ###############################################################################
 #
 #     writeback_line(fault_address, data, function_code)
 #     int fault_address;
 #     int *data;
 #     int function_code;
 #
 ###############################################################################

	global	_writeback_line,_wbl_err

_writeback_line:

	mov.l	_u+PROBE_ADDR,%d1	# save old probe address
	mov.l	&_wbl_err,_u+PROBE_ADDR	# setup return in case of error

        mov.l   4(%sp),%a0      	# get address
        mov.l   8(%sp),%a1      	# get address of data
	cmpi.l	12(%sp),&1		# check if fc == usr
	beq.b	writeback_line_usr

	cmpi.l	12(%sp),&5		# check if fc == sys
	beq.b	writeback_line_sys

        mov.l   &wb_msg2,-(%sp)  	# panic on unknown fc
        jsr     _panic

writeback_line_sys:
	mov.l	(%a1)+,(%a0)+		# write 4 bytes
        nop                     	# empty pipe before the mov executes
	mov.l	(%a1)+,(%a0)+		# write 4 bytes
        nop                     	# empty pipe before the mov executes
	mov.l	(%a1)+,(%a0)+		# write 4 bytes
        nop                     	# empty pipe before the mov executes
	mov.l	(%a1),(%a0)		# write 4 bytes
        nop                     	# empty pipe before the mov executes
	mov.l	%d1,_u+PROBE_ADDR	# restore probe return
	movq	&0,%d0			# initialize success return value
        rts
	
writeback_line_usr:
        mov.l   (%a1)+,%d0      	# get the data
        movs.l  %d0,(%a0)+      	# store the data
        nop                     	# empty pipe before the movs executes
        mov.l   (%a1)+,%d0      	# get the data
        movs.l  %d0,(%a0)+      	# store the data
        nop                     	# empty pipe before the movs executes
        mov.l   (%a1)+,%d0      	# get the data
        movs.l  %d0,(%a0)+      	# store the data
        nop                     	# empty pipe before the movs executes
        mov.l   (%a1)+,%d0      	# get the data
        movs.l  %d0,(%a0)       	# store the data
        nop                     	# empty pipe before the movs executes
	mov.l	%d1,_u+PROBE_ADDR	# restore probe return
	movq	&0,%d0			# initialize success return value
        rts

_wbl_err: 
	mov.l	%d1,_u+PROBE_ADDR	# restore probe return
	mov.l	&-1,%d0			# return error
	rts

 ###############################################################################
 #
 #     mc68040_ptest(fault_address, function_code, read_access)
 #     int fault_address, function_code, read_access;
 #
 #     This routine is used to execute the  ptest  instruction  for  the
 #     given  fault address in the space specified by the given function
 #     code.  The read_access parameter is 1 if we should test for  read
 #     access  and  0 if we should test for write access.  This function
 #     returns the value of the mmu  status  register  for  the  ptested
 #     address.
 #
 ###############################################################################

	global _mc68040_ptest

_mc68040_ptest:
	movc	%dfc,%d1 	# save destination function code register
	mov.l	8(%sp),%d0	# get the function code
	movc	%d0,%dfc	# load fc for the address to test
	mov.l	4(%sp),%a0	# get the test address
	short	0xF568		# ptestr (%a0)
	long	0x4E7A0805	# movc mmusr,%d0 	# return value in %d0
	movc	%d1,%dfc	# restore destination function code
	rts

	data
_bus_trial:		#if 0, a bus trial is not active. Make
	long	0	#it global so LAN driver can use it.

_LAN_return_addr:	# hw_int_test will put a value here for
	long	0	# _xbuserror to jump back to.

busmsg:			# message means unknown trial type.
	byte	"_xbuserror: Unexpected value in _bus_trial",0
no_addrmsg:		# message means can not jump back to LAN
	byte	"_xbuserror: Return address in _LAN_return_addr is zero",0
	text

	global _linef_trial

	data
_linef_trial:		#if 0, an MC68881 trial is not active.
	long	0
linefmsg:		# use as panic message, if need to
	byte	"_linef_emulate: Unexpected value in _linef_trial",0
	text

 # 	If a LINE F emulation trap occurs, there are two possibilities.  The
 # first is that _initialize_68881 is testing for an MC68881 and one is not
 # present, thus we trap to here.  If that is the case, the trial flag will be
 # set and we can just clean up and return to the caller of _initialize_68881.
 # The other case is that an unexpected line f emulation has occured.  If so
 # pass it off to be handled by _fault.

	global _linef_emulate

_linef_emulate:
	tst.l	_linef_trial	# if 0, an init 68881 trial is not active.
	beq.w	_fault		# if not a trial, treat emulation trap as fault.

	cmp.l	_linef_trial,&1 # if a 1, this is a trial and we clean up.
	beq.b	is_linef_trial
				 # if it is neither 0 nor 1, something is WRONG!
	mov.l	&linefmsg,-(%sp) # panic msg if trial flag is unexpected value.
	jsr	_panic
is_linef_trial:
	movq	&0,%d0			# mc68881 not present, return 0.
	lea	12(%sp),%sp		# pop off exception frame + clr.l
	mov.l	%a1,_linef_trial	# restore prev value of trial variable
	rts				# back to the caller of _intialize_68881

 ###############################################################################
 # Light Weight Call Interface
 ###############################################################################

 # Note, there really is not a true light weight call interface here.
 # I just thought I should stop taking whole trap numbers for every
 # light weight call. So far, the only routine here is a routine that
 # retrieves the users msemaphore lock id. The user interface to this
 # routine sets %d0 to 0 and %d1 to 1. A possible convention would be:
 #
 #      Value of %d1:
 #          0: lightweight system call, where %d0 contains standard syscall #
 #          1: misc. lightweight calls that don't have an equiv. syscall
 #          2: ???
 #
 #      It might make more sense for lightweight system calls to use
 #      the current syscall trap # (0), so that old binaries could take
 #      advantage of them. Of course, this is all a pipe dream, considering
 #      the current stage of the Series 300/400 lifecycle.

	global  _lw_call

_lw_call:
	cmpi.l  %d1,&0x1
	bne.w   lw_call_err
	tst.l   %d0
	bne.w   lw_call_err

msem_lock_id:
	mov.l   _u+U_PROCP,%a0
	mov.l   P_MSEMPINFO(%a0),%d0
	beq.w   msem_not_alloc
	mov.l   %d0,%a0
	mov.l   MSEM_LOCKID(%a0),%d0
	rte

msem_not_alloc:
lw_call_err:
	mov.l	&-1,%d0			# return error
	rte

 ###############################################################################
 # System Call Interface
 ###############################################################################
	global	_xsyscall,_syscall

_xsyscall:
	movm.l	%d0-%d7/%a0-%a7,-(%sp)	# save all 16 registers
	mov.l	%usp,%a0
	mov.l	%a0,60(%sp)		# save usr stack ptr
#ifdef	FSD_KI
	jsr	_ki_accum_push_TOS_sys	# accum clocks
#endif	FSD_KI
	tst.l	_smart_poll		# check if smart polling active
	beq.b	xsyscall0		# no, continue
	jsr	_do_smart_poll		# brain damaged hardware
xsyscall0:
	jsr	_syscall		# C handler for syscalls
#ifdef	FSD_KI
	tst.b	_ki_cf+KI_SYSCALLS	# /* XXX */ a little extra overhead
	beq.b	xreturn 		# branch if so

	jsr	_ki_syscalltrace	# log system calls
#endif	FSD_KI

 # syscall and xbuserror common return code

	global	xreturn			# for profiler only!!!!!!
	global	_vapor_malloc_free	# yes, go free the memory
xreturn:
	tst.l	_u+U_VAPOR_MLIST	# check if VAPOR_MALLOC memory to release
	beq.b	xreturnA		# no, continue
	btst	&5,64(%sp)		# supervisor mode?
	bne.b	xreturnA		# yes, continue
	jsr	_vapor_malloc_free	# user mode, go free the memory
xreturnA:
	tst.l	_smart_poll		# check if smart polling active
	beq.b	xreturnB		# no, continue
	jsr	_do_smart_poll		# brain damaged hardware
xreturnB:
	tst.l	_sw_queuehead		#any pending software triggers?
	beq.b	xreturnC		#branch if not
	movq	&7,%d0			# get int level mask
	and.b	64(%sp),%d0		# get level from exception stack frame
	mov.l	%d0,-(%sp)		# move level on stack
 	jsr	_splx			# service any pending software triggers
	addq.l	&4,%sp			# pop off parameter
xreturnC:
#ifdef	FSD_KI
	jsr	_ki_accum_pop_TOS_sys	# accum clocks
#endif	FSD_KI
	mov.l	60(%sp),%a0
	mov.l	%a0,%usp		# restore usr stack ptr
	movm.l	(%sp)+,%d0-%d7/%a0-%a6	# restore all other registers
	addq.l	&4,%sp			# and pop off usp

	bclr	&POP_STACK_BIT,_u+PCB_FLAGS
	bne.b	xreturn1
	rte


xreturn1:
	btst	&6,6(%sp)		# 68040 access error frame?
	bne.b	xreturn5		# branch if so
	btst	&5,6(%sp)		# 68020 bus error frame?
	bne.b	xreturn3		# branch if so
	btst	&4,6(%sp)		# type 0x8xxx or 0x9xxx?
	bne.b	xreturn2		# branch if 0x9xxx
	mov.w	(%sp),50(%sp)		# convert 68010 bus error stack frame
	mov.l	2(%sp),52(%sp)		# to short format as the pc was changed
	clr.w	56(%sp)			# and the faulted instruction must not
	add.l	&50,%sp			# be continued.
	rte

xreturn2:
	mov.w	(%sp),0xc(%sp)		# convert the 0x9xxx format stack frame
	mov.l	2(%sp),0xe(%sp)		# to the 0x0xxx format stack frame as
	clr.w	0x12(%sp)		# the faulted instruction must not be
	add.l	&0xc,%sp		# continued
	rte

xreturn3:
	btst	&4,6(%sp)		# type 0xAxxx or 0xBxxx?
	bne.b	xreturn4		# branch if 0xbxxx
	mov.w	(%sp),0x18(%sp)		# convert the 0xAxxx format stack frame
	mov.l	2(%sp),0x1a(%sp)	# to the 0x0xxx format stack frame as
	clr.w	0x1e(%sp)		# the faulted instruction must not be
	add.l	&0x18,%sp		# continued
	rte

xreturn4:
	mov.w	(%sp),0x54(%sp)		# convert the 0xBxxx format stack frame
	mov.l	2(%sp),0x56(%sp)	# to the 0x0xxx format stack frame as
	clr.w	0x5a(%sp)		# the faulted instruction must not be
	add.l	&0x54,%sp		# continued
	rte

xreturn5:
	# XXXX do we really want to do this?
	mov.w   (%sp),0x34(%sp)         # convert the 0x7xxx format stack frame
	mov.l   0x2(%sp),0x36(%sp)      # to the 0x0xxx format stack frame as
	clr.w   0x3a(%sp)               # the faulted instruction must not be
	add.l   &0x34,%sp               # continued
	rte

	# 
	# This routine is invoked by sigcleanup to complete the writebacks
	# of a bus error that caused the users bus error signal handler to
	# be invoked.
	#
	global _restart_writebacks

_restart_writebacks:
#ifdef  FSD_KI
	jsr     _ki_accum_push_TOS_vfault # accum clocks
#endif  FSD_KI

	# allocate room on the stack for the exception frame
	sub.l	&EXCEPTION_STACK_SIZE,%sp
	mov.l	%sp,%a0 			# save pointer to start

	# call bcopy to restore the exception frame
	mov.l	&EXCEPTION_STACK_SIZE,-(%sp)	# count
	mov.l	%a0,-(%sp)			# destination
	mov.l	_u+PCB_LOCREGS,-(%sp)		# source
	jsr	_bcopy
	add.l	&0xc,%sp

	# call the writeback handler
	mov.l	%sp,-(%sp)			# locregs pointer
	jsr	_writeback_handler
	add.l	&4,%sp

	bra	xreturn

	version 2
