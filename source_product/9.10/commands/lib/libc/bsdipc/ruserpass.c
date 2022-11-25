/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if !defined(lint) && !defined(_NAMESPACE_CLEAN)
static char sccsid[] = "@(#)ruserpass.c	5.1 (Berkeley) 3/1/89";
static char rcsid[] = "$Header: ruserpass.c,v 66.8 90/07/10 13:59:52 jmc Exp $";
#endif

#ifdef _NAMESPACE_CLEAN
#  ifdef __lint
#  define fileno _fileno
#  define getc _getc
#  define putc _putc
#  endif /* __lint */
#define fileno _fileno
#define fopen _fopen
#define fclose _fclose
#define fstat _fstat
#define fputs _fputs
#define ioctl _ioctl
#define getenv _getenv
#define strchr _strchr
#define strcmp _strcmp
#define strcpy _strcpy
#define strlen _strlen
#define sigaction _sigaction
#define sigemptyset _sigemptyset
#define kill _kill
#define getpid _getpid
#define perror _perror
#define setbuf _setbuf
#define res_init _res_init
#define printf _printf
#define fprintf _fprintf
#define sprintf _sprintf
#define strcasecmp _strcasecmp
#define strncasecmp _strncasecmp
/* local */
#define getlongpass _getlongpass
#ifdef _ANSIC_CLEAN
#define malloc _malloc
#define free _free
#endif /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */

#include <sys/types.h>
#include <stdio.h>
#include <utmp.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#ifndef _NAMESPACE_CLEAN
#include "ftp_var.h"
#endif /* _NAMESPACE_CLEAN */

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#ifdef SecureWare
#include <sys/security.h>
#endif /* SecureWare */

#ifdef _NAMESPACE_CLEAN
/*
 * Declarations from ftp_var.h, declared local and static for
 * the library version of ruserpass.
 * #include "ftp_var.h"
 */
static int	proxy;			/* proxy server connection active */
static char	line[BUFSIZ];		/* input line buffer */
struct macel {
	char mac_name[9];	/* macro name */
	char *mac_start;	/* start of macro in macbuf */
	char *mac_end;		/* end of macro in macbuf */
};
static int macnum;			/* number of defined macros */
static struct macel macros[16];
static char macbuf[4096];
#define index strchr
#endif /* _NAMESPACE_CLEAN */

char	*renvlook(), *malloc(), *index(), *getenv(), *getlongpass(), *getlogin();
char	*strcpy();
struct	utmp *getutmp();
static	FILE *cfile;

#define	DEFAULT	1
#define	LOGIN	2
#define	PASSWD	3
#define	ACCOUNT 4
#define MACDEF  5
#define	ID	10
#define	MACH	11

static char tokval[100];

static struct toktab {
	char *tokstr;
	int tval;
} toktab[]= {
	"default",	DEFAULT,
	"login",	LOGIN,
	"password",	PASSWD,
	"passwd",	PASSWD,
	"account",	ACCOUNT,
	"machine",	MACH,
	"macdef",	MACDEF,
	0,		0
};

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 254
#endif

#ifdef _NAMESPACE_CLEAN
#undef ruserpass
#pragma _HP_SECONDARY_DEF _ruserpass ruserpass
#define ruserpass _ruserpass
#endif /* _NAMESPACE_CLEAN */

ruserpass(host, aname, apass, aacct)
	char *host, **aname, **apass, **aacct;
{
	char *hdir, buf[BUFSIZ], *tmp;
	char myname[MAXHOSTNAMELEN], *mydomain;
	int t, i, c, usedefault = 0;
	struct stat stb;
	extern int errno;

	hdir = getenv("HOME");
	if (hdir == NULL)
		hdir = ".";
	(void) sprintf(buf, "%s/.netrc", hdir);
	cfile = fopen(buf, "r");
	if (cfile == NULL) {
		if (errno != ENOENT)
			perror(buf);
		return(0);
	}
	if ((_res.options & RES_INIT) || res_init() != -1)
		mydomain = _res.defdname;
	else
		mydomain = "";
next:
	while ((t = token())) switch(t) {

	case DEFAULT:
		usedefault = 1;
		/* FALL THROUGH */

	case MACH:
		if (!usedefault) {
			if (token() != ID)
				continue;
			/*
			 * Allow match either for user's input host name
			 * or official hostname.  Also allow match of 
			 * incompletely-specified host in local domain.
			 */
			if (strcasecmp(host, tokval) == 0)
				goto match;
#ifndef _NAMESPACE_CLEAN
			if (strcasecmp(hostname, tokval) == 0)
				goto match;
			if ((tmp = index(hostname, '.')) != NULL &&
			    strcasecmp(tmp+1, mydomain) == 0 &&
			    strncasecmp(hostname, tokval, tmp-hostname) == 0 &&
			    tokval[tmp - hostname] == '\0')
				goto match;
#endif /* ~_NAMESPACE_CLEAN */
			if ((tmp = index(host, '.')) != NULL &&
			    strcasecmp(tmp+1, mydomain) == 0 &&
			    strncasecmp(host, tokval, tmp - host) == 0 &&
			    tokval[tmp - host] == '\0')
				goto match;
			continue;
		}
	match:
		while ((t = token()) && t != MACH && t != DEFAULT) switch(t) {

		case LOGIN:
			if (token())
				if (*aname == 0) { 
					*aname = malloc((unsigned) strlen(tokval) + 1);
					(void) strcpy(*aname, tokval);
				} else {
					if (strcmp(*aname, tokval))
						goto next;
				}
			break;
		case PASSWD:
			if (strcmp(*aname, "anonymous")) {
#ifdef SecureWare
				if (ISSECURE) {
	fprintf(stderr, "Error - passwords not allowed in .netrc file.\n");
					goto bad;
				}
#endif /* SecureWare */
				if (fstat(fileno(cfile), &stb) >= 0 &&
				    (stb.st_mode & 077) != 0) {
	fprintf(stderr, "Error - .netrc file not correct mode.\n");
	fprintf(stderr, "Remove password or correct mode.\n");
					goto bad;
				}
			}
			if (token() && *apass == 0) {
				*apass = malloc((unsigned) strlen(tokval) + 1);
				(void) strcpy(*apass, tokval);
			}
			break;
		case ACCOUNT:
			if (fstat(fileno(cfile), &stb) >= 0
			    && (stb.st_mode & 077) != 0) {
	fprintf(stderr, "Error - .netrc file not correct mode.\n");
	fprintf(stderr, "Remove account or correct mode.\n");
				goto bad;
			}
			if (token() && *aacct == 0) {
				*aacct = malloc((unsigned) strlen(tokval) + 1);
				(void) strcpy(*aacct, tokval);
			}
			break;
		case MACDEF:
			if (proxy) {
				(void) fclose(cfile);
				return(0);
			}
			while ((c=getc(cfile)) != EOF && c == ' ' || c == '\t');
			if (c == EOF || c == '\n') {
				printf("Missing macdef name argument.\n");
				goto bad;
			}
			if (macnum == 16) {
				printf("Limit of 16 macros have already been defined\n");
				goto bad;
			}
			tmp = macros[macnum].mac_name;
			*tmp++ = c;
			for (i=0; i < 8 && (c=getc(cfile)) != EOF &&
			    !isspace(c); ++i) {
				*tmp++ = c;
			}
			if (c == EOF) {
				printf("Macro definition missing null line terminator.\n");
				goto bad;
			}
			*tmp = '\0';
			if (c != '\n') {
				while ((c=getc(cfile)) != EOF && c != '\n');
			}
			if (c == EOF) {
				printf("Macro definition missing null line terminator.\n");
				goto bad;
			}
			if (macnum == 0) {
				macros[macnum].mac_start = macbuf;
			}
			else {
				macros[macnum].mac_start = macros[macnum-1].mac_end + 1;
			}
			tmp = macros[macnum].mac_start;
			while (tmp != macbuf + 4096) {
				if ((c=getc(cfile)) == EOF) {
				printf("Macro definition missing null line terminator.\n");
					goto bad;
				}
				*tmp = c;
				if (*tmp == '\n') {
					if (*(tmp-1) == '\0') {
					   macros[macnum++].mac_end = tmp - 1;
					   break;
					}
					*tmp = '\0';
				}
				tmp++;
			}
			if (tmp == macbuf + 4096) {
				printf("4K macro buffer exceeded\n");
				goto bad;
			}
			break;
		default:
	fprintf(stderr, "Unknown .netrc keyword %s\n", tokval);
			break;
		}
		goto done;
	}
done:
	(void) fclose(cfile);
	return(0);
bad:
	(void) fclose(cfile);
	return(-1);
}

static
token()
{
	char *cp;
	int c;
	struct toktab *t;

	if (feof(cfile))
		return (0);
	while ((c = getc(cfile)) != EOF &&
	    (c == '\n' || c == '\t' || c == ' ' || c == ','))
		continue;
	if (c == EOF)
		return (0);
	cp = tokval;
	if (c == '"') {
		while ((c = getc(cfile)) != EOF && c != '"') {
			if (c == '\\')
				c = getc(cfile);
			*cp++ = c;
		}
	} else {
		*cp++ = c;
		while ((c = getc(cfile)) != EOF
		    && c != '\n' && c != '\t' && c != ' ' && c != ',') {
			if (c == '\\')
				c = getc(cfile);
			*cp++ = c;
		}
	}
	*cp = 0;
	if (tokval[0] == 0)
		return (0);
	for (t = toktab; t->tokstr; t++)
		if (!strcmp(t->tokstr, tokval))
			return (t->tval);
	return (ID);
}
/*
** getlongpass() --	replacement for standard getpass()
**
** getlongpass() gets a password in the same way as getpass except
** that it will get a password up to 50 characters in length.
** this is so ruserpass works with non-UNIX hosts which can have
** passwords longer than MAX_PASS_LEN characters ...
*/

#include <signal.h>
#include <termio.h>

# define	MAX_PASS_LEN		128
static	int	intrupt;

#ifdef _NAMESPACE_CLEAN
#undef getlongpass
#pragma _HP_SECONDARY_DEF _getlongpass getlongpass
#define getlongpass _getlongpass
#endif /* _NAMESPACE_CLEAN */

char *
getlongpass(prompt)
char *prompt;
{
    void catch();
    register char *p;
    register int c;
    FILE *fi, *fo;
    register int fno;
    static char pbuf[MAX_PASS_LEN];
    struct sigaction newsig, oldsig;
    struct termio  save_ttyb, ttyb;
    

    if ((fi = fopen("/dev/tty", "r")) == NULL)
	return (char *)NULL;

    if ((fo = fopen("/dev/tty", "w")) == NULL)
    {
	if (fi != stdin)
	    fclose(fi);
	return (char *)NULL;
    }

    /*
     * We don't want any buffering for our i/o.
     */
    setbuf(fi, (char *)NULL);
    setbuf(fo, (char *)NULL);

    /*
     * Install signal handler for SIGINT so that we can restore
     * the tty settings after we change them.  The handler merely
     * increments the variable "intrupt" to tell us that an
     * interrupt signal was received.
     */
    newsig.sa_handler = catch;
    sigemptyset(&newsig.sa_mask);
    newsig.sa_flags = 0;
    sigaction(SIGINT, &newsig, &oldsig);
    intrupt = 0;

    /*
     * Get the terminal characters (save for later restoration) and
     * reset them so that echo is off
     */
    fno = fileno(fi);
    (void) ioctl(fno, TCGETA, &ttyb);

    save_ttyb = ttyb;
    ttyb.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);

    (void) ioctl(fno, TCSETAW, &ttyb);
    (void) ioctl(fno, TCFLSH, 0);


    /*
     * Write the prompt and read in the user's response.
     */
    fputs(prompt, fo);
    for (p = pbuf; !intrupt && (c = getc(fi)) != '\n' && c != EOF; )
    {
	if (p < &pbuf[MAX_PASS_LEN-1])
	    *p++ = c;
    }
    *p = '\0';
    putc('\n', fo);
    fclose(fo);

    /*
     * Restore the terminal to its previous characteristics.
     * Restore the old signal handler for SIGINT.
     */
    (void) ioctl(fno, TCSETA, &save_ttyb);
    sigaction(SIGINT, &oldsig, (struct sigaction *)0);

    if (fi != stdin)
	fclose(fi);

    /*
     * If we got a SIGINT while we were doing things, send the SIGINT
     * to ourselves so that the calling program receives it (since we
     * were intercepting it for a period of time.)
     */
    if (intrupt)
	kill(getpid(), SIGINT);

    return pbuf;
}

static void
catch()
{
	intrupt++;
}
