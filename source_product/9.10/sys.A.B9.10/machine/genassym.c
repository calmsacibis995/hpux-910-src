/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/genassym.c,v $
 * $Revision: 1.9.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:04:20 $
 */

/* HPUX_ID: @(#)genassym.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

#include "../s200/pte.h"
#include "../h/param.h"
#include "../h/vmmeter.h"
#include "../h/vmparam.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/mbuf.h"
#include "../h/msgbuf.h"
#include "../s200/trap.h"
#include "../wsio/intrpt.h"
#include "../wsio/timeout.h"
#include "../wsio/tryrec.h"
#include "../h/errno.h"
#include "../s200/reg.h"
#include "../s200/sendsig.h"
#include "../wsio/hpibio.h"
#include "../h/buf.h"
#include "../wsio/iobuf.h"
#include "../s200io/dma.h"
#include "../wsio/hshpib.h"
#include "../s200io/ti9914.h"
#include "../wsio/dilio.h"
#include "../wsio/dil.h"
#include "../s200io/dragon.h"
#ifdef	FSD_KI
#include "../h/ki_calls.h"
#endif	FSD_KI
#include "../h/callout.h"

/* XXX move to pcb.h someday... */
#define pcb_locregs pcb_float[2]

main()
{
	register struct proc *p = (struct proc *)0;
	register struct vmmeter *vm = (struct vmmeter *)0;
	register struct user *up = (struct user *)0;
	struct text *tp = (struct text *)0;
	struct interrupt *ip = (struct interrupt *)0;
	struct sw_intloc *sw = (struct sw_intloc *)0;
	struct C__trystuff *tr = (struct C__trystuff *)0;
	struct sigframe *sf = (struct sigframe *)0;
	struct full_sigcontext *fsc = (struct full_sigcontext *)0;
	struct isc_table_type *sc = (struct isc_table_type *)0;
	struct buf *bp = (struct buf *)0;
	struct iobuf *iob = (struct iobuf *)0;
	struct dma_channel *dcp = (struct dma_channel *)0;
	struct simon *simp = (struct simon *)0;
	struct TI9914 *tip = (struct TI9914 *)0;
	struct msem_procinfo *msem_ptr = (struct msem_procinfo *)0;
#ifdef	FSD_KI
	struct ki_config *ki_cp = (struct ki_config *)0;
#endif	FSD_KI

	printf("	set	NQS 	,	0x%x\n", NQS);
	printf("	set	NQELS 	,	0x%x\n", NQELS);
	printf("	set	P_LINK 	,	0x%x\n", &p->p_link);
	printf("	set	P_FLAG	,	0x%x\n", &p->p_flag);
	printf("	set	P_FLAG2	,	0x%x\n", &p->p_flag2);
#ifdef	FSD_KI
	printf("	set	SSYS 	,	0x%x\n", SSYS);
#endif	FSD_KI
	printf("	set	P_RLINK ,	0x%x\n", &p->p_rlink);
	printf("	set	P_SEGPTR ,	0x%x\n", &p->p_segptr);

	printf("	set	P_ADDR  ,	0x%x\n", &p->p_addr);
	printf("	set	P_PRI   ,	0x%x\n", &p->p_pri);
	printf("	set	P_STAT  ,	0x%x\n", &p->p_stat);
	printf("	set	P_WCHAN	,	0x%x\n", &p->p_wchan);
	printf("        set     P_STACKPAGES ,  0x%x\n", &p->p_stackpages);
	printf("        set     P_MSEMPINFO ,   0x%x\n", &p->p_msem_info);
	printf("        set     P_VFORKBUF,     0x%x\n", &p->p_vforkbuf);
	printf("	set	SRUN 	,	0x%x\n", SRUN);

	printf("        set     MSEM_LOCKID ,   0x%x\n", &msem_ptr->lockid);

	printf("	set	V_SWTCH ,	0x%x\n", &vm->v_swtch);
	printf("	set	V_INTR  ,	0x%x\n", &vm->v_intr);
	printf("	set	V_TRAP  ,	0x%x\n", &vm->v_trap);
	printf("	set	V_SYSCALL  ,	0x%x\n", &vm->v_syscall);
	printf("	set	V_RUNQ  ,	0x%x\n", &vm->v_runq);

	printf("        set     KSTACK_PAGES ,  %d\n", KSTACK_PAGES);
	printf("        set     KSTACK_RESERVE , %d\n", KSTACK_RESERVE);
	printf("	set	UPAGES 	,	%d\n", UPAGES);
	printf("	set	MSGBUFPTECNT , %d\n", btoc(sizeof (struct msgbuf)));
	printf("	set	MCLBYTES  , %d\n", MCLBYTES);

	printf("	set	NBPG	,	%d\n", NBPG);
	printf("	set	PGSHIFT	,	%d\n", PGSHIFT);
	printf("	set	PGOFSET	,	%d\n", PGOFSET);

	printf("	set	CLSIZE	,	%d\n", CLSIZE);
	printf("	set	CLBYTES	,	%d\n", CLSIZE * NBPG);
	printf("	set	PG_REF 	,	%d\n", PG_REF);
	printf("	set	PG_NS	,	%d\n", PG_NS);
	printf("	set	PG_V 	,	%d\n", PG_V);
	printf("	set	PG_CB 	,	%d\n", PG_CB);
	printf("	set	PG_CI	,	%d\n", PG_CI);
	printf("	set	PG_PROT	,	%d\n", PG_PROT);
	printf("	set	NOT_PG_PROT,	0x%x\n", ~PG_PROT);
	printf("	set	PG_RO	,	%d\n", PG_RO);
	printf("	set	PG_RW	,	%d\n", PG_RW);
	printf("	set	PG_FRAME ,	%d\n", PG_FRAME);
	printf("	set	PG_INCR ,	%d\n", PG_INCR);

	printf("	set	SSEGPTR		,	0x%x\n", SSEGPTR);
	printf("	set	USEGPTR		,	0x%x\n", USEGPTR);
	printf("	set	PTLB		,	0x%x\n", PTLB);
	printf("	set	TLBSELP		,	0x%x\n", TLBSELP);
	printf("	set	MMU_CONTROL	,	0x%x\n", MMU_CONTROL);

	printf("	set	MMU_UMEN	,	0x%x\n", MMU_UMEN);
	printf("	set	MMU_SMEN	,	0x%x\n", MMU_SMEN);
	printf("	set	MMU_CEN		,	0x%x\n", MMU_CEN);
	printf("	set	MMU_IEN		,	0x%x\n", MMU_IEN);
	printf("	set	MMU_FPE		,	0x%x\n", MMU_FPE);
	printf("	set	IC_CLR 		,	0x%x\n", IC_CLR);
	printf("	set	IC_CE 		,	0x%x\n", IC_CE);
	printf("	set	IC_FREEZE	,	0x%x\n", IC_FREEZE);
	printf("	set	IC_ENABLE	,	0x%x\n", IC_ENABLE);
	/* new cacr bits for the MC68030, see pte.h for definitions */
	printf("	set	DC_CLR 		,	0x%x\n", DC_CLR);
	printf("	set	DC_CE 		,	0x%x\n", DC_CE);
	printf("	set	DC_FREEZE	,	0x%x\n", DC_FREEZE);
	printf("	set	DC_ENABLE	,	0x%x\n", DC_ENABLE);
	printf("	set	DC_BURST	,	0x%x\n", DC_BURST);
	printf("	set	DC_WA    	,	0x%x\n", DC_WA);
	printf("	set	IC_BURST	,	0x%x\n", IC_BURST);
	printf("	set	MC68040_IC_ENABLE ,	0x%x\n",MC68040_IC_ENABLE);
	printf("	set	MC68040_DC_ENABLE ,	0x%x\n",MC68040_DC_ENABLE);
	printf("	set	INVALID		,	0x%x\n", INVALID);

	printf("	set	PTESIZE		,	%d\n", sizeof(struct pte));
	printf("	set	LPTESIZE	,	%d\n", LPTESIZE);

	printf("	set	SG_V		,	0x%x\n", SG_V);
	printf("	set	SG_RO   	,	0x%x\n", SG_RO);
	printf("	set	SG_RW   	,	0x%x\n", SG_RW);
	printf("	set	SG_FRAME	,	0x%x\n", SG_FRAME);
	printf("	set	SG_ISHIFT	,	0x%x\n", SG_ISHIFT);
	printf("	set	SG_PSHIFT	,	0x%x\n", SG_PSHIFT);
	printf("	set	SG_IMASK	,	0x%x\n", SG_IMASK);
	printf("	set	SG_PMASK	,	0x%x\n", SG_PMASK);
	printf("	set	SEGTABSIZE	,	0x%x\n", SEGTABSIZE);
	printf("	set	SG3_FRAME	,	0x%x\n", SG3_FRAME);
	printf("	set	SG3_IMASK	,	0x%x\n", SG3_IMASK);
	printf("	set	SG3_ISHIFT	,	0x%x\n", SG3_ISHIFT);
	printf("	set	SG3_BMASK	,	0x%x\n", SG3_BMASK);
	printf("	set	SG3_PMASK	,	0x%x\n", SG3_PMASK);
	printf("	set	SG3_BSHIFT	,	0x%x\n", SG3_BSHIFT);
	printf("	set	BLK_FRAME	,	0x%x\n", BLK_FRAME);
	printf("	set	NBBT		,	0x%x\n", NBBT);
	printf("        set     U_QSAVE_PC      ,       0x%x\n", &up->u_qsave.val[0]);
	printf("        set     U_QSAVE_A6      ,       0x%x\n", &up->u_qsave.val[11]);
	printf("        set     U_QSAVE_SP      ,       0x%x\n", &up->u_qsave.val[12]);
	printf("	set	U_RSAVE		,	0x%x\n", &up->u_rsave);
	printf("	set	U_PSAVE		,	0x%x\n", &up->u_psave);
	printf("	set	EFAULT		,	0x%x\n", EFAULT);
	printf("	set	PCB_SSWAP	,	0x%x\n", &up->u_pcb.pcb_sswap);

	printf("	set	PCB_FLOAT	,	0x%x\n", up->u_pcb.pcb_float);
	printf("	set	PCB_MC68881	,	0x%x\n", up->u_pcb.pcb_mc68881);
	printf("	set	PCB_DRAGON_BANK	,	0x%x\n", &up->u_pcb.pcb_dragon_bank);
	printf("	set	PCB_DRAGON_REGS	,	0x%x\n", up->u_pcb.pcb_dragon_regs);
	printf("	set	PCB_DRAGON_SR	,	0x%x\n", &up->u_pcb.pcb_dragon_sr);
	printf("	set	PCB_DRAGON_CR	,	0x%x\n", &up->u_pcb.pcb_dragon_cr);
	printf("	set	DRAGON_STATUS_ADDR	,	0x%x\n",DRAGON_STATUS_ADDR);
	printf("	set	DRAGON_CNTRL_ADDR	,	0x%x\n",DRAGON_CNTRL_ADDR);
	printf("	set	DRAGON_BANK_ADDR	,	0x%x\n",DRAGON_BANK_ADDR);
	printf("	set	DRAGON_BUSY	,	0x%x\n",DRAGON_BUSY);

	printf("	set	PCB_FLAGS	,	0x%x\n", &up->u_pcb.pcb_flags);
	printf("	set	PROBE_BIT	,	0x%x\n", PROBE_BIT);
	printf("	set	POP_STACK_BIT	,	0x%x\n", POP_STACK_BIT);
	printf("	set	PROBE_ADDR	,	0x%x\n", &up->u_probe_addr);
	printf("	set	ENOENT		,	0x%x\n", ENOENT);

	printf("	set	PCB_TRYCHAIN	,	0x%x\n", &up->u_pcb.pcb_trychain);
	printf("	set	PCB_ESCAPECODE	,	0x%x\n", &up->u_pcb.pcb_escapecode);
	printf("	set	PCB_LOCREGS	,	0x%x\n", &up->u_pcb.pcb_locregs);
	printf("	set	TRY_PCTR	,	0x%x\n", &tr->pctr);
	printf("	set	TRY_LINK	,	0x%x\n", &tr->link);
	printf("	set	TRY_REGS	,	0x%x\n", tr->regs);

	printf("	set	U_SIGCODE	,	0x%x\n", &up->u_sigcode[0]);
	printf("	set	U_PROCP		,	0x%x\n", &up->u_procp);


	printf("	set	U_VAPOR_MLIST	,	0x%x\n", &up->u_vapor_mlist);
	printf("	set	RESCHED         ,	0x%x\n", T_RESCHED);

	printf("	set	NEXT_ISR        ,	0x%x\n", &ip->next);
	printf("	set	REG_OFFSET      ,	0x%x\n", &ip->regaddr);
	printf("	set	MASK_OFFSET     ,	0x%x\n", &ip->mask);
	printf("	set	VALUE_OFFSET    ,	0x%x\n", &ip->value);
	printf("	set	CHAIN_OFFSET    ,	0x%x\n", &ip->chainflag);
	printf("	set	MISC_OFFSET     ,	0x%x\n", &ip->misc);
	printf("	set	ISR_OFFSET      ,	0x%x\n", &ip->isr);
	printf("	set	TMP_OFFSET      ,	0x%x\n", &ip->temp);
	printf("	set	ISR_STRUCT_SIZE ,	0x%x\n", sizeof(struct interrupt));

	printf("	set	sw_link		,	0x%x\n", &sw->link);
	printf("	set	sw_proc		,	0x%x\n", &sw->proc);
	printf("	set	sw_arg		,	0x%x\n", &sw->arg);
	printf("	set	sw_lvl		,	0x%x\n", &sw->priority);
	printf("	set	sw_slvl		,	0x%x\n", &sw->sub_priority);

	printf("	set	SF_GPR_REGS	,	0x%x\n",
		&sf->sf_full.fs_regs[GPR_START]);
	printf("	set	SF_FLOAT_REGS	,	0x%x\n",
		&fsc->fs_regs[FLOAT_START]);
	printf("	set	SF_MC68881_REGS	,	0x%x\n",
		&fsc->fs_regs[MC68881_START]);

	printf("	set	INT_LVL		,	0x%x\n", &sc->int_lvl);
	printf("	set	TRANSFER	,	0x%x\n", &sc->transfer);
	printf("	set	CARD_PTR	,	0x%x\n", &sc->card_ptr);
	printf("	set	OWNER		,	0x%x\n", &sc->owner);
	printf("	set	DMA_CHAN	,	0x%x\n", &sc->dma_chan);
	printf("	set	INTLOC		,	0x%x\n", &sc->intloc);
	printf("	set	INTCOPY		,	0x%x\n", &sc->intcopy);
	printf("	set	MY_ADDRESS	,	0x%x\n", &sc->my_address);
	printf("	set	COUNT		,	0x%x\n", &sc->count);
	printf("	set	BUFFER		,	0x%x\n", &sc->buffer);
	printf("	set	STATE		,	0x%x\n", &sc->state);
	printf("	set	TFR_CONTROL	,	0x%x\n", &sc->tfr_control);

	/* termination reasons from dil.h */
	printf("	set	TR_COUNT	,	0x%x\n", TR_COUNT);
	printf("	set	TR_END		,	0x%x\n", TR_END);

	printf("	set	B_QUEUE		,	0x%x\n", &bp->b_queue);
	printf("	set	B_SC		,	0x%x\n", &bp->b_sc);

	printf("	set	MARKSTACK	,	0x%x\n", &iob->markstack);
	printf("	set	TIMEFLAG	,	0x%x\n", &iob->timeflag);

	printf("	set	CARD		,	0x%x\n", &dcp->card);
	printf("	set	EXTENT		,	0x%x\n", &dcp->extent);

	printf("	set	MED_IMSK	,	0x%x\n", &simp->med_imsk);
	printf("	set	MED_STATUS	,	0x%x\n", &simp->med_status);
	printf("	set	MED_CTRL	,	0x%x\n", &simp->med_ctrl);

	printf("	set	INTSTAT0	,	0x%x\n", &tip->intstat0);
	printf("	set	DATAIN		,	0x%x\n", &tip->datain);

	printf("	set	M_8BIT_PROC	,	0x%x\n", M_8BIT_PROC);
	printf("	set	M_INT_ENAB	,	0x%x\n", M_INT_ENAB);

	printf("	set	DMA_TFR		,	0x%x\n", DMA_TFR);
	printf("	set	SMART_POLL_IDLE	,	0x%x\n",
			SMART_POLL_IDLE);
#ifdef	FSD_KI
        printf ("       set     KI_SYS_CLOCK            , 0x%x\n", KT_SYS_CLOCK);
        printf ("       set     KI_USR_CLOCK            , 0x%x\n", KT_USR_CLOCK);

#define	kiE	ki_cp->kc_kernelenable
#define	kiC	ki_cp->kc_kernelcounts
	printf ("	set	KI_SWTCH	, 0x%x\n", &kiE[KI_SWTCH]);
	printf ("	set	KI_RESUME_CSW	, 0x%x\n", &kiE[KI_RESUME_CSW]);
	printf ("	set	KI_SETRQ	, 0x%x\n", &kiE[KI_SETRQ]);
	printf ("	set	KI_GETPRECTIME	, 0x%x\n", &kiE[KI_GETPRECTIME]);
	printf ("	set	KI_SYSCALLS	, 0x%x\n", &kiE[KI_SYSCALLS]);

	printf ("       set     KI_CLK_TOS_STACK_PTR, 0x%x\n", &up->ki_clk_tos_ptr);
	printf ("       set     KI_CLK_BEGINNING_STACK_ADDRS, 0x%x\n", &up->ki_clk_stack[KI_CLK_STACK_SIZE-1]);
	printf ("       set     KI_CLK_END_STACK_ADDRS, 0x%x\n", &up->ki_clk_stack[0]);

#endif  /* FSD_KI */
	printf("	set	EXCEPTION_STACK_SIZE , %d\n", sizeof (struct mc68040_exception_stack));

	return(0);
}
