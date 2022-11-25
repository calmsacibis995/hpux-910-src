/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/reg.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:07:52 $
 */
/* @(#) $Revision: 1.3.84.3 $ */       
#ifndef _MACHINE_REG_INCLUDED /* allow multiple inclusions */
#define _MACHINE_REG_INCLUDED


/*
 * Location of the users' stored
 * registers relative to SFC.
 * SFC and DFC are the source and destination
 * function code registers respectively.
 * R0-R7 are the data registers.
 * AR0-AR7 are the address registers.
 * RPS is the status register (long)
 * PC is the program counter
 *
 * Usage is u.u_ar0[XX].
 */

#define	R0	0
#define	R1	1
#define	R2	2
#define	R3	3
#define	R4	4
#define	R5	5
#define	R6	6
#define	R7	7
#define	AR0     8
#define	AR1     9
#define	AR2     10
#define	AR3	11
#define	AR4	12
#define	AR5	13
#define	AR6	14
#define	AR7	15
#define	SP	15

/*
 * Location of the users' stored
 * floating point registers (float card and mc68881).
 *
 * FERRBIT is the float card errbit register.
 * FSTATUS is the float card status register.
 * FR0-FR7 are the float card floating point registers.
 * Usage is u.u_pcb.pcb_float[XX].
 *
 * FMC68881_C is the mc68881 control register (32 bit).
 * FMC68881_S is the mc68881 status register (32 bit).
 * FMC68881_I is the mc68881 instruction address register (32 bit).
 * FMC68881_R0-FMC68881_R7 are the mc68881 floating point registers (96 bit).
 * Usage is u.u_pcb.pcb_mc68881[XX].
 * Note that the MC68881 float registers are 96 bits wide (12 bytes).
 */

#define FERRBIT	0
#define FSTATUS	1
#define FR0	2
#define FR1	3
#define FR2	4
#define FR3	5
#define FR4	6
#define FR5	7
#define FR6	8
#define FR7	9

#define FMC68881_C 	54
#define FMC68881_S	55
#define FMC68881_I	56
#define FMC68881_R0	57
#define FMC68881_R1	60
#define FMC68881_R2	63
#define FMC68881_R3	66
#define FMC68881_R4	69
#define FMC68881_R5	72
#define FMC68881_R6	75
#define FMC68881_R7	78

/*	Dragon register offsets from start of u.u_pcb.pcb_dragon_regs[XX].
 *	Only valid for core dump image .
 * 	Usage is u.u_pcb.pcb_dragon_regs[XX].
 *	Dragon sr/cr should be accessed via u.u_pcb.pcb_dragon_sr and
 *	u.u_pcb.pcb_dragon_cr respectively.
 */

#define DRAGON_FPA0	0
#define DRAGON_FPA1	2
#define DRAGON_FPA2	4
#define DRAGON_FPA3	6
#define DRAGON_FPA4	8
#define DRAGON_FPA5	10
#define DRAGON_FPA6	12
#define DRAGON_FPA7	14
#define DRAGON_FPA8	16
#define DRAGON_FPA9	18
#define DRAGON_FPA10	20
#define DRAGON_FPA11	22
#define DRAGON_FPA12	24
#define DRAGON_FPA13	26
#define DRAGON_FPA14	28
#define DRAGON_FPA15	30


#endif /* _MACHINE_REG_INCLUDED */
