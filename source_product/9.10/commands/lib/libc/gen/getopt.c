/* @(#) $Revision: 70.5 $ */
/*LINTLIBRARY*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define getopt _getopt
#define strcmp _strcmp
#define catopen _catopen
#define strlen _strlen
#define write _write
#define opterr _opterr
#define optind _optind
#define optopt _optopt
#define optarg _optarg
#if defined(NLS) || defined(NLS16)
#   define catgets _catgets
#   define catclose _catclose
#endif /* NLS || NLS16 */
#endif /* _NAMESPACE_CLEAN */

#if defined(NLS) || defined(NLS16)
#include <nl_types.h>
static nl_catd catd;
extern nl_catd catopen();
extern char *catgets();
static char catalog_name[] = "getopt_lib"; /* getopt(1) uses getopt_cmd */
#define NL_SETN 1
#define CATOPEN	 (catd = catopen(catalog_name, 0))
#define CATCLOSE ((void)catclose(catd))
#else
#define catgets(i,sn,mn,s) (s)
#define CATOPEN
#define CATCLOSE
#endif

#if defined(NLS) || defined(NLS16)
#include <nl_ctype.h>
#else
#define CHARAT(p)	(*p)
#define CHARADV(p)	(*p++)
#define ADVANCE(p)	(++p)
#define PCHAR(c,p)	(*p = c)
#define PCHARADV(c,p)	(*p++ = c)
#endif

#define UCHAR	unsigned char
#define UINT	unsigned int
#define NULL	0
#define EOF	(-1)

/*
 * ERR -- a macro to print an error message from getopt() in an
 *        efficient manner.
 */
#define ERR(s, c) \
    if (opterr && ! colon_specified)				\
    {								\
	extern int strlen();					\
	extern int write();					\
	char *errmsg;						\
	char errbuf[6];	/* for multibyte chars + "\n\0" */	\
	char *tmp2;						\
								\
	CATOPEN;						\
	errmsg = (s);						\
	tmp2 = errbuf;						\
	PCHARADV(c, tmp2);					\
	*tmp2++ = '\n';						\
	*tmp2 = '\0';						\
	(void)write(2, argv[0], (unsigned)strlen(argv[0]));	\
	(void)write(2, errmsg,  (unsigned)strlen(errmsg));	\
	(void)write(2, errbuf,  tmp2 - errbuf);			\
	CATCLOSE;						\
    }

extern int strcmp();
static UCHAR *xstrchr();

#ifdef _NAMESPACE_CLEAN
#undef opterr
#pragma _HP_SECONDARY_DEF _opterr opterr
#define opterr _opterr
#endif
int	opterr = 1;

#ifdef _NAMESPACE_CLEAN
#undef optind
#pragma _HP_SECONDARY_DEF _optind optind
#define optind _optind
#endif
int	optind = 1;

#ifdef _NAMESPACE_CLEAN
#undef optopt
#pragma _HP_SECONDARY_DEF _optopt optopt
#define optopt _optopt
#endif
int	optopt = 0;

#ifdef _NAMESPACE_CLEAN
#undef optarg
#pragma _HP_SECONDARY_DEF _optarg optarg
#define optarg _optarg
#endif
char	*optarg = (char *) 0;

static UCHAR *tmp;

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getopt
#pragma _HP_SECONDARY_DEF _getopt getopt
#define getopt _getopt
#endif

int
getopt(argc, argv, opts)
register int	argc;
register char	**argv, *opts;
{
	static int sp = 1;
	register int c;
	register char *cp;
	int colon_specified;

	if(sp == 1) {
		tmp = (UCHAR *) argv[optind];
		if(optind >= argc ||
		    (CHARADV(tmp) != '-') || (CHARAT(tmp) == '\0')) {
			return(EOF);
		}
		else if(strcmp(argv[optind], "--") == 0) {
			optind++;
			return(EOF);
		}
	}
	tmp = (UCHAR *) &argv[optind][sp];
#ifdef NLS16
	optopt = c = CHARAT(tmp);
#else
	optopt = c = *tmp & 0377;	/* 7/8 bit chars are unsigned */
#endif
	/*
	 * Added for XPG4 and POSIX.2.  getopt() is required to return
	 * a ':' if the first character in the option string passed in 
	 * is a ':' and it detects a missing option.  It is also required
	 * to act as if opterr is 0 when the first character in optstring
	 * is a ':'.
	 */
	if(*opts == ':')
	    colon_specified = 1;
	else
	    colon_specified = 0;

	if(c == ':' || (cp=(char *)xstrchr((UCHAR *)opts, c)) == NULL) {
		ERR((catgets(catd,NL_SETN,1, ": illegal option -- ")), c);
		ADVANCE(tmp);
		if(CHARAT(tmp) == '\0') {
			optind++;
			sp = 1;
		}
		else
		    sp = (int) tmp - (int) argv[optind];
		return('?');
	}
	ADVANCE(cp);
	if(CHARAT(cp) == ':') {
		ADVANCE(tmp);
		if (CHARAT(tmp) != '\0') {
			++optind;
			optarg = (char *) tmp;
		}
		else if(++optind >= argc) {
			ERR((catgets(catd,NL_SETN,2, ": option requires an argument -- ")), c);
			sp = 1;
			optarg = NULL;
			if(colon_specified)
			    return(':');
			else
			    return('?');
		} else
			optarg = argv[optind++];
		sp = 1;
	} else {
		ADVANCE(tmp);
		if(CHARAT(tmp) == '\0') {
			sp = 1;
			optind++;
		}
		else
			sp = (int) tmp - (int) argv[optind];
		optarg = NULL;
	}
	return(c);
}

/*
    The following function is required because as of 7/13/87, LIBC
    doesn't support 16 bit.
*/

static UCHAR *xstrchr(opts, c)
register UCHAR *opts;
register int c;
{
	register int x;

	while (x = CHARAT(opts)) {
		if (x == c)
			return(opts);
		else
			ADVANCE(opts);
	}
	return(NULL);
}
