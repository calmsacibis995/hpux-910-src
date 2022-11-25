/* @(#) $Revision: 32.1 $ */       
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */

#define TIMEOUT 	0	/* seconds elapsing before log-off (normal) */
#ifndef NLS
#define MAILCHECK	"600"	/* 10 minutes */
#else NLS
tchar	MAILCHECK[] ={'6','0','0',0};	/* 10 minutes */
#endif NLS
