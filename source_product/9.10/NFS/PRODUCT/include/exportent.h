/*   @(#)exportent.h	$Revision: 1.1.109.3 $	$Date: 92/03/13 10:52:26 $  */
/*   (c) Copyright 1992 Hewlett-Packard Company   */

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 *  1.5 88/02/07 (C) 1986 SMI
 */


/*
 * Exported file system table, see exportent(3)
 */ 

#ifndef _EXPORTENT_INCLUDED
#define _EXPORTENT_INCLUDED

#define TABFILE "/etc/xtab"		/* where the table is kept */

/*
 * Options keywords
 */
#define ACCESS_OPT	"access"	/* machines that can mount fs */
#define ROOT_OPT	"root"		/* machines with root access of fs */
#define RO_OPT		"ro"		/* export read-only */
#define RW_OPT		"rw"		/* export read-mostly */
#define ANON_OPT	"anon"		/* uid for anonymous requests */
#ifndef hpux
#define SECURE_OPT	"secure"	/* require secure NFS for access */
#define WINDOW_OPT	"window"	/* expiration window for credential */
#endif
#ifdef hpux
#define ASYNC_OPT	"async"		/* all mounts will be asynchronous */
#endif

struct exportent {
	char *xent_dirname;	/* directory (or file) to export */
	char *xent_options;	/* options, as above */
};

extern FILE *setexportent();
extern void endexportent();
extern int remexportent();
extern int addexportent();
extern char *getexportopt();
extern struct exportent *getexportent();

#endif /*_EXPORTENT_INCLUDED*/
