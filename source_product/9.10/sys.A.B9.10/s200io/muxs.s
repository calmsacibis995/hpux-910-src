 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/muxs.s,v $
 # $Revision: 1.3.84.3 $	$Author: kcs $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/09/17 21:15:22 $
 # HPUX_ID: @(#)muxs.s	55.1		88/12/23


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

 #
 #	Routines for the MUX card
 #
 #	ASSUMPTIONS
 #
 #		Header for the MUX card  from "mux.h"
 #
 #struct	mux_info_type {
 #	unsigned short flag		/* current state of this port */
 #	char *addr;
 #	char *transmit_buf;		/* transmit/receive buffer pointers */
 #	char *receive_buf;
 #	char *variable_list;		/* list for buffer management */
 #	char *bit_array;		/* special character array */
 #	char *modem_reg;		/* modem registers (if on this port) */
 #	char *config_reg;		/* pointer to configure data */
 #	char *port_cmd_reg;		/* pointer to port INTR/STATUS base */
 #	unsigned char port;		/* port number */
 #	unsigned char port_mask;	/* port mask for interrupts */
 #};
 #
 #**************************************************************

	text
	global _mux_enter		#| get data from mux
	global _mux_output		#| output a byte to the card
	global _mux_status		#| get the interrupt status
	global _mux_txempty		#| see if the transmit buffer is empty
	global _mux_rxempty		#| see if the receive buffer is empty
	global _mux_flush		#| flush the ports input/output buffers
	global _mux_modem_status		#| get the modem lines status
	global _mux_modem_control	#| set the modem lines
	global _mux_modem_mask		#| set the modem lines mask
	global _mux_break		#| send a break interrupt to the port
	global _mux_bit_array		#| change the bit array
	global _mux_config		#| configure the port
	global _mux_timeout_on		#| turn on the timeout clock
 #
 #	Constants
 #
       		set	TX_STOP,5		#| bit number in flag field
    		set	HIGH,0x2700		#| spl 7
     		set	HIGH6,0x2600		#| spl 6
       		set	TX_MASK,0x0f		#| masks for wrap around
        	set	TX_MASK2,0x1f		#| masks for wrap around
       		set	RX_MASK,0x00ff		#| mask for wrap around of index
        	set	RX_MASK2,0x01ff		#| mask for wrap around of index
          	set	CONFIG_INT,0x01		#| Send an configure interrupt
      		set	TX_INT,0x02		#| Send an transmit interrupt
         	set	BREAK_INT,0x04		#| Send an break interrupt
       		set	MOD_INT,0x10		#| modem interrupt bit
        	set	TIME_INT,0x20		#| timeout interrupt bit
 #
 #	mux structure offsets
 #
         	set	CARD_ADDR,2
     		set	TX_PT,6
     		set	RX_PT,10
        	set	VAR_LIST,14
         	set	BIT_ARRAY,18
      		set	MOD_PT,22
       		set	MOD_NUM,38
         	set	CONFIG_PT,26
           	set	PORT_CMD_PT,30
        	set	PORT_NUM,34
         	set	PORT_MASK,35
         	set	PORT_INTR,0x8e3f
        	set	INT_COND,0x8001
       		set	INT_CMD,0x8003
 #
 #	variable list offsets
 #
       		set	RX_HEAD,0
       		set	RX_TAIL,8
       		set	TX_HEAD,16
       		set	TX_TAIL,24
    		set	BAUD,2	#| offset to baud register
    		set	LINE,0	#| offset to line characteristics
 #
 #	modem register offsets
 #
      		set	MOD_ST,0	#| status register
      		set	MOD_CT,2	#| control register
      		set	MOD_MK,4	#| interrupt mask register
 #
 #	Offsets from card address
 #
       		set	SEM_REG,5
       		set	SEM_BIT,7
 #
 #	Enter as much data from the receive buffer as possible
 #
 #		mux_enter(mux, buffer);
 #
 #		return number of characters
 #
_mux_enter:	movm.l	%d2/%a2,-(%sp)		#| save registers
		movq	&0,%d0			#| clear register d0
		mova.l	12(%sp),%a0		#| get mux pointer
		mova.l	VAR_LIST(%a0),%a2		#| get variable list pointer
		mova.l	RX_PT(%a0),%a1		#| get RX buffer pointer
		mova.l	16(%sp),%a0		#| get buffer pointer
		movq	&0,%d1			#| clear register d1
		mov.b	RX_HEAD(%a2),%d1		#| get head offset
		cmp.b	%d1,RX_TAIL(%a2)		#| is buffer empty?
		beq.b	no_data			#| Yes, leave
		mov.l	%d1,%d2			#| copy it
		add.w	%d1,%d1			#| double it for index
enter_loop:	mov.b	0(%a1,%d1),(%a0)+		#| get character
		addq	&2,%d1			#| go to next byte
		mov.b	0(%a1,%d1),(%a0)+		#| get status
		addq	&2,%d1			#| go to next byte
		addq	&2,%d2			#| inc head pointer
		andi.w	&RX_MASK,%d2		#| mask off lower bits
		andi.w	&RX_MASK2,%d1		#| mask off lower bits of index
		mov.b	%d2,RX_HEAD(%a2)		#| update head pointer
		addq	&1,%d0			#| inc character count
		cmp.b	%d2,RX_TAIL(%a2)		#| are we finished?
		bne.b	enter_loop		#| No, keep going
no_data:		movm.l	(%sp)+,%d2/%a2		#| restore registers
		rts				#| leave, character count in d0
 #
 #	Output a single character
 #
 #		mux_output(mux, character);
 #
_mux_output:	mova.l	4(%sp),%a0	#| get mux info pointer
		mov.b	%d2,-(%sp)	#| save part of d2
		btst	&TX_STOP,1(%a0)	#| TX_STOP set?
		bne.b	no_send		#| Yes, don't send character
		mova.l	VAR_LIST(%a0),%a1	#| get variable pointer
		mov.b	TX_TAIL(%a1),%d0	#| get TAIL offset
		mov.b	%d0,%d1		#| copy offset
		mov.b	%d1,%d2		#| copy offset for intr check
		addq	&2,%d0		#| inc. TAIL by 2
		andi.b	&TX_MASK,%d0	#| mask it
		cmp.b	%d0,TX_HEAD(%a1)	#| Is buffer full?
		beq.b	no_send		#| Yes, don't send it
		movq	&0,%d0		#| clear d0
		mov.b	%d1,%d0		#| copy offset
		andi.b	&TX_MASK,%d0	#| mask it
		add.w	%d0,%d0		#| double it
		mova.l	TX_PT(%a0),%a0	#| get pointer to transmit buffer
		mov.b	13(%sp),0(%a0,%d0)	#| put character in buffer
		addq	&1,%d1		#| inc. TAIL
		andi.b	&TX_MASK,%d1	#| mask it
		mov.b	%d1,TX_TAIL(%a1)	#| updata TAIL
 #
 # Determine if we need to generate an interrupt?
 #
		cmp.b	%d2,TX_HEAD(%a1)	#| was the buffer empty?
		bne.b	no_intr		#| No, don't interrupt
 #
 # generate an TX interrupt to the card
 #
		mova.l	6(%sp),%a0	#| get mux_pointer again
		mov.b	PORT_MASK(%a0),-(%sp)	#| port mask for interrupt(sp+=2)
		mov.w	&TX_INT,-(%sp)		#| break interrupt mask for intr
		mov.l	PORT_CMD_PT(%a0),-(%sp)	#| push addr of port interrupt
		jsr	set_port_int	#| send and interrupt to the port
		addq	&8,%sp		#| re-adjust the stack
no_intr:		movq	&1,%d0		#| Yes, we got the character out
		mov.b	(%sp)+,%d2	#| restore d2
		rts
no_send:		movq	&0,%d0		#| No, we didn't get the character out
		mov.b	(%sp)+,%d2	#| restore d2
		rts
 #
 #	mux_status(mux_intr, buf)	-  return 5 interrupt status bytes
 #
 #		->	rts
 #			mux_intr pointer
 #			buf[4]
 #
 #		- return INT_STATUS byte, 4 bytes in buffer
 #
_mux_status:	mova.l	4(%sp),%a0		#| get mux_intr pointer
		mova.l	(%a0),%a0			#| get card address
		mova.l	8(%sp),%a1		#| get buffer pointer
		mov.w	%sr,%d1			#| save current status
stat_retry:	mov.w	&HIGH,%sr		#| raise priority
		btst	&SEM_BIT,SEM_REG(%a0)	#| try to get semaphore
		bne.b	stat_failed		#| NO, try again
		mov.l	%a0,%d0			#| get card address
		addi.l	&PORT_INTR,%d0		#| create port intr address
		mova.l	%d0,%a0			#| store address for port intr
		movp.l	0(%a0),%d0		#| get ports status
		mov.l	%d0,(%a1)			#| put in buffer
		movq	&0,%d0			#| clear d0
		movp.l	%d0,0(%a0)		#| clear out ports status
		mova.l	4(%sp),%a0		#| get mux_intr pointer
		mov.l	(%a0),%d0			#| get card address
		addi.l	&INT_COND,%d0		#| create INT_CMD address
		mova.l	%d0,%a0			#| store address for INT_CMD
		movq	&0,%d0			#| clear d0
		mov.b	(%a0),%d0			#| get INT_CMD status byte
		mov.b	&0,(%a0)			#| clear INT_CMD status byte
		mova.l	4(%sp),%a0		#| get mux_intr pointer
		mova.l	(%a0),%a0			#| get card address
		mov.b	%d0,SEM_REG(%a0)		#| clear semaphore
		mov.w	%d1,%sr			#| reset priority
		rts
stat_failed:	mov.w	%d1,%sr			#| reset priority
		bra.b	stat_retry		#| try again
 #
 #
 #	Send an interrupt to the card routines
 #
 #
 #	Local routines
 #
 #	  - set_modem_int
 #	  - set_port_int
 #
 #
 #  set_modem_int
 #
 #	ENTRY:	a0 - mux_pointer
 #
 #		stack 	->	return address
 #
set_mod_int:	mov.w	%sr,%d1			#| save current priorty
		mov.l	CARD_ADDR(%a0),%a1	#| get card address pointer
		mov.l	%a1,%d0			#| copy to d0
		addi.l	&INT_CMD,%d0		#| add offset to set intr
		mova.l	%d0,%a0			#| copy to address register
mod_retry:	mov.w	&HIGH,%sr		#| lock out all activity
		btst	&SEM_BIT,SEM_REG(%a1)	#| try to get semaphore
		bne.b	mod_failed		#| didn't get it
 #
 # Need at least 6 usec delay here for mux card. This uses snooze
 # so when future cpu's get faster this will still work.
 #
		movm.l	%d1/%a0-%a1,-(%sp)		#| save registers
		mov.l	&6,-(%sp)		#| push time onto stack
		jsr	_snooze			#| go wait
		addq	&4,%sp			#| adjust stack
		movm.l	(%sp)+,%d1/%a0-%a1		#| save registers
		or.b	&MOD_INT,(%a0)		#| set modem bit
		mov.b	%d0,SEM_REG(%a1)		#| clear semaphore reg
		mov.w	%d1,%sr			#| restore priorty
		rts
mod_failed:	mov.w	%d1,%sr			#| drop priorty to not miss intrs
		bra.b	mod_retry		#| try again
 #
 #  set_port_int
 #
 #	ENTRY:	a0 - mux_pointer
 #
 #		stack 	->	return address
 #				port int address
 #				INT_CMD mask (word)
 #				port mask (word)
 #
set_port_int:	mova.l	CARD_ADDR(%a0),%a0	#| get card address
		mova.l	4(%sp),%a1		#| get port address
		mov.l	%a0,%d0			#| copy to d0
		addi.l	&INT_CMD,%d0		#| add offset to set intr
		mov.w	%sr,%d1			#| save current priorty
port_retry:	mov.w	&HIGH,%sr		#| lock out all activity
		btst	&SEM_BIT,SEM_REG(%a0)	#| try to get semaphore
		bne.b	port_failed		#| didn't get it
		exg	%d0,%a0			#| swap registers
		swap	%d1			#| use byte for moving data
		mov.b	9(%sp),%d1		#| get first byte
		or.b	%d1,(%a1)			#| write CMD_TAB register of port
		mov.b	10(%sp),%d1		#| get second byte
		or.b	%d1,(%a0)			#| write port mask to command reg
		swap	%d1			#| readjust the data
		exg	%d0,%a0			#| swap registers
		mov.b	%d0,SEM_REG(%a0)		#| clear semaphore reg
		mov.w	%d1,%sr			#| restore priorty
		rts
port_failed:	mov.w	%d1,%sr			#| drop priorty to not miss intrs
		bra.b	port_retry		#| try again
 #
 #	mux_txempty(mux)
 #
 #		- Return 1 if transmit buffer is empty, else 0
 #
_mux_txempty:	mova.l	4(%sp),%a0	#| get mux pointer
		movq	&0,%d0		#| clear d0
		mova.l	VAR_LIST(%a0),%a1	#| get variable pointer list
		mov.b	TX_HEAD(%a1),%d1	#| get head offset
		cmp.b	%d1,TX_TAIL(%a1)	#| see if buffer is empty
		bne.b	not_txempty	#| no, just leave
		movq	&1,%d0		#| yes, it is empty
not_txempty:	rts
 #
 #	mux_rxempty(mux)
 #
 #		- Return 1 if receive buffer is empty, else 0
 #
_mux_rxempty:	mova.l	4(%sp),%a0	#| get mux pointer
		movq	&0,%d0		#| clear d0
		mova.l	VAR_LIST(%a0),%a1	#| get variable pointer list
		mov.b	RX_HEAD(%a1),%d1	#| get head offset
		cmp.b	%d1,RX_TAIL(%a1)	#| see if buffer is empty
		bne.b	not_rxempty	#| no, just leave
		movq	&1,%d0		#| yes, it is empty
not_rxempty:	rts
 #
 #	mux_modem_status(mux)
 #
 #		->	rts
 #			mux pointer
 #
 #		- return byte status
 #
_mux_modem_status:
		mova.l	4(%sp),%a0	#| get mux pointer
		movq	&0,%d0		#| clear d0
		mova.l	MOD_PT(%a0),%a1	#| get modem reg pointer
		mov.b	MOD_ST(%a1),%d0	#| get status byte
		rts
 #
 #	mux_modem_control(mux, control_mask)
 #
 #		->	rts
 #			mux pointer
 #			modem control byte
 #
 #		- return byte status
 #
_mux_modem_control:
		mova.l	4(%sp),%a0		#| get mux pointer
		mova.l	MOD_PT(%a0),%a1		#| get modem reg pointer
		mov.b	11(%sp),MOD_CT(%a1)	#| get status byte
		jsr	set_mod_int		#| send interrupt
		rts
 #
 #	mux_modem_mask(mux, mask)	-  set mask to get modem interrupts
 #
 #		->	rts
 #			mux pointer
 #			modem mask byte
 #
_mux_modem_mask:
		mova.l	4(%sp),%a0		#| get mux pointer
		mova.l	MOD_PT(%a0),%a1		#| get modem reg pointer
		mov.b	11(%sp),MOD_MK(%a1)	#| get status byte
		rts
 #
 #	mux_break(mux)	-  send break interrupt to card
 #
 #		->	rts
 #			mux pointer
 #
_mux_break:	mova.l	4(%sp),%a0		#| get mux pointer
		mov.b	PORT_MASK(%a0),-(%sp)	#| port mask for interrupt
		mov.w	&BREAK_INT,-(%sp)	#| break interrupt mask for intr
		mov.l	PORT_CMD_PT(%a0),-(%sp)	#| push addr for port interrupt
		jsr	set_port_int		#| send the interrupt
		addq	&8,%sp			#| re-adjust the stack
		rts
 #
 #	mux_bit_array(mux, character, flag)
 #			-  set the bit array for character according to flag
 #
 #		->	rts
 #			mux pointer
 #			character
 #			flag
 #
 #		- return byte status
 #
_mux_bit_array:	mova.l	4(%sp),%a0		#| get mux pointer
		mova.l	BIT_ARRAY(%a0),%a1	#| get pointer to bit array
		mov.l	8(%sp),%d0		#| get character
		add	%d0,%d0			#| double the value
		tst.b	15(%sp)			#| flag set?
		beq.b	bit_off			#| No, turn bit off
		mov.b	PORT_MASK(%a0),%d1
		or.b	%d1,0(%a1,%d0)		#| Yes, set bit
		rts
bit_off:		mov.b	PORT_MASK(%a0),%d1	#| get port mask
		not.b	%d1			#| invert bits
		and.b	%d1,0(%a1,%d0)		#| clear bit
		rts
 #
 #	mux_flush(mux)
 #
_mux_flush:	mova.l	4(%sp),%a0		#| get mux pointer
		mova.l	VAR_LIST(%a0),%a1		#| get variable list pointer
		mov.w	%sr,%d1			#| save current state
		mov.w	&HIGH,%sr		#| lock out others
		mov.b	TX_HEAD(%a1),%d0		#| get head pointer
		cmp.b	%d0,TX_TAIL(%a1)		#| empty already?
		beq.b	flush_done		#| Yes, leave
		addq	&1,%d0			#| increment head
		andi.b	&TX_MASK,%d0		#| wrap pointer
		mov.b	%d0,TX_TAIL(%a1)		#| set TAIL = HEAD + 1
flush_done:	mov.w	%d1,%sr			#| restore priorty
		rts
 #
 #	mux_config(mux, data)	-  configure the port
 #
 #		->	rts
 #			mux pointer
 #			data (word: [line:byte, baud:byte]
 #
_mux_config:	mova.l	4(%sp),%a0		#| get mux pointer
		mova.l	CONFIG_PT(%a0),%a1	#| get config pointer
		mov.b	10(%sp),LINE(%a1)		#| set line modes
		mov.b	11(%sp),BAUD(%a1)		#| set baud rate
		mov.b	PORT_MASK(%a0),-(%sp)	#| port mask for interrupt
		mov.w	&CONFIG_INT,-(%sp)	#| break interrupt mask for intr
		mov.l	PORT_CMD_PT(%a0),-(%sp)	#| push addr for port interrupt
		jsr	set_port_int		#| send the interrupt
		addq	&8,%sp			#| re-adjust the stack
		rts
 #
 #  mux_timeout_on
 #
 #		stack 	->	return address
 #
_mux_timeout_on:	mov.w	%sr,%d1			#| save current priorty
		mova.l	4(%sp),%a0		#| get mux pointer
		mov.l	CARD_ADDR(%a0),%a1	#| get card address pointer
		mov.l	%a1,%d0			#| copy to d0
		addi.l	&INT_CMD,%d0		#| add offset to set intr
		mova.l	%d0,%a0			#| copy to address register
tim_retry:	mov.w	&HIGH,%sr		#| lock out all activity
		btst	&SEM_BIT,SEM_REG(%a1)	#| try to get semaphore
		bne.b	tim_failed		#| didn't get it
		or.b	&TIME_INT,(%a0)		#| set modem bit
		mov.b	%d0,SEM_REG(%a1)		#| clear semaphore reg
		mov.w	%d1,%sr			#| restore priorty
		rts
tim_failed:	mov.w	%d1,%sr			#| drop priorty to not miss intrs
		bra.b	tim_retry		#| try again
