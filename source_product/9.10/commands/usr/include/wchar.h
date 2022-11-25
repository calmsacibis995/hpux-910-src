/* "@(#) wchar.h $Revision: 70.11 $" */

#ifndef _WCHAR_INCLUDED
#  define _WCHAR_INCLUDED

#  include <ctype.h>      /* _U, _L, etc             */
#  include <stddef.h>     /* NULL, size_t, etc       */
#  include <stdio.h>      /* stdio prototypes        */
#  include <stdlib.h>     /* multibyte prototypes    */
#  include <time.h>       /* tm struct               */

#  ifndef _WCHAR_T
#     define _WCHAR_T
      typedef unsigned int wchar_t;
#  endif

   typedef unsigned int wint_t;
   typedef unsigned int wctype_t;

#  define WEOF  (wint_t)(-1)

#  ifndef _WPI_PROTOS
#     define _WPI_PROTOS
#     if defined(__STDC__) || defined(__cplusplus)
	 /* Wide String Manipulation */
         extern wchar_t       *wcscat(wchar_t *, const wchar_t *);
         extern wchar_t       *wcsncat(wchar_t *, const wchar_t *, size_t);
         extern wchar_t       *wcscpy(wchar_t *, const wchar_t *);
         extern wchar_t       *wcsncpy(wchar_t *, const wchar_t *, size_t);
         extern wchar_t       *wcschr(const wchar_t *, wint_t);
         extern wchar_t       *wcsrchr(const wchar_t *, wint_t);
         extern size_t        wcslen(const wchar_t *);
         extern size_t        wcsspn(const wchar_t *, const wchar_t *);
         extern size_t        wcscspn(const wchar_t *, const wchar_t *);
         extern int           wcscmp(const wchar_t *, const wchar_t *);
         extern int           wcsncmp(const wchar_t *, const wchar_t *, size_t);
         extern wchar_t       *wcspbrk(const wchar_t *, const wchar_t *);
         extern wchar_t       *wcswcs(const wchar_t *, const wchar_t *);
         extern wchar_t       *wcstok(wchar_t *, const wchar_t *);
	 extern int           wcscoll(const wchar_t *, const wchar_t *);
         extern size_t        wcsxfrm(wchar_t *, const wchar_t *, size_t);
         extern int           wcswidth(const wchar_t *, size_t);
         extern int           wcwidth(wint_t);

	 /* Wide Character Classification */
         extern int           iswalnum(wint_t);
         extern int           iswalpha(wint_t);
         extern int           iswcntrl(wint_t);
         extern int           iswdigit(wint_t);
         extern int           iswgraph(wint_t);
         extern int           iswlower(wint_t);
         extern int           iswprint(wint_t);
         extern int           iswpunct(wint_t);
         extern int           iswspace(wint_t);
         extern int           iswupper(wint_t);
         extern int           iswxdigit(wint_t);
         extern wint_t        towlower(wint_t);
         extern wint_t        towupper(wint_t);
         extern wctype_t      wctype(const char *);
         extern int           iswctype(wint_t, wctype_t);

	 /* Wide Character I/O */
         extern wint_t        fgetwc(FILE *);
         extern wchar_t       *fgetws(wchar_t *, int, FILE *);
         extern wint_t        fputwc(wint_t, FILE *);
         extern int           fputws(const wchar_t *, FILE *);
         extern wint_t        getwc(FILE *);
         extern wint_t        getwchar(void);
         extern wint_t        putwc(wint_t, FILE *);
         extern wint_t        putwchar(wint_t);
         extern wint_t        ungetwc(wint_t, FILE *);

	 /* Wide Character Numeric/Time Conversion */
         extern double        wcstod(const wchar_t *, wchar_t **);
         extern long int      wcstol(const wchar_t *, wchar_t **, int);
         extern unsigned long int wcstoul(const wchar_t *, wchar_t **, int);
         extern size_t        wcsftime(wchar_t *, size_t, const char *,
				       const struct tm *);
#     else  /* _STDC_ || __cplusplus */
	 /* Wide String Manipulation */
         extern wchar_t       *wcscat();
         extern wchar_t       *wcsncat();
         extern wchar_t       *wcscpy();
         extern wchar_t       *wcsncpy();
         extern wchar_t       *wcschr();
         extern wchar_t       *wcsrchr();
         extern size_t        wcslen();
         extern size_t        wcsspn();
         extern size_t        wcscspn();
         extern int           wcscmp();
         extern int           wcsncmp();
         extern wchar_t       *wcspbrk();
         extern wchar_t       *wcswcs();
         extern wchar_t       *wcstok();
	 extern int           wcscoll();
         extern size_t        wcsxfrm();
         extern int           wcswidth();
         extern int           wcwidth();

	 /* Wide Character Classification */
         extern int           iswalnum();
         extern int           iswalpha();
         extern int           iswcntrl();
         extern int           iswdigit();
         extern int           iswgraph();
         extern int           iswlower();
         extern int           iswprint();
         extern int           iswpunct();
         extern int           iswspace();
         extern int           iswupper();
         extern int           iswxdigit();
         extern wint_t        towlower();
         extern wint_t        towupper();
         extern wctype_t      wctype();
         extern int           iswctype();

	 /* Wide Character I/O */
         extern wint_t        fgetwc();
         extern wchar_t       *fgetws();
         extern wint_t        fputwc();
         extern int           fputws();
         extern wint_t        getwc();
         extern wint_t        getwchar();
         extern wint_t        putwc();
         extern wint_t        putwchar();
         extern wint_t        ungetwc();

	 /* Wide Character Numeric/Time Conversion */
         extern double        wcstod();
         extern long int      wcstol();
         extern unsigned long int wcstoul();
         extern size_t        wcsftime();
#     endif /* _STDC_ || __cplusplus */
#  endif /* _WPI_PROTOS */

#  ifndef __lint
#     define getwc(stream)		fgetwc(stream)
#     define getwchar()			getwc(stdin)
#     define putwc(wc,stream)		fputwc(wc,stream)
#     define putwchar(wc)		putwc(wc,stdout)
#  endif /* __lint */

#endif  /* _WCHAR_INCLUDED */
