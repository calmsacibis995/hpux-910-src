static char *HPUX_ID = "@(#) $Revision: 66.1 $";
#ifndef NLS
#define catgets(i,sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
nl_catd nl_fn;
#endif NLS
/*	col - filter reverse carraige motions
 *
 *
 */

 /* The source is changed on Jan. 5, 1983 by Caroline Lazaar for two reasons:

	1_ LINELN of 800 was too small in the case of lines with lots of
	   overstrikes and backspaces. The maximum # of characters in an input
	   line (that makes sence) is calculated as follows:
		132 		Maximum # of columns per line.
		7		A BOLD char is actually 7 charcters.
				The character is printer 4 times each time
				followed by a back space except the last time
				which makes 3 back spaces.
		132*7=924	Maximum # of chars for a full line of BOLD
				charcters.
		924*2=1848	Maximum # of chars for a full line of BOLD
				characters that are underlined with BOLD
				underscores.
	2_ Changing the constant LINELN from 800 to 1848 seemed to work
	   fine since the storage is allocated and freed dynamically.
	   Now, if there exist a longer line the program will simply write
	   an error routine and quit.
*/


# include <stdio.h>
#ifdef NLS16
#include <langinfo.h>	/* NLS tools */
#include <nl_ctype.h>	/* NLS tools */
#include <locale.h>
#endif


# define PL 256
# define ESC '\033'
# define RLF '\013'
# define SI '\017'
# define SO '\016'
# define GREEK 0400	/* NLS(8 bit): use 9th bit for flag */
# define LINELN 1848    /* maximum number of chars allowed in an input line
			   counting the backspaces and overstirkes */

#ifdef NLS16
#define HP15_1st 01000  /* use 10th bit for kanji 1st code flag */
#define HP15_2nd 02000  /* use 11th bit for kanji 2nd code flag */
#define PADCHAR  04000  /* replace printed char with PADCHAR in */
			/* -l option treatment.			*/
#define UNDLINE 010000  /* This flag is set when Kanji underline*/
			/* is detected.				*/
#define SECONDof2(c) !(iscntrl(c)||isspace(c))
extern void Setchar();
#endif

typedef unsigned short int  lchar;	/* NLS(8 bit): define new type of 'long' characters */

lchar *page[PL];
lchar lbuff [LINELN], *line;
int bflag, xflag, fflag, pflag, lflag;
int half;
int cp, lp;
int ll, llh, mustwr;
int pcp = 0;
char *pgmname;
#ifdef NLS16
#define INSTRLEN 8 	/* The max. length of strings to be inserted in "line"*/
int hp15_stk;		/* Stack for first byte of HP15 code. */
lchar ibuf[INSTRLEN];	/* Stack to store strings to be inserted in "line". */
#endif

lchar blank[] = {' ','\0'};	/* NLS: declare this to maintain type consistency with emit() */

main (argc, argv)
	int argc; char **argv;
{
	int i;
	int greek;
	register int c;
#ifdef NLS16
	int c1,c2,c3,c4,c5;
#endif
	static lchar fbuff[BUFSIZ];


#if defined NLS || defined NLS16	/* initialize to the right language */
	nl_catd	nl_fn;

	if (!setlocale(LC_ALL,"")) {
		fputs(_errlocale(),stderr);
		nl_fn=(nl_catd)-1;
	}
	else
		nl_fn=catopen("col",0);
#endif NLS || NLS16

	setbuf (stdout, fbuff);
	pgmname = argv[0];

	for (i = 1; i < argc; i++) {
		register char *p;
		if (*argv[i] != '-') {
			fprintmsg(stderr, (catgets(nl_fn,NL_SETN,1, "%1$s: bad option %2$s\n")),
				pgmname, argv[i]);
			exit (2);
		}
		for (p = argv[i]+1; *p; p++) {
			switch (*p) {
			case 'b':
				bflag++;
				break;

			case 'x':
				xflag++;
				break;

			case 'f':
				fflag++;
				break;

			case 'p':
				pflag++;
				break;

			case 'l':
				lflag++;
				break;

			default:
				fprintmsg(stderr, (catgets(nl_fn,NL_SETN,2, "%1$s: bad option letter %2$c\n")),
					pgmname, *p);
				exit (2);
			}
		}
	}

	for (ll=0; ll<PL; ll++)
		page[ll] = 0;

	cp = 0;
	ll = 0;
	greek = 0;
	mustwr = PL;
	line = lbuff;
#ifdef NLS16
	/* Call Getchar() to set 16-bit code flag. */
	while ((c = Getchar()) != EOF) {
#else
	while ((c = getchar()) != EOF) {
#endif
		switch (c) {
		case '\n':
			incr();
			incr();
			cp = 0;
			continue;

		case '\0':
			continue;

		case ESC:
#ifdef NLS16
			/* Call Getchar() to set 16-bit code flag. */
			c = Getchar();
#else
			c = getchar();
#endif
			switch (c) {
			case '7':	/* reverse full line feed */
				decr();
				decr();
				break;

			case '8':	/* reverse half line feed */
				if (fflag)
					decr();
				else {
					if (--half < -1) {
						decr();
						decr();
						half += 2;
					}
				}
				break;

			case '9':	/* forward half line feed */
				if (fflag)
					incr();
				else {
					if (++half > 1) {
						incr();
						incr();
						half -= 2;
					}
				}
				break;

			default:
				if (pflag)	{	/* pass through esc */
					outc(ESC);
					cp++;
					outc((lchar)c);
					cp++;	}
				break;
			}
			continue;

		case SO:
			greek = GREEK;
			continue;

		case SI:
			greek = 0;
			continue;

		case RLF:
			decr();
			decr();
			continue;

		case '\r':
			cp = 0;
			continue;

		case '\t':
			cp = (cp + 8) & -8;
			continue;

		case '\b':
			if (cp > 0)
				cp--;
			continue;

		case ' ':
			cp++;
			continue;
#ifdef NLS16
		case '_':
			/* Underlined Kanji char is transported into 	*/ 
			/* internal code to prevent the sequence	*/
			/* __BSBSXX(XX is Kanji) to be destroyed.	*/
			c1=Getchar();
			if (c1=='_') {
				c2=Getchar();
				if ( c2=='\b' ) {
					c3=Getchar();
					if ( c3=='\b' ) {
						c4=Getchar();
						if ( c4&HP15_1st ) {
							c=c4|UNDLINE;
						} else {
							Setchar(c4);
							Setchar('\b');
							Setchar('\b');
							Setchar('_');
						}
					} else {
						Setchar(c3);
						Setchar('\b');
						Setchar('_');
					}
				} else {
					Setchar(c2);
					Setchar('_');
				}
			} else {
				Setchar(c1);
			}
#endif
		default:
#ifdef NLS16
			/* Remove to avoid clearing 16-bit code flag. */
#else
			c &= 0377;	/* NLS: strip with 0377 instead of 0177 */
#endif
#ifdef NLS16
			/* 16-bit code is considered as printable char. */
			if (    c&HP15_1st 
			     || c&HP15_2nd 
			     || isprint(c&0377)!=0 ){ 
#else
			if ((c & 0177) > 040 && (c & 0177) < 127)  {	
#endif
				/* if printable */
				outc((lchar)c | greek);
				cp++;
			}
			continue;
		}
	}

	for (i=0; i<PL; i++)
		if (page[(mustwr+i)%PL] != 0)
			emit (page[(mustwr+i) % PL], mustwr+i-PL);
	emit (blank, (llh + 1) & -2);
	return 0;
}

outc (c)
	register lchar c;
{
#ifdef NLS16
	int count;
#else
	register int count;
#endif

#ifdef NLS16
	if ( c&HP15_1st ) {
		hp15_stk = c; 	/* Save c in hp15_stk for next call */
		return;
	} else if ( c&HP15_2nd ) {
		cp--;	/* Set cp at the left edge of 16-bit char */
	}
#endif
	count = 0;
	if (lp > cp) {
		line = lbuff;
		lp = 0;
	}

#ifdef NLS16
	while (lp < cp) {
		switch (*line) {
		case '\0':
			*line = ' ';
			count++;
			lp++;
			line++;
			break;
		default:
			while (    line[1]=='\b'
                                || (line[0]&HP15_1st && line[2]=='\b') ) {
				if ( line[0]&HP15_1st) {
					line += 4;
					count += 4;
				} else {
					line += 2;
					count += 2;
				}
			}
			if ( (*line)&HP15_1st ) {
				line += 2;
				count += 2;
				lp += 2;
			} else {
				line += 1;
				count += 1;
				lp += 1;
			}
		}
		iferror(count);
	}
	if ( cp==lp ) {
		while (    line[1]=='\b'
                              	|| (line[0]&HP15_1st && line[2]=='\b') ) {
			if ( line[0]&HP15_1st) {
				line += 4;
				count += 4;
			} else {
				line += 2;
				count += 2;
			}
		}
	}
#else
	while (lp < cp) {
		switch (*line) {
		case '\0':
			*line = ' ';
			lp++;
			break;

		case '\b':
			lp--;
			break;

		default:
			lp++;
		}
		count++;
		iferror(count);
		line++;
	}
	while (*line == '\b') {
		count += 2;
		iferror(count);
		line += 2;
	}
#endif
#ifdef NLS16
	if ( cp==lp ) {
		if ( c&HP15_2nd ) {  /* Second code of HP15. */
			if (    bflag 
		     	     || (    (line[0]==' ' || line[0]=='\0')
		          	   &&(line[1]==' ' || line[1]=='\0') ) ) {
				if ( line[1]&HP15_1st ) {
					/* Avoid having counterpart of 16-bit
					** code left alone.
					*/
					line[2]=' ';
				}
				if (bflag)hp15_stk&=~UNDLINE;
				line[0]=hp15_stk;
				line[1]=c;
			} else {
                                if (line[0]&HP15_1st) {
					ibuf[0] = '\b';
					ibuf[1] = '\b';
					ibuf[2] = hp15_stk;
					ibuf[3] = c;
					insertstr(ibuf,&ibuf[4],&line[2],&count);
				} else {
					ibuf[0] = '\b';
					ibuf[1] = hp15_stk;
					ibuf[2] = c;
					ibuf[3] = '\b';
					ibuf[4] = '\b';
					ibuf[5] = ' ';
					insertstr(ibuf,&ibuf[6],&line[1],&count);
				}
				lp = 0;
				line = lbuff;
			}
			cp++;	/* Add second byte size of HP15 */
		} else {
			if (    bflag 
		     	     || ( line[0]==' ' || line[0]=='\0' ) ) {
				line[0]=c;
			} else {
				if ( line[0]&HP15_1st ) {
					ibuf[0] = '\b';
					ibuf[1] = '\b';
					ibuf[2] = c;
					ibuf[3] = ' ';
					insertstr(ibuf,&ibuf[4],&line[2],&count);
				} else {
					ibuf[0] = '\b';
					ibuf[1] = c;
					insertstr(ibuf,&ibuf[2],&line[1],&count);
				}
				lp = 0;
				line = lbuff;
			}
		}
	} else {
		if (c&HP15_2nd) {
			ibuf[0] = '\b';
			ibuf[1] = '\b';
			ibuf[2] = ' ';
			ibuf[3] = hp15_stk;
			ibuf[4] = c;
			ibuf[5] = '\b';
			ibuf[6] = '\b';
			ibuf[7] = ' ';
			insertstr(ibuf,&ibuf[8],&line[0],&count);
			lp = 0;
			line = lbuff;
			cp++;	/* Add second byte size of HP15 */
		} else {
			ibuf[0] = '\b';
			ibuf[1] = '\b';
			ibuf[2] = ' ';
			ibuf[3] = c;
			insertstr(ibuf,&ibuf[4],&line[0],&count);
			lp = 0;
			line = lbuff;
		}
	}
#else
	if (bflag || *line == '\0' || *line == ' ')
		*line = c;
	else {
		register lchar c1, c2, c3;
		count += 2;
		iferror(count);
		c1 = *++line;
		count += 2;
		iferror(count);
		*line++ = '\b';
		c2 = *line;
		count += 2;
		iferror(count);
		*line++ = c;
		while (c1) {
			c3 = *line;
			count += 2;
			iferror(count);
			*line++ = c1;
			c1 = c2;
			c2 = c3;
		}
		lp = 0;
		line = lbuff;
	}
#endif
}

store (lno)
{
	char *malloc();
	extern lchar *Strcpy();		/* NLS */

	lno %= PL;
	if (page[lno] != 0)
		free (page[lno]);
	/* NLS: now need twice as much memory */
	page[lno] = (lchar *) malloc(((unsigned)Strlen(lbuff) + 2) * 2);
	if (page[lno] == 0) {
		fprintf (stderr, (catgets(nl_fn,NL_SETN,3, "%s: no storage\n")), pgmname);
		exit (2);
	}
	Strcpy (page[lno],lbuff);
}

fetch(lno)
{
	register lchar *p;
	extern lchar *Strcpy();		/* NLS */

	lno %= PL;
	p = lbuff;
	while (*p)
		*p++ = '\0';
	line = lbuff;
	lp = 0;
	if (page[lno])
		Strcpy (line, page[lno]);
}
emit (s, lineno)
	lchar *s;
	int lineno;
{
	static int cline = 0;
	register int ncp;
	register lchar *p;
	static int gflag = 0;
	short bsfound;
	static int scnt = 0;
#ifdef NLS16
	int leftside;	/* Points the place of 8-bit or first byte of HP15 */
#endif
	
	/* CL August 23. Changes to this routine take care of the backspace
	   problem with the line printer. */

	if (*s) {
		if (gflag) {
			putchar (SI);
			gflag = 0;
		}
		while (cline < lineno - 1) {
			putchar ('\n');
			pcp = 0;
			cline += 2;
		}
		if (cline != lineno) {
			putchar (ESC);
			putchar ('9');
			cline++;
		}
		if (pcp)
			putchar ('\r');
	if (lflag) do {	
		pcp = 0;
		bsfound = 0;
		p = s;
		while (*p) {
			ncp = pcp;
			while (*p++ == ' ') {
				scnt++;
				if ((++ncp & 7) == 0 && !xflag && scnt > 1) {
					pcp = ncp;
					putchar ('\t');
				}
			}
			scnt = 0;
			if (!*--p)
				break;
			while (pcp < ncp) {
				putchar (' ');
				pcp++;
			}
#ifdef NLS16
			while (*p==PADCHAR) {
				p++;	/* Skip chars which was printed out */
			};
			leftside=pcp;
			if (gflag != (*p & GREEK)) { 
				if (gflag)
					putchar (SI);
				else
					putchar (SO);
				gflag ^= GREEK;
			}
			if ( *p & UNDLINE ) {
				/* Translate from internal kanji code */
				putchar('_');
				putchar('_');
				*p &= ~UNDLINE;
				p+=2;
				bsfound++;
			} else {
				putchar (*p & 0377);
				*p = ' ';
				pcp++;
				p++;
				if (*p&HP15_2nd) {
					putchar(*p&0377);
					*p = ' ';
					pcp++;
					p++;
				}
			}
			if (*p=='\b'){
				bsfound++;
				/* Erase printed char from buffer */
				if ( p[1]=='\b') {
					p[-2]=p[-1]
					     =p[0]=p[1]=PADCHAR;
					p += 2;
				} else {
					p[-1]=p[0]=PADCHAR;
					p += 1;
				}
				/* Skip to next char to be printed */
				while (    leftside < pcp 
					&& p[1]!=0 
					&& !(p[0]&HP15_1st&&(p[2]==0)) ) {
					while (  p[1]=='\b'
					       ||((p[0]&HP15_1st)&&p[2]=='\b')){
						if (p[2]=='\b') {
							p += 4;
						} else {
							p += 2;
						}
					}
					if (p[0]&HP15_1st){
						leftside += 2;
						p += 2;
					} else {
						leftside += 1;
						p += 1;
					}
					while (*p==PADCHAR) {
						/* Skip chars which was 
						** printed out.
						*/
						p++;
					}
				}
				if ( !( p[1]!=0 
					&& !(p[0]&HP15_1st&&(p[2]==0)) ) ) {
					/* To the end of buffer */
					if (p[0]&HP15_1st) {
						p+=2;
					} else {
						p++;
					}
				}
			}
#else
			if (*p != 255) {
				if (*p == '\b') {
					p[-1] = *p = 255;
					p += 2;
					while (*p == '\b')
						p +=2;
					bsfound++;
				}
				else {
					if (gflag != (*p & GREEK)) { 
						if (gflag)
							putchar (SI);
						else
							putchar (SO);
						gflag ^= GREEK;
					}
					pcp++;
					putchar (*p & 0377);
					*p++ = ' ';
				}
			}
			else
				p++;
#endif
		}
#ifdef NLS16
			if (bsfound) {
				for(p=s;*p==' '||*p==PADCHAR;p++);
				if (*p=='\0'){
					/* Cut off redundant blank strings which
					** come out due to 16-bit code support.
					*/
					bsfound = 0;
				} else {
		   			putchar('\r');
				}
			}
#else
			if (bsfound)
		   		putchar('\r');
#endif
		}while (bsfound);
		else {
		pcp = 0;
		p = s;
		while (*p) {
			ncp = pcp;
			while (*p++ == ' ') {
				scnt++;
				if ((++ncp & 7) == 0 && !xflag) {
					pcp = ncp;
					if ( scnt > 1 ) {
						putchar ('\t');
					} else {
						putchar (' ');
					}
				}
			}
			scnt = 0;
			if (!*--p)
				break;
			while (pcp < ncp) {
				putchar (' ');
				pcp++;
			}
			if (gflag != (*p & GREEK) && *p != '\b') {
				if (gflag)
					putchar (SI);
				else
					putchar (SO);
				gflag ^= GREEK;
			}
#ifdef NLS16
			if ( *p & UNDLINE ) {
				/* Translate from internal kanji code into */
				/* __BSBSXX(XX is kanji).		   */
				putchar('_');
				putchar('_');
				putchar('\b');
				putchar('\b');
			}
			putchar (*p & 0377);
#else
			putchar (*p & 0377);
#endif
			if (*p++ == '\b')
				pcp--;
			else
				pcp++;
		}
		}
			
	}
}

incr()
{
	store (ll++);
	if (ll > llh)
		llh = ll;
	if (ll >= mustwr && page[ll%PL]) {
		emit (page[ll%PL], ll - PL);
		mustwr++;
		free (page[ll%PL]);
		page[ll%PL] = 0;
	}
	fetch (ll);
}

decr()
{
	if (ll > mustwr - PL) {
		store (ll--);
		fetch (ll);
	}
}

iferror (c)
int c;
{

	if (c >= LINELN) {
	   fprintf(stderr,(catgets(nl_fn,NL_SETN,4, "col: Input line is TOO long!")));
	   exit(2);
	}
}

lchar *Strcpy(s, t)	/* NLS: copies string of lchars */
lchar *s, *t;
{
	while (*s++ = *t++)
		;
}

Strlen (s)		/* NLS: length of string of lchars */
lchar *s;
{
	lchar *p = s;
	while (*p) p++;
	return (p-s);
}

#ifdef NLS16
/* Insertstr() inserts the string of which head is pointed by "bp"
** and of which tail is pointed by "ep before the string specified
** by "buf". The contents of buf is shifed to right.
** The ternimator of buf is Null.
*/
insertstr(bp,ep,buf,count)
lchar *bp,*ep,*buf;
int *count;
{
	lchar inbuf[10];
	register lchar *p;
	register int i,strlen;
	strlen = (int)(ep-bp);
	*count+=strlen;
	iferror(*count);
	for(i=0;i<strlen;i++){
		inbuf[i]=buf[i];
		buf[i]=bp[i];
	}
	buf += strlen;
	while (inbuf[0]) {
		inbuf[strlen] = *buf;
		*count += 1;
		iferror(*count);
		*buf++ = inbuf[0];
		for(i=0;i<strlen;i++){
			inbuf[i]=inbuf[i+1];
		}
	}
}

/* Getchar() sets 16-bit code flag.
** HP15_1st is set when first byte of HP15 comes out.
** And HP15_2nd is set when first byte of HP15 comes out.
** When illegal HP15 series is found out, it was considered
** just as 8-bit code series.
** And Getchar() has stack. It can be set by Setchar() upto 5 integer.
*/
int hp15_2nd=0; 	/* Set when first byte of HP15 is got. */
int illegal_code=0;	/* Set when illegal HP15 is detacted.  */
int buf[5];
int *bufp = buf;
void Setchar( c ) 
int c;
{
	*bufp = c;
	bufp++;
}
int Getchar(){
	int ch,ch2;
	int	i;

	if ( buf!=bufp ) {
		bufp--;
		return(*bufp);
	}

	if ( (ch=getchar())==EOF ) {
		return(EOF);
	};
	if ( illegal_code ) {
		illegal_code=0;
	} else if ( hp15_2nd ) {
		hp15_2nd=0;
		ch |= HP15_2nd;
	} else {
		if ( FIRSTof2(ch&0377) ) {
			ch2 = getchar();
			ungetc(ch2, stdin);
			if ( SECONDof2(ch2&0377) ) {
				hp15_2nd = 1;
				ch |= HP15_1st;
			} else {
				illegal_code = 1;
			}
		}
	}
	return(ch);
}
#endif
