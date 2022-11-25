 # @(#) $Revision: 70.3 $     

 # Low level interface to KDB
 #

 #
 # KDB controls the following vectors:
 #	bus error 	(0x00000008)
 #	address error	(0x0000000c)
 #	trace		(0x00000024)
 #	trap14		(0x000000b8)	call kdb from shell
 #	trap15		(0x000000bc)	breakpoints
 #
 # KDB saves and restores the bus error and address error vectors.
 # Trace, trap14, and trap15 are always controlled by KDB.  KDB
 # code decides when to "pass on" these exceptions.
 #

 	set	 IC_CLR,0x0008     
 	set	 IC_CE,0x0004    
 	set	 IC_FREEZE,0x0002   
 	set	 IC_ENABLE,0x0001  
 	set	 IC_BURST,0x0010 
 	set	 DC_WA,0x2000
 	set	 DC_CLR,0x0800
 	set	 DC_CE,0x0400
 	set	 DC_FREEZE,0x0200
 	set	 DC_ENABLE,0x0100
 	set	 DC_BURST,0x1000


		set ILLVECTR,0x10


   	set	BUS,0x00000008
       	set	ADDRESS,0x0000000c
     	set	TRACE,0x00000024
 # debugging only -- used in place of trap15
      	set	TRAP13,0x000000b4
      	set	TRAP14,0x000000b8
      	set	TRAP15,0x000000bc

       		set	HIGHBUS,0xfffffffa
        	set	HIGHADDR,0xfffffff4
       		set	HIGHTRC,0xffffffd0
 # debugging only -- used in place of trap15
        	set	HIGHTR13,0xffffff46
        	set	HIGHTR14,0xffffff40
        	set	HIGHTR15,0xffffff3a

     		set	JMPOP,0x4ef9

 #
 # Addresses of mmu registers
 #

#define MMU_BASE_PADDR          (0x5F4000)
#define M68040  0x03

 	global _current_io_base
        set     _current_io_base, 0xfffffdf0

            	set	SUPER_SEGTAB,0x5F4000	
           	set	USER_SEGTAB,0x5F4004
     		set	PURGE,0x5f4008
       		set	MMUCNTL,0x5f400c
		set	MMUCNTLPHYS,0x5f400e
    		set	SMEN,0x02
    		set	UMEN,0x01

 #
 # kdb is the entry point called by the kernel.
 # kdbstart is the entry called by the bootstrap loader.
 #
	global _kdb,_kdbstart
	global _edata,_end
	global _kdb_loadpoint

_kdbstart:	
	mov.l	8(%sp),%a1

 #	movm.l	%d0-%d7/%a0-%a7,_kdb_kernelregs	#* save 2nd ld registers
 #	mov.w	%sr,_kdb_sr			#* save 2nd ld status register
 #	mov.l	(%sp),_kdb_pc			#* save 2nd ld program counter
 #
 # Determine if it is a 68010 or a 68020 processor.
 # The bootstrap pushed the processor type on the stack.
 # The stack frame pushed by the bootstrap is as follows:
 # 	 0(sp)	return address to bootstrap
 # 	 4(sp)	address of lowram
 # 	 8(sp)	physical loadpoint
 #	12(sp)	size of text in bytes
 # 	16(sp)	size of data in bytes
 # 	20(sp)	size of bss in bytes
 # 	24(sp)	processor type 0 = 68010, 1 = 68020
 #

#ifdef SDS
	mov.l	&_hpux_processor, %a0
	add.l	%a1,%a0
	mov.l	24(%sp),(%a0)
#endif /* SDS */

	mov.l   &_kdb_loadpoint,%a0     # get variable address
	add.l   %a1,%a0              	# add load point
	mov.l	%a1,(%a0)		# save for later

	bsr	_get_architecture

 # call relocate
	pea	kdbskip
	mov.l	&_dbg_addr,%a0
 	add.l	%a1,%a0
	mov.l	(%a0),-(%sp)

	mov.l	&_dbg_addr_present,%a0
 	add.l	%a1,%a0
	mov.l	&0,(%a0)
	mov.l	&0,-(%sp)

	mov.l	&_relocate,%a0
 	add.l	%a1,%a0
	jsr	(%a0)
kdbskip:
	mov.l   &0,_current_io_base	# mapper is off so logical == phys
	jsr	_kdb_purge
	bra.w	start_cont


_kdb:
	mov.l	%sp,kdb_temp
	cmp.l	kdb_temp,&_kdbstart
	blt.b	kl1
	cmp.l	kdb_temp,&_end
	blt.w	kdb1

kl1:	movm.l	%d0-%d7/%a0-%a7,_kdb_kernelregs	#* save hp-ux registers
	mov.w	(%sp)+,_kdb_sr			#* save hp-ux status register
	mov.l	(%sp)+,_kdb_pc			#* save hp-ux program counter
	mov.w	(%sp)+,_kdb_vecoffset		#* save hp-ux vector offset
	mov.l	%sp,_kdb_kernelregs+60		#* save proper stack pointer
	mov.w	&0x2700,%sr			#* disable everything

start_cont:
	
	bsr	_kdb_save_regs
	mov.l	&1,-(%sp)
	bsr	_save_vectors
	addq.l	&4,%sp
	tst.l	%d0
	bne	kdb1
kdb0:	
	mov.l	&_kdb_stack,%sp			#* give KDB a stack
 	mov.l	&_edata,%a0
 	mov.l	&_end,%d0
 	sub.l	&_edata,%d0
clrit:	
 	mov.b  &0,(%a0)+
 	subq.l	&1,%d0
 	cmp.l	%d0,&0
 	bne.b	clrit

	mov.l	&1,-(%sp)			#* first time flag = true
	jsr	_kdb_main			#* onto KDBland

kdb1: 	
	mov.l	&_kdb_stack,%sp			#* give KDB a stack
	mov.l	&0,-(%sp)			#* first time flag = false
	jsr	_kdb_main			#* onto KDBland


 ###############################################################################
 #
 ###############################################################################

 #
 #  This code is called when KDB is returning control
 #  back to hp-ux. 
 #

	global _kdb_cont,_kdb_trace

_kdb_cont:
	movm.l	%d0-%d7/%a0-%a7,_kdb_regs		#* save KDB registers

	bsr	_restore_vectors
	bsr	_kdb_restore_regs
	bsr	_update_time
	bsr	_kdb_purge

	movm.l	_kdb_kernelregs,%d0-%d7/%a0-%a7	#* restore hp-ux registers
	mov.w	_kdb_vecoffset,-(%sp)		#* push  proper stack frame
	mov.l	_kdb_pc,-(%sp)			#* so that a return can be
	mov.w	_kdb_sr,-(%sp)			#* made to hp-ux
	rte					#* return to HP-UX!

 ###############################################################################
 # kdb_bp()		
 #
 # Exception handler for KDB.  This code sorts out the type of exception from 
 # the vector offset word.  There are 3 exceptions which KDB handles (in 
 # addition to bus and address , see kdb_except()) :
 #	trace  -- single stepping
 #	trap14 -- shell calling kdb
 #	trap15 -- breakpoint
 ###############################################################################

	global _kdb_bp

_kdb_bp:	
	mov.w	&0x2700,%sr			#* disable everything
	mov.w	(%sp)+,_kdb_sr			#* save hp-ux status register
	mov.l	(%sp)+,_kdb_pc			#* save hp-ux program counter
	mov.w	(%sp)+,_kdb_vecoffset		#* save vector offset
	movm.l	%d0-%d7/%a0-%a7,_kdb_kernelregs	#* save hp-ux registers
	mov.w	_kdb_vecoffset,%d0		#* get vector offset
	and.l	&0x00000fff,%d0
	cmp.w	%d0,&0x0024			#* trace trap?
	bne.b	kdb_bp0				#* branch if not	
	mov.l	(%sp)+,_kdb_pc2			#* save away actual pc
	mov.l	&_kdb_kernelregs,%a0
	add.l	&4,0x3c(%a0)			#* adjust saved a7
	and.w	&0x0fff,_kdb_vecoffset		#* clear type field

kdb_bp0:	
	bsr	_kdb_save_regs
	mov.l	&0,-(%sp)
	bsr	_save_vectors
	addq.l	&4,%sp
	mov.w	_kdb_vecoffset,%d0		#* get vector offset
	and.l	&0x00000fff,%d0
	cmp.w	%d0,&TRACE			#* trace trap?
	beq.b	kdb2				#* if so then call trace
#ifdef KDBKDB
	cmp.w	%d0,&TRAP15			#* breakpoint?
#else
	cmp.w	%d0,&0x00b4			#* trap13?
#endif
	beq.b	kdb2				#* if not then branch
#ifndef KDBKDB
	cmp.w	%d0,&0x00b8			#* trap14?
	beq.w	kdb1				#* if so then enter kdb
#endif
	cmp.w	%d0,&0x007c			#* level 7 interrupt (nmi)?
	beq.w	kdb1				#* if so then enter kdb
	bra.w	_kdb_cont			#* otherwise ignore
kdb2:	movm.l	_kdb_regs,%d0-%d7/%a0-%a7	        #* restore KDB registers
	rts					#* return to where we left off

 ###############################################################################
 # kdb_except()		bus and address error handler
 ###############################################################################

	global	_kdb_except

_kdb_except:
	lea	_kdb_exception,%a0		#* address of save area
	mov.w	(%sp)+,(%a0)+			#* save sr
	mov.l	(%sp)+,(%a0)+			#* save pc
	mov.w	(%sp)+,%d0			#* get vector offset word
	mov.w	%d0,(%a0)+			#* save it away
	lsr.w	&8,%d0				#* get format field
	lsr.w	&4,%d0

	cmp.w	%d0,&0x7
	bne.b	not_access_err
	
	mov.l	&26,%d0
	bra	kdb3

not_access_err:
	
	cmp.w	%d0,&0xA
	bne.b	not_020_short
	
	mov.l	&12,%d0
	bra	kdb3

not_020_short:
	
	cmp.w	%d0,&0xB
	bne.b	not_020_long
	
	mov.l	&42,%d0
	bra	kdb3

not_020_long:
	
	cmp.w	%d0,&0x9
	bne.b	not_coproc
	
	mov.l	&6,%d0
	bra	kdb3

not_coproc:
	mov.l	&0,%d0

kdb3:	
	tst.l	%d0				#* enter loop
	beq.b	kdb4
	mov.w	(%sp)+,(%a0)+			#* save em
	subq.l	&1,%d0
	bra.b	kdb3				#* re enter kdb
kdb4:	
	mov.l	&_kdb_stack,%sp			#* reset KDB stack
	mov.w	_kdb_exception+6,%d0		#* get vector offset
	and.l	&0x00000fff,%d0			#* mask off mode
	mov.l	%d0,-(%sp)			#* push exception flag
	jsr	_kdb_main			#* onto KDBland


 ###############################################################################
 #
 ###############################################################################

	global _nprintf,_nscanf,_ngets
_nprintf:	jmp	_printf		#* jump table for kernel use
_nscanf:		jmp	_scanf
_ngets:		jmp	_gets

	global	_Kjmp
_Kjmp:
	mov.l	&_kdb_stack,%sp			#* give KDB a stack
	mov.l	&1,-(%sp)
	jsr	_DebugIt


 ###############################################################################
 #
 ###############################################################################
	global _set_space

_set_space:
	mov.l	4(%sp),%d0
	movc	%d0,%sfc
        rts
	
 #
 # fetch 1, 2, or 4 bytes from currently defined 
 # alternate address space
 # value = fetch(addr, length)
 # if data cannot be fetched then -1 is returned
 #

	global	_Ufetch		#leave it local for now
_Ufetch:
	mov.l	8(%sp),%d0			#* get length
	mov.l	4(%sp),%a0			#* get address
	mov.l	BUS,-(%sp)			#* save bus error vector
	mov.l	&fetch4,BUS			#* new vector
	cmp.l	%d0,&1				#* one byte?
	bne.b	fetch1				#* branch if not

	nop
	movs.b	(%a0),%d0
	nop
	bra.b	fetch3	
fetch1:
	cmp.l	%d0,&2				#* two bytes?
	bne.b	fetch2				#* branch if not

	nop
	movs.w	(%a0),%d0
	nop
	bra.b	fetch3	
fetch2:
	nop
	movs.l	(%a0),%d0				#* must be four bytes
	nop
fetch3:
	mov.l	(%sp)+,BUS			#* restore bus error vector	
	rts					#* return 

 # got bus error so return -1

fetch4:	
	and.w	&0xf000,6(%sp)
	cmp.w	6(%sp),&0x7000
	bne.b	not68040

	add.l	&0x3c,%sp			#* pop off 68040 bus error stack
	bra.b	fetch_done

not68040:
	and.w	&0xf000,6(%sp)
	cmp.w	6(%sp),&0x8000
	bne.b	not68010

	add.l	&58,%sp				#* pop off 68010 bus error stack
	bra.b	fetch_done

not68010:
	cmp.w	6(%sp),&0xa000
	bne.b	not68020_short

	add.l	&32,%sp				#* pop off 68020 short bus error
	bra.b	fetch_done

not68020_short:
	add.l	&92,%sp				#* pop off 68020 long bus error

fetch_done:
	mov.l	&0xffffffff,%d0	
	bra.b	fetch3


	global	_kdb_testr

_kdb_testr:
	mov.l	4(%sp),%a0			#* get address

	mov.l	%a7,_testr_sp			# save the stack pointer

	mov.l	_current_io_base,%d1
        beq.w   kdbtestr_nomap                  # branch if mapping is not on

	mov.l	BUS,_testr_bus			#* save bus error vector
	mov.l	&kdbtestr_buserr,BUS		#* new vector
	bra	kdbtestr_vecsaved

kdbtestr_nomap:
	mov.w	HIGHBUS,_testr_highbus		#* save hp-ux bus error
	mov.l	HIGHBUS+2,_testr_highbus+2	
	mov.w	&JMPOP,HIGHBUS			#* install KDB handler
	mov.l	&kdbtestr_buserr,HIGHBUS+2	# new vector

kdbtestr_vecsaved:

	nop
	tst.b	(%a0)
	nop

	mov.l	&1,%d0

kdbtestr_ret:
	mov.l	_current_io_base,%d1
        beq.w   kdbtestr_nomap1                 # branch if mapping is not on

	mov.l	_testr_bus,BUS			#* restore bus error vector	
	rts					#* return 

kdbtestr_nomap1:
	mov.w	_testr_highbus,HIGHBUS
	mov.l	_testr_highbus+2,HIGHBUS+2
	rts					#* return 

 # got bus error so return 0

kdbtestr_buserr:	
	mov.l	_testr_sp,%a7		# restore stack
	mov.l	&0,%d0	
	bra	kdbtestr_ret

 ###############################################################################
 #
 #     writeback(fault_address, data, size, function_code)
 #     int fault_address, data, size, function_code;
 #
 ###############################################################################

	global	_writeback

_writeback:
	mov.l	%d2,-(%sp)		# save register we will be using

	movc	%dfc,%d2 		# save destination function code reg
	mov.l	20(%sp),%d0		# get the function code
	movc	%d0,%dfc		# load fc for the address to test

	mov.l   8(%sp),%a0    		# get address
	mov.l   12(%sp),%d0     	# get data
	mov.l	16(%sp),%d1		# get the size
	bne.b	not_long		# branch if size != 0 (0 means long)

	nop
	movs.l  %d0,(%a0)       	# store the data
	nop				# empty pipe before the movs executes
	bra.b	wb_cleanup

not_long:
	subq	&1,%d1			# size = short?
	bne.b	not_byte
	
	nop
	movs.b  %d0,(%a0)       	# store the data
	nop				# empty pipe before the movs executes
	bra.b	wb_cleanup

not_byte:
	subq	&1,%d1			# size = long?
	bne.b	not_word

	nop
	movs.w  %d0,(%a0)       	# store the data
	nop				# empty pipe before the movs executes
	bra.b	wb_cleanup

not_word:
        mov.l   &wb_msg1,-(%sp)  	# panic on unknown size
        jsr     _Panic

wb_cleanup:
	movc	%d2,%dfc		# restore destination function code
	mov.l	(%sp)+,%d2		# restore d2
	movq	&0,%d0			# initialize success return value
	rts

wb_error: 
	movc	%d2,%dfc		# restore destination function code
	mov.l	(%sp)+,%d2		# restore d2
	mov.l	&-1,%d0			# return error
	rts

	# declare panic messages for writeback
	data
wb_msg1:
        byte    "_writeback: Unexpected size in writeback",0

	text

 ###############################################################################
 #
 #     writeback_line(fault_address, data, function_code)
 #     int fault_address;
 #     int *data;
 #     int function_code;
 #
 ###############################################################################

	global	_writeback_line

_writeback_line:

	# XXX check stack offsets
	movc	%dfc,%d1 		# save destination function code reg
	mov.l	12(%sp),%d0		# get the function code
	movc	%d0,%dfc		# load fc for the address to test

        mov.l   4(%sp),%a0      	# get address
        mov.l   8(%sp),%a1      	# get address of data
        mov.l   (%a1)+,%d0      	# get the data
	nop
        movs.l  %d0,(%a0)+      	# store the data
        nop                     	# empty pipe before the movs executes
        mov.l   (%a1)+,%d0      	# get the data
	nop
        movs.l  %d0,(%a0)+      	# store the data
        nop                     	# empty pipe before the movs executes
        mov.l   (%a1)+,%d0      	# get the data
	nop
        movs.l  %d0,(%a0)+      	# store the data
        nop                     	# empty pipe before the movs executes
        mov.l   (%a1)+,%d0      	# get the data
	nop
        movs.l  %d0,(%a0)       	# store the data
        nop                     	# empty pipe before the movs executes
	movc	%d1,%dfc		# restore destination function code
	movq	&0,%d0			# initialize success return value
        rts

wbl_err: 
	movc	%d1,%dfc		# restore destination function code
	mov.l	&-1,%d0			# return error
	rts


 ###############################################################################
 # get_architecture()
 #
 # Determine the processor type and MMU architecture.
 # loadpoint is in a1
 ###############################################################################
	global	_get_architecture

_get_architecture: 
	mov.l   &_kdb_processor,%a0     # get variable address
	add.l   %a1,%a0    	        # add load point

	mov.l   &_pmmu_exist,%a2     	# get variable address
	add.l   %a1,%a2    	        # add load point

	# Check bits 9 and 31 of the cacr to see if they are read/write.
        # If bit 31 is r/w then the processor is an mc68040.  If bit 9 
	# is r/w then the processor is an mc68030.

        # write a one to bits 9 and 31 of the cacr
        mov.l   &0x80000200,%d0
        long    0x4e7b0002 		# movec   %d0,cacr

       	# read cacr back and check bit 31
        long    0x4e7a0002 		# movec   cacr,%d0
        and.l   &0x80000000,%d0     	# is bit 31 set?
        bne.w   is68040      		# yes, then 68040

	########################################################################
	# We are running on either an mc68020 or an mc68030.  We will not do
	# common processor initialization here since we still have to dork with
	# the cacr to figure out the machine model.
	########################################################################

	# Bit 31 was not r/w.  Too bad, cause the 68040 is a hot processor.
	# Oh well, check bit 9 and see if we are on a 68030.

       	# read cacr back and check bit 9
        long    0x4e7a0002 	# movec   cacr,%d0
        and.l   &0x200,%d0     	# is bit 9 set?
        bne.w   is68030      	# yes, then 68030

	########################################################################
	# We are running on an mc68020 processor.
	########################################################################
	mov.l	&1,(%a0)	# save processor type, 1 means MC68020

	mov.l	&UMEN,MMUCNTL
	mov.l	MMUCNTL,%d0
	mov.l	&0,MMUCNTL
	and.l	&UMEN,%d0
	bne.b	no_pmmu

	# This box is using an MC68851 PMMU
	mov.l	&1,(%a2)
	bra	got_architecture
no_pmmu:

	# This box is using HP's memory mapped MMU
	mov.l	&0,(%a2)
	bra	got_architecture

is68030:
	########################################################################
	# We are running on an mc68030 processor.
	########################################################################
	mov.l	&2,(%a0)	# save processor type, 2 means MC68030

	# Set pmmu_exists to TRUE
	mov.l	&1,(%a2)
	bra	got_architecture

is68040:
	########################################################################
	# We are running on an mc68040 processor.
	########################################################################
        # disable caches
        mov.l   &0,%d0
        long    0x4e7b0002      # movec d0,cacr

	mov.l	&3,(%a0)	# save processor type, 3 means MC68040

        # Disable bus snooping
        mov.b   &7,MMU_BASE_PADDR+0x03  # set lower 3 bits of snoop ctl reg

	# flush all entries from both user and supervisory TLBs
	nop
	short	0xF518		# pflusha

	# invalidate all cache entries
	nop
	short	0xF4F8	# cpusha IC/DC

	# Set pmmu_exists to FALSE
	mov.l	&0,(%a2)

got_architecture:

        # disable caches
	clr.w	MMU_BASE_PADDR+0x0e
        mov.l   &0,%d0
        long    0x4e7b0002      # movec d0,cacr
	rts



 ###############################################################################
 # mmu_enabled()
 ###############################################################################

	global	_mmu_enabled

_mmu_enabled:

	# Based, on MMU architecture, determine if VM mapping is enabled.  
	# If it is, a zero (0) goes into _mapper_off.  If mapping is not 
	# yet enabled, a one (1).

        cmp.l   _kdb_processor,&3       # is it a 68040?
        bne     not_68040

        long    0x4E7A0003      	# movec TC,%d0
        and.l   &0x8000,%d0  	        # enabled?
	beq.b	mc68040_mmu_off
	mov.l	&0,_mapper_off		# let everyone know its on
	rts

mc68040_mmu_off:
	mov.l	&1,_mapper_off		# let everyone know its off
	rts

not_68040:
	tst.l	_pmmu_exist
	beq.b	hp_mmu

	subq	&4,%sp
	long	0xf0174200		# pmove	TC,(%sp)
	mov.l	(%sp)+,%d0
	and.l	&0x80000000,%d0		# enabled?
	beq.b	mc68030_mmu_off

	mov.l	&0,_mapper_off		# let everyone know its on
	rts

mc68030_mmu_off:
	mov.l	&1,_mapper_off		# let everyone know its off
	rts

hp_mmu:
	mov.l	&MMUCNTL,%a0
	add.l	_current_io_base,%a0
	mov.l	(%a0),%d0		# get mmu control register
	and.l	&SMEN,%d0		# system mapping enabled?
	beq.b	hp_mmu_off

	mov.l	&0,_mapper_off		# let everyone know its on
	rts

hp_mmu_off:
	mov.l	&1,_mapper_off		# let everyone know its off
	rts


 ###############################################################################
 # kdb_save_regs()
 ###############################################################################

	global	_kdb_save_regs

_kdb_save_regs:

	movc	%sfc,%d0		# save source function code reg
	mov.l	%d0,_kdb_fregs

	movc	%dfc,%d0		# save dest function code reg
	mov.l	%d0,_kdb_fregs+4

	movc 	%usp,%d0		# save user stack pointer
	mov.l	%d0,_kdb_usp

	rts

 ###############################################################################
 # kdb_restore_regs()
 ###############################################################################

	global	_kdb_restore_regs	

_kdb_restore_regs:
	
	mov.l	_kdb_fregs,%d0		# restore src function code reg
	movc 	%d0,%sfc

	mov.l	_kdb_fregs+4,%d0	# restore dest function code reg
	movc 	%d0,%dfc

	mov.l	_kdb_usp,%d0		# restore user stack pointer
	movc 	%d0,%usp

	rts

 ###############################################################################
 # Purge everything. TLBs, caches,...
 ###############################################################################
	global	_kdb_purge

_kdb_purge:
	cmp.l   _kdb_processor,&M68040      # have to treat the MC68040 special
	beq.w   kdb_purge_68040

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

kdb_purge_68040:
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

	bsr	_mmu_enabled

	mov.l	_mapper_off,%d0
        bne.w   kdbnomap1               # branch if mapping is not on

  	jsr	_WriteEnable			#* Allow mods to kernel text.
	jsr	_kdb_purge

	mov.l	BUS,_kdb_saved_bus		#* save hp-ux bus error
	mov.l	&_kdb_except,BUS		#* install KDB handler	
	mov.l	ADDRESS,_kdb_saved_addr		#* save hp-ux address error
	mov.l	&_kdb_except,ADDRESS		#* install KDB handler

	tst.l	4(%sp)
	beq	all_saved

	mov.l	TRAP14,_kdb_saved_trap14	#* save hp-ux trap14
	mov.l	&_kdb_bp,TRAP14			#* install KDB hander

#ifdef KDBKDB
	mov.l	TRAP15,_kdb_saved_trap15	#* save hp-ux trap15
	mov.l	&_kdb_bp,TRAP15			#* install KDB hander
#else
	mov.l	TRAP13,_kdb_saved_trap15	#* save hp-ux trap15
	mov.l	&_kdb_bp,TRAP13			#* install KDB hander
#endif

	mov.l	&0,%d0
	cmp.l	TRACE,&_kdb_bp
	beq.w	all_saved
	mov.l	TRACE,_kdb_saved_trace		#* save hp-ux trace trap
	mov.l	&_kdb_bp,TRACE			#* install KDB hander
	mov.l	&1,%d0
	bra.w	all_saved

kdbnomap1:
	mov.w	HIGHBUS,_kdb_highbus		#* save hp-ux bus error
	mov.l	HIGHBUS+2,_kdb_highbus+2	
	mov.w	&JMPOP,HIGHBUS			#* install KDB handler
	mov.l	&_kdb_except,HIGHBUS+2

	mov.w	HIGHADDR,_kdb_highaddr		#* save hp-ux address error
	mov.l	HIGHBUS+2,_kdb_highaddr+2
	mov.w	&JMPOP,HIGHADDR			#* install KDB handler
	mov.l	&_kdb_except,HIGHADDR+2

	tst.l	4(%sp)
	beq	all_saved

	mov.w	HIGHTR14,_kdb_hightr14		#* save hp-ux trap 14
	mov.l	HIGHTR14+2,_kdb_hightr14+2
	mov.w	&JMPOP,HIGHTR14			#* install KDB handler
	mov.l	&_kdb_bp,HIGHTR14+2

#ifdef KDBKDB
	mov.w	HIGHTR15,_kdb_hightr15		#* save hp-ux trap15
	mov.l	HIGHTR15+2,_kdb_hightr15+2
	mov.w	&JMPOP,HIGHTR15			#* install KDB handler
	mov.l	&_kdb_bp,HIGHTR15+2
#else
	mov.w	HIGHTR13,_kdb_hightr15		#* save hp-ux trap15
	mov.l	HIGHTR13+2,_kdb_hightr15+2
	mov.w	&JMPOP,HIGHTR13			#* install KDB handler
	mov.l	&_kdb_bp,HIGHTR13+2
#endif

	mov.w	HIGHTRC,_kdb_hightrc		#* save hp-ux trace trap
	mov.l	HIGHTRC+2,_kdb_hightrc+2
	mov.w	&JMPOP,HIGHTRC			#* install KDB handler
	mov.l	&_kdb_bp,HIGHTRC+2		#* install KDB hander
	mov.l	&0,%d0

all_saved:
	rts

 ###############################################################################
 # restore_vectors
 ###############################################################################

	global	_restore_vectors

_restore_vectors:

	bsr	_mmu_enabled
	mov.l	_mapper_off,%d0
        bne.w   mapoff	                # branch if mapping is not on

	mov.l	_kdb_saved_addr,ADDRESS	# restore hp-ux address handler
	mov.l	_kdb_saved_bus,BUS	# restore hp-ux bus handler
	mov.l	_kdb_saved_trace,TRACE	# restore hp-ux trace trap
	tst.l	_kdb_trace
	beq.b	do_writeprotect

	mov.l	&_kdb_bp,TRACE
	bra.b	do_writeprotect

mapoff:	
	mov.w	_kdb_highaddr,HIGHADDR
	mov.l	_kdb_highaddr+2,HIGHADDR+2
	mov.w	_kdb_highbus,HIGHBUS
	mov.l	_kdb_highbus+2,HIGHBUS+2
	rts

do_writeprotect:
  	jsr	_WriteProtect		# Disable mods to kernel text.
	rts

 ###############################################################################
 # update_time
 ###############################################################################

	global	_update_time

_update_time:

        set CLK_BASE,0x5f8000
        set TIMER2_OFF,9

	tst.l	_notime
	beq.b	kdb_notime

	lea	CLK_BASE,%a0
	add.l	_current_io_base,%a0
	movp.w	TIMER2_OFF(%a0),%d1	#* read tick count from clock
	mova.l	_kdb_tick_count,%a0	#* and set kernel tick count
	mov.w	%d1,(%a0)

kdb_notime:
	rts

 ###############################################################################
 # clear bss
 ###############################################################################

	global	_clear_bss

_clear_bss:

	mov.l	&_edata,%a0
	mov.l	&_end,%d0
	sub.l	&_edata,%d0
clr:	
	mov.b  &0,(%a0)+
	subq.l	&1,%d0
	cmp.l	%d0,&0
	bne.b	clr

	rts

 # these are the read, open, and close routines taken from the
 # secondary loader.

	global _open,_read,_close,_lseek,_opencdf
#ifdef SDS
_open:
	cmpi.l	_hpux_processor, &0xdeadbeef
	beq.b	sds_open
	mov.l	0xffff0802,%a0
	jmp	(%a0)
sds_open:
	mov.l	0xfff00002,%a0
	jmp	(%a0)
_close:
	cmpi.l	_hpux_processor, &0xdeadbeef
	beq.b	sds_close
	mov.l	0xffff0806,%a0
	jmp	(%a0)
sds_close:
	mov.l	0xfff00006,%a0
	jmp	(%a0)
_read:
	cmpi.l	_hpux_processor, &0xdeadbeef
	beq.b	sds_read
	mov.l	0xffff080a,%a0
	jmp	(%a0)
sds_read:
	mov.l	0xfff0000a,%a0
	jmp	(%a0)
_opencdf:
	cmpi.l	_hpux_processor, &0xdeadbeef
	beq.b	sds_opencdf
	mov.l	0xffff080e,%a0
	jmp	(%a0)
sds_opencdf:
	mov.l	0xfff0000e,%a0
	jmp	(%a0)
_lseek:
	cmpi.l	_hpux_processor, &0xdeadbeef
	beq.b	sds_lseek
	mov.l	0xffff0812,%a0
	jmp	(%a0)
sds_lseek:
	mov.l	0xfff00012,%a0
	jmp	(%a0)
#else /* not SDS */
_open:
	mov.l	0xffff0802,%a0
	jmp	(%a0)
_close:
	mov.l	0xffff0806,%a0
	jmp	(%a0)
_read:
	mov.l	0xffff080a,%a0
	jmp	(%a0)
_opencdf:
	mov.l	0xffff080e,%a0
	jmp	(%a0)
_lseek:
	mov.l	0xffff0812,%a0
	jmp	(%a0)
#endif /* else not SDS */

      	set	CRTMSG,0x150
	global	_crtmsg
_crtmsg:
	mov.l	4(%sp),%d0
	mov.l	8(%sp),%a0
	jsr	CRTMSG
	rts

#ifndef KFLPT
 # These are a bunch of stub routines for the floating point routines
 # that are pulled in by the loader

	global _afadd,_afdiv,_afmul,_afsub
	global _dtof,_ftod,_fcmp,_fix,_float,_fltoi,_fneg
_afadd:
_afdiv:
_afmul:
_afsub:
_dtof:
_ftod:
_fcmp:
_fix:
_float:
_fltoi:
_fneg:
	movq	&0,%d0
	movq	&0,%d1
	rts
#endif

 #
 # Various save areas for KDB
 #
	global	_mapper_off,_kdb_processor
	global	_kdb_kernelregs,_kdb_dregs,_kdb_aregs
	global	_kdb_sr,_kdb_pc,_kdb_vecoffset
	global	_kdb_saved_bus,_kdb_saved_addr
	global	_kdb_highbus,_kdb_highaddr
	global	_kdb_saved_trace,_kdb_saved_trap14,_kdb_saved_trap15
	global	_kdb_hightrc,_kdb_hightr14,_kdb_hightr15
	global	_kdb_stack,_kdb_tos,_kdb_usp,_kdb_regs
	global	_kdb_mregs,_kdb_fregs,_kdb_vbr
	global	_kdb_isp,_kdb_msp,_kdb_cacr,_kdb_caar
	global	_pmmu_exist, flag_68010
	global	_kdb_exception,_kdb_tick_count,_notime
	global	_dbg_addr_present,_dbg_addr,_vpc
	global	_eightbytes
	global	_testr_highbus,_testr_sp,_testr_bus
#ifdef SDS
	global	_hpux_processor
#endif /* SDS */

	data
flag_68010:		short	1
_dbg_addr_present:	long	1
#ifdef KDBKDB
_dbg_addr:		long	0x800000
#else
_dbg_addr:		long	0x880000
#endif
_notime:		long	0	#* flag for kdb using time or not
_kdb_tick_count:	long	0	#* location of kernels tick_count
kdb_temp:	long	0		#* temp location
_mapper_off:	long	0		#* mapper on/off flag
_kdb_processor:	long	0		#* 68010/68020 flag
#ifdef SDS
_hpux_processor:	long	0
#endif /* SDS */
_kdb_loadpoint:	long	0		#* for printing phys addrs
_pmmu_exist:	long	0		#* pmmu flag.
_testr_sp:	long	0
_testr_bus:	long	0
	lalign	4
_kdb_kernelregs:
_kdb_dregs:	space	4*(8)		#* hp-ux saved data registers
_kdb_aregs:	space	4*(8)		#* hp-ux saved address registers

_kdb_usp:	space	4*(1)		#* hp-ux saved user stack pointer

fill:		space	2*(1)
_kdb_sr:	space	2*(1)		#* hp-ux status register
	lalign	4
_vpc:					#* this is what cdb calls it
_kdb_pc:	space	4*(1)		#* hp-ux program counter
_kdb_pc2:	space	4*(1)		#* actual program counter (normal 6)
_kdb_vecoffset:	short	0		#* hp-ux vector offset

	lalign	4
_kdb_vbr:	space	4*(1)		#* vector base register save area
_kdb_isp:	space	4*(1)		#* interrupt stack pointer
_kdb_msp:	space	4*(1)		#* master stack pointer
_kdb_cacr:	space	4*(1)		#* cache control register
_kdb_caar:	space	4*(1)		#* cache address register
_kdb_fregs:	space	4*(2)		#* function code register save area	
_kdb_mregs:	space	4*(3)		#* mmu register save area

_kdb_saved_bus:	space	4*(1)		#* hp-ux bus error handler	
_kdb_saved_addr:	space	4*(1)		#* hp-ux address error handler	

_kdb_highbus:	space	2*(3)		#* hp-ux highram bus error handler	
_testr_highbus:	space	2*(3)		#* hp-ux highram bus error handler	
_kdb_highaddr:	space	2*(3)		#* hp-ux highram address error handler	

	lalign	4
_kdb_saved_trace:  space	4*(1)		#* hp-ux trace trap handler
_kdb_saved_trap14: space	4*(1)		#* hp-ux trap14 handler	
_kdb_saved_trap15: space	4*(1)		#* hp-ux trap15 handler	

_kdb_hightrc: 	  space	2*(3)		#* hp-ux highram trace trap handler
_kdb_hightr14: 	  space	2*(3)		#* hp-ux highram trap14 handler	
_kdb_hightr15: 	  space	2*(3)		#* hp-ux highram trap15 handler	

	lalign	4
_kdb_regs:	space	4*(16)		#* save area for KDB registers
_kdb_exception:	space	2*(46)		#* bus/address error save area
_eightbytes:	space	8
	bss
	lalign	4
_kdb_tos:	space	4*(3000)
_kdb_stack:	space	8


 ###############################################################################
 # Graphics transparent translation management
 ###############################################################################

	global _tt_window_on,_tt_window_off,_itt0,_dtt0

	data
_itt0:	long	0
_dtt0:	long	0
	text

 ###############################################################################
 # Enable graphics transparent translation segment
 ###############################################################################

_tt_window_on:

	# If the MMU is not enabled then do nothing
	bsr     _mmu_enabled
	mov.l   _mapper_off,%d0
	beq.w   enable_tt
	rts

enable_tt:

	# Branch to processor specific code
	mov.l	&_kdb_processor,%a0	# get processor variable address
	cmpi.l	(%a0),&M68040		# is it an MC68040
	beq	mc68040_tt_on		# branch if so

	# save the current tt control register values
	subq	&4,%sp			# allocate stack space
        long    0xf0170e00              # pmove TT1,(%sp)
	mov.l	(%sp),_itt0		# store the old value

	# 68030 r/w, supervisory only, cache inhibited
        mov.l   &0x00ff8543,%d0
	mov.l	%d0,(%sp)		# put it where pmove can get at it
        long    0xf0170c00              # pmove (%sp),TT1
	addq	&4,%sp			# pop the stack
	rts

mc68040_tt_on:

	# save the current tt control register values
	long	0x4e7a0004		# movec	%ITT0,%d0
	mov.l	%d0,_itt0
	long	0x4e7a0006		# movec	%DTT0,%d0
	mov.l	%d0,_dtt0

	# 68040 r/w, supervisory only, cache inhibited, serialized
        mov.l   &0x00ffa040,%d0
        long    0x4e7b0004      	# movec %d0,%ITT0
        long    0x4e7b0006      	# movec %d0,%DTT0
	rts

 ###############################################################################
 # Disable graphics transparent translation segment
 ###############################################################################

_tt_window_off:

	# If the MMU is not enabled then do nothing
	bsr     _mmu_enabled
	mov.l   _mapper_off,%d0
	beq.w   disable_tt
	rts

disable_tt:

	# Branch to processor specific code
	mov.l	&_kdb_processor,%a0	# get processor variable address
	cmpi.l	(%a0),&M68040		# is it an MC68040
	beq	mc68040_tt_off		# branch if so

	# set tt window for 68030
	mov.l	_itt0,-(%sp)
        long    0xf0170c00              # pmove (%sp),TT1
	addq	&4,%sp			# pop the stack
	rts

mc68040_tt_off:

	# restore saved current tt control register values
	mov.l	_itt0,%d0
	clr.l	%d0			# TURN IT OFF!!
        long    0x4e7b0004      	# movec %d0,%ITT0
	mov.l	_dtt0,%d0
	clr.l	%d0			# TURN IT OFF!!
        long    0x4e7b0006      	# movec %d0,%DTT0
	rts
