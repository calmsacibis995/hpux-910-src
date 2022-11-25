static char *HPUX_ID = "@(#) $Revision: 72.1 $";

/* @(#) $Revision: 72.1 $ */    
/*

        This program is a filter that  converts  backspace  overstrike  to
        line  overprint  with horizontal print positioning to enhance bold
        print.


*/

#include <stdio.h>

#define TRUE 1
#define FALSE 0

#define CR '\r'
#define BS '\b'
#define NL '\n'
#define TAB '\t'
#define FF '\f'
#define SO '\016'
#define SI '\017'
#define LINESIZE 1024
#define MAX_OVERPRINT 10
#define tabstop 8
#define USAGE "Usage: %s [-i(talics)] [-o(dd)] [-e(ven)] [-l(en)nn] [-n(roff)]\n"

int count;        /* count of number of newlines found */
int italics = 0;  /* State var true=translate underscore to italics */
int shiftout = 0; /* State var indicating shift out state */

int j;            /* index into buffs */
int k;            /* index into bufs[*] i.e. second dimension */
int nbufs;        /* counts how many buffers are used */
int col;          /* current position in line */
int len;          /* maximum length of line (so far) */
int newcol;       /* next column if no errors occurs */
int pagelen=0;    /* length of outputed page */
int defltlen;	  /* default page length */
int pagenum=1;	  /* Page counter */
int linenum=1;    /* Line counter */
int skipmode=2;   /* Indicates that all pages are printed */
int firstline=1;  /* Line number of first line on page to print */
int lastline; 	  /* Line number of last line on page to print */
short bufs[MAX_OVERPRINT][LINESIZE];



main( argc, argv )
int argc;
char *argv[];
{   
	int c,ct,last_char;
	char *progname;
	int nroffmode=0;

	/* process arguments */

	progname = argv[0];

	while ( --argc > 0 && (*++argv)[0] == '-' )
		switch ( (*argv)[1] ) {   
		case 'i':
			italics = 1;
			break;
		case 'l':
			pagelen = atoi((*argv)+2);
			break;
		case 'o':
			skipmode = 1;
			break;
		case 'e':
			skipmode = 0;
			break;
		case 'n':
			nroffmode = 1;
			break;
		case 'p':
			nroffmode = 2;
			break;
		default : 
			fprintf( stderr,USAGE,progname );
			exit(1);          
		}     

	if ( argc != 0 ) {   
		fprintf( stderr,USAGE,progname );
		exit(2);                    
	}

	defltlen = (nroffmode) ? 66 : 60;
	lastline = pagelen = (pagelen) ? pagelen : defltlen;
	switch (nroffmode) {
	case 1: firstline = 4;
		lastline = pagelen - 3;
		break;
	case 2: firstline = 3;
		lastline = pagelen - 4;
		break;
	}
	nbufs = 0; 
	col = 0; 
	len = 0;
	bufs[nbufs][col] = ' ';

	while ((c = getchar()) != EOF) {   
		last_char=c;
		switch (c) {   
		case BS: 
			newcol = col - 1;
			break;

		case CR: 
			newcol = 0;
			break;

		case FF: 
			if ((ct = getchar()) != NL) ungetc(ct,stdin);
			linenum = pagelen;
		case NL: 
			printline(c);
			break;

		case TAB: 
			newcol = tabstop*( col / tabstop + 1 );
			break;

		case ' ': 
			newcol = col + 1;
			break;

		case '_':
			if (italics && (ct = bufs[0][col]) != ' ') {
				bufs[0][col] = (ct | 0400);
				newcol = col + 1;
				break;
			}
		default: 
			newcol = col + 1;
			j = 0;
/* try to find a row in this column of initialized bufs to store c */
			while ((j<=nbufs) && (bufs[j][col] != ' ')) j++;
			if (italics && (j <= 1) && (bufs[0][col] == '_')) {
				/* setup for later shiftout operation */
				/* converts _\b<CHAR> to italics */
				bufs[0][col]= (c | 0400);
				break;
			}
/* initialize a new row in bufs and store c */
			if (j > nbufs) {
				if (nbufs+1 > MAX_OVERPRINT) {
					fprintf(stderr,"too many overprints\n");
					break;
				}
				nbufs++;
				for (k = 0; k <= len; k++)
					bufs[nbufs][k] = ' ';
			}
			bufs[j][col] = c; 
			break; 
		}

		/* advance column pointer */

		if (newcol >= LINESIZE) {
			fprintf(stderr,"line too long\n");
				/* if a raw PCL file is sent through */
				/* lprpp, then c=getchar() may never */
				/* see a NL prior to EOF--added test */
				/* for EOF.  Print request will abort*/
				/* with "line too long" message.     */
			while (((c=getchar()) != NL) && (c != EOF));
			ungetc(c,stdin);
		}
		else if (newcol < 0)
			fprintf(stderr,"attempt to backspace over the edge\n");
		else { 
			col = newcol;
			while ( len < col ) { 
				len++;
				for ( j = 0; j <= nbufs; j++ ) 
					bufs[j][len] = ' '; 
			}
		}
	}
	if ((last_char!=NL)&&(last_char!=FF))
	   {
	   printline(NL);
	   }
	if (!skipmode) {	/* Doing even numbered pages */
		if ((pagenum & 1) && (linenum != 1)) putchar(FF);
		if (!(pagenum & 1) && (linenum == 1)) putchar(FF);
	}
}

printline(term)
int term;
{
	if ((skipmode == 2) || ((pagenum & 1) == skipmode))
		if ((linenum >= firstline) && (linenum < lastline))
			outputline(term);
		else if (linenum == lastline) outputline(FF);


	if (++linenum > pagelen) {
		pagenum++;
		linenum=1;
	}
	nbufs = 0; 
	col = 0; 
	len = 0; 
	newcol = 0;
	bufs[nbufs][col] = ' ';
}

outputline(term)
int term;
{
	int c, overstrike;
/* print the first buffer (bufs[0][*]), with italics if necessary. */
	for (k = 0; k <= len-1; k++) {
		c=bufs[0][k];
		if (c & 0400) {
			if (!shiftout) {
				printf("\033(s1S");
				shiftout = 1;
			}
			putchar(c & 0377);
		}
		else {
			if (shiftout) {
				printf("\033(s0S");
				shiftout = 0;
			}
			putchar(c);
		}
/* Check the rest of column k for overstrikes (i.e. different chars) */
/* Print BS and overstrike char, then blank out bufs[j][k] */
/* If emboldended, just leave them alone for later printing */
		for (j = 1; j <= nbufs; j++) {
		    overstrike = bufs[j][k];
		    if (overstrike != ' ' && overstrike != c) {
			putchar(BS);
			putchar(overstrike);
			bufs[j][k] = ' ';
		    }
		}
	}
	if (shiftout) {
		printf("\033(s0S");
		shiftout = 0;
	}
/* now print the emboldening rows */
	for ( j = 1; j <= nbufs; j++ ) { 
		putchar(CR);
		switch(j) { 
		case 0:
			break;
		case 1:  
			/*
                            ______
			*/
			printf("\033&a+6h+0V");
			break;
		case 2:  
			/*
			           /
			    ______/
			*/
			printf("\033&a+3h+3V");
			break;
		case 3:  
			/*
				   /\
			    ______/  \
			*/
			printf("\033&a+3h-3V");
			break;
		default: 
			break;     
		}
		for ( k = 0; k <= len-1; k++ )
			putchar( bufs[j][k] );         
	} 
/* terminate the line */
        if (term == NL)
                putchar(CR);
	putchar(term);
}
