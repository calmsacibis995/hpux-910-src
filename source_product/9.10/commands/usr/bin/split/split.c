static char *HPUX_ID = "@(#) $Revision: 70.5 $";

#include <stdio.h>
#include <nl_types.h>
#include <locale.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <strings.h>
#define NL_SETN 1

#define PATH_SIZE 1024		/* file path size */

char 	buffer [ BUFSIZ ];	/* read/write buffer */

extern char *optarg;
extern int optind, opterr;

nl_catd catd;			/* message catalog descriptor */

/*********************************************************************
This function checks to see if the output file names will be 
truncated when they are created.
*/
void
check_name( fname )
char *fname;
{
char dirpath[ PATH_SIZE ];		/* directory to check for file name length */
char *ptr;						/* pointer to basename of file */


if ( (ptr = strrchr ( fname, '/' )) == NULL ){		/* no leading dir */
	strcpy ( dirpath, "." );
	ptr = fname;
	}
else {
	strcpy ( dirpath, fname );
	*( strrchr ( dirpath, '/' ) ) = '\0';	/* remove basename */
	ptr++;
	}

if ( strlen ( ptr ) > (int) pathconf ( dirpath, _PC_NAME_MAX ) ) {
	fputs((catgets(catd,NL_SETN,1, "output file name too long\n")), stderr);
	exit ( 1 );
	}
	
}

/*********************************************************************
This function writes bytes number of characters read from infile to 
the file named fname.
*/

int
write_bytes ( infile, fname, bytes )
FILE *infile;			/* input file stream, already opened */
char *fname;			/* output file name */
unsigned bytes;			/* number of bytes to write */
{
size_t 	read_size = BUFSIZ;	/* number of bytes to read from infile */
FILE 	*ofile;				/* output file stream */
int 	num_read = BUFSIZ;	/* number of bytes read */
int		num_write;			/* number of bytes written */
int		bytes_output = 0;	/* number of bytes written to output file */

ofile = fopen ( fname, "w" );
if ( ofile == NULL ) {
	fputs((catgets(catd,NL_SETN,2, "cannot open output file\n")), stderr);
	exit ( 1 );
	}

while (( num_read == BUFSIZ ) && ( bytes > 0 ) ) {
	if ( bytes >= BUFSIZ ) 		/* > one buffer to write */
		bytes -= BUFSIZ;
	else {		/* < one buffer to write, output file nearly full */
		read_size = bytes;
		bytes = 0;
		}

	num_read = fread ( buffer, 1, read_size, infile );

	num_write = fwrite ( buffer, 1, num_read, ofile );
	if ( ( num_write == 0 ) && ( num_read > 0 ) ) {
		fputs((catgets(catd,NL_SETN,3, "cannot write output file\n")), stderr);
		exit ( 1 );
		}

	bytes_output += num_write;

	}		/* while */

fclose ( ofile );
if ( bytes_output == 0 )		/* remove zero length output file created   */
	unlink ( fname );			/* when the input file is an exact multiple */
								/* of the output file size */

if ( num_read < read_size ) 	/* input file exhausted */
	return ( 1 );
else
	return ( 0 );
}


/*********************************************************************
This function writes lcount lines read from infile to the file named
fname.
*/

int 
write_lines ( infile, fname, lcount )
FILE *infile;		/* input file stream, already opened */
char *fname;		/* output file name */
unsigned lcount;	/* number of lines to write to output file */
{

FILE *ofile;			/* output file */
unsigned count = 0;		/* number of lines written so far */
int c = 10;				/* the char. read */

ofile = fopen ( fname, "w" );
if ( ofile == NULL ) {
	fputs((catgets(catd,NL_SETN,2, "cannot open output file\n")), stderr);
	exit ( 1 );
	}

while ( ( count < lcount ) && ( c != EOF ) ) {
	c = getc ( infile );
	if ( c == '\n' )
		count++;

	if ( c != EOF )
		putc ( c, ofile );
	}

fclose ( ofile );

if ( c == EOF )
	return ( 1 );
else
	return ( 0 );
}
/*********************************************************************
This function increments the aaa field at the end of the output file
name string.
*/

void 
increment_name ( start, end )
char * start; 	/* pointer to start of aaa field in name */
char * end; 	/* pointer to last char of aaa field in name */
{
while (1) {
	if ( start == end ) {
		fputs((catgets(catd,NL_SETN,4, "output file suffix length too small, aborting split\n")), stderr);
		exit(1);
		}

	if ( *end != 'z' ) {
		(*end)++;			/* increment character */
		break;
		}
	else {
		(*end) = 'a';		/* start over counting from a */
		end--;
		}
	}
}

/*********************************************************************/

void 
usage()
{
fputs((catgets(catd,NL_SETN,5,"usage: split [-l line_count ] [-a suffix_length ] [file] name ]]\n")), stderr);
fputs((catgets(catd,NL_SETN,6,"       split [-b byte_count[k|m]] [-a suffix_length ] [file] name ]]\n")), stderr);
exit(1);
}

/*********************************************************************/

main(argc, argv)
char *argv[];
{
	unsigned lcount = 1000;	/* number of lines per output file */
	char	fname[ PATH_SIZE ];	/* output file name with suffix */
	char	*infile_name;   /* input file name */
	char	*ofil = NULL;   /* output file name without suffix */

    char 	optch;
	int 	done = 0;		/* boolean, 1 = all output files written */
	int 	x;
	char 	larg [ 16 ] ;	/* used for artificial -l option */

	unsigned bytes = 0;		/* # of bytes in output file */
	char 	*byte_mult;		/* k or m * bytes output file size */
	int suffix_length=2;	/* # of char in the suffix */

	char 	*end;			/* points to end of output file name string var */
	char 	*start;			/* points to start of output file name string var*/
	FILE 	*infile;		/* input file */


    catd = catopen( "split", 0 );
    
	setlocale ( "LC_ALL", "" );	

	/* convert old -n option to -ln for proper getopt handling */
	for ( x = 1; x < argc; x++ )
		if ( isdigit ( argv [ x ] [ 1 ] )  && ( argv [ x ] [ 0 ] == '-' ) ) {
			strcpy ( larg, "-l" );
			strcat ( larg, &argv [ x ] [ 1 ] );
			argv [ x ] = larg;
			}

    while ( ( optch = getopt ( argc, argv, "a:b:l:" ) ) != EOF ) 
	    switch ( optch )
	    {
		case 'l':
			lcount = atoi( optarg );
			if (lcount == 0)
				lcount = 1000;
			break;

		case 'a':
			suffix_length = atoi ( optarg );
			if ( suffix_length == 0 )
				suffix_length = 2;
			break;

		case 'b':
			bytes = atoi ( optarg );
			if ( bytes != 0 ) {
				byte_mult = optarg + strlen ( optarg ) - 1;
				if ( *byte_mult == 'k' )
					bytes *= 1024;
				else if ( *byte_mult == 'm' )
					bytes *= 1048576;
				else if ( ! isdigit ( *byte_mult ) )
					usage();
				}
			break;

	    case '?':
			usage();
	    }

	
     if ( optind >= argc ) 
			infile = stdin;			/* use stdin */
     else {
        infile_name = argv [ optind++ ];
		if ( strcmp ( infile_name, "-" ) == 0 )
			infile = stdin;
		else  {
			if ( ( infile = fopen ( infile_name, "r" ) ) == NULL ) {
				fputs((catgets(catd,NL_SETN,7, "cannot open input file\n")), stderr);
	    		exit(1);
	    		}
			}
		}  

	ofil = argv [ optind ]; 

    if (ofil == NULL)
		ofil = "x";		/* default output file name */

	/* make fname = fname + # a's  (ie: file1aaa if suffix_length = 3) */
	strcpy ( fname, ofil );
	start = fname + strlen ( fname ) - 1;
	for ( x = strlen ( fname ) ; suffix_length ; x++, suffix_length-- )
		fname [ x ] = 'a';
	fname [ x ] = '\0';
	end = fname + strlen ( fname ) - 1;
	
	check_name ( fname );

	while ( ! done ) {
		if ( bytes ) 		/* byte output mode (-b) */
			done = write_bytes ( infile, fname, bytes );
		else 			/* line output mode (default or -l) */
			done = write_lines ( infile, fname, lcount );

		increment_name ( start, end );
		}		/* while not done */

	fclose ( infile );
	return(0);				
}
