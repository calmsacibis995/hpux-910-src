/* @(#) $Revision: 63.2 $ */      

#include		<sys/param.h>	/* for MAXPATHLEN and DEV_BSIZE */

/* These used for converting directory entries */
#define MAXINT		0xfffffff
#define	CDIR_IN		(1)
#define	CDIR_OUT	(0)

#define CLSEOF(tbf)	(tbf == TRUE? 0xfff:0xffff)
#define CFD(fd)		(&info[fd-OFFSET])

#define	boolean		int
#define	uchar		unsigned char
#define	ushort		unsigned short
#define uint		unsigned int

extern	int	errno;

/* -------------------------------------------------------------------------- */
/*   Disc caching can be implemented later if performance needs warrent it.   */
/*   Right now, only one cache buffer is provided.  Buffers must always be    */
/*   released once obtained.                                                  */
/* -------------------------------------------------------------------------- */
struct	cachebuf {
	uchar	*buf;		/* block data */
	int	block;		/* block number */
	struct	header	*hd;	/* HPUX file descriptor for device */
};

/* -------------------------------------------------------------------------- */
/*   Each device has a "header" structure allocated for it.  Devices are      */
/*   identified by the name of the device special file used to open it.	      */
/*   Problems will arise if two devices are known by the same name!!!!!!      */
/* -------------------------------------------------------------------------- */
struct	header {
	char	dname[MAXPATHLEN];/* character name for device file 	*/
	int	fd;		/* file descriptor for device		*/
	int	openc;		/* count of number of opens		*/
	struct header	*next;	/* pointer to next device		*/

	struct	cachebuf cb;	/* pointer to list of cache buffers  	*/
	int	part_off;
	int	blocksize;	/* size of cache buffers		*/
				/* DOS information follows.   Read from */
				/* boot sector on the disc.		*/
	char	idstring[9];
	int	byt_p_cls;	/* bytes per cluster */
	int	byt_p_sector;	/* bytes per sector */
	int	sec_p_cls;	/* sectors per cluster */
	int	reserved_sec;
	int	fats;		/* number of fats */
	int	dir_entries;	/* number of directory entries for root */
	int	total_sec;	/* number of sectors on disc */
	int	media_type;
	int	sec_p_fat;	/* sectors for FAT */
	int	sec_p_track;	/* sectors per track */
	int	heads;		/* number of heads */
	int	hidden_sec;
	int	data_area_start;/* sector address of first data cluster */
	int	root_dir_start;	/* sector address of root directory */
	boolean	twelve_bit_fat;	/* 1 if twelve bit FAT entries, else zero */
	int	max_cls;	/* maximum addressable cluster */
	int	fat_modified;	/* TRUE if in-memory FAT has been changed */
	uchar	*fat;		/* pointer to in memory FAT */
	uint	fatsize;	/* size of FAT in bytes */
	int	acls;		/* used to find unallocated clusters */	
};

#define	DIRSIZE	32	/* size in bytes of a dir entry */
#define	PART_SIZE	66
#define	HEADER_SIZE	30
#define BUILDHW(x,y)	  ((uchar)x) | (((uchar)y)<<8)
#define SWAPHW(x)	x = ((x>>8)&0xff) | ((x&0xff)<<8);

/* -------------------------------------------------------------------------- */
/* The first byte of a directory entry (file name) indicates the file status  */
/* -------------------------------------------------------------------------- */
#define DIRSUBDIR	0x2e	/* Special entry for the directory.  if */
				/* second byte also contains 0x2e, then */
				/* cluster is for .. 	 	*/
#define DIRUNUSED	0x00	/* this entry and all further entries   */
				/* have never been used. */
#define DIRERASED	0xe5	/* file has been erased. */

/* -------------------------------------------------------------------------- */
/* Each directory has 8 bits of attribute information.   These values         */
/* can be or'd together. 						      */
/* -------------------------------------------------------------------------- */
#define ATTRDONLY	1	/* file is marked read only */
#define ATTHIDDEN	2	/* hidden file.  excluded from normal */
				/* directory searches */
#define ATTSYS		4	/* system file.  excluded from normal */
				/* directory searches */
#define ATTVOLLBL	8	/* contains a volume label in first 11 */
				/* bytes.  only valid in root directory */
#define ATTDIR		16	/* sub directory.  excluded from normal */
				/* directory searches. */
#define ATTARCHIVE	32
#define ATTNONE		0	/* no attributes associated with file */

#define	FALSE		(0)
#define	TRUE		(1)
#define DEBUGON		(debugon==TRUE)
#define	OFFSET		1000	/* differentiate DOS fd's from native fd's */
#define	INFOSIZE	17	/* number of DOS file descriptor */
#define dosfile(dp)	(dp >= OFFSET && dp<OFFSET+INFOSIZE)

/* -------------------------------------------------------------------------- */
/*   In memory copy of directory entries.
/* -------------------------------------------------------------------------- */
struct	dir_entry {			/* in memory copy of dir entry */
	uchar	name[9];		/* file name; padded with blanks */
	uchar	ext[4];			/* file extension */
	uchar	attr;			/* defined by ATT**** */
	char	reserved[10];
	ushort	year;			/* 0-119; based at 1980 */
	ushort	month;			/* 1-12 */
	ushort	day;			/* 1-31 */
	ushort	hour;			/* 0-23 */
	ushort	minute;			/* 0-59 */
	ushort	second;			/* two second increments */
	ushort	cls;			/* first cls in file.  0x0 for  */
					/* root directory */
	uint	size;			/* size of file in bytes */
};



/* -------------------------------------------------------------------------- */
/*   When a file is opened, an "info" structure is allocated for it.	      */
/* -------------------------------------------------------------------------- */
struct info {
	struct 	header 	 *hdr;	/* pointer to 'superblock' for this device */
	struct	dir_entry dir;	/* in memory copy of directory entry */
	int	fp;		/* file pointer.  offset into file */
				/* from which to read/write */
	int	dir_modified;
	int	openc; 		/* TRUE if currently active or being used*/
	int	daddr;  	/* disc address of directory entry */
	int	cls;		/* a cluster belonging to the file */
	int	clsoffset;	/* logical offset into file of 'cls' */
	int	clsvalue;	/* contents of cluster 'cls' */ 
} info [INFOSIZE];

struct header	*devices;	/* pointer to list of current devices */

#ifdef	DEBUG
boolean	debugon;		/* set to true if debugging program   */
#endif	DEBUG


/* procedure definitions */
struct	cachebuf *get_block ();
char * dostail ();
