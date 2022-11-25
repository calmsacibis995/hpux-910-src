static char *HPUX_ID = "@(#) $Revision: 70.3 $";
#include <stdio.h>
#include <a.out.h>
#include <nl_types.h>
#define NL_SETN 1

#define OCTAL 1		/* these are used to control offset printing */
#define HEX   2
#define DECIMAL 3

#ifdef hp9000s800
#  define N_BADTYPE(x) ((x).a_magic != RELOC_MAGIC && (x).a_magic != SHARE_MAGIC\
		     && (x).a_magic != DEMAND_MAGIC)
#  define N_BADMACH(x) ((_PA_RISC_ID((x).system_id)) == 0)
#  define N_BADMAG(x) (N_BADTYPE(x) || N_BADMACH(x))
#endif hp9000s800

#ifdef hp9000s500
#  define N_BADTYPE(x) (((x).a_magic.file_type)!=EXEC_MAGIC&&\
     ((x).a_magic.file_type)!=SHARE_MAGIC&&\
     ((x).a_magic.file_type)!=RELOC_MAGIC)
#  define N_BADMACH(x) ((x).a_magic.system_id!=HP9000_ID)
#  define N_BADMAG(x)  (N_BADTYPE(x) || N_BADMACH(x))
#endif

long	ftell();

/*
 * strings
 */

#ifdef hp9000s800
   FILHDR filhdr;
   AOUTHDR aouthdr;
#else not hp9000s800
   struct exec filhdr;
#endif not hp9000s800

int	oflg = 0;        /* >0 = print offset, 1=octal, 2=hex, 3=decimal */
int	asdata = 0;
int	minlength = 4;   /* minimum length to consider a string */
nl_catd catd;

extern char *optarg;
extern int optind, opterr;

/***************************************************************/
void usage()
{
/* This usage message decribes the syntax for the POSIX version of the
   command.  Obsolete options are retained in the code for backward
   compatibility.  They may be removed at a future time. 
   Obsolete syntax:
      -o         same as -t o
      -number    same as -n number
      -          same as -a
*/
fputs((catgets(catd,NL_SETN,1, "Usage: strings [ -a ] [ -t format ] [ -n number ] [ file ... ]\n")), stderr);
exit(1);
}

/**************************************************************/

main(argc, argv)
	int argc;
	char *argv[];
{
	int argcnt = 0;
	int x;
	char optch;

	catd = catopen ( "strings", 0 );
	opterr = 0;

	/* replace any "-" parameters with -a since they are equivalent */
	for ( x = 0; x < argc; x++ )
	   if ( strcmp ( argv [ x ], "-" ) == 0 )
	      argv [ x ] = "-a";

	while ( ( optch = getopt ( argc, argv, "t:n:ao0123456789" ) ) != EOF ) 
	   switch ( optch ) {
		case 'o':
			argcnt++;
			oflg = OCTAL;
			break;

		case 't':
			if ( strcmp ( optarg, "o" ) == 0 )
			   oflg = OCTAL;            /* octal */
			else if ( strcmp ( optarg, "x" ) == 0 )
			   oflg = HEX;              /* hexidecimal */	
			else if ( strcmp ( optarg, "d" ) == 0 )
			   oflg = DECIMAL;          /* decimal */
			else
			   usage();

			if ( strncmp( ( optarg - 2 ), "-t", 2 )  == 0 )  
			  argcnt++;     /* only one parameter string */
			else
			  argcnt += 2;  /* 2 parmeter strings */
			break;
				

		case 'a':
			argcnt++;
			asdata++;
			break;

		case 'n':
			/* check for no space after -n, ie: one argv string */
			if ( strncmp( ( optarg - 2 ), "-n", 2 )  == 0 )  
			  argcnt++;     /* only one parameter string */
			else
			  argcnt += 2;  /* 2 parmeter strings */
			minlength = atoi ( optarg );
			if ( minlength < 1 ) {
			  usage();
			  }
			break;	

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			x = abs ( atoi ( argv [ optind - 1 ] ) );
			if ( x != 0 ) {
				minlength = x;
				argcnt++;
				}
			break;

		case '?':
				usage();
		}

	argc = argc - argcnt;
	argc--;
	argv = &argv [ optind ];


	do {
		if (argc > 0) {
			if (freopen(argv[0], "r", stdin) == NULL) {
				perror(argv[0]);
				exit(1);
			}
			argc--, argv++;
		}
		else
			asdata++;

		if (asdata) {
			find((long)0, (long) 100000000L);
			continue;
		}

		fseek(stdin, (long) 0, 0);
		if (fread((char *)&filhdr, sizeof filhdr, 1, stdin) != 1 || 
		    N_BADMAG(filhdr)) {
			fseek(stdin, (long) 0, 0);
			find((long)0, (long) 100000000L);
			continue;
		}
#       ifdef hp9000s800
	  	   fseek(stdin, (long) filhdr.aux_header_location, 0);
		   if (fread((char *)&aouthdr, sizeof aouthdr, 1, stdin) != 1) {
		   	   fseek(stdin, (long) 0, 0);
			   find((long)0, (long) 100000000L);
			   continue;
		   }
		   fseek(stdin, (long) aouthdr.exec_dfile, 0);
		   find(ftell(stdin), (long) aouthdr.exec_dsize);
#       endif

#       ifdef hp9000s200
		  fseek(stdin, (long) DATAPOS, 0);
		  find(ftell(stdin), (long) filhdr.a_data);
#       endif

#       ifdef hp9000s500
	  	  fseek(stdin, filhdr.a_datasegs.fs_offset, 0);
		  find(ftell(stdin), (long) filhdr.a_datasegs.fs_size);
#       endif
	} while (argc > 0);
}

find(start, cnt)
	long start;
	long cnt;
{
	char buf[BUFSIZ];
	register char *cp;
	register int c, cc;

	cp = buf, cc = 0;
	for (; cnt != 0; cnt--) {
		c = getc(stdin);
		start++;
		if (c == '\n' || dirt(c) || cnt == 0) {
			if (cp > buf && cp[-1] == '\n')
				--cp;
			*cp++ = 0;
			if (cp > &buf[minlength]) {
				if (oflg)
					pr_offset(start - cc - 1, oflg);
				fputs(buf, stdout);
				fputc('\n', stdout);
			}
			cp = buf, cc = 0;
		} else {
			if (cp < &buf[sizeof buf - 2])
				*cp++ = c;
			cc++;
		}
		if (ferror(stdin) || feof(stdin))
			break;
	}
}

dirt(c)
	int c;
{

	switch (c) {

	/* other whitespace could go here, but it increases the odds of
	   finding a bogus string, and formfeed and vertical tab are
	   rare in strings */
	case '\n':
	case '\t':
		return (0);

	default:
		return (c >= 0177 || c < ' ');
	}
}

/*
 * pr_offset(n,b) -- print a positive b-base number in a field of 7 (if
 *                 possible), followed by a space.
 */
pr_offset(n, b)
unsigned long n;
int b;
{
if ( b == OCTAL )
   printf ( "%7lo ", n );
else if ( b == HEX )
   printf ( "%7lx ", n );
else if ( b == DECIMAL )
   printf ( "%7lu ", n );
}
