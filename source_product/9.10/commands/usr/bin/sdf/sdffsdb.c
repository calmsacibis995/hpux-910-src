/* @(#) $Revision: 38.1 $ */      
static char *HPUX_ID = "@(#) $Revision: 38.1 $";
/*
 *	fsexam.c
 *
 *	Written for the HP midrange disc format.
 *	Last edit:  3 May 83, mlc
 */

#include "s500defs.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <ustat.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
/*
#include <sys/param.h>
#include <sys/filsys.h>
#include <sys/ino.h>
#include <sys/dir.h>
*/

#ifdef	DEBUG
int debug = 0;
#endif	DEBUG

#define	NO	0
#define YES	1

#define	BLKSZ	vol_hdr.s_blksz		/* volume block size */
#define	MAXBLK	vol_hdr.s_maxblk	/* last addressable block */
#define	FMAPSZ	MAXBLK/8+1		/* size of free map area */
#define	FARECSZ	sizeof(union fa_entry)	/* FA record size */
			/* offset of the 1st FA record after the free map */
#define	AFTERFMAP	3 + ((MAXBLK + 1 + ((8 * FARECSZ) - 1)) / (8 * FARECSZ))
#define	LASTFAREC	FArec.e1.di_size / FARECSZ - 1		/* last FA record number */
#define	MAXBLKSZ	4096		/* max block size of any volume */
#define	GLBLSECSZ	1024		/* used to read the superblock */

#define	MIN(a,b)	((a) < (b) ? (a) : (b))
#define	HIBIT		0x80000000
#define	VALIDPTR(x)	(((x) & HIBIT) == NULL)
#define	IOS_CONV	210866803200.0	/* used in time conversions */

typedef	long	dbaddr;		/* a bytewise device address mode */

extern	errno,	errinfo;
extern	char *sbrk();

int	nflag;		/* assume a no response */

dbaddr	fa_offset;	/* FA file offset in bytes */
dbaddr	setFAoff();

char	bitbucket[MAXBLKSZ];
char	*myname;
char	*enter_field	= "Please enter the number of the field to be modified (q to quit): ";
char	*enter_new	= "please enter new value: ";
char	*cant_mod	= "Unable to modify field %d.\n";

struct filecntl {
	int	rfdes;		/* read descriptor */
	int	wfdes;		/* write descriptor */
}	dfile;

struct	FAenode {
	ino_t	startrec;	/* first record in extent */
	ino_t	endrec;		/* last record in extent */
	dbaddr	offset;		/* bytes from start of disc */
	struct	FAenode *next;	/* points to next node */
} *firstFAenode;		/* ptr to start of linked list */

struct	filsys	vol_hdr;	/* the volume header */
union	fa_entry FArec;
struct	tm	*localtime();
char		*asctime();
char		*currentdev;

#define	SDF_LOCK	"/tmp/SDF..LCK"


main(argc, argv)
int	argc;
char	**argv;
{

	int	fd, pid, rmlock();

	sync();		/* dump all disc output */
	myname = argv[0];

	if (argc != 2) {
		fprintf(stderr, "Usage: %s device\n", myname);
		exit(-1);
	}

	if (access(SDF_LOCK, 0) == 0) {
cant:		error("%s: SDF lock file %s exists; can't proceed\n",
		    myname, SDF_LOCK);
		exit(1);
	}
	signal(SIGHUP, rmlock);
	signal(SIGQUIT, rmlock);
	signal(SIGINT, rmlock);
	signal(SIGTERM, rmlock);
	signal(SIGPIPE, rmlock);
	fd = open(SDF_LOCK, O_WRONLY | O_EXCL | O_CREAT, 0666);
	if (fd < 0)
		goto cant;
	pid = getpid();
	write(fd, (char *) &pid, sizeof(int));
	close(fd);

	while (--argc > 0 && **++argv == '-') {
		switch (*++*argv) {
			case 'n':	/* default no answer flag */
			case 'N':
				nflag++;
				break;

#ifdef	DEBUG
			case 'x':
				debug++;
				break;
#endif	DEBUG

			default:
				err("Bad option: %c, ignored.\n",**argv);
		}
	}	/* end of while */

	currentdev = *argv;
	menu(*argv++);
	unlink(SDF_LOCK);
	cleanup();
}


menu(dev)
char	*dev;
{
	char	option[80];

	if (setup(dev) == NO) return;
	if (bldFAll() == NO) return;

	for (;;) {
		printf("\nPlease select an option:\n\n");
		printf("\t1 - find inode numbers.\n");
		printf("\t2 - examine superblock.\n");
		printf("\t3 - examine inodes.\n");
		/*
		printf("\t4 - find file name.\n");
		 */
		printf("\tq - quit.\n");
		if (getline(stdin, option, sizeof(option)) == EOF)
			errexit("EOF encountered.\n");
		switch (option[0]) {
			case '1':
				finode();
				break;
			case '2':
				cksb();
				break;
			case '3':
				ckfarec();
				break;
			case 'q':
				return;
			default:
				err("Invalid response.\n");
		}
	}	/* end for loop */
}


finode()
{
	struct	stat sbuf;
	char	line[100], filename[1024], *c;

	for (;;) {
		printf("\nPlease enter the full pathname of the file (q to quit):\n");
		if (getline(stdin, line, sizeof(line)) == EOF)
			errexit("EOF encountered.\n");
		if (line[0] == 'q') break;
		strcpy(filename, currentdev);
		strcat(filename, ":");
		strcat(filename, line);
		if (sdfstat(filename, &sbuf)) {	/* changed to sdfstat() */
			err("Can't find %s.\n", line);
		}
		else {
			err("\tInode = %d.\n", sbuf.st_ino);
		}
	}	/* end of for loop */
}


cksb()
{
	dumpsb();
	if (reply("Would you like to modify the super block") == YES)
		fixsb();
}


char	*modify	= "Would you like to modify this record";
char	*ch_type = "Would you like to change its type field";

ckfarec()
{
	ino_t	recno;
	union	fa_entry rec;
	int	ioffset;

	for (;;) {
		recno = getint("\nPlease enter the number of the FA record to examine (q to quit): ");
		if (recno > 0x7fffffff) break;
		if (recno > 2 && recno < AFTERFMAP) {
			err("Record %d is inside the free map.\n", recno);
			continue;
		}
		if (recno > LASTFAREC) {
			err("Last FA record is %d\n.", LASTFAREC);
			continue;
		}
		ioffset = setFAoff(recno);
		if (bread(&dfile, &rec, ioffset, FARECSZ) == NO)
			return;
		switch (rec.e1.di_type) {
			case 1:
				dump1(&rec, recno);
				if (reply(modify) == YES)
					if (fix1(&rec, recno) == NO)
						return;
				break;
			case 2:
				dump2(&rec, recno);
				if (reply(modify) == YES)
					if (fix2(&rec, recno) == NO)
						return;
				break;
			case 5:
				err("\nRecord %d is unused (type == 5).\n", recno);
				if (reply(ch_type) == YES)
					if (fix5(&rec, recno) == NO)
						return;
				break;
			case 6:
				dump6(&rec, recno);
				if (reply(modify) == YES)
					if (fix6(&rec, recno) == NO)
						return;
				break;
			default:
				err("FA record type %d encountered.\n", rec.e1.di_type);
				if (reply(ch_type) == YES)
					if (fix5(&rec, recno) == NO)
						return;
				break;
		}
	}	/* end for loop */
}


setup(dev)			/* check the volume and do the initial setup */
char *dev;
{
	struct stat statarea;
	struct ustat ustatarea;
	dev_t rootdev;

	if (stat("/", &statarea) < 0)
		errexit("Can't stat root.\n");
	rootdev = statarea.st_dev;
	if (stat(dev, &statarea) < 0) {
		err("Can't stat %s.\n", dev);
		return(NO);
	}
/*
	if ((statarea.st_mode & S_IFMT) != S_IFBLK &&
	    (statarea.st_mode & S_IFMT) != S_IFCHR) {
		err("%s is not a block or character device.\n", dev);
		return(NO);
	}
 */
	if ((dfile.rfdes = open(dev,0)) < 0) {		/* open for reading */
		err("Can't open %s.\n", dev);
		return(NO);
	}
	if (nflag || (dfile.wfdes = open(dev,1)) < 0) {	/* open for writing */
		dfile.wfdes = -1;
		err("\tCan't write to %s, continuing for diagnostics only.\n", dev);
	}

	if (rootdev == statarea.st_rdev) {
		fprintf(stderr, "%s is the root device.\n", dev);
		fprintf(stderr, "Use \"fsdb\" instead of \"sdffsdb\".\n");
		exit(1);
	}
	if (ustat(statarea.st_rdev, &ustatarea) == 0) {
		fprintf(stderr, "%s is a mounted device.\n", dev);
		fprintf(stderr, "Use \"fsdb\" instead of \"sdffsdb\".\n");
		exit(1);
	}

		/* Get the volume header info, which is in block zero */

	BLKSZ = GLBLSECSZ;	/* init for bread */
	if (bread(&dfile, &vol_hdr, (dbaddr) 0, sizeof(vol_hdr)) == NO)
		return(NO);
	if (vol_hdr.s_format != 0x700) {
		err("%s has an unknown format.\n", dev);
		return(NO);
	}
	if (BLKSZ > MAXBLKSZ) {
		err("\nThe block size (%d) for %s is too large for %s.\n",
			BLKSZ, dev, myname);
		return(NO);
	}
	fa_offset = vol_hdr.s_fa * BLKSZ;

	/* Get info on the FA file itself */
	if (bread(&dfile, &FArec, fa_offset, FARECSZ) == NO)
		return(NO);

	printf("\nNormal FA record range: %d to %d.\n", AFTERFMAP, LASTFAREC);
	return(YES);
}


bldFAll()		/* Build the linked list of extents */
{			/* that make up the FA file. */
			/* Used only by "setFAoff()". */
	char	*lalloc();
	int	i;
	ino_t	em;
	struct	FAenode *thisnode;
	union	fa_entry glblrec;

	/* build the first node */
	if (bread(&dfile, &glblrec, fa_offset, FARECSZ) == NO)
		errexit("Can't read FA inode.\n");
	firstFAenode = thisnode = (struct FAenode *) lalloc(sizeof(struct FAenode));
	thisnode->startrec = 0;
	thisnode->endrec = glblrec.e1.di_extent[0].di_numblk * BLKSZ / FARECSZ - 1;
	thisnode->offset = fa_offset;
	thisnode->next = NULL;
	if (glblrec.e1.di_exnum == 1)
		return(YES);

	/* build nodes 2 thru 4 */
	for (i=1; i<glblrec.e1.di_exnum; i++) {
		thisnode->next = (struct FAenode *) lalloc(sizeof(struct FAenode));
		thisnode->next->startrec = thisnode->endrec + 1;
		thisnode->next->endrec = thisnode->endrec + glblrec.e1.di_extent[i].di_numblk
			* BLKSZ / FARECSZ;
		thisnode = thisnode->next;
		thisnode->offset = glblrec.e1.di_extent[i].di_startblk * BLKSZ;
		thisnode->next = NULL;
	}	/* end for loop */
	if (!VALIDPTR(em = glblrec.e1.di_exmap))
		return(YES);

	/* build all the rest */
	for(;;) {
		/* "em" has to be within the linked list built so far, I hope */
		if (bread(&dfile, &glblrec, setFAoff(em), FARECSZ) == NO)
			errexit("Bad extent map in FA inode.\n");
		for (i=0; i<glblrec.e2.e_exnum; i++) {
			thisnode->next = (struct FAenode *) lalloc(sizeof(struct FAenode));
			thisnode->next->startrec = thisnode->endrec + 1;
			thisnode->next->endrec = thisnode->endrec + glblrec.e2.e_extent[i].e_numblk
				* BLKSZ / FARECSZ;
			thisnode = thisnode->next;
			thisnode->offset = glblrec.e2.e_extent[i].e_startblk * BLKSZ;
			thisnode->next = NULL;
		}	/* end for loop */
		if (!VALIDPTR(em = glblrec.e2.e_next))
			return(YES);
	}	/* end infinite loop */
}


dbaddr
setFAoff(rec)
ino_t	rec;
{
	struct	FAenode *p = firstFAenode;

	while (rec > p->endrec && p->next != NULL)
		p = p->next;
	if (rec > p->endrec)
		errexit("Can't locate inode %d.\n", rec);
	return(p->offset + ((rec - p->startrec) * FARECSZ));
}


#define	MEMBLK	2000
static char *lasta = NULL;
static char *lastr = NULL;
char *
lalloc(req)			/* local heap allocator */
int	req;
{
	char *p;
	int n;

	if (lastr == NULL) lasta = lastr = sbrk(0);
	n = req + sizeof(char *) - 1;		/* round to next multiple */
	n &= ~(sizeof(char *) - 1);		/* of the word size */
	while (lasta + n >= lastr) {
		if ((int) sbrk(MEMBLK) == -1)
			errexit("Out of heap space.\n\n");
		lastr = sbrk(0);
	}
	p = lasta;
	lasta += n;
	return(p);
}


bread(fcp, buf, off, size)		/* read a block of fcp */
struct filecntl *fcp;
char	*buf;
dbaddr	off;
int	size;
{
	dbaddr	r;			/* offset from block boundary */
	int	i;
	int	blksz = BLKSZ;		/* so it won't change out from under us */

	if (r = off % blksz) {	/* take this block out if read is changed */
		if (lseek(fcp->rfdes, off-r, 0) < 0) {
			rwerr("seek", off-r);
			return(NO);
		}
		if (read(fcp->rfdes, bitbucket, blksz) != blksz) {
			rwerr("read", off-r);
			return(NO);
		}
		for (i = 0; i < MIN(blksz-r, size); i++)
			*buf++ = *(bitbucket + r + i);
		off += (dbaddr)(blksz-r);
		if ((size -= (blksz - r)) <= 0) return(YES);
	}

	/* "off" should be block aligned at this point */

	if (lseek(fcp->rfdes, off, 0) < 0)
		rwerr("seek", off);
	else if (size % blksz == 0) {
		if (read(fcp->rfdes, buf, size) == size)
			return(YES);
		else {
			rwerr("read", off);
			return(NO);
		}
	}
	else {
		while (size > 0) {	/* need to read in block increments */
			/* This next line could cause trouble if we're reading the end of the disc */
			if (read(fcp->rfdes, bitbucket, blksz) != blksz) {
				rwerr("read", off);
				return(NO);
			}
			for (i=0; i<MIN(size, blksz); i++)
				*buf++ = *(bitbucket + i);
			off += (dbaddr) blksz;
			size -= blksz;
		}
	}
	return(YES);
}


bwrite(fcp, buf, off, size)		/* write a block to fcp */
struct	filecntl *fcp;
char	*buf;
dbaddr	off;
int	size;
{
	dbaddr	r;			/* offset from block boundary */
	int	i, firstime = 1;
	char	*c;

	while (size > 0) {
		if ((r = off % BLKSZ) || size < BLKSZ) {
			bread(fcp, bitbucket, off-r, BLKSZ);
			c = bitbucket;
			for (i=0; i < MIN(BLKSZ-r, size); i++)
				*(c + r + i) = *buf++;
		}
		else {
			c = buf;
			buf += BLKSZ;
		}
		if (firstime) {
			if (lseek(fcp->wfdes, off-r, 0) < 0) {
				rwerr("seek", off-r);
				return(NO);
			}
			firstime = 0;
		}
		if (write(fcp->wfdes, c, BLKSZ) != BLKSZ) {
			rwerr("write", off-r);
			return(NO);
		}
		off += (dbaddr) (BLKSZ-r);
		size -= (BLKSZ - r);
	}	/* end of while loop */
	return(YES);
}


rwerr(s, off)
char *s;
dbaddr off;
{
	err("\nCan't %s: offset %d.", s, off);
	if (reply("Continue") == NO)
		errexit("Program terminated.\n");
}


reply(s)
char *s;
{
	char line[80];

	err("\n%s? ", s);
	if (nflag || dfile.wfdes < 0) {
		err("no\n\n");
		return(NO);
	}
	for (;;) {
		if (getline(stdin, line, sizeof(line)) == EOF)
			errexit("EOF encountered.\n");
		err("\n");
		if (line[0] == 'y' || line[0] == 'Y')
			return(YES);
		else if (line[0] == 'n' || line[0] == 'N' || line[0] == 'q')
			return(NO);
		err("Invalid response, please re-enter: ");
	}
}


getline(fp, loc, maxlen)	/* Read a line from file fp and put it in */
FILE *fp;			/* loc which has maxlen characters. */
char *loc;
{
	int n;
	char *p, *lastloc;

	p = loc;
	lastloc = &p[maxlen-1];
	while ((n = getc(fp)) != '\n') {
		if (n == EOF) return(EOF);
		if (!isspace(n) && p < lastloc) *p++ = n;
	}
	*p = 0;			/* terminate the string */
	return(p - loc);	/* return the length */
}


#define NUMSZ	20

char	*charerr	= "Invalid characters in number.  Please re-enter: ";

getint(str)		/* Read and return an integer */
char	*str;
{
	char	number[NUMSZ], *c;
	int	result;

	printf("%s", str);
getnum:	if (getline(stdin, number, sizeof(number)) == EOF)
		errexit("EOF encountered.\n");
	c = number;
	if (*c == 'q')
		return(-1);
	if (*c == '0') {
		c++;
		if (*c == 'x' || *c == 'X') {		/* hex */
			c++;
			if (strspn(c, "0123456789abcdefABCDEF") != strlen(c)) {
				err("charerr");
				goto getnum;
			}
			sscanf(c, "%x", &result);
		}
		else {					/* octal */
			if (strspn(c, "01234567") != strlen(c)) {
				err("charerr");
				goto getnum;
			}
			sscanf(number, "%o", &result);
		}
	}
	else {						/* decimal */
		if (*c == '+' || *c == '-') c++;
		if (strspn(c, "0123456789") != strlen(c)) {
			err(charerr);
			goto getnum;
		}
		sscanf(number, "%d", &result);
	}
	return(result);
}


char	*sbfn[] = {		/* superblock field names */
	/*  0 */	"",
	/*  1 */	"format",
	/*  2 */	"corrupt",
	/*  3 */	"name of root",
	/*  4 */	"init'd",
	/*  5 */	"block size",
	/*  6 */	"boot area start",
	/*  7 */	"boot area size",
	/*  8 */	"start of FA file",
	/*  9 */	"version",
	/* 10 */	"max block",
	/* 11 */	"password",
	/* 12 */	"backup",
};

dumpsb()
{
	long	clock;
	char	itime[26], btime[26];

	clock = vol_hdr.s_init.ti_date * 86400.0 + vol_hdr.s_init.ti_time - IOS_CONV;
	strcpy(itime, asctime(localtime(&clock)));
	clock = vol_hdr.s_bkup.ti_date * 86400.0 + vol_hdr.s_bkup.ti_time - IOS_CONV;
	strcpy(btime, asctime(localtime(&clock)));
	/* get rid of the newline */
	itime[24] = btime[24] ='\0';
	printf("Volume Header:\n\n");
	printf("1) %s:\t\t%#x",		sbfn[1], vol_hdr.s_format);
	printf("\t\t2) %s:\t%d\n",	sbfn[2], vol_hdr.s_corrupt);
	printf("3) %s:\t%.16s\n",	sbfn[3], vol_hdr.s_fname);
	printf("4} %s: %s",		sbfn[4], itime);
	printf("\t5) %s:\t%d\n",	sbfn[5], BLKSZ);
	printf("6) %s:\t%d",		sbfn[6], vol_hdr.s_boot);
	printf("\t\t7) %s:\t%d\n",	sbfn[7], vol_hdr.s_bootsz);
	printf("8) %s:\t%d",		sbfn[8], vol_hdr.s_fa);
	printf("\t\t9) %s:\t%d\n",	sbfn[9], vol_hdr.s_version);
	printf("10) %s:\t\t%d",		sbfn[10], MAXBLK);
	printf("\t\t11) %s:\t%.16s\n",	sbfn[11], vol_hdr.s_passwd);
	printf("12} %s: %s\n",		sbfn[12], btime);
	printf("\n");
}


fixsb()
{
	int	field, mflag = 0;
	char	pbuf[80];

	for (;;) {
		field = getint(enter_field);
		if (field > 0) {
			mflag = 1;
			sprintf(pbuf, "%s%s%s", sbfn[field], ": ", enter_new);
			switch (field) {
				case 1: vol_hdr.s_format = (ushort) getint(pbuf);
					break;
				case 2: vol_hdr.s_corrupt = (ushort) getint(pbuf);
					break;
				case 3: printf(pbuf);
					getline(stdin, vol_hdr.s_fname, 16);
					break;
				case 5: vol_hdr.s_blksz = getint(pbuf);
					break;
				case 6: vol_hdr.s_boot = (daddr_t) getint(pbuf);
					break;
				case 7: vol_hdr.s_bootsz = getint(pbuf);
					break;
				case 8: vol_hdr.s_fa = (daddr_t) getint(pbuf);
					break;
				case 9: vol_hdr.s_version = getint(pbuf);
					break;
				case 10: vol_hdr.s_maxblk = (daddr_t) getint(pbuf);
					break;
				case 11: printf(pbuf);
					getline(stdin, vol_hdr.s_passwd, 16);
					break;
				default: err(cant_mod, field);
			}
		}
		else {
			if (mflag)
				if (bwrite(&dfile, &vol_hdr, (dbaddr) 0, sizeof(struct filsys)) == NO)
					return(NO);
			return(YES);
		}
	}
}


char	start_ext[] = "start of extent";
char	size_ext[]  = "size of extent";

char	*ifn[] = {	 	/* inode field names */
	/*  0 */	"",
	/*  1 */	"record type",
	/*  2 */	"file type",
	/*  3 */	"link count",
	/*  4 */	"LIF type",
	/*  5 */	"created",
	/*  6 */	"capabilities",
	/*  7 */	"protection record",
	/*  8 */	"label record",
	/*  9 */	"blocks in file",
	/* 10 */	"largest addr byte",
	/* 11 */	"recommended extent size",
	/* 12 */	"number of extents",
	/* 13 */	start_ext,	size_ext,
	/* 15 */	start_ext,	size_ext,
	/* 17 */	start_ext,	size_ext,
	/* 19 */	start_ext,	size_ext,
	/* 21 */	"first extent map record",
	/* 22 */	"current size",
	/* 23 */	"parent directory",
	/* 24 */	"name",
	/* 25 */	"accessed",
	/* 26 */	"modified",
	/* 27 */	"logical record size",
	/* 28 */	"owner's user id",
	/* 29 */	"owner's group id",
	/* 30 */	"mode bits",
	/* 31 */	"unused",
	/* 32 */	"device description",
};


dump1(p, fa_node)		/* Print out the contents of a file */
struct	dinode *p;		/* information record (fir), aka inode */
ino_t	fa_node;
{
	int		i;
	long		clock;	/* I have to pass the address!!! */
	char		ctime[26], atime[26], mtime[26];

	if (p->di_type != 1) {
		err("Wrong FA record sent to \"dump1()\".\n");
		return;
	}
	clock = p->di_ctime.ti_date * 86400.0 + p->di_ctime.ti_time - IOS_CONV;
	strcpy(ctime, asctime(localtime(&clock)));
	strcpy(atime, asctime(localtime(&p->di_atime)));
	clock = p->di_mtime.ti_date * 86400.0 + p->di_mtime.ti_time - IOS_CONV;
	strcpy(mtime, asctime(localtime(&clock)));
	/* get rid of the newline */
	ctime[24] = atime[24] = mtime[24] = '\0';
	printf("Contents of FA record %d:\n\n", fa_node);
	printf("1) %s:\t\t%d (inode)",		ifn[1], p->di_type);
	printf("\t2) %s:\t\t%d\n",		ifn[2], p->di_ftype);
	printf("3) %s:\t\t%d",			ifn[3], p->di_count);
	printf("\t\t4) %s:\t\t%#hx\n",		ifn[4], p->di_uftype);
	printf("5} %s: %s",			ifn[5], ctime);
	printf("\t6) %s:\t%#x\n",		ifn[6], p->di_other);
	printf("7) %s:\t%d",			ifn[7], p->di_protect);
	printf("\t\t8) %s:\t%d\n",		ifn[8], p->di_label);
	printf("9) %s:\t%d",			ifn[9], p->di_blksz);
	printf("\t\t10) %s:\t%d\n",		ifn[10], p->di_max);
	printf("11) %s:\t%d",			ifn[11], p->di_exsz);
	printf("\t12) %s:\t%d\n",		ifn[12], p->di_exnum);
	for (i=0; i<MIN(p->di_exnum,4); i++) {
		printf("%d) %s %d:\t%d",	13+2*i, ifn[13+2*i], i+1, p->di_extent[i].di_startblk);
		printf("\t\t%d) %s %d:\t%d\n",	14+2*i, ifn[14+2*i], i+1, p->di_extent[i].di_numblk);
	}
	printf("21) %s:\t%d",			ifn[21], p->di_exmap);
	printf("\t22) %s:\t%d\n",		ifn[22], p->di_size);
	printf("\nThe next two fields only apply to directories.\n\n");
	printf("23) %s:\t%d",			ifn[23], p->di_parent);
	printf("\t\t24) %s:\t%.*s\n",		ifn[24], DIRSIZ, p->di_name);
	printf("\n");
	printf("25} %s: %s",			ifn[25], atime);
	printf("\t26} %s: %s\n",		ifn[26], mtime);
	printf("27) %s:\t%d",			ifn[27], p->di_recsz);
	printf("\t28) %s:\t%d\n",		ifn[28], p->di_uid);
	printf("29) %s:\t%d",			ifn[29], p->di_gid);
	printf("\t\t30) %s:\t\t0%o\n",		ifn[30], p->di_mode);
	printf("31} %s:\t\t%hd",		ifn[31], *(short *) p->di_res2);
	printf("\t\t32) %s:\t%#x\n",		ifn[32], p->di_dev);
}


fix1(p, fa_node)		/* Update the contents of a fir */
struct	dinode *p;
ino_t	fa_node;
{
	int	field, mflag = 0;
	dbaddr	ioffset;
	char	pbuf[80];

	for (;;) {
		field = getint(enter_field);
		if (field > 0) {
			mflag = 1;
			sprintf(pbuf, "%s%s%s", ifn[field], ": ", enter_new);
			switch (field) {
				case 1: p->di_type = (ushort) getint(pbuf);
					break;
				case 2: p->di_ftype = (ushort) getint(pbuf);
					break;
				case 3: p->di_count = (ushort) getint(pbuf);
					break;
				case 4: p->di_uftype = (short) getint(pbuf);
					break;
				case 6: p->di_other = getint(pbuf);
					break;
				case 7: p->di_protect = (ino_t) getint(pbuf);
					break;
				case 8: p->di_label = (ino_t) getint(pbuf);
					break;
				case 9: p->di_blksz = getint(pbuf);
					break;
				case 10: p->di_max = getint(pbuf);
					break;
				case 11: p->di_exsz = (ushort) getint(pbuf);
					break;
				case 12: p->di_exnum = (ushort) getint(pbuf);
					break;
				case 13:
				case 15:
				case 17:
				case 19: p->di_extent[(field-13)/2].di_startblk = (daddr_t) getint(pbuf);
					break;
				case 14:
				case 16:
				case 18:
				case 20: p->di_extent[(field-14)/2].di_numblk = getint(pbuf);
					break;
				case 21: p->di_exmap = (ino_t) getint(pbuf);
					break;
				case 22: p->di_size = getint(pbuf);
					break;
				case 23: p->di_parent = (ino_t) getint(pbuf);
					break;
				case 24: printf(pbuf);
					getline(stdin, p->di_name, 16);
					break;
				case 27: p->di_recsz = getint(pbuf);
					break;
				case 28: p->di_uid = (ushort) getint(pbuf);
					break;
				case 29: p->di_gid = (ushort) getint(pbuf);
					break;
				case 30: p->di_mode = (ushort) getint(pbuf);
					break;
				case 32: p->di_dev = (dev_t) getint(pbuf);
					break;
				default: err(cant_mod, field);
			}
		}
		else {
			ioffset = setFAoff(fa_node);
			if (mflag)
				if (bwrite(&dfile, p, ioffset, FARECSZ) == NO)
					return(NO);
			return(YES);
		}
	}	/* end for loop */
}


char	*emfn[] = {		/* extent map field names */
	/* 0 */		"",
	/* 1 */		"record type",
	/* 2 */		"number of extents",
	/* 3 */		"unused",
	/* 4 */		"next extent map",
	/* 5 */		"last extent map",
	/* 6 */		"owner's inode",
	/* 7 */		"extent's blk offset",
	/* 8-33 */	start_ext,	size_ext,
};

dump2(p, fa_node)		/* Print out the contents of an extent */
struct	em_rec *p;		/* map record. */
ino_t	fa_node;
{
	int	i;

	printf("Contents of FA record %d:\n\n", fa_node);
	printf("1) %s:\t%d (extent map)",	emfn[1], p->e_type);
	printf("\t\t2) %s:\t%d\n",		emfn[2], p->e_exnum);
	printf("3} %s:\t\t%d",			emfn[3], p->e_res1);
	printf("\t\t4) %s:\t%d\n",		emfn[4], p->e_next);
	printf("5) %s:\t%d",			emfn[5], p->e_last);
	printf("\t\t6) %s:\t%d\n",		emfn[6], p->e_inode);
	printf("7) %s:\t%d\n",			emfn[7], p->e_boffset);
	for (i=0; i<MIN(p->e_exnum,13); i++) {
		printf("%d) %s %d:\t%d",	8+2*i, emfn[8], i+1, p->e_extent[i].e_startblk);
		printf("\t\t%d) %s %d:\t%d\n",	9+2*i, emfn[9], i+1, p->e_extent[i].e_numblk);
	}
}


fix2(p, fa_node)		/* update the contents of a extent map */
struct	em_rec *p;
ino_t	fa_node;
{
	int	field, mflag = 0;
	dbaddr	ioffset;
	char	pbuf[80];

	for (;;) {
		field = getint(enter_field);
		if (field > 0) {
			mflag = 1;
			sprintf(pbuf, "%s%s%s", emfn[MIN(field, (8 + (field & 1)))], ": ", enter_new);
			switch (field) {
				case 1: p->e_type = (ushort) getint(pbuf);
					break;
				case 2: p->e_exnum = (ushort) getint(pbuf);
					break;
				case 4: p->e_next = (ino_t) getint(pbuf);
					break;
				case 5: p->e_last = (ino_t) getint(pbuf);
					break;
				case 6: p->e_inode = (ino_t) getint(pbuf);
					break;
				case 7: p->e_boffset = (daddr_t) getint(pbuf);
					break;
				case 8:
				case 10:
				case 12:
				case 14:
				case 16:
				case 18:
				case 20:
				case 22:
				case 24:
				case 26:
				case 28:
				case 30:
				case 32: p->e_extent[(field-8)/2].e_startblk = (daddr_t) getint(pbuf);
					break;
				case 9:
				case 11:
				case 13:
				case 15:
				case 17:
				case 19:
				case 21:
				case 23:
				case 25:
				case 27:
				case 29:
				case 31:
				case 33: p->e_extent[(field-9)/2].e_numblk = getint(pbuf);
					break;
				default: err(cant_mod, field);
			}
		}
		else {
			ioffset = setFAoff(fa_node);
			if (mflag)
				if (bwrite(&dfile, p, ioffset, FARECSZ) == NO)
					return(NO);
			return(YES);
		}
	}	/* end for loop */
}


fix5(p, fa_node)		/* Change the type of an unused record */
struct	dinode *p;
ino_t	fa_node;
{
	int	field;

	for(;;) {
		field = getint("Please enter the new value for the type field: ");
		switch (field) {
			case 1:
			case 2:
			case 5:
				p->di_type = field;
				if (bwrite(&dfile, p, setFAoff(fa_node), FARECSZ) == NO)
					return(NO);
			case -1:
				return(YES);
			default:
				err("WARNING:  Illegal value for the type field, updated regardless.\n");
				if (bwrite(&dfile, p, setFAoff(fa_node), FARECSZ) == NO)
					return(NO);
		}	/* end switch */
	}	/* end for loop */
}


char	*flfn[] = {		/* free list field names */
	/* 0 */		"",
	/* 1 */		"type",
	/* 2 */		"number of free recs",
	/* 3 */		"next record",
	/* 4 */		"free FA record",
};

dump6(p, fa_node)		/* Print out the contents of a free list record */
struct	free_rec *p;
ino_t	fa_node;
{
	int	i;

	printf("Contents of FA record %d:\n\n", fa_node);
	printf("1) %s:\t6 (free list record)",	flfn[1]);
	printf("\t2) %s:\t%d\n",		flfn[2], p->f_fnum);
	printf("3} %s:\t%d\n",			flfn[3], p->f_next);
	for (i=0; i<p->f_fnum; i++) {
		printf("%d} %s:\t%d\t",		i+4, flfn[4], p->f_uufar[i]);
		printf(i%2 ? "\n" : "\t");
	}
}


fix6(p, fa_node)		/* update the contents of a free list record */
struct	free_rec *p;
ino_t	fa_node;
{
	int	field, mflag = 0;
	dbaddr	ioffset;
	char	pbuf[80];

	for (;;) {
		field = getint(enter_field);
		if (field > 0) {
			mflag = 1;
			sprintf(pbuf, "%s%s%s", flfn[MIN(field, 4)], ": ", enter_new);
			switch (field) {
				case 1:
					p->f_type = (ushort) getint(pbuf);
					break;
				case 2:
					p->f_fnum = (ushort) getint(pbuf);
					break;
				default:
					err(cant_mod, field);
			}
		}
		else {
			ioffset = setFAoff(fa_node);
			if (mflag)
				if (bwrite(&dfile, p, ioffset, FARECSZ) == NO)
					return(NO);
			return(YES);
		}
	}	/* end loop */
}


cleanup()
{
	close(dfile.rfdes);
	if (dfile.wfdes != -1) close(dfile.wfdes);
}


err(s1, s2, s3, s4)
{
	fprintf(stderr, s1, s2, s3, s4);
}


errexit(s1, s2, s3, s4)
{
	err("%s: ", myname);
	err(s1, s2, s3, s4);
	cleanup();
	unlink(SDF_LOCK);
	exit(8);
}
