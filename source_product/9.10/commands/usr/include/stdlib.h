/* @(#) $Revision: 70.15 $ */
#ifndef _STDLIB_INCLUDED
#define _STDLIB_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE__STDC__
#  define EXIT_FAILURE 1
#  define EXIT_SUCCESS 0
#  define MB_CUR_MAX __nl_char_size
   extern int __nl_char_size;

#  ifndef NULL
#    define NULL 0
#  endif

#  define RAND_MAX 32767
   typedef struct {
	int quot;	/* quotient */
	int rem;	/* remainder */
   } div_t;
   typedef struct {
	long int quot;	/* quotient */
	long int rem;	/* remainder */
   } ldiv_t;

#  ifndef _SIZE_T
#    define _SIZE_T
     typedef unsigned int size_t;
#  endif

#  ifndef _WCHAR_T
#    define _WCHAR_T
     typedef unsigned int wchar_t;
#  endif

#  ifdef _PROTOTYPES 
#    ifndef _ATOF_DEFINED
#      define _ATOF_DEFINED
       extern double atof(const char *);
#    endif /* _ATOF_DEFINED */
     extern int atoi(const char *);
     extern long int atol(const char *);
     extern double strtod(const char *, char **);
     extern long int strtol(const char *, char **, int);
     extern unsigned long int strtoul(const char *, char **, int);
     extern int rand(void);
     extern void srand(unsigned int);
     extern int atexit(void (*) (void));
     extern void exit(int);
     extern char *getenv(const char *);
     extern int system(const char *);
#    ifndef __cplusplus
       extern int abs(int);
#    else
#      ifndef _MATH_INCLUDED
        inline int abs(int d) { return (d>0)?d:-d; }
#      endif /* _MATH_INCLUDED */
#    endif
     extern div_t div(int, int);
     extern ldiv_t ldiv(long int, long int);
     extern long int labs(long int);
     extern int mblen(const char *, size_t);
     extern int mbtowc(wchar_t *, const char *, size_t);
     extern int wctomb(char *, wchar_t);
     extern size_t mbstowcs(wchar_t *, const char *, size_t);
     extern size_t wcstombs(char *, const wchar_t *, size_t);
     extern void free(void *);
     extern void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
#  else /* _PROTOTYPES */
#    ifndef _ATOF_DEFINED
#      define _ATOF_DEFINED
       extern double atof();
#    endif /* _ATOF_DEFINED */
     extern int atoi();
     extern long int atol();
     extern double strtod();
     extern long int strtol();
     extern void free();
     extern unsigned long int strtoul();
     extern int rand();
     extern void srand();
     extern int atexit();
     extern void exit();
     extern char *getenv();
     extern int system();
     extern int abs();
     extern div_t div();
     extern ldiv_t ldiv();
     extern long int labs();
     extern void qsort();
     extern int mblen();
     extern int mbtowc();
     extern int wctomb();
     extern size_t mbstowcs();
     extern size_t wcstombs();
#  endif /* _PROTOTYPES */

#  ifdef _CLASSIC_ANSI_TYPES
     extern int abort();
     extern char *bsearch();
     extern char *calloc();
     extern char *malloc();
     extern char *realloc();
#  else
#    ifdef _PROTOTYPES 
       extern void abort(void);
       extern void *bsearch(const void *, const void *, size_t, size_t, int (*) (const void *, const void *));
       extern void *calloc(size_t, size_t);
       extern void *malloc(size_t);
       extern void *realloc(void *, size_t);
#    else /* _PROTOTYPES  */
       extern void abort();
       extern void *bsearch();
       extern void *calloc();
       extern void *malloc();
       extern void *realloc();
#    endif /* _PROTOTYPES */
#  endif

#endif /* _INCLUDE__STDC__ */


#ifdef _XPG4
#  ifdef _PROTOTYPES
     extern void setkey(const char *);
     extern void lcong48( unsigned short [] );
#  else /* _PROTOTYPES */
     extern void setkey();
     extern void lcong48();
#  endif /* _PROTOTYPES */
#endif /* _XPG4 */


#if defined( _XPG4 ) || defined ( _INCLUDE_AES_SOURCE )
#  ifdef _PROTOTYPES
     extern double drand48(void);
     extern double erand48(unsigned short []);
     extern long jrand48(unsigned short []);
     extern long lrand48(void);
     extern long mrand48(void);
     extern long nrand48(unsigned short []);
     extern void srand48(long);
     extern unsigned short *seed48(unsigned short []);
     extern int putenv(const char *);
#  else  /* _PROTOTYPES */
     extern double drand48();
     extern double erand48();
     extern long jrand48();
     extern long lrand48();
     extern long mrand48();
     extern long nrand48();
     extern void srand48();
     extern unsigned short *seed48();
     extern int putenv();
#  endif /* _PROTOTYPES */
#endif /* _XPG4 || _INCLUDE_AES_SOURCE */


#ifdef _INCLUDE_AES_SOURCE
# ifdef _PROTOTYPES
    extern int clearenv(void);
    extern int getopt(int, char * const [], const char *);
    extern char *getpass(const char *);
# else   /* no _PROTOTYPES */
    extern int clearenv();
    extern int getopt();
    extern char *getpass();
# endif	/* _PROTOTYPES */

  extern char *optarg;
  extern int optind;
  extern int opterr;
#endif	/* _INCLUDE_AES_SOURCE */


#ifdef _INCLUDE_HPUX_SOURCE
/* Constants defining mallopt operations */
#define M_MXFAST	1	/* set size of blocks to be fast */
#define M_NLBLKS	2	/* set number of block in a holding block */
#define M_GRAIN		3	/* set number of sizes mapped to one, for
				   small blocks */
#define M_KEEP		4	/* retain contents of block after a free until
				   another allocation */
#define M_BLOCK		5	/* enable signal blocking in all malloc
				   related routines */
#define M_UBLOCK	6	/* disable signal blocking set by M_BLOCK */

#ifndef _STRUCT_MALLINFO
#  define _STRUCT_MALLINFO

  /* structure filled by mallinfo */
  struct mallinfo  {
	int arena;	/* total space in arena */
	int ordblks;	/* number of ordinary blocks */
	int smblks;	/* number of small blocks */
	int hblks;	/* number of holding blocks */
	int hblkhd;	/* space in holding block headers */
	int usmblks;	/* space in small blocks in use */
	int fsmblks;	/* space in free small blocks */
	int uordblks;	/* space in ordinary blocks in use */
	int fordblks;	/* space in free ordinary blocks */
	int keepcost;	/* cost of enabling keep option */
  };	
#endif /* _STRUCT_MALLINFO */

#  ifndef _LONG_DOUBLE
#    define _LONG_DOUBLE
     typedef struct {
       unsigned int word1, word2, word3, word4;
     } long_double;
#  endif /* _LONG_DOUBLE */

#  ifdef _PROTOTYPES
#    ifndef _PWD_INCLUDED
#      include <pwd.h>
#    endif /* _PWD_INCLUDED */

#    ifndef _ERRNO_INCLUDED
#      include <errno.h>
#    endif  /* _ERRNO_INCLUDED */

     extern const char *fcvt(double, size_t, int *, int *);
     extern char *gcvt(double, size_t, char *);
     extern char *ecvt(double, size_t, int *, int *);
     extern char *nl_gcvt(double, size_t, char *, int);
     extern char *_ldecvt(long_double, size_t, int *, int *);
     extern char *_ldfcvt(long_double, size_t, int *, int *);
     extern char *_ldgcvt(long_double, size_t, char *);
     extern int getpw(int, char *);
     extern long a64l(const char *);
     extern char *l64a(long);
     extern void l3tol(long *, const char *, int);
     extern void ltol3(char *, const long *, int);
     extern double nl_atof(const char *, int);
     extern double nl_strtod(const char *, char **, int);
     extern char *ltostr(long, int);
     extern char *ultostr(unsigned long, int);
     extern char *ltoa(long);
     extern char *ultoa(unsigned long);
     extern void memorymap(int);
     extern struct mallinfo mallinfo(void);
     extern int mallopt(int, int);
     extern long_double strtold(const char *, char **);
#  else /* _PROTOTYPES */
     extern char *fcvt();
     extern char *gcvt();
     extern char *ecvt();
     extern char *nl_gcvt();
     extern char *_ldecvt();
     extern char *_ldfcvt();
     extern char *_ldgcvt();
     extern char *ltostr();
     extern char *ultostr();
     extern char *ltoa();
     extern char *ultoa();
     extern void memorymap();
     extern struct mallinfo mallinfo();
     extern int mallopt();
     extern long_double strtold();
#  endif /* _PROTOTYPES */

#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _STDLIB_INCLUDED */
