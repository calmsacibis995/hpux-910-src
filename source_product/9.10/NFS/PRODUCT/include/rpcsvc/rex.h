/* 	@(#)rex.h	$Revision: 1.6.109.1 $	$Date: 91/11/19 14:42:44 $  */

/*
 * rex - remote execution server definitions
 */

/*      (c) Copyright 1988 Hewlett-Packard Company  */
/*      (c) Copyright Copyright 1985 Sun Microsystems, Inc. */

#ifndef _RPCSVC_REX_INCLUDED
#define _RPCSVC_REX_INCLUDED

#define	REXPROG		100017
#define	REXPROC_NULL	0	/* no operation */
#define	REXPROC_START	1	/* start a command */
#define	REXPROC_WAIT	2	/* wait for a command to complete */
#define	REXPROC_MODES	3	/* send the tty modes */
#define REXPROC_WINCH	4	/* signal a window change */
#define REXPROC_SIGNAL	5	/* other signals */

#define	REXVERS	1

/* flags for rst_flags field */
#define REX_INTERACTIVE		1	/* Interative mode */

struct rex_start {
  /*
   * Structure passed as parameter to start function
   */
	char	**rst_cmd;	/* list of command and args */
	char	*rst_host;	/* working directory host name */
	char	*rst_fsname;	/* working directory file system name */
	char	*rst_dirwithin;	/* working directory within file system */
	char	**rst_env;	/* list of environment */
	u_short	rst_port0;	/* port for stdin */
	u_short	rst_port1;	/* port for stdout */
	u_short	rst_port2;	/* port for stderr */
	u_long	rst_flags;	/* options - see #defines above */
};

bool_t xdr_rex_start();

struct rex_result {
  /*
   * Structure returned from the start function
   */
   	int	rlt_stat;	/* integer status code */
	char	*rlt_message;	/* string message for human consumption */
};
bool_t xdr_rex_result();

struct rex_ttymode {
    /*
     * Structure sent to set-up the tty modes
     */
	struct sgttyb basic;	/* standard unix tty flags */
	struct tchars more;	/* interrupt, kill characters, etc. */
	struct ltchars yetmore;	/* special Bezerkeley characters */
	u_long andmore;		/* and Berkeley modes */
};

bool_t xdr_rex_ttymode();
bool_t xdr_rex_ttysize();

/*
 * If TIOCGSIZE is defined by termio then struct ttysize is also
 * already defined in termio.h
 */

#ifndef TIOCGSIZE
struct ttysize {
	int     ts_lines;       /* number of lines on terminal */
	int     ts_cols;        /* number of columns on terminal */
};
#endif /* ~TIOCGSIZE */

#endif /* _RPCSVC_REX_INCLUDED */

