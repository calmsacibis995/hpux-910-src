 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/locore.s,v $
 # $Revision: 1.9.84.18 $       $Author: rpc $
 # $State: Exp $   	$Locker:  $
 # $Date: 94/10/11 07:52:57 $

 # HPUX_ID: @(#)locore.s	55.1		88/12/23 

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
#include "../machine/cpu.h"	/* Get CPU relative numbers. */

 # Start address of the U-area in kernel virtual space

		global _u

		set _u,KUAREABASE

 # XXX this define should go in proc.h and be printed by genassym.c
 #define S268040_FP  0x00040000  /* Flag for 68040 fp emulation */
		set S268040_FP,0x40000


 # Miscellaneous equates

   		set LOW,0x2000		# interrupt level 0
    		set HIGH,0x2600		# total disable
        	set HIGHRAM,0x1000000	# one more than the highest physical
  					# byte of ram
        	set LINE1111,0x0000002c	# address of line 1111 emulator vector
         	set HIGHPAGE,0xfffff000	# highest numbered physical ram page
   		set MMU_RES7,0x0080	# mmu control reg bit 7.

 # Equate boot rom variables into kernel logical data space.
 # Currently the following variables are defined:
 #	SYSNAME		(ten bytes)
 #	SYSFLAG		(one byte)
 # 	SYSFLAG2	(one byte)
 #	MSUS		(four bytes)
 #	NDRIVES		(one byte)
 #	FAREA		(four bytes)
	
	global	_sysname, _sysflags, _farea, _ndrives, _sysflags2, _msus

        	set	_sysname,	0xfffffdc2
         	set	_sysflags,	0xfffffed2
        	set	_farea,		0xfffffed4
        	set	_ndrives,	0xfffffed8
          	set	_sysflags2,	0xfffffeda
     		set	_msus,		0xfffffedc

	global	_current_io_base

	set	_current_io_base, 0xfffffdf0



 ###############################################################################
 # This attempts to explain that which is about to happen...
 #
 # Physical memory is divided into the following regions:
 #	physical addresses 0x000000 - 0x200000  rom space
 #	physical addresses 0x200000 - 0x800000	io space
 #	physical addresses 0x800000 - 0x900000  test space (used by debugger)
 #	physical addresses 0x900000 - 0x1000000 ram space
 #
 # The bootstrap has loaded the kernel into ram at LOWRAM.  Thus
 # ram currently looks like:
 #
 #	LOWRAM:		start of kernel code
 #			kernel data (aligned to a page boundary)
 #			kernel bss
 #
 # At this point the kernel maps (segment and page tables)
 # must be setup.  This involves sizing the kernel; i.e.
 # finding out how many physical memory pages are occupied by
 # the kernel.  
 #
 # The kernel segment table starts on the next page boundary after
 # the kernel bss.  The number of kernel page tables required is the 
 # sum of the kernel pages, io space, test space, and size of installed ram.
 # Physically it looks like:
 #
 #	LOWRAM:		start of kernel code
 #			kernel data (aligned to a page boundary)
 #			kernel bss
 #			Kernel segment table (aligned to a page boundary)
 #			Start of kernel page tables
 #
 # Logically it will look like:
 #
 #	0x000000	vectors
 #	0x000400	start of kernel code
 #			kernel data
 #			kernel bss
 #
 #	NOTE: the sum total of the kernel logical address space used
 #	      by the kernel for kernel code, data, etc. must be less
 #	      than 0x200000 in order to map io space logical = physical.
 #
 #	0x200000	start of io space (mapped logical = physical)
 #      0x7fffff	end of io space
 #	0x800000	start of test space (used by the debugger)
 #	0x8fffff	end of test space
 #	LOWRAM		start of ram space (mapped logical = physical)
 #	0xffffff	end of ram space
 #
 # This initial sequence runs unmapped in low memory.
 # The kernel entry point is 0x400, as the exception vectors in page 
 # zero must be skipped over.  The code is p-relative and will run in low 
 # memory until the kernel map is turned on.  
 # The last step is to switch the pc to a logical value, just before 
 # main is called.  This requires an explicit long jump to the 
 # next location.  Data space is not accessible (as a logical address)
 # until the kernel map is turned on.
 #
 # The bootstrap calls start directly.  The following arguments are
 # pushed onto the stack by the bootstrap:
 #	
 #	dbg_present -- the first word on the stack is either 0 or 1
 #	    depending upon whether kernel was called with a non-
 #	    relocating debugger or a relocating one; else it is the
 #	    return value of the call from the secondary loader and
 #	    is hence an addr in high ram (If the secondary loader
 #	    changes this convention it needs to pass a value other
 #	    than 0 or 1)
 #
 #          Note: The convention has been changed to allow a 2 to be
 #          passed, which indicates additional parameters have been
 #          passed by the secondary loader. Look at the code in
 #          init_kdb for further details
 #
 #	dbg_addr  -- if the kernel is invoked by a relocating debugger
 #	    then this value is the value of kdb''s entry point,
 #	    else it is meaningless
 #      loadpoint  -- physical address at which the kernel was loaded,
 #                    guaranteed to be on a page boundary by the bootstrap
 #      text size  -- size of kernel text segment in bytes (rounded to pagesize)
 #      data size  -- size of the kernel data segment in bytes
 #      bss size   -- size of the kernel bss segment in bytes
 #      processor  -- 0 = 68010, 1 = 68020
 #      nparam     -- Number of additional parameters. Only valid if
 #                    a 2 was passed in dbg_present. All parameters
 #                    past this point are only valid if a 2 was passed in
 #                    dbg_present.
 #      ramfsbegin -- Beginning a a ram file system image if one exists
 #      ramfssize  -- Size of ram file system image
 #
 # NOTE: The stack pointer was put at HIGHRAM by the bootstrap and
 #	thus needs no adjustment until the map is turned on.
 #
 ###############################################################################

	text
	global _kdb_nop
_kdb_nop:
	rts

	data
	global _dbg_addr,_dbg_present
	global _kdb_printf,_kdb_scanf,_kdb_gets
	global _ramfsbegin,_ramfssize
_dbg_addr:	long	0	#address of kdb entry point
_dbg_present:	long	0	#flag: 0 -- old style kdb (put at 0x88001e)
				#      1 -- new style kdb (use dbg_addr)
				#      2 -- additional parameters, no debugger
				#default -- no debugger
_kdb_printf:	long	_kdb_nop
_kdb_scanf:	long	_kdb_nop
_kdb_gets:	long	_kdb_nop
_ramfsbegin:    long    0
_ramfssize:     long    0
	text


	global _three_level_tables
	global _loadp
	global _indirect_ptes
	global _tt_region_addr,_first_paddr
	global _Buffermap_ptes
	global _kgenmap_ptes,_ktext_ptes
	global	_high_addr,_highpages_alloced
#ifdef SDS_BOOT
	global	_striped_boot
#endif /* SDS_BOOT */

	data

	lalign  4

_three_level_tables:    long    0
_loadp:    		long    0
_kgenmap_ptes:    	long    0
_ktext_ptes:    	long    0
_Buffermap_ptes:	long    0
_high_addr:		long	0
_highpages_alloced:	long	0
_tt_region_addr:	long	0
_first_paddr:		long	0
#ifdef SDS_BOOT
_striped_boot:		long	0
#endif /* SDS_BOOT */

	text


	global  _start, _init_proc0, _main
#ifdef DDB
	global  _ddb_start_engine, _ddb_vectors
#endif DDB

 # Remember that this code must be pc-relative until the map is enabled

_start:
 ###############################################################################
 ###############################################################################
 ## This is the entry point to the kernel!
 ###############################################################################
 ###############################################################################

	mov.w	&HIGH,%sr		# disable interrupts

	mov.l	8(%sp),-(%sp)		# push loadpoint
	bsr	get_architecture
	addq.l	&4,%sp			# pop parameter

	# enable the floating point coprocessor and caches
	clr.w	MMU_BASE_PADDR+0x0e
	mov.w	&MMU_CEN+MMU_IEN+MMU_FPE,MMU_BASE_PADDR+0x0e

#ifdef SDS_BOOT
	mov.l	&_striped_boot, %a2	# get variable address
	add.l	8(%sp),%a2		# add load point
	mov.l	24(%sp), (%a2)		# pickup value passed by loader
	mov.l	(%a2),%d0
	mov.l	&0xdeadbeef,%d1
	cmp.l	%d1,%d0
	beq     is_stripe
	clr.l	(%a2)
is_stripe:

#endif /* SDS_BOOT */
	mov.l	&_bootrom_f_area, %a2	# get variable address
	add.l	8(%sp),%a2		# add load point
	mov.l	_farea, %a3
	mov.l	(%a3)+, (%a2)+
	mov.w	(%a3), (%a2)

	# initialize the kernel debugger
	bsr	_init_kdb

	# register assignments:
	#	processor     -> %a0
	#	pmmu_exist    -> %a1
	#	Syssegtab     -> %a5
	#	Sysmap        -> %a4
	#

	mov.l	&_Syssegtab,%a5		# get segment table pointer
	add.l	8(%sp),%a5		# a5 points to kernel segment table


	########################################################################
 	# Zero kernel bss 
 	# The dbf instruction is NOT used as it has a 65536 limit (16 bits)
	########################################################################
	mov.l	&_edata,%a2
	add.l	8(%sp),%a2		# add load point
	mov.l	20(%sp),%d0		# get bss byte count
	bra.b	clr2
clr:	mov.b	&0,(%a2)+
	subq.l	&1,%d0
clr2:	bne.b	clr

	########################################################################
 	# Initialize kernel memory sizes
	########################################################################
	mov.l	20(%sp),-(%sp)		# push bss size
	mov.l	20(%sp),-(%sp)		# push data size
	mov.l	20(%sp),-(%sp)		# push text size
	mov.l	20(%sp),-(%sp)		# push loadpoint size
	bsr	_set_mem_sizes		# call setup routine
	add.l	&16,%sp			# pop the parameters

 	########################################################################
 	# Setup kernel mapping tables
 	########################################################################

	# map the kernel internal I/O segment
	mov.l	&PG_RW+PG_CI+PG_V,-(%sp) # push pte bits (r/w cache inhibited)
	mov.l	&INTIOPAGES*NBPG,-(%sp)	# push internal I/O size in bytes
	mov.l	&PHYS_IO_BASE,-(%sp)	# push starting phys addr
	mov.l	&LOGICAL_IO_BASE,-(%sp)	# push starting logical addr
	mov.l	24(%sp),-(%sp)		# push loadpoint
	bsr	_kmap_init		# call mapping routine
	add.l	&20,%sp			# pop the stack

	# set up mapping for all of physical RAM
        mov.l   8(%sp),-(%sp)           # push loadpoint
	bsr	_init_tt_window		# call routine
	addq.l	&4,%sp			# pop parameter

	# map the kernel text segment
	mov.l	&PG_RO+PG_V,-(%sp)	# push pte bits (read only text)
	# XXXX mov.l	&PG_RW+PG_V,-(%sp)	# push pte bits (read only text)
	mov.l	16(%sp),-(%sp)		# push kernel text size in bytes
	mov.l	16(%sp),-(%sp)		# push starting phys addr (loadpoint)
	clr.l	-(%sp)			# push starting logical addr
	mov.l	24(%sp),-(%sp)		# push loadpoint
	bsr	_kmap_init		# call mapping routine
	add.l	&20,%sp			# pop the stack

	# save the start of text ptes
	mov.l	&_ktext_ptes,%a2	# get variable address
	add.l	8(%sp),%a2		# add load point
	mov.l	&_high_addr,%a3		# get variable address
	add.l	8(%sp),%a3		# add load point
	mov.l	(%a3),(%a2)		# set ktext_ptes

	# map the kernel data+bss segment
	mov.l	12(%sp),%d7		# kernel text byte count
	mov.l	16(%sp),%d6             # get data size
        add.l   20(%sp),%d6             # add bss size to it
        mov.l   8(%sp),%d5              # get loadpoint
	mov.l	%d5,%d4			# copy loadpoint
	add.l	%d7,%d4			# starting phys addr

#ifdef notyet
	mov.l	&PG_RW+PG_V+PG_CB,-(%sp) # push pte bits (read/write data)
#else /* notyet, CB_KERNEL_DATA */
	mov.l	&PG_RW+PG_V,-(%sp)	# push pte bits (read/write data)
#endif /* else notyet, CB_KERNEL_DATA */
        mov.l   %d6,-(%sp)              # push byte count
        mov.l   %d4,-(%sp)              # push address
	mov.l	%d7,-(%sp)		# push starting logical addr (textsize)
	mov.l	%d5,-(%sp)		# push loadpoint
	bsr	_kmap_init		# call mapping routine
	add.l	&20,%sp			# pop the stack

	# do a zero based initialization of kgenmap_ptes and Buffermap_ptes

	# save the start address of the kernel general mapping  page tables
	mov.l	&_kgenmap_ptes,%a2	# get global variable
	add.l	8(%sp),%a2		# make it physical
	clr.l	(%a2)			# zero based start

	# save address of Buffermap ptes
	mov.l	&_Buffermap_ptes,%a2	# get global variable
	add.l	8(%sp),%a2		# make it physical
	clr.l	(%a2)			# save address


	mov.l	8(%sp),-(%sp)		# push loadpoint
	bsr	_init_srp		# set rootpointer
	bsr	_init_mmu		# turn on the mmu

	# fix to change physical pc values to logical in kdb and ddb
	jmp	_load_logical_pc
_load_logical_pc:

	addq.l	&4,%sp			# pop parameter

 ###############################################################################
 ###############################################################################
 ## The MMU is now enabled!
 ###############################################################################
 ###############################################################################

	movq	&1,%d1			# user space = 1 
	movc	%d1,%dfc		# destination function code
	movc	%d1,%sfc		# source function code

	global	_current_io_base
        mov.l   &_current_io_base,%a0   # get variable address
	mov.l	&LOG_IO_OFFSET,(%a0)	# let the debugger know where io is

 	jsr _call_kdb			# call kdb if its present

#ifdef DDB
	mov.l	_ddb_boot,%d1
	cmpi.l	%d1,&1			# ddb_boot == TRUE?
	bne.b	skipddb
	jsr	_ddb_start_engine	# talk to host
	jsr	_ddb_vectors		# setup ddb interrupt vectors
_ddb_trap:
	trap	&0x0e			# send control to ddb
skipddb:
#endif DDB

	mov.l   &_Kstack_toppage+NBPG,%sp # let us run on common top stack page
		
	mov.l   _first_paddr,%d0        # get first addr pfn for main

	mov.l	%d0,-(%sp)
	jsr     _init_proc0             # map u-area and kernel stack
	mov.l   &KSTACKADDR,%sp         # Move onto kernel stack

 # copy the signal trampoline code into the u_ structure

	mov.l	&sigcode,%a0
	mov.l	&_u+U_SIGCODE,%a1
	mov.l	&sigcode_end-sigcode,%d0
	subq.l	&1,%d0
sigloop:	mov.b	(%a0)+,(%a1)+
	dbf	%d0,sigloop

	jsr	_main			# initialize the world

	clr.w	-(%sp)			#vector offset (short mode)
	mov.l	&0,-(%sp)		#starting address of init
	clr.w	-(%sp)			#return to user mode
	rte				#off to user land!



 ###############################################################################
 # get_architecture(loadpoint)
 ###############################################################################

get_architecture:

#define GA_REGS (3 * 4)
#define GA_LP   (4 + GA_REGS)

	########################################################################
 	## The following code will determine what type of processor we
	## are running on and set the _processor variable appropriately.
	## At this time the machine model will also be determined.
	########################################################################

	# register assignments:
	#	processor     -> %a0
	#	pmmu_exist    -> %a1
	#	machine_model -> %a2

	mov.l   &_processor,%a0         # get processor variable address
	add.l   GA_LP(%sp),%a0          # add load point

	mov.l   &_pmmu_exist,%a1        # get pmmu_exist variable address
	add.l   GA_LP(%sp),%a1          # add load point
	clr.l	(%a1)			# initialize to "no pmmu"

	mov.l   &_machine_model,%a2     # get machine_model variable address
	add.l   GA_LP(%sp),%a2          # add load point

	# Check bits 9 and 31 of the cacr to see if they are read/write.
        # If bit 31 is r/w then the processor is an mc68040.  If bit 9 
	# is r/w then the processor is an mc68030.

        # write a one to bits 9 and 31 of the cacr
        mov.l   &0x80000200,%d0
        long    0x4e7b0002 		# movec   %d0,cacr

       	# read cacr back and check bit 31
        long    0x4e7a0002 		# movec   cacr,%d0

        # disable caches
        mov.l   &0,%d1
        long    0x4e7b1002      	# movec d1,cacr
	
        tst.l   %d0     		# is bit 31 set?
        bmi.w   is68040      		# yes, then 68040
	########################################################################
	# We are running on either an mc68020 or an mc68030.  We will not do
	# common processor initialization here since we still have to dork with
	# the cacr to figure out the machine model.
	########################################################################

	# Bit 31 was not r/w.  Too bad, cause the 68040 is a hot processor.
	# Oh well, check bit 9 and see if we are on a 68030.

        and.l   &0x200,%d0     	# is bit 9 set?
        bne.w   is68030      	# yes, then 68030

	########################################################################
	# We are running on an mc68020 processor.
	########################################################################
	mov.l	&1,(%a0)	# save processor type, 1 means MC68020

	# Determine the machine model for mc68020s.  The current possibilities
	# are S320, S350, and S330 (S318, S319,...).

	# Check to see if bit 0 of the WOPR status and control register is r/w.
	# If it is r/w then we are running on a model S350 or S320 using the HP
	# MMU.  If it is not r/w then this is a model S330 (S318, S319,...) 
	# using the MC68851 PMMU in which case we set the pmmu_exists var to 
	# TRUE.
	mov.w	&MMU_UMEN,MMU_BASE_PADDR+0x0e	# write bit 0
	mov.w	MMU_BASE_PADDR+0x0e,%d0		# read it back
	and.w	&MMU_UMEN,%d0			# is bit 0 set?
	bne.b	not_s330			# branch if so

	# We are running on a model S330 (S318, S319,...).
	# Set pmmu_exists to TRUE
	mov.l	&1,(%a1)

	# Set the machine_model variable to S330
	mov.w	&MACH_MODEL_330,(%a2)

	# We do any MC68851 PMMU specific initialization here (currently none).

	bra.w	got_architecture

not_s330:
	# We are running on a model S350 or a model S320.
	# Neither of these 2 models support indirect pte format.
	# Initialize _indirect_ptes for easy checking in C code.
	mov.l	%a2,-(%sp)		# save a2
	mov.l	&_indirect_ptes,%a2	# get var addr
	add.l   GA_LP+4(%sp),%a2        # add load point
	clr.l	(%a2)			# clear _indirect_ptes
	mov.l	(%sp)+,%a2		# restore a2

	# Check to see if bit 7 of the WOPR status and control register is r/w.
	# If it is r/w then we are running on a model S350 using the HP MMU.
	# If it is not r/w then this is a model S320.
	mov.w	&MMU_RES7,MMU_BASE_PADDR+0x0e
	mov.w	MMU_BASE_PADDR+0x0e,%d0
	and.l	&MMU_RES7,%d0
	beq.w	is_s320

	# We are running on a model S350.
	mov.w	&MACH_MODEL_350,(%a2) 		# we are a model S350!
	bra	got_architecture

is_s320:
	# We are running on a model S320.
	mov.w   &MACH_MODEL_320,(%a2)           # we are a model S320!
	bra	got_architecture

is68030:
	########################################################################
	# We are running on an mc68030 processor.
	########################################################################
	mov.l	&2,(%a0)	# save processor type, 2 means MC68030

	# Set pmmu_exists to TRUE
	mov.l	&1,(%a1)

	# Disable transparent translation.  These registers are
	# zeroed on powerup to the chip however they are not
	# cleared on a soft reboot.  Basic and Pascal enable
	# TT for the first gigabyte of the address space so
	# we must clear them here or a reboot from Basic or
	# Pascal will cause us to double bus fault when the
	# PMMU is enabled.
	#
	# Note: the PMMU on the MC68030 only supports control
	# alterable addressing modes, the obvious coding of
	# move immediate data is not supported by the hardware
	# so don''t try it.

	clr.l	-(%sp)
	long	0xf0170800	# pmove (%sp),TT0
	long	0xf0170C00	# pmove (%sp),TT1
	addq	&4,%sp

	# Determine if this is Wolverine, Weasel, Ferret, Otter, or Matchless.

	# see if bit 7 of the mmu control register is r/w
	# r/w means Wolverine (S370).
	mov.w	&MMU_RES7,MMU_BASE_PADDR+0x0e	# write bit 7
	mov.w	MMU_BASE_PADDR+0x0e,%d0		# read it back
	and.w	&MMU_RES7,%d0			# is it set?
	beq.w	check_coprocessor		# no, then branch

	# We were able to write a 1 to bit 7.  Is it really r/w or is
	# bit 7 just fixed at 1?
	mov.w	&0,MMU_BASE_PADDR+0x0e		# write a 0 to bit 7
	mov.w	MMU_BASE_PADDR+0x0e,%d0		# read it back
	and.w	&MMU_RES7,%d0			# is bit 7 still set?
	bne.w	not_s370			# yes, then branch

	# Bit 7 is r/w so we are on a Wolverine, model S370.
	mov.w	&MACH_MODEL_370,(%a2)
	bra.w	got_architecture

not_s370:
	# Bit 7 of the MMU control register is fixed at one.  This means that
	# we are either a model S332 or we are running on a model S340.  See 
	# if bit 0 of the mmu control register is r/w.   r/w means Ferret. 
	# Note: MMU_CONTROL was just zero''ed
	mov.w	&1,MMU_BASE_PADDR+0x0e		# write bit 0
	mov.w	MMU_BASE_PADDR+0x0e,%d0		# read it back
	and.w	&1,%d0				# is it still set?
	bne.w	not_s340			# branch if so

	# Bit 0 is fixed at 0 so we are on a Ferret, model S340.
	mov.w	&MACH_MODEL_340,(%a2)
	bra.w	got_architecture

not_s340:
	# Bit 0 is r/w so we are on an Otter, model S332.
	mov.w	&MACH_MODEL_332,(%a2)
	bra.w	got_architecture

check_coprocessor:

	# enable the floating point coprocessor for the following check
	mov.w	&0x0000,MMU_BASE_PADDR+0x0e
	mov.w	&MMU_CEN+MMU_IEN+MMU_FPE,MMU_BASE_PADDR+0x0e

	# Bit 7 of the MMU control register is fixed at zero.  This means that
	# we are either a model S360 or we are running on a Matchless mc68040
	# emulator board.  Check to see if a coprocessor responds at coprocessor
	# id = 5.  The Matchless emulators will respond to this but the model
	# S360 will not.
	mov.l	LINE1111,%a3		# save the f-line exception vector
	addq	&2,%a3			# skip past the jsr part of instuction
	mov.l	(%a3),-(%sp)		# save old address on the stack
	mov.l	&is_s360,%d0		# get our vector
	add.l   GA_LP+4(%sp),%d0        # add load point
	mov.l	%d0,(%a3)		# use our vector

	# This will cause an f-line excption on the S360
	# fmove   %fp0,%fp0 (coprocessor id == 5)
        short   0xfA00
        short   0x0000

	mov.l	(%sp)+,(%a3)	# restore exception vector

	# Since we didnt take the exception we must be running on a matchless
	# emulator board.  Check the processor board id register to figure
	# out the machine model.

	bra	check_id_register_030

is_s360:
	# We are running on a model S360.
	add.w	&12,%sp		# strip off the exception stack frame
	mov.l	(%sp)+,(%a3)	# restore exception vector
	mov.w	&MACH_MODEL_360,(%a2)
	bra	got_architecture
check_id_register_030:
	mov.w	&MACH_MODEL_UNKNOWN,(%a2)
	mov.w	MMU_BASE_PADDR+0x0e,%d0	# read the MMU control register
	lsr	&8,%d0			# shift id bits to the bottom
	cmpi.w	%d0,&7			# table index in bounds?
	bgt	got_architecture
	mov.w	machine_table_030(%pc,%d0*2),(%a2)
	bra	got_architecture

machine_table_030:
	short	MACH_MODEL_UNKNOWN
	short	MACH_MODEL_345
	short	MACH_MODEL_UNKNOWN
	short	MACH_MODEL_375
	short	MACH_MODEL_UNKNOWN
	short	MACH_MODEL_40T
	short	MACH_MODEL_UNKNOWN
	short	MACH_MODEL_40S

check_id_register:
	mov.w	&MACH_MODEL_UNKNOWN,(%a2)
	mov.w	MMU_BASE_PADDR+0x0e,%d0	# read the MMU control register
	lsr	&8,%d0			# shift id bits to the bottom
	cmpi.w	%d0,&13			# table index in bounds?
	bgt	got_architecture
	mov.w	machine_table(%pc,%d0*2),(%a2)
	bra	got_architecture

machine_table:
	short	MACH_MODEL_UNKNOWN
	short	MACH_MODEL_345
	short	MACH_MODEL_385
	short	MACH_MODEL_380
	short	MACH_MODEL_43T
	short	MACH_MODEL_42T
	short	MACH_MODEL_43S
	short	MACH_MODEL_42S
	short	MACH_MODEL_WOODY33
	short	MACH_MODEL_WOODY25
	short	MACH_MODEL_MACE33
	short	MACH_MODEL_MACE25
	short	MACH_MODEL_43S
	short	MACH_MODEL_42S

is68040:
	########################################################################
	# We are running on an mc68040 processor.
	########################################################################
        mov.l   &3,(%a0)        	# save processor type, 3 means MC68040

	# Disable transparent translation.  These registers are
	# zeroed on powerup to the chip however they are not
	# cleared on a soft reboot.  Basic and Pascal enable
	# TT for the first gigabyte of the address space so
	# we must clear them here or a reboot from Basic or
	# Pascal will cause us to double bus fault when the
	# PMMU is enabled.
	mov.l	&0,%a3
        long    0x4e7bB004      	# movec %a3,%ITT0
        long    0x4e7bB005      	# movec %a3,%ITT1
        long    0x4e7bB006      	# movec %a3,%DTT0
        long    0x4e7bB007      	# movec %a3,%DTT1

	# Disable bus snooping
	mov.b	&7,MMU_BASE_PADDR+0x03	# set lower 3 bits of snoop ctl reg

 #######################################################################
 # Begin xxD43B Floating Point Workaround
 #
 # We need to know the version number of the fsave state frame, so that
 # we can determine if we are on a 'bad' chip or not.  Any D43B chips
 # will need this code.
 #######################################################################
	data
	global	_fsave_version
_fsave_version:	long	0
	text

	link	%a6,&0
	fmove.s	&0f1.0,%fp1		# Needed to get idle state frame
	move.l	&_fsave_version,%a3
	add.l   GA_LP+4(%sp),%a3        # add load point
 	fsave	-(%sp)
	clr.l	%d0
	move.b	(%sp),%d0
	move.l	%d0,(%a3)		# Store Version ID
	clr.l	-(%sp)			# Reset FPU
	frestore (%sp)+
	unlk	%a6
 #######################################################################
 # End xxD43B Floating Point Workaround
 #######################################################################

        # purge caches
        short   0xF4D8          # cinva IC/DC
	and.w	&0xfffb,CPU_STATUS_PADDR 
	or.w	&0x0004,CPU_STATUS_PADDR 

        # disable em
        clr.l   %d0
        long    0x4e7b0002      # movec d0,cacr

        # set machine_model variable for mc68040 machines
	bra check_id_register

got_architecture:
 ###############################################################################
 # 
 # The HP memory mapped MMU on the S320 and S350 can only do  a  two
 # level  tablewalk.   The  Motorola  MMUs have a programmable table
 # depth, however, for simplicity and consistency with HP''s MMU,  we
 # program these MMUs for a two level tablewalk.
 # 
 # The MMU on the MC68040  chip  does  not  support  a  programmable
 # translation  table  depth  and  is  only capable of walking three
 # level tables.  Because of this the series 300 family now requires
 # that  the  system  software  be able to handle both two and three
 # level translation tables.
 # 
 # We have completed tablewalk  independent  table  setup.   we  now
 # branch to tablewalk dependent table setup code.   For purposes of
 # testing we may do three level table walks on the MC68030.
 # 
 ###############################################################################

	# initialize _three_level_tables for easy checking in C code
	mov.l	&_three_level_tables,%a2	# get var addr
	add.l   GA_LP(%sp),%a2          # add load point

	# Branch the three level tablewalk specific code
        cmpi.l  (%a0),&3                # is this an MC68040?
        beq     three_lev_tables        # branch if so

	# Branch the three level tablewalk specific code
        tst.l	(%a1)                   # is this a Motorola MMU?
        bne	three_lev_tables        # branch if so

	clr.l	(%a2)			# _three_level_tables = 0
	bra.b	tabledepth_known

three_lev_tables:
	mov.l	&1,(%a2)		# _three_level_tables = 1

tabledepth_known:

	# register assignments:
	#	processor     -> %a0
	#	pmmu_exist    -> %a1
	#
	# %a2 is now available since it was holding the machine model
	# variable address which has now been set.

	# Skip 680[23]0 initialization if running on 68040
        cmpi.l  (%a0),&3                # is this an MC68040?
        beq     mc68040_start_cont      # branch if so

	# disable caches
	clr.l	%d0
	long	0x4e7b0002 		# movec	d0,cacr

	# fall into processor independent code

mc68040_start_cont:

	rts

 ###############################################################################
 # _set_mem_sizes(loadpoint, testsize, datasize, bsssize)
 #
 # setup memory sizes
 #
 ###############################################################################

	global	_set_mem_sizes,_maxmem,_physmem,_freemem,_physmembase
	global	_dos_mem_byte,_dos_mem_start

_set_mem_sizes:

        movm.l  %d0-%d3/%a0-%a2,-(%sp)  # save registers

#define	SMS_NREGS	7
#define SMS_REGS_SPACE	(4  * SMS_NREGS)
#define SMS_LP		(4  + SMS_REGS_SPACE)
#define SMS_TS		(8  + SMS_REGS_SPACE)
#define SMS_DS		(12 + SMS_REGS_SPACE)
#define SMS_BS		(16 + SMS_REGS_SPACE)

	# save loadpoint for later use
	mov.l	&_loadp,%a0		# get variable address
	add.l	SMS_LP(%sp),%a0		# add loadpoint
	mov.l	SMS_LP(%sp),(%a0)	# save loadpoint

	# compute memory size
	mov.l	&HIGHPAGE,%d0		# highest page
	and.l	&~(NBPG-1),%d0		# to next page boundary
	mov.l	&PGSHIFT,%d1		# get shift value
	lsr.l	%d1,%d0			# make it a page count
	mov.l	SMS_LP(%sp),%d2		# get loadpoint
	lsr.l	%d1,%d2			# make it a NBPG page count
	sub.l	%d2,%d0			# get page count

	# initialize physmembase
	mov.l	&_physmembase,%a0	# get variable address
	add.l	SMS_LP(%sp),%a0		# add loadpoint
	mov.l	%d2,(%a0)		# initialize memory offset

	# initialize maxmem
	mov.l	&_maxmem,%a0		# get variable address
	add.l	SMS_LP(%sp),%a0		# add loadpoint
	mov.l	%d0,(%a0)		# initialize maxmem

	# initialize physmem
	mov.l	&_physmem,%a1		# get variable address
	add.l	SMS_LP(%sp),%a1		# add loadpoint
	mov.l	%d0,(%a1)		# initialize physmem

	# see if we need to reserve memory for the dos card
	mov.l	&_dos_mem_byte,%a2	# get variable address
	add.l	SMS_LP(%sp),%a2		# add loadpoint
	mov.l	(%a2),%d1		# get value
	beq.b	no_dos_mem		# branch if zero
	
	# calculate the number of pages needed
	add.l	&NBPG-1,%d1		# make d1
	mov.l	&PGSHIFT,%d3		# a rounded
	lsr.l	%d3,%d1			# page count

	# allocate the space
	sub.l	%d1,(%a0)		# decrement maxmem

	# initialize dos_mem_start
	mov.l	(%a0),%d1		# get maxmem
	add.l	%d2,%d1			# add physmembase
	mov.l	&PGSHIFT,%d3		# make it
	lsl.l	%d3,%d1			# a byte count
	mov.l	&_dos_mem_start,%a2	# get variable address
	add.l	SMS_LP(%sp),%a2		# add loadpoint
	mov.l	%d1,(%a2)		# set dos_mem_start

no_dos_mem:

	# save address of reserved highpages area
	mov.l	(%a0),%d1		# get maxmem
	add.l	%d2,%d1			# add physmembase
	mov.l	&PGSHIFT,%d3		# make it
	lsl.l	%d3,%d1			# a byte count
	mov.l	&_high_addr,%a2		# get global variable
	add.l	SMS_LP(%sp),%a2		# make it physical
	mov.l	%d1,(%a2)		# set high_addr

	# initialize freemem
	mov.l	&_freemem,%a0		# get variable address
	add.l	SMS_LP(%sp),%a0		# add loadpoint
	mov.l	%d0,(%a0)		# initialize freemem

	# calculate first byte address past the kernel bss segment 
	# and save it in the kernel variable first_paddr
        mov.l   SMS_LP(%sp),%d0         # get loadpoint
        add.l   SMS_TS(%sp),%d0         # add in text size (already rounded)
        add.l   SMS_DS(%sp),%d0         # add in data size
        add.l   SMS_BS(%sp),%d0         # add in bss size
        add.l   &NBPG-1,%d0             # round to nearest page
        and.l   &PG_FRAME,%d0           # mask out frame number
        mov.l   &PGSHIFT,%d1		# shift it to the bottom
        lsr.l   %d1,%d0			# d0 has first addr
	mov.l	&_first_paddr,%a2	# get global variable
	add.l	SMS_LP(%sp),%a2		# make it physical
	mov.l	%d0,(%a2)		# save first addr

        movm.l  (%sp)+,%d0-%d3/%a0-%a2  # restore registers
	rts

 ###############################################################################
 # _init_kdb()
 ###############################################################################

    # This code also checks for the "additional parameters" convention.
    # This convention is currently only used for ram filesystem booting.
    # This code currently assumes that kdb and "additional parameters"
    # are mutually exclusive. The code can be changed to support another
    # dbg_present value of "3" which would indicate a kdb that under-
    # stood the "additional parameters" convention.

	global	_init_kdb

_init_kdb:

        mov.l   %a0,-(%sp)  		# save register

	mov.l	&_dbg_present,%a0
	add.l	16(%sp),%a0
	mov.l	8(%sp),(%a0)      	# save debugger flag
	cmp.l	(%a0),&1
	beq.w   kdbpresent

	cmp.l   (%a0),&2                # Check to see if there are
					# additional parameters
	bne.w   kdbdone

	# Currently we only support 2 additional parametes
	# for ram fs booting. Therefore we don't need to check
	# the number of additional parameters (which would be used
	# for future additions), since we will always have at least
	# two if the "additional parameters" flag is set. This code
	# will need to change once additional "additional parameters"
	# are added.

	mov.l   &_ramfsbegin,%a0
	add.l	16(%sp),%a0
	mov.l   40(%sp),(%a0)           # save ram fs begin address

	mov.l   &_ramfssize,%a0
	add.l	16(%sp),%a0
	mov.l   44(%sp),(%a0)          # save ram fs size

	bra.w   kdbdone

kdbpresent:
	mov.l	&_dbg_addr,%a0
	add.l	16(%sp),%a0
	mov.l	12(%sp),(%a0)      	# save debugger addr

	mov.l	&_kdb_printf,%a0
	add.l	16(%sp),%a0
	mov.l	44(%sp),(%a0)      	# save debugger printf addr

	mov.l	&_kdb_scanf,%a0
	add.l	16(%sp),%a0
	mov.l	48(%sp),(%a0)      	# save debugger scanf addr

	mov.l	&_kdb_gets,%a0
	add.l	16(%sp),%a0
	mov.l	52(%sp),(%a0)      	# save debugger gets addr

kdbdone:
        mov.l  (%sp)+,%a0 		# restore register
	rts

 ###############################################################################
 # _init_tt_window(loadpoint)
 ###############################################################################

	global _init_tt_window

_init_tt_window:

        movm.l  %d0-%d1/%a0-%a1,-(%sp)  # save registers

#define TT_REGS	(4 * 4)
#define TT_LP	(4 + TT_REGS)

	########################################################################
	# Enable transparent translation segments
	########################################################################

	mov.l	&_processor,%a0		# get processor variable address
	add.l	TT_LP(%sp),%a0		# make it physical
	cmpi.l	(%a0),&M68020		# is it a 68020?
	bne 	have_tt_hw	        # branch if not

	# calculate the physical starting address of ram
        mov.l   0xfffffdce,%d0      	# bootrom loram value
	and.l	&0xfff00000,%d0		# increase to megabyte boundry

        mov.l   &_tt_region_addr,%a1    # get kernel logical address
        add.l   TT_LP(%sp),%a1          # make it a physical address
        mov.l   %d0,(%a1)               # save start addr of ram segment

	# calculate the number of bytes of ram
	clr.l	%d1
	sub.l	%d0,%d1			# number of bytes of ram

	# map all of ram logical = physical
	mov.l	&PG_RW+PG_CI+PG_V,-(%sp) # push pte bits (r/w cache inhibited)
	mov.l	%d1,-(%sp)		# push internal I/O size in bytes
	mov.l	%d0,-(%sp)		# push starting phys addr
	mov.l	%d0,-(%sp)		# push starting logical addr
	mov.l	TT_LP+16(%sp),-(%sp)	# push loadpoint
	bsr	_kmap_init		# call mapping routine
	add.l	&20,%sp			# pop the stack

	bra	tt_done

have_tt_hw:

	cmpi.l	(%a0),&M68040		# is it an MC68040
	beq	mc68040_tt		# branch if so

        # Enable supervisor space, read/write, cacheable, write back,
        # transparent translation for all of physical RAM.
        mov.l   TT_LP(%sp),%d0          # loadpoint
        mov.l   &0xff000000,%d1         # 16 meg real RAM starting address

mc68030_tt_set0:
        and.l   %d1,%d0
        cmp.l   %d1,%d0
        beq     mc68030_tt_set1

        lsl.l   &1,%d1                  # double amount of RAM
        bra     mc68030_tt_set0

mc68030_tt_set1:                        # d0 has address of tt start
        mov.l   &_tt_region_addr,%a1    # get kernel logical address
        add.l   TT_LP(%sp),%a1          # make it a physical address
        mov.l   %d0,(%a1)               # save start addr of tt segment

        not.l   %d0
        lsr.l   &8,%d0
	clr.w	%d0
	# cache inhibit tt window
        # or.l    &0xff008143,%d0       # set the logical address base
        or.l    &0xff008543,%d0         # set the logical address base (CI)
	mov.l	%d0,-(%sp)		# put it where pmove can get at it
        long    0xf0170800              # pmove (%sp),TT0
	addq	&4,%sp			# pop the stack
	bra	tt_done

mc68040_tt:

        # Enable supervisor space, read/write, cacheable, write trhough,
        # transparent translation for all of physical RAM.
        mov.l   TT_LP(%sp),%d0          # loadpoint
        mov.l   &0xff000000,%d1         # 16 meg real RAM starting address

mc68040_tt_set0:
        and.l   %d1,%d0
        cmp.l   %d1,%d0
        beq     mc68040_tt_set1

        lsl.l   &1,%d1                  # double amount of RAM
        bra     mc68040_tt_set0

mc68040_tt_set1:                        # d0 has address of tt start
        mov.l   &_tt_region_addr,%a1    # get kernel logical address
        add.l   TT_LP(%sp),%a1          # make it a physical address
        mov.l   %d0,(%a1)               # save start addr of tt segment

        not.l   %d0
        lsr.l   &8,%d0
	clr.w	%d0

	# There are many choices, pick one!
#ifdef notyet
        or.l    &0xff00a020,%d0       	# cachable, copyback
#else /* notyet, CB_TTW */
        or.l    &0xff00a000,%d0         # cachable, write through
#endif /* else notyet, CB_TTW */
        # or.l    &0xff00a040,%d0       # cache inhibited, serialized
        # or.l    &0xff00a060,%d0       # cache inhibited, not serialized

        long    0x4e7b0005      	# movec %d0,%ITT1
        long    0x4e7b0007      	# movec %d0,%DTT1

tt_done:
        movm.l  (%sp)+,%d0-%d1/%a0-%a1  # restore registers
	rts

 ###############################################################################
 # _kmap_init(loadpoint, laddr, paddr, nbytes, pte_bits) 
 ###############################################################################

	global _kmap_init

_kmap_init:

        movm.l  %d1-%d6/%a0-%a4,-(%sp)  # save registers

#define KM_REGS	44
#define KM_LP	(4 + KM_REGS)
#define KM_LA	(8 + KM_REGS)
#define KM_PA	(12 + KM_REGS)
#define KM_NB	(16 + KM_REGS)
#define KM_PB	(20 + KM_REGS)

 	########################################################################
 	#
 	# The HP memory mapped MMU on the S320 and S350 can only do  a  two
 	# level  tablewalk.   The  Motorola  MMUs have a programmable table
 	# depth, however, for simplicity and consistency with HP''s MMU,  we
 	# program these MMUs for a two level tablewalk.
 	#
 	# The MMU on the MC68040  chip  does  not  support  a  programmable
 	# translation  table  depth  and  is  only capable of walking three
 	# level tables.  Because of this the series 300 family now requires
 	# that  the  system  software  be able to handle both two and three
 	# level translation tables.
 	#
 	# We have completed tablewalk  independent  table  setup.   we  now
 	# branch to tablewalk dependent table setup code.   For purposes of
 	# testing we may do three level table walks on the MC68030.
 	#
 	########################################################################

        # Branch to three level tablewalk specific code if necessary
	mov.l &_three_level_tables,%a0 	# get table depth variable
        add.l KM_LP(%sp),%a0		# make it physical
        tst.l (%a0)	           	# three level tables?
        bne   three_lev_init		# branch if so

	########################################################################
	# Two level table initialization
	########################################################################

	# calculate the starting segment table index
	mov.l	KM_LA(%sp),%d0		# logical start address
	and.l   &SG_IMASK,%d0		# mask off segent table index bits
        mov.l   &SG_ISHIFT,%d1
        lsr.l   %d1,%d0                 # d0 has start segment table index

	# calculate the ending segment table index
	mov.l	KM_LA(%sp),%d1		# logical start address
	add.l	KM_NB(%sp),%d1          # add the byte count
	subq.l	&1,%d1			# ending logical address
	and.l   &SG_IMASK,%d1		# mask off segent table index bits
        mov.l   &SG_ISHIFT,%d2
        lsr.l   %d2,%d1                 # d1 has end segment table index

	# calculate the number of segment table entries we will visit
	sub.l	%d0,%d1			# end index - start index
	addq.l	&1,%d1			# d1 has visited ste count
	mov.l	%d1,%d2			# d2 has allocate ste count

	# calculate the segment table offset
	lsl.l	&2,%d0			# d0 has segment table offset

	# get a pointer to the appropriate segment table entry
	mov.l   &_Syssegtab,%a0         # get logical segment table pointer
        add.l   KM_LP(%sp),%a0		# make it physical
        add.l   %d0,%a0                 # add segment offset
	mov.l   %a0,%a3                 # save starting ste pointer
	clr.l   %d4                  	# assume its not valid
        tst.l   (%a0)			# is page table allocated?
	beq.b	pagetable_not_allocated	# skip if not valid

	# The segment table entry was already set.  We must be continuing a
	# mapping in a partially used page table.  Just skip past the valid
	# entry.
	mov.l   &1,%d4                  # remember that it was valid
	mov.l   (%a0),%d5               # get ste
	and.l   &SG_FRAME,%d5           # mask out physical page frame
	mov.l   %d5,%a2                 # save addr of page table
	add.l   &4,%a0                  # bump to next ste
	subq.l	&1,%d2			# need one less
	ble.w	skip_pt_allocation

pagetable_not_allocated:

	# get a pointer to the start of the page tables
	# allocate page table space from highram
	mov.l	&_high_addr,%a1		# get address of variable
	add.l	KM_LP(%sp),%a1		# make it physical
	# alloc page count already in d2 XXX mov.l	%d1,%d2	# get page count
	mov.l	&12,%d3			# make it
	lsl.l	%d3,%d2			# a byte count
	sub.l	%d2,(%a1)		# allocate space
	mov.l	(%a1),%d2		# get address

	# keep track of how many pages we have taken
	mov.l	&_highpages_alloced,%a1	# get address of variable
        add.l   KM_LP(%sp),%a1		# make it physical
	add.l	%d1,(%a1)		# bump by page count

	tst.l	%d4			# did we skip one?
	bne.b	dont_update_addr	# branch if so

	mov.l	%d2,%a2			# a2 has first page table address

dont_update_addr:
	
	# bump the first addr past the newly allocated page tables
	add.l	%d1,(%a1)		# bump first addr pfn 

	# zero the allocated space
	mov.l	%d2,%a1			# get the address
	mov.l	%d1,%d4			# get page count
	mov.l	&10,%d3			# make it a long word count
	lsl.l	%d3,%d4
	bra.b	clear_2lev_pt

next_2lev_pt:
	clr.l	(%a1)+			# clear the long
	subq.l	&1,%d4			# decrement byte count
clear_2lev_pt:
	bne.b	next_2lev_pt		# if more keep going

got_2lev_pt:

	# construct a prototype ste
        and.l   &SG_FRAME,%d2           # get physical page frame number
        or.l    &SG_RW+SG_V,%d2         # d2 has proto ste

	# load the segment table entries
	bra.b load_2level_ste

next_2level_ste:
	# store the segment table entry
        mov.l   %d2,(%a0)+              # store new segment table entry
	add.l	&NBPG,%d2		# bump to next page table
load_2level_ste:
	dbf	%d1,next_2level_ste


skip_pt_allocation:
        tst.l   KM_PB(%sp)   		# should we load the ptes?
	beq	kmap_done		# branch if not

	# calculate the number of ptes required
	mov.l   KM_NB(%sp),%d0          # get the byte count
	add.l   &NBPG-1,%d0             # we need one pte per NBPG bytes
	mov.l   &12,%d1                 # round up
	lsr.l   %d1,%d0                 # d0 is the pte count

        # calculate the page table offset
	mov.l	KM_LA(%sp),%d1		# logical start address
        and.l   &SG_PMASK,%d1           # mask off page table index
        mov.l   &SG_PSHIFT-LPTESIZE,%d2 # get shift value in d2
        lsr.l   %d2,%d1                 # d1 has page table offset

	# construct a prototype pte
        mov.l   KM_PA(%sp),%d2		# physical start address
        and.l   &PG_FRAME,%d2		# mask off page frame number
        or.l    KM_PB(%sp),%d2   	# whatever bits the caller wanted
        or.l    &PG_V,%d2         	# and make it valid

	# get a pointer to the first pte
	add.l	%d1,%a2			# pointer to ptes

	# load the page table entries
	bra.b	load_2level_pte

next_2level_pte:
        mov.l   %d2,(%a2)+              # store new segment table entry
	add.l	&PG_INCR,%d2		# bump to next page table

	mov.l	%a2,%d4
	and.l	&0xfff,%d4		# did we cross a page table boundry?
	bne.b	load_2level_pte

	# get a pointer to the next pte
	add.l	&4,%a3			# bump to next ste
	mov.l	(%a3),%d3		# get ste
        and.l   &SG_FRAME,%d3           # mask out physical page frame
	mov.l	%d3,%a2			# save addr of page table
load_2level_pte:
	dbf	%d0,next_2level_pte

	bra	kmap_done		# all done


	########################################################################
	# Three level table initialization
	########################################################################

three_lev_init:

	# calculate the starting segment table index
	mov.l	KM_LA(%sp),%d0		# logical start address
	and.l   &SG3_IMASK,%d0		# mask off segent table index bits
        mov.l   &SG3_ISHIFT,%d1
        lsr.l   %d1,%d0                 # d0 has start segment table index

	# calculate the ending segment table index
	mov.l	KM_LA(%sp),%d1		# logical start address
	add.l	KM_NB(%sp),%d1          # add the byte count
	subq.l	&1,%d1			# ending logical address
	and.l   &SG3_IMASK,%d1		# mask off segent table index bits
        mov.l   &SG3_ISHIFT,%d2
        lsr.l   %d2,%d1                 # d1 has end segment table index

	# calculate the number of segment table entries we will visit
	sub.l	%d0,%d1			# end index - start index
	addq.l	&1,%d1			# d1 has visited ste count
	mov.l	%d1,%d2			# d2 has allocate ste count

	# calculate the segment table offset
	lsl.l	&2,%d0			# d0 has segment table offset

	# get a pointer to the appropriate segment table entry
	mov.l   &_Syssegtab,%a0         # get logical segment table pointer
        add.l   KM_LP(%sp),%a0		# make it physical
        add.l   %d0,%a0                 # add segment offset
	mov.l   %a0,%a4                 # save starting ste pointer
	clr.l	%d4			# assume its not valid
        tst.l   (%a0)			# is block table allocated?
	beq.b	blktable_not_allocated	# skip if not valid

	# The segment table entry was already set.  We must be continuing a
	# mapping in a partially used block table.  Just skip past the valid
	# entry.
	mov.l   &1,%d4                  # remember that it was valid
	mov.l   (%a0),%d5               # get ste
	and.l   &SG3_FRAME,%d5          # mask out physical page frame
	mov.l   %d5,%a2                 # save addr of block table
	add.l   &4,%a0                  # bump to next ste
	subq.l	&1,%d2			# need one less
	ble.w	skip_block_allocation
	
blktable_not_allocated:
	#
	# block table allocation
	# ----------------------
	#
	# get a pointer to the start of the block tables
	mov.l	%d2,%d5			# copy ste count
	mov.l	&_high_addr,%a1		# last used address in high ram
	add.l	KM_LP(%sp),%a1		# make it physical

	# bump the first addr past the newly allocated page tables
	mov.l	&9,%d3			# 512 bytes per table
	lsl.l	%d3,%d5			# byte count
	sub.l	%d5,(%a1)		# allocate space

	# zero the allocated space
	mov.l	(%a1),%d3		# get the address
	mov.l	%d3,%d6			# save copy
        and.l   &SG3_FRAME,%d3          # round to bt boundry
	mov.l	%d3,(%a1)		# store it back
	sub.l	%d3,%d6			# new byte allocated
	add.l	%d6,%d5			# bump byte count
	mov.l	%d3,%a3			# copy it
	bra.b	clear_3lev_bt

next_3lev_bt:
	clr.l	(%a3)+			# clear the long
	subq.l	&4,%d5			# decrement byte count
clear_3lev_bt:
	bne.b	next_3lev_bt		# if more keep going

	tst.l	%d4			# did we skip one?
	bne.b	dont_update_addr2	# branch if so

	mov.l	%d3,%a2			# a2 has first block table address

dont_update_addr2:
	
	# construct a prototype ste
        and.l   &SG3_FRAME,%d3          # get physical page frame number
        or.l    &SG_RW+SG_V,%d3         # d3 has proto ste

	# load the segment table entries
	bra.b load_3level_ste

next_3level_ste:
	# store the segment table entry
        mov.l   %d3,(%a0)+              # store new segment table entry
	add.l	&512,%d3		# bump to next block table
load_3level_ste:
	dbf	%d2,next_3level_ste

skip_block_allocation:

	# calculate the starting block table index
	mov.l	KM_LA(%sp),%d5		# logical start address
	and.l   &SG3_BMASK,%d5		# mask off block table index bits
        mov.l   &SG3_BSHIFT,%d6		# make it an index
        lsr.l   %d6,%d5                 # d5 has start block table index

	# calculate the ending block table index
	mov.l	KM_LA(%sp),%d4		# logical start address
	add.l	KM_NB(%sp),%d4          # add the byte count
	subq.l	&1,%d4			# ending logical address
	and.l   &SG3_BMASK,%d4		# mask off block table index bits
        mov.l   &SG3_BSHIFT,%d6		# make it an index
        lsr.l   %d6,%d4                 # d4 has end segment table index

	cmpi.l	%d1,&1			# is ste count == 1?
	bne.b	morethan1_ste

	# only one segment table entry so only one block table
	sub.l	%d5,%d4			# start blkidx - end blkidx
	addq.l	&1,%d4			# d4 has bte count
	bra.b	got_bte_count

morethan1_ste:
	mov.l	&128,%d6		# NBTEBT
	sub.l	%d5,%d6			# NBTEBT - start blkidx
	mov.l	%d1,%d3			# get ste count
	subq.l	&2,%d3			# ste count - 2
	lsl.l	&7,%d3			# times num of btes per blk table
	add.l	%d3,%d6			# add middle table btes
	add.l	%d6,%d4			# add last table btes
	addq.l	&1,%d4			# one more for zero based index

got_bte_count:
	mov.l	%d4,%d1			# get bte count

	# calculate the block table offset
	mov.l	%d5,%d2			# get the index
	lsl.l	&2,%d2			# make it an offset

	# get a pointer to the appropriate block table entry
	mov.l   %a2,%a0                 # get block table start
	add.l   %d2,%a0                 # add block table offset
	mov.l   %a0,%a3                 # save starting bte pointer
	clr.l   %d4                  	# assume its not valid
	tst.l   (%a0)                   # is it already valid?
	beq.b   pt_unallocated_3lev     # skip if not valid

	# The block table entry was already set.  We must be continuing a
	# mapping in a partially used page table.  Just skip past the valid 
	# entry.
	mov.l   &1,%d4                  # remember that it was valid
	mov.l   (%a0),%d3               # get bte
	and.l   &BLK_FRAME,%d3          # mask out physical page frame
	mov.l   %d3,%a2                 # save addr of page table
	add.l   &4,%a0                  # bump to next bte
	sub.l   &1,%d1                  # need one less bte
	beq.w   skip_3lev_pt_alloc      # skip page table allocation

pt_unallocated_3lev:

	# figure out how much table space we need
	mov.l	%d1,%d2			# get bte count
	mov.l	&8,%d3			# 256 bytes per table
	lsl.l	%d3,%d2			# byte count
	mov.l	%d2,%d5			# save a copy

        # allocate page table space from highram
        mov.l   &_high_addr,%a1         # get address of variable
        add.l   KM_LP(%sp),%a1		# make it physical
        sub.l   %d2,(%a1)               # allocate space
        mov.l   (%a1),%d2               # get address
	
	tst.l	%d4			# did we skip one?
	bne.b	dont_update_addr3	# branch if so

	mov.l	%d2,%a2			# a2 has first page table address

dont_update_addr3:
	
	
	# zero the allocated space
	mov.l	%d5,%d4			# get byte count
	mov.l	%d2,%a1			# get the address
	bra.b	clear_3lev_pt

next_3lev_pt:
	clr.l	(%a1)+			# clear the byte
	subq.l	&4,%d4			# decrement byte count
clear_3lev_pt:
	bne.b	next_3lev_pt		# if more keep going

got_3lev_pt:

	# construct a prototype bte
        and.l   &BLK_FRAME,%d2          # get physical page frame number
        or.l    &SG_RW+SG_V,%d2         # d2 has proto bte

	# load the block table entries
	bra.b load_bte

next_bte:
	# store the block table entry
        mov.l   %d2,(%a0)+              # store new block table entry
	add.l	&256,%d2		# bump to next page table

	# make sure we get the next bte properly
	mov.l	%a0,%d4
	and.l	&0x1ff,%d4		# did we cross a block table boundry?
	bne.b	load_bte

	# get a pointer to the next bte
	add.l	&4,%a4			# bump to next ste
	mov.l	(%a4),%d3		# get ste
        and.l   &SG3_FRAME,%d3          # mask out physical page frame
	mov.l	%d3,%a0			# save addr of page table
load_bte:
	dbf	%d1,next_bte

skip_3lev_pt_alloc:
        tst.l   KM_PB(%sp)   		# should we load the ptes?
	beq	kmap_done		# branch if not

	# calculate the number of ptes required
	mov.l   KM_NB(%sp),%d0          # get the byte count
	add.l   &NBPG-1,%d0             # we need one pte per NBPG bytes
	mov.l   &12,%d1                 # round up
	lsr.l   %d1,%d0                 # d0 is the pte count

        # calculate the page table offset
	mov.l	KM_LA(%sp),%d1		# logical start address
        and.l   &SG3_PMASK,%d1          # mask off page table index
        mov.l   &SG_PSHIFT-LPTESIZE,%d2 # get shift value in d2
        lsr.l   %d2,%d1                 # d1 has page table offset

	# construct a prototype pte
        mov.l   KM_PA(%sp),%d2		# physical start address
        and.l   &PG_FRAME,%d2		# mask off page frame number
        or.l    KM_PB(%sp),%d2 		# whatever bits the caller wanted
        or.l    &PG_V,%d2         	# and make it valid

	# get a pointer to the first pte
	add.l	%d1,%a2			# pointer to ptes

	# load the page table entries
	bra.b	load_3level_pte

next_3level_pte:
        mov.l   %d2,(%a2)+              # store new segment table entry
	add.l	&PG_INCR,%d2		# bump to next page table

	mov.l	%a2,%d4
	and.l	&0xff,%d4		# did we cross a page table boundry?
	bne.b	load_3level_pte

	# get a pointer to the next pte
	add.l	&4,%a3			# bump to next ste
	mov.l	(%a3),%d3		# get bte
        and.l   &BLK_FRAME,%d3          # mask out physical page frame
	mov.l	%d3,%a2			# save addr of page table
load_3level_pte:
	dbf	%d0,next_3level_pte

kmap_done:
        movm.l  (%sp)+,%d1-%d6/%a0-%a4  # restore registers
	rts

 ###############################################################################
 # _set_crp(segtabaddr)
 ###############################################################################

        global _set_crp

_set_crp:

	movm.l  %d7/%a1,-(%sp)  	# save registers

#define SC_REGS	(2 * 4)
#define SC_SEGT	(4 + SC_REGS)

	mov.l	SC_SEGT(%sp),%d7	# get argument
	cmpi.l	_processor,&M68040	# is it a 68040?
	bne 	setcrp_not040	        # branch if not

	# set crp fot 68040
	long	0x4e7b7806		# movc %d7,crp
	short   0xF518           	# pflusha
        movm.l  (%sp)+,%d7/%a1      	# restore registers
	rts

setcrp_not040:
	mov.l	&_eightbytes,%a1	# get addr of tmp */
	mov.l   &0x7fff0002,(%a1)	# uplim,4-byte-valid
	mov.l	%d7,4(%a1)		# segtabaddr is d7
	long	0xf0114c00		# pmove (a1),crp
	long	0xf0002400        	# pflusha
	jsr	_purge_dcache
        movm.l  (%sp)+,%d7/%a1      	# restore registers
	rts

 ###############################################################################
 # _init_srp(loadpoint)
 ###############################################################################

        global _init_srp

_init_srp:

	movm.l  %d0/%a0-%a1,-(%sp)  # save registers

#define IS_REGS (3 * 4)
#define IS_LP   (4 + IS_REGS)


	# get _pmmu_exist variable
	mov.l 	&_pmmu_exist,%a1		# get variables logical address
	add.l	IS_LP(%sp),%a1			# make it physical

	# Branch the three level tablewalk specific code
	mov.l 	&_three_level_tables,%a0	# get variables logical address
	add.l	IS_LP(%sp),%a0			# make it physical
        tst.l	(%a0)				# are we using 3 level tables?
        bne     init_srp_3 		       	# branch if so

 	########################################################################
 	# Two level SRP setup
 	########################################################################

	# get a pointer to the segment table
	mov.l	&_Syssegtab,%a0			# get pointer to segment table
	add.l   IS_LP(%sp),%a0                  # make it physical
	mov.l	%a0,%d0				# physical segment table pointer

	# is this an HP or a Motorola MMU?
	tst.l	(%a1)			# _pmmu_exists == 1?
	beq.w	set_hp_mmu_srp		# no, then set HP MMU SRP

	# Set supervisor segment table pointer.
	# The segment table is currently pointed to by d0.
	mov.l	&_eightbytes,%a0	# get tmp addr
	add.l	IS_LP(%sp),%a0		# add in loadpoint offset
	mov.l	&0x80000202,(%a0)	# upper 32 bit of SRP
	move.l	%d0,4(%a0)		# lower 32 bits of SRP
	long	0xf0104800		# pmov	(%a0),SRP

        movm.l  (%sp)+,%d0/%a0-%a1      # restore registers
	rts

set_hp_mmu_srp:
	# Set supervisor segment table pointer.
	# The segment table is currently pointed to by d0.
	and.l	&PG_FRAME,%d0		# mask off physical page frame
	mov.l	&PGSHIFT,%d2		# SRP only uses significant bits
	lsr.l	%d2,%d0			# shift off insignificant bits
	mov.l	%d0,CPU_SRP_PADDR	# set supervisor root ptr & clear tlb

        movm.l  (%sp)+,%d0/%a0-%a1      # restore registers
	rts

 	########################################################################
 	# Three level SRP setup
 	########################################################################

init_srp_3:

	# get a pointer to the segment table
	mov.l	&_Syssegtab,%a0			# get pointer to segment table
	add.l   IS_LP(%sp),%a0                  # make it physical
	mov.l	%a0,%d0				# physical segment table pointer

	mov.l	&_processor,%a1
	add.l   IS_LP(%sp),%a1          # make it physical
	cmp.l   (%a1),&M68040           # is this an MC68040?
	bne     do_68030_3levels        # branch if not

	# Set supervisor segment table pointer.
	# The segment table is currently pointed to by d0.
        long    0x4E7B0807		# movec	%d0,SRP

	# purge the tlb
	short	0xF518			# pflusha

        movm.l  (%sp)+,%d0/%a0-%a1      # restore registers
	rts

do_68030_3levels:

	# set mc68030 rootpointer for three level tables
	mov.l   &_eightbytes,%a0        # get tmp addr
	add.l   IS_LP(%sp),%a0          # add in loadpoint offset
	mov.l   &0x7fff0202,(%a0)
	move.l  %d0,4(%a0)
	long    0xf0104800              # pmov   (%a0),SRP

        movm.l  (%sp)+,%d0/%a0-%a1      # restore registers
	rts


 ###############################################################################
 # _init_mmu(loadpoint)
 ###############################################################################

        global	_init_mmu
	global	_moto_mmu_enabled2
	global	_moto_mmu_enabled3
	global	_hp_mmu_enabled
	global	_m040_mmu_enabled

_init_mmu:

	movm.l  %d0/%a0-%a1,-(%sp)  # save registers

#define IM_REGS (3 * 4)
#define IM_LP   (4 + IM_REGS)


	# Branch the three level tablewalk specific code
	mov.l 	&_three_level_tables,%a0	# get variables logical address
	add.l	IM_LP(%sp),%a0			# make it physical
        tst.l	(%a0)				# are we using 3 level tables?
        bne     init_mmu_3			# branch if so

 	########################################################################
 	# Two level MMU enable
 	########################################################################

	# purge on-chip logical caches
	mov.l	&IC_CLR+IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
	long	0x4e7b0002 			# movec	d0,cacr

	# turn off external cache (and all the rest of the stuff)
        clr.w   MMU_BASE_PADDR+0x0e

	# is this an HP or a Motorola MMU?
	mov.l 	&_pmmu_exist,%a0		# get variables logical address
	add.l	IM_LP(%sp),%a0			# make it physical
	tst.l	(%a0)				# _pmmu_exist == 1?
	beq.w	enable_hp_mmu			# no then its an HP MMU

	# Enable FP unit and caches
	mov.w &MMU_CEN+MMU_IEN+MMU_FPE,MMU_BASE_PADDR+0x0e

	# Enable the MMU hardware for 2 level translations.
        # the PMMU on the MC68030 only supports control
        # alterable addressing modes
        mov.l   &0x82c0aa00,-(%sp)
        long    0xf0174000       	       # pmov(%sp),TC
        add.l   &0x4,%sp

	# The MMU is now on, get to the jmp to make us logical
	jmp	_moto_mmu_enabled2
_moto_mmu_enabled2:

	movm.l  (%sp)+,%d0/%a0-%a1		# restore registers
	rts
	
enable_hp_mmu:
	# Enable FP unit and caches and MMU hardware.
	mov.w	&MMU_SMEN+MMU_UMEN+MMU_CEN+MMU_IEN+MMU_FPE,MMU_BASE_PADDR+0x0e

	# The MMU is now on, get to the jmp to make us logical
	jmp	_hp_mmu_enabled
_hp_mmu_enabled:

	movm.l  (%sp)+,%d0/%a0-%a1		# restore registers
	rts

init_mmu_3:
 	########################################################################
 	# Three level MMU enable
 	########################################################################

	mov.l	&_processor,%a1
	add.l   IM_LP(%sp),%a1          # make it physical
	cmp.l   (%a1),&M68040           # is this an MC68040?
	bne     enable_030mmu_3levels   # branch if not

	# Turn on the MMU
        mov.w   &0,MMU_BASE_PADDR+0x0e
        mov.w   &MMU_IEN+MMU_CEN+MMU_FPE,MMU_BASE_PADDR+0x0e
	mov.l	&0x00008000,%d0
	long	0x4E7B0003	# movec %d0,TC

	# Purge and enable the caches
        short   0xF4D8          # cinva IC/DC
#ifdef notyet
        # disable caches
        mov.l   &0,%d0
        long    0x4e7b0002      # movec d0,cacr
#else /* notyet, NOCACHE */
        mov.l   &MC68040_IC_ENABLE+MC68040_DC_ENABLE,%d0
        long    0x4e7b0002      # movec d0,cacr
#endif /* else notyet, NOCACHE */


	jmp	_m040_mmu_enabled
_m040_mmu_enabled:

	movm.l  (%sp)+,%d0/%a0-%a1		# restore registers
	rts
enable_030mmu_3levels:

	# purge the caches
        mov.l   &IC_CLR+IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
        long    0x4e7b0002 		# movec   d0,cacr
        mov.w   &0,MMU_BASE_PADDR+0x0e
        mov.w   &MMU_IEN+MMU_CEN+MMU_FPE,MMU_BASE_PADDR+0x0e

        # enable PMMU for three level tables
        mov.l   &0x82c07760,-(%sp)
        long    0xf0174000              # pmov(%sp),TC
        add.l   &0x4,%sp

	jmp	_moto_mmu_enabled3
_moto_mmu_enabled3:

	movm.l  (%sp)+,%d0/%a0-%a1		# restore registers
	rts

 ###############################################################################
 # _itablewalk(segment_table, logical_address)
 ###############################################################################

	global _itablewalk

_itablewalk:
	# get the appropriate segment table entry
	mov.l   4(%sp),%d0		# get logical segment table pointer
	bne.b	tablewalk0		# quit if NULL
	rts				# none, return NULL

tablewalk0:
	mov.l	%d0,%a0			# segment table pointer
	mov.l	8(%sp),%a1		# logical start address

        # Branch to three level tablewalk specific code?
        tst.l	_three_level_tables     # are we using 3 level tables?
        bne.b	three_lev_walk          # branch if so

 	########################################################################
 	# Two level tablewalk
 	########################################################################

	# calculate the segment table offset
        mov.l   &SG_ISHIFT-LPTESIZE,%d1	# prepare to shift them to the bottom
	mov.l	%a1,%d0			# logical start address
	and.l   &SG_IMASK,%d0		# mask off segent table index bits
        lsr.l   %d1,%d0                 # segment table offset

	add.l	%d0,%a0			# index into segment table
	mov.l	(%a0),%d0		# get ste entry
	bne.b	tablewalk1		# quit if NULL
	rts

tablewalk1:
	and.l	&SG_FRAME,%d0		# has page table address
	mov.l	%d0,%a0

        # calculate the page table offset
	mov.l	%a1,%d0			# logical start address
        and.l   &SG_PMASK,%d0           # mask off page table index
        mov.l   &SG_PSHIFT-LPTESIZE,%d1 # get shift value in d2
        lsr.l   %d1,%d0                 # d1 has page table offset

	add.l	%a0,%d0			# return pointer to pte in d0
	rts

 	########################################################################
 	# Three level tablewalk
 	########################################################################

	# a0 = get logical segment table pointer
	# a1 = logical start address

three_lev_walk:

	# calculate the segment table offset
        mov.l   &SG3_ISHIFT-LPTESIZE,%d1
	mov.l	%a1,%d0			# logical start address
	and.l   &SG3_IMASK,%d0		# mask off segent table index bits
        lsr.l   %d1,%d0                 # has segment table offset

	# get the appropriate segment table entry
	add.l	%d0,%a0			# index into segment table
	mov.l	(%a0),%d0		# get ste
	bne.b	tablewalk2		# quit if NULL
	rts

tablewalk2:
	and.l	&SG3_FRAME,%d0		# d0 has block table address
	mov.l	%d0,%a0			# block table pointer

	# calculate the block table offset
	mov.l	%a1,%d0			# logical start address
	and.l   &SG3_BMASK,%d0		# mask off block table index bits
        mov.l   &SG3_BSHIFT-LPTESIZE,%d1 
        lsr.l   %d1,%d0                 # d1 has block table offset
	
	# get the appropriate block table entry
	add.l	%d0,%a0			# add block table offset
	mov.l	(%a0),%d0		# get bte
	bne.b	tablewalk3		# quit if NULL
	rts

tablewalk3:
	and.l	&BLK_FRAME,%d0		# d0 has page table address
	mov.l	%d0,%a0			# page table address

        # calculate the page table offset
	mov.l	%a1,%d0			# logical start address
        and.l   &SG3_PMASK,%d0          # mask off page table index
        mov.l   &SG_PSHIFT-LPTESIZE,%d1
        lsr.l   %d1,%d0                 # has page table offset
	add.l	%a0,%d0			# return pointer to pte in d0
	rts

 ###############################################################################
 # _tablewalk(segment_table, logical_address)
 ###############################################################################

	global _tablewalk

_tablewalk:
	# get the appropriate segment table entry
	mov.l   4(%sp),%d0		# get logical segment table pointer
	bne.b	itablewalk0		# quit if NULL
	rts				# none, return NULL

itablewalk0:
	mov.l	%d0,%a0			# segment table pointer
	mov.l	8(%sp),%a1		# logical start address

        # Branch to three level tablewalk specific code?
        tst.l	_three_level_tables     # are we using 3 level tables?
        bne.b	ithree_lev_walk          # branch if so

 	########################################################################
 	# Two level tablewalk
 	########################################################################

	# calculate the segment table offset
        mov.l   &SG_ISHIFT,%d1		# prepare to shift them to the bottom
	mov.l	%a1,%d0			# logical start address
        lsr.l   %d1,%d0                 # segment table offset

	mov.l	(%a0,%d0.l*4),%d0	# get ste entry
	bne.b	itablewalk1		# quit if NULL
	rts

itablewalk1:
	and.l	&SG_FRAME,%d0		# has page table address
	mov.l	%d0,%a0

        # calculate the page table offset
	mov.l	%a1,%d0			# logical start address
        and.l   &SG_PMASK,%d0           # mask off page table index
        mov.l   &SG_PSHIFT-LPTESIZE,%d1 # get shift value in d2
        lsr.l   %d1,%d0                 # d1 has page table offset

	add.l	%a0,%d0			# return pointer to pte in d0

	mov.l	%d0,%a0			# make it an address
	mov.l	(%a0),%d1		# get the pte
	btst	&1,%d1			# is it an indirect pte?
	bne.b	indirect_pte2
	rts

indirect_pte2:
	mov.l	%d1,%d0
	and.b	&0xfc,%d0		# upper 30 bits is the address
	rts

 	########################################################################
 	# Three level tablewalk
 	########################################################################

	# a0 = get logical segment table pointer
	# a1 = logical start address

ithree_lev_walk:

	# calculate the segment table offset
        mov.l   &SG3_ISHIFT,%d1
	mov.l	%a1,%d0			# logical start address
        lsr.l   %d1,%d0                 # has segment table offset

	# get the appropriate segment table entry
	mov.l	(%a0,%d0.l*4),%d0	# get ste
	bne.b	itablewalk2		# quit if NULL
	rts

itablewalk2:
	and.l	&SG3_FRAME,%d0		# d0 has block table address
	mov.l	%d0,%a0			# block table pointer

	# calculate the block table offset
	mov.l	%a1,%d0			# logical start address
	and.l   &SG3_BMASK,%d0		# mask off block table index bits
        mov.l   &SG3_BSHIFT-LPTESIZE,%d1 
        lsr.l   %d1,%d0                 # d1 has block table offset
	
	# get the appropriate block table entry
	add.l	%d0,%a0			# add block table offset
	mov.l	(%a0),%d0		# get bte
	bne.b	itablewalk3		# quit if NULL
	rts

itablewalk3:
	and.l	&BLK_FRAME,%d0		# d0 has page table address
	mov.l	%d0,%a0			# page table address

        # calculate the page table offset
	mov.l	%a1,%d0			# logical start address
        and.l   &SG3_PMASK,%d0          # mask off page table index
        mov.l   &SG_PSHIFT-LPTESIZE,%d1
        lsr.l   %d1,%d0                 # has page table offset
	add.l	%a0,%d0			# return pointer to pte in d0

	mov.l	%d0,%a0			# make it an address
	mov.l	(%a0),%d1		# get the pte
	btst	&1,%d1			# is it an indirect pte?
	bne.b	indirect_pte3
	rts

indirect_pte3:
	mov.l	%d1,%d0
	and.b	&0xfc,%d0		# upper 30 bits is the address
	rts

 ###############################################################################
 # Test for the presence of the debugger.  If found then jump to it.
 # This function returns 1 if kdb was called and 0 otherwise.
 ###############################################################################

	global _call_kdb

_call_kdb:
	mov.l	&0,%d0		# return 0 if kdb is not called
	mov.l	_dbg_present,%d1
	bne.b	tstdb1
	mov.l	&0x88001e,_dbg_addr
	bra.b	tstdb2
tstdb1:
	cmp.l	%d1,&1
	bne.b	no_kdb
tstdb2:
	# call kdb
	mov.l	&1,_monitor_on
	mov.w	&0,-(%sp)
	pea	kdb_return
	mov.w	%sr,-(%sp)
	mov.l	_dbg_addr,%a0
	jmp	(%a0)
kdb_return:
	mov.l	&1,%d0		# indicate that kdb was called
no_kdb:
	rts


 ###############################################################################
 # icode - the first user process!
 # 
 # The icode is the user code for the first process.  It contains just enough
 # instructions to do en exec of /etc/init gettint the whole world going.
 ###############################################################################
	data
	global _icode
	global _szicode
_icode:
	mov.l	&NBPG,%sp
	mov.l	&envp-_icode,-(%sp)
	mov.l	&argp-_icode,-(%sp)
	mov.l	&name-_icode,-(%sp)
	clr.l	-(%sp)
	movq	&59,%d0   	#exec system call
	trap	&0
_icode1S:	bra.w	_icode1S
	     
argp:	long	name-_icode
envp:	long	0

#ifdef XTERMINAL
name:	byte	"/etc/xtinit",0
#else
name:	byte	"/etc/init",0
#endif /* XTERMINAL */
	lalign	2
iend:	short	1
_szicode:
	long	iend-_icode

	text
#ifdef NFS_DISKLESS
 ###############################################################################
 # kernel_rc code - run from main() early on to start networking, mount NFS 
 # root, get initial (NFS-based) swap, etc
 ###############################################################################

	data
	global _krc_code
	global _szkrccode
_krc_code:
	mov.l	&NBPG,%sp

	# open /dev/console on fd zero, and duplicate it on one and two

	movq	&2,%d0		# open("/dev/console", 2)
	mov.l	%d0,-(%sp)	# arg 1
	mov.l	&krc_cons-_krc_code,-(%sp)	# arg 0
	clr.l	-(%sp)		# dummy arg
	movq	&5,%d0
	trap	&0
	tst.l	%d0		# ret == 0 ?
	bne.l	krc_exit

	clr.l	-(%sp)		# arg 0
	clr.l	-(%sp)		# dummy arg
	movq	&41,%d0		# dup(0)
	trap	&0
	movq	&1,%d1
	cmp.l	%d1,%d0		# ret == 1 ?
	bne.l	krc_exit

	clr.l	-(%sp)		# arg 0
	clr.l	-(%sp)		# dummy arg
	movq	&41,%d0		# dup(0)
	trap	&0
	movq	&2,%d1
	cmp.l	%d1,%d0		# ret == 2 ?
	bne.l	krc_exit

	# execve("/bin/sh", krc_argp, krc_envp)
	mov.l	&krc_envp-_krc_code,-(%sp)	# envp, arg 2
	mov.l	&krc_argp-_krc_code,-(%sp)	# argp, arg 1
	mov.l	&krc_prog-_krc_code,-(%sp)	# prog, arg 0
	clr.l	-(%sp)		# dummy arg
	movq	&59,%d0   	#exec system call
	trap	&0
krc_exit:
	clr.l	-(%sp)		# dummy arg
	movq	&1,%d0		# exit system call
	trap	&0
      
krc_argp:	long	krc_name-_krc_code	# argp[0] = "/etc/kernel_rc"
		long	krc_name-_krc_code	# argp[1] = "/etc/kernel_rc"
		long     0
krc_envp:					
		long     0

krc_cons:	byte	"/dev/console", 0
krc_prog:	byte	"/bin/sh",0
	lalign	2
krc_name:	byte	"/etc/kernel_rc",0
	lalign	2
krc_end:	short	1
_szkrccode:
	long	krc_end-_krc_code
	text

#endif /* NFS_DISKLESS */

 ###############################################################################
 # pre_init_rc code
 ###############################################################################

	data
	global _fcode
	global _szfcode
_fcode:
	mov.l	&NBPG,%sp

	# open /dev/console on fd zero, and duplicate it on one and two

	movq	&2,%d0		# open("/dev/console", 2)
	mov.l	%d0,-(%sp)	# arg 1
	mov.l	&fcons-_fcode,-(%sp)	# arg 0
	clr.l	-(%sp)		# dummy arg
	movq	&5,%d0
	trap	&0
	tst.l	%d0		# ret == 0 ?
	bne.l	fexit

	clr.l	-(%sp)		# arg 0
	clr.l	-(%sp)		# dummy arg
	movq	&41,%d0		# dup(0)
	trap	&0
	movq	&1,%d1
	cmp.l	%d1,%d0		# ret == 1 ?
	bne.l	fexit

	clr.l	-(%sp)		# arg 0
	clr.l	-(%sp)		# dummy arg
	movq	&41,%d0		# dup(0)
	trap	&0
	movq	&2,%d1
	cmp.l	%d1,%d0		# ret == 2 ?
	bne.l	fexit

	# execve("/bin/sh", fargp, fenvp)
	mov.l	&fenvp-_fcode,-(%sp)	# envp, arg 2
	mov.l	&fargp-_fcode,-(%sp)	# argp, arg 1
	mov.l	&fprog-_fcode,-(%sp)	# prog, arg 0
	clr.l	-(%sp)		# dummy arg
	movq	&59,%d0   	#exec system call
	trap	&0
fexit:
	clr.l	-(%sp)		# dummy arg
	movq	&1,%d0		# exit system call
	trap	&0
	     
fargp:	long	fname-_fcode	# argp[0] = "/etc/pre_init_rc"
	long	fname-_fcode	# argp[1] = "/etc/pre_init_rc"
fenvp:	long	0

fcons:	byte	"/dev/console", 0
fprog:	byte	"/bin/sh",0
	lalign	2
fname:	byte	"/etc/pre_init_rc",0
	lalign	2
fend:	short	1
_szfcode:
	long	fend-_fcode
	text

 ###############################################################################
 # Boot Code
 ###############################################################################

	data
	global _boot_code
	global _szboot_code
_boot_code:
	mov.w	&0x2700,%sr		# no interrupts

        cmpi.l  _processor,&3           # is this an MC68040?
        beq     boot_68040        	# branch if so

	tst.l	_pmmu_exist
	beq.w	pmmuskip4

	# the PMMU on the MC68030 only supports control
	# alterable addressing modes
	mov.l	&0,-(%sp)
	long	0xf0174000		# pmove (%sp),TC
	addq	&4,%sp
	mov.w	&0,MMU_BASE_PADDR+0x0e	# turn off the mmu and caches
	jmp	0x1c0			# jmp into bootrom req_boot
pmmu_boot_loop:
    	stop	&0x2700
	bra.b	pmmu_boot_loop
	rts
pmmuskip4:
	mov.w	&0,MMU_BASE+0x0e	# turn off the mmu and caches
	jmp	0x1c0			# jmp into bootrom req_boot
boot_loop:
    	stop	&0x2700
	bra.b	boot_loop
	rts

boot_68040:
	clr.l	%d0

        # disable caches
        long    0x4e7b0002      	# movec d1,cacr

	# turn off the MMU
	long	0x4E7B0003		# movec %d0,TC

	jmp	0x1c0			# jmp into bootrom req_boot
boot_68040_loop:
    	stop	&0x2700
	bra.b	boot_68040_loop

bootend:
_szboot_code:
	long	bootend-_boot_code

 ###############################################################################
 # Rom Printf Code
 ###############################################################################

	data
	global _romprf_code
	global _szromprf
_romprf_code:
	mov.l	&0xfffffac0,%sp
	mov.w	&0x2700,%sr		#* no interrupts

        cmpi.l  _processor,&3           # is this an MC68040?
        beq     rom_prf_68040        	# branch if so

	tst.l	_pmmu_exist
	beq.w	pmmuskip5
	# the PMMU on the MC68030 only supports control
	# alterable addressing modes
	mov.l	&0,-(%sp)
	long	0xf0174000	# pmove (%sp),TC
	addq	&4,%sp
pmmuskip5:
	mov.w	&0,MMU_BASE_PADDR+0x0e  #* turn off the mmu and caches
	mov.l	&romprf_end-_romprf_code,%a0
	add.l	&0xfffff800,%a0
	movq	&6,%d0
	jsr	0x150			#* call crtmsg
romprf_loop:
    	stop	&0x2700
	bra.b	romprf_loop

rom_prf_68040:
	clr.l	%d0

        # disable caches
        long    0x4e7b0002      	# movec d1,cacr

	# turn off the MMU
	long	0x4E7B0003		# movec %d0,TC

	mov.l	&romprf_end-_romprf_code,%a0
	add.l	&0xfffff800,%a0
	movq	&6,%d0
	jsr     0x150                   #* call crtmsg

rom_prf_68040_loop:
    	stop	&0x2700
	bra.b	rom_prf_68040_loop

romprf_end:
_szromprf:
	long	romprf_end-_romprf_code

	
	text

 ###############################################################################
 # Recover routine called by a Pascal driver escape
 ###############################################################################
	global	recover
recover:	
	rts

	data
	global	_mc68881
_mc68881:
	byte	0
 # start mod. for BANANAJR	
	data
	global	_pmmu_exist, _machine_model
_pmmu_exist:
	long	0
_machine_model:
	short	0
 # end mod. for BANANAJR
	text



 # get the current sp value (more or less).  see selcode.c for details of
 # whats going on here.
		
	global	_get_sp
_get_sp:	mov.l	%sp,%d0
	subq.l	&2,%d0
	rts	
		

 ###############################################################################
 # Routine to simulate the Modcal TRY/RECOVER construct for use in/by C.
 # The necessary data is squirreled away in C__trystuff as follows:
 # struct C__trystuff {
 #	int	pctr;
 #	int	link;
 #	int	regs[12];
 #	};
 #
 # The following macros (in tryrec.h) can be used to simulate the
 # syntax of try/recover
 #	#define try if (C__tst()){struct C__trystuff TR; C__try(TR);
 #	#define recover c__rec(TR);} else 
 #
 # C__tst is used to save the return address of the if/else, which will return
 # 1 on a try, and 0 on an escape/recover.
 # C__try is called with a parameter of a structure in which to save the
 # current status.
 # The return address (and mark stack) are put in the area specified by the
 # parameter, and linked to the head (as used by escape) and the previous 
 # (to be used on a normal recover).
 # Any kernel try/recover/escape is independent of user space versions.
 # If there was no try/recover; escape will simply return.
 ###############################################################################
		
	global	_C__try,_C__rec,_C__tst,_escape
		
_C__tst:	mov.l	(%sp),-(%sp)		#Hide a copy of RA under TOS (saved pc)
	movq	&1,%d0
	rts	
		
_C__try:	mov.l	4(%sp),%a0		#Get location of structure
	movm.l	&0xFCFC,TRY_REGS(%a0)	#save registers
	mov.l	_u+PCB_TRYCHAIN,TRY_LINK(%a0) #link stack
	mov.l	%a0,_u+PCB_TRYCHAIN	#update the head
	rts	
		
_C__rec:	mov.l	4(%sp),%a0		#get block address
	mov.l	TRY_LINK(%a0),_u+PCB_TRYCHAIN #unlink it
	mov.l	(%sp)+,%a0		#get ra
	addq.l	&4,%sp			#pop off arg
	jmp	(%a0)			#return, and let caller pop saved pc
		
_escape:	mov.l	_u+PCB_TRYCHAIN,%a0	#put queue head pointer
	mov.l	%a0,%d0			#was there a link?
	beq.w	_escape1S			#no, bail out
	mov.l	4(%sp),_u+PCB_ESCAPECODE	#put in global escapecode
	movm.l	TRY_REGS(%a0),&0xFCFC	#restore registers
	mov.l	TRY_LINK(%a0),_u+PCB_TRYCHAIN	#restore the head
	clr.l	%d0			#return to C__tst location with a 0
	addq.l	&8,%sp			#clean off try arg and ra
	rts				#rts to saved pc (or caller if error)

_escape1S:	mov.l	&escmsg,-(%sp)		#panic if no try/recover
	jsr	_panic


 ###############################################################################
 # Testm: test if memory is present or not.
 #	testr(addr,width)  (read version)
 #	testw(addr,width)  (write)
 #	char*addr;
 #	int width;  /* width is 1, 2, or 4 normally*/
 #		/* check that*addr and *(addr+width-1) both accessible */
 #
 # NOTE: These routines can cause bus errors.  A "trial flag" is set to 
 # indicate to the bus error handler that one of these "special" routines was
 # the source of the bus error, should one occur.  _xbuserror expects to find
 # the old value of the bus_trial flag in %d1.
 ###############################################################################
 
	global	_testr,_testw

_testr:	mov.l	_bus_trial,%d1		#save old bus trial flag, and set to 1
	mov.l	&1,_bus_trial
	mov.l	4(%sp),%a0		#get arg (address to test)
	nop
	tst.b	(%a0)			#will return with 1 in d0 if no error
	nop
	add.l	8(%sp),%a0
	subq.l	&1,%a0
	nop
	tst.b	(%a0)
	nop
test_done:
	movq	&1,%d0			#will return with 1 in d0 if no error
	mov.l	%d1,_bus_trial		#restore old bus trial flag 
	rts

_testw:	mov.l	_bus_trial,%d1		#save old bus trial flag, and set to 1
	mov.l	&1,_bus_trial
	mov.l	4(%sp),%a0		#get arg (address to test)
	movq	&0,%d0
	nop
	mov.b	%d0,(%a0)			#write byte -- destructive
	nop
	add.l	8(%sp),%a0
	subq.l	&1,%a0
	nop
	mov.b	%d0,(%a0)			#write byte -- destructive
	nop
	bra	test_done

 ###############################################################################
 # These (*copy_prot) routines are provided for the VME and customer
 # written drivers.  They are used when the address may cause a bus error.
 #
 # if (!bcopy_prot(source, dest, numb_bytes))
 #         <produced busserror>
 ###############################################################################

	global	_bcopy_prot
_bcopy_prot:
	movm.l	4(%sp),%d0/%a0-%a1	# d0 = src; a0 = dst; a1 = cnt
	exg	%d0,%a1			# a1 = src; a0 = dst; d0 = cnt

	mov.l	_bus_trial,%d1		#save old bus trial flag, and set to 1
	mov.l	&1,_bus_trial

	subq.l	&1,%d0
	blt	test_done		# Done
	swap	%d0			# Pre-position
bcopy_prot0:
	swap	%d0			# Re-position
bcopy_prot1:
	mov.b	(%a1)+,(%a0)+		# and move 
	dbra	%d0,bcopy_prot1
	swap	%d0			# get remainder bytes
	dbra	%d0,bcopy_prot0
	bra	test_done

 # if (!scopy_prot(source, dest, numb_shorts))
 #         <produced busserror>

	global	_scopy_prot
_scopy_prot:
	movm.l	4(%sp),%d0/%a0-%a1	# d0 = src; a0 = dst; a1 = cnt
	exg	%d0,%a1			# a1 = src; a0 = dst; d0 = cnt

	mov.l	_bus_trial,%d1		#save old bus trial flag, and set to 1
	mov.l	&1,_bus_trial

	subq.l	&1,%d0
	blt	test_done		# Done
	swap	%d0			# Pre-position
scopy_prot0:
	swap	%d0			# Re-position
scopy_prot1:
	mov.w	(%a1)+,(%a0)+		# and move 
	dbra	%d0,scopy_prot1
	swap	%d0			# get remainder bytes
	dbra	%d0,scopy_prot0
	bra	test_done

 # if (!lcopy_prot(source, dest, numb_longs))
 #         <produced busserror>

	global	_lcopy_prot
_lcopy_prot:
	movm.l	4(%sp),%d0/%a0-%a1	# d0 = src; a0 = dst; a1 = cnt
	exg	%d0,%a1			# a1 = src; a0 = dst; d0 = cnt

	mov.l	_bus_trial,%d1		#save old bus trial flag, and set to 1
	mov.l	&1,_bus_trial

	subq.l	&1,%d0
	blt	test_done		# Done
	swap	%d0			# Pre-position
lcopy_prot0:
	swap	%d0			# Re-position
lcopy_prot1:
	mov.l	(%a1)+,(%a0)+		# and move 
	dbra	%d0,lcopy_prot1
	swap	%d0			# get remainder bytes
	dbra	%d0,lcopy_prot0
	bra	test_done



	global	_initialize_68881
	global	_is_mc68882
_is_mc68882:
	sub.w	&212,%sp
	short	0xf317  # fsave (sp)  for 882
	mov.w	(%sp),%d0
	andi	&0xff,%d0
	cmp.b	%d0,&0x18
	beq	is_881
	mov.l	&1,%d0
	add.w	&212,%sp
	rts
is_881:
	mov.l	&0,%d0
	add.w	&212,%sp
	rts

 ###############################################################################
 # If we attempt to initialize a 68881 and one is not there (as on 310) the
 # cpu will take a linef emulation exception.  So, we set a flag saying that we
 # are doing this trial and the _linef_emulate code will test for this sort
 # of trial.  If we take the exception, linef_emulate will clean up and return
 # to our caller.  Otherwise, restore the previous flag value and return.
 ###############################################################################

_initialize_68881:
	mov.l	_linef_trial,%a1	#save what was in the linef trial flag
	mov.l	&1,_linef_trial		# mark flag to say "trial is active"
	movq	&1,%d0			# plan to return 1, meaning "ok"
	clr.l	-(%sp)			#push null format word on stack
 #	frestore (sp)			restore null frame ==> reset
	short	0xf357
	addq.l	&4,%sp			#pop off format word
	mov.l	&0x0000ff00,%d1		#enable all trap conditions
 #	fmove	d1,<cntl>		
	short	0xf201
	short	0x9000
	mov.l	%a1,_linef_trial	#restore prev value of linef trial flag 
	rts				# no exception taken!

	global	_reset_mc68881
_reset_mc68881:
	tst.b	_mc68881		#floating point coprocessor?
	beq.w	reset_not_here
	clr.l	-(%sp)			#push null format word on stack
 #	frestore (sp)			restore null frame ==> reset
	short	0xf357
	addq.l	&4,%sp			#pop off format word
	mov.l	&0x0000ff00,%d1		#enable all trap conditions
 #	fmove	d1,<cntl>		
	short	0xf201
	short	0x9000
reset_not_here:
	rts


	data
	global	_monitor_on,_processor
_monitor_on:
	long	0
_processor:
	long	0


escmsg:		byte	"Unexpected escape.",0
		


 ###############################################################################
 #   This is the signal trampoline code which is used by sendsig() to
 #   invoke a user signal routine.
 #
 #   The trampoline code is invoked on the interrupt stack (possibly specified
 #   by the sigstack intrinsic), where it invokes the user signal handler.
 #   The interrupt stack has a sigframe structure (defined in sendsig.h) on it;
 #   this structure includes a sigcontext structure (defined in signal.h) in it.
 #   The following diagram shows the fields used by the trampoline code
 #   shows the location of the stack pointer at each step listed above.
 #
 #      struct sigframe (interrupt stack):
 #
 #	   sf_handler		user routine address
 #	   sf_signum		signal number          \     3 parameters 
 #	   sf_code		signal type field       |-- to user handler
 #      --- sf_scp		signal context ptr     /
 #      |
 #      |
 #      --> sf_full.fs_context	context available to user handler
 #			.
 #			.
 #			.
 #	   sf_full.fs_regs	space to save registers
 #
 #   The code does:
 #
 #      1) saves the registers on this stack (in sf_full.fs_regs)
 #      2) calls signal routine (sf_handler),
 #	  popping address off stack to leave handler''s arguments in place
 #      3) on return from signal routine traps to kernel for sigcleanup()
 ###############################################################################

 #	hidden intrinsic for post-signal cleanup
	set	SIGCLEANUP,139
	text

sigcode:
	movm.l	%d0-%d7/%a0-%a6,SF_GPR_REGS(%sp)	#| save 15 register values
	mov.l	(%sp)+,%a0			#| get address of user routine
	jsr	(%a0)				#| call user signal
	mov.l	&SIGCLEANUP,%d0			#| call kernel
	trap	&0
sigcode_end:
 #						| should never get here!


 ###############################################################################
 # Copy a null terminated string from the user address space into
 # the kernel address space.
 #
 # copyinstr(ufromaddr, ktoaddr, maxlength, &lencopied)
 # CAUTION: routine is for small long word moves and moves less than 65K bytes.
 ###############################################################################

Lcopyerr: 
	clr.l	_u+PROBE_ADDR		#clear probe return
Lcopyerr1: 
	mov.l	&EFAULT,%d0		#return EFAULT
	rts

	global	_copyinstr

_copyinstr:
	mov.l	12(%sp),%d0		# d0 = maxlength
	mov.l	16(%sp),%a0		# a0 = &lencpy
	mov.l	%d0,(%a0)		# *lencopied = maxlength
	mov.l	4(%sp),%a0		# a0 = src
	mov.l	8(%sp),%a1		# a1 = dst

	subq.w	&1,%d0			# adjust count for turkey dbra
	blt.b	Lcopyerr1		# zero or less return EFAULT

	mov.l	&Lcopyerr,_u+PROBE_ADDR	# set the probe error return

Lcopyinstr0:
	movs.b	(%a0)+,%d1		# move byte from userland
	mov.b	%d1,(%a1)+		# move byte to kerneland
	dbeq	%d0,Lcopyinstr0		# check null byte or no more buffer

	bne.b	Lcopyinstr1		# check if ran out of buffer 

	mov.l	16(%sp),%a1		# a1 = &lencpy
	sub.l	%d0,(%a1)		# *lencopied = maxlength - remainder;
	movq	&0,%d0
	mov.l	%d0,_u+PROBE_ADDR	# clear the probe error return
	rts

Lcopyinstr1:
	mov.l	&ENOENT,%d0		# return ENOENT (ran out of buffer)
	clr.l	_u+PROBE_ADDR		# clear the probe error return
	rts

 # Copy a null terminated string from the kernel
 # address space to the user address space.
 #
 # copyoutstr(fromaddr, toaddr, maxlength, &lencopied)
 # CAUTION: This routine is for small long byte moves and moves less than 65K bytes.

	global	_copyoutstr

_copyoutstr:
	mov.l	12(%sp),%d0		# d0 = maxlength
	mov.l	16(%sp),%a0		# a0 = &lencpy
	mov.l	%d0,(%a0)		# *lencopied = maxlength
	mov.l	4(%sp),%a0		# a0 = src
	mov.l	8(%sp),%a1		# a1 = dst

	subq.l	&1,%d0			# adjust count for turkey dbra
	blt.w	Lcopyerr1		# zero or less return EFAULT

	mov.l	&Lcopyerr,_u+PROBE_ADDR	# set the probe error return

Lcopyoutstr0:
	mov.b	(%a0)+,%d1		# move byte from kerneland
	movs.b	%d1,(%a1)+		# move byte to userland
	dbeq	%d0,Lcopyoutstr0	# check null byte or no more buffer

	bne.b	Lcopyinstr1		# check if ran out of buffer 

	mov.l	16(%sp),%a1		# a1 = &lencpy
	sub.l	%d0,(%a1)		# *lencopied = maxlength - remainder;
	movq	&0,%d0
	mov.l	%d0,_u+PROBE_ADDR	# clear the probe error return
	rts

 # Copy a null terminated string from one point to another in
 # the kernel address space.
 #
 # copystr(kfromaddr, ktoaddr, maxlength, &lencopied)
 # CAUTION: This routine is for small long byte moves and moves less than 65K bytes.

	global	_copystr

_copystr:
	mov.l	12(%sp),%d0		# d0 = maxlength
	mov.l	16(%sp),%a0		# a0 = &lencpy
	mov.l	%a0,%d1			# d1 = &lencpy
	mov.l	%d0,(%a0)		# *lencopied = maxlength
	mov.l	4(%sp),%a0		# a0 = src
	mov.l	8(%sp),%a1		# a1 = dst

	subq.l	&1,%d0			# adjust count for turkey dbra
	blt.w	Lcopyerr1		# zero or less longs return EFAULT

Lcopystr0:
	mov.b	(%a0)+,(%a1)+		# move byte
	dbeq	%d0,Lcopystr0		# check null byte or no more buffer

	bne.b	Lcopystr1		# check if ran out of buffer 

	mov.l	%d1,%a1			# get lencopyied pointer
	sub.l	%d0,(%a1)		# *lencopied = maxlength - remainder;
	movq	&0,%d0
	rts

Lcopystr1:
	mov.l	&ENOENT,%d0		#return ENOENT (null byte not copied)
	rts

 # copyout4(source, dest, length_longs) - copy kernel to userland (4 byte aligned)
 # CAUTION: This routine is for small long word moves. 
 #
 # Copy memory from src(USERland) to dest(KERNEL) for count longs.
 # err == 0 if ok, else == -1 if illegal address in buffers
 #
	global	_copyout4

_copyout4:
	mov.l	4(%sp),%a0		# 1 a0 = src (s1)
	mov.l	8(%sp),%a1		# 1 a1 = dst (s2)
	mov.l	12(%sp),%d0		# 1 d0 = count
	mov.l	&Lcopyerr,_u+PROBE_ADDR	# 2 set the probe error return
	bra.b	Lo4copy_entry		# 2 go do it

Lo4switch_table:
	bra.b	Lo40			# 2 clear probe flag and exit
	bra.b	Lo41			# 2
	bra.b	Lo42			# 2
Lo43:	mov.l	(%a0)+,%d1		#11 What a pig!!!
	movs.l	%d1,(%a1)+		# 1
Lo42:	mov.l	(%a0)+,%d1		#11 What a pig!!!
	movs.l	%d1,(%a1)+		# 1
Lo41:	mov.l	(%a0)+,%d1		#11 What a pig!!!
	movs.l	%d1,(%a1)+		# 1
Lo40:	movq.l	&0,%d0			# 1 exit d0 == 0
	mov.l	%d0,_u+PROBE_ADDR	# 1 clear the probe error return
	rts

 # 4 long block move loop (alignment doesn't seem necessary for the 040)

Lo4copy_ge_4:
	mov.l	(%a0)+,%d1		#51-67 move 16 bytes 
	movs.l	%d1,(%a1)+		#11 What a pig!!
	mov.l	(%a0)+,%d1		# 1
	movs.l	%d1,(%a1)+		#11 What a pig!!
	mov.l	(%a0)+,%d1		# 1
	movs.l	%d1,(%a1)+		#11 What a pig!!
	mov.l	(%a0)+,%d1		# 1
	movs.l	%d1,(%a1)+		#11 What a pig!!
Lo4copy_entry:
	subq.l	&4,%d0			# 1  check if another loop
	bcc.b	Lo4copy_ge_4		#2/3 loop if more bytes
	jmp	Lo4switch_table+4+4(%pc,%d0.w*2) # 7  move small block


#ifdef GPROF
	global copyout_smallmove	# for profiling only
copyout_smallmove:
#endif
 #
 # err = copyout(src, dest, count);
 #
 # Copy memory from src(KERNEL) to dest(USERland) for count bytes.
 # err == 0 if ok, else == -1 if illegal address in buffers
 #
 # move the least bytes of the count (forward copy)

Lo12:	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#11 What a pig!!!
Lo8:	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#11 What a pig!!!
Lo4:	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#11 What a pig!!!
Lo0:	movq.l	&0,%d0			# 1 exit d0 == 0
	mov.l	%d0,_u+PROBE_ADDR	# 1 clear the probe error return
	rts

Lo13:	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#11 What a pig!!!
Lo9:	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#11 What a pig!!!
Lo5:	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#11 What a pig!!!
Lo1:	mov.b	(%a0),%d0		# 1
	movs.b	%d0,(%a1)		#11 What a pig!!!
	bra.b	Lo0			# 2 clear probe flag and exit

Lo14:	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#11 What a pig!!!
Lo10:	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#11 What a pig!!!
Lo6:	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#11 What a pig!!!
Lo2:	mov.w	(%a0),%d0		# 1
	movs.w	%d0,(%a1)		#11 What a pig!!!
Loswitch_table:
	bra.b	Lo0			# 2 clear probe flag and exit
	bra.b	Lo1			# 2
	bra.b	Lo2
	bra.b	Lo3
	bra.b	Lo4
	bra.b	Lo5
	bra.b	Lo6
	bra.b	Lo7
	bra.b	Lo8
	bra.b	Lo9
	bra.b	Lo10
	bra.b	Lo11
	bra.b	Lo12
	bra.b	Lo13
	bra.b	Lo14
Lo15:	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#11 What a pig!!!
Lo11:	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#11 What a pig!!!
Lo7:	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#11 What a pig!!!
Lo3:	mov.w	(%a0)+,%d0		# 1
	movs.w	%d0,(%a1)+		#11 What a pig!!!
	mov.b	(%a0),%d0		# 1
	movs.b	%d0,(%a1)		#11 What a pig!!!
	bra.b	Lo0			# 2 clear probe flag and exit

 # copyout(source, dest, length) - copy userland to kernel

	global	_copyout,_copyout_tune,_map_copyout

	data
_copyout_tune:	long 512
	text

_copyout:
#ifdef GPROF
	jsr	mcount
#endif
	mov.l	12(%sp),%d1		# 1  d1 = count
copyout_4spec:
	mov.l	4(%sp),%a0		# 1  a0 = src (s1)
	mov.l	8(%sp),%a1		# 1  a1 = dst (s2)

	subi.l	&16,%d1			# 1  check if > threshold
	bcc.b	Locopy_ge_16		#2/3 yes, copy medium block

	mov.l	&Lcopyerr,_u+PROBE_ADDR	# set the probe error return
	jmp	Loswitch_table+16+16(%pc,%d1.w*2) # 7  move small block

 # 16 byte block move loop (alignment doesn't seem necessary for the 040)

Locopy_ge_16:
	cmp.l	%d1,_copyout_tune	#  2 check if big move
	bcs.b	Locopy_loop_fwd0	# greater or = than 1024+16 bytes
	mov.l	&Lcopyerr,_u+PROBE_ADDR	# set the probe error return
  	jmp	_map_copyout		# use page mapping routine

Locopy_loop_fwd0:
	mov.l	&Lcopyerr,_u+PROBE_ADDR	# set the probe error return

Locopy_loop_fwd:
	mov.l	(%a0)+,%d0		#51-67 move 16 bytes 
	movs.l	%d0,(%a1)+		#11 What a pig!!!
	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#11 What a pig!!!
	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#11 What a pig!!!
	mov.l	(%a0)+,%d0		# 1
	movs.l	%d0,(%a1)+		#1-6
	subi.l	&16,%d1			# 1  check if another loop
	bcc.b	Locopy_loop_fwd		#2/3 loop if more bytes

	jmp	Loswitch_table+16+16(%pc,%d1.w*2) #  7 move small block

 # copyin4(source, dest, length_longs) - copy userland to kernel(4 byte aligned)
 # CAUTION: This routine is for small long word moves and moves less than 256k bytes.

 # Copy memory from src(USERland) to dest(KERNEL) for count longs.
 # err == 0 if ok, else == -1 if illegal address in buffers
 #
	global	_copyin4

_copyin4:
	mov.l	4(%sp),%a0		# 1 a0 = src (s1)
	mov.l	8(%sp),%a1		# 1 a1 = dst (s2)
	mov.l	12(%sp),%d0		# 1 d0 = count
	mov.l	&Lcopyerr,_u+PROBE_ADDR	# 2 set the probe error return
	bra.b	Li4copy_entry		# 2 go do it

Li4switch_table:
	bra.b	Li40			# 2 clear probe flag and exit
	bra.b	Li41			# 2
	bra.b	Li42			# 2
Li43:	movs.l	(%a0)+,%d1		#11 What a pig!!!
	mov.l	%d1,(%a1)+		# 1
Li42:	movs.l	(%a0)+,%d1		#11 What a pig!!!
	mov.l	%d1,(%a1)+		# 1
Li41:	movs.l	(%a0)+,%d1		#11 What a pig!!!
	mov.l	%d1,(%a1)+		# 1
Li40:	movq.l	&0,%d0			# 1 exit d0 == 0
	mov.l	%d0,_u+PROBE_ADDR	# 1 clear the probe error return
	rts

 # 4 long block move loop (alignment doesn't seem necessary for the 040)

Li4copy_ge_4:
	movs.l	(%a0)+,%d1		#51-67 move 16 bytes 
	mov.l	%d1,(%a1)+		# 1
	movs.l	(%a0)+,%d1		#11 What a pig!!!
	mov.l	%d1,(%a1)+		# 1
	movs.l	(%a0)+,%d1		#11 What a pig!!!
	mov.l	%d1,(%a1)+		# 1
	movs.l	(%a0)+,%d1		#11 What a pig!!!
	mov.l	%d1,(%a1)+		#1-6
Li4copy_entry:
	subq.l	&4,%d0			# 1  check if another loop
	bcc.b	Li4copy_ge_4		#2/3 loop if more bytes
	jmp	Li4switch_table+4+4(%pc,%d0.w*2) # 7  move small block

#ifdef GPROF
	global copyin_smallmove		# for profiling only
copyin_smallmove:
#endif
 #
 # err = copyin(src, dest, count);
 #
 # Copy memory from src(USERland) to dest(KERNEL) for count bytes.
 # err == 0 if ok, else == -1 if illegal address in buffers
 #
 # move the least bytes of the count (forward copy)

Li12:	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		# 1
Li8:	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		# 1
Li4:	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		# 1
Li0:	movq.l	&0,%d0			# 1 exit d0 == 0
	mov.l	%d0,_u+PROBE_ADDR	# 1 clear the probe error return
	rts

Li13:	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		# 1
Li9:	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		# 1
Li5:	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		# 1
Li1:	movs.b	(%a0),%d0		#11 What a pig!!!
	mov.b	%d0,(%a1)		# 1
	bra.b	Li0			# 2 clear probe flag and exit

Li14:	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		# 1
Li10:	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		# 1
Li6:	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		# 1
Li2:	movs.w	(%a0),%d0		#11 What a pig!!!
	mov.w	%d0,(%a1)		# 1
Liswitch_table:
	bra.b	Li0			# 2 clear probe flag and exit
	bra.b	Li1			# 2
	bra.b	Li2
	bra.b	Li3
	bra.b	Li4
	bra.b	Li5
	bra.b	Li6
	bra.b	Li7
	bra.b	Li8
	bra.b	Li9
	bra.b	Li10
	bra.b	Li11
	bra.b	Li12
	bra.b	Li13
	bra.b	Li14
Li15:	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		# 1
Li11:	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		# 1
Li7:	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		# 1
Li3:	movs.w	(%a0)+,%d0		#11 What a pig!!!
	mov.w	%d0,(%a1)+		# 1
	movs.b	(%a0),%d0		#11 What a pig!!!
	mov.b	%d0,(%a1)		# 1
	bra.b	Li0			# 2 clear probe flag and exit

 # copyin(source, dest, length) - copy userland to kernel

	global	_copyin,_copyin_tune,_map_copyin

	data
_copyin_tune:	long 512
	text

_copyin:
#ifdef GPROF
	jsr	mcount
#endif
	mov.l	12(%sp),%d1		# 1  d1 = count
copyin_4spec:
	mov.l	4(%sp),%a0		# 1  a0 = src (s1)
	mov.l	8(%sp),%a1		# 1  a1 = dst (s2)

	subi.l	&16,%d1			# 1  check if > threshold
	bcc.b	Licopy_ge_16		#2/3 yes, copy medium block

	mov.l	&Lcopyerr,_u+PROBE_ADDR	# set the probe error return
	jmp	Liswitch_table+16+16(%pc,%d1.w*2) # 7  move small block

 # 16 byte block move loop (alignment doesn't seem necessary for the 040)

Licopy_ge_16:
	cmp.l	%d1,_copyin_tune	#  2 check if big move
	bcs.b	Licopy_loop_fwd0	# greater or = than 1024+16 bytes
  	jmp	_map_copyin		# use page mapping routine

Licopy_loop_fwd0:
	mov.l	&Lcopyerr,_u+PROBE_ADDR	# set the probe error return

Licopy_loop_fwd:
	movs.l	(%a0)+,%d0		#51-67 move 16 bytes 
	mov.l	%d0,(%a1)+		# 1
	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		# 1
	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		# 1
	movs.l	(%a0)+,%d0		#11 What a pig!!!
	mov.l	%d0,(%a1)+		#1-6
	subi.l	&16,%d1			# 1  check if another loop
	bcc.b	Licopy_loop_fwd		#2/3 loop if more bytes

	jmp	Liswitch_table+16+16(%pc,%d1.w*2) #  7 move small block

 ###############################################################################
 # non-local goto
 # saves away sfc, dfc, d2-d7/a2-a7, and  pc
 ###############################################################################

	global	_setjmp,_longjmp
	text	
		
_setjmp:		
	mov.l	(%sp),%d1		# get pc
	mov.l	4(%sp),%a0		# get label_t struct pointer
	movm.l	%d1-%d7/%a2-%a7,(%a0)	# save pc and d2-d7, a2-a7
	movq	&0,%d0			#always return a 0
	rts	

	global	_setjmp_min
_setjmp_min:
	mov.l	4(%sp),%a0		# get label_t struct pointer
	mov.l	(%sp),(%a0)		# save the pc
	mov.l	%a6,44(%a0)		# save a6
	mov.l	%sp,48(%a0)		# save sp
	movq	&0,%d0			# always return a 0
	rts	

_longjmp:		
	mov.l	4(%sp),%a0		#pointer to label_t structure
	cmp.l	%a7,48(%a0)		#check if trimming stack!!
	bgt.b	_longjmp1S
	movm.l	(%a0),%d1-%d7/%a2-%a7
	mov.l	%d1,(%sp)		#restore pc of call to setjmp to stack
	movq	&1,%d0			#always return a 1
	rts	

_longjmp1S:	pea	lj1
	jsr	_panic
lj1:	byte	"longjmp",0

	global	_whichqs,_qs
	global	_cnt

	global	_noproc
	comm	_noproc,4
	global	_runrun
	comm	_runrun,4


 ###############################################################################
 # _whichqs tells which of the queues _qs * have processes in them.
 # setrq puts processes into queues, remrq removes them from queues.
 # The running process is on no queue, other processes are on a queue 
 # related to p->p_pri.
 #
 # setrq(p)
 # Call should be made at spl6(), and p->p_stat should be SRUN
 #
 ###############################################################################
	global	_setrq
_setrq:
#ifdef	FSD_KI
	global	_ki_cf,_ki_setrq
	addq.l	&1,_cnt+V_RUNQ	# increment queue length

	tst.b	_ki_cf+KI_SETRQ
	beq	setrq_S0
	jsr	_ki_setrq	# Note: p (procp)  is on already stack 
setrq_S0:
#endif	FSD_KI
	mov.l	4(%sp),%a0
	mov.l	%a2,-(%sp)	#save a2
	tst.l	P_RLINK(%a0)	##firewall -- p->p_rlink must be 0
	beq.b	set1		##
	pea	set3
	jsr	_panic
set1:
	movq	&0,%d0		##get priority
	mov.b	P_PRI(%a0),%d0	##put on queue indexed by p->p_pri
	mov.l	&NQS-1,%d1	#compute # of bit to set in whichqs
	sub.l	%d0,%d1			#(bits numbered right to left)

	mov.l	&_qs,%a1
	lsl.l	&3,%d0		#make it an offset (8 bytes per entry)
	add.l	%d0,%a1
	subq.l	&4,%sp		# push sp long aligned
	mov.w	%sr,(%sp)	# and save the status reg
	mov.w	&HIGH,%sr
	mov.l	4(%a1),%a2	#get pointer to end of queue
	mov.l	%a0,4(%a1)	#fix backward link of header
	mov.l	(%a2),(%a0)	#fix forward link of new element
	mov.l	%a0,(%a2)		#fix forwark link of current last
	mov.l	%a2,4(%a0)	#fix backward link of new element
	lsr.l	&6,%d0		#byte offset of whichqs element, note lsl 3 above
	mov.l	&_whichqs,%a2
	add.l	%d0,%a2		#ptr to element in whichqs
	bset	%d1,(%a2)		#set not empty bit in queue bitmap
	mov.w	(%sp),%sr	# restore status register
	addq.l	&4,%sp		# pop sp long aligned
	mov.l	(%sp)+,%a2	#restore a2
	rts
	
set3:	byte	"setrq",0


 # remrq(p)
 # Call should be made at spl6().

	global	_remrq
_remrq:
#ifdef	FSD_KI
	subq.l	&1,_cnt+V_RUNQ	# decrement queue length
#endif	FSD_KI
	mov.l	4(%sp),%a0
	movq	&0,%d0
	mov.b	P_PRI(%a0),%d0
	mov.l	&NQS-1,%d1	#be consistent with the search order of swtch
	sub.l	%d0,%d1

	subq.l	&4,%sp		# push sp long aligned
	mov.w	%sr,(%sp)	# and save the status reg
	mov.w	&HIGH,%sr


rem1:
	mov.l	4(%a0),%a1
	mov.l	(%a0),(%a1)
	mov.l	(%a0),%a1
	mov.l	4(%a0),4(%a1)
	clr.l	P_RLINK(%a0)	### for firewall checking
	mov.l	&_qs,%a1
	lsl.l	&3,%d0
	add.l	%d0,%a1
	lsr.l	&6,%d0		#byte offset of whichqs element, note lsl 3 above
	mov.l	&_whichqs,%a0
	add.l	%d0,%a0		#address of byte in whichqs
	cmp.l	%a1,(%a1)		#is anyone else left on this queue ?
	bne.b	rem2		   #yes - whichqs is still correct
	bclr	%d1,(%a0)		   #no  - update whichqs
	bne.b	remdone
rempan:	pea	rem3
	jsr	_panic
rem2:
	btst	%d1,(%a0)		#just a consistency check
	beq.b	rempan		#make sure whichqs bit was/is still set
remdone:
	mov.w	(%sp),%sr	# restore status register
	addq.l	&4,%sp		# pop sp long aligned
	rts	

rem3:	byte	"remrq",0

 # Masterpaddr is the p->p_addr of the running process on the master
 # processor.  When a multiprocessor system, the slave processors will have
 # an array of slavepaddr.

	global	_masterpaddr
	data
_masterpaddr:
	long	0

	text

#ifdef	FSD_KI
 # this is a little kludge to pass the sleep caller thru to the  real sleep
	global	_sleep,__sleep
_sleep:
	mov.l	8(%sp),-(%sp)
	mov.l	8(%sp),-(%sp)
	jsr	__sleep			# call the renamed real sleep with caller
	addq.l	&8,%sp			# and pop off arg
	rts

#endif	FSD_KI

 ###############################################################################
 # Context Switch
 ###############################################################################
#ifdef IDLE_CNT
 #
 # Instrumentation for counting iterations in idle loop.  Mostly used
 #  by IND
 #
	data
	global	_idlecnt
_idlecnt:
	long	0		# Least-significant longword
	long	0		#  ...most significant

	text
#endif /* IDLE_CNT */

 # Switch to the highest priority process
 # swtch assumes that _whichqs and t_qs are arrays of integers

swerr:	byte	"swtch",0
	lalign  4

	global	_swtch, IDLE_CPU
_swtch:
#ifdef	FSD_KI
	global	_ki_cf,_ki_swtch,_ki_resume_csw,__swtch

__swtch:
	mov.l	&1,_noproc
IDLE_CPU:
	jsr	_ki_accum_push_TOS_csw	# start with a CSW, then IDLE??
	tst.b	_ki_cf+KI_SWTCH		# check if ki_swtch enabled
	beq.b	sw0c	 		#branch if not
	mov.l	4(%sp),-(%sp)		# push sleep caller on stack
	jsr	_ki_swtch		# Note: PC is on already stack 
	addq.l	&4,%sp			#pop off arg 
sw0c:
	jsr	_ki_accum_push_TOS_idle	#
#endif	FSD_KI
	clr.l	_runrun
	mov.l	%d2,-(%sp)	#d2 holds # of element in whichqs bit array
	mov.l	%a2,-(%sp)	#a2 holds pointer to word in whichqs bit array
sw1:
#ifdef IDLE_CNT
	addq.l	&1,_idlecnt		# Bump idle loop counter
	bne.b	sw1c			#  It is a 64-bit integer counter
	addq.l	&1,_idlecnt+4
sw1c:
#endif /* IDLE_CNT */
	mov.l	&_whichqs,%a2
	mov.l	&NQELS-1,%d2
swlp1:	tst.l	(%a2)+
	dbne	%d2,swlp1
	cmp.w	%d2,&-1
	bne.b	swfound
	jsr	_spl0			# go service the sw_triggers /* XXX */
 	jsr	_do_smart_poll
	bra.w	sw1		#as this is the idle loop
swfound:	mov.l	-(%a2),%d1
	mov.l	&31,%d0
sw2:	btst	%d0,%d1
	dbne	%d0,sw2
	cmp.w	%d0,&-1
	bne.b	sw1a
	mov.w	&LOW,%sr		#allow interrupts
	bra.b	sw1		#as this is the idle loop
sw1a:	mov.w	&HIGH,%sr	#lock out interrupts
	mov.l	(%a2),%d1
	bclr	%d0,%d1
	beq.b	sw1		#process moved by lbolt interrupt
	mov.l	&_qs+(NQS-1)*8,%a1	#address of last entry in qs
	lsl.l	&5,%d2		## of run queue at start of this whichqs element
	add.l	%d0,%d2		## of chosen run queue
	lsl.l	&3,%d2		#make it an offset (8 bytes per entry)
	sub.l	%d2,%a1		#address of chosen run queue
	mov.l	(%a1),%a0		#get first entry
	cmp.l	%a1,%a0		#if they are equal then the queue is empty
	beq.b	sw1b		
	mov.l	(%a0),(%a1)
	mov.l	(%a0),%a1
	mov.l	4(%a0),4(%a1)
	mov.l	4(%a1),%a1
#ifdef	FSD_KI
	subq.l	&1,_cnt+V_RUNQ	# decrement queue length
#endif	FSD_KI
	cmp.l	%a1,(%a1)		#is queue empty now?
	beq.b	sw3		#if so branch
	bset	%d0,%d1		#else set bit indicating that it is not empty
	bra.b	sw3
sw1b:	pea	swerr
	jsr	_panic

sw3:	mov.l	%d1,(%a2)
#ifdef	FSD_KI
	mov.l	%a0,-(%sp)		#save C registers !!!!!!!!!
	jsr	_ki_accum_pop_TOS	# pop the IDLE clock
	tst.b	_ki_cf+KI_RESUME_CSW
	beq	sw3b
	jsr	_ki_resume_csw		# with no parameters
sw3b:
	mov.l	(%sp)+,%a0		# restore C registers !!!!!!!!!
#endif	FSD_KI
	clr.l	_noproc

	cmp.l	%a0,_u+U_PROCP	## switching to current process?
	bne.b	sw4		## if so then can skip resume
	clr.l	P_RLINK(%a0)	##
	mov.l	(%sp)+,%a2
	mov.l	(%sp)+,%d2
#ifdef	FSD_KI
	jsr	_ki_accum_pop_TOS # pop off CSW clock
#endif	FSD_KI
	mov.w	&LOW,%sr
	rts			## skip resume

sw4:	tst.l	P_WCHAN(%a0)	##firewalls
	bne.b	sw1b		##
	movq	&0,%d1		##clear a register
	mov.b	P_STAT(%a0),%d1	##
	cmp.l	%d1,&SRUN	##
	bne.b	sw1b		##
	clr.l	P_RLINK(%a0)	##

	mov.l	(%sp)+,%a2
	mov.l	(%sp)+,%d2

 # Fall through to resume with %a0 containing proc pointer
 # for new process.


	global	_resume
	global	_float_present,_float_base
	global	_dragon_present

 # Note: resume expects a proc pointer passed in via register %a0

_resume:
	addq.l	&1,_cnt+V_SWTCH		# count true context switches
	mov.l   &_u+U_RSAVE,%a1
	mov.l	(%sp),%d1		# get pc 
	movm.l  %d1-%d7/%a2-%a7,(%a1)   # save pc and d2-d7, a2-a7

 #* Save the float registers/status (if float card present)
	tst.l	_float_present		#is float card present?
	beq.w	float_notpresent
	mov.l	&_u+PCB_FLOAT,%a2
	mov.l   _float_base,%a1         #a0=address of float card
	movq	&0,%d0			#prepare for shifting
	mov.w   0x20(%a1),%d0           #get error bit
	lsr.l	&3,%d0			#shift it
	mov.l	%d0,(%a2)+		#save last operation error bit
	mov.l   0x4580(%a1),(%a2)+      #save float status register
	movm.l  0x4550(%a1),%d0-%d7     #f7-f0 => d0-d7
	clr.b   0x21(%a1)               #clear state machine on card
	movm.l	%d0-%d7,(%a2)		#f7-f0 => save area

float_notpresent:
	tst.l	_dragon_present
	beq.w	dragon_skipsave
	mov.l	&_u+PCB_DRAGON_BANK,%a2  # addr of bank in u struct 
	cmp.w	(%a2),&-1		 # is process using dragon
	beq.w	dragon_skipsave		 # 
	mov.l	&_u+PCB_DRAGON_SR,%a2	 # if so get save addr in u for sr
	mov.l	DRAGON_STATUS_ADDR,(%a2) # save status reg.
	mov.l	&_u+PCB_DRAGON_CR,%a2	 # get save addr in u for cr
	mov.l	DRAGON_CNTRL_ADDR,(%a2)	 # save control reg.
dragon_skipsave:

 # worst case mc68881/882 save area:
 #	216 bytes for worst case internal save state
 #	12 bytes for 3 32 bit status/control registers
 #	96 bytes for 8 96 bit registers
 #	324 bytes total	
 #
 # see if there is a 68881 present
 # if so then save the required state
 #

	tst.b	_mc68881		#floating point coprocessor?
	beq.w	mc68881_notpresent1
	mov.l	&_u+PCB_MC68881,%a2
 #	fsave	(a2)			stop the 68881 and save internal state
	short	0xf312
	cmp.b	0(%a2),&0x00		#if null state then don''t save registers
	beq.w	mc68881_notpresent1

 #######################################################################
 # Begin xxD43B Floating Point Workaround
 #
 # This code is copied below, where signal processing can do an fsave
 #######################################################################
	cmp.w	(%a2),&0x4060		
	bne		save_regs_1
	btst.b	&1,0x48(%a2)
	beq		save_regs_1
	move.l  _u+U_PROCP,%a1
	or.l    &S268040_FP,P_FLAG2(%a1)
save_regs_1:
 #######################################################################
 # End xxD43B Floating Point Workaround
 #######################################################################
 #	fmovem	<c/s/i>,216(a2)		save cntrl, status, and instruction regs
	short	0xf22a
	short	0xbc00
	short	0x00d8
 #	fmovem	fp0-fp7,228(a2)		save floating point registers
	short	0xf22a
	short	0xf0ff
	short	0x00e4

mc68881_notpresent1:

 # Move %a0, which contains the proc pointer for the new process
 # to %a2 so we don't need to keep saving and restoring it when we
 # call other procedures.

	mov.l   %a0,%a2

 # Check to see if the stack pointer is currently on a common stack
 # page. If so, call transfer_stack() to give away the used common
 # stack page(s) to the user process. We will replace these pages
 # below.

	mov.l   _u+U_PROCP,%a0
	mov.l   P_STACKPAGES(%a0),%d0   # Get stored # of stack pages (old proc)
	mov.l   %d0,%d2                 # save in %d2 for use later
	movq	&PGSHIFT,%d1
	lsl.l   %d1,%d0                 # Convert to page offset
	mov.l   &KSTACKADDR,%a0
	sub.l   %d0,%a0                 # subtract from bottom of stack
	cmp.l   %sp,%a0                 # Check to see if stack is within bound
	bcc.b   no_transfer
	mov.l   %sp,%d0                 # it's not, so call transfer_stack
	mov.l   %d0,-(%sp)              # with the stack pointer as an
	jsr     _transfer_stack         # argument. We don't need to pop
					# stack since we switch to new stack soon
	mov.l   %d0,%d2                 # transfer_stack returns revised stack_pages.
					# store it in %d2 for use later

no_transfer:
	mov.l   %sp,%d0                 # Save current stack pointer
	mov.l   &_Kstack_toppage+NBPG,%sp # let us run on common top stack page
	mov.l   _u+U_PROCP,%a0
	tst.l   P_VFORKBUF(%a0)         # Check to see if we have any vfork work to do.
	beq.b   no_vfork
	mov.l   %d0,-(%sp)              # We do, so call vfork_transfer
					# with the stack pointer as an
	jsr     _vfork_transfer         # argument. We don't need to pop
					# stack since we switch to new stack soon

no_vfork:
	mov.w	&HIGH,%sr		# no interrupts

	mov.l   &KSTACKADDR,-(%sp)      # push logical address
	mov.l	&_Syssegtab,-(%sp)	# push segment table pointer
	jsr	_itablewalk		# get the pte pointer
	addq.l	&8,%sp			# pop the stack
	mov.l   %d0,%a0                 # %a0: pte past bottom of stack (kernvas)
	mov.l   P_STACKPAGES(%a2),%d1   # Get stored # of stack pages (new proc)
	mov.l   %d1,%d0
	lsl.l   &2,%d0
	sub.l   %d0,%a0                 # %a0: top private stack page pte

 # We need to check if we need to allocate or deallocate some common
 # kernel stack pages. If the number of private pages for both the old
 # and new process are the same we don't need to do anything. If they
 # are not equal then we need to allocate or deallocate common stack pages.

	mov.l   %d1,%d0
	sub.l   %d2,%d0                 # Subtract old pages from new pages
	beq.w   skip_allocate
	bcc.w   deallocate_stack

 # We need to allocate some common stack pages and map them

	neg.l   %d0                     # %d0 contains # of pages to allocate
	mov.l   _kstack_reserve,%d3
	sub.l   %d0,%d3                 # Do we have enough pages in reserve?
	bcc.w   allocate_stack

	pea     no_rsv_err
	jsr     _panic

no_rsv_err:     byte "resume: out of stack reserve",0,0

allocate_stack:
	mov.l   %d3,_kstack_reserve     # update kstack_reserve
	mov.l   &_kstack_rptes,%a3
	lsl.l   &2,%d3
	add.l   %d3,%a3                 # %a3 points to kstack rsv pte.

	mov.l   %a0,%a1
	mov.l   %d0,%d3
	lsl.l   &2,%d3
	sub.l   %d3,%a1                 # %a1:top stack pte to be allocated


asloop:
	mov.l   (%a3)+,(%a1)+
	subq.l  &1,%d0
	bne.b   asloop

	bra.w   skip_allocate

deallocate_stack:
	mov.l   _kstack_reserve,%d3
	mov.l   %d3,%d4
	add.l   %d0,%d4
	mov.l   %d4,_kstack_reserve     # update kstack_reserve

	mov.l   &_kstack_rptes,%a3
	lsl.l   &2,%d3
	add.l   %d3,%a3                 # %a3 points past kstack rsv pte.

	mov.l   %a0,%a1

dsloop:
	mov.l   (%a1)+,(%a3)+
	subq.l  &1,%d0
	bne.b   dsloop

 # We need to return some common stack pages to the reserve pool

skip_allocate:
	mov.l   P_ADDR(%a2),%a1         # Get stack & u ptes (new proc vas)
	mov.l   %d1,%d0
	lsl.l   &2,%d0
	sub.l   %d0,%a1                 # %a1: top stack pte (new proc vas)

	addq.l  &1,%d1                  # add 1 for u pte
	mov.l   (%a1)+,%d0              # Get pte
	btst	&1,%d0			# is it an indirect pte?
	bne.b   indirect_loop

direct_loop:
	and.l   &NOT_PG_PROT,%d0        # clear write protect bit
	or.l    &PG_V,%d0               # make it accessible to kernel
	mov.l   %d0,(%a0)+              # restore into kernel''s pte map
	subq.l  &1,%d1
	beq.w   stackmap_done
	mov.l   (%a1)+,%d0              # Get pte
	bra.b   direct_loop

indirect_loop:
	and.b   &0xfc,%d0               # mask out bottom two bits
	mov.l   %d0,%a3
	mov.l   (%a3),%d0
	and.l   &NOT_PG_PROT,%d0        # clear write protect bit
	or.l    &PG_V,%d0               # make it accessible to kernel
	mov.l   %d0,(%a0)+              # restore into kernel's pte map
	subq.l  &1,%d1
	beq.b   stackmap_done
	mov.l   (%a1)+,%d0              # get indirect pte
	bra.w   indirect_loop

stackmap_done:
	jsr	_purge_tlb

#ifndef NO_68010
	cmp.l   _processor,&M68010      # if not 68010 then purge i-cache
	beq.w	res11
#endif /* not NO_68010 */
	cmp.l	_processor,&M68040	# do 68040 specific stuff
	beq.w	res_68040
	mov.l	&IC_CLR+IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
	long	0x4e7b0002		# movec d0,cacr
	long	0x4e7b0002		# movec d0,cacr

	#purge data cache by turning it off
	and.w	&~(MMU_CEN+MMU_FPE),MMU_BASE+0x0e
	or.w	&(MMU_CEN+MMU_FPE),MMU_BASE+0x0e	#turn it back on
	bra	res11

res_68040:
	# set the user root pointer
	mov.l	_u+U_PROCP,%a1
	mov.l	P_SEGPTR(%a1),%d0
        long    0x4E7B0806	# movec	%d0,CRP

	jsr	_purge_tlb
	bra	pmmuskip9

res11:	
	mov.l	_u+U_PROCP,%a1
	mov.l	P_SEGPTR(%a1),%d0

	tst.l	_pmmu_exist
	beq.w	pmmuskip8
	mov.l	&_eightbytes,%a1
	mov.l	&0x80000002,(%a1)
	mov.l	%d0,4(%a1)
	long	0xf0114c00		#pmov	(%a1),%CRP
	bra.w	pmmuskip88
pmmuskip8:
	movq	&PGSHIFT,%d1
	lsr.l	%d1,%d0
	mov.l	%d0,USEGPTR
pmmuskip88:

	tst.l	_pmmu_exist
	beq.w	pmmuskip9
 #	pflusha				purge the entire tlb
	long	0xf0002400		#revisit for performance
	mov.l	&IC_CLR+IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
	long	0x4e7b0002		# movec d0,cacr

pmmuskip9:
 # End mod for BANANAJR
	tst.b	_mc68881		#floating point coprocessor?
	beq.w	mc68881_notpresent4
	mov.l	&_u+PCB_MC68881,%a1
	cmp.b	0(%a1),&0x00		#if null state then only restore state
	beq.w	mc68881_onlyrestore2
 #	fmovem	216(a1),<c/s/i>		restore control,status,and iaddr regs
	short	0xf229
	short	0x9c00
	short	0x00d8
 #	fmovem	228(a1),<fp0-fp7>	restore floating point registers
	short	0xf229
	short	0xd0ff
	short	0x00e4
mc68881_onlyrestore2:
 #	frestore (a1)			restore the 68881 internal state
	short	0xf351
mc68881_notpresent4:

	tst.l	_float_present		#is float card present?
	beq.w	it_isnt
	mov.l	&_u+PCB_FLOAT,%a1
	mov.l	_float_base,%a0		#a0=address of float card
	movm.l	8(%a1),%d0-%d7		#get float registers
	movm.l	%d0-%d7,0x44e0(%a0)	#restore float registers
	mov.l	4(%a1),0x4540(%a0)	#save float status register
	mov.l	(%a1),%d0			#get last operation error bit
	mov.w	%d0,0x20(%a0)		#restore it
it_isnt:
	tst.l	_dragon_present
	beq.w	dragon_skiprestore
	mov.l	&_u+PCB_DRAGON_BANK,%a1  # addr of bank in u struct 
	cmp.w	(%a1),&-1 		 # is process using dragon
	beq.w	dragon_skiprestore	 # 
	mov.w	(%a1),%d0		 # Get bank number from u struct
	mov.b	%d0,DRAGON_BANK_ADDR	 # restore the bank number
	mov.l	&_u+PCB_DRAGON_SR,%a1	 # get save sr addr in u
	mov.l	(%a1),DRAGON_STATUS_ADDR # restore status reg.
	mov.l	&_u+PCB_DRAGON_CR,%a1	 # get save cr addr in u 
	mov.l	(%a1),DRAGON_CNTRL_ADDR	 # save control reg.
dragon_skiprestore:
	mov.l	&_u+U_RSAVE,%a0
	movm.l	(%a0),%d1-%d7/%a2-%a7	#restore the general purpose registers
	mov.l	%d1,(%sp)		#and put pc on the stack for rts

 # Now that we are just about done, see if we need to replenish
 # the kernel stack pool.

	cmpi.l  _kstack_reserve,&KSTACK_RESERVE
	bcc.b   res2
	jsr     _stack_reserve_alloc

res2:
	mov.l	_u+PCB_SSWAP,%d0	# check if swtching to fork child
	bne.b	res3			# no, continue process
#ifdef	FSD_KI
	jsr	_ki_accum_pop_TOS	# pop off CSW  clocks
	tst.b	_ki_cf+KI_RESUME_CSW
	beq	res3b
	jsr	_ki_resume_csw		# Note: PC is on already stack 
res3b:
#endif	FSD_KI
	mov.w	&LOW,%sr			#and the sr
     	rts
res3:
	mov.l	%d0,-(%a7)
	clr.l	_u+PCB_SSWAP
#ifdef	FSD_KI
	mov.l   _u+U_PROCP,%a0
	tst.l   P_VFORKBUF(%a0)         # Check to see if we are vforked child
	beq.b   no_vfork2
	jsr     _ki_accum_pop_TOS       # Yes, then parent did extra push for us
no_vfork2:
	mov.l	_u+U_PROCP,%a1		# get proc pointer
	mov.l	P_FLAG(%a1),%d0		# get p->p_flag
	and.l	&SSYS,%d0		# check if SSYS process
	beq.b	res3d			# no, normal userland process
	mov.l	&KI_SYS_CLOCK,_u+KI_CLK_BEGINNING_STACK_ADDRS # put sys clock pointer on BOS
	bra.b	res3e
res3d:
	mov.l	&KI_USR_CLOCK,_u+KI_CLK_BEGINNING_STACK_ADDRS # put usr clock pointer on BOS
res3e:
#endif	FSD_KI
	mov.w	&LOW,%sr		#and the sr
	jsr	_longjmp
 #	no return

 #	DB_SENDRECV

	global	_dbresume

_dbresume:
#ifdef	FSD_KI
	jsr	_ki_accum_push_TOS_csw	# accum clocks
#endif	FSD_KI
	mov.l   4(%sp),%a0              # get proc pointer off the stack
	bra	_resume

	global  _get_nstackpages

_get_nstackpages:
	mov.l   &KSTACKADDR,%d0
	sub.l   %sp,%d0
	add.l   &NBPG-5,%d0     # don't count 4 bytes taken by return address
	movq	&PGSHIFT,%d1
	lsr.l	%d1,%d0
	rts

	global _vfork_switchU

_vfork_switchU:
	mov.l   4(%sp),%d1              # get P_ADDR parameter
	mov.l   8(%sp),%d0              # get P_STACKPAGES parameter
	mov.l   (%sp),%a1               # save return address
	mov.l   %sp,%a0                 # save stack pointer
	mov.l   &_Kstack_toppage+NBPG,%sp # let us run on common top stack page
	mov.l   %a0,-(%sp)              # save stack pointer
	mov.l   %a1,-(%sp)              # save return address on temp stack
	mov.l   %a2,-(%sp)              # save %a2
	mov.l   %d2,-(%sp)              # save %d2
	mov.l   %d1,%a2                 # put P_ADDR pointer in %a2
	mov.l   %d0,%d2                 # put P_STACKPAGES in %d2

	# Since vfork_state is VFORK_CHILDEXIT, vfork_transfer()
	# doesn't use it's parameter, so we don't bother pushing one
	# on the stack.

	# Note: After vfork_transfer is called, the U-area should
	# not be referenced since it now contains the parents information

	jsr     _vfork_transfer         # restore parents U-area

	mov.l   &KSTACKADDR,-(%sp)      # push logical address
	mov.l	&_Syssegtab,-(%sp)	# push segment table pointer
	jsr	_itablewalk		# get the pte pointer
	addq.l	&8,%sp			# pop the stack
	mov.l   %d0,%a0                 # %a0: pte past bottom of stack (kernvas)
	mov.l   %d2,%d0                 # Get stack pages
	lsl.l   &2,%d0
	sub.l   %d0,%a0                 # %a0: top private stack page pte

	mov.l   %a2,%a1                 # Get stack & u ptes (new proc vas)
	mov.l   %d2,%d0
	lsl.l   &2,%d0
	sub.l   %d0,%a1                 # %a1: top stack pte (new proc vas)

	addq.l  &1,%d2                  # add 1 for u pte
	mov.l   (%a1)+,%d0              # Get pte
	btst	&1,%d0			# is it an indirect pte?
	bne.b   indirect_switchu

direct_switchu:
	and.l   &NOT_PG_PROT,%d0        # clear write protect bit
	or.l    &PG_V,%d0               # make it accessible to kernel
	mov.l   %d0,(%a0)+              # restore into kernel''s pte map
	subq.l  &1,%d2
	beq.w   switchu_done
	mov.l   (%a1)+,%d0              # Get pte
	bra.b   direct_switchu

indirect_switchu:
	and.b   &0xfc,%d0               # mask out bottom two bits
	mov.l   %d0,%a2
	mov.l   (%a2),%d0
	and.l   &NOT_PG_PROT,%d0        # clear write protect bit
	or.l    &PG_V,%d0               # make it accessible to kernel
	mov.l   %d0,(%a0)+              # restore into kernel's pte map
	subq.l  &1,%d2
	beq.b   switchu_done
	mov.l   (%a1)+,%d0              # get indirect pte
	bra.w   indirect_switchu

switchu_done:
	jsr	_purge_tlb

	# Reset the users root pointer

	cmp.l	_processor,&M68040	# do 68040 specific stuff
	beq.w   switchu_68040
	mov.l	&IC_CLR+IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
	long	0x4e7b0002		# movec d0,cacr
	long	0x4e7b0002		# movec d0,cacr

	#purge data cache by turning it off
	and.w	&~(MMU_CEN+MMU_FPE),MMU_BASE+0x0e
	or.w	&(MMU_CEN+MMU_FPE),MMU_BASE+0x0e	#turn it back on
	bra     switchu_1

switchu_68040:
	# set the user root pointer
	mov.l	_u+U_PROCP,%a1
	mov.l	P_SEGPTR(%a1),%d0
        long    0x4E7B0806	# movec	%d0,CRP

	jsr	_purge_tlb
	bra     switchu_rts

switchu_1:
	mov.l	_u+U_PROCP,%a1
	mov.l	P_SEGPTR(%a1),%d0

	tst.l	_pmmu_exist
	beq.w   switchu_2
	mov.l	&_eightbytes,%a1
	mov.l	&0x80000002,(%a1)
	mov.l	%d0,4(%a1)
	long	0xf0114c00		#pmov	(%a1),%CRP
	bra.w   switchu_3

switchu_2:
	movq	&PGSHIFT,%d1
	lsr.l	%d1,%d0
	mov.l	%d0,USEGPTR

switchu_3:
	tst.l	_pmmu_exist
	beq.w   switchu_rts
 #	pflusha				purge the entire tlb
	long	0xf0002400		#revisit for performance
	mov.l	&IC_CLR+IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
	long	0x4e7b0002		# movec d0,cacr

switchu_rts:
	mov.l   (%sp)+,%d2              # Restore %d2 register
	mov.l   (%sp)+,%a2              # Restore %a2 register
	mov.l   (%sp)+,%d0              # get return address
	mov.l   (%sp),%sp               # move onto new stack
	mov.l   %d0,(%sp)               # restore return address
	rts


 ###############################################################################
 # Fetch user data -- 4 bytes (fuword) or 1 byte (fubyte).
 # If the data is not accessable a -1 is returned else the data 
 # is returned.
 # NOTE: Is not -1 valid data?  Probably need to alter the interface
 #       to include a boolean for status.  But what is good enough for
 #       Berkeley is good enough for HP?  (After all, none of the calling
 #	code checks for errors anyway)
 ###############################################################################

fuierr: clr.l	_u+PROBE_ADDR		#clear probe return
	mov.l	&-1,%d0			#return error
	rts

	global	_fuword,_fuiword
	global	_fubyte,_fuibyte
_fuword:
_fuiword:
	mov.l	&fuierr,_u+PROBE_ADDR	#setup return in case of error
	mov.l	4(%sp),%a0		#get address
	movs.l	(%a0),%d0		#fetch the data
	clr.l	_u+PROBE_ADDR		#clear probe return
	rts

_fubyte:
_fuibyte:
	mov.l	&fuierr,_u+PROBE_ADDR	#setup return in case of error
	mov.l	4(%sp),%a0		#get address
	movq	&0,%d0
	movs.b	(%a0),%d0		#fetch the data
	clr.l	_u+PROBE_ADDR		#clear probe return
	rts


 # Store user data -- 4 bytes (suword) or 1 byte (subyte).
 # If the data is not accessable a -1 is returned else the data 
 # is returned.

	global	_suword,_suiword
	global	_subyte,_suibyte
_suword:
_suiword:
	mov.l	&fuierr,_u+PROBE_ADDR	#setup return in case of error
	mov.l	4(%sp),%a0		#get address
	mov.l	8(%sp),%d1		#get data
	movs.l	%d1,(%a0)		#store the data
	movq	&0,%d0			#initialize success return value
	mov.l	%d0,_u+PROBE_ADDR	#clear probe return
	rts

_subyte:
_suibyte:
	mov.l	&fuierr,_u+PROBE_ADDR	#setup return in case of error
	mov.l	4(%sp),%a0		#get address
	mov.l	8(%sp),%d1		#get data
	movs.b	%d1,(%a0)		#store the data
	movq	&0,%d0			#initialize success return value
	mov.l	%d0,_u+PROBE_ADDR	#clear probe return
	rts



 ###############################################################################
 # Copy 1 relocation unit (NBPG bytes)
 # from user virtual address to physical address
 #	input:  user logical address	(source)
 #		frame number of page    (destination)
 ###############################################################################

copyserr:
	pea	cpyserr
	jsr	_panic

 	global	_copyseg
_copyseg:
	mov.l	&copyserr,_u+PROBE_ADDR		#setup return in case of error

cpys1:	
	mov.l	8(%sp),%d0		#get frame number (destination)
	movq	&PGSHIFT,%d1
	lsl.l	%d1,%d0			#make it an address
	mov.l	%d0,%a1
	mov.l	&NBPG/4,%d0		#copy NBPG/4 wordsworth
	mov.l	4(%sp),%a0		#get source address
	bra.w	cpyseg2
cpyseg1:	
	movs.l	(%a0)+,%d1		#copy 4 bytes per transfer
	mov.l	%d1,(%a1)+
cpyseg2:	
	dbf	%d0,cpyseg1		#done yet?
	clr.l	_u+PROBE_ADDR		#clear probe return
	rts

cpyserr:	byte	"copyseg",0

 # Clearseg
 #	input:	page frame number (NBPG size pages)
 # zero out physical memory
 # specified in relocation units (NBPG bytes)

 	global	_clearseg
_clearseg:
	mov.l	4(%sp),%d0		#get page frame number
	movq	&PGSHIFT,%d1
	lsl.l	%d1,%d0			#make it an address
	mov.l	%d0,%a0
	movq	&0,%d1			# much faster on 68020
	mov.l	&NBPG/4,%d0		#clear NBPG/4 words worth
	bra.w	clrseg2
clrseg1:	
	mov.l	%d1,(%a0)+		#clear 4 bytes per transfer
clrseg2:	
	dbf	%d0,clrseg1		#done yet?
	rts				#yes, return


 # Update save registers
 # Called by panic()
	global _update_rsave
_update_rsave:
	mov.w	%sr,%d0			# return old sr in d0 (low half)
	mov.w	&HIGH,%sr		# protect critical region
	swap	%d0			# position to high bits
	mov.l	&_u+U_RSAVE,%a0
	mov.l	(%sp),%d1		# get pc
	movm.l	%d1-%d7/%a2-%a7,(%a0)
	rts


 ###############################################################################
 #
 #	called from sendsig and newproc
 #		sendsig passes pointer to save area in sigframe
 #		newproc passes pointer to save area in u.u_pcb
 #
 ###############################################################################

 	global	_save_floating_point

_save_floating_point:
 #* if float card present then save its state
	tst.l	_float_present		#is float card present?
	beq.w	no_float
	mov.l	4(%sp),%a1		#get save buffer address
	movm.l	%d2-%d7,-(%sp)		#save some registers
	mov.l	_float_base,%a0		#a0=address of float card
	movq	&0,%d0			#prepare for shifting
	mov.w	0x20(%a0),%d0		#get error bit
	lsr.l	&3,%d0			#shift it
	mov.l	%d0,(%a1)+		#save last operation error bit
	mov.l	0x4580(%a0),(%a1)+	#save float status register
	movm.l	0x4550(%a0),%d0-%d7	#f7-f0 => d0-d7
	clr.b	0x21(%a0)		#clear state machine on card
	movm.l	%d0-%d7,(%a1)		#f7-f0 => save area
	movm.l	(%sp)+,%d2-%d7		#restore registers
no_float:
 # worst case mc68881 save area:
 #	184 bytes for worst case internal save state
 #	12 bytes for 3 32 bit status/control registers
 #	96 bytes for 8 96 bit registers
 #	292 bytes total	
 #
 # see if there is a 68881 present
 # if so then save the required state
 #
	tst.b	_mc68881		#floating point coprocessor?
	beq.w	no_mc68881
	mov.l	4(%sp),%a1		#get save buffer address	
	add.l	&PCB_MC68881-PCB_FLOAT,%a1 #add in offset to 68881 area
 #	fsave	(a1)			stop the 68881 and save internal state
	short	0xf311
	cmp.b	0(%a1),&0x00		#if null state then don''t save registers
	beq.w	no_mc68881

 #######################################################################
 # Begin xxD43B Floating Point Workaround
 #
 # This is copied from above, in the context switcher (_resume)
 #######################################################################
	cmp.w	(%a1),&0x4060		
	bne		save_regs_2
	btst	&1,0x48(%a1)
	beq		save_regs_2
	move.l	_u+U_PROCP,%a0
	or.l	&S268040_FP,P_FLAG2(%a0)
save_regs_2:
 #######################################################################
 # End xxD43B Floating Point Workaround
 #######################################################################

 #	fmovem	<c/s/i>,216(a1)		save control,status,and instruction regs
	short	0xf229
	short	0xbc00
	short	0x00d8
 #	fmovem	fp0-fp7,228(a1)		save floating point registers
	short	0xf229
	short	0xf0ff
	short	0x00e4
no_mc68881:
	rts

 	global	_restore_floating_point
 #
 #	restore_floating_point(&restore_area)
 #	struct full_sigcontext *restore_area;
 #
_restore_floating_point:
	tst.l	_float_present		#is float card present?
	beq.w	_restore_floating_point1S
	mov.l	4(%sp),%a1
	add.l	&SF_FLOAT_REGS,%a1
	movm.l	%d2-%d7,-(%sp)		#save some registers
	mov.l	_float_base,%a0		#a0=address of float card
	movm.l	8(%a1),%d0-%d7		#get float registers
	movm.l	%d0-%d7,0x44e0(%a0)	#restore float registers
	mov.l	4(%a1),0x4540(%a0)	#save float status register
	mov.l	(%a1),%d0			#get last operation error bit
	mov.w	%d0,0x20(%a0)		#restore it
	movm.l	(%sp)+,%d2-%d7		#restore the registers
_restore_floating_point1S:
	tst.b	_mc68881		#floating point coprocessor?
	beq.w	_restore_floating_point3S
 #	fsave	(a1)			stop the 68881
 #	dc.w	0xf311
	mov.l	4(%sp),%a1		#restore the old state
	add.l	&SF_MC68881_REGS,%a1
	cmp.b	0(%a1),&0x00		#if null state then only restore state
	beq.w	_restore_floating_point2S
 #	fmovem	216(a1),<c/s/i>		restore control,status,and iaddr regs
	short	0xf229
	short	0x9c00
	short	0x00d8
 #	fmovem	228(a1),<fp0-fp7>	restore floating point registers
	short	0xf229
	short	0xd0ff
	short	0x00e4
_restore_floating_point2S:
 #	frestore (a1)			restore the 68881 internal state
	short	0xf351
_restore_floating_point3S:
	rts

	global	_mulpc
 #
 # Unsigned 16-bit by 32-bit multiply for addupc (profiling)
 # Returns upper 32 bits of its 48 bit product.
 #
_mulpc:	mov.w	0xa(%a7),%d1	#d1 = scale (16 bit operand)
	mov.w	%d1,%d0		#d0 = scale
	mulu	6(%a7),%d0	#d0 = scale*pclo, unsigned
	clr.w	%d0		#d0 =
	swap	%d0		     #scale*pclo >> 16
	mulu	4(%a7),%d1	#d2 = scale*pchi, unsigned
	add.l	%d1,%d0		#d0 = ((pclo*scale) >> 16) + (pchi*scale)
	rts	


	global _ins, _inl

_inl:
	mova.l	4(%sp), %a0		# get register address argument
	mov.l	(%a0), %d0		# read in register
	rol.w	&8, %d0			# reverse bytes of first word
	swap	%d0			# swap the words 
	rol.w	&8, %d0			# reverse bytes of first (was second) word
	rts

_ins:
	mova.l	4(%sp), %a0		# get register address argument
	mov.l	&0, %d0			# clear register for return
	mov.w	(%a0), %d0		# read in register, need to clear top two bytes?
	rol.w	&8, %d0			# reverse bytes of first word
	rts

	global _rq_empty

_rq_empty:
	mov.l   &_whichqs,%a0
	mov.l   &NQELS-1,%d1
rqe:    tst.l   (%a0)+
	dbne    %d1,rqe
	cmp.w   %d1,&-1
	bne.b   rqe_found
	move.l	&1,%d0
	rts
rqe_found:
	clr.l	%d0
	rts
	
	global _get_fpiar,_get_fpiar_error

_get_fpiar:
	tst.b   _mc68881
	beq.w   no_fpiar

	fsave	-(%sp)
	mov.l   _u+PROBE_ADDR,%a0	# save the current probing address

	# setup return in case of error as the following instruction may fault
	mov.l   &_get_fpiar_error,_u+PROBE_ADDR
	fmove.l %fpiar,%d0
	mov.l   %a0,_u+PROBE_ADDR       # restore probe return
	frestore (%sp)+
	rts

no_fpiar:
	clr.l	%d0
	rts

_get_fpiar_error:
	mov.l   %a0,_u+PROBE_ADDR       # restore probe return
	frestore (%sp)+
	clr.l   %d0                	# return 0 instead of garbage
	rts


 ###############################################################################
 # Logical addresses 0x00000000 - 0x001fffff define kernel code and data.
 # Logical addresses 0x00200000 - 0x007fffff define io space.
 # Logical addresses 0x00800000 - 0x008fffff define old debugger space.
 # Logical addresses 0x00900000 - 0x00ffffff define general mapping space.
 ###############################################################################

		bss
 #
 # Allocate kernel segment table and common kernel stack pages
 #
		align	gap,NBPG

	global _Syssegtab, _Kstack_toppage

_Syssegtab: 	space	NBPG	# kernel segment table

		align	gap2,NBPG

_Kstack_toppage: space   NBPG

	text
	version 2
