#ifndef lint
    static const char *HPUX_ID = "@(#) $Revision: 72.11 $";
#endif

/*
 * Cpio recognizes it is writing to a streaming 9-track tape drive
 * (i.e. 7874 or 7978). These two drives have an immediate report
 * mode which is the cause of the problem. This mode loses data
 * when a write error occurs.
 *
 * The checkpointing feature is enabled with "C" option. This feature
 * allows the user to only rewrite the current tape on a write error.
 *
 * The following matrix describes how relevant  environment  parameters
 * interact when a "write" error occurs:
 *
 *
 *               (Note the "C" option invokes checkpointing.)
 *
 *               -------------------------------------------
 *        Option |     without          |     with         |
 *               |    "C" option        |  "C" option      |
 *     Device    |                      |                  |
 *  --------------------------------------------------------
 *  |Streaming   |                      |  Prompt user for |
 *  |9-track     |    Abort archive.    |  a new medium and|
 *  |device      |                      |  re-write the    |
 *  |            |                      |  current volume. |
 *  --------------------------------------------------------
 *  |Other       |                      | 2 user options:  |
 *  |device      |    Prompt user for   |   1) re-write    |
 *  |special     |    a new medium to   |      current vol.|
 *  |file        |    write next volume.|   2) write next  |
 *  |            |                      |      volume.     |
 *  --------------------------------------------------------
 *
 */

#ifdef TRUX
/*
 *  Cpio and Security 
 *
 *  Cpio has been modified to properly verify the privilege of the
 *  and sensitivity level of the user and the io device.  On export,
 *  the user is notified of files that cannot be exported.  On import,
 *  all files retrieved form the device are created with the sensitivity
 *  level of the device.  Imported files are created with empty privilege
 *  sets and empty ACLS.
 *
 *  "SecureWare" and "B1" and "TRUX" defines are "on" for the HP B1 and C2
 *  products.  The ISB1, ISC2 and ISSECURE macros (in security.h) are used
 *  at runtime to respectively determine whether a system is an HP C2 B1 or
 *  either.
 *
 *
 *  Mltape Command 
 *
 *  "MLTAPE" is "on" to build the mltape command. 
 *  Mltape is a command based on cpio, it allows import/export of  multilevel
 *  directories and retains the sensitivity level information associated
 *  with the files copied.  Mltape produces/reads  a tape with a different
 *  format (cpio like, but with the sensitivity level information).  Since
 *  mltape is a B1 command not supplied with the base or C2 products
 *  the MLTAPE code does not contain any runtime configurability statements.
 *
 */
#endif

#if defined(RFA) || defined(OLD_RFA)
/*
 * This file contains "#if" preprocessor statements to control the
 * HP Remote File Access capabilities of cpio:
 *
 *  RFA		-- full RFA support
 *		   network special files are archived and restored
 *		   properly
 *  OLD_RFA	-- transition to obsolete RFA support
 *		   network special files are not archived, a warning
 *		   is generated that they are obsolete.
 *		   network special files are not restored, a warning
 *		   message is generated that they are obsolete.
 */
#if defined(OLD_RFA) && !defined(S_IFNWK)
#   define S_IFNWK 0110000 /* network special file */
#endif
#endif /* RFA || OLD_RFA */

#ifdef __hp9000s300
/*
 * Major number of 7974 or 7978 device on s[23]00.
 *
 * Bit 2 of minor, when cleared, indicates that immediate report is on.
 */
#define STREAM			9
#define IMMED_REPORT_BIT	0x00000004L
#endif /* __hp9000s300 */


#ifdef __hp9000s800
/*
 * Major number for 9-track device on s800.
 * Bit 21 of minor, when cleared, indicates that immediate report is on
 */
#define STREAM			5
#define IMMED_REPORT_BIT        0x00200000L
#endif /* __hp9000s800 */


/*
 * cpio	COMPILE:	cc -O cpio.c -s -o cpio
 *	cpio -- copy file collections
 *
 *	Note: Enable symbolic link code with SYMLINKS symbol.
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/sysmacros.h>
#include <sys/mtio.h>
#include <sys/param.h>
#include <pwd.h>
#include <time.h>

#ifdef RT
#   include <rt/macro.h>
#   include <rt/types.h>
#   include <rt/stat.h>
#else
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <sys/errno.h>
#endif

#if defined(NLS) || defined(NLS16)
#   include <locale.h>
#endif

#ifdef NLS
#   define NL_SETN 1	/* set number */
#   include <msgbuf.h>
#else
#   define catgets(catd,NL_SETN,i, s) (s)
#endif


#ifdef NLS16
#   include <nl_ctype.h>
#else
#   define CHARAT(p)	(*p&0377)
#   define CHARADV(p)	((*p++)&0377)
#   define ADVANCE(p)	(p++)
#endif

#if defined(SecureWare) && defined(B1)
#   include <sys/security.h>
#   include <prot.h>
#   include <fcntl.h>
#   ifdef MLTAPE
        extern unsigned char *ie_findbuff();
        extern unsigned char *ie_allocbuff();
        extern char *ie_stripmld();
	static char *RealFile = NULL;
        static char *TapeFile = NULL;
#   endif
    short	Lflag;
    short	Rflag;
    short	Aflag = 0, Pflag = 0;
    char	*swfile = NULL;
#   if defined(MLTAPE) && defined(DEBUG)
	privvec_t	current_privs;
	extern char	*privstostr();
#   endif
#endif

#define EQ(x,y)	(strcmp(x,y)==0)

/* for VAX, Interdata, ... */
#define MKSHORT(v,lv) { 			\
    U.l = 1L;					\
    if (U.c[0]) {				\
	U.l=lv;					\
	v[0]=U.s[1];				\
	v[1]=U.s[0];				\
    }						\
    else {					\
	U.l=lv;					\
	v[0]=U.s[0];				\
	v[1]=U.s[1];				\
    }						\
}

/*
 * equivalent C-routine code for MKSHORT(v,lv)
 * (comments start with "--"):
 *
 *	-- Convert long integer lv to two short integers v[0] and v[1],
 *	   and perform word swapping if necessary.
 *
 *	MKSHORT(v,lv) {
 *		short	v[];	-- conversion result
 *		long	lv;	-- long word to be converted
 *		U.l = 1L; 	-- see if "1" is stored in low or high
 *				   order byte
 *
 *		if (U.c[0]) {
 *				-- "1" stored in high order byte, and
 *				-- it needs swapping of words
 *			U.l = lv;
 *			v[0] = U.s[1];
 *			v[1] = U.s[0];
 *		} else {
 *			U.l = lv;
 *			v[0] = U.s[0];
 *			v[1] = U.s[1];
 *		}
 *	}
 */

#if defined(SecureWare) && defined (B1) && defined (MLTAPE)
#define MAGIC   071727          /* mltape magic number */
#else
#define MAGIC	070707		/* cpio magic number */
#endif
#define IN	1		/* copy in */
#define OUT	2		/* copy out */
#define PASS	3		/* direct copy */

#define HDRSIZE	(Hdr.h_name - (char *)&Hdr)	/* header size minus filename field */

#ifdef __hp9000s300
/* #define LOCALMAGIC 0xbbbb	/*   6/6/83 Unique for Series 200 */
#define LOCALMAGIC 0xdddd 	/* 4/17/85 to reflect Manx file system changes*/
#endif

#ifdef __hp9000s800
#define LOCALMAGIC 0xcccc	/*   4/1/85 Unique for Series 800  */
#endif /* __hp9000s800 */

/*
 * 0xff?? -- foreign (to HP) designations
 */
#ifdef vax
#ifndef S_IFIFO
#define S_IFIFO	0010000
#endif
#define LOCALMAGIC 0xfff0	/*   1/19/84 Unique for Vax */
#endif

#define HASHSIZE 501    /* Size of hash tables used for links */
/*
 * In older versions of cpio ---
 *     OUT_OF_SPACE was used to indicate that too many devices were
 *     archived.
 *     UNREP_NO  was used to indicate that a 32 bit inode couldn't be
 *               mapped to a 16 bit inode.
 *
 * If the dev field of a file == OUT_OF_SPACE or the inode filed is
 * UNREP_INO, cpio will print a message and not link that file.  This
 * version of cpio never puts these values into an archive unless it
 * runs out of memory.
 */
#define OUT_OF_SPACE 0
#define UNREP_NO 0xffff

#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
#   define CHARS 76+18 	/* mltape format has larger header field */
#else
#   define CHARS 76	/* ASCII header size minus filename field */
#endif

#define BUFSIZE 512	/* In u370, can't use BUFSIZ nor BSIZE */
#define CPIOBSZ 4096	/* file read/write */
#define MAXCOMPONENTS 512 /* max no of components in path name */


/* The following definitions are used for the checkpointing feature. */

/* write states (NORMAL, CHECKPT, FIRSTPART): */
#define NORMAL  0       /* Normal state when writing to 7974/7978 */

#define CHECKPT 1       /* "just-entered-checkpointing" state.
			 * This state goes to FIRSTPART when the
			 * first file is ready to be processed.
			 */

#define FIRSTPART 2     /* The first file in a rewrite is ready
			 * to be processed. This state gets reset
			 * to NORMAL after the partial write of the
			 * header or body has been completed.
			 */

#define HEADER      01  /* Writing the header portion of the file. */
#define BODY        02  /* Writing the body portion of the file. */

#define CHKPTERR    10  /* code for errors due to checkpointing */

/* End of checkpointing feature definitions. */


#define TRUE    (1)     /* TRUE value */
#define FALSE   (0)     /* FALSE value */

#ifdef RT
extern long filespace;
#endif

struct  stat    Statb, Xstatb, SaveStatb;

/*
 * Cpio header format
 *
 *   Although the fields h_dev and h_ino used to contain the actual
 *   device and inode values of the file, they now simply contain
 *   a distinct value pair that is used to distinguish which files are
 *   linked to what.
 *
 *   h_dev  values from 1->65535 (inclusive)
 *   h_ino  values from 2->65534 (inclusive)
 *
 *   h_dev never has the value 0 (for historical reasons)
 *   h_ino never has the values 0 or 1 (for historical reasons)
 *
 *   The value of 65535 for h_ino indicates that it is unknown to
 *   which file it should be linked (so an extra copy is made).  This
 *   is for compatability with older/foreign versions of cpio.
 */
struct header {
    short   h_magic,		/* cpio magic number */
	    h_dev;		/* device id */
    ushort  h_ino,		/* inode number */
	    h_mode,		/* file mode */
	    h_uid,		/* user ID of file owner */
	    h_gid;		/* group ID of file group */
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
    short   h_Secsize;
    short   h_Mld;
    short   h_Tag;
#endif
    short   h_nlink,		/* number of links */
	    h_rdev,		/* device ID; only for special files */
	    h_mtime[2],		/* last modification time */
	    h_namesize,		/* length of path name h_name */
	    h_filesize[2];	/* file size */
    char    h_name[MAXPATHLEN]; /* null-terminated path name */
} Hdr;


#if defined(DUX) || defined(DISKLESS)
int bit[MAXCOMPONENTS]; /* bit string to tell if a part of the file is cdf */
int cnt;                /* how many components are there */
#endif

struct l_header {
	short	l_magic;
	dev_t	l_dev;
	ino_t   l_ino;
	ushort	l_mode,
		l_uid,			/* XXX -- too short ??? */
		l_gid;			/* XXX -- too short ??? */
	short	l_nlink;
	dev_t	l_rdev;
	short	l_mtime[2],
		l_namesize,
		l_filesize[2];
	char    l_name[MAXPATHLEN];
} Lhdr;


unsigned	Bufsize = BUFSIZE;	/* default record size */
short   Buf[CPIOBSZ/2], *Dbuf, *Dsave;
char    BBuf[CPIOBSZ], *Cbuf, *Csave;
int	Wct, Wc;

short	*Wp;
char    *Cp;

#ifdef RT
    short Actual_size[2];
#endif

int	Select;
short	Option,
	Dir,
	Uncond,
	Link,
	Rename,
	Toc,
	Verbose,
	Mod_time,
	Acc_time,
	Cflag,
#ifdef QFLAG
	Qflag,
#endif
	fflag,
#ifdef SYMLINKS
	hflag,
#endif /* SYMLINKS */
#ifdef ACLS
        aclflag = 0,
#endif /* ACLS */
	Swaphdrs,
#ifdef RT
	Extent,
#endif
	Swap,
	byteswap,
	bothswap,
	halfswap,
	Alien,
#if defined(RFA)
        Remote,
#endif /* RFA */
	Xflag,
	Resync = 0,             /* set if invoked with R option (resyncing) */
	Chkpt = 0,              /* set if invoked with C option (checkpointing) */
	had_good_hdr = FALSE,   /* set if we've seen a good header */
	bad_magic = 0,          /* number of files not able to recover */
	bad_try = 0,		/* file/dir could not be created */
	isstream = 0,           /* True if we're writing to a 7974 or 7978 */
				/* and immediate report mode is enabled. */
	ReelNo = 1,             /* Sequence number for current reel. */
	writestate = NORMAL;    /* Write state (NORMAL,CHECKPT,FIRSTPART). */

size_t	Nbytessaved,            /* Number of bytes saved in checkpointing buffer. */
	FirstfilePart;          /* Part of file being written on new reel. */
				/* (HEADER, BODY). */
short	had_first_read = FALSE;	/* set TRUE after first succesful read */

int	Ifile,
	Ofile,
	Input = 0,
	Output = 1;
long    Blocks = 0,
	Longfile,

	StartOffset,    /* Starting offset in "first" file archived on this */
			/* reel (offset is either in header or body of file). */
	ReelBlocks = 0, /* Number of blocks written to current reel. */

	Longtime;

char    Fullname[MAXPATHLEN],
	Name[MAXPATHLEN];
int	Pathend;
int	usrmask = 0;


FILE    *Rtty = (FILE *)0,
	*Wtty = (FILE *)0,
	*inputfp = stdin,       /* Normally, inputfp == stdin */
	*tempfp,                /* Pointer for file with file name list. */
	*devttyout;		/* for terminal device output */

#if defined(SecureWare) && defined(B1)
char    **Pattern;
#else
char    *All_Fit_Pattern = "*";
char    **Pattern = &All_Fit_Pattern;
#endif
char	Strhdr[MAXPATHLEN+76+2]; /* Strhdr is used as a buffer for the    */
				 /* Header String.  This string must be   */
				 /* capable of supporting MAXPATHLEN +    */
				 /* 76 bytes for Header Information (in   */
				 /* ascii) plus 2 bytes for null termin-  */
				 /* ator (normally 1 bytes is enough how- */
				 /* an extra byte doesn't hurt.		  */
char	*Chdr = Strhdr;
dev_t	Dev;
uid_t	Uid;
gid_t	Gid;
short	A_directory,
	A_special,
	L_special = 0;		/* symbolic link */

#ifdef RFA
short	N_special;		/* network special file	*/
#endif

#ifdef QFLAG
	/*
	 * indicates target file or directory is on
	 * remote file system (NFS)
	 */
	short went_remote;
#endif

#ifdef RT
	short One_extent, Multi_extent;
#endif

int	Filetype = S_IFMT;

#ifdef QFLAG
    /*
     * high byte of st_dev all ones means file is remote via NFS
     */
#   define NFS_remote	037700000000
#endif

char 	*cd();
/*	char	*Cd_name;	*/
long lseek();

#if defined(NLS) || defined(NLS16)
    nl_catd catd,catd2,catopen();
#endif

union { long l; short s[2]; char c[4]; } U;

/* for VAX, Interdata, ... */
long mklong(v)
short v[];
{
	U.l = 1;
	if (U.c[0])
		U.s[0] = v[1], U.s[1] = v[0];
	else
		U.s[0] = v[0], U.s[1] = v[1];
	return U.l;
}

unsigned short
usswap(i)
unsigned short i;
{
	short j;

	swab((char *)&i, (char *)&j, 2);
	return j;
}

short
sswap(i)
short i;
{
	short j;

	swab((char *)&i, (char *)&j, 2);
	return j;
}

long
lswap(i)
long i;
{
	union Conv {
	long longi;
	struct {
		short x,y;
		} shorts;
	} conv;

	swab((char *)&i, (char *)&conv, 4);
	/* for vax only */
/*  This doesn't seem to be needed for cpio from Vax??
	t = conv.shorts.x;
	conv.shorts.x = conv.shorts.y;
	conv.shorts.y = t;
*/
	/* end vax only */
	return conv.longi;
}

/* Function for stripping of leading & trailing blanks and tabs
   of the filenames being read from stdin or temp file
 */

static char   *strip( s )
char   *s;
{
 register      int     l1 = strlen( s );
 register      int     l2 = strspn( s, " \t" );
 if ( l1 == l2 ) /* line consist of ' 's and '\t's only */ return (char *) NULL;

 if ( l2 )       /* There are leading blanks, let's remove it */
  {
   strcpy( s, &s[l2] );
   l1 -= l2;
  }

  /* remove trailing blanks if there are any */
 while ( s[l1-1]==' ' || s[l1-1]=='\t' ) { s[l1-1] = '\0'; l1--; }
 return s;
}      /* End of strip */


char *remove2slash( s )
char *s;
{/*
  * Let's convert all occurences of substring "+//" in full filename into "+/", without
  * this conversion if user got output of cpio -ov ... and trying use it as patterns for restoring via
  * cpio -i ... "patterns" ... , s/he can't restore CDFs ( they don't match )
  */

 char    *p;
 static char  tmp[MAXPATHLEN];

 strcpy( tmp, s );
 while( (p = strstr(tmp,"+//")) != (char *) NULL ) strcpy(p+1, p+2);
 return tmp;
}      /* End of remove2slash */


/*	Uncomment the following in order to enable output debug information to  
	a trace file named  'cpio.debug' 

	Uncomment XDB to enable a infinite loop at the beginning of the 
	execution.  Thus allowing cpio to be started up with appropriate
	pipes and parameters.  XDB can then attach to the appropriate PID
	in order to trace execution.  When attaching, set the value of 
	'forever' to 0 (FALSE) to drop out of the infinite while loop.

*/

/*
#define DEBUG1 TRUE
#define XDB TRUE
*/


#ifdef DEBUG1
#ifndef F_GETFL
#include <fcntl.h>
#endif 
	FILE *dbg_stream;
#endif

/*  Command: cpio  (main)					    */
/* 								    */
/*  History: 
/*	JUN 24, 1992	jlee	modified -p option to save a base   */
/*				path name so that whenever a call   */
/*				causes the Fullpathname to be 	    */
/* 				modified, it will be possible to    */
/*				restore the original target path.   */
/*								    */
/*								    */
/*								    */

main(argc, argv)
char **argv;
{
	register ct;
	long	filesz;
	register char *fullp;
	register i;
	int ans;
#ifdef QFLAG
	char    skip_msg[MAXPATHLEN+40]; /* null-terminated message */
#endif /* QFLAG */
	char    Basename[MAXPATHLEN];
	short   baselength;

#ifdef DEBUG1
	int flcntrl;
	unsigned int flgmodes=0;
#endif

/* Do forever, XDB trap */
#ifdef XDB
	char forever;

	forever=TRUE;
	
	while(forever);
#endif

#ifdef DEBUG1

	dbg_stream = fopen("cpio.debug","a+");

	flcntrl = fcntl(STDIN_FILENO, F_GETFL , &flgmodes);
	fprintf(dbg_stream,"File control modes/flags for stdin: %x , errno = %x \n",flgmodes,errno);

	flcntrl = fcntl(STDOUT_FILENO, F_GETFL, flgmodes);
	fprintf(dbg_stream,"File control modes/flags for stdout: %x , errno = %x \n",flgmodes,errno);
#endif 

#ifdef NLS || NLS16
    struct locale_data *ld;
    char tmplang[SL_NAME_SIZE];
 

    if (!setlocale(LC_ALL, "")) {
        fprintf(stderr,"%s\n", _errlocale(argv[0]));
        putenv("LANG=");
        catd = catd2 = (nl_catd)-1;
    }
    else {
        catd = catopen("cpio",0);
	ld = getlocale(LOCALE_STATUS);
	if (strcmp(ld->LC_ALL_D,ld->LC_TIME_D)) {
		sprintf(tmplang,"LANG=%s",ld->LC_TIME_D);
		putenv(tmplang);
		catd2 = catopen("cpio",0);
		sprintf(tmplang,"LANG=%s",ld->LC_ALL_D);
		putenv(tmplang);
	}
	else
		catd2 = catd;
    }
#endif /* NLS || NLS16 */

#if defined(SecureWare) && defined(B1)
	if (ISB1) {
#ifdef MLTAPE
		ie_init(argc, argv, 1);
#else
		ie_init(argc, argv, 0);
#endif
	}
#endif

	signal(SIGSYS, SIG_IGN);
	if (argc<2 || *argv[1] != '-')
		usage();
	Uid = getuid();
	Gid = getgid();

#if defined(SecureWare) && defined(B1)
	if (ISB1)
		cpio_get_args(&argc, argv);
#endif
	while (*++argv[1]) {
		switch (*argv[1]) {
		case 'a':		/* reset access time */
			Acc_time++;
			break;
#ifdef ACLS
                case 'A':               /* keep ACLs quiet */
                        aclflag = 1;
                        break;
#endif /* ACLS */
		case 'B':		/* change record size to 5120 bytes */
			Bufsize = 5120;
			break;
#if defined(SecureWare) && defined(B1) &&defined(MLTAPE)
		case 'A':
			Aflag++;
			break;
		case 'P':
			Pflag++;
			break;
#endif
		case 'i':
			Option = IN;
			setbuf(stdout, NULL);
			if (argc > 2) {	/* save patterns, if any */
#ifndef SecureWare
			Pattern = argv+2;
#endif
			}
			break;
		case 'f':      /* do not consider patterns in cmd line */
			fflag++;
			break;
#ifdef SYMLINKS
		case 'h':	/* follow symbolic links */
			hflag++;
			break;
#endif /* SYMLINKS */
		case 'o':
			if (argc != 2)
				usage();
			Option = OUT;
			break;
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
		/* We already use 'P' option above */
#else
		case 'P':	/* read file written without the -c option..
				   .. on PDP-11 or VAX */
			Swaphdrs++;
			break;
#endif
		case 'p':
			if (argc != 3)
				usage();
			if (access(argv[2], 2) == -1) {
accerr:
				fprintf(stderr,catgets(catd,NL_SETN,2, "cannot write in <%s>\n"), argv[2]);
				exit(2);
			}
			strcpy(Fullname, argv[2]);   /* destination directory */


			/* check if directory entered is terminated with a '/'*/
			/* if yes, it's ok, otherwise add a '/'               */

			baselength = strlen(Fullname);
			if ( baselength > 0 && Fullname[baselength-1] != '/')
			  strcat(Fullname, "/");

			strcpy(Basename,Fullname); /* Save a copy of base path */
			stat(Fullname, &Xstatb);
			if ((Xstatb.st_mode&S_IFMT) != S_IFDIR)
				goto accerr;
			Option = PASS;
			setbuf(stdout, NULL);
			Dev = Xstatb.st_dev;
  			if (Xstatb.st_remote)
			    Dev = !Dev;
			break;
#ifdef QFLAG
		case 'Q':               /* hidden flag for s300 6.5 HP-UX update */
			Qflag++;
			break;
#endif /* QFLAG */
		case 'c':		/* ASCII header */
			Cflag++;
			break;
		case 'd':		/* create directories when needed */
			Dir++;
			break;
		case 'l':		/* link files, when necessary */
			Link++;
			break;
		case 'm':		/* retain mod time */
			Mod_time++;
			break;
		case 'r':		/* rename files interactively */
			Rename++;
			Rtty = fopen("/dev/tty", "r");
			Wtty = fopen("/dev/tty", "w");
			if (Rtty==NULL || Wtty==NULL) {
				fprintf(stderr,
					catgets(catd,NL_SETN,3, "Cannot rename (/dev/tty missing)\n"));
				exit(2);
			}
			break;
		case 'S':		/* swap halfwords */
			halfswap++;
			Swap++;
			break;
		case 's':		/* swap bytes */
			byteswap++;
			Swap++;
			break;
		case 'b':
			bothswap++;
			Swap++;
			break;
		case 't':		/* table of contents */
			Toc++;
			break;
		case 'u':		/* copy unconditionally */
			Uncond++;
			break;
		case 'v':		/* verbose table of contents */
			Verbose++;
			break;
		case 'x':		/* save or restore dev. special files */
			Xflag++;
			break;
		case '6':		/* for old, sixth-edition files */
			Filetype = 060000;
			break;
#ifdef RT
		case 'e':
			Extent++;
			break;
#endif /* RT */
		case 'R':               /* Resynchronizing option */
			Resync++;
			break;

		case 'C':               /* Checkpointing option. */
			Chkpt++;
			break;

		case 'U':		/* use umask() value */
			usrmask = umask(0);
			umask(usrmask);
			break;

		default:
			usage();
		}
	}
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
	Cflag = 1;
#endif
	if (!Option) {
		fprintf(stderr,catgets(catd,NL_SETN,4, "Options must include o|i|p\n"));
		exit(2);
	}
#ifdef RT
		setio(-1,1);		/* turn on physio */
#endif /* RT */

	if (Resync && Option != IN)
	    fprintf(stderr, catgets(catd,NL_SETN,5, "`R' option is irrelevant with the '-%c' option\n"), Option == PASS ? 'p':'o');

	if (Chkpt && Option != OUT) {
	    fprintf(stderr, catgets(catd,NL_SETN,94, "`C' option is irrelevant with the '-%c' option\n"), Option == PASS ? 'p':'i');
	    Chkpt = 0;
	}

	if (Option == PASS) {
/*
 * -p and -r can be used together
 * 5/10/89 by kazu
		if (Rename) {
		        fprintf(stderr,catgets(catd,NL_SETN,6, "Pass and Rename cannot be used together\n"));
		        exit(2);
		}
*/
		if (Bufsize == 5120) {
	        	fprintf(stderr,catgets(catd,NL_SETN,7, "`B' option is irrelevant with the '-p' option\n"));
	        	Bufsize = BUFSIZE;
	        }
	}else {
		if (Cflag) {
			Cp = Cbuf = (char *)malloc (Bufsize);
			if (Chkpt)
			    /* Create checkpointing buffer. */
			    Csave = (char *)malloc (Bufsize);
		} else {
			Wp = Dbuf = (short *)malloc(Bufsize);
			if (Chkpt)
			    /* Create checkpointing buffer. */
			    Dsave = (short *)malloc(Bufsize);
		}
	}

	Wct = Bufsize >> 1;
	Wc = Bufsize;

#if defined(SecureWare) && defined(B1)
	if (ISB1) {
	    if (Option==PASS)
	   	swfile = argv[0];
	    ie_check_device(
				swfile,
#ifdef MLTAPE
                                AUTH_DEV_MULTI,
#else
				AUTH_DEV_SINGLE,
#endif
				(Option==IN   ? AUTH_DEV_IMPORT  :
				(Option==OUT  ? AUTH_DEV_EXPORT  :
						AUTH_DEV_PASS)),
				(Option==IN   ? Input		 :
				(Option==OUT  ? Output           :
						NULL))
			    );
	}
#endif /* SecureWare && B1 */

	switch (Option) {

	case OUT:		/* get filename, copy header and file out     */
		isstream = isstreamer(Output);
		if (Chkpt) {
		    /* Get file pointer for temporary file which will hold
		     * file names for checkpointing.
		     */

		    if( (tempfp=tmpfile()) == (FILE *)0) {
			fprintf(stderr, catgets(catd,NL_SETN,95, "Can't create temporary file in directory %s for file name list.\n"),P_tmpdir);
			exit(CHKPTERR);
		    }

		    /* Initialize checkpointing variables. */

		    ReelNo = 1;         /* Sequence number for 1st reel. */
		    writestate = NORMAL;
		    Nbytessaved = 0;    /* Zero bytes saved in checkpoint buffer. */
		    FirstfilePart = HEADER; /* Start with header of first file. */
		    StartOffset = 0L;   /* Start at beginning of header. */
		    ReelBlocks = 0;     /* Zero blocks written to current reel. */
		    inputfp = stdin;    /* Read file names from stdin. */
		}
checkpoint:
		while (getname()) {
			short read_bad_block; /* TRUE if "read" from current file failed */

			read_bad_block = FALSE; /* reset flag for new file */
			if (A_special) { /* special file */
				/* Encode the 32bit _rdev into the filesize   */
				/* fields; only for special files	      */
				MKSHORT(Hdr.h_filesize, Lhdr.l_rdev);

				writeheader(writestate);

				/* Previous "write" may necessitate rewrite. */
				if (writestate == CHECKPT)
				    goto checkpoint;

#ifdef RT
				if (One_extent || Multi_extent) {
					actsize(0);

					writeheader(writestate);
					/* Previous "write" may necessitate rewrite. */
					if (writestate == CHECKPT)
					    goto checkpoint;
				}
#endif /* RT */
				if (Verbose)
				  fprintf(stderr, catgets(catd,NL_SETN,96, "%s\n"), Hdr.h_name);
				continue;
			} /* end of character special file handler */

			if (!L_special) {
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
			if(ie_ismld(Hdr.h_name))
		      		enablepriv(SEC_MULTILEVELDIR);
#endif
			    if ((Ifile = open(Hdr.h_name, 0)) < 0) {
				fprintf(stderr,catgets(catd,NL_SETN,97, "Cannot open <%s>.\n"), Hdr.h_name);
				perror("open");
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
				if(ie_ismld(Hdr.h_name))
		      			disablepriv(SEC_MULTILEVELDIR);
#endif
				continue;
			    }
			}
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
			if(ie_ismld(Hdr.h_name))
		      		disablepriv(SEC_MULTILEVELDIR);
#endif

			writeheader(writestate);

			/* Previous "write" may necessitate rewrite. */
			if (writestate == CHECKPT)
			    goto checkpoint;

#ifdef RT
			if (One_extent || Multi_extent) {
				actsize(Ifile);
				writeheader(writestate);

				/* Previous "write" may necessitate rewrite. */
				if (writestate == CHECKPT)
				    goto checkpoint;

				Hdr.h_filesize[0] = Actual_size[0];
				Hdr.h_filesize[1] = Actual_size[1];
			}
#endif /* RT */

			if (writestate == FIRSTPART && FirstfilePart == BODY) {
#ifdef DEBUG
fprintf(stderr, "Seek to byte offset %d of first file. ", StartOffset);
#endif DEBUG
			    if (lseek(Ifile, StartOffset, 0) == -1) {
				perror("lseek failed");
				exit(CHKPTERR);
			    }

			 filesz = mklong(Hdr.h_filesize) - StartOffset;
#ifdef DEBUG
fprintf(stderr, "Reading %ld instead of %ld bytes.\n", filesz, mklong(Hdr.h_filesize));
#endif /* DEBUG */
			} else
			    filesz = mklong(Hdr.h_filesize);

			for (; filesz>0; filesz-= CPIOBSZ) {
				ct = filesz>CPIOBSZ? CPIOBSZ: filesz;

#ifdef SYMLINKS
				if (L_special) {
					if (readlink(Hdr.h_name, Cflag? BBuf: (char *)Buf, CPIOBSZ) < 0) {
						fprintf(stderr,catgets(catd,NL_SETN,10, "Cannot read %s\n"), Hdr.h_name);
						read_bad_block = TRUE;
					}
				} else {
#endif /* SYMLINKS */

#ifdef DEBUG1
					fprintf(dbg_stream,"Executing a READ call -- pt 1\n");
#endif
					if (read(Ifile, Cflag? BBuf: (char *)Buf, ct) <= 0) {
						fprintf(stderr,catgets(catd,NL_SETN,11, "Cannot read %s\n"), Hdr.h_name);
						(void) lseek(Ifile, (long)ct, 1); /* "skip" over bad block */
						read_bad_block = TRUE;
					}
#ifdef SYMLINKS
				}
#endif /* SYMLINKS */
				Cflag? writehdr(BBuf,ct, BODY): bwrite(Buf,ct, BODY);

				/* Previous "write" may necessitate rewrite. */
				if (writestate == CHECKPT)
				    goto checkpoint;

			}
			if (read_bad_block == TRUE) {
			    fprintf(stderr, catgets(catd,NL_SETN,12, "warning: contents of %s are corrupt on archive\n"), Hdr.h_name);
			}

			if (!L_special) {
			    close(Ifile);
			}

			if (Acc_time && !L_special) {
			    struct utimbuf timebuf;

			    timebuf.actime = Statb.st_atime;
			    timebuf.modtime = Statb.st_mtime;
			    utime(Hdr.h_name, &timebuf.actime);
			}
			if (Verbose)
			 fprintf(stderr, catgets(catd,NL_SETN,98, "%s\n"), remove2slash(Hdr.h_name));
		}

	/* copy trailer, after all files have been copied */
		strcpy(Hdr.h_name, "TRAILER!!!");
		Hdr.h_magic = MAGIC;
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
		Hdr.h_Secsize = 0;
#endif
		MKSHORT(Hdr.h_filesize, 0L);
		Hdr.h_namesize = strlen("TRAILER!!!") + 1;


		if (Cflag)
		    bintochar(0L);
		writeheader(writestate);

		/* Previous "write" may necessitate rewrite. */
		if (writestate == CHECKPT)
		    goto checkpoint;


		switch (writestate) {
		    case NORMAL:
			Cflag? writehdr(Cbuf, Bufsize, BODY): bwrite(Dbuf, Bufsize, BODY);
			break;

		    case FIRSTPART:
#ifdef DEBUG
fprintf(stderr, "Write body of TRAILER starting at offset %d\n", StartOffset);
#endif /* DEBUG */
			if (Cflag)
			    writehdr(Cbuf+StartOffset, Bufsize-StartOffset, BODY);
			else
			    bwrite(((char *)Dbuf)+StartOffset, Bufsize-StartOffset, BODY);
			break;

		    case CHECKPT:
			fprintf(stderr, catgets(catd,NL_SETN,99, "unexpected write state: CHECKPT\n"));
			exit(CHKPTERR);
			break;

		    default:
			fprintf(stderr, catgets(catd,NL_SETN,100, "Unknown write state (%d).\n"), writestate);
			exit(CHKPTERR);
			break;
		}

		/* Previous "write" may necessitate rewrite. */
		if (writestate == CHECKPT)
		    goto checkpoint;

		/* Done. */
		break;

	case IN:
		pwd(); /* into Fullname[] with trailing '/' */
		while (gethdr()) {
			Ofile = ckname(Hdr.h_name)? openout(Hdr.h_name): 0;
#ifdef QFLAG
			/*
			 * if some subroutine (openout,missdir, or
			 * makdir) found the target was remote via NFS,
			 * then advance past this file in the archive
			 * and emit appropriate message.
			 */
			if (Qflag && went_remote) {
			    for (filesz=mklong(Hdr.h_filesize); filesz>0; filesz-= CPIOBSZ) {
				ct = filesz>CPIOBSZ? CPIOBSZ: filesz;
				Cflag? readhdr(BBuf,ct): bread(Buf, ct);
			    }
			    if (!Select)
				continue;
			    if (Verbose)
				if (Toc)
					pentry(Hdr.h_name);
				else {
					strcpy(skip_msg,"Skipped remote ");
					if (A_directory)
						strcat(skip_msg,"directory: ");
					else
						strcat(skip_msg,"file: ");
					strcat(skip_msg,Hdr.h_name);
					puts(skip_msg);
				/*	puts(Hdr.h_name); */
				}
			    else if (Toc)

				puts(Hdr.h_name);
			    continue;
			}
#endif /* QFLAG */

			/*
			 * Skip over symbolic link files iff
			 * ckname(file) returned TRUE.
			 */

			if (!(L_special && ckname(Hdr.h_name))) {
			    for (filesz=mklong(Hdr.h_filesize); filesz>0; filesz-= CPIOBSZ) {
				ct = filesz>CPIOBSZ? CPIOBSZ: filesz;
				Cflag? readhdr(BBuf,ct): bread(Buf, ct);
				if (Ofile) {
				    if (Swap)
					Cflag? swap(BBuf,ct): swap(Buf,ct);
				    if (write(Ofile, Cflag? BBuf: (char *)Buf, ct) < 0) {
					fprintf(stderr,catgets(catd,NL_SETN,13, "Cannot write %s\n"), Hdr.h_name);
					bad_try = 1;
					close(Ofile);
					Ofile = 0;
#if defined (SecureWare) && defined (B1) && defined(MLTAPE)
					goto not_imported;
#else
					continue;
#endif
				    }
				}
			    }
			}

		      /*
		       * openout always returns 0 for ".", nevertheless
		       * it's ought to change permission of current "."
		       * to one of being restored "."   ( for FSDdt06607 )
		       */
			if ( Ofile || !strcmp( Hdr.h_name, "." ) ) {
				if (Ofile) close(Ofile);
#if defined(SecureWare) && defined(B1)
#ifdef MLTAPE
				if (ie_chmod(Hdr.h_name, Hdr.h_mode&~usrmask, 0) < A_directory) {
#else
				if (ie_chmod(Hdr.h_name, Hdr.h_mode&~usrmask, 0) < 0) {
#endif
#else
				if (chmod(Hdr.h_name, Hdr.h_mode&~usrmask) < 0) {
#endif
					fprintf(stderr,catgets(catd,NL_SETN,14, "Cannot chmod <%s> (errno:%d)\n"), Hdr.h_name, errno);
				}
				set_time(Hdr.h_name, mklong(Hdr.h_mtime), mklong(Hdr.h_mtime));
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
                                ie_setprivs(Hdr.h_name);
not_imported:
				ie_resetlev();
#endif
			}
			if (!Select)
				continue;
			if (Verbose)
				if (Toc)
					pentry(Hdr.h_name);
				else
					puts(Hdr.h_name);
			else if (Toc)
				puts(Hdr.h_name);
		}
		break;

	case PASS:		/* move files around */
		fullp = Fullname + strlen(Fullname);

		while (getname()) {
			strcpy(Fullname,Basename); /* Restore Base path */
			if (A_directory && !Dir)
				fprintf(stderr,catgets(catd,NL_SETN,15, "Use `-d' option to copy <%s>\n"),Hdr.h_name);
			i = 0;
			while (Hdr.h_name[i] == '/')
				i++;
			strcpy(fullp, &(Hdr.h_name[i])); /* for cpio -pr */
			if (!ckname(fullp))		 /* 5/10/89 kazu */
				continue;
#if defined(DUX) || defined(DISKLESS)
			(void) restore(Fullname,bit,&cnt);
#endif /* defined(DUX) || defined(DISKLESS) */

			if (Link
			&& !A_directory
			&& !L_special
			&& Dev == Statb.st_dev) {
				if (link(Hdr.h_name, Fullname) < 0) { /* missing dir.? */
					if (errno == EEXIST) {
						fprintf(stderr, catgets(catd,NL_SETN,16, "Cannot link <%s> & <%s> (errno:%d)\n"),
						 Hdr.h_name, Fullname, errno);
					} else {
						unlink(Fullname);
						missdir(Fullname);
						if (link(Hdr.h_name, Fullname) < 0) {
							fprintf(stderr, catgets(catd,NL_SETN,17, "Cannot link <%s> & <%s> (errno:%d)\n"), Hdr.h_name, Fullname, errno);
							continue;
						}
					}
				}

/* try creating (only twice) */
				ans = 0;
				do {
					if (link(Hdr.h_name, Fullname) < 0) { /* missing dir.? */
						if (errno == EEXIST) {
							ans = 3;
							break;
							}
						else
							unlink(Fullname);
						ans += 1;
					}else {
						ans = 0;
						break;
					}
				}while (ans < 2 && missdir(Fullname) == 0);
				if (ans == 1) {
					fprintf(stderr,catgets(catd,NL_SETN,18, "Cannot create directory for <%s> (errno:%d)\n"), Fullname, errno);
					exit(2);/***/
				}else if (ans == 2) {
					fprintf(stderr,catgets(catd,NL_SETN,19, "Cannot link <%s> & <%s> (errno:%d)\n"), Hdr.h_name, Fullname, errno);
				}

				if (!Link)
#if defined(SecureWare) && defined(B1)
#ifdef MLTAPE
					if (ie_chmod(Hdr.h_name, Hdr.h_mode&~usrmask, A_directory) < 0) {
#else
					if (ie_chmod(Hdr.h_name, Hdr.h_mode&~usrmask, 0) < 0) {
#endif
#else
					if (chmod(Hdr.h_name, Hdr.h_mode&~usrmask) < 0) {
#endif
						fprintf(stderr,catgets(catd,NL_SETN,20, "Cannot chmod <%s> (errno:%d)\n"), Hdr.h_name, errno);
					}

				set_time(Hdr.h_name, mklong(Hdr.h_mtime), mklong(Hdr.h_mtime));
				goto ckverbose;
			}

#ifdef RT
			if (One_extent || Multi_extent)
				actsize(0);
#endif /* RT */
			if (!(Ofile = openout(Fullname)))
				continue;

			if ((Ifile = open(Hdr.h_name, 0)) < 0) {
				fprintf(stderr, catgets(catd,NL_SETN,97, "Cannot open <%s>.\n"), Hdr.h_name);
				close(Ofile);
				continue;
			}
			filesz = Statb.st_size;
			for (; filesz > 0; filesz -= CPIOBSZ) {
				ct = filesz>CPIOBSZ? CPIOBSZ: filesz;
#ifdef DEBUG1
				fprintf(dbg_stream,"Execute a read call -- pt 2\n");
#endif
				if (read(Ifile, Buf, ct) < 0) {
					fprintf(stderr,catgets(catd,NL_SETN,21, "Cannot read %s\n"), Hdr.h_name);
					bad_try = 1;
					break;
				}
				if (Ofile)
					if (write(Ofile, Buf, ct) < 0) {
						fprintf(stderr,catgets(catd,NL_SETN,22, "Cannot write %s\n"), Hdr.h_name);
						bad_try = 1;
						break;
					}
				Blocks += ((ct + (BUFSIZE - 1)) / BUFSIZE);
			}
			close(Ifile);
			if (Acc_time && !L_special) {
			    struct utimbuf timebuf;

			    timebuf.actime = Statb.st_atime;
			    timebuf.modtime = Statb.st_mtime;
			    utime(Hdr.h_name, &timebuf.actime);
			}
			if (Ofile) {
				close(Ofile);
#if defined(SecureWare) && defined(B1)
#ifdef MLTAPE
				if (ie_chmod(Fullname, Hdr.h_mode&~usrmask, A_directory) < 0) {
#else
				if (ie_chmod(Fullname, Hdr.h_mode&~usrmask, 0) < 0) {
#endif
#else
				if (chmod(Fullname, Hdr.h_mode&~usrmask) < 0) {
#endif
					fprintf(stderr,catgets(catd,NL_SETN,23, "Cannot chmod <%s> (errno:%d)\n"), Fullname, errno);
				}
				set_time(Fullname, Statb.st_atime, mklong(Hdr.h_mtime));
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
				ie_copyattr(Hdr.h_name,Fullname);
				ie_resetlev();
#endif
ckverbose:
				if (Verbose)
					puts(Fullname);
			}
		}
	}


	/* Report number of files unable to restore:
	 * (this message is only given if the resyncing
	 *  option was specified)
	 */

	if (bad_magic) {
#ifndef NLS
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
	    fprintf(stderr, "mltape: Unable to restore at least ");
#else
	    fprintf(stderr, "cpio: Unable to restore at least ");
#endif
	    fprintf(stderr, "%d %s\n", bad_magic, (bad_magic == 1) ? "file." : "files.");
#else
	    if (bad_magic == 1)
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
		fprintf(stderr, catgets(catd,NL_SETN,24, "mltape: Unable to restore at least %d file.\n"), bad_magic);
	    else
		fprintf(stderr, catgets(catd,NL_SETN,25, "mltape: Unable to restore at least %d files.\n"), bad_magic);
#else
		fprintf(stderr, catgets(catd,NL_SETN,24, "cpio: Unable to restore at least %d file.\n"), bad_magic);
	    else
		fprintf(stderr, catgets(catd,NL_SETN,25, "cpio: Unable to restore at least %d files.\n"), bad_magic);
#endif
#endif /* NLS */
	}

	/* print number of blocks actually copied */
	fprintf(stderr,catgets(catd,NL_SETN,26, "%ld blocks\n"), Blocks * (Bufsize>>9));
	exit(bad_try || bad_magic ? 1: 0);
	/*NOTREACHED*/					
}

#if defined(SecureWare) && defined(B1)
cpio_get_args(argc, argv)
int *argc;
char **argv;
{
int i;

	switch (argv[1][1]) {
	case 'p':
#ifdef MLTAPE
		if (argv[2][0] == '-' && argv[2][1] == 'M') {
			ie_ml_find(argv[3]);
                	for (i = 2; (i+2) <= *argc; i++)
                        	argv[i] = argv[i+2];
               		*argc = *argc - 2;
		}
#endif
		break;
	case 'i':
		if (argc > 0)	/* save any patterns */
			Pattern = argv + 2;
		if (argv[2][0] != '-' || argv[2][1] != 'I')
			usage();
		close(Input);
		swfile = argv[3];
		stopio(swfile);
		if (open(swfile, O_RDONLY) != Input)
			cannotopen(swfile, "input");
		for (i = 2; (i+2) <= *argc; i++)
			argv[i] = argv[i+2];
		*argc = *argc - 2;
		break;
	case 'o':
		if (argv[2][0] != '-' || argv[2][1] != 'O') {
                        usage();
                }
		close(Output);
               	swfile = argv[3];
              	stopio(swfile);
		if (open(swfile, O_WRONLY | O_CREAT | O_TRUNC, 0660) != Output)
                        cannotopen(swfile, "output");
                for (i = 2; (i+2) <= *argc; i++)
                        argv[i] = argv[i+2];
               	*argc = *argc - 2;
#ifdef MLTAPE
		if (argv[2][0] == '-' && argv[2][1] == 'M') {
			ie_ml_find(argv[3]);
                	for (i = 2; (i+2) <= *argc; i++)
                        	argv[i] = argv[i+2];
               		*argc = *argc - 2;
		}
#endif
		break;
	default:
		usage();
	}
}

static
cannotopen(sp, mode)
char *sp;
char *mode;
{
        fprintf(stderr, "Cannot open <%s> for %s.\n", sp, mode);
}
#endif /* SecureWare && B1 */


usage()
{

#if ! defined(SecureWare) || ! defined(B1)

/*
 * Base cpio command without SecureWare modifications
 */
#ifndef NLS
	fprintf(stderr,"Usage: cpio -o[aAcBvxCh] <name-list >collection\n%s\n%s\n",
	"       cpio -i[BcdmPrtuvfsSbx6RU] [pattern ...] <collection",
	"       cpio -p[adlmruvxU] directory <name-list\n");
#else /* NLS */
	fprintf(stderr,catgets(catd,NL_SETN,27, "Usage: cpio -o[aAcBvxCh] <name-list >collection\n"));
	fprintf(stderr,catgets(catd,NL_SETN,28, "       cpio -i[BcdmPrtuvfsSbx6RU] [pattern ...] <collection\n"));
        fprintf(stderr,catgets(catd,NL_SETN,29, "       cpio -p[adlmruvxU] directory <name-list\n\n"));
#endif /* not NLS */

#else  /* SecureWare && B1 */
if (ISB1) {

#ifndef MLTAPE

/*
 * B1 cpio command usage message
 */
#ifndef NLS
	fprintf(stderr,"Usage: cpio -o[aAcBvxCh] -O tape_device <name-list\n%s\n%s\n",
	"       cpio -i[BcdmPrtuvfsSbx6RU] -I tape_device [pattern ...] ",
	"       cpio -p[adlmruvxU] directory <name-list\n");
#else /* NLS */
	fprintf(stderr,catgets(catd,NL_SETN,27, "Usage: cpio -o[aAcBvxCh] -O tape_device <name-list\n"));
	fprintf(stderr,catgets(catd,NL_SETN,28, "       cpio -i[BcdmPrtuvfsSbx6RU] -I tape_device [pattern ...] \n"));
	fprintf(stderr,catgets(catd,NL_SETN,29, "       cpio -p[adlmruvxU] directory <name-list\n\n"));
#endif /* not NLS */

#else  /* MLTAPE */

/* 
 * B1 mltape command usage message
 */
#ifndef NLS
	fprintf(stderr,"Usage: mltape -o[aAPcBvxCh] -O tape_device <name-list\n%s\n%s\n",
	"       mltape -i[BcdmrtuvfsSbx6RU] -I tape_device [pattern ...] ",
	"       mltape -p[adlmruvxU] -M directory <name-list\n");
#else /* NLS */
	fprintf(stderr,catgets(catd,NL_SETN,27, "Usage: mltape -o[aAPcBvxCh] -O tape_device <name-list\n"));
	fprintf(stderr,catgets(catd,NL_SETN,28, "       mltape -i[BcdmrtuvfsSbx6RU] -I tape_device [pattern ...] \n"));
	fprintf(stderr,catgets(catd,NL_SETN,29, "       mltape -p[adlmruvxU] -M directory <name-list\n\n"));
#endif /* not NLS */

#endif /* MLTAPE */

} else {

/*
 * Base cpio command usage message
 */
#ifndef NLS
	fprintf(stderr,"Usage: cpio -o[aAcBvxCh] <name-list >collection\n%s\n%s\n",
	"       cpio -i[BcdmPrtuvfsSbx6RU] [pattern ...] <collection",
	"       cpio -p[adlmruvxU] directory <name-list\n");
#else /* NLS */
	fprintf(stderr,catgets(catd,NL_SETN,27, "Usage: cpio -o[aAcBvxCh] <name-list >collection\n"));
	fprintf(stderr,catgets(catd,NL_SETN,28, "       cpio -i[BcdmPrtuvfsSbx6RU] [pattern ...] <collection\n"));
	fprintf(stderr,catgets(catd,NL_SETN,29, "       cpio -p[adlmruvxU] directory <name-list\n\n"));
#endif /* not NLS */
}

#endif /* not SecureWare || not B1 */

	exit(2);
}

getname()		/* get file name, get info for header */
{
	register char *namep = Name;
	register ushort ftype;
	long tlong;
#ifdef RFA
	Remote = 0;	/* assume it's a local file */
#endif /* RFA */
	for (;;) {
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
		ie_resetlev();
#endif
		if (!Chkpt) {
		    if (gets(namep) == NULL)
			return 0;
		} else {
		    /* Get next file name from "inputfp" file pointer,
		     * which points to "stdin" or the temp file.
		     */

		    if (fgets(namep, MAXPATHLEN, inputfp) == (char *)0) {
			if (inputfp == stdin) {
			    /* Properly reset "writestate" because we still
			     * must write the "TRAILER!!!" of the archive.
			     */
			    if (writestate == CHECKPT)
				writestate = FIRSTPART;
			    return 0;   /* No more file names. */
			} else {
			    inputfp = stdin;
			    continue;
			}  /* End of if inputfp == stdin */
		    }  /* End of if for fgets call */

		    /* Log this file name to temporary file. */
		    if (inputfp == stdin) {
			if (fputs(namep, tempfp) == EOF) {
			    fprintf(stderr, "Can't write to temporary file.\n");
			    exit(CHKPTERR);
			}
		    }


		    /* Remove trailing new-line. */
		    namep[strlen(namep)-1] = '\0';


		    /* Reset "writestate" after we've processed
		     * the first part while we're rewriting this reel.
		     */
		    if (writestate == FIRSTPART)
			writestate = NORMAL;

#ifdef DEBUG
if (inputfp == stdin)
    fprintf(stderr, "Read \"%s\" from stdin and logged it in temp file.\n", namep);
else
    fprintf(stderr, "Read \"%s\" from temp file.\n", namep);
#endif /* DEBUG */
		}

		/* Remove both leading & trailing blanks */
		if ( strip( namep ) == (char *) NULL ) continue;

#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
                RealFile = namep;
                TapeFile = ie_stripmld(namep,&Hdr.h_Mld,&Hdr.h_Tag);
		if (TapeFile == (char *)-1) return(0);
		else if (TapeFile == (char *)0) continue;
                else strcpy(namep, TapeFile);
		if(Hdr.h_Mld)
		   enablepriv(SEC_MULTILEVELDIR);
#endif


		if (*namep == '.' && namep[1] == '/') {
			namep++;
			while (*namep == '/') namep++;
		}

		if (Option == OUT && strcmp(namep, "TRAILER!!!") == 0) {
			fprintf(stderr, catgets(catd,NL_SETN,107, "The file \"TRAILER!!!\" was not backed up.\n"));
			continue;
		}

		strcpy(Hdr.h_name, namep);
#if defined(DUX) || defined(DISKLESS)
		(void) findpath(Hdr.h_name,bit,&cnt);
#endif /* defined(DUX) || defined(DISKLESS) */


		if (writestate == CHECKPT && ReelNo != 1)
		    Statb = SaveStatb;  /* Restore stat info. */
		else {
#ifdef SYMLINKS
		    if (hflag) {
			if (stat(namep, &Statb) < 0) {
			    fprintf(stderr,catgets(catd,NL_SETN,101, "Cannot stat <%s>.\n"), Hdr.h_name);
			    perror_msg(Hdr.h_name);
			    continue;
			}
		    } else {
			if (lstat(namep, &Statb) < 0) {
			    fprintf(stderr,catgets(catd,NL_SETN,101, "Cannot stat <%s>.\n"), Hdr.h_name);
			    perror_msg(Hdr.h_name);
			    continue;
			}
		    }
#else /* SYMLINKS */
		    if (stat(namep, &Statb) < 0) {
			fprintf(stderr, catgets(catd,NL_SETN,101, "Cannot stat <%s>.\n"), Hdr.h_name);
			continue;
		    }
#endif /* SYMLINKS */
		}
		if (S_ISSOCK(Statb.st_mode)) {
		    fprintf(stderr,
			catgets(catd,NL_SETN,112, "Socket <%s> not backed up\n"), namep);
		    continue;
		}

#if defined OLD_RFA
		if ((Statb.st_mode & Filetype) == S_IFNWK) {
		    fprintf(stderr,
			catgets(catd,NL_SETN,109, "Obsolete network special file <%s> not backed up\n"), namep);
		    continue;
		}
#endif /* OLD_RFA */

#ifdef ACLS
                if (!aclflag) {
                    if (Statb.st_acl)
                        fprintf(stderr, catgets(catd,NL_SETN,108, "Optional acl entries for <%s> are not backed up.\n"), namep);
                }
#endif /* ACLS */

		/* We're ready to process the first part. */
#if defined(RFA)
		Remote = Statb.st_remote;
#endif /* RFA */
		if (writestate == CHECKPT)
		    writestate = FIRSTPART;


		ftype = Statb.st_mode & Filetype;
		if (A_directory = (ftype == S_IFDIR))
		    Statb.st_size = 0;
#ifdef SYMLINKS
		L_special = (ftype == S_IFLNK && !hflag);
#endif /* SYMLINKS */
#if defined(RFA)
		/*
		 * network special files aren't A_special --
		 * contents must be transported.
		 */
		N_special = (ftype == S_IFNWK);
#endif /* RFA */
		A_special = (ftype == S_IFBLK)
			|| (ftype == S_IFCHR)
			|| (ftype == S_IFIFO);

#if defined(RFA)
		if (A_special || N_special)
#else
		if (A_special)
#endif /* RFA */
			if (!Xflag)
				continue;
#ifdef RT
			A_special |= (ftype == S_IFREC);
			One_extent = (ftype == S_IF1EXT);
			Multi_extent = (ftype == S_IFEXT);
#endif /* RT */
		Lhdr.l_magic = MAGIC;
		Lhdr.l_namesize = strlen(Hdr.h_name) + 1;
		Lhdr.l_uid = Statb.st_uid;
		Lhdr.l_gid = Statb.st_gid;
		Lhdr.l_dev = Statb.st_dev;
		Lhdr.l_ino = Statb.st_ino;
		Lhdr.l_mode = Statb.st_mode;
		MKSHORT(Lhdr.l_mtime, Statb.st_mtime);
		Lhdr.l_nlink = Statb.st_nlink;
		tlong = A_special? 0L: Statb.st_size;
#ifdef CNODE_DEV
                /* Put the conde id Stab.st_rcnode, from the previous stat(2)
                 * into the last element of the name field "temporarily".
                 * The routine copy_to_hdr() will move it to the Hdr.h_rdev
                 * field, which eventually gets written to the target
                 */
        if (A_special && Statb.st_rcnode >= 0 && Statb.st_rcnode <= 255) {
                Lhdr.l_name[MAXPATHLEN-1] = Statb.st_rcnode;
        }
        else
                Lhdr.l_name[MAXPATHLEN-1] =  -1;
#endif /* CNODE DEV */

#ifdef RT
		if (One_extent || Multi_extent) tlong = Statb.st_size;
#endif /* RT */

#if defined(SecureWare) && defined(B1)
#ifdef MLTAPE
                Hdr.h_Secsize = ie_ml_export(namep,L_special,A_directory);
		disablepriv(SEC_MULTILEVELDIR);
		if(!Hdr.h_Secsize)
			continue;
#else /* not MLTAPE */
		if (ISB1) {
		    if (!ie_sl_export(namep))
		   	continue;
		}
#endif /* MLTAPE */
#endif /* SecureWare && B1 */

		MKSHORT(Lhdr.l_filesize, tlong);
		Lhdr.l_rdev = Statb.st_rdev;
		copy_to_hdr();	/*  4/5/82 Move Lhdr to Hdr fields */
		if (Cflag)
			bintochar(A_special? Lhdr.l_rdev : tlong);
			/*  6/29/82 Write 32bit _rdev if special file */
                        /*  5/26/83 Make sure to use long value of rdev */

		return 1;
	}
}

bintochar(t)		/* ASCII header write */
long t;
{
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
        sprintf(Chdr,
   "%.6ho%.6ho%.6ho%.6o%.6ho%.6ho%.6ho%.6ho%.6ho%.6ho%.6ho%.11lo%.6ho%.11lo%s",
               MAGIC,
                Hdr.h_dev,
                Hdr.h_ino,
                Statb.st_mode,
                Statb.st_uid,
                Statb.st_gid,
                Hdr.h_Secsize,
                Hdr.h_Mld,
                Hdr.h_Tag,
                Statb.st_nlink,
                Hdr.h_rdev & 00000177777,
                Statb.st_mtime,
                (short)strlen(Hdr.h_name)+1,
                t,
                Hdr.h_name);
#else
	sprintf(Chdr,"%.6o%.6ho%.6ho%.6ho%.6ho%.6ho%.6ho%.6ho%.11lo%.6ho%.11lo%s",
		MAGIC,Hdr.h_dev,Hdr.h_ino,Statb.st_mode,Statb.st_uid,
		Statb.st_gid,Statb.st_nlink,Hdr.h_rdev & 00000177777,
		Statb.st_mtime,(short)strlen(Hdr.h_name)+1,t,Hdr.h_name);
#endif
}

chartobin()		/* ASCII header read */
{
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
        sscanf(Chdr,
           "%6ho%6ho%6ho%6ho%6ho%6ho%6ho%6ho%6ho%6ho%6ho%11lo%6ho%11lo",
                &Hdr.h_magic,
                &Hdr.h_dev,
                &Hdr.h_ino,
                &Hdr.h_mode,
                &Hdr.h_uid,
                &Hdr.h_gid,
                &Hdr.h_Secsize,
                &Hdr.h_Mld,
                &Hdr.h_Tag,
                &Hdr.h_nlink,
                &Hdr.h_rdev,
                &Longtime,
                &Hdr.h_namesize,
                &Longfile);
#else
	sscanf(Chdr,"%6ho%6ho%6ho%6ho%6ho%6ho%6ho%6ho%11lo%6ho%11lo",
		&Hdr.h_magic,&Hdr.h_dev,&Hdr.h_ino,&Hdr.h_mode,&Hdr.h_uid,
		&Hdr.h_gid,&Hdr.h_nlink,&Hdr.h_rdev,&Longtime,&Hdr.h_namesize,
		&Longfile);
#endif
	MKSHORT(Hdr.h_filesize, Longfile);
	MKSHORT(Hdr.h_mtime, Longtime);
}

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
    *dev = lastdev;
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
	fprintf(stderr, catgets(catd,NL_SETN,73, "No memory for links\n"));
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

copy_to_hdr()		/* move Lhdr to Hdr fields */
{
	Hdr.h_magic 		= Lhdr.l_magic;
	Hdr.h_namesize 		= Lhdr.l_namesize;
	Hdr.h_uid 		= Lhdr.l_uid;
	Hdr.h_gid 		= Lhdr.l_gid;
	makedevino(Lhdr.l_nlink, Lhdr.l_dev, Lhdr.l_ino,
	    &Hdr.h_dev, &Hdr.h_ino);
	Hdr.h_mode 		= Lhdr.l_mode;
	Hdr.h_mtime[0] 		= Lhdr.l_mtime[0];
	Hdr.h_mtime[1] 		= Lhdr.l_mtime[1];
	Hdr.h_nlink 		= Lhdr.l_nlink;
	Hdr.h_filesize[0] 	= Lhdr.l_filesize[0];
	Hdr.h_filesize[1] 	= Lhdr.l_filesize[1];
#ifdef CNODE_DEV
        /* Writing to target: Copy the conde id from the last  field of
         * Lhdr.l_name[MAXPATHLEN-1] to the Hdr.h_rdev field. This gets written
to
         * the tape in place of the LOCALMAGIC TAG. This is esentially the
         * only place that we can stuff the info without changing any
         * structures.
        */
        if (Lhdr.l_name[MAXPATHLEN-1] != -1) {
                Hdr.h_rdev = Lhdr.l_name[MAXPATHLEN-1];
                Lhdr.l_name[MAXPATHLEN-1] = -1;
        }
#else
#if defined(RFA)
        Hdr.h_rdev = (A_special || N_special)? LOCALMAGIC : 0;
#else
        Hdr.h_rdev = A_special ? LOCALMAGIC : 0;
#endif /* RFA */
#endif /* CNODE_DEV */
}


lcopy_to_lhdr()		/* move Hdr to Lhdr fields */
{
	long	temp;

	Lhdr.l_magic 		= Hdr.h_magic;
	Lhdr.l_namesize 	= Hdr.h_namesize;
	Lhdr.l_uid 		= Hdr.h_uid;
	Lhdr.l_gid 		= Hdr.h_gid;
	Lhdr.l_dev	 	= Hdr.h_dev;
	Lhdr.l_ino 		= Hdr.h_ino;
	Lhdr.l_mode 		= Hdr.h_mode;
	Lhdr.l_mtime[0] 	= Hdr.h_mtime[0];
	Lhdr.l_mtime[1] 	= Hdr.h_mtime[1];
	Lhdr.l_nlink 		= Hdr.h_nlink;
	Lhdr.l_filesize[0] 	= Hdr.h_filesize[0];
	Lhdr.l_filesize[1] 	= Hdr.h_filesize[1];
	Lhdr.l_rdev 		=  Hdr.h_rdev ;
#ifdef CNODE_DEV
        strncpy(Lhdr.l_name,Hdr.h_name,MAXPATHLEN-1);
        Lhdr.l_name[MAXPATHLEN-2] = '\0'; /* Insure null termination. */
#else
	strncpy(Lhdr.l_name,Hdr.h_name,MAXPATHLEN);
	Lhdr.l_name[MAXPATHLEN-1] = '\0'; /* Insure null termination. */
#endif /* CNODE_DEV */

	Alien = 1;
	if (A_special) {		/*  6/29/82 */
		temp = mklong(Hdr.h_filesize);
		MKSHORT(Hdr.h_filesize, 0L);	/* Do these two lines on all */
		MKSHORT(Lhdr.l_filesize, 0L);  /* HP systems, for HPUX tapes */

#ifdef CNODE_DEV
        /* Put the conde id which was in the Hdr.h_rdev field back into
         * the last field of the Lhdr.l_name[MAXPATHLEN-1] location.
         */
	{
	    short localmagic;

	    localmagic = LOCALMAGIC;
	    Lhdr.l_name[MAXPATHLEN-1] = -1;
	    if (Xflag && ((Hdr.h_rdev >= 0 && Hdr.h_rdev <= 255) ||
			  (Hdr.h_rdev == localmagic)))
	    {
		    Alien = 0;
		    Lhdr.l_name[MAXPATHLEN-1] = Hdr.h_rdev;
		    Lhdr.l_rdev = temp;
	    }
	}
#else
		if (Xflag && (Hdr.h_rdev == (short)LOCALMAGIC)) {
			Alien = 0;
			Lhdr.l_rdev = temp;
		}
#endif /* CNODE_DEV */
	}

#if defined(RFA)
	if (N_special) {
#ifdef CNODE_DEV
		if (Xflag && ((Hdr.h_rdev >= 0 && Hdr.h_rdev <= 255) ||
			      (Hdr.h_rdev == (short)LOCALMAGIC))) {
			Lhdr.l_name[MAXPATHLEN-1] = Hdr.h_rdev;
			Alien = 0;
		}
#else
		if (Xflag && (Hdr.h_rdev == (short)LOCALMAGIC))
			Alien = 0;
#endif /* CNODE_DEV */
	}
#endif /* RFA */
}




/*
 * ascresync  - Try to find next ASCII header in archive.
 */

ascresync()
{
    static char header[1024];
    char *cp;
    int try, items;

    cp = header;
    try = 0;


    /* Read in ASCII header minus file field: */

    readhdr(cp, CHARS);

    while (1) {
	if (*cp == '0') {
#if defined(SecureWare) && defined(B1) && defined (MLTAPE)
            items = sscanf(cp,
        "%6ho%6ho%6ho%6ho%6ho%6ho%6ho%6ho%6ho%6ho%6ho%11lo%6ho%11lo",
                &Hdr.h_magic,
                &Hdr.h_dev,
                &Hdr.h_ino,
                &Hdr.h_mode,
                &Hdr.h_uid,
                &Hdr.h_gid,
                &Hdr.h_Secsize,
                &Hdr.h_Mld,
                &Hdr.h_Tag,
                &Hdr.h_nlink,
                &Hdr.h_rdev,
                &Longtime,
                &Hdr.h_namesize,
                &Longfile);
#else
	    items = sscanf(cp,"%6ho%6ho%6ho%6ho%6ho%6ho%6ho%6ho%11lo%6ho%11lo",
	        &Hdr.h_magic,&Hdr.h_dev,&Hdr.h_ino,&Hdr.h_mode,&Hdr.h_uid,
	        &Hdr.h_gid,&Hdr.h_nlink,&Hdr.h_rdev,&Longtime,&Hdr.h_namesize,
	        &Longfile);
#endif
	    MKSHORT(Hdr.h_filesize, Longfile);
	    MKSHORT(Hdr.h_mtime, Longtime);
	} else
	    items = 0; /* force wrong number of items */

	if (items == 11 && Hdr.h_magic == MAGIC) {
	    had_good_hdr = TRUE;
	    if (try != 0) {
#ifndef NLS
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
		fprintf(stderr, "mltaep: Re-synced after skipping %d ", try);
#else
		fprintf(stderr, "cpio: Re-synced after skipping %d ", try);
#endif
		fprintf(stderr, "%s.\n", (try == 1) ? "byte" : "bytes");
#else
		if (try == 1)
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
			fprintf(stderr, catgets(catd,NL_SETN,30, "mltape: Re-synced after skipping %d byte.\n"), try);
		else
			fprintf(stderr, catgets(catd,NL_SETN,31, "mltape: Re-synced after skipping %d bytes.\n"), try);
#else
			fprintf(stderr, catgets(catd,NL_SETN,30, "cpio: Re-synced after skipping %d byte.\n"), try);
		else
			fprintf(stderr, catgets(catd,NL_SETN,31, "cpio: Re-synced after skipping %d bytes.\n"), try);
#endif
#endif /* NLS */
		bad_magic++; /* another file unrecoverable */
	    }
	    break;
	} else {
	    if (try == 0) {
		if (!Resync)
		    return; /* user didn't give resyncing option */
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
		fprintf(stderr, catgets(catd,NL_SETN,32, "mltape: Out of phase; resyncing.\n"));
#else
		fprintf(stderr, catgets(catd,NL_SETN,32, "cpio: Out of phase; resyncing.\n"));
#endif
	    }

	    try++;

	    if ((char *)&cp[75] < (char *)&header[1023]) {
		cp++;
	    } else {
		/*
		 * Copy last 75 characters of header to
		 * the beginning of the buffer and reset cp:
		 */
		strncpy(header,++cp,75);
		cp = header;
	    }
	}
	readhdr(&cp[75], 1);
    }
}


/*
 * binresync  - Try to find next binary header in archive.
 */

binresync()
{
    short magic;
    int try = 0;

    /* Read in magic number of binary header: */

    bread((char *)&magic, sizeof Hdr.h_magic);

    while (1) {
	if (Swaphdrs)
	    magic = sswap(magic);
#ifdef DEBUG3
 	had_good_hdr = FALSE;
#endif

	if (magic == MAGIC) {
	    had_good_hdr = TRUE;
	    if (try != 0) {
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
		fprintf (stderr, catgets(catd,NL_SETN,33, "mltaep: Re-synced after skipping %d bytes.\n"), try*sizeof Hdr.h_magic);
#else
		fprintf (stderr, catgets(catd,NL_SETN,33, "cpio: Re-synced after skipping %d bytes.\n"), try*sizeof Hdr.h_magic);
#endif
		bad_magic++; /* another file unrecoverable */
	    }
	    break;
	} else {
	    if (try == 0) {
		if (!Resync)
		    return; /* user didn't give resyncing option */
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
		fprintf(stderr, catgets(catd,NL_SETN,34, "mltape: Out of phase; resyncing.\n"));
#else
		fprintf(stderr, catgets(catd,NL_SETN,34, "cpio: Out of phase; resyncing.\n"));
#endif
	    }

	    try++;
	    bread((char *)&magic, sizeof Hdr.h_magic);
	}
    }

    Hdr.h_magic = magic;
    bread(&Hdr.h_dev, HDRSIZE - sizeof Hdr.h_magic);

    if (Swaphdrs) {
	Hdr.h_dev = sswap(Hdr.h_dev);
	Hdr.h_ino = usswap(Hdr.h_ino);
	Hdr.h_mode = usswap(Hdr.h_mode);
	Hdr.h_uid = usswap(Hdr.h_uid);
	Hdr.h_gid = usswap(Hdr.h_gid);
	Hdr.h_nlink = sswap(Hdr.h_nlink);
	Hdr.h_rdev = sswap(Hdr.h_rdev);
	Hdr.h_mtime[0] = sswap(Hdr.h_mtime[0]);
	Hdr.h_mtime[1] = sswap(Hdr.h_mtime[1]);
	Hdr.h_namesize = sswap(Hdr.h_namesize);
	Hdr.h_filesize[0] = sswap(Hdr.h_filesize[0]);
	Hdr.h_filesize[1] = sswap(Hdr.h_filesize[1]);
    }
}


gethdr()		/* get file headers */
{
	register ushort ftype;
#if DEBUG3
	short doforever;
#endif

	if (Cflag)
	    ascresync();
	else
	    binresync();

	if (Hdr.h_magic != MAGIC) {
#if DEBUG3
	fprintf(stderr,"---Launch XDB\n");

	doforever=TRUE;
	while(doforever);
#endif  

		fprintf(stderr,catgets(catd,NL_SETN,35, "Out of phase--get help\n"));
		if (had_good_hdr == TRUE)
		    fprintf(stderr, catgets(catd,NL_SETN,36, "You may want to use \"R\" option\n"));
		else
#ifndef NLS
		    fprintf(stderr,"Perhaps the \"-c\" option %s be used\n", Cflag ? "shouldn't" : "should");
#else
		{
		    if (Cflag)
			fprintf(stderr,catgets(catd,NL_SETN,37, "Perhaps the \"-c\" option shouldn't be used\n"));
		    else
			fprintf(stderr,catgets(catd,NL_SETN,38, "Perhaps the \"-c\" option should be used\n"));
		}
#endif /* NLS */
		exit(2);
	}

	Hdr.h_magic = 0; /* Reset magic number. */

	if (Cflag)
		readhdr(Hdr.h_name, Hdr.h_namesize);
	else
		bread(Hdr.h_name, Hdr.h_namesize);
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
        readhdr(ie_allocbuff(Hdr.h_Secsize),Hdr.h_Secsize);
#endif
	if (EQ(Hdr.h_name, "TRAILER!!!"))
		return 0;
#if defined(DUX) || defined(DISKLESS)
	(void) restore(Hdr.h_name,bit,&cnt);
#endif /* defined(DUX) || defined(DISKLESS) */

	ftype = Hdr.h_mode & Filetype;
	A_directory = (ftype == S_IFDIR);
#ifdef SYMLINKS
	L_special =(ftype == S_IFLNK);
#endif /* SYMLINKS */
#if defined(RFA)
	N_special =(ftype == S_IFNWK);
#endif /* RFA */
	A_special =(ftype == S_IFBLK)
		|| (ftype == S_IFCHR)
		|| (ftype == S_IFIFO);
#ifdef RT
	A_special |= (ftype == S_IFREC);
	One_extent = (ftype == S_IF1EXT);
	Multi_extent = (ftype == S_IFEXT);
	if (One_extent || Multi_extent) {
		Actual_size[0] = Hdr.h_filesize[0];
		Actual_size[1] = Hdr.h_filesize[1];

		if (Cflag) {
			readhdr(Chdr,CHARS);
			chartobin();
		}
		else
			bread(&Hdr, HDRSIZE);

		if (Hdr.h_magic != MAGIC) {
			fprintf(stderr,catgets(catd,NL_SETN,39, "Out of phase--get RT help\n"));
			fprintf(stderr,catgets(catd,NL_SETN,40, "Perhaps the \"-c\" option should be used\n"));
			exit(2);
		}

		if (Cflag)
			readhdr(Hdr.h_name, Hdr.h_namesize);
		else
			bread(Hdr.h_name, Hdr.h_namesize);
	}
#endif /* RT */
	lcopy_to_lhdr();   /* [AN]_special must be set up before call.  6/29 */
	return 1;
}

ckname(namep)	/* check filenames with patterns given on cmd line */
register char *namep;
{
	++Select;
	if (fflag ^ !nmatch(namep, Pattern)) {
		Select = 0;
		return 0;
	}
	if (Rename && !A_directory) {	/* rename interactively */
		fprintf(Wtty, catgets(catd,NL_SETN,41, "Rename <%s>\n"), namep);
		fflush(Wtty);
		fgets(namep, 128, Rtty);
		if (feof(Rtty))
			exit(2);
		namep[strlen(namep) - 1] = '\0';
		if (EQ(namep, "")) {
			printf(catgets(catd,NL_SETN,42, "Skipped\n"));
			return 0;
		}
	}
	return !Toc;
}


openout(namep)	/* open files for writing, set all necessary info */
register char *namep;
{
#if defined(SecureWare) && defined(B1)
	int f;
#else
	register f;
#endif /* SecureWare && B1 */
	register char *np;
	int ans;
	long    filesz;
#if defined(DUX) || defined(DISKLESS)
        char *p;
#endif /* defined(DUX) || defined(DISKLESS) */
#ifdef QFLAG
	char *last_slash;
	char namep_old[MAXPATHLEN];
	int  slash = '/';

	/*
	 * initialize "target file on remote file system" flag
	 */
	went_remote = FALSE;
#endif /* QFLAG */

	if (!strncmp(namep, ".//", 3))
		namep += 3;		/*  6/29/82 Fix -p of / to . */
	if (!strncmp(namep, "./", 2))
		namep += 2;
	np = namep;

#ifdef CODE_DELETED
	if (Option == IN)
		Cd_name = namep = cd(namep);
#endif /* CODE_DELETED */

#ifdef OLD_RFA
	/*
	 * Network special files are obsolete and this is a network
	 * special file, print a warning about this file and skip it.
	 */
	if ((Hdr.h_mode & Filetype) == S_IFNWK) {
	    fprintf(stderr,
		catgets(catd,NL_SETN,113, "Skipped obsolete network special file <%s>\n"), namep);
	    return 0;
	}
#endif /* OLD_RFA */

	if (A_directory) {
		if (!Dir
		|| Rename
#if defined(DUX) || defined(DISKLESS)
		|| EQ(namep, "..+")
		|| EQ(namep, ".+")
#endif /* defined(DUX) || defined(DISKLESS) */
		|| EQ(namep, ".")
		|| EQ(namep, ".."))	/* do not consider . or .. files */
			return 0;


		/* Use stat(2) call here, not lstat(2). */
                /* if target directory doesn't exist, create it */
		if (stat(namep, &Xstatb) == -1) {

/* try creating (only twice) */
			ans = 0;
			do {
#if defined(DUX) || defined(DISKLESS)
                                if (bit[cnt] && *(namep + strlen(namep) -1)=='+') {
   				       p=namep+strlen(namep)-1;
                                       *p=0;
				}
#endif /* defined(DUX) || defined(DISKLESS) */
#if defined(SecureWare) && defined(B1)
#ifdef MLTAPE
				f = ie_ml_import(namep,0777,1,(int)Hdr.h_Mld, 0);
				if (f < 0) {
					ans = 3;
					break;
				}
				if (!f)
#else
				if (ie_openout(namep, 0777, 1, 0, 0) < 0)
#endif
#else
				if (makdir(namep) != 0) 
#endif

				{
				    ans += 1;

#ifdef QFLAG
				    /*
				     * if we failed because of
				     * read-only FS, likely it's NFS
				     * remote
				     */
				    if (Qflag && errno == EROFS) {
					went_remote = TRUE;
					return(0);
				    }
#endif /* QFLAG */
#if defined(DUX) || defined(DISKLESS)
				    if (bit[cnt])
					strcat(namep,"+");
#endif /* defined(DUX) || defined(DISKLESS) */
				}else {
				    ans = 0;
				    /* Use stat(2) call here, not lstat(2). */
				    /* ignore result */
				    (void) stat(namep,&Xstatb);

#ifdef QFLAG
				    if (Qflag && ((Xstatb.st_dev & NFS_remote) == NFS_remote)) {
					unlink(namep); /* If we made a remote directory, undo it */
					went_remote = TRUE;
					return(0);
				    }
#endif /* QFLAG */
#if defined(DUX) || defined(DISKLESS)
				    if (bit[cnt]) {
#if defined(SecureWare) && defined(B1)
#ifdef MLTAPE
					ie_chmod(namep,Xstatb.st_mode | 04000, A_directory);
#else
					ie_chmod(namep,Xstatb.st_mode | 04000, 0);
#endif
#else
					chmod(namep,Xstatb.st_mode | 04000);
#endif
					strcat(namep,"+");
				    }
#endif /* defined(DUX) || defined(DISKLESS) */

				    break;
				}
			}while (ans < 2 && missdir(namep) == 0);
#ifdef QFLAG
                        /*
			 * if missdir made a remote directory, it sets
			 * went_remote, so check for it
			 */
			if (Qflag && went_remote)
				return(0);
#endif /* QFLAG */
			if (ans == 1) {
				fprintf(stderr,catgets(catd,NL_SETN,43, "Cannot create directory for <%s> (errno:%d)\n"), namep, errno);
				bad_try = 1;
				return(0);
			}else if (ans == 2) {
				fprintf(stderr,catgets(catd,NL_SETN,44, "Cannot create directory <%s> (errno:%d)\n"), namep, errno);
				bad_try = 1;
				return(0);
			}
		}
#ifdef QFLAG
		/*
		 * if stat (way above) succeeded, or if making the
		 * directory remotely somehow succeeded, check one more
		 * time for remote directory and set went_remote if so
		 */
		if (Qflag && ((Xstatb.st_dev & NFS_remote) == NFS_remote)) {
			went_remote = TRUE;
			return(0);
		}
#endif /* QFLAG */

#if defined(SecureWare) && defined(B1)
#ifdef MLTAPE
		if(Option==PASS)
		   ie_copyattr(Hdr.h_name,namep);
		if(Hdr.h_Mld)
		      enablepriv(SEC_MULTILEVELDIR);
		if (ie_chmod(namep, Hdr.h_mode&~usrmask, A_directory) < 0) {
#else
		if (ie_chmod(namep, Hdr.h_mode&~usrmask, 0) < 0) {
#endif
#else
		if (chmod(namep, Hdr.h_mode&~usrmask) < 0) {
#endif
			fprintf(stderr,catgets(catd,NL_SETN,45, "Cannot chmod <%s> (errno:%d)\n"), namep, errno);
		}
#if defined(SecureWare) && defined(B1)
#ifndef MLTAPE
		if (ISB1 && !ie_sl_set_attributes(namep)) {
			unlink(namep);
			return(0);
		}
#endif
		if(((!ISB1 && Uid==0) || (ISB1 && hassysauth(SEC_OWNER))) && !L_special )
#else
		if (Uid == 0 && !L_special)
#endif
		/* !L_special besause we can NEVER change ownership of symbolic link itself */

			if (chown(namep, Hdr.h_uid, Hdr.h_gid) < 0) {
				fprintf(stderr,catgets(catd,NL_SETN,46, "Cannot chown <%s> (errno:%d)\n"), namep, errno);
			}
		set_time(namep, mklong(Hdr.h_mtime), mklong(Hdr.h_mtime));
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
		if(Hdr.h_Mld)
		   disablepriv(SEC_MULTILEVELDIR);
#endif
		return 0;
	} /* End of directory handler. */

	if (Hdr.h_nlink > 1)
		if (!postml(namep, np))
			return 0;
		
	/* Changed from stat to lstat to handle symlinks times*/
	  if (lstat(namep, &Xstatb) == 0) {
#ifdef QFLAG
		/*
		 * Now handling files, if target exists and is remote,
		 * set flag and get out
		 */
		if (Qflag && ((Xstatb.st_dev & NFS_remote) == NFS_remote)) {
			went_remote = TRUE;
			return(0);
		}
#endif /* QFLAG */
		if (Uncond &&
#if defined(RFA)
		   !(((Xstatb.st_mode & S_IWRITE) || A_special || N_special) && (Uid != 0)) &&
#else
#if defined(SecureWare) && defined(B1)
		   !(((Xstatb.st_mode & S_IWRITE) || A_special) && (!ISB1&&(Uid!=0))||(ISB1&&hassysauth(SEC_OWNER))) &&
#else
		   !(((Xstatb.st_mode & S_IWRITE) || A_special) && (Uid != 0)) &&
#endif
#endif /* RFA */
		   (Xstatb.st_mode & S_IFMT) != S_IFDIR
#if defined(RFA)
		   && (!(A_special || N_special) || Xflag))
#else
		   && (!A_special || Xflag))
#endif /* RFA */
	       {
			if (unlink(namep) < 0) {
#ifdef QFLAG
				/*
				 * wild hair  -- move file to #file if
				 * it's busy
				 */
				if (Qflag) {
				    if (errno == ETXTBSY) {
					last_slash = strrchr(namep,slash);
					/* If no slash found, cause # to precede the file name */
					if (last_slash == NULL)
						last_slash = namep - 1;
					strncpy(namep_old,namep,(last_slash - namep + 1));
					/* put a trailing null after what strncpy wrote */
					namep_old[last_slash-namep+1] = (char)0;
					strcat(namep_old,"#");
					strcat(namep_old,(last_slash + 1));
					unlink(namep_old);
					link(namep,namep_old);
					if (unlink(namep) < 0)
						fprintf(stderr,catgets(catd,NL_SETN,47, "cannot unlink current <%s> (errno:%d)\n"), namep, errno);
				    } else {
					fprintf(stderr,catgets(catd,NL_SETN,47, "cannot unlink current <%s> (errno:%d)\n"), namep, errno);
				    } /* if errno...else... */
				} else {
#endif /* QFLAG */
				    fprintf(stderr,catgets(catd,NL_SETN,47, "cannot unlink current <%s> (errno:%d)\n"), namep, errno);
#ifdef QFLAG
				} /* if Qflag...else... */
#endif /* QFLAG */
			} /* if unlink... */
		} /* if Uncond ... */
		if (!Uncond && (mklong(Hdr.h_mtime) <= Xstatb.st_mtime)) {
		/* There's a newer version of file on destination */
			if (mklong(Hdr.h_mtime) <= Xstatb.st_mtime)
				fprintf(stderr,catgets(catd,NL_SETN,48, "current <%s> newer\n"), np);
			return 0;
		}
		if (!Uncond && (Xstatb.st_mode & S_IFMT) != S_IFDIR &&
#if defined(RFA)
		    (!(A_special || N_special) || Xflag) && Uid == 0)
#else
#if defined(SecureWare) && defined(B1)
		    (!A_special || Xflag) &&
			(ISB1 && hassysauth(SEC_MKNOD) || !ISB1 && (Uid == 0)))
#else
		    (!A_special || Xflag) && Uid == 0)
#endif
#endif /* RFA */
		{
		/* There's an older version of special file on destination */
			if (unlink(namep) < 0)
				    fprintf(stderr,catgets(catd,NL_SETN,47, "cannot unlink current <%s> (errno:%d)\n"), namep, errno);
		}
	}

	if (Option == PASS &&
	   Lhdr.l_ino == Xstatb.st_ino &&
#if defined(RFA)
	   Lhdr.l_dev == Xstatb.st_dev && Remote == 0)
#else
	   Lhdr.l_dev == Xstatb.st_dev)
#endif /* RFA */
	{
		fprintf(stderr,catgets(catd,NL_SETN,49, "Attempt to pass file to self!\n"));
		exit(2);
	}

#if defined(RFA)
	if (A_special || N_special)
#else
	if (A_special)
#endif /* RFA */
	{
		if (!Xflag)
			return(0);
		if (Alien) {
			fprintf(stderr,catgets(catd,NL_SETN,50, "Cannot mknod <%s> (errno:%d)\n"), namep, errno);
			return(0);
		}

#if defined(RFA)
		if ((Hdr.h_mode & Filetype) == S_IFIFO ||
		   (Hdr.h_mode & Filetype) == S_IFNWK)
#else
		if ((Hdr.h_mode & Filetype) == S_IFIFO)
#endif /* RFA */
			Lhdr.l_rdev = 0;
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
		else if (!ie_can_make_node()) return(0);
#endif

/* try creating (only twice) */
		ans = 0;

#if defined(SecureWare) && defined(B1)
		if (ISB1)
			enablepriv(SEC_MKNOD);
#endif /* SecureWare */

		do {
#ifdef CNODE_DEV
                 if (Option == PASS)  /* XXX */
                   if (Hdr.h_rdev >= 0 && Hdr.h_rdev <= 255)
                        Lhdr.l_name[MAXPATHLEN-1] = Hdr.h_rdev;

                 if (Lhdr.l_name[MAXPATHLEN-1] >= 0 && Lhdr.l_name[MAXPATHLEN-1] <= 255) {
#if defined(SecureWare) && defined(B1)
#ifdef MLTAPE
			f = ie_ml_import(namep,Hdr.h_mode,2,Lhdr.l_rdev, Lhdr.l_name[MAXPATHLEN-1]);
			if (f < 0) {
				ans = 3;
				break;
			}
			if (!f)  {
#else
			if (ie_openout(namep, Hdr.h_mode, 2, Lhdr.l_rdev, Lhdr.l_name[MAXPATHLEN-1]) < 0) {
#endif
#else
                    if (mkrnod(namep,Hdr.h_mode,Lhdr.l_rdev,Lhdr.l_name[MAXPATHLEN-1]) < 0) {
#endif
                                ans += 1;
                        } else {
                                ans = 0;
                                break;
                        }
                  }
                  else {
#if defined(SecureWare) && defined(B1)
#ifdef MLTAPE
			f = ie_ml_import(namep,Hdr.h_mode,2,Lhdr.l_rdev, 0);
			if (f < 0) {
				ans = 3;
				break;
			}
			if (!f)  {
#else
			if (ie_openout(namep, Hdr.h_mode, 2, Lhdr.l_rdev, 0) < 0) {
#endif
#else
			if (mknod(namep, Hdr.h_mode, Lhdr.l_rdev) < 0) {
#endif
				ans += 1;
			}
			else {
				ans = 0;
				break;
			}
                  } /* End if-then-else */
#else
#if defined(SecureWare) && defined(B1)
#ifdef MLTAPE
			f = ie_ml_import(namep,Hdr.h_mode,2,Lhr.l_rdev, 0);
			if (f < 0) {
				ans = 3;
				break;
			}
			if (!f)
#else
			if (ie_openout(namep, Hdr.h_mode, 2, Lhdr.l_rdev, 0) < 0)
#endif
#else
                        if (mknod(namep, Hdr.h_mode, Lhdr.l_rdev) < 0)
#endif
			{
                                ans += 1;
                        }
			else {
                                ans = 0;
                                break;
                        }
#endif /* CNODE_DEV */
		} while (ans < 2 && missdir(np) == 0);

#if defined(SecureWare) && defined(B1)
		if (ISB1)
			disablepriv(SEC_MKNOD);
#endif /* SecureWare */

		if (ans == 1) {
			fprintf(stderr,catgets(catd,NL_SETN,51, "Cannot create directory for <%s> (errno:%d)\n"), namep, errno);
			bad_try = 1;
			return(0);
		}else if (ans == 2) {
			fprintf(stderr,catgets(catd,NL_SETN,52, "Cannot mknod <%s> (errno:%d)\n"), namep, errno);
			bad_try = 1;
			return(0);
		}

		/* Note: do same thing s5.2 does, but add network */
		/*	 special file handling. (3/18/85, pvs)	*/
#if defined(SecureWare) && defined(B1)
#ifdef MLTAPE
		if(!A_directory && Option==PASS)
		   ie_copyattr(Hdr.h_name,namep);
		if (ie_chmod(namep, Hdr.h_mode&~usrmask, A_directory) < 0) {
#else
		if (ie_chmod(namep, Hdr.h_mode&~usrmask, 0) < 0) {
#endif
#else
		if (chmod(namep, Hdr.h_mode&~usrmask) < 0) {
#endif
			fprintf(stderr,catgets(catd,NL_SETN,53, "Cannot chmod <%s> (errno:%d)\n"), namep, errno);
		}
#if defined(SecureWare) && defined(B1)
		if ((ISB1 && hassysauth(SEC_OWNER) || !ISB1 && (Uid == 0)) && !L_special )
#else
		if (Uid == 0 && !L_special )
#endif
			if (chown(namep, Hdr.h_uid, Hdr.h_gid) < 0) {
				fprintf(stderr,catgets(catd,NL_SETN,54, "Cannot chown <%s> (errno:%d)\n"), namep, errno);
		}
		set_time(namep, mklong(Hdr.h_mtime), mklong(Hdr.h_mtime));
#if defined(RFA)
		if (N_special) {
			if ((f = open(namep, 1)) < 0) {
				fprintf(stderr,catgets(catd,NL_SETN,55, "Cannot open <%s> (errno:%d)\n"),
				    namep, errno);
				bad_try = 1;
			} else {
				return f;
			}
		}
#endif /* RFA */
		return 0;
	}

#ifdef RT
	if (One_extent || Multi_extent) {

/* try creating (only twice) */
		ans = 0;
		do {
			if ((f = falloc(namep, Hdr.h_mode, longword(Hdr.h_filesize[0]))) < 0) {
				ans += 1;
			}else {
				ans = 0;
				break;
			}
		}while (ans < 2 && missdir(np) == 0);
		if (ans == 1) {
			fprintf(stderr,catgets(catd,NL_SETN,56, "Cannot create directory for <%s> (errno:%d)\n"), namep, errno);
			bad_try = 1;
			return(0);
		}else if (ans == 2) {
			fprintf(stderr,catgets(catd,NL_SETN,57, "Cannot create <%s> (errno:%d)\n"), namep, errno);
			bad_try = 1;
			return(0);
		}

		if (filespace < longword(Hdr.h_filesize[0])) {
			fprintf(stderr,catgets(catd,NL_SETN,58, "Cannot create contiguous file <%s> proper size\n"), namep);
			fprintf(stderr,catgets(catd,NL_SETN,59, "    <%s> will be created as a regular file\n"), namep);
			if (unlink(Fullname) != 0)
				fprintf(stderr,catgets(catd,NL_SETN,60, "<%s> not removed\n"), namep);
			Hdr.h_mode = (Hdr.h_mode & !S_IFMT) | S_IFREG;
			One_extent = Multi_extent = 0;
		}
	Hdr.h_filesize[0] = Actual_size[0];
	Hdr.h_filesize[1] = Actual_size[1];
	}
#endif /* RT */

#ifdef SYMLINKS
	if (L_special) { /* Symbolic link handler. */
		filesz=mklong(Hdr.h_filesize);

		switch (Option) {
		  case OUT:
		    break;

		  case IN:
		    Cflag? readhdr(BBuf,filesz): bread(Buf,filesz);
		    if (Swap)
			Cflag? swap(BBuf,filesz): swap(Buf,filesz);
		    break;

		  case PASS:
		    if (readlink(Hdr.h_name, Cflag? BBuf: (char *)Buf, CPIOBSZ) < 0) {
			fprintf(stderr,catgets(catd,NL_SETN,10, "Cannot read %s\n"), Hdr.h_name);
			bad_try = 1;
			return 0;
		    }
		    break;

		  default:
		    break;
		}


		/* symlink() needs null terminated string */
		if (Cflag) {
			BBuf[filesz] = '\0';
		} else {
			*((char *)Buf + filesz) = '\0';
		}

/* try creating (only twice) */
		ans = 0;
		do {
			if (symlink(Cflag? BBuf:(char *)Buf, namep) < 0) {
				ans += 1;
                                /*
                                 * If symlink failed because of
                                 * existing link
				 */
				if ( (errno == EEXIST) && (lstat(namep, &Xstatb) == 0) ) {
                                      if (S_ISLNK(Xstatb.st_mode)) {
				       /*
					* If we have unconditional restoring, let's
					* erase existing symbolic link ...
					*/
					if ( Uncond )
					 {
					  unlink( namep );
					  continue;
					 }
                                        ans = 0;
                                        break;
                                      }
                                }
#ifdef QFLAG
			        /*
				 * If symlink failed because of
				 * read-only file system, then
			         * assume it's remote via NFS, set
				 * flag, and get out
				 */
			        if (Qflag && (errno == EROFS)) {
				        went_remote = TRUE;
				        return(0);
			        }
#endif /* QFLAG */
			} else {
				ans = 0;
#ifdef QFLAG
				if (Qflag) {
			    	/*
				 * if symlink succeeded, stat the file.
				 * If it's remote then remove it, set
				 * the flag, and get out use lstat
				 * instead of stat, to inspect the
				 * symlink itself
				 */
			    	    if (lstat(namep, &Xstatb) == 0) {
					if ((Xstatb.st_dev & NFS_remote) == NFS_remote) {
						unlink(namep);
						went_remote = TRUE;
						return(0);
					}
			    	    }
				}
#endif /* QFLAG */
				break;
			}
		} while (ans < 2 && missdir(np) == 0);
#ifdef QFLAG
	        /*
		 * If missdir (really makdir, called from missdir)
		 * tried to make a remote directory, it set the flag.
		 * Check it, and get out if it's set
		 */
		if (Qflag && went_remote)
			return(0);
#endif /* QFLAG */
		if (ans == 1) {
			fprintf(stderr,catgets(catd,NL_SETN,61, "Cannot create link for <%s> (errno:%d)\n"), namep, errno);
			bad_try = 1;
			return 0;
		} else if (ans == 2) {
			fprintf(stderr,catgets(catd,NL_SETN,62, "Cannot create link <%s> (errno:%d)\n"), namep, errno);
			bad_try = 1;
			return 0;
		}

#if defined(SecureWare) && defined(B1)
		if ((ISB1 && hassysauth(SEC_OWNER) || !ISB1 && (Uid == 0)) && !L_special )
#else
		if (Uid == 0 && !L_special )
#endif
			if (chown(namep, Hdr.h_uid, Hdr.h_gid) < 0) {
				fprintf(stderr,catgets(catd,NL_SETN,64, "Cannot chown <%s> (errno:%d)\n"), namep, errno);
			}

		/* Note: chmod(2) and utime(2) make no sense on a   */
		/*       symbolic link because they operate on the  */
		/*       file the link points to instead of the     */
		/*       symbolic link itself.                      */

		return 0;
	} /* End of symbolic link handler. */
#endif /* SYMLINKS */

#ifdef RT
	if (!(One_extent || Multi_extent)) {
#endif /* RT */

/* try creating (only twice) */
	ans = 0;
#ifdef QFLAG
	if (Qflag) {
	    /*
	     * Finally creating target file, attempt to stat.  If it
	     * exists and is remote, then set flag and get out
	     */
	    if (stat(namep, &Xstatb) == 0) {
		if ((Xstatb.st_dev & NFS_remote) == NFS_remote) {
			went_remote = TRUE;
			return(0);
		}
	    }
	}
#endif /* QFLAG */

	do {
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
                if((f = ie_ml_import(namep,Hdr.h_mode,0,0, 0)) < 0) {
			ans = 3;
			break;
		}
		if (f<0) {
#else
		if ((f = creat(namep, ~usrmask&0777)) < 0) {
#endif
			ans += 1;
#ifdef QFLAG
			/*
			 * If create failed because of read-only file
			 * system, then assume it's remote via NFS,
			 * set flag, and get out
			 */
			if (Qflag && (errno == EROFS)) {
				went_remote = TRUE;
				return(0);
			}
#endif /* QFLAG */
		}else {
			ans = 0;
#ifdef QFLAG
			if (Qflag) {
			    /*
			     * if create succeeded, stat the file.  If
			     * it's remote then close and remove it,
			     * set the flag, and get out
			     */
			    if (stat(namep, &Xstatb) == 0) {
				if ((Xstatb.st_dev & NFS_remote) == NFS_remote) {
					close(f);
					unlink(namep);
					went_remote = TRUE;
					return(0);
				}
			    }
			}
#endif /* QFLAG */
			break;
		}
	}while (ans < 2 && missdir(np) == 0);
#ifdef QFLAG
	/*
	 * If missdir (really makdir, called from missdir) tried to
	 * make a remote directory, it set the flag.  Check it, and
	 * get out if it's set
	 */
	if (Qflag && went_remote)
		return(0);
#endif /* QFLAG */
	if (ans == 1) {
		fprintf(stderr,catgets(catd,NL_SETN,65, "Cannot create directory for <%s> (errno:%d)\n"), namep, errno);
		bad_try = 1;
		return(0);
	}else if (ans == 2) {
		fprintf(stderr,catgets(catd,NL_SETN,66, "Cannot create <%s> (errno:%d)\n"), namep, errno);
		bad_try = 1;
		return(0);
	}

#ifdef RT
	}
#endif /* RT */
#if defined(SecureWare) && defined(B1)
#ifndef MLTAPE
	if (ISB1 && !ie_sl_set_attributes(namep)) {
		unlink(namep);
		return(0);
	}
#endif

	if(hassysauth(SEC_OWNER))
#else
	if (Uid == 0 && !L_special)
#endif
		chown(namep, Hdr.h_uid, Hdr.h_gid);
	return f;
} /* openout */

/*								    */
/* read binary header from buffer (if not empty), else from archive */
/* 								    */
/*  History: 
/*	May 29, 1992	jlee	Change word counter (nleft) to be   */
/*				incremented outside of while loop.  */
/*								    */
/*				Added debugging statements to 	    */
/*				print Dbuf and associated counters  */
/*								    */
bread(b, c)
register short *b;			/* buffer pointer */
register c;				/* size in bytes */
{
	static nleft = 0;	/* words left in buffer Dbuf */
	static short *ip;
	register int rv;
	register short *p = ip;
	register int in;

#ifdef DEBUG1
	int 	doforever;

	doforever=TRUE;
	fprintf(dbg_stream,"*******************************************************\n");
#endif

#ifdef DEBUG1

	fprintf(dbg_stream,"Entering Bread -- Current block # %d,\n",Blocks);
	fprintf(dbg_stream,"	              nleft = %d ,   ip (offset)= %d\n",nleft,(ip-&Dbuf[0]) );  
	fprintf(dbg_stream,"                  Requested number of bytes = %d\n",c);
	if ( c == sizeof(Hdr.h_magic)) {
		fprintf(dbg_stream,"Looking for a header.  Bufsize = %d\n",Bufsize); 
#ifdef DEBUG2
		fprintf(stderr,"\n Looking for magic value Startup up XDB\n")	;
		while(doforever);
#endif
		fprintf(dbg_stream,"nleft = %d,    ip = %d\n",
			   nleft, (ip-&Dbuf[0]));
		fprintf(dbg_stream,"        (0)1  (1) 2  (2) 3  (3) 4  (4) 5  (5) 6  (6) 7  (7) 8  (8) 9  (9) 0\n");
		fprintf(dbg_stream,"---------------------------------------------------------------------------\n");
		fprintf(dbg_stream,"Dbuf=[");
		for(doforever=0;doforever!=Bufsize/2;doforever++) {
			fprintf(dbg_stream,"%6x ",(unsigned short )Dbuf[doforever]);
			if ( (doforever+1) % 10 == 0 ) fprintf(dbg_stream,"\n  %2d |",(doforever/10)+1);
		}
		fprintf(dbg_stream,"]\n-------------------------------------------------------\n");

	}
#endif

	c = (c+1)>>1;		/* word count from byte count */

	while (c--) {
		if (nleft == 0) {
			in = 0;

#ifdef DEBUG1	
			fprintf(dbg_stream,"Entering while loop: Starting Block #%d\n",Blocks);
#endif
			while ((rv=read(Input, &(((char *)Dbuf)[in]), Bufsize - in)) != Bufsize - in) {
#ifdef DEBUG1 	
				fprintf(dbg_stream,"Read Partial Block, Block being read is %d, Bytes read = %d/%d (%d) \n",Blocks+1,rv,in,Bufsize);
#endif
				if (rv == 0 && !had_first_read) {
					had_first_read = TRUE;
					continue;
				}
				if (rv <= 0) {
					Input = chgreel(0, Input);
					continue;
				}
				in += rv;
		/*		nleft += (rv >> 1); */
			}
#ifdef DEBUG1 	
				fprintf(dbg_stream,"Completed Reading of block: Block # read is %d, Bytes read = %d/%d (%d) \n",Blocks+1,rv,in,Bufsize);
#endif
			in += rv;
			nleft += (in >> 1);
			p = Dbuf;
			++Blocks;
		}

		*b++ = *p++;
		--nleft;
	}
	ip = p;
#ifdef DEBUG
	fprintf(dbg_stream,"Leaving Bread.  Static values are:\n          nleft = %d\n          ip (offset) = %d\n",
		nleft,(ip-&Dbuf[0]));
#endif	
}

/* read ASCII header from buffer (if not empty), else from archive */
readhdr(b, c)
register char *b;		/* buffer pointer */
register c;			/* size in bytes */
{
	static nleft = 0;	/* bytes left in buffer Cbuf */
	static char *ip;
	register int rv;
	register char *p = ip;
	register int in;

	while (c--) {
		if (nleft == 0) {
			in = 0;
			while ((rv=read(Input, &(((char *)Cbuf)[in]), Bufsize - in)) != Bufsize - in) {
				if (rv == 0 && !had_first_read) {
					had_first_read = TRUE;
					continue;
				}
				if (rv <= 0) {
					Input = chgreel(0, Input);
					continue;
				}
				in += rv;
				nleft += rv;
			}
			nleft += rv;
			p = Cbuf;
			++Blocks;
		}
		*b++ = *p++;
		--nleft;
	}
	ip = p;
}


/* writeheader - front end to "bwrite" and "writehdr" for
 *               writing headers.
 */

writeheader(state)
int state;
{
    switch (state) {
	case NORMAL:
	    if (Cflag)
		writehdr(Chdr,CHARS+Hdr.h_namesize, HEADER);
	    else
		bwrite(&Hdr, HDRSIZE+Hdr.h_namesize, HEADER);
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
                writehdr(ie_findbuff(),Hdr.h_Secsize,HEADER);
#endif
	    break;

	case FIRSTPART:
	    if (FirstfilePart == HEADER) {
#ifdef DEBUG
fprintf(stderr, "Write header of <%s> starting at offset %d\n", Hdr.h_name, StartOffset);
#endif /* DEBUG */
		if (Cflag)
		    writehdr(Chdr+StartOffset,CHARS+Hdr.h_namesize-StartOffset, HEADER);
		else
		    bwrite(((char *)&Hdr)+StartOffset, HDRSIZE+Hdr.h_namesize-StartOffset, HEADER);
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
                writehdr(ie_findbuff(),Hdr.h_Secsize,HEADER);
#endif
		/* Reset write state for next, normal, write. */
		if (writestate == FIRSTPART)
		    writestate = NORMAL;
	    }
	    return;

	case CHECKPT:
	    fprintf(stderr, catgets(catd,NL_SETN,102, "unexpected write state: CHECKPT\n"));
	    exit(CHKPTERR);
	    break;

	default:
	    fprintf(stderr, catgets(catd,NL_SETN,103, "Unknown write state (%d).\n"), state);
	    exit(CHKPTERR);
	    break;
    }

    return;
}


/* write binary header to buffer (if not full), else to archive */
bwrite(rp, c, part)
register short *rp;		/* buffer pointer */
register c;			/* size in bytes */
int part;                       /* Part of file being written (HEADER,BODY). */
{
	register short *wp = Wp;
	register int wv;	/* number of bytes written by write(2) */
	register int out;	/* number of bytes written sofar */
	static int bdycnt;      /* Bytes of body written for current file. */
	static int hdrcnt;      /* Bytes of header written for current file. */
	int ccnt;

	/* Reset "hdrcnt" or "bdycnt" for file. */
	if (part == HEADER)
	    bdycnt = 0;
	else
	    hdrcnt = 0;

	c = (c+1) >> 1;		/* word count from byte count */
	while (c != 0) {
		if (Wct == 0) {
			out = 0;
again:
			wv = write(Output, &(((char *)Dbuf)[out]), Bufsize - out);
                        if (wv != Bufsize - out)  {

			    /* Rewriting tape? */
			    if (mustrewrite(wv) == TRUE) {

				/* New write state. */
				writestate = CHECKPT;

				/* Restore buffer properly. */
				memcpy((void *)Dbuf, (void *)Dsave, Nbytessaved);
				Wct = (Bufsize-Nbytessaved) >> 1;
				Wp  = Dbuf + (Nbytessaved>>1);

				/* Restore "bwrite" state properly. */
				if (FirstfilePart == BODY)
				    bdycnt = StartOffset;
				else
				    hdrcnt = StartOffset;

				/* Adjust block count. */
				Blocks -= ReelBlocks;

				/* Get new tape. */
                                Output = chgreel(1, Output);
				return;
			    }

			    /* Another successfully written reel. */
			    ReelNo++;

			    if (Chkpt) {
#ifdef DEBUG
fprintf(stderr, "Save starting state:\n");
#endif /* DEBUG */
				/* Save starting state of next reel. */
				switch (part) {
				    case HEADER:
#ifdef DEBUG
fprintf(stderr, "\theader offset: %d\n", hdrcnt);
#endif /* DEBUG */
					StartOffset = hdrcnt;
					break;

				    case BODY:
#ifdef DEBUG
fprintf(stderr, "\tbody offset: %d\n", bdycnt);
#endif /* DEBUG */
					StartOffset = bdycnt;
					break;

				    default:
					fprintf(stderr, catgets(catd,NL_SETN,104, "Unknown part (%d).\n"), part);
					exit(CHKPTERR);
					break;
				}
				FirstfilePart = part;

				SaveStatb = Statb;  /* Save stat info. */

				/* Save "unwritten" part of buffer. */
				Nbytessaved = Bufsize - out - (wv>0?wv:0);
				memcpy((void *)Dsave,
				       (void *)(Dbuf+out+(wv>0?wv:0)),
				       Nbytessaved);
#ifdef DEBUG
fprintf(stderr, "\tNbytessaved: %d\n\n", Nbytessaved);
#endif /* DEBUG */
			    }
			    Output = chgreel(1, Output);
			    /* update "out" only if something was written */
			    if (wv > 0) out += wv;
			    goto again;
                        }
			Wct = Bufsize >> 1;
			wp = Dbuf;
			++Blocks;
			++ReelBlocks;   /* Gets properly reset in "chgreel". */
		}
		/* use "memcpy"; this gives a significant
		 * performance enhancement!!
		 */
		if (c >= Wct) {
		    memcpy((void *)wp, (void *)rp, Wct << 1);
		    ccnt = Wct << 1;
		    wp  += Wct;
		    rp  += Wct;
		    c   -= Wct;
		    Wct = 0;
		} else {
		    memcpy((void *)wp, (void *)rp, c << 1);
		    ccnt = c << 1;
		    wp  += c;
		    rp  += c;
		    Wct -= c;
		    c = 0;
		}
		if (part == BODY)
		    bdycnt += ccnt;
		else
		    hdrcnt += ccnt;
	}
	Wp = wp;
}

/* write ASCII header to buffer (if not full), else to archive */
writehdr(rp, c, part)
register char *rp;		/* buffer pointer */
register c;			/* size in bytes */
int part;                       /* Part of file being written (HEADER,BODY). */
{
	register char *cp = Cp;
	register int wv;	/* number of bytes written by write(2) */
	register int out;	/* number of bytes written sofar */
	static int bdycnt;      /* Bytes of body written for current file. */
	static int hdrcnt;      /* Bytes of header written for current file. */
	int ccnt;

	/* Reset "hdrcnt" or "bdycnt" for file. */
	if (part == HEADER)
	    bdycnt = 0;
	else
	    hdrcnt = 0;

	while (c != 0) {
		 if (Wc == 0) {
			out = 0;
again:
			wv = write(Output, &(((char *)Cbuf)[out]), Bufsize - out);
                        if (wv != Bufsize - out)  {

			    /* Rewriting tape? */
			    if (mustrewrite(wv) == TRUE) {

				/* New write state. */
				writestate = CHECKPT;

				/* Restore buffer properly. */
				memcpy((void *)Cbuf, (void *)Csave, Nbytessaved);
				Wc = Bufsize - Nbytessaved;
				Cp = Cbuf + Nbytessaved;

				/* Restore "bwrite" state properly. */
				if (FirstfilePart == BODY)
				    bdycnt = StartOffset;
				else
				    hdrcnt = StartOffset;

				/* Adjust block count. */
				Blocks -= ReelBlocks;

				/* Get new tape. */
				Output = chgreel(1, Output);
				return;
			    }

			    /* Another successfully written reel. */
			    ReelNo++;

			    if (Chkpt) {
#ifdef DEBUG
fprintf(stderr, "Save starting state:\n");
#endif /* DEBUG */
				/* Save starting state of next reel. */
				switch (part) {
				    case HEADER:
#ifdef DEBUG
fprintf(stderr, "\theader offset: %d\n", hdrcnt);
#endif /* DEBUG */
					StartOffset = hdrcnt;
					break;

				    case BODY:
#ifdef DEBUG
fprintf(stderr, "\tbody offset: %d\n", bdycnt);
#endif /* DEBUG */
					StartOffset = bdycnt;
					break;

				    default: /* This should never happen. */
					fprintf(stderr, catgets(catd,NL_SETN,104, "Unknown part (%d).\n"), part);
					exit(CHKPTERR);
					break;
				}
				FirstfilePart = part;

				SaveStatb = Statb;  /* Save stat info. */

				/* Save "unwritten" part of buffer. */
				Nbytessaved = Bufsize - out - (wv>0?wv:0);
				memcpy((void *)Csave,
				       (void *)(Cbuf+out+(wv>0?wv:0)),
				       Nbytessaved);
#ifdef DEBUG
fprintf(stderr, "\tNbytessaved: %d\n\n", Nbytessaved);
#endif /* DEBUG */
			    }
			    Output = chgreel(1, Output);
			    /* update "out" only if something was written */
			    if (wv > 0) out += wv;
			    goto again;
                        }
                        Wc = Bufsize;
                        cp = Cbuf;
                        ++Blocks;
			++ReelBlocks;   /* Gets properly reset in "chgreel". */
                 }
		/* use "memcpy"; this gives a significant
		 * performance enhancement!!
		 */
		if (c >= Wc) {
		    memcpy((void *)cp, (void *)rp, Wc);
		    ccnt = Wc;
		    cp += Wc;
		    rp += Wc;
		    c  -= Wc;
		    Wc = 0;
		} else {
		    memcpy((void *)cp, (void *)rp, c);
		    ccnt = c;
		    cp += c;
		    rp += c;
		    Wc -= c;
		    c  = 0;
		}
		if (part == BODY)
		    bdycnt += ccnt;
		else
		    hdrcnt += ccnt;
         }
         Cp = cp;
}


/* mustrewrite - return TRUE if we got a "write error" from
 *               a streaming 9-track tape drive (7974/78) with immediate
 *               report mode on and the environment parameters
 *               ("Chkpt", "isstream", user) indicate that we can rewrite
 *               the current tape.
 *
 *               Otherwise return FALSE.
 */

mustrewrite(retv)
    int retv;   /* Return value from write(2) call. */
{
    int save_error; /* Save value of "errno" or "errinfo" (s500 only). */
    char str[22];
    FILE *devtty;

    if (retv < 0 && errno != ENOSPC) {
	save_error = errno;
	perror("write failed");

	/* open dev/tty, or die with message to stderr */
	devtty = fopen("/dev/tty", "r");
	devttyout = fopen("/dev/tty", "w");

	if (devtty == (FILE *)0 || devttyout == (FILE *)0) {
	    fprintf(stderr,catgets(catd,NL_SETN,105, "Can't open /dev/tty.\n"));
	    exit(2);
	}
	derr("Unexpected write error (errno: %d).\n", save_error);
	if (Chkpt) {
	    if (isstream == FALSE) {
		derr("%ld blocks were written to this volume (#%d).\n", ReelBlocks * (Bufsize>>9), ReelNo);
		do {
		    derr("Do you want to [r]ewrite this volume or [i]gnore error? (r/i) \n");
		    fgets(str, 20, devtty);
		} while (*str != 'r' && *str != 'R' &&
			 *str != 'i' && *str != 'I');

		switch (*str) {
		    case 'r':   /* Rewrite this volume. */
		    case 'R':
			break;  /* Fall through to rewrite reel. */

		    case 'i':
		    case 'I':
			derr("Ignoring this write error.\n");
			fclose(devtty);
			fclose(devttyout);
			return FALSE;   /* Do not rewrite. */

		    default:    /* This should never happen. */
			break;
		}
	    }   /* Otherwise, fall through to rewrite reel. */
	} else {
	    if (isstream == TRUE) {
		derr("Archive aborted.\n");
		exit(2);
	    } else {
		fclose(devtty);
		fclose(devttyout);
		return FALSE;   /* Don't rewrite. */
	    }
	}

	/* Tape must be rewritten. */

	derr("Reel #%d will be rewritten.\n\n", ReelNo);

	/* Prepare temp file for reading. */
	if (inputfp == stdin)
	    fflush(tempfp); /* Only flush if we're writing to temp file. */
	rewind(tempfp);
	inputfp = tempfp;

	fclose(devtty);
	fclose(devttyout);

	return TRUE;    /* Rewrite tape. */
    } else {
	/* Succesful completion of writing this reel.
	 * Reached end-of-medium.
	 */

	/* (Note: file list may not have been exhausted.) */

	if (inputfp == stdin) {   /* Reading from standard input. */
	    fclose(tempfp); /* Done with old temp file. */

	    /* Create new file pointer to temporary file. */
	    if( (tempfp=tmpfile()) == (FILE *)0 ) {
		fprintf(stderr, catgets(catd,NL_SETN,95, "Can't create temporary file in directory %s for file name list.\n"), P_tmpdir);
		exit(CHKPTERR);
	    }

	    /* Log first file name. */
	    fprintf(tempfp, "%s\n", Hdr.h_name);
#ifdef DEBUG
fprintf(stderr, "Logged \"%s\" in temp file.\n", Hdr.h_name);
#endif /* DEBUG */
	} else if (inputfp == tempfp) {   /* Still reading from temp file. */
	    char buf[MAXPATHLEN];
	    FILE *newfp;

	    /* Create new temporary file pointer. */
	    if( (newfp=tmpfile()) == (FILE *)0 ) {
		fprintf(stderr, catgets(catd,NL_SETN,95, "Can't create temporary file in directory %s for file name list.\n"), P_tmpdir);
		exit(CHKPTERR);
	    }
#ifdef DEBUG
fprintf(stderr, "Created new temporary file for file name list.\n");
#endif /* DEBUG */

	    /* Log first file name to new file. */
	    fprintf(newfp, "%s\n", Hdr.h_name);
#ifdef DEBUG
fprintf(stderr, "Logged \"%s\" in new file.\n", Hdr.h_name);
#endif /* DEBUG */


	    /* Now copy "rest" of current temp file to the new temp file.
	     * This is necessary because the file list may not be
	     * exhausted yet. Copy the remaining file names to the
	     * new temp file.
	     */

	    if (fgets(buf, MAXPATHLEN, tempfp) == (char *)0) {
		/* There is nothing to copy (we've exhausted the file list). */
		inputfp = stdin;
		fclose(tempfp);
		tempfp = newfp;
	    } else {
		long offset;

#ifdef DEBUG
fprintf(stderr, "Copying rest of old temp file to new.\n");
#endif /* DEBUG */
		offset = ftell(newfp);  /* Save offset after 1st file name. */
		do {
		    fputs(buf, newfp);
		} while (fgets(buf, MAXPATHLEN, tempfp) != (char *)0);

		fclose(tempfp); /* Done with old temp file. */

		fflush(newfp);
		fseek(newfp, offset, 0); /* Skip 1st file name. */

		tempfp = inputfp = newfp;
	    }
	}

	return FALSE;   /* Don't rewrite. */
    }
}


/*
 * postml() -- linking function
 */
postml(namep, np)
char *namep;
register char *np;
{
    struct sym_struct
    {
	unsigned long l_dev;
	unsigned long l_ino;
	struct sym_struct *next;
	char name[2];
    };
    typedef struct sym_struct SYMBOL;

    static SYMBOL **tbl = NULL;
    SYMBOL *sym;
    int key;
    char *mlp;

    if (Option == IN &&
	(Lhdr.l_ino == UNREP_NO || Lhdr.l_dev == OUT_OF_SPACE))
    {
	/*
	 * File came from 32 bit ino that wasn't mapped properly or too
	 * many devices to be sure (old versions of cpio)
	 */
	fprintf(stderr, catgets(catd,NL_SETN,67, "Cannot link this file: %s\n"),
	    Lhdr.l_name);
	return 1;
    }

    /*
     * Allocate our hash table if we haven't already
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
    key = ((Lhdr.l_dev ^ Lhdr.l_ino) & 0x7fffffff) % HASHSIZE;
    for (sym = tbl[key]; sym != NULL; sym = sym->next)
	if (sym->l_ino == Lhdr.l_ino && sym->l_dev == Lhdr.l_dev)
	{
	    int attempt;

            /*
	     * Found it, do the link
	     */
	    if (strcmp(sym->name, np) == 0)
	    {
		printf(catgets(catd,NL_SETN,68, "%s and %s are same file!\n"),
			sym->name, np);
		return 0;
	    }

	    unlink(namep);

	    if (Option == IN && sym->name[0] != '/')
	    {
		strcpy(&Fullname[Pathend], sym->name);
		mlp = Fullname;
	    }
	    else
		mlp = sym->name;

#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
	/* See if we are allowed to link the file. */
	if (ie_ml_import(namep,0,3,0, 0)==-1) return(0);
#endif
	    /* try linking (only twice) */
	    attempt = 0;
	    do
	    {
		if (link(mlp, namep) < 0)
		    attempt++;
		else
		{
		    attempt = 0;
		    break;
		}
	    } while (attempt < 2 && missdir(np) == 0);

#ifdef QFLAG
	    if (Qflag && (went_remote || errno == EROFS))
		return 1;
#endif /* QFLAG */

	    switch (attempt)
	    {
	    case 0:
		if (Verbose)
		    printf(catgets(catd,NL_SETN,69, "%s linked to %s\n"),
			sym->name, np);
		set_time(namep, mklong(Lhdr.l_mtime),
		    mklong(Lhdr.l_mtime));
		return 0;
	    case 1:
		fprintf(stderr,
		    catgets(catd,NL_SETN,70, "Cannot create directory for <%s> (errno:%d)\n"),
		    np, errno);
		bad_try = 1;
		return 0;
	    case 2:
		fprintf(stderr,
		    catgets(catd,NL_SETN,71, "Cannot link <%s> & <%s> (errno:%d)\n"),
		    sym->name, np, errno);
		return 0;
	    }
	}

    /*
     * Didn't see this file before, put it into the hash table
     */
    if ((sym = (SYMBOL *)malloc(sizeof(SYMBOL)+strlen(np)+2)) == NULL)
    {
	static int first = 1;

	if (first)
	    fprintf(stderr, catgets(catd,NL_SETN,73, "No memory for links\n"));
	first = 0;
	return 1;
    }

    sym->l_dev = Lhdr.l_dev;
    sym->l_ino = Lhdr.l_ino;
    strcpy(sym->name, np);
    sym->next = tbl[key];
    tbl[key] = sym;
    return 1;
}

pentry(namep)		/* print verbose table of contents */
register char *namep;
{
	static short lastid = -1;
	static struct passwd *pw;
	register char *cp;

#ifdef NLS
	extern int __nl_langid[];
#endif

	printf("%-7o", Hdr.h_mode & 0177777);
	if (lastid == Hdr.h_uid)
		printf("%-6s", pw->pw_name);
	else {
		setpwent();
		if (pw = getpwuid((int)Hdr.h_uid)) {
			printf("%-6s", pw->pw_name);
			lastid = Hdr.h_uid;
		} else {
			printf("%-6d", Hdr.h_uid);
			lastid = -1;
		}
	}
	printf("%7ld ", mklong(Hdr.h_filesize));
	U.l = mklong(Hdr.h_mtime);

#if defined(NLS) || defined(NLS16)
	if (__nl_langid[LC_TIME] == 0 || __nl_langid[LC_TIME] == 99) {
        	cp = ctime((long *)&U.l);
		cp[24] = '\0';
		fprintf(stdout, " %s  %s\n", cp+4, namep);
	}
	else {
		cp=nl_cxtime((long *)&U.l,catgets(catd2,NL_SETN,106,"%3h %2d %H:%M:%S 19%y"));
		fprintf(stdout," %s  %s\n",cp, namep);
	}
#else
	cp = ctime((long *)&U.l);
	cp[24] = '\0';
	fprintf(stdout, " %s  %s\n", cp+4, namep);
#endif
}


		/* pattern matching functions */
nmatch(s, pat)
char *s, **pat;
{
#ifdef SecureWare
	if ((pat == (char **) 0) || (*pat == (char *) 0))
#else
	if (EQ(*pat, "*"))
#endif
		return 1;
	while (*pat) {
		if ((**pat == '!' && !gmatch(s, *pat+1)) ||
		    gmatch(s, *pat))
			return 1;
		++pat;
	}
	return 0;
}

gmatch(s, p)
register char *s, *p;
{
	if (fnmatch(p,s,0))
		return(0);
	else
		return(1);
}

makdir(namep)		/* make needed directories */
register char *namep;
{
    int rv;
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
	return ie_mkdir(namep);
#else
    rv = mkdir(namep, 0777);
#endif

#ifdef QFLAG
    if (Qflag) {
        if (rv != 0) {
	    /*
	     * If we failed to create a directory because the file
	     * system is read-only, then likely it's remote via NFS,
	     * so set flag and get out
	     */
	    if (errno == EROFS) {
		    went_remote = TRUE;
		    return(-1);
	    }
        } else {
	    /*
	     * else we succeeded in creating a directory, so stat to
	     * see if it's remote.  If so, remove it, set flag, and
	     * get out
	     */
	    if (stat(namep,&Xstatb) == 0) {
	        if ((Xstatb.st_dev & NFS_remote) == NFS_remote) {
		    unlink(namep);
		    went_remote = TRUE;
		    return(-1);
	        }
	    }
        }
    } /* end "if Qflag" */
#endif /* QFLAG */

#if defined(SecureWare) && defined(B1)
    if (rv!=-1 &&(ISB1 && hassysauth(SEC_OWNER) || !ISB1 && (Uid == 0)))
#else
    if (rv != -1 && Uid == 0)
#endif
	if (chown(namep, Hdr.h_uid, Hdr.h_gid) < 0) {
	    fprintf(stderr,catgets(catd,NL_SETN,76, "Cannot chown <%s> (errno:%d)\n"), namep, errno);
	}

    /* return 0 if successful, non-zero otherwise */
    return rv;
}




swap(buf, ct)		/* swap halfwords, bytes or both */
register ct;
register char *buf;
{
	register char c;
	register union swp { long	longw; short	shortv[2]; char	charv[4]; } *pbuf;
	int savect, n, i;
	char *savebuf;
	short cc;

	savect = ct;	savebuf = buf;
	if (byteswap || bothswap) {
		if (ct % 2) buf[ct] = 0;
		ct = (ct + 1) / 2;
		while (ct--) {
			c = *buf;
			*buf = *(buf + 1);
			*(buf + 1) = c;
			buf += 2;
		}
		if (bothswap) {
			ct = savect;
			pbuf = (union swp *)savebuf;
			if (n = ct % sizeof(union swp)) {
				if (n % 2)
					for (i = ct + 1; i <= ct + (sizeof(union swp) - n); i++) pbuf->charv[i] = 0;
				else
					for (i = ct; i < ct + (sizeof(union swp) - n); i++) pbuf->charv[i] = 0;
			}
			ct = (ct + (sizeof(union swp) -1)) / sizeof(union swp);
			while (ct--) {
				cc = pbuf->shortv[0];
				pbuf->shortv[0] = pbuf->shortv[1];
				pbuf->shortv[1] = cc;
				++pbuf;
			}
		}
	}
	else if (halfswap) {
		pbuf = (union swp *)buf;
		if (n = ct % sizeof(union swp))
			for (i = ct; i < ct + (sizeof(union swp) - n); i++) pbuf->charv[i] = 0;
		ct = (ct + (sizeof(union swp) -1)) / sizeof(union swp);
		while (ct--) {
			cc = pbuf->shortv[0];
			pbuf->shortv[0] = pbuf->shortv[1];
			pbuf->shortv[1] = cc;
			++pbuf;
		}
	}
}

set_time(namep, atime, mtime)	/* set access and modification times */
register *namep;
long atime, mtime;
{
	static long timevec[2];

	if (!Mod_time)
		return;
	timevec[0] = atime;
	timevec[1] = mtime;
	utime(namep, timevec);
}

#define MAX_DEVNAME_LENGTH     128
chgreel(x, fl)
	int x;		/* 0 is read, 1 is write	*/
	int fl;		/* file descriptor 		*/
{
	register f;
	char str[MAX_DEVNAME_LENGTH+1];
	FILE	*devtty;
	struct stat statb;
	int  Eotmsg = 0;   /* This is set to 1 for 3480. */

	/* Reset block count for reels. */
	ReelBlocks = 0;

        if (Eotmsg = is3480(fl))
		if ((f = load_next_reel(fl,x)) != -1)
			return(f); /* successful change of reel in auto mode. */

    /* open dev/tty, or die with message to stderr */
	devtty = fopen("/dev/tty", "r");
	devttyout = fopen("/dev/tty", "w");
	if (devtty == NULL || devttyout == NULL) {
		fprintf(stderr,catgets(catd,NL_SETN,77, "Can't open /dev/tty to prompt for more media.\n"));
		exit(2);
	}

    /* print end-of-reel message to dev/tty */
    /* Don't print this message for a 3480. */
    if (!Eotmsg) {
    	derr(catgets(catd,NL_SETN,79, "End of volume\n"));
    }

    /* see if raw file involved, 				*/
	fstat(fl, &statb);	/* no test */
#ifndef RT
     /* if not, quit with message to stderr */
	if ((statb.st_mode & S_IFMT) != S_IFCHR) {
		fprintf(stderr,catgets(catd,NL_SETN,81, "errno: %d, "), errno);
#ifndef NLS
		fprintf(stderr,"Can't %s\n", x? "write output": "read input");
#else
		if (x)
			fprintf(stderr,catgets(catd,NL_SETN,82, "Can't write output\n"));
		else
			fprintf(stderr,catgets(catd,NL_SETN,83, "Can't read input\n"));
#endif /* NLS */
		exit(2);
	}
#else
	if ((statb.st_mode & (S_IFBLK|S_IFREC))==0)
		exit(2);
#endif /* RT */

    /* close the original raw file (ie, first reel) */
	close(fl);	/* no test */

again:
    /* print continuation message to dev/tty */
	if (writestate == CHECKPT)
	    derr(catgets(catd,NL_SETN,110, "Mount a new tape to rewrite reel #%d.\n"), ReelNo);
	derr(catgets(catd,NL_SETN,84, "If you want to go on, type device/file name when ready\n"));

    /* read new name from dev/tty */
	str[0] = '\0';
	fgets(str, MAX_DEVNAME_LENGTH, devtty); /* no test */
	str[strlen(str) - 1] = '\0';

    /* if null name, quit with message to stderr */
	if (!*str) {
		fprintf(stderr,catgets(catd,NL_SETN,85, "User entered a null name for next device file.\n"));
		exit(2);
	}

    /* try new name; notify dev/tty if a problem */
	if ((f = open(str, x? 1: 0)) < 0) {
		/* open failed */
		derr(catgets(catd,NL_SETN,86, "That didn't work.\n"));
		goto again;
	}

    /* close tty, log reel change, return raw file descriptor after good open */
	fclose(devtty);		/* no test */
	fclose(devttyout);	/* no test */
	fprintf(stderr,catgets(catd,NL_SETN,87, "User opened file %s to continue.\n"), str);

	/* Reset flag for new reel (only when writing archive). */
	if (Option == OUT)
	    isstream = isstreamer(f);

	return f;
}

/*VARARGS*/
derr(a, b, c)   /* writes to dev/tty */
{
	(void) fprintf(devttyout, a, b, c);
	(void) fflush(devttyout);
}

missdir(namep)
register char *namep;
{
	register char *np;
	register ct = 2;
        int i=0;

	for (np = namep; *np; ++np)
		if (*np == '/') {
			if (np == namep) continue;  /* skip over 'root slash' */
			*np = '\0';
			if (stat(namep, &Xstatb) == -1) {
				if (Dir) {

#if defined(DUX) || defined(DISKLESS)
                                        if (bit[i] && *(np-1) == '+')
					   *(np-1)= '\0';
#endif /* defined(DUX) || defined(DISKLESS) */
					if ((ct = makdir(namep)) != 0) {
#if defined(DUX) || defined(DISKLESS)
                                        if (bit[i]) {
                                                i++;
                                                *(np-1)= '+';
                                        }
#endif /* defined(DUX) || defined(DISKLESS) */
#ifdef QFLAG
					/*
					 * if makdir encountered a
					 * remote file system in its
					 * attempts, then it set the
					 * went_remote flag.  Check
					 * it, and if set, get out
					 */
					if (Qflag && went_remote) {
						*np = '/';
						return(ct);
					}
#endif /* QFLAG */
						*np = '/';
						return(ct);
					}
#if defined(DUX) || defined(DISKLESS)
					/* ignore result */
					(void) stat(namep,&Xstatb);

                                        if (bit[i]) {
#if defined(SecureWare) && defined(B1)
#ifdef MLTAPE
                                        	ie_chmod(namep,Xstatb.st_mode | 04000, A_directory);
#else
                                        	ie_chmod(namep,Xstatb.st_mode | 04000, 0);
#endif
#else
                                        	chmod(namep,Xstatb.st_mode | 04000);
#endif
						*(np-1)= '+';
					}
#endif /* defined(DUX) || defined(DISKLESS) */
				}else {
					fprintf(stderr,catgets(catd,NL_SETN,88, "missing 'd' option\n"));
					return(-1);
				}
			}
#if defined(DUX) || defined(DISKLESS)
                        i++;
#endif /* defined(DUX) || defined(DISKLESS) */
			*np = '/';
		}
	if (ct == 2) ct = 0;		/* the file already exists */
	return ct;
}

pwd()		/* get working directory */
{
	if (getcwd(Fullname, MAXPATHLEN) == NULL)
	{
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
	    fputs(catgets(catd,NL_SETN,92,"mltape: can't get current working directory: "), stderr);
#else
	    fputs(catgets(catd,NL_SETN,92,"cpio: can't get current working directory: "), stderr);
#endif
	    perror("");
	    exit(2);
	}
	Pathend = strlen(Fullname) + 1;
	Fullname[Pathend - 1] = '/';	/* replace '\0' by '/' */
}
#ifdef CODE_DELETED
char * cd(n)		/* change directories */
register char *n;
{
	char *p_save = Name, *n_save = n, *p_end = 0;
	register char *p = Name;
	static char dotdot[]="../../../../../../../../../../../../../../../../../../../../../../../../../../../../../../../../";
	int slashes, ans;

	if (*n == '/') /* don't try to chdir on full pathnames */
		return n;
	for (; *p && *n == *p; ++p, ++n) { /* whatever part of strings == */
		if (*p == '/')
			p_save = p+1, n_save = n+1;
	}

	p = p_save;
	*p++ = '\0';
	for (slashes = 0; *p; ++p) { /* if prev is longer, chdir("..") */
		if (*p == '/')
			++slashes;
	}
	p = p_save;
	if (slashes) {
		slashes = slashes * 3 - 1;
		dotdot[slashes] = '\0';
		chdir(dotdot);
		dotdot[slashes] = '/';
	}

	n = n_save;
	for (; *n; ++n, ++p) {
		*p = *n;
		if (*n == '/')
			p_end = p+1, n_save = n+1;
	}
	*p = '\0';

	if (p_end) {
		*p_end = '\0';
		if (chdir(p_save) == -1) {
			if ((ans = missdir(p_save)) == -1) {
				fprintf(stderr,catgets(catd,NL_SETN,89, "Cannot chdir (no `d' option)\n"));
				exit(2);
			} else if (ans > 0) {
				fprintf(stderr,catgets(catd,NL_SETN,90, "Cannot chdir - no write permission\n"));
				exit(2);
			} else if (chdir(p_save) == -1) {
				fprintf(stderr,catgets(catd,NL_SETN,91, "Cannot chdir\n"));
				exit(2);
			}
		}
	} else
		*p_save = '\0';
	return n_save;
}
#endif  /* CODE_DELETED */
#ifdef RT
actsize(file)
register int file;
{
	long tlong;
	long fsize();
	register int tfile;

	Actual_size[0] = Hdr.h_filesize[0];	/* save file size */
	Actual_size[1] = Hdr.h_filesize[1];

	if (!Extent)
		return;

	if (file)
		tfile = file;
	else if ((tfile = open(Hdr.h_name,0)) < 0)
		return;

	tlong = fsize(tfile);
	MKSHORT(Hdr.h_filesize,tlong);

	if (Cflag)
		bintochar(tlong);

	if (!file)
		close(tfile);
}
#endif /* RT */



/* isstreamer - return TRUE if file descriptor refers to a 7974 or 7978
 *              and immediate report mode is enabled. Otherwise return FALSE.
 */

isstreamer(fd)
    int fd;
{
    struct stat sbuf;
    if (fstat(fd, &sbuf) < 0) {
	perror("fstat on output failed");
	exit(2);
    }

    /* Return TRUE if we can answer yes to these questoins:
     *  1. Is this a character special file?
     *  2. Is this a 7974 or 7978? (from major number)
     *  3. Does this drive have immediate report on? (from minor number)
     */

    if (((sbuf.st_mode&S_IFMT)&S_IFCHR)    &&              /* 1. */
	 major(sbuf.st_rdev) == STREAM     &&              /* 2. */
	 ((minor(sbuf.st_rdev))&IMMED_REPORT_BIT) == 0) {  /* 3. */
        fprintf(stderr, catgets(catd,NL_SETN,111, "(Using tape drive with immediate report mode enabled (reel #%d).)\n"), ReelNo);
	return TRUE;
    } else
	return FALSE;
}


#if defined(DUX) || defined(DISKLESS)
/*
 *----------------------------------------------------------------------
 *
 *      Title ................. : findpath()
 *      Purpose ............... : scan path for CDF components
 *
 *	Description:
 *
 *
 *      Given a path (possibly an unexpanded cdf with .., ., .+ &
 *      ..+ in it) return a fully expanded path, a bitstring which
 *      contains the permissions of each component, and the number of
 *      components.
 *
 *      Returns: expanded path (returned through "path" argument)
 */

findpath(path,bitstring,bitindex)
    char *path;
    int  bitstring[];
    int *bitindex;
{
    char tmp[MAXPATHLEN];
    short pass;
    char *r,*s,*lastslash;
    char *cp, *cp2;
    int end;
    char ts,save;
    struct stat t,dest;

    *bitindex= end = 0;
    pass = -1;


    /*
     * Remove extra slashes in pathname.
     */

    for (cp=path; *cp; cp++) {
	if (*cp == '/' && *(cp+1) == '/') {
	    for (cp2=cp+2; *cp2 == '/'; cp2++)
		;
	    overlapcpy(cp+1,cp2);
	}
    }

#ifdef DEBUG
fprintf(stderr,"findpath: path <%s>\n", path);
#endif /* DEBUG */

RESET:
    *bitindex=0;
    lastslash = cp = path;
    if (*cp == '/') {
	lastslash = cp;
	cp++;
    }

    while ((cp2=strchr(cp,'/')) != (char *)NULL || *cp) {
	if (cp2 == (char *)0 && *cp != '\0') {
	    cp2=cp;
	    while (*cp2)
		cp2++;
	    end = 1; /* Last component. */
	}


	/* Get a component by putting in a null. */

	save = *cp2;
	*cp2 = '\0';

#ifdef DEBUG
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
fprintf(stderr,"mltape: findpath: looking at component <%s> (pass=%d)\n", cp, pass);
#else
fprintf(stderr,"cpio: findpath: looking at component <%s> (pass=%d)\n", cp, pass);
#endif
#endif /* DEBUG */

	if (!strcmp(lastslash,"/..") || !strcmp(lastslash,"/..+")) {
	    if (lastslash == path) {
		strcpy(path,"/");
		if (!end)
		    strcat(path,cp2+1);
		cp=cp2;
		continue;
	    }
	    else
		*lastslash=0;

	    (void) getcdf(path,tmp,MAXPATHLEN);

	    if (!strcmp(path, tmp)) {
		*lastslash = '/';
		*cp2 = save;
		lastslash = cp2;
		cp = cp2+1;
		continue;
	    }
	    *lastslash='/';
	    /* ignore result */
	    (void) stat(path,&dest);
	    r = tmp;
	    if (*r == '/')
		r++;

	    while ((s = strchr(r,'/')) != (char *)NULL || *r) {
		if (*s == '\0' && *r) {
		    s=r;
		    while (*s++)
			;
		}
		ts = *s;
		*s = '\0';
		/* ignore result */
		(void) stat(tmp,&t);

		if (t.st_ino == dest.st_ino &&
		    t.st_cnode == dest.st_cnode &&
		    t.st_dev == dest.st_dev) {
		    if (!end) {
			strcat(tmp,"/");
			strcat(tmp,cp2+1);
		    } else
			cp = cp2;
		    break;
		}

		*s =ts;
		if (*s == '\0')
		    break;
		r=s+1;
	    }

	    strncpy(path,tmp,MAXPATHLEN);
	    cp=path;
	    if (*cp == '/') {
		lastslash = cp;
		cp++;
	    }
	    continue;
	}

	/* ordinary file */
	/* if a cdf and there is a plus at the end, take it out
	   else just sit bitstring */
	switch (pass) {
	    case -1:             /* first pass , just increment ptrs */
		*cp2=save;
		lastslash = cp2;
		break;

	    case 0:              /* second pass, stat */
		/* ignore result */
		(void) stat(path,&t);
		bitstring[*bitindex]=t.st_mode;
		*bitindex= *bitindex + 1;
		*cp2=save;
		lastslash = cp2;
		break;

	    case 1:                 /* Third pass, change '+'
				       to "+/"  */
		t.st_mode = bitstring[*bitindex];
		*cp2=save;
		lastslash = cp2;
		if ((t.st_mode & S_IFMT) == S_IFDIR &&
		     (t.st_mode & S_ISUID) != 0) {
		    if (cp2 > path && *(cp2-1) == '+') {
			strcpy(tmp,cp2);
			strcpy(cp2,"/");
			*(cp2+1) = '\0';
			strcat(path,tmp);
			cp2++;
		    }
		}
		*bitindex= *bitindex + 1;
		lastslash = cp2;
		break;

	    default:
		break;
	}
	if (*cp2 == '\0')
	    break;
	cp=cp2+1;

    }

#ifdef DEBUG
fprintf(stderr,"findpath: path <%s> (pass %d)\n", path, pass);
cp = getcwd((char *)0, MAXPATHLEN);
fprintf(stderr,"working directory: <%s>\n\n", cp);
free((void *)cp);
#endif /* DEBUG */

    if (++pass < 2)
	goto RESET;

    return;
}


/*
 *---------------------------------------------------------------------
 *
 *      Title ................. : restore()
 *      Purpose ............... : scan path for CDF components
 *
 *	Description:
 *
 *
 *      Given a path (possibly an expanded cdf with "+//" in it)
 *      return a compressed path, a bitstring which contains
 *      the permissions of each component, and the number of components.
 *
 *
 *      Returns:
 *
 *          compressed path ("str")
 *          permissions bit string ("bitstr")
 *          number of components in path ("cntc")
 */

restore(str,bitstr,cntc)
    char *str;
    int bitstr[];
    int *cntc;
{
    int i;
    char *p;

    *cntc = 0;
    for (i=0; i < MAXCOMPONENTS;i++)
	bitstr[i] = 0;

    p = str;
    if (*p == '/')
	p++;

    for (; *p && (p-str) < MAXPATHLEN; p++) {
	if (*p == '/')
	    (*cntc)++;

	if (*cntc >= MAXCOMPONENTS) {
#if defined(SecureWare) && defined(B1) && defined(MLTAPE)
	    fprintf(stderr, "mltape: too many components in file name %s",str);
#else
	    fprintf(stderr, "cpio: too many components in file name %s",str);
#endif
	    break;
	}

	if (*p == '+' && *(p+1) == '/' && (*(p+2) == '/' || *(p+2) == '\0')) {
	    bitstr[*cntc] = 1;
	    overlapcpy(p+1,p+2);
	}
    }

#ifdef DEBUG
    for (i=0; i < MAXCOMPONENTS; i++) {
	printf("bitstr[%d],%d ",i,bitstr[i]);
	printf("(%s/)\n",str);
	fflush(stdout);
    }
#endif /* DEBUG */

    return;
}


overlapcpy(s1,s2)
    char *s1;
    char *s2;
{

    /*
     * Copy overlapping areas.
     * Don't forget '\0' character at end.
     *
     * Note: this copy will only work if "s1" precedes "s2" in memory.
     */

    for (; *s2; s1++,s2++)
	*s1 = *s2;
    *s1 = '\0';
}
#endif /* defined(DUX) || defined(DISKLESS) */



/* Function: perror_msg()
 * Quote the file name to make leading/trailing/embedded blanks
 * visible and issue perror() message.
 */

static perror_msg(name)
char *name;
{
	char buf[MAXPATHLEN];

	strcpy(buf, "\"");
	strcat(buf, name);
	strcat(buf, "\"");
	perror(buf);
}

/* The following code is added for 3480 support. */
#ifndef MT_IS3480
#define MT_IS3480       0x0B /* 3480 device */
#endif

/* is3480() returns 1, if device is a 3480, otherwise, returns 0. */
is3480(mt)
int mt;
{
	struct mtget mtget_buf;

	if ( ioctl(mt, MTIOCGET, &mtget_buf) != -1 &&
		   mtget_buf.mt_type == MT_IS3480 )
			return(1);
	return(0);
}

/* load_next_reel() attempts to load the next cartridge. It returns
 * the file descriptor of the open tape device as the return value
 * if successful, otherwise, returns -1.
 */
load_next_reel(mt,wflg)
int mt;
int wflg;
{
	int f;
	struct mtop mt_com;

	if (wflg) {
		mt_com.mt_op = MTWEOF; /* write 2 filemarks at end of tape */
		mt_com.mt_count = 2;
		if (ioctl(mt, MTIOCTOP, &mt_com) < 0) {
			derr(catgets(catd,NL_SETN,120,"cpio: ioctl to write filemarks failed (%d). aborting..."),errno);
			exit(2);
		}
	}

	mt_com.mt_op = MTOFFL;
	mt_com.mt_count = 1;
        if ( ioctl(mt, MTIOCTOP, &mt_com) < 0 ) {
		derr(catgets(catd,NL_SETN,116,"cpio: ioctl to offline device failed. aborting...\n"));
		exit(2);
	}  

/*
 * The previous ioctl may put the 3480 device offline in the following cases:
 * 1. End of Magazine condition in auto mode.
 * 2. Device in manual/system mode.
 */

/* Check if cartridge is loaded to the drive. */
	if (DetectTape(mt)) {
		fprintf(stderr, catgets(catd,NL_SETN,117,"cpio: auto loaded next media.\n"));
		return(mt);
	} else {
		fprintf(stderr, catgets(catd,NL_SETN,118,"cpio: unable to auto load next media.\n"));
		return(-1); 
	}
}

/* This function checks if a cartridge is loaded into the 3480 drive. */
DetectTape(mt)
{
	struct mtget mtget;

/*	NOTE:  the code below is commented out due to the 3480 driver
 *	returning an error if the device is offline instead of allowing
 *	ioctl to return an OFFLINE indication. (DLM 7/20/93)
 *
 *	if ( ioctl(mt, MTIOCGET, &mtget) < 0 ) {
 *		derr(catgets(catd,NL_SETN,119,"cpio: ioctl to determine device online failed. aborting...\n"));
 *		exit(2);
 *	}
 */
	if ((ioctl(mt, MTIOCGET, &mtget) < 0) ||
	   (!GMT_ONLINE(mtget.mt_gstat))) return(0);

	else return(1);
}
