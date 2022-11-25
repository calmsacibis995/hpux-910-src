/* @(#) $Revision: 66.5 $ */     

/* modifications:
/*  
    03-08-84 MHO - Updated to the Sys5.2 version of this file, it 
		   includes NAMESIZE and CXREF additions.  Actually
		   picked up their source rather than modifying our
		   own because theirs is so much more readable.
*/

/* #ifdef FLEXNAMES            Bell's name size constant
/* #	define NCPS	128
/* #else
/* #	define NCPS	8
/* #endif
*/

#ifdef CXREF
extern int doCXREFdata;
#endif
#ifndef NLS
#define nl_msg(i, s) (s)
#else
#define NL_SETN 2	/* set number */
#include <msgbuf.h>
#endif /* NLS */

/* our name size constant, we don't have the conditional compile */
#define NAMESIZE  255   

extern int ncps;	/* actual number of chars. */
#ifdef CXREF
extern int xline;
#endif

#if defined(hpux) || defined(_HPUX_SOURCE) || defined(OSF)
#define COFF 128
#else  
#define COFF 0
#endif


#define isid(a)  ((fastab+COFF)[a]&IB)
#define IB 1
/*	#if '\377' < 0		it would be nice if this worked properly!!!!! */

yylex()
{
	static int ifdef = 0;
	static char *op2[] = {"||",  "&&" , ">>", "<<", ">=", "<=", "!=", "=="};
	static int  val2[] = {OROR, ANDAND,  RS,   LS,   GE,   LE,   NE,   EQ};
	static char *opc = "b\bt\tn\nf\fr\r\\\\";
	extern char fastab[];
	extern char *outp, *inp, *newp;
	extern int flslvl;
	register char savc, *s;
	char *skipbl();
	int val;
	register char **p2;
	struct symtab
	{
		char *name;
		char *value;
	} *sp, *lookup();

	extern int passcom;
	int savepass;

	for ( ;; )
	{
		savepass = passcom;  /* bell doesn't do this, don't */
		passcom = 0;         /* know why we do.             */
		newp = skipbl( newp );
		passcom = savepass;

		if ( *inp == '\n' ) 		/* end of #if */
			return( stop );
		savc = *newp;
		*newp = '\0';
		if ( *inp == '/' && inp[1] == '*' )
		{
			/* found a comment with -C option, still toss here */
			*newp = savc;
			outp = inp = newp;
			continue;
		}
		for ( p2 = op2 + 8; --p2 >= op2; )	/* check 2-char ops */
			if ( strcmp( *p2, inp ) == 0 )
			{
				val = val2[ p2 - op2 ];
				goto ret;
			}
		s = "+-*/%<>&^|?:!~(),";		/* check 1-char ops */
		while ( *s )
			if ( *s++ == *inp )
			{
				val= *--s;
				goto ret;
			}
		if ( *inp<='9' && *inp>='0' )		/* a number */
		{
			if ( *inp == '0' )
				yylval= ( inp[1] == 'x' || inp[1] == 'X' ) ?
					tobinary( inp + 2, 16 ) :
					tobinary( inp + 1, 8 );
			else
				yylval = tobinary( inp, 10 );
			val = number;
		}
		else if ( isid( *inp ) )
		{
			if ( strcmp( inp, "defined" ) == 0 )
			{
				ifdef = 1;
				++flslvl;
				val = DEFINED;
			}
			else
			{
				if ( ifdef != 0 )
				{
					register char *p;
					register int savech;

					/* make sure names <= ncps chars */
					if ( ( newp - inp ) > ncps )
						p = inp + ncps;
					else
						p = newp;
					savech = *p;
					*p = '\0';
					sp = lookup( inp, -1 );
					*p = savech;
					ifdef = 0;
					--flslvl;
				}
				else
					sp = lookup( inp, -1 );
#ifdef CXREF
                                if (doCXREFdata)
				  ref(inp, xline);
#endif
#ifdef STAT_ANAL
				{ extern void sa_macro_use_in_if ();
				sa_macro_use_in_if (inp); }
#endif
				yylval = ( sp->value == 0 ) ? 0 : 1;
				val = number;
			}
		}
		else if ( *inp == '\'' )	/* character constant */
		{
			val = number;
			if ( inp[1] == '\\' )	/* escaped */
			{
				char c;

				if ( newp[-1] == '\'' )
					newp[-1] = '\0';
				s = opc;
				while ( *s )
					if ( *s++ != inp[2] )
						++s;
					else
					{
						yylval = *s;
						goto ret;
					}
				if ( inp[2] <= '9' && inp[2] >= '0' )
					yylval = c = tobinary( inp + 2, 8 );
				else
					yylval = inp[2];
			}
			else
				yylval = inp[1];
		}
		else if ( strcmp( "\\\n", inp ) == 0 )
		{
			*newp = savc;
			continue;
		}
		else
		{
			*newp = savc;
			pperror( (nl_msg(1, "Illegal character %c in preprocessor if")),
				*inp );
			continue;
		}
	ret:
		/* check for non-ident after defined (note need the paren!) */
		if ( ifdef && val != '(' && val != DEFINED )
		{
			pperror( (nl_msg(2, "\"defined\" modifying non-identifier \"%s\" in preprocessor if")), inp );
			ifdef = 0;
			flslvl--;
		}
		*newp = savc;
		outp = inp = newp;
		return( val );
	}
}

tobinary( st, b )
	char *st;
{
	int n, c, t;
	char *s;

	n = 0;
	s = st;
	while ( c = *s++ )
	{
		switch( c )
		{
		case '0': case '1': case '2': case '3': case '4': 
		case '5': case '6': case '7': case '8': case '9': 
			t = c - '0';
			break;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': 
			t = (c - 'a') + 10;
			if ( b > 10 )
				break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': 
			t = (c - 'A') + 10;
			if ( b > 10 )
				break;
		default:
			t = -1;
			if ( c == 'l' || c == 'L')
			  {
			  if (*s == 'u' ||  *s == 'U') s++;
			  if ( *s == '\0' ) break;
			  }
			if ( c == 'u' || c == 'U')
			  {
			  if (*s == 'l' ||  *s == 'L') s++;
			  if ( *s == '\0' ) break;
			  }
			pperror( (nl_msg(3, "Illegal number %s")), st );
		}
		if ( t < 0 )
			break;
		n = n * b + t;
	}
	return( n );
}
