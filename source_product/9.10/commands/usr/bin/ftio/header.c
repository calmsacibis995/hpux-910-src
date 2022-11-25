/* HPUX_ID: @(#) $Revision: 70.1 $  */

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : header.c
 *	Purpose ............... : routines to create tape headers.
 *	Author ................ : David Williams. 
 *
 *	Description:
 *		These routines can create and read cpio (-b and -c) type
 *		headers (more maybe later).
 *
 *	Contents:
 *
 *
 *-----------------------------------------------------------------------------
 */

#include "ftio.h"
/*
#include <sys/types.h>
#include <sys/stat.h>
#include "f_struct.h"
*/


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : writeheader()
 *	Purpose ............... : Creates a cpio header at the address given. 
 *
 *	Description:
 *
 *	Writeheader() creates a cpio format header at the address specified,
 *	using the stat(2) information, and filename length supplied. It does
 *	not put the file name in.
 *
 *	Returns:
 *	
 *	The size of the header created.
 */

writeheader(buf, len_fname, stats)
char	*buf;
int 	len_fname;
struct	stat	*stats;
{
	void makedevino();
	struct cpio_f_hdr *hdr;
	long	fsz;		/* size of file */
	long	ftype;
	short	lmagic;
	unsigned short v_dev;
	unsigned short v_ino;



	ftype = stats->st_mode & S_IFMT;
		
	if (!Htype)		/* Htype = 0 means binary */
	{
#ifdef 	DEBUG
		/*
		 * make sure header will start on even address
		 */
		if ( (int)buf & 1 )
			printf("ODD BOUNDARY SUPPLIED!!!!!!");
#endif	DEBUG
		hdr = (struct cpio_f_hdr *)buf;

		/*
		 * copy over all the header gear
		 */

		hdr->h_magic = MAGIC; 			/* cpiomagicno*/
		hdr->h_namesize = len_fname; 		/* sizeofname */
		hdr->h_uid = stats->st_uid;		/* user id */
		hdr->h_gid = stats->st_gid;		/* group id */
#ifdef hpux

/* Altered inode representation - removing makeshortdev function
   and using makedevino for (dev,ino) pair scheme */

		makedevino(stats->st_nlink,stats->st_dev,
			stats->st_ino,&hdr->h_dev,&hdr->h_ino);

#ifdef DEBUG
fprintf(stderr, "DEBUG header: writeheader: fix inode point\n");
fprintf(stderr, "DEBUG header: writeheader: stats->st_nlink = %d\n", stats->st_nlink);
fprintf(stderr, "DEBUG header: writeheader: stats->st_ino = %d\n", stats->st_ino);
fprintf(stderr, "DEBUG header: writeheader: octal stats->st_ino = %.6ho\n", stats->st_ino);
fprintf(stderr, "DEBUG header: writeheader: hdr->h_ino = %hd\n", hdr->h_ino);
fprintf(stderr, "DEBUG header: writeheader: octal hdr->h_ino = %.6ho\n", hdr->h_ino);
#endif
		switch (ftype)
		{
			case S_IFDIR:           /* directory */
			case S_IFREG: 		/* regular */
#ifdef SYMLINKS
			case S_IFLNK:           /* symbolic link */
#endif SYMLINKS
				hdr->h_rdev = 0;	
				mkshort(hdr->h_filesize, stats->st_size);
				break;

			case S_IFNWK: 		/* network */
				hdr->h_rdev = LOCALMAGIC;	
				mkshort(hdr->h_filesize, stats->st_size);
				break;

			case S_IFCHR:		/* character special */
			case S_IFBLK:		/* block special */
			case S_IFIFO:		/* pipe */
#ifdef CNODE_DEV
				if((stats->st_rcnode >= 0) &&
				   (stats->st_rcnode <= 255))
				    hdr->h_rdev = stats->st_rcnode;
				else
				    hdr->h_rdev = LOCALMAGIC;
#else
				hdr->h_rdev = LOCALMAGIC;       
#endif /* CNODE_DEV */
				mkshort(hdr->h_filesize, stats->st_rdev);
				break;
			
			default: 		/* unsupported */
				ftio_mesg(FM_UNSPX, "??");
				return 0;
		}
#else
		hdr->h_dev = stats->st_dev;
		hdr->h_ino = stats->st_ino;
		hdr->h_rdev = stats->st_rdev;
#endif
		hdr->h_mode = stats->st_mode;		/* access mode*/
		hdr->h_nlink = stats->st_nlink;		/* no links */
		mkshort(hdr->h_mtime, stats->st_mtime);	/* mod time */

		return(HDRSIZE);
	}
	else	/* character hdr */
	{	

		/* 
		 * make an ascii header
		 */
		
#ifdef hpux
		switch (ftype)
		{
			case S_IFDIR:		/* dirctory */
			case S_IFREG: 		/* regular */
#ifdef SYMLINKS
			case S_IFLNK:           /* symbolic link */
#endif SYMLINKS
				lmagic = 0;	
				mkshort(&fsz, stats->st_size);
				break;

			case S_IFNWK: 		/* network */
				lmagic = LOCALMAGIC;	
				mkshort(&fsz, stats->st_size);
				break;

			case S_IFCHR:		/* character special */
			case S_IFBLK:		/* block special */
			case S_IFIFO:		/* pipe */
#ifdef CNODE_DEV
				lmagic = stats->st_rcnode;
#else
				lmagic = LOCALMAGIC;	
#endif /* CNODE_DEV */
				mkshort(&fsz, stats->st_rdev);
				break;
			
			default: 		/* unsupported */
				ftio_mesg(FM_UNSPX, "??");
				return 0;
		}


		makedevino(stats->st_nlink,stats->st_dev,
			stats->st_ino,&v_dev,&v_ino);



#endif
		sprintf(buf, 
 	      	"%.6ho%.6ho%.6ho%.6ho%.6ho%.6ho%.6ho%.6ho%.11lo%.6ho%.11lo",
#ifdef hpux
		MAGIC, v_dev, v_ino,
		stats->st_mode, stats->st_uid,stats->st_gid,stats->st_nlink,lmagic,
#else
		MAGIC,stats->st_dev,stats->st_ino,stats->st_mode,stats->st_uid,
		stats->st_gid,stats->st_nlink,stats->st_rdev & 00000177777,
#endif
		stats->st_mtime, (short)len_fname, fsz);
		
		return(CHARS);
	}
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : readheader()
 *	Purpose ............... : Read a cpio header in memory. 
 *
 *	Description:
 *
 *		Readheader() attempts to read a cpio header (ascii or
 *		binary) at a location in memory. If it successfully finds 
 *		one it returns the size of the filename, and creates
 *		a stat for the file in stats. If it could match a header
 *		readheader() returns -1.
 *
 *	Returns:
 *
 *		-1	if a file header was not found.
 *		size of the filename if a header was found.
 *
 */

static	struct	cpio_f_hdr hdr;

readheader(p, stats)
struct	cpio_f_hdr *p;
struct	stat	*stats;
{
	int	longfile,
		longtime;

	long	mklong();

	/*
	 *	If header type is binary.
	 */
	if (!Htype)	
	{
		/*
		 *	See if we can match to a binary header.
		 */
		if (p->h_magic != MAGIC)
			return -1;
		
		stats->st_dev = p->h_dev;
		stats->st_ino = p->h_ino;
		stats->st_mode = p->h_mode;
		stats->st_nlink = p->h_nlink;
		stats->st_uid = p->h_uid;
		stats->st_gid = p->h_gid;
		stats->st_rdev = p->h_rdev;
		stats->st_size = mklong(p->h_filesize);
		stats->st_mtime = mklong(p->h_mtime);


#ifdef SYMLINKS
		Symbolic_link = (stats->st_mode & S_IFMT) == S_IFLNK ? 1 : 0;
#endif SYMLINKS

		return p->h_namesize;
	}
	else
	{
		/*
		 *	See if we can match a character header.
		 *
		 *	As a quick check, first test the first byte
		 *	IS it as expected??
		 *
		 *	If that works out ok, check to see we can
		 *	extract all the elements.
		 */
		if (strncmp((char *)p, "070707", 6))	
			return(-1);
		
		if (  sscanf((char *)p,
			     "%6ho%6ho%6ho%6ho%6ho%6ho%6ho%6ho%11lo%6ho%11lo",
			     &hdr.h_magic,&hdr.h_dev,&hdr.h_ino,&hdr.h_mode,
			     &hdr.h_uid,&hdr.h_gid,&hdr.h_nlink,&hdr.h_rdev,
			     &longtime,&hdr.h_namesize,&longfile
		      ) != 11 
		    || 
		      hdr.h_magic != MAGIC	
		)	
			return -1;

		/*
		 *	Ok we have a valid header, put it were requested.
		 */
		stats->st_dev = hdr.h_dev;
		stats->st_ino = hdr.h_ino;
		stats->st_mode = hdr.h_mode;
		stats->st_nlink = hdr.h_nlink;
		stats->st_uid = hdr.h_uid;
		stats->st_gid = hdr.h_gid;
		stats->st_rdev = hdr.h_rdev;
		stats->st_size = longfile;
		stats->st_mtime = longtime;


#ifdef SYMLINKS
		Symbolic_link = (stats->st_mode & S_IFMT) == S_IFLNK ? 1 : 0;
#endif SYMLINKS

		return hdr.h_namesize;
	}
}



/* Adding two new functions for fixing problem with 
links>1 & inode number>65535 - June 1992 */


/*
 * The following two routines, "nextdevino()" and "makedevino()", map
 * the 32 bit each (dev,ino) pairs to 16 bit each (dev,ino) pairs.
 * without imposing any limits, other than there can be no more than
 * (2^32 - (65536 * 4) distinct files in an archive (which is a very
 * large number.
 *
 * A hash table is used to retain the mappings that were used for those
 * files whose link count is > 1.  We never need to know what files
 * with a link count <= 1 were mapped to, so these are not put in the
 * table.
 */

/*
 * nextdevino() -- calculate and return the next available 16 bit
 *                 each (dev,ino) pair.  The value of 0 is not used
 *                 for dev.  The values 0,1, and 65535 are not used
 *                 for ino.
 */
void
nextdevino(dev, ino)
unsigned short *dev;
unsigned short *ino;
{
    static unsigned short lastdev = 1;  /* 0 is special, don't use */
    static unsigned short lastino = 1;  /* 0,1 are special, don't use */

    if (++lastino == UNREP_NO)
    {
	lastino = 2;
	lastdev++;
    }
    /* printf("Before lastdev-dev \n");
    printf("Before ****lastdev-dev \n"); */
    *dev = lastdev;

    /* printf("After lastdev-dev \n"); */
    *ino = lastino;
}

/*
 * makedevino() -- map a 32 bit each (dev,ino) pair to a 16 bit each
 *                 (dev,ino) pair, keeping track of mappings for files
 *                 who have a link count > 1.
 */
void
makedevino(links, l_dev, l_ino, s_dev, s_ino)
int links;
unsigned long l_dev;
unsigned long l_ino;
unsigned short *s_dev;
unsigned short *s_ino;
{
    struct sym_struct
    {
        unsigned long l_dev;
	unsigned long l_ino;
	unsigned short s_dev;
	unsigned short s_ino;
	struct sym_struct *next;
	int count;
    };
    typedef struct sym_struct SYMBOL;

    static SYMBOL **tbl = NULL;
    SYMBOL *sym;
    int key;

    /*
     * Simple case -- link count is 1, just map dev and ino to the
     *                next available number
     */
    if (links <= 1)
    {
	nextdevino(s_dev, s_ino);
	return;
    }

    /*
     * We have a file with link count > 1, allocate our hash table if
     * we haven't already
     */
    if (tbl == NULL)
    {
	int i;

	if ((tbl=(SYMBOL **)malloc(sizeof(SYMBOL *)*HASHSIZE)) == NULL)
	{
	    perror("cpio");
            exit(3);
	}
	for (i = 0; i < HASHSIZE; i++)
	    tbl[i] = NULL;
    }

    /*
     * Search the hash table for this (dev,ino) pair
     */
    key = (l_dev ^ l_ino) % HASHSIZE;
    for (sym = tbl[key]; sym != NULL; sym = sym->next)
	if (sym->l_ino == l_ino && sym->l_dev == l_dev)
	{
	    *s_dev = sym->s_dev;
	    *s_ino = sym->s_ino;
	    return;
	}

    /*
     * Didn't find this one in the table, add it
     */
    nextdevino(s_dev, s_ino);
    if ((sym = (SYMBOL *)malloc(sizeof(SYMBOL))) == NULL)
    {
	fprintf(stderr, "No memory for links\n");
	*s_ino = UNREP_NO;
    }
    else
    {
	sym->l_dev = l_dev;
	sym->l_ino = l_ino;
	sym->s_dev = *s_dev;
	sym->s_ino = *s_ino;
	sym->next = tbl[key];
	tbl[key] = sym;
    }
}

