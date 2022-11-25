/*
 *  Long Double to Ascii conversion routines for e and f format.
 *  Written by Liz Sanville (December 1988 - January 1989).
 *
 *  ldcvt calls _qfns to do the long double to ascii conversion.
 *  _qfns always (*) returns an ascii buffer formatted as follows:
 *    [ -]0.ddddddddddddddddddddddddddddddddddE[-]?ddddd
 *  In english,
 *    a blank or minus followed by 0., 34 digits, E, an optional minus
 *    and a maximum of 5 digits.
 *  (*) Zero returns [ -]0.
 *      Not-a-Number returns Nan.
 *	Infinity returns [+-] INFINITY.
 */

#ifdef _NAMESPACE_CLEAN
#define strcpy  _strcpy
#define strncpy _strncpy
#define atoi    _atoi
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "ld.h"

#define	NDIG    	350
#define MAXQSIZ		34
#define NLEAD		3
#define NOCHANGE	-99

#define FCVT		0
#define ECVT		1

static	char 	lbuffer[NDIG];	/* long double buffer */
static	char 	ubuffer[NDIG];	/* user buffer */

static	char 	*ldcvt();


/* ldecvt */
char *_ldecvt(arg, ndigit, decpt, sign)
long_double arg;
int ndigit, *decpt, *sign;
{
   return( ldcvt( arg, ndigit, decpt, sign, ECVT ) );
}


/* ldfcvt */
char *_ldfcvt(arg, ndigit, decpt, sign)
long_double arg;
int ndigit, *decpt, *sign;
{
   return( ldcvt( arg, ndigit, decpt, sign, FCVT ) );
}


/* ldcvt */
static char *ldcvt(arg, ndigit, decpt, sign, eflag)
long_double arg;
int ndigit, *decpt, *sign, eflag;
{
   int status, err;
   int digit, i, j, spos;

#ifdef DEBUG
   /* The following should be removed after testing. 
    * Being used to insure random zero's in buffer not masking bugs.
    */
   for ( i = 0; i < NDIG; i++ )
      lbuffer[i] = ubuffer[i] = '*';
#endif DEBUG

   *decpt = 0;
   *sign = 0;

   status = 0;
   err = _qfns( &arg, lbuffer, &status );

   if ( lbuffer[0] == 'N' || lbuffer[1] == 'I' ) {
      /* NaN or +/-INFINITY */
      strcpy( ubuffer, lbuffer );
      return( ubuffer );
   }

   if ( status != 0 )
      if ( status != 0x08000000 ) {	/* ignore INEXACT status */
#ifdef DEBUG
	  fprintf(stderr,"ERROR:  num = %x %x %x %x, status = %x, err = %d\n",
		  arg.word1, arg.word2, arg.word3, arg.word4, status, err);
#endif DEBUG
	  for ( i = 0; i < ndigit; i++ ) ubuffer[i] = '0';
	  ubuffer[ndigit] = '\0';
	  return(ubuffer);
      }

   /* move this after check for zero to ignore sign of 0 (+/-0) */
   if ( lbuffer[0] == ' ' ) *sign = 0;	/* positive */
   else *sign = 1;			/* negative */

   if ( lbuffer[1] == '0' && lbuffer[2] == '.' && lbuffer[3] == '\0' ) {
      for ( i = 0; i < ndigit; i++ )
	 ubuffer[i] = '0';
      ubuffer[i] = '\0';
      return( ubuffer );
   }

   *decpt = atoi(&lbuffer[MAXQSIZ+4]);

   if ( eflag ) digit = ndigit;
   else digit = *decpt + ndigit;
   /*
   if ( digit >= NDIG - 1 ) digit = (ndigit<NDIG-2) ? ndigit : NDIG - 2;
   */
   if ( digit >= NDIG - 1 ) digit = NDIG - 2;
   if ( digit < 0 ) digit = 0;
   if ( digit >= MAXQSIZ )
      strncpy( ubuffer, &lbuffer[NLEAD], MAXQSIZ );	/* copy 34 digit */
      						 /* pad with zeros later */
   else {
      /* round( buffer, lead, ndigs )
       *   returns one less than position of last digit changed 
       *   (-99 if no change)
       */
      spos = 0;
      i = round( lbuffer, NLEAD, digit );
      if ( i == NLEAD-1 ) {
	 ubuffer[0] = '1';
	 *decpt += 1;
	 spos = 1;
      }
      strncpy( &ubuffer[spos], &lbuffer[NLEAD], digit ); /* copy n digit */
   }
   for ( i=digit-MAXQSIZ, j=MAXQSIZ; i > 0; i--, j++ )
      ubuffer[j] = '0';				/* pad with zeros */
   ubuffer[digit] = '\0';

   return( ubuffer );
}


static int round(buffer, lead, ndigs)
char *buffer;
int lead, ndigs;
{
   int i;

   if ( buffer[lead+ndigs] >= '5' ) {
      for ( i = lead+ndigs-1; i >= lead; i-- )
	 if ( buffer[i] != '9' ) {
	    buffer[i] += 1;
	    break;
	 } else buffer[i] = '0';
      return( i );
   }
   return( NOCHANGE );
}
