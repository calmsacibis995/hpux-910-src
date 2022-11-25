/* "@(#) wpi.h $Revision: 70.1 $"; */


/*
 * This file contains the constants and macros needed to do fast
 * wide character classification and conversion for some multibyte
 * locales (to date: "japanese" and "japanese.euc").  These are 
 * temporary macros, and should not be used outside of libc code, as
 * they will be replaced in a future release with macros that provide
 * more general access to the data structures that setlocale() provides.
 */


#define	__W_U	0x0001		/* upper */
#define	__W_L	0x0002		/* lower */
#define	__W_N	0x0004		/* digit */
#define	__W_S	0x0008		/* space */
#define	__W_P	0x0010		/* punct */
#define	__W_C	0x0020		/* control */
#define	__W_B	0x0040		/* blank */
#define	__W_X	0x0080		/* hexdigit */
#define	__W_A	0x0100		/* alpha */
#define	__W_PR	0x0200		/* print */
#define	__W_G	0x0400		/* graph */

   /*
    *  WARNING: The following structure declarations are provided solely
    *  for use in the macros found in this header file.  These are HP-UX
    *  specific declarations used internally and direct use of these 
    *  declarations is not supported.  These declarations may be removed
    *  or significantly changed in future releases of HP-UX.
    */
   typedef struct {
      unsigned int		magic;
      unsigned int		cls_off;
      unsigned int		toL_off;
      unsigned int		toU_off;
      unsigned short int	lang_id;
      char			lang_name[3*14+2+1];	/* <l>_<t>.<c>'\0' */
      char			reserved[65];
      } _nl_mb_hdr_t;

   typedef struct {
      unsigned short int	rows;
      unsigned short int	cols;
      unsigned char		msb[256];
      unsigned char		lsb[256];
      unsigned short int	tab[1];
      } _nl_mb_data_t;

   extern _nl_mb_hdr_t	*_nl_mb_hdr;
   extern _nl_mb_data_t	*_nl_mb_cls;
   extern _nl_mb_data_t	*_nl_mb_toL;
   extern _nl_mb_data_t	*_nl_mb_toU;
   extern wint_t	_nl_wc, _nl_cwc;

      /*
       *  WARNING: The macro declaration "__mb_lookup" is provided solely
       *  for use in other macros found in this header file.  This is an
       *  HP-UX specific macro used internally and direct use of this macro
       *  is not supported.  This macro may be removed or significantly
       *  changed in future releases of HP-UX.
       */
      /* 
       *  Lookup extended multibyte classification and conversion information:
       *  - If extended tables available (pointer is not null), lookup
       *    property through indirect row and column indidices and return
       *    the value.
       *  - Otherwise return 0 (invalid wide character or WEOF).
       *  NOTE: This macro evaluates its arguments more than once, potentially
       *  producing side-effects.
       */
#     define __mb_lookup(Arg,Tbl) \
         (Tbl == 0 ? 0 : \
	 (Tbl->msb[(Arg >> 8) & 0xff] == 0xff) || \
	 (Tbl->lsb[Arg & 0xff] == 0xff) ? 0 : \
	 (Tbl->tab[Tbl->msb[(Arg >> 8) & 0xff] * Tbl->cols \
	    + Tbl->lsb[Arg & 0xff]]))

      /*
       * Wide character classification and conversion macros:
       *  - Copy argument to prevent side-effects.
       *  - If argument < 256, invoke single-byte version.
       *  - Otherwise return extended classification/conversion.
       */
#     define iswalnum(Arg) \
	 ((_nl_wc = (Arg)) < 256 ? isalnum((int)_nl_wc) : \
	 __mb_lookup(_nl_wc,_nl_mb_cls) & (__W_A|__W_N))
#     define iswalpha(Arg) \
	 ((_nl_wc = (Arg)) < 256 ? isalpha((int)_nl_wc) : \
	 __mb_lookup(_nl_wc,_nl_mb_cls) & __W_A)
#     define iswcntrl(Arg) \
	 ((_nl_wc = (Arg)) < 256 ? iscntrl((int)_nl_wc) : \
	 __mb_lookup(_nl_wc,_nl_mb_cls) & __W_C)
#     define iswdigit(Arg) \
	 ((_nl_wc = (Arg)) < 256 ? isdigit((int)_nl_wc) : \
	 __mb_lookup(_nl_wc,_nl_mb_cls) & __W_N)
#     define iswgraph(Arg) \
	 ((_nl_wc = (Arg)) < 256 ? isgraph((int)_nl_wc) : \
	 __mb_lookup(_nl_wc,_nl_mb_cls) & __W_G)
#     define iswlower(Arg) \
	 ((_nl_wc = (Arg)) < 256 ? islower((int)_nl_wc) : \
	 __mb_lookup(_nl_wc,_nl_mb_cls) & __W_L)
#     define iswprint(Arg) \
	 ((_nl_wc = (Arg)) < 256 ? isprint((int)_nl_wc) : \
	 __mb_lookup(_nl_wc,_nl_mb_cls) & __W_PR)
#     define iswpunct(Arg) \
	 ((_nl_wc = (Arg)) < 256 ? ispunct((int)_nl_wc) : \
	 __mb_lookup(_nl_wc,_nl_mb_cls) & __W_P)
#     define iswspace(Arg) \
	 ((_nl_wc = (Arg)) < 256 ? isspace((int)_nl_wc) : \
	 __mb_lookup(_nl_wc,_nl_mb_cls) & __W_S)
#     define iswupper(Arg) \
	 ((_nl_wc = (Arg)) < 256 ? isupper((int)_nl_wc) : \
	 __mb_lookup(_nl_wc,_nl_mb_cls) & __W_U)
#     define iswxdigit(Arg) \
	 ((_nl_wc = (Arg)) < 256 ? isxdigit((int)_nl_wc) : \
	 __mb_lookup(_nl_wc,_nl_mb_cls) & __W_X)

#     define towlower(Arg) \
	 ((_nl_wc = (Arg)) < 256 ? tolower((int)_nl_wc) : \
	 (_nl_cwc = __mb_lookup(_nl_wc,_nl_mb_toL), \
	 _nl_cwc == 0 ? _nl_wc : _nl_cwc))
#     define towupper(Arg) \
	 ((_nl_wc = (Arg)) < 256 ? toupper((int)_nl_wc) : \
	 (_nl_cwc = __mb_lookup(_nl_wc,_nl_mb_toU), \
	 _nl_cwc == 0 ? _nl_wc : _nl_cwc))
