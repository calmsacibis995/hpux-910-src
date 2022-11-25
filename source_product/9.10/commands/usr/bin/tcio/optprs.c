/* @(#) $Revision: 66.1 $ */      

/*
 ##################
 # option parsing #
 ##################
 */

#include "tcio.h"

int Set_tape_size = 0;

parse_option(Option, block_number, filestoskip, argv, argc)
int *Option;
int *block_number;
int *filestoskip;
char *argv[];
int argc;
{
	char ch;
	extern int optind, opterr;
	extern char *optarg;

	*Option = -1;
	opterr = 0;	/* Suppress error diagnostics from getopt() */

#ifndef	DEBUG
	while ((ch = getopt(argc, argv, "iouVdrvZeXf:m:S:l:n:T:")) != EOF)
#else	DEBUG
	while ((ch = getopt(argc, argv, "iouVdB:rvZeXf:m:S:l:n:T:")) != EOF)
#endif	DEBUG
		switch (ch) {

			/* Input */
			case 'i':
				if (*Option >= 0)
					usage();
				*Option = IN;
				break;

			/* Output */
			case 'o':
				if (*Option >= 0)
					usage();
				*Option = OUT;
				break;

			/* Utility */
			case 'u':
				if (*Option >= 0)
					usage();
				*Option = UTILITY;
				break;

			/* suppress verify */
			case 'V':
				if (*Option != OUT && *Option != UTILITY
							|| --verifyflag < 0)
					usage();
				break;

			/* Check digit */
			case 'd':
				if (*Option != IN && *Option != OUT
							|| ++checkflag > 1)
					usage();
				break;

			case 'l':
				if (*Option < 0 || Merlinflag++ )
					usage();
				start_cart = atoi(optarg);
				if (start_cart < 1 || start_cart > 8 ) {
					fprintf(stderr,"tcio(1001): invalid cartridge number ( 1 <= cartridge <= 8 )\n");
					exit(USEERR);
				}
				break;

			case 'n':
				if (*Option < 0 || !Merlinflag ||Merlinflag_n++)
					usage();
				num_cart = atoi(optarg);
				break;

#ifdef	DEBUG
			/* PQP: Special DEBUG option to check EOT behavior
			 * Allows user to specify size of tape (in BUFSIZE blks)
			 */
			case 'B':
				if (*Option != OUT && *Option != IN)
					usage();
				Set_tape_size = atoi(optarg);
				break;
#endif	DEBUG

			/* Buffer size */
			case 'S':
				if (*Option != IN && *Option != OUT
					|| ++bufflag > 1)
					usage();
				maxindex = atoi(optarg) * BUFSIZE;
				if (maxindex > DEFINDEX)
					maxindex = DEFINDEX;
				else if (maxindex < MININDEX)
					maxindex = MININDEX;
				break;

			/* Release tape */
			case 'r':
				if (*Option < 0 || ++releaseflag > 1)
					usage();
				break;

			/* Mark tape */
			case 'm':
				if (*Option != UTILITY || ++markflag > 1)
					usage();
				*block_number = atoi(optarg);
				if (*block_number < 0) {
					err("tcio(1002): Invalid block number\n");
					exit(BNUMERR);
				}
				break;

			/* Verbose */
			case 'v':
				if (*Option < 0 || ++verboseflag > 1)
					usage();
				break;

			/* Don't write tape marks */
			case 'Z':
				if (*Option != IN && *Option != OUT
							|| ++Systemflag > 1)
					usage();
				break;

			/* end of data mark; NOT compatible with Series 500 */
			case 'e':
				if (*Option != OUT || ++eodmarkflag > 1)
					usage();
				break;

			/* file skip */
			case 'f':
				if (*Option != IN && *Option != OUT
					|| ++fileskipflag > 1)
					usage();
				*filestoskip = atoi(optarg);
				if (*filestoskip < 0) {
					err("tcio(1003): Invalid file number\n");
					exit(FNUMERR);
				}
				break;

                        case 'X': /* hidden flag to force prompts to stderr */
                                ++Xflag;
                                break;

			/* Specify alternate to /dev/tty. */
			case 'T':
				if ( *Option == UTILITY || ++Alt_tty > 1 )
				    usage();
				Tty = optarg;
				break;

			case '?':
			default:
				usage();
		}

        if (*Option < 0 || (fname = argv[optind++]) == NULL
		|| argv[optind] != NULL) 
		usage();

	return ;
}


usage()
{
	fprintf(stderr,"Usage: tcio -o[dervVZ] [-S buffersize] [-l number [-n limit]] [-T tty] file\n");
	fprintf(stderr,"       tcio -i[drvZ] [-S buffersize] [-l number [-n limit]] [-T tty] file\n");
	fprintf(stderr,"       tcio -u[rvV] [-m blocknumber] [-l number ] file\n");
	exit(USEERR);
}


errv(a, b, c, d, e)
{
	if (verboseflag) {
		fprintf(stderr, a, b, c, d, e);
		fflush(fdopen(stderr, "w"));
	}
}


err(a, b, c, d, e)
{
	fprintf(stderr, a, b, c, d, e);
	fflush(fdopen(stderr, "w"));
}


rel_exit(exit_no)
int exit_no;
{
	if (releaseflag)
		unload_tape(fildes);
	exit(exit_no);
}
