#ifndef __LIFDIR_INCLUDED__
#define __LIFDIR_INCLUDED__

/*
 * Requires the following to be defined:
 *	struct dentry
 *	struct lvol
 */

#define TRUE	1
#define FALSE	0

/* Special lif files */
#define ISL		"ISL"
#define HPUX		"HPUX"
#define AUTO		"AUTO"
#define LABEL		"LABEL"

/* Aligning and sizing IOs. */
#define ALIGN(v, a)	((v) & (~((a)-1)))
#define ROUNDUP(v, a)	ALIGN((v)+(a)-1, (a))
#define LIFSECTORSIZE	256
#define LIFALIGNMENT	8		/* sectors */
#define SECTORSPERBUF	LIFALIGNMENT
#define LIFBUFSIZE	(SECTORSPERBUF * LIFSECTORSIZE)
#define MAKEBUF(typ)	ROUNDUP(sizeof(typ), LIFBUFSIZE)

/* struct dentry types */
#define LIFTYPE_PURGED	((short int)0)
#define LIFTYPE_EOF	((short int)-1)
#define LIFTYPE_AUTO	((short int)-12289)
#define LIFTYPE_BIN	((short int)-23951)

/* Special values for various fields */
#define LIF_LVN		((short int)0100001)	/* dentry->lvn */
#define LIFID		32768			/* lvol->discid */

/*
 * List of buffers containing a lif directory.
 */
struct dlist {
	struct dlist	*next;		/* directory buffer queue */
	int		numentries;	/* # entries in buffer */
	int		lastent;	/* last "valid" entry. "valid" means: */
					/* read from disk or provided via */
					/* new_lifentry() */
	int		scanent;	/* last entry scanned */
	int		dirend;		/* last entry as given by hdr->dsize */
	int		append;		/* first directory entry appended */
	struct dentry	dentry[1];	/* first directory entry in array */
};

/*
 * Lif directory descriptor.
 */
struct lif_directory {
	char		name[MAXPATHLEN];	/* device/file containing lif */
	int		fd;	/* descriptor for device/file */
	int		flag;	/* Access mode and other status */
	struct dlist	*head;	/* Queue of buffers containing directory */
	struct dlist	*tail;
	struct dlist	*scan;	/* read_lifentry() returns from here */
	struct lvol	*hdr;	/* lif volume header */
	char		buf[MAKEBUF(struct lvol)];
};

#define READONLY	0x01
#define APPEND		0x02
#define FLUSHED		0x04
#define NEVERCHANGED	0x08
#define FIXEDSIZE	0x10

#endif  /* __LIFDIR_INCLUDED__ */
