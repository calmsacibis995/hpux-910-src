/*
 *  Ascii to Long Double conversion routine.
 *  Written by Liz Sanville (December 1988).
 *
 *  cvtnum calls _qfsn which converts the ascii buffer to a long double.
 *  NLS radix character support is present.
 */

#ifdef _NAMESPACE_CLEAN
#undef strtold
#pragma _HP_SECONDARY_DEF _strtold strtold
#define strtold _strtold
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "ld.h"

#define OVERFLOW	0x20000000
#define UNDERFLOW	0x10000000
#define INEXACT		0x08000000

/* This definition should reside somewhere - WORK NEEDED */
static long_double  _MAXLDBL = { 0x7ffeffff, 0xffffffff, 0xffffffff, 0xffffffff };
static long_double _NMAXLDBL = { 0xfffeffff, 0xffffffff, 0xffffffff, 0xffffffff };
static long_double     _ZERO = { 0, 0, 0, 0 };

extern int _nl_radix;
char *nl_rdxcvt();
long_double cvtnum();

static char *nl_rdxcvt(str, nlsradix, stdradix)
char *str;
unsigned int nlsradix, stdradix;
{
   unsigned int c;

   while ( *str ) {
      c = (char) *str;
      if ( c == nlsradix ) {
	 *str = stdradix;
	 return( str );
      } else if ( c == stdradix ) {
	 *str = '!';
	 return( str );
      }
      ++str;
   }
   return( (char *)0 );
}

long_double _atold(str)
char *str;
{
   return( cvtnum( str, (unsigned char **)0 ) );
}

long_double strtold(str, endptr)
char *str, **endptr;
{
   return( cvtnum( str, endptr ) );
}


static long_double cvtnum(str, endptr)
char *str, **endptr;
{
   int err, status;
   static long_double ld;
   char *converted;
  
   /* convert radix to standard radix '.' */
   if ( _nl_radix == STDRADIX ) converted = (char *)0;
   else converted = nl_rdxcvt( str, _nl_radix, STDRADIX );

   status = 0;	
   err = _qfsn( str, &ld, &status, ANSIC, endptr );

   /*   convert standard radix back to NLS radix  */
   /* also convert any '!' back to standard radix */
   if ( converted )
      if ( *converted == STDRADIX ) *converted = _nl_radix;
      else *converted = STDRADIX;

   if ( status != 0 )
      if ( status != INEXACT ) {
#ifdef DEBUG
	 fprintf(stderr,"ERROR:  str = %s, status = %x, err = %d\n",
		 str, status, err);
#endif DEBUG
	 if ( status & OVERFLOW ) {
	    int i;
	    errno = ERANGE;
	    for ( i = -1; isspace(str[++i]); )
	       ;
	    if ( str[i] == '-' ) return( _NMAXLDBL );
	    else return( _MAXLDBL );
	 } else if ( status & UNDERFLOW ) {
	    errno = ERANGE;
	    return( _ZERO );
	 }
      }

   return( ld );
}
