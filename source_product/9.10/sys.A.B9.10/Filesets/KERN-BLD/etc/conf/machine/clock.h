/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/clock.h,v $
 * $Revision: 1.4.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:02:54 $
 */
/* @(#) $Revision: 1.4.84.3 $ */    
#ifndef _MACHINE_CLOCK_INCLUDED /* allow multiple inclusions */
#define _MACHINE_CLOCK_INCLUDED

#define	SECDAY		((unsigned)(24*60*60))		/* seconds per day */
#define	SECYR		((unsigned)(365*SECDAY))	/* per common year */
#define	dysize(A) 	(((A)%4)? 365: 366)		/* days per year   */

#define	YRREF		1970
#define	LEAPYEAR(year)	((year)%4==0)	/* good till time becomes negative */

/*
 * Has the time-of-day clock wrapped around?
 */


/* defines for the BOBCAT battery backed real time clock */

/*
 * The RTC_BASE is 0x8000 less than it should be due to the offset of 0x8000
 * imposed by the HIL routines.  This comes from the offset of 0x8000 that is
 * present in the DIO HIL card.  This is present for the sake of compatibility
 * with the internal HIL, which is at 0x428000, so it was decided that, even
 * though the DIO card is at 0xXX0000, its registers would be at 0xXX8000.
 *
 * Compatibility can be a drag at times.
 */

#define		RTC_BASE		((0x420000-0x8000) + LOG_IO_OFFSET)

#define		RTC_ID			0xfe
#define		GET_CONFIG		0x11
#define		RTC_PRESENT		0x20
#define		EXTEND_PRESENT		0x20
#define		RTC_REG			0xe0
#define		RTC_TRIGGER_READ	0xc3
#define		RTC_TRIGGER_WRITE	0xc2

#define	conv_to_dec(A,B) (rtc_regs[A]*10 + rtc_regs[B])

#endif /* _MACHINE_CLOCK_INCLUDED */
