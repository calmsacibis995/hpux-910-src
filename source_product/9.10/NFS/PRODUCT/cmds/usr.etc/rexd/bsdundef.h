/* 	@(#)bsdundef.h 	$Revision: 1.5.109.1 $	$Date: 91/11/19 14:15:22 $  */

/* 
 * All of the following are conflicting values between termio.h and sgtty.h,
 * and it turns out that in all of these cases, we want the sgtty.h version
 */
#undef HUPCL
#undef ECHO
#undef CR0
#undef CR1
#undef CR2
#undef CR3
#undef NL0
#undef NL1
#undef TAB0
#undef TAB1
#undef FF0
#undef FF1
#undef BS0
#undef BS1
#undef B50
#undef B75
#undef B110
#undef B134
#undef B150
#undef B200
#undef B300
#undef B600
#undef B900
#undef B1200
#undef B1800
#undef B2400
#undef B3600
#undef B4800
#undef B7200
#undef B9600
#undef B19200
#undef B38400
#undef EXTA
#undef EXTB
