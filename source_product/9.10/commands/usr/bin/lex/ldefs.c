/* @(#) $Revision: 70.3 $ */    
# include <stdio.h>
# include <limits.h>
# include <nl_types.h>
# include <locale.h>

# define PP 1
# ifndef unix
# define unix 1
# endif

# ifdef u370
#   define CWIDTH 8
#   define CMASK 0377
#   define ASCII 1
# else
#   ifdef unix
#     if defined NLS || defined NLS16
#       define CWIDTH 8
#       ifdef CMASK 
#          if (CMASK != 0377)
#             undef CMASK
#             define CMASK 0377
#          else
#             define CMASK 0377
#	   endif
#	endif
#     else
#       define CMASK 0177
#       define CWIDTH 7
#     endif
#     define ASCII 1
# endif

# ifdef gcos
# define CWIDTH 9
# define CMASK 0777
# define ASCII 1
# endif

# ifdef ibm
# define CWIDTH 8
# define CMASK 0377
# define EBCDIC 1
# endif
# endif

# ifdef ASCII
#if defined NLS || defined NLS16
# define NCH 256
#else
# define NCH 128
#endif
# endif

# ifdef EBCDIC
# define NCH 256
# endif


# define TOKENSIZE 1000
# define DEFSIZE 200
# define DEFCHAR 5000
# define STARTCHAR 500
# define STARTSIZE 50
# define CCLSIZE 1000
# ifdef SMALL
# define TREESIZE 600
# define NTRANS 1500
# define NSTATES 300
# define MAXPOS 1500
# define NOUTPUT 1500
# define MAXPOS_STATE 300
# endif

# ifndef SMALL
# define TREESIZE 1000
# define NSTATES 500
# define MAXPOS 2500
# define NTRANS 2000
# define NOUTPUT 3000
# define MAXPOS_STATE 300
# endif
# define NACTIONS 100
# define ALITTLEEXTRA 30

# define RCCL NCH+90
# define RNCCL NCH+91
# define RSTR NCH+92
# define RSCON NCH+93
# define RNEWE NCH+94
# define FINAL NCH+95
# define RNULLS NCH+96
# define RCAT NCH+97
# define STAR NCH+98
# define PLUS NCH+99
# define QUEST NCH+100
# define DIV NCH+101
# define BAR NCH+102
# define CARAT NCH+103
# define S1FINAL NCH+104
# define S2FINAL NCH+105
# define RNOSCON NCH+106

# define DEFSECTION 1
# define RULESECTION 2
# define ENDSECTION 5
# define TRUE 1
# define FALSE 0
# define PC 1
# define PS 1
# define None -1

# ifdef DEBUG
# define LINESIZE 110
extern int yydebug;
extern int debug;		/* 1 = on */
extern int charc;
# endif

# ifndef DEBUG
# define freturn(s) s
# endif

#if defined(PAXDEV) || !defined(OSF)
#if defined NLS || defined NLS16
# define BIT8(c)		(0240<c && c>0377)
typedef unsigned char uchar;
#else
# define BIT8(c)		(0)
typedef char uchar;
#endif
#else
# define BIT8(c)		(0)
#endif

#ifdef NLS16                                                    
# define TwoByte(f,s)           (FIRSTof2(f) && SECof2(s))    
#  include <nl_ctype.h>
#  include <langinfo.h>
#endif

#ifdef POSIX_ERE
# include <setlocale.h>  /* This is a internal include file found in build */
#endif                   /* environments.  It is not distributed to users. */

/* Remove 300 macro defenition for ctos.  We use it with message output. */
#ifdef ctos
#undef ctos
#endif

extern int sargc;
extern char **sargv;
extern uchar buf[520];
extern int ratfor;		/* 1 = ratfor, 0 = C */
extern int yyline;		/* line number of file */
extern int sect;
extern int eof;
extern int lgatflg;
extern int divflg;
extern int funcflag;
extern int pflag;
extern int casecount;
extern int chset;	/* 1 = uchar set modified */
extern FILE *fin, *fout, *fother, *errorf;
extern int fptr;
extern char *ratname, *cname;
extern int prev;	/* previous input character */
extern int pres;	/* present input character */
extern int peek;	/* next input character */
extern int *name;
extern int *left;
extern int *right;
extern int *parent;
extern uchar *nullstr;
extern int tptr;
extern uchar pushc[TOKENSIZE];
extern uchar *pushptr;
extern uchar *slist;
extern uchar *slptr;
extern uchar **def, **subs, *dchar;
extern uchar **sname, *schar;
extern uchar *stype;
extern uchar *ccl;
extern uchar *ccptr;
extern uchar *dp, *sp;
extern int dptr, sptr;
extern uchar *bptr;		/* store input position */
extern uchar *tmpstat;
extern int count;
extern int **foll;
extern int *nxtpos;
extern int *positions;
extern int *actemp, *acneg; 	/* ptrs to space used in acompute */
extern int *gotof;
extern int *nexts;
extern uchar *nchar;
extern int **state;
extern int *sfall;		/* fallback state num */
extern uchar *cpackflg;		/* true if state has been character packed */
extern int *atable, aptr;
extern int nptr;
extern uchar symbol[NCH];
extern uchar cindex[NCH];
extern int xstate;
extern int stnum;
extern int ctable[];
extern int ZCH;
extern int ccount;
extern uchar match[NCH];
extern uchar *extra;
extern uchar *pcptr, *pchar;
extern int pchlen;
extern int nstates, maxpos;
extern int maxpos_state;	/* maximum positions per state */
extern int yytop;
extern int report;
extern int nls_wchar;

#ifdef NLS16
extern int nls16;
extern int opt_w_set;

typedef struct {
	char * 	locale;   /* (Optional) Locale name this refers to */
	uchar  	first1on,     /* beginning 1st valid FirstOfTwo bytes */
		first1off,    /* end of 1st valid FirstOfTwo bytes */
		sec1on,       /* beginning 1st valid SecOfTwo bytes */
		sec1off;      /* end of 1st valid SecOfTwo bytes */
	uchar	first2on,     /* beginning 2nd valid FirstOfTwo bytes */
		first2off,    /* end of 2nd valid FirstOfTwo bytes */
		sec2on,       /* beginning 2nd valid SecOfTwo bytes */
		sec2off;      /* end of 2nd valid SecOfTwo bytes */
      } Range_Factors, *Range_fact_ptr;

#endif

extern int ntrans, treesize, outsize;
extern long rcount;
extern int optim;
extern int *verify, *advance, *stoff;
extern int scon;
extern uchar *psave;
extern char *getl();
extern void exit();
extern char *calloc();
extern uchar *myalloc();
extern int buserr(), segviol();
extern int yytextarr;

extern int defsize;
extern int defchar;
extern int startsize;
extern int startchar;
extern int cclsize;
extern int nactions;

extern char * inputfile;	/* ptr to input file name for error messges */

extern int col_syms;

extern char ncformfile[];
extern int alt_ncform;
