static char *HPUX_ID = "@(#) $Revision: 56.1 $";
/*
**	Make all characters visible/invisible.
*/

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <msgbuf.h>
#endif NLS

char	*strrchr();

char	buffer[BUFSIZ];
char	*program_name;

int	iflag = 0;
int	nflag = 0;
int	silent = 0;
int	tflag = 0;
int	xflag = 0;

main(argc, argv)
int    argc;
char **argv;
{
	register FILE *fi;
	register int c;
	register char *p;
	extern	int optind;
	int	errflg = 0;
	int	stdinflg = 0;
	int	status = 0;
	int	dev, ino = -1;
	struct	stat	statb;

	program_name = argv[0];			/* name or program (vis/inv) */
	p = strrchr(program_name, '/');		/* find the last / */
	if (p!=NULL)				/* if there is indeed one: */
		program_name = p+1;		/* just use last component */

#ifdef STANDALONE
	if (argv[0][0] == '\0')
		argc = getargv ("vis", &argv, 0);
#endif STANDALONE
	if (strcmp(program_name, "inv") == 0) {
		iflag = 1;
	}
	while( (c=getopt(argc,argv,"nstux")) != EOF ) {
		switch(c) {
		case 'n':
			nflag++;
			tflag++;
			continue;
		case 's':
			silent++;
			continue;
		case 't':
			tflag++;
			continue;
		case 'u':
#ifndef	STANDALONE
			setbuf(stdout, (char *)NULL);
#endif STANDALONE
			continue;
		case 'x':
			xflag++;
			continue;
		case '?':
			errflg++;
			break;
		}
	}

#ifdef NLS
	nl_catopen("vis");
#endif NLS

	if (errflg) {
		if (!silent)
			fprintf(stderr,(nl_msg(1, "usage: %s -nstux [-|file] ...\n")), program_name);
		exit(2);
	}
	if(fstat(fileno(stdout), &statb) < 0) {
		if(!silent)
			fprintf(stderr, (nl_msg(2, "%s: cannot stat stdout\n")), program_name);
		exit(2);
	}
	statb.st_mode &= S_IFMT;
	if (statb.st_mode!=S_IFCHR && statb.st_mode!=S_IFBLK) {
		dev = statb.st_dev;
		ino = statb.st_ino;
	}
	if (optind == argc) {
		argc++;
		stdinflg++;
	}
	for (argv = &argv[optind];
	     optind < argc && !ferror(stdout); optind++, argv++) {

		if (stdinflg || (*argv)[0]=='-' && (*argv)[1]=='\0')
			fi = stdin;
		else {
			if ((fi = fopen(*argv, "r")) == NULL) {
				if (!silent)
				   fprintf(stderr, (nl_msg(3, "%s: cannot open %s\n")), program_name, *argv);
				status = 2;
				continue;
			}
		}
		if(fstat(fileno(fi), &statb) < 0) {
			if(!silent)
			   fprintf(stderr, (nl_msg(4, "%s: cannot stat %s\n")), program_name, *argv);
			status = 2;
			continue;
		}
		if (statb.st_dev==dev && statb.st_ino==ino) {
			if(!silent)
			   fprintf(stderr, (nl_msg(5, "%s: input %s is output\n")), program_name, stdinflg?"-": *argv);
			if (fclose(fi) != 0 ) 
				if(!silent)
					fprintf(stderr, (nl_msg(6, "%s: close error\n")), program_name);
			status = 2;
			continue;
		}

		if (!iflag)
			status = vis(fi);
		else
			status = inv(fi);

		if (fi!=stdin)
			fflush(stdout);
		if (fclose(fi) != 0) 
			if (!silent)
				fprintf(stderr, (nl_msg(7, "%s: close error\n")), program_name);
	}
	fflush(stdout);
	if (ferror(stdout)) {
		if (!silent)
			fprintf(stderr, (nl_msg(8, "%s: output error\n")), program_name);
		status = 2;
	}
	exit(status);
}

/* character type table */

#define PLAIN	0
#define EXTEND	1
#define BSPACE	2
#define HTAB	3
#define NLINE	4
#define VTAB	5
#define FFEED	6
#define RETURN	7
#define ESCAPE	8
#define SPACE	9
#define BSLASH	10
#define OTHER	11

char ctype[256] = {
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,
	BSPACE,	HTAB,	NLINE,	VTAB,	FFEED,	RETURN, OTHER,  OTHER,
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,
	OTHER,	OTHER,	OTHER,	ESCAPE,	OTHER,	OTHER,	OTHER,	OTHER,
	SPACE,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	BSLASH,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,
	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	PLAIN,	OTHER,
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,
	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,	OTHER,
	OTHER,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,
	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,
	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,
	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,
	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,
	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,
	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,
	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,
	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,
	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,
	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,
	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	EXTEND,	OTHER,
};

int
vis(fi)
	FILE *fi;
{
	register int c;
	register int i = 0;

	while((c = getc(fi)) != EOF) {
		switch(ctype[c]) {
			case PLAIN:
				printf("%c",c);
				break;
			case EXTEND:
			case OTHER:
				if (xflag)
					printf("\\x%.2x",c);
				else
					printf("\\%.3o", c);
				break;
			case BSPACE:
				printf("\\b");
				break;
			case HTAB:
				if(tflag)
					printf("\\t");
				else
					printf("%c",c);
				break;
			case NLINE:
				if(nflag)
					printf("\\n");
				else
					printf("%c",c);
				break;
			case VTAB:
				printf("\\v");
				break;
			case FFEED:
				printf("\\f");
				break;
			case RETURN:
				printf("\\r");
				break;
			case ESCAPE:
				printf("\\e");
				break;
			case SPACE:
				if(tflag)
					printf("\\s");
				else
					printf("%c",c);
				break;
			case BSLASH:
				printf("\\\\");
				break;
			}

			if(nflag && i == 15)
				printf("\n");
			i = (i + 1)%16;
		}
	return(0);
}

int
inv(fi)
	FILE *fi;
{
	register int c, hex, i, state, wd;

	hex = 0;
	state = 0;
	while ((c = getc(fi)) != EOF) {
		switch(state) {
		    case 0:
			switch(c) {
			    case EOF:
				return(0);
			    case '\\':
				state = 1;
				continue;
			    case '\n':
				if(!nflag)
					putchar(c);
				state = 0;
				continue;
			    case '\t':
			    case ' ':
				if(!tflag)
					putchar(c);
				state = 0;
				continue;
			    default:
				putchar(c);
				state = 0;
				continue;
			}
		    case 1:
			switch(c) {
			    case 'b':
				putchar('\b');
				state = 0;
				continue;
			    case 'e':
				putchar('\033');
				state = 0;
				continue;
			    case 'f':
				putchar('\f');
				state = 0;
				continue;
			    case 'n':
				putchar('\n');
				state = 0;
				continue;
			    case 'r':
				putchar('\r');
				state = 0;
				continue;
			    case 's':
				putchar(' ');
				state = 0;
				continue;
			    case 't':
				putchar('\t');
				state = 0;
				continue;
			    case 'v':
				putchar('\013');
				state = 0;
				continue;
			    case 'x':
				wd = 0;
				hex = 1;
				state = 2;
				continue;
			    case '\\':
				putchar('\\');
				state = 0;
				continue;
			    case '0':
			    case '1':
			    case '2':
			    case '3':
			    case '4':
			    case '5':
			    case '6':
			    case '7':
				wd = (c - '0');
				hex = 0;
				state = 2;
				continue;
			    default:
				if (!silent)
					fprintf(stderr,(nl_msg(9, "%s: state 1 illegal data \"%d\"\n")), program_name, c);
				return(1);
			}
    		    case 2:
			switch(c) {
			    case '0':
			    case '1':
			    case '2':
			    case '3':
			    case '4':
			    case '5':
			    case '6':
			    case '7':
				wd <<= 3 + hex;
				wd |= (c - '0');
				state = 3;
				continue;
			    case '8':
			    case '9':
				if(!hex) {
					if (!silent)
						fprintf(stderr,(nl_msg(10, "%s: state 2 illegal data \"%d\"\n")), program_name, c);
					return(1);
				}
				wd <<= 4;
				wd |= (c - '0');
				state = 3;
				continue;
			    case 'A':
			    case 'B':
			    case 'C':
			    case 'D':
			    case 'E':
			    case 'F':
				if(!hex) {
					if (!silent)
						fprintf(stderr,(nl_msg(11, "%s: state 2 illegal data \"%d\"\n")), program_name, c);
					return(1);
				}
				wd <<= 4;
				wd |= (c - 'A' + 10);
				state = 3;
				continue;
			    case 'a':
			    case 'b':
			    case 'c':
			    case 'd':
			    case 'e':
			    case 'f':
				if(!hex) {
					if (!silent)
						fprintf(stderr,(nl_msg(12, "%s: state 2 illegal data \"%d\"\n")), program_name, c);
					return(1);
				}
				wd <<= 4;
				wd |= (c - 'a' + 10);
				state = 3;
				continue;
			    default:
				if (!silent)
					fprintf(stderr,(nl_msg(13, "%s: state 2 illegal data \"%d\"\n")), program_name, c);
				return(1);
			}
    		    case 3:
			switch(c) {
			    case '0':
			    case '1':
			    case '2':
			    case '3':
			    case '4':
			    case '5':
			    case '6':
			    case '7':
				wd <<= 3 + hex;
				wd |= (c - '0');
				putchar(wd);
				hex = 0;
				state = 0;
				continue;
			    case '8':
			    case '9':
				if(!hex) {
					if (!silent)
						fprintf(stderr,(nl_msg(14, "%s: state 3 illegal data \"%d\"\n")), program_name, c);
					return(1);
				}
				wd <<= 4;
				wd |= (c - '0');
				putchar(wd);
				hex = 0;
				state = 0;
				continue;
			    case 'A':
			    case 'B':
			    case 'C':
			    case 'D':
			    case 'E':
			    case 'F':
				if(!hex) {
					if (!silent)
						fprintf(stderr,(nl_msg(15, "%s: state 3 illegal data \"%d\"\n")), program_name, c);
					return(1);
				}
				wd <<= 4;
				wd |= (c - 'A' + 10);
				putchar(wd);
				hex = 0;
				state = 0;
				continue;
			    case 'a':
			    case 'b':
			    case 'c':
			    case 'd':
			    case 'e':
			    case 'f':
				if(!hex) {
					if (!silent)
						fprintf(stderr,(nl_msg(16, "%s: state 3 illegal data \"%d\"\n")), program_name, c);
					return(1);
				}
				wd <<= 4;
				wd |= (c - 'a' + 10);
				putchar(wd);
				hex = 0;
				state = 0;
				continue;
			    default:
				if (!silent)
					fprintf(stderr,(nl_msg(17, "%s: state 3 illegal data \"%d\"\n")), program_name, c);
				return(1);
			}
		}
	}
	if (state != 0) {
		if (!silent)
			fprintf(stderr,(nl_msg(18, "%s: abnormal termination from state %d\n")), program_name, state);
		return(1);
	}
return(0);
}
