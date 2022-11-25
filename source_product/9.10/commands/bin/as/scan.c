/* @(#) $Revision: 70.1 $ */   

/* Lexical analyzer */

# include <stdio.h>

# include "symbols.h"
# include "adrmode.h"
# include "asgram.h"

# include <cvtnum.h>

# define	TXTCHARS	256
# define CSSZ	256

# ifdef TRACE
# define TRACETOKEN(t)	printf("%s=<%s>\n",t,yytext)
  char tracebuf[TXTCHARS+2];
# else
# define TRACETOKEN(t)
# endif

# ifdef LISTER1
# define ECHOMAX  100
char echobuf[ECHOMAX+2];
static char * echopt = &echobuf[0];
static char * echoend = &echobuf[ECHOMAX-1];
static int inputchar;
extern long newdot;
# endif
# if defined(LISTER2) || defined(LISTER1)
long listdot = 0;
extern short listflag;
#endif

int fp_cpid = 1;
int fp_round = C_NEAR;
int fp_size = SZNULL;
extern int cvtnum();

node nodes[1000];
int maxnode = 1000;
int inode = 0;

extern unsigned line; /* current input file line number */
int fpsize = 0;


extern hashed_symbol *lookup(); /* symbol table access routine */

extern FILE * fdin;

YYSTYPE yylval, yyval;

char	yytext[TXTCHARS+2];

short type0[CSSZ+1] = {
	EOF,
	SP,	0,	0,	0,	0,	0,	0,	0,
	0,	SP,	NL,	SP,	SP,	SP,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
	SP,	BANG,	DQ,	SH,	ALPH,	REGC,	AND,	SQ,
	LP,	RP,	MUL,	PLUS,	COMMA,	MINUS,	DOT,	DIV,
	DIG,	DIG,	DIG,	DIG,	DIG,	DIG,	DIG,	DIG,
	DIG,	DIG,	COLON,	SEMI,	LSH,	0,	RSH,	0,
	MOD,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,
	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,
	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,
	ALPH,	ALPH,	ALPH,	LB,	0,	RB,	XOR,	ALPH,
	0,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,
	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,
	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,	ALPH,
	ALPH,	ALPH,	ALPH,	LCB,	IOR,	RCB,	TILDE,	0,
	/* implicit zeroes for the 128 chars with hibit set */
};
short * type = &type0[1];

/* These macros are currently trivial  ( ie, directly translate to their
 * stdio counterpart.  However, using these macros in the code, provides
 * for flexibility in handling the input (to do own buffering, save input
 * line for a listing, etc. ).
 */
/* DEC. 1985. I've experimentally rewritten UNGETC() as an inline macro
 * for tuning. Because the scanner calls UNGETC at least once for every
 * token, the overhead of a true function call is high.  However, this
 * is questionable code: very susceptible to changes in the stdio
 * implementation.
 * *** ??? Should this really be restructured to read a line at a time???
 */

# ifndef LISTER1
# define GETCHAR()	getc(fdin)
/*# define UNGETC(x)	ungetc(x, fdin)
/*# define UNGETC(x)	{ if(fdin->_ptr==fdin->_base && fdin->_cnt==0)\
/*			    *fdin->_ptr = x; else *--fdin->_ptr = x;\
/*			    ++fdin->_cnt; }
*/
# define UNGETC(x)	{ *--fdin->_ptr = x; ++fdin->_cnt; }
# else
  /* the "inputchar" is needed to avoid an unwanted conversion from
   * integer to character of the value returned by getc.
   */
# define GETCHAR()	(echopt < echoend ? (inputchar=getc(fdin),\
	*echopt++ =inputchar, inputchar) : getc(fdin))
# define UNGETC(x)	(echopt--, ungetc(x, fdin))
/*# define UNGETC(x)	{ echopt--; if(fdin->_ptr==fdin->_base && fdin->_cnt==0)\
/*			    *fdin->_ptr = x; else *--fdin->_ptr = x;\
/*			    ++fdin->_cnt; }
*/
# endif


/* character classes to aid lexical scans for names and different kinds
 * of numbers. These lex-definitions and routines are taken from the
 * pcc.
 */
# define LEXALPH   0x0001
# define LEXDIG    0x0002
# define LEXOCT    0x0004
# define LEXHEX    0x0008
# define LEXWS     0x0010
# define LEXDOT    0x0020
# define LEXPAREN  0x0040

short lxmask[CSSZ+1];
static char * lxgcp;

lxenter( s, m )
	register unsigned char *s;
	register short m;
{

	/* enter a mask into lxmask for each character in the string s */ 
	register c;

	while ( c = *s++ ) lxmask[c+1] |= m;

}

# define lxget(c,m)  (lxgcp=yytext,lxmore(c,m))

/* lxmore:  read characters from the stdin and append to yytext as long as
 * the characters are in one of the LEX-types indicated by the mask "m".
 * A terminating null character is appended to yytext after the last
 * character matched.  The global pointer "lxgcp" is left pointing to this
 * null character, so that it would be overwritten by another lxmore call.
 * If (c!=0) it is appended to yytext before the scan begins.
 * The return value indicates how many characters lxmore matched.
 */
int
lxmore(c,m)
	register c;
	register short m;
{
	register char *cp;
	register int cnt;
	int warned = 0;		/* so only warn once about token too long */

	cnt = 0;
	cp = lxgcp;
	if (c) *cp++ = c;
	while( c = GETCHAR(), lxmask[c+1]&m ) {
	   /* 
	    * symbols can only be 255 chars long, not 256.
	    * An unsigned char is used to store the length
	    * of symbols in the LST, so 256 is zero!
	    */
	   if (cp < &yytext[TXTCHARS-1] ) {
		*cp++ = c;
		cnt++;
		}
	   else {
		if (!warned)
		   werror("token too long, truncated");
		warned = 1;
		}
	   }
	UNGETC(c);
	*(lxgcp = cp) = '\0';
	return(cnt);

}


lxinit() {

	/* set up character classes */
	lxenter("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$",LEXALPH);
	lxenter("0123456789", LEXDIG);
	lxenter("0123456789abcdefABCDEF", LEXHEX);
	lxenter("01234567", LEXOCT);
	lxenter(" \t\r\b\f\013\014", LEXWS);
	lxenter(".", LEXDOT);
	lxenter("()",LEXPAREN);

}


yylex()
{
	register long value;
	register short c;
	register short ctype;
	char fltchr[64];
	upsymins	id;
	static BYTE instseen = 0; /* instruction seen on line */
	static int endtoksent = 0;

	c = GETCHAR();
	while (type[c] == SP) c = GETCHAR();

	ctype = type[c];
	value = c;

	switch (ctype) {
			case REGC:
				/* check for a register symbol */
				lxget(c, LEXALPH|LEXDIG);
				TRACETOKEN("REGSYM");
				id.hsp = lookup(yytext,N_INSTALL,REGSYM);
				if (id.hsp == NULL || id.hsp->tag != REGSYM) {
					uerror("invalid register symbol (%s)",
					  yytext);
					return(ERROR);
					}
				yylval.unreg.regno = id.rsp->regno;
				yylval.unreg.regkind = id.rsp->rtype;
				return(id.rsp->rtype);
				break;
				
			case ALPH: /* some kind of alpha-numeric string */
			case DOT:
			case BANG: /* just for Fortran */
				lxget(c, LEXALPH|LEXDIG);
				TRACETOKEN("IDENT");

				/* look to see if it is a label */
				/* labels must occur before opcode -- need this
				 * semantic knowledge and lookahead to
				 * distinguish labels from instruction mnemonic
				 * with same name.
				 */
				if (!instseen) {
				   value = GETCHAR();
				   while (value == ' ' || value == '\t')
					value = GETCHAR();
				   UNGETC(value); /* throw it back */
				   if (value == ':') { /* it's a label */
					id.hsp = lookup(yytext,INSTALL,USRNAME);
					if (id.hsp == NULL || id.hsp->tag != USRNAME) {
					   uerror("invalid label symbol (%s)",
					     yytext);
					   return(ERROR);
					   }
					yylval.unsymptr = id.stp;
					return(IDENT);
					}
				   }

				/* look to see if it is a special symbol
				   or an instruction mnemonic */
				if (!instseen) { /* look for an instruction */
					instseen = 1;
					id.hsp = lookup(yytext,N_INSTALL,MNEMON);
					if (id.itp==NULL || id.itp->itag==0) {
						uerror("invalid instruction mnemonic (%s)",
						  yytext);
						return(ERROR);
						};
					/* it is some kind of instruction */
					yylval.uninsptr = id.itp;
					return(id.itp->iopclass);
					}
				else { /* some kind of operand ?? */

				       /* is it a reserved symbol? (eg, size
					* suffix ?
					* These start with DOT */
				       if (ctype == DOT) {
					   id.hsp = lookup(yytext,N_INSTALL,SPSYM);
					   if (id.hsp!=NULL&&id.hsp->tag==SPSYM){
						yylval.unlong = id.stp->svalue;
						return(id.stp->stype);
						}
					   }
					/* it is just a user's symbol */
					id.hsp = lookup(yytext,INSTALL,USRNAME);
					yylval.unsymptr = id.stp;
					return(IDENT);
					};


			case DIG: /* some kind of number */
			      { register short base;
				register char *cptr;
				if (value == '0') {
				   value = GETCHAR();
				   switch(value) {
					case 'x':
					case 'X':
						/* hex constant */
						TRACETOKEN("hex constant");
						base = 16;
						lxget(0,LEXHEX);
						if (strlen(yytext) > 8) {
						    if (hextofloat() == 0) {
						        return(FLTNUM);
						    }
						    else {
						        uerror("illegal hex floating point constant");
						        return(ERROR);
						    }
						}
						goto iconvert;

					case 'f': case 'F':
					case 'd': case 'D':
						/* floating point constant */
						TRACETOKEN("float constant");
						if (read_float_string()==0 &&
						    convert_float_string()==0) {
						   return(FLTNUM);
						   }
						else {
						   uerror("illegal floating point constant");
						   return(ERROR);
						   }
						




					default:
						/* octal constant */
						TRACETOKEN("octal constant");
						UNGETC(value);
						base = 8;
						lxget('0', LEXOCT);
						goto iconvert;

					}
				   }
				else {
				   /* decimal constant */
				   base = 10;
				   TRACETOKEN("decimal constant");
				   lxget(value, LEXDIG);
				   goto iconvert;
				   }

				iconvert:
				value = 0;
				cptr = yytext;
				while (c = *cptr++) {
				   if (c >= '0' && c <= '9') c -= '0';
				   else if (c >= 'a' && c <= 'f') c = c - 'a' + 10;
				   else if (c >= 'A' && c <= 'F') c = c - 'A' + 10;
				   else uerror("bad digit in integral constant");
				   value *= base;
				   value += c;
				   }
				yylval.unlong = value;
				TRACETOKEN((sprintf(tracebuf,"integral constant = %x",value),tracebuf));
				return(INTNUM);
			     }

			case SQ: /* '? --> return the integer value of ? */
				lxstr(0, 0);
				return(INTNUM);

			case DQ: /* this is the beginning of a string "..." */
				lxstr(1, '"');
				return(STRING);

			case EOF: /* no more input, that's all folks */
				if (endtoksent)
					return(EOF);
				else {
				   endtoksent = 1;
				   return(ENDTOKEN);
				   }

			case SH: /* # comment to end of line */
				c = GETCHAR();
				while (c != '\n') 
					c = GETCHAR();
				instseen = 0;
				return(NL);

			case NL: /* new line */
				instseen = 0;
				return(NL);

			case SEMI: /* logical end of statement */
				instseen = 0;
				return(SEMI);

			case 0: /* ????? */
				uerror("illegal input character (0x%2x)", c);
				return(ERROR);

			default: /* if all else fails, return token */
				yylval.unlong = c;
				return(ctype);
			};
}


int read_float_string() {
	/* read a floating point string into yytext */
	/* returns non-zero if error detected. */
	int n;
	register int c;

	/* allow a leading sign */
	if ((c = GETCHAR())!='-' && c!='+') {
		UNGETC(c);
		c = 0;
		}

	/* check for "specials" which will be letter-strings ( "nan", "inf")
	 * here we look for any letter string and let "cvtnum" check
	 * for validity.
	 */
	n = lxget(c, LEXALPH);
	if (n>0) {
		/* assume it's some kind of special.  If it's a NAN there
		 * should also be 
		 *	( HEXDIGITS )
		 */
		n += lxmore(0, LEXPAREN|LEXHEX);
		return(0);
		}

	/* else a "normal" number */
	n = lxget(c,LEXDIG);
	if ( (c=GETCHAR())=='.') {
	   n += lxmore('.',LEXDIG);
	   }
	else
	   UNGETC(c);
	if (n==0) {
	   return(1);
	   }

	if ( (c=GETCHAR())=='e' || c=='E') {  /* exponent */
	   if ( (c=GETCHAR())=='-' || c=='+') {
		*lxgcp++ = 'e';
		}
	   else {
		UNGETC(c);
		c = 'e';
		}
	   lxmore(c, LEXDIG);
	   }
	else {  /* no exponent */
	   UNGETC(c);
	   }

	return(0);

}

expvalue  fltexp;

/* convert the string in yytext[] to a floating point number */
convert_float_string()
{ int type;
  char * endptr;
  int inexact;
  int cvtret;

  /* set the type according to the current value of fp_size */
  switch(fp_size) {
	default:
		uerror("illegal or no size for float constant");
		type = C_SNGL;
		break;
	case SZSINGLE:
		type = C_SNGL;
		break;
	case SZDOUBLE:
		type = C_DBLE;
		break;
	case SZEXTEND:
		type = C_EXT;
		break;
	case SZPACKED:
		type = C_DPACK;
		break;
	}

  cvtret = cvtnum(yytext, &fltexp, type, fp_round, &endptr, &inexact);
  yylval.unfloat = &fltexp;
  if (cvtret == C_BADCHAR || *endptr != '\0')
	return(1);
  else  return(0);

}

/* we have a hex constant with more than 8 hex digits. It does not
 * fit in a long int, so it must be a double float constant in hex
 * format
 */
hextofloat()
{
    int digits;
    int maxdigits;
    int zeros;
    int i,j;
    char *p;
    char c;
    int d;

    digits = strlen(yytext);

    if ((digits < 9) || (digits > 24)) {
        return(1);
    }

    if (digits > 16) {
	maxdigits = 24;
    }
    else {
	maxdigits = 16;
    }

    zeros = maxdigits - digits;

    p = (char *) &fltexp;

    for (i = 0, j = 0; i < maxdigits; i++) {
       if (i < zeros) {
	  d = 0;
       }
       else {
	  c = yytext[i - zeros];
	  if ((c >= '0') && (c <= '9')) {
	      d = (int) (yytext[i - zeros] - '0');
	  }
	  else if ((c >= 'a') && (c <= 'f')) {
	      d = (int) (yytext[i - zeros] - 'a' + 10);
	  }
	  else if ((c >= 'A') && (c <= 'F')) {
	      d = (int) (yytext[i - zeros] - 'A' + 10);
	  }
	  else {
	      uerror("bad digit in hex double real constant");
              return(1);
	  }
       }
       if (i & 1) {
	   p[j++] |= d;
       }
       else {
	   p[j] = d << 4;
       }
    }
    yylval.unfloat = &fltexp;
    return(0);
}


/* get a character for a character constant or string read a string
 * up to lxmatch, taking escape sequences into account.
 * The string is stored in yytext[].
 * yylval.unlong is the character constant value, or for a string it
 * is the number of bytes in the string.
 * Note that it's o.k. to just leave the string in yytext[] and let the
 * parser access it there since all tokens that can legally follow a
 * string are simple chars (NL, SEMI, COMMA) and don't put anything into
 * yytext.  I suppose it might be cleaner to define and seperate buffer
 * array for these strings.
 */
lxstr(strflag, lxmatch)
  int strflag;		/* 1 for a string, 0 for a character */
  int lxmatch;		/* match character to end a string */
{
  register c;
  register val;
  register char * cptr;
  register int i;

  if (strflag)  {
	yylval.unlong = 0;
	cptr = yytext;
	}
  else
	yylval.unlong = 0;

  val = 0;
  i = 0;

  for(c=GETCHAR();;c=GETCHAR()) {
	if (strflag && (c == lxmatch))
		break;

	switch( c ) {
  	  case EOF:
  		uerror( "unexpected EOF" );
		goto endstr;

  	  case '\n':
  		uerror( "newline in string or char constant" );
  		UNGETC('\n');
		goto endstr;

  	  case '\\':
  		switch( c = GETCHAR() ){

  		case '\n':
# ifdef LISTER1
  			if (listflag)
  			   dump_echobuf();
# endif
			if (!strflag) {
			   uerror("newline in char constant");
			   line++;
			   return;
			   }
#ifdef LISTER2
			if (listflag) make_list_marker();
#endif
  			line++;
  			continue;

		case EOF:
			uerror( "unexpected EOF" );
			goto endstr;

  		default:
  			val = c;
  			goto mkcc;

  		case 'n':
  			val = '\n';
  			goto mkcc;

  		case 'r':
  			val = '\r';
  			goto mkcc;

  		case 'b':
  			val = '\b';
  			goto mkcc;

  		case 't':
  			val = '\t';
  			goto mkcc;

  		case 'f':
  			val = '\f';
  			goto mkcc;

  		case 'v':
  			val = '\013';
  			goto mkcc;

  		case '0':
  		case '1':
  		case '2':
  		case '3':
  		case '4':
  		case '5':
  		case '6':
  		case '7':
  			val = c-'0';
  			c=GETCHAR();  /* try for 2 */
  			if( lxmask[c+1] & LEXOCT ){
  				val = (val<<3) | (c-'0');
  				c = GETCHAR();  /* try for 3 */
  				if( lxmask[c+1] & LEXOCT ){
  					val = (val<<3) | (c-'0');
  					}
  				else UNGETC( c );
  				}
  			else UNGETC( c );

  			goto mkcc;

  			}
  	  default:
  		val =c;
  	  mkcc:
  		if (!strflag) {
  		   /* it is, after all, a "character" constant */
  		   yylval.unlong = (char) val;
		   return;		/* only one character */
  		   }
  		else { /* save the byte into yytext[] string buffer */
		   if (cptr <= &yytext[TXTCHARS])
			*cptr++ = val;
  		   ++i;
  		   continue;
  		   }
  		break;
  	     }
	  }

  endstr:
  if( strflag ){ /* end the string */
	*cptr = '\0';
	if ( i> TXTCHARS)
	    uerror("string too long");
	yylval.unlong = cptr - yytext;
  	}
  }


# ifdef LISTER1
dump_echobuf()
{
  *(echopt-1) = '\0';
  fprintf(stdout, "%04X	%s\n",listdot, echobuf);
  echopt = &echobuf[0];
  listdot = newdot;
}
# endif
