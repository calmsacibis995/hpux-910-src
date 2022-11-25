/************************************************************************/
/*									*/
/*	C FUNCTION:	 _qfns		( V1.0 )	8/20/88   XM	*/
/*									*/
/*	PURPOSE:	 To convert a quad precision floating point	*/
/*			 number to  a character string			*/
/*									*/
/*	SYNTAX:		 #include "quad.h"				*/
/*			 _qfns( q, s, status )				*/
/*			 quad     q;					*/
/*			 char     *s;					*/
/*			 unsigned *status;				*/
/*									*/
/*	ARGUMENTS:	 q -- input of quad floating number		*/
/*			 s -- output of character string,		*/
/*			      consisting of at least 44 characters	*/
/*			 status -- status code				*/
/*									*/
/*	RETURN VALUE:	 0 for no error					*/
/*			 other value for error				*/
/*									*/
/*	SUBSIDIARY:	 _qfmul						*/
/*									*/
/*	DEVELOPED BY:	 The Software Development Center( QHSOFT )	*/
/*			 & Department of Applied Mathematics,		*/
/*			 Tsinghua University, Beijing			*/
/*									*/
/*	------------							*/
/*									*/
/*	Variable list:							*/
/*			 exp0 -- the binary exponent of the floating	*/
/*				 point number				*/
/*			 exp  -- the decimal exponent of the floating	*/
/*				 point number				*/
/*			 sign -- the sign of a number			*/
/*			 rm   -- rounding mode				*/
/*			 stickybit -- the sticky bits			*/
/*									*/
/*	Temporary variables:						*/
/*			 i, j, k, f, g, n1, n2, c, c0			*/ 
/*									*/
/************************************************************************/

#ifdef _NAMESPACE_CLEAN
#define ltoa	_ltoa
#define strcpy	_strcpy
#endif /* _NAMESPACE_CLEAN */

#include "quad.h"

/***	Static variables					      ***/

/***	The exponential part of the following QUIN constants:	      ***/
/***	1E2560, 1E1280, 1E640, 1E320, 1E160, 1E80, 1E40, 1E20, 1E10,  ***/
/***	1E9,    1E8,    1E7,   1E6,   1E5,   1E4,  1E3,  1E2,  1E1,   ***/
/***	1E0							      ***/

unsigned _e0[NSR] =
	  { 0x61370000, 0x509B0000, 0x484D0000, 0x44260000, 0x42120000,
	    0x41080000, 0x40830000, 0x40410000, 0x40200000, 0x401C0000,
            0x40190000, 0x40160000, 0x40120000, 0x400F0000, 0x400C0000,
            0x40080000, 0x40050000, 0x40020000, 0x3FFF0000
          };

/***	The exponential part of the other specific QUIN constants:    ***/

unsigned _e1[NSR] =
	  { 0x1EC60000, 0x2F620000, 0x37B00000, 0x3BD70000, 0x3DEB0000,
	    0x3EF50000, 0x3F7A0000, 0x3FBC0000, 0x3FDD0000, 0x3FE10000,
            0X3FE40000, 0x3FE70000, 0x3FEB0000, 0x3FEE0000, 0x3FF10000,
            0x3FF50000, 0x3FF80000, 0x3FFB0000, 0x3FFB0000
          };

static int stickybit;

_qfns ( q, s, status )
char *s;
quad q;
unsigned *status;
{
  int           n1, n2, exp0, exp, c, c0;
  unsigned      sign, rm;
  register int  i,  j,  k;
  register QUIN f;
  QUIN          g;


/***	Pick up the sign and the exponent of Quad floating point      ***/
/***	number							      ***/

  sign = q[0] & 0x80000000;
  if( sign )    s[0] = '-';
  else	        s[0] = ' ';
  f[0] = q[0] & 0x7fffffff;
  f[1] = q[1];  f[2] = q[2];  f[3] = q[3];  f[4] = 0;
  s[1] = '0', s[2] = '.', s[3] = '\0';
  exp = 0;
  exp0 = f[0] & 0x7fff0000;

#if DEBUG
 printf("___QFNS__\n");
 printf("1..exp0=%x\n", exp0);
 W(q);
#endif


/***	Calculate the exponent by repeated dividing by 10 or 0.1      ***/
/***	( i.e. multiplying by 0.1 or 10 )  until the number lies      ***/
/***	in the range between 0.1 and 1.0.			      ***/

  stickybit = 0;
  if( !exp0 )

		/**  The exponent is zero ( E = Emin-1 and f = 0 )  **/

	      { for( k=0; k<SIZE; k++ ) if( f[k] )  break;
	        if( k == SIZE ) return( NOEXCEPTION );
	      }
  else if( exp0 == 0x7fff0000 )

		/**  Invalid exponent because of the operand q  **/
		/**  being infinity or NaN ( E = Emax + 1 )	**/

	      { 
		if( f[0] & 0xffff || f[1] || f[2] || f[3] )
	          { s[0] = 'N'; s[1] = 'a'; s[2] = 'N'; s[3] = '\0';
		  }
		else
		  { if ( q[0] &0x80000000 ) s[0] = '-';
		    else		    s[0] = '+';
		    s[1] = 'I'; s[2] = 'N'; s[3] = 'F'; s[4] = 'I';
		    s[5] = 'N'; s[6] = 'I'; s[7] = 'T'; s[8] = 'Y';
		    s[9] = '\0';
		  }

		/**  Trap if INVALIDTRAP enabled  **/

		if( *status & 0x00000020 )
			{ return( INVALIDOPERATION );
			}
		else
			{ *status |= 0x80000000;  /* set invalid flag	*/
			  return( NOEXCEPTION );
			}
	      }
  if( exp0 >= 0x3fff0000 )		/*   E >= 0 and E <= Emax   */
	      { for( k=0; k<NSR; k++ )
			if( ( f[0] & 0x7fff0000 ) >= _e0[k] )
			  { exp += _e[k];
			    _qfmul( _q1[k], f );
			  }
	      }
  else					/*   E >= Emin and E < 0    */
	      { for( k=0; k<NSR; k++ )
		   if( ( f[0] & 0x7fff0000 ) < _e1[k] )
		     { exp -= _e[k];
		       _qfmul( _q0[k], f );
		     }
	        if( ( f[0] & 0x7fffffff ) == 0x3ffb9999 )
		     { for( i=1; i<SIZEPLUS; i++ )
		         if( f[i] < 0x99999999 ) break;
		         else if( f[i] > 0x99999999 ) { i = 9; break; }
		         if( i < SIZEPLUS )
		           { exp--;
			     _qfmul( _q0[NSRMINUS], f );
			   }
		     }
		else if( ( f[0] & 0x7fffffff ) < 0x3ffb9999 )
		     { exp--;
		       _qfmul ( _q0[NSRMINUS], f );
		     }
	      }


/***	Multiply f by 10/16 and pick up first four bits to calculate  ***/
/***	decimal fraction					      ***/

  i = 3;
  exp0 = f[0] & 0x7fff0000;

#if DEBUG
 printf("2..exp,exp0=%x %x\n",exp,exp0);
 W(f);
#endif

  f[0] &= 0xffff;               n1 = 0;	   /* E = Emin-1 and f != 0  */
  if( exp0 ) { f[0] += 0x10000; n1 = 1;	}  /* Emin =< E <= Emax      */
  exp0 = ( exp0 >> EXPWPLUS ) - n1 - BIAS + HOF;
  LSHIFT( f, exp0, SIZEPLUS )

#if DEBUG
 printf("3..exp0=%x\n",exp0);
 W(f);
#endif

  do {	for( k=0; k<SIZEPLUS; k++ )  g[k] = f[k];
	RSHIFT( f, 1, SIZEPLUS )
	RSHIFT( g, 3, SIZEPLUS )
	c = 0;
	for( k=SIZE; k>=0; k-- )
	   {	c0 = f[k];
		f[k] += c;
		if( f[k]<c0 || f[k]<c )  c = 1;
		else   			 c = 0;
		c0 = f[k];
		f[k] = f[k] + g[k];
		if( f[k]<c0 || f[k]<g[k] )  c += 1; 
	   }
	s[i++] = ( ( f[0] & 0xf0000000 ) >> 28 ) + 0x30;
	LSHIFT( f, 4, SIZEPLUS )
     }  while( i < 38 );

#if DEBUG
 printf("4..i,s=%d %s\n",i,s);
 W(f);
#endif


/***	Rounding						      ***/

  stickybit |= f[0] | f[1] | f[2] | f[3] | f[4];
  rm = ( *status ) & 0x00000600;
  i--;
  if( ( !rm && ( s[i] == '5' && s[i-1] % 2 ||
	         s[i] == '5' && stickybit  ||
	         s[i] > '5' ) )
      ||
      ( ( rm == 0x00000400 && !sign || rm == 0x00000600 && sign ) &&
	       ( s[i] > '0' || stickybit ) ) )
		       { for( k=i-1; k>2; k-- )
			      if( s[k] != '9' )  { s[k] += 1;  break; }
			      else 	           s[k] = '0';
			 if( k == 2 )  { exp += 1; s[3] = '1'; }
		       }

/***	Increase the exponent part				      ***/

  s[i++] = 'E';
  for( j=i; j<=i+5; j++ ) s[j] = 0;
#ifdef SPRINTF
  sprintf( &s[i], "%-5d", exp );
#else
  strcpy( &s[i], ltoa(exp) );
  /* We must restrict the exponent to 5 digits.  More than 5 digits may have
   * been copied into s[i], so put a null in the 5th position again in case
   * more than 5 digits were copied.
   */
  s[i+5] = 0;
#endif SPRINTF

#if DEBUG
 printf("5..i,exp,k,s=%d %d %d %s\n",i,exp,k,s);
#endif

/***	Check for INEXACT					      ***/

  if( ( s[i] || stickybit ) &&
      ( q[3] || q[2] || q[1] & 0x3fff ||
		 ( q[0] & 0x7fff0000 ) > 0x40700000 ||
		 ( q[0] & 0x7fff0000 ) < 0x3f8e0000    ) )
    { if( *status & 0x1 )	/* trap if INEXACTTRAP enabled	*/
	return( INEXACT );
      else
	*status |= 0x08000000;	/*  set INEXACT flag	*/ 
    }

  return( NOEXCEPTION );
}
