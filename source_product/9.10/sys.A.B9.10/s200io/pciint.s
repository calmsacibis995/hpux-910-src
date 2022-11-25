 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/pciint.s,v $
 # $Revision: 1.2.84.3 $	$Author: kcs $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/09/17 21:16:20 $
 # HPUX_ID: @(#)pciint.s	52.1		88/04/19 


 #(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
 #(c) Copyright 1979 The Regents of the University of Colorado,a body corporate 
 #(c) Copyright 1979, 1980, 1983 The Regents of the University of California
 #(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
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

 #****************************************************************************
 #
 # ATTENTION:  This file has been superceded by pci_intr() in pci.c
 #             This file is here for archive purposes only.
 #             MAKE NO CHANGES TO THIS FILE.  See pci.c, pci_intr().
 #****************************************************************************
 #
 #common ISR routine for receive and transmit to sort it out
 # if id=4 have rcvr interrupt
 #    id=2 have xmtr interrupt
 # any other combination is error
 # Reading the id register will clear the transmitter interrupt
 # but a character must be read to clear the receiver interrupt
		
 # interrupt structure offsets
 # THESE MUST BE KEPT IN LINE WITH THE C STRUCTURE DEFINITION
		
          	set	REG_OFFSET,0		#offset to card register address 
           	set	MASK_OFFSET,4		#offset to register mask value
            	set	VALUE_OFFSET,5		#offset to register value
        	set	NEXT_ISR,6		#offset to isr link field
          	set	ISR_OFFSET,10		#offset to address of isr routine
            	set	CHAIN_OFFSET,14		#offset to chain flag
           	set	MISC_OFFSET,15		#offset to misc field
          	set	TMP_OFFSET,16		#offset to temp field
               	set	ISR_STRUCT_SIZE,20		#size in bytes of interrupt structure
		
       		set	PCIINTS,0x15		#interrupt status register in uart

 # interrupt level offsets into the table of pointers to linked isr lists (rupttable)
	global	_npci			#pci driver figures this out
	global	_pciint
	global	_printf
		
_pciint:	mov.l	4(%sp),%a0		#get isr struct ptr (skip over ra)
	mov.l	TMP_OFFSET(%a0),%d1	#get pci structure pointer
	mov.l	REG_OFFSET(%a0),%a1	#get card address
	mov.b	PCIINTS-3(%a1),%d0	#get status register value
	cmp.b	%d0,&0x02		#transmitter?
	beq.w	pcix
	btst	&0,%d0			#really interrupting?
	beq.w	pcir			#modem, send to receiver
	mov.l	&badpci,-(%sp)		#nope, gripe
	jsr	_printf
	addq.l	&4,%sp
	rts	
		
pcix:	mov.l	%d1,-(%sp)		#call with line number
	jsr	_pcixint
	addq.l	&4,%sp			#pop arg
	rts	
		
pcir:	mov.l	%d1,-(%sp)		#call with line number
	jsr	_pcirint		
	addq.l	&4,%sp			#pop arg
	rts	

badpci:	byte	"unknown pci int type",0
