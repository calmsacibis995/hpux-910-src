/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/cpu.h,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:03:07 $
 */
/* HPUX_ID: @(#)cpu.h	52.1		88/04/19 */

#ifndef _MACHINE_CPU_INCLUDED /* allow multiple inclusions */
#define _MACHINE_CPU_INCLUDED


/* NOTE: This file is included by locore.s, surround declarations,
 * structures, unions, typedefs or anything else you don't want included
 * by locore.s with "#ifndef LOCORE" - "#endif"
 */


/*
 * Symbolic values for "processor" global variable set in locore.s
 */


#define M68010	0x00
#define M68020	0x01
#define M68030	0x02
#define M68040	0x03


#ifndef LOCORE
extern short machine_model;

/*
 * HAS_DIO_II is true if the machine has DIO II bus.
 */
#define HAS_DIO_II ((machine_model != MACH_MODEL_310) && (machine_model != MACH_MODEL_320))

#endif /* not LOCORE */

/*
 * The MACH_MODEL_* defines are symbolic constants for each model stored in
 * the global variable "machine_model".  The high byte is the same as the
 * M680X0 define for the processor.  These are psuedo-unique values,
 * because the 318 & 319 appear as a 330 to the software.
 */
#define MACH_MODEL_310		0x0000
#define MACH_MODEL_320		0x0100
#define MACH_MODEL_330		0x0101
#define MACH_MODEL_318  	MACH_MODEL_330
#define MACH_MODEL_319  	MACH_MODEL_330
#define MACH_MODEL_332		0x0205
#define MACH_MODEL_340		0x0206
#define MACH_MODEL_350		0x0102
#define MACH_MODEL_360		0x0203
#define MACH_MODEL_370		0x0204
#define MACH_MODEL_375		0x0207
#define MACH_MODEL_345		0x0308
#define MACH_MODEL_380		0x0309
#define MACH_MODEL_385		0x030A
#define MACH_MODEL_40T		0x030B
#define MACH_MODEL_42T		0x030C
#define MACH_MODEL_43T		0x030D
#define MACH_MODEL_40S		0x030E
#define MACH_MODEL_42S		0x030F
#define MACH_MODEL_43S		0x0310
#define MACH_MODEL_WOODY25	0x0311
#define MACH_MODEL_WOODY33	0x0312
#define MACH_MODEL_MACE25	0x0313
#define MACH_MODEL_MACE33	0x0314
#define MACH_MODEL_UNKNOWN	0xffff


/* Note: KSTACKBASE must be on a boundary that is
 *       divisible by the minimum page table size.
 *       The resume code in locore depends on the
 *       kernel stack and uarea pte's being contiguous
 */

#define KSTACKBASE       0x00900000
#define KSTACKADDR      (KSTACKBASE + KSTACK_PAGES*NBPG)
#define KUAREABASE      KSTACKADDR
#define GENMAPSPACE     (KUAREABASE + UPAGES*NBPG)    /* laddr of general mapping area */

#define	LOGICAL_IO_BASE	0x20000000

#define	PHYS_IO_BASE	0x00200000

#define LOG_IO_OFFSET	(LOGICAL_IO_BASE - PHYS_IO_BASE)
#define	INTIOPAGES	(2048)		/* io space is 8Mb or 2048 4k pages */

/* UMM/WOPR status and control register bits */
#define MMU_UMEN     0x0001
#define MMU_SMEN     0x0002
#define MMU_CEN      0x0004
#define MMU_BERR     0x0008

/* WOPR only status bits */
#define MMU_IEN      0x0020
#define MMU_FPE      0x0040
#define MMU_WPF      0x2000
#define MMU_PF       0x4000
#define MMU_PTF      0x8000

/* PMMU bits in PMMU status register */
#define PMMU_B  0x80000000      /* Bus error */
#define PMMU_W  0x08000000      /* Write protect fault */
#define PMMU_I  0x04000000      /* Invalid PTE/STE */
#define PMMU_N  0x00070000      /* Level of table walk */

/* fields in the 68020 instruction cache control register */
#define IC_CLR       0x0008     /* clear entire cache */
#define IC_CE        0x0004     /* clear cache entry  */
#define IC_FREEZE    0x0002     /* freeze cache       */
#define IC_ENABLE    0x0001     /* enable cache       */

/* fields in the 68030 cache control register */
#define IC_BURST     0x0010     /* enable burst cache fills     */
#define DC_CLR       0x0800     /* clear entire cache           */
#define DC_CE        0x0400     /* clear cache entry		*/
#define DC_FREEZE    0x0200     /* freeze cache                 */
#define DC_ENABLE    0x0100     /* enable cache                 */
#define DC_BURST     0x1000     /* enable burst cache fills     */
#define DC_WA        0x2000     /* enable data cache write allocate */

/* MC68040 cache control register (CACR) bits for controlling on-chip caches */
#define MC68040_DC_ENABLE       0x80000000      /* enable dcache */
#define MC68040_IC_ENABLE       0x00008000      /* enable icache */

/* physical addresses of the CPU registers */
#define	CPU_SRP_PADDR		(0x5F4000)
#define	CPU_URP_PADDR		(0x5F4004)
#define	CPU_TLBPURGE_PADDR	(0x5F400A)
#define	CPU_STATUS_PADDR	(0x5F400E)
#define	CPU_TIMER_PADDR		(0x5F8000)
#define	MMU_FAULT_PADDR		(0x5FC000)
#define	MMU_BASE_PADDR		(0x5F4000)

/* logical addresses of the CPU registers */
#define	CPU_SRP_REG		(CPU_SRP_PADDR + LOG_IO_OFFSET)
#define	CPU_URP_REG		(CPU_URP_PADDR + LOG_IO_OFFSET)
#define	CPU_TLBPURGE_REG	(CPU_TLBPURGE_PADDR + LOG_IO_OFFSET)
#define	CPU_STATUS_REG		(CPU_STATUS_PADDR + LOG_IO_OFFSET)
#define	CPU_TIMER_REG		(CPU_TIMER_PADDR + LOG_IO_OFFSET)
#define	MMU_FAULT_ADDRESS	(MMU_FAULT_PADDR + LOG_IO_OFFSET)
#define	MMU_BASE		(MMU_BASE_PADDR + LOG_IO_OFFSET)

/* addresses for the internal DMA controller */
#define DMA_CARD_PADDR		(0x500000)
#define DMA_CARD_BASE		(DMA_CARD_PADDR +  LOG_IO_OFFSET)

/* addresses for the memory controller */
#define MEM_CTLR_PADDR		(0x5B0000)
#define MEM_CTLR_BASE		(MEM_CTLR_PADDR + LOG_IO_OFFSET)

/* addresses for the internal displays */
#define	DISPLAY_HI_RES_PADDR	(0x560000)
#define	DISPLAY_LO_RES_PADDR	(0x530000)
#define	DISPLAY_HI_RES		(DISPLAY_HI_RES_PADDR + LOG_IO_OFFSET)
#define	DISPLAY_LO_RES		(DISPLAY_LO_RES_PADDR + LOG_IO_OFFSET)

/* addresses for the EEPROM hidden registers */
#define HIDDEN_REG_PADDR        (0x400011)
#define HIDDEN_REG_EXTENT_PADDR (0x40003F)
#define HIDDEN_REG_BASE         ((unsigned char *)(HIDDEN_REG_PADDR + LOG_IO_OFFSET))
#define HIDDEN_REG_EXTENT       (((unsigned char *)HIDDEN_REG_EXTENT_PADDR + LOG_IO_OFFSET))

/* UMM/WOPR cpu board register addresses */

#define SSEGPTR     (MMU_BASE +0x00)
#define USEGPTR     (MMU_BASE +0x04)
#define PTLB        (MMU_BASE +0x08)
#define TLBSELP     (MMU_BASE +0x08)
#define MMU_CONTROL (MMU_BASE +0x0c)


#endif /* _MACHINE_CPU_INCLUDED */
