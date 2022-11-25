/* @(#) $Revision: 66.1 $ */   

/****************************************************************************

	DEBUGGER

****************************************************************************/
#include "defs.h"
/* #include <sys/ptrace.h>	moved to defs.h - SMT */

int		pid;
PROC_REGS	*cregs;

int offset[] = { FERRBIT, FSTATUS, FR0, FR1, FR2, FR3, FR4, FR5, FR6, FR7 };
 /*
 *
 * FMC68881_C is the mc68881 control register (32 bit).
 * FMC68881_S is the mc68881 status register (32 bit).
 * FMC68881_I is the mc68881 instruction address register (32 bit).
 * FMC68881_R0-FMC68881_R7 are the mc68881 floating point registers (96 bit).
 * Usage is u.u_pcb.pcb_mc68881[XX].
 * Note that the MC68881 float registers are 96 bits wide (12 bytes).
 */

int offset1[] = { FMC68881_C, FMC68881_S, FMC68881_I };
int offset2[] = { FMC68881_R0, FMC68881_R1, FMC68881_R2, FMC68881_R3, FMC68881_R4, FMC68881_R5, FMC68881_R6, FMC68881_R7};

#pragma OPT_LEVEL 1

/* convert extended precision to double */
double cxtod(w1, w2, w3)
long w1, w2, w3;
{
	asm ("  fmov.x 8(%a6),%fp0 ");	/* short 0xF22E,0x4800,0x0008 */
	asm ("  fmov.d %fp0,-(%sp) ");	/* short 0xF227,0x7400 */
	asm ("  mov.l  (%a7)+,%d0  ");
	asm ("  mov.l  (%a7)+,%d1  ");
}

#pragma OPT_LEVEL 2

printfloats()
{
	register int i, j;
	int data[10];
	int data1[3];
	int data2[8][3];
	union
	{   double dragon_data[17];
	    struct
	    {   int dragon_regs[32];
		int dragon_sr;
		int dragon_cr;
	    } dreg;
	} dragon_stuff;
	float *f_dragon_data;
	register int	base;

	if (float_soft && (umm_flag || w310_flag)) 
	{
		printf("No hardware float card\n");
		return;
	}

	if (!float_soft)
	{
		base = (int) ((struct user *)0)->u_pcb.pcb_float;
		for (i = 0; i <= 9; i++) 
			data[i] = pid ? ptrace(PT_RUAREA, pid, base + offset[i] * sizeof(int) , 0)
				      : *(int *)(((char *)(cregs->hw_regs.p_float)) + offset[i] * sizeof(int));
		printf("\tFloat card registers\n");
		printf("fs  %X", data[1]);
		while (charpos() % 22) printc(' ');
		printf("fe  %X\n", data[0]);
		for (i = 2; i <= 5; i++) 
		{
			printf("%s%d  %X", "f", i-2, data[i]);
			while (charpos() % 22) printc(' ');
			printf("%s%d  %X\n", "f", i+2, data[i+4]);
		}
	}

	if(umm_flag || w310_flag) return;

	/* WOPR kernel -- mc68020 processor */

	if (!mc68881)
	{
		printf("No Floating-point Coprocessor\n");
		goto display_dragon;
	}

	base = (int) ((struct user *)0)->u_pcb.pcb_mc68881;
	for (i = 0; i <= 2; i++) 
		if (pid) 
		   data1[i] = ptrace(PT_RUAREA, pid, base + offset1[i] * sizeof(int) , 0);
		else
		   data1[i] = *(int *)(((char *)cregs->hw_regs.mc68881) + offset1[i]*sizeof(int));

	for (i = 0; i <= 7; i++) 
	    for (j = 0; j <= 2; j++) 
		if (pid) 
		   data2[i][j] = ptrace(PT_RUAREA, pid, base + (offset2[i] +j) * sizeof(int) , 0);
		else
		   data2[i][j] = *(int *)(((char *)cregs->hw_regs.mc68881) + (offset2[i] +j) * sizeof(int));

	printf("\t\t\tMC68881 registers\n");
	printf("fpsr  %X", data1[1]);
	while (charpos() % 22) printc(' ');
	printf("fpcr  %X", data1[0]);
	while (charpos() % 44) printc(' ');
	printf("fpiar  %X\n", data1[2]);

	for (i = 0; i <= 7; i++) 
	{
		printf("%s%d  %8X %8X %8X\t", "fp", i, data2[i][0], data2[i][1], data2[i][2]);
		printdoub(cxtod(data2[i][0], data2[i][1], data2[i][2]), 13);
		printc('\n');
	}

display_dragon:

	if (Dragon_flag)
	{
		if (pid)
		{
			if(ptrace(PT_RFPREGS, pid, dragon_stuff.dragon_data, 0)== -1) return;
		}
		else /* look at core file */
		{
			/* see if process uses dragon */
			if (cregs->hw_regs.dragon_bank == -1) return;
			dragon_stuff.dreg.dragon_sr = cregs->hw_regs.dragon_sr;
			dragon_stuff.dreg.dragon_cr = cregs->hw_regs.dragon_cr;
			base = (int) cregs->hw_regs.dragon_regs;
			for(i=0; i < 16; i++)
			{
			  dragon_stuff.dragon_data[i] = *(double *)base;
			  base += 8;
			}
		}

		f_dragon_data = (float *) dragon_stuff.dragon_data;
		printf("\n\t\tFloating Point Accelerator registers\n");
		printf("fpasr  %X", dragon_stuff.dreg.dragon_sr);
		while (charpos() % 42) printc(' ');
		printf("fpacr  %X\n\n", dragon_stuff.dreg.dragon_cr);
		printf("Register");
		while (charpos() % 12) printc(' ');
		printf("Single Precision");
		while (charpos() % 42) printc(' ');
		printf("Double Precision\n");

		for (i=0; i<16; i++)
		{
			printf(" fpa%d", i);
			while (charpos() % 13) printc(' ');
			printdoub(f_dragon_data[2*i],6);
			while (charpos() % 40) printc(' ');
			printdoub(dragon_stuff.dragon_data[i],13);
			printc('\n');
		}
	}

}
