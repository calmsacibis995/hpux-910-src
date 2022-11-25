/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/trap.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:08:48 $
 */
/* @(#) $Revision: 1.3.84.3 $ */      
#ifndef _MACHINE_TRAP_INCLUDED /* allow multiple inclusions */
#define _MACHINE_TRAP_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#endif /* _KERNEL_BUILD */

/*
** Trap type values (the types are just the offset into the exception table)
*/

/* 0 (thru 4) is for the reset vector and not used by trap */

#define	T_BUSFLT	8	/* bus error            */
#define	T_ADDRFLT	12	/* address error        */
#define T_ILLFLT	16	/* illegal instruction  */
#define	T_ARITHTRAP	20	/* zero divide          */
#define T_CHKFLT	24	/* chk instruction      */
#define T_TRAPV		28	/* trapv instruction    */
#define	T_PRIVFLT	32	/* privilege violation  */
#define	T_TRCTRAP	36	/* trace trap           */
#define	T_LINEA		40	/* line 1010 trap  (iot)*/
#define	T_LINEF		44	/* line 1111 trap  (emt)*/

#define	T_CPROTO	52	/* coprocessor protocol violation */
#define	T_FORMAT	56	/* format error */

#define T_SPURIOUS	96	/* spurious interrupt   */

#define	T_SYSCALL	128	/* trap0 -- syscall     */
#define	T_BPTFLT	132	/* trap1 -- breakpoint  */
#define T_TRAP2		136	/* trap2 -- illegal    	*/
#define	T_TRAP3 	140	/* trap3 -- illegal     */
#define T_TRAP4		144	/* trap -- illegal	*/
#define T_TRAP5		148	/* trap -- illegal      */
#define T_TRAP6		152	/* trap -- illegal      */
#define T_TRAP7		156	/* trap -- illegal      */
#define T_TRAP8		160	/* trap -- fpe          */
#define T_TRAP9		164	/* trap -- illegal      */
#define T_TRAP10	168	/* trap -- illegal      */
#define T_TRAP11	172	/* trap -- illegal      */
#define T_TRAP12	176	/* trap -- illegal      */
#define T_TRAP13	180	/* trap -- illegal      */
#define T_TRAP14	184	/* trap -- illegal      */
#define T_TRAP15	188	/* trap -- illegal      */

/* 68881 floating point coprocessor exceptions */
#define T_BSUN		192	/* 68881 branch or set byte on unordered cond */
#define T_INEX		196	/* 68881 inexact result */
#define T_ZDIV		200	/* 68881 divide by zero */
#define T_UNDRFLW	204	/* 68881 underflow */
#define T_OPERR		208	/* 68881 operand error */
#define T_OVRFLW	212	/* 68881 overflow */
#define T_SNAN		216	/* 68881 signalling NAN */

/* MC68040 unimplemented floating point data type */
#define T_UNIMP_DATA	224

#define	T_RESCHED	256	/* trap -- reschedule   */

#define	T_SEGFLT	1008	/* limit violation	*/
#define	T_PROTFLT	1012	/* protection fault	*/
#define	T_PAGEFLT	1016	/* page fault		*/
#define	T_TABLEFLT	1020	/* page table fault	*/
#define	T_ATCMIS 	1024	/* pmmu atc mis fault	*/

/*
** Special status word masks
*/

#define	FC	0x8000			/* pipe C fault        -- 68020  */
#define	FB	0x4000			/* pipe B fault        -- 68020  */
#define	RC	0x2000			/* rerun stage C       -- 68020  */
#define	RB	0x1000			/* rerun stage B       -- 68020  */
#define	DF	0x0100			/* data access fault   -- 68020  */
#define	RM	0x0080			/* data read-mod-write -- 68020 */
#define	RRW	0x0040			/* data read/write     -- 68020 */
#define	SZ	0x0030			/* data size code      -- 68020 */
#define	AS	0x0007			/* data function code  -- 68020 */

/*
** The following is used by trap to decypher what kind of exception happened.
** It describes the worst case exception stack (those cased by bus error or
** address error).
*/

struct exception_stack {
	int 	e_regs[16];			/* 16 gprs +                */
	u_short	e_PS;				/* status register 	    */
	int	e_PC;				/* program counter          */
	u_short	e_offset;			/* vector offset (type)     */
    union {
	struct {
		u_short	e_ssw;			/* special status word      */
		int	e_address;		/* fault address            */
		short	e_unused1;		/* unused                   */
		u_short	e_data_ob;		/* data output buffer       */
		short	e_unused2;		/* unused                   */
		u_short	e_data_ib;		/* data input buffer        */
		short	e_unused3;		/* unused                   */
		u_short	e_ins_ib;		/* instruction input buffer */
		short	e_internal[16];		/* 16 words of internal info*/
	} e_68010;

	struct {
		u_short	e_ir1;			/* 1 internal register      */
		u_short	e_ssw;			/* special status word      */
		u_short	e_ips_c;		/* instruction pipe stage C */
		u_short	e_ips_b;		/* instruction pipe stage B */
		int	e_address;		/* fault address            */
		u_short	e_ir2[2];		/* 2 internal registers     */
		int    	e_data_ob;		/* data output buffer       */
		u_short	e_ir3[2];		/* 2 internal registers     */
	} e_68020_short;

	struct {
		u_short	e_ir1;			/* 1 internal register      */
		u_short	e_ssw;			/* special status word      */
		u_short	e_ips_c;		/* instruction pipe stage C */
		u_short	e_ips_b;		/* instruction pipe stage B */
		int	e_address;		/* fault address            */
		u_short	e_ir2[2];		/* 2 internal registers     */
		int    	e_data_ob;		/* data output buffer       */
		u_short	e_ir3[4];		/* 4 internal registers     */
		int    	e_stage_b;		/* stage B address          */
		u_short	e_ir4[2];		/* 2 internal registers     */
		int    	e_data_ib;		/* data input buffer        */
		u_short	e_ir5[22];		/* 22 internal registers    */
	} e_68020_long;
    } e_union;
};


/*******************************************************************************
** MC68040 special status word masks
*******************************************************************************/

/* Continuation pending mask bits */
#define	CONTINUATION_BITS	0xF000

/* Continuation - FP post excption pending */
#define	CP		0x8000

/* Continuation - Unimplemented FP excption pending */
#define	CU		0x4000

/* Continuation - Trace excption pending */
#define	CT		0x2000

/* Continuation - MOVEM instruction executuion pending */
#define	CM		0x1000

/* Misaligned access */
#define	MALIGN		0x0800

/* ATC fault */
#define	ATC		0x0400

/* Locked transfer */
#define	LK		0x0200

/* Read/Write */
#define	RW		0x0100 	/* read access = 1, write access = 0 */

/* Undefined                     */
#define	UD		0x0080

/* Transfer Size                 */
#define	SIZE		0x0060

/* Transfer Type                 */
#define	TT		0x0018

/* Transfer Modifier (aka fc)    */
#define	TM		0x0007

/*******************************************************************************
** MC68040 e_offset masks
*******************************************************************************/

/* Format mask */
#define	FORMAT_BITS			0xF000

/* MC68040 access error format value */
#define	MC68040_ACCESS_EXCEPTION	0x7000

/* Exception vector offset */
#define	VECTOR_OFFSET_BITS		0x0FFF

/*******************************************************************************
** MC68040 writeback status bits macros
*******************************************************************************/

#define	WB_VALID(status_byte)	((status_byte & 0x80))
#define	WB_SIZE(status_byte)	((status_byte & 0x60) >> 5)
#define	WB_TT(status_byte)	((status_byte & 0x18) >> 3)
#define	WB_TM(status_byte)	((status_byte & 0x07))

/*******************************************************************************
** MC68040 status register bit definitions
*******************************************************************************/

#define	MC68040_MMU_PADDR_BITS	0xFFFFF000
#define MC68040_BUS_ERROR	0x00000800
#define MC68040_GLOBALLY_SHARED	0x00000400
#define MC68040_UPA_1		0x00000200
#define MC68040_UPA_0		0x00000100
#define MC68040_SUPER_PROTECTED	0x00000080
#define MC68040_CACHE_MODE_BITS	0x00000060
#define MC68040_MODIFIED	0x00000010
#define MC68040_UD		0x00000008
#define MC68040_WRITE_PROTECTED	0x00000004
#define MC68040_TT_HIT		0x00000002
#define MC68040_RESIDENT	0x00000001


struct mc68040_exception_stack {
	int 	e_regs[16];		/* 16 gprs +                */
	u_short	e_PS;			/* status register 	    */
	int	e_PC;			/* program counter          */
	u_short	e_offset;		/* vector offset (type)     */
	int	e_eff_address;		/* effective address        */
        u_short e_ssw;                  /* special status word      */
	u_char  e_pad1;			/* reserved at zero         */
	u_char  e_wb3s;			/* write-back 3 status      */
	u_char  e_pad2;			/* reserved at zero         */
	u_char  e_wb2s;			/* write-back 2 status      */
	u_char  e_pad3;			/* reserved at zero         */
	u_char  e_wb1s;			/* write-back 1 status      */
	int	e_address;		/* fault address            */
	int	e_wb3a;			/* write-back 3 address     */
	int	e_wb3d;			/* write-back 3 data        */
	int	e_wb2a;			/* write-back 2 address     */
	int	e_wb2d;			/* write-back 2 data        */
	int	e_wb1a;			/* write-back 1 address     */
	int	e_wb1d;			/* write-back 1 data/push 0 */
	int	e_pd1;			/* push data 1              */
	int	e_pd2;			/* push data 2              */
	int	e_pd3;			/* push data 3              */
};

/* pneumonic for accessing push data word zero */
#define e_pd0	e_wb1d

#endif /* _MACHINE_TRAP_INCLUDED */
