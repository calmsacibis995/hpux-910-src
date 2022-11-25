static char *HPUX_ID = "@(#) $Revision: 72.3 $";
/*
**	convert and copy
*/

#include	<stdio.h>
#include        <unistd.h>
#include	<sys/param.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/sysmacros.h>
#include	<sys/stat.h>


#include	<sys/diskio.h>
#include 	<string.h>
#include 	<locale.h>
#include 	<fcntl.h>
#include	<nl_types.h>
#include	<msgbuf.h>

#define	LCASE	0x01
#define	UCASE	0x02
#define	SWAB	0x04
#define NERR	0x08
#define SYNC	0x10
#define BLOCK   0x20
#define UNBLOCK 0x40

#define MAX_FLEX_SIZE  4194303   /* Max No of DEV_BSIZE blocks -
				    = 4GB */
capacity_type cap;

nl_catd catd;		/* For POSIX */
unsigned ibs = 512;	/*  changed to 512 to match  */
unsigned obs = 512;	/*  HPUX specs 		     */
unsigned bs;
unsigned cbs;
unsigned ibc;
unsigned obc;
unsigned cbc;

int	fflag;
int	cflag;
int	nifr;
int	nipr;
int	nofr;
int	nopr;
int	ntrunc;
int	ibf;
int	obf;
int	skip;
int	seekn;
int	count;
int	nspace;
int 	notrunc_flag;
int     spec_char_dev = 0; /* To indicate we've such device as input */
off_t   prev_offset   = 0; /* To keep offset for error handling      */

char	*string;
char	*ifile;
char	*ofile;
char	*ibuf;
char	*obuf;
/*  char    *sbrk();  in unistd.h    */
char	*op;

/* This is an EBCDIC to ASCII conversion table
*/
char etoa [] =
{
	0000,0001,0002,0003,0234,0011,0206,0177,
	0227,0215,0216,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0235,0205,0010,0207,
	0030,0031,0222,0217,0034,0035,0036,0037,
	0200,0201,0202,0203,0204,0012,0027,0033,
	0210,0211,0212,0213,0214,0005,0006,0007,
	0220,0221,0026,0223,0224,0225,0226,0004,
	0230,0231,0232,0233,0024,0025,0236,0032,
	0040,0240,0241,0242,0243,0244,0245,0246,
	0247,0250,0325,0056,0074,0050,0053,0174,
	0046,0251,0252,0253,0254,0255,0256,0257,
	0260,0261,0041,0044,0052,0051,0073,0176,
	0055,0057,0262,0263,0264,0265,0266,0267,
	0270,0271,0313,0054,0045,0137,0076,0077,
	0272,0273,0274,0275,0276,0277,0300,0301,
	0302,0140,0072,0043,0100,0047,0075,0042,
	0303,0141,0142,0143,0144,0145,0146,0147,
	0150,0151,0304,0305,0306,0307,0310,0311,
	0312,0152,0153,0154,0155,0156,0157,0160,
	0161,0162,0136,0314,0315,0316,0317,0320,
	0321,0345,0163,0164,0165,0166,0167,0170,
	0171,0172,0322,0323,0324,0133,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,
	0340,0341,0342,0343,0344,0135,0346,0347,
	0173,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0350,0351,0352,0353,0354,0355,
	0175,0112,0113,0114,0115,0116,0117,0120,
	0121,0122,0356,0357,0360,0361,0362,0363,
	0134,0237,0123,0124,0125,0126,0127,0130,
	0131,0132,0364,0365,0366,0367,0370,0371,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0372,0373,0374,0375,0376,0377,
};
/* This is an ASCII to EBCDIC conversion table
*/
char atoe [] =
{
0000,0001,0002,0003,0067,0055,0056,0057,
0026,0005,0045,0013,0014,0015,0016,0017,
0020,0021,0022,0023,0074,0075,0062,0046,
0030,0031,0077,0047,0034,0035,0036,0037,
0100,0132,0177,0173,0133,0154,0120,0175,
0115,0135,0134,0116,0153,0140,0113,0141,
0360,0361,0362,0363,0364,0365,0366,0367,
0370,0371,0172,0136,0114,0176,0156,0157,
0174,0301,0302,0303,0304,0305,0306,0307,
0310,0311,0321,0322,0323,0324,0325,0326,
0327,0330,0331,0342,0343,0344,0345,0346,
0347,0350,0351,0255,0340,0275,0232,0155,
0171,0201,0202,0203,0204,0205,0206,0207,
0210,0211,0221,0222,0223,0224,0225,0226,
0227,0230,0231,0242,0243,0244,0245,0246,
0247,0250,0251,0300,0117,0320,0137,0007,
0040,0041,0042,0043,0044,0025,0006,0027,
0050,0051,0052,0053,0054,0011,0012,0033,
0060,0061,0032,0063,0064,0065,0066,0010,
0070,0071,0072,0073,0004,0024,0076,0341,
0101,0102,0103,0104,0105,0106,0107,0110,
0111,0121,0122,0123,0124,0125,0126,0127,
0130,0131,0142,0143,0144,0145,0146,0147,
0150,0151,0160,0161,0162,0163,0164,0165,
0166,0167,0170,0200,0212,0213,0214,0215,
0216,0217,0220,0152,0233,0234,0235,0236,
0237,0240,0252,0253,0254,0112,0256,0257,
0260,0261,0262,0263,0264,0265,0266,0267,
0270,0271,0272,0273,0274,0241,0276,0277,
0312,0313,0314,0315,0316,0317,0332,0333,
0334,0335,0336,0337,0352,0353,0354,0355,
0356,0357,0372,0373,0374,0375,0376,0377,
};
char	atoibm[] =
{
	0000,0001,0002,0003,0067,0055,0056,0057,
	0026,0005,0045,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0074,0075,0062,0046,
	0030,0031,0077,0047,0034,0035,0036,0037,
	0100,0132,0177,0173,0133,0154,0120,0175,
	0115,0135,0134,0116,0153,0140,0113,0141,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0172,0136,0114,0176,0156,0157,
	0174,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0321,0322,0323,0324,0325,0326,
	0327,0330,0331,0342,0343,0344,0345,0346,
	0347,0350,0351,0255,0340,0275,0137,0155,
	0171,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0221,0222,0223,0224,0225,0226,
	0227,0230,0231,0242,0243,0244,0245,0246,
	0247,0250,0251,0300,0117,0320,0241,0007,
	0040,0041,0042,0043,0044,0025,0006,0027,
	0050,0051,0052,0053,0054,0011,0012,0033,
	0060,0061,0032,0063,0064,0065,0066,0010,
	0070,0071,0072,0073,0004,0024,0076,0341,
	0101,0102,0103,0104,0105,0106,0107,0110,
	0111,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0142,0143,0144,0145,0146,0147,
	0150,0151,0160,0161,0162,0163,0164,0165,
	0166,0167,0170,0200,0212,0213,0214,0215,
	0216,0217,0220,0232,0233,0234,0235,0236,
	0237,0240,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0312,0313,0314,0315,0316,0317,0332,0333,
	0334,0335,0336,0337,0352,0353,0354,0355,
	0356,0357,0372,0373,0374,0375,0376,0377,
};


main(argc, argv)
char **argv;
{
	register char *ip;
	register c;
	int	(*conv)();

	int ebcdic(), ibm(), ascii(), null(), cnull(), term(), block(), unblock();
	int a;

	int find_size();

	if(!setlocale(LC_ALL, "")){
		fputs("setlocale failed, continuing with POSIX locale.",stderr);		putenv("LANG");
		catd=(nl_catd) - 1;
	}
	else
		catd=catopen("dd",0);

	conv = null;
	for(c=1; c<argc; c++) {
		string = argv[c];
		if(match("ibs=")) {
			ibs = number();
			continue;
		}
		if(match("obs=")) {
			obs = number();
			continue;
		}
		if(match("cbs=")) {
			cbs = number();
			continue;
		}
		if (match("bs=")) {
			bs = number();
			continue;
		}
		if(match("if=")) {
			ifile = string;
			continue;
		}
		if(match("of=")) {
			ofile = string;
			continue;
		}
		if(match("skip=")) {
			skip = number();
			continue;
		}
		if(match("seek=")) {
			seekn = number();
			continue;
		}
		if(match("count=")) {
			count = number();
			continue;
		}
		if(match("conv=")) {
		cloop:
			if(match(","))
				goto cloop;
			if(*string == '\0')
				continue;
			if(match("ebcdic")) {
				conv = ebcdic;
				goto cloop;
			}
			if(match("ibm")) {
				conv = ibm;
				goto cloop;
			}
			if(match("ascii")) {
				conv = ascii;
				goto cloop;
			}

			if(match("block")) {
				cflag |= BLOCK;
				conv = block;
				goto cloop;
			}
			if(match("unblock")) {
				cflag |= UNBLOCK;
				conv = unblock;
				goto cloop;
			}

			if(match("lcase")) {
				cflag |= LCASE;
				goto cloop;
			}
			if(match("ucase")) {
				cflag |= UCASE;
				goto cloop;
			}
			if(match("swab")) {
				cflag |= SWAB;
				goto cloop;
			}
			if(match("noerror")) {
				cflag |= NERR;
				goto cloop;
			}
			
			if(match("notrunc")) {
				notrunc_flag=1;
				goto cloop;
			}
			
			if(match("sync")) {
				cflag |= SYNC;
				goto cloop;
			}
		}
		fprintf(stderr,(catgets(catd,NL_SETN,1, "dd: bad arg %s\n")), string);
		exit(2);
	}
	if(conv == null && cflag&(LCASE|UCASE))
		conv = cnull;
	if (ifile)
		ibf = open(ifile, 0);
#ifndef STANDALONE
	else
		ibf = dup(0);
#endif
	if(ibf < 0) {
		fprintf(stderr,(catgets(catd,NL_SETN,2, "dd: cannot open %s\n")), ifile);
		exit(2);
	}

	if (ifile && ofile) {
		struct stat sbuf_in, sbuf_out;

		if ( stat(ifile,&sbuf_in) == -1 ) {
			perror(ifile);
			exit(0);
		}
		/*
		   If output file already exists AND it's not a
		   special file AND it's inode/device numbers are the
		   same - don't let him/her stomp on it.
		*/
		if ( stat(ofile,&sbuf_out) == 0 &&
		     !(sbuf_in.st_mode & (S_IFCHR | S_IFBLK)) &&
		     sbuf_in.st_dev == sbuf_out.st_dev &&
		     sbuf_in.st_ino == sbuf_out.st_ino ) {
			fprintf(stderr,(catgets(catd,NL_SETN,3, "dd: can't copy file onto itself\n")));
			exit(0);
		}
	}

	if ((ofile)  &&  ( notrunc_flag ))
		obf = open(ofile,O_RDWR);
	else
	if (ofile)
		obf = creat(ofile, 0666);
#ifndef STANDALONE
	else
		obf = dup(1);
#endif
	if(obf < 0) {
		fprintf(stderr,(catgets(catd,NL_SETN,4, "dd: cannot create %s\n")), ofile);
		exit(2);
	}

/* Check if input or output file corresponds to a 
   disk partition of greater than 4 GB - dd does 
   not support this .. 
 */
        find_size(ibf,ifile);
	find_size(obf,ofile);


	if (bs) {
		ibs = obs = bs;
		if (conv == null)
			fflag++;
	}
	if(ibs == 0 || obs == 0) {
		fprintf(stderr,(catgets(catd,NL_SETN,5, "dd: counts cannot be zero\n")));
		exit(2);
	}

	/*
	 * Allocate i/o buffers.  For performance purposes, we want
	 * to make sure that our buffers are page aligned.
	 *
	 * First, get the current break line page aligned.
	 */
	{
		unsigned long p = (unsigned long)sbrk(0);

		p &= (NBPG - 1);	/* bytes into curr page */
		if(p != 0)
			sbrk(NBPG - p); /* round up to next page */
	}

	/*
	 * If obuf will be allocated too, we allocate ibuf in multiple
	 * of pages, even if ibs is not.  This is so that obuf is also
	 * page aligned.
	 *
	 * For historical purposes, we add a 64 pad to the last
	 * buffer (I don't know why, but it won't hurt anything to
	 * leave it in).
	 */

	if(!fflag) {
		ibuf = sbrk((ibs + (NBPG-1)) & ~(NBPG-1));
		obuf = sbrk(obs + 64);
	} else {
		ibuf = obuf = sbrk(ibs + 64);
	}

	if(ibuf == (char *)-1 || obuf == (char *)-1) {
		fprintf(stderr,(catgets(catd,NL_SETN,6, "dd: not enough memory\n")));
		exit(2);
	}
	ibc = 0;
	obc = 0;
	cbc = 0;
	op = obuf;

#ifndef STANDALONE
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, term);
#endif

	{
	 struct stat tempstat;

	 (void) fstat(ibf, &tempstat);
  /*
   * If ibf isn't a special character file we'll use
   * lseek instead read for skipping
   */
	 spec_char_dev = ( tempstat.st_mode & _S_IFMT ) && _S_IFCHR;

	 if( skip )
	 {
	  if( spec_char_dev  )
	  {
	   lseek(ibf, (off_t) (ibs * skip), SEEK_CUR);
	  }
	  else
	   while(skip)
	   {
	    read(ibf, ibuf, ibs);
	    skip--;
	   }
	 }
	}

	while( seekn )
	{
	 lseek(obf, (off_t) obs, SEEK_CUR);
	 seekn--;
	}
	while(1) {
		if(ibc-- == 0) {
			ibc = 0;
			if(count == 0 || nifr+nipr != count) {
				/*
				 * If NERR is set, we must clear
				 * the buffer before the read.
				 */
				if (cflag&NERR)
					memset(ibuf, '\0', ibs);
				ibc = read(ibf, ibuf, ibs);

				/*
				 * save the current offset to jump over
				 * error block if error will occured
				 */
				if( ibc == ibs ) prev_offset += ibs;

				/*
				 * If SYNC is set and we didn't
				 * clear the buffer before the read,
				 * and we didn't read an entire
				 * buffers worth, we zero out the
				 * end of the buffer.
				 */

				if((cflag&(NERR|SYNC)) == SYNC &&
				   ibc != ibs &&
				   ibc != -1)  /* POSIX - fill in spaces if 
						  block or unblock defined */
				{
					if((cflag&BLOCK) || (cflag&UNBLOCK))
					memset(ibuf+ibc, ' ', ibs-ibc);
					else
					memset(ibuf+ibc, '\0', ibs-ibc);
				}
			}
			if(ibc == -1) {
				perror((catgets(catd,NL_SETN,7, "dd read error")));
				if((cflag&NERR) == 0) {
					flsh();
					term(2);
				}

				ibc = 0;
				for(c=0; c<ibs; c++)
					if(ibuf[c] != 0)
						ibc = c;


				stats();
      /*
       * Indeed, we have no ways to determine end of true data
       * in a bad block according to POSIX ...
       */

				if( !ibc ) ibc = 1; /* What else could we do */
				prev_offset += ibs;
				if( spec_char_dev ) lseek(ibf, prev_offset, SEEK_SET);
			/* To avoid repeating of the same read error */

			}
			if(ibc == 0) {
				flsh();
				/*
				 * Check return value from close(2) in case
				 * we get a delayed NFS write error.  The
				 * error message matches exactly what the
				 * flsh() routine does when write(2) fails
				 * even though: 1) the message goes to
				 * stdout, not stderr; 2) the message
				 * does not include the errno value because
				 * perror is not called.
				 */
				if (close(obf) != 0) {
		                   fprintf(stderr, (catgets(catd,NL_SETN,8, "I/O error \n")));
				   term(2);
				} else term(0);
			}
			if(ibc != ibs) {
				nipr++;
				if(cflag&SYNC)
					ibc = ibs;
			} else
				nifr++;
			ip = ibuf;

			/* POSIX - if current record is an odd number of bytes 
			the last byte should be ignored - for SWAB 
			Changed (ibc +1) >>1 to ibc >>1 to not to account for
			the last odd byte  
			
			*/
			c = ibc >> 1;
			if(cflag&SWAB && c)
				do {
					a = *ip++;
					ip[-1] = *ip;
					*ip++ = a;
				} while(--c);
			ip = ibuf;
			if (fflag) {
				obc = ibc;
				flsh();
				ibc = 0;
			}
			continue;
		}
		(*conv)(*ip++);
	}
   /* I have no idea who had inserted catclose here, but it
    * never can be reached from previous loop. Better I comment it
	catclose(catd);
    */
}

flsh()
{
	register c;
                                            /*-- ITG - MKR 8510.29 - 27.2 --*/
    if(obc) {                               /*If there is output,           */
        c = write(obf, obuf, obc);          /*  Write the output.           */
        if ( (obc == obs) && (c == obc) )   /*If we wrote a full record,    */
            nofr++;                         /*  Increase full record count. */
        else                                /*Otherwise, increment number   */
            nopr++;                         /*  of partial records.         */
        if(c != obc) {                      /*If record written incomplete, */
	    printf((catgets(catd,NL_SETN,8, "I/O error \n")));
					    /*  Indicate an error,          */
            term(2);                        /*  And describe what we did.   */
        }                                   /*End of error check            */
        obc = 0;                            /*Show we finished output.      */
	}                                       /*------------------------------*/
}

match(s)
char *s;
{
	register char *cs;

	cs = string;
	while(*cs++ == *s)
		if(*s++ == '\0')
			goto true;
	if(*s != '\0')
		return(0);

true:
	cs--;
	string = cs;
	return(1);
}

number()
{
	register char *cs;
	register n;

	cs = string;
	n = 0;
	while(*cs >= '0' && *cs <= '9')
		n = n*10 + *cs++ - '0';
	for(;;)
	switch(*cs++) {

	case 'k':
		n *= 1024;
		continue;

	case 'w':
		n *= 2;
		continue;

	case 'b':
		n *= 512;	/* changed to 512 to match specs */
		continue;

	case '*':
	case 'x':
		string = cs;
		n *= number();

	case '\0':
		return(n);
	}
	/* never gets here */
}

cnull(cc)
{
	register c;

	c = cc;
	if(cflag&UCASE && c>='a' && c<='z')
		c += 'A'-'a';
	if(cflag&LCASE && c>='A' && c<='Z')
		c += 'a'-'A';
	null(c);
}

null(c)
{

	*op = c;
	op++;
	if(++obc >= obs) {
		flsh();
		op = obuf;
	}
}

ascii(cc)
{
	register c;

	c = 0;
	c |= etoa[0377 & cc];
	if(cbs == 0) {
		cnull(c);
		return;
	}
	if(c == ' ')
		nspace++;
	else {
		while(nspace > 0) {
			null(' ');
			nspace--;
		}
		cnull(c);
	}
	if(++cbc >= cbs) {
		null('\n');
		cbc = 0;
		nspace = 0;
	}
}

ebcdic(cc)
{
	register c;

	c = cc;
	if(cflag&UCASE && c>='a' && c<='z')
		c += 'A'-'a';
	if(cflag&LCASE && c>='A' && c<='Z')
		c += 'a'-'A';
	c = 0 | atoe[c & 0377];
	if(cbs == 0) {
		null(c);
		return;
	}
	if(cc == '\n') {
		while(cbc < cbs) {
			null(atoe[' ']);
			cbc++;
		}
		cbc = 0;
		return;
	}
	if(cbc == cbs)
		ntrunc++;
	cbc++;
	if(cbc <= cbs)
		null(c);
}

ibm(cc)
{
	register c;

	c = cc;
	if(cflag&UCASE && c>='a' && c<='z')
		c += 'A'-'a';
	if(cflag&LCASE && c>='A' && c<='Z')
		c += 'a'-'A';
	c = 0 | atoibm[c & 0377];
	if(cbs == 0) {
		null(c);
		return;
	}
	if(cc == '\n') {
		while(cbc < cbs) {
			null(atoibm[' ']);
			cbc++;
		}
		cbc = 0;
		return;
	}
	if(cbc == cbs)
		ntrunc++;
	cbc++;
	if(cbc <= cbs)
		null(c);
}

term(c)
{

	stats();
	exit(c);
}

stats()
{

	fprintf(stderr,(catgets(catd,NL_SETN,9, "%u+%u records in\n")), nifr, nipr);
	fprintf(stderr,(catgets(catd,NL_SETN,10, "%u+%u records out\n")), nofr, nopr);
	if(ntrunc)
		fprintf(stderr,(catgets(catd,NL_SETN,11, "%u truncated records\n")), ntrunc);
}


unblock(cc)
{
	register c;

	c = cc & 0377;
	if(cbs == 0) {
		cnull(c);
		return;
	}
	if(c == ' ') {
		nspace++;
		goto out;
	}
	while(nspace > 0) {
		null(' ');
		nspace--;
	}
	cnull(c);

out:
	if(++cbc >= cbs) {
		null('\n');
		cbc = 0;
		nspace = 0;
	}
}


block(cc)
{
	register c;

	c = cc;
	if(cflag&UCASE && c>='a' && c<='z')
		c += 'A'-'a';
	if(cflag&LCASE && c>='A' && c<='Z')
		c += 'a'-'A';
	c &= 0377;
	if(cbs == 0) {
		null(c);
		return;
	}
	if(cc == '\n') {
		while(cbc < cbs) {
			null(' ');
			cbc++;
		}
		cbc = 0;
		return;
	}
	if(cbc == cbs)
		ntrunc++;
	cbc++;
	if(cbc <= cbs)
		null(c);
}

/* This function is called in main to check if the input or
   the output file corresponds to a disk partition of greater
   than 4 GB. The ioctl call will fail for some devices like 
   tapes in which case it is neglected 

*/

	
int find_size(fid,filename)
int fid;
char *filename;
{
	
	if (ioctl(fid, DIOC_CAPACITY, &cap) != -1)
		if(((u_long) cap.lba) >= MAX_FLEX_SIZE ){ 
			fprintf(stderr,(catgets(catd,NL_SETN,12,"\nERROR !! %s Section Size Exceeds 4GB !! \n")), filename);
			exit(2);
		}
}
