/* @(#) $Revision: 70.4 $ */      

#ifdef _NAMESPACE_CLEAN
#define catopen _catopen
#define catclose _catclose
#define close _close
#define strlen _strlen
#define strcpy _strcpy
#define getenv _getenv
#define open _open
#define dup _dup
#define fcntl _fcntl
#  ifdef __lint
#  define putchar _putchar
#  endif /* __lint */
#endif

#include <stdio.h>
#include <errno.h>            /* defines ENAMETOOLONG */
#include <fcntl.h>
#include <memory.h>
#include <sys/param.h>        /* defines MAXPATHLEN */
#include <nl_types.h>
#include <locale.h>	      /* defines LC_MESSAGES */

#ifdef _THREAD_SAFE
#include "rec_mutex.h"

extern REC_MUTEX _catalog_rmutex;
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
#ifndef ENAMETOOLONG
#define ENAMETOOLONG 248
#endif

#ifdef NLS
#include <limits.h>
#endif

#ifdef NLS16
#include <nl_ctype.h>
#else
#define ADVANCE(p)	(p++)
#define FIRSTof2(c)	(0)
#define CHARADV(p)	(*p++)
#define CHARAT(p)	(*p)
#define WCHAR(c, p)	(*p = c)
#define WCHARADV(c, p)	(*p++ = c)
#define SECof2(c)	(0)
#endif

#ifndef NL_SAFEFD	     /* minimum safe fd - avoid 0 1 2 */
#define NL_SAFEFD 3
#endif

#define LANG_DEFAULT ""
#define NLSPATH_DEFAULT "/usr/lib/nls/%l/%t/%c/%N.cat"

extern int errno;
extern unsigned char __langmap[N_CATEGORY][LC_NAME_SIZE];

static char *codeset;		/* codeset element from LANG      */
static int  l_codeset;		/* length of codeset, less '\0'   */
static char *xname;		/* filename after macro expansion */
static int  l_xname;		/* length of xname, less '\0'	  */
static char *lang;		/* LANG env variable  		  */
static int  l_lang;		/* length of lang, less '\0'      */
static char *language;  	/* language element, from LANG    */
static int  l_language;		/* length of language, less '\0'  */
static char *nlspath;		/* NLSPATH env variable           */
static char *territory;		/* territory element, from LANG   */
static int  l_territory;	/* length of territory, less '\0' */

char *getenv();
char *strcpy();

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef catclose
#pragma _HP_SECONDARY_DEF _catclose catclose
#define catclose _catclose
#endif

catclose(catfd)
nl_catd catfd;
{
    return(close(catfd));
}

/*  catopen makes no checks for valid parameters; in particular, no
    check for a NULL string is made.
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef catopen
#pragma _HP_SECONDARY_DEF _catopen catopen
#define catopen _catopen
#endif

nl_catd catopen(name, oflag)
register char *name;
int oflag;
{
    register char *tmp;
    char namebuf[MAXPATHLEN+1];
    char langbuf[NL_LANGMAX+1];
    char nlsbuf[MAXPATHLEN+1];
    int fd;
    int done = 0;

#ifdef _THREAD_SAFE
    nl_catd return_val;

    _rec_mutex_lock(&_catalog_rmutex);
#endif

    xname = NULL;
    l_xname = 0;

    if (strlen(name) > MAXPATHLEN) {
	errno = ENAMETOOLONG;
#ifdef _THREAD_SAFE
	_rec_mutex_unlock(&_catalog_rmutex);
#endif
	return(-1);
    }

    /* %N in "name"?  Not allowed -- recursive macro */

    tmp = name;
    while (*tmp) {
	if (CHARADV(tmp) == '%') {
	    if (CHARADV(tmp) == 'N') {
		errno = ENAMETOOLONG;
#ifdef _THREAD_SAFE
		_rec_mutex_unlock(&_catalog_rmutex);
#endif
		return(-1);
	    }
	}
    }

    getvars(langbuf, oflag);	/* get NLSPATH, get/split LANG */

    if (*name) {
	/* expand name macros if name isn't an empty string */
	if (!expand(name, namebuf, (char **)0)) {
#ifdef _THREAD_SAFE
	    _rec_mutex_unlock(&_catalog_rmutex);
#endif
	    return(-1);
	}
	xname = namebuf;
    } else
	/* otherwise just use the empty string */
	xname = name;

    l_xname = strlen(xname);

#ifdef DEBUG
    print16("expanded name: ", xname);
#endif

    tmp = xname; 	/* / in name?   use as path */
    while (*tmp && (CHARAT(tmp) != '/'))
	ADVANCE(tmp);
    if (*tmp)
#ifdef _THREAD_SAFE
    {
        return_val = safeopen(xname, O_RDONLY);
	_rec_mutex_unlock(&_catalog_rmutex);
        return(return_val);
    }
#else
	return(safeopen(xname, O_RDONLY));
#endif
     
    /*
     * Isolate each path within nlspath in turn and try to
     * expand and then open it.  If none of the nlspath paths
     * work, try the system default path.
     */

    /* the only exit is a successful open or the exhaustion of the paths */
    while (1) {

	/* exhausted current pathlist? */
	if (!nlspath || !*nlspath) {
	    if (done) {
#ifdef _THREAD_SAFE
		_rec_mutex_unlock(&_catalog_rmutex);
#endif
		return(-1);				/* exhausted both user and system default */
	    }
	    else {
		done++;					/* exhausted user, shift to system default */
		nlspath = NLSPATH_DEFAULT;
	    }
	}

	/* expand any tokens and then try to open the result */
	if (expand(nlspath, nlsbuf, &nlspath) && (fd = safeopen(nlsbuf, O_RDONLY)) != -1) {
#ifdef _THREAD_SAFE
	    _rec_mutex_unlock(&_catalog_rmutex);
#endif
	    return(fd);
	}

#ifdef DEBUG
        print16("expanded path: ", nlsbuf);
#endif

    }
} /* end catopen() */


static expand(str, buffer, newstr)
register char *str;
register char *buffer;
register char **newstr;
{
    register char *buf = buffer;
    register char *last = buf + MAXPATHLEN - 1;

#ifdef DEBUG
    printf("l_lang      = %d\n", l_lang);
    printf("l_language  = %d\n", l_language);
    printf("l_territory = %d\n", l_territory);
    printf("l_codeset   = %d\n", l_codeset);
    printf("l_xname     = %d\n", l_xname);
#endif

    while (*str && *str != ':')
    {
	if (*str != '%') {
	    *buf++ = *str;
	    if (FIRSTof2(*str))
		*buf++ = *++str;
	}
	else {
	    switch (*++str) {
		case 'c' :
		    if (codeset) {
			if (buf + l_codeset > last)
			    goto oops;
		        (void)strcpy(buf, codeset);
		        buf += l_codeset;
		    }
		    break;
		case 'l' :
		    if (language) {
			if (buf + l_language > last)
			    goto oops;
		        (void)strcpy(buf, language);
		        buf += l_language;
		    }
		    break;
		case 'L' :
		    if (lang) {
			if (buf + l_lang > last)
			    goto oops;
			(void)strcpy(buf, lang);
			buf += l_lang;
		    }
		    break;
		case 'N': 
		    if (xname) {
			if (buf + l_xname > last)
			    goto oops;
		        (void)strcpy(buf, xname);
		        buf += l_xname;
		    }
		    break;
		case 't' :
		    if (territory) {
			if (buf + l_territory > last)
			    goto oops;
		        (void)strcpy(buf, territory);
		        buf += l_territory;
		    }
		    break;
		case '%' :
		    *buf++ = '%';
		    break;
		default :
		    --str;
		    *buf++ = '%';
		    break;
	    }
	}
	++str;
	if (buf > last)
	    goto oops;
    }

    /*
     * If path turned out to be empty, use the catopen name argument.
     * Otherwise, just put NULL at the end of the constructed path.
     */
    if (buf == buffer)
	(void)strcpy(buf, xname);
    else
	*buf = '\0';
    
    if (newstr)
	*newstr = (*str) ? str+1 : str ;			/* save ptr to start of next path */

    /* and make a successful return */
    return(1);

    /* error return */
oops:
    errno = ENAMETOOLONG;
    while (*str && *str++ != ':');				/* advance to end of bad path */
    if (newstr)							/* and save start of next path */
	*newstr = str;
    return(0);
}

static getvars(langbuf, oflag)
char *langbuf;
int oflag;
{
    register char *tmp;
    register int state;

    language = territory = codeset = NULL;
    l_lang = l_language = l_territory = l_codeset = 0;
    
    nlspath = getenv("NLSPATH");

    switch (oflag) {
	case 0:				/* get catalogs from LANG.  Pre-XPG4. */
		if (!(lang = getenv("LANG")))
			lang = LANG_DEFAULT;
		break;
	case NL_CAT_LOCALE:		/* XPG4 get catalogs from LC_MESSAGES */
		lang = __langmap[LC_MESSAGES];		  /* set in setlocale */
		break;
	default:			/* oflag value invalid.  Use default. */
		lang = LANG_DEFAULT;
		break;
    }
    l_lang = strlen(lang);


    /*  split language_territory.codeset in copy of LANG */

    (void)strcpy(langbuf, lang);
    language = langbuf;
    tmp = language;
    state = 0;
    while (*tmp) {
	if (CHARAT(tmp) == '_' && state == 0){
		WCHARADV('\0', tmp);	/* delimit the language string */
		territory = tmp;
		state = 1;
	} else if (CHARAT(tmp) == '.' && state <= 1){
		WCHARADV('\0', tmp);	/* delimit the territory string */
		codeset = tmp;
		state = 2;
	} else if (CHARAT(tmp) == '@'){
		WCHARADV('\0', tmp);	/* delimit the codeset string */
					/* ignore the modifier string */
		break;		/* don't char about the rest of the string */
	} else
		ADVANCE(tmp);
    }
    if ((l_language = strlen(language)) < 1) {
	territory = NULL;
	codeset   = NULL;
    } else {
    	if (territory)
		l_territory = strlen(territory);
    	if (codeset)
		l_codeset = strlen(codeset);
    }

#ifdef DEBUG
    print16("LANG      = ", lang);
    print16("language  = ", language);
    print16("territory = ", territory);
    print16("codeset   = ", codeset);
#endif
}

/*************************** UTILITY FUNCTIONS ************************/

/*  print16() is provided in lieu of other NLS routines, to isolate
    testing as much as possible.
*/

#ifdef DEBUG
static print16(prefix, str)
char *prefix;
char *str;
{
    printf("%s", prefix);
    while (*str)
    {
	putchar(*str);
	if (FIRSTof2(*str))
	    putchar(str[1]);
	ADVANCE(str);
    }
    putchar('\n');
    __fflush(stdout);
}
#endif


/*  safeopen() will open a file, returning a file descriptor
*   >= NL_SAFEFD -- a threshhold intended to avoid conflicts with
*   stdin, stdout, and stderr.
*/

static safeopen(name, mode)
char *name;
int mode;
{
    int fds[NL_SAFEFD];
    register int fd;
    register int i;

    for (i=0; i<NL_SAFEFD; i++)
	fds[i] = 0;

    fd = open(name, mode);

    while (fd != -1) {
	if (fd >= NL_SAFEFD)
	    break;
	fds[fd] = 1;
	fd = dup(fd);
    }

    for (i=0; i<NL_SAFEFD; i++) 
	if (fds[i]) 
	    (void)close(i);

    if (fd != -1)
	    (void)fcntl(fd, F_SETFD, 1);	/* set close-on-exec flag */

    return(fd);
}
