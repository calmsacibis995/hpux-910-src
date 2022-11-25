/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/dragon.h,v $
 * $Revision: 1.4.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:12:19 $
 */
/* HPUX_ID: @(#)dragon.h	46.1		87/03/30 */
#ifndef _MACHINE_DRAGON_INCLUDED /* allows multiple inclusion */
#define _MACHINE_DRAGON_INCLUDED

#define	DRAGON_PRIM_ID	10
#define	DRAGON_SECD_ID	2		
#define	DRAGON_ID	0x4A
#define	DRAGON_INT_LVL	3
#define DRAGON_IE	0x80
#define DRAGON_IR	0x40
#define DRAGON_MAXBANK  32
#define DRAGON_STATUS_V 0
#define DRAGON_CNTRL_V  0x7500	
#define DRAGON_DIO_ADDR (0x5c0000 + LOG_IO_OFFSET)
#define	DRAGON_BANK_ADDR	(0x5c0007 + LOG_IO_OFFSET)
#define DRAGON_STATUS_ADDR	(0x5c0014 + LOG_IO_OFFSET)
#define DRAGON_CNTRL_ADDR	(0x5c0018 + LOG_IO_OFFSET)
#define DRAGON_BUSY		(0x5c0010 + LOG_IO_OFFSET)

#define DRAGON_REG0H	0x1018		/* offset into the dio1 dragon base */
#define DRAGON_REG0L	0x101C
#define DRAGON_REG1H	0x1118
#define DRAGON_REG1L	0x111C
#define DRAGON_REG2H	0x1218
#define DRAGON_REG2L	0x121C
#define DRAGON_REG3H	0x1318
#define DRAGON_REG3L	0x131C
#define DRAGON_REG4H	0x1418
#define DRAGON_REG4L	0x141C
#define DRAGON_REG5H	0x1518
#define DRAGON_REG5L	0x151C
#define DRAGON_REG6H	0x1618
#define DRAGON_REG6L	0x161C
#define DRAGON_REG7H	0x1718
#define DRAGON_REG7L	0x171C
#define DRAGON_REG8H	0x1818
#define DRAGON_REG8L	0x181C
#define DRAGON_REG9H	0x1918
#define DRAGON_REG9L	0x191C
#define DRAGON_REGAH	0x1A18
#define DRAGON_REGAL	0x1A1C
#define DRAGON_REGBH	0x1B18
#define DRAGON_REGBL	0x1B1C
#define DRAGON_REGCH	0x1C18
#define DRAGON_REGCL	0x1C1C
#define DRAGON_REGDH	0x1D18
#define DRAGON_REGDL	0x1D1C
#define DRAGON_REGEH	0x1E18
#define DRAGON_REGEL	0x1E1C
#define DRAGON_REGFH	0x1F18
#define DRAGON_REGFL	0x1F1C

/* memory layout of dragon card */
struct dragon_mmap_type {				/* address */
	unsigned char byte0;		/*   0	   */
	unsigned char id_reg;		/*   1 	   */
	unsigned char byte2;		/*   2 	   */
	unsigned char intr_control;	/*   3 	   */
	unsigned char byte4;		/*   4	   */
	unsigned char byte5;		/*   5	   */
	unsigned char byte6;		/*   6	   */
	unsigned char bank_number;	/*   7	   */
	unsigned short	dio2_addr;	/*   8,9   upper 16 bits only */
	unsigned char bytea_f[6];	/*   a-f   */
	unsigned char busy;		/*   10    bit 7 */
	unsigned char byte11_15[3];	/*   11-13 */
	unsigned int  statusreg;	/*   14    fpsr  equivalence */
	unsigned int  controlreg;	/*   18    fpcr  equivalence */
};

#endif /* _MACHINE_DRAGON_INCLUDED */
