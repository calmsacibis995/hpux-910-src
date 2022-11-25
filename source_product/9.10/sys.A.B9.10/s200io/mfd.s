 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/mfd.s,v $
 # $Revision: 1.3.84.3 $	$Author: kcs $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/09/17 21:14:51 $
 # HPUX_ID: @(#)mfd.s	55.1		88/12/23 */


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
 #*****************************************************************************
 # High RAM Memory Map and Equates
 #*****************************************************************************
 #ndrives        equ 0xfffffed8  number of drives minus one (255 means no drives)
 #*****************************************************************************
 # Local data storage that use to be in Boot RAM
 #*****************************************************************************
          data
f_area:   long f_areabuf
f_areabuf:   byte  0,0,0,0,255,0x10,0,0,255,0x50,0
retry:     byte  0
mfdrive:   byte  0           #drive number
 #*****************************************************************************
 # Hardware Peripherals Equates                                               *
 #*****************************************************************************
               set fbuffer,0x44e000    #floppy ram buffer
               set xcmdreg,0x445000    #extended cmd register
               set clrxstus,0x445400    #clear extended status
               set cmdreg,0x44c000    #command register
               set trkreg,0x44c002    #track register
               set secreg,0x44c004    #sector register
               set datareg,0x44c006    #data register
 #*****************************************************************************
            text
	    global _f_pwr_on
_f_pwr_on:    rts		#dummy for boot rom compatibility
 #
fnot_present: mov  &2080,-2(%a5)                                          #{gll}
          mova.l  -10(%a5),%sp                                            #{gll}
          rts                                                            #{gll}
 #
 # BOOT ROM EXTENTIONS TO FLOPPY DRIVERS
 #
 #**********************************************************************
 #                                                                     *
 #            Mini-floppy MULTI_SECTOR READ                          *
 #                                                                     *
 #    Parameter stack                                                 *
 #                                                                     *
 #              sp + 12  -  NUMBER OF SECTORS TO READ                  *
 #              sp +  8  -  logical record number                      *
 #              sp +  4  -  data buffer address pointer                *
 #              sp --->  -  return address                             *
 #                                                                     *
 #**********************************************************************


 #**********************************************************************
 #                                                                     *
 #                            FLPYMREAD                                *
 #                                                                     *
 #    Outer block minifloppy read routine.  The necessary modules    *
 #  are called to perform the minifloppy read function.                *
 #                                                                     *
 #      a0 - xcmdcopy memory pointer                               r   *
 #      a1 - trkside memory pointer                                r   *
 #      a4 - floppy xcmdreg pointer                                r   *
 #      a6 - flag byte memory pointer                              r   *
 #      d4 - physical track and side passed to this routine        r   *
 #      d7 - copy of extended command register  (xcmdreg)         r/w  *
 #                                                                     *
 #**********************************************************************
	global _flpymread
_flpymread: jsr save_regs_params	#save regs & copy params for call
	jsr flpymreada 			#call pascal routine
flpyexit: mov.w -2(%a5),%d0		#d0 = error code
	ext.l %d0			#extend to long
	suba  &58,%a5			#to proper place to restore regs
	mova.l %a5,%sp			#restore stack pointer
	movm.l (%sp)+,%d2-%d7/%a2-%a6  	#restore regs
	rts

	global _flpymwrite
_flpymwrite: jsr save_regs_params	#save regs & copy params for call
	jsr flpymwritea		 	#call pascal intry point
	bra.w flpyexit

save_regs_params: mov.l (%sp)+,%a0	#get return address
	mov.b 19(%sp),mfdrive		#get the drive number
	movm.l %d2-%d7/%a2-%a6,-(%sp)	#save regs
	mova.l %sp,%a5
	adda   &58,%a5			#set a5 ^ high stack word.
	mov.l 	56(%sp),-(%sp)		#copy params
	mov.l 	56(%sp),-(%sp)		#copy params
	mov.l 	56(%sp),-(%sp)		#copy params
	subq.l &4,%sp			#pop stack to return address
	mov.l %sp,-10(%a5)		#save in excape buffer	
	addq.l &4,%sp			#restore stack pointer
	clr.w -2(%a5)			#default error code
	jmp (%a0)			#return to caller

flpymreada: cmpi.b _ndrives,&255            #check for no device present   {gll}
          beq.w  fnot_present                                            #{gll}
         mov.l %a6,-(%sp)              #save a6 for pascal
         jsr f_set_up                  #set up registers and motor on
         jsr drivechk
         jsr fmotoron                  #turn on floppy motor
frd_q_sect: jsr fconvert                #logical to side-track conversion
         cmp.b %d4,(%a1)                 #is floppy already at trk & side?
         beq.w frd_quic                #branch if no seek is necessary
         jsr fseek                     #move head to desired side-track
frd_quic: jsr fread                     #read data into floppy buffer
         jsr fbufmem                   #move data from floppy to memory
         addi.l  &256,8(%sp)            #increment the address
         addq.l  &1,12(%sp)             #increment the sector number
         subq.l  &1,16(%sp)             #decriment sector count
         bne.w     frd_q_sect

mfexit:   bclr &0,%d7                    #reset motor on bit
         mov.b %d7,xcmdreg                #turn off floppy motor
         mov.b %d7,(%a0)                #store away xcmdreg copy
         bclr &0,(%a6)                  #reset miniflop driver active
         mova.l (%sp)+,%a6              #restore a6 for pascal
         mova.l (%sp)+,%a0              #get return pointer
         adda   &12,%sp                 #pop parameters
         jmp (%a0)
         
 #**********************************************************************
 #                                                                     *
 #              Mini-floppy MULTI-SECTOR WRITE                       *
 #                                                                     *
 #    Parameter stack                                                 *
 #                                                                     *
 #              sp + 12  -  number of sectors to read                  *
 #              sp +  8  -  logical record number                      *
 #              sp +  4  -  data buffer address pointer                *
 #              sp --->  -  return address                             *
 #                                                                     *
 #**********************************************************************

 #**********************************************************************
 #                                                                     *
 #                          FLPYMWRITE                                 *
 #                                                                     *
 #    Outer block minifloppy WRITE routine.  The necessary modules   *
 #  are called to perform the minifloppy write function.               *
 #                                                                     *
 #      a0 - xcmdcopy memory pointer                               r   *
 #      a1 - trkside memory pointer                                r   *
 #      a4 - floppy xcmdreg pointer                                r   *
 #      a6 - flag byte memory pointer                              r   *
 #      d4 - physical track and side passed to this routine        r   *
 #      d7 - copy of extended command register  (xcmdreg)         r/w  *
 #                                                                     *
 #**********************************************************************

flpymwritea: cmpi.b _ndrives,&255            #check for no device present  {gll}
          beq.w  fnot_present                                            #{gll}
         mov.l %a6,-(%sp)             #save a6 for pascal
         jsr f_set_up                  #set up registers and motor on
         jsr drivechk
         jsr fmotoron                  #turn on floppy motor
flpy_qw_sect: jsr fconvert              #logical to side-track conversion
         cmp.b %d4,(%a1)                 #is floppy already at trk & side
         beq.w fwrtquic                #branch if no seek is necessary
         jsr fseek                     #move head to desired side-track
fwrtquic: jsr fmembuf                   #move data from memory to floppy
         jsr fwrite                    #write data into floppy buffer
         addi.l   &256,8(%sp)           #increment the address
         addq.l   &1,12(%sp)            #increment the sector number
         subq.l   &1,16(%sp)            #decriment the sector count
         bne.w      flpy_qw_sect
         bra.w mfexit                    #clean up and exit
 #**********************************************************************
 #                                                                     *
 #                         Mini-floppy driver                          *
 #                                                                     *
 #**********************************************************************
 #                                                                     *
 #                    Mini-floppy entry points                         *
 #                                                                     *
 #       These mass-memory (floppy) drivers can be entered in one of   *
 #  three primary places, determined by the name of the pascal calling *
 #  procedure INITIALIZE, READ, and WRITE.  Each of these       *
 #  routines calls a set of modules (subroutines) to perform its       *
 #  driver function.                                                   *
 #                                                                     *
 #       For reasons of speed, most parameters are passed from module  *
 #  to module via the registers.  The assigned definition of all the   *
 #  the registers remains relatively constant thoughout these drivers. *
 #  The head of each routine defines the register usages for that      *
 #  routine.  Those people making direct entrys into any module must   *
 #  establish register and global storage requirements prior to entry. *
 #                                                                     *
 #**********************************************************************

 #**********************************************************************
 #                                                                     *
 #                      Flag byte bit definitions                      *
 #                                                                     *
 #      Bit #                           Use                            *
 #      =====    ================================================      *
 #        0      minifloppy driver active  (valid interrupt)           *
 #        1      expecting interrupt  (set while awaiting intr)        *
 #        2      interrupt occured  (set by interrupt handler)         *
 #        3      (not used)                                            *
 #        4      restore flag  (set when a restore has occured)        *
 #        5      reset flag  (set on reset during read or write)       *
 #        6      (not used)                                            *
 #        7      (not used)                                            *
 #                                                                     *
 #**********************************************************************
 #                                                                     *
 #                              FCONVERT                               *
 #                                                                     *
 #    This subroutine is used to convert the logical record address    *
 #  passed on the system stack, sp + 18 (at time used), to a target    *
 #  side, track, and sector                                            *
 #    An offset stored in own common memory and determined during    *
 #  the previous floppy access is added to this target address to      *
 #  account for possible spared tracks.  In the long term, this will   *
 #  reduce the total number of floppy seeks required.  The dimension   *
 #  of the offset is sides.                                          *
 #    No error checking is done. It is assumed that the logical record *
 #  is a valid address.                                                *
 #                                                                     *
 #      sp - (a7) stack pointer                                    r   *
 #      d3 - used to manipulate the offset address                r/w  *
 #      d4 - physical track and side   (lsb = side)                w   *
 #      d5 - logical track and side   (lsb = side)                r/w  *
 #      d6 - physical and logical sector                           w   *
 #      d6 - physical and logical sector                           w   *
 #**********************************************************************

fconvert: mov.w 18(%sp),%d5              #get logical address from stack
         mov.w %d5,%d6                  #copy to sector register
         and.w &0xf,%d6                  #convert to sector address
         lsr.w &4,%d5                   #target track and (side=lsb)
 #        move.b foffset,d3                                            {ag}
         mov.b 2(%a1),%d3               #get previous offset from own {ag}
         mov.b %d5,%d4                  #copy target trk/side to seek reg
         add.b %d3,%d4                   #seek track and (side=lsb)
         rts

 #**********************************************************************
 #                                                                     *
 #                               FREAD                                 *
 #                                                                     *
 #    This subroutine is used to read data from the floppy media into  *
 #  the floppy ram buffer.                                             *
 #    If and error occures on attempting to read the data from the     *
 #  media, three retrys are attempted before reporting an error code   *
 #  and aborting the read.                                             *
 #                                                                     *
 #      a2 - flopper fbuffer pointer                               r   *
 #      a3 - floppy cmdreg pointer                                 r   *
 #      a4 - floppy xcmdreg pointer                                r   *
 #      d0 - general - floppy status register copy - error #      r/w  *
 #      d1 - sr reg copy prior to interrupt disable                w   *
 #      d5 - logical track and side                                r   *
 #      d6 - sector to be read                                     r   *
 #      d7 - copy of extended command register  (xcmdreg)         r/w  *
 #      sr - disable interrupts                                   r/w  *
 #                                                                     *
 #**********************************************************************

 #fread    move.b #3,re_rdwrt            initialize reread counter to 3 {ag}

fread:     mov.l   %a0,-(%sp)             #save a0 for now                {ag}
          mova.l  f_area,%a0            #a0 points to floppy temp area  {ag}
          addq     &1,%a0                                               #{ag}
          mov.b   &3,(%a0)                                             #{ag}  

fretryrd: mov.b %d6,secreg              #write sector to sector register
         mov.b (%a2),%d0                #dummy read of floppy buffer
         bset &5,%d7                    #set local bit in xcmdreg
         bset &4,%d7                    #set read bit in xcmdreg
         mov %sr,%d1                    #copy current interrupt level
         ori &0x2700,%sr                 #disable interrupts >>>>>>>>>>>>>
         btst &0,%d5                    #is side zero or one desired?
         beq.w f_side_0                #branch to command for side zero
         mov.b &0x8a,(%a3)              #issue read side one to cmd reg
         bra.w f_side_1                #jump over side zero command
f_side_0: mov.b &0x82,(%a3)              #issue read side zero to cmd reg
f_side_1: mov.b %d7,(%a4)                #send copy to xcmdreg
         jsr fwaitinr                  #wait for interrupt
         mov.b (%a3),%d0                #get status word from floppy
         andi.b &0xbc,%d0                #check bits 2, 3, 4, 5 and 7
         bne.w frdretry                #if error bit set, retry
         mova.l  (%sp)+,%a0                                               #{ag}
         rts

 #frdretry subi.b #1,re_rdwrt            decrement reread counter         {ag}
frdretry:  subq.b &1,(%a0)                                                 #{ag}

         bne.w fretryrd                #is reread count expired (zero)?
         btst &7,%d0                    #is error bit 7 set?
         beq.w fbit3tst                #branch if not set
         mov.w &1080,%d0               #\\\\\\\\ error 80,1 ////////
         bra.w ferrexit                  #abort with error
fbit3tst: btst &3,%d0                    #is error bit 3 set?
         beq.w fbit2tst                #if not, test bit 2
         mov.w &1088,%d0               #\\\\\\\\ error 88,1 ////////
         bra.w ferrexit                  #abort with error
fbit2tst: btst &2,%d0                    #is error bit 2 set?
         beq.w frnf_err                #if not, default bit 4 or 5 error
         mov.w &4081,%d0               #\\\\\\\\ error 81,4 ////////
         bra.w ferrexit                  #abort with error
frnf_err: mov.w &1084,%d0               #\\\\\\\\ error 84,1 ////////
         bra.w ferrexit                  #abort with error

 #**********************************************************************
 #                                                                     *
 #                              FBUFMEM                                *
 #                                                                     *
 #    This subroutine transfers 256 bytes (one sector) from the        *
 #  floppy buffer to the memory buffer pointed to by the pascal        *
 #  parameter left on the system stack at sp + 20 (at time used).      *
 #    There is no error checking to be done.                           *
 #                                                                     *
 #      a0 - memory buffer address                                r/w  *
 #      a2 - floppy ram buffer address                             r   *
 #      sp - (a7) stack pointer                                    r   *
 #      d0 - contains constant value of 64                        r/w  *
 #      d1 - used as a one long word buffer                       r/w  *
 #                                                                     *
 #**********************************************************************

fbufmem:  movm.l %a0/%a2,-(%sp)           #stack a0, a2  (need two temps)
         mova.l 20(%sp),%a0             #get memory buffer address
         mov.l &64,%d0                 #initialize 8 * 64 = 256 bytes
fbufloop: movp.l 0(%a2),%d1              #floppy buffer to temp reg
         mov.l %d1,(%a0)+               #temp reg to memory buffer
         addq.w &8,%a2                  #add 8 to form word boundry
         subq.b &1,%d0                  #decrement 8byte counter
         bgt.w fbufloop                #have 256 bytes been transfered?
         movm.l (%sp)+,%a0/%a2           #restore stacked a0, a2
         rts

 #**********************************************************************
 #                                                                     *
 #                             FMOTORON                                *
 #                                                                     *
 #    This subroutine is used to turn on the floppy drive motor if     *
 #  it is not already running, and keep it on if it is already on.     *
 #    A check for media change is made prior to the attempt to turn    *
 #  on the motor.                                                      *
 #                                                                     *
 #      a0 - xcmdcopy memory pointer                               r   *
 #      a3 - floppy cmdreg pointer                                 r   *
 #      a4 - floppy xcmdreg pointer                                r   *
 #      d0 - copy of floppy status - elaspted time for clock      r/w  *
 #      d7 - copy of extended command register  (xcmdreg)         r/w  *
 #                                                                     *
 #**********************************************************************

fmotoron: jsr fmediack                #has media been changed?
         bset &0,%d7                    #set motor on bit for xcmdreg
         mov.b %d7,(%a4)                #send copy to xcmdreg
         mov.b %d7,(%a0)                #send xcmdreg to copy
         mov.b (%a3),%d0                #get floppy status
         btst &7,%d0                    #is drive ready?
         bne.w fwaitinx                #wait for index pulse if not rdy
fmotorts: rts

fwaitinx: mov.b &6,retry               #50ms x 6 = 300ms max wait time
fmtrloop: movq  &50,%d0                 #initialize for 50 milliseconds
         jsr f_clock                   #wait 50 milliseconds
         mov.b (%a3),%d0                #get floppy status
         btst &7,%d0                    #is drive ready?
         beq.w fmotorts                #branch if not yet index hole  
         subq.b &1,retry               #decrement retry counter
         bgt.w fmtrloop                #branch if 300ms not yet elasped
         mov.w &2080,%d0               #\\\\\\\\ error 80,2 ////////
         bra.w ferrexit                  #abort with error

 #**********************************************************************
 #                                                                     *
 #                             FMEDIACK                                *
 #                                                                     *
 #    This subroutine is used to determine if the media has been       *
 #  removed from or replaced into the drive.                           *
 #                                                                     *
 #      a4 - extended status reg pointer                           r   *
 #      d0 - floppy status register copy - error number           r/w  *
 #                                                                     *
 #**********************************************************************

fmediack: mov.b (%a4),%d0                #get extended status reg
         btst &1,%d0                    #is media change bit set?
         bne.w fchanged                #report media fchangedif bit on
         rts

fchanged: clr.b clrxstus                #clear floppy extended status
         mov.w &8080,%d0               #\\\\\\\\ error 80,8 ////////
         bra.w frstexit                  #abort with error (in ferrexit)
 # flpyseek
         
 #**********************************************************************
 #                                                                     *
 #                               Fseek                                 *
 #                                                                     *
 #    This routine attempts to move the head to the desired target     *
 #  track and selects the desired head (side 0 or 1).  Error checking  *
 #  is performed to insure the target track and side have been located.*
 #    It should be noted that spare tracks will cause a seek error     *
 #  to occure, thus a new track and side address must be calculated    *
 #  in an attempt to locate the correct target address.                *
 #    Note also that a seek without verify followed by a read address  *
 #  is performed rather that a seek with verify.  Using the seek with  *
 #  verify causes a one second hardware loop to be entered for every   *
 #  seek error that occures (and hence for every spared track encount- *
 #  ered).  Also by reading the address it is possible to calculate    *
 #  a new address using that present address and the target address    *
 #  rather than stepping one track at a time.                          *
 #    Five reseeks are attempted twice; five before and five more      *
 #  again after a restore of the drive.                                *
 #                                                                     *
 #      a1 - trkside memory pointer                                r   *
 #      a3 - floppy cmdreg pointer                                 r   *
 #      a4 - floppy xcmdreg pointer                                r   *
 #      a6 - flag byte memory pointer                              r   *
 #      d0 - temperary storage - error number                     r/w  *
 #      d1 - sr copy prior to interrupt disable                    w   *
 #      d2 - present track and side after a read address           r   *
 #      d3 - offset (number of spare tracks passed over)          r/w  *
 #      d4 - physical track and side passed to this routine       r/w  *
 #      d5 - logical track and side passed to this routine         r   *
 #      d7 - copy of extended command register  (xcmdreg)         r/w  *
 #      sr - disable interrupts                                   r/w  *
 #                                                                     *
 #  Bug history                                                       *
 #    {bug100} - check for negative offset    fixed 12/17/80    dfk    *
 #                                                                     *
 #**********************************************************************

fseek:    movq &1,%d0                   #wait one millisecond for
         jsr f_clock                   #write head (erase) to settle
         mov.b (%a1),%d0                #get current floppy track/side
         bge.w f_track                 #valid track if not negative
         mov %sr,%d1                    #copy current interrupt level
         ori &0x2700,%sr                 #disable interrupts >>>>>>>>>>>>>
         mov.b &0x0a,(%a3)              #restore floppy to trk 0
         jsr fwaitinr                  #wait for op complete
         clr.w %d0                      #force to track zero if negative
f_track:  lsr.b &1,%d0                   #strip off side bit (=trk)
         mov.b %d0,trkreg              #write track to floppy
         bclr &4,(%a6)                  #clear restore bit in flag byte
 
 #        move.b #5,reseek              initialize reseek to 5            {ag}
         mov.l  %a0,-(%sp)                                                #{ag}
         mova.l f_area,%a0                                               #{ag}
         mov.b  &5,2(%a0)                                                #{ag}
         mova.l (%sp)+,%a0                                                #{ag}

freseek:  mov.b %d4,%d0                  #get seek track and side
         andi.b &1,%d0                  #derive the side
         lsl.b &2,%d0                   #align side bit for xcmdref
         bclr &2,%d7                    #zero the side bit
         or.b %d0,%d7                    #set side bit in xcmd reg copy
         mov.b %d4,%d0                  #get seek track and side
         lsr.b &1,%d0                   #convert to track
         mov.b %d0,datareg             #write track to data register
         bclr &1,%d7                    #zero pre-comp bit in xcmd copy
         cmpi.b %d0,&18                 #is track less than 18?
         blt.w fseektrk                #branch if less than 18
         bset &1,%d7                    #set pre-comp bit in xcmd copy
fseektrk: mov.b %d7,(%a4)                #write copy to xcmdreg
         mov %sr,%d1                    #copy current interrupt level
         ori &0x2700,%sr                 #disable interrupts >>>>>>>>>>>>>
         mov.b &0x18,(%a3)              #seek and load without verify
         jsr fwaitinr                  #wait for operation complete
         movq &15,%d0                  #wait 15 milliseconds for
         jsr f_clock                            #heads to settle
         jsr frdaddrs                  #read address from floppy
         bclr &5,(%a6)                  #was a reset done in frdaddrs ?
         bne.w frestore                #restore and restart seek op
         cmp.b %d2,%d5                   #are we on the desired trk/side?
         bne.w fseekerr                #error if wrong track or side
         mov.b %d5,%d0                  #get current logical track
         lsr.b &1,%d0                   #strip off side bit
         mov.b %d0,trkreg              #set floppy track reg = log. trk.
         mov.b %d4,(%a1)                #store current trk/side
         rts
         
 #fseekerr subi.b #1,reseek              decrement reseek counter          {ag}
fseekerr: mov.l   %a0,-(%sp)                                                #{ag}
         mova.l  f_area,%a0                                               #{ag}
         subq.b   &1,2(%a0)                                                #{ag}
         mova.l  (%sp)+,%a0                                                #{ag}
 
         ble.w frestore                #go back & calculate new address
         cmpi.b %d2,&0xff                #check read addr for spared track
         beq.w fstepone                #branch if spared track
         cmpi.b %d2,&0x45                #read addr > trk 34 side 1 ?
         bgt.w frestore                #branch if addr to high
         mov.b %d5,%d0                  #get target track and side
         sub.b %d2,%d0                   #subtract present ads from target
         add.b %d0,%d4                   #add difference to seek trk/side
         add.b %d3,%d0                   #add offset to difference
         mov.b %d0,%d3                  #copy new offset to offset reg
foffchk:  cmpi.b %d3,&5                  #five allows for one step error
         bgt.w frestore                #branch if offset too great
         tst.b %d3                      #is offset negative ?            {bug100}
         blt.w frestore                #hardware miss-stepped           {bug100}
 #fstoroff move.b d3,foffset             store away new offset          {ag}
fstoroff: mov.b %d3,2(%a1)                #store away new offset          {ag}
         bra.w freseek                   #retry with new calculated addr
         
fstepone: addq.b &1,%d4                  #increment seek trk/side
         addq.b &1,%d3                  #increment offset
         bra.w foffchk                 #branch and store offset
         
frestore: bset &4,(%a6)                  #is restore flag set?   set flag
         beq.w fskrstor                #restore if flag was not set
         mov.w &1081,%d0               #\\\\\\\\ error 81,1 ////////
         bra.w ferrexit                  #abort with error
fskrstor: bclr &2,%d7                    #clear side bit in xcmd reg copy
         mov.b %d7,(%a4)                #set xcmd reg side bit to zero
         mov %sr,%d1                    #copy current interrupt level
         ori &0x2700,%sr                 #disable interrupts >>>>>>>>>>>>>
         mov.b &0x0a,(%a3)              #restore floppy to trk 0
         jsr fwaitinr                  #wait for op complete
         jsr frdaddrs                #read address, should be zero
         btst &5,(%a6)                  #was a reset done in frdaddrs ?
         beq.w fchkaddr                #if no reset then check addr
         mov.w &3081,%d0               #\\\\\\\\ error 81,3 ////////
         bra.w ferrexit                  #abort with error
fchkaddr: tst.b %d2                      #are we on track and side zero?
         beq.w fzerooff                #go zero offset if trk/side = 0
         mov.w &2081,%d0               #\\\\\\\\ error 81,2 ////////
         bra.w ferrexit                  #abort with error
fzerooff: clr.b %d3                      #reset offset to zero
         mov.b %d5,%d4                  #set seek trk/side = target
         
 #        move.b  #5,reseek              retry 5 more times after restore  {ag}
         mov.l  %a0,-(%sp)                                                 #{ag}
         mova.l f_area,%a0                                                #{ag}
         mov.b  &5,2(%a0)                                                 #{ag}
         mova.l (%sp)+,%a0                                                 #{ag}

         bra.w fstoroff                #start over after the restore

 #**********************************************************************
 #                                                                     *
 #                             FRDADDRS                                *
 #                                                                     *
 #    This subroutine is used to determine the present address of the  *
 #  floppy drive heads and side by use of a read address command.    *
 #    If and error occures on attempting to read the address from the  *
 #  floppy, three retry are tried twice; three before a reset is done  *
 #  and three times again after a reset is attempted.  If all these    *
 #  attempts fail an error code is returned.                           *
 #                                                                     *
 #      a2 - floppy fbuffer pointer                                r   *
 #      a3 - floppy cmdreg pointer                                 r   *
 #      a4 - floppy xcmdreg pointer                                r   *
 #      a6 - flag byte memory pointer                              r   *
 #      d0 - general - floppy status register copy - error #      r/w  *
 #      d1 - sr reg copy prior to interrupt disable                w   *
 #      d2 - present track and side    (lsb = side)                w   *
 #      d7 - copy of extended command register  (xcmdreg)         r/w  *
 #      sr - disable interrupts                                   r/w  *
 #                                                                     *
 #**********************************************************************

frdaddrs: bclr &5,(%a6)                  #clear reset bit in flag byte
fretryad: mov.b &3,retry               #initialize retry counter to 3
fadretry: mov.b (%a2),%d0                #dummy read of floppy buffer
         bset &5,%d7                    #set local bit in xcmdreg
         mov %sr,%d1                    #copy current interrupt level
         ori &0x2700,%sr                 #disable interrupts >>>>>>>>>>>>>
         mov.b &0xc0,(%a3)              #send read address command
         mov.b %d7,(%a4)                #send copy to xcmdreg
         jsr fwaitinr                  #wait for interrupt
         mov.b (%a2),%d2                #get track number from floppy
         lsl.b &1,%d2                   #start conversion to trk-side
         mov.b 2(%a2),%d0               #get side from floppy
         andi.b &1,%d0                  #strip off side bit
         or.b %d0,%d2                    #add side bit to trk/side
         mov.b (%a3),%d0                #get status byte from floppy
         andi.b &0x1c,%d0                #check bits 2,3, and 4
         bne.w ferretry                #if error bit set, check retrys
         rts

ferretry: subq.b &1,retry               #decrement retry counter(zero)?
         bne.w fadretry                #is retry count expired (zero)?
         bset &5,(%a6)                  #is resetflag set?    set flag
         bne.w fadrserr                #if flag was set, record error
         jsr freset                  #reset the floppy drive
         bra.w fretryad                #try again
fadrserr: btst &4,%d0                    #is error bit 4 set?
         beq.w ftst3bit                #if not, test bit 3
         mov.w &3084,%d0               #\\\\\\\\ error 84,3 ////////
         bra.w ferrexit                  #abort with error
ftst3bit: btst &3,%d0                    #is error bit 3 set?
         beq.w fbit2err                #if not, default to bit 2 error
         mov.w &1087,%d0               #\\\\\\\\ error 87,1 ////////
         bra.w ferrexit                  #abort with error
fbit2err: mov.w &6081,%d0               #\\\\\\\\ error 81,6 ////////
         bra.w ferrexit                  #abort with error
         
 #**********************************************************************
 #                                                                     *
 #                              FRESET                                 *
 #                                                                     *
 #    This subroutine is used to reset the floppy drive and its        *
 #  controller.  It is used as a last ditch effort to recover from     *
 #  an error condition, and just prior to a media initialization.      *
 #                                                                     *
 #      a0 - xcmdcopy memory pointer                               r   *
 #      a1 - trkside memory pointer                                r   *
 #      a3 - floppy cmdreg pointer                                 r   *
 #      a4 - floppy xcmdreg pointer                                r   *
 #      d0 - f_clock time  (centiseconds)                          w   *
 #      d1 - sr reg copy prior to interrupt disable                w   *
 #      d7 - copy of extended command register  (xcmdreg)         r/w  *
 #      sr - disable interrupts                                   r/w  *
 #                                                                     *
 #  Bug history                                                       *
 #    {bug80} - side zero bit not being reset    fixed 12/15/80   dfk  *
 #                                                                     *
 #**********************************************************************

freset:   jsr fmediack                  #has media been removed?
fresetqk: mov.b (%a3),%d0                #get status byte from floppy
         btst &2,%d0                    #is track 0 bit set ?
         beq.w fdoreset                #branch if not on track zero
         clr.b trkreg                  #clear floppy track reg
         mov.b &4,datareg             #step in; insure not on trk # -1
         mov %sr,%d1                    #copy current interrupt level
         ori &0x2700,%sr                 #disable interrupts >>>>>>>>>>>>>
         mov.b &0x18,(%a3)              #seek, head loaded without verify
         jsr fwaitinr                  #wait for operation complete
 #fdoreset bclr #3,d7                    turn off reset/   (reset quick) {bug80}
fdoreset: andi.b &0xf1,%d7                #reset, side zero, pre-comp/      {bug80}
         mov.b %d7,(%a4)                #send copy to xcmdreg
         movq  &10,%d0                 #initialize clock for 10 ms
         jsr f_clock                   #wait 10 milliseconds
         bset &3,%d7                    #turn off reset pulse
         mov %sr,%d1                    #copy current interrupt level
         ori &0x2700,%sr                 #disable interrupts >>>>>>>>>>>>>
         mov.b %d7,(%a4)                #send to xcmd register
         jsr fwaitinr                  #wait for command to complete
         mov.b %d7,(%a0)                #save xcmdreg copy
         clr.b clrxstus                #clear extended status register
 #        clr.b foffset                 set offset to zero              {ag}
         clr.b 2(%a1)                   #set offset to zero              {ag}
         mov.b &0xff,(%a1)              #force seek on next read/write
         rts

 #**********************************************************************
 #                                                                     *
 #                              F-SET-UP                               *
 #                                                                     *
 #    Used at beginning of each entry routine to set up registers      *
 #  and set driver active flag.                                        *
 #                                                                     *
 #      a0 - xcmdcopy memory pointer                              r/w  *
 #      a1 - trkside memory pointer                               r/w  *
 #      a2 - floppy fbuffer pointer                                w   *
 #      a3 - floppy cmdreg pointer                                 w   *
 #      a4 - floppy xcmdreg (write) and xstatus (read) pointer     w   *
 #      a6 - flag byte memory pointer                             r/w  *
 #      d7 - copy of extended command register  (xcmdreg)          w   *
 #                                                                     *
 #**********************************************************************

 #
 #f_set_up lea xcmdcopy,a0               xcmdreg copy memory pointer      {ag}
 #         lea trkside,a1                trkside memory pointer           {ag}
 #         lea fbuffer,a2                floppy buffer pointer            {ag}
 #         lea cmdreg,a3                 floppy command reg pointer       {ag}
 #         lea xcmdreg,a4                floppy extented command reg ptr  {ag}
 #         lea fflags,a6                 flags byte memory pointer        {ag}
 #         bset #0,(a6)                  set miniflop driver active     {ag}
 #         move.b (a0),d7                initialize xcmdcopy in d7        {ag}
 #         rts                                                            {ag}

f_set_up:  mova.l   f_area,%a0                                            #{ag}
          lea      3(%a0),%a6                                              #{ag}
          lea      4(%a0),%a1                                              #{ag}
          lea      5(%a0),%a0                                              #{ag}
          lea      fbuffer,%a2                                            #{ag}
          lea      cmdreg,%a3                                             #{ag}
          lea      xcmdreg,%a4                                            #{ag}
          movq    &0,%d7                                                 #{ag}
          mov.b   mfdrive,%d7                                              #{ag}
          lsl      &2,%d7                                                 #{ag}
          adda     %d7,%a0                                                 #{ag}
          adda     %d7,%a1                                                 #{ag}
          adda     %d7,%a6                                                 #{ag}
          bset     &0,(%a6)                                               #{ag}
          mov.b   (%a1),%d7      #d7 = trkside                             {ag}
          cmp.b    %d7,&0xff                                               #{ag}
          beq.w    i_lost_it                                             #{ag}
          sub.b    2(%a1),%d7     #subtract offset
          lsr.b    &1,%d7                                                 #{ag}
          mov.b   %d7,trkreg     #set up the track register               {ag}
i_lost_it: mov.b   (%a0),%d7                                               #{ag}
          mov.b   %d7,(%a4)
          rts                                                            #{ag}
drivechk:  tst.b  mfdrive                                                 #{ag}
          beq.w    driveok                                               #{ag}
          tst.b    _ndrives                                               #{ag}
          bne.w    driveok                                               #{ag}
          mov     &1082,%d0                                              #{ag}
          bra.w      frstexit                                              #{ag}
driveok:   mov.b   (%a0),%d7       #put xcmdcopy in d7                      {ag}
          rts

 #**********************************************************************
 #                                                                     *
 #                              FWRITE                                 *
 #                                                                     *
 #    This subroutine is used to write data from a memory buffer into  *
 #  the floppy ram buffer.                                             *
 #    If and error occures on attempting to write the data on to the   *
 #  media, three retrys are attempted before reporting an error code   *
 #  and aborting the write.                                            *
 #                                                                     *
 #      a2 - flopper fbuffer pointer                               r   *
 #      a3 - floppy cmdreg pointer                                 r   *
 #      a4 - floppy xcmdreg pointer                                r   *
 #      d0 - general - floppy status register copy - error #      r/w  *
 #      d1 - sr reg copy prior to interrupt disable                w   *
 #      d5 - logical track and side                                r   *
 #      d6 - sector to be written                                  r   *
 #      d7 - copy of extended command register  (xcmdreg)         r/w  *
 #      sr - disable interrupts                                   r/w  *
 #                                                                     *
 #**********************************************************************

 #fwrite   move.b #3,re_rdwrt            reread also used as rewrite cntr {ag}
fwrite:   mov.l   %a0,-(%sp)                                               #{ag}
         mova.l  f_area,%a0                                              #{ag}
         addq     &1,%a0                                                  #{ag}
         mov.b   &3,(%a0)                                                #{ag}
         
fretrywt: mov.b %d6,secreg              #write sector to sector register
         mov.b (%a2),%d0                #dummy read of floppy buffer
         bclr &4,%d7                    #reset read bit in xcmdreg
         bset &5,%d7                    #set local bit in xcmdreg
         mov %sr,%d1                    #copy current interrupt level
         ori &0x2700,%sr                 #disable interrupts >>>>>>>>>>>>>
         btst &0,%d5                    #is side zero or one desired?
         beq.w f_0_side                #branch to command for side zero
         mov.b &0xaa,(%a3)              #issue write side one to cmd reg
         bra.w f_1_side                #jump over side zero command
f_0_side: mov.b &0xa2,(%a3)              #issue write side zero to cmd reg
f_1_side: mov.b %d7,(%a4)                #send copy to xcmdreg
         jsr fwaitinr                  #wait for interrupt
         mov.b (%a3),%d0                #get status word from floppy
         andi.b &0xfc,%d0                #check bits 2, 3, 4, 5, 6 and 7
         bne.w fwtretry                #if error bit set, retry
         bset &4,%d7                    #set xcmdreg copy to read standby
         mova.l (%sp)+,%a0                                                 #{ag}
         rts

fwtretry: btst &6,%d0                    #is write protect bit set?
         bne.w fwrtprot                #branch if write protected
 #        subi.b #1,re_rdwrt            decrement rewrite (reread) cntr    {ag}
         subq.b &1,(%a0)                                                   #{ag}
         
         bne.w fretrywt                #is rewrite count expired (zero)?
         btst &7,%d0                    #is error bit 7 set?
         beq.w ftstbit4                #branch if bit is not set
         mov.w &3080,%d0               #\\\\\\\\ error 80,3 ////////
         bra.w ferrexit                #abort with error
ftstbit4: btst &4,%d0                    #is error bit 4 set?
         beq.w ftstbit3                #if not, test bit 3
         mov.w &2084,%d0               #\\\\\\\\ error 84,2 ////////
         bra.w ferrexit                #abort with error
ftstbit3: btst &3,%d0                    #is error bit 3 set?
         beq.w flosterr                #if not, default bit 2 or 5 error
         mov.w &7081,%d0               #\\\\\\\\ error 81,7 ////////
         bra.w ferrexit                #abort with error
flosterr: mov.w &5081,%d0               #\\\\\\\\ error 81,5 ////////
         bra.w ferrexit                #abort with error
fwrtprot: mov.w &1083,%d0               #\\\\\\\\ error 83,1 ////////
         bra.w ferrexit                #abort with error

 #**********************************************************************
 #                                                                     *
 #                              FMEMBUF                                *
 #                                                                     *
 #    This subroutine transfers 256 bytes (one sector) from buffer     *
 #  memory pointed to by the pascal parameter left on the stack        *
 #  at sp + 20 (at time used) to the floppy ram buffer.                *
 #    There is no error checking to be done.                           *
 #                                                                     *
 #      a0 - memory buffer address                                r/w  *
 #      a2 - floppy ram buffer address                             r   *
 #      sp - (a7) stack pointer                                    r   *
 #      d0 - contains constant value of 64                        r/w  *
 #      d1 - used as a one long word buffer                       r/w  *
 #                                                                     *
 #**********************************************************************

fmembuf:  movm.l %a0/%a2,-(%sp)           #stack a0, a2  (need two temps)
         mova.l 20(%sp),%a0             #get memory buffer address
         movq  &64,%d0                 #initialize 4 * 64 = 256 bytes
floopbuf: mov.l (%a0)+,%d1               #memory buffer to temp reg
         movp.l %d1,0(%a2)              #temp reg to floppy buffer
         addq.w &8,%a2                  #add 8 to form word boundry
         subq.b &1,%d0                  #decrement 8-byte loop counter
         bgt.w floopbuf                #have 256 bytes been transferred?
         movm.l (%sp)+,%a0/%a2           #restore stacked a0, a2
         rts

 #**********************************************************************
 #                                                                     *
 #                              FERREXIT                               *
 #                                                                     *
 #    This routine accomplishes the abort and exit of all the          *
 #  mass memory routines.  The floppy is reset and left in remote      *
 #  on track zero, side zero, and motor off.                           *
 #    This routine is totally self contained (no other subroutines     *
 #  are called) to prevent the possibility of endless recurrsive loops.*
 #                                                                     *
 #      a1 - trkside memory pointer                                r   *
 #      a3 - floppy cmdreg pointer                                 r   *
 #      a4 - floppy xcmdreg pointer                                r   *
 #      a5 - g$, pascal global data area pointer                   r   *
 #      a6 - flag byte memory pointer                              r   *
 #      sp - (a7) stack pointer                                    r   *
 #      d0 - error code number passed into this routine            r   *
 #      d1 - temperary general purpose storage                    r/w  *
 #                                                                     *
 #**********************************************************************

ferrexit: mov.b (%a4),%d1                #get extended status for media ck
         btst &1,%d1                    #is media fchangedbit set?
         beq.w frstexit                #branch if not set
         mov.w &9080,%d0               #\\\\\\\\ error 80,9 ////////
frstexit: tst.b  mfdrive                                                  #{ag}
         beq.w  frstex2                                                #{ag}
         mov.b &0x58,(%a4)                                              #{ag}
         mov.b &0x58,1(%a1)                                              #{ag}
         bra.w  frstex3                                                #{ag}
frstex2:  mov.b &0x18,(%a4)              #floppy standby, * motor off *
         mov.b &0x18,1(%a1)             #save xcmdreg copy
frstex3:  mov.b &0xd0,(%a3)              #cancel any floppy commands
         clr.b clrxstus                #clear extended status register
         clr.b (%a6)                    #clear flag bits
 #        clr.b foffset                 zero the offset variable         {ag}
         clr.b 2(%a1)                   #zero the offset variable         {ag}
         mov.b &0xff,(%a1)              #force seek on next read/write
         mov.w %d0,-2(%a5)              #store error code for escape
         mova.l -10(%a5),%sp            #return to pascal error escape
         mov.b (%a3),%d1                #clear any interrupt from floppy
fierrts:  rts

 #**********************************************************************
 #                                                                     *
 #                              FWAITINR                               *
 #                                                                     *
 #    This is the interrupt handler routine.  This routine is entered  *
 #  just prior to enabling interrupts.  The isr is enterd when the   *
 #  interrupt occures.  The interrupt must occure within 2 seconds of  *
 #  calling this routine or a error is reported.  Likewise, if an      *
 #  interrupt occures which was not expected by this routine an        *
 #  error is reported.                                                 *
 #                                                                     *
 #      a3 - floppy cmdreg pointer                                 r   *
 #      a4 - floppy xcmdreg pointer                                r   *
 #      a6 - flag byte memory pointer                              r   *
 #      2(a6) = xcmdcopy
 #      d0 - clock elasped time counter - error number            r/w  *
 #      d1 - sr reg copy prior to interrupt disable                w   *
 #      d7 - copy of extended command register  (xcmdreg)         r/w  *
 #                                                                     *
 #**********************************************************************

 #fwaitinr move.b d7,xcmdcopy            store away current xcmdreg       {ag}
fwaitinr:  mov.b %d7,2(%a6)                                                #{ag}

         bset &1,(%a6)                  #set expecting interrupt flag bit
         bclr &2,(%a6)                  #clear interrupt occured flag
         mov %d1,%sr                    #enable interrupts <<<<<<<<<<<<<<
         mov.w &2000,%d0               #initialize clock for 2 seconds
         cmpi.w %d1,&0x2200              #** check for locked out floppy *
         bge.w flockout                #branch if locked out
         jsr f_clock                 #wait two second for interrupt
 #        move.b xcmdcopy,d7            xcmdreg changed during interrupt  {ag}
         mov.b 2(%a6),%d7                                                 #{ag}
         
         bclr &1,(%a6)                  #test and reset "await intr" flag
         beq.w fierrts                 #jump over error if normal
         mov.w &8090,%d0               #\\\\\\\\ error 90,8 ////////
         bra.w ferrexit                #abort with error
         
flockout: mov.w &9090,%d0               #\\\\\\\\ error 90,9 ////////
         bra.w ferrexit

 #isrisrisrisrisrisrisr  interrupt entry point  isrisrisrisrisrisrisrisr

         global _fintrupt
_fintrupt: cmpi.b _ndrives,&255            #check for no device present {gll}
          beq.w  fintrend1                                          #{gll}
 #*        movem.l #<a0,d0>,-(sp)           need two work registers in isr
         mova.l f_area,%a0                                               #{ag}
         movq   &0,%d0                                                   #{ag}
         mov.b  mfdrive,%d0                                                #{ag}
         lsl     &2,%d0                                                   #{ag}
         lea     3(%a0,%d0),%a0           #address of FFLAGSx.b in f_area    {ag}
         
 #        move.b xcmdcopy,d0            get current xcmdreg setting       {ag}
         mov.b 2(%a0),%d0                                                 #{ag}

         bclr &5,%d0                    #clear local bit in xcmd reg
         mov.b %d0,xcmdreg             #send to xcmdreg
 #        move.b d0,xcmdcopy            update xcmdreg copy for rte       {ag}
         mov.b %d0,2(%a0)               #save in XCMDCOPYx.b in f_area     {ag}

         mov.b cmdreg,%d0              #reset interrupt bit from floppy
 #        bclr #1,fflags                test and reset "await intr" flag  {ag}
         bclr &1,(%a0)                  #expecting intr in FFLAGSx.b in f_area

 #        jeq fintrerr                jump to error routine if flag=0
 #        bset #2,fflags                reset clock to zero wait time     {ag}
         bset &2,(%a0)                  #interupt occured flag             {ag}

 #*fintrend movem.l (sp)+,#<a0,d0>           restore isr working register
fintrend1: rts                           #return from interrupt
         
 #fintrerr btst #0,fflags                not in mf driver when intred?   {ag}
 #fintrerr  btst #0,(a0)                                                   {ag}

 #        jeq fintrend                enable and exit if not massmem
 #        move.l #6090,(sp)             \\\\\\\\ error 90,6 ////////
 #        lea ferrexit,a0               abort with error
 #        move.l a0,10(sp)              overwrite return address for rte
 #        jra fintrend                branch and exit isr

 #**********************************************************************
 #                                                                     *
 #                               F-CLOCK                               *
 #                                                                     *
 #    This is the elasped time counter routine.  The time to be        *
 #  wasted away is passed in register d0 in milli-seconds.  The        *
 #  routine exits when that time expires.  The routine also exits      *
 #  before the elasped time expires if the intr occured flag is set. *
 #                                                                     *
 #      a6 - flag byte memory pointer                              r   *
 #      d0 - clock elasped time counter                           r/w  *
 #      d1 - one milli-second counter                             r/w  *
 #                                                                     *
 #**********************************************************************

f_clock:  mov.l &350,%d1                #initialize milli-counter
fmillsec: bclr &2,(%a6)                  #test and clear intr occured???
         bgt.w clkexit                 #exit if interrupt completed
         subq.l &1,%d1                  #decrement milli-counter
         bgt.w fmillsec                #exit loop if 1 millisecond
         subq.w &1,%d0                  #decrement elaspt time counter
         bgt.w f_clock                 #exit if time expired
clkexit:  rts

	 global _snooze
microsec_dly: 
         movm.l %d1/%a0-%a1,-(%sp)
	 mov.l %d0,-(%sp)
	 jsr _snooze
	 addq.l &4,%sp			#restore stack pointer
	 movm.l (%sp)+,%d1/%a0-%a1
         rts

	 global _mf_issig
issig_proc: 
         movm.l %d1/%a0-%a1,-(%sp)
	 jsr _mf_issig
	 movm.l (%sp)+,%d1/%a0-%a1
	 tst.l %d0			#check if signal sent?
	 bne.w ferr66_8			#Yes,
         rts

 #**********************************************************************
 #
 #                       Mini-floppy INITIALIZE
 #
 #    Parameter stack:
 #
 #              sp +  6  -  crt message line pointer
 #              sp +  4  -  interleave (one WORD long)
 #              sp --->  -  return address
 #
 #**********************************************************************

 # The following are offsets to reg. A6 after the LINK A6 is executed:

         set message,64                        #crt message pointer parameter
         set intleav,63                        #interleave BYTE parameter
         set sectable,-16                       #sector table
         set ffsectbl,-32                       #spare sector table (= ffs)
         set margnerr,-36                       #one long word, two word-bit-maps
         set lgclside,-37                       #logical side number
         set sparecnt,-38                       #number of spared tracks
 #**nmi_save equ -42                       saved keyboard timer pseudo vctr
 #**jmpjsrop equ -44                       saved pseudo vectors jmp/jsr

 #**nmi_jxx  equ 0xffffff9a                 jmp or jsr op location
 #**nmivectr equ 0xffffff9c                 keyboard timer pseudo vector adr

 #**jump_op  equ 0x4ef9                     hex code for a 68000 jmp op
 #**********************************************************************
 #
 #                             FLPYINIT
 #
 #    Minifloppy initialize routine.  There are four modules:  the
 #  outer module controls the track to track set-ups (track numbering,
 #  data content, gap length, and message display), and sparing of
 #  tracks.  The ftrkinit module does the actual writting of the
 #  info on the track/side of the media and the verifing of that data
 #  write.
 #    Two modules deal with the sector table: fsectble generates
 #  the table with the required interleave, and rotatsec shifts the
 #  sector format for each track/side.  Note that the interleave
 #  factor has no effect on the time required to perform the initiali-
 #  zation as the sector table is used during the verify phase also.
 #    Before the movem register save and after the movem register
 #  restore the registers are defined as shown in f-set-up.
 #
 #      a0 - current data pattern                                 r/w
 #      a1 - initialize flag byte pointer                         r/w
 #      a2 - floppy datareg pointer                               r/w
 #      a3 - floppy cmdreg pointer                                 r
 #      a4 - floppy xcmdreg/xstatus pointer                        r
 #      a6 - LINK space pointer                                   r/w
 #      d0 - general purpose temperary storage register - err #   r/w
 #      d1 - loop counter                                         r/w
 #      d2 - data to be written onto the media                    r/w
 #      d3 - logical track number                                 r/w
 #      d4 - logical side number                                  r/w
 #      d7 - copy of xcmdreg, and sector counter                  r/w
 #
 #  Bug history:
 #    {bug75} - overwriting trk 33/1    fixed 12/10/80    dfk
 #    {bug96} - add pre-comp code       fixed 12/16/80    dfk
 #
 #
 #     Modified by:      Hewlett-Packard Company
 #     Date:             Dec. 14, 1983
 #     Purpose:          Use timer when present
 #
 #**********************************************************************

fmessage: byte "INITIALIZE: TRACK   , SIDE  , SPARED    "
 #   
 # 	flpyminit(tracknumber, sectortable, 0, drivenumber);
 #
	global _flpyminit
_flpyminit: jsr save_regs_params	#save regs & copy params for call
	jsr flpyminita		 	#go adjust stack parameters
	bra.w flpyexit

flpyminita:
	mov.l 4(%sp),%d0			#get interleave number on stack
	mov.l 12(%sp),-(%sp)		#put crt address on stack
	mov.w %d0,-(%sp)			#put interleave on stack
	jsr flpyinit			#call pascal entry routine
	rts

flpyinit: cmpi.b _ndrives,&255            #Check for No Device Present    {gll}
         beq.w  fnot_present                                            #{gll}
         mov.l %a6,-(%sp)               #save a6 reg. for pascal rtn
         jsr f_set_up                  #set up registers
         jsr  drivechk
         jsr fmotoron                  #turn on floppy motor
         jsr freset                    #restore drive to ground zero
         btst &6,cmdreg                  #is media write protected?
         beq.w flockous                #branch if not protected
         mov.w &2083,%d0               #\\\\\\\\ error 83,2 ////////
         bra ferrexit                  #abort with error
flockous: mov %sr,%d1                    #copy current interrupt level
         ori &0x0700,%sr                 #disable interrupts >>>>>>>>>>>>>
         mov.b &0x0A,cmdreg              #force head load with a restore
         jsr   fwaitinr                #wait for op complete
         mov.w &400,%d0                #initialize clock for 400 ms
         jsr f_clock                   #be sure head loaded, up to speed
         mov %sr,-(%sp)                 #save current sr status
 #xxx         ori #0x0700,sr                 disable interrupts >>>>>>>>>>>>>
         movm.l %d1-%d7/%a0-%a4,-(%sp)     #save f_set_up enviroment
         link %a6,&-44                  #sector table and work storage
         jsr fsectble                  #create sector table (interlaeve)
         movq &0,%d0                   #clear registers for format wrt
         movq &0,%d1
         movq &0,%d2
         movq &0,%d3
         movq &0,%d4

 #        move.b d7,xcmdcopy            save xcmdreg copy                 {ag}
         mova.l f_area,%a0                                               #{ag}
         mov.b  mfdrive,%d1                                                #{ag}
         lsl     &2,%d1                                                   #{ag}
         mov.b  %d7,5(%a0,%d1)                                             #{ag}
         movq   &0,%d1                                                   #{ag}

         movq &0,%d7                   #clear register for format wrt
         clr.b (%a1)                    #clear initialize flag byte
         clr.l margnerr(%a6)            #clear margin error bit maps
         clr.w sparecnt(%a6)            #clear spare count and lgclside
	 tst.l message(%a6)	       #check if there is one
	 beq.w messkp1		       #no, skip the message
         ori &0x0700,%sr                 #disable interrupts >>>>>>>>>>>>>
         mova.l message(%a6),%a0        #get message crt display location
         lea fmessage+40,%a2            #get message location (last byte)
         mov.b &78,%d1                 #40 bytes, 0 thru 78 odd addrs
fnxtchar: mov.b -(%a2),1(%a0,%d1)         #display one byte
         subq.b &2,%d1                  #decrement display index/counter
         bge.b fnxtchar                #loop until all message displayed
messkp1:  lea datareg,%a2                #for speed, a2 = pointer

fintside: 
	 jsr issig_proc	       #check if signal sent to process?
	 mov.l &15000,%d0              #microsecond value
         jsr microsec_dly             #delay (15 ms)
         jsr fdispcyl                  #display cylinder and spares
         mov.b &75,%d2                 #post index gap length = 75
         mova.w &0x5B,%a0               #data pattern to be written
         jsr ftrkinit                  #initialize the track
         btst &1,(%a1)                  #abort type error?
         bne ferr66_3                  #branch if error
         btst &0,(%a1)                  #test write trk error flag
         bne fspartrk                  #branch spare trk if error
         mov.b &95,%d2                 #post index gap length = 95
         mova.w &0x25,%a0               #data pattern to be written
         jsr ftrkinit                  #initialize the track
         btst &1,(%a1)                  #abort type error?
         bne ferr66_3                  #branch if error
         btst &0,(%a1)                  #test write trk error flag
         bne fspartrk                  #branch to spare trk if error
         movq &85,%d2                  #post index gap length = 85
         mova.w &0x55,%a0               #data pattern to be written
         jsr ftrkinit                  #initialize the track
         btst &1,(%a1)                  #abort type error?
         bne ferr66_3                  #branch if error
         btst &0,(%a1)                  #test write trk error flag
         bne fspartrk                  #branch to spare trk if error
         movq &85,%d2                  #post index gap length = 85
         mova.w &0x20,%a0               #data pattern to be written
         jsr ftrkinit                  #initialize the track
         btst &1,(%a1)                  #abort type error?
         bne ferr66_3                  #branch if error
         btst &0,(%a1)                  #test write trk error flag
         bne fspartrk                  #branch to spare trk if error
         clr.l margnerr(%a6)            #clear mrgn err bit (sector) maps
flogaddr: bchg &0,lgclside(%a6)          #side bit copy for displaying
         bchg &0,%d4                    #test and not logical side bit
         beq.w fnxtside                #track stays same if side was 0
         addq.b &1,%d3                  #increment track number by one
         btst &4,(%a1)                  #test lasttrks flag
         bne.b fnxtside                #branch if set
         cmpi.b %d3,&33                 #tracks 0 thru 32, then lasttrks
 #        bge.s finitend                branch to end if done            {bug75}
         blt.b fnxtside                #branch if not last user trk/side {bug75}
         bset &4,(%a1)                  #set lasttrks flag              {bug75}
         movq &4,%d0                   #max of four spared tracks        {bug75}
         sub.b sparecnt(%a6),%d0         #subtract actual spared tracks    {bug75}
         mov.b %d0,margnerr(%a6)        #store diff in margin bits area   {bug75}
fnxtside: jsr rotatsec                  #rotate the sector table
fphyaddr: mov.l &1000,%d0               #push microsecond value
         jsr microsec_dly             #delay (1 ms)
 #        move.b xcmdcopy,d7            get xcmdreg copy                {ag}
fwrtstl1: mova.l f_area,%a0                                             #{ag}
         movq   &0,%d7                                                 #{ag}
         mov.b  mfdrive,%d7                                              #{ag}
         lsl     &2,%d7                                                 #{ag}
         lea     5(%a0,%d7),%a0                                           #{ag}
         mov.b  (%a0),%d7                                               #{ag}

         bchg &2,%d7                    #test and compliment side bit
         beq.b fphyside                #track stays same if side was 0
 # seek to next track at interrupt level 7
         jsr f_tof_on                  #set up keyboard timeout
         mov.b &0x58,cmdreg            #step in, update, no verify
	 movq &2,%d1
fisrlop1: movq &-1,%d5
fisrlop0: mov.b xcmdreg,%d0                #get xstatue register
         btst &3,%d0                    #is interrupt bit set?
         dbne %d5,fisrlop0                #wait for interrupt
         dbne %d1,fisrlop1                #wait a long time
         beq.w ferr66_4                  #too long for interrupt, error 66,4
         mov.b cmdreg,%d0                #reset interrupt from floppy
         jsr f_to_off                  #kill keyboard timeout

         cmpi.b %d3,&18                 #are we on track #18 side 0 ?    {bug96}
         bne.b fphyside                #branch if not on track #18/0    {bug96}
         bset &1,%d7                    #set pre-comp bit in xcmdreg     {bug96}
fphyside: mov.b %d7,xcmdreg                #write side bit to floppy
 #        move.b d7,xcmdcopy            save xcmdreg copy               {ag}
         mov.b %d7,(%a0)                                                #{ag}

         btst &4,(%a1)                  #test lasttrks flag
         bne.b flastrks                #branch if set
         bra fintside                  #initialize next track/side

 #finitend bset #4,(a1)                  set lasttrks flag            {bug75}
 #         moveq #4,d0                   max of four spared tracks      {bug75}
 #         sub.b sparecnt(a6),d0         subtract actual spared tracks  {bug75}
 #         move.b d0,margnerr(a6)        store diff in margin bits area {bug75}
flastrks: subq.b &1,margnerr(%a6)        #decrement last tracks count
 #        ble.s fintunlk                branch if all tracks written    {bug75}
         blt.b fintunlk                #branch if all tracks written    {bug75}
         movq &85,%d2                  #post index gap length = 85
         mova.w &0x20,%a0               #data pattern to be written
         jsr ftrkinit                  #initialize the track
         bra   flogaddr                #branch and write next track

fintunlk:
         movq &0,%d0
         mov.b sparecnt(%a6),%d0        #get number of spares
	 mov.w %d0,-2(%a5) 	       #return number of spares
	 unlk %a6                       #return temp storage to stack
         movm.l (%sp)+,%d1-%d7/%a0-%a4     #restore registers
         clr.b (%a6)                    #clear flags bits
         mov (%sp)+,%sr                 #enable interrupts <<<<<<<<<<<<<<
         bclr &0,%d7                    #reset motor on bit
         jsr fresetqk                  #reset drive for fresh start
         mova.l (%sp)+,%a6              #restore a6 for pascal rtn
         mova.l (%sp)+,%a0              #get return pointer
 #**bug (dlb)        addq.l #2,sp                  pop parameter
         addq.l &6,%sp                  #pop parameters
         jmp (%a0)

 #******* end of outer loop ********************************************

fspartrk: addq.b &1,sparecnt(%a6)        #add one to spare tracks counter
         cmpi.b sparecnt(%a6),&4        #is there more than four spares?
         bgt.b ferr66_2                #abort initialize with error
         tst.b %d3                      #is this track zero?
         bgt.b foktospr                #if not track 0 then ok to spare
         btst &0,%d4                    #is this side zero?
         beq.b ferr66_1                #if side 0 then abort init
foktospr: movm.l %d3-%d4,-(%sp)           #stack logical track side
         mova.w &0xff,%a0               #data pattern for spare track
         movq &85,%d2                  #post index gap lenght =85
         mov.b &0xff,%d3                #spare track # = ff
         mov.b &0xff,%d4                #spare side # = ff
         jsr fswaptbl                #save sectbl and replace with fs
         jsr ftrkinit                  #spare (write) the track
         jsr fswaptbl                #restore sectbl and the fs
         movm.l (%sp)+,%d3-%d4           #restore trk and side
         bra fphyaddr                  #try next track/side

ferr66_1: 
         jsr fdispcyl                #display final spare track count
         mov.w &1066,%d0               #\\\\\\\\ error 66,1 ////////
         bra.w finiterr
ferr66_2:
         jsr fdispcyl                #display final spare track count
         mov.w &2066,%d0               #\\\\\\\\ error 66,2 ////////
         bra.w finiterr
ferr66_3: 
         jsr fdispcyl                #display final spare track count
         mov.w &3066,%d0               #\\\\\\\\ error 66,3 ////////
         bra.w finiterr

ferr66_4:
         jsr f_to_off                  #kill keyboard timeout
         mov.w &4066,%d0               #\\\\\\\\ error 66,4 ////////
         bra.w finiterr
ferr66_5:
         jsr f_to_off                  #kill keyboard timeout
         mov.w &5066,%d0               #\\\\\\\\ error 66,5 ////////
         bra.w finiterr
ferr66_6:
         jsr f_to_off                  #kill keyboard timeout
         mov.w &6066,%d0               #\\\\\\\\ error 66,6 ////////
         bra.w finiterr
ferr66_7:
         jsr f_to_off                  #kill keyboard timeout
         mov.w &7066,%d0               #\\\\\\\\ error 66,7 ////////
         bra.w finiterr
ferr66_8:
         mov.w &8066,%d0               #\\\\\\\\ error 66,8 ////////
         bra.w finiterr
ferr90_7:
         jsr f_to_off                  #kill keyboard timeout
         mov.w &7090,%d0               #\\\\\\\\ error 90,7 ////////
finiterr: unlk %a6                       #return temp storage to stack
         movm.l (%sp)+,%d1-%d7/%a0-%a4     #restore registers
         clr.b (%a6)                    #clear fflags
         mov (%sp)+,%sr                 #enable interrupts <<<<<<<<<<<<<<
         bra ferrexit                  #abort initialization

fswaptbl: movq &15,%d1                  #table index/counter
fswploop: mov.b sectable(%a6,%d1),%d0     #temp store one table entry
         mov.b ffsectbl(%a6,%d1),sectable(%a6,%d1) #move one table entry
         mov.b %d0,ffsectbl(%a6,%d1)     #one entry now switched
         subq.b &1,%d1                  #decrement index/counter
         bge.b fswploop                #branch until all switched
         rts

fdispcyl: 
	 tst.l message(%a6)	       #check if there is one
	 beq.w fdispcy1		       #no, skip the message
	 mova.l message(%a6),%a0        #initialize display address
         adda   &37,%a0                 #start of track field
         mov.l %d3,%d0                  #copy track number
         divu &10,%d0                   #convert to decimal number
         jsr fdispchr                #display msd of track number
         swap %d0                       #get next half of track number
         jsr fdispchr                #display lsd of track number
         adda   &14,%a0                 #start of side field
         mov.b lgclside(%a6),%d0        #get side number
         jsr fdispchr                #display side number
         adda   &18,%a0                 #start of spares field
         mov.b sparecnt(%a6),%d0        #get number of spares
         jsr fdispchr                #display number of spares
fdispcy1: rts

fdispchr: ori.b &0x30,%d0                 #convert number to ascii
         andi.b &0x3f,%d0                #insure valid number
         mov.b %d0,(%a0)                #send number to crt
         addq.l &2,%a0                  #increment crt address
         rts

 #**********************************************************************
 #
 #                              ftrkinit
 #
 #    One entire track is written to comply with the HP LIF standard,
 #  and then read (verify) up to three times per track.  The first
 #  verify pass to complete error free terminates the verification
 #  phase for that track.  If a margin error occures, the sector it
 #  occured in has a corresponding bit set in a bit map.  Three margin
 #  errors must occure in the same sector for all three verification
 #  passes before that track is spared.  The first side of the first
 #  track cannot be spared.  A not ready, write protect, or lost
 #  data error aborts the initialize while all other errors will
 #  cause the track to be spared.  All tracks out to physical trk/side
 #  35/1 are initialized, but tracks beyond logical trk/side 32/1 are
 #  not verified.
 #    The hardware keyboard timeout mechanism is used while interrupts
 #  are locked out to detect catastrophic errors (door open).
 #
 #  Initialize flag byte:
 #                                 a1@
 #  -----------------------------------------------------------------
 #  |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
 #  -----------------------------------------------------------------
 #
 #            Bit #                Definition
 #              0       Spare flag - spare the track
 #              1       Error abort flag - abort initialize
 #              2       Verify pass 2 - second verify pass
 #              3       Verify pass 3 - third verify pass
 #              4       Lasttrks - initing tracks beyond logical 32/1
 #              5       Write error retry - one allowed
 #              6       Verify retry 1 - first verify retry
 #              7       Verify retry 2 - second retry
 #
 #  Bug history:
 #    {bug95} - false lost data errors    fixed 12/16/80    dfk
 #
 #  Modified:    to work w/ new keyboard scheme
 #               These modifications work with the INTERNAL KEYBD ONLY.
 #               I assumed that, since these routines do not check for
 #               a missing keyboard, that they must require the presence
 #               of an internal kbd anyway.  These mods were NOT TESTED
 #               as the internal minifloppies are not supported on Bobcat.
 #               These fixes were stuck in incase anybody ever tries to
 #               use this boot ROM on an old UMM board or something.
 #
 #**********************************************************************

ftrkinit: andi.b &0x10,(%a1)              #clear all but lasttrks flag
fwrttrk:  
         jsr f_tof_on                  #set up keyboard timeout
         movq &15,%d7                  #initialize to 16 sectors (0 ref)
         mov.b %d2,%d1                  #count = (75,95,85)
         movq &0x4e,%d2                 #data = 4e
         mov.b &0xf4,cmdreg              #issue write track (format)
         jsr finitwrt                #post index gap
fnextsec: movq &16,%d1                  #count = 16
         movq &0x4e,%d2                 #data = 4e
         jsr finitwrt                #gap 1
         movq &12,%d1                  #count = 12
         clr.b %d2                      #data = 00
         jsr finitwrt                #preamble
         movq &3,%d1                   #count = 3nd of preamble
         short 0x74f5                    #data = f5
         jsr finitwrt                #end of preamble
         movq &1,%d1                   #count = 1
         short 0x74fe                    #data = fe
         jsr finitwrt                #id address mark
         movq &1,%d1                   #count = 1
         mov.b %d3,%d2                  #data = logical track number
         jsr finitwrt                #track number
         movq &1,%d1                   #count = 1
         mov.b %d4,%d2                  #data = logical side number
         jsr finitwrt                #side number
         movq &1,%d1                   #count = 1
         mov.b sectable(%a6,%d7),%d2     #data = sector number (from tbl)
         jsr finitwrt                #sector number
         bra.w fwrtrecl                #(subr placed next for speed)

 #  d2 = data pattern, d1 = # of bytes to write, d5=timer

finitwrt: 
	 movq &7,%d6
finitlop0: movq &-1,%d5
finitlop1: mov.b xcmdreg,%d0                #get extended status register
         and.b &9,%d0                   #mask data-req and interrupt bits
         dbne %d5,finitlop1                 #stay in loop till bit 0 or 3
         dbne %d6,finitlop0                 #stay in loop till bit 0 or 3
	 beq.w ferr66_5                  #check if in loop too long?
         btst &0,%d0                    #is this a data request?
         beq.w ferr90_7                  #unexpected interrupt, return error 90,7
         mov.b %d2,(%a2)                #write data pattern to datareg
         subq.w &1,%d1                  #write out desired number bytes
         bgt.b finitlop1               #branch back till all bytes done
         rts

fwrtrecl: movq &1,%d1                   #count = 1
         mov.b %d1,%d2                  #data = 1
         jsr finitwrt                #record length
         movq &1,%d1                   #count = 1 (two bytes)
         short 0x74f7                    #data = f7
         jsr finitwrt                #crc
         movq &22,%d1                  #count = 22
         movq &0x4e,%d2                 #data = 4e
         jsr finitwrt                #gap 2
         movq &12,%d1                  #count = 12
         clr.b %d2                      #data = 00
         jsr finitwrt                #sync field
         movq &3,%d1                   #count = 3
         short 0x74f5                    #data = f5
         jsr finitwrt                #end of sync field
         movq &1,%d1                   #count = 1
         short 0x74fb                    #data = fb
         jsr finitwrt                #data address mark
         mov.w &256,%d1                #count = 256
         mov.w %a0,%d2                  #data = (5b,25,55)
         jsr finitwrt                #data pattern bytes
         movq &1,%d1                   #count = 1 (two bytes)
         short 0x74f7                    #data = f7
         jsr finitwrt                #crc
         movq &1,%d1                   #count = 1
         short 0x74ff                    #data = ff
         jsr finitwrt                #(for 1791-01 fcd chip)
         movq &28,%d1                  #count = 28
         movq &0x4e,%d2                 #data = 4e
         jsr finitwrt                #gap 3
         subq.b &1,%d7                  #16 sectors done
         bge fnextsec                  #loop until done
         movq &0x4e,%d2                 #write 4es until index pulse

	 movq &7,%d6
fwaitlop0: movq &-1,%d5
fwaitlop1: mov.b xcmdreg,%d0                #get extended status register
         and.b &9,%d0                   #mask data-req and interrupt bits
         dbne %d5,fwaitlop1                #repeat until bit 0 or 3
         dbne %d6,fwaitlop0                #repeat until bit 0 or 3
         beq.w ferr66_6                  #bit 0 or 3 ?
         btst &0,%d0                    #is this a data request?
         beq.b ftrkend                 #exit loop if interrupt (index)
         mov.b %d2,(%a2)                #write 4e to data register
         bra.w fwaitlop1                #continue 4es until interrupt

ftrkend:  mov.b cmdreg,%d0                #reset interrupt from floppy
         jsr f_to_off                  #kill keyboard timeout
         btst &4,(%a1)                  #test lasttrks flag bit
         bne   fverexit                #exit if set
         andi.b &0xe4,%d0                #test bits 2,5,6,7
         bne   fiwrterr                #branch if any error bits set
         bclr &0,(%a1)                  #is this a spare track operation?
         bne fverexit                  #branch if just sparing a track
 #        move.b  xcmdcopy,d7            get current xcmdreg setting       {ag}
         mov.l  %a0,-(%sp)                                                 #{ag}
         movq   &0,%d7                                                    #{ag}
         mov.b  mfdrive,%d7                                                 #{ag}
         lsl     &2,%d7                                                    #{ag}
         mova.l f_area,%a0                                                #{ag}
         mov.b  5(%a0,%d7),%d7                                              #{ag}
         mova.l (%sp)+,%a0                                                 #{ag}
fverify:  mov.b %d3,trkreg              #track register = current log trk
         mov.w &15,%d6                 #initialize to 16 sector read
fversec:  
         jsr f_tov_on                  #set up keyboard timeout
         mov.b sectable(%a6,%d6),secreg #desired sector to sector reg
         andi.b &0x2F,(%a1)              #reset verify retry flags
         ori.b &0x30,%d7                 #set local and read bits
         btst &0,%d4                    #which logical side?
         beq.b fverify0                #branch if side zero
         mov.b &0x8a,cmdreg              #issue read side one to cmdreg
         bra.w fverify1                #jump over side zero command
fverify0: mov.b &0x82,cmdreg              #issue read side zero to cmdreg
fverify1: mov.b %d7,xcmdreg                #send copy to xcmdreg

         movq &7,%d0			#number of times -1 thru loop
fverlop0: movq &-1,%d5			#65536 times thru inner loop
fverlop1: mov.b xcmdreg,%d1                #get extended status register
         btst &3,%d1                    #has interrupt occured?
         dbne %d5,fverlop1               #branch (loop) until interrupt
         dbne %d0,fverlop0
	 beq.w ferr66_7                  #check if in loop too long?

         bclr &5,%d7                    #turn off local bit in xcmd
         mov.b %d7,xcmdreg                #send to xcmdreg of floppy
         mov.b cmdreg,%d0                #reset interrupt from floppy
         jsr f_to_off                  #kill keyboard timeout
         btst &2,%d1                    #did a margin error occure?
         bne.b fmrgnerr                #branch if margin error occured
fchkerrs: mov.b cmdreg,%d0                #get status word from floppy     {bug95}
 #fchkerrs andi.b #0xfc,d0                any error bits 2 thru 7?       {bug95}
         andi.b &0xfc,%d0                #any error bits 2 thru 7?        {bug95}
         bne.b fvererr                 #branch if error
         subq.b &1,%d6                  #decrement sector counter
         bge.w fversec                 #verify next sector
         tst.w margnerr+2(%a6)          #margin error on this pass ???
         beq.w fverexit                #verify complete (bra) if no err
         bset &2,(%a1)                  #set and check verify pass 2 flag
         beq.w fverify                 #verify track again if not set
         bset &3,(%a1)                  #set and check verify pass 3 flag
         beq.w fverify                 #verify track again if not set
fverexit: rts

fiwrterr: andi.b &0x24,%d0                #status bits 2 or 5 ?
         beq.w fifailed                #abort initialization
         bset &5,(%a1)                  #set and test write retry flag
         beq fwrttrk                   #branch retry not yet attempted
         bra.w fifailed                #initialization abort

fvererr:  mov.b %d0,%d1                  #make a second working copy
         andi.b &0xC4,%d0                #not rdy, wrt protect, lost data?
         bne.w fifailed                #branch if error
         andi.b &0x38,%d1                #fault, rnf, or crc type error?
         bne.w fsparflg                #branch if error
         bset &6,(%a1)                  #has one retry been tried?
         beq.w fverify                   #branch and perform first retry
         bset &7,(%a1)                  #has two retrys been treid ?
         beq.w fverify                   #branch to perform second retry
fifailed: bset &1,(%a1)                  #set the error abort flag
         rts

fmrgnerr: mov.l margnerr(%a6),%d0        #get margin error map words
         bset %d6,%d0                    #test/set sector margin err flag1
         beq.w fvercont                #continue verify if not set
         swap %d0                       #get second margin error map word
         bset %d6,%d0                    #test/set sector margin err flag2
         beq.w fvercont                #continue verify if not set
fsparflg: bset &0,(%a1)                  #set the spare track flag
         rts

fvercont: mov.l %d0,margnerr(%a6)        #save (no need to swap back)
         bra.w fchkerrs                #continue, check for other errors

f_tof_on: 
 #**         dc.w 0x70d4   (moveq #0xd4,d0)  need 440ms timeout for trk wrt
         bra.w f_tmo_on
f_tov_on: 
 #**         moveq   #0x7c,d0               need 1320ms timeout for verify
f_tmo_on:
         movq   &-1,%d5
 #**         move.w kbd_fastv,jmpjsrop(a6)   save jmp or jsr op for nmi vctr .ag.
 #**         move.l kbd_fastv+2,nmi_save(a6)  save current nmi ptr value     .ag.
 #**         move.w #jump_op,kbd_fastv     move jump op code for nmi vctr    .ag.
 #**         pea ferr66_4                  floppy error routine to stack     .ag.
 #**         move.l (sp)+,kbd_fastv+2      error routine pointer to nmi
 #**         jbsr fwaitkbd                wait for keyboard ready
 #**         move.b #0xb2,int_kbdcontrol    keyboard attention command       {acr}
 #**         jbsr fwaitkbd                wait for keyboard ready
 #**         move.b d0,int_kbddata         least signf. byte, centisecs    {acr}
 #**         jbsr fwaitkbd                           (twos compliment)
 #**         move.b #0xff,int_kbddata       msb, timeout = 0                {acr}
         mov %sr,spsave                 #save current sr status
         ori &0x0700,%sr                 #disable interrupts >>>>>>>>>>>>>
         rts

 #**fwaitkbd btst #1,int_kbdcontrol        is keyboard ready ?              {acr}
 #**         bne.s fwaitkbd                loop until ready
 #**         rts
	data
spsave:	short 0
	text

f_to_off: 
 #**	    jbsr fwaitkbd                wait for keyboard ready
 #**         move.b #0xb2,int_kbdcontrol    reset interrupt                  {acr}

 #         move.w #100,d0                                                .ag.
 #f_towait subq.w #1,d0                  delay for keyboard to respond   .ag.
 #         bgt.s f_towait                                                .ag.

 #**f_towait  btst  #2,int_hpibstatus                                       .ag.
 #**          bne.s f_towait                                                .ag.
 #**
 #**         move.w jmpjsrop(a6),kbd_fastv restore saved jmp or jsr op      .ag.
 #**         move.l nmi_save(a6),kbd_fastv+2 restore saved nmi vector ptr   .ag.
         mov spsave,%sr                 #enable interrupts <<<<<<<<<<<<<<
         rts

 #**********************************************************************
 #
 #                             fsectble
 #
 #    Take the interlaeve factor (obtained from the stack) and gener-
 #  ate a table in memory which contains the correct sequence for
 #  sector addresses per track (one byte per sector).  Also build
 #  another equally sized table containing all 0xFFs.  Note that the
 #  table is actually built backwards in memory, i.e. the last sector
 #  number is the first entry in the table.  While being built, a one
 #  word bit map is used to maintain a space available record for
 #  the sector table.
 #
 #      a6 - LINK space pointer                                   r
 #      d0 - interleave factor obtained from stack               r/w
 #      d1 - loop counter                                        r/w
 #      d2 - index into table                                    r/w
 #      d3 - sector bit map                                      r/w
 #
 #**********************************************************************

fsectble: clr.l %d3                      #clear sector map register
         movq &15,%d2                  #initialize index to sector table
         clr.l %d1                      #clear loop and sector counter
         clr.l %d0                      #clear interleave register
         mov.b intleav(%a6),%d0         #get interleave from stack
         divu &16,%d0                   #convert interleave to 0 - 15
         swap %d0                       #get remainder (mod 16)
 #        move.b d0,intleav(a6)         store for use in rotate table   {ag}
secloop:  bset %d2,%d3                    #set mapped sector in map reg
         mov.b %d1,sectable(%a6,%d2)     #sector to mapped table location
         mov.b &0xff,ffsectbl(%a6,%d2)   #fill one entry in spare table
         addq.b &1,%d1                  #increment counter
         cmpi.b %d1,&15                 #have all 16 sectors been done?
         ble.b   nextsec               #next sector if not done
         addq.b  &3,%d0                                                 #{ag}
         sub.b   %d2,%d0                                                 #{ag}
         mov.b  %d0,intleav(%a6)                                        #{ag}
         rts

nextsec:  sub.b %d0,%d2                   #decrement by interleave amount
         bge.b sectest                 #test for mapping if no wrap
         addi.b &16,%d2                 #wrap around zero
sectest:  btst %d2,%d3                    #is mapped location empty?
         beq.b secloop                 #branch if empty
         subq.b &1,%d2                  #check next location for empty
         bra.w sectest                 #does index point to empty entry

 #**********************************************************************
 #
 #                        Rotate sector table
 #
 #    The sector table is rotated by three plus the interleave factor
 #  obtained from the stack.  The algorithm used is very simple:  the
 #  entire table is rotated one position per pass; the number of
 #  passes determines the total positions rotated.
 #
 #      a6 - LINK space pointer
 #      d0 - loop couunter, intialily set to 15
 #      d1 - sector table offset
 #      d2 - temperary storage for sector table entry
 #      d3 - temperary storage for sector table entry
 #      d4 - interleave factor   number of times to rotate
 #
 #**********************************************************************

rotatsec: movm.l %d0-%d4,-(%sp)           #stack, need some work regs
         mov.b intleav(%a6),%d4         #get interleave factor from stack
 #        addq.b #3,d4                  stagger rotate by three          {ag}
         mov.b sectable+15(%a6),%d2     #get first entry in sector table
rintrlev: movq &15,%d0                  #16 entries in sector tbl (0 rel)
         mov.l %d0,%d1                  #index for destination(move.long)
rotaloop: subq.b &1,%d1                  #destination = source index - 1
         mov.b sectable(%a6,%d1),%d3     #save destination entry
         mov.b %d2,sectable(%a6,%d1)     #store source
         mov.b %d3,%d2                  #destination becomes new source
         subq.b &1,%d0                  #decrement loop counter
         bgt.b rotaloop
         mov.b %d2,sectable+15(%a6)     #store last entry in 1st location
         subq.b &1,%d4                       #repeat = interleave factor
         bgt.b rintrlev
         movm.l (%sp)+,%d0-%d4           #pop saved work registers
         rts
