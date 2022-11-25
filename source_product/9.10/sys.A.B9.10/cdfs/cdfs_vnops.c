/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/cdfs/RCS/cdfs_vnops.c,v $
 * $Revision: 1.10.83.7 $        $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 95/01/20 15:23:34 $
 */

/* HPUX_ID: @(#)cdfs_vnops.c	54.7		88/12/07 */

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

/*	@(#)cdfs_vnodeops.c 1.1 86/02/03 SMI	*/
/*	@(#)cdfs_vnodeops.c	2.1 86/04/14 NFSSRC */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/conf.h"
#include "../h/kernel.h"
#include "../h/dir.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"
#include "../ufs/inode.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../nfs/nfs.h"
#include "../h/mount.h"
#include "../h/dnlc.h"
#include "../dux/dm.h"
#include "../dux/dux_dev.h"
#include "../dux/cct.h"
#ifdef POSIX
#include "../h/tty.h"
#include "../h/unistd.h"
#endif POSIX

#include "../h/vm.h"
#include "../machine/vmparam.h"
#include "../h/kern_sem.h"

#ifdef	MP
extern	sema_t filesys_sema;
#endif

extern int cdfs_bmap();
extern int cdfs_open();
extern int cdfs_close();
extern int cdfs_rdwr();
extern int cdfs_inval();

#define	cdfs_ioctl	cdfs_inval
#define	cdfs_select	cdfs_inval
#define	cdfs_readlink	cdfs_inval

extern int cdfs_getattr();

#define	cdfs_setattr	cdfs_badop

extern int cdfs_access();
extern int cdfs_lookup();
extern int cdfs_badop();

#define	cdfs_create	cdfs_badop
#define	cdfs_remove	cdfs_badop
#define	cdfs_link	cdfs_badop
#define	cdfs_rename	cdfs_badop
#define	cdfs_mkdir	cdfs_badop
#define	cdfs_rmdir	cdfs_badop
#define	cdfs_symlink	cdfs_badop

extern int cdfs_readdir();
extern int cdfs_fsync();
extern int cdfs_inactive();
extern int cdfs_strategy();

#define	cdfs_bread	cdfs_inval
#define	cdfs_brelse	cdfs_inval
#ifdef ACLS
#define	cdfs_setacl	cdfs_badop
#define	cdfs_getacl	cdfs_badop
#endif
#ifdef	POSIX
extern int cdfs_fpathconf();
#endif	POSIX

#define	cdfs_lockctl	cdfs_badop
#define	cdfs_lockf	cdfs_badop

extern int cdfs_fid();
extern int cdfs_fsctl();
extern int vfs_prefill();
extern int cdfs_pagein();
extern int vfs_pageout();

#define	FILE_CHUNK		4
#define	FILE_CHUNK_SHIFT	2 	/*log2(FILE_CHUNK)*/

struct vnodeops cdfs_vnodeops = {
	cdfs_open,
	cdfs_close,
	cdfs_rdwr,
	cdfs_ioctl,
	cdfs_select,
	cdfs_getattr,
	cdfs_setattr,
	cdfs_access,
	cdfs_lookup,
	cdfs_create,
	cdfs_remove,
	cdfs_link,
	cdfs_rename,
	cdfs_mkdir,
	cdfs_rmdir,
	cdfs_readdir,
	cdfs_symlink,
	cdfs_readlink,
	cdfs_fsync,
	cdfs_inactive,
	cdfs_bmap,
	cdfs_strategy,
	cdfs_bread,
	cdfs_brelse,
	cdfs_badop,
#ifdef ACLS
	cdfs_setacl,
	cdfs_getacl,
#endif
#ifdef	POSIX
	cdfs_fpathconf,
	cdfs_fpathconf,
#endif	POSIX
	cdfs_lockctl,
	cdfs_lockf,
	cdfs_fid,
	cdfs_fsctl,
	vfs_prefill,
	cdfs_pagein,
	vfs_pageout,
	NULL,
	NULL,
};


/******************************************************************************
 * cdfs_inval()
 *
 * Catch implementation problems.  This should not happen.
 */
cdfs_inval()
{
	panic ("cdfs_inval: illegal vnode operation");
}


/*****************************************************************************
 * cdfs_fsync()
 *
 * Sync doesn't apply to read-only file system.  But we don't want to issue
 * any error either.  We gladly reply, "I have done it."
 */
cdfs_fsync()
{
	return(0);
}


/*****************************************************************************
 * cdfs_badop()
 *
 * To catch any operation that involve "write" operation.  We say, "No, I can't
 * and I won't."
 */
cdfs_badop()
{
	return(EROFS);
}



/*ARGSUSED*/
cdfs_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	struct cdnode	*cdp;

	cdp = VTOCD(*vpp);

        /*
         * updatesitecount is apply to HP's diskless implemenation.  Other
         * implementations should follow their ufs_open model.
         */
        updatesitecount (&(cdp->cd_opensites), u.u_site, 1);
	return(0);
}




/*ARGSUSED*/
cdfs_close(vp, flag, cred)
	struct vnode *vp;
	int flag;
	struct ucred *cred;
{
	register struct cdnode *cdp = VTOCD(vp);;

        /*
         * updatesitecount is apply to HP's diskless implemenation.  Other
         * implementations should follow their ufs_close model.
         */
	updatesitecount (&(cdp->cd_opensites), u.u_site, -1);
	return(0);
}


/*****************************************************************************
 * cdfs_rdwr()
 *
 * read/write routine called from vnode layer.  "Write" operation will be
 * rejected right away.
 *
 * vp:      vnode of the accessed file
 * uiop:    uio structure that tells the details of the request
 * rw;      Is this is a read or write request.
 * ioflag:  not used.
 * cred:    not used.
 */
/*ARGSUSED*/
cdfs_rdwr(vp, uiop, rw, ioflag, cred)
struct vnode	*vp;
struct uio	*uiop;
enum uio_rw	rw;
int	ioflag;
struct ucred	*cred;
{
	struct	cdnode	*cdp;
	enum vtype	type;
	struct	vnode	*devvp;


        /*
         * If this is not a read request, return error.
         */
	if (rw != UIO_READ) 
		return(EROFS);

        /*
         * If no more data requested, return now.
         */
	if (uiop -> uio_resid == 0) 
		return(0);

        /*
         * Sanity check to see if we have big troubles.
         */
	type = vp->v_type;
	switch (vp -> v_fstype) {
	case VCDFS:
                /*
                 * ISO-9660 and HSG have only two types of files, regular
                 * files and directories.
                 */
		if ((type != VREG) && (type != VDIR)) {
			printf("cdfs_rdwr: invalid file type\n");
			return(EINVAL);
		}
		cdp = VTOCD(vp);
		break;
	default:
		panic("cdfs_rdwr: invalid file system type");
	}

	/*if no seperate dux_cdnode type, we need to handle it here. */
	
	u.u_error = 0;
	devvp = cdp -> cd_devvp;
	return(cdfs_rd(devvp, vp, uiop));
}


/******************************************************************************
 * cdfs_rd()
 *
 * cdfs_rd reads blocks that specified in uio structure.
 *
 * devvp: vnode for the device that the file system is on.
 * vp:    vnode of the file interested.  In the case we need to read
 *        directly from the device,  vp is the same of devvp.
 * uiop:  pointer to uio structure.  Uio tells where to read, where to put
 *        the data, how much to read ...
 */
cdfs_rd(devvp, vp, uiop)
struct vnode	*devvp, *vp;
register struct uio	*uiop;
{
	register struct cdnode	*cdp;
	register struct buf	*bp;
	register enum vtype	type;
	register u_int		file_offset;
	register u_int		bn, n;

	u_int	size;
	u_int	read_size;
	u_int	on, sn;

	int	nextread;
	int	diff;


        /*
         * If nothing left to read, return now.
         */
	if (uiop->uio_resid == 0)
		return (0);

	type = vp->v_type;
	switch(vp->v_fstype) {
	case VCDFS:
		cdp = VTOCD(vp);
		break;
	case VDEV_VN:
                /*
                 * If we want certain blocks from the device, we set type
                 * to VBLK to tell the code follows to bypass file system.
                 */
		if (type == VNON) {
			type = VBLK;
			break;
		}
	default:
		panic("cdfs_rd: illegal file type");
	}

	u.u_error = 0;
	do {
		file_offset = uiop->uio_offset;

                /*
                 * If type is VBLK, there is no need to go through file system.
                 */
		if (type == VBLK) {
			read_size = size = CDSBSIZE;
			bn = (file_offset/CDSBSIZE) * (CDSBSIZE/DEV_BSIZE);
			on = file_offset % CDSBSIZE;
			n = MIN(uiop->uio_resid, size - on);

                	/*
                	 * No need to keep the buffer in cache and no need 
			 * to read ahead.
                	 */
			rablock = 0;

		} else {
                	/*
                     	 * Since we need to go through file system to figure
                	 * out where the blocks are, we call cd_bmap() to
                	 * do the dirty job. (Note:  It is cd_bmap() not
                	 * cdfs_bmap())  cd_bmap returns the sector number
                	 * in "sn", size of the valid data in "size" and the
                	 * offset of block where the data are interested.
                	 */
			if (cd_bmap(vp,file_offset,NULL,&sn,&size,&on)==-1) {
				return(0);
			}

                	 /*
                 	  * We only read in the minimum of "the number of bytes
                	  * users requested", "the number of bytes left in this
                	  * block" and "the remainder of the file being read".
                 	  */
			n = MIN(uiop->uio_resid, size - on);
			diff = cdp->cd_size - file_offset;
			if (diff <= 0) 
				return(0);
			if (diff < n) 
				n = diff;

			/*
			 * Convert the sector number into "disk block" 
			 * number that the drivers know about.  Note that 
			 * sector is the minimum addressable unit of the 
			 * CDROM.  cd_bmap() returns the sector number 
			 * regardless if the logic block is 512, 1024
			 * or 2048.
			 */
			bn = lstodb(cdp->cd_fs, sn);

                	/*
                	 * Prepare for read ahead.  Since to find out where the
                	 * next read should be, cd_bmap() has put the block 
			 * number for next read in "rablock".
                	 */
			nextread = cdp->cd_nextr;
			cdp->cd_nextr = rablock;
			read_size = scroundup(cdp->cd_fs, size);
		}

		/*
                 * Make sure the users are doing sequencial read before
                 * we ask for read ahead.
                 */
		if (rablock && (nextread == rablock)) {
#ifdef	FSD_KI
			bp = breada(devvp, bn, read_size, rablock, rasize,
					B_cdfs_rd|B_data);
#else	not FSD_KI
			bp = breada(devvp, bn, read_size, rablock, rasize);
#endif	FSD_KI
		} else {
#ifdef	FSD_KI
			bp = bread(devvp, bn, read_size, B_cdfs_rd|B_data);
#else	not FSD_KI
			bp = bread(devvp, bn, read_size);
#endif	FSD_KI
		}

		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return (EIO);
		}

                /*
                 * "n" is the number of bytes that will actually copied to
                 * the requestor.  Make sure "n" don't cause us to copy beyond
                 * what was read in.
                 */
		n = MIN(n, bp->b_bcount - bp->b_resid);

		u.u_error = uiomove(bp->b_un.b_addr+on, n, UIO_READ, uiop);
		brelse(bp);

	} while (u.u_error == 0 && uiop->uio_resid > 0 && n != 0);

	return (u.u_error);
}

/******************************************************************************
 * cdfs_getattr()
 *
 * Get file attributes.  Used by stat(2) call.
 */
/*ARGSUSED*/
cdfs_getattr(vp, vap, cred, vsync)
struct	vnode		*vp;
register struct vattr	*vap;
struct ucred		*cred;
enum vsync		vsync;
{
	register struct cdnode	*cdp;
	long time_temp;

	cdp = VTOCD(vp);
	vap->va_type=CDFTOVT(cdp->cd_ftype);
	vap->va_mode=cdp->cd_mode;
	vap->va_uid=cdp->cd_uid;
	vap->va_gid=cdp->cd_gid;
	vap->va_fsid=cdp->cd_dev;
#if defined(__hp9000s800) && !defined(_WSIO)
	/*
	 * On a diskless client, map_mi_to_lu will be a no-op
	 * leaving va_fsid unchanged since the mi->lu mapping
	 * is not present for a remote device.  This is the desired
	 * behavior.
	 */
	(void)map_mi_to_lu(&vap->va_fsid, IFBLK);
#endif /* hp9000s800 */


        /*
         * site and cnode id is used only on HP's Diskless implementation.
         */
	vap->va_fssite = my_site;
	vap->va_nodeid=(long)(cdp->cd_num);
	vap->va_nlink=1;
	vap->va_size=cdp->cd_size;

        /*
         * All files should have only post_time since all CD-ROM are recorded
         * after Jan 1, 1970.  For those disks that has record date before
         * genesis date of UN*X are not tell the truth.  In this case, the
         * application need to interpret what it means.
         */
	if (cdp->cd_record_t.pre_time != 0)
		time_temp = cdp->cd_record_t.pre_time;
	else
		time_temp = cdp->cd_record_t.post_time;


        /*
         * "All times are created equal." applies to Read-Only media :-)
         */
	vap->va_mtime.tv_sec = time_temp;	/*  all the same on  */
	vap->va_ctime.tv_sec = time_temp;	/*  R/O media	     */
	vap->va_atime.tv_sec = time_temp;	

        /*
         * va_rdev is the device for character/block special files.  Since
         * we have only regular files and directories to worry about, IT
         * IS 0.
         */
	vap->va_rdev=0;

        /*
         * va_realdev is the real device number of the device the file is on.
         * In the case of HP's diskless implementation, we need to jump through
         * a few loops to get it.  Else, it is cd_dev (or vap->va_fsid).
         */
	vap->va_realdev = (dev_t) (remotecdp(cdp) ? 
			     ((struct mount *)(vp->v_vfsp->vfs_data))->m_rdev :
			      vap->va_fsid);	

	vap->va_blocksize=cdp->cd_fs->cdfs_sec_size;
	vap->va_blocks = (lblkroundup(cdp->cd_fs, cdp->cd_size) + 
			              DEV_BSIZE-1)/DEV_BSIZE;

	vap->va_fstype = MOUNT_CDFS;
	return(0);
}


/******************************************************************************
 * cdfs_access()
 *
 * Check the accessibilty of the file represented by vp.
 */
/*ARGSUSED*/
cdfs_access(vp, mode, cred)
struct	vnode	*vp;
int	mode;
struct ucred	*cred;
{
	register struct cdnode	*cdp;
	int	error;
	sv_sema_t ss;

	cdp=VTOCD(vp);
	PXSEMA(&filesys_sema, &ss);
	cdlock(cdp);
	error=cdaccess(cdp, mode);
	cdunlock(cdp);
	VXSEMA(&filesys_sema, &ss);
	return(error);
}



/******************************************************************************
 * get_dir_buffer()
 *
 * Fill a buffer with raw file descriptor data of a directory that contains
 * the directory entry pointed by cd_diroff.
 * Returns 0 when succeed.
 */
int
get_dir_buffer(dcdp,bpp) /* returns an error 0=ok */
register struct cdnode *dcdp;
struct buf   **bpp;
{
	register struct cdfs	*cdfsp = dcdp->cd_fs;
	int     offset = dcdp->cd_diroff;
	int	fchunk;
	int	bsize;
	int	bn;
	int	size;

        /*
         * Attempt to read beyond the end of directory. Dommmm...
         */
	if (offset >= dcdp->cd_size) 
		return(EIO);

        /*
         * Attempt to read 8K when ever possible.
         */
	bsize = cdfsp->cdfs_sec_size << FILE_CHUNK_SHIFT;

	fchunk = offset & ~(bsize-1);
	size = min(bsize, dcdp->cd_size-fchunk);

        /*
         * Find out where to start reading.  Note that directories cannot be
         * recored in the interleave mode, it is so much easier to figure
         * out where to read.
         */
	bn = lstodb(cdfsp, lbtolsb(cdfsp, dcdp->cd_loc + dcdp->cd_xarlen) + 
		          cdlsno(cdfsp, fchunk));
#ifdef FSD_KI
	*bpp = bread(dcdp->cd_devvp, bn, scroundup(cdfsp, size),
		B_cdfs_rd|B_data);
#else
	*bpp = bread(dcdp->cd_devvp, bn, scroundup(cdfsp, size));
#endif /* FSD_KI */
	if ((*bpp)->b_flags & B_ERROR) {
		brelse(*bpp);
		*bpp = 0;
		return(EIO);
	}
	return(0);
}


/*****************************************************************************
 * get_dir_rec()
 *
 * Given a cdnode we return a directory entry and the cdnode number for that
 * directory entry.  The directory entry is the one specified by cd_diroff
 * of the cdnode.  Before this function is called, the cd_diroff need to
 * be set to * the interest directory entry.  If truncate is ture, we only
 * copy size of a structure cddir only.  Else, we copy the whole entry
 * include system area.
 *
 * Note that cd_diroff is the offset from the first directory entry to the
 * interested directory entry.
 */
get_dir_rec(cdp, dir_recp, cdnop, truncate)
register struct cdnode	*cdp;
struct cddir		*dir_recp;
cdno_t			*cdnop;
int			truncate;
{
	struct	buf	*bp = 0;
	register int	offset;
	register int	buf_offset;
	register int	sector_size;
	register struct cddir  *tmp_cddirp;
	register int	rec_len;
	register int	bsize;
	int		tmp_loc;
	int		is_dir;
	int		error;

        /*
         * Cd_diroff always points to a valid directory entry.  It should
         * never go beyond the size of the directory itself.  If it ever
         * occure, we have a bug.  Since this is not a serious error, we
         * print a message to the console and reset cd_diroff.
         */
	if (cdp->cd_diroff > cdp->cd_size) {
		printf("get_dir_rec: cd_diroff is too big. reset.\n");
		cdp->cd_diroff = 0;
	}

        /*
         * Get a 8K buffer that contains the directory entry.
         */
	if (error = get_dir_buffer(cdp, &bp)) 
		return(error);

        /*
         * Locate the directory entry pointed by cd_diroff.
         */
	sector_size	= cdp->cd_fs->cdfs_sec_size;
	offset		= cdp->cd_diroff;
	bsize		= sector_size << FILE_CHUNK_SHIFT;
	buf_offset	= offset & (bsize-1);

	tmp_cddirp = (struct cddir *) (bp->b_un.b_addr+buf_offset);
	rec_len    = tmp_cddirp->cdd_reclen;

        /*
         * Sanity check.  If this condition occurs, either the disk is not
         * following the ISO-9660/HSG format or we have a bug.
         */
 	if (rec_len < sizeof(struct min_cddir) || rec_len > MAX_CDDIR_RECLEN) {
		printf("get_dir_rec: bad dir entry\n");
		error=ENOENT;
		goto out;
	}

        /*
         * copy the directory entry to the buffer callers provide.  If
         * truncate is set, we copy only bytes equals the size of struct
         * cddir.
         */
 	bcopy((char *)tmp_cddirp, (char *)dir_recp, 
 		truncate ? min(sizeof(struct cddir), rec_len) : rec_len);

        /*
         * Is the directory entry interested a directory itself?
         */
	if (cdp->cd_format == ISO9660FS) {
		is_dir = dir_recp->cdd_flag & CDROM_IS_DIR;
	} else {
               /*
                * HSG's file flag is in ISO's timezone
                */
		is_dir = dir_recp->cdd_timezone & CDROM_IS_DIR;
	}

        /*
         * If it is a directory, we get the disk address of "." entry in that
         * directory.  Else, it is a file, we get the disk address of the
         * directory entry we have now.
         */
	if (is_dir) {
                /*
                 * Some compilers are 4-byte aligned :-(
                 */
		bcopy(dir_recp->cdd_loc, &tmp_loc, 4);
                *cdnop = cdlbtooff(cdp->cd_fs, tmp_loc + dir_recp->cdd_xar_len);
	} else {
		*cdnop = cdlbtooff(cdp->cd_fs, cdp->cd_loc + cdp->cd_xarlen) + 
			 offset;
	}

        /*
         * Now we advance to the next directory entry.
         */
	offset += rec_len;

	if (offset < cdp->cd_size) {
		tmp_cddirp = (struct cddir *)((char *)tmp_cddirp + rec_len);

                /*
                 * Make sure tmp_cddirp points to a valid cddir first.
                 * The standard says "A directory entry should end in the
                 * same sector." and "Unused bytes are filled with zeros."
                 */
		if (((buf_offset+rec_len) != bsize) && tmp_cddirp->cdd_reclen) {
			cdp->cd_diroff = offset;
		} else {
                        /*
                         * Since the next entry, if exists, will start at
                         * the next sector, we move cd_diroff to it.
                         */
			cdp->cd_diroff = scroundup(cdp->cd_fs, offset);
		}
	}

        /*
         * Are we beyond end of file?  If so, reset cd_diroff back to
         * the beginning of the file.
         */
	if ((offset >= cdp->cd_size) || (cdp->cd_diroff >= cdp->cd_size)) {
		 cdp->cd_diroff = 0;
	}
out:
	if(bp) 
		brelse(bp);

	return(error);
}



#if defined(CDCASE) && defined(OSDEBUG)
int dkm_debug = 0;
#endif


/******************************************************************************
 * cdfs_lookup()
 *
 * Given a vnode of the directory and the name of the file/directory to be
 * looked up, cdfs_lookup return the vnode pointer of the target in vpp.
 */
/*ARGSUSED*/
cdfs_lookup(dvp, nm, vpp, cred, mvp)
struct vnode	*dvp;
register char	*nm;
struct vnode	**vpp;
struct ucred	*cred;
struct vnode	*mvp;
{
	register struct	cdnode	*dcdp;
	register struct	cdnode	*cdp=0;
	register char	*file_id;
	register int	saved_diroff;
	register int	direction;
	register int	namlen = strlen(nm);
#ifdef CDCASE
	register struct	cdfs	*cdfsp;
	register char   *NM;		
	int i, len;
	char case_buf[CDMAXNAMLEN+1];
#endif
	
	struct	vnode	*vp;
	struct	vnode	*dnlc_lookup();
	struct  cddir    dir_rec;

	int	error=0;
	cdno_t	cdno;
	int	shortcut=0;
	int	want_parent;
	
        /*
         * According to standard, the maximum length of a filename is 37.
         */
	if (namlen > CDMAXNAMLEN)
		return(ENAMETOOLONG);

	dcdp = VTOCD(dvp);
#ifdef CDCASE
	cdfsp = dcdp->cd_fs;
#endif

        /*
         * We can only lookup in a directory.  If not, return error now.
         */
	if ((dcdp->cd_ftype & CDFDIR) == 0) 
		return(ENOTDIR);

        /*
         * When performing pathname lookup of ".." from the "/" of a mounted
         * file system, mvp is the vnode represent the "/" of mounted fs.
         * dvp is the vnode of the mounted-on directory.  We need to check
         * the access right based on mvp not dvp.  If we are not doing ".."
         * lookup, mvp is the same as dvp.
         */
       if (error = VOP_ACCESS(mvp, CDEXEC, u.u_cred))
                return(error);

        /*
         * Before we do all the pushup, search in name cache first to see
         * it we can get it with minimum effort.  If we have in there,
         * up the reference count of the vnode and return the vnode.
         */
	vp = dnlc_lookup(dvp, nm, NOCRED);
	if (vp) {
		VN_HOLD(vp);
		*vpp = vp;
		return(0);
	}

	cdlock(dcdp);

        /*
         * If the user is asking for ".", "HP has it now". :-)
         */
	if ((*nm == '.') && (*(nm+1) == '\000')) {
		VN_HOLD(CDTOV(dcdp));
		cdp = dcdp;
	    	goto found; /* stop looking */
	}

        /*
         * If we are looking for "..", we set a flag to say so.  This
         * is to avoid this triple comparison in the loop.  Note
         * that the file_name of parent directory is "\001".
         */
	if ((*nm == '.') && (*(nm+1) == '.') && (*(nm+2) == '\000')) {
		want_parent = 1;
	} else {
		want_parent = 0;
	}

        /*
         * Since the directory entries in a directory is sorted by file
         * name, we can save some work by taking advantage of this
         * arrangement.  We start at the cd_diroff.  By comparing
         * the file id in the directory entry and the one user requested,
         * we know if the entry for the file, if exists, is before the
         * current entry or after.
         */
	if (*nm < '.') 
		shortcut = 1;

        /*
         * get the buffer of directory records to start looking at.
         * Saved_diroff also mark the end point of the search.
         */
	saved_diroff = dcdp->cd_diroff;
	
	do {
                /*
                 * First get the buffer the directory entry is in.
                 */
 		if (error = get_dir_rec(dcdp,&dir_rec,&cdno,1)) 
			goto out;

                /*
                 * We skip associate files and fall file extends except
                 * the last one.
                 */
		if (dcdp->cd_format == HSGFS) {
		       /*
		        * HSG's file flag is in ISO's timezone 
			*/
                       if ((dir_rec.cdd_timezone&(CD_ASSO|CD_LE)) != 0) 
				continue;
		} else {
                       if ((dir_rec.cdd_flag & (CD_ASSO|CD_LE)) != 0) 
				continue;
		}

#ifdef CDCASE
		/*
		 *   If this filesystem is supposed to be case-insensitive,
		 *   put an uppercase version of the name in case_buf; we'll
		 *   use it below 
		 */
		if (cdfsp->cdfs_flags & C_CONVERT_CASE) {
			file_id = nm;
			NM = case_buf;
			for (i = 0; i < namlen; i++, file_id++) 
				*NM++ = toupper(*file_id);
			*NM = '\0';	
			NM = case_buf;
		}			
		if (cdfsp->cdfs_flags & C_ZAP_VERSION) 
			if (dir_rec.cdd_file_id[namlen] == ';') 
				dir_rec.cdd_idlen = namlen;
		len = max(dir_rec.cdd_idlen, namlen);
#ifdef OSDEBUG
		if (dkm_debug)		
			printf("dirent: %s %c  nm: %s\n", 
			       dir_rec.cdd_file_id, 
			       dir_rec.cdd_file_id[namlen], nm);
#endif
#endif	/* CDCASE */		
		file_id = dir_rec.cdd_file_id;
		if (dir_rec.cdd_idlen != CDMAXNAMLEN) {
			file_id[dir_rec.cdd_idlen] = '\000';
		}

		if (want_parent && (*file_id == '\001') && 
		    (dir_rec.cdd_idlen == 1)) {
			/* file id = parent directory */
			cdunlock(dcdp);

                        /*
                         * we try to put the cdnode number of parent directory
                         * in all cdnode.  But if it is not available (Yes,
                         * they are cases this would happen.  e.g. when doing
                         * "cd ../..") we need to go through all the push up
                         * to get it.  It is costly.
                         */
			if (dcdp->cd_pnum == 0) {
				if (error = fill_pnum(dcdp)) 
					return(error);
			}

                        /*
                         * Now get the cdnode of the parent
                         */
			cdp = cdget(dcdp->cd_dev, dcdp->cd_fs, dcdp->cd_pnum,
				    0, &dir_rec);

			if (cdp == NULL) 
				return(u.u_error);

			break; /* stop looking */

                /*
                 * Compare the names.  Note that strncmp return >0, ==0 or
                 * <0 to indicate the value of comparison.  This piece of
                 * information allows as to take the shortcut.
                 */
#ifdef CDCASE
		} else if (((direction = strncmp(nm, file_id, len)) == 0) ||
			   ((cdfsp->cdfs_flags & C_CONVERT_CASE) && 
			    (direction = strncmp(NM, file_id, len)) == 0)) {
#else
		} else if ((direction = strncmp(nm,file_id,
					max(namlen,dir_rec.cdd_idlen))) == 0) {
#endif
                        /*
                         * Name matches, let's get the cdnode of the file.
                         */
			cdp = cdget(dcdp->cd_dev, dcdp->cd_fs, cdno, 
				    &(dcdp->cd_num),&dir_rec);
			cdunlock(dcdp);
			if (cdp == NULL) 
				return(u.u_error);

			break; /* stop looking because you found it */
                /*
                 * We only take the shortcut once.  If we have not yet done
                 * so, let's do it.
                 */
		} else if (shortcut == 0) {
		   shortcut=1;

                   /*
                    * If the name of the file requested is less than the
                    * name in the current entry, we restart from the begining
                    * of the directory.
                    */
		   if (direction < 0) {
			dcdp->cd_diroff = 0;
		   } else {
	                /*
	                * The file we are looking for is after the 
			* current entry, so we set the stop point at 
			* the begining of the directory.  If the file 
			* requested does not exist, we will stop 
			* searching when we finish with the last
                        * entry.  Note the cd_diroff will be reset 
			* to 0 when the last entry being searched.
                        */
			saved_diroff = 0;
		   }
		}
	} while ((error == 0) && (saved_diroff != dcdp->cd_diroff));

	if (cdp == NULL) { /* name not found */
		if (!error) 
			error = u.u_error = ENOENT;

	} else { /* name was found */
found:
		*vpp = CDTOV(cdp);              /* return the vnode found */
		dnlc_enter(dvp, nm, *vpp, NOCRED); /* add to name cache */
		cdunlock(cdp);
		return(0);
	}
out:
	cdunlock(dcdp);
	return(error);
}


/******************************************************************************
 * fill_pnum()
 *
 * Fill in the parent cdnode number in the cdnode provided.
 */
fill_pnum(cdp) 
register struct cdnode *cdp;
{
	struct cddir	*dotdot_cddirp;
	struct cddir	dotdot_cddir;

	cdno_t	cdno;
	int	error;

	dotdot_cddirp = &dotdot_cddir;
	cdlock(cdp);

        /*
         * First get the ".." directory entry.  Note the cdnode number for
         * the directory is the "." entry in the directory itself.
         */
	if (error = second_cddir_entry(cdp, dotdot_cddirp, &cdno)) {
		cdunlock(cdp);
		return(error);
	}
	cdunlock(cdp);

        /*
         * Get the disk address of the "." entry (the first entry).
         */
	cdp->cd_pnum = cdlbtooff(cdp->cd_fs, 
			*((int *)(dotdot_cddirp->cdd_loc)) + 
			 dotdot_cddirp->cdd_xar_len);
	return(0);
}

/******************************************************************************
 * second_cddir_entry()
 *
 * Get the directory record of the directory represented by cdp.
 */
second_cddir_entry(cdp, cddirp, cdnop)
register struct cdnode *cdp;
cdno_t	*cdnop;
struct cddir	*cddirp;
{
	int	error = 0;

	cdp->cd_diroff = 0;

        /*
         * get this first dir entry, the side effect of it is the cd_diroff
         * will be moved to next entry.  We cannot go directly to the second
         * entry for we do not know the size of the first.  (Don't forget
         * the first entry could have system use area.
         */
	if (error = get_dir_rec(cdp, cddirp, cdnop,1)) 
		return(error);

        /*
         * Now get the second entry (i.e. "..")
         */
	if (error = get_dir_rec(cdp, cddirp, cdnop,1)) 
		return(error);

        /*
         * Sanity check.  When you see this message on the console, there
         * is either a bug or a problem with the disk.
         */
	if (cddirp->cdd_file_id[0] != '\001') {
		printf("second_cddir_entry: bad directory entry \"..\"\n");
		error = ENOENT;
	}

	return(error);
}



/*****************************************************************************
 * cdfs_readdir()
 *
 * cdfs_readdir read in all or part of a directory and call cddir_to_dir to
 * translate cdfs directory to dir(3) lib routines' directory format.
 * This routine supports small-packet NFS links.
 *
 * Input: vp: vnode pointer to the directory.
 *        uiop: where to start, how much to read, where the output should be.
 * Output: uiop structure is updated.  The buffer pointed by uiop is filled
 *        with directory entries.
 */

#define	roundtoint(x)	(((x) + (sizeof(int) - 1)) & ~(sizeof(int) - 1))
#define	Dreclen(dp)	roundtoint(((dp)->d_namlen + 1 + sizeof(u_long) +\
				2 * sizeof(u_short)))

#define CDDIRBLKSIZ (cdp->cd_fs->cdfs_sec_size) /* ISO 9660 : par. 6.8.1.1 */

/*ARGSUSED*/
cdfs_readdir(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	struct cdnode	*cdp;
	off_t		offset;
	struct iovec	t_iovec;
	struct uio	t_uio;
	caddr_t		readbuf;
	int		readlen;
	int		bytes_read, bytes2return;
	caddr_t		first_new_dp;
	int		error = 0;
	caddr_t		kmem_alloc();
	struct direct	*dp, *lastdp;
#ifdef CDCASE
	int i;
	struct cdfs *cdfsp;
#endif	

	cdp = VTOCD(vp);
#ifdef CDCASE
	cdfsp = cdp->cd_fs;
#endif		
	if (uiop->uio_offset >= cdp->cd_size)
		return 0;	/* EOF */

	if (uiop->uio_iovcnt != 1)
		panic("cdfs_readdir: bad iovcnt");

	if ((int)uiop->uio_resid < (int)sizeof (struct direct))
		return EINVAL;

	/*
	 *	Refer to comments in getdirentries() to see why we
	 *	need to be able to accept any offset and size.
	 *	Read in all directory blocks which contain
	 *	the desired span. 
	 *	Skip all entries which start before uiop->uio_offset.
	 *	Since the user is free to set the file offset, the
	 *	first entry returned may lie anywhere past the initial
	 *	offset.
	 */
	offset = uiop->uio_offset & ~(CDDIRBLKSIZ-1);	/* round down */
	readlen = ((uiop->uio_offset & (CDDIRBLKSIZ-1))	/* round up */
			+ uiop->uio_resid + (CDDIRBLKSIZ-1)) & ~(CDDIRBLKSIZ-1);
	readlen = MIN(readlen, 8192);
	readbuf = kmem_alloc(readlen);
	t_uio.uio_seg = UIOSEG_KERNEL;
	t_uio.uio_fpflags = 0;
	t_uio.uio_iov = &t_iovec;
	t_uio.uio_iovcnt = 1;

nextblk:
	t_uio.uio_offset = offset;
	t_uio.uio_resid = t_iovec.iov_len = readlen;
	t_iovec.iov_base = readbuf;
	error = cdfs_rd(cdp->cd_devvp, vp, &t_uio);
	if (error)
		goto out;

	bytes_read = readlen - t_uio.uio_resid;
	error = cddir_to_dir(cdp, offset, readbuf, bytes_read);
	if (error)
		goto out;

	lastdp = dp = (struct direct *)readbuf;
	first_new_dp = (caddr_t) 0;

	/*
	 *	Skip entries before uio_offset and null entries.
	 */
	while ((caddr_t)dp < (readbuf + bytes_read)
	&& (offset < uiop->uio_offset || dp->d_fileno == 0)) {
		offset += dp->d_reclen;
		dp = (struct direct *)((caddr_t)dp + dp->d_reclen);
	}

	/*
	 *	Because we rounded up we have no partial entries.
	 */
	if ((caddr_t)dp < (readbuf + bytes_read))
		first_new_dp = (caddr_t) dp;
	else if (offset < cdp->cd_size)
                goto nextblk;
	/* else EOF */

	uiop->uio_offset = offset;

	/*
	 *	See how much we can return.
	 */
	while ((caddr_t)dp < (readbuf + bytes_read)
	&& ((caddr_t)dp - first_new_dp + Dreclen(dp)) <= uiop->uio_resid) {
		lastdp = dp;
#ifdef CDCASE
		/*
		 *  If we're supposed to be case insensitive and/or zap
		 *  version numbers, take care of it now
		 */
		for (i = 0; i < dp->d_namlen; i++)
			if ((cdfsp->cdfs_flags & C_CONVERT_CASE) &&
			    (dp->d_name[i] <= 'Z' && dp->d_name[i] >= 'A'))
				dp->d_name[i] += 32;		
			else if ((cdfsp->cdfs_flags & C_ZAP_VERSION) &&
				 (dp->d_name[i] == ';')) { 
				dp->d_name[i] = '\0';
				dp->d_namlen = i+1;
				break;
			}
#endif		
		dp = (struct direct *)((caddr_t)dp + dp->d_reclen);
	}

	if (first_new_dp){
		/*
		 *	See comment at start of rfs_readdir().
		 *	d_reclen may have been as big as 2048.
		 *	We'll lie to get something that will fit.
		 *	Since the right offset is returned, they'll leap over
		 *	the empty gap on their next call.
		 */
		lastdp->d_reclen = Dreclen(lastdp);
		bytes2return = (caddr_t) lastdp + lastdp->d_reclen
				- first_new_dp;
		error = uiomove(first_new_dp, bytes2return, UIO_READ, uiop);
	} /* else EOF */
out:
	kmem_free(readbuf, readlen);
	return error;
}


/*****************************************************************************
 * cdfs_inactive()
 *
 * Free the cdnode for other to use.
 */
/*ARGSUSED*/
cdfs_inactive(vp, cred)
struct vnode *vp;
struct ucred *cred;
{
        cdinactive(VTOI(vp));
        return (0);
}


/*****************************************************************************
 * cdfs_bmap()
 *
 * Prepare the block number to demand-load execution.
 */
cdfs_bmap(vp, lbn, vpp, bnp)
struct vnode	*vp;
daddr_t	lbn;
struct vnode	**vpp;
daddr_t	*bnp;
{
	/*
	 * pagin wants CLBYTES chunk and buffer management wants DEV_BSIZE 
	 * chunk
	 */
	if (bnp) 
		*bnp=(lbn << (PGSHIFT - DEV_BSHIFT));

	/*
	 *	Take a close look at the next line.  Normally, VOP_BMAP will
	 *	return the device vnode for a file/page/etc, but we
	 *	are being sneaky here.  The pagein code is presently the only
	 *	thing that cares what we pass back in vp, and it is interested
	 *	in reading in a whole page (of a demand-paged exec. file).
	 *	The problem is that the page may start in the middle of a 
	 *	2K block (thanks, HSG/ISO :-(), which would fail (the CD-ROM
	 *	drive is only willing to read starting on 2K boundaries).
	 *	To get around this, we give the pagein() code its own
	 *	vp back, which means that the subsequent swap() request
	 *	will eventually go through our code.  This will protect the
	 *	pagein()/swap() code from having to know about CDs.
	 */
	if(vpp) 
		*vpp=vp;
	return(0);
}

/* cd_bmap figure out the sector number the data in a file and size of valid
 * data should be read in and where is the beginning of the data read in.
 *
 * Input: vp: vnode pointer to the file that is interested.
 *        start: offset in the file reqested.
 * Output:
 *	 vpp: return device vnode pointer.
 *	 snp: return sector number.
 *	 sizep: size of valid data will be read in.
 *	 offsetp: valid data starts from the beginning of data read in.
 *	 rablock: sector number for read ahead.
 *	 rasize: size for next read ahead.
 *
 * return sector number in bnp
 * rablock is also set to the sector number for read ahead.
 */
cd_bmap(vp, start, vpp, snp, sizep, offsetp)
struct vnode	*vp;
register u_int	start;
struct vnode	**vpp;
register daddr_t	*snp;
register int	*sizep;
int	*offsetp;	/*pointer to offset of block we read in*/
{
	register struct	cdnode	*cdp = VTOCD(vp);
	register struct	cdfs	*cdfsp = cdp->cd_fs;

	register u_int	fusize; 	/* file unit size in bytes */
	register int	bsize;		/* has to be power of 2 */
	register int	bshift;
	register u_int	file_loc;

	int	pad = 0;		/* pad for round to sector */
	int	same_size;
	int	tmp_size;		/* scratch for size */
	int sz, sec_n, of;

        /*
         * Reset block number and size for read ahead.
         */
        rablock = 0;
        rasize = 0;

	fusize = cdlbtooff(cdfsp, cdp->cd_fusize);

	if (vpp) {	/*if requested, return dev vp here*/
                *vpp = vp;
	}

        /*
         * No, you cannot read data beyond the end of file.
         */
	if (start >= cdp->cd_size) { 
		 return(-1);
	}

        /*
         * if size of logic block is same as sector size, it is easier.
         */
	if (cdfsp->cdfs_lbsize != cdfsp->cdfs_sec_size)  
		same_size = 0;
	else	
		same_size = 1;

        /*
         * where does the content of file starts (in logic blocks)
         */
	file_loc = cdp->cd_loc+cdp->cd_xarlen;

        /*
         * As usual, set bsize to 8k, request is limited to 8k
         */
	bsize = cdfsp->cdfs_sec_size << FILE_CHUNK_SHIFT;
	bshift = cdfsp->cdfs_lsshift + FILE_CHUNK_SHIFT;

        /*
         * We handle interleave mode and non-interleave mode separately.
         * We expect most of the files read recorded in non-interleave mode.
         * Don't put burden on majority of cases with extra work.
         */
	if (fusize == 0) {  /*non-interleave is the most frequent case*/
	int	tmp_loc;

                /*
                 * We divide the file into 8k chunks.  If file is not
                 * started at the sector boundary, pad space is added to
                 * the beginning.  (CD-ROM drives allow access only in
                 * unit of sectors.)
                 */
		if (!same_size) {
                        /*
                         * If the beginning of file (not include XAR)
                         * is not at the sector boundary, add pad space
                         * to the beginning. pad is in bytes
                         */
			tmp_loc = file_loc;
			file_loc = lbtolsb(cdfsp, file_loc);
			pad = cdlbtooff(cdfsp, tmp_loc - 
				     lsbtolb(cdfsp, file_loc));
		}

                /*
                 * File_loc contains sector number at this point.
                 * Now we calculate the offset into the buffer.
                 */
		of = (start+pad) & (bsize - 1);
		if (offsetp) 
			*offsetp = of;

		sz = MIN(bsize, cdp->cd_size - (start - of));
		if (sizep) 
			*sizep = sz;

		sec_n = file_loc+(((start+pad)>>bshift)<<2);
		if (snp)  
			*snp = sec_n;

                /*
                 * If more data in this file, figure out the sector number and
                 * size of valid data for the next block of data.
                 */
		tmp_size = start - of + sz;
		if (cdp->cd_size > tmp_size) {
			rablock = lstodb(cdfsp, sec_n+4);
			rasize = scroundup(cdfsp, min(bsize, cdp->cd_size - 
						     (start - of + bsize)));
		}

	} else {	/*if interleave recode*/
                /*
                 * We starts read at either beginning of a file unit or
                 * 8k boundary inside a file unit.
                 */
		int	f_unit;	     /* which unit we are interested in */
		int	f_unit_size; /* unit size in logic block */
		int	unit_dist;   /* distance between two units in logic */
				     /* blk then convert to sector blks */
		int	fugsize;     /* unit gap size in logic block */
		int	boff;	     /* bsize block offset */
		
                /*
                 * Get file unit size, unit gap size and distance between
                 * two file units (sum of sizes of file unit and unit gap).
                 * All are in logic blocks.
                 */
		f_unit_size = cdp->cd_fusize;
		fugsize = cdp->cd_fugsize;
		unit_dist = fugsize + f_unit_size;

                /*
                 * If there is a XAR, skip gap between xar and beginning of
                 * file.
                 */
		if (cdp->cd_xarlen) file_loc += fugsize;

                /*
                 * Figure out which file unit we are looking for.
                 */
		f_unit = cdlbno(cdfsp, start) / cdp->cd_fusize;

		file_loc += (f_unit * unit_dist);   /*in logic blocks*/
		tmp_size = start;	/*remember the orginal offset*/

                /*
                 * To make the logic simple, we change the operation unit from
                 * logical blocks to sectors if different.
                 */
		if (!same_size) {
                        /*
                         * Notice each file unit has to start on sector boundary
                         */
			file_loc = lbtolsb(cdfsp, file_loc);
			unit_dist = lbtolsb(cdfsp, unit_dist);
		}

                /*
                 * From now on file_loc and unit_dist now contains sector
                 * number.  File_loc now has the sector number of the file
                 * units we are interested in.
                 *
                 * Now it is time to find out the offset from the begining of
                 * the file unit, the address of the 8k block in the file unit
                 * we want, and the offset into the 8K block that has the data.
                 */
		start = start % fusize;
		boff  = start & (~(bsize - 1));
		of    = start - boff;

		if (offsetp) 
			*offsetp = of;

                /*
                 * Now calculate the sector number of the 8K block we need.
                 */
		sec_n = file_loc + cdlsno(cdfsp, boff); /*sector number where*/
		if (snp) 
			*snp = sec_n; 		      /*read starts*/

                /*
                 * Figure out how much data we should read in.  It should be
                 * less than bsize(8k), less than remainder of the file unit,
                 * less then remainder of the file.
                 */
		sz = min(cdp->cd_size-(tmp_size-of), bsize);
		tmp_size = fusize - boff;
		sz = min(tmp_size, sz);
		if (sizep) 
			*sizep = sz;

                /*
                 * Since we have come this far, it is easy to get the address
                 * of next block.
                 */
		if ((tmp_size -= sz) > 0) { /*more data in this file unit*/
			rablock = lstodb(cdfsp, sec_n + FILE_CHUNK);
			rasize = min(tmp_size, bsize);

		} else {	/*go to next file unit*/
			rablock = lstodb(cdfsp, file_loc + unit_dist);
			rasize = min(bsize, fusize);
		}
	}
	return(0);
}

#ifdef POSIX
/******************************************************************************
 * cdfs_fpathconf()
 *
 * Given a vnode, perform pathconf(2).
 */
/*ARGSUSED*/
cdfs_fpathconf(vp, name, resultp, cred)
struct vnode *vp;
int name;
int *resultp;
struct ucred *cred;
{
	extern struct privgrp_map priv_global;
	enum vtype tp;

	tp = vp->v_type;

	switch(name) {

		case _PC_LINK_MAX:
			*resultp = 1;
			break;
	
		case _PC_MAX_CANON:
			return(EINVAL);
	
		case _PC_MAX_INPUT:
			return(EINVAL);
	
		case _PC_NAME_MAX:
			if (tp != VDIR) 
				return(EINVAL);
			*resultp = CDMAXNAMLEN;
			break;
	
		case _PC_PATH_MAX:
			if (tp != VDIR) 
				return(EINVAL);
			*resultp = MAXPATHLEN;
			break;
	
		case _PC_PIPE_BUF:
			return(EINVAL);
	
		case _PC_CHOWN_RESTRICTED:  /* root or mem. of chown privgrp */
			if (priv_global.priv_mask[(PRIV_CHOWN-1)/(NBBY*NBPW)] &
				1L << ((PRIV_CHOWN-1) % (NBBY*NBPW)))
				*resultp = -1; /* chown is not restricted */
			else
				*resultp = 1; /* it is restricted */
			break;

		case _PC_NO_TRUNC:
#ifdef POSIX_SET_NOTRUNC
			*resultp = (u.u_procp->p_flag2 & POSIX_NO_TRUNC?1:-1);
#else
			*resultp = 1;
#endif
			break;
	
		case _PC_VDISABLE:
			return(EINVAL);
	
		default:
			return(EINVAL);
	}
	return(0);
}


#endif	POSIX




/******************************************************************************
 * cddir_to_dir()
 *
 * cddir_to_dir translate a cdfs dir entry into dir entry format dir(3) lib.
 * routines expected.
 * Input:  cdp:    cdnode pointer of the directory being read.
 *         buf:    a buf contains part of a directory read in from the disk
 *         offset: offset (from the begining of the directory to the first
                   directory entry the buffer contains)
 *         size:   size of data in the buf
 * Output: all entries in the buffer are translated into regular dir entry
 *         format.  The file name of first ('\000') and second entry ('\001')
 *         of the directory are changed to "." and ".."
 * Exit code:  0: if cdp points to a directory cdnode and translation is
 *                completed.  Or, cdp points to a non directory cdnode, no
 *                translation is done.
 *             EINVAL: the filename of first and second entry is not "\000",
 *                     "\001" respectly. Or the directory size is less than
 *                     minimum size of a directory (a directory should have
 *                     a least 2 entries.
 */
cddir_to_dir(cdp, offset, buf, size)
struct cdnode *cdp;
int	offset;
char	*buf;
int	size;
{
	register struct cddir *cddirp;
	register struct direct *dirp;
	register char *cddnmp, *dnmp;
	register int	leftover;
	register int	loc;
	register int	namelen;
	register int	reclen;
	register int	i;

	int	is_dir;
	int	sec_size;
	int	unaligned;
	int	lastreclen;
	int	is_iso;
	int	tmp_loc;

        /*
         * Sanity check.  If this is not directory, we should not be here.
         * We silently return.
         */
	if (CDTOV(cdp)->v_type != VDIR) 
		return(0);

	if (cdp->cd_format == ISO9660FS)  
		is_iso = 1;
	else 
		is_iso = 0;
	
	cddirp = (struct cddir *) buf;

        /*
         * Directory always start at sector boundary and directory records
         * don't cross sectors.
         */
	sec_size = cdp->cd_fs->cdfs_sec_size;  /*note that directory entries are
					      aligned to sector boundary*/

	if (offset == 0) {
                /*
                 * A directory should have minimum of 2 entries.
                 */
		if (size < 2*sizeof(struct min_cddir)) {
			printf("cddir_to_dir: bad dir\n");
			return(EINVAL);
		}
	}

        /*
         * Since directories are all recoreded in non-interleave mode, loc
         * can be calculated easily.  Remember that we need the location
         * of the directory entry for cdnode numbers.
         */
	loc = cdlbtooff(cdp->cd_fs, cdp->cd_loc+cdp->cd_xarlen) + offset;

	while (size > 0) {
           /*
            * The directory entries may not occupy all space in a sector.
            * The space is filled with NULL's.  "leftover" keeps the size
            * of this space after fell out of the next while loop.
            */
	   leftover = MIN(size, sec_size);
	   size -= leftover;

           /*
            * We do convertion one sector at a time.
            */
	   while (leftover >= sizeof(struct min_cddir)) {
                /*
                 * Warning: The order of statments in this while loop
                 * should not be altered.  cddirp and dirp point to same
                 * location and copy data.  This is ugly buf efficient.
                 */
		if ((reclen = cddirp->cdd_reclen) == 0) 
			break;

		if (is_iso) {
			is_dir = cddirp->cdd_flag & CDROM_IS_DIR;
		} else {
                       /*
                        * HSG's file flag is in ISO's timezone
                        */
			is_dir = cddirp->cdd_timezone & CDROM_IS_DIR;
		}

		lastreclen = reclen;
		dirp = (struct direct *) cddirp;
		/*	HP-PA note: CDROM directory entries pad out
		**	to halfword boundaries, but HP-PA requires word
		**	alignment. Since the fixed part of the CD directory
		**	structure is 33 bytes vs. 8 for hp-ux, we can safely
		**	round "dirp" down to get alignment (as long as we
		**	save data before we write over it). If we shift the
		**	current entry but not the following, d_reclen needs
		**	to change as well.
		*/
		unaligned = ((int)dirp & 03) ? 1 : 0;
		dirp = (struct direct *) ((int)dirp & ~03);
		dirp->d_reclen = reclen;
		if (reclen & 03) { /* only one of this & next are unaligned */

                        /*
                         * Before doing much work, validate that the
                         * cdfs reclen is half word aligned.  This is
                         * a fix for a panic that would otherwise occur
                         * in cdfs_readdir.  If you hit this code, you
                         * probably have a bad disk.
                         */
                        if (reclen & 01) {
                              printf("cddir_to_dir: bad dir entry.\n");
                              return(EINVAL);
                        }

			if (unaligned)	/* next entry won't move */
				dirp->d_reclen += 2;
			else
				dirp->d_reclen -= 2; /* he moves, we don't */

		}
		/*	else they shift or don't together and everything's
		 *	just peachy.
		 */

		namelen = cddirp->cdd_idlen;

                /*
                 * If this entry is directory, we calculate the cdnode number
                 * differently.
                 */
		if (is_dir) {
			bcopy(cddirp->cdd_loc, &tmp_loc, 4);
                	dirp->d_fileno = cdlbtooff(cdp->cd_fs, 
				tmp_loc + cddirp->cdd_xar_len);
		} else {
			dirp->d_fileno = loc;
		}

		cddnmp = cddirp->cdd_file_id;
		dnmp = dirp->d_name;
		dirp->d_namlen = namelen;

                /*
                 * Handle special case for "." and "..".
                 */
		if (offset == 0) {
                        /*
                         * If this is the first entry of a directory, check
                         * if file name is '\000'.  If not, return error.
                         * Else, change the name to "."
                         */
			if (*cddnmp == '\000') {
				dirp->d_fileno = cdp->cd_num;
				*dnmp++ = '.';
			} else {
				printf("cddir_to_dir: bad first dir. entry\n");
				return(EINVAL);
			}

		} else if ((*cddnmp == '\001') && (namelen == 1)) {
			dirp->d_namlen = 2;

                        /*
                         * If we do not know the cnode number of the parent,
                         * we do pushup to get it.
                         */
			if (cdp->cd_pnum == 0) {
				if (fill_pnum(cdp)) 
					return (EINVAL);
			}

			dirp->d_fileno = cdp->cd_pnum;

			*dnmp++ = '.';
			*dnmp++ = '.';
		} else	{
			for(i=0; i < namelen; i++) 
				*dnmp++ = *cddnmp++;
		}

		*dnmp='\000';
		
                /*
                 * Move pointer to the next cdfs dir. entry
                 */
		cddirp = (struct cddir *)((char *)cddirp + reclen);
		loc += reclen;
		leftover -= reclen;
		offset += reclen;
	   }

           /*
            * Skip over those 0 bytes at the end of the sector
            */
	   cddirp = (struct cddir *)((char *)cddirp + leftover);
	   dirp->d_reclen += leftover;

           /*
            * If we had prepared to shift next record left 2 bytes (for
            * system require 4-byte alignment
            */
	   if ((unaligned && !(lastreclen&3)) || (!unaligned && (lastreclen&3)))
		dirp->d_reclen += 2;
	   offset += leftover;
	   loc += leftover;
	}
	return (0);
}


/*
 * cdfs_fid() -- given a vnode, return a pointer to a "file id" that can
 * be used to identify the file later on.
 */
cdfs_fid(vp, fidpp)
	struct vnode *vp;
	struct fid **fidpp;
{
	register struct cdfid *cdfid;

	cdfid = (struct cdfid *)kmem_alloc(sizeof(struct cdfid));
	bzero((caddr_t)cdfid, sizeof(struct cdfid));
	cdfid->cdfid_len = sizeof(struct cdfid) - 
		(sizeof(struct fid) - MAXFIDSZ);
	cdfid->cdfid_cdno = VTOCD(vp)->cd_num;
	cdfid->cdfid_gen = VTOCD(vp)->cd_fs->cdfs_gen;
	*fidpp = (struct fid *)cdfid;
	return (0);
}

/*
 * Buffer's data is in userland, or in some other
 *  currently inaccessable place.  We map in a page at a time
 *  and read it in.
 */
static void
cdfs_strat_map(bp)
    register struct buf *bp;
{
    register daddr_t blkno = bp->b_blkno;
    register caddr_t base_vaddr;
    register long count;
    register int pfn, nbytes, npage = 0, off;
    caddr_t vaddr;
    caddr_t old_vaddr;
    space_t old_space;
    extern caddr_t hdl_kmap();
    struct uio auio;
    struct uio *auiop = &auio;
    struct iovec aiov;
    struct iovec *aiovp = &aiov;
    struct vnode *vp;
 

    /* 
     * Record old addr from bp, set to KERNELSPACE 
     */
    old_vaddr = bp->b_un.b_addr;
    old_space = bp->b_spaddr;
    bp->b_spaddr = KERNELSPACE;
    base_vaddr = (caddr_t)((unsigned long)old_vaddr & ~(NBPG-1));

    /*
     * See how much to move, set count to one page max.  If we're
     *  reading partway into a page, go to the extra trouble of
     *  figuring out the correct initial count.
     */
    off = (int)old_vaddr & (NBPG-1);
    nbytes = bp->b_bcount;
    if (off) {
	count = NBPG-off;
	if (count > nbytes)
	    count = nbytes;
    } else {
	if (bp->b_bcount > NBPG)
	    count = NBPG;
	else
	    count = bp->b_bcount;
    }

    /* 
     * Move in data in page-size chunks 
     */
    while (nbytes > 0) {

	/* 
	 * Map the page into kernel memory, point bp to it 
	 */
	pfn = hdl_vpfn(old_space, base_vaddr+ptob(npage));
	vaddr = hdl_kmap(pfn);
	bp->b_un.b_addr = vaddr+off;

	/* 
	 * Set up block field 
	 */
	bp->b_blkno = blkno;
	bp->b_bcount = count;
	bp->b_flags &= ~B_DONE;

	/* 
	 * Set up the uio structure 
	 */
	auiop->uio_iov = aiovp;
	aiovp->iov_base = bp->b_un.b_addr;
	aiovp->iov_len = bp->b_bcount;
	auiop->uio_iovcnt = 1;
	auiop->uio_offset = bp->b_blkno << DEV_BSHIFT;
	auiop->uio_seg = UIOSEG_KERNEL;
	auiop->uio_resid = aiovp->iov_len;      /*byte count*/
	auiop->uio_fpflags = 0;
	vp = bp->b_vp;

	/* 
	 * Do the I/O 
	 */
	if (bp->b_error = cdfs_rd(VTOCD(vp)->cd_devvp, vp , auiop)) {
		bp->b_flags |= B_ERROR;
	}

	/* 
	 * Remap back to the user's space 
	 */
	hdl_remap(old_space, base_vaddr+ptob(npage), pfn);

	hdl_kunmap(vaddr);

	/* 
	 * Call it quits on error 
	 */
	if (geterror(bp) || auiop->uio_resid) 
		break;

	/* 
	 * Update counts and offsets
	 */
	nbytes -= count;
	blkno += btodb(count);
	++npage;
	off = 0;
    }

    bp->b_resid = auiop->uio_resid;
    bp->b_flags |= B_DONE;

    /* 
     * Restore bp fields and done 
     */
    bp->b_un.b_addr = old_vaddr;
    bp->b_spaddr = old_space;
}


/******************************************************************************
 * cdfs_strategy()
 *
 * Strategy routine to fill in the pages for demand-load execution.
 */
cdfs_strategy(bp)
struct buf	*bp;
{
	struct vnode	*vp;
	struct uio	auio;
	struct iovec	aiov;
	register struct	uio	*auiop = &auio;
	register struct	iovec	*aiovp = &aiov;

	/* 
	 * Buffer isn't in kernel virtual address space, so handle specially 
	 */
	if (bvtospace(bp,bp->b_un.b_addr) != KERNELSPACE) {
		cdfs_strat_map(bp);
		return;
	}

        /*
         * Prepare an uio structure for cdfs_rd()
         */
	auiop->uio_iov = aiovp;
	aiovp->iov_base = bp->b_un.b_addr;
	aiovp->iov_len = bp->b_bcount;
	auiop->uio_iovcnt = 1;
	auiop->uio_offset = bp->b_blkno << DEV_BSHIFT;
	auiop->uio_seg = UIOSEG_KERNEL;
	auiop->uio_resid = aiovp->iov_len;	/*byte count*/
	auiop->uio_fpflags = 0;

	vp = bp->b_vp;
	if (bp->b_error = cdfs_rd(VTOCD(vp)->cd_devvp, vp , auiop)) {
		bp->b_flags |= B_ERROR;
	} else {
		bp->b_resid = auiop->uio_resid;
	}
	bp->b_flags |= B_DONE;
}

/******************************************************************************
 * cdfs_fsctl()
 *
 * Function to get native file system information of ISO-9660/HSG.
 */
/*ARGSUSED*/
cdfs_fsctl(vp, command, uiop, cred)
struct	vnode	*vp;
register int	command;
struct uio	*uiop;
int	cred;
{
	register struct	cdfs	*cdfsp;
	register struct cdnode	*cdp;
	char	*infop;
	int	datalen;
	int	error = 0;

	cdp = VTOCD(vp);
	cdfsp = cdp->cd_fs;

	switch (command) {
#ifdef CDCASE
	case CDFS_CONV_CASE:		
		if (suser())
			cdfsp->cdfs_flags |= C_CONVERT_CASE;
		else
			return(EPERM);
		return(0);
	
	case CDFS_ZAP_VERS:
		if (suser())
			cdfsp->cdfs_flags |= C_ZAP_VERSION;
		else
			return(EPERM);
		return(0);
	
	case CDFS_ZERO_FLAGS:
		if (suser())
			cdfsp->cdfs_flags = 0;
		else
			return(EPERM);
		return(0);
#endif		
	case CDFS_DIR_REC:
	   {
		struct cdnode *dcdp;
 		char	kd[MAX_CDDIR_RECLEN];
 		struct cddir *kp;
		int	tmpcdno;

		if (cdp->cd_pnum == 0) {
			if (error = fill_pnum(cdp)) 
				return (error);
		}

                /*
                 * Get cdnode of the parent directory.
                 */
		dcdp = cdget(cdp->cd_dev, cdp->cd_fs, cdp->cd_pnum, 0, 0);
		if (dcdp == 0) 
			return(u.u_error);

		if (CDTOV(cdp)->v_type == VDIR) {
                   /*
                    * It is a great pain to find the directory record of the
                    * parent directory record.  I suppose I can use the "."
                    * entry in the directory itself.  But I feel uncomfortable
                    * because of the system use area might be different.  They
                    * should but the standard does not spell this out and the
                    * data preparer might miss it.  So...:-(
                    */
		   dcdp->cd_diroff = 0;
		   while ((error = get_dir_rec(dcdp, kd, &tmpcdno, 0)) == 0) {
			if (tmpcdno == cdp->cd_num) 
				break;

			if (dcdp->cd_diroff == 0) {
				printf("cdfs_fsctl: error in getting dir. rec\n");
				error=EINVAL;
				break;
			}
		   }
		} else {
                   /*
                    * dcdp->cd_num points to the beginning of dir. and
                    * cdp->cd_num points to the dir. entry of the target
                    */
		   dcdp->cd_diroff =  cdp->cd_num - dcdp->cd_num;
		   error = get_dir_rec(dcdp, kd, &tmpcdno, 0);
		}

		if (error == 0) {
 			kp = (struct cddir *)kd;
 			error = uiomove(kd, kp->cdd_reclen, UIO_READ, uiop);
		}

		cdunlock(dcdp);
		VN_RELE(CDTOV(dcdp));
		return(error);
	   }
	case CDFS_XAR:
	   {
		int	diff;
		int	xarlen;

		if ((xarlen= cdp->cd_xarlen) == 0) 
			return(ENOENT); 

		xarlen = cdlbtooff(cdfsp, xarlen); 
		if (xarlen < uiop->uio_resid) {
			diff = uiop->uio_resid-xarlen;
			uiop->uio_resid = xarlen;
			uiop->uio_iov->iov_len = xarlen;
		} else {
			diff = 0;
		}

		uiop->uio_offset = cdlbtooff(cdfsp, cdp->cd_loc);
		error = cdfs_rd(cdp->cd_devvp, cdp->cd_devvp, uiop);
		if (diff) {
			uiop->uio_resid += diff;
		}
		return(error);
	   }
	case CDFS_AFID:
		infop = cdfsp->cdfs_abstract;
		break;
	case CDFS_BFID:
		infop = cdfsp->cdfs_bibliographic;
		break;
	case CDFS_CFID:
		infop = cdfsp->cdfs_copyright;
		break;
	case CDFS_VOL_ID:
		infop = cdfsp->cdfs_vol_id;
		break;
	case CDFS_VOL_SET_ID:
		infop = cdfsp->cdfs_vol_set_id;
		break;
	default:
		return(EINVAL);
	}

	if ((datalen= cdfsnmlen(infop)) == 1) {
		return(ENOENT);
	}

	return(uiomove(infop, datalen, UIO_READ, uiop));
}


/******************************************************************************
 * cdfsnmlen()
 *
 * Get the length of filename.  Currently, we only recognized 7-bit characters.
 */
cdfsnmlen(nm)
register char *nm;
{
	register int i=0;
	while((*nm > ' ') && (*nm < 0x7f)) {
		i++;
		nm++;
	} 
	*nm = '\000';
	return(i+1);
}
