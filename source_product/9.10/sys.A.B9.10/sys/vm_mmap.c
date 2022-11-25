/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_mmap.c,v $
 * $Revision: 1.12.83.6 $        $Author: craig $
 * $State: Exp $        $Locker:  $
 * $Date: 94/01/05 16:38:00 $
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/vmmac.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/mman.h"
#include "../h/map.h"
#include "../h/errno.h"
#include "../h/debug.h"
#include "../h/vas.h"
#include "../h/vfs.h"
#include "../ufs/fs.h"
#include "../dux/duxfs.h"
#include "../h/file.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../netinet/in.h"
#include "../nfs/nfs_clnt.h"
#include "../h/pfdat.h"
#include "../ufs/inode.h"
#include "../machine/param.h"
#include "../h/swap.h"

extern int foreach_pregion();
int growvnode();
caddr_t smmap_hole_anonymous();
caddr_t smmap_hole_file();

smmap()
{
    register struct a {
	caddr_t addr;
	int	size;
	int	prot;
	int	flags;
	int	fdes;
	off_t	off;
    } *uap = (struct a *)u.u_ap;

    int do_mprotect();
    struct file *fp;
    register struct vnode *vp;
    register preg_t *prp;
    reg_t *rp = NULL;
    register int pflags;
    int size = btorp(uap->size);
    int off = uap->off;
    int map_none;
    vas_t *vas = u.u_procp->p_vas;
    int has_mapped = 0;	/* have we called mapvnode() yet? */

    /*
     * Make sure there are not any bogus flags.
     */
    if ((uap->flags & ~(MAP_FILE     | MAP_ANONYMOUS |
			MAP_VARIABLE | MAP_FIXED     |
			MAP_SHARED   | MAP_PRIVATE)) != 0) {
	u.u_error = EINVAL;
	return;
    }

    /*
     * Check the mapping was either shared or private.  At
     * least one flag must be set, but both cannot be set.
     */
    switch (uap->flags & (MAP_PRIVATE|MAP_SHARED)) {
    case 0:
    case (MAP_PRIVATE|MAP_SHARED):
	u.u_error = EINVAL;
	return;

    default:
	break;
    }

    /*
     * Check that MAP_FILE and MAP_ANONYMOUS are not both set.
     * Note that we should be checking that one of these bits
     * bits is set, but we cannot because older applications
     * did not use MAP_FILE.
     */
    if ((uap->flags & (MAP_FILE|MAP_ANONYMOUS)) == (MAP_FILE|MAP_ANONYMOUS)) {
	u.u_error = EINVAL;
	return;
    }

    /*
     * Check that MAP_VARIABLE and MAP_FIXED are not both set.
     * Note that we should be checking that one of these bits
     * bits is set, but we cannot because older applications
     * did not use MAP_VARIABLE.
     */
    if ((uap->flags & (MAP_VARIABLE|MAP_FIXED)) == (MAP_VARIABLE|MAP_FIXED)) {
	u.u_error = EINVAL;
	return;
    }

    /*
     * Do not allow a zero length memory mapped file.
     */
    if (size == 0) {
	u.u_error = EINVAL;
	return;
    }

    /*
     * Check that address and offset are page aligned.  Note that
     * AES specifies that the addr must be page aligned, even if
     * MAP_FIXED is not specified in the flags.
     */
    if (pagedown(off) != (u_int)off) {
	u.u_error = EINVAL;
	return;
    }
    off = btop(off);

    if ((caddr_t)pagedown(uap->addr) != uap->addr) {
	u.u_error = EINVAL;
	return;
    }

    /*
     * Only PROT_READ, PROT_WRITE and PROT_EXEC bits may be
     * set.
     */
    if ((uap->prot & ~(PROT_READ|PROT_WRITE|PROT_EXEC)) != 0) {
	u.u_error = EINVAL;
	return;
    }

    /*
     * If PROT_NONE was requested, we really map with PROT_READ, but
     * then use mprotect to protect the new pregion.  The user will
     * have to call mprotect() on this pregion eventually, so we might
     * as do it now, since the rest of the kernel does not expect a
     * pregion to be PROT_NONE unless it has been mprotected.
     */
    if (uap->prot == PROT_NONE) {
	map_none = 1;
	uap->prot = PROT_USER|PROT_READ;
    }
    else {
	map_none = 0;
	uap->prot |= PROT_USER;
    }

    pflags = (uap->flags & MAP_FIXED ? PF_EXACT : 0);

    /*
     * Do not allow a MAP_FIXED request at address 0.
     */
    if (pflags != 0 && uap->addr == 0) {
	u.u_error = ENOMEM;
	return;
    }

    vp = NULL;

    /*
     * Because of the implementation of hdl_attach() on PA-RISC,
     * hdl_attach() may need to override the returned address for the
     * attached pregion (i.e. so smmap() something different from
     * prp->p_vaddr.  What it does is to set u.u_rval1 to the correct
     * value, but only if u.u_rval1 is 0.  So, we must initialize
     * u.u_rval1 to 0 before we call attachreg().
     */
    u.u_rval1 = 0;

    vmemp_lock();		/* lock down vm empire */
    if (uap->flags & MAP_ANONYMOUS) {
	ushort reg_type;

	/*
	 * We can always grant write access to an anonymous
	 * memory mapped region, both MAP_SHARED and MAP_PRIVATE.
	 */
	pflags |= PF_WRITABLE;

	if (uap->flags & MAP_PRIVATE)
	    reg_type = RT_PRIVATE;
	else
	    reg_type = RT_SHARED;

	/*
	 * If this process might have holes in its address space, we
	 * try to find a hole that will satisfy this request.
	 */
	if (vas->va_flags & VA_HOLES) {
	    caddr_t new_addr;

	    /*
	     * Call smmap_hole_anonymous() to search for a hole.  We
	     * pass it the real prot, even if it was PROT_NONE.
	     */
	    new_addr = smmap_hole_anonymous(vas, uap->addr, reg_type,
				size, map_none ? PROT_NONE : uap->prot);

	    /*
	     * If smmap_hole_anonymous() returned -1, there is some sort
	     * of error that will prevent us from mapping the requested
	     * space.  Return right away (u.u_error has been set for us
	     * already).
	     */
	    if (new_addr == (caddr_t)-1)
		vmemp_return();

	    /*
	     * If new_addr is non-zero, smmap_hole_anonymous() found a
	     * hole and had completed the mapping.  We are done, so all
	     * we need to do is set u.u_rval1 to the new address and
	     * return.
	     */
	    if (new_addr != (caddr_t)0) {
		u.u_rval1 = (u_int)new_addr;
		vmemp_return();
	    }

	    /*
	     * If the user supplied a non-zero address, but did not set
	     * MAP_FIXED, we try searching again for a hole, but this
	     * time passing 0 as the address (smmap_hole_anonymous()
	     * treats a non-0 address as a fixed request).
	     */
	    if (uap->addr != 0 && !(pflags & PF_EXACT)) {
		new_addr = smmap_hole_anonymous(vas, (caddr_t)0, reg_type,
				    size, map_none ? PROT_NONE : uap->prot);
		if (new_addr == (caddr_t)-1)
		    vmemp_return();
		if (new_addr != (caddr_t)0) {
		    u.u_rval1 = (u_int)new_addr;
		    vmemp_return();
		}
	    }

	    /*
	     * No hole was found that can satisfy this request.  Fall
	     * through and try to allocate a new region and pregion to
	     * satisfy this request.
	     */
	}

	if ((rp = allocreg((struct vnode *)0, swapdev_vp, reg_type)) == NULL) {
	    u.u_error = ENOMEM;
	    goto bad;
	}

	if (growreg(rp, size, DBD_DZERO) == -1) {
	    u.u_error = ENOMEM;
	    goto bad;
	}
	if ((prp = attachreg(u.u_procp->p_vas, rp, uap->prot, PT_MMAP,
			     pflags, uap->addr, 0, size)) == NULL) {
	    u.u_error = ENOMEM;
	    goto bad;
	}
    }
    else {
	/*
	 * Make sure that fd is a valid file descriptor.
	 */
	GETF(fp, ((struct a *)u.u_ap)->fdes);

	/*
	 * Get a pointer to the asscoiated vnode.
	 * Check that the file is a regular file.
	 */
	vp = (struct vnode *)fp->f_data;
	if (vp->v_type != VREG) {
	    u.u_error = ENODEV;
	    vmemp_return();
	}

	/*
	 * All files must be readable to be mmap()ed.
	 */
	if (!(fp->f_flag & FREAD)) {
	    u.u_error = EACCES;
	    vmemp_return();
	}

	/*
	 * Determine if the new pregion can be written to.
	 */
	if (uap->flags & MAP_SHARED) {
	    if (fp->f_flag & FWRITE) {
		    pflags |= PF_WRITABLE;
	    }
	}
	else {
	    pflags |= PF_WRITABLE;
	}

	/*
	 * If they asked for write access, and the pregion is not
	 * writable, return EACCES.
	 */
	if ((uap->prot & PROT_WRITE) && !(pflags & PF_WRITABLE)) {
	    u.u_error = EACCES;
	    vmemp_return();
	}

	/*
	 * Check access permissions.
	 */
	if ((uap->prot & PROT_EXECUTE) && (uap->flags & MAP_SHARED)) {
	    /*
	     * Make sure that it is executable.
	     */
	    if (u.u_error = VOP_ACCESS(vp, VEXEC, u.u_cred))
		goto bad;

	    /*
	     * If opening for read-execute set VTEXT and make sure
	     * no-one already has this guy open for write.
	     */
	    if (!(uap->prot & PROT_WRITE)) {
		if ((fp->f_flag & FWRITE) || openforwrite(vp, 1)) {
		    u.u_error = ETXTBSY;
		    goto bad;
		}
		pflags |= PF_VTEXT;
	    }
	}

	/*
	 * If this process might have holes in its address space, we
	 * try to find a hole that will satisfy this request.
	 */
	if (vas->va_flags & VA_HOLES) {
	    caddr_t new_addr;
	    int reg_type = (uap->flags & MAP_PRIVATE) ? RT_PRIVATE : RT_SHARED;

	    /*
	     * Call smmap_hole_file() to search for a hole.  We pass
	     * it the real prot, even if it was PROT_NONE.
	     */
	    new_addr = smmap_hole_file(vas, uap->addr, reg_type, size,
		vp, off, pflags, map_none ? PROT_NONE : uap->prot);

	    /*
	     * If smmap_hole_file() returned -1, there is some sort of
	     * error that will prevent us from mapping the requested
	     * space.  Return right away (u.u_error has been set for
	     * us already).
	     */
	    if (new_addr == (caddr_t)-1) {
                if (pflags & PF_VTEXT)
                    dectext(vp, 1);
		vmemp_return();
            }

	    /*
	     * If new_addr is non-zero, smmap_hole_file() found a hole
	     * and had completed the mapping.  We are done, so all we
	     * need to do is set u.u_rval1 to the new address and
	     * return.
	     */
	    if (new_addr != (caddr_t)0) {
		/*
		 * Since we incremented the text reference when we
		 * called openforwrte(), but did not add a pregion
		 * to this region, we need to re-adjust the text
		 * reference count for this vnode.
		 */
		if (pflags & PF_VTEXT)
		    dectext(vp, 1);
		u.u_rval1 = (u_int)new_addr;
		vmemp_return();
	    }

	    /*
	     * If the user supplied a non-zero address, but did not set
	     * MAP_FIXED, we try searching again for a hole, but this
	     * time passing 0 as the address (smmap_hole_file() treats
	     * a non-0 address as a fixed request).
	     */
	    if (uap->addr != 0 && !(pflags & PF_EXACT)) {
		new_addr = smmap_hole_file(vas, (caddr_t)0, reg_type, size,
		    vp, off, pflags, map_none ? PROT_NONE : uap->prot);

		if (new_addr == (caddr_t)-1) {
                    if (pflags & PF_VTEXT)
                        dectext(vp, 1);
		    vmemp_return();
                }

		if (new_addr != (caddr_t)0) {
		    /*
		     * Since we incremented the text reference when we
		     * called openforwrte(), but did not add a pregion
		     * to this region, we need to re-adjust the text
		     * reference count for this vnode.
		     */
		    if (pflags & PF_VTEXT)
			dectext(vp, 1);
		    u.u_rval1 = (u_int)new_addr;
		    vmemp_return();
		}
	    }

	    /*
	     * No hole was found that can satisfy this request.  Fall
	     * through and try to allocate a new region and pregion to
	     * satisfy this request.
	     */
	}

	/*
	 * 8.0 hack to allow public shared libraries for PA.
	 * Should be replaced with OSF style installation of
	 * public shared libraries on all architectures.
	 * We will only allow a public mapping if we are mapping
	 * shared (and thus read-only for now) and if the file's
	 * modes meet our tests (no ACL, mode r-xr-xr-x).
	 *
	 * 910103 -- Well, we'll fix up the acl reference to
	 * be under ifdef ACLS, (for DSDe403769), but this whole
	 * concept doesn't really fly in a "secure" environment....
	 */
	if ((uap->flags & MAP_SHARED) && (pflags & PF_VTEXT)) {
#	    define RWX	  (VREAD | VWRITE | VEXEC)
#	    define RWXRWXRWX  (RWX | (RWX >> 3) | (RWX >> 6))
#	    define RX	  (VREAD | VEXEC)
#	    define RXRXRX 	  (RX | (RX >> 3) | (RX >> 6))
	    struct vattr vattr;

	    u.u_error = VOP_GETATTR(vp, &vattr, u.u_cred, VIFSYNC);
	    if (u.u_error != 0)
	    	goto bad;
	    if ((vattr.va_mode & RWXRWXRWX) == RXRXRX
#ifdef ACLS
		&& !vattr.va_acl
#endif
						      ) {
		pflags |= PF_PUBLIC;
	    }
#	    undef RWX
#	    undef RWXRWXRWX
#	    undef RX
#	    undef RXRXRX
	}

	/*
	 * Map the vnode.
	 */
	if (mapvnode(vp, off, size)) {
	    /*
	     * If mapvnode() said we could not map because the block
	     * size of the underlying filesystem is not a multiple of
	     * NBPG, leave errno alone.
	     */
	    if (u.u_error != EINVAL)
		u.u_error = ENOMEM;
	    goto bad;
	}
	has_mapped = 1;
	rp = vnodereg(vp);
	VASSERT(rp);
	reglock(rp);

	/*
	 * If mapping is private then duplicate region
	 * and attach it.
	 */
	if (uap->flags & MAP_PRIVATE) {
	    reg_t *dup_rp;

	    if ((dup_rp = dupreg(rp, RT_PRIVATE, off, size)) == NULL) {
		u.u_error = ENOMEM;
		goto bad;
	    }
	    if ((prp = attachreg(u.u_procp->p_vas, dup_rp, uap->prot, PT_MMAP,
				 pflags, uap->addr, 0, size)) == NULL) {
		freereg(dup_rp);
		u.u_error = ENOMEM;
		goto bad;
	    }

	    /*
	     * Release the source region and set rp to the
	     * new region.
	     */
	    regrele(rp);
	    rp = dup_rp;
	}
	else {
	    /*
	     * If we will be creating a writable mapping, we must keep
	     * a reference to a file table pointer that has this vnode
	     * open for write.  This is needed so that the i_writesites
	     * map for DUX are kept in sync with reality.  We also need
	     * this so we can page out over NFS with the correct
	     * credentials.
	     */
	    if ((fp->f_flag & FWRITE) && vp->v_vas->va_fp == (struct file *)0) {
		SPINLOCK(file_table_lock);
		fp->f_count++;
		vp->v_vas->va_fp = fp;
		vp->v_vas->va_wcount = 0;
		SPINUNLOCK(file_table_lock);
            } 

	    /*
	     * If we do not have one already, save a pointer to
	     * the credential struct associated with this file.
	     * But don't do this for root, because we don't want
	     * another non-root process to fail a page out from 
	     * using root's credentials.
	     */
            if ((fp->f_flag & FWRITE) && vp->v_vas->va_rss == 0 &&
                fp->f_cred->cr_uid != 0) {
	        crhold(fp->f_cred);
	        vp->v_vas->va_rss = (u_int)fp->f_cred;
	    }

	    /*
	     * Since the pregion hanging off the vnode has
	     * already added this region to the active chain,
	     * mark this pregion PF_NOPAGE to keep from inserting
	     * it on the active chain so vhand doesn't
	     * scan the region twice.
	     */
	    if ((prp = attachreg(u.u_procp->p_vas, rp, uap->prot, PT_MMAP,
				 pflags | PF_NOPAGE,
				 uap->addr, off, size)) == NULL) {
		u.u_error = ENOMEM;
		goto bad;
	    }
	}

        /*
         * For DUX vnodes, we must purge all pages for this vnode from
         * the page cache if the file contents have been changed.
         */
        if (vp->v_fstype == VDUX) {
            struct inode *ip = VTOI(vp);

            if (!(ip->i_flag & IPAGEVALID)) {
                int i, count;

                count = howmany(ip->i_size, DEV_BSIZE);
                for (i = 0; i < count; i+= NBPG/DEV_BSIZE)
                    munhash(vp, (daddr_t)i);
                ip->i_flag |= IPAGEVALID;
            }
        }
    }
    hdl_procattach(&u, u.u_procp->p_vas, prp);

    /*
     * Return the addr field of the prp.  See the comment above on
     * how hdl_attach() may override this and return something
     * different.
     */
    if (u.u_rval1 == 0)
	u.u_rval1 = (u_int)prp->p_vaddr;
#ifdef FSD_KI
    u.u_rval2 = (u_int)prp; /* passed back to KI with this kludge */
#endif
    regrele(rp);

    /*
     * If the request was for a pregion as PROT_NONE, use the mprotect()
     * mechanism to make it PROT_NONE.
     */
    if (map_none)
	foreach_pregion((caddr_t)u.u_rval1, ptob(size), do_mprotect, PROT_NONE);

    vmemp_unlock();
    return;

bad:
    /*
     * If we are MAP_ANONYMOUS we don't have a vnode assosiated with
     * this region and the region needs to be freed.  If this region
     * is associated with a vnode, unmapvnode will free the region
     * if this is the last reference to it.
     */
    if (uap->flags & MAP_ANONYMOUS) {
	VASSERT(vp == (struct vnode *)NULL);
	if (rp)
	    freereg(rp);
    }
    else {
	VASSERT(vp != (struct vnode *)NULL);
	if (rp)
	    regrele(rp);

	if (pflags & PF_VTEXT)
	    dectext(vp, 1);

	if (has_mapped) {
	    /*
	     * If we set vp->v_vas->va_fp, and the mapping failed, we
	     * may need to release our hold on the file table entry.
	     * The only time we need to do this is if a mapping failed
	     * before the pregion was attached to the region.  If a
	     * failure happens after the pregion is attached to the
	     * region, then detaching the pregion from the region will
	     * release the hold on the file table entry.
	     *
	     * Note that we manipulate va_wcount inside of the
	     * file_table_lock, to protect against MP race conditions.
	     */
	    SPINLOCK(file_table_lock);
	    if (vp->v_vas->va_fp != (struct file *)0 &&
		vp->v_vas->va_wcount == 0) {
		/*
		 * The only case where va_fp is non-null while va_wcount
		 * is 0 is if we setup the reference to the file but the
		 * attachreg() failed.  In this case, we know that the
		 * reference count on fp must be greater than one, since
		 * the file descriptor for it is still open.  Thus, we
		 * can avoid calling closef() and just adjust f_count
		 * locally.
		 */
		VASSERT(vp->v_vas->va_fp == fp);
		VASSERT(fp->f_count > 1);
		fp->f_count--;
		vp->v_vas->va_fp = (struct file *)0;

		/*
		 * NOTE: although we may have saved a pointer to the
		 * user credentials earlier, we cannot free them now,
		 * as there may be dirty pages that we still need to
		 * page out.  The credentials will be freed when the
		 * last unmapvnode() is done.
		 */
	    }
	    SPINUNLOCK(file_table_lock);

	    unmapvnode(vp);
	}
    }
    vmemp_unlock();
    return;
}

/*
 * munmap(addr, size) -- unmap a range of addresses
 */
munmap()
{
    register struct a {
	caddr_t addr;
	int	size;
    } *uap = (struct a *)u.u_ap;

    int do_munmap();

    vmemp_lock();		/* lock down the vm empire */

    /*
     * Check that address is page aligned.
     */
    if ((caddr_t)pagedown(uap->addr) != uap->addr) {
	u.u_error = EINVAL;
	vmemp_unlock();
	return;
    }

    foreach_pregion(uap->addr, uap->size, do_munmap, (void *)0);
    vmemp_unlock();
}

/*
 * do_munmap() --
 *    called by foreach_pregion() to perform an munmap() on pages within
 *    a single pregion.
 */
int
do_munmap(prp, idx, count, arg)
preg_t *prp;
int idx;
int count;
void *arg;
{
    void do_private_munmap();
    void do_shared_munmap();

    /*
     * Only allow munmap() on PT_MMAP pregions.
     */
    if (prp->p_type != PT_MMAP) {
	regrele(prp->p_reg);
	u.u_error = EINVAL;
	return 1;
    }

    /*
     * Are we unmapping the entire pregion?
     */
    if (idx == 0 && count == prp->p_count) {
	/*
	 * We must save vp and is_vtext before calling detachreg(),
	 * since after calling detachreg(), we can no longer reference
	 * things through "prp".
	 */
	struct vnode *vp = prp->p_reg->r_fstore;
	int is_vtext = prp->p_flags & PF_VTEXT;

	hdl_procdetach(&u, u.u_procp->p_vas, prp);
	detachreg(u.u_procp->p_vas, prp);

	if (vp != (struct vnode *)NULL) {
	    if (is_vtext)
		dectext(vp, 1);
	    unmapvnode(vp);
	}
	return 0;
    }

    /*
     * We will be doing a partial unmapping of a pregion.  This means
     * that smmap() needs to check for unmapped ranges in existing
     * pregions when attempting to allocate more address space.  We do
     * not want to do this all the time, so we use a hint in the vas so
     * that only processes that have done a partial unmapping take the
     * (arguably small) performance hit.
     */
    prp->p_vas->va_flags |= VA_HOLES;

    /*
     * A partial unmapping, lets worry about private mappings
     * first.
     */
    if (prp->p_reg->r_type == RT_PRIVATE)
	do_private_munmap(prp, idx, count);
    else
	do_shared_munmap(prp, idx, count);

    /*
     * Now that we have done all of that work, it is possible that the
     * entire pregion has now been mprotected() to be unmapped.  Check
     * for this, and if so, we call do_munmap() [recursively] to unmap
     * the entire pregion.
     */
    if (hdl_range_unmapped(prp, 0, prp->p_count))
	return do_munmap(prp, 0, (int)prp->p_count, (void *)0);

    regrele(prp->p_reg);
    return 0;
}

/*
 * do_private_munmap() --
 *    This routine is used to perform a partial unmapping of a private
 *    memory mapped region.
 */
void
do_private_munmap(prp, idx, count)
preg_t *prp;
int idx;
int count;
{
    reg_t *rp = prp->p_reg;

    /*
     * Make sure we are not called for cases we do not expect.
     */
    VASSERT(prp->p_type == PT_MMAP);
    VASSERT(rp->r_type == RT_PRIVATE);
    VASSERT(count < prp->p_count);
    VASSERT(rp->r_bstore == swapdev_vp);

    /*
     * If we are throwing away the end of this pregion, use growreg() to
     * do all of the work.  However, we only do this if the pregion has
     * not been mprotected().  We handle mprotected() pregions just like
     * we handle unmapping from the beginning or the middle, by using
     * mprotect().  Note that since we are shrinking the region, the
     * third parameter to growreg() (DBD_DZERO) does not mean anything,
     * it is only used when growing a region.
     */
    if (idx + count == prp->p_count && !IS_MPROTECTED(prp)) {
	growreg(rp, -count, DBD_DZERO);
	prp->p_count -= count;
	return;
    }

    /*
     * Either the pregion is currently mprotected(), or we are throwing
     * away a chunk at the beginning or in the middle of this pregion.
     * We will use mprotect() to unmap the hole.
     *
     * First throw away the pages and deallocate any swap space and
     * other reserved resources associated with them.
     */
    pgfree(rp, prp->p_off + idx, count);
    release_swap(count);
    rp->r_swalloc -= count;

    /*
     * If the pregion was locked in memory, we unreserve the amount
     * that we have just destroyed.
     */
    if (rp->r_mlockcnt != 0)
	lockmemunreserve(count);

    /*
     * Finally mark these pages as unmapped, if the user attempts
     * to access any of these, they will get a SIGSEGV.
     */
    hdl_mprotect(prp, idx, count, MPROT_UNMAPPED);
}

/*
 * do_shared_munmap() --
 *    unmap pages within a shared pregion.  This routine is only
 *    called for *partial* unmapping of shared pregions of type
 *    PT_MMAP.
 */
void
do_shared_munmap(prp, idx, count)
preg_t *prp;
int idx;
int count;
{
    extern int do_deltransc();
    extern struct vnode *swapdev_vp;
    reg_t *rp = prp->p_reg;
    int p_off = prp->p_off;
    int mmf;		/* 1 if MAP_FILE, 0 if MAP_ANONYMOUS */
    int not_shared;	/* is anyone else sharing this now? */
    int free_count;
    int i;

    /*
     * Make sure we are not called for cases we do not expect.
     */
    VASSERT(prp->p_type == PT_MMAP);
    VASSERT(rp->r_type == RT_SHARED);
    VASSERT(count < prp->p_count);

    mmf = (rp->r_fstore == rp->r_bstore);

    /*
     * We need to know if the underlying region is being shared by any
     * other other processes.  For MAP_FILE regions, we need to scan all
     * of the pregions hanging from the region and see if there are any
     * others with the same p_vaddr.  For MAP_ANONYMOUS regions, we can
     * just look at the region reference count.
     */
    if (mmf) {
	preg_t *prp2 = rp->r_pregs;

	for (; prp2 != (preg_t *)0; prp2 = prp2->p_prpnext)
	    if (prp2 != prp && prp2->p_vaddr == prp->p_vaddr)
		break; /* there is another pregion at the same place */

	not_shared = (prp2 == (preg_t *)0);
    }
    else {
	not_shared = (rp->r_refcnt == 1);
    }

    /*
     * Are we throwing away the end of a non-mprotected() pregion that
     * is not being shared by any other processes?  If so, we can shrink
     * the pregion, and for anonymous regions, shrink the region too.
     */
    if (not_shared && idx + count == prp->p_count && !IS_MPROTECTED(prp)) {
	if (mmf) {
	    /*
	     * Since this is an MMF, we cannot shrink the region.
	     * However, we still need to delete the translations for
	     * the given range of pages that is being unmapped.  Any
	     * valid pages will be paged out through the kernel pregion
	     * when needed.
	     */
	    foreach_chunk(rp, (int)(p_off + idx), count, do_deltransc,
		(caddr_t)prp);
	}
	else {
	    /*
	     * This is a MAP_ANONYMOUS region.  We want to free up any
	     * swap space and any valid pages that are being unmapped.
	     * growreg() does this all for us.
	     */
	    growreg(rp, -count, DBD_DZERO);
	}
#ifdef __hp9000s800
	/*
	 * We must free the shared address space that is no longer
	 * being used.
	 */
	quaddealloc34(prp->p_vaddr + ptob(prp->p_count - count), count);
#endif
	prp->p_count -= count;
	return;
    }

    /*
     * Are we throwing away the beginning of a non-mprotected() MMF
     * pregion that is not being shared by any other processes?  If so,
     * all we need to do is delete the translations for the chunk and
     * then adjust the pregion offset and count.
     */
    if (not_shared && mmf && idx == 0 && !IS_MPROTECTED(prp)) {
	foreach_chunk(rp, (int)p_off, count, do_deltransc, (caddr_t)prp);
#ifdef __hp9000s800
	/*
	 * We must free the shared address space that is no longer
	 * being used.
	 */
	quaddealloc34(prp->p_vaddr, count);
#endif
	prp->p_vaddr += ptob(count);
	prp->p_off += count;
	prp->p_count -= count;
	return;
    }

    /*
     * The underlying region is being shared by someone else, or we are
     * unmapping from the middle or a pregion.  We have to perform the
     * unmap by using mprotect().
     */
    hdl_mprotect(prp, idx, count, MPROT_UNMAPPED);

    /*
     * If this is a memory mapped file, there is nothing more to be
     * done, hdl_mprotect() did everything for us.
     */
    if (mmf)
	return;

    /*
     * This is a MAP_ANONYMOUS pregion, we must take care of releasing
     * swap space for portions that are no longer mapped by any process.
     *
     * If no other process is sharing this pregion, this is relatively
     * easy.
     */
    if (not_shared) {
	pgfree(rp, p_off + idx, count);
	release_swap(count);
	rp->r_swalloc -= count;
	if (rp->r_mlockcnt != 0)
	    lockmemunreserve(count);
	return;
    }

    /*
     * This is an anonymous shared pregion that is being used by at
     * least one other process.  We check each page in the range to
     * see if it is still mapped by anyone.  If not, we free it up,
     * otherwise we must leave it alone.
     */
    free_count = 0;
    for (i = idx; i < idx + count; i++) {
	caddr_t vaddr;

	/*
	 * Since the only way to share an MAP_ANONYMOUS region is by
	 * inheriting it across a fork(), we know that the p_vaddr
	 * of all pregions sharing this region is the same.
	 */
	prp = rp->r_pregs;
	vaddr = prp->p_vaddr + ptob(i);

	/*
	 * Look at the current page in each pregion.  Stop if we
	 * find one in which it is still mapped.
	 */
	for (; prp != (preg_t *)0; prp = prp->p_prpnext) {
	    if (hdl_page_mprot(prp, vaddr) != MPROT_UNMAPPED)
		break;
	}

	/*
	 * If prp is 0 here, that means that this page is no longer
	 * mapped by any other pregions.  We can free the page and
	 * associated swap resources for it (but we do all of the
	 * resource unreservation in one chunk for efficiency).
	 */
	if (prp == (preg_t *)0) {
	    pgfree(rp, p_off + i, 1);
	    free_count++;
	}
    }

    /*
     * Now unreserve all of the resources associated with the pages
     * we just freed.
     */
    if (free_count > 0) {
	release_swap(free_count);
	rp->r_swalloc -= free_count;
	if (rp->r_mlockcnt != 0)
	    lockmemunreserve(free_count);
    }
}

/*
 * mprotect(addr, len, how) -- change access on a memory region.
 */
mprotect()
{
    register struct a {
	caddr_t addr;
	size_t	len;
	int	prot;
    } *uap = (struct a *)u.u_ap;

    int do_mprotect();
    caddr_t addr = uap->addr;

    /*
     * Only PROT_READ, PROT_WRITE and PROT_EXEC bits may be set.
     */
    if ((uap->prot & ~(PROT_READ|PROT_WRITE|PROT_EXEC)) != 0) {
	u.u_error = EINVAL;
	return;
    }

    /*
     * The starting address must be page aligned.
     */
    if ((caddr_t)pagedown(addr) != addr) {
	u.u_error = EINVAL;
	return;
    }

    foreach_pregion(addr, uap->len, do_mprotect, uap->prot);
}

/*
 * do_mprotect() --
 *    called by foreach_pregion() to perform an mprotect() on pages within
 *    a single pregion.
 */
int
do_mprotect(prp, idx, count, how)
preg_t *prp;
int idx;
int count;
int how;
{
    int mprot;

    /*
     * For now, only allow mprotect() on PT_MMAP pregions.
     */
    if (prp->p_type != PT_MMAP) {
	regrele(prp->p_reg);
	u.u_error = EINVAL;
	return 1;
    }

    /*
     * If they want write access, and this pregion is not writable,
     * return EACCES.
     */
    if ((how & PROT_WRITE) && (prp->p_flags & PF_WRITABLE) == 0) {
	regrele(prp->p_reg);
	u.u_error = EACCES;
	return 1;
    }

    /*
     * If they want execute access and this pregion will not allow it,
     * return EACCES.
     *
     * We allow the pregion to be promoted to execute if it is writable.
     * So, the only thing that we end up disallowing PROT_EXEC to be
     * set on are SHARED, read-only mappings.
     */
    if ((how & PROT_EXEC) && !(prp->p_prot & PROT_EXEC) &&
			     !(prp->p_flags & PF_WRITABLE)) {
	regrele(prp->p_reg);
	u.u_error = EACCES;
	return 1;
    }

    /*
     * We do not allow mprotect() on public shared libraries (at least
     * for now, should probably fix this later).
     */
    if (prp->p_flags & PF_PUBLIC) {
	regrele(prp->p_reg);
	u.u_error = EINVAL;
	return 1;
    }

    /*
     * Determine the new protections.  Unless they specified PROT_NONE,
     * we always give PROT_READ and PROT_EXEC, even if they did not ask
     * for it.  So, the only thing we really care about is PROT_WRITE.
     */
    if (how == PROT_NONE)
	mprot = MPROT_NONE;
    else if (how & PROT_WRITE)
	mprot = MPROT_RW;
    else
	mprot = MPROT_RO;

    /*
     * Change the protections on the pages.  This cannot fail.
     */
    hdl_mprotect(prp, idx, count, mprot);

    regrele(prp->p_reg); /* we are responsible for doing this */
    return 0;
}

/*
 * msync(addr, len, flags) -- synchronize a mapped file.
 */
msync()
{
    register struct a {
	caddr_t addr;
	size_t	len;
	int	flags;
    } *uap = (struct a *)u.u_ap;

    int do_msync();
    caddr_t addr;	/* starting address (bytes) */
    int flg;		/* flags for VOP_PAGEOUT */

    /*
     * Validate parameters --
     *   . flags must contain only a combination of MS_SYNC,
     *     MS_ASYNC and MS_INVALIDATE.
     *   . flags must not have both MS_SYNC and MS_ASYNC set.
     *   . addr must be page aligned.
     */
    addr = uap->addr;
    flg = uap->flags;
    if ((flg & ~(MS_SYNC|MS_ASYNC|MS_INVALIDATE)) != 0 ||
	(flg & (MS_SYNC|MS_ASYNC)) == (MS_SYNC|MS_ASYNC) ||
	(caddr_t)pagedown(addr) != addr) {
	    u.u_error = EINVAL;
	    return;
    }

    /*
     * Compute the argument to VOP_PAGEOUT.
     */
    flg = PAGEOUT_HARD;
    if (uap->flags & MS_SYNC)
	flg |= PAGEOUT_WAIT;
    if (uap->flags & MS_INVALIDATE)
	flg |= PAGEOUT_FREE|PAGEOUT_PURGE;

    foreach_pregion(addr, uap->len, do_msync, flg);

    /*
     * If part of [addr..addr+len) was not valid for our address
     * space, foreach_pregion() will have set u.u_error to EFAULT.
     * However, for some strange reason, msync() is defined that such
     * an error should return ENOMEM, so we map EFAULT to ENOMEM.
     */
    if (u.u_error == EFAULT)
	u.u_error = ENOMEM;
}

/*
 * do_msync() --
 *    called by foreach_pregion() to perform an msync() on pages within
 *    a single pregion.
 */
int
do_msync(prp, idx, count, flags)
preg_t *prp;
int idx;
int count;
int flags;
{
    reg_t *rp = prp->p_reg;

    /*
     * Only PT_MMAP memory regions may be msynced.
     */
    if (prp->p_type != PT_MMAP) {
	regrele(rp);
	u.u_error = EINVAL;
	return 1;
    }

    /*
     * msync() is a no-op on all but MAP_SHARED regions.
     * We also do nothing if our back store is swap space,
     * (which is the case for MAP_ANONYMOUS|MAP_SHARED).
     */
    if (rp->r_type == RT_SHARED && rp->r_bstore != swapdev_vp) {
	/*
	 * Call the vnode pageout routine.  Currently, this
	 * will always be vfs_pageout().
	 */
	VOP_PAGEOUT(rp->r_bstore, prp, idx, idx + count - 1, flags);
    }

    regrele(rp); /* we are responsible for doing this */
    return 0;
}

/*
 * madvise(addr, len, behav) -- advise on expected paging behavior
 */
madvise()
{
    register struct a {
	caddr_t addr;
	size_t	len;
	int	behav;
    } *uap = (struct a *)u.u_ap;

    int do_madvise();
    caddr_t addr = uap->addr;

    /*
     * The "behav" argument must be one of the following.  Otherwise,
     * return EINVAL.
     */
    switch (uap->behav) {
    case MADV_NORMAL:
    case MADV_RANDOM:
    case MADV_SEQUENTIAL:
    case MADV_WILLNEED:
    case MADV_DONTNEED:
    case MADV_SPACEAVAIL:
	break;

    default:
	u.u_error = EINVAL;
	return;
    }

    /*
     * The starting address must be page aligned.
     */
    if ((caddr_t)pagedown(addr) != addr) {
	u.u_error = EINVAL;
	return;
    }

    foreach_pregion(addr, uap->len, do_madvise, (void *)0);
}

/*
 * do_madvise() --
 *    called by foreach_pregion() to perform an madvise() on pages within
 *    a single pregion.  Currently, we ignore madvise(), only only
 *    perform error checking on the arguments.
 */
int
do_madvise(prp, idx, count, arg)
preg_t *prp;
int idx;
int count;
void *arg;
{
    if (prp->p_type != PT_MMAP) {
	regrele(prp->p_reg);
	u.u_error = EINVAL;
	return 0;
    }

    /*
     * If we were going to do something, it would be done here...
     */

    regrele(prp->p_reg); /* we are responsible for doing this */
    return 0;
}


/*
 * do_invalidatec() --
 *    local function used by mtrunc() to invalidate a chunk
 *    of translations (both HI and HD layers).
 */
int
do_invalidatec(rp, idx, vd, count, arg)
reg_t *rp;
int idx;
struct vfddbd *vd;
register int count;
void *arg;
{
    extern dbd_t *default_dbd();
    int dealloc = 1;

    /*
     * The following is to avoid the dereference of the vnode if
     * DBDDEALLOC is never going to do anything.
     */
    if (rp->r_fstore == rp->r_bstore &&
	rp->r_fstore->v_op->vn_dbddealloc == NULL) {
	dealloc = 0;
    }

    for (; count--; idx++, vd++) {
	register vfd_t *vfd = &(vd->c_vfd);
	register dbd_t *dbd = &(vd->c_dbd);

	if (vfd->pgm.pg_v) {
	    register int pfn = vfd->pgm.pg_pfnum;
	    register pfd_t *pfd = &pfdat[pfn];

	    pfdatlock(pfd);
	    rp->r_nvalid--;
	    vfd->pgm.pg_v = 0;

	    hdl_unvirtualize(pfn);

	    if (PAGEINHASH(pfd))
		pageremove(pfd);
	    memfree(vfd);
	}
	vfd->pgi.pg_vfd = 0;

	/*
	 * Restore the DBD back to its default state.
	 */
	if (dealloc) {
	    switch (dbd->dbd_type) {
	    case DBD_FSTORE:
		VOP_DBDDEALLOC(rp->r_fstore, dbd);
		break;
	    case DBD_BSTORE:
		VOP_DBDDEALLOC(rp->r_bstore, dbd);
		break;
	    }
	}
	*dbd = *default_dbd(rp, idx);
    }
    return 0;
}

/*
 * do_inval_dbds() --
 *    local function used by regtrunc() to change the dbd_data
 *    field of a chunk of dbds to DBD_DINVAL (only for dbds
 *    that are DBD_FSTORE).
 */
int
do_inval_dbds(rp, idx, vd, count, arg)
reg_t *rp;
int idx;
struct vfddbd *vd;
register int count;
void *arg;
{
    VASSERT(rp->r_fstore->v_op->vn_dbddealloc == NULL);

    for (; count--; vd++) {
	register dbd_t *dbd = &(vd->c_dbd);

	if (dbd->dbd_type == DBD_FSTORE)
	    dbd->dbd_data = DBD_DINVAL;
    }
    return 0;
}

/*
 * regtrunc(rp, pglen, first_inval_dbd)
 *   given a region and a length (in pages) of the underlying file
 *   object, invalidate all pages past the given page (if any).
 *   Also sets the dbd_data field of all dbds from "first_inval_dbd"
 *   to the end of the file to DBD_DINVAL so that future pageins
 *   of those pages re-compute the disk address (because of fragment
 *   reallocation).
 */
void
regtrunc(rp, pglen, first_inval_dbd)
reg_t *rp;
int pglen;
int first_inval_dbd;
{
    off_t r_end;	/* file offset of end of region */
    int frag_start;	/* index where fragment starts */
    int frag_end;	/* index where fragment ends */

    /*
     * If this region maps a portion of the file past its new
     * length, invalidate all pages past that point.
     */
    r_end = rp->r_off + rp->r_pgsz;
    if (r_end > pglen) {
	int count;	/* number of pages to nuke */

	count = r_end - pglen;
	if (count >= rp->r_pgsz) {
	    /*
	     * The entire region is past the end of the file.
	     * Remove translations for the whole thing.
	     */
	    foreach_chunk(rp, 0, rp->r_pgsz, do_invalidatec, 0);
	}
	else {
	    /*
	     * Only the last "count" pages are past the end of
	     * the file.  Remove translations for just those.
	     */
	    foreach_chunk(rp, rp->r_pgsz - count, count,
			  do_invalidatec, 0);
	}
    }

    /*
     * Now invalidate DBDs for any potential fragments in this region.
     * If we do not need to do this (i.e. the file underlying this
     * region is a remote file), first_inval_dbd will be greater than
     * pglen.  In this case, we can return right away.
     */
    if (first_inval_dbd > pglen)
	return;

    /*
     * First, compute the page index of the range that we want
     * to invalidate.  This starts at the first invalid file
     * position and goes to the new end of the file.
     */
    frag_start = first_inval_dbd - rp->r_off;
    frag_end = pglen - rp->r_off;

    /*
     * We only need to do something if the end point of our range is
     * within the region (i.e. frag_end is greater than 0).
     */
    if (frag_end > 0) {
	/*
	 * The starting position may be before the start of this
	 * region.  In this case, frag_start will be negative.
	 * We want to adjust frag_start back to 0.
	 */
	if (frag_start < 0)
	    frag_start = 0;

	/*
	 * The end of this fragment may go past the end of the region.
	 * If so, adjust frag_end back to the last page of this region.
	 */
	if (frag_end > rp->r_pgsz)
	    frag_end = rp->r_pgsz;

	foreach_chunk(rp, frag_start, frag_end - frag_start,
		      do_inval_dbds, 0);
    }
}

/*
 * mtrunc(vp, len) --
 *   given a vnode pointer and a length, invalidate all pages that map
 *   to areas past the end of the file.
 *
 * NOTE: This operation is relatively expensive.  You should only call
 *       mtrunc() when the file is shrinking.
 */
void
mtrunc(vp, len)
struct vnode *vp;
size_t len;
{
    reg_t *rp;
    reg_t *head_rp;
    int first_inval_dbd;

    vnodelock(vp);
    if (vp->v_vas == (vas_t *)0) {
	vnodeunlock(vp);
	return;
    }

    /*
     * Compute the limit of trustworthy DBDs.  We must set the dbd_data
     * of all DBDs past this limit to DBD_DINVAL to force a remap of
     * the disk block (because of fragment reallocation).  This only
     * applies to VUFS files.
     */
    if (vp->v_fstype == VUFS) {
	register struct inode *ip = VTOI(vp);

	if (len >= (NDADDR << ip->i_fs->fs_bshift))
	    first_inval_dbd = btorp(len) + 1; /* no fragments */
	else {
	    int bsize = ip->i_fs->fs_bsize;

	    first_inval_dbd = btop(roundup(len, bsize) - bsize);
	}
    }
    else {
	/*
	 * A remote file.  Disk addresses are logical, not physical, so
	 * fragments are transparent.
	 */
	first_inval_dbd = btorp(len) + 1;
    }

    /*
     * We only invalidate pages *past* the end of the file, so we round
     * up the length to a page boundary.  Since all of our calculations
     * will be in units of pages, we convert the length to pages.
     */
    len = btorp(len);

    /*
     * Loop through all regions referencing this vnode.
     */
    head_rp = rp = vnodereg(vp);
    VASSERT(rp != (reg_t *)0);
    do {
	reg_t *rp_temp;
	int r_end;

	reglock(rp);

	/*
	 * If this region already ends at or before the new end of the
	 * file, we can just skip it.
	 */
	r_end = rp->r_off + rp->r_pgsz;
	if (r_end > len || r_end > first_inval_dbd) {
	    /*
	     * The vfds and dbds may be swapped out.  Swap them in.
	     */
	    if (rp->r_dbd)
		vfdswapi(rp);

	    regtrunc(rp, (int)len, first_inval_dbd);
	}

	rp_temp = rp->r_next;
	regrele(rp);

	/* Remove any semaphores that have been truncated */

	msem_trunc(rp, ptob(len));

	rp = rp_temp;
    } while (rp != head_rp);
    vnodeunlock(vp);
}

/*
 * This code created the vm data structures to map a vnode.  If the
 * caller knows that we do not need to grow the psuedo-pregion and
 * region (for example when duplicating an existing pregion), it
 * passes a '0' for the size parameter.
 */
int
mapvnode(vp, off, size)
	register struct vnode *vp;
	size_t off;
	size_t size;
{
	register preg_t *prp;
	register reg_t *rp;
        register struct vnode  *bstore;
	vm_sema_state;		/* semaphore save state */
	int bsize;
        struct vattr vattr;

	vmemp_lockx();		/* lock down VM empire */

	/*
	 * See if already mapped.
	 */
	vnodelock(vp);
	if (vp->v_vas != (vas_t *)NULL) {
		vas_t *vas = vp->v_vas;
		int ret = 0;	/* assume success */

		VASSERT(vp->v_flag & VMMF);

		/*
		 * Increment the vas reference count, and expand the
		 * psuedo-pregion to the correct size.
		 */
		vaslock(vas);
		vas->va_refcnt++;
		if (size != 0) {
			prp = (preg_t *)vas->va_next;
			rp = prp->p_reg;
			reglock(rp);
			if (growvnode(prp, off, size) != 0) {
				vas->va_refcnt--;
				ret = 1; /* failure */
			}
			regrele(rp);
		}
		vasunlock(vas);
		vnodeunlock(vp);
		vmemp_returnx(ret);
	}

	/*
	 * Make sure that the block size is a multiple of NBPG.  This
	 * *should* always be true for DUX and UFS, but NFS may have
	 * odd block sizes.
	 */
	switch (vp->v_fstype) {
	case VUFS:
	    bsize = VTOI(vp)->i_fs->fs_bsize;
	    break;

	case VDUX:
	    bsize = VTOI(vp)->i_dfs->dfs_bsize;
	    break;

	case VNFS:
	    bsize = vtoblksz(vp);
	    break;

	default:
	    /*
	     * VUFS, VNFS and VDUX are the only vnodes for which
	     * vfs_pagein() is used.  All other pagein routines
	     * (e.g. cdfs_pagein()) handle things themselves.
	     */
	    bsize = NBPG; /* to force it to be okay */
	    break;
	}

	if (bsize % NBPG != 0) {
	    u.u_error = EINVAL;
	    goto bad2;
	}

	/*
	 * Allocate a vas
	 */
	VASSERT((vp->v_flag & VMMF) == 0);
	if ((vp->v_vas = allocvas()) == (vas_t *)NULL)
		panic("mapvnode: allocvas failed");
	vasunlock(vp->v_vas);

	/*
	 * Create a region to hold the data.
         * If the file belongs to a remote file system and the sticky
         * bit is set then try to use the local swap device for the
         * text instead of using the executable image as the swap.
         * see dts DSDe405488
	 */
        if ((vp->v_fstype != VUFS) && (vp->v_flag & VTEXT) && !(VOP_GETATTR(vp,&vattr,u.u_cred,VASYNC)) && (vattr.va_mode & VSVTX))
                bstore = swapdev_vp;
        else
                bstore = vp;
        if ((rp = allocreg(vp, bstore, RT_SHARED)) == (reg_t *)NULL)
                goto bad;

#ifdef  OSDEBUG
        if ((vp->v_fstype != VUFS) &&(vp->v_flag & VTEXT) &&  (vattr.va_mode & VSVTX))
                 printf("mapvnode:Using the local swap for the remote text.\n");

#endif /* OSDEBUG */

	/*
	 * Attach the pregion -
	 * 	note: if vnode is bigger then 1 gig on HPPA we
	 *	need multiple pregions.
	 */
	VASSERT(size != 0);
	if ((prp = attachreg(vp->v_vas, rp, PROT_KRW, PT_MMAP, PF_EXACT,
					ptob(off), off, size)) ==
							(preg_t *)NULL) {
		freereg(rp);
		goto bad;
	}

	VASSERT(rp->r_pgsz == 0);
	if (growreg(rp, off + size, DBD_FSTORE) < 0) {
		/*
		 * growreg() could fail if we do not have enough swap
		 * space for the vfds and dbds for this region.
		 */
		detachreg(vp->v_vas, prp);
		goto bad;
	}

	/*
	 * Count the number of mapped pages created.
	 */
	cnt.v_nexfod += rp->r_pgsz;

	regrele(rp);

	/*
	 * Indicate that the vnode is mapped.
	 */
	vp->v_flag |= VMMF;

	/*
	 * Hold onto the vnode, this is released in unmapvnode().
	 */
	VN_HOLD(vp);
	update_duxref(vp, 1, 0);	/* reference for DUX */

	vnodeunlock(vp);
	vmemp_returnx(0);

bad:
	vaslock(vp->v_vas);
	VASSERT(vp->v_vas->va_refcnt == 1);
	vp->v_vas->va_refcnt--;
	freevas(vp->v_vas);
	vp->v_vas = (vas_t *)NULL;
bad2:
	vnodeunlock(vp);
	vmemp_returnx(1);
}

int
growvnode(prp, off, count)
preg_t *prp;
int off;
int count;
{
	reg_t *rp = prp->p_reg;
	int prp_change;
	int rp_change;

	VASSERT(rp->r_off == 0);
	VASSERT(prp->p_flags & PF_ACTIVE);

	/*
	 * If the psuedo-pregion starts after our new offset,
	 * move it back so that it matches our new offset.
	 * We must then adjust the address, vhand indexes and
	 * the pregion count.
	 */
	prp_change = prp->p_off - off;
	if (prp_change > 0) {
		prp->p_vaddr -= ptob(prp_change);
		prp->p_off -= prp_change;
		prp->p_count += prp_change;
		prp->p_agescan += prp_change;
		prp->p_stealscan += prp_change;
	}

	/*
	 * The psuedo-pregion and the region should always end at
	 * the same spot.
	 */
	VASSERT((prp->p_off + prp->p_count) == rp->r_pgsz);

	/*
	 * Grow the psuedo-pregion and the region so that they
	 * encompass our new range (off..off+size).
	 */
	rp_change = (off + count) - (prp->p_off + prp->p_count);
	if (rp_change > 0) {
		prp->p_count += rp_change;
		if (growreg(rp, rp_change, DBD_FSTORE) < 0) {
			/*
			 * growreg() could fail if we do not have enough
			 * swap space for the vfds and dbds for this
			 * region.  We need to backout all of the
			 * changes to the psuedo-pregion and return.
			 */
			prp->p_count -= rp_change;
			if (prp_change > 0) {
				prp->p_vaddr += ptob(prp_change);
				prp->p_off += prp_change;
				prp->p_count -= prp_change;
				prp->p_agescan -= prp_change;
				prp->p_stealscan -= prp_change;
			}
			return(1);
		}

		/*
		 * Count the number of mapped pages created.
		 */
		cnt.v_nexfod += rp_change;
	}
	return(0);
}

void
unmapvnode(vp)
	register struct vnode *vp;
{
	register vas_t *vas;

	vnodelock(vp);
	vas = vp->v_vas;

	VASSERT(vas != (vas_t *)NULL);
	VASSERT(vp->v_flag & VMMF);

	vaslock(vas);
	if (--vas->va_refcnt > 0) {
		/*
		 * Page out all of the pages through the shared psuedo
		 * region if the region reference count is 1.  We must
		 * do this or else we may have valid translations for
		 * addresses that we have returned to the resource map.
		 */
		preg_t *prp = vas->va_next;
		reg_t *rp = prp->p_reg;

		reglock(rp);
		if (rp->r_refcnt == 1 && rp->r_nvalid > 0) {
			/*
			 * Write out all modified pages to the disk.
			 */
			int flg = PAGEOUT_HARD|PAGEOUT_FREE;

			VOP_PAGEOUT(rp->r_bstore, prp, 0,
				    prp->p_count - 1, flg);
		}
		regrele(rp);

		vasunlock(vas);
		vnodeunlock(vp);
		return;
	}
	vp->v_vas = (vas_t *)NULL;
	vasunlock(vas);

	/*
	 * Now free all of the pregions.
	 */
	while (vas->va_next != (preg_t *)vas) {
		preg_t *prp;
		int flg;

		prp = vas->va_next;
		reglock(prp->p_reg);
		/*
		 * Write out all modified pages to the disk.
		 */
		flg = PAGEOUT_HARD|PAGEOUT_FREE;
		/* Page out DUX memory mapped files synchronously
		 * to avoid some nasty problems with the
		 * update_duxref(-1) happening before all the
		 * page I/O has completed.
		 */
       		if (vp->v_fstype == VDUX) {
			flg |= PAGEOUT_WAIT;
		}
		VOP_PAGEOUT(prp->p_reg->r_bstore, prp, 0,
			    prp->p_count - 1, flg);
		detachreg(vas, prp);
	}

	VASSERT(vas->va_refcnt == 0);

	/*
	 * For the last unmap of a vnode, we may still have a
	 * reference to a file table entry that we must get rid of.
	 * remove_pregion_from_region() usually takes care of this,
	 * but is called after unmapvnode().  Since we will be getting
	 * rid of the psuedo-vas for the vnode, we must take care of
	 * the file reference count here, and check in the function
	 * remove_pregion_from_region() for this situation.
	 *
	 * This may sound like a kludge, but it confines the impact
	 * of va_fp and va_wcount to managable places.
	 *
	 * Strange as it may seem, it is possible for va_wcount to be
	 * greater than 1 here.  This is because we set vp->v_vas to
	 * NULL earlier, before we started the VOP_PAGEOUT.  Since the
	 * vas is null, remove_pregion_from_region() will not adjust
	 * va_wcount for us.  So, the va_wcount can be more than one
	 * here, but that is okay.  We just set it to zero and free
	 * the file table entry.
	 */
	if (vas->va_wcount > 0) {
	    struct file *fp = vas->va_fp;

	    VASSERT(vas->va_wcount >= 1);
	    VASSERT(fp != (struct file *)0);
	    vas->va_wcount = 0;
	    vas->va_fp = (struct file *)0;
	    closef(fp);
	}

	if (vas->va_rss != 0) {
	    crfree((struct ucred *)vas->va_rss);
	    vas->va_rss = 0;
	}

	vaslock(vas);
	freevas(vas);

	/*
	 * Update the vnode reference counts.
	 */
	vp->v_flag &= ~VMMF;
	update_duxref(vp, -1, 0);	/* reference for DUX */
	VN_RELE(vp);

	vnodeunlock(vp);
}

#ifdef NEVER_CALLED
/*
 * Should this be locked ??
 */
vas_t *
vnodevas(vp)
	struct vnode *vp;
{
	return(vp->v_vas);
}

preg_t *
vnodeprp(vp, offset)
	struct vnode *vp;
	off_t offset;
{
	return(findprp(vp->v_vas, -1, (caddr_t)offset));
}
#endif /* NEVER_CALLED */

reg_t *
vnodereg(vp)
	register struct vnode *vp;
{
	register vas_t *vas;
	register reg_t *rp = (reg_t *)0;

	if ((vas = vp->v_vas) != 0) {
		vaslock(vas);
		if (vas->va_next)
			rp = vas->va_next->p_reg;
		vasunlock(vas);
	}
	return(rp);
}

/*
 * smmap_hole_anonymous() --
 *    Search the address space of a process for a PT_MMAP pregion of the
 *    right type containing a portion that is unmapped that we can use
 *    to satisfy the request for "size" pages.
 *
 *    If "addr" is non-zero, we will only succeed if we can map the
 *    requested space exactly at addr.
 *
 * Returns:
 *   -1 on error
 *    0 if no hole could be found
 *    the attach address if successful
 */
caddr_t
smmap_hole_anonymous(vas, addr, reg_type, size, prot)
vas_t *vas;
caddr_t addr;		/* requested attach address or 0 */
int reg_type;		/* RT_SHARED or RT_PRIVATE */
int size;		/* in pages */
short prot;		/* requested protection */
{
    preg_t *prp;
    reg_t *rp;
    int idx;

    /*
     * Map "prot" to an MPROT_* mode.
     */
    if (prot & PROT_WRITE)
	prot = MPROT_RW;
    else if (prot == PROT_NONE)
	prot = MPROT_NONE;
    else
	prot = MPROT_RO;

    for (prp = vas->va_next; prp != (preg_t *)vas; prp = prp->p_next) {
	/*
	 * If this is a pregion with the correct types and it might have
	 * holes in it, and it is not being shared by another process,
	 * see if it has a hole big enough for us to use.
	 */
	if (IS_MPROTECTED(prp) && prp->p_type == PT_MMAP &&
	    (rp = prp->p_reg)->r_type == reg_type &&
	    rp->r_fstore == (struct vnode *)0) {
	    /*
	     * We must lock the region before looking at its reference
	     * count.  Make sure that the reference count is one, we
	     * cannot re-use part of a shared anonymous pregion that
	     * is being shared, only one that is private or a shared
	     * one that only we are using.
	     */
	    reglock(rp);
	    if (prp->p_reg->r_refcnt != 1) {
		regrele(rp);
		continue;
	    }

	    /*
	     * If we get to choose the attach point, look for a hole
	     * anywhere in this pregion.
	     */
	    if (addr == (caddr_t)0) {
		idx = hdl_mprot_hole(prp, size);
	    }
	    else {
		/*
		 * The request was for a specific attach point, see if
		 * that is within this pregion and if so, if that range
		 * [addr, addr + size) is unmapped.
		 */
		idx = -1;
		if (addr >= prp->p_vaddr &&
		    addr < prp->p_vaddr + ptob(prp->p_count)) {
		    idx = btop(addr - prp->p_vaddr);
		    if (!hdl_range_unmapped(prp, idx, size))
			idx = -1;
		}
	    }

	    /*
	     * If there was not a hole in this pregion, release the
	     * region and keep searching.
	     */
	    if (idx == -1) {
		regrele(rp);
		continue;
	    }

	    /*
	     * We have found a hole that we can use.  We need to reserve
	     * resources for the pages we are about to allocate.
	     */
	    VASSERT((rp->r_flags & RF_SWLAZY) == 0);
	    if (!reserve_swap(rp, size, SWAP_NOWAIT)) {
		u.u_error = ENOMEM;
		regrele(rp);
		return (caddr_t)-1;
	    }

	    if (rp->r_mlockcnt != 0 && lockmemreserve(size) < 0) {
		release_swap(size);
		u.u_error = ENOMEM;
		regrele(rp);
		return (caddr_t)-1;
	    }
	    rp->r_swalloc += size;

	    /*
	     * Now call mprotect() to change the protections of these
	     * pages from MPROT_UNMAPPED to the correct mode.
	     */
	    hdl_mprotect(prp, idx, size, prot);

	    regrele(rp);
#ifdef FSD_KI
	    u.u_rval2 = (u_int)prp; /* passed back to KI with this kludge */
#endif
	    return prp->p_vaddr + ptob(idx);
	}
    }

    /*
     * Did not find anyplace to put this, tell smmap() that it will
     * have to allocate a new pregion.
     */
    return (caddr_t)0;
}

/*
 * smmap_hole_file() --
 *    Search the address space of a process for a PT_MMAP pregion of the
 *    right type containing a portion that is unmapped that we can use
 *    to satisfy the request for "size" pages.
 *
 *    If "addr" is non-zero, we will only succeed if we can map the
 *    requested space exactly at addr.
 *
 * Returns:
 *   -1 on error
 *    0 if no hole could be found
 *    the attach address if successful
 */
caddr_t
smmap_hole_file(vas, addr, reg_type, size, vp, off, pflags, prot)
vas_t *vas;
caddr_t addr;		/* requested attach address or 0 */
int reg_type;		/* RT_SHARED or RT_PRIVATE */
int size;		/* in pages */
struct vnode *vp;	/* which file to map */
int off;		/* the offset into the file (in pages) */
int pflags;		/* pregion flags that must match */
short prot;		/* requested protection */
{
    preg_t *prp;
    reg_t *rp;
    int idx;

    /*
     * Only PF_VTEXT and PF_WRITABLE must match.
     */
    pflags &= (PF_VTEXT|PF_WRITABLE);

    /*
     * Map "prot" to an MPROT_* mode.
     */
    if (prot & PROT_WRITE)
	prot = MPROT_RW;
    else if (prot == PROT_NONE)
	prot = MPROT_NONE;
    else
	prot = MPROT_RO;

    for (prp = vas->va_next; prp != (preg_t *)vas; prp = prp->p_next) {
	/*
	 * If this is a pregion with the correct types and it might have
	 * holes in it, and it is not being shared by another process,
	 * see if it has a hole big enough for us to use.  We are only
	 * interested in pregions that map the same file as 'vp'.
	 */
	if (IS_MPROTECTED(prp) && prp->p_type == PT_MMAP &&
	    (prp->p_flags & (PF_VTEXT|PF_WRITABLE)) == pflags &&
	    (rp = prp->p_reg)->r_type == reg_type && rp->r_fstore == vp) {
	    /*
	     * This region is for this file, and all of the types match.
	     * Now do some more checks, but first we must lock the
	     * region.
	     */
	    reglock(rp);

	    /*
	     * Compute the index into this pregion that corresponds to
	     * the requested file offset.  See if it then lies within
	     * this pregion.
	     */
	    idx = off - (prp->p_off + rp->r_off);
	    if (idx >= 0 && idx < prp->p_count) {
		/*
		 * This pregion might work, but make sure that if a
		 * specific attach point was requested, that it is the
		 * same as what we just found.  If not, we eep searching
		 * (there might be another one where we could be able to
		 * map it).
		 */
		if (addr != (caddr_t)0 && addr != prp->p_vaddr + ptob(idx)) {
		    regrele(rp);
		    continue;
		}

		/*
		 * Okay, so far so good.  Now see if the necessary range
		 * is unmapped.
		 */
		if (!hdl_range_unmapped(prp, idx, size)) {
		    regrele(rp);
		    continue;
		}
	    }
	    else {
		regrele(rp);
		continue;
	    }

	    /*
	     * If we get here, we have found a hole where we can do the
	     * mapping.  If the region type is RT_SHARED, all we need do
	     * is mprotect() the pages with the requested protections
	     * and we are done.
	     */
	    if (reg_type == RT_SHARED) {
		hdl_mprotect(prp, idx, size, prot);
		regrele(rp);
#ifdef FSD_KI
		u.u_rval2 = (u_int)prp; /* passed back to KI with this kludge */
#endif
		return prp->p_vaddr + ptob(idx);
	    }

	    /*
	     * This is a RT_PRIVATE region.  We must reserve the
	     * necessary resources for the pages that we are about
	     * to allocate.
	     */
	    VASSERT((rp->r_flags & RF_SWLAZY) == 0);
	    if (!reserve_swap(rp, size, SWAP_NOWAIT)) {
		u.u_error = ENOMEM;
		regrele(rp);
		return (caddr_t)-1;
	    }

	    if (rp->r_mlockcnt != 0 && lockmemreserve(size) < 0) {
		release_swap(size);
		u.u_error = ENOMEM;
		regrele(rp);
		return (caddr_t)-1;
	    }
	    rp->r_swalloc += size;

	    /*
	     * Now call mprotect() to change the protections of these
	     * pages from MPROT_UNMAPPED to the correct mode.
	     */
	    hdl_mprotect(prp, idx, size, prot);

	    regrele(rp);
#ifdef FSD_KI
	    u.u_rval2 = (u_int)prp; /* passed back to KI with this kludge */
#endif
	    return prp->p_vaddr + ptob(idx);
	}
    }

    /*
     * Did not find anyplace to put this, tell smmap() that it will
     * have to allocate a new pregion.
     */
    return (caddr_t)0;
}
