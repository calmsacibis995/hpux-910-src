static char *HPUX_ID = "@(#) $Revision: 70.4 $";
#
/* cut : cut and paste columns of a table (projection of a relation) */
/* make: cc -DNLS -DNLS16 cut.c */
# include <stdio.h>	
# include <limits.h>
# define NFIELDS LINE_MAX /* max no of fields or resulting line length */
# define USAGE "Usage: cut [-s] [-d<string>] [-c<list> | -f<list>] file ..."
# define CFLIST "bad list for c/f/b option"

#ifdef NLS16
#	include <nl_ctype.h>
#	define		BSTATUS b_status = BYTE_STATUS((unsigned char)*rbuf, b_status)
	int		b_status = ONEBYTE;
#endif

#ifdef NLS
#include <locale.h>
#endif NLS

int	sel[NFIELDS];	/* globals are initialized to zero */

main(argc, argv)
int	argc;
char	**argv;
{
	unsigned char	del = '\t';
	int num, j, count, poscnt, r, s, t;
	int	endflag, supflag, cflag, fflag, bflag, nflag, backflag, filenr;
	unsigned char 	buf[LINE_MAX];
	register int	c;
	register unsigned char	*p1, *rbuf;
	unsigned char	outbuf[NFIELDS];
	extern char *optarg;
	extern int optind;
	register unsigned char *p, *list;
	FILE *inptr;
	int infile=0;

#ifdef NLS16
	unsigned char *char_addr; /*holds address of first byte of 2-byte char*/
	int char_cnt;	 /*  holds "sel[]" count for the first byte above */
	if (!setlocale(LC_ALL,"")) 
		fputs(_errlocale(),stderr);
#endif

	r = num = endflag = supflag = cflag = fflag = bflag = nflag = 0;

	while((c = getopt(argc, argv, "nb:c:d:f:s")) != EOF)
		switch(c) {
			case 'b':
				if (fflag || cflag)
					diag(CFLIST);
				bflag++;
				list = (unsigned char *) optarg;
				break;
			case 'c':
				if (fflag || bflag)
					diag(CFLIST);
				cflag++;
				list = (unsigned char *) optarg;
				break;
			case 'd':
				/** changed the check from !=1 to <1
				*** for POSIX 1003.2 conformance
				*** 6/27/89 ec */
				if (cflag || bflag) 
				   diag ("d option used only with f option");
				if (strlen(optarg) < 1) 
					diag("no delimiter");
				else
					del = (int)*optarg;
				break;
			case 'f':
				if (cflag || bflag)
					diag(CFLIST);
				fflag++;
				list = (unsigned char *) optarg;
				break;
			case 'n':
				if (cflag || fflag)
				  diag ("n option used only with b option");
				nflag++;
				break;
			case 's':
				if (bflag || cflag)
				  diag ("s option used only with f option");
				supflag++;
				break;
			case '?':
				diag(USAGE);
		}

	argv = &argv[optind];
	argc -= optind;

	if (!(cflag || fflag || bflag))
		diag(CFLIST);

	do {
		p = list;
		switch(*p) {
			case '-':
				if (r)
					diag(CFLIST);
				r = 1;
				if (num == 0)
					s = 1;
				else {
					s = num;
					num = 0;
				}
				break;
			case '\0' :
			case ','  :
				if (num >= NFIELDS)
					diag(CFLIST);
				if (r) {
					if (num == 0)
						num = NFIELDS - 1;
					if (num < s)
						diag(CFLIST);
					for (j = s; j <= num; j++)
						sel[j] = 1;
				} else
					sel[num] = (num > 0 ? 1 : 0);
				s = num = r = 0;
				if (*p == '\0')
					continue;
				else
					break;
			default:
				if (*p < '0' || *p > '9')
					diag(CFLIST);
				num = 10 * num + *p - '0';
				break;
		}
		list++;
	}while (*p != '\0');
	for (j=t=0; j < NFIELDS; j++)
		t += sel[j];
	if (t == 0)
		diag("no fields");

	filenr = 0;
	do {	/* for all input files */
		if (argc == 0)
			inptr = stdin;
		else if(!infile && strcmp(argv[filenr],"-")== 0){
			inptr = stdin;
			infile++;
		}
		else
			if ((inptr = fopen(argv[filenr], "r")) == NULL) {
				fputs("cut: cannot open ", stderr);
				fputs(argv[filenr], stderr);
				fputc('\n', stderr);
				exit(2);
			}
		endflag = 0;
		do {	/* for all lines of a file */
		   count = 0;
		   poscnt = 1;
		   p1 = &outbuf[0] - 1 ;
		   rbuf = buf;
		   if ((fgets(buf, LINE_MAX, inptr)) == NULL) {
		 	endflag = 1;
		 	continue;
		   }
		   do { 	/* for all char of the line */
		      count++;
		      if (count == NFIELDS - 1)
				diag("line too long");

		      BSTATUS;			/* update for each char */
		      if (sel[poscnt]) {

/* 
 * The following conditions check for special handling of two-byte characters
 * when the specified list falls within a character. Special handling is
 * according to POSIX.2/D11 definition.
 */

			  if (cflag || nflag) {
			      if (b_status==SECOF2 && poscnt>1 && !sel[poscnt-1])
				  *++p1 = *(rbuf-1);
			      if (b_status==FIRSTOF2 && nflag && poscnt<NFIELDS-1 && !sel[poscnt+1]) {
				  rbuf++;
				  BSTATUS;
				  count++;
				  poscnt+=2;
				  continue;
			      }
			 }
			 if (*rbuf != '\n')
			     *++p1 = *rbuf;
/*
 *  When dealing with -c or -f option, if the first byt of a two-byte char
 *  is selected, automatically grap the second byte (we don't want to split
 *  two-byte chars in such cases).
 */
			 if ((cflag || fflag) && b_status==FIRSTOF2 ) {
			     *++p1 = *++rbuf;
			     BSTATUS;
			     count++;
			 }
			 if (bflag || cflag) {
			     poscnt++;
			     continue;
			 }
			 else {  /* f option only */
			     if (*rbuf==del && b_status==ONEBYTE)
				poscnt++;
			     continue;
			 }
		      }
		      else {
			 if ((cflag || fflag) && b_status==FIRSTOF2 ) {
			     rbuf++;
			     BSTATUS;
			     count++;
			 }
			 if (fflag && *rbuf!=del)
			     continue;
			 poscnt++;
		      }
		   } while (*rbuf++ != '\n');

		   if ( !endflag && (poscnt > 1 || !supflag)) {
			if ( poscnt == 1 ) {
				*(rbuf-1) = '\0';
				puts(buf);
			} else {
				if (*p1 == del && !sel[poscnt])
					*p1 = '\0';
						/*suppress trailing delimiter*/
				else
					*++p1 = '\0';
				puts(outbuf);
			}
		   }
		} while (!endflag);
		fclose(inptr);
	} while (++filenr < argc);
	exit(0);
}


diag(s)
char	*s;
{
	fputs("cut: ", stderr);
	fputs(s, stderr);
	fputc('\n', stderr);
	exit(2);
}
