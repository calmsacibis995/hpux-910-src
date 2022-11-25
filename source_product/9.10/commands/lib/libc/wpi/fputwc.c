/* static char *HPUX_ID = "@(#) fputwc.c $Revision: 70.3 $"; */

/*LINTLIBRARY*/


#include <sys/stdsyms.h>
#include <errno.h>
#include <nl_ctype.h>
#include <stdio.h>
#include <wchar.h>

#ifdef _NAMESPACE_CLEAN
#    undef fputwc
#    pragma _HP_SECONDARY_DEF _fputwc fputwc
#    define fputwc _fputwc
#endif

/*****************************************************************************
 *                                                                           *
 *  PUT WIDE CHARACTER ON STREAM                                             *
 *                                                                           *
 *  INPUTS:      wc            - the character to put out                    *
 *                                                                           *
 *               stream        - the stream to write on                      *
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

wint_t fputwc(wint_t wc, FILE *stream)

{
   unsigned char uc1, uc2;

   /* Check single byte locale and single byte data in multibyte locale first */
   if (__cs_SBYTE || (wc <= 0xff)) {
      /* Check for an invalid wide character in a single byte locale */
      if ((wc & 0xffffff00)) {
	 errno = EILSEQ;
	 return WEOF;
	 }
      if (putc((int)wc, stream) == EOF)
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
      
      /* Bytes okay; write them out */
      if (putc((int)uc1, stream) == EOF)
	 return WEOF;
      if (putc((int)uc2, stream) == EOF)
	 return WEOF;
      }

   return wc;
}
