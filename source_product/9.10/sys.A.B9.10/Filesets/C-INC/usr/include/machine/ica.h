/*
 * @(#)ica.h: $Revision: 1.15.83.5 $ $Date: 93/09/17 18:50:50 $
 * $Locker:  $
 * 
 */


/* This whole module is ica specific */

/*
 * Command to ica ioctl()
 */

#define IOC_ICASTART	1		/* Start ica, clear hitmap */
#define IOC_ICASTOP	2		/* Stop ica  */
#define IOC_ICAINIT	3		/* Initialize memory for text,maps */ 

/*  ica_initflag values */

#define ICA_INITOFF	0
#define ICA_INITON	1
#define ICA_ISON	2
#define ICA_ISOFF	3
#define ICA_INITFAIL	4

/* History information */
#define HISTORYLENGTH 8192	/* length in words, power of 2 */
#define HISTORYMASK	(HISTORYLENGTH-1)

/*
 * Interrupt related constants
 */


struct SPECSTATE {			/* Format of HP PA saved state */
	int	ss_gr[32];
	int	ss_sr[8];
	int	ss_rctr;	/* Recovery Counter Register */
	int	ss_ccr;		/* Coprocessor Confiquration Register */
	int	ss_sar;		/* Shift Amount Register */
	int	ss_pidr1;	/* Protection ID 1 */
	int	ss_pidr2;	/* Protection ID 2 */
	int	ss_pidr3;	/* Protection ID 3 */
	int	ss_pidr4;	/* Protection ID 4 */
	int	ss_iva;		/* Interrupt Vector Address */
	int	ss_eiem;	/* External Interrupt Enable Mask */
	int	ss_itmr;	/* Interval Timer */
	int	ss_pcsq0;	/* Program Counter Space queue (front) */
	int	ss_pcsq1;	/* Program Counter Space queue (back) */
	int	ss_pcoq0;	/* Program Counter Offset queue (front) */
	int	ss_pcoq1;	/* Program Counter Offset queue (back) */
	int	ss_iir;		/* Interruption Instruction Register */
	int	ss_isr;		/* Interruption Space Register */
	int	ss_ior;		/* Interruption Offset Register */
	int	ss_ipsw;	/* Interrpution Processor Status Word */
	int	ss_eirr;	/* External Interrupt Request */
	unsigned ss_ppda;	/* Physcial Page Directory Address */
	unsigned ss_hta;	/* Hash Table Address */
	int	ss_tr2;		/* Temporary register 2 */
	int	ss_tr3;		/* Temporary register 3 */
	int	ss_tr4;		/* Temporary register 4 */
	int	ss_tr5;		/* Temporary register 5 */
	int	ss_tr6;		/* Temporary register 6 */
	int	ss_tr7;		/* Temporary register 7 */
	int	ss_fcode;	/* Fault code for reasons other than hitting */
				/* a break instruction */
};


#define DEVICA  	"/dev/ica"

