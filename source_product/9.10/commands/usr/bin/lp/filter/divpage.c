/*  $Revision: 72.1 $  */

#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>

#define QUARTER		1
#define PORTRAIT	2

#define LANDSCAPE_HALF		0
#define LANDSCAPE_QUARTER	1
#define PORTRAIT_HALF		2
#define PORTRAIT_QUARTER	3

/*  control codes  */
#define TAB '\011'
#define BS '\010'
#define FF '\014'
#define CR '\015'
/*  extra column buffer size for appearance of Backspace character  */
#define OVERPRINT line_length*5

char *buffer;		/*  pointer to top of buffer  */
unsigned int  fontnum;	/*  font id number  */
unsigned int  style;		/*  dividing style  */
int  page_length;	/*  number of row in a page  */
int  line_length;	/*  number of column in a page  */
int  pages;		/*  how many pages in a sheet */
int  sides;		/*  how many sides in a sheet */
int  buf_maxrow;	/*  row size of buffer  */
int  buf_maxcol;	/*  column size of buffer  */
int  buf_col; 		/*  real column size of buffer ( = buf_maxcol + 1) */

char *cmdname;		/*  name of this command  */
FILE *fp;
int  noread;		/*  is there nothing text to print?  */

main( argc, argv )
	int argc;
	char *argv[];
{
	int i;
	int readeof;		/*  is EOF read ?  */
	char *filename;		/*  filename to print  */
	char ch;
	int errflg, lflg, pflg, hflg, qflg;
	char c;
	extern char *optarg;
	extern int optind;

	cmdname = argv[0];
	errflg = lflg = pflg = hflg = qflg = 0;
	page_length = line_length = pages = sides = style = fontnum = 0;
	filename = NULL;

	while ((c = getopt(argc,argv,"hlpqn:")) != EOF) {
		switch ( c ) {
			case 'l' :
				   if (pflg)
					errflg++;
				   else {
 					style &= ~PORTRAIT;
					lflg++;
				   }
				   break;
			case 'p' :
				   if (lflg)
					errflg++;
				   else {
					style |= PORTRAIT;
					pflg++;
				   }
				   break;
			case 'h' :
				   if (qflg)
					errflg++;
				   else {
					style &= ~QUARTER;
					hflg++;
				   }
				   break;
			case 'q' :
				   if (hflg)
					errflg++;
				   else {
					style |= QUARTER;
					qflg++;
				   }
				   break;
			case 'n' :
				   fontnum = atoi( optarg );
				   break;
			case '?' : errflg++;
				   break;
		}
	}
	if (errflg) {
		fprintf(stderr,
		    "usage: %s [ -p | -l] [ -h | -q ] [ -nFontID ] filename\n"
							,cmdname);
		exit(1);
	}
	filename = argv[optind];

	switch ( style ) {
		case LANDSCAPE_HALF :
			page_length = 66;
			line_length = 136;
			pages = 2;
			sides = 1;
			break;
		case PORTRAIT_QUARTER :
			page_length = 66;
			line_length = 80;
			pages = 2;
			sides = 2;
			break;
		case LANDSCAPE_QUARTER :
			page_length = 66;
			line_length = 136;
			pages = 2;
			sides = 2;
			break;
		case PORTRAIT_HALF :		/*  default style  */
		default :
			page_length = 66;
			line_length = 85;
			pages = 1;
			sides = 2;
			break;
	}

	if (filename == NULL) {		/* read from standard input  */
		fp = stdin;
	}
	else {				/* read from the file  */
		if ( (fp = fopen(filename,"r")) == NULL) {
			fprintf(stderr,"Cannot open %s\n",filename);
			exit(1);
		}
	}

	get_buffer();

	setup_printer();
	setup_page();

	if ( (ch = getc(fp)) == FF )	{
		if ( (ch = getc(fp)) != '\n' )	{
			ungetc( ch, fp );
		}
	} else		ungetc( ch, fp );

	do {				/*  print each sheet  */
		noread = 1;
		for (i=0; i<pages; i++) {	/* print each page in a sheet */
			readeof = read_text();
			if (noread && i==0)  break;
			if ( i != 0 )	putchar('\n');
			noread = 0;
			put_text();
			if (readeof) break;
		}
		if ( noread )  break;
		rule();
		setup_page();
		putchar(FF);			/* feed a sheet  */
	} while ( !readeof );

	free(buffer);
	close(fp);
	exit(0);
}

get_buffer()
{
	buf_maxrow = page_length * sides;
	buf_maxcol = line_length + OVERPRINT;
	buf_col = buf_maxcol + 1;
	if ( (buffer = (char *)malloc( buf_maxrow * buf_col)) == NULL) {
		fprintf(stderr,"%s: cannot get buffer\n",cmdname);
		exit(1);
	}
	return;
}

/*  read input text into a buffer with separating lines  */
read_text()
{
	int col_over;		/*  column overflowed flag  */
	int row,col;		/*  buffer row,column count  */
	int pr_col;		/*  print column (with consideration BS) */
	char ch;
	char *buf_p;

	row = col = pr_col = col_over = 0;
	buf_p = buffer;
	while ( (ch = getc(fp)) != EOF ) {
		noread = 0;
		if (ch == '\n') {
			if ( !col_over )  *buf_p = '\n';
			buf_p = buffer + (++row * buf_col);	/*to next row*/
			if (row >= buf_maxrow)	break;		/*next page?*/
			col = pr_col = col_over = 0;
		}
		else if (ch == FF) {			/* next page */
			if ( (ch = getc(fp)) != '\n' )	ungetc( ch, fp );
			if ( !col_over )  *buf_p = '\n';
			while ( ++row % page_length )
				*( buffer + ( row * buf_col ) ) = '\n';
			buf_p = buffer + ( row * buf_col );
			if (row >= buf_maxrow)	break;
			col = pr_col = col_over = 0;
		}
		else if ( !col_over ) {
			switch ( ch ) {
			    case TAB :
				do {		/* replace TAB by spaces */
					*buf_p++ = ' ';
					if (++col >= buf_maxcol ||
						   ++pr_col >= line_length) {
						*buf_p++ = '\n';
						col_over = 1;
						break;
					}
				} while (pr_col % 8);
				break;
			    case BS :		/*print-column is decremented*/
				if (pr_col == 0)  break;
				*buf_p++ = ch;
				if (++col >= buf_maxcol) {
					*buf_p++ = '\n';
					col_over = 1;
				}
				pr_col--;
				break;
                            case CR :
                                pr_col=0;

			    default :
				*buf_p++ = ch;
				if (++col >= buf_maxcol ||
						++pr_col >= line_length) {
					*buf_p++ = '\n';
					col_over = 1;
				}
				break;
			}
		}
	}
	if ( !col_over && ch!='\n' )  *buf_p = '\n';
	buf_p = buffer + (++row * buf_col);
	for (; row<buf_maxrow; row++) {	/* fill NLs until end of raw */
		*buf_p = '\n';
		buf_p += buf_col;
	}
	if ( ch == EOF )
		return( 1 );
	else
		return( 0 );
}

/*  print a text from buffer  */
put_text()
{
	int  pr_side;			/* current side  */
	int  row;			/* buffer row count */
	int  pr_row,pr_col;		/* print row/column count */
	char ch;
	char *buf_p;

	for (row=0; row < page_length; row++) {
		pr_side = 0;
		while ( pr_side < sides) {
			pr_row = row + page_length * pr_side;
			buf_p = buffer + (pr_row * buf_col);
			pr_col = 0;
			while( (ch = *buf_p++) != '\n') {
				putchar(ch);
				if (ch == BS)
					pr_col--;
				    else
					pr_col++;
			}
			if ( ++pr_side == sides )
				putchar('\n');
			    else {	/* fill spaces until next side */
				for (;pr_col<line_length; pr_col++)putchar(' ');
				putchar(' ');
				putchar(' ');
			}
		}
	}
}

/*  set up a printer  */
setup_printer()
{
	switch ( style ) {
	    case PORTRAIT_HALF :	/* this mode uses internal font */
		printf("\033&l1O");			/* landscape mode */
		printf("\033(8U");			/* Roman-8 */
		printf("\033(s0p8.5v0s0b0T");
			/* fixed,16.66CPI,8.5Pt,Upright,Medium,LinePrinter*/
		printf("\033(s16.66H\033)s16.66H");	/* pitch */

		printf("\033&l5.59C");			/* vertical motion */
		printf("\033&k7.10H");			/* horizontal motion */
		break;
	    case LANDSCAPE_QUARTER :
		printf("\033&l1O");			/* landscape mode */
		printf("\033(%dX",fontnum);		/* select font id */
		printf("\033&l2.76C");			/* vertical motion */
		printf("\033&k4.56H");			/* horizontal motion */
		break;
	    case LANDSCAPE_HALF :
		printf("\033&l0O");			/* portrait mode */
		printf("\033(%dX",fontnum);		/* select font id */
		printf("\033&l3.79C");			/* vertical motion */
		printf("\033&k6.60H");			/* horizontal motion */
		break;
	    case PORTRAIT_QUARTER :
		printf("\033&l0O");			/* portrait mode */
		printf("\033(%dX",fontnum);		/* select font id */
		printf("\033&l3.79C");			/* vertical motion */
		printf("\033&k5.65H");			/* horizontal motion */
		break;
	}
	printf("\033&l0L");			/* perf skip disale */

	return;
}

/*  setup printer for each page  */
setup_page()
{
	switch ( style ) {
	    case PORTRAIT_HALF :
		printf("\033&l2E");	/* top margin */
		printf("\033&a4L");	/* left margin */
		printf("\033&l%dF",page_length*pages);	/* page length */
		break;
	    case LANDSCAPE_QUARTER :
		printf("\033&l5E");	/* top margin */
		printf("\033&a4L");	/* left margin */
		printf("\033&l%dF",page_length*pages+1);	/* page length*/
		break;
	    case LANDSCAPE_HALF :
		printf("\033&l4E");	/* top margin */
		printf("\033&a3L");	/* left margin */
		printf("\033&l%dF",page_length*pages+1);	/* page length*/
		break;
	    case PORTRAIT_QUARTER :
		printf("\033&l4E");	/* top margin */
		printf("\033&a3L");	/* left margin */
		printf("\033&l%dF",page_length*pages+1);	/* page length*/
		break;
	}

	return;
}

/*  print rules  */
/*  rules are printed as narrow rectangles  */
rule()
{
	if ( style == PORTRAIT_HALF || style == LANDSCAPE_QUARTER ) {
		printf("\033&l0E\033&a0L");		/*  top/left margin  */
		/* horizontal lines */
		printf("\033*p0x50Y\033*c3178A\033*c1B\033*c0P");
		printf("\033*p0x2386Y\033*c3178A\033*c1B\033*c0P");
		/* vertical lines */
		printf("\033*p0x50Y\033*c1A\033*c2336B\033*c0P");
		printf("\033*p3178x50Y\033*c1A\033*c2336B\033*c0P");
	} else {
		printf("\033&l0E\033&a0L");		/*  top/left margin  */
		/* horizontal lines */
		printf("\033*p0x60Y\033*c2336A\033*c1B\033*c0P");
		printf("\033*p0x3238Y\033*c2336A\033*c1B\033*c0P");
		/* vertical lines */
		printf("\033*p0x60Y\033*c1A\033*c3178B\033*c0P");
		printf("\033*p2336x60Y\033*c1A\033*c3178B\033*c0P");
	}
	switch ( style ) {
	    case PORTRAIT_HALF :
		/* vertical line */
		printf("\033*p1587x50Y\033*c1A\033*c2336B\033*c0P");
		break;
	    case LANDSCAPE_QUARTER :
		/* horizontal line */
		printf("\033*p0x1223Y\033*c3178A\033*c1B\033*c0P");
		/* vertical line */
		printf("\033*p1587x50Y\033*c1A\033*c2336B\033*c0P");
		break;
	    case LANDSCAPE_HALF :
		/* horizontal line */
		printf("\033*p0x1654Y\033*c2336A\033*c1B\033*c0P");
		break;
	    case PORTRAIT_QUARTER :
		/* horizontal line */
		printf("\033*p0x1654Y\033*c2336A\033*c1B\033*c0P");
		/* vertical line */
		printf("\033*p1168x60Y\033*c1A\033*c3178B\033*c0P");
		break;
	}

	return;
}

