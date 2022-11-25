static char *HPUX_ID = "@(#) $Revision: 64.2 $";
/* @(#) $Revision: 64.2 $ */     
/*
** SYNOPSIS
**	insertmsg strings
**
**	assumes files are accessible as named in "strings"
**
**	procedure is
**		read a line from strings file
**		if file name at beginning of line is different
**		from name of file currently being worked on,
**		write remainder of current file out to modified file
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <msgbuf.h>
#include <locale.h>
#endif NLS

#ifdef NLS16
#include <nl_ctype.h>
#endif NLS16

char	*strcat();
char	*strcpy();

#define SAME 0
#define TRUE 1
#define FALSE 0
#define FNBFSIZ 1000
#define STRPREF "(catgets(catd,NL_SETN,%d, "
#define STRPOST "))"
char * hdrline[] = {
         "#ifndef NLS\n",
         "#define catgets(i, sn,mn,s) (s)\n",
         "#else NLS\n",
	 "#define NL_SETN 1	/* set number */\n",
         "#include <nl_types.h>\n",
         "#endif NLS\n"
};

char	this_fname[FNBFSIZ];		/* file named by current line of "strings" */
char	last_fname[FNBFSIZ] = "";	/* C source file currently being looked at */
char	nl_fname[FNBFSIZ];		/* put changed C source on this file */
char	nl_prefix[] = "nl_";
int	NHDLINES = 0;
int	set = 1;

main(argc,argv)
char ** argv;
{

	int increment = 1;	/* message number increment 	*/
	int msgno = 1;		/* messages numbered up from 1 */
	int first = TRUE;	/* no previous file */
	char *charnum;
	int i, c,
	    start,	/* byte offset of '"' at start of string */ 
	    length,	/* number of bytes from '"' thru '"' */
	    curpos,
	    n_good;

	struct stat statb;
	int ino,dev = -1;

	FILE *strings;	/* strings to be replaced */
	FILE *nl_Csource = 	/* changed C program */
		(FILE *)NULL;	/* initialize to shut lint up */
	/* old C program is read from redirected stdin */
	/* messages for inclusion in catalog are written to stdout */

#ifdef NLS || NLS16			/* initialize to the correct locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("insertmsg"), stderr);
		putenv("LANG=");
	}
	nl_catopen("insertmsg");
#endif NLS || NLS16

	if (argc < 2) {
		fprintf(stderr, (nl_msg(1, "usage: insertmsg [-h] [-n number] [-iamount [-snumber] stringlist\n")));
		exit(1);
	}
	argc--; argv++;
	while (argc > 0 && argv[0][0] == '-') {
	   switch (argv[0][1]) {
	   case 'n' :
		charnum = argv[0]+2;
		for (msgno=0; *charnum >= '0' && *charnum <= '9'; charnum++) {
		   msgno *= 10;
		   msgno += *charnum - '0';
		}
		msgno = (msgno == 0) ? 1 : msgno;
		if ( msgno > NL_MSGMAX ) msgnoerr();
		break;
	   case 'h' :
		NHDLINES = 6;
		break;
	   case 's' :
		charnum = argv[0]+2;
		for (set=0; *charnum >= '0' && *charnum <= '9'; charnum++) {
		   set *= 10;
		   set += *charnum - '0';
		}
		set = (set == 0) ? 1 : set;
		break;
	   case 'i' :
		charnum = argv[0]+2;
		for (increment=0; *charnum >= '0' && *charnum <= '9'; charnum++) {
		   increment *= 10;
		   increment += *charnum - '0';
		}
		increment = (increment == 0) ? 1 : increment;
		break;
	   default :
		fprintf(stderr, (nl_msg(1, "usage: insertmsg [-h] [-n number] [-iamount [-snumber] stringlist\n")));
		exit(1);
	  }
	  argc--; argv++;
	}
	if ((strings = fopen(argv[0], "r")) == NULL){
		fprintf(stderr, (nl_msg(2, "insertmsg: unable to open %s\n")),
			argv[0]);
		exit(1);
	}

	/* test below stolen directly from 4.2 cat.c
	** keep user from clobbering strings file */
	if (fstat(fileno(stdout), &statb) == 0) {
		statb.st_mode &= S_IFMT;
		if (statb.st_mode!=S_IFCHR && statb.st_mode!=S_IFBLK) {
			dev = statb.st_dev;
			ino = statb.st_ino;
		}
	}
	if (fstat(fileno(strings), &statb) == 0) {
		if ((statb.st_mode & S_IFMT) == S_IFREG 
		&& statb.st_dev==dev && statb.st_ino==ino) {
			fprintf(stderr, (nl_msg(3, "insertmsg: input %s is output\n")), argv[0]);
			exit(1);
		}
	}

	printf ("$set %d\n", set);	/* header for input to gencat(1) */
	while (!feof(strings)){
		/*	loop once per entry in strings file */
		if (fscanf (strings, "%s %d %d", this_fname, &start, &length) == EOF)
			break;
		if ( msgno > NL_MSGMAX ) msgnoerr();
		if (strcmp(this_fname, last_fname) != SAME){
			/*	starting a new file 
			**	copy out rest of previous file, 
			**	if there was one 
			**	open new C input & output files */
			if (!first){
				while ((c=getchar()) != EOF) putc(c, nl_Csource);
				(void) fclose (nl_Csource);
			} else 
				first = FALSE;

			if (freopen(this_fname, "r", stdin) == NULL) {
		        	fprintf(stderr, 
					(nl_msg(2,"insertmsg: unable to open %s\n")),
					this_fname);
		        	exit(1);
	                }
			make_nl_fname(nl_fname, this_fname);
			if ((nl_Csource=fopen(nl_fname,"w")) == NULL) {
		                fprintf(stderr, 
				       (nl_msg(2,"insertmsg: unable to open %s\n")),
			                nl_fname);
		                exit(1);
	                }

			strcpy(last_fname, this_fname);
			curpos = 0;
			/* check if a set number was given 	*/
			for (i=0; i<NHDLINES; i++) 
				if ((set != 1) && (i == 3))
	 				fprintf (nl_Csource, "#define NL_SETN %d	/* set number */\n", set);
				else
                                	fprintf(nl_Csource, hdrline[i]);
		}

		/*	put message number and string on standard
		**	output after removing surrounding quotes.
		**	look at where quotes should be to make sure
		**	we're not lost */
		printf("%d ", msgno);
		getc(strings);	/* ' ' between length and string */
		if (getc(strings) != '"')
			syncerr();
		for (i=0; i<length-2; i++) /* shorter by 2 "'s */
			putchar(getc(strings));
		putchar('\n');
		if (getc(strings) != '"')
			syncerr();

		/*	figure how much source file there is between
		**	tail end of last string and where current
		**	string begins - put this much out unchanged */
		n_good = start-curpos;
		for (i=0; i<n_good; i++){
			putc(getchar(), nl_Csource); /* good part of old file */
		}

		/*	advance current offset pointer to start of
		**	old string in unmodified source.
		**	dump old string as comment in call to
		**	message catalog in localized source
		**	advance curpos past string in orig source */
		curpos += n_good;
		fprintf(nl_Csource, STRPREF, msgno);
		for (i=0; i<length; i++)
			putc(getchar(),nl_Csource);
		fprintf(nl_Csource, STRPOST);
		curpos += length;
		msgno += increment;
	}
	if (nl_Csource) {
		/*	finish last file */
		while ((c=getchar()) != EOF) 
			putc(c, nl_Csource);
	}
	exit(0);
}

make_nl_fname(nl_fname, this_fname) 
char *nl_fname, *this_fname;
{
	char	*t, *bt, *n, *bn;
	int	c;

#ifndef NLS16
	for (t=bt=this_fname, n=bn=nl_fname; *t != '\0'; *n++ = *t++)
		if ( *t == '/') {
#else NLS16
	for (t=bt=this_fname, n=bn=nl_fname; *t != '\0'; c=CHARADV(t),WCHARADV(c,n))
		if ( CHARAT(t) == '/') {
#endif NLS16
			bt = t + 1;
			bn = n + 1;
		}

	strcpy(bn, nl_prefix);
	strcat(nl_fname, bt);
}

syncerr()
{
	fprintf(stderr, (nl_msg(4, "insertmsg: lost in strings file\n")));
	exit(1);
}

msgnoerr()
{
	fprintf(stderr, (nl_msg(5, "insertmsg: message number exceeds NL_MSGMAX\n")));
	exit(1);
}
