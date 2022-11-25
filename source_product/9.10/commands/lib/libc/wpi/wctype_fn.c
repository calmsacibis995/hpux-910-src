/* static char *HPUX_ID = "@(#) wctype_fn.c $Revision: 70.6 $"; */

/*LINTLIBRARY*/


#include <nl_ctype.h>
#include <wchar.h>
#include <wpi.h>

/*****************************************************************************
 *                                                                           *
 *  WIDE CHARACTER CLASSIFICATION AND CONVERSION FUNTCIONS                   *
 *                                                                           *
 *  INPUTS:      wc            - the character to classify/convert           *
 *                                                                           *
 *  OUTPUTS:     TRUE/FALSE for classification functions;                    *
 *               the converted wide character for conversion functions       *
 *                                                                           *
 *  AUTHOR:      Jim Stratton                                                *
 *                                                                           *
 *  DEFECTS:                                                                 *
 *                                                                           *
 *  CHANGELOG:                                                               *
 *                                                                           *
 *****************************************************************************/

/* iswctype() must appear first in this file, so that we invoke the macro
   versions of the classification routines. */

#ifdef _NAMESPACE_CLEAN
#    undef iswctype
#    pragma _HP_SECONDARY_DEF _iswctype iswctype
#    define iswctype _iswctype
#endif


/* The following #ifdef facilitates testing iswctype() with function versions  
   of the isw() routines. Normally we want the macro versions for performance.
*/
#ifdef NO_MACRO
#  undef iswalnum
#  undef iswalpha
#  undef iswcntrl
#  undef iswdigit
#  undef iswgraph
#  undef iswlower
#  undef iswprint
#  undef iswpunct
#  undef iswspace
#  undef iswupper
#  undef iswxdigit
#  undef towlower
#  undef towupper
#endif


/*****************************************************************************
 *****************************************************************************/

static int iswblank(wint_t wc);		/* Forward declaration */

int iswctype(wint_t wc, wctype_t wc_prop)

{
   switch (wc_prop) {
      /* The case selection numbers are returned by wctype() */
      case 1	: return iswalnum(wc);
      case 2	: return iswalpha(wc);
      case 3	: return iswblank(wc);
      case 4	: return iswcntrl(wc);
      case 5	: return iswdigit(wc);
      case 6	: return iswgraph(wc);
      case 7	: return iswlower(wc);
      case 8	: return iswprint(wc);
      case 9	: return iswpunct(wc);
      case 10	: return iswspace(wc);
      case 11	: return iswupper(wc);
      case 12	: return iswxdigit(wc);
      default   : return 0;
   }
}




/* --- undef these so that we don't pick up the macros of the same name ----
   --- defined in wchar.h.  Those would interfere with the function --------
   --- definitions below. -------------------------------------------------- */

#undef iswalnum
#undef iswalpha
#undef iswcntrl
#undef iswdigit
#undef iswgraph
#undef iswlower
#undef iswprint
#undef iswpunct
#undef iswspace
#undef iswupper
#undef iswxdigit
#undef towlower
#undef towupper

#ifdef _NAMESPACE_CLEAN

#   pragma _HP_SECONDARY_DEF _iswalnum  iswalnum
#   pragma _HP_SECONDARY_DEF _iswalpha  iswalpha
#   pragma _HP_SECONDARY_DEF _iswcntrl  iswcntrl
#   pragma _HP_SECONDARY_DEF _iswdigit  iswdigit
#   pragma _HP_SECONDARY_DEF _iswgraph  iswgraph
#   pragma _HP_SECONDARY_DEF _iswlower  iswlower
#   pragma _HP_SECONDARY_DEF _iswprint  iswprint
#   pragma _HP_SECONDARY_DEF _iswpunct  iswpunct
#   pragma _HP_SECONDARY_DEF _iswspace  iswspace
#   pragma _HP_SECONDARY_DEF _iswupper  iswupper
#   pragma _HP_SECONDARY_DEF _iswxdigit iswxdigit
#   pragma _HP_SECONDARY_DEF _towlower  towlower
#   pragma _HP_SECONDARY_DEF _towupper  towupper

#   define iswalnum  _iswalnum
#   define iswalpha  _iswalpha
#   define iswcntrl  _iswcntrl
#   define iswdigit  _iswdigit
#   define iswgraph  _iswgraph
#   define iswlower  _iswlower
#   define iswprint  _iswprint
#   define iswpunct  _iswpunct
#   define iswspace  _iswspace
#   define iswupper  _iswupper
#   define iswxdigit _iswxdigit
#   define towlower  _towlower
#   define towupper  _towupper

#endif


/*****************************************************************************
 *****************************************************************************/

int iswalnum(wint_t wc)

{
    return (wc < 256 ? isalnum((int)wc) :
       __mb_lookup(wc,_nl_mb_cls) & (__W_A|__W_N));
}


/*****************************************************************************
 *****************************************************************************/

int iswalpha(wint_t wc)

{
    return (wc < 256 ? isalpha((int)wc) : __mb_lookup(wc,_nl_mb_cls) & __W_A);
}


/*****************************************************************************
 *****************************************************************************/

static int iswblank(wint_t wc)		/* not standard, so not visible */

{
    /* Note: there is no isblank(), so ctype table is accessed directly */
    return (wc < 256 ? __ctype[(int)wc]&_B :
       __mb_lookup(wc,_nl_mb_cls) & __W_B);
}


/*****************************************************************************
 *****************************************************************************/

int iswcntrl(wint_t wc)

{
    return (wc < 256 ? iscntrl((int)wc) : __mb_lookup(wc,_nl_mb_cls) & __W_C);
}


/*****************************************************************************
 *****************************************************************************/

int iswdigit(wint_t wc)

{
    return (wc < 256 ? isdigit((int)wc) : __mb_lookup(wc,_nl_mb_cls) & __W_N);
}


/*****************************************************************************
 *****************************************************************************/

int iswgraph(wint_t wc)

{
    return (wc < 256 ? isgraph((int)wc) : __mb_lookup(wc,_nl_mb_cls) & __W_G);
}


/*****************************************************************************
 *****************************************************************************/

int iswlower(wint_t wc)

{
    return (wc < 256 ? islower((int)wc) : __mb_lookup(wc,_nl_mb_cls) & __W_L);
}


/*****************************************************************************
 *****************************************************************************/

int iswprint(wint_t wc)

{
    return (wc < 256 ? isprint((int)wc) : __mb_lookup(wc,_nl_mb_cls) & __W_PR);
}


/*****************************************************************************
 *****************************************************************************/

int iswpunct(wint_t wc)

{
    return (wc < 256 ? ispunct((int)wc) : __mb_lookup(wc,_nl_mb_cls) & __W_P);
}


/*****************************************************************************
 *****************************************************************************/

int iswspace(wint_t wc)

{
    return (wc < 256 ? isspace((int)wc) : __mb_lookup(wc,_nl_mb_cls) & __W_S);
}


/*****************************************************************************
 *****************************************************************************/

int iswupper(wint_t wc)

{
    return (wc < 256 ? isupper((int)wc) : __mb_lookup(wc,_nl_mb_cls) & __W_U);
}


/*****************************************************************************
 *****************************************************************************/

int iswxdigit(wint_t wc)

{
    return (wc < 256 ? isxdigit((int)wc) : __mb_lookup(wc,_nl_mb_cls) & __W_X);
}


/*****************************************************************************
 *****************************************************************************/

wint_t towlower(wint_t wc)

{
   wint_t cvt_wc;

   if (wc >= 0 && wc <= 255)
      return _tolower((int)wc);
   else if ((cvt_wc = __mb_lookup(wc,_nl_mb_toL)) == 0)
      return wc;
   else
      return cvt_wc;
}


/*****************************************************************************
 *****************************************************************************/

wint_t towupper(wint_t wc)

{
   wint_t cvt_wc;

   if (wc >= 0 && wc <= 255)
      return _toupper((int)wc);
   else if ((cvt_wc = __mb_lookup(wc,_nl_mb_toU)) == 0)
      return wc;
   else
      return cvt_wc;
}
