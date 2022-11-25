/* $Revision: 70.2 $ */

/*
*      rlp -- off line print
*
*/

#include "lp.h"

FILE	*cfile, *rfile;

char	OH_buf[SP_MAXHOSTNAMELEN];	/* buffer for original host  name */
char	RM_buf[SP_MAXHOSTNAMELEN];	/* buffer for remote machine name */
char	RP_buf[DESTMAX+1];	/* buffer for remote printer name */
char	arg[BUFSIZ];
char	buf[BUFSIZ];
char	bcfile[FILEMAX];	/* holds temp  control file name */
char	format = 'f';		/* format char for printing files */
char	hcfile[FILEMAX];	/* holds HP-UX control file name */
char	hdt_buf[512];		/* buffer to hold the header title */
char	host[SP_MAXHOSTNAMELEN];	/* host name */
char	opt_buf[512];		/* buffer to hold options */
char	printer_buf[DESTMAX+1];	/* printer name buffer */
char	work[FILEMAX];		/* work buffer */

char	*OH = OH_buf;		/* pointer to original host  name */
char	*RM = RM_buf;		/* pointer to remote machine name */
char	*RP = RP_buf;		/* pointer to remote printer name */
char	*classif;		/* class title on header page */
char	*filename;		/* pointer to a filename */
char	*fonts[4];		/* troff font names */
char    *jobname;		/* job name on header page */
char	*name;			/* program name */
char	*printer=printer_buf;	/* printer name */
char	*requestid;		/* hp-ux request id */
char	*pr_title = NULL;		/* pr'ing title */
char	*width = NULL;		/* width for versatec printing */

int	hdr = 1;		/* print header or not (default */
				/* is yes) */
int	iflag = 0;		/* indentation wanted */
int	indent;			/* amount to indent */
int     mail_made = 0;		/* mail sent */

int	cleanup();
int	cleanup_exit();
int	cleanup_term();

extern char **environ;	/* for cleanenv() */

int
main(argc, argv)
int argc;
char *argv[];
{
	extern	char *itoa();
	char	*arg_local;
	int	temp;
	int	err;
	int 	bsdprinter;		/* 1=BSD, 0=HPUX */
	int	safe_to_xmit = 0;	/* 1=safe to xmit through BSD system */

	cleanenv(&environ, "LANG", "LANGOPTS", "NLSPATH", 0);

	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, cleanup_exit);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, cleanup_exit);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, cleanup_exit);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, cleanup_term);

	name = argv[0];
	gethostname(host, sizeof (host));

	if(chdir(SPOOL) == -1)
		fatal("spool directory non-existent", 1);

	for (temp = 0; temp < 4; temp++){
		fonts[temp] = NULL;
	}

	while (argc > 1 && argv[1][0] == '-') {
		argc--;
		arg_local = *++argv;
		switch (arg_local[1]) {
 
		case 'I':		/* specifiy request id */
			if (arg_local[2])
				requestid = &arg_local[2];
			else if (argc > 1) {
				argc--;
				requestid = *++argv;
			}
			break;

		case 'C':		/* classification spec */
			hdr++;
			if (arg_local[2])
				classif = &arg_local[2];
			else if (argc > 1) {
				argc--;
				classif = *++argv;
			}
			break;
 
		case 'J':		/* job name */
			hdr++;
			if (arg_local[2])
				jobname = &arg_local[2];
			else if (argc > 1) {
				argc--;
				jobname = *++argv;
			}
			break;
 
		case 'T':		/* pr's title line */
			if (arg_local[2]){
				pr_title = &arg_local[2];
			}else{
				if (argc > 1) {
					argc--;
					pr_title = *++argv;
				}
			}
			break;
 
		case 'l':		/* literal output */
		case 'p':		/* print using ``pr'' */
		case 't':		/* print troff output (cat files) */
		case 'n':		/* print ditroff output */
		case 'd':		/* print tex output (dvi files) */
		case 'g':		/* print graph(1G) output */
		case 'c':		/* print cifplot output */
		case 'v':		/* print vplot output */
		case 'k':		/* Kerberized File */
		case 'o':		/* Postscript File */
		case 'z':		/* Palladium File */
			format = arg_local[1];
			break;
 
		case 'f':		/* print fortran output */
			format = 'r';
			break;
 
		case '4':		/* troff fonts */
		case '3':
		case '2':
		case '1':
			if (arg_local[2])
				fonts[arg_local[1] - '1'] = &arg_local[2];
			else if (argc > 1) {
				argc--;
				fonts[arg_local[1] - '1'] = *++argv;
			}
			break;
 
		case 'w':		/* page width */
			width = arg_local+2;
			break;
 
		case 'h':		/* do not want header page */
			hdr = 0;
			break;
 
		case 'i':		/* indent output */
			iflag++;
			indent = arg_local[2] ? atoi(&arg_local[2]) : 8;
			break;
 
		case 'S':		   /* The cl_hide routines cause */
			safe_to_xmit = 1;  /* problems with some BSD     */
			break;		   /* systems.  This option will */
					   /* be ued to allow HPUX       */
					   /* systems to transmit option */
					   /* lists and banner titles to */
					   /* remote HPUX systems via    */
					   /* "safe" BSD systems.        */
					   /* This option will only be   */
					   /* available in 8.0.          */
					   /* This is an undocumented    */
					   /* option.			 */

		} /* switch */
	} /* while */
	filename = argv[1];

	getremote_pm(RP, RM, OH_buf, printer_buf, requestid, &bsdprinter);
	open_control_files();


	if (strcmp(host,OH_buf)){	/* If this request is not on */
		jobname   = NULL;	/* the original system, */
		classif   = NULL;	/* do not allow anything to */
		pr_title  = NULL;	/* be changed.  Only the */
		format    = NULL;	/* original system is */
		width     = NULL;	/* allowed to set entries */
		fonts[0]  = NULL;
		fonts[1]  = NULL;
		fonts[2]  = NULL;
		fonts[3]  = NULL;
		mail_made = 0;
		hdr       = 1;
		iflag     = 0;
	}
	convert_send(bsdprinter, safe_to_xmit);
sendit_again:

	if ((err = sendit(bcfile)) != 0){ /* send to remote machine */
		if (err > 0){
			goto sendit_again;
		}
	}

	if (bcfile)
		unlink(bcfile);
	exit(0);
/* NOTREACHED */
}

/*
 *	Convert HP-UX format control file to BSD format control file.
 */
int
convert_send(bsdprinter, safe_to_xmit)
int bsdprinter;
int safe_to_xmit;
{
	int	temp;
	char	type;
	while (getrent(&type,arg,rfile) != EOF){
		switch (type){

		case R_JOBNAME:		/* 'J'  Job Name */
			if (hdr == 0){
				break;
			}
			if (jobname != NULL){
				strncpy(arg,jobname,MAXJCL);
				arg[100] = '\0';
			}
			putrent(type, arg, cfile);
			break;
		case R_CLASSIFICATION:	/* 'C'  Classification */
			if (hdr == 0){
				break;
			}
			if (classif != NULL){
				strncpy(arg,classif,MAXJCL);
				arg[31] = '\0';
			}
			putrent(type, arg, cfile);
			break;
		case R_LITERAL:		/* 'L'  Literal */
			if (hdr != 0){
				putrent(type, arg, cfile);
			}
			break;
		case R_TITLE:		/* 'T'  Title for pr */
			if (format != 'p'){
				break;
			}
			if (pr_title != NULL){
				strncpy(arg, pr_title, MAXPRTITLE);
				arg[79] = '\0';
				putrent(type, pr_title, cfile);
			}else{
				putrent(type, arg, cfile);
			}
			break;
		case R_HOSTNAME:	/* 'H'  Host Name */
			putrent(type, arg, cfile);
			break;
		case R_PERSON:		/* 'P'  Person */
			putrent(type, arg, cfile);
			break;
		case R_MAIL:		/* 'M'  Send mail */
			if (mail_made == 0){
				putrent(type, arg, cfile);
				mail_made = 1;
			}
			break;
		case R_PRIORITY:	/* 'A'	Priority of request */
			if (! bsdprinter)  /* only transmit to HPUX printer */
			    putrent(type, arg, cfile);
			break;
		case R_FORMATTEDFILE:	/* 'f'  Formatted File.  (BSD) */
		case R_FILETYPEl:	/* 'l'  Like ``f'' */
		case R_FILETYPEp:	/* 'p'  Name of a file to print using pr */
		case R_FILETYPEt:	/* 't'  Troff File */
		case R_FILETYPEn:	/* 'n'  Ditroff File */
		case R_FILETYPEd:	/* 'd'  DVI File */
		case R_FILETYPEg:	/* 'g'  Graph  File */
		case R_FILETYPEc:	/* 'c'  Cifplot File */
		case R_FILETYPEv:	/* 'v'  The file contains a raster image */
		case R_FILETYPEr:	/* 'r'  The file contains text data (FORTRAN CC) */
		case R_FILETYPEk:	/* 'k'  Kerberized File */
		case R_FILETYPEo:	/* 'o'  Postscript File */
		case R_FILETYPEz:	/* 'z'  Palladium File */
			if (format != NULL){
				putrent(format, arg, cfile);
			}else{
				putrent(type, arg, cfile);
			}
			break;
		case R_FONT1:		/* '1'  Troff Font R */
		case R_FONT2:		/* '2'  Troff Font I */
		case R_FONT3:		/* '3'  Troff Font B */
		case R_FONT4:		/* '4'  Troff Font S */
			putrent(type, arg, cfile);
			break;
		case R_WIDTH:		/* 'W'  Width */
			putrent(type, arg, cfile);
			break;
		case R_INDENT:		/* 'I'  Indent */
			putrent(type, arg, cfile);
			break;
		case R_UNLINKFILE:	/* 'U'  Unlink */
			putrent(type, arg, cfile);
			break;
		case R_FILENAME:	/* 'N'  File name (original) */
			putrent(type, arg, cfile);
			break;
		case R_FILE:		/* 'F'  File Name (HP-UX) */
					/* remove this entry */
			break;
		case R_OPTIONS:		/* 'O'  Options */
			strncpy(opt_buf,arg,sizeof(opt_buf));
			if (width != NULL){
				putrent('W', width, cfile);
			}
			for (temp = 0; temp < 4; temp++){
				if (fonts[temp] != NULL){
					type = temp + '1';
					putrent(type, fonts[temp], cfile);
				}
			}
			break;
		case R_COPIES:		/* 'K'  Copies */
			break;
		case R_HEADERTITLE:	/* 'B'  Header title (HP-UX) */
			strncpy(hdt_buf,arg,sizeof(hdt_buf));
			if (iflag != 0){
				putrent('I',itoa(indent), cfile);
			}
			break;
		case R_WRITE:		/* 'R'  Write */
			if (mail_made == 0){
				type = R_MAIL;	/*convt write request to mail*/
				putrent(type, arg, cfile);
				mail_made = 1;
			}
			break;
		default:
			break;
		} /* switch */
	} /* while */

	if (! bsdprinter || safe_to_xmit)	/* only transmit to HPUX   */
						/* or safe BSD systems */
	{
	    cl_hide(R_OPTIONS,opt_buf);
	    cl_hide(R_HEADERTITLE,hdt_buf);
	}

	fclose(cfile);
	fclose(rfile);
}

cleanup_exit()
{
      cleanup();
      exit(1);
}
/*
      Cleanup for SIGTERM differs from the other cleanup because
      the spool system assumes that commands executed from the
      model script will have a exit status that indicates that
      it is exiting because of a SIGTERM.  This requires that
      the SIGTERM be caught, cleanup occurs, and you must kill
      yourself with a SIGTERM to provide the correct exit status.

      If you do not, the next request in the spool queue for that
      printer will get skipped.
*/
cleanup_term()
{
      int     my_pid;

      cleanup();
      signal(SIGTERM, SIG_DFL);
      my_pid = getpid();
      kill (my_pid, SIGTERM);
      exit(0);
}
/*
 * Cleanup after interrupts and errors.
 */
int
cleanup()
{

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);

	if (bcfile)
		unlink(bcfile);

	sendit_status("%s: ready and waiting", printer);

	return(0);
}
/*
 * itoa - integer to string conversion
 */
char *
itoa(i)
	int i;
{
	static char b[10] = "########";
	char *p;

	p = &b[8];
	do
		*p-- = i%10 + '0';
	while (i /= 10);
	return(++p);
}

int
/*VARARGS1*/
log(msg, a1, a2, a3)
char *msg;
{
	short console = isatty(fileno(stderr));

	fprintf(stderr, console ? "\r\n%s: " : "%s: ", name);
	if (printer)
		fprintf(stderr, "%s: ", printer);
	fprintf(stderr, msg, a1, a2, a3);
	if (console)
		putc('\r', stderr);
	putc('\n', stderr);
	fflush(stderr);
}

int
open_control_files()
{
	int	index;
	char	*line;
	char	origin_host[SP_MAXHOSTNAMELEN];
	extern	char	*strrchr();

	index = '/';
	strncpy (hcfile,filename,FILEMAX);

	if ((line = strrchr(hcfile,index)) == NULL){
		line = hcfile;
	}else{
		line++;
	}
	*line = 'c';

	if (*++line == 'f'){
		*++line = 'A';
		strncpy( origin_host, ++line + 3 , SP_MAXHOSTNAMELEN );
		sprintf(line, "%03d%s", atoi( strchr(requestid, '-') + 1 )
			, origin_host);
	}else{
		*line = 'A';
		strncpy( origin_host, ++line + 4 , SP_MAXHOSTNAMELEN );
		sprintf(line, "%04d%s", atoi( strchr(requestid, '-') + 1 )
			, origin_host);
	}

	strcpy(bcfile,hcfile);
	if ((line = strrchr(bcfile,index)) == NULL){
		line = bcfile;
	}else{
		line++;
	}
	*line = 't';
	if ((rfile = fopen(hcfile,"r")) == NULL){
		sprintf(work,"Unable to find the control file %s",hcfile);
		fatal(work,1);
	}

	if ((cfile = fopen(bcfile,"w")) == NULL){
		sprintf(work,"Unable to creat the control file %s",bcfile);
		fatal(work,1);
	}
}

int
cl_hide(subtype,cl_ptr)
char	subtype;
char	*cl_ptr;
{
	int	arglen;
	int	length;
	int	xx;

	char	*loc_ptr;

	char	arg_buf[70];

	loc_ptr	= cl_ptr;

	xx	= 60;
		length = strlen(loc_ptr);
	do{
		arglen = xx;
		if (length < arglen)
			arglen = length;
		arg_buf[0] = ' ';
		arg_buf[1] = subtype;
		arg_buf[2] = '\0';
		if (length != 0){
			strncat(arg_buf,loc_ptr,arglen);
			arg_buf[(arglen + 2)] = '\0';
		}
		putrent('N',arg_buf,cfile);
		loc_ptr	= loc_ptr + arglen;
	}while	((length = strlen(loc_ptr)) > 0);
}
/*
 *	Get the remote printer name and the remote machine name.
 */
int
getremote_pm(remoteprinter,remotemachine,host_ptr,localprinter,request_id,
		bsdprinter)
char	*remoteprinter;
char	*remotemachine;
char	*host_ptr;
char	*localprinter;
char	*request_id;
int	*bsdprinter;
{
	int	seqno;
	struct	pstat	p;
	char	dest[DESTMAX];

	if (! (isrequest(request_id, dest, &seqno))){
		sprintf(work,"invalid request id (%s)",request_id);
		fatal(work,1);
	}

	if (getpdest(&p,dest) == EOF){
		sprintf(work,"no such printer exists");
		fatal(work,1);
	}

	if (p.p_remotedest == NULL){
		sprintf(work,"no remote machine specified");
		fatal(work,1);
	}

	if (p.p_remoteprinter == NULL){
		sprintf(work,"no remote printer specified");
		fatal(work,1);
	}

	endpent();

	if (p.p_rflags & P_OB3)
	    *bsdprinter = 1;
	else 
	    *bsdprinter = 0;

	strncpy (remotemachine, p.p_remotedest, SP_MAXHOSTNAMELEN);
	strncpy (remoteprinter, p.p_remoteprinter, DESTMAX+1);
	strncpy (host_ptr,      p.p_host, SP_MAXHOSTNAMELEN);
	strncpy (localprinter,  dest, DESTMAX+1);
}
