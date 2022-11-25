/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/clock.c,v $
 * $Revision: 1.7.84.7 $	$Author: drew $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/10/12 07:25:51 $
 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

#include "../h/param.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../s200/cpu.h"
#include "../s200/clock.h"
#include "../s200io/bootrom.h"
#include "../graf/hil.h"
#ifdef  FSD_KI
#include "../h/user.h"
#include "../h/ki_calls.h"
#include "../h/debug.h"
#endif  /* FSD_KI */

typedef enum {clock_none, clock_epson, clock_utility} clock_type;
clock_type check_for_rtc(), rtc_type = clock_none;

char *checkmsg = "check and reset the date\n";
char *rtcmsg = "Battery-backed real-time clock";

/*
 * Epson registers:
 *
 * register	name		format	range	comments
 * 0		seconds 	lsd	0-9
 * 1		seconds 	msd	0-5
 * 2		minutes 	lsd	0-9
 * 3		minutes 	msd	0-5
 * 4		hours   	lsd	0-9
 * 5		hours   	msd	0-1/0-2	((hour/10) & 03) + 8;
 * 6		day of week		0-6	unused
 * 7		day of month	lsd	0-9
 * 8		day of month	msd	0-3
 * 9		month		lsd	0-9
 * 10		month		msd	0-1
 * 11		year		lsd	0-9
 * 121		year		msd	0-9
 */

static int dmsize[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

struct tm * gmt_to_rtc();

struct hilrec *clock_hil;

/*
 * Machine-dependent clock routines.
 *
 * Startrtclock restarts the real-time clock, which provides
 * hardclock interrupts to kern_clock.c.  It is in clocks.s.
 *
 * Inittodr initializes the system time of day, based on the
 * file system value or the rtc whichever is later.  
 *
 */

inittodr(base)
time_t base;
{
	unsigned int timbuf = base;	/* assume no rtc */

	if (check_for_rtc() != clock_none) {
		msg_printf("    %s\n", rtcmsg);
		/*
		** rtc_to_gmt stores the rtc ttodr time in timbuf.
		** if filesystem time is more recent than the rtc time
		** then use the filesystem time and warn the user.
		*/
		if (rtc_to_gmt(&timbuf) || (timbuf < base)) {
			printf("WARNING: bad date in real-time clock--%s",
				checkmsg);
			timbuf = base;
		}
	} else
		printf("WARNING: no %s--%s\n", rtcmsg, checkmsg);

	/* We are not precise enough to worry about time.tv_usec */
	time.tv_sec = timbuf;
}

struct utility_rtc {
	char addr;
	char dummy1, dummy2, dummy3;
	char data;
	char dummy4, dummy5, dummy6;
} *ut_rtc = (struct utility_rtc *) (LOG_IO_OFFSET+0x410080);

#define get_ut_reg(reg) (ut_rtc->addr = (reg), ut_rtc->data)
#define write_ut_reg(reg, datum) {ut_rtc->addr = (reg); ut_rtc->data=(datum);}

resettodr(t)
struct timeval *t;
{
	register struct tm *ct;

	ct = gmt_to_rtc(t->tv_sec);

	switch(check_for_rtc()) {
	case clock_epson:
		write_rtc_reg(15, 13);	/* reset prescalar */

		/*
		 * If the seconds (and therefore, all the other values) change
		 * while we're writing things, the time could be inconsistent.
		 * To get around this, we write the seconds to 00 first,
		 * write everything else, then write the proper seconds.
		 * The 00 gives us an entire minute to finish writing the
		 * values before we get into trouble with the minutes changing.
		 */

		write_rtc_reg(0,  0);		/* LSD of seconds */
		write_rtc_reg(1,  0);		/* MSD of seconds */

		write_rtc_reg(2,  ct->tm_min % 10);
		write_rtc_reg(3,  ct->tm_min / 10);

		write_rtc_reg(4,  ct->tm_hour % 10);
		write_rtc_reg(5,  ((ct->tm_hour / 10) & 0x03) + 8);

		write_rtc_reg(7,  ct->tm_mday % 10);
		write_rtc_reg(8,  ct->tm_mday / 10);

		write_rtc_reg(9,  ct->tm_mon % 10);
		write_rtc_reg(10, ct->tm_mon / 10);

		write_rtc_reg(11, ct->tm_year % 10);
		write_rtc_reg(12, ct->tm_year / 10);

		write_rtc_reg(1,  ct->tm_sec / 10);
		write_rtc_reg(0,  ct->tm_sec % 10);	/* write this last */

		break;

	case clock_utility:
		write_ut_reg(0xA, 0x20); /* DV=32MHZ, RS=none */
		write_ut_reg(0xB, 0x86); /* Setting time, interrupts off,
					    binary data, 24 hour, no DST */
		/*
		 * We don't have the above problem of the clock ticking
		 * while we're setting it, because we told it to stop updating.
		 */
		write_ut_reg(0, ct->tm_sec);
		write_ut_reg(2, ct->tm_min);
		write_ut_reg(4, ct->tm_hour);
		write_ut_reg(7, ct->tm_mday);
		write_ut_reg(8, ct->tm_mon);
		write_ut_reg(9, ct->tm_year);
		(void) get_ut_reg(0xD);	 /* read register D to set valid bit */
		write_ut_reg(0xB, 0x06); /* start updating */
		break;
	}
}

struct tm *
gmt_to_rtc(tim)
long tim;
{
	register int d;
	long hms, day;
	static struct tm xtime;

	/* break initial number into days */
	hms = tim % 86400L;
	day = tim / 86400L;
	if (hms < 0) {
		hms += 86400L;
		day -= 1;
	}
	/* generate hours:minutes:seconds */
	xtime.tm_sec = hms % 60;
	d = hms / 60;
	xtime.tm_min = d % 60;
	d /= 60;
	xtime.tm_hour = d;

	/* year number */
	for (d=70; day >= dysize(d); d++)
		day -= dysize(d);
	if (d>100)
		d-=100;				/* just want last two digits */
	xtime.tm_year = d;

	/* generate month */

	if (dysize(d) == 366)
		dmsize[1] = 29;
	for (d=0; day >= dmsize[d]; d++)
		day -= dmsize[d];
	dmsize[1] = 28;
	xtime.tm_mday = day+1;
	xtime.tm_mon = d+1;
	xtime.tm_isdst = 0;
	return(&xtime);
}

rtc_to_gmt(timbuf)
register unsigned int *timbuf;
{
	register int i;
	int year, month, day, hour, min, sec;

	read_rtc(&hour, &min, &sec, &day, &month, &year);

	if (hour<0 || hour>23)
		return(1);
	if (min<0 || min>59)
		return(1);
	if (sec<0 || sec>59)
		return(1);
	if (day<1 || day>31)
		return(1);
	if (month<1 || month>12)
		return(1);
	if (year<0 || year>99)
		return(1);
	if (year < 70)
		year += 2000;
	else
		year += 1900;
	*timbuf = 0;
	for (i=1970; i<year; i++)
		*timbuf += dysize(i);
	/* Leap year */
	if (dysize(year)==366 && month >= 3)
		*timbuf += 1;
	while (--month)
		*timbuf += dmsize[month-1];
	*timbuf += (day-1);
	*timbuf *= 24;		/* days to hours */
	*timbuf += hour;
	*timbuf *= 60;		/* hours to minutes */
	*timbuf += min;
	*timbuf *= 60;		/* minutes to seconds */
	*timbuf += sec;
	return(0);		/* everything went ok */
}


clock_type
check_for_rtc()
{
	if (rtc_type != clock_none)		/* Already know the type? */
		return rtc_type;		/* Return it. */

	if (epson_present())
		rtc_type = clock_epson;
	else if (utility_present())
		rtc_type = clock_utility;
	else
		rtc_type = clock_none;

	return rtc_type;
}


epson_present()
{
	unsigned char data;


	if (sysflags & NOKBD)
		return(0);

	/* Make a fake hil structure */
	if (clock_hil == NULL) {
		clock_hil = (struct hilrec *) calloc(sizeof(struct hilrec));
		bzero(clock_hil, sizeof(*clock_hil));/* zero out most stuff */
		clock_hil->hilbase = (struct data8042 *) RTC_BASE;
		clock_hil->loopcmddone = 1;
	}

	data=0;				/* in case clock_cmd fails */
	clock_cmd(clock_hil, GET_CONFIG, NULL, 0, &data);

	if (data & EXTEND_PRESENT) {
		data=0;
		clock_cmd(clock_hil, RTC_ID, NULL, 0, &data);
		if (data & RTC_PRESENT)
			return(1);
	}
	return(0);
}

utility_present()
{
	return testr(&ut_rtc->data, 1);
}


unsigned char get_rtc_reg(reg_num)
int reg_num;
{
	unsigned char data;

	data = reg_num;

	clock_cmd(clock_hil, RTC_REG, &data, 1, NULL);
	clock_cmd(clock_hil, RTC_TRIGGER_READ, NULL, 0, &data);
	return(data);
}

read_rtc(hour, min, sec, day, mon, year)
int *hour, *min, *sec, *day, *mon, *year;
{
	register int i, good_read;
	int breg;
	unsigned char rtc_regs[13];

	switch (rtc_type) {
	case clock_epson:
		do {
			/* Read all the registers */
			for (i=0; i <= 12; i++)
				rtc_regs[i] = get_rtc_reg(i);

			/* Read them again, and insist on the same value */
			good_read = 1;			/* assume success */
			for (i=0; i <= 12; i++)
				if (rtc_regs[i] != get_rtc_reg(i))
					good_read = 0;
		} while (!good_read);

		*sec = conv_to_dec(1,0);
		*min = conv_to_dec(3,2);
		*hour = (rtc_regs[5] & 0x03)*10 + rtc_regs[4];
		*day = conv_to_dec(8,7);
		*mon = conv_to_dec(10,9); 
		*year = conv_to_dec(12,11);

		break;
	
	case clock_utility:
		/* Wait for the update bit to clear */
		while (get_ut_reg(0xA) & 0x80);

		/* Read the data within 244 micro-seconds */
		*sec  = get_ut_reg(0);
		*min  = get_ut_reg(2);
		*hour = get_ut_reg(4);
		*day  = get_ut_reg(7);
		*mon  = get_ut_reg(8);
		*year = get_ut_reg(9);
		breg  = get_ut_reg(0xB);

		/* Convert bcd to binary */
		if ((breg & 04)==0) {
			*sec  = (*sec >>4)*10 + (*sec  & 0x0f);
			*min  = (*min >>4)*10 + (*min  & 0x0f);
			*hour = (*hour>>4)*10 + (*hour & 0x0f);
			*day  = (*day >>4)*10 + (*day  & 0x0f);
			*mon  = (*mon >>4)*10 + (*mon  & 0x0f);
			*year = (*year>>4)*10 + (*year & 0x0f);
		}

		/* Convert 12-hour mode to 24-hour mode */
		if ((breg & 02)==0) {
			if ((*hour & 0x7f)==12)		/* 12 -> 0 */
				*hour -= 12;
			if (*hour & 0x80) {		/* if pm: */
				*hour &= ~0x80;		/* clear 12-hour bit */
				*hour += 12;		/* add 12 hours */
			}
		}

		/*
		printf("read_rtc: %02d:%02d:%02d %02d/%02d/%02d\n",
			*hour, *min, *sec, *mon, *day, *year);
		*/
		break;
	}
}

write_rtc_reg(reg, data)
unsigned int reg, data;
{
	unsigned char tmp;

	tmp = (unsigned char ) ((data << 4) + reg);

	clock_cmd(clock_hil, RTC_REG, &tmp, 1, NULL);
	clock_cmd(clock_hil, RTC_TRIGGER_WRITE, NULL, 0, NULL);
	clock_cmd(clock_hil, RTC_TRIGGER_READ, NULL, 0, &tmp);
}


/*
 * Oddly enough, clock commands don't cause interrupts.
 * Jack us up to level 7 to force polling instead.
 */
clock_cmd(hil_ptr, cmd, string, len, returned_data)
struct hilrec *hil_ptr;
char *string, *returned_data;
{
	int x;

	x = CRIT();
	hil_cmd(hil_ptr, cmd, string, len, returned_data);
	UNCRIT(x);
}

#ifdef	FSD_KI

#ifdef __hp9000s800
#pragma OPTIMIZE ON
#endif /* __hp9000s800 */

#ifdef __hp9000s300
#undef	ki_nunit_per_sec
#define	ki_nunit_per_sec	250000	/* XXX */
#define	_MFCTL(CR_IT, VAR)	(VAR = (struct ki_runtimes *)mfctl_CR_IT())
#define	_UINT_MFCTL(CR_IT, VAR)	(VAR = (u_int)mfctl_CR_IT())
#undef	GETPROCINDEX
#define	GETPROCINDEX(DUMMY)	(0)

u_int mfctl_CR_IT();
u_int read_adjusted_itmr();
#endif /* __hp9000s300 */

#ifdef OSDEBUG

int     ktc_push_disable = 0;
int     ktc_pop_disable = 0;
int     ktc_print_disable = 0;

ki_print_clk_stack()
{
	int *clkp;

	printf("KTC_STACK =");
	for (clkp = KI_CLK_BEGINNING_STACK_ADDRS;; clkp--)
	{
		printf(" %d", *clkp);	
		if (clkp <= KI_CLK_TOS_STACK_PTR ||
		    clkp <= KI_CLK_END_STACK_ADDRS) break;
	}
	printf("\n");
}

#ifndef IN_PROCESS_CONTEXT
#define IN_PROCESS_CONTEXT 1
#endif /* IN_PROCESS_CONTEXT XXX */

#define KI_TOS_POP_TEST(MSG) \
{ \
        if (ktc_pop_disable > 5) {return;} \
        if (KI_CLK_TOS_STACK_PTR >= KI_CLK_BEGINNING_STACK_ADDRS) \
        { 	ktc_pop_disable++; \
        	KI_CLK_TOS_STACK_PTR = KI_CLK_BEGINNING_STACK_ADDRS-1; \
		if (IN_PROCESS_CONTEXT) \
		{  \
			printf("Pid = %d, syscall = %d\n", \
				u.u_procp->p_pid, u.u_syscall); \
			ki_print_clk_stack();\
		} \
		printf(MSG); \
	} \
}

#define KI_TOS_PUSH_TEST(CLK) \
{ \
	if (ktc_push_disable > 5) {return;} \
	if ((KI_CLK_TOS_STACK_PTR-6) <= KI_CLK_END_STACK_ADDRS) \
        { 	ktc_push_disable++; \
		if (IN_PROCESS_CONTEXT) \
		{  \
			printf("Pid = %d, syscall = %d\n", \
				u.u_procp->p_pid, u.u_syscall); \
			ki_print_clk_stack();\
		} \
		printf(" KTC push of clk %d\n", CLK); \
	} \
}

#else	/* ! OSDEBUG */

#define KI_TOS_POP_TEST(MSG)
#define KI_TOS_PUSH_TEST(CLK)

#endif	/* ! OSDEBUG */

/* KV += D; where KV is ki_timeval *, D is the delta CR_IT value */
/* Watch out: D may be modified on return */
#define	KI_ACCUM_KV_DELTA(KV, D) \
{\
	while((u_int)((KV)->tv_nunit += (D)) >= ki_nunit_per_sec) \
	{\
		(KV)->tv_sec++;\
		(D) = -(ki_nunit_per_sec);\
	}\
}


/* Get the ki time when already NON-interruptable */
ki_getprectime6(tmvalp)
register struct	ki_timeval *tmvalp;
{
	register u_int			new_time;
	register struct ki_proc_info	*kt;
	register struct	ki_timeval	*kv;
	register u_int			delta;

	kt = &ki_proc(GETPROCINDEX(new_time));

	/* Get the native hardware free running counter */
	/* and correct for offset from monarch */
	_UINT_MFCTL(CR_IT, new_time);	
#ifdef __hp9000s800
	new_time += kt->kt_offset_correction;

	/* get the change in time */
	delta = new_time - kt->kt_inckclk;
#endif /* __hp9000s800 */

#ifdef __hp9000s300
	delta = (u_short)(new_time - kt->kt_inckclk);
#endif /* __hp9000s300 */
	/* set the last time there was a caller */
	kt->kt_inckclk = new_time;

	/* accumulate ki_time += delta */
	kv  = &kt->kt_time;	 /* XXX make macro go faster */

	KI_ACCUM_KV_DELTA(kv, delta);

	/* return ki_time to caller */
	*tmvalp = kt->kt_time;

	/* bump number of times called */
	kt->kt_kernelcounts[KI_GETPRECTIME]++;
}

/* Get the ki time when can be interrupted  */
ki_getprectime(tmvalp)
register struct	ki_timeval *tmvalp;
{
	register reg_X;

	DISABLE_INT(reg_X);
	ki_getprectime6(tmvalp);
	ENABLE_INT(reg_X);
}

/*********Begin updating TOS KTC clock*********************/
/* get pointer to my cpu's private structures		  */
/* Get this cpu's current time			    	  */
/* accumulate to the idle clock (assume int not recurred) */
/* get ki_runtimes of current cpu/clock number		  */
/* where:						  */
/*	register struct ki_proc_info	*kt;		  */
/*	register struct	ki_runtimes	*kp;		  */
/*	register u_int			delta;		  */
/*	register or const		kt_clkn;	  */
/******** Caution: "kt_incmclk" is updated by exit*********/

#ifdef __hp9000s300
#define KTC_ACCUM_TIME(KT, KP, KT_CLKN, D) \
{\
	_MFCTL(CR_IT, (KP));\
	(KT) = &ki_proc(GETPROCINDEX(D));\
	(D) = (u_short)((u_int)(KP) - (KT)->kt_incmclk);\
	(KT)->kt_incmclk = (u_int)(KP);\
	(KP) = &(KT)->kt_timestruct[KT_CLKN];\
	(KP)->kp_accumtm.tv_nunit += (D);\
	while((u_int)((KP)->kp_accumtm.tv_nunit) >= ki_nunit_per_sec) \
	{\
		(KP)->kp_accumtm.tv_sec++;\
		(KP)->kp_accumtm.tv_nunit -= ki_nunit_per_sec; \
	}\
}

/* Same as above except R may change on exit */
#define KTC_ACCUM_TIME_REG(KT, KP, KT_CLKN, R) \
{\
	_MFCTL(CR_IT, KP);\
	(KT) = &ki_proc(GETPROCINDEX(R));\
	(R) = (u_short)((u_int)(KP) - (KT)->kt_incmclk);\
	(KT)->kt_incmclk = (u_int)(KP);\
	(KP) = &(KT)->kt_timestruct[KT_CLKN];\
	while((u_int)((KP)->kp_accumtm.tv_nunit += (R)) >= ki_nunit_per_sec) \
	{\
		(KP)->kp_accumtm.tv_sec++;\
		(R) = -(ki_nunit_per_sec); \
	}\
}
#endif /*  __hp9000s300 */

#ifdef __hp9000s800
#define KTC_ACCUM_TIME(KT, KP, KT_CLKN, D) \
{\
	_MFCTL(CR_IT, (KP));\
	(KT) = &ki_proc(GETPROCINDEX(D));\
	(D) = (u_int)(KP) - (KT)->kt_incmclk;\
	(KT)->kt_incmclk = (u_int)(KP);\
	(KP) = &(KT)->kt_timestruct[KT_CLKN];\
	(KP)->kp_accumtm.tv_nunit += (D);\
	while((u_int)((KP)->kp_accumtm.tv_nunit) >= ki_nunit_per_sec) \
	{\
		(KP)->kp_accumtm.tv_sec++;\
		(KP)->kp_accumtm.tv_nunit -= ki_nunit_per_sec; \
	}\
}

/* Same as above except R may change on exit */
#define KTC_ACCUM_TIME_REG(KT, KP, KT_CLKN, R) \
{\
	_MFCTL(CR_IT, KP);\
	(KT) = &ki_proc(GETPROCINDEX(R));\
	(R) = (u_int)(KP) - (KT)->kt_incmclk;\
	(KT)->kt_incmclk = (u_int)(KP);\
	(KP) = &(KT)->kt_timestruct[KT_CLKN];\
	while((u_int)((KP)->kp_accumtm.tv_nunit += (R)) >= ki_nunit_per_sec) \
	{\
		(KP)->kp_accumtm.tv_sec++;\
		(R) = -(ki_nunit_per_sec); \
	}\
}
#endif /* __hp9000s800 */

/* Push the passed KTC clock number on the KTC clock stack */

ki_accum_push_TOS(ktc_clk)
{
	register u_int	reg_X;

	KI_TOS_PUSH_TEST(ktc_clk);

	DISABLE_INT(reg_X);

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
		register struct ki_proc_info	*kt;
		register struct	ki_runtimes	*kp;
		register u_int			delta;
		register u_int			reg_X;
		register u_int                  cur_clk;

		/* Get current clock number on top of stack */
		cur_clk = *KI_CLK_TOS_STACK_PTR;

		/* Check for sanity */
		VASSERT(cur_clk < KT_NUMB_CLOCKS);

		/* Accumulate cur_clk to the KTC Timers */
		KTC_ACCUM_TIME(kt, kp, cur_clk, delta);

		/* If a systemcall clock, also accumulate its individual clocks */
		if (cur_clk == KT_SYS_CLOCK)
		{
			register struct	ki_timeval	*kv;
			kv = &kt->kt_syscall_times[u.u_syscall];

			/* accumulate to current syscall clock */
			KI_ACCUM_KV_DELTA(kv, delta);
		} 
		else if (ktc_clk == KT_SYS_CLOCK)
		{
			register struct	ki_timeval	*kv;

			/* get "old" ki_time */
			u.u_syscall_time = kt->kt_time;

			/* Save KT_SYS_CLOCK for ki_swtch/ki_resume */
			/* This is a temp kludge for 9.0 ki_calls.h frozen */
			kp = &kt->kt_timestruct[KT_SYS_CLOCK];
			kp->kp_accumin = kp->kp_accumtm;

			/* Get the native hardware free running counter */
			/* and correct for offset from monarch */
			delta = kt->kt_incmclk; /* _MFCTL(CR_IT, delta); */
#ifdef __hp9000s800
			delta += kt->kt_offset_correction;

			/* get the change in time */
			delta -= kt->kt_inckclk;
#endif /* __hp9000s800 */

#ifdef __hp9000s300
			delta = (u_short)(delta - kt->kt_inckclk);
#endif /* __hp9000s300 */
			/* accumulate ki_time += delta */
			kv  = &u.u_syscall_time; /* XXX make macro go faster */

			/* now update to current time (+delta)  */
			KI_ACCUM_KV_DELTA(kv, delta);
		}
	}
	/* And push passed clock on the KTC clock stack */
	*(--KI_CLK_TOS_STACK_PTR) = ktc_clk;

	ENABLE_INT(reg_X);
}

/* 
 * Accumulate to the TOS clock and push KT_INTSYS_CLOCK or KT_INTUSR_CLOCK
 * or the IDLE clock, but dont push if clock is idle because...
 * NO u_area or it may be IN USE on another processor.
 */

#ifdef __hp9000s800
ki_accum_push_TOS_int(ssp)
struct save_state *ssp;
{
	/* This routine is entered with PSW_I off */
	register u_int			delta;
	register struct ki_proc_info	*kt;
	register struct	ki_runtimes	*kp;
	register u_int			cur_clk;

	/* Note: "return" is used frequently for better readability */

	/* If interrupt recurred - ignore */
	if (ssp->ss_flags & SS_PSPKERNEL) return;

	/* NO u_area (KTC stack) or it may be IN USE on another processor. */
	if (GETNOPROC(delta)) 
	{
		/* If enabled, accumulate on IDLE clock */
		if (ki_kernelenable[KI_GETPRECTIME]) 
		{
			/* Accumulate IDLE clock to the KTC Timers */
			KTC_ACCUM_TIME_REG(kt, kp, KT_IDLE_CLOCK, delta);
		}
		return;
	}
	/* We interrupted a running process, so figure out the mode */

	/* Get current clock number on top of stack */
	cur_clk = *KI_CLK_TOS_STACK_PTR;

	/* Check for sanity */
	VASSERT(cur_clk < KT_NUMB_CLOCKS);

	/* push new clock (keep overhead low as possible) */
	if (cur_clk == KT_USR_CLOCK)
	{
		KI_TOS_PUSH_TEST(KT_INTUSR_CLOCK);

		if (ki_kernelenable[KI_GETPRECTIME]) 
		{
			KTC_ACCUM_TIME_REG(kt, kp, KT_USR_CLOCK, delta);
			/* Note: delta has no useful information */
		}
       		*(--KI_CLK_TOS_STACK_PTR) = KT_INTUSR_CLOCK;
		return;
	}
	if (cur_clk == KT_SYS_CLOCK)
	{
		KI_TOS_PUSH_TEST(KT_INTSYS_CLOCK);

		if (ki_kernelenable[KI_GETPRECTIME]) 
		{
			register struct	ki_timeval	*kv;

			/* Enabled, accumulate cur_clk to the KTC Timers */
			KTC_ACCUM_TIME(kt, kp, KT_SYS_CLOCK, delta);
			/* Note: delta contains the incremental time for below */

			/* For better performance in MACRO */
			kv = &kt->kt_syscall_times[u.u_syscall];
			/* 
			 * Accumulate to current systemcall 
			 * or kernel daemon clock
			 */
			KI_ACCUM_KV_DELTA(kv, delta);
		}
       		*(--KI_CLK_TOS_STACK_PTR) = KT_INTSYS_CLOCK;
		return;
	}
	KI_TOS_PUSH_TEST(KT_INTSYS_CLOCK);

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
		/* Enabled, accumulate cur_clk to the KTC Timers */
		KTC_ACCUM_TIME(kt, kp, cur_clk, delta);
	}
	/* Interrupt system clock is used for everything else */
       	*(--KI_CLK_TOS_STACK_PTR) = KT_INTSYS_CLOCK;
}
#endif /* __hp9000s800 */

#ifdef __hp9000s300
/* 
 * Accumulate to the TOS clock and push KT_INTSYS_CLOCK or KT_INTUSR_CLOCK
 * depending on the TOS clock
 */
ki_accum_push_TOS_int()
{
	/* This routine is entered with PSW_I off */
	register struct ki_proc_info	*kt;
	register struct	ki_runtimes	*kp;
	register u_int			cur_clk;
	register u_int			delta;

	KI_TOS_PUSH_TEST("KI push_TOS_int");

	/* Get current clock number on top of stack */
	cur_clk = *KI_CLK_TOS_STACK_PTR;

	/* Check for sanity */
	VASSERT(cur_clk < KT_NUMB_CLOCKS);

	/* Only accumulate time if KT_CLOCKS are turned on */
	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
		/* Accumulate cur_clk to the KTC Timers */
		KTC_ACCUM_TIME(kt, kp, cur_clk, delta);
	}
	/* If a systemcall clock, also accumulate its individual clocks */
	switch(cur_clk)
	{
	case	KT_SYS_CLOCK:
		/* Only accumulate time if KT_CLOCKS are turned on */
		if (ki_kernelenable[KI_GETPRECTIME]) 
		{
			register struct	ki_timeval	*kv;
			kv = &kt->kt_syscall_times[u.u_syscall];

			/* accumulate to current syscall clock */
			KI_ACCUM_KV_DELTA(kv, delta);
		}
		break;

	case	KT_USR_CLOCK:
		/* push an interrupt from user clock */
        		*(--KI_CLK_TOS_STACK_PTR) = KT_INTUSR_CLOCK;
		return;

	case    KT_IDLE_CLOCK:
		/* push an idle clock */
       		*(--KI_CLK_TOS_STACK_PTR) = KT_INTIDLE_CLOCK;
       		return;

	default:
		break;
	} /* end switch */
	/* And push a INTSYS clock */
       	*(--KI_CLK_TOS_STACK_PTR) = KT_INTSYS_CLOCK;
}
#endif /* __hp9000s300 */


#ifdef __hp9000s300
ki_accum_pop_TOS_int()
#endif /* __hp9000s300 */

#ifdef __hp9000s800
ki_accum_pop_TOS_int(ssp)
struct save_state *ssp;
#endif /* __hp9000s800 */
{
	/* This routine is entered with PSW_I off */
	register u_int			reg_R;
	register struct ki_proc_info	*kt;
	register struct	ki_runtimes	*kp;
	register u_int			cur_clk;

#ifdef __hp9000s800
	/* If interrupt recurred - ignore */
	if (ssp->ss_flags & SS_PSPKERNEL) return;

	/* NO u_area andr KTC stack -- IDLE mode */
	if (GETNOPROC(reg_R)) 
	{
		/* only accumulate time if KT_CLOCKS are turned on */
		if (ki_kernelenable[KI_GETPRECTIME]) 
		{
			/* Accumulate INTIDLE clock to the KTC Timers */
			KTC_ACCUM_TIME_REG(kt, kp, KT_INTIDLE_CLOCK, reg_R);

			/* "pop of INTIDLE", so update its clock counts */
			kp->kp_clockcnt++;
		}
		return;
	}
#endif /* __hp9000s800 */
	KI_TOS_POP_TEST("KI pop_TOS_int\n");

	/* Get current clock number (INTUSR or INTSYS) */
	cur_clk = *KI_CLK_TOS_STACK_PTR;

	/* Check for sanity */
#ifdef __hp9000s800
	VASSERT(cur_clk == KT_INTSYS_CLOCK || cur_clk == KT_INTUSR_CLOCK);
#endif /* __hp9000s800 */

#ifdef __hp9000s300
	VASSERT(cur_clk == KT_INTSYS_CLOCK || cur_clk == KT_INTUSR_CLOCK || cur_clk == KT_INTIDLE_CLOCK);
#endif /* __hp9000s300 */

	/* only accumulate time if KT_CLOCKS are turned on */
	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
		/* Accumulate cur_clk to the KTC Timers */
		KTC_ACCUM_TIME_REG(kt, kp, cur_clk, reg_R);
	
		/* "pop of INTUSR or INTSYS", so update its counts */
		kp->kp_clockcnt++;
	}
	/* pop off the current clock */
	KI_CLK_TOS_STACK_PTR++; 
}


#ifdef __hp9000s800
/* Accumulate to the IDLE clock when leaving idle loop */
ki_accum_idle_clock()
{
	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
		register struct ki_proc_info	*kt;
		register struct	ki_runtimes	*kp;
		register u_int			reg_R;
		register u_int			reg_X;

		DISABLE_INT(reg_X);

		/* Accumulate IDLE clock to the KTC Timers */
		KTC_ACCUM_TIME_REG(kt, kp, KT_IDLE_CLOCK, reg_R);

		/* "pop of IDLE", so update its counts */
		kp->kp_clockcnt++;

		ENABLE_INT(reg_X);
	}
}
#endif /* __hp9000s800 */

/* Accumulate to current Top of stack clock and pop it */
ki_accum_pop_TOS()
{
	register u_int	reg_X;
	register u_int	cur_clk;

	KI_TOS_POP_TEST("KI pop_TOS_?\n");

	DISABLE_INT(reg_X);

	/* Get current clock number on top of stack */
	cur_clk = *KI_CLK_TOS_STACK_PTR;

	/* Check for sanity */
	VASSERT(cur_clk < KT_NUMB_CLOCKS);

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
		register struct ki_proc_info	*kt;
		register struct	ki_runtimes	*kp;
		register u_int			reg_R;

		/* Accumulate cur_clk to the KTC Timers */
		KTC_ACCUM_TIME_REG(kt, kp, cur_clk, reg_R);

		/* "pop of VFAULT or TRAP", so update its clock counts */
		kp->kp_clockcnt++;
	}
	KI_CLK_TOS_STACK_PTR++;				

	ENABLE_INT(reg_X);
}

ki_accum_pop_TOS_sys()
{
	register u_int			reg_X;

	KI_TOS_POP_TEST("KI pop_TOS_sys\n");

	DISABLE_INT(reg_X);

	if (ki_kernelenable[KI_GETPRECTIME])
	{
		register struct ki_proc_info	*kt;
		register struct	ki_runtimes	*kp;
		register struct	ki_timeval	*kv;
		register u_int			delta;

		KTC_ACCUM_TIME(kt, kp, KT_SYS_CLOCK, delta);

		/* To help speed of macro */
		kv = &kt->kt_syscall_times[u.u_syscall];

		KI_ACCUM_KV_DELTA(kv, delta);

		/* count the number syscalls */
		kt->kt_syscallcounts[u.u_syscall]++;

		/* "pop of sys", so update its clock counts */
		kp->kp_clockcnt++;
	}
/**	VASSERT(*KI_CLK_TOS_STACK_PTR == KT_SYS_CLOCK); XXX **/

	KI_CLK_TOS_STACK_PTR++;				

	ENABLE_INT(reg_X);
}

ki_accum_pop_TOS_csw()				
{
	register u_int	reg_X;

	KI_TOS_POP_TEST("KI pop_TOS_csw\n");

	DISABLE_INT(reg_X);

	if (ki_kernelenable[KI_GETPRECTIME])
	{
		register struct ki_proc_info	*kt;
		register struct	ki_runtimes	*kp;
		register u_int			reg_R;

		KTC_ACCUM_TIME_REG(kt, kp, KT_CSW_CLOCK, reg_R);

		/* When popping a CSW clock off a kernel deamon process,
	 	 * increment the number of times this daemon ran.
		 * Note: the 1st clock on stack is a SYS_CLOCK.
	 	 */
		if (*KI_CLK_BEGINNING_STACK_ADDRS == KT_SYS_CLOCK)
		{
			kt->kt_syscallcounts[u.u_syscall]++;
		}
		/* "pop of CSW", so update its clock counts */
		kp->kp_clockcnt++;
	}
	VASSERT(*KI_CLK_TOS_STACK_PTR == KT_CSW_CLOCK);

	KI_CLK_TOS_STACK_PTR++;

	ENABLE_INT(reg_X);
}

#ifdef __hp9000s800
/* Special way the 700/800 does it XXX (undo the trap) */
ki_replace_TOS_vfault()				
{
	/* Check for sanity */
	VASSERT(*KI_CLK_TOS_STACK_PTR == KT_TRAP_CLOCK);

	/* replace the trap clock number with vfault on the TOS */
	*KI_CLK_TOS_STACK_PTR = KT_VFAULT_CLOCK;	
}
#endif /* __hp9000s800 */

/****************Update the TOS KTC clock****************/
ki_accum_TOS_clock()
{
	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
		register struct ki_proc_info	*kt;
		register struct	ki_runtimes	*kp;
		register u_int			delta;
		register u_int			reg_X;
		register u_int                  cur_clk;

		DISABLE_INT(reg_X);

		/* Get current clock number on top of stack */
		cur_clk = *KI_CLK_TOS_STACK_PTR;

		/* Check for sanity */
		VASSERT(cur_clk < KT_NUMB_CLOCKS);

		/* Accumulate cur_clk to the KTC Timers */
		KTC_ACCUM_TIME(kt, kp, cur_clk, delta);

		/* If a systemcall clock, also accumulate its individual clocks */
		if (cur_clk == KT_SYS_CLOCK)
		{
			register struct	ki_timeval	*kv;
			kv = &kt->kt_syscall_times[u.u_syscall];

			/* accumulate to current syscall clock */
			KI_ACCUM_KV_DELTA(kv, delta);
		}
		ENABLE_INT(reg_X);
	}
}

#ifdef __hp9000s300
/* 
 * This is done because I am too lazy to modify (and test) the asm 
 * and the genassym.c routines for the 68k. 
 */
ki_accum_push_TOS_idle()
{
	ki_accum_push_TOS(KT_IDLE_CLOCK);
}

ki_accum_push_TOS_sys()
{
	ki_accum_push_TOS(KT_SYS_CLOCK);
}

ki_accum_push_TOS_csw()
{
	ki_accum_push_TOS(KT_CSW_CLOCK);
}

ki_accum_push_TOS_trap()
{
	ki_accum_push_TOS(KT_TRAP_CLOCK);
}

ki_accum_push_TOS_vfault()
{
	ki_accum_push_TOS(KT_VFAULT_CLOCK);
}
#endif /* __hp9000s300 */

/* Initialize KI time to begin NOW */
ki_init_clocks()
{

        /* Initialize the kt clock stack pointer */
        KI_CLK_TOS_STACK_PTR = KI_CLK_BEGINNING_STACK_ADDRS;
        *KI_CLK_TOS_STACK_PTR = KT_SYS_CLOCK;

	/* Initialize the KTC and KI clocks */
	ki_config_clear();

        /* Turn on the accumulation of time of ki clocks */
#ifdef OSDEBUG
	ki_kernelenable[KI_GETPRECTIME] = 1;
#else	/* !OSDEBUG */
	ki_kernelenable[KI_GETPRECTIME] = 0;
#endif	/* ! OSDEBUG */
}

/* reset the KI timers and clocks */
ki_config_clear()
{
	int my_index =		getprocindex();
	register		ii;
	u_int			monarch_CR_IT;
	int			saved_ki_getprectime;

	/* Save and turn off clocks while doing the zero (reset) */
	saved_ki_getprectime = ki_kernelenable[KI_GETPRECTIME];
	ki_kernelenable[KI_GETPRECTIME] = 0;

	for (ii = 0; ii < KI_MAX_PROCS; ii++ ) 
	{
#ifdef	MP
       		/* Skip if cpu not configured */
		if (mpproc_info[ii].prochpa == 0) continue;
#endif	/* MP */

		/* Hopefully other CPUs will finish up with clocks during this zero */
		/* Clear kernel stub counts */
       		bzero(&ki_kernelcounts(ii)[0], sizeof(u_int) * KI_MAXKERNCALLS);

		/* Clear systemcall counts and times */
		bzero(&ki_syscallcounts(ii)[0], sizeof(u_int) * KI_MAXSYSCALLS);
		bzero(&ki_syscall_times(ii)[0], sizeof(struct ki_timeval) * KI_MAXSYSCALLS);

		/* Clear KTC clocks, instruction counts and clockcounts */
       		bzero(ki_timesstruct(ii), sizeof(struct ki_runtimes) * KT_NUMB_CLOCKS);
	}
	ki_reset_time();

	/* restore the clock's enable flag */
	ki_kernelenable[KI_GETPRECTIME] = saved_ki_getprectime;
}

/* reset the KI timers and clocks */
ki_reset_time()
{
	u_int			monarch_CR_IT;
	register u_int		itmr;
	register		ii;
	int my_index =		getprocindex();

	/* Now try to do a respectible job of resetting the clocks from this cpu */
	/* It is "caveat emptor" if other cpus do this at the same time */

	/* Get the monarch's CR_IT, the KTC clocks for other CPUs start about NOW */
	monarch_CR_IT = read_adjusted_itmr();

	/* Get my CR_IT, the KTC clocks for this CPU start NOW */
	_UINT_MFCTL(CR_IT, itmr);	

	/* Get the recover counter stuff about here */
	/* Stop, read, reset, Start ?? or just read ?? */

	/* Now try to adjust the "old" CR_IT values to current time */
	for (ii = 0; ii < KI_MAX_PROCS; ii++) 
	{
#ifdef	MP
       		/* Skip if the cpu not configured */
		if (mpproc_info[ii].prochpa == 0) continue;
#endif	/* MP */

		/* The KI clocks use the monarch's view of CR_IT */
		ki_inckclk(ii) = monarch_CR_IT;

       		/* If my cpu, then use a more accurate method  */
		/* The KTC clocks use my_cpu view of CR_IT */
		if (my_index == ii) 
		{
			/* set my "old" CR_IT values current */
			ki_incmclk(ii) = itmr; /* for KTC clocks */
		} else
		{
			/*
			 * if  monarch_CR_IT == CR_IT + ki_offset_correction;, then
			 * CR_IT = monarch_CR_IT - ki_offset_correction;
			 */
			ki_incmclk(ii) = monarch_CR_IT - ki_offset_correction(ii);
		}
		/* set processor specific clock to current time */
		kernel_gettimeofday(&ki_time(ii));
		/* change struct timeval to struct ki_timeval */
#ifdef __hp9000s800
		ki_time(ii).tv_nunit *= itick_per_n_usec;
#endif /* __hp9000s800 */
#ifdef __hp9000s300
		ki_time(ii).tv_nunit >>= 2;
#endif /* __hp9000s300 */
	}
}
#endif	/* FSD_KI */
