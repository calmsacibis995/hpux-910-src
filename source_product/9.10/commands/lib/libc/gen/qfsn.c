/************************************************************************/
/*									*/
/*	C FUNCTION:	 _qfsn		( V1.0 )	/8/20/88   XM	*/
/*									*/
/*	PURPOSE:	 To convert a character string to a quad	*/
/*			 precision floating point number		*/
/*									*/
/*	SYNTAX:		 #include "quad.h"				*/
/*			 int _qfsn( s, q, status, langflag, gen )	*/
/*			 char     *s;					*/
/*			 quad     q;					*/
/*			 unsigned *status;				*/
/*			 int	  langflag;				*/
/*			 union	  gen; (int ftnp or char **ptr)		*/
/*									*/
/*	ARGUMENTS:	 s -- input of character string			*/
/*			 q -- output of quad floating point number	*/
/*			 status -- status code				*/
/*			 langflag -- 0 indicates ANSI C			*/
/*				     1 indicates FORTRAN		*/
/*			 ftnp -- FORTRAN scale factor			*/
/*			 ptr -- C end pointer				*/
/*									*/
/*	RETURN VALUE:	 0 for no error					*/
/*			 other value for error				*/
/*									*/
/*	SUBSIDIARY:	 _qfmul						*/
/*									*/
/*	DEVELOPED BY:	 The Software Development Center( QHSOFT )	*/
/*			 & Department of Applied Mathematics,		*/
/*			 Tsinghua University, Beijing, China		*/
/*									*/
/*	------------							*/
/*									*/
/*	Variable list:							*/
/*			 lnzd  -- location of the first nonzero digit	*/
/*			 ldp   -- location of the decimal point		*/
/*			 dcdi  -- the digit count of the decimal	*/
/*				  integer				*/
/*			 n     -- the number of the significant digits	*/
/*				  in a decimal floating point number	*/
/*			 rm    -- rounding mode				*/
/*			 fract -- a buffer used for storing the		*/
/*				  significant digits of a decimal	*/
/*				  floating point number			*/
/*			 exp   -- the decimal exponent of the floating	*/
/*				  point number				*/
/*			 stickybit -- the sticky bits			*/
/*			 guardbit  -- the guard bit			*/
/*			 expchars  -- set of allowed exponent characters*/
/*									*/
/*	Temporary variables:						*/
/*			 i, j, k, m, f, g, c, c0, inexact, flag, b	*/
/*									*/
/*	Modified by:	Chih-Ming Hu to rename some variables and 	*/
/*			procedures to avoid name space pollution and	*/
/*			to delete error message output			*/
/*	Date:		Dec. 5, 1988					*/
/*									*/
/*	Modified by:	Liz Sanville to add enhancements and fix bugs.	*/
/*			- introduce flag (0=ANSI C, 1=FORTRAN)		*/
/*			- whitespace input buffer returns 0		*/
/*			- '.' input buffer returns 0			*/
/*			- '' (null) input buffer returns 0		*/
/*			- [EeDdQq]{DIGIT}* returns 0 (e.g. e or e5)	*/
/*			- allow [Ee] exponent for C and [EeDdQq] for	*/
/*			  FORTRAN					*/
/*			- allow leading and trailing whitespace (not	*/
/*			  only blanks)					*/
/*			- allow {DIGIT}*[EeDdQq] followed by no digits	*/
/*			  (e.g. 1e)					*/
/*			- disallow +- (adjacent signs)			*/
/*	Date:		Dec. 1988 - Jan. 1989				*/
/*									*/
/*	Modified by:	Chih-Ming Hu so that '-' sign will be printed 	*/
/*			out if the output is -infinity              	*/
/*	Date:		Jan. 6, 1988					*/
/*									*/
/*	Modified by:	Liz Sanville to add ANSI C end pointer 		*/
/*			enhancement.  End pointer (gen.ptr) marks where	*/
/*			scanning terminated.				*/
/*	Date:		Jan. 16, 1988					*/
/*									*/
/************************************************************************/

#ifdef _NAMESPACE_CLEAN
#define strchr	_strchr
#define atoi	_atoi
#endif /* _NAMESPACE_CLEAN */

#include <ctype.h>
#include "quad.h"

/***	Static variables					      ***/

/***	The constants being used for right shifting		      ***/

static unsigned _rs[LENGTH] =
		      { 0x00000001, 0x00000003, 0x00000007, 0x0000000f,
		        0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
		        0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
		        0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
		        0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,  
		        0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
		        0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
		        0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff
		      };

static int stickybit;

_qfsn ( s, q, status, langflag, gen )
char *s;
quad q;
unsigned *status;
int langflag;
union { int ftnp; char **ptr; } gen;
{
  int           lnzd, ldp, dcdi, n, exp, m, c, c0, guardbit, inexact, flag;
  unsigned int  rm;
  register int  i, j, k;
  register QUIN f;
  QUIN          g;
  char		b[120];
  char		*expchars;
  struct { unsigned bcd : 4; } fract[BUFSIZE];


/***	To convert a string to a normalized decimal floating point    ***/
/***	number ( i.e. 0.A1A2...An * 10 ** exp ) and validate the      ***/
/***	string							      ***/

		/**  Initializing character pointer  **/
		/**  and some other variables         **/

  if ( langflag != FORTRAN ) exp = 0;
  else exp = -gen.ftnp;
  dcdi = -100;
  q[0] = q[1] = q[2] = q[3] = 0;
  for( i=0; i<SIZEPLUS; i++ )  f[i] = 0;
  stickybit = 0;
  i = n = lnzd = ldp = -1;
  rm = ( *status ) & 0x600;
  if ( langflag == ANSIC ) expchars = "Ee";
  else expchars = "EeDdQq";

			/**  Pick up the sign  **/

  if ( langflag == ANSIC && gen.ptr != (char **)0 ) 
     *gen.ptr = s;				/*  initialize ptr	*/
  while( isspace(s[++i]) );			/*  clear blanks	*/
  if( s[i] == '\0' )				/*  all blanks or null  */
  	return( NOEXCEPTION );  		/*  the result is zero  */
  if( s[i] == '+' )  
	i++;					/*  sign is '+'		*/
  else if( s[i] == '-' )  
  	q[0] = 0x80000000, i++;			/*  sign is '-'		*/

		/**  Pick up all significant digits  **/
		/**  A1A2...An			     **/

  do { if( s[i] == '.' )
	 { if( ldp > -1 )   /* check invalid decimal points	   	*/
		{ lnzd = -1;  break; }
	   if( lnzd < 0 )   /* no nonzero digits before decimal point	*/
	   	{ lnzd = -3;
		  dcdi = 0;
		}
	   else		    /* some nonzero digits before decimal point */
		  dcdi = i - lnzd;
	   ldp = i++;	    /* store the location of decimal point	*/
	 }
       if( s[i] < '0' || s[i] > '9' )  break;	/* nondigit occurs	*/
       if( s[i] == '0' && lnzd < 0 )		/* it is leading zero	*/
	  { lnzd = -2;			   /* when the mantissa is zero	*/
	    if( ldp >= 0 ) dcdi--;	   /* negative exponent		*/
	  }
       else
	  { if( lnzd < 0 )  lnzd = i;
	    if( s[i] > '0' && n >= BUFSIZEMINUS )   stickybit = 1;
	    if( ! ( n == -1 && s[i] == '0' )	/* clear leading zeroes	*/
		&& n < BUFSIZEMINUS )		/* it is a valid digit	*/
			  fract[++n].bcd = s[i];
	  }
       i++;
     }  while( 1 );
  while( ! fract[n].bcd )  n--;
  /* set gen.ptr to current scan pos if at least one significant digit */
  if ( langflag == ANSIC && gen.ptr != (char **)0 ) 
        if ( (n >= 0) || (lnzd == -2) ) *gen.ptr = &s[i];

#if DEBUG
 printf("___QFSN___\n");
 printf("1..i,lnzd,ldp,n,stickybit=%d %d %d %d %x\n",i,lnzd,ldp,n,stickybit);
 printf("fract=\n");
 for(j=0;j<5;j++) { for(k=0;k<8;k++) printf("%8x ",fract[j*8+k]);
		    printf("\n");
		  }
#endif

		/**  Check if the string is valid  **/
		/**  and calculate exp		   **/

	/** At this point, 					**/
	/**    lnzd == -1 means no digits encountered or more 	**/
	/**		  than one decimal point encountered.	**/
	/**    lnzd == -2 means only leading zero's encountered.**/
	/**    lnzd == -3 means only decimal point encountered.	**/

  j = i;
  while( isspace(s[i]) ) i++;			/*  skip blanks		*/
  if ( s[i] != '\0' ) i = j;			/*  reset i		*/
  if( (lnzd == -2 || lnzd == -3) && s[i] == '\0' )
			return( NOEXCEPTION );  /*  the result is zero  */
  if( dcdi == -100 )	dcdi = i - lnzd;      /*  bits of integral part	*/
  if ( s[i] != '\0' && strchr(expchars,s[i]) )
      {	j = ++i;
	if( s[i] == '\0' || isspace(s[i]) )
	    { 
	      if( langflag == ANSIC && (lnzd == -1 || lnzd == -3) )
			/* don't preserve sign bit */
			{ q[0] = 0;
			  return( NOEXCEPTION );
			}
	      if( lnzd < 0 )
			return( NOEXCEPTION );    /*  the result is zero  */
	      if ( langflag == FORTRAN ) 
			{ int j;
			  for ( j = i; s[j] != '\0' && isspace(s[j]) ; j++ )
				;
			  if ( s[j] != '\0' ) lnzd = -1;
	      		}
	    }
	else	/* exponent character present */
	    {
	      if( s[i] == '+' || s[i] == '-' ) 
			i++; 			  /* sign of the exponent */
	      k = i;
	      while( s[i] >= '0' && s[i] <= '9' ) i++;	  /* it is digits */
	      if( langflag == FORTRAN && ( s[i] == '\0' || isspace(s[i]) ) 
		  && i > k )
			/* is it valid  */
			{ if( lnzd < 0 )
			  	return( NOEXCEPTION );  /* the result is zero */
			  /* calculate the exponent */
			  exp = atoi(&s[j]);
			}
	      else if( langflag == ANSIC && i > k )
			/* is it valid  */
			{ /* set gen.ptr to after the exponent */
     			  if ( lnzd != -1 && lnzd != -3 && 
			       gen.ptr != (char **)0 ) 
			     *gen.ptr = &s[i];
			  if( lnzd < 0 )
			  	return( NOEXCEPTION );  /* the result is zero */
			  /* calculate the exponent */
			  exp = atoi(&s[j]);
			}
	      else  lnzd = -1;
	    }
      }
  else if( s[i] != '\0' )  lnzd = -1;

  if( langflag == FORTRAN && lnzd == -1 )/*   input illegal character   */
       { 
	 q[0] |= 0x7fff4000;
	 if( *status & 0x00000020 )	 /* trap if INVALIDTRAP enabled	*/
		{ return( INVALIDOPERATION );
		}
	 else
		{ *status |= 0x80000000;	/* set invalid flag	*/
		  return( NOEXCEPTION );
		}
       }
  else if( langflag == ANSIC && lnzd == -1 && n < 0 )
       { 
	 q[0] = 0;			 /*   don't preserve sign bit 	*/
	 if( *status & 0x00000020 )	 /* trap if INVALIDTRAP enabled	*/
		{ return( INVALIDOPERATION );
		}
	 return( NOEXCEPTION );
       }
  exp += dcdi;				/*    calculate the exponent	*/

#if DEBUG
 printf("2..n,exp,dcdi=%d %d %d\n",n,exp,dcdi);
#endif

/***	The character string is valid, and then check that if it flow ***/
/***	and continue handle					      ***/

  exp--;
  flag = ( ! stickybit ) && ( dcdi > -40 ) && ( n > dcdi - 1 );
  if( -4966 <= exp && exp <= 4932 )


/***	Express the fraction of the normalized decimal floating	      ***/
/***	point number in binary					      ***/

    { m = -1;
      for( k=1; k<SIZEPLUS; k++ )  g[k] = 0;
      for( i=0; i<SIZEPLUS; i++ )
        { for( j=28; j>=0; j-=4 )
	     { INC( f, fract[++m].bcd << j, i );   /* Am*16^(-m)+f[m-1]	*/
						   /*	 ==> f[m]	*/
	       if( n == m ) break;

		    /**  f[m]*(10/16) ==> f[m]	 **/

	       for( k=0; k<SIZEPLUS; k++ )  g[k] = f[k];
	       stickybit |= f[SIZE] & 0x7;
	       RSHIFT( f, 1, SIZEPLUS )		/* 	f/2 ==> f	*/
               RSHIFT( g, 3, SIZEPLUS )		/*	g/8 ==> g	*/
	       c = 0;
	       for( k=SIZE; k>=0; k-- )		/*	f + g ==> f	*/
		  { c0 = f[k];
		    f[k] += c;
		    if( f[k]<c0 || f[k]<c ) c = 1;
		    else		    c = 0;
		    c0 = f[k];
		    f[k] += g[k];
		    if( f[k]<c0 || f[k]<g[k] ) c += 1;
		  }
	     }
	  if( n == m ) break;
        }

#if DEBUG
 printf("3..n,m,stickybit=%d %d %x\n",n,m,stickybit);
 W(f);
#endif


/***	Normalize the above binary number			      ***/

      for( k=0; k<LENGTH; k++ )  if( f[0] & _ls[k] ) break;
      if( k == LENGTH ) { for( k=0; k<LENGTH; k++ )
				 if( f[1] & _ls[k] ) break;
		          k += HOF ;
		        }
      else  k -= EXPW;
      if( k > 0 )        LSHIFT( f, k, SIZEPLUS )
      else if( k < 0 ) { stickybit = f[SIZE] & _rs[-1-k];
		         RSHIFT( f, -k, SIZEPLUS )
		       }
      f[0] = ( ( n << 2 ) - k - 12 + Emax << EXPWPLUS ) + f[0] - 0x10000;

#if DEBUG
 printf("4..k,stickybit=%d %x\n",k,stickybit);
 W(f);
#endif


/***	Multiply the normalized number by  quad floating point        ***/
/***	number 10**(exp-n)					      ***/

      i = exp - n;
      if( i > 0 )
            { for( k=0; k<9; k++ )
		 if( i >=  _e[k] ) { _qfmul( _q0[k], f ); i -= _e[k]; }
              if( i > 0 ) _qfmul( _q0[NSRMINUS-i], f );
            }
      else if( i < 0 )
	     { for( k=0; k<9; k++ )
		  if( i <= -_e[k] ) { _qfmul( _q1[k], f ); i += _e[k]; }
               if( i < 0 ) _qfmul( _q1[NSRMINUS+i], f );
	     }
      else if( ! stickybit )
	     { q[0] |= f[0];  q[1] = f[1];  q[2] = f[2];  q[3] = f[3];
	       return( NOEXCEPTION );
	     }

      stickybit |= f[SIZE] & 0x7fffffff;
      guardbit = f[SIZE] & 0x80000000;
      inexact = guardbit | stickybit;

#if DEBUG
 printf("5..i,exp=%d %d\n",i,exp);
 printf("stickybit,guardbit=%x %x\n",stickybit,guardbit);
 W(f);
#endif


/***	The RM field in the argument status controls the rounding     ***/
/***	mode. It rounds to 128 bits for the quad floating point       ***/
/***	number.							      ***/
/***	   Rounding mode	  Description			      ***/
/***		0		Round to nearest		      ***/
/***		1		Round toward zero		      ***/
/***		2		Round toward positive infinity	      ***/
/***		3		Round toward negative infinity	      ***/

      if( ( !rm  && ( f[SIZE] == 0x80000000 && f[SIZEMINUS] & 0x00000001 ||
		      f[SIZE] == 0x80000000 && stickybit	         ||
		      f[SIZE] >  0x80000000 ) )
          ||
          ( ( rm == 0x00000400 && !q[0] || rm == 0x00000600 && q[0] ) &&
		    inexact ) )  INC( f, 1, SIZEMINUS )

#if DEBUG
 printf("6..");
 W(f);
#endif


/***	Test for OVERFLOW					      ***/

      if( f[0] >= 0x7fff0000 )
	{ if( *status & 0x4 )		/* trap if OVERFLOWTRAP enabled	*/
	    { f[0] -= BIAS_WRAP << EXPWPLUS;
	      q[0] |= f[0]; q[1] = f[1]; q[2] = f[2]; q[3] = f[3];
	      if( inexact )		/*     test for INEXACT		*/
		{ if( *status & 0x1 )	/* trap if INEXACTTRAP enabled	*/
		    return( IOVERFLOW | INEXACT );
		  else
		    *status |= 0x08000000;	/* set INEXACT flag	*/
		}
	      return( IOVERFLOW );
	    }
	  else
	    { inexact = 1;
	      *status |= 0x20000000;		/* set OVERFLOW flag	*/
	      switch( rm )
		{ case 0x000:	f[0] = 0x7fff0000;
				f[1] = f[2] = f[3] = 0;
				break;
		  case 0x200:	f[0] = 0x7ffeffff;
				f[1] = f[2] = f[3] = 0xffffffff;
				break;
		  case 0x400:	if( q[0] )
				  { f[0] = 0x7fff0000;
				    f[1] = f[2] = f[3] = 0;
				  }
				else
				  { f[0] = 0xfffeffff;
				    f[1] = f[2] =f[3] = 0xffffffff;
				  }
				break;
		  case 0x600:	if( q[0] )
				  { f[0] = 0x7ffeffff;
				    f[1] = f[2] = f[3] = 0xffffffff;
				  }
				else
				  { f[0] = 0xffff0000;
				    f[1] = f[2] = f[3] = 0;
				  }
		}
	    }
	}


/***	Test for UNDERFLOW					      ***/

      else if( ! ( f[0] & 0x7fff0000 ) )
	     { if( *status & 0x2 )	/* trap if UNDERFLOW enabled	*/
		 { f[0] += BIAS_WRAP << EXPWPLUS;
		   q[0] |= f[0]; q[1] = f[1]; q[2] = f[2]; q[3] = f[3];
		   if( inexact )	/*    Test for INEXACTTRAP	*/
		     { if( *status & 0x1 )/* trap if INEXACTTRAP enabled*/
			 return( IUNDERFLOW | INEXACT );
		       else
			 *status |= 0x08000000;	/* set INEXAVT flag	*/
		     }
		   return( IUNDERFLOW );
		 }
	     }	/* else missing - to be added by Chih-Ming (ets) */

    }


/***	Test for OVERFLOW					      ***/

  else if( exp > 4932 )
	 { if( *status & 0x4 )		/*  trap if OVERFLOW enabled	*/
	    { q[0] = 0x7fff0000;
	      return( IOVERFLOW );
	    }
	   else
	    { inexact = 1;
	      *status |= 0x20000000;		/* set OVERFLOW flag	*/
	      switch( rm )
		{ case 0x000:	f[0] = 0x7fff0000;
				f[1] = f[2] = f[3] = 0;
				break;
		  case 0x200:	f[0] = 0x7ffeffff;
				f[1] = f[2] = f[3] = 0xffffffff;
				break;
		  case 0x400:	if( q[0] )
				  { f[0] = 0x7fff0000;
				    f[1] = f[2] = f[3] = 0;
				  }
				else
				  { f[0] = 0xfffeffff;
				    f[1] = f[2] =f[3] = 0xffffffff;
				  }
				break;
		  case 0x600:	if( q[0] )
				  { f[0] = 0x7ffeffff;
				    f[1] = f[2] = f[3] = 0xffffffff;
				  }
				else
				  { f[0] = 0xffff0000;
				    f[1] = f[2] = f[3] = 0;
				  }
		}
	    }

	 }
  else


/***	Test for UNDERFLOW					      ***/

	 { if( *status & 0x2 )		/*  trap if UNDERFLOW enabled	*/
		{ return( IUNDERFLOW );
		}
	   else { *status |= 0x10000000;	/*  set UNDERFLOW flag	*/
		  return ( NOEXCEPTION );
		}
	 }


/***	Check for INEXACT					      ***/

#if DEBUG
 printf("7..inexact=%x\n",inexact);
#endif

      q[0] |= f[0]; q[1] = f[1]; q[2] = f[2]; q[3] = f[3];
      if( flag )
	{ for(i=0;i<120;i++) b[i]=0;
	  if( dcdi > 0 )
	    { k = 40 - dcdi;
	      for( i=0; i<=n; i++ ) b[k+i] = fract[i].bcd;
	      j = k + n + 1;
	      do { c = 0;
		   for( i=k; i<j; i++ )
		      { c0 = b[i] & 0x1;
			b[i] = ( b[i] >> 1 ) + c;
			if( c0 ) c = 5;
			else     c = 0;
		      }
		   if( c ) b[i++] = c;
		   for( k=k; k<40; k++ ) if( b[k] ) break;
		   if( k == 40 ) break;
		   j = i;
		 } while( 1 );
	    }
	  else
	    { for( j=40; j<40-dcdi; j++ ) b[j] = 0;
	      for( i=j; i<=n+j; i++ ) b[i] = fract[i-j].bcd;
	    }
	  k = 40;
	  m = 0;
	  i--;
	  while( m < 113 )
	       { c = 0;
		 for( j= i; j>=k; j-- )
		    { b[j] += b[j] + c;
		      if( b[j] > 9 ) { c = 1; b[j] -= 10; }
		      else	       c = 0;
		    }
		 if( ! b[i] ) i--;
		 if( i < k ) break;
		 if( m ) m++;
		 if( ! m && c ) m++;
	       }
	  if( i < k ) inexact = 0;
	  else       { for( j=k; j<=i; j++ ) if( b[j] ) break;
		       if( j == i + 1 ) inexact = 0;
		       else		inexact = 1;
		     }
	}
      if( inexact )			/*	Test for INEXACT	*/
		{ if( *status & 0x1 )	/* trap if INEXACTTRAP enabled	*/
			return( INEXACT );
		  else
			*status |= 0x08000000;	/*  set INEXACT flag	*/ 
		}
      return( NOEXCEPTION );
  
}
