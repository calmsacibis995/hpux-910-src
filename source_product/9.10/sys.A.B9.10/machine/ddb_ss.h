/*
 * @(#)ddb_ss.h: $Revision: 1.2.84.3 $ $Date: 93/09/17 21:03:52 $
 * $Locker:  $
 */

struct SPECSTATE {		/* Format of HP 9000 Series 300 saved state */
	int	ss_gr[16];	/* hp-ux saved data and address registers */
	int	ss_usp;		/* hp-ux saved user stack pointer */
	short   fill;           /* fill */
	short   ss_sr;          /* hp-ux status register */
	int	ss_pc;          /* hp-ux program counter */
	int	ss_pc2;		/* actual proram counter */
	int	ss_fcode;       /* Fault code for reasons other than hitting */
                                /* a break instruction */
};

