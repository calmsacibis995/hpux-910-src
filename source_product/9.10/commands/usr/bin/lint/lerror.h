/* @(#) $Revision: 70.3 $ */       
/* defines for lint message buffering scheme 
 * be sure to include lerror.h before lmanifest
 */

/* number of chars in NAME, and filename */
#	define LFNM 256		/* Only used for header file msg buffering */

#define	NUMBUF	24

#define INIT_MSGS 	32	/* Initial # of CRECORDs in "ctmpfile" arrays */
#define ADDL_MSGS	64	/* Number of CRECORDS by which to grow */

/* struct msginfo keeps auxiliary information used for message buffering */
struct msginfo {
    short msgbuf;		/* 0 means the message is not buffered
				   n means the message is in section n of the
					buffer file */
    short msgargs;		/* codes that indicates how to print the
				   arguments to the message */
    int msgclass;		/* code that indicates the warning class for
				   the message */
};

struct msg2info {
    char *msg2text;		/* string */
    short msg2type;		/* type of message from below */
    int msg2class;		/* code that indicates the warning class for
				   the message */
};
extern int warnmask;

extern struct msginfo msginfo[];
extern struct msg2info msg2info[];

/* warning types */
#define WANSI		01
#define WUCOMPARE	02
#define WDECLARE	04
#define WHEURISTIC	010
#define WKNR		020
#define WLONGASSIGN	040
#define WNULLEFF	0100
#define WEORDER		0200
#define WPORTABLE	0400
#define WRETURN		01000
#define WUSAGE		02000
#define WCONSTANT	04000
#define WUDECLARE	010000
#define WOBSOLETE	020000
#define WPROTO		040000
#define WREACHED	0100000
#define WSTORAGE	0200000
#define WALWAYS		0400000
#define WALIGN		01000000
#define WHINTS		02000000
#define WALLMSGS	03777777

# define PLAINTY	0
# define STRINGTY	01
# define NUMTY		02
# define HEXTY		02
# define CHARTY		04
# define ARG1TY		(STRINGTY|NUMTY|CHARTY)

# define STR2TY		010
# define NUM2TY		020
# define HEX2TY		020
# define CHAR2TY	040
# define ARG2TY		(STR2TY|NUM2TY|CHAR2TY)

# define STR3TY		0100

# define DBLSTRTY	(STRINGTY|STR2TY)
# define TRIPLESTR	(STRINGTY|STR2TY|STR3TY)

# define SIMPL		01000
# define WERRTY		02000
# define UERRTY		0

# define NOTHING	0
# define ERRMSG		01
# define FATAL		02
# define HCLOSE		010

struct crecord {
    int	code;
    int	lineno;
    union {
	char	*name1;
	char	char1;
	int	number;
    } arg1;
    union {
    	char	*name2;
	char char2;
	int num2;
    } arg2;
    union {
	char *name3;
    } arg3;
};

# define CRECORD	struct crecord
# define CRECSZ		sizeof ( CRECORD )

# define OKFSEEK	0
# define PERMSG		((long) CRECSZ * MAXBUF )

# define HDRFILE	-1
struct hrecord {
    int		msgndx;
    int		code;
    int		lineno;
    union {
	char	char1;
	int	number;
    } arg1;
    union {
	char char2;
	int num2;
    } arg2;
};

# define HRECORD	struct hrecord
# define HRECSZ		sizeof( HRECORD )

enum boolean { false, true };

/* for pass2 in particular */

#ifdef APEX
# define NUM2MSGS	14	/* # categories of collected pass2 msgs */
#else
# define NUM2MSGS	13	/* # categories of collected pass2 msgs */
#endif
# define INIT2MSG	64	/* initial size of array for each category */
# define ADDL2MSG	128	/* increment size when expanding each array */

struct c2record {
    char	*name;
    int		number;
    int		file1;
    int		line1;
    int		file2;
    int		line2;
};

# define C2RECORD	struct c2record
# define C2RECSZ	sizeof( C2RECORD )

# define NMONLY		1
# define NMFNLN		2
# define NM2FNLN	3
# define ND2FNLN	4
#ifdef APEX
# define NMFNSTD	5
#endif
