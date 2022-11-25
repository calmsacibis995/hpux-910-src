/* @(#) $Revision: 70.3 $ */
#ifndef _NL_TYPES_INCLUDED /* allow multiple inclusions */
#define _NL_TYPES_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_XOPEN_SOURCE
   typedef int  nl_catd;	/* catalogue descriptor for catopen, catgetmsg,
				   catgets and catclose */
#ifndef _NL_ITEM
#  define _NL_ITEM
   typedef int  nl_item;	/* used by nl_langinfo() to identify items of
				   data in a langinfo database */
#endif /* _NL_ITEM */

#  define  NL_SETD    1		/* Used by gencat when no $set directive is
				   specified in a message text source file.
				   This constant can be passed as the value of
				   set_id on subsequent calls to catgets(). */

#ifdef _XPG4
#  define  NL_CAT_LOCALE  1	/* Value that must be passed as the oflag 
				   argument to catopen() to ensure that message
				   catalogue selection depends on the 
				   LC_MESSAGES locale category, rather than
				   directly on the LANG environment variable.*/
#endif /* _XPG4 */

#  if defined(__STDC__) || defined(__cplusplus)
     extern int catclose(nl_catd);
     extern char *catgets(nl_catd, int, int, const char *);
     extern nl_catd catopen(const char *, int);
#  else /* not __STDC__ || __cplusplus */
     extern int catclose();
     extern char *catgets();
     extern nl_catd catopen();
#  endif /* __STDC__ || __cplusplus */
#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
/* enumerated types for right-to-left processing */

   typedef enum {NL_LTR,NL_RTL} nl_direct;	/* text orientation of language
						   used to set _nl_direct */
   typedef enum {NL_KEY,NL_SCREEN} nl_order;	/* data order of file used to 
						   set _nl_order */
   typedef enum {NL_LATIN,NL_NONLATIN} nl_mode;	/* mode of file used to set 
						   _nl_mode */
   typedef enum {NL_ASCII,NL_ALT} nl_outdgt;	/* output digits used to set 
						   _nl_outdigit */

#  if defined(__STDC__) || defined(__cplusplus)
     extern char *catgetmsg(nl_catd, int, int, char *, int);
     extern char *strord(char *, const char *, nl_mode);
#  else /* not __STDC__ || __cplusplus */
     extern char *catgetmsg();
     extern char *strord();
#  endif /* __STDC__ || __cplusplus */

#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _NL_TYPES_INCLUDED */
