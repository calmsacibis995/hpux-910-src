/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/pm_core.c,v $
 * $Revision: 1.5.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:09:11 $
 */

#include "../h/debug.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/vm.h"
#include "../h/uio.h"
#include "../h/errno.h"
#include "../h/file.h"
#include "../ufs/inode.h"
#include "../dux/lookupops.h"
#include "../h/core.h"
#include "../h/vas.h"
#include "../h/pregion.h"
#include "../h/utsname.h"
#ifdef  __hp9000s800
#include "../machine/save_state.h"
#endif
#ifdef  __hp9000s300
#define STACK_300_OVERFLOW_FIX
#include "../machine/reg.h"
#include "../machine/pcb.h"
#include "../machine/trap.h"
#include "../h/magic.h"
#include "../h/vfd.h"
#include "../h/dbd.h"
#endif
#ifdef	STACK_300_OVERFLOW_FIX
#include "../h/malloc.h"
#endif	/* STACK_300_OVERFLOW_FIX */

setcore()
{
	u.u_error = EOPNOTSUPP;
}

/*
 * Create a core image on the file "core".
 */
core()
{
	struct vnode *vp;
	struct vattr vattr;
	struct utsname  sysname;
	struct proc_exec exec_data;
	register struct proc *pp;
	int flags, csize = 0, cflg = 0;
	int cversion;
	sv_sema_t	vno_closeSS;

	if (u.u_uid != u.u_ruid || u.u_gid != u.u_rgid)
		return(0);

	/* Get pointers to regions */
	pp = u.u_procp;
	flags = pp->p_coreflags;
	/* if ( flags == CORE_NONE )
		return(0);
	*/
	csize = findpregsizes(pp->p_vas,flags);
	if ( flags & CORE_FORMAT ) {
		csize += sizeof(int) + sizeof(struct corehead);
		cflg++;
	}
	/* Kernel version */
	csize += sizeof(struct corehead) + sizeof(struct utsname);
	/* CORE_EXEC */
	csize += sizeof(struct corehead) + sizeof(struct proc_exec);
	if (csize >= u.u_rlimit[RLIMIT_CORE].rlim_cur)
		return(0);

	u.u_error = 0;
	PXSEMA(&filesys_sema, &vno_closeSS);
	u.u_error =
	    vn_open("core", UIOSEG_KERNEL, FWRITE|FTRUNC|FCREAT,
			0666 & ~u.u_cmask, &vp);
	vattr_null(&vattr);
	vattr.va_type = VREG;
	vattr.va_mode = 0644;
	if (!u.u_error)
		u.u_error =
			VOP_GETATTR(vp, &vattr, u.u_cred, VASYNC);
	if (u.u_error) {
		VXSEMA(&filesys_sema, &vno_closeSS);
		return(0);
	}
	if (vattr.va_nlink != 1) {
		u.u_error = EFAULT;
		goto out;
	}
	/* truncate file to 0 */
	vattr_null(&vattr);
	vattr.va_size = 0;
	VOP_SETATTR(vp, &vattr, u.u_cred, 0);

	/* get kernel version string */
	/* bcopy(&utsname, &sysname, sizeof(struct utsname)); */
	sysname = utsname;

	/* write kernel version string */
	write_core(CORE_KERNEL, KERNELSPACE, (caddr_t)&sysname,
		   sizeof(struct utsname), vp, UIOSEG_KERNEL, 0);

	/* write CORE_EXEC */
#ifdef  __hp9000s300
	exec_data.Ux_A = u.u_exdata.Ux_A;
#endif
#ifdef  __hp9000s800
	exec_data.exdata.u_magic = u.u_exdata.u_magic;
	exec_data.exdata.som_aux = u.u_exdata.som_aux;
#endif
	strcpy(exec_data.cmd, u.u_comm);
	write_core(CORE_EXEC, KERNELSPACE, (caddr_t)&exec_data,
		   sizeof(struct proc_exec), vp, UIOSEG_KERNEL, 0);

	if (u.u_error == 0 &&  cflg) {
		cversion = CORE_FORMAT_VERSION;
		write_core(CORE_FORMAT, KERNELSPACE, (caddr_t)&cversion,
 		sizeof(int), vp, UIOSEG_KERNEL, 0);
	}

	/* get other information out based on the value of flags */
	if (u.u_error == 0 ) {
		walkpregions(pp->p_vas, flags, vp);
	}


out:
	vn_close(vp, FWRITE);
	VN_RELE(vp);
	VXSEMA(&filesys_sema, &vno_closeSS);
	return (u.u_error == 0);
}

int
write_core(type, space, addr, len, vp, uioseg, fpflags)
	int type;
	space_t space;
	caddr_t addr;
	int len;
	struct vnode *vp;
	int uioseg;
	int fpflags;
{
	struct	corehead  ch;

	ch.type = type;
	ch.space = space;
	ch.addr = addr;
	ch.len = len;
	u.u_error = vn_rdwr(UIO_WRITE, vp,
	    (caddr_t)&ch,
	    sizeof(struct corehead), 0,
	    UIOSEG_KERNEL, IO_UNIT|IO_APPEND, (int *)0, 0);
	if (u.u_error == 0 && len > 0) {
		u.u_error = vn_rdwr(UIO_WRITE, vp,
	    	(caddr_t)addr, len, 0,
	    	uioseg, IO_UNIT|IO_APPEND, (int *)0, fpflags);
	}
	return(u.u_error);
}


int
findpregsizes(vas, flags)
	register vas_t *vas;
	int	flags;
{
	register preg_t *prp;
	int	csize = 0, chcount = 0, t = 0;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	vaslock(vas);
	for (prp = vas->va_next; prp != (preg_t *)vas; prp = prp->p_next) {
		switch (prp->p_type) {

		case PT_UAREA:
				if (flags & CORE_PROC) {
					t += sizeof(struct proc_info);
					chcount++;
				}
				break;
		/* case PT_TEXT:
				if (flags & CORE_TEXT) {
					csize += prp->p_count;
					chcount++;
				}
				break;
		*/
		case PT_DATA:
				if (flags & CORE_DATA) {
					csize += prp->p_count;
					chcount++;
				}
				break;
		case PT_STACK:
				if (flags & CORE_STACK) {
					csize += prp->p_count;
					chcount++;
				}
				break;
		case PT_SHMEM:
				if (flags & CORE_SHM) {
					reg_t *rp = prp->p_reg;
					/*
					 * Only writable,private shared
					 * memory regions are considered
					 * for core dump.
					 */
					if((prp->p_prot & PROT_WRITE) &&
					   (rp->r_type == RT_PRIVATE)) {
						csize += prp->p_count;
						chcount++;
					}
				}
				break;
		case PT_MMAP:
				if (flags & CORE_MMF) {
					reg_t *rp = prp->p_reg;
					/*
					 * Only writable,private memory
					 * mapped regions are considered
					 * for core dump.
					 */
					if((prp->p_prot & PROT_WRITE) &&
					   (rp->r_type == RT_PRIVATE)) {
						csize += prp->p_count;
						chcount++;
					}
				}
				break;
		default:	/* skip */
				;
		}
	}
	vasunlock(vas);
	vmemp_unlockx();	/* free up VM empire */
	t += ptob(csize) + (chcount * sizeof(struct corehead));
	return(t);
}

#ifdef  __hp9000s300
/*ARGSUSED*/
do_findoff(idx, vfd, dbd, arg)
	int idx;
	vfd_t	*vfd;
	register dbd_t *dbd;
	register int  *arg;
{
	if (dbd->dbd_type != DBD_DZERO) {
		*arg = idx;
		return(1);
	}
	return(0);
}
#endif


walkpregions(vas, flags, vp)
	register vas_t *vas;
	int	flags;
	struct vnode *vp;
{
	register preg_t *prp;
#ifdef	STACK_300_OVERFLOW_FIX
	struct proc_info *pinfo;
#else	/* STACK_300_OVERFLOW_FIX */
	struct proc_info pinfo;
#endif	/* STACK_300_OVERFLOW_FIX */

	for (prp = vas->va_next; prp != (preg_t *)vas; prp = prp->p_next) {
		switch (prp->p_type) {

		case PT_UAREA:
				if (flags & CORE_PROC) {
#ifdef	STACK_300_OVERFLOW_FIX
					/* might sleep here */
					VAPOR_MALLOC(pinfo,
						struct proc_info *,
						sizeof(struct proc_info),
						M_TEMP, M_WAITOK);
#endif	/* STACK_300_OVERFLOW_FIX */
#ifdef  __hp9000s300
					save_floating_point(u.u_pcb.pcb_float);
					if (dragon_present && (u.u_pcb.pcb_dragon_bank != -1))
						dragon_save_regs(u.u_pcb.pcb_dragon_regs);
#endif
					/* gather registers */
#ifdef	STACK_300_OVERFLOW_FIX
					gatherregs(&pinfo->hw_regs);
					pinfo->sig = u.u_arg[0];
					pinfo->trap_type = u.u_code;
					write_core(CORE_PROC, KERNELSPACE, pinfo,
#else	/* STACK_300_OVERFLOW_FIX */
					gatherregs(&pinfo.hw_regs);
					pinfo.sig = u.u_arg[0];
					pinfo.trap_type = u.u_code;
					write_core(CORE_PROC, KERNELSPACE,
						(caddr_t)&pinfo,
#endif	/* STACK_300_OVERFLOW_FIX */
					sizeof(struct proc_info), vp, UIOSEG_KERNEL, 0);
				}
				break;
		/*
		case PT_TEXT:
				if (flags & CORE_TEXT) {
					write_core(CORE_TEXT, prp->p_space, prp->p_vaddr,
					ptob(prp->p_count), vp, UIOSEG_USER, 0);
				}
				break;
		*/
		case PT_DATA:
				if (flags & CORE_DATA) {
					write_core(CORE_DATA, prp->p_space,
						prp->p_vaddr,
						(int)ptob(prp->p_count),
						vp, UIOSEG_USER, FSYNCIO);
				}
				break;
		case PT_STACK:
				if (flags & CORE_STACK) {
#ifdef  __hp9000s300
					caddr_t saddr;
					saddr = (caddr_t)ptob(btop(u.u_ar0[SP]));
					if (prpcontains(prp,prp->p_space,saddr))
					    write_core(CORE_STACK, prp->p_space, saddr,
					    prp->p_vaddr+ptob(prp->p_count)-saddr,
					    vp, UIOSEG_USER, FSYNCIO);
				        else {
					    int noff;
					    reglock(prp->p_reg);
					    foreach_entry(prp->p_reg,prp->p_off,
					    prp->p_count,do_findoff,&noff);
					    regrele(prp->p_reg);
					    write_core(CORE_STACK, prp->p_space,
					    prp->p_vaddr+ptob(noff - prp->p_off),
					    ptob(prp->p_count - (noff - prp->p_off)),
					    vp, UIOSEG_USER, FSYNCIO);
					}
#else
					write_core(CORE_STACK, prp->p_space,
						prp->p_vaddr,
						(int)ptob(prp->p_count),
						vp, UIOSEG_USER, FSYNCIO);
#endif
				}
				break;
		case PT_SHMEM:
				if (flags & CORE_SHM) {
					reg_t *rp = prp->p_reg;
					/*
					 * Only writable,private shared
					 * memory regions are considered
					 * for core dump.
					 */
					if((prp->p_prot & PROT_WRITE) &&
					   (rp->r_type == RT_PRIVATE)) {
						write_core(CORE_SHM,
						prp->p_space, prp->p_vaddr,
						ptob(prp->p_count), vp,
						UIOSEG_USER, 0);
					}
				}
				break;
		case PT_MMAP:
				if (flags & CORE_MMF) {
					reg_t *rp = prp->p_reg;
					/*
					 * Only writable,private shared
					 * memory regions are considered
					 * for core dump.
					 */
					if((prp->p_prot & PROT_WRITE) &&
					   (rp->r_type == RT_PRIVATE)) {
						write_core(CORE_MMF,
						prp->p_space, prp->p_vaddr,
						ptob(prp->p_count), vp,
						UIOSEG_USER, 0);
					}
				}
				break;
		default:	/* skip */
				;
		}
	}
}

#define	SIZEOF_SOFT_PCB	 	36
#define NUMOFREGS		15

gatherregs(regs)
#ifdef  __hp9000s800
struct save_state *regs;
#else
struct proc_regs *regs;
#endif
{

#ifdef __hp9000s300
	int i, *ptoregs;
#endif

#ifdef  __hp9000s800
	bcopy(u.u_sstatep, regs, sizeof(struct save_state));
#endif
#ifdef  __hp9000s300
	ptoregs = (int *)regs;
	for (i = 0; i < NUMOFREGS; i++) {
		*ptoregs++ = u.u_ar0[i];
	}
	regs->pc = ((struct exception_stack *)u.u_ar0)->e_PC;
	regs->ps = ((struct exception_stack *)u.u_ar0)->e_PS;
	regs->usp = u.u_pcb.pcb_usp;
	/* copy 10 float registers */
	bcopy(&(u.u_pcb.pcb_float[0]), &(regs->p_float[0]),
		(10 * sizeof(int)));
	/* copy 81 mc68881 registers */
	bcopy(&(u.u_pcb.pcb_mc68881[0]), &(regs->mc68881[0]),
		(81 * sizeof(int)));
	/* copy 32 dragon registers */
	bcopy(&(u.u_pcb.pcb_dragon_regs[0]), &(regs->dragon_regs[0]),
		(32 * sizeof(int)));
	regs->dragon_bank = u.u_pcb.pcb_dragon_bank;
	regs->dragon_sr = u.u_pcb.pcb_dragon_sr;
	regs->dragon_cr = u.u_pcb.pcb_dragon_cr;
#endif
	return;

}
