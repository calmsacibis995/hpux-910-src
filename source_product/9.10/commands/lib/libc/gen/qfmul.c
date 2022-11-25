/************************************************************************/
/*									*/
/*	C FUNCTION:	_qfmul		( V1.0 )	8/20/88   XM	*/
/*									*/
/*	PURPOSE:	To calculate the product of two positive	*/
/*			floating point multipliers			*/
/*									*/
/*	SYNTAX:		#include "quad.h"				*/
/*			_qfmul( _q1, _q2 )				*/
/*			QUIN   _q1, _q2;				*/
/*									*/
/*	ARGUMENTS:	_q1 -- input of quad floating point number	*/
/*			_q2 -- input and output of quad floating 	*/
/*			      number					*/
/*									*/
/*	DEVELOPED BY:	The Software Development Center( QHSOFT )	*/
/*			& Department of Applied Mathematics,		*/
/*			Tsinghua University, Beijing			*/
/*									*/
/*	------------							*/
/*									*/
/*	Veriable list:							*/
/*			_e1 -- the exponent of the floating point	*/
/*			      number _q1				*/
/*			_e2 -- the exponent of the floating point	*/
/*			      number _q2				*/
/*			exp -- the exponent of the product		*/
/*			stickybit -- the sticky bits			*/
/*									*/
/*	Temporary variables:						*/
/*			i, j, k, f1, f2, q, c0, c			*/
/*									*/
/************************************************************************/

#include "quad.h"

/***    Static variables                                             ***/

/***	The patterns for masking specific bits			      ***/

static unsigned  _a[LENGTH] = 
		      { 0x00000001, 0x00000002, 0x00000004, 0x00000008,
		        0x00000010, 0x00000020, 0x00000040, 0x00000080,
		        0x00000100, 0x00000200, 0x00000400, 0x00000800,
		        0x00001000, 0x00002000, 0x00004000, 0x00008000,
		        0x00010000, 0x00020000, 0x00040000, 0x00080000,
		        0x00100000, 0x00200000, 0x00400000, 0x00800000,
		        0x01000000, 0x02000000, 0x04000000, 0x08000000,
		        0x10000000, 0x20000000, 0x40000000, 0x80000000
		      };

static int stickybit;

_qfmul( _q1, _q2 )
QUIN   _q1, _q2;
{
#ifndef _QFMPY
  int                    exp;
  unsigned int           _e1,   _e2,   f1;
  register QUIN          f2;
  register unsigned int  q[SIZEPLUS+SIZEPLUS];
  register int           i,    j,    k;

#if DEBUG
 printf("___QFMUL___\n1..");
 W(_q1);W(_q2);
#endif


/***	Calculate the exponent of a product, put the fraction of two  ***/
/***	normalized multipliers into _q1 and f2 in a normal format      ***/

  _e1    = _q1[0] & 0x7fff0000;
  _e2    = _q2[0] & 0x7fff0000;
  f1    = _q1[0];
  _q1[0] = _q1[0] & 0x0000ffff | 0x00010000;
  f2[0] = _q2[0] & 0x0000ffff;
  for( i=1; i<SIZEPLUS;i++ )   f2[i] = _q2[i];
  exp = ( ( _e1 + _e2 ) >> EXPWPLUS ) - Emax - Emax;
  if( _e2 )  f2[0] |= 0x00010000;
  else	  { for( i=0; i<SIZEPLUS; i++ )          if( f2[i] ) break;
	    for( j=0; j<SIZEPLUS-i; j++ )        f2[j] = f2[j+i];
	    for( j=SIZEPLUS-i; j<SIZEPLUS; j++ ) f2[j] = 0;
	    for( j=0; j<LENGTH; j++ )        if( f2[0] & _ls[j] ) break;
	    if( j > EXPW )               LSHIFT( f2, j-EXPW, SIZEPLUS )
	    else if( j < EXPW )          RSHIFT( f2, EXPW-j, SIZEPLUS )
	    exp -= i * LENGTH + j - EXPWPLUS;
          }

#if DEBUG
 printf("2..exp=%d\n",exp); W(_q1); W(f2);
#endif


/***	Calculate the product of fraction			      ***/

  for( i=0; i<SIZEPLUS+SIZEPLUS; i++ )  q[i] = 0;
  for( j=SIZE; j>=0; j-- )


/***	Add the partial product _q1 to q according to the position of  ***/
/***	the least significant bit( the jth word, the kth bit )	      ***/

	    { if( _a[0] & f2[j] )
		  { i = SIZEPLUS;
		    while( i > 0 )  INC( q, _q1[i-1], j+i-- );
		  }
  	      for( k=1; k<LENGTH; k++ )
		if( _a[k] & f2[j] )
		  { i = SIZEPLUS;
		    while( i > 0 )
		      { INC( q, _q1[i-1]<<k, j+i-- );
		        if( i>0 || k>EXPW ) INC( q, _q1[i]>>(LENGTH-k), j+i );
		      }
		  }
	    }
  _q1[0] = f1;

#if DEBUG
 printf("3..");
 W(q);
#endif


/***	If the exponent is less than Emin, right shift the fraction   ***/
/***	to make the exponent equal to Emin			      ***/

  if( exp < Emin )	/* shifting all bits of each element of the   	*/
			/* array to right by Emin-exp bits		*/
    { i = ( Emin - exp );
      j = i / LENGTH;
      for( k=SIZEPLUS-1; k>=j; k-- )  q[k] = q[k-j];
      for( k=0; k<j; k++ )            q[k] = 0;
      j = i % LENGTH;
      RSHIFT( q, j, SIZEPLUS );
      exp = Emin;
    }


/***	Adjust exponent						      ***/

  if( q[0] >= 0x00000002 ) exp++;
  if( ! q[0] ) exp--;
  exp = ( exp + Emax ) << EXPWPLUS;


/***	To produce the result in normal float point format and put    ***/
/***	the result into _q2					      ***/

  if( q[0] < 0x00000002 )
      { _q2[0] = ( q[1] >> EXPWPLUS ) | exp;
	for( i=1; i<SIZEPLUS; i++ )
	   { _q2[i] = ( q[i+1] >> EXPWPLUS ) + ( q[i] << EXPWPLUS );
	     stickybit |= q[i+SIZEPLUS];
	   }
	stickybit |= q[SIZEPLUS] << EXPWPLUS;
      }
  else
      { _q2[0] = (q[1] >> HOF) + ( ( q[0] & 0x00000001 ) << EXPW ) | exp;
	for( i=1; i<SIZEPLUS; i++ )
	   { _q2[i] = ( q[i+1] >> HOF ) + ( q[i] << EXPW );
	     stickybit |= q[i+SIZEPLUS];
	   }
	stickybit |= q[SIZEPLUS] << EXPW;
      }
      
#if DEBUG
 printf("4..");
 W(_q2);
#endif

#else
  /* Long double typedef */
  typedef struct {
	 unsigned int word1, word2, word3, word4;
  } long_double;

  long_double _qres, _U_Qfmpy();

  _qres = _U_Qfmpy(_q1,_q2);
  _q2[0] = _qres.word1;
  _q2[1] = _qres.word2;
  _q2[2] = _qres.word3;
  _q2[3] = _qres.word4;
#endif _QFMPY

  return;
}

