/* HPUX_ID: @(#)mkboot.c	27.2     85/06/28  */
#include <stdio.h>
#include <a.out.h>
#include <volhdr.h>
#include <time.h>
#include <sys/stat.h>

#define	L_SECTORSIZE 256
#define	VOLNAME	       6
#define	FILENAME      10
#define	DEPB	       8

#define LIF_ASCII	1
#define HPUX_BOOT	-5822
#define LIF_EMPTY	0
#define LIF_END_DIR	-1

struct lvol {
	unsigned short discid;  /* 0..65535 */
	char volname[VOLNAME];
	int dstart;
	short dummy1;
	short dummy2;
	int dsize;
	short version;
	short dummy3;
	int tps;   /* tracks / surface */
	int spm;   /* surface / medium */
	int spt;   /* sector / track */
	unsigned short reserved[110];   /* 18..127 */
};

struct dentry {
	char fname[FILENAME];
	short ftype;
	int start;
	int size;
	char date[6];
	short lastvolnumber;
	int extension;
};



FILE *infile;
FILE *outfile;

int iflg, oflg, nflg;

char *names[8];
char *ifile, *ofile;
char *usage = "Usage: mkboot -i in -o out -n name1 ... name8";

struct exec aout;

char buff[L_SECTORSIZE];

struct lvol *l_vol;
struct dentry *l_dir;

struct load lhdr;			/* loader header */
struct HPUX_vol_type sys_record;

/*
 * timestamp() -- Fill in a BCD lif format date buffer with the
 *                local version of a given date and time.
 */
void
timestamp(date_buf, now)
char *date_buf;
time_t now;
{
    extern struct tm *localtime();
    register struct tm *date;
    struct bcd_date {
	unsigned year1  : 4;
	unsigned year2  : 4;
	unsigned mon1   : 4;
	unsigned mon2   : 4;
	unsigned day1   : 4;
	unsigned day2   : 4;
	unsigned hour1  : 4;
	unsigned hour2  : 4;
	unsigned min1   : 4;
	unsigned min2   : 4;
	unsigned sec1   : 4;
	unsigned sec2   : 4;
    } *bcd_date = (struct bcd_date *)date_buf;

    date = localtime(&now);	/* convert to local time */
    date->tm_mon++;		/* unix gives 0-11, we want 1-12 */

    bcd_date->year1 = date->tm_year / 10;
    bcd_date->year2 = date->tm_year % 10;
    bcd_date->mon1  = date->tm_mon  / 10;
    bcd_date->mon2  = date->tm_mon  % 10;
    bcd_date->day1  = date->tm_mday / 10;
    bcd_date->day2  = date->tm_mday % 10;
    bcd_date->hour1 = date->tm_hour / 10;
    bcd_date->hour2 = date->tm_hour % 10;
    bcd_date->min1  = date->tm_min  / 10;
    bcd_date->min2  = date->tm_min  % 10;
    bcd_date->sec1  = date->tm_sec  / 10;
    bcd_date->sec2  = date->tm_sec  % 10;
}

main(argc,argv)
int argc;
char **argv;
{
	extern int optind;
	extern char *optarg;

	register int c, start, size, i, filesize;
	register int nbytes;
	time_t file_time;
	struct stat st;

	while ((c = getopt(argc, argv, "i:o:s:n:")) != EOF)
		switch(c) {
		case 'i':
			iflg++;
			ifile = optarg;
			break;
		case 'o':
			oflg++;
			ofile = optarg;
			break;
		case 'n':
			nflg++;
			names[0] = optarg;
			for (i=1; optind < argc && i < 8; i++, optind++) {
				if (*argv[optind] == '-')
					break;
				names[i] = argv[optind];
			}
			break;
		case '?':
		default:
			fprintf(stderr, "%s\n", usage);
			exit(2);
			break;
		}

	if (!iflg || !oflg || !nflg) {
		fprintf(stderr, "%s\n", usage);
		exit(2);
	}

	if ((infile = fopen(ifile,"r")) == NULL) {
		fprintf(stderr,"can't open %s for input\n",ifile);
		exit(1);
	}

	if (fstat(fileno(infile), &st) == -1) {
		fprintf(stderr,"can't stat %s\n",ifile);
		exit(1);
	}
	file_time = st.st_mtime;

	if ((outfile = fopen(ofile,"w+")) == NULL) {
		fprintf(stderr,"can't open %s for output\n",ofile);
		exit(1);
	}

	if (fread(&aout, sizeof(aout), 1, infile) == 0) {
		fprintf(stderr,"can't read a.out header for %s\n",ifile);
		exit(1);
	}

	if (aout.a_magic.file_type == SHARE_MAGIC) {
		fprintf(stderr, "%s cannot be shared text\n", ifile);
		exit(2);
	}

	size = aout.a_text + aout.a_data;
	filesize = (size + sizeof(struct load) + L_SECTORSIZE-1)/L_SECTORSIZE;

	start = aout.a_entry;

	memset(buff, '\0', L_SECTORSIZE);

	l_vol = (struct lvol *) buff;

	l_vol->discid = 0x8000;

	strncpy(l_vol->volname,"BOOT  ", VOLNAME);
	l_vol->dstart = 2;
	l_vol->dummy1 = 0x1000;
	l_vol->dummy2 = 0;
	l_vol->dsize = 1;
	l_vol->version = 1;
	l_vol->tps   = 1;
	l_vol->spm   = 1;
	l_vol->spt   = filesize + 3;

	/* write out lif volume header to sector 0 */
	fseek(outfile, 0, SEEK_SET);
	if (fwrite(buff,L_SECTORSIZE, 1, outfile) == 0) {
		fprintf(stderr,"Write error on lif volume header\n");
		exit(1);
	}

	memset(buff, '\0', L_SECTORSIZE);

	lhdr.address = start;
	lhdr.count = size;

	fseek(outfile, L_SECTORSIZE * 3, SEEK_SET);
	if (fwrite(&lhdr, sizeof(struct load), 1, outfile) == 0) {
		fprintf(stderr,"Write error on load block\n");
		exit(1);
	}

	fseek(infile, sizeof(struct exec), SEEK_SET);

	for (i = 0 ; i < size; i += nbytes) {
		nbytes = fread(buff, 1, L_SECTORSIZE, infile);
		if (nbytes <= 0) {
			fprintf(stderr,"Unexpected EOF on input\n");
			exit(1);
		}

		/*
		 * If this is the last block and it is a partial block,
		 * zero pad it and write out an entire block.
		 */
		if (i + nbytes >= size && nbytes < L_SECTORSIZE)
		{
			memset(buff+nbytes, '\0', L_SECTORSIZE - nbytes);
			nbytes = L_SECTORSIZE;
		}

		if (fwrite(buff, 1, nbytes, outfile) != nbytes) {
			fprintf(stderr,"Write error on output\n");
			exit(1);
		}
	}

	fseek(outfile, L_SECTORSIZE*2, SEEK_SET);

	if (fread(buff, L_SECTORSIZE, 1, outfile) == 0) {
		fprintf(stderr,"Read error on directory block\n");
		exit(1);
	}

	l_dir = (struct dentry *)(&buff[0]);

	for (i=0; i < DEPB; i++) {
		strncpy(l_dir[i].fname,"          ", FILENAME);
		l_dir[i].ftype = LIF_END_DIR;
		l_dir[i].start = 3;
		l_dir[i].lastvolnumber = 0x8001;
		l_dir[i].size = filesize;
		l_dir[i].extension = start;
	}

	for (i=0; i < 8; i++) 
		if (names[i] == (char *)0)
			break;
		else {
			timestamp(l_dir[i].date, file_time);
			strncpy(l_dir[i].fname,"          ", FILENAME);
			strncpy(l_dir[i].fname, names[i], strlen(names[i]));
			l_dir[i].ftype = HPUX_BOOT;
		}

	fseek(outfile, L_SECTORSIZE*2, SEEK_SET);
	if (fwrite(buff, L_SECTORSIZE, 1, outfile) == 0) {
		fprintf(stderr,"Write error on directory block\n");
		exit(1);
	}

	fclose(outfile);
}
