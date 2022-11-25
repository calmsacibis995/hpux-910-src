 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/drvst.s,v $
 # $Revision: 1.5.84.4 $	$Author: marshall $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/12/03 16:49:50 $
 #
 # (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1990. ALL RIGHTS
 # RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 # REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 # THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 #
 #*************************** drvst.s **********************************
 #	
 #	AUTHOR:		  Mark Miller (RND)
 #	ADAPT TO UNIX:	  Jeff Wu
 #      bug fix:          Warren Burnett (January 1991)
 #                          (see #-=-=-=- comments)
 #	
 #	DATE BEGUN:	  26 October 1984
 #
 #	MODULE:		  etherh
 #	PROJECT:	  saw_project
 #	REVISION:	  2.2
 #	SCCS NAME:	  /users/fh/saw/sccs/etherh/system/s.drvst.s
 #	LAST DELTA DATE:  1/24/91
 #
 #	DESCRIPTION:
 #		This module contains the self-test code for the LAN card
 #		
 #	DEPENDENCIES:
 #		- get_network_time()
 #		- struct time_stamp
 #		- LAN_return_addr	defined in locore.s
 #		- bus_trial		defined in locore.s
 #		
 #**********************************************************************
		   
		global	_hw_int_test		#| exported function
		global	_hw_ext_test		#| exported function
		global	_get_network_time	#| external function

		text

 #
 #	The following offsets corresponds to the "C" structure
 #	"time_stamp"
 #

    		set	time,4

 #
 #	Stack allocation (offsets from a6)
 #

         	set	base_addr,8			#| base address of the card
          	set	local_size,-64			#| stack size used
       		set	regsave,-40			#| save non-scratch registers
       		set	timerec,-48			#| "time_stamp" record
       		set	scratch,-60			#| save scratch registers
         	set	oldbustrial,-64		#| the old bus trial flag

 #
 #   int
 #   hw_int_test( base_addr )
 #   char *base_addr;
 #
 #   -- this function performs internal tests on the DIO LAN card
 #   -- at address "base_addr", it returns 0 if the test passed, or
 #   -- it returns the error code.
 #
 #          NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE
 #
 #		     DO NOT PROPOGATE THIS TECHNIQUE OR CODE.
 #
 #  	It will NOT be supported by the HP-UX kernel lab.  This routine is
 # the LAST non-locore.s bus error proof routine that will ever be supported.
 #
 # 	If you need to read/write/copy bytes and be protected from a bus
 # error, you should use testr, testw or any of the *copy_prot routines 
 # located in locore.s.  Routines which survive bus error are very dangerous, 
 # and this will be the only one outside of locore.s allowed.
 #
 #	The mechanism involved is to set a trial flag during the selftest.  If
 # a bus error is encountered, the bus error handler will recognize the trial
 # flag to mean that the bus error should be "ignored".  The handler will
 # clear the exception frame from the stack and jmp back to whatever address
 # is in _LAN_return_addr, as long as that address is not zero.
 #

_hw_int_test:
		link	%a6,&local_size		#| allocate stack space
		movm.l %a2-%a5/%d2-%d7,regsave(%a6) #| save registers
		mova.l base_addr(%a6),%a0	#| 

		mov.l	_bus_trial,oldbustrial(%a6) #| save old bus trial flag
		lea	resume,%a2		#| resume is where to return to
		mov.l	%a2,_LAN_return_addr	#| if a bus error occurs.
		mov.l	&2,_bus_trial		#| set our bus error trial flag

		jsr	dst_mainline		#| subroutine
resume:
		mov.l	%d7,%d0
		mov.l	oldbustrial(%a6),_bus_trial #| restore bus trial flag
		clr.l	_LAN_return_addr	#| invalidate jump return addr
		movm.l regsave(%a6),%a2-%a5/%d2-%d7 #| restore registers
		unlk	%a6
		rts

 # 	If using a kernel with protected text, the bus error catching code 
 # is not needed.  Should the selftest induce a bus error, _xbuserror in 
 # locore.s will clean up the stack (if _bus_trial == 2) and jump to resume.

 #
 #   int
 #   hw_ext_test( base_addr )
 #   char *base_addr;
 #
 #   -- this function performs external loop tests on the DIO LAN card
 #   -- at address "base_addr", it returns 0 if the test passed, or
 #   -- it returns the error code.
 #

_hw_ext_test:
		link	%a6,&local_size		#| allocate stack space
		movm.l %a2-%a5/%d2-%d7,regsave(%a6) #| save registers
		mova.l base_addr(%a6),%a0	#| 

		jsr	ext_loop_test		#| subroutine

		mov.l	%d7,%d0
		movm.l regsave(%a6),%a2-%a5/%d2-%d7 #| restore registers
		unlk	%a6
		rts


 #
 #
 #	This routine allows the self-test to get the time.
 #	This routine invokes a network routine in order to get the
 #	time.
 #
 #	Note:	The scratch registers a0,a1,d0 are saved and restored
 #		on the stack
 #	
 #	ON EXIT:
 #		d1.l -- contains the number of milliseconds
 #			since the last time it was cycled.
 #		     

get_time:
		movm.l %a0-%a1/%d0,scratch(%a6)	#| save scratch registers
		lea	timerec(%a6),%a0		#| 
		mov.l %a0,-(%sp)		#| push parameter

		jsr	_get_network_time	#| system routine

		addq.l	&4,%sp			#| pop parameter off stack
		mov.l	time+timerec(%a6),%d1	#| return it to correct register
		movm.l scratch(%a6),%a0-%a1/%d0	#| restore scratch registers
		rts
		   
 #***************************************************************************
 #***************** GLOBAL CONSTANTS OF MODULE ******************************
 #***************************************************************************

           	   set	   novram_size,0x80			    # in bytes.
          	   set	   s_ram_size,0x4000		    # in bytes.
             	   set	   novram_offset,0xc000		    # offset from card base.
          	   set	   ram_offset,0x8000		    # offset from card base.
       		   set	   initblk,ram_offset
          	   set	   rdp_offset,0x4000		    # offset from card base.
          	   set	   rap_offset,0x4002		    # offset from card base.
            	   set	   tbuff_offset,0x28			    # offset from shared ram.
            	   set	   rbuff_offset,0x48			    # offset from shared ram.
           	   set	   tmd2_offset,0x8024		    # offset from card base.

                   set	   high_bank_offset,64			   # offset from novram base.
         	   set	   bank_flag,0			   # bit number.

            	   set	   error_offset,10			   # adjustment for cno.
            	   set	   card_rset_ec,1			   # card reset test err code.
           	   set	   ram_test_ec,14			   # shared ram test error code.
         	   set	   int_lb_ec,18			   # int loopback test err code.
          	   set	   int_req_ec,18			   # ir bit error code.
         	   set	   ext_lb_ec,25			   # ext loopback test err code.
      		   set	   ack_ec,34			   # acknowledge error code.

 #*****************************************************************************

 #*****************************************************************************
 #*****************  PACKET DATA STRUCTURES FOR THE LOOP TESTS. ***************
 #*****************************************************************************

		   data
               	   set	   lance_test_data,.
	lalign	2
init_block:	   space	   2*(3)			   # node address (swapped).
		   long	   0,0			   # disable multicast.
		   short	   0x8018		    # address of rmdr.
		   short	   0			   # rmdr has one entry.
		   short	   0x8020		    # address of tmdr.
		   short	   0			   # tmdr has one entry.

rmd_ring:	   short	   0x8048		    # rec buff addr (rmd0).
		   short	   0x8000		    # set lance buff own bit(rmd1).
		   short	   -32			   # buff byte len (rmd2).
		   short	   0			   # clear mess byte count (rmd3).

tmd_ring:	   short	   0x8028		    # trans buff addr (tmd0).
		   short	   0x8300		    # set lance buff own bit
 #						  ;   AND STP/ENP BITS(TMD1).
	lalign	2
		   space	   2*(1)			   # buff byte length (tmd2).
		   short	   0			   # clear trans status(tmd3).

	lalign	2
tbuf:		   space	   2*(3)			   # node address.
		   byte	   0,1,2,3,4,5,6,7,8,9	   # data.
		   byte	   10,11,12,13,14,15,17	   # data.
		   byte	   18,19,20,21,22	   # data.
	lalign	2
crc_appendage:	   space	   2*(2)			   # crc appendage.
		   
rbuf:		   long	   0,0,0,0,0,0,0,0	   # clear receive buffer.


 #*****************************************************************************
 #***************** LOOP TESTS CONSTANTS **************************************
 #*****************************************************************************


    		   set	   loop,0x0004		    # mode word bit 2.
     		   set	   dtcrc,0x0008		    # mode word bit 3.
         	   set	   collision,0x0010		    # mode word bit 4.
    		   set	   drty,0x0020		    # mode word bit 5.
        	   set	   internal,0x0040		    # mode word bit 6.
       		   set	   promisc,0x8000		    # mode word bit 15.

      		   set	   ir_bit,6			   # control register bit 6.
    		   set	   idon,8			   # csr0 bit 8.
    		   set	   tint,9			   # csr0 bit 9.
    		   set	   rint,10			   # csr0 bit 10.
    		   set	   rtry,10			   # tmd3 bit 10.

	lalign	2
words_to_compare:   space	   2*(1)

 #***************** MULTIPLY EACH OF THE FOLLOWING FOUR TIME VALUES BY 10 
 #***************** FOR USE WITH THE CNO DRIVER.
 #
 #	time units = 1 millisecond
 #
 #	Note: since the timer on Unix is updated every 20 ms, the
 #	following timeout values should have a minimum of 20 ms

                   set	   int_crc_loop_time,30			  
                   set	   int_coll_loop_time,420			  
             	   set	   ext_loop_time,420			  

 #****************************************************************************

 #****************************************************************************
 #     MAINLINE OF PROGRAM.						    *
 #									    *
 #     PURPOSE:								    *
 #     TO CALL EACH SUBTEST OF THE SELF_TEST UNTIL ALL SUBTESTS HAVE BEEN    *
 #     SUCCESSFULLY PERFORMED OR UNTIL AN ERROR IS FOUND.  IF NO ERROR IS    *
 #     FOUND, REGISTER D7 IS CLEARED AND THE CARD RESET BEFORE RETURNING TO  *
 #     THE CALLING PROGRAM.  OTHERWISE, PROGRAM EXECUTION JUMPS FROM THE	    *
 #     END OF THE SUBTEST TO THE END OF MAINLINE, SKIPPING ANY REMAINING	    *
 #     SUBTESTS, AND RETURNS TO THE CALLING PROGRAM WITH THE ERROR CODE	    *
 #     NUMBER IN D7 AFTER RESETING THE CARD.				    *
 #									    *
 #     ASSUMPTIONS:							    *
 #     IT IS ASSUMED THAT ONCE A SECTION OF THE DIO LAN CARD PASSES ITS	    *
 #     PORTION OF THE SELF-TEST, IT WILL REMAIN FUNCTIONAL FOR THE REST OF   *
 #     THE SELF-TEST.							    *
 #									    *
 #     ON ENTRANCE:							    *
 #	A0 CONTAINS THE DIO LAN CARD'S BASE ADDRESS.			    *
 #									    *
 #     GLOBAL REGISTER USAGE:						    *
 #	A0:  CARD BASE ADDRESS.						    *
 #	A2:  RAP ADDRESS.						    *
 #	A3:  RDP ADDRESS.						    *
 #	D7:  ERROR CODE.						    *
 #									    *
 #     ON EXIT:								    *
 #	D7 CONTAINS THE SELF-TEST'S ERROR CODE.				    *
 #									    *
 #     REGISTERS CHANGED:						    *
 #	BY THIS ROUTINE:  D0, D3, D7(IF NO ERRORS FOUND)		    *
 #	BY SUBROUTINES:	 A1..A4, D0..D7					    *
 #****************************************************************************

 #-=-=-=- fix to prevent NOVRAM corruption on post-370 systems
 #-=-=-=- for DTS #INDaa08682                 1/24/91 (warren)
	data
	drv_nvr_cnts: long 0,0,0,0,0,0,0,0
                      long 0,0,0,0,0,0,0,0
                      long 0,0,0,0,0,0,0,0
                      long 0,0,0,0,0,0,0,0
 #-=-=-=-
		   text
            	   set	   dst_mainline,.
		   
 #-=-=-=-
	mov.l	&novram_offset,%d0
	mov.l	&novram_size,%d1
	movea.l	%a0,%a4
	adda.l	%d0,%a4
	lea	drv_nvr_cnts,%a5
fix_l1:	sub.l	&4,%d1
	move.l	0(%a4,%d1),0(%a5,%d1)
	cmp.l	%d1,&0
	bne	fix_l1
 #-=-=-=-
		   bsr	   rset_and_reg_check	   # perform test.      

		   mov.l  &s_ram_size,%d3	   # set ram size var.
		   mov.l  &ram_offset,%d0	   # select s-ram for testing.
		   movq   &ram_test_ec+error_offset,%d7  # set error flag.

		   bsr	   test_rams		   # perform test.

		   mov.l  &novram_size,%d3	   # set ram size var.
		   mov.l  &novram_offset,%d0	   # select novram for testing.
		   movq   &ram_test_ec+error_offset+2,%d7  # set error flag.
		   
		   bsr	   test_rams		   # perform test.

		   bsr	   int_loop_tests	   # perform tests.

		   clr.l   %d7			   # return no error

reset_and_return:  
 #-=-=-=-
	mov.l	&novram_offset,%d0
	mov.l	&novram_size,%d1
	movea.l	%a0,%a4
	adda.l	%d0,%a4
	lea	drv_nvr_cnts,%a5
fix_l2:	sub.l	&4,%d1
	move.l	0(%a5,%d1),0(%a4,%d1)
	cmp.l	%d1,&0
	bne	fix_l2

		   mov.b  &1,1(%a0)	   # reset dio lan card.
 #-=-=-=-
		   rts				   # return to driver.

 #****************************************************************************

 #****************************************************************************
 #     FUNCTION NAME:  RESET AND REGISTER CHECK				    *
 #									    *
 #     PURPOSE:								    *
 #     THIS SUBROUTINE BOTH STOPS AND RESETS A DIO-LAN CARD WHILE CHECKING   *
 #     THE CARD'S ID REGISTER, CONTROL REGISTER, STATUS REGISTER, AND NOV-   * 
 #     RAM CHECKSUM, AND THE LANCE'S RAP, RDP, CSR0, CSR1, CSR2, AND CSR3    *
 #     REGISTERS.							    *
 #									    *
 #     ON ENTRANCE:							    *
 #	DON'T CARE.							    *
 #									    *
 #     ON EXIT:								    *
 #	A2 CONTAINS THE LANCE'S RAP ADDRESS.				    *
 #	A3 CONTAINS THE LANCE'S RDP ADDRESS.				    *
 #	D7 CONTAINS THE ERROR CODE NUMBER OF AN ERROR IF FOUND, OTHERWISE   *
 #	  IT CONTAINS 13 + THE ERROR CODE OFFSET.			    *
 #									    *
 #     REGISTERS CHANGED:						    *
 #	BY THIS ROUTINE:  A1, A2, A3, D4, D6, D7			    *
 #	BY SUBROUTINES:	 A4, D0..D7					    *
 #****************************************************************************

                   set	  rset_and_reg_check,.
		   
		   mova.l %a0,%a1			
		   mova.l %a0,%a2
		   mova.l %a0,%a3
		   adda.l  &novram_offset,%a1	   # put novram base addr in a1.
		   adda.w  &rap_offset,%a2	   # place rap addr in rap_addr.
		   adda.w  &rdp_offset,%a3	   # place rdp addr in rdp_addr.

clear_chk_id_reg:   movq   &error_offset+card_rset_ec,%d7  # set error flag.
		   mov.b  &0,3(%a0)	   # clear ie and lock bits in
		   mov.b  3(%a0),%d4	   #   cntrl reg, read back,
		   andi.b  &0x88,%d4		    #   check if cleared.
		   bne	   rset_error

set_chk_id_reg:	   addq.l  &1,%d7		   # set error flag.
		   mov.b  &0x08,3(%a0)	    # set lock bit in 
		   mov.b  3(%a0),%d4	   #   cntrl reg, read back, 
		   andi.b  &0x08,%d4		    #   and check if set.
		   beq	   rset_error		

stop_the_card:	   addq.l  &1,%d7		   # set error flag.
		   bsr	   card_stop
		   btst	   &0,%d4		   # check to see if card stopped.
		   bne	   rset_error		

 #***************** CARD STOPPED, RAP = CSR0

control_reg_chk:	   addq.l  &1,%d7		   # set error flag.
		   mov.b  3(%a0),%d4	   # check to see that bits
		   andi.b  &0x0c,%d4		    #   3 and 2 are still set
		   cmpi.b  %d4,&0x0c		    #   after stopping card.
		   bne	   rset_error		   # not set = error.

csr0_check_1:	   addq.l  &1,%d7		   # set error flag.
		   mov.w  (%a3),%d4	   # read csr0 register
		   cmpi.w  %d4,&4		   #   and check to see if
		   bne	   rset_error		   #   all but stop bit clear.

 #***************** RESTART CARD

re_start_lance:	   mov.w  &2,(%a3)

		   addq.l  &1,%d7		   # set error flag.
		   mov.l  %d7,%d4		   # copy error code.
		  
nv_checksum_calc:   bsr	   calc_nv_checksum	   # result in d2.

get_checksum:	   bsr	   form_byte		   # combine novram nibbles.   
		   cmp.b   %d2,%d0		   # compare checksums.
		   bne	   rset_error

		   cmp.l   %d7,%d4		   # check error code
		   bne.b   ie_lock_ack_check	   #   and adjust if  
		   addq.l  &1,%d7		   #   necessary.

 #***************** CARD STOPPED AGAIN, RESET PERFORMED IN CALC_NV_CHECKSUM

ie_lock_ack_check:  addq.l  &1,%d7		   # set error flag.
		   mov.b  3(%a0),%d4	   # isolate the ie, ir,
		   andi.b  &0xcc,%d4		    #   ack and lock bits and
		   bne	   rset_error		   #   check if reset cleared. 

csr0_check_2:	   addq.l  &1,%d7		   # set error flag.
		   cmpi.w  (%a3),&4	   # read csr0 to see if
		   bne	   rset_error		   #   all but stop bit clear.

		   movq   &3,%d6		   # set loop_count = 4.
		   addq.l  &1,%d7		   # set error flag.
rap_check:	   mov.w  %d6,(%a2)	   # write to rap then
		   cmp.w   %d6,(%a2)	   #   read back and compare.
		   bne	   rset_error
		   dbra	   %d6,rap_check

csr1_check:	   addq.l  &1,%d7		   # set error flag.
		   mov.w  &1,(%a2)	   # set rap = csr1.
		   
		   mov.w  &0,(%a3)	   # clear bits 15 - 1 and
		   cmpi.w  (%a3),&0	   #   check to see if cleared. 
		   bne	   rset_error
		   
		   mov.w  &0xfffe,(%a3)	    # set bits 15 - 1 and
		   cmpi.w  (%a3),&0xfffe	    #   check to see if set.
		   bne.b   rset_error

csr2_check:	   addq.l  &1,%d7		   # set error flag.
		   mov.w  &2,(%a2)	   # set rap = csr2.
		   
		   mov.w  &0x00ff,(%a3)	    # set bits 7 - 0,
		   mov.w  (%a3),%d6	   #   read back and
		   andi.w  &0x00ff,%d6		    #   check to see if
		   cmpi.w  %d6,&0x00ff		    #   those bits set.
		   bne.b   rset_error
		   
		   mov.w  &0,(%a3)	   # clear bits 7 - 0,	
		   mov.w  (%a3),%d6	   #   read back and
		   andi.w  &0x00ff,%d6		    #   check to see if
		   bne.b   rset_error		   #   those bits cleared.

csr3_check:	   addq.l  &1,%d7		   # set error flag.
		   mov.w  &3,(%a2)	   # set rap = csr3.
		   
		   mov.w  &0x0007,(%a3)	    # set bits 2 - 0,
		   mov.w  (%a3),%d6	   #   read back and
		   andi.w  &0x0007,%d6		    #   check to see if
		   cmpi.w  %d6,&0x0007		    #   those bits set. 
		   bne.b   rset_error
		   
		   mov.w  &0,(%a3)	   # clear bits 2 - 0,	
		   mov.w  (%a3),%d6	   #   read back and
		   andi.w  &0x0007,%d6		    #   check to see if
		   bne.b   rset_error		   #   those bits cleared.

rset_ok:		   rts				   # no errors found, continue.

 #*****************************************************************************
 #** THE FOLLOWING INSTRUCTION REMOVES THE CURRENT SUBROUTINE'S RETURN	   ***
 #** ADDRESS OFF OF THE SYSTEM USER'S STACK ENABLING MAINLINE'S RTS IN-	   ***
 #** STRUCTION, WHICH FOLLOWS THE INSTRUCTION FOUND AT MAINLINE'S RESET_	   ***
 #** AND_RETURN LABLE, TO RETURN PROGRAM EXECUTION TO THE CALLING ROUTINE.  ***
 #** IF THE CURRENT RETURN ADDRESS WERE NOT POPPED, MAINLINE'S RTS WOULD	   ***
 #** RETURN PROGRAM EXECUTION TO THE CURRENT SUBROUTINE (IE. ENDLESS LOOP). ***
 #*****************************************************************************

rset_error:	   addq.l  &4,%sp		   # remove return address.
		   bra	   reset_and_return	   # terminate program 
 #						     SAVING ERROR NUMBER.

 #***************************************************************************

 #***************************************************************************
 #									   *
 #     FUNCTION NAME:  TEST THE CARD'S RAM				   *
 #									   *
 #     PURPOSE:								   *
 #     THIS SUBROUTINE CHECKS OUT THE CARD'S SHARED RAM OR NOVRAM (DEPEND-  *  
 #     ING UPON THE PARAMETERS PASSED IN) FOR PROPER OPERATION.	EACH BYTE  *
 #     OF RAM IS WRITTEN TO (USING THIS PROGRAM AS THE TESTING DATA), IMME- *
 #     DIATELY READ BACK, THEN READ BACK THROUGH ONCE MORE AFTER ALL BYTES  *
 #     HAVE BEEN WRITTEN TO.  THE TESTING DATA IS THEN INVERTED AND THE	   *
 #     TEST REPEATED.							   *
 #									   *
 #     ON ENTRANCE:							   *
 #	D0 CONTAINS THE OFFSET OF THE SELECT RAM FROM THE CARD'S BASE.	   *
 #	D3 CONTAINS THE SIZE OF THE SELECTED RAM.			   *
 #	D7 CONTAINS 13 + THE ERROR OFFSET.				   *
 #									   *
 #     REGISTER USAGE:							   *
 #	D0: BYTES OF RAM LEFT TO TEST.					   *
 #	D1: BYTES OF TEST PATTERN LEFT.					   *
 #	D2: BIT #0 IS THE READ/WRITE FLAG.				   *
 #	D2: BIT #1 IS THE INVERT/NON-INVERT FLAG.			   *
 #	D3: THE SIZE OF THE RAM BEING TESTED (TYPE OF RAM INDICATOR).	   *
 #									   *
 #     ON EXIT:								   *
 #	D7 CONTAINS THE ERROR CODE NUMBER OF AN ERROR IF FOUND, OTHERWISE  *
 #	  15 + THE ERROR OFFSET AFTER THE SHARED RAM TEST OR 16 + THE	   *
 #	  ERROR OFFSET AFTER THE NOVRAM TEST.				   *
 #									   *
 #     REGISTERS CHANGED:						   *
 #	BY THIS ROUTINE:  A1, A4, D0, D1, D2, D4, D5, D7		   *
 #	BY SUBROUTINES:	 N/A						   *
 #***************************************************************************

 #***************** ALGORITHM OF THE WRITE/READ LOOP ************************
 #
 #     WHILE (NOT ERROR_FOUND) AND (NOT TERMINATE) DO
 #	BEGIN
 #	  TESTWORD_ADDR:= TESTWORD_ADDR - 4;  (* LONG WORD ACCESSES *)
 #	  READ (TESTWORDS);
 #	  IF INVERT_FLAG = TRUE THEN
 #	    INVERT THE TEST WORDS;
 #	  IF LOOP_TYPE = WRITE_AND_READ THEN
 #	    WRITE (TESTWORDS);
 #	  IF RAM_TYPE = NOVRAM THEN
 #	    READ, ISOLATE AND COMPARE NOVRAM NIBBLES WITH TESTWORDS NIBBLES
 #	  ELSE
 #	    READ AND COMPARE SHARED RAM LONG WORDS WITH TESTWORDS;
 #	  IF NOT ERROR_FOUND THEN
 #	    BEGIN
 #	    | TESTWORD_ADDR:= TESTWORD_ADDR - 4;
 #	    | IF TESTWORDS USED UP THEN
 #	    |	RE-INITIALIZE TESTWORD_ADDR
 #	    | ELSE
 #	    |	BEGIN
 #	    |	| RAM_LEFT:= RAM_LEFT - 4;
 #	    |	| IF NO RAM LEFT TO TEST THEN
 #	    |	|   IF LOOP_TYPE = WRITE_AND_READ THEN
 #	    |	|     BEGIN
 #	    |	|	LOOP_TYPE:= READ_ONLY;
 #	    |	|	RAM_LEFT_TO_TEST:= RAM_SIZE;
 #	    |	|     END
 #	    |	|   ELSE
 #	    |	|     IF INVERT_FLAG = FALSE THEN
 #	    |	|	BEGIN
 #	    |	|	  INVERT_FLAG:= TRUE;
 #	    |	|	  LOOP_TYPE:= WRITE_AND_READ;
 #	    |	|	  RAM_LEFT_TO_TEST:= RAM_SIZE;
 #	    |	|	END
 #	    |	|     ELSE
 #	    |	|	TERMINATE:= TRUE;
 #	    |	END;  (* ELSE *)
 #	    END;  (* OUTER THEN *)
 #	END;  (* WHILE *)
 #
 #*****************************************************************************


            	   set	   pattern_size,0x34c		    # pattern byte size.

         	   set	   test_rams,.

             	   set	   ramtest_begin,.
		   mova.l %a0,%a4		   # copy card address
		   adda.l  %d0,%a4		   # add selected ram'S BASE ADDR.
		   
		   lea	   dst_mainline,%a1	   # location of test data.   
		   clr.l   %d2

init_ram_size_var:  mov.l  %d3,%d0		   # copy ram size.

init_pattern_size:  mov.l  &pattern_size,%d1

		   subq.l  &4,%d0		   # ram size - 4 (loop count).
	    
               	   set	   write_read_loop,.
		   mov.l  0(%a1,%d1),%d4		   # store test word in d4.

invert_check:	   btst	   &1,%d2		   # test bit, branch if 
		   beq.b   write_check		   #   non invert loop.

           	   set	   invert_word,.
		   not.l   %d4			   # invert test word.

           	   set	   write_check,.
		   btst	   &0,%d2		   # check
		   bne.b   ram_type_check	   #   read/write flag.

write:		   mov.l  %d4,0(%a4,%d0)		   # write to ram on card.

ram_type_check:	   cmpi.w  %d3,&0x80		    # if novram, read, isolate,
		   bne.b   read_shared_ram	   #   and compare nibbles.  

read_novram_nibs:   mov.l  0(%a4,%d0),%d5		   # read nibbles. 
		   andi.l  &0x000f000f,%d5	    # isolate nibbles of novram. 
		   andi.l  &0x000f000f,%d4	    # isolate same in test word.
		   cmp.l   %d5,%d4		   # compare.		      
		   bra.b   error_check
		   
read_shared_ram:	   cmp.l   %d4,0(%a4,%d0)		   # check test word against   
 #						  ;   READ WORD.
           	   set	   error_check,.
		   bne.b   ram_error		   # bad ram found.
		   subi.l  &4,%d1		   # pattern.
		   bmi	   init_pattern_size	  
		   subi.l  &4,%d0		   # bytes of ram.
		   bpl.b   write_read_loop
		   bchg	   &0,%d2		   # change bit 0 to alternate
		   beq.b   init_ram_size_var	   #   read/write & read only loop.
		   bchg	   &1,%d2		   # change bit 1 to alternate
		   beq.b   init_ram_size_var	   #   invert and non-inv loop.
		   
		   cmpi.w  %d3,&0x80		    # perform check on
		   beq.b   ram_ok_exit		   #   shared ram only.
		
byte_write_check:   addq.l  &1,%d7		   # set error bit.
		   mov.b  &0x00,1(%a4)		    # a check 
		   mov.b  &0xff,(%a4)		    #   to make		    
		   cmpi.w  (%a4),&0xff00		    #   sure that	 
		   bne.b   skip_find_chip	   #   byte writing
		   mov.b  &0xaa,1(%a4)		    #   hardware
		   cmpi.w  (%a4),&0xffaa		    #   works.
		   bne.b   skip_find_chip

ram_ok_exit:	   mov.b  &1,1(%a0)	   # reset card (restore nram).
		   rts				   # no errors found, continue.

         	   set	   ram_error,.
check_ram_type:	   cmpi.w  %d3,&0x80		    # perform find chip
		   beq.b   skip_find_chip	   #   on shared ram only.

find_chip:	   cmp.b   %d4,3(%a4,%d0)		   # determine
		   bne.b   skip_find_chip	   #   which ram
		   swap	   %d4			   #   chip isn'T
		   cmp.b   %d4,1(%a4,%d0)		   #   working properly.
		   bne.b   skip_find_chip	   # save low byte chip error.

		   addq.l  &3,%d7		   # set high byte chip error.

 #*****************************************************************************
 #** THE FOLLOWING INSTRUCTION REMOVES THE CURRENT SUBROUTINE'S RETURN	   ***
 #** ADDRESS OFF OF THE SYSTEM USER'S STACK ENABLING MAINLINE'S RTS IN-	   ***
 #** STRUCTION, WHICH FOLLOWS THE INSTRUCTION FOUND AT MAINLINE'S RESET_	   ***
 #** AND_RETURN LABLE, TO RETURN PROGRAM EXECUTION TO THE CALLING ROUTINE.  ***
 #** IF THE CURRENT RETURN ADDRESS WERE NOT POPPED, MAINLINE'S RTS WOULD	   ***
 #** RETURN PROGRAM EXECUTION TO THE CURRENT SUBROUTINE (IE. ENDLESS LOOP). ***
 #*****************************************************************************

skip_find_chip:	   addq.l  &4,%sp		   # remove return address.
		   bra	   reset_and_return	   # terminate program
 #						  ;   SAVING ERROR NUMBER.

 #****************************************************************************

 #****************************************************************************
 #     FUNCTION NAME:  INTERNAL LOOPBACK TESTS				    *
 #									    *
 #     PURPOSE:								    *
 #     THIS SUBROUTINE SETS UP, CALLS FOR THE EXECUTION OF, AND CHECKS THE   *
 #     RESULTS OF THE FOLLOWING INTERNAL LOOP TESTS: 1) CRC GENERATION AND   *
 #     APPENDAGE WITH PHYSICAL ADDRESSING, 2) CRC CHECK OF RECEIVED PACKET'S *
 #     SOFTWARE APPENDED CRC WITH PHYSICAL ADDRESSING, 3) CRC GENERATION AND *
 #     APPENDAGE IN PROMISCUOUS MODE, 4) CRC CHECKING OF RECEIVED PACKET'S   *
 #     SOFTWARE APPENDED CRC WITH LOGICAL ADDRESSING, 5) CRC GENERATION AND  *
 #     APPENDAGE WITH BROADCAST ADDRESSING AND 6) COLLISION MODE ( 16 TRANS- *
 #     MIT ATTEMPTS ARE MADE).						    *
 #									    *
 #     ON ENTRANCE:							    *
 #	DON'T CARE.							    *
 #									    *
 #     ON EXIT:								    *
 #	D7 CONTAINS THE ERROR CODE NUMBER OF AN ERROR IF FOUND, OTHERWISE   *
 #	  24 + THE ERROR OFFSET.					    *
 #									    *
 #     REGISTERS CHANGED:						    *
 #	BY THIS ROUTINE:  D0, D1, D2, D7				    *
 #	BY SUBROUTINES:	 A1,A4,D0,D1,D2,D4,D5,D6,D7,STATUS(COMPARE PACKS)   *
 #****************************************************************************

              	   set int_loop_tests,.

 #***************** IE BIT AND LANCE OFF FROM PREVIOUS RESET.

		   movq   &error_offset+int_lb_ec,%d7  # set error flag. 
		   mov.w  &15,words_to_compare	   # initialize.

		   mov.l  &0x02142536,init_block   # store swapped
		   mov.w  &0x4758,init_block+4	    #   station address.
		   mov.l  &0x14023625,tbuf	    # station	 
		   mov.w  &0x5847,tbuf+4	    #   address
		   mov.l  &0x447b9fb0,crc_appendage  # crc appendage.

             	   set	   crc_gen_paddr,.			   # loop 1.
		   movq   &-28,%d0		   # packet bytes to trans * -1.
		   movq   &internal+loop,%d1	   # int loop with chip crc add.
		   bsr	   int_nonc_loop	   # perform loop.
		   
		   cmpi.w  %d0,&0		   # check for error
		   bne	   int_loop_error	   #   from the loop test.

            	   set	   crc_ck_paddr,.			
		   movq   &-32,%d0		   # packet bytes to trans * -1.
		   movq   &internal+loop+dtcrc,%d1  #int loop with host crc app.
		   bsr	   int_nonc_loop	   # perform loop.
		   
		   cmpi.w  %d0,&0		   # check for error
		   bne	   int_loop_error	   #   from the loop test.

             	   set	   crc_gen_pmode,.
		   mov.w  &13,words_to_compare	   # re-init, skip crc compare.
		   movq   &-28,%d0		   # packet bytes to trans * -1.
		   mov.l  &internal+loop+promisc,%d1  # int, promisc, chip crc.
		   mov.w  &0x5104,tbuf		    # change packet address.
		   bsr	   int_nonc_loop	   # perform loop.
		   
		   cmpi.w  %d0,&0		   # check for error
		   bne.b   int_loop_error	   #   from the loop test.
	      
		   
            	   set	   crc_ck_laddr,.
		   movq   &-32,%d0		   # packet bytes to trans * -1.
		   movq   &internal+loop+dtcrc,%d1    # int with host crc.
		   bset	   &3,lance_test_data+7	      # set l-addr filter.
		   mov.l  &0xaa68db66,crc_appendage  # crc for new addr.
		   bsr	   int_nonc_loop	  # perform loop.
		   cmpi.w  %d0,&0		   # check for error
		   bne.b   int_loop_error	   #   from the loop test.

             	   set	   crc_gen_baddr,.
		   movq   &-28,%d0		   # packet bytes to trans * -1.
		   movq   &internal+loop,%d1	   # int loop with chip crc app.
		   bclr	   &3,lance_test_data+7	   # clear logical addr filter.
		   mov.l  &0xffffffff,tbuf	    # set broadcast     
		   mov.w  &0xffff,tbuf+4	    #   address.
		   bsr	   int_nonc_loop	   # perform loop.
		   cmpi.w  %d0,&0		   # check for error
		   bne.b   int_loop_error	   #   from the loop test.

check_coll_loop:	   movq   &-28,%d0		   # packet byte to trans * -1.
		   movq   &internal+loop+collision,%d1  # int loop back with
 #						  ;   COLL AND CHIP CRC ADD.
 #		   mov.l  &int_coll_loop_time,%d2  # max loop time.
 # use busy loop rather than timeouts for loop test..prabha 01/27/87
		   mov.l  &1000,%d2                # max loop count.
		   addq.l  &1,%d7		   # incriment error flag.

		   bsr	   lance_loop_test	   # perform test.
 
		   btst	   &rtry,%d6		   # check for error from
		   beq.b   int_loop_error	   #   lance_loop_test.
 
		   btst	   &15,%d4		   # csr0 error sum check.
		   bne.b   int_loop_error	   # branch if set.
		   
no_int_loop_error:  rts				   # no error found, continue.

 #*****************************************************************************
 #** THE FOLLOWING INSTRUCTION REMOVES THE CURRENT SUBROUTINE'S RETURN	   ***
 #** ADDRESS OFF OF THE SYSTEM USER'S STACK ENABLING MAINLINE'S RTS IN-	   ***
 #** STRUCTION, WHICH FOLLOWS THE INSTRUCTION FOUND AT MAINLINE'S RESET_	   ***
 #** AND_RETURN LABLE, TO RETURN PROGRAM EXECUTION TO THE CALLING ROUTINE.  ***
 #** IF THE CURRENT RETURN ADDRESS WERE NOT POPPED, MAINLINE'S RTS WOULD	   ***
 #** RETURN PROGRAM EXECUTION TO THE CURRENT SUBROUTINE (IE. ENDLESS LOOP). ***
 #*****************************************************************************

int_loop_error:	   addq.l  &4,%sp		   # remove return address.
		   bra	   reset_and_return	   # terminate program
 #						      SAVING ERROR NUMBER.
 
 #**************************************************************************

 #***************************************************************************
 #									   *
 #     FUNCTION NAME:  EXTERNAL LOOPBACK TEST				   *
 #									   *
 #     PURPOSE:								   *
 #     THIS SUBROUTINE SETS UP, CALLS FOR THE EXECUTION OF, AND CHECKS THE  *
 #     RESULTS OF AN EXTERNAL LOOP TEST USING PHYSICAL ADDRESSING.	   *
 #									   *
 #     ON ENTRANCE:							   *
 #	DON'T CARE.							   *
 #									   *
 #     ON EXIT:								   *
 #	D7 CONTAINS THE ERROR CODE NUMBER OF AN ERROR IF FOUND, OTHERWISE  *
 #	  25 + THE ERROR OFFSET.					   *
 #									   *
 #     REGISTERS CHANGED:						   *
 #	BY THIS ROUTINE:  D0, D1, D2, D3, D4, D6, D7			   *
 #	BY SUBROUTINES:	 A1,A4,D0,D1,D2,D4,D5,D6,D7,STATUS(COMPARE PACKETS)*
 #***************************************************************************

             	   set ext_loop_test,.
		   
		   mova.l %a0,%a1			
		   mova.l %a0,%a2
		   mova.l %a0,%a3
		   adda.l  &novram_offset,%a1	   # put novram base addr in a1.
		   adda.w  &rap_offset,%a2	   # place rap addr in rap_addr.
		   adda.w  &rdp_offset,%a3	   # place rdp addr in rdp_addr.

sta_addr_to_initblk: bsr	   move_sta_addr	   # get card'S STATION ADDR.
		   
		   movq   &error_offset+ext_lb_ec,%d7  # set error flag.
		   movq   &5,%d3		   # number of trys counter.

               	   set	   ext_lpback_loop,.

set_up_packet:	   movq   &loop,%d1		   # ext loop with chip crc app.
		   movq   &-28,%d0		   # packet bytes to trans * -1.
 #		   mov.l  &ext_loop_time,%d2	   # max loop time.
 # use busy loop rather than timeouts for loop test..prabha 01/27/87
		   mov.l  &1000,%d2	           # max loop count.

		   bsr	   lance_loop_test	   # perform loop test.	 

		   cmpi.w  %d0,&0		   # check for error from
		   beq.b   elb_error_check	   #   lance loop test.
		   dbra	   %d3,ext_lpback_loop	   # max 6 external loop tries.

               	   set	   elb_error_check,.

		   cmpi.w  %d7,&error_offset+int_req_ec  # check for ir bit error.
		   beq.b   leave_with_error

		   btst	   &rtry,%d6		   # 16 collisions occurred?
		   bne.b   leave_with_error

		   addq.l  &1,%d7		   # incriment error flag.
		   mov.l  %d4,%d1		   # able to use network line?
		   and.w   &0x0300,%d1		    #   (idon but no tint).
		   cmpi.w  %d1,&0x0100
		   beq.b   leave_with_error
		   
		   addq.l  &1,%d7		   # incriment error flag.
		   mov.l  %d4,%d1		   # check to see if trans-
		   and.w   &0x0700,%d1		    #   mitted packet was
		   cmpi.w  %d1,&0x0300		    #   ever received (idon,
		   beq.b   leave_with_error	   #   tint, no rint).

		   addq.l  &1,%d7		   # incriment error flag.
		   mov.l  %d5,%d1		   # copy rmd1.
		   and.w   &0x1400,%d1		    # check oflo or buff err to
		   bne.b   leave_with_error	   #   see if received diff pack.
		   
		   addq.l  &1,%d7		   # incriment error flag.
		   andi.w  &0x4800,%d4		    # csr0 babl and merr 
		   cmpi.w  %d4,&0		   #   bits check.
		   bne.b   leave_with_error	   # branch if set.

		   addq.l  &1,%d7		   # incriment error flag.
		   btst	   &14,%d5		   # rmd1 error sum check.
		   bne.b   leave_with_error	   # branch if set. 

		   addq.l  &1,%d7		   # incriment error flag.
		   andi.l  &0xc000,%d6		    # check tmd3 for buff  
		   bne.b   leave_with_error	   #   or uflo error.

		   addq.l  &1,%d7		   # incriment error flag. 
		   cmpi.l  %d0,&0		   # check if trasmitted and
		   bne.b   leave_with_error	   #   received packets match.

no_ext_loop_error:  clr.l   %d7			   # no errors found.
		   mov.b  &1,1(%a0)	   # reset card before leave.
		   rts				   # return.

leave_with_error:   mov.b  &1,1(%a0)	   # reset card before leave.
		   rts				   # return.
 
 #***************************************************************************

 #***************************************************************************
 #									   *
 #     UTILITY SUBROUTINES:  CALCULATE_NV_CHECKSUM			   *
 #			    CARD_STOP					   *
 #			    COMPARE_PACKETS				   *
 #			    FORM_BYTE					   *
 #			    GET_TIME (AT FRONT, REPLACED BY CNO)	   *
 #			    INIT_LANCE_REGS				   *
 #			    LANCE_LOOP_TEST				   *
 #			    MOVE_PACKET_TO_CARD				   *
 #			    MOVE_STA_ADDR				   *
 #									   *
 #***************************************************************************

 #*****************************************************************************
 #     SUBROUTINE NAME: CALCULATE THE NOVRAM CHECKSUM			     *
 #									     *
 #     PURPOSE:								     *
 #     AFTER CLEARING NOVRAM AND RESETING THE CARD TO TRANSFER EAROM CONTENTS *
 #     TO NOVRAM, THIS SUBROUTINE CALCULATES THE CHECKSUM FOR THE SELECTED    *
 #     BANK OF NOVRAM.							     *
 #									     *
 #     ON ENTRANCE:							     *
 #	A1 CONTAINS THE ADDRESS OF THE NOVRAM.				     *
 #	D7 CONTAINS ERROR CODE 6 + THE ERROR OFFSET (IE. LOW EAROM BANK	     *
 #	  FAILURE).							     *
 #									     *
 #     ON EXIT:								     *
 #	D2 CONTAINS THE ACCUMULATED BYTE SUM.				     *
 #	D7 CONTAINS EITHER 6 OR 7 + THE ERROR OFFSET (IE. LOW OR HIGH BANK   *
 #	  EAROM).							     *
 #									     *
 #     REGISTERS CHANGED:						     *
 #	BY THIS ROUTINE:  D1, D2, D3, D7(IF HIGH BANK USED)		     *
 #	BY SUBROUTINES:	 A4, D0, D1					     *
 #*****************************************************************************

 #***************** ALGORITHM *************************************************
 #									     *
 #     CHECKSUM = (THE SUMMATION OF THE SELECTED BANK'S FIRST 15 CONCATENATED *
 #		  BYTES) MOD 256.					     *
 #									     *
 #     A CONCATENATED BYTE = | NIBBLE AT | NIBBLE AT |			     *
 #			    | WORD  N	| WORD N+2  |			     *
 #			    -------------------------			     *
 #		     BITS    7	       4 3	   0			     *
 #									     *
 #     NOTE: THE LOW BANK CARD ADDRESS = $C000, THE HIGH BANK CARD ADDRESS    *
 #	    = $C040.							     *
 #									     *
 #*****************************************************************************

                   set	   calc_nv_checksum,.

clear_novram:	   mov.l  &novram_size,%d3	   # clear novram before reset. 
		   subq.l  &2,%d3		   # novram size - 2 (loop count.)

clear_nram_loop:	   clr.w   0(%a1,%d3)		   # clear nibble in low byte.
		   subq.l  &2,%d3
		   bpl.b   clear_nram_loop

rset_the_card:	   mov.w  &1,(%a0)	   # write to reset reg.

		   btst	   &bank_flag,3(%a1)	   # check which bank.
		   beq.b   calc_checksum	   # branch if low bank.
		   addq.l  &1,%d7		   # set error flag to h bank.

calc_checksum:	   movq   &14,%d3		   # set loop count to 15.  
		   clr.l   %d2
		   clr.l   %d1

nv_more_bytes:	   bsr	   form_byte		   # get two nibbles.
		   add.b   %d0,%d2		   # sum returned bytes.
		   dbra	   %d3,nv_more_bytes

		   rts

 #*****************************************************************************

 #****************************************************************************
 #     SUBROUTINE NAME:	CARD STOP					    *
 #									    *
 #     PURPOSE:								    *
 #     THIS SUBROUTINE STOPS THE CARD BY SETTING THE RAP TO CSR0 AND SET-    *
 #     TING ITS STOP BIT THROUGH THE RDP.  IF WRITINE TO RAP FAILS TO RETURN *
 #     THE ACKNOWLEDGE BIT IN THE CARD'S STATUS REGISTER, D4 IS SET TO RE-   *
 #     PORT THE ERROR OTHERWISE D4 REMAINS CLEAR.  SINCE THE LOCK BIT IS	    *
 #     SET BEFORE ENTERING THIS SUBROUTINE, ONLY TEN ATTEMPTS TO WRITE TO    *
 #     THE RAP ARE ALLOWED (MORE THAN ONE ATTEMPT IS MADE IN CASE THE LANCE  *
 #     IS IN MASTER MODE INSTEAD OF SLAVE MODE WHEN THE FIRST ATTEMPTS ARE   *
 #     MADE).								    *
 #									    *
 #     ON ENTRANCE:							    *
 #	D7 CONTAINS ERROR CODE OF THE CALLING ROUTINE.			    *
 #									    *
 #     ON EXIT:								    *
 #	D4 CONTAINS THE CARD STOP RESULT.				    *
 #	D7 CONTAINS EITHER THE ERROR CODE OF THE CALLING ROUTINE OR 32 +    *
 #	  THE ERROR OFFSET IF THIS ROUTINE CAN'T ACCESS RAP (NO ACK).	    *
 #									    *
 #     REGISTERS CHANGED:						    *
 #	BY THIS ROUTINE:  D4, D5, D7(IF CAN'T ACCESS RAP (IE. NO ACK))	    *
 #	BY SUBROUTINES:	 N/A						    *
 #****************************************************************************
	      
         	   set	   card_stop,.			   # lock bit previously set.

		   clr.l   %d4
		   movq   &10,%d5		   # loop trys counter.

ack_loop_1:	   mov.w  &0,(%a2)	   # set rap = csr0.
		   subq.l  &1,%d5
		   bmi	   ack_error		   # > 10 attempts = error.
		   btst	   &2,3(%a0)	   # check acknowledge bit.
		   beq.b   ack_loop_1		   # branch if ack bit not set.

set_stop_bit:	   mov.w  &0x04,(%a3)	    # set stop bit in csr0.
		   cmpi.w  (%a3),&0x04	    # check if lance stopped.
		   bne.b   set_error

		   rts

ack_error:	   movq   &error_offset+ack_ec,%d7  # set error flag.
set_error:	   movq   &1,%d4		   # set error = true.

		   rts

 #*****************************************************************************

 #*****************************************************************************
 #     SUBROUTINE NAME:	COMPARE TRANSMITTED AND RECEIVED PACKETS	     *
 #									     *
 #     PURPOSE:								     *
 #     THIS SUBROUTINE COMPARES THE TRANSMITTED AND RECEIVED PACKETS. IF THE  *
 #     PACKETS MATCH, THE CONDITION CODE OF THE STATUS REGISTER WILL BE SET   *
 #     TO "EQUAL", OTHERWISE IT WILL BE SET TO "NOT EQUAL".		     *
 #									     *
 #     ON ENTRANCE:							     *
 #	A1 CONTAINS THE CARD'S SHARED RAM BASE ADDRESS.			     *
 #	D0 CONTAINS THE MUNBER OF WORDS TO COMPARE.			     *
 #									     *
 #     ON EXIT:								     *
 #	STATUS CONTAINS EQUAL OR NOT EQUAL (IE. PACKETS EQUAL (MATCH)).	     *
 #									     *
 #     REGISTERS CHANGED:						     *
 #	BY THIS ROUTINE:  A1, A4, D0, STATUS				     *
 #	BY SUBROUTINES:	 N/A						     *
 #*****************************************************************************

               	   set	    compare_packets,.

		   mova.l  %a1,%a4
		   adda.w   &tbuff_offset,%a1	   # transmit data buffer addr.
		   adda.w   &rbuff_offset,%a4	   # receive data buffer addr.

check_pac_word:	   cmpm.w   (%a4)+,(%a1)+
 #						  ; COMPARE PACKETS.
		   dbne	    %d0,check_pac_word	   # check next word.
		   
		   rts

 #****************************************************************************

 #***************************************************************************
 #     SUBROUTINE NAME:	FORM BYTE					   *
 #									   *
 #     PURPOSE:								   *
 #     THIS SUBROUTINE READS TWO CONSECUTIVE NIBBLES OF NOVRAM AND COMBINES *
 #     THEM SIDE BY SIDE TO FORM A BYTE (FIRST NIBBLE = BITS 4 TO 7).	   *
 #									   *
 #     ON ENTRANCE:							   *
 #	A1 CONTAINS THE CARD'S NOVRAM BASE ADDRESS.			   *
 #	D1 CONTAINS THE ADDRESS OF THE NOVRAM WORD TO READ.		   *
 #	D7 CONTAINS ERROR CODE 6 + THE ERROR OFFSET (IE. LOW EAROM BANK	   *
 #	  FAILURE CODE).						   *
 #									   *
 #     ON EXIT:								   *
 #	D0 CONTAINS THE NEWLY FORMED BYTE.				   *
 #	D1 CONTAINS THE ADDRESS OF THE NEXT NOVRAM WORD TO READ.	   *
 #	D7 CONTAINS EITHER 6 OR 7 + THE ERROR OFFSET (IE. LOW OR HIGH BANK *
 #	  EAROM).							   *
 #									   *
 #     REGISTERS CHANGED:						   *
 #	BY THIS ROUTINE:  A4, D0, D1					   *
 #	BY SUBROUTINES:	 N/A						   *
 #***************************************************************************

         	   set	  form_byte,.
		   clr.w   %d0
		   
		   mova.l %a1,%a4
		   btst	   &bank_flag,3(%a1)	  # check which bank.
		   beq.b   correct_bank		  # read low bank.
		   adda.l  &high_bank_offset,%a4	  # read high bank.

correct_bank:	   mov.b  1(%a4,%d1),%d0		  # get first nibble.
		   lsl.w   &8,%d0
		   mov.b  3(%a4,%d1),%d0		  # get next nibble.
		   lsl.b   &4,%d0		  # move nibbles side by side.
		   lsr.w   &4,%d0		  # shift them over to low byte.
		   addq.w  &4,%d1		  # adjust for next byte reads.

		   rts

 #*****************************************************************************

 #***************************************************************************
 #     SUBROUTINE NAME:	INITIALIZE THE LANCE'S REGISTERS		   *
 #									   *
 #     PURPOSE:								   *
 #     THIS SUBROUTINE INITIALIZES THE LANCE CSR0 THROUGH CSR1 REGISTERS	   *
 #     FOR PACKET TRANSMISSION.						   *
 #									   *
 #     REGISTERS CHANGED:						   *
 #	BY THIS ROUTINE:  NONE						   *
 #	BY SUBROUTINES:	 N/A						   *
 #***************************************************************************

               	   set	   init_lance_regs,.
		   
		   mov.w  &1,(%a2)	    # rap = csr1.
		   mov.w  &initblk,(%a3)	    # init block base address.
		   
		   mov.w  &2,(%a2)	    # rap = csr2.
		   mov.w  &0,(%a3)	    # initblk high addr bits.
		   
		   mov.w  &3,(%a2)	    # rap = csr3.
		   mov.w  &4,(%a3)	    # byte swap on.
		   
		   mov.w  &0,(%a2)	    # rap = csr0.
		   mov.w  &0x4b,(%a3)	     # start, init, inea, tdmd.
		   
		   rts

 #*****************************************************************************

 #*****************************************************************************
 #     SUBROUTINE NAME:	INTERNAL NON COLLISION LOOPBACK DRIVER		     *
 #									     *
 #     PURPOSE:								     *
 #     TO INITIALIZE THE ALLOTTED TIME FOR NON COLLISION INTERNAL LOOPBACKS,  *
 #     CALL THE ROUTINE WHICH PERFORMS THE LOOPBACK, AND CHECK THE RESULTS OF *
 #     THE LOOPBACK EXECUTION.						     *
 #									     *
 #     ON ENTRANCE:							     *
 #	D7 CONTAINS THE ERROR CODE OF THE LAST TEST.			     *
 #									     *
 #     ON EXIT:								     *
 #	D0 CONTAINS THE RESULT OF THE LOOPBACK TEST (BIT #0).		     *
 #	D6 CONTAINS TMD1.						     *
 #	D7 CONTAINS THE ERROR CODE OF THE CURRENT LOOPBACK TEST OR 18 + THE  *
 #	  ERROR OFFSET IF AN IR ERROR WAS FOUND IN THE GENERIC LOOPBACK TEST *
 #	  SUBROUTINE.							     *
 #									     *
 #     REGISTERS CHANGED:						     *
 #	BY THIS ROUTINE:  A1, D0, D2, D6, D7				     *
 #	BY SUBROUTINES:	 A1, A4, D0, D1, D4, D5, D6, D7, STATUS(COMP PACKS). *
 #*****************************************************************************

 #int_nonc_loop:	   movq   &int_crc_loop_time,%d2   # max loop time.
 # use busy loops instead of time outs....prabha 01/28/87 
int_nonc_loop:	   movq   &80,%d2                  # max loop count.
		   addq.l  &1,%d7		   # incriment error flag.
		   
		   bsr	   lance_loop_test	   # perform test.
		   
		   cmpi.w  %d0,&0		   # check for error from
		   bne	   int_nonc_loop_error	   #   lance loop test.
		   
		   btst	   &15,%d4		   # csr0 error sum check.
		   bne	   int_nonc_loop_error	   # branch if set.   

		   btst	   &14,%d5		   # rmd1 error sum check.
		   bne	   int_nonc_loop_error	   # branch if set. 

		   mova.l %a0,%a1		   # place shared ram
		   adda.l  &ram_offset,%a1	   #   address into a1.

		   mov.w  34(%a1),%d6		   # copy tmd1 to d6.
		   btst	   &14,%d6		   # tmd1 error sum check.
		   bne	   int_nonc_loop_error	   # branch if set. 

		   rts

int_nonc_loop_error: movq &1,%d0			   # set loop error flag.
		   rts

 #****************************************************************************

 #**************************************************************************
 #     SUBROUTINE NAME:	LANCE GENERIC LOOPBACK TEST			  *
 #									  *
 #     PURPOSE:								  *
 #     AFTER CALLING SUBROUTINES TO PREPARE A PACKET FOR TRANSFER AND TO	  *
 #     INITIALIZE THE LANCE REGISTERS FOR THE SELECTED INTERNAL LOOPBACK,  *
 #     THIS FUNCTION MONITORS THE LANCE DURING THE LOOPBACK ATTEMPT AND	  *
 #     CALLS A PROCEDURE TO COMPARE TRANSMITTED AND RECEIVED PACKETS WHEN  *
 #     APPROPRIATE.  THIS ROUTINE CHECKS FOR CORRECT CSR0 STATUS RESPONSE  *
 #     (IE. INT, TINT, RINT), IR BIT SET AFTER IDON, WHETHER OR NOT THE	  *
 #     LOOPBACK'S ALLOTTED TIME WAS EXCEEDED, AND WHETHER OR NOT THE PACK- *
 #     ETS MATCH (WHEN APPLICABLE).					  *
 #									  *
 #     ON ENTRANCE:							  *
 #	D0 CONTAINS THE PACKET LENGTH IN BYTES. THE LENGTH VARIES DUE TO  *
 #	  TO WHETHER OR NOT THE LANCE GENERATES THE CRC.		  *
 #	D1 CONTAINS THE LOOP OPERATION MODE WORD.			  *
 #	D2 CONTAINS THE LOOP COUNT TO PERFORM THE SELECTED TYPE OF	  *
 #	  LOOP.								  *
 #	D7 CONTAINS THE ERROR CODE OF THE CURRENT LOOPBACK TEST.	  *
 #									  *
 #     REGISTER USAGE:							  *
 #	D1: THE LAST RETURNED CLOCK TIME FROM THE SYSTEM CLOCK ROUTINE.	  *
 #									  *
 #     ON EXIT:								  *
 #	D0 CONTAINS THE RESULTS OF THIS TEST (BIT #0).			  *
 #	D4 CONTAINS CSR0.						  *
 #	D5 CONTAINS RMD1.						  *
 #	D6 CONTAINS TMD3.						  *
 #	D7 CONTAINS THE ERROR CODE OF THE CURRENT LOOPBACK TEST OR 18 +	  *
 #	  THE ERROR OFFSET IF THE IR BIT WON'T SET AFTER IDON.		  *
 #									  *
 #     REGISTERS CHANGED:						  *
 #	BY THIS ROUTINE:  A1, D0, D1, D4, D5, D6, D7(IF IR BIT NOT SET).  *
 #	BY SUBROUTINES:	 A1, A4, D0, D1, D4, D5, D7, STATUS(COMPARE PACKS)*
 #**************************************************************************

               	   set	   lance_loop_test,.
		   
initialize:	   bsr	   move_packet_to_card	   # copy data structures.
		   
		   btst	   &0,%d4	  # check to see if lance stopped.

		   bne	   loop_error		   # branch if set.
		   
		   bsr	   init_lance_regs	   # init csr0 through csr3.

		   mova.l %a0,%a1		   # place shared ram
		   adda.l  &ram_offset,%a1	   #   address into a1.
 # following lines commented out by prabha 01/27/87. Do not use 
 # get(network)time because the timer may not be started yet. 
 # Loop and check; d2 has the loop count

 #start_clock_time: bsr	   get_time
 #		   mov.l  %d1,%d0		   # d0 := start time
 #wait_check_loop:  bsr	   get_time

start_clock_time:  mov.l  %d2,%d0         # loop count
wait_check_loop:   mov.l  &1000,%d1       # start of wait loop.
waste_time:        nop                    # this loop provides approx.
                   nop                    # 10 x 1000 clock cycles.
                   nop                    # On a 25 Mhz clock it is equivalent
                   nop                    # to 0.4 msecs. d0 has an outer loop
                   nop                    # count that multiplies this number to
                   sub.w   &1,%d1         # provide a reasonable delay. The 
                   bpl.b   waste_time     # may need adjustments based on clock.

		   mov.w  (%a3),%d4	   # copy csr0 to d4.
		   btst	   &2,3(%a0)	   # make sure csr0
 #		   beq.b   check_time      #   was successfully read.
		   beq.b   check_loop_count  # check loop count 

		   btst	   &idon,%d4		   # check for init done int. 
 #		   beq.b   check_time              # branch if not set 
		   beq.b   check_loop_count	   # branch if not set. 
		     
		   btst	   &ir_bit,3(%a0)	   # check ir bit in cntrl reg.
		   beq.b   ir_error		   # branch if not set.	 

		   btst	   &tint,%d4		   # check for transmitter int.
		   bne.b   exit_wait_loop	   # exit loop if set

 #check_time:	   sub.l   %d0,%d1		  #| d1 := time - starttime
 #		   cmp.l   %d2,%d1	  #| (time-starttime) > TIMEOUT ?
 #		   bcs.b   loop_error		  #|
 #		   bra.b   wait_check_loop	  #|
check_loop_count:  sub.w   &1,%d0
                   bmi.b   loop_error
                   bra.b   wait_check_loop

exit_wait_loop:	   mov.w  26(%a1),%d5		   # copy rmd1 into d5.

collision_check:	   mov.w  38(%a1),%d6		   # copy tmd3 into d6.
		   btst	   &rtry,%d6		   # check for internal
		   bne.b   collision_occurred	   #   and external colls.   

 #****************  END WAIT_CHECK_LOOP, CHECK RINT SEPARATELY TO SAVE TIME.

		   movq   &30,%d0
rint_check:	   subq.l  &1,%d0
		   bmi.b   loop_error	 
		   mov.w  (%a3),%d4	   # copy csr0 to d4
		   btst	   &2,3(%a0)	   # check ack, make sure csr0
		   beq.b   rint_check		   #   was successfully read.
		   
		   btst	   &rint,%d4		   # check for receive int.
		   bne.b   compare_packs	   # branch if set.  
		   bra.b   rint_check	 

compare_packs:	   mov.w  words_to_compare,%d0	   # words to compare.
		   
		   bsr	   compare_packets	   #   check packet with crc.
		   bne.b   packet_error

no_loop_error:	   mov.w  &0,%d0		   # clear error flag.
		   
		   rts

ir_error:	   movq   &error_offset+int_req_ec,%d7  # set error flag.
loop_error:	   mov.w  26(%a1),%d5		   # copy rmd1 into d5.
		   mov.w  38(%a1),%d6		   # copy tmd3 into d6.

                   set	   collision_occurred,. 
            	   set	   packet_error,.
		   
		   mov.w  &1,%d0		   # set error flag.

		   rts

 #***************************************************************************

 #***************************************************************************
 #     SUBROUTINE NAME: MOVE PACKET'S DATA TO THE CARD'S SHARED RAM	   *
 #									   *
 #     PURPOSE:								   *
 #     THIS PROCEDURE COPIES THE PACKET DATA TO BE TRANSMITTED INTO THE	   *
 #     SHARED RAM ON THE DIO-LAN CARD.					   *
 #									   *
 #     ON ENTRANCE:							   *
 #	D0 CONTAINS THE LENGTH OF THE PACKET TO BE TRANSMITTED. THE LENGTH *
 #	  VARIES DUE TO WHETHER OR NOT THE LANCE WILL GENERATE THE CRC.	   *
 #	D1 CONTAINS THE LOOP OPERATION MODE WORD.			   *
 #	D7 CONTAINS THE CURRENT TEST'S ERROR CODE.			   *
 #									   *
 #     ON EXIT:								   *
 #	D4 CONTAINS THE RESULT OF WHETHER OR NOT THE CARD STOPPED.	   *
 #	D7 CONTAINS THE CURRENT TEST'S ERROR CODE OR 32 + THE ERROR OFF-   *
 #	  SET IF THE LANCE'S RAP COULD NOT BE ACCESSED.			   *
 #									   *
 #     REGISTERS CHANGED:						   *
 #	BY THIS ROUTINE:  A1, A4, D1					   *
 #	BY SUBROUTINES:	 D4, D5, D7(POSSIBLY IN CARD_STOP).		   *
 #***************************************************************************

                     set     move_packet_to_card,.
		   
		   mov.b  &8,3(%a0)	   # set lock bit (slave mode).
		   
		   bsr	   card_stop		   # stop the lance.
		   
		   mov.b  &0,3(%a0)	   # clear lock bit.

		   mova.l %a0,%a4		   # copy card ram address.
		   adda.l  &ram_offset,%a4	   # add shared ram offset.
		   lea	   lance_test_data,%a1	   # copy data start addr.
		   mov.w  %d1,(%a4)+		   # load operation mode.
		   movq   &51,%d1		   # loop count = 52.

move_data_loop:	   mov.w  (%a1)+,(%a4)+		   # transfer data from
		   dbra	   %d1,move_data_loop	   #   program to shared ram.

		   mov.l  &tmd2_offset,%d1
		   mov.w  %d0,0(%a0,%d1.l)	   # specify packet length.

		   rts
		   
 #****************************************************************************

 #***************************************************************************
 #     SUBROUTINE NAME:	MOVE THE CARD'S STATION ADDRESS			   *
 #									   *
 #     PURPOSE:								   *
 #     THIS SUBROUTINE READS CONSECUTIVE NIBBLES OF THE STATION'S ADDRESS   *
 #     STORED IN NOVRAM, CONCATENATES NIBBLE N+2 TO NIBBLE N (NIBBLE N =	   *
 #     BITS 4 TO 7), AND FIRST STORES THESE STATION ADDRESS BYTES INTO THE  *
 #     INITIALIZATION BLOCK AT TBUF, THEN STORES THE SAME STATION ADDRESS   *
 #     INTO THE INITIALIZATION BLOCK AFTER BYTE SWAPPING IT.		   *
 #									   *
 #     REGISTERS ON ENTRANCE:  DON'T CARE.				   *
 #									   *
 #     REGISTERS ON EXIT:  DON'T CARE.					   *
 #									   *
 #     REGISTERS CHANGED:						   *
 #	BY THIS ROUTINE:  A1, A4, D0, D1, D2				   *
 #	BY SUBROUTINES:	 N/A						   *
 #***************************************************************************

             	   set	  move_sta_addr,.
		   
		   clr.w   %d0
		   movq   &4,%d1		  # offset to first nibble.
		   movq   &5,%d2		  # loop count (= 5 + 1).

		   lea	   tbuf,%a4		  # location to store sta addr.
		   mova.l %a0,%a1		  # place address of 
		   adda.l  &novram_offset,%a1	  #   novram into a1.
		   
		   btst	   &bank_flag,3(%a1)	  # check which bank.
		   beq.b   load_tbuf		  # read low bank.
		   adda.l  &high_bank_offset,%a1	  # read high bank.

load_tbuf:	   mov.b  1(%a1,%d1),%d0		  # get first nibble.
		   lsl.w   &8,%d0
		   mov.b  3(%a1,%d1),%d0		  # get next nibble.
		   lsl.b   &4,%d0		  # move nibbles side by side.
		   lsr.w   &4,%d0		  # shift them over to low byte.
		   
		   addq.l  &4,%d1		  # adjust for next byte reads.

		   mov.b  %d0,(%a4)+		  # store addr bytes.
		   dbra	   %d2,load_tbuf	    
		   
 #*****************************************************************************

		   lea	   tbuf,%a4
		   lea	   init_block,%a1
		   movq   &2,%d2

load_init_block:	   mov.w  (%a4)+,%d0		  # get first address word.
		   rol.w   &8,%d0		  # swap bytes.
		   mov.w  %d0,(%a1)+		  # store byte swapped word.
		   dbra	   %d2,load_init_block

		   rts

 #***************************************************************************

 #		    end

