/* HPUX_ID: @(#) $Revision: 70.1 $  */

/* header.h
 * This file contains defines used for the 'version' which was added
 * at release 6.5.A
 */

extern unsigned short 	version;	/* value for the a_stamp field */
extern short	version_seen;		/* Initially 0; set to 1 when a version 					 * pseudo-op is seen.
					 */
extern short	vcmd_seen;		/* Initially 0; set to 1 if a -V
					 * command line option is seen.
					 */
extern short	floatop_seen;		/* Initially 0; set to 1 when any
					 * dragon or 68881 op is used.
					 */

#ifdef PIC
extern int highwater;
extern int highwater_seen;
#endif

/* default version numbers when no version pseudo-op is specified */
#define VER_OLD		0	/* pre 6.5 */
#define VER_10		1	/* assembled with as10 */
#define VER_20_NOFLT	2	/* assembled with as20, no floating point
				 * operations (or no floating point that
				 * assumes non-scratch FP registers
				 */
#define VER_20_FLT	3	/* assembleed with as20, float is present,
				 * and ASSUMED TO FOLLOW 6.5 FLOAT REGISTER
				 * CONVENTIONS (!!)
				 */

