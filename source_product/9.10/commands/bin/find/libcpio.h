/* @(#) $Revision: 66.1 $ */

/*
 * libcpio.h -- header file describing cpio archive library routines
 */

struct devinomap
{
    unsigned long l_dev;
    unsigned long l_ino;
    unsigned short s_dev;
    unsigned short s_ino;
    struct devinomap *next;
};

struct cpio_buffer
{
    long buflen;
    unsigned char *buffer;
};

struct cpio_desc
{
    struct cpio_buffer *bufp;
    unsigned long write_parm;
    long size;			/* size of archive so far */

/* private: */
    int (*open)();
    int (*read)();
    int (*write)();
    int (*close)();
#ifdef SYMLINKS
    int (*readlink)();
#endif

    unsigned long blksize;
    unsigned long flags;

    /*
     * For (dev,inode) mapping
     */
    unsigned short lastdev;
    unsigned short lastino;
    struct devinomap **map;
};

typedef struct cpio_desc CPIO;

/*
 * Flag values
 */
#define CPIO_ASCII		0x0001
#define EFFICIENT_LINKS		0x0002

/*
 * Return values for cpio_write()
 */
#define CPIO_OK		 0
#define CPIO_NOMEM	 1
#define CPIO_BADTYPE	 2
#define CPIO_BADBUFFER   3
#define CPIO_FILESHRANK  4	/* warning only */
#define CPIO_OPEN	10
#define CPIO_READ	11
#define CPIO_WRITE	12
#define CPIO_CLOSE	13
#define CPIO_READLINK	14

/*
 * cpio_initialize(CPIO *desc,
 *		   unsigned long flags,
 *		   unsigned long blksize);
 */
extern void cpio_initialize();

/*
 * cpio_openfn(CPIO *desc, int (*openfn)());
 * cpio_readfn(CPIO *desc, int (*readfn)());
 * cpio_writefn(CPIO *desc, int (*writefn)());
 * cpio_closefn(CPIO *desc, int (*closefn)());
 * cpio_readlinkfn(CPIO *desc, int (*readlinkfn)());
 */
extern void cpio_openfn();
extern void cpio_readfn();
extern void cpio_writefn();
extern void cpio_closefn();
#ifdef SYMLINKS
extern void cpio_readlinkfn();
#endif

/*
 * cpio_write(CPIO *desc,
 *	      char *path,
 *	      struct stat *statb);
 */
extern int cpio_write();

/*
 * cpio_finish(CPIO *desc);
 */
extern int cpio_finish();
