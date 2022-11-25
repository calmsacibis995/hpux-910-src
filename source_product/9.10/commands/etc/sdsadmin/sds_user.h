/* @(#) $Revision: 70.2 $ */
/*
 * sds_user.h
 */

#define bcopy(x, y, z)	memcpy(y, x, z)

/*
 * MAJOR KLUDGE !!!!
 *    These definitiions are also defined in lifdef.h and lifglobal.h,
 *    but the header files are not delivered to /usr/include and are
 *    not in the BE under /etc/conf.
 */
#   define LIFID	  0x8000
#   define LIFSIZE	    8192
#   define LIFSECTORSIZE     256
#   define MAXFILENAME	      10
#   define MAXVOLNAME	       6
#   define DESIZE	      32
#   define EOD		      -1
#   define PURGED	       0
#   define ASCII	       1
#   define BIN		  -23951

struct dentry {
        char fname[MAXFILENAME];        /* Name of file */
        short ftype;                    /* Type of file or EOD (end-of-dir) */
        int start;                      /* Starting sector */
        int size;                       /* Size in sectors */
        char date[6];                   /* Should be date_fmt, but on
                                           some machines (series 500)
                                           date_fmt gets padded to 8 bytes */
        short lastvolnumber;            /* Both lastvolume flag & volume # */
        int extension;                  /* Bizarre user-defined field */
};

struct lvol {
        unsigned short discid;          /* 0..65535 */
        char volname[MAXVOLNAME];       /* volume name */
        int dstart;                     /* directory start */
        short dummy1;                   /* 010000 for system 3000 */
        short dummy2;
        int dsize;                      /* length of directory */
        short version;                  /* extension level */
        short dummy3;
        int tps;                        /* tracks / surface */
        int spm;                        /* surface / medium */
        int spt;                        /* sector / track */
        char date[6];                   /* see dentry.date comment */
        unsigned short reserved1[99];
        int iplstart;                   /* IPL code location on media */
        int ipllength;                  /* size of IPL code */
        int iplentry;                   /* IPL entry blocksize for spectrum */
        unsigned short reserved[2];     /* 21..127 */
};

/* ---- END KLUDGE stuff from lifdef.h, lifglobal.h ---- */

/*
 * A structure to keep track of files that we are using.
 */
struct file_stuff
{
    char *name;
    int fd;	    /* fd for block device */
    int rfd;	    /* fd for raw device */
    dev_t orig_dev; /* original dev, unmodified */
    dev_t dev;	    /* non-partitioned version of device */
    int bus;	    /* which bus this is on (0..n) */
    u_long size;
    u_long blksz;
    char dname[64]; /* device name, for lookup from /etc/disktab */
    char prod_id[SDS_MAXNAME+1]; /* from inquiry */
};

extern struct file_stuff files[];
extern int n_files;
extern char *prog;

extern char *sds_skip_comment();
extern char *sds_word();
extern int sds_cvt_num();
extern int sds_keyword();
extern int sds_parse_raw();
extern int sds_get_info();

/*
 * files[TMP_FILE] is used to hold temporary files.
 */
#define TMP_FILE SDS_MAXDEVS
