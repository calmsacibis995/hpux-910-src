 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/ddb_asm.s,v $
 # $Revision: 1.2.84.3 $	$Author: kcs $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/09/17 21:03:33 $

#ifdef DDB

 # Low level interface to DDB
 #

 #
 # DDB controls the following vectors:
 #	bus error 	(0x00000008)
 #	address error	(0x0000000c)
 #	trace		(0x00000024)
 #	trap14		(0x000000b8)	ddb breakpoints
 #
 # DDB saves and restores the bus error and address error vectors.
 # Trace and trap14 are always controlled by DDB.  DDB
 # code decides when to "pass on" these exceptions.
 #


   	set	BUS,0x00000008
       	set	ADDRESS,0x0000000c
     	set	TRACE_TRAP,0x00000024
      	set	TRAP14,0x000000b8

 #
 # Addresses of mmu registers
 #

#define M68040  0x03

 	global _current_io_base
        set     _current_io_base, 0xfffffdf0

	set	MMUCNTLPHYS,0x5f400e

	global _ddb_vectors

_ddb_vectors:
	
	mov.l	&1,-(%sp)
	bsr	_save_vectors
	addq.l	&4,%sp
        bsr     _restore_vectors
        bsr     _ddb_purge

        rts                                     #* return to HP-UX!


 ###############################################################################
 # ddb_stop()		
 #
 # The clock isr calls ddb_poll_ss to check for an interrupt from the ddb
 # host.  If the host wants to interrupt, ddb_poll_ss calls ddb_stop.  By
 # doing a trap, ddb_stop sends us into the normal ddb path like a breakpoint.
 ###############################################################################

	global _ddb_stop
_ddb_stop:	
	trap    &0x0e				#* pass control to ddb
        rts                                     #* return


 ###############################################################################
 # ddb_call_proc()		
 #
 # This routine provides the target side code required to implement a ddb
 # command line procedure call.  The host side creates a stack frame for
 # the call and copies it to the target in ddb_proc_stack, copies the proc
 # address to ddb_proc_addr, and finally sets the ddb_do_proc flag.
 # The ddb_do_proc flag also contains the number of words on the stack + 1.
 #
 # ddb_call_proc then pushes the stack frame onto the real stack and calls
 # the mystery routine.  It then sets up the result, pops the stack frame,
 # and returns.
 ###############################################################################

	global _ddb_call_proc
_ddb_call_proc:
        mov.l   %d2,-(%sp)                      #* save register d2 on stack
        mov.l   _ddb_do_proc,%d2
        subq.l  &1,%d2                          #* decrement ddb_do_proc (n+1)
        mov.l   &_ddb_proc_stack,%a0            #* mv addr of stack buff to a0
        mov.l   _ddb_proc_addr,%a1              #* move addr of proc to a1
        cmp.l   %d2,&0x0                        #* check for zero params
        beq.b   push_done
push_start:
        subq.l  &1,%d2                          #* decrement param count
        mov.l   (%a0)+,-(%sp)                   #* push parameter onto stack
        cmp.l   %d2,&0x0                        #* check if done
        bne.b   push_start
push_done:
        jsr     (%a1)                           #* call mystery procedure
	mov.l	%d0,_ddb_proc_stack		#* put result in ddb_proc_stack

        mov.l   _ddb_do_proc,%d2
        subq.l  &1,%d2                          #* decrement param count (n+1)
        cmp.l   %d2,&0x0                        #* check for zero params
        beq.b   pop_done
pop_start:
        subq.l  &1,%d2                          #* decrement param count
        addq.l  &4,%sp                          #* pop one parameter
        cmp.l   %d2,&0x0                        #* check if done
        bne.b   pop_start
pop_done:
        mov.l   (%sp)+,%d2                      #* restore register d2
	rts



 ###############################################################################
 # ddb_bp()		
 #
 # Exception handler for DDB.  This code sorts out the type of exception from 
 # the vector offset word.  There are 3 exceptions which DDB handles (in 
 # addition to bus and address , see ddb_except()) :
 #	trace  -- single stepping
 #	trap14 -- breakpoint - control passes from kernel to ddb 
 ###############################################################################

	global _ddb_bp

_ddb_bp:	
	mov.w	&0x2700,%sr			#* disable everything
	mov.w	(%sp)+,_ddb_sr			#* save hp-ux status register
	mov.l	(%sp)+,_ddb_pc			#* save hp-ux program counter
	mov.w	(%sp)+,_ddb_vecoffset		#* save vector offset
	movm.l	%d0-%d7/%a0-%a7,_ddb_kernelregs	#* save hp-ux registers
	mov.w	_ddb_vecoffset,%d0		#* get vector offset
	and.l	&0x00000fff,%d0
	cmp.w	%d0,&0x0024			#* trace trap?
	bne.b	ddb_bp0				#* branch if not	
	mov.l	(%sp)+,_ddb_pc2			#* save away actual pc
	mov.l	&_ddb_kernelregs,%a0
	add.l	&4,0x3c(%a0)			#* adjust saved a7
	and.w	&0x0fff,_ddb_vecoffset		#* clear type field

ddb_bp0:	
	bsr	_ddb_save_regs
	mov.l	&0,-(%sp)
	bsr	_save_vectors
	addq.l	&4,%sp
	mov.w	_ddb_vecoffset,%d0		#* get vector offset
	and.l	&0x00000fff,%d0
	cmp.w	%d0,&TRACE_TRAP			#* trace trap?
 	beq.w	ddb1				#* if so then enter ddb
	cmp.w	%d0,&TRAP14			#* trap14?
 	beq.w	ddb1				#* if so then enter ddb
	bra.w	_ddb_cont			#* otherwise ignore
ddb1:
	jsr	_ddb_int			#* onto DDBland

 ###############################################################################
 #
 ###############################################################################

 #
 #  This code is called when DDB is returning control
 #  back to hp-ux. 
 #

	global _ddb_cont

_ddb_cont:

	bsr	_restore_vectors
	bsr	_ddb_restore_regs
	bsr	_ddb_purge

	movm.l	_ddb_kernelregs,%d0-%d7/%a0-%a7	#* restore hp-ux registers
	mov.w	_ddb_vecoffset,-(%sp)		#* push  proper stack frame
	mov.l	_ddb_pc,-(%sp)			#* so that a return can be
	mov.w	_ddb_sr,-(%sp)			#* made to hp-ux
	rte					#* return to HP-UX!


 ###############################################################################
 # ddb_except()		bus and address error handler
 ###############################################################################

	global	_ddb_except

_ddb_except:
	lea	_ddb_exception,%a0		#* address of save area
	mov.w	(%sp)+,(%a0)+			#* save sr
	mov.l	(%sp)+,(%a0)+			#* save pc
	mov.w	(%sp)+,%d0			#* get vector offset word
	mov.w	%d0,(%a0)+			#* save it away
	lsr.w	&8,%d0				#* get format field
	lsr.w	&4,%d0

	cmp.w	%d0,&0x7
	bne.b	not_access_err
	
	mov.l	&26,%d0
	bra	ddb3

not_access_err:
	
	cmp.w	%d0,&0xA
	bne.b	not_020_short
	
	mov.l	&12,%d0
	bra	ddb3

not_020_short:
	
	cmp.w	%d0,&0xB
	bne.b	not_020_long
	
	mov.l	&42,%d0
	bra	ddb3

not_020_long:
	
	cmp.w	%d0,&0x9
	bne.b	not_coproc
	
	mov.l	&6,%d0
	bra	ddb3

not_coproc:
	mov.l	&0,%d0

ddb3:	
        tst.l   %d0                             #* enter loop
        beq.b   ddb4
	mov.w	(%sp)+,(%a0)+			#* save em
	subq.l	&1,%d0
	bra.b	ddb3				#* re enter ddb
ddb4:
        mov.w   _ddb_exception+6,%d0            #* get vector offset
        and.l   &0x00000fff,%d0                 #* mask off mode
 #      mov.l   %d0,-(%sp)                      #* push exception flag
	rte


 ###############################################################################
 # ddb_save_regs()
 ###############################################################################

	global	_ddb_save_regs

_ddb_save_regs:

	movc	%sfc,%d0		# save source function code reg
	mov.l	%d0,_ddb_fregs

	movc	%dfc,%d0		# save dest function code reg
	mov.l	%d0,_ddb_fregs+4

	movc 	%usp,%d0		# save user stack pointer
	mov.l	%d0,_ddb_usp

	rts

 ###############################################################################
 # ddb_restore_regs()
 ###############################################################################

	global	_ddb_restore_regs	

_ddb_restore_regs:
	
	mov.l	_ddb_fregs,%d0		# restore src function code reg
	movc 	%d0,%sfc

	mov.l	_ddb_fregs+4,%d0	# restore dest function code reg
	movc 	%d0,%dfc

	mov.l	_ddb_usp,%d0		# restore user stack pointer
	movc 	%d0,%usp

	rts

 ###############################################################################
 # Purge everything. TLBs, caches,...
 ###############################################################################
	global	_ddb_purge

_ddb_purge:
	cmp.l   _processor,&M68040      # have to treat the MC68040 special
	beq.w   ddb_purge_68040

	tst.l	_pmmu_exist	# is it an HP MMU?
	beq.w	hpmmu
	
	########################################################################
	# This as an MC68851 or and MC68030 MMU
	########################################################################
	# flush the tlb
	long	0xf0002400	# pflusha

	# flush on chip data and intruction caches
	mov.l	&IC_CLR+IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
	long	0x4e7b0002 	# movec	d0,cacr

	bra	purge_end

hpmmu:
	########################################################################
	# This as an HP memory mapped MMU (s320 or s350)
	########################################################################

        mov.l   &0x5F400A,%a0
        add.l   _current_io_base,%a0
        mov.w   (%a0),%d0

	# flush on chip data and intruction caches
	mov.l   &IC_CLR+IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
	long    0x4e7b0002      # movec d0,cacr

	bra	purge_end

ddb_purge_68040:
	########################################################################
	# This as an MC68040 MMU
	########################################################################

	# flush all entries from both user and supervisory TLBs
	nop
	short	0xF518		# pflusha

	# The MC68040 has physical caches so we dont have
	# to invalidate corresponding cache entries
	nop
	short	0xF4F8	# cpusha IC/DC
	nop
purge_end:
        mov.l   &MMUCNTLPHYS,%a0
        add.l   _current_io_base,%a0
	btst	&2,1(%a0)
	beq	cache_off

        and.w   &0xfffb,(%a0)
        or.w    &0x0004,(%a0)

cache_off:
	rts

 ###############################################################################
 # save_vectors(all)
 ###############################################################################

	global	_save_vectors

_save_vectors:

   	jsr	_WriteEnable			#* Allow mods to kernel text.
	jsr	_ddb_purge

 #	mov.l	BUS,_ddb_saved_bus		#* save hp-ux bus error
 # 	mov.l	&_ddb_except,BUS		#* install DDB handler	
 #	mov.l	ADDRESS,_ddb_saved_addr		#* save hp-ux address error
 # 	mov.l	&_ddb_except,ADDRESS		#* install DDB handler

	tst.l	4(%sp)
	beq	all_saved

   	mov.l	TRAP14,_ddb_saved_trap14	#* save hp-ux trap14
   	mov.l	&_ddb_bp,TRAP14			#* install DDB handler

 	mov.l	&0,%d0
 	cmp.l	TRACE_TRAP,&_ddb_bp
 	beq.w	all_saved
 	mov.l	TRACE_TRAP,_ddb_saved_trace	#* save hp-ux trace trap
 	mov.l	&_ddb_bp,TRACE_TRAP		#* install DDB handler
	mov.l	&1,%d0

all_saved:
	rts

 ###############################################################################
 # restore_vectors
 ###############################################################################

	global	_restore_vectors

_restore_vectors:

 #	mov.l	_ddb_saved_addr,ADDRESS	# restore hp-ux address handler
 #	mov.l	_ddb_saved_bus,BUS	# restore hp-ux bus handler
	mov.l	_ddb_saved_trace,TRACE_TRAP	# restore hp-ux trace trap
	tst.l	_ddb_trace		# if not tracing, leave it else...
	beq.b	do_writeprotect

        mov.l   &_ddb_bp,TRACE_TRAP	# else tracing, so let DDB catch trap

do_writeprotect:
   	jsr	_WriteProtect		# Disable mods to kernel text.
	rts

 #
 # Various save areas for DDB
 #
	global	_ddb_kernelregs,_ddb_dregs,_ddb_aregs
	global	_ddb_sr,_ddb_pc,_ddb_vecoffset
	global	_ddb_saved_bus,_ddb_saved_addr
	global	_ddb_saved_trace,_ddb_saved_trap14
	global	_ddb_usp
	global	_ddb_fregs
	global  _contflag
	global  _intp_reason
	global	_ddb_exception
	global	_vpc
	global	_ddb_trace
	global	_ddb_do_proc,_ddb_proc_addr,_ddb_proc_stack

	data
	lalign	4
_ddb_trace:	long	0		#* flag for ddb to indicate tracing
_contflag:	long	0		#* flag for ddb to continue
_intp_reason:	long	0		#* flag for ddb interrupt reason
_ddb_do_proc:	long	0		#* flag to make cmd-line proc call
_ddb_proc_addr:	long	0		#* addr of proc to call
_ddb_proc_stack:	space	4*(256)	#* stack w/ params for proc call
_ddb_kernelregs:
_ddb_dregs:	space	4*(8)		#* hp-ux saved data registers
_ddb_aregs:	space	4*(8)		#* hp-ux saved address registers

_ddb_usp:	space	4*(1)		#* hp-ux saved user stack pointer

fill:		space	2*(1)
_ddb_sr:	space	2*(1)		#* hp-ux status register
	lalign	4
_vpc:					#* this is what cdb calls it
_ddb_pc:	space	4*(1)		#* hp-ux program counter
_ddb_pc2:	space	4*(1)		#* actual program counter (normal 6)
_ddb_vecoffset:	short	0		#* hp-ux vector offset

	lalign	4
_ddb_fregs:	space	4*(2)		#* function code register save area	

_ddb_saved_bus:	space	4*(1)		#* hp-ux bus error handler	
_ddb_saved_addr:	space	4*(1)		#* hp-ux address error handler	

	lalign	4
_ddb_saved_trace:  space	4*(1)		#* hp-ux trace trap handler
_ddb_saved_trap14: space	4*(1)		#* hp-ux trap14 handler	

	lalign	4
_ddb_exception:	space	2*(46)		#* bus/address error save area

#endif DDB
