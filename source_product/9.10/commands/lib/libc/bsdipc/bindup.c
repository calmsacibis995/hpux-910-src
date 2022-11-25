/* $Header: bindup.c,v 66.1 90/01/08 15:19:24 jmc Exp $ */

#ifdef _NAMESPACE_CLEAN
#define strcpy _strcpy
#define strcat _strcat
#define res_query _res_query
#define res_init _res_init
/* local */
#define BINDup _BINDup
#endif

#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#ifndef hpux
#include <arpa/inet.h>
#endif
#include <arpa/nameser.h>
#include <resolv.h>

#if PACKETSZ > 1024
#define	MAXPACKET	PACKETSZ
#else
#define	MAXPACKET	1024
#endif

static int usingBIND = 0;

#ifdef _NAMESPACE_CLEAN
#undef BINDup
#pragma _HP_SECONDARY_DEF _BINDup BINDup
#define BINDup _BINDup
#endif

BINDup(flag)
int flag;
{
	static int firsttime = 1;
	char answer[MAXPACKET];
	char name[MAXDNAME];
	int n;

	if(firsttime || flag){
		firsttime = 0;
		/*
		 * Need to initialize if not done already 
		 * so the default domain is set.  We don't
		 * care if res_init() fails.  Attempt the
		 * query anyway since we don't really care
		 * about the answer.
		 */
	
		if (!(_res.options & RES_INIT))
			res_init();
	
		strcpy(name, "localhost");
		if(_res.defdname[0] != '\0'){
			strcat(name, ".");
			strcat(name, _res.defdname);
		}
	
		/*
		 * Send the query and get the response.
		 * The response is not looked at since
		 * all we want to know is if the name
		 * server is running.  The errno will
		 * be ECONNREFUSED if the name server is
		 * not running.
		 */
	
		errno = 0;
		n = res_query(name, C_IN, T_A, answer, MAXPACKET);
	
		if((n < 0) && (errno == ECONNREFUSED))
			usingBIND = 0; 
		else
			usingBIND = 1;

	}
	return(usingBIND);
}
