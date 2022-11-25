/* @(#) $Revision: 70.11 $ */
#ifndef _STDIO_INCLUDED
#define _STDIO_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE__STDC__

#  define _NFILE	60
#  define BUFSIZ	1024
#  define _DBUFSIZ	8192

/* buffer size for multi-character output to unbuffered files */
#  define _SBFSIZ	8

   typedef struct {
	int		 __cnt;
	unsigned char	*__ptr;
	unsigned char	*__base;
	unsigned short	 __flag;
	unsigned char 	 __fileL;		/* low byte of file desc */
	unsigned char 	 __fileH;		/* high byte of file desc */
   } FILE;

   typedef struct {
	int		 __cnt;
	unsigned char	*__ptr;
	unsigned char	*__base;
	unsigned short	 __flag;
	unsigned char 	 __fileL;		/* low byte of file desc */
	unsigned char 	 __fileH;		/* high byte of file desc */
	unsigned char	*__bufendp;	/* end of buffer */
	unsigned char	 __smbuf[_SBFSIZ]; /* small buffer */
   } _FILEX;

/*
 * _IOLBF means that a file's output will be buffered line by line
 * In addition to being flags, _IONBF, _IOLBF and _IOFBF are possible
 * vales for "type" in setvbuf
 */
#  define _IOFBF		0000000
#  define _IOREAD		0000001
#  define _IOWRT		0000002
#  define _IONBF		0000004
#  define _IOMYBUF		0000010
#  define _IOEOF		0000020
#  define _IOERR		0000040
#  define _IOLBF		0000200
#  define _IORW			0000400
#  define _IOEXT		0001000
#  define _IODUMMY		0002000 /* dummy file for doscan, doprnt */

#  ifndef NULL
#    define	NULL	0
#  endif /* NULL */

#  ifndef _SIZE_T
#    define _SIZE_T
     typedef unsigned int size_t;
#  endif /* _SIZE_T */

   typedef long int fpos_t;

#  ifdef __hp9000s300
     typedef char *__va_list;
#  endif /* __hp9000s300 */

#  ifdef __hp9000s800
     typedef double *__va_list;
#  endif /* __hp9000s800 */


#  ifndef SEEK_SET
#    define SEEK_SET 0
#    define SEEK_CUR 1
#    define SEEK_END 2
#  endif /* SEEK_SET */

#  define _P_tmpdir "/usr/tmp/"

#  define L_tmpnam	(sizeof(_P_tmpdir) + 15)

#  ifndef TMP_MAX
#    define TMP_MAX 17576
#  endif /* TMP_MAX */

#  define FILENAME_MAX 	14
#  define FOPEN_MAX 	_NFILE

#  ifndef EOF
#    define	EOF	(-1)
#  endif /* EOF */
   
#ifdef _NAMESPACE_CLEAN
#  define	stdin	(&__iob[0])
#  define	stdout	(&__iob[1])
#  define	stderr	(&__iob[2])

   extern FILE __iob[];
#else /* not _NAMESPACE_CLEAN */
#  define	stdin	(&_iob[0])
#  define	stdout	(&_iob[1])
#  define	stderr	(&_iob[2])

   extern FILE _iob[];
#endif /* else not _NAMESPACE_CLEAN */

#  if defined(__STDC__) || defined(__cplusplus)
     extern int remove(const char *);
     extern int rename(const char *, const char *);
     extern FILE *tmpfile(void);
     extern char *tmpnam(char *);
     extern int fclose(FILE *);
     extern int fflush(FILE *);
     extern FILE *fopen(const char *, const char *);
     extern FILE *freopen(const char *, const char *, FILE *);
     extern void setbuf(FILE *, char *);
     extern int setvbuf(FILE *, char *, int, size_t);
     extern int fprintf(FILE *, const char *, ...);
     extern int fscanf(FILE *, const char *,...);
     extern int printf(const char *,...);
     extern int scanf(const char *,...);
     extern int sprintf(char *, const char *,...);
     extern int sscanf(const char *, const char *,...);
     extern int vprintf(const char *, __va_list);
     extern int vfprintf(FILE *, const char *, __va_list);
     extern int vsprintf(char *, const char *, __va_list);
     extern int fgetc(FILE *);
     extern char *fgets(char *, int, FILE *);
     extern int fputc(int, FILE *);
     extern int fputs(const char *, FILE *);
     extern int getc(FILE *);
     extern int getchar(void);
     extern char *gets(char *);
     extern int putc(int, FILE *);
     extern int putchar(int);
     extern int puts(const char *);
     extern int ungetc(int, FILE *);
     extern int fgetpos(FILE *, fpos_t *);
     extern int fseek(FILE *, long int, int);
     extern int fsetpos(FILE *, const fpos_t *);
     extern long int ftell(FILE *);
     extern void rewind(FILE *);
     extern void clearerr(FILE *);
     extern int feof(FILE *);
     extern int ferror(FILE *);
     extern void perror(const char *);
#  else /* not __STDC__ || __cplusplus */
     extern int remove();
     extern int rename();
     extern FILE *tmpfile();
     extern char *tmpnam();
     extern int fclose();
     extern int fflush();
     extern FILE *fopen();
     extern FILE *freopen();
     extern void setbuf();
     extern int setvbuf();
     extern int fprintf();
     extern int fscanf();
     extern int printf();
     extern int scanf();
     extern int sprintf();
     extern int sscanf();
     extern int vprintf();
     extern int vfprintf();
     extern int vsprintf();
     extern int fgetc();
     extern char *fgets();
     extern int fputc();
     extern int fputs();
     extern int getc();
     extern int getchar();
     extern char *gets();
     extern int putc();
     extern int putchar();
     extern int puts();
     extern int ungetc();
     extern int fgetpos();
     extern int fseek();
     extern int fsetpos();
     extern long int ftell();
     extern void rewind();
     extern void clearerr();
     extern int feof();
     extern int ferror();
     extern void perror();
#  endif /* __STDC__ || __cplusplus */

#  ifdef _CLASSIC_ANSI_TYPES
     extern int fread();
     extern int fwrite();
#  else
#    if defined(__STDC__) || defined(__cplusplus)
       extern size_t fread(void *, size_t, size_t, FILE *);
       extern size_t fwrite(const void *, size_t, size_t, FILE *);
#    else /* not __STDC__ || __cplusplus */
       extern size_t fread();
       extern size_t fwrite();
#    endif /* else not __STDC__ || __cplusplus */
#  endif

/*
 * WARNING:  The following function declarations are provided solely
 *  for use in the macros found in this header file.  These are HP-UX
 *  specific routines used internally and direct use of these routines 
 *  is not supported.  These routines may be removed or significantly
 *  changed in future releases of HP-UX.
 */
#  if defined(__STDC__) || defined(__cplusplus)
#   ifdef _NAMESPACE_CLEAN
     extern int __flsbuf(unsigned char, FILE *);
     extern int __filbuf(FILE *);
#   else /* not _NAMESPACE_CLEAN */
     extern int _flsbuf(unsigned char, FILE *);
     extern int _filbuf(FILE *);
#   endif /* _NAMESPACE_CLEAN */
#  else /* not __STDC__ || __cplusplus */
#   ifdef _NAMESPACE_CLEAN
     extern int __flsbuf();
     extern int __filbuf();
#   else /* not _NAMESPACE_CLEAN */
     extern int _flsbuf();
     extern int _filbuf();
#   endif /* _NAMESPACE_CLEAN */
#  endif /* __STDC__ || __cplusplus */
  
#  ifndef __lint
#    define clearerr(__p)	((void) ((__p)->__flag &= ~(_IOERR | _IOEOF)))
#    define feof(__p)	((__p)->__flag & _IOEOF)
#    define ferror(__p)	((__p)->__flag & _IOERR)
#  ifdef _NAMESPACE_CLEAN
#    define getc(__p)     (--(__p)->__cnt < 0 ? __filbuf(__p) : \
			(int) *(__p)->__ptr++)
#  else /* not _NAMESPACE_CLEAN */
#    define getc(__p)     (--(__p)->__cnt < 0 ? _filbuf(__p) : \
			(int) *(__p)->__ptr++)
#  endif /* else not _NAMESPACE_CLEAN */

#    define getchar()	getc(stdin)

#  ifdef _NAMESPACE_CLEAN
#    define putc(__c, __p)	(--(__p)->__cnt < 0 ? \
			__flsbuf((unsigned char) (__c), (__p)) : \
			(int) (*(__p)->__ptr++ = (unsigned char) (__c)))
#  else /* not _NAMESPACE_CLEAN */
#    define putc(__c, __p)	(--(__p)->__cnt < 0 ? \
			_flsbuf((unsigned char) (__c), (__p)) : \
			(int) (*(__p)->__ptr++ = (unsigned char) (__c)))
#  endif /* else not _NAMESPACE_CLEAN */

#    define putchar(__c)	putc((__c), stdout)
#  endif /* not __lint */
#endif /* _INCLUDE__STDC__ */

#ifdef _INCLUDE_POSIX_SOURCE
#  define L_ctermid	9
#  define L_cuserid	9

#  if defined(__STDC__) || defined(__cplusplus)
     extern char *ctermid(char *);
     extern int fileno(FILE *);
     extern FILE *fdopen(int, const char *);
#  else /* not __STDC */
     extern char *ctermid();
     extern int fileno();
     extern FILE *fdopen();
#  endif /* __STDC */
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_XOPEN_SOURCE
#  ifndef _AES_SOURCE

#    ifdef __hp9000s300
#      ifndef _VA_LIST
#        define _VA_LIST
         typedef char *va_list;
#      endif  /* _VA_LIST */
#    endif /* __hp9000s300 */

#    ifdef __hp9000s800
#      ifndef _VA_LIST
#        define _VA_LIST
         typedef double *va_list;
#      endif  /* _VA_LIST */
#    endif /* __hp9000s800 */

     extern char *optarg;
     extern int opterr;
     extern int optind;
     extern int optopt;

#    ifdef _PROTOTYPES
       extern int getopt(int, char * const [], const char *);
       extern char *cuserid(char *);
#    else
       extern int getopt();
       extern char *cuserid();
#    endif /* _PROTOTYPES */
#  endif /* not _AES_SOURCE */



#  define P_tmpdir 	_P_tmpdir

#  if defined(__STDC__) || defined(__cplusplus)
     extern int getw(FILE *);
     extern int putw(int, FILE *);
     extern int pclose(FILE *);
     extern FILE *popen(const char *, const char *);
     extern char *tempnam(const char *, const char *);
#  else /* not __STDC__ || __cplusplus */
     extern int getw();
     extern int putw();
     extern int pclose();
     extern FILE *popen();
     extern char *tempnam();
#  endif /* else not __STDC__ || __cplusplus */

#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _NAMESPACE_CLEAN
#  define _iob __iob
#endif /* _NAMESPACE_CLEAN */

#  if defined(__STDC__) || defined(__cplusplus)
     extern int nl_fprintf(FILE *, const char * ,...);
     extern int nl_fscanf(FILE *, const char * ,...);
     extern int nl_printf(const char * ,...);
     extern int nl_scanf(const char * ,...);
     extern int nl_sprintf(char *, const char * ,...);
     extern int nl_sscanf(const char *, const char * ,...);
     extern int vscanf(const char *, __va_list);
     extern int vfscanf(FILE *, const char *, __va_list);
     extern int vsscanf(char *, const char *, __va_list);

#  else /* not __STDC__ || __cplusplus */
     extern int vscanf();
     extern int vfscanf();
     extern int vsscanf();
#  endif /* __STDC__ || __cplusplus */

   extern unsigned char *__bufendtab[];

#  define _cnt		__cnt
#  define _ptr		__ptr
#  define _base		__base
#  define _flag		__flag

#ifdef _NAMESPACE_CLEAN
#  define _bufend(__p) \
    (*(((__p)->__flag & _IOEXT)  ? &(((_FILEX *)(__p))->__bufendp)	\
				: &(__bufendtab[(__p) - __iob])))
#else /* _NAMESPACE_CLEAN */
#  define _bufend(__p) \
    (*(((__p)->__flag & _IOEXT)  ? &(((_FILEX *)(__p))->__bufendp)	\
				: &(__bufendtab[(__p) - _iob])))
#endif /* _NAMESPACE_CLEAN */

#  define _bufsiz(__p)	(_bufend(__p) - (__p)->__base)
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _STDIO_INCLUDED */
