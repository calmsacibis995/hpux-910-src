# @(#) $Revision: 66.4 $       
#
ifdef(`_NAMESPACE_CLEAN',`
        global  __pbcdrel
        sglobal _pbcdrel',`
        global  _pbcdrel
 ')
		
      	set	SIGFPE,8

#***************************************************************************
#
#  The following is the compiler interface to the decimal -> real
#  conversion routine (bcdrel).
#
ifdef(`_NAMESPACE_CLEAN',`
__pbcdrel:
')
_pbcdrel: 
ifdef(`PIC',`
	link	%a6,&0
#	trap	#2
#	dc.w	-32
	movem.l	%d2-%d7/%a2,-(%sp)    
#
        mov.l   &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a2
	ifdef(`PROFILE',`
	mov.l   p_pbcdrel(%a2),%a0
	bsr.l   mcount
	')
#
	move.l	8(%a6),%a0
	bsr	bcdrel
	movem.l	(%sp)+,%d2-%d7/%a2',`
	ifdef(`PROFILE',`
	mov.l	&p_pbcdrel,%a0
	jsr	mcount
	')
	link	%a6,&0
#	trap	#2
#	dc.w	-32
        movem.l %d2-%d7,-(%sp)
	mov.l	8(%a6),%a0
	bsr	bcdrel
	movem.l	(%sp)+,%d2-%d7
')
	unlk	%a6
	rts	

#******************************************************************************
#
#       Procedure  : bcdrel
#
#       Description: Convert a bcd number into a real number.
#
#       Author     : Paul Beiser / Ted Warren
#
#       Revisions  : 1.0  06/01/81
#
#       Parameters : a0         - address of the bcd number
#
#       Registers  : a1         - address of tables
#                    d2-d7      - scratch
#                    
#       Result     : The result is returned in (d0,d1).
#
#       Error(s)   : Decimal strings too large return an error.
#
#       References : rmul, err_impvalue
#                    tb_pwt, tb_pwt4, tb_pwt8, tb_auxpt, tb_bcd
#
#       Misc       : A decimal string too small returns a result of 0.
#
#******************************************************************************
 
#
#  Only eight digits to convert so do it fast.
#
bcd8:     mulu    &10000,%d0
         move.b  (%a0)+,%d7       #fetch third pair
         move.b  0(%a1,%d7.w),%d7  #lookup binary value
         mulu    &100,%d7        #multiply by 100
         add.l   %d7,%d0          #and add to sum
         moveq   &0,%d7
         move.b  (%a0)+,%d7       #fourth pair
         move.b  0(%a1,%d7.w),%d7  #lookup binary value
         add.l   %d7,%d0          #last add for fraction
         addq.l  &4,%a0          #point at bcd exponent
         moveq   &0,%d1          #shift result right 6 places
         move.w  %d0,%d1          #across d0,d1 pair
         lsr.l   &6,%d0
         ror.l   &6,%d1
         clr.w   %d1
         move.l  %d0,%d6          #form index for normalizing
         swap	 %d6
         and.w   &0x1e,%d6        #look at bits 20, 19, 18, and 17
ifdef(`PIC',`
	 mov.l   pn_tb_4(%a2),%a1',`
	 lea  	 pn_tb_4,%a1
')
	 move.w	 0(%a1,%d6.w),%d6 #lookup shift value
         move.w  &1023+26-1,%d7  #exponent value if normalized
         sub.w   %d6,%d7          #subtract # of shifts required
         neg.w   %d6             #computed goto for normalizing
         addq.w  &4,%d6
         asl.w   &2,%d6
ifdef(`PIC',`
	 mov.l	 shiftr8(%a2),%a1',`
	 lea  	 shiftr8,%a1
')
         jmp     0(%a1,%d6.w)
shiftr8:  add.l   %d1,%d1
         addx.l  %d0,%d0          #entry for shift by 4
         add.l   %d1,%d1
         addx.l  %d0,%d0          #entry for shift by 3
         add.l   %d1,%d1
         addx.l  %d0,%d0          #entry for shift by 2
         add.l   %d1,%d1
         addx.l  %d0,%d0          #entry for shift by 1
         asl.w   &4,%d7          #shift exponent into position
         swap    %d7
         add.l   %d7,%d0          #add to fraction, removing hidden 1
ifdef(`PIC',`
         mov.l   tb_pwt8(%a2),%a1  #address of table used for 8 digit convert',`
	 lea  	 tb_pwt8,%a1	#address of table used for 8 digit convert
')
         bra.w     fractsgn       #determine sign and finish conversion
#
#  Table for number of normalization shifts versus value.
#  It must be in this location for short mode addressing.
#
	  data
pn_tb_4:  short    4,3,2,2,1,1,1,1,0,0,0,0,0,0,0,0
	  text

#******************************************************************************
#
#  Only four digits (8 at most) to convert so do it extremely fast.
#
bcd4:     clr.w   %d0
         move.b  (%a0)+,%d0       #get first two digits
         move.b  0(%a1,%d0.w),%d0  #lookup binary value
         mulu    &100,%d0        #weight by 100
         move.b  (%a0)+,%d7       #get second two digits
         move.b  0(%a1,%d7.w),%d7  #lookup binary value
         add.w   %d7,%d0
         tst.w   (%a0)           #four more digits?
         bne.w     bcd8           #branch only if 4 more digits
            addq.l  &6,%a0          #point at exponent
            moveq   &0,%d1          #if four digits then low order real =0
            asl.l   &7,%d0          #shift by at least 7 to post normalize
            move.l  %d0,%d6          #form an index
            swap    %d6             #for post normalization
            and.w   &0x1e,%d6        #look at bits 20,19,18, and 17
ifdef(`PIC',`
	    mov.l   pn_tb_4(%a2),%a1',`
	    lea     pn_tb_4,%a1
')
            move.w  0(%a1,%d6.w),%d6 #lookup shift value
            asl.l   %d6,%d0          #normalize real
            move.w  &1023+13-1,%d7  #form exponent
            sub.w   %d6,%d7          #subtract amount normalized
            asl.w   &4,%d7          #align into position
            swap    %d7
            add.l   %d7,%d0          #merge into fraction
ifdef(`PIC',`
            mov.l   tb_pwt4(%a2),%a1   #address of table for 4 digit convert',`
            lea     tb_pwt4,%a1   #address of table for 4 digit convert
')
            bra.w     fractsgn
         
#******************************************************************************
#
#  BCD to real conversion begins here.
#
bcdrel:
#	trap	#2
#	dc.w	-8
	addq.l  &2,%a0          #skip over sign
#
#  Convert first eight bcd digits to binary and store in d2.
#
         tst.b   (%a0)           #check for zero (remember, must be normalized!)
         bne.w     continue       #continue if non-zero
under1:      moveq   &0,%d0          #else return a value of 0
            move.l  %d0,%d1
            rts
ifdef(`PIC',`
continue: mov.l  tb_bcd(%a2),%a1    #address of 2 digit bcd to binary table',`
continue: lea    tb_bcd,%a1    #address of 2 digit bcd to binary table
')
         moveq   &0,%d3
         moveq   &0,%d7
         moveq   &0,%d2
         tst.l   4(%a0)          #check for 8 or less digits
         beq.w     bcd4
         move.b  (%a0)+,%d2       #fetch first bcd digit pair
         move.b  0(%a1,%d2.w),%d2  #lookup its binary value
         mulu    &62500,%d2      #multiply by 1,000,000
         asl.l   &4,%d2          #(62,500*16)
         move.b  (%a0)+,%d7       #fetch second pair
         move.b  0(%a1,%d7.w),%d7  #lookup binary value
         mulu    &10000,%d7      #multply by 10,000
         add.l   %d7,%d2          #and add to sum
         moveq   &0,%d7
         move.b  (%a0)+,%d7       #fetch third pair
         move.b  0(%a1,%d7.w),%d7  #lookup binary value
         mulu    &100,%d7        #multiply by 100
         add.l   %d7,%d2          #and add to sum
         moveq   &0,%d7
         move.b  (%a0)+,%d7       #fetch fourth pair
         move.b  0(%a1,%d7.w),%d7  #lookup binary value
         add.l   %d7,%d2          #add to sum
#
#  Convert bottom eight bcd digits and store in d3.
#
         move.b  (%a0)+,%d3       #fetch fifth bcd digit pair
         move.b  0(%a1,%d3.w),%d3  #lookup its binary value
         mulu    &62500,%d3      #multiply by 1,000,000
         asl.l   &4,%d3          #(62,500*16)
         move.b  (%a0)+,%d7       #fetch sixth pair
         move.b  0(%a1,%d7.w),%d7  #lookup binary value
         mulu    &10000,%d7      #multply by 10,000
         add.l   %d7,%d3          #and add to sum
         moveq   &0,%d7
         move.b  (%a0)+,%d7       #fetch seventh pair
         move.b  0(%a1,%d7.w),%d7  #lookup binary value
         mulu    &100,%d7        #multiply by 100
         add.l   %d7,%d3          #and add to sum
         moveq   &0,%d7
         move.b  (%a0)+,%d7       #fetch eighth pair
         move.b  0(%a1,%d7.w),%d7  #lookup binary value
         add.l   %d7,%d3          #add to sum
#
#  Multiply high order part by 1,000,000 and add low order part
#  1,000,000=0x5f5e100. Result=(((hi * 5f5e) * 0x1000) + (hi * 0x100)) + lo.
#
         moveq   &0,%d4
         move.w  %d2,%d1
         mulu    &0x5f5e,%d1      #hi.word(lower) * 5f5e
         move.l  %d2,%d0
         swap    %d0
         mulu    &0x5f5e,%d0      #hi.word(upper) * 5f5e
         swap    %d1
         move.w  %d1,%d4
         clr.w   %d1
         add.l   %d4,%d0
         move.w  %d0,%d4
         lsr.l   &4,%d0          #multiply by 0x1000 by shifting
         lsr.l   &4,%d1
         ror.l   &4,%d4
         clr.w   %d4
         or.l    %d4,%d1
         move.l  %d2,%d4
         clr.w   %d4
         swap    %d4
         lsr.w   &8,%d4          #multiply hi by 0x100 by shifting
         lsl.l   &8,%d2
         add.l   %d2,%d1          #add to previous result
         addx.l  %d4,%d0
         add.l   %d3,%d1          #add in conversion from lower 8 digits
         bcc.w     bcdr_nz
            addq.l  &1,%d0
#
#  Use jump table for post normalization and exponent location.
#
bcdr_nz:  move.l  %d0,%d6
         swap    %d6             #get upper 16 bits of fraction
         and.w   &0x3e,%d6        #mask off all but top 5 bits (17-21)
ifdef(`PIC',`
	 mov.l	 eval_exp(%a2),%a1',`
	 lea  	 eval_exp,%a1
')
         move.w  (%a1,%d6.w),%d7 #look up exponent
ifdef(`PIC',`
	 mov.l	 pn_table(%a2),%a1',`
	 lea  	 pn_table,%a1
')
         jmp     (%a1,%d6.w)
#
#  Exponent value table for converted bcd integer.
#  1023 (bias) + 52 (size of integer) - #postnorm shifts
#  -1 (gets rid of hidden one) all times 16 to bit align.
#
	 data
eval_exp: short    17120       
         short    17136       
         short    17152
         short    17152
         short    17168
         short    17168
         short    17168
         short    17168
         short    17184,17184,17184,17184
         short    17184,17184,17184,17184
         short    17200,17200,17200,17200
         short    17200,17200,17200,17200
         short    17200,17200,17200,17200
         short    17200,17200,17200,17200
	 text
pn_table: bra.b     pn_4
         bra.b     pn_3
         bra.b     pn_2
         bra.b     pn_2
         bra.b     pn_1
         bra.b     pn_1
         bra.b     pn_1
         bra.b     pn_1
         bra.b     pn_0
         bra.b     pn_0
         bra.b     pn_0
         bra.b     pn_0
         bra.b     pn_0
         bra.b     pn_0
         bra.b     pn_0
         bra.b     pn_0
         bra.b     pn_m1
         bra.b     pn_m1
         bra.b     pn_m1
         bra.b     pn_m1
         bra.b     pn_m1
         bra.b     pn_m1
         bra.b     pn_m1
         bra.b     pn_m1
         bra.b     pn_m1
         bra.b     pn_m1
         bra.b     pn_m1
         bra.b     pn_m1
         bra.b     pn_m1
         bra.b     pn_m1
         bra.b     pn_m1
         nop                    #must be there; can't branch to next instruction!
#
pn_m1:    lsr.l   &1,%d0          #16 digit bcd number was too large
         roxr.l  &1,%d1          #and so overflowed requiring a shift
         bra.w     pn_done        #to the right and dumping of one bit
#
pn_4:     add.l   %d1,%d1           
         addx.l  %d0,%d0
pn_3:     add.l   %d1,%d1
         addx.l  %d0,%d0
pn_2:     add.l   %d1,%d1
         addx.l  %d0,%d0
pn_1:     add.l   %d1,%d1
         addx.l  %d0,%d0
pn_0:
pn_done:  swap    %d7             #insert exponent
         add.l   %d7,%d0          #automatically removes hidden one
ifdef(`PIC',`
         mov.l   tb_pwt(%a2),%a1      #address of primary powers of ten table',`
         lea     tb_pwt,%a1      #address of primary powers of ten table
')
#
#  Check sign of bcd number.
#
fractsgn: tst.w   -10(%a0)        #test bcd sign    
         beq.w     firfl
            bset    &31,%d0         #set sign bit if negative
#
#  Fetch exponent, and test for proper range.
#
firfl:    move.w  (%a0),%d3        #get binary exponent
         cmp.w   %d3,&-309
         blt.w     under1         #number too small - return a 0
         cmp.w   %d3,&309
         bgt.w     err_impvalue   #number too large
#
#  Check for one or two multiplies.
#
         move.w  %d3,%d6
         add.w   &64,%d6         #bias to the positive
         bmi.w     bcdr_3         #E<-64?
         cmp.w   %d6,&128        #E>64?
         bgt.w     bcdr_3         #must do 2 multiplies, return here later
bcdr_4:      asl.w   &3,%d6          #convert logical to physical index
            move.l  0(%a1,%d6.w),%d2  #lookup values
            move.l  4(%a1,%d6.w),%d3
ifdef(`PIC',`
            bsr.l    rmul           #do the operation',`
            jsr      rmul           #do the operation
')
            rts
#
#  Exponent > abs(64).
#
bcdr_3:   move.w  %d3,-(%sp)       #save exponent for later
         asr.w   &6,%d3          #div 64
         bpl.w     divfix1        #this is Paul Beiser's patented DIV
            addq.w  &1,%d3
divfix1:  addq.w  &4,%d3          #bias to the positive
         asl.w   &3,%d3          #change logical to physical index
ifdef(`PIC',`
         mov.l   tb_auxpt(%a2),%a0       #address of secondary table',`
         lea     tb_auxpt,%a0       #address of secondary table
')
         movem.l  0(%a0,%d3.w),%d2-%d3 #fetch value
ifdef(`PIC',`
         bsr.l    rmul      #do the operation',`
         jsr      rmul      #do the operation
')
         move.w  (%sp)+,%d6       #restore exponent
         move.w  %d6,%d3          #find exponent mod 64
         asr.w   &6,%d3
         bpl.w     divfix2        #thank you Paul
            addq.w  &1,%d3
divfix2:  asl.w   &6,%d3
         sub.w   %d3,%d6
         add.w   &64,%d6         #bias to the positive
         bra.w     bcdr_4         #one more multiply to do
#
#  Generate the floating point exception.
#
err_impvalue:  trap   &SIGFPE
              rts

#****************************************************************************
#
#  These are the power-of-ten tables that are used in the 
#  bcd --> real conversions.
#  Decimal  in the range [10^(-64),10^(64)] convert into real numbers 
#  with one real
#  multiply while all other decimal <--> real conversions require
#  2 real multiplies and the use of the table tb_auxpt.
#
#  The table contains the real values:
#  10^(-80),10^(-79),...,10^(0),.10^(1),...,10^(64).
#
	data
tb_pwt:	long	0x2F52F8AC,0x174d6123,0x2F87B6d7,0x1d20B96C
	long	0x2FBDa48C,0xE468E7C7,0x2FF286D8,0x0EC190DC
	long	0x3027288E,0x1271F513,0x305CF2B1,0x970E7258
	long	0x309217AE,0xFE690777,0x30C69D9A,0xBE034955
tb_pwt8:	long	0x30FC4501,0x6D841BAA,0x3131AB20,0xE472914A
	long	0x316615E9,0x1D8F359D,0x319B9B63,0x64F30304
tb_pwt4:	long	0x31d1411E,0x1F17E1E3,0x32059165,0xa6DDDa5B
	long	0x323AF5BF,0x109550F2,0x3270D997,0x6a5d5297
tb_pwtt:	long	0x32a50FFD,0x44F4a73D,0x32Da53FC,0x9631d10D
	long	0x3310747D,0xDDDF22A8,0x3344919D,0x5556EB52
	long	0x3379B604,0xAAACa626,0x33B011C2,0xEAABE7D8
	long	0x33E41633,0xa556E1CE,0x34191BC0,0x8EAC9a41
	long	0x344F62B0,0xB257C0d2,0x34839DAE,0x6F76D883
	long	0x34B8851A,0x0B548Ea4,0x34EEa660,0x8E29B24D
	long	0x352327FC,0x58Da0F70,0x3557F1FB,0x6F10934C
	long	0x358DEE7A,0x4Ad4B81F,0x35C2B50C,0x6EC4F313
	long	0x35F7624F,0x8a762FD8,0x362d3AE3,0x6d13BBCE
	long	0x366244CE,0x242C5561,0x3696d601,0xAd376AB9
	long	0x36cc8B82,0x18854567,0x3701d731,0x4F534B61
	long	0x37364CFD,0xa3281E39,0x376BE03D,0x0BF225C7
	long	0x37a16C26,0x2777579C,0x37d5C72F,0xB1552D83
	long	0x380B38FB,0x9DAa78E4,0x3841039D,0x428A8B8F
	long	0x38754484,0x932d2E72,0x38AA95a5,0xB7F87a0F
	long	0x38E09D87,0x92FB4C49,0x3914C4E9,0x77Ba1F5C
	long	0x3949F623,0xd5A8a733,0x398039d6,0x65896880
	long	0x39B4484B,0xFEEBC2a0,0x39E95a5E,0xFEa6B347
	long	0x3a1FB0F6,0xBE506019,0x3a53CE9A,0x36F23C10
	long	0x3A88C240,0xC4AECB14,0x3ABEF2d0,0xF5Da7DD9
	long	0x3AF357C2,0x99A88Ea7,0x3B282DB3,0x4012B251
	long	0x3B5E3920,0x10175EE6,0x3B92E3B4,0x0a0E9B4F
	long	0x3BC79Ca1,0x0C924223,0x3BFD83C9,0x4FB6d2AC
	long	0x3C32725D,0xd1d243AC,0x3C670EF5,0x4646d497
	long	0x3C9Cd2B2,0x97D889BC,0x3Cd203AF,0x9EE75616
	long	0x3d06849B,0x86a12B9B,0x3d3C25C2,0x68497682
	long	0x3d719799,0x812DEa11,0x3Da5Fd7F,0xE1796495
	long	0x3DDB7CDF,0xD9d7BDBB,0x3E112E0B,0xE826d695
	long	0x3E45798E,0xE2308C3A,0x3E7Ad7F2,0x9ABCAF48
	long	0x3EB0C6F7,0xa0B5ED8D,0x3EE4F8B5,0x88E368F1
	long	0x3F1a36E2,0xEB1C432D,0x3F50624D,0xd2F1A9FC
	long	0x3F847AE1,0x47AE147B,0x3FB99999,0x9999999A
	long	0x3FF00000,0x00000000
	long	0x40240000,0x00000000,0x40590000,0x00000000
	long	0x408F4000,0x00000000,0x40C38800,0x00000000
	long	0x40F86a00,0x00000000,0x412E8480,0x00000000
	long	0x416312d0,0x00000000,0x4197d784,0x00000000
	long	0x41CDCd65,0x00000000,0x4202a05F,0x20000000
	long	0x42374876,0xE8000000,0x426d1A94,0xa2000000
	long	0x42a2309C,0xE5400000,0x42d6Bcc4,0x1E900000
	long	0x430C6BF5,0x26340000,0x4341C379,0x37E08000
	long	0x43763457,0x85D8a000,0x43ABC16D,0x674EC800
	long	0x43E158E4,0x60913d00,0x4415AF1D,0x78B58C40
	long	0x444B1AE4,0xd6E2EF50,0x4480F0CF,0x064Dd592
	long	0x44B52d02,0xC7E14AF6,0x44Ea7843,0x79D99DB4
	long	0x45208B2A,0x2C280291,0x4554ADF4,0xB7320335
	long	0x4589D971,0xE4FE8402,0x45C027E7,0x2F1F1281
	long	0x45F431E0,0xFAE6d721,0x46293E59,0x39a08CEA
	long	0x465F8DEF,0x8808B024,0x4693B8B5,0xB5056E17
	long	0x46C8a6E3,0x2246C99C,0x46FEd09B,0xEAD87C03
	long	0x47334261,0x72C74D82,0x476812F9,0xCF7920E3
	long	0x479E17B8,0x4357691B,0x47d2CEd3,0x2a16a1B1
	long	0x48078287,0xF49C4a1D,0x483d6329,0xF1C35Ca5
	long	0x48725DFA,0x371a19E7,0x48a6F578,0xC4E0a061
	long	0x48DCB2d6,0xF618C879,0x4911EFC6,0x59CF7d4C
	long	0x49466BB7,0xF0435C9E,0x497C06a5,0xEC5433C6
	long	0x49B18427,0xB3B4a05C,0x49E5E531,0xa0a1C873
	long	0x4a1B5E7E,0x08Ca3A8F,0x4a511B0E,0xC57E649A
	long	0x4A8561d2,0x76DDFDC0,0x4ABABa47,0x14957d30
	long	0x4AF0B46C,0x6CDd6E3E,0x4B24E187,0x8814C9CE
	long	0x4B5a19E9,0x6a19FC41,0x4B905031,0xE2503DA9
	long	0x4BC4643E,0x5AE44d13,0x4BF97d4D,0xF19d6057
	long	0x4C2FDCa1,0x6E04B86D,0x4C63E9E4,0xE4C2F344
	long	0x4C98E45E,0x1DF3B015,0x4ccF1d75,0xa5709C1B
	long	0x4d037269,0x87666191,0x4d384F03,0xE93FF9F5

#****************************************************************************
#
#  This table is used to convert those decimal numbers outside the
#  range of [10^(-64),10^(64)] to real numbers.
#
#  The table contains the real values:
#  10(^-256),10^(-192),...,10^(0),10^(64),...,10^(256).
#
tb_auxpt: long	0x0AC80628,0x64AC6F43,0x18123FF0,0x6EEA847A
	long	0x255BBa08,0xCF8C979D,0x32a50FFD,0x44F4a73D
	long	0x3FF00000,0x00000000
	long	0x4d384F03,0xE93FF9F5,0x5A827748,0xF9301d32
	long	0x67cc0E1E,0xF1a724EB,0x75154FDD,0x7F73BF3C
#****************************************************************************
#
#  The next table is used in converting pairs of decimal mantissa digits
#  into their binary value in the decimal --> real conversion. The
#  two decimal digits are treated as an offset into the table, where their
#  binary value is stored.
#
tb_bcd:	byte	0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0
	byte	10,11,12,13,14,15,16,17,18,19,0,0,0,0,0,0
	byte	20,21,22,23,24,25,26,27,28,29,0,0,0,0,0,0
	byte	30,31,32,33,34,35,36,37,38,39,0,0,0,0,0,0
	byte	40,41,42,43,44,45,46,47,48,49,0,0,0,0,0,0
	byte	50,51,52,53,54,55,56,57,58,59,0,0,0,0,0,0
	byte	60,61,62,63,64,65,66,67,68,69,0,0,0,0,0,0
	byte	70,71,72,73,74,75,76,77,78,79,0,0,0,0,0,0
	byte	80,81,82,83,84,85,86,87,88,89,0,0,0,0,0,0
	byte	90,91,92,93,94,95,96,97,98,99

ifdef(`PROFILE',`
		data
p_pbcdrel:	long	0
	')
