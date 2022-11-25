/* @(#) $Revision: 70.1 $ */    
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef SIGTSTP
#include <bsdtty.h>
#endif /*SIGTSTP*/
#include "ptyrequest.h"

/* The code in this file is pretty ugly. It is a fairly complex state   */
/* machine necessitated by the fact that we need to continually process */
/* packets from the lan at the same time that we are prompting the user */
/* for information in vt command mode. It would be much better to impl- */
/* ement this part of the code with co-routines, but, alas, no-one has  */
/* taken the time to write the necessary stack manipulation code for HP */
/* machines.                                                            */

#define doprompt(n)  fprintf(stderr,"[%s] vt> ",n);

#define DEFAULT_SHELL "/bin/sh"
#define DEFAULT_ARG0  "sh"

extern int inmask,saveinmask;
extern int transmitflag;
extern int ounacked;
extern int escapeflag;
extern int breakflag;
extern int doiomode;
extern int nrexmit;
extern int gotpacket;

struct termio tv0,tv1,tv2,tv3; /* Used only by vt, but space is declared */
			       /* here so that the loader will be happy  */
			       /* when compiling vtserver.               */
#ifdef SIGTSTP
struct	ltchars	lt0, lt1;
#endif /*SIGTSTP*/

int childpid = -1;

/* vt command states */

#define PRINTPROMPT  0
#define WAITINPUT    1
#define ECINPUT      2
#define FTINPUT      3

/* vt printprompt sub-states */

#define INIT         0
#define WAITXMIT     1
#define WAITSWITCH   2

/* vt ftinput sub-states */

#define RFILEINPUT   0
#define LFILEINPUT   1
#define USERINPUT    2
#define PASSWDINPUT  3

#define RFILEPROMPT  "(remote file) "
#define LFILEPROMPT  "(local file) "
#define USERPROMPT   "User: "
#define PASSWDPROMPT "Password: "

static int state = PRINTPROMPT;

void vtcommand(), reset(), doshellescape(), printescape(), printhelp();
void filetransfer(), loguser(), printrexmit(), remotecd();
void dobufchange();
long sendftrequest();
struct vtstatresponse *waitforstatus();

extern char *strchr(), *getenv();
extern long doftrecvio(), doftsendio();
extern int  vt_time();

#define MAXARGS 10
#define VTCMDTIMEOUT 1500 /* hundredths of a second. */

void
vtcommand(lanfd,buf,nchars,nodename)
    int lanfd;
    char *buf;
    int nchars;
    char *nodename;
{
    char *vtargv[MAXARGS];
    char *s1,*s2;
    int nargs;
    static int ppstate = INIT;
    static int ftstate = -1;
    static int direction = -1;
    static int vtcmdstime;
    static char remotefile[MAXARGSTRLEN];
    static char localfile[MAXARGSTRLEN];
    static char user[USERLEN] = "";
    static char password[PASSWORDLEN] = "";
    extern int vtswitched;

    /* Just return if there was nothing input (i.e. the select in doio() */
    /* timed out or break has been hit.)                                 */

#ifdef DEBUG
    logerr("vtcommand: entered","");
#endif
    if (state != PRINTPROMPT && nchars == 0) {
	breakflag = FALSE; /* In case break was hit */
	return;
    }

    /* Null terminate input string, possibly truncating last character */

    if (nchars == MAXDATASIZE)
	nchars--;
    buf[nchars] = '\0';

    switch (state) {

    case PRINTPROMPT:

	/* We need to go through a somewhat complex sequence of events  */
	/* here to guarantee that the channel is clear in case the user */
	/* decides to do some file transfer.                            */

	switch (ppstate) {

	case INIT:

	    /* Set terminal to modified cooked mode */

	    (void) ioctl(0,TCSETAW,&tv2); /* prevents multiple escape */
	    inmask     = 0;
	    doiomode   = VTCMDMODE;
	    ppstate    = WAITXMIT;
	    vtcmdstime = vt_time();

	    /* intentional fall-through */

	case WAITXMIT:

	    if ((vt_time() - vtcmdstime) >= VTCMDTIMEOUT) {
		logerr("Connection Timeout","");
		vtexit();
	    }

	    if (transmitflag == FALSE)
		return;

	    /* Send switch packet */

	    if (send_packet(lanfd,(char *)0,0,VTSWITCH) == -1) {
		logerr("Error in sending vtswitch packet","");
		vtexit();
	    }

	    ppstate = WAITSWITCH;
	    return;

	case WAITSWITCH:

	    if ((vt_time() - vtcmdstime) >= VTCMDTIMEOUT) {
		logerr("Connection Timeout","");
		vtexit();
	    }

	    if (ounacked > 0 || vtswitched == FALSE)
		return;

	    vtswitched = FALSE; /* reset for next time */
	    ppstate    = INIT;  /* reset for next time */
	    inmask     = saveinmask; /* Restore inmask */

	    putc('\n',stdout);
	    doprompt(nodename);
	    state = WAITINPUT;
	    return;
	}
	break; /* Should never get here */

    case WAITINPUT:

	if (buf[nchars - 1] == '\n')
	    buf[--nchars] = '\0';

	if ((nargs = tokenize(buf,vtargv,MAXARGS)) == -1) {
	    doprompt(nodename);
	    return;
	}

	if (nargs == 0) {
	    reset(lanfd);
	    return;
	}

	switch(vtargv[0][0]) {

	case 'b':

	    /* Since this is currently an undocumented option, make sure */
	    /* that the whole command name was typed.                    */

	    if (strcmp(vtargv[0],"buffer") != 0) {

		fprintf(stdout,"Unrecognized command \"%s\". Try Again.\n",buf);
		doprompt(nodename);
		return;
	    }

	    if (nargs < 2)
		dobufchange(lanfd,0);
	    else
		dobufchange(lanfd,atoi(vtargv[1]));

	    doprompt(nodename);
	    state = WAITINPUT;
	    return;

	case 'c':
	    if (user[0] == '\0') {
		fprintf(stdout,"File transfer user not logged. Use the 'user' command to do so.\n");
		doprompt(nodename);
		return;
	    }

	    if (nargs == 1)
		remotecd(lanfd,"");
	    else
		remotecd(lanfd,vtargv[1]);

	    doprompt(nodename);
	    return;

	case 'e':
	    if (nargs == 1) {
		fputs("New escape character: ",stdout);
		state = ECINPUT;
		return;
	    }
	    tv1.c_cc[VQUIT] = vtargv[1][0];
	    printescape();
	    reset(lanfd);
	    return;

	case '?':
	case 'h':
	    printhelp();
	    doprompt(nodename);
	    return;

	case 'l':
	    if (nargs == 1) {
		if ((s1 = getenv("HOME")) == (char *)0) {
		    fputs("HOME environment variable undefined. Directory not changed.\n",
			  stdout);
		    doprompt(nodename);
		    return;
		}
	    }
	    else
		s1 = vtargv[1];

	    if (chdir(s1) != 0)
		fputs("Directory change failed.\n",stdout);
	    doprompt(nodename);
	    return;

	case 'q':
	    logerr("vt terminated.","");
	    vtexit();
	    break;

	case 'g':
	case 'p':
	case 'r':
	case 's':
	    if (user[0] == '\0') {
		fprintf(stdout,"File transfer user not logged. Use the 'user' command to do so.\n");
		doprompt(nodename);
		return;
	    }

	    if (vtargv[0][0] == 'r' || vtargv[0][0] == 'g')
		direction = FTRECEIVE;
	    else
		direction = FTSEND;

	    if (nargs < 3) {
		if ((direction == FTRECEIVE && nargs == 1)
		    || (direction == FTSEND && nargs == 2) ) {

		    if (nargs == 1)
			localfile[0] = '\0';
		    else
			strcpy(localfile,vtargv[1]);

		    fputs(RFILEPROMPT,stdout);
		    ftstate = RFILEINPUT;
		}
		else {

		    if (nargs == 1)
			remotefile[0] = '\0';
		    else
			strcpy(remotefile,vtargv[1]);

		    fputs(LFILEPROMPT,stdout);
		    ftstate = LFILEINPUT;
		}
		state = FTINPUT;
		return;
	    }
	    else {
		if (direction == FTSEND) {
		    strcpy(localfile,vtargv[1]);
		    strcpy(remotefile,vtargv[2]);
		}
		else {
		    strcpy(localfile,vtargv[2]);
		    strcpy(remotefile,vtargv[1]);
		}

		ftstate = -1;
		filetransfer(lanfd,direction,localfile,remotefile);
		doprompt(nodename);
		state = WAITINPUT;
		return;
	    }

	case 'u':
	    if (user[0] != '\0') {
		fprintf(stdout,"User already logged (Can't be changed).\n");
		doprompt(nodename);
		return;
	    }

	    if (nargs == 1) {
		fputs(USERPROMPT,stdout);
		ftstate = USERINPUT;
		state = FTINPUT;
		return;
	    }

	    s1 = s2 = vtargv[1];
	    if ((s2 = strchr(s1,':')) != (char *)0) {
		*s2++ = '\0';
		strncpy(user,s1,USERLEN);
		user[USERLEN - 1] = '\0';
		if (*s2 == '\0') {
		    (void) ioctl(0,TCSETAW,&tv3);
		    fputs(PASSWDPROMPT,stdout);
		    ftstate = PASSWDINPUT;
		    state = FTINPUT;
		    return;
		}

		strncpy(password,s2,PASSWORDLEN);
		password[PASSWORDLEN - 1] = '\0';
	    }
	    else {
		strncpy(user,s1,USERLEN);
		user[USERLEN - 1] = '\0';
		password[0] = '\0';
	    }

	    ftstate = -1;
	    loguser(lanfd,user,password);
	    doprompt(nodename);
	    state = WAITINPUT;
	    return;

	case '\036':
	    printrexmit(lanfd);
	    doprompt(nodename);
	    state = WAITINPUT;
	    return;

	case '!':
	    if (nargs == 1 && vtargv[0][1] == '\0')
		doshellescape(lanfd,"");
	    else {

		/* remove nulls inserted by tokenize in buf array */

		s1 = buf;
		s2 = vtargv[nargs - 1];
		while (s1 < s2) {
		    if (*s1 == '\0')
			*s1 = ' ';
		    s1++;
		}

		s1 = strchr(buf,'!');
		doshellescape(lanfd,s1 + 1);
	    }
	    return;

	default:
	    fprintf(stdout,"Unrecognized command \"%s\". Try Again.\n",buf);
	    doprompt(nodename);
	    return;

	}
	return; /* Should never reach here */

    case ECINPUT:
	if (buf[0] != '\n')
	    tv1.c_cc[VQUIT] = buf[0];
	printescape();
	reset(lanfd);
	return;

    case FTINPUT:
	if (buf[nchars - 1] == '\n')
	    buf[--nchars] = '\0';

	switch (ftstate) {

	case LFILEINPUT:
	    if (buf[0] == '\0') {
		ftstate = -1;
		state = WAITINPUT;
		doprompt(nodename);
		return;
	    }

	    buf[MAXARGSTRLEN - 1] = '\0';
	    strcpy(localfile,buf);

	    if (remotefile[0] == '\0') {
		fputs(RFILEPROMPT,stdout);
		ftstate = RFILEINPUT;
		return;
	    }

	    ftstate = -1;
	    filetransfer(lanfd,direction,localfile,remotefile);
	    doprompt(nodename);
	    state = WAITINPUT;
	    return;

	case RFILEINPUT:
	    if (buf[0] == '\0') {
		ftstate = -1;
		state = WAITINPUT;
		doprompt(nodename);
		return;
	    }

	    buf[MAXARGSTRLEN - 1] = '\0';
	    strcpy(remotefile,buf);

	    if (localfile[0] == '\0') {
		fputs(LFILEPROMPT,stdout);
		ftstate = LFILEINPUT;
		return;
	    }

	    ftstate = -1;
	    filetransfer(lanfd,direction,localfile,remotefile);
	    doprompt(nodename);
	    state = WAITINPUT;
	    return;

	case USERINPUT:
	    if (buf[0] == '\0') {
		ftstate = -1;
		state = WAITINPUT;
		doprompt(nodename);
		return;
	    }

	    s1 = s2 = buf;
	    if ((s2 = strchr(s1,':')) != (char *)0) {
		*s2++ = '\0';
		strncpy(user,s1,USERLEN);
		user[USERLEN - 1] = '\0';
		if (*s2 == '\0') {
		    (void) ioctl(0,TCSETAW,&tv3);
		    fputs(PASSWDPROMPT,stdout);
		    ftstate = PASSWDINPUT;
		    return;
		}

		strncpy(password,s2,PASSWORDLEN);
		password[PASSWORDLEN - 1] = '\0';
	    }
	    else {
		strncpy(user,s1,USERLEN);
		user[USERLEN - 1] = '\0';
		password[0] = '\0';
	    }

	    ftstate = -1;
	    loguser(lanfd,user,password);
	    doprompt(nodename);
	    state = WAITINPUT;
	    return;

	 case PASSWDINPUT:
	    (void) ioctl(0,TCSETAW,&tv2);
	    putc('\n',stdout);
	    buf[PASSWORDLEN - 1] = '\0';
	    strcpy(password,buf);

	    ftstate = -1;
	    loguser(lanfd,user,password);
	    doprompt(nodename);
	    state = WAITINPUT;
	    return;

	default:
	    logerr("Bad state reached in ftinput","");
	    vtexit();
	}
	return; /* Should never reach here */

    }
}

void
reset(lanfd)
    int lanfd;
{
	/* Set up state for return to VTMODE */

	escapeflag = FALSE;
	state = PRINTPROMPT;
	inmask = saveinmask;
	doiomode = VTMODE;
	ioctl(0,TCSETAW,&tv1);
#ifdef SIGTSTP
	ioctl(0, TIOCSLTC, &lt1);
#endif /*SIGTSTP*/

	/* Send switch packet (transmitflag should be true at this point) */

	if (send_packet(lanfd,(char *)0,0,VTSWITCH) == -1) {
	    logerr("Error in sending vtswitch packet","");
	    vtexit();
	}

	return;
}



void
doshellescape(lanfd,cmdline)
    int lanfd;
    char *cmdline;
{
    struct sigvec vec;
    char *shell;
    char *shellarg0;
    extern char *getenv();
    extern char *strrchr();


    if ((childpid = fork()) == -1) {
	fprintf(stderr,"Shell escape failed (fork failure).\n");
	reset(lanfd);
	return;
    }

    if (childpid != 0) {

	/* Parent */

	/* Ignore SIGINT and SIGQUIT while shell escape is in progress */

	setvec(vec, SIG_IGN);
	(void) sigvector(SIGINT,  &vec, (struct sigvec *)0);
	(void) sigvector(SIGQUIT, &vec, (struct sigvec *)0);

	/* Set up state for shell escape */

	escapeflag = FALSE;
	state = PRINTPROMPT;
	inmask = 0;

	return;
    }

    /* Child */

    /* Restore signals to default state */

    setvec(vec, SIG_DFL);
    (void) sigvector(SIGINT,  &vec, (struct sigvec *)0);
    (void) sigvector(SIGQUIT,  &vec, (struct sigvec *)0);
    sigsetmask(0L);

    /* reset terminal to original modes */

    (void) ioctl(0,TCSETAW,&tv0);
#ifdef SIGTSTP
	ioctl(0, TIOCSLTC, &lt0);
#endif /*SIGTSTP*/

    /* unset ndelay mode */

    unsetndelay(0);

    if ((shell = getenv("SHELL")) == (char *)0)
	shell = DEFAULT_SHELL;

    if ((shellarg0 = strrchr(shell,'/')) == (char *)0)
	shellarg0 = shell;
    else
	shellarg0++;

    if (*cmdline == '\0') {
	execl(shell,shellarg0,(char *)0);
	execl(DEFAULT_SHELL,DEFAULT_ARG0,(char *)0);
    }
    else {
	execl(shell,shellarg0,"-c",cmdline,(char *)0);
	execl(DEFAULT_SHELL,DEFAULT_ARG0,"-c",cmdline,(char *)0);
    }
    fprintf(stderr,"Shell escape failed (exec failure).\n");
    _exit(1);
}

void
remotecd(lanfd,remotedir)
    int lanfd;
    char *remotedir;
{
    static struct vtftrequest vtftrq;

    /* Truncate remote directory if it is greater than MAXARGSTRLEN */

    remotedir[MAXARGSTRLEN - 1] = '\0';

    vtftrq.ftreqtype = FTCD;
    strcpy(vtftrq.filename,remotedir);

    /* Send ft request */

    (void) sendftrequest(lanfd,(char *)0,(int *)0,(int *)0,&vtftrq);
    return;
}
void
loguser(lanfd,user,password)
    int lanfd;
    char *user;
    char *password;
{
    static struct vtftrequest vtftrq;

    vtftrq.ftreqtype = FTLOGUSER;
    strcpy(vtftrq.user,user);
    strcpy(vtftrq.password,password);

    /* Send ft request */

    if (sendftrequest(lanfd,(char *)0,(int *)0,(int *)0,&vtftrq) < 0)
	user[0] = '\0'; /* Reset so user can try again */

    return;
}

void
dobufchange(lanfd,newbufsize)
    int lanfd;
    int newbufsize;
{
    static struct vtftrequest vtftrq;
    int remoterexmit;
    int minorversion;

    vtftrq.ftreqtype = FTCHGBUFSIZE;
    vtftrq.filesize  = newbufsize;

    /* Send ft request */

    if (sendftrequest(lanfd,(char *)0,&newbufsize,(int *)0,&vtftrq) < 0)
	return;

    fprintf(stdout,"Remote buffer size = %d\n",newbufsize);
    return;
}
void
printrexmit(lanfd)
    int lanfd;
{
    static struct vtftrequest vtftrq;
    int remoterexmit;
    int minorversion;

    vtftrq.ftreqtype = FTGETREXMIT;

    /* Send ft request */

    if (sendftrequest(lanfd,(char *)0,&remoterexmit,&minorversion,&vtftrq) < 0)
	return;

    fprintf(stdout,"# of local  rexmits  = %d\n",nrexmit);
    fprintf(stdout,"# of remote rexmits  = %d\n",remoterexmit);
    fprintf(stdout,"Remote minor version = %d\n",minorversion);
    return;
}

void
filetransfer(lanfd,direction,localfile,remotefile)
    int lanfd;
    int direction;
    char *localfile;
    char *remotefile;
{
    static struct vtftrequest vtftrq;
    struct vtstatresponse *vtstrptr;
    int ftfd;
    struct stat sbuf;
    int starttime,endtime;
    long ntransfered;
    int tenths;

    vtftrq.ftreqtype = direction;
    strcpy(vtftrq.filename,remotefile);

    breakflag = FALSE; /* If breakflag becomes true then file transfer */
		       /* was aborted.                                 */

    if (direction == FTSEND) {

	if ((ftfd = open(localfile,O_RDONLY)) < 0) {
	    logerr("Could not open local file for reading.","");
	    return;
	}

	if (fstat(ftfd,&sbuf) != 0) {
	    close(ftfd);
	    logerr("Could not stat local file.","");
	    return;
	}

	vtftrq.filesize = sbuf.st_size;
	vtftrq.modes    = sbuf.st_mode & 07777;

	/* Send ft request */

	if (sendftrequest(lanfd,localfile,(int *)0,(int *)0,&vtftrq) < 0) {
	    close(ftfd);
	    return;
	}

	/* Do the actual file tranfer */

	starttime = vt_time();
	ntransfered = doftsendio(lanfd,ftfd);
    }
    else {

	if ((ftfd = open(localfile,(O_WRONLY|O_TRUNC|O_CREAT),0644)) < 0) {
	    logerr("Could not open/create local file for writing.","");
	    return;
	}

	vtftrq.filesize = 0;
	vtftrq.modes    = 0;

	/* Send ft request */

	if (sendftrequest(lanfd,remotefile,(int *)0,(int *)0,&vtftrq) < 0) {
	    close(ftfd);
	    return;
	}

	/* Do the actual file tranfer */

	starttime = vt_time();
	ntransfered = doftrecvio(lanfd,ftfd);
    }

    endtime = vt_time();
    close(ftfd);

    /* Wait for status response */

    vtstrptr = waitforstatus(lanfd);

    if (breakflag == TRUE) {
	fprintf(stdout,"File transfer aborted!\n");
	return;
    }

    if (vtstrptr->error == TRUE) {
	logerr("File transfer failed:",vtstrptr->errormsg);
	return;
    }

    if (vtstrptr->arg1 != ntransfered) {
	logerr("File transfer failed: # bytes sent != # bytes received","");
	return;
    }

    tenths = (endtime - starttime) / 10;

    if (tenths == 0) {
	fprintf(stdout,"Transferred %d bytes\n",ntransfered);
    }
    else {
	fprintf(stdout,"Transferred %d bytes in %d.%d seconds (%d Kbytes/sec)\n",
			ntransfered,tenths/10,tenths % 10,
			(ntransfered + tenths * 50)/(tenths * 100) );
    }
    return;
}

long
sendftrequest(lanfd,filename,arg1,arg2,vtftrqptr)
    int lanfd;
    char *filename;
    int *arg1;
    int *arg2;
    struct vtftrequest *vtftrqptr;
{
    struct vtstatresponse *vtstrptr;
    int reqtype;

    reqtype = vtftrqptr->ftreqtype;

    if (send_packet(lanfd,(char *)vtftrqptr,
		    sizeof(struct vtftrequest),VTFTREQUEST) == -1) {
	logerr("Error in sending file transfer request packet","");
	vtexit();
    }

    vtstrptr = waitforstatus(lanfd);

    if (vtstrptr->error == TRUE) {
	logerr("Error:",vtstrptr->errormsg);
	return (-1);
    }

    if (reqtype == FTRECEIVE || reqtype == FTSEND) {
	if (reqtype == FTRECEIVE)
	    chmod(filename,vtstrptr->arg2);

	fprintf(stdout,"Copying File....\n");
    }

    /* Copy results if desired */

    if (arg1 != (int *)0)
	*arg1 = vtstrptr->arg1;

    if (arg2 != (int *)0)
	*arg2 = vtstrptr->arg2;

    return(0);
}

struct vtstatresponse *
waitforstatus(lanfd)
    int lanfd;
{
    int nread;
    int gotstatus;
    static struct vtstatresponse vtstr;
    struct timeval timeout;
    int ftlanmask;
    int ftlanfds;
    int nfds;

    nfds = lanfd + 1;
    ftlanmask = (1 << lanfd);

    timeout.tv_sec  = 0;
    timeout.tv_usec = SELECT_TIMEOUT;

    gotstatus = FALSE;
    do {
	ftlanfds = ftlanmask;
	if (select(nfds,&ftlanfds,(int *)0,(int *)0,&timeout) == -1) {
	    logerr("Select failed","");
	    vtexit();
	}

	if (ftlanfds != 0) {
	    do {
		if ((nread = read_packet(-1,lanfd,(char *)&vtstr)) == -1) {
		    logerr("Read failed on LAN fd","");
		    vtexit();
		}

		if (nread > 0) {
		    if (nread != sizeof (struct vtstatresponse)) {
			logerr("Partial packet read during file transfer","");
			vtexit();
		    }
		    gotstatus = TRUE;
		}

		if (gotpacket == TRUE) {
		    if (send_packet(lanfd,(char *)0,0,VTACK) == -1) {
			logerr("Error in sending ack packet.","");
			vtexit();
		    }
		}
	    } while (gotpacket == TRUE && gotstatus == FALSE);
	}

	if (ounacked > 0)
	    resend_packets(lanfd);

    } while (gotstatus == FALSE);

    return(&vtstr);
}

void
printescape()
{
    int escape_char;

    escape_char = tv1.c_cc[VQUIT];
    fputs("Escape Character is '",stdout);

    if ((escape_char >= 0200 && escape_char <= 0240) || escape_char == 0377) {
	putc('M',stdout);
	putc('-',stdout);
	escape_char -= 0200;
    }

    if (escape_char < 040) {
	putc('^',stdout);
	escape_char += 0100;
    }

    switch (escape_char) {

    case ' ':
	fputs("(space)",stdout);
	break;

    case 0177:
	fputs("DEL",stdout);
	break;

    default:
	putc(escape_char,stdout);
	if (escape_char > 0200) {
	    fputs(" (M-",stdout);
	    putc((escape_char - 0200),stdout);
	    putc(')',stdout);
	}
	break;
    }

    fputs("'.\n",stdout);
    return;
}

void
printhelp()
{
    fputs("Commands may be abbreviated. Commands are:\n\n",stdout);
    fputs("Command  Syntax                                Description\n",stdout);
    fputs("--------------------------------------------------------------------------\n",stdout);
    fputs("cd       c[d] [<remote directory>]             Change remote directory.\n",stdout);
    fputs("escape   e[scape] [<new escape character>]     Change vt escape character.\n",stdout);
    fputs("get      g[et] <remotefile> <localfile>        Receive a file from remote.\n",stdout);
    fputs("help,?   h[elp] or ?                           Print this help menu.\n",stdout);
    fputs("lcd      l[cd] [<local directory>]             Change local directory.\n",stdout);
    fputs("put      p[ut] <localfile> <remotefile>        Send a file to remote.\n",stdout);
    fputs("quit     q[uit]                                Terminate vt.\n",stdout);
    fputs("receive                                        Synonym for get.\n",stdout);
    fputs("send                                           Synonym for put.\n",stdout);
    fputs("user     u[ser] <username>[:[<password>]]      Log user for file transfer.\n",stdout);
    fputs("!        ! [<command and arguments>]           Shell escape.\n",stdout);
    fputs("<return>                                       Return to remote mode.\n",stdout);
    fputs("\nNOTE: The cd and lcd commands are only applicable for file transfers.\n",stdout);
    return;
}

tokenize(line,pargv,maxargs)
    char *line;
    char **pargv;
    int  maxargs;
{
    char *lptr;
    int pargc;

    pargc = 0;
    lptr = line;
    while (*lptr) {
	while (*lptr == ' ' || *lptr == '\t' )
	    *lptr++ = '\0';
	if (*lptr) {
	    if (pargc >= maxargs - 1) {
		fprintf(stderr,"Too many arguments.\n");
		return(-1);
	    }
	    pargv[pargc] = lptr;
	    while (*lptr != ' ' && *lptr != '\t' && *lptr != '\0')
		lptr++;

	    if (*lptr != '\0')
		*lptr++ = '\0';

	    if (strlen(pargv[pargc++]) >= MAXARGSTRLEN) {
		fprintf(stderr,"Argument too long.\n");
		return(-1);
	    }
	}
    }
    pargv[pargc] = (char *)0;
    return(pargc);
}
