/*
 @(#)rpc_svcout:	$Revision: 1.9.109.4 $	$Date: 95/01/04 12:15:55 $  
*/

#ifndef lint
#ifdef PATCH_STRING
static char rcsid[]="@(#) PATCH_9.X: rpc_svuout.c $Revision: 1.9.109.4 $Date: 94/06/22 $ PHNE_4902";
#endif
#endif
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * rpc_svcout.c, Server-skeleton outputter for the RPC protocol compiler
 * Copyright (C) 1987, Sun Microsytsems, Inc. 
 */
#include <stdio.h>
#include "rpc_parse.h"
#include "rpc_util.h"

static char RQSTP[] = "rqstp";
static char TRANSP[] = "transp";
static char ARG[] = "argument";
static char RESULT[] = "result";
static char ROUTINE[] = "local";

/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
char _errbuf[256];      /* For all messages */

/*
 * write most of the service, that is, everything but the registrations. 
 * Bkelley - 24 Jan 94 - Added passing name
 */
void
write_most(infile)
	char *infile;		/* our name */
{
	list *l;
	definition *def;
	version_list *vp;

	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind == DEF_PROGRAM) {
			for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
				f_print(fout, "\nstatic void ");
				pvname(def->def_name, vp->vers_num);
				f_print(fout, "();");
			}
		}
	}
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
        if (timerflag)
                f_print(fout, "\nstatic void closedown();");

	f_print(fout, "\n\n");

/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
        if (inetdflag) {
                f_print(fout, "\nstatic int _rpcpmstart = 0;");
                f_print(fout, "\t\t/* Started by a port monitor ? */\n");
                f_print(fout, "static int _rpcfdtype = 0;");
                f_print(fout, "\t\t/* Whether Stream or Datagram ? */\n");
                if (timerflag) {
                        f_print(fout, "static int _rpcsvcdirty = 0;");
                        f_print(fout, "\t\t/* Still serving ? */\n");
                }
        }

	f_print(fout, "main()\n");
	f_print(fout, "{\n");
	f_print(fout, "\tSVCXPRT *%s;\n", TRANSP);
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
        if (inetdflag) {
                write_inetmost(infile); /* Includes call to write_rpc_svc_fg() */
        } else {

		f_print(fout, "\n");
                print_pmapunset("\t");
	}
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
        if (logflag && !inetdflag) {
                open_log_file(infile, "\t");
        }

}



/*
 * write a registration for the given transport 
 */
void
write_register(transp)
	char *transp;
{
	list *l;
	definition *def;
	version_list *vp;
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 * NOTE: sp added frequently to sprintf for formatting for inetd, 
 *       so no specific comments on this additions in this routine.
 */
        char *sp;
        int isudp;
        char tmpbuf[32];

        if (inetdflag)
                sp = "\t";
        else
                sp = "";
        if (streq(transp, "udp"))
                isudp = 1;
        else
                isudp = 0;

	f_print(fout, "\n");
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */

        if (inetdflag) {
            f_print(fout, "\tif ((_rpcfdtype == 0) || (_rpcfdtype == %s)) {\n",
                                isudp ? "SOCK_DGRAM" : "SOCK_STREAM");
        }

/*
 * Fix SR#5003-189878  DTS#INDaa17889 use svcfd_create for tcp based 
 * rpc program starting from inetd. 
 */
        if (inetdflag && !isudp) {
      		f_print(fout, "%s\tif (_rpcpmstart)\n", sp);
                f_print(fout, "%s\t\t%s = svcfd_create(sock, 0, 0);\n", sp, TRANSP);
                f_print(fout, "%s\telse\n", sp);
                f_print(fout, "\t");
        }
        f_print(fout, "%s\t%s = svc%s_create(%s",
                sp, TRANSP, transp, inetdflag? "sock": "RPC_ANYSOCK");
        if (!isudp){
		f_print(fout, ", 0, 0");
	}
	f_print(fout, ");\n");
	f_print(fout, "%s\tif (%s == NULL) {\n", sp, TRANSP);
/*
 * Fix SR#5003-189878  DTS#INDaa17889 wrong syslog message
 */
	(void) sprintf(_errbuf, "cannot create %s service.", transp);
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
        (void) sprintf(tmpbuf, "%s\t\t", sp);
        print_err_message(tmpbuf);

	f_print(fout, "%s\t\texit(1);\n", sp);
	f_print(fout, "%s\t}\n", sp);

/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
        if (inetdflag) {
                f_print(fout, "%s\tif (!_rpcpmstart)\n\t", sp);
                f_print(fout, "%s\tproto = IPPROTO_%s;\n",
                                sp, isudp ? "UDP": "TCP");
        }

	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind != DEF_PROGRAM) {
			continue;
		}
		for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
			f_print(fout,
				"%s\tif (!svc_register(%s, %s, %s, ",
				sp, TRANSP, def->def_name, vp->vers_name);
			pvname(def->def_name, vp->vers_num);
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
                        if (inetdflag)
                                f_print(fout, ", proto)) {\n");
                        else
                                f_print(fout, ", IPPROTO_%s)) {\n",
                                        isudp ? "UDP": "TCP");
                        (void) sprintf(_errbuf, 
					"unable to register (%s, %s, %s).",
                                        def->def_name, vp->vers_name, transp);
                        print_err_message(tmpbuf);

			f_print(fout, "%s\t\texit(1);\n", sp);
			f_print(fout, "%s\t}\n", sp);
		}
	}
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
        if (inetdflag)
                f_print(fout, "\t}\n");
}


/*
 * write the rest of the service 
 */
void
write_rest()
{
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
        f_print(fout, "\n");
        if (inetdflag) {
                f_print(fout, "\tif (%s == (SVCXPRT *)NULL) {\n", TRANSP);
                (void) sprintf(_errbuf, "could not create a handle");
                print_err_message("\t\t");
                f_print(fout, "\t\texit(1);\n");
                f_print(fout, "\t}\n");
                if (timerflag) {
                     f_print(fout, "\tif (_rpcpmstart) {\n");
                     f_print(fout, "\t\t(void) signal(SIGALRM, closedown);\n");
                     f_print(fout, "\t\t(void) alarm(_RPCSVC_CLOSEDOWN);\n");
                     f_print(fout, "\t}\n");
                }
        }

	f_print(fout, "\tsvc_run();\n");
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
        (void) sprintf(_errbuf, "svc_run returned");
        print_err_message("\t");

	f_print(fout, "\texit(1);\n");
	f_print(fout, "}\n");
}

void
write_programs(storage)
	char *storage;
{
	list *l;
	definition *def;

	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind == DEF_PROGRAM) {
			write_program(def, storage);
		}
	}
}


static
write_program(def, storage)
	definition *def;
	char *storage;
{
	version_list *vp;
	proc_list *proc;
	int filled;

	for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
		f_print(fout, "\n");
		if (storage != NULL) {
			f_print(fout, "%s ", storage);
		}
		f_print(fout, "void\n");
		pvname(def->def_name, vp->vers_num);
		f_print(fout, "(%s, %s)\n", RQSTP, TRANSP);
		f_print(fout, "	struct svc_req *%s;\n", RQSTP);
		f_print(fout, "	SVCXPRT *%s;\n", TRANSP);
		f_print(fout, "{\n");

		filled = 0;
		f_print(fout, "\tunion {\n");
		for (proc = vp->procs; proc != NULL; proc = proc->next) {
			if (streq(proc->arg_type, "void")) {
				continue;
			}
			filled = 1;
			f_print(fout, "\t\t");
			ptype(proc->arg_prefix, proc->arg_type, 0);
			pvname(proc->proc_name, vp->vers_num);
			f_print(fout, "_arg;\n");
		}
		if (!filled) {
			f_print(fout, "\t\tint fill;\n");
		}
		f_print(fout, "\t} %s;\n", ARG);
		f_print(fout, "\tchar *%s;\n", RESULT);
		f_print(fout, "\tbool_t (*xdr_%s)(), (*xdr_%s)();\n", ARG, RESULT);
		f_print(fout, "\tchar *(*%s)();\n", ROUTINE);
		f_print(fout, "\n");
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
                if (timerflag)
                        f_print(fout, "\t_rpcsvcdirty = 1;\n");

		f_print(fout, "\tswitch (%s->rq_proc) {\n", RQSTP);

		if (!nullproc(vp->procs)) {
			f_print(fout, "\tcase NULLPROC:\n");
			f_print(fout, "\t\tsvc_sendreply(%s, xdr_void, NULL);\n", TRANSP);
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
                        print_return("\t\t");
                        f_print(fout, "\n");

		}
		for (proc = vp->procs; proc != NULL; proc = proc->next) {
			f_print(fout, "\tcase %s:\n", proc->proc_name);
			f_print(fout, "\t\txdr_%s = xdr_%s;\n", ARG, 
				stringfix(proc->arg_type));
			f_print(fout, "\t\txdr_%s = xdr_%s;\n", RESULT, 
				stringfix(proc->res_type));
			f_print(fout, "\t\t%s = (char *(*)()) ", ROUTINE);
			pvname(proc->proc_name, vp->vers_num);
			f_print(fout, ";\n");
			f_print(fout, "\t\tbreak;\n\n");
		}
		f_print(fout, "\tdefault:\n");
		printerr("noproc", TRANSP);
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
                print_return("\t\t");

		f_print(fout, "\t}\n");
/*
 * HPNFS
 * This change was made to accomodate the difference in the number of
 * parameters between BSD memset() and HP-UX memset().
 */
#ifdef hpux
		f_print(fout, "#ifdef hpux\n");
		f_print(fout, "\tmemset(&%s, 0, sizeof(%s));\n", ARG, ARG);
		f_print(fout, "#else /* hpux */\n");
		f_print(fout, "\tmemset(&%s, sizeof(%s));\n", ARG, ARG);
		f_print(fout, "#endif /* hpux */\n");
#else  hpux
		f_print(fout, "\tmemset(&%s, sizeof(%s));\n", ARG, ARG);
#endif hpux
		printif("getargs", TRANSP, "&", ARG);
		printerr("decode", TRANSP);
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
                print_return("\t\t");
		f_print(fout, "\t}\n");

		f_print(fout, "\t%s = (*%s)(&%s, %s);\n", RESULT, ROUTINE, ARG,
			RQSTP);
		f_print(fout, 
			"\tif (%s != NULL && !svc_sendreply(%s, xdr_%s, %s)) {\n",
			RESULT, TRANSP, RESULT, RESULT);
		printerr("systemerr", TRANSP);
		f_print(fout, "\t}\n");

		printif("freeargs", TRANSP, "&", ARG);
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
                (void) sprintf(_errbuf, "unable to free arguments");
                print_err_message("\t\t");

		f_print(fout, "\t\texit(1);\n");
		f_print(fout, "\t}\n");
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
                print_return("\t");

		f_print(fout, "}\n\n");
	}
}

static
printerr(err, transp)
	char *err;
	char *transp;
{
	f_print(fout, "\t\tsvcerr_%s(%s);\n", err, transp);
}

static
printif(proc, transp, prefix, arg)
	char *proc;
	char *transp;
	char *prefix;
	char *arg;
{
	f_print(fout, "\tif (!svc_%s(%s, xdr_%s, %s%s)) {\n",
		proc, transp, arg, prefix, arg);
}


nullproc(proc)
	proc_list *proc;
{
	for (; proc != NULL; proc = proc->next) {
		if (streq(proc->proc_num, "0")) {
			return (1);
		}
	}
	return (0);
}


/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
write_inetmost(infile)
        char *infile;
{
        f_print(fout, "\tint sock;\n");
        f_print(fout, "\tint proto;\n");
        f_print(fout, "\tstruct sockaddr_in saddr;\n");
        f_print(fout, "\tint asize = sizeof(saddr);\n");
/*
 * HPNFS
 */
        f_print(fout, "\tint fd_limit;\n");


        f_print(fout, "\n");
        f_print(fout,
        "\tif (getsockname(0, (struct sockaddr *)&saddr, &asize) == 0) {\n");
        f_print(fout, "\t\tint ssize = sizeof(int);\n\n");
        f_print(fout, "\t\tif (saddr.sin_family != AF_INET)\n");
        f_print(fout, "\t\t\texit(1);\n");
        f_print(fout, "\t\tif (getsockopt(0, SOL_SOCKET, SO_TYPE,\n");
        f_print(fout, "\t\t\t\t(char *)&_rpcfdtype, &ssize) == -1)\n");
        f_print(fout, "\t\t\texit(1);\n");
        f_print(fout, "\t\tsock = 0;\n");
        f_print(fout, "\t\t_rpcpmstart = 1;\n");
        f_print(fout, "\t\tproto = 0;\n");
        open_log_file(infile, "\t\t");
        f_print(fout, "\t} else {\n");
        write_rpc_svc_fg(infile, "\t\t");
        f_print(fout, "\t\tsock = RPC_ANYSOCK;\n");
        print_pmapunset("\t\t");        
        f_print(fout, "\t}\n");
}


static
print_return(space)
        char *space;
{
        if (exitnow)
                f_print(fout, "%sexit(0);\n", space);
        else {
                if (timerflag)
                        f_print(fout, "%s_rpcsvcdirty = 0;\n", space);
                f_print(fout, "%sreturn;\n", space);
        }
}

static
print_pmapunset(space)
        char *space;
{
        list *l;
        definition *def;
        version_list *vp;

        for (l = defined; l != NULL; l = l->next) {
                def = (definition *) l->val;
                if (def->def_kind == DEF_PROGRAM) {
                        for (vp = def->def.pr.versions; vp != NULL;
                                        vp = vp->next) {
                                f_print(fout, "%s(void) pmap_unset(%s, %s);\n",
                                        space, def->def_name, vp->vers_name);
                        }
                }
        }
}

static
print_err_message(space)
        char *space;
{
        if (logflag)
                f_print(fout, "%ssyslog(LOG_ERR, \"%s\");\n", space, _errbuf);
        else if (inetdflag)
                f_print(fout, "%s_msgout(\"%s\");\n", space, _errbuf);
        else
                f_print(fout, "%sfprintf(stderr, \"%s\\n\");\n", space, _errbuf);
}

/*
 * Write the server auxiliary function ( _msgout, timeout)
 */
void
write_svc_aux()
{
        if (!logflag)
                write_msg_out();
        write_timeout_func();
}

/*
 * Write the _msgout function
 */
static
write_msg_out()
{
        f_print(fout, "\n");
        f_print(fout, "static\n");
        f_print(fout, "_msgout(msg)\n");
        f_print(fout, "\tchar *msg;\n");
        f_print(fout, "{\n");
        f_print(fout, "#ifdef RPC_SVC_FG\n");
        f_print(fout, "\tif (_rpcpmstart)\n");
        f_print(fout, "\t\tsyslog(LOG_ERR, msg);\n");
        f_print(fout, "\telse\n");
        f_print(fout, "\t\t(void) fprintf(stderr, \"%%s\\n\", msg);\n");
        f_print(fout, "#else\n");
        f_print(fout, "\tsyslog(LOG_ERR, msg);\n");
        f_print(fout, "#endif\n");
        f_print(fout, "}\n");
}

/*
 * Write the timeout function
 */
static
write_timeout_func()
{
        if (!timerflag)
                return;
        f_print(fout, "\n");
        f_print(fout, "static void\n");
        f_print(fout, "closedown()\n");
        f_print(fout, "{\n");
        f_print(fout, "\tif (_rpcsvcdirty == 0) {\n");
        f_print(fout, "\t\textern fd_set svc_fdset;\n");
        f_print(fout, "\t\tstatic int size;\n");
        f_print(fout, "\t\tint i, openfd;\n");
        f_print(fout, "\n");
        f_print(fout, "\t\tif (_rpcfdtype == SOCK_DGRAM)\n");
        f_print(fout, "\t\t\texit(0);\n");
        f_print(fout, "\t\tif (size == 0)\n");
        f_print(fout, "\t\t\tsize = getdtablesize();\n");
        f_print(fout,
                "\t\tfor (i = 0, openfd = 0; i < size && openfd < 2; i++)\n");
        f_print(fout, "\t\t\tif (FD_ISSET(i, &svc_fdset))\n");
        f_print(fout, "\t\t\t\topenfd++;\n");
        f_print(fout, "\t\tif (openfd <= 1)\n");
        f_print(fout, "\t\t\texit(0);\n");
        f_print(fout, "\t}\n");
        f_print(fout, "\t(void) alarm(_RPCSVC_CLOSEDOWN);\n");
        f_print(fout, "}\n");

/* HPNFS
 * the getdtablesize() cannot be found in HP-UX 9.0 libraries,
 * so we dispatch this call with a function, if future HP-UX
 * supports this system call, we can simply delete this function.
 * 06/10/1993 Wusheng Chen IND
 */
        f_print(fout, "\nint\n");
        f_print(fout, "getdtablesize()\n");
        f_print(fout, "{\n");
        f_print(fout, "\tstruct rlimit lmt;\n\n");
        f_print(fout, "\tif (getrlimit(RLIMIT_NOFILE,&lmt)) return(-1);\n");
        f_print(fout, "\treturn(lmt.rlim_cur);\n");
        f_print(fout, "}\n");
}


/*
 * Support for backgrounding the server if self started.
 */
static
write_rpc_svc_fg(infile, sp)
        char *infile;
        char *sp;
{
        f_print(fout, "#ifndef RPC_SVC_FG\n");
        f_print(fout, "%sint i, pid;\n\n", sp);
        f_print(fout, "%spid = fork();\n", sp);
        f_print(fout, "%sif (pid < 0) {\n", sp);
        f_print(fout, "%s\tperror(\"cannot fork\");\n", sp);
        f_print(fout, "%s\texit(1);\n", sp);
        f_print(fout, "%s}\n", sp);
        f_print(fout, "%sif (pid)\n", sp);
        f_print(fout, "%s\texit(0);\n", sp);
/*
 * HPNFS
 * Sun's code close file descriptor up to 20 (default on SunOS)
 * We choose to consult the file descrptor table size and
 * close all possible opened fd.
 * 07/28/1993 Wusheng Chen IND
 */
        f_print(fout, "%sif ((fd_limit=getdtablesize())<1) exit(1);\n",sp);
        f_print(fout, "%sfor (i = 0 ; i < fd_limit ; i++)\n", sp);
        f_print(fout, "%s\t(void) close(i);\n", sp);
/*
 * HPNFS
 * Sun use a number to indicate O_RDWR open mode,
 * this is potentially unportable, the better way
 * is specify the open mode with O_RDWR which is
 * defined in fcntl.h
 * 06/10/1993 Wusheng Chen IND
 */
        f_print(fout, "%si = open(\"/dev/console\", O_RDWR);\n", sp);
        f_print(fout, "%s(void) dup2(i, 1);\n", sp);
        f_print(fout, "%s(void) dup2(i, 2);\n", sp);
/* Commented by Wusheng Chen */
/*      f_print(fout, "%si = open(\"/dev/tty\", O_RDWR);\n", sp);
        f_print(fout, "%sif (i >= 0) {\n", sp);
        f_print(fout, "#ifdef SUN\n");
        f_print(fout, "%s\t(void) ioctl(i, TIOCNOTTY, (char *)NULL);\n", sp);
        f_print(fout, "#endif\n");
        f_print(fout, "%s\t(void) close(i);\n", sp);
        f_print(fout, "%s}\n", sp); */
/*
 * HPNFS
 * TIOCNOTTY is a BSD ioctl call which is not supported on
 * HP-UX. It has been decided the proper alternative for HP-UX
 * is setsid().
 * 06/10/1993 Wusheng Chen IND
 */
        f_print(fout, "%ssetsid();\n",sp);

        if (!logflag)
                open_log_file(infile, sp);
        f_print(fout, "#endif\n");
        if (logflag)
                open_log_file(infile, sp);
}

static
open_log_file(infile, sp)
        char *infile;
        char *sp;
{
        char *s;

        s = rindex(infile, '.');
        if (s)
                *s = '\0';
        f_print(fout,"%sopenlog(\"%s\", LOG_PID, LOG_DAEMON);\n", sp, infile);
        if (s)
                *s = '.';
}


/*
 * This routine will write the necessary include to let the
 * signal handler function properly.
 */
void
write_sig_hand()
{
	f_print(fout, "#include <signal.h>\n\n");
	f_print(fout, "void un_register_prog(signo)\n");
        f_print(fout, "int signo;\n");
	f_print(fout, "{\n");
	print_pmapunset("\t");	
	f_print(fout, "\texit(1);\n");
	f_print(fout, "}\n");
}


/*
 * This will write out the calls to set up the signal handler
 */
void
write_sig_calls()
{
	f_print(fout, "\n\t(void) signal(SIGHUP, un_register_prog);\n");
	f_print(fout, "\t(void) signal(SIGINT, un_register_prog);\n");
	f_print(fout, "\t(void) signal(SIGQUIT, un_register_prog);\n");
	f_print(fout, "\t(void) signal(SIGTERM, un_register_prog);\n");
}
