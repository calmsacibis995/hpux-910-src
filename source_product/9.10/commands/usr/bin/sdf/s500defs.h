/* @(#) $Revision: 63.1 $ */     
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                s500defs.h                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#include <sys/types.h>

/*
 * The following are only a portion of the actual contents of "param.h"
 * as it appears on the series 500...
 */

#define	MAXLINK	1000		/* max links */

#define	NBPW	sizeof(int)	/* number of bytes in an integer */
#define BSIZE	1024		/* Block size of the average disc */
#define	NULL	0
#define	CMASK	0		/* default mask for file creation */
#define	CDLIMIT	(1L<<11)	/* default max write address */
#define	NODEV	(dev_t)(-1)
#define	ROOTINO	((ino_t)1)	/* i number of all roots */
#define	SUPERB	((daddr_t)0)	/* block number of the super block */
#define	DIRSIZ	14		/* max characters per directory */
#define	CLKTICK	16667		/* microseconds in a  clock tick */

struct	t_i {
	int	ti_date;	/* days since 24 Nov -4713 */
	int	ti_time;	/* seconds since midnight */
};

typedef	struct	t_i	time_ios;


struct	direct
{
	char	d_name[DIRSIZ+2];	/* 16 character file name */
	short	d_object_type;		/* shouldn't be referenced by C */
	short	d_file_code;		/* shouldn't be referenced by C */
	ino_t	d_ino;			/* use fir # for i-node */
};


struct filsys {
	ushort	s_format;		/* disc format, should = 0x700 for Unix */
	ushort	s_corrupt;		/* non-zero if directory may be corrupt */
	char	s_fname[16];		/* name of root directory, blank padded */
	time_ios	s_init;		/* date initialized / unique id */
	int	s_blksz;		/* the number of bytes per block */
	daddr_t	s_boot;			/* starting block of the boot area */
	int	s_bootsz;		/* size of the boot area in blocks */
	daddr_t	s_fa;			/* starting block of the FA file */
	int	s_version;		/* version number, = 0 for Unix */
	daddr_t	s_maxblk;		/* largest block that can be addressed */
	char	s_passwd[16];		/* volume password, not used by Unix */
	time_ios	s_bkup;		/* date of last backup, not used by Unix */
};					/* rest of block presently unused */


struct dinode {
	ushort	di_type;		/* = 1 for inodes */
	ushort	di_ftype;		/* file type */
	ushort	di_count;		/* reference count */
	short	di_uftype;		/* user file type (same as LIF's) */
	time_ios	di_ctime;	/* time created */
	unsigned	di_other;	/* public capabilities */
	ino_t	di_protect;		/* file's protection record (none == -1) */
	ino_t	di_label;		/* file's label record (none == -1) */
	int	di_blksz;		/* size of file in blocks */
	int	di_max;			/* largest byte that may be written to */
	ushort	di_exsz;		/* recommended extent size for file */
	ushort	di_exnum;		/* number of extents in inode (1..4) */
	struct	{
		daddr_t	di_startblk;	/* starting block of extent */
		int	di_numblk;	/* number of blocks in extent */
	}	di_extent[4];
	ino_t	di_exmap;		/* inode of 1st extent map (none == -1) */
	int	di_size;		/* current size in bytes */

		/* Warning!  The next two fields only apply to directory files */

	ino_t	di_parent;		/* inode of parent */
	char	di_name[16];		/* name of this directory */

		/* The rest of fields are defined only for our local */
		/* implementation of the structured directory format.  */

	time_t	di_atime;		/* time file was last accessed */
	time_ios	di_mtime;	/* time file was last modified */
	int	di_recsz;		/* logical record size */
	ushort	di_uid;			/* owner's user id */
	ushort	di_gid;			/* owner's group id */
	ushort	di_mode;		/* mode and type of file */
	char	di_res2[2];		/* unused */

		/* The next field is only used if the file is a */
		/* device file.  Otherwise it is set to zero. */

	dev_t	di_dev;			/* description of the device */
};

/*
 *	The inode (FIR) is only one of several types of FA records.
 *	Definitions for the rest follow:
 */

/*
 *	Extent map record:
 */

struct em_rec {
	ushort	e_type;			/* = 2 for extent maps */
	ushort	e_exnum;		/* # of extents in this record */
	int	e_res1;			/* unused */
	ino_t	e_next;			/* next map in list (none == neg) */
	ino_t	e_last;			/* last map in list (none == neg) */
	ino_t	e_inode;		/* inode number of owner */
	daddr_t	e_boffset;		/* blk offset of 1st extent from start of file */
	struct	{
		daddr_t	e_startblk;	/* starting block of extent */
		int	e_numblk;	/* number of blocks in extent */
	}	e_extent[13];
};

/*
 *	Protection record:
 */

struct p_rec {
	ushort	p_type;			/* = 3 for protection record */
	ushort	p_pwnum;		/* number of passwords in this record */
	int	p_res1[4];		/* unused */
	ino_t	p_next;			/* next rec in list (none == -1) */
	ino_t	p_inode;		/* inode number of owner */
	struct	{
		char	p_passwd[16];	/* a password */
		int	p_caps;		/* capability set */
	}	p_pwdes[5];
};

/*
 *	Label record:
 */

struct lab_rec {
	ushort	l_type;			/* = 4 for label records */
	ushort	l_res1;			/* unused */
	ino_t	l_next;			/* next rec in list */
	ino_t	l_inode;		/* inode number of owner */
	char	l_data[116];		/* user supplied, not used by Unix */
};

/*
 *	Unused record:
 */

struct uu_rec {
	ushort	u_type;			/* = 5 for unused */
	char	u_res1[126];		/* unused */
};

/*
 *	Free FA records record:
 */

struct free_rec {
	ushort	f_type;			/* = 6 for free records */
	ushort	f_fnum;			/* number of free records here (0..30) */
	ino_t	f_next;			/* next rec in list (none == neg) */
	ino_t	f_uufar[30];		/* pointers to unused FA records */
};

/*
 *	Combine these all together...
 */

union fa_entry {
	struct	dinode	e1;
	struct	em_rec	e2;
	struct	p_rec	e3;
	struct	lab_rec	e4;
	struct	uu_rec	e5;
	struct	free_rec	e6;
};


/*
 *	Defines for the record types
 */

#define R_INODE	1	/* inode/FIR record */
#define R_EM		2	/* extent map record */
#define R_PROT		3	/* protection record */
#define R_LBL		4	/* label record */
#define R_UNUSED	5	/* unused record */
#define R_FREEL	6	/* free list record */

/*
 * Support for device file types and field values.
 * Requires param.h first.
 */

#define major(x) 	(long) ((unsigned) x >> 24)

#define minor(x)	(long) (x & 0xffffff)

#define makedev(x,y)	(dev_t) ((x << 24) | (y & 0xffffff))

#define	MINOR_FORMAT	"0x%06.6x"

/* our own stuff */

#define	FA_SIZ		sizeof(union fa_entry)
#define	FILSYSSIZ	sizeof(struct filsys)
#define	D_SIZ		sizeof(struct direct)

#define	FA_INUM		0		/* FA file inode  */
#define	ROOT_INUM	1		/* root dir inode */
#define	RES_INUM	2		/* reserved inode */
#define	FMAP_INUM	3		/* free map inode */

#define	READ		0
#define	WRITE		1

#define	MAX_INT		0x7fffffff	/* largest integer (for addresses) */
