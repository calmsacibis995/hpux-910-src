static char *HPUX_ID = "@(#)$Revision: 66.1 $";
/* $Revision: 66.1 $  */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include "bif.h"
#include "bif2.h"
/* #include <sys/param.h>
   #include <sys/filsys.h>
   #include <sys/fblk.h> */

#define BIF_LOCK	"/tmp/BIF..LCK"
int rmlock();

#define EQ(x,y,z) (strncmp(x,y,z)==0)
struct stat	S;
union {
	struct filsys	s_sblock;
	char buf[BSIZE];
} u_sblock;
#define sblock u_sblock.s_sblock
struct ustat	Fs_info, *Fs;
int	Flg, tflag;
int	fd;
daddr_t	blkno	= 1;
daddr_t	alloc();

extern char * normalize();
char *pname;

main(argc, argv)
char **argv;
{
	register i;
	register char	c;
	int pid,ret,fd;

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

	while(argc > 1 && argv[1][0] == '-') {
		switch(c = argv[1][1]) {
			case 'f':
				Flg++;
				break;

			case 'q':
				break;

			case 't':
				tflag=1;
				break;

			default:
				fprintf(stderr,"bifdf: illegal arg -%c\n",c);
				exit(1);
		}
		argc--;
		argv++;
	}
	for(i = 1; i < argc; ++i) {
		printit(argv[i], "");
	}
	unlink(BIF_LOCK);
	exit(0);
}

printit(dev, fs_name)
char *dev, *fs_name;
{
	extern int errno;

	dev = normalize(dev);
	if((fd = open(dev, 0)) < 0) {
		fprintf(stderr,"bifdf: cannot open %s   errno = %d\n",dev,errno);
		return;
	} 
	sync();
	if(!Flg) {
		Fs = &Fs_info;
		if(stat(dev, &S) < 0) {
bad_dev:
			fprintf(stderr,"bifdf: cannot stat %s\n",dev);
			return;
		}
/*		if((S.st_mode & S_IFMT) != S_IFBLK)
			goto bad_dev;   */
		if(ustat(S.st_rdev, Fs) < 0 || tflag) {
			lseek(fd, (long)BSIZE, 0);
		/*	read(fd, &sblock, sizeof sblock); */
			read(fd, &sblock, BSIZE);
			Fs = (struct ustat *)&sblock.s_tfree;
		} 
		printf("%-8s(%-10s): %8ld blocks%8u i-nodes\n",fs_name,
			dev, Fs->f_tfree * 2, Fs->f_tinode);
		if(tflag)
			printf("                     (%8ld total blocks,%5d for i-nodes)\n",
				sblock.s_fsize * 2, sblock.s_isize);
	}
	else {
		daddr_t	i;

		bread(1L, (char *) &sblock, sizeof(sblock));
		i = 0;
		while(alloc())
			i++;
		printf("%-8s(%-10s): %8ld blocks\n",fs_name, dev, i * 2);
	}
	close(fd);
}

daddr_t
alloc()
{
	int i;
	daddr_t	b;
	struct fblk buf;

	i = --sblock.s_nfree;
	if(i<0 || i>=NICFREE) {
		printf("bad free count, b=%ld\n", blkno);
		return(0);
	}
	b = sblock.s_free[i];
	if(b == 0)
		return(0);
	if(b<sblock.s_isize || b>=sblock.s_fsize) {
		printf("bad free block (%ld)\n", b);
		return(0);
	}
	if(sblock.s_nfree <= 0) {
		bread(b, &buf, sizeof(buf));
		blkno = b;
		sblock.s_nfree = buf.df_nfree;
		for(i=0; i<NICFREE; i++)
			sblock.s_free[i] = buf.df_free[i];
	}
	return(b);
}

bread(bno, buf, cnt)
daddr_t bno;
char *buf;
{
	int n;
	extern errno;

	lseek(fd, bno<<BSHIFT, 0);
	if((n=read(fd, buf, cnt)) != cnt) {
		printf("read error %ld\n", bno);
		printf("count = %d; errno = %d\n", n, errno);
		exit(0);
	}
}

