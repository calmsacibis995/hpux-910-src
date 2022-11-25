/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/REG.300/RCS/config.c,v $
 * $Revision: 1.9.84.5 $	$Author: jwe $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/11 10:49:17 $
 */

static char HPUX_ID[] = "@(#) $Revision: 1.9.84.5 $";

/*
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *				CONFIG.C				     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									     *
 *									     *
 * SOURCE HISTORY:							     *
 * The  source  for  config  originated  from  Bell  System  III code.  The  *
 * implementation was totally  reconstructed to work with S200 hardware and  *
 * was running on S200 HP-UX  Release 2.0 in February,  1984.  Although the  *
 * inner workings were modified  substantially,  the Bell program  skeleton  *
 * was left pretty much  intact, as well as the data  structures  and basic  *
 * flow of control.  In  November,  1984, config was  modified to work with  *
 * the S200 Berkeley based Release 4.0 code.  Some details were changed and  *
 * several new compile-time and user options were added. 		     *
 *									     *
 * APPLICABILITY:							     *
 * 	Runs on S200 HP-UX Release 4.0:  will no longer work with 2.0/2.1    *
 *	Code is leveragable to other environments:  needs to be tailored     *
 *		to generate proper conf.c. 				     *
 *									     *
 * DEPENDENCIES: 							     *
 *	space.h: 	declares all space dependent on tunable parameters   *
 *	opt.h: 		used for dummy system entry points dependent on      *
 *                  	flags set by space.h 				     *
 *	optflag.h: 	used for ifdefs needed by space.h that are not       *
 *		     	user specifiable				     *
 *									     *
 * MODIFICATIONS:							     *
 *									     *
 *	Mods made on April 6, '87 to support DUX, Convergence Networking     *
 *	and RFA server configurability.  - daveg			     *
 *									     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "stdio.h"
#include <time.h>
#include <sys/utsname.h>
#include <ctype.h>

extern void exit();
extern char *strcat(), *strcpy();

#define equal(a,b) (strcmp((a),(b))==0)

#define DUMPDEV		/* dump device available */
#ifdef TRACE
#define DEBUG 		/* print out debugging tables */
/* #define DUMPDEV	   dump device available */
/* #define ARGDEV	   user specifiable argument device available */
#define OPTIONS	 	/* compile time options produced in kernel makefile */
#else
#define OPTIONS 	/* compile time options produced in kernel makefile */
#endif

/* Table sizes used by program */
#define	TSIZE	200		/* max configuration table */
#define	DSIZE	256		/* max device table */
#define	ASIZE	200		/* max alias table */
#define	PSIZE	128		/* max keyword table */
#define	OSIZE	64		/* max option table */
#define SSIZE	10		/* max swap device table */
#define	BSIZE	256		/* max block device table */
/* CSIZE and SDSIZE should be equal */
#define	CSIZE	256		/* max character device table */
#define SDSIZE  256             /* max streams device table */
#define SMSIZE  50              /* max streams module table */
#define MSIZE	32		/* max selcode map table */

/* Bit definitions for "type" field mask */
#define STRMOD  0x40            /* streams module */
#define STRDVR  0x20            /* streams driver */
#define CARD	0x10		/* really is interface card or pseudo-device */
#define ONCE	0x8		/* allow only one specification of device */
#define REQ	0x4		/* required device */
#define	BLOCK	0x2		/* block type device */
#define	CHAR	0x1		/* character type device */

/* Bit definitions for "mask" field mask -- driver routines available */

/* Bit definitions for streams flags in mask */
#define SYNC_QUEUE	0x40000
#define SYNC_QPAIR	0x20000
#define SYNC_MODULE	0x10000
#define SYNC_GLOBAL	0x8000
#define SVR3_OPEN	0x4000
#define SVR4_OPEN	0x2000

#ifdef AUTOCHANGER
#define MOUNT   0x1000   /* mount routine exists */
#endif /* AUTOCHANGER */

#ifdef SNAKES_IO
#define OPT1    0x800	/* Option1() routine exists*/
#endif SNAKES_IO

#define DUMP	0x400   /* dump routine exists */
#define SIZE	0x200	/* size routine exists */
#define LINK	0x100	/* link routine exists */
#define	OPEN	0x80	/* open routine exists */
#define	CLOSE	0x40	/* close routine exists */
#define	READ	0x20	/* read routine exists */
#define	WRITE	0x10	/* write routine exists */
#define	IOCTL	0x8	/* ioctl routine exists */
#define SELECT  0x4	/* select routine exists */
#define SELTRU  0x2	/* select routine is default of seltrue */
#define	ACLOSE 	0x1	/* device all closes flag (see conf.h) */

/* Mask for all streams flags bits in mask */
#define STREAMS_FLAGS	(SYNC_QUEUE | SYNC_QPAIR | SYNC_MODULE | SYNC_GLOBAL | SVR3_OPEN | SVR4_OPEN)

/* Programming tools */
#define	ERROR	1		/* error flag in return code*/
#define NUL	'\0'		/* null character constant */
/* default device spec to indicate auto configuration process is desired */
char *autoconfig =  { "makedev(-1,0xFFFFFF)" };

/* configuration table  - user data */
struct	t	{
	char	devname[41];	/* device, pseudo-device or card product name */
	char 	*handler;	/* ptr to handler name for ease of algorithms */
	unsigned char type;	/* CARD,ONCE,REQ,BLOCK,CHAR */
	int	blk;		/* major device number if block type device */
	int	chr;		/* major device number if char. type device */
 	int 	address;	/* device or card address: minor or selcode*/
	char	mkdevnam[9];	/* special file name for mkdev scripts */
	int	count;		/* sequence number for mkdev scripts */
	int     d_auto;		/* did check_depend make the entry? */
}	table	[TSIZE];

/* master device table - system information */
struct	t2	{
	char	devtype[41];	/* device type name */
	char	hndlr[41];	/* handler name */
	unsigned char type2;	/* CARD,ONCE,REQ,BLOCK,CHAR */
	unsigned int mask;	/* device mask indicating existing handlers */
	int	block;		/* major device number if block type device */
	int	charr;		/* major device number if char type device */
}	devinfo	[DSIZE];

/* alias table - system information */
struct	t3	{
	char	new[41];	/* alias of device */
	char	old[41];	/* reference name of device */
}	alias	[ASIZE];

/* tunable parameter table - system info & user data */
/*
 * Parameter names are limited to 20 characters; values (can be formulas)
 * are limited to 60 chars since the maximum length of an input line (line[])
 * is 100 chars.  Could be arbitrarily extended since scanf will allow wrap.
 */
struct	t4	{
	char	indef[21];		/* input parameter keyword */
	char	oudef[21];		/* output definition symbol */
	char	value[61];		/* actual parameter value or formula */
	char	defval[61];		/* default parameter value or formula */
	char	minval[61];		/* minimum parameter value or formula */
}	parms	[PSIZE];

#ifdef OPTIONS
/* options table - system info & user data */
/*
 * Option names are limited to 20 characters; values (can be formulas)
 * are limited to 70 chars since the maximum length of an input line (line[])
 * is 100 chars.  Could be arbitrarily extended since scanf will allow wrap.
 */
struct	t5	{
	char	optname[21];		/* output option symbol */
	char	optvalue[71];		/* actual option value or formula */
}	options	[OSIZE];
#endif

/*
 * Driver dependency structure, i.e. if one driver is present, what
 * other drivers should be in the dfile
 */
int DRIVER_DEPEND_SIZE=0;
struct driver_depend_type {
	char *trigger;
	char *dependent[6];
} driver_depend[50];

/*
 * The driver/library table.  This table defines which libraries a given
 * driver is dependent on.  If the driver is included in the dfile, then
 * then the libraries that driver is dependent on will be included on
 * the ld(1) command line.  Only optional libraries *need* to be
 * specified in this table, (but required ones can be included, as well).
 */
int DRIVER_LIBRARY_SIZE=0;
struct drver_library_type {
	char *driver;
	char *library[6];
} driver_library[50];

/*
 * The library table.  Each element in the library table describes
 * one unique library.  The flag member is a boolean value, it is
 * initialized to 1 if the library should *always* be included on
 * the ld(1) ommand line, or 0 if the library is optional (i.e. it
 * is only included when one or more drivers require it).  The order
 * of the library structure elements determines the order of the
 * libraries on the ld(1) command line, (i.e. defines an implicit
 * load order).
 */
int LIBRARY_TABLE_SIZE=0;
struct library_table_type {
	char *library;
	int flag;
} library_table[50];

/*
 * list of subsystems or drivers for which a #define <SUBSYSTEM_NAME>
 * (in upper case) should be printed out to conf.c.  This is used
 * by code in opt.h to stub out inter-module procedure references
 * when the particular subsystem is configured out.
 */
int SUBSYSTEMS_SIZE=0;
static char *subsystems[50];

int DRIVER_ID_MAX=0;
struct driver_id_type {
	char handle[41];
	char id[20][41];
} driver_id[20];

struct	t	*p;			/* configuration table pointer */
struct	t2	*q;			/* master device table pointer */
struct	t3	*r;			/* alias table pointer */
struct	t4	*kwdptr;		/* keyword table pointer */
struct	t2	*bdevices[BSIZE];	/* pointers to block devices */
struct	t2	*cdevices[CSIZE];	/* pointers to char. devices */
struct  t2      *sdevices[SDSIZE];      /* pointers to streams devices */
struct  t2      *strmods[SMSIZE];       /* pointers to streams modules */
#ifdef OPTIONS
struct	t5	*optptr;		/* options table pointer */
#endif

struct	t	*locate();		/* find entry in configuration table */
struct	t2	*find();		/* find entry in master device table */
struct	t4	*lookup();		/* finds a keyword in parms table */
#ifdef OPTIONS
struct	t5	*optlookup();		/* finds a name in options table */
#endif
struct utsname myname;

int	eflag	= 0;	/* error in configuration */
int	tflag	= 0;	/* table flag */
int	tmpflag = 0;	/* only print templates for mkdev */
int	aflag	= 0;	/* device address configuration desired */
#ifdef DEBUG
int	dflag	= 0;	/* debugging tables desired */
#endif
#ifdef NIGHTLY_BUILD
int	libsflag = 1;
#endif NIGHTLY_BUILD

char	*path;		/* holds the path name used in conf.c */
char	*path1;		/* holds the path name used in config.mk */

int	bmax	= -1;	/* max. major device number used for block device */
int	cmax	= -1;	/* max. major device number used for char. device */
int     smax    = -1;   /* max. major device number used for streams device */
int     mmax    = -1;   /* max. number used for streams module */
int	blockex	= 0;	/* end of block device table */
int	charex	= 0;	/* end of character device table */
int     streamex = 0;   /* end of streams devices table */
int	subtmp	= 0;	/* temporary for subscripting for bdevices & cdevices */
int     modsub  = -1;   /* temp for subscripting streams strmods */

int	abound	= -1;	/* current alias table size */
int	dbound	= -1;	/* current master device table size */
int	tbound	= -1;	/* current configuration table size */
int	sbound	= -1;	/* current swap device table size */
int	pbound	= -1;	/* current keyword table size */
#ifdef OPTIONS
int	obound	= -1;	/* current options table size */
#endif

int	rtmaj	= -1;	/* major device number for root device */
long	rtmin	= -1;	/* minor device number for root device */

#ifdef notdef
	cnmaj	= -1;	/* major device number for the console device. */
	cnmin	= -1;	/* minor device number for the console device. */
#endif notdef

/* Table used for multiple swap devices */
/* Size is arbitrary since there is no limit in the kernel for the number
   of swap devices */
struct 	swpdev	{
	int	sw_maj;		/* major number for a swap device */
	int	sw_min;		/* minor number for a swap device */
	int	sw_srt;		/* start of the swap area */
	int	sw_siz;		/* size of the swap area */
} swpdev[SSIZE];
int	swapcount = 0;	/* counter of number of swap devices specified */
#define	DEFSWAPSTART 	-1
int 	swapstart = -1;	/* auto-config swap area offset to expect file system */
int 	nswap = 0;	/* auto-config swap area size in decimal blocks */
int 	defnswap = 0;	/* default swap area size (use maximum area) */

#ifdef ARGDEV
int	argmaj	= -1;	/* major device number for execve args device */
long	argmin	= -1;	/* minor device number for execve args device */
#endif

#ifdef DUMPDEV
/* may need more (dumplo, dmmin, dmmax, dmtext) or different parameters */
int	dmpmaj	= -1;	/* major device number for dump device */
long	dmpmin	= -1;	/* minor device number for dump device */
#endif

FILE	*fdconf;	/* configuration file descriptor */
char	*mfile;		/* master device file */
char	*cfile;		/* output configuration table file */
char	*infile;	/* input configuration file */
FILE	*kmakefile;	/* kernel makefile file descriptor */
FILE	*cmakefile;	/* skeleton kernel makefile */
FILE	*makedevfile;	/* skeleton mkdev file */
char	*lfile;		/* file for kernel makefile */
char	conffile[256];	/* skeleton config.mk file */
char	devfile[256];	/* skeleton mkdev file */
FILE	*mkdevfile;	/* mkdev file descriptor */
char	*afile;		/* file for mkdev script */
char	tempstring[256];	 /* temp string variable used for printing */
char	tempstring1[256];	 /* temp string variable used for printing */
#ifdef NIGHTLY_BUILD
FILE	*libsfd;
char 	libsfile[256];
#endif NIGHTLY_BUILD


/* 160 characters on a line is arbitrary since scan provides wraparound */
char	line[161];	/* input line buffer area */
char	linkbuff[513];	/* link routine buffer */
char	*multiple_link[] = {
	"         ", "         ", "         ", "         ", "         ",
	"         ", "         ", "         ", "         ", "         ",
	"         ", "         ", "         ", "         ", "         ",
	"         ", "         ", "         ", "         ", "         ",
	"         ", "         ", "         ", "         ", "         ",
};
char	xbuff[150];	/* buffer for extern symbol definitions */
char	dvnam[41];	/* buffer area for user specified device name*/
char	mknam[9];	/* buffer area for user specified special file name*/
long 	minornum = -1;	/* specified device minor number 6 hex digits */
unsigned int     minornumt;

/* error messages */
char	*usage   = "usage: config [-t] [-m master] [-c c_file] [-l m_file] [-r dir] dfile\n        or\n       config [-r dir] -a\n";
char	*badopt  = "config: option re-specification\n";
char 	*illopt  = "config: illegal option\n";
char	*openerr = "config: error in opening file -- %s\n";


main(argc,argv)
int argc;
char *argv[];
{
	register int b, i, l, s, okay, length;
	register FILE *fd;
	char input[21], buff[41], c, symbol[21];
	char *argv2, *cp;
	struct t *pt, *pt1;
#ifdef OPTIONS
	char optstr[21];
#endif
	register unsigned long chkaddr1;
	register unsigned long chkaddr2;

#ifdef NIGHTLY_BUILD
	printf("You are running the nightly build version of config\n");
 	printf("Additional parameters:\n");
	printf("   -n \"path for conf.c (headers)\" \"path for config.mk (libs)\"\n");
#endif

	argc--;
	argv++;
	argv2 = argv[0];
	argv++;

	/*
	 *  Scan off the 'c', 'm', 'a', 'd', 'l', and 't' options.
         *  Check to make sure options aren't specified more than once.
         */
	while ((argc >= 1) && (argv2[0] == '-')) {
		for (i=1; (c=argv2[i]) != NULL; i++)
		switch (c) {

		/* Output configuration file specification */
		case 'c':
			if (cfile) {
				fprintf(stderr, badopt);
				exit(1);
			}
			if ((argv[0] == NULL) || (argv[0][0] == '-')) {
				fprintf(stderr, usage);
				exit(1);
			}
			cfile = argv[0];
			argv++;
			argc--;
			break;

		/* Input master device table file specification */
		case 'm':
			if (mfile) {
				fprintf(stderr, badopt);
				exit(1);
			}
			if ((argv[0] == NULL) || (argv[0][0] == '-')) {
				fprintf(stderr, usage);
				exit(1);
			}
			mfile = argv[0];
			argv++;
			argc--;
			break;

		/* Kernel makefile file specification */
		case 'l':
			if (lfile) {
				fprintf(stderr, badopt);
				exit(1);
			}
			if ((argv[0] == NULL) || (argv[0][0] == '-')) {
				fprintf(stderr, usage);
				exit(1);
			}
			lfile = argv[0];
			argv++;
			argc--;
			break;

		/* Driver table desired flag */
		case 't':
			tflag++;
			break;

		/* Mkdev script file specification; address checking desired
		 * User interface:  major # is in decimal
		 *		    minor # is in hex
		 */
		case 'a':
			if (afile) {
				fprintf(stderr, badopt);
				exit(1);
			}
			if ((argc <= 2) || (*argv[0] == '-')) {
				aflag++;
				break;
			}
			afile = argv[0];
			argv++;
			argc--;
			aflag++;
			break;
#ifdef DEBUG
		case 'd':
			dflag++;
			break;
#endif

		/* Kernel headers and libs path specification */
		/* A customer version of the -n (nightly build) option */
		case 'r':
			if (path) {
				fprintf(stderr, badopt);
				exit(1);
			}
			if ((argv[0] == NULL) || (argv[0][0] == '-')) {
				fprintf(stderr, usage);
				exit(1);
			}
			path = argv[0];
			argv++;
			argc--;
			path1 = path;
			break;

#ifdef NIGHTLY_BUILD
		case 'n':
			if (path) {
				fprintf(stderr, badopt);
				exit(1);
			}
			if ((argv[0] == NULL) || (argv[0][0] == '-')) {
				fprintf(stderr, usage);
				exit(1);
			}
			path = argv[0];
			printf("\nThe path used for the conf.c headers is: %s\n",
				 path);
			argv++;
			argc--;
			if (path1) {
				fprintf(stderr, badopt);
				exit(1);
			}
			if ((argv[0] == NULL) || (argv[0][0] == '-')) {
				fprintf(stderr, usage);
				exit(1);
			}
			path1 = argv[0];
			printf("The path used for the config.mk libraries is: %s\n",
				 path1);
			argv++;
			argc--;
			break;
#endif

		default:
			fprintf(stderr, illopt);
			fprintf(stderr, usage);
			exit(1);
		} /* end switch */
		argc--;
		argv2 = argv[0];
		argv++;
	} /* end while more options*/

	/* Set up defaults for unspecified files. */
	if (cfile == 0)
		cfile = "conf.c";
	if (mfile == 0)
		mfile = "/etc/master";
	if (lfile == 0)
		lfile = "config.mk";
	if (afile == 0)
		afile = "mkdev";
	if (path == NULL)
		path = "/etc/conf";
	if (path1 == NULL)
		path1 = "/etc/conf";
	/* setup conffile */

#ifdef SNAKES_IO
#ifdef NIGHTLY_BUILD
	strcat(conffile,"config.sys");
#else
	strcat(conffile,path1);
	strcat(conffile,"/config.sys");
#endif NIGHTLY_BUILD
#else NOT SNAKES_IO  /* s300 */

#ifdef NIGHTLY_BUILD
#ifdef CONFIG_INTERNAL
	strcat(conffile,"config.sys.internl");
#else
	strcat(conffile,"config.sys.nightly");
#endif CONFIG_INTERNAL

#else
	strcat(conffile,path1);
	strcat(conffile,"/config.sys");
#endif NIGHTLY_BUILD

#endif NOT SNAKES_IO  /* s300 */

	/* setup devfile */
#ifdef NIGHTLY_BUILD
	strcat(devfile,"mkdev.sys");
#else
	strcat(devfile,path1);
	strcat(devfile,"/mkdev.sys");
#endif

	if (argc == 0 && aflag)
		tmpflag++;
	else {
		if (argv2 == NULL) {
			fprintf(stderr,usage);
			exit(1);
		}
		infile = argv2;
		fd = fopen (infile,"r");
		if (fd == NULL) {
			fprintf(stderr,openerr,infile);
			exit(1);
		}
	}

	/* Open configuration file and set modes. */
	fdconf = fopen (cfile,"w");
	if (fdconf == NULL) {
		fprintf(stderr,openerr,cfile);
		exit(1);
	}
	chmod (cfile,0644);

	/* Open kernel makefile and set modes. */
	kmakefile = fopen (lfile,"w");
	if (kmakefile == NULL) {
		fprintf(stderr,openerr,lfile);
		exit(1);
	}
	chmod (lfile,0744);

	/* Open mkdev file and set modes. */
	if (aflag) {
		mkdevfile = fopen (afile,"w");
		if (mkdevfile == NULL) {
			fprintf(stderr,openerr,afile);
			exit(1);
		}
		chmod (afile,0744);

		/* Open the skeleton mkdev file. */
		makedevfile = fopen (devfile,"r");
		if (makedevfile == NULL) {
			fprintf(stderr,openerr,devfile);
			exit(1);
		}

		if (tmpflag) {
			print_templates();
			fclose(mkdevfile);
			exit(0);
		}
	}

	/* Open the skeleton config.mk file. */
	cmakefile = fopen (conffile,"r");
	if (cmakefile == NULL) {
		fprintf(stderr,openerr,conffile);
		exit(1);
	}
#ifdef NIGHTLY_BUILD
	/* Try to open a libs_file.  If we can't then flag that the open
	   failed and no writing to this file will occur.
	*/
	strcat(libsfile,"libs_file");
	if ((libsfd = fopen (libsfile, "w")) == NULL) {
		libsflag = 0;
	}
#endif NIGHTLY_BUILD


	p = table;	/* p is incremented with each device in enterdev */
	file(); 	/* Read in the master device table and alias table. */

	/*
	 * Scan the input. On errors, a message is printed and the entry
	 * in the user description file is skipped
	 */
	while (fgets(line,100,fd) != NULL) {
		if (line[0] == '*')	/* comment in input file */
			continue;
		l = sscanf(line,"%20s",input);
		if (l <= 0) {
			cp = line;
			length = strlen(line);
			while ((length > 0) &&
			   (*cp == ' ' || *cp == '\t' || *cp == '\n')){
			  	cp++;
				--length;
			}
			if (length <= 0)
				continue;
			else {
			    	error("Incorrect line format");
			    	continue;
			}
		}

		/* Root device specification */

		/*
		 * syntax:	root <name> <minor>
		 *
		 * The buff parameter (name) is necessary to locate the major
		 * number.  The minor number is required, if the root device is
		 * specified at all. (If not, the driver will fill in rootdev
		 * with values corresponding to the boot device.) Negative
		 * values are not checked to allow pseudo-address future
		 * extensions (e.g. allow -1 to be "special").
		 */

		if (equal(input,"root")) {
			l = sscanf(line,"%*8s%40s%x",buff,&minornum);
			if (l != 2) {
				error("Incorrect line format");
				continue;
			}
			if (rtmin >= 0) {
				error("Root device re-specification");
				continue;
			}
			if (minornum > 0xFFFFFF) {
				error("Invalid minor device number");
				continue;
			}
			if ((pt=locate(buff)) == 0) {
				sprintf(tempstring, "You haven't included the %s driver yet", buff);
				error(tempstring);
				continue;
			}
			if ((pt->type & BLOCK) == 0) {
				error("Not a block device");
				continue;
			}
			rtmin = minornum;
			rtmaj = pt->blk;
			continue;
		}

#ifdef notdef
		/* Console device specification */
	
		/* 
		 * syntax:	console <name> <minor> 
		 *
		 * The buff parameter (name) is necessary to locate the major
		 * number.  The minor number is required, if the console device
		 * is specified at all. (If not, the driver will fill in 
		 * cons_mux_dev with values corresponding to the console 
		 * device.) Negative values are not checked to allow 
		 * pseudo-address future extensions (e.g. allow -1 to be 
		 * "special").
		 */

		if (equal(input,"console")) {
			l = sscanf(line,"%*8s%40s%x",buff,&minornum);
			if (l != 2) {
				error("Incorrect line format");
				continue;
			}
			if (cnmin >= 0) {
				error("Console device re-specification");
				continue;
			}
			if (minornum > 0xFFFFFF) {
				error("Invalid minor device number for console");
				continue;
			}
			if ((pt=locate(buff)) == 0) {
				sprintf(tempstring, "You haven't included the %s driver yet", buff);
				error(tempstring);
				continue;
			}
			if ((pt->type & CHAR) == 0) {
				error("Not a block device");
				continue;
			}
			cnmin = minornum;
			cnmaj = pt->chr;
			continue;
		} 
#endif notdef

		/* Swap device specification */

		/*
		 * syntax:	swap <name> <minor> <start> [size]
		 *
		 * The buff parameter (name) is necessary to locate the major
		 * number.  The minor number is required, if the swap device is
		 * requested at all. (If not, the driver will fill in the swap
		 * device table with values corresponding to the boot device.)
		 * Negative values are not checked to allow pseudo-address
		 * future extensions (e.g. allow -1 to be "special").
		 * The start field is interpreted as follows:
		 *	< 0 	==> user expects/requires file system to be
		 *		    present on same device as swap area
		 *	>= 0	==> reserve <start> number of blocks from
		 *		    start of device
		 * The size of the swap area is optional:
		 * 	0  	==> auto-configuration (use max size possible)
		 *	!= 0	==> place an upper bound on the swap area size
		 *	> 0	==> make it adjacent to reserved area
		 *	< 0	==> make it adjacent to end of device
		 * A portion of the device may be unused if the swap area
		 * size is < the total size of the disc - reserved area.
		 */
		else if (equal(input,"swap")) {
			l = sscanf(line,"%*8s%40s%x%d%d", buff, &minornum,
				&swapstart, &nswap);
		        if (strcmp(buff, "auto") == 0) {
				/* have we already filled in SSIZE entries? */
				/* sbound==the index of the last entered    */
				/* swpdev, so table is full when            */
				/* sbound == SSIZE-1                        */
				if (sbound == SSIZE-1) {
					fprintf(stderr,"Can't configure > %d swap devices--ignored %s\n", SSIZE,line);
					continue;
				} else {
					if (sbound >= 0)
						fprintf(stderr,"Warning \"swap auto\" should be first swap directive in dfile\n");
					/* see if it's already there */
					for ( b=0; b<=sbound; b++) {
						if (swpdev[b].sw_maj == -1) {
							error("Swap re-specification");
							continue;
						}
					}
					sbound++;
					swpdev[sbound].sw_maj = -1;
					swpdev[sbound].sw_min =  0;
					swpdev[sbound].sw_srt = -1;
					swpdev[sbound].sw_siz =  0;
					swapstart = DEFSWAPSTART;  /* reset */
					nswap = defnswap;   	   /* reset */
					swapcount++;
					continue;
				}
			}
			if ((l < 3) || (l > 4)) {
				error("Incorrect line format");
				continue;
			}
			if (minornum > 0xFFFFFF) {
				error("Invalid minor device number");
				continue;
			}
			if ((pt=locate(buff)) == 0) {
				sprintf(tempstring, "You haven't included the %s driver yet", buff);
				error(tempstring);
				continue;
			}
			if ((pt->type & BLOCK) == 0) {
				error("Not a block device");
				continue;
			}
			/* enter new swap device into table */
			/* see if it's already there */
			for ( b=0; b<=sbound; b++) {
				if (swpdev[b].sw_min == minornum) {
					error("Swap re-specification");
					continue;
				}
			}

			/* have we already filled in SSIZE entries ?    */
			/* sbound==the index of the last entered swpdev,*/
			/* so table is full when sbound == SSIZE-1      */
			if (sbound == SSIZE-1) {
				fprintf(stderr,"Can't configure > %d swap devices--ignored %s\n", SSIZE,line);
				continue;
			} else {
				sbound++;
				swpdev[sbound].sw_maj = pt->blk;
				swpdev[sbound].sw_min = minornum;
				swpdev[sbound].sw_srt = swapstart;
				swpdev[sbound].sw_siz = nswap;
				swapstart = DEFSWAPSTART;  /* reset */
				nswap = defnswap;   /* reset for next device */
				swapcount++;
			}
			continue;
		}

		/* Default swap device size specification (nswap) */

		/*
		 * syntax:	swapsize	<#blocks>
		 *
		 * Only one parameter (a decimal number of blocks) is required.
		 * This parameter is used for all swap devices, if specified.
		 * Hence, the user can control whether the nswap line overrides
		 * any swap device specifications (in the swap lines) by order:
		 * If the nswap line occurs before any swap lines, swap lines
	 	 * will override; if it occurs after any swap lines, nswap will
		 * be the size for all swap devices.
	 	 */

		else if (equal(input,"swapsize")) {
			l = sscanf(line,"%*8s%d",&nswap);
			if (l != 1) {
				error("Incorrect line format");
				continue;
			}
			defnswap = nswap;
			/* loop will fail if no swap devices seen yet */
			for (s=0; s<=sbound; s++)
				swpdev[s].sw_siz = defnswap;
			continue;
		}

#ifdef ARGDEV
		/* (execve) argument device specification */

		/*
		 * syntax:	argdev <name> <minor>
		 *
		 * If specified at all, the arg device must have a name
		 * and a minor number.  Once again, the minor number is not
		 * checked for negative values for use as a pseudo-address.
		 */
		else if (equal(input,"arg")) {
			l = sscanf(line,"%*8s%40s%x",buff,&minornum);
			if (l != 2) {
				error("Incorrect line format");
				continue;
			}
			if (argmin >= 0) {
				error("Arg device re-specification");
				continue;
			}
			if (minornum > 0xFFFFFF) {
				error("Invalid minor device number");
				continue;
			}
			if ((pt=locate(buff)) == 0) {
				sprintf(tempstring, "You haven't included the %s driver yet", buff);
				error(tempstring);
				continue;
			}
			if ((pt->type & BLOCK) == 0) {
				error("Not a block device");
				continue;
			}
			argmin = minornum;
			argmaj = pt->blk;
			continue;
		}
#endif /*ARGDEV*/

#ifdef DUMPDEV
		/* Dump device specification */

		/*
		 * syntax:	dumpdev <name> <minor>
		 *
		 * If specified at all, the dump device must have a name
		 * and a minor number.  Once again, the minor number is not
		 * checked for negative values for use as a pseudo-address.
		 */
		else if (equal(input,"dump")) {
			l = sscanf(line,"%*8s%40s%x",buff,&minornum);
			if (l != 2) {
				error("Incorrect line format");
				continue;
			}
			if (dmpmin >= 0) {
				error("Dump device re-specification");
				continue;
			}
			if (minornum > 0xFFFFFF) {
				error("Invalid minor device number");
				continue;
			}
			if ((pt=locate(buff)) == 0) {
				sprintf(tempstring, "You haven't included the %s driver yet", buff);
				error(tempstring);
				continue;
			}
			if ((pt->type & BLOCK) == 0) {
				error("Not a block device");
				continue;
			}
			dmpmin = minornum;
			dmpmaj = pt->blk;
			continue;
		}
#endif /*DUMPDEV*/

#ifndef SNAKES_IO
		/* Check for obsolete parameters */
		else if (obsolete(input)) {
			fprintf(stderr,
			    "config: \"%s\" is no longer supported--ignored.\n",
			    input);
			continue;
		}
#endif

#ifdef OPTIONS
		/* User specifiable, compile time options for the makefile */

		/*
		 * syntax:	option <name> <value>
		 *
		 * If specified at all, the option name can be followed by
		 * a number or a formula.
		 */
		else if (equal(input,"option")) {
			l = sscanf(line,"%*8s%20s%70s",optstr,symbol);
			if ((l < 1) || (l > 2)) {
				error("Incorrect line format");
				continue;
			}
			/* see if it's already there */
			if ( (optptr=optlookup(optstr)) != 0) {
				error("Option re-specification");
				continue;
			}
			obound++;
			if (obound == OSIZE)
				error_exit("Option table overflow");
			optptr= (&options[obound]);
			strcpy(optptr->optname,optstr);
			if (l == 1 ) symbol[0] = NUL;
			strcpy(optptr->optvalue,symbol);
			continue;
		}
#endif /*OPTIONS*/

		/*
 		 * Device or parameter specification other than root, swap,
		 * pipe, or dump.
 		 */
		else {
			/* see if it's a tunable parameter */
			/*
		 	 * syntax:	<name> <value>
			 */
			kwdptr = lookup(input);
			if (kwdptr) {
				l = sscanf(line,"%20s%60s",input,symbol);
				if (l != 2) {
					error("Incorrect line format");
					continue;
				}
				if (strlen(kwdptr->value)) {
					error("Parameter re-specification");
					continue;
				}
	 /* The range check for a minimum value will be ignored, if a formula
	    or variable is used, rather than a +/- number */
			       if(strlen(symbol)==strspn(symbol,"+-0123456789"))
					okay = 1;
				else okay = 0;
				if (okay)
#ifdef SNAKES_IO
			    	    if ((*(kwdptr->minval) != NUL) && (atoi(symbol)<atoi(kwdptr->minval))) {
#else
			    	    if(atoi(symbol)<atoi(kwdptr->minval)) {
#endif
					strcpy(kwdptr->value,kwdptr->defval);
					fprintf(stderr,
						"Default value used for %s\n",
						kwdptr->indef);
					continue;
			    	    }
				strcpy(kwdptr->value,symbol);
				continue;
			}

			/* Otherwise, must be a device or board specification */
			/*
		 	 * syntax:		<name>
		 	 *   w/ -a option:
		  	 *	   device:	<name> <minor>
		  	 *	     card:	<name> [minor]
			 *
			 * Only required parameter is device, board, or pseudo-
			 * device name.  If addressing is allowed and the -a
			 * option is specified, minor numbers must be provided
			 * for all devices.  However, cards (and pseudo-devices)
			 * DO NOT require use of the minor field.  If address
			 * error checking is desired, this field can be used to
			 * specify the select code of each interface card.
			 */
			minornum = -1;		/* set to invalid */
			dvnam[0]=NUL;
			mknam[0]=NUL;
			l = sscanf(line,"%40s%x%8s",dvnam,&minornum,mknam);
			if (l < 1) {
				error("Incorrect line format");
				continue;
			}
			/* Does such a device exist in the master table? */
			/* If so, do we allow its specification? */
			if ((q=find(dvnam)) == 0) {
				error("No such device");
				continue;
			}
			if ((q->type2 & ONCE) && (pt1=locate(dvnam))) {
 				if (pt1->d_auto == 0)
					error("Only one specification allowed");
				continue;
			}
			if (aflag && (l == 1) && (!(q->type2 & CARD)) )
				{  /* must specify minor number if not card */
				error("Incorrect line format");
				continue;
			}
			if ((q->type2 & STRMOD) || (q->type2 & STRDVR)) {
				if ((q->mask & (SVR4_OPEN | SVR3_OPEN)) ==
                                               (SVR4_OPEN | SVR3_OPEN)) {
					error("Both SVR3_OPEN and SVR4_OPEN specified in flags field of master file");
					continue;
				}
			}
			if (aflag) {
				/* It's a card: it's select code must be 0-31 */
				/* New ttys: tty01  0xSc0000  [26/28 card]
                                             tty02  0xSc0000  [mux: modem]
                                             tty03  0xSc0104  [mux: port1]
                                             tty04  0xSc0204  [mux: port2]
                                             tty05  0xSc0304  [mux: port3] */
				if (q->type2 & CARD) {
					minornumt=((unsigned int) minornum/65536);
					if (minornumt>=MSIZE&&minornum!=-1) {
						error("Invalid select code");
						continue;
					}
				}
				/* It's a device: minor must be 0x0-0xFFFFFF */
				else if ((minornum>0xFFFFFF) || (minornum<0)) {
					error("Invalid device/card address");
					continue;
					}

				/* Get ready for address checking.  The major
				 * numbers are or'ed in as an effort to provide
				 * unique identification for devices which have
				 * the same minor numbers, but different majors,
				 * and are NOT the same device.
				 */
				chkaddr1=0; chkaddr2=0;
				switch (q->type2 & (CARD|BLOCK|CHAR)) {
				case CARD:
					chkaddr1=minornum; /* could be -1 ! */
					break;
				case BLOCK:
					chkaddr1=minornum|(q->block<<24);
					break;
				case CHAR:
					chkaddr2=minornum|(q->charr<<24);
					break;
				case BLOCK|CHAR:
					chkaddr1=minornum|(q->block<<24);
					chkaddr2=minornum|(q->charr<<24);
					break;
				}
				if (addrcheck(chkaddr1,chkaddr2,q->type2)) {
					error("Address collision");
					continue;
				}
			}

			/* Fill in the contents of the configuration table */
			/* entry for this device.  */
				enterdev(dvnam,minornum,'U');
				check_depend(dvnam);
				check_library_depend(dvnam);

		} /* end of else (tunable param, device or board) */
	} /* end of while scanning */

	/* Set default values for tunable parameters where applicable */
	for (kwdptr=parms; kwdptr<= &parms[pbound]; kwdptr++) {
		if (strlen(kwdptr->value) == 0) {
			if (strlen(kwdptr->defval) == 0) {
			    fprintf(stderr,
				"/etc/master: default value missing for %s\n",
				kwdptr->indef);
			    eflag++;
			}
#ifndef SNAKES_IO
			if (strlen(kwdptr->minval) == 0) {
			    fprintf(stderr,
				"/etc/master: range value missing for %s\n",
				kwdptr->indef);
			    eflag++;
			}
#endif
			strcpy(kwdptr->value,kwdptr->defval);
		}
	}

	/* See if configuration is to be terminated. */
	if (eflag) {
		fprintf(stderr,"\nConfiguration aborted.\n");
		exit (1);
	}

	/*
	 * Enter the correct entries for required devices. The minor
	 * number is assumed to be 0 (since the user doesn't specify it).
	 */
	for (q=devinfo; q<= &devinfo[dbound]; q++) {
		if ((q->type2 & REQ) && (!(locate(q->devtype))))
			enterdev(q->devtype, 0, 'U');
	}

	/* Configuration is okay, so write the two files and quit. */
#ifdef DEBUG
	if (dflag) {
		print_parms();	/* the user may have changed them */
#ifdef OPTIONS
		print_options();
#endif
		print_table();
		print_bdevices();
		print_cdevices();
	}
#endif
	prtconf(fd);
	prtmkfile();
	fclose(fdconf);
	fclose(kmakefile);
	fclose(cmakefile);
	fclose(mkdevfile);
	fclose(makedevfile);
	return(0);
}

#ifndef SNAKES_IO
/* Is word an obsolete parameter? */
obsolete(word)
register char *word;
{
	static char *obsolete_stuff[] = {	/* table of obsolete words */
		"rfa",
		"argdevnblk",
		"ntext",
		"dmmin",
		"dmmax",
		"dmtext",
		"dmshm",
		"shmmaxaddr",
		"shmbrk",
		"shmall",
		"filesizelimit",
		"dskless_cbufs",
		"dskless_mbufs",
		"netmeminit",
		"netmemthresh",
		"ntext",
		NULL				/* end of table */
	};
	register char **p;

	for (p=obsolete_stuff; *p; p++)
		if (strcmp(word, *p)==0)
			return 1;		/* it's obsolete */
	return 0;				/* couldn't find it */
}
#endif


/*
 * This routine is used to check any driver dependencies that may exist,
 * i.e. if one driver is present, what other drivers should be in the dfile
 * If it finds a dependency, it inserts the dependent drivers in the dfile.
 */
check_depend(name)
char *name;
{
	register int    i, k;
	register char	*dptr;

	for (i=0; i < DRIVER_DEPEND_SIZE; i++) {
		if (equal(name,driver_depend[i].trigger)) {
			k = 0;
			dptr = (char *)driver_depend[i].dependent[k];
			while (strcmp(dptr,NUL) != 0) {
				/* look for the device in the master file */
				if ((q=find(dptr)) == 0) {
					dptr = (char *)driver_depend[i].dependent[++k];
					continue;
				}
				/* see if it has been specified already */
				if ((q->type2 & ONCE) && (locate(dptr))) {
					dptr = (char *)driver_depend[i].dependent[++k];
					continue;
				}
				enterdev(dptr, 0, 'A');
				check_library_depend(dptr);
				dptr = (char *)driver_depend[i].dependent[++k];
			}
		}
	}
}

/*
 * This routine is called after we've determined that the driver "name"
 * will be configured into the system.  At that point it is necessary that
 * we check to see if the driver requires any extra libraries.  This
 * routine loops through the driver_library table looking for the
 * specified driver.  If it exists, it loops through the library list
 * and calls check_library() to add that library to the ld(1) list.
 */
check_library_depend(name)
char *name;
{
	int i, j;

	for (i = 0; i < DRIVER_LIBRARY_SIZE; i++) {
		if (equal(name, driver_library[i].driver)) {
			for (j = 0; j < 6; j++) {
				if (equal("", driver_library[i].library[j]))
					continue;
				check_library(driver_library[i].library[j]);
			}
		}
	}
}

/*
 * There is a dependency on the library "library" in dfile.  Loop through
 * library_table searching for the specified library.  If we don't find it
 * then we'll get upset.  Otherwise, flag the library as required and
 * leave.
 */
check_library(library)
char *library;
{
	int i;

	for (i = 0; i < LIBRARY_TABLE_SIZE; i++) {
		if (equal(library, library_table[i].library)) {
			library_table[i].flag = 1;
			return;
		}
	}
	error("dependent library not in library table");
}

/*
 * This routine is used to read in all of the tables from the master description
 * file.  The tables include: the master device table, the alias table, and the
 * tunable parameter table.  All tables are separated by lines containing a '$'
 * in column 1.  Comment lines contain a $ in column 1.
 */
file()
{
	register l;
	register FILE *fd;
	int typ, msk; 	/* temps for handling conversion */
	char t[6][30];
	int num, i, flag;
	char *malloc();

	fd = fopen(mfile,"r");
	if (fd == NULL) {
		fprintf(stderr, openerr, mfile);
		exit(1);
	}
	q = devinfo;

	/*
	 * Scan the master device table.  On errors, print a message
	 * and exit program (non recoverable). After normal exit, dbound
	 * points to last used master table entry.
	 */
	while (fgets(line,100,fd) != NULL) {

		/* Check for the delimiter that indicates beginning*/
		/* of the alias table entries.  */
		if (line[0] == '$') break;
		if (line[0] == '*') continue; 	/* Skip comments. */

		dbound++;
		if (dbound == DSIZE) error_exit("Device table overflow");
		l = sscanf(line,"%40s%40s%x%x%d%d", q->devtype,q->hndlr,&typ,&msk,
			&(q->block), &(q->charr));
		q->type2=typ;
		q->mask=msk;
		if (l < 6) error_exit("Device parameter count");

		/* Update the ends of the block and character device tables. */
		if (q->type2 & BLOCK)
		if ((blockex = max(blockex,q->block)) >= BSIZE)
			error_exit("Bad major device number");
		if (q->type2 & CHAR)
		if ((charex = max(charex,q->charr)) >= CSIZE)
			error_exit("Bad major device number");

		q++;
	}

	/*
	 * Scan the alias device table.  On errors, print a message
	 * and exit program (non recoverable).  After normal exit, abound
	 * last used alias table entry.
	 */
	r = alias;
	while (fgets(line,100,fd) != NULL) {

		/* Check for the delimiter that indicates the beginning of */
		/* the tunable parameter table entries.  */
		if (line[0] == '$') break;
		if (line[0] == '*') continue;	/* Skip comments. */

		abound++;
		if (abound == ASIZE) error_exit("Alias table overflow");
		l = sscanf(line,"%40s%40s",r->new,r->old);
		if (l < 2) error_exit("Alias parameter count");
		r++;
	}

	/*
	 * Scan the tunable parameter table.  On errors, print a message
	 * and exit program (non recoverable).  After normal exit, pbound
	 * last used alias table entry.
	 */
	kwdptr = parms;
	while (fgets(line,100,fd) != NULL) {
		if (line[0] == '$') break;
		if (line[0] == '*') continue;	/* Skip comments. */
		pbound++;
		if (pbound == PSIZE) error_exit("Keyword table overflow");
		l = sscanf(line,"%20s%20s%60s%60s",kwdptr->indef,
			kwdptr->oudef,kwdptr->defval,kwdptr->minval);
		if (l < 2)
			error_exit ("Tunable parameter count");
		if (l == 2)
			*(kwdptr->defval) = NUL;
		if (l == 3)
			*(kwdptr->minval) = NUL;
		*(kwdptr->value) = NUL;
		kwdptr++;
	}
	/* 
	 * Read in the driver_depend table.  Each line is defined to have a
	 * trigger (First paramter) and dependent drivers (all the rest).
	 * There is only up to 6 dependent drivers allowed.
	 */
	num = 0;
	while (fgets(line,100,fd) != NULL) {
		if (line[0] == '$') break;
		if (line[0] == '*') continue;	/* Skip comments. */
		for (i=0;i<7;i++) t[i][0]=NULL;
		/* Grab the trigger name. And dependent drivers.*/
		l = sscanf(line,"%s%s%s%s%s%s%s",
			t[0],t[1],t[2],t[3],t[4],t[5],t[6]);
		if ((l > 7) || (l < 2)) 
			error_exit("Driver Dependant Table Overflow/Underflow");
		driver_depend[num].trigger = NULL;
		for (i=0;i<7;i++) driver_depend[num].dependent[i] = NULL;
		driver_depend[num].trigger = malloc(strlen(t[0])+1);
		if (driver_depend[num].trigger == NULL)
				error_exit("Malloc error: driver dependent");
		strcpy(driver_depend[num].trigger,t[0]);
		i=0;
		while ((t[i+1] != NULL) && (i <= 7)) {
			driver_depend[num].dependent[i]=
						malloc(strlen(t[i+1])+1);
			if (driver_depend[num].dependent[i] == NULL)
				error_exit("Malloc error: driver dependent");
			strcpy(driver_depend[num].dependent[i],t[i+1]);
			i++;
		}
		num++;
	}
	DRIVER_DEPEND_SIZE=num;

	/* 
	 * Read in the driver_library table.  Each line is defined to have a
	 * trigger driver (First paramter) and what drivers need to be included
	 * with it.  Up to 6 dependent drivers are allowed.
	 */
	num=0;
	while (fgets(line,100,fd) != NULL) {
		if (line[0] == '$') break;
		if (line[0] == '*') continue;	/* Skip comments. */
		for (i=0;i<7;i++) t[i][0]=NULL;
		/* Grab the trigger name. And dependent drivers.*/
		l = sscanf(line,"%s%s%s%s%s%s%s",
			t[0],t[1],t[2],t[3],t[4],t[5],t[6]);
		if ((l > 7) || (l < 2)) 
			error_exit("Driver Library Table Overflow/Underflow");
		driver_library[num].driver = NULL;
		for (i=0;i<7;i++) driver_library[num].library[i] = NULL;
		driver_library[num].driver = malloc(strlen(t[0])+1);
		if (driver_library[num].driver == NULL)
				error_exit("Malloc error: driver dependent");
		strcpy(driver_library[num].driver,t[0]);
		i=0;
		while ((t[i+1] != NULL) && (i <= 7)) {
			driver_library[num].library[i]=malloc(strlen(t[i+1])+1);
			if (driver_library[num].library[i] == NULL)
				error_exit("Malloc error: driver dependent");
			strcpy(driver_library[num].library[i],t[i+1]);
			i++;
		}
		num++;
	}
	DRIVER_LIBRARY_SIZE=num;

	/* 
	 * Read in the driver library table.  Each line is defined to have a
	 * library and a flag that defines whether the library must be included
	 * (1), or a (0) indicating the library is optional.
	 */
	num=0;
	while (fgets(line,100,fd) != NULL) {
		if (line[0] == '$') break;
		if (line[0] == '*') continue;	/* Skip comments. */
		/* Grab the trigger name. And dependent drivers.*/
		t[0][0]=NULL;
		l = sscanf(line,"%s%d",t[0],&flag);
		if (l != 2)
			error_exit("Library Table count");
		library_table[num].library = NULL;
		library_table[num].flag = -1;
		library_table[num].library = malloc(strlen(t[0])+1);
		if (library_table[num].library == NULL)
			error_exit("Malloc error: driver library");
		strcpy(library_table[num].library,t[0]);
		if ((flag != 0) && (flag != 1))
			error_exit("Driver Library flag error");
		library_table[num].flag = flag;
		num++;
	}
	LIBRARY_TABLE_SIZE=num;


	/*
 	* list of subsystems or drivers for which a #define <SUBSYSTEM_NAME>
 	* (in upper case) should be printed out to conf.c.  This is used
 	* by code in opt.h to stub out inter-module procedure references
 	* when the particular subsystem is configured out.
 	*/
	num=0;
	while (fgets(line,100,fd) != NULL) {
		if (line[0] == '$') break;
		if (line[0] == '*') continue;	/* Skip comments. */
		t[0][0]=NULL;
		l = sscanf(line,"%s",t[0]);
		if (l != 1)
			error_exit("Subsystem Format Error");
		subsystems[num]= malloc(strlen(t[0])+1);
		if (subsystems[num] == NULL)
			error_exit("Subsystems Malloc Error");
		strcpy(subsystems[num],t[0]);
		num++;
	}
	SUBSYSTEMS_SIZE=num;

	/* This section is where we build drivers and their product numbers.
	*/
	num=0;
	while (fgets(line,100,fd) != NULL) {
		if (line[0] == '$') break;
		if (line[0] == '*') continue;	/* Skip comments. */
		l = sscanf(line,"%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
			driver_id[num].handle,driver_id[num].id[0],
			driver_id[num].id[1],driver_id[num].id[2],
			driver_id[num].id[3],driver_id[num].id[4],
			driver_id[num].id[5],driver_id[num].id[6],
			driver_id[num].id[7],driver_id[num].id[8],
			driver_id[num].id[9],driver_id[num].id[10],
			driver_id[num].id[11],driver_id[num].id[12],
			driver_id[num].id[13],driver_id[num].id[14],
			driver_id[num].id[15],driver_id[num].id[16],
			driver_id[num].id[17],driver_id[num].id[18],
			driver_id[num].id[19]);
		num++;
		if (l <= 0) 
			error_exit("Product Id format error");
	}
	DRIVER_ID_MAX=num;
		

#ifdef DEBUG
	if (dflag) {
		print_devinfo();
		print_alias();
		print_parms();
	}
#endif
	return(0);
}

/*
 * This routine returns the maximum of two integer values.
 */
max(a,b)
register int a, b;
{
	return(a>b ? a:b);
}

/*
 * This routine is used to print a configuration time error message
 * on stderr and then set a flag indicating a contaminated configuration.
 */
error(message)
char *message;
{
	fprintf(stderr,"%sconfig: %s\n",line,message);
	eflag++;
}


/*
 * This routine is used to print a configuration time error message
 * on stderr and exit immediately.  Used for non-recoverable errors.
 */
error_exit(message)
char *message;
{
	fprintf(stderr,"%sconfig: %s\n",line,message);
	exit(1);
}


/*
 * This routine is used to search through the master device table for
 * some specified device.  If the device is found, we return a pointer to
 * the device.  If the device is not found, we search the alias table for
 * this device.  If the device is not found in the alias table, we return a
 * zero.  If the device is found, we change its tempname to the reference name
 * of the device and re-initiate the search for this new name in the master
 * device table.
 */
struct t2 *
find(device)
char *device;
{
	register struct t2 *q;
	register struct t3 *r;
	register char *tempname;

	tempname = device;
	for (;;) {
		for (q=devinfo; q<= &devinfo[dbound]; q++) {
			if (equal(tempname,q->devtype))
				return(q);
		}
		/* If found an alias and the original name is not in the     */
		/* table then exit since alias or master table is incorrect  */
		if (tempname != device)
			error_exit("Incorrect alias table entry");
		for (r=alias; r<= &alias[abound]; r++) {
			if (equal(device,r->new)) {
				tempname = r->old;
				break;
			}
		}
		if (r > &alias[abound])
			return(0);
	}
}

/*
 * This routine is used to search the user configuration table for some
 * specified device.  If the device is found we return a pointer to
 * that device.  If the device is not found, we search the alias
 * table for this device.  If the device is not found in the alias table
 * we return a zero.  If the device is found, we change its tempname to
 * the reference name of the device and re-initiate the search for this
 * new name in the configuration table.
 */
struct t *
locate(device)
char *device;
{
	register struct t *p;
	register struct t3 *r;
	register char *tempname;

	tempname=device;
	for (;;) {
		for (p=table; p<= &table[tbound]; p++) {
			if (equal(tempname,p->devname))
				return(p);
		}
		/* If found an alias and the original name is not in the user */
		/* table, then return not located status. */
		if (tempname != device)
			return(0);
		for (r=alias; r<= &alias[abound]; r++) {
			if (equal(device,r->new)) {
				tempname = r->old;
				break;
			}
		}
		if (r > &alias[abound])
			return(0);
	}
}


/*
 * This routine is used to search the tunable parameter table
 * for the keyword that was specified in the configuration.  If the
 * keyword cannot be found in this table, a value of zero is returned.
 * If the keyword is found, a pointer to that entry is returned.
 */
struct t4 *
lookup(keyword)
char *keyword;
{
	register struct t4 *kwdptr;

	for (kwdptr=parms; kwdptr<= &parms[pbound]; kwdptr++) {
		if (equal(keyword,kwdptr->indef))
			return (kwdptr);
	}
	return(0);
}

#ifdef OPTIONS
/*
 * This routine is used to search the current options table
 * for the option that was just specified in the configuration.  If the
 * keyword cannot be found in this table, a value of zero is returned.
 * If the keyword is found, a pointer to the entry is returned.
 */
struct t5 *
optlookup(option_name)
char *option_name;
{
	register struct t5 *optptr;

	for (optptr=options; optptr<= &options[obound]; optptr++) {
		if (equal(option_name,optptr->optname))
			return(optptr);
	}
	return(0);
}

#endif /*OPTIONS*/

/*
 * This routine enters the device in the user configuration table.  The
 * pointer p points to next available entry in table.  tbound keeps track
 * of upperbound of table.  The pointer q is assumed to be already set
 * to the corresponding master device table entry (set by calling the find
 * routine from main().  Even though the user configuration table contains
 * entries for all of the devices specified, setq keeps linked lists of
 * one master device table entry for all devices that correspond to the
 * same type (same device driver).
 */
enterdev(name,mn,how)
char	*name;
register long 	mn;
char	how;
{
	tbound++;
	if (tbound == TSIZE) error_exit ("Configuration table overflow");

        if (q->type2 & STRMOD)
               modsub++;
	setq();
	strcpy(p->devname,name); /* if device was an alias, this insures it */
				   /* points to product name */
	p->handler = q->hndlr;
	p->type = q->type2;
	p->blk = q->block;
	p->chr = q->charr;
	p->address = mn;
	p->count = 0;
	if (mknam[0] == NUL) strcpy(p->mkdevnam, p->devname);
	else strcpy(p->mkdevnam,mknam);		/* get special file name */
	if (how == 'A')
		p->d_auto = 1;		/* called by check_depend */
	else
		p->d_auto = 0;		/* called by user request */
	p++;
	return(0);
}


/*
 * This routine is used to set the character and/or block table pointers
 * to point to an entry of the master device table.  It creates linked
 * lists of entries in the master device table.  It assumes q already
 * points to the master device entry under consideration (set by main
 * by calling the find routine when the user input is scanned).  It only
 * sets up one link for each device type found (so that the driver is only
 * installed once.  Before setq is called, blockex and charex represent the
 * upper bound on major numbers as set by reading in the master device table.
 * It ignores interface cards and pseudo-devices.
 */
setq()
{
	register struct t2 *ptr;

	switch (q->type2 & CHAR) {
	case 0:
		break;
	case CHAR:
		subtmp = q->charr;
		ptr = cdevices[subtmp];

		/* If there is a DIFFERENT device type already at this major */
		/* number (the master device file is wrong, then change this */
		/* device's number to a new one */
		if (ptr) {
			if (!equal(ptr->devtype,q->devtype)) {
				charex++;
				if (charex == CSIZE)
					error_exit("Character table overflow");
				q->charr = subtmp = charex;
			} else break;	/* already seen device type */
		}
		cdevices[subtmp] = q;
		cmax = max (cmax,subtmp);
		break;
	}
	switch (q->type2 & BLOCK) {
	case 0:
		break;
	case BLOCK:
		subtmp = q->block;
		ptr = bdevices[subtmp];
		if (ptr) {
			if (!equal(ptr->devtype,q->devtype)) {
				blockex++;
				if (blockex == BSIZE)
					error_exit("Block table overflow");
				q->block = subtmp = blockex;
			} else break;	/* already seen device type */
		}
		bdevices[subtmp] = q;
		bmax = max (bmax,subtmp);
		break;
	}

        switch (q->type2 & STRDVR) {
        case 0:
                break;
        case STRDVR:
                subtmp = q->charr;
                ptr = sdevices[subtmp];

                if (ptr) {
                        if (!equal(ptr->devtype,q->devtype)) {
                                streamex++;
                                if (streamex == SDSIZE)
                                   error_exit("Streams device table overflow");
                                q->charr = subtmp = streamex;
                        } else break;   /* already seen this streams type */
                }

                sdevices[subtmp] = q;
                smax = max (smax,subtmp);
                if (smax >= SDSIZE)
                      error_exit("Streams device table overflow");
                break;
        }
        switch (q->type2 & STRMOD) {
        case 0:
                break;
        case STRMOD:
                strmods[modsub] = q;
                mmax = max (mmax,modsub);
                if (mmax >= SMSIZE)
                      error_exit("Streams modules table overflow");
                break;
        }

	return(0);
}


/*
 * This routine  searches the user  configuration  table for devices  whose
 * addresses are  identical.  Note that, on devices that are both block and
 * character,  both block and char  dev_t's  must be checked  individually.
 * This is due to the fact that there is no easy  combination  of major and
 * minor numbers to guarantee  uniqueness  for such devices from just plain
 * character and just plain block devices.
 */
addrcheck(number1,number2,type)
register unsigned long number1;
register unsigned long number2;
register unsigned char type;
{
	register struct t *ptr;

	switch (type & (CARD|BLOCK|CHAR)) {
	case CARD:
		return(0);

	case BLOCK:
	case CHAR:
	case BLOCK|CHAR:
		for (ptr=table; ptr<= &table[tbound]; ptr++) {
			if (number1 && number2) {	/* block and char */
				if ((number1==((ptr->address)|(ptr->blk))) &&
				    (number2==((ptr->address)|(ptr->chr))))
					return(ERROR);
			}
			else if (number1) {	/* just block */
				if (number1==((ptr->address)|(ptr->blk)))
					return(ERROR);
			}
			else if (number2) {	/* just char */
				if (number1==((ptr->address)|(ptr->chr)))
					return(ERROR);
			}
		}
		break;
	default:
		break;
	}
	return(0);
}


/*
 * This routine writes out the configuration file (C program.)
 */
prtconf(fd)
FILE *fd;
{
	register i, j, k, l;
	register current, already_done, nswapdev;
	register struct t2 *ptr;
	register struct swpdev *sw;
	register struct t *p;
	char buf[256];
	int	sflag; 	/* temps for handling conversion */
#ifndef SNAKES_IO
	char word[256];
#endif
#ifdef SNAKES_IO
	struct t4 *maxvfs;
#endif

	/* Print configuration file heading. */
	cf("/*\n *  Configuration information\n */\n\n");

	/* Print defines and values for tunable parameters. */
	cf("\n");
	for (kwdptr=parms; kwdptr<= &parms[pbound]; kwdptr++) {
		cf("#define\t%s\t%s\n",
			kwdptr->oudef, kwdptr->value);
#ifndef SNAKES_IO
                /*
                 * This is the spot to declare configurables
                 * in conf.c rather than space.h. Just add
                 * more strcmps and ||s. Accck.
                 */
                if(!strcmp("indirect_ptes", kwdptr->indef)) {
                        cf("int\t%s = %s;\n", kwdptr->indef, kwdptr->oudef);
                }
#endif
	}

	/*
	 * This is new for release 7.0.  Print defines for certain
	 * included drivers.  This parallels the S800 uxgen features
	 * and aids in configuring pseudo-drivers that are really
	 * subsystems (e.g. dskless).  This could really have
	 * been avoided if we used the "options" feature instead
	 * of pseudo-drivers to configure major subsystems such
	 * diskless. -byb 2/8/88
	 */
	cf("\n");
	for (j = 0; j<SUBSYSTEMS_SIZE; j++) {
		for (p=table; p<= &table[tbound]; p++) {
			if (!strcmp(subsystems[j],p->devname)) {
				for (i=0; p->devname[i] != '\0'; i++)
					buf[i] = (char)_toupper(p->devname[i]);
				buf[i]='\0';
				cf("#define\t%s\n", buf);
			}
		}
	}

	/* Print include file headings. Quotes for include files are used for */
	/* flexibility when used in development kernel compilation */
	cf("\n#include\t\"%s/h/param.h\"\n", path);
	cf("#include\t\"%s/h/systm.h\"\n", path);
	cf("#include\t\"%s/h/tty.h\"\n", path);
	cf("#include\t\"%s/h/space.h\"\n", path);
	cf("#include\t\"%s/h/opt.h\"\n", path);
	cf("#include\t\"%s/h/conf.h\"\n", path);
	cf("\n");

#ifdef SNAKES_IO
	/* Note:  I talked with the LAN folks who said that the special 
	   defines that used to be generated here are no longer needed.
	   It was for historical reasons that do not exist for snakes.
	*/
#else
	/*
	 * Special defines must currently be generated for the networking
	 * code.  This should be changed when they straighten out the driver
	 * routines to be either one driver supporting ieee802 and Ethernet
	 * or make different entry points for the two different drivers.
	 * (Config cannot support their naming scheme as is.)  Note that
	 * these defines are generated regardless of whether the driver is
	 * present -- unused defines won't hurt.
	*/
	cf("#define	ieee802_open	lan_open\n");
	cf("#define	ieee802_close	lan_close\n");
	cf("#define	ieee802_read	lan_read\n");
	cf("#define	ieee802_write	lan_write\n");
	cf("#define	ieee802_link	lan_link\n");
	cf("#define	ieee802_select	lan_select\n");
	cf("#define	ethernet_open	lan_open\n");
	cf("#define	ethernet_close	lan_close\n");
	cf("#define	ethernet_read	lan_read\n");
	cf("#define	ethernet_write	lan_write\n");
	cf("#define	ethernet_link	lan_link\n");
	cf("#define	ethernet_select	lan_select\n");
#endif SNAKES_IO

	/*
	 * These special defines are generated so that the two drivers
	 * named, can share the same link routine.
	 * Note that this define is generated regardless of whether
	 * the driver is present.
	 */
	cf("#define	hpib_link	gpio_link\n");
	cf("#define	lla_link	lan_link\n");
	cf("#define	lan01_link	lan_link\n");

	/*
	 * The nodev and nulldev routines are used to indicate lack of presence
	 * of a driver or it's routines.  Specifically, nulldev just returns and
	 * can be used for open and close routines to enable you to get a
	 * file descriptor for drivers that do not provide such routines. Nodev,
	 * however, returns an ENODEV error code and should be used for drivers
	 * that are not present or routines not supplied other than open/close.
	 * The notty routine is used specifically for drivers that do
	 * not provide an ioctl routine and in that case we would like
	 * it to return ENOTTY over ENODEV.
	 */
	cf("\nextern nodev(), nulldev();\n");
	cf("extern seltrue(), notty();\n");
	cf("\n");

 	/* Initialize the link handler buffer. */
	sprintf(linkbuff,"\nint\t(*driver_link[])() = \n{\n");

	/*
 	 * Search the block and character device tables and generate an extern
 	 * statement for any routines that are needed.  Print devices
	 * that are block and char first, just block next, and char last.
	 * Ignore null entries, keep track of link routines and
	 * print user driver table if option is specified.
 	 *
	 * Note that these tables are searched instead of the configuration
	 * table to guarantee only one extern definition device type present.
	 * This eliminates the need for counters in the configuration table.
 	 */
	if (tflag) {
		printf("\n\t\t\t***************************\n");
		printf("\t\t\t* Table of Device Drivers *\n");
		printf("\t\t\t***************************\n");
		printf("Block Devices:\nmajor\tdevice type\thandler name\n");
	}
	for (i=0; i<=bmax; i++) {
		if ((ptr=bdevices[i]) == 0)
			continue;
		if (tflag)
			printf("%2d\t%s\t\t%s\n",i,ptr->devtype, ptr->hndlr);
		switch (ptr->type2 & (BLOCK|CHAR)) {
		case BLOCK|CHAR:
			sprintf(xbuff,"extern ");
			if (ptr->mask & OPEN) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_open(), ");
			}
			if (ptr->mask & CLOSE) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_close(), ");
			}
			if (ptr->mask & READ) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_read(), ");
			}
			if (ptr->mask & WRITE) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_write(), ");
			}
			if (ptr->mask & IOCTL) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_ioctl(), ");
			}
			if (ptr->mask & SIZE) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_size(), ");
			}
			if (ptr->mask & SELECT) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_select(), ");
			}
#ifdef SNAKES_IO
			if (ptr->mask & OPT1) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_option1(), ");
			}
#endif SNAKES_IO
#ifdef SAVECORE_300
                        if (ptr->mask & DUMP) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_dump(), ");
			}
#endif
			if (ptr->mask & LINK) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_link(), ");
				sprintf (&linkbuff[strlen(linkbuff)],
					"\t%s_link,\n",ptr->hndlr);
			}


#ifdef AUTOCHANGER
			if (ptr->mask & MOUNT) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_mount(), ");
			}
#endif /* AUTOCHANGER */

			/* Block devices MUST have a strategy routine */
			strcat (xbuff,ptr->hndlr);
			strcat (xbuff,"_strategy();");
			cf("%s\n",xbuff);
			break;
		case BLOCK:
			sprintf(xbuff,"extern ");
			if (ptr->mask & OPEN) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_open(), ");
			}
			if (ptr->mask & CLOSE) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_close(), ");
			}
			if (ptr->mask & LINK) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_link(), ");
				sprintf (&linkbuff[strlen(linkbuff)],
					"\t%s_link,\n",ptr->hndlr);
			}

#ifdef AUTOCHANGER
			if (ptr->mask & MOUNT) {
				strcat (xbuff,ptr->hndlr);
				strcat (xbuff,"_mount(), ");
			}
#endif /* AUTOCHANGER */
			/* Block devices MUST have a strategy routine */
			strcat (xbuff,ptr->hndlr);
			strcat (xbuff,"_strategy();");
			cf("%s\n",xbuff);
			break;
		default:
			break;
		}  /* end switch */
	} /* end for block devices */

	if (tflag)
		printf("\nCharacter Devices:\nmajor\tdevice type\thandler name\n");
	for (i=0; i<=cmax; i++) {
		if ((ptr=cdevices[i]) == 0)
			continue;
                if (ptr->type2 & STRDVR)
                        continue;
		if (tflag)
			printf("%2d\t%s\t\t%s\n",i,ptr->devtype,ptr->hndlr);
		if ((ptr->type2 & (BLOCK|CHAR)) != CHAR)
			continue;	/*then block, already processed above */
		/* case char */
		sprintf(xbuff,"extern ");
		if (ptr->mask & OPEN) {
			strcat (xbuff,ptr->hndlr);
			strcat (xbuff,"_open(), ");
		}
		if (ptr->mask & CLOSE) {
			strcat (xbuff,ptr->hndlr);
			strcat (xbuff,"_close(), ");
		}
		if (ptr->mask & READ) {
			strcat (xbuff,ptr->hndlr);
			strcat (xbuff,"_read(), ");
		}
		if (ptr->mask & WRITE) {
			strcat (xbuff,ptr->hndlr);
			strcat (xbuff,"_write(), ");
		}
		if (ptr->mask & IOCTL) {
			strcat (xbuff,ptr->hndlr);
			strcat (xbuff,"_ioctl(), ");
		}
		if (ptr->mask & SELECT) {
			strcat (xbuff,ptr->hndlr);
			strcat (xbuff,"_select(), ");
		}
		if (ptr->mask & LINK) {
			strcat (xbuff,ptr->hndlr);
			strcat (xbuff,"_link(), ");
			sprintf (&linkbuff[strlen(linkbuff)],
				"\t%s_link,\n",ptr->hndlr);
		}
#ifdef SNAKES_IO
		if (ptr->mask & OPT1) {
			strcat (xbuff,ptr->hndlr);
			strcat (xbuff,"_option1(), ");
		}
#endif SNAKES_IO
		xbuff[strlen(xbuff)-2] = ';';
		xbuff[strlen(xbuff)-1] = NUL;
		cf("%s\n",xbuff);
	} /* end for character devices */

        if (smax >= 0) {
		sprintf(xbuff,"extern ");
		strcat (xbuff,"hpstreams_clone_open(), ");
		strcat (xbuff,"hpstreams_open(), ");
		strcat (xbuff,"hpstreams_close(), ");
		strcat (xbuff,"hpstreams_read(), ");
		strcat (xbuff,"hpstreams_write(), ");
		strcat (xbuff,"hpstreams_ioctl(), ");
		strcat (xbuff,"hpstreams_select(), ");
		xbuff[strlen(xbuff)-2] = ';';
		xbuff[strlen(xbuff)-1] = NUL;
		cf("%s\n",xbuff);
	}

	/*
 	 * Search the configuration table for CARDS or PSEUDO-DEVICES and
	 * generate an extern statement for any link routines that are needed.
	 * Also, add link handler entries to the buffers as appropriate.
	 * If user table is specified, add to table of drivers.
	 */
	if (tflag) {
		printf("\nCard Drivers:\ndevice type\thandler name");
		if (aflag) printf("\tselect code");
		printf("\n");
	}
	cf("\n");
	current = already_done = 0;
	for (p=table; p <= &table[tbound]; p++) {
		if (p->type & CARD) {
			q=find(p->devname);
			if (tflag) {
				printf("%s\t\t%s",q->devtype,q->hndlr);
				if (aflag) printf("\t%x",p->address);
				printf("\n");
			}
			if (q->mask & LINK) {
				already_done = 0;
				for (k=0; k<current; k++) {
				       if (strcmp(multiple_link[k],q->hndlr) == 0) {
						already_done = 1;
						break;
					}
				}
				if (already_done == 0) {
					cf("extern %s_link();\n", q->hndlr);
					sprintf(&linkbuff[strlen(linkbuff)], "\t%s_link,\n", q->hndlr);
					multiple_link[current] = q->hndlr;
					current++;
				}
			}
			else error_exit("Link routine missing from master file");
		}
	}


        /*
         * Search the configuration table (struct t) for STRDVR's and
         * generate an extern statement for the link routine.  And
         * also add the link routine to the driver link buffer.
         */
        if (tflag) {
                printf("\nStreams Dvr/Mod req link rtn:\ndevice type\n");
        }
        cf("\n");
        current = already_done = 0;
        for (p=table; p <= &table[tbound]; p++) {
                if (p->type & (STRDVR|STRMOD)) {
                        q=find(p->devname);
                        if (q->mask & LINK) {
                              if (tflag) {
                              printf("%s\n",q->devtype);
                              }
                                already_done = 0;
                                for (k=0; k<current; k++) {
                                       if (multiple_link[k] == q->devtype) {
                                                already_done = 1;
                                                break;
                                        }
                                }
                                if (already_done == 0) {
                                        cf("extern %s_link();\n", q->devtype);
                                        sprintf(&linkbuff[strlen(linkbuff)], "\t%s_link,\n", q->devtype);
                                        multiple_link[current] = q->devtype;
                                        current++;
                                }
                        }
                }
        }

        /* Construct extern statements for streams devices (dmodsw) */

        if (tflag)
              printf("\nStreams Devices:\nmajor\tdevice type\tstreamtab\n");
        for (i=0; i<=smax; i++) {
              if ((ptr=sdevices[i]) == 0)
                    continue;
              if (tflag)
                    printf("%2d\t%s\t\t%s\n", i,ptr->devtype,ptr->hndlr);
              if (ptr->type2 & STRDVR) {
                    sprintf(xbuff,"extern struct streamtab %s;", ptr->hndlr);
                    cf("%s\n",xbuff);
              }
        } /* end for streams devices */


        /* Construct extern statements for streams modules */

        if (tflag)
              printf("\nStreams modules:\nmodule\tstreamtab\n");

        for (i=0; i<=mmax; i++) {
              if ((ptr=strmods[i]) == 0)
                    continue;
              if (tflag)
                    printf("%s\t%s\n", ptr->devtype,ptr->hndlr);
              if (ptr->type2 & STRMOD) {
                    sprintf(xbuff,"extern struct streamtab %s;", ptr->hndlr);
                   cf("%s\n",xbuff);
              }
        } /* end for streams modules */


	/*
 	 * Go through block device table and construct bdevsw of required
	 * routines. If a particular device or required routine is not present,
	 * fill in "nodev" entries. If an open or close routine isn't present,
	 * fill in "nulldev" (NOP) entries.
 	 */
	if (bmax >= 0) cf("\nstruct bdevsw bdevsw[] = {\n");
	else cf("\n/* No entries in bdevsw */\n");
	for (i=0; i<=bmax; i++) {
		ptr = bdevices[i];
		cf("/*%2d*/\t",i);
		if (ptr) {

#ifdef AUTOCHANGER
			if (ptr->mask & OPEN)
				cf("{%s_open,",ptr->hndlr);
			else cf("{nodev,");
#else  /* AUTOCHANGER */
			if (ptr->mask & OPEN)
				cf("%s_open,",ptr->hndlr);
			else cf("nodev,");
#endif /* AUTOCHANGER */

			if (ptr->mask & CLOSE)
				cf(" %s_close,",ptr->hndlr);
			else cf(" nodev,");
			/* Block devices MUST have strategy routine */
			cf(" %s_strategy,", ptr->hndlr);
#ifdef SAVECORE_300
			if (ptr->mask & DUMP)
				cf(" %s_dump,",ptr->hndlr);
			else cf(" nodev,");
#endif
			if (ptr->mask & SIZE)
				cf(" %s_size,",ptr->hndlr);
			else cf(" 0,");
			if (ptr->mask & ACLOSE)
		   		cf(" C_ALLCLOSES,");
			else cf(" 0,");

#ifdef AUTOCHANGER
			if (ptr->mask & MOUNT)
				cf(" %s_mount},",ptr->hndlr);
			else cf(" nodev},");
#endif /* AUTOCHANGER */

		   	cf("\n");
		}

#ifdef SAVECORE_300
#ifdef AUTOCHANGER
		else cf("{nodev, nodev, nodev, nodev, nodev, 0, nodev},\n");
#else  /* AUTOCHANGER */
		else cf("nodev, nodev, nodev, nodev, nodev, 0\n");
#endif /* AUTOCHANGER */
#else
#ifdef AUTOCHANGER
		else cf("{nodev, nodev, nodev, nodev, 0, nodev},\n");
#else  /* AUTOCHANGER */
		else cf("nodev, nodev, nodev, nodev, 0\n");
#endif /* AUTOCHANGER */
#endif
	}
	if (bmax >= 0) cf("};\n");

	/*
 	 * Go through character device table and construct cdevsw of required
	 * routines. If a particular device or required routine is not present,
	 * fill in "nodev" entries. If an open or close routine isn't present,
	 * fill in "nulldev" (NOP) entries.
 	 */
	if (cmax >= 0) cf("\nstruct cdevsw cdevsw[] = {\n");
	else cf("\n/* No entries in cdevsw */\n");

	for (j=0; j<=cmax; j++) {
		ptr = cdevices[j];
		cf("/*%2d*/\t",j);
		if (ptr) {
			if (ptr->mask & OPEN) {
                          if (ptr->type2 & STRDVR) {
                                if (strcmp(ptr->hndlr,"clninfo") == 0)
                                    cf("{hpstreams_clone_open, ");
                                else
                                    cf("{hpstreams_open, ");
                             }
                          else
				cf("{%s_open,",ptr->hndlr);
			}
			else cf("{nulldev, ");

			if (ptr->mask & CLOSE) {
                          if (ptr->type2 & STRDVR)
                               cf(" hpstreams_close,");
                          else
				cf(" %s_close,",ptr->hndlr);
			}
			else cf(" nulldev,");

			if (ptr->mask & READ) {
                          if (ptr->type2 & STRDVR)
                                cf(" hpstreams_read,");
                          else
				cf(" %s_read,",ptr->hndlr);
                        }
			else cf(" nodev,");

			if (ptr->mask & WRITE) {
			     if (ptr->type2 & STRDVR)
				  cf(" hpstreams_write,");
			     else
				  cf(" %s_write,", ptr->hndlr);
			}
			else cf(" nodev,");

			if (ptr->mask & IOCTL) {
			     if (ptr->type2 & STRDVR)
				  cf(" hpstreams_ioctl,");
			     else
			     	  cf(" %s_ioctl,", ptr->hndlr);
			     }
			else cf(" notty,");

			if (ptr->mask & SELECT) {
			     if (ptr->type2 & STRDVR)
				  cf(" hpstreams_select,");
			     else
				  cf(" %s_select,", ptr->hndlr);
			   }
			else if (ptr->mask & SELTRU)
				cf(" seltrue,");
			else cf(" nodev,");

#ifdef SNAKES_IO
			if (ptr->mask & OPT1) {
			     if (ptr->type2 & STRDVR)
				  cf(" hpstreams_option1,");
			     else
				  cf(" %s_option1,", ptr->hndlr);
			   }
			else cf(" nodev,");
#endif SNAKES_IO
			if (ptr->mask & ACLOSE)
		   		cf(" C_ALLCLOSES},");
			else cf(" 0},");
			cf("\n");
		}
#ifdef SNAKES_IO
		else cf("{nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0},\n");
#else
		else cf("{nodev, nodev, nodev, nodev, nodev, nodev, 0},\n");
#endif SNAKES_IO
	}	/* end for character devices */

	if (cmax >= 0) cf("};\n\n");

 	/* Print out block and character device counts, root, swap, and dump */
	/* device information, and the swplo, and nswap values.  */
	cf("int\tnblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);\n");
	cf("int\tnchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);\n");

        /* Go through streams devices table and contruct stream_devs   */
        if ((smax >= 0) || (modsub >= 0)) {
		cf("\nstruct streams_devs streams_devs[] = {\n");
        	for (l=0; l<=smax; l++) {
			ptr = sdevices[l];
			if (ptr) {
				sflag = (ptr->mask & STREAMS_FLAGS) / SVR4_OPEN;
				cf("\t{\"%s\",  \t&%s,\t%d,\t0x%.2x},\n",
				ptr->devtype, ptr->hndlr, ptr->charr, sflag);
			}
		}
		for (l=0; l<=modsub; l++) {
			ptr = strmods[l];
			if (ptr) {
				sflag = (ptr->mask & STREAMS_FLAGS) / SVR4_OPEN;
				cf("\t{\"%s\",  \t&%s,\t%d,\t0x%.2x},\n",
				ptr->devtype, ptr->hndlr, ptr->charr, sflag);
			}
		}
        	cf("};\n");
		cf("int\tstreams_devscnt = sizeof (streams_devs) / sizeof (streams_devs[0]);\n");
	}


        /* Print out root, swap, and dump */
        /* device information, and the swplo, and nswap values.  */

	if (rtmaj == -1)
		cf("\ndev_t\trootdev = %s;\n", autoconfig);
	else cf("\ndev_t\trootdev = makedev(%d, 0x%x);\n",
		     rtmaj,rtmin);

#ifdef SNAKES_IO
#ifdef notdef
	if (cnmaj == -1)
		cf("dev_t\tcons_mux_dev = %s;\n", autoconfig);
	else
		cf("\ndev_t\tcons_mux_dev = makedev(%d, 0x%x);\n",cnmaj,cnmin);
#else notdef
	cf("dev_t\tcons_mux_dev = %s;\n", autoconfig);
#endif notdef
	cf("dev_t\tcons_dev = makedev(0,0);\n");
	cf("dev_t\tswapdev = makedev(3,0);\n");
	cf("dev_t\targdev;\n");
	cf("\n");
#endif SNAKES_IO
	cf("\n/* The following three variables are dependent upon bdevsw and cdevsw. If\n");
	cf("   either changes then these variables must be checked for correctness */\n");
#ifndef SNAKES_IO
	cf("\ndev_t\tswapdev1 = makedev(5, 0x000000);\n");
	cf("int\tbrmtdev = 6;\n");
	cf("int\tcrmtdev = 45;\n");
#endif not SNAKES_IO
	/* print swap device table */
	cf("\nstruct swdevt swdevt[] = {\n");
	if ( swapcount < 1 ) {
		cf("\t{ SWDEF, 0, -1, %d },\n", defnswap);
		swapcount = 1;
	}
	else for (sw=swpdev; sw<=&swpdev[sbound]; sw++) {
			cf("\t{");
			if (sw->sw_maj == -1)
				cf(" SWDEF,");
			else cf(" makedev(%d, 0x%06.6x),",
				     sw->sw_maj, sw->sw_min);
			cf(" 0, ");
			cf(" %d, ",sw->sw_srt);
			cf(" %d },\n",sw->sw_siz);
		};
	/* If tunable parameter 'nswapdev' is greater than the number of
	 * swap devices explicitly specified, then pad the data structure
	 * until it has 'nswapdev' entries. Default 'nswapdev' to SSIZE
	 * in case it does not appear in the master file.
	 */
	if ((kwdptr = lookup("nswapdev")) != 0)
	    nswapdev=(int)strtol(kwdptr->value, (char **)NULL, 0);
	else
	    nswapdev = SSIZE;

	for (i=swapcount+1; i<=nswapdev; i++)
	  cf("\t{ NODEV, 0, 0, 0 },\n");
	cf("};\n\n");

#ifdef ARGDEV
	if (argmaj == -1)
		cf("dev_t\targdev = %s;\n", autoconfig);
	else cf("dev_t\targdev = makedev(0x%d, 0x%x);\n",
			argmaj,argmin);
#endif
#ifdef DUMPDEV
	if (dmpmaj == -1)
		cf("dev_t\tdumpdev = %s;\n", autoconfig);
	else cf("dev_t\tdumpdev = makedev(0x%d, 0x%x);\n",
			dmpmaj,dmpmin);
#endif

	/*
 	* Write out NULL entry into link buffers.
 	* Then write the buffers out into the configuration file.
 	*/
	sprintf(&linkbuff[strlen(linkbuff)],"\t(int (*)())0\n};\n");
	cf("%s",linkbuff);

        for (i=0;i<DRIVER_ID_MAX;i++) {
		cf("char *%s_table[] = {\n",driver_id[i].handle);
		for (j=0;driver_id[i].id[j][0] != NULL;j++)
			cf("\t\t\"%s\",\n",driver_id[i].id[j]);
		cf("\t\tNULL };\n");
	}

	/* Copy dfile to conf.c */
	rewind(fd);
	cf("char dfile_data[] = \"\\\n");
	while (fgets(buf, sizeof(buf), fd)) {
		buf[strlen(buf)-1] = '\0';		/* trash the newline */
		if (buf[0]=='*' || buf[0]=='\0')	/* comment line? */
			continue;			/* ignore it */
#ifndef SNAKES_IO
		if (sscanf(buf,"%20s",word)==1 && obsolete(word))
		continue;				/* ignore it */
#endif
		cf("%s\\n\\\n", buf);
	}
	cf("\";\n");

#ifdef SNAKES_IO
	/* TEMPORARY */

	cf("dev_t swapmir;\n");
	cf("dev_t rootmir;\n\n");
	/* int (*graphics_sigptr)(); */
	
	cf("#ifdef MAX_SUBSYS\n");
	cf("char	subsys_names[][MAX_SUBSYS];\n\n");
	
	cf("subsys_name_type	*p_subsys_names	= subsys_names;\n\n");
	
	cf("struct subsys_mgr_type subsys_mgr[];\n");
	cf("#endif MAX_SUBSYS\n");
	
	cf("#ifdef MAX_FILESYS\n");
	cf("char	filesys_names[][MAX_FILESYS];\n\n");
	
	cf("filesys_name_type	*p_filesys_names	= filesys_names;\n\n");
	
	cf("struct filesys_mgr_type filesys_mgr[];\n\n");
	
	cf("extern\tstruct vfsops ufs_vfsops;\n\n");

	cf("#ifdef PCFS\n");
	cf("extern\tstruct vfsops pcfs_vfsops;\n");
	cf("#endif\n\n");
	
	cf("struct vfsops *vfssw[] = {\n");
	cf("\t&ufs_vfsops,\t\t/* 0 = MOUNT_UFS */\n");
	cf("\t(struct vfsops *)0,\t/* 1 = MOUNT_NFS to be filled in by */\n");
	cf("\t\t\t\t/* nfs_init() or nfsc_link() */\n");
	cf("\t(struct vfsops *)0,\t/* 2 = MOUNT_CDFS to be filled in by */\n");
	cf("\t\t\t\t/* cdfsc_link() */\n");
	cf("#ifdef PCFS\n");
	cf("\t&pcfs_vfsops,\t\t/* 3 = MOUNT_PC */\n");
	cf("#else /* PCFS */\n");
	cf("\t(struct vfsops *)0,\t/* 3 = MOUNT_PC */\n");
	cf("#endif /* PCFS */\n");
	maxvfs = lookup("maxvfs");
	for (i=4; i < (int)strtol(maxvfs->value, (char **)NULL, 0); i++) {
		cf("\t(struct vfsops *)0,\t/* %d = Available */\n", i);
	}
	cf("};\n\n");
	cf("int\tnvfssw = sizeof (vfssw) / sizeof (vfssw[0]);\n");
	cf("#endif MAX_FILESYS\n");
	
	/* TEMPORARY */
#endif SNAKES_IO

	if (aflag) mkdev_script();
}


/*
 *  This routine prints a makefile to the file named by lfile.  If options
 *  are enabled, they are used in this makefile.  As the kernel libraries
 *  are changed, this file should be appropriately modified.
 */
prtmkfile()
{
	int i;
	register struct t *p;
	char cname[1024];
	char cline[1024];

        mf("ROOT = %s\n", path1);
	mf("LIBS = ");
#ifdef NIGHTLY_BUILD
	if (libsflag) 
		lf("LIBS =");
#endif
	for (i = 0; i < LIBRARY_TABLE_SIZE; i++) {
		if (library_table[i].flag) {
			mf("\\\n\t$(ROOT)/%s ", library_table[i].library);
#ifdef NIGHTLY_BUILD
			if (libsflag) 
				lf(" $(ROOT)/%s", library_table[i].library);
#endif
		}
	}
	mf("\n");
#ifdef NIGHTLY_BUILD
	if (libsflag)  {
		lf("\n");
		fclose(libsfd);
	}
#endif

#ifdef OPTIONS
	/* Print defines and values for options. */
	mf("OPTIONS =");
	for (optptr=options; optptr<= &options[obound]; optptr++) {
		mf(" -D%s", optptr->optname); 
		if ((*optptr->optvalue) != NUL ) 
			mf("=%s",optptr->optvalue);
	}
	mf("\n");
#endif /*OPTIONS*/
	strcpy(cname,cfile);
	cname[strlen(cname)-2]=0;
	mf("CONF=%s\n",cname);

	/* Read and write all of config.mk here */

	while (fgets(cline,1024,cmakefile) != NULL) {
		cline[strlen(cline)-1]=0;
		mf("%s\n",cline);
	}

}

#ifdef DEBUG

/* This routine prints out the master device table */

print_devinfo()
{
	register int i;
	register struct t2 *ptr;
	int	typ, msk; 	/* temps for handling conversion */

	printf("\n\t*** Master Device Table ***\n");
	printf("\tindex\tdevtype\thndlr\ttype2\tmask\tblock\tcharr\n");
	for (i=0; i<=dbound; i++) {
		ptr = &(devinfo[i]);
		typ = ptr->type2;
		msk = ptr->mask;
		printf("\t%2d\t%s\t%s\t%x\t%x\t%d\t%d\n", i,
			ptr->devtype, ptr->hndlr, typ,
			msk, ptr->block, ptr->charr);
	}
}

/* This routine prints out the alias device table */
print_alias()
{
	register int i;
	register struct t3 *ptr;

	printf("\n\t*** Alias Table ***\n");
	printf("\tindex\tnew (alias)\told (ref)\n");
	for (i=0; i<=abound; i++) {
		ptr= &(alias[i]);
		printf("\t%2d\t%s\t\t%s\n", i, ptr->new, ptr->old);
	}
}

/* This routine prints out the tunable parameter table */
print_parms()
{
	register int i;
	register struct t4 *ptr;

	printf("\n\t*** Tunable Parameter Table ***\n");
	printf("\tindex\tkeyword\tsymbol\tactual\tdefault\n");
	for (i=0; i<=pbound; i++) {
		ptr= &(parms[i]);
		printf("\t%2d\t%s\t%s\t%s\t%s\n", i, ptr->indef,
			ptr->oudef, ptr->value, ptr->defval);
	}
}

#ifdef OPTIONS
/* This routine prints out the options table */
print_options()
{
	register int i;
	register struct t5 *ptr;

	printf("\n\t*** Options Table ***\n");
	printf("\tindex\toption\tvalue\n");
	for (i=0; i<=obound; i++) {
		ptr= &(options[i]);
		printf("\t%2d\t%s\t%s\n", i, ptr->optname, ptr->optvalue);
	}
}
#endif /*OPTIONS*/

/* This routine prints out the user configuration table */
print_table()
{
	register int i;
	register struct t *ptr;
	int	typ; 	/* temp for handling conversion */

	printf("\n\t*** User Configuration Table ***\n");
	printf("\tindex\tdevname\thandler\ttype\tblk\tchr\taddress\n");
	for (i=0; i<=tbound; i++) {
		ptr= &(table[i]);
		typ=ptr->type;
		printf("\t%2d\t%s\t%s\t%x\t%d\t%d\t%x\n", i,
			ptr->devname, ptr->handler, typ,
			ptr->blk, ptr->chr, ptr->address);
	}
}

/* This routine prints out the block device table */
print_bdevices()
{
	register int i;
	register struct t2 *ptr;
	int	typ, msk; 	/* temps for handling conversion */

	printf("\n\t*** Block Device Table ***\n");
	printf("\tindex\tdevtype\thndlr\ttype2\tmask\tblock\n");
	for (i=0; i<=bmax; i++) {
		ptr=bdevices[i];
		if (ptr)  {
			typ=ptr->type2;
			msk=ptr->mask;
			printf("\t%2d\t%s\t%s\t%x\t%x\t%d\n", i,
				ptr->devtype, ptr->hndlr, typ, msk, ptr->block);
		}
		else printf("\t%2d  No device present\n",i);
	}
}

/* This routine prints out the character device table */
print_cdevices()
{
	register int i;
	register struct t2 *ptr;
	int	typ, msk; 	/* temps for handling conversion */

	printf("\n\t*** Character Device Table ***\n");
	printf("\tindex\tdevtype\thndlr\ttype2\tmask\tchar\n");
	for (i=0; i<=cmax; i++) {
		ptr=cdevices[i];
		if (ptr)  {
			typ=ptr->type2;
			msk=ptr->mask;
			printf("\t%2d\t%s\t%s\t%x\t%x\t%d\n", i, ptr->devtype,
				ptr->hndlr, typ, msk, ptr->charr);
		}
		else printf("\t%2d  No device present\n",i);
	}
}
#endif /*DEBUG*/

/* This routine prints the templates that the mkdev command usually
 * prints.  It is invoked with the -a option.  -a with a "dfile"
 * will do address checking; -a without a "dfile" will produce a list
 * of templates.
 */
print_templates()
{
	char cline[1024];

	/* Read and write all of the mkdev script here */
	while (fgets(cline,1024,makedevfile) != NULL) {
		cline[strlen(cline)-1]=0;
		df("%s\n",cline);
	}
}

/*
 * This routine builds a script of mknod commands for the user.  It ignores
 * interface  cards and  minimally  prints the  required  drivers'  special
 * files.  The special  file names used are the mkdevnam  fields of the user
 * configuration  table or, if not specified, they are generated here.  The
 * special file names  generated  follow the  convention  that each special
 * file for the same type of device is the  devname  (which is usually  the
 * product  number)  followed  by a dot and a  sequence  number.  The first
 * occurance of a device (number 0) has no sequence  number  printed.  Note
 * that for the require drivers special files to work properly, if the user
 * doesn't  specify them with  addresses, the minor number field is assumed
 * to be 0.
 */
mkdev_script()
{
	register int i, j, counter;
	register struct t *ptr, *ptr1, *ptr2;
	char sequence[3];

	/* set up sequence numbers for devices */
	for (i=0; i<=tbound; i++) {
		ptr1= &table[i];
		if (((ptr1->type) & (BLOCK|CHAR)) && (!ptr1->count)) {
			counter=0;	/* start anew for this device name */

			/* look for others with the same name and if found */
			/* give each a new, successive, sequence number */
			for (j=i+1; j<=tbound; j++) {
				ptr2 = &table[j];
				if (((ptr2->type) & (BLOCK|CHAR)) &&
				   (!strcmp(ptr1->mkdevnam,ptr2->mkdevnam)))
					ptr2->count = (++counter);
			}
		}
	}

	/* print the mkdev templates */
	print_templates();

	/* For each element of the table print the mknod statement. */
	for (i=0; i<=tbound; i++) {
		ptr = &table[i];
		sequence[0]=NUL;
		if (ptr->count) sprintf(sequence,"%d",ptr->count);
		switch (ptr->type & (BLOCK|CHAR)) {
		case CHAR:
		   if (ptr->address == 0 || ptr->address == -1) break;
		   df("/etc/mknod\t/dev/%s%c%s\tc\t%d\t0x%x\n",
				ptr->mkdevnam, (ptr->count?'.':NUL),
				sequence, ptr->chr, ptr->address);
		   break;
		case BLOCK:
		   if (ptr->address == 0) break;
		   df("/etc/mknod\t/dev/%s%c%s\tb\t%d\t0x%x\n",
				ptr->mkdevnam, (ptr->count?'.':NUL),
				sequence, ptr->blk, ptr->address);
		   break;
		case (CHAR|BLOCK):
		   if (ptr->address == 0) break;
		   df("/etc/mknod\t/dev/%s%c%s\tb\t%d\t0x%x\n",
				ptr->mkdevnam, (ptr->count?'.':NUL),
				sequence, ptr->blk, ptr->address);
		   df("/etc/mknod\t/dev/r%s%c%s\tc\t%d\t0x%x\n",
				ptr->mkdevnam, (ptr->count?'.':NUL),
				sequence, ptr->chr, ptr->address);
	 	   break;
		}
	};
}


/* Print stuff to the makefile */
#include <varargs.h>
/* VARARGS0 */
mf(va_alist)
va_dcl
{
	va_list args;
	char *fmt;

	va_start(args);
	fmt = va_arg(args, char *);
	vfprintf(kmakefile, fmt, args);
	va_end(args);
}


/* Print stuff to the configuration file */
/* VARARGS0 */
cf(va_alist)
va_dcl
{
	va_list args;
	char *fmt;

	va_start(args);
	fmt = va_arg(args, char *);
	vfprintf(fdconf, fmt, args);
	va_end(args);
}


/* Print stuff to the mkdev file */
/* VARARGS0 */
df(va_alist)
va_dcl
{
	va_list args;
	char *fmt;

	va_start(args);
	fmt = va_arg(args, char *);
	vfprintf(mkdevfile, fmt, args);
	va_end(args);
}

#ifdef NIGHTLY_BUILD

lf(va_alist)
va_dcl
{
	va_list args;
	char *fmt;

	va_start(args);
	fmt = va_arg(args, char *);
	vfprintf(libsfd, fmt, args);
	va_end(args);
}

#endif
