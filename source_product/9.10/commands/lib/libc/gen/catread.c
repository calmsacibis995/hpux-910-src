/* @(#) $Revision: 70.1 $ */      
#ifdef _NAMESPACE_CLEAN
#define catread _catread
#define strchr _strchr
#define strcpy _strcpy
#define strlen _strlen
#define strncpy _strncpy
#define strpbrk _strpbrk
#define catgetmsg _catgetmsg
#define msglenzero1 _msglenzero1
#  ifdef __lint
#  define isascii _isascii
#  define isdigit _isdigit
#  endif /* __lint */
#endif

#include <varargs.h>
#include <ctype.h>
#include <nl_ctype.h>
#include <errno.h>
#include <msgcat.h>

#define	NULL		0	/* Null pointer address */
#define MAX_PARMS	9	/* maximum '!n' parameters */
#define NUMBERED	999

char	*strcpy(),
	*strchr();
char	*strcpy_to_end();

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef catread
#pragma _HP_SECONDARY_DEF _catread catread
#define catread _catread
#endif

int
catread(fd, setnum, msgnum, buf, buflen, va_alist)	/* VARARGS */
int	fd, setnum, msgnum, buflen;
char	*buf;
va_dcl
/*
 * This procedure reads a message specified by `setnum' and
 * `msgnum' into the buffer pointed by `buf' from the message
 * file specified by `fd'.  `Parms' are the character strings
 * to substitute the `!' mark.  These strings must match the
 * `!' mark in the message, see note in fmtmsg.
 */
{
	char	*msgbuf,
		*catgetmsg();
	int	msglen;
	int	msglenzero1();
	static int	fmtmsg();
	va_list	parms;

	/*
	 * For messages longer than buflen - 1, catgetmsg truncates the
	 * message and sets errno = ERANGE
	 */
	if (*catgetmsg(fd, setnum, msgnum, buf, buflen) == NULL)
		if ( msglenzero1() )
			return(NULL);
		else
			return(ERROR);
	msglen = strlen(buf);
	if ( strpbrk(buf,"!~") ) {	/* need to format */
		msgbuf = strcpy_to_end(buf,buflen);
		va_start(parms);
		msglen = fmtmsg(buf, buflen, msgbuf, msglen, parms);
		va_end(parms);
	}
	return(msglen);
}

static int
fmtmsg(dbuf, dlen, sbuf, slen, parms)
int dlen, slen;
register char *dbuf, *sbuf;
va_list parms;
/*
 * Format the destination buffer from the source buffer and parameters.  The
 * message in `sbuf' is copied to `dbuf' while inserting message parameters.
 *
 *  Note.  This procedure cannot check the existence of a message parameter
 *	   corresponding to a '!' mark.  The procedure assumes that a
 *	   parameter exists for every '!' in the message.  If a message
 *	   contains a '!' mark for which there is no parameter, program
 *	   action is indeterminate.  The ususal action is a system fault
 *	   and program termination.
 */
{
	register char	*sptr,	  /* Source buffer pointer */
			*dptr;	  /* Destination buffer pointer */
	register int schar,	  /* source character */
		     scharsize;	  /* size in bytes of schar */

	char	*parmbuf[10];	  /* parameter buffer */
	int	parmnum = 0;	  /* current parameter number */
	int	parm_status = 1,  /* Counter of '!' char */
		n,		  /* Next parameter number */
		plen,		  /* Next parameter length */
		rlen,		  /* Remaining buffer length */
		clen;		  /* Copy parameter length */

	sptr = sbuf;
	dptr = dbuf;
	while (sptr < &sbuf[slen]) {
		schar = CHARADV(sptr);
		scharsize = (schar & 0xff00 ? 2 : 1);	/* assumes HP15 */
		if (dptr > &dbuf[dlen - scharsize]) {  /* only whole chars */
			errno = ERANGE;
			break;
		}
		if (schar == '~') {   	/* '~'  == escape character */
			schar = CHARADV(sptr);
			PCHARADV(schar, dptr);
		}
		else if (schar == '!') {	/* '!' mark */
			/* Next check whether the '!' parameter is
			 * numbered or not; don't check that all
			 * parameters are numbered or none.  This
			 * check must have been done by gencat(1).
			 */
			schar = CHARAT(sptr);
			if (isascii(schar) && isdigit(schar)) {	/* '!n' mark */
				if (parm_status == 1) {
					parm_status = NUMBERED;
				}
				else if (parm_status != NUMBERED) {
					errno = EINVAL;
					return(ERROR);
				}
				n = schar - '0';
				if (n < 1 || n > MAX_PARMS) {
					errno = EINVAL;
					return(ERROR);
				}
				ADVANCE(sptr);
			}
			else {
				if (parm_status == NUMBERED) {
					errno = EINVAL;
					return(ERROR);
				}
				n = parm_status++;
				if (n > MAX_PARMS) { 
					errno = EINVAL;
					return(ERROR);
				}
			}
			while (parmnum < n)	/* save parameter addresses */
				parmbuf[++parmnum] = va_arg(parms, char *);
			plen = strlen(parmbuf[n]);
			rlen = sptr - dptr; /* leave room for \0 */
			clen = plen < rlen ? plen : rlen;
			strncpy(dptr, parmbuf[n], clen); /* insert parmeter */
			dptr += clen;
			if (plen > rlen) {	/* buffer over-run */
				errno = ERANGE;
				break;
			}
		}
		else {		/* regular character */
			PCHARADV(schar, dptr);
		}
	}
	*dptr = '\0';		/* terminate string */
	return (dptr - dbuf);	/* Return formatted message length */
}

static char *
strcpy_to_end(fbgn,buflen)
char *fbgn;
long buflen;
{
/*
 * copy string to end of buffer: |abc.....| => |.....abc| to allow room
 * for in-place formatting.
 */
	char *tbgn;
	long flen, tlen;

	flen = strlen(fbgn);
	tbgn = fbgn + buflen - flen - 1;
	if ( flen + 1 <= buflen/2 ) {	/* source & destination independent */
		return(strcpy(tbgn, fbgn));
	}
	else {				/* source & destination overlap */
		char *tend, *fend;

		fend = fbgn + flen;
		tend = tbgn + flen;
		while ( fbgn <= fend ) *tend-- = *fend--;
		return (tbgn);
	}
}

