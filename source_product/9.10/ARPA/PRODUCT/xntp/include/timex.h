/*
 * @(#)timex.h: $Revision: 1.2.109.1 $ $Date: 94/10/26 16:55:32 $
 * $Locker:  $
 */

/* timex.h,v 3.1 1993/07/06 01:07:06 jbj Exp
 * Header file for the precision time interface
 *
 * Codes for the getloop()/setloop() system call
 */
#define TIME_PREC 1	/* precision (log2(sec)) */
#define TIME_TCON 2	/* time constant (log2(sec) */
#define TIME_TOLR 3	/* frequency tolerance (ppm << 16) */
#define TIME_FREQ 4	/* frequency offset (ppm << 16) */
#define TIME_STAT 5	/* status (see return codes) */

/*
 * System clock status and return codes used by all routines. Negative
 * values are for return codes only and do not affect the status.
 */
#define TIME_UNS 0	/* unspecified or unknown */
#define TIME_OK 1	/* operation succeeded */
#define TIME_INS 2	/* insert leap second at end of current day */
#define TIME_DEL 3	/* delete leap second at end of current day */
#define TIME_OOP 4	/* leap second in progress */
#define TIME_BAD 5	/* system clock is not synchronized */
#define TIME_ADR -1	/* operation failed: invalid address */
#define TIME_VAL -2	/* operation failed: invalid argument */
#define TIME_PRV -3	/* operation failed: priviledged operation */

/*
 * The FREQ_SCALE define establishes the decimal point on the frequency
 * variable used by the system clock. It is needed by application programs
 * in order to scale their values to the units used by the kernel. The
 * value should agree with the SHIFT_KF value in the kernel.
 */
#define FREQ_SCALE 20	/* shift for frequency scale factor */

