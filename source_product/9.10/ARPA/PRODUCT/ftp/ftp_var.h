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
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)ftp_var.h	5.5 (Berkeley) 6/29/88
 *	$Header: ftp_var.h,v 1.5.109.1 91/11/21 11:49:44 kcs Exp $
 */

/*
 * FTP global variables.
 */

/*
 * Options and other state info.
 */
int	trace;			/* trace packets exchanged */
int	hash;			/* print # for each buffer transferred */
int	sendport;		/* use PORT cmd for each data connection */
int	verbose;		/* print messages coming back from server */
int	connected;		/* connected to server */
int	fromatty;		/* input is from a terminal */
int	interactive;		/* interactively prompt on m* cmds */
int	debug;			/* debugging level */
int	bell;			/* ring bell on cmd completion */
int	doglob;			/* glob local file names */
int	autologin;		/* establish user account on connection */
int	proxy;			/* proxy server connection active */
int	proxflag;		/* proxy connection exists */
int	sunique;		/* store files on server with unique name */
int	runique;		/* store local files with unique name */
int	mcase;			/* map upper to lower case for mget names */
int	ntflag;			/* use ntin ntout tables for name translation */
int	mapflag;		/* use mapin mapout templates on file names */
int	code;			/* return/reply code for ftp command */
int	crflag;			/* if 1, strip car. rets. on ascii gets */
char	pasv[64];		/* passive port for proxy data connection */
char	*altarg;		/* argv[1] with no shell-like preprocessing  */
char	ntin[17];		/* input translation table */
char	ntout[17];		/* output translation table */
#include <sys/param.h>
char	mapin[MAXPATHLEN];	/* input map template */
char	mapout[MAXPATHLEN];	/* output map template */
char	typename[32];		/* name of file transfer type */
int	type;			/* file transfer type */
char	structname[32];		/* name of file transfer structure */
int	stru;			/* file transfer structure */
char	formname[32];		/* name of file transfer format */
int	form;			/* file transfer format */
char	modename[32];		/* name of file transfer mode */
int	mode;			/* file transfer mode */
char	bytename[32];		/* local byte size in ascii */
int	bytesize;		/* local byte size in binary */

char	*hostname;		/* name of host connected to */

struct	servent *sp;		/* service spec for tcp/ftp */

#include <setjmp.h>
jmp_buf	toplevel;		/* non-local goto stuff for cmd scanner */

#include <stdio.h>
char	line[BUFSIZ];		/* input line buffer */
char	*stringbase;		/* current scan point in line buffer */
char	argbuf[BUFSIZ];		/* argument storage buffer */
char	*argbase;		/* current storage point in arg buffer */
int	margc;			/* count of arguments on input line */
char	**margv;		/* args parsed from input line */
int	margsize;		/* current size of arg array */
int     cpend;                  /* flag: if != 0, then pending server reply */
int	mflag;			/* flag: if != 0, then active multi command */

int	options;		/* used during socket creation */

/*
 * Format of command table.
 */
struct cmd {
	char	*c_name;	/* name of command */
	char	*c_help;	/* help string */
	char	c_bell;		/* give bell when command completes */
	char	c_conn;		/* must be connected to use command */
	char	c_proxy;	/* proxy server may execute */
	int	(*c_handler)();	/* function to call */
};

struct macel {
	char mac_name[9];	/* macro name */
	char *mac_start;	/* start of macro in macbuf */
	char *mac_end;		/* end of macro in macbuf */
};

int macnum;			/* number of defined macros */
struct macel macros[16];
char macbuf[4096];

extern	char *tail();
extern	char *index();
extern	char *rindex();
extern	char *remglob();
extern	int errno;
extern	char *mktemp();
extern	char *strncpy();
extern	char *strncat();
extern	char *strcat();
extern	char *strcpy();

#if	defined(__ANSI__) || defined(sparc) || defined(hpux)
typedef void sig_t;
#else
typedef int sig_t;
#endif
