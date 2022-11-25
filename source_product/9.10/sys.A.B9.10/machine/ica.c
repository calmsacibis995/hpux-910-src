/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/ica.c,v $
 * $Revision: 1.6.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:03:14 $
 */

#undef ICA_DEBUG
#ifdef ICA_300
static char hpux_id[]="@(#) $Revision: 1.6.84.3 $";

/***********************************************************************
 *                                                                     *
 * The function of the ICA code is to implement an Instruction         *
 * Coverage Analyzer scheme.  The purpose of it is to identify each    *
 * instruction in the kernel as executed or not executed.  This is     *
 * done by setting breakpoints in the code under test, then noting     *
 * which instructions have been executed, and replacing the            *
 * breakpoint with the original instructions.  Note that some          *
 * routines absolutely cannot be covered this way (for example,        *
 * these routines) and some may be difficult for timing or other       *
 * reasons.                                                            *
 *                                                                     *
 * Routines in this file have the purpose of getting the space         *
 * needed for this and also initializing it.  The detection and        *
 * recording of this information is done in kern_ica.c                 *
 *                                                                     *
 ***********************************************************************/

#include "../h/debug.h"
#include "../h/errno.h"
#include "../h/ioctl.h"
#include "../h/param.h"
#include "../h/vas.h"
#include "../h/debug.h"
#include "../machine/ica.h"
#include "../machine/pte.h"
#include "../machine/vmparam.h"

#ifdef ICA_DEBUG
#include <sys/reboot.h>
#endif ICA_DEBUG

/* Global variables */

extern int etext;	/* Last address of text in kernel */

extern int
	*ica_breakmap,	/* Pointer to the breakmap bitmap. */
	*ica_hitmap,	/* Pointer to the hitmap bitmap. */
	*ica_initmap,	/* Pointer to the initmap bitmap. */
	*ica_jtblmap,	/* Pointer to the jump table bitmap */
	ica_initflag,
	ica_maplength,	/* Byte length of both of the maps */
	ica_textlength; /* Length of kernel text. Measured in words */

extern short
	*ica_text,	/* pointer to beginning of kernel text */
	*ica_textcopy;	/* pointer to copy of kernel text */

#define BREAKINSTR	0x4e45	/* trap 5 instruction */

/* define values of field in compare immediate instruction that tells how long
   the immediate operand is */

#define CMPI_BSZ	0	/* immediate operand is a byte */
#define CMPI_SSZ	1	/* immediate operand is a short integer */
#define CMPI_LSZ	2	/* immediate operand is a 32-bit integer */

/*----------------------------------------------------------------------------*/

/* Ica_init allocates memory for the hitmap, the initmap, the
 * breakmap, and the copy of kernel text.  It then copies the kernel
 * text into the space set aside for that.  The hitmap and initmap
 * are initialized to zero.
 */
ica_init()
{
	register int i;
	register char *mem_p;
	extern caddr_t sys_memall();

	if ((ica_initflag == ICA_ISON) || (ica_initflag == ICA_ISOFF))
	 	return(0);

	if(ica_initflag != ICA_INITON)
		return(EAGAIN);	/* They must be trying it again (?) */

	ica_initflag = ICA_INITFAIL;
	ica_text=0;  /* I happen to know that text starts at zero (?) */
	ica_textlength = (int)(&etext)>>1; /* Bytes to instructions */
	ica_maplength = (ica_textlength+7)>>3; /* Number of bytes in bitmap */

	/* Get space for hitmap, zero it */
	if ((ica_hitmap = (int *)sys_memall(ica_maplength)) == 0) {
		return(ENOMEM);
	}

	for(mem_p = (char *)ica_hitmap;
		mem_p < (char *)ica_hitmap + ica_maplength; mem_p++)
		*mem_p = 0;

	/* Get space for initmap, zero it */
	if ((ica_initmap = (int *)sys_memall(ica_maplength)) == 0) {
		return(ENOMEM);
	}

	for(mem_p = (char *)ica_initmap;
		mem_p < (char *)ica_initmap + ica_maplength; mem_p++)
		*mem_p = 0;

	/* Get space for breakmap, zero it */
	if ((ica_breakmap = (int *)sys_memall(ica_maplength)) == 0) {
		return(ENOMEM);
	}

	for(mem_p = (char *)ica_breakmap;
		mem_p < (char *)ica_breakmap + ica_maplength; mem_p++)
		*mem_p = 0;

	/* Get space for jump table map, zero it */
	if ((ica_jtblmap = (int *)sys_memall(ica_maplength)) == 0) {
		return(ENOMEM);
	}

	for(mem_p = (char *)ica_jtblmap;
		mem_p < (char *)ica_jtblmap + ica_maplength; mem_p++)
		*mem_p = 0;

	/* Get space for text */
	if ((ica_textcopy = (short *)sys_memall(ica_textlength << 1)) == 0) {
		return(ENOMEM);
	}

	/* Copy text */
	for(i = 0; i < ica_textlength; i++) {
		ica_textcopy[i] = ica_text[i];
	}

	ica_initflag = ICA_ISOFF;
	return(0);
}

/*----------------------------------------------------------------------------*/

/*ARGSUSED*/
ica_open(dev)
	dev_t dev;
{
	return(0);
}

/*----------------------------------------------------------------------------*/

ica_close()
{
	return(0);
}

/*----------------------------------------------------------------------------*/

ica_read()
{
	return(0);
}

/*----------------------------------------------------------------------------*/

ica_write()
{
	return(0);
}

/*----------------------------------------------------------------------------*/

/* The ioctl call is currently used to start and stop ica coverage.
 *
 * When you start ica coverage, it sets all the appropriate
 * breakpoints in the code, getting its info from the initmap.  Then
 * it zeroes out the hitmap.
 *
 * When you stop ica coverage, it replaces all the breakpoints left
 * with instructions.  This prevents any of them from executing, of
 * course.
 */

/*ARGSUSED*/
ica_ioctl(dev,cmd,data,flag)
dev_t dev;
int cmd;
caddr_t data;
int flag;
{
	switch (cmd & IOCCMD_MASK) {
	case IOC_ICAINIT:	/* Get memory for textcopy,maps */
		if(ica_initflag == ICA_INITOFF) {
			ica_initflag = ICA_INITON;
		}

		return(ica_init());
		break;

	case IOC_ICASTART:  /* Add breakpoints, and clear hitmap*/
		return(ica_setbreaks());
		break;

	case IOC_ICASTOP:  /* Clear breakpoints, don't clear hitmap*/
		return(ica_clearbreaks());
		break;

	default:
		printf("ica_ioctl: unimplemented command = %d\n", cmd);
		return(EINVAL);
		break;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/

/*  Sets all the breakpoints in the kernel, based on the bits
 *  set in ica_initmap.  Only the first word of an instruction
 *  is replaced with a trap instruction.  All bits in the hit
 *  map are cleared.
 */

ica_setbreaks()
{
	unsigned int	i;		/* index of current instruction */
	int		i_lngth;	/* length of current instruction in
					   16-bit units */
	int		j;
	int		ntp;		/* number of pages of kernel text */
	int		s;
	struct pte *kpte;

	if (ica_initflag != ICA_ISOFF) return(EINVAL);

	ica_initflag = ICA_ISON;

	/* since ICA is going to modify kernel text, it must first turn
	   off the kernel text write protection */

	ntp = (ica_textlength*2 + NBPG - 1)/NBPG;

	kpte = vastopte(&kernvas, 0);
	VASSERT(kpte != NULL);
	for(j = 0; j < ntp; j++,kpte++)
		*(int *)kpte &= ~PG_PROT;

	/* figure out where all the switch statement jump tables in the
	   kernel are so we don't write over these tables with traps */

	fjtbls();

	i = 0;

	while(i < ica_textlength) {
		s = spl6();
		i_lngth = 1;

		if (wisset(ica_initmap,i) && wisclr(ica_jtblmap,i)) {
			i_lngth = inst_size(&ica_text[i])/2;

			/* print a warning if the initmap indicates that
			 * only a partial instruction is to be instrumented.
			 */

			for(j = i; j < i + i_lngth; j++)
				if (!wisset(ica_initmap,j)) {
					printf("warning: initmap specifies partial instruction, index = 0x%x\n", i);
					break;
				}

			ica_text[i] = BREAKINSTR;
			wsetbit(ica_breakmap,i);
		} else if (wisset(ica_jtblmap,i)) {
			/* the current word of text is part of a jump table.
			 * do not replace it with a trap instruction and clear
			 * the corresponding bit in the initmap so the word
			 * will not be counted as being instrumented.
			 */

			wclrbit(ica_initmap,i);
		}

		for(j = i; j < i + i_lngth; j++)
			wclrbit(ica_hitmap,j);

		i += i_lngth;
		splx(s);
	}

	return(0);
}

/*----------------------------------------------------------------------------*/

/* the break map indicates which instruction words are still replaced by
 * trap instructions.  for each bit set in the break map, replace the trap
 * instruction with the original instruction word.
 */

ica_clearbreaks()
{
	unsigned int	i;		/* index of current instruction */
	int		s;

	if (ica_initflag == ICA_ISOFF) return(0);

	if (ica_initflag != ICA_ISON) return(EINVAL);

	for(i = 0; i < ica_textlength; i++)
		if (wisset(ica_breakmap,i)) {
			s = spl6();
			ica_text[i] = ica_textcopy[i];
			wclrbit(ica_breakmap,i);
			splx(s);
		}

	ica_initflag = ICA_ISOFF;
	return(0);
}

/*----------------------------------------------------------------------------*/

/* calculate the length of a 68000 instruction in bytes.  this
 * routine was lifted with minimal changes from the source for
 * cdb.
 */

int
inst_size(adr)
unsigned int	adr;
{
	static ushort dasm[117][3] = {
	{ 0xF000,0x1000,109	},	/* mov.b */
	{ 0xF000,0x2000,29	},
	{ 0xF000,0x3000,31	},
	{ 0xF000,0x4000,33	},
	{ 0xF000,0x5000,62	},
	{ 0xF000,0x6000,70	},
	{ 0xF000,0x7000,105	},	/* movq */
	{ 0xF000,0x8000,73	},
	{ 0xF000,0x9000,97	},	/* sub's same as add's */
	{ 0xF000,0xB000,80	},
	{ 0xF000,0xC000,91	},
	{ 0xF000,0xD000,97	},
	{ 0xF000,0xE000,104	},
	{ 0xFDBF,0x003C,110	},	/* ori/andi to ccr/sr */
	{ 0xFFBF,0x0A3C,110	},	/* eori to ccr/sr */
	{ 0x0100,0x0100,25	},
	{ 0xFF00,0x0800,28	},	/* static bit */
	{ 0x00C0,0x00C0,22	},
	{ 0xFF00,0x0E00,28	},	/* movs */
	{ 0x00C0,0x0080,114	},	/* ori;andi;subi;addi;eor;cmp (long) */
	{ 0x00C0,0x0040,111	},	/* ori;andi;subi;addi;eor;cmp (word) */
	{ 0x0000,0x0004,1	},	/* ori;andi;subi;addi;eor;cmp (byte) */
	{ 0xFFF0,0x06C0,105	},	/* rtm */
	{ 0x0800,0x0800,27	},
	{ 0x0000,0x0004,1	},	/* cmp2; chk2; callm */
	{ 0xF138,0x0108,110	},	/* movp */
	{ 0x0000,0x0002,1	},	/* dynamic bit */
	{ 0xF9FF,0x08FC,113	},	/* cas2 */
	{ 0x0000,0x0004,1	},	/* cas */
	{ 0x01C0,0x0040,108	},	/* mova.l */
	{ 0x0000,0x0002,10	},	/* mov.l */
	{ 0x01C0,0x0040,103	},	/* mova.w */
	{ 0x0000,0x0002,9	},	/* mov.w */
	{ 0x0FFE,0x0E70,105	},	/* reset; nop */
	{ 0x0FFF,0x0E72,110	},	/* stop */
	{ 0x0FFF,0x0E73,105	},	/* rte */
	{ 0x0FFF,0x0E74,110	},	/* rtd */
	{ 0x0FF8,0x0E70,105	},	/* rts; trapv; rtr */
	{ 0x0FFE,0x0E7A,110	},	/* movc */
	{ 0x0FFF,0x0AFC,105	},	/* illegal */
	{ 0x0FF0,0x0E40,105	},	/* trap */
	{ 0x0FF0,0x0E50,115	},	/* patch */
	{ 0x0FF0,0x0E60,105	},	/* mov usp */
	{ 0x0FF0,0x0840,105	},	/* swap; bkpt */
	{ 0x0FF8,0x0808,113	},	/* link.l */
	{ 0x0FB8,0x0880,105	},	/* ext.w; ext.l */
	{ 0x0FF8,0x09C0,105	},	/* extb */
	{ 0x0FC0,0x00C0,103	},	/* mov from sr */
	{ 0x0F00,0x0000,107	},	/* negx */
	{ 0x0B80,0x0880,28	},	/* movm */
	{ 0x0140,0x0100,54	},
	{ 0x00C0,0x00C0,56	},
	{ 0x0F80,0x0C00,112	},	/* mulu.l; muls.l; divu.l; divs.l */
	{ 0x0000,0x0002,1	},	/* clr; neg; not; nbcd; pea; tst; jsr */
	{ 0x0080,0x0080,103	},	/* chk.w */
	{ 0x0000,0x0002,3	},	/* chk.l */
	{ 0x0100,0x0100,107	},	/* lea */
	{ 0x0F00,0x0200,107	},	/* mov from ccr */
	{ 0x0F00,0x0400,107	},	/* mov to ccr */
	{ 0x0F00,0x0600,103	},	/* mov to sr */
	{ 0x0F00,0x0A00,107	},	/* tas */
	{ 0x0F00,0x0E00,107	},	/* jmp */
	{ 0x00C0,0x00C0,64	},
	{ 0x0000,0x0002,1	},	/* addq; subq */
	{ 0x00F8,0x00C8,110	},	/* dbcc */
	{ 0x00FF,0x00FB,113	},	/* trapcc.l */
	{ 0x00FF,0x00FA,110	},	/* trapcc.w */
	{ 0x00FF,0x00FC,105	},	/* trapcc */
	{ 0x0000,0x0002,1	},	/* scc */
	{ 0x0000,0x0000,0	},	/* spare slot */
	{ 0x00FF,0x00FF,113	},	/* bra.l; bsr.l; bcc.l */
	{ 0x00FF,0x0000,110	},	/* bra.w; bsr.w; bcc.w */
	{ 0x0000,0x0002,0	},	/* bra.b; bsr.b; bcc.b */
	{ 0x00C0,0x00C0,103	},	/* divu.w; divs.w */
	{ 0x01F0,0x0100,105	},	/* sbcd */
	{ 0x01F0,0x0140,110	},	/* pack */
	{ 0x01F0,0x0180,110	},	/* unpk */
	{ 0x00C0,0x0080,108	},	/* or.l */
	{ 0x00C0,0x0040,103	},	/* or.w */
	{ 0x0000,0x0002,1	},	/* or.b */
	{ 0x00C0,0x00C0,89	},
	{ 0x0100,0x0100,85	},
	{ 0x00C0,0x0080,108	},	/* cmp.l */
	{ 0x00C0,0x0040,103	},	/* cmp.w */
	{ 0x0000,0x0002,1	},	/* cmp.b */
	{ 0x0038,0x0008,105	},	/* cmpm */
	{ 0x00C0,0x0080,108	},	/* eor.l */
	{ 0x00C0,0x0040,103	},	/* eor.w */
	{ 0x0000,0x0002,1	},	/* eor.b */
	{ 0x0100,0x0100,108	},	/* cmpa.l */
	{ 0x0000,0x0002,2	},	/* cmpa.w */
	{ 0x01B0,0x0100,105	},	/* abcd; exg dn,dn; exg an,an */
	{ 0x01F8,0x0188,105	},	/* exg dn,an */
	{ 0x00C0,0x00C0,103	},	/* mulu.w; muls.w */
	{ 0x00C0,0x0080,108	},	/* and.l */
	{ 0x00C0,0x0040,103	},	/* and.w */
	{ 0x0000,0x0002,1	},	/* and.b */
	{ 0x00C0,0x00C0,102	},
	{ 0x0130,0x0100,105	},	/* addx; subx */
	{ 0x00C0,0x0080,108	},	/* add.l; sub.l */
	{ 0x00C0,0x0040,103	},	/* add.w; sub.w */
	{ 0x0000,0x0002,1	},	/* add.b; sub.b */
	{ 0x0100,0x0100,108	},	/* adda.l; suba.l */
	{ 0x0000,0x0002,2	},	/* adda.w; suba.w */
	{ 0x00C0,0x00C0,106	},
	{ 0x0000,0x0002,0	},	/* shift/rotate register */
	{ 0x0800,0x0800,28	},	/* bit field */
	{ 0x0000,0x0002,1	},	/* shift/rotate memory */
	{ 0x0000,0x0002,3	},
	{ 0x0000,0x0002,8	},
	{ 0x0000,0x0004,0	},
	{ 0x0000,0x0004,2	},
	{ 0x0000,0x0004,3	},
	{ 0x0000,0x0006,0	},
	{ 0x0000,0x0006,3	},
	{ 0x0008,0x0008,105	},	/* unlk */
	{ 0x0000,0x0004,0	}	/* link.w */
	};

	static ushort fmovm[8] = { 4,8,8,12,8,12,12,16 };
	static ushort ftype[7] = { 3,4,6,7,2,5,1 };
	static ushort imm[11] = { 0,2,2,4,4,8,12,12,2,2,4 };
	unsigned int start;
	unsigned long count, inst, length;
	ushort high, i, low, mode, reg, size, type, wait;

	start = adr;
	high = *(ushort *)adr;
	low = *((ushort *)adr + 1);

	if ((high & 0xF000) == 0xF000) {	/* f-line instruction? */
		if ((high & 0xE00) == 0) {	/* mmu instruction? */
			/* pflush, pload, pmove, ptest */
			size = 4;
			type = 1;
		} else {
			/* floating point co-processor instruction */
			switch (high & 0x01C0) {
			case 0x0000:
				switch (low & 0xE000) {
				case 0x0000: return 4;		/* generic reg-to-reg */
				case 0x4000:
					low = (low >> 10) & 7;
	
					if (low == 7) return 4;
	
					type = ftype[low];
					size = 4;
					break;
				case 0x8000:			/* fmovm control to */
					if ((high & 0x3F) == 0x3C) 	/* immediate? */
						return fmovm[(low >> 10) & 7];
				case 0x6000: 			/* fmov from */
				case 0xA000:			/*fmov[m] control from*/
				case 0xC000:			/* fmovm to */
				case 0xE000: 			/* fmovm from */
					size = 4; type = 1; break;
				}
	
				break;
			case 0x0040:
				switch (high & 0x0038) {
				case 0x0008: return 6;		/* fdbcc */
				case 0x0038:
	
					switch (high & 0x0007) {/* ftrapcc */
					case 0x0002: return 6;
					case 0x0003: return 8;
					default: return 4;
					}
	
				default: size = 4; type = 1; break; /* fscc */
				}
	
				break;
			case 0x0080: return 4;  		/* fbcc.w */
			case 0x00C0: return 6;  		/* fbcc.l */
			case 0x0100:				/* fsave */
			case 0x0140: size = 2; type = 1; break;	/* frestore */
			default:     return 2;
			}
		}
	} else {
		i = 0;
	
		for (;;) {
			if (size = dasm[i][0]) {
				if ((size & high) == dasm[i][1]) i = dasm[i][2];
				else i++;
			} else {
				size = dasm[i][1];
				type = dasm[i][2];
				break;
			}
		}
	}

	if (type) {
		adr = start + size;
		mode = (high >> 3) & 7;
		reg = high & 7;

		for (;;) {
			if (mode == 5) size += 2;
			else if (mode >= 6) {
				if ((mode == 6) || (reg == 3)) {
					inst = *(ushort *)adr;
					size += 2;

					if (inst & 0x0100) {
						if (inst & 0x0020)
							size += inst & 0x0010 ? 4 : 2;

						if (inst & 0x0002)
							size += inst & 0x0001 ? 4 : 2;
					}
				} else {
					if ((reg & 5) == 0) size += 2;
					else if (reg == 1) size += 4;
						else size += imm[type];
				}
			}

			if (type < 8) break;

			mode = (high >> 6) & 7;
			reg = (high >> 9) & 7;
			adr = start + size;
	    		type -= 7;
		}
	}

#ifdef ICA_DEBUG
	if ( size > 12 ) (void) printf("inst_size: WARNING: instruction 0x%x at 0x%x is size 0x%x!\n", *(ushort *) start, start, size);
#endif ICA_DEBUG
	return size;
}

/*----------------------------------------------------------------------------*/

/* When a switch statement is compiled on the series 300, the
 * compiler generates a table of jump offsets that is inserted
 * in-line into the code.  ICA must be able to recognize these
 * tables so that it doesn't replace them with trap instructions.
 * The key to this recognition is that the compiler generates a
 * specific sequence of instructions right before one of these
 * tables.  This is the only place where the compiler uses a program
 * counter indexed jump so this is a safe way of finding these
 * tables.  The switch code looks like the following:
 *
 * Unoptimized switch code:
 *
 *	cmpi.{b,w,l}	%dn,&k	# note: k+1 long values in table!
 *	bhi.{b,w,l}	address
 *	mov.l		(tbladdr,%dn.(w,l)*4,%dn
 *	jmp		0x2(%pc,%dn.l)
 *	lalign 4	# possible 2-byte pad to get to a fullword boundary
 *	<k+1 long values>
 *
 * Note that the beginning of the offset table can be detected
 * because it immediately follows the jmp instruction.  The end of
 * the table can be found because the size of the table is contained
 * in the compare immediate instruction.  To complicate matters just
 * a bit, the optimizer will occasionally re-arrange the above
 * sequence to look like either of the following:
 *
 * Optimized switch code (case 1):
 *
 *	mov.l		(tbladdr,%dn.(w,l)*4,%dn
 *	jmp		0x2(%pc,%dn.l)
 *	lalign 4	# possible 2-byte pad to get to a fullword boundary
 *	<k+1 long values>
 *      <arbitrary number of instructions>
 *	cmpi.{b,w,l}	%dn,&k
 *	bls.{b,w,l}	<address of above mov.l instruction>
 *
 * Optimized switch code (case 2):
 *
 *	cmpi.{b,w,l}	%dn,&k
 *	bls.{b,w,l}	<address of below mov.l instruction>
 *      <arbitrary number of instructions>
 *	mov.l		(tbladdr,%dn.(w,l)*4,%dn
 *	jmp		0x2(%pc,%dn.l)
 *	lalign 4	# possible 2-byte pad to get to a fullword boundary
 *	<k+1 long values>
 *
 * The following routines are designed to find these jump tables.
 * The output is a bit map with a bit set for each word of the
 * kernel that is contained in one of these jump tables.
 */

fjtbls()
{
	unsigned int first_i;	/* index of first instruction in
				 * sequence currently being examined.
				 * first_i is in terms of 16-bit words.
				 */

	unsigned int cur_i;	/* index of current instruction */

	int disp;		/* bls jump displacement value */

	first_i = 0;

	while(first_i < ica_textlength) {
		if (cmp_chk(&ica_text[first_i])) {
			cur_i = first_i + inst_size(&ica_text[first_i])/2;

			if (cur_i > ica_textlength) {
				(void) printf("fjtbls: ERROR: current instruction address goes beyond ica text!\n");
				(void) printf("        cur_i is 0x%x, ica_textlength is 0x%x\n",
					cur_i, ica_textlength);
				break;
			}

			/* first_i instruction is cmpi so this may be the
			 * beginning of a normal format or an optimized case #2
			 * jump table prelude.  Check the next two
			 * instructions to see if they match either expected
			 * format.
			 */

			if (bhi_chk(&ica_text[cur_i])) {
				/* second instruction is a bhi so this is a
				 * normal format jump table prelude.
				 */
#ifdef ICA_DEBUG
				(void) printf("fjtbls: first_i points at 0x%x, cur_i points at 0x%x\n",
					first_i*2, cur_i*2);
#endif ICA_DEBUG
				normal_format(&first_i,&cur_i);
			} else {
				if ( bls_chk(&ica_text[cur_i],&disp) ) {
					/* second instruction is a bls so
					 * this is probably an optimized case #2
					 * jump table prelude.  Let's look
					 * closer and see...
					 */
#ifdef ICA_DEBUG
					(void) printf("fjtbls: first_i points at 0x%x, cur_i points at 0x%x\n",
						first_i*2, cur_i*2);
#endif ICA_DEBUG
					optimized_fmt_case2(&first_i,&cur_i,disp);
				} else {
					/* This isn't anything!  Skip forward! */
					first_i = cur_i;
				}
			}
		} else {
			if (mov_chk(&ica_text[first_i])) {
				/* This is probably an optimized case #1
				 * jump table prelude.  Let's look closer
				 * and see...
				 */
#ifdef ICA_DEBUG
				(void) printf("fjtbls: first_i points at 0x%x, cur_i points at 0x%x\n",
					first_i*2, cur_i*2);
#endif ICA_DEBUG
				optimized_fmt_case1(&first_i,&cur_i);
			} else {
				/* This instruction isn't a probable
				 * jump table prelude.  Skip forward to
				 * the next instruction...
				 */
				first_i += (inst_size(&ica_text[first_i])/2);
			}
		}
	}

	/* All kernel instructions have been scanned... */
#ifdef ICA_DEBUG
	(void) printf("fjtbls: ica_text starts at 0x%x, ica_textlength is 0x%x\n",
		&ica_text[0], ica_textlength);
	(void) printf("fjtbls: finished at location 0x%x\n", first_i*2);
#endif ICA_DEBUG
	return 0;
}

/*----------------------------------------------------------------------------*/

/* The purpose of this routine is to recognize and decode unoptimized switch
 * statement sequences of the form:
 *
 *	cmpi.{b,w,l}	%dn,&k	# note: k+1 long values in table!
 *	bhi.{b,w,l}	address
 *	mov.l		(tbladdr,%dn.(w,l)*4,%dn or %an
 *	jmp		0x2(%pc,%dn.l)  or jmp  (%an)
 *	lalign 4	# possible 2-byte pad to get to a fullword boundary
 *	<k+1 long values>
 */

normal_format(first_i,cur_i)
unsigned int *first_i, *cur_i;
{
	int i;

	int tblsiz;		/* number of 16-bit words in table */

	unsigned int tbladdr;	/* index of jump table in terms of
				 * 16-bit words
				 */

	/* move past bhi instruction... */
	*cur_i += (inst_size(&ica_text[*cur_i])/2);

	if (*cur_i > ica_textlength) {
		printf("normal_format: ERROR: current instruction address goes beyond ica text!\n");
		printf("               cur_i is 0x%x, ica_textlength is 0x%x\n",
			*cur_i, ica_textlength);
		return -1;
	}

	/* is this a mov.l instruction? */
	if (!mov_chk(&ica_text[*cur_i])) {
#ifdef ICA_DEBUG
		(void) printf("normal_format: mov.l not found -- bailout at 0x%x\n",
			*cur_i*2);
#endif ICA_DEBUG
		*first_i = *cur_i;
		return -1;
	}

	/* yes, move past mov.l instruction... */
	*cur_i += (inst_size(&ica_text[*cur_i])/2);

	if (*cur_i > ica_textlength) {
		printf("normal_format: ERROR: current instruction address goes beyond ica text!\n");
		printf("               cur_i is 0x%x, ica_textlength is 0x%x\n",
			*cur_i, ica_textlength);
		return -1;
	}

	/* is this a jmp instruction? */
	if (!jmp_chk(&ica_text[*cur_i]) && !jmp_reg_chk(&ica_text[*cur_i])) {
#ifdef ICA_DEBUG
		(void) printf("normal_format: jmp not found -- bailout at 0x%x\n",
			*cur_i*2);
#endif ICA_DEBUG
		*first_i = *cur_i;
		return -1;
	}

	/* Yes, all instructions have matched so we have found
	 * a jump table.  Mark the bits corresponding to the
	 * table addresses in the bit map.
	 */

#ifdef ICA_DEBUG
	(void) printf("normal_format: switch code recognized at 0x%x\n", *first_i*2);
#endif ICA_DEBUG
	tbladdr = *cur_i + inst_size(&ica_text[*cur_i])/2;
	tblsiz = get_tblsiz(&ica_text[*first_i]);

	/* if jump table doesn't start on full word
	 * boundary, the compiler inserts a 2-byte pad
	 * to achieve alignment.
	 */

#ifdef ICA_DEBUG
	if (tbladdr % 2 != 0) {
		tblsiz += 1;
		(void) printf("normal_format: pad aligned jump table starts at 0x%x and is %1d words long\n", tbladdr*2, tblsiz/2);
	} else  (void) printf("normal_format: standard aligned jump table starts at 0x%x and is %1d words long\n", tbladdr*2, tblsiz/2);
#else
	if (tbladdr % 2 != 0) tblsiz += 1;
#endif ICA_DEBUG

	for (i = tbladdr; i < tbladdr + tblsiz; i++)
		wsetbit(ica_jtblmap,i);

	/* start next iteration of loop with instruction
	 * immediately following jump table
	 */

	*first_i = tbladdr + tblsiz;
	return 0;
}

/*----------------------------------------------------------------------------*/

/* The purpose of this routine is to recognize and decode optimized switch 
 * statement sequences like the following:
 *
 *	mov.l		(tbladdr,%dn.(w,l)*4,%dn
 *	jmp		0x2(%pc,%dn.l)
 *	lalign 4	# possible 2-byte pad to get to a fullword boundary
 *	<k+1 long values>
 *      <arbitrary number of instructions>
 *	cmpi.{b,w,l}	%dn,&k
 *	bls.{b,w,l}	<address of above mov.l instruction>
 */

optimized_fmt_case1(first_i,cur_i)
unsigned int *first_i, *cur_i;
{
	int found;		/* 1=jump table found/0=not found */
	int i;

	int tblsiz;		/* number of 16-bit words in table */

	unsigned int tbladdr;	/* index of jump table in terms of
				 * 16-bit words
				 */

	unsigned int startaddr;	/* remember address of first word of
				 * sequence when dealing with code
				 * rearranged by optimizer
				 */

	int disp;		/* bls displacement value */

	/* first instruction of sequence is a mov.l so let's
	 * check to see if this is an instance where
	 * the optimizer rearranged the instructions.
	 */

	*cur_i = *first_i + inst_size(&ica_text[*first_i])/2;

	if (*cur_i > ica_textlength) {
		(void) printf("optimized_fmt_case1: ERROR: current instruction address goes beyond ica text!\n");
		(void) printf("                     cur_i is 0x%x, ica_textlength is 0x%x\n",
			*cur_i, ica_textlength);
		return -1;
	}

	/* is this a jmp instruction? */

	if (!jmp_chk(&ica_text[*cur_i])) {
#ifdef ICA_DEBUG
		(void) printf("optimized_fmt_case1: jmp not found -- bailout at 0x%x\n",
			*cur_i*2);
#endif ICA_DEBUG
		*first_i = *cur_i;
		return -1;
	}

	/* we've got the start of the table! */

	tbladdr = *cur_i + inst_size(&ica_text[*cur_i])/2;

	/* Start looking for the compare instruction that gives
	 * the size of the table relative to the beginning of the table
	 * itself.  The instruction will not be in the table,
	 * of course, but we don't know where the table ends
	 * so we have to start somewhere.
	 */

	*cur_i = tbladdr;
	found = 0;

	while (*cur_i < ica_textlength) {

		/* is this a cmpi instruction? */

		if (!cmp_chk(&ica_text[*cur_i])) {
			/* since we don't know whether we are
			   examining instructions or data,
			   advance offset by one word */
			*cur_i += 1;
			continue;
		}

		startaddr = *cur_i;	/* remember address of cmpi! */

		/* Yes!  move past cmpi instruction... */
		*cur_i += (inst_size(&ica_text[*cur_i])/2);

		if (*cur_i > ica_textlength) {
			printf("optimized_fmt_case1: ERROR: current instruction address goes beyond ica text!\n");
			printf("                     cur_i is 0x%x, ica_textlength is 0x%x\n",
				*cur_i, ica_textlength);
			return -1;
		}

		/* is this a bls instruction? */

		if ( bls_chk(&ica_text[*cur_i],&disp) ) {
			/* bls address point to the original mov.l? */
			if ( !bls_dest_chk(&ica_text[*cur_i],&disp,&ica_text[*first_i]) ) {
				/* pc+disp+2 doesn't match mov.l address! */
				*cur_i = startaddr + 1;
				continue;
			}
		} else {
			*cur_i = startaddr + 1;
			continue;
		}

		/* Yes!  We've found the sequence at the end of the table... */

		found = 1;
		tblsiz = get_tblsiz(&ica_text[startaddr]);
#ifdef ICA_DEBUG
		(void) printf("optimized_fmt_case1: switch code recognized at 0x%x\n", *first_i*2);
		(void) printf("                     cmpi recognized at 0x%x\n",
			startaddr*2);
#endif ICA_DEBUG

		/* check to see if the table begins with a pad */

#ifdef ICA_DEBUG
		if (tbladdr % 2 != 0) {
			tblsiz += 1;
			(void) printf("                     pad aligned jump table starts at 0x%x and is %1d words long\n", tbladdr*2, tblsiz/2);
		} else  (void) printf("                     standard aligned jump table starts at 0x%x and is %1d words long\n", tbladdr*2, tblsiz/2);
#else
		if (tbladdr % 2 != 0) tblsiz += 1;
#endif ICA_DEBUG

		for (i = tbladdr; i < tbladdr + tblsiz; i++)
			wsetbit(ica_jtblmap,i);

		/* Make sure we back up to instruction after the jump table.
		   There may be more jump tables between it and the bls and
		   cmpi stmts. */
		*first_i = tbladdr + tblsiz;
		return 0;
	}

	/* we searched 'til the end of text and didn't find a cmpi/bls pair! */
	(void) printf("optimized_fmt_case1: apparent optimized switch code at 0x%x\n", *first_i*2);
	(void) printf("optimized_fmt_case1: ERROR: couldn't find size of table at 0x%x\n", tbladdr*2);
	return -1;
}

/*----------------------------------------------------------------------------*/

/* The purpose of this routine is to recognize optimized switch statement
 * sequences like the following:
 *
 *	cmpi.{b,w,l}	%dn,&k
 *	bls.{b,w,l}	<address of below mov.l instruction>
 *      <arbitrary number of instructions>
 *	mov.l		(tbladdr,%dn.(w,l)*4,%dn
 *	jmp		0x2(%pc,%dn.l)
 *	lalign 4	# possible 2-byte pad to get to a fullword boundary
 *	<k+1 long values>
 */

optimized_fmt_case2(first_i,cur_i,disp)
unsigned int *first_i, *cur_i;
int disp;
{
	int i;

	int tblsiz;		/* number of 16-bit words in table */

	unsigned int tbladdr;	/* index of jump table in terms of
				 * 16-bit words
				 */

	unsigned int bls_addr;	/* remember address of first word of
				 * the bls when dealing with code
				 * rearranged by optimizer
				 */

	/* Let's check to see if the target of the bls points to a mov.l
	 * instruction!
	 */

	bls_addr = *cur_i;		/* remember address of bls */
	*cur_i = *cur_i + (disp + 2)/2;	/* get effective jump address */

	/* Check and make sure we are not going backwards and picking up
	   an optimized case 1 jump table */
	if (*cur_i < bls_addr) {
		*first_i = bls_addr;
		return -1;
	}

	if (*cur_i > ica_textlength) {
		printf("optimized_fmt_case2: ERROR: current instruction address goes beyond ica text!\n");
		printf("                     cur_i is 0x%x, ica_textlength is 0x%x\n",
			*cur_i, ica_textlength);
		return -1;
	}

	/* is this a mov.l instruction? */

	if (!mov_chk(&ica_text[*cur_i])) {
#ifdef ICA_DEBUG
		(void) printf("optimized_fmt_case2: mov.l not found -- bailout at 0x%x\n",
			*cur_i*2);
#endif ICA_DEBUG
		*first_i = bls_addr;	/* back up to bls! */
		return -1;
	}

	/* move past mov.l instruction... */
	*cur_i += (inst_size(&ica_text[*cur_i])/2);

	if (*cur_i > ica_textlength) {
		printf("optimized_fmt_case2: ERROR: current instruction address goes beyond ica text!\n");
		printf("                     cur_i is 0x%x, ica_textlength is 0x%x\n",
			*cur_i, ica_textlength);
		return -1;
	}

	/* is this a jmp instruction? */

	if ( !jmp_chk(&ica_text[*cur_i]) ) {
#ifdef ICA_DEBUG
		(void) printf("optimized_fmt_case2: jmp not found -- bailout at 0x%x\n",
			*cur_i*2);
#endif ICA_DEBUG
		*first_i = bls_addr;	/* back up to bls! */
		return -1;
	}

	/* yes, move past jmp instruction... */
	*cur_i += (inst_size(&ica_text[*cur_i])/2);

#ifdef ICA_DEBUG
	(void) printf("optimized_fmt_case2: switch code recognized at 0x%x\n",
		*first_i*2);
#endif ICA_DEBUG
	tbladdr = *cur_i + inst_size(&ica_text[*cur_i])/2;

	/* The original cmpi.{b,w,l} instruction gave the size of
	 * the table relative to the start of the table itself, so
	 * get the table size from there!
	 */
	tblsiz = get_tblsiz(&ica_text[*first_i]);

	/* check to see if the table begins with a pad */

#ifdef ICA_DEBUG
	if (tbladdr % 2 != 0) {
		tblsiz += 1;
		(void) printf("        pad aligned jump table starts at 0x%x and is %1d words long\n", tbladdr*2, tblsiz/2);
	} else  (void) printf("        standard aligned jump table starts at 0x%x and is %1d words long\n", tbladdr*2, tblsiz/2);
#else
	if (tbladdr % 2 != 0) tblsiz += 1;
#endif ICA_DEBUG

	for(i = tbladdr; i < tbladdr + tblsiz; i++)
		wsetbit(ica_jtblmap,i);

	*first_i = tbladdr + tblsiz;
	return 0;
}

/*----------------------------------------------------------------------------*/

/* the following routines check instructions to see if they match the expected
   instructions in the prelude to a jump table. */

bhi_chk(i_addr)
short	*i_addr;		/* address of first word of instruction */
{
	if ((*i_addr & 0xff00) == 0x6200) {
		return(1);
	} else
		return(0);
}

/*----------------------------------------------------------------------------*/

cmp_chk(i_addr)
short	*i_addr;		/* address of first word of instruction */
{
	if ((*i_addr & 0xff38) == 0xc00) {
		return(1);
	} else
		return(0);
}

/*----------------------------------------------------------------------------*/

mov_chk(i_addr)
short	*i_addr;		/* address of first word of instruction */
				/* Checks for Address or data reg as dest. */
{
	if (((*i_addr & 0xf1bf) == 0x2030) &&
	    (((*(i_addr + 1)) & 0x87ff) == 0x5b0)) {
		return(1);
	} else
		return(0);
}

/*----------------------------------------------------------------------------*/

jmp_chk(i_addr)
short	*i_addr;		/* address of first word of instruction */
{
	if ((*i_addr == 0x4efb) && ((*(i_addr + 1)) & 0x8fff) == 0x802) {
		return(1);
	} else
		return(0);
}

/*----------------------------------------------------------------------------*/

jmp_reg_chk(i_addr)
short	*i_addr;		/* address of first word of instruction */
{
				/* This looks for jmp (%an) */
	if ((*i_addr & 0xfff8) == 0x4ed0) {
		return(1);
	} else
		return(0);
}

/*----------------------------------------------------------------------------*/

/* This routine checks to see if the instruction is a Branch Less or Same.
 * It then returns the two's-complement-encoded jump displacement value in
 * "disp".  Note that the effective jump address is pc+disp+2 as defined by
 * the 68000 architecture!
 */

bls_chk(i_addr,disp)
short *i_addr;	/* address of first word of instruction */
int *disp;	/* jump displacement value (taken from instruction) */
{
	char disp_field;	/* instruction's displacement field */

	if ( (*i_addr & 0xff00) != 0x6300 ) return(0);	/* not a bls! */

	/* extract displacement value from instruction.  displacement can
	   be 8, 16, or 32 bits. */

	disp_field = *i_addr & 0xff;

	/* examine the 8-bit displacement value to determine the actual
	 * displacement size (8, 16, or 32 bits).
	 */
	if (disp_field == 0xff) *disp = *((int *)(i_addr + 1));	/* 32-bit */
	else if (disp_field == 0) *disp = *(i_addr + 1);	/* 16-bit */
		else *disp = disp_field;			/*  8-bit */

	return 1;
}

/*----------------------------------------------------------------------------*/

/* This routine checks to see that the calculated bls effective address
 * matches the expected jump address!
 */

bls_dest_chk(i_addr,disp,dest)
short *i_addr;
int *disp;
short *dest;
{
	/* the addition of 2 is to account for the fact that the pc points
	 * to the second word of the instruction when the destination
	 * computation is done
	 */

	if ( ((char *)i_addr + *disp + 2) == (char *)dest ) return(1);
	else return(0);
}

/*----------------------------------------------------------------------------*/

/* extract the size of the jump table from the compare immediate instruction
   that checks the table index against the bounds of the table. */

get_tblsiz(i_addr)
short	*i_addr;		/* address of first word of instruction */
{
	int	op_size;	/* indicates size of immediate operand */
	int	tblsiz;

	/* extract the field from the instruction that tells how big the
	   immediate operand is */

	op_size = (*i_addr & 0xc0) >> 6;
	if (op_size == CMPI_BSZ)
		tblsiz = (*(i_addr + 1)) & 0xff;
	else if (op_size == CMPI_SSZ)
		tblsiz = *(i_addr + 1);
	else
		tblsiz = *((int *)(i_addr + 1));

	/* convert instruction argument to 16-bit words in table */

	tblsiz = (tblsiz + 1) * 2;
	return(tblsiz);
}
#ifdef ICA_DEBUG

/*----------------------------------------------------------------------------*/

reboot_vs_panic()
{
	reboot(RB_HALT);
	return 0;
}
#endif ICA_DEBUG
#endif ICA_300
