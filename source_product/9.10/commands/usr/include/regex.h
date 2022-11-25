/* @(#) $Revision: 70.10 $ */
#ifndef _REGEX_INCLUDED
#define _REGEX_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_INCLUDE_POSIX2_SOURCE) || defined(_REGEXP_INCLUDED)

#ifndef _SIZE_T
#  define _SIZE_T
   typedef unsigned int size_t ;
#endif /* _SIZE_T */
  

  /* Constants:   */
#define REG_NPAREN  255   /* The upper limit on number of substrings that 
                               can be reported.
                               Note: the upper limit for this number is 255 */

typedef int regoff_t;

typedef struct {
    unsigned char *__c_re;       /* pointer to compiled RE buffer  */ 
    unsigned char *__c_re_end;   /* pointer to byte following end of compiled
                                    RE   */
    unsigned char *__c_buf_end;  /* pointer to end of compiled RE buffer */
    size_t        __re_nsub;       /* number of parenthesized subexpressions */
    int           __anchor;      /* true if RE must be anchored */
    int           __flags;       /* storage for flags           */
} regex_t;


typedef struct {
    regoff_t    __rm_so;    /* byte offset from start of string to start of 
                             substring */
    regoff_t    __rm_eo;    /* byte offset from start of string of the first 
                             character after the end of substring */
} regmatch_t;

#ifndef _REGEXP_INCLUDED
#  define re_nsub      __re_nsub
#  define rm_so        __rm_so
#  define rm_eo        __rm_eo
#endif /* not _REGEXP_INCLUDED */

/* Regcomp Flags:   */
#define REG_EXTENDED  0001   /* use Extended Regular Expressions  */
#define REG_NEWLINE 0002     /* treat \n as a regular character   */
#define REG_ICASE   0004     /* ignore case in match     */
#define REG_NOSUB   0010     /* report success/fail only in regexec() */
#define _REG_NOALLOC 0040    /* don't allocate memory for buffer--use only 
                                what's provided   */
#define _REG_C_ESC  0200     /* special flag to ease /usr/bin/awk 
                                implementation  */
#define _REG_ALT    0400     /* special flag to ease /bin/grep implementation*/
#define _REG_EXP    0100     /* special flag to for regexp */


/* Regexec Flags:   */
#define REG_NOTBOL  0001     /* never match ^ as special char */
#define REG_NOTEOL  0002     /* never matchj $ as special char */


/* Error Codes: */
#define REG_NOMATCH 20      /* regexec() failed to match            */
#define REG_BADPAT  2       /* invalid regular expression           */
#define REG_ECOLLATE 21     /* invalid collation element referenced */
#define REG_ECTYPE  24      /* invalid character class type named   */
#define REG_EESCAPE 22      /* trailing \ in pattern                */
#define REG_ESUBREG 25      /* number in \digit invalid or in error */
#define REG_EBRACK  49      /* [ ] imbalance                        */
#define REG_EPAREN  42      /* \( \) imbalance or ( ) imbalance     */
#define REG_EBRACE  45      /* \{ \} imbalance                      */
#define REG_BADBR   3       /* Contents of \{\} invalid: Not a number, number 
                               too large, more than two numbers, first larger 
                               than second */
#define REG_ERANGE  23      /* invalid endpoint in range statement  */
#define REG_ESPACE  50      /* out of memory for compiled pattern   */
#define REG_BADRPT  26      /* ?, *, or + not preceded by valid regular 
                               expression  */
#define REG_ENEWLINE 36     /* \n found before end of pattern and REG_NEWLINE 
                               flag not set  */
#define REG_ENSUB 43        /* more than nine \( \) pairs or nesting level 
                               too deep  */
#define REG_EMEM 51         /* out of memory while matching expression */

#define REG_ENOSEARCH  41   /* no remembered search string              */
#define REG_EDUPOPER  26    /* duplication operator in illegal position */
#define REG_ENOEXPR 27      /* no expression within ( ) or on one side of an |*/
#ifdef _XPG4
#define REG_ENOSYS  28      /* does not support the function */
#endif /* _XPG4 */

#if defined(__STDC__) || defined(__cplusplus)
   extern int regcomp(regex_t *,const char *,int);
   extern int regexec(const regex_t *,const char *, size_t,regmatch_t [],int);
   extern void regfree(regex_t *);
   extern size_t regerror(int, const regex_t *, char *, size_t);
#else /* __STDC__ || __cplusplus */
   extern int regcomp();
   extern int regexec();
   extern void regfree();
   extern size_t regerror();
#endif /* __STDC__ || __cplusplus */

#endif /* _INCLUDE_POSIX2_SOURCE || _REGEXP_INCLUDED */

#ifdef __cplusplus
}
#endif

#endif /* _REGEX_INCLUDED */
