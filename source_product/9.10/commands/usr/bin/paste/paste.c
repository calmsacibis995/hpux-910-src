static char *HPUX_ID = "@(#) $Revision: 70.3 $";
#
/* paste: concatenate corresponding lines of each file in parallel. Release 1.4 */
/*	(-s option: serial concatenation like old (127's) paste command */
/*                                                                            */
/*     xb - Feb 9, 1983  -- fixed bug 1684: paste -s produced extra copies of */
/*                                                                            */
# include <stdio.h>	/* make :  cc paste.c  */
# include <limits.h>

#ifdef NLS
#include <nl_types.h>
#define NL_SETN 1
#endif

# define MAXOPNF (_NFILE-3)
# define MAXLINE LINE_MAX  	/* maximal line length (consistent with cut) */
#define RUB  '\177'
char del[MAXLINE];
  
main(argc, argv)
int argc;
char ** argv;
{
	int i, j, k, eofcount, nfiles, maxline, glue;
	int unopened_files;		/* The test for termination is to
					 * be at eof on all files.  Exclude
					 * the ones we couldn't open.
					 * DSDe414543.
					 */
	int delcount = { 1 } ;
	int onefile  = { 0 } ;
	register int c ;
	char outbuf[MAXLINE], l, t;
	register char *p;
	FILE *inptr[MAXOPNF];
	extern char *optarg;
	extern int optind;
	nl_catd catd;

	maxline = MAXLINE -2;
	unopened_files = 0;
	strcpy(del,"\t");
 
 catd = catopen("paste", 0);

	while ((c=getopt(argc,argv,"sd:"))!=EOF) {
		switch(c) {
			case 's':
				onefile++;
				break ;
			case 'd':
				if ((delcount = move(optarg, &del[0])) == 0)
					printf(catgets(catd,NL_SETN,1,"no delimiters\n"));
				break;
			case '?':
				 printf(catgets(catd,NL_SETN,2,"Usage: paste [-s] [-d<delimiterstring>] file1 file2 ...\n"));
				break;
		}
	}
 
	if ( ! onefile) {	/* not -s option: parallel line merging */
		for (i = 0; i+optind<argc && i < MAXOPNF; i++) {
			if (argv[i + optind][0] == '-') {
				inptr[i] = stdin;
			} else inptr[i] = fopen(argv[i + optind], "r");
			if (inptr[i] == NULL) {
				 printf(catgets(catd,NL_SETN,3,argv[i + optind]));
				 printf(catgets(catd,NL_SETN,4,": cannot open\n"));
				 unopened_files++;
			}
		}
		if (i+optind<argc) 
			 printf(catgets(catd,NL_SETN,5,"too many files\n"));
		nfiles = i;
  
		do {
			p = &outbuf[0];
			eofcount = 0;
			j = k = 0;
			for (i = 0; i < nfiles; i++) {
			    /* Don't attempt to read from files we were
			     * unable to open. DSDe414543
			     */
			    if (inptr[i] != NULL) {
				int gave_warning = 0;
				while((c = getc(inptr[i])) != '\n' && c != EOF)   {
					if (++j <= maxline) *p++ = c ;
					else {
					   if (gave_warning == 0) {
					      printf(catgets(catd,NL_SETN,6,
					         "line too long\n"));
					      gave_warning = 1;
					   }
					}
				}
				if ( (l = del[k]) != RUB) *p++ = l;
				k = (k + 1) % delcount;
				if( c == EOF) eofcount++;
			    }
			}
			if (l != RUB) *--p = '\n'; else  *p = '\n';
			*++p = 0;
			if (eofcount < (nfiles-unopened_files)) 
			   fputs(outbuf, stdout);
		}while (eofcount < (nfiles-unopened_files));
  
	} else {	/* -s option: serial file pasting (old 127 paste command) */
		for (i = optind; i < argc; i++) {
			/* the following 5 lines are added to fix bug 1684 */
			p = &outbuf[0];
			glue = 0;
			j = 0;
			k = 0;
			t = 0;
			/**************************************************/

			if (argv[i][0] == '-') {
				inptr[0] = stdin;
			} else inptr[0] = fopen(argv[i], "r");
			if (inptr[0] == NULL) {
				 printf(catgets(catd,NL_SETN,7,argv[i]));
				  printf(catgets(catd,NL_SETN,8,": cannot open\n"));
			}
	  
			/* Don't attempt to read from files we were
			 * unable to open. DSDe414543
			 */
			if (inptr[0] != NULL) {
			   while((c = getc(inptr[0])) != EOF)   {
				   if (j >= maxline) {
					   t = *--p;
					   *++p = 0;
					   fputs(outbuf, stdout);
					   p = &outbuf[0];
					   j = 0;
				   }
				   if (glue) {
					   glue = 0;
					   l = del[k];
					   if (l != RUB) {
						   *p++ = l ;
						   t = l ;
						   j++;
					   }
					   k = (k + 1) % delcount;
				   }
				   if(c != '\n') {
					   *p++ = c;
					   t = c;
					   j++;
				   } else glue++;
			   }
			}
			if (t != '\n') {
				*p++ = '\n';
				j++;
			}
			if (j > 0) {
				*p = 0;
				fputs(outbuf, stdout);
			}
		}
	}
	exit(0);
}

diag(s,r)
char *s;
int r;
{
	write(2, "paste: ", 7);
	while(*s)write(2,s++,1);
	if(r != 0) exit(r);
}

diag2(s,r)
char *s;
int r;
{
	while(*s)write(2,s++,1);
	if(r != 0) exit(r);
}
  
move(from, to)
char *from, *to;
{
int c, i;
	i = 0;
	do {
		c = *from++;
		i++;
		if (c != '\\') *to++ = c;
		else { c = *from++;
			switch (c) {
				case '0' : *to++ = RUB;
						break;
				case 't' : *to++ = '\t';
						break;
				case 'n' : *to++ = '\n';
						break;
				default  : *to++ = c;
						break;
			}
		}
	} while (c) ;
return(--i);
}
