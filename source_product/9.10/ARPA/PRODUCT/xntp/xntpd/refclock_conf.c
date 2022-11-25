/*
 * $Header: refclock_conf.c,v 1.2.109.3 94/11/09 10:52:17 mike Exp $
 * refclock_conf.c - reference clock configuration
 */
# include <stdio.h>
# include <sys/types.h>

# include "ntpd.h"
# include "ntp_refclock.h"

# ifdef REFCLOCK

static struct refclock  refclock_none =
{
    noentry, noentry, noentry, noentry, noentry, noentry, NOFLAGS
};

# 	ifdef	LOCAL_CLOCK
extern struct refclock  refclock_local;
# 	else	/* ! LOCAL_CLOCK */
# 		define	refclock_local	refclock_none
# 	endif	/* LOCAL_CLOCK */

# 	if defined(PST) || defined(PSTCLK) || defined(PSTPPS)
extern struct refclock  refclock_pst;
# 	else	/* ! defined (PST) || defined(PSTCLK) || defined(PSTPPS) */
# 		define	refclock_pst	refclock_none
# 	endif	/* defined (PST) || defined(PSTCLK) || defined(PSTPPS) */

# 	if defined(CHU) || defined(CHUCLK) || defined(CHUPPS)
extern struct refclock  refclock_chu;
# 	else	/* ! defined (CHU) || defined(CHUCLK) || defined(CHUPPS) */
# 		define	refclock_chu	refclock_none
# 	endif	/* defined (CHU) || defined(CHUCLK) || defined(CHUPPS) */

# 	if defined(GOES) || defined(GOESCLK) || defined(GOESPPS)
extern struct refclock  refclock_goes;
# 	else	/* ! defined (GOES) || defined(GOESCLK) || defined(GOESPPS) */
# 		define	refclock_goes	refclock_none
# 	endif	/* defined (GOES) || defined(GOESCLK) || defined(GOESPPS) */

# 	if defined(WWVB) || defined(WWVBCLK) || defined(WWVBPPS)
extern struct refclock  refclock_wwvb;
# 	else	/* ! defined (WWVB) || defined(WWVBCLK) || defined(WWVBPPS) */
# 		define	refclock_wwvb	refclock_none
# 	endif	/* defined (WWVB) || defined(WWVBCLK) || defined(WWVBPPS) */

# 	if defined(PARSE)
extern struct refclock  refclock_parse;
# 	else	/* ! defined (PARSE) */
# 		define	refclock_parse	refclock_none
# 	endif	/* defined (PARSE) */

# 	if defined(MX4200) || defined(MX4200CLK) || defined(MX4200PPS)
extern struct refclock  refclock_mx4200;
# 	else	/* ! defined (MX4200) || defined(MX4200CLK) || defined(MX4200PPS) */
# 		define	refclock_mx4200	refclock_none
# 	endif	/* defined (MX4200) || defined(MX4200CLK) || defined(MX4200PPS) */

# 	if defined(AS2201) || defined(AS2201CLK) || defined(AS2201PPS)
extern struct refclock  refclock_as2201;
# 	else	/* ! defined (AS2201) || defined(AS2201CLK) || defined(AS2201PPS) */
# 		define	refclock_as2201	refclock_none
# 	endif	/* defined (AS2201) || defined(AS2201CLK) || defined(AS2201PPS) */

# 	if defined(OMEGA) || defined(OMEGACLK) || defined(OMEGAPPS)
extern struct refclock  refclock_omega;
# 	else	/* ! defined (OMEGA) || defined(OMEGACLK) || defined(OMEGAPPS) */
# 		define	refclock_omega	refclock_none
# 	endif	/* defined (OMEGA) || defined(OMEGACLK) || defined(OMEGAPPS) */

# 	ifdef TPRO
extern struct refclock  refclock_tpro;
# 	else	/* ! TPRO */
# 		define	refclock_tpro	refclock_none
# 	endif	/* TPRO */

# 	if defined(LEITCH) || defined(LEITCHCLK) || defined(LEITCHPPS)
extern struct refclock  refclock_leitch;
# 	else	/* ! defined (LEITCH) || defined(LEITCHCLK) || defined(LEITCHPPS) */
# 		define	refclock_leitch	refclock_none
# 	endif	/* defined (LEITCH) || defined(LEITCHCLK) || defined(LEITCHPPS) */

# 	ifdef IRIG
extern struct refclock  refclock_irig;
# 	else	/* ! IRIG */
# 		define refclock_irig	refclock_none
# 	endif	/* IRIG */

# 	if defined(MSF) || defined(MSFCLK) || defined(MSFPPS)
extern struct refclock  refclock_msf;
# 	else	/* ! defined (MSF) || defined(MSFCLK) || defined(MSFPPS) */
# 		define refclock_msf	refclock_none
# 	endif	/* defined (MSF) || defined(MSFCLK) || defined(MSFPPS) */

/*
 * Order is clock_start(), clock_shutdown(), clock_poll(),
 * clock_control(), clock_init(), clock_buginfo, clock_flags;
 *
 * Types are defined in ntp.h.  The index must match this.
 */
struct refclock    *refclock_conf[] =
{
    &refclock_none,		/* 0 REFCLK_NONE */
    &refclock_local,		/* 1 REFCLK_LOCAL */
    &refclock_none,		/* 2 REFCLK_WWV_HEATH */
    &refclock_pst,		/* 3 REFCLK_WWV_PST */
    &refclock_wwvb,		/* 4 REFCLK_WWVB_SPECTRACOM
				   */
    &refclock_goes,		/* 5 REFCLK_GOES_TRUETIME 
				*/
    &refclock_irig,		/* 6 REFCLK_IRIG_AUDIO */
    &refclock_chu,		/* 7 REFCLK_CHU */
    &refclock_parse,		/* 8 REFCLK_PARSE */
    &refclock_mx4200,		/* 9 REFCLK_GPS_MX4200 */
    &refclock_as2201,		/* 10 REFCLK_GPS_AS2201 */
    &refclock_omega,		/* 11 REFCLK_OMEGA_TRUETIME
				   */
    &refclock_tpro,		/* 12 REFCLK_IRIG_TPRO */
    &refclock_leitch,		/* 13 REFCLK_ATOM_LEITCH */
    &refclock_msf,		/* 14 REFCLK_MSF_EES */
};

int     num_refclock_conf = sizeof (refclock_conf)/sizeof (struct refclock *);

# endif	/* REFCLOCK */
