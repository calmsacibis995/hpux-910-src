/* @(#) $Revision: 27.1 $ */   

#include "lifdef.h"
#include "global.h"
#include <stdio.h>

char *strcpy();

extern char *Fatal[];
extern int DEBUG;

main(argc, argv)
register int argc;
register char *argv[];
{
	register short n;	
	register int i;
	char d[MAXDIRPATH], f1[MAXFILENAME + 1], f2[MAXFILENAME + 1];
	register char *ptr;

	if (argc>1 && strcmp(argv[1], "-D")==0) {
		DEBUG = TRUE;
		argc--;
		argv++;
	}

	if (argc!=3)
		goto usage;
	ptr = argv[1];
	for (i = 0; *ptr != ':'; i++, ptr++){
		if (*ptr == '\0'){
			fprintf(stderr, "Use `mv' to rename HP-UX file %s.\n", argv[1]);
			exit(-1);
		}
		d[i] = *ptr;
	}
	d[i] = '\0';
	ptr++;
	for (i = 0; *ptr != '\0' && i < MAXFILENAME; i++, ptr++)
		f1[i] = *ptr;
	while (i < MAXFILENAME)
		f1[i++] = ' ';
	f1[i] = '\0';
	ptr = argv[2];
	for (i = 0; *ptr != '\0' && i < MAXFILENAME; i++, ptr++)
		f2[i] = *ptr;
	while (i < MAXFILENAME)
		f2[i++] = ' ';
	f2[i] = '\0';
	n = lifrn(d, f1, f2);
	if (n != TRUE) {
		fprintf(stderr, "lifrename : Can not change %s to %s; %s\n", f1, f2, Fatal[n] );
		exit(-1);
	}
	exit(0);

usage :
	fprintf(stderr, "USAGE: lifrename oldfile newfile \n");
	exit(-1);
}

lifrn(d, f1, f2)
register char *d, *f1, *f2;
{
	struct lfib frec;
	register int n, tempindex, frbyte;
	struct dentry dir;

	if(goodlifname(f2) != TRUE)
		return(BADLIFNAME);
	if ((frec.filedis = open(d, 2)) < 0) {
		perror("lifrename(open)");
		return(IOERROR);
	}
	strcpy(frec.dirpath, d);
	n = lifvol(&frec);
	if (n!=TRUE)
		return n;

	strcpy(frec.lfile.fname, f1); 
	frec.lfile.ftype = EOD;

	n = lfindfile(&frec);

	if (n!=TRUE)
		return(FILENOTFOUND);

	tempindex = frec.dindex;
	strcpy(frec.lfile.fname, f2); 
	if ((n = lfindfile(&frec)) == TRUE) {
		if (strcmp(f1, f2) == 0)
			return(TRUE);
		return(LIFEXISTS);
	}


	frbyte = (frec.dstart * QRT_K) + (tempindex * DESIZE);
	n = unitread(frec.filedis, &dir, sizeof(dir), frbyte, "lifclose");
	if (n == -1)
		return(IOERROR);
	mystrcpy(dir.fname, frec.lfile.fname, MAXFILENAME);
	n = unitwrite(frec.filedis, &dir, sizeof(dir), frbyte, "lifclose");
	if (n == -1)
		return(IOERROR);

	if (closeit(frec.filedis) < 0)
		return(IOERROR);
	
	return(TRUE);
}
