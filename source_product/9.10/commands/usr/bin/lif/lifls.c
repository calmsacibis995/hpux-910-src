/* @(#) $Revision: 49.1 $ */   

#include "lifdef.h"
#include "global.h"
#include <stdio.h>
#include <sgtty.h>

extern int DEBUG;
extern char bigbuf[];
extern char *Fatal[];

char *strcpy(), *strchr(), *strncpy();
char *peel();

int tflg, Lflg, iflg, vflg, nflg, Cflg, lflg = FALSE;

main(argc, argv)
register int argc;
register char *argv[];
{

	register short n;
	register int c;
	struct sgttyb sgbuf;
	extern int optind;

	while ((c=getopt(argc, argv, "lDCLvi"))!=EOF) {
		switch (c) {
		case 'l': lflg = TRUE;	break;
		case 'D': DEBUG = TRUE;	break;
		case 'C': Cflg = TRUE;	break;
		case 'L': Lflg = TRUE;	break;
		case 'v': vflg = TRUE;	break;
		case 'i': iflg = TRUE;	break;
		default: goto usage;
		}
	}
	argv = &argv[optind];		/* next unprocessed argument */
	argc = argc-optind;		/* arguments left */

	tflg = (gtty(1, &sgbuf) == 0);

	if (!vflg && !iflg && !Lflg && !Cflg && !lflg) nflg = TRUE;

	if (argc<1)
		goto usage;

	for (; argc>=1; argv++, argc--) {
		n = lifls(*argv);

		if (n != TRUE) {
			fprintf(stderr, "lifls: Can't list %s; %s\n",
				*argv, Fatal[n]);
			exit(-1);
		}

		if (argc>1)			/* if any more left */
			puts("");		/* give us a blank line */
	}
	exit(0);

usage :
	fprintf(stderr, "USAGE: lifls [-C -l -i -v -L] volname[:filename] \n");
	exit(-1);
}

int column_index=0;				/* what column we're in */

lifls(dirpath)
register char *dirpath;
{
	register int this_entry, readsize, eod, left_to_read;
	register int n, readat;
	register struct dentry *dp;
	struct lfib frec;
	char file[MAXFILENAME + 1];
	int single;			/* a single file listing is requested */
	int found = FALSE;		/* a single file exists*/
	register int nread;		/* number of bytes read */
	struct lvol vol_label;		/* label for this volume */

	column_index=0;
	strcpy(file, "");
	singlefile(dirpath, file, &single);
	if ((frec.filedis = open(dirpath, 0)) < 0) {
		perror("lifls(open)");
		return(NOTOPEN);
	}
	strcpy(frec.dirpath, dirpath);
	if ((n=lifvol(&frec)) != TRUE)		/* is it a lif volume? */
		return(n);			/* abort if not so */
	
	/* volume label header */
	nread = unitread(frec.filedis, (char *) &vol_label, sizeof(vol_label),
			0, "lifls");
	if (nread == -1)
		return(IOERROR);
	eod = FALSE;

	if (lflg)
		printheader(&frec, &vol_label);

	left_to_read = vol_label.dsize * 256;
	for (this_entry=0; !eod && left_to_read>0; ) {
		readat = vol_label.dstart*256
			+ this_entry*sizeof(struct dentry);
		readsize = min(left_to_read, K64);
		nread = unitread(frec.filedis, bigbuf,
			readsize, readat, "lifls");
		if (nread == -1)
			return(IOERROR);
		left_to_read -= nread;

		dp = (struct dentry *) bigbuf;

		for (; dp < (struct dentry *) (bigbuf + nread); dp++) {

			if (dp->ftype == EOD) {
				eod = TRUE;
				break;
			}
			list_a_file(dp, single, file, &found);
			this_entry++;
			if (single && found) {
				eod=TRUE;
				break;			/* save some time */
			}
		}
	} 

	if (single && (!found))			/* did we find our file? */
		return(FILENOTFOUND);		/* error if we didn't */

	if (nflg || Cflg)
		printf("\n");

	n=closeit(frec.filedis);		/* close our file */
	if (n<0)				/* did it fail? */
		return(IOERROR);		/* and tell dad */

	return(TRUE);				/* a success */
}

list_a_file(dp, single, file, found)
register struct dentry *dp;
register int single;
register char *file;
register int *found;
{
	register int match;

	match = fname_equal(file, dp->fname);
	if (dp->ftype!=PURGED || lflg) {
		if (!single || match) {
			*found = TRUE;
			printf("%-.10s", dp->fname);
		}
		if (!lflg) {
			if (vflg)
				printf("  %u", dp->lastvolnumber&VOLNUMBER);
			else if (iflg)
				printf("0x%x", dp->extension);
			else if (Lflg)
				printf("  %u", (dp->lastvolnumber&LASTVOL)>>15); 
			if ((!single && !tflg && !Cflg) || Lflg || iflg || vflg)
				printf("\n");
			else if (!single) {
				printf("   ");
				if (++column_index == 5) {
					column_index = 0;
					printf("\n");
				}
			}
		}
	}
/*
filename   type   start size     implement    created
1234567890 BINARY 12345 12345678 1234567890   57/11/02 10:00:00 volume 1 (last)
*/
	if (lflg && (!single || match)) {

		switch (dp->ftype) {
		case PURGED:	printf(" PURGED"); break;
		case ASCII:	printf(" ASCII "); break;
		case BIN:	printf(" BIN   "); break;
		case BINARY:	printf(" BINARY"); break;
		default:	printf(" %-6d", dp->ftype);
		}
		printf(" %-7d %-8d", dp->start, dp->size);
		printf(" %-8x   ", dp->extension);
		print_date(dp->date);
		if ((dp->lastvolnumber & VOLNUMBER)!=1
		|| (dp->lastvolnumber & LASTVOL)==0) {
			printf(" vol %u", dp->lastvolnumber & VOLNUMBER);
			if (dp->lastvolnumber & LASTVOL)
				printf(" (last)");
		}
		printf("\n");
	}
}


print_date(param)
char *param;
{
	register struct date_fmt *when;

	when = (struct date_fmt *) param;

	printf("%d%d/%d%d/%d%d ",
		when->year1, when->year2,
		when->mon1, when->mon2,
		when->day1, when->day2);

	printf("%d%d:%d%d:%d%d",
		when->hour1, when->hour2,
		when->min1, when->min2,
		when->sec1, when->sec2);
}


singlefile(dirpath, file, single)
register char *dirpath, *file;
register int *single;
{
	register char *p;

	*single = FALSE;

	/* Find the colon */
	p = strchr(dirpath, ':');
	if (p==NULL) {			/* no colon found */
		*single = FALSE;
		return;
	}

	if (p[1]=='\0') {		/* Nothing after the colon? */
		*p = '\0';		/* remove the colon */
		*single = FALSE;	/* no file name given */
		return;
	}

	*single = TRUE;			/* we do have a file */
	*p++ = '\0';			/* cork off the device name */

	strncpy(file, p, MAXFILENAME);	/* copy that sucker over */
	addb(file, MAXFILENAME);	/* pad with blanks */
	return;
}

printheader(frecp, lvolp)
register struct lfib *frecp;
register struct lvol *lvolp;
{
	register int len;

	/* Find length of volume name */
	for (len=MAXVOLNAME; len>0 && lvolp->volname[len-1]==' '; len--)
		;

	printf("volume %-.*s data size %d ",
		len, lvolp->volname,
		frecp->lastsector - frecp->dstart - frecp->dsize + 1);
	printf("directory size %d", lvolp->dsize);

	/* Print creation date for level 1 and above, and if it's there */
	if (lvolp->version>=1 && lvolp->date[0]!=0) {
		printf(" ");
		print_date(lvolp->date);
	}
	printf("\n");
	puts("filename   type   start   size     implement  created");
	puts("===============================================================");
}

fname_equal(s1, s2)
register char *s1, *s2;
{
	register int i;

	for (i=0; i<MAXFILENAME; i++)
		if (*s1++ != *s2++)	/* a mismatch? */
			return FALSE;	/* failure */

	if (*s1=='\0')			/* if s1 was just the right length */
		return TRUE;		/* success */
	else
		return FALSE;
}
