/*
 * @(#)lifio.h: $Revision: 66.1 $ $Date: 90/10/11 16:49:46 $
 * $Locker:  $
 */

#define MAXLIFOPEN	5
#define MAXFILENAME 	10
#define MAXVOLNAME   	6
#define MAXDIRPATH   	1
#define HALF_K	    (1024 / 2)

struct lif_dirent {
	char fname[MAXFILENAME+1];
	short ftype;
	int start;			
	int size;
};

struct _lifdir {
	int fd;
	int start;
	int size;
	int loc;
	struct lif_dirent lifent;
	int inuse;
};

typedef struct _lifdir LIFDIR;
typedef struct _lifdir LIFFILE;
