/* RCSID strings for NFS/300 group */
/* @(#)$Revision: 12.4 $	$Date: 92/02/07 17:07:43 $  */

/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/clnt_perr.c,v $
 * $Revision: 12.4 $	$Author: indnetwk $
 * $State: Exp $   	$Locker:  $
 * $Date: 92/02/07 17:07:43 $
 *
 * Revision 12.3  90/08/30  11:53:22  11:53:22  prabha
 * Yellow Pages/YP -> Network Information Services/NIS changes made.
 * 
 * Revision 12.2  90/06/08  10:11:16  10:11:16  dlr (Dominic Ruffatto)
 * Cleaned up ifdefs.
 * 
 * Revision 12.1  90/03/20  14:46:05  14:46:05  dlr (Dominic Ruffatto)
 * Commented out strings following #else and #endif lines (ANSII-C compliance).
 * 
 * Revision 12.0  89/09/25  16:08:25  16:08:25  nfsmgr (Source Code Administrator)
 * Bumped RCS revision number to 12.0 for HP-UX 8.0.
 * 
 * Revision 11.3  89/02/13  15:02:18  15:02:18  dds (Darren Smith)
 * Changed ifdef around malloc/free in NAME SPACE defines to use #ifdef
 * _ANSIC_CLEAN.  Also changed uses of BUFSIZ to use their own define of 1024
 * because in most cases we don't really need 8K buffers. dds
 * 
 * Revision 1.1.10.13  89/02/13  14:01:25  14:01:25  cmahon (Christina Mahon)
 * Changed ifdef around malloc/free in NAME SPACE defines to use #ifdef
 * _ANSIC_CLEAN.  Also changed uses of BUFSIZ to use their own define of 1024
 * because in most cases we don't really need 8K buffers. dds
 * 
 * Revision 1.1.10.12  89/01/26  11:53:22  11:53:22  cmahon (Christina Mahon)
 * Changes for NAME SPACE CLEANUP and other general cleanup.  Added secondary
 * ddefs for all externally visible names, and #ifdefs for external references
 * that changed.  Removed ifdefs for KERNEL from user-space only files.  
 * Removed NLS ifdefs since we always build with NLS on. dds.
 * 
 * Revision 1.1.10.11  89/01/16  15:08:45  15:08:45  cmahon (Christina Mahon)
 * First pass for name space clean up -- add defines of functions in libc
 * whose name has already been changed inside "ifdef _NAMESPACE_CLEAN"
 * dds
 * 
 * Revision 11.0  89/01/13  14:48:15  14:48:15  root (The Super User)
 * initial checkin with rev 11.0 for release 7.0
 * 
 * Revision 10.9  88/11/10  14:16:49  14:16:49  mikey (Mike Shipley)
 * No Comment.
 * 
 * Revision 1.1.10.10  88/11/10  13:15:59  13:15:59  cmahon (Christina Mahon)
 * No Comment.
 * 
 * Revision 1.1.10.9  88/11/10  11:30:56  11:30:56  cmahon (Christina Mahon)
 * Took out references to strerror() for the non-NFS3.2 portion
 * of the code.
 * 
 * Revision 1.1.10.8  88/11/09  16:09:27  16:09:27  cmahon (Christina Mahon)
 * Updated from using sys_errlist[] to strerror() to get a text version of
 * an error associated with the value in errno.
 * 
 * Revision 1.1.10.7  88/09/09  11:40:58  11:40:58  cmahon (Christina Mahon)
 * Localized error messages that had been added (user level code only).
 * 
 * Revision 1.1.10.6  88/08/23  09:52:53  09:52:53  cmahon (Christina Mahon)
 * Converted the routines to call string routines that create the correct
 * error messages.  This was done because in NFS3.2, the string functions
 * are visible to the user.
 * 
 * Revision 1.1.10.5  88/03/24  16:32:58  16:32:58  cmahon (Christina Mahon)
 * Fix related to DTS CNDdm01106.  Now the NLS routines are only called once
 *  This improves NIS performance on the s800.
 * 
 * Revision 1.1.10.4  87/08/07  14:36:12  14:36:12  cmahon (Christina Mahon)
 * Changed sccs what strings and fixed some rcs what strings
 * 
 * Revision 1.1.10.3  87/07/16  22:07:23  22:07:23  cmahon (Christina Mahon)
 * Version merge with 300 code on July 16, 1987.
 * 
 * Revision 1.3  86/07/28  15:09:11  15:09:11  dacosta (Herve' Da Costa)
 * Noted name change from clnt_perror.c to clnt_perr.c (12 chars)  D. Pan
 * 
 * Revision 1.2  86/07/28  11:38:41  11:38:41  pan (Derleun Pan)
 * Header added.  
 * 
 * $Endlog$
 */

#if	defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: clnt_perr.c,v 12.4 92/02/07 17:07:43 indnetwk Exp $ (Hewlett-Packard)";
#endif

/*
 * clnt_perr.c, USED TO BE CALLED clnt_perror.cc   D. Pan 7/25/86
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define catgets 		_catgets
#define clnt_pcreateerror 	_clnt_pcreateerror	/* In this file */
#define clnt_perrno 		_clnt_perrno		/* In this file */
#define clnt_perror 		_clnt_perror		/* In this file */
#define clnt_spcreateerror 	_clnt_spcreateerror	/* In this file */
#define clnt_sperrno 		_clnt_sperrno		/* In this file */
#define clnt_sperror 		_clnt_sperror		/* In this file */
#define fprintf 		_fprintf
#define nl_sprintf 		_nl_sprintf
#define rpc_createerr 		_rpc_createerr		/* In rpc_data.c */
#define sprintf 		_sprintf
#define strcpy 			_strcpy
#define strerror 		_strerror
#define strlen 			_strlen

#ifdef _ANSIC_CLEAN
#define malloc 			_malloc
#endif /* _ANSIC_CLEAN */

#endif /* _NAME_SPACE_CLEAN */

#include <rpc/types.h>	/* <> */
#include <rpc/xdr.h>	/* <> */
#include <rpc/auth.h>	/* <> */
#include <rpc/clnt.h>	/* <> */
#include <rpc/rpc_msg.h>	/* <> */
#include <stdio.h>
#define NL_SETN 5	/* set number */
#include <nl_types.h>
static nl_catd nlmsg_fd;

/* HPNFS
 * There will be a series of changes to strerror() to replace what was
 * returned by sys_errlist[].  The 800 will get it for 7.0   mds  11/10/88
 */

char *strerror();

static char *buf_errno;

static char *
_buf_errno()
{

        if (buf_errno == 0)
		buf_errno = (char *)malloc(256);
        return (buf_errno);
}


static char *buf;

static char *
_buf()
{

        if (buf == 0)
		buf = (char *)malloc(512);
        return (buf);
}

#ifdef _NAMESPACE_CLEAN
#undef clnt_sperror
#pragma _HP_SECONDARY_DEF _clnt_sperror clnt_sperror
#define clnt_sperror _clnt_sperror
#endif

char *
clnt_sperror(rpch, s)
	CLIENT *rpch;
	char *s;
{
	struct rpc_err e;
	char *clnt_sperrno();
	char *str = _buf();
	char *strstart = str;
	char *errbuf;

	nlmsg_fd = _nfs_nls_catopen();

	CLNT_GETERR(rpch, &e);
	sprintf(str, (catgets(nlmsg_fd,NL_SETN,1, "%s: ")), s);
	str += strlen(str);
	switch (e.re_status) {
		case RPC_SUCCESS:
		case RPC_CANTENCODEARGS:
		case RPC_CANTDECODERES:
		case RPC_TIMEDOUT:
		case RPC_PROGUNAVAIL:
		case RPC_PROCUNAVAIL:
		case RPC_CANTDECODEARGS:
		case RPC_UNKNOWNPROTO:
		case RPC_FAILED:
			(void) strcpy (str, clnt_sperrno(e.re_status));
			str += strlen(str);
			break;
		case RPC_CANTSEND:
			(void) strcpy (str, clnt_sperrno(e.re_status));
			str += strlen(str);
			if ((errbuf = strerror(e.re_errno)) == NULL) 
				errbuf = " ";
			
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,2, "; errno = %s")),
			    errbuf);
			str += strlen(str);
			break;
	
		case RPC_CANTRECV:
			(void) strcpy (str, clnt_sperrno(e.re_status));
			str += strlen(str);
			if ((errbuf = strerror(e.re_errno)) == NULL) 
				errbuf = " ";
			
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,3, "; errno = %s")),
			    errbuf);
			str += strlen(str);
			break;
	
		case RPC_VERSMISMATCH:
			(void) strcpy (str, clnt_sperrno(e.re_status));
			str += strlen(str);
			nl_sprintf(str, (catgets(nlmsg_fd,NL_SETN,4, "; low version = %1$lu, high version = %2$lu")), e.re_vers.low, e.re_vers.high);
			str += strlen(str);
			break;
	
		case RPC_AUTHERROR:
			(void) strcpy (str, clnt_sperrno(e.re_status));
			str += strlen(str);
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,5, "; why = ")));
			str += strlen(str);
			switch (e.re_why) {
			case AUTH_OK:
				sprintf(str, (catgets(nlmsg_fd,NL_SETN,6, "AUTH_OK")));
				str += strlen(str);
				break;
	
			case AUTH_BADCRED:
				sprintf(str, (catgets(nlmsg_fd,NL_SETN,7, "AUTH_BOGUS_CREDENTIAL")));
				str += strlen(str);
				break;
	
			case AUTH_REJECTEDCRED:
				sprintf(str, (catgets(nlmsg_fd,NL_SETN,8, "AUTH_REJECTED_CREDENTIAL")));
				str += strlen(str);
				break;
	
			case AUTH_BADVERF:
				sprintf(str, (catgets(nlmsg_fd,NL_SETN,9, "AUTH_BOGUS_VERIFIER")));
				str += strlen(str);
				break;
	
			case AUTH_REJECTEDVERF:
				sprintf(str, (catgets(nlmsg_fd,NL_SETN,10, "AUTH_REJECTED_VERIFIER")));
				str += strlen(str);
				break;
	
			case AUTH_TOOWEAK:
				sprintf(str, (catgets(nlmsg_fd,NL_SETN,11, "AUTH_TOO_WEAK (remote error)")));
				str += strlen(str);
				break;
	
			case AUTH_INVALIDRESP:
				sprintf(str, (catgets(nlmsg_fd,NL_SETN,12, "AUTH_INVALID_RESPONSE")));
				str += strlen(str);
				break;
	
			default:
				sprintf(str, (catgets(nlmsg_fd,NL_SETN,13, "AUTH_UNKNOWN_FAILURE")));
				str += strlen(str);
				break;
			}
			break;
	
		case RPC_PROGVERSMISMATCH:
			(void) strcpy (str, clnt_sperrno(e.re_status));
			str += strlen(str);
			nl_sprintf(str, (catgets(nlmsg_fd,NL_SETN,14, "; low version = %1$lu, high version = %2$lu")), e.re_vers.low, e.re_vers.high);
			str += strlen(str);
			break;
	
		default:
			nl_sprintf(str, (catgets(nlmsg_fd,NL_SETN,15, "RPC_UNKNOWN_FAILURE; s1 = %1$lu, s2 = %2$lu")), e.re_lb.s1, e.re_lb.s2);
			str += strlen(str);
			break;
	}
	sprintf(str, (catgets(nlmsg_fd,NL_SETN,16, "\n")));
	str += strlen(str);
	return(strstart);
}

#ifdef _NAMESPACE_CLEAN
#undef clnt_perror
#pragma _HP_SECONDARY_DEF _clnt_perror clnt_perror
#define clnt_perror _clnt_perror
#endif

void
clnt_perror(rpch, s)
        CLIENT *rpch;
        char *s;
{
	char *clnt_sperror();
        (void) fprintf(stderr,"%s",clnt_sperror(rpch,s));
}


/*
 * This interface for use by clntrpc
 */

#ifdef _NAMESPACE_CLEAN
#undef clnt_sperrno
#pragma _HP_SECONDARY_DEF _clnt_sperrno clnt_sperrno
#define clnt_sperrno _clnt_sperrno
#endif

char *
clnt_sperrno(num)
	enum clnt_stat num;
{
	 char *str = _buf_errno();
	 char *strstart = str;

	nlmsg_fd = _nfs_nls_catopen();

	switch (num) {
		case RPC_SUCCESS:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,17, "RPC_SUCCESS")));
			str += strlen(str);
			break;
	
		case RPC_CANTENCODEARGS:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,18, "RPC_CANT_ENCODE_ARGS")));
			str += strlen(str);
			break;
	
		case RPC_CANTDECODERES:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,19, "RPC_CANT_DECODE_RESULTS")));
			str += strlen(str);
			break;
	
		case RPC_CANTSEND:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,20, "RPC_CANT_SEND")));
			str += strlen(str);
			break;
	
		case RPC_CANTRECV:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,21, "RPC_CANT_RECV")));
			str += strlen(str);
			break;
	
		case RPC_TIMEDOUT:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,22, "RPC_TIMED_OUT")));
			str += strlen(str);
			break;
	
		case RPC_VERSMISMATCH:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,23, "RPC_VERSION_MISMATCH")));
			str += strlen(str);
			break;
	
		case RPC_AUTHERROR:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,24, "RPC_AUTH_ERROR")));
			str += strlen(str);
			break;
	
		case RPC_PROGUNAVAIL:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,25, "RPC_REMOTE_PROGRAM_UNAVAILABLE")));
			str += strlen(str);
			break;
	
		case RPC_PROGVERSMISMATCH:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,26, "RPC_PROGRAM_MISMATCH")));
			str += strlen(str);
			break;
	
		case RPC_PROCUNAVAIL:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,27, "RPC_UNKNOWN_PROCEDURE")));
			str += strlen(str);
			break;
	
		case RPC_CANTDECODEARGS:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,28, "RPC_CANT_DECODE_ARGS")));
			str += strlen(str);
			break;
		case RPC_UNKNOWNHOST:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,29, "RPC_UNKNOWNHOST")));
			str += strlen(str);
			break;
		case RPC_PMAPFAILURE:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,30, "RPC_PMAP_FAILURE")));
			str += strlen(str);
			break;
		case RPC_PROGNOTREGISTERED:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,31, "RPC_PROG_NOT_REGISTERED")));
			str += strlen(str);
			break;
		case RPC_SYSTEMERROR:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,32, "RPC_SYSTEM_ERROR")));
			str += strlen(str);
			break;
		case RPC_UNKNOWNPROTO:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,37, "RPC_UNKNOWN_PROTOCOL")));
			str += strlen(str);
			break;
		case RPC_FAILED:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,38, "RPC_FAILED")));
			str += strlen(str);
			break;
	}
	return(strstart);
}

#ifdef _NAMESPACE_CLEAN
#undef clnt_perrno
#pragma _HP_SECONDARY_DEF _clnt_perrno clnt_perrno
#define clnt_perrno _clnt_perrno
#endif

void
clnt_perrno(num)
        enum clnt_stat num;
{
	char *clnt_sperrno();
        (void) fprintf(stderr,"%s",clnt_sperrno(num));
}

/*
 * A handle on why an rpc creation routine failed (returned NULL.)
 */
extern struct rpc_createerr rpc_createerr;	/* Moved to rpc_data.c */

#ifdef _NAMESPACE_CLEAN
#undef clnt_spcreateerror
#pragma _HP_SECONDARY_DEF _clnt_spcreateerror clnt_spcreateerror
#define clnt_spcreateerror _clnt_spcreateerror
#endif

char *
clnt_spcreateerror(s)
	char *s;
{
        char *str = _buf();
        char *strstart = str;
        char *clnt_sperrno();
	char *errbuf;

	nlmsg_fd = _nfs_nls_catopen();

	sprintf(str, (catgets(nlmsg_fd,NL_SETN,33, "%s: ")), s);
	str += strlen(str);
        (void) strcpy (str, clnt_sperrno(rpc_createerr.cf_stat));
        str += strlen(str);
	switch (rpc_createerr.cf_stat) {
		case RPC_PMAPFAILURE:
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,34, " - ")));
			str += strlen(str);
			(void) strcpy (str,
				clnt_sperrno(rpc_createerr.cf_error.re_status));
			str += strlen(str);
			break;

		case RPC_SYSTEMERROR:
			if ((errbuf = strerror(rpc_createerr.cf_error.re_errno))				== NULL) 
				errbuf = " ";
			
			sprintf(str, (catgets(nlmsg_fd,NL_SETN,35, " - %s")),
			    errbuf);
			str += strlen(str);
			break;

	}
	sprintf(str, (catgets(nlmsg_fd,NL_SETN,36, "\n")));
	str += strlen(str);
	return(strstart);
}

#ifdef _NAMESPACE_CLEAN
#undef clnt_pcreateerror
#pragma _HP_SECONDARY_DEF _clnt_pcreateerror clnt_pcreateerror
#define clnt_pcreateerror _clnt_pcreateerror
#endif

void
clnt_pcreateerror(s)
        char *s;
{
	char *clnt_spcreateerror();
        (void) fprintf(stderr,"%s",clnt_spcreateerror(s));
}
