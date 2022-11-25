static char *HPUX_ID = "@(#) $Revision: 27.2 $";
/*	mkfs	COMPILE:	cc -O mkfs.c -s -i -o mkfs
 * Make a file system prototype.
 * usage: mkfs filsys proto/size [ m n ]
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include "bif.h"
#include "bif2.h"
/* #include <a.out.h>
   #include <sys/param.h>
   #include <sys/ino.h>
   #include <sys/inode.h>
   #include <sys/filsys.h>
   #include <sys/fblk.h>
   #include <sys/dir.h>
   #include <sys/mknod.h>
   #include <sys/stat.h>
   #include <volhdr.h>  */


#define BIF_LOCK	"/tmp/BIF..LCK"

#define	NINDIR	(BSIZE/sizeof(daddr_t))
#define	NFB	(NINDIR+500)	/* NFB must be greater than NINDIR+LADDR */
#define	NDIRECT	(BSIZE/sizeof(struct direct))
#define	NBINODE	(BSIZE/sizeof(struct dinode))
#define	LADDR	10
#define	MAXFN	500

time_t	utime;
FILE 	*fin;
int	fsi;
int	fso;
char	*charp;
char	buf[BSIZE];

char work0[BSIZE];
struct fblk *fbuf = (struct fblk *)work0;

struct exec head;
char	string[50];

char work1[BSIZE];
struct filsys *filsys = (struct filsys *)work1;
char	*fsys;
char	*proto;
int	f_n = MAXFN;
int	f_m = 3;
int	error;
ino_t	ino;
long	getnum();
daddr_t	alloc();

extern char * normalize();
char *pname;

int rmlock();

main(argc, argv)
int argc;
char *argv[];
{
	int f, c;
	long n, nb;
	struct stat statbuf;
	int pid,ret,fd;

	/*
	 * open relevent files
	 */

	time(&utime);
	if(argc < 3) {
		printf("usage: mkfs filsys proto/size [ m n ]\n");
		exit(1);
	}

	signal(SIGQUIT, rmlock);
	signal(SIGTERM, rmlock);
	signal(SIGPIPE, rmlock);
	signal(SIGINT, rmlock);
	signal(SIGHUP, rmlock);
	while(1) 
	{
		if(access(BIF_LOCK,0) < 0)  /* lock file doesn't exist */
		{
		    fd = open(BIF_LOCK, O_WRONLY|O_EXCL|O_CREAT, 0666);
		    if(fd < 0)
		    {
			printf("can't create lock file %s\n",BIF_LOCK);
			exit(-1);
		    }
		    pid = getpid();
		    write(fd, (char *) &pid, sizeof(int));
		    close(fd);
		    break;
		}
		else				/* lock file does exist */
		{
		    fd = open(BIF_LOCK, O_RDONLY);
		    if(fd < 0)
		    {
			printf("can't open lock file %s\n",BIF_LOCK);
			exit(-1);
		    }
		    ret = read(fd, (char *) &pid, sizeof(int));
		    close(fd);
		    if(ret == 0)
			continue;
		    if( kill(pid,0) < 0 && errno == ESRCH)
		    {
			unlink(BIF_LOCK);
			continue;
		    }
		    printf("somebody else has bif utilities locked\n");
		    exit(-1);	/* don't continue if exists and process active on it */
		}
	}	/* end while lock loop */

	fsys = argv[1];
	proto = argv[2];
	fso = creat(fsys, 0666);
	if(fso < 0) {
		printf("%s: cannot create\n", fsys);
		exit(1);
	}
	fsi = open(fsys, 0);
	if(fsi < 0) {
		printf("%s: cannot open\n", fsys);
		exit(1);
	}

	/* load the header (indicating no boot pgm) */
	((struct HPUX_vol_type *)buf)->HPUXid = HPUXID;
	((struct HPUX_vol_type *)buf)->HPUXowner = OWNERID;
	((struct HPUX_vol_type *)buf)->HPUXexecution_addr = 0;
	((struct HPUX_vol_type *)buf)->HPUXboot_start_sector = 0;
	((struct HPUX_vol_type *)buf)->HPUXboot_byte_count = 0;
	strncpy(&(((struct HPUX_vol_type *)buf)->HPUXfilename[1]), "HP-UX", 15);
	((struct HPUX_vol_type *)buf)->HPUXfilename[0] = (char) 5;

	fin = fopen(proto, "r");
	if(fin == NULL) {
		n = 0;
		for(f=0; c=proto[f]; f++) {
			if(c<'0' || c>'9') {
				if(c == ':') {
					nb = n;
					n = 0;
					continue;
				}
				printf("%s: cannot open\n", proto);
				exit(1);
			}
			n = n*10 + (c-'0');
		}
		if(!nb) {
			nb = n;
			n = n/(NBINODE*4);
		} else {
			n /= NBINODE;
		}
		filsys->s_fsize = nb;
		if(n <= 0)
			n = 1;
		if(n > 65500/NBINODE)
			n = 65500/NBINODE;
		filsys->s_isize = n + 2;
		printf("isize = %ld\n", n*NBINODE);
		charp = "d--777 0 0 $ ";
		goto f3;
	}

	/*
	 * get name of boot load program
	 * and read onto block 0
	 */

	getstr();
	if (strcmp(string,"/dev/null") == 0)
		goto f2;
	f = open(string, 0);
	if(f < 0) {
		printf("%s: cannot  open init\n", string);
		goto f2;
	}
	read(f, (char *)&head, sizeof head);
	if(head.a_magic.file_type != EXEC_MAGIC ||
	   head.a_magic.system_id != HP98x6_ID) {
		printf("%s: bad format\n", string);
		goto f1;
	}
	c = head.a_text + head.a_data;
	if(c > BSIZE-(BSIZE/4)) {
		printf("%s: too big\n", string);
		goto f1;
	}

	/* put the load header in place */
	((struct load *)(&buf[BSIZE/4]))->address = head.a_entry;
	((struct load *)(&buf[BSIZE/4]))->count = c;

	/* then the code */
	read(f, &buf[BSIZE/4 + sizeof(struct load)], c);

	/* and update the volume header to show that there is code */
	((struct HPUX_vol_type *)buf)->HPUXboot_byte_count =
		c + sizeof(struct load);
	((struct HPUX_vol_type *)buf)->HPUXexecution_addr = head.a_entry;
	((struct HPUX_vol_type *)buf)->HPUXboot_start_sector = 1;

f1:
	close(f);

	/*
	 * get total disk size
	 * and inode block size
	 */

f2:
	filsys->s_fsize = getnum();
	n = getnum();
	n /= NBINODE;
	filsys->s_isize = n + 3;

f3:
	/* write the volume header, with or without boot */
	wtfs((long)0, buf);

	fstat(fso,&statbuf);
	if ((statbuf.st_mode & IFMT) == IFCHR
	   || (statbuf.st_mode & IFMT) == IFBLK) {
		lseek(fsi, (filsys->s_fsize-1)*BSIZE, 0);
		n = read(fsi, buf, BSIZE);
		if (n != BSIZE) {
			printf("Cannot read last block.");
			exit(1);
		}
	}
	if(argc >= 5) {
		f_m = atoi(argv[3]);
		f_n = atoi(argv[4]);
		if(f_n <= 0 || f_n >= MAXFN)
			f_n = MAXFN;
		if(f_m <= 0 || f_m > f_n)
			f_m = 3;
	}
	filsys->s_dinfo[0] = f_m;
	filsys->s_dinfo[1] = f_n;
	printf("m/n = %d %d\n", f_m, f_n);
	if(filsys->s_isize >= filsys->s_fsize) {
		printf("%ld/%ld: bad ratio\n", filsys->s_fsize, filsys->s_isize-2);
		exit(1);
	}
	filsys->s_tinode = 0;
	for(c=0; c<BSIZE; c++)
		buf[c] = 0;
	for(n=2; n!=filsys->s_isize; n++) {
		wtfs(n, buf);
		filsys->s_tinode += NBINODE;
	}
	ino = 0;

	bflist();

	cfile((struct inode *)0);

	filsys->s_time = utime;
	wtfs((long)1, (char *)filsys);
	unlink(BIF_LOCK);
	exit(error);
}

cfile(par)
struct inode *par;
{
	struct inode in;
	int dbc, ibc;
	char db[BSIZE];
	daddr_t ib[NFB];
	int i, f, c;

	/*
	 * get mode, uid and gid
	 */

	getstr();
	in.i_mode  = gmode(string[0], "-bcd", IFREG, IFBLK, IFCHR, IFDIR);
	in.i_mode |= gmode(string[1], "-u", 0, ISUID, 0, 0);
	in.i_mode |= gmode(string[2], "-g", 0, ISGID, 0, 0);
	for(i=3; i<6; i++) {
		c = string[i];
		if(c<'0' || c>'7') {
			printf("%c/%s: bad octal mode digit\n", c, string);
			error = 1;
			c = 0;
		}
		in.i_mode |= (c-'0')<<(15-3*i);
	}
	in.i_uid = getnum();
	in.i_gid = getnum();

	/*
	 * general initialization prior to
	 * switching on format
	 */

	ino++;
	in.i_number = ino;
	for(i=0; i<BSIZE; i++)
		db[i] = 0;
	for(i=0; i<NFB; i++)
		ib[i] = (daddr_t)0;
	in.i_nlink = 1;
	in.i_size = 0;
	for(i=0; i<NADDR; i++)
		in.i_faddr[i] = (daddr_t)0;
	if(par == (struct inode *)0) {
		par = &in;
		in.i_nlink--;
	}
	dbc = 0;
	ibc = 0;
	switch(in.i_mode&IFMT) {

	case IFREG:
		/*
		 * regular file
		 * contents is a file name
		 */

		getstr();
		f = open(string, 0);
		if(f < 0) {
			printf("%s: cannot open\n", string);
			error = 1;
			break;
		}
		while((i=read(f, db, BSIZE)) > 0) {
			in.i_size += i;
			newblk(&dbc, db, &ibc, ib);
		}
		close(f);
		break;

	case IFBLK:
	case IFCHR:
		/*
		 * special file
		 * content is maj/min types
		 */

		i = getnum() & 0377;
		f = getnum() & 0377;
		in.i_rdev = makedev(i,f);
		break;

	case IFDIR:
		/*
		 * directory
		 * put in extra links
		 * call recursively until
		 * name of "$" found
		 */

		par->i_nlink++;
		in.i_nlink++;
		entry(in.i_number, ".", &dbc, db, &ibc, ib);
		entry(par->i_number, "..", &dbc, db, &ibc, ib);
		in.i_size = 2*sizeof(struct direct);
		for(;;) {
			getstr();
			if(string[0]=='$' && string[1]=='\0')
				break;
			entry(ino+1, string, &dbc, db, &ibc, ib);
			in.i_size += sizeof(struct direct);
			cfile(&in);
		}
		break;
	}
	if(dbc != 0)
		newblk(&dbc, db, &ibc, ib);
	iput(&in, &ibc, ib);
}

gmode(c, s, m0, m1, m2, m3)
char c, *s;
int m0,m1,m2,m3;
{
	int i;

	for(i=0; s[i]; i++)
		if(c == s[i])
		{
/*			return((&m0)[i]); */
			switch(i)
			{
			   case 0: return(m0);  break;
			   case 1: return(m1);  break;
			   case 2: return(m2);  break;
			   case 3: return(m3);  break;
			   default: break;
			}
		}
	printf("%c/%s: bad mode\n", c, s);
	error = 1;
	return(0);
}

long getnum()
{
	int i, c;
	long n;

	getstr();
	n = 0;
	for(i=0; c=string[i]; i++) {
		if(c<'0' || c>'9') {
			printf("%s: bad number\n", string);
			error = 1;
			return((long)0);
		}
		n = n*10 + (c-'0');
	}
	return(n);
}

getstr()
{
	int i, c;

loop:
	switch(c=getch()) {

	case ' ':
	case '\t':
	case '\n':
		goto loop;

	case EOF:
		printf("EOF\n");
		exit(1);

	case ':':
		while((c = getch()) != '\n' && c != EOF);
		goto loop;

	}
	i = 0;

	do {
		string[i++] = c;
		c = getch();
	} 
	while(c!=' '&&c!='\t'&&c!='\n'&&c!=EOF);
	string[i] = '\0';
}

rdfs(bno, bf)
daddr_t bno;
char *bf;
{
	int n;

	lseek(fsi, bno*BSIZE, 0);
	n = read(fsi, bf, BSIZE);
	if(n != BSIZE) {
		printf("read error: %ld\n", bno);
		exit(1);
	}
}

wtfs(bno, bf)
daddr_t bno;
char *bf;
{
	int n;

	lseek(fso, bno*BSIZE, 0);
	n = write(fso, bf, BSIZE);
	if(n != BSIZE) {
		printf("write error: %ld\n", bno);
		exit(1);
	}
}

daddr_t alloc()
{
	int i;
	daddr_t bno;

	filsys->s_tfree--;
	bno = filsys->s_free[--filsys->s_nfree];
	if(bno == 0) {
		printf("out of free space\n");
		exit(1);
	}
	if(filsys->s_nfree <= 0) {
		rdfs(bno, (char *)fbuf);
		filsys->s_nfree = fbuf->df_nfree;
		for(i=0; i<NICFREE; i++)
			filsys->s_free[i] = fbuf->df_free[i];
	}
	return(bno);
}

bfree(bno)
daddr_t bno;
{
	int i;

	filsys->s_tfree++;
	if(filsys->s_nfree >= NICFREE) {
		fbuf->df_nfree = filsys->s_nfree;
		for(i=0; i<NICFREE; i++)
			fbuf->df_free[i] = filsys->s_free[i];
		wtfs(bno, (char *)fbuf);
		filsys->s_nfree = 0;
	}
	filsys->s_free[filsys->s_nfree++] = bno;
}

entry(in, str, adbc, db, aibc, ib)
ino_t in;
char *str;
int *adbc, *aibc;
char *db;
daddr_t *ib;
{
	struct direct *dp;
	int i;

	dp = (struct direct *)db;
	dp += *adbc;
	(*adbc)++;
	dp->d_ino = in;
	for(i=0; i<DIRSIZ; i++)
		dp->d_name[i] = 0;
	for(i=0; i<DIRSIZ; i++)
		if((dp->d_name[i] = str[i]) == 0)
			break;
	if(*adbc >= NDIRECT)
		newblk(adbc, db, aibc, ib);
}

newblk(adbc, db, aibc, ib)
int *adbc, *aibc;
char *db;
daddr_t *ib;
{
	int i;
	daddr_t bno;

	bno = alloc();
	wtfs(bno, db);
	for(i=0; i<BSIZE; i++)
		db[i] = 0;
	*adbc = 0;
	ib[*aibc] = bno;
	(*aibc)++;
	if(*aibc >= NFB) {
		printf("file too large\n");
		error = 1;
		*aibc = 0;
	}
}

getch()
{

	if(charp)
		return(*charp++);
	return(getc(fin));
}

bflist()
{
	struct inode in;
	daddr_t ib[NFB];
	int ibc;
	char flg[MAXFN];
	int adr[MAXFN];
	int i, j;
	daddr_t f, d;

	for(i=0; i<f_n; i++)
		flg[i] = 0;
	i = 0;
	for(j=0; j<f_n; j++) {
		while(flg[i])
			i = (i+1)%f_n;
		adr[j] = i+1;
		flg[i]++;
		i = (i+f_m)%f_n;
	}

	ino++;
	in.i_number = ino;
	in.i_mode = IFREG;
	in.i_uid = 0;
	in.i_gid = 0;
	in.i_nlink = 0;
	in.i_size = 0;
	for(i=0; i<NADDR; i++)
		in.i_faddr[i] = (daddr_t)0;

	for(i=0; i<NFB; i++)
		ib[i] = (daddr_t)0;
	ibc = 0;
	bfree((daddr_t)0);
	filsys->s_tfree = 0;
	d = filsys->s_fsize-1;
	while(d%f_n)
		d++;
	for(; d > 0; d -= f_n)
		for(i=0; i<f_n; i++) {
			f = d - adr[i];
			if(f < filsys->s_fsize && f >= filsys->s_isize)
				if(badblk(f)) {
					if(ibc >= NINDIR) {
						printf("too many bad blocks\n");
						error = 1;
						ibc = 0;
					}
					ib[ibc] = f;
					ibc++;
				} else {
					bfree(f);
				}
		}
	iput(&in, &ibc, ib);
}

iput(ip, aibc, ib)
register struct inode *ip;
register int *aibc;
daddr_t *ib;
{
	register struct dinode *dp;
	daddr_t d;
	register int i,j,k;
	daddr_t ib2[NINDIR];	/* a double indirect block */
	daddr_t ib3[NINDIR];	/* a triple indirect block */

	filsys->s_tinode--;
	d = itod(ip->i_number);
	if(d >= filsys->s_isize) {
		if(error == 0)
			printf("ilist too small\n");
		error = 1;
		return;
	}
	rdfs(d, buf);
	dp = (struct dinode *)buf;
	dp += itoo(ip->i_number);

	dp->di_mode = ip->i_mode;
	dp->di_nlink = ip->i_nlink;
	dp->di_uid = ip->i_uid;
	dp->di_gid = ip->i_gid;
	dp->di_size = ip->i_size;
	dp->di_atime = utime;
	dp->di_mtime = utime;
	dp->di_ctime = utime;

	switch(ip->i_mode&IFMT) {

	case IFDIR:
	case IFREG:
		/* handle direct pointers */
		for(i=0; i<*aibc && i<LADDR; i++) {
			ip->i_faddr[i] = ib[i];
			ib[i] = 0;
		}
		/* handle single indirect block */
		if(i < *aibc)
		{
			for(j=0; i<*aibc && j<NINDIR; j++, i++)
				ib[j] = ib[i];
			for(; j<NINDIR; j++)
				ib[j] = 0;
			ip->i_faddr[LADDR] = alloc();
			wtfs(ip->i_faddr[LADDR], (char *)ib);
		}
		/* handle double indirect block */
		if(i < *aibc)
		{
			for(k=0; k<NINDIR && i<*aibc; k++)
			{
				for(j=0; i<*aibc && j<NINDIR; j++, i++)
					ib[j] = ib[i];
				for(; j<NINDIR; j++)
					ib[j] = 0;
				ib2[k] = alloc();
				wtfs(ib2[k], (char *)ib);
			}
			for(; k<NINDIR; k++)
				ib2[k] = 0;
			ip->i_faddr[LADDR+1] = alloc();
			wtfs(ip->i_faddr[LADDR+1], (char *)ib2);
		}
		/* handle triple indirect block */
		if(i < *aibc)
		{
			printf("triple indirect blocks not handled\n");
		}
		ltol3(dp->di_addr, ip->i_faddr, NADDR);
		break;

	case IFBLK:
	case IFCHR:
		dp->di_rdev = ip->i_rdev;
		break;

	default:
		printf("bad mode %o\n", ip->i_mode);
		exit(1);
	}

	wtfs(d, buf);
}

badblk(bno)
daddr_t bno;
{

	return(0);
}

rmlock()	/* remove device lock upon receipt of signal */
{
	register int fd;
	int pid;

	signal(SIGQUIT,SIG_IGN);
	signal(SIGTERM,SIG_IGN);
	signal(SIGPIPE,SIG_IGN);
	signal(SIGINT,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	fd = open(BIF_LOCK, O_RDONLY);
	if(fd < 0)
	     exit(-1);	/* proper behavior ? */
	read(fd, (char *) &pid, sizeof(int));
	if(pid == getpid())
		if(unlink(BIF_LOCK) < 0)
			printf("can't remove lockfile %s!\n",BIF_LOCK);
	exit(-1);
}
