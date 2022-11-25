/* @(#) $Revision: 70.2 $ */      
/*
 * Modifications:
 *		11/6/91 : AW
 *		Changes to do away with the MAXUSERS limitation
 *		5000 Users support.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/ino.h>
#include <sys/stat.h>
#include <sys/fs.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include "acctdef.h"
#ifdef CDROM
#include <sys/vfs.h>
#include <sys/mount.h>
#endif /* CDROM */
#include <malloc.h>


#define		MAXUSERS	1000
#define		MAXIGN		10
#define		UNUSED		-1
#define		FAIL		-1
#define		MAXNAME		8
#define		SUCCEED		0
#define		TRUE		1
#define		FALSE		0

struct	fs	sblock;
struct	dinode	dinode[MAXIPG];

int	VERBOSE = 0;
FILE	*ufd = 0;
static struct acct **index;
unsigned ino, nfiles;

struct acct  {
	int	uid;
	long	usage;
	char	name [MAXNAME+1];
	struct	acct *fwptr;		/* points to next in list */
};
struct acct *userlist[MAXUSERS];

char	*ignlist[MAXIGN];
int	igncnt = {0};

FILE *pwfile = NULL;
int   pflag=0;         /* flag set to indicate that the passwd file is a user
                          input file instead of /etc/passwd */

char	*cmd;

struct acct **hash();

main(argc, argv, environ)
int argc;
char **argv, **environ;
{
	extern	int	optind;
	extern	char	*optarg;
	register c;
	register FILE	*fd;
	register	rfd;
	struct	stat	sb;
	int	sflg = {FALSE};
	char	*pfile = {"/etc/passwd"};
	int	errfl = {FALSE};
#ifdef CDROM
	struct statfs	fsbuf;
#endif /* CDROM */

	cleanenv( &environ, "LANG", "LANGOPTS", "NLSPATH", 0);
	cmd = argv[0];
	while((c = getopt(argc, argv, "vu:p:si:")) != EOF) switch(c) {
	case 's':
		sflg = TRUE;
		break;
	case 'v':
		VERBOSE = 1;
		break;
	case 'i':
		ignore(optarg);
		break;
	case 'u':
		ufd = fopen(optarg, "a");
		break;
	case 'p':
		pfile = optarg;
		pflag = 1;
		break;
	case '?':
		errfl++;
		break;
	}
	if(errfl) {
		fprintf(stderr, "Usage: %s [-sv] [-p pw_file] [-u file] [-i ignlist] [file ...]\n", cmd);
		exit(10);
	}

	hashinit();
	if(sflg == TRUE) {
		if(optind == argc){
			adduser(stdin);
		} else {
			for( ; optind < argc; optind++) {
				if( (fd = fopen(argv[optind], "r")) == NULL) {
					fprintf(stderr, "%s: Cannot open %s\n", cmd, argv[optind]);
					continue;
				}
				adduser(fd);
				fclose(fd);
			}
		}
	}
	else {
		setup(pfile);
		for( ; optind < argc; optind++) {
#ifdef CDROM
			if (statfsdev(argv[optind],&fsbuf) < 0 || 
					fsbuf.f_fsid[1] != MOUNT_UFS) {
				fprintf(stderr, "%s: %s is not an hfs file system -- ignored\n", cmd, argv[optind]);
				continue;
			}
#endif /* CDROM */
			if( (rfd = open(argv[optind], O_RDONLY)) < 0) {
				fprintf(stderr, "%s: Cannot open %s\n", cmd, argv[optind]);
				continue;
			}
			if(fstat(rfd, &sb) >= 0){
				if ( (sb.st_mode & S_IFMT) == S_IFCHR ||
				     (sb.st_mode & S_IFMT) == S_IFBLK ) {
					ilist(argv[optind], rfd);
				} else {
					fprintf(stderr, "%s: %s is not a special file -- ignored\n", cmd, argv[optind]);
				}
			} else {
				fprintf(stderr, "%s: Cannot stat %s\n", cmd, argv[optind]);
			}
			close(rfd);
		}
	}
	output();
	exit(0);
}

adduser(fd)
register FILE	*fd;
{
	int	usrid;
	long	blcks;
	char	login[MAXNAME+10];
	struct	acct *acptr, **facptr;

	while(fscanf(fd, "%d %s %ld\n", &usrid, login, &blcks) == 3) {
		facptr = hash(usrid);		/* find entry in hash table */
		if(*facptr == NULL) {		/* add new entry */
			/* allocate space */
			acptr = (struct acct *) calloc(1, sizeof(struct acct));
			if(acptr == NULL) {
				fprintf(stderr, "diskusg: Out of memory\n");
				exit(2);
			}
			*facptr = acptr;	/* update entry */
			acptr->fwptr = NULL;
			acptr->uid = usrid;
			strncpy(acptr->name, login, MAXNAME);
		} else
			(*facptr)->usage += blcks;
	}
}

ilist(file, fd)
char	*file;
register fd;
{
	register dev_t dev;
	register i, j;

	if (fd < 0 ) {
		return (FAIL);
	}

	sync();

	dev = BBSIZE;		/* fake out for now */

	/* Read in super-block of filesystem */
	bread(fd, 1, &sblock, sizeof(sblock), dev);

	/* Check for filesystem names to ignore */
/*	if(!todo(sblock.s_fname))
		return;                                    */
	/* get actual filesystem blocksize */
	dev = sblock.fs_fsize ;	/* fragsize, since cgimin et.al are frags */

	for (i = 0; i < sblock.fs_ncg; i++) {
		bread(fd, cgimin(&sblock,i), dinode, sblock.fs_ipg*sizeof(struct dinode), dev);	/* read cgp inode area */
		for (j = 0; j < sblock.fs_ipg; j++) {
#ifdef IFCONT  /* this system has continuation inodes -- ACLS */
 			if ((dinode[j].di_mode & S_IFMT) && 
 			    ((dinode[j].di_mode & S_IFMT) != IFCONT))  /* skip cont. inodes */
#else /* no continuation inodes */
 			if ((dinode[j].di_mode & S_IFMT))
#endif
				if(count(j, dev) == FAIL) {
					if(VERBOSE)
						fprintf(stderr,"BAD UID: file system = %s, inode = %u, uid = %u\n",
					    	file, j+i*sblock.fs_ipg, dinode[j].di_uid);
					if(ufd)
						fprintf(ufd, "%s %u %u\n", file, j+i*sblock.fs_ipg, dinode[j].di_uid);
				}
		}
	}
	return (0);
}

	char	*skip();
ignore(str)
register char	*str;
{

	for( ; *str && igncnt < MAXIGN; str = skip(str), igncnt++)
		ignlist[igncnt] = str;
	if(igncnt == MAXIGN) {
		fprintf(stderr, "%s: ignore list overflow. Recompile with larger MAXIGN\n", cmd);
	}
}

bread(fd, bno, buf, cnt, dev)
register fd;
register unsigned bno;
register struct  dinode  *buf;
register dev_t dev;
{
	lseek(fd, (long)bno*dev, 0);
	if (read(fd, buf, cnt) != cnt)
	{
		fprintf(stderr, "read error %u\n", bno);
		exit(1);
	}
}

count(j, dev)
register j;
register dev_t dev;
{
	if ( dinode[j].di_nlink == 0 || dinode[j].di_mode == 0 )
		return(SUCCEED);
	index = hash(dinode[j].di_uid);
	if(*index == NULL) return(FAIL);		/* not found */
	(*index)->usage += BLOCKS(dinode[j].di_blocks);
	return (SUCCEED);
}

output()
{
int i;
struct acct *acptr;

	for (i=0; i < MAXUSERS; i++) {
		acptr = userlist[i];
		while(acptr != NULL) {	/* print all entries */
			if(acptr->usage != 0)
				printf("%u	%s	%ld\n",
				    acptr->uid,
				    acptr->name,
				    acptr->usage);
			acptr = acptr->fwptr;
		}
	}
}

#define SNGLIND(dev)	(FsNINDIR(dev))
#define DBLIND(dev)	(FsNINDIR(dev)*FsNINDIR(dev))
#define	TRPLIND(dev)	(FsNINDIR(dev)*FsNINDIR(dev)*FsNINDIR(dev))


struct acct **
hash(j)
int j;
{
register struct acct **q;

	q = &userlist[j % MAXUSERS];	/* get hash index */
	/* search linked list */
	while(*q != NULL) {
		if (j == (*q)->uid)
			return(q);	/* found entry */
		q = &((*q)->fwptr);
	}
	return(q);			/* return pointer address */
}

hashinit() {
int i;
	for(i=0; i < MAXUSERS; i++)
		userlist[i] = NULL;
}

setup(pfile)
char	*pfile;
{
	register struct passwd	*pw;
	struct acct *acptr;

/* Since getpwent supports yellowpages and fgetpwent does not, so getpwent */
/* is used instead of fgetpwent, except in the case when the user supply   */
/* the passwd file instead of the /etc/passwd file. dts: FSDlj08988        */

	if (pflag) {
		if(setpwent2(pfile)) {
			fprintf(stderr, "%s: Cannot open %s\n", cmd, pfile);
			exit(5);
		}
		pw=fgetpwent(pwfile);
	} else
		pw=getpwent();

	while ( pw != NULL )
	{
		index = hash(pw->pw_uid);
		if(*index == NULL)
		{
			/* allocate buffer */
			acptr = (struct acct *) calloc(1, sizeof(struct acct));
			if(acptr == NULL) {
				printf("diskusg: Out of memory\n");
				exit(3);
			}
			*index = acptr;			/* update entry */
			acptr->fwptr = NULL;
			acptr->uid = pw->pw_uid;
			strncpy(acptr->name, pw->pw_name, MAXNAME);
		}
		if (pflag)
			pw=fgetpwent(pwfile);
		else
			pw=getpwent();
	}
}

todo(fname)
register char	*fname;
{
	register	i;

	for(i = 0; i < igncnt; i++) {
		if(strncmp(fname, ignlist[i], 6) == 0) return(FALSE);
	}
	return(TRUE);
}

char	*
skip(str)
register char	*str;
{
	while(*str) {
		if(*str == ' ' ||
		    *str == ',') {
			*str = '\0';
			str++;
			break;
		}
		str++;
	}
	return(str);
}


int
setpwent2(pfile)
register char *pfile;
{
	if(pwfile == NULL)
		pwfile = fopen(pfile, "r");
	else
		rewind(pwfile);
	return(pwfile==NULL);
}

