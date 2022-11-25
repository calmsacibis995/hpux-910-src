/*
 * @(#)global.h: $Revision: 70.2 $ $Date: 93/11/23 11:01:48 $
 * $Locker:  $
 */

/* @(#) $Revision: 70.2 $ */     

/*
 * This defines a BCD date in the LIF format.
 * This may fail on some machines due to implementation-dependent
 * ordering of bit fields.  We expect the first byte to contain year1<<4+year2.
 */
struct date_fmt {
	unsigned year1	: 4;
	unsigned year2	: 4;
	unsigned mon1	: 4;
	unsigned mon2	: 4;
	unsigned day1	: 4;
	unsigned day2	: 4;
	unsigned hour1	: 4;
	unsigned hour2	: 4;
	unsigned min1	: 4;
	unsigned min2	: 4;
	unsigned sec1	: 4;
	unsigned sec2	: 4;
};

struct dentry {
	char fname[MAXFILENAME];	/* Name of file */
	short ftype;			/* Type of file or EOD (end-of-dir) */
	int start;			/* Starting sector */
	int size;			/* Size in sectors */
	char date[6];			/* Should be date_fmt, but on
					   some machines (series 500)
					   date_fmt gets padded to 8 bytes */
	short lastvolnumber;		/* Both lastvolume flag & volume # */
	int extension;			/* Bizarre user-defined field */
};

struct lvol {
	unsigned short discid;		/* 0..65535 */
	char volname[MAXVOLNAME];	/* volume name */
	int dstart;			/* directory start */
	short dummy1;			/* 010000 for system 3000 */
	short dummy2;
	int dsize;			/* length of directory */
	short version;			/* extension level */
	short dummy3;
	int tps;			/* tracks / surface */
	int spm;			/* surface / medium */
	int spt;			/* sector / track */
	char date[6];			/* see dentry.date comment */
	unsigned short reserved1[99];
	int iplstart; 			/* IPL code location on media */
	int ipllength;			/* size of IPL code */
	int iplentry;			/* IPL entry blocksize for spectrum */
	unsigned short reserved[2];     /* 21..127 */
};

struct lfib { 
	int filedis;			/* fd on our LIF volume */
	int dindex;
	int dstart;			/* Where the directory starts */
	int lastsector;
	int dsize;			/* Size of directory */
	char dirpath[MAXDIRPATH];
	char filename[MAXFILENAME+1];
	struct dentry lfile;
	char buffer[HALF_K];
};

struct dpointer {		/* cat entry */
	int sector;   		/* sector */
	int index;    		/* buffer subscript */
	int start;    		/* data start */
};
