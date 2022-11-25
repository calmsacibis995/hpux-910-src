static char *HPUX_ID = "@(#) $Revision: 64.1 $";

#include <stdio.h>
#include <ctype.h>
#include <nl_types.h>

#ifdef NLS16		/* 16 bit deroff */
#include <nl_ctype.h>
typedef unsigned short tchar;

/*
*    The following macros are ignorant of "ctype" -- they make assumptions
*    similar to those of the initial 16 bit "vi", that any 16 bit 
*    character is a "text" character and that only ASCII 0-9 are digits.
*/

#define IS_APOS(c)     ((c)<=0xff ? (chars[(c)]==APOS)    : 0)
#define IS_DIGIT(c)    ((c)<=0xff ? (chars[(c)]==DIGIT)   : 0)
#define IS_TEXT(c)     ((c)<=0xff ? (chars[(c)]==LETTER)  : 1)
#define IS_SPECIAL(c)  ((c)<=0xff ? (chars[(c)]==SPECIAL) : 0)

#else			/* 8 bit deroff */

typedef unsigned char tchar;

#define CHARAT(p)	(*p)
#define CHARADV(p)	(*p++)
#define ADVANCE(p)	(++p)
#define PCHAR(c,p)	(*p = c)
#define PCHARADV(c,p)	(*p++ = c)
#define t_atoi(a)	atoi(a)
#define t_getc(a)	getc(a)
#define t_ungetc(a,b)	ungetc(a,b)
#define t_putc(a,b)	putc(a,b)
#define t_puts(s)	puts(s)

#define IS_APOS(c)     (chars[(c)]==APOS)
#define IS_DIGIT(c)    (chars[(c)]==DIGIT)
#define IS_TEXT(c)     (chars[(c)]==LETTER)
#define IS_SPECIAL(c)  (chars[(c)]==SPECIAL)

#endif

#ifdef NLS
#include <locale.h>
#endif NLS

/* Deroff command -- strip troff, eqn, and Tbl sequences from
a file.  Has three flags argument, -w, to cause output one word per line
rather than in the original format.
-mm (or -ms) causes the corresponding macro's to be interpreted
so that just sentences are output
-ml  also gets rid of lists.
-i causes deroff to ignore .so and .nx commands.
Deroff follows .so and .nx commands, removes contents of macro
definitions, equations (both .EQ ... .EN and $...$),
Tbl command sequences, and Troff backslash constructions.

All input is through the C macro; the most recently read character is in c.
*/

#define NL_SETN 1

#define C ((c=t_getc(infile)) == EOF ? eof() : ((c==ldelim)&&(filesp==files) ? skeqn() : c) )
#define C1 ((c=t_getc(infile)) == EOF ? eof() : c)
#define SKIP while(C != '\n') 
#define SKIP_TO_COM SKIP; SKIP; pc=c; while(C != '.' || pc != '\n' || C > 'Z')pc=c

#define YES 1
#define NO 0
#define MS 0
#define MM 1
#define ONE 1
#define TWO 2

#define NOCHAR -2
#define SPECIAL 0
#define APOS 1
#define DIGIT 2
#define LETTER 3

#define LBSIZE	BUFSIZ		/* line buffer size */

int wordflag = NO;
int msflag = NO;
int iflag= NO;
int mac = MM;
int disp = 0;
int inmacro = NO;
int intable = NO;

tchar chars[256];  /* SPECIAL, APOS, DIGIT, or LETTER */

tchar line[LBSIZE];
tchar *lp;

int c;
int pc;
int ldelim	= NOCHAR;
int rdelim	= NOCHAR;

int argc;
char **argv;

extern int optind;
extern char*optarg;
char fname[50];	  /* contains raw multi-byte chars, not fixed-size tchars */
FILE *files[15];
FILE **filesp;
FILE *infile;
nl_catd catd;

main(ac, av)
int ac;
char **av;
{
	register int i;
	int errflg = 0;
	register optchar;
	FILE *opn();

#if defined NLS || defined NLS16	/* initialize to the right language */
	if (!setlocale(LC_ALL,"")) {
		fputs(_errlocale(),stderr);
		catd=(nl_catd)-1;
	}
	else
        	catd = catopen("deroff",0);
#endif NLS || NLS16

	argc = ac;
	argv = av;
	while ((optchar = getopt(argc, argv, "wim:")) != EOF) switch(optchar) {
		case 'w':
			wordflag = YES;
			break;
		case 'm':
			msflag = YES;
			if (CHARAT(optarg) == 'm')
				mac = MM;
			else if (CHARAT(optarg) == 's')
				mac = MS;
			else if (CHARAT(optarg) == 'l')
				disp = 1;
			else errflg++;
			break;
		case 'i':
			iflag = YES;
			break;
		case '?':
			errflg++;
		}
	if (errflg) {
            fprintf(stderr,(catgets(catd,NL_SETN,1,"Deroff: usage: deroff [ -w ] [ -m (m s l) ] [ -i ] [ file ] ... \n")));
	    exit(1);
        }
	if ( optind == argc )
		infile = stdin;
	else
		infile = opn(argv[optind++]);
	files[0] = infile;
	filesp = &files[0];

	for (i=0; i<0x100; i++) {	/* create a ctype variant */
	    if (isalpha(i))
		chars[i] = LETTER;
	    else if (isdigit(i))
		chars[i] = DIGIT;
	    else if (i=='\'' || i=='&')
		chars[i] = APOS;
	    else
		chars[i] = SPECIAL;
	}

	work();
}
char *calloc();






skeqn()
{
while((c = t_getc(infile)) != rdelim)
	if(c == EOF)
		c = eof();
	else if(c == '"')
		while( (c = t_getc(infile)) != '"')
			if(c == EOF)
				c = eof();
			else if(c == '\\')
				if((c = t_getc(infile)) == EOF)
					c = eof();
if(msflag)return(c='x');
return(c = ' ');
}


FILE *opn(p)
register char *p;
{
FILE *fd;

if( (fd = fopen(p, "r")) == NULL) {
    fprintf(stderr,(catgets(catd,NL_SETN,2,"Deroff: Cannot open file %s\n")),p);
    exit(1);
}

return(fd);
}



eof()
{
if(infile != stdin)
	fclose(infile);
if(filesp > files)
	infile = *--filesp;
else if(optind < argc)
	{
	infile = opn(argv[optind++]);
	}
else
	exit(0);

return(C);
}





getfname()
{
register char *p;
struct chain { struct chain *nextp; char *datap; } *chainblock;
register struct chain *q;
static struct chain *namechain	= NULL;
char *copys();

while(C == ' ') ;

p=fname;
for (;;) {
    PCHAR(c,p);
    if (c!= '\n' && c!=' ' && c!='\t' && c!='\\' ) {
	C;
	ADVANCE(p);
    }
    else
	break;
}

*p = '\0';
while(c != '\n')
	C;

/* see if this name has already been used */

for(q = namechain ; q; q = q->nextp)
	if( ! strcmp(fname, q->datap))
		{
		fname[0] = '\0';
		return;
		}
q = (struct chain *) calloc(1, sizeof(*chainblock));
q->nextp = namechain;
q->datap = copys(fname);
namechain = q;
}

work()
{
for( ;; )
	{
	if(C == '.'  ||  c == '\'')
		comline();
	else
		regline(NO,TWO);
	}
}




regline(macline,aconst)
int macline;
int aconst;
{
line[0] = c;
lp = line;
for( ; ; )
	{
	if(c == '\\')
		{
		*lp = ' ';
		backsl();
		if (c == '%') lp--;		/* no blank for hyphenation char */
		}
	if(c == '\n') break;
	if ((lp - line) == (LBSIZE - 1))
		{
    		fprintf(stderr,(catgets(catd,NL_SETN,4,"Deroff: Input buffer overflow\n")));
    		exit(1);
		}
	if(intable && c=='T')
		{
		*++lp = C;
		if(c=='{' || c=='}')
			{
			lp[-1] = ' ';
			*lp = C;
			}
		}
	else	*++lp = C;
	}

*lp = '\0';

if(line[0] != '\0')
	if(wordflag)
		putwords(macline);
	else if(macline)
		putmac(line,aconst);
	else
		t_puts(line);
}




putmac(s,aconst)
register tchar *s;
int aconst;
{
register tchar *t;

while(*s)
	{
	while(*s==' ' || *s=='\t')
		t_putc(*s++, stdout);
	for(t = s ; *t!=' ' && *t!='\t' && *t!='\0' ; ++t)
		;
	if(*s == '\"')s++;

	if(t>s+aconst && (IS_TEXT(s[0]) || s[0]=='\'') && IS_TEXT(s[1]))
		while(s < t)
			if(*s == '\"')s++;
			else
				t_putc(*s++, stdout);
	else
		s = t;
	}
putchar('\n');
}



putwords(macline)	/* break into words for -w option */
int macline;
{
register tchar *p, *p1;
int nlet;


for(p1 = line ; ;)
	{
	/* skip initial specials ampersands and apostrophes */
	while(!IS_TEXT(*p1) && !IS_DIGIT(*p1))
		if(*p1++ == '\0') return;
	nlet = 0;
	for(p = p1 ; !IS_SPECIAL(*p); ++p)
		if(IS_TEXT(*p)) ++nlet;

	if( (!macline && nlet>1)   /* MDM definition of word */
	   || (macline && nlet>2 && IS_TEXT(p1[0]) && IS_TEXT(p1[1])))
		{
		/* delete trailing ampersands and apostrophes */
		while(IS_APOS(p[-1]))
			 --p;
		while(p1 < p) t_putc(*p1++, stdout);
		putchar('\n');
		}
	else
		p1 = p;
	}
}



comline()
{
register int c1, c2;

com:
while(C==' ' || c=='\t')
	;
comx:
if( (c1=c) == '\n')
	return;
c2 = C;
if(c1=='.' && c2!='.')
	inmacro = NO;
if(c2 == '\n')
	return;

if(c1=='E' && c2=='Q' && filesp==files)
	eqn();
else if(c1=='T' && (c2=='S' || c2=='C' || c2=='&') && filesp==files){
	if(msflag){ stbl(); }
	else tbl(); }
else if(c1=='T' && c2=='E')
	intable = NO;
else if(!inmacro && c1=='d' && c2=='e')
	macro();
else if(!inmacro && c1=='i' && c2=='g')
	macro();
else if(!inmacro && c1=='a' && c2 == 'm')
	macro();
else if(c1=='s' && c2=='o')
	if(iflag)
		{ SKIP; }
	else
		{
		getfname();
		if( fname[0] )
			infile = *++filesp = opn( fname );
		}
else if(c1=='n' && c2=='x')
	if(iflag)
		{ SKIP; }
	else
		{
		getfname();
		if(fname[0] == '\0') exit(0);
		if(infile != stdin)
			fclose(infile);
		infile = *filesp = opn(fname);
		}
else if(c1=='h' && c2=='w')
	{ SKIP; }
else if(msflag && c1 == 'T' && c2 == 'L'){
	SKIP_TO_COM;
	goto comx; }
else if(msflag && c1=='N' && c2 == 'R')SKIP;
else if(msflag && c1 == 'A' && (c2 == 'U' || c2 == 'I')){
	if(mac==MM)SKIP;
	else {
		SKIP_TO_COM;
		goto comx; }
	}
else if(msflag && c1 == 'F' && c2 == 'S'){
	SKIP_TO_COM;
	goto comx; }
else if(msflag && (c1 == 'S' || c1 == 'P') && c2 == 'H'){
	SKIP_TO_COM;
	goto comx; }
else if(msflag && c1 == 'N' && c2 == 'H'){
	SKIP_TO_COM;
	goto comx; }
else if(msflag && c1 == 'O' && c2 == 'K'){
	SKIP_TO_COM;
	goto comx; }
else if(msflag && c1 == 'N' && c2 == 'D')
	SKIP;
else if(msflag && mac==MM && c1=='H' && (c2==' '||c2=='U'))
	SKIP;
else if(msflag && mac==MM && c2=='L'){
	if(disp || c1 == 'R')sdis('L','E');
	else{
		SKIP;
		putchar('.');
		}
	}
else if(msflag && (c1 == 'D' || c1 == 'N' || c1 == 'K'|| c1 == 'P') && c2 == 'S')
	{ sdis(c1,'E'); }		/* removed RS-RE */
else if(msflag && (c1 == 'K' && c2 == 'F'))
	{ sdis(c1,'E'); }
else if(msflag && c1 == 'n' && c2 == 'f')
	sdis('f','i');
else if(msflag && c1 == 'c' && c2 == 'e')
	sce();
else
	{
	if(c1=='.' && c2=='.')
		while(C == '.')
			;
	++inmacro;
	if(c1 <= 'Z' && msflag)regline(YES,ONE);
	else regline(YES,TWO);
	--inmacro;
	}
}



macro()
{
if(msflag){
	do { SKIP; }
		while(C!='.' || C!='.' || C=='.');	/* look for  .. */
	if(c != '\n')SKIP;
	return;
}
SKIP;
inmacro = YES;
}




sdis(a1,a2)
tchar a1,a2;
{
	register int c1,c2;
	register int eqnf;
	eqnf=1;
	SKIP;
	while(1){
		while(C != '.')SKIP;
		if((c1=C) == '\n')continue;
		if((c2=C) == '\n')continue;
		if(c1==a1 && c2 == a2){
			SKIP;
			if(eqnf)putchar('.');
			putchar('\n');
			return;
		}
		else if(a1 == 'D' && c1 == 'E' && c2 == 'Q'){eqn(); eqnf=0;}
		else SKIP;
	}
}
tbl()
{
while(C != '.');
SKIP;
intable = YES;
}
stbl()
{
while(C != '.');
SKIP_TO_COM;
if(c != 'T' || C != 'E'){
	SKIP;
	pc=c;
	while(C != '.' || pc != '\n' || C != 'T' || C != 'E')pc=c;
}
}

eqn()
{
register int c1, c2;
register int dflg;
int last;

last=0;
dflg = 1;
SKIP;

for( ;;)
	{
	if(C1 == '.'  || c == '\'')
		{
		while(C1==' ' || c=='\t')
			;
		if(c=='E' && C1=='N')
			{
			SKIP;
			if(msflag && dflg){
				putchar('x');
				putchar(' ');
				if(last){putchar('.'); putchar(' '); }
			}
			return;
			}
		}
	else if(c == 'd')	/* look for delim */
		{
		if(C1=='e' && C1=='l')
		    if( C1=='i' && C1=='m')
			{
			while(C1 == ' ');
			if((c1=c)=='\n' || (c2=C1)=='\n'
			    || (c1=='o' && c2=='f' && C1=='f') )
				{
				ldelim = NOCHAR;
				rdelim = NOCHAR;
				}
			else	{
				ldelim = c1;
				rdelim = c2;
				}
			}
			dflg = 0;
		}

	if(c != '\n') while(C1 != '\n'){ if(c == '.')last=1; else last=0; }
	}
}



backsl()	/* skip over a complete backslash construction */
{
int bdelim;

sw:  switch(C)
	{
	case '"':
		SKIP;
		return;
	case 's':
		if(C == '\\') backsl();
		else	{
			while(C>='0' && c<='9') ;
			t_ungetc(c,infile);
			c = '0';
			}
		--lp;
		return;

	case 'f':
	case 'n':
	case '*':
		if(C != '(')
			return;

	case '(':
		if(C != '\n') C;
		return;

	case '$':
		C;	/* discard argument number */
		return;

	case 'b':
	case 'x':
	case 'v':
	case 'h':
	case 'w':
	case 'o':
	case 'l':
	case 'L':
		if( (bdelim=C) == '\n')
			return;
		while(C!='\n' && c!=bdelim)
			if(c == '\\') backsl();
		return;

	case '\\':
		if(inmacro)
			goto sw;
	default:
		return;
	}
}




char *copys(s)
register char *s;
{
register char *t, *t0;

if( (t0 = t = calloc( (unsigned)(strlen(s)+1), sizeof(*t) ) ) == NULL) {
    fprintf(stderr,(catgets(catd,NL_SETN,3,"Deroff: Cannot allocate memory\n")));
    exit(1);
}

while( *t++ = *s++ )
	;
return(t0);
}
sce(){
register tchar *ap;
register int n, i;
tchar a[10];
	for(ap=a;C != '\n';ap++){
		*ap = c;
		if(ap == &a[9]){
			SKIP;
			ap=a;
			break;
		}
	}
	if(ap != a)n = atoi(a);
	else n = 1;
	for(i=0;i<n;){
		if(C == '.'){
			if(C == 'c'){
				if(C == 'e'){
					while(C == ' ');
					if(c == '0')break;
					else SKIP;
				}
				else SKIP;
			}
			else SKIP;
		}
		else {
			SKIP;
			i++;
		}
	}
}

#ifdef NLS16

/*************** Tools That May Eventually Be In libc.a **************/

/*
    WARNING:    the "ungetc" mechanism used here is not general-purpose;
	        it provides just a single character of "unget", and
		is intended for "deroff" use only.
*/

#define FALSE	0
#define TRUE	1

static int ungot = FALSE;
static tchar ungot_char;

t_ungetc(c, fp)
register tchar c;
register FILE *fp;
{
    ungot_char = c;
    ungot = TRUE;
}

t_getc(fp)
register FILE *fp;
{
    register int char1;
    register int char2;

    if (ungot) {
	ungot = FALSE;
	return(ungot_char);
    }

    char1 = getc(fp);

    if ((char1 == EOF) || !FIRSTof2(char1))
	return(char1);

    if ((char2 = getc(fp)) == EOF)
        return(char1);

    return((char1 << 8) + char2);
}


t_putc(c, fp)
register int c;
register FILE *fp;
{
    register int char1;
    register char *result;

    if (char1 = c & 0xff00) {
	char1 >>= 8;
	if ((result = putc(char1, fp)) == EOF)
	    return(EOF);
    }
    if ((result = putc(c&0xff, fp)) == EOF) {
	if (char1)
	    return(char1);
	else 
	    return(EOF);
    }
    else
	return(c&0xffff);
}

t_puts(s)
register tchar *s;
{
    while (*s)
	t_putc(*s++, stdout);
    putchar('\n');
}

t_atoi(s)
register tchar *s;
{
    int negative = FALSE;
    int result = 0;
	
    if (*s == '-') {
	++s;
	negative = TRUE;
    }
    while (*s && isascii(*s) && isdigit(*s)) {
	result *= 10;
	result += (*s++ - '0');
    }
    if (negative)
	result *= -1;
    return(result);
}

#endif
