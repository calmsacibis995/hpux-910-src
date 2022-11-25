#ifndef lint
#ifdef PATCH_STRING
static  char rcsid[] = "@(#) PATCH_9.X rpc_main.c: $Revision: 1.12.109.4 $	$Date: 94/11/22 09:59:24 $ PHNE_4902";
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
 * rpc_main.c, Top level of the RPC protocol compiler. 
 * Copyright (C) 1987, Sun Microsystems, Inc. 
 */

#include <stdio.h>
#include <sys/file.h>
#include "rpc_util.h"
#include "rpc_parse.h"
#include "rpc_scan.h"

/* NOTE: rpc_main.c, rpc_parse.c, rpc_scan.c and rpc_util.c share a 	*/
/* single message catalog (rpgen.cat).  For that reason we have 	*/
/* allocated messages 1 through 20 for rpc_main.c, 21 through 40 for  	*/
/* rpc_parse.c, 41 through 60 to rpc_scan.c and from 61 on for		*/
/* rpc_util.c.  If we need more than 20 messages in this file we will 	*/
/* need to take into account the message numbers that are already 	*/
/* used by the other files.						*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#define EXTEND	1		/* alias for TRUE */

char *strrchr();

struct commandline {
	int cflag;
	int hflag;
	int lflag;
	int sflag;
	int mflag;
	int uflag;
	char *infile;
	char *outfile;
};

static char *cmdname;
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
static char *svcclosetime = "120";

static char CPP[] = "/lib/cpp";

#ifdef NLS			   /* HPNFS */
static char CPPFLAGS[] = "-C -n";  /* cpp for HP-UX needs the -n to turn on */
				   /* NLS processing.                       */
nl_catd nlmsg_fd;
#else  NLS
static char CPPFLAGS[] = "-C";
#endif NLS

static char *allv[] = {
	"rpcgen", "-s", "udp", "-s", "tcp",
};
static int allc = sizeof(allv)/sizeof(allv[0]);
char tempbuf[100];

/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
int inetdflag;          /* Support for inetd */
int logflag;            /* Use syslog instead of fprintf for errors */
int indefinitewait;     /* If started by port monitors, hang till it wants */
int exitnow;            /* If started by port monitors, exit after the call */
int timerflag;          /* TRUE if !indefinite && !exitnow */

main(argc, argv)
	int argc;
	char *argv[];

{
	struct commandline cmd;

#ifdef NLS
        nl_init(getenv("LANG"));
        nlmsg_fd = catopen("rpcgen",0);
#endif NLS

	if (!parseargs(argc, argv, &cmd)) {
		f_print(stderr,
			(catgets(nlmsg_fd,NL_SETN,1, "usage: %s [-u] [-I [-K seconds]] [-L] infile\n")), cmdname);
		f_print(stderr,
			(catgets(nlmsg_fd,NL_SETN,2, "       %s [-c | -h | -l | -m | -u] [-o outfile] [infile]\n")),
			cmdname);
		f_print(stderr,
			(catgets(nlmsg_fd,NL_SETN,3, "       %s [-s udp|tcp]* [-o outfile] [-u] [infile]\n")),
			cmdname);
		exit(1);
	}
	if (cmd.cflag) {
		c_output(cmd.infile, "-DRPC_XDR", !EXTEND, cmd.outfile);
	} else if (cmd.hflag) {
		h_output(cmd.infile, "-DRPC_HDR", !EXTEND, cmd.outfile);
	} else if (cmd.lflag) {
		l_output(cmd.infile, "-DRPC_CLNT", !EXTEND, cmd.outfile);
	} else if (cmd.sflag || cmd.mflag) {
		s_output(argc, argv, cmd.infile, "-DRPC_SVC", !EXTEND,
			 cmd.outfile, cmd.mflag, cmd.uflag);
	} else {
		c_output(cmd.infile, "-DRPC_XDR", EXTEND, "_xdr.c");
		reinitialize();
		h_output(cmd.infile, "-DRPC_HDR", EXTEND, ".h");
		reinitialize();
		l_output(cmd.infile, "-DRPC_CLNT", EXTEND, "_clnt.c");
		reinitialize();
		s_output(allc, allv, cmd.infile, "-DRPC_SVC", EXTEND,
			 "_svc.c", cmd.mflag, cmd.uflag);
	}
	exit(0);
}

/*
 * add extension to filename 
 */
static char *
extendfile(file, ext)
	char *file;
	char *ext;
{
	char *res;
	char *p;

	res = alloc(strlen(file) + strlen(ext) + 1);
	if (res == NULL) {
		f_print(stderr, "Unable to extend the filename %s\n", file);
		exit(1);
	}
#ifdef hpux
	p = strrchr(file, '.');
#else
	p = rindex(file, '.');  /* Sun likes to use rindex instead of strrchr */
#endif hpux
	if (p == NULL) {
		p = file + strlen(file);
	}
	(void) strcpy(res, file);
	(void) strcpy(res + (p - file), ext);
	return (res);
}

/*
 * Open output file with given extension 
 */
static
open_output(infile, outfile)
	char *infile;
	char *outfile;
{
	if (outfile == NULL) {
		fout = stdout;
		return;
	}
	if (infile != NULL && streq(outfile, infile)) {
		nl_fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "%1$s: output would overwrite %2$s\n")), cmdname,
			infile);
		crash();
	}
	fout = fopen(outfile, "w");
	if (fout == NULL) {
		f_print(stderr, (catgets(nlmsg_fd,NL_SETN,5, "%s: unable to open ")), cmdname);
		perror(outfile);
		crash();
	}
	record_open(outfile);
}

/*
 * Open input file with given define for C-preprocessor 
 */
static
open_input(infile, define)
	char *infile;
	char *define;
{
	int pd[2];

	infilename = (infile == NULL) ? "<stdin>" : infile;
	(void) pipe(pd);
	switch (fork()) {
	case 0:
		(void) close(1);
		(void) dup2(pd[1], 1);
		(void) close(pd[0]);
		execl(CPP, CPP, CPPFLAGS, define, infile, NULL);
		perror((catgets(nlmsg_fd,NL_SETN,6, "execl")));
		exit(1);
	case -1:
		perror((catgets(nlmsg_fd,NL_SETN,7, "fork")));
		exit(1);
	}
	(void) close(pd[1]);
	fin = fdopen(pd[0], "r");
	if (fin == NULL) {
		f_print(stderr, (catgets(nlmsg_fd,NL_SETN,8, "%s: ")), cmdname);
		perror(infilename);
		crash();
	}
}

/*
 * Compile into an XDR routine output file
 */
static
c_output(infile, define, extend, outfile)
	char *infile;
	char *define;
	int extend;
	char *outfile;
{
	definition *def;
	char *include;
	char *outfilename;
	long tell;

	open_input(infile, define);	
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	f_print(fout, "#include <rpc/rpc.h>\n");
	if (infile && (include = extendfile(infile, ".h"))) {
		f_print(fout, "#include \"%s\"\n", include);
		free(include);
	}
	tell = ftell(fout);
	while (def = get_definition()) {
		emit(def);
	}
	if (extend && tell == ftell(fout)) {
		(void) unlink(outfilename);
	}
}

/*
 * Compile into an XDR header file
 */
static
h_output(infile, define, extend, outfile)
	char *infile;
	char *define;
	int extend;
	char *outfile;
{
	definition *def;
	char *outfilename;
	long tell;

	open_input(infile, define);
	outfilename =  extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	tell = ftell(fout);
	while (def = get_definition()) {
		print_datadef(def);
	}
	if (extend && tell == ftell(fout)) {
		(void) unlink(outfilename);
	}
}

/*
 * Compile into an RPC service
 * Write out the svc.c file
 */
static
s_output(argc, argv, infile, define, extend, outfile, nomain, unmap_it)
	int argc;
	char *argv[];
	char *infile;
	char *define;
	int extend;
	char *outfile;
	int nomain;
	int unmap_it;
{
	char *include;
	definition *def;
	int foundprogram;
	char *outfilename;

	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	f_print(fout, "#include <stdio.h>\n");
	f_print(fout, "#include <rpc/rpc.h>\n");
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
        if (strcmp(svcclosetime, "-1") == 0)
                indefinitewait = 1;
        else if (strcmp(svcclosetime, "0") == 0)
                exitnow = 1;
        else if (inetdflag) {
                f_print(fout, "#include <signal.h>\n");
                timerflag = 1;
        }
        if (inetdflag) {
                f_print(fout, "#include <sys/socket.h>\n");
                if (!nomain) {
/*
 * HPNFS
 * These are header file required by some
 * HP fixes to Sun's code.
 * 06/10/1993 Wusheng Chen IND
 */
                        f_print(fout, "#include <fcntl.h>\n");
                        f_print(fout, "#include <sys/resource.h>\n");
                }
        }
        if (logflag || inetdflag)
                f_print(fout, "#include <syslog.h>\n");

	if (infile && (include = extendfile(infile, ".h"))) {
		f_print(fout, "#include \"%s\"\n", include);
		free(include);
	}
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
       if (inetdflag) {
                f_print(fout, "\n#ifdef DEBUG\n#define RPC_SVC_FG\n#endif\n");
                if (timerflag)
                        f_print(fout, "#define _RPCSVC_CLOSEDOWN %s\n",
                                svcclosetime);
        }

	foundprogram = 0;
	while (def = get_definition()) {
		foundprogram |= (def->def_kind == DEF_PROGRAM);
	}
	if (extend && !foundprogram) {
		/*
		 * Since there was no program definition in the .x file
		 * just blow away the program file as none is needed.
		 */
		(void) unlink(outfilename);
		return;
	}
	if (nomain) {
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
                if (inetdflag) {
                        f_print(fout, "\nextern int _rpcpmstart;");
                        f_print(fout, "\t\t/* Started by a port monitor ? */\n");
                        f_print(fout, "extern int _rpcfdtype;");
                        f_print(fout, "\t\t/* Whether Stream or Datagram ? */\n");
                        if (timerflag) {
                                f_print(fout, "static int _rpcsvcdirty;");
                                f_print(fout, "\t/* Still serving ? */\n");
                        }
                }

		/*
		 * write out the server-side stubs, but don't write
		 * out a  main()
		 */
		write_programs((char *)NULL);
	} else {
		/* 
		 * OK, write out the whole server side program with  main()
		 */
		if (unmap_it) {
			/*
			 * HPNFS
			 * Write out the signal handler function that will
			 * catch signals and will unregister the program
			 * from the port mapper
			 */
			write_sig_hand();
		}
		write_most(infile);
		if (unmap_it) {
			write_sig_calls();
		}
		do_registers(argc, argv);
		write_rest();
		write_programs("static");
	}
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
        if (inetdflag)
                write_svc_aux();
}

static
l_output(infile, define, extend, outfile)
	char *infile;
	char *define;
	int extend;
	char *outfile;
{
	char *include;
	definition *def;
	int foundprogram;
	char *outfilename;

	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	f_print(fout, "#include <rpc/rpc.h>\n");
	f_print(fout, "#include <sys/time.h>\n");
	if (infile && (include = extendfile(infile, ".h"))) {
		f_print(fout, "#include \"%s\"\n", include);
		free(include);
	}
#ifdef hpux
	/*
	 * HPNFS
	 * Since HP-UX does not normally have NULL defined, and
	 * the client programs use NULL, we will make it defined
	 * if it is not already.   MDS   05/13/88
	 */
	f_print(fout, "\n");
	f_print(fout, "#ifdef hpux\n");
	f_print(fout, "\n");
	f_print(fout, "#ifndef NULL\n");
	f_print(fout, "#define NULL  0\n");
	f_print(fout, "#endif /* NULL */\n");
	f_print(fout, "\n");
	f_print(fout, "#endif /* hpux */\n");
#endif hpux
	foundprogram = 0;
	while (def = get_definition()) {
		foundprogram |= (def->def_kind == DEF_PROGRAM);
	}
	if (extend && !foundprogram) {
		(void) unlink(outfilename);
		return;
	}
	write_stubs();
}

/*
 * Perform registrations for service output 
 */
static
do_registers(argc, argv)
	int argc;
	char *argv[];

{
	int i;

	for (i = 1; i < argc; i++) {
		if (streq(argv[i], "-s")) {
			write_register(argv[i + 1]);
			i++;
		}
	}
}

/*
 * Parse command line arguments 
 */
static
parseargs(argc, argv, cmd)
	int argc;
	char *argv[];
	struct commandline *cmd;

{
	int i;
	int j;
	char c;
	char flag[(1 << 8 * sizeof(char))];
	int nflags;

	cmdname = argv[0];
	cmd->infile = cmd->outfile = NULL;
	if (argc < 2) {
		return (0);
	}
	flag['c'] = 0;
	flag['h'] = 0;
	flag['s'] = 0;
	flag['o'] = 0;
	flag['l'] = 0;
	flag['m'] = 0;
	flag['u'] = 0;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			if (cmd->infile) {
				return (0);
			}
			cmd->infile = argv[i];
		} else {
			for (j = 1; argv[i][j] != 0; j++) {
				c = argv[i][j];
				switch (c) {
				case 'c':
				case 'h':
				case 'l':
				case 'm':
				case 'u':
					if (flag[c]) {
						return (0);
					}
					flag[c] = 1;
					break;
/*
 * Added for inetd (-I) support.
 * (Added by Bob Kelley - IND Online - 24 Jan 94 to support inetd -I)
 */
                                case 'I':
                                        inetdflag = 1;
                                        break;
                                case 'L':
                                        logflag = 1;
                                        break;
                                case 'K':
                                        if (!inetdflag ||  (++i == argc)) {
                                                return (0);
                                        }
                                        svcclosetime = argv[i];
                                        goto nextarg;


				case 'o':
				case 's':
					if (argv[i][j - 1] != '-' || 
					    argv[i][j + 1] != 0) {
						return (0);
					}
					flag[c] = 1;
					if (++i == argc) {
						return (0);
					}
					if (c == 's') {
						if (!streq(argv[i], "udp") &&
						    !streq(argv[i], "tcp")) {
							return (0);
						}
					} else if (c == 'o') {
						if (cmd->outfile) {
							return (0);
						}
						cmd->outfile = argv[i];
					}
					goto nextarg;

				default:
					return (0);
				}
			}
	nextarg:
			;
		}
	}
	cmd->cflag = flag['c'];
	cmd->hflag = flag['h'];
	cmd->sflag = flag['s'];
	cmd->lflag = flag['l'];
	cmd->mflag = flag['m'];
	cmd->uflag = flag['u'];
	nflags = cmd->cflag + cmd->hflag +  cmd->lflag + cmd->mflag;
	if (nflags && cmd->uflag) {
		/* Don't let -u exist with -c, -h, -l or -m */
		return (0);
	} else {
		/* But it can exist with -s */
		nflags += cmd->sflag;
	}
	if (nflags == 0) {
		if (cmd->outfile != NULL || cmd->infile == NULL) {
			return (0);
		}
	} else if (nflags > 1) {
		return (0);
	}
	return (1);
}
