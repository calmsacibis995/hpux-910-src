/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/find.c,v $
 * $Revision: 1.25.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:31:07 $
 */


/*
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/


#include "inc.h"
#include "defs.h"
#include "types.h"
#include "externs.h"

struct proc *
pid_to_proc(pid)
register int pid;
{
	register int i;

	for(i = 0; i < nproc; i++){
		if (proc[i].p_pid == pid){
			return(&proc[i]);
		}
	}
	fprintf(outf," No such pid in proc table\n");
	return((struct proc *)-1);

}

struct proc *
ndx_toproc(ndx)
register int ndx;
{
	register int i;
	if ((ndx < 0) || (ndx >= nproc)){
		fprintf(outf," Proc ndx out of range\n");
		return((struct proc *)-1);
	}
	return(&proc[ndx]);
}

struct proc *
valid_proc(vaddr)
register struct proc *vaddr;
{
	if ((vaddr < vproc) || (vaddr >= (struct proc *)&vproc[nproc])){
		fprintf(outf," Proc address out of range\n");
		return((struct proc *)-1);
	}
	return(&proc[vaddr-vproc]);
}



struct buf *
valid_swbuf(vaddr)
register struct buf *vaddr;
{
	if ((vaddr >= vswbuf) && (vaddr < &vswbuf[nswbuf]))
		return(&swbuf[vaddr-vswbuf]);

	fprintf(outf," swap buf address out of range\n");
	return((struct buf *)-1);
}

struct buf *
ndx_toswbuf(ndx)
register int ndx;
{
	register int i;
	if ((ndx < 0) || (ndx >= nswbuf)){
		fprintf(outf," Swap buf ndx out of range\n");
		return((struct buf *)-1);
	}
	return(&swbuf[ndx]);
}


struct shmem *
ndx_toshmem(ndx)
register int ndx;
{
	register int i;
	if ((ndx < 0) || (ndx >= shminfo.shmmni)){
		fprintf(outf," Shmem ndx out of range\n");
		return((struct shmem *)-1);
	}
	return((struct shmem *)&shmem[ndx]);
}

struct shmid_ds *
valid_shmem(vaddr)
register struct shmid_ds *vaddr;
{
	if ((vaddr < vshmem) || (vaddr >= &vshmem[shminfo.shmmni])){
		fprintf(outf," shmem table address out of range\n");
		return((struct shmid_ds *)-1);
	}
	return(&shmem[vaddr - vshmem]);
}

struct inode *
valid_inode(vaddr)
register struct inode *vaddr;
{
	if ((vaddr < vinode) || (vaddr >= &vinode[ninode])){
		fprintf(outf," Inode address out of range\n");
		return((struct inode *)-1);
	}
	return(&inode[vaddr-vinode]);
}

struct inode *
ndx_toinode(ndx)
register int ndx;
{
	register int i;
	if ((ndx < 0) || (ndx >= ninode)){
		fprintf(outf," Inode ndx out of range\n");
		return((struct inode *)-1);
	}
	return(&inode[ndx]);
}


struct file *
valid_file(vaddr)
register struct file *vaddr;
{
	if ((vaddr < vfile) || (vaddr >= &vfile[nfile])){
		fprintf(outf," file address out of range\n");
		return((struct file *)-1);
	}
	return(&file[vaddr-vfile]);
}

struct file *
ndx_tofile(ndx)
register int ndx;
{
	register int i;
	if ((ndx < 0) || (ndx >= nfile)){
		fprintf(outf," File ndx out of range\n");
		return((struct file *)-1);
	}
	return(&file[ndx]);
}

#ifdef hp9000s800
struct hpde *
valid_pde(vaddr)
register struct hpde *vaddr;
{
	extern getepde();
	extern struct hpde *base_pdir;
	extern struct hpde *max_pdir;

	if ((vaddr >= vhtbl) && (vaddr <= &vhtbl[nhtbl])){
		return(&htbl[vaddr-vhtbl]);
	}

	if ((vaddr >= vpdir) && (vaddr <= &vpdir[scaled_npdir])){
		return(&pdir[vaddr-vpdir]);
	}

	/*
         * a dynamically allocated pde
         */
	 if ((vaddr >= base_pdir) && (vaddr <= max_pdir)){
		  if (getepde(vaddr, &tmppde))
			  return(0);
                  return(&tmppde);
	}

	fprintf(outf," Pdir address out of range\n");
	return((struct hpde *)-1);
}

struct hpde *
ndx_topde(ndx)
register int ndx;
{
	register int i;
	fprintf(outf," Pdir ndx not supported in new pdir\n");
	fprintf(outf," Use the pgtopde_table to get pde pointer\n");
		return((struct hpde *)-1);
}

#endif


/* REGION */

struct pfdat *
ndx_topfdat(ndx)
register int ndx;
{
	register int i;
	if ((ndx < 0) || (ndx >= physmem)){
		fprintf(outf," Pfdat ndx out of range\n");
		return((struct pfdat *)-1);
	}
	return(&pfdat[ndx]);
}

struct pfdat *
valid_pfdat(vaddr)
register struct pfdat *vaddr;
{
	if ((vaddr < vpfdat) || (vaddr >= &vpfdat[physmem])){
		fprintf(outf," Pfdat address out of range\n");
		return((struct pfdat *)-1);
	}
	return(&pfdat[vaddr- vpfdat]);
}


struct pfdat *
pfdatpage_valid(pg)
register int pg;
{
	if ((pg < firstfree) || (pg > physmem)){
		fprintf(outf," Page out of range of pfdat\n");
		return((struct pfdat *)-1);
	}
	return(&pfdat[pg]);

}


struct mapent *
valid_sysmap(vaddr)
register struct mapent *vaddr;
{
	if ((vaddr < (struct mapent *)vsysmap) ||
	    (vaddr >= (struct mapent *)&vsysmap[SYSMAPSIZE])){
		fprintf(outf," Sysmap address out of range\n");
		return((struct mapent *)-1);
	}
	return((struct mapent *)&sysmap[vaddr - (struct mapent *)vsysmap]);
}

struct mapent *
ndx_tosysmap(ndx)
register int ndx;
{
	register int i;
	if ((ndx < 0) || (ndx >= SYSMAPSIZE)){
		fprintf(outf," Sysmap ndx out of range\n");
		return((struct mapent *)-1);
	}
	return((struct mapent *)&sysmap[ndx]);
}

#ifdef LATER
struct seglist *
valid_seglist(vaddr)
register struct seglist *vaddr;
{
	if ((vaddr < vpsegs) || (vaddr >= &vpsegs[shminfo.shmmni])){
		fprintf(outf," Seglist address out of range\n");
		return((struct seglist *)-1);
	}
	return(&psegs[vaddr- vpsegs]);
}

struct seglist *
ndx_toseglist(ndx)
register int ndx;
{
	register int i;
	if ((ndx < 0) || (ndx >= shminfo.shmmni)){
		fprintf(outf," Seglist ndx out of range\n");
		return((struct seglist *)-1);
	}
	return(&psegs[ndx]);
}
#endif LATER

dbd_t *
valid_dbd(vaddr)
register dbd_t *vaddr;
{
	getchunk(KERNELSPACE, vaddr, &dbd_temp, sizeof(dbd_t),"valid_dbd");
	return(&dbd_temp);
}

swpt_t *
ndx_toswaptab(ndx)
register int ndx;
{
	register int i;
	if ((ndx < 0) || (ndx >= MAXSWAPCHUNKS)){
		fprintf(outf," Swaptab ndx out of range\n");
		return((swpt_t *)-1);
	}
	return(&swaptab[ndx]);
}

swpt_t *
valid_swaptab(vaddr)
register struct swaptab *vaddr;
{
	if ((vaddr < vswaptab) || (vaddr >= &vswaptab[MAXSWAPCHUNKS])){
		fprintf(outf," Swaptab address out of range\n");
		return((swpt_t *)-1);
	}
	return(&swaptab[vaddr- vswaptab]);
}


struct inode *
vtoi(v)
struct vnode *v;
{
return(VTOI((struct vnode *)((char *)inode + ((char *)v - (char *)vinode))));
}
struct vnode *
itov(i)
struct inode *i;
{
return(ITOV((i)));
}
