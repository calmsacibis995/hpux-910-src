/* $Revision: 72.2 $ */

/*
 * Receive printer jobs from the network, queue them and
 * start the printer daemon.
 */

#include "lp.h"
#include <string.h>

#define	FILMOD	0664

static	int	count;			/* holds the number of copies */

static	char    cfname[SP_MAXHOSTNAMELEN + 7];  /* tmp copy of cf before linking */
static	char    tfname[SP_MAXHOSTNAMELEN + 7];  /* tmp copy of cf before linking */
static	char	bcfile[FILEMAX];
static	char	hcfile[FILEMAX];
static	char    *dfname;		/* data files */

extern	char	*printer;	/* name of the printer to queue */
				/* the request on. */
extern	char	*from;		/* name of the printer to queue */
extern	char	work[BUFSIZ];
static	char	bsd_opt_buf[OPTMAX];
static	char	hpux_opt_buf[OPTMAX];
static	char	hpux_title_buf[TITLEMAX];

/*
	The structure outq is to be aligned for sharing on both
	the 300 and 800.  Before you make any changes, make sure
	the alignment and size of the structure match on both
	machines.  This is to support DUX on teh 300 and 800.

	The size of this structure must be multiple of 4 bytes.
*/
static struct outq o = {	/* output request to be appended to output queue */
	0L,	/* size of request */
	0L,	/* date of request */
	0L,	/* date of start printing */
	0,	/* sequence # */
	0,	/* not printing, not deleted */
	-1,	/* priority of this request */
	0,	/* Use three digit sequence numbers */
	NULL,	/* host name */
	NULL,	/* logname */
	NULL,	/* destination */
	NULL,	/* device where printing */
};

static	FILE	*cfile;			/* hp-ux control file */
static	FILE	*rfile;			/* received control file */

char	*find_dev();


recvjob()
{
	struct	qstat q;	/* acceptance status */

	if(getqdest(&q, printer) == EOF){	/* get acceptance status */
		endqent();
		putchar('\1');	/* return error code */
		sprintf(work, "acceptance status of destination \"%s\" unknown", printer);
		fatal(work, 1);
	}
	endqent();
	if(! q.q_accept) {	/* accepting requests ? */
		putchar('\1');	/* return error code */
		sprintf(work, "can't accept requests for destination \"%s\" -\n\t%s", printer , q.q_reason);
		fatal(work, 1);
	}

/* *********************************************************

	Do we need code to indicate that the queue is disabled?

			putchar('\1');		

****************************************************** */

	readjob();
}

char	*spsp = "";
#define ack()	(void) write(1, spsp, 1);

/*
 * Read printer jobs sent by rlp and lpd, copy them to the
 * spooling directory.
 * Return the number of jobs successfully transfered.
 */
int
readjob()
{
	int	size;
	char	*cp;
	char	line[BUFSIZ];

	ack();
	o.o_size = 0;
	for (;;) {
		/*
		 * Read a command to tell us what to do
		 */
		cp = line;
		do {
			if ((size = read(0, cp, 1)) != 1) {
				if (size < 0)
					fatal("Lost connection");
				return(0);
			}
		} while (*cp++ != '\n');
		*--cp = '\0';
		cp = line;
		switch (*cp++) {
		case '\1':	/* cleanup because data sent was bad */
			clean_up_files();
			continue;

		case '\2':	/* read cf file */
			size = 0;
			while (*cp >= '0' && *cp <= '9')
				size = size * 10 + (*cp++ - '0');
			if (*cp++ != ' ')
				break;
			strcpy(cfname, cp);
			strcpy(tfname, cp);
			tfname[0] = 't';
			if (!chksize(size)){
				(void) write(1, "\2", 1);
				continue;
			}
			if (!readfile(tfname, size)) {
				clean_up_files();
				continue;
			}
			convert_recv(tfname,cp);
			(void) unlink(bcfile);
			tfname[0] = '\0';
			o.o_size -= size;
			o.o_size *= count;
			queuejob();
			o.o_size = 0;
			continue;

		case '\3':	/* read df file */
			size = 0;
			while (*cp >= '0' && *cp <= '9')
				size = size * 10 + (*cp++ - '0');
			if (*cp++ != ' ')
				break;
			if (!chksize(size)){
				(void) write(1, "\2", 1);
				continue;
			}
			(void) readfile(dfname = cp, size);
			continue;
		}
		fatal("Unacceptable protocol",1);
	}
}

/*
 * Read files send by rlp and lpd and copy them to the
 * spooling directory.
 */
int
readfile(file, size)
char	*file;
int	size;
{
	char *cp;
	char buf[BUFSIZ];
	char filebuf[FILEMAX];
	int i, j, amt;
	int fd, err;

	sprintf(filebuf, "%s/%s/%s", REQUEST,printer,file);
	set_user_id(1);	/* set euid to LP admin */

	fd = open(filebuf, O_WRONLY|O_CREAT, FILMOD);
	if (fd < 0) {
		sprintf(work, "cannot create %s", file);
		fatal(work);
		return(0);
	}
	chmod(filebuf, 0440);

	set_user_id(0); /* set euid to root */
	ack();
	err = 0;
	for (i = 0; i < size; i += BUFSIZ) {
		amt = BUFSIZ;
		cp = buf;
		if (i + amt > size)
			amt = size - i;
		do {
			j = read(1, cp, amt);
			if (j <= 0) {
				(void) unlink(filebuf);
				fatal("Lost connection");
				return(0);
			}
			amt -= j;
			cp += j;
		} while (amt > 0);
		amt = BUFSIZ;
		if (i + amt > size)
			amt = size - i;
		if (write(fd, buf, amt) != amt) {
			err++;
			break;
		}
	}
	(void) close(fd);
	if (err) {
		sprintf(work, "%s: write error", file);
		fatal(work);
	}
	if (noresponse()) {		/* file sent had bad data in it */
		(void) unlink(filebuf);
		return(0);
	}
	ack();
	o.o_size += size;
	return(1);
}

int
noresponse()
{
	char resp;

	if (read(1, &resp, 1) != 1)
		fatal("Lost connection");
	if (resp == '\0')
		return(0);
	return(1);
}

/*
 * Remove all the files associated with the current job being transfered.
 */
int
clean_up_files()
{
	int	seqno;
	int	ob3;

	if (tfname[0])
		(void) unlink(tfname);
	if (dfname){
		if (isdigit(*(dfname + 2))){
			ob3 = 0;
			sscanf(dfname,"%2c%4d%s",work,&seqno,from);
		}else{
			ob3 = O_OB3;
			sscanf(dfname,"%3c%3d%s",work,&seqno,from);
		}
		rmreq(printer, seqno, from, ob3);
	}
}

/*
 *	Convert a received file to a HP-UX format control file.
 */
int
convert_recv(source,destination)
char	*source;
char	*destination;
{
	extern	char	*itoa();
	char	type;
	char	otype;
	char	*cp2;

	char	arg[BUFSIZ];
	char	oldarg[BUFSIZ];
	int	first_format	= -1;
	int	bsd_format	= -1;
	int	hpux_opt_found = FALSE;
	int	first_seq = 0;

	count	= -1;

	bsd_opt_buf[0]		= R_OPTIONS;
	bsd_opt_buf[1]		= '\0';
	hpux_opt_buf[0]		= R_OPTIONS;
	hpux_opt_buf[1]		= '\0';
	hpux_title_buf[0]	= R_HEADERTITLE;
	hpux_title_buf[1]	= '\0';

	sprintf(bcfile,"%s/%s/%s",REQUEST,printer,source);
	sprintf(hcfile,"%s/%s/%s",REQUEST,printer,destination);

	set_user_id(1); /* set euid to LP admin */

	if ((rfile = fopen(bcfile,"r")) == NULL){
		sprintf(work,"Unable to find the control file %s",bcfile);
		fatal(work,1);
	}

	if ((cfile = fopen(hcfile,"w")) == NULL){
		sprintf(work,"Unable to creat the control file %s",hcfile);
		fatal(work,1);
	}

	set_user_id(0); /* set euid to root */

	while (getrent(&type,arg,rfile) != EOF){
		switch (type){

		case R_JOBNAME:		/* 'J'  Job Name */
			add_bsd_opt(type, arg);
			break;
		case R_CLASSIFICATION:	/* 'C'  Classification */
			add_bsd_opt(type, arg);
			break;
		case R_LITERAL:		/* 'L'  Literal */
			break;
		case R_TITLE:		/* 'T'  Title for pr */
			break;
		case R_HOSTNAME:	/* 'H'  Host Name */
/*			if (cp2=strchr(arg,'.'))  removed  
			    *cp2='\0';           bug fix do to problems it created */
			strncpy(o.o_host,arg, sizeof(o.o_host)); /* Name of the system */
						/* where the request */
						/* originated */
			break;
		case R_PERSON:		/* 'P'  Person */
			strncpy(o.o_logname,arg,LOGMAX+1);
			break;
		case R_MAIL:		/* 'M'  Send mail */
		case R_WRITE:		/* 'R'	Write */
			break;
		case R_PRIORITY:	/* 'a'	Priority */
			o.o_priority = (short)atoi(arg);
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
                case R_FILETYPEk:       /* 'k'  Kerberized File */
                case R_FILETYPEo:       /* 'o'  Postscript File */
                case R_FILETYPEz:       /* 'z'  Palladium File */
			if (bsd_format == -1){
				if (type != 'f'){
					if (type == 'r'){
						type = 'f';
					}
					add_bsd_opt(type, arg);
					bsd_format = type;
				}
			}
			if (count == -1){
				count = 1;
				otype = type;
				strcpy(oldarg, arg);
				while (getrent(&type,arg,rfile) != EOF){
					if (otype == type){
						if (!strcmp(oldarg,arg)){
							count++;
						}else{
							break;
						}
					}else{
						break;
					}
				}
			}
			break;
		case R_FONT1:		/* '1'  Troff Font R */
		case R_FONT2:		/* '2'  Troff Font I */
		case R_FONT3:		/* '3'  Troff Font B */
		case R_FONT4:		/* '4'  Troff Font S */
			add_bsd_opt(type, arg);
			break;
		case R_WIDTH:		/* 'W'  Width */
			add_bsd_opt('w', arg);
			break;
		case R_INDENT:		/* 'I'  Indent */
			add_bsd_opt('i', arg);
			break;
		case R_FILENAME:	/* 'N'  File name (original) */
			if (arg[0] == ' ' &&
				((arg[1] == R_OPTIONS) || /* 'O' */
				 (arg[1] == R_HEADERTITLE))){ /* 'B' */
				add_hpux_opt(arg[1],&arg[2]);
				hpux_opt_found = TRUE;
			}
			break;
		default:
			break;
		}
	}
	fseek(rfile,0L,0);	/* rewind the file */
	while (getrent(&type,arg,rfile) != EOF){
		switch (type){

		case R_JOBNAME:		/* 'J'  Job Name */
		case R_CLASSIFICATION:	/* 'C'  Classification */
		case R_LITERAL:		/* 'L'  Literal */
		case R_HOSTNAME:	/* 'H'  Host Name */
		case R_PERSON:		/* 'P'  Person */
		case R_MAIL:		/* 'M'  Send mail */
		case R_WRITE:		/* 'R'	Write */
		case R_PRIORITY:	/* 'a' Priority */
			putrent(type, arg, cfile);
			break;
		case R_TITLE:		/* 'T'  Title for pr */
			if (first_format == -1){
				first_format = 1;
				putrent(hpux_title_buf[0], &hpux_title_buf[1],cfile);
				putrent(R_COPIES, itoa(count), cfile);
				if (hpux_opt_found == TRUE){
					putrent(hpux_opt_buf[0], &hpux_opt_buf[1],cfile);
				}else{
					putrent(bsd_opt_buf[0],&bsd_opt_buf[1],cfile);
				}
			}
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
		case R_FILETYPEv:	/* 'v'  File contains a raster image */
		case R_FILETYPEr:	/* 'r'  File contains text data (FORTRAN CC) */
                case R_FILETYPEk:       /* 'k'  Kerberized File */
                case R_FILETYPEo:       /* 'o'  Postscript File */
                case R_FILETYPEz:       /* 'z'  Palladium File */
			if (first_format == -1){
				first_format = 1;
				putrent(hpux_title_buf[0], &hpux_title_buf[1],cfile);
				putrent(R_COPIES, itoa(count), cfile);
				if (hpux_opt_found == TRUE){
					putrent(hpux_opt_buf[0], &hpux_opt_buf[1],cfile);
				}else{
					putrent(bsd_opt_buf[0],&bsd_opt_buf[1],cfile);
				}
			}
			if (first_seq == 0){
				putrent(R_FILE, arg, cfile); /* 'F'  File Name (HP-UX) */
				first_seq = 1;
			}
			putrent(type, arg, cfile);
			break;
		case R_FONT1:		/* '1'  Troff Font R */
		case R_FONT2:		/* '2'  Troff Font I */
		case R_FONT3:		/* '3'  Troff Font B */
		case R_FONT4:		/* '4'  Troff Font S */
		case R_WIDTH:		/* 'W'  Width */
		case R_INDENT:		/* 'I'  Indent */
			putrent(type, arg, cfile);
			break;
		case R_UNLINKFILE:	/* 'U'  Unlink */
			first_seq = 0;
			putrent(type, arg, cfile);
			break;
		case R_FILENAME:	/* 'N'  File name (original) */
			if (!(arg[0] == ' ' &&
				((arg[1] == R_OPTIONS) || /* 'O' */
				 (arg[1] == R_HEADERTITLE)))){ /* 'B' */
				putrent(type, arg, cfile);
			}
			break;
		default:
			break;
		}
	}
	fclose(cfile);
	fclose(rfile);
	chmod(hcfile, 0440);
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

add_bsd_opt(type, arg)
char	type;
char	*arg;
{
	char	opt_head[sizeof " -oBSDxx"];
	sprintf(opt_head," -oBSD%c",type);
	if ((BUFSIZ - strlen(bsd_opt_buf) -
	     strlen(opt_head) - strlen(arg)) > 1){
		strcat(bsd_opt_buf,opt_head);
		strcat(bsd_opt_buf,arg);
	}else{
		log("too many options");
	}
}
add_hpux_opt(type, arg)
char	type;
char	*arg;
{
	if (type == R_OPTIONS){
		strcat(hpux_opt_buf,arg);
	}else{
		strcat(hpux_title_buf,arg);
	}
}
queuejob()
{
	int	seqno;
	struct	pstat	p;
	char	realname[SP_MAXHOSTNAMELEN + 7];
	char	rnfile[FILEMAX];

	if (isdigit(tfname[2])){
		o.o_rflags = O_REM;	/* Do not use three digit */
					/* sequence numbers */
		sscanf(cfname,"%2c%4d",work,&seqno);
	}else{
		o.o_rflags = (O_OB3|O_REM);/* Use three digit sequence */
					/* numbers */
		sscanf(cfname,"%3c%3d",work,&seqno);
	}

	strncpy(o.o_dest,printer, sizeof(o.o_dest)); /* output */
					/* destination (class */
					/* or member) */
/*	o.o_logname */			/* logname of requester */
					/* obtained in convert_recv */
	o.o_seqno = seqno;		/* sequence number of request */
/*	o.o_size */			/* size of request -- # of */
					/* bytes of data. */
					/* Calculated in readfile */
	strcpy(o.o_dev,"-");		/* if printing, the name of */
					/* the printer. */
					/* if not printing, "-".  */
	time(&o.o_date);		/* date of entry into */
					/* output queue */
	o.o_flags = 0;			/* flag values */
/*	o.o_rflags */			/* rflag values obtained */
					/* above */
/*	o.o_host */			/* Name of the system where */
					/* the request originated */
					/* obtained in convert_recv */

        setpriority();

	/*
	The following fixes a defect resulting from file names generated
	from hostnames that include the domain information.  It
	reconciles the name in the outputq file with the one passed from
	the remote machine.  It's needed for the lpr software (FTP v2.0)
	on Domes-dos.
	From Julian Perry, with minor changes by Rob Robason.
	*/
	if (o.o_rflags & O_OB3) {
	    sprintf(realname, "cfA%03d%s", o.o_seqno, o.o_host);
	} else {
	    sprintf(realname, "cA%04d%s", o.o_seqno, o.o_host);
	}
	if (strcmp(cfname, realname)) {
	    sprintf(rnfile, "%s/%s/%s", REQUEST, o.o_dest, realname);
	    if (rename(hcfile, rnfile) == -1) {
		log("ERROR: attempt to rename request file name from ",hcfile);
		log("ERROR: attempt to rename request file name to ",rnfile);
	    } else {
		(void) strcpy(hcfile, rnfile);
		log("request file name changed to ",hcfile);
	    }
	}

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	addoent(&o);		/* enter request in output queue */
	endoent();

	sprintf(work, "%s %d %s %s %d", o.o_dest, o.o_seqno, o.o_logname, o.o_host, (o.o_rflags & O_OB3));


	/* enqueue can return 0,-1, or -2. We are only interested in -1 */
	/* since that indicates a fifo write error. We still print the  */
	/* request id message for a -2 return, since the most likely    */
	/* cause of that problem is that the scheduler is not running.  */

	if(enqueue(F_REQUEST, work) == -1){  /* inform scheduler of request */
		log("unable to queue request ",work);
	}
}

/*
 * Check to see if there is enough space on the disk for size bytes.
 * 1 == OK, 0 == Not OK.
 */
/* remove the define and include when the include files have the
   NFS ifdef removed */
/*
include <sys/types.h>
*/
#include <sys/vfs.h>

chksize(size)
int	size;
{

	long	spacefree;
	long	size_blocks;

	struct	statfs	buf;

	sprintf(work, "%s/%s", REQUEST, printer);
	statfs(work,&buf);

	size_blocks = (size + (buf.f_bsize -1)) / buf.f_bsize;

	spacefree = buf.f_bavail;
			
	if (size_blocks > spacefree)
		return(0);
	return(1);
}

/* setpriority -- set the value of priority in the outputq file */

setpriority()
{
        struct pstat p;
        short   class_default();

        if (o.o_priority < MINPRI || o.o_priority > MAXPRI){
                if (o.o_priority != -1){
                        fatal("bad priority", 1);
                }else{
                        if(getpdest(&p, o.o_dest) == EOF){
                                if(isclass(o.o_dest)){
                                        p.p_default = class_default(o.o_dest);
                                }else{
                                        p.p_default = 0;
                                }
                        }

                /* verify that printer pstat structure is not corrupt */

                        if (p.p_default < MINPRI || p.p_default > MAXPRI){
                            p.p_default = 0;
                        }

                        o.o_priority = p.p_default;
                        endpent();
                }
        }
}

/* class_default - get default priority when class destination was specified */

short
class_default(dest)
char    *dest;
{
        char    *c, printer[DESTMAX+1];
        char    class[sizeof(CLASS) + DESTMAX + 1];
        FILE    *fp;
        struct  pstat p;
        short   default_max = 0;

        sprintf(class, "%s/%s", CLASS, dest);
        if((fp = fopen(class, "r")) == NULL) {
                fatal("bad priority", 1);
        }else{
                while(fgets(printer, DESTMAX, fp) != NULL){
                        if(*(c=printer+strlen(printer)-1) == '\n')
                                *c = '\0';
                        if(getpdest(&p, printer) == EOF)
                                continue;
                        else
                                if(p.p_default > default_max)
                                        default_max = p.p_default;
                }
                return(default_max);
        }
}
