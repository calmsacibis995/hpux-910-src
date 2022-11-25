/* static char *HPUX_ID = "@(#) fgetwc.c $Revision: 70.2 $"; */

/*LINTLIBRARY*/


#include <sys/stdsyms.h>
#include <errno.h>
#include <nl_ctype.h>
#include <stdio.h>
#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef fgetwc
#    pragma _HP_SECONDARY_DEF _fgetwc fgetwc
#    define fgetwc _fgetwc
#endif

/*****************************************************************************
 *                                                                           *
 *  GET WIDE CHARACTER FROM STREAM                                           *
 *                                                                           *
 *  INPUTS:      stream        - the stream to read from                     *
 *                                                                           *
 *  OUTPUTS:     the character read or WEOF if error                         *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

wint_t fgetwc(FILE *stream)
{

   wint_t wc;		/* return value */
   int second;		/* possible 2nd of multibyte char */

   /* Get 1st, possibly only character */
   if ((wc = (wint_t)getc(stream)) == EOF)
      return WEOF;

   /* Check for a second character.  __cs_SBYTE will be true only if a 
      single-byte encoding scheme is in use. */
   if (!__cs_SBYTE && FIRSTof2(wc)) {
      if ((second = (wint_t)getc(stream)) == EOF) {
	 errno = EILSEQ;	/* Invalid multibyte char */
	 ferror(stream);	/* Set error indication on stream */
	 return WEOF;		/* Need a second; not there */
	 }
      else if (SECof2(second)) {
	 return ((wc << 8) | second);	/* Combine and return */
	 }
      else {
	 errno = EILSEQ;	/* Invalid multibyte char */
	 ferror(stream);	/* Set error indication on stream */
	 return WEOF;		/* Second was invalid */
      }
   }
   return wc;
}
