/* @(#) $Revision: 66.9 $ */      

#ifdef EUC
#ifndef _NL_CTYPE_INCLUDED
#define _NL_CTYPE_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_XOPEN_SOURCE

/*LINTLIBRARY*/

		/* Code Set Independent multibyte tools */

#ifdef _NAMESPACE_CLEAN
extern unsigned char *__1kanji;			/* ptr to 1st of 2 HP15 table */
extern unsigned char *__2kanji;			/* ptr to 2nd of 2 HP15 table */
#else /* not _NAMESPACE_CLEAN */
extern unsigned char *_1kanji;			/* ptr to 1st of 2 HP15 table */
extern unsigned char *_2kanji;			/* ptr to 2nd of 2 HP15 table */
#endif /* else not _NAMESPACE_CLEAN */

extern int __cs_SBYTE;				/* sigle-byte code scheme */
extern int __cs_HP15;				/* HP15 code scheme */
extern int __cs_EUC;				/* EUC code scheme */
extern unsigned char __out_csize[];		/* out_csize table */
extern unsigned char *__ein_csize;		/* expanded in_csize table */
extern unsigned char *__eout_csize;		/* expanded out_csize table */

#ifndef _WCHAR_T
#  define _WCHAR_T
   typedef unsigned int wchar_t;
#endif

/*
 * WARNING:  The following function declarations are provided solely
 *  for use in the macros found in this header file.  These are HP-UX
 *  specific routines used internally and direct use of these routines
 *  is not supported.  These routines may be removed or significantly 
 *  changed in future releases of HP-UX.
 */
#if defined(__STDC__) || defined(__cplusplus)
   extern unsigned char __euc_cs(wchar_t);
   extern wchar_t __get_euc(const unsigned char *);
   extern wchar_t __put_euc(wchar_t, unsigned char *);
   extern unsigned char *__put_adv_euc(wchar_t, unsigned char *);
#else /* not __STDC__ || __cplusplus */
   extern unsigned char __euc_cs();
   extern wchar_t __get_euc();
   extern wchar_t __put_euc();
   extern unsigned char *__put_adv_euc();	/* advance pointer for EUC */
#endif /* __STDC__ || __cplusplus */

#define __UCHAR(c)	((unsigned char)(c))
#define __UCHAR_P(p)	((unsigned char *)(p))

#define C_MBLEN(c)	(*(__ein_csize+(__UCHAR(c))))

#define C_COLWIDTH(c)	(*(__eout_csize+(__UCHAR(c))))

#define WC_COLWIDTH(wc)							\
    (__cs_SBYTE								\
	? 1								\
	: __cs_HP15							\
	    ? ((wc) & 0xff00)						\
		? (*(__eout_csize+((unsigned char) ((wc)>>8))))		\
		: 1							\
	    : (*(__out_csize+(__euc_cs(wc)))))

#define ADVANCE(p)							\
    (__cs_SBYTE								\
	? ((p)++)							\
	: __cs_HP15							\
	    ? ((p) += ((FIRSTof2(*(p))) ? 2 : 1))			\
	    : ((p) += C_MBLEN(*(p))))

#define CHARAT(p)							\
    (__cs_SBYTE								\
	? (unsigned char)(p)[0]						\
	: __cs_HP15							\
	    ? ((FIRSTof2((p)[0]))					\
		? ((unsigned short) (((p)[0]<<8)			\
		  | (unsigned char) (p)[1]))				\
		: ((unsigned char) (p)[0]))				\
	    : __get_euc(p))

#define _CHARAT(p)							\
    (__cs_SBYTE								\
	? (unsigned char)(p)[0]						\
	: ((FIRSTof2((p)[0]))						\
		? ((unsigned short) (((p)[0]<<8)			\
		  | (unsigned char) (p)[1]))				\
		: ((unsigned char) (p)[0])))

#define CHARADV(p)							\
    (__cs_SBYTE								\
	? (unsigned char)*(p)++						\
	: __cs_HP15							\
	    ? ((FIRSTof2((p)[0]))					\
		? (p) += 2, ((unsigned short) (((p)[-2]<<8)		\
		  | (unsigned char) (p)[-1]))				\
		: ((unsigned char)*(p)++))				\
	    : ( (C_MBLEN((p)[0]) == 1)					\
		? ((p)+=1, __get_euc((p)-1))				\
		: ((p)+=2, __get_euc((p)-2)) ) )

#define _CHARADV(p)							\
    (__cs_SBYTE								\
	? (unsigned char)*(p)++						\
	: ((FIRSTof2((p)[0]))						\
		? (p) += 2, ((unsigned short) (((p)[-2]<<8)		\
		  | (unsigned char) (p)[-1]))				\
		: ((unsigned char)*(p)++)))

#define WCHAR(c, p)							\
    (__cs_SBYTE								\
	? ((unsigned char) ((p)[0] = (unsigned char)(c)))		\
	: __cs_HP15							\
	    ? ((c) & 0xff00)						\
		? ((p)[0] = (unsigned char) ((c)>>8),			\
	 	  (p)[1] = (unsigned char) (c), (unsigned short) (c))	\
		: ((unsigned char) ((p)[0] = (unsigned char)(c)))	\
	    : __put_euc(c, p))

#define _WCHAR(c, p)							\
    (__cs_SBYTE								\
	? ((unsigned char) ((p)[0] = (unsigned char)(c)))		\
	: ((c) & 0xff00)						\
		? ((p)[0] = (unsigned char) ((c)>>8),			\
	 	  (p)[1] = (unsigned char) (c), (unsigned short) (c))	\
		: ((unsigned char) ((p)[0] = (unsigned char)(c))) )

#define WCHARADV(c, p)							\
    (__cs_SBYTE								\
	? ((unsigned char) (*(p)++ = (unsigned char)(c)))		\
	: __cs_HP15							\
	    ? ((c) & 0xff00)						\
		? (*(p)++ = (unsigned char) ((c)>>8),			\
	 	  *(p)++ = (unsigned char) (c), (unsigned short) (c))	\
		: ((unsigned char) (*(p)++ = (unsigned char)(c)))	\
	    : ( (p) = __put_adv_euc(c, p), c ) )

#define _WCHARADV(c, p)							\
    (__cs_SBYTE								\
	? ((unsigned char) (*(p)++ = (unsigned char)(c)))		\
	: ((c) & 0xff00)						\
		? (*(p)++ = (unsigned char) ((c)>>8),			\
	 	  *(p)++ = (unsigned char) (c), (unsigned short) (c))	\
		: ((unsigned char) (*(p)++ = (unsigned char)(c))) )


/*
 *  The following definitions are all obsolete.
 */

/* PCHAR, PCHARADV are obsolete; use WCHAR and WCHARADV */

#define PCHAR(c, p)	{ 						\
			    if ((c) & 0xff00) { 			\
				(p)[0] = (unsigned char) ((c)>>8); 	\
				(p)[1] = (unsigned char) (c);		\
			    }						\
			    else 					\
				(p)[0] = (unsigned char) (c);		\
			}

#define PCHARADV(c, p)	{ 						\
			    if ((c) & 0xff00) 				\
				*(p)++ = (unsigned char) ((c)>>8); 	\
			    *(p)++ = (unsigned char) (c);		\
			}						\

#include	<ctype.h>

#define	LC_CTYPE	2

#if defined(__STDC__) || defined(__cplusplus)
  extern char *idtolang(int);
#else /* not __STDC__ || __cplusplus */
  extern char *idtolang();
#endif /* else not __STDC__ || __cplusplus */

extern int __nl_langid[];
#ifdef _NAMESPACE_CLEAN
extern int __nl_failid;
#else /* not _NAMESPACE_CLEAN */
extern int _nl_failid;
#endif /* else not _NAMESPACE_CLEAN */

/*
** End result of nl_isstar() is to always execute isstar().
** But if the requested langid is not loaded and we didn't fail
** in trying to load it previously, then call setlocale to load it.
** If the language doesn't exist (idtolang returning an empty string)
** or setlocale can't load the language, then record this langid as failed.
*/
#ifdef _NAMESPACE_CLEAN
#define nl_isstar(c,langid,isstar)	(langid == __nl_langid[LC_CTYPE] \
					  ? isstar(c) \
					  : langid == __nl_failid \
					     ? isstar(c) \
					     : (strcmp(idtolang(langid),"")) \
						? (setlocale(LC_CTYPE,idtolang(langid))) \
						   ? isstar(c) \
						   : (__nl_failid = langid, isstar(c)) \
						: (__nl_failid = langid, setlocale(LC_CTYPE,"C"), isstar(c)))
#else /* not _NAMESPACE_CLEAN */
#define nl_isstar(c,langid,isstar)	(langid == __nl_langid[LC_CTYPE] \
					  ? isstar(c) \
					  : langid == _nl_failid \
					     ? isstar(c) \
					     : (strcmp(idtolang(langid),"")) \
						? (setlocale(LC_CTYPE,idtolang(langid))) \
						   ? isstar(c) \
						   : (_nl_failid = langid, isstar(c)) \
						: (_nl_failid = langid, setlocale(LC_CTYPE,"C"), isstar(c)))
#endif /* else not _NAMESPACE_CLEAN */

#define nl_isalpha(c,langid)	nl_isstar(c,langid,isalpha)
#define nl_isupper(c,langid)	nl_isstar(c,langid,isupper)
#define nl_islower(c,langid)	nl_isstar(c,langid,islower)
#define nl_isdigit(c,langid)	nl_isstar(c,langid,isdigit)
#define nl_isxdigit(c,langid)	nl_isstar(c,langid,isxdigit)
#define nl_isalnum(c,langid)	nl_isstar(c,langid,isalnum)
#define nl_isspace(c,langid)	nl_isstar(c,langid,isspace)
#define nl_ispunct(c,langid)	nl_isstar(c,langid,ispunct)
#define nl_isprint(c,langid)	nl_isstar(c,langid,isprint)
#define nl_isgraph(c,langid)	nl_isstar(c,langid,isgraph)
#define nl_iscntrl(c,langid)	nl_isstar(c,langid,iscntrl)

#define _K1 	01
#define _K2	02

#define __UCHAR_EOF(c)	((sizeof(c)) == 1 ? (unsigned char)(c) : (c))
#ifdef _NAMESPACE_CLEAN
#define FIRSTof2(c)	(*(__1kanji+(__UCHAR_EOF(c))))
#define _SECof2(c)	(*(__2kanji+(__UCHAR_EOF(c))))
extern __sec_tab[4];
#define SECof2(c) (__sec_tab[_SECof2(c)])
extern __status_tab [][4];
#define BYTE_STATUS(byte,laststatus)					\
			(__status_tab[(laststatus)][_SECof2(byte)])
#else /* not _NAMESPACE_CLEAN */
#define FIRSTof2(c)	(*(_1kanji+(__UCHAR_EOF(c))))
#define _SECof2(c)	(*(_2kanji+(__UCHAR_EOF(c))))
extern _sec_tab[4];
#define SECof2(c)	(_sec_tab[_SECof2(c)])
extern _status_tab [][4];
#define BYTE_STATUS(byte,laststatus)					\
			(_status_tab[(laststatus)][_SECof2(byte)])
#endif /* else not _NAMESPACE_CLEAN */

#define ONEBYTE		0
#define SECOF2		1
#define FIRSTOF2	2

#define _CS_SBYTE	(__cs_SBYTE)

#if defined(__STDC__) || defined(__cplusplus)
  extern int firstof2(int);
  extern int secof2(int);
  extern int byte_status(int, int);
  extern int c_colwidth(int);
#else /* not __STDC__ || __cplusplus */
  extern int firstof2();
  extern int secof2();
  extern int byte_status();
  extern int c_colwidth();
#endif /* else not __STDC__ || __cplusplus */

#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _NL_CTYPE_INCLUDED */


#else /* EUC */


#ifndef _NL_CTYPE_INCLUDED
#define _NL_CTYPE_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_XOPEN_SOURCE

/*LINTLIBRARY*/

#include	<ctype.h>

#define	LC_CTYPE	2

#if defined(__STDC__) || defined(__cplusplus)
  extern char *idtolang(int);
#else /* not __STDC__ || __cplusplus */
  extern char *idtolang();
#endif /* else not __STDC__ || __cplusplus */

extern int __nl_langid[];
#ifdef _NAMESPACE_CLEAN
extern int __nl_failid;
#else /* not _NAMESPACE_CLEAN */
extern int _nl_failid;
#endif /* else not _NAMESPACE_CLEAN */

/*
** End result of nl_isstar() is to always execute isstar().
** But if the requested langid is not loaded and we didn't fail
** in trying to load it previously, then call setlocale to load it.
** If the language doesn't exist (idtolang returning an empty string)
** or setlocale can't load the language, then record this langid as failed.
*/
#ifdef _NAMESPACE_CLEAN
#define nl_isstar(c,langid,isstar)	(langid == __nl_langid[LC_CTYPE] \
					  ? isstar(c) \
					  : langid == __nl_failid \
					     ? isstar(c) \
					     : (strcmp(idtolang(langid),"")) \
						? (setlocale(LC_CTYPE,idtolang(langid))) \
						   ? isstar(c) \
						   : (__nl_failid = langid, isstar(c)) \
						: (__nl_failid = langid, setlocale(LC_CTYPE,"C"), isstar(c)))
#else /* not _NAMESPACE_CLEAN */
#define nl_isstar(c,langid,isstar)	(langid == __nl_langid[LC_CTYPE] \
					  ? isstar(c) \
					  : langid == _nl_failid \
					     ? isstar(c) \
					     : (strcmp(idtolang(langid),"")) \
						? (setlocale(LC_CTYPE,idtolang(langid))) \
						   ? isstar(c) \
						   : (_nl_failid = langid, isstar(c)) \
						: (_nl_failid = langid, setlocale(LC_CTYPE,"C"), isstar(c)))
#endif /* else not _NAMESPACE_CLEAN */

#define nl_isalpha(c,langid)	nl_isstar(c,langid,isalpha)
#define nl_isupper(c,langid)	nl_isstar(c,langid,isupper)
#define nl_islower(c,langid)	nl_isstar(c,langid,islower)
#define nl_isdigit(c,langid)	nl_isstar(c,langid,isdigit)
#define nl_isxdigit(c,langid)	nl_isstar(c,langid,isxdigit)
#define nl_isalnum(c,langid)	nl_isstar(c,langid,isalnum)
#define nl_isspace(c,langid)	nl_isstar(c,langid,isspace)
#define nl_ispunct(c,langid)	nl_isstar(c,langid,ispunct)
#define nl_isprint(c,langid)	nl_isstar(c,langid,isprint)
#define nl_isgraph(c,langid)	nl_isstar(c,langid,isgraph)
#define nl_iscntrl(c,langid)	nl_isstar(c,langid,iscntrl)

		/* HP-15 tools */

#ifdef _NAMESPACE_CLEAN
extern unsigned char *__1kanji;		/* ptr to 1st of 2 kanji table	*/
extern unsigned char *__2kanji;		/* ptr to 2nd of 2 kanji table	*/
#else /* not _NAMESPACE_CLEAN */
extern unsigned char *_1kanji;		/* ptr to 1st of 2 kanji table	*/
extern unsigned char *_2kanji;		/* ptr to 2nd of 2 kanji table	*/
#endif /* else not _NAMESPACE_CLEAN */
#define _K1			01
#define _K2			02

#ifdef _NAMESPACE_CLEAN
#define FIRSTof2(x)  (*(__1kanji+(x)))
#define SECof2(x)    (*(__2kanji+(x)))
#else /* not _NAMESPACE_CLEAN */
#define FIRSTof2(x)  (*(_1kanji+(x)))
#define SECof2(x)    (*(_2kanji+(x)))
#endif /* else not _NAMESPACE_CLEAN */

/*
**  Note:  BYTE_STATUS depends upon SECof2(c) returning:
**
**             0, if c can be neither 1st nor 2nd byte of a kanji
**                << '1' is never returned >>
**             2, if c could be the 2nd but not the 1st byte of a kanji
**             3, if c could be either the 1st or the 2nd byte of a kanji
**
**  This is accomplished by having both the _K1 and the _K2 bit flags
**  (which must be '01' and '02' respectively) present in the _2kanji
**  table.  To guarantee that SECof2(c) returns true (ie. non-zero) only
**  if c could be a 2nd of 2, requires that '1st of 2' bytes be a subset
**  of '2nd of 2' bytes.
*/  

#define ONEBYTE 0
#define SECOF2  1
#define FIRSTOF2 2

extern int __nl_char_size;
#define _CS_SBYTE	(__nl_char_size == 1)

#ifdef _NAMESPACE_CLEAN
#define BYTE_STATUS(byte,laststatus) 	\
			(__status_tab[(laststatus)][SECof2(byte)])
#else /* not _NAMESPACE_CLEAN */
#define BYTE_STATUS(byte,laststatus) 	\
			(_status_tab[(laststatus)][SECof2(byte)])
#endif /* else not _NAMESPACE_CLEAN */


/*
    These macros deal with 7/8 bit and HP15 (16 bit) data, and
    evaluate in the same manner as the expressions shown to the
    right of each macro.
    
    ADVANCE(p)     :  (++p)
    CHARAT(p)      :  (*p)
    CHARADV(p)     :  (*p++)
    WCHAR(c, p)    :  (*p = c)		(evaluates to "c")
    WCHARADV(c, p) :  (*p++ = c)	(evaluates to "c")

    The following macros also deal with 7/8/16 bit data, but are
    not true expressions because of their use of statements.  They
    should be considered obsolescent; use WCHAR and WCHARADV instead.

    PCHAR(c, p)    :  {*p = c;}
    PCHARADV(c, p) :  {*p++ = c;}
*/

#define ADVANCE(p) ((p) += ((FIRSTof2((unsigned char) (p)[0])) && \
			    (SECof2((unsigned char) (p)[1]))    ? \
			    2 : 1))

#define CHARAT(p)  						    \
	((FIRSTof2((unsigned char) (p)[0])) 			 && \
	 (  SECof2((unsigned char) (p)[1]))   			  ? \
	((unsigned short) (((p)[0]<<8) | (unsigned char) (p)[1])) : \
	((unsigned char) (p)[0]))

#define CHARADV(p)							      \
	((FIRSTof2((unsigned char) (p)[0])) 			           && \
	 (  SECof2((unsigned char) (p)[1]))   			            ? \
	((p)+=2, (unsigned short) (((p)[-2]<<8) | (unsigned char) (p)[-1])) : \
	((unsigned char) *(p)++))

#define WCHAR(c, p)  						\
	(((c) & 0xff00)	      			              ? \
	((p)[0] = (unsigned char) ((c)>>8),			\
	 (p)[1] = (unsigned char) (c), (unsigned short) (c)) : 	\
	((unsigned char) ((p)[0] = (unsigned char)(c))))

#define WCHARADV(c, p)  					\
	(((c) & 0xff00)	      			              ? \
	(*(p)++ = (unsigned char) ((c)>>8),			\
	 *(p)++ = (unsigned char) (c), (unsigned short) (c)) : 	\
	((unsigned char) (*(p)++ = (unsigned char) (c))))

#define _CHARAT(p)		CHARAT(p)
#define _CHARADV(p)		CHARADV(p)
#define _WCHAR(c, p)		WCHAR(c, p)
#define _WCHARADV(c, p)		WCHARADV(c, p)

/* PCHAR, PCHARADV are obsolete; use WCHAR and WCHARADV */

#define PCHAR(c, p)	{ 						\
			    if ((c) & 0xff00) { 			\
				(p)[0] = (unsigned char) ((c)>>8); 	\
				(p)[1] = (unsigned char) (c);		\
			    }						\
			    else 					\
				(p)[0] = (unsigned char) (c);		\
			}

#define PCHARADV(c, p)	{ 						\
			    if ((c) & 0xff00) 				\
				*(p)++ = (unsigned char) ((c)>>8); 	\
			    *(p)++ = (unsigned char) (c);		\
			}						\

#ifdef _NAMESPACE_CLEAN
extern __status_tab [][4];
#else /* not _NAMESPACE_CLEAN */
extern _status_tab [][4];
#endif /* else not _NAMESPACE_CLEAN */

#if defined(__STDC__) || defined(__cplusplus)
  extern int firstof2(int);
  extern int secof2(int);
  extern int byte_status(int, int);
#else /* not __STDC__ || __cplusplus */
  extern int firstof2();
  extern int secof2();
  extern int byte_status();
#endif /* else not __STDC__ || __cplusplus */

#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _NL_CTYPE_INCLUDED */

#endif /* EUC */
