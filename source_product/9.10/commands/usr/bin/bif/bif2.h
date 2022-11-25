/* @(#) $Revision: 66.1 $ */     
/*
 *
 *	Layout of a.out file :
 *
 *	header			unsigned short machine stamp	~ set by machine
 *				unsigned short magic number 
 *				short configuration	~set by user
 *				short DSTD size		) in # entries
 *				long reserved hp spare
 *				long text size		)
 *				long data size		) in bytes
 *				long bss size		)
 *				long text relocation size)
 *				long data relocation size)
 *				long MIS size		)
 *				long LEST size 		)
 *				long DST size		)
 *				long entry point
 *				long spare field 1
 *				long spare field 2
 *				long spare field 3
 *				long spare field 4
 *
 *
 *
 *	header:			0
 *	text:			sizeof(header)
 *	data:			sizeof(header)+textsize
 *	MIS:			sizeof(header)+textsize+datasize
 *	LEST:			sizeof(header)+textsize+datasize+ 
 *				MISsize 
 *	DSTD			sizeof(header)+textsize+datasize+
 *				MISsize+LESTsize
 *	DST			sizeof(header)+textsize+datasize+
 *				MISsize+LESTsize+dstdir*sizeof(dstd_entry)
 *	text relocation:	sizeof(header)+textsize+datasize+
 *				MISsize+LESTsize+dstdir*sizeof(dstd_entry)+
 *				dstsize
 *	data relocation:	sizeof(header)+textsize+datasize+
 *				MISsize+LESTsize+dstdir*sizeof(dstd_entry)+
 *				dstsize+rtextsize
 *				rtextsize
 *
 */

/* header of a.out files */

/* #include <magic.h> */

/*
   magic.h:  info about HP-UX "magic numbers"
*/

/* where to find the magic number in a file and what it looks like: */
#define MAGIC_OFFSET	0L

struct magic
	 {
	  unsigned short int system_id;
	  unsigned short int file_type;
	 };
typedef struct magic MAGIC;

/* predefined (required) file types: */
#define RELOC_MAGIC	0x106		/* relocatable only */
#define EXEC_MAGIC	0x107		/* normal executable */
#define SHARE_MAGIC	0x108		/* shared executable */

#define AR_MAGIC	0xFF65

/* optional (implementation-dependent) file types: */
/* None, at present */

/* predefined HP-UX target system machine IDs */
#define HP9000_ID	0x208
#define HP98x6_ID	0x20A

struct exec {	
		MAGIC	a_magic;
		short	a_stamp;
		short	a_dstdir;
		long	a_sparehp;
		long	a_text;
		long	a_data;
		long	a_bss;
		long	a_trsize;
		long	a_drsize;
		long	a_pasint;
		long	a_lesyms;
		long	a_dsyms;
		long	a_entry;
		long	a_spare1;
		long	a_spare2;
		long	a_spare3;
		long	a_spare4;
};

#ifndef KERNEL

/* macros which define various positions in file based on an exec: filhdr */
#define TEXTPOS		sizeof(filhdr)
#define DATAPOS 	TEXTPOS + filhdr.a_text
#define MODCALPOS	DATAPOS + filhdr.a_data
#define LESYMPOS	MODCALPOS + filhdr.a_pasint /* mit = SYMPOS */
#define DSTDPOS		LESYMPOS + filhdr.a_lesyms
#define DSTPOS		DSTDPOS + (filhdr.a_dstdir * sizeof(struct dstdentry))
#define RTEXTPOS	DSTPOS + filhdr.a_dsyms
#define RDATAPOS	RTEXTPOS + filhdr.a_trsize

/* symbol management */
struct nlist_ {			/* mit = sym */	/* sizeof(struct nlist)=10 */
	long	n_value;	/* mit = svalue */
	unsigned char	n_type;		/* mit = stype */
	unsigned char	n_length;	/* length of ascii symbol name */
	short	n_unit;		/* symbolic debugging unit number */
	short	n_sdindex;	/* index into DST */
};

struct dstdentry {
	long	d_tstart;	/* relative pos of text unit to TEXTPOS */
	long	d_dstart;	/* relative pos of data unit to DATAPOS */
	long	d_bstart;	/* relative start of bss unit */
	long	d_mstart;	/* relative start of MIS unit */
	long	d_lesstart;	/* relative start of LEST unit */
	long	d_dststart;	/* relative start of DST unit */
	long	d_dstspare;	/* */
	};

/* NOTE: entries in dstdentry are relative offsets from the actual beginning
   of the major exec segment. (e.g. the first dstdentry in dstdir will always
   have 0 in all fields) */
/* NOTE: at this time there is no d_cstart for relative start of common units
   because I don't see how we can segment common by unit. Common space is
   allocated between the data segment and the bss area by the loader based 
   on the symbol table that is entirely rebuilt during the link editing 
   process (no duplicate entries in common). */

/* relocation commands */
struct r_info {		/* mit= reloc{rpos,rsymbol,rsegment,rsize,rdisp} */
	long r_address;		/* position of relocation in segment */
	short r_symbolnum;	/* id of the symbol of external relocations */
	char r_segment;		/* RTEXT, RDATA, RBSS, or REXTERN */
	char r_length;		/* RBYTE, RWORD, or RLONG */
};

/* symbol types */
#define	EXTERN	040	/* = 0x20 */
#define ALIGN	020	/* = 0x10 */	/* special alignment symbol type */
#define	UNDEF	00
#define	ABS	01
#define	TEXT	02
/* #define	DATA	03 */
#define	BSS	04
#define	COMM	05	/* internal use only */
/* #define REG	06 */

/* relocation regions */
#define	RTEXT	00
#define	RDATA	01
#define	RBSS	02
#define	REXT	03

/* relocation sizes */
#define RBYTE	00
#define RWORD	01
#define RLONG	02
#define RALIGN	03	/* special reloc flag to support .align symbols */

		/* values for type flag */
#define	N_UNDF	0	/* undefined */
#define	N_ABS	01	/* absolute */
#define	N_TEXT	02	/* text symbol */
#define	N_DATA	03	/* data symbol */
#define	N_BSS	04	/* bss symbol */
#define	N_TYPE	037
#define	N_REG	024	/* register name */
#define	N_FN	037	/* file name symbol */
#define	N_EXT	040	/* external bit, or'ed in */



#define SYMLENGTH	255		/* maximum length of a symbol */

/* These suffixes must also be maintained in the cc shell file */

#define OUT_NAME "a.out"
#define OBJ_SUFFIX ".o"
#define C_SUFFIX ".c"
#define ASM_SUFFIX ".s"

#define N_BADTYPE(x) (((x).a_magic.file_type)!=EXEC_MAGIC&&\
	((x).a_magic.file_type)!=SHARE_MAGIC)

#define N_BADMACH(x) ((x).a_magic.system_id!=HP98x6_ID)
#define N_BADMAG(x)  (N_BADTYPE(x) || N_BADMACH(x))

#endif

struct  ustat {
	daddr_t	f_tfree;	/* total free */
/*	ino_t	f_tinode;	 total inodes free */
	ushort	f_tinode;	/* total inodes free */
	char	f_fname[6];	/* filsys name */
	char	f_fpack[6];	/* filsys pack name */
	int	f_blksize;	/* filsys block size */
};
/* #include <sys/types.h>  */

/*
 * fundamental variables
 * don't change too often
 */

#define	NOFILE	20		/* max open files per process */
#define	NSHMEM	5 		/* max shared memory segments per process */
#define	NSTACKSEGS 8		/* stack limit when shared memory mapped in */
#define	MAXPID	30000		/* max process id */
#define	MAXUID	60000		/* max user id */
#define	MAXLINK	1000		/* max links */

/* NOTE: if the following defines are changed be sure to check mchs.s & mchc.c*/

#define	MAXMEM		8192	/* maximum pages available for processes */

#ifdef M68000
#define	MAXUMEM		16384   /* maximum pages per process (16 meg) */
#else
#define	MAXUMEM		262144  /* maximum pages per process (256 meg) */
#endif

#define	SWAPSIZE	64	/* granularity of partial swaps (in pages) */
#define	SSIZE		2	/* initial stack size (*1024 bytes) */
#define	SINCR		1	/* increment of stack (*1024 bytes) */
#define	UPAGES		4	/* initial size of user block (*1024) */
#define	USRSTART	0x2000	/* starting location for user processes */

#define	CANBSIZ	256		/* max size of typewriter line	*/
#define	NCARGS	5120		/* # characters in exec arglist */

/*
**	The value of HZ bears some explanation.  The ers says that
**	time is measured in 1/60ths of a second (60 HZ).  This is
**	fine for machines whose clocks can run at that frequency, but
**	ours cannot.  Therefore we run our clock at 50 HZ and convert
**	the 50 HZ ticks to 60 HZ ticks whenever the user requests ticks
**	(times, acct).	This gives us compatibility with the ers.
*/

#ifdef	KERNEL
#define	HZ	50		/* Ticks/second of the clock (in kernel land) */
#define SIXTYHZ(x)	((x)+((x)/5))	/* convert 50HZ to 60HZ */
#else
#define	HZ	60		/* Ticks/second of the clock (in user land)   */
#endif


/*
 * priorities
 * probably should not be
 * altered too much
 */

#define	PSWP	0
#define	PINOD	10
#define	PRIBIO	20
#define	PZERO	25
#define	NZERO	20
#define	PPIPE	26
#define	PWAIT	30
#define	PSLEP	40
#define	PUSER	50
#define	PIDLE	127

/*
 * signals
 * don't change: if you must then check /usr/include/signal.h
 *
 *  CHANGE TO USE <signal.h> instead!!
 *
 */

/*
 * fundamental constants of the implementation--
 * cannot be changed easily
 */

#define	NBPW	sizeof(int)	/* number of bytes in an integer */
#define	NULL	0
#define	CMASK	0		/* default mask for file creation */
#define	NODEV	(dev_t)(-1)
#define	CLKTICK	16667		/* microseconds in a  clock tick */
#define MAXDMA	65536		/* longest possible DMA */

/* Machine dependent bits and macros */

#define	SMODE	0x2000	/* supervisor mode bit */
#define	INTPRI	0x0700	/* priority bits */

#define	USERMODE(ps)	(((ps) & SMODE) == 0)
#define	BASEPRI(ps)	(((ps) & INTPRI) != 0)


#define	lobyte(X)	(((unsigned char *)&X)[1])
#define	hibyte(X)	(((unsigned char *)&X)[0])
#define	loword(X)	(((ushort *)&X)[1])
#define	hiword(X)	(((ushort *)&X)[0])


#define	fuibyte(x)	fubyte(x)
#define	fuiword(x)	fuword(x)
#define	suibyte(x,y)	subyte(x,y)
#define	suiword(x,y)	suword(x,y)

#define IN	0
#define OUT	1
#define copyin(x,y,z)	copy(x,y,z,IN)
#define copyout(x,y,z)	copy(x,y,z,OUT)
#define copypage(x,y)	blt(y,x,PAGESIZE)

/*
 * The I node is the focus of all
 * file activity in unix. There is a unique
 * inode allocated for each active file,
 * each current directory, each mounted-on
 * file, text file, and the root. An inode is 'named'
 * by its dev/inumber pair. (iget/iget.c)
 * Data, from mode on, is read in
 * from permanent inode on volume.
 */

#define	NADDR	13
#define	NSADDR	(NADDR*sizeof(daddr_t)/sizeof(short))

struct	inode
{
	char	i_flag;
	cnt_t	i_count;	/* reference count */
	dev_t	i_dev;		/* device where inode resides */
	ino_t	i_number;	/* i number, 1-to-1 with device address */
	ushort	i_mode;
	short	i_nlink;	/* directory entries */
	ushort	i_uid;		/* owner */
	ushort	i_gid;		/* group of owner */
	off_t	i_size;		/* size of file */
	struct {
		union {
			daddr_t i_a[NADDR];	/* if normal file/directory */
			short	i_f[NSADDR];	/* if fifio's */
			dev_t	i_r;		/* if device special */
		} i_p;
		daddr_t	i_l;		/* last logical block read (for read-ahead) */
	}i_blks;
};


extern struct inode inode[];	/* The inode table itself */

/* flags */
#define	ILOCK	01		/* inode is locked */
#define	IUPD	02		/* file has been modified */
#define	IACC	04		/* inode access time to be updated */
#define	IMOUNT	010		/* inode is mounted on */
#define	IWANT	020		/* some process waiting on lock */
#define	ITEXT	040		/* inode is pure text prototype */
#define	ICHG	0100		/* inode has been changed */

/* modes */
#define	IFMT	0xf000		/* type of file */
#define		IFDIR	0x4000 	/* directory */
#define		IFCHR	0x2000 	/* character special */
#define		IFBLK	0x6000 	/* block special */
#define		IFREG	0x8000 	/* regular */
#define		IFMPC	0x3000 	/* multiplexed char special */
#define		IFMPB	0x7000 	/* multiplexed block special */
#define		IFIFO	0x1000 	/* fifo special */
#define		IFUNA	0x0000	/* unassigned inode */
#define	ISUID	0x800		/* set user id on execution */
#define	ISGID	0x400		/* set group id on execution */
#define ISVTX	0x200		/* save swapped text even after use */
#define	IREAD	0x100		/* read, write, execute permissions */
#define	IWRITE	0x80
#define	IEXEC	0x40

#define	i_addr	i_blks.i_p.i_a
#define	i_lastr	i_blks.i_l
#define	i_rdev	i_blks.i_p.i_r

#define	i_faddr	i_blks.i_p.i_a
#define	NFADDR	10
#define	PIPSIZ	NFADDR*BSIZE
#define	i_frptr	i_blks.i_p.i_f[NSADDR-5]
#define	i_fwptr	i_blks.i_p.i_f[NSADDR-4]
#define	i_frcnt	i_blks.i_p.i_f[NSADDR-3]
#define	i_fwcnt	i_blks.i_p.i_f[NSADDR-2]
#define	i_fflag	i_blks.i_p.i_f[NSADDR-1]
#define	IFIR	01
#define	IFIW	02
struct	fblk
{
	long	df_nfree;		/* was int on the vax! */
	daddr_t	df_free[NICFREE];
};
/* vohldr.h: volume header for HP-UX "LIF" volumes (so the media format
   field can be recongnized) */

struct HPUX_vol_type {
	short	HPUXid;			/* must be HPUXID */
	char	HPUXreserved1[2];
	int	HPUXowner;		/* must be OWNERID */
	int	HPUXexecution_addr;
	int	HPUXboot_start_sector;
	int	HPUXboot_byte_count;
	char	HPUXfilename[16];
};

/* load header for boot rom */
struct load {
	int address;
	int count;
};

#define HPUXID	0x3000
#define OWNERID	0xFFFFE942

#define ctob(x)  ((unsigned) (x) << 10 )
