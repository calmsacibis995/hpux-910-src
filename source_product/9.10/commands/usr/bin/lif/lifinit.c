/* @(#) $Revision: 70.1 $ */    

#include <stdio.h>
#include "lifdef.h"
#include "global.h"
#define  PMOD	0644

char *strcpy();

extern int DEBUG;
extern char bigbuf[];
extern char *Fatal[];
extern int environ;

int errflag = 0;
int kflag = 0;

int blksize = 0;		/* blocksize */
int iplstrt = 0;		/* IPL start location */
int ipllen = 0;			/* IPL length */
int kay = 0;			/* number of 1K multiples */

main(argc, argv)
register int argc;
register char *argv[];
{
	register int n, volsize = 0, dirsize = 0, c;
	char volname[100];
	char dirpath[MAXPATHLEN];
	extern char *optarg;
	extern int optind;

	strcpy(volname, "");

	while ((c=getopt(argc, argv, "v:Dd:n:s:l:e:K:"))!=EOF) {
		switch (c) {
		case 'v':
			volsize = atoi(optarg);
			bugout("volsize is %d", volsize);
			break;
		case 'D':
			DEBUG = TRUE;
			break;
		case 'd':
			dirsize = atoi(optarg);
			bugout("dirsize is %d", dirsize);
			break;
		case 'n':
			strcpy(volname, optarg);
			bugout("volname is %s", volname);
			break;
		case 's':			/* IPL start location */
			iplstrt = atoi(optarg);
			bugout("iplstart location is %d\n",iplstrt);
			break;
		case 'l':
			ipllen = atoi(optarg);
			bugout("ipllength is %d\n",ipllen);
			break;
		case 'e':
			blksize = atoi(optarg);
			bugout("blocksize is %d\n",blksize);
			break;
		case 'K':
			kay = atoi(optarg);
			if ( kay <= 0 || kay > 256 ) {
				fprintf(stderr,"lifinit: invalid argument: %d\n",kay);
				exit(-1);
			}

			kflag++;
			break;
		default:
			goto usage;
		}
	}
	argc = argc-optind+1;
	argv = &argv[optind-1];
	if (argc!=2)
		goto usage;

	if ((n = creat(argv[argc-1], 0666)) < 0){
		fprintf(stderr, "lifinit: can not create %s\n", argv[argc-1]);
		exit(-1);
	}
	strcpy(dirpath,argv[argc-1]);
/*	n = lifinit(argv[argc-1], volsize, dirsize, volname); */
	n = lifinit(dirpath, volsize, dirsize, volname);
	if (n != TRUE){
		fprintf(stderr, "lifinit: can not initialize %s; %s\n", argv[argc-1], Fatal[n]);
		exit(-1);
	}
	exit(0);

usage:
	fprintf(stderr, "usage: lifinit [options] file\n");
	fprintf(stderr, "       -v<volume size>\n");
	fprintf(stderr, "       -n<volume name>\n");
	fprintf(stderr, "       -d<directory size>\n");
	fprintf(stderr, "       -s<IPL start location>\n");
	fprintf(stderr, "       -l<IPL length>\n");
	fprintf(stderr, "       -e<IPL entry point offset>\n");
	fprintf(stderr, "       -K<multiple>\n");
	exit(-1);
}


#define TWOMB	2000000

/* volsize in bytes; if 0 default value is used */
/* dirsize in # of files; if 0 default value is used */

lifinit(dirpath, volsize, dirsize, volname)
register char *dirpath;
int volsize;
register int dirsize;
register char *volname;
{
	register int n, i, dirstart;
	register struct lvol *lvolp;
	register struct dentry *dentryp;
	struct lfib frec;
	char *s, *get_volname();

	dirsize = (dirsize % 8) ?  dirsize / 8 + 1 : dirsize / 8;
	if(kflag)
		dirstart = kay * 4;
	else
		dirstart = 2;

	if ((frec.filedis = open(dirpath, 2)) < 0) {
		perror("lifinit(open)");
		return(IOERROR);
	}
	strcpy(frec.dirpath, dirpath);
	if (volsize == 0) {
		if ((n = findvsize(frec.filedis, &volsize)) != TRUE)
			return(n);
	}
	else volsize = (volsize / QRT_K); /* get volsize in terms of sectors */
	if (dirsize == 0) dirsize = get_d_ds(volsize*QRT_K);
	if (volsize < 1 || volsize <= dirsize)
		return(BIGDIRECTORY);
	if (((dirsize +2) > volsize) || (dirsize < 1)){
		return(BIGDIRECTORY);
	}

	n = unitread(frec.filedis, bigbuf, (dirstart +1) * QRT_K, 0, "lifinit");
	if (n == -1)
		return(IOERROR);
	/* zero full buffer for sectors 0 and 1 */
	for (i = 0; i<HALF_K; i++)
		bigbuf[i] = 0;
	lvolp = (struct lvol *)bigbuf;
	lvolp->discid = LIFID;
	if (*volname == '\0')
		s = get_volname(dirpath);
	else    s = volname;
	if (strcmp(s, "      ") != 0) {
		trnsname(s, MAXVOLNAME);
		addb(s, MAXVOLNAME);
	}
	strcpy(lvolp->volname, s);
	lvolp->dstart = dirstart;
	lvolp->dummy1 = 4096;
	lvolp->dummy2 = 0;
	lvolp->dsize = dirsize;
	lvolp->version = 1;
	lvolp->dummy3 = 0;
	lvolp->tps = 1;
	lvolp->spm = 1;
	lvolp->spt = volsize;
	lvolp->iplstart = iplstrt;
	lvolp->ipllength= ipllen;
	lvolp->iplentry= blksize;
	gettime(lvolp->date);			/* get current time */
	dentryp = (struct dentry *)(bigbuf + dirstart * QRT_K);
	strcpy(dentryp->fname, "          ");
	dentryp->ftype = EOD;  /* set logical EOD */
	n = unitwrite(frec.filedis, bigbuf, (dirstart +1) * QRT_K, 0, "lifinit");
	if (n == -1)
		exit(IOERROR);
	if (closeit(frec.filedis) < 0)
		return(IOERROR);
	return(TRUE);
}

get_d_ds(volsize)	/* returns the default directory size */
register int volsize;
{
	register int s;

	if (volsize <= 0)
		return (0);
	if (volsize <= TWOMB)
		s = volsize * 0.013;
	else
		s = volsize * 0.005;

	/* for small volumes min dirsize is 1 sector */
	if (s>0 && s<32)
		s = 32;
	s /= 32;	/* dirsize -- # of files */

	if (s % 8)
		s = s / 8 + 1;
	else
		s = s / 8;
	return(s);
}

char *get_volname(dirpath)
register char *dirpath;
{
	char *strrchr();
	register char *p;

	p = strrchr(dirpath, '/');		/* try to find a slash */
	if (p!=NULL)				/* if we found one */
		return p+1;			/* return the tail string */
	else					/* if no slash found */
		return dirpath;			/* return the entire string */
}
