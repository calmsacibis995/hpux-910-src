/* @(#) $Revision: 66.1 $ */      

/*
 *  lifcp oldfile newfile
 *
 *  Oldfile or newfile are any HP-UX acceptable file path
 *  or is a HP-UX acceptable path followed by :filename.
 *  lifcp - lif:new  will copy the standard input to lif:new.
 *  lifcp lif:old -  will copy the file lif:old to standard output.
 *
 */

#include <ndir.h>		/* for MAXNAMLEN */
#include "lifdef.h"
#include "global.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

char *strcat(), *strcpy();

extern int  errno;
extern char bigbuf[];
extern int DEBUG;
extern char *Fatal[];

char *strip();
struct lfib lfib1, lfib2;
int tflag = FALSE;
int flag = FALSE;
int Implem = 0;
int retcode = TRUE;
char ffile[MAXNAMLEN+1], fakef[MAXNAMLEN+1], tfile[MAXNAMLEN+1];
char fdir[MAXDIRPATH], tdir[MAXDIRPATH], faked[MAXDIRPATH];
char *peel();
struct stat stbuf;

int fromfile = TRUE;	/* no from file */
int Tflag = FALSE;	/* set if filetype is other that ASCII */
int tofile = TRUE;	/* no to file */
int errflag = FALSE;	/* set in getarg */
int filetype = ASCII;	/* default file type unless -T is used */
int copytype = -1;	/* copytype is depedent to file type unless -a, -b or
			   -r are specified */
char Lastvolume = 1;	/* last volume flag is 1 by default */
int lastvolumeset = FALSE;
int volnumberset = FALSE;
short Volnumber = 1;	/* volume number is 1 by default */
int kflag = FALSE;
int kay;
int agc;
char **agv;

main(argc, argv)
register int argc;
register char *argv[];
{
	extern char *optarg;
	extern int optind;

	register int dest_type, c;

	while ((c=getopt(argc, argv, "T:Di:barL:v:tK:"))!=EOF) {
		switch (c) {
		case 'T':
			flag = Tflag = TRUE;
			filetype = number(optarg, &errflag);
			if (errflag) goto usage;
			bugout("filetype is %x", filetype);
			break;
		case 'D':
			flag = DEBUG = TRUE;
			bugout("debugger is on");
			break;
		case 'i':
			Implem = number(optarg, &errflag);
			if (errflag) goto usage;
			bugout("implementation is %x", Implem);
			break;
		case 'b':
			flag = TRUE;
			if (copytype != -1)
				goto usage;
			copytype = BINARY;
			if (!Tflag) filetype = BINARY;
			bugout("copytype is binary");
			break;
		case 'a':
			flag = TRUE;
			if (copytype != -1)
				goto usage;
			copytype = ASCII;
			if (!Tflag) filetype = ASCII;
			bugout("copytype is ascii");
			break;
		case 'r':
			flag = TRUE;
			if (copytype != -1)
				goto usage;
			copytype = RAW_TYPE;
			if (!Tflag) filetype = BIN;
			bugout("copytype is raw");
			break;
		case 'L':
			flag = TRUE;
			Lastvolume = atoi(optarg);
			lastvolumeset = TRUE;
			bugout("Lastvolume flag=%d", Lastvolume);
			break;
		case 'v':
			flag = TRUE;
			Volnumber = number(optarg, &errflag);
			if (errflag) goto usage;
			volnumberset = TRUE;
			if ((unsigned short)Volnumber > 16384) {
				printf("Warning: volume number is an unsigned 14 bit entity\n");
				Volnumber = 16384;
			}
			bugout("volume number is %x", Volnumber);
			break;
		case 't':
			flag = tflag = TRUE;
			bugout("translate is on");
			break;
		case 'K':
			kay = atoi(optarg);
			if (kay <= 0 ) {
				fprintf(stderr,"lifcp: invalid argument: %d\n",kay);
				exit(-1);
				} 
			kflag++;
			break;
		default:
			goto usage;
		}
	}
	agc = argc-optind+1;
	agv = &argv[optind-1];

	if (agc<=1)
		goto usage;

	/* check for valid Implem field */
	if (filetype > -02001 && filetype < -0100000 && Implem) {
		fprintf(stderr, "Warning: Implementation field can only be set for file types -2001 to -100000 (octal)\n");
		Implem = 0;
	}

	dest_type = whatis(argv[argc-1], tdir, tfile, 1, &tofile, argc);
	if (dest_type==1)				/* if lif destination */
		retcode = copyto_lif(argv, argc);	/* whatever to lif */
	else						/* to unix/stdout */
		retcode = copyto_ws(dest_type, argc, argv);

	exit(retcode==TRUE ? 0 : -1);

usage :
	fprintf(stderr, "usage:	lifcp [options] src dst\n");
	fprintf(stderr, "	-L<last volume>\n");
	fprintf(stderr, "	-T<file type>\n");
	fprintf(stderr, "	-v<volume number>\n");
	fprintf(stderr, "	-i<implementation field>\n");
	fprintf(stderr, "	-a ascii mode\n");
	fprintf(stderr, "	-b binary mode\n");
	fprintf(stderr, "	-r raw mode\n");
	fprintf(stderr, "	-t translate file names\n");
	fprintf(stderr, "       -K<multiple>\n");
	exit(-1);
}

number (string, err)
register char *string;
register int *err;
{
	register int	base = 10;		/* selected base	*/
	register long	num  =  0;		/* function result	*/
	register int    negflag = 0;		/* minus sign encountered */
	register int	digit;			/* current digit	*/

	if (*string == '-') {		/* check for negative file type */
		negflag++;
		string++;
		}

	if (*string == '0') {			/* determine base */
		string++;			/* skip the 0 */
		if (*string == 'x' || *string == 'X') {
			base = 16;
			string++;
		}
		else
			base = 8;
	}

	while (digit = *string++) {
		if (base == 16) {		/* convert hex a-f or A-F */
			if ((digit >= 'a') && (digit <= 'f'))
				digit += '0' + 10 - 'a';
			else
			if ((digit >= 'A') && (digit <= 'F'))
				digit += '0' + 10 - 'A';
		}
		digit -= '0';

		if ((digit < 0) || (digit >= base)) {	/* out of range */
			fprintf (stderr, "lifcp: illegal number\n");
			*err = TRUE;
			return(0);
		}
		num = num*base + digit;
	}
	if(negflag)
		num = -num;
	return (num);
}


/*
 * Copy all the work station files to a lif volume.
 */
copyto_lif(argv, argc)
char *argv[];
register int argc;
{

	register int k;
	register int rc = TRUE;

	for( ; agv < (argv + argc - 2); agv++, agc--) {
		k = whatis(agv[1], fdir, ffile, 0, &fromfile, argc);
		if (k==1) {
			if (tofile == FALSE) strcpy(tfile, ffile);
			addb(tfile, MAXFILENAME);
			rc = lifcpll();
		}
		else if (k == 2) { /* standard input to lif */
			if (tofile == FALSE) strcpy(tfile, "STDIN");
			addb(tfile, MAXFILENAME);
			rc = lifcpul(strip(agv[1]), 1); /* standard input to lif */
		}
		else {
			if (fromfile == FALSE) {
				if (tflag) trnsname(fdir, MAXFILENAME);
				strcpy(ffile, fdir);
			}
			if (tofile == 0) {
				if (tflag) trnsname(ffile, MAXFILENAME);
				strcpy(tfile, ffile);
			}
			addb(tfile, MAXFILENAME);	/* blank pad tfile */
			rc = lifcpul(strip(agv[1]), 0);	/* HP-UX to lif */
		}
	}
	return(rc);
}



/*
 * Copy lif volume files to work station.
 */
copyto_ws(dest_type, argc, argv)
register int dest_type;
char *argv[];
{
	register int rc = TRUE;

	if (dest_type == 0) {			/* unix file */
		if (tofile == FALSE) {
			if (stat(tdir, &stbuf) < 0) {
				if (argc > 3) {
					fprintf(stderr, "lifcp: can not find %s\n", tdir);
					exit(-1);
				}
				tofile = 1;
				strcpy(tfile, tdir);
			}
			if (tofile == FALSE)
				if ((int)(stbuf.st_mode & S_IFMT) != (int)S_IFDIR) {
					if (!flag && argc > 3) {
						fprintf(stderr, "lifcp: %s is not a directory.\n", tdir);
						exit(-1);
					}
					fromfile = TRUE;
					strcpy (tfile, tdir);
				}
			strcat(tdir, "/");
		}
		strcpy(faked, tdir);
	}
	for(; agv < (argv + argc - 2); agv++, agc--) {
		if (whatis(agv[1], fdir, ffile, 0, &fromfile, argc) == 1) {
			if (dest_type == 0) {
				if (tofile == FALSE) { /* HP-UX directroy */
					strcpy(fakef, ffile);
					strcat(tdir, peel(fakef));
					rc = lifcplu(tdir, 0);
				}
				else rc = lifcplu(strip(argv[argc-1]), 0);
				strcpy(tdir, faked);
			}
			else rc = lifcplu(tdir, 1);
		}
		else {  /* whatis returned 0 or 2 */
			fprintf(stderr, "Use `cp' to copy HP-UX files.\n");
			exit(-1);
		}
	}
	return(rc);
}


lifcpll()
{
	char lastvolume;
	short volnumber;
	int implem;
	short done;
	int n;

	bugout("lifcpll: from %s:%s to %s:%s", fdir, ffile, tdir, tfile);
	if((n = (goodlifname(tfile))) != TRUE) {
		fprintf(stderr, "lifcp: can not create %s; %s \n",
		tfile, Fatal[n]);
		return(n);
	}
	if ((n = lifopen(&lfib1, fdir, LIFREAD)) != TRUE) {
		fprintf(stderr, "lifcp: can not open the LIF volume %s; %s\n",
			fdir, Fatal[n]);
		return(n);
	}
	strcpy(lfib1.lfile.fname, ffile);
	lfib1.lfile.ftype = EOD;
	if ((n = lfindfile(&lfib1)) != TRUE) {
		fprintf(stderr, "lifcp: %s does not exist.\n", ffile);
		return(n);
	}
	if ((n = lifopen(&lfib2, tdir, LIFUPDATE)) != TRUE) {
		fprintf(stderr, "lifcp: can not open the LIF volume %s; %s\n",
		fdir, Fatal[n]);
		return(n);
	}
	else {
		do {  /* copy file entry info */
			done = FALSE;
			lfib2.lfile = lfib1.lfile;
			strcpy(lfib2.lfile.fname, tfile);
			lfib2.lfile.ftype = EOD;
			lfib2.lfile.start = -1;
			lfib2.lfile.size = lfib1.lfile.size;
			if ((n = lfindfile(&lfib2)) == TRUE) {
				fprintf(stderr, "lifcp: file %s exists.\n", tfile);
				return(FALSE);
			}
			else {
				done = TRUE;
				if (Tflag)
					lfib2.lfile.ftype = filetype;
				else
					lfib2.lfile.ftype = lfib1.lfile.ftype;
				if (Implem != 0)
					implem = Implem;
				else implem = lfib1.lfile.extension;
				if (volnumberset==TRUE)
					volnumber = Volnumber;
				else volnumber = lfib1.lfile.lastvolnumber & 0077777;
				if (lastvolumeset==TRUE)
					lastvolume = Lastvolume;
				else lastvolume = (lfib1.lfile.lastvolnumber & 0100000) >> 15;
				if ((n = lifspace(&lfib2, implem, volnumber, lastvolume)) != TRUE) {
					fprintf(stderr, "lifcp: cannot create %s . %s\n", tfile, "NOSPCE");
					return(NOSPACE);
				}
				else
					n = movesects(lfib1.lfile.start,
					lfib1.dirpath, &lfib2.lfile.start,
					lfib2.dirpath, lfib1.lfile.size,
					lfib1.filedis, lfib2.filedis);
				if (n != TRUE) {
					fprintf(stderr, "lifcp: cannot move sectors; %s\n", Fatal[n]);
					return(FALSE);
				}
			}
		}
		while (done != TRUE);
	}
	return(TRUE);
}


lifcplu(ufile, std)
char *ufile;
int std;
{
	register FILE *outf;
	int j, n, m, maxnumread, numread, frsect, numsects;
	int accumulator, numwrite, bytesleft;
	int writelimit;
	unsigned short splength = 0, num();
	char eof = FALSE, spbyte = 0, issplit = FALSE, isalign = FALSE;

	bugout("lifcplu: from %s:%s to %s/%s unixfile %s",
			fdir, ffile, tdir, tfile, ufile);

	if ((n = lifopen(&lfib1, fdir, LIFREAD)) != TRUE) {
		fprintf(stderr, "lifcp: can not open the LIF volume %s; %s\n",
			fdir, Fatal[n]);
		return(n);
	}
	strcpy(lfib1.lfile.fname, ffile);
	lfib1.lfile.ftype = EOD;
	if ((n = lfindfile(&lfib1)) != TRUE) {
		fprintf(stderr, "lifcp: %s does not exist.\n", ffile);
		return(n);
	}
	if (std)
		outf = stdout;
	else
		outf = fopen(ufile, "w");
	if (outf == NULL) {
		fprintf(stderr, "lifcp: cannot create %s for writing\n", ufile);
		return(FALSE);
	}
	if (copytype == -1)	/* -a, -b or -r not specified */
		copytype = maptype(lfib1.lfile.ftype);
	frsect = lfib1.lfile.start;
	numsects = lfib1.lfile.size;
	while (numsects > 0) {	/* more sectors to read */
		maxnumread = min(numsects*256, K64);
		n = unitread(lfib1.filedis, bigbuf, maxnumread,
			frsect*256, "lifcp");
		if (n == -1)
			return(IOERROR);
		numread = min(maxnumread, numsects * QRT_K);
		bugout("frsect %d numsect %d copytype %d",
			frsect, numsects, copytype);
		bugout("maxnumread %d numread %d", maxnumread, numread);
		
		/*
		 * Series 500 has/(had?) a bug where you can't write
		 * more than 5120 bytes to a pipe.
		 * Hence break up the write if we're writing to stdout.
		 */
/*	 	if (outf == stdout)   take out and just use arbitrary 5K */
/*			this could be tuned for greater speed */
			writelimit=5120;

		accumulator = 0;
		numwrite = numread;
		bytesleft = numread;
		if (copytype == RAW_TYPE) {
#ifdef hp9000s500
		/* only the s500 needs this. Otherwise the following
		while loop will do infinite fwrites of zero bytes.
		*/
			if (numread > writelimit) numwrite=writelimit;
#endif hp9000s500
			while (accumulator != numread) {
				if (numwrite > bytesleft) numwrite=bytesleft;
				m=fwrite((char *) bigbuf+accumulator, 1, numwrite, outf);
				accumulator+=numwrite;
				bytesleft-=numwrite;
				if (m != numwrite)
					return(IOERROR);
			}
			if (accumulator!=numread)
					return (IOERROR);
			
		}
		else {	/* copytype is ASCII or BINARY */
			if(issplit == RECORDSP) {
				issplit = DATASP;
				splength = num(spbyte, bigbuf[0]);
				n = scan (outf, bigbuf + 1,
					numread-1, &eof, &issplit,
					&splength, &spbyte, &isalign);
			}
			else
				n = scan(outf, bigbuf,
					numread, &eof, &issplit,
					&splength, &spbyte, &isalign);
			if (n == ERROR)
				return(IOERROR);
			bugout("issplit %d splength %d spbyte %o",
					issplit, splength, spbyte);
			if (eof)
				break;
		}
		j = numread/QRT_K;
		frsect += j;
		numsects -= j;
	}
	n = lifclose (&lfib1, LIFKEEP);
	fclose(outf);
	return(TRUE);
}

lifcpul(ufile, std)
char *ufile;
int std;
{
	FILE *inf;
	int tobyteoff, maxnumread, numwrite;
	int tobyte, nbigbuf, totalbyte = 0;
	char eof = FALSE, eline = FALSE;
	register int n, i, nread;
	unsigned short rl, *shortp;
	char smallbuf[ 32 * ONE_K];

	bugout("lifcpul: from %s/%s to %s:%s unixfile %s",
			 fdir, ffile, tdir, tfile, ufile);
	if (std)
		inf = stdin;
	else
		inf = fopen(ufile, "r");
		
	if (inf == NULL) {
		perror(ufile);
		fprintf(stderr, "lifcp: Can't open %s for reading\n", ufile);
		return(FALSE);
	}
	if ((n = lifopen(&lfib1, tdir, LIFUPDATE)) != TRUE) {
		fprintf(stderr, "lifcp: can not open the LIF volume %s; %s\n",
			tdir, Fatal[n]);
		return(n);
	}
	if ((n = lifcreate(&lfib1, tfile, filetype, MAXFILESIZE, Implem,
		Volnumber, Lastvolume)) != TRUE) {
		fprintf(stderr, "lifcp: Cannot create %s; %s\n",
			tfile, Fatal[n]);
		return(FALSE);
	}
	if (copytype == -1)
		copytype = ASCII;
	tobyte = lfib1.lfile.start * QRT_K;
	while (!eof) {
		n = startup(bigbuf, &maxnumread, &tobyteoff, tobyte, &nbigbuf, lfib1.filedis);
		if (n == -1)
			return(IOERROR);
		if (copytype == RAW_TYPE) {
			n = fread(bigbuf + tobyteoff, 1, maxnumread, inf);
			if (n == -1)
				return(IOERROR);
			else if (n == 0)
				eof = TRUE;
			else if (n < maxnumread) {
				/* pad with nulls */
				numwrite = (((n - 1) / QRT_K) +1) * QRT_K;
				for (i = n; i < numwrite; i++)
					bigbuf[i+tobyteoff] = '\0';
			}
			else
				numwrite = maxnumread;
			if (!eof) {
				n = unitwrite(lfib1.filedis, bigbuf,
					numwrite + tobyteoff,
					tobyte - tobyteoff, "lifcp");
				if (n == -1)
					return(IOERROR);
				tobyte += numwrite;
				totalbyte += numwrite;
			}
		}
		else if (copytype == BINARY) {
			nread = fread(bigbuf+tobyteoff+2, 1, maxnumread-2, inf);
			if (nread == -1)
				return(IOERROR);
			else if (nread == 0) {
				numwrite = 2;
				rl = (unsigned short)-1;
				eof = TRUE;
			}
			else {
				numwrite = nread + 2;
				rl = nread;
			}
			shortp = (unsigned short *)(bigbuf + tobyteoff);
			*shortp = rl;
			n = unitwrite(lfib1.filedis, bigbuf,
				allign(numwrite + tobyteoff),
				tobyte-tobyteoff, "lifcp");
			if (n == -1)
				return(IOERROR);
			incr(&tobyte, numwrite, tobyteoff);
			incr(&totalbyte, numwrite, tobyteoff);
		}
		else if (copytype == ASCII) {
			n = readline(inf, smallbuf, &rl, &eof, &eline);
			if (n == -1)
				return(IOERROR);
			while (!eof || rl != (unsigned short)EOD) {
				if (nbigbuf+ rl + 4 > maxnumread) {
					n = unitwrite(lfib1.filedis, bigbuf,
						nbigbuf,
						tobyte-tobyteoff, "lifcp");
					if (n == -1)
						return(IOERROR);
					tobyte += nbigbuf - tobyteoff;
					n = startup(bigbuf, &maxnumread,
					&tobyteoff, tobyte, &nbigbuf,
					lfib1.filedis);
					if ( n == -1)
						return(IOERROR);
				}
				if (rl != 0) {
					mystrcpy(bigbuf + nbigbuf, &rl, 2);
					nbigbuf += 2;
					mystrcpy(bigbuf + nbigbuf, smallbuf,
						allign(rl));
					incr(&nbigbuf, rl, 0);
					incr(&totalbyte, (rl + 2), 0);
				}
				if (eline) {
					mystrcpy(bigbuf + nbigbuf , &rl, 2);
					nbigbuf += 2;
					totalbyte += 2;
					eline = FALSE;
				}
				if (eof) {
					if (totalbyte % QRT_K) {
						rl = (unsigned short)-1;
						mystrcpy(bigbuf+nbigbuf, &rl, 2);
						nbigbuf += 2;
						totalbyte += 2;
					}
					break;
				}
				n = readline(inf, smallbuf, &rl, &eof, &eline);
				if (n == -1)
					return(IOERROR);
			}
			n = unitwrite(lfib1.filedis, bigbuf,
				nbigbuf, tobyte-tobyteoff, "lifcp");
			if (n == -1)
				return(IOERROR);
			tobyte += nbigbuf - tobyteoff;
		}
	}
	n = lifclose(&lfib1, LIFMINSIZE, totalbyte);
	if (n != TRUE) {
		fprintf(stderr, "lifcp: cannot close %s; %s\n", tfile, Fatal[n]);
		return(FALSE);
	}
	fclose(inf);
	return(TRUE);
}

readline(inf, smallbuf, accum, eof, empty_line)
register FILE *inf;
register char *smallbuf;
register unsigned short *accum;
register char *eof, *empty_line;
{
	register int c;

	*accum = 0;
	while (*accum < (32 * ONE_K)) {
		c = getc(inf);
		if (c==EOF) {
			if (ferror(inf))
				return -1;
			*eof=TRUE;
			return TRUE;
		}
		if (c == '\n') {
			if (*accum == 0)
				*empty_line = TRUE;
			return TRUE;
		}
		smallbuf[(*accum)++] = c;
	}
	fprintf(stderr, "lifcp: input line too long.\n");
	return -1;
}



/*
 * If str is lif (contains :) then return 1,
 * else if standard input or output return 2,
 * otherwise return 0,
 * also set dir protion of str to head and file portion to tail
 * and set isfile to TRUE if there is a file portion.
 */
whatis(str, head, tail, todir, isfile, argc)
register char *str, *head, *tail;
register int *isfile, todir, argc;
{

	register int i,j;
	register int colon_seen = FALSE;
	register int bs = FALSE;		/* back slash */

	if (strcmp(str,"-")==0)			/* just a dash? */
		return(2);			/* standard input or output */

	for (i=j=0; *str!='\0'; str++) {
		if (colon_seen)
			tail[j++] = *str;
		if (bs)
			head[i++] = *str;

		if (*str == '\\') bs = TRUE;

		if (*str!=':' && !colon_seen) {
			if (!bs)
				head[i++] = *str;
		} else {
			if (!bs)
				colon_seen = TRUE;
			bs = FALSE;
		}
	}
	head[i] = '\0';
	if (j == 0) *isfile = 0;  /* not lif */
	while (j < MAXFILENAME)
		tail[j++] = ' ';
	tail[j] = '\0';

	if (colon_seen)
		return(1); /* lif */

	/* It's a unix file */
	*isfile = 1;
	j = 0;
	i = strlen(head);
	while ((i != 0) && (head[--i] != '/'));
	if ((i == 0) && todir) {
		if (stat(head, &stbuf) <0)
			/* not a directory */
			goto notd;
		if((int)(stbuf.st_mode & S_IFMT) == (int)S_IFDIR) {
			*isfile = 0;
			return(0);
		}
		else {
notd:		/* not a directory */
			if (!flag && argc > 3) {
				fprintf(stderr, "lifcp: %s is not a directory.\n", head);
				exit(-1);
			}
			else {  /*argc == 3 */
				strcpy(tail, head);
				strcpy(head, "");
				return(0);
			}
		}
	}
	else if((i == 0) && !todir) {
		strcpy(tail, head);
		strcpy(head, "");
	}
	else {
		int k;
		k = i;
		while( i < (strlen(head)-1))
			tail[j++] = head[++i];
		head[k] = '\0';
	}
	return(0);
}


/*
 * Take out the trailing blanks from str.
 */
char *peel(str)
register char *str;
{
	register char *p;

	p = str;
	while (*p!=' ' && *p!='\0')
		p++;
	*p = '\0';
	return(str);
}


/*
 *  Strip the "\\" in case of file names such as ABC\\:DE\\:M.
 *  The file name is actually ABC:DE:M.
 */
char *strip(p)
register char *p;
{

	register char *tmp, *str;
	register movetmp;

	str = p;
	movetmp = TRUE;
	while (TRUE) {
		while (*str!='\\' && *str!='\0')
			str++;
		if (*str == '\0')
			return(p);
		if (movetmp)
			tmp = str;
		else
			movetmp = TRUE;
		str++;
		while (*str!='\\') {
			*tmp++ = *str++;
			if (*tmp=='\0')
				return(p);
		}
		movetmp = FALSE;
	}
}



maptype(type)
register int type;
{
	if (type == ASCII || type == BINARY)
		return(type);
	else
		return(RAW_TYPE);
}



/*
 * A record length of -1 denotes logical end of file.
 * A file will have end of file marker unless its length is at the
 * maximum length defind in the directory. Also the length field of
 * the last record may exeed the physical space remaining; in this
 * case the file is teminated by physical length of the file.
 */
scan(fp, bufp, nbytes, eof, issplit, splength, spbyte, isalign)
FILE *fp;	/* file discriptor */
char *bufp;	/* data buffer pointer */
int nbytes;	/* length of the data buffer */
char *eof;
char *issplit;	/* issplit is set if data or record header is split */
unsigned short *splength;	/* if issplit is set to DATASP, splength is
				   set to the number of remaining bytes*/
char *spbyte;	/* if issplit is set to RECORDSP, spbyte contains the first
		   byte of record header */
char *isalign;	/* set if when DATASP occures and record length is alligned */
{

	char *p;
	unsigned short *rlength;
	unsigned short allignedrl;
	int n;

	p = bufp;
	bugout("scan: nbyte %d issplit %d splength %d spbyte %d",
			nbytes, *issplit, *splength, *spbyte);

	if(*issplit == DATASP) {
		if (*isalign) {
			*isalign = FALSE;
			n = fwrite(p, 1, (int) (*splength - 1), fp);
		}
		else
			n = fwrite(p, 1, *splength, fp);
		if (n == -1)
			return(ERROR);
		if (copytype == ASCII)
			n = putnewline(fp);
		if (n == ERROR)
			return(ERROR);
		p += *splength;
		nbytes -= *splength;
		*splength = 0;
		*issplit = FALSE;
	}
	while (!*issplit && !*eof) {
		if (nbytes <= 0)
			break;
	
		if (nbytes == 1) {
			bugout("RECORDSP");
			*issplit = RECORDSP;
			*spbyte = *p;
		}
		else {
			rlength = (unsigned short *)p;
			p += 2;
			nbytes -= 2;
			if (*rlength == (unsigned short)-1) {
				*eof = TRUE;
				break;
			}
			allignedrl = allign(*rlength);
			if (nbytes >= allignedrl) {
				n = fwrite(p, 1, *rlength, fp);
				if (n != *rlength)
					return(ERROR);
				if (copytype == ASCII)
					n = putnewline(fp);
				if (n == ERROR)
					return(ERROR);
				nbytes -= allignedrl;
				p += allignedrl;
			}
			else {
				n = fwrite(p, 1, nbytes, fp);
				if (n != nbytes)
					return(ERROR);
				bugout("DATASP");
				*issplit = DATASP;
				*splength = allignedrl - nbytes;
				if (*rlength != allignedrl) *isalign = TRUE;
			}
		}
	}
	return(TRUE);
}

unsigned short num(c1, c2)
char c1, c2;
{
	return(c1 << 8 || c2);
}

putnewline(fp)
FILE *fp;
{
	putc('\n', fp);
	return 0;
}

startup(buffer, max, byteoff, tobyte, nbigbuf, fp)
register char *buffer;
register int *max, *byteoff;
register int tobyte, *nbigbuf, fp;
{
	register int n;

	*byteoff = tobyte - (tobyte / ONE_K * ONE_K);  /* what is this doing? */
	*nbigbuf = *byteoff;
	if (*byteoff) {
		*max = K63;
		n = unitread(fp, buffer, *byteoff,
			tobyte - *byteoff, "lifcp");
		if (n == -1)
			return(-1);
	}
	else
		*max = K64;
	bugout("startup max %d tobyte %d byteoff %d", *max, tobyte, *byteoff);
	return(TRUE);
}

/*
 * Bump up to the next even boundary
 */
allign(x)
register int x;
{
	if (x & 1)			/* if x is odd */
		x++;			/* make it even */
	return x;
}



incr(count, n, add)
register int *count;
register int n, add;
{
	*count += n;
	if ((n+add) & 1)		/* if sum is odd */
		++*count;		/* make it even */
}
