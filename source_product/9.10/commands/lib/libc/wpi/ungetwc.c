/* static char *HPUX_ID = "@(#) ungetwc.c $Revision: 70.2 $"; */

/*LINTLIBRARY*/


#include <sys/stdsyms.h>
#include <errno.h>
#include <nl_ctype.h>
#include <stdio.h>
#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef ungetwc
#    pragma _HP_SECONDARY_DEF _ungetwc ungetwc
#    define ungetwc _ungetwc

#    define ungetc _ungetc
#endif

/*****************************************************************************
 *                                                                           *
 *  PUSH BACK A WIDE CHARACTER ONTO A STREAM                                 *
 *                                                                           *
 *  INPUTS:      wc            - the character to push back                  *
 *                                                                           *
 *               stream        - the stream to push onto                     *
 *                                                                           *
 *  OUTPUTS:     the original character wc                                   *
 *               WEOF on error.                                              *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

wint_t ungetwc(wint_t wc, FILE *stream)
{

   unsigned char uc1, uc2;

   /* Check single byte locale and single byte data in multibyte locale first */
   if (__cs_SBYTE || (wc <= 0xff)) {
      /* Check for an invalid wide character in a single byte locale */
      if ((wc & 0xffffff00)) {
	 errno = EILSEQ;
	 return WEOF;
	 }
      if (ungetc((int)wc, stream) == EOF)
	 return WEOF;
      }
   else {		/* Multibyte */
      /* Check for an invalid wide character in a multi byte locale */
      if (wc & 0xffff0000) {
	 errno = EILSEQ;
	 return WEOF;
	 }

      /* Make sure first and second bytes are valid */
      uc1 = (unsigned char)(wc >> 8);
      uc2 = (unsigned char)(wc & 0x000000ff);
      if (!FIRSTof2(uc1) || !SECof2(uc2)) {
	 errno = EILSEQ;
	 return WEOF;
	 }

      /* Bytes okay; unget them in the proper order for re-reading */
      if (ungetc((int)uc2, stream) == EOF)
	 return WEOF;
      if (ungetc((int)uc1, stream) == EOF)
	 return WEOF;
      }

   return wc;
}
