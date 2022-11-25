static char *HPUX_ID = "@(#) $Revision: 70.3 $";

/*  fsdb - MANX: file system debugger    */

#include	<sys/types.h>
#include	<sys/param.h>
#include 	<sys/fs.h>
#include 	<sys/sysmacros.h>
#include	<sys/inode.h>
#include 	<sys/ino.h>
#ifdef LONGFILENAMES
#define		DIRSIZ_MACRO
#endif LONGFILENAMES
#define KERNEL
#define _KERNEL
#include	<sys/dir.h>
#undef KERNEL
#undef _KERNEL

#include	<signal.h>
#include	<stdio.h>
#include	<setjmp.h>
#ifdef TRUX
#include <sys/security.h>
#endif

#define loword(X)	(((ushort *)&X)[1])
#define lobyte(X)	(((unsigned char *)&X)[1])
#define HIBIT	0100000

#define LARGEBLK	8192
#define	INOSZ	(sizeof(struct dinode))
#ifdef LONGFILENAMES
#define DIRSZ	DIRSTRCTSIZ		/* DIRSIZE=max len of short file name */
#define DIRSIZE	DIRSIZ_CONSTANT
#else not LONGFILENAMES
#define DIRSZ	(sizeof(struct direct)) /* DIRSIZ=max length of dirname */
#define DIRSIZE	DIRSIZ
#endif not LONGFILENAMES
			/* Note: on B1 code use the following define for NI:*/
			/* #define NI      (DEV_BSIZE / disk_dinode_size()) */
#define NI	(DEV_BSIZE / INOSZ)
#define ND	(DEV_BSIZE / DIRSZ)
#define	ADRMSK	(DEV_BSIZE -1)
#define NIPDB   8

#define NBUF 	3
#define MODE	0
#define LINK 	2
#define UID	4
#define GID	6
#define SZ 	12        /* di_ic.ic_size.val[1] */
#define AT	16
#define MT	24
#define CT	32
#define A0	40
#define AI	88	/* start of indirect address array (di_ib) 	*/
#define MINOR	40
#define MAJOR	40
#ifdef CNODE_DEV
#define CNODE	48	/* start of cnode id field db[2] */
#endif /* CNODE_DEV */
#define ST	100
#ifdef ACLS
#define CONT	124	/* number of associated contin. inode */
#endif /* ACLS */

#define NUMB	50     /* cannot of same value with fields above */
#define INODE	51
#define BLOCK	52
#define DIR	53


#define OBJ_BYTE	1	/* Read/put a byte [8 bits] */
#define OBJ_WORD	2	/*  a word [16 bits] */
#define OBJ_LONG	3	/*  a long [32 bits] */
#define OBJ_DIR		4	/*  a directory entry [variable > 12] */
#define OBJ_INO		5	/*  an inode entry [sizeof(struct dinode)] */

#ifdef OLD_RFA
/*
 * Define IFNWK if it isn't defined, so that we can still understand
 * network special files, even if they aren't supported.
 */
#ifndef IFNWK
#   define IFNWK 0110000   /* network special file */
#endif /* not IFNWK */
#endif /* OLD_RFA */

struct buf {
	struct	buf  *fwd;
	struct	buf  *back;
	char	*blkaddr;
	short	valid;
	long	blkno;
} buf[NBUF], bhdr;

struct dinode *ip;
#ifdef ACLS
struct cinode *cp;
#endif /* ACLS */

jmp_buf env;

char sbuf[SBSIZE],
     buffers[NBUF][LARGEBLK],
    *bread(),
    *objname(),
    *p;

struct fs *sblock;
long fragsize;

int	retcode;
#ifdef LONGFILENAMES
int	longfiles;
int	finddirect();
#endif LONGFILENAMES

daddr_t bmap();

long    addr,        /* current absolute disc address */
	cur_ino,     /* disc address of the current inode: working address */
	iblock,      /* size of inode-list per cyl group in dblocks */
	inumber,     /* current inode number */
	isize,	     /* size of inode-list per cyl group in bytes */
	maxblock,     /* max number of blocks in fs */
	maxcyl,	     /* number of cylinder groups in fs */
	maxi,	     /* max number of inodes in fs */
        super;       /* disc address of super block */
long	value, temp, oldaddr, oldvalue, erraddr;

short objsz = 2;     /* size of item in the file system to be read or put */
short objtype = OBJ_WORD;	/* Type of item to be read or put. */
short override = 0;  /* checking on */
short hex = 0;       /* printed number by default is in decimal */
short nshift,        /* log2 of NINDIR */
      nmask;         /* NINDIR - 1 */

short	fd, c_count, i, j, k, oldobjsz, oldobjtype, error, type;
short	count, pid, rpid, mode, char_flag, prt_flag;

#ifdef TRUX
char auditbuf[80];
#endif TRUX

/*
 * main - continuously looping input scanner.  Lines are scanned
 * a character at a time and are processed as soon as
 * sufficient information has been collected. When an error
 * is detected, the remainder of the line is flushed.
 */

main(argc,argv)
short	argc;
char	**argv;
{

extern	long get();
extern	long getnumb();
extern	err();

register   char  *cptr;
register   char c = '\0';             /* character input */
register   struct buf *bp;

long fblk,                     /* fragment: secondary block */
     dblk;                     /* dev block: DEV_BSIZE bytes long */
int  cyl;
#if defined(SecureWare) && defined(B1)
int iincyl;     /* inode in cylinder */
#endif

#if defined(SecureWare) && defined(B1) && !defined(SEC_STANDALONE) && !defined(STANDALONE)
if ( ISB1 ){
	set_auth_parameters(argc, argv);
	if (!authorized_user("sysadmin")) {
        fprintf(stderr, "fsdb: you must have the 'sysadmin' authorization\n");
		exit(1);
	}
	(void)forcepriv(SEC_ALLOWMACACCESS);
	(void)forcepriv(SEC_ALLOWDACACCESS);
}
#endif
setbuf(stdin,NULL);
super = SBLOCK;               /* default super block */

switch (argc) {
	case 2:
		/* fsdb special */
		break;
	case 3:
		/* fsdb special - */
		if (strcmp(argv[2], "-") == 0)
			override = 0;
		else
			usage();
		break;
	case 4:
		/* fsdb special -b xxx */
		if (strcmp(argv[2], "-b") == 0)
			super = atoi(argv[3]);
		else
			usage();
		break;
	case 5:
		/* fsdb special -b xxx  - */
		if (strcmp(argv[2], "-b") == 0)
			super = atoi(argv[3]);
		else
			usage();
		if (strcmp(argv[4], "-") == 0)
			override == 0;
		else
			usage();
		break;
	default:
		usage();
	}


/* construct linked list of buffer */
bhdr.fwd = bhdr.back = &bhdr;
for(i=0; i<NBUF; i++) {
	bp = &buf[i];
	bp->blkaddr = buffers[i];
	bp->valid = 0;
	insert(bp);
}

/* open root device with R/W mode */
if((fd = open(argv[1],2)) < 0) {
	perror(argv[0]);
	exit(1);
}

#ifdef TRUX
if (ISB1) {
	sprintf(auditbuf, "fsdb opened %s for read/write",argv[1]);
	audit_subsystem(auditbuf, "allowed to view and modify the disk",
		ET_SUBSYSTEM);
}
#endif TRUX

/* get information from super block for various checks later on */
reload();

signal(2,err);
setjmp(env);

/* interprete character input */
for(;;) {
	if(error) {
		if(c != '\n') while (getc(stdin) != '\n');
			c_count = 0;
			prt_flag = 0;
			char_flag = 0;
			error = 0;
			addr = erraddr;
			printf("?\n");
			/* type = -1; allows "d31 + +" to work */
	}

	c_count++;
	switch(c = getc(stdin)) {

	case '\n': /* command end */
#ifdef DEBUG
printf("case n: objsz=%d, objtype=%s\n", objsz, objname(objtype));
#endif
		erraddr = addr;
		if(c_count == 1) addr = addr + objsz; /* just EOLN */

		c_count = 0; /* beginning of new line*/
		if(prt_flag) {
			prt_flag = 0;
	 		continue;
		}

		temp = get(objtype);
		/* if an error has been flagged, it is probably
		 * due to alignment.  This will have set objsz
		 * to 1 hence the next get should work.
		 */
		if(error) temp = get(objtype);
#ifdef DEBUG
printf("case EOLN; addr=%ld\n", addr);
#endif
		switch(objtype) {
		case OBJ_BYTE:
			cptr = ".B";
			break;
		case OBJ_WORD:
			cptr = "";
			break;
		case OBJ_LONG:
			cptr = ".D";
			break;
		case OBJ_DIR:
			if(boundcheck(addr,erraddr)) continue;
			fprnt('d', 1);
			prt_flag = 0;
			continue;
		case OBJ_INO:
			fprnt('i',1);
			cur_ino = addr;
			prt_flag = 0;
			continue;
		} /* switch(objtype) end */
		if (hex)
			printf("0x%-12x%s: 0x%-12x\n", addr, cptr, temp);
		else
			printf("0%-12lo%s: 0%-12lo (%ld)\n",addr,cptr,temp,temp);
		continue;

	default: /* catch absolute addresses, b and i#'s */
		if(c<='9' && c>='0') {
			ungetc(c,stdin);
			addr = getnumb();
			objsz = 2;
			objtype = OBJ_WORD;
			value = addr;
			type = NUMB;
#ifdef DEBUG
printf("case EOLN, default: addr=%ld\n", addr);
#endif
			continue;
		}
		if(feof(stdin)) exit(0);
		error++;
		continue;

	case 'i': /* i# to inode conversion */
		/* enter i */
		if(c_count == 1) {
			addr = cur_ino;
			value = get(OBJ_INO);
			objtype = OBJ_INO;
			type = INODE;
			continue;
		}

		/*  enter i# i */
		if(type==NUMB)value = addr;  /* addr: i# */

		/* convert i number to disc address */
		inumber = value;
		cyl = inumber / sblock->fs_ipg;
		fblk = cgimin(sblock, cyl);
		dblk = fsbtodb(sblock , fblk);
#if defined(SecureWare) && defined(B1)
		if ( ISB1 ){
		    iincyl = inumber % sblock->fs_ipg;
		    addr = dbtob(dblk) + (iincyl/INOPB(sblock) * sblock->fs_bsize) +
		    (iincyl % INOPB(sblock)) * disk_dinode_size();
#ifdef DEBUG
printf ("iincyl %d inumber %d INOPB %d disk_dinode_size %d dblk %d addr %d\n",
iincyl, inumber, INOPB(sblock), disk_dinode_size(), dblk, addr);
#endif
		}
		else
		    addr = dbtob(dblk) + (( inumber % sblock->fs_ipg) * INOSZ);
#else
		addr = dbtob(dblk) + (( inumber % sblock->fs_ipg) * INOSZ);
#endif
#ifdef DEBUG
printf("case i; addr=%ld, fblk=%d, dblk=%d\n", addr, fblk, dblk);
#endif
		if(icheck(addr)) continue;
		cur_ino = addr;
		value = get(OBJ_INO);      /* get mode and type of inode*/
		objtype = OBJ_INO;
		type = INODE;
		continue;

	case 'b':
		/* entered block number is fsblock of DEV_BSIZE bytes long */
		if(type == NUMB)
			dblk = addr; 	/* value is block # */
		if (type == A0)
			dblk= fsbtodb(sblock,value);

                /* convert disk block to absolute address */
		addr = dbtob(dblk);
#ifdef DEBUG
printf("case b; dblk=%ld, addr = %ld\n", dblk, addr);
#endif

		/* get long here: it's make sense if this is a data block
                 * if it contains directory entries and get(OBJ_LONG) will
		 * yield inode number of that entry
		 */
		value = get(OBJ_LONG);	  /* get inumber if this is data block*/
		type = BLOCK;
		continue;

	case 'd': /* directory offsets */
		value = getnumb();
		if ( error || (value > (ND -1)))
		{
			error++;
			continue;
		}
		if(value != 0) if(dircheck()) continue;
		/* assume addr already points to beginning of data
	  	 * blocks
		 */
#ifdef LONGFILENAMES
		addr = (addr & ~ADRMSK );

		if (longfiles)
			dirforward(value);
		else
			addr += (value * DIRSZ);
#else not LONGFILENAMES
		addr = (addr & ~ADRMSK ) + (value * DIRSZ);
#endif not LONGFILENAMES
		value = get(OBJ_DIR);  	/* i-number */
		objtype = OBJ_DIR;
		type = DIR;
		continue;

	case '\t':
	case ' ':
	case '.':
		continue;

	case '+': /* address addition */
		c = getc(stdin);
		ungetc(c,stdin);
		/* value of objsz is inherit from previous command
		 * if objtype == OBJ_DIR, then +2 means 2 dirs down the blk
		 * it doesn't mean addr += 2
                 */
		if(c > '9' || c < '0') temp = 1;
		else temp = getnumb();

		if(error) continue;
#ifdef LONGFILENAMES
		if((objtype == OBJ_DIR) && (longfiles)) {
			if (dirforward(temp) != 0) {
				printf("block overflow\n");
				continue;
			}
			value = get(objtype);
			continue;
		}
#endif LONGFILENAMES
		temp *= objsz;
		/* check if next dir is still in the same
		 * data block
		 */
		if(objtype == OBJ_DIR)
			if(boundcheck(addr,addr+temp)) {
				c = '+';
				continue;
			}
		addr = addr + temp;
		value = get(objtype);
		continue;

	case '-': /* address subtraction */
		c = getc(stdin);
		ungetc(c,stdin);
		if(c > '9' || c < '0') temp = 1;
		else temp = getnumb();
		if(error) continue;

#ifdef LONGFILENAMES
		if((objtype == OBJ_DIR) && (longfiles)) {
			if (dirbackward(temp) != 0) {
				printf("block overflow\n");
				continue;
			}
			continue;
		}
#endif LONGFILENAMES
		temp *= objsz;
		if(objtype == OBJ_DIR)
			if(boundcheck(addr,addr-temp)) {
				c = '-';
				continue;
			}
		addr = addr - temp;
		value = get(objtype);
		continue;

	case '*': temp = getnumb();
		if(error) continue;
		addr = addr * temp;
		value = get(objtype);
		continue;

	case '/': temp = getnumb();
		if(error) continue;
		addr = addr / temp;
		value = get(objtype);
		continue;

	case 'q': /* quit */
		if(c_count != 1 || (c = getc(stdin)) != '\n') {
			error++;
			continue;
		}
		exit(0);

	case '>': /* save current address */
		oldaddr = addr;
		oldvalue = value;
		oldobjsz = objsz;
		oldobjtype = objtype;
		continue;

	case '<': /* restore saved address */
		addr = oldaddr;
		value = oldvalue;
		objsz = oldobjsz;
		objtype = oldobjtype;
		continue;

	case 'a': /* access time */
		if((c = getc(stdin)) == 't') {
			addr = cur_ino + AT;
			type = AT;
			value = get(OBJ_LONG);
			continue;
		}

		/* data block addresses */
		ungetc(c,stdin);
		value = getnumb();
		if ( error || value > (NDADDR+NIADDR) ) {
			error++;
			continue;
		}
		addr = cur_ino + A0 + (value * 4);
		value = get(OBJ_LONG);
#ifdef DEBUG
printf("address of data block %ld =", value);
#endif
		type = A0;
		continue;

	case 'm': /* mt, md, maj, min */
		addr = cur_ino;
		mode = get(OBJ_WORD);

		switch(c = getc(stdin)) {
		case 't': /* modification time */
			addr += MT;
			type = MT;
			value = get(OBJ_LONG);
			continue;
		case 'd': /* mode */
			addr += MODE;
			type = MODE;
			value = get(OBJ_WORD);
			continue;
		case 'i':  /* minor device number */
			if ((c = getc(stdin)) != 'n')
			{
				error++;
				continue;
			}
			if (devcheck(mode)) continue;
			addr += MINOR;
			temp = get(OBJ_LONG);
			value = minor(temp);
			type = MINOR;
			continue;
		case 'a':   /* major device number */
			if ((c = getc(stdin)) != 'j')
			{
				error++;
				continue;
			}
			if (devcheck(mode)) continue;
			addr += MAJOR;
			temp = get(OBJ_LONG);
			value = major(temp);
			type = MAJOR;
			continue;
		}
		error++;
		continue;

#ifndef ACLS
#ifndef CNODE_DEV
	case 'c': /* access time */
		if((c = getc(stdin)) == 't') {
			addr = cur_ino + CT;
			type = CT;
			value = get(OBJ_LONG);
		} else
			error++;
#else /*  CNODE_DEV & NO ACLS */
	case 'c': /* ct or cno - inode change time or cnode id */
		addr = cur_ino;
		mode = get(OBJ_WORD);

		switch(c = getc(stdin)) {
		case 't':	/* last time inode changed */
			addr += CT;
			type = CT;
			value = get(OBJ_LONG);
			continue;
		case 'n':	/* cnode id for device file */
			if ((c = getc(stdin)) != 'o')
			{
				error++;
				continue;
			}
			if (devcheck(mode)) continue;
			addr += CNODE;
			type = CNODE;
			value = get(OBJ_LONG);
			continue;
		}
		error++;
#endif /* CNODE_DEV & no ACLS */
#else /* ACLS */
#ifndef CNODE_DEV
	case 'c': /* ct or ci - inode change time or continuation inode num */
		addr = cur_ino;
		mode = get(OBJ_WORD);

		switch(c = getc(stdin)) {
		case 't':	/* last time inode changed */
			addr += CT;
			type = CT;
			value = get(OBJ_LONG);
			continue;
		case 'i':	/* continuation inode number */
			addr += CONT;
			type = CONT;
			value = get(OBJ_LONG);
			continue;
		}
		error++;
#else /* CNODE_DEV && ACLS */
	case 'c': /* ct or ci or cno - inode change time or continuation
		   *  inode num or cnode id
		   */
		addr = cur_ino;
		mode = get(OBJ_WORD);

		switch(c = getc(stdin)) {
		case 't':	/* last time inode changed */
			addr += CT;
			type = CT;
			value = get(OBJ_LONG);
			continue;
		case 'i':	/* continuation inode number */
			addr += CONT;
			type = CONT;
			value = get(OBJ_LONG);
			continue;
		case 'n':	/* cnode id for device file */
			if ((c = getc(stdin)) != 'o')
			{
				error++;
				continue;
			}
			if (devcheck(mode)) continue;
			addr += CNODE;
			type = CNODE;
			value = get(OBJ_LONG);
			continue;
		}
		error++;
#endif /* CNODE_DEV */
#endif /* ACLS */
		continue;

	case 's': /* file size */
		if((c = getc(stdin)) != 'z') {
			error++;
			continue;
		}
		addr = cur_ino + SZ;
		value = get(OBJ_LONG);
		type = SZ;
		continue;

	case 'l': /* link count */
		if((c = getc(stdin)) != 'n')
		{
			error++;
			continue;
		}
		addr = cur_ino + LINK;
		value = get(OBJ_WORD);
#ifdef DEBUG
printf("case l: addr=%ld\, cur_ino=%ldn", addr), cur_ino;
#endif
		type = LINK;
		continue;

	case 'g': /* group id */
		if((c=getc(stdin))!= 'i' || (c=getc(stdin)) != 'd') {
			error++;
			continue;
		}
		addr = cur_ino + GID;
		value = get(OBJ_WORD);
		type = GID;
		continue;

	case 'u': /* user id */
		if((c=getc(stdin))!= 'i' || (c=getc(stdin)) != 'd') {
			error++;
			continue;
		}
		addr = cur_ino + UID;
		value = get(OBJ_WORD);
		type = UID;
		continue;

	case 'n': /* directory name */
		c = getc(stdin);
		if(c != 'm') {
			error++;
			continue;
		}
		if(dircheck()) continue;
		type = DIR;
		objtype = OBJ_DIR;
		objsz = DIRSIZE;   /* =14: max name length */
		continue;

#if defined(SecureWare) && defined(B1)
	case 'P':
#ifdef INODE_CHECKSUM
	case 'c':
#endif
	case 't':
		if( ISB1 )
		    fsdb_command(c, cur_ino, &addr, &type);
		else
		    printf("fsdb: symbol %c ignored on non-B1 system\n", c);

		continue;
#endif

	case '=': /* assignment operation	*/
		switch(c = getc(stdin)) {
		case '"': /* character string */
			puta();
			continue;
		case '+': /* =+ operator */
			temp = getnumb();
			value = get(objtype);
			if(!error) put(value+temp,objsz);
			continue;
		case '-': /* =- operator */
			temp = getnumb();
			value = get(objtype);
			if(!error) put(value-temp,objsz);
			continue;
		default: /* nm and regular assignment */
			/* ????? */
			ungetc(c,stdin);
			if((type == DIR) && (c > '9' || c < '0')) {
				puta();
				continue;
			}
			value = getnumb();
#ifdef DEBUG
printf("case =: addr=%ld\n", addr);
#endif
			if(!error) put(value,objsz);
			continue;
		}

	case '!': /* shell command */
		if(c_count != 1) {
			error++;
			continue;
		}
		/* code below is not changed from 5.2; fork /bin/sh
		   instead of default shell
		   sh -t : exit after executing one command
 		 */

		{
			void (*old_handler)()=signal(SIGINT,SIG_IGN);

		/* quit catching SIG_INT while the subshell is running -
		 * in response to defect FSDlj01024 
		 */
			if((pid = fork()) == 0) {
				signal(SIGINT,SIG_DFL);
				execl("/bin/sh", "sh", "-t", 0);
				error++;
				continue;
			}
			while((rpid = wait(&retcode)) != pid && rpid != -1);
			signal(SIGINT,old_handler);
		}
		printf("!\n");
		c_count = 0;
		continue;

	case 'F': /* buffer status : not document in manual page */
		for(bp=bhdr.fwd; bp!= &bhdr; bp=bp->fwd)
			printf("%6lu %d\n",bp->blkno,bp->valid);
		continue;

	case 'f': /* file print facility */
		if((c=getc(stdin)) >= '0' && c <= '9') {
			ungetc(c,stdin);
			temp = getnumb();
			if (error) continue;
			c = getc(stdin);
		}
		else temp = 0;
		count = 0;
		addr = cur_ino;
		mode = get(OBJ_WORD);
		if(!override) {
			if((mode & IFMT)==0)
				printf("warning: inode not allocated\n");
		}

		if (((mode & IFMT) == IFCHR) || (mode & IFMT) == IFBLK) {
			printf("special device\n");
			error++;
			continue;
		}
		temp = bmap(temp);
		if ((addr = dbtob(temp)) == 0)
			continue;
#ifdef DEBUG
printf("casef: temp=%ld, addr=%ld\n", temp, addr);
#endif
		fprnt(c,0);
		continue;

	case 'L':  /* list file system block numbers */
		if (type==NUMB)
		{
			temp = blkstofrags(sblock,addr);  /* addr:large blk# */
			temp = fsbtodb(sblock, temp);
			for (i = sblock->fs_bsize/DEV_BSIZE; i>0; i--)
			{
				if ( hex)
					printf("%lx, ", temp++);
				else
					printf("%ld, ", temp++);
			}
			printf("\n");
		}
		else
			error++;
		continue;


	case 'O': /* override flip flop */
		if(override = !override)
			printf("error checking off\n");
		else {
			printf("error checking on\n");
			reload();
		}
		prt_flag++;
		continue;

	case 'X':   /* hexadecimal flip flop */
		if ( hex = !hex)
			printf("hexadecimal mode on\n");
		else
			printf("hexadecimal mode off\n");
		prt_flag++;
		continue;

	case 'B': /* byte offsets */
		objsz = 1;
		objtype = OBJ_BYTE;
		continue;

	case 'W': /* word offsets */
		objsz = 2;
		objtype = OBJ_WORD;
		addr = addr & ~01;
		continue;

	case 'D': /* double word offsets */
		objsz = 4;
		objtype = OBJ_LONG;
		addr = addr & ~01;
		continue;
#ifndef STANDALONE
	case 'A': abort();
#endif

	case ',': /* general print facilities */
	case 'p':
		if(( c = getc(stdin)) >= '0' && c <= '9') {
			ungetc(c,stdin);
			count = getnumb();
			if(error) continue;
			if((count < 0) || (count > DEV_BSIZE))
				count = 1;
			c = getc(stdin);
		}
		else if(c == '*')
			{
				count = 0;	/* print to end of block */
				c = getc(stdin);
			}
			else count = 1;
		fprnt(c,count);
	} /* switch(c) end */
} /* for end */
} /* main end */

/*======
 * getnumb - read a number from the input stream.  A leading
 * zero signifies octal interpretation. If the first character
 * is not numeric this is an error, otherwise continue
 * until the first non-numeric.
 */

long
getnumb()
{

extern	short  error;
long	number, base;
register	char  c;
char *basestr;

if(((c = getc(stdin)) < '0')||(c > '9')) {
	error++;
	ungetc(c,stdin);
	return(-1);
}
if(c == '0')
{
	if ( ( c=getc(stdin)) == 'x')
	{
		base = 16;
		basestr="0123456789ABCDEF";
#ifdef DEBUG
printf("first char of hex = %c\n", c);
#endif
	}
	else
	{
		base = 8;
		basestr="01234567";
		ungetc(c,stdin);
	}
	number = 0;
}
else
{
	base = 10;
	basestr = "0123456789";
	number = c - '0';
}

#ifdef DEBUG
printf("getnumb: first value of num=%d\n", number);
#endif

while ( ( ((c=getc(stdin))>='0') && (c<='9') ) ||
	( (c>='A') && (c<='F') ) )
{
	if ( strchr(basestr, c) == 0)
	{
		error++;
		return(-1);
	}
	number = (number * base) + (c - '0')*(c <='9') + (c -'7')*(c>='A');
}
ungetc(c,stdin);
return(number);
}

/*=======
 * get - read a byte, word or double word from the file system.
 * The entire block containing the desired item is read
 * and the appropriate data is extracted and returned.
 * Inode and directory size requests result in word
 * fetches. Directory names (objsz == DIRSZ) result in byte
 * fetches.
 */

long
get(type)
short	type;
{

long	vtemp;
char *bptr;
short offset;

objtype = type;
objsz = getsize(type);
if(allign(objsz)) return(-1);
if((bptr = bread(addr)) == 0) return(-1);

vtemp = 0;
#if defined(SecureWare) && defined(B1)
if( ISB1 )
	offset = blkoff(sblock, addr);
else
	offset = addr & ADRMSK;
#else
offset = addr & ADRMSK;
#endif
bptr = bptr + offset;
#if defined(SecureWare) && defined(B1)
/* all block i/o is in file system block size units */
if(( ISB1 ) ? (offset + objsz > sblock->fs_bsize) :
   	      (offset + objsz > DEV_BSIZE))
#else
if(offset + objsz > DEV_BSIZE)
#endif
{
	error++;
	printf ("get(type) - %d too long\n",offset + objsz);
	return(-1);
}

switch(objtype) {
case OBJ_LONG:
case OBJ_DIR:
	vtemp = *(long *)bptr;
	return(vtemp);
case OBJ_INO:
case OBJ_WORD:
	loword(vtemp) = *(short *)bptr;
	return(vtemp);
case OBJ_BYTE:
	lobyte(loword(vtemp)) = *bptr;
	return(vtemp);
} /* switch end */

error++;
printf ("get(%d) - invalid type\n",type);
return(0);
}

/*======
 * icheck - check if the current address is within the I-list.
 */
icheck(address)
long	address;
{
long dblk, frags;
int cyl;
long saddr, faddr;

if(override) return(0);

/* find cylinder group number */
dblk = btodb(address);
frags = dbtofsb(sblock,dblk);
cyl = dtog(sblock, frags);

/* find the starting and ending addresses of inode-table */
frags = cgimin(sblock, cyl);
saddr = fsbtodb(sblock, frags) * DEV_BSIZE;
#if defined(SecureWare) && defined(B1)
if( ISB1 )
    faddr = saddr + howmany(sblock->fs_ipg, INOPB(sblock)) * sblock->fs_bsize - 1;
else
    faddr = saddr + (sblock->fs_ipg * INOSZ) - 1;
#else
faddr = saddr + (sblock->fs_ipg * INOSZ) - 1;
#endif

if ( address >=saddr && address <=faddr)
	return(0);
printf("inode out of range\n");
error++;
return(1);
}

/*=======
 * putf - print a byte as an ascii character if possible.
 * The exceptions are tabs, newlines, backslashes
 * and nulls which are printed as the standard c
 * language escapes. Characters which are not
 * recognized are printed as \?.
 */
putf(c)
register char  c;
{

if(c<=037 || c>=0177 || c=='\\') {
	putc('\\',stdout);
	switch(c) {
	case '\\': putc('\\',stdout);
		break;
	case '\t': putc('t',stdout);
		break;
	case '\n': putc('n',stdout);
		break;
	case '\0': putc('0',stdout);
		break;
	default: putc('?',stdout);
	} /* switch end */
}
else {
	putc(' ',stdout);
	putc(c,stdout);
}
putc(' ',stdout);
}

/*======
 * put - write an item into the buffer for the current address
 * block.  The value is checked to make sure that it will
 * fit in the size given without truncation.  If successful,
 * the entire block is written back to the file system.
 */
put(item,lngth)
long item;
short lngth;
{

register char *bptr, *sbptr;
short offset;
long	s_err,nbytes;

objsz = lngth;
if(allign(objsz)) return;
#ifdef DEBUG
printf("in put: addr=%ld\n", addr);
#endif
if((sbptr = bread(addr)) == 0) return;
#if defined(SecureWare) && defined(B1)

if( ISB1 )
    offset = blkoff(sblock, addr);
else
    offset = addr & ADRMSK;

if((ISB1) ? (offset + lngth > sblock->fs_bsize) :
   	    (offset + lngth > DEV_BSIZE))
#else
offset = addr & ADRMSK;
if(offset + lngth > DEV_BSIZE)
#endif
{
	error++;
	printf("block overflow\n");
	return;
}
bptr = sbptr + offset;
switch(objsz) {
case DIRSZ:
case 4: *(long *)bptr = item;
	goto rite;
case INOSZ:
case 2: if(item & ~0177777L) break;
	*(short *)bptr = item;
	goto rite;
case DIRSIZE:
case 1: if(item & ~0377) break;
	*bptr = lobyte(loword(item));
rite:
#if defined(SecureWare) && defined(B1)
	if((ISB1) ? ((s_err = lseek(fd,addr - offset, 0)) == -1) :
	   	    ((s_err = lseek(fd,addr & ~(long)ADRMSK,0)) == -1))
#else
	if((s_err = lseek(fd,addr & ~(long)ADRMSK,0)) == -1)
#endif
	{
		error++;
		printf("seek error : 0%lo\n",addr);
		return;
	}
#if defined(SecureWare) && defined(B1)
	if((ISB1) ?
	   ((nbytes = write(fd,sbptr,sblock->fs_bsize)) != sblock->fs_bsize) :
	   ((nbytes = write(fd,sbptr,DEV_BSIZE)) != DEV_BSIZE))
#else
	if((nbytes = write(fd,sbptr,DEV_BSIZE)) != DEV_BSIZE)
#endif
	{
		error++;
		printf("write error : addr   = 0%lo\n",addr);
		printf("           : s_err  = 0%lo\n",s_err);
		printf("           : nbytes = 0%lo\n",nbytes);
		return;
	}
	return;
#if defined(SecureWare) && defined(B1)
default:
	if(ISB1){
	    if ((s_err = fsdb_large_size_write(objsz, bptr, item)) == 0)
		goto rite;
	    else if (s_err == 1)  {
		error++;
		return;
	    }
	    break;
	}
	else{
	    error++;
	    return;
	}
#else
default: error++;
	return;
#endif
}
printf("truncation error\n");
error++;
return;
}

/*======
 * bread - check if the desired block is in the file system.
 * Search the incore buffers to see if the block is already
 * available. If successful, unlink the buffer control block
 * from its position in the buffer list and re-insert it at
 * the head of the list.  If failure, use the last buffer
 * in the list for the desired block. Again, this control
 * block is placed at the head of the list. This process
 * will leave commonly requested blocks in the in-core buffers.
 * Finally, a pointer to the buffer is returned.
 */
char *
bread(address)
long	address;
{

register struct buf *bp;
long	dblk;
long    fblk;
long	s_err, nbytes;

#if defined(SecureWare) && defined(B1)
/* block i/o is in file system block size units */
if( ISB1 ){
	dblk = lblkno(sblock, address);
	fblk = blkstofrags(sblock, dblk);
}
else{
	dblk = (long) btodb(address);
	fblk = dbtofsb(sblock, dblk);
}
#else
dblk = (long) btodb(address);
fblk = dbtofsb(sblock, dblk);
#endif

if(!override)
	if(fblk >= maxblock) {
		printf("block out of range\n");
		error++;
		return(0);
	}
for(bp=bhdr.fwd; bp!= &bhdr; bp=bp->fwd)
	if(bp->blkno==dblk && bp->valid) goto xit;

bp = bhdr.back;
bp->blkno = dblk;
bp->valid = 0;

#if defined(SecureWare) && defined(B1)
if((ISB1) ? ((s_err = lseek(fd, address - blkoff(sblock, address), 0)) == -1) :
            ((s_err = lseek(fd, (long)dbtob(dblk) ,0)) == -1))
#else
if((s_err = lseek(fd, (long)dbtob(dblk) ,0)) == -1)
#endif
{
	error++;
	printf("seek error : 0%lo\n",addr);
	return(0);
}
#if defined(SecureWare) && defined(B1)
if((ISB1) ?

       ((nbytes = read(fd,bp->blkaddr,sblock->fs_bsize)) != sblock->fs_bsize) :
       ((nbytes = read(fd,bp->blkaddr,fragsize)) != fragsize))
#else
if((nbytes = read(fd,bp->blkaddr,fragsize)) != fragsize)
#endif
{
	error++;
	printf("read error : addr   = 0%lo\n",addr);
	printf("           : s_err  = 0%lo\n",s_err);
	printf("           : nbytes = 0%lo\n",nbytes);
	return(0);
}

bp->valid++;
xit:	bp->back->fwd = bp->fwd;
bp->fwd->back = bp->back;
insert(bp);
return(bp->blkaddr);
}

/*======
 * insert - place the designated buffer control block
 * at the head of the linked list of buffers.
 */
insert(bp)
register struct buf *bp;
{

bp->back = &bhdr;
bp->fwd = bhdr.fwd;
bhdr.fwd->back = bp;
bhdr.fwd = bp;
}


/*======
 * objname - returns the string name for a given type.
 */
char *
objname(type)
{
    char *res, buf[100];

    switch(type)
    {
case OBJ_BYTE:
	res = "OBJ_BYTE";
	break;
case OBJ_WORD:
	res = "OBJ_WORD";
	break;
case OBJ_LONG:
	res = "OBJ_LONG";
	break;
case OBJ_DIR:
	res = "OBJ_DIR";
	break;
case OBJ_INO:
	res = "OBJ_INO";
	break;
default:
	res = buf;
	sprintf(res, "Unknown type - %d", type);
	break;
    }

    return(res);
}


/*======
 * getsize - returns the size for a given type.
 */
getsize(type)
{
    int res = 1;		/* By default, return byte size. */

    switch(type)
    {
case OBJ_BYTE:
	res = 1;
	break;
case OBJ_WORD:
	res = 2;
	break;
case OBJ_LONG:
	res = 4;
	break;
case OBJ_DIR:
	res = 4;	/* Directories are accessed on long boundaries. */
	break;		/* And in long file name systems are variable. */
case OBJ_INO:
#if defined(SecureWare) && defined(B1)
	if( ISB1 )
		res = disk_dinode_size();
	else
		res = INOSZ;
#else
	res = INOSZ;
#endif
	break;
    }
    return(res);
}


dirbackward(count)
/*======
 * dirbackward - go backward ("-") a certain count.  Returns -1 on
 * failure and 0 on success.  "erraddr" has failure address.
 */
int count;
{
long dirlocs[(DEV_BSIZE/12)+1];	/* Pointer for every possible entry. */
struct direct *dirp;
char *cptr;
long lastaddr,		/* Last valid addr in this block. */
     tempaddr;		/* Address used to scan directory. */

erraddr = addr;
if ((addr & 3) != 0)	/* Check for long alignment */
	return(-1);

tempaddr = addr & ~ADRMSK;

if ((cptr = bread(tempaddr)) == 0)
	return(-1);

#if defined(SecureWare) && defined(B1)
if(ISB1)
	cptr += blkoff(sblock, tempaddr) & ~ADRMSK;
#endif
lastaddr = addr | ADRMSK;

for(dirp = ((struct direct *)cptr), i = 0; tempaddr < addr; i++)
{
	if (tempaddr >= lastaddr)
	{
		return(-1);
	}

	dirlocs[i] = tempaddr;

	tempaddr += dirp->d_reclen;
	dirp = (struct direct *)((char *)dirp + dirp->d_reclen);
}

dirlocs[i] = tempaddr;

if (count > i)
	return(-1);

addr = dirlocs[i-count];

return (0);
}


dirforward(count)
/*======
 * dirforward - go forward ("+") a certain count.  Returns -1 on
 * failure and 0 on success.  "erraddr" has failure address.
 */
int count;
{
struct direct *dirp;
char *cptr;
long lastaddr;		/* Last valid addr in this block. */
int offset;

erraddr = addr;
if ((addr & 3) != 0)	/* Check for long alignment */
	return(-1);

if ((cptr = bread(addr)) == 0)
	return(-1);

#if defined(SecureWare) && defined(B1)
if( ISB1 )
	offset = blkoff(sblock, addr);
else
	offset = addr & ADRMSK;
#else
offset = addr & ADRMSK;
#endif

lastaddr = addr | ADRMSK;

for(dirp = ((struct direct *)(cptr + offset)); count > 0; count--)
{
	if (addr >= lastaddr)
	{
		return(-1);
	}

	erraddr = addr;
	addr += dirp->d_reclen;
	dirp = (struct direct *)((char *)dirp + dirp->d_reclen);
}

if (addr >= lastaddr)
	return(-1);

return (0);
}


/*======
 * allign - before a get or put operation check the
 * current address for a boundary corresponding to the
 * size of the object.
 */
allign(ment)
short ment;
{
switch(ment) {
#ifdef hp9000s800
case 4: if(addr & 03L) break;  /* double words need to be double word aligned */
#else
case 4: if(addr & 01L) break;
#endif
	return(0);
case DIRSIZE: if((addr & 037) != 010) break;
	return(0);
case DIRSZ:
case INOSZ:
case 2: if(addr & 01L) break;
case 1: return(0);
#if defined(SecureWare) && defined(B1)
default:
	if ((ISB1) && (fsdb_large_size_allign(ment, addr)))
		return 0;
	break;
#endif
} /* switch end */

error++;
objsz = 1;
objtype = OBJ_BYTE;
printf("alignment\n");
return(1);
}

/*======
 * err - called on interrupts.  Set the current address
 * back to the last address stored in erraddr. Reset all
 * appropriate flags.  If the prt_flag is set, the interrupt
 * occured while transferring characters to a buffer.
 * These are "erased" by invalidating the buffer, causing
 * the entire block to be re-read upon the next reference.
 * A reset call is made to return to the main loop;
 */
err()
{
#ifndef STANDALONE
	signal(2,err);
	addr = erraddr;
	error = 0;
	c_count = 0;
	if(char_flag) {
		bhdr.fwd->valid = 0;
		char_flag = 0;
	}
	prt_flag = 0;
	printf("\n?\n");
	fseek(stdin, 0L, 2);
	longjmp(env,0);
#endif
}

/*======
 * devcheck - check that the given mode represents a
 * special device. The IFCHR bit is on for both
 * character and block devices (it is also on for
 * other file types now).
 */
devcheck(md)
register short md;
{
if(override) return(0);
if(((md & IFMT) == IFCHR) || (md & IFMT) == IFBLK) return(0);
printf("not char or block device\n");
error++;
return(1);
}

/*======
 * nullblk - return error if address is zero.  This is done
 * to prevent block 0 from being used as an indirect block
 * for a large file or as a data block for a small file.
 */
nullblk(bn)
long	bn;
{
	if(bn != 0) return(0);
	printf("non existent block\n");
	error++;
	return(1);
}

/*======
 * dircheck - check if the current address can be in a directory.
 * This means it is not in the I-list, block 0 or the super block.
 */

dircheck()
{
	char cbuf[MAXBSIZE];
	struct cg *cgrp;
	long dblk,
	     frags,
	     newcyl;

	static long cur_cyl,
	     lscyl,
	     lfcyl,
	     hscyl,
	     hfcyl;

	if(override) return (0);

	newcyl = dtog(sblock,(addr / sblock->fs_fsize)); /* cyl for dir */
	if ( newcyl > maxcyl || newcyl < 0)
	{
		printf(">>> cylinder out of range\n");
		error++;
		return(1);
	}
	if ( newcyl != cur_cyl || hfcyl == 0)
	{
		/* read in cylinder group information */
		frags = cgtod(sblock,cur_cyl);
		dblk = fsbtodb(sblock, frags);
		if ( lseek(fd, (long)dbtob(dblk), 0) == -1)
		{
			printf(">>>seek to cylinder group failed\n");
			error++;
			return(1);
		}
		if ( read(fd, cbuf, sblock->fs_cgsize)!= sblock->fs_cgsize)
		{
			printf(">>>read cylinder group information failed\n");
			error++;
			return(1);
		}
		cgrp = ((struct cg *) cbuf);

		lscyl = fsbtodb(sblock, cgbase(sblock, newcyl)) * DEV_BSIZE;
		lfcyl = fsbtodb(sblock, cgsblock(sblock, newcyl)) * DEV_BSIZE - 1;
		hscyl = fsbtodb(sblock, cgdmin(sblock, newcyl)) * DEV_BSIZE;
		hfcyl = fsbtodb(sblock, cgbase(sblock, (newcyl + 1))) * DEV_BSIZE - 1;
		cur_cyl = newcyl;
	}

#ifdef DEBUG
	printf("dircheck: addr = %ld, lscyl = %ld, lfcyl = %ld, hscyl = %ld, hfcyl = %ld\n",
			addr, lscyl, lfcyl, hscyl, hfcyl);
	printf("newcyl=%d:\n", newcyl);
	printf("cgbase =%ld\n", cgbase(sblock, newcyl));
	printf("cgsblock =%ld\n", cgsblock(sblock, newcyl));
	printf("cgdmin =%ld\n", cgdmin(sblock, newcyl));
	printf("cgbase2 =%ld\n", cgbase(sblock, (newcyl+1)));
#endif

	if ((addr < lscyl) || ((addr > lfcyl)&&(addr < hscyl)) || (addr > hfcyl))
	{
		printf(">>> block is not in data-block region\n");
		error++;
		return(1);
	}
	return(0);
}


/*========
 * puta - put ascii characters into a buffer.  The string
 * terminates with a quote or newline.  The leading quote,
 * which is optional for directory names, was stripped off
 * by the assignment case in the main loop.  If the type
 * indicates a directory name, the entry is null padded to
 * DIRSIZE bytes.  If more than 14 characters have been given
 * with this type or, in any case, if a block overflow
 * occurs, the current block is made invalid. See the
 * description for err.
 */

puta()
{
struct direct *dirp;
#ifdef LONGFILENAMES
struct direct *newdirp, *prevdirp;
long newoffset, tempaddr;
int needsize;
#endif LONGFILENAMES
register char *bptr, c;
register offset;
char	*sbptr;
char	charbuf[MAXNAMLEN+1], *bufptr;
int	buflen, maxnamelen;
long	dblock;
long	s_err,nbytes;

if(type == DIR) {
#ifdef LONGFILENAMES
	if (longfiles) {
		/* Point to d_name */
		addr = (addr & ~03) + 8;
		maxnamelen = MAXNAMLEN;
	}
	else {	/* Point to d_name in short filesystem directory */
		addr = (addr & ~037) + 8;
		maxnamelen = DIRSIZ_CONSTANT;
	}
#else not LONGFILENAMES
	addr = (addr & ~037) + 8;
	maxnamelen = DIRSIZE;
#endif not LONGFILENAMES
}

if((sbptr = bread(addr)) == 0) return;
char_flag++;
#if defined(SecureWare) && defined(B1)
if( ISB1 )
	sbptr = (char *) (addr & ~ADRMSK);
#endif
offset = addr & ADRMSK;
bptr = sbptr + offset;

bufptr = charbuf;

buflen = 0;

while((c = getc(stdin)) != '"')
{
	buflen++;

	if (offset + buflen >= DEV_BSIZE)
	{
		bhdr.fwd->valid = 0;
		error++;
		char_flag = 0;
		printf("block overflow\n");
		return;
	}
	if(c == '\n')
	{
		ungetc(c,stdin);
		buflen--;
		break;
	}
	if(c == '\\')
	{
			switch(c = getc(stdin))
			{
			case 't': *bufptr++ = '\t'; break;
			case 'n': *bufptr++ = '\n'; break;
			case '0': *bufptr++ = '\0'; break;
			default: *bufptr++ = c; break;
			}
	}
		else *bufptr++ = c;
} /* while */

charbuf[buflen] = '\0';

char_flag = 0;
if(type == DIR)
{
	dirp = (struct direct *)(bptr - 8);

#ifdef LONGFILENAMES
	if (longfiles)
	{
		/* Need 4 bytes for d_ino, 2 for d_reclen, 2 for d_namelen and
		 * one for each character in the name, 1 for terminating null
		 * rounded up to the nearst long boundary. */
		needsize = 8 + (buflen + 1 + 3) & ~ 3;

		if (dirp->d_reclen < needsize)
		{
			if ((newoffset =
				finddirect((struct direct *)sbptr, needsize)) ==
				    -1)
			{
				bhdr.fwd->valid = 0;
				error++;
				char_flag = 0;
				printf("not enough space\n");
				return;
			}

			addr -= 8;
			if (dirbackward(1))
			{
				error++;
				char_flag = 0;
				printf("block overflow\n");
				return;
			}

			/* Deallocate directory entry */
			prevdirp = (struct direct *)(sbptr + (addr & ADRMSK));
			prevdirp->d_reclen += dirp->d_reclen;

			offset = newoffset + 8;	/* Point to d_name. */
			bptr = sbptr + offset;

			newdirp = (struct direct *)(bptr - 8);
			newdirp->d_ino = dirp->d_ino;
			dirp = newdirp;

			addr = (addr & ~ADRMSK) + newoffset;
		}
		else
			addr -= 8;
	}
#endif LONGFILENAMES

	if (buflen >= maxnamelen)
	{
		bhdr.fwd->valid = 0;
		error++;
		char_flag = 0;
		printf("name too long\n");
		return;
	}

	(void)strcpy(bptr, charbuf);

	dirp->d_namlen = buflen;

	bptr += buflen;

#ifdef LONGFILENAMES
    if (!longfiles)
#endif LONGFILENAMES
	while(++buflen < DIRSIZE) *bptr++ = '\0';
}
else (void)strcpy(bptr, charbuf);

#if defined(SecureWare) && defined(B1)
if( !ISB1 )
	dblock=btodb(addr);
if((ISB1 ) ? ((s_err = lseek(fd, addr - blkoff(sblock, addr), 0)) == -1) :
   	     ((s_err = lseek(fd, (long) dbtob(dblock), 0)) == -1))
#else
dblock=btodb(addr);
if((s_err = lseek(fd, (long) dbtob(dblock), 0)) == -1)
#endif
{
	error++;
	printf("seek error : 0%lo\n",addr);
	return;
}
#if defined(SecureWare) && defined(B1)
if((ISB1) ?
   ((nbytes = write(fd,sbptr - blkoff(sblock,(long)sbptr),sblock->fs_bsize))
  							!= sblock->fs_bsize) :
   ((nbytes = write(fd,sbptr,fragsize)) != fragsize))
#else
if((nbytes = write(fd,sbptr,fragsize)) != fragsize)
#endif
{
	error++;
	printf("write error : addr  = 0%lo\n",addr);
	printf("           : s_err  = 0%lo\n",s_err);
	printf("           : nbytes = 0%lo\n",nbytes);
	return;
}
return;
}

#ifdef LONGFILENAMES
/*======
 * finddirect - finds a directory entry that can be "size" long.
 * This is called from puta() when we need to move a directory around
 * because the user wants a longer name.  Returns the "addr" address of
 * the new entry.  Returns -1 if a new larger directory entry connot
 * be created.
 */
int
finddirect(base, size)
long base;
int size;
{
struct direct *dirp;
int resoffset = -1;
int avail, using, done, diroffset;

dirp = (struct direct *)base;
diroffset = 0;

for (done = 0; done != 1; )
{
	using = DIRSIZ(dirp);

	if (diroffset >= DEV_BSIZE)
	{
		return(-1);
	}

	avail = dirp->d_reclen - using;
	if (avail >= size)
	{
		done = 1;
		dirp->d_reclen = using;
		resoffset = diroffset + using;
		dirp = (struct direct *)(base + resoffset);
		dirp->d_ino = 0;
		dirp->d_reclen = avail;
		dirp->d_namlen = 0;
	}
	else
	{
		diroffset += dirp->d_reclen;
		dirp = (struct direct *)((char *)dirp + dirp->d_reclen);
	}
}
return (resoffset);
}
#endif LONGFILENAMES


/*=======
 * fprnt - print data as characters, octal or decimal words, octal
 * bytes, directories or inodes	A total of "count" entries
 * are printed. A zero count will print all entries to the
 * end of the current block.  If the printing would cross a
 * block boundary, the attempt is aborted and an error returned.
 * This is done since logically sequential blocks are generally
 * not physically sequential.  The error address is set
 * after each entry is printed.  Upon completion, the current
 * address is set to that of the last entry printed.
 */

fprnt(style,count)
register char style;
register count;
{
short offset;
char *cptr;
short *iptr;
short *tptr;
struct direct *dirp;
int cyl;
long fsblk, dsblk;
#if defined(SecureWare) && defined(B1)
short blkoffset;
#endif

prt_flag++;
offset = addr & ADRMSK;
#ifdef DEBUG
printf("fprnt: addr=%ld, offset=%d, count=%d\n", addr, offset, count);
#endif

if((cptr = bread(addr)) == 0){
	ip = 0;
	iptr = 0;
	dirp = 0;
	return;
}
ip = ((struct dinode *)cptr);
#if defined(SecureWare) && defined(B1)
/* correct to fragment offset */
if( ISB1 )
	cptr += blkoff(sblock,addr) & ~ADRMSK;
#endif
erraddr = addr;

switch (style) {

case 'c': /* print as characters */
case 'b': /* or octal bytes */
	if(count == 0) count = DEV_BSIZE - offset;
	if(offset + count > DEV_BSIZE) break;
	objsz = 1;
	objtype = OBJ_BYTE;
	cptr = cptr + offset;
	for(i=0; count--; i++) {
		if(i % DIRSZ == 0)
		{
			if (hex)
				printf("\n0x%-12lx: ", addr);
			else
				printf("\n%0-12lo: ",addr);
		}
		if(style == 'c') putf(*cptr++);
		else
		{
			if (hex)
				printf("0x%-8x",*cptr++ & 0377);
			else
				printf("0%-8o",*cptr++ & 0377);
		}
		erraddr = addr;
		addr++;
	}
	addr--;     /* addr points to address of last item*/
	putc('\n',stdout);
	return;

case 'd': /* print as directories from beginning block to offset */
	if(dircheck()) return;
#ifdef LONGFILENAMES
    if (longfiles)
    {
	long lastaddr;		/* Last valid addr in this block. */
	long i, ptr;		/* Used to find i'th entry. */

	addr = addr & ~03;	/* Direct entries must be long aligned. */
	if(count == 0) count = DEV_BSIZE/12;	/* Max number of entries. */
	type = DIR;
	objsz = DIRSZ;
	objtype = OBJ_DIR;
	lastaddr = (addr & ~ADRMSK) + DEV_BSIZE;

	for(dirp = ((struct direct *)cptr), ptr = addr, i = 0; offset > 0;
	    dirp = (struct direct *)((char *)dirp + dirp->d_reclen),
	    i++, offset -= dirp->d_reclen)
	{
		if (lastaddr <= ptr)
		{
			erraddr = ptr;
			break;
		}
		ptr += dirp->d_reclen;
	}

	if (ptr >= lastaddr)
		break;

	while( count-- ) {
		if (hex)
			printf("d%d: 0x%-4x  ", i++, dirp->d_ino);
		else
			printf("d%d: %-4d  ",i++, dirp->d_ino);
		cptr = dirp->d_name;
		for(j=0; j<dirp->d_namlen; j++) {
			if(*cptr == '\0') break;
			putf(*cptr++);
		}
		putc('\n',stdout);
		erraddr = addr;
		objsz = dirp->d_reclen;
		addr = addr + objsz;

		dirp = (struct direct *)((char *)dirp + objsz);

		if (addr >= lastaddr)
			break;
	}
	addr = erraddr;
    }
    else
    {
#endif LONGFILENAMES
	addr = addr & ~037;
	offset = offset / DIRSZ;
	if(count == 0) count = ND - offset;
	if(count + offset > ND) break;
	type = DIR;
	objsz = DIRSZ;
	objtype = OBJ_DIR;
	for(dirp = (struct direct *)(cptr + (offset * DIRSZ)); count--; ) {
		if (hex)
			printf("d%d: 0x%-4x  ", offset++, dirp->d_ino);
		else
			printf("d%d: %-4d  ",offset++,dirp->d_ino);
		cptr = dirp->d_name;
		for(j=0; j<DIRSZ; j++) {
			if(*cptr == '\0') break;
			putf(*cptr++);
		}
		putc('\n',stdout);
		erraddr = addr;
#ifdef LONGFILENAMES
		addr = addr + DIRSTRCTSIZ;

		dirp = (struct direct *)((char *)dirp + DIRSTRCTSIZ);
#else not LONGFILENAMES
		addr = addr + sizeof(struct direct);

		dirp++;
#endif not LONGFILENAMES
	}
	addr = erraddr;
#ifdef LONGFILENAMES
    }
#endif LONGFILENAMES
	return;

case 'x': /* printf as hexadecimal words */
case 'o': /* print as octal words */
case 'e': /* print as decimal words */
	addr = addr & ~01;
	offset = offset >> 1;
	iptr = ((short *)cptr) + offset;
	if(count == 0) count = DEV_BSIZE / 2 - offset;
	if(offset + count > DEV_BSIZE / 2) break;
	objsz = 2;
	objtype = OBJ_WORD;
	for(i=0; count--; i++) {
		if(i % 8 == 0) {

			/*  this code deletes lines of zeros  */
			tptr = iptr;
			k = count -1;	/* always print last line */
			for(j = i; k-- > 0; j++)
			if(*tptr++ != 0) break;
			if(j > (i + 7)) {
				j = (j - i) >> 3;
				while(j-- > 0){
					iptr = iptr + 8;
					count = count - 8;
					i = i + 8;
					addr = addr + 16;/*hn:???*/
				}
				printf("\n*");
			}
			if (hex)
				printf("\n0x%-6lx:", addr);
			else
				printf("\n0%-6lo:",addr);
		}
		switch (style){
			case 'x':
				printf("  0x%-6x", *iptr++ & 0177777);
				break;
			case 'o':
				printf("  0%-6o",*iptr++ & 0177777);
				break;
			default:
	           		printf("  %6d",*iptr++);
				break;
			}
		erraddr = addr;
		addr = addr + 2;
	}
	addr = erraddr;
	putc('\n',stdout);
	return;

case 'i': /* print as inodes */
	if(icheck(addr)) return;
#if defined(SecureWare) && defined(B1)
	if( ISB1 ){
	    /* find inode offset from the beginning of the block */
	    blkoffset = blkoff(sblock, addr);
	    addr = (addr - blkoffset) +
	  	(blkoffset / disk_dinode_size()) * disk_dinode_size();
	    offset = blkoffset/disk_dinode_size();
	    if (count == 0) count = INOPB(sblock) - offset;
	    if (count + offset > INOPB(sblock)) break;
	}
	else{
	    addr = addr & ~0177;
#ifdef DEBUG
printf("case i:offset in block=%d\n", offset);
#endif
	    offset = offset / sizeof(struct dinode);
	    if(count == 0) count = NI - offset;
	    if(count + offset > NI) break;
	}
#else
	addr = addr & ~0177;
#ifdef DEBUG
printf("case i:offset in block=%d\n", offset);
#endif
	offset = offset / sizeof(struct dinode);
	if(count == 0) count = NI - offset;
	if(count + offset > NI) break;
#endif
	type = INODE;
#if defined(SecureWare) && defined(B1)
	if( ISB1 )
	    objsz = disk_dinode_size();
	else
	    objsz = INOSZ;
#else
	objsz = INOSZ;
#endif
	objtype = OBJ_INO;
#if defined(SecureWare) && defined(B1)
	if( ISB1 )
	    disk_inode_incr(&ip, offset);
	else
	    ip = ip + offset;
#else
	ip = ip + offset;
#endif
	/* get inode number */
        dsblk = btodb(addr);
	fsblk= dbtofsb(sblock,dsblk);
	cyl = dtog(sblock, fsblk);

	temp = cgimin(sblock, cyl);
#ifdef DEBUG
printf("offset of inode table in cyl=%d\n", temp);
printf("dsblk=%d, fsblk=%d\n", dsblk, fsblk);
printf("fprnt; cyl=%d, offset=%d\n", cyl, offset);
#endif

#if defined(SecureWare) && defined(B1)
	if( ISB1 )
	    temp = (cyl*sblock->fs_ipg) +
	  	(fsblk - temp) / sblock->fs_frag * INOPB(sblock) + offset;
	else
	    temp = (cyl*sblock->fs_ipg) +
		((fsblk-temp)*(sblock->fs_fsize/INOSZ))+offset;
	for(i=0; count--; ((ISB1)? (struct dinode *) disk_inode_incr(&ip, 1) : ip++))
#else
	temp = (cyl*sblock->fs_ipg) + ((fsblk-temp)*(sblock->fs_fsize/INOSZ))+offset;
	for(i=0; count--; ip++)
#endif
	{
		if (hex)
			printf("i#:0x%lx  md: ", temp++);
		else
			printf("i#:%ld  md: ",temp++);
		p = " ugtrwxrwxrwx";
		mode = ip->di_mode;
		switch(mode & IFMT) {
		case IFDIR: putc('d',stdout); break;
		case IFCHR: putc('c',stdout); break;
		case IFBLK: putc('b',stdout); break;
#ifdef ACLS
		case IFCONT: putc('C',stdout); break;
#endif /* ACLS */
#ifdef SYMLINKS
		case IFLNK: putc('l',stdout); break;
#endif /* SYMLINKS */
		case IFREG: putc('f',stdout); break;
		case IFIFO: putc('p',stdout); break;
		case IFSOCK: putc('s',stdout); break;
#if defined(RFA) || defined(OLD_RFA)
		case IFNWK: putc('n',stdout); break;
#endif /* RFA || OLD_RFA */
		default: putc('-',stdout); break;
		}
		for(mode = mode << 4; *++p; mode = mode << 1) {
			if(mode & HIBIT) putc(*p,stdout);
			else putc('-',stdout);
		}
#ifndef ACLS
		if (hex)
		{
			printf("  ln:0x%-5x  uid:0x%-5x  gid:0x%-5x",
			ip->di_nlink, ip->di_uid, ip->di_gid);
			printf("  sz: 0x%lx\n", ip->di_size);
		}
		else
		{
			printf("  ln:%5d  uid:%5d  gid:%5d",
			ip->di_nlink,ip->di_uid,ip->di_gid);
			printf("  sz: %8lu\n", ip->di_size);
		}

	        if(((ip->di_mode & IFMT) == IFCHR) ||
		    ((ip->di_mode & IFMT) == IFBLK))
		    {
#else /* ACLS */
		if ((ip->di_mode & IFMT) != IFCONT) 	/* a primary inode */
		{
		    if (hex)
		    {
			printf(" ln:0x%-5x uid:0x%-5x gid:0x%-5x",
			ip->di_nlink, ip->di_uid, ip->di_gid);
			printf(" sz: 0x%lx", ip->di_size);
			printf(" ci:0x%lx\n", ip->di_contin);
		    }
		    else
		    {
			printf(" ln:%5d uid:%5d gid:%5d",
			ip->di_nlink,ip->di_uid,ip->di_gid);
			printf(" sz: %8lu", ip->di_size);
			printf(" ci:%ld\n", ip->di_contin);
		    }
		    if(((ip->di_mode & IFMT) == IFCHR) ||
			((ip->di_mode & IFMT) == IFBLK))
		    {
#endif /* ACLS */
#ifdef CNODE_DEV
			if (hex)
				printf("maj:0x%-6x  min:0x%-6x  cno:0x%-5x  ",
				major(ip->di_rdev), minor(ip->di_rdev),
				ip->di_rsite);
			else
				printf("maj:0%-6o  min:0%-6o  cno:%5d  ",
				major(ip->di_rdev), minor(ip->di_rdev),
				ip->di_rsite);
#else /* no CNODE_DEV */
			if (hex)
				printf("maj:0x%-6x  min:0x%-6x  ",
				major(ip->di_rdev), minor(ip->di_rdev));
			else
				printf("maj:0%-6o  min:0%-6o  ",
				major(ip->di_rdev), minor(ip->di_rdev));
#endif /* CNODE_DEV */
		    }
#ifdef IC_FASTLINK
		    else if ((ip->di_mode & IFMT) == IFLNK &&
			     (ip->di_flags & IC_FASTLINK) != 0)
		    {
			    printf("symlink: %s\n", ip->di_symlink);
		    }
#endif /* IC_FASTLINK */
		    else
		    {
			for(i = 0; i < NDADDR; i++) {
				if (hex)
					printf("a%-2d:0x%-6lx  ",i,ip->di_db[i]);
				else
					printf("a%-2d:%6lu  ",i,ip->di_db[i]);
				if(i%6 == 5) putc('\n',stdout);
			}
			for( ; i < NDADDR + NIADDR; i++) {
				if (hex)
					printf("a%-2d:0x%-6lx  ",
						i, ip->di_ib[i-NDADDR]);
				else
					printf("a%-2d:%6lu  ",
						i, ip->di_ib[i-NDADDR]);
				if(i%6 == 5) putc('\n',stdout);
			}
		    }
#ifdef ACLS
		    putc('\n',stdout);
		    printf("at: %s",ctime(&ip->di_atime));
		    printf("mt: %s",ctime(&ip->di_mtime));
		    printf("ct: %s",ctime(&ip->di_ctime));
		    if(count) putc('\n',stdout);
		}
		else 	/* a continuation inode */
		{
		    cp=(struct cinode *)ip;
		    if (hex)
		    {
			printf("  ln:0x%-5x\n", cp->ci_nlink);
		    }
		    else
		    {
			printf("  ln:%5d\n", cp->ci_nlink);
		    }
		    for(i = 0; i < NOPTENTRIES; i++) {
			if (hex)
				printf("  uid:0x%-5x  gid:0x%-5x  mode:",
				cp->ci_acl[i].uid, cp->ci_acl[i].gid);
			else
				printf("  uid:%5d  gid:%5d  md:",
				cp->ci_acl[i].uid, cp->ci_acl[i].gid);
			printf("%d", cp->ci_acl[i].mode);
			putc('\n',stdout);
		    }
		    putc('\n',stdout);
		    if(count) putc('\n',stdout);
		}
#else /* no ACLS */
		putc('\n',stdout);
		printf("at: %s",ctime(&ip->di_atime));
		printf("mt: %s",ctime(&ip->di_mtime));
		printf("ct: %s",ctime(&ip->di_ctime));
#if defined(SecureWare) && defined(B1)
		if(ISB1)
                    fsdb_extended_inode_print(ip);
#endif
		if(count) putc('\n',stdout);
#endif /* ACLS */
		cur_ino = erraddr = addr;
#if defined(SecureWare) && defined(B1)
		if(ISB1)
		    addr += disk_dinode_size();
		else
		    addr = addr + sizeof(struct dinode);
#else
		addr = addr + sizeof(struct dinode);
#endif
	}
	addr = erraddr;
	return;


default: error++;
	printf("no such print option\n");
	return;
}
error++;
printf("block overflow\n");
}

/*======
 * reload - read new values for fsize, maxcyl, maxi isize
 * and iblock. These are the basiz for most of the error
 * checking procedures.
 */
reload()
{
#ifdef LONGFILENAMES
longfiles= 0;
#endif LONGFILENAMES

/* read in super block information */
if ( lseek(fd, (long)dbtob(super), 0) == -1)
{
	printf(">>> seek to super block failed\n");
	exit(1);
}

if ( read(fd, sbuf, SBSIZE) != SBSIZE)
{
	printf(">>> read super block failed\n");
	exit(1);
}

sblock = ((struct fs *)sbuf);

#ifdef LONGFILENAMES
#if defined(FD_FSMAGIC)
if ((sblock->fs_magic == FS_MAGIC_LFN) || (sblock->fs_featurebits & FSF_LFN))
#else /* not new magic number */
if (sblock->fs_magic == FS_MAGIC_LFN)
#endif /* new magic number */
{
	longfiles = 1;
#ifdef DEBUG
	printf("Long file names on the filesystem.\n");
#endif DEBUG
}
#endif LONGFILENAMES

maxblock = sblock->fs_size;
maxcyl = sblock->fs_ncg;
maxi = sblock->fs_ipg * sblock->fs_ncg;
#if defined(SecureWare) && defined(B1)
if( ISB1 )
	isize = howmany(sblock->fs_ipg, INOPB(sblock)) * sblock->fs_bsize;
else
	isize = sblock->fs_ipg * INOSZ;
#else
isize = sblock->fs_ipg * INOSZ;
#endif
iblock = btodb(isize);
fragsize = sblock->fs_fsize;
for (nshift = 0; nshift /= 2; nshift++);
nmask = nshift -1;
#if defined(SecureWare) && defined(B1)
if(ISB1){
	disk_set_file_system(sblock, sblock->fs_bsize);
	if (FsSEC(sblock))
	    printf("Tagged ");
	else
	    printf("Untagged ");
}
#endif
if (hex)
{
	printf("file system size=0x%lx(frags)   isize/cyl group=0x%lx(Kbyte blocks)\n", maxblock, iblock);
	printf("primary block size=0x%lx(bytes)\n", sblock->fs_bsize);
	printf("fragment size=0x%lx(bytes)\n", sblock->fs_fsize);
	printf("no. of cyl groups=0x%x\n", maxcyl);
}
else
{
	printf("file system size = %ld(frags)   isize/cyl group=%ld(Kbyte blocks)\n", maxblock,iblock);
	printf("primary block size=%ld(bytes)\n", sblock->fs_bsize);
	printf("fragment size=%ld\n", sblock->fs_fsize);
	printf("no. of cyl groups = %d\n", maxcyl);
}
}

/*=======
 * boundcheck - check if two long addresses crosses boundaries of
 * disk blocks.
 * Used to check for block over/under flows when stepping through
 * a file system.
 */

boundcheck(addr1,addr2)
long addr1;
long addr2;
{

	if(override) return(0);
	if (btodb(addr1) == btodb(addr2)) return 0;
	error++;
	printf("block overflow\n");
	return(1);
}

/*
	map a directory fragment count to a physical frag # on disk
		using current cur_ino value.
		temp = bmap(temp / sblock->fs_frag) + (temp % sblock->fs_frag);
 */
long
bmap(dirfrag)
long dirfrag;
{
	long dirdaddr,
	     fragstart;
	int  j, sh;
	long dirblock, foffset;


	dirblock = dirfrag / fragsize;	/* Direct Block value */
	foffset = dirfrag % fragsize;	/* offset within Direct Block */
	addr = cur_ino;
	if (dirblock < NDADDR )
	{
		addr += A0 + dirblock * 4;
		fragstart= get(OBJ_LONG) + foffset;
		dirdaddr=fsbtodb(sblock, fragstart);
		return(nullblk(dirdaddr) ? 0L : dirdaddr);
	}
	sh = 0;
	dirdaddr = 1;
	dirblock -= NDADDR;
	for (j=3; j>0; j--)
	{
		sh += nshift;
		dirdaddr = dirdaddr <<nshift;
		if ( dirblock < dirdaddr)
			break;
		dirblock -=dirdaddr;
	}

	if ( j==0)
	{
		error++;
		printf("file too big\n");
		return(0L);
	}

	/* get disk block number containing the indirect block */
	addr += A0 + (NDADDR * 4) + (NIADDR - j)*4;
	fragstart = get(OBJ_LONG) + foffset;
	dirdaddr=fsbtodb(sblock, fragstart);

	if (nullblk(dirdaddr))
		return(0L);

	for (; j<=3; j++)
	{
		sh -= nshift;
		addr = dbtob(dirdaddr) +  ( 4 * ((dirblock >>sh) & nmask));
		fragstart = get(OBJ_LONG) + foffset;
		dirdaddr = fsbtodb(sblock, fragstart);
		if (nullblk(dirdaddr))
			return(0L);
	}
	return(dirdaddr);
}

/*===========
 */
usage()
{
printf("usage: fsdb special [-b #] [-] \n");
exit(1);
}
