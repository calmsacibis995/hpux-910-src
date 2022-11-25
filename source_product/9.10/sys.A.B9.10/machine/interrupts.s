 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/interrupts.s,v $
 # $Revision: 1.7.84.5 $	$Author: drew $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/10/27 12:13:38 $

 # HPUX_ID: @(#)interrupts.s	55.1		88/12/23

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

                set LOW,0x2000          # interrupt level 0
                set HIGH,0x2600         # total disable

 ###############################################################################
 # Critical Section Management Routines
 ###############################################################################

 #	xsr = CRIT();   Should this be level 6 or 7 ???????????
	global	_CRIT
_CRIT:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	mov.w	%sr,%d0			# return old sr in d0 (low half)
	mov.w	&HIGH,%sr		# protect critical region
	rts

 #	UNCRIT(xsr);
	global	_UNCRIT
_UNCRIT:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
#ifdef OSDEBUG
	mov.w	&0xd8e0,%d0		# bit mask of bits that should be zero
	and.w	6(%sp),%d0		# mask off new passed interrupt level
	beq.b	crit_ok			# branch if all expected bits are zero
	mov.l	&crit_is_bad,-(%sp)	# push panic message address
	jsr	_panic			# !@#$%^&
crit_ok:
#endif /* OSDEBUG */
	tst.l	_sw_queuehead		#any pending software triggers?
	beq.b	UNCRIT0			#branch if not

	movq	&7,%d0			# mask to just int level
	and.b	6(%sp),%d0		# get passed interrupt level
	mov.l	%d0,-(%sp)		# push it
	mov.w	&HIGH,%sr		# protect critical region
	bsr	sw_service		#service any pending software triggers
	addq.l	&4,%sp			# pop off parameter
UNCRIT0:
	mov.w	6(%sp),%sr		# get new passed interrupt level
	rts
#ifdef GPROF
        nop                             # to aviod gprof hash
        nop                             #   collision
 # The next two ruotines are used by mcount so we can profile spl7
 # and splx

        global  _mcount_CRIT
_mcount_CRIT:
        mov.w   %sr,%d0                 #return old sr in d0
        mov.w   &0x2700,%sr             # set new int level to 7
        rts

        global  _mcount_UNCRIT
_mcount_UNCRIT:
        mov.w   6(%sp),%sr              # restore passed int level
        rts
        nop                             # to avoid gprof hash bucket
        nop                             #   collision
#endif /* GPROF */

    	set HIGH5,0x2500	#disable at 5
	global	_CRIT5
_CRIT5:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	mov.w	%sr,%d0			# return old sr in d0 (low half)
	mov.w	&HIGH5,%sr		# protect critical region
	rts

    	set HIGH4,0x2400	#disable at 4
	global	_CRIT4
_CRIT4:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	mov.w	%sr,%d0			# return old sr in d0 (low half)
	mov.w	&HIGH4,%sr		# protect critical region
	rts

    	set HIGH3,0x2300	#disable at 3
	global	_CRIT3
_CRIT3:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	mov.w	%sr,%d0			# return old sr in d0 (low half)
	mov.w	&HIGH3,%sr		# protect critical region
	rts

    	set HIGH2,0x2200	#disable at 2
	global	_CRIT2
_CRIT2:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	mov.w	%sr,%d0			# return old sr in d0 (low half)
	mov.w	&HIGH2,%sr		# protect critical region
	rts

    	set HIGH1,0x2100	#disable at 1
	global	_CRIT1
_CRIT1:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	mov.w	%sr,%d0			# return old sr in d0 (low half)
	mov.w	&HIGH1,%sr		# protect critical region
	rts


 #	x = spl7();
	global	_spl7
_spl7:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	movq	&7,%d0			# get interrupt level mask
	mov.w	%sr,%d1			# get current int level
	lsr.w	&8,%d1			# position to lo byte
	and.w	%d1,%d0			# mask to just old int level
	mov.w	&0x2700,%sr		# set new int level 7
	rts				# exit d0 == old int value

 #	x = spl6();	 It is ILLEGAL to put stuff on sw_queue at level 7
	global	_spl6
_spl6:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	movq	&7,%d0			# get interrupt level mask
	mov.w	%sr,%d1			# get current int level
	lsr.w	&8,%d1			# position to lo byte
	and.w	%d1,%d0			# mask to just old int level
	mov.w	&0x2600,%sr		# set new int level 6
	rts				# exit d0 == old int value

	data
no_vect_panic_msg:	# sorry time to die
	byte	"Interrupt to uninitialized vector",0

#ifdef OSDEBUG
splx_is_bad:
	byte	"splx: Caller passed bad interrupt value",0
crit_is_bad:
	byte	"crit: Caller passed bad sr value",0
#endif /* OSDEBUG */
	lalign	4
	text

 #	x = splx(y);
	global	_splx
_splx:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
#ifdef OSDEBUG
	mov.l	&0xfffffff8,%d0		# bit mask of bits that should be zero
	and.l	4(%sp),%d0		# mask off new passed interrupt level
	beq.b	splx_ok			# branch if all expected bits are zero
	mov.l	&splx_is_bad,-(%sp)	# push panic message address
	jsr	_panic			# !@#$%^&
splx_ok:
#endif /* OSDEBUG */
	movq	&7,%d0			# get interrupt level mask
	mov.w	%sr,%d1			# get current int level
	lsr.w	&8,%d1			# position to lo byte
	and.w	%d1,%d0			# mask to just old int level
	mov.l	4(%sp),%d1		# get new passed interrupt level
	tst.l	_sw_queuehead		# anybody in sw_queue?
	bne.b	spl_common		# if no queue, then
	ori.b	&0x20,%d1		# merge in the system mode bit
	lsl.w	&8,%d1			# position to 2nd byte
	mov.w	%d1,%sr			# set new int level
	rts				# and exit d0 == old level

#ifdef GPROF
        nop                             # to aviod gprof hash bucket
        nop                             #   collision
#endif

	global	spl_common		# for profiler only!!!!!!
spl_common:
	mov.w	&HIGH,%sr		# protect critical region
	mov.l	%d0,-(%sp)		# save old interrupt level
	mov.l	%d1,-(%sp)		# save new interrupt level, pass new
	bsr	sw_service		# service pending sw triggers
	mov.l	(%sp)+,%d1		# restore new interrupt level
	mov.l	(%sp)+,%d0		# restore old interrupt level for pass
	ori.b	&0x20,%d1		# merge in the system mode bit
	lsl.w	&8,%d1			# position to 2nd byte
	mov.w	%d1,%sr			# set new int level
	rts				# and exit d0 == old level

 #	x = spl5();
	global	_spl5
	global	_splimp
_splimp:
_spl5:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	movq	&7,%d0			# get interrupt level mask
	mov.w	%sr,%d1			# get current int level
	lsr.w	&8,%d1			# position to lo byte
	and.w	%d1,%d0			# mask to just old int level
	movq	&5,%d1			# get new int level
	cmp.b	%d1,%d0			# compare with old level
	bge.b	spl5_0			# if going down, check sw_queue
	tst.l	_sw_queuehead		# anybody in sw_queue?
	bne.w	spl_common		# if no queue, then
spl5_0:
	mov.w	&0x2500,%sr		# set new int level 5
	rts				# exit d0 == old int value

 #	x = spl4();
	global	_spl4
_spl4:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	movq	&7,%d0			# get interrupt level mask
	mov.w	%sr,%d1			# get current int level
	lsr.w	&8,%d1			# position to lo byte
	and.w	%d1,%d0			# mask to just old int level
	movq	&4,%d1			# get new int level
	cmp.b	%d1,%d0			# compare with old level
	bge.b	spl4_0			# if going down, check sw_queue
	tst.l	_sw_queuehead		# anybody in sw_queue?
	bne.w	spl_common		# if no queue, then
spl4_0:
	mov.w	&0x2400,%sr		# set new int level 4
	rts				# exit d0 == old int value

 #	x = spl3();
	global	_spl3
_spl3:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	movq	&7,%d0			# get interrupt level mask
	mov.w	%sr,%d1			# get current int level
	lsr.w	&8,%d1			# position to lo byte
	and.w	%d1,%d0			# mask to just old int level
	movq	&3,%d1			# get new int level
	cmp.b	%d1,%d0			# compare with old level
	bge.b	spl3_0			# if going down, check sw_queue
	tst.l	_sw_queuehead		# anybody in sw_queue?
	bne.w	spl_common		# if no queue, then
spl3_0:
	mov.w	&0x2300,%sr		# set new int level 3
	rts				# exit d0 == old int value

 #	x = spl2();
	global	_spl2
	global	_splnet
_splnet:
_spl2:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	movq	&7,%d0			# get interrupt level mask
	mov.w	%sr,%d1			# get current int level
	lsr.w	&8,%d1			# position to lo byte
	and.w	%d1,%d0			# mask to just old int level
	movq	&2,%d1			# get new int level
	cmp.b	%d1,%d0			# compare with old level
	bge.b	spl2_0			# if going down, check sw_queue
	tst.l	_sw_queuehead		# anybody in sw_queue?
	bne.w	spl_common		# if no queue, then
spl2_0:
	mov.w	&0x2200,%sr		# set new int level 2
	rts				# exit d0 == old int value

 #	x = spl1();
	global	_spl1
_spl1:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	movq	&7,%d0			# get interrupt level mask
	mov.w	%sr,%d1			# get current int level
	lsr.w	&8,%d1			# position to lo byte
	and.w	%d1,%d0			# mask to just old int level
	movq	&1,%d1			# get new int level
	cmp.b	%d1,%d0			# compare with old level
	bge.b	spl1_0			# if going down, check sw_queue
	tst.l	_sw_queuehead		# anybody in sw_queue?
	bne.w	spl_common		# if no queue, then
spl1_0:
	mov.w	&0x2100,%sr		# set new int level 1
	rts				# exit d0 == old int value

 #	x = spl0();
	global	_spl0
_spl0:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	movq	&7,%d0			# get interrupt level mask
	mov.w	%sr,%d1			# get current int level
	lsr.w	&8,%d1			# position to lo byte
	and.w	%d1,%d0			# mask to just old int level
	movq	&0,%d1			# get new int level
	tst.l	_sw_queuehead		# anybody in sw_queue?
	bne.w	spl_common		# if no queue, then
	mov.w	&0x2000,%sr		# set new int level 0
	rts				# exit d0 == old int value

	global	_splsx

 #
 # old_level = splsx(new_level)
 #	Switch to a new software sub-level for software trigger locking
 #	purposes.
 #
_splsx:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
	mov.l	_sw_level,%d0		#return current sub-level
	mov.l	4(%sp),%d1		#new sub-level
	mov.l	%d1,_sw_level		#set new sub-level
	tst.l	_sw_queuehead		#any pending software triggers?
	beq.w	_splsxret		#branch if not
	cmp.l	%d1,%d0			#did sub-level drop?
	bge.w	_splsxret		#branch if not

	mov.l	%d0,-(%sp)		#Save return value
	subq.l	&4,%sp			#allocate stack space (long alligned)
	mov	%sr,%d0			#copy sr to d0 (to pass to sw_service)
	mov	%sr,(%sp)		#copy sr to stack (save across call)
	lsr.w	&8,%d0			#shift IPL to lsb
	andi.l	&7,%d0			#mask off extra bits
	mov.l	%d0,-(%sp)		#pass it
	mov	&HIGH,%sr		#level must be HIGH for sw_service
	bsr	sw_service		#service any pending software triggers
	addq.l	&4,%sp			#pop parameter
	mov	(%sp),%sr			#restore previous sr
	addq.l	&4,%sp			#deallocate stack space
	mov.l	(%sp)+,%d0		#Restore return value
_splsxret:
	rts


 ###############################################################################
 # Interrupt Service
 #
 # Interrupt levels 1 - 6 come to one of the following levXint entry points.
 # There is a linked list of interrupt service routines on each of the levels.
 # The list is serviced in fifo order -- normally the system isrs are first
 # in the list.  An isr will be selected if the value stored in the list
 # structure equals the contents of the card register anded with a saved mask
 # value for the card.  See the isrlink and isrunlink routines in machdep.c.
 # The selected isr can choose not to service the interrupt.  It will then be
 # passed on (chained) to the next selected isr in the list.
 # If there are no isrs for the interrupt level, or if no isr cares to service
 # the interrupt, then it is dismissed (ignored).
 #
 # interrupt level offsets into the table of linked isr lists (rupttable)
 # see machdep.c
 ###############################################################################

	global  _rupttable
	global  _interrupt,_Interrupt
	global	_intr_vect_table
	global	_intr_vect_dio
	global	_intr_vect_vme
	global	_intr_vect_eisa
	global	_dio_IRQ
	data
	lalign	4
_interrupt:	long 0		# interrupt nest counter

_intr_vect_table:
 	space	4*(24)		# vector numbs 0-23 are not interrupt vectors
_intr_vect_dio:
	long	0		# spurious interrupt at level 0
	long	_dio_IRQ	# level 1 dio interrupt
	long	_dio_IRQ	# level 2 dio interrupt
	long	_dio_IRQ	# level 3 dio interrupt
	long	_dio_IRQ	# level 4 dio interrupt
	long	_dio_IRQ	# level 5 dio interrupt
	long	_dio_IRQ	# level 6 dio interrupt
	long	0		# level 7 cant happen interrupt

 	space	4*(32)		# vector numbs 32-63 are not interrupt vectors

 # Room for the VME interrupt ISRs - up to 64

_intr_vect_vme:
 	space	4*(64)		#

 #	16 vectors for EISA
_intr_vect_eisa:
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
	long	_eisa_IRQ	# eisa card interrupt
 #  	128-16=112  vectors for future ?
 	space	4*(112)		#
	text

	global	_runrun,_sw_queuehead
	global	_sw_level

 # This is the primary entry to handle interrupts (except level 7)
 # _lev7int in dmachain.s

_Interrupt:
	addq.l	&1,_cnt+V_INTR
	mov.l	%d0,-(%sp)		# save the 4 C registers
	mov.l	%d1,-(%sp)		# save the 4 C registers
	mov.l	%a0,-(%sp)		# save the 4 C registers
	mov.l	%a1,-(%sp)		# save the 4 C registers
	subq.l  &4,%sp                  # allocate stack space (long alligned)
	mov     %sr,(%sp)		# save sr on stack
	mov     &HIGH,%sr               # level must be HIGH
	tst.l	_interrupt		# check if nested interrupts
	bne.b	intr_1S			# yes ignore timestamps
	tst.l   _mainentered            # check if main entered
	beq.b   intr_1S                 # no ignore timestamps

	jsr	_ki_accum_push_TOS_int	# accum clocks
intr_1S:
	addq.l	&1,_interrupt		# begin interrupt processing
	mov     (%sp),%sr               # restore previous sr
	addq.l  &4,%sp                  # deallocate stack space

	mov.l   _sw_level,-(%sp)        #save current sub-level
	clr.l	_sw_level		#hardware interrupts are at sub-level 0
     	mov.l	24(%sp),%d0		#exception format word
     	andi.l	&0x3ff,%d0		#isolate the vector offset field
 	mov.l	(_intr_vect_table,%d0),%a0 # get pointer to isr routine
	tst.l	%a0			# check if there is one?
	bne.b	intr_3S			# yes, continue
	pea	no_vect_panic_msg	# sorry time to die
	jsr 	_panic			# gone
	addq	&4,%sp			# welllll
intr_3S:
	lsr.l   &2,%d0                  # divide vect offset by 4 to vector #
	mov.l	%d0,-(%sp)		# pass the vector number
	jsr	(%a0)			# dispatch the code for the interrupt
	addq.l	&4,%sp			# pop vector # argument
	tst.l	_smart_poll		# check if smart polling active
	beq.b	int_ret0S		# no, continue
	jsr	_do_smart_poll		# brain damaged hardware
int_ret0S:
	mov.w	&HIGH,%sr		#protect (all exits below reset it!)
	mov.l	(%sp)+,_sw_level		#restore sub-level

	tst.l	_sw_queuehead		#any pending software triggers?
	beq.b	int_ret1S			#branch if not

	movq	&7,%d0			#prepare to...
	and.b	16(%sp),%d0		#extract interrupt level for upcoming rte
	mov.l	%d0,-(%sp)		#pass it
	bsr	sw_service		#service any pending software triggers
	addq.l	&4,%sp			#pop the parameter
int_ret1S:
	subq.l	&1,_interrupt		# uncharge interrupts for profiler
	bne.b	int_ret4S		# skip if nested interrupt
	tst.l   _mainentered            # check if main entered
	beq.b   int_ret4S               # no ignore timestamps
	jsr	_ki_accum_pop_TOS	# accum clocks

int_ret4S:
	mov.l	(%sp)+,%a1		# restore the 4 C registers
	mov.l	(%sp)+,%a0		# restore the 4 C registers
	mov.l	(%sp)+,%d1		# restore the 4 C registers
	mov.l	(%sp)+,%d0		# restore the 4 C registers
	btst	&5,(%sp)			#supervisor mode?
	bne.b	int_ret2S			#branch if so
	tst.l	_runrun			#need to reschedule processor?
	bne.b	int_ret3S			#branch if so; fake a reschedule trap
int_ret2S:
	rte				#return from exception

int_ret3S:	and	&0xf8ff,%sr		#lower processor priority
	andi.w	&0xf000,6(%sp)		#clear the vector offset field
	ori.w	&RESCHED,6(%sp)		#set reschedule trap format
	jmp	_fault

 # Routine to handle dio_I & dio_II interrupts at level 1 to 6

_dio_IRQ:
	mov.l	4(%sp),%d0		# get the vector number
 	lea.l	(_rupttable-0x60,%d0.w*4),%a0 # get list head pointer
dio_IRQ0:
	mov.l	(%a0),%d0		#at the end of the list yet?
	beq.b	dio_IRQ1		#if so, simply dismiss
	mova.l	%d0,%a0			#else, prepare to poll...
	mova.l	(%a0)+,%a1		#get REG
	mov.b	(%a1),%d1		#get register data
	and.b	(%a0)+,%d1		#filter it through MASK
	cmp.b	%d1,(%a0)+		#compare it with VALUE
	bne.b	dio_IRQ0		#branch if mismatch (a0 points to NEXT)

	addq.l	&4,%a0			#skip over NEXT field
	mova.l	(%a0)+,%a1		#isr routine address
	sf	(%a0)			#clear chaining flag
	mov.l	%a0,-(%sp)		#save ptr to interrupt.chainflag
	mov.l	%d0,-(%sp)		#push isr argument
	jsr	(%a1)			#call isr
	addq.l	&4,%sp			#pop isr argument
	mova.l	(%sp)+,%a0		#restore ptr to interrupt.chainflag
	tst.b	(%a0)			#did the isr service the interrupt?
	subq.l	&8,%a0			#point back to NEXT (does NOT set CC)
	bne.b	dio_IRQ0		#branch if mismatch (a0 points to NEXT)
dio_IRQ1:
	rts				# done, exit interrupt

 #
 # sw_service(new_level)
 #   . check for software triggers to execute based on new_level
 #   . enter with level HIGH; will return with level HIGH
 #   . enter with sub-level already set; it will be preserved
 #
	global	sw_service		# for profiler only!!!!!!
sw_service:
#ifdef GPROF_MCOUNT
	jsr     mcount
#endif
sw_service0S:
	mov.l	_sw_queuehead,%d0	#get software queue
	bne.w	sw_service2S			#branch if queue non-empty
sw_service1S:
	rts

sw_service2S:	mova.l	%d0,%a0
	mov.l	4(%sp),%d0		#new interrupt level
	cmp.b	%d0,sw_lvl(%a0)		#compare it with the trigger''s level
	bgt.b	sw_service1S			#branch if higher; exit
	blt.b	sw_service3S			#branch if lower; service it
	mov.l	_sw_level,%d0		#pending level same; examine sub-level
	cmp.b	%d0,sw_slvl(%a0)		#compare it with the trigger''s sub-level
	bge.b	sw_service1S			#branch if not less than; exit

sw_service3S:	mov.l	sw_link(%a0),_sw_queuehead  #first, de-link this trigger
	movq	&0x20,%d0		#build new sr; start with supervisor bit
	or.b	sw_lvl(%a0),%d0		#merge in new processor interrupt level
	asl.w	&8,%d0			#finally, shift into proper position
	mov.l	_sw_level,-(%sp)		#save current sub-level
	pea	sw_proc(%a0)		#save proc pointer to clear later
	mov.l	sw_arg(%a0),-(%sp)	#push the argument
	mov.b	sw_slvl(%a0),_sw_level+3	#set new sub-level (note byte trick)
	mov.l	sw_proc(%a0),%a1		#get procedure address
	mov.l	&1,sw_proc(%a0)		#indicate that the trigger is in progress
	mov.w	%d0,%sr			#set new processor level
	jsr	(%a1)			#call the routine
	addq.l	&4,%sp			#pop the parameter
	mov.l	(%sp)+,%a0		#now get proc pointer again
	mov.w	&HIGH,%sr		#cover ourselves
	mov.l	(%sp)+,_sw_level		#restore previous sub-level
	cmp.l	(%a0),&1			#has a new trigger occured?
	bhi.b	sw_service0S		#if so, don''t mess with it
	clr.l	(%a0)			#else, indicate trigger completed
	bra.b	sw_service0S		#go check for more triggers
	lalign	4
	text
