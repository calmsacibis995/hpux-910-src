/* @(#)  $Revision: 70.6 $ */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "restore.h"
#ifdef __hpux
#  include <string.h>
#  include <varargs.h>
#  include "dumprestore.h"
#else /* __hpux */
#  include <protocols/dumprestore.h>
#endif /* __hpux */
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/file.h>
#include <setjmp.h>
#include <sys/stat.h>

/*
 * 
 * Uncomment the following to enable debug printing 
 *
 * #define DEBUG 1
 *
 * extern FILE	*dbgstream;
 */

#define	MAXTAPES	128

static long	fssize = MAXBSIZE;
static int	mt = -1;
static int	pipein = 0;
static char	magtape[BUFSIZ];
static int	bct;
static char	*tbf;
static union	u_spcl endoftapemark;
static long	blksread;
static u_char	tapesread[MAXTAPES];
static jmp_buf	restart;
static int	gettingfile = 0;	/* restart has a valid frame */

static int	ofile;
static char	*map;
static char	lnkbuf[MAXPATHLEN + 1];
#ifdef __hpux
static char	netbuf[MAXPATHLEN + 1];
#endif /* __hpux */
static int	pathlen;

#if defined(TRUX) && defined(B1)
short   Lflag, Rflag, Cflag;
short   Aflag, Pflag ;
#endif /* (TRUX) && defined(B1) */

int		Bcvt;		/* Swap Bytes (for CCI or sun) */
static int	Qcvt;		/* Swap quads (for sun) */
#ifdef __hpux
static char     *host;          /* The name of remoto host. */
struct  stat *rmtfilestatus();
int  rmterrmsg;
int  rmtregfile = 0;       /* This flag indicates if the specified file is   */
                           /* regular file or special file on remote machine.*/
#endif /* __hpux */

#ifdef OLD_RFA
/*
 * When OLD_RFA is defined, we still undertand network special files,
 * but print a warning message indicating that they aren't being
 * restored.
 */
#ifndef IFNWK
#define IFNWK 0110000 /* network special file */
#endif /* not IFNWK */
#endif /* OLD_RFA */

/*
 * Set up an input source
 */
setinput(source)
	char *source;
{
	extern struct mtget *rmtstatus();
#ifdef __hpux
	struct          stat *rmtstatbuf;
        char *tape, *cp;
#else /* __hpux */
#ifdef RRESTORE
	char *host, *tape;
#endif /* RRESTORE */
#endif /* __hpux */

	flsht();
	if (bflag)
		newtapebuf(ntrec);
	else
		newtapebuf(NTREC > HIGHDENSITYTREC ? NTREC : HIGHDENSITYTREC);
	terminal = stdin;
#ifdef __hpux
	if (strchr(source, ':') != NULL) {
	    if ((host = strtok(source, ":")) != NULL) {
                 tape = strtok((char *) NULL, ":");
		 if (tape == 0) {
nohost:
		     msg("need keyletter ``f'' and device ``host:tape''\n");
		     done(1);
		 }
		 (void) strcpy(magtape, tape);
#if defined(TRUX) && defined(B1)
		/* broke in IC3 
		rem_host = m6getrhnam(host);
		if (rem_host->host_type == M6CON_UNLABELED )
			msg("\nError: remote host %s is not a trusted host\n\n", host);
			done(1);	*/
#endif /* (TRUX) && (B1) */
		 if (rmthost(host, 'R') == 0)
		     done(1);
		 setuid(getuid());   /* rmthost() is the only reason to be setuid */
		 rmterrmsg = FALSE;
		 rmtopen(tape,0);
		 if (rmtstatus() == (struct mtget *)NULL) {
		     rmterrmsg = TRUE;
		     if ((rmtopen(tape, 0) < 0) || (rmtstatbuf=rmtfilestatus()) == NULL) {
			 msg("cannot open %s:%s\n", host, tape);
			 done(1);
		     } else if ((rmtstatbuf->st_mode & S_IFMT) == S_IFREG)
			 rmtregfile = TRUE;
		 }
		 rmterrmsg = TRUE;
	     }
	} else {
	    if (strcmp(source, "-") == 0) {
		/*
		 * Since input is coming from a pipe we must establish
		 * our own connection to the terminal.
		 */
		terminal = fopen("/dev/tty", "r");
		if (terminal == NULL) {
		    perror("Cannot open(\"/dev/tty\")");
		    terminal = fopen("/dev/null", "r");
		    if (terminal == NULL) {
			perror("Cannot open(\"/dev/null\")");
			done(1);
		    }
		}
		pipein++;
	    }
	    (void) strcpy(magtape, source);
	}
#else /* __hpux */
#ifdef RRESTORE
	host = source;
	tape = index(host, ':');
	if (tape == 0) {
nohost:
		msg("need keyletter ``f'' and device ``host:tape''\n");
		done(1);
	}
	*tape++ = '\0';
	(void) strcpy(magtape, tape);
	if (rmthost(host, 'R') == 0)
		done(1);
	setuid(getuid());	/* no longer need or want root privileges */
#else /* RRESTORE */
	if (strcmp(source, "-") == 0) {
		/*
		 * Since input is coming from a pipe we must establish
		 * our own connection to the terminal.
		 */
		terminal = fopen("/dev/tty", "r");
		if (terminal == NULL) {
			perror("Cannot open(\"/dev/tty\")");
			terminal = fopen("/dev/null", "r");
			if (terminal == NULL) {
				perror("Cannot open(\"/dev/null\")");
				done(1);
			}
		}
		pipein++;
	}
	(void) strcpy(magtape, source);
#endif /* RRESTORE */
#endif /* __hpux */
}

newtapebuf(size)
	long size;
{
	static tbfsize = -1;

	ntrec = size;
	if (size <= tbfsize)
		return;
	if (tbf != NULL)
		free(tbf);
	tbf = (char *)malloc(size * TP_BSIZE);
	if (tbf == NULL) {
		fprintf(stderr, "Cannot allocate space for tape buffer\n");
		done(1);
	}
	tbfsize = size;
}

/*
 * Verify that the tape drive can be accessed and
 * that it actually is a dump tape.
 */
setup()
{
	int i, j, *ip;
	struct stat stbuf;
	extern char *ctime();
	extern int xtrmap(), xtrmapskip();

	vprintf(stdout, "Verify tape and initialize maps\n");
#ifdef __hpux
#if defined(TRUX) && defined(B1)
	forcepriv(SEC_ALLOWMACACCESS); /* to be on the safe side */
	forcepriv(SEC_ALLOWDACACCESS);
#endif /*(TRUX) && (B1) */
	if ( host ) {
	    if ((mt = rmtopen(magtape, 0)) < 0) {
		perror(magtape);
		done(1);
	    }
        } else {
	    if (pipein)
		mt = 0;
	    else {
		if ((mt = open(magtape, 0)) < 0) {
		    perror(magtape);
		    done(1);
		}
#if defined(TRUX) && defined(B1)
		if (mt !=0){
		   vprintf(stdout, "Checking the input device security... \n");
                   check_device(magtape, AUTH_DEV_MULTI, AUTH_DEV_IMPORT,mt);
		   vprintf(stdout, "Completed device checking\n");
		}
#endif /*(TRUX) && (B1) */
	    }
	}
#else /* __hpux */
#ifdef RRESTORE
	if ((mt = rmtopen(magtape, 0)) < 0)
#else /* RRESTORE */
	if (pipein)
		mt = 0;
	else if ((mt = open(magtape, 0)) < 0)
#endif /* RRESTORE */
	{
		perror(magtape);
		done(1);
	}
#endif /* __hpux */
	volno = 1;
	setdumpnum();
	flsht();
	if (!pipein && !bflag)
		findtapeblksize();
	if (gethead(&spcl) == FAIL) {
#if defined(TRUX) && defined(B1)
		vprintf(stdout, "Tape was not made on a BLS system\n");
		(void) fflush(stderr);
		(void) fflush(stdout);
		done(1);
#endif /* (TRUX) && (B1) */
		bct--; /* push back this block */
		cvtflag++;
		if (gethead(&spcl) == FAIL) {
			fprintf(stderr, "Tape is not a dump tape\n");
			done(1);
		}
		fprintf(stderr, "Converting to new file system format.\n");
	}
	if (pipein) {
#if defined(TRUX) && defined(B1)
		endoftapemark.s_spcl.c_magic = it_is_b1fs ? BLS_B1_MAGIC : BLS_C2_MAGIC;
#else /* (TRUX) && (B1) */
		endoftapemark.s_spcl.c_magic = cvtflag ? OFS_MAGIC : NFS_MAGIC;
#endif /* (TRUX) && (B1) */
		endoftapemark.s_spcl.c_type = TS_END;
		ip = (int *)&endoftapemark;
		j = sizeof(union u_spcl) / sizeof(int);
		i = 0;
		do
			i += *ip++;
		while (--j);
		endoftapemark.s_spcl.c_checksum = CHECKSUM - i;
	}
	if (vflag || command == 't') {
		fprintf(stdout, "Dump   date: %s", ctime(&spcl.c_date));
		fprintf(stdout, "Dumped from: %s",
			(spcl.c_ddate == (time_t)0) ? "the epoch\n" : ctime(&spcl.c_ddate));
	}
	dumptime = spcl.c_ddate;
	dumpdate = spcl.c_date;
	if (stat(".", &stbuf) < 0) {
		perror("cannot stat .");
		done(1);
	}
	if (stbuf.st_blksize > 0 && stbuf.st_blksize <= MAXBSIZE)
		fssize = stbuf.st_blksize;
	if (((fssize - 1) & fssize) != 0) {
		fprintf(stderr, "bad block size %d\n", fssize);
		done(1);
	}
	if (checkvol(&spcl, (long)1) == FAIL) {
		fprintf(stderr, "Tape is not volume 1 of the dump\n");
		done(1);
	}
	if (readhdr(&spcl) == FAIL)
		panic("no header after volume mark!\n");
	findinode(&spcl, 1);
	if (checktype(&spcl, TS_CLRI) == FAIL) {
		fprintf(stderr, "Cannot find file removal list\n");
		done(1);
	}
	maxino = (spcl.c_count * TP_BSIZE * NBBY) + 1;
	dprintf(stdout, "maxino = %d\n", maxino);
	map = calloc((unsigned)1, (unsigned)howmany(maxino, NBBY));
	if (map == (char *)NIL)
		panic("no memory for file removal list\n");
	clrimap = map;
	curfile.action = USING;
	getfile(xtrmap, xtrmapskip);
	if (checktype(&spcl, TS_BITS) == FAIL) {
		fprintf(stderr, "Cannot find file dump list\n");
		done(1);
	}
	map = calloc((unsigned)1, (unsigned)howmany(maxino, NBBY));
	if (map == (char *)NULL)
		panic("no memory for file dump list\n");
	dumpmap = map;
	curfile.action = USING;
	getfile(xtrmap, xtrmapskip);
}

/*
 * Prompt user to load a new dump volume.
 * "Nextvol" is the next suggested volume to use.
 * This suggested volume is enforced when doing full
 * or incremental restores, but can be overrridden by
 * the user when only extracting a subset of the files.
 */
getvol(nextvol)
	long nextvol;
{
	long newvol;
	long savecnt, i;
	union u_spcl tmpspcl;
#	define tmpbuf tmpspcl.s_spcl

	if (nextvol == 1) {
		for (i = 0;  i < MAXTAPES;  i++)
			tapesread[i] = 0;
		gettingfile = 0;
	}
	if (pipein) {
		if (nextvol != 1)
			panic("Changing volumes on pipe input?\n");
		if (volno == 1)
			return;
		goto gethdr;
	}
	savecnt = blksread;
again:
	if (pipein)
		done(1); /* pipes do not get a second chance */
	if (command == 'R' || command == 'r' || curfile.action != SKIP)
		newvol = nextvol;
	else
		newvol = 0;
	while (newvol <= 0) {
		int n = 0;

		for (i = 0;  i < MAXTAPES;  i++)
			if (tapesread[i])
				n++;
		if (n == 0) {
			fprintf(stderr, "%s%s%s%s%s",
			    "You have not read any tapes yet.\n",
			    "Unless you know which volume your",
			    " file(s) are on you should start\n",
			    "with the last volume and work",
			    " towards the first.\n");
		} else {
			fprintf(stderr, "You have read volumes");
			strcpy(tbf, ": ");
			for (i = 0; i < MAXTAPES; i++)
				if (tapesread[i]) {
					fprintf(stderr, "%s%d", tbf, i+1);
					strcpy(tbf, ", ");
				}
			fprintf(stderr, "\n");
		}
		do	{
			fprintf(stderr, "Specify next volume #: ");
			(void) fflush(stderr);
			(void) fgets(tbf, BUFSIZ, terminal);
		} while (!feof(terminal) && tbf[0] == '\n');
		if (feof(terminal))
			done(1);
		newvol = atoi(tbf);
		if (newvol <= 0) {
			fprintf(stderr,
			    "Volume numbers are positive numerics\n");
		}
		if (newvol > MAXTAPES) {
			fprintf(stderr,
			    "This program can only deal with %d volumes\n",
			    MAXTAPES);
			newvol = 0;
		}
	}
	if (newvol == volno) {
		tapesread[volno-1]++;
		return;
	}
	closemt();
	fprintf(stderr, "Mount tape volume %d\n", newvol);
	fprintf(stderr, "then enter tape name (default: %s) ", magtape);
	(void) fflush(stderr);
	(void) fgets(tbf, BUFSIZ, terminal);
	if (feof(terminal))
		done(1);
	if (tbf[0] != '\n') {
		(void) strcpy(magtape, tbf);
		magtape[strlen(magtape) - 1] = '\0';
	}
#ifdef __hpux
	if ( host ) {
	    if ((mt = rmtopen(magtape, 0)) == -1) {
		fprintf(stderr, "Cannot open %s\n", magtape);
		volno = -1;
		goto again;
	    }
	} else {
	    if ((mt = open(magtape, 0)) == -1) {
		fprintf(stderr, "Cannot open %s\n", magtape);
		volno = -1;
		goto again;
	    }
	}
#else /* __hpux */
#ifdef RRESTORE
	if ((mt = rmtopen(magtape, 0)) == -1)
#else /* RRESTORE */
	if ((mt = open(magtape, 0)) == -1)
#endif /* RRESTORE */
	{
		fprintf(stderr, "Cannot open %s\n", magtape);
		volno = -1;
		goto again;
	}
#endif /* __hpux */
gethdr:
	volno = newvol;
	setdumpnum();
	flsht();
	if (readhdr(&tmpbuf) == FAIL) {
		fprintf(stderr, "tape is not dump tape\n");
		volno = 0;
		goto again;
	}
	if (checkvol(&tmpbuf, volno) == FAIL) {
		fprintf(stderr, "Wrong volume (%d)\n", tmpbuf.c_volume);
		volno = 0;
		goto again;
	}
	if (tmpbuf.c_date != dumpdate || tmpbuf.c_ddate != dumptime) {
		fprintf(stderr, "Wrong dump date\n\tgot: %s",
			ctime(&tmpbuf.c_date));
		fprintf(stderr, "\twanted: %s", ctime(&dumpdate));
		volno = 0;
		goto again;
	}
	tapesread[volno-1]++;
	blksread = savecnt;
	if (curfile.action == USING) {
		if (volno == 1)
			panic("active file into volume 1\n");
		return;
	}
	(void) gethead(&spcl);
	findinode(&spcl, curfile.action == UNKNOWN ? 1 : 0);
	if (gettingfile) {
		gettingfile = 0;
		longjmp(restart, 1);
	}
}

/*
 * handle multiple dumps per tape by skipping forward to the
 * appropriate one.
 */
#ifdef __hpux
setdumpnum()
{
	struct mtop tcom;
	struct stat statbuf;

	if (dumpnum == 1 || volno != 1)
		return;
	if (host) {
		if (rmtregfile) {
			fprintf(stderr, "Cannot have multiple dumps on regular file\n");
			done(1);
	     	}
	}
	else{
		stat(magtape, &statbuf);
		if (((statbuf.st_mode & S_IFMT) == S_IFREG)) {
	    		fprintf(stderr, "Cannot have multiple dumps on regular file\n");
	    		done(1);
		}
	}

	if (pipein) {
		fprintf(stderr, "Cannot have multiple dumps on pipe input\n");
		done(1);
	}
	tcom.mt_op = MTFSF;
	tcom.mt_count = dumpnum - 1;

	if ( host ) {
	    if (rmtioctl(MTFSF, dumpnum - 1) < 0)
		perror("rmtioctl MTFSF");
	} else {
	    if (ioctl(mt, (int)MTIOCTOP, (char *)&tcom) < 0)
		perror("ioctl MTFSF");
	}
}
#else /* __hpux */
setdumpnum()
{
	struct mtop tcom;

	if (dumpnum == 1 || volno != 1)
		return;
	if (pipein) {
		fprintf(stderr, "Cannot have multiple dumps on pipe input\n");
		done(1);
	}
	tcom.mt_op = MTFSF;
	tcom.mt_count = dumpnum - 1;
#ifdef RRESTORE
	rmtioctl(MTFSF, dumpnum - 1);
#else /* RRESTORE */
	if (ioctl(mt, (int)MTIOCTOP, (char *)&tcom) < 0)
		perror("ioctl MTFSF");
#endif /* RRESTORE */
}
#endif /* __hpux */

extractfile(name)
	char *name;
{
#ifdef __hpux
	int length;
	int mknodres;
#endif /* __hpux */
	int mode;
	time_t timep[2];
	struct entry *ep;
	extern int xtrlnkfile(), xtrlnkskip();
#ifdef __hpux
	extern int xtrnetfile(), xtrnetskip();
#endif /* __hpux */
	extern int xtrfile(), xtrskip();
#ifdef SHORT_NAMES_ONLY
	char *trunc_namep, trunc_name[MAXNAMLEN];
#endif /* SHORT_NAMES_ONLY */
#if defined(TRUX) && defined(B1) 
	char *file, *tmptr, *newfile,origname[MAXNAMLEN];
	ino_t grandparent_ino;
	struct inotab *grandparent_itp;
        struct sec_attr sec; /* topass to proclvl. It will be empty */
	struct context  savefile;

	strcpy(origname, name);
#endif 
	curfile.name = name;
	curfile.action = USING;
	timep[0] = curfile.dip->di_atime;
	timep[1] = curfile.dip->di_mtime;
	mode = curfile.dip->di_mode;
	switch (mode & IFMT) {

	default:
#if defined(TRUX) && defined(B1) && (B1DEBUG)
		msg("extractfile: case Unknown, ino %d\n", curfile.ino);
#endif 
		fprintf(stderr, "%s: unknown file mode 0%o\n", name, mode);
		skipfile();
		return (FAIL);

	case IFSOCK:
#if defined(TRUX) && defined(B1) && (B1DEBUG)
	  msg("extractfile:IFSOCK ino %d name %s\n",curfile.ino, curfile.name);
#endif 
		vprintf(stdout, "skipped socket %s\n", name);
		skipfile();
		return (GOOD);

	case IFDIR:
#if defined(TRUX) && defined(B1) && (B1DEBUG)
	  msg("extractfile:IFDIR ino %d name %s\n",curfile.ino, curfile.name);
#endif 
		if (mflag) {
			ep = lookupname(name);
			if (ep == NIL || ep->e_flags & EXTRACT)
				panic("unextracted directory %s\n", name);
			skipfile();
			return (GOOD);
		}
		vprintf(stdout, "extract file %s\n", name);
		return (genliteraldir(name, curfile.ino));

	case IFLNK:
#if defined(TRUX) && defined(B1) && (B1DEBUG)
	  msg("extractfile:IFLNK ino %d name %s\n",curfile.ino, curfile.name);
#endif 
		lnkbuf[0] = '\0';
		pathlen = 0;
		getfile(xtrlnkfile, xtrlnkskip);
		if (pathlen == 0) {
			vprintf(stdout,
			    "%s: zero length symbolic link (ignored)\n", name);
			return (GOOD);
		}
		return (linkit(lnkbuf, name, SYMLINK));

#if defined(RFA) || defined(OLD_RFA)
	case IFNWK:
#if defined(OLD_RFA)
		vprintf(stdout,
		    "%s: obsolete network special file (ignored)\n",
		    name);
		skipfile();
#else
		vprintf(stdout, "extract network special %s\n", name);
#ifdef CNODE_DEV
#if defined(TRUX) && defined(B1)
	  msg_b1("extractfile:IFNWK ino %d name %s\n",curfile.ino,curfile.name);
#endif /* TRUX && B1 */
                curfile.dip->di_rsite = curfile.dip->di_rsite - CSD_MAGIC;
                if((curfile.dip->di_rsite < 0) ||
                   (curfile.dip->di_rsite > 255))
                    mknodres = mknod(name, mode, (int)curfile.dip->di_rdev);
                else
                    mknodres = mkrnod(name, mode, (int)curfile.dip->di_rdev, curfile.dip->di_rsite);
#else
                mknodres = mknod(name, mode, (int)curfile.dip->di_rdev);
#endif /* CNODE_DEV */
                if (mknodres < 0) {
			fprintf(stderr, "%s: ", name);
			(void) fflush(stderr);
			perror("cannot create network special");
			skipfile();
			return (FAIL);
		}
		netbuf[0] = '\0';
		getfile(xtrnetfile, xtrnetskip);
		if ((ofile = open(name, 1)) < 0) {
                        perror("open");
                        done(1);
                }
                length = strlen (netbuf);
                if (write(ofile, netbuf, length+1) != length+1) {
			perror("write");
                        done(1);
                }
		(void) fchown(ofile, curfile.dip->di_uid, curfile.dip->di_gid);
		(void) fchmod(ofile, mode);
		(void) close(ofile);
#if defined(TRUX) && defined(B1)
		set_secure(curfile,mode&IFMT, origname);/*set tags onthe file */
		(void) chmod(name, mode); 	/* to regain lost suid/sgid */
#endif /* TRUX && B1 */
		utime(name, timep);
#endif /* OLD_RFA */
		return(GOOD);
#endif /* RFA || OLD_RFA */

#ifdef __hpux
	case IFIFO:
		vprintf(stdout, "extract fifo %s\n", name);
#if defined(TRUX) && defined(B1)
	  msg_b1("extractfile:IFIFO ino %d name %s\n",curfile.ino,curfile.name);
#endif /* TRUX && B1 */
#ifdef CNODE_DEV
                curfile.dip->di_rsite = curfile.dip->di_rsite - CSD_MAGIC;
                if((curfile.dip->di_rsite < 0) ||
                   (curfile.dip->di_rsite > 255))
                    mknodres = mknod(name, mode, (int)curfile.dip->di_rdev);
                else
                    mknodres = mkrnod(name, mode, (int)curfile.dip->di_rdev, curfile.dip->di_rsite);
#else
                mknodres = mknod(name, mode, (int)curfile.dip->di_rdev);
#endif /* CNODE_DEV */

                if(mknodres < 0) {

#if defined(TRUX) && defined(B1)
		   rm_mldchildname(name);
#ifdef CNODE_DEV
                   curfile.dip->di_rsite = curfile.dip->di_rsite - CSD_MAGIC;
                   if ((curfile.dip->di_rsite <0)||(curfile.dip->di_rsite >255))
                       mknodres = mknod(name, mode, (int)curfile.dip->di_rdev);
                   else
                       mknodres = mkrnod(name, mode, (int)curfile.dip->di_rdev, curfile.dip->di_rsite);
#else  /* CNODE_DEV */
                	mknodres = mknod(name, mode, (int)curfile.dip->di_rdev);
#endif /* CNODE_DEV */
		   if (mknodres < 0 ) { /* SO was NOT due to mldchildname */
			fprintf(stderr, "%s: ", name);
			(void) fflush(stderr);
			perror("cannot create fifo");
			skipfile();
			return (FAIL);
		   }
		}
#else /* TRUX && B1 */
			fprintf(stderr, "%s: ", name);
			(void) fflush(stderr);
			perror("cannot create fifo");
			skipfile();
			return (FAIL);
		}
#endif /* TRUX && B1 */
		(void) chown(name, curfile.dip->di_uid, curfile.dip->di_gid);
		(void) chmod(name, mode);
#if defined(TRUX) && defined(B1)
		set_secure(curfile,mode&IFMT,origname);	/* set tags onthe file*/
		(void) chmod(name, mode); 	/* to regain lost suid/sgid */
#endif /* TRUX && B1 */
		skipfile();
		utime(name, timep);
		return (GOOD);
#endif /* __hpux */

	case IFCHR:
	case IFBLK:
		vprintf(stdout, "extract special file %s\n", name);
#if defined(TRUX) && defined(B1)
	 	msg_b1("extractfile:IFCHR/BLK ino %d name %s\n",
			curfile.ino,curfile.name);
#endif /* TRUX && B1 */
#ifdef CNODE_DEV
                curfile.dip->di_rsite = curfile.dip->di_rsite - CSD_MAGIC;
                if((curfile.dip->di_rsite < 0) ||
                   (curfile.dip->di_rsite > 255))
                    mknodres = mknod(name, mode, (int)curfile.dip->di_rdev);
                else
                    mknodres = mkrnod(name, mode, (int)curfile.dip->di_rdev, curfile.dip->di_rsite);
#else
                mknodres = mknod(name, mode, (int)curfile.dip->di_rdev);
#endif /* CNODE_DEV */
                if(mknodres < 0) {

#if defined(TRUX) && defined(B1)
		   rm_mldchildname(name);
#ifdef CNODE_DEV
                   curfile.dip->di_rsite = curfile.dip->di_rsite - CSD_MAGIC;
                   if ((curfile.dip->di_rsite <0)||(curfile.dip->di_rsite >255))
                       mknodres = mknod(name, mode, (int)curfile.dip->di_rdev);
                   else
                       mknodres = mkrnod(name, mode, (int)curfile.dip->di_rdev, curfile.dip->di_rsite);
#else  /* CNODE_DEV */
                	mknodres = mknod(name, mode, (int)curfile.dip->di_rdev);
#endif /* CNODE_DEV */
		   if (mknodres < 0 ){ /* SO was NOT due to mldchildname */
			fprintf(stderr, "%s: ", name);
			(void) fflush(stderr);
			perror("cannot create special file");
			skipfile();
			return (FAIL);
		   }
		}
#else /* TRUX && B1 */
			fprintf(stderr, "%s: ", name);
			(void) fflush(stderr);
			perror("cannot create special file");
			skipfile();
			return (FAIL);
		}
#endif /* TRUX && B1 */
		(void) chown(name, curfile.dip->di_uid, curfile.dip->di_gid);
		(void) chmod(name, mode);
#if defined(TRUX) && defined(B1)
                set_secure(curfile,mode&IFMT,origname);	/* set tags onthe file*/
		(void) chmod(name, mode); 	/* to regain lost suid/sgid */
#endif /* TRUX && B1 */
		skipfile();
		utime(name, timep);
		return (GOOD);

	case IFREG:
		vprintf(stdout, "extract file %s\n", name);
#if defined(TRUX) && defined(B1)
	  msg_b1("extractfile:IFREG ino %d name %s\n",curfile.ino,curfile.name);
#endif /* TRUX && B1 */
		if ((ofile = creat(name, 0666)) < 0) {
#if defined(TRUX) && defined(B1)
/* if the parent was an MLD child we could get here. So remove the parent's
 * name and try again if failed then continue as before
 */
		    rm_mldchildname(name);
		    ofile = creat(name, 0666);
		    if (ofile  < 0) { /* Was Not beacause of mld */
			fprintf(stderr, "%s: ", newfile);
			(void) fflush(stderr);
			perror("cannot create file");
			skipfile();
			return (FAIL);
		   }
#else /* TRUX && B1 */

			fprintf(stderr, "%s: ", name);
			(void) fflush(stderr);
			perror("cannot create file");
			skipfile();
			return (FAIL);
#endif /* TRUX && B1 */
		}
#ifdef SHORT_NAMES_ONLY
		if ((trunc_namep = rindex(name, '/')) != NULL) {
			trunc_namep++;
			strcpy(trunc_name, trunc_namep);
		} else {
	  		trunc_namep = name;
			strcpy(trunc_name, trunc_namep);
		}
		if (strlen(trunc_name) > DIRSIZ) {
			fprintf(stderr, "Warning: %s truncated --> ", name);
			trunc_name[DIRSIZ] = NULL;
			fprintf(stderr, "%s\n", trunc_name);
		}
#endif /* SHORT_NAMES_ONLY */
		(void) fchown(ofile, curfile.dip->di_uid, curfile.dip->di_gid);
		(void) fchmod(ofile, mode);
#if defined(TRUX) && defined(B1)
		savefile.dip = (char *) malloc( sizeof(struct sec_dinode));
		savefile.name = (char *)malloc(MAXNAMLEN);
		strcpy(savefile.name, curfile.name);
		copy_secinfo(savefile.dip, curfile.dip);
#endif /* TRUX && B1 */
		getfile(xtrfile, xtrskip);
		(void) close(ofile);
#if defined(TRUX) && defined(B1)
		set_secure(savefile,mode&IFMT,origname);/*set various tags onthe file */
		(void) chmod(savefile.name, mode);/*to regain posible sgidsuid*/
		free(savefile.name);
		free(savefile.dip);
#endif /* TRUX && B1 */
		utime(name, timep);
		return (GOOD);
	}
	/* NOTREACHED */
}

/*
 * skip over bit maps on the tape
 */
skipmaps()
{

	while (checktype(&spcl, TS_CLRI) == GOOD ||
	       checktype(&spcl, TS_BITS) == GOOD)
		skipfile();
}

/*
 * skip over a file on the tape
 */
skipfile()
{
	extern int null();

	curfile.action = SKIP;
	getfile(null, null);
}

/*
 * Do the file extraction, calling the supplied functions
 * with the blocks
 */
getfile(f1, f2)
	int	(*f2)(), (*f1)();
{
	register int i;
	int curblk = 0;
	off_t size;
	static char clearedbuf[MAXBSIZE];
	char buf[MAXBSIZE / TP_BSIZE][TP_BSIZE];
	char junk[TP_BSIZE];
#if defined(TRUX) && defined(B1)
        if (it_is_b1fs){
		size = spcl.udino.s_dinode.di_node.di_size;
		msg_b1("getfile: curfile.ino = %d\n", curfile.ino);
        }else
		size = spcl.udino.c_dinode.di_size;
#else /* TRUX & B1 */
	size = spcl.c_dinode.di_size;
#endif /* TRUX & B1 */

	if (checktype(&spcl, TS_END) == GOOD)
		panic("ran off end of tape\n");
	if (ishead(&spcl) == FAIL) 
		panic("not at beginning of a file\n");
	if (!gettingfile && setjmp(restart) != 0)
		return;
	gettingfile++;
loop:
	for (i = 0; i < spcl.c_count; i++) {
		if (spcl.c_addr[i]) {
			readtape(&buf[curblk++][0]);
			if (curblk == fssize / TP_BSIZE) {
				(*f1)(buf, size > TP_BSIZE ?
				     (long) (fssize) :
				     (curblk - 1) * TP_BSIZE + size);
				curblk = 0;
			}
		} else {
			if (curblk > 0) {
				(*f1)(buf, size > TP_BSIZE ?
				     (long) (curblk * TP_BSIZE) :
				     (curblk - 1) * TP_BSIZE + size);
				curblk = 0;
			}
			(*f2)(clearedbuf, size > TP_BSIZE ?
				(long) TP_BSIZE : size);
		}
		if ((size -= TP_BSIZE) <= 0) {
			for (i++; i < spcl.c_count; i++)
				if (spcl.c_addr[i])
					readtape(junk);
			break;
		}
	}
	if (readhdr(&spcl) == GOOD && size > 0) {
		if (checktype(&spcl, TS_ADDR) == GOOD)
			goto loop;
		dprintf(stdout, "Missing address (header) block for %s\n",
			curfile.name);
	}
	if (curblk > 0)
		(*f1)(buf, (curblk * TP_BSIZE) + size);
	findinode(&spcl, 1);
	gettingfile = 0;
}

/*
 * The next routines are called during file extraction to
 * put the data into the right form and place.
 */
xtrfile(buf, size)
	char	*buf;
	long	size;
{

	if (write(ofile, buf, (int) size) == -1) {
		fprintf(stderr, "write error extracting inode %d, name %s\n",
			curfile.ino, curfile.name);
		perror("write");
		done(1);
	}
}

xtrskip(buf, size)
	char *buf;
	long size;
{

#ifdef lint
	buf = buf;
#endif /* lint */
	if (lseek(ofile, size, 1) == (long)-1) {
		fprintf(stderr, "seek error extracting inode %d, name %s\n",
			curfile.ino, curfile.name);
		perror("lseek");
		done(1);
	}
}

xtrlnkfile(buf, size)
	char	*buf;
	long	size;
{

	pathlen += size;
	if (pathlen > MAXPATHLEN) {
		fprintf(stderr, "symbolic link name: %s->%s%s; too long %d\n",
		    curfile.name, lnkbuf, buf, pathlen);
		done(1);
	}
	(void) strcat(lnkbuf, buf);
}

xtrlnkskip(buf, size)
	char *buf;
	long size;
{

#ifdef lint
	buf = buf, size = size;
#endif /* lint */
	fprintf(stderr, "unallocated block in symbolic link %s\n",
		curfile.name);
	done(1);
}

#ifdef RFA
xtrnetfile(buf, size)
	char	*buf;
	long	size;
{
	(void) strcat(netbuf, buf);
}

xtrnetskip(buf, size)
	char *buf;
	long size;
{

#ifdef lint
	buf = buf, size = size;
#endif /* lint */
	fprintf(stderr, "unallocated block in network special file %s\n",
		curfile.name);
	done(1);
}
#endif /* RFA */

xtrmap(buf, size)
	char	*buf;
	long	size;
{

	bcopy(buf, map, size);
	map += size;
}

xtrmapskip(buf, size)
	char *buf;
	long size;
{

#ifdef lint
	buf = buf;
#endif /* lint */
	panic("hole in map\n");
	map += size;
}

null() {;}

/*
 * Do the tape i/o, dealing with volume changes
 * etc..
 */
readtape(b)
	char *b;
{
	register long i;
	long rd, newvol;
	int cnt;
#ifdef DEBUG
	unsigned short dbcntr;
	static char dbprtflag=0;
#endif DEBUG

	if (bct < ntrec) {
		bcopy(&tbf[(bct++*TP_BSIZE)], b, (long)TP_BSIZE);
		blksread++;
		return;
	}
	for (i = 0; i < ntrec; i++)
		((struct s_spcl *)&tbf[i*TP_BSIZE])->c_magic = 0;
	bct = 0;
	cnt = ntrec*TP_BSIZE;
	rd = 0;
getmore:
#ifdef __hpux
	if ( host ) {
	    i = rmtread(&tbf[rd], cnt);
	} else {
	    i = read(mt, &tbf[rd], cnt);
	}
#else /* __hpux */
#ifdef RRESTORE
	i = rmtread(&tbf[rd], cnt);
#else /* RRESTORE */
	i = read(mt, &tbf[rd], cnt);
#endif /* RRESTORE */
#endif /* __hpux */
	if (i > 0 && i != ntrec*TP_BSIZE) {
		if (pipein) {
			rd += i;
			cnt -= i;
			if (cnt > 0)
				goto getmore;
			i = rd;
		} else {
			if (i % TP_BSIZE != 0)
				panic("partial block read: %d should be %d\n",
					i, ntrec * TP_BSIZE);
			bcopy((char *)&endoftapemark, &tbf[i],
				(long)TP_BSIZE);
		}
	}
	if (i < 0) {
		fprintf(stderr, "Tape read error while ");
		switch (curfile.action) {
		default:
			fprintf(stderr, "trying to set up tape\n");
			break;
		case UNKNOWN:
			fprintf(stderr, "trying to resyncronize\n");
			break;
		case USING:
			fprintf(stderr, "restoring %s\n", curfile.name);
			break;
		case SKIP:
			fprintf(stderr, "skipping over inode %d\n",
				curfile.ino);
			break;
		}
		if (!yflag && !reply("continue"))
			done(1);
		i = ntrec*TP_BSIZE;
		bzero(tbf, i);
#ifdef __hpux
		if ( host ) {
		    if (rmtseek(i, 1) < 0) {
			perror("continuation failed");
			done(1);
		    }
		} else {
		    if (lseek(mt, i, 1) == (long)-1) {
			perror("continuation failed");
			done(1);
		    }
		}
#else /* __hpux */
#ifdef RRESTORE
		if (rmtseek(i, 1) < 0)
#else /* RRESTORE */
		if (lseek(mt, i, 1) == (long)-1)
#endif /* RRESTORE */
		{
			perror("continuation failed");
			done(1);
		}
#endif /* __hpux */
	}
	if (i == 0) {
		if (!pipein) {
			newvol = volno + 1;
			volno = 0;
			getvol(newvol);
			readtape(b);
			return;
		}
		if (rd % TP_BSIZE != 0)
			panic("partial block read: %d should be %d\n",
				rd, ntrec * TP_BSIZE);
		bcopy((char *)&endoftapemark, &tbf[rd], (long)TP_BSIZE);
	}

#ifdef DEBUG
        if ( dbprtflag ) {
	   fprintf(dbgstream,"============================================================\n");
	   for ( dbcntr=0; dbcntr != ntrec; dbcntr++) {
	      fprintf(dbgstream,"TBF Index # %d\n",dbcntr);
	      dbprintrec( &tbf[dbcntr*TP_BSIZE], TP_BSIZE, 20, 10);
           }
        }
#endif
	bcopy(&tbf[(bct++*TP_BSIZE)], b, (long)TP_BSIZE);
	blksread++;
}

findtapeblksize()
{
	register long i;

	for (i = 0; i < ntrec; i++)
		((struct s_spcl *)&tbf[i * TP_BSIZE])->c_magic = 0;
	bct = 0;
#ifdef __hpux
	if ( host ) {
	    i = rmtread(tbf, ntrec * TP_BSIZE);
	} else {
	    i = read(mt, tbf, ntrec * TP_BSIZE);
	}
#else /* __hpux */
#ifdef RRESTORE
	i = rmtread(tbf, ntrec * TP_BSIZE);
#else /* RRESTORE */
	i = read(mt, tbf, ntrec * TP_BSIZE);
#endif /* RRESTORE */
#endif /* __hpux */
	if (i <= 0) {
		perror("Tape read error");
		done(1);
	}
	if (i % TP_BSIZE != 0) {
		fprintf(stderr, "Tape block size (%d) %s (%d)\n",
			i, "is not a multiple of dump block size", TP_BSIZE);
		done(1);
	}
	ntrec = i / TP_BSIZE;
	vprintf(stdout, "Tape block size is %d\n", ntrec);
}

flsht()
{

	bct = ntrec+1;
}

closemt()
{
	if (mt < 0)
		return;
#ifdef __hpux
	if ( host ) {
	    rmtclose();
	} else {
	    (void) close(mt);
	}
#else /* __hpux */
#ifdef RRESTORE
	rmtclose();
#else /* RRESTORE */
	(void) close(mt);
#endif /* RRESTORE */
#endif /* __hpux */
}

checkvol(b, t)
	struct s_spcl *b;
	long t;
{

	if (b->c_volume != t)
		return(FAIL);
	return(GOOD);
}

readhdr(b)
	struct s_spcl *b;
{

	if (gethead(b) == FAIL) {
		dprintf(stdout, "readhdr fails at %d blocks\n", blksread);
		return(FAIL);
	}
	return(GOOD);
}

/*
 * read the tape into buf, then return whether or
 * or not it is a header block.
 */
gethead(buf)
	struct s_spcl *buf;
{
	long i, *j;
	union u_ospcl {
		char dummy[TP_BSIZE];
		struct	s_ospcl {
			long	c_type;
			long	c_date;
			long	c_ddate;
			long	c_volume;
			long	c_tapea;
			u_short	c_inumber;
			long	c_magic;
			long	c_checksum;
			struct odinode {
				unsigned short odi_mode;
				u_short	odi_nlink;
				u_short	odi_uid;
				u_short	odi_gid;
				long	odi_size;
				long	odi_rdev;
				char	odi_addr[36];
				long	odi_atime;
				long	odi_mtime;
				long	odi_ctime;
			} c_dinode;
			long	c_count;
			char	c_addr[256];
		} s_ospcl;
	} u_ospcl;

	if (!cvtflag) {
		readtape((char *)buf);
#if defined(TRUX) && defined(B1)
		if(buf->c_magic != BLS_B1_MAGIC &&
						buf->c_magic != BLS_C2_MAGIC) {
			if (swabl(buf->c_magic) != BLS_B1_MAGIC &&
					  swabl(buf->c_magic) != BLS_C2_MAGIC)
/* B1 version does'nt support cvtflag, so we either return FAIL or go to good */
#else /* (TRUX) && (B1) */
		if (buf->c_magic != NFS_MAGIC){
			if (swabl(buf->c_magic) != NFS_MAGIC)
#endif /* (TRUX) && (B1) */
				return (FAIL);
			if (!Bcvt) {
				vprintf(stdout, "Note: Doing Byte swapping\n");
				Bcvt = 1;
			}
		}
		if (checksum((int *)buf) == FAIL)
			return (FAIL);
		if (Bcvt)
			swabst("8l4s31l", (char *)buf);
		goto good;
	}
#if !defined(TRUX) && !defined(B1)
	readtape((char *)(&u_ospcl.s_ospcl));
	bzero((char *)buf, (long)TP_BSIZE);
	buf->c_type = u_ospcl.s_ospcl.c_type;
	buf->c_date = u_ospcl.s_ospcl.c_date;
	buf->c_ddate = u_ospcl.s_ospcl.c_ddate;
	buf->c_volume = u_ospcl.s_ospcl.c_volume;
	buf->c_tapea = u_ospcl.s_ospcl.c_tapea;
	buf->c_inumber = u_ospcl.s_ospcl.c_inumber;
	buf->c_checksum = u_ospcl.s_ospcl.c_checksum;
	buf->c_magic = u_ospcl.s_ospcl.c_magic;
	buf->c_dinode.di_mode = u_ospcl.s_ospcl.c_dinode.odi_mode;
	buf->c_dinode.di_nlink = u_ospcl.s_ospcl.c_dinode.odi_nlink;
	buf->c_dinode.di_uid = u_ospcl.s_ospcl.c_dinode.odi_uid;
	buf->c_dinode.di_gid = u_ospcl.s_ospcl.c_dinode.odi_gid;
	buf->c_dinode.di_size = u_ospcl.s_ospcl.c_dinode.odi_size;
	buf->c_dinode.di_rdev = u_ospcl.s_ospcl.c_dinode.odi_rdev;
	buf->c_dinode.di_atime = u_ospcl.s_ospcl.c_dinode.odi_atime;
	buf->c_dinode.di_mtime = u_ospcl.s_ospcl.c_dinode.odi_mtime;
	buf->c_dinode.di_ctime = u_ospcl.s_ospcl.c_dinode.odi_ctime;
	buf->c_count = u_ospcl.s_ospcl.c_count;
	bcopy(u_ospcl.s_ospcl.c_addr, buf->c_addr, (long)256);
	if (u_ospcl.s_ospcl.c_magic != OFS_MAGIC ||
	    checksum((int *)(&u_ospcl.s_ospcl)) == FAIL)
		return(FAIL);
	buf->c_magic = NFS_MAGIC;
#endif /* !(TRUX) && (B1) */

good:	;

#if defined(TRUX) && defined(B1)
	switch (buf->c_magic){
		case BLS_B1_MAGIC:
			it_is_b1fs = 1;
			break;
		case BLS_C2_MAGIC:
			it_is_b1fs = 0;
			break;
		default:	/* BAD TAPE MAGIC */
			return(FAIL);
	}
#endif /* (TRUX) && (B1) */

#if defined(TRUX) && defined(B1)
        if (it_is_b1fs){
		j = buf->udino.s_dinode.di_node.di_ic.ic_size.val;
		i = j[1];
	}
        else{
		j = buf->udino.c_dinode.di_ic.ic_size.val;
		i = j[1];
	}
#else /* TRUX & B1 */
	j = buf->c_dinode.di_ic.ic_size.val;
	i = j[1];
#endif /* TRUX & B1 */

	if 

#if defined(TRUX) && defined(B1)
	    (it_is_b1fs	? (buf->udino.s_dinode.di_node.di_size == 0 &&
	    (buf->udino.s_dinode.di_node.di_mode & IFMT)==IFDIR && Qcvt==0) 
	 		: (buf->udino.c_dinode.di_size == 0 &&
	    (buf->udino.c_dinode.di_mode & IFMT) == IFDIR && Qcvt==0))
#else /* TRUX & B1 */
	    (buf->c_dinode.di_size == 0 &&
	    (buf->c_dinode.di_mode & IFMT) == IFDIR && Qcvt==0) 
#endif /* TRUX & B1 */
	{
		if (*j || i) {
			printf("Note: Doing Quad swapping\n");
			Qcvt = 1;
		}
	}
	if (Qcvt) {
		j[1] = *j; *j = i;
	}
	switch (buf->c_type) {

	case TS_CLRI:
	case TS_BITS:
		/*
		 * Have to patch up missing information in bit map headers
		 */
		buf->c_inumber = 0;
#if defined(TRUX) && defined(B1)
		if (it_is_b1fs) 
			buf->udino.s_dinode.di_node.di_size = 
						buf->c_count * TP_BSIZE;
		else
			buf->udino.c_dinode.di_size = buf->c_count * TP_BSIZE;
#else /* TRUX & B1 */
		buf->c_dinode.di_size = buf->c_count * TP_BSIZE;
#endif /* TRUX & B1 */
		for (i = 0; i < buf->c_count; i++)
			buf->c_addr[i]++;
		break;

	case TS_TAPE:
	case TS_END:
		buf->c_inumber = 0;
		break;

#if defined(TRUX) && defined(B1)
	case TS_SEC_INODE:
#endif /* (TRUX) && (B1) */
	case TS_INODE:
	case TS_ADDR:
		break;

	default:
		panic("gethead: unknown inode type %d\n", buf->c_type);
		break;
	}
	if (dflag)
		accthdr(buf);
	return(GOOD);
}

/*
 * Check that a header is where it belongs and predict the next header
 */
accthdr(header)
	struct s_spcl *header;
{
	static ino_t previno = 0x7fffffff;
	static int prevtype;
	static long predict;
	long blks, i;

	if (header->c_type == TS_TAPE) {
		dprintf(stdout, "Volume header\n");
		return;
	}
	if (previno == 0x7fffffff)
		goto newcalc;
	switch (prevtype) {
	case TS_BITS:
		dprintf(stdout, "Dump mask header");
		break;
	case TS_CLRI:
		dprintf(stdout, "Remove mask header");
		break;
#if defined(TRUX) && defined(B1)
	case TS_SEC_INODE:
		dprintf(stdout, "File header, ino %d", previno);
		break;
#endif /* (TRUX) && (B1) */
	case TS_INODE:
		dprintf(stdout, "File header, ino %d", previno);
		break;
	case TS_ADDR:
		dprintf(stdout, "File continuation header, ino %d", previno);
		break;
	case TS_END:
		dprintf(stdout, "End of tape header");
		break;
	}
	if (predict != blksread - 1)
		dprintf(stdout, "; predicted %d blocks, got %d blocks",
			predict, blksread - 1);
	dprintf(stdout, "\n");
newcalc:
	blks = 0;
	if (header->c_type != TS_END)
		for (i = 0; i < header->c_count; i++)
			if (header->c_addr[i] != 0)
				blks++;
	predict = blks;
	blksread = 0;
	prevtype = header->c_type;
	previno = header->c_inumber;
}

/*
 * Find an inode header.
 * Complain if had to skip, and complain is set.
 */
findinode(header, complain)
	struct s_spcl *header;
	int complain;
{
	static long skipcnt = 0;

	curfile.name = "<name unknown>";
	curfile.action = UNKNOWN;
	curfile.dip = (struct dinode *)NIL;
	curfile.ino = 0;
	curfile.ts = 0;
	if (ishead(header) == FAIL) {
		skipcnt++;
		while (gethead(header) == FAIL)
			skipcnt++;
	}
	for (;;) {
#if defined(TRUX) && defined(B1)
		if (checktype(header, TS_SEC_INODE) == GOOD) {
			curfile.dip = &(header->udino.s_dinode.di_node);
			curfile.ino = header->c_inumber;
			curfile.ts = TS_SEC_INODE;
			msg_b1("findinode: mode=%o nlink=%d ino=%d mac=%d pprivs=%d mld=%d\n", 
			     header->udino.s_dinode.di_node.di_mode,
			     header->udino.s_dinode.di_node.di_nlink,
			     curfile.ino,
			     header->udino.s_dinode.di_tag[1], 
			     header->udino.s_dinode.di_ppriv[0],
			     header->udino.s_dinode.di_type_flags
			     );
			break;
		}
#endif /* (TRUX) && (B1) */

		if (checktype(header, TS_INODE) == GOOD) {
#if defined(TRUX) && defined(B1)
			curfile.dip = &header->udino.c_dinode;
#else /* (TRUX) && (B1) */
			curfile.dip = &header->c_dinode;
#endif /* (TRUX) && (B1) */
			curfile.ino = header->c_inumber;
			curfile.ts = TS_INODE;
			break;
		}
		if (checktype(header, TS_END) == GOOD) {
			curfile.ino = maxino;
			curfile.ts = TS_END;
			break;
		}
		if (checktype(header, TS_CLRI) == GOOD) {
			curfile.name = "<file removal list>";
			curfile.ts = TS_CLRI;
			break;
		}
		if (checktype(header, TS_BITS) == GOOD) {
			curfile.name = "<file dump list>";
			curfile.ts = TS_BITS;
			break;
		}
		while (gethead(header) == FAIL)
			skipcnt++;
	}
	if (skipcnt > 0 && complain)
		fprintf(stderr, "resync restore, skipped %d blocks\n", skipcnt);
	skipcnt = 0;
}

/*
 * return whether or not the buffer contains a header block
 */
ishead(buf)
	struct s_spcl *buf;
{

#if defined(TRUX) && defined(B1)
	if (buf->c_magic != BLS_B1_MAGIC && buf->c_magic != BLS_C2_MAGIC)
#else /* (TRUX) && (B1) */
	if (buf->c_magic != NFS_MAGIC)
#endif /* (TRUX) && (B1) */
		return(FAIL);
	return(GOOD);
}

checktype(b, t)
	struct s_spcl *b;
	int	t;
{

	if (b->c_type != t)
		return(FAIL);
	return(GOOD);
}

checksum(b)
	register int *b;
{
	register int i, j;

	j = sizeof(union u_spcl) / sizeof(int);
	i = 0;
	if(!Bcvt) {
		do
			i += *b++;
		while (--j);
	} else {
		/* What happens if we want to read restore tapes
			for a 16bit int machine??? */
		do
			i += swabl(*b++);
		while (--j);
	}

	if (i != CHECKSUM) {
		fprintf(stderr, "Checksum error %o, inode %d file %s\n", i,
			curfile.ino, curfile.name);
		return(FAIL);
	}
	return(GOOD);
}
#ifdef __hpux

msg_b1(va_alist)
	va_dcl
{
#ifdef B1DEBUG
    va_list ap;
    char *fmt;

    va_start(ap);
    fmt = va_arg(ap, char *);
    fprintf(stderr,"RESTORE: ");
    vfprintf(stderr, fmt, ap);
    fflush(stdout);
    fflush(stderr);
    va_end(ap);
#endif B1DEBUG
}

msg(va_alist)
	va_dcl
{
    va_list ap;
    char *fmt;

    va_start(ap);
    fmt = va_arg(ap, char *);
    vfprintf(stderr, fmt, ap);
#if defined(TRUX) && defined(B1) && defined (B1DEBUG)
    fflush(stdout);
    fflush(stderr);
#endif /* TRUX & B1 & B1DEBUG */
    va_end(ap);
}
#else /* __hpux */
#ifdef RRESTORE
/* VARARGS1 */
msg(cp, a1, a2, a3)
	char *cp;
{

	fprintf(stderr, cp, a1, a2, a3);
}
#endif /* RRESTORE */
#endif /* __hpux */

swabst(cp, sp)
register char *cp, *sp;
{
	int n = 0;
	char c;
	while(*cp) {
		switch (*cp) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = (n * 10) + (*cp++ - '0');
			continue;

		case 's': case 'w': case 'h':
			c = sp[0]; sp[0] = sp[1]; sp[1] = c;
			sp++;
			break;

		case 'l':
			c = sp[0]; sp[0] = sp[3]; sp[3] = c;
			c = sp[2]; sp[2] = sp[1]; sp[1] = c;
			sp += 3;
		}
		sp++; /* Any other character, like 'b' counts as byte. */
		if (n <= 1) {
			n = 0; cp++;
		} else
			n--;
	}
}

swabl(x) { unsigned long l = x; swabst("l", (char *)&l); return l; }

#ifdef DEBUG

dbprintrec(record, size, width, base)
unsigned char	*record;	/* Pointer to first byte of buffer to print */
unsigned short 	size;		/* Size of record to print		    */
unsigned short	width;		/* Width of line (# of columns)		    */
unsigned short	base;		/* Number Base for Row Headers		    */
{
unsigned short	i,asccnt;	/* Counters 				    */
unsigned short 	ascstart;	/* Position Marker			    */

   /* Printer header for record */

   fprintf(dbgstream,"       ");
   for (i=0; i != width; i++) 
      fprintf(dbgstream, "%2d ",i);
   fprintf(dbgstream,"  Ascii Char\n      ");

   for (i=0; i != (width*3)+width+2; i++);
      fprintf(dbgstream,"-");


   /* Loop to print 'size' number of bytes */ 

   for ( i=0; i != size; i++) {

      if ( (i+1) % width == 1 ) { 	         /* If new line, then, output */
	 fprintf(dbgstream,"\n %4d  ", i/base); /* <lf> and row header	      */
	 ascstart = i;			/* set beginning of ascii counter     */
      } /* End  if */

      fprintf(dbgstream, "%2x ", record[i]); /* Output current value as hex   */

      if ( (i+1) % width == 0 ) {		/* End of current line */
	 fprintf(dbgstream,"   ");
	 for (asccnt=ascstart; asccnt <= i; asccnt++) {
	    if ( isprint(record[asccnt] ))
	       fprintf(dbgstream,"%1c",record[asccnt]);
            else
	       fprintf(dbgstream,".");
	 } /* End of loop to print ascii text */

      } /* End of EOL Processing. */
   }  /* end of outter loop, all data has been printed as hex */

   /* Now we must check if the last line has completed printing, if not */
   /* then black fill the line, and output the ascii representation     */
   /* of the data.						        */

   if ( ! ((i+1) % width == 1) ) {
      while ((i)% width != 0 ) {	/* For current position to the */
	 fprintf(dbgstream,"   ");	/* end of line, print blanks.  */
	 i++;
      } /* end while */
      fprintf(dbgstream,"   ");
      for (asccnt == ascstart; asccnt == size; asccnt++) {
	    if ( isprint(record[asccnt] ))
	       fprintf(dbgstream,"%1c",record[asccnt]);
            else
	       fprintf(dbgstream,".");
      } /* End of loop to print ascii text - finishing */
      i++;
   } /* End of if statement */
   /* Now print a New line to flush buffer */
   fprintf(dbgstream,"\n");
   return;
}
#endif

#if defined(TRUX) && defined(B1)
static int mand_tag = 1;       /* hardcoded, change later */
static int acl_tag = 0;        /* hardcoded, change later */

/* Before creating each file the process level has to be changed to that of 
 * the file. The file label will be automatically set this way. UNless 
 * the files already exist (see set_secure) but this here is useful for 
 * creating hidden mld files. The only way to cread the mld child is to 
 * be at a desitred process when creating mld
 */

chproclvl(sec, curfile, fname, i)
        struct context curfile;
	struct sec_attr sec;
	char *fname;
	int i;


{
        struct sec_attr  secino;
	struct sec_dinode *dip = curfile.dip;

	if (it_is_b1fs){
		forcepriv(SEC_ALLOWMACACCESS); /* to be on the safe side */
		forcepriv(SEC_ALLOWDACACCESS);
		if (!i) 
			secino.di_tag[mand_tag] = dip->di_tag[mand_tag];
		else     secino.di_tag[mand_tag] = sec.di_tag[mand_tag];

		if(secino.di_tag[mand_tag] == SEC_WILDCARD_TAG_VALUE)
			return; /* mand_tag_to_ir for a WILDCARD fails */
        	if (mand_tag_to_ir(&(secino.di_tag[mand_tag]), ir) == 0) {
               		fprintf(stderr,"ERROR: cannot get an ir from tag\n");
			mand_copy_ir(mand_syshi, ir);
			if (setslabel(ir) < 0) 
				panic("cannot change proc level to syshi\n");
			else 	return;
		}
		if (setslabel(ir) < 0) {
               		msg_b1("ERROR: setslabel () failed on this file, %s \n",fname);
               		msg("ERROR: User clearance is insufficient.\n");
		     	done(1);
		}
	}
}

/* Set the security attributes of the file with the following condition:
 * mand_alloc_ir was done in main. mand_tag_to_ir done in set_proclvl.
 * if the file's parent was an mld child the path name has that
 * parent's name in it and it maybe different than the original 
 * system we did the dump from. So take the parent's name out of
 * the path. It should be hidden to us anyway. since we didn't 
 * force the multileveldir priv.
 */


set_secure(curfile, fmode, fname)
	struct context curfile;
	int fmode;
	char *fname;
{
        char acl_buff[1024];
	obj_t obj;
	int acl_size;
	struct sec_dinode *secino = curfile.dip;

    if (it_is_b1fs){
	force_all_privs();
	obj.o_file = fname;

        if (secino->di_type_flags == SEC_I_MLDCHILD || 
			                  secino->di_type_flags == SEC_I_MLD ){
		disable_some_privs();
                return; /* don't change the mld child/dir. OS should've */
	}

        if (secino->di_ppriv[0] != 0) 
		acl_size=0;

	/* SET SENSITIVITY LABEL */


 	/* mand_tag_to_ir for a WILDCARD fails */
	if(secino->di_tag[mand_tag] != SEC_WILDCARD_TAG_VALUE){
			
        	if (mand_tag_to_ir(&(secino->di_tag[mand_tag]), ir) == 0) {
	   	   fprintf(stderr, "ERROR: Cannot get ir from tag(%d) of %s. Will set to SYSHI\n", 
		   secino->di_tag[mand_tag], fname);
  	   	   mand_copy_ir(mand_syshi, ir);
		}
        	if (chslabel(fname, ir) == -1){
	   	   if (fmode != IFDIR){
	   	      fprintf(stderr,"ERROR: Can not set label(tag=%d) on %s. Will set to SYSHI\n", 
		      secino->di_tag[mand_tag], fname);
		      mand_copy_ir(mand_syshi, ir);
        	      if (chslabel(fname, ir) == -1)
               	         fprintf(stderr,"ERROR: Can not set %s to SYSHI\n", fname);
	   	   }
	   	   else 
                      fprintf(stderr,"WARNING: Can not set label(tag=%d) on %s. Dir not empty?\n", 
	              secino->di_tag[mand_tag], fname);
	        }
	}
	else{	 /* set to WILDCARD label by passing 0 to chslabel*/
  	   if (chslabel(fname, (mand_ir_t *)0) == -1){
	   	fprintf(stderr,"ERROR: Can not set label(tag=%d) on %s. Will set to SYSHI\n", 
		secino->di_tag[mand_tag], fname);
		mand_copy_ir(mand_syshi, ir);  /* use ir since allocated */
        	if (chslabel(fname, ir) == -1)
               	   fprintf(stderr,"ERROR: Can not set %s to SYSHI\n", fname);
	   }
	}

	/* SET PRIVILEGES */

        if (chpriv(SEC_POTENTIAL_PRIV, secino->di_ppriv,OT_REGULAR,&obj) == -1){
                fprintf(stderr,"ERROR: User has insufficient kernel authorizations.\n");
                fprintf(stderr,"ERROR: Did not set pprivs on %s\n", curfile.name);
		audit_rec(MSG_SETPPRIVS);
	}
        if (chpriv(SEC_GRANTED_PRIV, secino->di_gpriv, OT_REGULAR, &obj) == -1){
                fprintf(stderr,"ERROR: User has insufficient kernel authorizations.\n");
                fprintf(stderr,"ERROR: Did not set gprivs on %s\n", curfile.name);
		audit_rec(MSG_SETGPRIVS);
	}

	disable_some_privs();

        if(secino->di_tag[acl_tag] == SEC_WILDCARD_TAG_VALUE)
		return; /* otherwise we'll get a NULL acl instead of wildcard */

        if ((acl_size = acl_tag_to_ir(&(secino->di_tag[acl_tag]), acl_buff)) == -1){
                fprintf(stderr,"Can not get an acl_ir from the tag, %s\n",curfile.name);
        }else
		if (chacl (curfile.name,acl_buff, acl_size) == -1)
                        fprintf(stderr,"ERROR: Can not set acl on %s\n",curfile.name );
   }
}

/*
 * This routine is called to note an exception, error or auditable condition.
 * The code value selects an element of the list[] array contained in the  file
 * restore.h.  Each element contains a string to be printed to stderr,
 * a string that if not NULL is used in creating an audit record, a string
 * representing the audit result, and action code, which can be either
 * TERMINATE or NOACTION.
 */

audit_rec(code) 
	int code;
{
        if(audmsg[code].msg)
           fprintf(stderr,"%s\n",audmsg[code].msg);
        if(audmsg[code].aud_op != NULL)
           audit_subsystem(audmsg[code].aud_op,audmsg[code].aud_res,ET_SUBSYSTEM);
        if(audmsg[code].action == TERMINATE) {
           exit(1);
        }
        return(-1);
}


/*
 * remove the mldchild name from the path.
 * i.e. /tmp/mac00000000002/foo/bar will be  /tmp/foo/bar
 * Also nested mld's don't make sense at all & should 
 * not be used but we can handle it here. 
 */

rm_mldchildname(name)
	char *name;
{
#define maxname	50   /* arbitrary, but large enough */
        char 	*newfile, *file, savename[MAXPATHLEN];
	char 	*path_elements[maxname]; 

	int 	maxelement ;
	int 	i;
     	ino_t 	fino;
        struct 	inotab *fitp;

   if (it_is_b1fs){
	strcpy(savename, name);

	if ((file = (char *)calloc(MAXPATHLEN,1)) == NULL) {
		msg("Error: malloc failed %s\n", name); 
		return;
	}
	if ((path_elements[0] = strtok(name, "/")) == NULL){
		msg_b1("rm_mldchildname: Pathname is %s\n", name);
		return;
	}

	for (i=1; i<maxname; i++)
		if ((path_elements[i] = strtok((char *) NULL, "/")) != NULL) {
			continue;
		}else
			break;
	maxelement = i;

	for (i=0; i<maxelement; i++){
		if (i!=0) file = (char *)strcat((char *)file, "/");
		newfile = strcat(file, path_elements[i]);

		/* test to see if it is mld child */
                fino = dirlookup(newfile);
                fitp = inotablookup(fino);

		/* Replace the mldchildname(s) with "/" */
                if (fitp->secinfo.di_type_flags == SEC_I_MLDCHILD){

			chproclvl(fitp->secinfo, curfile, name, 1);

			/* by the time this loop is done, our proc will have 
			 * the level of the last mldchild inthe path. So the 
			 * file should go to the right place.
			 */
                        path_elements[i]="/";   
		}
	}
  	/* now reconstruct the file . This takes care of nested mld */
	file[0]= 0;
	for (i=0; i<maxelement; i++){
		if (i!=0) file = (char *)strcat((char *)file, "/");
		newfile = strcat(file, path_elements[i]);
 	}

	msg_b1("rm_mldchildname: Name was %s now is %s\n", savename, newfile);
	strcpy(name, newfile);
	free(file);
	return; 
    }
}

copy_secinfo(ndip,odip)
struct sec_dinode *ndip, *odip;

{
   if (it_is_b1fs){
	int i;

	for (i=0; i< SEC_SPRIVVEC_SIZE; i++){
		ndip->di_gpriv[i]= odip->di_gpriv[i];
		ndip->di_ppriv[i]= odip->di_ppriv[i];
	}
	for (i=0; i< SEC_TAG_COUNT; i++)
		ndip->di_tag[i]= odip->di_tag[i];
	ndip->di_type_flags = odip->di_type_flags;
	ndip->di_checksum = odip->di_checksum; 
    }
}
force_all_privs() /* needed for changing privs */
{

	forcepriv(SEC_SUSPEND_AUDIT);
	forcepriv(SEC_CONFIG_AUDIT);
	forcepriv(SEC_WRITE_AUDIT);
	forcepriv(SEC_EXECSUID);
	forcepriv(SEC_CHMODSUGID);
	forcepriv(SEC_CHOWN);
	forcepriv(SEC_ACCT);
	forcepriv(SEC_LIMIT);
	forcepriv(SEC_LOCK);
	forcepriv(SEC_LINKDIR);
	forcepriv(SEC_MKNOD);
	forcepriv(SEC_MOUNT);
	forcepriv(SEC_SYSATTR);
	forcepriv(SEC_SETPROCIDENT);
	forcepriv(SEC_CHROOT);
	forcepriv(SEC_DEBUG);
	forcepriv(SEC_SHUTDOWN);
	forcepriv(SEC_FILESYS);
	forcepriv(SEC_REMOTE);
	forcepriv(SEC_KILL);
	forcepriv(SEC_OWNER);
	forcepriv(SEC_DOWNGRADE);
	forcepriv(SEC_WRITEUPCLEARANCE);
	forcepriv(SEC_WRITEUPSYSHI);
	forcepriv(SEC_CHPRIV);
	forcepriv(SEC_MULTILEVELDIR);
	forcepriv(SEC_ALLOWMACACCESS);
	forcepriv(SEC_ALLOWDACACCESS);
	forcepriv(SEC_NETNOAUTH);
	forcepriv(SEC_NETPRIVADDR);
	forcepriv(SEC_NETBROADCAST);
	forcepriv(SEC_NETRAWACCESS);
	forcepriv(SEC_NETSETID);
	forcepriv(SEC_NETPRIVSESSION);
	forcepriv(SEC_ALLOWNETACCESS);
}

disable_some_privs()
{
	disablepriv(SEC_NETPRIVSESSION);
	disablepriv(SEC_MULTILEVELDIR);
	disablepriv(SEC_ALLOWNETACCESS);
}

/* check_device(                 *** this is almost a copy of ie_check_device 
 *      device pathname,         *** in libscmd. ie_check_device calls a bread
 *      single or multi level,   *** defined onlyin tar&cpio and checks mlmagic
 *      import or export,        *** I don't use that magic So Ihave to have 
 *      device file descriptor)  *** my own routine.
 *
 * This procedure checks the device to see if it can be used for the requested
 * operation.  It also fills out the needed IR's and flags so the software can
 * handle single/multi levels for the sensitivity label, information label and
 * national caveat set.
 *
 * The routine does not return anything.  It exits upon failure.
 */ 

#include <errno.h>
#include <pwd.h>

#define SL_SLABEL       1
#define SL_ILABEL       2
#define SL_NCAVLABEL    4
#define SL_NOT_MULTI    0x100
check_device(dev,lev,direction,fd)
	char *dev;	
	int lev;	/* AUTH_DEV_SINGLE or AUTH_DEV_MULTI */
	int direction;	/* AUTH_DEV_IMPORT,AUTH_DEV_EXPORT,AUTH_DEV_PASS */
	int fd;		/* device file descriptor	      */
{
	struct dev_asg *d;
	char **alias;
	int comp1,comp2;
	int st;
	int i,x;
	struct passwd *pw;
	int asg_impexp;    	/*Remember transfer direction.*/
	struct stat sb;          /* Holds file status structure. */
	char *lab = NULL;
	char *WildCard           = "WILDCARD";
	/*
	 * Pointers for irs. describing the process.
	 */
	mand_ir_t        *proc_ir;      /* process level        */
	mand_ir_t        *proc_clr;     /* process clearance    */
	  /*
	   * Pointers for irs. describing the device.
	   */
	mand_ir_t        *tape_ir;    /* dev default level    */
	mand_ir_t        *tape_max_ir;/* dev max level        */
	mand_ir_t        *tape_min_ir;/* dev min level        */
	int 	level_flags = 0 ;

	if(((proc_ir     = mand_alloc_ir()) == NULL) ||
	   ((proc_clr    = mand_alloc_ir()) == NULL)){
		msg("check_device(): Not enough memory for malloc\n");
		done(1);
	}

	forcepriv(SEC_ALLOWMACACCESS); 

	tape_ir = ir; /* ir is allocated */
	asg_impexp  = direction;
	/********************
	 *  PASS MODE 
	 ********************/
	/* In pass mode there is no device to check. */
	if (direction == AUTH_DEV_PASS) {
		/* In pass mode we consider the destination directory to be
		 * a device with a range set from the user's clearance
		 * down to syslo.
		 */
		tape_max_ir = proc_clr;
		tape_min_ir = mand_syslo;
		level_flags |= SL_ILABEL;
		level_flags |= SL_NCAVLABEL;

		return;
	}


	/*********************************
	 * IMPORT OR EXPORT
 	 *********************************/

	setgid(starting_rgid());

	/* We do not allow the user to specify a device through indirection */
	if (dev == NULL) {
	   if (direction == AUTH_DEV_IMPORT)
		fprintf(stderr,"ERROR: specify input device with -I option\n");
	   else fprintf(stderr,"ERROR: specify output device with -O option\n");
	   exit(1);
	}

	/* Make sure the device has a proper label. */
	if((statslabel(dev,tape_ir)!=0) && (errno != EINVAL)) 
		   	audit_rec(MSG_DEVLAB);

	msg_b1("*ie_check_device():\n");
	msg_b1(" tape: %s\n",dev);

	/* Check the device assignment data base to see if the requested device
	 * has been configured by the SSO.
	 */

	/* forcepriv(SEC_ALLOWDACACCESS); */

	while( (d = getdvagent()) != NULL) {	/* get a device record. */
   		if(d->uflg.fg_devs) {
			/* Get pointer to the list of valid device pathnames	
			 * for this device.
			 */
      			alias = d->ufld.fd_devs;
	
			/* Search the list for a match for our device. */
      			while(*alias != NULL) {
         			if(strcmp(dev,*alias) == 0) break;
	 			alias++; 
      			}
       		}
		/* break out of loop if we found a match. */
   		if(*alias != NULL) break;
	}

	/* njh if(!hassysauth(SEC_ALLOWDACACCESS))
		disablepriv(SEC_ALLOWDACACCESS); */

	/* If we didn't find the device in the data base, then only accept if
	 * it is a regular file or the file does not exist.
	 */
	if (d == NULL) {
		st = stat(dev,&sb);

	   	if (((st ==  0) && ((sb.st_mode & S_IFMT) == S_IFREG)) ||
		    ((st == -1) && (errno == ENOENT))) {
			/* Input/Output to disk file.  Consider the disk file 
			 * to be a multilevel device ranging from the 
			 * process clearance down to syslo.
		 	 */
	      		tape_ir     = proc_ir;
	      		tape_min_ir = mand_syslo;
	      		tape_max_ir = proc_clr;


			/* If ILB's are not configured, we will consider the 
			 * file to be a single-level.
	 		 */
			level_flags |= SL_ILABEL;

			/* If NCAV's are not configured, we will consider the
			 * file to be single-level device.
			 */
			level_flags |= SL_NCAVLABEL;

	      		msg_b1("ie_check_device(): disk save set specified\n");
	      		return;
		}
		/* Trying to redirect to device, note error and exit. */
		else audit_rec(MSG_DEVDB);
	}

	/* 
	 * Is user authorized to use this device?
	 */
	if (d->uflg.fg_users) {
		pw = getpwuid(getluid());    /* Get user name. */
		alias = d->ufld.fd_users;    /* Get list of allowed users */
		while(*alias != NULL)  {     /* Search for match. */
			if(strcmp(pw->pw_name,*alias) == 0) break;
				alias++; 
		}
		if(*alias == NULL) audit_rec(MSG_AUTHDEV);
	}

	/* 
	 * Make sure that the device is authorized for the requested operation,
	 * i.e. import or export.
	 */
	if (!d->uflg.fg_assign) audit_rec(MSG_DEVIMPEXP);
	if(!ISBITSET(d->ufld.fd_assign,asg_impexp)) 
           audit_rec(MSG_DEVIMPEXP);

	/*********************************************
	 * Check the device sensitivity label range.
	 ********************************************/
	/* Is it a multi-level device? */
	if (ISBITSET(d->ufld.fd_assign,AUTH_DEV_MULTI)) {
		/* Can only assign it one way. */
		if (ISBITSET(d->ufld.fd_assign,AUTH_DEV_SINGLE)) 
			audit_rec(MSG_DEVASSIGN);

		if (d->uflg.fg_max_sl) 
			tape_max_ir = d->ufld.fd_max_sl;
		else if (d->sflg.fg_max_sl) {
			tape_max_ir = d->sfld.fd_max_sl;
		}
		else audit_rec(MSG_DEVSENSLEV);

		if (d->uflg.fg_min_sl) 
			tape_min_ir = d->ufld.fd_min_sl;
		else if (d->sflg.fg_min_sl) {
			tape_min_ir = d->sfld.fd_min_sl;
		}
		else audit_rec(MSG_DEVSENSLEV);
	}
	/* Is it a single level device? */
	else if (ISBITSET(d->ufld.fd_assign,AUTH_DEV_SINGLE)) {
		if (d->uflg.fg_cur_sl) {
			tape_ir = d->ufld.fd_cur_sl;
			level_flags |= SL_SLABEL;
		}
		/* Is it a system default single level? */
		else if (d->sflg.fg_cur_sl) {
			tape_ir = d->sfld.fd_cur_sl;
			level_flags |= SL_SLABEL;
		}
		else audit_rec(MSG_DEVNOLEV);
	}

	/* The SSO must specify the device in one of the options above, else
	 * we have an error, so notify the user and exit. 
	 */
	else audit_rec(MSG_DEVNOLEV);

	/* If single-level, only files that match the designated level
	 * exactly can be exported.  Restrict the min/max_ir to the 
	 * designated device level.
	 */
	if (level_flags & SL_SLABEL) {
		tape_max_ir = tape_ir;
		tape_min_ir = tape_ir;
#ifdef B1DEBUG
		if (tape_ir) lab = mand_ir_to_er(tape_ir);
		else lab = WildCard;
		msg_b1("Device single-level for sensitivity labels: %s\n",lab);
#endif
	}
	else {	/* If the device is multi-level, make sure that we are using
		 * one of the multilevel programs.
		 */
	   if (lev != AUTH_DEV_MULTI) {
		fprintf(stderr,
		       "ERROR: the device is configured for multi-level sls\n");
		exit(1);
	   }
#ifdef B1DEBUG
	   if (tape_max_ir) lab = mand_ir_to_er(tape_max_ir);
	   else lab = WildCard;
	   msg_b1("Device multi-level for sens. labels, max: %s\n",lab);
	   if (tape_min_ir) lab = mand_ir_to_er(tape_min_ir);
	   else lab = WildCard;
	   msg_b1("                                     min: %s\n",lab);
#endif B1DEBUG
	}

	/*
	 * Now make sure that the specified sensitivity range is valid.
	 */
	/* No wildcards allowed. */
	if(tape_max_ir == (mand_ir_t *)0) audit_rec(MSG_DEVMAXLEV);
	if(tape_min_ir == (mand_ir_t *)0) audit_rec(MSG_DEVMINLEV);

  	/* Determine the relationship between the process's clearance and the 
	 * device. 
	 */

	/* njh forcepriv(SEC_ALLOWMACACCESS); */

   	comp1 = mand_ir_relationship(proc_clr,tape_max_ir);
   	comp2 = mand_ir_relationship(proc_clr,tape_min_ir);

	/* njh
	if(!hassysauth(SEC_ALLOWMACACCESS))
		disablepriv(SEC_ALLOWMACACCESS);
	*/

	/* Since we have forcepriv'd allowmac, no need to see if the
	   process dominates tape. (as in ie_check_device()) */

	/* For single-level devices, set the process level equal to 
	 * the device label.
 	 */

	/* njh forcepriv(SEC_ALLOWMACACCESS); */

	if (level_flags & SL_SLABEL) {
		proc_ir     = tape_ir;
	 	if(setslabel(proc_ir) < 0) {
			/* njh
			if(!hassysauth(SEC_ALLOWMACACCESS))
				disablepriv(SEC_ALLOWMACACCESS);
			*/
			audit_rec(MSG_CHGLEV);
		}
	}

	/* njh
	if(!hassysauth(SEC_ALLOWMACACCESS))
		disablepriv(SEC_ALLOWMACACCESS);
	*/

	/*******************************************
	 * Check the device information label range.
	 *******************************************/
	/* If ILB's are not configured for this system, then we will ignore
	 * them by considering the device to be single level.
	 */
	level_flags |= SL_ILABEL;
	msg_b1("Information labels not configured.\n");

	/* There are no checks here on the value for the information label, for
	 * we allow the wildcard on single-level devices.
	 */


	/*****************************************
	 * Check the device national caveat range.
	 *****************************************/
	/* If NCAV's are not configured for this system, then we will ignore
	 * them by considering the device to be single level.
	 */
	level_flags |= SL_NCAVLABEL;
	msg_b1("National Caveats not configured.\n");

	/* If we have made it this far, we have passed all tests and import/
	 * export will proceed.  
	 */
}
void
writehdr()
{
}
void
bread()
{
}
void
bwrite()
{
}
#endif  /* TRUX && B1 */

