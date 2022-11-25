 # HPUX_ID: @(#)odys.s	01.1		89/04/26


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


 #*****************************************************************************
 #									     *
 #   *****   ****    *     * 		     ***			     *
 #   *   *   *   *    *   *		    *	*			     *
 #   *   *   *    *    * *	    ***	    *	     ***    ** *    ** *     *
 #   *   *   *	  *     *		    *	    *	*   * *	*   * *	*    *
 #   *   *   *    *     *        	    *	    *	*   * *	*   * *	*    *
 #   *   *   * 	 *      *     	 	    *	*   *	*   * *	*   * *	*    *
 #   *****   ****	*             	     ***     ***    * *	*   * *	*    *
 #									     *
 #*****************************************************************************

 #*****************************************************************************
 #									     *
 #    Purpose -- Assembly language routines which interfaces to	C.	     *
 #		 These routines	constitute a low-level driver for	     *
 #		 Programmable Datacomm Interface Cards which uses	     *
 #		 shared	buffers	to transfer data (98628	& 98629).	     *
 #		 These routines	should run in the Kernel mode in Unix.	     *
 #		 Thus they cannot be directly called by	users.		     *
 #		 These routines	are also lacking in ISR	handlers, so the     *
 #		 higher	level routine should provide one.		     *
 #    History -- These routines	originated as Pascal 2.1 DC drivers.	     *
 #		 Latest revision 8-15-83				     *
 # 		    lock everything from C-callable routines		     *
 #		They are also used for the 98659A and have been		     *
 #		been modified by John Hansen for that purpose                *
 #		pdi on system calls has been changed to ody		     *
 #		Only noticable changes made were in the try/recover sections *
 #    Related								     *
 #      Files -- /usr/src/uts/cf/conf.c					     *
 #		 /usr/src/uts/iolib/kernel.c				     *
 #		 <sys/pdi.h>						     *
 #									     *
 #*****************************************************************************

 #*****************************************************************************
 #									     *
 #    MODULE summary							     *
 #									     *
 #	BUFFERS	  :  contains routines which sets up buffer pointers.	     *
 #	RxBUFFER  :  contains routines which read the receive buffers.	     *
 #	TxBUFFER  :  contains routines which write the transmit	buffers.     *
 #	COMMAND	  :  contains C-callable routines.			     *
 #									     *
 #*****************************************************************************
 

 #*****************************************************************************
 #									     *
 #    The following were changed from Pascal 2.1:			     *
 #	 1.  Transfer was not needed and was deleted.			     *
 #	 2.  Routines which used to interface to Pascal	was changed to	     *
 #	     interface to C.  These routines were almost rewritten so	     *
 #	     that they do not block.  These are:  direct_control,	     *
 #	     direct_status, control_bfd, enter_data, output_data, and	     *
 #	     output_end.						     *
 #	 3.  Interrupts are disabled at the highest C-procedure level	     *
 #	 4.  Select_code_table locations changed.			     *
 #	 5.  Escapes in	Pascal were imitated.				     *
 #									     *
 #									     *
 #    Proposed enhancements:						     *
 #	 1.  Store the buffer pointers at reset	time instead of	finding	     *
 #	     it	each time.  This could provide some speed increase,	     *
 #	     especially	for character oriented transfers.		     *
 #	 2.  Change eir	and dir	so that	they are in-line.		     *
 #									     *
 #									     *
 #*****************************************************************************

 #*****************************************************************************
 #									     *
 #		Entry points						     *

	global	_ody_enter
	global	_ody_output
	global	_ody_status
	global	_ody_control
	global	_ody_qcontrol
	global	_ody_init
	global  _ody_inqueue

 #*****************************************************************************
 #		       Data Comm card RAM locations			     *
 #		     (byte offsets from	base address)			     *

        	set	RESET_ID,0x000000
       		set	INT_DMA,0x000002
         	set	SEMAPHORE,0x000004
        	set	INT_COND,0x004000
       		set	COMMAND,0x004002
        	set	DATA_REG,0x004004
            	set	PRIMARY_ADDR,0x004006
    		set	DSDP,0x004008
          	set	ERROR_CODE,0x00400C

 #*********************** Data Structures Descriptor **************************

          	set	ATTRIBUTES,0x000000
             	set	TR_QUEUE_ADDR,0x000002
           	set	PRIM_0_ADDR,0x000006

 #**************************** Queue ******************************************

               	set	TXENDBLOCKSPACE,0x000000
               	set	RXDATABUFF_NUMB,0x000001
      		set	TXBUFF,0x000004
      		set	RXBUFF,0x000024

         	set	CTRL_AREA,0x000000
         	set	DATA_AREA,0x000010

 #***************************** Buffer record *********************************

    		set	ADDR,0x000000
    		set	SIZE,0x000004
    		set	FILL,0x000008
     		set	EMPTY,0x00000C

 #***************************** Control block **********************************

       		set	POINTER,0x000000
         	set	TERMFIELD,0x000004
         	set	MODEFIELD,0x000006
           	set	CTRLBLKSIZE,0x000008

 #************************ INT_COND bit offsets *******************************

         	set error_int,0	#| Bits for interrupt
      		set rx_int,1	#| register
      		set tx_int,2
           	set ON_INTR_int,3
            	set RC_reset_int,4
         	set trace_int,5

 #*****************************************************************************

 #*****************************************************************************
 #			  select_code_table				     *
 #		holds "global" variables for each card			     *
 #	       the following are offsets for this table			     *

      		set c_addr,00 		#.. 03  | 4 byte pointer
           	set which_RXbuf,04	#       | byte
                                        # not used 05
                set last_term_mode,06   #.. 07  | two bytes
         	set last_term,06
         	set last_mode,07

 #									     *
 #*****************************************************************************

 #*****************************************************************************
 #									     *
 #	     ***     ***    *	*   *	*    ***    *	*   ****	     *
 #	    *	*   *	*   ** **   ** **   *	*   *	*    *	*	     *
 #	    *	    *	*   * *	*   * *	*   *	*   **	*    *	*	     *
 #	    *	    *	*   * *	*   * *	*   *****   * *	*    *	*	     *
 #	    *	    *	*   *	*   *	*   *	*   *  **    *	*	     *
 #	    *	*   *	*   *	*   *	*   *	*   *	*    *	*	     *
 #	     ***     ***    *	*   *	*   *	*   *	*   ****	     *
 #									     *
 #*****************************************************************************
 #*****************************************************************************
 #									     *
 #     Stack Usage:  all C-interface routines will set the stack	     *
 #		    to the following upon entry	to the routine.		     *
 #									     *
 #		    HIGH						     *
 #		    +--------------------------------+	   -----	     *
 #		    |				     |			     *
 #		    :  parameters		     :	   on		     *
 #		    |				     |	   entry	     *
 #		    +--------------------------------+			     *
 #		    |  return address		     |			     *
 #		    +================================+	   -----	     *
 #		    |				     |			     *
 #		    :  saved registers (9 total)     :	   movem.l	     *
 #		    |				     |			     *
 #		    +--------------------------------+	   -----	     *
 #		    |  old a5			     |	        	     *
 #	 a5 ------> +--------------------------------+	        	     *
 #		    |  term_and_mode (two bytes)     |      	             *
 #	            +--------------------------------+	   link 	     *
 #		    |  data_address   		     |	      		     *
 #	            +--------------------------------+	        	     *
 #		    |  data_number    		     |	      		     *
 #	            +--------------------------------+	   -----	     *
 #		    |  recover address		     |	   pea		     *
 #	 sp ------> +--------------------------------+	   -----	     *
 #		    LOW							     *
 #									     *
 #*****************************************************************************
 #									     *
 #     Register not saved:						     *
 #									     *
 #	 a0,a1,d0,d1  --  are scratch registers	and do not need	to be	     *
 #			  saved.					     *
 #	 a5	      --  used for TRY-RECOVER saved by	link instruction.    *
 #	 a6	      --  saved	by routines using it.			     *
 #									     *
 #*****************************************************************************

 #*****************************************************************************
 #									     *
 #     Parameter	offsets	from a5					     *
 #									     *

    		set	desc,44		#| pointer to global variable
   		set	buf,48		#| pointer to buffer area
       		set	address,48	#| card base address
      		set	nbytes,52	#| number of bytes in buffer area
       		set	statptr,56	#| where to put term and mode
        	set	register,48+3	#| pseudo-register number (byte)
     		set	value,52+3	#| value of control/status (byte)
        	set	valueptr,52	#| address of returned value

 #********************** local variables **************************************

             	set 	term_and_mode,-2 #.. -1	| Encompasses the two below
    		set 	term,-2     	 #      | byte
    		set 	mode,-1     	 #      | byte
            	set     data_address,-6  #.. -3	| pointer
           	set     data_number,-10  #.. -7	| integer

       		set	recover,-14 #.. -11	| recover address offset from a5

 #********************* error mnemonics ***************************************

            	set	carddown_err,-1		#| semaphore locked error
          	set	nodata_err,-2		#| no control block error
          	set	noroom_err,-2		#| no room to transmit error
            	set	overflow_err,-3		#| too much data	error

 #********************* other constants ***************************************

       		set	 ctrlbit,1      #| bit (from RX_stuff_avail) indicating
 #					 | if control blocks are available
        	set	endblock,0x0500	#| TERM and MODE of an END block


 #*****************************************************************************
 #									     *
 #     ROUTINE escape:  Routine which "escapes" to the last try-recover	     *
 #	      ======   block.  See the stack usage for the implementation    *
 #		       of try-recover in this driver.			     *
 #									     *
 #     At entry:							     *
 #	      a5.l = points to recover information			     *
 #									     *
 #     Upon exit:							     *
 #	      escape is	performed, the recover routine is called	     *
 #									     *
 #*****************************************************************************
escape:

	movea.l	recover(%a5),%a0	#| jump to the recover routine
	jmp	(%a0)			#| jump to the recover routine


 #*****************************************************************************
 #									     *
 #     ROUTINE escape_handler:  Routine which implements the escape 	     *
 #	      ==============   function.  This routine is specialized	     *
 #			       so that it returns a CARD_DOWN error.	     *
 #			       This routine cooperates with escape.	     *
 #									     *
 #*****************************************************************************
escape_handler:

	movq	&carddown_err,%d0	#| return error condition
	mov.b	&0x80,INT_DMA(%a3)	#| enable interrupts
	unlk	%a5
	movm.l	(%sp)+,%a2-%a4/%d2-%d7	#| restore registers
	rts


 #*****************************************************************************
 #									     *
 #     int ody_inqueue( desc )				     		     *
 #									     *
 #     struct ody_global	*desc;					     *
 #									     *
 #----------------------------------------------------------------------------*
 #									     *
 #     This routine will return then number of characters in the input buffer*
 #	  for a ody card.						     *
 #     The parameters are:						     *
 #	  desc	--  ody	internal variables.				     *
 #     This routine returns the number of bytes actually in the buffer       *
 #     (this can	be zero) or a negative number indicating an error:   *
 #	  -1	--  the	card does not respond (	semaphore locked ).	     *
 #									     *
 #*****************************************************************************
 #*****************************************************************************
 #									     *
 #     Algorithm:							     *
 #									     *
 #	 set up	TRY block						     *
 #	     get parameters						     *
 #	     RETURN(RX_BUFF_bytes)					     *
 #	 RECOVER							     *
 #	     RETURN(-1)							     *
 #									     *
 #*****************************************************************************
_ody_inqueue:

	movm.l	%a2-%a4/%d2-%d7,-(%sp)	#| save registers
	link	%a5,&-10		#| save context
	pea	escape_handler		#| where to jump if escape

	mova.l	desc(%a5),%a4		#| a4 := pointer to attributes
	mova.l	c_addr(%a4),%a3		#| a3 := card address
	clr.b	INT_DMA(%a3)		#| disable interrupts
	bsr	find_RXBUF		#| a2 := RX buffer descriptor address

	bsr	RX_stuff_avail		#| take care of term=255 control blocks

	bsr	RX_BUFF_bytes		#| d0 := number of characters
	mov.b	&0x80,INT_DMA(%a3)	#| enable interrupts
	unlk	%a5			#| restore stack
	movm.l	(%sp)+,%a2-%a4/%d2-%d7	#|    and registers
	rts

 #*****************************************************************************
 #									     *
 #     int ody_control( desc, register, value )				     *
 #									     *
 #     struct ody_global *desc;						     *
 #     int register, value;						     *
 #									     *
 #----------------------------------------------------------------------------*
 #									     *
 #     This procedure does a direct control to the ody device.		     *
 #     (direct means that it is not queued).				     *
 #     This routine returns zero if the control was successful		     *
 #     or a negative number indicating an error: 			     *
 #	  -1	--  the	card does not respond (	COMMAND	was not	cleared	).   *
 #									     *
 #*****************************************************************************
_ody_control:

	movm.l	%a2-%a4/%d2-%d7,-(%sp)	#| save registers
	link	%a5,&-10		#| save context
	pea	escape_handler		#| where to jump if escape

	mova.l	desc(%a5),%a4		#| a4 := pointer to attributes
	mova.l	c_addr(%a4),%a3		#| a3 := card address
	clr.b	INT_DMA(%a3)		#| disable interrupts

	mov.b	value(%a5),DATA_REG(%a3)	#| set value
	mov.b	register(%a5),COMMAND(%a3)	#| set register

 #
 #   Wait until it is done ( COMMAND = 0 )
 #

	mov.l	&10000,%d2		#| set loop counter
	pea	100.w			#|   to loop for 1 second maximum
ctloop:	tst.b	COMMAND(%a3)
	beq.b	ctldone
	jsr	_snooze			#| idle for 100 uS
	subq.l	&1,%d2
	bne.b	ctloop
	bra	escape			#| escape if counter expired

ctldone:	clr.l	%d0		#| return with no problems
	mov.b	&0x80,INT_DMA(%a3)	#| enable interrupts
	unlk	%a5			#| restore stack
	movm.l	(%sp)+,%a2-%a4/%d2-%d7	#|    and registers
	rts

 #*****************************************************************************
 #									     *
 #     int ody_status( desc, reg )					     *
 #									     *
 #     struct ody_global *desc;						     *
 #     int reg;								     *
 #									     *
 #----------------------------------------------------------------------------*
 #									     *
 #     This function examines the status of a ody device.		     *
 #     Most status are device dependent.  But the following registers	     *
 #     are device independent and executed by the drivers:		     *
 #									     *
 #	  0:  Gives ID							     *
 #	  1:  Returns hardware interrupt status	( 1=ON,	0=OFF)		     *
 #	  2:  Returns 0	if no activity is on this card			     *
 #	  5:  Returns state of RX buffer				     *
 #	  9:  Returns last entered TERM					     *
 #	 10:  Returns last entered MODE					     *
 #	 11:  Returns the amount of space available in outbound	queue	     *
 #       16:  Returns number of bytes loaded in outbound queue               *
 #									     *
 #									     *
 #     This routine returns zero if the status was successful		     *
 #     or a negative number indicating an error: 			     *
 #	  -1	--  the	card does not respond (	COMMAND	was not	cleared	).   *
 #									     *
 #*****************************************************************************
_ody_status:

	movm.l	%a2-%a4/%d2-%d7,-(%sp)	#| save registers
	link	%a5,&-10		#| save context
	pea	escape_handler		#| where to jump if escape

	mova.l	desc(%a5),%a4		#| a4 := pointer to attributes
	mova.l	c_addr(%a4),%a3		#| a3 := card address
	clr.b	INT_DMA(%a3)		#| disable interrupts

	clr.l	%d0			#| clear top half of result register
	mov.b	register(%a5),%d0	#| d0 := register number

 #
 #   Check for intercepted registers
 #

;;	beq.b	sts0			#|Only 11 and 16 are presently used
;;	cmpi.b	%d0,&1			#|Don't need to spend time on the
;;	beq.b	sts1			#|other intercepted registers
;;	cmpi.b	%d0,&2
;;	beq.b	sts2
;;	cmpi.b	%d0,&5
;;	beq.b	sts5
;;	cmpi.b	%d0,&9
;;	beq.b	sts9
;;	cmpi.b	%d0,&10
;;	beq.b	sts10
	cmpi.b	%d0,&11
	beq.b	sts11
	cmpi.b	%d0,&16
	beq.b	sts16

 #
 #   Enquire status from card
 #

	bset	&7,%d0			#| set bit 7 to indicate status request
	mov.b	%d0,COMMAND(%a3)	#| set register

 #
 #   Wait until it is done ( COMMAND = 0 )
 #

	mov.l	&10000,%d2		#| set loop counter
	pea	100.w			#|   to loop for 1 second maximum
qcloop:	tst.b	COMMAND(%a3)
	beq.b	qcdone
	jsr	_snooze			#| idle for 100 uS
	subq.l	&1,%d2
	bne.b	qcloop
	bra	escape			#| escape if counter expired
qcdone:	clr.l	%d0
	mov.b	DATA_REG(%a3),%d0
	bra	gotsts

;;sts0:	mov.b	RESET_ID(%a3),%d0
;;	bra.b	gotsts

;;sts1:	mov.b	&0x01,%d0		#| Interrupts are always enabled except 
;;	bra.b	gotsts			#| in utilities they are always disabled

;;sts2:	clr.b	%d0			#| No overlapping done by utilities
;;	bra.b	gotsts

;;sts5:	bsr	find_RXBUF
;;	bsr	RX_stuff_avail		#| d0 := result
;;	bra.b	gotsts

;;sts9:	mov.b	last_term(%a4),%d0
;;	bra.b	gotsts

;;sts10:	mov.b	last_mode(%a4),%d0
;;	bra.b	gotsts

sts11:	bsr	find_TXBUF		#| check for at least 3 control blocks
	bsr	find_CTRL_AREA		#|
	bsr	TXCTRLBUFFroom		#|
	clr.b	%d0			#|   assume zero result
	subi.l	&12,%d3			#|
	blt.b	gotsts

	bsr	find_DATA_AREA
	bsr	TXDATABUFFroom
	mov.w	%d3,%d0
	bra.b	gotsts

sts16:	bsr	find_TXBUF		#|a2.l base address -a1, d4, d5
	bsr	find_CTRL_AREA		#|a1.l ctrl area base address
					#|d5.l buff_size
	bsr	TXCTRLBUFFroom 		#|d3.l bytes available -1
	addq.l	&1,%d3
	sub.l	%d3,%d5			#|d5.l bytes still in ctrl buffer
	mov.l	%d5,%d1			#|d1.l bytes still in ctrl buffer
	bsr	find_DATA_AREA
	bsr	TXDATABUFFroom
	addq.l	&1,%d3
	sub.l	%d3,%d5			#|d3.l bytes still in data buffer
	mov.l	%d5,%d0			#|d0.l bytes still in data buffer
	add.l	%d1,%d0			#|d0.l total bytes remaining
#	bra.s	gotsts


gotsts:
 #					| d0 should = result
	mov.b	&0x80,INT_DMA(%a3)	#| enable interrupts
	unlk	%a5			#| restore stack
	movm.l	(%sp)+,%a2-%a4/%d2-%d7	#|    and registers
	rts

 #*****************************************************************************
 #									     *
 #     int ody_qcontrol( desc, register, value ) 			     *
 #									     *
 #     struct ody_global *desc;						     *
 #     int register, value;						     *
 #									     *
 #----------------------------------------------------------------------------*
 #									     *
 #     This procedure queues a control block to the ody device, to	     *
 #     be executed later.						     *
 #									     *
 #     This routine returns zero if the control was successful		     *
 #     or a negative number indicating an error:	 		     *
 #	  -1	--  the	card does not respond (	SEMAPHORE was locked )	     *
 #	  -2	--  the	card does not have room	to write a control block     *
 #		    (at	this time)					     *
 #									     *
 #*****************************************************************************
_ody_qcontrol:

	movm.l	%a2-%a4/%d2-%d7,-(%sp)	#| save registers
	link	%a5,&-10			#| save context
	pea	escape_handler		#| where to jump if escape

	mova.l	desc(%a5),%a4		#| a4 := pointer to attributes
	mova.l	c_addr(%a4),%a3		#| a3 := card address
	clr.b	INT_DMA(%a3)		#| disable interrupts

 #
 #   Check for room in transmit queue
 #

	bsr	find_TXBUF		#| a2 := pointer to TX area
	bsr	find_CTRL_AREA		#| d5 := TX CTRL BUFF size
	bsr	TXCTRLBUFFroom		#| d3 := number of bytes left
	tst.l	%d3			#| IF no room left
	beq.b	no_room2		#|    THEN goto error

 #
 #   Enqueue the	control	block
 #

	mov.b	register(%a5),term(%a5)	#| set register
	mov.b	value(%a5),mode(%a5)	#| set value
	bsr	putctrlblk		#| enqueue the control block

	clr.l	%d0			#| return with no problems
	mov.b	&0x80,INT_DMA(%a3)	#| enable interrupts
	unlk	%a5			#| restore stack
	movm.l	(%sp)+,%a2-%a4/%d2-%d7	#|    and registers
	rts

no_room2:

	movq	&noroom_err,%d0		#| return error condition
	mov.b	&0x80,INT_DMA(%a3)	#| enable interrupts
	unlk	%a5			#| restore stack
	movm.l	(%sp)+,%a2-%a4/%d2-%d7	#|    and registers
	rts

 #*****************************************************************************
 #									     *
 #     int ody_enter( desc, buf, nbyte, stat )				     *
 #									     *
 #     struct ody_global *desc;						     *
 #     char *buf;							     *
 #     unsigned nbyte;							     *
 #     unsigned short *stat						     *
 #									     *
 #----------------------------------------------------------------------------*
 #									     *
 #     This routine will transfer characters from a ody card.		     *
 #     It will transfer: 						     *
 #	  -  up	until the first	control	block and consume the control block  *
 #	  -  up	to the end of the buffer				     *
 #	  -  nbytes							     *
 #     The parameters are:						     *
 #	  desc	--  ody	internal variables.				     *
 #	  buf	--  buffer to transfer data.				     *
 #	  nbyte	--  maximum size of buffer.				     *
 #	  stat	--  the	"term and mode"	of the control block entered	     *
 #     This routine returns the number of bytes actually transferred	     *
 #     (this can	be zero) or a negative number indicating an error:   *
 #	  -1	--  the	card does not respond (	semaphore locked ).	     *
 #									     *
 #*****************************************************************************
 #*****************************************************************************
 #									     *
 #     Algorithm:							     *
 #									     *
 #	 set up	TRY block						     *
 #	     get parameters						     *
 #	     datasize := min( RX_BUFF_bytes, nbyte )			     *
 #	     REPEAT							     *
 #		GETCHAR							     *
 #	     UNTIL datasize bytes are read				     *
 #	     RETURN(datasize)						     *
 #	     IF	ctrlblknext THEN BEGIN					     *
 #		getctrlblk						     *
 #		stat :=	term_and_mode;					     *
 #	     ELSE stat := 0;						     *
 #	 RECOVER							     *
 #	     RETURN(-1)							     *
 #									     *
 #*****************************************************************************
_ody_enter:

	movm.l	%a2-%a4/%d2-%d7,-(%sp)	#| save registers
	link	%a5,&-10	 	#| save context
	pea	escape_handler		#| where to jump if escape

	mova.l	desc(%a5),%a4		#| a4 := pointer to attributes
	mova.l	c_addr(%a4),%a3		#| a3 := card address
	clr.b	INT_DMA(%a3)		#| disable interrupts
	bsr	find_RXBUF		#| a2 := RX buffer descriptor address

	bsr	RX_stuff_avail		#| take care of term=255 control blocks

	bsr	RX_BUFF_bytes		#| d0 := number of characters
	cmp.l	%d0,nbytes(%a5)		#| IF number of characters < nbytes
	ble.b	set_ent			#|
	mov.l	nbytes(%a5),%d0		#| THEN d0 := nbytes

set_ent:	mov.l	%d0,data_number(%a5)	#| set up parameter for GETCHAR
	mov.l	%d0,-(%sp)		#| save number of characters
	mov.l	buf(%a5),data_address(%a5)

ent_loop:

	bsr	getchars		#| REPEAT getchar
	tst.l	data_number(%a5)	#|    until number of char = 0
	bne.b	ent_loop		#|

	bsr	ctrlblknext		#| if next is control block
	tst.b	%d0			#|
	beq.b	no_blk			#|
	bsr	getctrlblk		#| consume the control block
	mova.l	statptr(%a5),%a0
	mov.w	term_and_mode(%a5),(%a0)#| return the entered term & mode
	mov.w  term_and_mode(%a5),last_term_mode(%a4)
	bra.b	ent_end
no_blk:
	mova.l	statptr(%a5),%a0	#| no term and mode
	clr.w	(%a0)			#|
	clr.w	last_term_mode(%a4)
ent_end:
	mov.l	(%sp)+,%d0		#| return number of chars transferred
	mov.b	&0x80,INT_DMA(%a3)	#| enable interrupts
	unlk	%a5			#| restore stack
	movm.l	(%sp)+,%a2-%a4/%d2-%d7	#|    and registers
	rts

 #*****************************************************************************
 #									     *
 #     int ody_output( desc, buf, nbyte )				     *
 #									     *
 #     struct ody_global *desc;						     *
 #     char *buf;							     *
 #     unsigned nbyte;							     *
 #									     *
 #----------------------------------------------------------------------------*
 #									     *
 #     This routine will transfer up to nbyte characters to a ody card.	     *
 #     The variables are:						     *
 #	  desc	--  ody	internal variables.				     *
 #	  buf	--  buffer to transfer data.				     *
 #	  nbyte	--  maximum size of buffer.				     *
 #     This routine returns the actual number of characters transferred	     *
 #     (can be zero) or a negative number indicating an error:		     *
 #	  -1	--  the	card does not respond (	semaphore locked ).	     *
 #									     *
 #*****************************************************************************
 #*****************************************************************************
 #									     *
 #     Algorithm:							     *
 #									     *
 #	 set up	TRY block						     *
 #	     get parameters						     *
 #	     datasize =	min( TXDATABUFFroom, nbyte )			     *
 #	     REPEAT							     *
 #		putchars						     *
 #	     UNTIL datasize bytes are written				     *
 #	     RETURN(datasize)						     *
 #	 RECOVER							     *
 #	     RETURN(-1)							     *
 #									     *
 #*****************************************************************************
_ody_output:

	movm.l	%a2-%a4/%d2-%d7,-(%sp)	#| save registers
	link	%a5,&-10		#| save context
	pea	escape_handler		#| where to jump if escape

	mova.l	desc(%a5),%a4		#| a4 := pointer to attributes
	mova.l	c_addr(%a4),%a3		#| a3 := card address
	clr.b	INT_DMA(%a3)		#| disable interrupts

	bsr	find_TXBUF		#| a2 := pointer to TX area
	bsr	find_DATA_AREA		#| d5 := TX DATA BUFF size
	bsr	TXDATABUFFroom		#| d3 := number of bytes left

	cmp.l	%d3,nbytes(%a5)		#| data_number :=
	blt.b	small_room		#|    min( nbyte, number of bytes left )
	mov.l	nbytes(%a5),%d3

small_room:

	mov.l	%d3,-(%sp)		#| save number of chars to be transferred
	mov.l	%d3,data_number(%a5)	#| set up for putchars
	mov.l	buf(%a5),data_address(%a5)

out_loop:

	bsr	putchars		#| REPEAT putchars
	tst.l	data_number(%a5)	#|
	bne.b	out_loop		#| UNTIL nbytes are written

	mov.l	(%sp)+,%d0		#| return number of chars transferred
	mov.b	&0x80,INT_DMA(%a3)	#| enable interrupts
	unlk	%a5			#| restore stack
	movm.l	(%sp)+,%a2-%a4/%d2-%d7	#|    and registers
	rts

 #*****************************************************************************
 #									     *
 #     int ody_init( desc, address )					     *
 #									     *
 #     struct ody_global *desc;						     *
 #     char *address;							     *
 #									     *
 #----------------------------------------------------------------------------*
 #									     *
 #     This procedure initializes the ody card's global variable space	     *
 #									     *
 #									     *
 #									     *
 #									     *
 #*****************************************************************************

_ody_init:
	movm.l	%a2-%a4/%d2-%d7,-(%sp)	#| save registers
	link	%a5,&-10 		#| save context
	pea	escape_handler		#| where to jump if escape

	mova.l	desc(%a5),%a4		#| a4 := pointer to attributes

	mov.l	address(%a5),%d0	#| store card address
	bset	&0,%d0			#| with lsb set
	mov.l	%d0,c_addr(%a4)
	clr.b	which_RXbuf(%a4)

	unlk	%a5			#| restore stack
	movm.l	(%sp)+,%a2-%a4/%d2-%d7	#|    and registers
	rts


 #*****************************************************************************
 #									     *
 #	    ****    *	*   *****   *****   *****   ****     ***	     *
 #	     *	*   *	*   *	    *	    *	    *	*   *	*	     *
 #	     *	*   *	*   *	    *	    *	    *	*   *		     *
 #	     ***    *	*   ****    ****    ****    ****     ***	     *
 #	     *	*   *	*   *	    *	    *	    * *		*	     *
 #	     *	*   *	*   *	    *	    *	    *  *    *	*	     *
 #	    ****     ***    *	    *	    *****   *	*    ***	     *
 #									     *
 #*****************************************************************************

 #*****************************************************************************
 #									     *
 #     routine gain_access: gets access to SEMAPHORE on card for buffer	     *
 #	      ===========  utilities.  If access is not	gained in a	     *
 #			   preset time,	an escape is performed.		     *
 #									     *
 #     At entry:						 	     *
 #	      a3.l =  card base	address	($00xx0001)			     *
 #									     *
 #     Upon normal exit:					 	     *
 #	  If escape performed then					     *
 #	      Timeout occurred						     *
 #	  Otherwise							     *
 #	      Access was gained						     *
 #									     *
 #     This bashes d2.l.					 	     *
 #									     *
 #*****************************************************************************

gain_access: 
 #	clr.b	INT_DMA(a3)		disable interrupts
	mov.l	&157500,%d2		#Initialize counter [CALIBRATED 1 SEC]
	mov.w	%sr,%d7			#save old sr
galoop:	mov.w	&0x2700,%sr		#protect the semaphore
	tst.b	SEMAPHORE(%a3)		#Fetch semaphore bit in bit 7 (sign)
	bpl.b	gadone			#If bit 7 true then done!
	mov.w	%d7,%sr			#restore old sr
	subq.l	&1,%d2			#Loop for preset time
	bne.b	galoop

 #		Timed out: escape, but first...

 #	move.b	#$80,INT_DMA(a3)	enable interrupts
	bra	escape			#Now escape

 #*****************************************************************************
 #									     *
 #     routine release_access: releases access to SEMAPHORE on card which    *
 #	      ==============  was previously gained with gain_access.	     *
 #									     *
 #     At entry:					 		     *
 #	      a3.l =  card base	address	($00xx0001)			     *
 #									     *
 #     Upon normal exit:						     *
 #	      no registers are bashed.					     *
 #									     *
 #*****************************************************************************

release_access: 
	mov.b	%d0,SEMAPHORE(%a3)	#Store don't-care into semaphore.
	mov.w	%d7,%sr			#restore old sr
 #	move.b  #$80,INT_DMA(a3)	restore interrupts
gadone:	rts

 #*****************************************************************************
 #									     *
 #     routine find_TRBUF:     Sets up pointer in a2 to point to the record   *
 #	      ===========     describing the card's TRBUFF structure.	     *
 #									     *
 #     routine find_TXBUF:     Sets up pointer in a2 to point to the record   *
 #	      ===========     describing the card's TXBUFF structure.	     *
 #									     *
 #     routine find_RXBUF:     Sets up pointer in a2 to point to the record   *
 #	      ===========     describing the card's RXBUFF structure.	     *
 #									     *
 #     At entry:						 	     *
 #	      a3.l =  card base	address	($00xx0001)			     *
 #									     *
 #     Upon exit:							     *
 #	      a2.l =  buffer record base address (CTRLBUFF_ADDR,	     *
 #		      CTRLBUFF_SIZE, CTRLBUFF_FILL, CTRLBUFF_EMPTY,	     *
 #		      DATABUFF_ADDR, etc).  (shifted, +1+selectcode)	     *
 #	      This bashes a1, d4 and d5.				     *
 #									     *
 #*****************************************************************************

find_TRBUF: 
	movq	&0,%d5
	mova.l	&TR_QUEUE_ADDR,%a2
	bra.b	findTR

find_TXBUF: 
	movq	&TXBUFF,%d5
	bra.b	find

find_RXBUF: 
	movq	&RXBUFF,%d5
find:	mova.l	&PRIM_0_ADDR,%a2

findTR:	clr.l	%d4
	movp.w	DSDP(%a3),%d4
	ror.w	&7,%d4
	add.l	%a3,%d4
	mova.l	%d4,%a1			#| a1 points to Data Struct Descriptor

	clr.l	%d4
	adda.l	%a2,%a1			#| add offset to which queue table
	movp.w	0(%a1),%d4
	ror.w	&7,%d4
	add.l	%a3,%d4
	add.l	%d5,%d4
	mova.l	%d4,%a2			#| a2 points to buffer record
	rts

 #*****************************************************************************
 #									     *
 #     routine find_RX_DATA:   Sets up pointers to point to the appropriate   *
 #	      ============    receive data buffer.  This is to be used after *
 #			      the routine find_RXBUF which sets	up the	     *
 #			      pointer (in a2) to the receive control buffer  *
 #			      descriptor record	structure.		     *
 #									     *
 #     At entry:					 		     *
 #	      a3.l =  card base	address	($00xx0001)			     *
 #	      a2.l =  Buffer record base address (CTRLBUFF_ADDR,	     *
 #		      CTRLBUFF_SIZE, CTRLBUFF_FILL, CTRLBUFF_EMPTY,	     *
 #		      DATABUFF0_ADDR, etc).  (shifted, +1+selectcode)	     *
 #	      a4.l =  pointer to select_code_table structure		     *
 #									     *
 #     Upon exit:							     *
 #	      a1.l =  data area	base address (shifted, +1+selectcode)	     *
 #	      d4.l =  address of first byte PAST data area (shifted, +1+sc)  *
 #	      d5.l =  XXxxxxBUFF_SIZE (unshifted, not adjusted)		     *
 #									     *
 #*****************************************************************************

find_RX_DATA: 
	mova.l	&DATA_AREA,%a1
	clr.l	%d5			#| compute offset for WHICH rx data
	mov.b	which_RXbuf(%a4),%d5	#| buffer we are using
	asl.l	&4,%d5
	adda.l	%d5,%a1

	bra.b	findare			#| Now go do the rest of it!

 #*****************************************************************************
 #									     *
 #     routine find_DATA_AREA: Sets up pointers to point to the data buffer.  *
 #	      ==============  This is to be used in conjunction	with the     *
 #			      routines find_XXBUF which	will set up the	     *
 #			      pointer (in a2) to the buffer we are using.    *
 #			      THIS SHOULD NOT BE USED WITH THE RECEIVE	     *
 #			      BUFFER!  USE THE PREVIOUS	ROUTINE	INSTEAD!     *
 #									     *
 #     routine find_CTRL_AREA: Sets up pointers to point to the ctrl buffer.  *
 #	      ==============  This is to be used in conjunction	with the     *
 #			      routines find_XXBUF which	will set up the	     *
 #			      pointer (in a2) to the buffer we are using.    *
 #									     *
 #     At entry:					 		     *
 #	      a3.l =  card base	address	($00xx0001)			     *
 #	      a2.l =  Data buffer record base address (CTRLBUFF_ADDR,	     *
 #		      CTRLBUFF_SIZE, CTRLBUFF_FILL, CTRLBUFF_EMPTY,	     *
 #		      DATABUFF_ADDR, etc).  (shifted, +1+selectcode)	     *
 #									     *
 #     Upon exit:							     *
 #	      a1.l =  data area	base address (shifted, +1+selectcode)	     *
 #	      d4.l =  address of first byte PAST data area (shifted, +1+sc)  *
 #	      d5.l =  XXxxxxBUFF_SIZE (  shifted,     adjusted)		     *
 #									     *
 #*****************************************************************************

find_DATA_AREA: 
	mova.l	&DATA_AREA,%a1
	bra.b	findare

find_CTRL_AREA: 
	mova.l	&CTRL_AREA,%a1

findare:	adda.l	%a2,%a1	 	#| a1 points to data/ctrl part of record
	clr.l	%d5
	movp.w	SIZE(%a1),%d5
	ror.w	&8,%d5			#| d5 = SIZE in bytes

	clr.l	%d4
	movp.w	ADDR(%a1),%d4
	ror.w	&7,%d4
	add.l	%a3,%d4
	mova.l	%d4,%a1			#| a1 points to front of buffer area
	add.l	%d5,%d4
	add.l	%d5,%d4			#| d4 points past end of buffer area

	rts

 #*****************************************************************************
 #									     *
 #	****		****	*   *	*****	*****	*****	****	     *
 #	*   *		 *  *	*   *	*	*	*	*   *	     *
 #	*   *	*   *	 *  *	*   *	*	*	*	*   *	     *
 #	****	 * *	 ***	*   *	****	****	****	****	     *
 #	* *	  *	 *  *	*   *	*	*	*	* *	     *
 #	*  *	 * *	 *  *	*   *	*	*	*	*  *	     *
 #	*   *	*   *	****	 ***	*	*	*****	*   *	     *
 #									     *
 #*****************************************************************************

 #*****************************************************************************
 #									     *
 #     routine set_RXBUF_a6:   Routine to set a6 as the base address	     *
 #	      ============    pointer to whichever Rx buffer is	being used.  *
 #									     *
 #     At entry:				 			     *
 #	      a2.l =  data buffer base address (shifted, +1+selectcode)	     *
 #	      a3.l =  card base	address	($00xx0001)			     *
 #	      a4.l =  pointer to sc_subtabletype structure		     *
 #									     *
 #     Upon exit:							     *
 #	      a6.l =  Base address of DATA_BUFFERS[WHICH_RXBUF]		     *
 #	      This also	bashes d0.					     *
 #									     *
 #*****************************************************************************

set_RXBUF_a6: 
	clr.l	%d0			#| Setup d0.l=offset
	mov.b	which_RXbuf(%a4),%d0	#| to which Rx buffer
	asl.l	&4,%d0			#| being used
	lea	DATA_AREA(%a2,%d0),%a6
	rts

 #*****************************************************************************
 #									     *
 #     routine RX_BUFF_bytes:  Function which returns the number of	     *
 #	      =============   characters until the first control block in    *
 #			      the Receive Buffer.  If there are	no control   *
 #			      blocks, this just	returns	the number of	     *
 #			      characters in the	buffer.	 This only works     *
 #			      on the current Rx	data buffer, and does not    *
 #			      extract TERM=255 control blocks!		     *
 #									     *
 #     At entry:						 	     *
 #	      a2.l =  data buffer base address (shifted, +1+selectcode)	     *
 #	      a3.l =  card base	address	($00xx0001)			     *
 #	      a4.l =  pointer to sc_subtabletype structure		     *
 #									     *
 #     Upon exit:							     *
 #	      d0.l =  Number of	characters.				     *
 #	      a1, d4 and d5 are	left with values from find_RX_DATA.	     *
 #	      This also	bashes a0, d1 and d2.				     *
 #									     *
 #     This routine uses	the card's SEMAPHORE to	gain access. 	     *
 #									     *
 #     This routine calls gain_access, release_access, and find_RX_DATA.     *
 #									     *
 #*****************************************************************************

RX_BUFF_bytes: 
	bsr	find_RX_DATA		#| Setup a1 = data buffer base addr
 #						d4 = end of data buffer	addr
 #						d5 = RXDATABUFF_SIZE
	mov.l	%a6,-(%sp)
	bsr	set_RXBUF_a6

	clr.l	%d0			#| Get garbage out of top of d0,d1
	clr.l	%d1
	movp.w	CTRL_AREA+EMPTY(%a2),%d1 #| Fetch pointers (bytes in wrong order)
	bsr	gain_access		#| Need access to FILL pointers
	movp.w	FILL(%a6),%d0
	movp.w	CTRL_AREA+FILL(%a2),%d2
	bsr	release_access
	cmp.w	%d1,%d2			#| If the two ctrl block pointers are not
	beq.b	RBb1			#| equal, then we want to use the pointer
	ror.w	&7,%d1			#| field from the next control block to
	add.l	%a3,%d1			#| indicate how much data may be removed
	mova.l	%d1,%a0
	movp.w	POINTER(%a0),%d0		#| --- Use it as the "FILL" pointer

RBb1:	ror.w	&8,%d0			#| Switch bytes for FILL
	movp.w	EMPTY(%a6),%d1		#| and get EMPTY and switch bytes
	ror.w	&8,%d1			#| d0="FILL", d1=EMPTY

	sub.w	%d1,%d0			#| Compute d0 := FILL-EMPTY
	bge.b	RBb2
	add.w	%d5,%d0			#| If negative, add data buffer size
RBb2:	mova.l	(%sp)+,%a6
	rts	#| Now d0 = ("FILL"-EMPTY) mod SIZE --- of data buffer

 #*****************************************************************************
 #									     *
 #     routine getctrlblk:     Routine which gets a control block from the   *
 #	      ==========      Receive buffer.  It must have already been     *
 #			      determined that there is a control block at    *
 #			      the front	of the buffer, since this routine    *
 #			      does NOT check for that condition.  The TERM   *
 #			      and MODE fields of the removed block are left  *
 #			      in the appropriate (.term	and .mode) in the    *
 #			      sc_subtabletype structure.		     *
 #									     *
 #     At entry: 							     *
 #	      a2.l =  RX buffer	record base address from find_RXBUF	     *
 #	      a3.l =  card base	address	($00xx0001)			     *
 #	      a4.l =  pointer to sc_subtabletype structure		     *
 #									     *
 #     Upon exit:							     *
 #	      sc_subtabletype.term =  TERM field of control block (8 bits)   *
 #	      sc_subtabletype.mode =  MODE field of control block (8 bits)   *
 #	      a1, d4 and d5 are	left with the values from find_CTRL_AREA.    *
 #	      This bashes d0, d2, and a0.				     *
 #									     *
 #     This routine uses the card's SEMAPHORE to gain access.	 	     *
 #									     *
 #     This routine calls gain_access, release_access, and find_CTRL_AREA.   *
 #									     *
 #*****************************************************************************

getctrlblk: 
	bsr	find_CTRL_AREA		#| Setup a1 = ctrl buffer base addr
 #						d4 = end of ctrl buffer	addr
 #						d5 = TRCTRLBUFF_SIZE

	clr.l	%d0			#| Clear	top of d0
	movp.w	CTRL_AREA+EMPTY(%a2),%d0 #| Get control buffer EMPTY pointer
	ror.w	&7,%d0			#| Now make it into a 68000 pointer
	add.l	%a3,%d0
	mova.l	%d0,%a0			#| Move to a0 so we can use it

	mov.b	TERMFIELD(%a0),term(%a5)	#| Store term & mode fields
	mov.b	MODEFIELD(%a0),mode(%a5)

	add.w	&CTRLBLKSIZE,%d0	#| Bump pointer by control block size
	cmp.w	%d4,%d0			#| and check for wraparound.
	bne.b	gcb1
	mov.w	%a1,%d0			#| If so, set to front of buffer
gcb1:	bclr	&0,%d0			#| Make it into a Z80
	rol.w	&7,%d0			#| type pointer with bytes reversed
	bsr	gain_access		#| Now store the updated EMPTY pointer
	movp.w	%d0,CTRL_AREA+EMPTY(%a2)
	bsr	release_access
	rts	#|<<<CAN'T COMBINE WITH ABOVE!!!!!

 #*****************************************************************************
 #									     *
 #     routine getchars:       Routine which takes characters from the	     *
 #	      ========	      Receive buffer and puts them in the area	     *
 #			      pointed to by sc_subtabletype.data_address     *
 #			      and sized	by sc_subtabletype.data_number.	     *
 #			      The number of characters			     *
 #			      actually transfered is the minimum of:	     *
 #			      (1) the number of	characters available before  *
 #			      the first	Receive	buffer control block|	     *
 #			      (2) sc_subtabletype.data_number|		     *
 #			      and (3) the number of characters		     *
 #			      available	until the Receive buffer wraparound  *
 #			      point.  THIS NUMBER MAY BE ZERO!		     *
 #			      This alters data_address and data_number to    *
 #			      reflect where to start going next	time this    *
 #			      is called.  The criteria for ending the	     *
 #			      transfer at a higher level must be determined  *
 #			      by data_number, RX_stuff_avail and	     *
 #			      ctrl_blk_next/getctrlblk.			     *
 #									     *
 #     At entry:	 						     *
 #	      a2.l =  RX buffer	record base address from find_RXBUF	     *
 #	      a3.l =  card base	address	($00xx0001)			     *
 #	      a4.l =  pointer to sc_subtabletype structure		     *
 #									     *
 #     Upon exit:							     *
 #	      data_address and data_number are updated,	plus the EMPTY	     *
 #	      pointer in the card's Receive data buffer.		     *
 #	      In sc_subtabletype last_enter_term and last_enter_mode are     *
 #	      zeroed if	any data is moved.				     *
 #	      a1 and d4	are left with the values from find_RX_DATA.	     *
 #	      This bashes d0, d1, d2, d3, d4, d5, a0, and a1.		     *
 #									     *
 #     This routine uses the card's SEMAPHORE to gain access.		     *
 #									     *
 #     This routine calls gain_access, release_access, and RX_BUFF_bytes.    *
 #									     *
 #*****************************************************************************

getchars: 
	bsr	RX_BUFF_bytes		#| Setup a1 = data buffer base addr
 #						d3 = offset to which Rxbuff used
 #						d4 = end of data buffer	addr
 #						d5 = RXDATABUFF_SIZE
	mov.l	%d0,%d3			#| d3.l = available characters

	movm.l	%a2/%a6,-(%sp)		#| Saved for local use
	bsr	set_RXBUF_a6

	clr.l	%d0
	movp.w	EMPTY(%a6),%d0		#| Get RXDATABUFF_EMPTY and make
	ror.w	&7,%d0			#| it into a 68000 pointer
	add.l	%a3,%d0
	mova.l	%d0,%a2			#| Save EMPTY for later!

	sub.l	%d4,%d0
	neg.l	%d0			#| d0 = wraparound address - EMPTY
	and.l	&0x0000FFFE,%d0
	ror.w	&1,%d0			#| d0.l = number of bytes till wraparound

	cmp.l	%d3,%d0			#| If d0>d3 then set d0 := d3
	bgt.b	gc1
	mov.l	%d3,%d0

gc1:	mov.l	data_number(%a5),%d2	#| Fetch number of positions available
	cmp.l	%d2,%d0			#| If d0>d2 then set d0 := d2
	bgt.b	gc2
	mov.l	%d2,%d0

gc2:	mov.l	%d0,%d3			#| d3.l saves number of chars actually
 #					| transferred below
	beq.b	gcdone			#| If zero, no work to be done
 #	clr.w	last_enter_term(a5)	| This also clears last_enter_mode.
	subq.w	&1,%d0			#| Make offset correct for dbf instr.
	mova.l	data_address(%a5),%a0	#| Get character pointer into a0

gcloop:	mov.b	(%a2),(%a0)+		#| Transfer a character & bump dest ptr
	addq.w	&2,%a2			#| Bump source pointer (odd bytes)
	dbf	%d0,gcloop		#| Then decrement d0 & loop

	sub.l	%d3,%d2			#| Decrement datacnt by # bytes
	mov.l	%d2,data_number(%a5)	#| Now store adjusted address and
	mov.l	%a0,data_address(%a5)	#| number fields

	mov.l	%a2,%d1			#| Store pointer for computations
	cmp.l	%d4,%a2			#| Now check to see if EMPTY was moved
	bne.b	gc3			#| past end of buffer.  If so, set to
	mov.l	%a1,%d1			#| the front of the buffer.
gc3:	bclr	&0,%d1			#| Fix up the 68000 pointer to be the
	rol.w	&7,%d1			#| card's type of pointer
	bsr	gain_access
	movp.w	%d1,EMPTY(%a6)		#| Remember d1 = card's EMPTY pointer.
	bsr	release_access
gcdone:	movm.l	(%sp)+,%a6/%a2
	rts

 #*****************************************************************************
 #									     *
 #     routine RX_stuff_avail: Routine which determines whether there is     *
 #	      ==============  ANYTHING (data or	control	blocks)	in the	     *
 #			      Receive buffer.  This consumes any TERM=255    *
 #			      control blocks before returning the function.  *
 #									     *
 #     At entry:							     *
 #	      a2.l =  RX buffer	record base address from find_RXBUF	     *
 #	      a3.l =  card base	address	($00xx0001)			     *
 #	      a4.l =  pointer to sc_subtabletype structure		     *
 #									     *
 #     Upon exit:							     *
 #	      d0.l =  $00 if buffer is empty,				     *
 #		      $01 if ctrl buffer is empty and data buffer is not,    *
 #		      $02 if data buffer is empty and ctrl buffer is not,    *
 #		      $03 if both data and ctrl	buffers	are not	empty.	     *
 #	      a1 and d4	are left with the values from find_RX_DATA.	     *
 #	      This bashes d0, d1, d2, d3, d4, d5, a0 and a1.		     *
 #									     *
 #     This routine uses	the card's SEMAPHORE to	gain access.	     *
 #									     *
 #     This routine calls gain_access and release_access.		     *
 #									     *
 #*****************************************************************************

RX_stuff_avail: 
	bsr	find_RX_DATA		#| Setup a1 = data buffer base addr
 #						d4 = end of data buffer	addr
 #						d5 = RXDATABUFF_SIZE
	mov.l	%a6,-(%sp)
	bsr	set_RXBUF_a6

	bsr	gain_access
	movp.w	FILL(%a6),%d3		#| Fetch FILL & EMPTY (bytes reversed but
	movp.w	CTRL_AREA+FILL(%a2),%d1
	bsr	release_access		#| we're just checking equality)
	clr.l	%d2
	clr.l	%d0
	movp.w	CTRL_AREA+EMPTY(%a2),%d2
	cmp.w	%d2,%d1			#| Compare ctrl buff FILL & EMPTY
	bne.b	setbit1			#| If not equal, then set bit 1
chkdata:	movp.w	EMPTY(%a6),%d2
	cmp.w	%d2,%d3			#| Compare data buff FILL & EMPTY
	beq.b	return
	addq.b	&1,%d0			#| And set bit 0 if not equal
return:	mova.l	(%sp)+,%a6
	rts

setbit1:	addq.b	&2,%d0		#| Set "ctrl not empty" bit
	ror.w	&7,%d2			#| Something in control buffer - see if
 #					| this control block is	at the head of
	add.l	%a3,%d2			#| the queue (bytes reversed!)
	mova.l	%d2,%a0
	movp.w	POINTER(%a0),%d1
	movp.w	EMPTY(%a6),%d2
	cmp.w	%d1,%d2			#| if POINTER field<>DATABUFF_EMPTY
	bne.b	chkdata			#|   then go check data buff
	cmpi.b	TERMFIELD(%a0),&255	#| else if it's a TERM=255 control block
	bne.b	chkdata			#| No, go back and check data buff

	bsr	getctrlblk		#| Otherwise consume the control block
	mov.b	mode(%a5),which_RXbuf(%a4)  #and switch to new data buffer
	mova.l	(%sp)+,%a6
	bra.b	RX_stuff_avail		#| And go back and re-compute result

 #*****************************************************************************
 #									     *
 #     routine ctrlblknext:    Routine which determines whether the next     *
 #	      ===========     thing to be consumed from	the Receive buffer   *
 #			      is a control block.  THE RESULT OF THIS	     *
 #			      FUNCTION IS NOT VALID UNLESS RX_BUFFER_empty   *
 #			      RETURNS FALSE!!!				     *
 #									     *
 #     At entry:							     *
 #	      a2.l =  RX buffer	record base address from find_RXBUF	     *
 #	      a3.l =  card base	address	($00xx0001)			     *
 #									     *
 #     Upon exit:							     *
 #	      d0.b =  $FF if control block is next, $00	if data	is next.     *
 #	      This bashes d2, d5 and a0.				     *
 #									     *
 #*****************************************************************************

ctrlblknext: 
	bsr	gain_access
	movp.w	CTRL_AREA+FILL(%a2),%d2	#| Check if ctrl buffer is empty
	bsr	release_access
	clr.l	%d0
	movp.w	CTRL_AREA+EMPTY(%a2),%d0#| Fetch ctrl buffer EMPTY pointer
	cmp.w	%d2,%d0			#| If equal then return d0.b=$00
	beq.b	cbn1
	ror.w	&7,%d0
	add.l	%a3,%d0
	mova.l	%d0,%a0
	movp.w	POINTER(%a0),%d0	#| Fetch the POINTER field from the

	clr.l	%d5			#| Setup d5.l=offset
	mov.b	which_RXbuf(%a4),%d5	#| to which Rx buffer
	asl.l	&4,%d5			#| being used

	movm.l	%d0/%a6,-(%sp)
	bsr	set_RXBUF_a6
	movp.w	EMPTY(%a6),%d2		#| first ctrl block and compare to the
	movm.l	(%sp)+,%d0/%a6

	cmp.w	%d2,%d0			#| data buffer EMPTY pointer
	seq	%d0			#| Then set d0 if equal
	rts

cbn1:	clr.l	%d0
	rts


 #*****************************************************************************
 #									     *
 #	*****		****	*   *	*****	*****	*****	****	     *
 #	  *		 *  *	*   *	*	*	*	*   *	     *
 #	  *	*   *	 *  *	*   *	*	*	*	*   *	     *
 #	  *	 * *	 ***	*   *	****	****	****	****	     *
 #	  *	  *	 *  *	*   *	*	*	*	* *	     *
 #	  *	 * *	 *  *	*   *	*	*	*	*  *	     *
 #	  *	*   *	****	 ***	*	*	*****	*   *	     *
 #									     *
 #*****************************************************************************
 #*****************************************************************************
 #									     *
 #     routine TXCTRLBUFFroom: Function which returns the number of byte     *
 #	      ==============  positions	as yet unused in the Transmit ctrl   *
 #			      Buffer.					     *
 #									     *
 #     routine TXDATABUFFroom: Function which returns the number of byte     *
 #	      ==============  positions	as yet unused in the Transmit data   *
 #			      Buffer.					     *
 #									     *
 #     At entry:							     *
 #	      a2.l =  TXBUFF base address (shifted, +1+selectcode)	     *
 #	      a3.l =  card base	address	($00xx0001)			     *
 #	      d5.l =  TXDATABUFF_SIZE or TXCTRLBUFF_SIZE		     *
 #		      (unshifted, not adjusted)				     *
 #									     *
 #     Upon exit:							     *
 #	      d0.l =  TXDATABUFF_FILL or TXCTRLBUFF_FILL (unshifted)	     *
 #	      d3.l =  Number of	bytes left				     *
 #	      This also	bashes d2.					     *
 #									     *
 #     This routine uses the card's SEMAPHORE to gain access.		     *
 #									     *
 #     This routine calls gain_access and release_access.		     *
 #									     *
 #*****************************************************************************

TXCTRLBUFFroom: 
	clr.l	%d0			#| Get garbage out of top of d0&d3
	clr.l	%d3
	movp.w	CTRL_AREA+FILL(%a2),%d0
	bsr	gain_access		#| Need access to EMPTY
	movp.w	CTRL_AREA+EMPTY(%a2),%d3 #| Fetch pointers (bytes in wrong order)
	bra.b	room1

TXDATABUFFroom: 
	clr.l	%d0			#| Get garbage out of top of d0&d3
	clr.l	%d3
	movp.w	DATA_AREA+FILL(%a2),%d0
	bsr	gain_access		#| Need access to EMPTY
	movp.w	DATA_AREA+EMPTY(%a2),%d3 #| Fetch pointers (bytes in wrong order)
room1:	bsr	release_access

	ror.w	&8,%d0			#| Switch bytes in d0 & d3
	ror.w	&8,%d3

	sub.w	%d0,%d3			#| Compute d3 := EMPTY-FILL
	subq.w	&1,%d3			#| (EMPTY-FILL-1)
	bge.b	room2
	add.w	%d5,%d3			#| If negative, add size

room2:	rts				#| Return (EMPTY-FILL-1) mod SIZE

 #*****************************************************************************
 #									     *
 #     routine putchars:      Routine which takes characters from the	     *
 #	      ========	      area sc_subtabletype.data_address	sized by     *
 #			      sc_subtabletype.data_number and moves	     *
 #			      them to the Transmit buffer.  The	number of    *
 #			      characters actually transfered is	the minimum  *
 #			      of: (1) the number of characters available|    *
 #			      (2) the number of	byte positions left in the   *
 #			      Transmit buffer| and (3) the number of byte    *
 #			      positions	in the Transmit	buffer until the     *
 #			      wraparound point.	 THIS NUMBER CAN BE ZERO.    *
 #			      This alters data_address and data_number to    *
 #			      reflect where to start going next	time this    *
 #			      is called.  The entire transfer is done when   *
 #			      data_number goes to zero.			     *
 #									     *
 #     At entry:							     *
 #	      a2.l =  TX buffer	record base address (shifted, +1+selectcode) *
 #	      a3.l =  card base	address	($00xx0001)			     *
 #	      a4.l =  pointer to sc_subtabletype structure		     *
 #									     *
 #     Upon exit:							     *
 #	      data_number and data_address are updated,	plus FILL in the     *
 #	      card's Transmit buffer.					     *
 #	      a1, d4 and d5 are	left with the values from find_DATA_AREA.    *
 #	      This bashes d0, d1, d2, d3, d4, d5, a0, and a1.		     *
 #									     *
 #     Interrupts:							     *
 #	      This does	its own	enabling/disabling.  Interrupts	are left ON. *
 #									     *
 #     This routine uses the card's SEMAPHORE to gain access.		     *
 #									     *
 #     This routine calls gain_access, release_access, TXDATABUFFroom,	     *
 #     and find_DATA_AREA.						     *
 #									     *
 #*****************************************************************************

putchars: 
	bsr	find_DATA_AREA		#| Setup a1 = data buffer base addr
 #						d4 = end of data buffer	addr
 #						d5 = TXDATABUFF_SIZE
 #	clr.b	INT_DMA(a3)		| disable interrupts 
	bsr	TXDATABUFFroom		#| d3.l = available buffer positions
	mov.l	%d0,%d1			#| d0.l = d1.l = TXDATABUFF_FILL
	mov.l	%d4,%d0
	andi.l	&0x0000FFFE,%d0
	asr.l	&1,%d0			#| d0.l = unshifted TXDATABUFF_END
	sub.l	%d1,%d0			#| d0.l = remaining positions to wrap

	cmp.l	%d3,%d0			#| If d0>d3 then set d0 := d3
	bgt.b	pc1
	mov.l	%d3,%d0

pc1:	mov.l	data_number(%a5),%d2	#| Fetch number of chars avail into d2
	cmp.l	%d2,%d0			#| If d0>d2 then set d0 := d2
	bgt.b	pc2
	mov.l	%d2,%d0

pc2:	mov.l	%d0,%d3			#| d3.l saves number of chars actually
 #					 | transferred below
	beq.b	pcdone			#| If zero, no work to be done
	subq.w	&1,%d0			#| Make offset correct for dbf instr.
	mova.l	data_address(%a5),%a0	#| Get character pointer into a0

	lsl.w	&1,%d1
	add.l	%a3,%d1
	mov.l	%a1,-(%sp)		#| Save a1 so we can use the register
	mova.l	%d1,%a1			#| Now a1 is useable pointer

pcloop:	mov.b	(%a0)+,(%a1)		#| Transfer a character & bump source ptr
	addq.w	&2,%a1			#| Bump destination pointer (odd bytes)
	dbf	%d0,pcloop		#| Then decrement d0 & loop

	sub.l	%d3,%d2
	mov.l	%d2,data_number(%a5)	#| Now store adjusted number and
	mov.l	%a0,data_address(%a5)	#| address fields

	mov.l	%a1,%d1			#| Move 68000 FILL pointer into d1
	mov.l	(%sp)+,%a1		#| Restore a1 before we forget!
	cmp.l	%d4,%d1			#| Now check to see if FILL was moved
	bne.b	pc3			#| past end of buffer.  If so, set to
	mov.l	%a1,%d1			#| the front of the buffer.
pc3:	bclr	&0,%d1			#| Fix up the 68000 pointer to be the
	rol.w	&7,%d1			#| card's type of pointer
	bsr	gain_access
	movp.w	%d1,DATA_AREA+FILL(%a2)	#| Remember d1 = card's FILL pointer.
	bsr	release_access
pcdone:	
 #	move.b	#0x80,INT_DMA(a3)	| enable interrupts 
	rts
	

 #*****************************************************************************
 #									     *
 #     routine putctrlblk:     Routine which puts a control block into the   *
 #	      ==========      Transmit buffer area of the card.	 The	     *
 #			      appropriate pointers are updated to reflect    *
 #			      the control block.  This routine assumes that  *
 #			      there is enough room to put the control	     *
 #			      block into the transmit control buffer.	     *
 #									     *
 #     At entry:							     *
 #	      sc_subtabletype.term =  TERM field for control block (8 bits)  *
 #	      sc_subtabletype.mode =  MODE field for control block (8 bits)  *
 #	      a2.l =  TX buffer	record base address (shifted, +1+selectcode) *
 #	      a3.l =  card base	address	($00xx0001)			     *
 #	      a4.l =  pointer to sc_subtabletype structure		     *
 #									     *
 #     Upon exit:							     *
 #	      FILL in the card's transmit control buffer is updated.	     *
 #	      a1, d4 and d5 are	left with the values from find_CTRL_AREA.    *
 #	      This bashes d0, d1, d2, d3, d4, d5, a0 and a1.		     *
 #									     *
 #     This routine uses	the card's SEMAPHORE to	gain access.	     *
 #									     *
 #     This routine calls find_CTRL_AREA, gain_access, release_access.	     *
 #									     *
 #*****************************************************************************

putctrlblk: 
	bsr	find_CTRL_AREA		#| Setup a1 = ctrl buffer base addr
 #						d4 = end of ctrl buffer	addr
 #						d5 = TXCTRLBUFF_SIZE

	clr.l	%d0			#|   remove garbage from top of d0
	movp.w	CTRL_AREA+FILL(%a2),%d0	#|
	ror.w	&8,%d0			#|   get d0 = CTRLBUFF_FILL (unshifted)
	lsl.w	&1,%d0			#|   Make CTRLBUFF_FILL into a 68000
	add.l	%a3,%d0			#|   pointer
	mova.l	%d0,%a0			#| a0 := CTRLBUFF_FILL (68000 format)

	movp.w	DATA_AREA+FILL(%a2),%d0	#| Get the DATA_FILL pointer to put
	movp.w	%d0,POINTER(%a0)		#| into the POINTER FIELD
	mov.w	term_and_mode(%a5),%d0
	movp.w	%d0,TERMFIELD(%a0)
	adda.l	&CTRLBLKSIZE,%a0		#| Bump pointer by TWO bytes

	mov.l	%a0,%d1			#| Move 68000 FILL pointer into d1
	cmp.l	%d4,%d1			#| Now check to see if FILL was moved
	bne.b	pcb1			#| past end of buffer.  If so, set to
	mov.l	%a1,%d1			#| the front of the buffer.
pcb1:	bclr	&0,%d1			#| Fix up the 68000 pointer to be the
	rol.w	&7,%d1			#| card's type of pointer
	bsr	gain_access
	movp.w	%d1,CTRL_AREA+FILL(%a2)
	bsr	release_access
	rts
