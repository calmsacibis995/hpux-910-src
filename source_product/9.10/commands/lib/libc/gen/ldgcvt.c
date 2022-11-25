/*
 *  Long Double to Ascii conversion routine for g format.
 *  Written by Liz Sanville (December 1988 - January 1989).
 *
 *  NLS radix character support is present.
 */

#ifdef _NAMESPACE_CLEAN
#define strcpy  _strcpy
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "ld.h"

#define	NDIG    	350

#define FCVT		0
#define ECVT		1

extern char *_ldecvt();

extern int _nl_radix;

static int nl_rdxcvt(str, stdradix, nlsradix)
char *str;
unsigned int stdradix, nlsradix;
{
   unsigned int c;

   while ( *str ) {
      c = (char) *str;
      if ( c == stdradix ) *str = nlsradix;
      ++str;
   }
   return( 0 );
}

/* ldgcvt */
char *_ldgcvt(arg, ndigit, buf)
long_double arg;
int ndigit;
char *buf;
{
   int i;
   int decpt, sign;
   char *ubuffer = buf;
   char *ebuffer;

   ebuffer = _ldecvt( arg, ndigit, &decpt, &sign, ECVT );

   if ( ndigit <= 0 ) {
      strcpy( buf, ebuffer );
      return( buf );
   }

   if ( ebuffer[0] == 'N' || ebuffer[1] == 'I' ) {
      strcpy( ubuffer, ebuffer );
      return( buf);
   }

   if ( sign )
      *ubuffer++ = '-';

   if ( decpt > ndigit || decpt <= -4 ) {	/* E-style */
      decpt--;
      *ubuffer++ = *ebuffer++;
      *ubuffer++ = '.';
      for ( i = 1; i < ndigit; i++ )
	 *ubuffer++ = *ebuffer++;
      while ( ubuffer[-1] == '0' )	/* strip trailing zero's */
	 ubuffer--;
      if ( ubuffer[-1] == '.' )
	 ubuffer--;
      *ubuffer++ = 'e';
      if ( decpt < 0 ) {
	 decpt = -decpt;
	 *ubuffer++ = '-';
      } else
	 *ubuffer++ = '+';
      for ( i = 1000; i != 0; i /= 10 )
	 if ( i <= decpt || i <= 10 )	/* force 2 digits */
	    *ubuffer++ = (decpt / i) % 10 + '0';
   } else {					/* F-style */
      if ( decpt <= 0 ) {
	 *ubuffer++ = '0';
	 *ubuffer++ = '.';
	 while ( decpt < 0 ) {
	    decpt++;
	    *ubuffer++ = '0';
	 }
      }
      for ( i = 1; i <= ndigit; i++ ) {
	 *ubuffer++ = *ebuffer++;
	 if ( i == decpt )
	    *ubuffer++ = '.';
      }
      if ( ndigit < decpt ) {
	 while ( ndigit++ < decpt )
	    *ubuffer++ = '0';
	 *ubuffer++ = '.';
      }
      while ( *--ubuffer == '0' && ubuffer > buf )
	 ;				/* string trailing zero's */
      if ( *ubuffer != '.' )
	 ubuffer++;
   }
   *ubuffer = '\0';

   if ( _nl_radix != STDRADIX )
      nl_rdxcvt( buf, STDRADIX, _nl_radix );

   return( buf );
}
