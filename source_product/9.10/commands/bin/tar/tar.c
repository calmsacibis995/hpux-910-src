#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 72.16 $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <sys/sysmacros.h>
#include <string.h>
#include <sys/param.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include <tar.h>

static	jmp_buf	jmpbuf;

#if defined(SecureWare) && defined(B1)
#include <sys/security.h>
#include <prot.h>
static char *device = NULL;
/* The following are unused by tar but are expected to be declared
 * externally by export_sec.c
 */
short   Rflag;
short   Lflag = 0, Cflag = 0, Aflag = 0, Pflag = 0;
#endif

#ifdef NLS
#define NL_SETN 1	/* message set number */
#define MAXLNAME 14	/* maximum length of a language name */
#include <nl_types.h>
#include <locale.h>
nl_catd nlmsg_fd;
nl_catd nlmsg_tfd;
#else
#define catgets(i, sn, mn, s)	(s)
#endif NLS

char	*strcat();
daddr_t	bsrch();

#define TBLOCK		512
#define NBLOCK		64
#define MAXBLOCK	64

#define NAMSIZ		100	
#define MODE_SZ		8	
#define UID_SZ		8
#define GID_SZ		8
#define SIZE_SZ		12
#define MTIME_SZ	12
#define DSIZE		1024
#define CHKSUM_SZ	8
#define MAGIC_SZ	6
#define VERSION_SZ	2
#define UNAME_SZ	32
#define GNAME_SZ	32
#define DEV_SZ		8
#define PREFIX_SZ	155
union hblock {
	char dummy[TBLOCK];
	struct header {
		char name[NAMSIZ];
		char mode[MODE_SZ];
		char uid[UID_SZ];
		char gid[GID_SZ];
		char size[SIZE_SZ];
		char mtime[MTIME_SZ];
		char chksum[CHKSUM_SZ];
		char typeflag;
		char linkname[NAMSIZ];
		char magic[MAGIC_SZ];
		char version[VERSION_SZ];
		char uname[UNAME_SZ];
		char gname[GNAME_SZ];
		char devmajor[DEV_SZ];
		char devminor[DEV_SZ];
		char prefix[PREFIX_SZ];
	} dbuf;
} dblock, *tbuf;

struct linkbuf {
	ino_t	inum;
	dev_t	devnum;
	int	count;
	char	pathname[NAMSIZ + 1];      /* add 1 for the last null */
	struct	linkbuf *nextp;
} *ihead;

struct stat stbuf;

int	rflag, xflag, vflag, Vflag, tflag, mt, cflag, mflag, oflag, pflag;
int	bflag = 0;	/* -b option on or off ? */
int	nine_tt	= 0;	/* Nine track tape ?	*/

char n_buf[NAMSIZ + PREFIX_SZ];
struct passwd *pw_ent;
struct group  *gr_ent;

#define TIO_READ 1	/* tells tape_io() to read */
#define TIO_WRITE 2	/* tells tape_io() to write */
#define OLDV 1		/* The old version of tar */
#define NEWV 2		/* The new version of tar (POSIX/ustar) */
int	Version = NEWV;	/* OLDV or NEWV depending on -O and -N */
int	Tmagic;		/* 1 if TMAGIC in current header, 0 otherwise */
#ifdef ACLS
int	aclflag = 0;	/* quiet flag for ACLS */
#endif

#ifdef SYMLINKS
int	hflag;
struct symbuf {
	struct symbuf	*previous;
	struct symbuf	*next;
	char		sympath[DSIZE];
};
#endif	/*  SYMLINKS  */
#if defined(DISKLESS) || defined(DUX)
int	Hflag;
void	checkhd();
#endif	/*  defined(DISKLESS) || defined(DUX)  */
int	term, chksum, wflag, recno, first, linkerrok;
int	freemem = 1;

#ifndef MT_ISQIC        /* add the MT_ISQIC ifdef for backward compiling  */
#define MT_ISQIC  0x09  /* on the 8.0/S300 and 8.07/S700, etc.            */
#endif

#ifndef MT_ISEXABYTE        /* Add the MT_ISEXABYTE define for */
#define MT_ISEXABYTE  0x0A  /* support for EXB-8505 device.    */
#endif

int	nblock = 20;  /* The default size retained as the old NBLOCK value of 20
			 rather than have the new block size of 64 ( NBLOCK ) */


long   n_tmp_entries = 0;    /* Number of entries in tmp file */
daddr_t   *tmp_offsets;   /* pointer to array of offsets of entries in tmp file */


FILE	*tfile;
char	tname[] = "/tmp/tarXXXXXX";


char	*usefile;
char	*usefil1;
char	magtape[]	= "/dev/rmt/0m";
char	magtap1[]	= "/dev/rmt8";
char	dummych[]	= "";
int	special		= 1;

/*
**	Checks if the device whose file descriptor is 'fd' is a nine
**	track device or not.
**	Assumption  : Expects 'fd' to be a valid file descriptor.
**	Side Effect : Sets nine_tt to 1 if nine track device else resets
**		      nine_tt to 0.
**	Return value: None.
*/

void
Check_9TrackTape(fd)
int fd;
{
        struct mtget buff;

	if (ioctl(fd, MTIOCGET, &buff) != -1)
		nine_tt	= (((buff.mt_type == MT_ISSCSI1)
			    || (buff.mt_type == MT_ISSTREAM)) ? 1 : 0);
	else
		nine_tt	= 0;
}



main(argc, argv)
int	argc;
char	*argv[];
{
	char *cp;
	int speed;
	int onintr(), onquit(), onhup(), onterm();
	struct mtget mtget_bfr;


#ifdef NLS || NLS16			/* initialize to the current locale */
	char *pc;
	unsigned char lctime[5+4*MAXLNAME+4];
	unsigned char savelang[5+MAXLNAME+1];

	if (!setlocale(LC_ALL, "")) {		/* setlocale fails */
		fputs(_errlocale("tar"), stderr);
		putenv("LANG=");
		nlmsg_fd = (nl_catd)-1;		/* use default messages */
		nlmsg_tfd = (nl_catd)-1;
	} else {				/* setlocale succeeds */
		nlmsg_fd = catopen("tar", 0);	/* use $LANG messages */
		strcpy(lctime, "LANG=");	/* $LC_TIME affects some msgs */
		strcat(lctime, getenv("LC_TIME"));
		if (lctime[5] != '\0') {	/* if $LC_TIME is set */
			strcpy(savelang, "LANG=");	/* save $LANG */
			strcat(savelang, getenv("LANG"));
			if ((pc = strchr(lctime, '@')) != NULL) /*if modifier*/
				*pc = '\0';	/* remove modifer part */
			putenv(lctime);		/* use $LC_TIME for some msgs */
			nlmsg_tfd = catopen("tar", 0);
			putenv(savelang);	/* reset $LANG */
		} else				/* $LC_TIME is not set */
			nlmsg_tfd = nlmsg_fd;	/* use $LANG messages */
	}
#endif NLS || NLS16

#if defined(SecureWare) && defined(B1)
	if(ISB1)
        	ie_init(argc,argv);
#endif

	if (argc < 2)
		usage();

	tfile = NULL;
	usefile =  magtape;
	usefil1 =  magtap1;
	argv[argc] = 0;
	argv++;
	for (cp = *argv++, speed = 0; *cp; cp++, speed--)
		switch(*cp) {
#ifdef ACLS
		case 'A':
		        aclflag = 1;
			break;
#endif
		case 'O':
			Version = OLDV;
			break;
		case 'N':
			Version = NEWV;
			break;
		case 'f':
			if (!(*argv))
				usage();
			usefile = *argv++;
			usefil1 = dummych;
			stat(usefile, &stbuf);
                        /* check if file exists and is not a directory */
			if (stbuf.st_mode &&
                           ((stbuf.st_mode & S_IFMT) == S_IFDIR))
			    usage();
			if(!strcmp(usefile, "-"))
				special = 0;
			if((stbuf.st_mode & S_IFIFO) ||
			   (stbuf.st_mode & S_IFSOCK))
			    special = 0;
			break;
		case 'c':
			cflag++;
			rflag++;
			break;
		case 'u':
			mktemp(tname);
			if ((tfile = fopen(tname, "w")) == NULL) {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "Tar: cannot create temporary file (%s)\n")), tname);
				done(1);
			}
			/* FALL THROUGH */
		case 'r':
			rflag++;
			break;
		case 'V':
			Vflag++;	/* Fall through to v */
		case 'v':
			vflag++;
			break;
		case 'w':
			wflag++;
			break;
#if defined(DISKLESS) || defined(DUX)
		case 'H':
			Hflag++;
			break;
#endif	/*  defined(DISKLESS) || defined(DUX)  */
		case 'x':
			xflag++;
			break;
		case 't':
			tflag++;
			break;
		case 'm':
			if (speed != 1)		/* if speed=1 'm' means */
				mflag++;	/* medium speed		*/
			break;
		case '-':
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			magtape[9] = *cp;
			magtap1[8] = *cp;
			speed = 2;		/* next time will be 1 */

			usefile = magtape;
			usefil1 = magtap1;
			break;
		case 'b':
			if (!(*argv))
				usage();
			nblock = atoi(*argv++);
#ifndef hp9000s800
			if (nblock > NBLOCK || nblock <= 0) {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "Invalid blocksize. (Max %d)\n")), NBLOCK);
#else hp9000s800
			if (nblock > MAXBLOCK || nblock <= 0) {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "Invalid blocksize. (Max %d)\n")), MAXBLOCK);
#endif	/*  hp9000s800  */
				done(1);
			} else
				bflag++;
			break;
		case 'l':
			if (speed == 1) {
				magtape[10] = *cp;
				usefile = magtape;
				usefil1 = magtap1;
			}
			else
				linkerrok++;
			break;
		case 'o':
			oflag++;
			break;
		case 'p':
			pflag++;
			break;
		case 'h':
			if (speed == 1) {
				magtape[10] = *cp;
				usefile = magtape;
				usefil1 = magtap1;
				break;
			}
#ifdef SYMLINKS
			hflag++;
			break;
#else not SYMLINKS
			/*	else fall thru -- error */
#endif	/*  not SYMLINKS  */
		default:
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "tar: %c: unknown option\n")), *cp);
			usage();
		}
	tbuf = (union hblock *)malloc(NBLOCK*TBLOCK);
	if (tbuf == NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "tar: blocksize %d too big, can't get memory\n")), nblock);
		done(1);
	}
#if defined(SecureWare) && defined(B1)
	if(ISB1)
        	device = usefile;
#endif	
	if (rflag) {
		if (xflag || tflag)
			usage();
		if (cflag && tfile != NULL)
			usage();
		if (signal(SIGINT, SIG_IGN) != SIG_IGN)
			signal(SIGINT, onintr);
		if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
			signal(SIGHUP, onhup);
		if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
			signal(SIGQUIT, onquit);
/*
		if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
			signal(SIGTERM, onterm);
*/
		if (strcmp(usefile, "-") == 0) {
			if (cflag == 0) {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "Can only create standard output archives\n")));
				done(1);
			}
			mt = dup(1);
			if (!bflag)
				nblock = 1;
		}
		else
		  if (!cflag) {
#if defined(SecureWare) && defined(B1)
		    if (((ISB1) && (mt = open(usefile, O_RDWR)) < 0 &&
		        (mt = open((device=usefil1),O_RDWR)) < 0) ||
		        ((!ISB1) && (mt = open(usefile,O_RDWR)) < 0 &&
		        (mt = open(usefil1,O_RDWR)) < 0)) {
#else
		    if ((mt = open(usefile,O_RDWR)) < 0 && (mt = open(usefil1,O_RDWR)) < 0) {
#endif
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6,
				"tar: cannot open %s\n")), usefile);
			done(1);
			}
		
		    stat(usefile, &stbuf);
		    if( S_ISCHR(stbuf.st_mode)) {

		    /* This ioctl will succeed only for mag tape devices   *
		     * If ioctl fails it means it is not a mag tape device *
		     * The following check is only for QIC devices.        *
		     */

		      if(ioctl(mt,MTIOCGET,&mtget_bfr) != -1)
			if((mtget_bfr.mt_type == MT_ISQIC) ||
			   (mtget_bfr.mt_type == MT_ISEXABYTE)) {
			  fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,78,"tar: option not supported for this device %s\n")), usefile);

			  done(1);
                        }
                    } /* if S_ISCHR */
		  } /* !cflag */
		  else {
		    if ((mt =  creat(usefile, 0666)) < 0) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6,
				"tar: cannot open %s\n")), usefile);
			done(1);
			}
		  }
		Check_9TrackTape(mt); /* Check if device is a nine track tape */

                /* If the user intends to write, make sure the medium is
                 * not write protected.  DSDe412405
                 */
                if (cflag || rflag)
                   if (ioctl(mt,MTIOCGET,&mtget_bfr) != -1)
                      if (GMT_WR_PROT(mtget_bfr.mt_gstat)) {
                        fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,999,
 "tar: cannot write to %s:  write protected\n")), usefile);
                        done(1);
                      }

		dorep(argv);
		close(mt);
	}
	else if (xflag)  {
		if (tflag)
			usage();
		if (oflag && pflag)
			usage();
		if (!(oflag || pflag)) {
			if (geteuid() == 0)
				pflag++;
			else
				oflag++;
		}
		if (strcmp(usefile, "-") == 0) {
			mt = dup(0);
			if (!bflag)
				nblock = 1;
		}
		else
#if defined(SecureWare) && defined(B1)
                if (((ISB1) && ((mt = open(usefile         ,0)) < 0 &&
                    (mt = open((device=usefil1),0)) < 0)) ||
		    ((!ISB1) && ((mt = open(usefile,0)) < 0 &&
		    (mt = open(usefil1,0)) < 0))) {
#else	
		if ((mt = open(usefile,0)) < 0 && (mt = open(usefil1,0)) < 0) {
#endif
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "tar: cannot open %s\n")), usefile);
			done(1);
		}
		Check_9TrackTape(mt); /* Check if device is a nine track tape */
		doxtract(argv);
                close(mt);
	}
	else if (tflag) {
		if (strcmp(usefile, "-") == 0) {
			mt = dup(0);
			if (!bflag)
				nblock = 1;
		}
		else
#if defined(SecureWare) && defined(B1)
                if (((ISB1) && ((mt = open(usefile         ,0)) < 0 &&
                    (mt = open((device=usefil1),0)) < 0)) ||
		    ((!ISB1) && ((mt = open(usefile,0)) < 0 &&
		    (mt = open(usefil1,0)) < 0))) {
#else
		if ((mt = open(usefile,0)) < 0 && (mt = open(usefil1,0)) < 0) {
#endif
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "tar: cannot open %s\n")), usefile);
			done(1);
		}
		Check_9TrackTape(mt); /* Check if device is a nine track tape */
		dotable();
		close(mt);
	}
	else
		usage();
	done(0);
}

usage()
{
#if defined(DISKLESS) || defined(DUX)
#ifdef ACLS
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,7, "tar: usage  tar [-]{txruc}[ONvVwAfblHhm{op}][0-7[lmh]] [tapefile] [blocksize] file1 file2...\n")));	/* For version 1.2 of tar */
#else
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,8, "tar: usage  tar [-]{txruc}[ONvVwfblHhm{op}][0-7[lmh]] [tapefile] [blocksize] file1 file2...\n")));	/* For version 1.2 of tar */
#endif
#else /*  not defined(DISKLESS) || defined(DUX) */
#ifdef SYMLINKS
#ifdef ACLS
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,9, "tar: usage  tar [-]{txruc}[ONvVwAfblhm{op}][0-7[lmh]] [tapefile] [blocksize] file1 file2...\n")));	/* For version 1.2 of tar */
#else
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,10, "tar: usage  tar [-]{txruc}[ONvVwfblhm{op}][0-7[lmh]] [tapefile] [blocksize] file1 file2...\n")));	/* For version 1.2 of tar */
#endif
#else /*  not SYMLINKS  */
#ifdef ACLS
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,11, "tar: usage  tar [-]{txruc}[ONvVwAfblm{op}][0-7[lmh]] [tapefile] [blocksize] file1 file2...\n")));	/* For version 1.2 of tar */
#else
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,12, "tar: usage  tar [-]{txruc}[ONvVwfblm{op}][0-7[lmh]] [tapefile] [blocksize] file1 file2...\n")));	/* For version 1.2 of tar */
#endif
#endif	/*  not SYMLINKS  */
#endif	/*  not defined(DISKLESS) || defined(DUX)  */
	done(1);
}

dorep(argv)
char	*argv[];
{
	register char *cp, *cp2;
	char wdir[DSIZE+1];
	char argbuf[NAMSIZ+PREFIX_SZ+2];
	int full = 0;

#if defined(SecureWare) && defined(B1)
	if (ISB1 && (strcmp(device, "-") != 0)) {
		close(mt);
        	stopio(device);
        	mt = open(device,O_RDWR);
        	ie_check_device(device,AUTH_DEV_SINGLE,AUTH_DEV_EXPORT,mt);
	}
#endif

	if (!cflag) {
		getdir();
		do {
			passtape();
			if (term)
				done(0);
			getdir();
		} while (!endtape());
		if (tfile != NULL) {
			char buf[200];

			/* JLM 8/9/82 Use sort twice instead of awk  */
			sprintf(buf, "sort +0 -1 +1nr -o %s %s; sort -um +0 -1 -o %s %s",
				tname, tname, tname, tname);
			fflush(tfile);
			system(buf);
			freopen(tname, "r", tfile);
                        tmp_offsets = (daddr_t *) malloc( n_tmp_entries * sizeof( daddr_t *) );
                        {
                          char tmpbuf[ MAXPATHLEN + 10 ];
                          int  i;

                          *tmp_offsets = 0;
                          for( i=1; fgets( tmpbuf, MAXPATHLEN+10, tfile ); i++ )
                          {
                           *(tmp_offsets+i) = ftell( tfile );
                          }
                          n_tmp_entries = i-1;
                          rewind( tfile );
                        }

			fstat(fileno(tfile), &stbuf);
		}
	}

	getwdir(wdir);
	if (cflag && !(*argv)) {  /* create archive of no files */
            fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,13, "Attempt to create archive of no files. Nothing dumped.\n")));
	    done(1);
        }
	while (*argv && ! term) {
        /*
         * Remove all trailing '/' characters, but leave the first
         * character alone.
         */
		if (setjmp(jmpbuf))
			break;
		if (strlen(*argv) > NAMSIZ + PREFIX_SZ) {
			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,59, "tar: %s: pathname too long\n")), *argv);
			argv++;
			continue;
		}
		strncpy(argbuf, *argv, sizeof(argbuf)-1);
		cp2 = cp = argbuf;
		cp += strlen(argbuf);
		while (*--cp == '/' && cp != argbuf)
			*cp = '\0';
		if (!strcmp(cp2, "-C") && argv[1]) {
			strcpy(argbuf, *++argv);
#if defined(DISKLESS) || defined(DUX)
			if (Hflag)
				checkhd(argbuf);
#endif
			if (chdir(argbuf) < 0) {
				full = 1;
				fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,76, "tar: Cannot change directory to '%s'\n")), argbuf);
				argv++;
				continue;
			} else {
				full = 0;
				getwdir(wdir);
			}
			argv++;
			continue;
		}
		if (full && *argbuf != '/') {
			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,77, "tar: Path '%s' skipped\n")), argbuf);
			argv++;
			continue;

		}
		for (cp = argbuf; *cp; cp++)
			if (*cp == '/')
				cp2 = cp;
		if (cp2 != argbuf) {
			*cp2 = '\0';
			if (chdir(argbuf)) {
				*cp2 = '/';
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "tar: cannot open %s\n")), argbuf);
				argv++;
				continue;
			}
			*cp2 = '/';
			cp2++;
		}
#ifdef SYMLINKS
		putfile(argbuf, cp2, (struct symbuf *)0);
#else
		putfile(argbuf, cp2);
#endif
		argv++;
#if defined(DISKLESS) || defined(DUX)
		if (Hflag)
			checkhd(wdir);
#endif
		chdir(wdir);
	}
	putempty();
	putempty();
	flushtape();
	if (linkerrok == 1)
		for (; ihead != NULL; ihead = ihead->nextp)
			if (ihead->count != 0)
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,14, "Missing links to %s\n")), ihead->pathname);
}

endtape()
{
	if (dblock.dbuf.name[0] == '\0') {
		backtape();
		return(1);
	}
	else
		return(0);
}

getdir()
{
	register struct stat *sp;
	int i;

	readtape( (char *) &dblock);
	if (dblock.dbuf.name[0] == '\0')
		return;
	sp = &stbuf;
	Tmagic = strcmp(dblock.dbuf.magic, TMAGIC) ? 0 : 1;
	sscanf(dblock.dbuf.mode, "%o", &i);
	sp->st_mode = i;

	/*
	 * Use the gname and uname fields of the
	 * header to generate the gid and uid,
	 * otherwise, use the gid and uid fields
	 */
	if (Tmagic && (gr_ent = getgrnam(dblock.dbuf.gname)) != NULL)
		sp->st_gid = gr_ent->gr_gid;
	else {
		sscanf(dblock.dbuf.gid, "%o", &i);
		sp->st_gid = i;
	}
	if (Tmagic && (pw_ent = getpwnam(dblock.dbuf.uname)) != NULL)
		sp->st_uid = pw_ent->pw_uid;
	else {
		sscanf(dblock.dbuf.uid, "%o", &i);
		sp->st_uid = i;
	}
	sscanf(dblock.dbuf.size, "%lo", &sp->st_size);
	sscanf(dblock.dbuf.mtime, "%lo", &sp->st_mtime);
	sscanf(dblock.dbuf.chksum, "%o", &chksum);
	if (chksum != checksum()) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,15, "directory checksum error\n")));
		done(2);
	}
	if (tfile != NULL) {
		get_name(n_buf);
		fprintf(tfile, "%s %s\n", n_buf, dblock.dbuf.mtime);
		n_tmp_entries++;
	}
}

passtape()
{
	long blocks;
	char buf[TBLOCK];

	if ((dblock.dbuf.typeflag == LNKTYPE) ||
	    (dblock.dbuf.typeflag == CHRTYPE) ||
	    (dblock.dbuf.typeflag == BLKTYPE) )
	       return;

	blocks = stbuf.st_size;
	blocks += TBLOCK-1;
	blocks /= TBLOCK;

	while (blocks-- > 0)
		readtape(buf);
}

#ifdef SYMLINKS
putfile(longname, shortname, symchain)
char *longname;
char *shortname;
struct symbuf *symchain;
#else
putfile(longname, shortname)
char *longname;
char *shortname;
#endif
{
	int infile;
	long blocks;
	char buf[TBLOCK];
	register char *cp, *cp2;
	struct dirent *dp;
	DIR *dirp;
	int i, j;
	char wayback[MAXPATHLEN];       /* Holds path back to parent dir */
	struct stat before, after;      /* Used to compute wayback */
#ifdef SYMLINKS
	int islink = 0;
	struct symbuf *curr_syml, *p;
	char *q;
#endif

#if defined(SecureWare) && defined(B1)
         if((ISB1) && (!ie_sl_export(shortname))) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN, 73, "tar: %s: cannot access file\n")), longname);
                return;
         }
#endif

#if defined(DISKLESS) || defined(DUX)
	if (Hflag)
		checkhd(shortname);
#endif	/*  defined(DISKLESS) || defined(DUX)  */

#ifdef SYMLINKS
	if ((i = lstat(shortname, &stbuf)) == 0 && hflag && S_ISLNK(stbuf.st_mode)) {
		islink = 1;
		if ((curr_syml = (struct symbuf *)malloc(sizeof(struct symbuf))) == (struct symbuf *)0) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,74, "tar: Out of memory. Cannot detect symbolic link loops\n")));
			return;
		}
		getwdir(curr_syml->sympath);
		strcat(curr_syml->sympath, "/");
		strcat(curr_syml->sympath, shortname);
		curr_syml = (struct symbuf *)realloc(curr_syml, (curr_syml->sympath + strlen(curr_syml->sympath) + 1) - (char *)curr_syml);
		curr_syml->previous = symchain;
		curr_syml->next = (struct symbuf *)0;
		if (symchain)
			symchain->next = curr_syml;
		for (p = curr_syml->previous; p; p = p->previous) {
			if (strcmp(curr_syml->sympath, p->sympath) == 0) {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,75, "tar: Loop of symbolic links detected, tar terminated\n")));
				while (p->previous)
					p = p->previous;
				do {
					if (p->previous)
						free(p->previous);
					fprintf(stderr, "\t%s  -->  %.*s\n", p->sympath, readlink(p->sympath, buf, sizeof(buf)), buf);
				} while (p = p->next);
				free(p);
				longjmp(jmpbuf, 1);
			}
		}

   		i = stat(shortname, &stbuf);
	}
#else not SYMLINKS
	i = stat(shortname, &stbuf);
#endif	/*  not SYMLINKS  */
	
	if (i < 0) {

#if defined(DISKLESS) || defined(DUX)
		checkhd(shortname);
		if (stat(shortname, &stbuf) < 0) {
#endif
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,16, "tar: cannot stat %s.  Not dumped.\n")), longname);
		return;

#if defined(DISKLESS) || defined(DUX)
		}  /* DISKLESS */
#endif

	}  /* i < 0 */


	if (tfile != NULL && checkupdate(longname) == 0 && ((stbuf.st_mode & S_IFMT) != S_IFDIR )) {
		return;
	}
	if (checkw('r', longname) == 0) {
		return;
	}

#ifdef ACLS
	if(!aclflag) {
	    if(stbuf.st_acl)
	        fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,17, "Optional acl entries for <%s> not dumped\n")),longname);
	}
#endif	
        switch (stbuf.st_mode & S_IFMT) {
        case S_IFDIR:
		for (i = 0, cp = buf; *cp = longname[i]; cp++, i++);
		if (*(cp-1) != '/') {
			*cp = '/';
			*++cp = '\0';
		}
		if ( !(tfile != NULL && checkupdate(longname) == 0) )
		if (!oflag) {
			stbuf.st_size = 0;
			tomodes(&stbuf);
			if (put_name(buf))
			       return;
			if (Version == NEWV) {
				/* POSIX demands leading zero-fill */
				dblock.dbuf.typeflag = DIRTYPE;
				sprintf(dblock.dbuf.chksum, "%07o", checksum());
			}
			else
				sprintf(dblock.dbuf.chksum, "%6o", checksum());
			writetape( (char *) &dblock);
		}
		if ((dirp = opendir(shortname)) == NULL) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,18, "tar: %s: directory read error\n")), longname);
			return;
		}
#if defined(DUX) || defined(DISKLESS)
                /*
                 * If hidden directories are not explicitly  being
                 * searched,  remember  where  we are before doing
                 * the chdir().
                 *
                 * This  is   necessary   because   our   starting
                 * directory may have been a CDF even though we're
                 * not searching CDFs, and  after  traversing  all
                 * the  sub-directories,  a  simple ".." would not
                 * bring  us  back  to   our   original   starting
                 * directory.
                 */

		if ( ! Hflag ) {
		    stat(".", &before);
		}
#endif /* defined(DUX) || defined(DISKLESS) */

		if (chdir(shortname) < 0) {
		    perror(shortname);
		    return;
		}

#if defined(DUX) || defined(DISKLESS)
                /*
                 * Figure out how to get back  to  where  we  just
                 * were.
                 *
                 * If we are explicitly searching CDFs,
                 * we  first try to see if "..+" will get us back.
                 * If "..+" doesn't exist, use "..".
                 *
                 * If we are not explicitly searching  CDFs,  then
                 * we  use  the  "before/after"  stat(2)  info  to
                 * chdir(2)) directory.
                 */

		strcpy(wayback, "..+");
		if ( Hflag ) {
		     if ( stat(wayback, &after) == -1 ) {
			 wayback[strlen(wayback)-1] = '\0';
		     }
		 } else { /* ( ! Hflag ) */
		     while (1) {
			 if ( stat(wayback, &after) == -1 ) {
			     wayback[strlen(wayback)-1] = '\0';
			     break;
			 }
			 else if ((before.st_ino == after.st_ino) &&
			              (before.st_dev == after.st_dev))
			     break;
			 else strcat(wayback, "/..+");
		     }
		 }
#endif /* defined(DUX) || defined(DISKLESS) */

		while (errno=0, ((dp = readdir(dirp)) != NULL && !term)) {
			if (dp->d_ino == 0)
				continue;
			if (!strcmp(".", dp->d_name) ||
			    !strcmp("..", dp->d_name))
				continue;
			strcpy(cp, dp->d_name);
			i = telldir(dirp);
			closedir(dirp);
#ifdef SYMLINKS
			if (islink) {
				putfile(buf, cp, curr_syml);
			} else
				putfile(buf, cp, symchain);
#else
			putfile(buf, cp);
#endif
			dirp = opendir(".");
			seekdir(dirp, i);
		}
                if( errno && !term )
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,18, "tar: %s: directory read error\n")), longname);
		closedir(dirp);
#ifdef SYMLINKS
		if (islink) {
			strcpy(buf, curr_syml->sympath);
			q = strrchr(buf, '/');
			*q = '\0';
#if defined(DUX) || defined(DISKLESS)
			if (Hflag)
				checkhd(buf);
			else
			        chdir(buf);

#else /* defined(DUX) || defined(DISKLESS) */
				chdir(buf);
#endif /* defined(DUX) || defined(DISKLESS) */
			if (curr_syml->previous)
				curr_syml->previous->next = (struct symbuf *)0;
			free(curr_syml);
		} else
#endif
#if defined(DUX) || defined(DISKLESS)
			chdir(wayback);
#else /* defined(DUX) || defined(DISKLESS) */
			chdir("..");
#endif /* defined(DUX) || defined(DISKLESS) */
		break;

#ifdef SYMLINKS
        case S_IFLNK:
		tomodes(&stbuf);
		if (put_name(longname))
		        return;
		if (stbuf.st_size + 1 >= NAMSIZ) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,19, "tar: %s: symbolic link too long\n")), longname);
			return;
		}
		i = readlink(shortname, dblock.dbuf.linkname, NAMSIZ - 1);
		if (i < 0) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,20, "tar: can't read symbolic link ")));
			perror(longname);
			return;
		}
		dblock.dbuf.linkname[i] = '\0';
		dblock.dbuf.typeflag = SYMTYPE;
		if (vflag)
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,21, "a %1$s symbolic link to %2$s\n")), longname, dblock.dbuf.linkname);
		if (Version == NEWV) {
			/* POSIX demands leading zero-filled numbers */
			sprintf(dblock.dbuf.size, "%011lo", 0);
			sprintf(dblock.dbuf.chksum, "%07o", checksum());
		} else {
			sprintf(dblock.dbuf.size, "%11lo", 0);
			sprintf(dblock.dbuf.chksum, "%6o", checksum());
		}
		(void) writetape((char *)&dblock);
		break;
#endif	/*  SYMLINKS  */

	    case S_IFCHR:	/* Fall through */
	    case S_IFBLK:
		/* Old version doesn't handle char and block devs */
		if (Version == OLDV)
			goto default_l;

		tomodes(&stbuf);

#if defined(DISKLESS) || defined(DUX)
		/*
		 * For char and block, we put the cnode
		 * info in the size field for mkrnod()
		 */
		sprintf(dblock.dbuf.size, "%011lo", stbuf.st_rcnode);
#else
		sprintf(dblock.dbuf.size, "%011lo", 0);
#endif	/*  defined(DISKLESS) || defined(DUX)  */

		if ((stbuf.st_mode & S_IFMT) == S_IFCHR)
			dblock.dbuf.typeflag = CHRTYPE;
		else
			dblock.dbuf.typeflag = BLKTYPE;

		/* add major and minor device numbers to the header */
		sprintf(dblock.dbuf.devmajor, "%07o", major(stbuf.st_rdev));
		sprintf(dblock.dbuf.devminor, "%07o", minor(stbuf.st_rdev));

		if (put_name(longname))
			return;
		sprintf(dblock.dbuf.chksum, "%07o", checksum());
		if (vflag)
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,22, "a %s 0 blocks\n")), longname);
		writetape((char *) &dblock);
		break;
		
	    case S_IFIFO:
		/* Old version doesn't handle fifo types */
		if (Version == OLDV)
			goto default_l;

		tomodes(&stbuf);
		dblock.dbuf.typeflag = FIFOTYPE;
		if ( put_name(longname))
			return;
		sprintf(dblock.dbuf.chksum, "%07o", checksum());
		if (vflag)
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,22, "a %s 0 blocks\n")), longname);
		writetape((char *) &dblock);
		break;

	case S_IFREG:
		if ((infile = open(shortname, 0)) < 0) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,23, "tar: %s: cannot open file\n")), longname);
			return;
		}

		tomodes(&stbuf);
		if (put_name(longname)) {
			close(infile);
			return;
		}
		
		/* See if this is a hard link ( LNKTYPE ) */
		if (stbuf.st_nlink > 1) {
			struct linkbuf *lp;
			int found = 0;

#ifdef SecureWare
			close(infile);
#endif
			for (lp = ihead; lp != NULL; lp = lp->nextp) {
				if (lp->inum == stbuf.st_ino && lp->devnum == stbuf.st_dev) {
					found++;
					break;
				}
			}
			if (found) {
				if (*(lp->pathname) == 0) {
					fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,72, "tar: %s: link to name too long\n")), longname);
				} else {
					/* POSIX demands leading zero-filled numbers */
					/* Changed on 9/30/92 for linkname length = 100 */
					strncpy(dblock.dbuf.linkname, lp->pathname, NAMSIZ);
					dblock.dbuf.typeflag = LNKTYPE;
					if (Version == NEWV) {
						sprintf(dblock.dbuf.size, "%011lo", 0);
						sprintf(dblock.dbuf.chksum, "%07o", checksum());
					} else
						/* OLDV didn't set size field */
						sprintf(dblock.dbuf.chksum, "%6o", checksum());

					writetape( (char *) &dblock);
					if (vflag) {
						fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,24, "a %1$s link to %2$s\n")), longname, lp->pathname);
					}
				}
				lp->count--;
				close(infile);
				return;
			}
			else {
				lp = (struct linkbuf *) malloc(sizeof(*lp));
				if (lp == NULL) {
					if (freemem) {
						fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,25, "Out of memory. Link information lost\n")));
						freemem = 0;
					}
				}
				else {
					lp->nextp = ihead;
					ihead = lp;
					lp->inum = stbuf.st_ino;
					lp->devnum = stbuf.st_dev;
					lp->count = stbuf.st_nlink - 1;
					if (strlen(longname) > NAMSIZ)  /* changed on 9/30/93 */
						*(lp->pathname) = 0;
					else  {
						strncpy(lp->pathname, longname, NAMSIZ);
						lp->pathname[NAMSIZ] = '\0';
                                        }
				}
			}
		}  /* endif for LNKTYPE check */

		blocks = (stbuf.st_size + (TBLOCK-1)) / TBLOCK;
		if (vflag)
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,26, "a %1$s %2$ld blocks\n")), longname, blocks);

		if (Version == NEWV) {
			/* POSIX demands leading zero-filled numbers */
			dblock.dbuf.typeflag = REGTYPE;
			sprintf(dblock.dbuf.chksum, "%07o", checksum());
		} else
			sprintf(dblock.dbuf.chksum, "%6o", checksum());

		writetape( (char *) &dblock);

		while ((i = read(infile, buf, TBLOCK)) > 0 && blocks > 0) {
			writetape(buf);
			blocks--;
		}
		close(infile);
		if (blocks != 0 || i != 0) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,27, "%s: file changed size\n")), longname);

 		if(stbuf.st_fstype == MOUNT_NFS )
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,80, "tar: Cannot read/write over NFS\n")));
		
		}
		while (blocks-- >  0)
			putempty();
		break;

	default:
	default_l:	/* label so I can goto here */
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,28, "tar: %s is not a file. Not dumped\n")), longname);
		break;
	}
}



doxtract(argv)
char	*argv[];
{
	long blocks, bytes;
	char buf[TBLOCK];
	char **cp;
	int ofile;

#if defined(SecureWare) && defined(B1)
	if(ISB1 && (strcmp(device, "-") != 0)) {
        	close(mt);
        	stopio(device);
        	mt = open(device,0);
        	ie_check_device(device,AUTH_DEV_SINGLE,AUTH_DEV_IMPORT,mt);
	}
#endif

	for (;;) {
		getdir();
		if (endtape())
			break;
		get_name(n_buf);
		if (*argv == 0)
			goto gotit;

		for (cp = argv; *cp; cp++)
			if (prefix(*cp, n_buf))
				goto gotit;
		passtape();
		continue;

gotit:
		if (checkw('x', n_buf) == 0) {
			passtape();
			continue;
		}
		if (checkdir(n_buf))
			continue;
		Version = strcmp(dblock.dbuf.magic, TMAGIC) ? OLDV : NEWV;

		switch (dblock.dbuf.typeflag) {
#ifdef SYMLINKS
		    case SYMTYPE:  {	/* Symbolic link */
			struct stat lnstat;
			if (geteuid() == 0) {
				if (lstat(n_buf, &lnstat) == 0) {
				    int empty_directory = 1;
				    if ((lnstat.st_mode & S_IFMT) == S_IFDIR) {
					DIR *dirp;
					struct dirent *dp;
					dirp = opendir(n_buf);
					while ((dp=readdir(dirp)) != NULL) {
						if (!strcmp(dp->d_name,".") ||
						    !strcmp(dp->d_name,".."))
							continue;
						else {
							empty_directory = 0;
							break;
						}
					}
					closedir(dirp);
					if (empty_directory)
						unlink(n_buf);
				} else unlink(n_buf);
			    }
			} else unlink(n_buf);
                        if (symlink(dblock.dbuf.linkname, n_buf)<0) {
                                fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,29, "tar: %s: symbolic link failed: ")), n_buf);
                                perror("");
                                continue;
                        }
                        if (vflag)
                                fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,30, "x %1$s symbolic link to %2$s\n")), n_buf, dblock.dbuf.linkname);
                        continue;	/* Go on to next file */
		}
#endif	/*  SYMLINKS  */
		    case LNKTYPE:  {	/* Hard links */
			struct stat lnstat;
			char   link_name2[NAMSIZ + 1];

			if (geteuid() == 0) {
			   if (stat(n_buf, &lnstat) == 0) {
				int empty_directory = 1;
				if ((lnstat.st_mode & S_IFMT) == S_IFDIR) {
					DIR *dirp;
					struct dirent *dp;
					dirp = opendir(n_buf);
					while ((dp=readdir(dirp)) != NULL) {
						if (!strcmp(dp->d_name,".") ||
						    !strcmp(dp->d_name,".."))
							continue;
						else {
							empty_directory = 0;
							break;
						}
					}
					closedir(dirp);
					if (empty_directory)
						unlink(n_buf);
				} else unlink(n_buf);
			    }
			} else unlink(n_buf);
			strncpy(link_name2, dblock.dbuf.linkname, NAMSIZ);
			link_name2[NAMSIZ] = '\0';
			if (link(link_name2, n_buf) < 0) {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,31, "%s: cannot link\n")), n_buf);
				continue;
			}
			if (vflag)
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,32, "%1$s linked to %2$s\n")), n_buf, link_name2);
			continue;	/* Do next file */
		        }

		    case CHRTYPE: {	/* Character Special device */
		      long maj, min;
		      sscanf(dblock.dbuf.devmajor, "%lo", &maj);
		      sscanf(dblock.dbuf.devminor, "%lo", &min);
#if defined(DISKLESS) || defined(DUX)
		      sscanf(dblock.dbuf.size, "%ho", &stbuf.st_rcnode);
		      if (mkrnod(n_buf,
				 (S_IFCHR | (stbuf.st_mode & 0777)),
				  makedev(maj, min),
				 stbuf.st_rcnode) < 0) 
#else
		      if (mknod(n_buf,
				 (S_IFCHR | (stbuf.st_mode & 0777)),
				  makedev(maj, min)) < 0) 
#endif	/*  defined(DISKLESS) || defined(DUX)  */
			{

			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,33, "tar: %s couldn't create character device\n")),n_buf);
						  continue;
					  }
			if (vflag)
				fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,34, "x %s, 0 bytes, 0 blocks, character device\n")), n_buf);
			break;	/* continue processing this file */
		    }

		    case BLKTYPE: {	/* Block Special device */
		      long maj, min;
		      sscanf(dblock.dbuf.devmajor, "%lo", &maj);
		      sscanf(dblock.dbuf.devminor, "%lo", &min);
#if defined(DISKLESS) || defined(DUX)
		      sscanf(dblock.dbuf.size, "%ho", &stbuf.st_rcnode);
		      if (mkrnod(n_buf,
				 (S_IFBLK | (stbuf.st_mode & 0777)),
				  makedev(maj, min),
				 stbuf.st_rcnode) < 0) 
#else
		      if (mknod(n_buf,
				 (S_IFBLK | (stbuf.st_mode & 0777)),
				  makedev(maj, min)) < 0 )
#endif	/*  defined(DISKLESS) || defined(DUX)  */
			{
			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,35, "tar: %s couldn't create block device\n")),n_buf);
						  continue;
					  }
			if (vflag)
				fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,36, "x %s, 0 bytes, 0 blocks, character device\n")), n_buf);
			break;	/* continue processing this file */
		    }
		    case FIFOTYPE:	/* FIFO */
			if (mknod(n_buf,(S_IFIFO | (stbuf.st_mode & 0777)), 0) < 0) {
				fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,37, "tar: %s couldn't create fifo \n")),n_buf);
				continue;
			}
			if (vflag)
				fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,38, "x %s, 0 bytes, 0 blocks, fifo\n")), n_buf);
			break;	/* finish processing this file */
			
		    case DIRTYPE:
			if (n_buf[strlen(n_buf)-1] == '/') {
				/* Failed to create the directory in check_dir() */
				fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,71, "tar: %s couldn't create directory \n")),n_buf);
				continue;
			}
			if (access(n_buf, 01) < 0) {
				if (mkdir(n_buf, 0777) < 0) {
					perror(n_buf);
					fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,71, "tar: %s couldn't create directory \n")),n_buf);
					continue;
				}

			}
			break;
		    default:		/* All other types treat as regular */
		    case CONTTYPE:	/* Fall through */
		        if (Version == NEWV)
				fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,39, "tar: %s creating as regular file.\n")));
		    case AREGTYPE:	/* Fall through */
		    case REGTYPE:	/* Fall through */
		    /* If there is soft link with same name as the file in
		       archive and there's no -h option delete the softlink
		    */
			if( !hflag )
			{
				struct stat my_buff;
				int res;

				if (!(res = lstat(n_buf, &my_buff)))	{
				if ((my_buff.st_mode & S_IFMT) == S_IFLNK)
					unlink(n_buf);
				}
			}
			if((ofile=creat(n_buf, stbuf.st_mode & 07777)) < 0) {
				fprintf(stderr,
					(catgets(nlmsg_fd,NL_SETN,40, "tar: %s - cannot create\n")), n_buf);
				passtape();
				continue;
			}

#if defined(SecureWare) && defined(B1)
	if (ISB1 && !ie_sl_set_attributes(n_buf)) {
		close(ofile);
		unlink(n_buf);
		passtape();
		continue;
	}
#endif
			/*
			 * Now write the body of the file.
			 */
			blocks = ((bytes = stbuf.st_size) + TBLOCK-1)/TBLOCK;
			if (vflag)
			    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,41, "x %1$s, %2$ld bytes, %3$ld tape blocks\n")) ,n_buf, bytes, blocks);
			while (blocks-- > 0) {
				readtape(buf);
				if (bytes > TBLOCK) {
					if (write(ofile, buf, TBLOCK) < 0) {
						fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,42, "tar: %s: HELP - extract write error\n")), n_buf);
						done(2);
					}
				} else
				if (write(ofile, buf, (unsigned) bytes) < 0) {
					fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,42, "tar: %s: HELP - extract write error\n")), n_buf);
					done(2);
				}
				bytes -= TBLOCK;
			}
			if(close(ofile) <0){
					fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,79, "tar: Cannot close %s\n")), n_buf);
			
			done(2);
			}

			break;	/* Done writing body of file */
		}	/* End of switch */

		if (mflag == 0) {
			time_t timep[2];

			timep[0] = time(NULL);
			timep[1] = stbuf.st_mtime;
			utime(n_buf, timep);
		}


		/*
		 * Do chmod(2) and chown(2) after, instead of before,
		 * we write (& utime(2)) the file because NFS is stateless
		 * and write(2)'s are asynchronous.
		 */

		if (!oflag) {
		    if (Version == NEWV) {
			    /*
			     * Use the gname and uname fields of the
			     * header to generate the gid and uid,
			     * otherwise, use the gid and uid fields
			     *
			     * We may want to cache unames and gnames ?
			     */
			    if ((gr_ent = getgrnam(dblock.dbuf.gname)) != NULL)
			           stbuf.st_gid = gr_ent->gr_gid;
			    if ((pw_ent = getpwnam(dblock.dbuf.uname)) != NULL)
			           stbuf.st_uid = pw_ent->pw_uid;
		    }
		    if ( chmod(n_buf, stbuf.st_mode & 07777) == -1 )
			perror((catgets(nlmsg_fd,NL_SETN,43, "tar: chmod failed")));
#if defined(SecureWare) && defined(B1)
		if (!oflag && (!ISB1 || hassysauth(SEC_OWNER)))
#else
		if (!oflag)
#endif
		    if ( chown(n_buf, stbuf.st_uid, stbuf.st_gid) == -1 )
			perror((catgets(nlmsg_fd,NL_SETN,44, "tar: chown failed")));
                }
	}
}

dotable()
{
#if defined(SecureWare) && defined(B1)
	if(ISB1 && (strcmp(device, "-") != 0)) {
        	close(mt);
        	stopio(device);
        	mt = open(device,0);
        	ie_check_device(device,AUTH_DEV_SINGLE,AUTH_DEV_IMPORT,mt);
	}
#endif
	for (;;) {
		getdir();
		if (endtape())
			break;
		get_name(n_buf);
		Tmagic = strcmp(dblock.dbuf.magic, TMAGIC) ? 0 : 1;
		if (vflag)
			longt(&stbuf, n_buf);
		printf("%s", n_buf);
		if (dblock.dbuf.typeflag == LNKTYPE)
			printf((catgets(nlmsg_fd,NL_SETN,45, " linked to %s")), dblock.dbuf.linkname);
#ifdef SYMLINKS
                if (dblock.dbuf.typeflag == SYMTYPE)
                        printf((catgets(nlmsg_fd,NL_SETN,46, " symbolic link to %s")), dblock.dbuf.linkname);
#endif	/*  SYMLINKS  */
		printf("\n");
		passtape();
	}
}

putempty()
{
	char buf[TBLOCK];
	char *cp;

	for (cp = buf; cp <= buf + TBLOCK - 1; cp++)
		*cp = '\0';
	writetape(buf);
}

char	t[] = { '-', '-', 'l', 'c', 'b', 'd', 'p', '-' };

longt(st, name)
register struct stat *st;
register char *name;
{
	register char *cp;
#ifdef NLS
	char *nl_cxtime();
#else
	char *ctime();
#endif NLS
	
	if (Vflag) {
		/* print out type of file ala ll */
		if (Tmagic)
			/* New format */
			printf("%c",t[dblock.dbuf.typeflag - '0']);
		else
			/* Old format */
			if (name[strlen(name)-1] == '/')
				printf("d");
			else if (dblock.dbuf.typeflag == SYMTYPE)
				printf("l");
			else
				printf("-");
	}
	pmode(st);
	printf(" %3d/%1d", st->st_uid, st->st_gid);

	/* we use the size field for st_rcnode
	   for BLK and CHR types */
	if (Tmagic &&
	    (dblock.dbuf.typeflag == CHRTYPE ||
	     dblock.dbuf.typeflag == BLKTYPE ||
	     dblock.dbuf.typeflag == FIFOTYPE ||
	     dblock.dbuf.typeflag == DIRTYPE))
		stbuf.st_size = 0;
	printf(" %6ld", st->st_size);

#ifdef NLS
	cp = nl_cxtime(&st->st_mtime, (catgets(nlmsg_tfd,NL_SETN,47, "%b %2d %H:%M %Y")));
	printf(" %s ", cp);
#else
	cp = ctime(&st->st_mtime);
	printf(" %-12.12s %-4.4s ", cp+4, cp+20);
#endif NLS
}

int	m1[] = { 1, TUREAD, 'r', '-' };
int	m2[] = { 1, TUWRITE, 'w', '-' };
int	m3[] = { 2, TSUID, 's', TUEXEC, 'x', '-' };
int	m4[] = { 1, TGREAD, 'r', '-' };
int	m5[] = { 1, TGWRITE, 'w', '-' };
int	m6[] = { 2, TSGID, 's', TGEXEC, 'x', '-' };
int	m7[] = { 1, TOREAD, 'r', '-' };
int	m8[] = { 1, TOWRITE, 'w', '-' };
int	m9[] = { 2, TSVTX, 't', TOEXEC, 'x', '-' };

int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

pmode(st)
register struct stat *st;
{
	register int **mp;

	for (mp = &m[0]; mp < &m[9];)
		select(*mp++, st);
}

select(pairp, st)
int *pairp;
struct stat *st;
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n>=0 && (st->st_mode&*ap++)==0)
		ap++;
	printf("%c", *ap);
}

checkdir(name)
register char *name;
{
	register char *cp=name;
	int i, pid;
	for (cp++; *cp; cp++) {	/* start at *name++ */
		if (*cp == '/') {
			*cp = '\0';
			if (access(name, 01) < 0) {
#if defined(DISKLESS) || defined(DUX)
				if ((stbuf.st_mode & S_IFDIR) &&
                                       (stbuf.st_mode & S_CDF))
					cp[-1] = '\0';
#endif	/*  defined(DISKLESS) || defined(DUX)  */
				if (mkdir(name, 0777) < 0) {
					perror(name);
					*cp = '/';
					return(0);
				}

#if defined(DISKLESS) || defined(DUX)
				if ((stbuf.st_mode & S_IFDIR) &&
                                       (stbuf.st_mode & S_CDF)) {
				    chmod(name, stbuf.st_mode & 07777);
				    cp[-1] = '+';
				}
#endif	/*  defined(DISKLESS) || defined(DUX)  */

				if (!oflag) {
				    if (cp[1] == '\0')
		    		        chmod(name, stbuf.st_mode & 07777);
#if defined(SecureWare) && defined(B1)
				    if (ISB1)
					ie_sl_set_attributes(name);
				    if (!ISB1 || hassysauth(SEC_OWNER))
					chown(name, stbuf.st_uid, stbuf.st_gid);
#else
		    		    chown(name, stbuf.st_uid, stbuf.st_gid);
#endif
                		}
			}
			*cp = '/';
		}
	}
	return(cp[-1]=='/');
}

onintr()
{
	signal(SIGINT, SIG_IGN);
	term++;
}

onquit()
{
	signal(SIGQUIT, SIG_IGN);
	term++;
}

onhup()
{
	signal(SIGHUP, SIG_IGN);
	term++;
}

onterm()
{
	signal(SIGTERM, SIG_IGN);
	term++;
}

tomodes(sp)
register struct stat *sp;
{
	register char *cp;

	/*
	 * Note: The order in which these fields is processed is
	 *       important. Some fields must not end in a Null and
	 *       this is accomplished by overwriting the Null in the
	 *       processing of the next field.
	 */
	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		*cp = '\0';

	if (Version == NEWV) {
		strncpy(dblock.dbuf.version, TVERSION, TVERSLEN);
		if ((pw_ent = getpwuid(sp->st_uid)) == NULL)
			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,48, "tar: couldn't get uname for uid %d\n")),sp->st_uid);
		else
			strcpy(dblock.dbuf.uname, pw_ent->pw_name);
		if ((gr_ent = getgrgid(sp->st_gid)) == NULL)
			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,49, "tar: couldn't get gname for gid %d\n")),sp->st_gid);
		else
			strcpy(dblock.dbuf.gname, gr_ent->gr_name);
		strcpy(dblock.dbuf.magic, TMAGIC);
		sprintf(dblock.dbuf.mode, "%07o", sp->st_mode & 077777);
		sprintf(dblock.dbuf.uid, "%07o", sp->st_uid);
		sprintf(dblock.dbuf.gid, "%07o", sp->st_gid);
		sprintf(dblock.dbuf.size, "%011lo", sp->st_size);
		sprintf(dblock.dbuf.mtime, "%011lo", sp->st_mtime);
		sprintf(dblock.dbuf.devmajor, "%07o", 0);
		sprintf(dblock.dbuf.devminor, "%07o", 0);
	} else {
		/* OLDV does not have leading zero-fill */
		sprintf(dblock.dbuf.mode, "%6o ", sp->st_mode & 077777);
		sprintf(dblock.dbuf.uid, "%6o ", sp->st_uid);
		sprintf(dblock.dbuf.gid, "%6o ", sp->st_gid);
		sprintf(dblock.dbuf.size, "%11lo ", sp->st_size);
		sprintf(dblock.dbuf.mtime, "%11lo ", sp->st_mtime);
	}
}

checksum()
{
	register i;
	register char *cp;

	for (cp = dblock.dbuf.chksum; cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)]; cp++)
		*cp = ' ';
	i = 0;
	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		i += *cp;
	return(i);
}

checkw(c, name)
char *name;
{
	if (wflag) {
		printf("%c ", c);
		if (vflag)
			longt(&stbuf, name);
		printf("%s: ", name);
		if (response() == 'y'){
			return(1);
		}
		return(0);
	}
	return(1);
}

response()
{
	char c;

	c = getchar();
	if (c != '\n')
		while (getchar() != '\n');
	else c = 'n';
	return(c);
}

checkupdate(arg)
char	*arg;
{
	long	mtime;
	daddr_t seekp;
	daddr_t	lookup();

	rewind(tfile);
	for (;;) {
		if ((seekp = lookup(arg)) < 0)
			return(1);
		fseek(tfile, seekp, 0);
		/* get time (skip name field) */
		fscanf(tfile, "%*s %lo", &mtime);
		if (stbuf.st_mtime > mtime)
			return(1);
		else
			return(0);
	}
}

done(n)
{
	unlink(tname);
	exit(n);
}

prefix(s1, s2)
register char *s1, *s2;
{
	while (*s1)
		if (*s1++ != *s2++)
			return(0);
	if (*s2)
		return(*s2 == '/');
	return(1);
}

getwdir(s)
char *s;
{
	extern char *getcwd();

	if (getcwd(s, DSIZE) == (char *)0) {
		if (errno == ENAMETOOLONG)
			fprintf(stderr, catgets(nlmsg_fd,NL_SETN,52, "tar: pwd depth limit exceeded!\n"));
		else
			perror(catgets(nlmsg_fd,NL_SETN,51, "tar: pwd failed"));
	}
}

#define	N	200
daddr_t
lookup(s)
char *s;
{
	register i;
	daddr_t a;

	for(i=0; s[i]; i++)
		if(s[i] == ' ')
			break;
	a = bsrch(s, i);
	return(a);
}

/*-------------------------------------------------*/

int ll;
char *line;
char tmpbuff[ MAXPATHLEN + 10 ];

/*-------------------------------------------------*/
int comp_tmp( a, b )

daddr_t *a, *b;
{
 if( a == (daddr_t *) line && b != (daddr_t *) line )
 {
  fseek( tfile, *b, 0 );
  fgets( tmpbuff, MAXPATHLEN + 9, tfile );
  return strncmp( a, tmpbuff, ll);
 }
 if( a != (daddr_t *) line && b == (daddr_t *) line )
 {
  fseek( tfile, *a, 0 );
  fgets( tmpbuff, MAXPATHLEN + 9, tfile );
  return strncmp( tmpbuff, b, ll);
 }
}
/*-------------------------------------------------*/
daddr_t bsrch( str, leng )

char   *str;
int    leng;

{
 daddr_t *offset;
 ll = leng;
 line = str;

 offset = (daddr_t *)bsearch( str, &(*tmp_offsets), n_tmp_entries, sizeof(daddr_t), comp_tmp );

 return offset ? *offset : ( daddr_t ) -1;
}
/*-------------------------------------------------*/
readtape(buffer)
char *buffer;
{
	register int i;
	static int changed = 0;

	if (recno >= nblock || first == 0) {
		if ((first == 0) && !bflag && nine_tt)
			i = read(mt, (char *)tbuf, TBLOCK*NBLOCK);
		else
			i = bread(mt, (char *)tbuf, TBLOCK*nblock);
		if (i < 0)	{
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,53, "Tar: tape read error\n")));
			done(3);
		}
		if (first == 0) {
			if ((i % TBLOCK) != 0) {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,54, "Tar: tape blocksize error\n")));
				done(3);
			}
			i /= TBLOCK;
			if (i != nblock) {
				if (i == 0) {
					fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,55, "Tar: blocksize = 0; broken pipe?\n")));
					done(3);
				}
				if (i < nblock)
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,56, "Tar: blocksize = %d\n")), i);
				nblock = i;
			}
		}
		else if (i != TBLOCK*nblock) {
		                if(!changed)
				    changed = 1;
				else {
		                    fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,57, "Tar: error! blocksize changed\n")));
				    done(3);
				}
	        }
		recno = 0;
	}
	first = 1;
	copy(buffer, &tbuf[recno++]);
	return(TBLOCK);
}

writetape(buffer)
char *buffer;
{
	first = 1;
	if (nblock == 0)
		nblock = 1;
	if (recno >= nblock) {
		if (tape_io(tbuf, TBLOCK*nblock, TIO_WRITE) < 0) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,58, "Tar: tape write error\n")));
			done(2);
		}
		recno = 0;
	}
	copy(&tbuf[recno++], buffer);
	if (recno >= nblock) {
		if (tape_io(tbuf, TBLOCK*nblock, TIO_WRITE) < 0) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,58, "Tar: tape write error\n")));
			done(2);
		}
		recno = 0;
	}
	return(TBLOCK);
}

extern int errno;
/*
 *	Multi-volume support for POSIX.1
 *	Do the real tape i/o. If end of tape is encountered,
 *	then call chgreel() to change the reels. Then finish the i/o
 *	on the current buffer.
 */
tape_io(buffer, size, flag)
char *buffer;
int size;
int flag;	/* TIO_READ if we are reading, TIO_WRITE if writing */
{
	register int wc;	/* bytes written during last i/o */
	register int out=0;	/* total bytes of i/o */

	if (flag == TIO_WRITE)
		wc = write(mt, buffer, size);
	else
		wc = read(mt, buffer, size);

	if (wc != size) {
		if (first == 0)
			return(wc);
		/*
		 * we didn't read/write a full buffer
		 * either a tape error or we need
		 * to change tapes.
		 */
		if (wc < 0 && flag == TIO_READ) {
			/* error */
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,63, "Tar: tape error (%d)\n")),errno);
			exit(2);
		} else
			out += (wc>0) ? wc : 0;	/* increment only if positive */
		/*
		 * change reels and finish writing current
		 * buffer from where we had to change the reel.
		 * (POSIX does not allow us to duplicate data across
		 * volumes.
		 */
		while (out < size) {
			if (flag == TIO_WRITE) {
			/* for 'r' option open device with O_RDWR */
				mt = (rflag ? chgreel(O_RDWR)
					    :chgreel(O_WRONLY));
				wc = write(mt, (buffer+out), size-out);
			} else {
				mt = (rflag ? chgreel(O_RDWR)
					    :chgreel(O_RDONLY));
				wc = read(mt, (buffer+out), size-out);
			}
			if (wc < 0)
				fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,64, "Tar: error on new volume\n")));
			else
				out+=wc;
		}
	}
	return size;
}

backtape()
{
	static int mtdev = 1;
	static struct mtop mtop = {MTBSR, 1};
	struct mtget mtget;

	if (mtdev == 1)
		mtdev = ioctl(mt, MTIOCGET, &mtget);
	if (mtdev != -1) {
		if (ioctl(mt, MTIOCTOP, &mtop) < 0) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,59, "Tar: tape backspace error\n")));
			done(4);
		}
	} else
		lseek(mt, (long) -TBLOCK*nblock, 1);
	recno--;
}

flushtape()
{
	if (recno > 0) {
		if (tape_io(tbuf, TBLOCK*nblock, TIO_WRITE) < 0) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,58, "Tar: tape write error\n")));
			done(2);
		}
	}
}

copy(to, from)
register char *to, *from;
{
	register i;

	i = TBLOCK;
	do {
		*to++ = *from++;
	} while (--i);
}

bread(fd, buf, size)
	int fd;
	char *buf;
	int size;
{
	int count;
	static int lastread = 0;

	if(special)
		/* we are NOT reading from FIFO, SOCKET, or stdin */
		return(tape_io(buf, size, TIO_READ));

	for (count = 0; count < size; count += lastread) {
		lastread = read(fd, buf, size - count);
		if (lastread <= 0) {
			if (count > 0)
				return (count);
			return (lastread);
		}
		buf += lastread;
	}
	return (count);
}


#if defined(DISKLESS) || defined(DUX)
void
checkhd(name)
char *name;
{
	struct stat sb;

	strcat(name, "+");
	if (stat(name, &sb) == 0 && S_ISCDF(sb.st_mode))
			return;
	name[strlen(name)-1] = '\0';
}
#endif	/*  defined(DISKLESS) || defined(DUX)  */

/*
 *  Write the name of the file into the header. If the -new
 *  flag is set (Version = NEWV) we write it according to P1003.1
 *  (POSIX). If -old option was used (Version = OLDV) we write the name
 *  in the old way (just using the name field of the header).
 *  Return 0 on success, 1 on failure
 */
put_name(longname)
char *longname;
{
	char *cp3;
	int  nsize;

	nsize = strlen(longname);
	if (Version == NEWV) {
		/*
		 *  Write a new (POSIX) format name.
		 *  Put the filename into the header, splitting it between
		 *  the prefix and name fields if necessary. P1003.1
		 *  states that if the filename is split, only the
		 *  trailing component name goes into the name field,
		 *  the rest except '/' goes into the  prefix field .
		 */
		if (nsize > NAMSIZ + PREFIX_SZ) {
			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,59, "tar: %s: pathname too long\n")),
				longname);
			return (1);
		}
		if (nsize <= NAMSIZ)
			strncpy(dblock.dbuf.name, longname, NAMSIZ);
		else {
			/* Path too long for name field, so split it. */
			
			/* Find the trailing component of the pathname */
			cp3 = longname + nsize -2;
			while (*cp3 != '/' && cp3 > longname)
				cp3--;
			
			/* cp3 now points to the '/' that separates
			 * the filename from the rest of the path
			 */
			if ((cp3 - longname) > PREFIX_SZ) {
				fprintf(stderr,
					(catgets(nlmsg_fd,NL_SETN,60, "tar: %s: prefix too long\n")),
					longname);
				return (1);
			}
			if ((nsize - (cp3 - longname + 1)) > NAMSIZ) {
				fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,61, "tar: %s: file name too long\n")),longname);
				return (1);
			}
			*cp3++ = '\0';
			strncpy(dblock.dbuf.prefix, longname, PREFIX_SZ);
			strncpy(dblock.dbuf.name, cp3--, NAMSIZ);
			*cp3 = '/';  /* restore '/' now that we are done */
		}
	} else {	/* Old format -- Just stuff it in name field */
		if (nsize > NAMSIZ) {
			/*  For strict compatibility, don't print the trailing '/' that
			 *  we add to directory names for POSIX
			 */
			if (*(longname+nsize-1) == '/')
				*(longname+nsize-1) = '\0';
			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,62, "%s: file name too long\n")), longname);
			return (1);
		}
		strcpy(dblock.dbuf.name, longname);
	}
	return (0);	/* succesful return */
}

/*
 *  Extract the filename from the header.
 */
get_name(name_buf)
char *name_buf;
{
	/*
	 * Create name from name and prefix fields
	 * of the header and return in name_buf.
	 * The name and prefix fields may or may not be
	 * null terminated; strncat() puts on the null for us
	 * but we put it on for strncpy()
	 */
	if (Tmagic && *dblock.dbuf.prefix) {
		/* prefix is not NULL so start with it */
		strncpy(name_buf, dblock.dbuf.prefix, PREFIX_SZ);
		name_buf[PREFIX_SZ] = NULL;
		strcat(name_buf, "/");
		strncat(name_buf, dblock.dbuf.name, NAMSIZ);
	} else {
		/* No prefix */
		strncpy(name_buf, dblock.dbuf.name, NAMSIZ);
		name_buf[NAMSIZ] = NULL;
	}
}

/*
 * code from cpio modified a bit
 * Announce that the current tape is done and
 * prompt for another one. Try to open and go
 * on with the archive.
 */
chgreel(flag)
int flag;	/* flag to open O_RDONLY or O_WRONLY */
{
	register f;		/* value of open call for device/file */
/* Max length of name of device file user can enter ( fix for DSDe409313 )*/
#define MAX_NEW_NAME   130
	char str[MAX_NEW_NAME];
	FILE *devtty, *devttyout;	/* for terminal device output */
	struct stat statb;
	int  Eotmsg = 0;   /* This is set to 1 for 3480. */

        if (Eotmsg = is3480(mt))
		if ((f = load_next_reel(mt,flag)) != -1)
			return(f); /* successful change of reel in auto mode. */

	/* open dev/tty, or die with message to stderr */ 	
	devtty = fopen("/dev/tty", "r");
	devttyout = fopen("/dev/tty", "w");

	if ((devtty == NULL) || (devttyout == NULL)) {
	fputs((catgets(nlmsg_fd,NL_SETN,65, "Can't open /dev/tty to prompt for more media.\n")), stderr);
			exit(2);
	}

	/* print end-of-reel message to dev/tty */
	/* Don't print this message for a 3480. */
	if (!Eotmsg) {
		fprintf(devttyout,(catgets(nlmsg_fd,NL_SETN,66, "Tar: end of tape\n")));
		fflush(devttyout);
	}

	/* close the original raw file (ie, first reel) */
	close(mt);

 again:
    fputs((catgets(nlmsg_fd,NL_SETN,67, "Tar: to continue, enter device/file name when ready or null string to quit.\n")), devttyout);
	fflush(devttyout);

	/* read new name from dev/tty */
	fgets(str, MAX_NEW_NAME-1 , devtty);
	str[strlen(str) - 1] = '\0';

	/* if null name, quit with message to stderr */
	if (!*str) {
	fputs((catgets(nlmsg_fd,NL_SETN,68, "User entered a null name for next device file.\n")), stderr);
		exit(2);
	}

	/* try new name, notify dev/tty if a problem */
	if ((f = open(str, flag)) < 0) {
		/* open failed */
		fprintf(devttyout,(catgets(nlmsg_fd,NL_SETN,69, "Tar: couldn't open %s\n")),str);
		goto again;
	}

	/* close tty, log reel change, return raw file descriptor after good
	   open */
	fclose(devtty);
	fclose(devttyout);
	fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,70,"User opened file %s to continue.\n")), str);
	return f;
}

#if defined(SecureWare) && defined(B1)

/*
 * The following functions are dummies to satisfy references in
 * export_sec.c.  They are unused by tar.
 */

void
readhdr()
{
}

void
writehdr()
{
}

void
bwrite()
{
}

#endif

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
		   mtget_buf.mt_type == MT_IS3480)
			return(1);
	return(0);
}

/* load_next_reel() attempts to load the next cartridge. It returns
 * the file descriptor of the open tape device as the return value
 * if successful, otherwise returns -1.
 */
load_next_reel(mt,flag)
int mt;
int flag;
{
	int f;
	struct mtop mt_com;

	if (flag == TIO_WRITE) {
		mt_com.mt_op = MTWEOF; /* Write 2 filemarks at end of tape */
		mt_com.mt_count = 2;
		if (ioctl(mt, MTIOCTOP, &mt_com) < 0 ) {
			fprintf(stderr, catgets(nlmsg_fd,NL_SETN,86,"Tar: ioctl to write filemarks failed (%d). aborting...\n"), errno);
			done(4);
		}
	}

	mt_com.mt_op = MTOFFL;
	mt_com.mt_count = 1;
        if ( ioctl(mt, MTIOCTOP, &mt_com) < 0 ) {
		fprintf(stderr, catgets(nlmsg_fd,NL_SETN,82,"Tar: ioctl to offline device failed. aborting...\n"));
		done(4);
	}  

/*
 * The previous ioctl may put the 3480 device offline in the following cases:
 * 1. End of Magazine (stacker) condition in auto mode.
 * 2. Device in manual/system mode.
 */

/* Check if cartridge is loaded to the drive. */
	if (DetectTape(mt)) {
		fprintf(stderr, catgets(nlmsg_fd,NL_SETN,83,"Tar: auto loaded next media.\n"));
		return(mt);
	} else {
		fprintf(stderr, catgets(nlmsg_fd,NL_SETN,84,"Tar: unable to auto load next media.\n"));
		return(-1);
	}
}

/* This function checks if a cartridge is loaded into the 3480 drive. */
DetectTape(mt)
{
	struct mtget mtget;
/*
 *	NOTE:  The code below is commented out due to the 3480 driver
 *	returning an error if the device is offline instead of allowing
 *	ioctl to return an OFFILE indication. (DLM 7/21/93)
 *
 *	if ( ioctl(mt, MTIOCGET, &mtget) < 0 ) {
 *		fprintf(stderr, catgets(nlmsg_fd,NL_SETN,85,"Tar: ioctl to determine device online failed. aborting...\n"));
 *		done(4);
 *	}
 */

	if ((ioctl(mt, MTIOCGET, &mtget) < 0) ||
	    (!GMT_ONLINE(mtget.mt_gstat))) return(0);
	else return(1);
}
