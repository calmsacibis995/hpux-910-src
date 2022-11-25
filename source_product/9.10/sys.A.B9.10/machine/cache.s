 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/cache.s,v $
 # $Revision: 1.5.84.3 $	$Author: kcs $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/09/17 21:02:40 $

 # HPUX_ID: @(#)cache.s	55.1		88/12/23 

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
#include "../machine/cpu.h"
#include "../h/errno.h"

 ###############################################################################
 # Cache Management Routines
 ###############################################################################
 	global	_purge_icache
_purge_icache:
	# Purge the internal instruction cache
	# without affecting any other caches

	cmp.l	_processor,&M68040	# have to treat the MC68040 special
	bne.b	purge_icache_now
	nop
	short	0xF4F8	# cpusha IC/DC
	# short	0xF478	# cpusha DC
	rts				# 68040 has physical caches

purge_icache_now:

	# This is an MC68020 or an MC68030.  The MC68020 will ignore the
	# undefined bits (IC_BURST, DC_WA, DC_CLR, DC_ENABLE, DC_BURST).
	# Purge the instruction cache leaving both caches inabled for
	# burst fills and the data cache set for write allocate.

	mov.l	&IC_CLR+IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0

	# movec	d0,cacr
	long	0x4e7b0002

	rts

	global	_purge_dcache_s, _purge_dcache_u, _purge_dcache
_purge_dcache_s:
	cmp.l   _processor,&M68040      # have to treat the MC68040 special
	beq.w   purge_dcache_s_mc68040
	tst.l	_pmmu_exist
	beq.w	notpmmu3
	long	0xf0002400
	# flush on chip data cache
	mov.l	&IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
	long	0x4e7b0002		# movec d0,cacr
	rts
notpmmu3:
	mov.l	CPU_SRP_REG,%d0
	mov.l	%d0,CPU_SRP_REG
purge_dcache_s_mc68040:
	rts

_purge_dcache_u:
	cmp.l   _processor,&M68040      # have to treat the MC68040 special
	beq.w   purge_dcache_u_mc68040
	tst.l	_pmmu_exist
	beq.w	notpmmu4
	long	0xf0002400
	# flush on chip data cache
	mov.l	&IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
	long	0x4e7b0002	# movec d0,cacr
	rts
notpmmu4:
	mov.l	CPU_URP_REG,%d0
	mov.l	%d0,CPU_URP_REG
purge_dcache_u_mc68040:
	rts

_purge_dcache:
	cmp.l   _processor,&M68040      # have to treat the MC68040 special
	beq.w   no_purge
	tst.l	_pmmu_exist
	beq.w	notpmmu5
	long	0xf0002400
	# flush on chip data cache
	mov.l	&IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
	long	0x4e7b0002	# movec d0,cacr
	rts
notpmmu5:
	and.w	&0xfffb,CPU_STATUS_REG 
	or.w	&0x0004,CPU_STATUS_REG 
no_purge:
	rts


purge_dcache_mc68040:
	nop
	short	0xF4F8	# cpusha IC/DC
	# short	0xF478	# cpusha DC
        rts

	global	_purge_dcache_physical

_purge_dcache_physical:
	cmp.l   _processor,&M68040      # have to treat the MC68040 special
	beq.w   purge_dcache_mc68040
	and.w	&0xfffb,CPU_STATUS_REG 
	or.w	&0x0004,CPU_STATUS_REG 
        rts

	global _push_cached_page

_push_cached_page:                      # MC68040 only
	mov.l   4(%sp),%a0              #get page physical address
	nop
	short   0xF470  # cpushp DC,%a0
	rts

	# CACHECTL
	#
	# This routine is a fast path cache invalidate/push for user
	# processes. From user land the user can either call this
	# via either cachectl(3C) or issuing a trap 12 in assembly
	# language, which would save the stack pushes. The parameters
	# are as follows (as documented on the cachectl(3C) man page).
	#
	#   %d0 = opmask
	#   %a1 = address
	#   %d1 = length
	#
	# We only look at address and length on MC68040 processors
	#
	# There is some duplication of code here in order to avoid
	# subroutine calls.

	# Commands:

#define CC_PURGE_BIT  0
#define CC_FLUSH_BIT  1
#define CC_IPURGE_BIT 2

	# Flags:

#define CC_EXTPURGE_BIT 31

	global _cachectl

_cachectl:
	btst &CC_IPURGE_BIT,%d0     # most common reason to call cachectl
	bne.w cc_ipurge
	btst &CC_FLUSH_BIT,%d0      # second most common reason
	bne.w cc_flush
	btst &CC_PURGE_BIT,%d0
	bne.w cc_purge
	btst &CC_EXTPURGE_BIT,%d0
	bne.w cc_extpurge
	movq &EINVAL,%d0            # set return value
	rte

cc_ipurge:
	cmp.l   _processor,&M68040      # have to treat the MC68040 special
	beq.w   cc_ipurge_mc68040

	# CC_IPURGE only purges the instruction cache
	# if we don't have a copyback cache.

	# This is an MC68020 or an MC68030.  The MC68020 will ignore the
	# undefined bits (IC_BURST, DC_WA, DC_CLR, DC_ENABLE, DC_BURST).
	# Purge the instruction cache leaving both caches enabled for
	# burst fills and the data cache set for write allocate.

	mov.l   &IC_CLR+IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d1

	# movec d1,cacr
	long    0x4e7b1002
	bra.w   test_extpurge

cc_ipurge_mc68040:
	cmpi.l  %d1,&16             # check for possible line push
	beq.w   cc_ipurge_line
	cmpi.l  %d1,&4096           # check for possible page push
	beq.w   cc_ipurge_page

	# Flush it all!

cc_ipurge_all:
	nop                         # necessary due to 040 chip bug
	short   0xF4F8              # cpusha IC/DC
	bra.w   test_extpurge

cc_ipurge_line:
	mov.l   %a1,%d1             # test for null address
	beq.w   cc_ipurge_all

	# push the line

	jsr     cc_get_addr         # %a1 has vaddr
	beq.w   cc_ipurge_all
	nop                         # necessary due to 040 chip bug
	short   0xF4E8              # cpushl IC/DC,%a0
	bra.w   test_extpurge

cc_ipurge_page:
	mov.l   %a1,%d1             # test for null address
	beq.w   cc_ipurge_all

	# push the page

	jsr     cc_get_addr         # %a1 has vaddr
	beq.w   cc_ipurge_all
	nop                         # necessary due to 040 chip bug
	short   0xF4F0              # cpushp IC/DC,%a0
	bra.w   test_extpurge

cc_flush:
	cmp.l   _processor,&M68040      # have to treat the MC68040 special
	bne.w   cc_purge_not_mc68040    # flush is purge if not mc68040
	cmpi.l  %d1,&16                 # check for possible line push
	beq.w   cc_flush_line
	cmpi.l  %d1,&4096               # check for possible page push
	beq.w   cc_flush_page

	# Flush it all!

cc_flush_all:
	nop                         # necessary due to 040 chip bug
	short   0xF478              # cpusha DC
	bra.w   test_extpurge

cc_flush_line:
	mov.l   %a1,%d1             # test for null address
	beq.w   cc_flush_all

	# push the line

	jsr     cc_get_addr         # %a1 has vaddr
	beq.w   cc_flush_all
	nop                         # necessary due to 040 chip bug
	short   0xF468              # cpushl DC,%a0
	bra.w   test_extpurge

cc_flush_page:
	mov.l   %a1,%d1             # test for null address
	beq.w   cc_flush_all

	# push the page

	jsr     cc_get_addr         # %a1 has vaddr
	beq.w   cc_flush_all
	nop                         # necessary due to 040 chip bug
	short   0xF470              # cpushp DC,%a0
	bra.w   test_extpurge

cc_purge:
	cmp.l   _processor,&M68040  # have to treat the MC68040 special
	beq.w   cc_purge_mc68040

cc_purge_not_mc68040:
	tst.l   _pmmu_exist
	beq.w   cc_1
	long    0xf0002400          # pflusha (do we need this?)
	# flush on chip data cache
	mov.l   &IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d1
	long    0x4e7b1002          # movec d1,cacr
	bra.w   test_extpurge

cc_1:
	mov.l   CPU_URP_REG,%d1
	mov.l   %d1,CPU_URP_REG
	bra.w   test_extpurge

cc_purge_mc68040:
	cmpi.l  %d1,&16             # check for possible line invalidate
	beq.w   cc_purge_line
	cmpi.l  %d1,&4096           # check for possible page invalidate
	beq.w   cc_purge_page

	# purge it all!

cc_purge_all:
	nop                         # necessary due to 040 chip bug
	short   0xF478              # cpusha DC (use cpusha instead of cinva
				    # since if we allowed cinva it would
				    # probably crash the system
	bra.w   test_extpurge

cc_purge_line:
	mov.l   %a1,%d1             # test for null address
	beq.w   cc_purge_all

	# purge the line

	jsr     cc_get_addr         # %a1 has vaddr
	beq.w   cc_purge_all
	short   0xF448              # cinvl DC,%a0
	bra.w   test_extpurge

cc_purge_page:
	mov.l   %a1,%d1             # test for null address
	beq.w   cc_purge_all

	# purge the page

	jsr     cc_get_addr         # %a1 has vaddr
	beq.w   cc_purge_all
	nop                         # necessary due to 040 chip bug
	short   0xF450              # cinvp DC,%a0

	# intentional fall through to test_extpurge

test_extpurge:
	btst &CC_EXTPURGE_BIT,%d0   # test for external cache purge
	beq.w   cc_done

cc_extpurge:
	and.w   &0xfffb,CPU_STATUS_REG
	or.w    &0x0004,CPU_STATUS_REG

cc_done:
	movq    &0,%d0              # indicate success by clearing %d0
	rte

	# cc_get_addr takes a virtual address and walks the page tables
	# for the current process in order to compute the physical address.
	# The condition code Z bit determines whether or not there is a
	# good physical address in register %a0 upon return. If the input
	# virtual address was valid then the physical address will be in
	# register %a0 so that it can be used for a cpush or cinv.
	# Note: cachectl does not currently save %a1, since this routine
	#       does not change it. It assumes %d0, %d1 and %a0 are available
	#       for scratch use. These semantics are tuned according to the
	#       way cc_get_addr is called.

	# Input Parameter:
	#   a1 = logical start address
	#
	# Output Parameter:
	#   a0 = physical address corresponding to input virtual address

cc_get_addr:
	# Get address of segment table in %a0

	mov.l   _u+U_PROCP,%a0
	mov.l   P_SEGPTR(%a0),%a0

	# calculate the segment table offset

        mov.l   &SG3_ISHIFT-LPTESIZE,%d1
	mov.l	%a1,%d0			# logical start address
	and.l   &SG3_IMASK,%d0		# mask off segent table index bits
        lsr.l   %d1,%d0                 # has segment table offset

	# get the appropriate segment table entry
	add.l	%d0,%a0			# index into segment table
	mov.l	(%a0),%d0		# get ste
	beq.w   cc_ga_bad               # quit if NULL

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
	beq.w   cc_ga_bad               # quit if NULL

	and.l	&BLK_FRAME,%d0		# d0 has page table address
	mov.l	%d0,%a0			# page table address

        # calculate the page table offset
	mov.l	%a1,%d0			# logical start address
        and.l   &SG3_PMASK,%d0          # mask off page table index
        mov.l   &SG_PSHIFT-LPTESIZE,%d1
        lsr.l   %d1,%d0                 # has page table offset
	adda.l  %d0,%a0                 # a0 now has pte pointer
	mov.l   (%a0),%d0               # get pte
	btst    &0,%d0                  # check valid bit
	beq.w   cc_ga_bad               # quit if not valid

	and.l   &SG_FRAME,%d0           # keep frame address part
	mov.l   %d0,%a0                 # store it in %a0
	mov.l   %a1,%d0                 # get original logical address
	and.l   &~SG_FRAME,%d0          # mask out offset
	adda.l  %d0,%a0                 # create physical address for push/invalidate
	mov.l   %a0,%d1                 # set Z bit appropriately
	rts

cc_ga_bad:
	movq    &0,%d1                  # set Z bit to indicate bad address
	rts
