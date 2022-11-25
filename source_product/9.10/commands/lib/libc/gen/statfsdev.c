
/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define close _close
#define fcntl _fcntl
#define fstat _fstat
#define fstatfsdev _fstatfsdev
#define lseek _lseek
#define open _open
#define read _read
#define statfsdev _statfsdev
#define strncmp _strncmp
#endif

#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <errno.h>

/* for cdfs file systems */
#include <sys/cdfsdir.h>
#include <sys/cdfs.h>

extern off_t lseek();

static int
fstatcdfs(fd,fsbuf)
int		fd;		/* file descriptor */
struct statfs	*fsbuf;
{
	union {
		struct icddfs	iso;	/* ISO volume descriptor */
		struct hcddfs	hsg;	/* HSF volume descriptor */
		char		dummy[CDSBSIZE];
	}	cdbuf;	

	/* read the primary volume descriptor (for now) */
	if (lseek(fd,(off_t)(CDSBLOCK*CDSBSIZE),0) == -1 ||
			read(fd,&cdbuf,CDSBSIZE) != CDSBSIZE)
		return(-1);

	if (strncmp(cdbuf.iso.cdrom_std_id,"CD001",STD_ID_SIZ) == 0) {
		fsbuf->f_blocks  = cdbuf.iso.cdrom_vol_size_msb;
		fsbuf->f_bsize   = cdbuf.iso.cdrom_logblk_siz_msb;
	} else if (strncmp(cdbuf.hsg.cdrom_std_id,"CDROM",STD_ID_SIZ) == 0) {
		fsbuf->f_blocks  = cdbuf.hsg.cdrom_vol_size_msb;
		fsbuf->f_bsize   = cdbuf.hsg.cdrom_logblk_siz_msb;
	} else
		return(-1);

	/* both ISO and HSG come here */
	fsbuf->f_bavail  = 0;
	fsbuf->f_bfree   = 0;
	fsbuf->f_ffree   = 0;
	fsbuf->f_files   = -1;	/* undefined for cdfs */
	fsbuf->f_type    = 0;
	fsbuf->f_fsid[1] = MOUNT_CDFS;
	return(0);
}

/* for hfs file systems */
#include <sys/param.h>
#include <sys/fs.h>

static int		/* private (unexported) routine */
fstathfs(fd,fsbuf)
int		fd;		/* file descriptor */
struct statfs	*fsbuf;
{
	union {
		struct fs	hfs;
		char		dummy[SBSIZE];
	}	hbuf;	

	/* read the super block */
	if (lseek(fd,(off_t)(SBLOCK*DEV_BSIZE),0) == -1 ||
			read(fd,&hbuf,SBSIZE) != SBSIZE)
		return(-1);

#if defined(FD_FSMAGIC)
	if ((hbuf.hfs.fs_magic == FS_MAGIC) ||
			(hbuf.hfs.fs_magic == FS_MAGIC_LFN) ||
			(hbuf.hfs.fs_magic == FD_FSMAGIC)) {
#else /* not new magic number */
	if (hbuf.hfs.fs_magic == FS_MAGIC ||
			hbuf.hfs.fs_magic == FS_MAGIC_LFN) {
#endif /* new magic number */
		fsbuf->f_blocks  = hbuf.hfs.fs_dsize;
		fsbuf->f_bfree   = hbuf.hfs.fs_cstotal.cs_nffree +
			hbuf.hfs.fs_cstotal.cs_nbfree * hbuf.hfs.fs_frag;
		fsbuf->f_bavail  = fsbuf->f_bfree -
			(hbuf.hfs.fs_dsize * hbuf.hfs.fs_minfree + 99) / 100;
		fsbuf->f_bsize   = hbuf.hfs.fs_fsize;
		fsbuf->f_files   = (hbuf.hfs.fs_ncyl + hbuf.hfs.fs_cpg - 1) /
					hbuf.hfs.fs_cpg * hbuf.hfs.fs_ipg;
		fsbuf->f_ffree   = hbuf.hfs.fs_cstotal.cs_nifree;
		fsbuf->f_type    = 0;
		fsbuf->f_fsid[1] = MOUNT_UFS;
#if defined(FD_FSMAGIC)
		fsbuf->f_magic   = hbuf.hfs.fs_magic;
		fsbuf->f_featurebits   = hbuf.hfs.fs_featurebits;
#endif /* new magic number */
		return(0);
	}

	return(-1);
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef statfsdev
#pragma _HP_SECONDARY_DEF _statfsdev statfsdev
#define statfsdev _statfsdev
#endif

int
statfsdev(fsspec,fsbuf)
char		*fsspec;	/* file system specification */
struct statfs	*fsbuf;
{
	int	fd,Ret;

	if ((fd = open(fsspec,O_RDONLY|O_NDELAY)) < 0)
		return(-1);
	Ret = fstatfsdev(fd,fsbuf);
	close(fd);
	return(Ret);
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef fstatfsdev
#pragma _HP_SECONDARY_DEF _fstatfsdev fstatfsdev
#define fstatfsdev _fstatfsdev
#endif

int
fstatfsdev(fd,fsbuf)
int		fd;	/* file descriptor */
struct statfs	*fsbuf;
{
	int		Ret;
	int		fcntl_stat;
	off_t		position;
	struct stat	sbuf;

	/* find out the device number, if any, for returning f_fsid[0]  */
	if (fstat(fd,&sbuf) < 0)
		return(-1);
	if ((sbuf.st_mode & S_IFMT) != S_IFBLK &&
			(sbuf.st_mode & S_IFMT) != S_IFCHR)
		fsbuf->f_fsid[0] = -1;
	else
		fsbuf->f_fsid[0] = sbuf.st_rdev;

	/* save the current fcntl mode bits */
	if ((fcntl_stat = fcntl(fd,F_GETFL,0)) == -1)
		return(-1);

	/* set O_NDELAY to avoid waiting forever on a tty */
	if (!(fcntl_stat & O_NDELAY))
		if (fcntl(fd,F_SETFL,fcntl_stat|O_NDELAY) == -1)
			return(-1);

	/* save the current position in the file */
	if ((position = lseek(fd,(off_t)0,1)) == -1) {
		/* this seek will fail if fd is for a pipe or fifo */
		Ret = -1;
		goto restore;
	}

	if (fstathfs(fd,fsbuf) == 0 ||
			fstatcdfs(fd,fsbuf) == 0) 
		Ret = 0;
	else {
		Ret = -1;
		errno = EINVAL;
	}

	/* restore the original position in the file */
	if (lseek(fd,position,0) == -1)
		Ret = -1;

restore:	/* restore the original fcntl mode bits */
	if (!(fcntl_stat & O_NDELAY))
		if (fcntl(fd,F_SETFL,fcntl_stat) == -1)
			return(-1);

	return(Ret);
}
