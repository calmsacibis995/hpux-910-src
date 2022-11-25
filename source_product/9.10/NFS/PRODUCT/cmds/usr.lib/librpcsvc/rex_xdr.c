/* 	@(#)rex_xdr.c	$Revision: 1.5.109.2 $	$Date: 93/09/02 09:24:58 $  */
static  char rcsid[] = "@(#)rex_xdr.c:  $Revision: 1.5.109.2 $ $Date: 93/09/02 09:24:58 $";

/*
 * rex_xdr - remote execution external data representations
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

#ifdef PATCH_STRING
static char *patch_3066="@(#) PATCH_9.0: rex_xdr.o $Revision: 1.5.109.2 $ 93/09/01 PHNE_3066";
#endif


#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/errno.h>
#include <sys/ioctl.h>

#ifdef hpux
#include <sgtty.h>
#include <bsdterm.h>
#include <sys/termio.h>
#endif hpux
#include <rpcsvc/rex.h>

/*
 * xdr_rex_start - process the command start structure
 */
xdr_rex_start(xdrs, rst)
	XDR *xdrs;
	struct rex_start *rst;
{
	return
		xdr_argv(xdrs, &rst->rst_cmd) &&
		xdr_string(xdrs, &rst->rst_host, 1024) &&
		xdr_string(xdrs, &rst->rst_fsname, 1024) &&
		xdr_string(xdrs, &rst->rst_dirwithin, 1024) &&
		xdr_argv(xdrs, &rst->rst_env) &&
		xdr_u_short(xdrs, &rst->rst_port0) &&
		xdr_u_short(xdrs, &rst->rst_port1) &&
		xdr_u_short(xdrs, &rst->rst_port2) &&
		xdr_u_long(xdrs, &rst->rst_flags);
}

xdr_argv(xdrs, argvp)
	XDR *xdrs;
	char ***argvp;
{
	register char **argv = *argvp;
	register char **ap;
	int i, count;

	/*
	 * find the number of args to encode or free
	 */
	if ((xdrs->x_op) != XDR_DECODE)
		for (count = 0, ap = argv; *ap != 0; ap++)
			count++;
	/* XDR the count */
	if (!xdr_u_int(xdrs, &count))
		return (FALSE);

	/*
	 * now deal with the strings
	 */
	if (xdrs->x_op == XDR_DECODE) {
		*argvp = argv = (char **)mem_alloc((count+1)*sizeof (char **));
		for (i = 0; i <= count; i++)	/* Note: <=, not < */
			argv[i] = 0;
	}

	for (i = 0, ap = argv; i < count; i++, ap++)
		if (!xdr_string(xdrs, ap, 10240))
			return (FALSE);

	if (xdrs->x_op == XDR_FREE && argv != NULL) {
		mem_free(argv, (count+1)*sizeof (char **));
		*argvp = NULL;
	}
	return (TRUE);
}

/*
 * xdr_rex_result - process the result of a start or wait operation
 */
xdr_rex_result(xdrs, result)
	XDR *xdrs;
	struct rex_result *result;
{
	return
		xdr_int(xdrs, &result->rlt_stat) &&
		xdr_string(xdrs, &result->rlt_message, 1024);

}

/*
 * xdr_rex_ttymode - process the tty mode information
 */
xdr_rex_ttymode(xdrs, mode)
	XDR *xdrs;
	struct rex_ttymode *mode;
{
  int six = 6;
  int four = 4;
  char *speedp = NULL;
  char *morep = NULL;
  char *yetmorep = NULL;
#ifdef hpux
  short flags = mode->basic.sg_flags;	/* sg_flags is an int */
  bool_t ret;
#endif hpux

  if (xdrs->x_op != XDR_FREE) {
  	speedp = &mode->basic.sg_ispeed;
	morep = (char *)&mode->more;
  	yetmorep = (char *)&mode->yetmore;
  }
#ifdef hpux
  ret = xdr_bytes(xdrs, &speedp, &four, 4) &&
	xdr_short(xdrs, &flags) &&
	xdr_bytes(xdrs, &morep, &six, 6) &&
	xdr_bytes(xdrs, &yetmorep, &six, 6) &&
	xdr_u_long(xdrs, &mode->andmore);
  if (xdrs->x_op == XDR_DECODE )
	mode->basic.sg_flags = flags;
#else hpux
  return
	xdr_bytes(xdrs, &speedp, &four, 4) &&
	xdr_short(xdrs, &mode->basic.sg_flags) &&
	xdr_bytes(xdrs, &morep, &six, 6) &&
	xdr_bytes(xdrs, &yetmorep, &six, 6) &&
	xdr_u_long(xdrs, &mode->andmore);
#endif hpux
}


/*
 * xdr_rex_ttysize - process the tty size information
 */
xdr_rex_ttysize(xdrs, size)
	XDR *xdrs;
	struct ttysize *size;
{
  return
	xdr_short(xdrs, &size->ts_lines) &&
	xdr_short(xdrs, &size->ts_cols);
}
