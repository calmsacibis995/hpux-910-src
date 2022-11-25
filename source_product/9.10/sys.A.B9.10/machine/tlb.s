 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/tlb.s,v $
 # $Revision: 1.4.84.3 $	$Author: kcs $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/09/17 21:08:31 $

 # HPUX_ID: @(#)tlb.s	55.1		88/12/23 

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

 ###############################################################################
 # TLB Management Routines
 ###############################################################################
 # Start mod for BANANAJR
	global	_purge_tlb_select

_purge_tlb_select:
	mov.l	4(%sp),%a0		#get page logical address
	cmp.l   _processor,&M68040      # have to treat the MC68040 special
	beq.w   purge_tlb_select_mc68040
	tst.l	_pmmu_exist
	beq.w	purge_tlb_selecta
 #	pflusha				purge the entire tlb
	# MC68030 only has a 22 entry ATC so just flush the whole thing
	long	0xf0002400		#revisit for performance
	# flush on chip data cache
	mov.l	&IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
	long	0x4e7b0002		# movec d0,cacr
	bra.w	purge_tlb_selectb
purge_tlb_selecta:
	movc	%dfc,%d1		 	#save old dfc
	movq	&3,%d0			#setup dfc to
	movc	%d0,%dfc			#access purge space
	movq	&0,%d0			#zero means purge
 	movs.l	%d0,(%a0)
	movc	%d1,%dfc			#restore dfc
purge_tlb_selectb:
	rts
 # End mod for BANANAJR

purge_tlb_select_mc68040:

	# flush entry from both user and supervisory TLBs

	# pflush	(%a0)	# blast the user entry
	short	0xF508

	# set the destination function code register up for a pflush 
	# destination function code = 5 means flush supervisory entries
	movc	%dfc,%d1	# save old dfc
	mov.l	&5,%d0
	movc	%d0,%dfc
	
	# pflush (%a0)		# blast the supervisory entry
	short	0xF508

	# put back the dfc
	movc	%d1,%dfc

	# The MC68040 has physical caches so we dont have
	# to invalidate corresponding cache entries

	rts


	global	_purge_tlb_select_user

_purge_tlb_select_user:
	mov.l	4(%sp),%a0		# get page logical address

	cmp.l	_processor,&M68040	# have to treat the MC68040 special
	beq.w	purge_tlb_select_mc68040_user

	tst.l	_pmmu_exist
	beq.w	purge_tlb_selecta_user

	# MC68030 only has a 22 entry ATC so just flush the whole thing

	# pflusha			# purge the entire tlb
	long	0xf0002400		#revisit for performance

	# flush on chip data cache
	mov.l	&IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0

	# movec	d0,cacr
	long	0x4e7b0002

	bra.w	purge_tlb_selectb_user

purge_tlb_selecta_user:
	movc	%dfc,%d1	 	# save old dfc
	movq	&3,%d0			# setup dfc to
	movc	%d0,%dfc		# access purge space
	movq	&0,%d0			# zero means purge
 	movs.l	%d0,(%a0)
	movc	%d1,%dfc		# restore dfc

purge_tlb_selectb_user:
	rts

purge_tlb_select_mc68040_user:
	# pflush	(%a0)	# blast the user entry
	short	0xF508

	# The MC68040 has physical caches so we dont have
	# to invalidate corresponding cache entries

	rts

	global	_purge_tlb_select_super

_purge_tlb_select_super:
	mov.l	4(%sp),%a0		#get page logical address

	cmp.l	_processor,&M68040	# have to treat the MC68040 special
	beq.w	purge_tlb_select_mc68040_super

	tst.l	_pmmu_exist
	beq.w	purge_tlb_selecta_super

	# MC68030 only has a 22 entry ATC so just flush the whole thing

	# pflusha			# purge the entire tlb
	long	0xf0002400		# revisit for performance

	# flush on chip data cache
	mov.l	&IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0

	# movec	d0,cacr
	long	0x4e7b0002

	bra.w	purge_tlb_selectb_super

purge_tlb_selecta_super:
	movc	%dfc,%d1	 	# save old dfc
	movq	&3,%d0			# setup dfc to
	movc	%d0,%dfc		# access purge space
	movq	&0,%d0			# zero means purge
 	movs.l	%d0,(%a0)
	movc	%d1,%dfc		# restore dfc

purge_tlb_selectb_super:
	rts

purge_tlb_select_mc68040_super:

	# flush entry from supervisory TLB

	# set the destination function code register up for a pflush 
	# destination function code = 5 means flush supervisory entries
	movc	%dfc,%d1	# save old dfc
	mov.l	&5,%d0
	movc	%d0,%dfc
	
	# pflush (%a0)		# blast the TLB
	short	0xF508

	# put back the dfc
	movc	%d1,%dfc

	# The MC68040 has physical caches so we dont have
	# to invalidate corresponding cache entries

	rts


	global	_purge_tlb
_purge_tlb:
	cmp.l   _processor,&M68040      # have to treat the MC68040 special
	beq.w   purge_tlb_mc68040
	tst.l	_pmmu_exist
	beq.w	notpmmu
	long	0xf0002400	# pflush a (purge all entires in tlb)
	# flush on chip data and intruction caches
	mov.l	&IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
	long	0x4e7b0002	# movec	d0,cacr
	rts
notpmmu:
	mov.w	CPU_TLBPURGE_REG,-(%sp)
	addq.l	&2,%sp
	rts
purge_tlb_mc68040:
	# flush all entries from both user and supervisory TLBs

	# pflusha		# blast the TLB
	short	0xF518

	# The MC68040 has physical caches so we dont have
	# to invalidate corresponding cache entries

	rts
	
	global	_purge_tlb_user, _purge_tlb_super
_purge_tlb_user:
	cmp.l   _processor,&M68040      # have to treat the MC68040 special
	beq.w   purge_tlb_user_mc68040
	tst.l	_pmmu_exist
	beq.w	notpmmu1
	long	0xf0002400
	# flush on chip data cache
	mov.l	&IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
	long	0x4e7b0002	# movec d0,cacr
	rts
notpmmu1:
	movq	&0,%d0 
	mov.w	%d0,CPU_TLBPURGE_REG
	rts
purge_tlb_user_mc68040:

	# flush all entries from the user TLB

	# set the destination function code register up for a pflush 
	# destination function code = 1 means flush user entries
	movc	%dfc,%d1 	# save old dfc
	mov.l	&1,%d0
	movc	%d0,%dfc
	
	# pflusha		# blast the TLB
	short	0xF518

	# put back the dfc
	movc	%d1,%dfc

	# The MC68040 has physical caches so we dont have
	# to invalidate corresponding cache entries

	rts

_purge_tlb_super:
	cmp.l   _processor,&M68040      # have to treat the MC68040 special
	beq.w   purge_tlb_super_mc68040
	tst.l	_pmmu_exist
	beq.w	notpmmu2
	long	0xf0002400
	# flush on chip data cache
	mov.l	&IC_ENABLE+IC_BURST+DC_WA+DC_CLR+DC_ENABLE+DC_BURST,%d0
	long	0x4e7b0002		# movec	d0,cacr
	rts
notpmmu2:
	mov.l	&0x8000,%d0 
	mov.w	%d0,CPU_TLBPURGE_REG
	rts
purge_tlb_super_mc68040:

	# flush all entries from the supervisory TLB

	# set the destination function code register up for a pflush 
	# destination function code = 5 means flush supervisory entries
	movc	%dfc,%d1	# save old dfc
	mov.l	&5,%d0
	movc	%d0,%dfc
	
	# pflusha		# blast the TLB
	short	0xF518

	# put back the dfc
	movc	%d1,%dfc

	# The MC68040 has physical caches so we dont have
	# to invalidate corresponding cache entries

	rts

